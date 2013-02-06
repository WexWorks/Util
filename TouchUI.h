//  Copyright (c) 2012 The 11ers, LLC. All Rights Reserved.

#ifndef TOUCH_UI_H_
#define TOUCH_UI_H_

#include <map>
#include <math.h>
#include <vector>
#include <string>


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
  
  // Event structure, used in Obj-C, to pass OS events
  struct Event {
    struct Touch {                            // One finger
      Touch(size_t id, int x, int y, double timestamp)
        : id(id), x(x), y(y), timestamp(timestamp) {}
      size_t id;                              // Unique descriptor per touch
      int x, y;                               // Pixel location of this touch
      double timestamp;                       // Event time
    };
    
    Event(EventPhase phase) : phase(phase) {}
    EventPhase phase;                         // FSM location
    std::vector<Touch> touchVec;              // List of touches
  };
  
  
  // Base class for all user interaction elements.
  class Widget {
  public:
    Widget() : mIsEnabled(true), mIsHidden(false), mIsScaling(false),
               mIsDragging(false), mIsHorizontalDrag(false), mMVP(NULL) {}
    virtual ~Widget() {}
    
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
    virtual bool OnScale(EventPhase phase, float scale, float x, float y,
                         double timestamp) { return false; }
    virtual bool OnDrag(EventPhase phase, float x, float y,
                        double timestamp) { return false; }
    virtual bool IsScaling() const { return mIsScaling; }
    virtual bool IsDragging() const { return mIsDragging; }
    virtual bool IsHorizontalDrag() const { return mIsHorizontalDrag; }
    
    // Enable state indicates whether Touch events are processed
    virtual bool Enabled() const { return mIsEnabled; }
    virtual void Enable(bool status) { mIsEnabled = status; }
    
    // Hidden state controls whether Draw methods are invoked
    virtual bool Hidden() const { return mIsHidden; }
    virtual void Hide(bool status) { mIsHidden = status; }
    
    virtual size_t TouchStartCount() const { return mTouchStart.size(); }
    virtual const Event::Touch &TouchStart(size_t idx) const { return mTouchStart[idx]; }
    
    virtual void SetMVP(const float *mvp) { mMVP = mvp; }
    virtual const float *MVP() const { return mMVP; }
    virtual void GetNDCRect(float *x0, float *y0, float *x1, float *y1) const {
      *x0 = *y0 = -1; *x1 = *y1 = 1;
    }
    
  private:
    static const float kMinScale = 0.03;      // Min scaling amount before event
    static const int kMinPanPix = 40;         // Min pan motion before event

    Widget(const Widget&);                    // Disallow copy ctor
    void operator=(const Widget&);            // Disallow assignment
    
    bool mIsEnabled;                          // Stop processing events
    bool mIsHidden;                           // Stop drawing
    bool mIsScaling;                          // True if processing scale event
    bool mIsDragging;                         // True if processing pan event
    bool mIsHorizontalDrag;                   // True if drag started horiz
    std::vector<Event::Touch> mTouchStart;    // Tracking touches
    const float *mMVP;
  };
  
  
  // Base class for all rectangular widgets
  class ViewportWidget : public Widget {
  public:
    ViewportWidget() {
      mViewport[0] = mViewport[1] = mViewport[2] = mViewport[3] = 0;
    }
    virtual ~ViewportWidget() {}
    virtual bool SetViewport(int x, int y, int w, int h) {
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
    virtual void GetNDCRect(float *x0, float *y0, float *x1, float *y1) const {
      if (MVP()) {
        *x0 = Left(); *y0 = Bottom(); *x1 = Right(); *y1 = Top();
      } else {
        Widget::GetNDCRect(x0, y0, x1, y1);
      }
    }

    virtual bool OnTapTouch(const Event::Touch &touch) { return false; }
    virtual bool OnLongTouch(const Event::Touch &touch) { return false; }
    virtual bool ProcessGestures(const Event &event);
    
  protected:
    virtual bool TouchStartInside() const;
    
    int mViewport[4];                         // [x, y, w, h]
  };
  
  
  // Text label, display only
  class Label : public ViewportWidget {
  public:
    Label() : mText(NULL), mPts(0), mPtW(0), mPtH(0), mAlign(1) {
      mColor[0] = mColor[1] = mColor[2] = mColor[3] = 1;
    }
    virtual ~Label();
    virtual bool Init(const char *text, float pts);
    virtual bool SetText(const char *text, float pts);
    virtual void SetJustify(int glesUtilAlign) { mAlign = glesUtilAlign; }
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void SetTextColor(float r, float g, float b, float a) {
      mColor[0] = r; mColor[1] = g; mColor[2] = b; mColor[3] = a;
    }
    virtual bool Draw();
    
    // Initialize the font used to draw all labels
    static void SetFont(void *font) { sFont = font; }
    
  private:
    static void *sFont;
    const char *mText;
    float mPts, mPtW, mPtH;
    float mColor[4];
    int mAlign;
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
    virtual bool Init(float opacity, float u0, float v0, float u1, float v1, 
                      unsigned int texture);
    virtual bool Step(float seconds);
    virtual bool Draw();
    virtual bool Touch(const Event &event) { return false; }
    virtual bool SetTarget(int x, int y, int w, int h, float opacity,float sec);
    virtual bool Dormant() const { return mSecondsRemaining <= 0; }

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
  
  
  //
  // Buttons
  //
  
  // Button touch behavior, invokes OnTouchTap when activated
  class Button : public ViewportWidget {
  public:
    Button() {}
    virtual ~Button() {}
    
    // Viewport contains entire texture, toggled
    virtual bool Touch(const Event &event);
    virtual bool Pressed() const;
    
    // Override with action
    virtual bool OnTouchTap(const Event::Touch &touch) { return false; };
    
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
    ImageButton() : mDefaultTex(0), mPressedTex(0),
                    mIsBlendEnabled(false), mIsTexOwned(false) {}
    virtual ~ImageButton();
    
    virtual bool Init(bool blend, unsigned int defaultTex,
                      unsigned int pressedTex, bool ownTex=false);
    virtual bool Draw();
    
  protected:
    unsigned int mDefaultTex;                 // Default texture id
    unsigned int mPressedTex;                 // Transient pressed texture id
    bool mIsBlendEnabled;                     // True if GL_BLEND enabled
    bool mIsTexOwned;                         // True if we delete textures
  };
  
  
  // Maintains selection state with button semantics
  class CheckboxButton : public Button {
  public:
    CheckboxButton() : mSelected(false) {}
    virtual ~CheckboxButton() {}
    
    virtual void SetSelected(bool status) { mSelected = status; }
    virtual bool Selected() const { return mSelected; }
    
  protected:
    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      SetSelected(!Selected());
      return Button::InvokeTouchTap(touch);
    }
    
    bool mSelected;
  };
  
  
  // Checkbox button drawn using three (four?) textures
  class CheckboxImageButton : public CheckboxButton {
  public:
    CheckboxImageButton() : mBlendEnabled(false), mDeselectedTex(0),
    mPressedTex(0), mSelectedTex(0) {}
    virtual ~CheckboxImageButton() {}
    
    virtual bool Init(bool blend, unsigned int deselectedTex,
                      unsigned int pressedTex, unsigned int selectedTex);
    virtual bool Draw();
    
  protected:
    bool mBlendEnabled;
    unsigned int mDeselectedTex, mPressedTex, mSelectedTex;
  };
  
  
  // Button drawn by extending (scaling middle, but not edges) of the
  // background textures with a string centered on top
  class TextButton : public Button {
  public:
    TextButton() : mLabel(NULL), mDefaultTex(0), mPressedTex(0) {
      memset(mDim, 0, sizeof(mDim));
    }
    virtual bool Init(const char *text, float pts, size_t w, size_t h,
                      unsigned int defaultTex, unsigned int pressedTex);
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void SetLabelColor(float r, float g, float b, float a) {
      mLabel->SetTextColor(r, g, b, a);
    }
    virtual bool Draw();
    
  protected:
    Label *mLabel;
    size_t mDim[2];
    unsigned int mDefaultTex, mPressedTex;
  };
  
  
  // Checkbox button with extended background and foreground text
  class TextCheckbox : public CheckboxButton {
  public:
    TextCheckbox() : mLabel(NULL), mDeselectedTex(0), mPressedTex(0),
                     mSelectedTex(0) {}
    virtual bool Init(const char *text, float pts, size_t w, size_t h,
                      unsigned int deselectedTex, unsigned int pressedTex,
                      unsigned int selectedTex);
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void SetLabelColor(float r, float g, float b, float a) {
      mLabel->SetTextColor(r, g, b, a);
    }
    virtual bool Draw();
    
  protected:
    Label *mLabel;
    size_t mDim[2];
    unsigned int mDeselectedTex, mPressedTex, mSelectedTex;
  };
  
  
  // Manages a set of CheckboxButtons, ensuring that only a single
  // checkbox of the group, or none, is selected at any given time
  class RadioButton : public ViewportWidget {
  public:
    RadioButton() : mIsNoneAllowed(false) {}
    virtual ~RadioButton();
    virtual void Add(CheckboxButton *button);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual bool Touch(const Event &event);
    virtual bool Draw();
    virtual CheckboxButton *Selected() const;
    virtual void SetSelected(CheckboxButton *button);
    virtual void SetIsNoneAllowed(bool status) { mIsNoneAllowed = status; }
    virtual bool IsNoneAllowed() const { return mIsNoneAllowed; }
    virtual bool OnNoneSelected() { return false; }
    
  protected:
    bool mIsNoneAllowed;
    std::vector<CheckboxButton *> mButtonVec;
  };
  
  
  // Movable button with optional constraints
  class Handle : public Button {
  public:
    Handle() : mIsSegment(false) { memset(mLine, 0, sizeof(mLine)); }
    virtual ~Handle() {}
    virtual bool Touch(const Event &event);
    virtual void SetUnconstrained() { memset(mLine, 0, sizeof(mLine)); }
    virtual void SetXConstrained(bool status) { SetConstraintDir(0, status); }
    virtual void SetYConstrained(bool status) { SetConstraintDir(status, 0); }
    virtual void SetConstraintDir(float x, float y);
    virtual void SetConstraintSegment(float x0, float y0, float x1, float y1) {
       mIsSegment = true; mLine[0]=x0; mLine[1]=y0; mLine[2]=x1; mLine[3]=y1;
    }
    
  private:
    virtual bool Constrained() const {
      return mLine[0] != 0 || mLine[1] != 0 || mLine[2] != 0 || mLine[3] != 0;
    }
    
    float mLine[4];                             // [ax, ay, bx, by]
    bool mIsSegment;                            // Direction or segment
  };
  
  
  // Handle with an associated texture for display
  class ImageHandle : public Handle {
  public:
    ImageHandle() : mDefaultTex(0), mPressedTex(0) {}
    virtual ~ImageHandle() {}
    
    virtual bool Init(unsigned int defaultTex, unsigned int pressedTex);
    virtual bool Draw();
    
  private:
    unsigned int mDefaultTex, mPressedTex;
  };
  
  
  // Slider using a constrained handle
  
  class Slider : public ViewportWidget {
  public:
    Slider() : mHandle(NULL), mSliderTex(0) {}
    virtual ~Slider();
    
    virtual bool Init(unsigned int sliderTex, size_t handleW, size_t handleH,
                      unsigned int handleTex, unsigned int handlePressedTex);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual bool Touch(const Event &event);
    virtual bool Draw();
    virtual float Value() const { return mHandleT; }
    virtual bool OnValueChanged(float value) const { return false; }
    
  private:
    ImageHandle *mHandle;
    unsigned int mSliderTex;
    float mHandleT;
  };
  
  
  //
  // Groups
  //
  
  // Group operations
  class Group : public Widget {
  public:
    Group() : mIsMultitouch(false) {}
    virtual ~Group() {}
    
    virtual bool Add(Widget *widget);
    virtual bool Remove(Widget *widget);
    virtual void Clear();
    virtual void SetMultitouch(bool status) { mIsMultitouch = status; }
    
    // Perform the following actions on all the widgets in the group
    // (dynamic cast used for Viewport and AnimateViewport methods)
    virtual bool Draw();
    virtual bool Touch(const Event &event);
    virtual bool Step(float seconds);
    virtual bool Dormant() const;
    virtual bool Enabled() const;
    virtual void Enable(bool status);
    virtual bool Hidden() const;
    virtual void Hide(bool status);
    virtual void SetMVP(const float *mvp);
    
  protected:
    std::vector<Widget *> mWidgetVec;         // Grouped widgets
    bool mIsMultitouch;                       // Touch first or all widgets?
  };
  
  
  // A set of horizontally arrangend widgets over a stretched background tex.
  // Add widgets from left to right, with spacers interspersed, and the
  // widgets will be automatically adjusted on rotation. Use ViewportWidget
  // for fixed spacing (flexible spacer is a ViewportWidget with negative size).
  // Set the size of each widget for spacing, but the (x,y) is ignored.
  class Toolbar : public ViewportWidget {
  public:
    static const size_t kStdHeight = 44;
    
    Toolbar() : mEdgeTex(0), mCenterTex(0), mEdgeDim(0) {}
    virtual bool Init(unsigned int centerTex);
    virtual bool SetEdge(unsigned int edgeTex, unsigned int edgeDim);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void Add(ViewportWidget *widget) { mWidgetVec.push_back(widget); }
    virtual bool AddFixedSpacer(int w);
    virtual bool AddFlexibleSpacer();
    virtual void Clear() { mWidgetVec.clear(); }
    virtual bool Touch(const Event &event);
    virtual bool Draw();
    
  private:
    unsigned int mEdgeTex, mCenterTex;
    unsigned int mEdgeDim;
    std::vector<ViewportWidget *> mWidgetVec;
  };
  
  
  // A horizontal or vertical scrollable list of clickable frames
  class FlinglistImpl : public AnimatedViewport {
  public:
    class Frame {
    public:
      virtual bool Draw() = 0;                // Viewport set prior to call
      virtual bool OnTouchTap(const Event::Touch &touch) { return false; }
      virtual bool OnLongTouch(int x, int y) { return false; }
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
    virtual bool Init(int frameDim, bool vertical, float pixelsPerCm);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetFrameDim(int dim);
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
                               int x, int y) { return false; }
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
    int FindFrameIdx(int x, int y) const;
    int TotalHeight() const { return mFrameDim * mFrameVec.size(); }
    int ScrollMin() const { const int dim = mVertical ? Height() : Width();
      return dim / 2 + mScrollableDim / 2 - dim + mSnapLocationOffset; }
    int ScrollMax() const {
      int x = ScrollMin() + Size() * mFrameDim - mScrollableDim;
      return x < ScrollMin() ? ScrollMin() : x;
    }
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
  
  
  // Manages a rectangular frame with pan and zoom.
  // Animated "soft limits" keep the image within a set of bounds,
  // but allow some movement past the edge.
  // Derive a custom class and implement content definition methods.
  // Unlike the Flinglist classes, the Frame is drawn using a
  // set of NDC coords and a uv-rectangle, and it is up to the
  // implementation to draw the appropriate region in the image.
  // Similarly, any event processing for sub-widgets within the frame
  // requires an invserse transformation of the touch event coordinates.
  class Frame : public AnimatedViewport {
  public:
    Frame() : mScale(1), mScaleVelocity(0),
              mStartScale(0), mPrevScale(0),
              mPrevScaleTimestamp(0), mPrevDragTimestamp(0),
              mTargetScale(0),
              mIsTargetScaleActive(false),
              mIsTargetWindowActive(false),
              mIsDirty(false),
              mScaleMin(0), mScaleMax(0) {
      memset(mCenterUV, 0, sizeof(mCenterUV));
      memset(mStartCenterUV, 0, sizeof(mStartCenterUV));
      memset(mPrevDragXY, 0, sizeof(mPrevDragXY));
      memset(mCenterVelocityUV, 0, sizeof(mCenterVelocityUV));
    }
    virtual ~Frame() {}

    // Content Definition -- Override these to define the Frame content
    virtual size_t ImageWidth() const = 0;    // Width of frame image in pixels
    virtual size_t ImageHeight() const = 0;   // Height of frame image in pixels
    
    // Draw the Frame content within the UV region [u0,v0]x[u1,v1]
    // into NDC rect [x0,y0]x[x1,y1] into the current viewport.
    virtual bool DrawImage(float x0, float y0, float x1, float y1,
                           float u0, float v0, float u1,float v1) = 0;
    
    // Content manipulation -- Override to define Frame behavior
    virtual bool IsVerticalInverted() const { return false; }
    virtual bool IsHorizontalInverted() const { return false; }
    virtual bool IsXLocked() const { return false; }
    virtual bool IsYLocked() const { return false; }
    virtual bool IsScaleLocked() const { return false; }
    virtual bool IsSnappingToPixelCenter() const { return false; }
    virtual float DragDamping() const { return 0.9; }
    virtual float DragFling() const { return 2; }
    virtual float ScaleDamping() const { return 0.9; }
    virtual float ScaleFling() const { return 1; }
    
    // Widget Methods -- Override if you need custom behavior
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual bool Draw();                      // Render all visible views
    virtual bool Step(float seconds);         // Animate views
    virtual bool Dormant() const;             // True if all views dormant
    virtual bool OnScale(EventPhase phase, float scale, float x, float y,
                         double timestamp);
    virtual bool OnDrag(EventPhase phase, float x, float y, double timestamp);
    virtual bool SnapToFitFrame();            // Whole image in frame
    virtual bool SnapToFitWidth(float v);     // v in [0, 1] [top, bot]
    virtual float Scale() const { return mScale; }
    
    // Compute the current display region that would be sent to DrawImage
    virtual void ComputeDisplayRect(float *x0, float *y0, float *x1, float *y1,
                                    float *u0, float *v0, float *u1, float *v1) const;
    
    // Convert the region in NDC and UV space provided to Frame::Draw into
    // a 4x4 matrix (really 2D, so it could be 3x3), that transforms points
    // in pixel coordinates in the Frame image into NDC for use as a MVP.
    static void RegionToM44f(float dst[16], int imageWidth, int imageHeight,
                             float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1);
    
  private:
    Frame(const Frame &);                     // Disallow copy ctor
    void operator=(const Frame &);            // Disallow assignment
    
    bool Reset();
    void ComputeScaleRange();
    float U2Ndc(float u) const;
    float V2Ndc(float v) const;
    float Ndc2U(float x) const;
    float Ndc2V(float y) const;
    
    float mScale, mScaleVelocity;             // Image scale, 1 -> 1 pixel
    float mCenterUV[2], mCenterVelocityUV[2]; // Center of screen in UV 0 -> 1
    float mStartScale, mStartCenterUV[2];     // Values at TOUCH_BEGIN
    float mPrevScale, mPrevDragXY[2];         // For velocity
    double mPrevScaleTimestamp;               // For velocity
    double mPrevDragTimestamp;                // For velocity
    float mTargetScale;                       // Rubberband targets
    bool mIsTargetScaleActive;                // True if we use target scale
    bool mIsTargetWindowActive;               // True if we use target center
    bool mIsDirty;                            // True if we need to repaint
    float mScaleMin, mScaleMax;               // Valid scale range
  };
  
  
  // Implement to compare two buttons for ordering in the ButtonGridFrame::Sort
  struct CompareFunctor {
    virtual bool operator()(const Button *a, const Button *b) const = 0;
  };
  
  // Proxy because std::sort accepts Compare by-value, slicing derived classes.
  // Example: MyCompareFunctor f; Sort(tui::CompareButton(f));
  struct CompareButton {
    CompareButton(CompareFunctor &f) : cmp(f) {}
    bool operator()(const Button *a, const Button *b) const { return cmp(a, b); }
    CompareFunctor &cmp;
  };
  
  
  // A frame using grid of custom buttons. Buttons are drawn in pixel
  // coordinates of the frame (not screen) using the MVP transform.
  // Buttons are clipped prior to drawing to 
  class ButtonGridFrame : public Frame {
  public:
    ButtonGridFrame();
    virtual bool Init(int wideCount, int narrowCount,
                      int buttonPad, int topPad, int bottomPad);
    virtual void Add(Button *button);
    virtual bool Delete(Button *button);
    virtual void Clear();                     // Delete all buttons and clear
    virtual size_t ButtonCount() const { return mButtonVec.size(); }
    virtual Button *Button(size_t i) { return mButtonVec[i]; }
    virtual void Sort(const CompareButton &compare);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual size_t ImageWidth() const { return Width(); }
    virtual size_t ImageHeight() const;
    virtual bool Touch(const tui::Event &event);
    virtual bool DrawImage(float x0, float y0, float x1, float y1,
                           float u0, float v0, float u1, float v1);

  private:
    std::vector<tui::Button *> mButtonVec;    // Button grid
    float mMVPBuf[16];                        // Current view transform
    int mButtonHorizCount[2];                 // Wide & narrow counts
    int mButtonHorizCountIdx;                 // Wide or narrow cur count
    int mButtonDim;                           // Size in pixels
    int mButtonPad;                           // Pixels between buttons
    int mTopPad, mBottomPad;                  // Pixels above and below
  };
  
  
} // namespace tui

#endif  // TOUCH_UI_H_
