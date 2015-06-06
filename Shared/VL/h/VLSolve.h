/*
    File:           VLSolve.h

    Function:       Contains routines for solving a system of linear equations.
                    Includes the overrelaxation (a more general version of 
                    Gauss Seidel) and conjugate gradient methods, for both
                    normal and sparse matrices.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Solve_H
#define VL_Solve_H

TMReal SolveOverRelax
(
    const TMat& A,
    TVec&       x,
    const TVec& b,
    TMReal      epsilon,
    TMReal      omega = 1.0,
    int*        steps = 0
);
///< Solves A x = b, if specified, *steps contains max iterations, and actual #
///< of iterations is returned. omega controls overrelaxation.
TMReal SolveOverRelax
(
    const TSparseMat&   A,
    TVec&               x,
    const TVec&         b,
    TMReal              epsilon,
    TMReal              omega = 1.0,
    int*                steps = 0
);
///< Solves A x = b, if specified, *steps contains max iterations, and actual #
///< of iterations is returned. omega controls overrelaxation.

TMReal SolveConjGrad(const TMat& A, TVec& x, const TVec& b, TMReal epsilon, int* steps = 0);
///< Solves A x = b. If specified, *steps contains max iterations, and actual #
///< of iterations is returned.
TMReal SolveConjGrad_AtA(const TMat& A, TVec& x, const TVec& b, TMReal epsilon, int* steps = 0);
///< Solves AtA x = At b, without having to form AtA
            
TMReal SolveConjGrad(const TSparseMat& A, TVec& x, const TVec& b, TMReal epsilon, int* steps = 0);
///< Solves AtA x = At b
TMReal SolveConjGrad_AtA(const TSparseMat& A, TVec& x, const TVec& b, TMReal epsilon, int* steps = 0);
///< Solves AtA x = At b, without having to form AtA
#endif
