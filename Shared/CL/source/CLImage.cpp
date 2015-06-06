//
//  File:       CLImage.cpp
//
//  Function:   Image data support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLImage.h>

#include <CLBits.h>
#include <CLFileSpec.h>
#include <CLMemory.h>

#define STBI_NO_STDIO
#include "stb_image.h"

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

//#define EMBED_DEBUG(M_FORMAT...) printf(M_FORMAT)
#define EMBED_DEBUG(M_FORMAT...)

using namespace nCL;

namespace
{
    struct cPixel32
    {
        union
        {
            struct
            {
                uint8_t mR;
                uint8_t mG;
                uint8_t mB;
                uint8_t mA;
            };
            uint8_t  mChannel[4];
            uint32_t mAsUInt32;
        };
    };

    class cSTBAlloc : public cIAllocator
    {
    public:
        virtual uint8_t* Alloc(size_t size, size_t align = 4) override { return 0; }
        virtual void     Free (void* p) override
        {
            stbi_image_free(p);
        }
    };
    cSTBAlloc sSTBAlloc;

    enum tImageKind
    {
        kImagePPM,
        kImageTIFF,
        kImageJPEG,
        kImageGIF,
        kImagePNG,
        kImageTGA,
        kImagePSD,
        kImageRGBE,
        kImageBMP,
        kMaxImageKinds
    };

    cFileSpecExtension kImageExtensions[] =
    {
        "png",      kImagePNG,
        "jpg",      kImageJPEG,
        "jpeg",     kImageJPEG,
        "gif",      kImageGIF,
        "tga",      kImageTGA,
        "bmp",      kImageBMP,
        "psd",      kImagePSD,
        "pic",      kImageRGBE,
        "hdr",      kImageRGBE,
        0, 0
    };

    // stb supports jpeg, png, bmp, tga, psd, hdr, pic, gif

}

