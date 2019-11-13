//
//  CClannishDBProxy.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 15/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef CotClib_CConnectionREST_h
#define CotClib_CConnectionREST_h

#include "CClan.h"
#include "CHJSON.h"
#include "CDelegate.h"
#include "XtraLifeHelpers.h"
#include "XtraLife_thread.h"
#include "Misc/helpers.h"

#define ADMIN_EVENT_DOMAIN "private"

namespace XtraLife {

    class CEventListener;
    class CHttpRequest;

    typedef enum  etTypeFS {
		fsApp = 1,
		fsAppUser,
		fsAppMatch
	} etTypeFS;

	class CClannishRESTProxy {
	public:
		
		static CClannishRESTProxy *Instance();

		bool isSetup();
		bool isLoggedIn();
		void SetNetworkState(bool on);
		
		void Ping(CInternalResultHandler *onFinished);
		
		void Setup(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void Login(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void LoginAnonymous(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void LogWithExternalNetwork(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void Logout(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void Convert(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void MailPassword(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void ChangePassword(const char *aNewPassword, CInternalResultHandler *onFinished);
		void ChangeEmail(const char *aNewEmail, CInternalResultHandler *onFinished);

		void BatchGame(const Helpers::CHJSON *ajSON, const Helpers::CHJSON *aInput, CInternalResultHandler *onFinished);
		void BatchUser(const Helpers::CHJSON *ajSON, const Helpers::CHJSON *aInput, CInternalResultHandler *onFinished);

		void CheckUser(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void UserExist(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void ListUsers(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		
		void ListFriends(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void BlacklistFriends(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void ChangeRelationshipStatus(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);

		void RegisterDevice(const char *os, const char *token, CInternalResultHandler *onFinished);
		void UnregisterDevice(const char *os, const char *token, CInternalResultHandler *onFinished);

		void Outline(CInternalResultHandler *onFinished);

		void GetGodfatherCode(const char *aDomain, CInternalResultHandler *onFinished);
		void GetGodfather(const char *aDomain, CInternalResultHandler *onFinished);
		void SetGodfather(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void GetGodchildren(const char *aDomain, CInternalResultHandler *onFinished);
	
		void GetUserProfile(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);
		void SetUserProfile(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);

		void UserSetProperties(const char *aDomain, const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void UserGetProperties(const char *aDomain, CInternalResultHandler *onFinished);
		void UserSetProperty(const char *aDomain, const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void UserGetProperty(const char *aDomain, const char *key, CInternalResultHandler *onFinished);
		void UserDelProperty(const char *aDomain, const char *key, CInternalResultHandler *onFinished);
		void FindOpponents(const char *aDomain, const Helpers::CHJSON *ajSON,  CInternalResultHandler *onFinished);

		/**
		 * @param aJSON configuration for the transaction (directly forwarded to the /v2.2/gamer/tx API call)
		 * @param useV2_2 should be set to true, else will perform the call on the legacy resource (v1)
		 * @param onFinished handler for the result (which contents will differ depending on whether useV2_2 was passed)
		 */
		void Transaction(const Helpers::CHJSON *aJSON, bool useV2_2, CInternalResultHandler *onFinished);
		void Balance(const char *domain, const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void TxHistory(const char *domain, const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);

        void vfsRead(const char *domain, const char *key, CInternalResultHandler *onFinished);
        void vfsWrite(const char *domain, const char *key, const Helpers::CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished);
        void vfsReadv3(const char *domain, const char *key, CInternalResultHandler *onFinished);
        void vfsWritev3(const char *domain, const char *key, const Helpers::CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished);
		void vfsDelete(const char *domain, const char *key, bool isBinary, CInternalResultHandler *onFinished);
		void UploadData(const char *url, const void *ptr, size_t size, CInternalResultHandler *onFinished);
		void DownloadData(const char *url, CInternalResultHandler *onFinished);
	   
		void vfsReadGame(const char *domain, const char *key, CInternalResultHandler *onFinished);
		void vfsWriteGame(const char *domain, const char *key, const Helpers::CHJSON *aJSON, bool isBinary, CInternalResultHandler *onFinished);
		void vfsDeleteGame(const char *domain, const char *key, bool isBinary, CInternalResultHandler *onFinished);
        void vfsReadGamev3(const char *domain, const char *key, CInternalResultHandler *onFinished);

		void Score(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void FriendsBestHighScore(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void GetRank(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void BestHighScore(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void CenteredScore(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void UserBestScore(const char *domain, CInternalResultHandler *onFinished);
		void GetRankArray(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void BestHighScoreArray(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);

		void LinkWith(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void Unlink(const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);

		void GetNetworkFriends(const char *network, const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);

		void InvitByMail(const Helpers::CHJSON *ajSON, CInternalResultHandler *onFinished);

		// configuration should contain: domain
		void ListAchievements(const Helpers::CHJSON *configuration, CInternalResultHandler *onFinished);
		void SetAchievementGamerData(const char *domain, const char *achName, const Helpers::CHJSON *data, CInternalResultHandler *onFinished);
		void PushEvent(const char *domain, const char *gamerid, const Helpers::CHJSON *aJSON, CInternalResultHandler *onFinished);
		void ResetAchievements(const char *domain, CInternalResultHandler *onFinished);

		CCloudResult *ForceActivate();

		// Match API
		void CreateMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void FinishMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void DeleteMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void JoinMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void InvitePlayerToMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void DismissInvitationToMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void DrawFromShoeInMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void LeaveMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void FetchMatch(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void ListMatches(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void PostMove(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		
		// Store API
		void GetProductList(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void GetPurchaseHistory(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void SendAppStorePurchaseToServer(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);

		// Index API
		void DeleteIndexedObject(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void GetIndexedObject(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void IndexObject(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);
		void SearchIndexedObjects(const Helpers::CHJSON *config, CInternalResultHandler *onFinished);

		/**
		 * Registers an event listener for a given event domain.
		 * @param domain domain to listen for, such as "/private" (use NULL for default)
		 * @param listener listener to receive events
		 * Note about memory management: when you inherit from CEventListener, your class becomes a
		 * CRefClass. The system will keep a reference to it while registered, and call Release when
		 * it is unregistered. Thus, if you want the object to be released automatically when
		 * unregistered, simply call Release() on it:
			CEventListener *listener = new MyEventListener;
			
		 */
		eErrorCode RegisterEventListener(const char *domain, CEventListener *listener);
		/**
		 * Unregisters an event listener, killing the associated thread when no more listeners are left for a given domain.
		 * @param domain the domain for which the listener was registered
		 * @param listener the listener to remove (Release will be called on it, potentially deleting it)
		 * @param acquireLock internal parameter, leave it to true when using from outside
		 */
		eErrorCode UnregisterEventListener(const char *domain, CEventListener *listener, bool acquireLock = true);
		void StartEventListening();
		void StopEventListening();
		
		void Suspend();
		void Resume();

		const char *GetGamerID();
		const char *GetNetworkID();
		const char *GetNetwork();
		const char *GetDisplayName();
		const char *GetMail();
		const char *GetAppID();
		
		bool isLinkedWith(const char *network) { return IsEqual(mNetwork, network) || mLinks->Has(network);  }

		bool autoRegisterForNotification() { return mRegisterForNotification; }
		
	private:
		// Background thread issuing "pop" commands to the server.
		class PopEventLoopThread;
		Helpers::CMutex popEventThreadMutex;
		Helpers::CConditionVariable suspendedThreadLock;

		/** singleton */
		Helpers::CHJSON *mQueuesURL;
		Helpers::CHJSON *mJSONBatch;

		Helpers::cstring mApiKey, mApiSecret;
		Helpers::cstring mGamerId, mGamerSecret;
		Helpers::cstring mNetwork, mNetworkId;
		Helpers::cstring mDisplayName, mEmail;
		Helpers::cstring mAppID, mAppVersion, mSdkVersion;
		std::vector< Helpers::autoref<PopEventLoopThread> > popEventThreads;
		int mPopEventLoopDelay;			// in sec

		Helpers::CHJSON *mLinks;
		
		bool mNetSate, mSuspend;
		bool mRegisterForNotification;
		
		friend struct singleton_holder<CClannishRESTProxy>;
		friend void CClan::Terminate();

		// Can not be subclassed
		CClannishRESTProxy();
		virtual ~CClannishRESTProxy();
		/**
		 * You need to call this in CClan::Terminate().
		 */
		void Terminate();

		bool HandleEvent(const Helpers::CHJSON *ajSON);
		// Please acquire popEventThreadMutex before calling this!
		PopEventLoopThread *FindEventThread(const char *domain);

		CCloudResult *LoginResultHandler(CCloudResult *result);
		CCloudResult *LogoutResultHandler(CCloudResult *result);

		/**
		 * From a configuration JSON that potentially has an OSN field, builds a body containing only this node.
 		 * @param config configuration (that may be null), possibly containing an `osn` node
		 * @return either NULL or a CHJSON with only the `osn` node from the original `config`  parameter
		 */
		Helpers::CHJSON *MakeBodyWithOsn(const Helpers::CHJSON *config);

		/**
		 * Builds an HTTP request targetting our server, without adding credentials.
		 * @param url URL relative to the server (e.g. /api/login)
		 */
		CHttpRequest *MakeUnauthenticatedHttpRequest(const char *url);
		/**
		 * Builds an HTTP request targetting our server, adding in the credentials if needed.
		 * @param url URL relative to the server (e.g. /api/login)
		 */
		CHttpRequest *MakeHttpRequest(const char *url);
	};
	
}

#endif
