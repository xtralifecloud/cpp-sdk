//
//  curltool.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 10/10/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#include <list>

#include "include/CHJSON.h"
#include "include/CDelegate.h"
#include "include/CFastDelegate.h"
#include "include/CHttpFailureEventArgs.h"
#include "Core/XtraLife_private.h"
#include "Core/CCallback.h"
#include "Misc/curltool.h"

#include "curl.h"

using std::list;
using namespace XtraLife;
using namespace XtraLife::Helpers;

namespace XtraLife {
	
    /**
	 * Used to store the credentials passed to http_init.
	 */
	struct CRESTAppCredentials {
		CRESTAppCredentials() : loadBalancerId(0), loadBalancerCount(0), needsChooseNewLoadBalancer(true) {}

		int loadBalancerId;					// selected load balancer (1..loadBalancerCount)
		int loadBalancerCount;				// maximum number of load balancers
		Helpers::cstring serverBaseName;				// templated, with [id] being the load balancer ID
		bool needsChooseNewLoadBalancer;	// set to true to choose a new balancer at the next request
	};

	/// IOBuf structure
	typedef struct IOBuf 
	{
		char* 	buffer;
		size_t 	size;
		size_t 	capacity;

		char* 	result;
		char* 	lastMod;
		char* 	eTag;
		int 	contentLen;
		int 	len;
		int 	code;
		bool    binary;
		bool	obsolete;
	} IOBuf;


	IOBuf *curl_iobuf_new();
	void curl_iobuf_free(IOBuf *bf);

	/**
	 * HTTP request dispatcher. Call enqueueRequest and it will be processed.
	 */
	class RequestDispatcher : public XtraLife::Helpers::CThread {
		// Pending requests; memory is owned here until they are processed
		XtraLife::Helpers::CProtectedVariable< list<CHttpRequest*> > mRequestGuard;
		bool mAlreadyStarted, mActive;
		int threadId;

		RequestDispatcher() : mAlreadyStarted(false), mActive(false) {}
		RequestDispatcher(const RequestDispatcher &copy_not_allowed);
		~RequestDispatcher() { 	CONSOLE_VERBOSE("Destroying request dispatcher object %d\n", threadId); }

		void DequeueProcessedRequest();
		virtual void Run();

	public:
		// Set this to meaningful values before running
		static CRESTAppCredentials mCredentials;

		/**
		 * To be called from the main thread.
		 */
		static RequestDispatcher *Instance();

		/**
		 * To be called from the main thread. Indicates that there is a request to process.
		 */
		void EnqueueRequest(CHttpRequest *request);
		/**
		 * Blocking method, meant to be called internally.
		 */
		static CCloudResult *PerformRequest(CURL *ch, CHttpRequest *req);
		static bool ShouldChangeLoadBalancer(const CCloudResult *result);
		static bool ShouldRetryRequest(CHttpRequest *request, const CCloudResult *result);
		void Terminate();
		/**
		 * Call from the main thread. If the thread is running but waiting for the next request, unblocks it.
		 */
		void UnblockThread();
	};

	CRESTAppCredentials RequestDispatcher::mCredentials;
	static autoref<RequestDispatcher> requestDispatcherInstance;
	// This ID indicates the ID of the current (active) HTTP thread, disallowing old ones (which aren't yet deleted because they have pending requests ongoing) to call callbacks related to older CClan instances. Does only apply to enqueued requests (that is http_perform).
	static int g_activeRequestDispatcherThreadId = 0;
	char g_curlUserAgent[128];
	static int g_defaultTimeout, g_defaultConnectTimeout;
	// g_httpInited is set to false to stop any HTTP request
	static bool g_httpVerbose, g_httpInited = false;
	static CConditionVariable *g_synchronousCancelVariable;
	owned_ref<CDelegate<void(CHttpFailureEventArgs&)>> g_failureDelegate;
	// First we retry immediately (1 ms) on the other load balancer, then we delay a bit. Do not put a zero in there (means infinite).
	static const int RETRY_DELAYS_MILLISEC[] = {1, 1, 400, 400, 800, 800, 1600, 1600, 3200, 3200, 6400, 6400};
	static size_t _currentDelayId = 0;	// index in DELAYS_MILLISEC
	void SSLBIO_SetCustomCertificate();

