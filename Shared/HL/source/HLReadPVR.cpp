//
//  File:       HLReadPVR.cpp
//
//  Function:   Support for PVR compressed texture files
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLReadPVR.h>

#include <HLGLUtilities.h>

#include <CLLog.h>
#include <CLMemory.h>

using namespace nCL;
using namespace nHL;

namespace
{
    const size_t kPVRHeaderSize = sizeof(cPVRTextureHeaderV3);

    const uint64_t PVRTEX_PFHIGHMASK = 0xffffffff00000000ull;

    enum tPVRComponentType
    {
        kPVR_UnsignedByteNorm,
        kPVR_SignedByteNorm,
        kPVR_UnsignedByte,
        kPVR_SignedByte,
        kPVR_UnsignedShortNorm,
        kPVR_SignedShortNorm,
        kPVR_UnsignedShort,
        kPVR_SignedShort,
        kPVR_UnsignedIntegerNorm,
        kPVR_SignedIntegerNorm,
        kPVR_UnsignedInteger,
        kPVR_SignedInteger,
        kPVR_SignedFloat,	kPVR_Float=kPVR_SignedFloat, //the name kPVR_Float is now deprecated.
        kPVR_UnsignedFloat,
        kPVR_NumVarTypes
    };


    // The PVR code uses a packed 8-byte rep for pixel formats, using a uint64_t
    // The top four bytes are either 0 for a compressed texture or channel names.
    // The bottom four are the compressed format index, or channel bit counts. E.g., RGB565 is 'R' 'G' 'B' 0 5 6 5 0

    //Generate a 4 channel PixelID.
    #define PVRTGENPIXELID4(C1Name, C2Name, C3Name, C4Name, C1Bits, C2Bits, C3Bits, C4Bits) ( ( (uint64_t)C1Name) + ( (uint64_t)C2Name<<8) + ( (uint64_t)C3Name<<16) + ( (uint64_t)C4Name<<24) + ( (uint64_t)C1Bits<<32) + ( (uint64_t)C2Bits<<40) + ( (uint64_t)C3Bits<<48) + ( (uint64_t)C4Bits<<56) )
    #define PVRTGENPIXELID3(C1Name, C2Name, C3Name, C1Bits, C2Bits, C3Bits)( PVRTGENPIXELID4(C1Name, C2Name, C3Name, 0, C1Bits, C2Bits, C3Bits, 0) )
    #define PVRTGENPIXELID2(C1Name, C2Name, C1Bits, C2Bits) ( PVRTGENPIXELID4(C1Name, C2Name, 0, 0, C1Bits, C2Bits, 0, 0) )
    #define PVRTGENPIXELID1(C1Name, C1Bits) ( PVRTGENPIXELID4(C1Name, 0, 0, 0, C1Bits, 0, 0, 0))

