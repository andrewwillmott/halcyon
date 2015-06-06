//
//  File:       HLEffectsManager.h
//
//  Function:   Manager for audiovisual effects.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECTS_MANAGER_H
#define HL_EFFECTS_MANAGER_H

#include <IHLEffectsManager.h>

#include <HLEffectType.h>

#include <CLLink.h>
#include <CLMemory.h>
#include <CLParams.h>
#include <CLRandom.h>

namespace nHL
{
    class cIEffectType;

    // --- cEffectsManager -----------------------------------------------------

    class cEffectParamsAlloc : public cEffectParams, public nCL::cAllocLinkable
    {};

    struct cEffectInstance
    {
        struct cFlags
        {
            bool mOneShot  : 1;     ///< Play once, and tidy up once done.
            bool mPaused   : 1;     ///< True if the effect is paused
            bool mRealTime : 1;     ///< Keep running even if game paused
            bool mHidden   : 1;
        };

        tTag            mTag        = 0;
        cFlags          mFlags      = { 0 };
        tEffectType     mType       = kMaxEffectTypes;
        tEIRef          mRef;
        cTransform      mSourceTransform;
        cTransform      mEffectTransform;

        cLink<cEffectParamsAlloc> mParams;

        cEffectInstance() {}    // required for default initialisation? wtf?
    };

    class cEffectsManager :
        public cIEffectsManager,
        public nCL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        cEffectsManager();
        ~cEffectsManager();

        // cIEffectsManager
        bool Init() override;
        bool PostInit() override;
        bool Shutdown() override;

        bool LoadEffects(const cObjectValue* config) override;

        void Update(float realDT, float gameDT) override;

        const cEffectsManagerParams* Params() const override;
              cEffectsManagerParams* Params() override;

        tEIRef CreateInstance (tTag tag) override;
        bool   DestroyInstance(tEIRef ref) override;
        tEIRef CreateOneShotInstance(tTag tag) override;
        bool   DestroyInstanceOnStop(tEIRef ref) override;
        void   RemoveAllInstances() override;

        void              SetSourceTransform(tEIRef ref, const cTransform& xform) override;
        const cTransform& SourceTransform   (tEIRef ref) const override;
        void              SetEffectTransform(tEIRef ref, const cTransform& xform) override;
        const cTransform& EffectTransform   (tEIRef ref) const override;

        void StartSources(tEIRef ref) override;
        void StopSources (tEIRef ref) override;
        void StartEffect (tEIRef ref) override;
        void StopEffect  (tEIRef ref) override;
        bool IsActive    (tEIRef ref) const override;

        void SetVisible  (tEIRef ref, bool enabled) override;
        bool Visible     (tEIRef ref) const override;
        void SetPaused   (tEIRef ref, bool enabled) override;
        bool Paused      (tEIRef ref) const override;
        void SetRealTime (tEIRef ref, bool realTime) override;
        bool RealTime    (tEIRef ref) const override;

        void CreateInstances (int count, const tTag tags[], tEIRef effectRefs[]) override;
        void DestroyInstances(int count, tEIRef effectRefs[]) override;

        nCL::cParams*   Params(tEIRef ref) override;
        void            AddData(tEIRef ref, cIEffectData* data) override;

        tTag                EffectTag   (tEIRef ref);
        const cObjectValue* EffectConfig(tEIRef ref);

        void RegisterPhysicsController(tTag tag, cIPhysicsController* controller) override;
        cIPhysicsController* PhysicsController(tTag tag) const override;

        void          RegisterEffectType(tEffectType type, cIEffectType* manager, tTag tag, tTag setTag) override;
        cIEffectType* EffectType        (tEffectType type) override;
        tTag          EffectTypeTag     (tEffectType type) override;
        tEffectType   EffectTypeFromTag(tTag tag) override;

        const char* StatsString() const override;
        void DebugMenu(cUIState* uiState) override;

    protected:
        // Data decls
        typedef nCL::map<tTag, cLink<cIPhysicsController>> tTagToControllerMap;

        // Data
        cIAllocator*                mAllocator;

        cEffectsManagerParams       mParams;
        nCL::tSeed32                mSeed = nCL::kDefaultSeed32; // Master seed

        // Top-level effects
        nCL::cSlotArray             mInstanceSlots;
        nCL::vector<cEffectInstance> mInstanceEffects;

        // Effect type management
        cLink<cIEffectType>         mEffectTypes   [kMaxEffectTypes];
        tTag                        mEffectTypeTags[kMaxEffectTypes] = { 0 };
        tTag                        mEffectSetTags [kMaxEffectTypes] = { 0 };

        // Extras
        tTagToControllerMap         mPhysicsControllers;

        // Debug/Info
        bool mEnabled = true;
        bool mShowStats = false;

        float mOverallMSPF  = 0.0f;
        float mPreMSPF      = 0.0f;
        float mPostMSPF     = 0.0f;
        float mTypeMSPF[kMaxEffectTypes] = { 0.0f };

        mutable nCL::string mStats;
    };


    // --- Inlines -------------------------------------------------------------

    inline const cEffectsManagerParams* cEffectsManager::Params() const
    {
        return &mParams;
    }
    inline cEffectsManagerParams* cEffectsManager::Params()
    {
        return &mParams;
    }

    inline nCL::cParams* cEffectsManager::Params(tEIRef ref)
    {
        if (mInstanceSlots.InUse(ref))
            return mInstanceEffects[ref].mParams;

        return 0;
    }

    inline cIEffectType* cEffectsManager::EffectType(tEffectType type)
    {
        CL_INDEX(type, kMaxEffectTypes);
        return mEffectTypes[type];
    }
}

#endif
