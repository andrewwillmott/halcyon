/*
    File:           VLMat4.h

    Function:       Defines a 4 x 4 matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Mat4_H
#define VL_Mat4_H

#include "VL.h"
#include "VLVec4.h"


// --- Mat4 Class -------------------------------------------------------------

class TVec3;

class TMat4 
{
public:
    
    // Constructors
    
    TMat4();
    TMat4(TMReal a, TMReal b, TMReal c, TMReal d,
          TMReal e, TMReal f, TMReal g, TMReal h,
          TMReal i, TMReal j, TMReal k, TMReal l,
          TMReal m, TMReal n, TMReal o, TMReal p);
    TMat4(const TMVec4& v0, const TMVec4& v1, const TMVec4& v2, const TMVec4& v3);
    TMat4(const TMat4& m);
    explicit TMat4(const TMat3& m, TMReal scale = TMReal(vl_1));
    TMat4(ZeroOrOne k);
    TMat4(Block k);
                    
    // Accessor functions   
    int           Elts() const { return 16; };
    int           Rows() const { return  4; };
    int           Cols() const { return  4; };
    
    TMVec4&       operator [] (int i);
    const TMVec4& operator [] (int i) const;

    TMReal&       operator () (int i, int j);
    TMReal        operator () (int i, int j) const;
    
    TMReal*       Ref() const;           // Return pointer to data
    const TMReal* ConstRef() const;      // Return pointer to data

    // Assignment operators 
    TMat4&       operator =  (const TMat4& m);
    TMat4&       operator =  (ZeroOrOne k);
    TMat4&       operator =  (Block k);
    TMat4&       operator += (const TMat4& m);
    TMat4&       operator -= (const TMat4& m);
    TMat4&       operator *= (const TMat4& m);
    TMat4&       operator *= (TMReal s);
    TMat4&       operator /= (TMReal s);

    // Comparison operators 
    bool         operator == (const TMat4& m) const; // M == N?
    bool         operator != (const TMat4& m) const; // M != N?
    
    // Arithmetic operators 
    TMat4        operator + (const TMat4& m) const;  // M + N
    TMat4        operator - (const TMat4& m) const;  // M - N
    TMat4        operator - () const;                // -M
    TMat4        operator * (const TMat4& m) const;  // M * N
    TMat4        operator * (TMReal s) const;        // M * s
    TMat4        operator / (TMReal s) const;        // M / s

    // Initialisers 
    void        MakeZero();                         // Zero matrix
    void        MakeIdentity();                     // Identity matrix
    void        MakeDiag(TMReal k = vl_one);        // I
    void        MakeBlock(TMReal k = vl_one);       // all elts = k

    // Homogeneous Transforms   
    TMat4&      MakeHRot(const TMVec3& axis, Real theta); // Rotate by theta radians about axis   
    TMat4&      MakeHRot(const TQuaternion& q); // Rotate by quaternion   
    TMat4&      MakeHScale(const TMVec3& s);    // Scale by components of s

    TMat4&      MakeHTrans(const TMVec3& t);    // Translation by t

    TMat4&      Transpose();                    // transpose in place
    TMat4&      AddShift(const TMVec3& t);      // concat shift
    
protected:
    // Data
    TMVec4  row[4]; 
};


// --- Matrix operators -------------------------------------------------------

TMVec4   operator * (const TMat4& m, const TMVec4& v);   // m * v
TMVec4   operator * (const TMVec4& v, const TMat4& m);   // v * m
TMVec4&   operator *= (TMVec4& a, const TMat4& m);       // v *= m
TMat4    operator * (TMReal s, const TMat4& m);          // s * m

TMat4    trans(const TMat4& m);              // Transpose            
TMReal   trace(const TMat4& m);              // Trace
TMat4    adj(const TMat4& m);                // Adjoint
TMReal   det(const TMat4& m);                // Determinant
TMat4    inv(const TMat4& m);                // Inverse
TMat4    oprod(const TMVec4& a, const TMVec4& b); // Outer product

// The xform functions help avoid dependence on whether row or column
// vectors are used to represent points and vectors.
TVec4    xform(const TMat4& m, const TVec4& v); // Transform of v by m
TVec3    xform(const TMat4& m, const TVec3& v); // Hom. xform of v by m
TMat4    xform(const TMat4& m, const TMat4& n); // Xform v -> m(n(v))
    

// --- Inlines ----------------------------------------------------------------

inline TMat4::TMat4()
{
}

inline TMat4::TMat4(const TMVec4& v0, const TMVec4& v1, const TMVec4& v2, const TMVec4& v3)
{
    row[0] = v0;
    row[1] = v1;
    row[2] = v2;
    row[3] = v3;
}

inline TMat4::TMat4(const TMat4& m)
{
    row[0] = m[0];
    row[1] = m[1];
    row[2] = m[2];
    row[3] = m[3];
}

inline TMVec4& TMat4::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, 4, "(Mat4::[i]) index out of range");
    return(row[i]);
}

inline const TMVec4& TMat4::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, 4, "(Mat4::[i]) index out of range");
    return(row[i]);
}

inline TMReal& TMat4::operator () (int i, int j)
{
    CL_RANGE_MSG(i, 0, 4, "(Mat2::(i,j)) index out of range");
    CL_RANGE_MSG(j, 0, 4, "(Mat2::(i,j)) index out of range");
    
    return(row[i][j]);
}

inline TMReal TMat4::operator () (int i, int j) const
{
    CL_RANGE_MSG(i, 0, 4, "(Mat2::(i,j)) index out of range");
    CL_RANGE_MSG(j, 0, 4, "(Mat2::(i,j)) index out of range");
    
    return(row[i][j]);
}

inline TMReal* TMat4::Ref() const
{
    return (TMReal*) row;
}

inline const TMReal* TMat4::ConstRef() const
{
    return((const TMReal*) row);
}

inline TMat4::TMat4(ZeroOrOne k)
{
    MakeDiag(TMReal(k));
}

inline TMat4::TMat4(Block k)
{
    MakeBlock(TMReal(k));
}

inline TMat4& TMat4::operator = (ZeroOrOne k)
{
    MakeDiag(TMReal(k));

    return(*this);
}

inline TMat4& TMat4::operator = (Block k)
{
    MakeBlock(TMReal(k));

    return(*this);
}

inline TMat4 operator * (TMReal s, const TMat4& m)
{
    return(m * s);
}

#ifdef VL_ROW_ORIENT
inline TVec3 xform(const TMat4& m, const TVec3& v)
{ return(proj(TVec4(v, 1.0) * m)); }
inline TVec4 xform(const TMat4& m, const TVec4& v)
{ return(v * m); }
inline TMat4 xform(const TMat4& m, const TMat4& n)
{ return(n * m); }
#else
inline TVec3 xform(const TMat4& m, const TVec3& v)
{ return(proj(m * TVec4(v, 1.0))); }
inline TVec4 xform(const TMat4& m, const TVec4& v)
{ return(m * v); }
inline TMat4 xform(const TMat4& m, const TMat4& n)
{ return(m * n); }
#endif

#endif
