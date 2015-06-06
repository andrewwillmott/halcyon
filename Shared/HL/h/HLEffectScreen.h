//
//  File:       HLEffectScreen.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_SCREEN_H
#define HL_EFFECT_SCREEN_H

#include <IHLEffectsManager.h>
#include <IHLRenderer.h>
#include <HLAnimUtils.h>

#include <CLColour.h>
#include <CLLink.h>

namespace nHL
{
    class cIEffectType;
    class cIRenderer;

    enum tScreenMode : uint8_t
    {
        kScreenAdditive,
        kScreenBlend,
        kScreenTint,                        ///< Multiply
        kScreenBackground,                  ///< Drawn behind everything else
        kScreenSkybox,                      ///< Environment cube map
        kMaxScreenModes
    };

    struct cDescScreen
    {
        struct cFlags
        {
            bool mLoop    : 1;
            bool mSustain : 1;
        };

        cFlags mFlags = { 0 };

        tScreenMode mMode = kScreenBlend;

        float mLife  = 2.0f;
        float mDelay = 0.0f;
        int   mSort  = 0;

        nCL::vector<Vec3f> mColourFrames;
        nCL::vector<float> mAlphaFrames;

        nCL::tTag mTextureTag = 0;

        void Config(const cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };

    struct cEffectScreen
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescScreen* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const nCL::cParams* params);

    //protected:
        struct cFlags
        {
            bool mActive    : 1;
            bool mVisible   : 1;
        };

        cFlags mFlags = { 0 };

        const cDescScreen* mDesc = 0;

        tPtAge mAge = 0;
        tPtAge mAgeStep = 0;

        cColourAlpha mColourAlpha;
        tTextureRef mTextureRef = kNullTextureRef;
    };


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectScreen::IsActive() const
    {
        return mFlags.mActive;
    }
}

#endif
