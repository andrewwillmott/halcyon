/*
    File:           VLVol.h

    Function:       Defines a generic resizeable stack of matrices.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 2013, Andrew Willmott
 */

#ifndef VL_Vol_H
#define VL_Vol_H

#include "VL.h"


// --- Vol Class --------------------------------------------------------------


struct TVol
{
    // Constructors
    TVol();
    TVol(int slices, int rows, int cols);
    TVol(int slices, int rows, int cols, TMReal* ndata); // Create reference volume
    TVol(const TVol& v);                                 // Copy constructor
    TVol(int slices, int rows, int cols, ZeroOrOne k);   // I * k
    TVol(int slices, int rows, int cols, Block k);       // block matrix (m[i][j][k] = k)

    ~TVol();

    // Accessor methods
    int     Slices() const;
    int     Rows  () const;
    int     Cols  () const;

    int     Elts() const;

#ifdef VL_Mat_H
    TMat    operator [] (int i);        // Indexing by slice
    TMat    operator [] (int i) const;  // Indexing by slice
#endif

    TMReal&       operator () (int i, int j, int k);
    const TMReal& operator () (int i, int j, int k) const;

    void SetSize(int slices, int rows, int cols);
    void SetSize(const TVol& v);

    TMReal* Ref() const;
    const TMReal* ConstRef() const;

    void Swap(TVol& v);

    // Assignment operators
    TVol&   operator =  (const TVol& v);   // Assignment of a volume
    TVol&   operator =  (ZeroOrOne k);     // Set to k * I...
    TVol&   operator =  (Block k);         // Set to a block matrix...

    // In-Place Operators
    TVol&   operator += (const TVol& v);
    TVol&   operator -= (const TVol& v);
    TVol&   operator *= (const TVol& v);
    TVol&   operator *= (TMReal s);
    TVol&   operator /= (TMReal s);

    // Initialisers
    void    MakeZero();
    void    MakeIdentity();
    void    MakeDiag(TMReal k);
    void    MakeBlock(TMReal k);

    void    MakeNull();             ///< Make this a zero-sized volume
    void    MakeRef(const TVol& v); ///< Make this a reference of 'v'

    bool    IsNull() const;         ///< Returns true if this is a null-sized volume.
    bool    IsRef() const;          ///< Returns true if this is a reference to storage elsewhere

    // Representation as vector
    TMVec       AsVec();
    const TMVec AsVec() const;

protected:
    TMReal*  mData;
    uint32_t mSlices;
    uint32_t mRows;
    uint32_t mCols;
};


// --- Vol Functions ----------------------------------------------------------

void Add     (const TVol& m, const TVol& n, TVol& result);
void Subtract(const TVol& m, const TVol& n, TVol& result);
void Negate  (const TVol& m, TVol& result);
//void Multiply(const TVol& m, const TVol& n, TVol& result);
void Multiply(const TVol& m, TMReal s, TVol& result);
void MultiplyAccum(const TVol& m, TMReal s, TVol& result);
void Divide  (const TVol& m, TMReal s, TVol& result);

//void Multiply(const TVol& m, const TMVec& v, TMVec& result);
//void Multiply(const TMVec& v, const TVol& m, TMVec& result);

void   Transpose(const TVol& m, TVol& result);
TMReal InnerProduct(const TVol& a, const TVol& b);
#ifdef VL_Mat_H
void   OuterProduct(const TMVec& a, const TMVec& b, TVol& result);
#endif
bool   Invert(const TVol& m, TVol& VL_RESTRICT result, TMReal* determinant = 0, TMReal pEps = TMReal(1e-20));


// --- Vol Inlines -------------------------------------------------------------

inline TVol::TVol() :
    mData(0),
    mSlices(0),
    mRows(0),
    mCols(0)
{
}

inline TVol::TVol(int slices, int rows, int cols) :
    mData(VL_NEW TMReal[slices * rows * cols]),
    mSlices(slices),
    mRows(rows),
    mCols(cols)
{
}

inline TVol::TVol(int slices, int rows, int cols, TMReal* data) :
    mData(data),
    mSlices(slices | VL_REF_FLAG),
    mRows(rows),
    mCols(cols)
{
}

inline TVol::~TVol()
{
    if (!IsRef())
        VL_DELETE[] mData;
}

inline int TVol::Slices() const
{
    return mSlices & VL_REF_MASK;
};

