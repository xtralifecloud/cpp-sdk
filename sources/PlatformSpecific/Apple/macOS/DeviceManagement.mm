#include <Cocoa/Cocoa.h>

#include "CHJSON.h"
#include "RegisterDevice.h"
#include "CUserManager.h"

#include <sys/sysctl.h>

using namespace XtraLife;
using namespace XtraLife::Helpers;

namespace XtraLife {

	void AchieveRegisterDevice(unsigned long len, const void *bytes)
	{
		NSData *deviceToken = [NSData dataWithBytes:bytes length:len];
		NSString *tokenString = [NSString stringWithFormat:@"%@", deviceToken];
		tokenString = [tokenString stringByReplacingOccurrencesOfString:@"<" withString:@""];
		tokenString = [tokenString stringByReplacingOccurrencesOfString:@">" withString:@""];
		tokenString = [tokenString stringByReplacingOccurrencesOfString:@" " withString:@""];
		
		CUserManager::Instance()->RegisterDevice("macos", [tokenString UTF8String]);
	}

} // namespace

void RegisterDevice(void)
 {
	 [[NSApplication sharedApplication] registerForRemoteNotificationTypes:(NSRemoteNotificationType)(NSRemoteNotificationTypeBadge+NSRemoteNotificationTypeSound+NSRemoteNotificationTypeAlert)];
 }

void UnregisterDevice(void)
{
	[[NSApplication sharedApplication] unregisterForRemoteNotifications];
}

namespace XtraLife {
    namespace Helpers {

        CHJSON *collectDeviceInformation() {
            
            /*** WARNING : when you change the content of this json, don't forget to update the SDKVERSION ***/
            XtraLife::Helpers::CHJSON *j = new XtraLife::Helpers::CHJSON();
            
            size_t size;
            
            sysctlbyname("hw.model", NULL, &size, NULL, 0);
            char *machine = new char[size];
            sysctlbyname("hw.model", machine, &size, NULL, 0);

            NSProcessInfo *info = [NSProcessInfo processInfo];
            j->Put("id", [info.hostName UTF8String]);
            j->Put("osname", [info.operatingSystemName UTF8String]);
            j->Put("osversion", [info.operatingSystemVersionString UTF8String]);
            j->Put("name", [info.hostName UTF8String]);
            j->Put("model", machine);
            j->Put("version", "1");
            
            delete [] machine;
            
            return j;
        }
    
    }
}
