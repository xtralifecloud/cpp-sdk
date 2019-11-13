#include "CStdioBasedFileImpl.h"
#import <Foundation/Foundation.h>
#include "Misc/helpers.h"
#include "XtraLife_private.h"

using namespace XtraLife;

extern char *CFilesystem_appFolder;

static void buildFullPath(char *destPath, size_t bufSize, const char *relativePath) {
	// Fetch default directory
	char appDataPath[1024], directoryOnly[1024];
	NSArray *URLs = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
														   inDomains:NSUserDomainMask];
	[[URLs.lastObject path] getCString:appDataPath maxLength:numberof(appDataPath) encoding:NSUTF8StringEncoding];
	safe::sprintf(destPath, bufSize, "%s/%s/%s", appDataPath, CFilesystem_appFolder, relativePath);

	// Extract the path
	safe::strcpy(directoryOnly, destPath);
	for (char *ptr = directoryOnly + strlen(directoryOnly) - 1; ptr >= directoryOnly; ptr--) {
		if (*ptr == '/') {
			*ptr = '\0';
			break;
		}
	}
	// Create the directory
	NSURL *directoryUrl = [NSURL fileURLWithPath:[NSString stringWithCString:directoryOnly encoding:NSUTF8StringEncoding]];
	BOOL success =
		[[NSFileManager defaultManager] createDirectoryAtURL:directoryUrl
								  withIntermediateDirectories:YES
												   attributes:nil
														error:nil];
	if (!success) {
		CONSOLE_ERROR("Could not create directory %s\n", directoryOnly);
	}
}

bool CFilesystemHandler::Delete(const char *relativePath) {
	char fullPath[1024];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	NSString *fullPathString = [NSString stringWithCString:fullPath encoding:NSUTF8StringEncoding];
	return [[NSFileManager defaultManager] removeItemAtPath:fullPathString error:nil];
}

CInputFile *CFilesystemHandler::OpenForReading(const char *relativePath) {
	char fullPath[1024];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return new CInputFileStdio(fullPath);
}

COutputFile *CFilesystemHandler::OpenForWriting(const char *relativePath) {
	char fullPath[1024];
	buildFullPath(fullPath, sizeof(fullPath), relativePath);
	return new COutputFileStdio(fullPath);
}
