// Copyright (c) 2012 by The 11ers, LLC -- All Rights Reserved

#ifndef TRISTRIP_H
#define TRISTRIP_H

#include <vector>

#include <ImathVec.h>


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
  void Clear();
  void Reserve(size_t vertexCount, size_t indexCount);
  bool Append(const TriStrip &tristrip);

  // Mutable access
  Imath::V3f &P(size_t i) { return mP[i]; }
  Imath::V4f &Attr(size_t a, size_t v) { return mA[a][v]; }
  unsigned short &Idx(size_t i) { return mIdx[i]; }
  unsigned short &Material(size_t i) { return mMaterial[i]; }
  
  // Constant access
  size_t VertexCount() const { return mP.size(); }
  size_t IndexCount() const { return mIdx.size(); }
  const Imath::V3f &P(size_t i) const { return mP[i]; }
  const Imath::V4f &Attr(size_t a, size_t v) const { return mA[a][v]; }
  bool AttrEnabled(size_t i) const { return mFlags & (1L << i); }
  const unsigned short &Idx(size_t i) const { return mIdx[i]; }
  const unsigned short &Material(size_t i) const { return mMaterial[i]; }
  
private:
  static const size_t kMaxAttr = 3;  
  
  unsigned long mFlags;
  std::vector<Imath::V3f> mP;
  std::vector<unsigned short> mIdx;
  std::vector<Imath::V4f> mA[kMaxAttr];
  std::vector<unsigned short> mMaterial;
};


#endif  // TRISTRIP_H
