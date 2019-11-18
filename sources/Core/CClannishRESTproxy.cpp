//
//  CClannishRESTProxy.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 15/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#include <algorithm>
#include <chrono>
#include <thread>

#include "Core/CClannishRESTProxy.h"
#include "include/CUserManager.h"
#include "Core/XtraLife_private.h"
#include "Misc/helpers.h"
#include "Misc/util.h"
#include "Misc/curltool.h"

using namespace XtraLife::Helpers;

// In seconds
#define POP_REQUEST_RECOVER_TIMEOUT 2
#define EVENT_THREAD_HOLD 20

#ifndef __IOS__
void endedPop() {}
#endif

namespace XtraLife {
	
	static singleton_holder<CClannishRESTProxy> managerSingleton;
	
	CClannishRESTProxy::CClannishRESTProxy() {
		Init();
		mRegisterForNotification = true;
		mLinks = new CHJSON();
	}
	
	CClannishRESTProxy::~CClannishRESTProxy() {
		// Terminate all running listeners
		if (mLinks) delete mLinks;
		mLinks = NULL;
	}
	
	CClannishRESTProxy *CClannishRESTProxy::Instance() {
		return managerSingleton.Instance();
	}

	void CClannishRESTProxy::Terminate() {
		StopEventListening();
		managerSingleton.Release();
	}
	
	bool CClannishRESTProxy::isSetup()
	{
		return mApiKey != NULL;
	}
	
	bool CClannishRESTProxy::isLoggedIn()
	{
		return mGamerId != NULL;
	}

	void CClannishRESTProxy::SetNetworkState(bool on) {
		bool previousState = g_networkState;
		CONSOLE_ERROR( on ? "Network Activity Resumed !\n"  : "Network Activity Suspended !\n");
		mNetSate = on;
		g_networkState = on;
		// Network just got back! -> trigger pending requests if any
		if (g_networkState && !previousState) {
			http_trigger_pending();
		}
	}

	void CClannishRESTProxy::Suspend() {
		mSuspend = true;
	}

	void CClannishRESTProxy::Resume() {
		mSuspend = false;
		suspendedThreadLock.SignalAll();
	}

	const char *CClannishRESTProxy::GetGamerID() { return mGamerId; }
	const char *CClannishRESTProxy::GetNetwork() { return mNetwork; }
	const char *CClannishRESTProxy::GetNetworkID() { return mNetworkId; }
	const char *CClannishRESTProxy::GetDisplayName() { return mDisplayName; }
	const char *CClannishRESTProxy::GetMail() { return mEmail; }
	const char *CClannishRESTProxy::GetAppID() { return mAppID; }
		
	void CClannishRESTProxy::Setup(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		const char *env;
		
		int lbCount = 2;
		
		srand((unsigned) time(NULL));
		mApiKey = ajSON->GetString("key");
		mApiSecret = ajSON->GetString("secret");
		mAppVersion = ajSON->GetString("appVersion");
		mSdkVersion = ajSON->GetString("sdkVersion");
		mPopEventLoopDelay = ajSON->GetInt("eventLoopTimeout", 590);
		env = ajSON->GetString("env");
		mRegisterForNotification = ajSON->GetBool("autoRegisterForNotification", true);

		// Old name for this key
		if (ajSON->GetBool("disactiveRegister")) { mRegisterForNotification = false; }
		
		if (IsEqual(env, "sandbox"))
			env = "https://sandbox-api[id].clanofthecloud.mobi";
		else if (IsEqual(env, "prod")) {
			env = "https://prod-api[id].clanofthecloud.mobi";
			lbCount = 16;
		}

		// Insufficient information provided
		if (!mApiKey || !mApiSecret || !env) {
			return InvokeHandler(onFinished, enBadAppCredential, "Missing required parameter");
		}

		// Default to 5 sec for connection
		int connectTimeout = ajSON->GetInt("connectTimeout", 5);
		int httpTimeout = ajSON->GetInt("httpTimeout");
		bool httpVerbose = ajSON->GetBool("httpVerbose");
		http_init(env, lbCount, connectTimeout, httpTimeout, httpVerbose, &suspendedThreadLock);
		return InvokeHandler(onFinished, enNoErr);
	}

	void CClannishRESTProxy::Ping(CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		CHttpRequest *req = MakeUnauthenticatedHttpRequest("/v1/ping");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::LogWithExternalNetwork(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (isLoggedIn()) { return InvokeHandler(onFinished, enAlreadyLogged); }
		
		// LoginResultHandler is a good handler for all login related tasks
		CHttpRequest *req = MakeUnauthenticatedHttpRequest("/v1/login");
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(this, &CClannishRESTProxy::LoginResultHandler, onFinished));
		return http_perform(req);
	}

