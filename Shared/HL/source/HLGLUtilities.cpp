//
//  File:       HLGLUtilities.cpp
//
//  Function:   GLES utility functions
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#include <HLGLUtilities.h>

#include <HLReadPVR.h>

#include <CLSampleUtilities.h>

#include <CLFileSpec.h>
#include <CLImage.h>
#include <CLLog.h>
#include <CLString.h>
#include <CLValue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include <CLDefs.h>

//#include "stb_image.h"

using namespace nCL;
using namespace nHL;

namespace
{
    const char * GetGLErrorString(GLenum error)
    {
        const char *str;

        switch( error )
        {
            case GL_NO_ERROR:
                str = "GL_NO_ERROR";
                break;
            case GL_INVALID_ENUM:
                str = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                str = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                str = "GL_INVALID_OPERATION";
                break;		
    #if defined __gl_h_ || defined __gl3_h_
            case GL_OUT_OF_MEMORY:
                str = "GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                str = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
    #endif
    #if defined __gl_h_
            case GL_STACK_OVERFLOW:
                str = "GL_STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                str = "GL_STACK_UNDERFLOW";
                break;
            case GL_TABLE_TOO_LARGE:
                str = "GL_TABLE_TOO_LARGE";
                break;
    #endif
            default:
                str = "(ERROR: Unknown Error Enum)";
                break;
        }
        
        return str;
    }
}

void nHL::GetGLError()
{
    GLenum err = glGetError();
    
    while (err != GL_NO_ERROR)
    {
        const char* errorString = GetGLErrorString(err);

        CL_LOG_E("GL", "GLError %s\n", errorString);
        
        CL_DEBUG_BREAK();
        
        err = glGetError();
    }
}

GLsizei nHL::GetGLTypeSize(GLenum type)
{
    switch (type)
    {
    case GL_BYTE:
        return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:
        return sizeof(GLubyte);
    case GL_SHORT:
        return sizeof(GLshort);
    case GL_UNSIGNED_SHORT:
        return sizeof(GLushort);
    case GL_INT:
        return sizeof(GLint);
    case GL_UNSIGNED_INT:
        return sizeof(GLuint);
    case GL_FLOAT:
        return sizeof(GLfloat);
    }

    return 0;
}

const char* nHL::GetGLTypeName(GLenum type)
{
    switch (type)
    {
    case GL_FLOAT:
        return "float";
    case GL_FLOAT_VEC2:
        return "vec2";
    case GL_FLOAT_VEC3:
        return "vec3";
    case GL_FLOAT_VEC4:
        return "vec4";
    case GL_FLOAT_MAT2:
        return "mat2";
    case GL_FLOAT_MAT3:
        return "mat3";
    case GL_FLOAT_MAT4:
        return "mat4";
#if 0
    case GL_FLOAT_MAT2x3:
        return "mat2x3";
    case GL_FLOAT_MAT2x4:
        return "mat2x4";
    case GL_FLOAT_MAT3x2:
        return "mat3x2";
    case GL_FLOAT_MAT3x4:
        return "mat3x4";
    case GL_FLOAT_MAT4x2:
        return "mat4x2";
    case GL_FLOAT_MAT4x3:
        return "mat4x3";
#endif
    case GL_INT:
        return "int";
    case GL_INT_VEC2:
        return "ivec2";
    case GL_INT_VEC3:
        return "ivec3";
    case GL_INT_VEC4:
        return "ivec4";
#if 0
    case GL_UNSIGNED_INT_VEC:
        return "uvec";
    case GL_UNSIGNED_INT_VEC2:
        return "uvec2";
    case GL_UNSIGNED_INT_VEC3:
        return "uvec3";
    case GL_UNSIGNED_INT_VEC4:
        return "uvec4";
#endif

#ifdef GL_SAMPLER_1D
    case GL_SAMPLER_1D:
        return "sampler1D";
#endif
    case GL_SAMPLER_2D:
        return "sampler2D";
#ifdef GL_SAMPLER_3D
    case GL_SAMPLER_3D:
        return "sampler3D";
#endif
    case GL_SAMPLER_CUBE:
        return "samplerCube";
#ifdef GL_SAMPLER_1D_SHADOW
    case GL_SAMPLER_1D_SHADOW:
        return "sampler1DShadow";
    case GL_SAMPLER_2D_SHADOW:
        return "sampler2DShadow";
#endif
    }

    return "unknown";
}

cGLMeshInfo::cGLMeshInfo() :
    mMesh(0),
    mNumElts(0),
    mEltType(0)
    // mTextures[]
{
    for (int i = 0; i < kMaxTextureKinds; i++)
        mTextures[i] = 0;
}


void nHL::DestroyMesh(GLuint meshName)
{
    // Bind the VA so we can get data from it
    glBindVertexArray(meshName);
    
    // For every possible attribute set in the VA
    GLuint buffers[17];
    int bufferCount = 0;
    
    for (int i = 0; i < 16; i++)
    {
        // Get the VBO set for that attibute
        GLint vb = 0;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vb);
        
        if (vb)
            buffers[bufferCount++] = vb;
    }
    
    // Get any element array VB set in the VA
    GLint ib;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ib);
    
    if (ib)
        buffers[bufferCount++] = ib;
    
    glBindVertexArray(0);

    glDeleteBuffers(bufferCount, buffers);
    glDeleteVertexArrays(1, &meshName);
}



