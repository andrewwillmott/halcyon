//
//  File:       HLReadPVR.h
//
//  Function:   PVR image format support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_READ_PVR_H
#define HL_READ_PVR_H

#include <HLDefs.h>

namespace nHL
{
    enum tPVRPixelFormats
    {
        kPF_PVRTCI_2bpp_RGB,
        kPF_PVRTCI_2bpp_RGBA,
        kPF_PVRTCI_4bpp_RGB,
        kPF_PVRTCI_4bpp_RGBA,
        kPF_PVRTCII_2bpp,
        kPF_PVRTCII_4bpp,
        kPF_ETC1,
        kPF_DXT1,
        kPF_DXT2,
        kPF_DXT3,
        kPF_DXT4,
        kPF_DXT5,

        //These formats are identical to some DXT formats.
        kPF_BC1 = kPF_DXT1,
        kPF_BC2 = kPF_DXT3,
        kPF_BC3 = kPF_DXT5,

        //These are currently unsupported:
        kPF_BC4,
        kPF_BC5,
        kPF_BC6,
        kPF_BC7,

        //These are supported
        kPF_UYVY,
        kPF_YUY2,
        kPF_BW1bpp,
        kPF_SharedExponentR9G9B9E5,
        kPF_RGBG8888,
        kPF_GRGB8888,
        kPF_ETC2_RGB,
        kPF_ETC2_RGBA,
        kPF_ETC2_RGB_A1,
        kPF_EAC_R11_Unsigned,
        kPF_EAC_R11_Signed,
        kPF_EAC_RG11_Unsigned,
        kPF_EAC_RG11_Signed,

        kMaxPVRPixelFormats
    };

    const uint32_t kPVRTEX3_ID            = 0x03525650;   // 'P''V''R'3
    const uint32_t kPVRTEX3_ID_REV        = 0x50565203;   // If endianness is backwards then PVR3 will read as 3RVP, hence why it is written as an int.

    // PVR Header file flags.
    const uint32_t kPVRTEX3_Compressed    = (1 << 0);       //  Texture has been file compressed using PVRTexLib (currently unused)
    const uint32_t kPVRTEX3_Premultiplied = (1 << 1);       //  Texture has been premultiplied by alpha value.

    // Mip Map level specifier constants. Other levels are specified by 1,2...n
    const uint32_t kPVRTEX_TopMIPLevel          = 0;
    const uint32_t kPVRTEX_AllMIPLevels         = -1; //This is a special number used simply to return a total of all MIP levels when dealing with data sizes.

    struct cPVRTextureHeaderV3
    {
        uint32_t    mVersion        = kPVRTEX3_ID;  // Version of the file header, used to identify it.
        uint32_t    mFlags          = 0;            // Various format flags.
        uint64_t    mPixelFormat    = kMaxPVRPixelFormats;       //The pixel format, 8cc value storing the 4 channel identifiers and their respective sizes.
        uint32_t    mColourSpace    = 0;            // The cColour Space of the texture, currently either linear RGB or sRGB.
        uint32_t    mChannelType    = 0;            // Variable type that the channel is stored in. Supports signed/unsigned int/short/byte or float for now.
        uint32_t    mHeight         = 1;            // Height of the texture.
        uint32_t    mWidth          = 1;            // Width of the texture.
        uint32_t    mDepth          = 1;            // Depth of the texture. (Z-slices)
        uint32_t    mNumSurfaces    = 1;            // Number of members in a Texture Array.
        uint32_t    mNumFaces       = 1;            // Number of faces in a Cube Map. Maybe be a value other than 6.
        uint32_t    mMIPMapCount    = 1;            // Number of MIP Maps in the texture - NB: Includes top level.
        uint32_t    mMetaDataSize   = 0;            // Size of the accompanying meta data.
    };

    bool LoadPVRTexture
    (
        const char*         path,
        uint                textureName            ///< GL texture to update
    );

    bool LoadPVRTexture
    (
        const void* textureData,
        uint        textureName,                ///< GL texture to update
        int         skipMIPs = 0,               ///< Number of top mip levels to skip if any
        bool        allowDecompress = false     ///< Allow decompression of textures without hardware support
    );

}

#endif
