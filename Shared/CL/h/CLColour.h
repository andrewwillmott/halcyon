//
//  File:       CLColour.h
//
//  Function:   Definitions and classes for handling tri-stimulus colours
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1995-2000, Andrew Willmott
//

#ifndef CL_COLOUR_H
#define CL_COLOUR_H

#include <CLDefs.h>
#include <CLMath.h> // For RealToUint8 etc.
#include <VL234f.h>

namespace nCL
{
    typedef Mat3f ClrMat;
    typedef Mat4f ClrTrans;     // See colour transformations below.
    typedef float ClrReal;

    // --- Colours -------------------------------------------------------------

    struct cColour : Vec3f
    /// A tristimulous colour
    {
    public:
        cColour()                   : Vec3f(vl_0) {}
        cColour(float s)            : Vec3f(s) {}
        cColour(const Vec3f& c)     : Vec3f(c) {}
        cColour(float a, float b, float c) : Vec3f(a, b, c) {}
    };

    struct cColourAlpha : Vec4f
    /// A cColour with an alpha channel.
    {
    public:
        cColourAlpha()                                : Vec4f(vl_w) {}
        cColourAlpha(const cColour& c)                : Vec4f(c, ClrReal(1)) {}
        cColourAlpha(const cColour& c, ClrReal alpha) : Vec4f(c, alpha) {}
        cColourAlpha(ClrReal a, ClrReal b, ClrReal c) : Vec4f(a, b, c, ClrReal(1)) {}
        cColourAlpha(ClrReal a, ClrReal b, ClrReal c, ClrReal alpha) : Vec4f(a, b, c, alpha) {}

        cColour& AsColour()       { return (cColour&) *this; }
        cColour  AsColour() const { return (const cColour&) *this; }
        float&   AsAlpha()       { return SELF[3]; }
        float    AsAlpha() const { return SELF[3]; }

        cColourAlpha& operator = (const cColour& c)
        { SELF = cColourAlpha(c); return SELF; }
    };


    // --- Useful colour constants ------------------------------------------------

    // RGB Constants

    const cColour kColourBlack(0.0f);
    const cColour kColourWhite(1.0f);

    const cColour kColourGrey  (0.5f);
    const cColour kColourGrey05(0.05);
    const cColour kColourGrey10(0.1);
    const cColour kColourGrey25(0.25);
    const cColour kColourGrey50(0.5);
    const cColour kColourGrey75(0.75);
    const cColour kColourGrey90(0.90);
    const cColour kColourGrey95(0.95);

    const cColour kColourRed   (1.0f, 0.0f, 0.0f);
    const cColour kColourOrange(1.0f, 0.5f, 0.0f);
    const cColour kColourYellow(1.0f, 1.0f, 0.0f);
    const cColour kColourGreen (0.0f, 1.0f, 0.0f);
    const cColour kColourCyan  (0.0f, 1.0f, 1.0f);
    const cColour kColourBlue  (0.0f, 0.0f, 1.0f);
    const cColour kColourPurple(1.0f, 0.0f, 1.0f);

    const cColourAlpha kColourBlackA0(0.0f, 0.0f);
    const cColourAlpha kColourBlackA1(0.0f, 1.0f);
    const cColourAlpha kColourWhiteA1(1.0f, 1.0f);

    const cColourAlpha kColourRedA1   (1.0f, 0.0f, 0.0f);
    const cColourAlpha kColourOrangeA1(1.0f, 0.5f, 0.0f);
    const cColourAlpha kColourYellowA1(1.0f, 1.0f, 0.0f);
    const cColourAlpha kColourGreenA1 (0.0f, 1.0f, 0.0f);
    const cColourAlpha kColourCyanA1  (0.0f, 1.0f, 1.0f);
    const cColourAlpha kColourBlueA1  (0.0f, 0.0f, 1.0f);
    const cColourAlpha kColourPurpleA1(1.0f, 0.0f, 1.0f);

    // HSV constants
    const int kHSVRed    =    0;
    const int kHSVOrange =   30;
    const int kHSVYellow =   60;
    const int kHSVGreen  =  120;
    const int kHSVCyan   =  180;
    const int kHSVBlue   =  240;
    const int kHSVPurple =  300;


    // --- cColour transformations -------------------------------------------------

