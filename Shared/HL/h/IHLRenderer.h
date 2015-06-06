//
//  File:       IHLRenderer.h
//
//  Function:   Renderer system
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_RENDERER_H
#define HL_I_RENDERER_H

#include <HLDefs.h>
#include <VL234f.h>

namespace nCL
{
    class cAllocImage32;
}

namespace nHL
{
    using nCL::cAllocImage32;
    class cICamera;
    class cUIState;

    typedef int tShaderDataRef;
    const tShaderDataRef kNullDataRef = -1;

    // Built-in shader data IDs
    enum tBuiltInShaderDataID : tShaderDataRef
    {
        kDataIDModelToWorld,
        kDataIDWorldToCamera,
        kDataIDCameraToClip,

        // Derived transforms -- normally set automatically from the above.
        kDataIDModelToCamera,
        kDataIDWorldToClip,
        kDataIDModelToClip,

        // Inverses, ditto
        kDataIDWorldToModel,
        kDataIDCameraToWorld,
        kDataIDClipToCamera,

        kDataIDCameraToModel,
        kDataIDClipToModel,
        kDataIDClipToWorld,

        kDataIDViewSize,        ///< View size in pixels, Vec2f
        kDataIDViewCentre,      ///< View centre in pixels, Vec2f
        kDataIDViewOffset,      ///< View offset in pixels, Vec2f

        kDataIDDeviceOrient,        ///< Device orientation transform, applied immediately pre-clip, Mat3f
        kDataIDOrientedViewSize,    ///< Oriented version of the underlying device view size, Vec2f

        kDataIDTime,
        kDataIDPulse,

        kDataIDPointer1,
        kDataIDPointer2,
        kDataIDPointer3,
        kDataIDPointer4,

        kMaxBuiltInShaderDataID
    };

    // Standard vertex attributes, consistent across all shaders
    enum tVertexAttributes
    {
        kVBPositions,
        kVBTexCoords,
        kVBColours,
        kVBNormals,
        kMaxAttributes
    };

    // Standard texture types, consistent across all shaders
    enum tTextureKind
    {
        kTextureDiffuseMap,
        kTextureNormalMap,
        kTextureShadowMap,
        kTextureAlternateMap,
        kMaxTextureKinds
    };

    const tTag   kMainTag    = CL_TAG("main");
    const tTag   kDefaultTag = CL_TAG("default");

    // Built-in system render layers
    const tTag kRenderLayerModelManager    = CL_TAG("models");
    const tTag kRenderLayerEffectsManager  = CL_TAG("effects");
    const tTag kRenderLayerDebugDraw       = CL_TAG("debugDraw");

    // Render flags to control layers
    typedef uint64_t tRenderFlags;
    enum { kMaxRenderFlags = 64 };

    // Reserved render flags
    const int kRenderFlagDebug = 0;

    struct cRenderLayerState
    {
        tTag         mLayerTag   = nCL::kNullTag;   ///< ID of layer currently being rendered
        tRenderFlags mFlags      = 0;   ///< Current set of render flags -- see SetRenderFlag()
        uint32_t     mLayerFlags = 0;   ///< Flags passed into RegisterLayer()
    };

    // Interface for something that dispatches stuff to the renderer, usually a layer.
    class cIRenderer;

    class cIRenderLayer
    {
    public:
        virtual int Link(int count) const = 0;

