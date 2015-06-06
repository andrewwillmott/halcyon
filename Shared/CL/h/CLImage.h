//
//  File:       CLImage.h
//
//  Function:   CPU-side image support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_IMAGE_H
#define CL_IMAGE_H

#include <CLColour.h>
#include <CLMemory.h>
#include <CLSTL.h>  // TEMP

namespace nCL
{
    class cFileSpec;

    void ReleaseData(uint8_t** data, cIAllocator* alloc);

    struct cImage8 : public cAllocPtr<uint8_t>
    {
        int mW = 0;
        int mH = 0;

        cImage8() = default;
        cImage8(int w, int h, uint8_t* data) : cAllocPtr<uint8_t>(data), mW(w), mH(h) {}
        cImage8(int w, int h, uint8_t* data, cIAllocator* dealloc) : cAllocPtr<uint8_t>(data, dealloc), mW(w), mH(h) {}
    };

    struct cImage32 : public cAllocPtr<uint32_t>
    {
        int mW = 0;
        int mH = 0;

        cImage32() {}
        cImage32(int w, int h, uint8_t* data)  : cAllocPtr<uint32_t>((uint32_t*) data), mW(w), mH(h) {}
        cImage32(int w, int h, uint32_t* data) : cAllocPtr<uint32_t>(data),             mW(w), mH(h) {}
        cImage32(int w, int h, uint32_t* data, cIAllocator* dealloc) : cAllocPtr<uint32_t>(data, dealloc), mW(w), mH(h) {}
    };

    struct cImageFloat : public cAllocPtr<float>
    {
        int mW = 0;
        int mH = 0;
        int mN = 1;

        cImageFloat() {}
        cImageFloat(int w, int h, float* data) : cAllocPtr<float>(data), mW(w), mH(h) {}
        cImageFloat(int w, int h, float* data, cIAllocator* dealloc) : cAllocPtr<float>(data, dealloc), mW(w), mH(h) {}
    };

    struct cAllocImage8 : public cImage8, public nCL::cAllocLinkable
    {
        using cImage8::cImage8;
    };
    struct cAllocImage32 : public cImage32, public nCL::cAllocLinkable
    {
        using cImage32::cImage32;
    };
    struct cAllocImageFloat : public cImageFloat, public nCL::cAllocLinkable
    {
        using cImageFloat::cImageFloat;
    };

    bool LoadImage(const nCL::cFileSpec& spec, cImage8*  info);
    bool LoadImage(const nCL::cFileSpec& spec, cImage32* info);
    bool LoadImage(const nCL::cFileSpec& spec, cImageFloat* info);

    bool FindImageExtension(nCL::cFileSpec* spec);

    bool SaveImage(const cImage8& image, nCL::cFileSpec& spec);
    bool SaveImage(const cImage32& image, nCL::cFileSpec& spec);

    size_t CompressImage(const  cImage8& image, nCL::cIAllocator* alloc, nCL::vector<uint8_t>* data);   ///< Compress image to png format
    size_t CompressImage(const cImage32& image, nCL::cIAllocator* alloc, nCL::vector<uint8_t>* data);   ///< Compress image to png format

    void FlipImage(cImage8&& image);
    void FlipImage(cImage8* image);
    void FlipImage(cImage32&& image);
    void FlipImage(cImage32* image);
    void FlipImage(cImageFloat&& image);
    void FlipImage(cImageFloat* image);

    void SwizzleImage(cImage32&& image);
    void SwizzleImage(cImage32* image);

    void EmbedData(cImage32* image, size_t dataSize, const void* data);
    bool ExtractData(const cImage32& image, size_t* dataSize, void** data);

    bool InitImageSystem();
    bool ShutdownImageSystem();
}

#endif
