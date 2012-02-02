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
                            GLenum clampS, GLenum clampT,
                            GLsizei w, GLsizei h, GLenum format, GLenum type,
                            const void *pix) {
  glBindTexture(target, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(target, 0, format, w, h, 0, format, type, pix);
  if (Error())
    return false;
  
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, clampS);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, clampT);
  
  bool needs_mip_chain = minFilter != GL_NEAREST && minFilter != GL_LINEAR;
  needs_mip_chain |= magFilter && magFilter != GL_NEAREST;
  if (needs_mip_chain)
    glGenerateMipmap(target);
  if (Error())
    return false;
  
  return true;
}


bool GlesUtil::DrawTexture(GLuint tex, const float ndcViewport[4]) {
  static bool gInitialized = false;
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAUV = 0, gUCTex = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *dbgVpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = aP;\n"
    "}\n";
    static const char *dbgFpCode =
    "precision mediump float;\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uCTex;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(uCTex, vUV);\n"
    "}\n";
    GLuint vp = GlesUtil::CreateShader(GL_VERTEX_SHADER, dbgVpCode);
    if (!vp)
      return false;
    GLuint fp = GlesUtil::CreateShader(GL_FRAGMENT_SHADER, dbgFpCode);
    if (!fp)
      return false;
    gProgram = GlesUtil::CreateProgram(vp, fp);
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAUV = glGetAttribLocation(gProgram, "aUV");
    gAP = glGetAttribLocation(gProgram, "aP");
    gUCTex = glGetUniformLocation(gProgram, "uCTex");
    if (Error())
      return false;
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  
  glUseProgram(gProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(gUCTex, 0);
  
  glEnableVertexAttribArray(gAP);
  const float x0 = ndcViewport[0];
  const float y0 = ndcViewport[1];
  const float x1 = x0 + ndcViewport[2];
  const float y1 = y0 + ndcViewport[3];
  const float P[8] = {x0, y0,  x0, y1,  x1, y0,  x1, y1 };
  const float UV[8] = {0, 0,  0, 1,  1, 0,  1, 1 };
  glVertexAttribPointer(gAUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glVertexAttribPointer(gAP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
