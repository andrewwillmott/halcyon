//
//  File:       HLRenderUtils.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLRenderUtils.h>

#include <IHLRenderer.h>
#include <HLGLUtilities.h>

#include <CLColour.h>

using namespace nHL;
using namespace nCL;

namespace
{
    cEltInfo kDrawRectFormat[] =
    {
        { kVBPositions, 2, GL_FLOAT,         8, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };

    struct cDrawRectVertex
    {
        Vec2f    mCoord;
        float    mUV[2];
        cRGBA32  mColour;
    };
}

void nHL::DrawRect(cIRenderer* renderer, cRGBA32 colour, const Vec2f rp[4])
{
    float s0 = 0.0f;
    float s1 = 1.0f - s0;
    float t0 = 0.0f;
    float t1 = 1.0f - t0;

#ifndef GL_QUADS
    cDrawRectVertex screenQuad[6] =
    {
        { rp[0], s0, t0, colour },
        { rp[1], s1, t0, colour },
        { rp[2], s1, t1, colour },

        { rp[2], s1, t1, colour },
        { rp[3], s0, t1, colour },
        { rp[0], s0, t0, colour }
    };

    renderer->DrawBuffer(GL_TRIANGLES, CL_SIZE(kDrawRectFormat), kDrawRectFormat, CL_SIZE(screenQuad), screenQuad);
#else
    cDrawRectVertex screenQuad[4] =
    {
        { rp[0], s0, t0, colour },
        { rp[1], s1, t0, colour },
        { rp[2], s1, t1, colour },
        { rp[3], s0, t1, colour }
    };

    renderer->DrawBuffer(GL_QUADS, CL_SIZE(kDrawRectFormat), kDrawRectFormat, CL_SIZE(screenQuad), screenQuad);
#endif
}

void nHL::DrawRect(cIRenderer* renderer, cRGBA32 colour, const Vec4f& rIn)
{
    Vec4f r = 2.0f * rIn - vl_1;

    Vec2f rp[4] =
    {
        { r[0], r[1] },
        { r[2], r[1] },
        { r[2], r[3] },
        { r[0], r[3] }
    };

    DrawRect(renderer, colour, rp);
}

void nHL::DrawRect(cIRenderer* renderer, cRGBA32 colour)
{
    Vec2f rp[4] =
    {
        { -1.0f, -1.0f },
        { +1.0f, -1.0f },
        { +1.0f, +1.0f },
        { -1.0f, +1.0f }
    };

    DrawRect(renderer, colour, rp);
}

void nHL::DrawRectFlipY(cIRenderer* renderer, cRGBA32 colour)
{
    Vec2f rp[4] =
    {
        { -1.0f, +1.0f },
        { +1.0f, +1.0f },
        { +1.0f, -1.0f },
        { -1.0f, -1.0f }
    };

    DrawRect(renderer, colour, rp);
}

void nHL::DrawScreenRect(cIRenderer* renderer, const cRGBA32 c, const cBounds2& rect)
{
    Vec2f screenSize = renderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    Vec2f invScreenSize = inv(screenSize);

    Mat3f m = HScale3f(Vec2f(2, -2) * invScreenSize) * HTrans3f(Vec2f(-1, 1));

    m = m * renderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);

    Vec2f rp[4] =
    {
        { rect.mMin[0], rect.mMin[1] },
        { rect.mMax[0], rect.mMin[1] },
        { rect.mMax[0], rect.mMax[1] },
        { rect.mMin[0], rect.mMax[1] }
    };

    for (int i = 0; i < 4; i++)
        rp[i] = xform(m, rp[i]);

    DrawRect(renderer, c, rp);
}
