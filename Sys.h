//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include <stdlib.h>


namespace tui { class Event; }

namespace sys {


class Callbacks {
public:
  Callbacks() {}
  virtual ~Callbacks() {}
  
  // Implement this interface using OS-side code (Obj-C, Java, ...)
  virtual bool FindResourcePath(const char *name, char path[1024]) = 0;
  virtual bool FindAppCachePath(const char *name, char path[1024]) = 0;
  virtual bool LoadImageResource(const char *name, size_t *w, size_t *h,
                                 size_t *channelCount,
                                 const unsigned char **texel) = 0;
  virtual bool LoadText(const char *name, const char **text) = 0;
  virtual bool OpenImagePicker(const int viewport[4]) = 0;
};

  
class App {
public:
  App() {}
  virtual ~App() {}
  
  virtual bool Init(Callbacks &callbacks) = 0;        // Initialization
  virtual bool Touch(const tui::Event &event) = 0;    // Process events
  virtual bool Step(float seconds) = 0;               // Animate widgets
  virtual bool Dormant() = 0;                         // True = no refresh
  virtual bool Draw() = 0;                            // Draw UI & images

  // Called at startup and whenever the device orientation changes
  virtual bool SetDeviceResolution(int w, int h, float pixelsPerCm) = 0;
  
  // Called by OpenImagePicker
  virtual bool LoadImage(const char *name, size_t w, size_t h,
                         size_t channelCount, size_t bytePerChannelCount,
                         const unsigned char *pixel) = 0;
};

  
};      // namespace sys

#endif  // SYS_H