//------------------------------------------------------------------------------
// Shader support
//------------------------------------------------------------------------------

namespace
{
    const int kMaxAttributeNameLength = 20;

    const cEnumInfo kAttributeLocationsEnum[] =
    {
        "inPosition",   kVBPositions,
        "inTexCoord",   kVBTexCoords,
        "inUV",         kVBTexCoords,
        "inColour",     kVBColours,
        "inColor",      kVBColours,
        "inNormal",     kVBNormals,
        0, 0
    };
    // CL_CT_ASSERT(sizeof(kAttributeLocationsEnum) / sizeof(kAttributeLocationsEnum[0]) == kMaxVertexAttributes + 1);

    const cEnumInfo kTextureKindsEnum[] =
    {
        "diffuseMap",       kTextureDiffuseMap,
        "normalMap",        kTextureNormalMap,
        "shadowMap",        kTextureShadowMap,
        "alternateMap",     kTextureAlternateMap,

        "diffuseTexture",      kTextureDiffuseMap,
        "normalMapTexture",    kTextureNormalMap,

        "mapA",             kTextureDiffuseMap,
        "mapB",             kTextureNormalMap,
        "mapC",             kTextureShadowMap,
        "mapD",             kTextureAlternateMap,

        0, 0
    };
}

namespace
{
    size_t LoadFromFile(tStrConst filepathname, tString* str)
    {
        FILE* file = fopen(filepathname, "r");

        if (!file)
            return 0;
        
        // Get the size of the source
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);

        str->resize(fileSize);

        // Read entire file into the string from beginning of the file
        fseek(file, 0, SEEK_SET);
        fread(str->data(), 1, fileSize, file);
        
        fclose(file);
        
        return fileSize;
    }
}

namespace
{
    GLuint CreateShader(GLenum shaderType, const char* version, const char* source, const char* sourceName)
    {
        bool success = true;

        const char* kShaderStrings[] =
        {
            version,
            source
        };

        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, CL_SIZE(kShaderStrings), kShaderStrings, 0);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if (status == 0)
        {
            CL_LOG_E("GL", "Failed to compile %s\n", sourceName);
            success = false;
        }

        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            GLchar log[logLength];
            glGetShaderInfoLog(shader, logLength, &logLength, log);

