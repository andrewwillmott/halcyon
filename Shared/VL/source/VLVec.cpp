/*
    File:           VLVec.cpp

    Function:       Implements VLVec.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLVec.h"

#include <stdarg.h>

#ifdef VL_USE_MEMCPY
    #include <string.h>
#endif


// --- Vec Constructors -------------------------------------------------------


TVec::TVec(const TVec& v) : mData(0), mElts(v.Elts())
{
    if (mElts)
    {
        mData = VL_NEW TVReal[mElts];

    #ifdef VL_USE_MEMCPY
        memcpy(mData, v.Ref(), sizeof(TVReal) * Elts());
    #else
        for (int i = 0; i < Elts(); i++)
            mData[i] = v[i];
    #endif
    }
}

TVec::TVec(const TSubVec& v) : mData(), mElts(v.Elts())
{
    mData = VL_NEW TVReal[mElts];    
    CL_ASSERT_MSG(mData != 0, "(Vec) Out of memory");
    
    for (int i = 0; i < Elts(); i++)
        mData[i] = v[i];
}

TVec::TVec(int n, double elt0, ...) : mData(), mElts(n)
{
    CL_ASSERT_MSG(n > 0,"(Vec) illegal vector size");

    va_list ap;
        
    mData = VL_NEW TVReal[n];
    va_start(ap, elt0);
        
    SetReal(mData[0], elt0);

    int i = 1;
    while (--n)
        SetReal(mData[i++], va_arg(ap, double));

    va_end(ap);
}

TVec::~TVec()
{
    if (!IsRef())
    {
        VL_DELETE[] mData;
        mData = 0;
    }
}


// --- Vec Assignment Operators -----------------------------------------------


TVec& TVec::operator = (const TVec& v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else if (!mData)
    {
        MakeRef(v);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

#ifdef VL_USE_MEMCPY
    memcpy(mData, v.mData, sizeof(TVReal) * Elts());
#else
    for (int i = 0; i < Elts(); i++)
        mData[i] = v[i];
#endif

    return(*this);
}

TVec& TVec::operator = (const TSubVec& v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else
        CL_ASSERT_MSG(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

    for (int i = 0; i < Elts(); i++)
        mData[i] = v[i];

    return(*this);
}

#ifdef VL_Vec2_H
TVec& TVec::operator = (const TVec2& v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else if (!mData)
    {
        MakeRef(v);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");
    
    mData[0] = v[0];
    mData[1] = v[1];
    
    return(*this);
}

TVec& TVec::operator = (const TVec3& v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else if (!mData)
    {
        MakeRef(v);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");
    
    mData[0] = v[0];
    mData[1] = v[1];
    mData[2] = v[2];
    
    return(*this);
}

TVec& TVec::operator = (const TVec4& v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else if (!mData)
    {
        MakeRef(v);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");
    
    mData[0] = v[0];
    mData[1] = v[1];
    mData[2] = v[2];
    mData[3] = v[3];
    
    return(*this);
}
#endif

void TVec::SetSize(int n)
{
    if (IsRef())
    {
        // We don't allow this operation on references.
        CL_ASSERT_MSG(n == Elts(), ("(Vec::SetSize) Trying to resize a vector reference"));
        return;
    }

    // Don't reallocate if we already have enough storage
    if (n <= int(mElts))
    {
        mElts = n;
        return;
    }
        
    // Otherwise, delete old storage and reallocate
    mElts = n;
    VL_DELETE[] mData;
    mData = 0;
    mData = VL_NEW TVReal[mElts]; // may throw an exception
}

TVec& TVec::MakeZero()
{
#ifdef VL_USE_MEMCPY
    memset(mData, 0, sizeof(TVReal) * Elts());
#else
    for (int i = 0; i < Elts(); i++)
        mData[i] = vl_zero;  
#endif

    return(*this);
}

TVec& TVec::MakeUnit(int i, TVReal k)
{
    MakeZero();
    mData[i] = k;

    return(*this);
}

TVec& TVec::MakeBlock(TVReal k)
{
    for (int i = 0; i < Elts(); i++)
        mData[i] = k;

    return(*this);
}

TVec& TVec::Clamp(Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    for (int i = 0; i < Elts(); i++)
        if (len((*this)[i]) < fuzz)
            (*this)[i] = vl_zero;
            
    return(*this);
}

TVec& TVec::Clamp()
{
    return(Clamp(Real(1e-7)));
}


// --- Vec In-Place operators -------------------------------------------------


TVec& TVec::operator += (const TVec& b)
{
    CL_ASSERT_MSG(Elts() == b.Elts(), "(Vec::+=) vector sizes don't match");   

    for (int i = 0; i < Elts(); i++) 
        mData[i] += b[i];
    
    return(*this);
}

TVec& TVec::operator -= (const TVec& b)
{
    CL_ASSERT_MSG(Elts() == b.Elts(), "(Vec::-=) vector sizes don't match");   

    for (int i = 0; i < Elts(); i++) 
        mData[i] -= b[i];
        
    return(*this);
}

TVec& TVec::operator *= (const TVec& b)
{
    CL_ASSERT_MSG(Elts() == b.Elts(), "(Vec::*=) Vec sizes don't match");

    for (int i = 0; i < Elts(); i++) 
        mData[i] *= b[i];
    
    return(*this);
}

TVec& TVec::operator *= (TVReal s)
{
    for (int i = 0; i < Elts(); i++) 
        mData[i] *= s;
    
    return(*this);
}

TVec& TVec::operator /= (const TVec& b)
{
    CL_ASSERT_MSG(Elts() == b.Elts(), "(Vec::/=) Vec sizes don't match");

    for (int i = 0; i < Elts(); i++) 
        mData[i] /= b[i];
    
    return(*this);
}

TVec& TVec::operator /= (TVReal s)
{
    for (int i = 0; i < Elts(); i++) 
        mData[i] /= s;
    
    return(*this);
}


// --- Vec Comparison Operators -----------------------------------------------


bool operator == (const TVec& a, const TVec& b)
{
    for (int i = 0; i < a.Elts(); i++) 
        if (a[i] != b[i])
            return(0);
    
    return(1);
}

bool operator != (const TVec& a, const TVec& b)
{
    for (int i = 0; i < a.Elts(); i++) 
        if (a[i] != b[i])
            return(1);
    
    return(0);
}


// --- Vec Arithmetic Operators -----------------------------------------------


TVec operator + (const TVec& a, const TVec& b)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::+) Vec sizes don't match");

    TVec    result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] + b[i];
    
    return(result);
}

TVec operator - (const TVec& a, const TVec& b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::-) Vec sizes don't match");
    
    TVec    result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] - b[i];
    
    return(result);
}

TVec operator - (const TVec& v)
{
    TVec    result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = - v[i];
    
    return(result);
}

TVec operator * (const TVec& a, const TVec& b)          
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::*) Vec sizes don't match");
    
    TVec    result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] * b[i];
    
    return(result);
}

TVec operator * (const TVec& v, TVReal s) 
{
    TVec    result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * s;
    
    return(result);
}

TVec operator / (const TVec& a, const TVec& b)          
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::/) Vec sizes don't match");
    
    TVec    result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] / b[i];
    
    return(result);
}

TVec operator / (const TVec& v, TVReal s) 
{
    TVec    result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] / s;
    
    return(result);
}

TVec operator * (TVReal s, const TVec& v)
{
    TVec    result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * s;
    
    return(result);
}

TVReal dot(const TVec& a, const TVec& b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::dot) Vec sizes don't match");

    TMReal  sum = TMReal(vl_zero);
        
    for (int i = 0; i < a.Elts(); i++) 
        sum += a[i] * b[i];
    
    return(sum);
}

TVec clamped(const TVec& v, Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    TVec    result(v);
                    
    return(result.Clamp(fuzz));
}

TVec clamped(const TVec& v)
{
    return(clamped(v, Real(1e-7)));
}

void Add(const TVec& a, const TVec& b, TVec& result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::+) Vec sizes don't match");

    result.SetSize(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] + b[i];
}

void Subtract(const TVec& a, const TVec& b, TVec& result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::-) Vec sizes don't match");
    
    result.SetSize(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] - b[i];
}

void Negate(const TVec& v, TVec& result)
{
    result.SetSize(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = - v[i];
}

void Multiply(const TVec& a, const TVec& b, TVec& result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::*) Vec sizes don't match");
    
    result.SetSize(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] * b[i];
}

void Multiply(const TVec& v, TVReal s, TVec& result)
{
    result.SetSize(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * s;
}

void MultiplyAccum(const TVec& v, const TVReal s, TVec& result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(Vec::MultiplyAccum) Vec sizes don't match");
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] += s * v[i];
}

void Divide(const TVec& a, const TVec& b, TVec& result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::/) Vec sizes don't match");
    
    result.SetSize(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] / b[i];
}

void Divide(const TVec& v, TVReal s, TVec& result)
{
    result.SetSize(v.Elts());

    TVReal t = TVReal(vl_1) / s;
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * t;
}

void Normalize(const TVec& v, TVec& result)
{
    Divide(v, len(v), result);
}

void NormalizeSafe(const TVec& v, TVec& result)
{
    Divide(v, len(v) + TVReal(1e-8), result);
}
