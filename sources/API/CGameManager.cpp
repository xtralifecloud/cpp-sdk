//
//  CGameManager.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 10/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#include "include/CGameManager.h"
#include "include/CClan.h"
#include "include/CHJSON.h"
#include "Core/CCallback.h"
#include "Core/CClannishRESTProxy.h"
#include "Misc/helpers.h"

using namespace XtraLife::Helpers;

namespace XtraLife {
	
	static singleton_holder<CGameManager> managerSingleton;

	CGameManager::CGameManager() {
	}
	
	CGameManager::~CGameManager() {
	}

	CGameManager *CGameManager::Instance() {
		return managerSingleton.Instance();
	}

	void CGameManager::Terminate() {
		managerSingleton.Release();
	}
	
	void CGameManager::Score(long long aHighScore, const char *aMode, const char *aScoreType, const char *aInfoScore, bool aForce, const char *aDomain, CResultHandler *aHandler)
    {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); }
		
		CHJSON json;
		json.Put("score", (double)aHighScore);
		json.Put("mode", aMode);
		json.Put("order", aScoreType);
		json.Put("domain", aDomain);
		if (aInfoScore)
			json.Put("info", aInfoScore);
		if (aForce)
			json.Put("mayvary", aForce);
		
		CClannishRESTProxy::Instance()->Score(&json, MakeBridgeDelegate(aHandler));
	}
	
    void CGameManager::Score(CResultHandler *aHandler, long long aHighScore, const char *aMode, const char *aScoreType, const char *aInfoScore, bool aForce, const char *aDomain)
    {
        this->Score(aHighScore, aMode, aScoreType, aInfoScore, aForce, aDomain, aHandler);
    }
    
    void CGameManager::GetRank(long long aScore, const char *aMode, const char *aDomain, CResultHandler *handler)
	{
		if (!CClan::Instance()->isSetup()) { InvokeHandler(handler, enSetupNotCalled); }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(handler, enNotLogged); }
		
		CHJSON json;
		json.Put("score", (double)aScore);
		json.Put("mode", aMode);
		json.Put("domain", aDomain);
		CClannishRESTProxy::Instance()->GetRank(&json, MakeBridgeDelegate(handler));

	}
	
    void CGameManager::GetRank(CResultHandler *aHandler, long long aScore, const char *aMode, const char *aDomain)
    {
        this->GetRank(aScore, aMode, aDomain, aHandler);
    }
    
    void CGameManager::CenteredScore(CResultHandler *aHandler, int aCount, const char *aMode, const char *aDomain)
    {
        this->CenteredScore(aCount, aMode, aDomain, aHandler);
	}
	
    void CGameManager::CenteredScore(int aCount, const char *aMode, const char *aDomain, CResultHandler *aHandler)
    {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); }
        
        CHJSON json;
        json.Put("count", aCount);
        json.Put("mode", aMode);
        json.Put("domain", aDomain);
        
        CClannishRESTProxy::Instance()->CenteredScore(&json, MakeBridgeDelegate(aHandler));
    }
    
    void CGameManager::BestHighScore(int aCount,int aPage, const char *aMode, const char *aDomain, CResultHandler *aHandler)
    {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); }
		
		CHJSON json;
		json.Put("count", aCount);
		json.Put("mode", aMode);
		json.Put("page", aPage);
		json.Put("domain", aDomain);
		CClannishRESTProxy::Instance()->BestHighScore(&json, MakeBridgeDelegate(aHandler));
		
	}

    void CGameManager::BestHighScore(CResultHandler *aHandler, int aCount,int aPage, const char *aMode, const char *aDomain)
    {
        this->BestHighScore(aCount, aPage, aMode, aDomain, aHandler);
    }

    void CGameManager::UserBestScores(const char *aDomain, CResultHandler *aHandler)
    {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); }
		CClannishRESTProxy::Instance()->UserBestScore(aDomain, MakeBridgeDelegate(aHandler));
	}

    void CGameManager::UserBestScores(CResultHandler *aHandler, const char *aDomain)
    {
        this->UserBestScores(aDomain, aHandler);
    }
    
    void CGameManager::GetValue(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        
        CClannishRESTProxy::Instance()->vfsReadGamev3(domain, key, MakeBridgeDelegate(aHandler));
    }
    
    void CGameManager::KeyValueRead(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        
        CClannishRESTProxy::Instance()->vfsReadGame(domain, key, MakeBridgeDelegate(aHandler));
    }
    
	/****
	void CGameManager::KeyValueWrite(const CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		const char *domain = aConfiguration->GetString("domain");
		const char *key = aConfiguration->GetString("key");
		const CHJSON *value = aConfiguration->Get("data");
		if (!value) { InvokeHandler(aHandler, enBadParameters, "Missing data"); return; }
		
		CClannishRESTProxy::Instance()->vfsWriteGame(domain, key, value, false, MakeBridgeDelegate(aHandler));
	}
	
	void CGameManager::KeyValueDelete(const CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		const char *domain = aConfiguration->GetString("domain");
		const char *key = aConfiguration->GetString("key");
		CClannishRESTProxy::Instance()->vfsDeleteGame(domain, key, false, MakeBridgeDelegate(aHandler));
	}
	***/
	
    void CGameManager::GetBinary(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsReadGamev3(domain, key, MakeInternalResultHandler(this, &CGameManager::binaryReadDone, aHandler));
    }
    
    void CGameManager::BinaryRead(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsReadGame(domain, key, MakeInternalResultHandler(this, &CGameManager::binaryReadDone, aHandler));
    }
    
	void CGameManager::binaryReadDone(const CCloudResult *result, CResultHandler *aHandler) {
		if (result->GetErrorCode() == enNoErr) {
			const char *url = result->GetJSON()->GetString("value");
			if (url == NULL || *url ==0 ) return InvokeHandler(aHandler, enServerError);
			CClannishRESTProxy::Instance()->DownloadData(url, MakeBridgeDelegate(aHandler));
		} else
			InvokeHandler(aHandler, result);
	}

    void CGameManager::getBinaryDone(const CCloudResult *result, CResultHandler *aHandler) {
        if (result->GetErrorCode() != enNoErr) { InvokeHandler(aHandler, result); return; }
        const CHJSON* jsonBlob = result->GetJSON()->Get("result");
        const CHJSON* jsonUrl = jsonBlob->Get(0);
        const char *url = jsonUrl->valueString();
        if (url == NULL || *url ==0 ) return InvokeHandler(aHandler, enServerError);
        CClannishRESTProxy::Instance()->DownloadData(url, MakeBridgeDelegate(aHandler));
    }

    /****
	void CGameManager::BinaryWrite(const CHJSON *aConfiguration, const void* aPointer, size_t aSize, CResultHandler *aHandler)
	{
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		const char *domain = aConfiguration->GetString("domain");
		const char *key = aConfiguration->GetString("key");
		CClannishRESTProxy::Instance()->vfsWriteGame(domain, key, NULL, true, MakeInternalResultHandler(this, &CGameManager::binaryWriteDone, aPointer, aSize, aHandler));
	}
	
	void CGameManager::binaryWriteDone(const XtraLife::CCloudResult *result, const void* aPointer, size_t aSize, CResultHandler *aHandler) {
		if (result->GetErrorCode() == enNoErr) {
			const char *url = result->GetJSON()->GetString("putURL");
			if (url == NULL || *url ==0 ) return InvokeHandler(aHandler, enServerError);
			CClannishRESTProxy::Instance()->UploadData(url, aPointer, aSize, MakeBridgeDelegate(aHandler));
		} else
			InvokeHandler(aHandler,result);
	}
	
	void CGameManager::BinaryDelete(const CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		const char *domain = aConfiguration->GetString("domain");
		const char *key = aConfiguration->GetString("key");
		CClannishRESTProxy::Instance()->vfsDeleteGame(domain, key, true, MakeBridgeDelegate(aHandler));
	}
	****/

	void CGameManager::Batch(CResultHandler *aHandler, const XtraLife::Helpers::CHJSON *aConfiguration, const XtraLife::Helpers::CHJSON *aParameters) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		CClannishRESTProxy::Instance()->BatchGame(aConfiguration, aParameters, MakeBridgeDelegate(aHandler));
	}

}

