//  Copyright (c) 2012 WexWorks. All rights reserved.

#ifndef SYS_H
#define SYS_H

#include <map>
#include <string>
#include <vector>

namespace tui { class Event; }
namespace OIIO { class ParamValueList; }

namespace sys {

// Save the image data in file with the metadata copied from url but with
// the fields below modified (i.e. replace keywords in original url metadata)
struct ShareImage {
  ShareImage(const char *url,const char *file, const char *name,
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
  virtual ~ShareImage() {}
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
  virtual ~AddImage() {}
  enum Type { Unknown, Image, Video };
  virtual bool operator()(const char *album, const char *name,
                          const char *url, int index, Type type) = 0;
};

struct AddAlbum {
  virtual ~AddAlbum() {}
  virtual bool operator()(const char *name, const char *url,
                          size_t assetCount, int libraryId) = 0;
};

struct SetThumbnail {
  virtual ~SetThumbnail() {}
  virtual bool operator()(const char *url, size_t w, size_t h, int orientation,
                          size_t thumbW, size_t thumbH, bool swizzle,
                          size_t bytesPerRow, size_t bitsPerPixel,
                          const unsigned char *pixel) = 0;
};

struct SetImageDate {
  virtual ~SetImageDate() {}
  virtual bool operator()(const char *url, double epochSec) = 0;
};

struct SetImageMetadata {
  virtual ~SetImageMetadata() {}
  virtual bool operator()(const char *url, const OIIO::ParamValueList *meta) =0;
};

struct SetImageCache {
  virtual ~SetImageCache() {}
  virtual bool operator()(const char *url, const char *cachePath) = 0;
};

struct PickImage {
  virtual ~PickImage() {}
  virtual bool operator()(const char *url) = 0;
};

struct SetAlert {
  virtual ~SetAlert() {}
  virtual bool operator()(bool isOK, const char *inputText) = 0;
};

struct SetShareOptions {
  enum ResMode { Source, Fixed, Percent };
  enum Format { ORIGINAL, JPEG, PNG, TIFF };
  virtual ~SetShareOptions() {}
  virtual bool operator()(const char *service, const char *filenameTemplate,
                          const char *comment, ResMode resMode, float dim,
                          const char *shot, const char *album, Format format,
                          float quality, const char *author,
                          const char *copyright, bool setKeywords,bool stripAll,
                          bool stripLocation, bool stripCamera) = 0;
};

  
// Derive a custom Os class that implements the interface using Os-side
// code, e.g. Obj-C or Java, to provide system resources to the C++ code.
// Most of these functions may be asynchronous, providing a callback which
// is invoked once the operation is completed on the Os-side.

class Os {
public:
  Os() {}
  virtual ~Os() {}
  
  // Write information to the console or log file
  virtual void Info(const char *fmt, ...) = 0;
  virtual void Warning(const char *fmt, ...) = 0;
  virtual void Error(const char *fmt, ...) = 0;

  // Returns a URL to a file in the app's local documents folder
  virtual bool FindAppCachePath(const char *name, char path[1024]) = 0;
  
  // Returns the requested user default setting, returned as string
  virtual bool FindUserDefault(const char *name, char value[1024]) = 0;
  
  // Load a text file from the app's resource bundle (blocking)
  virtual bool LoadText(const char *name, const char **text) = 0;
  
  // Load all image albums in the system
  virtual bool LoadSystemAlbums(AddAlbum *addAlbum) = 0;
  
  // Load the named album
  virtual bool LoadAlbum(const char *url, AddAlbum *addAlbum) = 0;
  
  // Load a all the image names in an album
  virtual bool LoadAlbumImageNames(const char *url, int firstIdx, int lastIdx,
                                   AddImage *addImage) = 0;

  // Return the last modification time of the image
  virtual bool LoadImageDate(const char *url, SetImageDate *setDate) = 0;
  
  // Load the thumbnail representation of the image
  virtual bool LoadImageThumbnail(const char *url, size_t maxDim,
                                  SetThumbnail *setThumb) = 0;
  