	static void shouldRetryDefaultRoutine(CHttpFailureEventArgs &e) {
		_currentDelayId++;
		// Check that we didn't fail too many times
		if (_currentDelayId < numberof(RETRY_DELAYS_MILLISEC)) {
			e.RetryIn(RETRY_DELAYS_MILLISEC[_currentDelayId]);
		} else {
			// Give up but do not retry too soon, reset the delay at its last iteration
			_currentDelayId = numberof(RETRY_DELAYS_MILLISEC) - 1;
			e.Abort();
		}
	}

	CUrlBuilder::CUrlBuilder(const char *path, const char *server) {
		safe::strcpy(url, server ? server : "");
		Subpath(path);
	}

	CUrlBuilder& CUrlBuilder::Subpath(const char *path) {
		// Ensure that there is a slash at the end of the current path
		size_t len = strlen(url);
		if (len == 0 || url[len - 1] != '/') {
			safe::strcat(url, "/");
		}
		// Skip leading slash
		if (path[0] == '/') { path++; }
		safe::strcat(url, path);
		return *this;
	}

	CUrlBuilder& CUrlBuilder::QueryParam(const char *name, const char *value) {
		// Add separator
		char *slashLocation = strchr(url, '?');
		safe::strcat(url, slashLocation ? "&" : "?");
		safe::strcat(url, name);
		if (value) {
			safe::strcat(url, "=");
			safe::strcat(url, value);
		}
		return *this;
	}

	CUrlBuilder& CUrlBuilder::QueryParam(const char *name, int value) {
		char buffer[32];
		safe::sprintf(buffer, "%d", value);
		return QueryParam(name, buffer);
	}

	CHttpRequest::CHttpRequest(const char *url) : url(url), method(NULL), callback(NULL), connectTimeout(g_defaultConnectTimeout), timeout(g_defaultTimeout), retryPolicy(NonpermanentErrors), binaryUpload(false), binaryDownload(false), cancellationFlag(NULL) {}
}

#define CAPACITY 4096

bool g_networkState = true;

/// Create a new I/O buffer
/// \return a newly allocated I/O buffer
XtraLife::IOBuf *XtraLife::curl_iobuf_new() {
	IOBuf *bf = (IOBuf*) malloc(sizeof(IOBuf));
	memset(bf, 0, sizeof(IOBuf));
	
	bf->capacity = CAPACITY;
	bf->buffer = (char *) malloc(bf->capacity);
	bf->binary = false;
	return bf;
}

/// Release IO Buffer
/// \param  bf I/O buffer to be deleted
void XtraLife::curl_iobuf_free(IOBuf *bf) { 
	/// Release Things
	if ( bf->buffer  != NULL ) free ( bf->buffer  );
	if ( bf->result  != NULL ) free ( bf->result  );
	if ( bf->lastMod != NULL ) free ( bf->lastMod );
	if ( bf->eTag	!= NULL ) free ( bf->eTag	);
	free (bf);
}

/// Chomp (remove the trailing '\n' from the string
/// \param str string
static void __chomp(char *str) {
	if ( str[0] == 0 ) return;
	size_t ln = strlen(str);
	ln--;
	if ( str[ln] == '\n' ) str[ln] = 0;
	if ( ln == 0 ) return ;
	ln--;
	if ( str[ln] == '\r' ) str[ln] = 0;
}