#ifdef STBI_NO_STDIO

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImage8* info)
{
    cMappedFileInfo mapInfo = MapFile(spec.Path());

    if (!mapInfo.mData)
        return false;

    int n;
    info->mData = stbi_load_from_memory(mapInfo.mData, mapInfo.mSize, &info->mW, &info->mH, &n, 1);

    UnmapFile(mapInfo);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImage32* info)
{
    cMappedFileInfo mapInfo = MapFile(spec.Path());

    if (!mapInfo.mData)
        return false;

    int n;
    info->mData = (uint32_t*) stbi_load_from_memory(mapInfo.mData, mapInfo.mSize, &info->mW, &info->mH, &n, 4);

    UnmapFile(mapInfo);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImageFloat* info)
{
    cMappedFileInfo mapInfo = MapFile(spec.Path());

    if (!mapInfo.mData)
        return false;

    int n;
    info->mData = stbi_loadf_from_memory(mapInfo.mData, mapInfo.mSize, &info->mW, &info->mH, &info->mN, 1);

    UnmapFile(mapInfo);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

#else

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImage8* info)
{
    int n;
    info->mData = stbi_load(spec.Path(), &info->mW, &info->mH, &n, 1);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImage32* info)
{
    int n;
    info->mData = (uint32_t*) stbi_load(spec.Path(), &info->mW, &info->mH, &n, 4);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

bool nCL::LoadImage(const nCL::cFileSpec& spec, cImageFloat* info)
{
    int n;
    info->mData = stbi_loadf(spec.Path(), &info->mW, &info->mH, &info->mN, 1);

    if (info->mData)
    {
        info->mAllocator = &sSTBAlloc;
        return true;
    }

    return false;
}

#endif

bool nCL::FindImageExtension(nCL::cFileSpec* spec)
{
    return spec->FindFileExtension(kImageExtensions, false) >= 0;
}


namespace
{
    // Tiny PNG writer
    // http://www.w3.org/TR/PNG

    mz_bool OutputToVectorFunc(const void* pData, int len, void* pUser)
    {
        const uint8_t* data = (const uint8_t*) pData;
        vector<uint8_t>* v  = (vector<uint8_t>*) pUser;

        v->insert(v->end(), data, data + len);

        return MZ_TRUE;
    }

    const uint8_t kChannelsToPNGType[] =
    {
        0,      // placeholder
        0,      // Greyscale
        4,      // Greyscale+alpha
        2,      // TrueColour (RGB)
        6       // TrueColour+alpha (RGBA)
    };

    size_t CompressToPNG(cIAllocator* alloc, const void* imageData, int w, int h, int numChannels, vector<uint8_t>* outputBuffer)
    {
        outputBuffer->clear();

        tdefl_compressor* compressor = (tdefl_compressor*) alloc->Alloc(sizeof(tdefl_compressor));

        if (!compressor)
            return NULL;

        int rowStride = w * numChannels;

        outputBuffer->reserve(57 + max(64, (1 + rowStride) * h));

        // write dummy header
        outputBuffer->resize(41, 0);

        // compress image data
        tdefl_init(compressor, OutputToVectorFunc, outputBuffer, TDEFL_DEFAULT_MAX_PROBES | TDEFL_WRITE_ZLIB_HEADER);

        uint8_t filterMethod = 0;
        for (int y = 0; y < h; ++y)
        {
            tdefl_compress_buffer(compressor, &filterMethod, 1, TDEFL_NO_FLUSH);
            tdefl_compress_buffer(compressor, (uint8_t*) imageData + y * rowStride, rowStride, TDEFL_NO_FLUSH);
        }

        if (tdefl_compress_buffer(compressor, NULL, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE)
        {
            alloc->Free(compressor);
            return NULL;
        }

        // write real header
        // length32, type32 (ABCD), data, CRC32
        size_t dataSize = outputBuffer->size() - 41;

        uint8_t header[41] =
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,     // standard 8-byte header: ?PNG \r\n ? \r
            0x00, 0x00, 0x00, 0x0d,     // IHDR length
            0x49, 0x48, 0x44, 0x52,     // IHDR
            0, 0,
            (uint8_t) (w >> 8),
            (uint8_t) w,
            0,0,
            (uint8_t) (h >> 8),
            (uint8_t) h,
            8,  // bit depth per channel
            kChannelsToPNGType[numChannels], // colour format. No BGRA etc. =P
            0,              // compression method
            0,              // filter method
            0,              // interlace method
            0, 0, 0, 0,     // CRC32

            (uint8_t) (dataSize >> 24),    // IDAT size
            (uint8_t) (dataSize >> 16),
            (uint8_t) (dataSize >> 8),
            (uint8_t)  dataSize,
            0x49, 0x44, 0x41, 0x54 // IDAT
        };

        uint32_t c = (uint32_t) mz_crc32(MZ_CRC32_INIT, header + 12, 17);

        for (int i = 0; i < 4; ++i, c <<= 8)
            (header + 29)[i] = (c >> 24);

        memcpy(outputBuffer->data(), header, 41);

        // TODO: write SRGB chunk

        // write footer (IDAT CRC-32, followed by IEND chunk)
        const uint8_t footer[] =
        {
            0, 0, 0, 0,                 // CRC32
            0, 0, 0, 0,                 // size=0
            0x49, 0x45, 0x4e, 0x44,     // IEND
            0xae, 0x42, 0x60, 0x82      // CRC32
        };

        outputBuffer->insert(outputBuffer->end(), (const uint8_t*) footer, (const uint8_t*) footer + sizeof(footer));

        c = (uint32_t) mz_crc32(MZ_CRC32_INIT, outputBuffer->iat(41 - 4), dataSize + 4);

        for (int i = 0; i < 4; ++i, c <<= 8)
            outputBuffer->at(outputBuffer->size() - 16 + i) = (c >> 24);

        alloc->Free(compressor);
        return outputBuffer->size();
    }
}

bool nCL::SaveImage(const cImage8& image, cFileSpec& spec)
{
    vector<uint8_t> imageData;
    size_t dataSize = CompressToPNG(Allocator(kDefaultAllocator), image.mData, image.mW, image.mH, 1, &imageData);

    if (!dataSize)
        return false;

    if (!spec.HasExtension())
        spec.AddExtension("png");

    FILE* file = spec.FOpen("wb");
    if (!file)
        return false;

    bool success = true;

    if (fwrite(imageData.data(), imageData.size(), 1, file) == 0)
        success = false;
    if (fclose(file) != 0)
        success = false;

    return success;
}

bool nCL::SaveImage(const cImage32& image, cFileSpec& spec)
{
    vector<uint8_t> imageData;
    size_t dataSize = CompressToPNG(Allocator(kDefaultAllocator), image.mData, image.mW, image.mH, 4, &imageData);

    if (!dataSize)
        return false;

    if (!spec.HasExtension())
        spec.AddExtension("png");

    FILE* file = spec.FOpen("wb");
    if (!file)
        return false;

    bool success = true;

    if (fwrite(imageData.data(), imageData.size(), 1, file) == 0)
        success = false;
    if (fclose(file) != 0)
        success = false;

    return success;
}

size_t nCL::CompressImage(const cImage8& image, nCL::cIAllocator* alloc, vector<uint8_t>* data)
{
    return CompressToPNG(alloc, image.mData, image.mW, image.mH, 1, data);
}

size_t nCL::CompressImage(const cImage32& image, nCL::cIAllocator* alloc, vector<uint8_t>* data)
{
    return CompressToPNG(alloc, image.mData, image.mW, image.mH, 4, data);
}


namespace
{
    void FlipImage(int w, int h, uint8_t* data)
    {
        int stride = w;

        for (int i = 0; i < h / 2; i++)
        {
            uint8_t* row0 = data + stride * i;
            uint8_t* row1 = data + stride * (h - i - 1);

            int j = 0;

            for ( ; j < ((stride + 3) & ~3); j += 1)    // advance to 32-bit boundary
            {
                uint8_t w0 = *(row0 + j);
                uint8_t w1 = *(row1 + j);

                *(row0 + j) = w1;
                *(row1 + j) = w0;
            }

            for ( ; j < stride - 3; j += 4)
            {
                uint32_t w0 = *reinterpret_cast<uint32_t*>(row0 + j);
                uint32_t w1 = *reinterpret_cast<uint32_t*>(row1 + j);

                *reinterpret_cast<uint32_t*>(row0 + j) = w1;
                *reinterpret_cast<uint32_t*>(row1 + j) = w0;
            }

            for ( ; j < stride; j += 1)
            {
                uint8_t w0 = *(row0 + j);
                uint8_t w1 = *(row1 + j);

                *(row0 + j) = w1;
                *(row1 + j) = w0;
            }
        }
    }

    void FlipImage(int w, int h, uint32_t* data)
    {
        // Much easier, we can assume word alignment.
        for (int i = 0; i < h / 2; i++)
        {
            uint32_t* row0 = data + w * i;
            uint32_t* row1 = data + w * (h - i - 1);

            for (int j = 0; j < w; j++)
            {
                uint32_t w0 = *row0;
                uint32_t w1 = *row1;

                *row0++ = w1;
                *row1++ = w0;
            }
        }
    }

    inline uint32_t SwizzleRB(uint32_t a)
    {         // a b g r  -> a r g b
        return ((a <<  0) & 0xFF00FF00)
             | ((a << 16) & 0x00FF0000)
             | ((a >> 16) & 0x000000FF);
    }

    void SwizzleImage(int w, int h, uint32_t* data)
    {
        //
        for (int i = 0; i < w * h; i++)
            data[i] = SwizzleRB(data[i]);
    }
}

void nCL::FlipImage(cImage8&& image)
{
    ::FlipImage(image.mW, image.mH, image.mData);
}

void nCL::FlipImage(cImage8* image)
{
    ::FlipImage(image->mW, image->mH, image->mData);
}

void nCL::FlipImage(cImage32&& image)
{
    ::FlipImage(image.mW, image.mH, image.mData);
}

void nCL::FlipImage(cImage32* image)
{
    ::FlipImage(image->mW, image->mH, image->mData);
}

void nCL::FlipImage(cImageFloat* image)
{
    ::FlipImage(image->mW, image->mH, (uint32_t*) image->mData);
}

void nCL::SwizzleImage(cImage32&& image)
{
    ::SwizzleImage(image.mW, image.mH, image.mData);
}

void nCL::SwizzleImage(cImage32* image)
{
    ::SwizzleImage(image->mW, image->mH, image->mData);
}


bool nCL::InitImageSystem()
{
    stbi_convert_iphone_png_to_rgb(1);
    // stbi_set_unpremultiply_on_load
    // TODO: install memory system in stb
    // stbi_hdr_to_ldr_gamma(float gamma);
    // stbi_hdr_to_ldr_scale(float scale);

    // stbi_ldr_to_hdr_gamma(float gamma);
    // stbi_ldr_to_hdr_scale(float scale);
    return true;
}

bool nCL::ShutdownImageSystem()
{
    return true;
}


namespace
{
    // Image embedding
    // Ideas:
    // - Write to lowest bits of data, then next lowest, etc.
    // - Write in a pr sequence rather than linearly
    // - Write data xor'd with pr stream of bits

    // Straight forward to get non-repeating PR sequence for power of two, e.g.,
    // http://www.mactech.com/articles/mactech/Vol.06/06.12/SafeDissolve/index.html
    // or, Halton, or grey code?

    // https://github.com/BIC-MNI/bicgl/blob/master/G_graphics/random_order.c

    inline int Log2Int2(uint32_t a)
    {
        return 31 - LeadingZeroes32(a);
    }

    uint32_t kPermuteMasks[32] =
    {
        0x01,
        0x03,
        0x06,
        0x0C,

        0x14,
        0x30,
        0x60,
        0xB8,

        0x0110,
        0x0240,
        0x0500,
        0x0CA0,

        0x1B00,
        0x3500,
        0x6000,
        0xB400,

        0x00012000,
        0x00020400,
        0x00072000,
        0x00090000,

        0x00140000,
        0x00300000,
        0x00400000,
        0x00D80000,

        0x01200000,
        0x03880000,
        0x07200000,
        0x09000000,

        0x14000000,
        0x32800000,
        0x48000000,
        0xA3000000
    };

    inline uint32_t Next(uint32_t index, uint32_t mask)
    {
        uint32_t bit = index & 1;
        return (index >> 1) ^ (bit * mask);
    }

    inline uint32_t Next(uint32_t index, uint32_t n, uint32_t mask)
    {
        do
        {
            uint32_t bit = (index & 1) | (index == 0);
            index = (index >> 1) ^ (bit * mask);
        }
        while (index >= n);

        return index;
    }

    struct cDataEmbed
    {
        cDataEmbed(size_t size, uint8_t* data) : mSize(size), mData(data) {}

        size_t      mSize = 0;      ///< Target size
        uint8_t*    mData = 0;      ///< Target data

        uint32_t    mCursor     = 0;
        uint8_t     mAccessBit  = 1;   // bit currently being written
        uint8_t     mAccessMask = ~1;  // ~mAccessBit
        uint32_t    mDataMask   = 0;   // stop if bitmask = this

        uint32_t    mMaskSequence   = 0;
        uint32_t    mPermuteMask    = 0;

        void Begin()
        {
            int pow2 = Log2Int(mSize - 1);

            mCursor = 0;

            mDataMask       = (1 << pow2) - 1;
            mAccessBit      = 1;
            mAccessMask     = ~mAccessBit;

            mPermuteMask    = kPermuteMasks[pow2];
            mMaskSequence   = kFNVOffset32;
        }

        void EndEmbed()
        {
            // Add(endMarker);
        }

        bool EndExtract()
        {
            // Add(endMarker);
            return mAccessBit != 0;
        }

        void Embed(uint8_t data)
        {
            EMBED_DEBUG("data 0x%02x\n", data);

            uint32_t nextMaskSequence = (mMaskSequence ^ data) * kFNVPrime32;
            data ^= mMaskSequence;
            EMBED_DEBUG("mask 0x%08x, encode 0x%02x\n", mMaskSequence, data);

            for (int i = 0; i < 8; i++)
            {
                uint8_t bit = data & 1;
                data >>= 1;

                EMBED_DEBUG("write %d/%lu: %d=%d\n", mCursor, mSize, mAccessBit, bit);

                CL_INDEX(mCursor, mSize);
                mData[mCursor] = (mData[mCursor] & mAccessMask) | (bit * mAccessBit);

                if (mCursor == 1)    // last in sequence is always '1', so this will be next after that.
                {
                    EMBED_DEBUG("MOVING TO NEXT BIT\n");

                    mAccessBit <<= 1;
                    mAccessMask = ~mAccessBit;
                    mCursor = 0;
                }
                else
                    mCursor = Next(mCursor, mSize, mPermuteMask);
            }

            mMaskSequence = nextMaskSequence;
        }

        void Embed(size_t size, const uint8_t data[])
        {
            for (size_t i = 0; i < size; i++)
                Embed(data[i]);
        }

        uint8_t Extract()
        {
            uint8_t data = 0;

            for (int i = 0; i < 8; i++)
            {
                CL_INDEX(mCursor, mSize);
                uint8_t bit = (mData[mCursor] & mAccessBit) ? 1 : 0;

                EMBED_DEBUG("read %d/%lu: %d=%d\n", mCursor, mSize, mAccessBit, bit);

                data |= (bit << i);

                if (mCursor == 1)    // last in sequence is always '1', so this will be next after that.
                {
                    EMBED_DEBUG("MOVING TO NEXT BIT\n");

                    mAccessBit <<= 1;
                    mAccessMask = ~mAccessBit;
                    mCursor = 0;
                }
                else
                    mCursor = Next(mCursor, mSize, mPermuteMask);
            }

            EMBED_DEBUG("data 0x%02x\n", data);
            data ^= mMaskSequence;
            EMBED_DEBUG("mask 0x%08x, encode 0x%02x\n", mMaskSequence, data);
            mMaskSequence = (mMaskSequence ^ data) * kFNVPrime32;

            return data;
        }

        void Extract(size_t size, uint8_t data[])
        {
            for (size_t i = 0; i < size; i++)
                data[i] = Extract();
        }
    };
}

namespace nCL
{
    uint32_t* GetEmbedPermuteMasks()   // for testing
    {
        return kPermuteMasks;
    }
}

void nCL::EmbedData(cImage32* image, size_t dataSize, const void* data)
{
    cDataEmbed dataEmbed(image->mW * image->mH * sizeof(image->mData[0]), (uint8_t*) image->mData);

    dataEmbed.Begin();
    dataEmbed.Embed(sizeof(dataSize), (const uint8_t*) &dataSize);
    dataEmbed.Embed(dataSize,         (const uint8_t*) data);
    dataEmbed.EndEmbed();
}

bool nCL::ExtractData(const cImage32& image, size_t* dataSize, void** dataOut)
{
    cDataEmbed dataEmbed(image.mW * image.mH * sizeof(image.mData[0]), (uint8_t*) image.mData);

    dataEmbed.Begin();
    dataEmbed.Extract(sizeof(size_t), (uint8_t*) dataSize);

    if (*dataSize >= dataEmbed.mSize) // sanity check
        return false;

    auto alloc = Allocator(kDefaultAllocator); // XXX
    uint8_t* data = alloc->Alloc(*dataSize);

    dataEmbed.Extract(*dataSize, data);

    bool success = dataEmbed.EndExtract();

    if (success)
        *dataOut = data;
    else
        alloc->Free(data);

    return success;
}
