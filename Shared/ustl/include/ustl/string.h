// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef USTRING_H
#define USTRING_H

#include "memblock.h"
#include "utf8.h"
#include <stdarg.h>     // for va_list, va_start, and va_end (in string::format)

namespace ustl {

/// \class string ustring.h ustl.h
/// \ingroup Sequences
///
/// \brief STL basic_string&lt;char&gt; equivalent.
///
/// An STL container for text string manipulation.
/// Differences from C++ standard:
///     - string is a class, not a template. Wide characters are assumed to be
///             encoded with utf8 at all times except when rendering or editing,
///             where you would use a utf8 iterator.
///     - format member function - you can, of course use an \ref ostringstream,
///             which also have format functions, but most of the time this way
///             is more convenient. Because uSTL does not implement locales,
///             format is the only way to create localized strings.
///     - const char* cast operator. It is much clearer to use this than having
///             to type .c_str() every time.
///     - length returns the number of _characters_, not bytes.
///             This function is O(N), so use wisely.
///
/// An additional note is in order regarding the use of indexes. All indexes
/// passed in as arguments or returned by find are byte offsets, not character
/// offsets. Likewise, sizes are specified in bytes, not characters. The
/// rationale is that there is no way for you to know what is in the string.
/// There is no way for you to know how many characters are needed to express
/// one thing or another. The only thing you can do to a localized string is
/// search for delimiters and modify text between them as opaque blocks. If you
/// do anything else, you are hardcoding yourself into a locale! So stop it!
///
class string : public memblock
{
public:
    typedef char                value_type;
    typedef value_type*         pointer;
    typedef const value_type*   const_pointer;
    typedef wchar_t             wvalue_type;
    typedef wvalue_type*        wpointer;
    typedef const wvalue_type*  const_wpointer;
    typedef pointer             iterator;
    typedef const_pointer       const_iterator;
    typedef value_type&         reference;
    typedef value_type          const_reference;
    typedef ::ustl::reverse_iterator<iterator>          reverse_iterator;
    typedef ::ustl::reverse_iterator<const_iterator>    const_reverse_iterator;
    typedef utf8in_iterator<const_iterator>             utf8_iterator;
    static const size_type npos = INT_MAX;              ///< Value that means the end of string.

public:
                            string ()               : memblock () { relink ("",0); }
                            string (const string& s);
                            string (const string& s, uoff_t o, size_type n);
    explicit                string (const cmemlink& l);
                            string (const_pointer s);
                            string (const_pointer s, size_type len);
                            string (const_pointer s1, const_pointer s2);
    explicit                string (size_type n, value_type c = 0);
    
    pointer                 data ()             { return (string::pointer      (m_Data)); }
    const_pointer           c_str () const      { return (string::const_pointer(m_Data)); }
    void                    reserve(size_type n);
    void                    resize (size_type n);
    void                    resize (size_type n, value_type c);
    void                    clear ()            { resize (0); }
    const_iterator          begin () const      { return (const_iterator(m_Data)); }
    iterator                begin ()            { return (iterator      (m_Data)); }
    const_iterator          end () const        { return (const_iterator(m_Data + m_Size)); }
    iterator                end ()              { return (iterator      (m_Data + m_Size)); }
    const_reverse_iterator  rbegin () const     { return (const_reverse_iterator(m_Data + m_Size)); }
    reverse_iterator        rbegin ()           { return (reverse_iterator      (iterator(m_Data + m_Size))); }
    const_reverse_iterator  rend () const       { return (const_reverse_iterator(m_Data)); }
    reverse_iterator        rend ()             { return (reverse_iterator      (iterator(m_Data))); }
    utf8_iterator           utf8_begin () const { return (utf8_iterator (m_Data)); }
    utf8_iterator           utf8_end () const   { return (utf8_iterator (m_Data + m_Size)); }
    const_reference         at (uoff_t pos) const   { UASSERT (pos <= m_Size && begin()); return (const_iterator(m_Data)[pos]); }
    reference               at (uoff_t pos)         { UASSERT (pos <= m_Size && begin()); return (iterator      (m_Data)[pos]); }
    const_iterator          iat (uoff_t pos) const  { return (begin() + (__builtin_constant_p(pos) && pos >= npos ? m_Size : min(pos,m_Size))); }
    iterator                iat (uoff_t pos)        { return (const_cast<iterator>(const_cast<const string*>(this)->iat(pos))); }
    const_iterator          wiat (uoff_t i) const;
    iterator                wiat (uoff_t i)         { return (const_cast<iterator>(const_cast<const string*>(this)->wiat(i))); }
    const_reference         back () const       { return (at(m_Size - 1)); }
    reference               back ()             { return (at(m_Size - 1)); }
    size_type               length () const     { return (distance (utf8_begin(), utf8_end())); }
    
