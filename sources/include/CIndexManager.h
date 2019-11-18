//
//  CIndexManager.h
//  XtraLife
//
//  Created by Florian Bronnimann on 11/06/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CIndexManager_h
#define XtraLife_CIndexManager_h

#include "XtraLife.h"
#include "CDelegate.h"
/*! \file CIndexManager.h
 */

namespace XtraLife
{
    namespace Helpers {
        class CHJSON;
    }

    /** The CIndexManager class is used to index and search for objects (e.g. matches, players, ?).

		The index is global to a domain (or your game if private, as usual), therefore all data is
		shared by all players.
		
		You can index arbitrary data and then look for it using either the properties or the
		payload. The properties are indexed, while the payload is only returned in the results.

		For instance, if you would like to look for matches using certain criteria, such as only
		"public" matches involving "team B", you will not store these properties in the matches
		themselves since they are not indexed, but instead index the matches under a "matches"
		index, along with these properties, and then perform a query to find them. Note that you
		may need server-side hooks in order to keep the index updated as the match gets modified
		(if you are indexing, for instance, the number of players, you will want to override the
		match-join and match-leave events). Refer to the server documentation for more info.
	*/
	class FACTORY_CLS CIndexManager {
	public:

		/**
		 * Use this method to obtain a reference to the CIndexManager.
		 * @return the one and only instance of this manager
		 */
		static CIndexManager *Instance();
		
		/**
		 * Deletes an indexed entry. If you just want to update an entry, simply use IndexObject.
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration JSON data describing the indexed data to remove
		 - domain (optional string): the domain on which the action is to be taken (if not passed,
		   the private domain is used).
		 - index (string): the name of the index for scoping.
		 - objectid (string): the object ID (as passed when indexing) to remove.
		 * @result if noErr, the json passed to the handler may contain the following:
		 {
			done: 1
		 }
		 */
		void DeleteObject(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);
		
		/**
		 * Fetches a previously indexed object.
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration JSON data describing the indexed data to retrieve
		 - domain (optional string): the domain on which the action is to be taken (if not passed,
		   the private domain is used).
		 - index (string): the name of the index for scoping.
		 - objectid (string): the object ID (as passed when indexing) to fetch.
		 * @result if noErr, the json passed to the handler may contain the following:
		 {
			_index: 'cloud.xtralife.test.m3nsd85gnqd3',
			_type: 'matchIndex',
			_id: '55706319d11b8125d58c8abe',
			_version: 1,
			found: true,
			_source:
			{
				"rank": "captain",
				"age": 28,
				"world": "utopia",
				"payload": {
					"name": "Captain America",
					"lastPlayed": 1433428652427
				}
			}
		}
		 */
		void FetchObject(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Indexes a new object.
		 *
		 * Use this API to add or update an object in an index. You can have as many indexes as you need: one
		 * for gamer properties, one for matches, one for finished matches, etc. It only depends on what you
		 * want to search for.
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration JSON data describing the data to be indexed
		 - domain (optional string): the domain on which the action is to be taken (if not passed, the private
		   domain is used).
		 - index (string): the name of the index for scoping.
		 - objectid (string): the ID of the object to be indexed. It can be anything; this ID only needs to
		   uniquely identify your document. Therefore, using the match ID to index a match is recommended for
		   instance.
		 - properties (object): a freeform object, whose attributes will be indexed and searchable. These
		   properties are typed! So if 'age' is once passed as an int, it must always be an int, or an error
		   will be thrown upon insertion.
		 - payload (optional object): another freeform object. These properties are attached to the document
		   in the same way as the properties, however those are not indexed (cannot be looked for in a search
		   request). Its content is returned in searches (payload property).
		 * @result if noErr, the json passed to the handler may contain the following:
		 {
			done: 1
		 }
		 */
		void IndexObject(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

		/**
		 * Searches the index.
		 * 
		 * You can search documents in the index with this API. It allows you to make complex queries.
		 * See the Elastic documentation to learn the full syntax. It?s easy and quite powerful.
		 * http://www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl-query-string-query.html
		 *
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration JSON data describing the data to be indexed
		 - domain (optional string): the domain on which the action is to be taken (if not passed, the private
		   domain is used).
		 - index (string): the name of the index for scoping.
		 - query (string): query query string. Example: "item:silver". See Elastic documentation.
		 - sort (optional array): name of properties (fields) to sort the results with.
		   Example: `["item:asc"]`.
		 - search: JSON describing what is to search. Can be passed instead of query and sort parameters, and
		   allows to use the full DSL capabilities of ElasticSearch. See
		   https://www.elastic.co/guide/en/elasticsearch/reference/current/query-dsl.html for more information.
		 - limit (optional number): the maximum number of results to return. For pagination, defaulting to 30.
		 - skip (optional number): the first element to start from. Use skip=30&limit=30 to fetch the second
		   page.
		 * @result if noErr, the json passed to the handler may contain the following:
			{ total: 1,
			  max_score: 1,
			  hits:
			   [ { _index: 'cloud.xtralife.test.m3nsd85gnqd3',
				   _type: 'matchIndex',
				   _id: '55706319d11b8125d58c8abe',
				   _score: 1,
				   _source: {
						"rank": "captain",
						"age": 28,
						"world": "utopia",
						"payload": {
							"name": "Captain America",
							"lastPlayed": 1433428652427
						}
				   }
				 }
			   ]
			 }
	    */
		void Search(const Helpers::CHJSON *aConfiguration, CResultHandler *aHandler);

	private:

		/**
		 * Not publicly instantiable/subclassable. Use Instance().
		 */
		CIndexManager();
		~CIndexManager();
		/**
		 * You need to call this in CClan::Terminate().
		 */
		void Terminate();

		friend class CClan;
		friend struct singleton_holder<CIndexManager>;
	};
	
}


#endif
