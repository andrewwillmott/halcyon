/*
    File:           VLSparseVec.h

    Function:       Defines a sparse vector.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_SparseVec_H
#define VL_SparseVec_H

#include "VL.h"
#include "VLVec.h"
#include "VLSubSVec.h"

#include <vector>

// --- SparseVec Class --------------------------------------------------------

#define TSparsePairs VL_V_SUFF(Pairs)

class TSparsePair
{
public: 
    TSparsePair() : index(VL_SV_MAX_INDEX), elt(0) {}
    TSparsePair(int i, TVReal elt) : index(i), elt(elt) {}

    int     index;      // Index
    TVReal  elt;        // Non-zero element at that index
};

struct TSparsePairs
{
    typedef TSparsePair tItem;

    TSparsePairs() : mItems(0), mNumItems(0), mAllocatedItems(0) {}
    TSparsePairs(const TSparsePairs& pairs);
    ~TSparsePairs();

    const tItem& operator[](int i) const;
          tItem& operator[](int i);

    TSparsePairs& operator=(const TSparsePairs& pairs);

    void Clear();
    void Append(const tItem& v);
    int  NumItems() const;

    void Insert(int i, int n);
    void Delete(int i, int n);

    void Swap(TSparsePairs& pairs);

    void Grow(int n);

    tItem*       mItems;
    int          mNumItems;
    int          mAllocatedItems;
};

class TSubSVec;
class TSVIter;

class TSparseVec
/*
    Function:   Provides a sparse vector class

    NOTE: SparseVecs can be manipulated as follows:
    
    1] Use SetElts() -- e.g., 
        sv.SetSize(5); sv.SetElts(1, 1.0, 4, 4.0, VL_SV_END);
        Sets sv to [0.0, 1.0, 0.0, 0.0 4.0]
    
    2] Use the SVIter iterator. (See description below.)
            
    3] Use the Overlay method: a.Overlay(b) performs a[i] = b[i] for all non-zero b[i].

    4] Direct access:
        SetNumElts(size), Begin(), AddElt()/AddNZElt() new element pairs in
        order, then End(). (Use AddNZElt if you know the element will 
        be non-zero.)
    
    5] As a last resort, use Get()/Set(). 
        These calls are not efficient for multiple accesses. 
*/
{
public:
    TSparseVec();                       // Null vector: space allocated later
    TSparseVec(int n);                                      
    TSparseVec(int n, int indices[], TVReal elts[]);    
    TSparseVec(int n, int idx0, double elt0, ...);              
    TSparseVec(const TSparseVec &v);    // Copy constructor
    TSparseVec(const TSubSVec &v);
    explicit TSparseVec(const TVec &v);
    TSparseVec(int n, ZeroOrOne);       // Zero or all-ones vector  
    TSparseVec(int n, Axis a);          // Unit vector
    
   ~TSparseVec();                       // Destructor

    // Accessor functions
    int         Elts() const { return numElts; }

    TSparseVec  &operator =  (const TSparseVec &v);
    TSparseVec  &operator =  (ZeroOrOne k) { MakeBlock(TVReal(k)); return(*this); }                      
    TSparseVec  &operator =  (Axis k)      { MakeUnit(k); return(*this); }                        
    TSparseVec  &operator =  (const TSubSVec &v);
    TSparseVec  &operator =  (const TVec &v);
    
    void        SetSize(int n);

    //  Sparse methods
    TSparseVec  Overlay(const TSparseVec &a) const;
    void        SetElts(int idx0, double elt0, ...);
    void        Set(int index, TVReal elt);
    TVReal      Get(int index) const;
    
    //  Vector initialisers
    void        MakeZero();
    void        MakeUnit(int i, TVReal k = vl_one);
    void        MakeBlock(TVReal k = vl_one);

    //  Low level Utils
    void        SetNumElts(int n)               { numElts = n; }
    void        Begin()                         { pairs.Clear(); }
    void        AddElt(int i, TVReal a)         { if (len(a) > sFuzz) { pairs.Append(TSparsePair(i, a)); } }
    void        AddNZElt(int i, TVReal a)       { pairs.Append(TSparsePair(i, a)); }
    void        AddNZElt(const TSparsePair &p)  { pairs.Append(p); }
    void        End()                           { pairs.Append(TSparsePair(VL_SV_MAX_INDEX, 0)); }

    void        Swap(TSparseVec& v)             { pairs.Swap(v.pairs); }

    //  Settings
    static void SetCompactPrint(bool on);
    static bool IsCompact() { return sCompactPrint; }
    static void SetFuzz(TVReal sFuzz);
    static bool IsNonZero(TVReal a)             { return(len(a) > sFuzz); }

    friend class TSVIter;
    friend TSparseVec &operator *= (TSparseVec &v, TVReal s);
    friend TSparseVec &operator /= (TSparseVec &v, TVReal s);
    friend bool operator == (const TSparseVec &a, const TSparseVec &b);
    friend bool operator != (const TSparseVec &a, const TSparseVec &b);
    friend TSparseVec operator + (const TSparseVec &a, const TSparseVec &b);
    friend TSparseVec operator - (const TSparseVec &a, const TSparseVec &b);
    friend TSparseVec operator * (const TSparseVec &a, const TSparseVec &b);
    friend TSparseVec operator / (const TSparseVec &a, const TSparseVec &b);
    friend TSparseVec operator - (const TSparseVec &v);
        
    int             numElts;
    TSparsePairs    pairs;

    static bool     sCompactPrint;  //   Print in normal or compact (only non-zero elts) style
    static Real     sFuzz;          //   x s.t. |x| <= sFuzz is considered zero.
};
    
