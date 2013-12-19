// Copyright (c) 2013 WexWorks. All rights reserved

#include "GlesUtil.h"
#include "Sys.h"
#include "Timer.h"
#include "TouchUI.h"

#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <cstdarg>


//
// Os
//

bool sys::Os::CreateGLTextures(size_t count, const char **file,
                               size_t *w, size_t *h, unsigned int tex[]) {
#if REPLACE_ALL_REZ_TEX
  static GLuint sTex = 0;
  static const int sW = 2, sH = 2;
  if (!sTex) {
    glGenTextures(1, &sTex);
    const unsigned char pixel[sW * sH] = { 0, 255, 255, 0 };
    if (!glt::StoreTexture(sTex, GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR,
                           GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, sW, sH,
                           GL_LUMINANCE, GL_UNSIGNED_BYTE, &pixel[0],
                           "Standin"))
      return false;
  }
  for (size_t i = 0; i < count; ++i) {
    tex[i] = sTex;
    dim[0] = sW;
    dim[1] = sH;
  }
#else
  size_t dim[2] = { 0 };
  for (size_t i = 0; i < count; ++i) {
    if (!CreateGLTexture(file[i], GL_LINEAR, GL_LINEAR,
                         GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                         &tex[i], w, h))
      return false;
    if (w == 0 || h == 0)
      return false;
    if (dim[0] == 0) {
      dim[0] = *w;
      dim[1] = *h;
    } else if (dim[0] != *w || dim[1] != *h) {
      return false;
    }
  }
#endif
  return true;
}


//
// AndroidOs
//

class AndroidOs : public sys::Os {
public:

  AndroidOs() : mEnv(NULL), mSysClazz(NULL), mCreateGLTextureMid(NULL),
                mPickImageMid(NULL), mPickImage(NULL) {}

  bool Init(JNIEnv *env, jobject sysObj) {
    mEnv = env;
    jclass clazz = mEnv->FindClass("com.WexWorks.Util.Sys");
    if (!clazz)
      return false;
    mSysClazz = reinterpret_cast<jclass>(mEnv->NewGlobalRef(clazz)); // keep!
    mCreateGLTextureMid = NULL;
    mPickImageMid = NULL;
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
    if (!mPickImageMid) {
      const char *fname = "PickImage";
      const char *sig = "()V";
      mPickImageMid = mEnv->GetStaticMethodID(mSysClazz, fname, sig);
      if (!mPickImageMid)
        return false;
    }
    if (mPickImage)
      return false;
    mPickImage = pickImage;     // Store for FSM
    jobject o = mEnv->CallStaticObjectMethod(mSysClazz, mPickImageMid);
    return true;
  }

  bool PickedImage(const char *url) {
    if (!mPickImage)            // Only allow one outstanding pick via FSM
      return false;
    if (!(*mPickImage)(url))
      return false;
    mPickImage = NULL;          // Clear for FSM
    return true;
  }

  bool ComputeHistogram(const char *url, size_t w, size_t h,
                        const unsigned char *rgba,
                        size_t hCount, unsigned long *histogram) {
    return true;
  }

  float PixelScale() const {
    return 2.5;
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
    if (!mCreateGLTextureMid) {
      const char *fname = "CreateGLTexture";
      const char *sig = "(Ljava/lang/String;IIII)Lcom/WexWorks/Util/Sys$GLTexture;";
      mCreateGLTextureMid = mEnv->GetStaticMethodID(mSysClazz, fname, sig);
      if (!mCreateGLTextureMid)
        return false;
    }
    jstring n = mEnv->NewStringUTF(name);
    jobject o = mEnv->CallStaticObjectMethod(mSysClazz, mCreateGLTextureMid,
                                             n, minFilter, magFilter, wrapS, wrapT);
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
  jmethodID mCreateGLTextureMid;
  jmethodID mPickImageMid;
  sys::PickImage *mPickImage;

};  // class AndroidOs


//
// JNI Methods
//

static sys::App *gApp = NULL;
static AndroidOs *gOs = NULL;
static int gAppHeight = 0;

extern "C" {
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_Init(JNIEnv *env, jobject obj, jlong app);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_SetDeviceResolution(JNIEnv *env, jobject obj, jint w, jint h);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_Step(JNIEnv *env, jobject obj);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_Touch(JNIEnv * env, jobject obj, jint phase, jfloat timestamp, jint count, jfloatArray xy, jintArray id);
JNIEXPORT void JNICALL Java_com_WexWorks_Util_Sys_SetPickedImage(JNIEnv *env, jobject obj, jstring path);
};


#if !defined(DYNAMIC_ES3)
static GLboolean gl3stubInit() {
  return GL_TRUE;
}
#endif


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Init(JNIEnv* env, jobject obj, jlong app) {
  if (gApp) {
    // Called by onResume where we should re-create all textures...
    // FIXME: Currently hoping context is preserved...
//    delete gApp;
//    gApp = NULL;
    return;
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
  if (!gApp)
    return;
  if (!gApp->SetDeviceResolution(w, h))
    gOs->Error("Error setting device resolution %d x %d", w, h);
  gAppHeight = h;
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Step(JNIEnv* env, jobject obj) {
  if (!gApp)
    return;

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


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_Touch(JNIEnv *env, jobject obj, jint phase,
                                 jfloat timestamp, jint count, jfloatArray xy,
                                 jintArray id) {
  if (!gApp)
    return;

  jfloat *xydata = env->GetFloatArrayElements(xy, 0);
  jint *iddata = env->GetIntArrayElements(id, 0);

  static tui::Event tevent(tui::TOUCH_CANCELLED);

  tevent.Init(tui::EventPhase(phase));
  for (size_t i = 0; i < count; ++i) {
    const float x = xydata[i*2+0];
    const float y = gAppHeight - 1 - xydata[i*2+1];
    int j = iddata[i];
//    static const char *phaseName[] = { "Begin", "Move", "End", "Cancel" };
//    gOs->Info("Touch %s (%d) at %f x %f @ %f", phaseName[phase], j, x, y, timestamp);
    tevent.AddTouch(iddata[i], x, y, timestamp);
  }
  tevent.PrepareToSend();

  gApp->Touch(tevent);                              // Ignore return value

  env->ReleaseFloatArrayElements(xy, xydata, 0);
  env->ReleaseIntArrayElements(id, iddata, 0);
}


JNIEXPORT void JNICALL
Java_com_WexWorks_Util_Sys_SetPickedImage(JNIEnv *env, jobject obj, jstring path) {
  if (!gApp)
    return;
  const char *p = path ? env->GetStringUTFChars(path, 0) : "<none>";
  gOs->PickedImage(p);
  if (path)
    env->ReleaseStringUTFChars(path, p);
}

