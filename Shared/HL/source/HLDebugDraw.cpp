//
//  File:       DebugDraw.cpp
//
//  Function:   For drawing non-ship debug stuff
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLDebugDraw.h>

#ifndef CL_RELEASE

#include <HLCamera.h>
#include <HLGLUtilities.h>
#include <HLServices.h>

#include <CLBounds.h> 
#include <CLColour.h>
#include <CLMatUtil.h>
#include <CLString.h>
#include <CLTransform.h>
#include <CLVecUtil.h>

using namespace nCL;
using namespace nHL;

namespace
{
    struct cVertex2D
    {
        cVertex2D()                   : mCoord(vl_zero), mColour(kRGBA32BlackA1) {}
        cVertex2D(Vec2f v, cRGBA32 c) : mCoord(v), mColour(c) {}
        cVertex2D(Vec2f v, Vec4f c)   : mCoord(v), mColour(ColourAlphaToRGBA32(c)) {}

        Vec2f    mCoord;
        cRGBA32  mColour;
    };
    
    struct cVertexText2D
    {
        cVertexText2D()                             : mCoord(vl_zero), mUV(vl_0), mColour(kRGBA32BlackA1) {}
        cVertexText2D(Vec2f v, Vec2f uv, cRGBA32 c) : mCoord(v), mUV(uv), mColour(c) {}
        cVertexText2D(Vec2f v, Vec2f uv, Vec4f c)   : mCoord(v), mUV(uv), mColour(ColourAlphaToRGBA32(c)) {}

        Vec2f    mCoord;
        Vec2f    mUV;
        cRGBA32  mColour;
    };
    
    struct cVertex3D
    {
        cVertex3D()                   : mPosition(vl_zero), mColour(kRGBA32BlackA1) {}
        cVertex3D(Vec3f v, cRGBA32 c) : mPosition(v),       mColour(c) {}
        cVertex3D(Vec3f v, Vec4f c)   : mPosition(v),       mColour(ColourAlphaToRGBA32(c)) {}

        Vec3f   mPosition;
        cRGBA32 mColour;
    };

