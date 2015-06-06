/*
    File:           VLSparseVec.cpp

    Function:       Implements VLSparseVec.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLSparseVec.h"
#include "VLSubSVec.h"

#include <stdarg.h>

const int kFirstAllocation = 16;


// --- TSparsePairs ------------------------------------------------------------

TSparsePairs::TSparsePairs(const TSparsePairs& pairs) :
    mItems(pairs.mItems),
    mNumItems(pairs.mNumItems),
    mAllocatedItems(pairs.mAllocatedItems)
{
    if (mAllocatedItems)
        mItems = new tItem[mAllocatedItems];

	for (int i = 0; i < mNumItems; i++)
		mItems[i] = pairs.mItems[i];
}

TSparsePairs::~TSparsePairs()
{
	if (mAllocatedItems)
        delete[] mItems;
}

TSparsePairs& TSparsePairs::operator=(const TSparsePairs& pairs)
{
	if (mAllocatedItems < pairs.mNumItems)
	{
		if (mAllocatedItems)
            delete[] mItems;
		mAllocatedItems = pairs.mAllocatedItems;	
		mItems = new tItem[mAllocatedItems];
	}

    mNumItems = pairs.mNumItems;
	for (int i = 0; i < mNumItems; i++)
		mItems[i] = pairs.mItems[i];

	return(*this);
}

inline void TSparsePairs::Clear()
{
    mNumItems = 0;
    mAllocatedItems = 0;
    delete[] mItems;
    mItems = 0;
}

void TSparsePairs::Grow(int n)
{
	int oldAllocated = mAllocatedItems;

	if (mAllocatedItems == 0)
		mAllocatedItems = kFirstAllocation;
	else
		mAllocatedItems *= 2;

    while (mAllocatedItems < n + mNumItems)
		mAllocatedItems *= 2;

	tItem* newItems = new tItem[mAllocatedItems];

	for (int i = 0; i < mNumItems; i++)
		newItems[i] = mItems[i];
	
	if (oldAllocated)
        delete[] mItems;

	mItems = newItems;
}

namespace
{
    template<class T> void Swap2(T& a, T& b)
    {
        T t(a);
        a = b;
        b = t;
    }
}

void TSparsePairs::Swap(TSparsePairs& b)
{
    Swap2(mItems,          b.mItems);
    Swap2(mNumItems,       b.mNumItems);
    Swap2(mAllocatedItems, b.mAllocatedItems);
}

void TSparsePairs::Insert(int i, int n)
//	Make space at position i for n items.
{
	CL_RANGE_MSG(i, 0, mNumItems, "(Array:Insert) Illegal index");
	CL_ASSERT_MSG(n > 0, "(Array:Insert) Illegal insert amount");

    if (n + mNumItems > mAllocatedItems)
        Grow(n);

    mNumItems += n;
	
	for (int j = mNumItems - 1; j >= i + n; j--)
		mItems[j] = (mItems - n)[j];
}

void TSparsePairs::Delete(int i, int n)
//	Delete n items at position i.
{
	CL_RANGE_MSG(i, 0, mNumItems, "(Array:Delete) Illegal index");
	CL_RANGE_MSG(n, 1, mNumItems + 1, "(Array:Delete) Illegal insert amount");

	mNumItems -= n;
		
	for (int j = i; j < mNumItems; j++)
		mItems[j] = (mItems + n)[j];
}



// --- TSparseVec --------------------------------------------------------------

TSparseVec::TSparseVec() : numElts(0), pairs()
{
    End();
}

TSparseVec::TSparseVec(int n) : numElts(n), pairs()
{
    CL_ASSERT_MSG(n > 0,"(SparseVec) illegal vector size");
    End();
}

TSparseVec::TSparseVec(const TSparseVec &v) : numElts(v.numElts), pairs(v.pairs)
{
}

TSparseVec::TSparseVec(const TSubSVec &v)
{
    *this = v;
}

TSparseVec::TSparseVec(const TVec &v)
{
    *this = v;
}

TSparseVec::TSparseVec(int n, int indices[], TVReal elts[]) : numElts(n), pairs()
{
    CL_ASSERT_MSG(n > 0,"(SparseVec) illegal vector size");
    int i = 0;
    
    while (1)
    {
        if (indices[i] < 0)
            break;
        AddElt(indices[i], elts[i]);
        i++;
    }
    
    End();
}

TSparseVec::TSparseVec(int n, int idx0, double elt0, ...) : numElts(n), pairs()
{
    CL_ASSERT_MSG(n > 0,"(SparseVec) illegal vector size");
    va_list     ap;
    int         idx;
    TVReal      elt;
    
    va_start(ap, elt0);
    CL_ASSERT_MSG(idx0 >= 0 && idx0 < numElts, "(SparseVec) illegal first index");
    SetReal(elt, elt0);
    AddElt(idx0, elt);
        
    while (1)
    {
        idx = va_arg(ap, int);
        if (idx < 0)
            break;
        CL_ASSERT_MSG(idx < numElts, "(SparseVec) illegal index");
        SetReal(elt, va_arg(ap, double));
        AddElt(idx, elt);
    }
    
    End();
    va_end(ap);
}

TSparseVec::TSparseVec(int n, ZeroOrOne k) : numElts(n), pairs()
{
    CL_ASSERT_MSG(n > 0,"(SparseVec) illegal vector size");
    MakeBlock(TVReal(k));
}

TSparseVec::TSparseVec(int n, Axis a) : numElts(n), pairs()
{
    CL_ASSERT_MSG(n > 0,"(SparseVec) illegal vector size");
    MakeUnit(a);
}


TSparseVec::~TSparseVec()
{
}

TSparseVec &TSparseVec::operator = (const TSparseVec &v)
{
    numElts  = v.numElts;
    pairs = v.pairs;
    
    return(*this);
}

TSparseVec &TSparseVec::operator = (const TVec &v)
{
    int i;

    SetNumElts(v.Elts());
    Begin();
    
    for (i = 0; i < v.Elts(); i++)  
        AddElt(i, v[i]);
    
    End();
    
    return(*this);
}

TSparseVec &TSparseVec::operator = (const TSubSVec &v)
{
    v.Store(*this);
    return(*this);
}

void TSparseVec::SetSize(int n)
{
    numElts = n;
    // Begin();    
    // End();
}

void TSparseVec::MakeZero()
{
    Begin();
    End();
}

void TSparseVec::MakeUnit(int i, TVReal k)
{
    Begin();
    AddElt(i, k);
    End();
}

void TSparseVec::MakeBlock(TVReal k)
{
    if (len(k) == 0.0)
        MakeZero();
    else
        CL_WARNING("(SparseVec::MakeBlock) Inefficient use of sparse vector");
}


bool TSparseVec::sCompactPrint = false;
Real TSparseVec::sFuzz = Real(1e-10);

void TSparseVec::SetCompactPrint(bool on)
{ sCompactPrint = on; }

void TSparseVec::SetFuzz(TVReal theFuzz)
{ sFuzz = Real(theFuzz); }


// --- Vector operations ------------------------------------------------------


#include <ctype.h>

/*  [Note]

    When operating on sparse vectors in place, it is faster to create a 
    new vector on the fly, and swap it in at the end, than to perform
    inserts/deletes on the original. (O(n) vs O(n^2).)
    
    Supporting Analysis:
    
    There will be O(n) new/deleted elts. 
    Copying will involve O(n) copies in creating the new array, plus 
    a temporary memory overhead the size of the old vector. Operating
    in place will require O(n) insert/deletes, each of which require
    O(n) copies, and thus requires O(n^2) copies overall.
*/

