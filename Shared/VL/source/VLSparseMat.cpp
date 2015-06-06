/*
    File:           VLSparseMat.cpp

    Function:       Implements VLSparseMat.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLSparseMat.h"
#include <stdarg.h>


// --- SparseMat Constructors & Destructors -----------------------------------


TSparseMat::TSparseMat() : row(0), rows(0), cols(0)
{
}

TSparseMat::TSparseMat(int m, int n) : row(0), rows(m), cols(n)
{
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i].SetSize(cols);
}

TSparseMat::TSparseMat(int m, int n, ZeroOrOne k) : row(0), rows(m), cols(n)
{
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i].SetSize(cols);

    MakeDiag(TMReal(k));
}

TSparseMat::TSparseMat(int m, int n, Block k) : row(0), rows(m), cols(n)
{
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i].SetSize(cols);

    MakeBlock(TMReal(k));
}

TSparseMat::TSparseMat(const TSparseMat &m) : row(0), rows(m.rows), cols(m.cols)
{
    CL_ASSERT_MSG(m.row != 0, "(SparseMat) Can't construct from null matrix");
    
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i] = m.row[i];
}

TSparseMat::TSparseMat(const TSubSMat &m) : row(0), rows(m.Rows()), cols(m.Cols())
{
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i] = m[i];
}

TSparseMat::TSparseMat(const TMat &m) : row(0), rows(m.Rows()), cols(m.Cols())
{
    row = VL_NEW TMSparseVec[rows];
    for (int i = 0; i < rows; i++)
        row[i] = m[i];
}

TSparseMat::~TSparseMat()
{
    VL_DELETE[] row;
}

void TSparseMat::SetSize(int m, int n)
{
    CL_ASSERT_MSG(m > 0 && n > 0, "(SparseMat::SetSize) illegal matrix size");

    if (m != rows || n != cols)
    {
        if (m > rows)
        {
            VL_DELETE[] row;   
            row = VL_NEW TMSparseVec[m];
            cols = 0; // reflect default vec size
        }

        if (n != cols)
            for (int i = 0; i < m; i++)
                row[i].SetSize(n);

        rows = m;
        cols = n;
    }
}


// --- SparseMat Assignment Operators -----------------------------------------


TSparseMat &TSparseMat::operator = (const TSparseMat &m)
{   
    if (rows == 0)
        SetSize(m.Rows(), m.Cols());
    else
        CL_ASSERT_MSG(rows == m.Rows() && cols == m.Cols(),
             "(SparseMat::=) matrices not the same size");
        
    for (int i = 0; i < rows; i++)
        row[i] = m.row[i];
    
    return(*this);
}
      
TSparseMat &TSparseMat::operator = (const TMat &m)
{   
    SetSize(m.Rows(), m.Cols());

    for (int i = 0; i < rows; i++)
        row[i] = m[i];

    return(*this);
}

TSparseMat &TSparseMat::operator = (const TSubSMat &m)
{   
    SetSize(m.Rows(), m.Cols());

    for (int i = 0; i < rows; i++)
        row[i] = m[i];

    return(*this);
}

void TSparseMat::MakeZero()
{
    for (int i = 0; i < rows; i++)
    {
        row[i].Begin();
        row[i].End();
    }
}

void TSparseMat::MakeIdentity()
{
    for (int i = 0; i < rows; i++)
        row[i].MakeUnit(i, vl_one);
}

void TSparseMat::MakeDiag(TMReal k)
{
    for (int i = 0; i < rows; i++)
        row[i].MakeUnit(i, k);      
}

void TSparseMat::MakeBlock(TMReal k)
{
    for (int i = 0; i < rows; i++)
        row[i].MakeBlock(k);
}


// --- Mat Assignment Operators -----------------------------------------------


TSparseMat &operator += (TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::+=) matrix rows don't match");    
    
    for (int i = 0; i < m.Rows(); i++) 
        m[i] += n[i];
    
    return(m);
}

TSparseMat &operator -= (TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::-=) matrix rows don't match");    
    
    for (int i = 0; i < m.Rows(); i++) 
        m[i] -= n[i];
    
    return(m);
}

TSparseMat &operator *= (TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(m.Cols() == n.Cols(), "(SparseMat::*=) matrix columns don't match"); 
    
    for (int i = 0; i < m.Rows(); i++) 
        m[i] *= (TSparseMat &) n;
    
    return(m);
}

TSparseMat &operator *= (TSparseMat &m, TMReal s)
{   
    for (int i = 0; i < m.Rows(); i++) 
        m[i] *= s;
    
    return(m);
}

TSparseMat &operator /= (TSparseMat &m, TMReal s)
{   
    for (int i = 0; i < m.Rows(); i++) 
        m[i] /= s;
    
    return(m);
}

// --- Mat Comparison Operators -----------------------------------------------

bool operator == (const TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::==) matrix rows don't match");    
    
    for (int i = 0; i < m.Rows(); i++) 
        if (m[i] != n[i])
            return(0);

    return(1);
}

bool operator != (const TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::!=) matrix rows don't match");    
    
    for (int i = 0; i < m.Rows(); i++) 
        if (m[i] != n[i])
            return(1);

    return(0);
}

void Add(const TSparseMat &m, const TSparseMat &n, TSparseMat& result)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::+) matrix rows don't match"); 
    
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Add(m[i], n[i], result[i]);
}

void Subtract(const TSparseMat &m, const TSparseMat &n, TSparseMat& result)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::-) matrix rows don't match"); 
    
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Subtract(m[i], n[i], result[i]);
}

void Negate(const TSparseMat &m, TSparseMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Negate(m[i], result[i]);
}

/*
void Negate(TSparseMat& m)
{
    for (int i = 0; i < m.Rows(); i++) 
        Negate(m[i]);
}
*/
void Multiply(const TSparseMat &m, const TSparseMat &n, TSparseMat& result)
{
    CL_ASSERT_MSG(m.Cols() == n.Rows(), "(SparseMat::*m) matrix cols don't match");    
    
    result.SetSize(m.Rows(), n.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Multiply(m[i], n, result[i]);
}

void Multiply(const TSparseMat &m, TMReal s, TSparseMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Multiply(m[i], s, result[i]);
}

void Divide(const TSparseMat &m, TMReal s, TSparseMat& result)
{
    result.SetSize(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        Divide(m[i], s, result[i]);
}

void Multiply(const TSparseVec &v, const TSparseMat &m, TSparseVec& result)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
        
    result.SetSize(m.Cols());
    TSVIter j(v);

    // v * M = v[0] * m[0] + v[1] * m[1] + ...

    for (int i = 0; i < m.Rows(); i++)  // XXX use Inc() and .index instead
    {       
        j.Inc(i);
        if (j.Exists(i))
            MultiplyAccum(m[i], j.Data(), result);
    }
}

void Multiply(const TSparseMat &m, const TSparseVec &v, TSparseVec& result)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::*v) matrix and vector sizes don't match");
    
    result.SetSize(m.Rows());
    result.Begin();
    
    for (int i = 0; i < m.Rows(); i++) 
        result.AddElt(i, dot(m[i], v));
    
    result.End();
}

