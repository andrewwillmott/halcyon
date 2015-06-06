//
//  File:       CLServices.h
//
//  Function:   Provides access to various CL singletons
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2004
//

#ifndef CL_SERVICES_H
#define CL_SERVICES_H

#include <CLDefs.h>

namespace nCL
{
    struct cServices
    {
        // ...
    };
}

const nCL::cServices* CL(); ///< Accessor for global services.

nCL::cServices* CLSetup();  ///< Accessor for services for setup and teardown.

#endif
