//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "ImathVec.h"
#include <math.h>


// Process events to control a 3D camera orbit

class Trackball {
public:
  Trackball() {}
  virtual ~Trackball() {}
  
  virtual void Init(const Imath::V3f &origin, const Imath::V3f &position) {
    mOrigin = origin; mPosition = position; Cartesian2Spherical();
  }
  
  virtual void SetTheta(float theta) { mTheta = theta; Spherical2Cartesian(); }
  virtual void SetPhi(float phi) { mPhi = phi; Spherical2Cartesian(); }
  virtual void SetRadius(float r) { mRadius = r; Spherical2Cartesian(); }

  virtual const Imath::V3f &Position() const { return mPosition; }
  virtual const Imath::V3f &Origin() const { return mOrigin; }
  virtual float Theta() const { return mTheta; }
  virtual float Phi() const { return mPhi; }
  virtual float Radius() const { return mRadius; }

  enum TouchPhase { TOUCH_BEGAN, TOUCH_MOVED, TOUCH_ENDED, TOUCH_CANCELLED };
  virtual void Touch(TouchPhase phase, int x, int y) {
    switch (phase) {
      case TOUCH_BEGAN: lastTouch[0] = x; lastTouch[1] = y; break;
      case TOUCH_MOVED: {
        int dx = x - lastTouch[0];
        int dy = y - lastTouch[1];
        const float k = 0.001;
        mTheta += k * dy;
        mPhi -= k * dx;
        lastTouch[0] = x; lastTouch[1] = y;
        Spherical2Cartesian();
        break;
      }
      case TOUCH_ENDED:
      case TOUCH_CANCELLED:
        break;
    }
  }
  
private:
  virtual void Cartesian2Spherical() {
    Imath::V3f p = mPosition - mOrigin;
    mRadius = sqrtf(p.dot(p));
    mTheta = acosf(p.y / mRadius);
    mPhi = atan2f(p.x, p.z);
  }
  virtual void Spherical2Cartesian() {
    Imath::V3f s(sin(mTheta) * sin(mPhi), cos(mTheta), sin(mTheta) * cos(mPhi));
    mPosition = mRadius * s + mOrigin;
  }
  
  float mTheta, mPhi, mRadius;
  Imath::V3f mOrigin;
  Imath::V3f mPosition;
  int lastTouch[2];
};


#endif  // TRACKBALL_H
