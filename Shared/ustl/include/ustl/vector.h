// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef UVECTOR_H
#define UVECTOR_H

#include "memblock.h"
#include "memory.h"
#include "predalgo.h"

namespace ustl
{

    /// \class vector uvector.h ustl.h
    /// \ingroup Sequences
    ///
    /// \brief STL vector equivalent.
    ///
    /// Provides a typed array-like interface to a managed memory block, including
    /// element access, iteration, modification, resizing, and serialization. In
    /// this design elements frequently undergo bitwise move, so don't put it in
    /// here if it doesn't support it. This mostly means having no self-pointers.
    ///
    template <typename T>
    class vector : public memblock
    {
    public:
        typedef T                           value_type;
        typedef value_type*                 pointer;
        typedef const value_type*           const_pointer;
        typedef value_type&                 reference;
        typedef const value_type&           const_reference;
        typedef pointer                     iterator;
        typedef const_pointer               const_iterator;
        typedef memblock::size_type         size_type;
        typedef memblock::written_size_type written_size_type;
        typedef memblock::difference_type   difference_type;

        typedef ::ustl::reverse_iterator<iterator>          reverse_iterator;
        typedef ::ustl::reverse_iterator<const_iterator>    const_reverse_iterator;

    public:
                             vector();
        explicit             vector(size_type n);
                             vector(size_type n, const T& v);
                             vector(const vector<T>& v);
                             vector(const_iterator i1, const_iterator i2);
                             ~vector()     throw();

        const vector<T>&     operator=(const vector<T>& v);
        bool                 operator==(const vector<T>& v) const   { return m_Data == v.m_Data; }

        void                 reserve(size_type n);
        void                 resize (size_type n);
        void                 resize (size_type n, const T& v);
        size_type            capacity() const               { return m_Capacity / sizeof(T);       }
        size_type            size()     const               { return m_Size / sizeof(T);           }
        size_type            max_size() const               { return memblock::max_size() / sizeof(T); }
//      bool                 empty ()   const               { return memblock::empty();                }

        iterator             begin()                        { return iterator(m_Data);                  }
        const_iterator       begin()    const               { return const_iterator(m_Data);            }
        iterator             end()                          { return iterator(m_Data + m_Size);         }
        const_iterator       end()      const               { return const_iterator(m_Data + m_Size);   }

        reverse_iterator             rbegin()               { return reverse_iterator(iterator(m_Data + m_Size));               }
        const_reverse_iterator       rbegin()   const       { return const_reverse_iterator(const_iterator(m_Data + m_Size));   }
        reverse_iterator             rend()                 { return reverse_iterator(iterator(m_Data));                        }
        const_reverse_iterator       rend()     const       { return const_reverse_iterator(const_iterator(m_Data));            }

        iterator             iat(size_type i)               { UASSERT(sizeof(T) * i <= m_Size); return       iterator(m_Data) + i; }
        const_iterator       iat(size_type i) const         { UASSERT(sizeof(T) * i <= m_Size); return const_iterator(m_Data) + i; }
        reference            at(size_type i)                { UASSERT(sizeof(T) * i <  m_Size); return       pointer(m_Data)[i]; }
        const_reference      at(size_type i) const          { UASSERT(sizeof(T) * i <  m_Size); return const_pointer(m_Data)[i]; }
        reference            operator[](size_type i)        { UASSERT(sizeof(T) * i <  m_Size); return       pointer(m_Data)[i]; }
        const_reference      operator[](size_type i) const  { UASSERT(sizeof(T) * i <  m_Size); return const_pointer(m_Data)[i]; }

        reference            front()                        { UASSERT(!empty()); return       pointer(m_Data)[0]; }
        const_reference      front()     const              { UASSERT(!empty()); return const_pointer(m_Data)[0]; }
        reference            back()                         { UASSERT(!empty()); return       iterator(m_Data + m_Size)[-1]; }
        const_reference      back()     const               { UASSERT(!empty()); return const_iterator(m_Data + m_Size)[-1]; }

        void                 push_back(const T& v = T());
        void                 pop_back()                     { destroy(iterator(m_Data + m_Size) - 1); memblock::resize(m_Size - sizeof(T)); }

        void                 clear()                        { destroy(iterator(m_Data), iterator(m_Data + m_Size)); memblock::clear(); }
        void                 deallocate()                   { destroy(iterator(m_Data), iterator(m_Data + m_Size)); memblock::deallocate(); }

        void                 assign(const_iterator i1, const_iterator i2);
        void                 assign(size_type n, const T& v);
        void                 swap(vector<T>& v)             { memblock::swap(v); }
        iterator             insert(iterator ip, const T& v = T());
        iterator             insert(iterator ip, size_type n, const T& v);
        iterator             insert(iterator ip, const_iterator i1, const_iterator i2);
        iterator             erase(iterator ep, size_type n = 1);
        iterator             erase(iterator ep1, iterator ep2);
        
        const_pointer        data() const;
        pointer              data();
        
        void                 manage(pointer p, size_type n)         { memblock::manage(p, n * sizeof(T)); }
//      bool                 is_linked()     const                  { return memblock::is_linked(); }
//      void                 unlink()                               { memblock::unlink(); }
//      void                 copy_link()                            { memblock::copy_link(); }
        void                 link(const_pointer p, size_type n)     { memblock::link(p, n * sizeof(T)); }
        void                 link(pointer p, size_type n)           { memblock::link(p, n * sizeof(T)); }
        void                 link(const vector<T>& v)               { memblock::link(v); }
        void                 link(vector<T>& v)                     { memblock::link(v); }
        void                 link(const_pointer first, const_pointer last)  { memblock::link(first, last); }
        void                 link(pointer first, pointer last)              { memblock::link(first, last); }
        size_t               stream_size()     const                { return container_stream_size(*this); }
        
