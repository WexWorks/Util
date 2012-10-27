//  Copyright (c) 2012 The 11ers, LLC. All Rights Reserved.

#ifndef TOUCH_UI_H_
#define TOUCH_UI_H_

#include <map>
#include <vector>
#include <string>

namespace GlesUtil { class Text; };


/*
 The touch user interface library provides a lightweight,
 non-invasive widget collection with OpenGL-ES rendering.  TouchUI is
 dependent only on OpenGL-ES and the STL vector template.
 
 Widgets are designed to work with touch-based interfaces and include
 common widgets found on mobile platforms such as fling-able lists,
 and buttons, and includes support for animated and user-derived
 widgets.
 
 TouchUI does *not* own the window system interactions, but is instead
 designed to live inside application-created external OpenGL
 windows/contexts/surfaces, allowing it to be used with a variety of
 window-system toolkits, or called from platform-specific toolkits
 such as Cocoa (iOS), Android, GLUT or custom game engines frameworks.
 
 */

namespace tui {

  // Touch finite state machine stage
  enum EventPhase { TOUCH_BEGAN, TOUCH_MOVED, TOUCH_ENDED, TOUCH_CANCELLED };
  enum FlingDirection { FLING_LEFT, FLING_RIGHT, FLING_UP, FLING_DOWN };
  
  // Event structure, used in Obj-C, to pass OS events
  struct Event {
    struct Touch {                            // One finger
      Touch(size_t id, int x, int y, double timeStamp) : id(id), x(x), y(y),
                                                         timeStamp(timeStamp) {}
      size_t id;                              // Unique descriptor per touch
      int x, y;                               // Pixel location of this touch
      double timeStamp;                       // Event time
    };
    
    Event(EventPhase phase) : phase(phase) {}
    EventPhase phase;                         // FSM location
    std::vector<Touch> touchVec;              // List of touches
  };
  
  
  // Base class for all user interaction elements.
  class Widget {
  public:
    Widget() : mIsEnabled(true), mIsHidden(false), mIsScaling(false),
               mIsPanning(false), mPrevScale(0), mPrevScaleVel(0) {
      memset(mPrevPan, 0, sizeof(mPrevPan));
      memset(mPrevPanVel, 0, sizeof(mPrevPanVel));
    }
    virtual ~Widget() {}
    
    // Can be called multiple times, usually to change position
    virtual bool Init() { return true; }
    
    // Called to draw contents. Viewport set prior to call.
    virtual bool Draw() { return true; }
    
    // Return true if event is consumed and no further processing should occur
    virtual bool Touch(const Event &event) {
      if (!Enabled()) return false;
      return ProcessGestures(event);
    }
    
    // Track touch events and call the Gesture event callbacks
    virtual bool ProcessGestures(const Event &event);
    
    // Gesture callbacks invoked by ProcessGestures. Return true if consumed.
    virtual bool OnFling(FlingDirection dir);
    virtual bool OnScale(EventPhase phase, float scale, float velocity,
                         float originX, float originY) { return false; }
    virtual bool OnPan(EventPhase phase, float panX, float panY,
                       float velocityX, float velocityY) { return false; }
    virtual bool IsScaling() const { return mIsScaling; }
    virtual bool IsPanning() const { return mIsPanning; }
    
    // Enable state indicates whether Touch events are processed
    virtual bool Enabled() const { return mIsEnabled; }
    virtual void Enable(bool status) { mIsEnabled = status; }
    
    // Hidden state controls whether Draw methods are invoked
    virtual bool Hidden() const { return mIsHidden; }
    virtual void Hide(bool status) { mIsHidden = status; }
    
  private:
    static const float kMinScale = 0.01;      // Min scaling amount before event
    static const int kMinPanPix = 10;         // Min pan motion before event

    Widget(const Widget&);                    // Disallow copy ctor
    void operator=(const Widget&);            // Disallow assignment
    
