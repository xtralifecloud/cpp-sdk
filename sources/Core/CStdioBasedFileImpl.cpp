#include "CStdioBasedFileImpl.h"

using namespace XtraLife;

CInputFileStdio::CInputFileStdio(const char *fileName) {
	underlyingFile = fopen(fileName, "rb");
}

CInputFileStdio::~CInputFileStdio() {
	Close();
}

void CInputFileStdio::Close() {
	if (underlyingFile) {
		fclose(underlyingFile);
	}
	underlyingFile = NULL;
}

bool CInputFileStdio::IsOpen() {
	return underlyingFile != NULL;
}

size_t CInputFileStdio::Tell() {
	if (!underlyingFile) {
		return -1L;
	}
	return ftell(underlyingFile);
}

size_t CInputFileStdio::Read(void *destBuffer, size_t numBytes) {
	return fread(destBuffer, 1, numBytes, underlyingFile);
}

bool CInputFileStdio::Seek(long offset, int whence) {
	if (!underlyingFile) {
		return false;
	}
	return !fseek(underlyingFile, offset, whence);
}

COutputFileStdio::COutputFileStdio(const char *fileName) {
	underlyingFile = fopen(fileName, "wb");
}

COutputFileStdio::~COutputFileStdio() {
	Close();
}

void COutputFileStdio::Close() {
	if (underlyingFile) {
		fclose(underlyingFile);
	}
	underlyingFile = NULL;
}

bool COutputFileStdio::IsOpen() {
	return underlyingFile != NULL;
}

size_t COutputFileStdio::Write(const void *sourceBuffer, size_t numBytes) {
	return fwrite(sourceBuffer, 1, numBytes, underlyingFile);
}
