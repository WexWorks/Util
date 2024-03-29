// Copyright (c) 2011 by The 11ers, LLC -- All Rights Reserved

#include "TouchUI.h"

#include "GlesUtil.h"

#include <ImathMatrix.h>

#include <algorithm>
#include <assert.h>
#include <math.h>


using namespace tui;

const float Widget::kMinScale = 0.03;
const int Widget::kMinPanPix = 40;
const float ViewportWidget::kDoubleTapSec = 0.25;
const int InfoBox::kTimeoutSec = 6;
const float InfoBox::kFadeSec = 0.5;
const size_t Toolbar::kStdHeight = 44;
const int FlinglistImpl::kDragMm = 4;
const int FlinglistImpl::kJiggleMm = 10;
const float FlinglistImpl::kJiggleSeconds = 0.2;
const int FlinglistImpl::kSnapVelocity = 10;
const float Frame::kDragDamping = 0.9;
const float Frame::kDragFling = 2;
const float Frame::kScaleDamping = 0.9;
const float Frame::kScaleFling = 1;

unsigned int FlinglistImpl::mFlingProgram = 0;
unsigned int FlinglistImpl::mGlowProgram = 0;
unsigned int Sprite::mSpriteProgram = 0;
int Sprite::mSpriteAP = 0;
int Sprite::mSpriteAUV = 0;
int Sprite::mSpriteUCTex = 0;
int Sprite::mSpriteUOpacity = 0;


template <class T> inline T clamp (T a, T l, T h){
  return (a < l)? l : ((a > h)? h : a);
}


//
// Event
//

void Event::Init(tui::EventPhase phase) {
  // Now remove any events in the END phase from the active lists
  // so we properly handle transitions from multi- to single-touch.
  for (size_t i = 0; i < mEndTouchVec.size(); ++i) {
    for (size_t j = 0; j < ActiveTouchCount(); ++j) {
      if (mEndTouchVec[i].id == mCurTouchVec[j].id) {
        mCurTouchVec.erase(mCurTouchVec.begin() + j);
        mStartTouchVec.erase(mStartTouchVec.begin() + j);
        break;
      }
    }
  }
  mEndTouchVec.clear();

  this->phase = phase;
  this->touchVec.clear();
}


static float Length(int ax, int ay, int bx, int by) {
  float dx = ax - bx;
  float dy = ay - by;
  return sqrtf(dx * dx + dy * dy);
}


void Event::PrepareToSend() {
  // Touch events can be sent in groups or subsets of the active group.
  // E.g.: Ab, Bb, ABm, ABm, Bm, ABm, Am, Cb, ABCm, ABCm, BCe, Ae
  // for event ids A, B & C, and suffixes b = began, m = moved, and e = ended.
  // We track the starting position for each touch id along with the
  // current position for all active touches. Touches are added to the
  // active set on a 'start' and removed on 'end' or 'cancel'.
  
  // Run through this event's touches, adding new entries to the start
  // and cur tracking vectors and updating existing entries. After this
  // loop, the active set will include all current and ending touches.
  mBeginTouchVec.clear();
  for (size_t i = 0; i < TouchCount(); ++i) {
    const Event::Touch &touch(touchVec[i]);
    
    size_t j = 0;                         // Find entry in active lists
    for (/*EMPTY*/; j < ActiveTouchCount(); ++j) {
      if (touch.id == mStartTouchVec[j].id)
        break;
    }
    if (j < ActiveTouchCount()) {         // Found! Remove or update
      if (phase == TOUCH_BEGAN)           // Happens occasionally
        mStartTouchVec[j] = touch;        // Update start position
      if (phase == TOUCH_MOVED)           // End & Cancel handled below
        mCurTouchVec[j] = touch;          // Update current position
    } else if (phase == TOUCH_BEGAN) {
      assert(mStartTouchVec.size() == mCurTouchVec.size());
      mStartTouchVec.push_back(touch);
      mCurTouchVec.push_back(touch);
      mBeginTouchVec.push_back(touch);
    } else {
      // Moving a canceled point, ignore
    }
  }
  
  // Single touch events are simple and use the position of the one
  // active set member for all actions. Multiple touch events use the
  // centroid of all active touches for panning and the average distance
  // from each touch to the centroid for scaling. Compute the centroid
  // for all active points (including ones that are on their END phase).
  mStartCentroid[0] = mStartCentroid[1] = 0;
  mCurCentroid[0] = 0; mCurCentroid[1] = 0;
  for (size_t i = 0; i < ActiveTouchCount(); ++i) {
    mStartCentroid[0] += mStartTouchVec[i].x;
    mStartCentroid[1] += mStartTouchVec[i].y;
    mCurCentroid[0] += mCurTouchVec[i].x;
    mCurCentroid[1] += mCurTouchVec[i].y;
  }
  mStartCentroid[0] /= ActiveTouchCount();
  mStartCentroid[1] /= ActiveTouchCount();
  mCurCentroid[0] /= ActiveTouchCount();
  mCurCentroid[1] /= ActiveTouchCount();
  
  mPan[0] = mCurCentroid[0] - mStartCentroid[0];
  mPan[1] = mCurCentroid[1] - mStartCentroid[1];
  
  // Compute the average distance from each touch to the centroid for
  // all active points (including ones that are on their END phase).
  mStartRadius = mCurRadius = 0;
  for (size_t i = 0; i < ActiveTouchCount(); ++i) {
    mStartRadius += Length(mStartCentroid[0], mStartCentroid[1],
                           mStartTouchVec[i].x, mStartTouchVec[i].y);
    mCurRadius += Length(mCurCentroid[0], mCurCentroid[1],
                         mCurTouchVec[i].x, mCurTouchVec[i].y);
  }
  mStartRadius /= ActiveTouchCount();
  mCurRadius /= ActiveTouchCount();
  mScale = mCurRadius / mStartRadius;
  
  if (this->phase == TOUCH_ENDED || this->phase == TOUCH_CANCELLED) {
    mEndTouchVec = touchVec;
    // Special case for cancel events that have no touches -- all done!
    if (this->phase == TOUCH_CANCELLED && touchVec.empty())
      mEndTouchVec = mCurTouchVec;
  }
}


void Event::OnTouchBegan(Widget *widget) const {
  for (size_t i = 0; i < mBeginTouchVec.size(); ++i)
    widget->OnTouchBegan(mBeginTouchVec[i]);
}


void Event::OnTouchEnded(Widget *widget) const {
  for (size_t i = 0; i < mEndTouchVec.size(); ++i)
    widget->OnTouchEnded(mEndTouchVec[i]);
}


void Event::Print() const {
  const char *phaseName[4] = { "Began", "Moved", "Ended", "Canceled" };
  printf("%s %zd touches and %zd active: ", phaseName[int(phase)],
         touchVec.size(), ActiveTouchCount());
  for (size_t i = 0; i < touchVec.size(); ++i)
    printf("%zd, ", touchVec[i].id);
  printf("  - starts: ");
  for (size_t i = 0; i < ActiveTouchCount(); ++i)
    printf("%zd, ", mStartTouchVec[i].id);
  printf("\n");
}


//
// Widget
//

// Should be called by all Widgets that support gestures.
// Filter the event stream tracking the supported gestures,
// including scale and pan, invoking callbacks and passing
// coordinates in screen pixel units.

