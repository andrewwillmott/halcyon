// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef USET_H
#define USET_H

#include "vector.h"

namespace ustl
{

/// \class set uset.h ustl.h
/// \ingroup Sequences
///
/// \brief Unique sorted container. Sorted vector with all values unique.
///
template <typename T, typename Comp = less<T> >
class set : public vector<T>
{
public:
    typedef const set<T,Comp>&                          rcself_t;
    typedef vector<T>                                   base_class;
    typedef typename base_class::value_type             key_type;
    typedef typename base_class::value_type             data_type;
    typedef typename base_class::value_type             value_type;
    typedef typename base_class::size_type              size_type;
    typedef typename base_class::pointer                pointer;
    typedef typename base_class::const_pointer          const_pointer;
    typedef typename base_class::reference              reference;
    typedef typename base_class::const_reference        const_reference;
    typedef typename base_class::const_iterator         const_iterator;
    typedef typename base_class::iterator               iterator;
    typedef typename base_class::reverse_iterator       reverse_iterator;
    typedef typename base_class::const_reverse_iterator const_reverse_iterator;
    typedef pair<iterator,bool>                         insertrv_t;

public:
    inline                      set ()                  : base_class()   { }
    explicit inline             set (size_type n)       : base_class (n) { }
    inline                      set (rcself_t v)        : base_class (v) { }
    inline                      set (const_iterator i1, const_iterator i2) : base_class() { insert (i1, i2); }

    inline rcself_t             operator= (rcself_t v)  { base_class::operator= (v); return (*this); }

    inline void                 assign    (const_iterator i1, const_iterator i2)    { base_class::clear(); insert (i1, i2); }
    inline void                 push_back (const_reference v)                       { insert (v); }

    inline const_iterator       find  (const_reference v) const;
    inline iterator             find  (const_reference v)               { return (const_cast<iterator>(const_cast<rcself_t>(*this).find (v))); }

    insertrv_t                  insert(const_reference v);
    inline iterator             insert(iterator, const_reference v)     { return (insert(v).first); }
    inline void                 insert(const_iterator i1, const_iterator i2);

    inline iterator             erase (const_iterator it)               { return iterator(memblock::erase(memblock::iterator(it), sizeof(T))); }
    inline size_t               erase (const_reference v);
};


/// Finds p in container
template <typename T, typename Comp>
inline typename set<T,Comp>::const_iterator set<T, Comp>::find(const_reference v) const
{
    Comp less;

    const_iterator i = lower_bound (base_class::begin(), base_class::end(), v, less);
    const_iterator itEnd = base_class::end();

    if ((i != itEnd) && !less(v, *i))
        return i;

    return itEnd;
}

/// Inserts \p v into the container, maintaining the sort order.
template <typename T, typename Comp>
typename set<T,Comp>::insertrv_t set<T,Comp>::insert (const_reference v)
{
    Comp f;
    iterator ip = lower_bound (base_class::begin(), base_class::end(), v, f);
    bool bInserted = (ip == base_class::end() || f(v, *ip));
    if (bInserted)
        ip = base_class::insert (ip, v);
    return (make_pair (ip, bInserted));
}

/// Inserts the contents of range [i1,i2)
template <typename T, typename Comp>
inline void set<T,Comp>::insert (const_iterator i1, const_iterator i2)
{
    UASSERT(i1 <= i2);
    base_class::reserve (base_class::size() + (i2 - i1));
    for (; i1 < i2; ++i1)
        push_back (*i1);
}

template <typename T, typename Comp>
inline size_t set<T,Comp>::erase(const_reference v)
{
    iterator ip = find(v);

    if (ip != base_class::end())
    {
        base_class::erase(ip);
        return 1;
    }

    return 0;
}

}

#endif
