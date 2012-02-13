//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "TriStrip.h"

#include <assert.h>
#include <string.h>
#include <limits>


void TriStrip::Clear() {
  mFlags = 0;
  mP.resize(0);
  mIdx.resize(0);
  for (size_t i = 0; i < kMaxAttr; ++i)
    mA[i].resize(0);
  mMaterial.resize(0);
}


void TriStrip::Init(size_t vertexCount, size_t indexCount, unsigned long flags){
  Clear();
  mFlags = flags;
  mP.resize(vertexCount);
  mIdx.resize(indexCount);
  for (size_t i = 0; i < kMaxAttr; ++i) {
    if (AttrEnabled(i))
      mA[i].resize(vertexCount);
  }
  if (flags & MATERIAL_FLAG)
    mMaterial.resize(vertexCount);
}


void TriStrip::Reserve(size_t vertexCount, size_t indexCount) {
  mP.reserve(vertexCount);
  mIdx.reserve(indexCount);
  for (size_t i = 0; i < kMaxAttr; ++i) {
    if (AttrEnabled(i))
      mA[i].reserve(vertexCount);
  }
  if (mFlags & MATERIAL_FLAG)
    mMaterial.reserve(vertexCount);
}


bool TriStrip::Append(const TriStrip &tristrip) {
  if (mFlags != tristrip.mFlags)
    return false;
  if (tristrip.mP.empty())
      return true;
  const size_t maxIdx = std::numeric_limits<unsigned short>::max();
  if (mP.size() + tristrip.mP.size() > maxIdx)
    return false;

  const size_t oldVCount = mP.size();
  const size_t oldIdxCount = mIdx.size();
  const size_t addVCount = tristrip.mP.size();
  const bool addDegenerate = oldIdxCount && !tristrip.mIdx.empty();
  const size_t addIdxCount = tristrip.mIdx.size() + (addDegenerate ? 4 : 0);
  const size_t vCount = oldVCount + addVCount;
  const size_t iCount = oldIdxCount + addIdxCount;
  
  mP.resize(vCount);
  size_t bytes = addVCount * sizeof(Imath::V3f);
  memcpy(&mP[oldVCount], &tristrip.mP[0], bytes);
  
  mIdx.resize(iCount);
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
  for (size_t i = 0, j = firstNewIdx; i < tristrip.mIdx.size(); ++i, ++j)
    mIdx[j] = tristrip.mIdx[i] + oldVCount;
  
  for (size_t i = 0; i < kMaxAttr; ++i) {
    if (AttrEnabled(i)) {
      mA[i].resize(vCount);
      bytes = addVCount * sizeof(Imath::V4f);
      memcpy(&mA[i][oldVCount], &tristrip.mA[i][0], bytes);
    }
  }
  
  if (mFlags & MATERIAL_FLAG) {
    mMaterial.resize(vCount);
    bytes = addVCount * sizeof(unsigned short); 
    memcpy(&mMaterial[oldVCount], &tristrip.mMaterial[0], bytes);
  }
  
  return true;
}