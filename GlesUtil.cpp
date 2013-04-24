//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "GlesUtil.h"

#include <OpenGLES/ES2/glext.h>

#include <assert.h>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <math.h>


static GLint gErrorCode = GL_NO_ERROR;


bool GlesUtil::Error() {
#if DEBUG
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


bool GlesUtil::IsFramebufferComplete() {
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status == GL_FRAMEBUFFER_COMPLETE)
    return true;
#if DEBUG
  const char *errstr = NULL;
  switch (status) {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      errstr = "Incomplete attachment";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      errstr = "Missing attachment";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      errstr = "Attachments do not have the same dimensions";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE:
      errstr = "Internal attachment format not renderable";
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      errstr = "Combination of attachment internal formats is not renderable";
      break;
    default:
      errstr = "Unknown framebuffer error";
      break;
  }
  printf("GL FBO ERROR: %s\n", errstr);
#endif
  return false;
}


//
// Drawing
//

bool GlesUtil::DrawColorLines2f(unsigned int count, const float *P,
                                float r, float g, float b, float a,
                                const float *MVP) {
  GLuint aP, uC, uMVP;
  GLuint program = GlesUtil::ConstantProgram(&aP, &uC, &uMVP);
  if (!program)
    return false;
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  glEnableVertexAttribArray(aP);
  glDrawArrays(GL_LINES, 0, count);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawBox2f(GLuint aP, float x0, float y0, float x1, float y1,
                         GLuint aUV, float u0, float v0, float u1, float v1) {
  const float P[8] = { x0, y0,  x0, y1,  x1, y0,  x1, y1 };
  if (aUV != -1) {
    const float UV[8] = { u0, v0,  u0, v1,  u1, v0,  u1, v1 };
    glEnableVertexAttribArray(aUV);
    glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  }
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  if (aUV != -1)
    glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawBox2f4uv(GLuint aP,  float x0, float y0, float x1, float y1,
                            GLuint aUV, float u0, float v0, float u1, float v1,
                                        float s0, float t0, float s1, float t1){
  const float P[8] = { x0, y0,  x0, y1,  x1, y0,  x1, y1 };
  if (aUV != -1) {
    const float UV[16] = { u0,v0,s0,t0, u0,v1,s0,t1, u1,v0,s1,t0, u1,v1,s1,t1 };
    glEnableVertexAttribArray(aUV);
    glVertexAttribPointer(aUV, 4, GL_FLOAT, GL_FALSE, 0, UV);
  }
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  if (aUV != -1)
    glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawColorBox2f(float x0, float y0, float x1, float y1,
                              float r, float g, float b, float a,
                              const float *MVP) {
  GLuint aP, uC, uMVP;
  GLuint program = GlesUtil::ConstantProgram(&aP, &uC, &uMVP);
  if (!program)
    return false;
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  if (!GlesUtil::DrawBox2f(aP, x0, y0, x1, y1, -1, 0, 0, 0, 0))
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
  if (aUV != -1) {
    glEnableVertexAttribArray(aUV);
    glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  }
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, idx);
  glDisableVertexAttribArray(aP);
  if (aUV != -1)
    glDisableVertexAttribArray(aUV);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawColorBoxFrame2f(float x0, float y0, float x1, float y1,
                                   float w, float h, float r, float g, float b,
                                   float a, const float *MVP) {
  GLuint aP, uC, uMVP;
  GLuint program = ConstantProgram(&aP, &uC, &uMVP);
  if (!program)
    return false;
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  if (!DrawBoxFrame2f(aP, x0, y0, x1, y1, w, h, -1))
    return false;

  return true;
}


bool GlesUtil::DrawGradientBox2f(float x0, float y0, float x1, float y1,
                                 bool isVertical, float r0, float g0, float b0,
                                 float r1,float g1,float b1, const float *MVP) {
  GLuint aP, aC, uMVP;
  GLuint program = VertexColorProgram(&aP, &aC, &uMVP);
  if (!program)
    return false;
  glUseProgram(program);
  const float P[8] = { x0, y0,  x0, y1,  x1, y0,  x1, y1 };
  const float vC[4*4] = { r0,g0,b0,1,  r1,g1,b1,1, r0,g0,b0,1,  r1,g1,b1,1 };
  const float hC[4*4] = { r0,g0,b0,1,  r0,g0,b0,1, r1,g1,b1,1,  r1,g1,b1,1 };
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP ? MVP : I);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glEnableVertexAttribArray(aC);
  glVertexAttribPointer(aC, 4, GL_FLOAT, GL_FALSE, 0, isVertical ? vC : hC);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(aP);
  glDisableVertexAttribArray(aC);
  if (Error())
    return false;
  return true;
}


bool GlesUtil::DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             float r, float g, float b, float a,
                             const float *MVP) {
  GLuint aP, aUV, uC, uMVP, uTex;
  GLuint program = TextureProgram(&aP, &aUV, &uC, &uMVP, &uTex);
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
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


bool GlesUtil::DrawTexture2f(GLuint tex, float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             const float *MVP) {
  return DrawTexture2f(tex, x0, y0, x1, y1, u0, v0, u1, v1, 1, 1, 1, 1, MVP);
}


bool GlesUtil::Draw3SliceTexture2f(GLuint tex,
                                   float x0, float y0, float x1, float y1,
                                   float u0, float v0, float u1, float v1,
                                   int texW, int texH, int vpW, int vpH,
                                   float r, float g, float b, float a,
                                   const float *MVP) {
  const int edgeDim = vpH * texW / (2 * texH);
  const float ew = edgeDim / float(MVP ? 1 : 0.5 * vpW);
  const float mu = 0.5 * (u0 + u1);
  if (!GlesUtil::DrawTexture2f(tex, x0,y0,x0+ew,y1, u0,v0,mu,v1, r,g,b,a, MVP))
    return false;
  if (!GlesUtil::DrawTexture2f(tex, x0+ew,y0,x1-ew,y1,mu,v0,mu,v1,r,g,b,a, MVP))
    return false;
  if (!GlesUtil::DrawTexture2f(tex, x1-ew,y0,x1,y1, mu,v0,u1,v1, r,g,b,a, MVP))
    return false;
  return true;
}


bool GlesUtil::DrawTwoTexture2f(GLuint uvTex, float stTex,
                                float x0, float y0, float x1, float y1,
                                float u0, float v0, float u1, float v1,
                                float s0, float t0, float s1, float t1,
                                float r, float g, float b, float a,
                                const float *MVP) {
  GLuint aP, aUV, uC, uMVP, uUVTex, uSTTex;
  GLuint program = TwoTextureProgram(&aP, &aUV, &uC, &uMVP, &uUVTex, &uSTTex);
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, uvTex);
  glUniform1i(uUVTex, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, stTex);
  glUniform1i(uSTTex, 1);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  if (!DrawBox2f4uv(aP, x0, y0, x1, y1, aUV, u0, v0, u1, v1, s0, t0, s1, t1))
    return false;
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  if (Error())
    return false;
  
  return true;
}


bool GlesUtil::DrawTextureStrip2f(GLuint tex, GLuint vcount, const float *P,
                                  const float *UV, float r, float g, float b,
                                  float a, const float *MVP) {
  GLuint aP, aUV, uC, uMVP, uTex;
  GLuint program = TextureProgram(&aP, &aUV, &uC, &uMVP, &uTex);
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(uTex, 0);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);

  glEnableVertexAttribArray(aUV);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, vcount);
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;

  return true;
}


