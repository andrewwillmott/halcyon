/*
    File:           CLColour.cpp

    Function:       Implements CLColour.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          Useful info on colour (Gamma & cColour FAQs):                    
                        http://www.inforamp.net/~poynton/notes/colour_and_gamma
                    Useful info on colour transforms:
                        http://www.sgi.com/grafica/matrix/index.html
                        http://www.sgi.com/grafica/interp/index.html
*/

#include <CLColour.h>
#include <CLVecUtil.h>

using namespace nCL;

/** \typedef ClrMat
    A colour basis transformation matrix.
    Used for converting between tri-stimulus colour spaces.
*/
/** \typedef ClrTrans
    A colour transformation. 
    Can be applied to an Image with Image::Transform.
*/
/** \typedef ClrReal
    The type used for colour components.
*/

cColour nCL::Clip(cColour c)
{
    cColour cc;

    if      (c[0] >= 1.0f)
        cc[0] = 1.0f;
    else if (c[0] <= 0.0f)
        cc[0] = 0.0f;
    else
        cc[0] = c[0];

    if      (c[1] >= 1.0f)
        cc[1] = 1.0f;
    else if (c[1] <= 0.0f)
        cc[1] = 0.0f;
    else
        cc[1] = c[1];

    if      (c[2] >= 1.0f)
        cc[2] = 1.0f;
    else if (c[2] <= 0.0f)
        cc[2] = 0.0f;
    else
        cc[2] = c[2];

    return cc;
}

cColour nCL::ClipZero(cColour c)
{
    cColour cc;

    if (c[0] <= 0.0f)
        cc[0] = 0.0f;
    else
        cc[0] = c[0];

    if (c[1] <= 0.0f)
        cc[1] = 0.0f;
    else
        cc[1] = c[1];

    if (c[2] <= 0.0f)
        cc[2] = 0.0f;
    else
        cc[2] = c[2];


    return cc;
}

cColour nCL::HSVColour(ClrReal hue, ClrReal saturation, ClrReal value)
// Convert HSV to RGB...
{
    hue = hue - 360 * floorf(hue / 360.0f);

    int         face = (int) floorf(hue / 120.0f);
    ClrReal     d = (hue - 120.0f * face) / 60.0f;
    cColour     result;

    if (face == 3)
        face--;
    
    result[face] = value;
    
    if (d > 1)
    {
        result[(face + 1) % 3] = value * (1.0f - saturation * (2.0f - d));
        result[(face + 2) % 3] = value * (1.0f - saturation);
    }
    else
    {
        result[(face + 1) % 3] = value * (1.0f - saturation);
        result[(face + 2) % 3] = value * (1.0f - saturation * d);
    }

    return result;
}

cColour nCL::HueColour(ClrReal hue)
// Convert HSV to RGB...
{
    hue = hue - 360 * floorf(hue / 360.0f);

    int     face = (int) floorf(hue / 120.0f);
    ClrReal d = (hue - 120.0f * face) / 60.0f;
    cColour result;

    if (face == 3)
        face--;
    
    result[ face         ] = 1.0f;
    result[(face + 1) % 3] = fabsf(d - 1.0f);
    result[(face + 2) % 3] = 0.0f;

    return result;
}

namespace
{
    inline float Halton2(uint32_t a)
    {
        uint32_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);
        a = ((b & 0x00FF00FF) << 8)  | ((b & 0xFF00FF00) >> 8);

        union
        {
            uint32_t ru;
            float rf;
        } result;

        result.ru = 0x3f800000 | (a << 7);
        return result.rf - 1.0f;
    }
}

cColour nCL::LabelColour(int i, ClrReal saturation, ClrReal value)
{
    return HSVColour(Halton2(i) * 360.0f, saturation, value);
}
cColour nCL::LabelColour(int i)
{
    return HueColour(Halton2(i) * 360.0f);
}

// --- cColour transformations ----------------

ClrTrans nCL::RGBScale(ClrReal rscale, ClrReal gscale, ClrReal bscale)
{
    return HScale4f(cColour(rscale, gscale, bscale));
}

