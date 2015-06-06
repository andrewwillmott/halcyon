// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef MEMLINK_H
#define MEMLINK_H

#include "cmemlink.h"
#include "algorithm.h"

namespace ustl
{

    /// \class memlink memlink.h ustl.h
    /// \ingroup MemoryManagement
    ///
    /// \brief Wrapper for pointer to block with size.
    ///
    /// Use this class the way you would a pointer to an allocated unstructured block.
    /// The pointer and block size are available through member functions and cast operator.
    ///
    /// Example usage:
    /// \code
    ///     void* p = malloc(46721);
    ///     memlink a, b;
    ///     a.link(p, 46721);
    ///     UASSERT(a.size() == 46721));
    ///     b = a;
    ///     UASSERT(b.size() == 46721));
    ///     UASSERT(b.begin() + 34 == a.begin + 34);
    ///     UASSERT(0 == memcmp(a, b, 12));
    ///     a.fill(673, b, 42, 67);
    ///     b.erase(87, 12);
    /// \endcode
    ///
    class memlink : public cmemlink
    {
    public:
        typedef value_type*                 pointer;
        typedef const value_type*           const_pointer;
        typedef pointer                     iterator;
        typedef const_pointer               const_iterator;
        typedef const memlink&              rcself_t;
        
    public:
        inline              memlink()                              : cmemlink() {}
        inline              memlink(void* p, size_type n)          : cmemlink(p, n) {}
        inline              memlink(const void* p, size_type n)    : cmemlink(p, n) {}
        inline              memlink(rcself_t l)                    : cmemlink(l) {}
        inline explicit     memlink(const cmemlink& l)             : cmemlink(l) {}
        
        inline pointer      data()                             { return(const_cast<pointer>(m_Data)); }

        inline iterator         begin()                        { return(iterator(m_Data)); }
        inline iterator         iat(size_type i)               { UASSERT(i <= m_Size); return(iterator(m_Data) + i); }
        inline iterator         end()                          { return(iterator(m_Data + m_Size)); }
        inline const_iterator   begin() const                  { return(const_iterator(m_Data)); }
        inline const_iterator   iat(size_type i) const         { UASSERT(i <= m_Size); return(const_iterator(m_Data) + i); }
        inline const_iterator   end() const                    { return(const_iterator(m_Data + m_Size)); }

        size_type           writable_size() const              { return(m_Size); }

        inline rcself_t     operator=(const cmemlink& l)           { link(l); return(*this); }
        inline rcself_t     operator=(rcself_t l)                  { cmemlink::operator=(l); return(*this); }

        inline void         link(const void* p, size_type n);
        inline void         link(void* p, size_type n);
        inline void         link(const cmemlink& l)                { link(l.m_Data, l.m_Size); }
        inline void         link(memlink& l)                       { link(l.m_Data, l.m_Size); }
        inline void         link(const void* first, const void* last)  { link(first, distance(first, last)); }
        inline void         link(void* first, void* last)              { link(first, distance(first, last)); }
        inline void         relink(const void* p, size_type n);
        inline void         relink(void* p, size_type n);
        inline void         swap(memlink& l)                       { ::ustl::swap(m_Data, l.m_Data); ::ustl::swap(m_Size, l.m_Size); }

        void                fill  (iterator start, const void* p, size_type elsize, size_type elCount = 1);
        inline void         insert(iterator start, size_type size);
        inline void         erase (iterator start, size_type size);
    };

    inline void memlink::link(const void* p, size_type n)
    {
        UASSERT(p != 0 || n == 0);
        unlink();
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }

    inline void memlink::link(void* p, size_type n)
    {
        UASSERT(p != 0 || n == 0);
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }

    /// A fast alternative to link which can be used when relinking to the same block (i.e. when it is resized)
    inline void memlink::relink(const void* p, size_type n)
    {
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }
    inline void memlink::relink(void* p, size_type n)
    {
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }

    /// Shifts the data in the linked block from \p start to \p start + \p n.
    /// The contents of the uncovered bytes is undefined.
    inline void memlink::insert(iterator start, size_type n)
    {
        UASSERT(m_Data || !n);
        UASSERT(start >= m_Data && start + n <= end());
        rotate(start, end() - n, end());
    }

    /// Shifts the data in the linked block from \p start + \p n to \p start.
    /// The contents of the uncovered bytes is undefined.
    inline void memlink::erase(iterator start, size_type n)
    {
        UASSERT(m_Data || !n);
        UASSERT(start >= m_Data && start + n <= end());
        rotate(start, start + n, end());
    }

    /// Use with memlink-derived classes to allocate and link to stack space.
    #define alloca_link(m,n)        (m).link (alloca (n), (n))

}

#endif
