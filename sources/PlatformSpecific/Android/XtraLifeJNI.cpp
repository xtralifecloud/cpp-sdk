#include "XtraLifeJNI.h"
#include "include/CClan.h"
#include "include/CUserManager.h"
#include "include/CHJSON.h"
#include "Core/CCallback.h"
#include "Core/XtraLife_private.h"
#include "JNIUtilities.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

JavaVM *sJavaVM = NULL;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	sJavaVM	= vm;
	__android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Launching Clan C++ v%s", SDKVERSION);
	
	return JNI_VERSION_1_6;
}

JNIEnv *JNIGetEnv() {
	JNIEnv *env = NULL;
	if (sJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		__android_log_print(ANDROID_LOG_ERROR, "Clan C++", "Couldn't even retrieve JNIEnv");
	}

	return env;
}

#pragma mark - Java native functions implementation

extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_ClanInvokeHandler(JNIEnv* aEnv, jclass aClass, jlong handlerId, jstring jsonStr) {
	CInternalResultHandler *handler = (CInternalResultHandler*) handlerId;
	// Make a CCloudResult out of the JSON string
	JavaString jsonCstr(aEnv, jsonStr);
	CCloudResult result (CHJSON::parse(jsonCstr));
	InvokeHandler(handler, &result);
	return 0;
}


namespace XtraLife
{
    void AchieveRegisterDevice(unsigned long len, const void *bytes)
    {
        CUserManager::Instance()->RegisterDevice("android", (const char*) bytes);
    }
}

void RegisterDevice(void)
{
    JNIEnv*		currentJNIEnv;
    jclass		clanClass;
    jmethodID	queryRegisterDevice;
    jthrowable	exc;
    
    __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Calling Java QueryRegisterDevice");
    
    __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Using JavaVM: %p", sJavaVM);
    
    if (sJavaVM->GetEnv(reinterpret_cast<void**>(&currentJNIEnv), JNI_VERSION_1_6) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Clan C++", "Couldn't even retrieve JNIEnv");
        return;
    }
    
    clanClass	= currentJNIEnv->FindClass("cloud/xtralife/sdk/Clan");
    exc = currentJNIEnv->ExceptionOccurred();
    if(exc)
    {
        __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "No notification module available");
        currentJNIEnv->ExceptionClear();
        
        return;
    }
    
    __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "clanClass is:%p", clanClass);
    if(clanClass)
    {
        queryRegisterDevice	= currentJNIEnv->GetStaticMethodID(clanClass, "QueryRegisterDevice", "()V");
        __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "QueryRegisterDevice is:%p", queryRegisterDevice);
        
        currentJNIEnv->CallStaticVoidMethod(clanClass, queryRegisterDevice);
    }
}

void UnregisterDevice(void)
{
    JNIEnv*		currentJNIEnv;
    jclass		clanClass;
    jmethodID	unregisterDevice;
    jthrowable	exc;
    
    __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Calling Java UnregisterDevice");
    
    if (sJavaVM->GetEnv(reinterpret_cast<void**>(&currentJNIEnv), JNI_VERSION_1_6) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Clan C++", "Couldn't even retrieve JNIEnv");
        return;
    }
    
    clanClass	= currentJNIEnv->FindClass("cloud/xtralife/sdk/Clan");
    exc = currentJNIEnv->ExceptionOccurred();
    if(exc)
    {
        __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "No notification module available");
        currentJNIEnv->ExceptionClear();
        
        return;
    }
    
    __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "clanClass is:%p", clanClass);
    if(clanClass)
    {
        unregisterDevice	= currentJNIEnv->GetStaticMethodID(clanClass, "UnregisterDevice", "()V");
        __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "UnregisterDevice is:%p", unregisterDevice);
        
        currentJNIEnv->CallStaticVoidMethod(clanClass, unregisterDevice);
    }
}

extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_Clan_RegisterDevice(JNIEnv* aEnv, jclass aClass, jstring aToken)
{
	JavaString	token(aEnv, aToken);

	AchieveRegisterDevice(strlen(token), token.c_str());
	__android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Registered device:android with token:%s", token.c_str());

	return enNoErr;
}

extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_Clan_HandleRemoteNotification(JNIEnv* aEnv, jclass aClass, jobject aMessage) {
	return enNotImplemented;
}

extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_Clan_Suspended(JNIEnv* aEnv, jclass aClass)
{
	__android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Suspended");

	if (CClan::Instance())
		CClan::Instance()->Suspend();

	return 0;
}

extern "C" JNIEXPORT jint JNICALL Java_cloud_xtralife_sdk_Clan_Resumed(JNIEnv* aEnv, jclass aClass)
{
	__android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Resumed");
	
	if (CClan::Instance())
		CClan::Instance()->Resume();

	return 0;
}

namespace XtraLife {
    namespace Helpers {
        CHJSON *collectDeviceInformation() {

            JNIEnv*		currentJNIEnv;
            jclass		clanClass;
            jmethodID	collect;
            
            __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "Calling Java CollectDeviceInformation");
            
            if (sJavaVM->GetEnv(reinterpret_cast<void**>(&currentJNIEnv), JNI_VERSION_1_6) != JNI_OK)
            {
                __android_log_print(ANDROID_LOG_ERROR, "Clan C++", "Couldn't even retrieve JNIEnv");
            }
            
            clanClass	= currentJNIEnv->FindClass("cloud/xtralife/sdk/Clan");
            __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "clanClass is:%p", clanClass);
            collect = currentJNIEnv->GetStaticMethodID(clanClass, "CollectDeviceInformation", "()Ljava/lang/String;");
            __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "CollectDeviceInformation is:%p", collect);
            jstring info = (jstring) currentJNIEnv->CallStaticObjectMethod(clanClass, collect);

            CHJSON *j = CHJSON::parse(JavaString(currentJNIEnv, info));
            __android_log_print(ANDROID_LOG_VERBOSE, "Clan C++", "CollectDeviceInformation: %s", j->printFormatted().c_str());

            return j;
        }
    }
}