    void                    append (const_iterator i1, const_iterator i2)   { append (i1, distance (i1, i2)); }
    void                    append (const_pointer s, size_type len);
    void                    append (const_pointer s);
    void                    append (size_type n, const_reference c);
    void                    append (size_type n, wvalue_type c)             { insert (m_Size, c, n); }
    void                    append (const_wpointer s1, const_wpointer s2)   { insert (m_Size, s1, s2); }
    void                    append (const_wpointer s)                       { const_wpointer se (s); for (;se&&*se;++se) ; append (s, se); }
    void                    append (const string& s)                        { append (s.begin(), s.end()); }
    void                    append (const string& s, uoff_t o, size_type n) { append (s.iat(o), s.iat(o+n)); }
    void                    assign (const_iterator i1, const_iterator i2)   { assign (i1, distance (i1, i2)); }
    void                    assign (const_pointer s, size_type len);
    void                    assign (const_pointer s);
    void                    assign (const_wpointer s1, const_wpointer s2)   { clear(); append (s1, s2); }
    void                    assign (const_wpointer s1)                      { clear(); append (s1); }
    void                    assign (const string& s)                        { assign (s.begin(), s.end()); }
    void                    assign (const string& s, uoff_t o, size_type n) { assign (s.iat(o), s.iat(o+n)); }
    size_type               copyto (pointer p, size_type n, const_iterator start = NULL) const;
    int                     compare (const string& s) const { return (compare (begin(), end(), s.begin(), s.end())); }
    int                     compare (const_pointer s) const { return (compare (begin(), end(), s, s + strlen(s))); }
    static int              compare (const_iterator first1, const_iterator last1, const_iterator first2, const_iterator last2);
    
                            operator const value_type* () const;
                            operator value_type* ();
    const string&           operator= (const string& s)     { assign (s.begin(), s.end()); return (*this); }
    const string&           operator= (const_reference c)   { assign (&c, 1); return (*this); }
    const string&           operator= (const_pointer s)     { assign (s); return (*this); }
    const string&           operator= (const_wpointer s)    { assign (s); return (*this); }
    const string&           operator+= (const string& s)    { append (s.begin(), s.m_Size); return (*this); }
    const string&           operator+= (const_reference c)  { append (1, c); return (*this); }
    const string&           operator+= (const_pointer s)    { append (s); return (*this); }
    const string&           operator+= (wvalue_type c)      { append (1, c); return (*this); }
    const string&           operator+= (const_wpointer s)   { append (s); return (*this); }
    string                  operator+ (const string& s) const;
    bool                    operator== (const string& s) const      { return (memblock::operator== (s)); }
    bool                    operator== (const_pointer s) const;
    bool                    operator== (const_reference c) const    { return (m_Size == 1 && c == at(0)); }
    bool                    operator!= (const string& s) const      { return (!operator== (s)); }
    bool                    operator!= (const_pointer s) const      { return (!operator== (s)); }
    bool                    operator!= (const_reference c) const    { return (!operator== (c)); }
    bool                    operator< (const string& s) const       { return (0 > compare (s)); }
    bool                    operator< (const_pointer s) const       { return (0 > compare (s)); }
    bool                    operator< (const_reference c) const     { return (0 > compare (begin(), end(), &c, &c + 1)); }
    bool                    operator> (const_pointer s) const       { return (0 < compare (s)); }
    
