/*
    File:           VLSubSMat.h

    Function:       Defines a scatter-gather sparse matrix, i.e., a submatrix of another sparse 
                    matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VL_SubSMat_H
#define VL_SubSMat_H

#include "VL.h"
#include "VLMat.h"


// --- SubSMat Class ----------------------------------------------------------


class TSubSVec;
class TSparseVec;
class TSparseMat;

class TSubSMat
{
public:
    
    // Constructors
    TSubSMat(int m, int n, int start, TMSparseVec *target);
    TSubSMat(const TSubSMat &m);            
    
    // Accessor functions
    int         Rows() const { return rows; };
    int         Cols() const { return cols; };

    TMSubSVec   operator [] (int i) const;        

    // Assignment operators
    TSubSMat    &operator = (const TSubSMat &m);
    TSubSMat    &operator = (const TSparseMat &m);  
    TSubSMat    &operator = (const TMat &m);    

protected:
    // Data
    int                     rows;
    int                     cols;
    int                     start;
    TMSparseVec             *target; 
};

TMSubSVec   col(const TSparseMat &m, int i);
TMSubSVec   diag(const TSparseMat &m, int diagNum); //  -i = diag. starting on row i, +i = diag. starting on col i
TSubSMat    sub(const TSparseMat &m, int top, int left, int nrows, int ncols);
TSubSMat    sub(const TSparseMat &m, int nrows, int ncols);


// --- SubSMat Inlines --------------------------------------------------------

#include "VLSubSVec.h"
#include "VLSparseVec.h"

inline TMSubSVec TSubSMat::operator [] (int i) const
{
    CL_ASSERT_MSG(i >= 0 && i < Rows(), "(SubSMat::[]) index out of range");
    return(sub(target[i], start, Cols()));
}

#endif

