// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ustl/memlink.h"

namespace ustl
{

/// Fills the linked block with the given pattern.
/// \arg start   Offset at which to start filling the linked block
/// \arg p       Pointer to the pattern.
/// \arg elSize  Size of the pattern.
/// \arg elCount Number of times to write the pattern.
/// Total number of bytes written is \p elSize * \p elCount.
///
void memlink::fill(iterator start, const void* p, size_type elSize, size_type elCount)
{
    UASSERT(data() || !elCount || !elSize);
    UASSERT(start >= begin() && start + elSize * elCount <= end());
    if (elSize == 1)
        fill_n (start, elCount, *reinterpret_cast<const uint8_t*>(p));
    else while (elCount--)
        start = copy_n (const_iterator(p), elSize, start);
}

}