    bool mIsEnabled;                          // Stop processing events
    bool mIsHidden;                           // Stop drawing
    bool mIsScaling;                          // True if processing scale event
    bool mIsPanning;                          // True if processing pan event
    std::vector<Event::Touch> mTouchStart;    // Tracking touches
    float mPrevScale, mPrevPan[2];            // Last reported values
    float mPrevScaleVel, mPrevPanVel[2];      // Last reported velocity
  };
  
  
  // Base class for all rectangular widgets
  class ViewportWidget : public Widget {
  public:
    ViewportWidget() {
      mViewport[0] = mViewport[1] = mViewport[2] = mViewport[3] = 0;
    }
    virtual ~ViewportWidget() {}
    virtual bool Init(int x, int y, int w, int h) {
      if (w == 0 || h == 0)
        return false;
      mViewport[0] = x; mViewport[1] = y;
      mViewport[2] = w; mViewport[3] = h;
      return true;
    }
    
    int Left() const { return mViewport[0]; }
    int Bottom() const { return mViewport[1]; }
    int Right() const { return mViewport[0] + mViewport[2]; }
    int Top() const { return mViewport[1] + mViewport[3]; }
    int Width() const { return mViewport[2];}
    int Height() const { return mViewport[3];}
    bool Inside(int x, int y) const {
      return x >= Left() && x <= Right() && y >= Bottom() && y <= Top();
    }
    
  protected:
    int mViewport[4];                         // [x, y, w, h]
  };
  
  
  // Group operations
  class Group : public Widget {
  public:
    Group() {}
    virtual ~Group() {}
    
    virtual bool Add(Widget *widget);
    virtual bool Remove(Widget *widget);
    virtual void Clear();
    
    // Perform the following actions on all the widgets in the group
    virtual bool Draw();
    virtual bool Touch(const Event &event);
    virtual bool Enabled() const;
    virtual void Enable(bool status);
    virtual bool Hidden() const;
    virtual void Hide(bool status);

  protected:
    std::vector<Widget *> mWidgetVec;         // Grouped widgets
  };
  
  
  // Base class for any animated widgets
  class AnimatedViewport : public ViewportWidget {
  public:
    AnimatedViewport() {}
    virtual ~AnimatedViewport() {}
    virtual bool Step(float seconds) = 0;     // Update animated components
    virtual bool Dormant() const = 0;         // True if Step can be skipped
  };
  
  
  // Moves a texture from one viewport to another
  class Sprite : public AnimatedViewport {
  public:
    Sprite() : mOriginalOpacity(0), mTargetOpacity(0),
               mSecondsToTarget(0), mSecondsRemaining(0),
               mOpacity(0), mSpriteTexture(0) {
      mOriginalViewport[0] = mOriginalViewport[1] = 0;
      mOriginalViewport[2] = mOriginalViewport[3] = 0;
      mTargetViewport[0] = mTargetViewport[1] = 0;
      mTargetViewport[2] = mTargetViewport[3] = 0;
      mSpriteUV[0] = mSpriteUV[1] = mSpriteUV[2] = mSpriteUV[3] = 0;
    }
    virtual bool Init(int x, int y, int w, int h, float opacity,
                      float u0, float v0, float u1, float v1, 
                      unsigned int texture);
    virtual bool Step(float seconds);
    virtual bool Draw();
    virtual bool Touch(const Event &event) { return false; }
    virtual bool SetTarget(int x, int y, int w, int h, float opacity,float sec);
    virtual bool Dormant() const { return mSecondsRemaining <= 0; }
    virtual unsigned int Texture() const { return mSpriteTexture; }
    
