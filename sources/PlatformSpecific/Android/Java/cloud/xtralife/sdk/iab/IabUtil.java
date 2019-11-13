package cloud.xtralife.sdk.iab;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/**
 * Lightens up the IabHelper class.
 */
public class IabUtil {
	private static final String TAG = "IabUtil";

	// Workaround to bug where sometimes response codes come as Long instead of Integer
	static int getResponseCodeFromBundle(Bundle b) {
		Object o = b.get(IabHelper.RESPONSE_CODE);
		if (o == null) {
			Log.v(TAG, "Bundle with null response code, assuming OK (known issue)");
			return IabHelper.BILLING_RESPONSE_RESULT_OK;
		}
		else if (o instanceof Integer) return ((Integer)o).intValue();
		else if (o instanceof Long) return (int)((Long)o).longValue();
		else {
			Log.e(TAG, "Unexpected type for bundle response code.");
			Log.e(TAG, o.getClass().getName());
			return IabHelper.BILLING_RESPONSE_LOGIC_ERROR;
		}
	}

	// Workaround to bug where sometimes response codes come as Long instead of Integer
	static int getResponseCodeFromIntent(Intent i) {
		Object o = i.getExtras().get(IabHelper.RESPONSE_CODE);
		if (o == null) {
			Log.e(TAG, "Intent with no response code, assuming OK (known issue)");
			return IabHelper.BILLING_RESPONSE_RESULT_OK;
		}
		else if (o instanceof Integer) return ((Integer)o).intValue();
		else if (o instanceof Long) return (int)((Long)o).longValue();
		else {
			Log.e(TAG, "Unexpected type for intent response code.");
			Log.e(TAG, o.getClass().getName());
			throw new RuntimeException("Unexpected type for intent response code: " + o.getClass().getName());
		}
	}

	/**
	 * Returns a human-readable description for the given response code.
	 *
	 * @param code The response code
	 * @return A human-readable string explaining the result code.
	 *     It also includes the result code numerically.
	 */
	static String getResponseDesc(int code) {
		String[] iab_msgs = ("0:OK/1:User Canceled/2:Unknown/" +
			"3:Billing Unavailable/4:Item unavailable/" +
			"5:Developer Error/6:Error/7:Item Already Owned/" +
			"8:Item not owned").split("/");
		String[] iabhelper_msgs = ("0:OK/-1001:Remote exception during initialization/" +
			"-1002:Bad response received/" +
			"-1003:Purchase signature verification failed/" +
			"-1004:Send intent failed/" +
			"-1005:User cancelled/" +
			"-1006:Unknown purchase response/" +
			"-1007:Missing token/" +
			"-1008:Unknown error/" +
			"-1009:Subscriptions not available/" +
			"-1010:Invalid consumption attempt").split("/");

		if (code <= IabHelper.IABHELPER_ERROR_BASE) {
			int index = IabHelper.IABHELPER_ERROR_BASE - code;
			if (index >= 0 && index < iabhelper_msgs.length) return iabhelper_msgs[index];
			else return String.valueOf(code) + ":Unknown IAB Helper Error";
		}
		else if (code < 0 || code >= iab_msgs.length)
			return String.valueOf(code) + ":Unknown";
		else
			return iab_msgs[code];
	}

}
