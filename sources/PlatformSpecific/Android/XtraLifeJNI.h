#ifndef __XTRALIFEJNI__
#define __XTRALIFEJNI__

#include <jni.h>
#include <android/log.h>

extern JNIEnv *JNIGetEnv();

void RegisterDevice(void);
bool JNIIsConnected(void);

#endif	// __XTRALIFEJNI__