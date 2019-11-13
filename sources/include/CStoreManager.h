//
//  CStoreManager.h
//  XtraLife
//
//  Created by Florian on 19/01/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CStoreManager_h
#define XtraLife_CStoreManager_h

#include "XtraLife.h"
#include "CDelegate.h"
#include "CClan.h"

/*! \file CStoreManager.h
 */

namespace XtraLife {
    namespace Helpers {
        class CHJSON;
    }

	/**
		The CStoreManager class allows to easily make in-app purchase on supported
	 	platforms. You basically unlock features by running on transaction once the
		CotC servers have successfully verified the financial transaction.
	*/
	struct FACTORY_CLS CStoreManager {
		/**
		 * Use this method to obtain a reference to the CStoreManager.
		 * @return the one and only instance of this manager
		*/
		static CStoreManager *Instance();
				
		/**
		 * Fetches a "catalog" of products available for purchase as configured on the server.
		 * Note that this call returns the catalog as configured on the CotC server, which may not
		 * be exhaustive if additional products are configured on iTunes Connect but not reported
		 * to the CotC servers.
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration unused
		 * @result if noErr, the json passed to the handler may contain:
		 - "products" (array of objects): information about the products, which may contain:
		     - "productId" (string): the ID of product as matching on iTunes Connect. This is the
			   actual product that will be purchased. You set its price and such on Apple servers.
		     - "price" (number): this entry is present if the product has been found on Apple
		       servers. It contains the price in the local currency of the current user.
		     - "currency" (string): if the product has a match on iTunes Connect, contains the local
		       currency of the current user.
		 */
		void FetchProductInformation(CResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);
		
		/**
		 * Fetches the list of transactions made by the logged in user. Only successful transactions
		 * show here.
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration currently unused
		 * @result if noErr, the json passed to the handler may contain:
		 - "transactions" (array of objects): information about the transactions, which may contain:
		     - "store" (string): the type of Store on which the purchase has been made.
			 - "productId" (string): the ID of the product paid on the in-app store.
			 - "dateTime": ISO 8601 time of purchase.
			 - "price" (number): price paid (the price might have been changed since then on iTunes
		       Connect; any such change does not reflect here).
			 - "currency" (string): currency of the price paid.
			 - "storeTransactionId" (string): the ID of transaction on the original store. You might
			   want to use it for customer service. Depends on the store type.
		 */
		void GetPurchaseHistory(CResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);

		/**
		 * Launches a purchase process for a given product. The purchase process is asynchronous,
		 * may take a lot of time and may not necessarily ever finish (i.e. the application may be
		 * killed before it actually finishes). That said, only one purchase process is running at
		 * at a time, and refusal to purchase the good will lead to a notification in the result
		 * handler as far as notified by the underlying store.
		 *
		 * @param aHandler result handler whenever the call finishes (it might also be synchronous)
		 * @param aConfiguration parameters on how and what to purchase. May contain:
		 - "productId" (string): the ID of product, that should either be hardcoded in your
		   application or fetched from a previous call to #FetchProductInformation.
		 * @result if noErr, the json passed to the handler may contain:
		 - "validation" (object): status of purchase
		   - "ok" (number): 1 if the purchase has been done properly (should always be the case).
		   - "repeated" (number): 1 if the purchase has been considered as a duplicate. This means
		     that the order was already processed previously and the reward associated with the
		     purchase was not given. This does not necessarily means that there was a piracy attempt
		     or that the reward was never given; it might simply be that the order was validated on
		     the server and the transaction executed, but the client did not get the answer at the
		     time, thus trying again.
		   - "purchase" (optional object): information about the purchase made. Only supplied in
		     case of successful purchase. If the purchase was already validated before (repeated
			 transaction), this field is only available if the purchase was made by the same user
			 as now. It is omitted in other cases.

			 You might want to use it for customer service. Depends on the store type and might
			 not be supplied in case of a repeated transaction that was made by another user.
		 */
		void LaunchPurchase(CResultHandler *aHandler, const Helpers::CHJSON *aConfiguration);
		
	private:
		bool currentlyProcessesProduct;
		
		CStoreManager();
		~CStoreManager();

		/**
		 * You need to call this in CClan::Terminate().
		 */
		void Terminate();
		
		/**
		 * Always wrap the CResultHandler with this if you want the operation to be mutually
		 * exclusive. Check and set currentlyProcessesProduct on entry, then pass a wrapped
		 * CResultHandler to the next functions.
		 */
		CInternalResultHandler *BuildExclusiveHandler(CResultHandler *aHandler);
		

		friend void CClan::Terminate();
		friend struct singleton_holder<CStoreManager>;
	};
}


#endif
