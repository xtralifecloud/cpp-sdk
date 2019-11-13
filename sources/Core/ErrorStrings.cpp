//
//  ErrorStrings.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 23/09/11.
//  Copyright 2011 Clan of the CLoud. All rights reserved.
//

#include "XtraLife.h"
#include "Misc/helpers.h"

namespace XtraLife {
	// The first two are special, the other are descriptions for codes starting from 1001
	static const char *eErrorStrings[enLastError - enFirstError + 1] = {
		"No error",
		"Undefined error",

		"Please call setup prior to issuing this command",
		"You need be logged in to use this functionality",
		"An event listener for this domain is already registered",
		"Networking error (unable to reach the server)",
		"Server side error",
		"Functionality not implemented",
		"User already logged, call Logout before Login",
		"Bad application credentials passed at Setup",
		"Try to use External Community login but the CClan constructor does not specify it",
		"Parameters passed to the function are either out of constraints or missing some field",
		"User is not logged into GooglePlay Services",
		"Error on SocialSDK  Layer (Facebook, GooglePlus or GameCenter)",
		"The Google+ application is not installed on the device",
		"The user has canceled posting on Google+",
		"An error occured when using GooglePlay services",
		"The operation has been canceled",
		"The user attempt to resore a session through an url while he is still logged with another account",
		"This operation is already in progress",
		"Can't be friend with yourself!",
		"You are trying to use an object that was created prior to a Terminate call",
		"No match to resume",
		"The permission to perform this operation has been refused by the user",
		"The product does not exist",
		"Logic error, please check your code",
		"Error with the external store",
		"User canceled the purchase",
		"Internal error",
	};

	const char *errorString(eErrorCode err) {
		if (err == 0) {
			// No error
			return eErrorStrings[0];
		} else if (err >= enFirstError && err < enFirstError + (int) numberof(eErrorStrings)) {
			// Normal errors
			return eErrorStrings[err - enFirstError + 1];
		}
		// Undefined error
		return eErrorStrings[1];
	}
}
