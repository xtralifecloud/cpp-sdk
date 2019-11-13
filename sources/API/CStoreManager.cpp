#include "CStoreManager.h"
#include "CCallback.h"
#include "CStoreInterface.h"
#include "CClannishRESTProxy.h"
#include "XtraLife_private.h"
#include "Misc/helpers.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

static singleton_holder<CStoreManager> managerSingleton;

//////////////////////////// Common manager stuff ////////////////////////////
CStoreManager::CStoreManager() : currentlyProcessesProduct(false) {}
CStoreManager::~CStoreManager() {}
CStoreManager* CStoreManager::Instance() { return managerSingleton.Instance(); }
void CStoreManager::Terminate() { managerSingleton.Release(); }

//////////////////////////// Product & purchase ////////////////////////////
void CStoreManager::FetchProductInformation(CResultHandler *onComplete, const CHJSON *configuration) {
	if (currentlyProcessesProduct) { return InvokeHandler(onComplete, enOperationAlreadyInProgress, "The Store is busy with another operation"); }
	currentlyProcessesProduct = true;
	CInternalResultHandler *handler = BuildExclusiveHandler(onComplete);
	
	// First ask the list of products configured on the server
	struct ListOfProductsReceived: CInternalResultHandler {
		_BLOCK2(ListOfProductsReceived, CInternalResultHandler,
				CStoreManager*, self,
				CInternalResultHandler*, handler);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() == enNoErr) {
				CStoreInterface *glue = CStoreInterface::Instance();
				glue->GetInformationAboutProducts(result->GetJSON()->GetSafe("products"), handler);
			} else {
				CONSOLE_VERBOSE("Error in FetchProductInformation: %s\n", result->GetJSONString().c_str());
				InvokeHandler(handler, result);
			}
		}
	};
	
	CClannishRESTProxy::Instance()->GetProductList(configuration, new ListOfProductsReceived(this, handler));
}

void CStoreManager::GetPurchaseHistory(CResultHandler *aHandler, const CHJSON *aConfiguration) {
	CClannishRESTProxy::Instance()->GetPurchaseHistory(aConfiguration, MakeBridgeDelegate(aHandler));
}

void CStoreManager::LaunchPurchase(CResultHandler *onComplete, const CHJSON *configuration) {
	if (currentlyProcessesProduct) { return InvokeHandler(onComplete, enOperationAlreadyInProgress, "The Store is busy with another operation"); }
	currentlyProcessesProduct = true;
	CInternalResultHandler *handler = BuildExclusiveHandler(onComplete);

	const char *productId = configuration->GetString("productId");
	if (!productId) { return InvokeHandler(handler, enBadParameters, "Missing productId"); }
	
	// First ensure that the product is purchaseable
	struct ListOfProductsReceived: CInternalResultHandler {
		_BLOCK3(ListOfProductsReceived, CInternalResultHandler,
				CStoreManager*, self,
				cstring, productId,
				CInternalResultHandler*, handler);
		void Done(const CCloudResult *result) {
			if (result->GetErrorCode() != enNoErr) {
				return InvokeHandler(handler, result);
			}

			// Check that it is in the list (we do not want to allow buying a product that is configured in the App Store but not on our servers).
			const CHJSON *products = result->GetJSON()->GetSafe("products");
			const CHJSON *foundNode = NULL;
			FOR_EACH (const CHJSON *node, *products) {
				if (IsEqual(node->GetString("productId"), productId)) {
					foundNode = node;
					break;
				}
			}
			if (!foundNode) {
				return InvokeHandler(handler, enProductNotFound, "Product ID doesn't match an existing product configured in this store on the backoffice.");
			}
			
			// Okay so the product exists, launch the purchase in a device-specific way
			CStoreInterface *glue = CStoreInterface::Instance();
			glue->LaunchPurchase(foundNode, handler);
		}
	};
	
	CClannishRESTProxy::Instance()->GetProductList(configuration, new ListOfProductsReceived(this, productId, handler));
}

// Private
CInternalResultHandler *CStoreManager::BuildExclusiveHandler(CResultHandler *handler) {
	struct MyHandler: CInternalResultHandler {
		_BLOCK2(MyHandler, CInternalResultHandler,
				CStoreManager*, self,
				CResultHandler*, handler);
		void Done(const CCloudResult *result) {
			self->currentlyProcessesProduct = false;
			InvokeHandler(handler, result);
		}
	};
	return new MyHandler(this, handler);
}
