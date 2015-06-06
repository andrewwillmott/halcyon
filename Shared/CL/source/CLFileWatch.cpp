//
//  File:       CLFileWatch.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLFileWatch.h>

#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLString.h>
#include <CLTag.h>

// This is an implementation based on kevents, supported on OSX and iOS. TODO: does this exist on Android?
// Update -- seems like it does, see https://github.com/plattypus/Android-4.0.1_r1.0/blob/master/external/dbus/bus/dir-watch-kqueue.c FI.

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

using namespace nCL;

namespace
{
    const unsigned int kNodeAllEvents =
          NOTE_DELETE
        | NOTE_WRITE
        | NOTE_EXTEND
        | NOTE_ATTRIB
        | NOTE_LINK
        | NOTE_RENAME
        | NOTE_REVOKE;

    const unsigned int kNodeWatchEvents =
          NOTE_DELETE
        | NOTE_WRITE
        | NOTE_EXTEND
        | NOTE_RENAME
        | NOTE_REVOKE;


    void GetFlags(uint32_t flags, tString* s)
    {
        if (flags & NOTE_DELETE) { if (!s->empty()) s->append(" | "); s->append("NOTE_DELETE"); }
        if (flags & NOTE_WRITE)  { if (!s->empty()) s->append(" | "); s->append("NOTE_WRITE" ); }
        if (flags & NOTE_EXTEND) { if (!s->empty()) s->append(" | "); s->append("NOTE_EXTEND"); }
        if (flags & NOTE_ATTRIB) { if (!s->empty()) s->append(" | "); s->append("NOTE_ATTRIB"); }
        if (flags & NOTE_LINK)   { if (!s->empty()) s->append(" | "); s->append("NOTE_LINK"  ); }
        if (flags & NOTE_RENAME) { if (!s->empty()) s->append(" | "); s->append("NOTE_RENAME"); }
        if (flags & NOTE_REVOKE) { if (!s->empty()) s->append(" | "); s->append("NOTE_REVOKE"); }
    }

    struct cFileWatchInternal
    {
        int mKernelQueue = -1;
        int mDirFD = -1;
    };
}

/*
    Sketch:
    
    - if a file is added, we need to add it as a FD to watch
    - if a FD vnode is deleted, we need to remove it.
    - if a new file is added, we should check it's not one of our existing ones renamed.
*/



cFileWatcher::cFileWatcher() :
    mInternal(new cFileWatchInternal)
{
}

cFileWatcher::~cFileWatcher()
{
    delete mInternal;
    mInternal = 0;
}

bool cFileWatcher::Init()
{
    CL_ASSERT(mInternal->mKernelQueue < 0);

    int kq = kqueue();
    
    if (kq < 0)
    {
        CL_LOG_D("FileWatch", "Could not open kernel queue.  Error was %s.\n", strerror(errno));

        close(mInternal->mDirFD);
        mInternal->mDirFD = 0;

        return false;
    }

    mInternal->mKernelQueue = kq;

    return true;
}

bool cFileWatcher::Shutdown()
{
    if (mInternal->mKernelQueue < 0)
        return false;

    // Happily, it suffices to close watched fds -- the corresponding queue events will be removed automatically.
    for (auto rp : mRefToPath)
        close(rp.first);

    mRefToPath.clear();
    mPathToRef.clear();
    mRefToDirectoryInfo.clear();

    close(mInternal->mKernelQueue);
    mInternal->mKernelQueue = 0;

    return true;
}


int cFileWatcher::AddFile(const char* filePath)
{
    tStringID pathID = StringIDFromString(filePath);
    return AddFileID(pathID);
}

int cFileWatcher::AddFileID(tStringID pathID)
{
    auto it = mPathToRef.find(pathID);
    if (it != mPathToRef.end())
        return it->second;

    const char* filePath = StringFromStringID(pathID);
    int fd = open(filePath, O_EVTONLY);

    if (fd < 0)
    {
        CL_LOG("FileWatch", "error opening %s\n", filePath);
        return -1;
    }

    struct kevent newEvent;

    newEvent.ident  = fd;
    newEvent.filter = EVFILT_VNODE;
    newEvent.flags  = EV_ADD | EV_CLEAR;
    newEvent.fflags = kNodeWatchEvents;
    newEvent.data   = 0;
    newEvent.udata  = 0;

    if (kevent(mInternal->mKernelQueue, &newEvent, 1, 0, 0, 0) < 0)
    {
        CL_LOG("FileWatch", "error adding new event for %s\n", filePath);
        close(fd);
        return -1;
    }

    CL_LOG_D("FileWatch", "now watching file %s on fd %d\n", filePath, fd);
    mPathToRef[pathID] = fd;
    mRefToPath[fd] = pathID;

    return fd;
}

int cFileWatcher::AddDirectory(const char* dirPath)
{
    tStringID pathID = StringIDFromString(dirPath);

    return AddDirectoryID(pathID);
}

