//
//  MyGame.cpp
//  simpleapp
//
//  Created by Michael El Baki on 23/10/2019.
//  Copyright © 2019 XtraLife. All rights reserved.
//

#include "MyGame.h"

#include <string>
#include <iostream>

#include "CHJSON.h"
#include "CClan.h"
#include "CUserManager.h"

void MyGame::Startup()
{
    mRunning = true;
    
    XtraLife::Helpers::CHJSON json;
    //json.Put("key", "xxxxxxxxx");       // To get your own application id and secret
    //json.Put("secret", "yyyyyyyyy");    // create an account at https://account.clanofthecloud.com
    json.Put("key", "cloudbuilder-key");       // To get your own application id and secret
    json.Put("secret", "azerty");    // create an account at https://account.clanofthecloud.com
    json.Put("env", "sandbox");         // "sandbox" to develop and test, "production" to go live
    XtraLife::CClan::Instance()->Setup(&json, XtraLife::MakeResultHandler(this, &MyGame::SetupDone));
}
void MyGame::SetupDone(XtraLife::eErrorCode errorCode, const XtraLife::CCloudResult *result)
{
    if (errorCode == XtraLife::eErrorCode::enNoErr)
    {
        MyGame::Log("Setup complete!");
        
        this->AnonymousLogin();
    }
    else
    {
        MyGame::Log("Setup failed with message %s", result->GetErrorString());
    }
}

void MyGame::AnonymousLogin()
{
    XtraLife::CUserManager::Instance()->LoginAnonymous(NULL, XtraLife::MakeResultHandler(this, &MyGame::LoginHandler));
}

void MyGame::LoginHandler(XtraLife::eErrorCode aErrorCode, const XtraLife::CCloudResult *aResult)
{
    //  If anonymous account creation went well, then we can dive in the data returned.
    if(aErrorCode == XtraLife::eErrorCode::enNoErr)
    {
        //  From the result, we get the embedded JSON, which could be displayed with a call to result->printFormatted();
        const XtraLife::Helpers::CHJSON* json = aResult->GetJSON();
        //  Inside this JSON, we have lots of data that you can browse, for example here we'll grab profile and credentials.
        std::string id = json->GetString("gamer_id");
        std::string secret = json->GetString("gamer_secret");
        const XtraLife::Helpers::CHJSON* profile = json->Get("profile");
        std::string name = profile->GetString("displayName");
        MyGame::Log("gamer id: %s", id.c_str());
        MyGame::Log("game secret: %s", secret.c_str());
    }
    else
        MyGame::Log("Could not login due to error: %d - %s", aErrorCode, aResult->GetErrorString());
}

template<typename... Args> void MyGame::Log(const char* fmt, Args... args)
{
    size_t size = snprintf(nullptr, 0, fmt, args...);
    std::string buf;
    buf.reserve(size + 1);
    buf.resize(size);
    snprintf(&buf[0], size + 1, fmt, args...);
    
    std::cout << buf << std::endl;
}
