//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef GLES_UTIL_H
#define GLES_UTIL_H

#include <OpenGLES/ES2/gl.h>


// Helpful routines for common OpenGLES 2 operations.
// Objects created by these routines must be destroyed by the caller

namespace GlesUtil {


// Called internally as needed by utility functions, and should be
// called by applications that call OpenGLES functions directly.
// It is up to the caller to use ErrorString to display messages.
bool Error();
const char *ErrorString();


// Drawing utilities:
//   Attributes: aP is used for position, aUV for texture coordinates.
bool DrawBox2f(GLuint aP, float x0, float y0, float x1, float y1,
               GLuint aUV, float u0, float v0, float u1, float v1);
bool DrawColorBox2f(float x0, float y0, float x1, float y1,
                    float r, float g, float b, float a, const float *MVP = 0);
bool DrawBoxFrame2f(GLuint aP, float x0, float y0, float x1, float y1,
                    float w, float h, GLuint aUV);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                   float u0, float v0, float u1, float v1,
                   const float *MVP = 0);


// Texture functions:
//   Target:  GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP
//   Filters: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
//            GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST,
//            GL_LINEAR_MIPMAP_LINEAR
//            Note: Avoid mips with min=near|linear & mag=near
//   Wraps:   GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT
//            Note: Rectangular textures require clamp or GL_OES_texture_npot
//   Format:  GL_RGBA, GL_RGB, GL_LIMINANCE_ALPHA, GL_LUMINANCE, GL_ALPHA
//   Type:    GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_4_4_4_4, *_5_5_5_1, *_5_6_5

bool StoreTexture(GLuint tex, GLenum target,
                  GLenum minFilter, GLenum magFilter,
                  GLenum clampS, GLenum clampT,
                  GLsizei w, GLsizei h, GLenum format, GLenum type,
                  const void *pix, const char *name=0);
bool StoreSubTexture(GLuint tex, GLenum target, GLint miplevel, GLint x,GLint y,
                     GLsizei w, GLsizei h, GLenum format, GLenum type,
                     const void *pix);
GLint MaxTextureSize();


// Shader functions:
//   Type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER

GLuint CreateShader(GLenum type, const char *source_code);
GLuint CreateProgram(GLuint vp, GLuint fp, const char *name=0);
GLuint ConstantProgram(GLuint *aP, GLuint *uC, GLuint *uMVP);
GLuint TextureProgram(GLuint *aP, GLuint *aUV, GLuint *uMVP, GLuint *uTex);


// Buffer functions:
//   Target: GL_ARRAY_BUFFER, GL_ELEMNT_ARRAY_BUFFER
//   Usage:  GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STRAM_DRAW

GLuint CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                    GLenum usage, const char *name=0);
bool StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                    GLsizeiptr size, void *data);


// Check for a named extension:

bool IsExtensionEnabled(const char *extension);
bool IsMSAAResolutionSupported(GLuint w, GLuint h);

  
// Bitmapped font drawing:
//   Given a set of font metrics and a font image, create a font texture
//   and provide a function for drawing a character string at a specified
//   2D location in pixel space.
//
// Issues with the current implementation:
//   - Wasted texture space. We have all 256 ASCII values, but many are unused,
//     because we typically use a texture with a 16x16 grid of ASCII characters.
//   - Assumes the texture has pre-multiplied color (for blend op).
//   - Unicode support would be nice.
//   - Perhaps some more ways of specifying color, or doing shading?
//   - Rotation? Scaling? Arbitrary transform?
  
class Text {
public:
  Text();
  ~Text();
  
  // Provide font metrics and an image with font glyphs.
  // Cell dimensions are assumed to be constant for all characters.
  // Texture is assumed to be in ASCII order, starting with firstASCII.
  // Individual character widths passed in charWidth for kerning.
  // Channel count implies tex is 1=alpha-only, 2=lum-alpha, 3=rgb, 4=rgba.
  bool Init(GLuint cellW, GLuint cellH, const unsigned char charWidth[256],
            GLuint firstASCII, GLuint texW, GLuint texH, GLuint channelCount,
            const unsigned char *texel);
  
  // Draw the string at the specified location in pixel space.
  // Draw sets the glViewport and enables GL_BLEND, and it is up to
  // the caller to reset this state to previous values (to avoid glGet).
  bool Draw(const char *text, float x, float y);
  
  // Compute the width of text in pixels.
  GLuint ComputeWidth(const char *text) const;
  GLuint Height() const { return mCharDim[1]; }
  
  
private:
  Text(const Text &);                                 // Disallow copy ctor
  void operator=(const Text &);                       // Disallow assignment
  
  GLuint mFontTexDim[2];                              // Size of font texture
  GLuint mCharDim[2];                                 // Size of character cell
  float mCellUVDim[2];                                // Char size in UV space
  int mFirstASCII;                                    // ASCII offset
  unsigned char mCharWidth[256];                      // Pixel width of chars
  int mColumnCount;                                   // Cell columns in texture
  GLuint mFontTex;                                    // Font texture
  GLuint mProgram;                                    // GLSL program
  GLuint mAUV, mAP, mUMVP, mUCTex;                    // Shader variables
};


};      // namespace GlesUtil



#endif  // GLES_UTIL_H
