//
//  File:       HLCookerTool
//
//  Function:   Tool for processing ('cooking') game data
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLArgSpec.h>
#include <CLFileSpec.h>
#include <CLJSON.h>
#include <CLSystem.h>

#include <CLData.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLTag.h>
#include <CLValue.h>

using namespace nCL;

namespace nHL
{
    bool CookTextures(cObjectValue* config, cDataStore* store);
    bool CookSounds  (cObjectValue* config, cDataStore* store);
}

int main(int argc, const char** argv)
{
    cArgSpec argSpec;
    const char* inputFileStr = 0;

    enum tOptions
    {
        kFlagProcessTextures,
        kFlagProcessSounds,
        kFlagConvertConfig,
        kFlagDumpConfig,
        kMaxFlags
    };

    argSpec.ConstructSpec
    (
         "Cooker tool for HL data",

        "<inputFile:cstr>", &inputFileStr,
            "Read the given top-level json file and process it",

         "-textures^", kFlagProcessTextures,
            "Process textures",
         "-sounds^", kFlagProcessSounds,
            "Process sounds",
         "-convert^", kFlagConvertConfig,
            "Convert config to binary form",
         "-dump^", kFlagDumpConfig,
            "Dump input",
         0
    );

    if (argSpec.Parse(argc, argv) != kArgNoError)
    {
        printf("%s\n", argSpec.HelpString(argv[0]));
        return -1;
    }

    InitTool();

    cFileSpec inputFile(inputFileStr);
    cValue value;

    string errorMessages;
    int errorLine = -1;

    if (ReadFromJSONFile(inputFile, value.AsObject(), &errorMessages, &errorLine))
    {
        printf("read %s successfully\n", inputFile.Path());

        cLink<cIConfigSource> configSource = CreateDefaultConfigSource(Allocator(kDefaultAllocator));
        ApplyImports(value.AsObject(), configSource);

        if (argSpec.Flag(kFlagProcessTextures))
        {
            printf("=== Converting Textures ===\n");
            nHL::CookTextures(value.InsertMember(CL_TAG("textures")).AsObject(), 0);
            printf("=== Done Converting Textures ===\n\n");
        }

        if (argSpec.Flag(kFlagProcessSounds))
        {
            printf("=== Converting Sounds ===\n");
            nHL::CookSounds(value.InsertMember(CL_TAG("sounds")).AsObject(), 0);
            printf("=== Done Converting Sounds ===\n\n");
        }

        if (argSpec.Flag(kFlagDumpConfig))
        {
            printf("\n");
            cJSONStyledWriter writer;
            string textOutput;

            writer.Write(value, &textOutput);
            printf("=== Text dump ===\n%s\n", textOutput.c_str());
        }
    }
    else
    {
        if (errorMessages.empty())
            fprintf(stderr, "Couldn't read %s\n", inputFile.Path());
        else
            fprintf(stderr, "Errors starting line %d:\n%s:\n", errorLine, errorMessages.c_str());
    }

    return 0;
}
