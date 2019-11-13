//
//  XtraLife_private.h
//
//  Created by Roland Van Leeuwen on 20/09/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef XtraLife_private_h
#define XtraLife_private_h

#include "CLogLevel.h"

#ifndef NULL
	#define NULL 0
#endif

#define SDKVERSION "3.2.0"

#define LIBCOTC_VERSION     SDKVERSION
#define LIBCOTC_UA          "cloudbuilder"

#define kForbiddenCharForMail "\"'<>,"
#define kForbiddenChar "\"'<>,-."
#define kAllowedChar "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_"

#if defined(WIN32) || defined(__WP8__)

#define STDCALL __stdcall

#else	// WIN32

#ifndef STDCALL
#define STDCALL
#endif

#endif

typedef enum {
	eNone=0,
	eRequested,
	eAsked,
	eFriend,
	eReject,
	eForget
} eRelationState;

#ifdef _WINDOWS
	#define LIBCOTC_OS "Windows"
#elif defined(__IOS__)
	#define LIBCOTC_OS "iOS"
#elif defined(__MACOS__)
	#define LIBCOTC_OS "macOS"
#elif defined(__ANDROID__)
	#define LIBCOTC_OS "Android"
#else
	#define LIBCOTC_OS "UNKNOWN"
#endif

void setuuid(char *dest);

#undef CURL_TIMER

extern LOG_LEVEL g_debugLevel;

#ifdef __ANDROID__
	#include <android/log.h>
	#define CONSOLE(...)   __android_log_print(ANDROID_LOG_ERROR, "XtraLife[stderr]", __VA_ARGS__);
#else
    #define CONSOLE(...)    fprintf(stderr, __VA_ARGS__);
#endif

#define CONSOLE_ERROR(...) 		{ if (g_debugLevel>=LOG_LEVEL_ERROR)  CONSOLE( __VA_ARGS__) }
#define CONSOLE_WARNING(...) 	{ if (g_debugLevel>=LOG_LEVEL_WARNING) CONSOLE( __VA_ARGS__) }
#define CONSOLE_VERBOSE(...) 	{ if (g_debugLevel>=LOG_LEVEL_VERBOSE) CONSOLE( __VA_ARGS__) }
#define CONSOLE_EXTRA(...) 		{ if (g_debugLevel>=LOG_LEVEL_EXTRA) CONSOLE( __VA_ARGS__) }

#if defined(__WINDOWS_32__)
#if _MSC_VER < 1900
#	define snprintf	_snprintf
#endif
#endif

#if defined(__IOS__)
#	define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#	define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#	define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#	define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#	define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)
#endif

#ifdef _WINDOWS
	#if _MSC_VER >= 1700
	#	define FOR_EACH(var, list) for (var : list)
	#else
	#	define FOR_EACH(var, list) for each (var in list)
	#endif
#else
	#define FOR_EACH(var, list) for (var : list)
#endif


#endif 