    void                    insert (const uoff_t ip, wvalue_type c, size_type n = 1);
    void                    insert (const uoff_t ip, const_wpointer first, const_wpointer last, const size_type n = 1);
    iterator                insert (iterator start, const_reference c, size_type n = 1);
    iterator                insert (iterator start, const_pointer s, size_type n = 1);
    iterator                insert (iterator start, const_pointer first, const_iterator last, size_type n = 1);
    void                    insert (uoff_t ip, const_pointer s, size_type nlen)             { insert (iat(ip), s, s + nlen); }
    void                    insert (uoff_t ip, size_type n, value_type c)                   { insert (iat(ip), c, n); }
    void                    insert (uoff_t ip, const string& s, uoff_t sp, size_type slen)  { insert (iat(ip), s.iat(sp), s.iat(sp + slen)); }

    iterator                erase (const_iterator p);
    string&                 erase (uoff_t epo = 0, size_type n = npos);
    iterator                erase (const_iterator first, const_iterator last);

    void                    push_back (const_reference c)   { append (1, c); }
    void                    push_back (wvalue_type c)       { append (1, c); }
    void                    pop_back ()                 { resize (m_Size - 1); }
    void                    replace (iterator first, iterator last, const_pointer s);
    void                    replace (iterator first, iterator last, const_pointer i1, const_pointer i2, size_type n = 1);
    void                    replace (iterator first, iterator last, const string& s)                        { replace (first, last, s.begin(), s.end()); }
    void                    replace (iterator first, iterator last, const_pointer s, size_type slen)        { replace (first, last, s, s + slen); }
    void                    replace (iterator first, iterator last, size_type n, value_type c)              { replace (first, last, &c, &c + 1, n); }
    void                    replace (uoff_t rp, size_type n, const string& s)                               { replace (iat(rp), iat(rp + n), s); }
    void                    replace (uoff_t rp, size_type n, const string& s, uoff_t sp, size_type slen)    { replace (iat(rp), iat(rp + n), s.iat(sp), s.iat(sp + slen)); }
    void                    replace (uoff_t rp, size_type n, const_pointer s, size_type slen)               { replace (iat(rp), iat(rp + n), s, s + slen); }
    void                    replace (uoff_t rp, size_type n, const_pointer s)                               { replace (iat(rp), iat(rp + n), string(s)); }
    void                    replace (uoff_t rp, size_type n, size_type count, value_type c)                 { replace (iat(rp), iat(rp + n), count, c); }
    string                  substr (uoff_t o, size_type n = npos) const     { return (string (*this, o, n)); }

    uoff_t                  find (const_reference c, uoff_t pos = 0) const;
    uoff_t                  find (const string& s, uoff_t pos = 0) const;
    uoff_t                  rfind(const_reference c, uoff_t pos = npos) const;
    uoff_t                  rfind(const string& s, uoff_t pos = npos) const;
    uoff_t                  find_first_of (const string& s, uoff_t pos = 0) const;
    uoff_t                  find_first_not_of (const string& s, uoff_t pos = 0) const;
    uoff_t                  find_last_of (const string& s, uoff_t pos = npos) const;
    uoff_t                  find_last_of (const_pointer s, uoff_t pos = npos) const;
    uoff_t                  find_last_of (const_reference c, uoff_t pos = npos) const;
    uoff_t                  find_last_not_of (const string& s, uoff_t pos = npos) const;
    uoff_t                  find_last_not_of (const_reference c, uoff_t pos = npos) const;