  protected:
    int mOriginalViewport[4];                 // Initial viewport
    int mTargetViewport[4];                   // Destination viewport
    float mOriginalOpacity;                   // Initial opacity
    float mTargetOpacity;                     // Destination opacity
    float mSecondsToTarget;                   // Time to destination
    float mSecondsRemaining;                  // Time left since SetTarget
    float mOpacity;                           // Interpolated opacity
    unsigned int mSpriteTexture;              // Sprite
    float mSpriteUV[4];                       // [u0, v0, u1, v1] in sprite tex
    static unsigned int mSpriteProgram;       // Program for drawing sprites
    static int mSpriteAP, mSpriteAUV;         // Position and UV attributes
    static int mSpriteUCTex, mSpriteUOpacity; // Opacity uniform index
  };
  
  
  // A horizontal (or vertical?) scrollable list of clickable frames
  class FlinglistImpl : public AnimatedViewport {
  public:
    class Frame {
    public:
      virtual bool Draw() = 0;                // Viewport set prior to call
      virtual bool OnTouchTap(const Event::Touch &touch) { return false; }
      virtual bool OnLongTouch(const int locationInView[2]) { return false; }
    };
    
    FlinglistImpl() : mFrameDim(0), mScrollableDim(0),
                      mVertical(false), mPixelsPerCm(0),
                      mViewportInset(0), mTouchFrameIdx(-1), 
                      mMovedAfterDown(false), mMovedOffAxisAfterDown(false),
                      mScrollOffset(0), mScrollVelocity(0),
                      mScrollBounce(0), mThumbFade(0), mSnapToCenter(false),
                      mSnapSeconds(0), mSnapRemainingSeconds(0),
                      mSnapOriginalOffset(0), mSnapTargetOffset(0),
                      mSnapLocationOffset(0),
                      mOverpullOffTex(0), mOverpullOnTex(0),
                      mDragHandleTex(0), mGlowDragHandle(false),mGlowSeconds(0),
                      mLongPressSeconds(0), mSingleFrameFling(false) {
      mTouchStart[0] = mTouchStart[1] = 0;
      mOverpullDim[0] = mOverpullDim[1] = 0;
      mDragHandleDim[0] = mDragHandleDim[1] = 0;
    }
    virtual ~FlinglistImpl() {}
    virtual bool Init(int x, int y, int w, int h, int frameDim,
                      bool vertical, float pixelsPerCm);
    virtual void SetSnapToCenter(bool status) { mSnapToCenter = status; }
    virtual void SetSingleFrameFling(bool status) { mSingleFrameFling = status;}
    virtual bool SetDragHandle(unsigned int texture, size_t w, size_t h);
    virtual bool SetOverpull(unsigned int offTex, unsigned int onTex,
                             size_t w, size_t h);
    virtual void SetScrollableDim(int dim) { mScrollableDim = dim; }
    virtual void GlowDragHandle(bool status) { mGlowDragHandle = status; }
    virtual bool Clear();
    virtual bool Draw();
    virtual bool Step(float seconds);
    virtual bool Dormant() const;
    virtual bool Snap(const Frame *frame, float seconds);
    virtual bool CancelSnap();
    virtual bool Jiggle();
    virtual bool Touch(const Event &event);
    virtual size_t Size() const { return mFrameVec.size(); }
    virtual bool Viewport(const Frame *frame, int viewport[4]) const;
    virtual void OverpullViewport(int viewport[4]) const;
    virtual bool VisibleFrameRange(int *min, int *max) const;
    virtual int FrameIdx(const Frame *frame) const;
    virtual size_t FrameCount() const { return mFrameVec.size(); }
    virtual int ScrollDistance(const Frame *frame) const;
    virtual const Frame *operator[](size_t i) const { return mFrameVec[i]; }
    virtual Frame *operator[](size_t i) { return mFrameVec[i]; }
    
    virtual void OnOverpullRelease() {}
    virtual void OnMove() {}
    virtual bool OnOffAxisMove(const Event::Touch &touch,
                               const int touchStart[2]) { return false; }
    virtual void OnTouchEnded() {}

  protected:
    static const int kDragMm = 4;             // Threshold for drag events
    static const int kJiggleMm = 10;          // Offset 
    static const int kSnapVelocity = 10;      // In pixels/sec
    static const float kLongPressSeconds;     // Threshold for timeout
    static const float kJiggleSeconds;        // Length of jiggle

