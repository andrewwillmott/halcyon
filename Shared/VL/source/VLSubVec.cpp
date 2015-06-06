/*
    File:           VLSubVec.cpp

    Function:       Implements VLSubVec.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/

#include "VLSubVec.h"
#include "VLVec.h"


// --- Vector Memory Management -----------------------------------------------


TSubVec::TSubVec(int n, int span, TVReal data[]) : elts(n), span(span),
    data(data)
{
}

TSubVec::TSubVec(const TSubVec &v) : elts(v.elts), span(v.span), data(v.data)
{
}

TSubVec::TSubVec(const TVec &v) : elts(v.Elts()), span(1), data(v.Ref())
{
}

TSubVec &TSubVec::operator = (const TSubVec &v)
{
    CL_ASSERT_MSG(Elts() == v.Elts(), "(SubVec::=) Vector sizes don't match");
    int i;
    
    for (i = 0; i < Elts(); i++)
        (*this)[i] = v[i];
        
    return(*this);
}

TSubVec &TSubVec::operator = (const TVec &v)
{
    CL_ASSERT_MSG(Elts() == v.Elts(), "(SubVec::=) Vector sizes don't match");
    int i;
    
    for (i = 0; i < Elts(); i++)
        (*this)[i] = v[i];
        
    return(*this);
}


// --- SubVec In-Place operators ----------------------------------------------


TSubVec &TSubVec::operator += (const TSubVec &sv)
{
    CL_ASSERT_MSG(Elts() == sv.Elts(), "(Vec::+=) vector sizes don't match");  

    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] += sv[i];
    
    return(*this);
}

TSubVec &TSubVec::operator -= (const TSubVec &sv)
{
    CL_ASSERT_MSG(Elts() == sv.Elts(), "(Vec::-=) vector sizes don't match");  

    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] -= sv[i];
        
    return(*this);
}

TSubVec &TSubVec::operator *= (const TSubVec &sv)
{
    CL_ASSERT_MSG(Elts() == sv.Elts(), "(Vec::*=) Vec sizes don't match");

    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] *= sv[i];
    
    return(*this);
}

TSubVec &TSubVec::operator *= (TVReal s)
{
    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] *= s;
    
    return(*this);
}

TSubVec &TSubVec::operator /= (const TSubVec &sv)
{
    CL_ASSERT_MSG(Elts() == sv.Elts(), "(Vec::/=) Vec sizes don't match");

    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] /= sv[i];
    
    return(*this);
}

TSubVec &TSubVec::operator /= (TVReal s)
{
    int     i;
    
    for (i = 0; i < Elts(); i++) 
        (*this)[i] /= s;
    
    return(*this);
}


// --- Vec Arithmetic Operators -----------------------------------------------


TVec operator + (const TSubVec &a, const TSubVec &b)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::+) Vec sizes don't match");

    TVec result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] + b[i];
    
    return(result);
}

TVec operator - (const TSubVec &a, const TSubVec &b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::-) Vec sizes don't match");
    
    TVec result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] - b[i];
    
    return(result);
}

TVec operator - (const TSubVec &v)
{
    TVec result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = - v[i];
    
    return(result);
}

TVec operator * (const TSubVec &a, const TSubVec &b)          
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::*) Vec sizes don't match");
    
    TVec result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] * b[i];
    
    return(result);
}

TVec operator * (const TSubVec &v, TVReal s) 
{
    TVec result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * s;
    
    return(result);
}

TVec operator / (const TSubVec &a, const TSubVec &b)          
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::/) Vec sizes don't match");
    
    TVec result(a.Elts());
    
    for (int i = 0; i < a.Elts(); i++) 
        result[i] = a[i] / b[i];
    
    return(result);
}

TVec operator / (const TSubVec &v, TVReal s) 
{
    TVec result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] / s;
    
    return(result);
}

TVec operator * (TVReal s, const TSubVec &v)
{
    TVec result(v.Elts());
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] = v[i] * s;
    
    return(result);
}

