LOCAL_PATH := $(call my-dir)
PROJ_PATH :=$(LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_PATH := $(PROJ_PATH)

LOCAL_MODULE    := benchmark-activity
LOCAL_SRC_FILES := Activity.cpp Pipeline.cpp image.cpp debug.cpp
LOCAL_C_INCLUDES=$(POWER_MEASUREMENT_DIR)/include
LOCAL_CFLAGS := -D__ANDROID__ -D__CL_ENABLE_EXCEPTIONS $(LOCAL_CFLAGS)
LOCAL_CPPFLAGS : = $(LOCAL_CFLAGS) -std=c++11
LOCAL_LDLIBS    := -lm -llog -landroid
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := $(OPENCL_MODULE)
include $(BUILD_SHARED_LIBRARY)

$(call import-module,opencl)
$(call import-module,android/native_app_glue)

