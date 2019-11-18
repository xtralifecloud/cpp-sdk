//
//  CStoreInterface.h
//  XtraLife
//
//  Created by roland on 19/01/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#ifndef __XtraLife__CStoreInterface__
#define __XtraLife__CStoreInterface__

#include "include/CDelegate.h"

namespace XtraLife {
	
    namespace Helpers {
        class CHJSON;
    }

/**
	 * Abstract class.
	 */
	struct CStoreInterface {
		static CStoreInterface *Instance();
		virtual ~CStoreInterface() {}

		/**
		 * Implements the actual listing of products. Note that this is not necessarily called when
		 * the customer is actually querying the list of products.
		 * @param productIds list of products to query information for on the store
		 * @param onFinished callback that should be called on the same thread as this call was
		 *        performed with the following information:
		 * - "products" (array): list of products as queried; may not include all products queried
		 *   for, if they do not actually exist on the store.
		 *    - "productId" (string): product identifier as formatted in the query.
		 *    - "price" (number): the price amount. Number matching the currency. For instance,
		 *      0.69 (€) or 99 (¥).
		 *    - "currency" (string): the currency represented by the price. Should be the ISO code
		 *      for this currency, whenever supplied by the store.
		 */
		virtual void GetInformationAboutProducts(const Helpers::CHJSON *productIds, CInternalResultHandler *onFinished) = 0;

		/**
		 * Implements the actual purchase flow. This does not represent the whole purchase process
		 * though, only the first part where the store front end is presented to the user and allows
		 * him to pay. The purchase process will also incur an additional step consisting in a
		 * verification of the payment before actually delivering the goods. That is why you need to
		 * call CClannishRESTProxy::SendAppStorePurchaseToServer after your purchase has been done
		 * and forward the result.
		 * @param productInfo product to be purchased (contains a node productId).
		 * @param onFinished callback that should be called on the same thread as this call was
		 *        performed with the information returned by CClannishRESTProxy::SendAppStorePurchaseToServer.
		 */
		virtual void LaunchPurchase(const Helpers::CHJSON *productInfo, CInternalResultHandler *onFinished) = 0;
	};
}



#endif