	//////////////////////////// Login methods ////////////////////////////
	void CClannishRESTProxy::LoginAnonymous(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (isLoggedIn()) { return InvokeHandler(onFinished, enAlreadyLogged); }
		
		// LoginResultHandler is a good handler for all login related tasks
		CHttpRequest *req = MakeUnauthenticatedHttpRequest("/v1/login/anonymous");
		req->SetCallback(MakeBridgeCallback(this, &CClannishRESTProxy::LoginResultHandler, onFinished));
		req->SetBody(aJSON->Duplicate());
		http_perform(req);
	}

	void CClannishRESTProxy::Login(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (isLoggedIn()) { return InvokeHandler(onFinished, enAlreadyLogged); }

		// LoginResultHandler is a good handler for all login related tasks
		CHttpRequest *req = MakeUnauthenticatedHttpRequest("/v1/login");
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(this, &CClannishRESTProxy::LoginResultHandler, onFinished));
		http_perform(req);
	}

	CCloudResult *CClannishRESTProxy::LoginResultHandler(CCloudResult *result) {
		const CHJSON *rc = result->GetJSON();
		int status = result->GetHttpStatusCode();
		if (status >= 200 && status < 300) {
			// Extract meaningful info
			const CHJSON *profileNode = rc->GetSafe("profile");
			// Store the credentials for the next requests
			mGamerId = rc->GetString("gamer_id");
			mGamerSecret = rc->GetString("gamer_secret");
			mNetwork = rc->GetString("network");
			mNetworkId = rc->GetString("networkid");
			if (mLinks) delete mLinks;
			mLinks = rc->GetSafe("links")->Duplicate();
			if (profileNode) {
				mDisplayName = profileNode->GetString("displayName");
				mEmail = profileNode->GetString("email");
			}
			
			mSuspend = false;
			return result;
		} else {
			result->SetErrorCode(enServerError);
			return result;
		}
	}

	void CClannishRESTProxy::Convert(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/convert");
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::MailPassword(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isSetup()) { InvokeHandler(onFinished, enSetupNotCalled); return; }
		cstring url;
		csprintf(url, "/v1/login/%s", ajSON->GetString("email"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("GET");
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		http_perform(req);
	}
	
	void CClannishRESTProxy::ChangePassword(const char *aNewPassword, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { InvokeHandler(onFinished, enNotLogged); return; }
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/password");
		CHJSON *j = new CHJSON();
		j->Put("password", aNewPassword);
		req->SetBody(j);
		req->SetCallback(MakeBridgeCallback(onFinished));
		http_perform(req);
	}

	void CClannishRESTProxy::ChangeEmail(const char *aNewEmail, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { InvokeHandler(onFinished, enNotLogged); return; }
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/email");
		CHJSON *j = new CHJSON();
		j->Put("email", aNewEmail);
		req->SetBody(j);
		req->SetCallback(MakeBridgeCallback(onFinished));
		http_perform(req);
	}

	void CClannishRESTProxy::BatchGame(const CHJSON *ajSON, const CHJSON *aInput, CInternalResultHandler *onFinished) {
		cstring url;
		csprintf(url, "/v1/batch/%s/%s", ajSON->GetString("domain", "private"), ajSON->GetString("name"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(aInput->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::BatchUser(const CHJSON *ajSON, const CHJSON *aInput, CInternalResultHandler *onFinished) {
		cstring url;
		csprintf(url, "/v1/gamer/batch/%s/%s", ajSON->GetString("domain", "private"), ajSON->GetString("name"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(aInput->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	/////////////////////////////////// Logout methods ///////////////////////////////////
	void CClannishRESTProxy::Logout(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { InvokeHandler(onFinished, enNotLogged); return; }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/logout");
		req->SetMethod("POST");
		req->SetCallback(MakeBridgeCallback(this, &CClannishRESTProxy::LogoutResultHandler, onFinished));
		http_perform(req);
	}
	
	CCloudResult *CClannishRESTProxy::LogoutResultHandler(CCloudResult *result) {
		mGamerId = NULL;
		mGamerSecret = NULL;
		mDisplayName = NULL;
		mEmail = NULL;
		mNetwork = NULL;
		if (mLinks) delete mLinks;
		mLinks = new CHJSON();
		return result;
	}
	
	void CClannishRESTProxy::Outline(CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/outline");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::SetGodfather(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		const char *  domain = aJSON->GetString("domain", "private");
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/godfather").Subpath(domain));
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::GetGodfatherCode(const char *aDomain, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/godfather").Subpath(aDomain));
		req->SetMethod("PUT");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::GetGodfather(const char *aDomain, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/godfather").Subpath(aDomain));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::GetGodchildren(const char *aDomain, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/godchildren").Subpath(aDomain));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::GetUserProfile(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		CHttpRequest *req = MakeHttpRequest("/v1/gamer/profile");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::SetUserProfile(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/profile");
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::GetRank(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v2.6/gamer/scores/%s/%s", aJSON->GetString("domain"), aJSON->GetString("mode"));
		CHttpRequest *req = MakeHttpRequest(url);
		CHJSON json;
		json.Put("score", aJSON->GetDouble("score"));
		req->SetBody(json.Duplicate());
		req->SetMethod("PUT");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::GetRankArray(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		return InvokeHandler(onFinished, enNotImplemented);
	}
	
	void CClannishRESTProxy::Score (const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v2.6/gamer/scores/%s/%s?order=%s&mayvary=%s", aJSON->GetString("domain"), aJSON->GetString("mode"), aJSON->GetString("order"), aJSON->GetBool("mayvary") ? "true" : "false");
		CHttpRequest *req = MakeHttpRequest(url);
		CHJSON json;
		json.Put("score", aJSON->GetDouble("score"));
		json.Put("info", aJSON->GetString("info"));
		req->SetBody(json.Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::CenteredScore(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v2.6/gamer/scores/%s/%s?count=%d&page=me", aJSON->GetString("domain"), aJSON->GetString("mode"), aJSON->GetInt("count"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::BestHighScore (const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v2.6/gamer/scores/%s/%s?count=%d&page=%d", aJSON->GetString("domain"), aJSON->GetString("mode"), aJSON->GetInt("count"), aJSON->GetInt("page"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UserBestScore(const char *domain, CInternalResultHandler *onFinished) {
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/bestscores").Subpath(domain));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::BestHighScoreArray(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		return InvokeHandler(onFinished, enNotImplemented);
	}

	void CClannishRESTProxy::FriendsBestHighScore(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v2.6/gamer/scores/%s/%s?count=%d&page=%d&type=friendscore", aJSON->GetString("domain"), aJSON->GetString("mode"), aJSON->GetInt("count"), aJSON->GetInt("page"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::CheckUser(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v1/gamer/gamer_id/%s", ajSON->GetString("gamer_id"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UserExist(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		// not need to be logged..
		
		cstring url;
		csprintf(url, "/v1/users/%s/%s", ajSON->GetString("network"), ajSON->GetString("id"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::ListUsers(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v1/gamer?q=%s&limit=%d&skip=%d", ajSON->GetString("q"), ajSON->GetInt("limit"), ajSON->GetInt("skip") );
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::ListFriends(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/friends").Subpath(aJSON->GetString("domain", "private")));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::BlacklistFriends(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/friends").Subpath(aJSON->GetString("domain", "private")).QueryParam("status","blacklist"));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::ChangeRelationshipStatus(const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		cstring url;
		csprintf(url, "/v2.6/gamer/friends/%s/%s?status=%s", aJSON->GetString("domain", "private"), aJSON->GetString("id"), aJSON->GetString("status"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("POST");
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}


	void CClannishRESTProxy::RegisterDevice(const char *os, const char *token, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v1/gamer/device?os=%s&token=%s", os, token);
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("POST");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UnregisterDevice(const char *os, const char *token, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v1/gamer/device?os=%s&token=%s", os, token);
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	//////////////////////////// Match API ////////////////////////////
	void CClannishRESTProxy::CreateMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		const char *domain = config->GetString("domain");
		CHJSON *dupConfig = config->Duplicate();
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		dupConfig->Delete("domain");

		CUrlBuilder url("/v1/gamer/matches");
		url.QueryParam("domain", (domain && domain[0]) ? domain : "private");
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(dupConfig);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::FinishMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		const char *matchId = config->GetString("id"), *lastEventId = config->GetString("lastEventId");
		if (!matchId || !lastEventId) { return InvokeHandler(onFinished, enBadParameters, "Missing either id or lastEventId"); }
		CUrlBuilder url("/v1/gamer/matches");
		url.Subpath(matchId).Subpath("finish").QueryParam("lastEventId", lastEventId);

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("POST");
		req->SetBody(MakeBodyWithOsn(config));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::DeleteMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId));
		req->SetBody(config->Duplicate());
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::JoinMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId).Subpath("join"));
		req->SetMethod("POST");
		req->SetBody(MakeBodyWithOsn(config));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::InvitePlayerToMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id"), *playerId = config->GetString("gamer_id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }
		if (!playerId) { return InvokeHandler(onFinished, enBadParameters, "Missing gamer id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId).Subpath("invite").Subpath(playerId));
		req->SetMethod("POST");
		req->SetBody(MakeBodyWithOsn(config));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::DrawFromShoeInMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id"), *lastEventId = config->GetString("lastEventId");
		if (!matchId || !lastEventId) { return InvokeHandler(onFinished, enBadParameters, "Missing either id or lastEventId"); }

		CUrlBuilder url("/v1/gamer/matches");
		url.Subpath(matchId).Subpath("shoe").Subpath("draw");
		url.QueryParam("count", config->GetInt("count", 1)).QueryParam("lastEventId", lastEventId);
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("POST");
		req->SetBody(MakeBodyWithOsn(config));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::DismissInvitationToMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId).Subpath("invitation"));
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::LeaveMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId).Subpath("leave"));
		req->SetMethod("POST");
		req->SetBody(MakeBodyWithOsn(config));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::FetchMatch(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id");
		if (!matchId) { return InvokeHandler(onFinished, enBadParameters, "Missing match id"); }

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::ListMatches(const CHJSON *config, CInternalResultHandler *onFinished) {
		const CHJSON *filter = config->Get("filter");
		int limit = config->GetInt("limit"), skip = config->GetInt("skip");
		const char *domain = config->GetString("domain");

		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }

		CUrlBuilder url("/v1/gamer/matches");
		url.QueryParam("domain", (domain && domain[0]) ? domain : "private");
		// Here, the JSON for filtering needs to be url-encoded
		if (filter) { url.QueryParam("properties", filter->print()); }
		if (config->Has("participating")) { url.QueryParam("participating"); }
		if (config->Has("finished")) { url.QueryParam("finished"); }
		if (config->Has("invited")) { url.QueryParam("invited"); }
		if (config->Has("full")) { url.QueryParam("full"); }
		if (limit) { url.QueryParam("limit", limit); }
		if (skip) { url.QueryParam("skip", skip); }

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::PostMove(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		const char *matchId = config->GetString("id"), *lastEventId = config->GetString("lastEventId");
		const CHJSON *move = config->Get("move");
		if (!matchId || !move || !lastEventId) { return InvokeHandler(onFinished, enBadParameters, "Missing either match id, move node or lastEventId"); }

		CHJSON *json = new CHJSON;
		json->Put("move", move);
		json->Put("globalState", config->Get("globalState"));
		json->Put("osn", config->Get("osn"));

		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v1/gamer/matches").Subpath(matchId).Subpath("move").QueryParam("lastEventId", lastEventId));
		req->SetBody(json);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	//////////////////////////// END match API ////////////////////////////
	
	//////////////////////////// store API ////////////////////////////
	void CClannishRESTProxy::GetProductList(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/store/products");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::GetPurchaseHistory(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/store/purchaseHistory");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::SendAppStorePurchaseToServer(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }	
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/store/validateReceipt");
		req->SetBody(config->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	//////////////////////////// END store API ////////////////////////////

	//////////////////////////// index API ////////////////////////////
	void CClannishRESTProxy::DeleteIndexedObject(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!config->Has("index") || !config->Has("objectid")) {
			return InvokeHandler(onFinished, enBadParameters, "Missing index or objectid in configuration");
		}

		CUrlBuilder url("/v1/index");
		url.Subpath(config->GetString("domain", "private"))
			.Subpath(config->GetString("index"))
			.Subpath(config->GetString("objectid"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::GetIndexedObject(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!config->Has("index") || !config->Has("objectid")) {
			return InvokeHandler(onFinished, enBadParameters, "Missing index or objectid in configuration");
		}

		CUrlBuilder url("/v1/index");
		url.Subpath(config->GetString("domain", "private"))
			.Subpath(config->GetString("index"))
			.Subpath(config->GetString("objectid"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::IndexObject(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!config->Has("index") || !config->Has("objectid") || !config->Has("properties")) {
			return InvokeHandler(onFinished, enBadParameters, "Missing index, objectid or properties in configuration");
		}

		CUrlBuilder url("/v1/index");
		url.Subpath(config->GetString("domain", "private")).Subpath(config->GetString("index"));
		CHttpRequest *req = MakeHttpRequest(url);
		CHJSON *data = new CHJSON();
		data->Put("id", config->GetString("objectid"));
		data->Put("properties", config->Get("properties"));
		data->Put("payload", config->Get("payload"));
		req->SetBody(data);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::SearchIndexedObjects(const CHJSON *config, CInternalResultHandler *onFinished) {
		if (!config->Has("index") || (!config->Has("query") && !config->Has("search"))) {
			return InvokeHandler(onFinished, enBadParameters, "Missing index or query in configuration");
		}
		const CHJSON *sortingProperties = config->Get("sort");
		if (sortingProperties && sortingProperties->type() != CHJSON::jsonArray) {
			return InvokeHandler(onFinished, enBadParameters, "sort must be an array");
		}

		CUrlBuilder url("/v1/index");
		url.Subpath(config->GetString("domain", "private"));
		url.Subpath(config->GetString("index"));
		url.Subpath("search");
		url.QueryParam("from", config->GetInt("skip"));
		url.QueryParam("max", config->GetInt("limit"));
		if (config->Has("query")) {
			url.QueryParam("q", config->GetString("query"));
			if (sortingProperties) { url.QueryParam("sort", sortingProperties->print()); }
		}
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("POST");
		if (config->Has("search")) {
			req->SetBody(config->GetSafe("search")->Duplicate());
		}
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	//////////////////////////// END index API ////////////////////////////
	

	void CClannishRESTProxy::LinkWith(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/link");
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::Unlink(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		CHttpRequest *req = MakeHttpRequest("/v1/gamer/unlink");
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::GetNetworkFriends(const char *network, const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		char url[128];
		safe::sprintf(url, "/v2.12/gamer/friends/private?network=%s", network);
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::InvitByMail(const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		return InvokeHandler(onFinished, enNotImplemented);
	}
	
	void CClannishRESTProxy::UserGetProperties(const char *aDomain, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/property").Subpath(aDomain));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::UserSetProperties(const char *aDomain, const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/property").Subpath(aDomain));
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::UserGetProperty(const char *aDomain, const char *key, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/property").Subpath(aDomain).Subpath(key));
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UserDelProperty(const char *aDomain, const char *key, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/property").Subpath(aDomain).Subpath(key));
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UserSetProperty(const char *aDomain, const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/property").Subpath(aDomain).Subpath(aJSON->GetString("key")));
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	

	void CClannishRESTProxy::FindOpponents(const char *aDomain, const CHJSON *ajSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = MakeHttpRequest(CUrlBuilder("/v2.6/gamer/matchproperties").Subpath(aDomain));
		req->SetMethod("GET");
		req->SetBody(ajSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
    void CClannishRESTProxy::vfsReadv3(const char *domain, const char *key, CInternalResultHandler *onFinished) {
        if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
        
        CUrlBuilder url("/v3.0/gamer/vfs");
        url.Subpath((domain && domain[0]) ? domain : "private");
        if (key && *key) {
            url.Subpath(key);
        }
        
        CHttpRequest *req = MakeHttpRequest(url);
        req->SetCallback(MakeBridgeCallback(onFinished));
        return http_perform(req);
    }
    
    void CClannishRESTProxy::vfsWritev3(const char *domain, const char *key, const CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished) {
        if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
        
        CUrlBuilder url("/v3.0/gamer/vfs");
        url.Subpath((domain && domain[0]) ? domain : "private");
        
        if (key && *key) {
            url.Subpath(key);
        }
        if (isBinary) {
            url.QueryParam("binary");
        }
        
        CHttpRequest *req = MakeHttpRequest(url);
        req->SetBody(aJSON->Duplicate());
        req->SetMethod("PUT");
        req->SetCallback(MakeBridgeCallback(onFinished));
        return http_perform(req);
    }
    
    void CClannishRESTProxy::vfsRead(const char *domain, const char *key, CInternalResultHandler *onFinished) {
        if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
        
        CUrlBuilder url("/v1/gamer/vfs");
        url.Subpath((domain && domain[0]) ? domain : "private");
        if (key && *key) {
            url.Subpath(key);
        }
        
        CHttpRequest *req = MakeHttpRequest(url);
        req->SetCallback(MakeBridgeCallback(onFinished));
        return http_perform(req);
    }
    
    void CClannishRESTProxy::vfsWrite(const char *domain, const char *key, const CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished) {
        if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
        
        CUrlBuilder url("/v1/gamer/vfs");
        url.Subpath((domain && domain[0]) ? domain : "private");
        
        if (key && *key) {
            url.Subpath(key);
        }
        if (isBinary) {
            url.QueryParam("binary");
        }
        
        CHttpRequest *req = MakeHttpRequest(url);
        req->SetBody(aJSON->Duplicate());
        req->SetMethod("PUT");
        req->SetCallback(MakeBridgeCallback(onFinished));
        return http_perform(req);
    }
    
	void CClannishRESTProxy::vfsDelete(const char *domain, const char *key, bool isBinary, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		CUrlBuilder url("/v1/gamer/vfs");
		url.Subpath((domain && domain[0]) ? domain : "private");
		if (key && *key) {
			url.Subpath(key);
		}
		if (isBinary) {
			url.QueryParam("binary");
		}
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::UploadData(const char *url, const void *ptr, size_t size, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		CHttpRequest *req = new CHttpRequest(url);
		req->SetBody(ptr, size);
		req->SetMethod("PUT");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::DownloadData(const char *url, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
	   
		CHttpRequest *req = new CHttpRequest(url);
		req->SetMethod("GET", true);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::vfsReadGame(const char *domain, const char *key, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		
		CUrlBuilder url("/v1/vfs");
		url.Subpath((domain && domain[0]) ? domain : "private");

		if (key && *key) {
			url.Subpath(key);
		}

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}
	
	void CClannishRESTProxy::vfsWriteGame(const char *domain, const char *key, const CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		
		CUrlBuilder url("/v1/vfs");
		url.Subpath((domain && domain[0]) ? domain : "private");

		if (key && *key) {
			url.Subpath(key);
		}
		if (isBinary) {
			url.QueryParam("binary");
		}

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(aJSON->Duplicate());
		req->SetMethod("PUT");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
		
	}

	void CClannishRESTProxy::vfsDeleteGame(const char *domain, const char *key, bool isBinary, CInternalResultHandler *onFinished) {
		if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		
		CUrlBuilder url("/v1/vfs");
		url.Subpath((domain && domain[0]) ? domain : "private");
		
		if (key && *key) {
			url.Subpath(key);
		}
		if (isBinary) {
			url.QueryParam("binary");
		}
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetMethod("DELETE");
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

    void CClannishRESTProxy::vfsReadGamev3(const char *domain, const char *key, CInternalResultHandler *onFinished) {
        if (!isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
        
        CUrlBuilder url("/v3.0/vfs");
        url.Subpath((domain && domain[0]) ? domain : "private");
        
        if (key && *key) {
            url.Subpath(key);
        }
        
        CHttpRequest *req = MakeHttpRequest(url);
        req->SetCallback(MakeBridgeCallback(onFinished));
        return http_perform(req);
    }
    
	void CClannishRESTProxy::Balance (const char *domain, const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v1/gamer/tx/%s/balance" , domain && *domain ? domain : "private");
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::Transaction (const CHJSON *aJSON, bool useV2_2, CInternalResultHandler *onFinished) {
		if (!CClan::Instance()->isSetup()) { return InvokeHandler(onFinished, enSetupNotCalled); }
		if (!CClan::Instance()->isUserLogged()) { return InvokeHandler(onFinished, enNotLogged); }

		const CHJSON *transaction = aJSON->Get("transaction");
		if (!transaction) { return InvokeHandler(onFinished, enBadParameters, "Missing transaction in configuration"); }
		
		CUrlBuilder url(useV2_2 ? "/v2.2/gamer/tx" : "/v1/gamer/tx");
		const char *domain = aJSON->GetString("domain");
		url.Subpath(domain && domain[0] ? domain : "private");
		CHJSON *tx = new CHJSON();
		tx->Put("transaction", transaction);
		tx->Put("description", aJSON->Get("description"));
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(tx);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::TxHistory (const char *domain, const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		if (aJSON && aJSON->GetString("unit"))
			csprintf(url, "/v1/gamer/tx/%s?unit=%s&skip=%d&limit=%d", domain && *domain ? domain : "private",  aJSON->GetString("unit"), aJSON->GetInt("skip", 0), aJSON->GetInt("limit", 100));
		else if (aJSON)
			csprintf(url, "/v1/gamer/tx/%s?skip=%d&limit=%d" , domain && *domain ? domain : "private", aJSON->GetInt("skip", 0), aJSON->GetInt("limit", 100));
		else
			csprintf(url, "/v1/gamer/tx/%s" , domain && *domain ? domain : "private");

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::ListAchievements(const CHJSON *configuration, CInternalResultHandler *onFinished) {
		CUrlBuilder url("/v1/gamer/achievements");
		const char *domain = configuration->GetString("domain");
		url.Subpath((domain && domain[0]) ? domain : "private");

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	void CClannishRESTProxy::SetAchievementGamerData(const char *domain, const char *achName, const CHJSON *data, CInternalResultHandler *onFinished) {
		if (!data) { return InvokeHandler(onFinished, enBadParameters, "Missing gamer data JSON"); }

		CUrlBuilder url("/v1/gamer/achievements");
		url.Subpath(domain).Subpath(achName).Subpath("gamerdata");

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		req->SetBody(data->Duplicate());
		return http_perform(req);
	}

	void CClannishRESTProxy::ResetAchievements(const char *domain, CInternalResultHandler *onFinished) {
		CUrlBuilder url("/v1/gamer/achievements");
		url.Subpath(domain).Subpath("reset");

		CHttpRequest *req = MakeHttpRequest(url);
		req->SetCallback(MakeBridgeCallback(onFinished));
		req->SetMethod("POST");
		return http_perform(req);
	}

	//////////////////////////// Events ////////////////////////////

	void CClannishRESTProxy::PushEvent (const char *domain, const char *gamerid, const CHJSON *aJSON, CInternalResultHandler *onFinished) {
		if (!isLoggedIn()) { return InvokeHandler(onFinished, enNotLogged); }
		
		cstring url;
		csprintf(url, "/v1/gamer/event/%s/%s" , domain && *domain ? domain : "private", gamerid);
		CHttpRequest *req = MakeHttpRequest(url);
		req->SetBody(aJSON->Duplicate());
		req->SetCallback(MakeBridgeCallback(onFinished));
		return http_perform(req);
	}

	/**
	 * Pop command issuing thread. Launched when logged in and stopped when tearing down the clan.
	 */
	class CClannishRESTProxy::PopEventLoopThread: public CThread {
		CClannishRESTProxy *self;
		bool stopped;

		bool AmIMainPopThread();
		void KillThread();

	public:
		chain<CEventListener> listeners;
		cstring domain;

		PopEventLoopThread(CClannishRESTProxy *parent, const char *domain) : self(parent), stopped(false), domain(domain) { CONSOLE_VERBOSE("Creating pop event loop for domain %s\n", domain);
		}
		void AddListener(CEventListener *listener) { if (listener) { listeners.Add(listener); } }
		void RemoveListener(CEventListener *listener) { if (listener) { listeners.Remove(listener); } }
		virtual void Run();
		void Terminate() { stopped = true; Join(); }
		~PopEventLoopThread() { printf("Removing event loop for %s\n", domain.c_str()); }
	};

	eErrorCode CClannishRESTProxy::RegisterEventListener(const char *domain, CEventListener *listener) {
		// Sanity checks
		if (!isSetup()) { return enSetupNotCalled; }
		if (!isLoggedIn()) { return enNotLogged; }
		if (!domain) { return enBadParameters; }

		// Already has this domain running?
		CMutex::ScopedLock lock (popEventThreadMutex);
		PopEventLoopThread *existingThread = FindEventThread(domain);
		if (existingThread) {
			// Do not add twice in the list
			FOR_EACH (CEventListener *l, existingThread->listeners) {
				if (l == listener) {
					return enEventListenerAlreadyRegistered;
				}
			}
			existingThread->AddListener(listener);
		} else {
			// Create a new thread
			autoref<PopEventLoopThread> thread;
			thread <<= new PopEventLoopThread(this, domain);
			thread->AddListener(listener);
			thread->Start();
			popEventThreads.push_back(thread);
		}
		return enNoErr;
	}

	eErrorCode CClannishRESTProxy::UnregisterEventListener(const char *domain, CEventListener *listener, bool acquireLock) {
		// Sanity checks
		if (!isSetup()) { return enSetupNotCalled; }

		// Try to find the handler
		CMutex::ConditionallyScopedLock lock (popEventThreadMutex, acquireLock);
		PopEventLoopThread *thread = FindEventThread(domain);
		if (!thread) { return enNoErr; }

		// Remove listener
		thread->RemoveListener(listener);
		// And remove the entry as well if no more listeners are registered
		if (thread->listeners.isEmpty()) {
			/**
			 * We do not wait on the thread, if it is currently running a request, it will
			 * continue until timeout, see shouldStop, discard the response and terminate.
			 */
			thread->Terminate();
			popEventThreads.erase(std::remove(popEventThreads.begin(), popEventThreads.end(), thread), popEventThreads.end());
		}
		return enNoErr;
	}

	CClannishRESTProxy::PopEventLoopThread *CClannishRESTProxy::FindEventThread(const char *domain) {
		FOR_EACH (PopEventLoopThread *t, popEventThreads) {
			if (t->domain.IsEqual(domain)) {
				return t;
			}
		}
		return NULL;
	}

	void CClannishRESTProxy::PopEventLoopThread::KillThread() {
		CMutex::ScopedLock lock (self->popEventThreadMutex);
		CONSOLE_WARNING("Killing thread for domain %s because of a network error\n", domain.c_str());
		// Removing all listeners will make the main code stop this thread
		FOR_EACH (CEventListener *l, listeners) {
			self->UnregisterEventListener(domain, l, false);
		}
	}

	void CClannishRESTProxy::StartEventListening() {
		RegisterEventListener(ADMIN_EVENT_DOMAIN, NULL);
	}

	void CClannishRESTProxy::StopEventListening() {
		CMutex::ScopedLock lock (popEventThreadMutex);
		// Unregister all event listeners and free memory
		FOR_EACH (PopEventLoopThread *t, popEventThreads) {
			t->Terminate();
		}
		// This doesn't actually end the threads; but removes them from the list (they still have a refcount > 0 because they retain themselves while running).
		// So they will be released when the thread finishes, we just won't interact with them anymore and let them die.
		popEventThreads.clear();
	}

	bool CClannishRESTProxy::PopEventLoopThread::AmIMainPopThread() {
		return domain.IsEqual(ADMIN_EVENT_DOMAIN);
	}

	void CClannishRESTProxy::PopEventLoopThread::Run() {
		cstring messageToAcknowledge;
		bool lastResultPositive = true, lastResultNetworkError = false, mainThread = AmIMainPopThread();
		owned_ref<CCloudResult> lastResult;

		// Thread end condition
		while (!stopped) {
			int delay = self->mPopEventLoopDelay;
			// On hold
			if (self->mSuspend) {
				CONSOLE_VERBOSE("Suspending pop thread %s\n", domain.c_str());
				self->suspendedThreadLock.Wait();
				// Wait between 0 to 5 sec to avoid all threads to wake up at the same time
				if (!AmIMainPopThread()) {
					std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 50) * 100));
				}
				continue;
			}

			// Process last request
			if (lastResult) {
				int status = lastResult->GetHttpStatusCode();
				// Chris request: sometimes ngnix returns a 499 instead of a 204 in case of timeout
				if (status == 499) { status = 204; }
				bool success = lastResult->GetErrorCode() == enNoErr && status < 300;
				bool networkError = (lastResult->GetErrorCode() == enNetworkError);

				// Network state notifications (on main thread). Unlike normal notifications, only notify actual network errors (not 5xx and such).
				if (mainThread) {
					struct SetNetworkThread: CCallback {
						_BLOCK1(SetNetworkThread, CCallback, CClannishRESTProxy*, self);
						void Done(const CCloudResult *result) {
							self->SetNetworkState(result->GetErrorCode() == enNoErr);
						}
					};
					// Run on main thread
					if (networkError != lastResultNetworkError) {
						CallbackStack::pushCallback(new SetNetworkThread(self), new CCloudResult(lastResult->GetErrorCode()));
					}
				}

				if (status == 200) {
					messageToAcknowledge = lastResult->GetJSON()->GetString("id");
					// Notify event on main thread
					struct NotifyEvent: CCallback {
						_BLOCK1(NotifyEvent, CCallback, PopEventLoopThread*, self);
						void Done(const CCloudResult *result) {
							FOR_EACH (CEventListener *l, self->listeners) {
								l->onEventReceived(self->domain, result);
							}
						}
					};
					CallbackStack::pushCallback(new NotifyEvent(this), lastResult.detachOwnership());
				}
				else if (status != 204 && lastResultPositive) {
					// Non retriable error -> kill ourselves
					bool needKill = (status >= 400 && status < 500);
					// Signal errors (do not signal multiple failures to avoid spam when offline)
					struct NotifyEvent: CCallback {
						_BLOCK3(NotifyEvent, CCallback,
							PopEventLoopThread*, self,
							eErrorCode, code,
							bool, killAfterwards);
						void Done(const CCloudResult *result) {
							// Notify
							FOR_EACH (CEventListener *l, self->listeners) {
								l->onEventError(code, self->domain, result);
							}
							// And kill if needed
							if (killAfterwards) {
								self->KillThread();
							}
						}
					};
					CallbackStack::pushCallback(new NotifyEvent(this, enServerError, needKill), new CCloudResult(enServerError, lastResult->GetJSON()->Duplicate()));
				}
				lastResultPositive = success;
				lastResultNetworkError = networkError;
				lastResult <<= NULL;
			}

			if (!lastResultPositive) {
				// Network down -- wait 20 sec to avoid bombing the poor internet
				CONSOLE_VERBOSE("Event thread for domain %s put on hold for %ds\n", domain.c_str(), EVENT_THREAD_HOLD);
				self->suspendedThreadLock.Wait(EVENT_THREAD_HOLD * 1000);
				if (mainThread) {
					// On the main thread, try again with a smaller timeout so that we can notify that the network is back as soon as the server is reached
					delay = POP_REQUEST_RECOVER_TIMEOUT;
				}
				// Do only continue if it still makes sense after the wait
				if (stopped) { break; }
				if (self->mSuspend) { continue; }
			}

			// Determine URL
			CUrlBuilder url("/v1/gamer/event");
			url.Subpath(domain).QueryParam("timeout", delay * 1000);
			if (messageToAcknowledge) {
				url.QueryParam("ack", messageToAcknowledge.c_str());
			}
			CONSOLE_VERBOSE("Pop request to %s\n", url.BuildUrl());

			// Blocking pop request
			CHttpRequest *req = self->MakeHttpRequest(url);
			req->SetCancellationFlag(&stopped);
			req->SetMethod("GET");
			req->SetRetryPolicy(CHttpRequest::NonpermanentErrors);
			req->SetTimeout(delay + 30);
			lastResult <<= http_perform_synchronous(req);

			// If the thread has been requested to terminate, don't process the response (CURLE_ABORTED_BY_CALLBACK = 42)
			if (lastResult->GetCurlErrorCode() == 42) {
				stopped = true;
			}
		}
	}

	CHttpRequest * CClannishRESTProxy::MakeUnauthenticatedHttpRequest(const char *url) {
		CHttpRequest *result = new CHttpRequest(url);
		result->SetHeader("x-apikey", mApiKey);
		result->SetHeader("x-apisecret", mApiSecret);
		result->SetHeader("x-sdkversion", mSdkVersion);
		return result;
	}

	CHttpRequest * CClannishRESTProxy::MakeHttpRequest(const char *url) {
		CHttpRequest *result = MakeUnauthenticatedHttpRequest(url);
		char basicAuthHeader[256];
		// Add basic authentication
		if (isLoggedIn()) {
			make_basic_authentication_header(mGamerId, mGamerSecret, basicAuthHeader, safe::charsIn(basicAuthHeader));
			result->SetHeader("Authorization", basicAuthHeader);
		}
		return result;
	}

	CHJSON *CClannishRESTProxy::MakeBodyWithOsn(const CHJSON *config) {
		CHJSON *result = NULL;
		if (config && config->Has("osn")) {
			result = new CHJSON;
			result->Put("osn", config->Get("osn"));
		}
		return result;
	}
}