bool Widget::ProcessGestures(const tui::Event &event) {
  bool consumed = false;                            // Return value default
  const float timestamp = event.touchVec.empty() ?0:event.touchVec[0].timestamp;
  
  event.OnTouchBegan(this);                         // Invoke OnTouchBegan
  
  switch (event.phase) {
    case TOUCH_BEGAN:
      if (mIsDragging)
        OnDrag(TOUCH_CANCELLED, 0, 0, 0);
      if (mIsScaling)
        OnDrag(TOUCH_CANCELLED, 0, 0, 0);
      mIsDragging = false;                          // Reset on new touches
      mIsScaling = false;
      mIsCanceled = false;
      break;
    case TOUCH_MOVED:
    case TOUCH_ENDED:     /*FALLTHRU*/
      assert(!event.touchVec.empty());
      if (mIsCanceled) {
        /*EMPTY*/
      } else if (event.ActiveTouchCount() > 1) {    // Multitouch
        // Compute the pan based on the distance between the segment midpoints
        EventPhase phase = event.phase;
        if (!mIsDragging && TouchStartInside(event)) {
          if (fabsf(event.Pan()[0]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = true;
          } else if (fabsf(event.Pan()[1]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = false;
          }
          if (mIsDragging)
            phase = TOUCH_BEGAN;
        }
        if (mIsDragging &&
            OnDrag(phase, event.Pan()[0], event.Pan()[1], timestamp))
            consumed = true;

        // Compute the scale based on the ratio of the segment lengths
        phase = event.phase;                        // Default to current
        if (!mIsScaling && TouchStartInside(event) &&
            (event.Scale() > 1+kMinScale || event.Scale() < 1-kMinScale)) {
          mIsScaling = true;
          phase = TOUCH_BEGAN;                      // First scale event,
        }
        if (mIsScaling && OnScale(phase, event.Scale(), event.CurCentroid()[0],
                                  event.CurCentroid()[1], timestamp))
            consumed = true;
      } else if (event.ActiveTouchCount() == 1) {   // Single-touch
        EventPhase phase = event.phase;
        if (!mIsDragging && TouchStartInside(event)) {
          if (fabsf(event.Pan()[0]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = true;
          } else if (fabsf(event.Pan()[1]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = false;
          }
          phase = TOUCH_BEGAN;
        }
        if (mIsDragging && OnDrag(phase, event.Pan()[0], event.Pan()[1], timestamp))
            consumed = true;
      }
      
      break;
      
    case TOUCH_CANCELLED:
      if (mIsDragging)
        if (OnDrag(TOUCH_CANCELLED, 0, 0, 0))
          consumed = true;
      if (mIsScaling)
        if (OnDrag(TOUCH_CANCELLED, 0, 0, 0))
          consumed = true;
      mIsCanceled = true;
      break;
  }                                                 // switch (event.phase)
  
  event.OnTouchEnded(this);                         // Invoke OnTouchEnded
  
  if (event.IsDone()) {                             // All done!
    mIsDragging = false;
    mIsScaling = false;
    mIsCanceled = false;
  }
  
  return consumed;
}


//
// Viewport
//

int ViewportWidget::sDefaultCancelPad = 35;         // Points, not pixels!


bool ViewportWidget::ProcessGestures(const Event &event) {
  // Handle the viewport events, OnTouchTap and OnDoubleTap
  bool consumed = false;
  if (event.phase == TOUCH_ENDED && !IsDragging() && !IsScaling() &&
      event.ActiveTouchCount() == 1) {
    for (size_t i = 0; !consumed && i < event.touchVec.size(); ++i) {
      const Event::Touch &t(event.touchVec[i]);
      for (size_t j = 0; j < event.ActiveTouchCount(); ++j) {
        if (t.id == event.StartTouch(j).id && Inside(t.x, t.y, mCancelPad) &&
            Inside(event.StartTouch(j).x, event.StartTouch(j).y, mCancelPad)) {
          if (t.timestamp - mLastTapTimestamp < kDoubleTapSec) {
            consumed = OnDoubleTap(t);
          } else {
            mLastTapTimestamp = t.timestamp;
            consumed = OnTouchTap(t);
          }
          break;
        }
      }
    }
  }
  
  if (Widget::ProcessGestures(event) || consumed)
    return true;
  if (mEventOpaque && TouchStartInside(event))      // Hide events behind vp
    return true;
  
  return false;
}


bool ViewportWidget::TouchStartInside(const Event &event) const {
  bool touchInside = false;
  for (size_t i = 0; i < event.ActiveTouchCount(); ++i) {
    if (Inside(event.StartTouch(i).x, event.StartTouch(i).y)) {
      touchInside = true;
      break;
    }
  }
  return touchInside;
}


//
// Label
//

std::map<std::string, const glt::FontSet *> Label::sFontMap;


bool Label::AddFontSet(const char *name, const glt::FontSet &fontSet) {
  std::string key(name);
  if (sFontMap.find(key) != sFontMap.end())
    return false;
  sFontMap[key] = &fontSet;
  return true;
}


Label::~Label() {
  free((void *)mText);
}


bool Label::Init(const char *text, float pts, const char *font) {
  return SetText(text, pts, font);
}


bool Label::SetText(const char *text, float pts, const char *font) {
  free((void *)mText);
  mText = strdup(text);
  if (pts > 0)                                        // Retain if pts==0
    mPts = pts;
  else if (mPts == 0)                                 // Fail if already zero
    return false;
  if (font || !mFont) {
    std::map<std::string, const glt::FontSet *>::const_iterator i;
    if (font == NULL)
      font = "System";
    i = sFontMap.find(font);
    if (i == sFontMap.end())
      return false;
    mFont = &i->second->ClosestFont(mPts);
  }
  size_t len = strlen(mText);
  mLineCount = len > 0 ? 1 : 0;
  for (size_t i = 0; i < len; ++i)
    mLineCount += mText[i] == '\n';
  return true;
}


bool Label::FitViewport() {
  const float ptScale = mPts / float(mFont->charDimPt[0]);
  int tw = ceilf(ptScale * glt::TextWidth(mText, mFont, true /*kern*/));
  int th = ceilf(ptScale * mLineCount * mFont->charDimPt[1]);
  int w = tw + 2 * mPadPt[0];
  int h = th + 2 * mPadPt[1];
  h = std::max(h, mTexDim[1]);
  w = std::max(w, mTexDim[0]);
  w = std::max(w, h);
  
  if (!ViewportWidget::SetViewport(Left(), Bottom(), w, h))
    return false;

  return true;
}


void Label::SetMVP(const float *mvp) {
  ViewportWidget::SetMVP(mvp);
}


bool Label::Step(float seconds) {
  if (mTimeoutSec == 0 || Hidden())
    return true;
  mRemainingSec -= seconds;
  float opacity = 1;
  if (mRemainingSec <= 0) {
    Hide(true);
  } else if (mRemainingSec > mTimeoutSec - mFadeSec) {
    opacity = (mTimeoutSec - mRemainingSec) / mFadeSec;
  } else if (mRemainingSec < mFadeSec) {
    opacity = mRemainingSec / mFadeSec;
  }
  SetOpacity(opacity);
  return true;
}


bool Label::Draw() {
  if (Hidden())
    return true;
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  
  float r, g, b, a;
  const float k = Enabled() ? 1 : 0.5;
  if (mTex) {
    r = k * mBkgTexColor[0]; g = k * mBkgTexColor[1];
    b = k * mBkgTexColor[2]; a = mOpacity * k * mBkgTexColor[3];
    if (!glt::Draw3SliceTexture2f(mTex, x0, y0, x1, y1, 0, 1, 1, 0,
                                  mTexDim[0], mTexDim[1], Width(), Height(),
                                  r, g, b, a, MVP()))
      return false;
  }

  float ndcH, ptW, ptH;
  const float ptScale = mPts / float(mFont->charDimPt[0]);
  if (!MVP()) {
    // Compute mPtW, mPtH here, based on the actual viewport?
    int w = std::max(Width() - 2 * mPadPt[0], 2.0f);  // Avoid negative vp
    int h = std::max(Height() - 2 * mPadPt[1], 2.0f);
    ptW = 2.0 * ptScale / w;
    ptH = 2.0 * ptScale / h;
    glViewport(Left() + mPadPt[0], Bottom() + mPadPt[1], w, h);
    ndcH = 2;
  } else {
    x0 += mPadPt[0];                                  // push in, no vp change
    y0 += mPadPt[1];
    x1 -= mPadPt[0];
    y1 -= mPadPt[1];
    ndcH = Height() - 2 * mPadPt[1];
    ptW = ptH = ptScale;
  }
  
  glt::Align align = (glt::Align)mAlign;
  const float textNDCHeight = ptH * mLineCount * mFont->charDimPt[1];
  const float padNDCHeight = 0.5 * (ndcH - textNDCHeight);
  float y = y1 - padNDCHeight;
  glt::FontStyle style;
  style.C[0] = k * mTextColor[0]; style.C[1] = k * mTextColor[1];
  style.C[2] = k * mTextColor[2]; style.C[3] = mOpacity * mTextColor[3];
  style.dropshadowC[0] = mTextDropshadowColor[0];
  style.dropshadowC[1] = mTextDropshadowColor[1];
  style.dropshadowC[2] = mTextDropshadowColor[2];
  style.dropshadowC[3] = mOpacity * mTextDropshadowColor[3];
  style.dropshadowOffsetPts[0] = mTextDropshadowOffsetPts[0];
  style.dropshadowOffsetPts[1] = mTextDropshadowOffsetPts[1];
  if (!glt::DrawParagraph(mText, x0, y0, x1, y, align, mFont, ptW, ptH, &style,
                          MVP(), mTextRange[0], mTextRange[1], mWrapLines))
    return false;
  glDisable(GL_BLEND);
  return true;
}


//
// InfoBox
//

bool InfoBox::SetViewport(int x, int y, int w, int h) {
  const int cx = x + w / 2;                             // Center of screen
  const int cy = y + h / 2;
  const int ww = Width() ? Width() : 2;                 // Avoid zero
  const int wh = Height() ? Height() : 2;
  if (!Label::SetViewport(cx - ww / 2, cy - wh / 2, ww, wh))
    return false;
  return true;
}


bool InfoBox::SetText(const char *text, float pts, const char *font) {
  if (!Label::SetText(text, pts, font))
    return false;
  SetFade(kTimeoutSec, kFadeSec);
  Hide(false);
  const int cx = Left() + Width() / 2;                  // Center of label
  const int cy = Bottom() + Height() / 2;
  FitViewport();                                        // Resize
  const int w = Width() ? Width() : 2;
  const int h = Height() ? Height() : 2;
  if (!Label::SetViewport(cx - w / 2, cy - h / 2, w, h))
    return false;
  return true;
}


//
// ProgressBar
//

bool ProgressBar::SetRange(float min, float max) {
  if (max <= min)
    return false;
  mRange[0] = min; mRange[1] = max; return true;
  return true;
}


bool ProgressBar::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  if (mCoreTex || mShellTex) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  }
  
  if (mShellTex) {
    if (!glt::Draw3SliceTexture2f(mShellTex, x0, y0, x1, y1, 0, 1, 1, 0,
                                       mTexDim[0], mTexDim[1], Width(),Height(),
                                       1, 1, 1, 1))
      return false;
  } else if (!glt::DrawColorBox2f(x0, y0, x1, y1, 0.8, 0.8, 0.8, 1))
    return false;
  float t = (mValue - mRange[0]) / (mRange[1] - mRange[0]);
  if (t > 0)
    t = std::max(t, 0.05f);
  else
    t = 0;
  float x = x0 + t * (x1 - x0);
  float k = 0.1 * sinf(mSeconds*3) + 1;
  if (mCoreTex) {
    if (!glt::Draw3SliceTexture2f(mCoreTex, x0, y0, x, y1, 0, 1, 1, 0,
                                  mTexDim[0], mTexDim[1], Width(),Height(),
                                  k, k, k, 1))
      return false;
  } else if (!glt::DrawColorBox2f(x0, y0, x, y1, k * mRGBA[0], k * mRGBA[1],
                                  k * mRGBA[2], mRGBA[3]))
    return false;

  glDisable(GL_BLEND);
  
  return true;
}


//
// Sprite
//

bool Sprite::Init(float opacity, float u0, float v0, float u1, float v1,
                  unsigned int texture) {
  if (!texture)
    return false;
  mOpacity = opacity;
  mSpriteTexture = texture;
  mSpriteUV[0] = u0;
  mSpriteUV[1] = v0;
  mSpriteUV[2] = u1;
  mSpriteUV[3] = v1;
  memcpy(mOriginalViewport, mViewport, sizeof(mOriginalViewport));
  
  // Sprite programs allow for a constant opacity multiplier
  if (!mSpriteProgram) {
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uCTex;\n"
    "uniform float uOpacity;\n"
    "void main() {\n"
    "  vec4 C = texture2D(uCTex, vUV);\n"
    "  gl_FragColor = vec4(C.xyz, C.w * uOpacity);\n"
    "}\n";
    GLuint vp = glt::CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = glt::CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    mSpriteProgram = glt::CreateProgram(vp, fp, "tui::Sprite");
    if (!mSpriteProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(mSpriteProgram);
    mSpriteAUV = glGetAttribLocation(mSpriteProgram, "aUV");
    mSpriteAP = glGetAttribLocation(mSpriteProgram, "aP");
    mSpriteUCTex = glGetUniformLocation(mSpriteProgram, "uCTex");
    mSpriteUOpacity = glGetUniformLocation(mSpriteProgram, "uOpacity");
    if (glt::Error())
      return false;
  }
  
  return true;
}


// A simple ease-in-ease out function that is a bit more continuous
// than the usual S-curve function.

static float Smootherstep(float v0, float v1, float t) {
  t = t * t * t * (t * (t * 6 - 15) + 10);
  return v0 + t * (v1 - v0);
}


// Animate the sprite by recomputing its viewport and opacity using
// the smootherstep interpolator.

bool Sprite::Step(float seconds) {
  if (mSecondsToTarget == 0)
    return true;
  if (seconds > mSecondsRemaining) {
    memcpy(mViewport, mTargetViewport, sizeof(mViewport));
    mSecondsToTarget = 0;
    mSecondsRemaining = 0;
    return true;
  }
  mSecondsRemaining -= seconds;
  const float t = 1 - mSecondsRemaining / mSecondsToTarget;
  mOpacity = Smootherstep(mOriginalOpacity, mTargetOpacity, t);
  for (size_t i = 0; i < 4; ++i)
    mViewport[i] = Smootherstep(mOriginalViewport[i], mTargetViewport[i], t);
  return true;
}


// Use a custom program to draw the sprite to enable transparency.

bool Sprite::Draw() {
  if (Hidden())
    return true;
  if (mOpacity < 1) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  glUseProgram(mSpriteProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mSpriteTexture);
  glUniform1i(mSpriteUCTex, 0);
  glUniform1f(mSpriteUOpacity, mOpacity);
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  if (!glt::DrawBox2f(mSpriteAP, x0, y0, x1, y1, mSpriteAUV, mSpriteUV[0],
                           mSpriteUV[1], mSpriteUV[2], mSpriteUV[3]))
    return false;
  
  return true;
}


bool Sprite::SetTarget(int x, int y, int w, int h, float opacity, float sec) {
  mOriginalOpacity = mOpacity;
  mTargetViewport[0] = x;
  mTargetViewport[1] = y;
  mTargetViewport[2] = w;
  mTargetViewport[3] = h;
  mTargetOpacity = opacity;
  mSecondsToTarget = sec;
  mSecondsRemaining = sec;
  return true;
}


//
// Spinner
//

bool Spinner::Init(unsigned int texture, float incAngle, float incMinSec) {
  mTex = texture;
  mIncAngle = incAngle;
  mIncSec = incMinSec;
  return true;
}


bool Spinner::Step(float seconds) {
  if (!IsAnimating())
    return true;
  float sec = mLastUpdateSec + seconds;
  if (sec > mIncSec) {
    mLastUpdateSec = 0;
    mAngle += mIncAngle;
  } else {
    mLastUpdateSec = sec;
  }
  return true;
}


bool Spinner::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  
  const float *mvp = MVP();
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!mvp)
    mvp = &I[0];
  Imath::M44f M(mvp[0],mvp[1],mvp[2], mvp[3],  mvp[4], mvp[5], mvp[6], mvp[7],
                mvp[8],mvp[9],mvp[10],mvp[11], mvp[12],mvp[13],mvp[14],mvp[15]);

  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  const float cx = 0.5 * (x0 + x1);
  const float cy = 0.5 * (y0 + y1);
  Imath::M44f R, T0, T1;
  T0.translate(Imath::V3f(-cx, -cy, 0));
  T1.translate(Imath::V3f(cx, cy, 0));
  R.rotate(Imath::V3f(0, 0, -mAngle));
  Imath::M44f T = T0 * R * T1 * M;
  mvp = T.getValue();
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  const float g = Enabled() ? 1 : 0.5;
  if (!glt::DrawTexture2f(mTex, x0,y0,x1,y1, 0,1,1,0, g,g,g,1, mvp))
    return false;
  glDisable(GL_BLEND);
  
  return true;
}


//
// Flipbook
//

bool Flipbook::Init(size_t count, const unsigned int *tex) {
  mTexVec.resize(count);
  memcpy(&mTexVec[0], tex, count * sizeof(unsigned int));
  SetFrameRate(30);
  return true;
}


bool Flipbook::Step(float seconds) {
  if (!IsAnimating())
    return true;
  mFrameSec += seconds;
  int dIdx = int(mFrameSec * mFPS);
  mFrameIdx = (mFrameIdx + dIdx) % mTexVec.size();
  mFrameSec -= dIdx / mFPS;
  return true;
}


bool Flipbook::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  if (!glt::DrawTexture2f(mTexVec[mFrameIdx], x0, y0, x1, y1, 0,1,1,0, MVP()))
    return false;
  glDisable(GL_BLEND);

  return true;
}


//
// Button
//

int Button::FindPress(size_t id) const {
  for (size_t i = 0; i < mPressVec.size(); ++i)
    if (mPressVec[i].id == id)
      return i;
  return -1;
}


bool Button::Touch(const Event &event) {
  if (!Enabled() || Hidden())
    return false;
  
  // Check each touch, add to PressedIdVector if inside on begin,
  // marking as down, mark as up if id already in PressedIdVector
  // is down and outside, trigger OnTouchTap if ended inside.
  for (size_t t = 0; t < event.touchVec.size(); ++t) {
    const struct Event::Touch &touch = event.touchVec[t];
    int idx = 0;
    switch (event.phase) {
      case TOUCH_BEGAN:
        idx = FindPress(touch.id);
        if (idx >= 0) {                     // Clear out if FSM is out-of-order
          std::vector<Press>::iterator i = mPressVec.begin() + idx;
          mPressVec.erase(i);
        }
        if (Inside(touch.x, touch.y)) {
          mPressVec.push_back(Press(touch.id, true, touch.x, touch.y));
          OnTouchBegan(touch);              // Buttons ignore ProcessGestures
          return false;
        }
        break;
      case TOUCH_MOVED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          mPressVec[idx].pressed = Inside(touch.x, touch.y, mCancelPad);
          mPressVec[idx].x = touch.x;       // Update current touch position
          mPressVec[idx].y = touch.y;
          if (OnDrag(TOUCH_MOVED, touch.x, touch.y, touch.timestamp))
            return true;
          return WasPressed();
        }
        break;
      case TOUCH_ENDED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          std::vector<Press>::iterator i = mPressVec.begin() + idx;
          mPressVec.erase(i);
          if (Inside(touch.x, touch.y, mCancelPad)) {
            if (InvokeTouchTap(touch))
              return true;
            return false;
          }
        }
        break;
      case TOUCH_CANCELLED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          std::vector<Press>::iterator i = mPressVec.begin() + idx;
          mPressVec.erase(i);
          return false;
        }
        break;
    }
  }
  
  // Clear all touches on cancel (which may not include a touch vector)
  if (event.phase == TOUCH_CANCELLED && event.touchVec.empty())
    mPressVec.clear();
  
  return false;
}


bool Button::Pressed() const {
  for (size_t i = 0; i < mPressVec.size(); ++i)
    if (mPressVec[i].pressed)
      return true;
  return false;
}


void Button::AverageTouchPosition(double *x, double *y) const {
  if (mPressVec.empty()) {
    *x = *y = 0;
    return;
  }
  
  double px = 0, py = 0;
  for (size_t i = 0; i < mPressVec.size(); ++i) {
    px += mPressVec[i].x;
    py += mPressVec[i].y;
  }
  *x = px / mPressVec.size();               // Average
  *y = py / mPressVec.size();
}


//
// ImageButton
//


ImageButton::~ImageButton() {
  if (mIsTexOwned) {
    glDeleteTextures(1, &mDefaultTex);
    glDeleteTextures(1, &mPressedTex);
  }
}


bool ImageButton::Init(bool blend, unsigned int defaultTex,
                       unsigned int pressedTex, bool ownTex) {
  if (defaultTex == 0 || pressedTex == 0)
    return false;
  
  mIsBlendEnabled = blend;
  mIsTexOwned = ownTex;
  mDefaultTex = defaultTex;
  mPressedTex = pressedTex;
  
  return true;
}


