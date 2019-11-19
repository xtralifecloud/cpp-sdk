//
//  AppDelegate.m
//  simpleapp-iOS
//
//  Created by Michael El Baki on 19/11/2019.
//  Copyright © 2019 XtraLife. All rights reserved.
//

#import "AppDelegate.h"

#import "MyGame.h"
#import "CClan.h"
#import <memory>

@interface AppDelegate ()

@end

@implementation AppDelegate

NSTimer* mTimer;
std::unique_ptr<MyGame> mGame;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    MyGame::Log("Hello XtraLife!");

    mGame.reset(new MyGame);
    //MyGame* game = new MyGame;
    mGame->Startup();

    mTimer = [NSTimer scheduledTimerWithTimeInterval:0.01 repeats:YES block:^(NSTimer * _Nonnull timer) {
        XtraLife::CClan::Instance()->ProcessIdleTasks();
    }];
    return YES;
}


#pragma mark - UISceneSession lifecycle


- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}


- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}


@end
