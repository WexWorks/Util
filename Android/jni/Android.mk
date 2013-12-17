LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := libilmbase
LOCAL_CFLAGS     := -Werror -fexceptions -frtti
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Imath
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Iex
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Half
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/config
LOCAL_C_INCLUDES += ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.8/include
LOCAL_SRC_FILES  := ../../../ilmbase-2.1.0/Imath/ImathBox.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathColorAlgo.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathFun.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathMatrixAlgo.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathRandom.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathShear.cpp \
                    ../../../ilmbase-2.1.0/Imath/ImathVec.cpp \
                    ../../../ilmbase-2.1.0/Iex/IexBaseExc.cpp \
                    ../../../ilmbase-2.1.0/Iex/IexThrowErrnoExc.cpp \
                    ../../../ilmbase-2.1.0/Half/half.cpp

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
                    
LOCAL_MODULE     := libUtil
LOCAL_WHOLE_STATIC_LIBRARIES := libilmbase
LOCAL_CFLAGS     := -Werror -fexceptions -frtti -DDYNAMIC_ES3
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../..
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Imath
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Iex
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/Half
LOCAL_C_INCLUDES += ${LOCAL_PATH}/../../../ilmbase-2.1.0/config
LOCAL_C_INCLUDES += ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.8/include
LOCAL_SRC_FILES  := Sys.cpp \
				   gl3stub.c \
				   ../../GlesUtil.cpp \
				   ../../Timer.cpp \
				   ../../TouchUI.cpp \
				   ../../TriStrip.cpp
LOCAL_LDLIBS     := -llog -lGLESv2 -lEGL

include $(BUILD_SHARED_LIBRARY)
