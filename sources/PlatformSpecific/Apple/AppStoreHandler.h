//
//  C
//  XtraLife
//
//  Created by roland on 19/01/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#import <StoreKit/StoreKit.h>

#include "CStoreInterface.h"
#include <map>

// This file is only included in the equivalent .mm, so we are not really polluting the namespace
using namespace XtraLife;
using namespace XtraLife::Helpers;

namespace XtraLife { struct CAppStoreHandler; }

/**
 * Private Objective-C interface to communicate with the OS.
 */
@interface CAppStoreHandlerObjC: NSObject <SKProductsRequestDelegate, SKRequestDelegate, SKPaymentTransactionObserver> {
@private
	CAppStoreHandler *glue;
	std::map<SKRequest*,
		owned_ref<CDelegate<void (SKProductsResponse*, NSError*)>>> productsResponseHandlers;
	std::map<cstring,
		CGloballyKeptHandler<CInternalResultHandler>> purchaseHandlers;
}

- (id)initWithGlue:(CAppStoreHandler*)glue;
- (void)getInformationAboutProducts:(const CHJSON *)productIds
					   whenComplete:(CInternalResultHandler *)complete;
- (void)launchPurchase:(NSString*)productId
		  whenComplete:(CInternalResultHandler *)complete;

// Private
- (const CHJSON*)cotcProductForAppStoreId:(NSString*)appStoreProductId inProductInfo:(const CHJSON*)productInfo;
- (NSString *)hashedValueForAccountName:(NSString*)userAccountName;
- (void)invokeHandlerForRequest:(SKRequest*)request withResponse:(SKProductsResponse*)response andError:(NSError*)error;
- (void)invokeOnMain:(CInternalResultHandler*)handler error:(eErrorCode)code desc:(const char*)description;
- (void)invokeOnMain:(CInternalResultHandler *)handler result:(const CCloudResult*)result;
- (void)invokePurchaseHandler:(const char*)productId result:(const CCloudResult*)result;
- (void)notifyServerOfFailedTransaction:(SKPaymentTransaction*)transaction;
- (void)notifyServerOfTransaction:(SKPaymentTransaction*)transaction;
- (void)processTransaction:(SKPaymentTransaction*)transaction;
- (void)postReceiptToServer:(NSData*)receiptData forProductId:(const cstring&)productId transaction:(SKPaymentTransaction*)tx;
- (void)setCallback:(CDelegate<void (SKProductsResponse*, NSError*)>*)callback
	 forNextRequest:(id)request;

@end

namespace XtraLife {

	/**
	 * Customized in-app store glue for iOS.
	 */
	struct CAppStoreHandler: CStoreInterface {
		CAppStoreHandler();
		virtual ~CAppStoreHandler();

		/**
		 * Fetches information about products.
		 * @param productIds The product ID list is nil-terminated.
		 * @param onFinished called with info about the list of products
		 */
		virtual void GetInformationAboutProducts(const Helpers::CHJSON *productIds, CInternalResultHandler *onFinished);
		virtual void LaunchPurchase(const CHJSON *productInfo, CInternalResultHandler *onFinished);
				
	private:
		CAppStoreHandlerObjC *objCHandler;
	};
}
