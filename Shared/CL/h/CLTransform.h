//
//  File:       CLTransform.h
//
//  Function:   Representation of a standard Scale/Rotate/Translate transformation.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_TRANSFORM_H
#define CL_TRANSFORM_H

#include <CLBounds.h>

namespace nCL
{
    /// Helper for manipulating standard (uniform) scale/rotate/translate transforms.
    struct cTransform
    {
        cTransform();
        explicit cTransform(Vec3f t);
        cTransform(float s, Vec3f t);
        cTransform(float s, const Mat3f& r, Vec3f t);

        // Access
        float        Scale() const;
        const Mat3f& Rot  () const;
        const Vec3f& Trans() const;

        bool         IsIdentity() const;

        // Manipulation
        void MakeIdentity();

        void SetScale(float s);
        void SetRot  (const Mat3f& m);
        void SetRot  (const Vec3f& axis, Real theta);
        void SetRot  (const Vec4f& q);                 // Rotate by quaternion
        void SetTrans(Vec3f t);

        cTransform& Prepend(const cTransform& transform);
        cTransform&  Append(const cTransform& transform);

        cTransform& PrependScale(float scale);
        cTransform&  AppendScale(float scale);

        cTransform& PrependRot(const Mat3f& rotation);
        cTransform&  AppendRot(const Mat3f& rotation);

        cTransform& PrependTrans(Vec3f translation);
        cTransform&  AppendTrans(Vec3f translation);

        // Specialised axis rotations, as they're much cheaper than full rotations
        cTransform& PrependRotX(float angle);
        cTransform& PrependRotY(float angle);
        cTransform& PrependRotZ(float angle);
        cTransform&  AppendRotX(float angle);
        cTransform&  AppendRotY(float angle);
        cTransform&  AppendRotZ(float angle);

        // Transformations
        Vec3f    TransformPoint    (Vec3f p) const;
        Vec3f    TransformVector   (Vec3f v) const;
        Vec3f    TransformDirection(Vec3f d) const;

        void     TransformPoints    (int count, Vec3f p[]) const;
        void     TransformVectors   (int count, Vec3f v[]) const;
        void     TransformDirections(int count, Vec3f d[]) const;

        void     TransformPoints    (int count, const Vec3f* posIn, size_t posInStride, Vec3f* posOut, size_t posOutStride) const;
        void     TransformVectors   (int count, const Vec3f* velIn, size_t velInStride, Vec3f* velOut, size_t velOutStride) const;
        void     TransformDirections(int count, const Vec3f* dirIn, size_t dirInStride, Vec3f* dirOut, size_t dirOutStride) const;

        float    TransformBounds(float radius) const;               ///< Transform bounding sphere
        cBounds3 TransformBounds(const cBounds3& bbox) const;       ///< Transform bounding box

        Vec3f    BackTransformPoint    (Vec3f p) const;
        Vec3f    BackTransformVector   (Vec3f v) const;
        Vec3f    BackTransformDirection(Vec3f d) const;

        // Misc
        Vec3f       Axis(int i) const;          ///< Return transformed direction for given axis, e.g., Axis(vl_z)
        cTransform  Inverse() const;            ///< Return inverse transform
        void        MakeMat4(Mat4f* m) const;   ///< Create the corresponding 4x4 transform matrix

        // Data
        Vec3f mTranslation = vl_0;
        float mScale       = 1.0f;
        Mat3f mRotation    = vl_I;          ///< mRotation[i] is direction i of the transform.
    };


    cTransform Transform(const cTransform& a, const cTransform& b);     ///< Returns a(b), i.e., the equivalent of a.Prepend(b)

    bool HasNAN(const cTransform& t);   ///< Returns true if any element of 't' contains a NaN


    // --- Inlines -------------------------------------------------------------

    inline cTransform::cTransform() : mTranslation(vl_zero), mScale(vl_one), mRotation(vl_I)
    {}

    inline cTransform::cTransform(Vec3f t) : mTranslation(t), mScale(vl_one), mRotation(vl_I)
    {}

    inline cTransform::cTransform(float s, Vec3f t) : mTranslation(t), mScale(s), mRotation(vl_I)
    {}

    inline cTransform::cTransform(float s, const Mat3f& r, Vec3f t) : mTranslation(t), mScale(s), mRotation(r)
    {}

    inline void cTransform::SetScale(float s)
    {
        mScale = s;
    }

    inline void cTransform::SetRot(const Mat3f& m)
    {
        mRotation = m;
    }

    inline void cTransform::SetRot(const Vec3f& axis, Real theta)
    {
        mRotation.MakeRot(axis, theta);
    }

    inline void cTransform::SetRot(const Vec4f& q)
    {
        mRotation.MakeRot(q);
    }

    inline void cTransform::SetTrans(Vec3f t)
    {
        mTranslation = t;
    }

    inline float cTransform::Scale() const
    {
        return mScale;
    }

