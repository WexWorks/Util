//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "GlesUtil.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


static GLint gErrorCode = GL_NO_ERROR;


bool GlesUtil::Error() {
#if DEBUG
  gErrorCode = glGetError();            // Note: GLES only has one active err
  return gErrorCode != GL_NO_ERROR;
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
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1) {
  GLuint aP, aUV, uTex;
  GLuint program = TextureProgram(&aP, &aUV, &uTex);
  glUseProgram(program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(uTex, 0);
  
  return DrawBox2f(aP, x0, y0, x1, y1, aUV, u0, v0, u1, v1);
}


bool GlesUtil::StoreTexture(GLuint tex, GLenum target,
                            GLenum minFilter, GLenum magFilter,
                            GLenum clampS, GLenum clampT,
                            GLsizei w, GLsizei h, GLenum format, GLenum type,
                            const void *pix) {
  if (magFilter != GL_NEAREST && magFilter != GL_LINEAR)
    return false;
  glBindTexture(target, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(target, 0, format, w, h, 0, format, type, pix);
  if (Error())
    return false;
  
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, clampS);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, clampT);
  
  const bool needs_mip_chain = minFilter != GL_NEAREST && minFilter !=GL_LINEAR;
  if (pix && needs_mip_chain)
    glGenerateMipmap(target);
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


GLuint GlesUtil::ConstantProgram(GLuint *aP, GLuint *uC) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gUC = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "void main() {\n"
    "  gl_Position = aP;\n"
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
    gProgram = GlesUtil::CreateProgram(vp, fp);
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gUC = glGetUniformLocation(gProgram, "uC");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *uC = gUC;
  return gProgram;
}


GLuint GlesUtil::TextureProgram(GLuint *aP, GLuint *aUV, GLuint *uTex) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAUV = 0, gUCTex = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec2 aUV;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = aP;\n"
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
  
  *aP = gAP;
  *aUV = gAUV;
  *uTex = gUCTex;
  return gProgram;
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


bool GlesUtil::IsExtensionEnabled(const char *extension) {
  static const unsigned char *extensionString = NULL;
  if (!extensionString)
    extensionString = glGetString(GL_EXTENSIONS);
  return strstr((const char *)extensionString, extension) != NULL;
}
