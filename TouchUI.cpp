// Copyright (c) 2011 by The 11ers, LLC -- All Rights Reserved

#include "TouchUI.h"

#include "GlesUtil.h"

#include <math.h>
#include <assert.h>


using namespace tui;

const float tui::FlinglistImpl::kLongPressSeconds = 1.0;
const float tui::FlinglistImpl::kJiggleSeconds = 0.2;

unsigned int FlinglistImpl::mFlingProgram = 0;
unsigned int FlinglistImpl::mGlowProgram = 0;
unsigned int Sprite::mSpriteProgram = 0;
int Sprite::mSpriteAP = 0;
int Sprite::mSpriteAUV = 0;
int Sprite::mSpriteUCTex = 0;
int Sprite::mSpriteUOpacity = 0;


//
// Widget
//

static float length(int ax, int ay, int bx, int by) {
  float dx = ax - bx;
  float dy = ay - by;
  return sqrtf(dx * dx + dy * dy);
}


// Should be called by all Widgets that support gestures.
// Filter the event scream tracking the supported gestures,
// including scale and pan, invoking callbacks and passing
// coordinates in screen pixel units.

bool Widget::ProcessGestures(const tui::Event &event) {

  EventPhase trackingPhase = event.phase; // Change if touch ids change
  size_t idx[mTouchStart.size()];         // Vector indices of start touches
  if (mTouchStart.size() != event.touchVec.size()) {
    trackingPhase = TOUCH_BEGAN;          // Restart when # touches changes
  } else {
    for (size_t i = 0; i < mTouchStart.size(); ++i) {
      size_t j = 0;
      for (/*EMPTY*/; j < event.touchVec.size(); ++j) {
        if (event.touchVec[j].id == mTouchStart[i].id)
          break;
      }
      if (j < event.touchVec.size()) {
        idx[i] = j;                       // Identify touch index in vector
      } else {
        trackingPhase = TOUCH_BEGAN;      // Restart if we lose a touch
        break; // Can't find previous touch, start over?
      }
    }
  }
  
  // Localize the two touch events we track
  const Event::Touch &t0(event.touchVec[idx[0]]), &t1(event.touchVec[idx[1]]);

  bool consumed = false;
  
  switch (trackingPhase) {
    case TOUCH_BEGAN:
      mTouchStart.clear();                // Unnecessary?
      mTouchStart = event.touchVec;       // Save initial touch down info
      mIsDragging = false;                // Avoid jump due to mPrevPan tracking
      mIsScaling = false;
      break;
    case TOUCH_MOVED:
      if (mTouchStart.size() > 1) {       // Multitouch processing
        // Compute the pan based on the distance between the segment midpoints
        const float mid0[2] = { 0.5 * (mTouchStart[0].x + mTouchStart[1].x),
                                0.5 * (mTouchStart[0].y + mTouchStart[1].y) };
        const float mid1[2] = { 0.5 * (t0.x + t1.x), 0.5 * (t0.y + t1.y) };
        const float pan[2] = { mid1[0] - mid0[0], mid1[1] - mid0[1] };
        EventPhase gesturePhase = TOUCH_MOVED;
        if (!mIsDragging) {
          if (fabsf(pan[0]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = true;
          } else if (fabsf(pan[1]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = false;
          }
          if (mIsDragging) {
            gesturePhase = TOUCH_BEGAN;
            mPrevPan[0] = pan[0];
            mPrevPan[1] = pan[1];
          }
        }
        if (mIsDragging) {
          mPrevPanVel[0] = pan[0] - mPrevPan[0];
          mPrevPanVel[1] = pan[1] - mPrevPan[1];
          if (OnDrag(gesturePhase, pan[0],pan[1], mPrevPanVel[0], mPrevPanVel[1],
                     mTouchStart[0].x, mTouchStart[0].y))
            consumed = true;
          mPrevPan[0] = pan[0];
          mPrevPan[1] = pan[1];
        }

        // Compute the scale based on the ratio of the segment lengths
        float len0 = length(mTouchStart[0].x, mTouchStart[0].y,
                            mTouchStart[1].x, mTouchStart[1].y);
        float len1 = length(t0.x, t0.y, t1.x, t1.y);
        float scale = len1 / len0;        // Relative scale change
        gesturePhase = TOUCH_MOVED;       // Default to move
        if (!mIsScaling && (scale > 1+kMinScale || scale < 1-kMinScale)) {
          mIsScaling = true;
          gesturePhase = TOUCH_BEGAN;     // First scale event,
          mPrevScale = scale;
        }
        if (mIsScaling) {
          mPrevScaleVel = scale - mPrevScale;
          if (OnScale(gesturePhase, scale, mPrevScaleVel, mid1[0], mid1[1]))
            consumed = true;
          mPrevScale = scale;
        }

      } else {                              // Single-touch processing
        // Translate by the difference between the previous and
        // the current touch locations, accounting for scale.
        const float pan[2] = {t0.x - mTouchStart[0].x, t0.y - mTouchStart[0].y};
        EventPhase gesturePhase = TOUCH_MOVED;
        if (!mIsDragging) {
          if (fabsf(pan[0]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = true;
          } else if (fabsf(pan[1]) > kMinPanPix) {
            mIsDragging = true;
            mIsHorizontalDrag = false;
          }
          gesturePhase = TOUCH_BEGAN;
          mPrevPan[0] = pan[0];
          mPrevPan[1] = pan[1];
        }
        if (mIsDragging) {
          mPrevPanVel[0] = pan[0] - mPrevPan[0];
          mPrevPanVel[1] = pan[1] - mPrevPan[1];
          if (OnDrag(gesturePhase, pan[0],pan[1], mPrevPanVel[0],mPrevPanVel[1],
                     mTouchStart[0].x, mTouchStart[0].y))
            consumed = true;
          mPrevPan[0] = pan[0];
          mPrevPan[1] = pan[1];
        }
      }
      break;
    case TOUCH_CANCELLED: /*FALLTHRU*/
    case TOUCH_ENDED:
      mTouchStart.clear();
      if (mIsScaling) {
        const float mid[2] = { 0.5*(t0.x + t1.x), 0.5*(t0.y + t1.y) };
        mIsScaling = false;
        if (OnScale(TOUCH_ENDED, mPrevScale, mPrevScaleVel, mid[0], mid[1]))
          consumed = true;
      }
      if (mIsDragging) {
        mIsDragging = false;
        if (OnDrag(TOUCH_ENDED, mPrevPan[0], mPrevPan[1], mPrevPanVel[0],
                   mPrevPanVel[1], mTouchStart[0].x, mTouchStart[0].y))
          consumed = true;
      }
      break;
  }
  
  return consumed;
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
  bool err = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (!mWidgetVec[i]->Draw())
      err = true;
  }
  return err;
}


bool Group::Touch(const Event &event) {
  bool consumed = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (mWidgetVec[i]->Touch(event))
      consumed = true;
  }
  return consumed;
}


bool Group::Enabled() const {
  bool enabled = false;
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    if (mWidgetVec[i]->Enabled())
      enabled = true;
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
      hidden = true;
  }
  return hidden;
}


