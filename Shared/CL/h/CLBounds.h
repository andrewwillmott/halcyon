//
//  File:       CLBounds.h
//
//  Function:   Ops on AABBs
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1999-2013
//


#ifndef CL_BOUNDS_H
#define CL_BOUNDS_H

#include <VL234f.h>
#include <float.h>

namespace nCL
{
    struct cBounds2
    {
        Vec2f mMin;
        Vec2f mMax;

        cBounds2();    ///< Empty bounding box
        cBounds2(const cBounds2& b);
        cBounds2(Vec2f min, Vec2f max);
        cBounds2(Vec2f centre, float halfSide);
        explicit cBounds2(Vec2f p);

        cBounds2& operator=(const cBounds2& b);
        
        // Access
        const Vec2f& Min() const { return mMin; }
        const Vec2f& Max() const { return mMax; }
        Vec2f&       Min()       { return mMin; }
        Vec2f&       Max()       { return mMax; }
        
        Vec2f        Centre() const;                ///< Centre of bounds
        Vec2f        Width()  const;                ///< Dimensions of bounds (max - min)
        Vec2f        Corner (int index)  const;     ///< Returns ith corner point (out of 4)
        void         FindCorners(Vec2f c[4]) const; ///< Returns all corners

        // Initialization
        void  MakeEmpty();
        void  MakeInfinite();
        void  MakePoint (Vec2f p);
        void  MakeSquare(Vec2f centre, float halfSide);
        void  MakeRect  (Vec2f centre, Vec2f halfSide);

        // Updating with point(s)
        cBounds2& Add(Vec2f p);
        cBounds2& Add(int count, const Vec2f p[], size_t stride = sizeof(Vec2f));
        cBounds2& Add(const cBounds2& b);

        // Modification
        void  Scale  (float s);                     ///< Scale about centre.
        void  Scale  (Vec2f s);                     ///< Scale about centre.
        void  Offset (Vec2f v);                     ///< Offset entire bounds
        void  Inflate(float s);                     ///< Enlarge by this amount on all sides
        void  Inflate(Vec2f v);                     ///< Enlarge by specified amount on all sides
        void  IntersectWith(const cBounds2& b);     ///< Replace with the intersection with bbox.
        
        // Tests
        bool  IsEmpty() const;                      ///< True if bbox is degenerate, i.e., mMin[i] > mMax[i] for some i
        bool  Contains  (Vec2f p) const;
        bool  Contains  (const cBounds2& b) const;
        bool  Intersects(const cBounds2& b) const;
        bool  Overlaps  (const cBounds2& b) const;

        // Mapping
        Vec2f MapFromLocal(Vec2f p) const;          ///< Map from bbox space (mMin = (0,0,0), mMax = (1,1,1)) to world space
        Vec2f MapToLocal  (Vec2f p) const;          ///< Map to bbox space from world space.
        Vec2f Clamp       (Vec2f p) const;          ///< Return p clamped to the grid bounds.

        cBounds2 MapFromLocal(const cBounds2& b) const;        ///< Map from local bbox space to world space
        cBounds2 MapToLocal  (const cBounds2& b) const;        ///< Map to local bbox space from world space.
    };

    struct cBounds3
    {
        Vec3f mMin;
        Vec3f mMax;

        cBounds3();    ///< Empty bounding box
        cBounds3(const cBounds3& b);
        cBounds3(Vec3f min, Vec3f max);
        cBounds3(Vec3f centre, float halfSide);
        explicit cBounds3(Vec3f p);

        cBounds3& operator=(const cBounds3& b);
        
        // Access
        const Vec3f& Min() const { return mMin; }
        const Vec3f& Max() const { return mMax; }
        Vec3f&       Min()       { return mMin; }
        Vec3f&       Max()       { return mMax; }

        Vec3f        Centre() const;                ///< Centre of bounds
        Vec3f        Width()  const;                ///< Dimensions of bounds (max - min)
        Vec3f        Corner (int index)  const;     ///< Returns ith corner point (out of 8)
        void         FindCorners(Vec3f c[8]) const; ///< Returns all corners

