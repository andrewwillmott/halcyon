//
//  File:       HLDebugDraw.h
//
//  Function:   For drawing non-ship debug stuff
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_DEBUGDRAW_H
#define HL_DEBUGDRAW_H

#ifndef CL_RELEASE

#include <IHLRenderer.h>
#include <VL234f.h>
#include <CLColour.h>
#include <CLLink.h>
#include <CLMemory.h>
#include <CLSTL.h>

typedef Vec3f tColourRGB;
typedef Vec4f tColourRGBA;

namespace
{
    class cDebugDrawInternal;
}

namespace nCL
{
    class cTransform;
}

namespace nHL
{
    // --- DebugDraw -----------------------------------------------------------

    class cDebugDraw :
        public cIRenderLayer,
        public nCL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        cDebugDraw();
        ~cDebugDraw();

        bool Init();
        bool PostInit();    ///< Called after all systems have been configured.
        bool Shutdown();

        // cIRenderLayer
        virtual void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;

        // cDebugDraw

        // Basic drawing commands
        void SetScreenSize(Vec2f ss);           ///< Set screen size for 2D mode
        void SetWorldToClip(const Mat4f& m);    ///< Set projection for 3D mode
        void Clear();                           ///< Call at the beginning of the frame/sim tick.
        void Reset();                           ///< Reset draw state back to default values

        void SetColour(float r, float g, float b);
        void SetColour(const tColourRGB& rgb);
        void SetAlpha (float alpha);

        void SetColourAlpha(float r, float g, float b, float a);
        void SetColourAlpha(const tColourRGBA& rgba);

        // 2D
        void DrawLines    (int numVerts, const Vec2f points[], const Vec4f colours[] = 0);  ///< Positions are in pixels, 0 = top of screen
        void DrawTriangles(int numVerts, const Vec2f points[], const Vec4f colours[] = 0);

        Vec2f DrawText(float x, float y, const char* str);                ///< Draw the given text string. Position is in pixels, 0 = top of screen. Returns size of text.

        void SetTransform(const Mat3f& m);
        void ClearTransform2D();
        void PushTransform(const Mat3f& m);             ///< Push current 2D transform, multiply in m
        void PopTransform2D();                          ///< Pop transform stack

        // 3D
        void DrawLines    (int numVerts, const Vec3f points[], const Vec4f colours[] = 0);
        void DrawTriangles(int numVerts, const Vec3f points[], const Vec4f colours[] = 0);

        void SetTransform(const Mat4f& m);
        void SetTransform(const cTransform& xform);
        void ClearTransform3D();

        void PushTransform(const Mat4f& m);             ///< Push current transform, multiply in m
        void PushTransform(const cTransform& xform);    ///< Push current transform, multiply in xform
        void PopTransform();                            ///< Pop transform stack

        void SetDepthEnabled(bool enabled);

        // Queries
        Vec2f ScreenSize();                     ///< Returns screen size in pixels
        Vec2f FindScreenCoords(Vec3f p, float offsetUp = 0.0f, float offsetRight = 0.0f);   ///< Returns screen coords of a world space location. Offsets in pixels.

        Vec2f TextSize(const char* s);          ///< Returns text width x height in pixels.
        float TextHeight();                     ///< Returns current line height.

        const Mat3f& Transform2();              ///< Returns current 2D transform
        const Mat4f& Transform3();              ///< Returns current 3D transform

