//
//  File:       CLUtilities.cpp
//
//  Function:   Various useful system-level utilities
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLUtilities.h>

#include <CLFileSpec.h>
#include <CLString.h>

#ifdef CL_APPLE
    #include <execinfo.h>
#endif

#include <unistd.h> // for chdir

using namespace nCL;

namespace
{

}


bool nCL::OpenTextFile(const char* path, int errorLine)
{
    string command;

    if (errorLine >= 0)
        command.format("/usr/local/bin/edit \"%s:%d\"", path, errorLine);    // TextWrangler always installs here?
    else
        command.format("/usr/local/bin/edit \"%s\"", path);    // TextWrangler always installs here?

    int result = system(command.c_str());

    if (result != 0)
    {
        /// Fallback
        command.format("open -t \"%s\"", path);
        result = system(command.c_str());
    }

    return result == 0;
}

// Opens directory containing text files, preferably in TextWrangler's browser mode.
bool nCL::OpenTextDirectory(const char* path)
{
    string command;

    command.format("/usr/local/bin/edit \"%s\"", path);
    int result = system(command.c_str());

    if (result != 0)
    {
        /// Fallback: open in Finder
        command.format("open \"%s\"", path);
        result = system(command.c_str());
    }

    return result == 0;
}

bool nCL::OpenForEdit(const char* path)
{
    string command(path);

    size_t slashPos = command.find_last_of(kDirectorySeparator);

    // change to directory of file to give p4 a chance to find P4CONFIG
    if (slashPos != tString::npos)
    {
        setenv("P4CONFIG", "p4config.txt", 1);
        command.resize(slashPos);
        chdir(command.c_str());
        path += slashPos + 1;
    }

    command.format("$HOME/bin/p4 edit \"%s\"", path);
    int result = system(command.c_str());

    return result == 0;
}

bool nCL::OpenForAdd(const char* path)
{
    string command(path);

    size_t slashPos = command.find_last_of(kDirectorySeparator);

    // change to directory of file to give p4 a chance to find P4CONFIG
    if (slashPos != tString::npos)
    {
        setenv("P4CONFIG", "p4config.txt", 1);
        command.resize(slashPos);
        chdir(command.c_str());
        path += slashPos + 1;
    }

    command.format("$HOME/bin/p4 add \"%s\"", path);
    int result = system(command.c_str());

    return result == 0;
}

const char* nCL::PlatformName()
{
#if   defined(CL_IOS)
    return "iOS";
#elif defined(CL_OSX)
    return "OSX";
#else
    return "Unknown";
#endif
}

const char* nCL::ConfigName()
{
#if   defined(CL_DEBUG)
    return "Debug";
#elif defined(CL_DEVELOP)
    return "Develop";
#elif defined(CL_RELEASE)
    return "Release";
#else
    return "Unknown";
#endif
}

int nCL::FindCallstack(string* callstack, int maxScopes)
{
#ifdef CL_APPLE
    void* symbols[1024];

    maxScopes++;    // for our own scope
    if (maxScopes >= CL_SIZE(symbols))
        maxScopes = CL_SIZE(symbols);

    int numSymbols = backtrace(symbols, maxScopes);
    char** callstackSymbols = backtrace_symbols(symbols, numSymbols);

    callstack->clear();

    for (int i = 1; i < numSymbols; i++)    // skip our own scope
    {
        callstack->append(callstackSymbols[i]);
        callstack->append("\n");
    }

    free(callstackSymbols);
    return numSymbols - 1;
#else
    return 0;
#endif
}
