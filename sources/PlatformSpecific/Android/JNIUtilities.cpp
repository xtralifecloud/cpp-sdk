#include <jni.h>
#include <android/log.h>

//#include "CClan.h"
//#include "JNIUtilities.h"
#include "CHJSON.h"
#include "XtraLife.h"
#include "XtraLifeHelpers.h"
#include "XtraLife_private.h"
#include "XtraLifeJNI.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

CHJSON *BuildJsonFromHashMap(JNIEnv* env, jobject jHashMap) {
	CHJSON *result = new CHJSON;
	// Get the EntrySet of the Map.
	jclass mapClass = env->FindClass("java/util/Map");
	jmethodID entrySet = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
	jobject set = env->CallObjectMethod(jHashMap, entrySet);
	// Obtain an iterator
	jclass setClass = env->FindClass("java/util/Set");
	jmethodID iterator = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
	jobject iter = env->CallObjectMethod(set, iterator);
	// Get the method IDs for the Iterator class
	jclass iteratorClass = env->FindClass("java/util/Iterator");
	jmethodID hasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
	jmethodID next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");
	// Same for the iterator entry class
	jclass entryClass = env->FindClass("java/util/Map$Entry");
	jmethodID getKey = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
	jmethodID getValue = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");
	// Iterate over the EntrySet
	while (env->CallBooleanMethod(iter, hasNext)) {
		jobject entry = env->CallObjectMethod(iter, next);
		jstring key = (jstring) env->CallObjectMethod(entry, getKey);
		jstring value = (jstring) env->CallObjectMethod(entry, getValue);
		const char* keyStr = env->GetStringUTFChars(key, NULL);
		if (keyStr) {
			const char* valueStr = env->GetStringUTFChars(value, NULL);
			if (valueStr) {
				// Put in the JSON
				result->Put(keyStr, valueStr);
				env->ReleaseStringUTFChars(value, valueStr);
				env->DeleteLocalRef(value);
			}
			env->ReleaseStringUTFChars(key, keyStr);
			env->DeleteLocalRef(key);
		}
		env->DeleteLocalRef(entry);
	}
	env->DeleteLocalRef(mapClass);
	env->DeleteLocalRef(set);
	env->DeleteLocalRef(setClass);
	env->DeleteLocalRef(iter);
	env->DeleteLocalRef(iteratorClass);
	env->DeleteLocalRef(entryClass);
	return result;
}

void JNIInvokeJavaHandler(int handlerId, eErrorCode errorCode, const CHJSON *json) {
	JNIEnv *env = JNIGetEnv();
	if (env) {
		jclass clazz = env->FindClass("com/clanofthecloud/cloudbuilder/JavaHandler");
		if (clazz) {
			jmethodID method = env->GetStaticMethodID(clazz, "invokeJavaHandler", "(IILjava/lang/String;)V");
			if (method) {
				cstring result;
				json->print(result);
				jstring resultStr = env->NewStringUTF(result);
				env->CallStaticVoidMethod(clazz, method, handlerId, (int) errorCode, resultStr);
				return;
			}
		}
	}
	CONSOLE_ERROR("Fatal issue with JNI!") return;
}
