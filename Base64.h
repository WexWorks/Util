// Copyright (c) by The 11ers.  All rights reserved.

#ifndef BASE64_H
#define BASE64_H

#include <vector>

// Implements URL-safe Base-64 encoding of arbitrary binary data.
// Base64 encoded 3 8-bit values into 4 6-bit values using only the
// safe ASCII values. However, it normally uses some characters that
// are not safe to send as URL-encoded values through, say, HTTP POST.
// We fix that using the standard technique of replacing '+' with '-'
// and '/' with '_' to make those characters URL-safe, and we percent-
// encode any trailing '=' characters.

namespace Base64 {
  void Encode(const unsigned char *src, size_t len,
              std::vector<unsigned char> &dst);
  
  void Decode(unsigned char *src, size_t len,
              std::vector<unsigned char> &dst);
};

#endif // BASE64_H
