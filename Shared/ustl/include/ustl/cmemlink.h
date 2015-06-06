// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef USTL_CMEMLINK_H
#define USTL_CMEMLINK_H

#include "algobase.h"

/// The ustl namespace contains all ustl classes and algorithms.
namespace ustl
{
    /// \class cmemlink cmemlink.h ustl.h
    /// \ingroup MemoryManagement
    ///
    /// \brief A read-only pointer to a sized block of memory.
    ///
    /// Use this class the way you would a const pointer to an allocated unstructured block.
    /// The pointer and block size are available through member functions and cast operator.
    ///
    /// Example usage:
    ///
    /// \code
    ///     void* p = malloc (46721);
    ///     cmemlink a, b;
    ///     a.link (p, 46721);
    ///     UASSERT(a.size() == 46721));
    ///     b = a;
    ///     UASSERT(b.size() == 46721));
    ///     UASSERT(b.at(34) == a.at(34));
    ///     UASSERT(0 == memcmp (a, b, 12));
    /// \endcode
    ///
    class cmemlink
    {
    public:
        typedef char                value_type;
        typedef const value_type*   pointer;
        typedef const value_type*   const_pointer;
        typedef value_type          reference;
        typedef value_type          const_reference;
        typedef size_t              size_type;
        typedef uint32_t            written_size_type;
        typedef ptrdiff_t           difference_type;
        typedef const_pointer       const_iterator;
        typedef const_iterator      iterator;
        typedef const cmemlink&     rcself_t;
        
    public:
        inline              cmemlink()                           : m_Data(NULL), m_Size(0) { }
        inline              cmemlink(const void* p, size_type n) : m_Data(const_pointer(p)), m_Size(n) { UASSERT(p || !n); }
        inline              cmemlink(const cmemlink& l)          : m_Data(l.m_Data), m_Size(l.m_Size) {}
        inline virtual     ~cmemlink() {}

        inline const_pointer cdata () const             { return m_Data; }

        inline iterator     begin() const               { return iterator(m_Data); }
        inline iterator     iat(size_type i) const      { UASSERT(i <= m_Size); return iterator(m_Data) + i; }
        inline iterator     end() const                 { return iterator(m_Data + m_Size); }

        inline void         resize (size_type n)        { m_Size = n; }
        inline size_type    size () const               { return m_Size; }
        inline size_type    max_size () const           { return m_Size; }
        inline bool         empty () const              { return m_Size == 0; }

        inline rcself_t     operator=  (const cmemlink& l)  { link (l); return (*this); }
        bool                operator== (const cmemlink& l) const;

        void                link (const void* p, size_type n);
        inline void         link (const cmemlink& l)                    { link (l.m_Data, l.m_Size); }
        inline void         link (const void* first, const void* last)  { link (first, distance (first, last)); }
        inline void         relink (const void* p, size_type n);
        virtual void        unlink ();
        inline void         swap (cmemlink& l)              { ::ustl::swap(m_Data, l.m_Data); ::ustl::swap (m_Size, l.m_Size); }

    public: // public due to non-const variant use
        const_pointer       m_Data;         ///< Pointer to the data block (const)
        size_type           m_Size;         ///< size of the data block
    };

    //----------------------------------------------------------------------

    /// \brief Attaches the object to pointer \p p of size \p n.
    ///
    /// If \p p is NULL and \p n is non-zero, bad_alloc is thrown and current
    /// state remains unchanged.
    ///
    inline void cmemlink::link(const void* p, size_type n)
    {
        UASSERT(p != 0 || n == 0);
        unlink();
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }

    /// A fast alternative to link which can be used when relinking to the same block (i.e. when it is resized)
    inline void cmemlink::relink(const void* p, size_type n)
    {
        m_Data = reinterpret_cast<const_pointer>(p);
        m_Size = n;
    }

    inline void cmemlink::unlink()
    {
        m_Data = NULL;
        m_Size = 0;
    }

    //----------------------------------------------------------------------

    /// Use with cmemlink-derived classes to link to a static array
    #define static_link(v)  link(VectorBlock(v))

}

#endif
