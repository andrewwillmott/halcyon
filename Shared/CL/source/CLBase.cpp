/*
    File:       CLBase.cpp

s    Function:   Implements CLBase.h

    Author:     Andrew Willmott

    Notes:      
*/

#include <CLBase.h>


int cDefaultObject::Link() const
{
    mRefCount++;

    return mRefCount;
}

void cDefaultObject::Unlink() const
{
    if (--mRefCount == 0)
        delete this;
}

bool cDefaultObject::GetInterface(tCLID id, void** ppInterface) const
{
    return false;
}

cIObject* ccDefaultObject::Clone() const
{
    return 0;
}

void ccDefaultObject::Free()
{
}

void ccDefaultObject::Print(ostream &s) const
{
}

void ccDefaultObject::Parse(istream &s)
{
}
