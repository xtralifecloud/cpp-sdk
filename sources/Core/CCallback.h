//
//  CCallback.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 28/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CCallback_h
#define XtraLife_CCallback_h

#include "CDelegate.h"
#include "CFastDelegate.h"
#include "XtraLifeHelpers.h"
#include "XtraLife_thread.h"

namespace XtraLife {
	
	/**
	 * Standard way of calling something on the main thread.
		CCallback *callback = MakeDelegate(myGame, &MyGame::Method);
		CallbackStack::pushCallback(callback, new CCloudResult(enNoErr));
	 */
	typedef CDelegate<void (const CCloudResult*)> CCallback;
	
	class CallbackStack {
	public:
		CallbackStack(CCallback *aCall, CCloudResult *aResult) { call = aCall; result = aResult; next = NULL; }
		~CallbackStack();

		// Returns whether a callback was actually executed
		static bool popCallback();
		static void pushCallback(CCallback *aCall, CCloudResult *aResult);
		// Dangerous! Removes any pending callback but doesn't call them. May cause memory leaks and
		// logic errors for any code relying on these callbacks. Only perform that at termination.
		static void removeAllPendingCallbacksWithoutCallingThem();

		CallbackStack 	*next;
		static CallbackStack *gHead;
		static Helpers::CMutex gStackMutex;
		static bool gCallbackQueueHandledByRabbitFactory;
	protected:
		CCallback 		*call;
		CCloudResult *result;

	};

	class CThreadCloud : public XtraLife::Helpers::CThread {
	public:
		CThreadCloud();
		~CThreadCloud();
		Helpers::CHJSON *mJSON;
		bool mBool;
		void run(const char *webmethod=NULL);
		virtual CCloudResult *execute() = 0;
		virtual void done(const CCloudResult *cr)=0;
	private:
		virtual void Run();
		virtual void do_done(const CCloudResult *cr);
	};
	
	#define DECLARE_TASK(MANAGER, NAME) \
		class NAME: public CThreadCloud { public : \
		NAME(MANAGER *m)  : CThreadCloud() { mBool = false; NAME::self = m; mUserReference=NULL; mBinaryData = NULL; mBinarySize = 0; mBinaryRelease = false; mOptions = 0; }; \
		CCloudResult* execute(); \
		void done(const CCloudResult *res); \
		MANAGER *self; void *mUserReference; void *mBinaryData; unsigned long mBinarySize; bool mBinaryRelease; int mOptions; };


	#define TASK(NAME) \
		CThreadCloud *task = new NAME(this);
		
	#define TASKM(NAME, m) \
		CThreadCloud *task = new NAME(m);

	/** Internal use only */
	CCloudResult *Invoke(CCallback *call, CCloudResult *result);

	//////////////////////////// Private CDelegate-related stuff ////////////////////////////
	/**
	 * Used to hold a global delegate that will be called later.
	 * Not a good practice, but sometimes you lose control over your objects (typically if you go through JNI or such) so you might want to hold a callback for later.
	 * The callback is not called upon destruction. If you want that, you need to call .Set(NULL).
		static CGloballyKeptHandler<CResultHandler> handler;
		handler.Set(myHandler);
		handler.Set(otherHandler);		// will send an enCanceled to myHandler
		handler.Invoke(enNoErr);		// invokes otherHandler
		handler.Set(yetAnotherHandler);	// will just assign the new handler
	 */
	template<class DelegateType>
	struct CGloballyKeptHandler {
		DelegateType *delegate;
		CGloballyKeptHandler() : delegate(NULL) {}
		DelegateType *Set(DelegateType *other) {
			if (delegate) { InvokeHandler(delegate, enCanceled); }
			delegate = other;
			return NULL;
		}
		bool IsSet() { return delegate != NULL; }
		void Invoke(eErrorCode code) {
			InvokeHandler(delegate, code);
			delegate = NULL;
		}
		void Invoke(eErrorCode code, const char *message) {
			InvokeHandler(delegate, code, message);
			delegate = NULL;
		}
		void Invoke(const CCloudResult *result) {
			InvokeHandler(delegate, result);
			delegate = NULL;
		}
	};

