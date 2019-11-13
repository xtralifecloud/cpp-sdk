#ifndef __REGISTERDEVICE__
#define __REGISTERDEVICE__

namespace XtraLife {
    namespace Helpers
    {
        class CHJSON;
        CHJSON *collectDeviceInformation();
    }
}

void RegisterDevice(void);
void UnregisterDevice(void);

#endif	// __REGISTERDEVICE__