        // Initialization
        void  MakeEmpty();
        void  MakeInfinite();
        void  MakePoint(Vec3f p);
        void  MakeCube (Vec3f centre, float halfSide);
        void  MakeBox  (Vec3f centre, Vec3f halfSide);

        // Updating with point(s)
        cBounds3& Add(Vec3f p);
        cBounds3& Add(int count, const Vec3f p[], size_t stride = sizeof(Vec3f));
        cBounds3& Add(const cBounds3& b);

        // Modification
        void  Scale  (float s);                     ///< Scale about centre.
        void  Scale  (Vec3f s);                     ///< Scale about centre.
        void  Offset (Vec3f v);                     ///< Offset entire bounds
        void  Inflate(float s);                     ///< Enlarge by this amount on all sides
        void  Inflate(Vec3f v);                     ///< Enlarge by specified amount on all sides
        void  IntersectWith(const cBounds3& b);     ///< Replace with the intersection with bbox.
        
        // Tests
        bool  IsEmpty() const;                      ///< True if bbox is degenerate.
        bool  Contains  (Vec3f p) const;
        bool  Contains  (const cBounds3& b) const;
        bool  Intersects(const cBounds3& b) const;
        bool  Overlaps  (const cBounds3& b) const;

        // Mapping
        Vec3f MapFromLocal(Vec3f p) const;          ///< Map from bbox space (mMin = (0,0,0), mMax = (1,1,1)) to world space
        Vec3f MapToLocal  (Vec3f p) const;          ///< Map to bbox space from world space.
        Vec3f Clamp       (Vec3f p) const;          ///< Return p clamped to the grid bounds.

        cBounds3 MapFromLocal(const cBounds3& b) const;  ///< Map from local bbox space to world space
        cBounds3 MapToLocal  (const cBounds3& b) const;  ///< Map to local bbox space from world space.
    };


    // --- Inlines--------------------------------------------------------------

    // cBounds2

    inline cBounds2::cBounds2() : mMin(+FLT_MAX), mMax(-FLT_MAX)
    {
    }

    inline cBounds2::cBounds2(const cBounds2& b) : 
        mMin(b.mMin), 
        mMax(b.mMax)
    {}

    inline cBounds2::cBounds2(Vec2f minPt, Vec2f maxPt) : 
        mMin(minPt), 
        mMax(maxPt) 
    {}

    inline cBounds2::cBounds2(Vec2f centre, float halfSide) :
        mMin(centre),
        mMax(centre)
    {
        Vec2f w(halfSide);
        mMin -= w;
        mMax += w;
    }
    
    inline cBounds2::cBounds2(Vec2f p) :
        mMin(p), 
        mMax(p) 
    {}


    inline cBounds2& cBounds2::operator=(const cBounds2& b)
    {
        mMax = b.mMax;
        mMin = b.mMin;
        
        return *this; 
    }

    inline Vec2f cBounds2::Centre() const
    {
        return (mMin + mMax) * 0.5f;
    }

    inline Vec2f cBounds2::Width() const
    {
        return mMax - mMin;
    }

    inline Vec2f cBounds2::Corner(int index) const
    {
        Vec2f result;
        
        result[0] = (index & 1) ? mMax[0] : mMin[0];
        result[1] = (index & 2) ? mMax[1] : mMin[1];

        return result;
    }

    inline void cBounds2::MakeEmpty()
    {
        mMin = Vec2f(+FLT_MAX);
        mMax = Vec2f(-FLT_MAX);
    }


    inline void cBounds2::MakeInfinite() 
    {
        mMin = Vec2f(-FLT_MAX);
        mMax = Vec2f(+FLT_MAX);
    }


    inline void cBounds2::MakeSquare(Vec2f p, float halfSide)
    {
        mMin = p - Vec2f(halfSide);
        mMax = p + Vec2f(halfSide);
    }

    inline void cBounds2::MakeRect(Vec2f p, Vec2f halfSide)
    {
        mMin = p - halfSide;
        mMax = p + halfSide;
    }

    inline void cBounds2::MakePoint(Vec2f p)
    {
        mMin = p;
        mMax = p;
    }