            if (!success)
                CL_LOG_E("GL", "%s shader log:\n%s", sourceName, log);
            else
                CL_LOG  ("GL", "%s shader log:\n%s", sourceName, log);
        }

        if (success)
            return shader;

        glDeleteShader(shader);
        return 0;
    }

    bool LinkShaders(GLuint shaderProgram, const char* programName)
    {
        bool result = true;
        glLinkProgram(shaderProgram);
        GL_CHECK_DETAIL;

        GLint status;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);

        if (status == 0)
        {
            CL_LOG_E("GL", "Failed to link %s\n", programName);
            result = false;
        }
        
        GLint logLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            GLchar log[logLength];
            glGetProgramInfoLog(shaderProgram, logLength, &logLength, log);

            if (!result)
                CL_LOG_E("GL", "%s link log:\n%s\n", programName, log);
            else
                CL_LOG  ("GL", "%s link log:\n%s\n", programName, log);
        }

        return result;
    }

    bool ValidateShaders(GLuint shaderProgram, const char* programName)
    {
        bool result = true;

        glValidateProgram(shaderProgram);

        GLint status;
        glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &status);

        if (status == 0)
        {
            CL_LOG_E("GL", "Failed to validate %s\n", programName);
            result = false;
        }
        
        GLint logLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            GLchar log[logLength];
            glGetProgramInfoLog(shaderProgram, logLength, &logLength, log);

            if (!result)
                CL_LOG_E("GL", "%s validate log:\n%s\n", programName, log);
            else
                CL_LOG  ("GL", "%s validate log:\n%s\n", programName, log);
        }
        
        return result;
    }

    void BindAttributes(GLuint shaderProgram)
    {
        // Bind attributes
        // get count
        GLint numAttributes;
        glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTES, &numAttributes);
        GL_CHECK_DETAIL;

        for (int i = 0; i < numAttributes; i++)
        {
            GLsizei length;
            GLint size;
            GLenum type;
            GLchar name[kMaxAttributeNameLength];

            glGetActiveAttrib(shaderProgram, i, kMaxAttributeNameLength, &length, &size, &type, name);
            GL_CHECK_DETAIL;

            CL_LOG_D("GL", "  Shader attribute: %s, length %d, type %s (0x%04x)\n", name, size, GetGLTypeName(type), type);

            int attrLocation = ParseEnum(kAttributeLocationsEnum, name);

            if (attrLocation >= 0)
            {
                CL_LOG_D("GL", "    binding to attribute index %d\n", attrLocation);
                glBindAttribLocation(shaderProgram, attrLocation, name);
            }
            else
            {
                CL_LOG_E("GL", "    No location found for %s\n", name);
            }

            GL_CHECK_DETAIL;
        }
    }

    void BindSamplers(GLuint shaderProgram)
    /// Binds samplers uniforms to corresponding texture stage
    {
        GLint numUniforms;
        glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numUniforms);
        GL_CHECK_DETAIL;

        glUseProgram(shaderProgram);
        GL_CHECK_DETAIL;

        int samplersToAssign[numUniforms];
        int numSamplersToAssign = 0;
        uint32_t unitsUsed = 0;

        for (int i = 0; i < numUniforms; i++)
        {
            char uniformName[256];
            GLint size;
            GLenum type;
            glGetActiveUniform(shaderProgram, i, 256, 0, &size, &type, uniformName);
            GL_CHECK_DETAIL;

            CL_LOG_D("GL", "  uniform %s, size %d, type %s (0x%04x)\n", uniformName, size, GetGLTypeName(type), type);

            if (IsSamplerType(type))
            {
                GLint uniformLocation = glGetUniformLocation(shaderProgram, uniformName);
                GL_CHECK_DETAIL;

                if (uniformLocation >= 0)
                {
                    GLint texLocation = ParseEnum(kTextureKindsEnum, uniformName);

                    if (texLocation >= 0)
                    {
                        CL_LOG_D("GL", "    binding sampler %s (%d) to texture index %d\n", uniformName, uniformLocation, texLocation);

                        glUniform1i(uniformLocation, texLocation);
                        unitsUsed |= 1 << texLocation;
                        GL_CHECK_DETAIL;
                    }
                    else
                    {
                        // Treat as a target tag.
                        samplersToAssign[numSamplersToAssign] = uniformLocation;
                        numSamplersToAssign++;
                    }
                }
                else
                {
                    CL_LOG_D("GL", "    shader doesn't contain sampler uniform %s - ignoring\n", uniformName);
                }
            }
        }

        int unit = 0;
        while (unitsUsed & (1 << unit))
            unit++;

        for (int i = 0; i < numSamplersToAssign; i++)
        {
            CL_LOG_D("GL", "    binding sampler uniform %d to texture index %d\n", samplersToAssign[i], unit);
            glUniform1i(samplersToAssign[i], unit);

            do
                unit++;
            while (unitsUsed & (1 << unit));
        }


        GL_CHECK_DETAIL;
    }

    const char* VersionString()
    {
        float  glLanguageVersion;
        
    #ifdef CL_GLES
        sscanf((char *)glGetString(GL_SHADING_LANGUAGE_VERSION), "OpenGL ES GLSL ES %f", &glLanguageVersion);
    #else
        sscanf((char *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%f", &glLanguageVersion);
    #endif
        
        // GL_SHADING_LANGUAGE_VERSION returns the version standard version form
        //  with decimals, but the GLSL version preprocessor directive simply
        //  uses integers (thus 1.10 should 110 and 1.40 should be 140, etc.)
        //  We multiply the floating point number by 100 to get a proper
        //  number for the GLSL preprocessor directive
        GLuint version = 100 * glLanguageVersion;

        // Get the size of the version preprocessor string info so we know
        //  how much memory to allocate for our sourceString
        static char versionString[sizeof("#version 123\n")];
        sprintf(versionString, "#version %d\n", version);

        return versionString;
    }

    GLuint BuildShaderProgram
    (
        const char*     vertexSource,
        const char*     fragmentSource,
        GLuint          shaderProgram,
        const char*     vsName = 0,
        const char*     fsName = 0,
        const char*     programName = 0
    )
    {
        GL_CHECK;

    #if CL_LOG_ENABLED
        if (!vsName)
            vsName = "memory";
        if (!fsName)
            fsName = "memory";
        if (!programName)
            programName = "unknown";
    #endif

        const char* versionString = VersionString();

        // Create a program object
        if (shaderProgram == 0)
            shaderProgram = glCreateProgram();
        else
            DestroyAttachedShaders(shaderProgram);

        CL_LOG_D("GL", "  program: %s (%d)\n", programName, shaderProgram);

        GLuint vs = CreateShader(GL_VERTEX_SHADER, versionString, vertexSource, vsName);

        if (vs == 0)
            return 0;

        glAttachShader(shaderProgram, vs);

        GLuint fs = CreateShader(GL_FRAGMENT_SHADER, versionString, fragmentSource, fsName);

        if (fs == 0)
            return 0;

        glAttachShader(shaderProgram, fs);

        if (!LinkShaders(shaderProgram, programName))
            return 0;

        if (!ValidateShaders(shaderProgram, programName))
            return 0;

        // Bind attributes to their correct locations
        BindAttributes(shaderProgram);

        // re-link =P
        if (!LinkShaders(shaderProgram, programName))
            return 0;

        BindSamplers(shaderProgram);

        GL_CHECK;

        glUseProgram(0);

        return shaderProgram;
    }
}

GLuint nHL::LoadShaders(const char* vsPath, const char* fsPath, GLuint shaderProgram, const char* materialName)
{
    GL_CHECK;

    tString vsSource;
    tString fsSource;

    if (LoadFromFile(vsPath, &vsSource) == 0)
    {
        CL_LOG_E("GL", "couldn't open %s for %s\n", vsPath, materialName);
        return 0;
    }
    if (LoadFromFile(fsPath, &fsSource) == 0)
    {
        CL_LOG_E("GL", "couldn't open %s for %s\n", fsPath, materialName);
        return 0;
    }

    // Build Program
    CL_LOG_D("GL", "  Creating shader from\n    vs: %s\n    ps: %s\n", vsPath, fsPath);

    return BuildShaderProgram(vsSource.c_str(), fsSource.c_str(), shaderProgram, vsPath, fsPath, materialName);
}


void nHL::DestroyAttachedShaders(GLuint shaderProgram)
{
    // strip off any existing shaders and delete them
    GLsizei count;
    GLuint previousShaders[16];

    glGetAttachedShaders(shaderProgram, CL_SIZE(previousShaders), &count, previousShaders);

    for (int i = 0; i < count; i++)
    {
        glDetachShader(shaderProgram, previousShaders[i]);
        glDeleteShader(previousShaders[i]);
    }
}

void nHL::DestroyShaderProgram(GLuint shaderProgram)
{
    DestroyAttachedShaders(shaderProgram);
    glDeleteProgram(shaderProgram);
}



//------------------------------------------------------------------------------
// Texture loading
//------------------------------------------------------------------------------

GLuint nHL::LoadTexture32(const cFileSpec& spec, GLuint texture)
{
    bool result = false;

    const char* pvrFile = spec.PathWithExtension("pvr");

    if (1)  // TODO: exists?
    {
        if (texture == 0)
            glGenTextures(1, &texture);

        result = LoadPVRTexture(pvrFile, texture);

        if (result)
            glBindTexture(GL_TEXTURE_2D, texture);
    }

    cImage32 image;

    if (!result && LoadImage(spec, &image))
    {
        CL_LOG_D("GL", "Successfully read %s, %d x %d\n", spec.Path(), image.mW, image.mH);
        
        // Create a texture object to apply to model
        if (texture == 0)
            glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // Indicate that pixel rows are tightly packed
        //  (defaults to stride of 4 which is kind of only good for
        //  RGBA or FLOAT data types)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        // Allocate and load image data into texture
        glTexImage2D(GL_TEXTURE_2D, 0 /*mip*/, GL_RGBA, image.mW, image.mH, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.mData);

        // Create mipmaps for this texture for better image quality
        glGenerateMipmap(GL_TEXTURE_2D);

        result = true;
    }

    if (result)
    {
        // Set up filter and wrap modes for this texture object
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        GL_CHECK;

        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }

    CL_LOG_E("GL", "Couldn't read %s!\n", spec.Path());

    return 0;
}

GLuint nHL::LoadTexture8(const cFileSpec& spec, GLuint texture)
{
    cImage8 image;

    if (LoadImage(spec, &image))
    {
        CL_LOG_D("GL", "Successfully read %s, %d x %d\n", spec.Path(), image.mW, image.mH);
        
        // Create a texture object to apply to model
        if (texture == 0)
            glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // Indicate that pixel rows are tightly packed
        //  (defaults to stride of 4 which is kind of only good for
        //  RGBA or FLOAT data types)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        // Allocate and load image data into texture
        glTexImage2D(GL_TEXTURE_2D, 0 /*mip*/, GL_ALPHA, image.mW, image.mH, 0, GL_ALPHA, GL_UNSIGNED_BYTE, image.mData);

        // Set up filter and wrap modes for this texture object
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        
        // Create mipmaps for this texture for better image quality
        glGenerateMipmap(GL_TEXTURE_2D);
        
        GL_CHECK;

        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }

    CL_LOG_E("GL", "Couldn't read %s!\n", spec.Path());

    return 0;
}

const cEnumInfo* nHL::TextureKindsEnum()
{
    return kTextureKindsEnum;
}


//------------------------------------------------------------------------------
// Mesh loading
//------------------------------------------------------------------------------

void nHL::DispatchMesh(const cGLMeshInfo* meshInfo)
{
    GL_CHECK;

    // Bind our vertex array object
    glBindVertexArray(meshInfo->mMesh);

    for (int i = 0; i < kMaxTextureKinds; i++)
        if (meshInfo->mTextures[i])
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, meshInfo->mTextures[i]);
        }

    GL_CHECK;
    glDrawElements(GL_TRIANGLES, meshInfo->mNumElts, meshInfo->mEltType, 0);
    GL_CHECK;

    for (int i = 0; i < kMaxTextureKinds; i++)
        if (meshInfo->mTextures[i])
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    glBindVertexArray(0);
    GL_CHECK;
}

