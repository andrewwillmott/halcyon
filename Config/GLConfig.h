//
//  File:       GLConfig.h
//
//  Function:   Basic setup for GL/GLES, to minimise cross-platform differences
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef GL_CONFIG_H
#define GL_CONFIG_H

// TODO: move this to GLPlatform.h

// #define CL_OSX_GLES

#if defined(CL_IOS)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
#elif defined(CL_OSX_GLES)
    #define GL_GLEXT_PROTOTYPES
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#elif defined(CL_OSX)
    #include <OpenGL/OpenGL.h>

    // OpenGL 3.2 is only supported on MacOS X Lion and later
    // CGL_VERSION_1_3 is defined as 1 on MacOS X Lion and later
    #if defined(CL_USE_GL3) && !CGL_VERSION_1_3
        #undef CL_USE_GL3
    #endif

    #ifdef CL_USE_GL3
        #include <OpenGL/gl3.h>
        #include <OpenGL/gl3ext.h>
    #else
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
    #endif

#else
    #error "Unsupported platform"
    // TODO: Android
#endif


//The name of the VertexArrayObject are slightly different in
// OpenGLES, OpenGL Core Profile, and OpenGL Legacy
// The arguments are exactly the same across these APIs however
#if CL_IOS
    #define CL_GLES

    #define glBindVertexArray glBindVertexArrayOES
    #define glGenVertexArrays glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES

    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
    #define glMapBufferRange glMapBufferRangeEXT
    #define glFlushMappedBufferRange glFlushMappedBufferRangeEXT
    #define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE

    #define glUseShaderProgram glUseShaderProgramEXT
    #define glActiveProgram glActiveProgramEXT

    #define glDiscardFramebuffer glDiscardFramebufferEXT

    #define GL_WRITE_ONLY GL_WRITE_ONLY_OES
    #define GL_BUFFER_ACCESS GL_BUFFER_ACCESS_OES
    #define GL_BUFFER_MAPPED GL_BUFFER_MAPPED_OES
    #define GL_BUFFER_MAP_POINTER GL_BUFFER_MAP_POINTER_OES

    #define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_APPLE
    #define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_APPLE

    #define GL_RGBA8 GL_RGBA8_OES
    #define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
    #define GL_HALF_FLOAT GL_HALF_FLOAT_OES

    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES
    #define GL_DEPTH24_STENCIL8     GL_DEPTH24_STENCIL8_OES

    #define GL_ALPHA8            GL_ALPHA8_EXT
    #define GL_LUMINANCE8        GL_LUMINANCE8_EXT
    #define GL_LUMINANCE8_ALPHA8 GL_LUMINANCE8_ALPHA8_EXT

    #define GL_MIN GL_MIN_EXT
    #define GL_MAX GL_MAX_EXT

#elif defined(CL_OSX_GLES)
    #define CL_GLES

    #define glBindVertexArray glBindVertexArrayOES
    #define glGenVertexArrays glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES

    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
    #define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleOES

    #define glUseShaderProgram glUseShaderProgramEXT
    #define glActiveProgram glActiveProgramEXT

    #define glDiscardFramebuffer glDiscardFramebufferEXT

    // the PowerVR SDK defines these
    #define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_APPLE
    #define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_APPLE

    #define GL_RGBA8 GL_RGBA8_OES
    #define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
    #define GL_HALF_FLOAT GL_HALF_FLOAT_OES

    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES
    #define GL_DEPTH24_STENCIL8     GL_DEPTH24_STENCIL8_OES

    #define GL_ALPHA8            GL_ALPHA8_EXT
    #define GL_LUMINANCE8        GL_LUMINANCE8_EXT
    #define GL_LUMINANCE8_ALPHA8 GL_LUMINANCE8_ALPHA8_EXT

    #define GL_MIN GL_MIN_EXT
    #define GL_MAX GL_MAX_EXT

#else
    #ifndef CL_USE_GL3
        #define glBindVertexArray glBindVertexArrayAPPLE
        #define glGenVertexArrays glGenVertexArraysAPPLE
        #define glGenerateMipmap glGenerateMipmapEXT
        #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
    #endif
#endif

#endif
