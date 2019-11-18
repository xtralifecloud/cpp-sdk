//
//  CIndexManager.cpp
//  XtraLife
//
//  Created by Florian Bronnimann on 11/06/15.
//  Copyright (c) 2015 Clan of the Cloud. All rights reserved.
//

#include "include/CIndexManager.h"
#include "include/CClan.h"
#include "Core/CCallback.h"
#include "Core/CClannishRESTProxy.h"
#include "Misc/helpers.h"

using namespace XtraLife::Helpers;

namespace XtraLife {
	
	static singleton_holder<CIndexManager> managerSingleton;

	CIndexManager::CIndexManager() {
	}
	
	CIndexManager::~CIndexManager() {
	}

	CIndexManager *CIndexManager::Instance() {
		return managerSingleton.Instance();
	}

	void CIndexManager::Terminate() {
		managerSingleton.Release();
	}

	void CIndexManager::DeleteObject(const XtraLife::Helpers::CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }

		CClannishRESTProxy::Instance()->DeleteIndexedObject(aConfiguration, MakeBridgeDelegate(aHandler));
	}

	void CIndexManager::FetchObject(const XtraLife::Helpers::CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }

		CClannishRESTProxy::Instance()->GetIndexedObject(aConfiguration, MakeBridgeDelegate(aHandler));
	}

	void CIndexManager::IndexObject(const XtraLife::Helpers::CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }

		CClannishRESTProxy::Instance()->IndexObject(aConfiguration, MakeBridgeDelegate(aHandler));
	}

	void CIndexManager::Search(const XtraLife::Helpers::CHJSON *aConfiguration, CResultHandler *aHandler) {
		if (!CClan::Instance()->isSetup()) { InvokeHandler(aHandler, enSetupNotCalled); }

		CClannishRESTProxy::Instance()->SearchIndexedObjects(aConfiguration, MakeBridgeDelegate(aHandler));
	}
}

