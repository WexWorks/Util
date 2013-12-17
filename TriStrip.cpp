//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "TriStrip.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits>

using Imath::V3f;
using Imath::V4f;


void TriStrip::Clear() {
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
  if (mP.size() + tristrip.mP.size() + 4 > maxIdx)
    return false;

  const size_t oldVCount = mP.size();
  const size_t oldIdxCount = mIdx.size();
  const size_t addVCount = tristrip.mP.size();
  const bool addDegenerate = oldIdxCount && !tristrip.mIdx.empty();
  const size_t addIdxCount = tristrip.mIdx.size() + (addDegenerate ? 4 : 0);
  const size_t vCount = oldVCount + addVCount;
  const size_t iCount = oldIdxCount + addIdxCount;
  
  mP.resize(vCount);
  size_t bytes = addVCount * sizeof(V3f);
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
      bytes = addVCount * sizeof(V4f);
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
  for (size_t i = 0; i < kMaxAttr; ++i)
    mA[i].insert(mA[i].end(), src.mA[i].begin(), src.mA[i].end());
  mMaterial.insert(mMaterial.end(), src.mMaterial.begin(), src.mMaterial.end());
  mP.resize(src.mP.size());
  for (size_t i = 0; i < src.mP.size(); ++i)
    mP[i] = src.mP[i] * T;
}


void TriStrip::InitDisc(const V3f &center, const V3f &N,
                        float radius, size_t vertexCount,
                        unsigned long flags, int uvAttrIdx) {
  mFlags = flags;
  V3f X = N.cross(V3f(1, 0, 0));
  if (X.length() < 0.001)
    X = N.cross(V3f(0, 0, 1));
  V3f Y = N.cross(V3f(0, 1, 0));
  if (Y.length() < 0.001)
    Y = N.cross(V3f(0, 0, 1));
  mP.push_back(center);
  V3f P = radius * Y;
  mP.push_back(P);
  const float k = 1.0 / (vertexCount - 1);
  for (size_t i = 1; i < vertexCount; ++i) {
    float theta = 2 * M_PI * k * i;
    float u = sinf(theta);
    float v = cosf(theta);
    P = radius * (u * X + v * Y);
    mP.push_back(P);

    mIdx.push_back(i);
    mIdx.push_back(0);
    mIdx.push_back(i+1);
  }
  
  mIdx.push_back(1);
  assert(vertexCount == mP.size() - 1);
  for (size_t i = 0; i < kMaxAttr; ++i) {
    if (AttrEnabled(i))
      mA[i].resize(mP.size());
    if (i == uvAttrIdx) {
      for (size_t j = 0; j < vertexCount; ++j) {
        float theta = 2 * M_PI * k * i;
        mA[i][j].x = sinf(theta);
        mA[i][j].y = cosf(theta);
      }
    }
  }
}


void TriStrip::InitExtrusion(size_t vertexCount, const V3f *face,
                             size_t segmentCount, const V3f *scale,
                             unsigned long attrNFlag){
  if (vertexCount < 3)
    return;
  if (segmentCount < 1)
    return;
  
  mFlags = attrNFlag;
  
  int attrNIdx = attrNFlag == ATTR_0_FLAG ? 0 :
                 attrNFlag == ATTR_1_FLAG ? 1 :
                 attrNFlag == ATTR_2_FLAG ? 2 : -1;
  
  // Walk around each segment, adding a tristrip loop
  for (int i = 0; i < segmentCount; ++i) {
    for (int j = 0; j < vertexCount; ++j) {
      V3f P(scale[i].x * face[j].x, scale[i].y * face[j].y, scale[i].z);
      mP.push_back(P);
      V4f N(0, 1, 0, 0);
      if (attrNFlag)
        mA[attrNIdx].push_back(N);
      if (i) {
        int k0 = (i - 1) * vertexCount;
        int k1 = i * vertexCount;
        unsigned short vidx[6] = { k0+j, k1+j, k1+j+1, k0+j+1, k0+j, k1+j+1 };
        if (j == vertexCount - 1) {
          vidx[2] = vidx[5] = k1;
          vidx[3] = k0;
        };
        for (int k = 0; k < 6; ++k)
          mIdx.push_back(vidx[k]);
      }
    }
  }
  
  // Special case triangle and quad cases, which are tristrips without
  // degeneracies, and place a vertex in the center for larger polygons.
  int k0 = (segmentCount - 1) * vertexCount;
  int k1 = k0 + vertexCount;
  switch (vertexCount) {
    case 3: {
      int idx[9] = { k1, 0, 0, 1, 2, 2, k0, k0+1, k0+2 };
      for (int i = 0; i < 9; ++i)
        mIdx.push_back(idx[i]);
      break;
    }
    case 4: {
      int idx[11] = { k1, 0, 0, 1, 2, 3, 3, k0, k0+1, k0+2, k0+3 };
      for (int i = 0; i < 11; ++i)
        mIdx.push_back(idx[i]);
      break;
    }
      
    default:
      abort();
      break;
  }
}