ClrTrans nCL::RGBToLum()
{
    ClrTrans result;
    
#ifdef VL_ROW_ORIENT
    result[0] = Vec4f(Vec3f(kRGBToLinearLum[0]), 0.0f);
    result[1] = Vec4f(Vec3f(kRGBToLinearLum[1]), 0.0f);
    result[2] = Vec4f(Vec3f(kRGBToLinearLum[2]), 0.0f);
    result[3] = vl_w;
#else
    result[0] = Vec4f(kRGBToLinearLum, 0.0f);
    result[1] = Vec4f(kRGBToLinearLum, 0.0f);
    result[2] = Vec4f(kRGBToLinearLum, 0.0f);
    result[3] = vl_w;
#endif

    return result;
}

ClrTrans nCL::RGBComplement()
{
    // Basically RGBSaturate(-1) with 1/3 greyscale convert
    ClrTrans result;

    result.MakeBlock(2.0f / 3.0f);
    result(0, 0) -= 1.0f;
    result(1, 1) -= 1.0f;
    result(2, 2) -= 1.0f;
    result(3, 3) =  1.0f;

    return result;
}

ClrTrans nCL::RGBSaturate(ClrReal sat)
{
    // saturate by interpolating with greyscale pixel
    return lerp(RGBToLum(), ClrTrans(vl_I), sat);
}

ClrTrans nCL::RGBReplace(const cColour& c)
// replace with colour c
{
    ClrTrans result = vl_0;

#ifdef VL_ROW_ORIENT
    result[3] = { c, 1.0f };
#else
    result[0][3] = c[0];
    result[1][3] = c[1];
    result[2][3] = c[2];
    result[3][3] = 1.0f;
#endif

    return result;
}

ClrTrans nCL::RGBPixelMix(ClrReal mix, const cColour& c)
{
    return lerp(RGBReplace(c), ClrTrans(vl_I), mix);
}

ClrTrans nCL::RGBOffset(const cColour& c)
{
    // Simple translation
    return HTrans4f(c);
}

ClrTrans nCL::RGBHueRotate(ClrReal degrees)
{
    // align grey vector with z axis (vl_z)
    ClrTrans trans = HRot4f(norm(cColour(1, -1, 0)), vl_quarterPi);

    // what is the luminance vector in the new space?
    cColour newLum = xform(trans, kRGBToLinearLum);
    // shear so plane parallel to this vector is now horizontal (x-y)
    // if dot(x, newLum) = c, (shear * x)[2] * newLum[2] = c,
    // i.e. is constant
    ClrTrans shear = vl_I;
#ifdef VL_ROW_ORIENT
    shear(0, 2) = newLum[0] / newLum[2];
    shear(1, 2) = newLum[1] / newLum[2];
#else
    shear(2, 0) = newLum[0] / newLum[2];
    shear(2, 1) = newLum[1] / newLum[2];
#endif

    trans = xform(shear, trans);

    // rotate the hue!
    trans = xform(HRot4f(vl_z, DegreesToRadians(degrees)), trans);

    // unshear
#ifdef VL_ROW_ORIENT
    shear(0, 2) = -shear(0, 2);
    shear(1, 2) = -shear(1, 2);
#else
    shear(2, 0) = -shear(2, 0);
    shear(2, 1) = -shear(2, 1);
#endif

    trans = xform(shear, trans);

    // unalign
    trans = xform(HRot4f(norm(cColour(1, -1, 0)), -vl_quarterPi), trans);

    return trans;
}


// Note: all these conversions should be applied only to gamma-corrected
// (non-linear) colours. The y component is luma -- non-linear version
// of CIE luminance.
// 
// Generally the errors resulting in transforming from one RGB system
// to another in non-linear space are insignificant, because most
// terms are close to zero or one. Not so for a conversion to
// something like CIE XYZ.