    cEltInfo kV2DFormat[] =
    {
        { kVBPositions, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };
    cEltInfo kVText2DFormat[] =
    {
        { kVBPositions, 2, GL_FLOAT,         8, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };
    cEltInfo kV3DFormat[] =
    {
        { kVBPositions, 3, GL_FLOAT,        12, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };
    
    struct cDebugDrawInternal
    {
        tMaterialRef mDebug2DMaterialIndex       = -1;
        tMaterialRef mDebugText2DMaterialIndex   = -1;
        tMaterialRef mDebug3DMaterialIndex       = -1;

        tTextureRef  mFontTexture                = -1;

        vector<cVertex2D>       mLines2D;
        vector<cVertex2D>       mTris2D;
        vector<cVertexText2D>   mText2D;
        vector<cVertex3D>       mLines3D;
        vector<cVertex3D>       mTris3D;

        cRGBA32 mColour      = kRGBA32WhiteA1;
    };
}

// ---cDebugDraw ---------------------------------------------------------------

cDebugDraw::cDebugDraw() :
    mInternal(Create<cDebugDrawInternal>(AllocatorFromObject(this)))
{
}

cDebugDraw::~cDebugDraw()
{
    Destroy(&mInternal, AllocatorFromObject(this));
}


//#include "../Fonts/stb_font_arial_14_usascii.inl"
//#include "../Fonts/stb_font_courier_14_usascii.inl"

#include "../Fonts/stb_font_consolas_14_usascii.inl"

namespace
{
    const float kLineHeight = ceilf(STB_SOMEFONT_LINE_SPACING * 1.5f); // stb gives us ascender height, but not complete line height

    stb_fontchar sFontData[STB_SOMEFONT_NUM_CHARS];
    uint8_t sFontPixels[STB_SOMEFONT_BITMAP_HEIGHT_POW2][STB_SOMEFONT_BITMAP_WIDTH];
}


bool cDebugDraw::Init()
{
    STB_SOMEFONT_CREATE(sFontData, sFontPixels, STB_SOMEFONT_BITMAP_HEIGHT_POW2);
    mInternal->mFontTexture = HL()->mRenderer->CreateTexture(CL_TAG("font"), kFormatA8, STB_SOMEFONT_BITMAP_WIDTH, STB_SOMEFONT_BITMAP_HEIGHT_POW2, sFontPixels[0]);

    return true;
}

bool cDebugDraw::PostInit()
{
    const cServices* hl = HL();

    mInternal->mDebug2DMaterialIndex     = hl->mRenderer->MaterialRefFromTag(CL_TAG("debugDraw2D"));
    mInternal->mDebugText2DMaterialIndex = hl->mRenderer->MaterialRefFromTag(CL_TAG("debugDrawText2D"));
    mInternal->mDebug3DMaterialIndex     = hl->mRenderer->MaterialRefFromTag(CL_TAG("debugDraw"));

    return true;
}

bool cDebugDraw::Shutdown()
{
    HL()->mRenderer->DestroyTexture(mInternal->mFontTexture);
    mInternal->mFontTexture = 0;

    return true;
}

void cDebugDraw::Reset()
{
    mInternal->mColour = kRGBA32WhiteA1;
    mTransform2D = vl_I;
    mTransform3D = vl_I;
    // don't clear the stack, a caller may be surrounding us
    // with a push/pop.
}

void cDebugDraw::SetColour(float r, float g, float b)
{
    SetColour(tColourRGB(r, g, b));
}

void cDebugDraw::SetColour(const tColourRGB& rgb)
{
    cRGBA32 c = ColourToRGBA32(rgb);

    mInternal->mColour.mChannel[0] = c.mChannel[0];
    mInternal->mColour.mChannel[1] = c.mChannel[1];
    mInternal->mColour.mChannel[2] = c.mChannel[2];
}

void cDebugDraw::SetAlpha(float alpha)
{
    mInternal->mColour.mChannel[3] = ClampUnitRealToUInt8(alpha);
}

void cDebugDraw::SetColourAlpha(float r, float g, float b, float a)
{
    mInternal->mColour = ColourAlphaToRGBA32(tColourRGBA(r, g, b, a));
}

void cDebugDraw::SetColourAlpha(const tColourRGBA& rgba)
{
    mInternal->mColour = ColourAlphaToRGBA32(rgba);
}

// --- 2D ----------------------------------------------------------------------

void cDebugDraw::DrawLines(int numVerts, const Vec2f points[], const Vec4f colours[])
{
    CL_ASSERT((numVerts % 2) == 0);

    mInternal->mLines2D.reserve(mInternal->mLines2D.size() + numVerts);

    Mat3f m = mTransform2D * HScale3f(Vec2f(2, -2) * mInvScreenSize) * HTrans3f(Vec2f(-1, 1));

    m = m * HL()->mRenderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);

    if (colours)
        for (int i = 0; i < numVerts; i++)
            mInternal->mLines2D.push_back(cVertex2D(xform(m, points[i]), colours[i]));
    else
        for (int i = 0; i < numVerts; i++)
            mInternal->mLines2D.push_back(cVertex2D(xform(m, points[i]), mInternal->mColour));
}

void cDebugDraw::DrawTriangles(int numVerts, const Vec2f points[], const Vec4f colours[])
{
    CL_ASSERT((numVerts % 3) == 0);

    mInternal->mTris2D.reserve(mInternal->mTris2D.size() + numVerts);

    Mat3f m = mTransform2D * HScale3f(Vec2f(2, -2) * mInvScreenSize) * HTrans3f(Vec2f(-1, 1));

    m = m * HL()->mRenderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);

    if (colours)
        for (int i = 0; i < numVerts; i++)
            mInternal->mTris2D.push_back(cVertex2D(xform(m, points[i]), colours[i]));
    else
        for (int i = 0; i < numVerts; i++)
            mInternal->mTris2D.push_back(cVertex2D(xform(m, points[i]), mInternal->mColour));
}

Vec2f cDebugDraw::DrawText(float x, float y, const char* str) // draw with top-left point x,y
{
    // This function positions characters on integer coordinates, and assumes 1:1 texels to pixels
    // Appropriate if nearest-neighbor sampling is used

    Mat3f m = mTransform2D * HScale3f(Vec2f(2, -2) * mInvScreenSize) * HTrans3f(Vec2f(-1, 1));

    const Mat3f& od = HL()->mRenderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);
    m = m * od;

    float w = 0.0f;
    Vec2f c(x, y);

