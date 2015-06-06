//
//  File:       HLCamera.cpp
//
//  Function:   Camera support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLCamera.h>

#include <IHLRenderer.h>
#include <HLUI.h>

#include <CLInputState.h>
#include <CLMatUtil.h>
#include <CLSTL.h>
#include <CLTransform.h>
#include <CLValue.h>
#include <CLVecUtil.h>

#include <VLf.h>

using namespace nCL;
using namespace nHL;

namespace
{
    cStandardCamera kDefaultStandardCamera;
}


// --- cCamera -----------------------------------------------------------

void cCamera::Config(const cObjectValue* config)
{
    if (!config)
    {
        // Reset();
        return;
    }

    mNear = config->Member("near").AsFloat(mNear);
    mFar  = config->Member("far" ).AsFloat(mFar);
}

void cCamera::Dispatch(cIRenderer* renderer)
{
    Mat4f w2c;
    FindView(&w2c);

    Vec2f orientedSize = renderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    CL_ASSERT(orientedSize != vl_0);

    Mat4f c2c;
    FindProjection(orientedSize, &c2c);

    ApplyPostClipOps(renderer, &c2c);

    renderer->SetShaderDataT(kDataIDWorldToCamera, w2c);
    renderer->SetShaderDataT(kDataIDCameraToClip,  c2c);
}

void cCamera::SetCameraToWorld(const cTransform& xform)
{
    mCameraToWorld = xform;
}

const cTransform& cCamera::CameraToWorld()
{
    return mCameraToWorld;
}

void cCamera::SetSceneBounds(const cBounds3& bounds)
{
    mSceneBounds = bounds;
}

void cCamera::FindViewProjection(Vec2f viewSize, Mat4f* vp)
{
	Mat4f projection;
    FindProjection(viewSize, &projection);

	Mat4f view;
    FindView(&view);

    *vp = view * projection;
}

void cCamera::FindProjection(Vec2f viewSize, Mat4f* projection)
{
    // Calculate the projection matrix
    float fh = 23 * viewSize[1];    // TODO
    float fw = 23 * viewSize[0];

    float ma = MinElt(viewSize);

    MakePerspectiveZUp(fh / ma, fw / ma, mNear, mFar, projection);
}

void cCamera::FindView(Mat4f* viewIn)
{
    mCameraToWorld.Inverse().MakeMat4(viewIn);
}

// cOrthoCamera

void cOrthoCamera::Config(const cObjectValue* config)
{
    if (!config)
    {
        // Reset();
        return;
    }

    cCamera::Config(config);

    mLeft   = config->Member("left"  ).AsFloat(mLeft);
    mRight  = config->Member("right" ).AsFloat(mRight);
    mBottom = config->Member("bottom").AsFloat(mBottom);
    mTop    = config->Member("top"   ).AsFloat(mTop);
}

void cOrthoCamera::FindProjection(Vec2f viewSize, Mat4f* projection)
{
    // camera space has y...
    /*
        [x y z 1] [     0    0]
                  [     fna  0]
                  [      0   0]
                  [     fnb  1]

    
    */

    Mat4f& m = *projection;

    m = vl_0;
    m(0, 0) = 2.0f / (mRight - mLeft);
    m(3, 0) = -(mLeft + mRight) / (mRight - mLeft);

    m(2, 1) = 2.0f / (mTop - mBottom);
    m(3, 1) = -(mTop + mBottom) / (mTop - mBottom);

    m(1, 2) = 2.0f / (mFar - mNear);
    m(3, 2) = (mFar + mNear) / (mFar - mNear);

    m(3, 3) = 1.0f;
}

void cOrthoCamera::SetFrustum(const cBounds3& bounds)
{
    mLeft   = bounds.mMin[0];
    mRight  = bounds.mMax[0];
    mBottom = bounds.mMin[1];
    mTop    = bounds.mMax[1];
    mNear   = bounds.mMin[2];
    mFar    = bounds.mMax[2];
}

nCL::cBounds3 cOrthoCamera::Frustum()
{
    return cBounds3(Vec3f(mLeft, mBottom, mNear), Vec3f(mRight, mTop, mFar));
}



// --- cSimpleCamera -----------------------------------------------------------

void cStandardCamera::Reset()
{
// not safe now we have a vtable    SELF = kDefaultStandardCamera;

    // TODO: stick in another struct to avoid duplication
    mZUp               = true;
    mCameraTheta       = 0.0f;
    mCameraPhi         = 0.0f;
    mCameraDistance    = 500.0f;
    mCameraOffsetX     = 0.0f;
    mCameraOffsetY     = 0.0f;
    mCameraFOV         = 46.0f;

    UpdateCameraToWorld();
}

