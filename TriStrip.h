// Copyright (c) 2012 by The 11ers, LLC -- All Rights Reserved

#ifndef TRISTRIP_H
#define TRISTRIP_H

#include <vector>
#include <assert.h>

#include "ImathMatrix.h"
#include "ImathVec.h"


// Stores the geometry buffers required to render a triangle strip.
// Utility functions for accessing individual vertex and index values,
// as well as Append, which contacenates two strips, joining them
// with degenerate vertices.  Flags are used to control which attributes
// are active.

class TriStrip {
public:
  TriStrip() : mFlags(0) {}
  ~TriStrip() {}
  enum Flags { ATTR_0_FLAG=1, ATTR_1_FLAG=2, ATTR_2_FLAG=4, MATERIAL_FLAG=8 };
  void Init(size_t vertexCount, size_t indexCount, unsigned long flags);
  void InitDisc(const Imath::V3f &center, const Imath::V3f &N, float radius,
                size_t vertexCount, unsigned long flags, int uvAttrIdx);
  void InitExtrusion(size_t vertexCount, const Imath::V3f *face,
                     size_t segmentCount, const Imath::V3f *scale,
                     unsigned long attrNFlag);
  void InitBox(const Imath::V3f &min, const Imath::V3f &max,
               unsigned long attrNFlag, unsigned long attrUVFlag);
  void InitTransform(const TriStrip &src, const Imath::M44f &T);
  void Clear();
  void Reserve(size_t vertexCount, size_t indexCount);
  bool Append(const TriStrip &tristrip);
  void ToLines(std::vector<unsigned short> &lineIdx) const;
  
  // Mutable access
  Imath::V3f &P(size_t i) {  assert(i < mP.size()); return mP[i]; }
  Imath::V4f &Attr(size_t a, size_t v) {
    assert(a < kMaxAttr); assert(v < mA[a].size()); return mA[a][v]; }
  unsigned short &Idx(size_t i) { assert(i < mIdx.size()); return mIdx[i]; }
  unsigned short &Material(size_t i) {
    assert(i < mMaterial.size()); return mMaterial[i]; }
  
  // Constant access
  size_t VertexCount() const { return mP.size(); }
  size_t IndexCount() const { return mIdx.size(); }
  bool Empty() const { return mIdx.size() == 0; }
  const Imath::V3f &P(size_t i) const { assert(i < mP.size()); return mP[i]; }
  const Imath::V4f &Attr(size_t a, size_t v) const {
    assert(a < kMaxAttr); assert(v < mA[a].size()); return mA[a][v]; }
  bool AttrEnabled(size_t i) const {
    assert(i < kMaxAttr); return mFlags & (1L << i); }
  const unsigned short &Idx(size_t i) const {
    assert(i < mIdx.size()); return mIdx[i]; }
  const unsigned short &Material(size_t i) const {
    assert(i < mMaterial.size());return mMaterial[i]; }
  
private:
  static const size_t kMaxAttr = 3;  
  
  unsigned long mFlags;
  std::vector<Imath::V3f> mP;
  std::vector<unsigned short> mIdx;
  std::vector<Imath::V4f> mA[kMaxAttr];
  std::vector<unsigned short> mMaterial;
};


#endif  // TRISTRIP_H
