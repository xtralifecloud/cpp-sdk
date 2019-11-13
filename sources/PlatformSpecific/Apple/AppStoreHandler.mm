//
//  CAppStoreHandler.m
//  XtraLife
//
//  Created by roland on 19/01/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#import <CommonCrypto/CommonCrypto.h>
#include "XtraLife_private.h"
#include "CCallback.h"
#include "CHJSON.h"
#include "CClannishRESTProxy.h"
#include "Misc/helpers.h"
#include "CUserManager.h"
#include "AppStoreHandler.h"

#if defined(__IOS__)
#	define STORE_TYPE "appstore"
#	define STORE_PRODUCT_ID "appStoreId"
#elif defined(__MACOS__)
#	define STORE_TYPE "macstore"
#	define STORE_PRODUCT_ID "macStoreId"
#else
#	error Unsupported platform
#endif

using namespace XtraLife;
using namespace XtraLife::Helpers;

//////////////////////////// Singleton ////////////////////////////
singleton_holder<CAppStoreHandler> singleton;
CStoreInterface *CStoreInterface::Instance() {
	return singleton.Instance();
}
CAppStoreHandler::CAppStoreHandler() {
	objCHandler = [[CAppStoreHandlerObjC alloc] initWithGlue:this];
}
CAppStoreHandler::~CAppStoreHandler() {
	[objCHandler release];
}

//////////////////////////// Actual functionality ////////////////////////////
void CAppStoreHandler::GetInformationAboutProducts(const CHJSON *productIds, CInternalResultHandler *onFinished) {
	[objCHandler getInformationAboutProducts:productIds whenComplete:onFinished];
}


void CAppStoreHandler::LaunchPurchase(const CHJSON *productInfo, CInternalResultHandler *onFinished) {
	NSString *product = [NSString stringWithUTF8String:productInfo->GetString(STORE_PRODUCT_ID)];
	[objCHandler launchPurchase:product whenComplete:onFinished];
}

//////////////////////////// Obj-C code ////////////////////////////
@implementation CAppStoreHandlerObjC

- (id)initWithGlue:(XtraLife::CAppStoreHandler *)_glue {
	// Register for transaction events
	if ((self = [super init])) {
		glue = _glue;
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
	}
	return self;
}