void Group::Hide(bool status) {
  for (size_t i = 0; i < mWidgetVec.size(); ++i)
    mWidgetVec[i]->Hide(status);
}


//
// Viewport
//

bool ViewportWidget::ProcessGestures(const Event &event) {
  if (event.phase == TOUCH_ENDED) {
    for (size_t i = 0; i < event.touchVec.size(); ++i) {
      const Event::Touch &t(event.touchVec[i]);
      for (size_t j = 0; j < TouchStartCount(); ++j) {
        if (t.id == TouchStart(j).id && Inside(t.x, t.y) &&
            Inside(TouchStart(j).x, TouchStart(j).y)) {
          if (OnTapTouch(t))
            break;
        }
      }
    }
  }
  
  return Widget::ProcessGestures(event);
}


//
// Sprite
//

bool Sprite::Init(int x, int y, int w, int h, float opacity,
                  float u0, float v0, float u1, float v1, unsigned int texture){
  if (!texture)
    return false;
  if (!AnimatedViewport::Init(x, y, w, h))
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
    GLuint vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    mSpriteProgram = GlesUtil::CreateProgram(vp, fp, "tui::Sprite");
    if (!mSpriteProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(mSpriteProgram);
    mSpriteAUV = glGetAttribLocation(mSpriteProgram, "aUV");
    mSpriteAP = glGetAttribLocation(mSpriteProgram, "aP");
    mSpriteUCTex = glGetUniformLocation(mSpriteProgram, "uCTex");
    mSpriteUOpacity = glGetUniformLocation(mSpriteProgram, "uOpacity");
    if (GlesUtil::Error())
      return false;
  }
  
  return true;
}


// A simple ease-in-ease out function that is a bit more continuous
// than the usual S-curve function.

static float smootherstep(float v0, float v1, float t) {
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
  mOpacity = smootherstep(mOriginalOpacity, mTargetOpacity, t);
  for (size_t i = 0; i < 4; ++i)
    mViewport[i] = smootherstep(mOriginalViewport[i], mTargetViewport[i], t);
  return true;
}


// Use a custom program to draw the sprite to enable transparency.