    while (*str)
    {
        int codepoint = *str++;

        stb_fontchar* cd = &sFontData[codepoint - STB_SOMEFONT_FIRST_CHAR];

    #ifdef CL_IOS
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x0, cd->y0)), Vec2f(cd->s0, cd->t0), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x0, cd->y1)), Vec2f(cd->s0, cd->t1), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x1, cd->y1)), Vec2f(cd->s1, cd->t1), mInternal->mColour));

        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x1, cd->y1)), Vec2f(cd->s1, cd->t1), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x1, cd->y0)), Vec2f(cd->s1, cd->t0), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x0, cd->y0)), Vec2f(cd->s0, cd->t0), mInternal->mColour));
    #else
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x0, cd->y0)), Vec2f(cd->s0, cd->t0), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x0, cd->y1)), Vec2f(cd->s0, cd->t1), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x1, cd->y1)), Vec2f(cd->s1, cd->t1), mInternal->mColour));
        mInternal->mText2D.push_back(cVertexText2D(xform(m, c + Vec2f(cd->x1, cd->y0)), Vec2f(cd->s1, cd->t0), mInternal->mColour));
    #endif

        c[0] += cd->advance_int;
        w    += cd->advance_int;
    }

    return Vec2f(w, kLineHeight);
}

void cDebugDraw::SetTransform(const Mat3f& m)
{
    mTransform2D = m;
}

void cDebugDraw::ClearTransform2D()
{
    mTransform2D = vl_I;
}

void cDebugDraw::PushTransform(const Mat3f& m)
{
    mTransformStack2D.push_back(mTransform2D);
    mTransform2D = m * mTransform2D;
}

void cDebugDraw::PopTransform2D()
{
    CL_ASSERT(!mTransformStack2D.empty());
    mTransform2D = mTransformStack2D.back();
    mTransformStack2D.pop_back();
}



// --- 3D ----------------------------------------------------------------------

void cDebugDraw::DrawLines(int numVerts, const Vec3f points[], const Vec4f colours[])
{
    CL_ASSERT((numVerts % 2) == 0);

    mInternal->mLines3D.reserve(mInternal->mLines3D.size() + numVerts);

    if (colours)
        for (int i = 0; i < numVerts; i++)
            mInternal->mLines3D.push_back(cVertex3D(xform(mTransform3D, points[i]), colours[i]));
    else
        for (int i = 0; i < numVerts; i++)
            mInternal->mLines3D.push_back(cVertex3D(xform(mTransform3D, points[i]), mInternal->mColour));
}

void cDebugDraw::DrawTriangles(int numVerts, const Vec3f points[], const Vec4f colours[])
{
    CL_ASSERT((numVerts % 3) == 0);

    mInternal->mTris3D.reserve(mInternal->mTris3D.size() + numVerts);

    if (colours)
        for (int i = 0; i < numVerts; i++)
            mInternal->mTris3D.push_back(cVertex3D(xform(mTransform3D, points[i]), colours[i]));
    else
        for (int i = 0; i < numVerts; i++)
            mInternal->mTris3D.push_back(cVertex3D(xform(mTransform3D, points[i]), mInternal->mColour));
}

void cDebugDraw::SetTransform(const Mat4f& m)
{
    mTransform3D = m;
}

void cDebugDraw::SetTransform(const cTransform& xform)
{
    xform.MakeMat4(&mTransform3D);
}

void cDebugDraw::ClearTransform3D()
{
    mTransform3D = vl_I;
}

void cDebugDraw::PushTransform(const Mat4f& m)
{
    mTransformStack3D.push_back(mTransform3D);
    mTransform3D = m * mTransform3D;
}

void cDebugDraw::PushTransform(const cTransform& xform)
{
    Mat4f m;
    xform.MakeMat4(&m);
    PushTransform(m);
}

void cDebugDraw::PopTransform()
{
    CL_ASSERT(!mTransformStack3D.empty());
    mTransform3D = mTransformStack3D.back();
    mTransformStack3D.pop_back();
}



// --- Queries -----------------------------------------------------------------

Vec2f cDebugDraw::TextSize(const char* s)
{
    float w = 0.0f;

    while (*s)
    {
        int codepoint = *s++;

        stb_fontchar* cd = &sFontData[codepoint - STB_SOMEFONT_FIRST_CHAR];

        w += cd->advance_int;
    }

    return Vec2f(w, kLineHeight);
}

float cDebugDraw::TextHeight()
{
    return kLineHeight;
}

