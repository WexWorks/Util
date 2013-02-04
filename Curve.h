//  Copyright (c) 2013 The 11ers. All rights reserved.

#ifndef CURVE_H
#define CURVE_H

#include <vector>
#include <assert.h>


//
// Cubic1D
//

// Cubic interpolation in 1D (e.g. F(x)), using extrapolation to generate
// the first and last points needed for the continuous cubic interpolators.

class Cubic1D {
public:
  Cubic1D() : mDirty(false) {}
  virtual ~Cubic1D() {}
  
  virtual void AddKnot(float y) { mKnot.push_back(y); mDirty = true; }
  
  virtual double Interpolate(float t) {
    assert(mKnot.size() > 1);
    // Construct external points using derivatives if needed
    if (mDirty) {
      mDirty = false;
      mFirstKnot = 2 * mKnot[0] - mKnot[1];
      size_t j = mKnot.size() - 1;
      mLastKnot = 2 * mKnot[j] - mKnot[j-1];
    }
    
    // Remap t to the proper segment
    size_t segmentCount = mKnot.size() - 1;
    size_t segmentIdx = t >= 1 ? segmentCount - 1 : floorf(t * segmentCount);
    assert(segmentIdx >= 0 && segmentIdx < mKnot.size() - 1);
    float s = segmentCount * t - segmentIdx;
    float y0 = segmentIdx == 0 ? mFirstKnot : mKnot[segmentIdx-1];
    float y1 = mKnot[segmentIdx];
    float y2 = mKnot[segmentIdx + 1];
    float y3 = segmentIdx == segmentCount - 1 ? mLastKnot : mKnot[segmentIdx + 2];
    return Cubic(y0, y1, y2, y3, s);
  }
  
  virtual double Cubic(double y0,double y1, double y2,double y3, double t) {
    double t2 = t * t;
    double a0 = y3 - y2 - y0 + y1;
    double a1 = y0 - y1 - a0;
    double a2 = y2 - y0;
    double a3 = y1;
    
    return a0 * t * t2 + a1 * t2 + a2 * t + a3;
  }
  
private:
  std::vector<double> mKnot;
  double mFirstKnot, mLastKnot;
  bool mDirty;
};


class CatmullRom1D : public Cubic1D {
public:
  // Similar to Cubic interpolation, but uses the slope between the
  // previous and next points as the derivative of the current point.
  virtual double CatmullRom(double y0,double y1, double y2,double y3, double t) {
    double t2 = t * t;
    double a0 = -0.5*y0 + 1.5*y1 - 1.5*y2 + 0.5*y3;
    double a1 = y0 - 2.5*y1 + 2*y2 - 0.5*y3;
    double a2 = -0.5*y0 + 0.5*y2;
    double a3 = y1;
    
    return a0 * t * t2 + a1 * t2 + a2 * t + a3;
  }
};


//
// Cubic2D
//

// Consider using a template!

template <class F>
class Cubic2DT {
public:
  Cubic2D() {}
  virtual ~Cubic2D() {}
  virtual void AddKnot(float x, float y) {
    mXCubic.AddKnot(x); mYCubic.AddKnot(y);
  }
  virtual double X(float t) { return mXCubic.Interpolate(t); }
  virtual double Y(float t) { return mYCubic.Interpolate(t); }
  
private:
  F mXCubic, mYCubic;
};


typedef Cubic2DT<Cubic1D> Cubic2D;
typedef Cubic2DT<CatmullRom1D> CatmullRom2D;


#endif
