LOCAL_DIR           := $(call my-dir)
XTRALIFE_DIR        := $(LOCAL_DIR)/../../../..
REPOSITORIES_DIR    := $(XTRALIFE_DIR)/repositories

include $(CLEAR_VARS)

LOCAL_MODULE	:= curl
LOCAL_SRC_FILES	:= $(REPOSITORIES_DIR)/curl/libs/Android/$(TARGET_ARCH_ABI)/libcurl.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE	:= libcrypto
LOCAL_SRC_FILES	:= $(REPOSITORIES_DIR)/openssl/libs/Android/$(TARGET_ARCH_ABI)/libcrypto.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE	:= libssl
LOCAL_SRC_FILES	:= $(REPOSITORIES_DIR)/openssl/libs/Android/$(TARGET_ARCH_ABI)/libssl.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := xtralife

LOCAL_CXXFLAGS      := -std=c++11
LOCAL_C_INCLUDES 	:=  $(REPOSITORIES_DIR)/curl/include $(XTRALIFE_DIR)/sources
LOCAL_SRC_FILES		:= 	$(XTRALIFE_DIR)/sources/Core/CCallback.cpp				                            \
						$(XTRALIFE_DIR)/sources/Core/CClannishRESTproxy.cpp		                            \
						$(XTRALIFE_DIR)/sources/Core/CHjSON.cpp					                            \
						$(XTRALIFE_DIR)/sources/Core/ErrorStrings.cpp			                            \
						$(XTRALIFE_DIR)/sources/Core/XtraLife_thread.cpp				                    \
						$(XTRALIFE_DIR)/sources/Misc/XtraLifeHelpers.cpp				                    \
						$(XTRALIFE_DIR)/sources/PlatformSpecific/Android/XtraLifeJNI.cpp	                \
						$(XTRALIFE_DIR)/sources/PlatformSpecific/Android/JNIUtilities.cpp	                \
						$(XTRALIFE_DIR)/sources/PlatformSpecific/Android/CAndroidFilesystemHandlerImpl.cpp	\
						$(XTRALIFE_DIR)/sources/PlatformSpecific/Android/GooglePlayStoreHandler.cpp         \
						$(XTRALIFE_DIR)/sources/ExternalModules/cJSON/cJSON.c				                \
						$(XTRALIFE_DIR)/sources/API/CClan.cpp				                                \
						$(XTRALIFE_DIR)/sources/API/CFilesystemManager.cpp				                    \
						$(XTRALIFE_DIR)/sources/API/CGameManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/API/CTribeManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/API/CUserManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/API/CMatchManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/API/CStoreManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/API/CIndexManager.cpp				                        \
						$(XTRALIFE_DIR)/sources/Core/CStdioBasedFileImpl.cpp		                        \
						$(XTRALIFE_DIR)/sources/ExternalModules/Base64/base64.cpp				            \
						$(XTRALIFE_DIR)/sources/Misc/util.cpp				                                \
						$(XTRALIFE_DIR)/sources/Misc/helpers.cpp			                                \
						$(XTRALIFE_DIR)/sources/Misc/curltool.cpp

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_LDLIBS                        := -llog
LOCAL_STATIC_LIBRARIES              := curl ssl crypto

include $(BUILD_SHARED_LIBRARY)

