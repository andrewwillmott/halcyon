/*
    File:           VLSubSVec.h

    Function:       Defines a scatter-gather sparse vector, i.e., a subvector
                    of another vector or the row, column or diagonal of a
                    sparse matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_SubSVec_H
#define VL_SubSVec_H

#include "VL.h"


class TSparseVec;

class TSubSVec
{
public:
                    TSubSVec(int start, int length, TSparseVec *target,
                             int span = 1);
                    TSubSVec(const TSubSVec &v);
    
    inline int      Elts() const { return(elts); };

    TSubSVec        &operator = (const TSubSVec &v);
    TSubSVec        &operator = (const TSparseVec &v);
    void            Store(TSparseVec &sv) const;

protected: 
    
    int             elts;       // # of elements
    int             start;      // start element in the target vector
    int             colSpan;    // columns between elements
    int             rowSpan;    // rows between elements
    TSparseVec      *target;    // the sparse vector this is a subvector of
};

// --- Sub-vector operator ----------------------------------------------------
    
TSubSVec    sub(const TSparseVec &v, int start, int length); 

#endif