TSparseVec &operator += (TSparseVec &a, const TSparseVec &b)
{
    TSparseVec t;
    Add(a, b, t);
    a.Swap(t);
    return(a);
}

TSparseVec &operator -= (TSparseVec &a, const TSparseVec &b)
{
    TSparseVec t;
    Subtract(a, b, t);
    a.Swap(t);
    return(a);
}

TSparseVec &operator *= (TSparseVec &a, const TSparseVec &b)
{
    TSparseVec t;
    Multiply(a, b, t);
    a.Swap(t);
    return(a);
}

TSparseVec &operator *= (TSparseVec &v, TVReal s)
{           
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        v.pairs[i].elt *= s;

    return(v);
}

TSparseVec &operator /= (TSparseVec &a, const TSparseVec &b)
{
    TSparseVec t;
    Multiply(a, b, t);
    a.Swap(t);
    return(a);
}

TSparseVec &operator /= (TSparseVec &v, TVReal s)
{
    TVReal t = vl_1 / s;
            
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        v.pairs[i].elt *= t;

    return(v);
}


// --- Vec Comparison Operators -----------------------------------------------


bool operator == (const TSparseVec &a, const TSparseVec &b)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::==) Vec sizes don't match");

    for (int i = 0; i < a.pairs.NumItems() - 1; i++)
    {
        if (a.pairs[i].index != b.pairs[i].index)
            return(false);

        if (a.pairs[i].elt   != b.pairs[i].elt)
            return(false);
    }

    return(true);
}


