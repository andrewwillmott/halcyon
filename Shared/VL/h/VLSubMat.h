/*
    File:           VLSubMat.h

    Function:       Defines a scatter-gather matrix, i.e., a submatrix of another matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_SubMat_H
#define VL_SubMat_H

#include "VL.h"
#include "VLSubVec.h"

// --- SubMat Class -----------------------------------------------------------

class TMat;
class TVec;

class TSubMat
{
public:
    
    // Constructors
    
                        TSubMat(int m, int n, int span, TMReal data[]);
                        TSubMat(const TSubMat &m);          
    
    // Accessor functions
    
    int                 Rows() const;
    int                 Cols() const;

    inline TMVec        operator [] (int i);
    inline const TMVec  operator [] (int i) const;

    TMReal&             operator () (int i, int j);
    const TMReal&       operator () (int i, int j) const;

    // Assignment operators
    
    TSubMat             &operator = (const TSubMat &m); // Assignment of a matrix
    TSubMat             &operator = (const TMat &m);    

//protected:
    int                 rows;
    int                 cols;
    int                 span;
    TMReal*             data;
};


// --- Submatrix functions ----------------------------------------------------
    
TSubMat     sub(const TMat &m, int top, int left, int height, int width);
TSubMat     sub(const TMat &m, int rows, int cols);
TMSubVec    row(const TMat &m, int i);
TMSubVec    col(const TMat &m, int i);
TMSubVec    diag(const TMat &m, int diagNum = 0);
//  -i = diag. starting on row i, +i = diag. starting on col i

TSubMat     sub(const TSubMat &m, int top, int left, int height, int width);
TSubMat     sub(const TSubMat &m, int rows, int cols);
TMSubVec    row(const TSubMat &m, int i);
TMSubVec    col(const TSubMat &m, int i);
TMSubVec    diag(const TSubMat &m, int diagNum = 0);

// --- SubMat Inlines ---------------------------------------------------------


#include "VLVec.h"

inline int TSubMat::Rows() const
{
    return rows;
};

inline int TSubMat::Cols() const
{
    return cols;
};

inline TMVec TSubMat::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, Rows(), "(SubMat::(i)) index out of range");
    return TMVec(cols, data + i * span);
}

inline const TMVec TSubMat::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, Rows(), "(SubMat::(i)) index out of range");
    return TMVec(cols, data + i * span);
}

inline TMReal& TSubMat::operator() (int i, int j)
{
    CL_RANGE_MSG(i, 0, Rows(), "(SubMat::(i,j)) i index out of range");
    CL_RANGE_MSG(j, 0, Cols(), "(SubMat::(i,j)) j index out of range");
    return data[i * span + j];
}

inline const TMReal& TSubMat::operator() (int i, int j) const
{
    CL_RANGE_MSG(i, 0, Rows(), "(SubMat::(i,j)) i index out of range");
    CL_RANGE_MSG(j, 0, Cols(), "(SubMat::(i,j)) j index out of range");
    return data[i * span + j];
}

#endif

