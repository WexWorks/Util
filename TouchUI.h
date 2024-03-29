//  Copyright (c) 2012 The 11ers, LLC. All Rights Reserved.

#ifndef TOUCH_UI_H_
#define TOUCH_UI_H_

#include <assert.h>
#include <map>
#include <math.h>
#include <vector>
#include <string>

namespace glt { struct Font; struct FontSet; };


/*
 The Touch User Interface library provides a lightweight,
 noninvasive widget collection with OpenGL-ES rendering.
 
 Widgets are designed to work with touch-based interfaces and include
 common widgets found on mobile platforms such as fling-able lists,
 and buttons, and includes support for animated and user-derived
 widgets.
 
 TouchUI does *not* own the window system interactions, but is instead
 designed to live inside application-created external OpenGL
 windows/contexts/surfaces, allowing it to be used with a variety of
 window-system toolkits, or called from platform-specific toolkits
 such as UIKit, Android, GLUT or custom game engines frameworks.
 */

namespace tui {

  // Touch finite state machine stage
  enum EventPhase { TOUCH_BEGAN, TOUCH_MOVED, TOUCH_ENDED, TOUCH_CANCELLED };
  
  class Widget;                               // Forward decl
  
  // Finite state machine holding multi-touch event state passed from OS to App
  struct Event {
    struct Touch {                            // One finger
      Touch(size_t id, int x, int y, double timestamp)
        : id(id), x(x), y(y), timestamp(timestamp) {}
      size_t id;                              // Unique descriptor per touch
      int x, y;                               // Pixel location of this touch
      double timestamp;                       // Event time
    };
    
    Event(EventPhase phase) : phase(phase) {}
    
    void Init(EventPhase phase);              // Call before adding touches
    void AddTouch(size_t id, int x, int y, double timestamp) {
      touchVec.push_back(Touch(id, x, y, timestamp));
    }
    void PrepareToSend();                     // Call after adding touches
    
    void OnTouchBegan(Widget *widget) const;  // Call widget OnTouchBegin
    void OnTouchEnded(Widget *widget) const;  // Call widget OnTouchEnd
    
    const float *StartCentroid() const { return mStartCentroid; }
    const float *CurCentroid() const { return mCurCentroid; }
    const float *Pan() const { return mPan; }
    float StartRadius() const { return mStartRadius; }
    float CurRadius() const { return mCurRadius; }
    float Scale() const { return mScale; }
    size_t TouchCount() const { return touchVec.size(); }
    size_t ActiveTouchCount() const { return mCurTouchVec.size(); }
    const Touch &StartTouch(size_t idx) const { return mStartTouchVec[idx]; }
    bool IsDone() const {
      // FIXME: Figure out event patterns!
//      assert(mEndTouchVec.size() <= ActiveTouchCount());
      return ActiveTouchCount() - int(mEndTouchVec.size()) == 0;
    }
    void Print() const;
    
    // FIXME: Make these two instance variables private!
    EventPhase phase;                         // FSM location
    std::vector<Touch> touchVec;              // List of touches
    
  private:
    std::vector<Touch> mStartTouchVec;        // Starting touches
    std::vector<Touch> mCurTouchVec;          // Current touches
    std::vector<Touch> mBeginTouchVec;        // New touches in this event
    std::vector<Touch> mEndTouchVec;          // Finished touches in this event
    float mStartCentroid[2];                  // Centroid of start touches
    float mCurCentroid[2];                    // Centroid of current touches
    float mPan[2];                            // Delta of cur and start centroid
    float mStartRadius;                       // Avg radius for start touches
    float mCurRadius;                         // Avg radius for current touches
    float mScale;                             // Ratio of cur to start radius
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
      if (!Enabled() || Hidden()) return false;
      return ProcessGestures(event);
    }
    
    // Track touch events and call the Gesture event callbacks
    virtual bool ProcessGestures(const Event &event);
    
    // Gesture callbacks invoked by ProcessGestures. Return true if consumed.
    virtual bool OnScale(EventPhase phase, float scale, float x, float y,
                         double timestamp) { return false; }
    virtual bool OnDrag(EventPhase phase, float x, float y,
                        double timestamp) { return false; }
    virtual void OnTouchBegan(const Event::Touch &touch) {}
    virtual void OnTouchEnded(const Event::Touch &touch) {}
    virtual bool IsScaling() const { return mIsScaling; }
    virtual bool IsDragging() const { return mIsDragging; }
    virtual bool IsHorizontalDrag() const { return mIsHorizontalDrag; }
    
    // Enable state indicates whether Touch events are processed
    virtual bool Enabled() const { return mIsEnabled; }
    virtual void Enable(bool status) { mIsEnabled = status; }
    
    // Hidden state controls whether Draw methods are invoked
    virtual bool Hidden() const { return mIsHidden; }
    virtual void Hide(bool status) { mIsHidden = status; }
    
    virtual void SetMVP(const float *mvp) { mMVP = mvp; }
    virtual const float *MVP() const { return mMVP; }
    virtual void GetNDCRect(float *x0, float *y0, float *x1, float *y1) const {
      *x0 = *y0 = -1; *x1 = *y1 = 1;
    }
    
  protected:
    virtual bool TouchStartInside(const Event &event) const { return true; }

  private:
    static const float kMinScale;             // Min scaling amount before event
    static const int kMinPanPix;              // Min pan motion before event

    Widget(const Widget&);                    // Disallow copy ctor
    void operator=(const Widget&);            // Disallow assignment
    
