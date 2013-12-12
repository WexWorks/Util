package com.WexWorks.Util;

import android.R;
import android.R.integer;
import android.R.string;
import android.content.Context;
import android.net.Uri;
import android.util.Log;

import java.net.URI;


public class Sys {

  static {
    System.loadLibrary("Util");
  }

  static Context sContext;
  
  public void Init(Context context, long app) {
    sContext = context;
    Init(app);
  }
  public native void Init(long app);
  public native void SetDeviceResolution(int w, int h);
  public native void Step();
  
  public String FindResourcePath(String name) {
    String pkgName = sContext.getApplicationInfo().packageName;
    int id = sContext.getResources().getIdentifier("Icon.png", "drawable", pkgName);
    String p = "android.resource://" + pkgName + "/drawable/Icon.png";
    Log.i("WexWorks", p);
    Uri uri = Uri.parse(p);
    if (uri == null)
      return "Unknown";
    return uri.getPath();
  }
  
}
