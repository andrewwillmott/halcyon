//
//  File:       HLRenderer.h
//
//  Function:   Renderer system
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_RENDERER_H
#define HL_RENDERER_H

#include <IHLRenderer.h>

#include <HLGLUtilities.h>

#include <CLColour.h>
#include <CLData.h>
#include <CLFileWatch.h>
#include <CLImage.h>
#include <CLLink.h>
#include <CLMemory.h>
#include <CLSTL.h>
#include <CLSlotArray.h>

#include <VL234f.h>
#include <VL234i.h>


namespace nCL
{
    class cInputState;
    class cValue;
}

namespace nHL
{
    // --- Shader/Geometry ----------------------------------------------------------

    // Geometry
    struct cQuadMesh
    {
        uint32_t mMesh = 0;
        uint32_t mVB = 0;
        size_t   mNumQuads = 0;
        size_t   mVertexSize = 0;
        size_t   mBufferSize = 0;

        size_t   mVBCursor = 0;       // In bytes, not vertices
        int      mIndexStart = 0;   // where in the index array to draw from next.
    };


    // Shader data
    struct cShaderDataInfo
    {
        uint16_t         mSize       = 0;
        nCL::tDataOffset mOffset     = nCL::kNullDataOffset;

        tShaderDataUpdateFunc*       mUpdateFunc = 0;
        nCL::vector<tShaderDataRef>  mUpdateDependencies;  // Shader data that mUpdateFunc relies on.
        nCL::set<tShaderDataRef>     mUpdateDependents;    // Shader data that's dependent on the contents of this.

        tShaderDataConfigFunc*       mConfigFunc = 0;
    };

    // Mapping between a shader uniform, and corresponding CPU-side shader data.
    struct cShaderDataBinding
    {
        tShaderDataRef mDataRef;
        GLuint   mLocation;
        GLenum   mType;
        int      mCount;
        GLboolean mTranspose;

        cShaderDataBinding() :
            mDataRef (kNullDataRef),
            mLocation(0),
            mType    (GL_FLOAT),
            mCount   (0),
            mTranspose(GL_FALSE)
        {}
    };

    struct cTextureBinding
    {
        tTag    mKind;
        int     mUnit;
    };

    struct cMaterial
    {
        uint32_t                        mShaderProgram = 0;             ///< Compiled shader programs
        uint32_t                        mTextures[kMaxTextureKinds] = {0}; ///< Samplers to set
        nCL::vector<uint32_t>           mRenderStates;                  ///< Render states to set
        nCL::vector<cShaderDataBinding> mShaderDataBindings;            ///< Shader data we reference
        nCL::vector<cTextureBinding>    mTextureBindings;               ///< Texture kinds we reference
        bool                            mIsValid = true;                ///< True if compiled successfully

    #ifndef CL_RELEASE
        // Hotload support
        int mVSRef = -1;
        int mFSRef = -1;
    #endif
    };

    struct cTextureInfo
    {
        Vec2i mSize = vl_0;
    };

    struct cFrameBufferInfo
    {
        Vec2i mSize = vl_0;
        bool  mDeviceOriented = false;
    };

    struct cCopyBackBufferInfo
    {
        uint32_t mUpdateCount = 0;
        cLink<cAllocImage32> mImage;
    };


    // --- cDataLayer ----------------------------------------------------------

    enum tRenderCommandType
    {
        kRCDrawLayer,
        kRCDrawJobGroup,

        kRCSetFrameBuffer,
        kRCSetCamera,
        kRCSetMaterial,
        kRCSetTexture,
        kRCSetShaderData,

        kRCClear,
        kRCDrawRect,
        kRCBlit,
        kRCResolve,
        kRCGenerateMips,
        kRCDiscardBuffer,
        kRCCopyBack,

        kMaxRCTypes,
    };

    struct cRCSetInfo
    {
        tTag    mTag;
        int32_t mStage;
    };

    struct cRCShaderDataInfo
    {
        tShaderDataRef  mRef;           ///< Shader data ref
        const cValue*   mConfig;        ///< TODO: this is dodgy, need cObjectValue + id, or, proper data.
    };

    struct cRCClearInfo
    {
        uint32_t mBufferFlags;
        float    mC[4];
    };

    struct cRCDrawRectInfo
    {
        float        mRect[4];
        nCL::cRGBA32 mColourAlphaU32;

        uint mFlipY : 1;
        uint mFlipX : 1;
    };

    struct cRCBlitInfo
    {
        float       mSrcRect[4];
        float       mDstRect[4];
        tTag        mSourceTag;
        uint32_t    mBufferFlags;
    };

    struct cRCDiscardInfo
    {
        GLenum mAttachment;
    };

    struct cRCCopyBackInfo
    {
        int     mXY[2];
        int     mWH[2];
        tTag    mDestTag;
    };


    class cRenderer;

    struct cRenderCommand // : public cWriteableDataStore
    {
        tRenderCommandType mType = kMaxRCTypes;

        tRenderFlags mRenderFlagMask = 0;       ///< Defines which flags we are dependent on
        tRenderFlags mRenderFlagValues = 0;     ///< Values we expect those flags to have