    const void GetGLES2TextureFormat(const cPVRTextureHeaderV3* textureHeader, GLenum& internalformat, GLenum& format, GLenum& type)
    {
        uint64_t pixelFormat = textureHeader->mPixelFormat;

        //Initialisation. Any invalid formats will return 0 always.
        format = 0;
        type = 0;
        internalformat = 0;

        //Get the last 32 bits of the pixel format.
        uint64_t pixelFormatPartHigh = pixelFormat & PVRTEX_PFHIGHMASK;

        //Check for a compressed format (The first 8 bytes will be 0, so the whole thing will be equal to the last 32 bits).
        if (pixelFormatPartHigh == 0)
        {
            //Format and type == 0 for compressed textures.
            switch (pixelFormat)
            {
        #if GL_IMG_texture_compression_pvrtc
            case kPF_PVRTCI_2bpp_RGB:
                {
                    internalformat=GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
                    return;
                }
            case kPF_PVRTCI_2bpp_RGBA:
                {
                    internalformat=GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
                    return;
                }
            case kPF_PVRTCI_4bpp_RGB:
                {
                    internalformat=GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
                    return;
                }
            case kPF_PVRTCI_4bpp_RGBA:
                {
                    internalformat=GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
                    return;
                }
        #endif
        #ifdef GL_ETC1_RGB8_OES
            case kPF_ETC1:
                {
                    internalformat=GL_ETC1_RGB8_OES;
                    return;
                }
        #endif
            default:
                return;
            }
        }

        tPVRComponentType channelType = (tPVRComponentType) textureHeader->mChannelType;

        switch (channelType)
        {
        case kPVR_Float:
            {
                switch (pixelFormat)
                {
            #ifdef GL_HALF_FLOAT_OES
                case PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16):
                    {
                        type=GL_HALF_FLOAT_OES;
                        format = GL_RGBA;
                        internalformat=GL_RGBA;
                        return;
                    }
                case PVRTGENPIXELID3('r', 'g', 'b', 16, 16, 16):
                    {
                        type=GL_HALF_FLOAT_OES;
                        format = GL_RGB;
                        internalformat=GL_RGB;
                        return;
                    }
                case PVRTGENPIXELID2('l', 'a', 16, 16):
                    {
                        type=GL_HALF_FLOAT_OES;
                        format = GL_LUMINANCE_ALPHA;
                        internalformat=GL_LUMINANCE_ALPHA;
                        return;
                    }
                case PVRTGENPIXELID1('l', 16):
                    {
                        type=GL_HALF_FLOAT_OES;
                        format = GL_LUMINANCE;
                        internalformat=GL_LUMINANCE;
                        return;
                    }
                case PVRTGENPIXELID1('a', 16):
                    {
                        type=GL_HALF_FLOAT_OES;
                        format = GL_ALPHA;
                        internalformat=GL_ALPHA;
                        return;
                    }
            #endif
                case PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32):
                    {
                        type=GL_FLOAT;
                        format = GL_RGBA;
                        internalformat=GL_RGBA;
                        return;
                    }
                case PVRTGENPIXELID3('r', 'g', 'b', 32, 32, 32):
                    {
                        type=GL_FLOAT;
                        format = GL_RGB;
                        internalformat=GL_RGB;
                        return;
                    }
                case PVRTGENPIXELID2('l', 'a', 32, 32):
                    {
                        type=GL_FLOAT;
                        format = GL_LUMINANCE_ALPHA;
                        internalformat=GL_LUMINANCE_ALPHA;
                        return;
                    }
                case PVRTGENPIXELID1('l', 32):
                    {
                        type=GL_FLOAT;
                        format = GL_LUMINANCE;
                        internalformat=GL_LUMINANCE;
                        return;
                    }
                case PVRTGENPIXELID1('a', 32):
                    {
                        type=GL_FLOAT;
                        format = GL_ALPHA;
                        internalformat=GL_ALPHA;
                        return;
                    }
                }
                break;
            }
        case kPVR_UnsignedByteNorm:
            {
                type = GL_UNSIGNED_BYTE;
                switch (pixelFormat)
                {
                case PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8):
                    {
                        format = internalformat = GL_RGBA;
                        return;
                    }
                case PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8):
                    {
                        format = internalformat = GL_RGB;
                        return;
                    }
                case PVRTGENPIXELID2('l', 'a', 8, 8):
                    {
                        format = internalformat = GL_LUMINANCE_ALPHA;
                        return;
                    }
                case PVRTGENPIXELID1('l', 8):
                    {
                        format = internalformat = GL_LUMINANCE;
                        return;
                    }
                case PVRTGENPIXELID1('a', 8):
                    {
                        format = internalformat = GL_ALPHA;
                        return;
                    }
                case PVRTGENPIXELID4('b', 'g', 'r', 'a', 8, 8, 8, 8):
                    {
                        format = internalformat = GL_BGRA;
                        return;
                    }
                }
                break;
            }
        case kPVR_UnsignedShortNorm:
            {
                switch (pixelFormat)
                {
                case PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4):
                    {
                        type = GL_UNSIGNED_SHORT_4_4_4_4;
                        format = internalformat = GL_RGBA;
                        return;
                    }
                case PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1):
                    {
                        type = GL_UNSIGNED_SHORT_5_5_5_1;
                        format = internalformat = GL_RGBA;
                        return;
                    }
                case PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5):
                    {
                        type = GL_UNSIGNED_SHORT_5_6_5;
                        format = internalformat = GL_RGB;
                        return;
                    }
                }
                break;
            }
        default:
            return;
        }
    }

    void GetMinimumDimensionsForFormat(uint64_t pixelFormat, uint32_t &minX, uint32_t &minY, uint32_t &minZ)
    {
        switch(pixelFormat)
        {
        case kPF_DXT1:
        case kPF_DXT2:
        case kPF_DXT3:
        case kPF_DXT4:
        case kPF_DXT5:
        case kPF_BC4:
        case kPF_BC5:
        case kPF_ETC1:
        case kPF_ETC2_RGB:
        case kPF_ETC2_RGBA:
        case kPF_ETC2_RGB_A1:
        case kPF_EAC_R11_Unsigned:
        case kPF_EAC_R11_Signed:
        case kPF_EAC_RG11_Unsigned:
        case kPF_EAC_RG11_Signed:
            minX = 4;
            minY = 4;
            minZ = 1;
            break;
        case kPF_PVRTCI_4bpp_RGB:
        case kPF_PVRTCI_4bpp_RGBA:
            minX = 8;
            minY = 8;
            minZ = 1;
            break;
        case kPF_PVRTCI_2bpp_RGB:
        case kPF_PVRTCI_2bpp_RGBA:
            minX = 16;
            minY = 8;
            minZ = 1;
            break;
        case kPF_PVRTCII_4bpp:
            minX = 4;
            minY = 4;
            minZ = 1;
            break;
        case kPF_PVRTCII_2bpp:
            minX = 8;
            minY = 4;
            minZ = 1;
            break;
        case kPF_UYVY:
        case kPF_YUY2:
        case kPF_RGBG8888:
        case kPF_GRGB8888:
            minX = 2;
            minY = 1;
            minZ = 1;
            break;
        case kPF_BW1bpp:
            minX = 8;
            minY = 1;
            minZ = 1;
            break;
        default: //Non-compressed formats all return 1.
            minX = 1;
            minY = 1;
            minZ = 1;
            break;
        }
    }

    uint32_t GetBitsPerPixel(uint64_t pixelFormat)
    {
        if ((pixelFormat & PVRTEX_PFHIGHMASK) != 0)
        {
            uint8_t* PixelFormatChar = (uint8_t*) &pixelFormat;

            return PixelFormatChar[4] + PixelFormatChar[5] + PixelFormatChar[6] + PixelFormatChar[7];
        }
        else
        {
            switch (pixelFormat)
            {
            case kPF_BW1bpp:
                return 1;
            case kPF_PVRTCI_2bpp_RGB:
            case kPF_PVRTCI_2bpp_RGBA:
            case kPF_PVRTCII_2bpp:
                return 2;
            case kPF_PVRTCI_4bpp_RGB:
            case kPF_PVRTCI_4bpp_RGBA:
            case kPF_PVRTCII_4bpp:
            case kPF_ETC1:
            case kPF_EAC_R11_Unsigned:
            case kPF_EAC_R11_Signed:
            case kPF_ETC2_RGB:
            case kPF_ETC2_RGB_A1:
            case kPF_DXT1:
            case kPF_BC4:
                return 4;
            case kPF_DXT2:
            case kPF_DXT3:
            case kPF_DXT4:
            case kPF_DXT5:
            case kPF_BC5:
            case kPF_EAC_RG11_Unsigned:
            case kPF_EAC_RG11_Signed:
            case kPF_ETC2_RGBA:
                return 8;
            case kPF_YUY2:
            case kPF_UYVY:
            case kPF_RGBG8888:
            case kPF_GRGB8888:
                return 16;
            case kPF_SharedExponentR9G9B9E5:
                return 32;
            case kMaxPVRPixelFormats:
                return 0;
            }
        }

        return 0;
    }

    uint32_t GetMIPLevelSize(const cPVRTextureHeaderV3* textureHeader, int mipLevel)
    {
        // The smallest divisible sizes for a pixel format
        uint32_t smallestWidth  = 1;
        uint32_t smallestHeight = 1;
        uint32_t smallestDepth  = 1;

        uint64_t pixelFormatPartHigh = textureHeader->mPixelFormat & PVRTEX_PFHIGHMASK;
        
        //If the pixel format is compressed, get the pixel format's minimum dimensions.
        if (pixelFormatPartHigh == 0)
            GetMinimumDimensionsForFormat((tPVRPixelFormats)textureHeader->mPixelFormat, smallestWidth, smallestHeight, smallestDepth);

        uint64_t uiDataSize = 0;    // for large texture support

        if (mipLevel == -1)
        {
            for (uint8_t currentMIP = 0; currentMIP < textureHeader->mMIPMapCount; ++currentMIP)
            {
                //Get the dimensions of the current MIP Map level.
                uint32_t width  = max(1, textureHeader->mWidth  >> currentMIP);
                uint32_t height = max(1, textureHeader->mHeight >> currentMIP);
                uint32_t depth  = max(1, textureHeader->mDepth  >> currentMIP);

                //If pixel format is compressed, the dimensions need to be padded.
                if (pixelFormatPartHigh == 0)
                {
                    width  += -width  % smallestWidth ;
                    height += -height % smallestHeight;
                    depth  += -depth  % smallestDepth ;
                }

                //Add the current MIP Map's data size to the total.
                uiDataSize += (uint64_t) GetBitsPerPixel(textureHeader->mPixelFormat) * (uint64_t) width * (uint64_t) height * (uint64_t) depth;
            }
        }
        else
        {
            //Get the dimensions of the specified MIP Map level.
            uint32_t uiWidth  = max(1, textureHeader->mWidth  >> mipLevel);
            uint32_t uiHeight = max(1, textureHeader->mHeight >> mipLevel);
            uint32_t uiDepth  = max(1, textureHeader->mDepth  >> mipLevel);

            //If pixel format is compressed, the dimensions need to be padded.
            if (pixelFormatPartHigh == 0)
            {
                uiWidth  = uiWidth  +( (-1 * uiWidth ) % smallestWidth);
                uiHeight = uiHeight +( (-1 * uiHeight) % smallestHeight);
                uiDepth  = uiDepth  +( (-1 * uiDepth ) % smallestDepth);
            }

            //Work out the specified MIP Map's data size
            uiDataSize = GetBitsPerPixel(textureHeader->mPixelFormat)*uiWidth*uiHeight*uiDepth;
        }

        return (uint32_t)(uiDataSize / 8);
    }
}