	//////////////////////////// Handler invokers ////////////////////////////
	// Finding all references to InvokeHandlers with other than an error code: InvokeHandler\([^,]*, (?!en)|Invoke\((?!en)
	extern inline void InvokeHandler(CInternalResultHandler *handler, eErrorCode code, const char *message = NULL) {
		if (handler) {
			CCloudResult result(code, message);
			handler->Invoke(&result);
			delete handler;
		}
	}

	extern inline void InvokeHandler(CInternalResultHandler *handler, const CCloudResult *result) {
		if (handler) {
			handler->Invoke(result);
			delete handler;
		}
	}

	extern inline void InvokeHandler(CDelegate<void(eErrorCode)> *handler, eErrorCode code) {
		if (handler) {
			handler->Invoke(code);
			delete handler;
		}
	}

	//////////////////////////// Blocks ////////////////////////////
#	pragma warning(disable:4355)
	/**
	 * Allows to create a "closure" by capturing surrounding variables. Typically works this way:
		 struct MyCallback: CCallback {
			 _BLOCK1(MyCallback, CCallback, int, param);
			 void Done(const CCloudResult *result) {
				 printf("Was invoked with param = %d", param);
			 }
		 };
		 CCallback *c = new MyCallback(123);
	 */
	#define	_BLOCK0(cls, base)  cls() : base(this, &cls::Done) {}
	#define	_BLOCK1(cls, base, p1, v1)  p1 v1; cls(p1 v1) : v1(v1), base(this, &cls::Done) {}
	#define	_BLOCK2(cls, base, p1, v1, p2, v2)  p1 v1; p2 v2; cls(p1 v1, p2 v2) : v1(v1), v2(v2), base(this, &cls::Done) {}
	#define	_BLOCK3(cls, base, p1, v1, p2, v2, p3, v3)  p1 v1; p2 v2; p3 v3; cls(p1 v1, p2 v2, p3 v3) : v1(v1), v2(v2), v3(v3), base(this, &cls::Done) {}
	#define	_BLOCK4(cls, base, p1, v1, p2, v2, p3, v3, p4, v4)  p1 v1; p2 v2; p3 v3; p4 v4; cls(p1 v1, p2 v2, p3 v3, p4 v4) : v1(v1), v2(v2), v3(v3), v4(v4), base(this, &cls::Done) {}

