#include <shlobj.h>

#include "Core/CStdioBasedFileImpl.h"
#include "Misc/helpers.h"

using namespace XtraLife;

extern char *CFilesystem_appFolder;

static void buildFullPath(char *destPath, size_t bufSize, const char *relativePath) {
	char appDataPath[1024], directoryOnly[1024];
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
	safe::sprintf(destPath, bufSize, "%s/%s/%s", appDataPath, CFilesystem_appFolder, relativePath);
	// Replace forward slashes to standardize the path
	for (char *ptr = destPath; *ptr; ptr++) {
		if (*ptr == '/') { *ptr = '\\'; }
	}
	// Extract the path
	safe::strcpy(directoryOnly, destPath);
	for (char *ptr = directoryOnly + strlen(directoryOnly) - 1; ptr >= directoryOnly; ptr--) {
		if (*ptr == '\\') {
			*ptr = '\0';
			break;
		}
	}
	// Create the directory
	SHCreateDirectoryExA(NULL, directoryOnly, NULL);
}

bool CFilesystemHandler::Delete(const char *relativePath) {
	char fullPath[MAX_PATH];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return DeleteFileA(fullPath) != 0;
}

CInputFile *CFilesystemHandler::OpenForReading(const char *relativePath) {
	char fullPath[MAX_PATH];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return new CInputFileStdio(fullPath);
}

COutputFile *CFilesystemHandler::OpenForWriting(const char *relativePath) {
	char fullPath[MAX_PATH];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return new COutputFileStdio(fullPath);
}