void nHL::DestroyMesh(cGLMeshInfo* meshInfo)
{
    if (meshInfo->mMesh)
    {
        DestroyMesh(meshInfo->mMesh);
        meshInfo->mMesh = 0;
    }
    
    glDeleteTextures(kMaxTextureKinds, meshInfo->mTextures);
    for (int i = 0; i < kMaxTextureKinds; i++)
        meshInfo->mTextures[i] = 0;
}

// See esp. http://developer.apple.com/library/ios/#documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/WorkingwithEAGLContexts/WorkingwithEAGLContexts.html

void Resolve(GLuint resolveFB, GLuint sampleFB)
{
    // TODO: what's Mac equiv?
#if GL_APPLE_framebuffer_multisample //  && defined(CL_IOS)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFB);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sampleFB);
    glResolveMultisampleFramebufferAPPLE();

#elif GL_EXT_framebuffer_multisample
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFB);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sampleFB);

    int width  = 1024;
    int height = 1024;

    glBlitFramebuffer
    (
        0, 0, width, height,
        0, 0, width, height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
#endif
}

void DiscardMS()
{
#if GL_EXT_discard_framebuffer
    const GLenum discards[]  = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };

    glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, CL_SIZE(discards), discards);
#endif
}

void DiscardDepth()
{
#if GL_EXT_discard_framebuffer
    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };

    glDiscardFramebufferEXT(GL_FRAMEBUFFER, CL_SIZE(discards), discards);
