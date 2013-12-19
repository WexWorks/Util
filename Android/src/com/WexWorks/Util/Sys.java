package com.WexWorks.Util;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import static android.opengl.GLES20.*;
import android.net.Uri;
import android.opengl.GLUtils;
import android.util.Log;



public class Sys {

  static {
    System.loadLibrary("Util");
  }

  public static int PICK_IMAGE_INTENT = 1;
  private static Activity sActivity;

  public static void Init(Activity activity, long app) {
    sActivity = activity;
    Init(app);
  }
  
  //
  // OS -> C++ calls
  //
  
  public static native void Init(long app);
  public static native void SetDeviceResolution(int w, int h);
  public static native void Step();
  public static native void Touch(int phase, float timestamp,
      int count, float xy[], int id[]);
  public static native void SetPickedImage(String path);
  
  
  //
  // C++ -> OS Calls from Sys.cpp
  //
  
  public static class GLTexture {
    int id=0, w=0, h=0;
    GLTexture(int id, int w, int h) { this.id = id; this.w = w; this.h = h; }
  };
  
  public static GLTexture CreateGLTexture(String name,
      int minFilter, int magFilter, int wrapS, int wrapT) {
    Bitmap bitmap = null;
    if (name.charAt(0) == '/') {
      bitmap = BitmapFactory.decodeFile(name);
    } else {
      BitmapFactory.Options opt = new BitmapFactory.Options();
      opt.inScaled = false;
      String pkgName = sActivity.getApplicationInfo().packageName;
      int resId = sActivity.getResources().getIdentifier(name, "drawable", pkgName);
      bitmap = BitmapFactory.decodeResource(sActivity.getResources(), resId, opt);
    }
    if (bitmap == null) {
      Log.e("WexWorks.Util.Sys", "Cannot find texture image " + name);
      return null;
    }
    final int[] tid = new int[1];
    glGenTextures(1, tid, 0);
    if (tid[0] == 0) {
      Log.e("WexWorks.Util.Sys", "Cannot generate new texture id");
      return null;
    }
    glBindTexture(GL_TEXTURE_2D, tid[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLUtils.texImage2D(GL_TEXTURE_2D, 0/*level*/, bitmap, 0/*border*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (glGetError() != GL_NO_ERROR) {
      Log.e("WexWorks.Util.Sys", "GL Error creating texture");
      return null;
    }
    
    Log.i("WexWorks.Util.Sys", "Created texture " + name + " " + tid[0] + " " +
        bitmap.getWidth() + " x " + bitmap.getHeight());
    
    return new GLTexture(tid[0], bitmap.getWidth(), bitmap.getHeight());
  }
  
  public static void PickImage() {
    Intent intent = new Intent(Intent.ACTION_PICK,
        android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
    intent.setType("image/*");
    sActivity.startActivityForResult(intent, PICK_IMAGE_INTENT);
  }
}