bool Sprite::Draw() {
  if (Hidden())
    return true;
  if (mOpacity < 1) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  glViewport(Left(), Bottom(), Width(), Height());
  glUseProgram(mSpriteProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mSpriteTexture);
  glUniform1i(mSpriteUCTex, 0);
  glUniform1f(mSpriteUOpacity, mOpacity);
  if (!GlesUtil::DrawBox2f(mSpriteAP, -1, -1, 1, 1, mSpriteAUV, mSpriteUV[0],
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
// Flinglist
//

bool FlinglistImpl::Init(int x, int y, int w, int h, int frameDim,
                         bool vertical, float pixelsPerCm) {
  if (!AnimatedViewport::Init(x, y, w, h))
    return false;

  mFrameDim = frameDim;
  mScrollableDim = frameDim;
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
    GLuint vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, vp_code);
    if (!vp)
      return false;
    static const char *fp_code =
#if !defined(OSX)
    "precision mediump float;\n"
#endif
    "uniform vec4 uC;\n"
    "void main() { gl_FragColor = uC; }";
    GLuint fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, fp_code);
    if (!vp)
      return false;
    mFlingProgram = GlesUtil::CreateProgram(vp, fp, "tui::Fling");
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
    vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, glow_vp_code);
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
    fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, glow_fp_code);
    if (!fp)
      return false;
    mGlowProgram = GlesUtil::CreateProgram(vp, fp, "tui::FlingGlow");
    if (!mGlowProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
  }
  
  return true;
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


static int clamp(int x, int min, int max) {
  return x < min ? min : x > max ? max : x;
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
    *min = clamp(*min, 0, mFrameVec.size() - 1);
    *max = floor((mScrollOffset + Height()) / float(mFrameDim));
    *max = clamp(*max, 0, mFrameVec.size() - 1);
  } else {
    *min = ceil(mScrollOffset / float(mFrameDim) - 1);
    *min = clamp(*min, 0, mFrameVec.size() - 1);
    *max = floor((mScrollOffset + Width()) / float(mFrameDim));
    *max = clamp(*max, 0, mFrameVec.size() - 1);
  }
  assert(*min <= *max);
  return true;
}


