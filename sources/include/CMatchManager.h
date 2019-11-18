//
//  CMatchManager.h
//  XtraLife
//
//  Created by Florian B on 28/11/14.
//  Copyright (c) 2014 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CMatchManager_h
#define XtraLife_CMatchManager_h

#include "XtraLife.h"
#include "XtraLifeHelpers.h"
#include "CDelegate.h"
#include "CUserManager.h"

/*! \file CMatchManager.h
 */
namespace XtraLife {
    namespace Helpers {
        class CHJSON;
    }
	struct CMatch;
	template<class T> struct chain;

	/**
	 * @defgroup matches Match API
	 * @{
	 * Special type for high level operations. Includes a match object that can be used for subsequent operations. Use MakeDelegate in inherit this. @code
		class MyClass {
			void Main() {
				CMatchResultHandler *handler = MakeDelegate(this, &MyClass::MatchDone);
			}

			void MatchDone(eErrorCode errorCode, const CCloudResult *result, Match *) {
			}
		};
		//
		// Or
		//
		class MyClass {
			void Main() {
				struct MatchDone: CMatchResultHandler {
					MatchDone() : CMatchResultHandler(this, &MatchDone::Done) {}
					void Done(eErrorCode errorCode, const CCloudResult *result, Match *) {
						// Match created.
					}
				};
				CMatchResultHandler *handler = new MatchDone;
			}
		}; @endcode
	*/
	typedef CDelegate<void (eErrorCode aErrorCode, const CCloudResult *aResult, CMatch *)> CMatchResultHandler;

	/**
	 * Implement this to get notified of events related to matches.
	 */
	struct FACTORY_CLS CMatchEventListener: Helpers::CRefClass {
		/**
		 * Called when an event is received. The event has a type, which can be determined and processed as follows:
		 @code
			// Default to "" for non-null string comparison
			const char *type = event->GetJSON()->GetString("type", "");
			if (!strcmp(type, "match.move")) {
				int change = event->GetJSON()->GetSafe("event")->GetSafe("move")->GetInt("change");
			}
		 @endcode
		 The following types of events may be received:
		 - match.joined: broadcasted when a player joins a match. The player himself doesn't receive the event.
		@code {
			"type": "match.join",
			"event": {
				"_id": "548866edba0ec600005e1941",
				"match_id": "548866ed74c2e0000098db95",
				"playersJoined": [{
					"gamer_id": "548866ed74c2e0000098db94",
					"profile": {
						"displayName": "Guest",
						"lang": "en"
					}
				}]
			},
			"id": "9155987d-6322-46d4-9403-e90ab4ba5fa3"
		} @endcode
		 - match.leave: broadcasted when a player leaves the match. The player himself doesn't receive the event.
		@code {
			"type": "match.leave",
			"event": {
				"_id": "548866edba0ec600005e1941",
				"match_id": "548866ed74c2e0000098db95",
				"playersLeft": [{
					"gamer_id": "548866ed74c2e0000098db94",
					"profile": {
						"displayName": "Guest",
						"lang": "en"
					}
				}]
			},
			"id": "9155987d-6322-46d4-9403-e90ab4ba5fa3"
		} @endcode
		 - match.finish: broadcasted to all participants except the one who initiated the request when a match is finished.
		@code {
			"type": "match.finish",
			"event": {
				"_id": "54784d07a0fcf2000086457d",
				"finished": 1,
				"match_id": "54784d07f10c190000cb96e3"
			},
			"id": "d2f908d1-40d8-4f73-802e-fe0086bb6cd5"
		} @endcode
		 - match.move: broadcasted when a player makes a move. The player himself doesn't receive the event.
		@code
		{
			"type": "match.move",
			"event": {
				"_id": "54784176a8df72000049340e",
				"player_id": "547841755e4cfe0000bf6907",
				"move": { "what": "changed" }
			},
			"id": "d0fb71c1-3cd0-4bbd-a9ec-29cb46cf9203"
		} @endcode
		 - match.invite: received by another player when someone invites him to the match.
		@code {
			"type": "match.invite",
			"event": {
				"_id": "54784d07a0fcf2000086457d",
				"match_id": "54784d07f10c190000cb96e3"
				"inviter": {
					"gamer_id": "5487260cf45e5f84c7cd7223",
					"profile": {
						"displayName": "Guest",
						"lang": "en"
					}
				}
			},
			"id": "d2f908d1-40d8-4f73-802e-fe0086bb6cd9"
		} @endcode
		*/
		virtual void onMatchEventReceived(const CCloudResult *aEvent) = 0;
	};