void TriStrip::InitBox(const V3f &min, const V3f &max,
                       unsigned long attrNFlag, unsigned long attrUVFlag) {
  V3f face[4] = { V3f(min.x, min.y, 0), V3f(max.x, min.y, 0),
                  V3f(max.x, max.y, 0), V3f(min.x, max.y, 0) };
  V3f scale[2] = { V3f(1, 1, min.z), V3f(1, 1, max.z) };
  InitExtrusion(4, face, 2, scale, attrNFlag);
}


const float kCubeVertexData[216] = {
  // posX, posY, posZ,       nX, nY, nZ,
  0.5f, -0.5f, -0.5f,        1.0f, 0.0f, 0.0f,
  0.5f, 0.5f, -0.5f,         1.0f, 0.0f, 0.0f,
  0.5f, -0.5f, 0.5f,         1.0f, 0.0f, 0.0f,
  0.5f, -0.5f, 0.5f,         1.0f, 0.0f, 0.0f,
  0.5f, 0.5f, -0.5f,         1.0f, 0.0f, 0.0f,
  0.5f, 0.5f, 0.5f,          1.0f, 0.0f, 0.0f,
  
  0.5f, 0.5f, -0.5f,         0.0f, 1.0f, 0.0f,
  -0.5f, 0.5f, -0.5f,        0.0f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f,          0.0f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f,          0.0f, 1.0f, 0.0f,
  -0.5f, 0.5f, -0.5f,        0.0f, 1.0f, 0.0f,
  -0.5f, 0.5f, 0.5f,         0.0f, 1.0f, 0.0f,
  
  -0.5f, 0.5f, -0.5f,        -1.0f, 0.0f, 0.0f,
  -0.5f, -0.5f, -0.5f,       -1.0f, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f,         -1.0f, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f,         -1.0f, 0.0f, 0.0f,
  -0.5f, -0.5f, -0.5f,       -1.0f, 0.0f, 0.0f,
  -0.5f, -0.5f, 0.5f,        -1.0f, 0.0f, 0.0f,
  
  -0.5f, -0.5f, -0.5f,       0.0f, -1.0f, 0.0f,
  0.5f, -0.5f, -0.5f,        0.0f, -1.0f, 0.0f,
  -0.5f, -0.5f, 0.5f,        0.0f, -1.0f, 0.0f,
  -0.5f, -0.5f, 0.5f,        0.0f, -1.0f, 0.0f,
  0.5f, -0.5f, -0.5f,        0.0f, -1.0f, 0.0f,
  0.5f, -0.5f, 0.5f,         0.0f, -1.0f, 0.0f,
  
  0.5f, 0.5f, 0.5f,          0.0f, 0.0f, 1.0f,
  -0.5f, 0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
  0.5f, -0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
  0.5f, -0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
  -0.5f, 0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
  -0.5f, -0.5f, 0.5f,        0.0f, 0.0f, 1.0f,
  
  0.5f, -0.5f, -0.5f,        0.0f, 0.0f, -1.0f,
  -0.5f, -0.5f, -0.5f,       0.0f, 0.0f, -1.0f,
  0.5f, 0.5f, -0.5f,         0.0f, 0.0f, -1.0f,
  0.5f, 0.5f, -0.5f,         0.0f, 0.0f, -1.0f,
  -0.5f, -0.5f, -0.5f,       0.0f, 0.0f, -1.0f,
  -0.5f, 0.5f, -0.5f,        0.0f, 0.0f, -1.0f
};