int FlinglistImpl::FrameIdx(const Frame *frame) const {
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


// Draw the visible frames, the over-scrolled region and the scroll thumb.

bool FlinglistImpl::Draw() {
  if (Hidden())
    return true;
  
  // Draw each of the visible frames using a separate viewport and scissor
  int minIdx, maxIdx;
  if (!VisibleFrameRange(&minIdx, &maxIdx))
    return true;

  glEnable(GL_SCISSOR_TEST);

  // Use an inset viewport to allow the strip to draw chrome outside the
  // displayed frame region without changing any of the other viewport-
  // dependent calculations (like the frame viewports).
  const int vp[4] = { mViewport[0] + mViewportInset,
                      mViewport[1] + mViewportInset,
                      mViewport[2] - 2 * mViewportInset,
                      mViewport[3] - 2 * mViewportInset };
  
  for (size_t i = minIdx; i <= maxIdx; ++i) {
    int frameViewport[4] = { 0 };
    if (!Viewport(mFrameVec[i], frameViewport))
      return false;
    int scissor[4] = { frameViewport[0], frameViewport[1],
      frameViewport[2], frameViewport[3] };
    if (scissor[0] < vp[0]) scissor[0] = vp[0];
    if (scissor[1] < vp[1]) scissor[1] = vp[1];
    if (scissor[0] + scissor[2] > vp[0] + vp[2])
      scissor[2] -= scissor[0] + scissor[2] - vp[0] - vp[2];
    if (scissor[1] + scissor[3] > vp[1] + vp[3])
      scissor[3] -= scissor[1] + scissor[3] - vp[1] - vp[3];
    if (scissor[2] <= 0 || scissor[3] <= 0)
      continue;
    glViewport(frameViewport[0], frameViewport[1],
               frameViewport[2], frameViewport[3]);
    glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    if (GlesUtil::Error())
      return false;
    if (!mFrameVec[i]->Draw())
      return false;
  }
  
  // Draw the over-scrolled region with a tinted blue highlight
  if (mScrollBounce != 0) {
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glScissor(vp[0], vp[1], vp[2], vp[3]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glUseProgram(mFlingProgram);
    int uC = glGetUniformLocation(mFlingProgram, "uC");
    glUniform4f(uC, 0, 0.75, 1, 0.25);
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
    if (GlesUtil::Error())
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
  
        if (!GlesUtil::DrawTexture2f(overpullTex, xf0, y0, xf1, y1, 0, 1, 1, 0))
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
    if (!GlesUtil::DrawTexture2f(mDragHandleTex, x, y, x+w, y+h, 0, 1, 1, 0))
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
      if (!GlesUtil::DrawBoxFrame2f(aP, x-gw, y-gh, x+w+gw, y+h+gh, gw, gh, aUV))
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
      if (GlesUtil::Error())
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
    assert(!mSingleFrameFling);
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
      if (fabsf(fabsf(mScrollBounce) < 3))
        mScrollBounce = 0;
    }
  }
  
  if (!MovedAfterDown() && mTouchFrameIdx >= 0 &&
      mScrollOffset == initialScrollOffset) {
    // Only call long touch when we first cross the threshold
    if (mLongPressSeconds < kLongPressSeconds) {
      if (mLongPressSeconds == 0)
        mLongPressSeconds = 0.001;  // non-zero, first touch down, start count
      else
        mLongPressSeconds += seconds;
      if (mLongPressSeconds >= kLongPressSeconds) {
        // If long touch returns true, eat the next touch event,
        // otherwise, reset for another long touch after a delay
        if (mFrameVec[mTouchFrameIdx]->OnLongTouch(mTouchStart)) {
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
  
  for (size_t t = 0; t < event.touchVec.size(); ++t) {
    int xy[2] = { event.touchVec[t].x, event.touchVec[t].y };
    int xory = mVertical ? 1 : 0;
    switch (event.phase) {
      case TOUCH_BEGAN:
        if (Inside(xy[0], xy[1]) && mTouchFrameIdx < 0) {
          mTouchFrameIdx = FindFrameIdx(xy);
          mTouchStart[0] = xy[0];
          mTouchStart[1] = xy[1];
          mScrollVelocity = 0;
          mThumbFade = 0;
          mScrollBounce = 0;
        }
        if (mTouchFrameIdx >= 0) {
          mSnapSeconds = mSnapRemainingSeconds = 0;
          mSnapOriginalOffset = mSnapTargetOffset = 0;
          mLongPressSeconds = 0;
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
            OnOffAxisMove(event.touchVec[t], mTouchStart))
            return true;
        if (!mMovedAfterDown)
          break;
        mScrollVelocity = 0.5 * (d + mScrollVelocity);
        if (mScrollVelocity > mFrameDim / 2)
          mScrollVelocity = mFrameDim / 2;
        else if (mScrollVelocity < -mFrameDim / 2)
          mScrollVelocity = -mFrameDim / 2;
        int offset = mScrollOffset + d;
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
        if (!mMovedAfterDown)
          mScrollBounce  = mScrollVelocity = 0;
        OnMove();
      }
        break;
      case TOUCH_CANCELLED: /*FALLTHRU*/
      case TOUCH_ENDED: {
        bool tapStatus = mTouchFrameIdx >= 0;
        if (mTouchFrameIdx >= 0 && !MovedAfterDown())
          tapStatus = mFrameVec[mTouchFrameIdx]->OnTouchTap(event.touchVec[t]);
        if (mOverpullOnTex && mScrollBounce < OverpullPixels()){
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
      }
        break;
    }
  }
  
  // Absorb the event (return true) if any frame is selected
  return mTouchFrameIdx != -1;
}


int FlinglistImpl::FindFrameIdx(const int *xy) const {
  if (xy[0] < mViewport[0] || xy[0] > Right())
    return -1;
  int i = -1;
  if (mVertical)
    i = trunc((Top() - xy[1] + mScrollOffset) / mFrameDim);
  else 
    i = trunc((Right() - xy[0] + mScrollOffset) / mFrameDim);
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


bool FilmstripImpl::Prepend(Frame *frame) {
  if (!FlinglistImpl::Prepend(frame))
    return false;
  if (mSelectedFrameIdx >= 0) {
    mSelectedFrameIdx++;
    OnSelectionChanged();
  }
  
  return true;
}


bool FilmstripImpl::Delete(Frame *frame) {
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
  }
  FlinglistImpl::SnapIdx(idx, seconds);
}


//
// Buttons
//

int Button::FindPress(size_t id) const {
  for (size_t i = 0; i < mPressVec.size(); ++i)
    if (mPressVec[i].id == id)
      return i;
  return -1;
}


bool Button::Touch(const Event &event) {
  // Don't process events when disabled
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
        if (Inside(touch.x, touch.y)) {
          mPressVec.push_back(Press(touch.id, true, touch.x, touch.y));
          return true;
        }
        break;
      case TOUCH_MOVED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          mPressVec[idx].pressed = Inside(touch.x, touch.y);
          return true;
        }
        break;
      case TOUCH_ENDED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          std::vector<Press>::iterator i = mPressVec.begin() + idx;
          mPressVec.erase(i);
          if (Inside(touch.x, touch.y)) {
            if (InvokeTouchTap(touch))
              return true;
            return true;
          }
        }
        break;
      case TOUCH_CANCELLED:
        idx = FindPress(touch.id);
        if (idx >= 0) {
          std::vector<Press>::iterator i = mPressVec.begin() + idx;
          mPressVec.erase(i);
          return true;
        }
        break;        
    }
  }
  
  return false;
}


bool Button::Pressed() const {
  for (size_t i = 0; i < mPressVec.size(); ++i)
    if (mPressVec[i].pressed)
      return true;
  return false;
}


bool ImageButton::Init(int x, int y, int w, int h, bool blend,
                       unsigned int defaultTexture,unsigned int pressedTexture){
  if (!Button::Init(x, y, w, h))
    return false;
  
  if (defaultTexture == 0 || pressedTexture == 0)
    return false;
  
  mBlendEnabled = blend;
  mDefaultTexture = defaultTexture;
  mPressedTexture = pressedTexture;
  
  return true;
}


bool ImageButton::Draw() {
  if (Hidden())
    return true;
  
  if (mBlendEnabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  glViewport(Left(), Bottom(), Width(), Height());
  
  unsigned int texture = Pressed() ? mPressedTexture : mDefaultTexture;
  if (!GlesUtil::DrawTexture2f(texture, -1, -1, 1, 1, 0, 1, 1, 0))
    return false;
  
  return true;
}


bool CheckboxImageButton::Init(int x, int y, int w, int h, bool blend,
                               unsigned int deselectedTex,
                               unsigned int pressedTex,
                               unsigned int selectedTex) {
  if (!Button::Init(x, y, w, h))
    return false;
  
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    glDisable(GL_BLEND);
  }
  glViewport(Left(), Bottom(), Width(), Height());
  
  unsigned int texture = Pressed() ? mPressedTex : Selected() ?
                                    mSelectedTex : mDeselectedTex;
  if (!GlesUtil::DrawTexture2f(texture, -1, -1, 1, 1, 0, 1, 1, 0))
    return false;
  
  return true;
}


//
// RadioGroup
//

CheckboxButton *RadioGroup::Selected() const {
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mWidgetVec[i]);
    if (cb && cb->Selected())
      return cb;
  }
  return NULL;
}


