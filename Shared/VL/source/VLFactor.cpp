/*
    File:       VLFactor.cpp

    Function:   Implements some matrix factorizations: SVD, QR

    Author:     Andrew Willmott

    Notes:      Hopefully this is a bit more understandable
                than most of the other SVD code out there.

                This should also make it a bit easier to optimize for modern
                compilers, but that's a task for another day.
*/

#include "VLFactor.h"

void RotateRight(TMat& U, const int i, const int j, const TMReal c, const TMReal s);

void ClearSuperEntry
(
    TVec&           diagonal,
    TVec&           superDiag,
    TMat&           U,
    const int       k,
    const int       l,
    const TMReal    precision
);

int FindSplit
(
    TVec&           diagonal,
    TVec&           superDiag,
    TMat&           U,
    const int       k,
    const TMReal    precision
);


/*
    NOTE
    
    Householder transformations zero all elements of a vector v apart
    from the first element.
    
        H = I - 2uut
            where ||u||^2 = 1
            HtH = HHt = HH = I
            - it's a reflection, so we would expect HH = I

        Hv = [c 0 0 0 0]
            where c = +/- ||v||
            and u = f(v - [c 0 0..])
            where f = 1/sqrt(2c(c - v[0]))

        For convenience, we can write
            Hv = v - 2u(u.v)
        or
            Hv = v - 2w(w.v)/g
            where g = c(c - v[0])
            w = v - [c 0 0..]
*/

//#define DO_VL_SUB

#ifndef VL_MIXED

#ifdef DO_VL_SUB
// experiment to see just how much slower using the generic sub-matrix
// calls makes things.
// Answer: quite a bit. Must improve this.

TMReal LeftHouseholder(TMat &A, TMat &U, const int i)
// Zeroes out those elements below the diagonal in column i by use of a
// Householder transformation matrix H: A' = HA. U is replaced with UH,
// so that U'A' = UH HA = UA
{
    CL_ASSERT_MSG(i < A.Rows(), "bad i");

    int         j, k;
    TMReal      scale, vSqrLen, oldDiag, newDiag, g, dotProd;
    int         clen = A.Rows() - i;
    TSubVec     Ai = sub(col(A, i), i, clen);

    // scale factor is the sum of abs of elements below and including
    // the diagonal in column i
    scale = 0.0;
    for (k = 0; k < Ai.Elts(); k++)
        scale += fabs(Ai[k]);
        
    // return if nothing to eliminate...
    if (scale == 0.0)
        return(0.0);

    // scale the column, find sum of squares
    Ai = Ai / scale;
    vSqrLen = sqrlen(Ai);
    
    oldDiag = Ai[0];            // v[0]
    newDiag = sqrt(vSqrLen);    // c
    if (oldDiag > 0.0)
        newDiag = -newDiag;
        
    g = vSqrLen - oldDiag * newDiag;    // c(c - v0)

    // replace column i of A with w
    Ai[0] = oldDiag - newDiag;      // v[0] -= c

    // Apply H to A by transforming remaining columns of A
    for (j = i + 1; j < A.Cols(); j++)
    {
        TSubVec     Aj = sub(col(A, j), i, clen);

        // Hv = v - w(v.w) / g
        Aj = Aj - Ai * (dot(Ai, Aj) / g);
    }

    // Apply H to rows of U
    for (j = 0; j < A.Rows(); j++)  
    {
        TSubVec     Uj = sub(row(U, j), i, clen);

        // vH = v - (v.w)w / g
        Uj = Uj - Ai * (dot(Ai, Uj) / g);
    }

    return(newDiag * scale);
}

#else