bool GlesUtil::DrawTextureStrip2fi(GLuint tex, unsigned short icount,
                                   const float *P, const float *UV,
                                   const unsigned short *idx, float r, float g,
                                   float b, float a, const float *MVP) {
  GLuint aP, aUV, uC, uMVP, uTex;
  GLuint program = TextureProgram(&aP, &aUV, &uC, &uMVP, &uTex);
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(uTex, 0);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  
  glEnableVertexAttribArray(aUV);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawElements(GL_TRIANGLE_STRIP, icount, GL_UNSIGNED_SHORT, idx);
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  if (Error())
    return false;
  
  return true;
}


bool GlesUtil::DrawDropshadowStrip2fi(unsigned short icount, const float *P,
                                      const float *UV, const unsigned short *idx,
                                      float r, float g, float b, float a,
                                      const float *MVP) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLuint aP, aUV, uC, uMVP;
  GLuint program = GlesUtil::DropshadowFrameProgram(&aP, &aUV, &uC, &uMVP);
  if (!program)
    return false;
  glUseProgram(program);
  glUniform4f(uC, r, g, b, a);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);
  glEnableVertexAttribArray(aUV);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, UV);
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, P);
  glDrawElements(GL_TRIANGLE_STRIP, icount, GL_UNSIGNED_SHORT, idx);
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  glDisable(GL_BLEND);
  if (Error())
    return false;

  return true;
}


