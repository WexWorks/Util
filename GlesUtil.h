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
bool DrawQuad2f(GLuint aP, float x0, float y0, float x1, float y1,
                GLuint aUV, float u0, float v0, float u1, float v1);
bool DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1); // NDC
  
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
                  const void *pix);


// Shader functions:
//   Type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER
GLuint CreateShader(GLenum type, const char *source_code);
GLuint CreateProgram(GLuint vp, GLuint fp);
  
// Buffer functions:
//   Target: GL_ARRAY_BUFFER, GL_ELEMNT_ARRAY_BUFFER
//   Usage:  GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STRAM_DRAW
GLuint CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                    GLenum usage);
bool StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                    GLsizeiptr size, void *data);

};      // namespace GlesUtil



#endif  // GLES_UTIL_H
