LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := codm_skin
LOCAL_SRC_FILES := main.cpp
LOCAL_LDLIBS := -llog -ldl
LOCAL_CFLAGS := -std=c++17 -fno-rtti -fvisibility=hidden -O2
LOCAL_CPPFLAGS := -std=c++17
include $(BUILD_SHARED_LIBRARY)
