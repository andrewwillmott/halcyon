/*
    File:           VLMixed.cpp
    
    Function:       Implements Mixed.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1997-2000, Andrew Willmott
*/

#include "VLMixed.h"


// --- Vector dot products ----------------------------------------------------


TMReal dot(const TMVec &a, const TVec &b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(Vec::dot) Vec sizes don't match");

    TMReal  sum = vl_zero;
    int     i;
        
    for (i = 0; i < a.Elts(); i++) 
        sum += a[i] * b[i];
    
    return(sum);
}


#ifdef VL_SparseVec_H

    TMReal dot(const TMSparseVec &a, const TSparseVec &b) 
    {
        CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::dot) Vec sizes don't match");

        TMReal      sum = vl_zero;
        TMSVIter    j(a);
        int         i;
        
        for (i = 0; i < b.pairs.NumItems() - 1; i++)
        {
            if (j.IncTo(b.pairs[i].index))
                sum += j.Data() * b.pairs[i].elt;
        }
        
        return(sum);
    }

    TMReal dot(const TMSparseVec &a, const TVec &b)
    {
        CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::dot) Vec sizes don't match");

        TMReal      sum = vl_zero;
        int         i;
        
        for (i = 0; i < a.pairs.NumItems() - 1; i++)
            sum += a.pairs[i].elt * b[a.pairs[i].index];
        
        return(sum);
    }

#endif


// --- Matrix-vector multiply -------------------------------------------------


TVec4 operator * (const TMat4 &m, const TVec4 &v)           // m * v
{
    TVec4 result;
    
    result[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + v[3] * m[0][3];
    result[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + v[3] * m[1][3];
    result[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + v[3] * m[2][3];
    result[3] = v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m[3][2] + v[3] * m[3][3];
    
    return(result);
}

TVec4 operator * (const TVec4 &v, const TMat4 &m)           // v * m
{
    TVec4 result;
    
    result[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
    result[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
    result[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
    result[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];
    
    return(result);
}

TVec4 &operator *= (TVec4 &v, const TMat4 &m)               // v *= m
{
    TVReal  t0, t1, t2;

    t0   = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
    t1   = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
    t2   = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
    v[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];
    v[0] = t0;
    v[1] = t1;
    v[2] = t2;
    
    return(v);
}

TVec operator * (const TMat &m, const TVec &v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(Mat::*v) matrix and vector sizes don't match");
    
    int     i;
    TVec    result(m.Rows());
    
    for (i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
    
    return(result);
}

TVec operator * (const TVec &v, const TMat &m)          // v * m
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TMVec   temp(m.Cols(), vl_zero);    // accumulate in high precision
    TVec    result(m.Cols());           // return low precision.
    int     i;
    
    for (i = 0; i < m.Rows(); i++) 
        temp += m[i] * v[i];
    
    for (i = 0; i < temp.Elts(); i++)
        result[i] = temp[i];
        
    return(result);
}

TVec &operator *= (TVec &v, const TMat &m)              // v *= m
{
    v = v * m;      // Can't optimise much here...
    
    return(v);
}


void Multiply(const TMat &m, const TVec &v, TVec& result)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(Mat::*v) matrix and vector sizes don't match");
    
    result.SetSize(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
}

void Multiply(const TVec &v, const TMat &m, TVec& result)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    result.SetSize(m.Cols());
    result.MakeZero();
    
    for (int i = 0; i < m.Rows(); i++) 
        MultiplyAccum(m[i], v[i], result);
}

void MultiplyAccum(const TMVec &v, const TVReal s, TVec &result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(SparseVec::MultiplyAccum) Vec sizes don't match");
    
    for (int i = 0; i < v.Elts(); i++) 
        result[i] += s * v[i];
}

void OuterProduct(const TVec &a, const TVec &b, TMat& result)
{
    result.SetSize(a.Elts(), b.Elts());
    
    for (int i = 0; i < a.Elts(); i++)
        for (int j = 0; j < b.Elts(); j++)
            result(i, j) = a[i] * b[j];
}


#ifdef VL_SparseVec_H

TSparseVec operator * (const TSparseMat &m, const TSparseVec &v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::m*v) matrix and vector sizes don't match");
    
    int         i;
    TSparseVec  result(m.Rows());
    
    result.Begin();

    for (i = 0; i < m.Rows(); i++) 
        result.AddElt(i, dot(m[i], v));
    
    result.End();
        
    return(result);
}

TSparseVec operator * (const TSparseVec &v, const TSparseMat &m)            // v * m
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TMSparseVec temp(m.Cols());
    TSparseVec  result(m.Cols());
    int         i;
    TSVIter     j(v);
    TMSVIter    k;
    
    temp = vl_zero; 
    
    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (i = 0; i < m.Rows(); i++) 
    {       
        j.Inc(i);
        if (j.Exists(i))
            temp += m[i] * j.Data();
    }
    
    result.SetNumElts(temp.Elts());
    result.Begin();
        
    for (k.Begin(temp.Elts()); !k.AtEnd(); k.Inc())
        result.AddNZElt(k.Index(), k.Data());

    result.End();

    return(result);
}

TSparseVec &operator *= (TSparseVec &v, const TSparseMat &m)        // v *= m
{
    TSparseVec t = v * m;
    v.Swap(t);
    return(v);
}

TVec operator * (const TSparseMat &m, const TVec &v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::*v) matrix and vector sizes don't match");
    
    int         i;
    TVec        result(m.Rows());
    
    for (i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
    
    return(result);
}

TVec operator * (const TVec &v, const TSparseMat &m)            // v * m
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TVec        result(m.Cols());
    int         i, j;

    result = vl_zero;   

    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (i = 0; i < m.Rows(); i++) 
        if (len(v[i]) > 0)
            for (j = 0; j < m[i].pairs.NumItems() - 1; j++)
                result[m[i].pairs[j].index] += v[i] * m[i].pairs[j].elt;
    
    return(result);
}

TVec &operator *= (TVec &v, const TSparseMat &m)                    // v *= m
{
    v = v * m;
    return(v);
}

void Multiply(const TSparseMat &m, const TVec &v, TVec& result)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::*v) matrix and vector sizes don't match");
    
    result.SetSize(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
}

void Multiply(const TVec &v, const TSparseMat &m, TVec& result)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
        
    result.SetSize(m.Cols());
    result.MakeZero();

    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (int i = 0; i < v.Elts(); i++) 
        if ((len(v[i]) > TMSparseVec::sFuzz)) // XXX push into MA?
            MultiplyAccum(m[i], v[i], result);
}