TMReal LeftHouseholder(TMat &A, TMat &U, const int i)
// Zeroes out those elements below the diagonal in column i by use of a
// Householder transformation matrix H: A' = HA. U is replaced with UH,
// so that U'A' = UH HA = UA
{
    int     j, k;
    TMReal  scale, vSqrLen, oldDiag, newDiag, g, dotProd;
    
    CL_ASSERT_MSG(i < A.Rows(), "bad i");

    // scale factor is the sum of abs of elements below and including
    // the diagonal in column i
    scale = 0.0;
    for (k = i; k < A.Rows(); k++)
        scale += fabs(A[k][i]);
        
    // return if nothing to eliminate...
    if (scale == 0.0)
        return(0.0);

    // scale the column, find sum of squares
    vSqrLen = 0.0;
    for (k = i; k < A.Rows(); k++)
    {
        A[k][i] /= scale;
        vSqrLen += sqr(A[k][i]);
    }
    
    oldDiag = A[i][i];          // v[0]
    newDiag = sqrt(vSqrLen);    // c
    if (oldDiag > 0.0)
        newDiag = -newDiag;
        
    g = vSqrLen - oldDiag * newDiag;    // c(c - v0)

    // replace column i of A with w
    A[i][i] = oldDiag - newDiag;        // v[0] -= c

    // Apply H to A by transforming remaining columns of A
    for (j = i + 1; j < A.Cols(); j++)
    {
        // Hv = v - w(v.w) / g

        // find dot product of the columns [i:i..m] (w) and [j:i..m]
        dotProd = 0.0;
        for (k = i; k < A.Rows(); k++)
            dotProd += A[k][j] * A[k][i];

        dotProd /= g;

        // calculate A's new column j
        for (k = i; k < A.Rows(); k++)
            A[k][j] -= A[k][i] * dotProd;
    }

    // Apply H to rows of U
    for (j = 0; j < A.Rows(); j++)  
    {
        // vH = v - (v.w)w / g

        dotProd = 0.0;
        for (k = i; k < A.Rows(); k++)
            dotProd += A[k][i] * U[j][k];

        dotProd /= g;

        for (k = i; k < A.Rows(); k++)
            U[j][k] -=  A[k][i] * dotProd;
    }

    return(newDiag * scale);
}

#endif

TMReal RightHouseholder(TMat &A, TMat &V, const int i)
// Zeroes out those elements to the right of the super-diagonal in row i
// by use of a Householder transformation matrix H: A' = AH. V is
// replaced with VH, so that A'V't = AH (HV)t = AVt
{
    int     j, k;
    TMReal  scale, vSqrLen, oldSuperDiag, newSuperDiag, g, dotProd;
    
    CL_ASSERT_MSG(i < A.Cols() - 1, "bad i");
    
    // scale factor is the sum of abs of elements to the right of the
    // diagonal in row i
    scale = 0.0;
    for (k = i + 1; k < A.Cols(); k++)
        scale += fabs(A[i][k]);

    // return if nothing to eliminate...
    if (scale == 0.0)
        return(0.0);

    vSqrLen = 0.0;
    for (k = i + 1; k < A.Cols(); k++)
    {
        A[i][k] /= scale;
        vSqrLen += sqr(A[i][k]);    
    }
    
    oldSuperDiag = A[i][i + 1];
    newSuperDiag = sqrt(vSqrLen);
    if (oldSuperDiag > 0.0)
        newSuperDiag = -newSuperDiag;
        
    g = oldSuperDiag * newSuperDiag - vSqrLen;
    A[i][i + 1] = oldSuperDiag - newSuperDiag;

    // transform the remaining rows below i
    for (j = i + 1; j < A.Rows(); j++)  
    {
        dotProd = 0.0;
        for (k = i + 1; k < A.Cols(); k++)
            dotProd += A[i][k] * A[j][k];

        dotProd /= g;

        for (k = i + 1; k < A.Cols(); k++)
            A[j][k] += A[i][k] * dotProd;
    }

    // Accumulate the transform in V
    for (j = 1; j < A.Cols(); j++)  
    {
        dotProd = 0.0;
        for (k = i + 1; k < A.Cols(); k++)
            dotProd += A[i][k] * V[j][k];

        dotProd /= g;

        for (k = i + 1; k < A.Cols(); k++)
            V[j][k] += A[i][k] * dotProd;       
    }

    // return new super-diagonal element A[i][i+1]
    return(newSuperDiag * scale);
}

#endif

void Bidiagonalize(TMat &A, TMat &U, TMat &V, TVec &diagonal, TVec &superDiag)
// bidiagonalize matrix A by using householder transformations to eliminate
// columns below the diagonal and rows to the right of the super-diagonal.
// A is modified, and the diagonal and superDiag set.
{
    int     i;
    TMat    Us;
        
    CL_ASSERT_MSG(A.Rows() >= A.Cols(), "matrix must have rows >= cols");

    diagonal.SetSize(A.Cols());
    superDiag.SetSize(A.Cols() - 1);
    U.SetSize(A.Rows(), A.Cols());
    Us.SetSize(A.Rows(), A.Rows());
    V.SetSize(A.Cols(), A.Cols());
    Us = vl_I;
    V = vl_I;
    
    for (i = 0; i < A.Cols(); i++)
    {
        diagonal[i] = LeftHouseholder(A, Us, i);

        if (i < A.Cols() - 1)
            superDiag[i] = RightHouseholder(A, V, i);
    }

    U = sub(Us, 0, 0, A.Rows(), A.Cols());
}
    