- (void)dealloc {
	[[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
	[super dealloc];
}

- (void)getInformationAboutProducts:(const CHJSON *)productIds whenComplete:(CInternalResultHandler*)complete {
	// Ask for appstore product IDs
	NSMutableSet *productIdentifiers = [NSMutableSet set];
	for (const CHJSON *node: *productIds) {
		// Do not show those which are not configured for AppStore
		const char *pid = node->GetString(STORE_PRODUCT_ID);
		if (pid) { [productIdentifiers addObject:[NSString stringWithUTF8String:pid]]; }
	}
	SKProductsRequest *productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
	productsRequest.delegate = self;

	struct ReceivedResponse: CDelegate<void (SKProductsResponse*, NSError*)> {
		_BLOCK3(ReceivedResponse, CDelegate,
				CAppStoreHandlerObjC*, me,
				CInternalResultHandler*, complete,
				CHJSON*, mProductInfo);
		void Done(SKProductsResponse *response, NSError *error) {
			owned_ref<CHJSON> productInfo (mProductInfo); // To ensure release when exited
			if (error) {
				return [me invokeOnMain:complete error:enErrorWithExternalStore desc:error.description.UTF8String];
			}
			
			CHJSON *result = new CHJSON;
			// Report invalid products in the console
			if (response.invalidProductIdentifiers.count > 0) {
				char result[1024], temp[1024];
				int index = 0;
				safe::strcpy(result, "");
				for (NSString *productId in response.invalidProductIdentifiers) {
					safe::sprintf(temp, "%s%s", index++ > 0 ? ", " : "", productId.UTF8String);
					safe::strcat(result, temp);
				}
				CONSOLE_WARNING("Queried invalid product identifiers: [%s]\n", result);
			}
			
			CHJSON *products = CHJSON::Array();
			for (SKProduct *product in response.products) {
				CHJSON node;
				NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
				[formatter setLocale:product.priceLocale];
								
				// We could add much more info, but let's limit to that for now
				node.Put("productId", [me cotcProductForAppStoreId:product.productIdentifier inProductInfo:productInfo]->GetString("productId"));
				node.Put("internalProductId", product.productIdentifier.UTF8String);
				node.Put("price", product.price.doubleValue);
				node.Put("currency", formatter.currencyCode.UTF8String);
				products->Add(node.Duplicate());
			}
			result->Put("products", products);

			CCloudResult callResult (enNoErr, result);
			return [me invokeOnMain:complete result:&callResult];
		}
	};

	[self setCallback:new ReceivedResponse(self, complete, productIds->Duplicate()) forNextRequest:productsRequest];
	[productsRequest start];
}

- (void)launchPurchase:(NSString *)productId whenComplete:(XtraLife::CInternalResultHandler *)complete {
	// We need to keep the handler as the transaction thing is completely asynchronous.
	// There is only one entry per product ID, and the CGloballyKeptHandler will make sure that each handler for each product ID receives a response once.
	if (purchaseHandlers[productId.UTF8String].IsSet()) {
		// TODO Florian -- document the fact that a purchase can only be started once
		return InvokeHandler(complete, enCanceled, "A purchase for this product is already in progress");
	}
	purchaseHandlers[productId.UTF8String].Set(complete);

	// First we need to query the SKProduct
	SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
										  initWithProductIdentifiers:[NSSet setWithObject:productId]];
	productsRequest.delegate = self;
	
	struct ReceivedResponse: CDelegate<void (SKProductsResponse*, NSError*)> {
		_BLOCK3(ReceivedResponse, CDelegate,
				CAppStoreHandlerObjC*, me,
				cstring, productId,
				CInternalResultHandler*, complete);
		void Done(SKProductsResponse *response, NSError *error) {
			if (error) {
				return [me invokeOnMain:complete error:enErrorWithExternalStore desc:error.description.UTF8String];
			}
			if (response.products.count < 1) {
				return [me invokeOnMain:complete error:enProductNotFound desc:"App Store does not list this product. Check your configuration."];
			}
			
			// The product exists, just launch a payment for it
			NSString *username = [NSString stringWithUTF8String:CUserManager::Instance()->GetGamerID()];
			SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:response.products.firstObject];
			payment.quantity = 1;
			payment.applicationUsername = [me hashedValueForAccountName:username];

			// Pending transactions?
			if (SKPaymentQueue.defaultQueue.transactions.count > 0) {
				bool currentPaymentAlreadyPending = false;
				CONSOLE_VERBOSE("There are %d pending transactions, will perform them.\n", (int) SKPaymentQueue.defaultQueue.transactions.count);
				for (SKPaymentTransaction *tx in SKPaymentQueue.defaultQueue.transactions) {
					[me processTransaction:tx];
					if (IsEqual(tx.payment.productIdentifier.UTF8String, productId)) {
						CONSOLE_VERBOSE("Product %s already purchased, discarding additional purchase attempt\n.", productId.c_str());
						currentPaymentAlreadyPending = true;
					}
				}
				if (currentPaymentAlreadyPending) { return; }
			}
			[[SKPaymentQueue defaultQueue] addPayment:payment];
		}
	};
	[self setCallback:new ReceivedResponse(self, productId.UTF8String, complete) forNextRequest:productsRequest];
	[productsRequest start];
}

// SKRequestDelegate protocol
- (void)requestDidFinish:(SKRequest *)request {
	[self invokeHandlerForRequest:request withResponse:nil andError:nil];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
	// We do not actually care about the error
	[self invokeHandlerForRequest:request withResponse:nil andError:error];
}

// SKProductsRequest delegate
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
	[self invokeHandlerForRequest:request withResponse:response andError:nil];
}

// SKPaymentTransactionObserver protocol
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for (SKPaymentTransaction *transaction in transactions) {
		[self processTransaction:transaction];
	}
}

// Private
- (const CHJSON*)cotcProductForAppStoreId:(NSString *)appStoreProductId inProductInfo:(const CHJSON *)productInfo {
	// Finds the product with a given appstore ID in the list
	const char *p = appStoreProductId.UTF8String;
	FOR_EACH (const CHJSON *node, *productInfo) {
		if (IsEqual(node->GetString(STORE_PRODUCT_ID), p) ||
			IsEqual(node->GetString("internalProductId"), p)) {
			return node;
		}
	}
	return CHJSON::Empty();
}

- (NSString *)hashedValueForAccountName:(NSString*)userAccountName {
	const int HASH_SIZE = 32;
	unsigned char hashedChars[HASH_SIZE];
	const char *accountName = [userAccountName UTF8String];
	size_t accountNameLen = strlen(accountName);
 
	// Confirm that the length of the user name is small enough
	// to be recast when calling the hash function.
	if (accountNameLen > UINT32_MAX) {
		CONSOLE_ERROR("Account name too long to hash: %s", userAccountName.UTF8String);
		return nil;
	}
	CC_SHA256(accountName, (CC_LONG)accountNameLen, hashedChars);
 
	// Convert the array of bytes into a string showing its hex representation.
	NSMutableString *userAccountHash = [[NSMutableString alloc] init];
	for (int i = 0; i < HASH_SIZE; i++) {
		// Add a dash every four bytes, for readability.
		if (i != 0 && i%4 == 0) {
			[userAccountHash appendString:@"-"];
		}
		[userAccountHash appendFormat:@"%02x", hashedChars[i]];
	}
 	return userAccountHash;
}

