//
//  File:       CLSystem.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#include <CLSystem.h>

#include <CLImage.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLTag.h>

namespace
{

}


void nCL::InitTool()
{
    InitAllocators();

    // RegisterDirectory(kDirectoryResources, resourcePath);
    // RegisterDirectory(kDirectoryApp, appSpec.Directory());

    InitLogSystem();
    InitTagSystem(Allocator(kDefaultAllocator));
    InitImageSystem();
}

void nCL::ShutdownTool()
{
    ShutdownImageSystem();

    ShutdownTagSystem();
    ShutdownLogSystem();

    ShutdownAllocators();
}
