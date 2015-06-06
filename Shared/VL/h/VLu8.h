//
//  File:       VLu8.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef VLu8_H
#define VLu8_H

// Define Volu8
#ifdef VL_H
    #include "VLUndef.h"
#endif
#define VL_V_REAL uint8_t
#define VL_V_SUFF(X) X ## u8
#include <VLVec.h>
#include <VLMat.h>
#include <VLVol.h>
#include "VLUndef.h"

#endif
