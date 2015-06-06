/*
    File:           VLSparseMat.h

    Function:       Defines a sparse matrix.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_SparseMat_H
#define VL_SparseMat_H

#include "VL.h"
#include "VLMat.h"
#include "VLSparseVec.h"
#include "VLSubSMat.h"


// --- SparseMat Class --------------------------------------------------------


class TSparseMat
{
public:
    
    // Constructors
    
    TSparseMat();                           // Null matrix: no space allocated
    TSparseMat(int rows, int cols);         // Ordinary uninitialised matrix
    TSparseMat(int rows, int cols, ZeroOrOne k); // I * k
    TSparseMat(int rows, int cols, Block k);// block matrix (m[i][j] = k)
    TSparseMat(const TSparseMat &m);        // Copy constructor 
    TSparseMat(const TSubSMat &m);          // Conversion constructors...   
    TSparseMat(const TMat &m);          

    ~TSparseMat();          
    
    // Accessor functions
    
    inline int          Rows() const { return rows; };
    inline int          Cols() const { return cols; };

    inline TMSparseVec  &operator [] (int i);       // Indexing by row
    inline const TMSparseVec
                        &operator [] (int i) const; // no checking    

    inline TMSparseVec  *Ref() const;               // Return pointer to data

    // Assignment operators
    
    TSparseMat          &operator =  (const TSparseMat &m);     
    TSparseMat          &operator =  (const TSubSMat &m);   
    TSparseMat          &operator =  (const TMat &m);   
    inline TSparseMat   &operator =  (ZeroOrOne k);
    inline TSparseMat   &operator =  (Block k);

    void                SetSize(int m, int n);
    bool                IsSquare() const { return(rows == cols); };

    //  Matrix initialisers
    
    void                MakeZero();
    void                MakeIdentity();
    void                MakeDiag(TMReal k = vl_one);
    void                MakeBlock(TMReal k = vl_one);

protected:
    //  Private ...
    TMSparseVec *row;
    int         rows;
    int         cols;
};


// --- SparseMat In-Place Operators -------------------------------------------

TSparseMat  &operator += (TSparseMat &m, const TSparseMat &n);
TSparseMat  &operator -= (TSparseMat &m, const TSparseMat &n);
TSparseMat  &operator *= (TSparseMat &m, const TSparseMat &n);
TSparseMat  &operator *= (TSparseMat &m, TMReal s);
TSparseMat  &operator /= (TSparseMat &m, TMReal s);

// --- SparseMat Comparison Operators -----------------------------------------

bool        operator == (const TSparseMat &m, const TSparseMat &n);
bool        operator != (const TSparseMat &m, const TSparseMat &n);

// --- SparseMat Arithmetic Operators -----------------------------------------

TSparseMat  operator + (const TSparseMat &m, const TSparseMat &n);
TSparseMat  operator - (const TSparseMat &m, const TSparseMat &n);
TSparseMat  operator - (const TSparseMat &m);
TSparseMat  operator * (const TSparseMat &m, const TSparseMat &n);
TSparseMat  operator * (const TSparseMat &m, TMReal s);
TSparseMat  operator / (const TSparseMat &m, TMReal s);

TSparseVec  &operator *= (TSparseVec &v, const TSparseMat &m);
TMSparseVec operator * (const TSparseVec &v, const TSparseMat &m);
TSparseVec  operator * (const TSparseMat &m, const TSparseVec &v);
TMVec       &operator *= (TMVec &v, const TSparseMat &m);
TMVec       operator * (const TMVec &v, const TSparseMat &m);
TMVec       operator * (const TSparseMat &m, const TMVec &v);

TSparseMat  trans(const TSparseMat &m);
TMReal      trace(const TSparseMat &m);
TSparseMat  oprod(const TSparseVec &a, const TSparseVec &b);
TSparseMat  oprods(const TVec &a, const TVec &b);
TSparseMat  inv(const TSparseMat &m, TMReal *determinant = 0, TMReal pEps = 1e-20);


// --- SparseMat Functions ----------------------------------------------------

void Add     (const TSparseMat &m, const TSparseMat &n, TSparseMat& result);
void Subtract(const TSparseMat &m, const TSparseMat &n, TSparseMat& result);
void Negate  (const TSparseMat &m, TSparseMat& result);
void Multiply(const TSparseMat &m, const TSparseMat &n, TSparseMat& result);
void Multiply(const TSparseMat &m, TMReal s, TSparseMat& result);
void Divide  (const TSparseMat &m, TMReal s, TSparseMat& result);

void Multiply(const TSparseVec &v, const TSparseMat &m, TSparseVec& result);
void Multiply(const TSparseMat &m, const TSparseVec &v, TSparseVec& result);
void Multiply(const TMVec &v, const TSparseMat &m, TMVec& result);
void Multiply(const TSparseMat &m, const TMVec &v, TMVec& result);

void Transpose(const TSparseMat &m, TSparseMat& result);
void OuterProduct(const TSparseVec &a, const TSparseVec &b, TSparseMat& result);
void OuterProduct(const TVec &a, const TVec &b, TSparseMat& result);
bool Invert(const TSparseMat &m, TSparseMat& result, TMReal *determinant = 0, TMReal pEps = 1e-20);


// --- SparseMat Inlines ------------------------------------------------------


inline TMSparseVec &TSparseMat::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, Rows(), "(SparseMat::[i]) i index out of range");
    
    return(row[i]);
}

inline const TMSparseVec &TSparseMat::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, Rows(), "(SparseMat::[i]) i index out of range");
    
    return(row[i]);
}

inline TMSparseVec *TSparseMat::Ref() const
{
    return(row);
}

inline TSparseMat &TSparseMat::operator = (ZeroOrOne k)
{
    MakeDiag(TMReal(k));
    return(*this);
};

inline TSparseMat &TSparseMat::operator = (Block k)
{
    MakeBlock(TMReal(k));
    return(*this);
};

#endif