    inline const Mat3f& cTransform::Rot() const
    {
        return mRotation;
    }

    inline const Vec3f& cTransform::Trans() const
    {
        return mTranslation;
    }

    inline void cTransform::MakeIdentity()
    { 
        mTranslation = vl_zero;
        mScale       = vl_one;
        mRotation    = vl_I;
    }

    inline cTransform& cTransform::Prepend(const cTransform& t)
    {
        mTranslation += (t.mTranslation * mScale) * mRotation;
        mRotation    =   t.mRotation * mRotation;
        mScale       *=  t.mScale;

        return *this;
    }

    inline cTransform& cTransform::Append(const cTransform& t)
    {
        mTranslation *=  t.mScale;
        mTranslation *=  t.mRotation;
        mTranslation +=  t.mTranslation;
        mRotation    =   mRotation * t.mRotation;
        mScale       *=  t.mScale;

        return *this;
    }

    inline cTransform& cTransform::PrependScale(float scale)
    {
        mScale *= scale;

        return *this;
    }

    inline cTransform& cTransform::AppendScale(float scale)
    {
        mScale *= scale;
        mTranslation *= scale;

        return *this;
    }

    inline cTransform& cTransform::PrependRot(const Mat3f& rotation)
    {
        mRotation = rotation * mRotation;
        return *this;
    }

    inline cTransform& cTransform::AppendRot(const Mat3f& rotation)
    {
        mRotation *= rotation;
        mTranslation *= rotation;
        return *this;
    }

    inline cTransform& cTransform::PrependTrans(Vec3f t)
    {
        mTranslation += (t * mScale) * mRotation;
        return *this;
    }

    inline cTransform& cTransform::AppendTrans(Vec3f translation)
    {
        mTranslation += translation;
        return *this;
    }

    inline cTransform& cTransform::PrependRotX(float angle)
    {
        float s = sinf(angle); float c = cosf(angle);

        Vec3f r1(mRotation[1]);
        Vec3f r2(mRotation[2]);

        mRotation[1] =  c * r1 + s * r2;
        mRotation[2] = -s * r1 + c * r2;

        return *this;
    }
    
    inline cTransform& cTransform::PrependRotY(float angle)
    {
        float s = sinf(angle); float c = cosf(angle);

        Vec3f r2(mRotation[2]);
        Vec3f r0(mRotation[0]);

        mRotation[2] =  c * r2 + s * r0;
        mRotation[0] = -s * r2 + c * r0;

        return *this;
    }
    
    inline cTransform& cTransform::PrependRotZ(float angle)
    {
        float s = sinf(angle);
        float c = cosf(angle);

        Vec3f r0(mRotation[0]);
        Vec3f r1(mRotation[1]);

        mRotation[0] =  c * r0 + s * r1;
        mRotation[1] = -s * r0 + c * r1;

        return *this;
    }

    // these could be better, but we're hoping people don't use them much
    inline cTransform& cTransform::AppendRotX(float angle)
    {
        return Append(cTransform().PrependRotX(angle));
    }
    inline cTransform& cTransform::AppendRotY(float angle)
    {
        return Append(cTransform().PrependRotY(angle));
    }
    inline cTransform& cTransform::AppendRotZ(float angle)
    {
        return Append(cTransform().PrependRotZ(angle));
    }


    inline Vec3f cTransform::TransformPoint(Vec3f p) const
    {
        return (p * mScale) * mRotation + mTranslation;
    }

    inline Vec3f cTransform::TransformVector(Vec3f v) const
    {
        return (v * mScale) * mRotation;
    }

    inline Vec3f cTransform::TransformDirection(Vec3f d) const
    {
        return d * mRotation;
    }

    inline void cTransform::TransformPoints(int count, Vec3f p[]) const
    {
        TransformPoints(count, p, sizeof(p[0]), p, sizeof(p[0]));
    }
    inline void cTransform::TransformVectors(int count, Vec3f v[]) const
    {
        TransformVectors(count, v, sizeof(v[0]), v, sizeof(v[0]));
    }
    inline void cTransform::TransformDirections(int count, Vec3f d[]) const
    {
        TransformDirections(count, d, sizeof(d[0]), d, sizeof(d[0]));
    }

    inline float cTransform::TransformBounds(float r) const
    {
        return r * mScale + len(mTranslation);
    }

    inline Vec3f cTransform::BackTransformPoint(Vec3f p) const
    {
        p -= mTranslation;
        p /= mScale;
        return mRotation * p;
    }

    inline Vec3f cTransform::BackTransformVector(Vec3f v) const
    {
        v /= mScale;
        return mRotation * v;
    }

    inline Vec3f cTransform::BackTransformDirection(Vec3f d) const
    {
        return mRotation * d;
    }

    inline Vec3f cTransform::Axis(int a) const
    {
        return mRotation[a];
    }
}

#endif