/// Handles reception of the data
/// \param ptr pointer to the incoming data
/// \param size size of the data member
/// \param nmemb number of data memebers
/// \param stream pointer to I/O buffer
/// \return number of bytes processed
static size_t writefunc(void * ptr, size_t size, size_t nmemb, void * stream) {
	XtraLife::IOBuf* rec = (XtraLife::IOBuf*) stream;
	size_t bytes = size * nmemb;
	
	// Check the buffer size
	if ((rec->size + bytes) >= rec->capacity) { // == for the trailing '0'
		// Reallocate the buffer
		rec->capacity = 2 * (rec->size + bytes) + CAPACITY;
		char* buf = (char*) malloc(rec->capacity);
		memcpy(buf, rec->buffer, rec->size);
		free(rec->buffer);
		rec->buffer = buf;
	}
	
	// Copy the buffer contents
	memcpy(rec->buffer + rec->size, ptr, bytes);
	rec->size += bytes;
	if (!rec->binary) rec->buffer[rec->size] = 0; // C string ending
	return bytes;
}

/// Handles sending of the data
/// \param ptr pointer to the incoming data
/// \param size size of the data member
/// \param nmemb number of data memebers
/// \param stream pointer to I/O buffer
/// \return number of bytes written
static size_t readfunc(void * ptr, size_t size, size_t nmemb, void * stream) {
	XtraLife::CHttpRequest *req = (XtraLife::CHttpRequest *) stream;
	
	size_t sz = req->getNextSize(size*nmemb);
	memcpy(ptr, req->getNextData(sz), sz);
	return sz;
}

/// Process incoming header
/// \param ptr pointer to the incoming data
/// \param size size of the data member
/// \param nmemb number of data members
/// \param stream pointer to I/O buffer
/// \return number of bytes processed
static size_t header(char *ptr, size_t size, size_t nmemb, void *stream) {
	XtraLife::IOBuf *b = (XtraLife::IOBuf*) stream;
	if (!strncmp(ptr, "HTTP/1.1", 8)) {
		b->result = strdup ( ptr + 9 );
		__chomp(b->result);
		b->code   = atoi ( ptr + 9 );
	} else if (!strncmp(ptr, "ETag: ", 6)) {
		b->eTag = strdup ( ptr + 6 );
		__chomp(b->eTag);
	} else if (!strncmp(ptr, "Last-Modified: ", 14)) {
		b->lastMod = strdup ( ptr + 15 );
		__chomp(b->lastMod);
	} else if (!strncmp(ptr, "Content-Length: ", 15)) {
		b->contentLen = atoi ( ptr + 16 );
	} else if (!strncmp(ptr, "X-Obsolete: ", 12)) {
		b->obsolete = true;
	}
	
	return nmemb * size;
}

// Abort process ASAP when the lib is de-inited
static int progresscallback(bool *cancellationFlag, double dltotal, double dlnow, double ultotal, double ulnow) {
	if (!XtraLife::g_httpInited || (cancellationFlag && *cancellationFlag)) { return -1; }
	return 0;
}

//////////////////////////// Request dispatcher ////////////////////////////
void XtraLife::RequestDispatcher::DequeueProcessedRequest() {
	list<CHttpRequest*> *pendingRequests = mRequestGuard.LockVar();
	// First request has been processed, free memory
	delete pendingRequests->front();
	pendingRequests->pop_front();
	pendingRequests = mRequestGuard.UnlockVar();
}

void XtraLife::RequestDispatcher::EnqueueRequest(CHttpRequest *request) {
	// Sanity check
	if (!g_httpInited) {
		CONSOLE_VERBOSE("Discarding HTTP call because the HTTP layer is not initialized.\n");
		return;
	}

	// Enqueue request and signal worker thread
	list<CHttpRequest*> *pendingRequests = mRequestGuard.LockVar();
	pendingRequests->push_back(request);
	if (!mAlreadyStarted) {
		// Start thread on first time
		mAlreadyStarted = mActive = true;
		Start();
	} else {
		// Or signal that it has data to process
		mRequestGuard.SignalAll();
	}
	pendingRequests = mRequestGuard.UnlockVar();
}

