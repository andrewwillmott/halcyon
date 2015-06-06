/*
    File:           VLMat.cpp

    Function:       Implements VLMat.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/

#include "VLMat.h"

#include <string.h>
#include <stdarg.h>


// --- Mat Constructors & Destructors -----------------------------------------


TMat::TMat(int rows, int cols, ZeroOrOne k) : mData(), mRows(rows), mCols(cols)
{
    CL_ASSERT_MSG(rows > 0 && cols > 0, "(Mat) illegal matrix size");
    
    mData = VL_NEW TMReal[rows * cols];
    
    MakeDiag(TMReal(k));
}

TMat::TMat(int rows, int cols, Block k) : mData(), mRows(rows), mCols(cols)
{
    CL_ASSERT_MSG(rows > 0 && cols > 0, "(Mat) illegal matrix size");
    
    mData = VL_NEW TMReal[rows * cols];
    
    MakeBlock(TMReal(k));
}

TMat::TMat(int rows, int cols, double elt0, ...) : mData(), mRows(rows), mCols(cols)
// The double is hardwired here because it is the only type that will work 
// with var args and C++ real numbers. 
{
    CL_ASSERT_MSG(rows > 0 && cols > 0, "(Mat) illegal matrix size");
    
    va_list ap;

    mData = VL_NEW TMReal[rows * cols];
    va_start(ap, elt0);
        
    SetReal(mData[0], elt0);
    
    for (int i = 1; i < cols; i++)
        SetReal(SELF[0][i], va_arg(ap, double));

    for (int i = 1; i < rows; i++)
        for (int j = 0; j < cols; j++)
            SetReal(SELF(i, j), va_arg(ap, double));

    va_end(ap);
}

TMat::TMat(const TMat& m) : mData(), mRows(m.Rows()), mCols(m.mCols)
{
    CL_ASSERT_MSG(m.mData != 0, "(Mat) Can't construct from null matrix");
    
    int elts = mRows * mCols;
    
    mData = VL_NEW TMReal[elts];
#ifdef VL_USE_MEMCPY
    memcpy(mData, m.mData, elts * sizeof(TMReal));
#else
    for (int i = 0; i < elts; i++)
        mData[i] = m.mData[i];
#endif
}

TMat::TMat(const TSubMat& m) : mData(), mRows(m.Rows()), mCols(m.Cols())
{
    mData = VL_NEW TMReal[mRows * mCols];

    for (int i = 0; i < mRows; i++)
        SELF[i] = m[i];
}


// --- Mat Assignment Operators -----------------------------------------------


TMat& TMat::operator = (const TMat& m)  
{   
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else if (!mData)
    {
        MakeRef(m);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");

    for (int i = 0; i < Rows(); i++) 
        SELF[i] = m[i];

    return SELF;
}

TMat& TMat::operator = (const TSubMat& m)
{   
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else
        CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
        
    for (int i = 0; i < Rows(); i++) 
        SELF[i] = m[i];

    return SELF;
}
      
#ifdef VL_Mat2_H
TMat& TMat::operator = (const TMat2& m)
{   
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else if (!mData)
    {
        MakeRef(m);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
        
    for (int i = 0; i < Rows(); i++) 
        SELF[i] = m[i];

    return SELF;
}
      
TMat& TMat::operator = (const TMat3& m)
{   
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else if (!mData)
    {
        MakeRef(m);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
        
    for (int i = 0; i < Rows(); i++) 
        SELF[i] = m[i];

    return SELF;
}
      
TMat& TMat::operator = (const TMat4& m)
{   
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else if (!mData)
    {
        MakeRef(m);
        return SELF;
    }
    else
        CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");

    for (int i = 0; i < Rows(); i++) 
        SELF[i] = m[i];

    return SELF;
}
#endif

void TMat::SetSize(int rows, int cols)
{
    CL_ASSERT_MSG(rows > 0 && cols > 0, "(Mat::SetSize) Illegal matrix size.");

    if (IsRef())
    {
        // Abort! We don't allow this operation on references.
        CL_ERROR("(Mat::SetSize) Trying to resize a matrix reference");
        return;
    }

    int    elts = rows * cols;
    int    oldElts = Rows() * mCols;

    mRows = rows;
    mCols = cols;

    // Don't reallocate if we already have enough storage
    if (elts <= oldElts)
        return;

    // Otherwise, delete old storage and reallocate
    VL_DELETE[] mData;
    mData = VL_NEW TMReal[elts];
}

void TMat::SetSize(const TMat& m)
{
    SetSize(m.Rows(), m.Cols());
}

void TMat::MakeZero()
{
    int n = Rows() * mCols;
    
    for (int i = 0; i < n; i++)
        mData[i] = vl_zero;
}

void TMat::MakeIdentity()
{
    for (int i = 0; i < Rows(); i++)
        for (int j = 0; j < Cols(); j++)
            SELF(i, j) = (i == j) ? TMReal(vl_one) : TMReal(vl_zero);
}

void TMat::MakeDiag(TMReal k)
{
    for (int i = 0; i < Rows(); i++)
        for (int j = 0; j < Cols(); j++)
            SELF(i, j) = (i == j) ? k : vl_zero;
}

void TMat::MakeBlock(TMReal k)
{
    int n = Rows() * mCols;
    
    for (int i = 0; i < n; i++)
        mData[i] = k;       
}


// --- Mat Assignment Operators -----------------------------------------------


TMat& TMat::operator += (const TMat& m)
{
    CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::+=) matrix rows don't match");    

    for (int i = 0; i < Rows(); i++) 
        SELF[i] += m[i];

    return SELF;
}

TMat& TMat::operator -= (const TMat& m)
{
    CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::-=) matrix rows don't match");    
    
    for (int i = 0; i < Rows(); i++) 
        SELF[i] -= m[i];
    
    return SELF;
}

TMat& TMat::operator *= (const TMat& m)
{
    CL_ASSERT_MSG(Cols() == m.Cols(), "(Mat::*=) matrix columns don't match"); 
    
    for (int i = 0; i < Rows(); i++) 
        SELF[i] = SELF[i] * m;
    
    return SELF;
}

TMat& TMat::operator *= (TMReal s)
{   
    for (int i = 0; i < Rows(); i++) 
        SELF[i] *= s;
    
    return SELF;
}

TMat& TMat::operator /= (TMReal s)
{   
    for (int i = 0; i < Rows(); i++) 
        SELF[i] /= s;
    
    return SELF;
}


// --- Mat Comparison Operators -----------------------------------------------


bool operator == (const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::==) matrix rows don't match");  
    
    for (int i = 0; i < m.Rows(); i++) 
        if (m[i] != n[i])
            return(0);

    return(1);
}

bool operator != (const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::!=) matrix rows don't match");  
    
    for (int i = 0; i < m.Rows(); i++) 
        if (m[i] != n[i])
            return(1);

    return(0);
}


// --- Mat Arithmetic Operators -----------------------------------------------


TMat operator + (const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::+) matrix rows don't match");   
    
    TMat result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Add(m[i], n[i], ri);
    }
    
    return(result);
}

TMat operator - (const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::-) matrix rows don't match");   
    
    TMat result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Subtract(m[i], n[i], ri);
    }
    
    return(result);
}

TMat operator - (const TMat& m)
{
    TMat result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Negate(m[i], ri);
    }
    
    return(result);
}

TMat operator * (const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(m.Cols() == n.Rows(), "(Mat::*m) matrix cols don't match");  
    
    TMat result(m.Rows(), n.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Multiply(m[i], n, ri);
    }
    
    return(result);
}

TMVec operator * (const TMat& m, const TMVec& v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(Mat::*v) matrix and vector sizes don't match");
    
    TMVec result(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(v, m[i]);
    
    return(result);
}

TMVec operator * (const TMVec& v, const TMat& m)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TMVec result(m.Cols(), vl_zero);
    
    for (int i = 0; i < m.Rows(); i++) 
        MultiplyAccum(m[i], v[i], result);
    
    return(result);
}

TMat operator * (const TMat& m, TMReal s)
{
    TMat result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Multiply(m[i], s, ri);
    }
    
    return(result);
}

TMat operator / (const TMat& m, TMReal s)
{
    TMat result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX VecRef
        Divide(m[i], s, ri);
    }
    
    return(result);
}


// --- Mat Special Functions --------------------------------------------------


TMat trans(const TMat& m)
{
    TMat    result(m.Cols(), m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        for (int j = 0; j < m.Cols(); j++)
            result(j,i) = m(i,j);
    
    return(result);
}

TMReal trace(const TMat& m)
{
    TMReal  result = vl_0;
    
    for (int i = 0; i < m.Rows(); i++) 
        result += m(i,i);
    
    return(result);
}

TMat& TMat::Clamp(Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    for (int i = 0; i < Rows(); i++)
        SELF[i].Clamp(fuzz);
            
    return SELF;
}

TMat& TMat::Clamp()
{
    return(Clamp(Real(1e-7)));
}

TMat clamped(const TMat& m, Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    TMat    result(m);
            
    return(result.Clamp(fuzz));
}

TMat clamped(const TMat& m)
{
    return(clamped(m, Real(1e-7)));
}

TMat oprod(const TMVec& a, const TMVec& b)
// returns outerproduct of a and b:  a * trans(b)
{
    TMat    result;
    
    result.SetSize(a.Elts(), b.Elts());
    for (int i = 0; i < a.Elts(); i++)
    {
        TMVec ri(result[i]); // XXX VecRef
        Multiply(b, a[i], ri);
    }

    return(result);
}


#ifndef VL_NO_SOLVE
//
//  inv: matrix inversion using Gaussian pivoting
//

#if !defined(CL_CHECKING) && !defined(VL_CHECKING)
// we #define away pAssertEps if it is not used, to avoid
// compiler warnings.
#define pAssertEps
#endif

TMat inv(const TMat& m, TMReal* determinant, TMReal pAssertEps)
{
    CL_ASSERT_MSG(m.IsSquare(), "(inv) Matrix not square");

    int  n = m.Rows();
    TMat A(m);
    TMat B(n, n, vl_I);      

    // ---------- Forward elimination ---------- ------------------------------
    
    TMReal det = TMReal(vl_1);
    
    for (int i = 0; i < n; i++)             // Eliminate in column i, below diag
    {       
        TMReal max = TMReal(-1);
        
        int j = 0;
        for (int k = i; k < n; k++)         // Find a pivot for column i 
            if (len(A(k, i)) > max)
            {
                max = len(A(k, i));
                j = k;
            }
            
        CL_ASSERT_MSG(max > pAssertEps, "(inv) Matrix not invertible");
                    
        if (j != i)                     // Swap rows i and j
        {           
            for (int k = i; k < n; k++)
            {
                TMReal aik = A(i, k);
                TMReal ajk = A(j, k);

                 A(i, k) = ajk;
                 A(j, k) = aik;
            }
            for (int k = 0; k < n; k++)
            {
                TMReal bik = B(i, k);
                TMReal bjk = B(j, k);

                 B(i, k) = bjk;
                 B(j, k) = bik;
            }

            det = -det;
        }
        
        TMReal pivot = A(i, i);
        CL_ASSERT_MSG(len(pivot) > pAssertEps, "(inv) Matrix not invertible");
        det *= pivot;
        
        for (int k = i + 1; k < n; k++)     // Only do elements to the right of the pivot 
            A(i, k) /= pivot;
            
        for (int k = 0; k < n; k++)
            B(i, k) /= pivot;
            
        // We know that A(i, i) will be set to 1, so don't bother to do it
    
        for (int j = i + 1; j < n; j++)
        {                               // Eliminate in rows below i 
            TMReal t = A(j, i);         // We're gonna zero this guy 
            for (int k = i + 1; k < n; k++) // Subtract scaled row i from row j 
                A(j, k) -= A(i, k) * t; // (Ignore k <= i, we know they're 0) 
            for (int k = 0; k < n; k++)
                B(j, k) -= B(i, k) * t;
        }
    }

    // ---------- Backward elimination ---------- -----------------------------

    for (int i = n - 1; i > 0; i--)         // Eliminate in column i, above diag 
    {       
        for (int j = 0; j < i; j++)         // Eliminate in rows above i 
        {       
            TMReal t = A(j, i);             // We're gonna zero this guy 
            for (int k = 0; k < n; k++)     // Subtract scaled row i from row j 
                B(j, k) -= B(i, k) * t;
        }
    }
    
    if (determinant)
        *determinant = det;

    return(B);
}
#endif


void Add(const TMat& m, const TMat& n, TMat& result)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::+) matrix rows don't match");   
    
    if (!result.IsRef())
        result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]); // XXX hack necessary because we don't have Ref distinction yet.
        Add(m[i], n[i], ri);
    }
}

void Subtract(const TMat& m, const TMat& n, TMat& result)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(Mat::-) matrix rows don't match");   
    
    if (!result.IsRef())
        result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        Subtract(m[i], n[i], ri);
    }
}

void Negate(const TMat& m, TMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        Negate(m[i], ri);
    }
}

void Multiply(const TMat& m, const TMat& n, TMat& result)
{
    CL_ASSERT_MSG(m.Cols() == n.Rows(), "(Mat::*m) matrix cols don't match");  
    
    result.SetSize(m.Rows(), n.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        Multiply(m[i], n, ri);
    }
}

void Multiply(const TMat& m, TMReal s, TMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        Multiply(m[i], s, ri);
    }
}

void MultiplyAccum(const TMat& m, TMReal s, TMat& result)
{
    CL_ASSERT_MSG(m.Rows() == result.Rows(), "matrix sizes don't match");

    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        MultiplyAccum(m[i], s, ri);
    }
}

void Divide(const TMat& m, TMReal s, TMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        TMVec ri(result[i]);
        Divide(m[i], s, ri);
    }
}

void Multiply(const TMat& m, const TMVec& v, TMVec& result)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(Mat::*v) matrix and vector sizes don't match");
    
    result.SetSize(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(v, m[i]);
}

void Multiply(const TMVec& v, const TMat& m, TMVec& result)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    result.SetSize(m.Cols());
    result.MakeZero();
    
    for (int i = 0; i < m.Rows(); i++) 
        MultiplyAccum(m[i], v[i], result);
}

void Transpose(const TMat& m, TMat& result)
{
    result.SetSize(m.Cols(), m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        for (int j = 0; j < m.Cols(); j++)
            result(j, i) = m(i, j);
}

void OuterProduct(const TMVec& a, const TMVec& b, TMat& result)
{
    result.SetSize(a.Elts(), b.Elts());
    
    for (int i = 0; i < a.Elts(); i++)
    {
        TMVec ri(result[i]); // XXX VecRef
        Multiply(b, a[i], ri);
    }
}

#ifndef VL_NO_SOLVE
bool Invert(const TMat& m, TMat& VL_RESTRICT B, TMReal* determinant, TMReal epsilon)
{
    CL_ASSERT(&m != &B);
    CL_ASSERT_MSG(m.IsSquare(), "(inv) Matrix not square");

    int  n = m.Rows();
    TMat A(m);
    B.SetSize(n, n);
    B.MakeIdentity();
    
    if (determinant)
        *determinant = 0;

    // ---------- Forward elimination ----------------------------------------
    
    TMReal det = vl_1;
    
    for (int i = 0; i < n; i++)             // Eliminate in column i, below diag
    {       
        TMReal max = -1.0;
        
        int j = 0;
        for (int k = i; k < n; k++)         // Find a pivot for column i 
            if (len(A[k][i]) > max)
            {
                max = len(A(k, i));
                j = k;
            }
            
        if (max < epsilon)
            return false;
                    
        if (j != i)                     // Swap rows i and j
        {           
            for (int k = i; k < n; k++)
            {
                TMReal aik = A(i, k);
                TMReal ajk = A(j, k);

                 A(i, k) = ajk;
                 A(j, k) = aik;
            }
            for (int k = 0; k < n; k++)
            {
                TMReal bik = B(i, k);
                TMReal bjk = B(j, k);

                 B(i, k) = bjk;
                 B(j, k) = bik;
            }

            det = -det;
        }
        
        TMReal pivot = A(i, i);
        if (abs(pivot) < epsilon)
            return false;
            
        det *= pivot;
        
        for (int k = i + 1; k < n; k++)     // Only do elements to the right of the pivot 
            A(i, k) /= pivot;
            
        for (int k = 0; k < n; k++)
            B(i, k) /= pivot;
            
        // We know that A(i, i) will be set to 1, so don't bother to do it
    
        for (int j = i + 1; j < n; j++)
        {                               // Eliminate in rows below i 
            TMReal t = A(j, i);         // We're gonna zero this guy 
            for (int k = i + 1; k < n; k++) // Subtract scaled row i from row j 
                A(j, k) -= A(i, k) * t; // (Ignore k <= i, we know they're 0) 
            for (int k = 0; k < n; k++)
                B(j, k) -= B(i, k) * t;
        }
    }

    // ---------- Backward elimination ---------- -----------------------------

    for (int i = n - 1; i > 0; i--)         // Eliminate in column i, above diag 
    {       
        for (int j = 0; j < i; j++)         // Eliminate in rows above i 
        {       
            TMReal t = A(j, i);             // We're gonna zero this guy 
            for (int k = 0; k < n; k++)     // Subtract scaled row i from row j 
                B(j, k) -= B(i, k) * t;
        }
    }
    
    if (determinant)
        *determinant = det;

    return true;
}
#endif