    protected:
        Vec2f                   mScreenSize         = vl_0;
        Vec2f                   mInvScreenSize      = vl_0;
        Mat3f                   mTransform2D        = vl_I;
        nCL::vector<Mat3f>      mTransformStack2D;
        Mat4f                   mTransform3D        = vl_I;
        nCL::vector<Mat4f>      mTransformStack3D;
        Mat4f                   mWorldToClip        = vl_I;
        cDebugDrawInternal*     mInternal           = 0;
    };

    cDebugDraw* CreateDebugDraw(nCL::cIAllocator* alloc);

    // --- Draw Utilities ------------------------------------------------------

    // 2D draw utils, coordinates are in screen-space pixels.
    void DrawLine(cDebugDraw* dd, float x1, float y1, float x2, float y2);
    void DrawLine(cDebugDraw* dd, Vec2f from, Vec2f to);

    void DrawRect(cDebugDraw* dd, float x1, float y1, float x2, float y2);
    void DrawRect(cDebugDraw* dd, Vec2f min, Vec2f max);

    void FillRect(cDebugDraw* dd, float x1, float y1, float x2, float y2);
    void FillRect(cDebugDraw* dd, Vec2f min, Vec2f max);

    void DrawCircle  (cDebugDraw* dd, float x1, float y1, float radius);
    void DrawCircle  (cDebugDraw* dd, Vec2f origin,       float radius);

    void FillCircle  (cDebugDraw* dd, float x1, float y1, float radius);
    void FillCircle  (cDebugDraw* dd, Vec2f origin,       float radius);

    void FillGradientRect(cDebugDraw* dd, float x1, float y1, float x2, float y2, const cColourAlpha& c, float s0, float s1);

    Vec2f DrawText (cDebugDraw* dd, float x, float y, const char* str);              ///< Like base dd->DrawText, but handles -ve x/y referencing opposite screen borders. Returns size of str in pixels.
    Vec2f DrawTextF(cDebugDraw* dd, float x, float y, const char* format, ...);
    Vec2f DrawTextCentred (cDebugDraw* dd, float x, float y, const char* str);              ///< Like base dd->DrawText, but handles -ve x/y referencing opposite screen borders. Returns size of str in pixels.
    Vec2f DrawTextCentredF(cDebugDraw* dd, float x, float y, const char* format, ...);

    // 3D draw utils, coordinates are in world space
    void DrawLine(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2);
    void DrawLine(cDebugDraw* dd, Vec3f from, Vec3f to);

    void DrawBox(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2);
    void DrawBox(cDebugDraw* dd, Vec3f min, Vec3f max);

    void FillBox(cDebugDraw* dd, float x1, float y1, float z1, float x2, float y2, float z2);
    void FillBox(cDebugDraw* dd, Vec3f min, Vec3f max);

    void DrawAxes    (cDebugDraw* dd, Vec3f p, float size = 1.0f);
    void DrawFrustum (cDebugDraw* dd, const Mat4f& clipToWorld);

    void DrawCircle  (cDebugDraw* dd, Vec3f origin, float radius, Vec3f normal = vl_z);
    void FillCircle  (cDebugDraw* dd, Vec3f origin, float radius, Vec3f normal = vl_z);

    void FillCone    (cDebugDraw* dd, Vec3f origin, Vec3f normal, float height, float radius);
    void FillCylinder(cDebugDraw* dd, Vec3f origin, Vec3f normal, float height, float radius);

    Vec2f DrawText        (cDebugDraw* dd, Vec3f position, float x, float y, const char* str);            ///< x/y are offsets from p in screen space pixels
    Vec2f DrawTextF       (cDebugDraw* dd, Vec3f position, float x, float y, const char* format, ...);    ///< Format version of DrawText
    Vec2f DrawTextCentred (cDebugDraw* dd, Vec3f position, float x, float y, const char* str);            ///< Draws text centred on p. x/y are offsets from p in screen space pixels.
    Vec2f DrawTextCentredF(cDebugDraw* dd, Vec3f position, float x, float y, const char* format, ...);    ///< Format version of DrawTextCentred


    // --- cDebugTextRect -------------------------------------------------------

    class cDebugTextRect
    /// Helper class for assembling a run of debug text in a display box
    {
    public:
        cDebugTextRect& Clear();                                ///< Clear back to empty rect
        cDebugTextRect& SetColour(tColourRGB c);                ///< Set colour for newly added text
        cDebugTextRect& Print (const char* s);                  ///< Add given line to rect
        cDebugTextRect& Printf(const char* format, ...);        ///< Add given formatted line to rect

        void SetRectParams(tColourRGBA backgroundColour, tColourRGB borderColour = vl_0, float borderPad = 4.0f, float linePad = 0.0f);   ///< Set display params for the background rect.

        void Draw(cDebugDraw* debugDraw, float sx, float sy);                       ///< 2D display
        void Draw(cDebugDraw* debugDraw, Vec3f p, float offsetY, float offsetX);    ///< 3D display anchored to some world point 'p'

    protected:
        void  AddLine(tColourRGB c, const char* format, va_list args);
        Vec2f FindTextSize(cDebugDraw* debugDraw);
        void  DrawTextRect(cDebugDraw* debugDraw, Vec2f ss, Vec2f ts);

        tColourRGB      mTextColour   = vl_1;       ///< Current text colour
        tColourRGBA     mRectColour   = vl_0;       ///< Rectangle colour
        tColourRGB      mBorderColour = vl_0;       ///< Border colour or vl_0 for none
        float           mBorderPad    = 0.0f;       ///< Space between text and rect border
        float           mLinePad      = 0.0f;       ///< Space between lines

        nCL::vector<nCL::string>    mLines;
        nCL::vector<tColourRGB>     mColours;
    };

    // --- Inlines -------------------------------------------------------------

    inline const Mat3f& cDebugDraw::Transform2()
    {
        return mTransform2D;
    }

    inline const Mat4f& cDebugDraw::Transform3()
    {
        return mTransform3D;
    }

    inline void cDebugDraw::SetScreenSize(Vec2f ss)
    {
        mScreenSize = ss;
        mInvScreenSize = Vec2f(vl_one) / mScreenSize;
    }
    inline void cDebugDraw::SetWorldToClip(const Mat4f& m)
    {
        mWorldToClip = m;
    }

    inline Vec2f cDebugDraw::ScreenSize()
    {
        return mScreenSize;
    }

    inline void DrawLine(cDebugDraw* dd, float x1, float y1, float x2, float y2)
    {
        DrawLine(dd, Vec2f(x1, y1), Vec2f(x2, y2));
    }
    inline void DrawRect(cDebugDraw* dd, Vec2f min, Vec2f max)
    {
        DrawRect(dd, min[0], min[1], max[0], max[1]);
    }
    inline void FillRect(cDebugDraw* dd, Vec2f min, Vec2f max)
    {
        FillRect(dd, min[0], min[1], max[0], max[1]);
    }

    inline void DrawCircle  (cDebugDraw* dd, float x1, float y1, float radius)
    {
        DrawCircle(dd, Vec2f(x1, y1), radius);
    }
    inline void FillCircle  (cDebugDraw* dd, float x1, float y1, float radius)
    {
        FillCircle(dd, Vec2f(x1, y1), radius);
    }
}

#endif
#endif
