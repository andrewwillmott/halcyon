/*
    File:           VLVol.cpp

    Function:       Implements VLVol.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 2013, Andrew Willmott

    Notes:          

*/

#include "VLVol.h"

TVol::TVol(int slices, int rows, int cols, ZeroOrOne k) :
    mData(VL_NEW TMReal[slices * rows * cols]),
    mSlices(slices),
    mRows(rows),
    mCols(cols)
{
    CL_ASSERT_MSG(slices > 0 && rows > 0 && cols > 0, "(Vol) illegal volume size");
    
    MakeDiag(TMReal(k));
}

TVol::TVol(int slices, int rows, int cols, Block k) :
    mData(VL_NEW TMReal[slices * rows * cols]),
    mSlices(slices),
    mRows(rows),
    mCols(cols)
{
    CL_ASSERT_MSG(slices > 0 && rows > 0 && cols > 0, "(Vol) illegal volume size");
    
    MakeBlock(TMReal(k));
}

TVol::TVol(const TVol& v) :
    mData(),
    mSlices(v.mSlices),
    mRows(v.mRows),
    mCols(v.mCols)
{
    CL_ASSERT_MSG(v.mData != 0, "(Vol) Can't construct from null matrix");
    
    int numElts = mSlices * mRows * mCols;
    
    mData = VL_NEW TMReal[numElts];

#ifdef VL_USE_MEMCPY
    memcpy(mData, v.mData, numElts * sizeof(TMReal));
#else
    for (int i = 0; i < numElts; i++)
        mData[i] = v.mData[i];
#endif
}


void TVol::SetSize(int slices, int rows, int cols)
{
    if (IsRef())
    {
        // Abort! We don't allow this operation on references.
        CL_ERROR("(Vol::SetSize) Trying to resize a volume reference");
    }

    CL_ASSERT_MSG(slices >= 0 && rows >= 0 && cols >= 0, "(Vol::SetSize) Illegal matrix size.");

    int oldElts = mSlices * mRows * mCols;
    int newElts = slices * rows * cols;

    mSlices = slices;
    mRows   = rows;
    mCols   = cols;

    // Don't reallocate if we already have enough storage
    if (newElts <= oldElts)
        return;

    // Otherwise, delete old storage and reallocate
    if (!IsRef())
        VL_DELETE[] mData;

    if (newElts > 0)
        mData = VL_NEW TMReal[newElts];
    else
        mData = 0;
}

TVol& TVol::operator = (const TVol& v)
{   
    if (!IsRef())
        SetSize(v.mSlices, v.mRows, v.mCols);
    else
        CL_ASSERT_MSG(mSlices == v.mSlices, "(Vol::=) Vol slices don't match");
        
#ifdef VL_Mat_H
    for (int i = 0; i < mSlices; i++)
        (*this)[i] = v[i];
#else
    int numElts = mSlices * mRows * mCols;

    for (int i = 0; i < numElts; i++)
        mData[i] = v.mData[i];
#endif

    return(*this);
}

#ifdef TODO
TVol& TVol::operator *= (const TVol& v)
{
    CL_ASSERT_MSG(Slices() == m.Slices(), "(Vol::*=) volume slices don't match");
    
    for (int i = 0, n = Slices(); i < n; i++)
        SELF[i] = SELF[i] * v;
    
    return SELF;
}
#endif

void TVol::MakeIdentity()
{
    MakeDiag(vl_one);
}

void TVol::MakeDiag(TMReal k)
{
    MakeZero();

    if (k == vl_zero)
        return;

    int n = mCols;
    if (mRows < n)
        n = mRows;
    if (mSlices < n)
        n = mSlices;

    for (int i = 0; i < n; i++)
        (*this)(i, i, i) = k;
}

void TVol::MakeBlock(TMReal k)
{
    int n = mSlices * mRows * mCols;
    
    for (int i = 0; i < n; i++)
        mData[i] = k;
}


void Add(const TVol& m, const TVol& n, TVol& r)
{
    CL_ASSERT_MSG(m.Slices() == n.Slices(), "(Add) sizes don't match");
    CL_ASSERT_MSG(m.Rows()   == n.Rows()  , "(Add) sizes don't match");
    CL_ASSERT_MSG(m.Cols()   == n.Cols()  , "(Add) sizes don't match");

    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    const TMReal* nd = n.Ref();
    TMReal*       rd = r.Ref();

    for (int i = 0; i < numElts; i++)
        rd[i] = md[i] + nd[i];
}

void Subtract(const TVol& m, const TVol& n, TVol& r)
{
    CL_ASSERT_MSG(m.Slices() == n.Slices(), "(Subtract) sizes don't match");
    CL_ASSERT_MSG(m.Rows()   == n.Rows()  , "(Subtract) sizes don't match");
    CL_ASSERT_MSG(m.Cols()   == n.Cols()  , "(Subtract) sizes don't match");

    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    const TMReal* nd = n.Ref();
    TMReal*       rd = r.Ref();

    for (int i = 0; i < numElts; i++)
        rd[i] = md[i] - nd[i];
}

void Negate(const TVol& m, TVol& r)
{
    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    TMReal*       rd = r.Ref();

    for (int i = 0; i < numElts; i++)
        rd[i] = -md[i];
}

void Multiply(const TVol& m, TMReal s, TVol& r)
{
    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    TMReal*       rd = r.Ref();

    for (int i = 0; i < numElts; i++)
        rd[i] = md[i] * s;
}

void MultiplyAccum(const TVol& m, TMReal s, TVol& r)
{
    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    TMReal*       rd = r.Ref();

    for (int i = 0; i < numElts; i++)
        rd[i] += md[i] * s;
}

void Divide(const TVol& m, TMReal s, TVol& r)
{
    r.SetSize(m);

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    TMReal*       rd = r.Ref();

    TMReal invS = TMReal(1) / s;

    for (int i = 0; i < numElts; i++)
        rd[i] = md[i] * invS;
}

TMReal InnerProduct(const TVol& m, const TVol& n)
{
    CL_ASSERT_MSG(m.Slices() == n.Slices(), "(InnerProduct) sizes don't match");
    CL_ASSERT_MSG(m.Rows()   == n.Rows()  , "(InnerProduct) sizes don't match");
    CL_ASSERT_MSG(m.Cols()   == n.Cols()  , "(InnerProduct) sizes don't match");

    int numElts = m.Slices() * m.Rows() * m.Cols();
    const TMReal* md = m.Ref();
    const TMReal* nd = n.Ref();

    TMReal result = md[0] * nd[0];

    for (int i = 1; i < numElts; i++)
        result += md[i] * nd[i];

    return result;
}