Vec2f cDebugDraw::FindScreenCoords(Vec3f p, float offX, float offY)
{
    Vec2f sp = xform(mWorldToClip, p).AsVec2();
    sp[1] = -sp[1];
    sp = (sp + vl_1) * 0.5f;
    sp *= mScreenSize;

    return sp + Vec2f(offX, -offY);
}

void cDebugDraw::Clear()
{
    mInternal->mLines2D.clear();
    mInternal->mTris2D.clear();
    mInternal->mText2D.clear();
    mInternal->mLines3D.clear();
    mInternal->mTris3D.clear();

    Reset();

    mTransformStack2D.clear();
    mTransformStack3D.clear();
}

void cDebugDraw::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
{
    Mat4f w2c = vl_I;
    Mat4f c2c(mWorldToClip);

    ApplyPostClipOps(renderer, &mWorldToClip);
    renderer->SetShaderDataT(kDataIDWorldToCamera, w2c);
    renderer->SetShaderDataT(kDataIDCameraToClip,  mWorldToClip);

    if (mInternal->mDebug3DMaterialIndex >= 0)
    {
        renderer->SetMaterial(mInternal->mDebug3DMaterialIndex);

        if (!mInternal->mTris3D.empty())
            renderer->DrawBuffer(GL_TRIANGLES,  CL_SIZE(kV3DFormat), kV3DFormat, mInternal->mTris3D.size(),  mInternal->mTris3D.data());

        if (!mInternal->mLines3D.empty())
            renderer->DrawBuffer(GL_LINES,      CL_SIZE(kV3DFormat), kV3DFormat, mInternal->mLines3D.size(), mInternal->mLines3D.data());
    }

    if (mInternal->mDebug2DMaterialIndex >= 0)
    {
        renderer->SetMaterial(mInternal->mDebug2DMaterialIndex);

        if (!mInternal->mTris2D.empty())
            renderer->DrawBuffer(GL_TRIANGLES, CL_SIZE(kV2DFormat), kV2DFormat, mInternal->mTris2D.size(),  mInternal->mTris2D.data());

        if (!mInternal->mLines2D.empty())
            renderer->DrawBuffer(GL_LINES,     CL_SIZE(kV2DFormat), kV2DFormat, mInternal->mLines2D.size(), mInternal->mLines2D.data());
    }

    if (!mInternal->mText2D.empty() && mInternal->mDebugText2DMaterialIndex >= 0)
    {
        renderer->SetMaterial(mInternal->mDebugText2DMaterialIndex);
        renderer->SetTexture(kTextureDiffuseMap, mInternal->mFontTexture);    // TODO: need CreateTexture() etc. to get proper texture index.

    #ifndef GL_QUADS
        // TODO: get canonical quad->tri IB from renderer instead?
        renderer->DrawBuffer(GL_TRIANGLES,  CL_SIZE(kVText2DFormat), kVText2DFormat, mInternal->mText2D.size(), mInternal->mText2D.data());
    #else
        renderer->DrawBuffer(GL_QUADS,      CL_SIZE(kVText2DFormat), kVText2DFormat, mInternal->mText2D.size(), mInternal->mText2D.data());
    #endif
    }

    renderer->SetMaterial(0);
}


cDebugDraw* nHL::CreateDebugDraw(nCL::cIAllocator* alloc)
{
    return new(alloc) cDebugDraw;
}


// -----------------------------------------------------------------------------
// MARK: - 2D primitives -

namespace
{
    const int kSubdivisions = 18;
    Vec3f sCircleVertices[kSubdivisions];

    struct cCircleInit
    {
        cCircleInit()
        {
            float delta = vl_twoPi / kSubdivisions;
            
            for (int i = 0; i < kSubdivisions; i++)
                sCircleVertices[i] = Vec3f(sinf(i * delta), cosf(i * delta), 0.0f);
        }
    } sCircleInit;

    cTransform RadialTransform(Vec3f origin, Vec3f normal, float radius)
    {
        cTransform transform;
        
        transform.mScale       = radius;
        transform.mRotation    = MakeRot(vl_z, normal);
        transform.mTranslation = origin;

        return transform;
    }
}

void nHL::DrawLine(cDebugDraw* dd, Vec2f from, Vec2f to)
{
    const Vec2f lines[] =
    {
        from,
        to
    };

    dd->DrawLines(CL_SIZE(lines), lines);
}