//
// Texture
//

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
  bool isPow2 = w == 1;
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


GLuint GlesUtil::CreateShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  if (!shader)
    return 0;
  
  glShaderSource(shader, 1, &source, NULL);
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


//
// Programs
//

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
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "Constant");
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


GLuint GlesUtil::VertexColorProgram(GLuint *aP, GLuint *aC, GLuint *uMVP) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAC = 0, gUMVP = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec4 aC;\n"
    "uniform mat4 uMVP;\n"
    "varying vec4 vC;\n"
    "void main() {\n"
    "  vC = aC;\n"
    "  gl_Position = uMVP * aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "varying vec4 vC;\n"
    "void main() {\n"
    "  gl_FragColor = vC;\n"
    "}\n";
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "Constant");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gAC = glGetAttribLocation(gProgram, "aC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *aC = gAC;
  *uMVP = gUMVP;
  return gProgram;
}


GLuint GlesUtil::DropshadowFrameProgram(GLuint *aP, GLuint *aUV, GLuint *uC,
                                        GLuint *uMVP) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAUV = 0, gUC = 0, gUMVP = 0;
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
    "uniform vec4 uC;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  float v = vUV.y * vUV.y * (3.0 - 2.0 * vUV.y);"
    "  gl_FragColor = v * uC;\n"
    "}\n";
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "Dropshadow");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gAUV = glGetAttribLocation(gProgram, "aUV");
    gUC = glGetUniformLocation(gProgram, "uC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *aUV = gAUV;
  *uC = gUC;
  *uMVP = gUMVP;
  return gProgram;
}



GLuint GlesUtil::TextureProgram(GLuint *aP, GLuint *aUV, GLuint *uC,
                                GLuint *uMVP, GLuint *uTex) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gAUV = 0, gUC = 0, gUMVP = 0, gUCTex = 0;
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
    "uniform vec4 uC;\n"
    "void main() {\n"
    "  gl_FragColor = uC * texture2D(uCTex, vUV);\n"
    "}\n";
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "Texture");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAUV = glGetAttribLocation(gProgram, "aUV");
    gAP = glGetAttribLocation(gProgram, "aP");
    gUC = glGetUniformLocation(gProgram, "uC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    gUCTex = glGetUniformLocation(gProgram, "uCTex");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *aUV = gAUV;
  *uC = gUC;
  *uMVP = gUMVP;
  *uTex = gUCTex;
  return gProgram;
}


GLuint GlesUtil::ScreenTextureProgram(GLuint *aP, GLuint *uC, GLuint *uMVP,
                                      GLuint *uTex) {
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP = 0, gUC = 0, gUMVP = 0, gUCTex = 0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "uniform mat4 uMVP;\n"
    "void main() {\n"
    "  gl_Position = uMVP * aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "uniform sampler2D uCTex;\n"
    "uniform vec4 uC;\n"
    "void main() {\n"
    "  gl_FragColor = uC * texture2D(uCTex, gl_FragCoord.xy);\n"
    "}\n";
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "TwoTexture");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gUC = glGetUniformLocation(gProgram, "uC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    gUCTex = glGetUniformLocation(gProgram, "uCTex");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *uC = gUC;
  *uMVP = gUMVP;
  *uTex = gUCTex;
  return gProgram;
}


