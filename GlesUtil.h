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
                       const float *MVP = 0);
bool DrawDropshadowBox2f(float x0, float y0, float x1, float y1,
                         float r, float g, float b, float a,
                         bool isVertical, const float *MVP = 0);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                   float u0, float v0, float u1, float v1,
                   const float *MVP = 0);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                   float u0, float v0, float u1, float v1,
                   float r, float g, float b, float a,
                   const float *MVP = 0);
bool Draw3SliceTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                         float u0, float v0, float u1, float v1,
                         int texW, int texH, int vpW, int vpH,
                         float r, float g, float b, float a,const float *MVP=0);
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


// Bitmapped Fonts:
//   (ptW,ptH):  Scaling from points to NDC/MVP space.
//               For [-1,-1]x[1,1] use (1/vpW,1/vpH) and
//               when rendering in pixel units, (1,1).
//    Font:      Font texture, size and kerning information, created externally
//               assuming a 16x16 grid of ASCII characters (e.g F2IBuilder).
//    FontSet:   Set of Fonts of the same family, ordered by size.
//    FontStyle: Rendering parameter for each draw call.
//
//  Build a Font by loading a texture and setting the sizing and kerning data.
//  Create a font set using multiple fonts from the same family, sorted.
//  Characters are constructed out of quads in a tristrip defined in
//  points, which are scaled to NDC/MVP coordinates using (ptW, ptH).
//  For MVP=Identity, the default, NDC is [-1,-1]x[1,1] and we scale by
//  (1/vpW, 1/vpH). When rendering in pixels, use (1,1) for points=pixels.
//  Drawing locations & rects are specified in MVP space (i.e. NDC default).
//  Kerning is supported using a point offset per character.
//  Unused ASCII character slots can be used for special character icons.

struct Font {
  Font();                                             // Zero out memory
  float charDimUV[2];                                 // UV space between chars
  int charDimPt[2];                                   // Point size of character
  unsigned char charWidthPt[256];                     // Kerning offset in pts
  GLuint tex;                                         // 16x16 ASCII char grid
  enum { MagGlassChar=16, StarChar=17, FlagChar=18, InfoChar=19, LevelsChar=20 };
};

struct FontSet {                                      // Sizes for one font
  FontSet() : fontVec(0), fontCount(0) {}             // Zero out memory
  const Font *fontVec;                                // Increasing pt size
  unsigned int fontCount;                             // Size of fontVec
  const Font &Font(float pts) const;                  // Return best match
};

struct FontStyle {                                    // Render style (one pass)
  FontStyle() {                                       // Initialize to defaults
    C[0] = C[1] = C[2] = C[3] = 1;                    // White text
    dropshadowOffsetPts[0] = dropshadowOffsetPts[1]=0;// Dropshadow disabled
    dropshadowC[0] = dropshadowC[1] = dropshadowC[2] = 0; dropshadowC[3] = 1;
  }
  float C[4];                                         // Text color
  float dropshadowOffsetPts[2];                       // 0 -> no dropshadow
  float dropshadowC[4];                               // Dropshadow color
};
  
// Return the length, in pts, of a given string. Pts are an arbitrary
// unit used for all text drawing. Often pts=pixels, but you can scale
// pts using ptW,ptH in the drawing functions and the MVP also affects pts.
// The returned length is the longest line ('\n') within the input string.
// If isKerned is true, the returned width is the exact number of points
// from the leftmost part of the first character to the rightmost part of
// the last character. If isKerned is false, the width is the number of
// characters in the longest line multiplied by the font point width.
  
unsigned int TextWidth(const char *text, const Font *font, bool isKerned);

// Report the font sizes requested for the specified font.
// Only one debug font allowed, any previous font will be ignored.
  
void DebugFontSizes(const FontSet &fontSet, const char *name);
  
// Draw a one-line string of text. Starting location is the lower
// left corner of the first character in MVP space. Linefeeds are ignored.
// (ptW, ptH) scale points into MVP (e.g. NDC) space, and are typically set
// to (2 / vpW, 2 / vpH) to scale to [-1, -1] x [1, 1] assuming the text
// is intended to fill the viewport and the width was computed with TextWidth
// with kerning enabled. If the MVP defines NDC space in pixels, and the
// points are pixels (font point size matches desired size in pixels),
// then ptW == ptH == 1.

bool DrawText(const char *text, float x, float y, const Font *font,
              float ptW, float ptH, const FontStyle *style = 0,
              const float *MVP = 0, float charPadPt = 0,
              int firstChar = 0, int lastChar = -1);

// Paragraph rendering with text justification and word wrapping within
// the specified rectangle. Text is aligned within the specified rectangle,
// starting one line down from the top (y1). No explicit clipping is performed,
// however lines that fall below the bottom of the rect (y0) will be skipped.
// Consider adding tab stops to paragraph layout?
  
enum Align { LeftJustify, CenterJustify, RightJustify, FullJustify };

bool DrawParagraph(const char *text, float x0, float y0, float x1, float y1,
                   Align align, const Font *font, float ptW, float ptH,
                   const FontStyle *style = 0, const float *MVP = 0,
                   int firstChar = 0, int lastCar = -1, bool wrapLines = true);


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


// Tristrip builders:
//   Build common primitives into pre-allocated tristrip buffers.
//   Use the Draw*Strip* functions to render the geometry.
//   Consider storing geometry in buffers for improved performance.
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