void Multiply(const TMVec &v, const TSparseMat &m, TMVec& result)
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    result.SetSize(m.Cols());
    result.MakeZero();

    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (int i = 0; i < m.Rows(); i++) 
        if (len(v[i]) > 0)
            for (int j = 0, nj = m[i].pairs.NumItems() - 1; j < nj; j++)
                result[m[i].pairs[j].index] += v[i] * m[i].pairs[j].elt;
}

void Multiply(const TSparseMat &m, const TMVec &v, TMVec& result)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::*v) matrix and vector sizes don't match");
    
    result.SetSize(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
}

void Transpose(const TSparseMat &m, TSparseMat& result)
{
    result.SetSize(m.Cols(), m.Rows());
        
    for (int i = 0; i < result.Rows(); i++)
        result[i].Begin();

    for (int i = 0; i < m.Rows(); i++)
    {
        // For row i of 'm', add its elements to the ends of the
        // appropriate row of 'result'.
    
        for (size_t j = 0; j < m[i].pairs.NumItems() - 1; j++)
            result[m[i].pairs[j].index].AddNZElt(i, m[i].pairs[j].elt);
    }
    
    for (int i = 0; i < result.Rows(); i++)
        result[i].End();
}

void OuterProduct(const TSparseVec &a, const TSparseVec &b, TSparseMat& result)
{
    result.SetSize(a.Elts(), b.Elts());
    TSVIter i;
        
    for (i.Begin(a); !i.AtEnd(); i.Inc())
        Multiply(b, i.Data(), result[i.Index()]);
}

