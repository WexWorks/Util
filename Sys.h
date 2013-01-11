//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include <stdlib.h>


namespace tui { class Event; }

namespace sys {


// Derive a custom Callback class that implements the interface using OS-side
// code, e.g. Obj-C or Java, to provide system resources to the C++ code.

class Callbacks {
public:
  Callbacks() {}
  virtual ~Callbacks() {}
  
  virtual bool FindResourcePath(const char *name, char path[1024]) = 0;
  virtual bool FindAppCachePath(const char *name, char path[1024]) = 0;
  virtual bool LoadImageResource(const char *name, size_t *w, size_t *h,
                                 size_t *channelCount,
                                 const unsigned char **texel) = 0;
  virtual bool LoadText(const char *name, const char **text) = 0;
  virtual bool LoadImageDirectory(const char *name) = 0;
  virtual bool LoadImage(const char *name, void *data) = 0;

  virtual float PixelScale() const = 0;
};


// Implement this class to provide a standard framework invoked from OS-side
// code to pass system events from the OS down to C++ code.

class App {
public:
  virtual ~App() {}
  
  static App *Create();                               // Factory
  
  virtual bool Init(Callbacks *callbacks) = 0;        // Setup app
  virtual bool Touch(const tui::Event &event) = 0;    // Process events
  virtual bool Step(float seconds) = 0;               // Animate widgets
  virtual bool Dormant() = 0;                         // True = no refresh
  virtual bool Draw() = 0;                            // Draw UI & images

  // Called at startup and whenever the device orientation changes
  virtual bool SetDeviceResolution(int w, int h) = 0;
  
  // Called by Callbacks::LoadImageDirectory
  virtual bool AddImage(const char *album, const char *name, const char *url,
                        size_t w, size_t h, const unsigned char *thumb) = 0;
  virtual bool AddAlbum(const char *name, const char *url, size_t w, size_t h,
                        const unsigned char *thumb) = 0;

  // Called by Callbacks::LoadImage
  virtual bool LoadImage(const char *name, void *data, size_t w, size_t h,
                         size_t channelCount, size_t bytePerChannelCount,
                         const unsigned char *pixel) = 0;
  
protected:
  App() {}                                            // Derived class factory

private:
  App(const App &);                                   // Disallow copy
  void operator=(const App &);                        // Disallow assignment
};

  
};      // namespace sys

#endif  // SYS_H