    ClrTrans RGBScale(ClrReal rscale, ClrReal gscale, ClrReal bscale);  ///< Scale rgb components
    ClrTrans RGBOffset(const cColour& c);    ///< Straight rgb shift
    ClrTrans RGBReplace(const cColour& c);   ///< Replace any input with colour c
    ClrTrans RGBToLum();                    ///< Convert to greyscale
    ClrTrans RGBComplement();               ///< Complement colour

    ClrTrans RGBSaturate(ClrReal sat);
    ///< Saturate image
    ///<    sat < 1.0 decreases saturation towards 0.0 (greyscale),
    ///<    sat > 1.0 increases it.
    ClrTrans RGBPixelMix(ClrReal mix, const cColour& c);
    ///< Mix with the given colour. If you use c = kColourBlack, mix < 1.0 will darken the image,
    ///< and mix > 1.0 will lighten it.
    ///< If you use the average colour of an image, c = img.AverageColour(),
    ///< mix < 1.0 will lower contrast, mix > 1.0 raise it.
    ClrTrans RGBHueRotate(ClrReal degrees);
    ///< Rotate hue in HSV space.


    // --- Packed Colours ------------------------------------------------------

    enum tRGBAChannel
    {
        kRGBA_R = 0,
        kRGBA_G = 1,
        kRGBA_B = 2,
        kRGBA_A = 3
    };

    struct cRGBA32
    {
        union
        {
            uint8_t  mChannel[4];
            uint32_t mAsUInt32;
        };
    };

    const cRGBA32 kRGBA32BlackA1 = {   0,   0,   0, 255 };
    const cRGBA32 kRGBA32WhiteA1 = { 255, 255, 255, 255 };

    struct cRGBE32 : cRGBA32 {};

    cColour RGBColour  (ClrReal r, ClrReal g, ClrReal b);
    cColour HSVColour  (ClrReal hue, ClrReal saturation, ClrReal value);
    cColour HueColour  (ClrReal hue);   ///< Full saturation/value for convenience
    cColour LabelColour(int i, ClrReal saturation, ClrReal value);     ///< Returns successive colours that are well separated from each other.
    cColour LabelColour(int i);   ///< Full saturation/value for convenience

    Vec3f   RGBA32ToColour(cRGBA32 rgba);
    cRGBA32 ColourToRGBA32(Vec3f c);

    Vec4f   RGBA32ToColourAlpha(cRGBA32 rgba);
    cRGBA32 ColourAlphaToRGBA32(Vec4f c);

    cColour U8ToColour(uint8_t b);
    uint8_t ColourToU8(cColour c);

    cColour RGBEToColour(cRGBE32 rgbe);
    cRGBE32 ColourToRGBE(cColour c);
    cRGBE32 RGBEMult(cRGBE32 rgbe, Real s);

    cColour Clip    (cColour c);    ///< Clips colour to [0, 1] for display
    cColour ClipZero(cColour c);    ///< Clips colour to 0 or positive values only.
    cColour Mix     (cColour c1, cColour c2, ClrReal mix);  ///< Mix two colours


    // --- Useful conversion factors/functions ------------------------------------


    const cColour kRGBToNTSCLum(0.299f, 0.587f, 0.114f);
    const cColour kRGBToLuma   (0.299f, 0.587f, 0.114f);
    ///< Converts RGB to greyscale, assuming we're in an NTSC colourspace with gamma = 2.2

    const cColour kRGBToLinearLum(0.3086f, 0.6094f, 0.0820f);
    const cColour kRGBToLuminance(0.3086f, 0.6094f, 0.0820f);
    ///< Convert RGB to greyscale, assuming a linear colour space.

    const ClrMat cNTSCToCIE
    (
        0.67f, 0.21f, 0.14f,
        0.33f, 0.71f, 0.08f,
        0.00f, 0.08f, 0.78f
    ); 

    const ClrMat cCIEToNTSC
    (
        1.730f, -0.482f, -0.261f,
       -0.814f,  1.652f, -0.023f,
        0.083f, -0.169f,  1.284f
    ); 


    // Note: the 'd's below mean non-linear space. (Gamma-corrected.)
    // YdPbPr has component ranges Yd = [0, 1]; Pb, Pr = [-0.5, 0.5]
    const ClrMat cYdPbPrToRGBd
    (
        1.0f,  0.0f,     1.4075f,
        1.0f, -0.3455f, -0.7169f,
        1.0f,  1.7790f,  0.0f
    );

    const ClrMat kRGBdToYdPbPr
    (
        0.299f,   0.587f,   0.114f,
        0.5000f, -0.4187f, -0.0813f,
       -0.1687f, -0.3313f,  0.500f
    );

