//
//  File:       HLEffectType.h
//
//  Function:   Defines management for a particular kind of effect
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_TYPE_H
#define HL_EFFECT_TYPE_H

#include <IHLEffectsManager.h>

#include <CLMemory.h>
#include <CLParams.h>
#include <CLSlotArray.h>

namespace nHL
{
    class cIRenderer;


    // --- cEffectParams -------------------------------------------------------

    class cEffectParams : public nCL::cParams
    {
    public:
        // Effect use
        bool        HasData() const;          ///< Returns true if any parameters are set
        int         NumData() const;

        const cIEffectData* Data(int i) const;

        // Manager only
        void        AddData(const cIEffectData* data);
        void        ClearData();

    protected:
        nCL::vector<nCL::cLink<const cIEffectData>> mData;
    };


    // --- cIEffectType --------------------------------------------------------

    class cIEffectType
    {
    public:
        virtual int  Link(int count) const = 0;

        virtual void Init(cIEffectsManager* manager, cIAllocator* alloc) = 0;
        virtual void PostInit() = 0;
        virtual void Shutdown() = 0;

        virtual void Config(const cObjectValue* config) = 0;  ///< Configure all effects of this type from the given object.

        virtual void PreUpdate (float realDT, float gameDT) = 0;    ///< Called before all effects are ticked
        virtual void PostUpdate(float realDT, float gameDT) = 0;    ///< Called after all effects are ticked

        virtual bool HasEffect(tTag tag) const = 0;    ///< Returns true if given effect exists and can be created.

        virtual tEIRef  CreateInstance (tTag tag) = 0;
        virtual bool    DestroyInstance(tEIRef ref) = 0;

        virtual int     NumInstances() const = 0;   // returns number of instances
        virtual int     NumActiveInstances() const = 0;   // returns number of active instances

        // Instance management
        virtual void SetTransforms(tEIRef ref, const cTransform& sourceToEffect, const cTransform& effectToWorld) = 0;
        virtual void Start        (tEIRef ref, tTransitionType transition) = 0;
        virtual void Stop         (tEIRef ref, tTransitionType transition) = 0;
        virtual bool IsActive     (tEIRef ref) const = 0;
        virtual bool Update       (tEIRef ref, float dt, const cEffectParams* params) = 0;

        virtual const char* StatsString(const char* typeName) const = 0;    ///< Return stats about this effect type or 0 if none
        virtual void DebugMenu(cUIState* uiState) = 0;      ///< Display debug menu
    };


    // --- cEffectType*<> ------------------------------------------------------

    // The following are standard implemenations for an effect type for convenience only. These
    // can be extended or replaced entirely.

    class cEffectTypeBase :
        public cIEffectType,
        public cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;
        
        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void PostInit() override;
        void Shutdown() override;

        void PreUpdate (float realDT, float gameDT) override;
        void PostUpdate(float realDT, float gameDT) override;

        // cEffectType
        cIEffectsManager*   Manager() const;
        cIRenderer*         Renderer() const;
        cIAllocator*        Allocator() const;

    protected:
        // Data decls
        typedef nCL::map<tTag, int> tTagToIndexMap;

        // Data
        cIEffectsManager*           mManager = 0;
        cIRenderer*                 mRenderer = 0;
        cIAllocator*                mAllocator = 0;

        tTagToIndexMap              mTagToIndex;
        nCL::cSlotArray             mSlots;

        // Hotload support only
        nCL::vector<tTag>           mEffectTags;

    #ifndef CL_RELEASE
        mutable nCL::string         mStats;
        bool                        mEnabled = true;
    #endif
    };

    // Standard implementation of an effect type
    template<class T_DESC, class T_EFFECT> struct cEffectType : public cEffectTypeBase
    {
    public:
        void Shutdown() override;

        void Config(const cObjectValue* config) override;

        bool HasEffect(tTag tag) const override;

        tEIRef CreateInstance (tTag tag) override;
        bool   DestroyInstance(tEIRef ref) override;

        int NumInstances() const override;
        int NumActiveInstances() const override;

        void SetTransforms(tEIRef ref, const cTransform& sourceToEffect, const cTransform& effectToWorld) override;
        void Start        (tEIRef ref, tTransitionType transition) override;
        void Stop         (tEIRef ref, tTransitionType transition) override;
        bool IsActive     (tEIRef ref) const override;
        bool Update       (tEIRef ref, float dt, const cEffectParams* params) override;

        const char* StatsString(const char* typeName) const override;
        void DebugMenu(cUIState* uiState) override;

        // cEffectType
        void RemoveAllInstances();

    protected:
        // Data
        nCL::vector<T_DESC>         mDescs;
        nCL::vector<cLink<T_EFFECT>> mEffects;
    };

    // Alternate implementation of an effect type that stores instances by value
    template<class T_DESC, class T_EFFECT> struct cEffectTypeValue : public cEffectTypeBase
    {
    public:
        void Shutdown() override;

        void Config(const cObjectValue* config) override;

        bool HasEffect(tTag tag) const override;

        tEIRef CreateInstance (tTag tag) override;
        bool   DestroyInstance(tEIRef ref) override;

        int NumInstances() const override;
        int NumActiveInstances() const override;

        void SetTransforms(tEIRef ref, const cTransform& sourceToEffect, const cTransform& effectToWorld) override;
        void Start        (tEIRef ref, tTransitionType transition) override;
        void Stop         (tEIRef ref, tTransitionType transition) override;
        bool IsActive     (tEIRef ref) const override;
        bool Update       (tEIRef ref, float dt, const cEffectParams* params) override;

        const char* StatsString(const char* typeName) const override;
        void DebugMenu(cUIState* uiState) override;

        void RemoveAllInstances();

    protected:
        // Data
        nCL::vector<T_DESC>         mDescs;
        nCL::vector<T_EFFECT>       mEffects;
    };


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectParams::HasData() const
    {
        return !mData.empty();
    }

    inline int  cEffectParams::NumData() const
    {
        return mData.size();
    }

    inline const cIEffectData* cEffectParams::Data(int i) const
    {
        return mData[i];
    }

    inline void cEffectParams::AddData(const cIEffectData* data)
    {
        mData.push_back(data);
    }

    inline void cEffectParams::ClearData()
    {
        return mData.clear();
    }

    inline cIEffectsManager* cEffectTypeBase::Manager() const
    {
        return mManager;
    }

    inline cIRenderer* cEffectTypeBase::Renderer() const
    {
        return mRenderer;
    }

    inline cIAllocator* cEffectTypeBase::Allocator() const
    {
        return mAllocator;
    }
}

#endif
