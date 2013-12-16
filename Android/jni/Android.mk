LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := libUtil
LOCAL_CFLAGS     := -Werror -DDYNAMIC_ES3
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../..
LOCAL_C_INCLUDES += ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.8/include
LOCAL_SRC_FILES  := Sys.cpp \
				   gl3stub.c \
				   ../../Timer.cpp \
				   ../../GlesUtil.cpp \
				   ../../TouchUI.cpp
LOCAL_LDLIBS     := -llog -lGLESv2 -lEGL

include $(BUILD_SHARED_LIBRARY)