/* 
    YIQ is just YUV rotated 33 degrees. It is the colour system used
    by NTSC. (IQ are combined with quadrature modulation, hence the Q.)

    U = 0.492(B'-Y)
    V = 0.877(R'-Y)

    I = V cos(33)-U sin(33)=0.736(R'-Y)-0.258(B'-Y)
    Q = V sin(33)+U cos(33)=0.478(R'-Y)-0.413(B'-Y)

    Chroma signal:
        F = 3.579545 Mhz
        w = 2 pi F
        C = Icos(wt+33) + Qsin(wt+33)
        (33 degrees)
        227.5 cycles per horizontal line
        amplitude is saturation, phase offset is hue
        composite = Y + C

    Composite is clamped to -0.2 to 1.2 for historical reasons,
    so highly saturated colours will clip, if you're lucky. If
    you're not, they'll be misinterpreted as a control code,
    or make their way into the sound channel. Whee!
*/

namespace
{
    const Real kCos33 = cos(33 * vl_pi / 180);
    const Real kSin33 = sin(33 * vl_pi / 180);
}

cColour nCL::RGBToYUV(cColour rgb)
{
    cColour yuv;
    yuv[0] = dot(kRGBToLuma, rgb);
    yuv[1] = 0.492 * (rgb[2] - yuv[0]);
    yuv[2] = 0.877 * (rgb[0] - yuv[0]);
    return yuv;
}

cColour nCL::YUVToYIQ(cColour yuv)
{
    cColour yiq;
    
    yiq[0] = yuv[0];
    // rotate by 33 degrees
    yiq[1] = kCos33 * yuv[2] - kSin33 * yuv[1];
    yiq[2] = kSin33 * yuv[2] + kCos33 * yuv[1];
    
    return yiq;
}

cColour nCL::YIQToYUV(cColour yiq)
{
    cColour yuv;
    
    yuv[0] = yiq[0];
    // rotate by -33 degrees
    yuv[2] =  kCos33 * yiq[1] + kSin33 * yiq[2];
    yuv[1] = -kSin33 * yiq[1] + kCos33 * yiq[2];
    
    return yuv;
}

bool nCL::IsHotColour(cColour rgb, Real maxExcursion)
{
    cColour yiq = kRGBdToYIQd * rgb;
    
    // amplitude of the chroma component.
    Real c = sqrt(sqr(yiq[1]) + sqr(yiq[2]));
    Real y = yiq[0];

    return (y + c > maxExcursion) || (y - c < -maxExcursion);
}

void nCL::DesaturateHotColour(cColour& rgb, Real maxExcursion)
{
    cColour yiq = kRGBdToYIQd * rgb;
    
    // amplitude of the chroma component.
    Real c = sqrt(sqr(yiq[1]) + sqr(yiq[2]));
    Real y = yiq[0];
    
    if (y + c > 1.0 + maxExcursion)
    {
        Real f = ( 0.999 + maxExcursion - y) / c;
        // desaturate so y + c is small enough
        yiq[1] *= f;
        yiq[2] *= f;
        
        rgb = cYIQdToRGBd * yiq;
    }
    else if (y - c < -maxExcursion)
    {
        Real f = (-0.001 + maxExcursion + y) / c;
        // desaturate so y - c is big enough
        yiq[1] *= f;
        yiq[2] *= f;

        rgb = cYIQdToRGBd * yiq;
    }
}


cColourAlpha nCL::RGBToCMYK(cColour rgb)
/// This routine uses the same technique as Postscript Level 2. See the reference manual, page 306.
{
    cColourAlpha cmyk =
    {
        1.0f - rgb[0],
        1.0f - rgb[1],
        1.0f - rgb[2],
        1.0f - MaxCmpt(rgb)
    };

    // assume the identity function for UCR and BG
    cmyk.AsColour() -= Vec3f(cmyk.AsAlpha());

    return cmyk;
}

cColour nCL::CMYKToRGB(cColourAlpha cmyk)
/// This routine uses the same technique as Postscript Level 2. See the reference manual, page 306.
{
    cColour rgb;

    cmyk.AsColour() += Vec3f(cmyk.AsAlpha());

    rgb[0] = 1.0f - cmyk[0];
    rgb[1] = 1.0f - cmyk[1];
    rgb[2] = 1.0f - cmyk[2];

    return rgb;
}

cColour nCL::RGBToCMY(cColour rgb)
{
    cColour cmy =
    {
        1.0f - rgb[0],
        1.0f - rgb[1],
        1.0f - rgb[2]
    };

    return cmy;
}