class TSVIter
/*
    Function:   Useful for iterating over a sparse vector.
    
    Data() :    returns the current element's data
    Index() :   returns the current element's index

    1] Use Begin(), Inc(), AtEnd() to iterate over the non-zero elements 
        of the vector:
        
        // sv = sv * 2
        SVIter[fd]    j;
        SparseVec[fd] sv;
        for (j.Begin(sv); !j.AtEnd(); j.Inc())
            j.Data() *= 2.0;        

    2]  Use one of the following:

        Inc(int i)    moves on to elt i, where i will increase by 1 on each call
        IncTo(int i)  moves on to elt i, where i will increase monotonically

        within another for-loop to access the elements of the sparse vector
        corresponding to i. For example:

        // v = v + sv
        for (j.Begin(sv), i = 0; i < v.NumItems(); i++)
        {
            j.Inc(i);
            if (j.Exists())
                v[i] += j.Data();
        }

        // a += dot(sv1, sv2)
        for (j.Begin(sv2), k.Begin(sv1); !k.AtEnd(); k.Inc())
        {
            j.IncTo(k.Index());     // find corresponding elt in sv2
            if (j.Exists())
                a += j.Data() * k.Data();
        }
*/
{
public:
    TSVIter() {}
    TSVIter(const TSparseVec &sv) : pairIndex(0), list(&sv.pairs) {}

    void    Begin()
            { pairIndex = 0; }
            // move to the beginning of the current sparse vector
            
    void    Begin(const TSparseVec &sv)
            { pairIndex = 0; list = &sv.pairs; }
            // move to the beginning of sparse vector sv

    void    Inc()
            { pairIndex++; }
            // move to the next non-zero element in the vector
            
    bool    AtEnd()
            { return(pairIndex + 1 >= (*list).NumItems()); }
            // returns true if we're at the end of the vector

    TVReal  Data()
            { return((*list)[pairIndex].elt); }
            // WARNING: only call this if you *know* the element is non-zero
    int     Index()
            { return((*list)[pairIndex].index); }

    TSparsePair Pair()
            { return((*list)[pairIndex]); }

    void    Inc(int i)
            { if (i > (*list)[pairIndex].index) pairIndex++; }
            // Move on to the element indicated by i. i must increase by
            // 1 on each subsequent call.

    bool    IncTo(int i)
            { OrdFindBinary(i); return(i == (*list)[pairIndex].index); }
            // Move on to the element with index i.
            // returns true if element i is non-zero.
    
    bool    Exists(int i)
            { return(i == (*list)[pairIndex].index); }

    TVReal  Elt(int i)
            { return(Exists(i) ? Data() : TVReal(vl_zero)); }
            // call this if you don't want to bother with Exists()/Data()

    int     PairIdx()
            { return(pairIndex); }
            //  the index of the pair containing the first non-zero element
            //  occuring at or after the element index set by
            //  the last Inc()/IncTo() call.
            
    void    OrdFindBinary(int i);
            // Find the pair with index i using binary search
    void    OrdFindLinear(int i);   
            // Find the pair with index i using linear search

protected:
    int                 pairIndex;   // current index into sparse list
    const TSparsePairs* list;        // sparse list
};


