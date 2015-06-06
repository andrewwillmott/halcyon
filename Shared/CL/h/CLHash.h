//
//  File:       CLHash.h
//
//  Function:   Various useful hash functions
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_HASH_H
#define CL_HASH_H

#include <CLDefs.h>
#include <ctype.h>  // for tolower()

namespace nCL
{
    const uint32_t kFNVOffset32 = UINT32_C(0x811C9DC5);
    const uint64_t kFNVOffset64 = UINT64_C(0xCBF29CE484222325);

    uint32_t HashU32 (const uint8_t* data, const uint8_t* dataEnd, uint32_t hashValue = kFNVOffset32);
    ///< Hash byte-based data to a 32-bit value. 'hashValue' can be used to chain hashes: HashU32(data1, HashU32(data2)), etc.
    ///< This is currently the FNV1a hash.
    uint32_t IHashU32(const uint8_t* data, const uint8_t* dataEnd, uint32_t hashValue = kFNVOffset32);    ///< Case-insensitive version of HashU32

    uint32_t StrHashU32 (const char* s, uint32_t hashValue = kFNVOffset32); ///< Basic string hash. 'hashValue' can be used to chain hashes.
    uint32_t StrIHashU32(const char* s, uint32_t hashValue = kFNVOffset32); ///< Case insensitive version of StrHashU32.

    uint64_t HashU64    (const uint8_t* data, const uint8_t* dataEnd, uint64_t hashValue = kFNVOffset64);
    uint64_t IHashU64   (const uint8_t* data, const uint8_t* dataEnd, uint64_t hashValue = kFNVOffset64);
    uint64_t StrHashU64 (const char* s, uint64_t hashValue = kFNVOffset64);
    uint64_t StrIHashU64(const char* s, uint64_t hashValue = kFNVOffset64);


    // CRC
    uint32_t CRC32      (const uint8_t* data, size_t size, uint32_t crc = 0);   ///< Standard CRC, matches gzip/crc32. 'crc' can be used for chaining.
    uint32_t CRC32Nibble(const uint8_t* data, size_t size, uint32_t crc = 0);   ///< Variant that puts less pressure on cache -- use in mixed data situations.

    // MD5 (See http://www.ietf.org/rfc/rfc1321.txt)
    void MD5(const uint8_t* data, size_t size, uint8_t hash[16]);

    // SipHash
    void SipHash(const uint8_t* src, size_t inlen, const uint8_t k[16], uint8_t dst[8]);

    uint32_t MurmurHash3(uint32_t);


    // --- Inlines -------------------------------------------------------------

    // See http://www.isthe.com/chongo/tech/comp/fnv/

    const uint32_t kFNVPrime32 = 0x01000193;    // 0x0100 0193

    inline uint32_t HashU32(const uint8_t* data, const uint8_t* dataEnd, uint32_t hashValue)
    {
        for ( ; data < dataEnd; data++)
            hashValue = (hashValue ^ (*data)) * kFNVPrime32;

        return hashValue;
    }

    inline uint32_t IHashU32(const char* data, const char* dataEnd, uint32_t hashValue)
    {
        for ( ; data < dataEnd; data++)
            hashValue = (hashValue ^ tolower(*data)) * kFNVPrime32;

        return hashValue;
    }

    inline uint32_t StrHashU32(const char* s, uint32_t hashValue)
    {
        const uint8_t* data = (const uint8_t*) s;
        uint32_t c;

        while ((c = *data++) != 0)
            hashValue = (hashValue ^ c) * kFNVPrime32;

        return hashValue;
    }

    inline uint32_t StrIHashU32(const char* s, uint32_t hashValue)
    {
        const uint8_t* data = (const uint8_t*) s;
        uint32_t c;

        while ((c = *data++) != 0)
            hashValue = (hashValue ^ tolower(c)) * kFNVPrime32;

        return hashValue;
    }

    const uint64_t kFNVPrime64 = 0x00000100000001B3;    // 0x0000 0100 0000 01B3

    inline uint64_t HashU64(const uint8_t* data, const uint8_t* dataEnd, uint64_t hashValue)
    {
        for ( ; data < dataEnd; data++)
            hashValue = (hashValue ^ (*data)) * kFNVPrime64;

        return hashValue;
    }

    inline uint64_t IHashU64(const char* data, const char* dataEnd, uint64_t hashValue)
    {
        for ( ; data < dataEnd; data++)
            hashValue = (hashValue ^ tolower(*data)) * kFNVPrime64;

        return hashValue;
    }

    inline uint64_t StrHashU64(const char* s, uint64_t hashValue)
    {
        const uint8_t* data = (const uint8_t*) s;
        uint64_t c;

        while ((c = *data++) != 0)
            hashValue = (hashValue ^ c) * kFNVPrime64;

        return hashValue;
    }

    inline uint64_t StrIHashU64(const char* s, uint64_t hashValue)
    {
        const uint8_t* data = (const uint8_t*) s;
        uint64_t c;

        while ((c = *data++) != 0)
            hashValue = (hashValue ^ tolower(c)) * kFNVPrime64;

        return hashValue;
    }

    inline uint32_t MurmurHash3(uint32_t h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;

        return h;
    }

    inline uint64_t MurmurHash3(uint64_t h)
    {
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccd;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53;
        h ^= h >> 33;

        return h;
    }
}

#endif
