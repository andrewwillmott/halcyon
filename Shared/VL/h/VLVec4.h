/*
    File:           VLVec4.h

    Function:       Defines a length-4 vector.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Vec4_H
#define VL_Vec4_H

#include "VL.h"


// --- Vec4 Class -------------------------------------------------------------


class TVec2;
class TVec3;

class TVec4
{
public:
    // Constructors
    TVec4();
    TVec4(TVReal x, TVReal y, TVReal z, TVReal w);  // [x, y, z, w]
    TVec4(const TVec4& v);                      // Copy constructor
    TVec4(const TVec3& v, TVReal w);            // Hom. 3D vector
    TVec4(ZeroOrOne k);
    TVec4(Axis k);
    explicit TVec4(TVReal s);
    explicit TVec4(const TVReal v[]);
    
    // Accessor functions
    TVReal&       operator [] (int i);
    const TVReal& operator [] (int i) const;
    
    int           Elts() const { return 4; };
    TVReal*       Ref() const;                   // Return pointer to data

    // Assignment operators
    TVec4&        operator =  (const TVec4& a);  
    TVec4&        operator =  (ZeroOrOne k);
    TVec4&        operator =  (Axis k);
    TVec4&        operator += (const TVec4& a);
    TVec4&        operator -= (const TVec4& a);
    TVec4&        operator *= (const TVec4& a);
    TVec4&        operator *= (TVReal s);
    TVec4&        operator /= (const TVec4& a);
    TVec4&        operator /= (TVReal s);
    
    // Comparison operators
    bool          operator == (const TVec4& a) const; // v == a ?
    bool          operator != (const TVec4& a) const; // v != a ?

    // Arithmetic operators
    TVec4         operator + (const TVec4& a) const;  // v + a
    TVec4         operator - (const TVec4& a) const;  // v - a
    TVec4         operator - () const;                // -v
    TVec4         operator * (const TVec4& a) const;  // v * a (vx * ax, ...)
    TVec4         operator * (TVReal s) const;        // v * s
    TVec4         operator / (const TVec4& a) const;  // v / a (vx / ax, ...)
    TVec4         operator / (TVReal s) const;        // v / s

    // Initialisers
    TVec4&        MakeZero();                        // Zero vector
    TVec4&        MakeUnit(int i, TVReal k = vl_one);// kI[i]
    TVec4&        MakeBlock(TVReal k = vl_one);      // All-k vector

    TVec4&        Normalize();                       // normalize vector

    // Conversion
    TVec2&          AsVec2();
    const TVec2&    AsVec2() const;
    TVec3&          AsVec3();
    const TVec3&    AsVec3() const;

protected:
    // Data
    TVReal elt[4]; 
};


// --- Vec operators ----------------------------------------------------------

TVec4   operator * (TVReal s, const TVec4& v);  // Left mult. by s
TVReal  dot  (const TVec4& a, const TVec4& b);  // v . a
TVec4   cross(const TVec4& a, const TVec4& b, const TVec4& c); // 4D cross prod.

TVReal  len      (const TVec4& v);              // || v ||
TVReal  sqrlen   (const TVec4& v);              // v . v
TVec4   norm     (const TVec4& v);              // v / || v ||
TVec4   norm_safe(const TVec4& v);              // v / || v ||, handles || v || = 0
TVec4   inv      (const TVec4& v);              // 1 / v
TVec3   proj     (const TVec4& v);              // hom. projection

void    normalize(TVec4& v);                    // v = norm(v)


// --- Inlines ----------------------------------------------------------------

#include "VLVec3.h"

inline TVReal& TVec4::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, 4, "(Vec4::[i]) index out of range");
    return elt[i];
}

inline const TVReal& TVec4::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, 4, "(Vec4::[i]) index out of range");
    return elt[i];
}


inline TVec4::TVec4()
{
}

inline TVec4::TVec4(TVReal x, TVReal y, TVReal z, TVReal w)
{
    elt[0] = x;
    elt[1] = y;
    elt[2] = z;
    elt[3] = w;
}

inline TVec4::TVec4(const TVec4& v) 
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = v[3];
}

inline TVec4::TVec4(const TVec3& v, TVReal w)   
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = w;
}

inline TVec4::TVec4(TVReal s)
{
    elt[0] = s;
    elt[1] = s;
    elt[2] = s;
    elt[3] = s;
}

inline TVec4::TVec4(const TVReal v[])
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = v[3];
}

inline TVReal* TVec4::Ref() const
{
    return (TVReal*) elt;
}

inline TVec4& TVec4::operator = (const TVec4& v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = v[3];
    
    return SELF;
}

inline TVec4& TVec4::operator += (const TVec4& v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    elt[2] += v[2];
    elt[3] += v[3];
    
    return SELF;
}

inline TVec4& TVec4::operator -= (const TVec4& v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    elt[2] -= v[2];
    elt[3] -= v[3];
    
    return SELF;
}

inline TVec4& TVec4::operator *= (const TVec4& v)
{
    elt[0] *= v[0];
    elt[1] *= v[1];
    elt[2] *= v[2];
    elt[3] *= v[3];
    
    return SELF;
}

inline TVec4& TVec4::operator *= (TVReal s)
{
    elt[0] *= s;
    elt[1] *= s;
    elt[2] *= s;
    elt[3] *= s;
    
    return SELF;
}

inline TVec4& TVec4::operator /= (const TVec4& v)
{
    elt[0] /= v[0];
    elt[1] /= v[1];
    elt[2] /= v[2];
    elt[3] /= v[3];
    
    return SELF;
}

inline TVec4& TVec4::operator /= (TVReal s)
{
    elt[0] /= s;
    elt[1] /= s;
    elt[2] /= s;
    elt[3] /= s;
    
    return SELF;
}


inline TVec4 TVec4::operator + (const TVec4& a) const
{
    TVec4 result;
    
    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];
    result[2] = elt[2] + a[2];
    result[3] = elt[3] + a[3];
    
    return result;
}

inline TVec4 TVec4::operator - (const TVec4& a) const
{
    TVec4 result;
    
    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];
    result[2] = elt[2] - a[2];
    result[3] = elt[3] - a[3];
    
    return result;
}

inline TVec4 TVec4::operator - () const
{
    TVec4 result;
    
    result[0] = -elt[0];
    result[1] = -elt[1];
    result[2] = -elt[2];
    result[3] = -elt[3];
    
    return result;
}

inline TVec4 TVec4::operator * (const TVec4& a) const           
{
    TVec4 result;

    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];
    result[2] = elt[2] * a[2];
    result[3] = elt[3] * a[3];
    
    return result;
}

inline TVec4 TVec4::operator * (TVReal s) const
{
    TVec4 result;
    
    result[0] = elt[0] * s;
    result[1] = elt[1] * s;
    result[2] = elt[2] * s;
    result[3] = elt[3] * s;
    
    return result;
}

inline TVec4 TVec4::operator / (const TVec4& a) const
{
    TVec4 result;
    
    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];
    result[2] = elt[2] / a[2];
    result[3] = elt[3] / a[3];
    
    return result;
}

inline TVec4 TVec4::operator / (TVReal s) const
{
    TVec4 result;
    
    result[0] = elt[0] / s;
    result[1] = elt[1] / s;
    result[2] = elt[2] / s;
    result[3] = elt[3] / s;
    
    return result;
}

inline TVec4 operator * (TVReal s, const TVec4& v)
{
    return v * s;
}

inline TVec4& TVec4::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = vl_zero;
    return SELF;
}

inline TVec4& TVec4::MakeBlock(TVReal k)
{
    elt[0] = k; elt[1] = k; elt[2] = k; elt[3] = k;
    return SELF;
}

inline TVec4& TVec4::Normalize()
{
    CL_ASSERT_MSG(sqrlen(*this) > 0.0, "normalising length-zero vector");
    *this /= len(*this);
    return SELF;
}

inline TVec4::TVec4(ZeroOrOne k)    
{
    MakeBlock(TVReal(k));
}

inline TVec4::TVec4(Axis k)     
{
    MakeUnit(k, vl_1);
}

inline TVec4& TVec4::operator = (ZeroOrOne k)
{
    MakeBlock(TVReal(k));
    
    return SELF;
}

inline TVec4& TVec4::operator = (Axis k)
{
    MakeUnit(k, vl_1);
    
    return SELF;
}

inline TVec2& TVec4::AsVec2()
{
    return (TVec2&) SELF;
}

inline const TVec2& TVec4::AsVec2() const
{
    return (const TVec2&) SELF;
}

inline TVec3& TVec4::AsVec3()
{
    return (TVec3&) SELF;
}

inline const TVec3& TVec4::AsVec3() const
{
    return (const TVec3&) SELF;
}



inline TVReal dot(const TVec4& a, const TVec4& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

inline TVReal len(const TVec4& v)
{
    return sqrt(dot(v, v));
}

inline TVReal sqrlen(const TVec4& v)
{
    return dot(v, v);
}

inline TVec4 norm(const TVec4& v)   
{
    CL_ASSERT_MSG(sqrlen(v) > 0.0, "normalising length-zero vector");
    return v / len(v);
}

inline TVec4 norm_safe(const TVec4& v)   
{
    return v / (len(v) + TVReal(1e-8));
}

inline TVec4 inv(const TVec4& v)
{
    return TVec4(TVReal(1) / v[0], TVReal(1) / v[1], TVReal(1) / v[2], TVReal(1) / v[3]);
}


#endif
