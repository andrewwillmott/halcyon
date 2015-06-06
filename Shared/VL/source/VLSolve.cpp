/*
    File:           VLSolve.cpp

    Function:       Implements linear system solvers

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLSolve.h"

/** Solves Ax = b via gaussian elimination.

    Each iteration modifies the current approximate solution x.
    Omega controls overrelaxation: omega = 1 gives Gauss-Seidel,
    omega somewhere beteen 1 and 2 gives the fastest convergence.

    x is the initial guess on input, and solution vector on output.

    If steps is zero, the routine iterates until the residual is
    less than epsilon. Otherwise, it performs at most *steps 
    iterations, and returns the actual number of iterations performed
    in *steps.

    Returns approximate squared length of residual vector: |Ax-b|^2.

    [Strang, "Introduction to Applied Mathematics", 1986, p. 407]
*/

TMReal SolveOverRelax
(
    const TMat&     A,
    TVec&           x,
    const TVec&     b,
    TMReal          epsilon,
    TMReal          omega,
    int*            steps
)
{
    int     i, j, jMax;
    TMReal  sum;
    TMReal  diagonal, xOld, error;

    CL_ASSERT_MSG(A.IsSquare(), "(SolveOverRelax) Matrix not square");
    j = 0;

    if (steps)
        jMax = *steps;
    else
        jMax = VL_MAX_STEPS;
        
    do
    {
        error = 0.0;

        for (i = 0; i < A.Rows(); i++)
        {
            sum = b[i] - dot(A[i], x);      
            diagonal = A[i][i];
            sum += diagonal * x[i];     
            // I.e., sum = b[i] - (A[i] * x - A[i,i])
            xOld = x[i];
            
            if (diagonal == 0)
                CL_WARNING("(SolveOverRelax) diagonal element = 0");
            else if (omega == 1.0)  // Gauss-Seidel 
                x[i] = sum / diagonal;
            else                    // Overrelax
                x[i] = lerp(xOld, sum / diagonal, (Real) omega); 
                
            sum -= diagonal * xOld;
            error += sqr(sum);
        }
        j++;
    }
    while (error > epsilon && j < jMax);

    if (steps)
        *steps = j;

    return(error);
}

/** Solves Ax = b via gaussian elimination for a sparse matrix.

    See the dense version above for details.
*/

TMReal SolveOverRelax
(
    const TSparseMat&   A,
    TVec&               x,
    const TVec&         b,
    TMReal              epsilon,
    TMReal              omega,
    int*                steps
)
{
    int     i, j, jMax;
    TMReal  sum;
    TMReal  diagonal, xOld, error;

    CL_ASSERT_MSG(A.IsSquare(), "(SolveOverRelax) Matrix not square");

    j = 0;
    if (steps)
        jMax = *steps;
    else
        jMax = VL_MAX_STEPS;
        
    do
    {
        error = 0.0;
        
        for (i = 0; i < A.Rows(); i++)
        {
            // sum = b[i] - (A[i] dot x - A[i,i])            
        
            sum = b[i] - dot(A[i], x);      
            diagonal = A[i].Get(i);
            sum += diagonal * x[i];     
            
            xOld = x[i];
            
            if (diagonal == 0)
                CL_WARNING("(SolveOverRelax) diagonal element = 0");
            else if (omega == 1)
                x[i] = sum / diagonal;  // Gauss-Seidel 
            else
                x[i] = lerp(xOld, sum / diagonal, (Real) omega); 
                
            sum -= diagonal * xOld;
            error += sqr(sum);
        }
        j++;
    }
    while (error > epsilon && j < jMax);

    if (steps)
        *steps = j;
        
    return(error);
}


/**
    Solve Ax = b by conjugate gradient method, for symmetric, positive
    definite A.
    
    x is the initial guess on input, and solution vector on output.

    Returns squared length of residual vector.

    If A is not symmetric, this will solve the system (A + At)x/2 = b

    [Strang, "Introduction to Applied Mathematics", 1986, p. 422]
*/