cColour nCL::CMYToRGB(cColour cmy)
{
    cColour rgb;

    rgb[0] = 1.0f - cmy[0];
    rgb[1] = 1.0f - cmy[1];
    rgb[2] = 1.0f - cmy[2];

    return rgb;
}

namespace
{
    const float kLMThreshold0 = (6.0 / 29.0);
    const float kLMThreshold1 = (6.0 / 29.0) * (6.0 / 29.0) * (6.0 / 29.0);
    const float kLM0 = (29.0 / 6.0) * (29.0 / 6.0) * (1.0 / 3.0);
    const float kLM1 = 4.0 / 29.0;
    const float kInvLM0 = 1.0 / kLM0;

    inline float LabMap(float t)
    {
        if (t > kLMThreshold0)
            return cbrt(t);

        return kLM0 * t + kLM1;
    }

    inline float InvLabMap(float t)
    {
        if (t > kLMThreshold1)
            return t * t * t;

        return (t - kLM1) * kInvLM0;
    }
}

cColour nCL::CIEXYZToLab(cColour xyz)
{
    float xm = LabMap(xyz[0]);
    float ym = LabMap(xyz[1]);
    float zm = LabMap(xyz[2]);

    float L = 116.0f * ym - 16.0f;
    float a = 500.0f * (xm - ym);
    float b = 200.0f * (ym - zm);

    return cColour(L, a, b);
}

cColour nCL::LabToCIEXYZ(cColour Lab)
{
    float Lm = (Lab[0] + 16.0f) * (1.0f / 116.0f);
    float am =  Lab[1]          * (1.0f / 500.0f);
    float bm =  Lab[2]          * (1.0f / 200.0f);

    float xm = InvLabMap(Lm);
    float ym = InvLabMap(Lm + am);
    float zm = InvLabMap(Lm - bm);

    return cColour(xm, ym, zm);
}

namespace
{
    const int kRGBE_Excess = 128;
}

cColour nCL::RGBEToColour(cRGBE32 rgbe)
{
    if (rgbe.mChannel[3] == 0)
        return kColourBlack;

    Real f = ldexp(1.0, (int) rgbe.mChannel[3] - (kRGBE_Excess + 8));

    cColour  result;
    
    result[0] = f * (rgbe.mChannel[0] + 0.5);
    result[1] = f * (rgbe.mChannel[1] + 0.5);
    result[2] = f * (rgbe.mChannel[2] + 0.5);

    return result;
}

cRGBE32 nCL::ColourToRGBE(cColour c)
{
    cRGBE32  result;
    
    Real d = MaxCmpt(c);
    
    if (d <= 1e-32)
    {
        result.mAsUInt32 = 0;
    }
    else
    {
        int e;
        
        d = frexp(d, &e) * 256.0 / d;
        
        result.mChannel[0] = (uint8_t) (c[0] * d);
        result.mChannel[1] = (uint8_t) (c[1] * d);
        result.mChannel[2] = (uint8_t) (c[2] * d);
        result.mChannel[3] = e + kRGBE_Excess;
    }
    
    return result; 
}

cRGBE32 nCL::RGBEMult(cRGBE32 rgbe, Real s)
{
    cColour      temp;
    cRGBE32  result;
    
    result.mAsUInt32 = 0;

    if (rgbe.mChannel[3] == 0)
        return result;

    Real f = ldexp(1.0, (int) rgbe.mChannel[3] - (kRGBE_Excess + 8));

    temp[0] = f * (rgbe.mChannel[0] + 0.5);
    temp[1] = f * (rgbe.mChannel[1] + 0.5);
    temp[2] = f * (rgbe.mChannel[2] + 0.5);

    Real d = MaxCmpt(temp);
    if (d <= 1e-32)
        return result;
    else
    {
        int e;
        d = frexp(d, &e) * 256.0 / d;
        
        result.mChannel[0] = (uint8_t) (temp[0] * d);
        result.mChannel[1] = (uint8_t) (temp[1] * d);
        result.mChannel[2] = (uint8_t) (temp[2] * d);
        result.mChannel[3] = e + kRGBE_Excess;
    }

    return result; 
}