void nHL::DrawRect(cDebugDraw* dd, float x1, float y1, float x2, float y2)
{
    const Vec2f lines[] =
    {
        Vec2f(x1, y1),  Vec2f(x2 + 1, y1),  // TODO: look up GL rasterisation rules -- why is top-right dropout happening?
        Vec2f(x2, y1),  Vec2f(x2, y2),
        Vec2f(x2, y2),  Vec2f(x1, y2),
        Vec2f(x1, y2),  Vec2f(x1, y1),
    };

    dd->DrawLines(CL_SIZE(lines), lines);
}

void nHL::FillRect(cDebugDraw* dd, float x1, float y1, float x2, float y2)
{
    Vec2f tris[] =
    {
        Vec2f(x1, y1),
        Vec2f(x1, y2),
        Vec2f(x2, y2),

        Vec2f(x2, y2),
        Vec2f(x2, y1),
        Vec2f(x1, y1)
    };

    dd->DrawTriangles(CL_SIZE(tris), tris);
}

void nHL::DrawCircle(cDebugDraw* dd, Vec2f o, float r)
{
    const Vec3f* vertices = sCircleVertices;

    Vec2f lines[kSubdivisions * 2];
    Vec2f* v = lines;

    Vec2f s(r, -r);

    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = o + s * Vec2f(vertices[i    ].Ref());
        (*v++) = o + s * Vec2f(vertices[i + 1].Ref());
    }

    (*v++) = o + s * Vec2f(vertices[kSubdivisions - 1].Ref());
    (*v++) = o + s * Vec2f(vertices[0                ].Ref());

    dd->DrawLines(CL_SIZE(lines), lines);
}

void nHL::FillCircle(cDebugDraw* dd, Vec2f o, float r)
{
    const Vec3f* vertices = sCircleVertices;

    Vec2f tris[kSubdivisions * 3];
    Vec2f* v = tris;

    Vec2f s(r, -r);
    
    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = o;
        (*v++) = o + s * (Vec2f&) vertices[  i  ];
        (*v++) = o + s * (Vec2f&) vertices[i + 1];
    }

    (*v++) = o;
    (*v++) = o + s * (Vec2f&) vertices[kSubdivisions - 1];
    (*v++) = o + s * (Vec2f&) vertices[0                ];

    dd->DrawTriangles(CL_SIZE(tris), tris);
}


// -----------------------------------------------------------------------------
// MARK: - 3D primitives -

void nHL::DrawLine(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2)
{
    DrawLine(dd, Vec3f(x1, y1, z1), Vec3f(x2, y2, z2));
}

void nHL::DrawLine(cDebugDraw* dd, Vec3f from, Vec3f to)
{
    const Vec3f lines[2] = { from, to };
    dd->DrawLines(2, lines);
}

void nHL::DrawBox(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2)
{
    DrawBox(dd, Vec3f(x1, y1, z1), Vec3f(x2, y2, z2));
}

void nHL::DrawBox(cDebugDraw* dd, Vec3f min, Vec3f max)
{
    Vec3f c[8];
    cBounds3(min, max).FindCorners(c);

    const Vec3f lines[] =
    {
        c[0], c[1],
        c[2], c[3],
        c[4], c[5],
        c[6], c[7],

        c[0], c[4],
        c[1], c[5],
        c[2], c[6],
        c[3], c[7],

        c[0], c[2],
        c[1], c[3],
        c[4], c[6],
        c[5], c[7]
    };

    dd->DrawLines(CL_SIZE(lines), lines);
}

void nHL::FillBox(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2)
{
    FillBox(dd, Vec3f(x1, y1, z1), Vec3f(x2, y2, z2));
}

void nHL::FillBox(cDebugDraw* dd, Vec3f min, Vec3f max)
{
    Vec3f c[8];

    cBounds3(min, max).FindCorners(c);

    Vec3f tris[6 * 6] =
    {
        c[0], c[2], c[1],
        c[1], c[2], c[3],
        c[5], c[1], c[7],
        c[1], c[3], c[7],
        c[4], c[5], c[7],
        c[7], c[6], c[4],
        c[0], c[6], c[2],
        c[4], c[6], c[0],

        c[6], c[7], c[2],
        c[7], c[3], c[2],
        c[0], c[5], c[4],
        c[0], c[1], c[5]
    };

    dd->DrawTriangles(CL_SIZE(tris), tris);
}

