package cloud.xtralife.sdk.iab;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.vending.billing.IInAppBillingService;
import cloud.xtralife.sdk.Clan;
import cloud.xtralife.sdk.EErrorCode;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Performs low level tasks related to Android In-App Billing.
 */
public class IabHelper {
	// Interfaces for listeners
	public static interface SetupListener {
		void onDone(IabHelper handler, IabResult result);
	}
	public static interface CloudResultListener {
		// If code is 0, at least the result variable has a valid value. Else only message has a valid value.
		void onDone(EErrorCode code, JSONObject result, String message);
	}

	private static final String TAG = "IabHelper";
	// Billing response codes
	public static final int BILLING_RESPONSE_RESULT_OK = 0;
	public static final int BILLING_RESPONSE_RESULT_USER_CANCELED = 1;
	public static final int BILLING_RESPONSE_RESULT_BILLING_UNAVAILABLE = 3;
	public static final int BILLING_RESPONSE_RESULT_ITEM_UNAVAILABLE = 4;
	public static final int BILLING_RESPONSE_RESULT_DEVELOPER_ERROR = 5;
	public static final int BILLING_RESPONSE_RESULT_ERROR = 6;
	public static final int BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED = 7;
	public static final int BILLING_RESPONSE_RESULT_ITEM_NOT_OWNED = 8;
	// Thrown by us
	public static final int BILLING_RESPONSE_LOGIC_ERROR = -199;

	// IAB Helper error codes
	public static final int IABHELPER_ERROR_BASE = -1000;
	public static final int IABHELPER_REMOTE_EXCEPTION = -1001;
	public static final int IABHELPER_BAD_RESPONSE = -1002;
	public static final int IABHELPER_VERIFICATION_FAILED = -1003;
	public static final int IABHELPER_SEND_INTENT_FAILED = -1004;
	public static final int IABHELPER_USER_CANCELLED = -1005;
	public static final int IABHELPER_UNKNOWN_PURCHASE_RESPONSE = -1006;
	public static final int IABHELPER_MISSING_TOKEN = -1007;
	public static final int IABHELPER_UNKNOWN_ERROR = -1008;
	public static final int IABHELPER_SUBSCRIPTIONS_NOT_AVAILABLE = -1009;
	public static final int IABHELPER_INVALID_CONSUMPTION = -1010;

	// Keys for the responses from InAppBillingService
	public static final String RESPONSE_CODE = "RESPONSE_CODE";
	public static final String RESPONSE_GET_SKU_DETAILS_LIST = "DETAILS_LIST";
	public static final String RESPONSE_BUY_INTENT = "BUY_INTENT";
	public static final String RESPONSE_INAPP_PURCHASE_DATA = "INAPP_PURCHASE_DATA";
	public static final String RESPONSE_INAPP_SIGNATURE = "INAPP_DATA_SIGNATURE";
	public static final String RESPONSE_INAPP_ITEM_LIST = "INAPP_PURCHASE_ITEM_LIST";
	public static final String RESPONSE_INAPP_PURCHASE_DATA_LIST = "INAPP_PURCHASE_DATA_LIST";
	public static final String RESPONSE_INAPP_SIGNATURE_LIST = "INAPP_DATA_SIGNATURE_LIST";
	public static final String INAPP_CONTINUATION_TOKEN = "INAPP_CONTINUATION_TOKEN";

	// Item types
	public static final String ITEM_TYPE_INAPP = "inapp";
	public static final String ITEM_TYPE_SUBS = "subs";

	// some fields on the getSkuDetails response bundle
	public static final String GET_SKU_DETAILS_ITEM_LIST = "ITEM_ID_LIST";
	public static final String GET_SKU_DETAILS_ITEM_TYPE_LIST = "ITEM_TYPE_LIST";

	// Other variables
	private static class SingletonHolder {
		private static final IabHelper INSTANCE = new IabHelper();
	}

	private IInAppBillingService mService;
	private ServiceConnection mServiceConn;
	private boolean mDisposed, mSetupDone;
	private int mRequestCode;
	private String mPurchasingItemType;
	private CloudResultListener mPurchaseListener;

	/**
	 * Call this to retrieve an instance of the IabHelper and ensure that you are on the UI thread
	 * of a given activity. Will do the setup if not already done.
	 * @param activity activity that will be used to invoke your listener on the main thread.
	 * @param listener Listener called when the operation has finished (if async).
	 */
	public static void getHandler(Activity activity, final SetupListener listener) {
		final IabHelper result = SingletonHolder.INSTANCE;
		activity.runOnUiThread(new Runnable() {
			public void run() {
				result.setupInappBillingIfNecessary(listener);
			}
		});
	}

