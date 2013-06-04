//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include "paramlist.h"

#include <map>
#include <string>
#include <vector>

namespace tui { class Event; }

namespace sys {

// Save the image data in file with the metadata copied from url but with
// the fields below modified (i.e. replace keywords in original url metadata)
struct ShareImage {
  ShareImage(const char *url,const char *file,const char *name,
             const char *album, const char *albumURL,
             int w, int h, const std::vector<std::string> &keywords,
             bool isFlagged, bool stripLocationInfo, bool stripCameraInfo,
             int orientation, int starRating, const char *author,
             const char *copyright, const char *comment)
  : url(url), file(file), name(name), album(album), albumURL(albumURL),
    width(w), height(h), keywords(keywords), isFlagged(isFlagged),
    stripLocationInfo(stripLocationInfo), stripCameraInfo(stripCameraInfo),
    orientation(orientation), starRating(starRating), author(author),
    copyright(copyright), comment(comment) {}
  
  virtual void Done(bool status, const char *msg) const {}
  
  std::string url;                                    // Original metadata
  std::string file;                                   // New pixel data
  std::string name;                                   // User-visible image name
  std::string album;                                  // User-visible album name
  std::string albumURL;                               // URL for album
  int width, height;                                  // New image size
  std::vector<std::string> keywords;                  // New kewords
  bool isFlagged, stripLocationInfo, stripCameraInfo; // New bool metadata
  int orientation, starRating;                        // New int metadata
  std::string author, copyright, comment;             // New string metadata
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

struct SetImageDate {
  virtual bool operator()(const char *url, double epochSec) = 0;
};

struct SetImageMetadata {
  virtual bool operator()(const char *url, const OIIO::ParamValueList &meta) =0;
};

struct SetImagePixels {
  virtual bool operator()(const char *url, size_t w, size_t h,
                          size_t channelCount, size_t bytesPerChannel,
                          const unsigned char *pixel,
                          size_t histogramCount, unsigned long *histogram) = 0;
};

struct SetAlert {
  virtual bool operator()(bool isOK, const char *inputText) = 0;
};

struct SetShareOptions {
  enum ResMode { Source, Fixed, Percent };
  enum Format { JPEG, TIFF, PNG };
  virtual ~SetShareOptions() {}
  virtual bool operator()(const char *service, const char *filenameTemplate,
                          const char *comment, ResMode resMode, float dim,
                          const char *shot, const char *album, Format format,
                          float quality, const char *author,
                          const char *copyright, bool setKeywords,bool stripAll,
                          bool stripLocation, bool stripCamera) = 0;
};

  
// Derive a custom Callback class that implements the interface using OS-side
// code, e.g. Obj-C or Java, to provide system resources to the C++ code.
// Most of these functions may be asynchronous, providing a callback which
// is invoked once the operation is completed on the OS-side.

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
  
  // Load the named album
  virtual bool LoadAlbum(const char *url, AddAlbum *addAlbum) = 0;
  
  // Load a all the image names in an album
  virtual bool LoadAlbumImageNames(const char *url, AddImage *addImage) = 0;

  // Return the last modification time of the image
  virtual bool LoadImageDate(const char *url, SetImageDate *setDate) = 0;
  
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
                        const char *cancel, bool hasTextInput,bool isTextSecure,
                        SetAlert *setAlert) = 0;
  
  // Open a sharing option dialog and return the options via callback
  virtual bool GetShareOptions(const char *service, const int fromRect[4],
                               SetShareOptions *setOptions) = 0;
  
  // Open a sharing dialog to gather comments and post the files
  virtual bool ShareImageFiles(const char *service, const int fromRect[4],
                               const std::vector<const ShareImage *> &image) =0;
  
  // Share the named file in the background without opening any dialogs
  virtual bool ShareImage(const char *service, const ShareImage &image) = 0;

  // Bring the app out of paused mode, if enabled, and force at least one redraw
  virtual void ForceRedraw() = 0;
};


// Implement this class to provide a standard framework invoked from OS-side
// code to pass system events from the OS down to C++ code.

class App {
public:
  enum NotifyMessage { ImageUpdated,                  // URLs in data
                       AlbumInserted,                 // URLs in data
                       AlbumUpdated,                  // URLs in data
                       AlbumDeleted,                  // URLs in data
                       ReloadAll };                   // Data is empty
  
  virtual ~App() {}
  
  static App *Create();                               // Factory
  
  virtual bool Init(Callbacks *callbacks) = 0;        // Setup app
  virtual bool Touch(const tui::Event &event) = 0;    // Process events
  virtual bool Step(float seconds) = 0;               // Animate widgets
  virtual bool Dormant() = 0;                         // True = no refresh
  virtual bool Draw() = 0;                            // Draw UI & images
  virtual bool SetDeviceResolution(int w, int h) = 0; // Startup & orientation
  virtual bool Notify(NotifyMessage msg, const std::vector<std::string> &data)=0;
  
protected:
  App() {}                                            // Derived class factory

private:
  App(const App &);                                   // Disallow copy
  void operator=(const App &);                        // Disallow assignment
};

  
};      // namespace sys

#endif  // SYS_H
