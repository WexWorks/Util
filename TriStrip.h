// Copyright (c) 2012 by The 11ers, LLC -- All Rights Reserved

#ifndef TRISTRIP_H
#define TRISTRIP_H

#include <vector>
#include <assert.h>

#include <ImathMatrix.h>
#include <ImathVec.h>


// Stores the geometry buffers required to render a triangle strip.
// Utility functions for accessing individual vertex and index values,
// as well as Append, which contacenates two strips, joining them
// with degenerate vertices.  Flags are used to control which attributes
// are active.

class TriStrip {
public:
  TriStrip() : mVertexCount(0), mIndexCount(0), mFlags(0) {}
  ~TriStrip() {}
  enum Flags { ATTR_0_FLAG=1, ATTR_1_FLAG=2, ATTR_2_FLAG=4, MATERIAL_FLAG=8 };
  void Init(size_t vertexCount, size_t indexCount, unsigned long flags);
#if 0
  void InitDisc(const Imath::V3f &center, const Imath::V3f &N, float radius,
                size_t vertexCount, unsigned long flags, int uvAttrIdx);
  void InitExtrusion(size_t vertexCount, const Imath::V3f *face,
                     size_t segmentCount, const Imath::V3f *scale,
                     unsigned long attrNFlag);
  void InitBox(const Imath::V3f &min, const Imath::V3f &max,
               unsigned long attrNFlag, unsigned long attrUVFlag);
#endif
  void InitTransform(const TriStrip &src, const Imath::M44f &T);
  void Clear();
  void Reserve(size_t vertexCapacity, size_t indexCapacity);
  void Resize(size_t vertexCount, size_t indexCount);
  bool Append(const TriStrip &tristrip);
  void ToLines(std::vector<unsigned short> &lineIdx) const;
  
  // Mutable access
  Imath::V3f &P(size_t i) {  assert(i < mVertexCount); return mP[i]; }
  Imath::V4f &Attr(size_t a, size_t v) {
    assert(a < kMaxAttr); assert(v < mVertexCount); return mA[a][v]; }
  unsigned short &Idx(size_t i) { assert(i < mIndexCount); return mIdx[i]; }
  unsigned short &Material(size_t i) {
    assert(i < mVertexCount); return mMaterial[i]; }
  
  // Constant access
  size_t VertexCount() const { return mVertexCount; }
  size_t IndexCount() const { return mIndexCount; }
  bool Empty() const { return IndexEmpty(); }
  const Imath::V3f &P(size_t i) const { assert(i < mVertexCount); return mP[i]; }
  const Imath::V4f &Attr(size_t a, size_t v) const {
    assert(a < kMaxAttr); assert(v < mVertexCount); return mA[a][v]; }
  bool AttrEnabled(size_t i) const {
    assert(i < kMaxAttr); return mFlags & (1L << i); }
  const unsigned short &Idx(size_t i) const {
    assert(i < mIndexCount); return mIdx[i]; }
  const unsigned short &Material(size_t i) const {
    assert(i < mVertexCount);return mMaterial[i]; }
  
private:

  void VertexResize(size_t newSize);
  void IndexResize(size_t newSize);
  void SetIndexCapacity(size_t newCapacity);
  void SetVertexCapacity(size_t newCapacity);
  size_t VertexCapacity() const;
  size_t IndexCapacity() const;
  size_t MaxCapacity() const;
  bool VertexEmpty() const { return VertexCount() == 0; }
  bool IndexEmpty() const { return IndexCount() == 0; }

  static const size_t kMaxAttr = 3;
  
  size_t mVertexCount;
  size_t mIndexCount;
  unsigned long mFlags;
  std::vector<Imath::V3f> mP;
  std::vector<unsigned short> mIdx;
  std::vector<Imath::V4f> mA[kMaxAttr];
  std::vector<unsigned short> mMaterial;
};


#endif  // TRISTRIP_H
