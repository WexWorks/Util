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

  AndroidOs() : mEnv(NULL), mSysClazz(NULL), mCreateResourceGLTextureMid(NULL) {}

  bool Init(JNIEnv *env, jobject sysObj) {
    mEnv = env;
    mSysClazz = mEnv->FindClass("com.WexWorks.Util.Sys");
    mCreateResourceGLTextureMid = NULL;
    return true;
  }

  // OS-specific error logging
  void Info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO,"WexWorks.Util.Sys", fmt, ap);
    va_end(ap);
  }

  void Warning(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_WARN,"WexWorks.Util.Sys", fmt, ap);
    va_end(ap);
  }

  void Error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR,"WexWorks.Util.Sys", fmt, ap);
    va_end(ap);
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

  bool PickImage(sys::PickImage *pickImage) {
    return true;
  }

  bool ComputeHistogram(const char *url, size_t w, size_t h,
                        const unsigned char *rgba,
                        size_t hCount, unsigned long *histogram) {
    return true;
  }

  float PixelScale() const {
    return 2;
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

  void ForceRedraw() {

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

  bool CreateGLTexture(const char *name, int minFilter, int magFilter,
                       int wrapS, int wrapT, unsigned int *id,
                       size_t *w, size_t *h) {
    if (!mCreateResourceGLTextureMid) {
      const char *fname = "CreateResourceGLTexture";
      const char *sig = "(Ljava/lang/String;IIII)Lcom/WexWorks/Util/Sys$GLTexture;";
      mCreateResourceGLTextureMid = mEnv->GetStaticMethodID(mSysClazz, fname, sig);
      Info("Found CreateResourceGLTexture %d\n", mCreateResourceGLTextureMid);
      if (!mCreateResourceGLTextureMid)
        return false;
    }

    Info("Calling CreateResourceGLTexture %d\n", mCreateResourceGLTextureMid);
    jstring n = mEnv->NewStringUTF(name);
    jobject o = mEnv->CallStaticObjectMethod(mSysClazz, mCreateResourceGLTextureMid,
                                             n, minFilter, magFilter, wrapS, wrapT);

    Info("Called CreateResourceGLTexture returned %p\n", o);
    if (o) {
      jclass clazz = mEnv->GetObjectClass(o);
      jfieldID fid = mEnv->GetFieldID(clazz, "id", "I");
      *id = mEnv->GetIntField(o, fid);
      fid = mEnv->GetFieldID(clazz, "w", "I");
      *w = mEnv->GetIntField(o, fid);
      fid = mEnv->GetFieldID(clazz, "h", "I");
      *h = mEnv->GetIntField(o, fid);
    } else {
      *id = *w = *h = 0;
    }

    mEnv->DeleteLocalRef(n);
    mEnv->DeleteLocalRef(o);

    return true;
  }

private:
  JNIEnv *mEnv;
  jclass mSysClazz;
  jmethodID mCreateResourceGLTextureMid;

};  // class AndroidOs


//
// JNI Methods
//

static sys::App *gApp = NULL;
static AndroidOs *gOs = NULL;

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
  gOs = new AndroidOs;
  if (!gOs->Init(env, obj))
    return;
  if (!gApp->Init(gOs)) {
    gOs->Error("Cannot initialize application");
    return;
  }
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_SetDeviceResolution(JNIEnv* env, jobject obj,
                                               jint w, jint h) {
  if (!gApp) {
    gOs->Error("Invalid app in resize");
    return;
  }
  if (!gApp->SetDeviceResolution(w, h)) {
    gOs->Error("Error setting device resolution %d x %d", w, h);
  }
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Step(JNIEnv* env, jobject obj) {
  if (!gApp) {
    gOs->Error("Invalid app in step");
    return;
  }

  static Timer sTimer;
  static float sLastFrameSec = 0;
  float sec = sTimer.Elapsed();
  float dSec = sec - sLastFrameSec;
  sLastFrameSec = sec;

  if (!gApp->Step(dSec)) {
    gOs->Error("Error stepping %f seconds", dSec);
    return;
  }

  if (!gApp->Draw()) {
    gOs->Error("Error redrawing");
    return;
  }
}

