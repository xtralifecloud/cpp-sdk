//
//  performOnMain.mm
//  XtraLife
//
//  Created by Roland Van Leeuwen on 27/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#import <UIKit/UIKit.h>

#include <sys/sysctl.h>

#include "CHJSON.h"
#include "CUserManager.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

UIBackgroundTaskIdentifier bgTask = UIBackgroundTaskInvalid;

namespace XtraLife {

    void AchieveRegisterDevice(unsigned long len, const void *bytes)
    {
        NSData *deviceToken = [NSData dataWithBytes:bytes length:len];
        NSString *tokenString = [NSString stringWithFormat:@"%@", deviceToken];
        tokenString = [tokenString stringByReplacingOccurrencesOfString:@"<" withString:@""];
        tokenString = [tokenString stringByReplacingOccurrencesOfString:@">" withString:@""];
        tokenString = [tokenString stringByReplacingOccurrencesOfString:@" " withString:@""];

        CUserManager::Instance()->RegisterDevice("ios", [tokenString UTF8String]);
    }

}

void RegisterDevice(void)
 {
	 if ([[UIApplication sharedApplication] respondsToSelector:@selector(registerUserNotificationSettings:)]){
		 [[UIApplication sharedApplication] registerUserNotificationSettings:[UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeSound | UIUserNotificationTypeAlert | UIUserNotificationTypeBadge) categories:nil]];
		 [[UIApplication sharedApplication] registerForRemoteNotifications];
	 }
	 else{
		 [[UIApplication sharedApplication] registerForRemoteNotificationTypes:
		  (UIUserNotificationTypeBadge | UIUserNotificationTypeSound | UIUserNotificationTypeAlert)];
	 }
 }

void UnregisterDevice(void)
{
	[[UIApplication sharedApplication] unregisterForRemoteNotifications];
}

namespace XtraLife {
    namespace Helpers {

        CHJSON *collectDeviceInformation() {

            CHJSON *j = new CHJSON();
            
            UIDevice *d = [UIDevice currentDevice];
            size_t size;

            sysctlbyname("hw.machine", NULL, &size, NULL, 0);
            char *machine = new char[size];
            sysctlbyname("hw.machine", machine, &size, NULL, 0);

            j->Put("id", [[d.identifierForVendor UUIDString] UTF8String]);
            j->Put("osname", [d.systemName UTF8String]);
            j->Put("osversion", [d.systemVersion UTF8String]);
            j->Put("name", [d.name UTF8String]);
            j->Put("model", machine);
            j->Put("version", "1");
            
            delete [] machine;

            return j;
        }
    
    }
}