    // Useful for NTSC/PAL hot colour detection and
    // removal, but should not be used for anything else.
    const ClrMat kRGBdToYIQd
    (
        0.299f,  0.587f,  0.114f,
        0.596f, -0.275f, -0.321f,
        0.212f, -0.523f,  0.311f
    );

    const ClrMat cYIQdToRGBd
    (
        1.0,   0.956,   0.621,
        1.0,  -0.272,  -0.647,
        1.0,  -1.105,   1.702
    );

    // Note: YdCbCr is YdPbPr with the components biased and scaled to fit into
    // 0-255 with some headroom.

    // Preparing colours for TV
    bool IsHotColour(cColour rgb, Real maxExcursion = 0.2f);
    void DesaturateHotColour(cColour& rgb, Real maxExcursion = 0.2f);

    // Rough conversions only -- real conversions require
    // dealing with spectra.
    cColourAlpha RGBToCMYK(cColour rgb);    // Returns CMY + K in alpha
    cColour      CMYKToRGB(cColourAlpha cmyk);
    cColour      RGBToCMY(cColour rgb);
    cColour      CMYToRGB(cColour cmy);

    cColour CIEXYZToLab(cColour xyz);   // XYZ assumed to be already normalised by the white point
    cColour LabToCIEXYZ(cColour Lab);   // Returns normalised XYZ, multiply by white point to get final colour.

    // Academic interest only
    cColour RGBToYUV(cColour rgb);
    cColour YUVToYIQ(cColour yuv);
    cColour YIQToYUV(cColour yiq);
}

// --- inlines ----------------------------------------------------------------

inline nCL::cColour nCL::RGBColour(ClrReal r, ClrReal g, ClrReal b)
{ 
   return cColour(r, g, b);
}

inline Vec3f nCL::RGBA32ToColour(cRGBA32 rgba)
{
    Vec3f result;

    result[0] = UInt8ToUnitReal(rgba.mChannel[kRGBA_R]);
    result[1] = UInt8ToUnitReal(rgba.mChannel[kRGBA_G]);
    result[2] = UInt8ToUnitReal(rgba.mChannel[kRGBA_B]);

    return result;
}

inline nCL::cRGBA32 nCL::ColourToRGBA32(Vec3f c)
{
    cRGBA32 result;

    result.mChannel[kRGBA_R] = ClampUnitRealToUInt8(c[0]);
    result.mChannel[kRGBA_G] = ClampUnitRealToUInt8(c[1]);
    result.mChannel[kRGBA_B] = ClampUnitRealToUInt8(c[2]);
    result.mChannel[kRGBA_A] = 255;

    return result; 
}

inline Vec4f nCL::RGBA32ToColourAlpha(cRGBA32 rgba)
{
    Vec4f result;

    result[0] = UInt8ToUnitReal(rgba.mChannel[kRGBA_R]);
    result[1] = UInt8ToUnitReal(rgba.mChannel[kRGBA_G]);
    result[2] = UInt8ToUnitReal(rgba.mChannel[kRGBA_B]);
    result[3] = UInt8ToUnitReal(rgba.mChannel[kRGBA_A]);

    return result;
}

inline nCL::cRGBA32 nCL::ColourAlphaToRGBA32(Vec4f c)
{
    cRGBA32 result;

    result.mChannel[kRGBA_R] = ClampUnitRealToUInt8(c[0]);
    result.mChannel[kRGBA_G] = ClampUnitRealToUInt8(c[1]);
    result.mChannel[kRGBA_B] = ClampUnitRealToUInt8(c[2]);
    result.mChannel[kRGBA_A] = ClampUnitRealToUInt8(c[3]);

    return result; 
}

inline nCL::cColour nCL::U8ToColour(uint8_t b)
{
    cColour  result;

    ClrReal temp = UInt8ToUnitReal(b);

    result[0] = temp;
    result[1] = temp;
    result[2] = temp;

    return result;
}

inline uint8_t nCL::ColourToU8(cColour c)
{
    ClrReal temp;

    temp = c[0] + c[1] + c[2];

    return ClampUnitRealToUInt8(temp * (1.0 / 3.0));
}

inline nCL::cColour nCL::Mix(cColour c1, cColour c2, ClrReal mix)
{
    return c1 * (1.0f - mix) + c2 * mix;
}

#endif
