#include "include/CFilesystemManager.h"
#include "include/CHJSON.h"
#include "include/XtraLifeHelpers.h"
#include "Misc/helpers.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

static singleton_holder<CFilesystemManager> managerSingleton;
char *CFilesystem_appFolder = NULL;

CFilesystemManager::CFilesystemManager() {
	mHandler = new CFilesystemHandler(), mAutoFreeHandler = true;
}

CFilesystemManager *CFilesystemManager::Instance() {
	return managerSingleton.Instance();
}

void CFilesystemManager::Terminate() {
	managerSingleton.Release();
}

bool CFilesystemManager::Delete(const char *relativePath) {
	return mHandler->Delete(relativePath);
}

CInputFile *CFilesystemManager::OpenFileForReading(const char *relativeName) {
	return mHandler->OpenForReading(relativeName);
}

COutputFile *CFilesystemManager::OpenFileForWriting(const char *relativeName) {
	return mHandler->OpenForWriting(relativeName);
}

void CFilesystemManager::SetFilesystemHandler(CFilesystemHandler *handler, bool deleteAutomatically) {
	if (mAutoFreeHandler && mHandler) {
		delete mHandler;
	}
	mHandler = handler, mAutoFreeHandler = deleteAutomatically;
}

XtraLife::Helpers::CHJSON * CFilesystemManager::ReadJson(const char *relativeName) {
	owned_ref<CInputFile> file (OpenFileForReading(relativeName));
	if (file->IsOpen()) {
		cstring contents (file->ReadAll(), true);
		return CHJSON::parse(contents);
	}
	return NULL;
}

bool CFilesystemManager::WriteJson(const char *relativeName, const XtraLife::Helpers::CHJSON *json) {
	owned_ref<COutputFile> file (OpenFileForWriting(relativeName));
	if (file->IsOpen()) {
		cstring result = json->print();
		file->Write(result, strlen(result));
		return true;
	}
	return false;
}

void XtraLife::CFilesystemManager::SetFolderHint(const char *newAppFolder) {
	if (CFilesystem_appFolder) { free(CFilesystem_appFolder); }
	CFilesystem_appFolder = strdup(newAppFolder);
}

char *CInputFile::ReadAll() {
	if (!IsOpen()) {
		return NULL;
	}

	// Determine size
	size_t start, size;
	Seek(0, SEEK_SET);
	start = Tell();
	Seek(0, SEEK_END);
	size = Tell() - start;
	
	// Read all
	char *result = new char[size + 1];
	Seek(0, SEEK_SET);
	Read(result, size);
	result[size] = '\0';
	return result;
}
