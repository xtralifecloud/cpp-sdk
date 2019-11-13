//
//  CCallback.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 27/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#include <stdlib.h>

#include "CCallback.h"
#include "CHJSON.h"

using namespace XtraLife::Helpers;

/**
 * Important note on CCloudResult: its mJson member shall NEVER be NULL.
 */
namespace XtraLife 
{
	CallbackStack *CallbackStack::gHead = NULL;
	CMutex CallbackStack::gStackMutex;
	bool CallbackStack::gCallbackQueueHandledByRabbitFactory = true;
	
	CallbackStack::~CallbackStack() { delete call; delete result;}
	
	void CallbackStack::pushCallback(CCallback *call, CCloudResult *result)
	{		
		CallbackStack *ts = new CallbackStack(call, result);
		CMutex::ScopedLock lock(CallbackStack::gStackMutex);
		if (CallbackStack::gHead == NULL)
			CallbackStack::gHead = ts;
		else
		{
			CallbackStack *p = CallbackStack::gHead;
			while (p->next != NULL)
				p = p->next;
			p->next = ts;
		}
	}
	
	bool CallbackStack::popCallback() {	
		CallbackStack *p = NULL;

		gStackMutex.Lock();
		if (CallbackStack::gHead != NULL) {
			p  = CallbackStack::gHead;
			CallbackStack::gHead = p->next;
		}
		gStackMutex.Unlock();
		
		if (p) {
			p->call->Invoke(p->result);
			delete p;
		}
		return p != NULL;
	}

	void CallbackStack::removeAllPendingCallbacksWithoutCallingThem() {
		gStackMutex.Lock();
		while (CallbackStack::gHead != NULL) {
			CallbackStack *callback = CallbackStack::gHead;
			CallbackStack::gHead = CallbackStack::gHead->next;
			delete callback;
		}
		gStackMutex.Unlock();
	}
	
	// Ref-counted data holder (holds data and frees it when it gets destroyed; that is the ref count drops to zero).
	struct CCloudResult::DataHolder : CRefClass {
		void *ptr;
		DataHolder(void *ownedData) : ptr(ownedData) {}
		~DataHolder() { if (ptr) { free(ptr); } }
	};

	cstring CCloudResult::GetJSONString() const {
		return mJson->print();
	}
	
	const CHJSON *CCloudResult::GetJSON() const {
		return mJson;
	}
	
	eErrorCode CCloudResult::GetErrorCode() const {
		return (eErrorCode) mJson->GetInt("_error");
	}
	
	const char* CCloudResult::GetErrorString() const {
		return errorString(this->GetErrorCode());
	}

	int CCloudResult::GetHttpStatusCode() const {
		return mJson->GetInt("_httpcode");
	}

	int CCloudResult::GetCurlErrorCode() const {
		return mJson->GetInt("_curlerror");
	}

	CCloudResult::CCloudResult() : mHasBinary(false), mBinary(NULL), mSize(0), mObsolete(false) {
		this->mJson = new CHJSON();
	}
	
	CCloudResult::CCloudResult(eErrorCode err) : mHasBinary(false), mBinary(NULL), mSize(0), mObsolete(false) {
		this->mJson = new CHJSON();
		mJson->Put("_error", err);
	}

	CCloudResult::CCloudResult(eErrorCode err, const char *message) : mHasBinary(false), mBinary(NULL), mSize(0), mObsolete(false) {
		this->mJson = new CHJSON();
		mJson->Put("_error", err);
		if (message) { mJson->Put("_description", message); }
	}

	CCloudResult::CCloudResult(eErrorCode err, CHJSON *ajson) : mHasBinary(false), mBinary(NULL), mSize(0), mObsolete(false) {
		if(ajson == NULL)
			this->mJson = new CHJSON();
		else if (ajson->type() == CHJSON::jsonObject)
			this->mJson = ajson;
		else {
			this->mJson = new CHJSON();
			this->mJson->Put("value", ajson);
		}
		mJson->Put("_error", err);
	}
	
	CCloudResult::CCloudResult(CHJSON *ajson) : mHasBinary(false), mBinary(NULL), mSize(0), mObsolete(false) {
		if (!ajson)
			this->mJson = new CHJSON();
		else if (ajson->type() == CHJSON::jsonObject)
			this->mJson = ajson;
		else {
			this->mJson = new CHJSON();
			this->mJson->Put("value", ajson);
		}
	}
	
	void CCloudResult::SetErrorCode(eErrorCode err) {
		mJson->Put("_error", err);
	}

	void CCloudResult::SetCurlErrorCode(int err) {
		mJson->Put("_curlerror", err);
	}
	
	void CCloudResult::SetHttpStatusCode(int err) {
		mJson->Put("_httpcode", err);
	}

	CCloudResult *CCloudResult::Duplicate() const {
		CCloudResult *n = new CCloudResult(this->mJson->Duplicate());
		if (this->mHasBinary) {
			n->mBinary = Retain(this->mBinary);
			n->mSize = this->mSize;
			n->mHasBinary = true;
		}
		return n;
	}

	void CCloudResult::SetBinary(void *buffer, size_t size) {
		mHasBinary = true;
		Release(mBinary), mBinary = new DataHolder(buffer);
		mSize = size;
	}
	
	const void *CCloudResult::BinaryPtr() const {
		return mBinary->ptr;
	}

	cstring CCloudResult::Print() const {
		cstring result;
		const char *obsoleteStr = mObsolete? " [obsolete call]" : "";
		if (HasBinary()) {
			csprintf(result, "[CCloudResult%s error=%d (%s), http code=%d binary data size=%d]", obsoleteStr, GetErrorCode(), GetErrorString(), GetHttpStatusCode(), BinarySize());
		} else {
			csprintf(result, "[CCloudResult%s error=%d (%s), http code=%d json=%s]", obsoleteStr, GetErrorCode(), GetErrorString(), GetHttpStatusCode(), mJson->printFormatted().c_str());
		}
		return result;
	}

	CCloudResult::~CCloudResult() {
		delete mJson;
		Release(mBinary);
		mSize = 0;
	}
	
	CCloudResult *Invoke(CCallback *call, CCloudResult *result)
	{
		if (call)
		{
			if (CallbackStack::gCallbackQueueHandledByRabbitFactory)
			{
				CallbackStack::pushCallback(call, result);
				return NULL;
			}
			else
			{
				call->Invoke(result);
				delete call;
				delete result;
				return NULL;
			}
		}
		return result;
	}
	
	CThreadCloud::CThreadCloud() {
		mJSON = new CHJSON();
	}
	
	CThreadCloud::~CThreadCloud() {
		delete mJSON;
	}
		
	void CThreadCloud::run(const char *webmethod)
	{
		if (webmethod == NULL ) // || !CThreadCloud::BacthAdd(method, mJSON))
			this->Start();
	}

	void CThreadCloud::Run() {
		CCallback *callback = MakeDelegate(this, &CThreadCloud::do_done);
		Invoke(callback, this->execute());
	}
	
	void CThreadCloud::do_done(const CCloudResult *result) {
		this->done(result);
		delete this;
	}
}