void RadioGroup::SetSelected(CheckboxButton *button) {
  // Deselect any currently selected buttons
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mWidgetVec[i]);
    if (cb && cb != button && cb->Selected())
      cb->SetSelected(false);
  }
  
  if (!button->Selected())
    button->SetSelected(true);

  if (Selected() == NULL)
    OnNoneSelected();
}


bool RadioGroup::Touch(const Event &event) {
  for (size_t i = 0; i < mWidgetVec.size(); ++i) {
    CheckboxButton *cb = dynamic_cast<CheckboxButton *>(mWidgetVec[i]);
    if (!mIsNoneAllowed && cb->Selected())
      continue;                               // Don't deselect if selected
    if (cb->Touch(event) && cb->Selected()) {
      SetSelected(cb);
      break;
    }
  }
  
  if (Selected() == NULL)
    OnNoneSelected();
  
  return false;
}


//
// FrameViewer
//


void FrameViewer::ComputeScaleRange() {
  if (!mFrame || !mFrame->ImageWidth() || !mFrame->ImageHeight())
    return;
  
  // Compute the scale so that the entire image fits within
  // the screen boundary, comparing the aspect ratios of the
  // screen and the image and fitting to the proper axis.
  const float frameAspect = Width()/float(Height());
  const float imageAspect = mFrame->ImageWidth() / float(mFrame->ImageHeight());
  const float frameToImageRatio = frameAspect / imageAspect;
  
  if (frameToImageRatio < 1) {                // Frame narrower than image
    mScaleMin = Width() / float(mFrame->ImageWidth());
  } else {                                    // Image narrower than frame
    mScaleMin = Height() / float(mFrame->ImageHeight());
  }
  
  mScaleMax = 32;
  assert(mScaleMin < mScaleMax);
}


bool FrameViewer::Init(int x, int y, int w, int h) {
  if (!AnimatedViewport::Init(x, y, w, h))
    return false;
  ComputeScaleRange();
  Step(0.001);                                // Clamp scale and animate
  return true;
}


bool FrameViewer::SetFrame(class Frame *frame) {
  mFrame = frame;
  ComputeScaleRange();
  Step(0.001);                                // Clamp scale and animate
  return true;
}


// Helper functions to convert magnitude in image UV space to/from NDC units.

float FrameViewer::U2Ndc(const class Frame &frame, float u) const {
  return 2 * u * mScale * frame.ImageWidth() / Width();
}


float FrameViewer::V2Ndc(const class Frame &frame, float v) const {
  return 2 * v * mScale * frame.ImageHeight() / Height();
}


float FrameViewer::Ndc2U(const class Frame &frame, float x) const {
  return x / (2 * mScale * frame.ImageWidth() / Width());
}


float FrameViewer::Ndc2V(const class Frame &frame, float y) const {
  return y / (2 * mScale * frame.ImageHeight() / Height());
}


// Compute the NDC and UV rectangles needed to render the current
// frame using mScale and mCenterUV. We compute these on-demand
// rather than storing them to ensure that they are always valid
// and we only need to adjust mScale and mCenterUV when moving.