inline int TVol::Rows() const
{
    return mRows;
};

inline int TVol::Cols() const
{
    return mCols;
};

inline int TVol::Elts() const
{
    return mCols * mRows * (mSlices & VL_REF_MASK);
};

inline void TVol::SetSize(const TVol& v)
{
    SetSize(v.mSlices, v.mRows, v.mCols);
}

#ifdef VL_Mat_H
inline TMat TVol::operator [] (int i)
{
    CL_INDEX_MSG(i, Slices(), "(Vol::[i]) i index out of range");
    return TMat(mRows, mCols, mData + (mRows * mCols) * i);
}

// TODO: we have no const support in Matf references (or indeed Vecf). How
// do we fix this? Really need ConstMat or runtime flag.
inline TMat TVol::operator [] (int i) const
{
    CL_INDEX_MSG(i, Slices(), "(Vol::[i]) i index out of range");
    return TMat(mRows, mCols, mData + (mRows * mCols) * i);
}
#endif

inline TMReal& TVol::operator () (int i, int j, int k)
{
    CL_INDEX_MSG(i, mSlices, "(Vol::e(i,j,k)) i index out of range");
    CL_INDEX_MSG(j, mRows,   "(Vol::e(i,j,k)) j index out of range");
    CL_INDEX_MSG(k, mCols,   "(Vol::e(i,j,k)) k index out of range");

    return mData[(i * mRows + j) * mCols + k];
}

inline const TMReal& TVol::operator () (int i, int j, int k) const
{
    CL_INDEX_MSG(i, mSlices, "(Vol::e(i,j,k)) i index out of range");
    CL_INDEX_MSG(j, mRows,   "(Vol::e(i,j,k)) j index out of range");
    CL_INDEX_MSG(k, mCols,   "(Vol::e(i,j,k)) k index out of range");

    return mData[(i * mRows + j) * mCols + k];
}

inline TMReal* TVol::Ref() const
{
    return mData;
}

inline const TMReal* TVol::ConstRef() const
{
    return mData;
}

inline void TVol::Swap(TVol& v)
{
    TMReal*  data(mData);
    uint32_t slices(mSlices);
    uint32_t rows(mRows);
    uint32_t cols(mCols);

    mData   = v.mData;
    mSlices = v.mSlices;
    mRows   = v.mRows;
    mCols   = v.mCols;

    v.mData   = data;
    v.mSlices = slices;
    v.mRows   = rows;
    v.mCols   = cols;
}

inline TVol& TVol::operator = (ZeroOrOne k)
{
    MakeDiag(TMReal(k));
    
    return(*this);   
}

inline TVol& TVol::operator = (Block k)
{
    MakeBlock(TMReal(k));
    
    return(*this);   
}

inline TVol& TVol::operator += (const TVol& m)
{
    AsVec() += m.AsVec();
    return SELF;
}

inline TVol& TVol::operator -= (const TVol& m)
{
    AsVec() -= m.AsVec();

    return SELF;
}

inline TVol& TVol::operator *= (TMReal s)
{   
    AsVec() *= s;
    return SELF;
}

inline TVol& TVol::operator /= (TMReal s)
{   
    AsVec() /= s;
    return SELF;
}


inline void TVol::MakeZero()
{
    if (!mData)
        return;

    int count = mSlices * mRows * mCols;
    memset(mData, 0, count * sizeof(TMReal));
}

inline void TVol::MakeNull()
{
    if (!IsRef())
        delete[] mData;

    mData   = 0;
    mSlices &= VL_REF_FLAG;
    mRows   = 0;
    mCols   = 0;
}

inline void TVol::MakeRef(const TVol& v)
{
    if (!IsRef())
        delete[] mData;

    mData   = v.Ref();
    mSlices = v.Slices() | VL_REF_FLAG;
    mRows   = v.Rows();
    mCols   = v.Cols();
}

inline bool TVol::IsNull() const
{
    return mCols == 0;
}

inline bool TVol::IsRef() const
{
    return (mSlices & VL_REF_FLAG) != 0;
}

inline const TMVec TVol::AsVec() const
{
    return TMVec(mCols * mRows * (mSlices & VL_REF_MASK), mData);
}

inline TMVec TVol::AsVec()
{
    return TMVec(mCols * mRows * (mSlices & VL_REF_MASK), mData);
}


#endif