TMReal SolveConjGrad
(
    const TMat&      A,             // Solve Ax = b.
    TVec&            x,
    const TVec&      b,
    TMReal           epsilon,       // how low should we go?
    int*             steps          // iterations to converge.
)
{
    CL_ASSERT_MSG(A.IsSquare(), "(SolveConjGrad) Matrix not square");
    
    TVec r(A.Rows());        // Residual vector, b - Ax 
    TVec t(A.Rows());        // temp!

    // r = b - A * x;
    Multiply(A, x, t);
    Subtract(b, t, r);
    
    TMReal rSqrLen = sqrlen(r);
    int i = 0;
            
    if (rSqrLen > epsilon)
    {
        TVec d(r);
        
        int iMax;
        if (steps)
            iMax = *steps;
        else
            iMax = VL_MAX_STEPS;
            
        while (i < iMax)    
        {   
            i++;
            // t = A * d;      
            Multiply(A, d, t);
            TMReal u = dot(d, t); 
            
            if (len(u) < TMReal(1e-12))
            {
                CL_WARNING("(SolveConjGrad) d'Ad = 0");
                break;
            }
            
            TMReal alpha = rSqrLen / u;        // How far should we go?            
            // x += alpha * d;             // Take a step along direction d
            MultiplyAccum(d,  alpha, x);
            
            if (i & 0x3F)
                // r -= alpha * t; 
                MultiplyAccum(t, -alpha, r);
            else
            {
                // For stability, correct r every 64th iteration
                //r = b - A * x;
                Multiply(A, x, t);
                Subtract(b, t, r);
            }
                
            TMReal rSqrLenOld = rSqrLen;
            rSqrLen = sqrlen(r); 
            
            if (rSqrLen <= epsilon)
                break;                  // Converged! Let's get out of here
            
            TMReal beta = rSqrLen / rSqrLenOld;
            // d = r + beta * d;           //  Change direction
            d *= beta;       
            d += r;
        }
    }
    
    if (steps)
        *steps = i;
    
    return(rSqrLen);
}

TMReal SolveConjGrad_AtA
(
    const TMat&      A,             // Solve Ax = b.
    TVec&            x,
    const TVec&      b,
    TMReal           epsilon,       // how low should we go?
    int*             steps          // iterations to converge.
)
{
    TVec    r (A.Cols());        // Residual vector, Atb - AtAx 
    TVec    t (A.Cols());        // temp
    TVec    t2(A.Rows());        // temp

    // r = Atb;
    Multiply(b, A, r);
    //r -= At A * x;             
    Multiply(A, x, t2);
    Multiply(t2, A, t); // tmp_t A = trans(A_t tmp)
    Subtract(r, t, r);
    
    TMReal rSqrLen = sqrlen(r);
    
    int i = 0;
    
    if (rSqrLen > epsilon)              // If we haven't already converged...
    {
        TVec d(r);
        
        int iMax;
        if (steps)
            iMax = *steps;      
        else
            iMax = VL_MAX_STEPS;

        while (i < iMax)
        {   
            i++;
            // t = AtA * d;
            Multiply(A, d, t2);
            Multiply(t2, A, t);
            TMReal u = dot(d, t);
            
            if (u == 0.0)
            {
                CL_WARNING("(SolveConjGrad) d'Ad = 0");
                break;
            }
            
            TMReal alpha = rSqrLen / u;        // How far should we go?
            // x += alpha * d;                 // Take a step along direction d
            // r -= alpha * t; 
            MultiplyAccum(d,  alpha, x);
            MultiplyAccum(t, -alpha, r);

            TMReal rSqrLenOld = rSqrLen;
            rSqrLen = sqrlen(r); 
            
            if (rSqrLen <= epsilon)
                break;                  // Converged! Let's get out of here
            
            TMReal beta = rSqrLen / rSqrLenOld;
            // d = r + beta * d;           //  Change direction
            Multiply(d, beta, d);
            Add(d, r, d);
        }
    }
    
    if (steps)
        *steps = i;
    
    return(rSqrLen);
}


