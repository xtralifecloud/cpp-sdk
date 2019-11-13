//
//  CFilesystemManager.h
//  XtraLife
//
//  Base classes for persisting/reading data from a virtual file system.
//
//  Created by Florian B on 13/08/14.
//  Copyright 2014 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CFilesystemManager_h
#define XtraLife_CFilesystemManager_h

#include <stdlib.h>

#include "XtraLife.h"
#include "CClan.h"
#include "CDelegate.h"

namespace XtraLife {

    namespace Helpers {
        class CHJSON;
    }
	/**
	 * Platform-dependent file structures (default implementations provided in CStdioBasedFileImpl.cpp).
	 * Represents a file open for reading. Getting an instance of CInputFile does not guarantee the file
	 * has actually been opened. Check isOpen() or the result of the read functions.
	*/
	class FACTORY_CLS CInputFile {
	public:
		virtual ~CInputFile() {}

		/**
		 * Should close the file for use by other processes. If already done, should have effect.
		*/
		virtual void Close() = 0;

		/**
		 * Should tell whether the file has been opened successfully.
		 * @return true if the file is ready for read, false otherwise.
		*/
		virtual bool IsOpen() = 0;

		/**
		 * Reads a chunk of data from the current cursor position.
		 * @param aDestBuffer where to put the read data.
		 * @param aNumBytes the maximum number of bytes to read.
		 * @return the number of bytes actually read (may be less than numBytes if the stream came to an end).
		*/
		virtual size_t Read(void *aDestBuffer, size_t aNumBytes) = 0;

		/**
		 * Reads the whole file from the current cursor position.
		 * @return resulting string (or null in case of error), to be freed manually!
		*/
		char *ReadAll();

		/**
		 * Should give the absolute position of the cursor in the file.
		 * @return the position, starting from zero.
		*/
		virtual size_t Tell() = 0;

		/**
		 * Replaces the cursor just like fseek.
		 * @param aOffset offset relative to whence.
		 * @param aWhence one of SEEK_SET (beginning), SEEK_CUR or SEEK_END.
		*/
		virtual bool Seek(long aOffset, int aWhence) = 0;
	};
	
	/**
	 * Represents a file open for writing. Getting an instance of COutputFile does not guarantee the file
	 * has actually been opened. Check isOpen() or the result of the writes functions.
	*/
	class FACTORY_CLS COutputFile {
	public:
		virtual ~COutputFile() {}

		/**
		 * Should close the file for use by other processes. If already done, should have effect.
		*/
		virtual void Close() = 0;

		/**
		 * Should tell whether the file has been opened successfully.
		 * @return true if the file is ready for write, false otherwise.
		*/
		virtual bool IsOpen() = 0;

		/**
		 * Should write a chunk of data at the end of the file.
		 * @param aSourceBuffer data to write.
		 * @param aNumBytes number of bytes to write.
		*/
		virtual size_t Write(const void *aSourceBuffer, size_t aNumBytes) = 0;
	};
	
	/**
	 * Platform-dependent file system handler, can even be overriden by the app developer.
	 */
	class FACTORY_CLS CFilesystemHandler {
	public:
		/**
		 * Should delete a file with a given name.
		 * @param aRelativePath file name.
		*/
		virtual bool Delete(const char *aRelativePath);

		/**
		 * Should open a file for input.
		 * @param aRelativePath file name.
		 * @return an appropriate CInputFile reading from however the stream is represented on your device.
		*/
		virtual CInputFile *OpenForReading(const char *aRelativePath);

		/**
		 * Should open a file for output.
		 * @param aRelativePath file name.
		 * @return an appropriate COutputFile writing to however the stream is represented on your device.
		*/
		virtual COutputFile *OpenForWriting(const char *aRelativePath);
	};
	
	/**
	 * File system manager, allowing to write and read files. This class is used by the system but users may
	 also have access to it.
	 * You can pass a custom file system handler in order to customize how and where files are written on your system.
	*/
	class FACTORY_CLS CFilesystemManager {
	public:
		/**
		 * @return a static instance of this manager.
		*/
		static CFilesystemManager *Instance();
		
		/**
		 * Deletes a file.
		 * @param aRelativePath file name.
		 * @return whether the deletion succeeded.
		*/
		bool Delete(const char *aRelativePath);

		/**
		 * Opens a file for input.
		 * @param aRelativeName file name.
		 * @return an input file -- may not actually be opened (check isOpen()).
		*/
		CInputFile *OpenFileForReading(const char *aRelativeName);
		
		/**
		 * Opens a file for output.
		 * @param aRelativeName file name.
		 * @return an output file -- may not actually be writable (check isOpen()).
		*/
		COutputFile *OpenFileForWriting(const char *aRelativeName);
		
		/**
		 * Allows to replace the file system handler. You can use that if you want to persist
		 * data in a special fashion, such as in RAM.
		 * @param aHandler handler to use from now on.
		 * @param aDeleteAutomatically whether to delete this instance when no longer used.
		*/
		void SetFilesystemHandler(CFilesystemHandler *aHandler, bool aDeleteAutomatically = true);

		/**
		 * Reads a JSON persisted to the storage.
		 * @param aRelativeName file name.
		 * @return a JSON that you need to delete.
		*/
		Helpers::CHJSON *ReadJson(const char *aRelativeName);
		bool WriteJson(const char *aRelativeName, const Helpers::CHJSON *aJSON);

	private:
		CFilesystemHandler *mHandler;
		bool mAutoFreeHandler;

		/**
		 * Manual construction and subclassing not allowed.
		*/
		CFilesystemManager();
		
		/**
		 * Call this to set the app folder name. Will be used by the default CFilesystemHandler.
		*/
		void SetFolderHint(const char *aAppFolder);
		
		/**
		 * You need to call this in CClan::Terminate().
		*/
		void Terminate();

		friend void CClan::Setup(const Helpers::CHJSON*, CResultHandler*);
		friend void CClan::Terminate();
		friend struct singleton_holder<CFilesystemManager>;
	};
}

#endif
