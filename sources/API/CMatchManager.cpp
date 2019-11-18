#include "include/CMatchManager.h"
#include "include/CHJSON.h"
#include "Core/CCallback.h"
#include "Core/CClannishRESTProxy.h"
#include "Core/XtraLife_private.h"
#include "Misc/helpers.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

static singleton_holder<CMatchManager> managerSingleton;

//////////////////////////// Basic ////////////////////////////
namespace XtraLife {
	void InvokeHandler(CMatchResultHandler *handler, const CCloudResult *result, CMatch *match);
	void InvokeHandler(CMatchResultHandler *handler, eErrorCode code, const char *message = NULL);
}

static CClannishRESTProxy *REST() { return CClannishRESTProxy::Instance(); }
	
//////////////////////////// Common manager stuff ////////////////////////////
CMatchManager::CMatchManager() {}
CMatchManager::~CMatchManager() {}
CMatchManager* CMatchManager::Instance() { return managerSingleton.Instance(); }
void CMatchManager::Terminate() { managerSingleton.Release(); }

//////////////////////////// High-level matches ////////////////////////////
void CMatchManager::HLCreateMatch(CMatchResultHandler *handler, const CHJSON *matchConfig) {
	struct Delegate: CInternalResultHandler {
		_BLOCK2(Delegate, CInternalResultHandler,
		        CMatchManager*, self,
		        CMatchResultHandler*, handler);

		void Done(const CCloudResult *result) {
			InvokeHandler(handler, result,
				result->GetErrorCode() == enNoErr ? new CMatch(self, result) : NULL);
		}
	};
	REST()->CreateMatch(matchConfig, new Delegate(this, handler));
}

void CMatchManager::HLJoinMatch(CMatchResultHandler *handler, const CHJSON *configuration) {
	struct Delegate: CInternalResultHandler {
		_BLOCK2(Delegate, CInternalResultHandler,
				CMatchManager*, self,
				CMatchResultHandler*, handler);

		void Done(const CCloudResult *result) {
			InvokeHandler(handler, result,
				result->GetErrorCode() == enNoErr ? new CMatch(self, result) : NULL);
		}
	};
	REST()->JoinMatch(configuration, new Delegate(this, handler));
}

void CMatchManager::HLRestoreMatch(CMatchResultHandler *handler, const CHJSON *configuration) {
	const char *matchId = configuration->GetString("id"), *domain = configuration->GetString("domain");
	struct ResumeMatch: CInternalResultHandler {
		_BLOCK2(ResumeMatch, CInternalResultHandler,
			CMatchManager*, self,
			CMatchResultHandler*, handler);

		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() == enNoErr &&
				(result->GetJSON()->GetSafe("matches")->size() > 0 || result->GetJSON()->Has("match")))
			{
				InvokeHandler(handler, result, new CMatch(self, result));
			} else {
				InvokeHandler(handler, enNoMatchToResume, "Unable to fetch match");
			}
		}
	};
	// Either resume a given match, either resume the last one
	CHJSON config;
	if (matchId) {
		config.Put("id", matchId);
		REST()->FetchMatch(&config, new ResumeMatch(this, handler));
	} else {
		struct PickLastMatch: CInternalResultHandler {
			_BLOCK2(PickLastMatch, CInternalResultHandler,
			        CMatchManager*, self,
					CMatchResultHandler*, handler);
			void Done(const CCloudResult *result) {
				if (result->GetErrorCode() == enNoErr) {
					// In case many matches were returned, use the last one
					const char *lastMatchId = NULL;
					FOR_EACH (const CHJSON* item, *result->GetJSON()->GetSafe("matches")) {
						lastMatchId = item->GetString("_id");
					}
					CHJSON config;
					config.Put("id", lastMatchId);
					REST()->FetchMatch(&config, new ResumeMatch(self, handler));
				} else {
					InvokeHandler(handler, enNoMatchToResume, "No pending matches");
				}
			}
		};
		config.Put("domain", (domain && domain[0]) ? domain : "private");
		config.Put("participating", true);
		config.Put("full", true);
		REST()->ListMatches(&config, new PickLastMatch(this, handler));
	}
}

void CMatchManager::HLDestroyMatch(CMatch *match) {
	Release(match);
}

//////////////////////////// CMatch public ////////////////////////////
CMatch::CMatch(CMatchManager *matchManager, const CCloudResult *result) : expectedManager(matchManager), eventListeners(*(new chain<CMatchEventListener>)) {
	playerData <<= new CHJSON;
	matchData <<= new CHJSON;
	UpdateFromMatchData(result->GetJSON());
}