#ifndef VL_MIXED
TMReal QRFactorization(TMat &A, TMat &Q, TMat &R)
// Factor A into an orthogonal matrix Q and an upper-triangular matrix R.
// Destroys A.
{
    TMReal  normAcc = 0.0, diagElt;
    int     i, j;
    TMat    Qs;
        
    CL_ASSERT_MSG(A.Rows() >= A.Cols(), "matrix must have rows >= cols");

    Qs.SetSize(A.Rows(), A.Rows());
    Q.SetSize(A.Rows(), A.Cols());
    R.SetSize(A.Cols(), A.Cols());
    Qs = vl_I;
    R = vl_0;
    
    // for each column
    for (i = 0; i < A.Cols(); i++)
    {
        // apply householder
        diagElt = LeftHouseholder(A, Qs, i);
        // copy over row i of A to R
        R[i][i] = diagElt;
        j = A.Cols() - i - 1;
        if (j)
            last(R[i], j) = last(A[i], j);
        normAcc = std::max<TMReal>(normAcc, fabs(diagElt));
    }

    Q = sub(Qs, 0, 0, A.Rows(), A.Cols());

    return(normAcc);
}
#endif

/*
    NOTE
    
    Givens rotations perform a rotation in a 2D plane. Can be
    used to zero an entry of a matrix.
    
    We pick axes i and j, then form G such that it is basically
    I with entries:

        i    j
    i:  c   -s
         ...
    j:  s    c


    s^2 + c^2 = 1
    inv(G) = Gt

    A' = GA
        modifies rows i and j only:
            row'[i] = c * row[i] - s * row[j]
            row'[j] = c * row[j] + s * row[i]
        can force:
        A'[i][j] = 0 
            if c * A[i][j] - s * A[j][j] = 0
            if c = A[j][j], s = A[i][j]
            must normalize to retain identity:
                norm = sqrt(c^2 + s^2), c /= norm, s /= norm
            A'[j][j] = c * (norm * c) + s * (norm * s) = norm
        A'[i][i] = 0 
            if c * A[i][i] - s * A[j][i] = 0
            if c = A[j][i], s = A[i][i]

    A' = AGt


*/

#ifndef VL_MIXED
void RotateRight(TMat& U, const int i, const int j, const TMReal c, const TMReal s)
// rotate U by the given Givens rotation: U' = UGt
// where G is defined as above
{
    TMVec   temp;
    
    temp = col(U, i);
    col(U, i) =  c * col(U, i) - s * col(U, j);
    col(U, j) =  c * col(U, j) + s * temp;
}
#endif

void ClearSuperEntry
(
    TVec &diagonal, 
    TVec &superDiag, 
    TMat &U, 
    const int k, 
    const int l, 
    const TMReal precision
)
// We have a zero diagonal element at diag[l]. This means we can zero
// out the super-diagonal entry to its right with a series of rotations.
// Each rotation clears an entry (starting with the original
// super-diagonal entry) but creates another entry immediately to its
// right which must in turn be zeroed. Eventually we run off the end of
// the matrix, and the process terminates.
{
    TMReal  c = 0.0, s = 1.0;
    TMReal  f;
    int     i;
    TMReal  norm;

    // We zero the superdiagonal element l by using the row immediately
    // below it in a Givens rotation. Unfortunately, the superdiagonal
    // in this row in turn creates another entry A[l][l+2]. We must keep
    // applying rotations in the plane of the axes l and l + n to keep
    // zeroing each new entry created until we reach the right hand side
    // of the matrix.

    // initialise: start with f being the original super diagonal entry
    // we're eliminating
    f = superDiag[l];
    superDiag[l] = 0.0;

    // at each stage, f = A[l][i], rotate in the l/i plane
    for (i = l + 1; true; i++)
    {
        if (fabs(f) + precision == precision)  // if f is zero, we don't have to eliminate further
            break;

        // to eliminate f at each stage we pick s = -f, c = di
        s = -f;
        c = diagonal[i];

        // normalize
        norm = sqrt(sqr(c) + sqr(s));       
        c /= norm;
        s /= norm;

        // apply inverse rotation to U
        RotateRight(U, i, l, c, s);

        // do the rotation, zeroing f and creating f' immediately to its right
        diagonal[i] = norm;     // di' = c * di - s * f = norm
        if (i == k)             // done?
            break;
        f = s * superDiag[i];   // f'  = c * 0  + s * superdiag[i]
        superDiag[i] *= c;      // ei' = c * ei - s * 0
    }
}