  // Load only the metadata, not the pixels, from an image
  virtual bool LoadImageMetadata(const char *url,
                                 SetImageMetadata *setMetadata) = 0;
  
  // Convert a file URL into a local file path that we can open in C++
  virtual bool CacheImage(const char *url, const char *cachePath,
                          SetImageCache *setCache) = 0;

  // Open the system image picker and return the picked image
  virtual bool PickImage(PickImage *pickImage) = 0;

  // Compute an image histogram for a block of pixels using background render
  virtual bool ComputeHistogram(const char *url, size_t w, size_t h,
                                const unsigned char *rgba,
                                size_t hCount, unsigned long *histogram) = 0;
  
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

  // Open the product store for the specified item
  virtual bool ShowStore(const char *product, const int fromRect[4]) = 0;
  
  // Open a window and show a video playback
  virtual bool ShowVideo(const char *url, size_t w, size_t h) = 0;
  
  // Bring the app out of paused mode, if enabled, and force at least one redraw
  virtual void ForceRedraw() = 0;

  // Create a new OpenGL context, sharing optionally. Context 0 is UI thread.
  virtual bool CreateGLContext(int id, int shareId = -1) = 0;
  
  // Make the named OpenGL context active in the current thread
  virtual bool SetGLContext(int id) = 0;
  
  // Destroy the named OpenGL context
  virtual bool DeleteGLContext(int id) = 0;
  
  // Return the OpenGL context for the current thread, or -1 for none
  virtual int CurrentGLContext() = 0;
  
  // Returns a newly created texture id for the named resource
  virtual bool CreateGLTexture(const char *name, int minFilter, int magFilter,
                               int wrapS, int wrapT, unsigned int *id,
                               size_t *w, size_t *h) = 0;

  // Calls CreateGLTexture for each file in the list, verifying same size
  virtual bool CreateGLTextures(size_t count, const char **file,
                                size_t *w, size_t *h, unsigned int tex[]);
};


//
// GLContextGuard
//

class GLContextGuard {
public:
  GLContextGuard(int id, Os *os) : mId(id), mOs(os) {
    mLastId = mOs->CurrentGLContext();
    mOs->SetGLContext(mId);
  }
  ~GLContextGuard() { mOs->SetGLContext(mLastId); }
  
private:
  int mId;
  int mLastId;
  Os *mOs;
};


//
// App
//

// Implement this class to provide a standard framework invoked from Os-side
// code to pass system events from the Os down to C++ code.

class App {
public:
  virtual ~App() {}
  
  virtual bool Init(Os *os) = 0;                      // Setup app
  virtual bool Touch(const tui::Event &event) = 0;    // Process events
  virtual bool Step(float seconds) = 0;               // Animate widgets
  virtual bool Dormant() = 0;                         // True = no refresh
  virtual bool Draw() = 0;                            // Draw UI & images
  virtual void SetDeviceName(const char *name) = 0;   // Device-specific fixes
  virtual bool SetDeviceResolution(int w, int h) = 0; // Startup & orientation
  virtual void ReduceMemory() = 0;                    // Low mem warning
  virtual void DeleteCache(int level) = 0;            // level=0..2
  virtual void ReportError(const std::string &msg) = 0;
  virtual bool PurchaseItem(const std::vector<std::string> &idVec) = 0;
  
  // Changes to the device gallery typically made in external apps
  virtual bool UpdateImage(const std::vector<std::string> &urlVec) = 0;
  virtual bool InsertAlbum(const std::vector<std::string> &urlVec) = 0;
  virtual bool UpdateAlbum(const std::vector<std::string> &urlVec) = 0;
  virtual bool DeleteAlbum(const std::vector<std::string> &urlVec) = 0;
  virtual bool ReloadAllAlbums() = 0;

protected:
  App() {}                                            // Derived class factory

private:
  App(const App &);                                   // Disallow copy
  void operator=(const App &);                        // Disallow assignment
};

  
};      // namespace sys

#endif  // SYS_H
