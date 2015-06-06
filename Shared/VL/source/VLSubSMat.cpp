/*
    File:           VLSubSMat.cpp

    Function:       Implements VLSubSMat.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLSubSMat.h"
#include "VLSparseMat.h"
#include "VLSubSVec.h"


// --- SubSMat Constructors & Destructors -------------------------------------


TSubSMat::TSubSMat(int m, int n, int start, TMSparseVec *target) : 
    rows(m), cols(n), start(start), target(target)
{
}

TSubSMat::TSubSMat(const TSubSMat &m) :
    rows(m.rows), cols(m.cols), start(m.start), target(m.target)
{
}


// --- SubSMat Assignment Operators -------------------------------------------

TSubSMat &TSubSMat::operator = (const TSubSMat &m)
{   
    int i;
    
    for (i = 0; i < m.Rows(); i++) 
        (*this)[i] = m[i];
        
    return(*this);
}
      
TSubSMat &TSubSMat::operator = (const TSparseMat &m)
{   
    int i;
    
    for (i = 0; i < m.Rows(); i++) 
        (*this)[i] = TMSparseVec(m[i]); // XXX want TSV.Assign()
        
    return(*this);
}

TSubSMat &TSubSMat::operator = (const TMat &m)
{   
    int i;
    
    for (i = 0; i < m.Rows(); i++) 
        (*this)[i] = TMSparseVec(m[i]);
        
    return(*this);
}

TSubSMat sub(const TSparseMat &m, int top, int left, int nrows, int ncols)
{
    CL_ASSERT_MSG(left >= 0 && ncols > 0 && left + ncols <= m.Cols(), 
           "(sub(SparseMat)) illegal subset of matrix");
    CL_ASSERT_MSG(top >= 0 && nrows > 0 && top + nrows <= m.Rows(), 
           "(sub(SparseMat)) illegal subset of matrix");

    return(TSubSMat(nrows, ncols, left, m.Ref() + top));
}

TSubSMat sub(const TSparseMat &m, int nrows, int ncols)
{
    CL_ASSERT_MSG(ncols > 0 && ncols <= m.Cols(), 
           "(sub(SparseMat)) illegal subset of matrix");
    CL_ASSERT_MSG(nrows > 0 && nrows <= m.Rows(), 
           "(sub(SparseMat)) illegal subset of matrix");

    return(TSubSMat(nrows, ncols, 0, m.Ref()));
}

TMSubSVec col(const TSparseMat &m, int i)
{
    CL_RANGE_MSG(i, 0, m.Cols(), "(col(SparseMat)) illegal column index");

    return(TSubSVec(i, m.Rows(), m.Ref(), m.Cols()));
}

TMSubSVec diag(const TSparseMat &m, int diagNum)
{
    if (diagNum == 0)
        return(TSubSVec(0, std::min<int>(m.Rows(), m.Cols()), m.Ref(), m.Cols() + 1));
    else if (diagNum < 0)
        return(TSubSVec(0, std::min<int>(m.Rows() + diagNum, m.Cols()), 
                        m.Ref() - diagNum, m.Cols() + 1));
    else
        return(TSubSVec(diagNum, std::min<int>(m.Cols() - diagNum, m.Rows()), m.Ref(), 
                        m.Cols() + 1));
}

