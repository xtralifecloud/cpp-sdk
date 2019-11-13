
#ifndef XtraLife_CDelegate_h
#define XtraLife_CDelegate_h

#include "XtraLife.h"
#include "CFastDelegate.h"
#include "XtraLifeHelpers.h"

#pragma warning(disable:4355)
namespace XtraLife {

    namespace Helpers {
        class CHJSON;
    }

	/**
		Used to manage the data returned by a callback.
		Typically a result always has an Error Code and  usually a JSON struct if the request
		returns data, but it can be NULL in case of error.
	*/
	class FACTORY_CLS CCloudResult {
	public:
		~CCloudResult();
		/**
			The error code of the called method; see enum eErrorCode for more details
			@return an Error Code.
			To get a message corresponding to the error, use the array eErrorString[ec].
		*/
		eErrorCode GetErrorCode() const;

		/**
			The error message of the called method; see enum eErrorCode for more details
			@return a string.
		*/
		const char *GetErrorString() const;

		/**
			For an HTTP request, returns the HTTP code (may be 0 in case the request failed).
		*/
		int GetHttpStatusCode() const;
		
		int GetCurlErrorCode() const;
		
		/**
			Returns a string which reflects the content of the JSON class.
			@result a JSON formatted string; if JSON class is NULL, the string is NULL.
		*/
		Helpers::cstring GetJSONString() const;
		
		/**
			Returns the JSON value associated to "result" key, which holds data pertaining to
			the original request; see CHJSON for more informations; can be NULL.
			@return a JSON class.
		 */
		const Helpers::CHJSON *GetJSON() const;

		bool HasBinary() const { return mHasBinary;}
		size_t BinarySize() const { return mSize; }
		const void *BinaryPtr() const;

		bool IsObsolete() const { return mObsolete; }
		
		/**
			@return a copy of this object, to be deleted by you.
		*/
		CCloudResult *Duplicate() const;
		
		/**
			You shouldn't need these
		*/
		CCloudResult();
		CCloudResult(eErrorCode err);
		CCloudResult(eErrorCode err, const char *message);
		CCloudResult(eErrorCode err, Helpers::CHJSON *ajson);
		// Do not use, supposes that the error code is already inside
		CCloudResult(Helpers::CHJSON *ajson);
	
		void SetErrorCode(eErrorCode err);
		void SetCurlErrorCode(int err);
		void SetHttpStatusCode(int err);
		void SetBinary(void *buffer, size_t size);
		void SetObsolete(bool obsolete) { mObsolete = obsolete; }

		/**
		 * Offers a pretty-printed view of this result, including the JSON inside when available.
		 * @return a string with tabs, line endings.
		 */
		Helpers::cstring Print() const;

	private:
		struct DataHolder;
		Helpers::CHJSON  *mJson;
		DataHolder *mBinary;
		size_t  mSize;
		bool    mHasBinary, mObsolete;
	};

	/**
	 * Result handler for asynchronous methods.
	 * Create either with:
	 * - new CResultHandler(this, &Object::myMethod);
	 * - MakeResultHandler(this, &Object::myMethod, parameter)
	 *
	 * And make it point to a method taking an eErrorCode and a const CCloudResult (and a parameter if you use the second variant).
	 * E.g.:
	 * void Object::Method(eErrorCode errorCode, const CCloudResult *result, int param) {
	 * }
	 * -> MakeResultHandler(this, &Object::Method, 123);
	 */
	typedef CDelegate<void (eErrorCode errorCode, const CCloudResult *result)> CResultHandler;

