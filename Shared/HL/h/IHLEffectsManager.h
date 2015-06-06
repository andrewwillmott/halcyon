//
//  File:       IHLEffectsManager.h
//
//  Function:   Manager for audiovisual effects.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_EFFECTS_MANAGER_H
#define HL_I_EFFECTS_MANAGER_H

#include <HLDefs.h>
#include <CLSlotRef.h>
#include <CLTransform.h>

namespace nCL
{
    class cObjectValue;
    class cParams;
}

namespace nHL
{
    class cUIState;

    typedef nCL::cSlotRef tEIRef;   // effect instance reference

    typedef uint8_t tEffectTypeStore;
    enum tEffectType : tEffectTypeStore
    {
        kMaxInternalEffectTypes = 32,
        kMaxExternalEffectTypes = 32,

        kEffectGroup            = 0,
        kEffectParticles,
        kEffectRibbon,
        kEffectSprite,
        kEffectShake,
        kEffectScreen,
        kEffectSound,
        kEffectModel,
        kEffectExample,

        kEffectExternalFirst = kMaxInternalEffectTypes,
        kEffectExternalLast  = kEffectExternalFirst + kMaxExternalEffectTypes - 1,

        kMaxEffectTypes
    };

    enum tTransitionType : uint8_t
    {
        kTransitionSource,      ///< Turn on/off sources
        kTransitionImmediate,   ///< Turn on/off effect as a whole -- e.g., start in steady state, or immediately stop everything.
        kMaxTransitions
    };

    enum tEffectParams : uint8_t
    {
        kEffectParamColour,             ///< Scale effect colour
        kEffectParamAlpha,              ///< Scale effect alpha
        kEffectParamSize,               ///< Scale effect entity size (so, scale individual particles rather than entire effect)

        kEffectParamEmitColour,         ///< Scale colour of newly created entities, Vec3f(1.0f)
        kEffectParamEmitAlpha,          ///< Scale alpha of newly created entities, 1.0f
        kEffectParamEmitRate,           ///< Scale creation rate, 1.0f
        kEffectParamEmitSpeed,          ///< Scale created entity speed, 1.0f
        kEffectParamEmitLife,           ///< Scale created entity lifetime
        kEffectParamEmitSize,           ///< Scale created entity size
        kEffectParamEmitVolume,         ///< Set emit volume, cBounds3.

        kEffectParamAttractor,          ///< Set location of particle attractor, Vec3f.

        kEffectParamTextureOverride,    ///< Resource ID of override texture, uint32(0).

        kEffectParamSetFrame,           ///< Particle frame to start with, 0. int32(0).
        kEffectParamAnimSpeed,          ///< Scale on animation speed, 1.0f

        kEffectParamShader1,            ///< Shader parameter to be used in rendering, Vec4f(0.0f)
        kEffectParamShader2,            ///< Shader parameter to be used in rendering, Vec4f(0.0f)
        kEffectParamShader3,            ///< Shader parameter to be used in rendering, Vec4f(0.0f)
        kEffectParamShader4,            ///< Shader parameter to be used in rendering, Vec4f(0.0f)

        kMaxEffectParams
    };

    class cIEffectData
    {
    public:
        virtual void* AsInterface(uint32_t iid) = 0;
        virtual int   Link(int delta) const = 0;
    };

    struct cEffectsManagerParams
    {
        struct cFlags
        {
            bool mDebugNoAlpha       : 1; ///< Disable alpha to show fill rate
            bool mDebugBoundingBoxes : 1; ///< Show effect bounds
        };

        cFlags mFlags = { 0 };

        // Camera
        nCL::cTransform mCameraToWorld;             ///< Camera location and orientation
        Mat4f           mProjectionMatrix = vl_I;   ///< World to clip space transform
        Vec2f           mViewOffset = vl_0;         ///< How much camera should be offset by

        // External forces
        Vec3f       mWindDirection = vl_x;
        Vec3f       mSunDirection  = vl_z;

        // Global transform
        nCL::cTransform mTransform;                 ///< Global transform for all effects
    };

    class cIPhysicsController
    /// Interface for controlling particles or other point-based entities
    {
    public:
        virtual int Link(int count) const = 0;

        virtual void Update
        (
            const cTransform& effectToWorld,
            int count,
            const float dts[],  size_t dtStride,
            Vec3f positions[],  size_t positionStride,
            Vec3f velocities[], size_t velocityStride
        ) = 0;
        ///< Called to apply the controller to the given points
    };

    class cIEffectType;


    /// --- cIEffectsManager ---------------------------------------------------

    class cIEffectsManager
    {
    public:
        virtual int Link(int count) const = 0;

        virtual bool Init() = 0;
        virtual bool PostInit() = 0;    ///< Called once all systems have been configured for the first time.
        virtual bool Shutdown() = 0;

