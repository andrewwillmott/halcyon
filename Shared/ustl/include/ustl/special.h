// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef USPECIAL_H
#define USPECIAL_H

#include "ustl/vector.h"
#include "ustl/string.h"
#include "ustl/set.h"
#include "ustl/multiset.h"
#include "ustl/bitset.h"
#include "ustl/ctralgo.h"
#include "ustl/function.h"
#include <ctype.h>

namespace ustl
{

//----------------------------------------------------------------------
// Alogrithm specializations not in use by the library code.
//----------------------------------------------------------------------

template <> inline void swap (cmemlink& a, cmemlink& b)                 { a.swap (b); }
template <> inline void swap (memlink& a, memlink& b)                   { a.swap (b); }
template <> inline void swap (memblock& a, memblock& b)                 { a.swap (b); }
template <> inline void swap (string& a, string& b)                     { a.swap (b); }
#define TEMPLATE_SWAP_PSPEC(type, template_decl)        \
template_decl inline void swap (type& a, type& b) { a.swap (b); }
TEMPLATE_SWAP_PSPEC (TEMPLATE_TYPE1 (vector,T),         TEMPLATE_DECL1 (T))
TEMPLATE_SWAP_PSPEC (TEMPLATE_TYPE1 (set,T),            TEMPLATE_DECL1 (T))
TEMPLATE_SWAP_PSPEC (TEMPLATE_TYPE1 (multiset,T),       TEMPLATE_DECL1 (T))
TEMPLATE_SWAP_PSPEC (TEMPLATE_TYPE2 (tuple,N,T),        TEMPLATE_FULL_DECL2 (size_t,N,typename,T))

//----------------------------------------------------------------------
// Streamable definitions. Not used in the library and require streams.
//----------------------------------------------------------------------

//----{ pair }----------------------------------------------------------

/// \brief Takes a pair and returns pair.first
/// This is an extension, available in uSTL and the SGI STL.
template <typename Pair> struct select1st : public unary_function<Pair,typename Pair::first_type> {
    typedef typename Pair::first_type result_type;
    inline const result_type&   operator()(const Pair& a) const { return (a.first); }
    inline result_type&         operator()(Pair& a) const { return (a.first); }
};

/// \brief Takes a pair and returns pair.second
/// This is an extension, available in uSTL and the SGI STL.
template <typename Pair> struct select2nd : public unary_function<Pair,typename Pair::second_type> {
    typedef typename Pair::second_type result_type;
    inline const result_type&   operator()(const Pair& a) const { return (a.second); }
    inline result_type&         operator()(Pair& a) const { return (a.second); }
};

/// \brief Converts a const_iterator pair into an iterator pair
/// Useful for converting pair ranges returned by equal_range, for instance.
/// This is an extension, available in uSTL.
template <typename Container>
inline pair<typename Container::iterator, typename Container::iterator>
unconst (const pair<typename Container::const_iterator, typename Container::const_iterator>& i, Container&)
{
    typedef pair<typename Container::iterator, typename Container::iterator> unconst_pair_t;
    return (*noalias_cast<unconst_pair_t*>(&i));
}

//----{ vector }--------------------------------------------------------

template <typename T>
inline size_t stream_align_of (const vector<T>&)
{
    typedef typename vector<T>::written_size_type written_size_type;
    return (stream_align_of (written_size_type()));
}

//----{ tuple }---------------------------------------------------------

template <size_t N, typename T>
struct numeric_limits<tuple<N,T> > {
    typedef numeric_limits<T> value_limits;
    static inline tuple<N,T> min () { tuple<N,T> v; fill (v, value_limits::min()); return (v); }
    static inline tuple<N,T> max () { tuple<N,T> v; fill (v, value_limits::max()); return (v); }
    static const bool is_signed = value_limits::is_signed;
    static const bool is_integer = value_limits::is_integer;
    static const bool is_integral = value_limits::is_integral;
};

template <size_t N, typename T>
inline size_t stream_align_of (const tuple<N,T>&) { return (stream_align_of (NullValue<T>())); }

//----------------------------------------------------------------------

}

// This is here because there really is no other place to put it.
#if SIZE_OF_BOOL != SIZE_OF_CHAR
// bool is a big type on some machines (like DEC Alpha), so it's written as a byte.
ALIGNOF(bool, sizeof(uint8_t))
CAST_STREAMABLE(bool, uint8_t)
#endif

ALIGNOF(ustl::CBacktrace, sizeof(void*))
ALIGNOF (ustl::string, stream_align_of (string::value_type()))

#endif