int FindSplit(TVec &diagonal, TVec &superDiag, TMat &U, const int k, const TMReal prec)
// Check for a zero entry along the superdiagonal; if we find one, we can
// QR iterate on two separate matrices above and below it.
// If there is a zero on the *diagonal*, we can call ClearSuperEntry to
// zero out the corresponding superdiagonal entry to its right.
{
    int     l;
    
    for (l = k - 1; l >= 0; l--)
        if (superDiag[l] + prec == prec)
            // can split here
            return(l + 1);
        else if (diagonal[l] + prec == prec)
        {
            // can create a split here
            ClearSuperEntry(diagonal, superDiag, U, k, l, prec);
            return(l + 1);
        }

    return(0);
}

void Diagonalize(TVec &diagonal, TVec &superDiag, TMat &U, TMat &V)
// Diagonalise the bidiagonal matrix A = (diagonal, superDiag), accumulating 
// transforms into U on the left and Vt on the right.
//
// diag(A) = diagonal and diag(A, 1) = superDiag, that is to say, diagonal[0]
// is A[0][0], and superDiag[0] is A[0][1].
{
    int     i, k, l;
    TMReal  prec;
    
    // work out a good precision value
    prec = fabs(diagonal[0]);
    for (i = 1; i < diagonal.Elts(); i++)
        prec = std::max<TMReal>(prec, fabs(diagonal[i]) + fabs(superDiag[i - 1]));

    // work our way up from the bottom of the matrix
    for (k = diagonal.Elts() - 1; k >= 0; k--)
    {
        while (1)
        {
            // if there's a split, start from there rather than A[0][0]
            l = FindSplit(diagonal, superDiag, U, k, prec);
            
            // are we done?
            if (l == k)     // last super-diag entry must be zero -- what we wanted.
                break;

            // QR iterate over the sub-matrix of A, A[l..k][l..k], until
            // we've nuked super diagonal entry A[k - 1][k]

            // For extra stability, we shift the QR computation.  We
            // want the shift to be as close as possible to the largest
            // eigenvalue, which of course we don't know yet... so we
            // take the eigenvalues of the 2x2 matrix at the bottom of
            // our window of A, and use those.

            TMReal  shift;
            TMReal  e0;
            TMReal  e1 = superDiag[k - 1];
            TMReal  d1 = diagonal[k - 1];
            TMReal  d2 = diagonal[k];
            TMReal  dl = diagonal[l];

            if (k > 1)
                e0 = superDiag[k - 2];
            else
                e0 = 0.0;
                
            //      d0  e0
            //          d1  e1
            //   k:         d2

            shift = (d1 - d2) * (d1 + d2) + (e0 - e1) * (e0 + e1);
            shift /= TMReal(2.0 * e1 * d1);
            shift += TMReal(sign(shift) * sqrt(sqr(shift) + 1));
            shift = ((dl - d2) * (dl + d2) + e1 * (d1 / shift - e1)) / dl;


            TMReal      cos_th, sin_th, cos_ph, sin_ph;
            TMReal      d1n, norm, e2, f1, g0, g1, e1n;
            
            d1 = dl;    // d1 == A[i - 1][i - 1]: initialise it to that
            e1 = superDiag[l];  
            // the first rotation is the QR-shifted one. After that, we
            // are continually eliminating below-diagonal and
            // above-super diagonal elements created by the previous
            // rotation, until we hit the bottom-right of the matrix and
            // we have a bidiagonal matrix again. The QR-shift will
            // ensure that the new matrix has smaller super-diagonal
            // elements, however.
            g0 = e1;
            e0 = shift;

            for (i = l + 1; true; i++)
            {
                // rotate in plane of axes i - 1, i:
                //    d0  e0  g0      d0  e0'  0
                //        d1  e1   ->     d1'  e1'
                //  i:        d2          f1   d2'
                // we are rotating on the right, and so affecting columns
                d2 = diagonal[i];

                // eliminate g0 using e0 (except for the first iteration,
                // where e0 and g0 would be off the top of the matrix, and
                // we're performing the QR-shift.)
                sin_th = g0;
                cos_th = e0;

                norm = sqrt(sqr(cos_th) + sqr(sin_th));
                cos_th /= norm;
                sin_th /= norm;

                // perform the rotation
                if (i > 1)
                    superDiag[i - 2] = norm;    // e0' = cos_th * e0 + sin_th * g0
                d1n = cos_th * d1 + sin_th * e1;    
                e1n = -sin_th * d1 + cos_th * e1;

                f1 = d2 * sin_th;           // f1  = c *  0 + s * d2
                d2 *= cos_th;               // d2' = c * d2 + s * 0

                // Accumulate rotation in V
                RotateRight(V, i, i - 1, cos_th, sin_th);  

                // in eliminating g0, we've created f1: eet must be
                // destroyed

                // rotate in plane of axes i - 1, i:
                //      d0  e0             d0   e0
                //      d1  e1      ->     d1'  e1'  g1'
                //  i:  f1  d2  e2         0    d2'  e2'
                // we are rotating on the left, and so affecting rows

                // standard rotation to eliminate the f1 we've just created
                cos_ph = d1n;
                sin_ph = f1;
                
                norm = sqrt(sqr(cos_ph) + sqr(sin_ph));

                if (norm == 0.0)
                {
                    // rotation angle is arbitrary
                    cos_ph = cos_th;
                    sin_ph = sin_th;
                }
                else
                {
                    cos_ph /= norm;
                    sin_ph /= norm;
                }
                
                // as usual, for the elimination pair, f1 = 0, d1 = norm 
                diagonal[i - 1] = norm;     // d1'
                // rotate e1 and d2
                e1 =  cos_ph * e1n + sin_ph * d2;
                d2 = -sin_ph * e1n + cos_ph * d2;
                
                // Accumulate rotation into U
                RotateRight(U, i, i - 1, cos_ph, sin_ph);

                if (i == k)
                    break;
                    
                // rotate g1 and e2
                e2 = superDiag[i];
                g1 = sin_ph * e2;
                e2 = cos_ph * e2;

                d1 = d2;        // the new d1 will be the old d2
                e0 = e1;
                e1 = e2;
                g0 = g1;
            }

            if (l > 0) 
                superDiag[l - 1] = 0.0;     // Supposed to be eliminated by now
            superDiag[k - 1] = e1;          // write in the last superdiagonal
            diagonal[k] = d2;               // and diagonal
        }       
        
        // Force singular value to be +ve if needs be
        if (diagonal[k] < 0)        
        {
            diagonal[k] = -diagonal[k];
            // flip the corresponding row of Vt to balance out
            col(V, k) = -col(V, k);
        }
    }
}