    inline cBounds2& cBounds2::Add(Vec2f p)
    {
        if      (mMin[0] > p[0])
            mMin[0] = p[0];
        else if (mMax[0] < p[0])
            mMax[0] = p[0];
        
        if      (mMin[1] > p[1])
            mMin[1] = p[1];
        else if (mMax[1] < p[1])
            mMax[1] = p[1];

        return *this;
    }

    inline cBounds2& cBounds2::Add(const cBounds2& b)
    {
        if (mMin[0] > b.mMin[0])
            mMin[0] = b.mMin[0];
        else
        if (mMax[0] < b.mMax[0])
            mMax[0] = b.mMax[0];
        
        if (mMin[1] > b.mMin[1])
            mMin[1] = b.mMin[1];
        else
        if (mMax[1] < b.mMax[1])
            mMax[1] = b.mMax[1];
        
        return *this;
    }

    inline void cBounds2::Scale(float s)
    {
        const Vec2f center = Centre();
        mMin = center + (mMin - center) * s;
        mMax = center + (mMax - center) * s;
    }

    inline void cBounds2::Scale(Vec2f s)
    {
        const Vec2f center = Centre();
        mMin = center + (mMin - center) * s;
        mMax = center + (mMax - center) * s;
    }

    inline void cBounds2::Offset(Vec2f v)
    {
        mMin += v;
        mMax += v;
    }

    inline void cBounds2::Inflate(float s)
    {
        mMin -= Vec2f(s);
        mMax += Vec2f(s);
    }

    inline void cBounds2::Inflate(Vec2f v)
    {
        mMin -= v;
        mMax += v;
    }


    inline void cBounds2::IntersectWith(const cBounds2& b) 
    {
        if (mMin[0] < b.mMin[0])
            mMin[0] = b.mMin[0];
        if (mMin[1] < b.mMin[1])
            mMin[1] = b.mMin[1];

        if (mMax[0] > b.mMax[0])
            mMax[0] = b.mMax[0];
        if (mMax[1] > b.mMax[1])
            mMax[1] = b.mMax[1];

        if (mMin[0] > mMax[0] || mMin[1] > mMax[1])
            MakeEmpty();
    }

    inline bool cBounds2::IsEmpty() const 
    {
        return mMin[0] > mMax[0];
    }

    inline bool cBounds2::Contains(Vec2f p) const
    {
        return p[0] >= mMin[0] && p[0] <= mMax[0]
            && p[1] >= mMin[1] && p[1] <= mMax[1];
    }

    inline bool cBounds2::Contains(const cBounds2& b) const
    {
        return b.mMin[0] >= mMin[0] && b.mMax[0] <= mMax[0]
            && b.mMin[1] >= mMin[1] && b.mMax[1] <= mMax[1];
    }

    inline bool cBounds2::Intersects(const cBounds2& b) const 
    {
        return mMax[0] >= b.mMin[0] && mMin[0] <= b.mMax[0]
            && mMax[1] >= b.mMin[1] && mMin[1] <= b.mMax[1];
    }

    inline bool cBounds2::Overlaps(const cBounds2& b) const
    {
        return b.mMax[0] >= mMin[0] && b.mMin[0] <= mMax[0]
            && b.mMax[1] >= mMin[1] && b.mMin[1] <= mMax[1];
    }

    inline Vec2f cBounds2::MapFromLocal(Vec2f p) const
    {
        return mMin + p * (mMax - mMin);
    }

    inline Vec2f cBounds2::MapToLocal(Vec2f p) const
    {
        return (p - mMin) / (mMax - mMin);
    }

    inline Vec2f cBounds2::Clamp(Vec2f p) const
    {
        if      (p[0] < mMin[0])
                 p[0] = mMin[0];
        else if (p[0] > mMax[0])
                 p[0] = mMax[0];

        if      (p[1] < mMin[1])
                 p[1] = mMin[1];
        else if (p[1] > mMax[1])
                 p[1] = mMax[1];

        return p;
    }

    inline cBounds2 cBounds2::MapFromLocal(const cBounds2& b) const
    {
        Vec2f delta(mMax - mMin);

        return cBounds2
        (
            mMin + b.mMin * delta,
            mMin + b.mMax * delta
        );
    }