	/**
	 * Lists details about products available on the Store.
	 * @param skus List of SKUs (product ID) to query for.
	 * @param listener Called upon termination, whether successful or not. The JSON object returned
	 *                 if the code is enNoErr is ready to be sent back as a response to listProducts.
	 */
	public void getProductDetails(final ArrayList<String> skus, final CloudResultListener listener) {
		if (!mSetupDone || mDisposed) {
			listener.onDone(EErrorCode.enLogicError, null, "Setup not done properly.");
			return;
		}
		// No product, would fail if queried like that to Google
		if (skus.size() == 0) {
			postEmptyProductList(listener);
			return;
		}

		final Handler handler = new Handler();
		(new Thread(new Runnable() {
			public void run() {
			Context context = getApplicationContext();
			Bundle querySkus = new Bundle();
			querySkus.putStringArrayList(GET_SKU_DETAILS_ITEM_LIST, skus);
			if (Clan.sVerboseLog) {
				Log.v(TAG, "Querying for products SKUs on Google Play:");
				for (String s: skus) Log.v(TAG, "    - " + s);
			}
			try {
				Bundle skuDetails = mService.getSkuDetails(3, context.getPackageName(), ITEM_TYPE_INAPP, querySkus);
				if (Clan.sVerboseLog) Log.v(TAG, "Got answer from Google Play: " + skuDetails.toString());

				if (skuDetails.containsKey(RESPONSE_GET_SKU_DETAILS_LIST)) {
					ArrayList<String> responseList = skuDetails.getStringArrayList(
						RESPONSE_GET_SKU_DETAILS_LIST);

					try {
						// Received products, decode and fill them in the response
						JSONObject result = new JSONObject();
						JSONArray products = new JSONArray();
						for (String thisResponse : responseList) {
							SkuDetails d = new SkuDetails(ITEM_TYPE_INAPP, thisResponse);
							JSONObject p = new JSONObject();
							// We could add much more info (see SkuDetails), but let's limit to that for now
							p.put("internalProductId", d.getSku());
							p.put("price", d.getPriceAmount());
							p.put("currency", d.getPriceCurrency());
							products.put(p);
						}
						result.put("products", products);
						postResult(handler, listener, EErrorCode.enNoErr, result, null);

					} catch (JSONException e) {
						e.printStackTrace();
						postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Invalid response from Google Play: " + e.getMessage());
					}
				} else {
					int response = IabUtil.getResponseCodeFromBundle(skuDetails);
					if (response != BILLING_RESPONSE_RESULT_OK) {
						postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "getSkuDetails() failed: " + IabUtil.getResponseDesc(response));
					} else {
						postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "getSkuDetails() returned a bundle with neither an error nor a detail list.");
					}
				}
			} catch (RemoteException ex) {
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "getSkuDetails(): exception on remote call.");
			}
			}
		})).start();
	}

	/**
	 * Launches the purchase flow.
	 * @param activity Parent activity.
	 * @param requestCode Request code that will be triggered to the activity when the purchase flow
	 *                    finishes or cancels.
	 * @param sku SKU of the item to be purchased.
	 * @param developerPayload Custom data to be provided to Google servers.
	 * @param listener Listener getting called asynchronously when the purchase ends. In case the
	 *                 listener is called with enNoErr, the message parameter contains the
	 *                 continuation token, which is to be passed back to terminatePurchase.
	 */
	public void launchPurchase(final Activity activity, final int requestCode, final String sku, final String developerPayload, final CloudResultListener listener) {
		if (!mSetupDone || mDisposed || mPurchaseListener != null) {
			listener.onDone(EErrorCode.enLogicError, null, "Setup not done properly or purchase already in process.");
			return;
		}

		final Handler handler = new Handler();
		(new Thread(new Runnable() {
		public void run() {
			Context context = getApplicationContext();
			String itemType = ITEM_TYPE_INAPP;
			try {
				// Query already purchased items
				List<Purchase> purchases = new ArrayList<Purchase>();
				int response = queryPurchases(purchases);
				if (response != BILLING_RESPONSE_RESULT_OK) {
					Log.e(TAG, "Query purchases failed, Error response: " + IabUtil.getResponseDesc(response));
					postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Query purchases failed");
					return;
				}

				// Find if the one we're trying to purchase is already purchased
				for (Purchase p: purchases) {
					if (p.getSku().equals(sku)) {
						postResult(handler, listener, EErrorCode.enNoErr, getPurchaseResultJson(p), p.getToken());
						return;
					}
				}

				// Launch the actual purchase
				Log.v(TAG, "Constructing buy intent for " + sku + ", item type: " + itemType);
				Bundle buyIntentBundle = mService.getBuyIntent(3, context.getPackageName(), sku, itemType, developerPayload);
				response = IabUtil.getResponseCodeFromBundle(buyIntentBundle);
				if (response != BILLING_RESPONSE_RESULT_OK) {
					Log.e(TAG, "Unable to buy item, Error response: " + IabUtil.getResponseDesc(response));
					postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Unable to buy item");
					return;
				}

				PendingIntent pendingIntent = buyIntentBundle.getParcelable(RESPONSE_BUY_INTENT);
				Log.v(TAG, "Launching buy intent for " + sku + ". Request code: " + requestCode);
				mRequestCode = requestCode;
				mPurchaseListener = listener;
				mPurchasingItemType = itemType;
				activity.startIntentSenderForResult(pendingIntent.getIntentSender(), requestCode, new Intent(), 0, 0, 0);
			} catch (IntentSender.SendIntentException e) {
				Log.e(TAG, "SendIntentException while launching purchase flow for sku " + sku);
				e.printStackTrace();
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "launchPurchase(): failed to send buy intent.");
			} catch (RemoteException e) {
				Log.e(TAG, "RemoteException while launching purchase flow for sku " + sku);
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "launchPurchase(): exception on remote call.");
			} catch (JSONException e) {
				Log.e(TAG, "JSONException while launching purchase flow for sku " + sku);
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "launchPurchase(): exception decoding JSON.");
			}
		}
		})).start();
	}

	/**
	 * Entry point from the outside for the #handleActivityResult method.
	 */
	public static boolean sHandleActivityResult(int requestCode, int resultCode, Intent data) {
		return SingletonHolder.INSTANCE.handleActivityResult(requestCode, resultCode, data);
	}

	/**
	 * Consumes a purchase so it won't be reported again as a pending purchase.
	 * @param sku The SKU of the item.
	 * @param token The consumption token. Passed to the listener from launchPurchase as the message
	 *              parameter.
	 * @param listener Listener called in case of success or failure.
	 */
	public void terminatePurchase(final String sku, final String token, final CloudResultListener listener) {
		final Handler handler = new Handler();
		final String itemType = ITEM_TYPE_INAPP;
		(new Thread(new Runnable() {
			public void run() {
			if (!itemType.equals(ITEM_TYPE_INAPP)) {
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Items of type '" + itemType + "' can't be consumed.");
				return;
			}

			try {
				Log.v(TAG, "Consuming sku: " + sku + ", token: " + token);
				int response = mService.consumePurchase(3, getApplicationContext().getPackageName(), token);
				if (response == BILLING_RESPONSE_RESULT_OK) {
					Log.v(TAG, "Successfully consumed sku: " + sku);
					postResult(handler, listener, EErrorCode.enNoErr, new JSONObject(), null);
				}
				else {
					Log.v(TAG, "Error consuming consuming sku " + sku + ". " + IabUtil.getResponseDesc(response));
					postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Error consuming sku " + sku);
				}
			}
			catch (RemoteException e) {
				postResult(handler, listener, EErrorCode.enErrorWithExternalStore, null, "Remote exception while consuming " + sku);
			}
			}
		})).start();
	}

	/**
	 * @return Retrieves the application context.
	 */
	private Context getApplicationContext() {
		return Clan.GetContext().getApplicationContext();
	}

	private JSONObject getPurchaseResultJson(Purchase purchase) throws JSONException {
		// Post the data to the listener
		JSONObject result = new JSONObject();
		result.put("store", "googleplay");
		result.put("internalProductId", purchase.getSku());
		result.put("receipt", purchase.getOriginalJson());
		result.put("signature", purchase.getSignature());
		return result;
	}

	private boolean handleActivityResult(int requestCode, int resultCode, Intent data) {
		if (!mSetupDone || mDisposed || requestCode != mRequestCode || mPurchaseListener == null) { return false; }

		if (data == null) {
			mPurchaseListener.onDone(EErrorCode.enErrorWithExternalStore, null, "Null data in IAB result");
			mPurchaseListener = null;
			return true;
		}

		int responseCode = IabUtil.getResponseCodeFromIntent(data);
		String purchaseData = data.getStringExtra(RESPONSE_INAPP_PURCHASE_DATA);
		String dataSignature = data.getStringExtra(RESPONSE_INAPP_SIGNATURE);

		if (resultCode == Activity.RESULT_OK && responseCode == BILLING_RESPONSE_RESULT_OK) {
			Log.v(TAG, "Successful result code from purchase activity.");
			Log.v(TAG, "Purchase data: " + purchaseData);
			Log.v(TAG, "Data signature: " + dataSignature);
			Log.v(TAG, "Extras: " + data.getExtras());

			if (purchaseData == null || dataSignature == null) {
				Log.e(TAG, "BUG: either purchaseData or dataSignature is null.");
				Log.v(TAG, "Extras: " + data.getExtras().toString());
				mPurchaseListener.onDone(EErrorCode.enErrorWithExternalStore, null, "IAB returned null purchaseData or dataSignature");
				mPurchaseListener = null;
				return true;
			}

			try {
				Purchase purchase = new Purchase(mPurchasingItemType, purchaseData, dataSignature);
				// Post the data to the listener
				mPurchaseListener.onDone(EErrorCode.enNoErr, getPurchaseResultJson(purchase), purchase.getToken());
				mPurchaseListener = null;
				return  true;
			}
			catch (JSONException e) {
				Log.e(TAG, "Failed to parse purchase data.");
				e.printStackTrace();
				mPurchaseListener.onDone(EErrorCode.enErrorWithExternalStore, null, "Failed to parse purchase data");
				mPurchaseListener = null;
				return true;
			}
		}
		else if (resultCode == Activity.RESULT_OK) {
			// result code was OK, but in-app billing response was not OK.
			Log.v(TAG, "Result code was OK but in-app billing response was not OK: " + IabUtil.getResponseDesc(responseCode));
			mPurchaseListener.onDone(EErrorCode.enErrorWithExternalStore, null, "Problem purchasing item");
			mPurchaseListener = null;
		}
		else if (resultCode == Activity.RESULT_CANCELED) {
			Log.v(TAG, "Purchase canceled - Response: " + IabUtil.getResponseDesc(responseCode));
			mPurchaseListener.onDone(EErrorCode.enUserCanceledPurchase, null, null);
			mPurchaseListener = null;
		}
		else {
			Log.e(TAG, "Purchase failed. Result code: " + Integer.toString(resultCode) + ". Response: " + IabUtil.getResponseDesc(responseCode));
			mPurchaseListener.onDone(EErrorCode.enErrorWithExternalStore, null, "Unknown purchase response");
			mPurchaseListener = null;
		}
		return true;
	}

	/**
	 * Post the result on the main thread. Use that instead of calling the listener directly for all
	 * asynchronous tasks.
	 * @param handler A handler that you need to create before starting the async task.
	 * @param listener The listener to call.
	 * @param code The error code.
	 * @param result The JSON result if any.
	 * @param message The error message, used if the error code is not 0.
	 */
	private static void postResult(Handler handler, final CloudResultListener listener, final EErrorCode code, final JSONObject result, final String message) {
		// Post result on main thread
		handler.post(new Runnable() {
			public void run() {
				listener.onDone(code, result, message);
			}
		});
	}

	/**
	 * Sends an empty product list.
	 * @param listener listener to receive the event
	 */
	private static void postEmptyProductList(CloudResultListener listener) {
		try {
			JSONObject result = new JSONObject();
			JSONArray products = new JSONArray();
			result.put("products", products);
			listener.onDone(EErrorCode.enNoErr, result, null);
		} catch (JSONException e) {
			e.printStackTrace();
			listener.onDone(EErrorCode.enInternalError, null, "Error encoding JSON: " + e.getMessage());
		}
	}

	/**
	 * Queries for purchases already made by the customer.
	 * @param purchases List of purchases, filled with actual purchases found.
	 * @return The error code (BILLING_RESPONSE_RESULT_OK if everything went ok)
	 */
	private int queryPurchases(List<Purchase> purchases) throws JSONException, RemoteException {
		String itemType = ITEM_TYPE_INAPP;
		Context context = getApplicationContext();
		// Query purchases
		boolean verificationFailed = false;
		String continueToken = null;

		Log.v(TAG, "Querying owned items, item type: " + itemType);
		do {
			Log.v(TAG, "Calling getPurchases with continuation token: " + continueToken);
			Bundle ownedItems = mService.getPurchases(3, context.getPackageName(), itemType, continueToken);
			int response = IabUtil.getResponseCodeFromBundle(ownedItems);
			Log.d(TAG, "Owned items response: " + String.valueOf(response));
			if (response != BILLING_RESPONSE_RESULT_OK) {
				Log.v(TAG, "getPurchases() failed: " + IabUtil.getResponseDesc(response));
				return response;
			}
			if (!ownedItems.containsKey(RESPONSE_INAPP_ITEM_LIST)
				|| !ownedItems.containsKey(RESPONSE_INAPP_PURCHASE_DATA_LIST)
				|| !ownedItems.containsKey(RESPONSE_INAPP_SIGNATURE_LIST)) {
				Log.e(TAG, "Bundle returned from getPurchases() doesn't contain required fields.");
				return IABHELPER_BAD_RESPONSE;
			}

			ArrayList<String> ownedSkus = ownedItems.getStringArrayList(
				RESPONSE_INAPP_ITEM_LIST);
			ArrayList<String> purchaseDataList = ownedItems.getStringArrayList(
				RESPONSE_INAPP_PURCHASE_DATA_LIST);
			ArrayList<String> signatureList = ownedItems.getStringArrayList(
				RESPONSE_INAPP_SIGNATURE_LIST);

			for (int i = 0; i < purchaseDataList.size(); ++i) {
				String purchaseData = purchaseDataList.get(i);
				String signature = signatureList.get(i);
				String sku = ownedSkus.get(i);
				Log.v(TAG, "Sku is owned: " + sku);
				Purchase purchase = new Purchase(itemType, purchaseData, signature);
				if (TextUtils.isEmpty(purchase.getToken())) {
					Log.w(TAG, "BUG: empty/null token!");
					Log.v(TAG, "Purchase data: " + purchaseData);
				}
				// Record ownership and token
				purchases.add(purchase);
			}

			continueToken = ownedItems.getString(INAPP_CONTINUATION_TOKEN);
			Log.v(TAG, "Continuation token: " + continueToken);
		} while (!TextUtils.isEmpty(continueToken));

		return verificationFailed ? IABHELPER_VERIFICATION_FAILED : BILLING_RESPONSE_RESULT_OK;
	}

	/**
	 * Sets up the billing as necessary.
	 */
	private void setupInappBillingIfNecessary(final SetupListener listener) {
		// Already done
		if (mSetupDone) {
			listener.onDone(this, new IabResult(BILLING_RESPONSE_RESULT_OK, "Already loaded."));
			return;
		}

		final Context context = getApplicationContext();
		Log.v(TAG, "Starting in-app billing.");
		mServiceConn = new ServiceConnection() {
			@Override
			public void onServiceDisconnected(ComponentName name) {
				Log.v(TAG, "Billing service disconnected.");
				mService = null;
			}

			@Override
			public void onServiceConnected(ComponentName name, IBinder service) {
				if (mDisposed) return;
				Log.v(TAG, "Billing service connected.");
				mService = IInAppBillingService.Stub.asInterface(service);
				String packageName = context.getPackageName();
				try {
					Log.v(TAG, "Checking for in-app billing 3 support.");

					// check for in-app billing v3 support
					int response = mService.isBillingSupported(3, packageName, ITEM_TYPE_INAPP);
					if (response != BILLING_RESPONSE_RESULT_OK) {
						listener.onDone(null, new IabResult(response, "Error checking for billing v3 support."));
						return;
					}

					Log.v(TAG, "In-app billing version 3 supported for " + packageName);
					mSetupDone = true;
				}
				catch (RemoteException e) {
					listener.onDone(null, new IabResult(IABHELPER_REMOTE_EXCEPTION, "RemoteException while setting up in-app billing."));
					e.printStackTrace();
					return;
				}

				listener.onDone(IabHelper.this, new IabResult(BILLING_RESPONSE_RESULT_OK, "Setup successful."));
			}
		};

		Intent serviceIntent = new Intent("com.android.vending.billing.InAppBillingService.BIND");
		serviceIntent.setPackage("com.android.vending");
		if (!context.getPackageManager().queryIntentServices(serviceIntent, 0).isEmpty()) {
			// service available to handle that Intent
			context.bindService(serviceIntent, mServiceConn, Context.BIND_AUTO_CREATE);
		}
		else {
			// no service available to handle that Intent
			listener.onDone(this, new IabResult(BILLING_RESPONSE_RESULT_BILLING_UNAVAILABLE, "Billing service unavailable on device."));
		}
	}
}