	/**
	 * Builds a CResultHandler.
	 * @param object object to which the method belongs.
	 * @param method the method to be called on the object
	 * @return a CResultHandler to pass to various API calls
	 */
	template<class O>
	CResultHandler *MakeResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*)) {
		return new CResultHandler(object, method);
	}

	/**
	 * Builds a CResultHandler with additional parameters.
	 * @param object object to which the method belongs.
	 * @param method the method to be called on the object
	 * @param param additional parameter; a parameter with the same type as the one passed should be accept as last argument by the method
	 * @return a CResultHandler to pass to various API calls
	 * @note Other variants of this method accepting up to 3 parameters exist, and use the same schema.
	 */
	template<class O, typename T>
	CResultHandler *MakeResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T), T param) {
		struct ParameterizedResultHandler: CResultHandler {
			T param; O *obj; void(O::*method)(eErrorCode, const CCloudResult*, T);
			ParameterizedResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T), T param)
				: obj(object), method(method), param(param), CResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(eErrorCode code, const CCloudResult *result) {
				(obj->*method)(code, result, param);
			}
		};
		return new ParameterizedResultHandler(object, method, param);
	}

	template<class O, typename T, typename U>
	CResultHandler *MakeResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T, U), T p1, U p2) {
		struct ParameterizedResultHandler: CResultHandler {
			T p1; U p2; O *obj; void(O::*method)(eErrorCode, const CCloudResult*, T, U);
			ParameterizedResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T, U), T p1, U p2)
				: obj(object), method(method), p1(p1), p2(p2), CResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(eErrorCode code, const CCloudResult *result) {
				(obj->*method)(code, result, p1, p2);
			}
		};
		return new ParameterizedResultHandler(object, method, p1, p2);
	}

	template<class O, typename T, typename U, typename V>
	CResultHandler *MakeResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T, U, V), T p1, U p2, V p3) {
		struct ParameterizedResultHandler: CResultHandler {
			T p1; U p2; V p3; O *obj; void(O::*method)(eErrorCode, const CCloudResult*, T, U, V);
			ParameterizedResultHandler(O *object, void(O::*method)(eErrorCode, const CCloudResult*, T, U, V), T p1, U p2, V p3)
				: obj(object), method(method), p1(p1), p2(p2), p3(p3), CResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(eErrorCode code, const CCloudResult *result) {
				(obj->*method)(code, result, p1, p2, p3);
			}
		};
		return new ParameterizedResultHandler(object, method, p1, p2, p3);
	}

	/** \cond INTERNAL_USE */
	extern inline void InvokeHandler(CResultHandler *handler, const CCloudResult *result) {
		if (handler) {
			handler->Invoke(result->GetErrorCode(), result);
			delete handler;
		}
	}

	extern inline void InvokeHandler(CResultHandler *handler, eErrorCode code, const char *message = NULL) {
		if (handler) {
			CCloudResult result(code, message);
			handler->Invoke(code, &result);
			delete handler;
		}
	}

	typedef CDelegate<void (const CCloudResult *)> CInternalResultHandler;

	template<class O, typename T>
	CInternalResultHandler *MakeInternalResultHandler(O *object, void(O::*method)(const CCloudResult*, T), T p1) {
		struct ParameterizedResultHandler: CInternalResultHandler {
			T p1; O *obj; void(O::*method)(const CCloudResult*, T);
			ParameterizedResultHandler(O *object, void(O::*method)(const CCloudResult*, T), T p1)
			: obj(object), method(method), p1(p1), CInternalResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(const CCloudResult *result) {
				(obj->*method)(result, p1);
			}
		};
		return new ParameterizedResultHandler(object, method, p1);
	}
	

	template<class O, typename T, typename U>
	CInternalResultHandler *MakeInternalResultHandler(O *object, void(O::*method)(const CCloudResult*, T, U), T p1, U p2) {
		struct ParameterizedResultHandler: CInternalResultHandler {
			T p1; U p2; O *obj; void(O::*method)(const CCloudResult*, T, U);
			ParameterizedResultHandler(O *object, void(O::*method)(const CCloudResult*, T, U), T p1, U p2)
			: obj(object), method(method), p1(p1), p2(p2), CInternalResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(const CCloudResult *result) {
				(obj->*method)(result, p1, p2);
			}
		};
		return new ParameterizedResultHandler(object, method, p1, p2);
	}

	template<class O, typename T, typename U, typename V>
	CInternalResultHandler *MakeInternalResultHandler(O *object, void(O::*method)(const CCloudResult*, T, U, V), T p1, U p2, V p3) {
		struct ParameterizedResultHandler: CInternalResultHandler {
			T p1; U p2; V p3; O *obj; void(O::*method)(const CCloudResult*, T, U, V);
			ParameterizedResultHandler(O *object, void(O::*method)(const CCloudResult*, T, U, V), T p1, U p2, V p3)
			: obj(object), method(method), p1(p1), p2(p2), p3(p3), CInternalResultHandler(this, &ParameterizedResultHandler::Done) {}
			void Done(const CCloudResult *result) {
				(obj->*method)(result, p1, p2, p3);
			}
		};
		return new ParameterizedResultHandler(object, method, p1, p2, p3);
	}

	/** \endcond */
}
#pragma warning(default:4355)

#endif