	/**
	 * The CMatchManager class is used to manage matches in a raw way.
	 * It also provides a higher level class which can be used to easily run a match at an higher level.
	 * Some methods accept a parameter named `osn`. This parameter can be used to forward a push notification to the users who are not active at the moment. It is typically a JSON with made of attributes which represent language -> message pairs. Here is an example: {"en": "Help me!", "fr": "Aidez moi!"}.
	 */
	struct FACTORY_CLS CMatchManager {

		static CMatchManager *Instance();
		void Terminate();

		//////////////////////////// High level API ////////////////////////////
		/**
		 * High level method which creates a match and passes it to the result handler in the form of a CMatch.
		 * Memory management of #CMatch objects is described on the class itself.
         * @param aHandler match result handler, to be called when the operation is completed
		 * @param aConfiguration configuration of the match, as described in CMatchManager#CreateMatch
		 */
		void HLCreateMatch(CMatchResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);
        
		/**
		 * High level method to join a match, resulting in a CMatch object allowing for easy manipulation
		 * on the match.
         * @param aHandler match result handler, to be called when the operation is completed
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match to join. Needs to be either extracted from an invitation or simply by
		   listing matches (see also #ListMatches).
		 Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
		 */
		void HLJoinMatch(CMatchResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);
        
		/**
		 * High level method to fetch a CMatch object corresponding to a match which the player already belongs to.
		 * It basically allows to continue the game in the same state as when the match has initially been created
		 * or joined to, all in a transparent manner.
         * @param aHandler match result handler, to be called when the operation is completed
		 * @param aConfiguration is a JSON object. It may contain the following keys:
		 - "id" (string): ID of the match to resume. If may not be passed, in which case the last match from
		   the given domain to which the user is participating will be used.
		 - "domain" (string): the domain on which the match to be resumed belongs; not used if `id` is passed
		   and defaulted to private if not passed.
		 */
		void HLRestoreMatch(CMatchResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);
        
		/**
		 * Destroys a match object (not actually deleting the match or anything, just freeing the resource
		 * associated with the match).
		 * @param aMatch match object to be dismissed
		 */
		void HLDestroyMatch(CMatch *aMatch);

		//////////////////////////// Raw API ////////////////////////////

