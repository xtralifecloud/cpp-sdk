//
//  CUserManager.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 26/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#include "include/CUserManager.h"
#include "include/CFilesystemManager.h"
#include "include/CHJSON.h"
#include "Core/CCallback.h"
#include "Core/CStoreInterface.h"
#include "Core/CClannishRESTProxy.h"
#include "Core/XtraLife_private.h"
#include "Core/RegisterDevice.h"
#include "Misc/helpers.h"

#define LOGIN_PARAMS_PATH "xtralife-system/LoginParams.json"

using namespace XtraLife::Helpers;

namespace XtraLife {
	static singleton_holder<CUserManager> managerSingleton;

	CUserManager::CUserManager() :
	loginDoneHandler(*(new CGloballyKeptHandler<CResultHandler>)),
	linkDoneHandler(*(new CGloballyKeptHandler<CResultHandler>)),
	convertDoneHandler(*(new CGloballyKeptHandler<CResultHandler>)),
	binaryDoneHandler(*(new CGloballyKeptHandler<CResultHandler>)) {
	}
	
	CUserManager::~CUserManager() {
		delete &linkDoneHandler;
		delete &loginDoneHandler;
		delete &convertDoneHandler;
		delete &binaryDoneHandler;
	}

	CUserManager *CUserManager::Instance() {
		return managerSingleton.Instance();
	}

	void CUserManager::Terminate() {
		managerSingleton.Release();
	}

	const char *CUserManager::GetDisplayName()
	{
		return CClannishRESTProxy::Instance()->GetDisplayName();
	}
	
	const char *CUserManager::GetGamerID()
	{
		return CClannishRESTProxy::Instance()->GetGamerID();
	}

	const char *CUserManager::GetMail()
	{
		return CClannishRESTProxy::Instance()->GetMail();
	}


	void CUserManager::Logout(CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		
		if (mDeviceOS && mDeviceToken) {
			CClannishRESTProxy::Instance()->UnregisterDevice(mDeviceOS, mDeviceToken, NULL);
			mDeviceToken = NULL;
			mDeviceOS = NULL;
		}
		
		CClannishRESTProxy::Instance()->Logout(NULL, MakeBridgeDelegate(this, &CUserManager::LogoutDone, aHandler));
	}
	
	void CUserManager::LogoutDone(CCloudResult *res) {
		// Unregister event listeners
		CClannishRESTProxy::Instance()->StopEventListening();
		UnregisterDevice();
		// Clear login parameters
		persistedLoginParams.mGamerId = NULL;
		persistedLoginParams.mGamerSecret = NULL;
		mAccountNetwork = NULL;
		CommitLoginParams();
		//XtraLife::CClannishRESTProxy::Instance()->Suspend(); mainthread ?
	}

	void CUserManager::didLogin(const CCloudResult *result) {
		if (result->GetErrorCode() == enNoErr) {
			persistedLoginParams.mGamerId = result->GetJSON()->GetString("gamer_id");
			persistedLoginParams.mGamerSecret = result->GetJSON()->GetString("gamer_secret");
			mAccountNetwork = result->GetJSON()->GetString("network");
			CommitLoginParams();
			// If we're effectively logged in, launch a listener for the private event domain
			if (CClannishRESTProxy::Instance()->autoRegisterForNotification())
				this->RegisterForNotification();
			CClannishRESTProxy::Instance()->StartEventListening();
			// We do that to start AppStore processes once logged in, and to restore potential purchases
			CStoreInterface::Instance();
		}
        loginDoneHandler.Invoke(result);
	}
	
	bool CUserManager::IsAnonymousAccount() {
		return IsEqual(mAccountNetwork, "anonymous");
	}

	void CUserManager::RegisterForNotification() {
		::RegisterDevice();
	}
	