        virtual void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) = 0;
    };

    // Camera
    class cICamera
    {
    public:
        virtual int  Link(int count) const = 0;

        virtual void Config(const cObjectValue* value) = 0; ///< Configure the camera from the given object value
        virtual void Dispatch(cIRenderer* renderer) = 0;    ///< Set camera shader data on renderer

        virtual void                   SetCameraToWorld(const nCL::cTransform&) = 0;  ///< Set camera's current world space transform.
        virtual const nCL::cTransform& CameraToWorld() = 0;  ///< Get camera's current world space transform.

        virtual void                   SetSceneBounds(const nCL::cBounds3& b) = 0;  ///< Set bounds of scene or area of interest. Use is camera-specific

        virtual void FindView          (Mat4f* v) = 0;
        virtual void FindProjection    (Vec2f viewSize, Mat4f* p) = 0;
        virtual void FindViewProjection(Vec2f viewSize, Mat4f* vp) = 0;
    };

    // Mesh definitions
    struct cEltInfo
    {
        tVertexAttributes mVAType;

        int     mNumCmpts;
        int     mCmptType;
        int     mDataSize;
        bool    mNormalised;
    };

    struct cGLMeshInfo;

    enum tBufferFormat : uint8_t
    {
        kFormatRGB,
        kFormatRGBA,
        kFormatA,
        kFormatL,
        kFormatLA,
        kFormatDepth,
        kFormatStencil,

        kFormatRGBA4,
        kFormatRGB5A1,
        kFormatRGB565,

        kFormatRGBA8,
        
        kFormatA8,
        kFormatL8,
        kFormatLA8,

        kFormatD16,
        kFormatD24,
        kFormatD32,

        kFormatS8,

        kFormatD24S8,

        kMaxBufferFormats
    };

    typedef void tShaderDataUpdateFunc(cIRenderer* renderer, size_t dataSize, void* data);
    typedef void tShaderDataConfigFunc(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config);

    typedef int tMaterialRef;
    typedef int tTextureRef;

    const tMaterialRef kNullMaterialRef = -1;
    const tTextureRef  kNullTextureRef = -1;

    bool IsValid(int ref);


    // --- cIRenderer ----------------------------------------------------------

    class cIRenderer
    {
    public:
        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual void SetFrameBufferInfo(uint32_t fb, int width, int height, int orientation) = 0;    ///< Sets default framebuffer info.

        virtual void Update(float dt) = 0;      ///< Handle any main-thread stuff
        virtual void Render() = 0;              ///< Top level render call -- dispatch the next frame

        // Cameras
        virtual void RegisterCamera(tTag cameraTag, cICamera* camera) = 0;     ///< Register given camera under the given ID, or remove if camera = 0.
        virtual cICamera*    Camera(tTag cameraTag) const = 0;                 ///< Returns given camera, or 0 if none

        // Layers
        virtual void LoadLayersAndBuffers(const nCL::cObjectValue* config) = 0;  ///< Load layers & buffer info specified by the given config

        virtual void   RegisterLayer(tTag layerTag, cIRenderLayer* layer, uint32_t flags = 0) = 0;    ///< Register given layer under the given ID, or remove if layer = 0.
        virtual cIRenderLayer* Layer(tTag layerTag) const = 0;   ///< Returns given code/data layer, or 0 if none

        virtual tTag   SetRenderLayer(tTag layerTag) = 0;    ///< Switch to the given layer for rendering. Returns previous layer.

        virtual void AddRenderJob(tTag jobGroupTag, tTag layerTag) = 0;   ///< Add given layer to be run as a one-time job on the next frame.

        // Render flags
        virtual void SetRenderFlag (int flag, bool enabled) = 0;    ///< Set the given render flag, which may affect render layers
        virtual bool RenderFlag    (int flag) const = 0;            ///< Returns current render flag value
        virtual void SetRenderFlags(const cObjectValue* flags) = 0; ///< Set flags defined by the given cObjectValue

        // Materials
        virtual void LoadMaterials(const nCL::cObjectValue* config) = 0;  ///< Load materials specified by the given config

        virtual tMaterialRef MaterialRefFromTag(tTag materialTag) = 0;  ///< Returns the index of the given material. If not found, may return -1 or 0 for invalid material.
        virtual bool         SetMaterial(tMaterialRef ref) = 0;         ///< Set the given material (shader + render state) for drawing. Returns true if drawing should take place.

        // Textures
        virtual void LoadTextures(const nCL::cObjectValue* config) = 0;   ///< Load textures specified by the given config.

        virtual tTextureRef CreateTexture
        (
            tTag                texturetag,
            tBufferFormat       format,
            int                 width,
            int                 height,
            const uint8_t*      data = 0,
            const cObjectValue* config = 0
        ) = 0;      ///< Create texture with the given format and optional config.
        virtual bool        DestroyTexture(tTextureRef ref) = 0;    ///< Destroy the given texture.

        virtual tTextureRef TextureRefFromTag(tTag textureTag) = 0;            ///< Returns index of the given texture, or 0 if not found.

        virtual void        SetTextures(const cObjectValue* object) = 0;           ///< Set texture kinds according to the given object
        virtual void        SetTexture(tTextureKind kind, tTextureRef textureIndex) = 0;   ///< Set given texture.
        virtual void        SetTexture(tTag kind, tTextureRef textureIndex) = 0;   ///< Set given texture kind to hold the given texture.

        virtual void        UpdateTexture(tTextureRef ref, tBufferFormat format, const void* data) = 0; ///< Update texture with new contents from CPU

        // Shader Data
        virtual void LoadShaderData(const nCL::cObjectValue* config) = 0;   ///< Set up data-defined shader data

        virtual void        SetShaderData (const nCL::cObjectValue* object) = 0; ///< Set the shader data defined by 'object'
        virtual bool        SetShaderData (tShaderDataRef ref, size_t dataSize, const void* data) = 0; ///< Set the given shader data for access by shaders. Returns false if no such shader data exists.
        virtual const void* ShaderData    (tShaderDataRef ref) const = 0;      ///< Returns current value of given shader data, or 0 if none.
        virtual size_t      ShaderDataSize(tShaderDataRef ref) const = 0;      ///< Returns current size of given shader data, or 0 if none.
        
        // Typed versions
        template<class T> void      SetShaderDataT(const T& data);
        template<class T> void      SetShaderDataT(tShaderDataRef ref, const T& data);
        template<class T> const T&  ShaderDataT   ();
        template<class T> const T&  ShaderDataT   (tShaderDataRef ref);

        virtual tShaderDataRef ShaderDataRefFromTag(tTag tag) const = 0;  ///< Get ref for shader data with given ID
        virtual tShaderDataRef AddShaderData       (tTag tag) = 0;        ///< Add new 'code' shader data

        virtual void SetShaderDataUpdate(tShaderDataRef ref, tShaderDataUpdateFunc f, int numDeps, const tShaderDataRef deps[]) = 0;
        ///< Sets a function to auto-update the given shader data, and lists other data if any that this function is dependent on.
        virtual void SetShaderDataConfig(tShaderDataRef ref, tShaderDataConfigFunc f) = 0;
        ///< Sets a function to set the given shader data from a value

        // Copy-back buffers
        virtual const cAllocImage32*   CopyBackBuffer(tTag tag, uint32_t* updateCount = 0) = 0;    ///< Returns the given buffer if it exists, and updates 'updateCount', or returns 0 if it hasn't been updated since input 'updateCount'.
        virtual uint32_t UpdateCountForCopyBackBuffer(tTag tag) = 0;   ///< Returns current update count for the given buffer. This will only change within the render Update() call on the main thread.

        // Rendering

        // Quad rendering, for particles etc.
        virtual int  CreateQuadMesh (int numQuads, int numElts, cEltInfo elts[]) = 0;    ///< Create a mesh of the given vertex format to be used in quad rendering, and return slot, or 0 on failure.
        virtual void DestroyQuadMesh(int quadMesh) = 0;

        virtual int  GetQuadBuffer           (int quadMesh, int count, uint8_t** buffer) = 0;
        ///< Get a buffer of up to 'count' quads to fill. The actual buffer size is returned.
        virtual void DispatchAndReleaseBuffer(int quadMesh, int numQuads) = 0;
        ///< Draw the given buffer and release once done.

        virtual void DrawBuffer(uint32_t mode, int numElts, cEltInfo elts[], int count, const void* buffer) = 0;
        ///< Simple unindexed buffer rendering, intended for dev use. 'mode' is e.g., GL_TRIANGLES

        virtual void DrawMesh(const cGLMeshInfo* meshInfo) = 0; ///< Draw predefined mesh

        virtual void PushState(const char* debugTag) = 0;   ///< push camera/shader/render state
        virtual void PopState (const char* debugTag) = 0;   ///< pop camera/shader/render state. If specified debugTag will be matched against the corresponding PushState to check nesting.

    #ifndef CL_RELEASE
        // Development
        virtual void DebugMenu(cUIState* uiState) = 0;
    #endif
    };

    cIRenderer* CreateRenderer (nCL::cIAllocator* alloc);
    void        DestroyRenderer(cIRenderer* renderer);


    // --- Inlines -------------------------------------------------------------

    inline bool IsValid(tMaterialRef ref)
    {
        return ref >= 0;
    }

    template<class T> inline void cIRenderer::SetShaderDataT(const T& data)
    {
        SetShaderData(T::kID, sizeof(T), &data);
    }

    template<class T> inline void cIRenderer::SetShaderDataT(tShaderDataRef ref, const T& data)
    {
        SetShaderData(ref, sizeof(T), &data);
    }

    template<class T> inline const T& cIRenderer::ShaderDataT()
    {
        return *reinterpret_cast<const T*>(ShaderData(T::kIID));
    }

    template<class T> inline const T& cIRenderer::ShaderDataT(tShaderDataRef ref)
    {
        return *reinterpret_cast<const T*>(ShaderData(ref));
    }
}

#endif
