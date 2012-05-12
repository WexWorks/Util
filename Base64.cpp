// Copyright (c) by The 11ers.  All rights reserved.

#include "Base64.h"

namespace Base64 {

// Translation table from RFC1113
static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789-_"; // URL encode + to -, / to _

// Decoding translation table
static const char cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@"
                           "ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ"
                           "[\\]^_`abcdefghijklmnopq";

  
// Encode 3 8-bit binary bytes as four 6-bit characters
static void EncodeBlock(unsigned char in[3], unsigned char out[4], int len) {
  out[0] = cb64[in[0] >> 2];
  out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
  out[2] = (unsigned char) (len > 1 ? cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
  out[3] = (unsigned char) (len > 2 ? cb64[in[2] & 0x3f] : '=');
}

  
// Encode a source buffer, adding padding and line breaks
void Encode(const unsigned char *src, size_t len,
            std::vector<unsigned char> &dst) {
  const size_t lineSize = 0;  // Do not insert linefeeds (illegal in JSON)
  size_t blocksOut = 0;
  for (size_t j = 0; j < len; /*EMPTY*/) {
    size_t k = 0;
    unsigned char in[3], out[4];
    for (size_t i = 0; i < 3; ++i) {
      if (j < len) {
        ++k;
        in[i] = src[j++];
      } else {
        in[i] = 0;
      }
    }
    if (k) {
      EncodeBlock(in, out, k);
      ++blocksOut;
      for (size_t i = 0; i < 4; ++i)
        dst.push_back(out[i]);
    }
    if (lineSize > 0 && (blocksOut >= lineSize/4 || j >= len)) {
      if (blocksOut) {
        dst.push_back('\r');
        dst.push_back('\n');
      }
      blocksOut = 0;
    }
  }
  
  // Percent-encode any trailing equals signs
  size_t neq = 0;
  for (size_t i = 0; i < 4; ++i)
    if (dst[dst.size() - 1 - i] == '=')
      neq++;
  if (neq) {
    dst.resize(dst.size() - neq);
    for (size_t i = 0; i < neq; ++i) {
      dst.push_back('%');
      dst.push_back('3');
      dst.push_back('D');
    }
  }
}


// Decode 4 6-bit characters into 3 8-bit binary bytes
static void DecodeBlock(unsigned char in[4], unsigned char out[3]) {   
  out[0] = (unsigned char) (in[0] << 2 | in[1] >> 4);
  out[1] = (unsigned char) (in[1] << 4 | in[2] >> 2);
  out[2] = (unsigned char) (((in[2] << 6) & 0xc0) | in[3]);
}


// Decode a Base64 encoded buffer, discarding padding, line breaks and noise
void Decode(unsigned char *src, size_t len,
            std::vector<unsigned char> &dst) {
  size_t neq = 0;
  size_t firstPercentIdx = 0;
  for (size_t j = 1; j < 14; ++j) {
    if (src[len - j] == '%') {
      neq++;
      firstPercentIdx = len - j;
    }
  }
  if (neq) {
    len -= 2 * neq;
    for (size_t j = 0; j < neq; ++j)
      src[firstPercentIdx + j] = '=';
  }
  
  for (size_t j = 0; j < len; /*EMPTY*/) {
    size_t k = 0;
    unsigned char in[4], out[3];
    for (size_t i = 0; i < 4 && j < len; ++i) {
      unsigned char v = 0;
      while (j < len && v == 0) {
        v = src[j++];
        if (v == '-') v = '+';    // Reverse URL decode
        if (v == '_') v = '/';
        v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[v - 43]);
        if (v)
          v = (unsigned char) ((v == '$') ? 0 : v - 61);
      }
      if (j < len) {
        ++k;
        if (v)
          in[i] = (unsigned char)(v - 1);
      } else {
        in[i] = 0;
      }      
    }
    if (k) {
      DecodeBlock(in, out);
      for (size_t i = 0; i < k - 1; ++i)
        dst.push_back(out[i]);
    }
  }
}

};  // namespace Base64