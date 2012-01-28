//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef GLES_UTIL_H
#define GLES_UTIL_H

#include <OpenGLES/ES2/gl.h>


namespace GlesUtil {

bool Error();
const char *ErrorString();
  
//   Target:  GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP
//   Filters: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
//            GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST,
//            GL_LINEAR_MIPMAP_LINEAR
//   Format:  GL_RGBA, GL_RGB, GL_LIMINANCE_ALPHA, GL_LUMINANCE, GL_ALPHA
//   Type:    GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_4_4_4_4, *_5_5_5_1, *_5_6_5
bool StoreTexture(GLuint tex, GLenum target,
                  GLenum minFilter, GLenum magFilter,
                  GLsizei w, GLsizei h, GLenum format, GLenum type,
                  const void *pix);
  
//   Type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER
GLuint CreateShader(GLenum type, const char *source_code);
GLuint CreateProgram(GLuint vp, GLuint fp);
  
//   Target: GL_ARRAY_BUFFER, GL_ELEMNT_ARRAY_BUFFER
//   Usage:  GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STRAM_DRAW
GLuint CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                    GLenum usage);
bool StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                    GLsizeiptr size, void *data);

};      // namespace GlesUtil



#endif  // GLES_UTIL_H
