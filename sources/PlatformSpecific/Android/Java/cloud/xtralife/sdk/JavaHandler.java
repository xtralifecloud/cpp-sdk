package cloud.xtralife.sdk;

import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

/**
 * Allows to call a Java callback from C++.
 */
public abstract class JavaHandler {
	abstract void onInvoked(EErrorCode code, JSONObject result);

	public static int makeHandler(JavaHandler handler) {
		for (int i = 0; i < handlers.size(); i++) {
			if (handlers.get(i) == null) {
				handlers.set(i, handler);
				return i;
			}
		}
		handlers.add(handler);
		return handlers.size() - 1;
	}

	public static void invokeJavaHandler(int handler, int errorCode, String jsonResult) {
		if (handler >= handlers.size() || handlers.get(handler) == null) { return; }
		try {
			handlers.get(handler).onInvoked(EErrorCode.fromCode(errorCode), new JSONObject(jsonResult));
		} catch (JSONException e) {
			e.printStackTrace();
			handlers.get(handler).onInvoked(EErrorCode.enLogicError, new JSONObject());
		}
		handlers.set(handler, null);
	}

	private static ArrayList<JavaHandler> handlers = new ArrayList<JavaHandler>();
}
