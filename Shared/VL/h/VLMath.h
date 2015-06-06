/*
    File:           VLMath.h

    Function:       Various math definitions for VL
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_MATH_H
#define VL_MATH_H

#include <stdlib.h>

// --- Inlines ----------------------------------------------------------------

// additions to arithmetic functions

#ifdef VL_HAS_IEEEFP
#include <ieeefp.h>
#define vl_is_finite(X) finite(X)
#elif defined (__GNUC__) && defined(VL_USE_MISC)
#define vl_is_finite(X) finite(X)
#else
#define vl_is_finite(X) (1)
#endif

#ifdef VL_HAS_DRAND
    inline double vl_rand()
    { return(drand48()); }
    inline double vl_srand(Uint seed)
    { srand48(seed); }
#else
    inline double vl_rand()
    { return(rand() / (RAND_MAX + 1.0)); }
    inline void vl_srand(uint32_t seed)
    { srand(seed); }
#endif

#ifdef VL_COMPLEX
    #include <complex.h>
#endif

#ifdef VL_NO_CMATH
   #ifdef VL_HAS_ABSF
      inline float abs(float x)
      { return (fabsf(x)); }
   #endif
   inline double abs(double x)
   { return (fabs(x)); }
#endif

inline float len(float x)
{ return (fabsf(x)); }
inline double len(double x)
{ return (fabs(x)); }

template<class T_Value> inline T_Value sqr(T_Value x)
{ return(x * x); }

template<class T_Value> inline T_Value cube(T_Value x)
{ return(x * x * x); }

inline float sqrlen(float r)
{ return(sqr(r)); }
inline double sqrlen(double r)
{ return(sqr(r)); }

inline float lerp(float a, float b, float s)
{ return((1.0f - s) * a + s * b); }
inline double lerp(double a, double b, double s)
{ return((1.0 - s) * a + s * b); }

template<class T_Value>
    inline T_Value lerp(T_Value x, T_Value y, Real s)
    { return(x + (y - x) * s); }
    
template<>
    inline float lerp<float>(float x, float y, Real s)
    { return(float(x + (y - x) * s)); }

inline double sign(double d)
{
    if (d < 0)
        return(-1.0);
    else
        return(1.0);
}

inline double sinc(double x) 
{
  if (fabs(x) < 1.0e-6) 
    return 1.0;
  else 
    return(sin(x) / x);
}

inline float sincf(float x) 
{
  if (fabsf(x) < 1.0e-6f) 
    return 1.0f;
  else 
    return(sinf(x) / x);
}

#ifndef VL_HAVE_SINCOS
    inline void sincosf(double phi, double* sinv, double* cosv)
    {
        *sinv = sin(phi);
        *cosv = cos(phi);
    }
    inline void sincosf(float phi, float* sinv, float* cosv)
    {
        *sinv = sinf(phi);
        *cosv = cosf(phi);
    }
#endif

template<class T_Value> inline T_Value clip(T_Value x, T_Value min, T_Value max)
{
    if (x <= min)
        return(min);
    else if (x >= max)
        return(max);
    else
        return(x);
}

// useful routines

inline void SetReal(float &a, double b)
{ a = float(b); }
inline void SetReal(double &a, double b)
{ a = b; }

template <class S, class T> inline void ConvertVec(const S &u, T &v)
{
    for (int i = 0; i < u.Elts(); i++)
        v[i] = u[i];
}

template <class T> inline void ConvertVec(const T &u, T &v)
{ v = u; }

template <class S, class T> inline void ConvertMat(const S &m, T &n)
{
    for (int i = 0; i < m.Rows(); i++)
        ConvertVec(m[i], n[i]);
}

template <class T> inline void ConvertMat(const T &m, T &n)
{ n = m; }


#endif
