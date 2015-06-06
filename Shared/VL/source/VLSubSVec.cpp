/*
    File:           VLSubSVec.cpp

    Function:       Implements VLSubSVec.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          
                    
*/


#include "VLSubSVec.h"
#include "VLSparseVec.h"


// --- Vector Memory Management -----------------------------------------------


TSubSVec::TSubSVec(int start, int length, TSparseVec *target, int span) :
    elts(length), 
    start(start), 
    colSpan(),
    rowSpan(),
    target(target)
{
    if (span == 1)
    {
        colSpan = 1;
        rowSpan = 0;
    }       
    else
    {
        rowSpan = span / target->Elts();
        colSpan = span % target->Elts();
    }
}

TSubSVec::TSubSVec(const TSubSVec &v) : 
    elts(v.elts), 
    start(v.start),
    colSpan(v.colSpan), 
    rowSpan(v.rowSpan), 
    target(v.target)
{
}

TSubSVec &TSubSVec::operator = (const TSparseVec &v)
{
    if (rowSpan)
    {
        TSVIter j;
        
        for (j.Begin(v); !j.AtEnd(); j.Inc())
            target[j.Index() * rowSpan]
                .Set(start + j.Index() * colSpan, j.Data());
    }
    else
    {
        TSVIter     j(target[0]);

        int srcSize = v.pairs.NumItems() - 1;

        // find indices of the first & last elements within
        // the subrange of the target vector                
        j.IncTo(start);
        int cstart = j.PairIdx();
        j.IncTo(start + colSpan * v.Elts());
        int dstSize = j.PairIdx() - cstart;

        // resize the hole in the index/elt array

        if (srcSize > dstSize)
            target->pairs.Insert(cstart, srcSize - dstSize);
        else if (dstSize > srcSize)
            target->pairs.Delete(cstart, dstSize - srcSize);

        // copy over the source vector pairs        
        for (int i = 0, k = cstart; i < srcSize; i++, k++)
        {
            target->pairs[k].elt = v.pairs[i].elt;
            target->pairs[k].index = v.pairs[i].index + start;
        }
    }
    
    return(*this);
}

TSubSVec &TSubSVec::operator = (const TSubSVec &v)
{
    // for the moment, use a cast... inefficient! XXX
    
    return(*this = TSparseVec(v));
}

void TSubSVec::Store(TSparseVec &sv) const
{
    sv.SetNumElts(Elts());
    sv.Begin();
    
    if (rowSpan)
    {
        int     i, ccol = start;
        
        for (i = 0; i < Elts(); i++)
        {
            // We only want one element from each SparseVec, so
            // use Get()...
            sv.AddElt(i, target[i].Get(ccol));
            ccol += colSpan;
        }
    }
    else
    {
        int     endIndex = start + elts * colSpan;
        TSVIter j(target[0]);

        for (j.IncTo(start); j.PairIdx() < endIndex; j.Inc())
            sv.AddNZElt(j.Index() - start, j.Data());
    }
    
    sv.End();
}

// --- Sub-vector function ----------------------------------------------------

TSubSVec sub(const TSparseVec &v, int start, int length)
{
    CL_ASSERT_MSG(start >= 0 && length > 0 && start + length <= v.Elts(),
        "(sub(SparseVec)) illegal subset of vector");

    return(TSubSVec(start, length, (TSparseVec*) &v));
}