    string&                 vformat (const char* fmt, va_list args);
    string&                 append_vformat (const char* fmt, va_list args);
    string&                 format (const char* fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
    string&                 append_format (const char* fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

    size_t                  stream_size () const;

    static hashvalue_t      hash (const char* f1, const char* l1);
};

//----------------------------------------------------------------------

/// Assigns itself the value of string \p s
inline string::string (const string& s) : memblock()
{
    if (s.is_linked())  // Assume this guy is a constant -- don't copy
        relink (s.c_str(), s.m_Size);
    else
        assign(s);
}

/// Links to \p s
inline string::string (const_pointer s) : memblock ()
{
    assign(s);
}

/// Creates a string of length \p n filled with character \p c.
inline string::string (size_type n, value_type c) : memblock ()
{
    resize(n, c);
}


/// Assigns itself the value of string \p s
inline string::string (const cmemlink& s) : memblock ()
{
    assign (const_iterator (s.begin()), s.m_Size);
}

/// Assigns itself a [o,o+n) substring of \p s.
inline string::string(const string& s, uoff_t o, size_type n) : memblock()
{
    assign (s, o, n);
}

/// Copies the value of \p s of length \p len into itself.
inline string::string(const_pointer s, size_type len) : memblock ()
{
    assign (s, len);
}

/// Copies into itself the string data between \p s1 and \p s2
inline string::string (const_pointer s1, const_pointer s2) : memblock ()
{
    UASSERT (s1 <= s2 && "Negative ranges result in memory allocation errors.");
    assign (s1, s2);
}

/// Assigns itself the value of string \p s
inline void string::assign (const_pointer s)
{
    assign(s, strlen(s));
}

/// Assigns itself the value of string \p s of length \p len.
inline void string::assign (const_pointer s, size_type len)
{
    resize (len);
    copy_n (s, len, pointer(m_Data));
}

/// Appends to itself the value of string \p s of length \p len.
inline void string::append (const_pointer s)
{
    append (s, strlen (s));
}

/// Appends to itself the value of string \p s of length \p len.
inline void string::append (const_pointer s, size_type len)
{
    resize (m_Size + len);
    copy_n (s, len, end() - len);
}

/// Appends to itself \p n characters of value \p c.
inline void string::append (size_type n, value_type c)
{
    resize (m_Size + n);
    fill_n (end() - n, n, c);
}

/// Returns the pointer to the first character.
inline string::operator const string::value_type* () const
{
    UASSERT ((!end() || !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking.");
    return m_Data;
}

/// Returns the pointer to the first character.
inline string::operator string::value_type* ()
{
    UASSERT ((end() && !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking.");
    return iterator(m_Data);
}

/// Concatenates itself with \p s
inline string string::operator+ (const string& s) const
{
    string result (*this);
    result += s;
    return (result);
}

/// Resize to \p n and fill new entries with \p c
inline void string::resize (size_type n, value_type c)
{
    const size_type oldn = m_Size;
    resize (n);
    fill_n (iat(oldn), max(ssize_t(n - oldn), 0), c);
}

//----------------------------------------------------------------------
// Operators needed to avoid comparing pointer to pointer

#define PTR_STRING_CMP(op, impl)        \
inline bool op (const char* s1, const string& s2) { return impl; }

PTR_STRING_CMP (operator==, (s2 == s1))
PTR_STRING_CMP (operator!=, (s2 != s1))
PTR_STRING_CMP (operator<,  (s2 >  s1))
PTR_STRING_CMP (operator<=, (s2 >= s1))
PTR_STRING_CMP (operator>,  (s2 <  s1))
PTR_STRING_CMP (operator>=, (s2 <= s1))

#undef PTR_STRING_CMP

inline string operator+ (const char* cs, const string& ss)
{
    string r;
    r.reserve (strlen(cs) + ss.m_Size);
    r += cs;
    r += ss;
    return (r);
}

//----------------------------------------------------------------------

inline hashvalue_t hash_value (const char* first, const char* last)
{ return (string::hash (first, last)); }
inline hashvalue_t hash_value (const char* v)
{ return (hash_value (v, v + strlen(v))); }

//----------------------------------------------------------------------

}

#endif