    protected:
        inline iterator             insert_space(iterator ip, size_type n);
    };

    /// Allocates space for at least \p n elements.
    template <typename T>
    inline void vector<T>::reserve(size_type n)
    {
        memblock::reserve(n * sizeof(T), true);
    }

    /// Resizes the vector to contain \p n elements.
    template <typename T>
    inline void vector<T>::resize(size_type n)
    {
        const size_type nb = n * sizeof(T);
        const size_type pnb = m_Size;

        if(pnb != nb)
        {
            if(m_Capacity < nb)
                memblock::reserve(nb, true);

            iterator it0 = iterator(m_Data + m_Size);
            memblock::resize(nb);
            iterator it1 = iterator(m_Data + m_Size);

            if (it1 > it0)
                construct(it0, it1);
            else
                destroy  (it0, it1);
        }
    }

    template <typename T>
    inline void vector<T>::resize(size_type n, const T& v)
    {
        const size_type nb = n * sizeof(T);
        const size_type pnb = m_Size;
        
        if (pnb != nb)
        {
            if (m_Capacity < nb)
                memblock::reserve(nb, true);

            iterator it0 = iterator(m_Data + m_Size);
            memblock::resize(nb);
            iterator it1 = iterator(m_Data + m_Size);

            uninitialized_fill(it0, it1, v);
        }
    }

    /// Initializes empty vector.
    template <typename T>
    inline vector<T>::vector()
    {
    }

    /// Initializes a vector of size \p n.
    template <typename T>
    inline vector<T>::vector(size_type n)
    {
        resize(n);
    }

    /// Copies \p n elements from \p v.
    template <typename T>
    vector<T>::vector(size_type n, const T& v)
    {
        resize(n);
        ::ustl::fill(begin(), end(), v);
    }

    /// Copies \p v.
    template <typename T>
    vector<T>::vector(const vector<T>& v)
    {
        resize(v.size());
        ::ustl::copy(v.begin(), v.end(), begin());
    }

    /// Copies range [\p i1, \p i2]
    template <typename T>
    vector<T>::vector(const_iterator i1, const_iterator i2)
    {
        resize(i2 - i1);
        ::ustl::copy(i1, i2, begin());
    }

    /// Destructor
    template <typename T>
    inline vector<T>::~vector() throw()
    {
        destroy(begin(), end());
    }

    /// Copies the range [\p i1, \p i2]
    template <typename T>
    inline void vector<T>::assign(const_iterator i1, const_iterator i2)
    {
        UASSERT(i1 <= i2);
        resize(i2 - i1);
        ::ustl::copy(i1, i2, begin());
    }

    /// Copies \p n elements with value \p v.
    template <typename T>
    inline void vector<T>::assign(size_type n, const T& v)
    {
        resize(n);
        ::ustl::fill(begin(), end(), v);
    }

    /// Copies contents of \p v.
    template <typename T>
    inline const vector<T>& vector<T>::operator=(const vector<T>& v)
    {
        assign(v.begin(), v.end());
        return *this;
    }

    /// Inserts \p n uninitialized elements at \p ip.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::insert_space(iterator ip, size_type n)
    {
        const uoff_t ipmi = memblock::iterator(ip) - m_Data;
        reserve(size() + n);
        return iterator(memblock::insert(memblock::iterator(m_Data + ipmi), n * sizeof(T)));
    }

    /// Inserts \p n elements with value \p v at offsets \p ip.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::insert(iterator ip, size_type n, const T& v)
    {
        ip = insert_space(ip, n);
        uninitialized_fill(ip, ip + n, v);
        return ip;
    }

    /// Inserts value \p v at offset \p ip.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::insert(iterator ip, const T& v)
    {
        ip = insert_space(ip, 1);
        construct(ip, v);
        return ip;
    }

    /// Inserts range [\p i1, \p i2] at offset \p ip.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::insert(iterator ip, const_iterator i1, const_iterator i2)
    {
        UASSERT(i1 <= i2);
        ip = insert_space(ip, i2 - i1);
        uninitialized_copy(i1, i2, ip);
        return ip;
    }

    /// Removes \p count elements at offset \p ep.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::erase(iterator ep, size_type n)
    {
        return iterator(memblock::erase(memblock::iterator(ep), n * sizeof(T)));
    }

    /// Removes elements from \p ep1 to \p ep2.
    template <typename T>
    inline typename vector<T>::iterator vector<T>::erase(iterator ep1, iterator ep2)
    {
        UASSERT(ep1 <= ep2);
        return iterator(memblock::erase(memblock::iterator(ep1), (ep2 - ep1) * sizeof(T)));
    }

    /// Inserts value \p v at the end of the vector.
    template <typename T>
    inline void vector<T>::push_back(const T& v)
    {
        resize(size() + 1, v);
    }

    template <typename T>
    inline const T* vector<T>::data() const
    {
        return const_pointer(m_Data);
    }

    template <typename T>
    inline T* vector<T>::data()
    {
        return pointer(m_Data);
    }

    /// Use with vector classes to allocate and link to stack space. \p n is in elements.
    #define typed_alloca_link(m,T,n)       (m).link((T*) alloca((n) * sizeof(T)), (n))
}

#endif
