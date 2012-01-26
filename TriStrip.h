//  Created by Daniel Wexler on 1/25/12.

#ifndef TRISTRIP_H
#define TRISTRIP_H

#include <ImathVec.h>
#include <vector>


class TriStrip {
public:
  TriStrip() {}
  ~TriStrip() {}
  enum Flags { UV_FLAG=1, MATERIAL_FLAG=2 };
  void Init(size_t vertexCount, size_t indexCount, unsigned long flags);
  void Clear();
  bool Append(const TriStrip &tristrip);

  // Mutable access
  Imath::V3f &P(size_t i) { return mP[i]; }
  unsigned short &Idx(size_t i) { return mIdx[i]; }
  unsigned short &Material(size_t i) { return mMaterial[i]; }
  Imath::V2f &UV(size_t i) { return mUV[i]; }
  
  // Constant access
  size_t VertexCount() const { return mP.size(); }
  size_t IndexCount() const { return mIdx.size(); }
  const Imath::V3f &P(size_t i) const { return mP[i]; }
  const Imath::V2f &UV(size_t i) const { return mUV[i]; }
  const unsigned short &Idx(size_t i) const { return mIdx[i]; }
  const unsigned short &Material(size_t i) const { return mMaterial[i]; }
  
private:
  unsigned long mFlags;
  std::vector<Imath::V3f> mP;                       
  std::vector<unsigned short> mIdx;
  std::vector<Imath::V2f> mUV;
  std::vector<unsigned short> mMaterial;
};


#endif  // TRISTRIP_H