bool ImageButton::Draw() {
  if (Hidden())
    return true;
  
  if (mIsBlendEnabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  unsigned int texture = Pressed() ? mPressedTex : mDefaultTex;
  const float g = Enabled() ? 1 : 0.5;
  if (!glt::DrawTexture2f(texture, x0,y0,x1,y1, 0,1,1,0, g,g,g,1, MVP()))
    return false;
  
  return true;
}


//
// CheckboxImageButton
//

bool CheckboxImageButton::Init(bool blend, unsigned int deselectedTex,
                               unsigned int pressedTex,
                               unsigned int selectedTex) {
  if (deselectedTex == 0 || pressedTex == 0 || selectedTex == 0)
    return false;
  
  mBlendEnabled = blend;
  mDeselectedTex = deselectedTex;
  mPressedTex = pressedTex;
  mSelectedTex = selectedTex;
  
  return true;
}


bool CheckboxImageButton::Draw() {
  if (Hidden())
    return true;
  
  if (mBlendEnabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  unsigned int texture = Pressed() ? mPressedTex : Selected() ?
                                    mSelectedTex : mDeselectedTex;
  const float g = Enabled() ? 1 : 0.5;
  if (!glt::DrawTexture2f(texture, x0,y0,x1,y1, 0,1,1,0, g,g,g,1, MVP()))
    return false;
  
  glDisable(GL_BLEND);
  
  return true;
}


//
// TextButton
//

bool TextButton::Init(const char *text, float pts, size_t w, size_t h,
                      unsigned int defaultTex, unsigned int pressedTex,
                      const char *font, float padX, float padY) {
  if (!mLabel.Init(text, pts, font))
    return true;
  mDim[0] = w;
  mDim[1] = h;
  mDefaultTex = defaultTex;
  mPressedTex = pressedTex;
  mLabel.SetBackgroundTex(mDefaultTex, mDim[0], mDim[1]);
  if (padX < 0)
    padX = pts;
  if (padY < 0)
    padY = pts/1.5;
  mLabel.SetViewportPad(padX, padY);
  return true;
}


bool TextButton::FitViewport() {
  if (!mLabel.FitViewport())
    return false;
  if (!Button::SetViewport(mLabel.Left(), mLabel.Bottom(),
                           mLabel.Width(), mLabel.Height()))
    return false;
  return true;
}


bool TextButton::SetViewport(int x, int y, int w, int h) {
  if (!Button::SetViewport(x, y, w, h))
    return false;
  if (!mLabel.SetViewport(x, y, w, h))
    return false;
  return true;
}


void TextButton::SetMVP(const float *mvp) {
  Button::SetMVP(mvp);
  mLabel.SetMVP(mvp);
}


bool TextButton::Draw() {
  if (Hidden())
    return true;
  
  const GLuint tex = Pressed() ? mPressedTex : mDefaultTex;
  mLabel.SetBackgroundTex(tex, mDim[0], mDim[1]);
  
  if (!mLabel.Draw())
    return false;
  
  return true;
}


//
// TextCheckbox
//

bool TextCheckbox::Init(const char *text, float pts, size_t w, size_t h,
                        unsigned int deselectedTex, unsigned int pressedTex,
                        unsigned int selectedTex, const char *font,
                        float padX, float padY) {
  if (!mLabel.Init(text, pts, font))
    return true;
  mDim[0] = w;
  mDim[1] = h;
  mDeselectedTex = deselectedTex;
  mPressedTex = pressedTex;
  mSelectedTex = selectedTex;
  mLabel.SetBackgroundTex(mDeselectedTex, mDim[0], mDim[1]);
  if (padX < 0)
    padX = pts;
  if (padY < 0)
    padY = pts/1.5;
  mLabel.SetViewportPad(padX, padY);
  return true;
}


bool TextCheckbox::FitViewport() {
  if (!mLabel.FitViewport())
    return false;
  if (!Button::SetViewport(mLabel.Left(), mLabel.Bottom(),
                           mLabel.Width(), mLabel.Height()))
    return false;
  return true;
}


bool TextCheckbox::SetViewport(int x, int y, int w, int h) {
  if (!Button::SetViewport(x, y, w, h))
    return false;
  if (!mLabel.SetViewport(x, y, w, h))
    return false;
  return true;
}


void TextCheckbox::SetMVP(const float *mvp) {
  CheckboxButton::SetMVP(mvp);
  mLabel.SetMVP(mvp);
}


bool TextCheckbox::Draw() {
  if (Hidden())
    return true;

  const GLuint tex = Pressed() ? mPressedTex : Selected() ?
                       mSelectedTex : mDeselectedTex;
  mLabel.SetBackgroundTex(tex, mDim[0], mDim[1]);
  
  if (!mLabel.Draw())
    return false;
  
  return true;
}


//
// RadioButton
//

RadioButton::~RadioButton() {
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    delete mButtonVec[i];
  mButtonVec.clear();
}


void RadioButton::Add(CheckboxButton *button) {
  mButtonVec.push_back(button);
  int w = 0, h = 0;
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    w += mButtonVec[i]->Width();
    h = std::max(h, mButtonVec[i]->Height());
  }
  SetViewport(Left(), Bottom(), w, h);
}


void RadioButton::Clear() {
  mButtonVec.clear();
}


void RadioButton::Destroy() {
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    delete mButtonVec[i];
  Clear();
}


CheckboxButton *RadioButton::Selected() const {
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mButtonVec[i]);
    if (cb && cb->Selected())
      return cb;
  }
  return NULL;
}


int RadioButton::SelectedIdx() const {
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mButtonVec[i]);
    if (cb && cb->Selected())
      return i;
  }
  return -1;
}


void RadioButton::SetSelected(CheckboxButton *button) {
  bool wasSelected = Selected() != NULL;
  
  // Deselect any currently selected buttons
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mButtonVec[i]);
    if (cb && cb != button && cb->Selected())
      cb->SetSelected(false);
  }
  
  if (button && !button->Selected())
    button->SetSelected(true);
  
  if (Selected() == NULL && wasSelected)
    OnNoneSelected();
}


bool RadioButton::SetViewport(int x, int y, int w, int h) {
  if (!ViewportWidget::SetViewport(x, y, w, h))
    return false;
  int tx = 0;
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    ViewportWidget *w = mButtonVec[i];
    int wy = y + 0.5 * (h - w->Height());
    if (!w->SetViewport(x + tx, wy, w->Width(), w->Height()))
      return false;
    tx += w->Width();
  }
  return true;
}


void RadioButton::SetMVP(const float *mvp) {
  ViewportWidget::SetMVP(mvp);
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    mButtonVec[i]->SetMVP(mvp);
}


bool RadioButton::Touch(const Event &event) {
  if (!Enabled() || Hidden())
    return false;
  bool wasSelected = Selected() != NULL;
  bool consumed = false;
  
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mButtonVec[i]);
    if (!mIsNoneAllowed && cb->Selected())
      continue;                               // Don't deselect if selected
    bool oldStatus = cb->Selected();
    if (cb->Touch(event))
      consumed = true;
    if (oldStatus != cb->Selected() && cb->Selected()) {      // State changed
      SetSelected(cb);
      break;
    }
  }
  
  if (Selected() == NULL && wasSelected)
    OnNoneSelected();
  
  return consumed;
}


bool RadioButton::Draw() {
  if (Hidden())
    return true;
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    if (!mButtonVec[i]->Draw())
      return false;
  }
  return true;
}


void RadioButton::Hide(bool status) {
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    mButtonVec[i]->Hide(status);
}


//
// Handle
//

void Handle::SetConstraintDir(float x, float y) {
  // Compute the constraint line [ax, ay, bx, by].
  // Do it here, once, when the direction is set, not on each touch!
  mLine[0] = Left() + Width() / 2.0;
  mLine[1] = Bottom() + Height() / 2.0;
  mLine[2] = mLine[0] + x;
  mLine[3] = mLine[1] + y;
  mIsSegment = false;
}


// Closest point along the line AB to the point P.
// Projects P onto AB and interpolates result.

static void ClosestPoint(double ax, double ay, double bx, double by, bool clamp,
                         double px, double py, double *dx, double *dy) {
  const double apx = px - ax;
  const double apy = py - ay;
  const double abx = bx - ax;
  const double aby = by - ay;
  const double ab2 = abx * abx + aby * aby;
  const double ap_ab = apx * abx + apy * aby;
  double t = ap_ab / ab2;
  if (clamp) {
    t = ::clamp(t, 0.0, 1.0);
    if (t < 0.0f)
      t = 0.0f;
    else if (t > 1.0f)
      t = 1.0f;
  }
  *dx = ax + t * abx;
  *dy = ay + t * aby;
}


bool Handle::Touch(const Event &event) {
  if (!Enabled() || Hidden())
    return false;
  bool consumed = Button::Touch(event);
  bool drag = Pressed() || (Constrained() && WasPressed());
  drag = WasPressed();
  if (drag) {
    consumed = true;

    double px = 0, py = 0;                        // Average touch position
    AverageTouchPosition(&px, &py);
    double vx, vy;
    if (Constrained()) {
      ClosestPoint(mLine[0], mLine[1], mLine[2], mLine[3], mIsSegment,
                   px, py, &vx, &vy);
    } else {
      vx = px;
      vy = py;
    }
    vx -= Width() / 2.0;                          // Offset to corner
    vy -= Height() / 2.0;
    SetViewport(vx, vy, Width(), Height());       // Move the handle
    OnDrag(event.phase, px, py, event.touchVec[0].timestamp);
  }
  return consumed;
}


//
// ImageHandle
//

bool ImageHandle::Init(unsigned int defaultTex, unsigned int pressedTex) {
  mDefaultTex = defaultTex;
  mPressedTex = pressedTex;
  return true;
}


bool ImageHandle::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  GLuint tex = Pressed() ? mPressedTex : mDefaultTex;
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  float g = Enabled() ? 1 : 0.5;
  if (!glt::DrawTexture2f(tex, x0, y0, x1, y1, 0, 1, 1, 0, g,g,g,1, MVP()))
    return false;
  glDisable(GL_BLEND);
  
  return true;
}


//
// Slider
//

Slider::~Slider() {
  delete mHandle;
}


bool Slider::Init(unsigned int sliderTex, tui::Handle *handle) {
  mSliderTex = sliderTex;
  mHandleT = 0.5;
  mHandle = handle;
  mHandle->SetYConstrained(true);
  return true;
}


bool Slider::Init(unsigned int sliderTex, size_t handleW, size_t handleH,
                  unsigned int handleTex, unsigned int handlePressedTex) {
  ImageHandle *handle = new ImageHandle;
  if (!handle->Init(handleTex, handlePressedTex))
    return false;
  handle->SetViewport(0, 0, handleW, handleH);
  if (!Init(sliderTex, handle))
    return false;
  
  return true;
}


bool Slider::SetViewport(int x, int y, int w, int h) {
  if (!ViewportWidget::SetViewport(x, y, w, h))
    return false;
  if (!SetValue(0.5))
    return false;
  const float pad = mHandle->Height() / 2;
  const float hcy = Bottom() + 0.5 * Height();
  mHandle->SetConstraintSegment(Left() + pad, hcy, Right() - pad, hcy);

  return true;
}


void Slider::SetMVP(const float *mvp) {
  ViewportWidget::SetMVP(mvp);
  mHandle->SetMVP(mvp);
}


bool Slider::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  float x0, y0, x1, y1;
  GetNDCRect(&x0, &y0, &x1, &y1);
  float g = Enabled() ? 1 : 0.5;
  if (!glt::DrawTexture2f(mSliderTex, x0, y0, x1, y1, 0, 1, 1, 0,
                          g * mBkgTexColor[0], g * mBkgTexColor[1],
                          g * mBkgTexColor[2], mBkgTexColor[3], MVP()))
    return false;
  glDisable(GL_BLEND);
  if (!mHandle->Draw())
    return false;
  return true;
}


bool Slider::SetValue(float value) {
  mHandleT = value;
  assert(value >= 0 && value <= 1);
  float sw = Width() - mHandle->Width();
  int hx = Left() + mHandleT * sw;
  int hy = Bottom() + (Height() - mHandle->Height()) / 2;
  if (!mHandle->SetViewport(hx, hy, mHandle->Width(), mHandle->Height()))
    return false;
  if (!OnValueChanged(Value()))
    return false;
  return true;
}


bool Slider::Touch(const Event &event) {
  if (!Enabled() || Hidden())
    return false;
  float oldVal = Value();
  bool consumed = mHandle->Touch(event);
  const float pad = mHandle->Height() / 2;
  float x = mHandle->Left() + 0.5 * mHandle->Width();
  float newVal = (x - Left() - pad) / float(Width() - 2 * pad);
  if (consumed && oldVal != newVal) {
    mHandleT = newVal;
    consumed = OnValueChanged(newVal);
  }
  return consumed;
}


//
// StarRating
//

bool StarRating::Init(size_t count, float pts, const char *font) {
  mStarCount = count;
  mTextColor[0] = mTextColor[1] = mTextColor[2] = mTextColor[3] = 1;
  mSelectedColor[0] = 0.5;
  mSelectedColor[1] = 0.5;
  mSelectedColor[2] = 0.9;                              // Blue
  mSelectedColor[3] = 1;
  char text[512];
  for (size_t i = 0; i < count; ++i)
    text[i] = glt::Font::StarChar;
  text[count] = '\0';
  if (!mLabel.Init(text, pts, font))
    return false;
  if (!FitViewport())
    return false;
  return true;
}


bool StarRating::FitViewport() {
  if (!mLabel.FitViewport())
    return false;
  if (!Button::SetViewport(mLabel.Left(), mLabel.Bottom(),
                           mLabel.Width(), mLabel.Height()))
    return false;
  return true;
}


bool StarRating::SetViewport(int x, int y, int w, int h) {
  if (!Button::SetViewport(x, y, w, h))
    return false;
  if (!mLabel.SetViewport(x, y, w, h))
    return false;
  return true;
}


bool StarRating::Draw() {
  if (Hidden())
    return true;
  
  int v = Pressed() ? mDragValue : mValue;
  v = std::max(v, 0);
  if (v > 0) {
    mLabel.SetTextColor(mSelectedColor[0], mSelectedColor[1],
                        mSelectedColor[2], mSelectedColor[3]);
    mLabel.SetTextRange(0, v - 1);
    if (!mLabel.Draw())
      return false;
  }
  if (v < int(mStarCount)) {
    mLabel.SetTextColor(mTextColor[0], mTextColor[1],
                        mTextColor[2], mTextColor[3]);
    mLabel.SetTextRange(v, mStarCount);
    if (!mLabel.Draw())
      return false;
  }
  return true;
}


bool StarRating::OnDrag(tui::EventPhase phase, float x, float y,
                        double timestamp) {
  ComputeDragValue(x);
  return true;
}


bool StarRating::SetValue(int value) {
  if (value > int(mStarCount))
    return false;
  mValue = value;
  return true;
}


//
// Group
//

bool Group::Add(Widget *widget) {
  mWidgetVec.push_back(widget);
  return true;
}


bool Group::Remove(Widget *widget) {
  std::vector<Widget *>::iterator i = std::find(mWidgetVec.begin(),
                                                mWidgetVec.end(), widget);
  if (i == mWidgetVec.end())
    return false;
  mWidgetVec.erase(i);
  return true;
}


void Group::Clear() {
  mWidgetVec.clear();
}


bool Group::Draw() {
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (!mWidgetVec[i]->Draw())
      status = false;
  }
  return status;
}


bool Group::Touch(const Event &event) {
  if (mWidgetVec.empty())
    return false;
  bool consumed = false;
  // Process events in reverse order of drawing for "visibility"
  for (int i = mWidgetVec.size() - 1; i >= 0; --i) {
    if (mWidgetVec[i]->Touch(event)) {
      consumed = true;
      if (!mIsMultitouch) {
        for (size_t j = 0; j < mWidgetVec.size(); ++j) {
          if (j != i) {
            mWidgetVec[j]->Touch(Event(TOUCH_CANCELLED));
          }
        }
        break;
      }
    }
  }
  return consumed;
}


bool Group::Step(float seconds) {
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    AnimatedViewport *av = dynamic_cast<AnimatedViewport *>(mWidgetVec[i]);
    if (!av)
      continue;
    if (!av->Step(seconds))
      status = false;
  }
  return status;
}


bool Group::Dormant() const {
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    AnimatedViewport *av = dynamic_cast<AnimatedViewport *>(mWidgetVec[i]);
    if (!av)
      continue;
    if (!av->Dormant())
      status = false;                                     // FIXME: break?
  }
  return status;
}


bool Group::Enabled() const {
  bool enabled = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (mWidgetVec[i]->Enabled())
      enabled = true;                                     // FIXME: break?
  }
  return enabled;
}


void Group::Enable(bool status) {
  for (size_t i = 0; i < mWidgetVec.size(); ++i)
    mWidgetVec[i]->Enable(status);
}


bool Group::Hidden() const {
  bool hidden = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (mWidgetVec[i]->Hidden())
      hidden = true;                                      // FIXME: break?
  }
  return hidden;
}


void Group::Hide(bool status) {
  for (size_t i = 0; i < mWidgetVec.size(); ++i)
    mWidgetVec[i]->Hide(status);
}


void Group::SetMVP(const float *mvp) {
  Widget::SetMVP(mvp);
  for (size_t i = 0; i < mWidgetVec.size(); ++i)
    mWidgetVec[i]->SetMVP(mvp);
}


//
// Toolbar
//

