/*
    File:           VLVec2.h

    Function:       Defines a length-2 vector.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Vec2_H
#define VL_Vec2_H

#include "VL.h"


// --- Vec2 Class -------------------------------------------------------------


class TVec2
{
public:

    // Constructors
    TVec2();
    TVec2(TVReal x, TVReal y);      // (x, y)
    TVec2(const TVec2& v);          // Copy constructor
    TVec2(ZeroOrOne k);             // v[i] = vl_zero
    TVec2(Axis k);                  // v[k] = 1
    explicit TVec2(TVReal s);
    explicit TVec2(const TVReal v[]);

    // Accessor functions
    TVReal&       operator [] (int i);
    const TVReal& operator [] (int i) const;
    
    int           Elts() const { return(2); };
    TVReal*       Ref() const;                  // Return pointer to data

    // Assignment operators
    TVec2&    operator =  (const TVec2& a);                      
    TVec2&    operator =  (ZeroOrOne k);                     
    TVec2&    operator =  (Axis k);                      
    
    TVec2&    operator += (const TVec2& a);
    TVec2&    operator -= (const TVec2& a);
    TVec2&    operator *= (const TVec2& a);
    TVec2&    operator *= (TVReal s);
    TVec2&    operator /= (const TVec2& a);
    TVec2&    operator /= (TVReal s);

    // Comparison operators
    bool     operator == (const TVec2& a) const; // v == a?
    bool     operator != (const TVec2& a) const; // v != a?

    // Arithmetic operators
    TVec2    operator + (const TVec2& a) const;  // v + a
    TVec2    operator - (const TVec2& a) const;  // v - a
    TVec2    operator - () const;                // -v
    TVec2    operator * (const TVec2& a) const;  // v * a (vx * ax, ...)
    TVec2    operator * (TVReal s) const;        // v * s
    TVec2    operator / (const TVec2& a) const;  // v / a (vx / ax, ...)
    TVec2    operator / (TVReal s) const;        // v / s

    // Initialisers
    TVec2&    MakeZero();                        // Zero vector
    TVec2&    MakeUnit(int i, TVReal k = vl_one);// I[i]
    TVec2&    MakeBlock(TVReal k = vl_one);      // All-k vector
    
    TVec2&    Normalize();                       // normalize vector

protected:
    // Data
    TVReal  elt[2];
};


// --- Vec operators ----------------------------------------------------------

TVec2    operator * (TVReal s, const TVec2& v);// s * v

TVReal   dot      (const TVec2& a, const TVec2& b); // v . a
TVReal   len      (const TVec2& v);                 // || v ||
TVReal   sqrlen   (const TVec2& v);                 // v . v
TVec2    norm     (const TVec2& v);                 // v / || v ||
TVec2    norm_safe(const TVec2& v);                 // v / || v ||, handles || v || = 0
TVec2    cross    (const TVec2& v);                 // cross prod.
TVec2    inv      (const TVec2& v);                 // inverse: 1 / v

    
// --- Inlines ----------------------------------------------------------------


inline TVReal& TVec2::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, 2, "(Vec2::[i]) index out of range");
    return(elt[i]);
}

inline const TVReal& TVec2::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, 2, "(Vec2::[i]) index out of range");
    return(elt[i]);
}

inline TVec2::TVec2()
{
}

inline TVec2::TVec2(TVReal x, TVReal y)
{
    elt[0] = x;
    elt[1] = y;
}

inline TVec2::TVec2(const TVec2& v) 
{
    elt[0] = v[0];
    elt[1] = v[1];
}

inline TVec2::TVec2(TVReal s)
{
    elt[0] = s;
    elt[1] = s;
}

inline TVec2::TVec2(const TVReal v[])
{
    elt[0] = v[0];
    elt[1] = v[1];
}

inline TVReal* TVec2::Ref() const
{
    return((TVReal *) elt);
}

inline TVec2& TVec2::operator = (const TVec2& v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    
    return(*this);
}

inline TVec2& TVec2::operator += (const TVec2& v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    
    return(*this);
}

inline TVec2& TVec2::operator -= (const TVec2& v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    
    return(*this);
}

inline TVec2& TVec2::operator *= (const TVec2& v)
{
    elt[0] *= v[0];
    elt[1] *= v[1];
    
    return(*this);
}

inline TVec2& TVec2::operator *= (TVReal s)
{
    elt[0] *= s;
    elt[1] *= s;
    
    return(*this);
}

inline TVec2& TVec2::operator /= (const TVec2& v)
{
    elt[0] /= v[0];
    elt[1] /= v[1];
    
    return(*this);
}

inline TVec2& TVec2::operator /= (TVReal s)
{
    elt[0] /= s;
    elt[1] /= s;
    
    return(*this);
}

inline TVec2 TVec2::operator + (const TVec2& a) const
{
    TVec2 result;
    
    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];
    
    return(result);
}

inline TVec2 TVec2::operator - (const TVec2& a) const
{
    TVec2 result;
    
    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];
    
    return(result);
}

inline TVec2 TVec2::operator - () const
{
    TVec2 result;
    
    result[0] = -elt[0];
    result[1] = -elt[1];
    
    return(result);
}

inline TVec2 TVec2::operator * (const TVec2& a) const           
{
    TVec2 result;
    
    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];
    
    return(result);
}

inline TVec2 TVec2::operator * (TVReal s) const
{
    TVec2 result;
    
    result[0] = elt[0] * s;
    result[1] = elt[1] * s;
    
    return(result);
}

inline TVec2 operator * (TVReal s, const TVec2& v)
{
    return(v * s);
}

inline TVec2 TVec2::operator / (const TVec2& a) const
{
    TVec2 result;
    
    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];
    
    return(result);
}

inline TVec2 TVec2::operator / (TVReal s) const
{
    TVec2 result;
    
    result[0] = elt[0] / s;
    result[1] = elt[1] / s;
    
    return(result);
}

inline TVReal dot(const TVec2& a, const TVec2& b)
{
    return(a[0] * b[0] + a[1] * b[1]);
}

inline TVec2 cross(const TVec2& a)
{
    TVec2 result;
    
    result[0] = -a[1];
    result[1] =  a[0];
    
    return(result);
}

inline TVReal len(const TVec2& v)
{
    return(sqrt(dot(v, v)));
}

inline TVReal sqrlen(const TVec2& v)
{
    return(dot(v, v));
}

inline TVec2 norm(const TVec2& v)   
{
    CL_ASSERT_MSG(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline TVec2 norm_safe(const TVec2& v)   
{
    return(v / (len(v) + TVReal(1e-8)));
}

inline TVec2 inv(const TVec2& v)
{
    return TVec2(TVReal(1) / v[0], TVReal(1) / v[1]);
}

inline TVec2& TVec2::MakeUnit(int i, TVReal k)
{
    if (i == 0)
    { elt[0] = k; elt[1] = vl_zero; }
    else if (i == 1)
    { elt[0] = vl_zero; elt[1] = k; }
    else 
        CL_ERROR("(Vec2::Unit) illegal unit vector");
    return(*this);
}

inline TVec2& TVec2::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero;
    return(*this);
}

inline TVec2& TVec2::MakeBlock(TVReal k)
{
    elt[0] = k; elt[1] = k;
    return(*this);
}

inline TVec2& TVec2::Normalize()
{
    CL_ASSERT_MSG(sqrlen(*this) > 0.0, "normalising length-zero vector");
    *this /= len(*this);
    return(*this);
}


inline TVec2::TVec2(ZeroOrOne k) 
{
    elt[0] = TVReal(k);
    elt[1] = TVReal(k);
}

inline TVec2::TVec2(Axis k) 
{
    MakeUnit(k, vl_one);
}

inline TVec2& TVec2::operator = (ZeroOrOne k)
{
    elt[0] = TVReal(k); elt[1] = TVReal(k);
    
    return(*this);
}

inline TVec2& TVec2::operator = (Axis k)
{
    MakeUnit(k, vl_1);
    
    return(*this);
}

inline bool TVec2::operator == (const TVec2& a) const
{
    return(elt[0] == a[0] && elt[1] == a[1]);
}

inline bool TVec2::operator != (const TVec2& a) const
{
    return(elt[0] != a[0] || elt[1] != a[1]);
}

#endif
