//
//  File:       CLData.h
//
//  Function:   Helpers for storing relocatable data. This allows loading data objects
//              as is (or even mapping them in, read-only) without fixup required.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_DATA_H
#define CL_DATA_H

#include <CLDefs.h>
#include <CLSTL.h>

namespace nCL
{
    /*
        Example:
        
        struct cBob
        {
            cDataObject<cA> mData;
            cDataArray<cB>  mArray;
        };
    
        cBob bob;
        
        bob.mData.Set(store, cA());
        bob.mArray.Set(store, count, array);
        bob.mArray.Set(store, stlArray);

        ...

        F(bob.mData(store));

        int numElts = bob.mArray.NumElts();
        const cB* elts = bob.mArray.Elts(store);
    */

    typedef uint32_t tDataOffset;
    const tDataOffset kNullDataOffset = ~0;

    struct cDataStore
    /// Basically handles translating between offsets and pointers.
    {
        virtual uint8_t*       Data(tDataOffset d) = 0;                 ///< Returns corresponding data pointer, or 0 if d = kNullDataOffset
        virtual const uint8_t* Data(tDataOffset d) const = 0;

        virtual tDataOffset Allocate(size_t size, int alignment = 0) = 0;   ///< Allocate given data and return offset
        virtual void        Free    (tDataOffset d) = 0;                    ///< Free this allocation and all that took place after it.
    };

    template<class T> class cDataObject
    /// Syntactic sugar for an object reference in a cDataStore.
    {
    public:
        bool IsNull() const;    ///< Returns true if this object contains no data.

        const T* operator()(const cDataStore* s) const;
        T*       operator()(cDataStore* s);

        void Set(cDataStore* s, const T& value);    ///< Set to 'value', allocating on demand.

    protected:
        tDataOffset mOffset = kNullDataOffset;
    };

    template<class T> class cDataArray
    {
    public:
        bool IsNull() const;    ///< Returns true if this object contains no data.

        int      NumElts() const;
        const T* Elts(const cDataStore* s) const;
        T*       Elts(cDataStore* s);

        void Set(cDataStore* s, int numElts, const T elts[]);    ///< Set to the given array, allocating on demand.

        template<class T_VECTOR> void Set(cDataStore* store, const T_VECTOR& container); ///< Set to the contents of the given vector.

    protected:
        tDataOffset mOffset = kNullDataOffset;
        tDataOffset mNumElts = 0;
    };


    // Specific data stores
    struct cWriteableDataStore : public cDataStore
    /// Simple linear allocator. (Better would be a page backed store.)
    {
        uint8_t*       Data(tDataOffset d) override;
        const uint8_t* Data(tDataOffset d) const override;

        tDataOffset Allocate(size_t size, int alignment = 0) override;
        void        Free    (tDataOffset d) override;

    protected:
        nCL::vector<uint8_t> mData;
    };

    struct cReadOnlyDataStore : public cDataStore
    /// Read-only fixed block of contiguous data, usually created by cWriteableDataStore and then persisted.
    {
        cReadOnlyDataStore(const uint8_t* data);

        uint8_t*       Data(tDataOffset d) override;
        const uint8_t* Data(tDataOffset d) const override;

        tDataOffset Allocate(size_t size, int alignment = 0) override;
        void        Free    (tDataOffset d) override;

    protected:
        const uint8_t* mData;   ///< Base pointer
    };


    // --- cDataArrayRef<T> ----------------------------------------------------

    template<class T> class cDataArrayRef
    /// WIP: STL-like wrapper over a cDataArray dereference.
    {
    public:
        typedef T* iterator;
        typedef T* const_iterator;

        cDataArrayRef(int numElts, const T* elts) : mNumElts(numElts), mElts(elts) {}
        cDataArrayRef(int numElts, T*       elts) : mNumElts(numElts), mElts(elts) {}

              T& operator[](int i)       { CL_INDEX(i, mNumElts); return mElts[i]; }
        const T& operator[](int i) const { CL_INDEX(i, mNumElts); return mElts[i]; }
        
        int  size()  const { return mNumElts; }
        bool empty() const { return mNumElts == 0; }

        const T* data()  const { return mElts; }
        const T* begin() const { return mElts; }
        const T* end()   const { return mElts + mNumElts; }

    protected:
        int32_t mNumElts;
        T*      mElts;
    };


    // --- Utilities -----------------------------------------------------------

    // Useful for when you don't want strong typing.
    template<class T> tDataOffset Add    (const T& add, cDataStore* store);
    tDataOffset                   AddCStr(const char* s, cDataStore* store);

    template<class T> const T& As    (tDataOffset offset, const cDataStore* store);
    const char*                AsCStr(tDataOffset offset, const cDataStore* store);


    // --- Inlines -------------------------------------------------------------

    // cDataObject
    template<class T> inline bool cDataObject<T>::IsNull() const
    {
        return mOffset == kNullDataOffset;
    }

    template<class T> inline const T* cDataObject<T>::operator()(const cDataStore* s) const
    {
        return (const T*) s->Data(mOffset);
    }

    template<class T> inline T* cDataObject<T>::operator()(cDataStore* s)
    {
        return (T*) s->Data(mOffset);
    }

    template<class T> inline void cDataObject<T>::Set(cDataStore* s, const T& value)
    { 
        if (mOffset == kNullDataOffset)
        {
            mOffset = s->Allocate(sizeof(T), 0);
            new (s->Data(mOffset)) T(value);
        }
        else
            *(T*) s->Data(mOffset) = value;
    }

    // cDataArray
    template<class T> inline bool cDataArray<T>::IsNull() const
    {
        return mOffset == kNullDataOffset;
    }

    template<class T> inline int cDataArray<T>::NumElts() const
    {
        return mNumElts;
    }

    template<class T> inline const T* cDataArray<T>::Elts(const cDataStore* s) const
    {
        return (const T*) s->Data(mOffset);
    }

    template<class T> inline T* cDataArray<T>::Elts(cDataStore* s)
    {
        return (T*) s->Data(mOffset);
    }

    template<class T> inline void cDataArray<T>::Set(cDataStore* s, int numElts, const T* elts)
    {
        if (numElts == 0)
        {
            mOffset = kNullDataOffset;
            mNumElts = 0;
        }
        else if (numElts == mNumElts)
        {
            T* data = s->Data(mOffset);
            for (int i = 0; i < numElts; i++)
                data[i] = elts[i];
        }
        else
        {
            mOffset = s->Allocate(sizeof(T) * numElts, 0);
            T* data = s->Data(mOffset);

            for (int i = 0; i < numElts; i++)
                new(data++) T(elts[i]);
        }
    }

    template<class T> template<class T_VECTOR> inline void cDataArray<T>::Set(cDataStore* s, const T_VECTOR& v)
    {
        Set(s, v.size(), v.data());
    }

    template<class T> inline tDataOffset Add(const T& add, cDataStore* store)
    {
        tDataOffset offset = store->Allocate(sizeof(T), alignof(T));
        *(T*) store->Data(offset) = add;

        return offset;
    }

    template<class T> inline const T& As(tDataOffset offset, const cDataStore* store)
    {
        return *(const T*) store->Data(offset);
    }

    inline tDataOffset AddCStr(const char* s, cDataStore* store)
    {
        tDataOffset offset = store->Allocate(strlen(s) + 1, 0);
        strcpy((char*) store->Data(offset), s);

        return offset;
    }

    inline const char* AsCStr(tDataOffset offset, const cDataStore* store)
    {
        return (const char*) store->Data(offset);
    }
}


#endif