void SVDFactorization(TMat &A, TMat &U, TMat &V, TVec &diagonal)
{
    TVec    superDiag;

    // Find the bidiagonal matrix, and then eliminate the
    // elements above the diagonal to get the final
    // result.  
    Bidiagonalize(A, U, V, diagonal, superDiag);
    Diagonalize(diagonal, superDiag, U, V);
}




#ifdef UNFINISHED
// in progress

// Factors sym. pos def. matrix a into uTu.
bool Cholesky(const Mat4d& a, Mat4d* result)
{
    Mat4d& u = *result;
    const int n = 4;

    for (int i = 0; i < n; i++)
    {
        for (int j = i; j < n; j++)
        {
            float sum = a(i, j);
            
            for (int k = i - 1; k >= 0; k--) 
                sum -= u(k, i) * u(k, j);
                
            if (i == j)
            {
                if (sum <= 0.0)
                    return false;

                u(i, i) = sqrt(sum);
            }
            else
            {
                u(i, j) = sum / u(i, i);
                u(j, i) = 0;
            }
        }
    }

    return true;
}

bool Cholesky(const Mat3d& a, Mat3d* result)
{
    Mat3d& u = *result;
    const int n = 3;

    for (int i = 0; i < n; i++)
    {
        for (int j = i; j < n; j++)
        {
            float sum = a(i, j);
            
            for (int k = i - 1; k >= 0; k--) 
                sum -= u(k, i) * u(k, j);
                
            if (i == j)
            {
                if (sum <= 0.0)
                    return false;

                u(i, i) = sqrt(sum);
            }
            else
            {
                u(i, j) = sum / u(i, i);
                u(j, i) = 0;
            }
        }
    }

    return true;
}

