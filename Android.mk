LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := Util
LOCAL_CFLAGS     := -Werror -fexceptions -frtti -DDYNAMIC_ES3
LOCAL_CFLAGS     += -march=armv6 -marm -mfloat-abi=softfp -mfpu=vfp
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../oiio/lib/OpenEXR
LOCAL_C_INCLUDES += ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.8/include
LOCAL_SRC_FILES  := \
                    Base64.cpp \
                    GlesUtil.cpp \
                    Json.cpp \
                    lodepng.cpp \
                    TaskMgr.cpp \
                    Thread.cpp \
                    Timer.cpp \
                    TouchUI.cpp \
                    TriStrip.cpp

LOCAL_EXPORT_C_INCLUDE += ${LOCAL_PATH}

include $(BUILD_STATIC_LIBRARY)