void cStandardCamera::Config(const cObjectValue* config)
{
    if (!config)
    {
        Reset();
        return;
    }

    mCameraTheta    = config->Member("heading" ).AsFloat(mCameraTheta);
    mCameraPhi      = config->Member("pitch"   ).AsFloat(mCameraPhi);
    mCameraDistance = config->Member("distance").AsFloat(mCameraDistance);
    mCameraOffsetX  = config->Member("offsetX" ).AsFloat(mCameraOffsetX);
    mCameraOffsetY  = config->Member("offsetY" ).AsFloat(mCameraOffsetY);
    mCameraFOV      = config->Member("fov"     ).AsFloat(mCameraFOV);

    mNear = config->Member("nearClip").AsFloat(mNear);
    mFar  = config->Member("farClip" ).AsFloat(mFar);

    UpdateCameraToWorld();
}

void cStandardCamera::SetDepthClip(float near, float far)
{
    mNear = near;
    mFar  = far;
}

void cStandardCamera::Dispatch(cIRenderer* renderer)
{
    Mat4f w2c;
    FindView(&w2c);

    Vec2f orientedSize = renderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    CL_ASSERT(orientedSize != vl_0);

    Mat4f c2c;
    FindProjection(orientedSize, &c2c);

    ApplyPostClipOps(renderer, &c2c);

    renderer->SetShaderDataT(kDataIDWorldToCamera, w2c);
    renderer->SetShaderDataT(kDataIDCameraToClip,  c2c);
}

void cStandardCamera::UpdateCameraToWorld()
{
    mCameraToWorld.mTranslation = -Vec3f(mCameraOffsetX, mCameraDistance, mCameraOffsetY);
    mCameraToWorld.mScale = 1.0f;
    mCameraToWorld.mRotation = vl_I;

	mCameraToWorld.AppendRotX(-mCameraPhi   * vl_pi / 180.0f);
    mCameraToWorld.AppendRotZ(-mCameraTheta * vl_pi / 180.0f);
}

void cStandardCamera::FindProjection(Vec2f viewSize, Mat4f* projection)
{
    // Calculate the projection matrix
    float fh = mCameraFOV * viewSize[1];
    float fw = mCameraFOV * viewSize[0];

    float ma = MinElt(viewSize);

	if (mZUp)
        MakePerspectiveZUp(fh / ma, fw / ma, mNear, mFar, projection);
    else
        MakePerspectiveYUp(fh / ma, fw / ma, mNear, mFar, projection);
}

void cStandardCamera::FindView(Mat4f* viewIn)
{
    Mat4f& view = *viewIn;

    if (mZUp)
        view.MakeHTrans(Vec3f(mCameraOffsetX, mCameraDistance, mCameraOffsetY));
    else
        view.MakeHTrans(Vec3f(mCameraOffsetX, mCameraOffsetY, -mCameraDistance));
    
	PreRotateX(mCameraPhi   * vl_pi / 180.0f, &view);
    PreRotateZ(mCameraTheta * vl_pi / 180.0f, &view);
}

void cStandardCamera::FindViewProjection(Vec2f viewSize, Mat4f* vp)
{
	Mat4f projection;
    FindProjection(viewSize, &projection);

	Mat4f view;
    FindView(&view);

    *vp = view * projection;
}


// --- cSimpleCamera -----------------------------------------------------------

void cSimpleCamera::Reset()
{
    cStandardCamera::Reset();
    
    mStartCameraTheta = 0;
    mStartCameraPhi = 0;
    mStartCameraDistance = 0;
    mStartCameraOffsetX = 0;
    mStartCameraOffsetY = 0;
}

void cSimpleCamera::SetRelativeDistance(float d)
{
    mCameraDistance = MaxElt(mSceneBounds.Width()) * d;
    UpdateCameraToWorld();
}

void cSimpleCamera::StartModalDrag()
{
    mStartCameraTheta    = mCameraTheta;
    mStartCameraPhi      = mCameraPhi;
    mStartCameraDistance = mCameraDistance;
    mStartCameraOffsetX  = mCameraOffsetX;
    mStartCameraOffsetY  = mCameraOffsetY;
}

void cSimpleCamera::UpdateModalDrag(tMode mode, float dx, float dy)
{
    switch (mode)
    {
    case kModePanXY:
        mCameraOffsetX = mStartCameraOffsetX + mCameraDistance * dx;
        mCameraOffsetY = mStartCameraOffsetY - mCameraDistance * dy;
        break;
        
    case kModeZoomX:
        mCameraDistance = mStartCameraDistance + dx;
        break;

    case kModeZoomY:
        mCameraDistance = mStartCameraDistance - dy;
        break;
        
    case kModeZoomXY:
        mCameraDistance = mStartCameraDistance + mCameraDistance * (dx - dy);
        break;
        
    case kModeRotate:
        mCameraTheta = mStartCameraTheta + dx * 180;
        mCameraPhi   = mStartCameraPhi   + dy * 180;
        break;

    default:
        break;
    }
    
    if (mCameraPhi < -90)
        mCameraPhi = -90;
    if (mCameraPhi >  90)
        mCameraPhi =  90;

    if (mCameraDistance < mNear)
        mCameraDistance = mNear;

    if (0)
    {
        // z is out of the screen, y is up as you look at the springboard, x is to the right.
        Vec3f up(-mAccelerometer[0], mAccelerometer[2], -mAccelerometer[1]);

        Mat3f orient = AlignZToDir(up);

        Mat4f m;
        m[0] = Vec4f(orient[0], 0.0f);
        m[1] = Vec4f(orient[1], 0.0f);
        m[2] = Vec4f(orient[2], 0.0f);
        m[3] = vl_w;
        //view = m * view;
    }

    UpdateCameraToWorld();
}