	//////////////////////////// Bridge delegates ////////////////////////////
	/**
	 * Creates a standard delegate that will simply call the given method on an object.
	 * This is useful if you want to simply forward the result of the call to another method.
	 * @param obj instance on which to call the method
	 * @param method method to call (should be something overloadable by the client, that is virtual)
	 * @param handler handler invoked after the method
	 * @return an internal delegate
	 */
	template<class Obj>
	extern inline CInternalResultHandler *MakeBridgeDelegate(Obj *obj, void (Obj::*method)(CCloudResult*), CResultHandler *handler) {
		struct ContextDelegate: CInternalResultHandler {
			Obj *self;
			void (Obj::*method)(CCloudResult*);
			CResultHandler *handler;
			ContextDelegate(Obj *self, void (Obj::*method)(CCloudResult*), CResultHandler *handler) : self(self), method(method), handler(handler), CDelegate(this, &ContextDelegate::Invoke) {}
			void Invoke(const CCloudResult *result) {
				if (method) {
					(self->*method)((CCloudResult*) result);
				}
				InvokeHandler(handler, result);
			}
		};
		return new ContextDelegate(obj, method, handler);
	}
	/**
	 * Variant for an internal result handler.
	 */
	template<class Obj>
	extern inline CInternalResultHandler *MakeBridgeDelegate(Obj *obj, void (Obj::*method)(CCloudResult*), CInternalResultHandler *handler) {
		struct ContextDelegate: CInternalResultHandler {
			Obj *self;
			void (Obj::*method)(CCloudResult*);
			CInternalResultHandler *handler;
			ContextDelegate(Obj *self, void (Obj::*method)(CCloudResult*), CInternalResultHandler *handler) : self(self), method(method), handler(handler), CDelegate(this, &ContextDelegate::Invoke) {}
			void Invoke(const CCloudResult *result) {
				if (method) {
					(self->*method)((CCloudResult*) result);
				}
				InvokeHandler(handler, result);
			}
		};
		return new ContextDelegate(obj, method, handler);
	}
	template<class Obj>
	extern inline CInternalResultHandler *MakeBridgeDelegate(Obj *obj, void (Obj::*method)(CCloudResult*)) {
		return MakeBridgeDelegate(obj, method, (CInternalResultHandler*) 0);
	}
	template<class Obj>
	extern inline CInternalResultHandler *MakeBridgeDelegate(Obj *obj, CCloudResult* (Obj::*method)(CCloudResult*), CInternalResultHandler *handler) {
		struct ContextDelegate: CInternalResultHandler {
			Obj *self;
			CCloudResult* (Obj::*method)(CCloudResult*);
			CInternalResultHandler *onFinished;
			ContextDelegate(Obj *self, CCloudResult* (Obj::*method)(CCloudResult*), CInternalResultHandler *onFinished) : self(self), method(method), onFinished(onFinished), CDelegate(this, &ContextDelegate::Invoke) {}
			void Invoke(const CCloudResult *result) {
				if (method) {
					CCloudResult *res = ((self->*method)((CCloudResult*) result));
					InvokeHandler(onFinished, res);
					if (res != result) { delete res; }
				} else {
					InvokeHandler(onFinished, result);
				}
				delete this;
			}
		};
		return new ContextDelegate(obj, method, handler);
	}
	extern inline CInternalResultHandler *MakeBridgeDelegate(CResultHandler *handler) { return MakeBridgeDelegate<CResultHandler>(NULL, NULL, handler); }
	template<class Obj>
	extern inline CDelegate<void(eErrorCode)> *MakeBridgeDelegate(Obj *obj, void (Obj::*method)(const eErrorCode)) {
		typedef void (Obj::*method_t)(eErrorCode);
		struct ContextDelegate: CDelegate<void(eErrorCode)> {
			_BLOCK2(ContextDelegate, CDelegate,
				Obj*, self,
				method_t, method);
			void Done(eErrorCode code) {
				if (method) {
					(self->*method)(code);
				}
			}
		};
		return new ContextDelegate(obj, method);
	}
	template<class Obj>
	extern inline CInternalResultHandler *MakeBridgeDelegate(Obj *obj, eErrorCode (Obj::*method)(eErrorCode, const Helpers::CHJSON*)) {
		typedef eErrorCode (Obj::*method_t)(eErrorCode, const Helpers::CHJSON*);
		struct ContextDelegate: CInternalResultHandler {
			_BLOCK2(ContextDelegate, CDelegate,
				Obj*, self,
				method_t, method);
			void Done(const CCloudResult *result) {
				if (method) {
					(self->*method)(result->GetErrorCode(), result->GetJSON());
				}
			}
		};
		return new ContextDelegate(obj, method);
	}
	template<class Obj>
	extern inline CCallback *MakeBridgeCallback(Obj *obj, CCloudResult* (Obj::*method)(CCloudResult*), CInternalResultHandler *handler) {
		struct ContextCallback {
			Obj *self;
			CCloudResult* (Obj::*method)(CCloudResult*);
			CInternalResultHandler *handler;
			ContextCallback(Obj *self, CCloudResult* (Obj::*method)(CCloudResult*), CInternalResultHandler *handler) : self(self), method(method), handler(handler) {}
			void Invoke(const CCloudResult *result) {
				if (method) {
					CCloudResult *res = ((self->*method)((CCloudResult*) result));
					InvokeHandler(handler, res);
					if (res != result) {
						delete res;
					}
				} else {
					InvokeHandler(handler, result);
				}
				delete this;
			}
		};
		return MakeDelegate(new ContextCallback(obj, method, handler), &ContextCallback::Invoke);
	}
	extern inline CInternalResultHandler *MakeBridgeCallback(CInternalResultHandler *handler) { return MakeBridgeCallback<CResultHandler>(NULL, NULL, handler); }
}

#endif