    inline cBounds2 cBounds2::MapToLocal(const cBounds2& b) const
    {
        Vec2f invDelta(Vec2f(vl_one) / (mMax - mMin));

        return cBounds2
        (
            (b.mMin - mMin) * invDelta,
            (b.mMax - mMin) * invDelta
        );
    }

    // cBounds3

    inline cBounds3::cBounds3() : mMin(+FLT_MAX), mMax(-FLT_MAX)
    { 
    }

    inline cBounds3::cBounds3(const cBounds3& b) : 
        mMin(b.mMin), 
        mMax(b.mMax)
    {}

    inline cBounds3::cBounds3(Vec3f minPt, Vec3f maxPt) : 
        mMin(minPt), 
        mMax(maxPt) 
    {}

    inline cBounds3::cBounds3(Vec3f centre, float halfSide) :
        mMin(centre),
        mMax(centre)
    {
        Vec3f w(halfSide);
        mMin -= w;
        mMax += w;
    }

    inline cBounds3::cBounds3(Vec3f p) :
        mMin(p), 
        mMax(p) 
    {}


    inline cBounds3& cBounds3::operator=(const cBounds3& b)
    {
        mMax = b.mMax;
        mMin = b.mMin;
        
        return *this; 
    }

    inline Vec3f cBounds3::Centre() const
    {
        return (mMin + mMax) * 0.5f;
    }

    inline Vec3f cBounds3::Width() const
    {
        return mMax - mMin;
    }

    inline Vec3f cBounds3::Corner(int index) const
    {
        Vec3f result;
        
        result[0] = (index & 1) ? mMax[0] : mMin[0];
        result[1] = (index & 2) ? mMax[1] : mMin[1];
        result[2] = (index & 4) ? mMax[2] : mMin[2];
        
        return result;
    }

    inline void cBounds3::MakeEmpty()
    {
        mMin = Vec3f(+FLT_MAX);
        mMax = Vec3f(-FLT_MAX);
    }


    inline void cBounds3::MakeInfinite() 
    {
        mMin = Vec3f(-FLT_MAX);
        mMax = Vec3f(+FLT_MAX);
    }


    inline void cBounds3::MakeCube(Vec3f p, float halfSide)
    {
        mMin = p - Vec3f(halfSide);
        mMax = p + Vec3f(halfSide);
    }

    inline void cBounds3::MakeBox(Vec3f p, Vec3f halfSide)
    {
        mMin = p - halfSide;
        mMax = p + halfSide;
    }

    inline void cBounds3::MakePoint(Vec3f p)
    {
        mMin = p;
        mMax = p;
    }

    inline cBounds3& cBounds3::Add(Vec3f p)
    {
        if (mMin[0] > p[0])
            mMin[0] = p[0];
        else
        if (mMax[0] < p[0])
            mMax[0] = p[0];
        
        if (mMin[1] > p[1])
            mMin[1] = p[1];
        else
        if (mMax[1] < p[1])
            mMax[1] = p[1];
        
        if (mMin[2] > p[2])
            mMin[2] = p[2];
        else
        if (mMax[2] < p[2])
            mMax[2] = p[2];
        
        return *this;
    }

    inline cBounds3& cBounds3::Add(const cBounds3& b)
    {
        if (mMin[0] > b.mMin[0])
            mMin[0] = b.mMin[0];
        else
        if (mMax[0] < b.mMax[0])
            mMax[0] = b.mMax[0];
        
        if (mMin[1] > b.mMin[1])
            mMin[1] = b.mMin[1];
        else
        if (mMax[1] < b.mMax[1])
            mMax[1] = b.mMax[1];
        
        if (mMin[2] > b.mMin[2])
            mMin[2] = b.mMin[2];
        else
        if (mMax[2] < b.mMax[2])
            mMax[2] = b.mMax[2];
        
        return *this;
    }

    inline void cBounds3::Scale(float s)
    {
        const Vec3f center = Centre();
        mMin = center + (mMin - center) * s;
        mMax = center + (mMax - center) * s;
    }

