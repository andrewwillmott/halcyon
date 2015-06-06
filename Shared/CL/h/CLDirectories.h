//
//  File:       CLDirectories.h
//
//  Function:   Basically a registry for various useful directories.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_DIRECTORIES_H
#define CL_DIRECTORIES_H

#include <CLFileSpec.h>

namespace nCL
{
    enum tStandardDirectories
    {
        kDirectoryApp,          ///< App executable directory
        kDirectoryData,         ///< App data
        kDirectoryResources,    ///< Platform-dependent app-associated OS resources.
        kDirectoryDocuments     ///< Writeable directory for preferences, saved games, etc.
    };

    void RegisterDirectory(uint32_t id, const cFileSpec& spec);
    void RegisterDirectory(uint32_t id, tStrConst directory);

    tStrConst Directory(uint32_t id);
    ///< Returns directory for the given ID, or 0 if it does not exist.
    bool SetDirectory(cFileSpec* spec, uint32_t id);
    ///< Sets the directory portion of the spec to the given ID. Returns false if that directory is not found.
}

#endif
