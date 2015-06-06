//
//  File:       ReadLXO.h
//
//  Function:   Support for reading Modo LXO files
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef READ_LXO_H
#define READ_LXO_H

#include <stdint.h>

namespace nCL
{
    class cFileSpec;
}

namespace nHL
{
    class cGLMeshInfo;
    uint32_t LoadLXOScene(cGLMeshInfo* meshInfo, const char* fileName);
    uint32_t LoadLXOScene(cGLMeshInfo* meshInfo, const nCL::cFileSpec& spec);
}

#endif
