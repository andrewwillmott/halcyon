/*
    File:       VLConstants.h

    Function:   Contains various constants for VL.

    Author:     Andrew Willmott

    Copyright:  (c) 1999-2000, Andrew Willmott
*/

#ifndef VLConstants_H
#define VLConstants_H

#include <math.h>


// --- Mathematical constants -------------------------------------------------

#ifdef M_PI
    const Real          vl_pi = M_PI;
    const Real          vl_halfPi = M_PI_2;
    const Real          vl_quarterPi = M_PI_4;
#elif defined(_PI)
    const Real          vl_pi = _PI;
    const Real          vl_halfPi = vl_pi / 2.0;
    const Real          vl_quarterPi = vl_pi / 4.0;
#else
    const Real          vl_pi = Real(3.14159265358979323846);
    const Real          vl_halfPi = Real(vl_pi / 2.0);
    const Real          vl_QuarterPi = Real(vl_pi / 4.0);
#endif

    const Real          vl_twoPi = Real(2) * vl_pi;

#ifdef HUGE_VAL
    const Real vl_inf = Real(HUGE_VAL);
#endif

enum    ZeroOrOne   { vl_zero = 0, vl_0 = 0, vl_one = 1, vl_I = 1, vl_1 = 1 };
enum    Block       { vl_Z = 0, vl_B = 1, vl_block = 1 };
enum    Axis        { vl_x, vl_y, vl_z, vl_w };
typedef Axis        vl_axis;

const uint32_t      VL_REF_FLAG = uint32_t(1) << (sizeof(uint32_t) * 8 - 1);
const uint32_t      VL_REF_MASK = (~VL_REF_FLAG);
enum                { VL_SV_END = -1 };
const uint32_t      VL_SV_MAX_INDEX = (1 << 30);
const int           VL_MAX_STEPS = 10000;

#endif