// --- SparseVec Methods ------------------------------------------------------


TSparseVec TSparseVec::Overlay(const TSparseVec &b) const
{
    TSparseVec result(Elts());
    
    result.Begin();
    
    for (int i = 0, j = 0; ; )
        if (pairs[i].index == b.pairs[j].index)
        {
            if (pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            result.AddNZElt(b.pairs[j]);
            i++;
            j++;
        }
        else if (pairs[i].index < b.pairs[j].index)
        { result.AddNZElt(pairs[i]); i++; }
        else
        { result.AddNZElt(b.pairs[j]); j++; }
            
    result.End();   
    return(result);
}

void TSparseVec::SetElts(int idx0, double elt0, ...)
{
    va_list     ap;
    int         idx;
    TVReal      elt;
    
    va_start(ap, elt0);
    Begin();
    CL_ASSERT_MSG(idx0 >= 0 && idx0 < numElts, "(SparseVec::SetElts) illegal first index");
    SetReal(elt, elt0);
    AddElt(idx0, elt);
        
    while (1)
    {
        idx = va_arg(ap, int);
        if (idx < 0)
            break;
        CL_ASSERT_MSG(idx < numElts, "(SparseVec::SetElts) illegal index");
        SetReal(elt, va_arg(ap, double));
        AddElt(idx, elt);
    }
    
    End();
    va_end(ap);
}

void TSparseVec::Set(int index, TVReal elt)
{
    TSVIter j(*this);
    
    if (len(elt) <= sFuzz)
        return;
                    
    if (!j.IncTo(index))
    {
        pairs.Insert(j.PairIdx(), 1);
        pairs[j.PairIdx()].index = index;
    }

    pairs[j.PairIdx()].elt = elt;
}

TVReal TSparseVec::Get(int index) const
{
    TSVIter j(*this);
    
    if (j.IncTo(index))
        return(j.Data());
    else
        return(vl_zero);
}

// --- Vec Arithmetic Operators -----------------------------------------------

TSparseVec operator + (const TSparseVec &a, const TSparseVec &b)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::+) Vec sizes don't match");

    TSparseVec result(a.Elts());

    result.Begin();
    
    // Step through a and b in parallel
    for (int i = 0, j = 0; ; )
        if (a.pairs[i].index == b.pairs[j].index)
        {
            // We have two elements at the same index. 
            // Are we at the end of both arrays?
            if (a.pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            // If not, add the result
            
            result.AddElt(a.pairs[i].index, a.pairs[i].elt + b.pairs[j].elt);   // +
            i++;
            j++;
        }
        else if (a.pairs[i].index < b.pairs[j].index)
        // result[x] = a.pairs[i] + 0       
        {   result.AddNZElt(a.pairs[i]); i++;   }
        else
        // result[x] = b.pairs[j] + 0       
        {   result.AddNZElt(b.pairs[j]); j++;   }
            
    result.End();   
    return(result);
}

