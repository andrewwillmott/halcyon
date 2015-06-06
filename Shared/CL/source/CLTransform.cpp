//
//  File:       CLTransform.cpp
//
//  Function:   Representation of a standard Scale/Rotate/Translate transformation.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLTransform.h>

#include <CLVecUtil.h>
#include <CLMatUtil.h>  // for HasNAN

using namespace nCL;

namespace
{
    inline bool IsIdentityRotation(const Mat3f& m)
    {
        // For an orthonormal (rotation) matrix only
        // Essentially, if x && y are within 1 degree of normal, assume identity
        return m(0, 0) > 0.9999f && m(1, 1) > 0.9999f;
    }
}


bool cTransform::IsIdentity() const
{
    return mScale == 1.0f && IsIdentityRotation(mRotation) && mTranslation == vl_0;
}

void cTransform::TransformPoints(int count, const Vec3f* p, size_t stride, Vec3f* pOut, size_t outStride) const
{
    if (!IsIdentityRotation(mRotation))
    {
        for (int i = 0; i < count; i++)
        {
            *pOut = (*p * mScale) * mRotation + mTranslation;

            (uint8_t*&) p += stride;
            (uint8_t*&) pOut += outStride;
        }

        return;
    }

    bool hasS = mScale != 1.0f;
    bool hasT = mTranslation != vl_zero;

    if (hasS && hasT)
    {
        for (int i = 0; i < count; i++)
        {
            *pOut = (*p * mScale) + mTranslation;
            
            (uint8_t*&) p += stride;
            (uint8_t*&) pOut += outStride;
        }
    }
    else if (hasS)
    {
        for (int i = 0; i < count; i++)
        {
            *pOut = *p * mScale;
            
            (uint8_t*&) p += stride;
            (uint8_t*&) pOut += outStride;
        }
    }
    else if (hasT)
    {
        for (int i = 0; i < count; i++)
        {
            *pOut = *p + mTranslation;
            
            (uint8_t*&) p += stride;
            (uint8_t*&) pOut += outStride;
        }
    }
    else if (pOut != p)
    {
        for (int i = 0; i < count; i++)
        {
            *pOut = *p;
            
            (uint8_t*&) p += stride;
            (uint8_t*&) pOut += outStride;
        }
    }
}

void cTransform::TransformVectors(int count, const Vec3f* v, size_t stride, Vec3f* vOut, size_t outStride) const
{
    if (!IsIdentityRotation(mRotation))
    {
        for (int i = 0; i < count; i++)
        {
            *vOut = (*v * mScale) * mRotation;

            (uint8_t*&) v += stride;
            (uint8_t*&) vOut += outStride;
        }
    }
    else if (mScale != 1.0f)
    {
        for (int i = 0; i < count; i++)
        {
            *vOut = *v * mScale;
            
            (uint8_t*&) v += stride;
            (uint8_t*&) vOut += outStride;
        }
    }
    else if (v != vOut)
    {
        for (int i = 0; i < count; i++)
        {
            *vOut = *v;
            
            (uint8_t*&) v += stride;
            (uint8_t*&) vOut += outStride;
        }
    }
}

void cTransform::TransformDirections(int count, const Vec3f* d, size_t stride, Vec3f* dOut, size_t outStride) const
{
    if (!IsIdentityRotation(mRotation))
    {
        for (int i = 0; i < count; i++)
        {
            *dOut = *d * mRotation;

            (uint8_t*&) d += stride;
            (uint8_t*&) dOut += outStride;
        }
    }
    else if (dOut != d)
    {
        for (int i = 0; i < count; i++)
        {
            *dOut = *d;

            (uint8_t*&) d += stride;
            (uint8_t*&) dOut += outStride;
        }
    }
}

// TODO: version in MatUtil uses Matf.
namespace
{
    void FindAbs(const Mat3f& m, Mat3f* absM)
    {
        const float* e  = m.ConstRef();
        float*       ae = absM->Ref();

        for (int i = 0, n = m.Elts(); i < n; i++)
            ae[i] = fabsf(e[i]);
    }
}

cBounds3 cTransform::TransformBounds(const cBounds3& bbox) const
{
    cBounds3 result;

    if (!IsIdentityRotation(mRotation))
    {
        Vec3f width   = bbox.Width();
        Vec3f centre  = bbox.Centre();
        float halfScale = mScale * 0.5f;

        Mat3f absRot;
        FindAbs(mRotation, &absRot);

        width *= halfScale;
        width = width * absRot;

        centre *= mScale;
        centre *= mRotation;
        centre += mTranslation;

        return cBounds3(centre - width, centre + width);
    }
    else
        return cBounds3(bbox.mMin * mScale + mTranslation, bbox.mMax * mScale + mTranslation);
}

cTransform cTransform::Inverse() const
{
    cTransform t;

    t.mScale       = 1.0f / mScale;
    t.mRotation    = trans(mRotation);
    t.mTranslation = (mTranslation * -t.mScale) * t.mRotation;

    return t;
}

void cTransform::MakeMat4(Mat4f* m) const
{
    (*m)[0] = Vec4f(mRotation[0] * mScale, 0.0f);
    (*m)[1] = Vec4f(mRotation[1] * mScale, 0.0f);
    (*m)[2] = Vec4f(mRotation[2] * mScale, 0.0f);
    (*m)[3] = Vec4f(mTranslation, 1.0f);
}

cTransform nCL::Transform(const cTransform& a, const cTransform& b)
{
    cTransform result;

    result.mTranslation = a.mTranslation + (b.mTranslation * a.mScale) * a.mRotation;
    result.mRotation    = b.mRotation * a.mRotation;
    result.mScale       = a.mScale * b.mScale;

    return result;
}

bool nCL::HasNAN(const cTransform& t)
{
    return IsNAN(t.mScale) || HasNAN(t.mTranslation) || HasNAN(t.mRotation);
}
