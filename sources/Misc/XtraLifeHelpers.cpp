//
//  XtraLifeHelpers.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 04/05/12.
//  Copyright (c) 2012 Clan of the Cloud. All rights reserved.
//

#include <time.h>
#include <stdlib.h>

#include "include/XtraLifeHelpers.h"

namespace XtraLife {
    namespace Helpers {
	
        void Init(void)
        {
            static bool sInitDone = false;

            if(!sInitDone)
            {
                srand((unsigned int) time(0));
    //			cJSON_InitHooks(NULL);

                sInitDone	= true;
            }
        
        }

        CRefClass::~CRefClass() noexcept(false) {
            if ((int) __ref_count > 0) throw "Freeing an object that is still retained elsewhere";
        }

    }
}
