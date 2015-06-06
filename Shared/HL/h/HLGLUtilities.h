//
//  File:       HLGLUtilities.h
//
//  Function:   GLES utility functions
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef HL_GL_UTILITIES_H
#define HL_GL_UTILITIES_H

#include <IHLRenderer.h>
#include <GLConfig.h>
#include <VL234f.h>
#include <CLSTL.h>

// #define DETAILED_GL_CHECKING

#ifndef GL_CHECKING
    #ifdef CL_DEBUG
        #define GL_CHECKING 1
    #else
        #define GL_CHECKING 0
    #endif
#endif

#ifndef GL_CHECKING_DETAILED
    #define GL_CHECKING_DETAILED 0
#endif

#if GL_CHECKING
    #define GL_CHECK GetGLError()
#else
    #define GL_CHECK
#endif

#if GL_CHECKING_DETAILED
    #define GL_CHECK_DETAIL GetGLError()
#else
    #define GL_CHECK_DETAIL
#endif


namespace nCL
{
    class cValue;
    class cFileSpec;
}

namespace nHL
{
    struct cGLMeshInfo
    {
        GLuint  mMesh;      // 'VertexArray' (horrible name) object containing mesh vertex & index data
        
        GLuint  mNumElts;
        GLuint  mEltType;
        
        GLuint  mTextures[kMaxTextureKinds];

        cGLMeshInfo();
    };


    // Shaders
    GLuint LoadShaders(const char* vsPath, const char* fsPath, GLuint shaderProgram = 0, const char* materialName = 0);
    ///< Loads & compiles shader files specified by vsPath, fsPath
    void DestroyAttachedShaders(GLuint shaderProgram);      ///< Detach & destroy all attached shaders
    void DestroyShaderProgram  (GLuint shaderProgram);      ///< Destroy shader program and all attached shaders

    // Textures
    GLuint LoadTexture32(const nCL::cFileSpec& spec, GLuint texture = 0);    // Load 32-bit rgba texture
    GLuint LoadTexture8 (const nCL::cFileSpec& spec, GLuint texture = 0);    // Load 8-bit mono texture
    const nCL::cEnumInfo* TextureKindsEnum();

    // Meshes
    bool LoadMesh    (cGLMeshInfo* info, const char* modelName, const char* textureName);
    void DispatchMesh(const cGLMeshInfo* meshInfo);
    void DestroyMesh (cGLMeshInfo* meshInfo);

    void DestroyMesh(GLuint meshName);  // Destroys given mesh (VA) and all its included VBs

    // Render state
    typedef uint32_t tRenderStateToken;
    void AddRenderStates(const nCL::cObjectValue* object, nCL::vector<tRenderStateToken>* renderStates);

    enum tRenderStateCommands
    {
        kRSNull = 0,

        kRSCullMode             = 1,         // 1: NONE, FRONT, BACK, FRONT_AND_BACK

        kRSDepthCompare         = 2,     // 1: NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GREATER, GEQUAL, NOTEQUAL
        kRSDepthWriteDisable    = 3,
        kRSDepthWriteEnable     = 4,

        kRSBlendEnable          = 5,
        kRSBlendDisable         = 6,
        kRSBlendFunc            = 7,       // 2: source, dest:
    //    kRSBlendFuncSep,    // 4: sourceRGB, destRGB, sourceA, destA
        kRSBlendColour          = 8,     // float r, float g, float b, float a
        kRSBlendEquation        = 9,   // [GL_MIN, GL_MAX, GL_FUNC_ADD...]
    //    kRSBlendEquationSep,    // 2 x [GL_MIN, GL_MAX, GL_FUNC_ADD...]
        // TODO: stencil commands
        // TODO: mask commands
        // TODO: multisampling
        // 
        kRSEnd,
        kMaxRenderStateCommands
    };

    class cRenderStateInfo
    /// Provides management of render state, including push/pop, and avoiding
    /// redundant device calls
    {
    public:
        cRenderStateInfo();
        ~cRenderStateInfo();

        void SetDeviceState();  ///< Ensure device matches our current state
        void ResetState();      ///< Reset state to defaults.

        void ApplyRenderState(int numTokens, const tRenderStateToken tokens[]);
        void PushState(const char* debugTag = 0);
        void PopState (const char* debugTag = 0);

    protected:
        // Data
        tRenderStateToken   mCullMode;
        uint32_t            mCullModeStamp;

        tRenderStateToken   mDepthCompare;
        uint32_t            mDepthCompareStamp;

        tRenderStateToken   mDepthWrite;
        uint32_t            mDepthWriteStamp;

        tRenderStateToken   mBlend;
        uint32_t            mBlendStamp;

        tRenderStateToken   mBlendSource;
        tRenderStateToken   mBlendDest;
        uint32_t            mBlendFuncStamp;

        Vec4f               mBlendColour;
        uint32_t            mBlendColourStamp;

        tRenderStateToken   mBlendEquation;
        uint32_t            mBlendEquationStamp;

        nCL::vector<tRenderStateToken>   mSavedStates;
        nCL::vector<int>                 mStateMarkers;
    #ifndef CL_RELEASE
        nCL::vector<const char*>         mScopeTags;
    #endif
    };


    // RTs
    GLuint CreateRenderBuffer(int width, int height, bool withDepthBuffer = true, int multiSample = 0);
    GLuint CreateRenderTexture(int width, int height);

    void DiscardMS();
    void DiscardDepth();

    GLuint BuildFBO  (GLuint width, GLuint height);
    void   DestroyFBO(GLuint fboName);

    // Utilities
    void        GetGLError();   ///< Dump current set of GL errors and reset

    bool        IsSamplerType(GLenum type); ///< Returns true if GL_SAMPLER_* type
    GLsizei     GetGLTypeSize(GLenum type); ///< Size of given type (e.g., GL_BYTE)
    const char* GetGLTypeName(GLenum type); ///< Debug name for given type

    void GetGLLimits();


    // --- Inlines -------------------------------------------------------------

    inline bool IsSamplerType(GLenum type)
    {
    #ifdef CL_GLES
        return (type >= GL_SAMPLER_2D && type <= GL_SAMPLER_CUBE);
    #else
        return (type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_SHADOW);
    #endif
    }
}

#endif
