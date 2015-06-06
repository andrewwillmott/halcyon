//
//  File:       CLArrayRef.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_ARRAYREF_H
#define CL_ARRAYREF_H

namespace nCL
{
    template<class T> class cConstArrayRef
    {
    public:
        typedef const T* iterator;

        cConstArrayRef(const T* array, int count)    : mBegin(array), mEnd(array + count) {}
        cConstArrayRef(const T* begin, const T* end) : mBegin(begin), mEnd(end) {}

        const T& operator[](int i) const { CL_ASSERT(i >= 0 && mBegin + i < mEnd); return mBegin[i]; }
        const T& at        (int i) const { CL_ASSERT(i >= 0 && mBegin + i < mEnd); return mBegin[i]; }

        const T* data()  const { return mBegin; }
        int      size()  const { return mEnd - mBegin; }
        bool     empty() const { return mBegin == mEnd; }

        const T* begin() const { return mBegin; }
        const T* end  () const { return mEnd; }

        cConstArrayRef& operator += (size_t delta) { CL_ASSERT(mBegin + delta <= mEnd); mBegin += delta; return *this; }
        cConstArrayRef& operator -= (size_t delta) { CL_ASSERT(mBegin - delta <= mEnd); mBegin -= delta; return *this; }
        cConstArrayRef& operator ++ ()             { CL_ASSERT(mBegin + 1     <= mEnd); mBegin++; return *this; }
        cConstArrayRef& operator -- ()             { CL_ASSERT(mBegin - 1     <= mEnd); mBegin--; return *this; }

    protected:
        const T* mBegin;
        const T* mEnd;
    };

    template<class T> class cArrayRef
    {
    public:
        typedef const T* iterator;

        cArrayRef(T* array, int count) : mBegin(array), mEnd(array + count) {}
        cArrayRef(T* begin, T* end)    : mBegin(begin), mEnd(end) {}

        T& operator[](int i) const { CL_ASSERT(i >= 0 && mBegin + i < mEnd); return mBegin[i]; }
        T& at        (int i) const { CL_ASSERT(i >= 0 && mBegin + i < mEnd); return mBegin[i]; }

        T*   data()  const { return mBegin; }
        int  size()  const { return mEnd - mBegin; }
        bool empty() const { return mBegin == mEnd; }

        T*   begin() const { return mBegin; }
        T*   end  () const { return mEnd; }

        cArrayRef& operator += (size_t delta) { CL_ASSERT(mBegin + delta <= mEnd); mBegin += delta; return *this; }
        cArrayRef& operator -= (size_t delta) { CL_ASSERT(mBegin - delta <= mEnd); mBegin -= delta; return *this; }
        cArrayRef& operator ++ ()             { CL_ASSERT(mBegin + 1     <= mEnd); mBegin++; return *this; }
        cArrayRef& operator -- ()             { CL_ASSERT(mBegin - 1     <= mEnd); mBegin--; return *this; }

    protected:
        T* mBegin;
        T* mEnd;
    };

#ifdef CL_CHECKING
    #define cConstSliceRef cConstArrayRef
    #define cSliceRef cArrayRef
#else
    template<class T> class cConstSliceRef
    {
    public:
        typedef const T* iterator;

        cConstSliceRef(const T* array, int)      : mBegin(array) {}
        cConstSliceRef(const T* begin, const T*) : mBegin(begin) {}

        const T& operator[](int i) const { return mBegin[i]; }
        const T& at        (int i) const { return mBegin[i]; }

        const T* data()  const { return mBegin; }
        bool     empty() const { return mBegin == 0; }

        cConstSliceRef& operator += (size_t delta) { mBegin += delta; return *this; }

    protected:
        const T* mBegin;
    };

    template<class T> class cSliceRef
    {
    public:
        typedef const T* iterator;

        cSliceRef(T* array, int) : mBegin(array) {}
        cSliceRef(T* begin, T*)  : mBegin(begin) {}

        T& operator[](int i) const { return mBegin[i]; }
        T& at        (int i) const { return mBegin[i]; }

        T*   data()  const { return mBegin; }
        bool empty() const { return mBegin == 0; }

        cSliceRef& operator += (size_t delta) { mBegin += delta; return *this; }

    protected:
        T* mBegin;
    };
#endif

    template<class T> inline cConstArrayRef<T>  CArrayRef(const T* data, size_t size) { return cConstArrayRef<T>(data, size); }
    template<class T> inline cArrayRef<T>       ArrayRef(T*        data, size_t size) { return cArrayRef<T>     (data, size); }
    template<class T> inline cConstSliceRef<T>  CSliceRef(const T* data, size_t size) { return cConstSliceRef<T>(data, size); }
    template<class T> inline cSliceRef<T>       SliceRef(T*        data, size_t size) { return cSliceRef<T>     (data, size); }
}


#define CL_CARRAY(M_P) CArrayRef(M_P, CL_SIZE(M_P))
#define CL_ARRAY(M_P)  ArrayRef (M_P, CL_SIZE(M_P))

#ifdef CL_CHECKING
    #define CL_CSLICE(M_P) CSliceRef(M_P, CL_SIZE(M_P))
    #define CL_SLICE(M_P)  SliceRef (M_P, CL_SIZE(M_P))
#else
    #define CL_CSLICE(M_P) CSliceRef(M_P, 0)
    #define CL_SLICE(M_P)  SliceRef (M_P, 0)
#endif

#endif
