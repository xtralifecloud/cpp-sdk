//
//  dummyClass.cpp
//  CloudBuilder
//
//  Created by roland on 17/01/13.
//  Copyright (c) 2013 Clan of the Cloud. All rights reserved.
//

#include "CCallback.h"
#include "CStoreGlue.h"
#include "GameCenterHandler.h"
#include "CTribeManager.h"

using namespace CotCHelpers;

namespace CloudBuilder {

	static const char *ErrorDesc = "Functionality unavailable on this platform";

	// CStoreGlue
#ifdef WIN32
	struct CDummyStoreGlue: CStoreGlue {
		virtual void GetInformationAboutProducts(const CotCHelpers::CHJSON *productIds, CInternalResultHandler *onFinished) { InvokeHandler(onFinished, enNotImplemented, ErrorDesc); }
		virtual void LaunchPurchase(const CotCHelpers::CHJSON *productInfo, CInternalResultHandler *onFinished) { InvokeHandler(onFinished, enNotImplemented, ErrorDesc); }
	};

	singleton_holder<CDummyStoreGlue> singleton;

	CStoreGlue *CStoreGlue::Instance() {
		return singleton.Instance();
	}
#endif
}
