//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "TriStrip.h"
#include <string.h>
#include <assert.h>


void TriStrip::Clear() {
  mFlags = 0;
  mP.resize(0);
  mIdx.resize(0);
  mUV.resize(0);
  mMaterial.resize(0);
}


void TriStrip::Init(size_t vertexCount, size_t indexCount, unsigned long flags){
  Clear();
  mFlags = flags;
  mP.resize(vertexCount);
  mIdx.resize(indexCount);
  if (flags & UV_FLAG)
    mUV.resize(vertexCount);
  if (flags & MATERIAL_FLAG)
    mMaterial.resize(vertexCount);
}


bool TriStrip::Append(const TriStrip &tristrip) {
  if (mFlags != tristrip.mFlags)
    return false;
  if (tristrip.mIdx.empty())
    return true;
  if (tristrip.mP.empty())
      return true;

  const size_t oldVCount = mP.size();
  const size_t oldIdxCount = mIdx.size();
  const size_t addVCount = tristrip.mP.size();
  const size_t addIdxCount = tristrip.mIdx.size();
  const size_t vCount = oldVCount + addVCount;
  const size_t iCount = oldIdxCount + addIdxCount;
  
  mP.resize(vCount);
  size_t bytes = addVCount * sizeof(Imath::V3f);
  memcpy(&mP[oldVCount], &tristrip.mP[0], bytes);
  
  mIdx.resize(iCount);
  for (size_t i = 0, j = oldIdxCount; i < addIdxCount; ++i, ++j)
    mIdx[j] = tristrip.mIdx[i] + oldVCount;
  
  if (mFlags && UV_FLAG) {
    mUV.resize(vCount);
    bytes = addVCount * sizeof(Imath::V2f);
    memcpy(&mUV[oldVCount], &tristrip.mUV[0], bytes);
  }
  
  if (mFlags && MATERIAL_FLAG) {
    mMaterial.resize(vCount);
    bytes = addVCount * sizeof(short); 
    memcpy(&mMaterial[oldVCount], &tristrip.mMaterial[0], bytes);
  }
  
  return true;
}