TSparseVec operator - (const TSparseVec &a, const TSparseVec &b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::-) Vec sizes don't match");

    TSparseVec  result(a.Elts());
    
    result.Begin();

    for (int i = 0, j = 0; ; )
    {
        if (a.pairs[i].index == b.pairs[j].index)
        {
            if (a.pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            result.AddElt(a.pairs[i].index, a.pairs[i].elt - b.pairs[j].elt); // -
            i++;
            j++;
        }
        else if (a.pairs[i].index < b.pairs[j].index)
        {
            result.AddNZElt(a.pairs[i]);
            i++;
        }
        else
        {
            result.AddNZElt(b.pairs[j].index, -b.pairs[j].elt);
            j++;
        }
    }
    
    result.End();   
    return(result);
}

TSparseVec operator - (const TSparseVec &v)
{
    TSparseVec result(v.Elts());
    
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddNZElt(v.pairs[i].index, -v.pairs[i].elt);
    
    result.End();   
    return(result);
}

TSparseVec operator * (const TSparseVec &a, const TSparseVec &b)            
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::*) Vec sizes don't match");

    TSparseVec  result(a.Elts());
    TSVIter     j(a);
    
    result.Begin();
    
    for (int i = 0, n = int(b.pairs.NumItems()) - 1; i < n; i++)
        if (j.IncTo(b.pairs[i].index))
            result.AddElt(b.pairs[i].index, j.Data() * b.pairs[i].elt);

    result.End();   
    return(result);
}

TSparseVec operator * (const TSparseVec &v, TVReal s) 
{
    TSparseVec result(v.Elts());
        
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, s * v.pairs[i].elt);

    result.End();   
    return(result);
}

TSparseVec operator / (const TSparseVec &a, const TSparseVec &b)            
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::/) Vec sizes don't match");
    
    TSparseVec  result(a.Elts());
    TSVIter     j(a);
    
    result.Begin();
    
    for (int i = 0, n = int(b.pairs.NumItems()) - 1; i < n; i++)
        if (j.IncTo(b.pairs[i].index))
            result.AddElt(b.pairs[i].index, j.Data() / b.pairs[i].elt);
    
    result.End();
    return(result);
}

TSparseVec operator / (const TSparseVec &v, TVReal s) 
{
    TSparseVec  result(v.Elts());
    TVReal      t = vl_1 / s;

    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, v.pairs[i].elt * t);
    
    result.End();   
    return(result);
}

TSparseVec operator * (TVReal s, const TSparseVec &v)
{
    return(v * s);
}

TVReal dot(const TSparseVec &a, const TSparseVec &b) 
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::dot) Vec sizes don't match");

    TMReal      sum = vl_zero;
    TSVIter     j(a);
    
    for (int i = 0, n = int(b.pairs.NumItems()) - 1; i < n; i++)
        if (j.IncTo(b.pairs[i].index))
            sum += j.Data() * b.pairs[i].elt;
    
    return(sum);
}

TVReal dot(const TSparseVec &a, const TVec &b)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::dot) Vec sizes don't match");

    TMReal      sum = vl_zero;
    
    for (int i = 0, n = (a.pairs.NumItems()) - 1; i < n; i++)
        sum += a.pairs[i].elt * b[a.pairs[i].index];
    
    return(sum);
}

TVReal sqrlen(const TSparseVec &v)
{
    TVReal      sum = vl_zero;
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        sum += sqrlen(v.pairs[i].elt);
    
    return(sum);
}

