/*
    File:           VLVec3.h

    Function:       Defines a length-3 vector.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Vec3_H
#define VL_Vec3_H

#include "VL.h"
 
 
// --- Vec3 Class -------------------------------------------------------------


class TVec2;

class TVec3
{
public:
    // Constructors
    TVec3();
    TVec3(TVReal x, TVReal y, TVReal z);// [x, y, z]
    TVec3(const TVec3& v);              // Copy constructor
    TVec3(const TVec2& v, TVReal w);    // Hom. 2D vector
    TVec3(ZeroOrOne k);
    TVec3(Axis a);
    explicit TVec3(TVReal s);
    explicit TVec3(const TVReal v[]);
    
    // Accessor functions
    TVReal&       operator [] (int i);
    const TVReal& operator [] (int i) const;

    int          Elts() const { return(3); };
    TVReal*      Ref() const;          // Return pointer to data

    // Assignment operators
    TVec3&    operator =  (const TVec3& a);
    TVec3&    operator =  (ZeroOrOne k);
    TVec3&    operator += (const TVec3& a);
    TVec3&    operator -= (const TVec3& a);
    TVec3&    operator *= (const TVec3& a);
    TVec3&    operator *= (TVReal s);
    TVec3&    operator /= (const TVec3& a);
    TVec3&    operator /= (TVReal s);
    
    // Comparison operators
    bool      operator == (const TVec3& a) const; // v == a?
    bool      operator != (const TVec3& a) const; // v != a?
    bool      operator <  (const TVec3& a) const; // v <  a?
    bool      operator >= (const TVec3& a) const; // v >= a?

    // Arithmetic operators
    TVec3     operator + (const TVec3& a) const;  // v + a
    TVec3     operator - (const TVec3& a) const;  // v - a
    TVec3     operator - () const;                // -v
    TVec3     operator * (const TVec3& a) const;  // v * a (vx * ax, ...)
    TVec3     operator * (TVReal s) const;        // v * s
    TVec3     operator / (const TVec3& a) const;  // v / a (vx / ax, ...)
    TVec3     operator / (TVReal s) const;        // v / s    

    // Initialisers
    TVec3&    MakeZero();                        // Zero vector
    TVec3&    MakeUnit(int i, TVReal k = vl_one);// I[i]
    TVec3&    MakeBlock(TVReal k = vl_one);      // All-k vector

    TVec3&    Normalize();                       // normalize vector

    // Conversion
    TVec2&          AsVec2();
    const TVec2&    AsVec2() const;

protected:
    // Data
    TVReal elt[3]; 
};


// --- Vec operators ----------------------------------------------------------

TVec3    operator * (TVReal s, const TVec3& v); // s * v
TVReal   dot(const TVec3& a, const TVec3& b);   // v . a

TVReal   len      (const TVec3& v);             // || v ||
TVReal   sqrlen   (const TVec3& v);             // v . v
TVec3    norm     (const TVec3& v);             // v / || v ||
TVec3    norm_safe(const TVec3& v);             // v / || v ||, handles || v || = 0
TVec3    cross    (const TVec3& a, const TVec3& b); // a x b
TVec3    cross_x  (const TVec3& v);             // v x e_x
TVec3    cross_y  (const TVec3& v);             // v x e_y
TVec3    cross_z  (const TVec3& v);             // v x e_z
TVec3    inv      (const TVec3& v);             // inverse: 1 / v
TVec2    proj     (const TVec3& v);             // homogeneous projection


// --- Inlines ----------------------------------------------------------------

#include "VLVec2.h"

inline TVReal& TVec3::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, 3, "(Vec3::[i]) index out of range");
    return(elt[i]);
}

inline const TVReal& TVec3::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, 3, "(Vec3::[i]) index out of range");
    return(elt[i]);
}

inline TVec3::TVec3()
{
}

inline TVec3::TVec3(TVReal x, TVReal y, TVReal z)
{
    elt[0] = x;
    elt[1] = y;
    elt[2] = z;
}

inline TVec3::TVec3(const TVec3& v) 
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
}

inline TVec3::TVec3(const TVec2& v, TVReal w)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = w;
}

inline TVec3::TVec3(TVReal s)
{
    elt[0] = s;
    elt[1] = s;
    elt[2] = s;
}

inline TVec3::TVec3(const TVReal v[])
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
}

inline TVReal* TVec3::Ref() const
{
    return((TVReal* ) elt);
}

inline TVec3& TVec3::operator = (const TVec3& v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    
    return(*this);
}

inline TVec3& TVec3::operator += (const TVec3& v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    elt[2] += v[2];
    
    return(*this);
}

inline TVec3& TVec3::operator -= (const TVec3& v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    elt[2] -= v[2];
    
    return(*this);
}

inline TVec3& TVec3::operator *= (const TVec3& a)
{
    elt[0] *= a[0];
    elt[1] *= a[1];
    elt[2] *= a[2];
    
    return(*this);
}

inline TVec3& TVec3::operator *= (TVReal s)
{
    elt[0] *= s;
    elt[1] *= s;
    elt[2] *= s;
    
    return(*this);
}

inline TVec3& TVec3::operator /= (const TVec3& a)
{
    elt[0] /= a[0];
    elt[1] /= a[1];
    elt[2] /= a[2];
    
    return(*this);
}

inline TVec3& TVec3::operator /= (TVReal s)
{
    elt[0] /= s;
    elt[1] /= s;
    elt[2] /= s;
    
    return(*this);
}

inline TVec3 TVec3::operator + (const TVec3& a) const
{
    TVec3 result;
    
    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];
    result[2] = elt[2] + a[2];
    
    return(result);
}

inline TVec3 TVec3::operator - (const TVec3& a) const
{
    TVec3 result;
    
    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];
    result[2] = elt[2] - a[2];
    
    return(result);
}

inline TVec3 TVec3::operator - () const
{
    TVec3 result;
    
    result[0] = -elt[0];
    result[1] = -elt[1];
    result[2] = -elt[2];
    
    return(result);
}

inline TVec3 TVec3::operator * (const TVec3& a) const
{
    TVec3 result;
    
    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];
    result[2] = elt[2] * a[2];
    
    return(result);
}

inline TVec3 TVec3::operator * (TVReal s) const
{
    TVec3 result;
    
    result[0] = elt[0] * s;
    result[1] = elt[1] * s;
    result[2] = elt[2] * s;
    
    return(result);
}

inline TVec3 TVec3::operator / (const TVec3& a) const
{
    TVec3 result;
    
    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];
    result[2] = elt[2] / a[2];
    
    return(result);
}

inline TVec3 TVec3::operator / (TVReal s) const
{
    TVec3 result;
    
    result[0] = elt[0] / s;
    result[1] = elt[1] / s;
    result[2] = elt[2] / s;
    
    return(result);
}

inline TVec3 operator * (TVReal s, const TVec3& v)
{
    return(v * s);
}

inline TVec3& TVec3::MakeUnit(int n, TVReal k)
{
    switch (n)
    {
    case 0:
        { elt[0] = k; elt[1] = vl_zero; elt[2] = vl_zero; } break;
    case 1:
        { elt[0] = vl_zero; elt[1] = k; elt[2] = vl_zero; } break;
    case 2:
        { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = k; } break;
    default:
        CL_ERROR("(Vec3::Unit) illegal unit vector");
    }
    return(*this);
}

inline TVec3& TVec3::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero;
    return(*this);
}

inline TVec3& TVec3::MakeBlock(TVReal k)
{
    elt[0] = k; elt[1] = k; elt[2] = k;
    return(*this);
}

inline TVec3& TVec3::Normalize()
{
    CL_ASSERT_MSG(sqrlen(*this) > 0.0, "normalising length-zero vector");
    *this /= len(*this);
    return(*this);
}


inline TVec3::TVec3(ZeroOrOne k) 
{
    elt[0] = TVReal(k); elt[1] = TVReal(k); elt[2] = TVReal(k);
}

inline TVec3& TVec3::operator = (ZeroOrOne k)
{
    elt[0] = TVReal(k); elt[1] = TVReal(k); elt[2] = TVReal(k);
    
    return(*this);
}

inline TVec3::TVec3(Axis a)
{
    MakeUnit(a, vl_one);
}


inline bool TVec3::operator == (const TVec3& a) const
{
    return(elt[0] == a[0] && elt[1] == a[1] && elt[2] == a[2]);
}

inline bool TVec3::operator != (const TVec3& a) const
{
    return(elt[0] != a[0] || elt[1] != a[1] || elt[2] != a[2]);
}

inline bool TVec3::operator < (const TVec3& a) const
{
    return(elt[0] < a[0] && elt[1] < a[1] && elt[2] < a[2]);
}

inline bool TVec3::operator >= (const TVec3& a) const
{
    return(elt[0] >= a[0] && elt[1] >= a[1] && elt[2] >= a[2]);
}

inline TVec2& TVec3::AsVec2()
{
    return (TVec2&) SELF;
}

inline const TVec2& TVec3::AsVec2() const
{
    return (const TVec2&) SELF;
}


inline TVReal dot(const TVec3& a, const TVec3& b)
{
    return(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

inline TVReal len(const TVec3& v)
{
    return(sqrt(dot(v, v)));
}

inline TVReal sqrlen(const TVec3& v)
{
    return(dot(v, v));
}

inline TVec3 norm(const TVec3& v)   
{
    CL_ASSERT_MSG(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline TVec3 norm_safe(const TVec3& v)
{
    return(v / (len(v) + TVReal(1e-8)));
}

inline TVec3 cross(const TVec3& a, const TVec3& b)
{
    TVec3 result;

    result[0] = a[1] * b[2] - a[2] * b[1];  
    result[1] = a[2] * b[0] - a[0] * b[2];  
    result[2] = a[0] * b[1] - a[1] * b[0];  

    return(result);
}

inline TVec3 cross_x(const TVec3& v)
{ return TVec3(0, v[2], -v[1]); }

inline TVec3 cross_y(const TVec3& v)
{ return TVec3(-v[2], 0, v[0]); }

inline TVec3 cross_z(const TVec3& v)
{ return TVec3(v[1], -v[0], 0); }

inline TVec3 inv(const TVec3& v)
{
    return TVec3(TVReal(1) / v[0], TVReal(1) / v[1], TVReal(1) / v[2]);
}

inline TVec2 proj(const TVec3& v) 
{
    TVec2 result;
    
    CL_ASSERT_MSG(v[2] != 0, "(Vec3/proj) last elt. is zero");
    
    result[0] = v[0] / v[2];
    result[1] = v[1] / v[2];
    
    return(result);
}

#endif