GLuint GlesUtil::TwoTextureProgram(GLuint *aP, GLuint *aUV, GLuint *uC,
                                   GLuint *uMVP, GLuint *uUVTex,GLuint *uSTTex){
  static bool gInitialized = false;             // WARNING: Static variables!
  static GLuint gProgram = 0;
  static GLuint gAP=0, gAUV=0, gUC=0, gUMVP=0, gUUVTex=0, gUSTTex=0;
  if (!gInitialized) {
    gInitialized = true;
    
    // Note: the program below is leaked and should be destroyed in _atexit()
    static const char *vpCode =
    "attribute vec4 aP;\n"
    "attribute vec4 aUV;\n"
    "uniform mat4 uMVP;\n"
    "varying vec4 vUV;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  gl_Position = uMVP * aP;\n"
    "}\n";
    static const char *fpCode =
    "precision mediump float;\n"
    "uniform sampler2D uUVTex;\n"
    "uniform sampler2D uSTTex;\n"
    "uniform vec4 uC;\n"
    "varying vec4 vUV;\n"
    "void main() {\n"
    "  vec4 cUV = texture2D(uUVTex, vUV.xy);\n"
    "  vec4 cST = texture2D(uSTTex, vUV.zw);\n"
    "  vec4 C = cUV + (1.0 - cUV.a) * cST;\n"
    "  gl_FragColor = uC * C;\n"
    "}\n";
    GLuint vp = CreateShader(GL_VERTEX_SHADER, vpCode);
    if (!vp)
      return false;
    GLuint fp = CreateShader(GL_FRAGMENT_SHADER, fpCode);
    if (!fp)
      return false;
    gProgram = CreateProgram(vp, fp, "Texture");
    if (!gProgram)
      return false;
    glDeleteShader(vp);
    glDeleteShader(fp);
    glUseProgram(gProgram);
    gAP = glGetAttribLocation(gProgram, "aP");
    gAUV = glGetAttribLocation(gProgram, "aUV");
    gUC = glGetUniformLocation(gProgram, "uC");
    gUMVP = glGetUniformLocation(gProgram, "uMVP");
    gUUVTex = glGetUniformLocation(gProgram, "uUVTex");
    gUSTTex = glGetUniformLocation(gProgram, "uSTTex");
    if (Error())
      return false;
  }
  
  *aP = gAP;
  *aUV = gAUV;
  *uC = gUC;
  *uMVP = gUMVP;
  *uUVTex = gUUVTex;
  *uSTTex = gUSTTex;
  
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


//
// Text
//

#include <set>

static std::set<int> sDebugFontPtSet;
static char sDebugFontName[1024] = { 0 };
static const GlesUtil::FontSet *sDebugFontSet = NULL;


void GlesUtil::DebugFontSizes(const GlesUtil::FontSet &fontSet,
                              const char *name) {
  sDebugFontSet = &fontSet;
  strcpy(sDebugFontName, name);
  sDebugFontPtSet.clear();
}


const GlesUtil::Font &GlesUtil::FontSet::Font(float pts) const {
  if (this == sDebugFontSet) {
    if (sDebugFontPtSet.find(int(pts)) == sDebugFontPtSet.end()) {
      printf("Loading \"%s\" size %d\n", sDebugFontName, int(pts));
      sDebugFontPtSet.insert(int(pts));
    }
  }
  
  for (size_t i = 0; i < fontCount; ++i) {            // Search font vec
    if (fontVec[i].charDimPt[0] >= pts)
      return fontVec[i];                              // Closest fit
  }
  return fontVec[fontCount - 1];                      // Return largest
}


unsigned int GlesUtil::TextWidth(const char *text, const Font *font,
                                 bool isKerned) {
  if (!text)
    return 0;
  
  const size_t len = strlen(text);
  if (len == 0)
    return 0;
  
  size_t w = 0;
  size_t maxW = 0;
  for (size_t i = 0; i < len; ++i) {
    const int k = (unsigned char)text[i];             // Glyph index
    if (k == '\n') {
      maxW = std::max(w, maxW);                       // Longest line
      w = 0;                                          // Restart
    }
    w += isKerned ? font->charWidthPt[k] : font->charDimPt[0];
  }
  maxW = std::max(w, maxW);
  
  return maxW;
}


struct V2f { float x, y; };                           // 2D Position & UV

bool GlesUtil::DrawText(const char *text, float x, float y, const Font *font,
                        float ptW, float ptH, float r, float g, float b,float a,
                        const float *MVP, float charPadPt,
                        int firstChar, int lastChar) {
  if (text == NULL)
    return false;
  if (font == NULL)
    return false;
  if (ptW == 0 || ptH == 0)
    return false;
  
  const size_t len = strlen(text);
  if (len == 0)
    return true;                                      // Nothing to draw, ok
  
  // Create attribute arrays for the text indexed tristrip
  lastChar = lastChar < 0 ? std::numeric_limits<int>::max() : lastChar;
  if (lastChar < firstChar)
    return false;
  const size_t n = len < lastChar-firstChar+1? len : lastChar-firstChar+1;
  lastChar = firstChar + n;                           // Loop terminator
  std::vector<V2f> P(4*n);
  std::vector<V2f> UV(4*n);
  const size_t idxCount = (4 + 3) * n - 3;            // 4/quad + 3 degen - last
  std::vector<unsigned short> idx(idxCount);
  const int colCount = floor(1 / font->charDimUV[0]);
  
  // Build a tristrip of characters with point coordinates and UVs.
  // Characters are drawn as quads with UV coordinates that map into
  // the font texture. Character quads are fixed sized (charDimPt), and
  // can overlap due to kerning via charWidthPt. Quads are rendered as
  // a tristrip with degenerate tris connecting each quad.
  float curX = 0;
  for (size_t i = 0; i < lastChar; ++i) {
    const int x0 = curX;                              // Integer pixel coords
    const int y0 = 0;
    const int x1 = curX + font->charDimPt[0];
    const int y1 = font->charDimPt[1];
    const int k = (unsigned char)text[i];             // Glyph index
    curX += font->charWidthPt[k] + charPadPt;         // Kerning offset in X
    if (i < firstChar)
      continue;
    const size_t j = (i - firstChar) * 4;             // Vertex index
    P[j+0].x = x + x0 * ptW;                          // Vertex positions
    P[j+0].y = y + y0 * ptH;
    P[j+1].x = x + x0 * ptW;
    P[j+1].y = y + y1 * ptH;
    P[j+2].x = x + x1 * ptW;
    P[j+2].y = y + y0 * ptH;
    P[j+3].x = x + x1 * ptW;
    P[j+3].y = y + y1 * ptH;
    const int row = k / colCount;                     // Glyph location
    const int col = k % colCount;
    assert(row < 16 && col < 16 && row >= 0 && col >= 0);
    const float u0 = col * font->charDimUV[0];        // Cell texture coords
    const float v0 = row * font->charDimUV[1];
    const float u1 = (col + 1) * font->charDimUV[0];
    const float v1 = (row + 1) * font->charDimUV[1];
    UV[j+0].x = u0; UV[j+0].y = v1;                   // Vertex texture coords
    UV[j+1].x = u0; UV[j+1].y = v0;                   // Ick! Flipped texture.
    UV[j+2].x = u1; UV[j+2].y = v1;
    UV[j+3].x = u1; UV[j+3].y = v0;
    const size_t q = (i - firstChar) * 7;             // First idx
    idx[q+0] = j;
    idx[q+1] = j+1;
    idx[q+2] = j+2;
    idx[q+3] = j+3;
    if (i < lastChar - 1) {                           // Skip last degen
      idx[q+4] = j+3;                                 // Degenerate tri
      idx[q+5] = j+4;
      idx[q+6] = j+4;
    }
  }
  
  // Set up the texture shader
  GLuint program, aP, aUV, uC, uMVP, uTex;
  program = TextureProgram(&aP, &aUV, &uC, &uMVP, &uTex);
  glUseProgram(program);                              // Setup program
  glUniform4f(uC, r, g, b, a);
  glEnableVertexAttribArray(aUV);                     // Vertex arrays
  glEnableVertexAttribArray(aP);
  glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 0, &UV[0].x);
  glVertexAttribPointer(aP, 2, GL_FLOAT, GL_FALSE, 0, &P[0].x);
  glActiveTexture(GL_TEXTURE0);                       // Setup texture
  glBindTexture(GL_TEXTURE_2D, font->tex);
  glUniform1i(uTex, 0);
  static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  if (!MVP)
    MVP = &I[0];  
  glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP);

  // Draw the tristrip with all the character rectangles
  glDrawElements(GL_TRIANGLE_STRIP, idxCount, GL_UNSIGNED_SHORT, &idx[0]);
  
  glDisableVertexAttribArray(aUV);
  glDisableVertexAttribArray(aP);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  if (Error())
    return false;
  
  return true;
}


