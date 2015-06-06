//
//  File:       CLSamples.h
//
//  Function:   Sampling tables for various unit entities.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1996
//

#ifndef CL_SAMPLES_H
#define CL_SAMPLES_H

namespace nCL
{
    // 16-sample square/triangle patterns
    const int kNumSampArrays = 16;

    extern float kSquareSamples16  [16][16][2];
    extern float kTriangleSamples16[16][16][2];

    // stratified samples.
    const int kMaxStratSamples = 1024;

    extern float kSquareSamples  [1024][2];
    extern float kTriangleSamples[1024][2];
    extern float kCircleSamples  [1024][2];
    extern float kCubeSamples    [1024][3];
    extern float kSphereSamples  [1024][3];
}

#endif