int cFileWatcher::AddDirectoryID(tStringID pathID)
{
    auto it = mPathToRef.find(pathID);
    if (it != mPathToRef.end())
        return it->second;

    const char* dirPath = StringFromStringID(pathID);
    int fd = open(dirPath, O_EVTONLY);

    if (fd < 0)
    {
        CL_LOG("FileWatch", "error opening %s\n", dirPath);
        return -1;
    }

    struct kevent newEvent;

    newEvent.ident  = fd;
    newEvent.filter = EVFILT_VNODE;
    newEvent.flags  = EV_ADD | EV_CLEAR;
    newEvent.fflags = kNodeWatchEvents;
    newEvent.data   = 0;
    newEvent.udata  = 0;

    if (kevent(mInternal->mKernelQueue, &newEvent, 1, 0, 0, 0) < 0)
    {
        CL_LOG("FileWatch", "error adding new event for %s\n", dirPath);
        close(fd);
        return -1;
    }

    CL_LOG_D("FileWatch", "now watching directory %s\n", dirPath); 

    mPathToRef[pathID] = fd;
    mRefToPath[fd] = pathID;

    cDirectoryInfo& dirInfo = mRefToDirectoryInfo[fd];
    dirInfo.Read(dirPath);

    cFileSpec fileSpec;
    fileSpec.SetDirectory(dirPath);

    for (int i = 0, n = dirInfo.mFiles.size(); i < n; i++)
    {
        fileSpec.SetNameAndExtension(dirInfo.mFiles[i].mName);

        AddFile(fileSpec.Path());
    }

    return true;
}

bool cFileWatcher::FilesChanged
(
    vector<int>* changedFiles,
    vector<int>* addedFiles,
    vector<int>* removedFiles
)
{
    if (mInternal->mKernelQueue < 0)
        return false;

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;

    struct kevent eventData[16];
    int eventCount;

    bool changes = false;

    // Check for queued events
    do
    {
        eventCount = kevent(mInternal->mKernelQueue, 0, 0, eventData, CL_SIZE(eventData), &timeout);

        if (eventCount < 0)
        {
            CL_LOG_D("FileWatch", "kevent error: %s.\n", strerror(errno));
            break;
        }

        for (int i = 0; i < eventCount; i++)
        {
            if (eventData[i].flags == EV_ERROR)
            {
                CL_LOG("FileWatch", "kevent error: %s.\n", strerror(eventData[i].data));

                continue;
            }

            int fd = eventData[i].ident;

            auto it = mRefToDirectoryInfo.find(fd);
            
            if (it != mRefToDirectoryInfo.end())
            {
                CheckForDirectoryChanges(&it->second, addedFiles, removedFiles);
                changes = true;
            }
            else
            {
                changes = true;

                if (eventData[i].fflags & NOTE_DELETE)
                {
                    if (removedFiles)
                        removedFiles->push_back(fd);

                    auto it1 = mRefToPath.find(fd);
                    tStringID pathID = it1->second;

                    mRefToPath.erase(it1);
                    mPathToRef.erase(pathID);

                    close(fd);  // remove watch event
                }

                if (eventData[i].fflags & NOTE_RENAME)
                {
                    // we should update mRefToPath etc. how to get the new name of the fd though?
                    // perhaps we need to signal to directory info that rename rather than add/delete took place?
                }

                if (changedFiles && (eventData[i].fflags & (NOTE_WRITE | NOTE_EXTEND)))
                    changedFiles->push_back(fd);
            }

        #ifdef CL_DEBUG_LOCAL
            tString filterFlags;
            GetFlags(eventData[i].fflags, &filterFlags);

            CL_LOG_D("FileWatch", "Event %" PRIdPTR " occurred.  Filter %d, flags %d, filter flags %s, filter data %" PRIdPTR ", udata %" PRIdPTR "\n",
                   eventData[i].ident,
                   eventData[i].filter,
                   eventData[i].flags,
                   filterFlags.c_str(),
                   eventData[i].data,
                   intptr_t(eventData[i].udata)
               );
       #endif
        }
    }
    while (eventCount > 0);

    return changes;
}

// Internal

bool cFileWatcher::CheckForDirectoryChanges
(
    cDirectoryInfo* dirInfo,
    vector<int>* addedFiles,
    vector<int>* removedFiles
)
{
    vector<cDirectoryInfo::cInfo> fileInfo;
    fileInfo.swap(dirInfo->mFiles);

    dirInfo->ReadAgain();

    for (int i = 0, ni = dirInfo->mFiles.size(); i < ni; i++)
    {
        bool found = false;

        for (int j = 0, nj = fileInfo.size(); j < nj; j++)
        {
            if (dirInfo->mFiles[i].mName == fileInfo[j].mName)
            {
                found = true;
                fileInfo[j].mName.clear();
                break;
            }
        }

        if (!found)
        {
            tStrConst addName = dirInfo->mFiles[i].mName.c_str();

            cFileSpec fileSpec;
            fileSpec.SetDirectory(dirInfo->mPath.c_str());
            fileSpec.SetNameAndExtension(addName);

            int addRef = AddFile(fileSpec.Path());

            if (addedFiles && addRef >= 0)
                addedFiles->push_back(addRef);
        }
    }

#ifdef DISABLED
    // For the moment, we get a direct notification about removed files,
    // so that should take care of the addition to removedFiles and removal
    // of the fd.
    for (int j = 0, nj = fileInfo.size(); j < nj; j++)
        if (!fileInfo[j].mName.empty())
        {
            tStrConst removeName = fileInfo[j].mName.c_str();

            cFileSpec fileSpec;
            fileSpec.SetDirectory(dirInfo->mPath.c_str());
            fileSpec.SetNameAndExtension(removeName);

            tStringID removePathID = StringIDFromString(fileSpec.Path());
            auto it = mPathToRef.find(removePathID);

            if (it != mPathToRef.end())
            {
                if (removedFiles)
                    removedFiles->push_back(it->second);
            }
        }
#endif

    return true;
}
