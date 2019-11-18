#include <Windows.h>
#include <string>

#include "include/CHJSON.h"
#include "Core/RegisterDevice.h"

using std::string;
using namespace XtraLife::Helpers;

void RegisterDevice(void)
{
}

void UnregisterDevice(void)
{
}

namespace XtraLife {
	namespace Helpers {

		CHJSON *collectDeviceInformation()
		{
			CHJSON* info = new CHJSON;
			char	name[100];
			DWORD size = 100;
			OSVERSIONINFO osvi;
			string version;

			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			version = std::to_string((unsigned long long) osvi.dwMajorVersion);
			version += ".";
			version += std::to_string((unsigned long long) osvi.dwMinorVersion);

			GetComputerNameA(name, &size);

			info->Put("id", name);
			info->Put("osname", "Windows");
			info->Put("osversion", version.c_str());
			info->Put("name", name);
			info->Put("version", "1");

			return new CHJSON;
		}

	}
}
