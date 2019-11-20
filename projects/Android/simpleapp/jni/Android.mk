LOCAL_DIR           := $(call my-dir)
XTRALIFE_DIR        := $(LOCAL_DIR)/../../../..
REPOSITORIES_DIR    := $(XTRALIFE_DIR)/repositories

include $(CLEAR_VARS)
LOCAL_MODULE	:= xtralife
LOCAL_SRC_FILES	:= $(LOCAL_DIR)/../temp/libs/$(BUILD_TYPE)/$(TARGET_ARCH_ABI)/libxtralife.so

include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := simpleapp

LOCAL_CXXFLAGS		:= -std=c++11 -Wno-format-security
LOCAL_C_INCLUDES 	:= $(REPOSITORIES_DIR)/xtralife/include $(XTRALIFE_DIR)/sources/Samples/simpleapp
LOCAL_SRC_FILES		:= $(XTRALIFE_DIR)/sources/Samples/simpleapp/MyGame.cpp \
                       $(XTRALIFE_DIR)/sources/Samples/simpleapp/PlatformSpecific/Android/main.cpp

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_LDLIBS                        := -llog
LOCAL_SHARED_LIBRARIES              := xtralife

include $(BUILD_SHARED_LIBRARY)