void nHL::DrawAxes(cDebugDraw* dd, Vec3f origin, float size)
{
    dd->SetColour(kColourRed);
    DrawLine(dd, origin, origin + Vec3f(vl_x) * size);
    dd->SetColour(kColourGreen);
    DrawLine(dd, origin, origin + Vec3f(vl_y) * size);
    dd->SetColour(kColourBlue);
    DrawLine(dd, origin, origin + Vec3f(vl_z) * size);
}

void nHL::DrawFrustum(cDebugDraw* dd, const Mat4f& clipToWorld)
{
    Vec4f hc[8];

    hc[0] = -clipToWorld[0] - clipToWorld[1] + clipToWorld[2];
    hc[1] = -clipToWorld[0] + clipToWorld[1] + clipToWorld[2];
    hc[2] =  clipToWorld[0] + clipToWorld[1] + clipToWorld[2];
    hc[3] =  clipToWorld[0] - clipToWorld[1] + clipToWorld[2];

    hc[4] = hc[0] + clipToWorld[2];
    hc[5] = hc[1] + clipToWorld[2];
    hc[6] = hc[2] + clipToWorld[2];
    hc[7] = hc[3] + clipToWorld[2];

    Vec3f c[8];
    for (int i = 0; i < 8; i++)
        c[i] = proj(hc[i]);

    Vec3f lines[] =
    {
        c[0], c[1],
        c[1], c[2],
        c[2], c[3],
        c[3], c[0],

        c[4], c[5],
        c[5], c[6],
        c[6], c[7],
        c[7], c[4],

        c[0], c[4],
        c[1], c[5],
        c[2], c[6],
        c[3], c[7]
    };

    dd->DrawLines(CL_SIZE(lines), lines);
}

void nHL::DrawCircle
(
    cDebugDraw* dd,
    Vec3f origin, 
    float radius,
    Vec3f normal
)
{
    const Vec3f* vertices = sCircleVertices;

    Vec3f lines[kSubdivisions * 2];
    Vec3f* v = lines;

    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = vertices[i];
        (*v++) = vertices[i + 1];
    }

    (*v++) = vertices[kSubdivisions - 1];
    (*v++) = vertices[0];

    dd->PushTransform(RadialTransform(origin, normal, radius));
    dd->DrawLines(CL_SIZE(lines), lines);
    dd->PopTransform();
}

void nHL::FillCircle
(
    cDebugDraw* dd,
    Vec3f origin,
    float radius,
    Vec3f normal
)
{
    const Vec3f* vertices = sCircleVertices;

    Vec3f tris[kSubdivisions * 3];
    Vec3f* v = tris;

    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = vl_zero;
        (*v++) = vertices[i + 1];
        (*v++) = vertices[i];
    }

    (*v++) = vl_zero;
    (*v++) = vertices[0];
    (*v++) = vertices[kSubdivisions - 1];

    dd->PushTransform(RadialTransform(origin, normal, radius));
    dd->DrawTriangles(CL_SIZE(tris), tris);
    dd->PopTransform();
}

void nHL::FillCone(cDebugDraw* dd, Vec3f origin, Vec3f normal, float height, float radius)
{
    const Vec3f* vertices = sCircleVertices;
    Vec3f tip(0.0f, 0.0f, height / radius);

    Vec3f tris[kSubdivisions * 6];
    Vec3f* v = tris;

    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = vl_zero;
        (*v++) = vertices[i];
        (*v++) = vertices[i + 1];

        (*v++) = tip;
        (*v++) = vertices[i + 1];
        (*v++) = vertices[i];
    }

    (*v++) = vl_zero;
    (*v++) = vertices[kSubdivisions - 1];
    (*v++) = vertices[0];

    (*v++) = tip;
    (*v++) = vertices[0];
    (*v++) = vertices[kSubdivisions - 1];

    dd->PushTransform(RadialTransform(origin, normal, radius));
    dd->DrawTriangles(CL_SIZE(tris), tris);
    dd->PopTransform();
}


