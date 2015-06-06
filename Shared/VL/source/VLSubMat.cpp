/*
    File:           VLSubMat.cpp

    Function:       Implements VLSubMat.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLSubMat.h"
#include "VLMat.h"


// --- SubMat Constructors & Destructors --------------------------------------


TSubMat::TSubMat(int m, int n, int span, TMReal data[]) :
        rows(m), cols(n),
        span(span), data(data)
{
}

TSubMat::TSubMat(const TSubMat &m) :
        rows(m.rows), cols(m.cols),
        span(m.span), data(m.data)
{
}


// --- SubMat Assignment Operators --------------------------------------------


TSubMat &TSubMat::operator = (const TSubMat &m)
{   
    CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
    for (int i = 0; i < Rows(); i++) 
        (*this)[i] = m[i];

    return(*this);
}
      
TSubMat &TSubMat::operator = (const TMat &m)
{
    CL_ASSERT_MSG(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
    for (int i = 0; i < Rows(); i++) 
        (*this)[i] = m[i];

    return(*this);
}


// --- Sub functions: Mat ------------------------------------------------------


TSubMat sub(const TMat &m, int top, int left, int height, int width)
{
    CL_ASSERT_MSG(left >= 0 && width > 0 && left + width <= m.Cols(), "(sub(Mat)) illegal subset of matrix");
    CL_ASSERT_MSG(top >= 0 && height > 0 && top + height <= m.Rows(), "(sub(Mat)) illegal subset of matrix");

    TSubMat result(height, width, m.Cols(), m.Ref() + top * m.Cols() + left);

    return(result);
}

TSubMat sub(const TMat &m, int nrows, int ncols)
{
    CL_ASSERT_MSG(ncols > 0 && nrows > 0 && nrows <= m.Rows() && ncols <= m.Cols(), 
        "(sub(Mat)) illegal subset of matrix");

    TSubMat result(nrows, ncols, m.Cols(), m.Ref());

    return(result);
}

TMSubVec col(const TMat &m, int i)
{
    CL_RANGE_MSG(i, 0, m.Cols(), "(col(Mat)) illegal column index");

    return(TMSubVec(m.Rows(), m.Cols(), m.Ref() + i));
}

TMSubVec row(const TMat &m, int i)
{
    CL_RANGE_MSG(i, 0, m.Rows(), "(row(Mat)) illegal row index");

    return(TMSubVec(m.Cols(), 1, m[i].Ref()));
}

TMSubVec diag(const TMat &m, int diagNum)
{
    CL_RANGE_MSG(diagNum, 1 - m.Rows(), m.Cols(), "(row(Mat)) illegal row index");

    if (diagNum == 0)
        return(TMSubVec(std::min<int>(m.Rows(), m.Cols()), m.Cols() + 1, m.Ref()));
    else if (diagNum < 0)
        return(TMSubVec(std::min<int>(m.Rows() + diagNum, m.Cols()), m.Cols() + 1,
            m.Ref() - diagNum * m.Cols()));
    else
        return(TMSubVec(std::min<int>(m.Cols() - diagNum, m.Rows()), m.Cols() + 1,
            m.Ref() + diagNum));
}

// --- Sub functions: SubMat ---------------------------------------------------


TSubMat sub(const TSubMat &m, int top, int left, int height, int width)
{
    CL_ASSERT_MSG(left >= 0 && width > 0 && left + width <= m.Cols(),
        "(sub(SubMat)) illegal subset of matrix");
    CL_ASSERT_MSG(top >= 0 && height > 0 && top + height <= m.Rows(), 
        "(sub(SubMat)) illegal subset of matrix");

    TSubMat result(height, width, m.span, m.data + top * m.span + left);

    return(result);
}

TSubMat sub(const TSubMat &m, int nrows, int ncols)
{
    CL_ASSERT_MSG(ncols > 0 && nrows > 0 && nrows <= m.Rows() && ncols <= m.Cols(), 
        "(sub(SubMat)) illegal subset of matrix");

    TSubMat result(nrows, ncols, m.span, m.data);

    return(result);
}

TMSubVec col(const TSubMat &m, int i)
{
    CL_RANGE_MSG(i, 0, m.Cols(), "(col(SubMat)) illegal column index");

    return(TMSubVec(m.rows, m.span, m.data + i));
}

TMSubVec row(const TSubMat &m, int i)
{
    CL_RANGE_MSG(i, 0, m.Rows(), "(row(SubMat)) illegal row index");

    return(TMSubVec(m.cols, 1, m.data + i * m.span));
}

TMSubVec diag(const TSubMat &m, int diagNum)
{
    CL_RANGE_MSG(diagNum, 1 - m.Rows(), m.Cols(), "(row(Mat)) illegal row index");

    if (diagNum == 0)
        return(TMSubVec(std::min<int>(m.rows, m.cols), m.span + 1, m.data));
    else if (diagNum < 0)
        return(TMSubVec(std::min<int>(m.rows + diagNum, m.cols), m.span + 1,
            m.data - diagNum * m.span));
    else
        return(TMSubVec(std::min<int>(m.cols - diagNum, m.rows), m.span + 1,
            m.data + diagNum));
}
