//
//  File:       LibVLu8.cpp
//
//  Function:   Instantiates code necessary for VLu8.h
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//



// To make the Mat stuff work, would need:
// - way to disable SubVec*
// - proper way of turning off sqrt() etc., or sqrt impl
//   - should dot() always return a scalar type?
// - Sort out Mat reference stuff -- passing mL[0] to routine that will modify Matf is use case

#include <stdint.h>
#include <string.h>

namespace
{
    // TODO: for u8 at least, we really just want to turn off the kind of thing
    // that uses these
    inline uint8_t len(uint8_t a) { return a; }

    void SetReal(uint8_t& u, double e)
    {
        u = uint8_t(e * 255.0f);
    }
}

#include "VLUndef.h"
#define VL_V_REAL uint8_t
#define VL_V_SUFF(X) X ## u8
#include "VLVec.cpp"
#include "VLMat.cpp"
#include "VLVol.cpp"
