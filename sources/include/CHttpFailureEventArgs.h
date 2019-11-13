#ifndef XtraLife_HttpFailureEventArgs_h
#define XtraLife_HttpFailureEventArgs_h

#include "XtraLifeHelpers.h"

namespace XtraLife {
	struct CHttpFailureEventArgs {

		/**
			* Call this to abort the request. It won't be tried ever again.
			*/
		void Abort() {retryDelay = -1; }
		/**
			* Call this to retry the request later.
			* @param milliseconds time in which to try again. No other request will be executed during this time
			* (they will be queued) as to respect the issuing order. Please keep this in mind when setting a high
			* delay.
			*/
		void RetryIn(int milliseconds) { retryDelay = milliseconds; }
		/**
			* Set a pointer to user data.
			* You can call this from the callback; in case the request fails again, this data will be set to the
			* same value as set last time. It is always set to NULL when the request fails for the first time.
			* @param data pointer to be set
			* @param releaseAtTheEnd whether to automatically free the memory (call C++ delete on the pointer)
			*/
		void SetUserData(intptr_t data, bool releaseAtTheEnd) {
			mUserData = data;
			mReleasePointer = releaseAtTheEnd;
		}
		/**
			* @return the original URL that the request failed to reach
			*/
		const char *Url() { return mUrl; }
		/**
			* You can set this member from the callback; in case the request fails again, this data will be set
			* to the same value as set last time. It is always set to NULL when the request fails for the first time.
			* @return the user data as set previously
			*/
		intptr_t UserData() { return mUserData; }
		
	private:
		CHttpFailureEventArgs(const char *requestUrl, intptr_t userData) : mUserData(userData), retryDelay(-2), mUrl(requestUrl) {}
		XtraLife::Helpers::cstring mUrl;
		int retryDelay;
		intptr_t mUserData;
		bool mReleasePointer;
		friend class RequestDispatcher;
	};
}

#endif
