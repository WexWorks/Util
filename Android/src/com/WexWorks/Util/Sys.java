package com.WexWorks.Util;

import android.R;
import android.R.integer;
import android.R.string;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;
import android.graphics.BitmapFactory;
import static android.opengl.GLES20.*;
import android.opengl.GLUtils;

import java.net.URI;
import java.nio.IntBuffer;


public class Sys {

  static {
    System.loadLibrary("Util");
  }

  static Context sContext;
  
  //
  // OS -> C++ calls
  //
  
  public void Init(Context context, long app) {
    sContext = context;
    Init(app);
  }
  public native void Init(long app);
  public native void SetDeviceResolution(int w, int h);
  public native void Step();

  //
  // C++ -> OS Calls
  //
  
  public class CreateTexture {
    int id=0, w=0, h=0;
    CreateTexture(int id, int w, int h) { this.id = id; this.w = w; this.h = h; }
  };
  
  public CreateTexture CreateResourceGLTexture(String name) {
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
    GLUtils.texImage2D(GL_TEXTURE_2D, 0, bitmap, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return new CreateTexture(tid[0], bitmap.getWidth(), bitmap.getHeight());
  }
  
}