TVReal len(const TSparseVec &v)
{
    return(sqrt(sqrlen(v)));
}

TSparseVec norm(const TSparseVec &v)    
{
    return(v / len(v));
}

TSparseVec norm_safe(const TSparseVec &v)    
{
    return(v / (len(v) + TVReal(1e-8)));
}

// --- SparseVec Functions ----------------------------------------------------


void Add(const TSparseVec &a, const TSparseVec &b, TSparseVec &result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::+) Vec sizes don't match");

    result.SetSize(a.Elts());
    result.Begin();
    
    // Step through a and b in parallel
    for (int i = 0, j = 0; ; )
        if (a.pairs[i].index == b.pairs[j].index)
        {
            // We have two elements at the same index. 
            // Are we at the end of both arrays?
            if (a.pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            // If not, add the result
            
            result.AddElt(a.pairs[i].index, a.pairs[i].elt + b.pairs[j].elt);   // +
            i++;
            j++;
        }
        else if (a.pairs[i].index < b.pairs[j].index)
        // result[x] = a.pairs[i] + 0       
        {   result.AddNZElt(a.pairs[i]); i++;   }
        else
        // result[x] = b.pairs[j] + 0       
        {   result.AddNZElt(b.pairs[j]); j++;   }
            
    result.End();   
}

void Subtract(const TSparseVec &a, const TSparseVec &b, TSparseVec &result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::-) Vec sizes don't match");

    result.SetSize(a.Elts());

    result.Begin();
    
    for (int i = 0, j = 0; ; )
        if (a.pairs[i].index == b.pairs[j].index)
        {
            if (a.pairs[i].index == VL_SV_MAX_INDEX)
                break;
            
            result.AddElt(a.pairs[i].index, a.pairs[i].elt - b.pairs[j].elt); // -
            i++;
            j++;
        }
        else if (a.pairs[i].index < b.pairs[j].index)
        {   result.AddNZElt(a.pairs[i]); i++;   }
        else
        {   result.AddNZElt(b.pairs[j].index, -b.pairs[j].elt); j++;    }
    
    result.End();   
}

void Negate(const TSparseVec &v, TSparseVec &result)
{
    result.SetSize(v.Elts());
    
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddNZElt(v.pairs[i].index, -v.pairs[i].elt);
    
    result.End();   
}

void Negate(TSparseVec &v)
{
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        v.pairs[i].elt = -v.pairs[i].elt;
}

void Multiply(const TSparseVec &a, const TSparseVec &b, TSparseVec &result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::*) Vec sizes don't match");

    result.SetSize(a.Elts());
    TSVIter j(a);
    
    result.Begin();
    
    for (int i = 0, n = int(b.pairs.NumItems()) - 1; i < n; i++)
        if (j.IncTo(b.pairs[i].index))
            result.AddElt(b.pairs[i].index, j.Data() * b.pairs[i].elt);
    
    result.End();   
}

void Multiply(const TSparseVec &v, TVReal s, TSparseVec &result)
{
    result.SetSize(v.Elts());
        
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, s * v.pairs[i].elt);

    result.End();   
}

void MultiplyAccum(const TSparseVec &v, TVReal s, TVec &result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(SparseVec::MultiplyAccum) Vec sizes don't match");
            
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result[v.pairs[i].index] += s * v.pairs[i].elt;
}

void MultiplyAccum(const TSparseVec &v, TVReal s, TSparseVec &result)
{
    CL_ASSERT_MSG(v.Elts() == result.Elts(), "(SparseVec::MultiplyAccum) Vec sizes don't match");
    
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

void Divide(const TSparseVec &a, const TSparseVec &b, TSparseVec &result)
{
    CL_ASSERT_MSG(a.Elts() == b.Elts(), "(SparseVec::/) Vec sizes don't match");
    
    result.SetSize(a.Elts());
    TSVIter j(a);
    
    result.Begin();
    
    for (int i = 0, n = int(b.pairs.NumItems()) - 1; i < n; i++)
        if (j.IncTo(b.pairs[i].index))
            result.AddElt(b.pairs[i].index, j.Data() / b.pairs[i].elt);
    
    result.End();
}

void Divide(const TSparseVec &v, TVReal s, TSparseVec &result)
{
    result.SetSize(v.Elts());
    TVReal t = vl_1 / s;
            
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, v.pairs[i].elt * t);
    
    result.End();   
}