bool nHL::LoadPVRTexture
(
    const void* pointer,
    uint        textureName,
    int         skipMIPs,
    bool        allowDecompress
)
{
    //Compression bools
    bool isCompressedFormatSupported = false;
    bool isCompressedFormat = false;

    //Get the header from the main pointer.
    const cPVRTextureHeaderV3* textureHeader = (const cPVRTextureHeaderV3*) pointer;
    const uint8_t* textureData = (const uint8_t*) pointer + kPVRHeaderSize + textureHeader->mMetaDataSize;

    //Setup GL Texture format values.
    GLenum textureFormat = 0;
    GLenum textureInternalFormat = 0;  // often this is the same as textureFormat, but not for BGRA8888 on iOS, for instance
    GLenum textureType = 0;

    //Get the OGLES format values.
    GetGLES2TextureFormat(textureHeader, textureInternalFormat, textureFormat, textureType);

    //Check supported texture formats.
#ifdef CL_IOS
    bool isPVRTCSupported      = true;
    bool isBGRA8888Supported   = true;
    bool isFloat16Supported    = true;
    bool isFloat32Supported    = true;
    bool isETCSupported        = false;
#elif CL_OSX
    bool isPVRTCSupported      = false;
    bool isBGRA8888Supported   = true;
    bool isFloat16Supported    = true;
    bool isFloat32Supported    = true;
    bool isETCSupported        = false;
#else
    bool isPVRTCSupported      = IsGLExtensionSupported("GL_IMG_texture_compression_pvrtc");
    bool isBGRA8888Supported   = IsGLExtensionSupported("GL_IMG_texture_format_BGRA8888");
    bool isFloat16Supported    = IsGLExtensionSupported("GL_OES_texture_half_float");
    bool isFloat32Supported    = IsGLExtensionSupported("GL_OES_texture_float");
    bool isETCSupported        = IsGLExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture");
#endif

    // Check compressed formats
    if (textureFormat == 0 && textureType == 0 && textureInternalFormat != 0)
    {
    #if GL_IMG_texture_compression_pvrtc
        if (textureInternalFormat >= GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG && textureInternalFormat <= GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
        {
            //Check for PVRTCI support.
            if (isPVRTCSupported)
            {
                isCompressedFormatSupported = true;
                isCompressedFormat = true;
            }
        }
    #endif
    #ifdef GL_ETC1_RGB8_OES
        else if (textureInternalFormat == GL_ETC1_RGB8_OES)
        {
            if (isETCSupported)
            {
                isCompressedFormatSupported = true;
                isCompressedFormat = true;
            }
        }
    #endif
    }

    if (textureFormat == GL_BGRA)
    {
    #ifdef TARGET_OS_IPHONE
        textureInternalFormat = GL_RGBA;
    #endif

        if (!isBGRA8888Supported)
        {
            CL_LOG_E("PVR", "Unable to load GL_BGRA texture as extension GL_IMG_texture_format_BGRA8888 is unsupported.\n");
            return false;
        }
    }

#ifdef GL_HALF_FLOAT_OES
    //Check for floating point textures
    if (textureType == GL_HALF_FLOAT_OES && !isFloat16Supported)
    {
        CL_LOG_E("PVR", "Unable to load GL_HALF_FLOAT_OES texture as extension GL_OES_texture_half_float is unsupported.\n");
        return false;
    }
#endif

    if (textureType == GL_FLOAT && !isFloat32Supported)
    {
        CL_LOG_E("PVR", "Unable to load GL_FLOAT texture as extension GL_OES_texture_float is unsupported.\n");
        return false;
    }

    //Deal with unsupported texture formats
    if (textureInternalFormat == 0)
    {
        CL_LOG_E("PVR", "pixel type not supported.\n");
        return false;
    }

    //PVR files are never row aligned.
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    //Initialise a texture target.
    GLint target = GL_TEXTURE_2D;
    
    if (textureHeader->mNumFaces > 1)
    {
        target = GL_TEXTURE_CUBE_MAP;
    }

    //Check if this is a texture array.
    if (textureHeader->mNumSurfaces > 1)
    {
        CL_LOG_E("PVR", "Texture arrays are not available in OGLES2.0.\n");
        return false;
    }

    //Bind the 2D texture
    glBindTexture(target, textureName);

    GL_CHECK;

    //Initialise the current MIP size.
    uint32_t currentMIPSize = 0;

    //Loop through the faces
    //Check if this is a cube map.
    if (textureHeader->mNumFaces > 1)
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    //Initialise the width/height
    uint32_t mipWidth  = textureHeader->mWidth  >> skipMIPs;
    uint32_t mipHeight = textureHeader->mHeight >> skipMIPs;


    const uint8_t* surfaceData = textureData;

    for (int mipLevel = skipMIPs; mipLevel < textureHeader->mMIPMapCount; mipLevel++)
    {
        //Get the current MIP size.
        currentMIPSize = GetMIPLevelSize(textureHeader, mipLevel);

        GLint textureTarget = target;

        for (int face = 0; face < textureHeader->mNumFaces; face++)
        {
            //Upload the texture
            if (isCompressedFormat && isCompressedFormatSupported)
            {
                glCompressedTexImage2D(textureTarget, mipLevel - skipMIPs, textureInternalFormat, mipWidth, mipHeight, 0, currentMIPSize, surfaceData);
            }
            else
            {
                glTexImage2D(textureTarget, mipLevel - skipMIPs, textureInternalFormat, mipWidth, mipHeight, 0, textureFormat, textureType, surfaceData);
            }

            surfaceData += currentMIPSize;

            textureTarget++;
        }

        mipWidth  = ustl::max(1, mipWidth >> 1);
        mipHeight = ustl::max(1, mipHeight >> 1);

        if (glGetError())
        {
            CL_LOG_E("PVR", "glTexImage2D() failed.\n");
            return false;
        }
    }

    if (glGetError())
    {
        CL_LOG_E("PVR", "glTexImage2D() failed.\n");
        return false;
    }
    
    return true;
}

bool nHL::LoadPVRTexture
(
    const char*         path,
    uint                textureName            ///< GL texture to update
)
{
    cMappedFileInfo mapInfo = MapFile(path);

    if (!mapInfo.mData)
        return false;

    bool result = LoadPVRTexture(mapInfo.mData, textureName, 0, false);

    UnmapFile(mapInfo);

    return result;
}