    virtual bool Append(Frame *frame);        // Do not allow generic frames,
    virtual bool Prepend(Frame *frame);       //   use templates instead.
    virtual bool Delete(Frame *frame);
    int FindFrameIdx(const int xy[2]) const;
    int TotalHeight() const { return mFrameDim * mFrameVec.size(); }
    int ScrollMin() const { const int dim = mVertical ? Height() : Width();
      return dim / 2 + mScrollableDim / 2 - dim + mSnapLocationOffset; }
    int ScrollMax() const { return ScrollMin() + Size() * mFrameDim - mScrollableDim; }
    int ScrollToOffset(int i) const { return i * mFrameDim + ScrollMin(); }
    virtual void SnapIdx(size_t idx, float seconds);
    virtual int FlingingSnapIdx() const;
    virtual bool MovedAfterDown() const {
      return mMovedAfterDown || mMovedOffAxisAfterDown; }
    virtual int OverpullPixels() const {
      return -0.15 * std::max(Width(), Height()); }
    virtual void OverpullNDCRange(float *x0, float *y0,
                                  float *x1, float *y1) const;
    
    std::vector<Frame *> mFrameVec;           // Frames (caller owns pointer)
    int mFrameDim;                            // Pixel dimensions of each frame
    int mScrollableDim;                       // Default to mFrameDim
    bool mVertical;                           // True=vertical, false=horizontal
    float mPixelsPerCm;                       // Adjust the drag thresholds
    int mViewportInset;                       // Inset visible frame region
    int mTouchFrameIdx;                       // Index in mFrameVec or -1
    int mTouchStart[2];                       // Location of first down touch
    bool mMovedAfterDown;                     // True if touch moved
    bool mMovedOffAxisAfterDown;              // True if touch moved
    float mScrollOffset;                      // Distance currently scrolled
    float mScrollVelocity;                    // Fling velocity
    float mScrollBounce;                      // !=0 outside valid scroll range
    float mThumbFade;                         // Seconds remaining visible
    bool mSnapToCenter;                       // True if we always snap frames
    float mSnapSeconds;                       // Total time for snap
    float mSnapRemainingSeconds;              // Time left before snap finished
    float mSnapOriginalOffset;                // Offset at start of snap
    float mSnapTargetOffset;                  // Offset at end of snap
    float mSnapLocationOffset;                // Offset to snap location
    size_t mOverpullDim[2];                   // Size of overpull texture
    unsigned int mOverpullOffTex;             // Overpull inactive texture
    unsigned int mOverpullOnTex;              // Overpull active texture
    size_t mDragHandleDim[2];                 // Size of drag handle texture
    unsigned int mDragHandleTex;              // Drag handle texture
    bool mGlowDragHandle;                     // Pulsating glow if true
    float mGlowSeconds;                       // Track for pulsating glow
    float mLongPressSeconds;                  // Seconds since not moved
    bool mSingleFrameFling;                   // Fling interaction mode
    static unsigned int mFlingProgram;        // Non-item drawing
    static unsigned int mGlowProgram;         // Drag handle glow
  };

  
  // Templated Flinglist that only allows a single type of Frame
  template <class F>
  class Flinglist : public FlinglistImpl {
  public:
    // Create a type-safe interface for a specific frame type
    virtual bool Append(F *frame) { return FlinglistImpl::Append(frame); }
    virtual bool Prepend(F *frame) { return FlinglistImpl::Prepend(frame); }
    virtual bool Delete(F *frame) { return FlinglistImpl::Delete(frame); }
    virtual const F *operator[](size_t i) const { 
      return static_cast<F *>(mFrameVec[i]);
    }
    virtual F *operator[](size_t i) {
      return static_cast<F *>(mFrameVec[i]);
    }
  };
  
  
  // Flinglist that only moves one frame at a time with a recticle selector
  class FilmstripImpl : public FlinglistImpl {
  public:
    FilmstripImpl() : mSelectedFrameIdx(-1) {}
    virtual ~FilmstripImpl() {}
    virtual void OnSelectionChanged() {}
    