bool GlesUtil::DrawText(const char *text, float x, float y,
                        const Font *font, float ptW, float ptH,
                        const float *MVP, float charPadPt) {
  return DrawText(text, x, y, font, ptW, ptH, 1, 1, 1, 1, MVP, charPadPt);
}


static bool DrawJustified(const char *text, float x0, float x1,
                          float y, float textW, GlesUtil::Align align,
                          const GlesUtil::Font *font, float ptW, float ptH,
                          float r, float g, float b, float a, const float *MVP,
                          int fc, int lc) {
  const float w = x1 - x0;
  float x, pad = 0;
  switch (align) {
    case GlesUtil::LeftJustify:   x = 0;                break;
    case GlesUtil::RightJustify:  x = w - textW;        break;
    case GlesUtil::CenterJustify: x = (w - textW) / 2;  break;
    case GlesUtil::FullJustify:
      x = 0;
      pad = std::max(textW - w, 0.0f);
      if (pad > 0) {
        int len = strlen(text);
        pad /= len;
      }
      break;
  }
  if (!DrawText(text, x + x0, y, font, ptW, ptH, r,g,b,a, MVP, pad, fc, lc))
    return false;
  return true;
}


bool GlesUtil::DrawParagraph(const char *text, float x0, float y0,
                             float x1, float y1, Align align, const Font *font,
                             float ptW, float ptH, float r, float g, float b,
                             float a, const float *MVP,
                             int firstChar, int lastChar) {
  const float wrapW = x1 - x0;
  const float eps = ptW / 4;          // Due to width accumulation
  if (wrapW <= 0)
    return false;
  float w = 0;
  float y = y1 - ptH * font->charDimPt[1];
  std::vector<char> str;
  int len = strlen(text);
  size_t lastSep = 0;
  float lastSepW = 0;
  for (size_t i = 0; y + ptH * font->charDimPt[1] > y0 && i <= len; ++i) {
    const char &c(text[i]);
    if (isspace(c)) {
      lastSep = str.size();
      lastSepW = w;
    }
    if (c == '\0' || c == '\n') {
      str.push_back('\0');
    } else {
      str.push_back(c);
      w += ptW * font->charWidthPt[(unsigned char)c];
      if (w <= wrapW + eps)
        continue;
      if (w - lastSepW > wrapW) {
        // FIXME: What happens if a single word is longer than the line?
        continue;
      }
      str[lastSep] = '\0';
      w = lastSepW;
      i -= str.size() - lastSep;      // Move back and re-do skipped characters
    }
    
    // Draw the current line. We either hit a return, the end of the string,
    // or went past the width of the rectangle and moved back to last separator
    if (!DrawJustified(&str[0], x0,x1, y, w, align, font, ptW,ptH, r,g,b,a,
                       MVP, firstChar, lastChar))
      return false;
    
    w = 0;
    y -= ptH * font->charDimPt[1];
    str.resize(0);
  }
  return true;
}