void OuterProduct(const TVec &a, const TVec &b, TSparseMat& result)
{
    result.SetSize(a.Elts(), b.Elts());
    
    for (int i = 0; i < a.Elts(); i++)
        if (TSparseVec::IsNonZero(a[i]))
        {
            result[i] = b;
            result[i] *= a[i];
        }
}

bool Invert(const TSparseMat &m, TSparseMat& B, TMReal *determinant, TMReal pEps)
{
    CL_ASSERT_MSG(m.IsSquare(), "(inv) Matrix not square"); 

    //  Note that the sparse version of inv() is actually simpler than
    //  the normal version.

    int             n = m.Rows();
    TMReal          t, det, pivot;
    TMReal          max;
    TSparseMat      A(m);
    
    B.SetSize(n, n);
    B.MakeDiag(vl_one);

    // ---------- Forward elimination ---------- ------------------------------
    
    det = vl_1;
    
    for (int i = 0; i < n; i++)     
    {           
        // Eliminate in column i, below diagonal
        
        max = -1.0;
        
        // Find a pivot for column i 
        // For the sparse rows, we take advantage of the fact that if A(i, k) exists,
        // it will be the first element in the sparse vector. (Previous elements will have
        // been zeroed by previous elimination steps.)
        int k = i; // in case the matrix is singular, set *something*
        for (int j = i; j < n; j++) 
            if ((A[j].pairs[0].index == i) && (len(A[j].pairs[0].elt) > max))
            {
                pivot = A[j].pairs[0].elt;
                max = len(pivot);
                k = j;
            }
            
        // If no nonzero pivot exists, A must be singular...

        if (max > pEps)
        {
            if (determinant)
                *determinant = vl_0;
            return false;
        }
            
        // Swap rows i and k

        if (k != i)
        {   
            //  Sparse matrix rows are just arrays: we can swap the contents
            //  of the two arrays efficiently by using the array Swap function.

            A[i].Swap(A[k]);
            B[i].Swap(B[k]);
                
            det = -det;
        }
        
        det *= pivot;
        
        A[i] /= pivot;
        B[i] /= pivot;    
           
        for (int j = i + 1; j < n; j++)
        {
            // Eliminate in rows below i 
            // Again, if A[j,i] exists, it will be the first non-zero element of the row.
            
            if (A[j].pairs[0].index == i)
            {
                t = A[j].pairs[0].elt;
                A[j] -= A[i] * t;
                B[j] -= B[i] * t;
            }
        }
    }

    // ---------- Backward elimination ---------- -----------------------------

    TMSparseVec temp;
    for (int i = 1; i < n; i++)         // Eliminate in column i, above diag 
    {       
        for (int j = 0; j < i; j++)         // Eliminate in rows above i 
        {       
            if (A[j].pairs[1].index == i)
            {
                t = A[j].pairs[1].elt;
                Multiply(A[i], t, temp);
                A[j] -= temp;
                Multiply(B[i], t, temp);
                B[j] -= temp;
            }
        }
    }

    if (determinant)
        *determinant = det;

    return true;
}