void FrameViewer::ComputeFrameDisplayRect(const class Frame &frame,
                                          float *x0, float *y0,
                                          float *x1, float *y1,
                                          float *u0, float *v0,
                                          float *u1, float *v1) {
  const size_t sw = mScale * frame.ImageWidth();
  if (sw >= Width()) {                        // Image wider than screen
    *x0 = -1;                                 // Fill entire screen width
    *x1 = 1;
    const float halfWidthUV = std::min(0.5f * Width() / sw, 0.5f);
    *u0 = mCenterUV[0] - halfWidthUV;         // Fill the UV rect around center
    *u1 = mCenterUV[0] + halfWidthUV;
    if (*u0 < 0) {                            // Adjust NDC if UV out of range
      const float inset = U2Ndc(frame, -1 * *u0);
      if (frame.IsHorizontalInverted())
        *x1 -= inset;
      else
        *x0 += inset;
      *u0 = 0;
      if (*u1 < *u0)
        *u1 = *u0;
    }
    if (*u1 > 1) {                            // Adjust NDC if UV out of range
      const float inset = U2Ndc(frame, *u1 - 1);
      if (frame.IsHorizontalInverted())
        *x0 += inset;
      else
        *x1 -= inset;
      *u1 = 1;
      if (*u0 > *u1)
        *u0 = *u1;
    }
  } else {                                    // Image narrower than screen
    const float pad = (Width() - sw) / float(Width());
    *x0 = -1 + pad;                           // Pad left & right edges
    *x1 = 1 - pad;
    const float offset = U2Ndc(frame, 0.5 - mCenterUV[0]);
    *x0 += offset;                            // Allow image to float
    *x1 += offset;
    *u0 = 0;
    *u1 = 1;
  }
  const size_t sh = mScale * frame.ImageHeight();
  if (sh >= Height()) {                       // Image taller than screen
    *y0 = -1;                                 // Fill entire screen height
    *y1 = 1;
    const float halfHeightUV = std::min(0.5f * Height() / sh, 0.5f);
    *v0 = mCenterUV[1] - halfHeightUV;        // Fill the UV rect around center
    *v1 = mCenterUV[1] + halfHeightUV;
    if (*v0 < 0) {                            // Adjust NDC if UV out of range
      const float inset = V2Ndc(frame, -1 * *v0);
      if (frame.IsVerticalInverted())
        *y1 -= inset;
      else
        *y0 += inset;
      *v0 = 0;
      if (*v1 < *v0)
        *v1 = *v0;
    }
    if (*v1 > 1) {                            // Adjust NDC if UV out of range
      const float inset = V2Ndc(frame, *v1 - 1);
      if (frame.IsVerticalInverted())
        *y0 += inset;
      else
        *y1 -= inset;
      *v1 = 1;
      if (*v0 > *v1)
        *v0 = *v1;
    }
  } else {                                    // Image shorter than screen
    const float pad = (Height() - sh) / float(Height());
    *y0 = -1 + pad;                           // Pad the top & bottom edges
    *y1 = 1 - pad;
    const float offset = V2Ndc(frame, 0.5 - mCenterUV[1]);
    *y0 -= offset;                            // Allow image to float
    *y1 -= offset;
    *v0 = 0;
    *v1 = 1;
  }

  // Invert the texture coordinates as specified by the frame
  if (frame.IsHorizontalInverted())
    std::swap(*u0, *u1);
  if (frame.IsVerticalInverted())
    std::swap(*v0, *v1);
}


// Origin and pan are specified in pixel coordinates for the frame.
// The image is rendered in NDC space, which, at scale=1=fit-to-window.