XtraLife::RequestDispatcher * XtraLife::RequestDispatcher::Instance() {
	return requestDispatcherInstance ? requestDispatcherInstance : (requestDispatcherInstance <<= new RequestDispatcher);
}

CCloudResult *XtraLife::RequestDispatcher::PerformRequest(CURL *ch, CHttpRequest *req) {
	char fullurl[1024], lb_id_str[16], buffer[1024];
	static long g_reqCount = 0;
	long gcount = ++g_reqCount;
	
#ifdef DEBUG
	if (!strncmp(req->url, "http", 4)) {
		safe::strcpy(fullurl, req->url);
	} else
#endif
	{
		// Choose new load balancer
		if (mCredentials.needsChooseNewLoadBalancer) {
			int lb_count = mCredentials.loadBalancerCount, lb_id;
			do { lb_id = (rand() % lb_count) + 1; }
			while (lb_id == mCredentials.loadBalancerId && lb_count > 1);
			mCredentials.loadBalancerId = lb_id;
			mCredentials.needsChooseNewLoadBalancer = false;
		}

		// fullUrl = serverBaseName.replace("[id]", lb_id) + req.url;
		safe::strcpy(fullurl, mCredentials.serverBaseName);
		safe::sprintf(lb_id_str, "%02d", mCredentials.loadBalancerId);
		safe::replace_string(fullurl, "[id]", lb_id_str);
		if (g_httpVerbose) {
			CONSOLE_VERBOSE("Building URL with base %s -> %s\n", (const char *) mCredentials.serverBaseName, fullurl); 
		}
		safe::strcat(fullurl, req->url);
		if (g_httpVerbose) {
			CONSOLE_VERBOSE("Appending %s -> %s\n", (const char *)req->url, fullurl);
		}
	}

	IOBuf *b = curl_iobuf_new();
	curl_easy_reset(ch);
	struct curl_slist *slist = NULL;

	// Has JSON body?
	cstring jsonBody;
	if (req->json) {
		jsonBody = req->json->print();
		slist = curl_slist_append(slist, "Content-Type: application/json");
	}
	
	// Plus additional headers defined in the request
	for (std::map<const char*, cstring>::iterator it = req->headers.begin(); it != req->headers.end(); ++it) {
		safe::sprintf(buffer, "%s: %s", it->first, it->second.c_str());
		slist = curl_slist_append(slist, buffer);
	}
	
	print_current_time(buffer);
	const char *method = req->method ? req->method : (jsonBody ? "POST" : "GET");
	CONSOLE_VERBOSE("%s - %s URL[%ld]: %s\n", buffer, method, gcount,fullurl);
	curl_easy_setopt(ch, CURLOPT_URL, fullurl);
//	curl_easy_setopt(ch, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(ch, CURLOPT_USERAGENT, g_curlUserAgent);
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, slist);
	curl_easy_setopt(ch, CURLOPT_HEADERFUNCTION, header);
	curl_easy_setopt(ch, CURLOPT_HEADERDATA, b);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, b);
	curl_easy_setopt(ch, CURLOPT_PROGRESSDATA, req->cancellationFlag);
	curl_easy_setopt(ch, CURLOPT_PROGRESSFUNCTION, progresscallback);
	curl_easy_setopt(ch, CURLOPT_NOPROGRESS, 0);

	// Bypass OpenSSL checks
    curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0);
	// Post if JSON body is provided
	if (jsonBody) {
		curl_easy_setopt(ch, CURLOPT_POST, 1);
		curl_easy_setopt(ch, CURLOPT_POSTFIELDS, jsonBody.c_str());
	} else if (req->binaryUpload) {
		curl_easy_setopt(ch, CURLOPT_POST, 1);
		curl_easy_setopt(ch, CURLOPT_READDATA, req );
		curl_easy_setopt(ch, CURLOPT_READFUNCTION, readfunc );
		curl_easy_setopt(ch, CURLOPT_UPLOAD, 1 );
		curl_easy_setopt(ch, CURLOPT_INFILESIZE, req->dataLength );
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, false); // AWS fix
	} else if (req->binaryDownload) {
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, false); // AWS fix
	}
	
	curl_easy_setopt(ch, CURLOPT_CONNECTTIMEOUT, req->connectTimeout);
	curl_easy_setopt(ch, CURLOPT_TIMEOUT, req->timeout);
	if (req->method) {
		curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, req->method);
	}

	if (g_httpVerbose) {
		curl_easy_setopt(ch, CURLOPT_VERBOSE, 1L);
		if (jsonBody) {
			CONSOLE_VERBOSE("JSON body: %s\n", jsonBody.c_str());
		}
	}

	CURLcode retCode = curl_easy_perform(ch);

	CONSOLE_VERBOSE("response URL[%ld] %d: '%s':\n", gcount , retCode, b->result);
	if (g_httpVerbose) {
		if (retCode != CURLE_OK)
			CONSOLE_VERBOSE("Error: %s\n", curl_easy_strerror(retCode));
		if (b->size > 0 && !req->binaryDownload) {
			CONSOLE_VERBOSE("size: %ld\n'%s'\n", b->size, b->buffer);
		} else {
			CONSOLE_VERBOSE("size: %ld\n", b->size);
		}
	}

	// Query info about the result
	CCloudResult *result = NULL;
	if (retCode == 0) {
		long httpCode = 0;
		if (b->result) {
			if (req->binaryDownload && b->code==200) {
				XtraLife::Helpers::CHJSON *resjson = new XtraLife::Helpers::CHJSON();
				resjson->Put("url", req->url);
				result = new CCloudResult(enNoErr, resjson);
				result->SetBinary(b->buffer, b->size);
				b->buffer = NULL; // will be released by CCloudResult...
			} else if (req->binaryUpload) {
				CHJSON *resjson = new CHJSON();
				resjson->Put("url", req->url);
				result = new CCloudResult(enNoErr, resjson);
			} else {
				CHJSON *resjson = CHJSON::parse(b->buffer);
				if (resjson == NULL) resjson = new CHJSON();
				result = new CCloudResult(enNoErr, resjson);
			}
			if (b->obsolete) {
				result->SetObsolete(true);
				CONSOLE_WARNING("Using an obsolete method, consider migrating\n");
			}
		}
		if (!result) {
			result = new CCloudResult();
		}
		if (curl_easy_getinfo(ch, CURLINFO_RESPONSE_CODE, &httpCode) == CURLE_OK) {
			result->SetHttpStatusCode((int) httpCode);
			if (httpCode >= 400) {
				result->SetErrorCode(XtraLife::enServerError);
			}
		}
	} else {
		result = new CCloudResult();
		result->SetCurlErrorCode(retCode);
		result->SetErrorCode(XtraLife::enNetworkError);
	}

	curl_slist_free_all(slist);