void MultiplyAccum(const TMSparseVec &v, TVReal s, TVec &result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(SparseVec::MultiplyAccum) Vec sizes don't match");
    
    result.SetSize(v.Elts());
            
    for (int i = 0, n = int(v.pairs.NumItems()) - 1; i < n; i++)
        result[v.pairs[i].index] += s * v.pairs[i].elt;
}

void MultiplyAccum(const TMSparseVec &v, TVReal s, TSparseVec &result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(SparseVec::+) Vec sizes don't match");
    
    TSparseVec t;
    
    t.SetSize(v.Elts());
    t.Begin();
    
    // Step through a and b in parallel
    for (int i = 0, j = 0; ; )
    {
        if (v.pairs[i].index == result.pairs[j].index)
        {
            // We have two elements at the same index. 
            // Are we at the end of both arrays?
            if (v.pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            // If not, add the result
            
            t.AddElt(v.pairs[i].index, v.pairs[i].elt * s + result.pairs[j].elt);   // +
            i++;
            j++;
        }
        else if (v.pairs[i].index < result.pairs[j].index)
        // result[x] = a.pairs[i] * s + 0       
        {   
            t.AddNZElt(v.pairs[i].index, v.pairs[i].elt * s); 
            i++;
        }
        else
        // result[x] = b.pairs[j] + 0       
        {   
            t.AddNZElt(result.pairs[j]); 
            j++;
        }
    }
            
    t.End();
    
    result.Swap(t);
}



#endif
