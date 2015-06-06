//
//  File:       CLMemory.cpp
//
//  Function:   Memory subsystem
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLMemory.h>

#include <CLLog.h>
#include <CLSTL.h>

#include <stdlib.h>

#include <errno.h>
#include <string.h> // for strerror =P

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#if !defined(CL_MEM_TRACK)
    #ifdef CL_CHECKING
        #define CL_MEM_TRACK 1
    #else
        #define CL_MEM_TRACK 0
    #endif
#endif

#if CL_MEM_TRACK
    #include <pthread.h>
#endif

#define CL_ASSERT_MONO_THREAD \
    { \
        static pthread_t sSingleThread = 0; \
        if (sSingleThread) \
            CL_ASSERT(sSingleThread == pthread_self()); \
        else \
            sSingleThread == pthread_self(); \
    }

using namespace nCL;

namespace
{
    class cNullAllocator : public cIAllocator
    {
    public:
        uint8_t* Alloc(size_t size, size_t align = 4)
        {
            CL_ERROR("Shouldn't be called");
            return 0;
        }

        void Free(void* p)
        {
            // Purposely a no-op. E.g., you'd use this 'allocator' for memory allocated on the stack.
        }
    };

    class cBasicHeapAllocator : public cIAllocator
    {
    public:
        uint8_t* Alloc(size_t size, size_t align = 4)
        {
            mNumAllocs++;

            uint8_t* result = (uint8_t*) calloc(size, 1);

        #if CL_MEM_TRACK
            CL_ASSERT_MONO_THREAD;

            mCurrentAllocs.insert(result);
        #endif

            return result;
        }

        void Free(void* p)
        {
            CL_ASSERT(p);
            CL_ASSERT(mNumAllocs > 0);
            mNumAllocs--;

        #if CL_MEM_TRACK
            CL_ASSERT_MONO_THREAD;

            auto it = mCurrentAllocs.find(p);
            CL_ASSERT(it != mCurrentAllocs.end());
            mCurrentAllocs.erase(it);
        #endif

            free(p);
        }

        int mNumAllocs = 0;
    #if CL_MEM_TRACK
        nCL::set<void*> mCurrentAllocs;
    #endif
    };


    cIAllocator*        sAllocators[kMaxAllocators] = { 0 };

    cNullAllocator      sNullAllocator;
    cBasicHeapAllocator sHeapAllocator;
    cStackAllocator     sLocalAllocator;
}

cIAllocator* nCL::Allocator(tAllocatorKind allocatorKind)
{
    return sAllocators[allocatorKind];
}

bool nCL::SetAllocator(tAllocatorKind allocatorKind, cIAllocator* allocator)
{
    if (!sAllocators[allocatorKind])
    {
        sAllocators[allocatorKind] = allocator;
        return true;
    }

    return false;
}


bool nCL::InitAllocators()
{
    CL_ASSERT(sAllocators[kNullAllocator] == 0);

    sAllocators[kNullAllocator]    = &sNullAllocator;
    sAllocators[kDefaultAllocator] = &sHeapAllocator;
    sAllocators[kLoggingAllocator] = &sHeapAllocator;
    sAllocators[kValueAllocator]   = &sHeapAllocator;
    sAllocators[kLocalAllocator]   = &sLocalAllocator;

    // Generic
    sAllocators[kGraphicsAllocator] = sAllocators[kDefaultAllocator];
    sAllocators[kUIAllocator]       = sAllocators[kDefaultAllocator];
    sAllocators[kAudioAllocator]    = sAllocators[kDefaultAllocator];
    sAllocators[kGameAllocator]     = sAllocators[kDefaultAllocator];
    sAllocators[kNetworkAllocator]  = sAllocators[kDefaultAllocator];

    return true;
}

bool nCL::ShutdownAllocators()
{
    for (int i = 0; i < kUserAllocator1; i++)
        sAllocators[i] = 0;

    return true;
}


namespace
{
    // getpagesize();
    const size_t kPageSize = 4096;
}

size_t nCL::PageSize()
{
    return kPageSize;
}

void* nCL::AllocPages(size_t numPages)
{
    size_t size = numPages * kPageSize;
    void* p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0); // may be MAP_ANONYMOUS on some systems
    return p;
}

void nCL::FreePages(void* p, size_t numPages)
{
    int result = munmap(p, numPages * kPageSize);
    CL_ASSERT(result == 0);
}

cMappedFileInfo nCL::MapFile(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return { 0, 0 };

    size_t dataSize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    bool result = false;

    const void* data = mmap(0, dataSize, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);

    close(fd);

    if (data == MAP_FAILED)
        return { 0, 0 };

    return { (const uint8_t*) data, dataSize };
}

void nCL::UnmapFile(cMappedFileInfo info)
{
    if (info.mData != 0)
    {
        if (munmap((void*) info.mData, info.mSize) != 0)
            CL_LOG_E("Memory", "unmap error %s", strerror(errno));
    }
}


// cStackAllocator

uint8_t* cStackAllocator::Alloc(size_t size, size_t align)
{
    if (size >= mBlockSize)
        return 0;

    uint8_t* p = mBlockCurrent;
    mBlockCurrent += size;

    if (mBlockCurrent <= mBlockEnd)
        return p;

    if (mNumBlocks == kMaxBlocks)
    {
        CL_ERROR("Out of block memory");
        mBlockCurrent = 0;
        mBlockEnd     = 0;
        return 0;
    }

    mBlockCurrent = (uint8_t*) calloc(1, mBlockSize);
    mBlockEnd     = mBlockCurrent + mBlockSize;

    mBlocks[mNumBlocks] = mBlockCurrent;
    mNumBlocks++;

    return mBlockCurrent;
}

