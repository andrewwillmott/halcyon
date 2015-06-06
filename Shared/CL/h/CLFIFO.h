//
//  File:       CLFIFO.h
//
//  Function:   Classic lockless FIFO for inter-thread communication
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_FIFO_H
#define CL_FIFO_H

#include <CLBits.h>
#include <CLMemory.h>

#if defined(CL_IOS) || defined (CL_OSX)
    #include <libKern/OSAtomic.h>
#endif

namespace nCL
{
    template <class T> class cFIFO
    {
    public:
        cFIFO() = default;
        cFIFO(uint32_t maxSize, nCL::cIAllocator* alloc);
        ~cFIFO();

        void Resize(uint32_t maxSize, nCL::cIAllocator* alloc);   ///< Do not call while fifo is in active use
        void Clear();   ///< Do not call while fifo is in active use
        int  Size();    ///< How many items in the queue. This is a max size if called from the writing thread, min size if called from the reading thread.

        bool Write(const T& item);  ///< Write the given item -- returns false if the fifo is full.
        bool Read (T* item);        ///< Read next available item into 'item', or returns false if none exists.

        // Lower-level API
        T*       NextWriteItem();   ///< Returns a pointer to space to write the next item, or 0 if the fifo is full
        const T* NextReadItem();    ///< Returns a pointer to the next item available for reading, or 0 if none.

        void CommitWrite();     ///< Call when next item has finished being written
        void CommitRead();      ///< Call to discard next item available for reading.

    protected:
        T*                  mBuffer = 0;
        cIAllocator*        mAllocator = 0;

        volatile int32_t    mReadIndex  = 0;
        volatile int32_t    mWriteIndex = 0;
        int32_t             mMask = 0;
    };



    // --- Inlines -------------------------------------------------------------

    template<class T> inline cFIFO<T>::cFIFO(uint32_t maxSize, nCL::cIAllocator* alloc)
    {
        maxSize = CeilPow2(maxSize);

        mBuffer = CreateArray<T>(alloc, maxSize);
        mMask = maxSize - 1;
    }
    
    template<class T> inline cFIFO<T>::~cFIFO()
    {
        Destroy(&mBuffer, mAllocator);
    }

    template<class T> inline void cFIFO<T>::Resize(uint32_t maxSize, nCL::cIAllocator* alloc)
    {
        CL_ASSERT(mReadIndex == mWriteIndex);

        Destroy(&mBuffer, mAllocator);

        maxSize = CeilPow2(maxSize);

        mBuffer = CreateArray<T>(alloc, maxSize);
        mMask = maxSize - 1;
        mReadIndex = 0;
        mWriteIndex = 0;
    }
    
    template<class T> inline void cFIFO<T>::Clear()
    {
        mReadIndex = 0;
        mWriteIndex = 0;
    }

    template<class T> inline int cFIFO<T>::Size()
    {
        return (mWriteIndex - mReadIndex) & mMask;
    }

    template<class T> inline bool cFIFO<T>::Write(const T& item)
    {
        T* nextItem = NextWriteItem();

        if (nextItem)
        {
            *nextItem = item;
            CommitWrite();
            return true;
        }

        return false;
    }

    template<class T> inline bool cFIFO<T>::Read(T* item)
    {
        const T* nextItem = NextReadItem();

        if (nextItem)
        {
            *item = *nextItem;
            CommitRead();
            return true;
        }

        return false;
    }

    template<class T> inline T* cFIFO<T>::NextWriteItem()
    {
        uint32_t nextWriteIndex = (mWriteIndex + 1) & mMask;

        if (nextWriteIndex == mReadIndex)
            return 0;

        return mBuffer + mWriteIndex;
    }
    
    template<class T> inline const T* cFIFO<T>::NextReadItem()
    {
        if (mReadIndex == mWriteIndex)
            return 0;

        return mBuffer + mReadIndex;
    }
    
    template<class T> inline void cFIFO<T>::CommitWrite()
    {
    #if defined(CL_IOS) || defined (CL_OSX)
        // We're not doing a 'real' CAS here, rather this is to force a memory barrier in cross-CPU way
        OSAtomicCompareAndSwap32(mWriteIndex, (mWriteIndex + 1) & mMask, &mWriteIndex);
    #else
        // Assume we're on strongly ordered CPU. (I.e., x86, *not* ARM or PPC.)
        mWriteIndex = (mWriteIndex + 1) & mMask;
    #endif
    }

    template<class T> inline void cFIFO<T>::CommitRead()
    {
    #if defined(CL_IOS) || defined (CL_OSX)
        // We're not doing a 'real' CAS here, rather this is to force a memory barrier in cross-CPU way
        OSAtomicCompareAndSwap32(mReadIndex,  (mReadIndex  + 1) & mMask, &mReadIndex);
    #else
        // Assume we're on strongly ordered CPU. (I.e., x86, *not* ARM or PPC.)
        mReadIndex = (mReadIndex + 1) & mMask;
    #endif
    }
}

#endif
