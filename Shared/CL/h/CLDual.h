//
//  File:       CLDual.h
//
//  Function:   Dual number implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//
//  Notes:      See Gino van den Bergen's presentations for more info
//

#ifndef CL_DUAL_H
#define CL_DUAL_H

#include <VL234f.h>
#include <cmath>

namespace nCL
{
    // d = a + b ε, ε^2 = 0
    struct cDual
    {
        typedef float tReal;

        cDual() = default;
        cDual(tReal real, tReal dual = tReal(0)) : mReal(real), mDual(dual) {}

        tReal mReal = tReal(0);       // { real, dual }
        tReal mDual = tReal(0);

        cDual&    operator += (const cDual& a);
        cDual&    operator -= (const cDual& a);
        cDual&    operator *= (const cDual& a);
        cDual&    operator *= (float s);
        cDual&    operator /= (const cDual& a);
        cDual&    operator /= (float s);

        // Comparison operators
        bool     operator == (const cDual& a) const; // v == a?
        bool     operator != (const cDual& a) const; // v != a?

        // Arithmetic operators
        cDual    operator + (const cDual& a) const;  // v + a
        cDual    operator - (const cDual& a) const;  // v - a
        cDual    operator - () const;                // -v
        cDual    operator * (const cDual& a) const;  // v * a (vx * ax, ...)
        cDual    operator * (float s) const;        // v * s
        cDual    operator / (const cDual& a) const;  // v / a (vx / ax, ...)
        cDual    operator / (float s) const;        // v / s
    };

    inline cDual& cDual::operator += (const cDual& v)
    {
        mReal += v.mReal;
        mDual += v.mDual;
        
        return *this;
    }

    inline cDual& cDual::operator -= (const cDual& v)
    {
        mReal -= v.mReal;
        mDual -= v.mDual;

        return *this;
    }

    inline cDual& cDual::operator *= (const cDual& v)
    {
        mDual *= v.mReal;
        mDual += mReal * v.mDual;
        mReal *= v.mReal;

        return *this;
    }

    inline cDual& cDual::operator *= (tReal s)
    {
        mReal *= s;
        mDual *= s;
        
        return *this;
    }

    inline cDual& cDual::operator /= (const cDual& v)
    {
        mReal /= v.mReal;
        mDual /= v.mReal;
        mDual -= (mReal / v.mReal) * v.mDual;

        return *this;
    }

    inline cDual& cDual::operator /= (tReal s)
    {
        mReal /= s;
        mDual /= s;

        return *this;
    }

    inline cDual cDual::operator + (const cDual& v) const
    {
        return { mReal + v.mReal, mDual + v.mDual };
    }

    inline cDual cDual::operator - (const cDual& v) const
    {
        return { mReal - v.mReal, mDual - v.mDual };
    }

    inline cDual cDual::operator - () const
    {
        return { -mReal, -mDual };
    }

    inline cDual cDual::operator * (const cDual& v) const
    {
        return { mReal * v.mReal, mReal * v.mDual + mDual * v.mReal };
    }

    inline cDual cDual::operator * (TVReal s) const
    {
        return { mReal * s , mDual * s};
    }

    inline cDual cDual::operator / (const cDual& v) const
    {
        return { mReal / v.mReal, (mDual * v.mReal - mReal * v.mDual) / sqr(v.mReal) };
    }


    // f(d) = f(a) + b f'(a) ε
    using std::sqrt;
    using ::sqr;
    using std::exp;
    using std::log;

    using std::sin;
    using std::cos;
    using std::tan;

    using std::asin;
    using std::acos;
    using std::atan;

    inline cDual sqrt(cDual d)
    {
        cDual::tReal s = sqrt(d.mReal);
        return { s, cDual::tReal(0.5) * d.mDual / s };
    }
    inline cDual sqr(cDual d)
    {
        cDual::tReal s = sqr(d.mReal);
        return { s, cDual::tReal(2) * d.mDual * d.mReal };
    }
    inline cDual exp(cDual d)
    {
        cDual::tReal s = exp(d.mReal);
        return { s, d.mDual * s };
    }
    inline cDual log(cDual d)
    {
        return { log(d.mReal), d.mDual / d.mReal };
    }

    inline cDual sin(cDual d)
    {
        return { sin(d.mReal), d.mDual * cos(d.mReal) };
    }
    inline cDual cos(cDual d)
    {
        return { cos(d.mReal), -d.mDual * sin(d.mReal) };
    }
    inline cDual tan(cDual d)
    {
        cDual::tReal s = tan(d.mReal);
        return { s, d.mDual * (cDual::tReal(1) + sqr(s)) };
    }

    inline cDual asin(cDual d)
    {
        return { asin(d.mReal),  d.mDual / sqrt(cDual::tReal(1) - sqr(d.mReal)) };
    }
    inline cDual acos(cDual d)
    {
        return { acos(d.mReal), -d.mDual / sqrt(cDual::tReal(1) - sqr(d.mReal)) };
    }
    inline cDual atan(cDual d)
    {
        return { atan(d.mReal), d.mDual / (cDual::tReal(1) + sqr(d.mReal)) };
    }

    // Dual<Vec3> = point/tangent for curves
    // Vec3<Dual> = plucker coords, { q - p, p x q }
    //   da . db = { u, v }  v = 0 if lines intersect, +ve if cross right-handed, -ve left handed
    // translation: { u, v + t x u }
    // rotation: { Ru, Rv }
}

#endif