#endif
}


//--- cRenderStateInfo ---------------------------------------------------------

cRenderStateInfo::cRenderStateInfo() :
    mCullMode(GL_INVALID_ENUM),
    mCullModeStamp(0),

    mDepthCompare(GL_INVALID_ENUM),
    mDepthCompareStamp(0),

    mDepthWrite(GL_INVALID_ENUM),
    mDepthWriteStamp(0),

    mBlend(GL_INVALID_ENUM),
    mBlendStamp(0),

    mBlendSource(GL_INVALID_ENUM),
    mBlendDest(GL_INVALID_ENUM),
    mBlendFuncStamp(0),

    mBlendColour(vl_1),
    mBlendColourStamp(0),

    mBlendEquation(GL_INVALID_ENUM),
    mBlendEquationStamp(0),

    mSavedStates(),
    mStateMarkers()
#ifndef CL_RELEASE
  , mScopeTags()
#endif
{}

cRenderStateInfo::~cRenderStateInfo()
{
    CL_ASSERT(mStateMarkers.empty());
}

void cRenderStateInfo::SetDeviceState()
{
    glEnable(GL_DEPTH_TEST);    // We always have this on.

    if (mCullMode == GL_NONE)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(mCullMode);
    }
    GL_CHECK;

    glDepthFunc(mDepthCompare);
    GL_CHECK;

    glDepthMask(mDepthWrite == kRSDepthWriteEnable ? GL_TRUE : GL_FALSE);
    GL_CHECK;

    if (mBlend == kRSBlendEnable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    glBlendFunc(mBlendSource, mBlendDest);
    GL_CHECK;

    glBlendColor(mBlendColour[0], mBlendColour[1], mBlendColour[2], mBlendColour[3]);
    GL_CHECK;

    glBlendEquation(mBlendEquation);
    GL_CHECK;
}

void cRenderStateInfo::ResetState()
{
    mCullMode = GL_BACK;
    mCullModeStamp = 0;

    mDepthCompare = GL_LESS;
    mDepthCompareStamp = 0;

    mDepthWrite = kRSDepthWriteEnable;
    mDepthWriteStamp = 0;

    mBlend = kRSBlendDisable;
    mBlendStamp = 0;

    mBlendSource = GL_ONE;
    mBlendDest   = GL_ZERO;
    mBlendFuncStamp = 0;

    mBlendEquation = GL_FUNC_ADD;
    mBlendEquationStamp = 0;
}

// make it easy to match push/pops. ID? char*?
void cRenderStateInfo::PushState(const char* debugTag)
{
    mStateMarkers.push_back(mSavedStates.size());

#ifndef CL_RELEASE
    mScopeTags.push_back(debugTag);
#endif
}

void cRenderStateInfo::PopState(const char* debugTag)   // uint32_t id
{
#ifndef CL_RELEASE
    CL_ASSERT(mScopeTags.back() == debugTag);
    mScopeTags.pop_back();
#endif

    int scopeStart = mStateMarkers.back();
    mStateMarkers.pop_back();   // now stamp is at N-1

    // TODO: what stops this from saving more state?
    // If state was saved during level N's scope, it's guaranteed it'll be stamped with N. (Or above?)
    // We want to restore it stamped with level N-1 (which should happen with pop above)
    // We don't want to save state in doing so. This shouldn't happen because the stamp should be >= N.
    // in fact, the redundant save test of < means we're not reliant on the pop for this. So its
    // only purpose is to ensure the new stamps are set correctly.
    // Of course, this means the stamp may have changed; is this important?
    // E.g., blend = add, push, push, push, blend = subtract (stamp = 3), pop -> blend = add (stamp = 2)., pop, blend = subtract -> won't save =P
    // Claim: if we clamp all stamps after a pop() to N-1 we'll be fine?!
    int scopeEnd = mSavedStates.size();
    if (scopeEnd > scopeStart)
        ApplyRenderState(scopeEnd - scopeStart, &mSavedStates[scopeStart]);

    CL_ASSERT(mSavedStates.size() == scopeEnd); // this should not have changed
    mSavedStates.resize(scopeStart);
}