CMatch::~CMatch() {
	REST()->UnregisterEventListener(GetDomain(), this);
	delete &eventListeners;
}

const char *CMatch::GetDomain() {
	return matchData->GetString("domain");
}

const char* CMatch::GetGamerId() {
	return CUserManager::Instance()->GetGamerID();
}

const char* CMatch::GetLastEventId() {
	return lastEventId;
}

const char* CMatch::GetMatchId() {
	return matchData->GetString("_id");
}

const CHJSON * CMatch::GetPlayers() {
	return playerData;
}

CMatch::State CMatch::GetStatus() {
	const char *status = matchData->GetString("status", "finished");
	if (IsEqual(status, "running")) {
		return RUNNING;
	}
	return FINISHED;
}

bool CMatch::IsCreator() {
	return IsEqual(matchData->GetSafe("creator")->GetString("gamer_id"), CUserManager::Instance()->GetGamerID());
}

void CMatch::RegisterEventListener(CMatchEventListener *eventListener) {
	if (eventListeners.isEmpty()) {
		REST()->RegisterEventListener(GetDomain(), this);
	}
	eventListeners.Add(eventListener);
}

void CMatch::UnregisterEventListener(CMatchEventListener *eventListener) {
	eventListeners.Remove(eventListener);
	if (eventListeners.isEmpty()) {
		REST()->UnregisterEventListener(GetDomain(), this);
	}
}

void CMatch::PostMove(CMatchResultHandler *handler, const CHJSON *moveData, const CHJSON *optionalUpdatedGameState, const CHJSON *optionalAdditionalData) {
	if (!CheckManager()) { return InvokeHandler(handler, enObjectDestroyed); }

	struct PostedMove: CInternalResultHandler {
		_BLOCK2(PostedMove, CInternalResultHandler,
			CMatch*, self,
			CMatchResultHandler*, handler);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() == enNoErr) {
				// Keep the updated match
				self->UpdateFromMatchData(result->GetJSON());
			}
			return InvokeHandler(handler, result, self);
		}
	};

	CHJSON config;
	config.Put("id", GetMatchId());
	config.Put("lastEventId", GetLastEventId());
	config.Put("move", moveData);
	config.Put("globalState", optionalUpdatedGameState);
	if (optionalAdditionalData) { config.Put("osn", optionalAdditionalData->Get("osn")); }
	REST()->PostMove(&config, new PostedMove(this, handler));
}

void CMatch::DrawFromShoe(CMatchResultHandler *handler, int count, const CHJSON *optionalAdditionalData) {
	if (!CheckManager()) { return InvokeHandler(handler, enObjectDestroyed); }

	struct DrawnObject: CInternalResultHandler {
		_BLOCK2(DrawnObject, CInternalResultHandler,
			CMatch*, self,
			CMatchResultHandler*, handler);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() == enNoErr) {
				// Keep the updated match
				self->UpdateFromMatchData(result->GetJSON());
			}
			return InvokeHandler(handler, result, self);
		}
	};

	CHJSON config;
	config.Put("id", GetMatchId());
	config.Put("lastEventId", GetLastEventId());
	config.Put("count", count);
	if (optionalAdditionalData) { config.Put("osn", optionalAdditionalData->Get("osn")); }
	REST()->DrawFromShoeInMatch(&config, new DrawnObject(this, handler));
}

void CMatch::Leave(CMatchResultHandler *handler, const CHJSON *optionalAdditionalData) {
	if (!CheckManager()) { return InvokeHandler(handler, enObjectDestroyed); }

	struct Left: CInternalResultHandler {
		_BLOCK2(Left, CInternalResultHandler,
				CMatch*, self,
				CMatchResultHandler*, handler);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() == enNoErr) {
				// Keep the updated match
				self->UpdateFromMatchData(result->GetJSON());
			}
			return InvokeHandler(handler, result, self);
		}
	};
	CHJSON config;
	config.Put("id", GetMatchId());
	if (optionalAdditionalData) { config.Put("osn", optionalAdditionalData->Get("osn")); }
	REST()->LeaveMatch(&config, new Left(this, handler));
}

