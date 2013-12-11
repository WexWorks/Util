// Copyright (c) 2013 WexWorks. All rights reserved

#include "Sys.h"
#include "Timer.h"

#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <cstdarg>


//
// AndroidOs
//

class AndroidOs : public sys::Os {
public:

  bool FindResourcePath(const char *name, char path[1024]) {
    return true;
  }

  bool FindAppCachePath(const char *name, char path[1024]) {
    return true;
  }

  bool FindUserDefault(const char *name, char value[1024]) {
    return true;
  }

  bool LoadText(const char *name, const char **text) {
    return true;
  }

  bool LoadSystemAlbums(sys::AddAlbum *addAlbum) {
    return true;
  }

  bool LoadAlbum(const char *url, sys::AddAlbum *addAlbum) {
    return true;
  }

  bool LoadAlbumImageNames(const char *url, int firstIdx, int lastIdx,
                           sys::AddImage *addImage) {
    return true;
  }

  bool LoadImageDate(const char *url, sys::SetImageDate *setDate) {
    return true;
  }

  bool LoadImageThumbnail(const char *url, size_t maxDim,
                          sys::SetThumbnail *setThumb) {
    return true;
  }

  bool LoadImageMetadata(const char *url,
                         sys::SetImageMetadata *setMetadata) {
    return true;
  }

  bool CacheImage(const char *url, const char *cachePath,
                  sys::SetImageCache *setCache) {
    return true;
  }

  bool ComputeHistogram(const char *url, size_t w, size_t h,
                        const unsigned char *rgba,
                        size_t hCount, unsigned long *histogram) {
    return true;
  }

  float PixelScale() const {
    return 1;
  }

  void AlertBox(const char *title, const char *msg, const char *ok,
                const char *cancel, bool hasTextInput,bool isTextSecure,
                sys::SetAlert *setAlert) {}

  bool GetShareOptions(const char *service, const int fromRect[4],
                       sys::SetShareOptions *setOptions) {
    return true;
  }

  bool ShareImageFiles(const char *service, const int fromRect[4],
                       const std::vector<const sys::ShareImage *> &image) {
    return true;
  }

  bool ShareImage(const char *service, const sys::ShareImage &image) {
    return true;
  }

  bool ShowStore(const char *product, const int fromRect[4]) {
    return true;
  }

  bool ShowVideo(const char *url, size_t w, size_t h) {
    return true;
  }

  bool CreateGLContext(int id, int shareId = -1) {
    return true;
  }

  bool SetGLContext(int id) {
    return true;
  }

  bool DeleteGLContext(int id) {
    return true;
  }

  int CurrentGLContext() {
    return 0;
  }

  void ForceRedraw() {

  }


  // OS-specific error logging
  static void Error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR,"WexWorks.Util.Sys", fmt, ap);
    va_end(ap);
  }
};  // class AndroidOs


//
// JNI Methods
//

static sys::App *gApp = NULL;


extern "C" {
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_Init(JNIEnv *env, jobject obj, jlong app);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_SetDeviceResolution(JNIEnv *env, jobject obj, jint w, jint h);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_Step(JNIEnv *env, jobject obj);
};


#if !defined(DYNAMIC_ES3)
static GLboolean gl3stubInit() {
  return GL_TRUE;
}
#endif


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Init(JNIEnv* env, jobject obj, jlong app) {
  if (gApp) {
    delete gApp;
    gApp = NULL;
  }
  gApp = (sys::App *)app;
  sys::Os *os = new AndroidOs;
  if (!gApp->Init(os)) {
    AndroidOs::Error("Cannot initialize application");
    return;
  }
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_SetDeviceResolution(JNIEnv* env, jobject obj,
                                               jint w, jint h) {
  if (!gApp) {
    AndroidOs::Error("Invalid app in resize");
    return;
  }
  if (!gApp->SetDeviceResolution(w, h)) {
    AndroidOs::Error("Error setting device resolution %d x %d", w, h);
  }
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Step(JNIEnv* env, jobject obj) {
  if (!gApp) {
    AndroidOs::Error("Invalid app in step");
    return;
  }

  static Timer sTimer;
  static float sLastFrameSec = 0;
  float sec = sTimer.Elapsed();
  float dSec = sec - sLastFrameSec;
  sLastFrameSec = sec;

  if (!gApp->Step(dSec)) {
    AndroidOs::Error("Error stepping %f seconds", dSec);
    return;
  }

  if (!gApp->Draw()) {
    AndroidOs::Error("Error redrawing");
    return;
  }
}

