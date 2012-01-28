//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "GlesUtil.h"
#include <stdlib.h>


static GLint gErrorCode = GL_NO_ERROR;


bool GlesUtil::Error() {
  gErrorCode = glGetError();            // Note: GLES only has one active err
  return gErrorCode != GL_NO_ERROR;
}


const char *GlesUtil::ErrorString() {
  switch (gErrorCode) {
    case GL_NO_ERROR:           return "No error";
    case GL_INVALID_ENUM:       return "Invalid enum";
    case GL_INVALID_OPERATION:  return "Invalid operation";
    case GL_INVALID_VALUE:      return "Invalid value";
    case GL_OUT_OF_MEMORY:      return "Out of memory";
    default:                    return "Unknown";
  }
}


bool GlesUtil::StoreTexture(GLuint tex, GLenum target,
                            GLenum minFilter, GLenum magFilter,
                            GLsizei w, GLsizei h, GLenum format, GLenum type,
                            const void *pix) {
  glBindTexture(target, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(target, 0, format, w, h, 0, format, type, pix);
  if (Error())
    return false;
  
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
  
  bool needs_mip_chain = minFilter != GL_NEAREST && minFilter != GL_LINEAR;
  needs_mip_chain |= magFilter && magFilter != GL_NEAREST;
  if (needs_mip_chain)
    glGenerateMipmap(target);
  if (Error())
    return false;
  
  return true;
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
      char* buf = (char*) malloc(infoLen);
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


GLuint GlesUtil::CreateProgram(GLuint vp, GLuint fp) {
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
  
  if (Error())
    return 0;
  
  return program;
}


GLuint GlesUtil::CreateBuffer(GLenum target, GLsizeiptr bytes, void *data,
                              GLenum usage) {
  GLuint buf;
  glGenBuffers(1, &buf);
  if (buf) {
    glBindBuffer(target, buf);
    glBufferData(target, bytes, data, usage);
  }
  if (Error())
    return 0;
  return buf;
}


bool GlesUtil::StoreSubBuffer(GLuint id, GLenum target, GLintptr offset,
                               GLsizeiptr size, void *data) {
  glBindBuffer(target, id);
  glBufferSubData(target, offset, size, data);
  if (Error())
    return false;
  return true;
}
