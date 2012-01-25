//  Created by Daniel Wexler on 1/25/12.

#ifndef TRISTRIP_H
#define TRISTRIP_H

#include <ImathVec.h>
#include <vector>


class TriStrip {
public:
  TriStrip() {}
  ~TriStrip() {}
  void Init(size_t vertexCount, size_t indexCount);
  Imath::V3f &P(size_t i) { return mP[i]; }
  const Imath::V3f &P(size_t i) const { return mP[i]; }
  Imath::V2f &uv(size_t i) { return mUV[i]; }
  const Imath::V2f &uv(size_t i) const { return mUV[i]; }
  void Append(const TriStrip &tristrip);
  
private:
  std::vector<Imath::V3f> mP;
  std::vector<Imath::V2f> mUV;
  std::vector<short> idx;
};


#endif  // TRISTRIP_H
