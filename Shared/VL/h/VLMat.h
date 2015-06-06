/*
    File:           VLMat.h

    Function:       Defines a generic resizeable matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Mat_H
#define VL_Mat_H

#include "VL.h"
#include "VLVec.h"
#include "VLSubMat.h"


// --- Mat Class --------------------------------------------------------------


class TMat
{
public:
    
    // Constructors
    
    TMat();                          // Null matrix: no space allocated
    TMat(int rows, int cols);        // Ordinary uninitialised matrix
    TMat(int rows, int cols, double elt0, ...); // Mat(2, 2, 1.0, 2.0, 3.0, 4.0)
    TMat(int nrows, int ncols, TMReal* data);   // Create reference matrix
    TMat(const TMat& m);                        // Copy constructor
    TMat(int rows, int cols, ZeroOrOne k);      // I * k
    TMat(int rows, int cols, Block k);          // block matrix (m[i][j] = k)
    explicit TMat(const TSubMat& m);            // Conversion constructors...
#ifdef VL_Mat2_H
    TMat(const TMat2& m);
    TMat(const TMat3& m);
    TMat(const TMat4& m);
#endif

    ~TMat();

    // Accessor methods
    int     Rows() const;
    int     Cols() const;

    int     Elts() const;

    TMVec   operator [] (int i);        // Indexing by row
    TMVec   operator [] (int i) const;  // Indexing by row

    TMReal&       operator () (int i, int j); 
    const TMReal& operator () (int i, int j) const;
    
    void    SetSize(int nrows, int ncols);
    void    SetSize(const TMat& m);
    bool    IsSquare() const;

    TMReal*       Ref() const;               // Return pointer to data
    const TMReal* ConstRef() const;          // Return pointer to data

    // Assignment operators
    TMat&   operator =  (const TMat& m);   // Assignment of a matrix
    TMat&   operator =  (const TSubMat& m);// Assignment of submatrix
    TMat&   operator =  (ZeroOrOne k);     // Set to k * I...
    TMat&   operator =  (Block k);         // Set to a block matrix...
#ifdef VL_Mat2_H
    TMat&   operator =  (const TMat2& m);  
    TMat&   operator =  (const TMat3& m);  
    TMat&   operator =  (const TMat4& m);
#endif

    // In-Place Operators
    TMat&   operator += (const TMat& m);
    TMat&   operator -= (const TMat& m);
    TMat&   operator *= (const TMat& m);
    TMat&   operator *= (TMReal s);
    TMat&   operator /= (TMReal s);

    // Matrix initialisers
    void    MakeZero();
    void    MakeIdentity();
    void    MakeDiag(TMReal k);
    void    MakeBlock(TMReal k);

    void    MakeNull();             ///< Make this a zero-sized matrix
    void    MakeRef(const TMat& m); ///< Make this a reference of 'm'

    bool    IsNull() const;         ///< Returns true if this is a null-sized matrix.
    bool    IsRef() const;          ///< Returns true if this is a reference to storage elsewhere

    TMat&   Clamp(Real fuzz);
    TMat&   Clamp();

    // Representation as vector
    TMVec       AsVec();
    const TMVec AsVec() const;

    // Data
protected:
    TMReal*  mData;
    uint32_t mRows;
    uint32_t mCols;
};


// --- Reference Matrix Class --------------------------------------------------

class TRefMat : public TMat
{
public:
    TRefMat();
    TRefMat(const TMat& m);
    TRefMat(int rows, int cols, TMReal* data);
#ifdef VL_Mat2_H
    TRefMat(const TMat2& m);
    TRefMat(const TMat3& m);
    TRefMat(const TMat4& m);
#endif

    // Assignment operators
    TMat&   operator =  (const TMat& m)     { return TMat::operator=(m); }
    TMat&   operator =  (const TSubMat& m)  { return TMat::operator=(m); }
    TMat&   operator =  (ZeroOrOne k)       { return TMat::operator=(k); }
    TMat&   operator =  (Block k)           { return TMat::operator=(k); }
};


// --- Mat Comparison Operators -----------------------------------------------

bool    operator == (const TMat& m, const TMat& n);
bool    operator != (const TMat& m, const TMat& n);

// --- Mat Arithmetic Operators -----------------------------------------------

TMat    operator + (const TMat& m, const TMat& n);
TMat    operator - (const TMat& m, const TMat& n);
TMat    operator - (const TMat& m);
TMat    operator * (const TMat& m, const TMat& n);
TMat    operator * (const TMat& m, TMReal s);
TMat    operator * (TMReal s, const TMat& m);
TMat    operator / (const TMat& m, TMReal s);

TMVec   operator * (const TMat& m, const TMVec& v);
TMVec   operator * (const TMVec& v, const TMat& m);

TMat    trans(const TMat& m);                   // Transpose
TMReal  trace(const TMat& m);                   // Trace
TMat    inv(const TMat& m, TMReal* determinant = 0, TMReal pEps = TMReal(1e-20));
                                                // Inverse
TMat    oprod(const TMVec& a, const TMVec& b);  // Outer product

TMat    clamped(const TMat& m, Real fuzz);
TMat    clamped(const TMat& m);


// --- Mat Functions ----------------------------------------------------------

void Add     (const TMat& m, const TMat& n, TMat& result);
void Subtract(const TMat& m, const TMat& n, TMat& result);
void Negate  (const TMat& m, TMat& result);
void Multiply(const TMat& m, const TMat& n, TMat& result);
void Multiply(const TMat& m, TMReal s, TMat& result);
void MultiplyAccum(const TMat& m, TMReal s, TMat& result);
void Divide  (const TMat& m, TMReal s, TMat& result);

void Multiply(const TMat& m, const TMVec& v, TMVec& result);
void Multiply(const TMVec& v, const TMat& m, TMVec& result);

void   Transpose(const TMat& m, TMat& result);
TMReal InnerProduct(const TMat& m, const TMat& n);
void   OuterProduct(const TMVec& a, const TMVec& b, TMat& result);
bool   Invert(const TMat& m, TMat& VL_RESTRICT result, TMReal* determinant = 0, TMReal pEps = TMReal(1e-20));


// --- Mat Inlines ------------------------------------------------------------


inline TMat::TMat() : mData(0), mRows(0), mCols(0)
{
}

inline TMat::TMat(int rows, int cols) : mData(), mRows(rows), mCols(cols)
{
    CL_ASSERT_MSG(rows > 0 && cols > 0, "(Mat) illegal matrix size");
    
    mData = VL_NEW TMReal[rows * cols];
}

inline TMat::TMat(int rows, int cols, TMReal* data) :
    mData(data), mRows(rows | VL_REF_FLAG), mCols(cols)
{
}

#ifdef VL_Mat2_H
inline TMat::TMat(const TMat2& m) : mData(m.Ref()), mRows(2 | VL_REF_FLAG), mCols(2)
{
}

inline TMat::TMat(const TMat3& m) : mData(m.Ref()), mRows(3 | VL_REF_FLAG), mCols(3)
{
}

inline TMat::TMat(const TMat4& m) : mData(m.Ref()), mRows(4 | VL_REF_FLAG), mCols(4)
{
}
#endif

inline int TMat::Elts() const
{
    return mCols * (mRows & VL_REF_MASK);
}

inline int TMat::Rows() const
{
    return (mRows & VL_REF_MASK);
}

inline int TMat::Cols() const
{
    return mCols;
}

inline TMVec TMat::operator [] (int i)
{
    CL_INDEX_MSG(i, Rows(), "(Mat::[i]) i index out of range");
    
    return TMVec(mCols, mData + i * mCols);
}

inline TMVec TMat::operator [] (int i) const
{
    CL_INDEX_MSG(i, Rows(), "(Mat::[i]) i index out of range");
    
    return TMVec(mCols, mData + i * mCols);
}

inline TMReal& TMat::operator () (int i, int j)
{
    CL_INDEX_MSG(i, Rows(), "(Mat::e(i,j)) i index out of range");
    CL_INDEX_MSG(j, Cols(), "(Mat::e(i,j)) j index out of range");
    
    return mData[i * mCols + j];
}

inline const TMReal& TMat::operator () (int i, int j) const
{
    CL_INDEX_MSG(i, Rows(), "(Mat::e(i,j)) i index out of range");
    CL_INDEX_MSG(j, Cols(), "(Mat::e(i,j)) j index out of range");
    
    return mData[i * mCols + j];
}

inline bool TMat::IsSquare() const
{
    return Rows() == mCols;
}

inline TMReal* TMat::Ref() const
{
    return mData;
}

inline const TMReal* TMat::ConstRef() const
{
    return mData;
}

inline TMat operator * (TMReal s, const TMat& m)
{
    return m * s;
}

inline TMat& TMat::operator = (ZeroOrOne k)
{
    MakeDiag(TMReal(k));
    return SELF;
}

inline TMat& TMat::operator = (Block k)
{
    MakeBlock(TMReal(k));
    return SELF;
}

inline void TMat::MakeNull()
{
    if (!IsRef())
        delete[] mData;

    mData   = 0;
    mRows   &= VL_REF_FLAG;
    mCols   = 0;
}

inline void TMat::MakeRef(const TMat& m)
{
    if (!IsRef())
        delete[] mData;
    mData = m.Ref();
    mRows = m.Rows() | VL_REF_FLAG;
    mCols = m.Cols();
}

inline bool TMat::IsNull() const
{
    return mCols == 0;
}

inline bool TMat::IsRef() const
{
    return (mRows & VL_REF_FLAG) != 0;
}

inline const TMVec TMat::AsVec() const
{
    return TMVec(mCols * Rows(), mData);
}

inline TMVec TMat::AsVec()
{
    return TMVec(mCols * Rows(), mData);
}

inline TMat::~TMat()
{
    if (!IsRef())
        VL_DELETE[] mData;
}

inline TRefMat::TRefMat() : TMat(0, 0, (TMReal*) 0)
{
}

inline TRefMat::TRefMat(const TMat& m) : TMat(m.Rows(), m.Cols(), m.Ref())
{
}

inline TRefMat::TRefMat(int rows, int cols, TMReal* data) : TMat(rows, cols, data)
{
}

inline TMReal InnerProduct(const TMat& m, const TMat& n)
{
    CL_ASSERT_MSG(m.Rows() == n.Rows(), "(InnerProduct) matrix sizes don't match");
    CL_ASSERT_MSG(m.Cols() == n.Cols(), "(InnerProduct) matrix sizes don't match");

    int numElts = m.Rows() * m.Cols();
    return dot(TMVec(numElts, m.Ref()), TMVec(numElts, n.Ref()));
}


#endif
