//
//  curltool.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 10/10/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef curltool_h
#define curltool_h

#include <map>

#include "Core/CCallback.h"
#include "Misc/helpers.h"

namespace XtraLife {
    namespace Helpers {
        class CHJSON;
    }
}

namespace XtraLife {
	struct CRESTAppCredentials;
	class CCloudResult;

    extern char g_curlUserAgent[128];
    
	/**
	 * Description of an HTTP request to be performed.
	 */
	struct CHttpRequest {
		enum RetryPolicy {
			NonpermanentErrors,			// Retry when a 5xx range HTTP code has been received and if the host can not be reached (recommended)
			AllErrors,					// Retry when any response more than 2xx is received or if any connection anomaly happens
			Never,						// Disable auto retry mechanism
		};

		/**
		 * Creates a request. The method is unset, meaning that the system will deduce the type depending on whether there is a body (POST) or not (GET).
		 * @param url URL to connect to
		 */
		CHttpRequest(const char *url);
		/**
		 * Sets the body of the HTTP request.
		 * @param json JSON object representing the body to send. This object will be owned by the request, so you need to pass a new instance and not delete it!
		 */
		void SetBody(Helpers::CHJSON *json) { this->json <<= json; }
		/**
		 * Sets the body of the HTTP request.
		 * @param ptr Pointer to binary data
		 * @param size size of the  binary data
		 */
		void SetBody(const void *body, size_t size) { binaryUpload = true; this->currentPos = 0; this->data = body; this->dataLength = size; }
		/**
		 * @param callback callback called once and only once upon completion, whether successful or not
		 */
		void SetCallback(CCallback *callback) { this->callback = callback; }
		/**
		 * Sets a header on the request.
		 * @param name the header name (expected to be a constant literal as it is not copied)
		 * @param value the header value (copied)
		 */
		void SetHeader(const char *name, const char *value) { headers[name] = value; }
		/**
		 * Sets the method for the request.
		 * @param method either "GET", "POST", "PUT" or "DELETE" (expected to be a constant literal as it is not copied)
		 */
		void SetMethod(const char *method, bool binary=false) { this->method = method; this->binaryDownload = binary; }
		/**
		 * Defines in what condition the request should be automatically retried under the hood (prior to calling the callback).
		 * Check out the RetryPolicy enum for more information.
		 * @param retryPolicy retry policy to use for this request
		 */
		void SetRetryPolicy(RetryPolicy retryPolicy) { this->retryPolicy = retryPolicy; }
		/**
		 * Sets a timeout for the connection to the server. Defaults to 5 sec.
		 */
		void SetConnectTimeout(int connectTimeoutSec) { this->connectTimeout = connectTimeoutSec; }
		/**
		 * Sets a timeout. By default (or if zero), the request lasts as long as the underlying system decides.
		 * @param timeoutSec timeout in seconds
		 */
		void SetTimeout(int timeoutSec) { this->timeout = timeoutSec; }
		/**
		 * @param setToTrueFromAnyThreadToAbort sets the cancellation flag for this request
		 */
		void SetCancellationFlag(bool *setToTrueFromAnyThreadToAbort) { cancellationFlag = setToTrueFromAnyThreadToAbort; }

		void *getNextData(size_t size) { char *p = (char*)this->data + this->currentPos; this->currentPos += size; return p;}
		size_t getNextSize(size_t maxSize) { return (maxSize >= this->dataLength-this->currentPos) ? this->dataLength-this->currentPos : maxSize; }
		
	private:
		const char *method;
		Helpers::cstring url;
		Helpers::owned_ref<Helpers::CHJSON> json;
		std::map<const char*, Helpers::cstring> headers;
		CCallback *callback;
		int connectTimeout, timeout;
		RetryPolicy retryPolicy;
		const void *data;
		size_t dataLength;
		bool binaryUpload;
		bool binaryDownload;
		size_t currentPos;
		bool *cancellationFlag;
		
		// Not allowed
		CHttpRequest(const CHttpRequest &other);
		CHttpRequest& operator = (const CHttpRequest &);
		friend class RequestDispatcher;
		friend CCloudResult *http_perform_synchronous(CHttpRequest *request);
	};

	/**
	 * Utility class that allows to build URLs more easily.
	 */
	struct CUrlBuilder {
		// Usually the max admitted URL size is around 2000 chars
		char url[2048];

		CUrlBuilder(const char *path, const char *server = NULL);
		/**
		 * Add a path to the URL. Make your calls to Subpath first.
		 * @param path path to append
		 * @return the same builder, enabling chaining operations
		 */
		CUrlBuilder& Subpath(const char *path);
		/**
		 * Add a query param to the URL. Do not make a call to Subpath after that.
		 * @param name parameter name
		 * @param value parameter value (if not passed, the key only is passed, without =value)
		 * @return the same builder, enabling chaining operations
		 */
		CUrlBuilder& QueryParam(const char *name, const char *value = NULL);
		CUrlBuilder& QueryParam(const char *name, int value);
		/**
		 * Creates a string that contains the final built URL.
		 * @return the URL; the lifetime of the URL is this object, so you shouldn't retain it
		 * further than this CUrlBuilder object will live. In practice it means that it is safe to
		 * have constructs such as this:
		 * @code CHttpRequest *req = new CHttpRequest(CUrlBuilder("/v1").Subpath("/api").BuildUrl());
		 */
		const char *BuildUrl() { return url; }
		/** Shortcut to build URL */
		operator const char *() { return url; }
	};

	/**
	 * Call prior to any request.
	 * @param serverUrl
	 * @param loadBalancerCount
	 * @param connectTimeout pass 0 for default
	 * @param timeout pass 0 for default
	 * @param httpVerbose
	 * @param sharedSynchronousWaitAborter you can signal this condition variable in order to abort waiting on synchronous operations
	 */
	void http_init(const char *serverUrl, int loadBalancerCount, int connectTimeout, int timeout, bool httpVerbose, Helpers::CConditionVariable *sharedSynchronousWaitAborter);
	/**
	 * Performs an HTTP request.
	 * @param request information about the request; the object will be owned by this function, so pass a 'new' reference and do not release it yourself
	 */
	void http_perform(CHttpRequest *request);
	/**
	 * Blocking version. The callback is never called (but the synchronous delegate is), and the result is returned directly.
	 * @param request information about the request; the object will be owned by this function, so pass a 'new' reference and do not release it yourself
	 * @return result; must be deleted by you. MAY BE NULL, meaning that the processing of the request must be stalled.
	 */
	CCloudResult *http_perform_synchronous(CHttpRequest *request);
	/**
	 * Blocking call that terminates all running HTTP tasks. Will typically wait until the current request returns and stop afterwards.
	 */
	void http_terminate();
	/**
	 * Triggers pending requests which may have been queued since there was no network connection.
	 * Call this function to indicate that a retry should be done.
	 */
	void http_trigger_pending();

	// Internal
	void configureCurlCerts(void *ch);
}

extern bool g_networkState;

#endif