bool FrameViewer::Draw() {
  glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
  glClearColor(0.5, 0.5, 0.5, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  
  if (mFrame) {
    // Compute the NDC & UV rectangle for the image and draw
    float x0, y0, x1, y1, u0, v0, u1, v1;
    ComputeFrameDisplayRect(*mFrame, &x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
    if (!mFrame->Draw(x0, y0, x1, y1, u0, v0, u1, v1))
      return false;
  }
  
  return true;
}


bool FrameViewer::Step(float seconds) {
  if (seconds == 0)
    return true;
  if (!mFrame || !mFrame->ImageWidth() || !mFrame->ImageHeight())
    return true;
  if (mIsDirty)
    mIsDirty = false;

  if (!mFrame->Step(seconds))
    return false;
  
  seconds = std::min(seconds, 0.1f);      // clamp to avoid debugging issues
  
  // Apply inertial scaling
  mScaleVelocity *= 0.85;
  if (fabsf(mScaleVelocity) < 0.01)
    mScaleVelocity = 0;
  else if (!IsScaling() && !mIsTargetScaleActive)
    mScale += mScaleVelocity * seconds;

  if (mScale > mScaleMax && mScaleVelocity > 0.01) {
    mTargetScale = mScaleMax;
    mIsTargetScaleActive = true;
  } else if (mScale < mScaleMin && mScaleVelocity < -0.01) {
    mTargetScale = mScaleMin;
    mIsTargetScaleActive = true;
    mIsTargetWindowActive = true;
  }
  
  // Apply target scale if enabled & not actively moving
  if (!IsScaling() && mIsTargetScaleActive) {
    if (fabs(mScale - mTargetScale) < 0.01) {
      mScale = mTargetScale;
      mScaleVelocity = 0;
      mIsTargetScaleActive = false;
    } else {
      mScale += 10 * seconds * (mTargetScale - mScale);
    }
  }

  // Apply inertial panning
  mCenterVelocityUV[0] *= kDamping;
  mCenterVelocityUV[1] *= kDamping;
  if (fabsf(mCenterVelocityUV[0]) < 1.0 / mFrame->ImageWidth())
    mCenterVelocityUV[0] = 0;
  if (fabsf(mCenterVelocityUV[1]) < 1.0 / mFrame->ImageHeight())
    mCenterVelocityUV[1] = 0;
  if (!IsDragging() && mCenterVelocityUV[0] != 0)
    mCenterUV[0] += mCenterVelocityUV[0] * seconds;
  if (!IsDragging() && mCenterVelocityUV[1] != 0)
    mCenterUV[1] += mCenterVelocityUV[1] * seconds;

  // Move the image so that it fits in the "valid" NDC rectangle.
  // When the image is fit to the display, this will target the center,
  // but when the image is zoomed in, it merely moves us back such
  // that the full NDC rectangle is used. The shorthand for this is that
  // the X & Y NDC values must mirror each other, e.g. |x0| == |x1|.
  // We can also compute the center of each NDC axis and compare that
  // against (0.5, 0.5) to know which direction to move.
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeFrameDisplayRect(*mFrame, &x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  const float centerNDC[2] = { 0.5 * (x0 + x1), 0.5 * (y0 + y1) };
  const bool aligned[2] = { fabsf(centerNDC[0])==0, fabsf(centerNDC[1])==0 };
  if (!aligned[0] || !aligned[1])
    mIsTargetWindowActive = true;

  if (!IsDragging() && mIsTargetWindowActive) {
    assert(mFrame->ImageWidth() && mFrame->ImageHeight() && mScale != 0);

    // Compute a UV offset to move us halfway to each edge
    float offUV[2] = { Ndc2U(*mFrame,  0.75 * centerNDC[0]),
                       Ndc2V(*mFrame, -0.75 * centerNDC[1]) };

    // Compute the size of a screen pixel in scaled UV units
    const float screenPixUV[2] = { Ndc2U(*mFrame, 0.5 / Width()),
                                   Ndc2V(*mFrame, 0.5 / Height()) };
    
    // If we get within a single screen pixel, snap to edge
    // FIXME: Find a smoother way to snap rather than 10 pixels.
    bool snap[2] = { false, false };
    if (!aligned[0] && fabsf(offUV[0]) < 10 * screenPixUV[0]) {
      offUV[0] = Ndc2U(*mFrame, centerNDC[0]);
      snap[0] = true;
    }
    if (!aligned[1] && fabsf(offUV[1]) < 10 * screenPixUV[1]) {
      offUV[1] = Ndc2V(*mFrame, -centerNDC[1]);
      snap[1] = true;
    }
    
    if ((aligned[0] || snap[0]) && (aligned[1] || snap[1])) {
      mIsTargetWindowActive = false;
      mIsDirty = true;
    }
    
    mCenterUV[0] += offUV[0];
    mCenterUV[1] += offUV[1];
  }

  return true;
}


bool FrameViewer::Dormant() const {
  if (mIsDirty)
    return false;
  if (mIsTargetWindowActive || mIsTargetScaleActive)
    return false;
  if (mScaleVelocity != 0)
    return false;
  if (mCenterVelocityUV[0] != 0 || mCenterVelocityUV[1] != 0)
    return false;
  return true;
}


// Adjust mScale and offset mCenterUV so that the origin remains
// invariant under scaling.

bool FrameViewer::OnScale(EventPhase phase, float scale, float velocity,
                          float xorg, float yorg) {
  assert(!isnan(scale) && !isnan(velocity));
  if (!mFrame)
    return false;

  if (phase == TOUCH_MOVED) {                 // Update scale on move
    float s = mScale * (1 + velocity);
    if (s < mScaleMin || s > mScaleMax)
      velocity *= 0.333;                      // Make it harder to pull
    mScale *= 1 + velocity;                   // Multiply scales
  }
  if (phase == TOUCH_ENDED) {
    mScaleVelocity = 50 * velocity;
    mIsTargetScaleActive = false;
    mTargetScale = 0;
  } else {
    mScaleVelocity = 0;
  }

  // Compute the visible NDC & UV display rectangles of the image
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeFrameDisplayRect(*mFrame, &x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  
  // Figure out the UV coordinates of the input point in the display rects
  float uNDC = (2.0 * xorg / Width() - 1 - x0) / (x1 - x0);
  float vNDC = (2.0 * yorg / Height() - 1 - y0) / (y1 - y0);
  float screenU = u0 + uNDC * (u1 - u0);
  float screenV = v0 + vNDC * (v1 - v0);

  // Move the center of the image toward the origin to keep
  // that point invariant, scaling by velocity (change in scale)
  mCenterUV[0] -= (mCenterUV[0] - screenU) * velocity;
  mCenterUV[1] -= (mCenterUV[1] - screenV) * velocity;
  
  return true;
}


// Translate the mCenterUV value based on the input pan distances
// in screen pixels.

bool FrameViewer::OnDrag(EventPhase phase, float x, float y, float dx, float dy,
                         float xorg, float yorg) {
  if (!mFrame)
    return false;
  
  // Convert velocity into image UV coordinates
  float x0, y0, x1, y1, u0, v0, u1, v1;
  ComputeFrameDisplayRect(*mFrame, &x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1);
  float du = -2 * (u1 - u0) * dx / Width()  / (x1 - x0);
  float dv = -2 * (v1 - v0) * dy / Height() / (y1 - y0);
  
  if (fabsf(x0) != fabsf(x1))                 // Translated off-center
    du *= 0.333;
  if (fabsf(y0) != fabsf(y1))
    dv *= 0.333;
  
  if (phase == TOUCH_MOVED) {                 // Update position on move
    mCenterUV[0] += du;
    mCenterUV[1] += dv;
  }
  
  if (phase == TOUCH_ENDED) {                 // Update animation on end
    float vscale = mScale < 1 ? 1 / mScale : mScale;
    mCenterVelocityUV[0] = 10 * vscale * du;
    mCenterVelocityUV[1] = 10 * vscale * dv;
  } else {                                    // Disable animation on move/begin
    mCenterVelocityUV[0] = mCenterVelocityUV[1] = 0;
  }
  
  return true;
}


// Center the image and compute the scaling required to fit the image
// within the current frame.

bool FrameViewer::SnapCurrentToFitFrame() {
  mCenterUV[0] = 0.5;                         // Center image
  mCenterUV[1] = 0.5;
  ComputeScaleRange();                        // Recompute if needed
  mScale = mScaleMin;                         // Fit image to screen
  mScaleVelocity = 0;
  mCenterVelocityUV[0] = 0;
  mCenterVelocityUV[1] = 0;
  mTargetScale = 0;
  mIsTargetWindowActive = false;
  mIsTargetScaleActive = false;
  return true;
}


//
// TextBox
//

bool TextBox::Init(int x, int y, int w, int h, GlesUtil::Text *gltext) {
  if (!ViewportWidget::Init(x, y, w, h))
    return false;
  mGlesText = gltext;
  return true;
}


bool TextBox::Draw() {
  glViewport(Left(), Bottom(), Width(), Height());
  size_t pos = 0;
  float y = Height() - mGlesText->Height();
  for (size_t nextPos = mText.find('\n', pos); pos != std::string::npos;
       pos = nextPos == std::string::npos ? nextPos : nextPos+1, nextPos = mText.find('\n',pos), y -=mGlesText->Height()){
    assert(nextPos == std::string::npos || nextPos - pos < 4096);
    char buf[4096];
    size_t bytes = nextPos == std::string::npos ? mText.length() - pos :
                              nextPos - pos;
    if (!bytes)
      continue;
    memcpy(buf, &mText.c_str()[pos], bytes);
    buf[bytes] = '\0';
    float x;
    switch (mAlign) {
      case LeftJustify:   x = 0;                                              break;
      case RightJustify:  x = Width() - int(mGlesText->ComputeWidth(buf));         break;
      case CenterJustify: x = Width() / 2 - int(mGlesText->ComputeWidth(buf) / 2); break;
    }
    if (!mGlesText->Draw(buf, x+Left(), y+Bottom()))
      return false;
  }
  return true;
}


//
// TabBar
//


int TabBar::FindTabIdx(int touchY) const {
  int y = Bottom();
  for (size_t i = 0; i < mTabVec.size(); ++i) {
    int h = mTabVec[i]->Height();
    if (touchY >= y && touchY < y + h)
      return i;
    y += h - mOverlap;
  }
  return -1;
}


bool TabBar::OnTapTouch(const Event::Touch &touch) {
  // If touch start and end are in the same tab, select it
  assert(touch.x >= Left() && touch.x < Right());
  int i = FindTabIdx(touch.y);
  if (i < 0)
    return false;
  Select(mTabVec[i]);  
  return true;
}


// Manage tab re-ordering

bool TabBar::OnDrag(EventPhase phase, float x, float y, float dx, float dy,
                    float xorg, float yorg) {
  return false;
}


bool TabBar::Draw() {
  // Compute tab viewport and pass to draw function
  const int x = Left();
  const int w = Width();
  int y = Bottom();
  for (size_t i = 0; i < mTabVec.size(); ++i) {
    const size_t h = mTabVec[i]->Height();
    if (!mTabVec[i]->Draw(x, y, w, h))
      return false;
    y += h - mOverlap;
  }
  return true;
}


bool TabBar::Step(float seconds) {
  return true;
}


bool TabBar::Dormant() const {
  return true;
}


bool TabBar::Select(const Tab *tab) {
  std::vector<Tab *>::const_iterator i;
  i = std::find(mTabVec.begin(), mTabVec.end(), tab);
  if (i == mTabVec.end()) {
    mCurTab = -1;
    return false;
  }
  
  int j = i - mTabVec.begin();
  if (j != mCurTab) {
    mCurTab = j;
    mTabVec[mCurTab]->OnSelect();
  }
  return true;
}