void cRenderStateInfo::ApplyRenderState(int numTokens, const tRenderStateToken tokens[])
{
    const tRenderStateToken* endTokens = tokens + numTokens;
    GLenum mode;

    while (tokens < endTokens)
    {
        tRenderStateCommands command = tRenderStateCommands(*tokens++);
        
        switch (command)
        {
        case kRSCullMode:
            mode = *tokens++;

            if (mode != mCullMode)
            {
                if (mCullModeStamp < mStateMarkers.size())
                {
                    mSavedStates.push_back(kRSCullMode);
                    mSavedStates.push_back(mCullMode);
                }

                mCullMode = mode;
                mCullModeStamp = mStateMarkers.size();

                if (mode == GL_NONE)
                    glDisable(GL_CULL_FACE);
                else
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(mode);
                }
                GL_CHECK;
            }
            break;

        case kRSDepthCompare:
            mode = *tokens++;

            if (mode != mDepthCompare)
            {
                if (mDepthCompareStamp < mStateMarkers.size())
                {
                    mSavedStates.push_back(kRSDepthCompare);
                    mSavedStates.push_back(mDepthCompare);
                }

                mDepthCompare = mode;
                mDepthCompareStamp = mStateMarkers.size();

                glDepthFunc(mode);
                GL_CHECK;
            }
            break;

        case kRSDepthWriteEnable:
        case kRSDepthWriteDisable:
            if (command != mDepthWrite)
            {
                if (mDepthWriteStamp < mStateMarkers.size())
                    mSavedStates.push_back(mDepthWrite);

                mDepthWrite = command;
                mDepthWriteStamp = mStateMarkers.size();

                glDepthMask(command == kRSDepthWriteEnable ? GL_TRUE : GL_FALSE);
                GL_CHECK;
            }
            break;

        case kRSBlendEnable:
        case kRSBlendDisable:
            if (command != mBlend)
            {
                if (mBlendStamp < mStateMarkers.size())
                    mSavedStates.push_back(mBlend);

                mBlend = command;
                mBlendStamp = mStateMarkers.size();

                if (command == kRSBlendEnable)
                    glEnable(GL_BLEND);
                else
                    glDisable(GL_BLEND);
                GL_CHECK;
            }
            break;

        case kRSBlendFunc:
            {
                GLenum blendSource = *tokens++;
                GLenum blendDest   = *tokens++;

//              if (blendSource != mBlendRGBSource || blendSource != mBlendASource
//               || blendDest   != mBlendRGBDest   || blendDest   != mBlendADest)
//                if (blendSource != mBlendSource || blendDest != mBlendDest)
                {
                    if (mBlendFuncStamp < mStateMarkers.size())
                    {
                        mSavedStates.push_back(kRSBlendFunc);
                        mSavedStates.push_back(mBlendSource);
                        mSavedStates.push_back(mBlendDest);
                    }

                    mBlendSource = blendSource;
                    mBlendDest   = blendDest;
                    mBlendFuncStamp = mStateMarkers.size();

                    glBlendFunc(blendSource, blendDest);
                    GL_CHECK;
                }
            }
            break;

        case kRSBlendColour:
            {
                Vec4f bc;

                bc[0] = (const float&) *tokens++;
                bc[1] = (const float&) *tokens++;
                bc[2] = (const float&) *tokens++;
                bc[3] = (const float&) *tokens++;

                if (bc != mBlendColour)
                {
                    if (mBlendColourStamp < mStateMarkers.size())
                    {
                        mSavedStates.push_back(kRSBlendColour);
                        mSavedStates.push_back((tRenderStateToken&) mBlendColour[0]);
                        mSavedStates.push_back((tRenderStateToken&) mBlendColour[1]);
                        mSavedStates.push_back((tRenderStateToken&) mBlendColour[2]);
                        mSavedStates.push_back((tRenderStateToken&) mBlendColour[3]);
                    }

                    mBlendColour = bc;
                    mBlendColourStamp = mStateMarkers.size();

                    glBlendColor(bc[0], bc[1], bc[2], bc[3]);
                    GL_CHECK;
                }
            }
            break;

        case kRSBlendEquation:
            mode = *tokens++;

//            if (mode != mBlendEquation)
            {
                if (mBlendEquationStamp < mStateMarkers.size())
                {
                    mSavedStates.push_back(kRSBlendEquation);
                    mSavedStates.push_back(mBlendEquation);
                }

                mBlendEquation = mode;
                mBlendEquationStamp = mStateMarkers.size();

                glBlendEquation(mode);
                GL_CHECK;
            }
            break;

        default:
            CL_ERROR("Unknown token");
            return;
        };

        GL_CHECK;
    }
}

// GL_FRONT, GL_BACK, and GL_FRONT_AND_BACK
cEnumInfo kEnumCullMode[] =
{
    "none",         GL_NONE,
    "back",         GL_BACK,
    "front",        GL_FRONT,
    "frontAndBack", GL_FRONT_AND_BACK,
    0, 0
};

