/*
    File:           VLSubVec.h

    Function:       Defines a scatter-gather vector, i.e., a subvector of another vector, 
                    or the row, column or diagonal of a matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_SubVec_H
#define VL_SubVec_H

#include "VL.h"

class TVec;
 
class TSubVec
{
public:
    // Constructors
    TSubVec(int n, int span, TVReal data[]);
    TSubVec(const TSubVec &v);
    TSubVec(const TVec &v);

    // Accessor functions
    int         Elts() const { return(elts); };

    TVReal      &operator [] (int i);
    TVReal      operator [] (int i) const;

    TSubVec     &operator = (const TSubVec &v);
    TSubVec     &operator = (const TVec &v);

    // In-Place operators
    TSubVec     &operator += (const TSubVec &b);
    TSubVec     &operator -= (const TSubVec &b);
    TSubVec     &operator *= (const TSubVec &b);
    TSubVec     &operator *= (TVReal s);
    TSubVec     &operator /= (const TSubVec &b);
    TSubVec     &operator /= (TVReal s);

    // Data
    int         elts;
    int         span;
    TVReal      *data; 
};


// --- Vec Arithmetic Operators -----------------------------------------------

TVec    operator + (const TSubVec &a, const TSubVec &b);
TVec    operator - (const TSubVec &a, const TSubVec &b);
TVec    operator - (const TSubVec &v);
TVec    operator * (const TSubVec &a, const TSubVec &b);      
TVec    operator * (const TSubVec &v, TVReal s);
TVec    operator / (const TSubVec &a, const TSubVec &b);
TVec    operator / (const TSubVec &v, TVReal s);
TVec    operator * (TVReal s, const TSubVec &v);


// --- Sub-vector functions ---------------------------------------------------

TSubVec  sub  (const TSubVec &v, int start, int length);   
TSubVec  first(const TSubVec &v, int length);    
TSubVec  last (const TSubVec &v, int length);     


// --- Inlines ----------------------------------------------------------------

inline TVReal &TSubVec::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, elts, "Vec::[i]");
    
    return(data[i * span]);
}

inline TVReal TSubVec::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, elts, "Vec::[i]");

    return(data[i * span]);
}

inline TSubVec sub(const TSubVec &v, int start, int length)
{
    CL_ASSERT_MSG(start >= 0 && length > 0 && start + length <= v.Elts(),
        "(sub(Vec)) illegal subset of vector");

    return(TSubVec(length, v.span, v.data + start * v.span));
}

inline TSubVec first(const TSubVec &v, int length)
{
    CL_ASSERT_MSG(length > 0 && length <= v.Elts(),
        "(first(Vec)) illegal subset of vector");

    return(TSubVec(length, v.span, v.data));
}

inline TSubVec last(const TSubVec &v, int length)
{
    CL_ASSERT_MSG(length > 0 && length <= v.Elts(),
        "(last(Vec)) illegal subset of vector");

    return(TSubVec(length, v.span, v.data + (v.elts - length) * v.span));
}

#endif
