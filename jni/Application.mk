#-------------------------------------------------------------------------------

# Build Paths (EDIT THESE)

RCCB_ROOT    := $(call my-dir)/..

#-------------------------------------------------------------------------------

# Build Configuration

TEST_NAME_PREFIX  := rccb
TEST_USE_MPI      := yes

INCLUDES          := -I$(RCCB_ROOT)/src 
DEFINES           := -DANDROID -DRCCB_PLUGIN
COMPILER_WARNINGS := -Wall
COMPILER_FLAGS    := -O3 $(COMPILER_WARNINGS) 
LIBS_STATIC       :=
LIBS_SHARED       := 

APP_PROJECT_PATH  := $(RCCB_ROOT)
APP_BUILD_SCRIPT  := $(APP_PROJECT_PATH)/Android.mk

APP_OPTIM         := release
APP_CFLAGS        := $(INCLUDES) $(DEFINES) $(COMPILER_FLAGS) -fexceptions -std=c++11
APP_CPPFLAGS      := $(APP_CFLAGS)

APP_ABI           := armeabi-v7a
APP_PLATFORM      := android-18

# gnustl_static or stlport_static or nothing
#APP_STL           := stlport_static
APP_STL = gnustl_static

# NDK toolchain, 4.6.3, 4.7 (or clang3.1 but ARM optimizations don't work?)
NDK_TOOLCHAIN_VERSION := 4.8

# Enable C++0x
#APP_USE_CPP0X := true

PLATFORM := 8974

ifeq ($(PLATFORM), arndale)
	PLATFORM_FLAGS := -D__MALI__
	OPENCL_MODULE := OpenCL_nexus10
endif
ifeq ($(PLATFORM), nexus10)
	PLATFORM_FLAGS := -D__MALI__
	OPENCL_MODULE := OpenCL_nexus10
	APP_STL := stlport_static
endif
ifeq ($(PLATFORM), 8974)
	PLATFORM_FLAGS := -D__ADRENO__
	OPENCL_MODULE := OpenCL_8974
endif