        union
        {
            cRCSetInfo          mSet;
            cRCShaderDataInfo   mShaderData;
            cRCClearInfo        mClear;
            cRCDrawRectInfo     mDrawRect;
            cRCBlitInfo         mBlit;
            cRCDiscardInfo      mDiscard;
            cRCCopyBackInfo     mCopyBack;
        };

        void Config(const cObjectValue* config, const cRenderer* renderer);
    };

    typedef cLink<cIRenderLayer> tAutoLayer;

    struct cLayerInfo
    {
        tAutoLayer  mLayer;
        bool        mEnabled = true;
        uint32_t    mFlags = 0;         ///< Only relevant for code-registered layers
    };

    struct cDataLayer :
        public cIRenderLayer,
        public nCL::cAllocLinkable
    {
        CL_ALLOC_LINK_DECL;

        uint32_t mID = 0;

        nCL::vector<cRenderCommand> mCommands;

        void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;

        void Config(const cValue& config, const cRenderer* renderer);
    };



    // --- cRenderer -----------------------------------------------------------

    class cRenderer :
        public cIRenderer,
        public nCL::cAllocatable
    {
    public:
        cRenderer();
        ~cRenderer();

        virtual bool Init();
        virtual bool Shutdown();

        // cIRenderer
        void SetFrameBufferInfo(uint32_t fb, int width, int height, int orientation) override;

        void Update(float dt) override;
        void Render() override;

        void RegisterCamera(tTag cameraTag, cICamera* camera) override;
        cICamera* Camera(tTag cameraTag) const override;

        // Layers
        void LoadLayersAndBuffers(const nCL::cObjectValue* config) override;
        void RegisterLayer(tTag layerTag, cIRenderLayer* dispatcher, uint32_t flags) override;
        cIRenderLayer* Layer(tTag layerTag) const override;

        tTag SetRenderLayer(tTag layerTag) override;
        void AddRenderJob(tTag jobGroupTag, tTag layerTag) override;

        void SetRenderFlag(int flag, bool enabled) override;
        bool RenderFlag(int flag) const override;
        void SetRenderFlags(const cObjectValue* v) override;

        // Materials
        void LoadMaterials(const nCL::cObjectValue* config) override;

        int  MaterialRefFromTag(tTag id) override;
        bool SetMaterial(int ref) override;

        // Textures
        void LoadTextures(const cObjectValue* config) override;

        tTextureRef CreateTexture (tTag textureTag, tBufferFormat format, int w, int h, const uint8_t* data, const cObjectValue* config) override;
        bool        DestroyTexture(tTextureRef ref) override;

        int  TextureRefFromTag(tTag textureTag) override;

        void SetTextures(const cObjectValue* object) override;
        void SetTexture(tTextureKind kind, int ref) override;
        void SetTexture(tTag target, int ref) override;

        void UpdateTexture(tTextureRef ref, tBufferFormat format, const void* data) override;

        // Shader data
        void LoadShaderData(const nCL::cObjectValue* config) override;

        void        SetShaderData (const nCL::cObjectValue* object) override;
        bool        SetShaderData (tShaderDataRef ref, size_t dataSize, const void* data) override;
        const void* ShaderData    (tShaderDataRef ref) const override;
        size_t      ShaderDataSize(tShaderDataRef ref) const override;

        tShaderDataRef ShaderDataRefFromTag(tTag tag) const override;
        tShaderDataRef AddShaderData(tTag tag) override;

        void SetShaderDataUpdate(tShaderDataRef ref, tShaderDataUpdateFunc f, int numDeps, const tShaderDataRef deps[]) override;
        void SetShaderDataConfig(tShaderDataRef ref, tShaderDataConfigFunc f) override;

        // Copy-back buffers
        const cAllocImage32* CopyBackBuffer(tTag tag, uint32_t* updateCount) override;
        uint32_t UpdateCountForCopyBackBuffer(tTag tag) override;

        // Render
        int  CreateQuadMesh (int numQuads, int numElts, cEltInfo elts[]) override;    ///< Create a mesh of the given vertex format to be used in quad rendering
        void DestroyQuadMesh(int quadMesh) override;

        int  GetQuadBuffer           (int quadMesh, int count, uint8_t** buffer) override;    ///< Get a buffer of up to 'count' quads to fill. The actual buffer size is returned.
        void DispatchAndReleaseBuffer(int quadMesh, int numQuads) override;                   ///< Draw the given buffer and release once done.

        void DrawBuffer(uint32_t mode, int numElts, cEltInfo elts[], int count, const void* buffer) override;

        void DrawMesh(const cGLMeshInfo* meshInfo) override;

        void PushState(const char* debugTag) override;
        void PopState (const char* debugTag) override;

        // cRenderer
        int  RenderFlagFromTag(nCL::tTag tag) const;

        void ApplyCommand(const cRenderCommand& command, const cRenderLayerState& state);
        
    #ifndef CL_RELEASE
        void DebugMenu(cUIState* uiState) override;
    #endif

        void SetStateForDispatch();  ///< set up state for current material given current shader data

    protected:
        // Utilities
        void LoadRenderBuffers(const cObjectValue* config);
        void LoadFrameBuffers(const cObjectValue* config);
        void LoadLayers(const cObjectValue* config);

        const cLayerInfo* LayerInfo(tTag layerTag) const;

        bool    DispatchJobGroup(tTag jobGroupTag, const cRenderLayerState& state);
        bool    DispatchLayer(tTag layerTag, const cRenderLayerState& state, const char* label);

        void    ResetState();

        void    GetBindingsFromProgram(GLuint program, nCL::vector<cShaderDataBinding>* bindings);
        void    UploadShaderData(GLuint programID, const nCL::vector<cShaderDataBinding>& bindings);
        void*   ShaderData(tShaderDataRef ref);

        void    GetBindingsFromProgram(GLuint program, nCL::vector<cTextureBinding>* bindings);
        void    BindTextures(GLuint programID, const nCL::vector<cTextureBinding>& bindings);


        // Data definitions
        typedef cLink<cICamera> tAutoCamera;
        typedef nCL::map<tTag, int>   tTagToIndexMap;

        // Data
        cIAllocator* mAllocator = 0;

        nCL::map<tTag, tAutoCamera>     mCameras;

        nCL::map<tTag, cLayerInfo>      mCodeLayers;
        nCL::map<tTag, cLayerInfo>      mDataLayers;

        tTag                            mRenderLayer = kMainTag;
        nCL::multimap<tTag, tTag>       mRenderJobs;

        tTagToIndexMap                  mRenderBufferTagToIndex;
        nCL::vector<cTextureInfo>       mRenderBufferInfo;

        tTagToIndexMap                  mFrameBufferTagToIndex;
        nCL::vector<cFrameBufferInfo>   mFrameBufferInfo;
        uint                            mDefaultFrameBuffer = 0;
        uint                            mCurrentFrameBuffer = 0;

        tRenderFlags                    mRenderFlags = 0;
        cRenderStateInfo                mRenderState;

        tTagToIndexMap                  mRenderFlagTags;

        // Keeping materials & textures permanently loaded for now.
        tTagToIndexMap                  mMaterialTagToIndex;
        nCL::vector<cMaterial>          mMaterials;

        tTagToIndexMap                  mTextureTagToIndex;
        nCL::vector<cTextureInfo>       mTextureInfo;

        tTagToIndexMap                  mKindTagToTextureIndex;

        // Shader data naming etc.
        tTagToIndexMap                  mShaderDataTagToIndex;
        nCL::vector<cShaderDataInfo>    mShaderData;   // Current shader data
        nCL::cWriteableDataStore        mShaderDataStore;

        // Copy-back buffers
        tTagToIndexMap                  mCopyBackBufferTagToIndex;
        nCL::vector<cCopyBackBufferInfo> mCopyBackBuffers;

        // General state management
        nCL::vector<cShaderDataInfo>    mSavedStates;
        nCL::vector<int>                mStateMarkers;

        tMaterialRef                    mCurrentMaterial = -1;
        int                             mDeviceOrientation = 0;
        Mat3f                           mDeviceOrient = vl_I;
        cLink<cICamera>                 mCurrentCamera;
        nCL::vector<nCL::cLink<cICamera>>  mCameraStack;

        // Built-in shader data
        float mTime  = 0.0f;
        float mPulse = 0.0f;

        // Quad mesh support
        nCL::cSlotArrayT<cQuadMesh> mQuadMeshSlots;

        // Dev support
        nCL::cFileWatcher mDocumentsWatcher;    ///< For shader reloading
        
        nCL::multimap<int, int> mRefToMaterialIndexMap;
        nCL::set<int>           mMaterialsToReload;
        int                     mMaterialReloadCounter = 0;
    };


    // --- Inlines -------------------------------------------------------------

    inline void cRenderer::SetRenderFlag(int flag, bool enabled)
    {
        CL_INDEX(flag, kMaxRenderFlags);

        tRenderFlags flagSet = (1 << flag);

        if (enabled)
            mRenderFlags |= flagSet;
        else
            mRenderFlags &= ~flagSet;
    }

    inline bool cRenderer::RenderFlag(int flag) const
    {
        CL_INDEX(flag, kMaxRenderFlags);

        return ((1 << flag) & mRenderFlags) != 0;
    }

    inline const void* cRenderer::ShaderData(tShaderDataRef ref) const
    {
        CL_INDEX(ref, mShaderData.size());
        return mShaderDataStore.Data(mShaderData[ref].mOffset);
    }

    inline size_t cRenderer::ShaderDataSize(tShaderDataRef ref) const
    {
        CL_INDEX(ref, mShaderData.size());
        return mShaderData[ref].mSize;
    }

    inline void* cRenderer::ShaderData(tShaderDataRef ref)
    {
        CL_INDEX(ref, mShaderData.size());
        return mShaderDataStore.Data(mShaderData[ref].mOffset);
    }
}

#endif
