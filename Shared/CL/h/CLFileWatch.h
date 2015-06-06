//
//  File:       CLFileWatch.h
//
//  Function:   Notifies when files/directories modified
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_FILE_WATCH_H
#define CL_FILE_WATCH_H

#include <CLFileSpec.h>
#include <CLSTL.h>
#include <CLString.h>
#include <CLTag.h>

namespace
{
    struct cFileWatchInternal;
}

namespace nCL
{
    struct cFileSpec;

    struct cFileWatcher
    {
        cFileWatcher();
        ~cFileWatcher();

        bool Init();
        bool Shutdown();

        int  AddFile     (const char* filePath);    ///< Begins watching given file, returns handle to it.
        int  AddDirectory(const char* dirPath);     ///< Begins watching given directory and all contained files.

        int  AddFileID     (tStringID filePath);    ///< Begins watching given file, returns handle to it.
        int  AddDirectoryID(tStringID dirPath);     ///< Begins watching given directory and all contained files.

        bool FilesChanged
        (
            vector<int>* changedFiles  = 0,
            vector<int>* addedFiles    = 0,
            vector<int>* removedFiles  = 0
        );
        ///< Checks for changes since the last call, and optionally fills in details of which files/directories have changed.

        tStringID   PathIDForRef(int ref);
        const char* PathForRef(int ref);


    protected:
        // Utils
        bool CheckForDirectoryChanges
        (
            cDirectoryInfo* dirInfo,
            vector<int>* addedFiles,
            vector<int>* removedFiles
        );

        // Data
        cFileWatchInternal* mInternal = 0;

        map<int, tStringID> mRefToPath;
        map<tStringID, int> mPathToRef;

        map<int, cDirectoryInfo> mRefToDirectoryInfo;
    };



    // --- Inlines -------------------------------------------------------------

    inline tStringID cFileWatcher::PathIDForRef(int ref)
    {
        auto it = mRefToPath.find(ref);

        if (it != mRefToPath.end())
            return it->second;

        return tStringID(0);
    }

    inline const char* cFileWatcher::PathForRef(int ref)
    {
        auto it = mRefToPath.find(ref);

        if (it != mRefToPath.end())
            return StringFromStringID(it->second);

        return 0;
    }

}

#endif