  protected:
    virtual bool Prepend(Frame *frame);
    virtual bool Delete(Frame *frame);
    virtual void SnapIdx(size_t idx, float seconds);
    
    int mSelectedFrameIdx;                    // Selected (and centered) frame
  };
  
  
  // Templated Filmstrip that only allows a single type of Frame
  template <class F>
  class Filmstrip : public FilmstripImpl {
  public:
    // Create a type-safe interface for a specific frame type
    virtual bool Append(F *frame) { return FilmstripImpl::Append(frame); }
    virtual bool Prepend(F *frame) { return FilmstripImpl::Prepend(frame); }
    virtual bool Delete(F *frame) { return FilmstripImpl::Delete(frame); }
    virtual const F *operator[](size_t i) const { 
      return static_cast<const F *>(mFrameVec[i]);
    }
    virtual F *operator[](size_t i) {
      return static_cast<F *>(mFrameVec[i]);
    }
    virtual F *SelectedFrame() {
      if (mSelectedFrameIdx < 0) return NULL;
      return static_cast<F *>(mFrameVec[mSelectedFrameIdx]);
    }
    virtual const F *SelectedFrame() const {
      if (mSelectedFrameIdx < 0) return NULL;
      return static_cast<const F *>(mFrameVec[mSelectedFrameIdx]);
    }
  };
  
  
  // Button touch behavior, invokes OnTouchTap when activated
  class Button : public ViewportWidget {
  public:
    Button() {}
    virtual ~Button() {}
    
    // Viewport contains entire texture, toggled
    virtual bool Touch(const Event &event);
    virtual bool Pressed() const;

    // Override with action on either select or deselect
    virtual bool OnTouchTap(const Event::Touch &touch) = 0;
    
  protected:
    struct Press {
      Press(size_t id, bool pressed, int x, int y) :
        id(id), pressed(pressed), x(x), y(y) {}
      size_t id;                              // Touch id
      bool pressed;                           // True if currently pressed
      int x, y;                               // Press location
    };

    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      return OnTouchTap(touch);
    }
    int FindPress(size_t id) const;
    
