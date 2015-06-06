//
//  File:       HLCamera.h
//
//  Function:   Camera support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_CAMERA_H
#define HL_CAMERA_H

#include <IHLRenderer.h>

#include <CLBounds.h>
#include <CLLink.h>
#include <CLMemory.h>
#include <CLTransform.h>

namespace nHL
{
    class cIRenderer;
    class cUIState;


    //--- cCamera ------------------------------------------------------

    class cCamera :
        public cICamera,
        public cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        // cICamera
        void Config(const cObjectValue* value) override; ///< Configure the camera from the given object value
        void Dispatch(cIRenderer* renderer) override;    ///< Set camera shader data on renderer

        void              SetCameraToWorld(const cTransform&) override;  ///< Set camera's current world space transform.
        const cTransform& CameraToWorld() override;  ///< Get camera's current world space transform.

        void SetSceneBounds(const cBounds3& bounds);

        void FindView          (Mat4f* v) override;
        void FindProjection    (Vec2f viewSize, Mat4f* p) override;
        void FindViewProjection(Vec2f viewSize, Mat4f* vp) override;

        // cCamera
    protected:
        cTransform mCameraToWorld;  ///< Camera location
        cBounds3   mSceneBounds = { vl_0, vl_1 };

        float mNear = 1.0f;
        float mFar  = 10000.0f;
    };

    class cOrthoCamera : public cCamera
    {
    public:

        // cICamera
        void Config(const cObjectValue* value) override;
        void FindProjection(Vec2f viewSize, Mat4f* p) override;

        // cOrthoCamera
        void     SetFrustum(const cBounds3& frustum);
        cBounds3 Frustum();

    protected:
        float mLeft     = -1.0f;
        float mRight    = +1.0f;
        float mBottom   = -1.0f;
        float mTop      = +1.0f;
    };


    //--- cStandardCamera ------------------------------------------------------

    class cStandardCamera : public cCamera
    /// Standard camera model -- camera is aimed at a target, with heading/pitch/distance relative to that target
    {
    public:
        // cICamera

        void Config(const cObjectValue* value) override; ///< Configure the camera from the given object value
        void Dispatch(cIRenderer* renderer) override;    ///< Set camera shader data on renderer

        void FindView          (Mat4f* v) override;
        void FindProjection    (Vec2f viewSize, Mat4f* p) override;
        void FindViewProjection(Vec2f viewSize, Mat4f* vp) override;

        // cStandardCamera
        void Reset();
        void SetDepthClip(float near, float far);

    protected:
        void UpdateCameraToWorld();

        // Data
        bool     mZUp               = true;
        float    mCameraTheta       = 0.0f;
        float    mCameraPhi         = 0.0f;
        float    mCameraDistance    = 500.0f;
        float    mCameraOffsetX     = 0.0f;
        float    mCameraOffsetY     = 0.0f;
        float    mCameraFOV         = 46.0f;
    };

    class cSimpleCamera : public cStandardCamera
    {
    public:
        enum tMode
        {
            kModeRotate,
            kModePanXY,
            kModeZoomX,
            kModeZoomY,
            kModeZoomXY,
            kModeNone
        };

        void Config(const cObjectValue* value); ///< Configure the camera from the given object value
        void Reset();

        void SetRelativeDistance(float d);      ///< Sets mCameraDistance relative to scene bounds

        // Generic drag API.
        void StartModalDrag();
        void UpdateModalDrag(tMode mode, float dx, float dy);        ///< Apply x/y delta using the given mode

    protected:
        // dragging internals
        float    mStartCameraTheta      = 0.0f;
        float    mStartCameraPhi        = 0.0f;
        float    mStartCameraDistance   = 0.0f;
        float    mStartCameraOffsetX    = 0.0f;
        float    mStartCameraOffsetY    = 0.0f;

        Vec3f    mAccelerometer         = -Vec3f(vl_y);
    };

    // Utilities
    void ApplyPostClipOps(cIRenderer* renderer, Mat4f* clipInOut);
    ///< Apply things like view offsets and device orientation to the given transform to clip space.

    void HandleStandardInput(cUIState* uiState, cSimpleCamera* camera);
    ///< Standard 'object-based' camera UI update -- mouse rotates about view-at point, modifiers allow shift/zoom, etc.
    void HandleStandardInput(cUIState* uiState, cOrthoCamera* camera, Vec2f scale = Vec2f(vl_1));
    ///< Simple panning for ortho camera.
}


#endif