void nHL::FillCylinder
(
    cDebugDraw* dd,
    Vec3f origin, 
    Vec3f normal,
    float height,
    float radius
)
{
    const Vec3f* vertices = sCircleVertices;
    Vec3f offset(0.0f, 0.0f, height / radius);

    Vec3f tris[kSubdivisions * 6];
    Vec3f* v = tris;

    for (int i = 0; i < kSubdivisions - 1; i++)
    {
        (*v++) = vertices[i    ]         ;
        (*v++) = vertices[i    ] + offset;
        (*v++) = vertices[i + 1]         ;

        (*v++) = vertices[i    ] + offset;
        (*v++) = vertices[i + 1] + offset;
        (*v++) = vertices[i + 1]         ;
    }

    (*v++) = vertices[kSubdivisions - 1];
    (*v++) = vertices[kSubdivisions - 1] + offset;
    (*v++) = vertices[0];

    (*v++) = vertices[kSubdivisions - 1] + offset;
    (*v++) = vertices[0] + offset;
    (*v++) = vertices[0];

    // TODO: why aren't we adding endcaps here? Is anything relying on that?

    dd->PushTransform(RadialTransform(origin, normal, radius));
    dd->DrawTriangles(CL_SIZE(tris), tris);
    dd->PopTransform();
}


void nHL::FillGradientRect(cDebugDraw* dd, float x1, float y1, float x2, float y2, const cColourAlpha& c, float s0, float s1)
{
    cColourAlpha c0(c.AsColour() * s0, c.AsAlpha());
    cColourAlpha c1(c.AsColour() * s1, c.AsAlpha());

    Vec2f quadP[6] =
    {
        { x1, y1 },
        { x2, y1 },
        { x2, y2 },

        { x1, y1 },
        { x2, y2 },
        { x1, y2 }
    };

    Vec4f quadC[6] =
    {
        c0,
        c1,
        c1,
        c0,
        c1,
        c0
    };

    dd->DrawTriangles(6, quadP, quadC);
}


Vec2f nHL::DrawText(cDebugDraw* dd, float x, float y, const char* str) // draw with top-left point x,y
{
    // Switch -ve values to other side of the screen...

    if (x < 0 || y < 0)
    {
        Vec2f ss = dd->ScreenSize();
        Vec2f ts = dd->TextSize(str);

        if (x < 0)
            x += ss[0] - ts[0];

        if (y < 0)
            y += ss[1] - ts[1];
    }

    return dd->DrawText(x, y, str);
}

Vec2f nHL::DrawTextCentred(cDebugDraw* dd, float x, float y, const char* str)
{
    Vec2f ts = dd->TextSize(str);
    Vec2f ss = Vec2f(x, y) - 0.5f * ts;

    return dd->DrawText(ss[0], ss[1], str);
}

namespace
{
    string sDebugStringScratch;
}

Vec2f nHL::DrawTextF(cDebugDraw* dd, float x, float y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sDebugStringScratch.vformat(format, args);
    va_end(args);

    return DrawText(dd, x, y, sDebugStringScratch.c_str());
}

Vec2f nHL::DrawTextCentredF(cDebugDraw* dd, float x, float y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sDebugStringScratch.vformat(format, args);
    va_end(args);

    return DrawTextCentred(dd, x, y, sDebugStringScratch.c_str());
}



Vec2f nHL::DrawText(cDebugDraw* dd, Vec3f p, float x, float y, const char* str)
{
    p = xform(dd->Transform3(), p);

    Vec2f os = dd->FindScreenCoords(p);

    return dd->DrawText(os[0]+x, os[1]+y, str);
}

Vec2f nHL::DrawTextF(cDebugDraw* dd, Vec3f p, float x, float y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sDebugStringScratch.vformat(format, args);
    va_end(args);

    return DrawText(dd, p, x, y, sDebugStringScratch.c_str());
}

Vec2f nHL::DrawTextCentred(cDebugDraw* dd, Vec3f p, float x, float y, const char* str)
{
    p = xform(dd->Transform3(), p);

    Vec2f os = dd->FindScreenCoords(p);
    Vec2f ts = dd->TextSize(str);
    
    Vec2f ss = os + Vec2f(x, y) - 0.5f * ts;

    return dd->DrawText(ss[0], ss[1], str);
}

Vec2f nHL::DrawTextCentredF(cDebugDraw* dd, Vec3f p, float x, float y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sDebugStringScratch.vformat(format, args);
    va_end(args);

    return DrawTextCentred(dd, p, x, y, sDebugStringScratch.c_str());
}



// --- cDebugTextRect ----------------------------------------------------------

cDebugTextRect& cDebugTextRect::Clear()
{
    mColours.clear();
    mLines.clear();
    return *this;
}