    std::vector<Press> mPressVec;             // Support multiple touch events
  };
  
  
  // Button that draws one of two images depending on pressed state
  class ImageButton : public Button {
  public:
    ImageButton() : mDefaultTexture(0), mPressedTexture(0),
                    mBlendEnabled(false) {}
    virtual ~ImageButton() {}
    
    virtual bool Init(int x, int y, int w, int h, bool blend,
                      unsigned int defaultTexture, unsigned int pressedTexture);
    virtual bool Draw();
    
  protected:
    unsigned int mDefaultTexture;             // Default texture id
    unsigned int mPressedTexture;             // Transient pressed texture id
    bool mBlendEnabled;                       // True if GL_BLEND enabled
  };
  
  
  // Maintains selection state with button semantics
  class CheckboxButton : public Button {
  public:
    CheckboxButton() : mSelected(false), mBlendEnabled(false),
                       mDeselectedTex(0), mPressedTex(0), mSelectedTex(0) {}
    virtual ~CheckboxButton() {}
    
    virtual bool Init(int x, int y, int w, int h, bool blend,
                      unsigned int deselectedTex, unsigned int pressedTex,
                      unsigned int selectedTex);
    virtual bool Draw();
    virtual void SetSelected(bool status) { mSelected = true; }
    virtual bool Selected() const { return mSelected; }
    
  protected:
    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      SetSelected(!Selected());
      return Button::InvokeTouchTap(touch);
    }
    
    bool mSelected;
    bool mBlendEnabled;
    unsigned int mDeselectedTex, mPressedTex, mSelectedTex;
  };
  
  
  // Manages a set of CheckboxButtons, ensuring that only a single
  // checkbox of the group is selected at any given time.
  class RadioGroup : public Group {
  public:
    virtual bool Add(CheckboxButton *button) { return Add(button); }
    virtual bool Remove(CheckboxButton *button) { return Remove(button); }
    virtual void SetSelected(CheckboxButton *button);
    virtual bool Touch(const Event &event);
    
  private:
    virtual bool Add(Widget *widget) { return Group::Add(widget); }
    virtual bool Remove(Widget *widget) { return Group::Add(widget); }
  };
  
  
  // Manages a rectangular frame with pan and zoom.
  // Animated "soft limits" keep the image within a set of bounds,
  // but allow some movement past the edge.
  class FrameViewer : public AnimatedViewport {
  public:
    // Manages a rectangular region that can be panned and zoomed
    class Frame {
    public:
      Frame() : mFrameViewer(NULL) {}
      virtual void Init(FrameViewer *viewer) { mFrameViewer = viewer; }
      
      // Draw the UV region [u0,v0]x[u1,v1] into NDC rect [x0,y0]x[x1,y1]
      // into the current viewport.
      virtual bool Draw(float x0, float y0, float x1, float y1,
                        float u0, float v0, float u1, float v1) { return true; }
      virtual bool InvertVertical() const { return false; }
      virtual bool InvertHorizontal() const { return false; }
      virtual size_t Width() const = 0;       // Width of frame image in pixels
      virtual size_t Height() const = 0;      // Height of frame image in pixels
      
      protected:
        FrameViewer *mFrameViewer;            // Backpointer
    };
    
    FrameViewer() : mFrame(NULL),
                    mScale(1), mScaleVelocity(0),
                    mIsTargetScaleActive(false),
                    mIsTargetCenterActive(false),
                    mScaleMin(0), mScaleMax(0) {
      memset(mCenterUV, 0, sizeof(mCenterUV));
      memset(mCenterVelocityUV, 0, sizeof(mCenterVelocityUV));
    }
    virtual ~FrameViewer() {}
    
    virtual bool Init(int x, int y, int w, int h);
    virtual bool SetFrame(Frame *frame);      // Set current frame
    virtual Frame *Frame() const { return mFrame; }
    virtual bool Draw();                      // Render all visible views
    virtual bool Step(float seconds);         // Animate views
    virtual bool Dormant() const;             // True if all views dormant
    virtual bool OnScale(EventPhase phase, float scale, float velocity,
                         float originX, float originY);
    virtual bool OnPan(EventPhase phase, float panX, float panY,
                       float velocityX, float velocityY);
    virtual bool SnapCurrentToFitFrame();     // Whole image in frame

  protected:
    static const float kDamping = 0.7;        // Viscous damping

    FrameViewer(const FrameViewer &);         // Disallow copy ctor
    void operator=(const FrameViewer &);      // Disallow assignment
    
    void ComputeScaleRange();
    void ComputeFrameDisplayRect(const class Frame &frame, float *x0, float *y0,
                                 float *x1, float *y1, float *u0, float *v0,
                                 float *u1, float *v1);
    float ConvertUToNDC(const class Frame &frame, float u) const;
    float ConvertVToNDC(const class Frame &frame, float u) const;
    
    class Frame *mFrame;                      // Active frame
    float mScale, mScaleVelocity;             // Image scale, 1 -> 1 pixel
    float mCenterUV[2], mCenterVelocityUV[2]; // Center of screen in UV 0 -> 1
    float mTargetScale;                       // Rubberband targets
    bool mIsTargetScaleActive;                // True if we use target scale
    bool mIsTargetCenterActive;               // True if we use target center
    float mScaleMin, mScaleMax;               // Valid scale range
  };
  
  
  // Renders text into a box with a specified alignment.
  class TextBox : public ViewportWidget {
  public:
    enum Align { Left, Right, Center };

    TextBox() : mAlign(Left) {}
    virtual ~TextBox() {}
    
    virtual bool Init(int x, int y, int w, int h, GlesUtil::Text *gltext);
    virtual void SetText(const char *text) { mText = text; }
    virtual void SetAlign(Align align) { mAlign = align; }
    virtual bool Draw();
    
  protected:
    Align mAlign;
    std::string mText;
    GlesUtil::Text *mGlesText;
  };
  
} // namespace tui

#endif  // TOUCH_UI_H_