// --- Mat Arithmetic Operators -----------------------------------------------

TSparseMat operator + (const TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::+) matrix rows don't match"); 
    
    TSparseMat  result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = m[i] + n[i];
    
    return(result);
}

TSparseMat operator - (const TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(n.Rows() == m.Rows(), "(SparseMat::-) matrix rows don't match"); 
    
    TSparseMat  result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = m[i] - n[i];
    
    return(result);
}

TSparseMat operator - (const TSparseMat &m)
{
    TSparseMat  result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = -m[i];
    
    return(result);
}

TSparseMat operator * (const TSparseMat &m, const TSparseMat &n)
{
    CL_ASSERT_MSG(m.Cols() == n.Rows(), "(SparseMat::*m) matrix cols don't match");    
    
    TSparseMat  result(m.Rows(), n.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = m[i] * n;
    
    return(result);
}

TSparseVec operator * (const TSparseMat &m, const TSparseVec &v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::m*v) matrix and vector sizes don't match");
    
    TSparseVec  result(m.Rows());
    
    result.Begin();
    
    for (int i = 0; i < m.Rows(); i++) 
        result.AddElt(i, dot(m[i], v));
    
    result.End();
        
    return(result);
}

TMVec operator * (const TSparseMat &m, const TMVec &v)
{
    CL_ASSERT_MSG(m.Cols() == v.Elts(), "(SparseMat::*v) matrix and vector sizes don't match");
    
    TMVec       result(m.Rows());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = dot(m[i], v);
    
    return(result);
}

TSparseMat operator * (const TSparseMat &m, TMReal s)
{
    TSparseMat  result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = m[i] * s;
    
    return(result);
}

TSparseMat operator / (const TSparseMat &m, TMReal s)
{
    TSparseMat  result(m.Rows(), m.Cols());
    
    for (int i = 0; i < m.Rows(); i++) 
        result[i] = m[i] / s;
    
    return(result);
}


// --- Mat-Vec Functions ------------------------------------------------------


TMSparseVec operator * (const TSparseVec &v, const TSparseMat &m)           // v * m
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TMSparseVec result(m.Cols());
    TSVIter     j(v);

    result = vl_zero;   
    
    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (int i = 0; i < m.Rows(); i++) 
    {       
        j.Inc(i);
        if (j.Exists(i))
            result += m[i] * j.Data();
    }
    
    return(result);
}