//	curl_easy_cleanup(ch);
	curl_iobuf_free(b);
	return (CCloudResult*) result;
}

void XtraLife::RequestDispatcher::Run() {
	list<CHttpRequest*> *pendingRequests = mRequestGuard.LockVar();
	bool needNewBalancer = true;	// if needed to change the load-balancer at next attempt
	intptr_t lastRequestData = 0;
	bool needsFreeRequestData = false;
	int retryIn = 0;

	Retain(this);
	threadId = ++g_activeRequestDispatcherThreadId;
	CONSOLE_VERBOSE("Starting HTTP thread %d\n", threadId);

	CURL *ch = curl_easy_init();

	while (mActive) {
		// Upon custom error delegate, process requests anyway
		bool process = g_networkState || g_failureDelegate;
		// Process requests as they come
		while (!pendingRequests->empty() && mActive && process) {
			CHttpRequest *req = pendingRequests->front();
			// Allow other threads to push additional requests while we handle them
			pendingRequests = mRequestGuard.UnlockVar();
			CCloudResult *result = PerformRequest(ch, req);
			retryIn = 0;

			// If the request failed due to a recoverable error, pause for a while
			if (ShouldRetryRequest(req, result)) {
				// Each delay is tested twice on a different load-balancer
				if (needNewBalancer)  {
					mCredentials.needsChooseNewLoadBalancer = true;
					needNewBalancer = false;
				} else {
					needNewBalancer = true;
				}

				CHttpFailureEventArgs e(req->url, lastRequestData);
				if (g_failureDelegate)
					(*g_failureDelegate)(e);
				else
					shouldRetryDefaultRoutine(e);
				lastRequestData = e.UserData();
				if (e.retryDelay == -2) {
					CONSOLE_ERROR("The HTTP failure delegate did not call Abort or RetryIn. Aborting\n");
					e.Abort();
				}

				if (e.retryDelay != -1) {
					CONSOLE_VERBOSE("Request failed, will retry in %dms\n", e.retryDelay);
					retryIn = e.retryDelay;
				}
				else {
					CONSOLE_VERBOSE("Giving up request to %s, failed to many times\n", req->url.c_str());
					// Do not call callbacks for old threads
					if (threadId == g_activeRequestDispatcherThreadId) {
						CallbackStack::pushCallback(req->callback, result);
					}
					DequeueProcessedRequest();
				}
				
				// Acquire lock for next loop iteration
				pendingRequests = mRequestGuard.LockVar();
				break;
			} else if (ShouldChangeLoadBalancer(result)) {
				// Even if the policy doesn't tell to retry, we might want to try another load balancer next time
				mCredentials.needsChooseNewLoadBalancer = true;
			}

			// Once finished (reset error/delay variables)
			_currentDelayId = 0;
			needNewBalancer = true;
			// Do not call callbacks for old threads
			if (threadId == g_activeRequestDispatcherThreadId) {
				CallbackStack::pushCallback(req->callback, result);
			}
			DequeueProcessedRequest();
			// Acquire lock for next loop iteration
			pendingRequests = mRequestGuard.LockVar();
		}

		// We won't need the object data anymore
		if (retryIn == 0) {
			if (needsFreeRequestData) { delete (void*)lastRequestData; }
			lastRequestData = 0;
			}

		// Wait for the next job
		if (mActive) {
			// Wait indefinitely unless there is an error
			mRequestGuard.Wait(retryIn);
		}
	}

	curl_easy_cleanup(ch);
	
	CONSOLE_VERBOSE("Finished HTTP thread %d\n", threadId);
	mRequestGuard.UnlockVar();
	Release(this);
}

