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

  
// Global queries:
bool IsFramebufferComplete();                         // Validate FBO
bool IsExtensionEnabled(const char *extension);       // Check for extension
bool IsMSAAResolutionSupported(GLuint w, GLuint h);   // Test MSAA res

  
// Drawing functions:
//   Attributes: aP is used for position, aUV for texture coordinates.
//   MVP defaults to unit matrix, implying NDC space [-1,-1]x[1,1]
//   MVP is the "model view projection" matrix from coords -> NDC space.
  
bool DrawColorLines2f(unsigned int count, const float *P,
                      float r, float g, float b, float a, const float *MVP = 0);
bool DrawBox2f(GLuint aP, float x0, float y0, float x1, float y1,
               GLuint aUV, float u0, float v0, float u1, float v1);
bool DrawBox2f4uv(GLuint aP, float x0, float y0, float x1, float y1,
                 GLuint aUVST, float u0, float v0, float u1, float v1,
                               float s0, float t0, float s1, float t1);
bool DrawColorBox2f(float x0, float y0, float x1, float y1,
                    float r, float g, float b, float a, const float *MVP = 0);
bool DrawBoxFrame2f(GLuint aP, float x0, float y0, float x1, float y1,
                    float w, float h, GLuint aUV);
bool DrawColorBoxFrame2f(float x0, float y0, float x1, float y1,
                         float w, float h, float r, float g, float b, float a,
                         const float *MVP=0);
bool DrawGradientBox2f(float x0, float y0, float x1, float y1, bool isVertical,
                       float r0, float g0, float b0, float r1,float g1,float b1,
                       const float *MVP=0);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                   float u0, float v0, float u1, float v1,
                   const float *MVP = 0);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                   float u0, float v0, float u1, float v1,
                   float r, float g, float b, float a,
                   const float *MVP = 0);
bool DrawTwoTexture2f(GLuint uvTex, float stTex,
                      float x0, float y0, float x1, float y1,
                      float u0, float v0, float u1, float v1,
                      float s0, float t0, float s1, float t1,
                      float r, float g, float b, float a,
                      const float *MVP = 0);
bool DrawTextureStrip2f(GLuint tex, unsigned int vcount, const float *P,
                        const float *UV, float r, float g, float b, float a,
                        const float *MVP = 0);
bool DrawTextureStrip2fi(GLuint tex, unsigned short icount, const float *P,
                         const float *UV, const unsigned short *idx,
                         float r, float g, float b, float a,const float *MVP=0);
bool DrawDropshadowStrip2fi(unsigned short icount,const float *P,const float *UV,
                            const unsigned short *idx, float r, float g,float b,
                            float a, const float *MVP = 0);

// Bitmapped font drawing functions:
//   Characters are defined in points and scaled to MVP space using (ptW,ptH),
//   which are typically set using the viewport (e.g. 2/vpWidth).
//   MVP defaults to unit matrix, implying NDC space [-1,-1]x[1,1].
//   Drawing locations & rects are specified in MVP space (i.e. NDC default).
//
// Issues:
//   - Unicode is not currently supported. Unused ASCII character slots
//     in the texture could be filled with unicode characters?
//   - There is no way to specify a font color or shading.

struct Font {
  float charDimUV[2];                                 // UV space between chars
  int charDimPt[2];                                   // Point size of character
  unsigned char charWidthPt[256];                     // Kerning offset in pts
  GLuint tex;                                         // 16x16 ASCII char grid
  enum { MagGlassChar=16, StarChar=17, FlagChar=18, InfoChar=19, LevelsChar=20 };
};

// Return the length, in pts, of a given string. Pts are an arbitrary
// unit used for all text drawing. Often pts=pixels, but you can scale
// pts using ptW,ptH in the drawing functions and the MVP also affects pts.
unsigned int TextWidth(const char *text, const Font *font, float charPadPt = 0);

// Draw a one-line string of text. Starting location is the lower
// left corner of the first character in MVP space. Linefeeds are ignored.
// (ptW, ptH) scale points into MVP (e.g. NDC) space, and are typically set
// to (2 / vpW, 2 / vpH) to scale to [-1, -1] x [1, 1]
// Note: we should remove ptW,ptH and put xform into MVP, simpler.

bool DrawText(const char *text, float x, float y, const Font *font,
              float ptW, float ptH, const float *MVP = 0, float charPadPt = 0);
bool DrawText(const char *text, float x, float y, const Font *font,
              float ptW, float ptH, float r, float g, float b, float a,
              const float *MVP = 0, float charPadPt = 0);

// Paragraph rendering with text justification and word wrapping within
// the specified rectangle. Text is aligned within the specified rectangle,
// starting one line down from the top (y1). No explicit clipping is performed,
// however lines that fall below the bottom of the rect (y0) will be skipped.
// Consider adding tab stops to paragraph layout?
// Note: we should remove ptW,ptH and put xform into MVP, simpler.
 
enum Align { LeftJustify, CenterJustify, RightJustify, FullJustify };

bool DrawParagraph(const char *text, float x0, float y0, float x1, float y1,
                   Align align, const Font *font, float ptW, float ptH,
                   const float *MVP = 0);
bool DrawParagraph(const char *text, float x0, float y0, float x1, float y1,
                   Align align, const Font *font, float ptW, float ptH,
                   float r, float g, float b, float a, const float *MVP = 0);


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
//   Name:    Debugging only. Names texture in XCode.

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
//   Name:    Debugging only. Names texture in XCode.

GLuint CreateShader(GLenum type, const char *source);
GLuint CreateProgram(GLuint vp, GLuint fp, const char *name=0);
  
GLuint ConstantProgram(GLuint *aP, GLuint *uC, GLuint *uMVP);
GLuint VertexColorProgram(GLuint *aP, GLuint *aC, GLuint *uMVP);
GLuint DropshadowFrameProgram(GLuint *aP, GLuint *aUV, GLuint *uC,
                              GLuint *uMVP);
GLuint TextureProgram(GLuint *aP, GLuint *aUV, GLuint *uC, GLuint *uMVP,
                      GLuint *uTex);
GLuint ScreenTextureProgram(GLuint *aP, GLuint *uC, GLuint *uMVP, GLuint *uTex);
GLuint TwoTextureProgram(GLuint *aP, GLuint *aUVST, GLuint *uC, GLuint *uMVP,
                         GLuint *uUVTex, GLuint *uSTTex);

// Buffer functions:
//   Target: GL_ARRAY_BUFFER, GL_ELEMNT_ARRAY_BUFFER
//   Usage:  GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STRAM_DRAW
//   Name:    Debugging only. Names texture in XCode.

GLuint CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                    GLenum usage, const char *name=0);
bool StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                    GLsizeiptr size, void *data);


// Tristrip functions build common primitives into pre-allocated buffers.
void RoundedRectSize2fi(int cornerSegments, unsigned short *vertexCount,
                        unsigned short *idxCount);
void BuildRoundedRect2fi(float x0, float y0, float x1, float y1,
                         float u0, float v0, float u1, float v1,
                         float radiusX, float radiusY, int segments,
                         float *P, float *UV, unsigned short *idx);
void RoundedFrameSize2fi(int cornerSegments, unsigned short *vertexCount,
                         unsigned short *idxCount);
void BuildRoundedFrame2fi(float x0, float y0, float x1, float y1,
                          float radiusX, float radiusY, int segments,
                          float *P, float *UV, unsigned short *idx);


};      // namespace GlesUtil



#endif  // GLES_UTIL_H