- (void)invokeHandlerForRequest:(SKRequest *)request withResponse:(SKProductsResponse *)response andError:(NSError *)error {
	// Unexpected request
	if (productsResponseHandlers.find(request) == productsResponseHandlers.end()) { return; }
	// Move ownership to a temp object as we'll remove it from the list from this thread
	auto handler = productsResponseHandlers[request].detachOwnership();
	productsResponseHandlers.erase(request);

	[response retain];
	dispatch_async(dispatch_get_main_queue(), ^{
		handler->Invoke(response, error);
		[response release];
		delete handler;
	});
}

- (void)invokeOnMain:(CInternalResultHandler *)handler error:(eErrorCode)code desc:(const char *)description {
	dispatch_async(dispatch_get_main_queue(), ^{
		InvokeHandler(handler, code, description);
	});
}

- (void)invokeOnMain:(CInternalResultHandler *)handler result:(const XtraLife::CCloudResult *)result {
	CCloudResult *copy = result->Duplicate();
	dispatch_async(dispatch_get_main_queue(), ^{
		InvokeHandler(handler, copy);
		delete copy;
	});
}

- (void)invokePurchaseHandler:(const char *)productId result:(const XtraLife::CCloudResult *)result {
	CCloudResult *copy = result->Duplicate();
	cstring* pid = new cstring(productId);
	dispatch_async(dispatch_get_main_queue(), ^{
		purchaseHandlers[pid->c_str()].Invoke(copy);
		delete copy;
		delete pid;
	});
}
						
