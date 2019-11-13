package cloud.xtralife.sdk;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

import cloud.xtralife.sdk.iab.IabHelper;
import cloud.xtralife.sdk.iab.IabResult;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

/**
 * This class exposes the in-app billing functionality. It needs to be called by the developer to
 * complete the integration. You need to call the InitGooglePlayStore and HandleActivityResult
 * static methods.
 */
public class GooglePlayStoreHandler {
	private static final String TAG = "GooglePlayStoreHandler";
	private static class SingletonHolder {
		private static final GooglePlayStoreHandler INSTANCE = new GooglePlayStoreHandler();
	}
	public static int STORE_REQUEST_CODE = 0xC07C;
	private static Activity activity;

	/**
	 * Call this at startup in order to be able to make in-app payments through Clan.
	 * Does NOT work if any attempt to use the Clan has been made prior to calling this.
	 * @param activity Activity on the behalf of which any purchase flow will be launched. Note that
	 *                 you NEED to forward the onActivityResult to this class!
	 */
	public static void InitGooglePlayStore(Activity activity) {
		GooglePlayStoreHandler.activity = activity;
	}

	/**
	 * Handles an activity result that's part of the purchase flow in in-app billing.
	 * You must call this method from your Activity's {@link android.app.Activity@onActivityResult}
	 * method. This method MUST be called from the UI thread of the Activity.
	 * @param requestCode The requestCode as you received it. A request code matching
	 *                    STORE_REQUEST_CODE indicates a request that needs to be handled by this
	 *                    class. If the STORE_REQUEST_CODE collides with another of your codes, you
	 *                    may overwrite it before you start using the CStoreManager.
	 * @param resultCode The resultCode as you received it.
	 * @param data The data (Intent) as you received it.
	 * @return Returns true if the result was related to a purchase flow and was handled;
	 *     false if the result was not related to a purchase, in which case you should
	 *     handle it normally.
	 */
	public static void HandleActivityResult(int requestCode, int resultCode, Intent data) {
		IabHelper.sHandleActivityResult(requestCode, resultCode, data);
	}

	// These are only used internally, do not call them yourself. Protected is just used as a marker
	// since JNI doesn't enforce access control restrictions.
	/**
	 * Lists the products on sale in the Google Play Store.
	 * @param paramsJson Currently unused.
	 * @param onCompletedHandler Called upon completion.
	 */
	protected void listProducts(String paramsJson, final long onCompletedHandler) {
		if (!checkInitCalled(onCompletedHandler)) { return; }
		try {
			// Convert the JSON array to a standard ArrayList SKU list
			final JSONArray products = new JSONArray(paramsJson);
			final ArrayList<String> skus = new ArrayList<String>();
			for (int i = 0; i < products.length(); i++) {
				JSONObject product = products.getJSONObject(i);
				skus.add(product.getString("googlePlayId"));
			}

			// Everything went well; connect to the IAB service
			IabHelper.getHandler(activity, new IabHelper.SetupListener() {
				@Override
				public void onDone(IabHelper handler, IabResult result) {
				if (handler == null) {
					Clan.InvokeHandler(onCompletedHandler, EErrorCode.enErrorWithExternalStore, result.toString());
					return;
				}
				// Now we can query the products
				handler.getProductDetails(skus, new IabHelper.CloudResultListener() {
					@Override
					public void onDone(EErrorCode code, JSONObject result, String message) {
					if (code != EErrorCode.enNoErr) {
						Clan.InvokeHandler(onCompletedHandler, code, result, message);
						return;
					}
					// We now need to enrich the skus with the product ID
					enrichProductDetails(result, products);
					Clan.InvokeHandler(onCompletedHandler, code, result, message);
					}
				});
				}
			});

		} catch (JSONException e) {
			Log.e("GooglePlayStoreHandler", "Decoding param JSON. Check that you have configured all the products for Google Play on the backoffice.", e);
			Clan.InvokeHandler(onCompletedHandler, EErrorCode.enInternalError, "Decoding param JSON: " + e.getMessage() + ". Check that you have configured all the products for Google Play on the backoffice.");
		}
	}

	/**
	 * Launch the purchase of a product.
	 * @param paramsJson Should contain productId = the product ID to purchase.
	 * @param onCompletedHandler Called upon completion.
	 */
	protected void purchaseProduct(String paramsJson, final long onCompletedHandler) {
		if (!checkInitCalled(onCompletedHandler)) { return; }
		try {
			JSONObject params = new JSONObject(paramsJson);
			final String cotcProductId = params.getString("productId");
			final String gpSku = params.getString("googlePlayId");

			// Everything went well; connect to the IAB service
			IabHelper.getHandler(activity, new IabHelper.SetupListener() {
				@Override
				public void onDone(final IabHelper handler, IabResult result) {
				if (handler == null) {
					Clan.InvokeHandler(onCompletedHandler, EErrorCode.enErrorWithExternalStore, result.toString());
					return;
				}

				// Fetch additional info about the product, we'll include this into the receipt
				ArrayList<String> skus = new ArrayList<String>();
				skus.add(gpSku);
				handler.getProductDetails(skus, new IabHelper.CloudResultListener() {
					@Override
					public void onDone(EErrorCode code, JSONObject productDetails, String message) {
						if (code != EErrorCode.enNoErr) {
							Clan.InvokeHandler(onCompletedHandler, code, productDetails, message);
							return;
						}
						launchPurchase(productDetails, handler, gpSku, onCompletedHandler, cotcProductId);
					}
				});
			}
		});

		} catch (JSONException e) {
			Log.e("GooglePlayStoreHandler", "Decoding param JSON", e);
			Clan.InvokeHandler(onCompletedHandler, EErrorCode.enInternalError, "Decoding param JSON: " + e.getMessage());
		}
	}

