//
//  File:       HLReadObj.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_READ_OBJ_H
#define HL_READ_OBJ_H

#include <CLDefs.h>

namespace nCL
{
    class cFileSpec;
}

namespace nHL
{
    class cGLMeshInfo;
    bool LoadObj(cGLMeshInfo* meshInfo, const nCL::cFileSpec& spec);

}

#endif
