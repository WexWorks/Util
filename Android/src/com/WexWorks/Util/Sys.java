package com.WexWorks.Util;

// Wrapper for native library

public class Sys {

  static {
    System.loadLibrary("Util");
  }

  public static native void Init(long app);
  public static native void SetDeviceResolution(int w, int h);
  public static native void Step();
}