cEnumInfo kEnumCompare[] =
{
    "none",             GL_NEVER,
    "always",           GL_ALWAYS,
    "less",             GL_LESS,
    "lessOrEqual",      GL_LEQUAL,
    "equal",            GL_EQUAL,
    "greater",          GL_GREATER,
    "greaterOrEqual",   GL_GEQUAL,
    "notEqual",         GL_NOTEQUAL,
    0, 0
};

cEnumInfo kEnumBlendFactor[] =
{
    "zero",                 GL_ZERO,
    "one",                  GL_ONE,
    "sourceColour",         GL_SRC_COLOR,
    "invSourceColour",      GL_ONE_MINUS_SRC_COLOR,
    "destColour",           GL_DST_COLOR,
    "invDestColour",        GL_ONE_MINUS_DST_COLOR,
    "sourceAlpha",          GL_SRC_ALPHA,
    "invSourceAlpha",       GL_ONE_MINUS_SRC_ALPHA,
    "destAlpha",            GL_DST_ALPHA,
    "invDestAlpha",         GL_ONE_MINUS_DST_ALPHA,
    "constantColour",       GL_CONSTANT_COLOR,
    "invConstantColour",    GL_ONE_MINUS_CONSTANT_COLOR,
    "constantAlpha",        GL_CONSTANT_ALPHA,
    "invConstantAlpha",     GL_ONE_MINUS_CONSTANT_ALPHA,
    0, 0
};

cEnumInfo kEnumBlendType[] =
{
    "min",              GL_MIN,
    "max",              GL_MAX,
    "add",              GL_FUNC_ADD,
    "subtract",         GL_FUNC_SUBTRACT,
    "sourceMinusDest",  GL_FUNC_SUBTRACT,
    "destMinusSource",  GL_FUNC_REVERSE_SUBTRACT,
    0, 0
};


void nHL::AddRenderStates(const cObjectValue* object, vector<tRenderStateToken>* renderStates)
{
    GLenum cullMode = AsEnum(object->Member("cullMode"), kEnumCullMode, GL_INVALID_ENUM);

    if (cullMode != GL_INVALID_ENUM)
    {
        renderStates->push_back(kRSCullMode);
        renderStates->push_back(cullMode);
    }

    GLenum depthCompare = AsEnum(object->Member("depthCompare"), kEnumCompare, GL_INVALID_ENUM);

    if (depthCompare != GL_INVALID_ENUM)
    {
        renderStates->push_back(kRSDepthCompare);
        renderStates->push_back(depthCompare);
    }

    const cValue& depthWriteValue = object->Member("depthWrite");

    if (depthWriteValue.IsIntegral())   // allows 0/1 too
    {
        if (depthWriteValue.AsBool())
            renderStates->push_back(kRSDepthWriteEnable);
        else
            renderStates->push_back(kRSDepthWriteDisable);
    }

    const cValue& blendValue = object->Member("blend");

    if (blendValue.IsIntegral())   // Using IsIntegral allows 0/1 in addition to true/false
    {
        if (blendValue.AsBool())
            renderStates->push_back(kRSBlendEnable);
        else
            renderStates->push_back(kRSBlendDisable);
    }
    else if (blendValue.IsArray() && blendValue.NumElts() == 3)
    {
        // blendType blendSource blendDest
        GLenum blendType = AsEnum(blendValue.Elt(0), kEnumBlendType, GL_INVALID_ENUM);

        if (blendType != GL_INVALID_ENUM)
        {
            renderStates->push_back(kRSBlendEnable);
            renderStates->push_back(kRSBlendEquation);
            renderStates->push_back(blendType);
        }
        else
            CL_LOG_E("GL", "Bad blend type\n");

        GLenum blendSource = AsEnum(blendValue.Elt(1), kEnumBlendFactor, GL_INVALID_ENUM);
        GLenum blendDest   = AsEnum(blendValue.Elt(2), kEnumBlendFactor, GL_INVALID_ENUM);

        if (blendSource != GL_INVALID_ENUM && blendDest != GL_INVALID_ENUM)
        {
            renderStates->push_back(kRSBlendFunc);
            renderStates->push_back(blendSource);
            renderStates->push_back(blendDest);
        }
        else
            CL_LOG_E("GL", "Bad source or dest blend\n");
    }

    GLenum blendSource = AsEnum(object->Member("blendSource"), kEnumBlendFactor, GL_INVALID_ENUM);
    GLenum blendDest   = AsEnum(object->Member("blendDest"), kEnumBlendFactor, GL_INVALID_ENUM);

    if (blendSource != GL_INVALID_ENUM && blendDest != GL_INVALID_ENUM)
    {
        renderStates->push_back(kRSBlendFunc);
        renderStates->push_back(blendSource);
        renderStates->push_back(blendDest);
    }

    const cValue& blendColour = object->Member("blendColour");

    if (blendColour.IsArray() && blendColour.NumElts() == 4)
    {
        Vec4f bc;
        SetFromValue(blendColour, 4, bc.Ref());
        
        renderStates->push_back((uint32_t&) bc[0]);
        renderStates->push_back((uint32_t&) bc[1]);
        renderStates->push_back((uint32_t&) bc[2]);
        renderStates->push_back((uint32_t&) bc[3]);
    }

    GLenum blendType = AsEnum(object->Member("blendType"), kEnumBlendType, GL_INVALID_ENUM);

    if (blendType != GL_INVALID_ENUM)
    {
        renderStates->push_back(kRSBlendEquation);
        renderStates->push_back(blendType);
    }

// TODO    kRSBlendFuncSep,    // 4: sourceRGB, destRGB, sourceA, destA
// TODO    kRSBlendEquationSep,    // 2 x [GL_MIN, GL_MAX, GL_FUNC_ADD...]
}

