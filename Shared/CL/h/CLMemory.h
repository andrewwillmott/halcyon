//
//  File:       CLMemory.h
//
//  Function:   Memory subsystem
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_MEMORY_H
#define CL_MEMORY_H

#include <CLDefs.h>

namespace nCL
{
    class cIAllocator
    {
    public:
        virtual uint8_t* Alloc(size_t size, size_t align = 4) = 0;
        virtual void     Free (void* p) = 0;
    };
    ///< Basic allocator interface

    enum tAllocatorKind
    {
        kNullAllocator,     // Free() is a no-op, Alloc should never be called.
        kDefaultAllocator,  // Basic heap allocator
        kLocalAllocator,    // Allocator meant for temporary allocations that don't outlive the local scope
        kLoggingAllocator,  // Logs allocations/frees
        kDebugAllocator,    // Debug allocations
        kValueAllocator,    // Default allocator for cValues

        // Some generic types
        kGraphicsAllocator,
        kUIAllocator,
        kAudioAllocator,
        kGameAllocator,
        kNetworkAllocator,

        kUserAllocator1 = 16, // First of the user allocators

        kMaxAllocators = 256
    };
    typedef uint16_t tAllocID;

    struct cMemAllocInfo
    {
        tAllocID mOldAllocKind;
        tAllocID mNewAllocKind;
    };

    cIAllocator* Allocator(tAllocatorKind allocatorKind);
    ///< Get given allocator

    bool SetAllocator(tAllocatorKind allocatorKind, cIAllocator* allocator);
    ///< Sets new allocator. Fails if there is an existing current allocator, which has outstanding allocations.
    ///< Typically you should only be setting up allocators at app startup. You must handle thread safety yourself.

    bool InitAllocators();
    ///< Initialise allocator system -- sets up at least kNullAllocator and kDefaultAllocator
    bool ShutdownAllocators();
    ///< Shutdown allocator system.

    size_t PageSize();                      ///< Returns size of a VM page in bytes
    void*  AllocPages(size_t numPages);     ///< Allocates and returns the given number of VM pages, cleared to 0.
    void   FreePages(void* p, size_t numPages); ///< Return given pages to memory

    struct cMappedFileInfo
    {
        const uint8_t* mData;
        size_t         mSize;
    };

    cMappedFileInfo MapFile(const char* path);          // Map the given file into memory (as read-only data). mData will be 0 if the read failed.
    void            UnmapFile(cMappedFileInfo info);    // Release the given file.


    // --- cStackAllocator -----------------------------------------------------

    // Efficient allocator where Free(p) also frees all memory allocated *after* p.
    // Note: not thread safe
    const int kMaxBlocks = 256;

    class cStackAllocator : public cIAllocator
    {
    public:
        cStackAllocator() = default;
        cStackAllocator(size_t blockSize) : mBlockSize(blockSize) {}

        uint8_t* Alloc(size_t size, size_t align = 4) override;
        void     Free (void* p) override;

        // cStackAllocator
        const void* PushMark();                 ///< Returns mark at current allocation point
        void        PopMark(const void* mark);  ///< Frees all allocations that occurred after the given mark. NOTE: you can get the effect of Push/PopMark through cIAllocator via Alloc/Free of a zero-sized item

        size_t      PointerToOffset(void* p);
        void*       OffsetToPointer(size_t offset);

        // Access for dumping memory to a file etc.
        int         NumBlocks() const;
        uint8_t*    BlockPointer(int i) const;
        size_t      BlockSize   (int i) const;

    protected:
        const size_t  mBlockSize = 1024 * 1024;

        uint8_t* mBlockCurrent    = 0;
        uint8_t* mBlockEnd        = 0;

        int      mNumBlocks       = 0;  ///< Number of blocks currently allocated
        uint8_t* mBlocks[kMaxBlocks] = { 0 };
    };

    // --- Allocations helpers -------------------------------------------------

    template<class T> T* Create     (nCL::cIAllocator* alloc);
    template<class T> T* Create     (nCL::cIAllocator* alloc, const T& copy);
    void*                CreateArray(cIAllocator* alloc, size_t elementSize, size_t elementCount);
    template<class T> T* CreateArray(cIAllocator* alloc, size_t elementCount);

                      void Destroy     (uint8_t** p, nCL::cIAllocator* alloc);
    template<class T> void Destroy     (T** p,       nCL::cIAllocator* alloc);
    template<class T> void DestroyArray(T** p,       size_t count, nCL::cIAllocator* alloc);
    // Destroy cIAllocator-allocated object and zero 'p'.


    // --- cAllocatable --------------------------------------------------------

    class cAllocatable
    /// Base class for an object that can be allocated via 'p = new(allocator) cBob',
    /// using any allocator, and then deleted from the corresponding allocator via
    /// 'delete p', or 'delete this'.
    {
    public:
        virtual ~cAllocatable() {}  // ensure class delete gets called in all situations -- see C++11 §12.4/12

        static void* operator new   (size_t n, cIAllocator* a);
        static void  operator delete(void* p);
    };