TMVec operator * (const TMVec &v, const TSparseMat &m)          // v * m
{
    CL_ASSERT_MSG(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");
    
    TMVec result(m.Cols());

    result = vl_zero;   

    // v * M = v[0] * m[0] + v[1] * m[1] + ...
    
    for (int i = 0; i < m.Rows(); i++) 
        if (len(v[i]) > 0)
            for (int j = 0, nj = m[i].pairs.NumItems() - 1; j < nj; j++)
                result[m[i].pairs[j].index] += v[i] * m[i].pairs[j].elt;
    
    return(result);
}

TSparseVec &operator *= (TSparseVec &v, const TSparseMat &m)        // v *= m
{
    TSparseVec t = v * m;
    v.Swap(t);
    return(v);
}

TMVec &operator *= (TMVec &v, const TSparseMat &m)                  // v *= m
{
    v = v * m;
    return(v);
}


// --- Mat Special Functions --------------------------------------------------


TSparseMat trans(const TSparseMat &m)
{
    TSparseMat  result(m.Cols(), m.Rows());
        
    for (int i = 0; i < result.Rows(); i++)
        result[i].Begin();

    for (int i = 0; i < m.Rows(); i++)
    {
        // For row i of 'm', add its elements to the ends of the
        // appropriate row of 'result'.
    
        for (size_t j = 0; j < m[i].pairs.NumItems() - 1; j++)
            result[m[i].pairs[j].index].AddNZElt(i, m[i].pairs[j].elt);
    }
    
    for (int i = 0; i < result.Rows(); i++)
        result[i].End();
            
    return(result);
}

TMReal trace(const TSparseMat &m)
{
    TSVIter j;
    TMReal  result = vl_0;
    
    for (int i = 0; i < m.Rows(); i++) 
    {
        j.Begin(m[i]);
        
        // Find element i of m[i], and if it exists,
        // add it to the result.
        
        if (j.IncTo(i))
            result += j.Data();
    }
    
    return(result);
}

TSparseMat oprod(const TSparseVec &a, const TSparseVec &b)
// returns outerproduct of a and b:  a * trans(b)
{
    TSparseMat  result;
    TSVIter i;
    
    result.SetSize(a.Elts(), b.Elts());
    result = vl_0;
        
    for (i.Begin(a); !i.AtEnd(); i.Inc())
    {
        result[i.Index()] = b;
        result[i.Index()] *= i.Data();
    }
    
    return(result);
}

TSparseMat oprods(const TVec &a, const TVec &b)
// returns outerproduct of a and b:  a * trans(b)
{
    TSparseMat  result;

    result.SetSize(a.Elts(), b.Elts());
    
    for (int i = 0; i < a.Elts(); i++)
        if (TSparseVec::IsNonZero(a[i]))
        {
            result[i] = b;
            result[i] *= a[i];
        }
        else
            result[i] = vl_0;

    return(result);
}


TSparseMat inv(const TSparseMat &m, TMReal *determinant, TMReal pEps)
{
    CL_ASSERT_MSG(m.IsSquare(), "(inv) Matrix not square"); 

    //  Note that the sparse version of inv() is actually simpler than
    //  the normal version.

    int             n = m.Rows();
    TMReal          t, det, pivot;
    TMReal          max;
    TSparseMat      A(m);
    TSparseMat      B(n, n, vl_I);      

    // ---------- Forward elimination ---------- ------------------------------
    
    det = vl_1;
    
    for (int i = 0; i < n; i++)     
    {           
        // Eliminate in column i, below diagonal
        
        max = -1.0;
        
        // Find a pivot for column i 
        // For the sparse rows, we take advantage of the fact that if A(i, k) exists,
        // it will be the first element in the sparse vector. (Previous elements will have
        // been zeroed by previous elimination steps.)
        int k = i; // in case the matrix is singular, set *something*
        for (int j = i; j < n; j++) 
            if ((A[j].pairs[0].index == i) && (len(A[j].pairs[0].elt) > max))
            {
                pivot = A[j].pairs[0].elt;
                max = len(pivot);
                k = j;
            }
            
        // If no nonzero pivot exists, A must be singular...

        CL_ASSERT_MSG(max > pEps, "(inv) Matrix is singular");
            
        // Swap rows i and k

        if (k != i)
        {   
            //  Sparse matrix rows are just arrays: we can swap the contents
            //  of the two arrays efficiently by using the array Swap function.

            A[i].Swap(A[k]);
            B[i].Swap(B[k]);
                
            det = -det;
        }
        
        det *= pivot;
        
        A[i] /= pivot;
        B[i] /= pivot;    
           
        for (int j = i + 1; j < n; j++)
        {
            // Eliminate in rows below i 
            // Again, if A[j,i] exists, it will be the first non-zero element of the row.
            
            if (A[j].pairs[0].index == i)
            {
                t = A[j].pairs[0].elt;
                A[j] -= A[i] * t;
                B[j] -= B[i] * t;
            }
        }
    }

    // ---------- Backward elimination ---------- -----------------------------

    for (int i = 1; i < n; i++)         // Eliminate in column i, above diag 
    {       
        for (int j = 0; j < i; j++)         // Eliminate in rows above i 
        {       
            if (A[j].pairs[1].index == i)
            {
                t = A[j].pairs[1].elt;
                A[j] -= A[i] * t;
                B[j] -= B[i] * t;
            }
        }
    }

    if (determinant)
        *determinant = det;

    return(B);
}
