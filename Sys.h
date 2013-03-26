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


// Callback functors, executed asynchronously, e.g. from system blocks

struct AddImage {
  virtual bool operator()(const char *album, const char *name,
                          const char *url) = 0;
};

struct AddAlbum {
  virtual bool operator()(const char *name, const char *url,
                          size_t assetCount) = 0;
};

struct SetImageThumbnail {
  virtual bool operator()(const char *url, size_t w, size_t h,
                          const unsigned char *thumb) = 0;
};

struct SetImageMetadata {
  virtual bool operator()(const char *url, const Metadata &metadata) = 0;
};

struct SetImagePixels {
  virtual bool operator()(const char *url, size_t w, size_t h,
                          size_t channelCount, size_t bytesPerChannel,
                          const unsigned char *pixel,
                          size_t histogramCount, unsigned long *histogram) = 0;
};

struct SetAlertText {
  virtual bool operator()(const char *inputText) = 0;
};
  
struct RenderImageToFile {
  // Passing w != 0 && h == 0 uses w as a max dimension.
  // Passing w == h == 0 uses source image resolution.
  virtual bool operator()(const char *url, const char *file, size_t w, size_t h,
                          size_t channelCount, size_t bytesPerChannel) = 0;
};


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
  
  // Load all image albums in the system
  virtual bool LoadSystemAlbums(AddAlbum *addAlbum) = 0;
  
  // Load a all the image names in an album
  virtual bool LoadAlbumImageNames(const char *url, AddImage *addImage) = 0;
  
  // Load the thumbnail representation of the image
  virtual bool LoadImageThumbnail(const char *url,
                                  SetImageThumbnail *setThumb) = 0;
  
  // Load only the metadata, not the pixels, from an image
  virtual bool LoadImageMetadata(const char *url,
                                 SetImageMetadata *setMetadata) = 0;
  
  // Load the full image of pixels into memory
  virtual bool LoadImagePixels(const char *url, size_t histogramCount,
                               unsigned long *histogram,
                               SetImagePixels *setPixels) = 0;

  // Return the "scaling factor" for this display (poorly defined!)
  virtual float PixelScale() const = 0;
  
  // Display an alert box with optional text input and button names
  virtual void AlertBox(const char *title, const char *msg, const char *ok,
                        const char *cancel,bool secure,SetAlertText *setText)=0;
  
  // Share the images in the file list to the named service
  virtual bool ShareImageFiles(const char *service, const int widgetRect[4],
                               const std::vector<std::string> &url,
                               RenderImageToFile *render) = 0;
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