Vec3d BacksolveUtU(const Mat3d& U, const Vec3d& b)
/// Solve (UtU) x = b, assuming U is upper triangular
{
    Vec3d r1, r2;
    
    // solve U_t x' = b
    r1[0] = (b[0]) / U(0, 0);
    r1[1] = (b[1] - r1[0] * U(0, 1)) / U(1, 1);
    r1[2] = (b[2] - r1[0] * U(0, 2) - r1[1] * U(1, 2)) / U(2, 2);
    
    // solve U x = x'
    r2[2] = (r1[2]) / U(2, 2);
    r2[1] = (r1[1] - r2[2] * U(1, 2)) / U(1, 1);
    r2[0] = (r1[0] - r2[1] * U(0, 1) - r2[2] * U(0, 2)) / U(0, 0);
    
    return r2;
}

Vec3d BacksolveUt(const Mat3d& U, const Vec3d& b)
{
    Vec3d r;
    
    // solve U_t x' = b
    r[0] = (b[0]) / U(0, 0);
    r[1] = (b[1] - r[0] * U(0, 1)) / U(1, 1);
    r[2] = (b[2] - r[0] * U(0, 2) - r[1] * U(1, 2)) / U(2, 2);
    
    return r;
}

Vec3d BacksolveU(const Mat3d& U, const Vec3d& b)
{
    Vec3d r;
    
    // solve U x = x'
    r[2] = (b[2]) / U(2, 2);
    r[1] = (b[1] - r[2] * U(1, 2)) / U(1, 1);
    r[0] = (b[0] - r[1] * U(0, 1) - r[2] * U(0, 2)) / U(0, 0);
    
    return r;
}

Vec3d BacksolveL(const Mat3d& L, const Vec3d& b)
{
    Vec3d r;
    
    // solve L x' = b
    r[0] = (b[0]) / L(0, 0);
    r[1] = (b[1] - r[0] * L(0, 1)) / L(1, 1);
    r[2] = (b[2] - r[0] * L(0, 2) - r[1] * L(1, 2)) / L(2, 2);
    
    return r;
}
#endif

#ifdef UNFINISHED

/*
    Rewrite: v_t A v  + 2v_tb + c = e
    value at origin = c
    v_t A v is quadric about the origin.
    offset in direction b...
*/

Mat3d RandomRotation()
{
    Mat3d result;
    MakeRandomRotation(RandomVector(), result);
    return result;
}

void TestC()
{
    Mat4d q;
    
    Vec4d p0(1, 0, 0, -1);  // yz plane through x=1
    Vec4d p1(0, 1, 0, -3);  // xz plane through y=1
    Vec4d p2(0, 0.5, 1, -2);  // xy plane through z=1
    
    q  = oprod(p0, p0);
    q += oprod(p1, p1);
    q += oprod(p2, p2);
    // q = q * q;
    
    
    
    //q = Mat4d(1, 2, 3, 4, 0, 1, 2, 3, 0, 0, 1, 2, 0, 0, 0, 1);
    // q = vl_I;
    // q *= 4;
    // q(0, 3) = 1;
    //q(3, 0) = 1;
//    q = trans(q) * q;
    cout << q << endl;
//    q = vl_I;
//    q(3, 3) = -1;
    Mat4d U;
    if (Cholesky(q, &U))
    {
        cout << U << endl;
        cout << trans(U) * U << endl;
    
    }
    else
        cout << "can't factor" << endl;

    Mat3d m0(q(0, 0), q(0, 1), q(0, 2), 
            q(1, 0), q(1, 1), q(1, 2), 
            q(2, 0), q(2, 1), q(2, 2));
            
    Mat3d m(RandomRotation() * m0 + RandomRotation() * m0 + RandomRotation() * m0);

    Mat3d U3;
    if (Cholesky(m, &U3))
    {
        cout << "factor3:" << endl;
        cout << U3 << endl;
        cout << trans(U3) * U3 << endl;
        
        Vec3d v(-q(0, 3), -q(1, 3), -q(2, 3));
        Vec3d c = BacksolveUtU(U3, v);
    
        cout << "centre: " << c << endl;
    }
    else
        cout << "can't factor3" << endl;
        
    if (fabs(det(m)) > 1e-3)
    {
        Vec3d v(-q(0, 3), -q(1, 3), -q(2, 3));
        
        Vec3d c = inv(m) * v;
        cout << "centre: " << c << endl;
    }
    else
        cout << "no centre" << endl;
}

#endif
