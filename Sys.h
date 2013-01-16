//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include <map>
#include <string>
#include <vector>


namespace tui { class Event; }

namespace sys {


// Derive a custom Callback class that implements the interface using OS-side
// code, e.g. Obj-C or Java, to provide system resources to the C++ code.

class Callbacks {
public:
  Callbacks() {}
  virtual ~Callbacks() {}
  
  // Returns a URL to a file within the app's resource bundle
  virtual bool FindResourcePath(const char *name, char path[1024]) = 0;
  
  // Returns a URL to a file in the app's local documents folder
  virtual bool FindAppCachePath(const char *name, char path[1024]) = 0;
  
  // Load an image from the app's resource bundle (blocking)
  virtual bool LoadImageResource(const char *name, size_t *w, size_t *h,
                                 size_t *channelCount,
                                 const unsigned char **texel) = 0;
  
  // Load a text file from the app's resource bundle (blocking)
  virtual bool LoadText(const char *name, const char **text) = 0;
  
  // Load a directory of albums (folders) and images (asynchronous)
  virtual bool LoadImageDirectory(const char *url) = 0;
  
  // Load only the metadata, not the pixels, from an image (asynchronous)
  virtual bool LoadImageMetadata(const char *url) = 0;
  
  // Load the full image of pixels into memory (asynchronous)
  virtual bool LoadImagePixels(const char *url) = 0;

  // Return the "scaling factor" for this display (poorly defined!)
  virtual float PixelScale() const = 0;
};


// Image metadata, broken into sections, each with a title and key-value map
struct MetadataSection {
  std::string title;
  std::map<std::string, std::string> keyValueMap;
};

struct Metadata {
  ~Metadata() { for (size_t i=0; i<sectionVec.size();++i) delete sectionVec[i];}
  void Copy(const Metadata &src) {
    for (size_t i = 0; i < src.sectionVec.size(); ++i) {
      MetadataSection *s = new MetadataSection;
      *s = *src.sectionVec[i];
      sectionVec.push_back(s);
    }
  }
  
  std::vector<const MetadataSection *> sectionVec;
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
  
  // Called asynchronously by Callbacks::LoadImageDirectory
  virtual bool AddImage(const char *album, const char *name, const char *url)=0;
  virtual bool AddAlbum(const char *name, const char *url, size_t w, size_t h,
                        const unsigned char *thumb) = 0;

  // Called asynchronously by Callbacks::LoadImageMetadata
  virtual bool SetImageThumbnail(const char *url, size_t w, size_t h,
                                 const unsigned char *thumb) = 0;
  
  // Called asynchronously by Callbacks::LoadImageMetadata
  virtual bool SetImageMetadata(const char *url, const Metadata &metadata) = 0;
  
  // Called asynchronously by Callbacks::LoadImagePixels
  virtual bool SetImagePixels(const char *url, size_t w, size_t h,
                              size_t channelCount, size_t bytesPerChannel,
                              const unsigned char *pixel) = 0;
  
protected:
  App() {}                                            // Derived class factory

private:
  App(const App &);                                   // Disallow copy
  void operator=(const App &);                        // Disallow assignment
};

  
};      // namespace sys

#endif  // SYS_H
