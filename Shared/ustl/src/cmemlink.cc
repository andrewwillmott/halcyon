// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ustl/cmemlink.h"

/// Compares to memory block pointed by l. Size is compared first.
bool ustl::cmemlink::operator== (const cmemlink& l) const
{
    return (l.m_Size == m_Size &&
            (l.m_Data == m_Data || 0 == memcmp (l.m_Data, m_Data, m_Size)));
}
