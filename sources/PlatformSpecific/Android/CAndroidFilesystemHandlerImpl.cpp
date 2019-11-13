#include <jni.h>

#include "CStdioBasedFileImpl.h"
#include "helpers.h"
#include "XtraLife_private.h"
#include "JNIUtilities.h"

using namespace XtraLife;

extern char *CFilesystem_appFolder;
extern JavaVM *sJavaVM;

JNIEnv *getEnv() {
	JNIEnv *env;
	if (sJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		CONSOLE_ERROR("Couldn't retrieve JNIEnv.");
		return NULL;
	}
	return env;
}

static void buildFullPath(char *destPath, size_t bufSize, const char *relativePath) {
	// Fetch default directory
	char directoryOnly[1024];
	JNIEnv *env = getEnv();
	if (!env) { return; }

	jclass clanClass = env->FindClass("cloud/xtralife/sdk/Clan");
	jmethodID method = env->GetStaticMethodID(clanClass, "GetDataDirectory", "()Ljava/lang/String;");
	jstring rv = (jstring) env->CallStaticObjectMethod(clanClass, method, 0);
	JavaString appDataPath (env, rv);

	safe::sprintf(destPath, bufSize, "%s/%s/%s", appDataPath.c_str(), CFilesystem_appFolder, relativePath);

	// Extract the path
	safe::strcpy(directoryOnly, destPath);
	for (char *ptr = directoryOnly + strlen(directoryOnly) - 1; ptr >= directoryOnly; ptr--) {
		if (*ptr == '/') {
			*ptr = '\0';
			break;
		}
	}

	// For further reference, string memory is to be managed by us (directoryOnly in that case)
	jstring path = env->NewStringUTF(directoryOnly);
	method = env->GetStaticMethodID(clanClass, "CreateDirectory", "(Ljava/lang/String;)Z");
	env->CallStaticBooleanMethod(clanClass, method, path);
}

bool CFilesystemHandler::Delete(const char *relativePath) {
	char fullPath[1024];
	JNIEnv *env = getEnv();
	if (!env) { return false; }

	buildFullPath(fullPath, sizeof(fullPath), relativePath);

	// Use JNI to delete the actual folder
	jclass clanClass = env->FindClass("cloud/xtralife/sdk/Clan");
	jstring path = env->NewStringUTF(fullPath);
	jmethodID method = env->GetStaticMethodID(clanClass, "DeleteFile", "(Ljava/lang/String;)Z");
	if (env->CallStaticBooleanMethod(clanClass, method, path) == 0) {
		CONSOLE_ERROR("Could not delete file %s\n", fullPath);
		return false;
	}
	return true;
}

CInputFile *CFilesystemHandler::OpenForReading(const char *relativePath) {
	char fullPath[1024];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return new CInputFileStdio(fullPath);
}

COutputFile *CFilesystemHandler::OpenForWriting(const char *relativePath) {
	char fullPath[1024];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	CONSOLE_VERBOSE("Writing to %s\n", fullPath);
	return new COutputFileStdio(fullPath);
}