bool GlesUtil::DrawParagraph(const char *text, float x0, float y0,
                             float x1, float y1, Align align, const Font *font,
                             float ptW, float ptH,  const float *MVP) {
  return DrawParagraph(text, x0, y0, x1, y1, align, font, ptW,ptH, 1,1,1,1,MVP);
}


void GlesUtil::RoundedRectSize2fi(int segments, unsigned short *vertexCount,
                                  unsigned short *idxCount) {
  *vertexCount = 4 * (segments + 1 /*inset corner*/);
  *idxCount = 3*5 /*rects*/ + 4 /*fans*/ * 4 /*pts/tri*/ * (segments - 1);
}


//
// Tristrip Shapes
//

void GlesUtil::BuildRoundedRect2fi(float x0, float y0, float x1, float y1,
                                   float u0, float v0, float u1, float v1,
                                   float radiusX, float radiusY, int segments,
                                   float *P, float *UV, unsigned short *idx) {
  // Compute the inset corners and put them into the vertex array
  P[0*2+0] = x0 + radiusX;  P[0*2+1] = y0 + radiusY;
  P[1*2+0] = x0 + radiusX;  P[1*2+1] = y1 - radiusY;
  P[2*2+0] = x1 - radiusX;  P[2*2+1] = y0 + radiusY;
  P[3*2+0] = x1 - radiusX;  P[3*2+1] = y1 - radiusY;
  
  const float du = (u1 - u0) * radiusX / (x1 - x0);
  const float dv = (v1 - v0) * radiusY / (y1 - y0);
  UV[0*2+0] = u0 + du; UV[0*2+1] = v0 + dv;
  UV[1*2+0] = u0 + du; UV[1*2+1] = v1 - dv;
  UV[2*2+0] = u1 - du; UV[2*2+1] = v0 + dv;
  UV[3*2+0] = u1 - du; UV[3*2+1] = v1 - dv;
  
  // Compute the rounded corner vertices by computing a quarter circle
  // and mirroring and translating it to each corner
  const float k = 1.0 / (segments - 1);
  for (int i = 0; i < segments; ++i) {
    const float theta = i * k * M_PI_2;
    const float s = sinf(theta);
    const float c = cosf(theta);
    const float x = radiusX * s;                    // Up-right quadrant
    const float y = radiusY * c;
    P[(4 + (0*segments + i))*2 + 0] = P[0*2+0] - x; // x0, y0
    P[(4 + (0*segments + i))*2 + 1] = P[0*2+1] - y;
    P[(4 + (1*segments + i))*2 + 0] = P[1*2+0] - x; // x0, y1
    P[(4 + (1*segments + i))*2 + 1] = P[1*2+1] + y;
    P[(4 + (2*segments + i))*2 + 0] = P[2*2+0] + x; // x1, y0
    P[(4 + (2*segments + i))*2 + 1] = P[2*2+1] - y;
    P[(4 + (3*segments + i))*2 + 0] = P[3*2+0] + x; // x1, y1
    P[(4 + (3*segments + i))*2 + 1] = P[3*2+1] + y;
    
    const float us = du * s;
    const float vc = dv * c;
    UV[(4 + (0*segments + i))*2 + 0] = UV[0*2+0] - us;
    UV[(4 + (0*segments + i))*2 + 1] = UV[0*2+1] - vc;
    UV[(4 + (1*segments + i))*2 + 0] = UV[1*2+0] - us;
    UV[(4 + (1*segments + i))*2 + 1] = UV[1*2+1] + vc;
    UV[(4 + (2*segments + i))*2 + 0] = UV[2*2+0] + us;
    UV[(4 + (2*segments + i))*2 + 1] = UV[2*2+1] - vc;
    UV[(4 + (3*segments + i))*2 + 0] = UV[3*2+0] + us;
    UV[(4 + (3*segments + i))*2 + 1] = UV[3*2+1] + vc;
  }
  
  int j = 0;
  idx[j++] = 4 + 0 * segments;                      // top -> bot rect
  idx[j++] = 4 + 1 * segments;
  idx[j++] = 4 + 2 * segments;
  idx[j++] = 4 + 3 * segments;
  idx[j++] = idx[3];                                // degen connector
  
  for (int i = 0; i < segments - 1; ++i) {          // x0, y0
    idx[j++] = 0;
    idx[j++] = 4 + i;
    idx[j++] = 4 + i + 1;
    idx[j++] = 4 + i + 1;                           // degen connector
  }
  
  idx[j++] = 0;                                     // left rect filler
  idx[j++] = 4 + segments - 1;
  idx[j++] = 1;
  idx[j++] = 4 + 2 * segments - 1;
  idx[j++] = 4 + 2 * segments - 1;
  
  for (int i = segments - 1; i > 0; --i) {         // x0, y1
    idx[j++] = 1;
    idx[j++] = 4 + segments + i;
    idx[j++] = 4 + segments + i - 1;
    idx[j++] = 4 + segments + i - 1;                // degen connector
  }
  
  
  for (int i = 0; i < segments - 1; ++i) {          // x1, y0
    idx[j++] = 2;
    idx[j++] = 4 + 2 * segments + i + 1;
    idx[j++] = 4 + 2 * segments + i;
    idx[j++] = 4 + 2 * segments + i;                // degen connector
  }
  
  idx[j++] = 2;                                     // right rect filler
  idx[j++] = 3;
  idx[j++] = 4 + 3 * segments - 1;
  idx[j++] = 4 + 4 * segments - 1;
  idx[j++] = 4 + 4 * segments - 1;
  
  for (int i = 0; i < segments - 1; ++i) {          // x1, y1
    idx[j++] = 3;
    idx[j++] = 4 + 3 * segments + i;
    idx[j++] = 4 + 3 * segments + i + 1;
    idx[j++] = 4 + 3 * segments + i + 1;            // degen connector
  }
  
#if DEBUG
  unsigned short nv, ni;
  RoundedRectSize2fi(segments, &nv, &ni);
  assert(ni == j);
  assert(4 + 3 * segments + segments - 1 == nv - 1);
#endif
}


