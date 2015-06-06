//
//  File:       CLSystemInfo.h
//
//  Function:   Utilities to get info about whatever we're running on
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_SYSTEM_INFO_H
#define CL_SYSTEM_INFO_H

#include <CLDefs.h>

namespace nCL
{
    void GetSystemInfo();

    size_t UsedMemory();
    float  UsedMemoryInMB();

    size_t FreeMemory();
    float  FreeMemoryInMB();
}

#endif
