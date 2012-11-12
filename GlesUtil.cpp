//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "GlesUtil.h"

#include <OpenGLES/ES2/glext.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <math.h>


static GLint gErrorCode = GL_NO_ERROR;


bool GlesUtil::Error() {
#if 1
  gErrorCode = glGetError();            // Note: GLES only has one active err
  if (gErrorCode != GL_NO_ERROR) {
    printf("GL ERROR: %s\n", ErrorString());
    return true;
  }
  return false;
#else
  return false;
#endif
}


const char *GlesUtil::ErrorString() {
  switch (gErrorCode) {
    case GL_NO_ERROR:           return "No error";
    case GL_INVALID_ENUM:       return "Invalid enum";
    case GL_INVALID_OPERATION:  return "Invalid operation";
    case GL_INVALID_VALUE:      return "Invalid value";
    case GL_OUT_OF_MEMORY:      return "Out of memory";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
                                return "Invalid framebuffer operation";
    default:                    return "Unknown";
  }
}


bool GlesUtil::DrawBox2f(GLuint aP, float x0, float y0, float x1, float y1,
                          GLuint aUV, float u0, float v0, float u1, float v1) {
  const float P[8] = { x0, y0,  x0, y1,  x1, y0,  x1, y1 };
  const float UV[8] = { u0, v0,  u0, v1,  u1, v0,  u1, v1 };
  glEnableVertexAttribArray(aUV);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawBoxFrame2f(GLuint aP, float x0, float y0, float x1, float y1,
                              float w, float h, GLuint aUV) {
  float xi0 = x0 + w;                        // inner box
  float yi0 = y0 + h;
  float xi1 = x1 - w;
  float yi1 = y1 - h;
  const float P[16] = { x0, y0,  x0, y1,  x1, y0,  x1, y1,
                        xi0, yi0, xi0, yi1, xi1, yi0, xi1, yi1 };
  const float UV[16] = { 0, 0, 0.25, 0, 0.75, 0, 0.5, 0,
                         0, 1, 0.25, 1, 0.75, 1, 0.5, 1 };
  const unsigned short idx[10] = { 4, 0, 5, 1, 7, 3, 6, 2, 4, 0 };
  glEnableVertexAttribArray(aUV);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, idx);
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             const float *MVP) {
  GLuint aP, aUV, uMVP, uTex;
  GLuint program = TextureProgram(&aP, &aUV, &uMVP, &uTex);
  glUseProgram(program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(uTex, 0);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  if (!DrawBox2f(aP, x0, y0, x1, y1, aUV, u0, v0, u1, v1))
    return false;
  
  glBindTexture(GL_TEXTURE_2D, 0);
  if (Error())
    return false;
  
  return true;
}


bool GlesUtil::StoreTexture(GLuint tex, GLenum target,
                            GLenum minFilter, GLenum magFilter,
                            GLenum clampS, GLenum clampT,
                            GLsizei w, GLsizei h, GLenum format, GLenum type,
                            const void *pix, const char *name) {
  if (magFilter != GL_NEAREST && magFilter != GL_LINEAR)
    return false;
  GLenum bindTarget = target == GL_TEXTURE_2D ? GL_TEXTURE_2D :
                                                GL_TEXTURE_CUBE_MAP;
  glBindTexture(bindTarget, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(target, 0, format, w, h, 0, format, type, pix);
  if (Error())
    return false;
  
  glLabelObjectEXT(GL_TEXTURE, tex, 0, name);
  glTexParameteri(bindTarget, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(bindTarget, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(bindTarget, GL_TEXTURE_WRAP_S, clampS);
  glTexParameteri(bindTarget, GL_TEXTURE_WRAP_T, clampT);
  if (Error())
    return false;
  
  const bool needs_mip_chain = minFilter != GL_NEAREST &&
                               minFilter != GL_LINEAR &&
                               (bindTarget == GL_TEXTURE_2D ||
                                target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
  if (needs_mip_chain)
    glGenerateMipmap(bindTarget);
  
  glBindTexture(bindTarget, 0);
  if (Error())
    return false;
  
#if DEBUG
  bool isPow2 = false;
  for (size_t i = 1; i < 16; ++i) {
    if (w == (1L << i)) {
      isPow2 = true;
      break;
    }
  }
  isPow2 = isPow2 && w == h;
  assert(!needs_mip_chain || isPow2);
  assert((clampS == GL_CLAMP_TO_EDGE && clampT == GL_CLAMP_TO_EDGE) || isPow2);
#endif
  
  return true;
}


bool GlesUtil::StoreSubTexture(GLuint tex, GLenum target, GLint miplevel,
                               GLint x, GLint y, GLsizei w, GLsizei h,
                               GLenum format, GLenum type,
                               const void *pix) {
  glBindTexture(target, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexSubImage2D(target, miplevel, x, y, w, h, format, type, pix);
  glBindTexture(target, 0);
  if (Error())
    return false;
  return true;
}


GLint GlesUtil::MaxTextureSize() {
  GLint texSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
  return texSize;
}


bool GlesUtil::IsMSAAResolutionSupported(GLuint w, GLuint h) {
  const GLint maxSize = MaxTextureSize() / 2;
  return w <= maxSize && h <= maxSize;
}


GLuint GlesUtil::CreateShader(GLenum type, const char *source_code) {
  GLuint shader = glCreateShader(type);
  if (!shader)
    return 0;
  
  glShaderSource(shader, 1, &source_code, NULL);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
      char *buf = (char*) malloc(infoLen);
      if (buf) {
        glGetShaderInfoLog(shader, infoLen, NULL, buf);
        free(buf);
      }
      glDeleteShader(shader);
      shader = 0;
    }
  }
  
  if (Error())
    return 0;
  
  return shader;
}


GLuint GlesUtil::CreateProgram(GLuint vp, GLuint fp, const char *name) {
  if (!vp || !fp)
    return 0;
  
  GLuint program = glCreateProgram();
  if (!program)
    return 0;
  
  glAttachShader(program, vp);
  glAttachShader(program, fp);
  glLinkProgram(program);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    GLint bufLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
    if (bufLength) {
      char* buf = (char*) malloc(bufLength);
      if (buf) {
        glGetProgramInfoLog(program, bufLength, NULL, buf);
        free(buf);
      }
    }
    glDeleteProgram(program);
    program = 0;
  }
  
  glLabelObjectEXT(GL_PROGRAM_OBJECT_EXT, program, 0, name);
  if (Error())
    return 0;
  
  return program;
}


GLuint GlesUtil::ConstantProgram(GLuint *aP, GLuint *uC, GLuint *uMVP) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gUC = 0, gUMVP = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "uniform mat4 uMVP;\n"
    "void main() {\n"
    "  gl_Position = uMVP * aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "uniform vec4 uC;\n"
    "void main() {\n"
    "  gl_FragColor = uC;\n"
    "}\n";
    GLuint vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = GlesUtil::CreateProgram(vp, fp, "Constant");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gUC = glGetUniformLocation(gProgram, "uC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *uC = gUC;
  *uMVP = gUMVP;
  return gProgram;
}


GLuint GlesUtil::TextureProgram(GLuint *aP, GLuint *aUV, GLuint *uMVP,
                                GLuint *uTex) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAUV = 0, gUMVP = 0, gUCTex = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "uniform mat4 uMVP;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = uMVP * aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uCTex;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(uCTex, vUV);\n"
    "}\n";
    GLuint vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = GlesUtil::CreateProgram(vp, fp, "Texture");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAUV = glGetAttribLocation(gProgram, "aUV");
    gAP = glGetAttribLocation(gProgram, "aP");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    gUCTex = glGetUniformLocation(gProgram, "uCTex");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *aUV = gAUV;
  *uMVP = gUMVP;
  *uTex = gUCTex;
  return gProgram;
}


GLuint GlesUtil::CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                              GLenum usage, const char *name) {
  GLuint buf;
  glGenBuffers(1, &buf);
  if (buf) {
    glBindBuffer(target, buf);
    glBufferData(target, bytes, data, usage);
  }
  glLabelObjectEXT(GL_BUFFER_OBJECT_EXT, buf, 0, name);
  glBindBuffer(target, 0);
  if (Error())
    return 0;
  return buf;
}


bool GlesUtil::StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                               GLsizeiptr size, void *data) {
  glBindBuffer(target, id);
  glBufferSubData(target, offset, size, data);
  glBindBuffer(target, 0);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::IsExtensionEnabled(const char *extension) {
  static const unsigned char *extensionString = NULL;
  if (!extensionString)
    extensionString = glGetString(GL_EXTENSIONS);
  return strstr((const char *)extensionString, extension) != NULL;
}


struct V2f { float x, y; };                           // 2D Position & UV


GlesUtil::Text::Text() : mFirstASCII(0), mColumnCount(0), mFontTex(0), mProgram(0),
               mAUV(-1), mAP(-1), mUMVP(-1), mUCTex(-1) {
  memset(mFontTexDim, 0, sizeof(mFontTexDim));
  memset(mCharDim, 0, sizeof(mCharDim));
  memset(mCellUVDim, 0, sizeof(mCellUVDim));
  memset(mCharWidth, 0, sizeof(mCharWidth));
}


GlesUtil::Text::~Text() {
  glDeleteTextures(1, &mFontTex);
  glDeleteProgram(mProgram);
}


bool GlesUtil::Text::Init(GLuint cellW, GLuint cellH,
                          const unsigned char *charWidth,
                          GLuint firstASCII, GLuint texW, GLuint texH,
                          GLuint channelCount, const unsigned char *alpha) {
  assert(mFontTex == 0);
  assert(mFontTexDim[0] == 0 && mFontTexDim[1] == 0);
  assert(mCharDim[0] == 0 && mCharDim[1] == 0);
  
  if (cellW == 0 || cellH == 0)
    return false;
  if (texW == 0 || texH == 0 || alpha == NULL)
    return false;
  
  mCharDim[0] = cellW;
  mCharDim[1] = cellH;
  memcpy(mCharWidth, charWidth, sizeof(mCharWidth));
  mFirstASCII = firstASCII;
  mFontTexDim[0] = texW;
  mFontTexDim[1] = texH;
  mCellUVDim[0] = cellW / float(texW);
  mCellUVDim[1] = cellH / float(texH);
  assert(mFontTexDim[0] % mCharDim[0] == 0);          // Not strictly required
  mColumnCount = mFontTexDim[0] / mCharDim[0];
  mProgram = GlesUtil::TextureProgram(&mAP, &mAUV, &mUMVP, &mUCTex);
  
  // Create a texture from the input image
  glGenTextures(1, &mFontTex);
  if (mFontTex == 0)
    return false;
  GLenum glformat = 0;
  switch (channelCount) {
    case 1: glformat = GL_ALPHA;           break;
    case 2: glformat = GL_LUMINANCE_ALPHA; break;
    case 3: glformat = GL_RGB;             break;
    case 4: glformat = GL_RGBA;            break;
  }
  if (!GlesUtil::StoreTexture(mFontTex, GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR,
                              GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
                              mFontTexDim[0], mFontTexDim[1],
                              glformat, GL_UNSIGNED_BYTE, alpha, "Font"))
    return false;
  
  return true;
}


bool GlesUtil::Text::Draw(const char *text, float x, float y) {
  if (text == NULL)
    return false;
  
  size_t len = strlen(text);
  if (len == 0)
    return false;
  
  // Create attribute arrays for the text indexed tristrip
  std::vector<V2f> P(4*len);
  std::vector<V2f> UV(4*len);
  const size_t idxCount = (4 + 3) * len - 3;          // 4/quad + 3 degen - last
  std::vector<unsigned short> idx(idxCount);
  
  // Build a tristrip of characters with pixel coordinates and UVs.
  // Characters are drawn as quads with UV coordinates that map into
  // the font texture. Character quads are fixed sized (mCharDim), and
  // can overlap due to kerning via mCharWidth. Quads are rendered as
  // a tristrip with degenerate tris connecting each quad.
  float curX = 0;
  for (size_t i = 0; i < len; ++i) {
    const int x0 = curX;                              // Integer pixel coords
    const int y0 = 0;
    const int x1 = curX + mCharDim[0];
    const int y1 = mCharDim[1];
    const int k = text[i] - mFirstASCII;              // Glyph index
    curX += mCharWidth[k];                            // Kerning offset in X
    const size_t j = i * 4;                           // Vertex index
    P[j+0].x = x0; P[j+0].y = y0;                     // Vertex positions
    P[j+1].x = x0; P[j+1].y = y1;
    P[j+2].x = x1; P[j+2].y = y0;
    P[j+3].x = x1; P[j+3].y = y1;
    const int row = k / mColumnCount;                 // Glyph location
    const int col = k % mColumnCount;
    assert(row < 16 && col < 16 && row >= 0 && col >= 0);
    const float u0 = col * mCellUVDim[0];             // Cell texture coords
    const float v0 = row * mCellUVDim[1];
    const float u1 = (col + 1) * mCellUVDim[0];
    const float v1 = (row + 1) * mCellUVDim[1];
    UV[j+0].x = u0; UV[j+0].y = v1;                   // Vertex texture coords
    UV[j+1].x = u0; UV[j+1].y = v0;                   // Ick! Flipped texture.
    UV[j+2].x = u1; UV[j+2].y = v1;
    UV[j+3].x = u1; UV[j+3].y = v0;
    const size_t q = i * 7;                           // First idx
    idx[q+0] = j;
    idx[q+1] = j+1;
    idx[q+2] = j+2;
    idx[q+3] = j+3;
    if (i < len - 1) {                                // Skip last degen
      idx[q+4] = j+3;                                 // Degenerate tri
      idx[q+5] = j+4;
      idx[q+6] = j+4;
    }
  }
  
  // Set the current viewport to the text string
  // Turn on blending to handle overlapping glyphs
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  
  // Allocate a viewport big enough to fit the entire string
  const int fx = floorf(x);
  const int fy = floorf(y);
  const int w = curX + (x - fx > 0 ? 1 : 0);
  const int h = mCharDim[1] + (y - fy > 0 ? 1 : 0);
  glViewport(fx, fy, w, h);
  
  // Set up the shaders
  glUseProgram(mProgram);                             // Setup program
  glEnableVertexAttribArray(mAUV);                    // Vertex arrays
  glEnableVertexAttribArray(mAP);
  glVertexAttribPointer(mAUV, 2, GL_FLOAT, GL_FALSE, 0, &UV[0].x);
  glVertexAttribPointer(mAP, 2, GL_FLOAT, GL_FALSE, 0, &P[0].x);
  glActiveTexture(GL_TEXTURE0);                       // Setup texture
  glBindTexture(GL_TEXTURE_2D, mFontTex);
  const float MVP[16] = { 2.0/w,0,0,0, 0,2.0/h,0,0, 0,0,1,0, -1,-1,0,1 };
  glUniformMatrix4fv(mUMVP, 1, GL_FALSE, MVP);
  
  // Draw the tristrip with all the character rectangles
  glDrawElements(GL_TRIANGLE_STRIP, idxCount, GL_UNSIGNED_SHORT, &idx[0]);
  
  glDisableVertexAttribArray(mAUV);
  glDisableVertexAttribArray(mAP);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  return true;
}


GLuint GlesUtil::Text::ComputeWidth(const char *text) const {
  if (text == NULL)
    return 0;
  
  size_t len = strlen(text);
  if (len == 0)
    return 0;
  
  GLuint x = 0;
  for (size_t i = 0; i < len; ++i) {
    const int k = text[i] - mFirstASCII;              // Glyph index
    x += mCharWidth[k];                            // Kerning offset in X
  }
  
  return x;
}
