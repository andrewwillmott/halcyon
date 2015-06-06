/*
    File:           VLMat2.h

    Function:       Defines a 2 x 2 matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Mat2_H
#define VL_Mat2_H

#include "VL.h"
#include "VLVec2.h"


// --- Mat2 Class -------------------------------------------------------------

class TMat2 
{
public:
    // Constructors
    TMat2();
    TMat2(TMReal a, TMReal b, TMReal c, TMReal d);
    TMat2(const TMat2& m);
    TMat2(const TMVec2& v0, const TMVec2& v1);
    TMat2(ZeroOrOne k);
    TMat2(Block k);
    
    // Accessor functions
    int           Elts() const { return 4; };
    int           Rows() const { return 2; };
    int           Cols() const { return 2; };
        
    TMVec2&       operator [] (int i);
    const TMVec2& operator [] (int i) const;

    TMReal&       operator () (int i, int j);
    TMReal        operator () (int i, int j) const;
    
    TMReal*       Ref() const;               // Return pointer to data
    const TMReal* ConstRef() const;

    // Assignment operators
    TMat2&        operator =  (const TMat2& m); 
    TMat2&        operator =  (ZeroOrOne k); 
    TMat2&        operator =  (Block k); 
    TMat2&        operator += (const TMat2& m);     
    TMat2&        operator -= (const TMat2& m);      
    TMat2&        operator *= (const TMat2& m);      
    TMat2&        operator *= (TMReal s);                
    TMat2&        operator /= (TMReal s);                
    
    // Comparison operators    
    bool          operator == (const TMat2& m) const; // M == N?
    bool          operator != (const TMat2& m) const; // M != N?
    
    // Arithmetic operators    
    TMat2         operator + (const TMat2& m) const;  // M + N
    TMat2         operator - (const TMat2& m) const;  // M - N
    TMat2         operator - () const;                // -M
    TMat2         operator * (const TMat2& m) const;  // M * N
    TMat2         operator * (TMReal s) const;        // M * s
    TMat2         operator / (TMReal s) const;        // M / s
    
    // Initialisers
    void          MakeZero();                         // Zero matrix
    void          MakeIdentity();                     // Identity matrix
    void          MakeDiag(TMReal k = vl_one);        // I
    void          MakeBlock(TMReal k = vl_one);       // all elts=k
    
    // Vector Transformations
    TMat2&        MakeRot(Real theta);
    TMat2&        MakeScale(const TMVec2& s);

protected:
    // Data
    TMVec2  row[2];     // Rows of the matrix
};


// --- Matrix operators -------------------------------------------------------


TMVec2&  operator *= (TMVec2& v, const TMat2& m);   // v *= m
TMVec2   operator * (const TMat2& m, const TMVec2& v);// m * v
TMVec2   operator * (const TMVec2& v, const TMat2& m);// v * m
TMat2    operator * (TMReal s, const TMat2& m);      // s * m

TMat2    trans(const TMat2& m);              // Transpose
TMReal   trace(const TMat2& m);              // Trace
TMat2    adj(const TMat2& m);                // Adjoint
TMReal   det(const TMat2& m);                // Determinant
TMat2    inv(const TMat2& m);                // Inverse
TMat2    oprod(const TMVec2& a, const TMVec2& b); // Outer product

// The xform functions help avoid dependence on whether row or column
// vectors are used to represent points and vectors.
TVec2    xform(const TMat2& m, const TVec2& v); // Transform of v by m
TMat2    xform(const TMat2& m, const TMat2& n); // Xform v -> m(n(v))


// --- Inlines ----------------------------------------------------------------


inline TMat2::TMat2()
{
}

inline TMat2::TMat2(TMReal a, TMReal b, TMReal c, TMReal d)
{
    row[0][0] = a;  row[0][1] = b;
    row[1][0] = c;  row[1][1] = d;
}

inline TMat2::TMat2(const TMat2& m)
{
    row[0] = m[0];
    row[1] = m[1];
}

inline TMat2::TMat2(const TMVec2& v0, const TMVec2& v1)
{
    row[0] = v0;
    row[1] = v1;
}

inline TMVec2& TMat2::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, 2, "(Mat2::[i]) index out of range");
    return(row[i]);
}

inline const TMVec2& TMat2::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, 2, "(Mat2::[i]) index out of range");
    return(row[i]);
}

inline TMReal& TMat2::operator () (int i, int j)
{
    CL_RANGE_MSG(i, 0, 2, "(Mat2::(i,j)) index out of range");
    CL_RANGE_MSG(j, 0, 2, "(Mat2::(i,j)) index out of range");
    
    return(row[i][j]);
}

inline TMReal TMat2::operator () (int i, int j) const
{
    CL_RANGE_MSG(i, 0, 2, "(Mat2::(i,j)) index out of range");
    CL_RANGE_MSG(j, 0, 2, "(Mat2::(i,j)) index out of range");
    
    return(row[i][j]);
}

inline TMReal* TMat2::Ref() const
{
    return((TMReal*) row);
}

inline const TMReal* TMat2::ConstRef() const
{
    return((const TMReal*) row);
}

inline void TMat2::MakeZero()
{
    row[0][0] = vl_zero; row[0][1] = vl_zero;
    row[1][0] = vl_zero; row[1][1] = vl_zero;
}

inline void TMat2::MakeDiag(TMReal k)
{
    row[0][0] = k;          row[0][1] = vl_zero;
    row[1][0] = vl_zero;    row[1][1] = k;
}

inline void TMat2::MakeIdentity()
{
    row[0][0] = vl_one;     row[0][1] = vl_zero;
    row[1][0] = vl_zero;    row[1][1] = vl_one;
}

inline void TMat2::MakeBlock(TMReal k)
{
    row[0][0] = k; row[0][1] = k;
    row[1][0] = k; row[1][1] = k;
}

inline TMat2::TMat2(ZeroOrOne k)
{
    MakeDiag(TMReal(k));
}

inline TMat2::TMat2(Block k)
{
    MakeBlock(TMReal(k));
}

inline TMat2& TMat2::operator = (ZeroOrOne k)
{
    MakeDiag(TMReal(k));

    return(*this);
}
      
inline TMat2& TMat2::operator = (Block k)
{
    MakeBlock(TMReal(k));

    return(*this);
}
      
inline TMat2& TMat2::operator = (const TMat2& m)
{
    row[0] = m[0];
    row[1] = m[1];
    
    return(*this);
}
      
inline TMat2& TMat2::operator += (const TMat2& m)
{
    row[0] += m[0];
    row[1] += m[1];
    
    return(*this);
}

inline TMat2& TMat2::operator -= (const TMat2& m)
{
    row[0] -= m[0];
    row[1] -= m[1];

    return(*this);
}

inline TMat2& TMat2::operator *= (const TMat2& m)
{
    *this = *this * m; 

    return(*this);
}

inline TMat2& TMat2::operator *= (TMReal s)
{
    row[0] *= s;
    row[1] *= s;

    return(*this);
}

inline TMat2& TMat2::operator /= (TMReal s)
{
    row[0] /= s;
    row[1] /= s;

    return(*this);
}


inline TMat2 TMat2::operator + (const TMat2& m) const
{
    TMat2 result;
    
    result[0] = row[0] + m[0];
    result[1] = row[1] + m[1];

    return(result); 
}

inline TMat2 TMat2::operator - (const TMat2& m) const
{
    TMat2 result;
    
    result[0] = row[0] - m[0];
    result[1] = row[1] - m[1];

    return(result); 
}

inline TMat2 TMat2::operator - () const
{
    TMat2 result;
    
    result[0] = -row[0];
    result[1] = -row[1];

    return(result); 
}

inline TMat2 TMat2::operator * (const TMat2& m) const
{
#define _N_(x,y) row[x][y]
#define _M_(x,y) m.row[x][y]
#define _R_(x,y) result[x][y]

    TMat2 result;

    _R_(0,0) = _N_(0,0) * _M_(0,0) + _N_(0,1) * _M_(1,0);
    _R_(0,1) = _N_(0,0) * _M_(0,1) + _N_(0,1) * _M_(1,1);
    _R_(1,0) = _N_(1,0) * _M_(0,0) + _N_(1,1) * _M_(1,0);
    _R_(1,1) = _N_(1,0) * _M_(0,1) + _N_(1,1) * _M_(1,1);

    return(result); 
    
#undef _N_
#undef _M_
#undef _R_
}

inline TMat2 TMat2::operator * (TMReal s) const
{
    TMat2 result;
    
    result[0] = row[0] * s;
    result[1] = row[1] * s;

    return(result); 
}

inline TMat2 TMat2::operator / (TMReal s) const
{
    TMat2 result;
    
    result[0] = row[0] / s;
    result[1] = row[1] / s;

    return(result); 
}

inline TMat2  operator *  (TMReal s, const TMat2& m)
{
    return(m * s);
}

inline TMVec2 operator * (const TMat2& m, const TMVec2& v)
{
    TMVec2 result;

    result[0] = m[0][0] * v[0] + m[0][1] * v[1];
    result[1] = m[1][0] * v[0] + m[1][1] * v[1];

    return(result);
}

inline TMVec2 operator * (const TMVec2& v, const TMat2& m)          
{
    TMVec2 result;

    result[0] = v[0] * m[0][0] + v[1] * m[1][0];
    result[1] = v[0] * m[0][1] + v[1] * m[1][1];

    return(result);
}

inline TMVec2& operator *= (TMVec2& v, const TMat2& m)      
{
    TMReal t;
    
    t    = v[0] * m[0][0] + v[1] * m[1][0];
    v[1] = v[0] * m[0][1] + v[1] * m[1][1];
    v[0] = t;

    return(v);
}


inline TMat2 trans(const TMat2& m)
{
    TMat2 result;

    result[0][0] = m[0][0]; result[0][1] = m[1][0]; 
    result[1][0] = m[0][1]; result[1][1] = m[1][1]; 
        
    return(result);
}

inline TMReal trace(const TMat2& m)
{
    return(m[0][0] + m[1][1]);
}
            
inline TMat2 adj(const TMat2& m)            
{
    TMat2 result;

    result[0] = -cross(m[1]);
    result[1] =  cross(m[0]);
            
    return(result);
}

#ifdef VL_ROW_ORIENT
inline TVec2 xform(const TMat2& m, const TVec2& v)
{ return(v * m); }
inline TMat2 xform(const TMat2& m, const TMat2& n)
{ return(n * m); }
#else
inline TVec2 xform(const TMat2& m, const TVec2& v)
{ return(m * v); }
inline TMat2 xform(const TMat2& m, const TMat2& n)
{ return(m * n); }
#endif

#endif