#if 0

void AllRenderState()
{
    // These are sourced from the GLES quick reference card

    // Rasterisation
    glEnable(GL_CULL_FACE);
    glCullFace(enum mode);
    // mode: FRONT, BACK, FRONT_AND_BACK Enable/Disable(CULL_FACE)
    glFrontFace(enum dir);
    // dir: CCW, CW
    glLineWidth(float width);
    glPolygonOffset(float factor, float units);
    glEnable(POLYGON_OFFSET_FILL);

    // Fragment ops
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(enum func);
    // compOp: NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GREATER, GEQUAL, NOTEQUAL

    glEnable(GL_BLEND);
    glBlendFunc(enum src, enum dst);
    glBlendFuncSeparate(enum srcRGB, enum dstRGB, enum srcAlpha, enum dstAlpha);
    // ZERO, ONE, [ONE_MINUS_]SRC_COLOR,[ONE_MINUS_]DST_COLOR, [ONE_MINUS_]SRC_ALPHA, [ONE_MINUS_]DST_ALPHA, [ONE_MINUS_]CONSTANT_COLOR, [ONE_MINUS_]CONSTANT_ALPHA
    // SRC_ALPHA_SATURATE
    glBlendColor(clampf red, clampf green, clampf blue, clampf alpha);

    glBlendEquation(enum mode);
    glBlendEquationSeparate(enum modeRGB, enum modeAlpha);
    // mode:  FUNC_ADD, FUNC_SUBTRACT, FUNC_REVERSE_SUBTRACT

    // Stencil
    glEnable(STENCIL_TEST);
    glStencilFunc(enum func, int ref, uint mask);   // func: NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GREATER, GEQUAL, NOTEQUAL
    glStencilFuncSeparate(enum face, enum func, int ref, uint mask);
    glStencilOp(enum sfail, enum dpfail, enum dppass);
    glStencilOpSeparate(enum face, enum sfail, enum dpfail, enum dppass);
    // face: GL_FRONT,
    // stencilOp: KEEP, ZERO, REPLACE, INCR, DECR, INVERT, INCR_WRAP, DECR_WRAP
    // stencilFunc: NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GREATER, GEQUAL, NOTEQUAL

    // Multisample
    glEnable(SAMPLE_COVERAGE);
    glEnable(SAMPLE_ALPHA_TO_COVERAGE);
    glSampleCoverage(clampf value, boolean invert);
    // Dithering
    glEnable(GL_DITHER);

    // Scissor
    glEnable(GL_SCISSOR_TEST);
    glScissor();

    // Framebuffer
    glColorMask();
    glDepthMask();
    glStencilMask();
    glStencilMaskSeparate();

    glClear(bitfield);  // COLOR_BUFFER_BIT, DEPTH_BUFFER_BIT, STENCIL_BUFFER_BIT
    glClearColor();
    glClearDepthf();
    glClearStencil();

    // Viewport
    void glDepthRangef(clampf n, clampf f);
    void glViewport(int x, int y, sizei w, sizei h);
}
#endif


void nHL::GetGLLimits()
{
    GLint limit;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    CL_LOG("GL", "Maximum size of the texture: %d\n", limit);

    glGetIntegerv(GL_DEPTH_BITS, &limit);
    CL_LOG("GL", "Number of depth buffer planes: %d\n", limit);

    glGetIntegerv(GL_STENCIL_BITS, &limit);
    CL_LOG("GL", "Number of stencil buffer planes: %d\n", limit);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &limit);
    CL_LOG("GL", "Maximum number of vertex attributes: %d\n", limit);

#ifdef CL_IOS
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &limit);
    CL_LOG("GL", "Maximum number of uniform vertex vectors: %d\n", limit);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &limit);
    CL_LOG("GL", "Maximum number of uniform fragment vectors: %d\n", limit);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &limit);
    CL_LOG("GL", "Maximum number of varying vectors: %d\n", limit);
#endif

    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &limit);
    CL_LOG("GL", "Maximum number of texture units usable in a vertex shader: %d\n", limit);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &limit);
    CL_LOG("GL", "Maximum number of texture units usable in a fragment shader: %d\n", limit);

    CL_LOG("GL", "Extensions: %s\n", glGetString(GL_EXTENSIONS));
}

#ifdef TODO

// Useful resource for accelerating buffer format conversions/fills.
#include <Accelerate/Accelerate.h>


// https://developer.apple.com/library/ios/DOCUMENTATION/Performance/Reference/vImage_conversion/Reference/reference.html
#endif