void CMatch::Finish(CMatchResultHandler *handler, bool deleteToo, const CHJSON *optionalAdditionalData) {
	if (!CheckManager()) { return InvokeHandler(handler, enObjectDestroyed); }

	struct Deleted: CInternalResultHandler {
		_BLOCK2(Deleted, CInternalResultHandler,
				CMatch*, self,
				CMatchResultHandler*, handler);
		void Done(const CCloudResult *result) {
			return InvokeHandler(handler, result, self);
		}
	};
	struct Finished: CInternalResultHandler {
		_BLOCK3(Finished, CInternalResultHandler,
				CMatch*, self,
				CMatchResultHandler*, handler,
				bool, deleteToo);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() != enNoErr) { return InvokeHandler(handler, result, self); }
			self->UpdateFromMatchData(result->GetJSON());

			// Now delete if necessary
			if (deleteToo) {
				CHJSON config;
				config.Put("id", self->GetMatchId());
				REST()->DeleteMatch(&config, new Deleted(self, handler));
			}
			else {
				return InvokeHandler(handler, result, self);
			}
		}
	};
	CHJSON config;
	config.Put("id", GetMatchId());
	config.Put("lastEventId", GetLastEventId());
	if (optionalAdditionalData) { config.Put("osn", optionalAdditionalData->Get("osn")); }
	REST()->FinishMatch(&config, new Finished(this, handler, deleteToo));
}

//////////////////////////// CMatch private ////////////////////////////
bool CMatch::CheckManager() {
	return CMatchManager::Instance() == expectedManager;
}

void CMatch::UpdateFromMatchData(const CHJSON *json) {
	// Copy match data to matchData -- only keep parts of the match to save memory
	FOR_EACH (const CHJSON *node, *json->GetSafe("match")) {
		if (IsEqual(node->name(), "players")) {
			playerData <<= node->Duplicate();
		} else if (!IsEqual(node->name(), "globalState") && !IsEqual(node->name(), "events")) {
			matchData->Put(node->name(), node->Duplicate());
		}
	}
	lastEventId = matchData->GetString("lastEventId");
}

//////////////////////////// CEventListener interface ////////////////////////////
void CMatch::onEventReceived(const char *domain, const CCloudResult *event) {
	const char *type = event->GetJSON()->GetString("type");
	// Match kind of event?
	if (!strncmp(type, "match.", 6)) {
		// Is this event for us?
		const CHJSON *eventNode = event->GetJSON()->GetSafe("event");
		if (IsEqual(eventNode->GetString("match_id"), GetMatchId())) {
			// Handle it for ourselves
			const char *type = event->GetJSON()->GetString("type");
			// Keep for subsequent requests
			lastEventId = eventNode->GetString("_id");
			if (IsEqual(type, "match.join")) {
				// Add joined players to the list
				FOR_EACH (const CHJSON *node, *eventNode->GetSafe("playersJoined")) {
					playerData->Add(node->Duplicate());
				}
			}
			else if (IsEqual(type, "match.leave")) {
				// Remove players who left from the list
				CHJSON *players = CHJSON::Array();
				FOR_EACH(const CHJSON *existing, *playerData) {
					bool wasDeleted = false;
					FOR_EACH (const CHJSON *left, *eventNode->GetSafe("playersLeft")) {
						if (IsEqual(existing->GetString("gamer_id"), left->GetString("gamer_id"))) { wasDeleted = true; }
					}
					if (!wasDeleted) {
						players->Add(existing->Duplicate());
					}
				}
				playerData <<= players;
			}
			else if (IsEqual(type, "match.finish")) {
				matchData->Put("status", "finished");
			}

			// And broadcast it to the listeners
			FOR_EACH (CMatchEventListener *l, eventListeners) {
				l->onMatchEventReceived(event);
			}
		}
	}
}

void CMatch::onEventError(eErrorCode aErrorCode, const char *aDomain, const CCloudResult *result) {
	// Don't care, will be handled well enough by the PopEventLoopThread
}

//////////////////////////// Raw match API ////////////////////////////
void CMatchManager::CreateMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->CreateMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::ListMatches(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->ListMatches(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::FetchMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->FetchMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::JoinMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->JoinMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::InvitePlayer(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->InvitePlayerToMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::DismissInvitation(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->DismissInvitationToMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::PostMove(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->PostMove(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::DrawFromShoe(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->DrawFromShoeInMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::LeaveMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->LeaveMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::FinishMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->FinishMatch(configuration, MakeBridgeDelegate(handler));
}

void CMatchManager::DeleteMatch(const CHJSON *configuration, CResultHandler *handler) {
    return REST()->DeleteMatch(configuration, MakeBridgeDelegate(handler));
}

//////////////////////////// Helpers ////////////////////////////
void XtraLife::InvokeHandler(CMatchResultHandler *handler, const CCloudResult *result, CMatch *match) {
	if (handler) {
		handler->Invoke(result->GetErrorCode(), result, match);
		delete handler;
	}
}

void XtraLife::InvokeHandler(CMatchResultHandler *handler, eErrorCode code, const char *message) {
	if (handler) {
		CCloudResult result(code, message);
		handler->Invoke(code, &result, NULL);
		delete handler;
	}
}
