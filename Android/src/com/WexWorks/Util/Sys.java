package com.WexWorks.Util;

import android.content.Context;
import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import static android.opengl.GLES20.*;
import android.opengl.GLUtils;


public class Sys {

  static {
    System.loadLibrary("Util");
  }

  private static Context sContext;
  
  //
  // OS -> C++ calls
  //
  
  public static void Init(Context context, long app) {
    sContext = context;
    Init(app);
  }
  
  public static native void Init(long app);
  public static native void SetDeviceResolution(int w, int h);
  public static native void Step();

  //
  // C++ -> OS Calls
  //
  
  public static class GLTexture {
    int id=0, w=0, h=0;
    GLTexture(int id, int w, int h) { this.id = id; this.w = w; this.h = h; }
  };
  
  public static GLTexture CreateResourceGLTexture(String name,
      int minFilter, int magFilter, int wrapS, int wrapT) {
    String pkgName = sContext.getApplicationInfo().packageName;
    int resId = sContext.getResources().getIdentifier(name, "drawable", pkgName);
    BitmapFactory.Options opt = new BitmapFactory.Options();
    opt.inScaled = false;
    Bitmap bitmap = BitmapFactory.decodeResource(sContext.getResources(), resId, opt);
    if (bitmap == null) {
      Log.e("WexWorks.Util.Sys", "Cannot find drawable " + name);
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
    return new GLTexture(tid[0], bitmap.getWidth(), bitmap.getHeight());
  }
  
}