	void CUserManager::CommitLoginParams() {
		if (CClan::Instance()->useAutoResume()) {
			CHJSON result;
			result.Put("gamerId", persistedLoginParams.mGamerId);
			result.Put("gamerSecret", persistedLoginParams.mGamerSecret);
			CFilesystemManager::Instance()->WriteJson(LOGIN_PARAMS_PATH, &result);
		}
	}

	void CUserManager::ReadLoginParams(PersistedLoginParams *dest) {
		if (CClan::Instance()->useAutoResume()) {
			CHJSON *result = CFilesystemManager::Instance()->ReadJson(LOGIN_PARAMS_PATH);
			if (result) {
				dest->mGamerId = result->GetString("gamerId");
				dest->mGamerSecret = result->GetString("gamerSecret");
			}
		}
	}

	void CUserManager::LoginAnonymous(const CHJSON* aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enAlreadyLogged); return; }
		if (loginDoneHandler.IsSet()) { InvokeHandler(aHandler, enOperationAlreadyInProgress); return; }
		
		loginDoneHandler.Set(aHandler);
		CHJSON j;
		j.Put("device", collectDeviceInformation());
        if (aConfiguration && aConfiguration->Has("options"))
            j.Put("options", aConfiguration->Get("options")->Duplicate());
		CClannishRESTProxy::Instance()->LoginAnonymous(&j, MakeBridgeDelegate(this, &CUserManager::LoginAnonymousDone));
	}
	
	void CUserManager::LoginAnonymousDone(CCloudResult *res) {
		didLogin(res);
	}
	
	void CUserManager::ResumeSession(const CHJSON* aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enAlreadyLogged); return; }
		if (loginDoneHandler.IsSet()) { InvokeHandler(aHandler, enOperationAlreadyInProgress); return; }
		
		const char *identifier = aConfiguration->GetString("id");
		const char *secret = aConfiguration->GetString("secret");
		// Empty parameters?
		if (!identifier || !secret) {
			InvokeHandler(aHandler, enBadParameters, "Missing parameter id or secret in configuration");
			return;
		}
		loginDoneHandler.Set(aHandler);		// we won't call it from here

		CHJSON json;
		json.Put("network", "anonymous");
		json.Put("id", identifier);
		json.Put("secret", secret);
		json.Put("device", collectDeviceInformation());
		if (aConfiguration->Has("options")) json.Put("options", aConfiguration->Get("options")->Duplicate());
		CClannishRESTProxy::Instance()->Login(&json, MakeBridgeDelegate(this, &CUserManager::LoginDone));
	}
	
    void CUserManager::LoginWithShortCode(const CHJSON* aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
        if (CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enAlreadyLogged); return; }
        if (loginDoneHandler.IsSet()) { InvokeHandler(aHandler, enOperationAlreadyInProgress); return; }
        
        const char *shortcode = aConfiguration->GetString("shortcode");
        // Empty parameters?
        if (!shortcode) {
            InvokeHandler(aHandler, enBadParameters, "Missing parameter shortcode in configuration");
            return;
        }
        loginDoneHandler.Set(aHandler);		// we won't call it from here
        
        CHJSON json;
        json.Put("network", "restore");
        json.Put("id", "");
        json.Put("secret", shortcode);
        json.Put("device", collectDeviceInformation());
        if (aConfiguration->Has("options")) json.Put("options", aConfiguration->Get("options")->Duplicate());
        CClannishRESTProxy::Instance()->Login(&json, MakeBridgeDelegate(this, &CUserManager::LoginDone));
    }
    
	void CUserManager::LoginDone(CCloudResult *result) {
		didLogin(result);
	}
	
	void CUserManager::LoginNetwork(const CHJSON* aConfiguration, CResultHandler *aHandler) {
		if (loginDoneHandler.IsSet()) { InvokeHandler(aHandler, enOperationAlreadyInProgress); return; }

		const char *identifier = aConfiguration->GetString("id");
		const char *secret = aConfiguration->GetString("secret");
		const char *network = aConfiguration->GetString("network");

		if (strcmp(network, "googleplus")==0) {
			InvokeHandler(aHandler, enBadParameters, "googleplus network obsolete");
		} else if (strcmp(network, "facebook")==0) {
			InvokeHandler(aHandler, enBadParameters, "facebook network obsolete");
            
        } else if(strncmp(network, "external:", 9)==0) {
            loginDoneHandler.Set(aHandler);
            CHJSON *j = aConfiguration->Duplicate();
            j->Put("device", collectDeviceInformation());
            CClannishRESTProxy::Instance()->LogWithExternalNetwork(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
            delete j;
        } else if (strcmp(network, "facebookId")==0 && identifier && secret) {
			loginDoneHandler.Set(aHandler);
			CHJSON *j = aConfiguration->Duplicate();
			j->Put("device", collectDeviceInformation());
			j->Put("network", "facebook");
			CClannishRESTProxy::Instance()->LogWithExternalNetwork(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
			delete j;
		} else if (strcmp(network, "googleplusId")==0 && identifier && secret) {
			loginDoneHandler.Set(aHandler);
			CHJSON *j = aConfiguration->Duplicate();
			j->Put("device", collectDeviceInformation());
			j->Put("network", "googleplus");
			CClannishRESTProxy::Instance()->LogWithExternalNetwork(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
			delete j;
		} else if (strcmp(network, "email")==0 && identifier && secret) {
			loginDoneHandler.Set(aHandler);
			CHJSON *j = aConfiguration->Duplicate();
			j->Put("device", collectDeviceInformation());
			CClannishRESTProxy::Instance()->Login(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
			delete j;
		} else if (strcmp(network, "restore")==0 && identifier && secret) {
			loginDoneHandler.Set(aHandler);
			CHJSON *j = aConfiguration->Duplicate();
			j->Put("device", collectDeviceInformation());
			CClannishRESTProxy::Instance()->Login(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
			delete j;
		} else if (strcmp(network, "gamecenter")==0) {
            loginDoneHandler.Set(aHandler);
            CHJSON *j = aConfiguration->Duplicate();
            j->Put("device", collectDeviceInformation());
            CClannishRESTProxy::Instance()->LogWithExternalNetwork(j, MakeBridgeDelegate(this, &CUserManager::LoginDone));
            delete j;
        }
	}

	void CUserManager::MailPassword(const CHJSON *aJson, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		CClannishRESTProxy::Instance()->MailPassword(aJson, MakeBridgeDelegate(aHandler));
	}

	void CUserManager::ChangePassword(const char *aNewPassword, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->ChangePassword(aNewPassword, MakeBridgeDelegate(aHandler));
	}

	void CUserManager::ChangeEmail(const char *aNewEmail, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->ChangeEmail(aNewEmail, MakeBridgeDelegate(aHandler));
	}


	//////////////////////////// Link ////////////////////////////
    void CUserManager::Link(const CHJSON* aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { return InvokeHandler(aHandler, enNotLogged); }
        
        CHJSON args;
        const char *network = aConfiguration->GetString("network");
        
        if (IsEqual(network, "facebookId")) {
            args.Put("network", "facebook");
        }
        else if (IsEqual(network, "googleplusId")) {
            args.Put("network", "googleplus");
        }
        else if (IsEqual(network, "gamecenter")) {
        }
        else {
            return InvokeHandler(aHandler, enBadParameters, "Unrecognized network (facebookId, googleplusId, gamecenter supported)");
        }
        
        args.Put("id", aConfiguration->GetString("id"));
        args.Put("secret", aConfiguration->GetString("secret"));
        CClannishRESTProxy::Instance()->LinkWith(&args, MakeBridgeDelegate(aHandler));
    }

	void CUserManager::Unlink(const char *aNetwork, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CHJSON json;
		json.Put("network", aNetwork);

		CClannishRESTProxy::Instance()->Unlink(&json, MakeBridgeDelegate(aHandler));
	}
	
	void CUserManager::linkDone(const CCloudResult *res) {
		linkDoneHandler.Invoke(res);
	}
	
	void CUserManager::Convert(const CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { return InvokeHandler(aHandler, enNotLogged); }
		if (!IsAnonymousAccount()) { return InvokeHandler(aHandler, enLogicError, "Conversion only applies to anonymous accounts"); }
		const char *network = aConfiguration->GetString("network");
		if (!network || !network[0]) { return InvokeHandler(aHandler, enBadParameters, "Missing network key"); }
		if (!aConfiguration->Has("id") || !aConfiguration->Has("secret")) {
			return InvokeHandler(aHandler, enBadParameters, "account conversion requires id, secret keys");
		}

		if (convertDoneHandler.IsSet()) { InvokeHandler(aHandler, enOperationAlreadyInProgress); return; }

		if (IsEqual(network, "googleplus")) {
			InvokeHandler(aHandler, enExternalCommunityNotSetup, "Google+ obsolete");
			return;
		} else if (IsEqual(network, "facebook")) {
			InvokeHandler(aHandler, enExternalCommunityNotSetup, "Facebook obsolete");
			return;
		} else if (IsEqual(network, "facebookId")) {
			CHJSON json;
			json.Put("network", "facebook");
			json.Put("id", aConfiguration->GetString("id"));
			json.Put("secret", aConfiguration->GetString("secret"));
			CClannishRESTProxy::Instance()->Convert(&json, MakeDelegate(this, &CUserManager::convertDone));
		} else if (IsEqual(network, "googleplusId")) {
			CHJSON json;
			json.Put("network", "googleplus");
			json.Put("id", aConfiguration->GetString("id"));
			json.Put("secret", aConfiguration->GetString("secret"));
			CClannishRESTProxy::Instance()->Convert(&json, MakeDelegate(this, &CUserManager::convertDone));
		} else if (IsEqual(network, "gamecenter")) {
            CClannishRESTProxy::Instance()->Convert(aConfiguration, MakeBridgeDelegate(aHandler));
		} else if (IsEqual(network, "email")) {
			CClannishRESTProxy::Instance()->Convert(aConfiguration, MakeBridgeDelegate(aHandler));
		}
		else
			InvokeHandler(aHandler, enBadParameters, "Unrecognized network");
	}

	void CUserManager::convertDone(const CCloudResult *res) {
		convertDoneHandler.Invoke(res);
	}
	
	//////////////////////////// User actions ////////////////////////////
	void CUserManager::UserExist(const char *aIdent, const char *aNetwork, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		
		CHJSON json;
		json.Put("id",aIdent);
		json.Put("network", aNetwork);
		CClannishRESTProxy::Instance()->UserExist(&json, MakeBridgeDelegate(aHandler));
	}

	void CUserManager::Outline(CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->Outline(MakeBridgeDelegate(aHandler));
	}

	void CUserManager::GetGodfatherCode(const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->GetGodfatherCode(aDomain, MakeBridgeDelegate(aHandler));
	}
	
    void CUserManager::GetGodfather(const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->GetGodfather(aDomain, MakeBridgeDelegate(aHandler));
	}

    void CUserManager::SetGodfather(const char *aCode, const CHJSON *aOptions, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CHJSON *j;
		if (aOptions)
			j = aOptions->Duplicate();
		else
			j = new CHJSON();
		j->Put("godfather", aCode);
		CClannishRESTProxy::Instance()->SetGodfather(j, MakeBridgeDelegate(aHandler));
		delete j;
	}
	
	void CUserManager::GetGodchildren(const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->GetGodchildren(aDomain, MakeBridgeDelegate(aHandler));
	}
    
	void CUserManager::SetProfile(const CHJSON *aJson, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		
		CClannishRESTProxy::Instance()->SetUserProfile(aJson, MakeBridgeDelegate(aHandler));
	}
		
	void CUserManager::GetProfile(CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }

		CHJSON emptyJson;
		CClannishRESTProxy::Instance()->GetUserProfile(&emptyJson, MakeBridgeDelegate(aHandler));
	}
	

	void CUserManager::SetProperties(const CHJSON* aPropertiesList, const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		
		if (aPropertiesList->type() != CHJSON::jsonObject) {
			InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (must be an object)");
			return;
		}
			
		int nb = aPropertiesList->size();
		for (int i=0; i<nb; i++) {
			const CHJSON * j = aPropertiesList->GetSafe(i);
			CHJSON::jsonType t = j->type();
			if (t != CHJSON::jsonTrue && t != CHJSON::jsonFalse && t != CHJSON::jsonString && t != CHJSON::jsonNumber) {
				InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (unrecognized property type)");
				return;
			}
		}
			  
		CClannishRESTProxy::Instance()->UserSetProperties(aDomain, aPropertiesList, MakeBridgeDelegate(aHandler));
	}
	
    void CUserManager::GetProperties(const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }

		CClannishRESTProxy::Instance()->UserGetProperties(aDomain, MakeBridgeDelegate(aHandler));
   }

	void CUserManager::SetProperty(const CHJSON* aProperty, const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		
		if (aProperty->type() != CHJSON::jsonObject) {
			InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (must be an object)");
			return;
		}
		
		if (!aProperty->Has("key")) return InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (key not found)");
		
		if (!aProperty->Has("value")) return InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (value not found)");

		const CHJSON * j = aProperty->GetSafe("value");
		CHJSON::jsonType t = j->type();
		if (t != CHJSON::jsonTrue && t != CHJSON::jsonFalse && t != CHJSON::jsonString && t != CHJSON::jsonNumber) {
			InvokeHandler(aHandler, enBadParameters, "Malformed properties JSON (unrecognized property type)");
			return;
		}
		
		CClannishRESTProxy::Instance()->UserSetProperty(aDomain, aProperty, MakeBridgeDelegate(aHandler));
	}

    void CUserManager::GetProperty(const char *aField, const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->UserGetProperty(aDomain, aField, MakeBridgeDelegate(aHandler));
		
	}

    void CUserManager::DeleteProperty(const char *aField, const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->UserDelProperty(aDomain, aField, MakeBridgeDelegate(aHandler));
		
	}

	void CUserManager::Balance(const char *aDomain, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }

		CClannishRESTProxy::Instance()->Balance(aDomain, NULL, MakeBridgeDelegate(aHandler));
	}

	void CUserManager::TxHistory(const char *aDomain, const CHJSON *aJSONCurrency, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); return; }
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }

		CClannishRESTProxy::Instance()->TxHistory(aDomain, aJSONCurrency, MakeBridgeDelegate(aHandler));
	}

    
    void CUserManager::GetValue(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        
        CClannishRESTProxy::Instance()->vfsReadv3(domain, key, MakeBridgeDelegate(aHandler));
    }
    
    void CUserManager::SetValue(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        const CHJSON *value = aConfiguration->Get("data");
        if (!value) { InvokeHandler(aHandler, enBadParameters, "Missing data"); return; }
        
        CClannishRESTProxy::Instance()->vfsWritev3(domain, key, value, false, MakeBridgeDelegate(aHandler));
    }
    
    void CUserManager::DeleteValue(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsDelete(domain, key, false, MakeBridgeDelegate(aHandler));
    }
    		
	void CUserManager::binaryWriteDone(const CCloudResult *result, const void* aPointer, size_t aSize, CResultHandler *aHandler) {
		if (result->GetErrorCode() == enNoErr) {
			const char *url = result->GetJSON()->GetString("putURL");
			char *geturl = strdup(result->GetJSON()->GetString("getURL"));
			CONSOLE_ERROR("get url = %s", geturl);
			if (url == NULL || *url ==0 ) return InvokeHandler(aHandler, enServerError);
			CClannishRESTProxy::Instance()->UploadData(url, aPointer, aSize, MakeInternalResultHandler(this, &CUserManager::binaryUploadDone, geturl,  aHandler));
		} else
			InvokeHandler(aHandler,result);
	}
	
	void CUserManager::binaryUploadDone(const CCloudResult *result, char* geturl, CResultHandler *aHandler)
	{
		CONSOLE_ERROR("upload get url = %s", geturl);
		CHJSON *json = 	 (CHJSON *) result->GetJSON();
		json->Put("getURL", geturl);
		free(geturl);
		InvokeHandler(aHandler,result);
	}

    void CUserManager::SetBinary(const CHJSON *aConfiguration, const void* aPointer, size_t aSize, CResultHandler *aHandler)
    {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsWritev3(domain, key, NULL, true, MakeInternalResultHandler(this, &CUserManager::binaryWriteDone, aPointer, aSize, aHandler));
    }
    
    void CUserManager::GetBinary(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsReadv3(domain, key, MakeInternalResultHandler(this, &CUserManager::getBinaryDone, aHandler));
    }
    
    void CUserManager::getBinaryDone(const CCloudResult *result, CResultHandler *aHandler) {
        if (result->GetErrorCode() != enNoErr) { InvokeHandler(aHandler, result); return; }
        const CHJSON* jsonBlob = result->GetJSON()->Get("result");
        const CHJSON* jsonUrl = jsonBlob->Get(0);
        const char *url = jsonUrl->valueString();
        if (url == NULL || *url ==0 ) return InvokeHandler(aHandler, enServerError);
        CClannishRESTProxy::Instance()->DownloadData(url, MakeBridgeDelegate(aHandler));
    }
    
    void CUserManager::DeleteBinary(const CHJSON *aConfiguration, CResultHandler *aHandler) {
        if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
        const char *domain = aConfiguration->GetString("domain");
        const char *key = aConfiguration->GetString("key");
        CClannishRESTProxy::Instance()->vfsDelete(domain, key, true, MakeBridgeDelegate(aHandler));
    }
    
	void CUserManager::ListAchievements(const CHJSON *configuration, CResultHandler *handler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(handler, enNotLogged); return; }

		CClannishRESTProxy::Instance()->ListAchievements(configuration, MakeBridgeDelegate(handler));
	}

	void CUserManager::SetAchievementData(const CHJSON *configuration, CResultHandler *handler) {
		// Requires login
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(handler, enNotLogged); return; }

		// Check parameters
		const char *domain = configuration->GetString("domain");
		const char *name = configuration->GetString("name");
		const CHJSON *data = configuration->Get("data");
		if (!name || !data) { InvokeHandler(handler, enBadParameters, "Missing required parameter name or data"); return; }

		// Default domain
		domain = (domain && *domain) ? domain : "private";
		CClannishRESTProxy::Instance()->SetAchievementGamerData(domain, name, data, MakeBridgeDelegate(handler));
	}

	void CUserManager::EarnAchievement(const char *unit, int increment, const CHJSON *gamerData, CResultHandler *handler) {
		struct TransactionDone: CInternalResultHandler {
			_BLOCK1(TransactionDone, CInternalResultHandler,
					CResultHandler*, resultHandler);

			void Done(const CCloudResult *result) {
				InvokeHandler(resultHandler, result);
			}
		};

		struct DoTransaction: CInternalResultHandler {
			_BLOCK2(DoTransaction, CInternalResultHandler,
					CHJSON*, txConfig,
					CResultHandler*, resultHandler);

			void Done(const CCloudResult *result) {
				if (result->GetErrorCode() != enNoErr) {
					InvokeHandler(resultHandler, result);
					return;
				}

				CClannishRESTProxy::Instance()->Transaction(txConfig, true, new TransactionDone(resultHandler));
				delete txConfig;
			}
		};

		char desc[1024];
		CHJSON *txConfig = new CHJSON, *tx = new CHJSON;
		CInternalResultHandler *next = new DoTransaction(txConfig, handler);
		safe::sprintf(desc, "Earned achievement %s by %d", unit, increment);
		tx->Put(unit, increment);
		txConfig->Put("transaction", tx);
		txConfig->Put("description", desc);

		if (!gamerData) {
			InvokeHandler(next, enNoErr);
			return;
		}
		CClannishRESTProxy::Instance()->SetAchievementGamerData("private", unit, gamerData, next);
	}

	// Not exposed as intended, as it is only for us (dev environments)
	void CUserManager_ResetAchievements(const char *domain, CResultHandler *handler) {
		domain = (domain && *domain) ? domain : "private";
		CClannishRESTProxy::Instance()->ResetAchievements(domain, MakeBridgeDelegate(handler));
	}

	void CUserManager::TransactionExtended(const CHJSON *configuration, CResultHandler *handler) {
		CClannishRESTProxy::Instance()->Transaction(configuration, true, MakeBridgeDelegate(handler));
	}

	void CUserManager::RegisterDevice(const char *aOS, const char *aToken) {
		mDeviceOS = aOS;
		mDeviceToken = aToken;
		CClannishRESTProxy::Instance()->RegisterDevice(aOS, aToken, NULL);
	}
	
	void CUserManager::RegisterEventListener(const char *domain, CEventListener *listener) {
		const char *realdomain = (domain && *domain) ? domain : "private";
		CClannishRESTProxy::Instance()->RegisterEventListener(realdomain, listener);
	}

	void CUserManager::UnregisterEventListener(const char *domain, CEventListener *listener) {
		CClannishRESTProxy::Instance()->UnregisterEventListener(domain, listener);
	}
	
	void CUserManager::PushEvent(const char *aDomain, const char* aGamerid, const CHJSON *aEvent, const CHJSON *aNotification, CResultHandler *aHandler) {
		CHJSON *message = new CHJSON();
		message->Put("type", "user");
		message->Put("event", aEvent);
		message->Put("from", this->GetGamerID());
		message->Put("name", this->GetDisplayName());
		message->Put("to" , aGamerid);
		if (aNotification)
			message->Put("osn", aNotification);
		CClannishRESTProxy::Instance()->PushEvent(aDomain, aGamerid, message, MakeBridgeDelegate(aHandler));
	}

	void CUserManager::DoSetupProcesses(CResultHandler *aHandler) {
		PersistedLoginParams paramsToRestore;
		ReadLoginParams(&paramsToRestore);

		if (paramsToRestore.mGamerId) {
			struct LoginBridge: CResultHandler {
				CResultHandler *aHandler;
				LoginBridge(CResultHandler *aHandler) : aHandler(aHandler), CResultHandler(this, &LoginBridge::Done) {}
				void Done(eErrorCode code, const CCloudResult *result) {
					// Whether login succeeds or fails, the setup is considered OK
					InvokeHandler(aHandler, result);
				}
			};
			CHJSON json;
			json.Put("network", "anonymous");
			json.Put("id", paramsToRestore.mGamerId);
			json.Put("secret", paramsToRestore.mGamerSecret);
			ResumeSession(&json, new LoginBridge(aHandler));
			return;
		}

		InvokeHandler(aHandler, enNoErr);
	}

	void CUserManager::Batch(const CHJSON *aConfiguration, const CHJSON *aParameters, CResultHandler *aHandler) {
		if (!CClan::Instance()->isUserLogged()) { InvokeHandler(aHandler, enNotLogged); return; }
		CClannishRESTProxy::Instance()->BatchUser(aConfiguration, aParameters, MakeBridgeDelegate(aHandler));
	}

}
