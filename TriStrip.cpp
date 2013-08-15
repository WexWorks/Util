//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "TriStrip.h"

#include <assert.h>
#include <string.h>
#include <limits>

using Imath::V3f;
using Imath::V4f;

void TriStrip::SetVertexCapacity(size_t newCapacity) {
  if (newCapacity > VertexCapacity()) {
    mP.resize(newCapacity);
    for (size_t i = 0; i < kMaxAttr; ++i) {
      if (AttrEnabled(i))
        mA[i].resize(newCapacity);
    }
    if (mFlags & MATERIAL_FLAG)
      mMaterial.resize(newCapacity);

  }
  
  if (mVertexCount > newCapacity)
    mVertexCount = newCapacity;
}

size_t TriStrip::VertexCapacity() const {
  return mP.size();
}

size_t TriStrip::MaxCapacity() const {
  return std::numeric_limits<unsigned short>::max();
}

void TriStrip::VertexResize(size_t newSize) {
  if (newSize > VertexCapacity())
    SetVertexCapacity(newSize);
  mVertexCount = newSize;
}

size_t TriStrip::IndexCapacity() const {
  return mIdx.size();
}

void TriStrip::SetIndexCapacity(size_t newCapacity) {
  if (newCapacity > mIdx.size()) {
    mIdx.resize(newCapacity);
  }
  
  if (mIndexCount > newCapacity)
    mIndexCount = newCapacity;
}

void TriStrip::IndexResize(size_t newSize) {
  if (newSize > IndexCapacity())
    SetIndexCapacity(newSize);
  mIndexCount = newSize;
}

void TriStrip::Clear() {
  mVertexCount = 0;
  mIndexCount = 0;
}

void TriStrip::Init(size_t vertexCount, size_t indexCount, unsigned long flags){
  Clear();
  mFlags = flags;
  VertexResize(vertexCount);
  IndexResize(indexCount);
}


bool TriStrip::Append(const TriStrip &tristrip) {
  if (mFlags != tristrip.mFlags)
    return false;
  if (tristrip.VertexEmpty())
      return true;
  if (VertexCount() + tristrip.VertexCount() + 4 > MaxCapacity())
    return false;

  const size_t oldVCount = VertexCount();
  const size_t oldIdxCount = IndexCount();
  const size_t addVCount = tristrip.VertexCount();
  const bool addDegenerate = oldIdxCount && !tristrip.IndexEmpty();
  const size_t addIdxCount = tristrip.IndexCount() + (addDegenerate ? 4 : 0);
  const size_t vCount = oldVCount + addVCount;
  const size_t iCount = oldIdxCount + addIdxCount;
  
  VertexResize(vCount);
  size_t bytes = addVCount * sizeof(V3f);
  memcpy(&mP[oldVCount], &tristrip.mP[0], bytes);
  
  IndexResize(iCount);
  size_t firstNewIdx = oldIdxCount;
  if (addDegenerate) {
    // Duplicate the last vertex of the last strip and the first two
    // vertices of the new strip to join them with two degenerate tris
    firstNewIdx += 4;
    mIdx[oldIdxCount+0] = mIdx[oldIdxCount-1];
    const unsigned short firstIdx = tristrip.mIdx[0] + oldVCount;
    const unsigned short secondIdx = tristrip.mIdx[1] + oldVCount;
    mIdx[oldIdxCount+1] = firstIdx;
    mIdx[oldIdxCount+2] = firstIdx;
    mIdx[oldIdxCount+3] = secondIdx;
  }
  for (size_t i = 0, j = firstNewIdx; i < tristrip.IndexCount(); ++i, ++j)
    mIdx[j] = tristrip.mIdx[i] + oldVCount;
  
  for (size_t i = 0; i < kMaxAttr; ++i) {
    if (AttrEnabled(i)) {
      bytes = addVCount * sizeof(V4f);
      memcpy(&mA[i][oldVCount], &tristrip.mA[i][0], bytes);
    }
  }
  
  if (mFlags & MATERIAL_FLAG) {
    bytes = addVCount * sizeof(unsigned short); 
    memcpy(&mMaterial[oldVCount], &tristrip.mMaterial[0], bytes);
  }
  
  return true;
}


void TriStrip::ToLines(std::vector<unsigned short> &lineIdx) const {
  lineIdx.reserve(VertexCount()*3);
  for (size_t i = 0; i < IndexCount() - 2; ++i) {
    lineIdx.push_back(mIdx[i]);
    lineIdx.push_back(mIdx[i+1]);
    if (i < IndexCount() - 4 &&
        mIdx[i+1] == mIdx[i+2] && mIdx[i+3] == mIdx[i+4]) {
      i += 3;     // Skip degenerates
      continue;
    }
    lineIdx.push_back(mIdx[i]);
    lineIdx.push_back(mIdx[i+2]);
  }
  
  // Close off the final triangle
  lineIdx.push_back(mIdx[IndexCount()-2]);
  lineIdx.push_back(mIdx[IndexCount()-1]);
}


void TriStrip::InitTransform(const TriStrip &src, const Imath::M44f &T) {
  mFlags = src.mFlags;
  mIdx = src.mIdx;
  for (size_t i = 0; i < kMaxAttr; ++i) {
    mA[i].clear();
    mA[i].insert(mA[i].end(), src.mA[i].begin(), src.mA[i].end());
  }
  
  mMaterial.clear();
  mMaterial.insert(mMaterial.end(), src.mMaterial.begin(), src.mMaterial.end());
  
  mP.resize(src.mP.size());
  for (size_t i = 0; i < src.mP.size(); ++i)
    mP[i] = src.mP[i] * T;
  
  // Validate new vectors sizes
  VertexResize(mP.size());
  IndexResize(mIdx.size());  
}

