//
//  File:       HLServices.cpp
//
//  Function:   Access to HL singletons.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLServices.h>

#include <string.h> // for memset

namespace
{
    struct nHL::cServices sServices;
}

nHL::cServices::cServices()
{
    memset(this, 0, sizeof(*this));
}

nHL::cServices::~cServices()
{
}

const nHL::cServices* HL()
{
    return &sServices;
}

nHL::cServices* HLServiceSetup()
{
    return &sServices;
}
