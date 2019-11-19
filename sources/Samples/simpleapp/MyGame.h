//
//  MyGame.hpp
//  simpleapp
//
//  Created by Michael El Baki on 23/10/2019.
//  Copyright © 2019 XtraLife. All rights reserved.
//

#ifndef MyGame_h
#define MyGame_h

#include "XtraLife.h"

namespace XtraLife
{
    class CCloudResult;
}

class MyGame
{
public:
    MyGame() { mRunning = false; }
    
    void Startup();
    void SetupDone(XtraLife::eErrorCode errorCode, const XtraLife::CCloudResult *result);
    void AnonymousLogin();
    void LoginHandler(XtraLife::eErrorCode aErrorCode, const XtraLife::CCloudResult *aResult);

    template<typename... Args> static void Log(const char* fmt, Args... args);
    
    bool IsRunning() { return mRunning; }
    
private:
    bool mRunning;
};

#endif /* MyGame_h */
