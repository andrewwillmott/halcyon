//
//  File:       CLUtilities.h
//
//  Function:   Various useful system-level utilities
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_UTILITIES_H
#define CL_UTILITIES_H

#include <CLString.h>

namespace nCL
{
    bool OpenTextFile     (const char* path, int errorLine);    ///< Open given file in editor at given line if possible
    bool OpenTextDirectory(const char* path);                   ///< Open given directory

    bool OpenForEdit(const char* path);                         ///< Open given file in SCM
    bool OpenForAdd (const char* path);                         ///< Add given file to SCM

    const char* PlatformName();     ///< Returns canonical platform name
    const char* ConfigName();       ///< Returns canonical config name -- Release/Developer/Debug

    int FindCallstack(nCL::string* callstack, int maxScopes = 100);  ///< Find string describing current callstack. Returns number of scopes found, or 0 if unimplemented.
}

#endif
