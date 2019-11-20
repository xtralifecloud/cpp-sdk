#include <jni.h>

#include <memory>

#include "MyGame.h"
#include "CClan.h"

std::unique_ptr<MyGame> game;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
extern "C" void Java_cloud_xtralife_simpleapp_MainActivity_XtraLifeSetupJNIBridge(JNIEnv* env, jobject thiz) {
    MyGame::Log("Hello XtraLife!");

    game.reset(new MyGame());
    game->Startup();
}

extern "C" void Java_cloud_xtralife_simpleapp_MainActivity_XtraLifeUpdateJNIBridge(JNIEnv* env, jobject thiz) {
    // Process this periodically
    if(game)
        XtraLife::CClan::Instance()->ProcessIdleTasks();
}