//
//  File:       HLAudioCook.cpp
//
//  Function:   Convert standard audio sound file config setup to cooked data
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#include <CLData.h>
#include <CLFileSpec.h>
#include <CLImage.h>
#include <CLLog.h>
#include <CLValue.h>

using namespace nCL;

namespace nHL
{
    bool CookSounds(cObjectValue* config, cDataStore* store);
}

namespace
{
    enum tCompressionFormat
    {
        kFormatSFX,
        kFormatMusic,
        kMaxFormats
    };

    const cEnumInfo kCompressionFormatEnum[] =
    {
        "sfx",      kFormatSFX,
        "music",    kFormatMusic,
        0, 0
    };

    const char* const kCompressionOptions[] =
    {
        "-f caff -d ima4 -c 2",  // music, stereo
        "-f caff -d ima4 -c 1",  // music, stereo
        "-f caff -d LEI16@22050 -c 2", // sfx: drop sample rate to 22k.
        "-f caff -d LEI16@22050 -c 1", // sfx: drop sample rate to 22k.
    };
}

bool nHL::CookSounds(cObjectValue* config, cDataStore* store)
{
    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        const char* filePath = info->Member("file").AsString();

        if (filePath)
        {
            cFileSpec spec;
            FindSpec(&spec, c, filePath);

            CL_LOG("Cooker", "Processing sound %s @ %s\n", name, spec.Path());

            if (!spec.HasExtension())
                spec.SetExtension("aif");

            const char* compressionFormat = info->Member("compression").AsString();

            // Just call out to afconvert?

            
        }
    }

    return true;
}