        virtual bool LoadEffects(const nCL::cObjectValue* config) = 0;

        virtual void Update(float realDT, float gameDT) = 0;

        virtual const cEffectsManagerParams* Params() const = 0;    ///< Parameters to control the effects manager
        virtual       cEffectsManagerParams* Params() = 0;          ///< Parameters to control the effects manager

        virtual tEIRef CreateInstance (tTag effectTag) = 0;         ///< Creates an effect instance and returns a reference to it.
        virtual bool   DestroyInstance(tEIRef ref) = 0;             ///< Releases the given instance. Returns true if the instance was destroyed. Any calls with tEIRef after this point will be silently ignored.

        virtual tEIRef CreateOneShotInstance(tTag effectTag) = 0;   ///< Creates an effect instance that will be auto-destroyed once it stops.
        virtual bool   DestroyInstanceOnStop(tEIRef ref) = 0;       ///< Destroys effect once it has stopped -- effectively converts it to a "one shot" effect.

        virtual void   RemoveAllInstances() = 0;                    ///< Use to remove all orphaned instances

        virtual void              SetSourceTransform(tEIRef ref, const nCL::cTransform& xform) = 0;   ///< Sets source transform for given effect.
        virtual const cTransform& SourceTransform   (tEIRef ref) const = 0; ///< Returns current source transform

        virtual void              SetEffectTransform(tEIRef ref, const nCL::cTransform& xform) = 0;   ///< Sets effect-as-a-whole transform for given effect
        virtual const cTransform& EffectTransform   (tEIRef ref) const = 0; ///< Returns current effect transform

        virtual void StartSources(tEIRef ref) = 0;          ///< Starts effect sources
        virtual void StopSources (tEIRef ref) = 0;          ///< Stops effect sources. The effect will self-stop automatically. (E.g., when there are no particles left alive.)
        virtual void StartEffect (tEIRef ref) = 0;          ///< Starts given effect in its steady state.
        virtual void StopEffect  (tEIRef ref) = 0;          ///< Stops given effect immediately.
        virtual bool IsActive    (tEIRef ref) const = 0;    ///< Returns true if effect is still active.

        virtual void SetVisible  (tEIRef ref, bool enabled) = 0;  ///< Sets visibility for given effect, returns previous visibility.
        virtual bool Visible     (tEIRef ref) const = 0;          ///< Returns current visibility
        virtual void SetPaused   (tEIRef ref, bool enabled) = 0;  ///< Sets whether given effect is paused, returns previous pause status.
        virtual bool Paused      (tEIRef ref) const = 0;          ///< Returns current pause status
        virtual void SetRealTime (tEIRef ref, bool realTime) = 0; ///< Sets whether to use real time or game time.
        virtual bool RealTime    (tEIRef ref) const = 0;          ///< Returns current real time setting.

        // 'Bulk' effect APIs
        virtual void CreateInstances (int count, const tTag effectTags[], tEIRef effectRefs[]) = 0; ///< Creates effects in bulk
        virtual void DestroyInstances(int count, tEIRef effectRefs[]) = 0;                          ///< Destroys effects in bulk

        virtual nCL::cParams* Params (tEIRef ref) = 0;                      ///< Returns parameters associated with an effect for setting/getting. E.g., Params(ref)->SetParam(kEffectParamRate, 1.0f);
        virtual void          AddData(tEIRef ref, cIEffectData* data) = 0;  ///< Add given data (usually positions etc. for creating new objects) to the given effect.

        virtual tTag                EffectTag   (tEIRef ref) = 0;
        virtual const cObjectValue* EffectConfig(tEIRef ref) = 0;

        // Physics plugins
        virtual void RegisterPhysicsController(tTag tag, cIPhysicsController* controller) = 0;  ///< Register given controller under given id
        virtual cIPhysicsController* PhysicsController(tTag tag) const = 0;                     ///< Returns controller for given id or 0 if none

        // Effects type management
        virtual void          RegisterEffectType(tEffectType type, cIEffectType* manager, tTag tag, tTag setTag) = 0;     ///< Register manager for the given effect type
        virtual cIEffectType* EffectType        (tEffectType type) = 0;     ///< Returns manager for the given effect type
        virtual tTag          EffectTypeTag     (tEffectType type) = 0;     ///< Returns tag for the given effect type

        virtual tEffectType   EffectTypeFromTag(tTag tag) = 0;  ///< Returns effect type for the given tag, or kMaxEffectTypes if none.

        // Debug/profile
        virtual const char* StatsString() const = 0;
        virtual void DebugMenu(cUIState* uiState) = 0;
    };

    cIEffectsManager* CreateEffectsManager(nCL::cIAllocator* alloc);
}

#endif
