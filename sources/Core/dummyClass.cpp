//
//  dummyClass.cpp
//  CloudBuilder
//
//  Created by roland on 17/01/13.
//  Copyright (c) 2013 Clan of the Cloud. All rights reserved.
//

#include "Core/CCallback.h"
#include "Core/CStoreInterface.h"
#include "include/CTribeManager.h"
#include "Misc/helpers.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

namespace XtraLife {

	static const char *ErrorDesc = "Functionality unavailable on this platform";

	// CStoreGlue
#ifdef _WINDOWS
	struct CDummyStoreGlue: CStoreInterface {
		virtual void GetInformationAboutProducts(const CHJSON *productIds, CInternalResultHandler *onFinished) { InvokeHandler(onFinished, enNotImplemented, ErrorDesc); }
		virtual void LaunchPurchase(const CHJSON *productInfo, CInternalResultHandler *onFinished) { InvokeHandler(onFinished, enNotImplemented, ErrorDesc); }
	};

	singleton_holder<CDummyStoreGlue> singleton;

	CStoreInterface* CStoreInterface::Instance() {
		return singleton.Instance();
	}
#endif
}
