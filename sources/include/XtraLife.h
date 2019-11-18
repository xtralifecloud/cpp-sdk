//
//  XtraLife.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 20/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef __XtraLife__
#define __XtraLife__

#if defined(_WINDOWS)

#if defined(XTRALIFE_EXPORTS)
#define FACTORY_CLS  __declspec(dllexport)
#else
#define FACTORY_CLS  __declspec(dllimport)
#endif  // XTRALIFE_EXPORTS

#else	// _WINDOWS

// all others...
#ifndef FACTORY_CLS
#define FACTORY_CLS
#endif

#ifndef NULL
#define NULL 0L
#endif

#endif

#ifdef _MSC_VER
#define DEPRECATED __declspec(deprecated)
#else
#define DEPRECATED __attribute__ ((deprecated))
#endif

/*! \file XtraLife.h
 */

/** Namespace XtraLife is the default namespace for all Clan of the Cloud APIs.
 */
namespace XtraLife
{
	/** Enum used to specify errors returns by CotC functions and methods.
	 * To be kept in sync with EErrorCode.java (Android).
	 */
	typedef enum
	{
		///   No error.
		enNoErr = 0,
		
		enFirstError = 1000,
		
		/// 1001 - Trying to access some features, but CClan::Setup has not been called.
		enSetupNotCalled,
		
		/// 1002 - Trying to access some features, but the user is not logged yet.
		enNotLogged,
		
		/// 1003 - Trying to register an event listener twice for a domain
		enEventListenerAlreadyRegistered,
		
		/// 1004 - Networking problem (unable to reach the server)
		enNetworkError,
		
		/// 1005 - Internal server error. Contact CotC.
		enServerError,
		
		/// 1006 - Functionality not yet implemented. Contact CotC.
		enNotImplemented,
		
		/// 1007 - The user is already logged in another session.
		enAlreadyLogged,
		
		/// 1008 - Invalid parameters passed to CClan::Setup.
		enBadAppCredential,
		
		/// 1009 - Trying to use external community features but CClan constructor did not specify it.
		enExternalCommunityNotSetup,
		
		/// 1010 - Something is missing in the JSON you sent with your query.
		enBadParameters,
		
		/// 1011 - User is not logged into GooglePlayServices.
		enUserNotLoggedInGooglePlayServices,
		
		/// 1012 - An error occured when communicating with an external community on the client side.
		enExternalCommunityError,
		
		/// 1013 - The Google+ application is not installed on the device.
		enGooglePlusAppNotInstalled,
		
		/// 1014 - The user has canceled posting on Google+.
		enUserCanceledGooglePlusPost,
		
		/// 1015 - An error occured when using GooglePlay services.
		enErrorGooglePlayServices,
		
		/// 1016 - The operation has been canceled
		enCanceled,
		
		/// 1017 - The operation has been canceled
		enAlreadyLoggedWhileRestoringASession,

		/// 1018 - This operation (or a similar one) is already in progress, and this request was discarded as a result
		enOperationAlreadyInProgress,

		/// 1019 - You can not be a friend with yourself
		enFriendYourself,

		/// 1020 - You are trying to use an object that was created prior to a Terminate call
		enObjectDestroyed,

		/// 1021 - No match to resume
		enNoMatchToResume,

		/// 1022 - The permission has not been given by the user to perform the operation
		enExternalCommunityRefusedPermission,
		
		/// 1023 - Product not found
		enProductNotFound,

		/// 1024 - Logic error
		enLogicError,
		
		/// 1025 - Generic problem with the external "app" store
		enErrorWithExternalStore,
		
		/// 1026 - The user canceled the purchase
		enUserCanceledPurchase,
		
		/// 1027 - Internal error with the library
		enInternalError,
				
		/// You shouldn't receive this error, it's just a convenient value
		enLastError
		
	} eErrorCode;

	/** Call this function when you want a written description for an error
		@param err is the error whose description is required.
		@result is the string explaining the error.
	 */
	const char *errorString(eErrorCode err);

	/** \cond INTERNAL_USE */
	template<class DelegateType> struct CGloballyKeptHandler;
	template<class T> struct singleton_holder;
	/** \endcond */
}

#endif