    bool mIsEnabled;                          // Stop processing events
    bool mIsHidden;                           // Stop drawing
    bool mIsScaling;                          // True if processing scale event
    bool mIsDragging;                         // True if processing pan event
    bool mIsHorizontalDrag;                   // True if drag started horiz
    bool mIsCanceled;                         // True when widget is canceled
    const float *mMVP;
  };
  
  
  // Base class for all rectangular widgets
  class ViewportWidget : public Widget {
  public:
    static void SetDefaultCancelPad(int pad) { sDefaultCancelPad = pad; }
    
    ViewportWidget() : mCancelPad(sDefaultCancelPad), mEventOpaque(false),
                       mLastTapTimestamp(0) {
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
    bool Inside(int x, int y, int pad = 0) const {
      return x >= Left()   - pad && x <= Right() + pad &&
             y >= Bottom() - pad && y <= Top()   + pad;
    }
    bool Intersect(int x, int y, int w, int h) const {
      return !(Left() > x+w || Right() < x) && !(Bottom() > y+h || Top() < y);
    }
    virtual void SetCancelPad(int pad) { mCancelPad = pad; }
    virtual void SetEventOpaque(bool status) { mEventOpaque = status; }
    virtual void GetNDCRect(float *x0, float *y0, float *x1, float *y1) const {
      if (MVP()) {
        *x0 = Left(); *y0 = Bottom(); *x1 = Right(); *y1 = Top();
      } else {
        Widget::GetNDCRect(x0, y0, x1, y1);
      }
    }

    virtual bool OnTouchTap(const Event::Touch &touch) { return false; }
    virtual bool OnDoubleTap(const Event::Touch &touch) { return false; }
    virtual bool ProcessGestures(const Event &event);
    
  protected:
    virtual bool TouchStartInside(const Event &event) const;
    
    int mViewport[4];                         // [x, y, w, h]
    int mCancelPad;                           // Pixel pad around button
    
  private:
    static int sDefaultCancelPad;             // Relax touch-up clipping
    static const float kDoubleTapSec;         // Threshold for OnDoubleTap
    
    int mEventOpaque;                         // Consume event that start inside
    float mLastTapTimestamp;                  // Signal OnDoubleTap
  };
  
  
  // Base class for any animated widgets
  class AnimatedViewport : public ViewportWidget {
  public:
    AnimatedViewport() {}
    virtual ~AnimatedViewport() {}
    virtual bool Step(float seconds) = 0;     // Update animated components
    virtual bool Dormant() const = 0;         // True if Step can be skipped
  };
  
  
  // Progress bar
  class ProgressBar : public AnimatedViewport {
  public:
    ProgressBar() : mValue(0), mSeconds(0), mCoreTex(0), mShellTex(0) {
      mRange[0] = 0; mRange[1] = 100;
      mRGBA[0] = 0.3; mRGBA[1] = 0.7; mRGBA[2] = 1; mRGBA[3] = 1;
      mTexDim[0] = mTexDim[1] = 0;
    }
    virtual void SetTextures(unsigned int coreTex, unsigned int shellTex,
                             size_t w, size_t h) {
      mCoreTex = coreTex; mShellTex = shellTex; mTexDim[0] = w; mTexDim[1] = h;
    }
    virtual bool SetRange(float min, float max);
    virtual void SetValue(float value) { mValue = value; }
    virtual float Value() const { return mValue; }
    virtual void SetColor(float r, float g, float b, float a) {
      mRGBA[0] = r; mRGBA[1] = g; mRGBA[2] = b; mRGBA[3] = a;
    }
    virtual bool Draw();
    virtual bool Step(float seconds) { mSeconds += seconds; return true; }
    virtual bool Dormant() const { return Hidden(); }
    
  private:
    float mRange[2];                          // [min, max] value
    float mValue;                             // Current value
    float mRGBA[4];                           // Value color
    float mSeconds;
    unsigned int mCoreTex, mShellTex;         // Inner/outer textures
    size_t mTexDim[2];                        // Size of both textures
  };
  
  
  // Text label, display only, with optional animated fade
  class Label : public AnimatedViewport {
  public:
    Label() : mText(NULL), mFont(NULL), mPts(0), mOpacity(1), mAlign(1),
              mLineCount(0), mTex(0), mWrapLines(true),
              mTimeoutSec(0), mFadeSec(0), mRemainingSec(0) {
      mTextColor[0] = mTextColor[1] = mTextColor[2] = mTextColor[3] = 1;
      mTextRange[0] = 0; mTextRange[1] = -1;
      mTextDropshadowOffsetPts[0] = 0; mTextDropshadowOffsetPts[1] = 0;
      mTextDropshadowColor[0] = mTextDropshadowColor[1] = 0;
      mTextDropshadowColor[2] = mTextDropshadowColor[3] = 0;
      mBkgTexColor[0] = mBkgTexColor[1] = mBkgTexColor[2] = mBkgTexColor[3] = 1;
      mTexDim[0] = mTexDim[1] = 0;
      mPadPt[0] = mPadPt[1] = 0;
    }
    virtual ~Label();
    virtual bool Init(const char *text, float pts, const char *font = NULL);
    virtual bool SetText(const char *text, float pts, const char *font = NULL);
    virtual void SetJustify(int glesUtilAlign) { mAlign = glesUtilAlign; }
    virtual bool FitViewport();
    virtual void SetMVP(const float *mvp);
    virtual void SetTextRange(int firstChar, int lastChar) {
      mTextRange[0] = firstChar; mTextRange[1] = lastChar;
    }
    virtual void SetTextColor(float r, float g, float b, float a) {
      mTextColor[0] = r; mTextColor[1] = g; mTextColor[2] = b; mTextColor[3] =a;
    }
    virtual void SetBackgroundTexColor(float r, float g, float b, float a) {
      mBkgTexColor[0]=r; mBkgTexColor[1]=g; mBkgTexColor[2]=b;mBkgTexColor[3]=a;
    }
    virtual void SetBackgroundTex(unsigned long tex, int wPts, int hPts) {
      mTexDim[0] = wPts; mTexDim[1] = hPts; mTex = tex;
    }
    virtual void SetDropshadow(float r, float g, float b, float a,
                               float xOffsetPts, float yOffsetPts) {
      mTextDropshadowColor[0] = r; mTextDropshadowColor[1] = g;
      mTextDropshadowColor[2] = b; mTextDropshadowColor[3] = a;
      mTextDropshadowOffsetPts[0] = xOffsetPts;
      mTextDropshadowOffsetPts[1] = yOffsetPts;
    }
    virtual void SetFade(float timeoutSec, float fadeSec) {
      mTimeoutSec = mRemainingSec = timeoutSec; mFadeSec = fadeSec;
    }
    virtual void SetOpacity(float opacity) { mOpacity = opacity; }
    virtual void SetViewportPad(float xPts, float yPts) {
      mPadPt[0] = xPts; mPadPt[1] = yPts;
    }
    virtual void SetWrapLines(bool status) { mWrapLines = status; }
    virtual float BackgroundPadXPts() const { return mPadPt[0]; }
    virtual float BackgroundPadYPts() const { return mPadPt[1]; }
    virtual int TextLineCount() const { return mLineCount; }
    virtual const glt::Font &Font() const { return *mFont; }
    virtual float Points() const { return mPts; }
    virtual const char *Text() const { return mText; }
    
    virtual bool Step(float seconds);         // Only if fading
    virtual bool Dormant() const { return mTimeoutSec == 0 || Hidden(); }
    virtual bool Draw();
    
    static bool AddFontSet(const char *name, const glt::FontSet &fontSet);
    
  private:
    static std::map<std::string, const glt::FontSet *> sFontMap;
    
    const char *mText;                        // Label text
    const glt::Font *mFont;                   // Rendering font
    float mPts;                               // Font size
    float mTextColor[4], mBkgTexColor[4];     // Text and texture colors
    float mTextDropshadowColor[4];            // Optional dropshadow color
    float mTextDropshadowOffsetPts[2];        // Zero == off (default is 0)
    float mOpacity;                           // Multiplies other colors
    int mAlign;                               // Paragraph alignment
    int mTextRange[2];                        // Character subrange
    int mLineCount;                           // Lines of text, ie. '\n'
    unsigned long mTex;                       // Background texture
    int mTexDim[2];                           // Size of bkgrnd tex
    float mPadPt[2];                          // Pad around text
    bool mWrapLines;                          // Wrap text?
    float mTimeoutSec,mFadeSec,mRemainingSec; // Fade timeout (default is off)
  };
  
  
  // Fading label inside a transparent box
  class InfoBox : public Label {
  public:
    InfoBox() {}
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual bool SetText(const char *text, float pts, const char *font = NULL);

  private:
    static const int kTimeoutSec;                         // Total visible time
    static const float kFadeSec;                          // Fade in & out time
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
  
  
  // Spinning activity monitor used to indicate actions of unknown duration
  class Spinner : public AnimatedViewport {
  public:
    Spinner() : mTex(0), mIncAngle(0), mIncSec(0), mIsAnimating(false),
                mAngle(0), mLastUpdateSec(0) {}
    virtual bool Init(unsigned int texture, float incAngle, float incMinSec);
    virtual bool Step(float seconds);
    virtual bool Draw();
    virtual bool Touch(const Event &event) { return false; }
    virtual bool Dormant() const { return IsAnimating(); }
    virtual void Animate(bool status) { mIsAnimating = status; }
    virtual bool IsAnimating() const { return mIsAnimating; }
    
  private:
    unsigned int mTex;                        // Texture to rotate & display
    float mIncAngle, mIncSec;                 // Amount to increment and when
    bool mIsAnimating;                        // True when spinning
    float mAngle, mLastUpdateSec;             // Current angle and sec
  };
  
  
  // Image sequence played back at a specified rate
  class Flipbook : public AnimatedViewport {
  public:
    Flipbook() : mFPS(0), mIsAnimating(false), mFrameSec(0), mFrameIdx(0) {}
    virtual bool Init(size_t count, const unsigned int *tex);
    virtual bool Step(float seconds);
    virtual bool Draw();
    virtual bool Touch(const Event &event) { return false; }
    virtual bool Dormant() const { return IsAnimating(); }
    virtual void Animate(bool status) { mIsAnimating = status; }
    virtual bool IsAnimating() const { return mIsAnimating; }
    virtual void SetFrameRate(float fps) { mFPS = fps; }
    virtual float FrameRate() const { return mFPS; }

  private:
    std::vector<unsigned int> mTexVec;        // Image texture sequence
    float mFPS;                               // Frames / sec playback rate
    bool mIsAnimating;                        // True when flipping
    float mFrameSec;                          // Seconds on current frame
    size_t mFrameIdx;                         // Current index in mTexVec
  };


  //
  // Buttons
  //
  
  // Button touch behavior, invokes OnTouchTap when activated
  class Button : public ViewportWidget {
  public:
    Button() {}
    virtual ~Button() {}
    
    virtual bool Touch(const Event &event);
    virtual bool Pressed() const;
    virtual bool WasPressed() const { return !mPressVec.empty(); }
    virtual bool Canceled() const { return !Pressed() && WasPressed(); }
    virtual void AverageTouchPosition(double *x, double *y) const;
    
  protected:
    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      return OnTouchTap(touch);
    }

  private:
    struct Press {
      Press(size_t id, bool pressed, int x, int y) :
      id(id), pressed(pressed), x(x), y(y) {}
      size_t id;                              // Touch id
      bool pressed;                           // True if currently pressed
      int x, y;                               // Press location
    };
    
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
  
  
  // Checkbox button that toggles on touch down with long-press to lock
  class ToggleLockCheckbox : public CheckboxImageButton {
  public:
    virtual void OnTouchBegan(const Event::Touch &touch) {
      mToggleStartTimestamp = touch.timestamp;
      SetSelected(true);
    }

  protected:
    static const double kLongTouchSec = 2;
    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      double dt = touch.timestamp - mToggleStartTimestamp;
      if (dt < kLongTouchSec)
        SetSelected(false);
      return Button::InvokeTouchTap(touch);
    }
    
  private:
    double mToggleStartTimestamp;
  };
  
  
  // Button drawn by extending (scaling middle, but not edges) of the
  // background textures with a string centered on top
  class TextButton : public Button {
  public:
    TextButton() : mDefaultTex(0), mPressedTex(0) {
      memset(mDim, 0, sizeof(mDim));
    }
    virtual bool Init(const char *text, float pts, size_t w, size_t h,
                      unsigned int defaultTex, unsigned int pressedTex,
                      const char *font = NULL, float padX=-1, float padY=-1);
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void Enable(bool status) {
      Button::Enable(status); mLabel.Enable(status);
    }
    virtual void SetText(const char *text) {
      mLabel.SetText(text, mLabel.Points());
    }
    virtual void SetTextColor(float r, float g, float b, float a) {
      mLabel.SetTextColor(r, g, b, a);
    }
    virtual void SetBackgroundTexColor(float r, float g, float b, float a) {
      mLabel.SetBackgroundTexColor(r, g, b, a);
    }
    virtual void SetDropshadow(float r, float g, float b, float a,
                               float xOffsetPts, float yOffsetPts) {
      mLabel.SetDropshadow(r, g, b, a, xOffsetPts, yOffsetPts);
    }
    virtual void SetViewportPad(float xPts, float yPts) {
      mLabel.SetViewportPad(xPts, yPts);
    }
    virtual void SetWrapLines(bool status) { mLabel.SetWrapLines(status); }
    virtual const char *Text() const { return mLabel.Text(); }
    virtual bool Draw();
    
  protected:
    Label mLabel;
    size_t mDim[2];
    unsigned int mDefaultTex, mPressedTex;
  };
  
  
  // Checkbox button with extended background and foreground text
  class TextCheckbox : public CheckboxButton {
  public:
    TextCheckbox() : mDeselectedTex(0), mPressedTex(0), mSelectedTex(0) {}
    virtual bool Init(const char *text, float pts, size_t w, size_t h,
                      unsigned int deselectedTex, unsigned int pressedTex,
                      unsigned int selectedTex, const char *font = NULL,
                      float padX=-1, float padY=-1);
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void Enable(bool status) {
      CheckboxButton::Enable(status); mLabel.Enable(status);
    }
    virtual void SetTextColor(float r, float g, float b, float a) {
      mLabel.SetTextColor(r, g, b, a);
    }
    virtual void SetBackgroundTexColor(float r, float g, float b, float a) {
      mLabel.SetBackgroundTexColor(r, g, b, a);
    }
    virtual void SetDropshadow(float r, float g, float b, float a,
                               float xOffsetPts, float yOffsetPts) {
      mLabel.SetDropshadow(r, g, b, a, xOffsetPts, yOffsetPts);
    }
    virtual void SetViewportPad(float xPts, float yPts) {
      mLabel.SetViewportPad(xPts, yPts);
    }
    virtual void SetWrapLines(bool status) { mLabel.SetWrapLines(status); }
    virtual const char *Text() const { return mLabel.Text(); }
    virtual bool Draw();
    
  protected:
    Label mLabel;
    size_t mDim[2];
    unsigned int mDeselectedTex, mPressedTex, mSelectedTex;
  };
  
  
  // Manages a set of CheckboxButtons, ensuring that only a single
  // checkbox of the group, or none, is selected at any given time.
  // NOTE: Make sure the checkbox buttons return true from OnTouchTap
  //       or the radio button will not deselect the other buttons.
  // NOTE: In CheckboxButton::OnTouchTap, the radio button state does
  //       not yet account for the newly pressed button.
  class RadioButton : public ViewportWidget {
  public:
    RadioButton() : mIsNoneAllowed(false) {}
    virtual ~RadioButton();
    virtual void Add(CheckboxButton *button);
    virtual void Clear();
    virtual void Destroy();
    virtual size_t Count() const { return mButtonVec.size(); }
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual bool Touch(const Event &event);
    virtual bool Draw();
    virtual void Hide(bool status);
    virtual CheckboxButton *Selected() const;
    virtual CheckboxButton *Button(int i) { return mButtonVec[i]; }
    virtual int SelectedIdx() const;
    virtual void SetSelected(CheckboxButton *button);
    virtual void SetSelectedIdx(int idx) { SetSelected(mButtonVec[idx]);}
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
    Slider() : mHandle(NULL), mSliderTex(0), mHandleT(0) {
      mBkgTexColor[0] = mBkgTexColor[1] = mBkgTexColor[2] = mBkgTexColor[3] = 1;
    }
    virtual ~Slider();
    
    virtual bool Init(unsigned int sliderTex, Handle *handle);
    virtual bool Init(unsigned int sliderTex, size_t handleW, size_t handleH,
                      unsigned int handleTex, unsigned int handlePressedTex);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void SetBackgroundTexColor(float r, float g, float b, float a) {
      mBkgTexColor[0]=r; mBkgTexColor[1]=g; mBkgTexColor[2]=b;mBkgTexColor[3]=a;
    }
    virtual bool Touch(const Event &event);
    virtual bool Draw();
    virtual bool SetValue(float value);
    virtual float Value() const { return mHandleT; }
    virtual bool OnValueChanged(float value) { return false; }
    virtual const class Handle *Handle() const { return mHandle; }
    
  private:
    class Handle *mHandle;
    unsigned int mSliderTex;
    float mHandleT;
    float mBkgTexColor[4];                      // Texture multiplier
  };
  
  
  // Star-rating widget
  // Override OnTouchTap to get value changed on up event.
  class StarRating : public Button {
  public:
    StarRating() : mStarCount(0), mValue(0) {
      mTextColor[0] = mTextColor[1] = mTextColor[2] = mTextColor[3] = 0;
      mSelectedColor[0]=mSelectedColor[1]=mSelectedColor[2]=mSelectedColor[3]=0;
    }
    virtual bool Init(size_t count, float pts, const char *font = NULL);
    virtual bool FitViewport();
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetDefaultColor(float r, float g, float b, float a) {
      mTextColor[0] = r; mTextColor[1] = g; mTextColor[2] = b; mTextColor[3] =a;
    }
    virtual void SetSelectedColor(float r, float g, float b, float a) {
      mSelectedColor[0] = r; mSelectedColor[1] = g;
      mSelectedColor[2] = b; mSelectedColor[3] = a;
    }
    virtual bool Draw();
    virtual bool OnDrag(EventPhase phase, float x, float y, double timestamp);
    virtual void SetMVP(const float *mvp) {
      Button::SetMVP(mvp); mLabel.SetMVP(mvp);
    }
    virtual void Enable(bool status) {
      Button::Enable(status); mLabel.Enable(status);
    }
    virtual bool SetValue(int value);
    virtual int Value() const { return mValue; }
    
  private:
    virtual void ComputeDragValue(float x) {
      mDragValue = roundf(0.25 + (mStarCount * (x - Left())) / float(Width()));
      mDragValue = std::min(mDragValue, int(mStarCount));
      mDragValue = std::max(mDragValue, 0);
    }
    virtual bool InvokeTouchTap(const Event::Touch &touch) {
      ComputeDragValue(touch.x);
      SetValue(mDragValue);
      return Button::InvokeTouchTap(touch);
    }

    size_t mStarCount;                        // Number of stars
    int mValue, mDragValue;                   // Current rating
    float mTextColor[4], mSelectedColor[4];   // Text colors
    Label mLabel;                             // Draw
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
  
  
  // A set of horizontally arranged widgets over a stretched background tex.
  // Add widgets from left to right, with spacers interspersed, and the
  // widgets will be automatically adjusted on rotation. Use ViewportWidget
  // for fixed spacing (flexible spacer is a ViewportWidget with negative size).
  // Set the size of each widget for spacing, but the (x,y) is ignored.
  class Toolbar : public AnimatedViewport {
  public:
    static const size_t kStdHeight;
    
    Toolbar() : mBackgroundTex(0) {
      mBackgroundTexDim[0] = mBackgroundTexDim[1] = 0;
    }
    virtual void SetBackgroundTex(unsigned int tex, int w, int h) {
      mBackgroundTex = tex; mBackgroundTexDim[0] = w; mBackgroundTexDim[1] = h;
    }
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual void Add(ViewportWidget *widget) { mWidgetVec.push_back(widget); }
    virtual bool AddFixedSpacer(int w);
    virtual bool AddFlexibleSpacer();
    virtual void Clear();
    virtual bool Touch(const Event &event);
    virtual bool Step(float seconds);
    virtual bool Dormant() const;
    virtual bool Draw();
    
  private:
    unsigned int mBackgroundTex;
    unsigned int mBackgroundTexDim[2];
    std::vector<ViewportWidget *> mWidgetVec;
  };
  
  
  //
  // Frames
  //
  
  // A horizontal or vertical scrollable list of clickable frames
  class FlinglistImpl : public AnimatedViewport {
  public:
    class Frame {
    public:
      virtual ~Frame() {}
      virtual bool Draw() = 0;                // Viewport set prior to call
      virtual bool OnTouchTap(const Event::Touch &touch) { return false; }
      virtual bool OnLongTouch(int x, int y) { return false; }
      virtual void OnTouchBegan(const Event::Touch &touch) {};
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
                      mLongPressSeconds(0), mLongPressTimeout(1),
                      mSingleFrameFling(false), mIsLocked(false) {
      mTouchStart[0] = mTouchStart[1] = 0;
      mOverpullDim[0] = mOverpullDim[1] = 0;
      mOverpullColor[0] = 0; mOverpullColor[1] = 0.75;
      mOverpullColor[2] = 1; mOverpullColor[3] = 0.25;
      mDragHandleDim[0] = mDragHandleDim[1] = 0;
    }
    virtual ~FlinglistImpl() {}
    virtual bool Init(bool vertical, float pixelsPerCm);
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetFrameDim(int dim);
    virtual void SetSnapToCenter(bool status) { mSnapToCenter = status; }
    virtual void SetSingleFrameFling(bool status) { mSingleFrameFling = status;}
    virtual bool SetDragHandle(unsigned int texture, size_t w, size_t h);
    virtual bool SetOverpull(unsigned int offTex, unsigned int onTex,
                             size_t w, size_t h);
    virtual void SetOverpullColor(float r, float g, float b, float a) {
      mOverpullColor[0] = r; mOverpullColor[1] = g;
      mOverpullColor[2] = b; mOverpullColor[3] = a;
    }
    virtual void SetScrollableDim(int dim) { mScrollableDim = dim; }
    virtual void SetLongPressTimeout(float sec) { mLongPressTimeout = sec; }
    virtual void GlowDragHandle(bool status) { mGlowDragHandle = status; }
    virtual bool Clear();
    virtual bool Draw();
    virtual bool Step(float seconds);
    virtual bool Dormant() const;
    virtual bool Snap(const Frame *frame, float seconds);
    virtual void SnapIdx(size_t idx, float seconds);
    virtual bool CancelSnap();
    virtual bool Jiggle();
    virtual void Lock(bool status) { mIsLocked = status; }
    virtual bool IsLocked() const { return mIsLocked; }
    virtual bool Touch(const Event &event);
    virtual size_t Size() const { return mFrameVec.size(); }
    virtual void SetViewportInset(float inset) { mViewportInset = inset; }
    virtual bool Viewport(const Frame *frame, int viewport[4]) const;
    virtual void OverpullViewport(int viewport[4]) const;
    virtual bool VisibleFrameRange(int *min, int *max) const;
    virtual int FrameIdx(const Frame *frame) const;
    virtual size_t FrameCount() const { return mFrameVec.size(); }
    virtual int ScrollDistance(const Frame *frame) const;
    virtual const Frame *operator[](size_t i) const { return mFrameVec[i]; }
    virtual Frame *operator[](size_t i) { return mFrameVec[i]; }
    
    virtual void OnOverpullRelease() {}
    virtual bool OnLongTouch(int x, int y) { return false; }
    virtual void OnMove() {}
    virtual bool OnOffAxisMove(const Event::Touch &touch,
                               int x, int y) { return false; }
    virtual void OnTouchEnded() {}

  protected:
    static const int kDragMm;                 // Threshold for drag events
    static const int kJiggleMm;               // Offset
    static const int kSnapVelocity;           // In pixels/sec
    static const float kJiggleSeconds;        // Length of jiggle

    virtual bool Append(Frame *frame);        // Do not allow generic frames,
    virtual bool Prepend(Frame *frame);       //   use templates instead.
    virtual bool Delete(Frame *frame);
    virtual bool DrawFrame(Frame *frame);     // Set viewport and draw
    int FindFrameIdx(int x, int y) const;
    int TotalHeight() const { return mFrameDim * mFrameVec.size(); }
    int ScrollMin() const { const int dim = mVertical ? Height() : Width();
      return dim / 2 + mScrollableDim / 2 - dim + mSnapLocationOffset; }
    int ScrollMax() const {
      int x = ScrollMin() + Size() * mFrameDim - mScrollableDim;
      return x < ScrollMin() ? ScrollMin() : x;
    }
    int ScrollToOffset(int i) const { return i * mFrameDim + ScrollMin(); }
    virtual int FlingingSnapIdx() const;
    virtual bool MovedAfterDown() const {
      return mMovedAfterDown || mMovedOffAxisAfterDown; }
    virtual int OverpullPixels() const {
      return -0.15 * std::max(Width(), Height()); }
    virtual void OverpullNDCRange(float *x0, float *y0,
                                  float *x1, float *y1) const;
    virtual void ClampScrollableDim();
    
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
    float mOverpullColor[4];                  // Defaults to transparent blue
    size_t mOverpullDim[2];                   // Size of overpull texture
    unsigned int mOverpullOffTex;             // Overpull inactive texture
    unsigned int mOverpullOnTex;              // Overpull active texture
    size_t mDragHandleDim[2];                 // Size of drag handle texture
    unsigned int mDragHandleTex;              // Drag handle texture
    bool mGlowDragHandle;                     // Pulsating glow if true
    float mGlowSeconds;                       // Track for pulsating glow
    float mLongPressSeconds;                  // Seconds since not moved
    float mLongPressTimeout;                  // Seconds to trigger long press
    bool mSingleFrameFling;                   // Fling interaction mode
    bool mIsLocked;                           // Disable sliding only
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
  
  
  // Flinglist that only moves one frame at a time with a recticle selector.
  // Optionally support for pinch & zoom via an embedded tui::Frame for the
  // selected filmstrip frame. The tui::Frame stays centered over the filmstrip,
  // controlling motion until the end of each touch sequence when a decision
  // is made to either scroll to the next frame, or allow the tui::Frame to
  // restore to the rest location. The filmstrip in the background is moved
  // to align with the overpull borders of the frame as it is moved. The active
  // frame is drawn inside the tui::Frame's static viewport, while all other
  // frames are drawn in the flinglist viewports. This allows the tui::Frame
  // event processing to proceed without any viewport adjustments. When active,
  // Frame::Draw will be called instead of FlinglistImpl::Frame::Draw.
  
  class Frame;                                // Forward decl for P&Z in strip
  
  class FilmstripImpl : public FlinglistImpl {
  public:
    FilmstripImpl() : mSelectedFrameIdx(-1), mPZFrame(NULL), mIsPZEvent(false),
                      mPZStartScrollOffset(0) {}
    virtual ~FilmstripImpl() {}
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual bool Touch(const Event &event);
    virtual bool Step(float seconds);
    virtual bool Dormant() const;
    virtual void OnSelectionChanged() {}

    // Optional support for P&Z for the selected frame.
    virtual void SetPinchAndZoom(class tui::Frame *frame);
    virtual tui::Frame *Frame() { return mPZFrame; }
    
  protected:
    virtual bool Prepend(FlinglistImpl::Frame *frame);
    virtual bool Delete(FlinglistImpl::Frame *frame);
    virtual void SnapIdx(size_t idx, float seconds);
    virtual bool DrawFrame(FlinglistImpl::Frame *frame);
    
    int mSelectedFrameIdx;                    // Selected (and centered) frame
    class tui::Frame *mPZFrame;               // Pinch & Zoom inside cur frame
    bool mIsPZEvent;
    float mPZStartScrollOffset;
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
    Frame() : mIsScaleLocked(false), mSnapMode(SNAP_CENTER),
              mScale(1), mScaleVelocity(0),
              mStartScale(0), mPrevScale(0),
              mPrevScaleTimestamp(0), mPrevDragTimestamp(0),
              mTargetScale(0),
              mIsTargetScaleActive(false),
              mIsTargetScaleCenterActive(false),
              mIsTargetCenterActive(false),
              mIsDirty(false),
              mScaleMin(0), mScaleMax(0),
              mViewportMinScalePad(0),
              mIsSnapDirty(false),
              mOverpullDeceleration(0.5) {
      memset(mDim, 0, sizeof(mDim));
      memset(mIsLocked, 0, sizeof(mIsLocked));
      memset(mCenterUV, 0, sizeof(mCenterUV));
      memset(mStartCenterUV, 0, sizeof(mStartCenterUV));
      memset(mPrevDragXY, 0, sizeof(mPrevDragXY));
      memset(mCenterVelocityUV, 0, sizeof(mCenterVelocityUV));
      memset(mTargetCenterUV, 0, sizeof(mTargetCenterUV));
      mSnapNDCRect[0]=mSnapNDCRect[1] = -1; mSnapNDCRect[2]=mSnapNDCRect[3] = 1;
    }
    virtual ~Frame() {}

    virtual void SetImageDim(size_t w, size_t h) { mDim[0] = w; mDim[1] = h; }
    virtual size_t ImageWidth() const { return mDim[0]; }
    virtual size_t ImageHeight() const { return mDim[1]; }
    virtual bool IsAnimating() const { return mIsTargetScaleActive ||
      mIsTargetScaleCenterActive || mIsTargetCenterActive || mScaleVelocity!=0||
      mCenterVelocityUV[0] != 0 || mCenterVelocityUV[1] != 0; }
    virtual void Lock(bool horizontal, bool vertical, bool scale) {
      mIsLocked[0] = horizontal; mIsLocked[1] = vertical; mIsScaleLocked =scale;
    }
    virtual bool IsXLocked() const { return mIsLocked[0]; }
    virtual bool IsYLocked() const { return mIsLocked[1]; }
    virtual float UCenter() const { return mCenterUV[0]; }
    virtual float VCenter() const { return mCenterUV[1]; }
    virtual float XOverpull() const;
    virtual float YOverpull() const;
    virtual void SetOverpullDeceleration(float k) { mOverpullDeceleration = k; }
    virtual void SetViewportMinScalePad(int pix) { mViewportMinScalePad = pix; }
    virtual int ViewportMinScalePad() const { return mViewportMinScalePad; }
    virtual void SetSnapModeCenter() { mSnapMode = SNAP_CENTER; }
    virtual void SetSnapModeUpperLeft() { mSnapMode = SNAP_UPPER_LEFT; }
    virtual void SetSnapModePixelCenter() { mSnapMode = SNAP_PIXEL; }
    virtual void SetSnapModeNDCRect(float u0, float v0, float u1, float v1) {
      mSnapMode = SNAP_NDC_RECT; mSnapNDCRect[0] = u0; mSnapNDCRect[1] = v0;
      mSnapNDCRect[2] = u1; mSnapNDCRect[3] = v1;
    }
    
    // Widget Methods -- Override if you need custom behavior
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual bool Step(float seconds);         // Animate views
    virtual bool Dormant() const;             // True if all views dormant
    virtual float Scale() const { return mScale; }
    virtual float MinScale() const { return mScaleMin; }
    virtual float MaxScale() const { return mScaleMax; }
    virtual float FitScale() const;
    virtual bool OnScale(EventPhase phase, float scale, float x, float y,
                         double timestamp);
    virtual bool OnDrag(EventPhase phase, float x, float y, double timestamp);
    virtual void OnTouchBegan(const Event::Touch &touch);
    virtual void OnTouchEnded(const Event::Touch &touch);
    virtual bool OnDoubleTap(const Event::Touch &touch);
    
    // Frame adjustments
    virtual void SnapToFitFrame(bool isAnimated=false);
    virtual void SnapToFitWidth(float v, bool isAnimated=false);
    virtual void SnapToFitHeight(float u, bool isAnimated=false);
    virtual void SnapToUVCenter(float u, float v, bool isAnimated=false);
    virtual void SnapToScale(float scale, bool isAnimated=false);
    virtual void SnapToLimits(bool isAnimated=false);
    virtual bool IsSnapping() const {
      return mIsTargetScaleActive || mIsTargetScaleCenterActive || mIsTargetCenterActive;
    }
    
    // Compute the current display region that would be sent to DrawImage
    virtual void ComputeDisplayRect(float *x0, float *y0, float *x1, float *y1,
                                    float *u0, float *v0, float *u1, float *v1) const;
    
    // Compute the UV coordinates of a point in ndc coordinates
    void NDCToUV(float x, float y, float *u, float *v);

    // Convert the region in NDC and UV space provided to Frame::Draw into
    // a 4x4 matrix (really 2D, so it could be 3x3), that transforms points
    // in pixel coordinates in the Frame image into NDC for use as a MVP.
    static void RegionToM44f(float dst[16], int imageWidth, int imageHeight,
                             float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1);
    
  private:
    static const float kDragDamping;
    static const float kDragFling;
    static const float kScaleDamping;
    static const float kScaleFling;
    
    Frame(const Frame &);                     // Disallow copy ctor
    void operator=(const Frame &);            // Disallow assignment
    
    bool Reset();
    void ComputeScaleRange();                 // Compute mScaleMin/Max
    void ResetView();                         // Scale=min, uv=0.5
    void CancelMotion();                      // Zero out animation variables
    float U2Ndc(float u) const;
    float V2Ndc(float v) const;
    float Ndc2U(float x) const;
    float Ndc2V(float y) const;
    
    enum SnapMode { SNAP_CENTER, SNAP_UPPER_LEFT, SNAP_PIXEL, SNAP_NDC_RECT };
    
    size_t mDim[2];                           // Image dimensions
    bool mIsLocked[2];                        // Lock horizontal/vertical move
    bool mIsScaleLocked;                      // Lock scaling
    SnapMode mSnapMode;                       // Limit NDC rect
    float mSnapNDCRect[4];                    // [u0, v0, u1, v1] SNAP_NDC_RECT
    
    float mScale, mScaleVelocity;             // Image scale, 1 -> 1 pixel
    float mCenterUV[2], mCenterVelocityUV[2]; // Center of screen in UV 0 -> 1
    float mStartScale, mStartCenterUV[2];     // Values at TOUCH_BEGIN
    float mPrevScale, mPrevDragXY[2];         // For velocity
    double mPrevScaleTimestamp;               // For velocity
    double mPrevDragTimestamp;                // For velocity
    float mTargetScale;                       // Rubberband targets
    bool mIsTargetScaleActive;                // True if we use target scale
    bool mIsTargetScaleCenterActive;          // True if scale adjusts center
    bool mIsTargetCenterActive;               // True if we use target center
    float mTargetCenterUV[2];                 // Window center target
    bool mIsDirty;                            // True if we need to repaint
    float mScaleMin, mScaleMax;               // Valid scale range
    int mViewportMinScalePad;                 // Pad to inset for fit-to-frame
    bool mIsSnapDirty;                        // True after snap for clip
    float mOverpullDeceleration;              // Slow motion on overpull
  };
  
  
  // Implement to compare two buttons for ordering in the ButtonGridFrame::Sort
  struct CompareFunctor {
    virtual ~CompareFunctor() {}
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
    ButtonGridFrame();                        // count<0 -> fixed # row/col
    virtual bool Init(int wideCount, int narrowCount,
                      int buttonPad, int topPad, int bottomPad);
    virtual void Add(Button *button);
    virtual bool Remove(Button *button);      // Remove, but don't delete
    virtual void Destroy();                   // Delete all buttons and clear
    virtual void Clear();                     // Clear list, but don't delete
    virtual size_t ButtonCount() const { return mButtonVec.size(); }
    virtual class Button *Button(size_t i) { return mButtonVec[i]; }
    virtual const class Button *Button(size_t i) const { return mButtonVec[i]; }
    virtual void VisibleButtonRange(float u0, float v0, float u1, float v1,
                                    int *minIdx, int *maxIdx);
    virtual void Sort(const CompareButton &compare);
    virtual bool Snap(size_t i, bool isAnimated=false); // Button #i is visible
    virtual bool SetViewport(int x, int y, int w, int h);
    virtual void SetMVP(const float *mvp);
    virtual bool Touch(const tui::Event &event);
    virtual bool Draw();
    virtual bool Step(float seconds);

  private:
    std::vector<tui::Button *> mButtonVec;    // Button grid
    float mMVPBuf[16];                        // Current view transform
    int mButtonHorizCount[2];                 // Wide & narrow counts
    int mButtonHorizCountIdx;                 // Wide or narrow cur count
    int mButtonDim;                           // Size in pixels
    int mButtonPad;                           // Pixels between buttons
    int mTopPad, mBottomPad;                  // Pixels above and below
    bool mIsViewportDirty;                    // Call SetViewport if true
  };
  
  
} // namespace tui

#endif  // TOUCH_UI_H_
