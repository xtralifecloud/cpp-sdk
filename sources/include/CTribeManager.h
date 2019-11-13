//
//  CTribeManager.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 16/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CTribeManager_h
#define XtraLife_CTribeManager_h

#include "XtraLife.h"
#include "CDelegate.h"

/*! \file CTribeManager.h
 */

namespace XtraLife
{
    namespace Helpers {
        class CHJSON;
    }

    /**
		The CTribeManager class is used to manage friends for a profile, and to allow
		this profile to send friendship requests to other profiles. This class manages
		all the notifications involved in the process.
	*/
	class FACTORY_CLS CTribeManager {
	public:
		/**
			* Use this method to obtain a reference to the CTribeManager.
			* @return the one and only instance of this manager
		*/
		static CTribeManager *Instance();
		
		/**
			Method used to check/retreive users from Clan of the Cloud community.
			@param aContainsString is a C string which is a pseudo, an email or displayName
			@param aLimit is the number of results you want to retrieve, default is 10
			@param aSkip is the number of results to skip before returning users. the default is 0
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			"count" : 50,
			"results" : [{
				"_id" : "xxxx",
				"profile" : {}
			}]
			} where count is the maximum possible anwsers
		*/
        void ListUsers(const char *aContainsString,  int aLimit, int aSkip, CResultHandler *aHandler);
        DEPRECATED void ListUsers(CResultHandler *aHandler, const char *aContainsString,  int aLimit, int aSkip);
		
		/**
			Method used to retrieve the application's friends of the currently logged in profile.
            @param aDomain is the domain where you want to get friends. "private" means it's for this
            game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			"friends" : [
				{"gamer_id": "5402c6c3bcf7c96fa04e625f", profile" :{}}
			],
		*/
        void ListFriends(const char *aDomain, CResultHandler *aHandler);
        DEPRECATED void ListFriends(CResultHandler *aHandler, const char *aDomain = "private");

		/**
			Method used to retrieve the application's blacklisted gamers of the currently logged in profile.
            @param aDomain is the domain where you want to get blacklist friends. "private" means it's for this
            game only.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			"blacklisted" : [
				{"gamer_id": "5402c6c3bcf7c96fa04e625f", profile" :{}}
			],
		*/
        void BlacklistFriends(const char *aDomain, CResultHandler *aHandler);
        DEPRECATED void BlacklistFriends(CResultHandler *aHandler, const char *aDomain = "private");

		/**
			Method used to change to a friendship inside the application.
			@param aOptions a json object which must contain
		 		"id" (string) : is a C string holding the gamer_id of the user to change the relation with.
				"status" (string) : is string with either:
						"add" : to be friend with
					 	"blacklist" : to blacklist the gamer
						"forget" : to remove any relationship
		 		"domain" (string) : the domain on which the friendship has to be modified.
		 		"osn" (json): an optional json object to specify the OS notification to be send (see the aNaotifiaction param of CUserManager::PushEvent)
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			"done" : 1
		 */
        void ChangeRelationshipStatus(const Helpers::CHJSON *aOptions, CResultHandler *aHandler);
        DEPRECATED void ChangeRelationshipStatus(CResultHandler *aHandler, const Helpers::CHJSON *aOptions);
		
		/**
			Method used to retrieve the best High Scores for this application.
			@param  aCount is the number of best scores you want to retrieve, default is 10
			@param  aPage is the number of the page to retrieve. the default is 1
			@param  aMode is a C string holding the game mode for which you want to retrieve
			the best High Scores.
            @param aDomain is the domain where you want to retrieve leaderboards. "private" means it's
            for this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			{ <aMode>: {
				"maxpage" : 10,
				"rankOfFirst" : 1,
				"page" : 1,
				"scores" : []
			}
		*/
        void FriendsBestHighScore(int aCount, int aPage, const char *aMode, const char *aDomain, CResultHandler *aHandler);
        DEPRECATED void FriendsBestHighScore(CResultHandler *aHandler, int aCount, int aPage, const char *aMode, const char *aDomain = "private");
		
		/**
			Method used to retrieve all the friends in an external network, for the currently logged in profile.
            @param aConfiguration is a JSON object which must contain
		 		- "network" : string, name of network in ("facebookId", "googleplusId", "gamecenter")
		 		- "friends" :  each key is a friend ID for the network and contains data that will be returning after matching. (for game center this param is ignored)
					i.e. : { "100199910": { "name" : "roland"},  "10015675710": { "name" : "mike"}}
		 		-  "automatching" : boolean (optionnal)
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result gives an indication if the request was correctly sent or not. enNoErr is
			sent if the request was sent.
		*/
        void ListNetworkFriends(const Helpers::CHJSON* aConfiguration, CResultHandler *aHandler);
        DEPRECATED void ListNetworkFriends(CResultHandler *aHandler, const Helpers::CHJSON* aConfiguration);
		DEPRECATED void ListNetworkFriends(CResultHandler *aHandler, const char *aNetwork, const Helpers::CHJSON* aFriends);

		/**
			Method used to retrieve random opponents for a match.
			@param  aFilter is a JSON object which describes the users' properties that should be matched
			to be considered for inclusion in the returned list.The syntax is the one used by MongoDB:
			http://docs.mongodb.org/manual/reference/operator/query/#query-selectors
            @param aDomain is the domain where you want to retrieve opponents. "private" means it's
            for this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
		*/
        void FindOpponents(const Helpers::CHJSON *aFilter, const char *aDomain, CResultHandler *aHandler);
        DEPRECATED void FindOpponents(CResultHandler *aHandler, const Helpers::CHJSON *aFilter, const char *aDomain="private");

        CTribeManager();
        ~CTribeManager();

        /**
         * You need to call this in CClan::Terminate().
         */
        void Terminate();

        friend struct singleton_holder<CTribeManager>;
        
    private:
        CGloballyKeptHandler<CResultHandler> &friendHandler;
	};
}


#endif
