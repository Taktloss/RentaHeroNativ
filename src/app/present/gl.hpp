// SPDX-License-Identifier: GPL-3.0-or-later
// OpenGL-Funktionen über SDL_GL_GetProcAddress (keine Bindung an libGL/opengl32 beim Linken)
#pragma once

#include <SDL3/SDL_opengl.h>

namespace rah::gl {

// GL 1.0/1.1 (ohne PFN-Typen in glext.h)
typedef const GLubyte*(APIENTRY* PFNGLGETSTRINGPROC)(GLenum);
typedef void(APIENTRY* PFNGLGETINTEGERVPROC)(GLenum, GLint*);
typedef void(APIENTRY* PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void(APIENTRY* PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(APIENTRY* PFNGLCLEARPROC)(GLbitfield);
typedef void(APIENTRY* PFNGLENABLEPROC)(GLenum);
typedef void(APIENTRY* PFNGLDISABLEPROC)(GLenum);
typedef void(APIENTRY* PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef void(APIENTRY* PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void(APIENTRY* PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void(APIENTRY* PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void(APIENTRY* PFNGLTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void(APIENTRY* PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void(APIENTRY* PFNGLPIXELSTOREIPROC)(GLenum, GLint);
typedef void(APIENTRY* PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void(APIENTRY* PFNGLREADPIXELSPROC)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);

#define RAH_GL_FUNCS(X)                                                   \
    X(PFNGLGETSTRINGPROC, GetString)                                      \
    X(PFNGLGETINTEGERVPROC, GetIntegerv)                                  \
    X(PFNGLVIEWPORTPROC, Viewport)                                        \
    X(PFNGLCLEARCOLORPROC, ClearColor)                                    \
    X(PFNGLCLEARPROC, Clear)                                              \
    X(PFNGLENABLEPROC, Enable)                                            \
    X(PFNGLDISABLEPROC, Disable)                                          \
    X(PFNGLGENTEXTURESPROC, GenTextures)                                  \
    X(PFNGLDELETETEXTURESPROC, DeleteTextures)                            \
    X(PFNGLBINDTEXTUREPROC, BindTexture)                                  \
    X(PFNGLTEXIMAGE2DPROC, TexImage2D)                                    \
    X(PFNGLTEXSUBIMAGE2DPROC, TexSubImage2D)                              \
    X(PFNGLTEXPARAMETERIPROC, TexParameteri)                              \
    X(PFNGLPIXELSTOREIPROC, PixelStorei)                                  \
    X(PFNGLACTIVETEXTUREPROC, ActiveTexture)                              \
    X(PFNGLCREATESHADERPROC, CreateShader)                                \
    X(PFNGLSHADERSOURCEPROC, ShaderSource)                                \
    X(PFNGLCOMPILESHADERPROC, CompileShader)                              \
    X(PFNGLGETSHADERIVPROC, GetShaderiv)                                  \
    X(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)                        \
    X(PFNGLDELETESHADERPROC, DeleteShader)                                \
    X(PFNGLCREATEPROGRAMPROC, CreateProgram)                              \
    X(PFNGLATTACHSHADERPROC, AttachShader)                                \
    X(PFNGLBINDATTRIBLOCATIONPROC, BindAttribLocation)                    \
    X(PFNGLLINKPROGRAMPROC, LinkProgram)                                  \
    X(PFNGLGETPROGRAMIVPROC, GetProgramiv)                                \
    X(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog)                      \
    X(PFNGLDELETEPROGRAMPROC, DeleteProgram)                              \
    X(PFNGLUSEPROGRAMPROC, UseProgram)                                    \
    X(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation)                    \
    X(PFNGLUNIFORM1IPROC, Uniform1i)                                      \
    X(PFNGLUNIFORM1FPROC, Uniform1f)                                      \
    X(PFNGLUNIFORM2FPROC, Uniform2f)                                      \
    X(PFNGLUNIFORMMATRIX4FVPROC, UniformMatrix4fv)                        \
    X(PFNGLGENBUFFERSPROC, GenBuffers)                                    \
    X(PFNGLBINDBUFFERPROC, BindBuffer)                                    \
    X(PFNGLBUFFERDATAPROC, BufferData)                                    \
    X(PFNGLDELETEBUFFERSPROC, DeleteBuffers)                              \
    X(PFNGLGENVERTEXARRAYSPROC, GenVertexArrays)                          \
    X(PFNGLBINDVERTEXARRAYPROC, BindVertexArray)                          \
    X(PFNGLDELETEVERTEXARRAYSPROC, DeleteVertexArrays)                    \
    X(PFNGLVERTEXATTRIBPOINTERPROC, VertexAttribPointer)                  \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, EnableVertexAttribArray)          \
    X(PFNGLGENFRAMEBUFFERSPROC, GenFramebuffers)                          \
    X(PFNGLBINDFRAMEBUFFERPROC, BindFramebuffer)                          \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC, FramebufferTexture2D)                \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, CheckFramebufferStatus)            \
    X(PFNGLDELETEFRAMEBUFFERSPROC, DeleteFramebuffers)                    \
    X(PFNGLDRAWARRAYSPROC, DrawArrays)                                    \
    X(PFNGLREADPIXELSPROC, ReadPixels)                                    \
    X(PFNGLGENSAMPLERSPROC, GenSamplers)                                  \
    X(PFNGLBINDSAMPLERPROC, BindSampler)                                  \
    X(PFNGLSAMPLERPARAMETERIPROC, SamplerParameteri)                      \
    X(PFNGLSAMPLERPARAMETERFVPROC, SamplerParameterfv)                    \
    X(PFNGLDELETESAMPLERSPROC, DeleteSamplers)

struct Api {
#define RAH_GL_DECL(type, name) type name = nullptr;
    RAH_GL_FUNCS(RAH_GL_DECL)
#undef RAH_GL_DECL
    // Aus dem aktuellen Kontext laden; false, wenn eine Funktion fehlt (*missing = Name)
    bool load(const char** missing);
};

}  // namespace rah::gl