void cStackAllocator::Free(void* p)
{
    CL_ASSERT(mNumBlocks > 0);

    int block = mNumBlocks;

    do
    {
        block--;

        if (block < 0)
        {
            CL_ERROR("invalid free");
            return;
        }
    }
    while (p < mBlocks[block] || p > mBlocks[block] + mBlockSize);

    mBlockCurrent = (uint8_t*) p;

    if (block + 1 != mNumBlocks)
    {
        mBlockEnd = mBlocks[block] + mBlockSize;
        block++;

        // free in reverse order to be nice to whatever allocator we're using.
        for (int i = mNumBlocks - 1; i >= block; i--)
            free(mBlocks[i]);

        mNumBlocks = block;
    }
}

size_t cStackAllocator::PointerToOffset(void* pIn)
{
    uint8_t* p = (uint8_t*) pIn;
    int block = mNumBlocks;

    do
    {
        block--;

        if (block < 0)
        {
            CL_ERROR("invalid pointer");
            return ~size_t(0);
        }
    }
    while (p < mBlocks[block] || p > mBlocks[block] + mBlockSize);

    return p - mBlocks[block];
}

void* cStackAllocator::OffsetToPointer(size_t offset)
{
    size_t block = offset / mBlockSize;
    CL_ASSERT(block < kMaxBlocks);

    return mBlocks[block] + (offset - mBlockSize * block);
}

int cStackAllocator::NumBlocks() const
{
    return mNumBlocks;
}

uint8_t* cStackAllocator::BlockPointer(int i) const
{
    return mBlocks[i];
}

size_t cStackAllocator::BlockSize(int i) const
{
    return (i == mNumBlocks - 1) ? (mBlockEnd - mBlocks[mNumBlocks - 1]) : mBlockSize;
}


// cAllocatable

void* cAllocatable::operator new(size_t n, cIAllocator* a)
{
    // We can't write 'a' directly to a member variable of cAllocatable, because
    // the constructor has not been run yet, and may stomp over that variable.
    // (There are ways of hacking around this, but it's pretty risky.)
    // So instead we manually manage the allocator pointer ourselves -- alloc
    // additional space for it in front of the object.

    uint8_t* p = a->Alloc(n + sizeof(cIAllocator*), sizeof(void*));    // TODO: if we ever have Alloc(size, alignment, offset), use alignof()

    if (p)
    {
        *(cIAllocator**) p = a;
        p += sizeof(cIAllocator*);
    }

    return p;
}

void cAllocatable::operator delete(void* p)
{
    (uint8_t*&) p -= sizeof(cIAllocator*);
    cIAllocator* a = *(cIAllocator**) p;
    a->Free(p);
}

cIAllocator* nCL::AllocatorFromObject(const void* thisPtr)
{
    const uint8_t* p = (const uint8_t*) thisPtr;
    p -= sizeof(cIAllocator*);
    return *(cIAllocator**) p;
}

int nCL::cAllocLinkable::Link(int count) const
{
    if ((mLinkCount += count) > 0 || count >= 0)
        return mLinkCount;

    delete this;
    return 0;
}

// Global operators. Direct to system heap for now, as these are mostly called
// by system code, but we might want to add tracking here later.

namespace
{
#if CL_MEM_TRACK
    nCL::set<void*> sGlobalAllocs;
    nCL::set<void*> sGlobalArrayAllocs;

    pthread_mutex_t sAllocMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
}

void* operator new     (size_t size)
{
    void* result = malloc(size);
#if CL_MEM_TRACK
    pthread_mutex_lock(&sAllocMutex);
    sGlobalAllocs.insert(result);
    pthread_mutex_unlock(&sAllocMutex);
#endif
    return result;
}

void* operator new[]   (size_t size)
{
    void* result = malloc(size);
#if CL_MEM_TRACK
    pthread_mutex_lock(&sAllocMutex);
    sGlobalArrayAllocs.insert(result);
    pthread_mutex_unlock(&sAllocMutex);
#endif
    return result;
}

void  operator delete  (void* p) throw()
{
#if CL_MEM_TRACK
    pthread_mutex_lock(&sAllocMutex);
    int count = sGlobalAllocs.erase(p);

    if (count == 0)
    {
        count = sGlobalArrayAllocs.erase(p);

        CL_EXPECT_MSG(count != 1, "Mismatched array global new/non-array delete\n");
    }

    // Can't assert, OS code has bugs =P
    CL_EXPECT_MSG(count == 1, "Mismatched global new/delete\n");
    //CL_EXPECT_MSG(count == 1, "Mismatched global new/delete\n");
    pthread_mutex_unlock(&sAllocMutex);
#endif
    free(p);
}

void  operator delete[](void* p) throw()
{
#if CL_MEM_TRACK
    pthread_mutex_lock(&sAllocMutex);
    int count = sGlobalArrayAllocs.erase(p);
    CL_ASSERT(count == 1);
    pthread_mutex_unlock(&sAllocMutex);
#endif
    free(p);
}

