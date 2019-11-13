//
//  CGameManager.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 10/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CGameManager_h
#define XtraLife_CGameManager_h

#include "XtraLife.h"
#include "XtraLifeHelpers.h"
#include "CDelegate.h"

/*! \file CGameManager.h
 */

namespace XtraLife
{
	class CClan;
	class CCloudResult;

	/** The CGameManager class is helpful when you want to store global data for your
		application. These data will be accessible to all users who have installed and
		are using this application. It can be useful for introducing new levels, new
		assets, or anything that you wish to add to the application in future updates.
		It also provides helper methods to store and retrieve high scores.
	*/
	class FACTORY_CLS CGameManager {
	public:

		/**
			* Use this method to obtain a reference to the CGameManager.
			* @return the one and only instance of this manager
		*/
		static CGameManager *Instance();
		
		/** Method used to post a High Score for this application.
			@param  aHighScore is the score you want to post.
			@param  aMode is a C string holding the game mode for which this score should be
			considered.
			@param	aScoreType string with "hightolow" or "lowtohigh"
			@param  aInfoScore is an optional string wich describe the score
			@param  aForce is an optional boolean used in case we authorize the new high score
			to be inferior to the previous one.
            @param  aDomain is the domain in which the score has to be pushed. "private" means it's
            local to this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain: @code
			"done" : 1,
			"rank" : 123 @endcode
			`done` means that the score has been recorded as the highest, while `rank` is the effective rank
			of the player (returned even if the player has not actually been classed, that is if `done` is 0).
		*/
        void Score(long long aHighScore, const char *aMode, const char *aScoreType, const char *aInfoScore, bool aForce, const char *aDomain, CResultHandler *aHandler);

		/** Method used to retrieve the rank of a High Score for this application.
			@param  aHighScore is the score whose rank you want to retrieve.
			@param  aMode is a C string holding the game mode for which you want to retrieve
			rank for this score.
            @param  aDomain is the domain in which the rank has to be retrieved. "private" means it's
            local to this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			 "rank" : 1
		*/
        void GetRank(long long aHighScore, const char *aMode, const char *aDomain, CResultHandler *aHandler);

		/** Method used to retrieve the best High Scores for this application.
			@param  aCount is the number of best scores you want to retrieve, default is 10
			@param  aPage is the number of the page to retrieve. the default is 1
			@param  aMode is a C string holding the game mode for which you want to retrieve
			the best High Scores.
            @param  aDomain is the domain in which the best scores have to be retrieved. "private"
            means it's local to this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			{ <aMode>: {
				"maxpage" : 10,
				"rankOfFirst" : 1,
				"page" : 1,
				"scores" : []
			}
		*/
        void BestHighScore(int aCount, int aPage, const char *aMode, const char *aDomain, CResultHandler *aHandler);
		
		/** Method used to retrieve scores centered on the user score for this application.
			@param  aCount is the number of best scores you want to retrieve, default is 10
			@param  aMode is a C string holding the game mode for which you want to retrieve
			the best High Scores.
            @param  aDomain is the domain in which the best scores have to be retrieved. "private"
            means it's local to this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			{ <aMode>: {
				"maxpage" : 10,
				"rankOfFirst" : 1,
				"page" : 1,
				"scores" : []
			}
		*/
        void CenteredScore(int aCount, const char *aMode, const char *aDomain, CResultHandler *aHandler);
		
		/** Method used to retrieve the best Scores of the logged user.
            @param  aDomain is the domain in which the best scores have to be retrieved. "private"
            means it's local to this game only.
            @param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain list of :
			<modes>:	{
			"timestamp":	"2014-09-12T15:30:56.938Z",
			"score":	100,
			"info":	"some text",
			"order":	"lowtohigh",
			"rank":	3
			}
		 */
        void UserBestScores(const char *aDomain, CResultHandler *aHandler);

		/**
			Method to get a single key of the global JSON object stored for this game and domain.
			@param aConfiguration is a JSON configuration, that may contain
			- domain: the domain on which the action is to be taken (if not passed, the private domain is used)
			- key: name of the key to retrieve from the global JSON object (if not passed, all keys are returned)
			@param aHandler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr and no binary key was passed in the configuration, the json passed to the handler may contain:
			{
				"<key1>" : "value1", "<key2>" : "value2", ...
			}
		 */
        void GetValue(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);
		
		/**
			 Method to read a single key containing binary data stored for this domain.
			 @param aConfiguration JSON allowing for extensible configuration, that may contain:
			 - domain: the domain on which the action is to be taken (if not passed, the private domain is used)
			 - key: name of the key to retrieve
			 @param aHandler result handler whenever the call finishes (it might also be synchronous)
			 @result if noErr, the json passed to the handler may contain:
			 {
			 "url" : "<signed URL>",
			 }
			 CCloudResult.HasBinary() must be true and you can acces to the data through :
		 */
        void GetBinary(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);
		
		/**
		 * Run a batch on the server side.
		 * Batch is edited on BackOffice server.
		 * @param aConfiguration JSON data describing the dat to pass to the batch
		 	- domain: the domain on which the action is to be taken (if not passed, the private domain is used)
		 	- name: the name identifying the batch
		 * @param aParameters JSON data describing the data to pass to the batch
			- input: user defined data
		 * @param aHandler the handler called with the result when the call completes
		 * @result if noErr, the json passed to the handler will contain the result of the executed batch
		 */
		void Batch(CResultHandler *aHandler, const Helpers::CHJSON *aConfiguration, const Helpers::CHJSON *aParameters);

        DEPRECATED void Score(CResultHandler *aHandler, long long aHighScore, const char *aMode, const char *aScoreType, const char *aInfoScore, bool aForce, const char *aDomain="private");
        DEPRECATED void GetRank(CResultHandler *aHandler, long long aHighScore, const char *aMode, const char *aDomain="private");
        DEPRECATED void BestHighScore(CResultHandler *aHandler, int aCount, int aPage, const char *aMode, const char *aDomain="private");
        DEPRECATED void CenteredScore(CResultHandler *aHandler, int aCount, const char *aMode, const char *aDomain="private");
        DEPRECATED void UserBestScores(CResultHandler *aHandler, const char *aDomain="private");
        DEPRECATED void BinaryRead(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);
        DEPRECATED void KeyValueRead(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

    private:
		/**
		 * Not publicly instantiable/subclassable. Use Instance().
		 */
		CGameManager();
		~CGameManager();
		/**
		 * You need to call this in CClan::Terminate().
		 */
		void Terminate();

        void binaryReadDone(const CCloudResult *result, CResultHandler *aHandler);
        void getBinaryDone(const CCloudResult *result, CResultHandler *aHandler);

		friend class CClan;
		friend struct singleton_holder<CGameManager>;
	};
	
}


#endif
