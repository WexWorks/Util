//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include <map>
#include <string>
#include <vector>


namespace tui { class Event; }

namespace sys {

// Image metadata, broken into sections, each with a title and key-value map
typedef std::map<std::string, std::string> MetadataMap;

struct MetadataSection {
  std::string title;
  MetadataMap keyValueMap;
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


// Function callback types
typedef bool (*AddImageCB)(void *cbData, const char *album, const char *name,
                           const char *url);
typedef bool (*AddAlbumCB)(void *cbData, const char *name, const char *url,
                           size_t w, size_t h, const unsigned char *thumb);
typedef bool (*SetImageThumbnailCB)(void *cbData, const char *url, size_t w,
                                    size_t h, const unsigned char *thumb);
typedef bool (*SetImageMetadataCB)(void *cbData, const char *url,
                                   const Metadata &metadata);
typedef bool (*SetImagePixelsCB)(void *cbData,const char *url,size_t w,size_t h,
                                 size_t channelCount, size_t bytesPerChannel,
                                 const unsigned char *pixel);
typedef void (*SetAlertTextCB)(void *cbData, const char *inputText);


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
  
  // Load all image folders in the system
  virtual bool LoadSystemFolders(void *cbData, AddAlbumCB addAlbumCB) = 0;
  
  // Load a folder of images
  virtual bool LoadFolderImages(const char *url, void *cbData,
                                AddImageCB addImageCB) = 0;
  
  // Load only the metadata, not the pixels, from an image
  virtual bool LoadImageMetadata(const char *url, bool getThumbnail,
                                 bool getMetadata, void *cbData,
                                 SetImageThumbnailCB setThumbCB,
                                 SetImageMetadataCB setMetadataCB) = 0;
  
  // Load the full image of pixels into memory
  virtual bool LoadImagePixels(const char *url, void *cbData,
                               SetImagePixelsCB setPixelsCB) = 0;

  // Return the "scaling factor" for this display (poorly defined!)
  virtual float PixelScale() const = 0;
  
  // Display an alert box with optional text input and button names
  virtual void AlertBox(const char *title, const char *msg, const char *ok,
                        const char *cancel, bool secure, void *cbData,
                        SetAlertTextCB setTextCB) = 0;
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
  virtual bool SetDeviceResolution(int w, int h) = 0; // Startup & orientation
  
protected:
  App() {}                                            // Derived class factory

private:
  App(const App &);                                   // Disallow copy
  void operator=(const App &);                        // Disallow assignment
};

  
};      // namespace sys

#endif  // SYS_H