// --- SparseVec In-Place operators -------------------------------------------

TSparseVec      &operator += (TSparseVec &a, const TSparseVec &b);
TSparseVec      &operator -= (TSparseVec &a, const TSparseVec &b);
TSparseVec      &operator *= (TSparseVec &a, const TSparseVec &b);
TSparseVec      &operator *= (TSparseVec &v, TVReal s);
TSparseVec      &operator /= (TSparseVec &a, const TSparseVec &b);
TSparseVec      &operator /= (TSparseVec &v, TVReal s);

// --- SparseVec Comparison Operators -----------------------------------------

bool            operator == (const TSparseVec &a, const TSparseVec &b);
bool            operator != (const TSparseVec &a, const TSparseVec &b);

// --- SparseVec Arithmetic Operators -----------------------------------------

TSparseVec      operator + (const TSparseVec &a, const TSparseVec &b);
TSparseVec      operator - (const TSparseVec &a, const TSparseVec &b);
TSparseVec      operator - (const TSparseVec &v);
TSparseVec      operator * (const TSparseVec &a, const TSparseVec &b);      
TSparseVec      operator * (const TSparseVec &v, TVReal s);
TSparseVec      operator / (const TSparseVec &a, const TSparseVec &b);
TSparseVec      operator / (const TSparseVec &v, TVReal s);
TSparseVec      operator * (TVReal s, const TSparseVec &v);


TVReal          dot(const TSparseVec &a, const TSparseVec &b);
TVReal          dot(const TSparseVec &a, const TVec &b);
TVReal          len(const TSparseVec &v);
TVReal          sqrlen(const TSparseVec &v);
TSparseVec      norm(const TSparseVec &v);
TSparseVec      norm_safe(const TSparseVec &v);


// --- SparseVec Functions ----------------------------------------------------

void Add     (const TSparseVec &a, const TSparseVec &b, TSparseVec &result);
void Subtract(const TSparseVec &a, const TSparseVec &b, TSparseVec &result);
void Negate  (const TSparseVec &v, TSparseVec &result);
void Negate  (TSparseVec &v);
void Multiply(const TSparseVec &a, const TSparseVec &b, TSparseVec &result);
void Multiply(const TSparseVec &v, TVReal s, TSparseVec &result);
void MultiplyAccum(const TSparseVec &v, TVReal s, TVec &result);
void MultiplyAccum(const TSparseVec &v, TVReal s, TSparseVec &result);
void Divide  (const TSparseVec &a, const TSparseVec &b, TSparseVec &result);
void Divide  (const TSparseVec &v, TVReal s, TSparseVec &result);

void Normalize(const TSparseVec &v, TSparseVec &result);
void NormalizeSafe(const TSparseVec &v, TSparseVec &result);
void Normalize(TSparseVec &v);
void NormalizeSafe(TSparseVec &v);


// --- Inlines -----------------------------------------------------------------

inline const TSparsePairs::tItem& TSparsePairs::operator[](int i) const
{
	CL_RANGE(i, 0, mNumItems);
	return(mItems[i]);
}

inline TSparsePairs::tItem& TSparsePairs::operator[](int i)
{
	CL_RANGE(i, 0, mNumItems);
	return(mItems[i]);
}

inline int TSparsePairs::NumItems() const
{
    return mNumItems;
}

inline void TSparsePairs::Append(const tItem &t)
{
	if (mNumItems >= mAllocatedItems)
		Grow(1);

	mItems[mNumItems++] = t;
}

inline bool operator != (const TSparseVec &a, const TSparseVec &b)
{
	return(!(a == b));
}


#endif
