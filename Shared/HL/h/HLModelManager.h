//
//  File:       HLModelManager.h
//
//  Function:   Model management: rendering & display
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_MODEL_MANAGER_H
#define HL_MODEL_MANAGER_H

#include <IHLModelManager.h>
#include <IHLRenderer.h>
#include <HLGLUtilities.h>

#include <CLLink.h>
#include <CLMemory.h>
#include <CLSlotArray.h>
#include <CLSTL.h>
#include <CLTransform.h>
#include <CLVecUtil.h>

namespace nCL
{
    class cValue;
    class cObjectValue;
}

namespace nHL
{
    typedef cLink<const nCL::cObjectValue> tModelConfig;

    struct cModel
    {
        tTag         mTag = kNullTag;    //!< Tag of this model
        tModelConfig mConfig;            //!< Config that defines this model

        cGLMeshInfo mMeshLOD0;
        cTransform  mMeshTransform;         ///< Internal transform from config, applied to referenced meshes before the model transform
        cBounds3    mBounds;                ///< Overall AABB bounds in model space. (Not mesh space.)
        float       mBoundingRadius = 1.0f; ///< Spherical bounds with respect to the origin in model space. (Not mesh space.)
        tTag        mMaterialTag   = kNullTag;
        int         mMaterialIndex = -1;
    };

    enum tMIFlags : uint32_t
    {
        kMIFlagHidden = 1,
        kMaxMIFlags
    };
    typedef uint32_t tMIFlagSet;

    class cModelManager :
        public cIModelManager,
        public cIRenderLayer,
        public nCL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        cModelManager();
        ~cModelManager();

        // cIModelManager
        bool Init() override;
        bool Shutdown() override;

        bool LoadModels(const nCL::cObjectValue* config) override;

        void Update(float dt) override;

        tMIRef CreateInstance (tTag tag) override;
        bool   DestroyInstance(tMIRef ref) override;

        void   RemoveAllInstances() override;

        bool                SetTransform(tMIRef ref, const cTransform& xform) override;
        const cTransform&   Transform   (tMIRef ref) const override;

        bool                SetVisible  (tMIRef ref, bool enabled) override;
        bool                Visible     (tMIRef ref) const override;

        void   CreateInstances (int count, const tTag modelTags[], tMIRef modelRefs[]) override;
        void   DestroyInstances(int count, tMIRef modelTags[]) override;

        tTag                ModelTag    (tMIRef ref) const override;
        const cObjectValue* ModelConfig (tMIRef ref) const override;

        cIRenderLayer* AsLayer() override;

        // cIRenderLayer
        void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;
        ///< Draw all models according to state

        // cModelManager
    protected:
        // Data definitions
        typedef nCL::map<tTag, int> tTagToIndexMap;

        // Data
        tTagToIndexMap              mModelTagToIndex;
        nCL::vector<cModel>         mModels;

        nCL::cSlotArray             mInstanceSlots;
        nCL::vector<cTransform>     mInstanceTransforms;
        nCL::vector<tMIFlagSet>     mInstanceFlags;
        nCL::vector<int>            mInstanceModelIndex;

        cTransform                  mNullTransform;
    };


    inline bool cModelManager::Visible(tMIRef ref) const
    {
        if (mInstanceSlots.InUse(ref))
            return !(mInstanceFlags[ref] & kMIFlagHidden);

        return false;
    }

    inline const cTransform& cModelManager::Transform(tMIRef ref) const
    {
        if (mInstanceSlots.InUse(ref))
            return mInstanceTransforms[ref];

        return mNullTransform;
    }

    inline tTag cModelManager::ModelTag(tMIRef ref) const
    {
        if (mInstanceSlots.InUse(ref))
            return mModels[mInstanceModelIndex[ref]].mTag;
        return kNullTag;
    }
    inline const cObjectValue* cModelManager::ModelConfig(tMIRef ref) const
    {
        if (mInstanceSlots.InUse(ref))
            return mModels[mInstanceModelIndex[ref]].mConfig;
        return nullptr;
    }

    inline cIRenderLayer* cModelManager::AsLayer()
    {
        return this;
    }
}


#endif