bool XtraLife::RequestDispatcher::ShouldChangeLoadBalancer(const CCloudResult *result) {
	CURLcode curlCode = (CURLcode) result->GetCurlErrorCode();
	// Known CURL error (network related) -> try later
	if (curlCode == CURLE_GOT_NOTHING ||
		curlCode == CURLE_COULDNT_CONNECT ||
		curlCode == CURLE_OPERATION_TIMEDOUT) {
			return true;
	}
	// Temporary problem server status code
	int httpStatus = result->GetHttpStatusCode();
	return ((httpStatus > 500 && httpStatus < 600) || httpStatus < 100); // do not retry the 500 errors (server crash)
}

bool XtraLife::RequestDispatcher::ShouldRetryRequest(CHttpRequest *request, const CCloudResult *result) {
	// Time to rest
	if (result->GetCurlErrorCode() == CURLE_ABORTED_BY_CALLBACK) { return false; }

	switch (request->retryPolicy) {
		case CHttpRequest::NonpermanentErrors:
			return ShouldChangeLoadBalancer(result);
		case CHttpRequest::AllErrors:
			return result->GetCurlErrorCode() != 0 || result->GetHttpStatusCode() >= 300 || result->GetHttpStatusCode() < 100;
		case CHttpRequest::Never:
		default:
			return false;
	}
}

