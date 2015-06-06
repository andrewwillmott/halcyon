//
//  File:       HLServices.cpp
//
//  Function:   Access to CL singletons.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2004-2013
//

#include <CLServices.h>

namespace
{
    struct nCL::cServices sServices;
}

const nCL::cServices* CL()
{
    return &sServices;
}

nCL::cServices* CLServiceSetup()
{
    return &sServices;
}