/** Solves Ax = b via conjugate gradient for a sparse matrix.

    See the dense version above for details.
*/


TMReal SolveConjGrad
(
    const TSparseMat&   A,
    TVec&               x,
    const TVec&         b,
    TMReal              epsilon,
    int*                steps
)
{
    CL_ASSERT_MSG(A.IsSquare(), "(SolveConjGrad) Matrix not square");

    TVec    r(b.Elts());        // Residual vector, b - Ax 
    TVec    t(b.Elts());

    //r = b - A * x;
    Multiply(A, x, t);
    Subtract(b, t, r);
    
    TMReal rSqrLen = sqrlen(r);
    
    int i = 0;
    if (rSqrLen > epsilon)              // If we haven't already converged...
    {
        TVec d(r);
        
        int iMax;
        if (steps)
            iMax = *steps;      
        else
            iMax = VL_MAX_STEPS;
            
        while (i < iMax)
        {   
            i++;
            // t = A * d;      
            Multiply(A, d, t);
            TMReal u = dot(d, t);
            
            if (u == 0.0)
            {
                CL_WARNING("(SolveConjGrad) d'Ad = 0");
                break;
            }
            
            TMReal alpha = rSqrLen / u;        // How far should we go?
            // x += alpha * d;                    // Take a step along direction d
            // r -= alpha * t; 
            MultiplyAccum(d,  alpha, x);
            MultiplyAccum(t, -alpha, r);

            TMReal rSqrLenOld = rSqrLen;
            rSqrLen = sqrlen(r); 
            
            if (rSqrLen <= epsilon)
                break;                  // Converged! Let's get out of here
            
            TMReal beta = rSqrLen / rSqrLenOld;
            // d = r + beta * d;           //  Change direction
            d *= beta;
            d += r;
        }
    }

    if (steps)
        *steps = i;
    
    return(rSqrLen);
}


TMReal SolveConjGrad_AtA
(
    const TSparseMat&   A,
    TVec&               x,
    const TVec&         b,
    TMReal              epsilon,
    int*                steps
)
{
    TVec    r (A.Cols());        // Residual vector, Atb - AtAx 
    TVec    t (A.Cols());        // temp
    TVec    t2(A.Rows());        // temp

    // r = Atb;
    Multiply(b, A, r);
    //r -= At A * x;             
    Multiply(A, x, t2);
    Multiply(t2, A, t); // tmp_t A = trans(A_t tmp)
    Subtract(r, t, r);
    
    TMReal rSqrLen = sqrlen(r);
    int i = 0;

    if (rSqrLen > epsilon)              // If we haven't already converged...
    {
        TVec d = r;                      
        int iMax;
        if (steps)
            iMax = *steps;      
        else
            iMax = VL_MAX_STEPS;
            
        while (i < iMax)
        {   
            i++;
            // t = AtA * d;
            Multiply(A, d, t2);
            Multiply(t2, A, t);
            TMReal u = dot(d, t);
            
            if (u == 0.0)
            {
                CL_WARNING("(SolveConjGrad) d'Ad = 0");
                break;
            }
            
            TMReal alpha = rSqrLen / u;        // How far should we go?
            // x += alpha * d;                 // Take a step along direction d
            // r -= alpha * t; 
            MultiplyAccum(d,  alpha, x);
            MultiplyAccum(t, -alpha, r);

            TMReal rSqrLenOld = rSqrLen;
            rSqrLen = sqrlen(r); 
            
            if (rSqrLen <= epsilon)
                break;                  // Converged! Let's get out of here
            
            TMReal beta = rSqrLen / rSqrLenOld;
            // d = r + beta * d;           //  Change direction
            Multiply(d, beta, d);
            Add(d, r, d);
        }
    }
    
    if (steps)
        *steps = i;
    
    return(rSqrLen);
}
