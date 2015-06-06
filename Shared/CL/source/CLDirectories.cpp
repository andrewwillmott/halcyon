//
//  File:       CLDirectories.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLDirectories.h>

#include <CLSTL.h>

using namespace nCL;

namespace
{
    typedef map<uint32_t, tString> tDirectories;

    tDirectories sDirectories;
}

void InitDirectories()
{
}

void ShutdownDirectories()
{
    sDirectories.clear();
}


void nCL::RegisterDirectory(uint32_t id, const cFileSpec& spec)
{
    sDirectories[id] = spec.Directory();
}

void nCL::RegisterDirectory(uint32_t id, tStrConst directory)
{
    sDirectories[id] = directory;
}

tStrConst nCL::Directory(uint32_t id)
{
    tDirectories::iterator it = sDirectories.find(id);

    if (it != sDirectories.end())
        return it->second.c_str();

    return 0;
}

bool nCL::SetDirectory(cFileSpec* spec, uint32_t id)
{
    tDirectories::iterator it = sDirectories.find(id);

    if (it != sDirectories.end())
    {
        spec->SetDirectory(it->second.c_str());
        return true;
    }

    return false;
}

