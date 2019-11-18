#include "XtraLifeJNI.h"
#include "include/CDelegate.h"

namespace XtraLife {
	namespace Helpers {
		class CHJSON;
	}

	/**
	 * Customized in-app store glue for iOS.
	 */
	struct CGooglePlayStoreHandler: CStoreInterface {
		CGooglePlayStoreHandler();
		virtual ~CGooglePlayStoreHandler();

		/**
		 * Fetches information about products.
		 * @param productIds The product ID list is nil-terminated.
		 * @param onFinished called with info about the list of products
		 */
		virtual void GetInformationAboutProducts(const XtraLife::Helpers::CHJSON *productIds, CInternalResultHandler *onFinished);
		virtual void LaunchPurchase(const XtraLife::Helpers::CHJSON *productInfo, CInternalResultHandler *onFinished);

	private:
		/**
		 * Fetches the Java class corresponding to the GooglePlayStoreHandler.
		 * @param outEnv JNI environment, fetched for you.
		 * @param outClass The GooglePlayStoreHandler class, can be useful to make call afterwards.
		 * @param outInstance The instance, ready to call methods on.
		 * @param onError Called in case of error (return = false) with an appropriate error code.
		 * @return Whether the whole process was successful. You should end your method here in case
		 * it returns false.
		 */
		bool GetJavaGooglePlayHandler(JNIEnv* &outEnv, jclass &outClass, jobject &outInstance, CInternalResultHandler *onError);
		/**
		 * Calls a Java Method that accepts a (String json, long internalResultHandlerId).
		 * @param env JNI environment, as previously retrieved by GetJavaGooglePlayHandler for example
		 * @param clazz The class containing the method definition
		 * @param instance The instance of the object to perform the call on
		 * @param methodName Name of the method to call
		 * @param params Parameters to the method (JSON)
		 * @param handler Handler to be called from the Java code upon completion
		 */
		void CallStandardJavaMethod(JNIEnv *env, jclass clazz, jobject instance, const char *methodName,
			const XtraLife::Helpers::CHJSON *params, CInternalResultHandler *handler);
	};
}