void XtraLife::RequestDispatcher::Terminate() {
	// Wait for the end of the thread
	if (mAlreadyStarted) {
		// Wake up the thread and make it exit from its loop
		mActive = false;
		// Mark it as inactive
		g_activeRequestDispatcherThreadId++;
		mRequestGuard.SignalAll();
	}
	Join();
	requestDispatcherInstance <<= NULL;
}

void XtraLife::RequestDispatcher::UnblockThread() {
	if (mAlreadyStarted) {
		mRequestGuard.SignalAll();
	}
}

void XtraLife::http_init(const char *serverUrl, int loadBalancerCount, int connectTimeout, int timeout, bool httpVerbose, CConditionVariable *synchronousWaitAborter) {
	CRESTAppCredentials &creds = RequestDispatcher::Instance()->mCredentials;
	creds.serverBaseName = serverUrl;
	creds.loadBalancerCount = loadBalancerCount;
	g_defaultConnectTimeout = connectTimeout;
	g_defaultTimeout = timeout;
	g_httpVerbose = httpVerbose;
	g_synchronousCancelVariable = synchronousWaitAborter;
	g_httpInited = true;
}

void XtraLife::http_perform(XtraLife::CHttpRequest *request) {
	RequestDispatcher::Instance()->EnqueueRequest(request);
}

CCloudResult *XtraLife::http_perform_synchronous(CHttpRequest *request) {
	// Sanity check
	if (!g_httpInited) {
		CONSOLE_VERBOSE("Discarding HTTP call because the HTTP layer is not initialized.\n");
		return new CCloudResult(enLogicError, "HTTP request performed after a Terminate");
	}

	CRESTAppCredentials &creds = RequestDispatcher::Instance()->mCredentials;
	bool needNewBalancer = true;	// if needed to change the load-balancer at next attempt
	// Do not retry too often if the last synchronous request has failed
	static bool failedLastTime = false;
	size_t currentDelayId = failedLastTime ? numberof(RETRY_DELAYS_MILLISEC) - 1 : 0;
	CURL *ch = curl_easy_init();

	while (true) {
		CCloudResult *result = RequestDispatcher::PerformRequest(ch, request);
		if (RequestDispatcher::ShouldRetryRequest(request, result)) {
			// Each delay is tested twice on a different load-balancer
			if (needNewBalancer)  {
				creds.needsChooseNewLoadBalancer = true;
				needNewBalancer = false;
			} else {
				currentDelayId++;
				needNewBalancer = true;
			}

			// Check that we didn't fail too many times
			if (currentDelayId < numberof(RETRY_DELAYS_MILLISEC)) {
				CONSOLE_VERBOSE("Request failed, will retry in %dms\n", RETRY_DELAYS_MILLISEC[currentDelayId]);
				g_synchronousCancelVariable->Wait(RETRY_DELAYS_MILLISEC[currentDelayId]);
			} else {
				CONSOLE_VERBOSE("Giving up request to %s, failed to many times\n", request->url.c_str());
				failedLastTime = true;
				curl_easy_cleanup(ch);
				return result;
			}
		} else {
			// Success case -- Even if the policy doesn't tell to retry, we might want to try another load balancer next time
			if (RequestDispatcher::ShouldChangeLoadBalancer(result)) {
				creds.needsChooseNewLoadBalancer = true;
			}
			failedLastTime = false;
			curl_easy_cleanup(ch);
			return result;
		}
	}
}

void XtraLife::http_terminate() {
	g_httpInited = false;
	RequestDispatcher::Instance()->Terminate();
}

void XtraLife::http_trigger_pending() {
	RequestDispatcher::Instance()->UnblockThread();
}

