// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include <ustl/memblock.h>

#include <ustl/algorithm.h>
#include <ustl/memory.h>

#include <errno.h>

namespace ustl
{

memblock::memblock ()                           : memlink (), m_Capacity (0) { }
memblock::memblock (const void* p, size_type n) : memlink (), m_Capacity (0) { assign (p, n); }
memblock::memblock (size_type n)                : memlink (), m_Capacity (0) { resize (n); }
memblock::memblock (const cmemlink& b)          : memlink (), m_Capacity (0) { assign (b); }
memblock::memblock (const memlink& b)           : memlink (), m_Capacity (0) { assign (b); }
memblock::memblock (const memblock& b)          : memlink (), m_Capacity (0) { assign (b); }
memblock::~memblock ()                          { deallocate(); }

void memblock::unlink ()
{
    m_Capacity = 0;
    memlink::unlink();
}

/// resizes the block to \p newSize bytes, reallocating if necessary.
void memblock::resize (size_type newSize, bool bExact)
{
    if (m_Capacity < newSize)
        reserve (newSize, bExact);
    memlink::resize (newSize);
}

/// Frees internal data.
void memblock::deallocate ()
{
    if (m_Capacity)
    {
        UASSERT (cdata() && "Internal error: space allocated, but the pointer is NULL");
        UASSERT (data() && "Internal error: read-only block is marked as allocated space");

        free(data());
    }
    unlink();
}

/// Assumes control of the memory block \p p of size \p n.
/// The block assigned using this function will be freed in the destructor.
void memblock::manage (void* p, size_type n)
{
    UASSERT (p || !n);
    UASSERT (!m_Capacity && "Already managing something. deallocate or unlink first.");
    link (p, n);
    m_Capacity = n;
}

/// "Instantiate" a linked block by allocating and copying the linked data.
void memblock::copy_link ()
{
    const pointer p (begin());
    const size_t sz (size());
    if (is_linked())
        unlink();
    assign (p, sz);
}
 
/// Copies data from \p p, \p n.
void memblock::assign (const void* p, size_type n)
{
    UASSERT((p != m_Data || n <= m_Capacity) && "Self-assignment can not resize");
    resize (n);
    copy_n (pointer(p), n, iterator(m_Data));
}

/// \brief Reallocates internal block to hold at least \p newSize bytes.
///
/// Additional memory may be allocated, but for efficiency it is a very
/// good idea to call reserve before doing byte-by-byte edit operations.
/// The block size as returned by size() is not altered. reserve will not
/// reduce allocated memory. If you think you are wasting space, call
/// deallocate and start over. To avoid wasting space, use the block for
/// only one purpose, and try to get that purpose to use similar amounts
/// of memory on each iteration.
///
void memblock::reserve (size_type newSize, bool bExact)
{
    if (newSize <= m_Capacity)
        return;

    pointer oldBlock = is_linked() ? NULL : data();
    const size_t alignedSize (Align (newSize, 64));

    if (!bExact)
        newSize = alignedSize;

    pointer newBlock = (pointer) realloc (oldBlock, newSize);

    UASSERT(newBlock);
    
    if (newBlock)
    {
        if (!oldBlock && cdata())
            copy_n (cdata(), min (size() + 1, newSize), newBlock);
        link (newBlock, size());
        m_Capacity = newSize;
    }
}

/// Shifts the data in the linked block from \p start to \p start + \p n.
memblock::iterator memblock::insert (iterator start, size_type n)
{
    const uoff_t ip = start - begin();
    UASSERT (ip <= size());
    resize (size() + n, false);
    memlink::insert (iat(ip), n);
    return (iat (ip));
}

/// Shifts the data in the linked block from \p start + \p n to \p start.
memblock::iterator memblock::erase (iterator start, size_type n)
{
    UASSERT (start + n <= end());
    memlink::erase (start, n);
    memlink::resize (size() - n);
    return start;   // We don't reallocate, so this is fine.
}

}
