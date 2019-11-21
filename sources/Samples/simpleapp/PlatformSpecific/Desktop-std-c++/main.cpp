//
//  main.cpp
//  simpleapp
//
//  Created by Michael El Baki on 23/10/2019.
//  Copyright © 2019 XtraLife. All rights reserved.
//

#include <chrono>
#include <thread>

#include "CClan.h"
#include "MyGame.h"

int main(int argc, const char * argv[]) {
    MyGame::Log("Hello XtraLife!");
    
    std::unique_ptr<MyGame> game(new MyGame);
    game->Startup();
    
    while(game->IsRunning())
    {
        XtraLife::CClan::Instance()->ProcessIdleTasks();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
