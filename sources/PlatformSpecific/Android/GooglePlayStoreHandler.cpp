#include "XtraLife.h"
#include "helpers.h"
#include "CStoreInterface.h"
#include "GooglePlayStoreHandler.h"
#include "XtraLife_private.h"
#include "CCallback.h"
#include "JNIUtilities.h"
#include "CClannishRESTProxy.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

//////////////////////////// Singleton & lifecycle ////////////////////////////
singleton_holder<CGooglePlayStoreHandler> singleton;
CStoreInterface *CStoreInterface::Instance() {
	return singleton.Instance();
}

CGooglePlayStoreHandler::CGooglePlayStoreHandler() {}
CGooglePlayStoreHandler::~CGooglePlayStoreHandler() {}

//////////////////////////// Google Play Store Handler implementation ////////////////////////////
void CGooglePlayStoreHandler::GetInformationAboutProducts(const CHJSON *productIds, CInternalResultHandler *onFinished) {
	JNIEnv *env;
	jclass clazz;
	jobject instance;

	if (GetJavaGooglePlayHandler(env, clazz, instance, onFinished)) {
		CallStandardJavaMethod(env, clazz, instance, "listProducts", productIds, onFinished);
	}
}

void CGooglePlayStoreHandler::LaunchPurchase(const CHJSON *productInfo, CInternalResultHandler *onFinished) {
	JNIEnv *env;
	jclass clazz;
	jobject instance;
	if (GetJavaGooglePlayHandler(env, clazz, instance, onFinished)) {
		CallStandardJavaMethod(env, clazz, instance, "purchaseProduct", productInfo, onFinished);
	}
}

//////////////////////////// Private ////////////////////////////
bool CGooglePlayStoreHandler::GetJavaGooglePlayHandler(JNIEnv* &env, jclass &clazz, jobject &instance, CInternalResultHandler *onError) {
	// GooglePlayStoreHandler.getInstance()
	env = JNIGetEnv();
	if (!env) { InvokeHandler(onError, enInternalError, "JNI: No environment set up"); return false; }
	clazz = env->FindClass("cloud/xtralife/sdk/GooglePlayStoreHandler");
	if (!clazz) { InvokeHandler(onError, enInternalError, "JNI: GooglePlayStoreHandler class not found"); return false; }
	jmethodID method = env->GetStaticMethodID(clazz, "getInstance", "()Lcloud/xtralife/sdk/GooglePlayStoreHandler;");
	if (!method) { InvokeHandler(onError, enInternalError, "JNI: GooglePlayStoreHandler.getInstance() not found"); return false; }
	instance = env->CallStaticObjectMethod(clazz, method);
	if (!instance) { InvokeHandler(onError, enInternalError, "JNI: No GooglePlayStoreHandler instance"); return false; }
	return true;
}

void CGooglePlayStoreHandler::CallStandardJavaMethod(JNIEnv *env, jclass clazz, jobject instance, const char *methodName, const CHJSON *params, CInternalResultHandler *handler) {
	jmethodID method = env->GetMethodID(clazz, methodName, "(Ljava/lang/String;J)V");
	if (!method) { return InvokeHandler(handler, enInternalError, csprintf("JNI: Method %s not found", methodName)); }

	cstring paramsStr;
	params->print(paramsStr);
	env->CallVoidMethod(instance, method, env->NewStringUTF(paramsStr), handler);
}

/**
 * Post purchase as called from the Java code.
 */
extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_GooglePlayStoreHandler_postPurchase(JNIEnv* aEnv, jclass aClass, jint javaHandlerId, jstring jsonStr) {
	struct SentPurchaseToServer: CInternalResultHandler {
		_BLOCK1(SentPurchaseToServer, CInternalResultHandler,
				jint, javaHandlerId);

		void Done(const CCloudResult *result) {
			JNIInvokeJavaHandler(javaHandlerId, result->GetErrorCode(), result->GetJSON());
		}
	};

	JavaString jsonCstr(aEnv, jsonStr);
	owned_ref<CHJSON> json (CHJSON::parse(jsonCstr));
	CClannishRESTProxy::Instance()->SendAppStorePurchaseToServer(json, new SentPurchaseToServer(javaHandlerId));
	return 0;
}