    inline void cBounds3::Scale(Vec3f s)
    {
        const Vec3f center = Centre();
        mMin = center + (mMin - center) * s;
        mMax = center + (mMax - center) * s;
    }


    inline void cBounds3::Offset(Vec3f v)
    {
        mMin += v;
        mMax += v;
    }

    inline void cBounds3::Inflate(float s)
    {
        mMin -= Vec3f(s);
        mMax += Vec3f(s);
    }

    inline void cBounds3::Inflate(Vec3f v)
    {
        mMin -= v;
        mMax += v;
    }


    inline void cBounds3::IntersectWith(const cBounds3& b)
    {
        if (mMin[0] < b.mMin[0])
            mMin[0] = b.mMin[0];
        if (mMin[1] < b.mMin[1])
            mMin[1] = b.mMin[1];
        if (mMin[2] < b.mMin[2])
            mMin[2] = b.mMin[2];

        if (mMax[0] > b.mMax[0])
            mMax[0] = b.mMax[0];
        if (mMax[1] > b.mMax[1])
            mMax[1] = b.mMax[1];
        if (mMax[2] > b.mMax[2])
            mMax[2] = b.mMax[2];
        
        if (mMin[0] > mMax[0] || mMin[1] > mMax[1] || mMin[2] > mMax[2])
            MakeEmpty();
    }

    inline bool cBounds3::IsEmpty() const 
    {
        return mMin[0] > mMax[0];
    }

    inline bool cBounds3::Contains(Vec3f p) const
    {
        return p[0] >= mMin[0] && p[0] <= mMax[0]
            && p[1] >= mMin[1] && p[1] <= mMax[1]
            && p[2] >= mMin[2] && p[2] <= mMax[2];
    }

    inline bool cBounds3::Contains(const cBounds3& b) const
    {
        return b.mMin[0] >= mMin[0] && b.mMax[0] <= mMax[0]
            && b.mMin[1] >= mMin[1] && b.mMax[1] <= mMax[1]
            && b.mMin[2] >= mMin[2] && b.mMax[2] <= mMax[2];
    }

    inline bool cBounds3::Intersects(const cBounds3& b) const
    {
        return mMax[0] >= b.mMin[0] && mMin[0] <= b.mMax[0]
            && mMax[1] >= b.mMin[1] && mMin[1] <= b.mMax[1]
            && mMax[2] >= b.mMin[2] && mMin[2] <= b.mMax[2];
    }

    inline bool cBounds3::Overlaps(const cBounds3& b) const
    {
        return b.mMax[0] >= mMin[0] && b.mMin[0] <= mMax[0]
            && b.mMax[1] >= mMin[1] && b.mMin[1] <= mMax[1]
            && b.mMax[2] >= mMin[2] && b.mMin[2] <= mMax[2];
    }

    inline Vec3f cBounds3::MapFromLocal(Vec3f p) const
    {
        return mMin + p * (mMax - mMin);
    }

    inline Vec3f cBounds3::MapToLocal(Vec3f p) const
    {
        return (p - mMin) / (mMax - mMin);
    }

    inline Vec3f cBounds3::Clamp(Vec3f p) const
    {
        if      (p[0] < mMin[0])
                 p[0] = mMin[0];
        else if (p[0] > mMax[0])
                 p[0] = mMax[0];

        if      (p[1] < mMin[1])
                 p[1] = mMin[1];
        else if (p[1] > mMax[1])
                 p[1] = mMax[1];

        if      (p[2] < mMin[2])
                 p[2] = mMin[2];
        else if (p[2] > mMax[2])
                 p[2] = mMax[2];

        return p;
    }

    inline cBounds3 cBounds3::MapFromLocal(const cBounds3& b) const
    {
        Vec3f delta(mMax - mMin);

        return cBounds3
        (
            mMin + b.mMin * delta,
            mMin + b.mMax * delta
        );
    }

    inline cBounds3 cBounds3::MapToLocal(const cBounds3& b) const
    {
        Vec3f invDelta(Vec3f(vl_one) / (mMax - mMin));

        return cBounds3
        (
            (b.mMin - mMin) * invDelta,
            (b.mMax - mMin) * invDelta
        );
    }
}

#endif