	private boolean checkInitCalled(long onErrorHandler) {
		// Not needed anymore
		return true;
	}

	/**
	 * When coming back from google, the product list has only the google SKUs. We want to put back
	 * the names of the products as they appear on the BO.
	 * @param result Result got from getProductDetails.
	 * @param products List of products returned by CotC.
	 */
	private void enrichProductDetails(JSONObject result, JSONArray products) {
		try {
			JSONArray values = result.getJSONArray("products");
			for (int i = 0; i < values.length(); i++) {
				JSONObject enrichedObject = values.getJSONObject(i);
				String googleSku = enrichedObject.getString("internalProductId");
				for (int j = 0; j < products.length(); j++) {
					JSONObject product = products.getJSONObject(j);
					if (googleSku.equals(product.getString("googlePlayId"))) {
						enrichedObject.put("productId", product.getString("productId"));
					}
				}
			}
		} catch (JSONException e) {
			Log.e("GooglePlayStoreHandler", "Enriching products JSON", e);
		}
	}

	private static GooglePlayStoreHandler getInstance() { return SingletonHolder.INSTANCE; }

	// 3rd step of purchaseProduct (methods sorted by alphabetical order + accesbility).
	private void handleEndOfPurchase(Activity activity, final EErrorCode previousCode, final JSONObject previousResult, final long onCompletedHandler, final String gpSku, final String consumptionToken) {
		IabHelper.getHandler(activity, new IabHelper.SetupListener() {
			@Override
			public void onDone(IabHelper handler, IabResult result) {
				if (previousCode != EErrorCode.enNoErr) {
					Clan.InvokeHandler(onCompletedHandler, previousCode, previousResult, null);
					return;
				}

				// Purchase verified -> consume it
				handler.terminatePurchase(gpSku, consumptionToken, new IabHelper.CloudResultListener() {
					@Override
					public void onDone(EErrorCode code, JSONObject result, String message) {
						if (code != EErrorCode.enNoErr) {
							Clan.InvokeHandler(onCompletedHandler, code, result, message);
							return;
						}
						// The result that we return to the user is the one from our servers
						Clan.InvokeHandler(onCompletedHandler, previousCode, previousResult, null);
					}
				});
			}
		});
	}

	// 2nd step of purchaseProduct.
	private void launchPurchase(JSONObject productDetails, final IabHelper handler, final String gpSku, final long onCompletedHandler, final String cotcProductId) {
		try {
			// This object will be used to report additional information with the receipt
			final JSONObject boughtProductInfo = productDetails.getJSONArray("products").getJSONObject(0);

			// Now we can launch the purchase
			handler.launchPurchase(activity, STORE_REQUEST_CODE, gpSku, null, new IabHelper.CloudResultListener() {
				@Override
				public void onDone(EErrorCode code, JSONObject result, String message) {
					if (code != EErrorCode.enNoErr) {
						Clan.InvokeHandler(onCompletedHandler, code, result, message);
						return;
					}

					// Put additional information with the receipt for the server
					try {
						result.put("productId", cotcProductId);
						result.put("price", boughtProductInfo.getDouble("price"));
						result.put("currency", boughtProductInfo.getString("currency"));

						// Verify the purchase data (use our server)
						final String consumptionToken = message;
						postPurchase(JavaHandler.makeHandler(new JavaHandler() {
							@Override
							void onInvoked(final EErrorCode previousCode, final JSONObject previousResult) {
								handleEndOfPurchase(activity, previousCode, previousResult, onCompletedHandler, gpSku, consumptionToken);
							}
						}), result.toString());
					} catch (JSONException e) {
						Log.e("GooglePlayStoreHandler", "Adding info to receipt JSON", e);
						Clan.InvokeHandler(onCompletedHandler, EErrorCode.enInternalError, "Adding info to receipt JSON: " + e.getMessage());
					}
				}
			});
		} catch (JSONException e) {
			Log.e("GooglePlayStoreHandler", "Decoding product JSON", e);
			Clan.InvokeHandler(onCompletedHandler, EErrorCode.enInternalError, "Decoding product JSON: " + e.getMessage());
		}
	}

	/**
	 * Continues the purchase process in native code.
	 * @param handlerId Java handler ID, created through JavaHandler.makeHandler.
	 * @param json Json containing information for the callee. Should contain:
	- "store" (string): "googleplay"
	- "productId" (string): ID of the product being purchased
	- "receipt" (string): Receipt data, as specified by Google.
	 */
	private static native void postPurchase(int handlerId, String json);
}