bool Toolbar::SetViewport(int x, int y, int w, int h) {
  if (!ViewportWidget::SetViewport(x, y, w, h))
    return false;
  
  // Find the total width of all fixed items, and then split that up
  // equally between the flexible spacers (those with width<0)
  int flexibleCount = 0;
  int totalWidth = 0;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    const int w = mWidgetVec[i]->Width();
    if (w < 0)
      flexibleCount++;
    else
      totalWidth += w;
  }
  
  int flexibleSpacing = flexibleCount ? (Width()-totalWidth)/flexibleCount : 0;
  if (flexibleSpacing < 0)
    flexibleSpacing = 0;
  
  // Now set the viewport for all widgets (except flexible spacers)
  int wx = x;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    ViewportWidget *widget = mWidgetVec[i];
    if (widget->Width() > 0) {
      int wy = Bottom() + 0.5 * (Height() - widget->Height());
      widget->SetViewport(wx, wy, widget->Width(), widget->Height());
      wx += widget->Width();
    } else {
      wx += flexibleSpacing;
    }
  }
  
  return true;
}


void Toolbar::SetMVP(const float *mvp) {
  ViewportWidget::SetMVP(mvp);
  for (size_t i = 0; i < mWidgetVec.size(); ++i)
    mWidgetVec[i]->SetMVP(mvp);
}


class Spacer : public ViewportWidget { /* for dynamic typing */ };

bool Toolbar::AddFixedSpacer(int w) {
  assert(w > 0);
  Spacer *spacer = new Spacer;
  spacer->SetViewport(0, 0, w, 1);
  Add(spacer);
  return true;
}


bool Toolbar::AddFlexibleSpacer() {
  Spacer *spacer = new Spacer;
  spacer->SetViewport(0, 0, -1, -1);
  Add(spacer);
  return true;
}


void Toolbar::Clear() {
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    Spacer *spacer = dynamic_cast<Spacer *>(mWidgetVec[i]);
    if (spacer)
      delete spacer;
  }
  mWidgetVec.clear();
}


bool Toolbar::Touch(const tui::Event &event) {
  if (!Enabled() || Hidden())
    return false;
  bool consumed = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (mWidgetVec[i]->Touch(event))
      consumed = true;                          // FIXME: Consider return?
  }
  if (consumed)
    return true;
  return AnimatedViewport::Touch(event);
}


bool Toolbar::Step(float seconds) {
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    AnimatedViewport *av = dynamic_cast<AnimatedViewport *>(mWidgetVec[i]);
    if (!av)
      continue;
    if (!av->Step(seconds))
      status = false;
  }
  return status;
}


bool Toolbar::Dormant() const {
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    AnimatedViewport *av = dynamic_cast<AnimatedViewport *>(mWidgetVec[i]);
    if (!av)
      continue;
    if (!av->Dormant())
      status = false;
  }
  return status;
}


bool Toolbar::Draw() {
  if (Hidden())
    return true;
  if (!MVP())
    glViewport(Left(), Bottom(), Width(), Height());
  if (mBackgroundTex) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    float x0, y0, x1, y1;
    GetNDCRect(&x0, &y0, &x1, &y1);
    if (!glt::Draw3SliceTexture2f(mBackgroundTex, x0, y0, x1, y1, 0,1,1,0,
                                  mBackgroundTexDim[0],mBackgroundTexDim[1],
                                  Width(), Height(), 1, 1, 1, 1))
      return false;
    glDisable(GL_BLEND);
  }
  
  bool status = true;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (!mWidgetVec[i]->Draw())
      status = false;
  }
  return status;
}


//
// Flinglist
//