void Normalize(const TSparseVec &v, TSparseVec &result)
{
    result.SetSize(v.Elts());
    TVReal t = vl_1 / len(v);
            
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, v.pairs[i].elt * t);
    
    result.End();   
}

void NormalizeSafe(const TSparseVec &v, TSparseVec &result)
{
    result.SetSize(v.Elts());
    TVReal t = vl_1 / (len(v) + TVReal(1e-8));
            
    result.Begin();
    
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        result.AddElt(v.pairs[i].index, v.pairs[i].elt * t);
    
    result.End();   
}

void Normalize(TSparseVec &v)
{
    TVReal t = vl_1 / len(v);
            
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        v.pairs[i].elt *= t;
}

void NormalizeSafe(TSparseVec &v)
{
    TVReal t = vl_1 / len(v);
            
    for (int i = 0, n = v.pairs.NumItems() - 1; i < n; i++)
        v.pairs[i].elt *= t;
}


// --- SparseVec iterator -----------------------------------------------------


void TSVIter::OrdFindLinear(int i)
{
    // Linear search for the right pair
    while (!AtEnd() && (i > Index()))
        pairIndex++;
}

#define SV_MIXED_SEARCH

#ifndef VL_SV_CONST
#define VL_SV_CONST
const int kLinearSearchRange = 10;   //  Linear search over intervals smaller than this...
const int kSparsenessEst = 5;        // estimated elts per non-zero elt.
const int kLSRSparse = kSparsenessEst * kLinearSearchRange;
#endif

void TSVIter::OrdFindBinary(int i)
//  Move index to point to the pair that corresponds to i, or if no such pair exists,
//  the pair with the next index after i that does exist.
{
#ifdef SV_MIXED_SEARCH  
    // Mixture of linear & binary, parameterised by kLinearSearchRange.
    // If the item we're looking for is farther away from the current 
    // pair than kLSRSparse, we binary search.

    if ((i - Index()) > kLSRSparse)
    {
#endif

    // --- Binary search on the pairs list-------------------------------------
    
    // A really nice thing to do here would be to back out
    // hierarchically from the current index instead of just
    // giving up on it. Similar to storing the current octree
    // node in RT, and doing the same.

        int j = 0, k = (*list).NumItems() - 1, m;
        
        //  Test for trivial cases: i lies before or after the pairs list
        
        if (k < 0 || i <= (*list)[j].index)
        {   pairIndex = 0;     return; }
        if ((*list)[k].index < i)
        {   pairIndex = k + 1; return; }
        if ((*list)[k].index == i)
        {   pairIndex = k;     return; }
        
        while(k > j + 1 + kLinearSearchRange)
        {
            // precondition: j.index < i < k.index
            CL_ASSERT_MSG((*list)[j].index < i && i < (*list)[k].index, "Precondition failed.");
            
            // Naive midpoint picking
            m = (j + k) / 2;    // m ~ [j+1..k-1]

            // Linear midpoint picking
            // m = j + 1 +  ((k - j - 1) * (i - (*list)[j])) / (*list)[k] - (*list)[j]);
                        
            if (i > (*list)[m].index)
                j = m;
            else
            {
                k = m;  

                if (i >= (*list)[k].index)
                {   
                    pairIndex = k;
                    return; 
                }                   
            }
        }

        pairIndex = j + 1;

#ifdef SV_MIXED_SEARCH
    }

    while (i > Index())     // Linear search, assuming sentinel
        pairIndex++;
#endif
}   