void GlesUtil::RoundedFrameSize2fi(int segments, unsigned short *vertexCount,
                                   unsigned short *idxCount) {
  *vertexCount = 4 * (segments + 1 /*inset corner*/);
  *idxCount = 4*5 /*rects*/ + 4 /*fans*/ * 4 /*pts/tri*/ * (segments - 1) + 7;
}


void GlesUtil::BuildRoundedFrame2fi(float x0, float y0, float x1, float y1,
                                    float radiusX, float radiusY, int segments,
                                    float *P, float *UV, unsigned short *idx) {
  // Compute the inset corners and put them into the vertex array
  P[0*2+0] = x0 + radiusX;  P[0*2+1] = y0 + radiusY;
  P[1*2+0] = x0 + radiusX;  P[1*2+1] = y1 - radiusY;
  P[2*2+0] = x1 - radiusX;  P[2*2+1] = y0 + radiusY;
  P[3*2+0] = x1 - radiusX;  P[3*2+1] = y1 - radiusY;
  
  UV[0*2+0] = 0;    UV[0*2+1] = 1;
  UV[1*2+0] = 0.25; UV[1*2+1] = 1;
  UV[2*2+0] = 1;    UV[2*2+1] = 1;
  UV[3*2+0] = 0.75; UV[3*2+1] = 1;
  
  // Compute the rounded corner vertices by computing a quarter circle
  // and mirroring and translating it to each corner
  const float k = 1.0 / (segments - 1);
  for (int i = 0; i < segments; ++i) {
    const float theta = i * k * M_PI_2;
    const float s = sinf(theta);
    const float c = cosf(theta);
    const float x = radiusX * s;                    // Up-right quadrant
    const float y = radiusY * c;
    P[(4 + (0*segments + i))*2 + 0] = P[0*2+0] - x; // x0, y0
    P[(4 + (0*segments + i))*2 + 1] = P[0*2+1] - y;
    P[(4 + (1*segments + i))*2 + 0] = P[1*2+0] - x; // x0, y1
    P[(4 + (1*segments + i))*2 + 1] = P[1*2+1] + y;
    P[(4 + (2*segments + i))*2 + 0] = P[2*2+0] + x; // x1, y0
    P[(4 + (2*segments + i))*2 + 1] = P[2*2+1] - y;
    P[(4 + (3*segments + i))*2 + 0] = P[3*2+0] + x; // x1, y1
    P[(4 + (3*segments + i))*2 + 1] = P[3*2+1] + y;
    
    UV[(4 + (0*segments + i))*2 + 0] = 0;
    UV[(4 + (0*segments + i))*2 + 1] = 0;
    UV[(4 + (1*segments + i))*2 + 0] = 0.25;
    UV[(4 + (1*segments + i))*2 + 1] = 0;
    UV[(4 + (2*segments + i))*2 + 0] = 0.75;
    UV[(4 + (2*segments + i))*2 + 1] = 0;
    UV[(4 + (3*segments + i))*2 + 0] = 1;
    UV[(4 + (3*segments + i))*2 + 1] = 0;
  }
  
  int j = 0;

  idx[j++] = 0;                                     // left
  idx[j++] = 4 + segments - 1;
  idx[j++] = 1;
  idx[j++] = 4 + 2 * segments - 1;
  idx[j++] = 4 + 2 * segments - 1;

  idx[j++] = 1;                                     // top
  idx[j++] = 1;                                     // top
  idx[j++] = 4 + segments;
  idx[j++] = 3;
  idx[j++] = 4 + 3 * segments;
  idx[j++] = 4 + 3 * segments;

  idx[j++] = 2;                                     // right
  idx[j++] = 2;                                     // right
  idx[j++] = 3;
  idx[j++] = 4 + 3 * segments - 1;
  idx[j++] = 4 + 4 * segments - 1;
  idx[j++] = 4 + 4 * segments - 1;

  idx[j++] = 4;                                     // bottom
  idx[j++] = 4;                                     // bottom
  idx[j++] = 0;
  idx[j++] = 4 + 2 * segments;
  idx[j++] = 2;
  idx[j++] = 2;

  idx[j++] = 0;
  for (int i = 0; i < segments - 1; ++i) {          // x0, y0
    idx[j++] = 0;
    idx[j++] = 4 + i;
    idx[j++] = 4 + i + 1;
    idx[j++] = 4 + i + 1;                           // degen connector
  }
  
  idx[j++] = 1;
  for (int i = segments - 1; i > 0; --i) {          // x0, y1
    idx[j++] = 1;
    idx[j++] = 4 + segments + i;
    idx[j++] = 4 + segments + i - 1;
    idx[j++] = 4 + segments + i - 1;                // degen connector
  }
  
  idx[j++] = 2;
  for (int i = segments - 1; i > 0; --i) {          // x1, y0
    idx[j++] = 2;
    idx[j++] = 4 + 2 * segments + i;
    idx[j++] = 4 + 2 * segments + i - 1;
    idx[j++] = 4 + 2 * segments + i - 1;            // degen connector
  }
  
  idx[j++] = 3;
  for (int i = 0; i < segments - 1; ++i) {          // x1, y1
    idx[j++] = 3;
    idx[j++] = 4 + 3 * segments + i;
    idx[j++] = 4 + 3 * segments + i + 1;
    idx[j++] = 4 + 3 * segments + i + 1;            // degen connector
  }
}