cDebugTextRect& cDebugTextRect::SetColour(tColourRGB c)
{
    mTextColour = c;
    return *this;
}

cDebugTextRect& cDebugTextRect::Print(const char* s)
{
    mColours.push_back(mTextColour);
    mLines.push_back();
    mLines.back().assign(s);

    return *this;
}

cDebugTextRect& cDebugTextRect::Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    mColours.push_back(mTextColour);
    mLines.push_back();
    mLines.back().vformat(format, args);

    return *this;
}

void cDebugTextRect::SetRectParams(tColourRGBA bgc, tColourRGB bdc, float borderPad, float linePad)
{
    mRectColour = bgc;
    mBorderColour = bdc;
    mBorderPad = borderPad;
    mLinePad = linePad;
}

void cDebugTextRect::Draw(cDebugDraw* debugDraw, float screenX, float screenY)
{
    int numLines = mLines.size();
    
    if (numLines == 0)
        return;

    Vec2f ts = FindTextSize(debugDraw);

    DrawTextRect(debugDraw, Vec2f(screenX, screenY), ts);
}

void cDebugTextRect::Draw(cDebugDraw* dd, Vec3f p, float offsetX, float offsetY)
{
    Vec2f os = dd->FindScreenCoords(p);
    Vec2f ts = FindTextSize(dd);

    int numLines = mLines.size();
    if (numLines == 0)
        return;

    float paneWidth  = ts[0] + (mBorderPad * 2.0f);
    float paneHeight = ts[1] + (mLinePad * (numLines - 1)) + (mBorderPad * 2.0f);

    Vec2f ss = os + Vec2f(offsetX, offsetY);

    if (offsetX < 0)
        ss[0] -= paneWidth;
    if (offsetY < 0)
        ss[1] -= paneHeight;

    DrawTextRect(dd, ss, ts);

    if (mBorderColour != vl_0)
    {
        dd->SetColourAlpha(mRectColour);
        DrawLine(dd, os[0], os[1], ss[0], ss[1]);
    }
}

// protected

void cDebugTextRect::DrawTextRect(cDebugDraw* dd, Vec2f ss, Vec2f ts)
{
    int numLines = mLines.size();

    Vec2f r0;
    Vec2f r1;
    Vec2f pt;
    Vec2f sw = dd->ScreenSize();

    ts += Vec2f(2.0f * mBorderPad);
    ts[1] += mLinePad * (numLines - 1);

    if (ss[0] >= 0.0f)
    {
        pt[0] = ss[0] + mBorderPad;
        r0[0] = ss[0];
        r1[0] = ss[0] + ts[0];
    }
    else
    {
        pt[0] = ss[0] - mBorderPad;
        ss[0] += sw[0];
        r0[0] = ss[0] - ts[0];
        r1[0] = ss[0];
    }

    if (ss[1] >= 0.0f)
    {
        pt[1] = ss[1] + mBorderPad;
        r0[1] = ss[1];
        r1[1] = ss[1] + ts[1];
    }
    else
    {
        pt[1] = ss[1] - mBorderPad;
        ss[1] += sw[1];
        r0[1] = ss[1] - ts[1];
        r1[1] = ss[1];
    }

    dd->SetColourAlpha(mRectColour);

    FillRect(dd, r0[0], r0[1], r1[0], r1[1]);

    dd->SetAlpha(1.0f);

    if (mBorderColour != vl_0)
    {
        dd->SetColour(mBorderColour);

        DrawRect(dd, r0[0], r0[1], r1[0], r1[1]);
    }

    for (int i = 0; i < numLines; i++)
    {
        dd->SetColour(mColours[i]);
        Vec2f ls = DrawText(dd, pt[0], pt[1], mLines[i].c_str());
        if (pt[1] >= 0.0f)
            pt[1] += ls[1] + mLinePad;
        else
            pt[1] -= ls[1] + mLinePad;
    }
}

Vec2f cDebugTextRect::FindTextSize(cDebugDraw* debugDraw)
{
    Vec2f result = vl_0;

    for (int i = 0, n = mLines.size(); i < n; i++)
    {
        float sx, sy;
        Vec2f ls = debugDraw->TextSize(mLines[i].c_str());

        result[0] = Max(result[0], ls[0]);
        result[1] += ls[1];
    }

    return result;
}

#endif
