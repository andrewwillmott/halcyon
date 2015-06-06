//
//  File:       HLTextureCook.cpp
//
//  Function:   Convert standard texture file config setup to cooked data
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
    bool CookTextures(cObjectValue* config, cDataStore* store);
}

bool nHL::CookTextures(cObjectValue* config, cDataStore* store)
{
    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        // tBufferFormat format = tBufferFormat(AsEnum(info->Member("type"), kBufferFormatEnum, kFormatRGBA));

        // int numChannels = FormatNumChannels(format);

        cImage32 image32;
        cImage8 image8;
        const char* texturePath = info->Member("image").AsString();

        if (texturePath)
        {
            cFileSpec spec;

            FindSpec(&spec, c, texturePath);

            if (!spec.HasExtension())
                spec.SetExtension("png");

            CL_LOG("Cooker", "Processing texture %s @ %s\n", name, spec.Path());

            Vec2i textureSize = vl_0;
            const void* data = 0;

            // TODO: factor this out of renderer, or take cue from compression setting?
            // tBufferFormat format = tBufferFormat(AsEnum(info->Member("type"), kBufferFormatEnum, kFormatRGBA));
            // int numChannels = FormatNumChannels(format);
            int numChannels = 4;


            if (numChannels == 4)
            {
                if (LoadImage(spec, &image32))
                {
                    CL_LOG("Cooker", "  read %s, %d x %d\n", spec.Path(), image32.mW, image32.mH);
                    
                    data = image32.mData;
                    textureSize = { image32.mW, image32.mH };
                }
                else
                    CL_LOG_E("Cooker", "  failed to load %s\n", spec.Path());
            }
            else if (numChannels == 1)
            {
                if (LoadImage(spec, &image8))
                {
                    CL_LOG("Cooker", "  read %s, %d x %d\n", spec.Path(), image8.mW, image8.mH);

                    data = image8.mData;
                    textureSize = { image8.mW, image8.mH };
                }
                else
                    CL_LOG("Cooker", "Failed to load %s\n", spec.Path());
            }
        }

        const char* compressionFormat = info->Member("compression").AsString();

        // TODO: convert to PVR (etc.)
        // TODO: rewrite image to point to the compressed file, or add compressedImage: ?
    }

    return true;
}

