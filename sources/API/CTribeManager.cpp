//
//  CTribeManager.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 20/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#include "include/CTribeManager.h"
#include "include/CClan.h"
#include "include/CHJSON.h"
#include "Core/CCallback.h"
#include "Core/CClannishRESTProxy.h"
#include "Core/XtraLife_private.h"
#include "Misc/helpers.h"

using namespace XtraLife::Helpers;

namespace XtraLife {

	static singleton_holder<CTribeManager> managerSingleton;
	
	CTribeManager::CTribeManager() : friendHandler(*(new CGloballyKeptHandler<CResultHandler>)) {
	}
	
	CTribeManager::~CTribeManager() {
		delete &friendHandler;
	}
	
	CTribeManager *CTribeManager::Instance() {
		return managerSingleton.Instance();
	}

	void CTribeManager::Terminate() {
		managerSingleton.Release();
	}

    void CTribeManager::ListUsers(const char *aContainsString, int aLimit, int aSkip, CResultHandler *aHandler)
    {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        CHJSON json;
        json.Put("q",aContainsString);
        json.Put("skip",aSkip);
        json.Put("limit",aLimit);
        CClannishRESTProxy::Instance()->ListUsers(&json, MakeBridgeDelegate(aHandler));
    }
    
    void CTribeManager::ListUsers(CResultHandler *aHandler, const char *aContainsString, int aLimit, int aSkip)
    {
        this->ListUsers(aContainsString, aLimit, aSkip, aHandler);
    }
    
    void CTribeManager::ListFriends(const char *aDomain, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        CHJSON json;
        json.Put("domain", aDomain);
        CClannishRESTProxy::Instance()->ListFriends(&json, MakeBridgeDelegate(aHandler));
    }
    
	void CTribeManager::ListFriends(CResultHandler *aHandler, const char *aDomain) {
        this->ListFriends(aDomain, aHandler);
	}
			
    void CTribeManager::BlacklistFriends(const char *aDomain, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        CHJSON json;
        json.Put("domain", aDomain);
        CClannishRESTProxy::Instance()->BlacklistFriends(&json, MakeBridgeDelegate(aHandler));
    }
    
    void CTribeManager::BlacklistFriends(CResultHandler *aHandler, const char *aDomain) {
        this->BlacklistFriends(aDomain, aHandler);
    }
    
    void CTribeManager::ChangeRelationshipStatus(const CHJSON *aOptions, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        if (aOptions == NULL) { InvokeHandler(aHandler, enBadParameters); return; }
        if (IsEqual(aOptions->GetString("id", ""), CClannishRESTProxy::Instance()->GetGamerID())) { InvokeHandler(aHandler, enFriendYourself); return; }
        CClannishRESTProxy::Instance()->ChangeRelationshipStatus(aOptions, MakeBridgeDelegate(aHandler));
    }
    
    void CTribeManager::ChangeRelationshipStatus(CResultHandler *aHandler, const CHJSON *aOptions) {
        this->ChangeRelationshipStatus(aOptions, aHandler);
    }
    
    void CTribeManager::ListNetworkFriends(const CHJSON* aConfiguration, CResultHandler *aHandler)
    {
        const char *network = aConfiguration->GetString("network");
        
        CONSOLE_VERBOSE("List network friends\n");
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        
        if (IsEqual(network, "googleplus")) {
            InvokeHandler(aHandler, enExternalCommunityNotSetup, "Obsolete network");
        } else if (IsEqual(network, "facebook")) {
            InvokeHandler(aHandler, enExternalCommunityNotSetup, "Obsolete network");
        } else if (IsEqual(network, "googleplusId")) {
            CClannishRESTProxy::Instance()->GetNetworkFriends("googleplus", aConfiguration, MakeBridgeDelegate(aHandler));
        } else if (IsEqual(network, "facebookId")) {
            CClannishRESTProxy::Instance()->GetNetworkFriends("facebook", aConfiguration, MakeBridgeDelegate(aHandler));
        } else if (IsEqual(network, "gamecenter")) {
            CClannishRESTProxy::Instance()->GetNetworkFriends("gamecenter", aConfiguration, MakeBridgeDelegate(aHandler));
        } else {
            InvokeHandler(aHandler, enExternalCommunityNotSetup, "Unrecognized network");
        }
    }
    
    void CTribeManager::ListNetworkFriends(CResultHandler *aHandler, const CHJSON* aConfiguration)
    {
        this->ListNetworkFriends(aConfiguration, aHandler);
    }
    
	void CTribeManager::ListNetworkFriends(CResultHandler *aHandler, const char *network, const CHJSON* aFriends ) {
		CHJSON config;
		config.Put("network" , network);
		config.Put("friends" , aFriends->Duplicate());
		ListNetworkFriends(&config, aHandler);
	}
	
    void CTribeManager::FriendsBestHighScore(int aCount, int aPage, const char *aMode, const char *aDomain, CResultHandler *aHandler)
    {
        CHJSON json;
        json.Put("count", aCount);
        json.Put("mode", aMode);
        json.Put("page", aPage);
        json.Put("domain", aDomain);
        CClannishRESTProxy::Instance()->FriendsBestHighScore(&json, MakeBridgeDelegate(aHandler));
    }
    
    void CTribeManager::FriendsBestHighScore(CResultHandler *aHandler, int aCount, int aPage, const char *aMode, const char *aDomain)
    {
        this->FriendsBestHighScore(aCount, aPage, aMode, aDomain, aHandler);
    }
    
    void CTribeManager::FindOpponents(const CHJSON *aFilter, const char *aDomain, CResultHandler *aHandler) {
        CClannishRESTProxy::Instance()->FindOpponents(aDomain, aFilter, MakeBridgeDelegate(aHandler));
    }
    
    void CTribeManager::FindOpponents(CResultHandler *aHandler, const CHJSON *aFilter, const char *aDomain) {
        this->FindOpponents(aFilter, aDomain, aHandler);
    }
}

