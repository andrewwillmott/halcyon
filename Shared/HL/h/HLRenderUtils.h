//
//  File:       HLRenderUtils.h
//
//  Function:   Render support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_RENDER_UTILS_H
#define HL_RENDER_UTILS_H

#include <HLDefs.h>

#include <CLBounds.h>
#include <CLColour.h>

namespace nHL
{
    class cIRenderer;

    void DrawRect(cIRenderer* renderer, nCL::cRGBA32 c, const Vec2f vp[4]);
    void DrawRect(cIRenderer* renderer, nCL::cRGBA32 c, const Vec4f& r);
    void DrawRect(cIRenderer* renderer, nCL::cRGBA32 colour);

    void DrawRectFlipY(cIRenderer* renderer, nCL::cRGBA32 colour);

    void DrawScreenRect(cIRenderer* renderer, nCL::cRGBA32 c, const cBounds2& rect);                    ///< Draw rect given in screen coords (pixels)
}

#endif