void cSimpleCamera::Config(const cObjectValue* config)
{
    cStandardCamera::Config(config);

    if (config && config->HasMember("relativeDistance"))
        SetRelativeDistance(config->Member("relativeDistance").AsFloat());
}

// Utilities


void nHL::ApplyPostClipOps(cIRenderer* renderer, Mat4f* clipInOut)
{
    // Apply any device orientation
    Mat3f od           = renderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);
    Vec2f vo           = renderer->ShaderDataT<Vec2f>(kDataIDViewOffset);
    Vec2f vs           = renderer->ShaderDataT<Vec2f>(kDataIDViewSize);

    CL_ASSERT(vs != vl_0);

    vo /= vs;

    // Pre-transform od to take view offset into account
    od(2, 0) += vo[0] * od(0, 0) + vo[1] * od(1, 0);
    od(2, 1) += vo[0] * od(0, 1) + vo[1] * od(1, 1);

    // Post-transform projection matrix to take orientation into account
    const Mat4f& c2c = *clipInOut;

    Vec4f c0 = { c2c(0, 0), c2c(1, 0), c2c(2, 0), c2c(3, 0) };
    Vec4f c1 = { c2c(0, 1), c2c(1, 1), c2c(2, 1), c2c(3, 1) };
    Vec4f c3 = { c2c(0, 3), c2c(1, 3), c2c(2, 3), c2c(3, 3) };

    Vec4f nc0 = od(0, 0) * c0 + od(1, 0) * c1 + od(2, 0) * c3;
    Vec4f nc1 = od(0, 1) * c0 + od(1, 1) * c1 + od(2, 1) * c3;

    col(c2c, 0) = nc0;
    col(c2c, 1) = nc1;
}

void nHL::HandleStandardInput(cUIState* uiState, cSimpleCamera* camera)
{
    tUIItemID itemID = ItemID(0x00e0fa12);

    // Claim any remaining screen interactions
    uiState->AddItemCanvas(itemID);
    
    int count = uiState->InteractionCount();

    if (count == 0)
        return;

    for (int i = 0; i < count; i++)
    {
        int pi = uiState->InteractionPointerIndex(i);

        if (uiState->InputState()->PointerWentDown(pi))
            camera->StartModalDrag();
        else
        {
            // TODO: at the least InteractionPointer(0)?
            const cPointerInfo& pointerInfo = uiState->InteractionPointerInfo(i);
            
            cSimpleCamera::tMode mode = cSimpleCamera::kModeNone;
            
            if      (pointerInfo.mStartModifiers == kModButton3 || pointerInfo.mStartModifiers == (kModAlt | kModButton1))
                mode = cSimpleCamera::kModePanXY;
            else if (pointerInfo.mStartModifiers == kModButton2 || pointerInfo.mStartModifiers == (kModControl | kModButton1))
                mode = cSimpleCamera::kModeZoomXY;
            else if (pointerInfo.mStartModifiers == kModButton1)
                mode = cSimpleCamera::kModeRotate;
            
            float dScale = (1.0f / MinElt(uiState->CanvasRect().Width()));
            
            float dx = (pointerInfo.mCurrentPos[0] - pointerInfo.mStartPos[0]) * dScale;
            float dy = (pointerInfo.mCurrentPos[1] - pointerInfo.mStartPos[1]) * dScale;
            
            camera->UpdateModalDrag(mode, dx, dy);
        }
    }
}

void nHL::HandleStandardInput(cUIState* uiState, cOrthoCamera* camera, Vec2f scale)
{
    tUIItemID itemID = ItemID(0x01fa3f75);

    // Claim any remaining screen interactions
    uiState->AddItemCanvas(itemID);

    int count = uiState->InteractionCount();
    
    if (count == 0)
        return;
    
    const cPointerInfo& pointerInfo = uiState->InteractionPointerInfo(0);

    Vec2f wh = uiState->CanvasRect().Width();
    Vec2f dp = uiState->InteractionPointDeltaLast();

    dp /= wh;
    dp *= scale;

    Vec3f fw = camera->Frustum().Width();
    dp[0] *= fw[0];
    dp[1] *= fw[1];

    cTransform xform(camera->CameraToWorld());
    xform.PrependTrans(Vec3f(-dp[0], 0.0f, dp[1]));
    camera->SetCameraToWorld(xform);
}