		/**
		 * Creates a match, available for joining by others players.
		 @param aConfiguration is a JSON object holding the parameters of the match to create.
		 It may contain the following:
		 - "domain" (string): the domain on which to create the match.
		 - "maxPlayers" (number): the maximum number of players who may be in the game at a time.
		 - "description" (optional string): string describing the match (available for other who want to join).
		 - "customProperties" (optional object of strings or numbers): freeform object containing the properties of the match,
		   which may be used by other players to search for a suited match.
		 - "shoe" (optional array of objects): freeform object containing a list of objects which will be shuffled upon match
		   creation. This offers an easy way to make a random generator that is safe, unbiased (since made on the server) and
		   can be verified by all players once the game is finished.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain: @code
		 "match": {
		     "domain" (string): the domain to which the match belongs.
			 "status" (string): the status of the game (running, finished).
			 "description" (string): description of the match, defined by the user upon creation.
			 "customProperties" (object of strings): custom properties, as passed.
			 "events" (object of events): list of existing events, which may be used to reproduce the state of the game.
			 "maxPlayers" (number): as passed.
			 "players" (array of strings): the list of players IDs, should only contain you for now.
			 "globalState" (object of strings): the global state of the game, which may be modified using a move.
			 "lastEventId" (string or 0): the ID of the last event happened during this game; keep this for later.
			 "match_id" (string): the ID of the created match. Keep this for later.
			 "seed" (31-bit integer): a random seed that can be used to ensure consistent state across players of the game
			 "shoe" (array of objects): an array of objects that are shuffled when the match starts. You can put anything
			 you want inside and use it as values for your next game. This field is only returned when finishing a match.
		 } @endcode
		 */
        void CreateMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Lists the matches available to join.
		 @param aConfiguration is a JSON object allowing to filter the matches to be returned.
		 You may pass any of these:
		 - "domain" (string): the domain on which to look for available matches.
		 - "filter" (object of string): a list of properties that need to match inside the `customProperties` field of a
		   match. Do NOT pass "participating" if you pass this attribute.
		 - "participating" (anything): pass this attribute along with any value (but NO filter in that case) to list all
		   matches to which you are participating.
		 - "invited" (anything): filter out by matches you are invited to.
		 - "finished" (anything): also include finished games.
		 - "full" (anything): pass this attribute to also include games where the maximum number of player has been reached.
		 - "limit" (number): the maximum number of results to return. For pagination, defaulting to 30.
		 - "skip" (number): the first element to start from. Use skip=30&limit=30 to fetch the second page.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "matches" (array of objects): match objects as described in #CreateMatch
		 */
        void ListMatches(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Fetches the latest info about a match.
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): fetched match object as described in #CreateMatch
		 */
        void FetchMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Joins a match. Other players will be notified in the form of an event of type 'match.join'.
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match to join.
		 Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch. Please take note of the `lastEventId` contained.
		 */
        void JoinMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Allows to invite a player to join a match. You need to be part of the match to send an invitation.
		 * This can be used for private matches as described in the chapter @ref matches.
		 @param aConfiguration is a JSON object.
		 The manadatory keys are:
		 - "id" (string): the ID of the match to invite the player to.
		 - "gamer_id" (string): the ID of the player to invite.
		 Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch with low information detail.
		 */
        void InvitePlayer(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Dismisses a pending invitation on a given match. Fails if the currently logged in player was not invited.
		 @param aConfiguration is a JSON object. Must contain:
		 - "id" (string): the ID of the match from which to remove the invitation.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch with low information detail.
		 */
        void DismissInvitation(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Posts a move in the match, notifying other players in the form of an event of type 'match.move'.
		 @param aConfiguration is a JSON object.
		 It may contain the following:
		 - "id" (string): the ID of the match in which to post the move.
		 - "move" (object of strings or numbers): freeform object describing the move made by the player; will be reported to other players and left pending in the queue of the match.
		   May be used by newcomers to reproduce the same game state locally as a player who joined earlier (events would then be "replayed" locally).
		 - "globalState" (optional object of strings or numbers): freeform object describing the global state of the game.
		   When passed, it indicates that there is a consistent game state that newcomers may use to start the game from.
		   As such, the list of pending events in the match will be cleared (but people who are currently in the game will still receive all notifications triggered since they joined).
		   Thus, you should consider passing a global state sometimes, it will ease the work for newcomers.
		 - "lastEventId" (string): the ID of the last event received for this match.
	     Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch. Please take note of the `lastEventId` contained.
		 */
        void PostMove(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Draws one or more randomized items from the shoe (see #CreateMatch).
		 @param aConfiguration is a JSON object.
		 The following attributes are mandatory:
		 - "id" (string): the ID of the match.
		 - "lastEventId" (string): the ID of the last event received for this match.
		 And optionally:
		 - "count" (integer): the number of items to draw (defaults to 1).
		 - "osn" (object): please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "drawnItems" (array of objects): the objects drawn as passed in the shoe element at #CreateMatch.
		 - "match" (object): updated match object as described in #CreateMatch. Please take note of the `lastEventId` for next requests.
		 */
        void DrawFromShoe(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Leaves a match. Only works if you have joined it prior to calling this. Other players will be notified in the form of an event of type 'match.leave'.
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match to leave.
		 Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch.
		 */
        void LeaveMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Finishes a match. Only works if you are the one who created it in the first place. Other players will be notified in the form of an event of type 'match.finish'.
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match to leave.
		 - "lastEventId" (string): the ID of the last event received for this match.
		    You received it either as a response of an operation on a match (such as when joining, or a move done by you) or as an event later (in case pass the `_id` of the last event received).
		Optionally, an `osn` node can be passed. Please see the definition of the CMatchManager class for more information about this.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in #CreateMatch. This key is NOT present if the `delete` attribute was passed.
		 */
        void FinishMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Deletes a match. Only works if you are the one who created it and it is already finished.
		 @param aConfiguration is a JSON object.
		 The mandatory keys are:
		 - "id" (string): the ID of the match to delete.
         @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 @result if noErr, the json passed to the handler may contain:
		 - "done" (number): 1 if done properly.
		 */
        void DeleteMatch(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

	private:
		friend struct singleton_holder<CMatchManager>;

		CMatchManager();
		~CMatchManager();
	};
	
	/**
	 * Represents a match with which you can interact through high level functionality.
	 *
	 * Memory management works as follows: in the CMatchResultHandler, you get a transient CRefClass object,
	 * which you have ownership on until you call CMatchManager#HLDestroyMatch. Thus, you may use autoref or
	 * call Retain/Release, but you shouldn't expect the CMatch to be usable beyond a corresponding call to
	 * CMatchManager#HLDestroyMatch.
	 */
	struct FACTORY_CLS CMatch : CEventListener {
		enum State {
			RUNNING = 1, FINISHED,
		};

		/**
		 * Constructs a high level match object allowing to use high level functions to control your match.
		 * You must call this with the result of one of the following calls:
		 * - CMatchManager#CreateMatch (will result in a match that you are administrating)
		 * - CMatchManager#JoinMatch (will result in a normal match, except if you actually created it)
		 * - CMatchManager#FetchMatch (same as join, but works only for a match that you have actually joined/created first)
		 * - CMatchManager#ListMatches, though you should avoid using it; it takes the last entry, so it works if you passed the `participating` parameter and are only participating to one match at a time.
		 * You should never call this constructor by yourself, but instead get your object as a result of a call like HLCreateMatch.
		 * @param matchManager current manager, used internally to prevent the use of this object after terminate
		 * @param result result received from a previous call as documented above
		 */
		CMatch(CMatchManager *matchManager, const CCloudResult *result);
		~CMatch();
		/**
		 * @return the ID of the gamer (uses the UserManager for that purpose).
		 */
		const char *GetGamerId();
		/**
		 * This could be useful if you intend to use the raw API with an high level match, else forget it.
		 * @return the ID of the last processed event
		 */
		const char *GetLastEventId();
		/**
		 * @return the match ID, may be useful to join a match later or invite friends.
		 */
		const char *GetMatchId();
		/**
		 * @return the list of players as a JSON, that you can iterate like that: @code
			for (const CHJSON *node: *match->GetPlayers()) {
				printf("Player ID: %s\n", node->GetString("gamer_id"));
			} @endcode
		 */
		const Helpers::CHJSON *GetPlayers();
		/**
		 * @return the current status of the match.
		 */
		State GetStatus();
		/**
		 * @return whether you are the creator of the match, and as such have special privileges (like the ability to finish and delete a match).
		 */
		bool IsCreator();
		/**
		 * Registers an event listener for this match. The benefit over the method in CMatchManager is that the events
		 * actually broadcasted are limited to those concerning this match.
		 * @param aEventListener the listener to be called whenever an event comes for this match. It
		 * is a CRefClass, so if you don't need it, you can call Release(). The system will keep it as
		 * long as needed and free it then. As such, it is common to call Release() immediately after
		 * having registered an event listener. Never call delete on it.
		 */
		void RegisterEventListener(CMatchEventListener *aEventListener);
		/**
		 * Unregisters a previously registered event listener.
		 * @param aEventListener event listener to release (will free the memory if you released it on your side as well)
		 */
		void UnregisterEventListener(CMatchEventListener *aEventListener);
		/**
		 * Posts a move to other players.
         * @param aHandler the result handler, which may be called synchronously
		 * @param aMoveData a freeform JSON indicating your move so that others can track your progress
		 * @param aOptionalUpdatedGameState a freeform JSON replacing the global game state, to be used by players who join from now on
		 * @param aOptionalAdditionalData if not null, can contain additional configuration. Currently supported nodes are 'osn'.
		 * Please see the definition of the CMatchManager class for more information about this.
		 * @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in CMatchManager#CreateMatch.
		 */
		void PostMove(CMatchResultHandler *aHandler, const Helpers::CHJSON *aMoveData, const Helpers::CHJSON *aOptionalUpdatedGameState, const Helpers::CHJSON *aOptionalAdditionalData);

		/**
		 * Draws an item from the shoe.
         * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aCount the number of items to draw from the shoe.
		 * @param aOptionalAdditionalData if not null, can contain additional configuration. Currently supported nodes are 'osn'.
		 * Please see the definition of the CMatchManager class for more information about this.
		 * @result if noErr, the json passed to the handler may contain:
		 - "drawnItems" (array of objects): the objects drawn as passed in the shoe element at CMatchManager#CreateMatch.
		 - "match" (object): updated match object as described in CMatchManager#CreateMatch.
		 */
		void DrawFromShoe(CMatchResultHandler *aHandler, int aCount, const Helpers::CHJSON *aOptionalAdditionalData);

		/**
		 * Leaves the match.
         * @param aHandler the result handler, which may be called synchronously
		 * @param aOptionalAdditionalData if not null, can contain additional configuration. Currently supported nodes are 'osn'.
		 * Please see the definition of the CMatchManager class for more information about this.
		 * @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in CMatchManager#CreateMatch.
		 */
		void Leave(CMatchResultHandler *aHandler, const Helpers::CHJSON *aOptionalAdditionalData);

		/**
		 * Requires to be the creator of the match.
         * @param aHandler the result handler, which may be called synchronously
		 * @param aDeleteToo if true, deletes the match if it finishes successfully or is already finished
		 * @param aOptionalAdditionalData if not null, can contain additional configuration. Currently supported nodes are 'osn'.
		 * Please see the definition of the CMatchManager class for more information about this.
		 * @result if noErr, the json passed to the handler may contain:
		 - "match" (object): updated match object as described in CMatchManager#CreateMatch if the match hasn't been deleted.
		 - "done" (number): set to 1 if the match has been successfully deleted as requested
		 */
		void Finish(CMatchResultHandler *aHandler, bool aDeleteToo, const Helpers::CHJSON *aOptionalAdditionalData);

	private:
		CMatchManager *expectedManager;
		Helpers::cstring lastEventId;
		chain<CMatchEventListener> &eventListeners;
		// Stores a part of the match data only (the players are separated because updated frequently, and some properties are not included)
		Helpers::owned_ref<Helpers::CHJSON> matchData, playerData;

		bool CheckManager();
		void UpdateFromMatchData(const Helpers::CHJSON *json);
		const char *GetDomain();

		virtual void onEventReceived(const char *aDomain, const CCloudResult *aEvent);
		virtual void onEventError(eErrorCode aErrorCode, const char *aDomain, const CCloudResult *result);
	};
	/** @} */
}

#endif