bool FlinglistImpl::Init(bool vertical, float pixelsPerCm) {
  mVertical = vertical;
  mPixelsPerCm = pixelsPerCm;

  if (mScrollOffset < ScrollMin())
    mScrollOffset = ScrollMin();
  else if (mScrollOffset > ScrollMax())
    mScrollOffset = ScrollMax();
  
  // The fling program is used to draw the thumb and other decorations,
  // but the main content is drawn by the client who derives a new
  // class and overrides the main Draw function.
  
  if (!mFlingProgram) {
    static const char *vp_code =
    "attribute vec2 aP;\n"
    "void main() {\n"
    "  gl_Position = vec4(aP.x, aP.y, 0, 1);\n"
    "}\n";
    GLuint vp = glt::CreateShader(GL_VERTEX_SHADER, vp_code);
    if (!vp)
      return false;
    static const char *fp_code =
#if !defined(OSX)
    "precision mediump float;\n"
#endif
    "uniform vec4 uC;\n"
    "void main() { gl_FragColor = uC; }";
    GLuint fp = glt::CreateShader(GL_FRAGMENT_SHADER, fp_code);
    if (!vp)
      return false;
    mFlingProgram = glt::CreateProgram(vp, fp, "tui::Fling");
    if (!mFlingProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    
    static const char *glow_vp_code =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = aP;\n"
    "}\n";
    vp = glt::CreateShader(GL_VERTEX_SHADER, glow_vp_code);
    if (!vp)
      return false;
    
    static const char *glow_fp_code =
    "precision mediump float;\n"
    "uniform vec4 uC;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  float opacity = sin(vUV.y * 1.571);\n"
    "  gl_FragColor = vec4(uC.xyz, uC.w * opacity);\n"
    "}";
    fp = glt::CreateShader(GL_FRAGMENT_SHADER, glow_fp_code);
    if (!fp)
      return false;
    mGlowProgram = glt::CreateProgram(vp, fp, "tui::FlingGlow");
    if (!mGlowProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
  }
  
  return true;
}


void FlinglistImpl::ClampScrollableDim() {
  if (mScrollableDim)
    mScrollableDim = std::max(mScrollableDim, mFrameDim);
  else
    mScrollableDim = mFrameDim;
  if (mVertical)
    mScrollableDim = std::min(mScrollableDim, Height());
  else
    mScrollableDim = std::min(mScrollableDim, Width());
}

bool FlinglistImpl::SetViewport(int x, int y, int w, int h) {
  if (!AnimatedViewport::SetViewport(x, y, w, h))
    return false;
  ClampScrollableDim();
  return true;
}


void FlinglistImpl::SetFrameDim(int dim) {
  mFrameDim = dim;
  ClampScrollableDim();
}


bool FlinglistImpl::SetOverpull(unsigned int offTex, unsigned int onTex,
                                size_t w, size_t h) {
  mOverpullDim[0] = w;
  mOverpullDim[1] = h;
  mOverpullOffTex = offTex;
  mOverpullOnTex = onTex;
  return true;
}


bool FlinglistImpl::SetDragHandle(unsigned int texture, size_t w, size_t h) {
  mDragHandleDim[0] = w;
  mDragHandleDim[1] = h;
  mDragHandleTex = texture;
  return true;
}


bool FlinglistImpl::Append(Frame *frame) {
  if (!frame)
    return false;
  mFrameVec.push_back(frame);
  return true;
}


// Make sure to adjust any indices used to track frames when Prepending

bool FlinglistImpl::Prepend(Frame *frame) {
  if (!frame)
    return false;
  mFrameVec.insert(mFrameVec.begin(), frame);
  mScrollOffset += mFrameDim;
  if (mTouchFrameIdx >= 0)
    mTouchFrameIdx++;
  return true;
}


bool FlinglistImpl::Delete(Frame *frame) {
  for (std::vector<Frame *>::iterator i = mFrameVec.begin();
       i != mFrameVec.end(); ++i) {
    if (*i == frame) {
      mFrameVec.erase(i);
      return true;
    }
  }
  return false;
}


// Compute the viewport of any frame in the list.  The viewport may be
// offscreen.  First, we need to find the index of the frame and then
// compute its viewport using the widget's viewport, the scrolling offset
// and the size of each frame.

bool FlinglistImpl::Viewport(const Frame *frame, int viewport[4]) const {
  for (size_t i = 0; i < mFrameVec.size(); ++i) {
    if (mFrameVec[i] == frame) {
      int xory = mVertical ? 1 : 0;
      const int offset = mViewport[2+xory] + mScrollOffset;
      viewport[0] = mViewport[0];
      viewport[1] = mViewport[1];
      viewport[xory] += offset;
      viewport[3-xory] = mViewport[3-xory];
      viewport[2+xory] = mFrameDim;
      viewport[xory] -= (i + 1) * mFrameDim;
      return true;
    }
  }
  return false;
}


// Compute floating point region in NDC space covered by overpull textures

void FlinglistImpl::OverpullNDCRange(float *x0, float *y0,
                                     float *x1, float *y1) const {
  float dim = 0.3 * std::max(Width(), Height());
  float w = dim / float(Width());
  float h = dim / float(Height());
  *x0 = 1 + 2 * OverpullPixels() / float(Width());
  *x1 = *x0 + w;
  *y0 = 0 - h / 2;     // 0.0 is the middle of the screen
  *y1 = 0 + h / 2;
}


// Compute integer viewport covered by overpull texture

void FlinglistImpl::OverpullViewport(int viewport[4]) const {
  float x0, y0, x1, y1;
  OverpullNDCRange(&x0, &y0, &x1, &y1);
  viewport[0] = 0.5 * (x0 + 1) * Width();
  viewport[1] = 0.5 * (y0 + 1) * Height();
  viewport[2] = 0.5 * (x1 - x0) * Width();
  viewport[3] = 0.5 * (y1 - y0) * Height();
}


// Find the range of visible frames using the following math (see Viewport):
//
//   frameMin = vp[0] + vp[2] + scrollOffset - (i+1) * frameDim
//   frameMax = frameMin + frameDim
//   frameMax > vp[0]             Visible frame max
//   frameMin < vp[0] + vp[2]     Visible frame min
//
// Solve for i, and make sure to clamp the result to a valid range.

bool FlinglistImpl::VisibleFrameRange(int *min, int *max) const {
  if (mFrameVec.empty() || mFrameDim == 0 || Width() == 0 || Height() == 0) {
    *min = *max = -1;
    return false;
  }
  if (mVertical) {
    *min = ceil(mScrollOffset / float(mFrameDim) - 1);
    *min = clamp(*min, 0, int(mFrameVec.size()) - 1);
    *max = floor((mScrollOffset + Height()) / float(mFrameDim));
    *max = clamp(*max, 0, int(mFrameVec.size()) - 1);
  } else {
    *min = ceil(mScrollOffset / float(mFrameDim) - 1);
    *min = clamp(*min, 0, int(mFrameVec.size()) - 1);
    *max = floor((mScrollOffset + Width()) / float(mFrameDim));
    *max = clamp(*max, 0, int(mFrameVec.size()) - 1);
  }
  assert(*min <= *max);
  return true;
}


int FlinglistImpl::FrameIdx(const Frame *frame) const {
  // FIXME: Consider using a map to avoid linear search
  for (size_t i = 0; i < mFrameVec.size(); ++i)
    if (mFrameVec[i] == frame)
      return i;
  return -1;
}


int FlinglistImpl::ScrollDistance(const Frame *frame) const {
  int idx = FrameIdx(frame);
  int offset = ScrollToOffset(idx);
  return offset - mScrollOffset;
}


bool FlinglistImpl::Clear() {
  mFrameVec.clear();
  mTouchFrameIdx = -1;
  memset(mTouchStart, 0, sizeof(mTouchStart));
  mMovedAfterDown = false;
  mMovedOffAxisAfterDown = false;
  mScrollOffset = 0;
  mScrollVelocity = 0;
  mScrollBounce = 0;
  mThumbFade = 0;
  return true;
}


bool FlinglistImpl::DrawFrame(FlinglistImpl::Frame *frame) {
  int frameViewport[4] = { 0 };
  if (!Viewport(frame, frameViewport))
    return false;
  const int vp[4] = { mViewport[0] + mViewportInset,
                      mViewport[1] + mViewportInset,
                      mViewport[2] - 2 * mViewportInset,
                      mViewport[3] - 2 * mViewportInset };
  int scissor[4] = { frameViewport[0], frameViewport[1],
                     frameViewport[2], frameViewport[3] };
  if (scissor[0] < vp[0]) scissor[0] = vp[0];
  if (scissor[1] < vp[1]) scissor[1] = vp[1];
  if (scissor[0] + scissor[2] > vp[0] + vp[2])
    scissor[2] -= scissor[0] + scissor[2] - vp[0] - vp[2];
  if (scissor[1] + scissor[3] > vp[1] + vp[3])
    scissor[3] -= scissor[1] + scissor[3] - vp[1] - vp[3];
  if (scissor[2] <= 0 || scissor[3] <= 0)
    return true;;
  glViewport(frameViewport[0], frameViewport[1],
             frameViewport[2], frameViewport[3]);
  glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
  glEnable(GL_SCISSOR_TEST);
  if (glt::Error())
    return false;
  
  if (!frame->Draw())
    return false;
  
  glDisable(GL_SCISSOR_TEST);
  
  return true;
}


// Draw the visible frames, the over-scrolled region and the scroll thumb.

bool FlinglistImpl::Draw() {
  if (Hidden())
    return true;
  
  // Draw each of the visible frames using a separate viewport and scissor
  int minIdx, maxIdx;
  if (!VisibleFrameRange(&minIdx, &maxIdx))
    return true;

  for (size_t i = minIdx; i <= maxIdx; ++i) {
    if (!DrawFrame(mFrameVec[i]))
      return false;
  }

  glEnable(GL_SCISSOR_TEST);

  // Use an inset viewport to allow the strip to draw chrome outside the
  // displayed frame region without changing any of the other viewport-
  // dependent calculations (like the frame viewports).
  const int vp[4] = { mViewport[0] + mViewportInset,
                      mViewport[1] + mViewportInset,
                      mViewport[2] - 2 * mViewportInset,
                      mViewport[3] - 2 * mViewportInset };
  
  // Draw the over-scrolled region with a tinted blue highlight
  if (mScrollBounce != 0) {
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glScissor(vp[0], vp[1], vp[2], vp[3]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glUseProgram(mFlingProgram);
    int uC = glGetUniformLocation(mFlingProgram, "uC");
    glUniform4fv(uC, 1, mOverpullColor);
    int aP = glGetAttribLocation(mFlingProgram, "aP");
    glEnableVertexAttribArray(aP);
    float v[8] = { -1, -1,   -1, 1,   1, -1,   1, 1 };
    if (mVertical) {
      if (mScrollBounce < 0) {
        v[1] = v[5] = 1 - 2 * -mScrollBounce / vp[3];
      } else {
        v[3] = v[7] = 2 * mScrollBounce / vp[3] - 1;
      }
    } else {
      if (mScrollBounce < 0) {
        v[0] = v[2] = 1 - 2 * -mScrollBounce / vp[2];
      } else {
        v[4] = v[6] = 2 * mScrollBounce / vp[2] - 1;
      }
    }
    glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(aP);
    if (glt::Error())
      return false;
    
    // Currently hardcoded to only appear on right side.
    // Should generalize with left/right flags?
    if (mOverpullOnTex && mScrollBounce < OverpullPixels()/4 && !mVertical) {
      if (mVertical) {
        abort();
      } else {
        float x0, y0, x1, y1, mid;
        OverpullNDCRange(&x0, &y0, &x1, &y1);
        mid = v[0];
        mid = std::max(mid, x0);
        mid = std::min(mid, x1);
        
        // Draw and pull the off texture up until we pull far enough, 
        // then we draw the on texture always in the same place.
        GLuint overpullTex;
        float xf0, xf1;
        
        if (mid > x0) { 
          overpullTex = mOverpullOffTex;
          xf0 = v[0];
          xf1 = xf0 + (x1-x0);
        } else {
          overpullTex = mOverpullOnTex;
          xf0 = x0;
          xf1 = x1;
        }
  
        if (!glt::DrawTexture2f(overpullTex, xf0,y0,xf1,y1, 0,1,1,0,MVP()))
          return false;
      }
    }
  }
  
  // Currently hardcoded to only appear on right edge for overpull
  // Could generalize with left/right/every flags?
  if (mDragHandleTex && mScrollOffset == 0 && mScrollBounce <= 0) {
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glScissor(vp[0], vp[1], vp[2], vp[3]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // This doesn't seem to have any effect, mDragHandleTex still looks
    // alpha blended, not added.
//    glBlendEquation(GL_FUNC_ADD);
    float w = 2 * mDragHandleDim[0] / float(vp[2]);
    float h = 2 * mDragHandleDim[1] / float(vp[3]);
    float x, y;
    if (mVertical) {
      x = -w/2;
      if (mScrollBounce <= 0) {                   // <= forces handle on right
        y = 1 - 2 * -mScrollBounce / vp[3] - h;
      } else {
        y = 2 * mScrollBounce / vp[3] - 1;
      }
    } else {
      y = -h/2;
      if (mScrollBounce <= 0) {
        x = 1 - 2 * -mScrollBounce / vp[2] - w;
      } else {
        x = 2 * mScrollBounce / vp[2] - 1;
      }
    }
    if (!glt::DrawTexture2f(mDragHandleTex, x, y, x+w, y+h, 0,1,1,0,MVP()))
      return false;
    
    if (mGlowDragHandle) {
      glUseProgram(mGlowProgram);
      int aP = glGetAttribLocation(mGlowProgram, "aP");
      int aUV = glGetAttribLocation(mGlowProgram, "aUV");
      int uC = glGetUniformLocation(mGlowProgram, "uC");
      glUniform4f(uC, 0.4, 1, 1, 1);
      const float dim = 8 * (1 + sin(mGlowSeconds * 3));
      const float gw = dim / Width();
      const float gh = dim / Height();
      if (!glt::DrawBoxFrame2f(aP, x-gw, y-gh, x+w+gw, y+h+gh, gw, gh, aUV))
        return false;
    }
  }
  
  // Draw the scroll thumb that indicates where the visible region is
  // within the entire range of frames.
  // Note: Use full viewport for thumb, NOT inset viewport
  if (mMovedAfterDown || mScrollVelocity != 0 || mThumbFade > 0) {
    glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
    glScissor(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glUseProgram(mFlingProgram);
    int uC = glGetUniformLocation(mFlingProgram, "uC");
    float alpha = mThumbFade == 0 ? 1 : mThumbFade;
    const float g = 0.8;
    glUniform4f(uC, g, g, g, alpha);
    int aP = glGetAttribLocation(mFlingProgram, "aP");
    glEnableVertexAttribArray(aP);
    const int thumbWidth = 5 /*pixels*/;
    int xory = mVertical ? 1 : 0;
    float xmin = 2 * thumbWidth / float(mViewport[3-xory]);
    float ymin = 1 - 2 * mScrollOffset / float(TotalHeight());
    float ymax = 1 - 2 * (mScrollOffset + mViewport[2+xory]) / float(TotalHeight());
    const float ypix = 2.0 / mViewport[2+xory];
    ymin -= 2 * ypix;
    ymax += 2 * ypix;
    if ((ymin < ymax && (ymin > -1 || ymax < 1)) ||
        (ymin > ymax && (ymax > -1 || ymin < 1))) {
      float v[8];
      if (mVertical) {
        xmin = 1 - xmin;
        v[0] = xmin; v[1] = ymin;
        v[2] = xmin; v[3] = ymax;
        v[4] = 1;    v[5] = ymin;
        v[6] = 1;    v[7] = ymax;
      } else {
        xmin = -1 + xmin;
        v[0] = ymin; v[1] = xmin;
        v[2] = ymax; v[3] = xmin;
        v[4] = ymin; v[5] = -1;
        v[6] = ymax; v[7] = -1;
      }
      glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glUniform4f(uC, g, g, g, 0.75 * alpha);
      const float xpix = 2.0 / mViewport[3-xory];
      v[1-xory] = v[3-xory] = mVertical ? xmin + xpix : xmin - xpix;
      v[5-xory] = v[7-xory] = mVertical ? 1 - xpix : -1 + xpix;
      v[2+xory] = v[6+xory] = ymin;
      v[0+xory] = v[4+xory] = ymin + ypix;
      glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      v[2+xory] = v[6+xory] = ymax;
      v[0+xory] = v[4+xory] = ymax - ypix;
      glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glUniform4f(uC, g, g, g, 0.4 * alpha);
      v[1-xory] = v[3-xory] = mVertical ? xmin + 2 * xpix : xmin - 2 * xpix;
      v[5-xory] = v[7-xory] = mVertical ? 1 - 2 * xpix : -1 + 2 * xpix;
      v[2+xory] = v[6+xory] = ymin + ypix;
      v[0+xory] = v[4+xory] = ymin + 2 * ypix;
      glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      v[2+xory] = v[6+xory] = ymax - ypix;
      v[0+xory] = v[4+xory] = ymax - 2 * ypix;
      glVertexAttribPointer(aP, 2, GL_FLOAT, false, 0, v);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glDisableVertexAttribArray(aP);
      if (glt::Error())
        return false;
    }
  }
  
  glDisable(GL_SCISSOR_TEST);
  
  return true;
}


// Animate the list, moving it using the current velocity, adjusting the
// overscroll and thumb animations and/or snapping it to a selected index.

bool FlinglistImpl::Step(float seconds) {
  if (mFrameVec.empty())     // Otherwise oscillate b/c ScrollMin>ScrollMax
    return true;
  
  mGlowSeconds = mGlowDragHandle ? mGlowSeconds + seconds : 0;
  
  float initialScrollOffset = mScrollOffset;
  
  if (mSnapSeconds > 0) {
    if (seconds > mSnapRemainingSeconds) {
      assert(mSnapTargetOffset >= ScrollMin());
      assert (mSnapTargetOffset <= ScrollMax());
      mScrollOffset = mSnapTargetOffset;
      mSnapSeconds = mSnapRemainingSeconds = 0;
      mSnapOriginalOffset = mSnapTargetOffset = 0;
    } else {
      mSnapRemainingSeconds -= seconds;
      float t = mSnapRemainingSeconds / mSnapSeconds;
      mScrollOffset = t * mSnapOriginalOffset + (1 - t) * mSnapTargetOffset;
      assert(mScrollOffset >= ScrollMin());
      assert (mScrollOffset <= ScrollMax());
    }
    OnMove();
  } else if (mSnapToCenter && mTouchFrameIdx < 0 && mSnapSeconds == 0 &&
             mScrollVelocity != 0 && fabsf(mScrollVelocity) < kSnapVelocity) {
    // Snap after flinging to handle the !mSingleFrameFling mode
//    assert(!mSingleFrameFling);
    SnapIdx(FlingingSnapIdx(), 0.15);
  }

  
  if (mTouchFrameIdx < 0 && !MovedAfterDown() && mScrollVelocity) {
    float drag = 4 * seconds * mScrollVelocity;
    if (fabsf(drag) > fabsf(mScrollVelocity) || fabsf(mScrollVelocity) < 0.1) {
      mScrollVelocity = 0;
      mThumbFade = 1;
    } else {
      mScrollVelocity -= drag;
    }
    mScrollOffset += seconds * 100 * mScrollVelocity;
    OnMove();
  }
  
  if (mScrollOffset < ScrollMin()) {
    SnapIdx(0, 0);                              // Sets selected frame index
    mScrollOffset = ScrollMin();
    mScrollBounce = mScrollVelocity = 0;
    mThumbFade = 1;
  } else if (mScrollOffset > ScrollMax()) {
    SnapIdx(Size()-1, 0);
    mScrollOffset = ScrollMax();
    mScrollVelocity = mScrollBounce = 0;
    mThumbFade = 1;
  }
  if (mTouchFrameIdx < 0 && !MovedAfterDown() && mThumbFade > 0) {
    mThumbFade -= seconds;
    if (mThumbFade < 0.01)
      mThumbFade = 0;
  } else {
    mThumbFade = 0;
  }
  if (mTouchFrameIdx < 0 && !MovedAfterDown() && mScrollBounce != 0) {
    const float kBounceSec = 0.1;
    if (seconds > kBounceSec) {
      mScrollBounce = 0;
    } else {
      float drag = seconds / kBounceSec * mScrollBounce;
      mScrollBounce -= drag;
      if (fabsf(fabsf(mScrollBounce)) < 3)
        mScrollBounce = 0;
    }
  }
  
  if (mLongPressSeconds > 0 && !MovedAfterDown() && mTouchFrameIdx >= 0 &&
      mScrollOffset == initialScrollOffset) {
    // Only call long touch when we first cross the threshold
    if (mLongPressSeconds < mLongPressTimeout) {
      // HACK: Avoid big values for seconds due to first refreshes since pause
      mLongPressSeconds += seconds > 0.1 ? 0.05 : seconds;
      if (mLongPressSeconds >= mLongPressTimeout) {
        // If long touch returns true, eat the next touch event,
        // otherwise, reset for another long touch after a delay
        if (OnLongTouch(mTouchStart[0], mTouchStart[1]) ||
            mFrameVec[mTouchFrameIdx]->OnLongTouch(mTouchStart[0],
                                                   mTouchStart[1])) {
          mTouchFrameIdx = -1;
          mMovedAfterDown = false;
          mMovedOffAxisAfterDown = false;
        } else {
          // FIXME! This works from the POV of the Flinglist, but Glaze is
          // advancing the filmstrip using Snap() and that should change the
          // mTouchFrameIdx for the current event, but it doesn't because we
          // didn't get any events. Maybe we need to re-compute mTouchFrameIdx
          // here in Step based on the last touch location? Dangerous.
          // The result is that we call OnLongTouch each time, but the
          // frame we call it on is incorrect (e.g. no longer a peekaboo).
          mLongPressSeconds = 0;
        }
      }
    }
  } else {
    mLongPressSeconds = 0;
  }

  return true;
}


// Returns true if the widget is done animating

bool FlinglistImpl::Dormant() const {
  return mScrollVelocity == 0 && mScrollBounce == 0 && mThumbFade == 0 &&
         mSnapSeconds == 0 && mTouchFrameIdx == -1 && !mGlowDragHandle;
}


// Find the specified frame and set it as a snap (rubber band) target

bool FlinglistImpl::Snap(const tui::FlinglistImpl::Frame *frame, float seconds){
  for (size_t i = 0; i < mFrameVec.size(); ++i) {
    if (mFrameVec[i] == frame) {
      SnapIdx(i, seconds);
      return true;
    }
  }
  return false;
}


bool FlinglistImpl::CancelSnap() {
  if (mSnapSeconds == 0)
    return false;
  // Snap to the next frame in the current direction
  int idx = round((mScrollOffset - mSnapLocationOffset) / mFrameDim);
  if (idx > 0 && mSnapTargetOffset < mScrollOffset - 100)
    idx--;
  else if (idx < Size() - 1 && mSnapTargetOffset > mScrollOffset + 100)
    idx++;
  SnapIdx(idx, 0.25);
  return true;
}


// Offset slightly to one side and snap it back to the current location.
// Used to indicate re-selection of the current frame.

bool FlinglistImpl::Jiggle() {
  mSnapTargetOffset = mScrollOffset;
  mSnapSeconds = mSnapRemainingSeconds = kJiggleSeconds;
  mScrollOffset += kJiggleMm * mPixelsPerCm / 10;
  mScrollVelocity = 0;
  mSnapOriginalOffset = mScrollOffset;
  return true;
}


// Event processing for Flinglist (and Filmstrip).  Manage touch events
// to support dragging of the list and manage the overdraw and thumb animations.

bool FlinglistImpl::Touch(const Event &event) {
  if (!Enabled() || Hidden() || Size() == 0)
    return false;
  
  // Only process single-touch events for flinglists
  if (event.touchVec.empty()) {
    assert(event.phase == TOUCH_CANCELLED);
    mTouchFrameIdx = -1;
    mMovedAfterDown = false;
    mMovedOffAxisAfterDown = false;
    mLongPressSeconds = 0;
    if (fabsf(mScrollVelocity) < 0.1)
      mScrollVelocity = 0;
  } else if (event.touchVec.size() > 1) {
    mMovedAfterDown = false;
    mMovedOffAxisAfterDown = false;
  } else {
    
    int xy[2] = { event.touchVec[0].x, event.touchVec[0].y };
    int xory = mVertical ? 1 : 0;
    switch (event.phase) {
      case TOUCH_BEGAN:
        if (Inside(xy[0], xy[1]) && mTouchFrameIdx < 0) {
          mTouchFrameIdx = FindFrameIdx(xy[0], xy[1]);
          mTouchStart[0] = xy[0];
          mTouchStart[1] = xy[1];
          mScrollVelocity = 0;
          mThumbFade = 0;
          mScrollBounce = 0;
        }
        if (mTouchFrameIdx >= 0) {
          mSnapSeconds = mSnapRemainingSeconds = 0;
          mSnapOriginalOffset = mSnapTargetOffset = 0;
          mLongPressSeconds = 0.0001;
          OnTouchBegan(event.touchVec[0]);
          mFrameVec[mTouchFrameIdx]->OnTouchBegan(event.touchVec[0]);
        }
        break;
      case TOUCH_MOVED: {
        if (mTouchFrameIdx < 0)
          break;
        float d = xy[xory] - mTouchStart[xory];
        if (fabs(d) > mPixelsPerCm / 10 * kDragMm) {
          mMovedAfterDown = true;
          mLongPressSeconds = 0;
        }
        int dOffAxis = abs(xy[!xory] - mTouchStart[!xory]);
        if (!mMovedAfterDown && dOffAxis > mPixelsPerCm / 10 * kDragMm)
          mMovedOffAxisAfterDown = true;
        if (mMovedOffAxisAfterDown &&
            OnOffAxisMove(event.touchVec[0], mTouchStart[0], mTouchStart[1]))
          return true;
        if (!mMovedAfterDown)
          break;
        if (!mIsLocked) {
          mScrollVelocity = 0.5 * (d + mScrollVelocity);
          if (mScrollVelocity > mFrameDim / 2)
            mScrollVelocity = mFrameDim / 2;
          else if (mScrollVelocity < -mFrameDim / 2)
            mScrollVelocity = -mFrameDim / 2;
          float offset = mScrollOffset + d;
          const bool scrollback = false; // allow bounce scrolling
          if (offset < ScrollMin()) {
            mScrollOffset = ScrollMin();
            if (scrollback && offset - ScrollMin() > mScrollBounce) {
              mScrollBounce = mScrollVelocity = mThumbFade = 0;
              mTouchStart[0] = xy[0];
              mTouchStart[1] = xy[1];
            } else {
              mScrollBounce = offset - ScrollMin();
              mThumbFade = 1;
              mScrollVelocity = 0;
            }
          } else if (offset > ScrollMax()) {
            mScrollOffset = ScrollMax();
            if (scrollback && offset - ScrollMax() < mScrollBounce) {
              mScrollBounce = mScrollVelocity = mThumbFade = 0;
              mTouchStart[0] = xy[0];
              mTouchStart[1] = xy[1];
            } else {
              mScrollBounce = offset - ScrollMax();
              mThumbFade = 1;
              mScrollVelocity = 0;
            }
          } else {
            mScrollOffset = offset;
            mTouchStart[0] = xy[0];
            mTouchStart[1] = xy[1];
          }
        }
        if (!mMovedAfterDown)
          mScrollBounce  = mScrollVelocity = 0;
        OnMove();
        break;
      }
      case TOUCH_CANCELLED: /*FALLTHRU*/
      case TOUCH_ENDED: {
        bool tapStatus = mTouchFrameIdx >= 0;
        if (mTouchFrameIdx >= 0 && !MovedAfterDown())
          tapStatus = mFrameVec[mTouchFrameIdx]->OnTouchTap(event.touchVec[0]);
        if (mOverpullOnTex && mScrollBounce < OverpullPixels()) {
          OnOverpullRelease();
          mScrollBounce = 0;
        }
        OnTouchEnded();
        mTouchFrameIdx = -1;
        mMovedAfterDown = false;
        mMovedOffAxisAfterDown = false;
        mLongPressSeconds = 0;
        if (fabsf(mScrollVelocity) < 0.1)
          mScrollVelocity = 0;
        // Compute the snap-to frame as needed, so we don't go dormant
        if (mSnapToCenter && mTouchFrameIdx < 0 && mSnapSeconds == 0 &&
            (mSingleFrameFling || fabsf(mScrollVelocity) < kSnapVelocity)) {
          SnapIdx(FlingingSnapIdx(), 0.15);
        }
        if (tapStatus)
          return true;
        break;
      }
    }
  }
  
  // Absorb the event (return true) if any frame is selected
  return mTouchFrameIdx != -1;
}


int FlinglistImpl::FindFrameIdx(int x, int y) const {
  if (x < mViewport[0] || x > Right())
    return -1;
  int i = -1;
  if (mVertical)
    i = trunc((Top() - y + mScrollOffset) / mFrameDim);
  else 
    i = trunc((Right() - x + mScrollOffset) / mFrameDim);
  if (i < 0 || i > int(mFrameVec.size()) - 1)
    return -1;
  return i;
}


void FlinglistImpl::SnapIdx(size_t idx, float seconds) {
  assert(idx >= 0 && idx < Size());
  mSnapTargetOffset = ScrollToOffset(idx);
  if (mSnapTargetOffset != mScrollOffset) {
    if (seconds <= 0) {
      mScrollOffset = mSnapTargetOffset;
      mSnapTargetOffset = mSnapOriginalOffset = 0;
      mSnapSeconds = mSnapRemainingSeconds = 0;
    } else {
      mSnapOriginalOffset = mScrollOffset;
      mSnapSeconds = mSnapRemainingSeconds = seconds;
    }
    mScrollVelocity = 0;  // Effectively disables flinging in favor of snapping
  } else {
    mSnapTargetOffset = 0;
  }
}


// Determine snap frame based on the final offset

int FlinglistImpl::FlingingSnapIdx() const {
  int idx;
  
  if (mSingleFrameFling && mScrollVelocity > kSnapVelocity) {
    idx = floor(mScrollOffset / mFrameDim);
    idx += idx < Size() - 1;
  } else if (mSingleFrameFling && mScrollVelocity < -kSnapVelocity) {
    idx = ceil(mScrollOffset / mFrameDim);
    idx -= idx > 0;
  } else {
    // If we're not moving fast enough, use middle of frame to snap
    float centerOffset = mScrollOffset + (mVertical ? Height() : Width()) / 2;
    idx = floor((centerOffset - mSnapLocationOffset) / mFrameDim);      
  }
  if (idx < 0)
    idx = 0;
  else if (Size() && idx >= Size())
    idx = Size() - 1;
  
  return idx;
}


//
// FilmstripImpl
//

bool FilmstripImpl::SetViewport(int x, int y, int w, int h) {
  if (!FlinglistImpl::SetViewport(x, y, w, h))
    return false;
  if (mPZFrame)
    mPZFrame->SetViewport(x, y, w, h);
  return true;
}


void FilmstripImpl::SetPinchAndZoom(tui::Frame *frame) {
  mPZFrame = frame;
  if (mPZFrame) {
    mPZFrame->SetViewport(Left(), Bottom(), Width(), Height());
    mPZFrame->SetOverpullDeceleration(1);
    mPZFrame->Lock(mVertical, !mVertical, false);
  }
}


bool FilmstripImpl::Touch(const tui::Event &event) {
  if (Hidden() || !Enabled())
    return false;
  
  if (mPZFrame && mPZFrame->Enabled() && !mMovedAfterDown &&
      !mMovedOffAxisAfterDown) {
    float ds = mPZFrame->Scale() - mPZFrame->FitScale();
    bool isFit = fabs(ds) < 0.001;
    if (!isFit)
      mPZFrame->Lock(false, false, false);
    else
      mPZFrame->Lock(false, true, false);
    mIsPZEvent = mPZFrame->Touch(event);

    switch (event.phase) {
      case TOUCH_BEGAN:
        mPZStartScrollOffset = mScrollOffset;
        break;
        
      case TOUCH_MOVED:
        if (mIsPZEvent && mPZFrame->IsDragging() && !mPZFrame->IsScaling() &&
            isFit) {
          mPZFrame->SnapToFitFrame();
          mPZFrame->Touch(Event(TOUCH_CANCELLED));
          mIsPZEvent = false;
        } else if (mIsPZEvent) {
          FlinglistImpl::Touch(Event(TOUCH_CANCELLED));
          mScrollVelocity = 0;
          if (!mIsLocked) {
            float overpull = mPZFrame->XOverpull();
            float offset = mPZStartScrollOffset + overpull;
            if (offset < ScrollMin()) {
              mScrollOffset = ScrollMin();
              mScrollBounce = offset - ScrollMin();
              mThumbFade = 1;
            } else if (offset > ScrollMax()) {
              mScrollOffset = ScrollMax();
              mScrollBounce = offset - ScrollMax();
              mThumbFade = 1;
            } else {
              mScrollOffset = offset;
            }
          }
        }
        break;
        
      case TOUCH_ENDED:
        // Decide if we snap or let PZFrame handle movement
        if (mIsPZEvent && event.IsDone()) {
          OnTouchEnded();
          float overpull = mPZFrame->XOverpull();
          if (!IsLocked() && mOverpullOnTex &&
              mScrollBounce < OverpullPixels()) {
            OnOverpullRelease();
            mScrollBounce = 0;
          } else if (!IsLocked() && overpull > mFrameDim/2 &&
                     mSelectedFrameIdx < mFrameVec.size() - 1) {
            Snap(mFrameVec[mSelectedFrameIdx + 1], 0.15);
            mIsPZEvent = false;
          } else if (!IsLocked() && overpull < -mFrameDim/2 &&
                     mSelectedFrameIdx > 0) {
            Snap(mFrameVec[mSelectedFrameIdx - 1], 0.15);
            mIsPZEvent = false;
          } else if (!mPZFrame->IsAnimating()) {
            mPZFrame->SnapToUVCenter(mPZFrame->UCenter(), mPZFrame->VCenter(),
                                     true);   // Sets dirty flag for animation
          }
        }
        break;
        
      case TOUCH_CANCELLED:
        break;
    }
    
    if (mIsPZEvent) {
      mLongPressSeconds = 0;
      return true;
    }
  }
  
  return FlinglistImpl::Touch(event);
}


bool FilmstripImpl::Step(float seconds) {
  if (mPZFrame) {
    if (!mPZFrame->Step(seconds))
      return false;
    if (!mIsLocked && mIsPZEvent) {
      mScrollVelocity = 0;
      float overpull = mPZFrame->XOverpull();
      float offset = mPZStartScrollOffset + overpull;
      if (offset < ScrollMin()) {
        mScrollOffset = ScrollMin();
        mScrollBounce = offset - ScrollMin();
        mThumbFade = 1;
      } else if (offset > ScrollMax()) {
        mScrollOffset = ScrollMax();
        mScrollBounce = offset - ScrollMax();
        mThumbFade = 1;
      } else {
        mScrollOffset = offset;
      }
    }
    if (mPZFrame->IsSnapping())
      mLongPressSeconds = 0;      // Cancel OnLongTouch
    if (mPZFrame->IsScaling() || mPZFrame->IsDragging())
      return true;
  }
  if (!FlinglistImpl::Step(seconds))
    return false;
  return true;
}


bool FilmstripImpl::DrawFrame(FlinglistImpl::Frame *frame) {
  if (mPZFrame && mSelectedFrameIdx >= 0 &&
      mFrameVec[mSelectedFrameIdx] == frame &&
      (mPZFrame->Scale() != mPZFrame->FitScale() ||
       mPZFrame->IsScaling() || mPZFrame->IsDragging())) {
    glViewport(mPZFrame->Left(), mPZFrame->Bottom(),
               mPZFrame->Width(), mPZFrame->Height());
    if (!mPZFrame->Draw())
      return false;
    return true;
  }
  
  return FlinglistImpl::DrawFrame(frame);
}


bool FilmstripImpl::Dormant() const {
  if (!FlinglistImpl::Dormant())
    return false;
  if (mPZFrame && !mPZFrame->Dormant())
    return false;
  return true;
}


bool FilmstripImpl::Prepend(FlinglistImpl::Frame *frame) {
  if (!FlinglistImpl::Prepend(frame))
    return false;
  if (mSelectedFrameIdx >= 0) {
    mSelectedFrameIdx++;
    OnSelectionChanged();
    if (mPZFrame)
      mPZFrame->SnapToFitFrame();
  }
  
  return true;
}


bool FilmstripImpl::Delete(FlinglistImpl::Frame *frame) {
  int idx = FrameIdx(frame);
  if (idx < 0)
    return false;
  if (mSelectedFrameIdx >= idx) {
    if (mSelectedFrameIdx > 0 || Size() == 1)
      SnapIdx(mSelectedFrameIdx - 1, 0);
  }
  return FlinglistImpl::Delete(frame);
}


void FilmstripImpl::SnapIdx(size_t idx, float seconds) {
  assert(idx >= 0 && idx < Size());
  if (mSelectedFrameIdx != idx) {
    mSelectedFrameIdx = idx;
    OnSelectionChanged();
    if (mPZFrame)
      mPZFrame->SnapToFitFrame();
  }
  FlinglistImpl::SnapIdx(idx, seconds);
}


//
// Frame
//

bool Frame::Reset() {
  ComputeScaleRange();
  return true;
}


float Frame::XOverpull() const {
  if (mSnapMode != SNAP_CENTER)               // FIXME: Handle all modes!
    return 0;

  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  x0 = x0 < -1 ? -1 : (x0 > 1 ? 1 : x0);
  x1 = x1 < -1 ? -1 : (x1 > 1 ? 1 : x1);

  float dx0 = x0 + 1;
  float dx1 = 1 - x1;
  float d = (dx0 - dx1) / 2;
  
  return Width() * d;
}


float Frame::YOverpull() const {
  abort();
  return 0;
}


float Frame::FitScale() const {
  // Compute the scale so that the entire image fits within
  // the screen boundary, comparing the aspect ratios of the
  // screen and the image and fitting to the proper axis.
  const float frameAspect = Width() / float(Height());
  const float imageAspect = ImageWidth() / float(ImageHeight());
  const float frameToImageRatio = frameAspect / imageAspect;
  
  float scale;
  if (frameToImageRatio < 1)                  // Frame narrower than image
    scale = (Width() - 2 * mViewportMinScalePad) / float(ImageWidth());
  else                                        // Image narrower than frame
    scale = (Height() - 2 * mViewportMinScalePad) / float(ImageHeight());
  return scale;
}


void Frame::ComputeScaleRange() {
  if (!ImageWidth() || !ImageHeight()) {
    mScaleMin = mScaleMax = 0;                // Zero to force reset on load
    return;
  }
  
  mScaleMin = FitScale();
  mScaleMax = std::max(32.0f, mScaleMin * 4); // Increase past 32 smoothly
//  if (mScaleMin > 1)
//    mScaleMin = 1;

  assert(mScaleMin < mScaleMax);
}


bool Frame::SetViewport(int x, int y, int w, int h) {
  if (!AnimatedViewport::SetViewport(x, y, w, h))
    return false;
  if (!Reset())
    return false;
  return true;
}


bool Frame::Step(float seconds) {
  if (seconds == 0)
    return true;
  if (!ImageWidth() || !ImageHeight())
    return true;
  if (IsDragging() || IsScaling())
    return true;                              // No motion during touch events
  if (mIsDirty)
    mIsDirty = false;

  seconds = std::min(seconds, 0.1f);          // clamp to avoid debugging issues
  
  // Apply inertial scaling
  mScaleVelocity *= kScaleDamping;
  if (mIsScaleLocked || fabsf(mScaleVelocity) < 0.01)
    mScaleVelocity = 0;
  else if (!IsScaling() && !mIsTargetScaleActive)
    mScale += mScaleVelocity * seconds;

  if (mIsScaleLocked) {
    mIsTargetScaleActive = false;
    mTargetScale = 0;
  } else if (mScale > mScaleMax /* && mScaleVelocity > 0.01*/) {
    mTargetScale = mScaleMax;
    mIsTargetScaleActive = true;
  } else if (mScale < mScaleMin /* && mScaleVelocity < -0.01*/) {
    mTargetScale = mScaleMin;
    mIsTargetScaleActive = true;
    mIsTargetScaleCenterActive = true;
  }
  
  // Apply target scale if enabled & not actively moving
  if (mIsTargetScaleActive) {
    if (fabs(mScale - mTargetScale) < 0.01) {
      mScale = mTargetScale;
      mScaleVelocity = 0;
      mIsTargetScaleActive = false;
    } else {
      float k = std::min(7 * seconds, 1.0f);
      mScale += k * (mTargetScale - mScale);
    }
  }
  
  // Apply target center if enabled & not actively moving
  if (mIsTargetCenterActive || mIsTargetScaleActive) {
    if (mIsSnapDirty) {
      // Check to see if the target center is past the limit at target scale.
      // We don't want to stop translating, just limit the translation to
      // the closest point along that inset edge of the image, where we just
      // cover the full screen. Without this, we keep bouncing at the edge!
      // We do this here, rather than in the SnapTo* functions to avoid any
      // forced function call ordering. All math is done using the target
      // center and scale, so that we clamp immediately and never overshoot.
      mIsSnapDirty = false;                 // Clamp once, after SnapTo*
      const float scale = mTargetScale == 0 ? mScale : mTargetScale;
      const float x = scale * mTargetCenterUV[0] * ImageWidth();
      const float y = scale * mTargetCenterUV[1] * ImageHeight();
      const float w2 = std::min(scale * ImageWidth(), float(Width())) / 2;
      const float h2 = std::min(scale * ImageHeight(), float(Height())) / 2;
      const float tw = scale * ImageWidth();
      const float th = scale * ImageHeight();
      if (x < w2)
        mTargetCenterUV[0] = (w2 + scale) / tw;
      else if (tw - x < w2)
        mTargetCenterUV[0] = 1 - (w2 + scale) / tw;
      if (y < h2)
        mTargetCenterUV[1] = (h2 + scale) / th;
      else if (th - y < h2)
        mTargetCenterUV[1] = 1 - (h2 + scale) / th;
    }
    bool isMoving = false;
    const size_t dim[2] = { ImageWidth(), ImageHeight() };
    for (size_t i = 0; i < 2; ++i) {
      static const float snapThreshold = 5 / float(dim[i]);
      if (fabs(mCenterUV[i] - mTargetCenterUV[i]) < snapThreshold) {
        mCenterUV[i] = mTargetCenterUV[i];
        mCenterVelocityUV[i] = 0;
      } else {
        isMoving = true;
        float k = std::min(7 * seconds, 1.0f);
        mCenterUV[i] += k * (double(mTargetCenterUV[i]) - mCenterUV[i]);
      }
    }
    if (!isMoving)
      mIsTargetCenterActive = false;
  }
  
  const int iw = ImageWidth();
  const int ih = ImageHeight();

  // Apply inertial panning
  mCenterVelocityUV[0] *= kDragDamping;
  mCenterVelocityUV[1] *= kDragDamping;
  if (fabsf(mCenterVelocityUV[0]) < 1.0 / iw)
    mCenterVelocityUV[0] = 0;
  if (fabsf(mCenterVelocityUV[1]) < 1.0 / ih)
    mCenterVelocityUV[1] = 0;
  if (!IsDragging() && !mIsLocked[0] && mCenterVelocityUV[0] != 0)
    mCenterUV[0] += mCenterVelocityUV[0] * seconds;
  if (!IsDragging() && !mIsLocked[1] && mCenterVelocityUV[1] != 0)
    mCenterUV[1] += mCenterVelocityUV[1] * seconds;

  // Snap the NDC window to specified limit mode
  const int sw = mScale * iw;                           // Screen pixel dim
  const int sh = mScale * ih;
  const float padW = std::max((int(Width()) - sw) / float(Width()), 0.0f);
  const float padH = std::max((int(Height()) - sh) / float(Height()), 0.0f);
  const float pw = clamp(U2Ndc(1.0 / iw), -1.0f, 1.0f); // One pixel in NDC
  const float ph = clamp(V2Ndc(1.0 / ih), -1.0f, 1.0f);
  
  float tx0, ty0, tx1, ty1;                             // Target NDC window
  switch (mSnapMode) {
    case SNAP_CENTER:     tx0 = -1 + padW;        ty0 = -1 + padH;
      tx1 = 1 - padW;         ty1 = 1 - padH;       break;
    case SNAP_UPPER_LEFT: tx0 = -1;               ty0 = -1 + 2 * padH;
      tx1 = 1 - 2 * padW;     ty1 = 1;              break;
    case SNAP_PIXEL:      tx0 = -pw/2;            ty0 = -ph/2;
      tx1 = pw/2;             ty1 = ph/2;           break;
    case SNAP_NDC_RECT:   tx0 = mSnapNDCRect[0];  ty0 = mSnapNDCRect[1];
      tx1 = mSnapNDCRect[2];  ty1 = mSnapNDCRect[3];break;
  }
  
  // Compute the current window and compare edges to target window
  // converting the distance from the edges in NDC into UV coords.
  // When the crop window is smaller than the image, we keep it inside,
  // and when the crop window is larger, we keep the image inside the crop.
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  float su = 0.75, sv = 0.75;                           // UV Scaling
  float du = 0, dv = 0;                                 // Change in UV
  const bool isWider = tx1 - tx0 > x1 - x0;             // Crop wider image?
  if (mSnapMode == SNAP_NDC_RECT && isWider) {          // Image inside crop
    if (x0 < tx0)                                       // Left
      du = Ndc2U(x0 - tx0);
    else if (x1 > tx1)                                  // Right
      du = Ndc2U(x1 - tx1);
  } else {                                              // Crop inside image
    if (x0 > tx0)                                       // Left
      du = Ndc2U(x0 - tx0);
    else if (x1 < tx1)                                  // Right
      du = Ndc2U(x1 - tx1);
  }
  const bool isTaller = ty1 - ty0 > y1 - y0;            // Crop taller image?
  if (mSnapMode == SNAP_NDC_RECT && isTaller) {         // Image inside crop
    if (y0 < ty0)                                       // Bottom
      dv = Ndc2V(y0 - ty0);
    else if (y1 > ty1)                                  // Top
      dv = Ndc2V(y1 - ty1);
  } else {                                              // Crop inside image
    if (y0 > ty0)                                       // Bottom
      dv = Ndc2V(y0 - ty0);
    else if (y1 < ty1)                                  // Top
      dv = Ndc2V(y1 - ty1);
  }
  
  // Snap the central pixel when the velocity falls below a threshold
  const float dUVPixels = 5;                            // pixel threshold
  const float spu = Ndc2U(2.0f / Width()), spv = Ndc2V(2.0f /Height());
  if (mSnapMode == SNAP_PIXEL && du == 0 && dv == 0 &&  // No motion
      fabsf(mCenterVelocityUV[0]) < dUVPixels / iw &&   // No momentum
      fabsf(mCenterVelocityUV[1]) < dUVPixels / ih) {
    const float pix[2] = { mCenterUV[0] * iw, mCenterUV[1] * ih };
    float cpix[2] = { floor(pix[0]), floor(pix[1]) };   // Center pixel
    assert(cpix[0] >= 0 && cpix[0] < iw);               // du == dv == 0
    assert(cpix[1] >= 0 && cpix[1] < ih);
    mCenterVelocityUV[0] = mCenterVelocityUV[1] = 0;    // Force to zero
    du = (0.5 - (pix[0] - cpix[0])) / iw;               // Move to pix center
    dv = -(0.5 -(pix[1] - cpix[1])) / ih;
  }
  
  // Snap target scale to computed final scale.
  // This is a backup, and should not be needed if the target clamp
  // math is correct above, but just in case we're wrong...
  if (!mIsTargetScaleActive) {                          // At final scale?
    if (du != 0)                                        // Clamping U?
      mTargetCenterUV[0] = mCenterUV[0] + du;           // Target clamp U
    if (dv != 0)                                        // Clamping V?
      mTargetCenterUV[1] = mCenterUV[1] + dv;           // Target clamp V
  }
  
  // Snap to the final target position if we are within the threshold
  if (fabsf(du) < dUVPixels * 0.5 * spu)                // Within U thresh?
    su = 1;                                             // Snap to target U
  if (fabsf(dv) < dUVPixels * 0.5 * spv)                // Within V thresh?
    sv = 1;                                             // Snap to target V
  
  // Adjust the center of the current window by moving toward target
  assert(du == 0 || !mIsLocked[0]);
  //    assert(dv == 0 || !mIsLocked[1]);
  assert(!isinf(du) && !isinf(dv) && !isinf(su) && !isinf(sv));
  mCenterUV[0] += su * du;                              // Move toward target
  mCenterUV[1] -= sv * dv;
  
  // Determine if we need to continue moving or if we've hit the target
  if (su < 1 || sv < 1)
    mIsTargetScaleCenterActive = true;
  else
    mIsTargetScaleCenterActive = false;
  mIsDirty = fabsf((1 - su) * du) > 0.5 * spu || fabsf((1 - sv) * dv) > 0.5 * spv;

  return true;
}


bool Frame::Dormant() const {
  if (mIsDirty)
    return false;
  if (mIsTargetCenterActive || mIsTargetScaleCenterActive)
    return false;
  if (mIsTargetScaleActive)
    return false;
  if (mScaleVelocity != 0)
    return false;
  if (mCenterVelocityUV[0] != 0 || mCenterVelocityUV[1] != 0)
    return false;
  return true;
}


// Adjust mScale and offset mCenterUV so that the origin remains
// invariant under scaling.

bool Frame::OnScale(EventPhase phase, float scale, float x, float y,
                    double timestamp) {
  assert(!isnan(scale));
  if (mIsScaleLocked) {
    mScaleVelocity = 0;
    mIsTargetScaleActive = false;
    mTargetScale = 0;
    return false;
  }

  float dscale = 0;
  
  switch (phase) {
    case TOUCH_BEGAN:
      mStartScale = mScale;
      mPrevScale = scale;
      mPrevScaleTimestamp = timestamp;
      mScaleVelocity = 0;
      return true;
      
    case TOUCH_MOVED: {
      dscale = scale - mPrevScale;            // Change from prev touch
      float s = mScale * (1 + dscale);        // Presumed new scale
      if (s < mScaleMin || s > mScaleMax)     // Check range
        dscale *= 0.5;                        // Make it harder to pull
      mScale *= 1 + dscale;                   // Multiply scales
      float dt = timestamp - mPrevScaleTimestamp;
      if (dt > 0.00001)
        dt = 1/dt;
      else
        dt = 0;                               // Avoid divide by zero
      if (dt != 0)
        mScaleVelocity = kScaleFling * dt * dscale;
      mPrevScale = scale;
      mPrevScaleTimestamp = timestamp;
      break;
    }
      
    case TOUCH_ENDED:
    case TOUCH_CANCELLED:
      mIsTargetScaleActive = false;
      mTargetScale = 0;
      return true;
  }

  // Figure out the UV coordinates of the input point in the display rects
  float screenU, screenV;
  float xNDC = 2.0 * x / Width() - 1;
  float yNDC = 2.0 * y / Height() - 1;
  NDCToUV(xNDC, yNDC, &screenU, &screenV);
  
  float du = (mCenterUV[0] - screenU) * dscale;
  float dv = (mCenterUV[1] - screenV) * dscale;
  
  // Move the center of the image toward the origin to keep
  // that point invariant, scaling by velocity (change in scale).
  // TRICKY! Must use relative changes to mCenterUV both here and in
  //         OnDrag because absolute changes would override each other.
  if (!mIsLocked[0])
    mCenterUV[0] -= du;
  if (!mIsLocked[1])
    mCenterUV[1] -= dv;
  
  return true;
}


// Translate the mCenterUV value based on the input pan distances
// in screen pixels.

bool Frame::OnDrag(EventPhase phase, float x, float y, double timestamp) {
  if (phase == TOUCH_BEGAN) {
    mStartCenterUV[0] = mCenterUV[0];
    mStartCenterUV[1] = mCenterUV[1];
    mPrevDragXY[0] = x;
    mPrevDragXY[1] = y;
    mPrevDragTimestamp = timestamp;
    return true;
  }
  
  // Convert velocity from pixel into image UV coordinates
  float dx = (x - mPrevDragXY[0]) / Width();  // Change within uv-rect from prev
  float dy = (y - mPrevDragXY[1]) / Height();
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  float du = -dx * (u1 - u0);
  float dv = -dy * (v1 - v0);
  
  if (mSnapMode == SNAP_CENTER) {             // FIXME: Handle all modes!
    if (fabsf(x0) != fabsf(x1))               // Translated off-center
      du *= mOverpullDeceleration;
    if (fabsf(y0) != fabsf(y1))
      dv *= mOverpullDeceleration;
  }

  if (phase == TOUCH_MOVED) {                 // Update position on move
    if (!mIsLocked[0])
      mCenterUV[0] += du;
    if (!mIsLocked[1])
      mCenterUV[1] += dv;
    float dt = timestamp - mPrevDragTimestamp;
    if (dt > 0.00001)
      dt = 1/dt;
    else
      dt = 0;                                 // Avoid divide by zero
    if (dt != 0) {
      mCenterVelocityUV[0] = kDragFling * dt * du;
      mCenterVelocityUV[1] = kDragFling * dt * dv;
    }
    mPrevDragXY[0] = x;
    mPrevDragXY[1] = y;
    mPrevDragTimestamp = timestamp;
  }
    
  return true;
}


void Frame::OnTouchBegan(const tui::Event::Touch &touch) {
  mCenterVelocityUV[0] = 0;                   // Stop animation
  mCenterVelocityUV[1] = 0;
  mScaleVelocity = 0;
}


void Frame::OnTouchEnded(const Event::Touch &touch) {
  mIsDirty = true;                            // Snap to center if touch dormant
}


bool Frame::OnDoubleTap(const Event::Touch &touch) {
  float min = MinScale();                     // Zoom from min -> 1:1 -> 4x min
  float fit = FitScale();                     // Special case for low rez images
  float target = min < fit ? fit : std::max(4 * min, 1.0f);
  float t = (Scale() - min) / (target - min);
  if (t < 0.5) {                              // Closer to the target or fit?
    float x = 2 * touch.x / float(Width()) - 1;
    float y = 2 * touch.y / float(Height()) - 1;
    float u, v;
    NDCToUV(x, y, &u, &v);
    SnapToUVCenter(u, v, true);
    SnapToScale(target, true);
  } else {
    SnapToScale(min, true);                   // 1:1 or fit screen
  }
  
  return true;
}


// Center the image and compute the scaling required to fit the image
// within the current frame.

void Frame::SnapToFitFrame(bool isAnimated) {
  ComputeScaleRange();                        // Recompute if needed
  if (isAnimated) {
    mTargetCenterUV[0] = 0.5;
    mTargetCenterUV[1] = 0.5;
    mTargetScale = FitScale();
    mIsTargetCenterActive = true;
    mIsTargetScaleActive = true;
    mIsSnapDirty = true;
  } else {
    ResetView();                              // Initialize all view controls
    CancelMotion();                           // Stop any motion
  }
}


void Frame::ResetView() {
  mCenterUV[0] = 0.5;                         // Center image
  mCenterUV[1] = 0.5;
  mScale = mScaleMin == 0 ? 1 : FitScale();   // Fit image to screen
}


void Frame::CancelMotion() {
  mScaleVelocity = 0;
  mCenterVelocityUV[0] = 0;
  mCenterVelocityUV[1] = 0;
  mTargetScale = 0;
  mTargetCenterUV[0] = mTargetCenterUV[1] = 0;
  mIsTargetCenterActive = false;
  mIsTargetScaleCenterActive = false;
  mIsTargetScaleActive = false;
}


// Center the image and compute the scaling required to fit the image
// across the current frame, offsetting the y location using v=[0,1]

void Frame::SnapToFitWidth(float v, bool isAnimated) {
  ComputeScaleRange();
  CancelMotion();
  if (!Width() || !ImageWidth()) {
    ResetView();
    return;
  }
  mScale = Width() / float(ImageWidth());
  const float h2 = std::min(0.5f * Height() / (mScale * ImageHeight()), 0.5f);
  float v2 = clamp(v, h2, 1 - h2);
  if (isAnimated) {
    SnapToUVCenter(mCenterUV[0], v2, isAnimated);
  } else {
    mCenterUV[0] = 0.5;
    mCenterUV[1] = v2;
  }
}


void Frame::SnapToFitHeight(float u, bool isAnimated) {
  ComputeScaleRange();
  CancelMotion();
  if (!Height() || !ImageHeight()) {
    ResetView();
    return;
  }
  mScale = Height() / float(ImageHeight());
  const float w2 = std::min(0.5f * Width() / (mScale * ImageWidth()), 0.5f);
  float u2 = clamp(u, w2, 1 - w2);
  if (isAnimated) {
    SnapToUVCenter(u2, mCenterUV[1]);
  } else {
    mCenterUV[0] = u2;
    mCenterUV[1] = 0.5;
  }
}


void Frame::SnapToUVCenter(float u, float v, bool isAnimated) {
  ComputeScaleRange();
  if (isAnimated) {
    mIsTargetCenterActive = true;
    mTargetCenterUV[0] = u;                   // Clamped in step w/target scale
    mTargetCenterUV[1] = v;
    mIsSnapDirty = true;
  } else {                                    // Clamp now with current scale
    const float w2 = std::min(0.5f * Width() / (mScale * ImageWidth()), 0.5f);
    const float h2 = std::min(0.5f * Height() / (mScale * ImageHeight()), 0.5f);
    float u2 = clamp(u, w2, 1 - w2);
    float v2 = clamp(v, h2, 1 - h2);
    mCenterUV[0] = u2;
    mCenterUV[1] = v2;
//    CancelMotion();
  }
}


void Frame::SnapToScale(float scale, bool isAnimated) {
  ComputeScaleRange();
  if (isAnimated) {
    mTargetScale = scale;
    mIsTargetScaleActive = true;
    mIsSnapDirty = true;
  } else {
    mScale = scale;
    CancelMotion();
  }
}


void Frame::SnapToLimits(bool isAnimated) {
  ComputeScaleRange();
  if (Scale() < MinScale())
    SnapToScale(MinScale(), isAnimated);
  else if (Scale() > MaxScale())
    SnapToScale(MaxScale(), isAnimated);
}


// Helper functions to convert magnitude in image UV space to/from NDC units.

float Frame::U2Ndc(float u) const {
  return 2 * u * mScale * ImageWidth() / Width();
}


float Frame::V2Ndc(float v) const {
  return 2 * v * mScale * ImageHeight() / Height();
}


float Frame::Ndc2U(float x) const {
  return x / (2 * mScale * ImageWidth() / Width());
}


float Frame::Ndc2V(float y) const {
  return y / (2 * mScale * ImageHeight() / Height());
}


// Compute the NDC and UV rectangles needed to render the current
// frame using mScale and mCenterUV. We compute these on-demand
// rather than storing them to ensure that they are always valid
// and we only need to adjust mScale and mCenterUV when moving.

void Frame::ComputeDisplayRect(float *x0, float *y0, float *x1, float *y1,
                               float *u0, float *v0, float *u1, float *v1) const {
  if (Width() == 0 || Height() == 0) {        // Avoid inf
    *x0 = *y0 = *x1 = *y1 = 0;
    *u0 = *v0 = *u1 = *v1 = 0;
    return;
  }
  
  int sw = mScale * ImageWidth();
  int sh = mScale * ImageHeight();
  float halfWidthUV = std::min(0.5f * Width() / sw, 0.5f);
  float halfHeightUV = std::min(0.5f * Height() / sh, 0.5f);
  float padW = (int(Width()) - sw) / float(Width());
  float padH = (int(Height()) - sh) / float(Height());
  bool isWider = sw >= Width();
  bool isTaller = sh >= Height();
  if (isWider) {                              // Image wider than screen
    *x0 = -1;                                 // Fill entire screen width
    *x1 = 1;
    *u0 = mCenterUV[0] - halfWidthUV;         // Fill the UV rect around center
    *u1 = mCenterUV[0] + halfWidthUV;
    if (*u0 < 0) {                            // Adjust NDC if UV out of range
      const float inset = U2Ndc(-1 * *u0);
      *x0 += inset;
      *u0 = 0;
      if (*u1 < *u0)
        *u1 = *u0;
    }
    if (*u1 > 1) {                            // Adjust NDC if UV out of range
      const float inset = U2Ndc(*u1 - 1);
      *x1 -= inset;
      *u1 = 1;
      if (*u0 > *u1)
        *u0 = *u1;
    }
  } else if (mSnapMode == SNAP_UPPER_LEFT) {  // Image narrower, snap to corner
    float offset = U2Ndc(0.5 - mCenterUV[0]); // Off-center adjustment
    *x0 = -1 + offset;
    *x1 = 1 - 2 * padW + offset;
    *u0 = 0;
    *u1 = 1;
  } else {                                    // Image narrower than screen
    float offset = U2Ndc(0.5 - mCenterUV[0]); // Off-center adjustment
    assert(!isinf(offset) && !isnan(offset));
    *x0 = -1 + padW + offset;                 // Pad left & right edges
    *x1 = 1 - padW + offset;
    *u0 = 0;
    *u1 = 1;
  }
  
  if (isTaller) {                             // Image taller than screen
    *y0 = -1;                                 // Fill entire screen height
    *y1 = 1;
    *v0 = mCenterUV[1] - halfHeightUV;        // Fill the UV rect around center
    *v1 = mCenterUV[1] + halfHeightUV;
    if (*v0 < 0) {                            // Adjust NDC if UV out of range
      const float inset = V2Ndc(-1 * *v0);
      *y1 -= inset;
      *v0 = 0;
      if (*v1 < *v0)
        *v1 = *v0;
    }
    if (*v1 > 1) {                            // Adjust NDC if UV out of range
      const float inset = V2Ndc(*v1 - 1);
      *y0 += inset;
      *v1 = 1;
      if (*v0 > *v1)
        *v0 = *v1;
    }
  } else if (mSnapMode == SNAP_UPPER_LEFT) {  // Image narrower, snap to corner
    float offset = V2Ndc(0.5 - mCenterUV[1]); // Off-center adjustment
    *y0 = -1 + 2 * padH - offset;
    *y1 = 1 - offset;
    *v0 = 0;
    *v1 = 1;
  } else  {                                    // Image shorter than screen
    float offset = V2Ndc(0.5 - mCenterUV[1]);  // Off-center adjustment
    *y0 = -1 + padH - offset;                  // Pad the top & bottom edges
    *y1 = 1 - padH - offset;
    *v0 = 0;
    *v1 = 1;
  }
  
  std::swap(*v0, *v1);                        // OpenGL coordinate flip!
}


// Convert a point in screen NDC space into image UV

void Frame::NDCToUV(float x, float y, float *u, float *v) {
  // Compute the visible NDC & UV display rectangles of the image
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  
  float sNDC = (x - x0) / (x1 - x0);
  float tNDC = (y - y0) / (y1 - y0);
  *u = u0 + sNDC * (u1 - u0);
  *v = v0 + tNDC * (v1 - v0);
  *u = *u < 0 ? 0 : *u > 1 ? 1 : *u;
  *v = *v < 0 ? 0 : *v > 1 ? 1 : *v;
}


// Convert the region in NDC and UV space provided to Frame::Draw into
// a 4x4 matrix (really 2D, so it could be 3x3), that transforms points
// in pixel coordinates in the Frame image into NDC for use as a MVP.

void Frame::RegionToM44f(float dst[16], int imageWidth, int imageHeight,
                         float x0, float y0, float x1, float y1,
                         float u0, float v0, float u1, float v1) {
  Imath::M44f m;
  m.translate(Imath::V3f(-1, -1, 0));                   // -> [-1, 1]
  m.scale(Imath::V3f(2, 2, 1));                         // -> [0, 2]
  m.translate(Imath::V3f((x0 + 1)/2, (y0 + 1)/2, 0));   // -> [x0, y0] in [0,1]
  m.scale(Imath::V3f((x1 - x0)/2, (y1 - y0)/2, 1));     // -> [x0, y0]x[x1, y1]
  m.scale(Imath::V3f(1.0/(u1 - u0), 1.0/(v1 - v0), 1)); // -> [u0, v0]x[u1, v1]
  m.translate(Imath::V3f(-u0, -v0, 0));                 // -> [u0, v0]
  m.translate(Imath::V3f(0, 1, 0));                     // -> [0, 1] offset y
  m.scale(Imath::V3f(1, -1, 1));                        // -> [-1, 0] invert y
  m.scale(Imath::V3f(1.0/imageWidth, 1.0/imageHeight, 1));// texel -> [0, 1]
  memcpy(dst, m.getValue(), 16 * sizeof(float));
}


//
// ButtonGridFrame
//

ButtonGridFrame::ButtonGridFrame() : mButtonHorizCountIdx(0), mButtonDim(0),
                                     mButtonPad(0), mTopPad(0), mBottomPad(0),
                                     mIsViewportDirty(false) {
  memset(mButtonHorizCount, 0, sizeof(mButtonHorizCount));
  memset(mMVPBuf, 0, sizeof(mMVPBuf));
}


bool ButtonGridFrame::Init(int wideCount, int narrowCount,
                           int buttonPad, int topPad, int bottomPad) {
  mButtonHorizCount[0] = wideCount;
  mButtonHorizCount[1] = narrowCount;
  mButtonPad = buttonPad;
  mTopPad = topPad;
  mBottomPad = bottomPad;
  SetMVP(mMVPBuf);
  SnapToFitWidth(0);
  return true;
}


void ButtonGridFrame::Add(tui::Button *button) {
  mButtonVec.push_back(button);
  mIsViewportDirty = true;
}


bool ButtonGridFrame::Remove(tui::Button *button) {
  for (std::vector<tui::Button *>::iterator i = mButtonVec.begin();
       i != mButtonVec.end(); ++i) {
    if (*i == button) {
      mButtonVec.erase(i);
      mIsViewportDirty = true;
      return true;
    }
  }
  return false;
}


void ButtonGridFrame::Destroy() {
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    delete mButtonVec[i];
  Clear();
}


void ButtonGridFrame::Clear() {
  mButtonVec.clear();
  mIsViewportDirty = true;
}


void ButtonGridFrame::VisibleButtonRange(float u0, float v0, float u1, float v1,
                                         int *minIdx, int *maxIdx) {
  assert(u0 == 0 && u1 == 1);                                 // Later...
  
  if (mIsViewportDirty)
    SetViewport(Left(), Bottom(), Width(), Height());         // re-layout

  if (u0 > u1)
    std::swap(u0, u1);
  if (v0 > v1)
    std::swap(v0, v1);
  
  const int t0 = v0 * ImageHeight();                          // visible pixels
  const int t1 = v1 * ImageHeight();
  
  // Button index 0 starts at y=height-(top+dim)
  const int horizCount = mButtonHorizCount[mButtonHorizCountIdx];
  const int lastRow = ButtonCount() / horizCount;
  const int padRow0       = (t0 - mTopPad) / (mButtonDim + mButtonPad);
  const int padRow0Offset = (t0 - mTopPad) % (mButtonDim + mButtonPad);
  const int row0 = clamp(padRow0 + int(padRow0Offset >= mButtonDim), 0, lastRow);
  const int row1 = clamp((t1 - mTopPad) / (mButtonDim + mButtonPad), 0, lastRow);
  
  *minIdx = row0 * horizCount;
  *maxIdx = std::min((row1 + 1) * horizCount - 1, int(ButtonCount()) - 1);
}


void ButtonGridFrame::Sort(const CompareButton &compare) {
  std::sort(mButtonVec.begin(), mButtonVec.end(), compare);
  mIsViewportDirty = true;
}


bool ButtonGridFrame::Snap(size_t i, bool isAnimated) {
  if (i >= ButtonCount())
    return false;
  if (mIsViewportDirty)
    SetViewport(Left(), Bottom(), Width(), Height());         // re-layout

  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  int minIdx, maxIdx;
  VisibleButtonRange(u0, v0, u1, v1, &minIdx, &maxIdx);

  tui::Button *b = Button(i);
  if (!b)
    return false;
  if (IsXLocked()) {
    int ptop = b->Top() + mButtonPad + mTopPad - Height() / 2;
    int pbot = b->Bottom() - mButtonPad - mBottomPad + Height() / 2;
    float vtop = (int(ImageHeight()) - ptop) / float(ImageHeight());
    float vbot = (int(ImageHeight()) - pbot) / float(ImageHeight());
    if (VCenter() > vtop)
      SnapToFitWidth(vtop, isAnimated);
    else if (VCenter() < vbot)
      SnapToFitWidth(vbot, isAnimated);
    else
      SnapToUVCenter(UCenter(), VCenter(), isAnimated);   // Clamp to limits
  } else if (IsYLocked()) {
    int plft = b->Left() - mButtonPad + Width() / 2;
    int prgt = b->Right() + mButtonPad - Width() / 2;
    float ulft = plft / float(ImageWidth());
    float urgt = prgt / float(ImageWidth());
    if (UCenter() > ulft)
      SnapToFitHeight(ulft, isAnimated);
    else if (UCenter() < urgt)
      SnapToFitHeight(urgt, isAnimated);
    else
      SnapToUVCenter(UCenter(), VCenter(), isAnimated);   // Clamp to limits
  }
  return true;
}


bool ButtonGridFrame::SetViewport(int x, int y, int w, int h) {
  if (!Frame::SetViewport(x, y, w, h))
    return false;
  
  // Calculate the new layout and set the image size
  mButtonHorizCountIdx = w > h ? 0 : 1;
  const int hc = mButtonHorizCount[mButtonHorizCountIdx];
  mButtonDim = (w - mButtonPad * (hc + 1)) / hc;
  const int vertCount = ceilf(mButtonVec.size() / float(hc));
  int bh = vertCount * (mButtonPad + mButtonDim) + mTopPad + mBottomPad;
  SetImageDim(Width(), bh);

  // Layout the individual buttons using the new image size
  int px = mButtonPad;
  int py = ImageHeight() - mTopPad - mButtonDim;
  for (size_t i = 0; i < mButtonVec.size(); ++i) {
    if (!mButtonVec[i]->SetViewport(px, py, mButtonDim, mButtonDim))
      return false;
    if ((i + 1) % hc == 0) {
      px = mButtonPad;
      py -= mButtonDim + mButtonPad;
    } else {
      px += mButtonDim + mButtonPad;
    }
  }
  
  mIsViewportDirty = false;
  
  return true;
}


void ButtonGridFrame::SetMVP(const float *mvp) {
  Frame::SetMVP(mvp);
  for (size_t i = 0; i < mButtonVec.size(); ++i)
    mButtonVec[i]->SetMVP(mvp);
}


bool ButtonGridFrame::Touch(const Event &event) {
  if (!Enabled() || Hidden())
    return false;
  
  if (mIsViewportDirty)
    SetViewport(Left(), Bottom(), Width(), Height());         // re-layout

  if (Frame::Touch(event)) {
    for (size_t i = 0; i < mButtonVec.size(); ++i)        // Cancel checkboxes
      mButtonVec[i]->Touch(tui::Event(tui::TOUCH_CANCELLED));
    return true;
  }
  
  if (IsDragging() || IsScaling())
    return false;
  
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  Imath::M44f T;
  const float w = ImageWidth(), h = ImageHeight();
  RegionToM44f(T.getValue(), w, h, x0, y0, x1, y1, u0, v0, u1, v1);
  T.invert();

  Imath::V3f pmin(-1, -1, 0), pmax(1, 1, 0);              // Frame bounds
  Imath::V3f qmin = pmin * T, qmax = pmax * T;            // Transformed to NDC
  
  tui::Event e(event.phase);
  for (size_t i = 0; i < event.touchVec.size(); ++i) {
    // Touch location in NDC coordinates
    Imath::V3f p(2.0 * (event.touchVec[i].x - Left()) / Width() - 1,
                 2.0 * (event.touchVec[i].y - Bottom()) / Height() - 1, 0);
    Imath::V3f q = p * T;         // Convert to pixel coords for all widgets
    bool outside = q.x < qmin.x || q.y < qmin.y || q.x > qmax.x || q.y > qmax.y;
    if (event.phase == tui::TOUCH_BEGAN && outside)       // Not move/end/cancel
      continue;                                           // Clip to bounds
    e.touchVec.push_back(Event::Touch(event.touchVec[i].id, q.x, q.y,
                                      event.touchVec[i].timestamp));
  }

  if (!e.touchVec.empty()) {                              // Any unclipped?
    for (size_t i = 0; i < mButtonVec.size(); ++i) {
      if (mButtonVec[i]->Touch(e))
        return true;
    }
  }
  
  return false;
}


bool ButtonGridFrame::Step(float seconds) {
  if (!ButtonCount())
    return true;
  return tui::Frame::Step(seconds);
}


bool ButtonGridFrame::Draw() {
  if (Hidden())
    return true;
  if (ButtonCount() == 0)
    return true;
  
  if (mIsViewportDirty)
    SetViewport(Left(), Bottom(), Width(), Height());         // re-layout

  // Compute the NDC & UV rectangle for the image and draw
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeDisplayRect(&x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  RegionToM44f(mMVPBuf, ImageWidth(), ImageHeight(), x0,y0,x1,y1, u0,v0,u1,v1);

  int minIdx, maxIdx;
  VisibleButtonRange(u0, v0, u1, v1, &minIdx, &maxIdx);
  
  for (size_t i = minIdx; i <= maxIdx; ++i) {
    glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);    
    if (!mButtonVec[i]->Draw())
      return false;
  }
  
  return true;
}

