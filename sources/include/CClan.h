//
//  CClan.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 10/04/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CClan_h
#define XtraLife_CClan_h

#include "XtraLife.h"
#include "CHttpFailureEventArgs.h"
#include "CLogLevel.h"
#include "CDelegate.h"

/*! \file CClan.h
 */

namespace XtraLife
{
    namespace Helpers {
        class CHJSON;
    }

	class CUserManager;
	
	/** The XtraLife::CClan class is the most important class. This is your primary
		entry point in the XtraLife SDK. All the setup is done in this class.
	 */
	class FACTORY_CLS CClan {
	public:
		
		/** Function which returns the only instance of this class.
			@result is the only instance of the CClan class.
		*/
		static CClan *Instance();

		/** Destructor
		*/
		virtual	~CClan();

		/** Method to be called in order to know if the XtraLife SDK has been
			correctly setup or not.
			@result is a boolean. True if setup is correct, false otherwise.
		*/
		bool isSetup();
		
		/** Method to be called in order to know if the user is logged in or not.
			@result is a boolean. True if user is logged in, false otherwise.
		*/
		bool isUserLogged();

		/** This method is the first to call just after the constructor. This will prepare
			everything so you can make requests later.
			@param	aConfiguration is a JSON object holding the necessary connection details.
			The mandatory keys are:
			 - "key": string containing the community key.
			 - "secret": string containing the community secret.
			 - "env": string containing the environment. Currently, "sandbox" (testing environment)
			   and "prod" (production environment) are supported.

			The optional keys are:
			- "appFolder": used with the default CFileSystemHandler to set the folder in which
			  the data will be saved. On Windows, it would be `%USERPROFILE%\AppData\Roaming\<appFolder>\`.
			- "autoresume" : boolean to control if after a Setup the system has to proceed an automatic resumesession when possible.
			- "autoRegisterForNotification": by default, RegisterForNotification is called right after login.
			  By setting this key to 'false', you can manage when you want to register for notifications by
			  calling the CUserManager::RegisterForNotification at your convenience.
			- "connectTimeout": sets a custom timeout allowed when connecting to the servers. Defaults to 5.
			- "httpTimeout": sets a custom timeout for all requests. Defaults to no limit.
			- "eventLoopTimeout": sets a custom timeout for the long polling event loop. Should be used with care and set to a
			  high value (at least 60). Defaults to 590.
			- "httpVerbose": set to true to output detailed information about the requests performed to XtraLife servers. Can be used
			  for debugging, though it will pollute the logs very much.
			@param handler result handler whenever the call finishes (it might also be synchronous)
			@result if noErr, the json passed to the handler may contain:
			{ "_error" : 0}
		*/
		void Setup(const XtraLife::Helpers::CHJSON* aConfiguration, CResultHandler *handler);
				
		/** Method you must call when your application is going to the background, so we can
			ensure data integrity when the application is not active.
		*/
		void Suspend();
		
		/** Method you must call when your application is going to the foreground, so we can
			ensure data integrity when the application is getting the focus again.
		*/
		void Resume();
		
		/** Method to call before quitting the application, so the XtraLife can leave
			in a clean state.
			Warning: invalidates all pointers to objects returned by XtraLife as memory is freed, including those returned by Instance().
		*/
		void Terminate();
		
		/** Method to ping the server
			For now it returns the server time in `utc` key.
		 */
		void Ping(CResultHandler *handler);

		/** Method to handle launch app with url:
		 used to connect with facebook and google
		 Also used , in case the url match with "://xtralife/", for sponsorship and anonymous login on a new device.
		 @param aOptions is a json containing field "url" with the url.
		 On iOS, for GooglePlus, a field "source" must contains sourceApplication as defined in the method:
		 - (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
		 @param handler result handler whenever the call finishes (it might also be synchronous)
		*/
		void HandleURL(const XtraLife::Helpers::CHJSON *aOptions, CResultHandler *handler);
		
		/**
			* Call this in your main loop when you are in an idle state. Allows to process some events
			* and callbacks pending and run them in your main thread.
		*/
		void ProcessIdleTasks();

		/**
		 * When called once from your app, this function disables the default behavior of the HTTP layer (unless
		 * called back with a null parameter). That is, no more retry by default, no more "offline mode" with
		 * requests put in a pending state.
		 *
		 * In this "mode", requests are still executed serially (meaning that any request -- API call -- that was
		 * issued later on is only executed after success or failure of the previous). However, in case of failure,
		 * the request is not re-attempted automatically, instead the callback is called. From this callback, you
		 * can decide to either retry the request later, or abort it. Aborting it has the effect of allowing the
		 * next request to proceed.
		 *
		 * There is an exception with the domain event loop which is run once logged in. The behaviour of this
		 * loop cannot be altered, the callback is not called and the request will be retried anyway.
		 *
		 * Also note that the callback is only invoked upon retriable error (network related). If the server
		 * returns a non-recoverable error code or there is an error with an argument, the API call will fail
		 * immediately.
		 *
		 * @param aCallback the callback to be called whenever a request fails. Unlike all other uses of the
		 * delegates, this one can be called multiple times. Memory is managed by the library, that is the
		 * callback is freed when not in use anymore (additional call to this method or Terminate).
		 */
		void SetHttpFailureCallback(CDelegate<void(CHttpFailureEventArgs&)> *aCallback);

		/**
		 * Sets the logging level to be used for future calls. Note that this doesn't apply to all calls, some other calls related to native plugins may
		 * need their own setup in order to show a verbose output.
		 * @param logLevel logging level, DEBUG_LEVEL_WARNING by default.
		 */
		void SetLogLevel(LOG_LEVEL logLevel);

		/** \cond INTERNAL_USE */
		bool useAutoResume() { return mAutoresume; }
		
	private:
		bool hasCalledHandleUrl, isActive;
		friend class _RegisterAndLogin;
		friend class CUserManager;
		friend struct singleton_holder<CClan>;
		
		bool mAutoresume;
		
		CClan();
		
		CGloballyKeptHandler<CResultHandler> &restoreLoginHandler;
		void RestoreLogin(XtraLife::eErrorCode code, const XtraLife::CCloudResult *result);
		/** \endcond */
	};
}

#endif