    cIAllocator* AllocatorFromObject(const void* thisPtr);
    ///< For a cAllocatable-inheriting object, returns the allocator used to
    ///< provide its storage. *Must* be called on the this pointer of the
    ///< top-level object returned from new, not that of an inherited class.


    class cAllocLinkable : public cAllocatable
    /// Allocatable that supports Link() - can also inherit from both cAllocatable and cLinkable but this is quicker
    {
    public:
        int Link(int count) const;
    protected:
        mutable int mLinkCount = 0;
    };
    #define CL_ALLOC_LINK_DECL  int Link(int count) const override { return nCL::cAllocLinkable::Link(count); }


    // --- cAllocPtr --------------------------------------------------------

    template<class T> struct cAllocPtr
    /// Simple pointer to data, plus optional allocator to use to deallocate it
    /// on destruction.
    {
        T*              mData = 0;
        cIAllocator*    mAllocator = 0;

        cAllocPtr() = default;
        cAllocPtr(T* data);
        cAllocPtr(T* data, cIAllocator* a);
        ~cAllocPtr();

        operator T*() const;
        void Release();
    };
}

// --- Global new/destroy ------------------------------------------------------

// Include this in a class definition to ensure it can't be allocated on the heap via vanilla new
#define CL_NO_ALLOC \
    private: \
        static void* operator new   (size_t n); \
        static void* operator new   (size_t n, nCL::cIAllocator* a); \
        static void  operator delete(void* p); \


// Need these to ensure placement new still works
void* operator new     (size_t size, void*) throw();
void* operator new[]   (size_t size, void*) throw();

// New using cIAllocator
void* operator new     (size_t size, nCL::cIAllocator*) throw();
void* operator new[]   (size_t size, nCL::cIAllocator*) throw();

// Note: do NOT use delete for data created via new(alloc), it is the partner of
// vanilla new() only, and can't be generalised to partner with specialised forms of new().
// Instead use Destroy() above.


// --- Inlines -----------------------------------------------------------------

inline const void* nCL::cStackAllocator::PushMark()
{
    return Alloc(0);
}

inline void nCL::cStackAllocator::PopMark(const void* mark)
{
    Free((void*) mark);
}

template<class T> inline nCL::cAllocPtr<T>::cAllocPtr(T* data) : mData(data)
{}

template<class T> inline nCL::cAllocPtr<T>::cAllocPtr(T* data, cIAllocator* a) : mData(data), mAllocator(a)
{}

template<class T> inline nCL::cAllocPtr<T>::~cAllocPtr()
{
    Destroy(&mData, mAllocator);
}

template<class T> inline nCL::cAllocPtr<T>::operator T*() const
{
    return mData;
}

template<class T> inline void nCL::cAllocPtr<T>::Release()
{
    Destroy(&mData, mAllocator);
}

template<class T> inline T* nCL::Create(nCL::cIAllocator* alloc)
{
    void* p = alloc->Alloc(sizeof(T));
    return ::new(p) T;
}

template<class T> inline T* nCL::Create(nCL::cIAllocator* alloc, const T& copy)
{
    void* p = alloc->Alloc(sizeof(T));
    return ::new(p) T(copy);
}

inline void* nCL::CreateArray(cIAllocator* alloc, size_t elementSize, size_t elementCount)
{
    // TODO: handle overflow
    return alloc->Alloc(elementSize * elementCount);
}
template<class T> T* nCL::CreateArray(cIAllocator* alloc, size_t elementCount)
{
    // TODO: handle overflow
    return (T*) alloc->Alloc(sizeof(T) * elementCount);
}

inline void nCL::Destroy(uint8_t** data, cIAllocator* alloc)
{
    if (alloc)
        alloc->Free(*data);
    *data = 0;
}

template<class T> inline void nCL::Destroy(T** obj, nCL::cIAllocator* alloc)
{
    if (*obj)
    {
        (*obj)->~T();

        if (alloc)
            alloc->Free(*obj);

        *obj = 0;
    }
}

template<class T> inline void nCL::DestroyArray(T** obj, size_t count, nCL::cIAllocator* alloc)
{
    if (*obj)
    {
        for (size_t i = 0; i < count; i++)
            obj[i]->~T();

        if (alloc)
            alloc->Free(*obj);

        *obj = 0;
    }
}

inline void* operator new(size_t size, nCL::cIAllocator* alloc) throw()
{
    CL_ERROR("Must call new(alloc) on cAllocatable!");
    return alloc->Alloc(size);
}

inline void* operator new[](size_t size, nCL::cIAllocator* alloc) throw()
{
    CL_ERROR("Must call new(alloc)[] on cAllocatable!");
    return alloc->Alloc(size);
}


#endif