- (void)notifyServerOfFailedTransaction:(SKPaymentTransaction *)transaction {
	const char *productId = transaction.payment.productIdentifier.UTF8String;
	char message[1024];
	safe::sprintf(message, "Purchase refused, failed AppStore transaction (%s)", transaction.error.description.UTF8String);
	CCloudResult result(enUserCanceledPurchase, message);
	
	[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
	[self invokePurchaseHandler:productId result:&result];
}

- (void)notifyServerOfTransaction:(SKPaymentTransaction *)transaction {
	// Transaction successful, send it to our servers
	cstring productId = transaction.payment.productIdentifier.UTF8String;
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	if ([[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		// We can send the receipt directly
		NSData *receiptData = [NSData dataWithContentsOfURL:receiptURL];
		[self postReceiptToServer:receiptData forProductId:productId transaction:transaction];
	}
	else {
		CONSOLE_VERBOSE("AppStore receipt not found, refreshing\n");
		SKReceiptRefreshRequest *refreshReceiptRequest = [[SKReceiptRefreshRequest alloc] initWithReceiptProperties:@{}];
		struct ReceivedReceipt: CDelegate<void (SKProductsResponse*, NSError*)> {
			_BLOCK3(ReceivedReceipt, CDelegate,
					CAppStoreHandlerObjC*, me,
					SKPaymentTransaction*, transaction,
					cstring, productId);
			void Done(SKProductsResponse *response, NSError *error) {
				[transaction autorelease];
				
				// After refresh we can try again
				NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
				if (![[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
					CCloudResult result(enErrorWithExternalStore, "Unable to fetch product license, the user is not logged in to the AppStore");
					CONSOLE_WARNING("Failed to fetch AppStore receipt\n");
					return [me invokePurchaseHandler:productId result:&result];
				}

				NSData *receiptData = [NSData dataWithContentsOfURL:receiptURL];
				CONSOLE_VERBOSE("AppStore receipt refreshed successfully\n");
				[me postReceiptToServer:receiptData forProductId:productId transaction:transaction];
			}
		};
		[self setCallback:new ReceivedReceipt(self, [transaction retain], productId) forNextRequest:refreshReceiptRequest];
		refreshReceiptRequest.delegate = self;
		[refreshReceiptRequest start];
	}
}

- (void)postReceiptToServer:(NSData *)receiptData forProductId:(const cstring&)productId transaction:(SKPaymentTransaction *)tx {
	/*
	 * We need to do that since this method might be called asynchronously at startup (to restore
	 * transactions). So we need to:
	 * 1) fetch the list of products from CotC in order to have the mapping from this AppStore
	 *    product to the CotC product.
	 * 2) fetch the list of products from AppStore to get price/etc.
	 * 3) consolidate all and send the result to CotC along with the receipt.
	 */
	// Executed when CotC has responded (last)
	struct SentToServer: CInternalResultHandler {
		_BLOCK3(SentToServer, CInternalResultHandler,
				CAppStoreHandlerObjC*, me,
				SKPaymentTransaction*, finishedTx,
				cstring, productId);
		void Done(const CCloudResult *result) {
			// Transaction successful or considered invalid, we need to acknowledge it
			if (result->GetErrorCode() == enNoErr || result->GetHttpStatusCode() == 442) {
				CONSOLE_VERBOSE("Finalizing transaction %s\n", finishedTx.transactionIdentifier.UTF8String);
				[[SKPaymentQueue defaultQueue] finishTransaction:finishedTx];
			}
			[me invokePurchaseHandler:productId result:result];
			[finishedTx release];
		}
	};

	// Executed when we got product info from iOS (second)
	struct GotProductInfo: CInternalResultHandler {
		_BLOCK4(GotProductInfo, CInternalResultHandler,
				CAppStoreHandlerObjC*, me,
				SKPaymentTransaction*, finishedTx,
				cstring, productId,
				NSData*, receiptData);
		void Done(const CCloudResult *result) {
			CInternalResultHandler *whenDone = new SentToServer(me, finishedTx, productId);
			if (result->GetErrorCode() == enNoErr) {
				// Use the product list to get more information
				const CHJSON *products = result->GetJSON()->GetSafe("products");
				const CHJSON *prod = [me cotcProductForAppStoreId:[NSString stringWithUTF8String:productId] inProductInfo:products];
				if (prod != CHJSON::Empty()) {
					// Then send the receipt to the server for validation
					CHJSON config;
					config.Put("store", STORE_TYPE);
					config.Put("productId", prod->GetString("productId"));
					config.Put("internalProductId", prod->GetString("internalProductId"));
					config.Put("price", prod->GetDouble("price"));
					config.Put("currency", prod->GetString("currency"));
					config.Put("receipt", [[receiptData base64EncodedStringWithOptions:0] UTF8String]);
					CClannishRESTProxy::Instance()->SendAppStorePurchaseToServer(&config, whenDone);
				}
				else {
					return [me invokeOnMain:whenDone error:enProductNotFound desc:"Product ID doesn't match an existing product configured in this store on the backoffice."];
				}
			}
			else {
				[me invokeOnMain:whenDone result:result];
			}
			[receiptData release];
		}
	};
	
	// Executed when we have the list of products from CotC (first)
	struct GotCotcProductInfo: CInternalResultHandler {
		_BLOCK4(GotCotcProductInfo, CInternalResultHandler,
				CAppStoreHandlerObjC*, me,
				SKPaymentTransaction*, finishedTx,
				cstring, productId,
				NSData*, receiptData);
		void Done(const CCloudResult *result) {
			CInternalResultHandler *whenDone = new GotProductInfo(me, finishedTx, productId, receiptData);
			if (result->GetErrorCode() == enNoErr) {
				[me getInformationAboutProducts:result->GetJSON()->GetSafe("products")
								   whenComplete:whenDone];
			}
			else {
				InvokeHandler(whenDone, result);
			}
		}
	};
	
	// We need to enrich the receipt with product information
	CClannishRESTProxy::Instance()->GetProductList(CHJSON::Empty(), new GotCotcProductInfo(self, [tx retain], productId, [receiptData retain]));
}

- (void)processTransaction:(SKPaymentTransaction *)transaction {
	switch (transaction.transactionState) {
			// Call the appropriate custom method for the transaction state.
		case SKPaymentTransactionStatePurchasing:
			break;
		case SKPaymentTransactionStateDeferred:
			break;
		case SKPaymentTransactionStateFailed:
			[self notifyServerOfFailedTransaction:transaction];
			break;
		case SKPaymentTransactionStatePurchased:
			[self notifyServerOfTransaction:transaction];
			break;
		case SKPaymentTransactionStateRestored:
			// TODO Florian - test that
			[self notifyServerOfTransaction:transaction.originalTransaction];
			break;
		default:
			// For debugging
			CONSOLE_VERBOSE("Unexpected AppStore transaction state %s", @(transaction.transactionState).description.UTF8String);
			break;
	}
}

- (void)setCallback:(XtraLife::CDelegate<void (SKProductsResponse *, NSError *error)> *)callback forNextRequest:(id)request {
	productsResponseHandlers[request] <<= callback;
}

@end

