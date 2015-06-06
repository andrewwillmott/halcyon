// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ustl/string.h"
#include <stdio.h>      // for vsnprintf (in string::format)

namespace ustl {

//----------------------------------------------------------------------

const uoff_t string::npos;

//----------------------------------------------------------------------

/// Resize to \p n and fill new entries with \p c
void string::reserve(size_type n)
{
    if (n <= m_Capacity)
        return;

    const_pointer oldBlock(m_Capacity ? m_Data : 0);
    const size_t newSize(Align(n + 1, 64));     // +1 for terminating 0
    
    pointer newBlock = (pointer) realloc((void*) oldBlock, newSize);

    UASSERT(newBlock);
    
    if (newBlock)
    {
        if (!oldBlock && m_Data)    // if we alloc'd
            copy_n(m_Data, min(m_Size, newSize), newBlock);

        link(newBlock, m_Size);

        m_Capacity = newSize - 1;
    }
}

/// Resize the string to \p n characters. New space contents is undefined.
void string::resize(size_type n)
{
    if (!(n | m_Capacity))  // n == 0 and we don't already have storage
        return relink("", 0);

    if (m_Capacity < n)
        reserve(n);

    m_Size = n;
    at(n) = 0;  // ensure we have terminating 0. 
}

/// Copies into itself at offset \p start, the value of string \p p of length \p n.
string::size_type string::copyto(pointer p, size_type n, const_iterator start) const
{
    UASSERT(p && n);
    if (!start)
        start = begin();
    const size_type btc = min(n, m_Size);
    copy_n(start, btc, p);
    p[btc] = 0;

    return btc;
}

/// Returns comparison value regarding string \p s.
/// The return value is:
/// \li 1 if this string is greater (by value, not length) than string \p s
/// \li 0 if this string is equal to string \p s
/// \li -1 if this string is less than string \p s
///
/*static*/ int string::compare (const_iterator first1, const_iterator last1, const_iterator first2, const_iterator last2)
{
    UASSERT (first1 <= last1 && (first2 <= last2 || !last2) && "Negative ranges result in memory allocation errors.");

    const size_type len1 = distance (first1, last1);
    const size_type len2 = distance (first2, last2);

    const int rvbylen = sign (int(len1 - len2));

    int rv = memcmp (first1, first2, min (len1, len2));

    return (rv ? rv : rvbylen);
}

/// Returns true if this string is equal to string \p s.
bool string::operator== (const_pointer s) const
{
    if (!s)
        s = "";

    return (m_Size == strlen(s) && 0 == memcmp (c_str(), s, m_Size));
}

/// Returns the beginning of character \p i.
string::const_iterator string::wiat (uoff_t i) const
{
    utf8in_iterator<string::const_iterator> cfinder (begin());

    cfinder += i;

    return cfinder.base();
}

/// Inserts wide character \p c at \p ipo \p n times as a UTF-8 string.
///
/// \p ipo is a byte position, not a character position, and is intended
/// to be obtained from one of the find functions. Generally you are not
/// able to know the character position in a localized string; different
/// languages will have different character counts, so use find instead.
///
void string::insert (const uoff_t ipo, wchar_t c, size_type n)
{
    iterator ip (iat(ipo));

    ip = iterator(memblock::insert(memblock::iterator(ip), n * Utf8Bytes(c)));
    fill_n(utf8out(ip), n, c);

    *end() = 0;
}

/// Inserts sequence of wide characters at \p ipo (byte position from a find call)
void string::insert(const uoff_t ipo, const wchar_t* first, const wchar_t* last, const size_type n)
{
    iterator ip(iat(ipo));

    size_type nti = distance(first, last);
    size_type bti = 0;

    for (uoff_t i = 0; i < nti; ++ i)
        bti += Utf8Bytes(first[i]);

    ip = iterator(memblock::insert(memblock::iterator(ip), n * bti));

    utf8out_iterator<string::iterator> uout (utf8out (ip));

    for (uoff_t j = 0; j < n; ++ j)
        for (uoff_t k = 0; k < nti; ++ k, ++ uout)
            *uout = first[k];

    *end() = 0;
}

/// Inserts character \p c into this string at \p start.
string::iterator string::insert(iterator start, const_reference c, size_type n)
{
    start = iterator(memblock::insert(memblock::iterator(start), n));

    fill_n(start, n, c);
    *end() = 0;

    return start;
}

/// Inserts \p count instances of string \p s at offset \p start.
string::iterator string::insert (iterator start, const_pointer s, size_type n)
{
    if (!s) s = "";
    return (insert (start, s, s + strlen(s), n));
}

/// Inserts [first,last] \p n times.
string::iterator string::insert (iterator start, const_pointer first, const_pointer last, size_type n)
{
    UASSERT (first <= last);
    UASSERT (begin() <= start && end() >= start);
    UASSERT ((first < begin() || first >= end() || m_Size + abs_distance(first,last) < capacity()) && "Insertion of self with autoresize is not supported");
    start = iterator (memblock::insert (memblock::iterator(start), distance(first, last) * n));
    fill (memblock::iterator(start), first, distance(first, last), n);
    *end() = 0;
    return (start);
}

/// Erases \p size bytes at \p ep.
string::iterator string::erase(const_iterator ep)
{
    string::iterator rv = memblock::erase(memblock::iterator(ep), 1);
    *end() = 0;
    return (rv);
}

/// Erases \p size bytes at \p ep.
string::iterator string::erase(const_iterator first, const_iterator last)
{
    string::iterator rv = memblock::erase(memblock::iterator(first), last - first);
    *end() = 0;
    return (rv);
}

/// Erases \p n bytes at byte offset \p epo.
string& string::erase(uoff_t epo, size_type n)
{
    size_type maxN = m_Size - epo;
    if (n > maxN)
        n = maxN;

    memblock::erase(memblock::iterator(iat(epo)), n);
    *end() = 0;
    return *this;
}

/// Replaces range [\p start, \p start + \p len] with string \p s.
void string::replace (iterator first, iterator last, const_pointer s)
{
    if (!s) s = "";
    replace (first, last, s, s + strlen(s));
}

/// Replaces range [\p start, \p start + \p len] with \p count instances of string \p s.
void string::replace (iterator first, iterator last, const_pointer i1, const_pointer i2, size_type n)
{
    UASSERT (first <= last);
    UASSERT (n || distance(first, last));
    UASSERT (first >= begin() && first <= end() && last >= first && last <= end());
    UASSERT ((i1 < begin() || i1 >= end() || abs_distance(i1,i2) * n + m_Size < capacity()) && "Replacement by self can not autoresize");
    const size_type bte = distance(first, last), bti = distance(i1, i2) * n;
    if (bti < bte)
        first = iterator (memblock::erase (memblock::iterator(first), bte - bti));
    else if (bte < bti)
        first = iterator (memblock::insert (memblock::iterator(first), bti - bte));
    fill (memblock::iterator(first), i1, distance(i1, i2), n);
    *end() = 0;
}

/// Returns the offset of the first occurence of \p c after \p pos.
uoff_t string::find (const_reference c, uoff_t pos) const
{
    const_iterator found = ::ustl::find (iat(pos), end(), c);
    return (found < end() ? (uoff_t) distance(begin(),found) : npos);
}

/// Returns the offset of the first occurence of substring \p s of length \p n after \p pos.
uoff_t string::find (const string& s, uoff_t pos) const
{
    if (s.empty() || s.m_Size > m_Size - pos)
        return (npos);
    const uoff_t endi = s.m_Size - 1;
    const_reference endchar = s[endi];
    uoff_t lastPos = endi;
    while (lastPos-- && s[lastPos] != endchar) ;
    const size_type skip = endi - lastPos;
    const_iterator i = iat(pos) + endi;
    for (; i < end() && (i = ::ustl::find (i, end(), endchar)) < end(); i += skip)
        if (memcmp (i - endi, s.c_str(), s.m_Size) == 0)
            return (distance (begin(), i) - endi);
    return (npos);
}

/// Returns the offset of the last occurence of character \p c before \p pos.
uoff_t string::rfind (const_reference c, uoff_t pos) const
{
    if (!empty())
        for (int i = min(pos,m_Size-1); i >= 0; --i)
            if (at(i) == c)
                return (i);

    return (npos);
}

/// Returns the offset of the last occurence of substring \p s of size \p n before \p pos.
uoff_t string::rfind (const string& s, uoff_t pos) const
{
    const_iterator d = iat(pos) - 1;
    const_iterator sp = begin() + s.m_Size - 1;
    const_iterator m = s.end() - 1;
    for (long int i = 0; d > sp && size_type(i) < s.m_Size; -- d)
        for (i = 0; size_type(i) < s.m_Size; ++ i)
            if (m[-i] != d[-i])
                break;
    return (d > sp ? (uoff_t) distance (begin(), d + 2 - s.m_Size) : npos);
}

/// Returns the offset of the first occurence of one of characters in \p s of size \p n after \p pos.
uoff_t string::find_first_of (const string& s, uoff_t pos) const
{
    for (uoff_t i = min(pos,m_Size); i < m_Size; ++ i)
        if (s.find (at(i)) != npos)
            return (i);
    return (npos);
}

/// Returns the offset of the first occurence of one of characters not in \p s of size \p n after \p pos.
uoff_t string::find_first_not_of (const string& s, uoff_t pos) const
{
    for (uoff_t i = min(pos,m_Size); i < m_Size; ++ i)
        if (s.find (at(i)) == npos)
            return (i);
    return (npos);
}

/// Returns the offset of the last occurence of one of characters in \p s of size \p n before \p pos.
uoff_t string::find_last_of (const string& s, uoff_t pos) const
{
    if (!empty())
        for (int i = min(pos,m_Size-1); i >= 0; -- i)
            if (s.find (at(i)) != npos)
                return (i);

    return (npos);
}

uoff_t string::find_last_of (const_reference c, uoff_t pos) const
{
    if (!empty())
        for (int i = min(pos,m_Size-1); i >= 0; -- i)
            if (at(i) == c)
                return (i);
    return (npos);
}

/// Returns the offset of the last occurence of one of characters not in \p s of size \p n before \p pos.
uoff_t string::find_last_not_of (const string& s, uoff_t pos) const
{
    if (!empty())
        for (int i = min(pos,m_Size-1); i >= 0; -- i)
            if (s.find (at(i)) == npos)
                return (i);

    return (npos);
}

uoff_t string::find_last_not_of (const_reference c, uoff_t pos) const
{
    if (!empty())
        for (int i = min(pos,m_Size-1); i >= 0; -- i)
            if (at(i) != c)
                return (i);
    return (npos);
}

/// Equivalent to a vsprintf on the string.
string& string::vformat (const char* format, va_list args)
{
#if HAVE_VA_COPY
    va_list args2;
#else
    #define args2 args
    #undef __va_copy
    #define __va_copy(x,y)
#endif

    __va_copy (args2, args);
    size_t rv = vsnprintf ((char*) m_Data, m_Capacity ? m_Capacity + 1 : 0, format, args2);

    if (rv > m_Capacity)
    {
        reserve (rv);
        __va_copy (args2, args);
        rv = vsnprintf ((char*) m_Data, m_Capacity + 1, format, args2);
    }

    resize(rv);

    return *this;
}

string& string::append_vformat(const char* format, va_list args)
{
    size_t os = m_Size;

    char dummy[1];  // in case vsnprintf writes a null even though size=0
    size_t fs = vsnprintf(dummy, 0, format, args);

    resize(os + fs);
    vsnprintf (data() + os, fs + 1, format, args);

    return *this;
}

/// Equivalent to a sprintf on the string.
string& string::format(const char* fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    vformat(fmt, args);
    va_end (args);

    return *this;
}

/// Equivalent to a sprintf on the string.
string& string::append_format(const char* fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    append_vformat(fmt, args);
    va_end (args);

    return *this;
}

/// Returns the number of bytes required to write this object to a stream.
size_t string::stream_size () const
{
    return (Utf8Bytes(m_Size) + m_Size);
}

/// Returns a hash value for [first, last)
hashvalue_t string::hash (const char* first, const char* last)
{
    hashvalue_t h = 0;
    // This has the bits flowing into each other from both sides of the number
    for (; first < last; ++ first)
        h = *first + ((h << 7) | (h >> (BitsInType(hashvalue_t) - 7)));
    return (h);
}

}
