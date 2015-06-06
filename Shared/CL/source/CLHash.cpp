//
//  File:       CLHash.cpp
//
//  Function:   Implementation of various useful hash functions
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLHash.h>

//#include <Kernel/libkern/crypto/md5.h>
//#include <openssl/md5.h>

namespace
{
    // See e.g. http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c

    const uint32_t kCRC32Table[256] =
    {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
}

uint32_t nCL::CRC32(const uint8_t* data, size_t size, uint32_t crc)
{
    crc = ~crc;

    while (size >= 8)
    {
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);

        size -= 8;
    }

    while (size--)
        crc = kCRC32Table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);

    return ~crc;
}

namespace
{
    // Interesting variant that processes data a nibble at a time, and hence needs
    // a much smaller 16-entry table. May be useful in some cache-constrained situations.

    const uint32_t kCRC32NibbleTable[16] =
    {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };
}

uint32_t nCL::CRC32Nibble(const uint8_t* data, size_t size, uint32_t crc)
{
    crc = ~crc;

    while (size--)
    {
        uint8_t b = *data++;

        crc = (crc >> 4) ^ kCRC32NibbleTable[(crc & 0xF) ^ (b & 0x0F)];
        crc = (crc >> 4) ^ kCRC32NibbleTable[(crc & 0xF) ^ (b  >>  4)];
    }

    return ~crc;
}



// If we need a local implementation on some platforms, see http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
// (OpenSSL version.)

void nCL::MD5(const uint8_t* data, size_t size, uint8_t hash[16])
{
#ifdef _CRYPTO_MD5_H_
    MD5_CTX context;

    MD5Init  (&context);
    MD5Update(&context, data, size);
    MD5Final (hash, &context);
#elif defined(HEADER_MD5_H)
    MD5_CTX context;

    MD5_Init  (&context);
    MD5_Update(&context, data, size);
    MD5_Final (hash, &context);
#else
    CL_ERROR("Unimplemented");
#endif
}



// See https://131002.net/siphash/

#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define U32TO8_LE(p, v)         \
  (p)[0] = (uint8_t)((v)      ); (p)[1] = (uint8_t)((v) >>  8); \
  (p)[2] = (uint8_t)((v) >> 16); (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)         \
  U32TO8_LE((p),     (uint32_t)((v)      ));   \
  U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p) \
  (((uint64_t)((p)[0])      ) | \
   ((uint64_t)((p)[1]) <<  8) | \
   ((uint64_t)((p)[2]) << 16) | \
   ((uint64_t)((p)[3]) << 24) | \
   ((uint64_t)((p)[4]) << 32) | \
   ((uint64_t)((p)[5]) << 40) | \
   ((uint64_t)((p)[6]) << 48) | \
   ((uint64_t)((p)[7]) << 56))

#define SIPROUND    \
  do {              \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;     \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;     \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

/* SipHash-2-4 */
void nCL::SipHash(const uint8_t* in, size_t inlen, const uint8_t* k, uint8_t* out)
{
    /* "somepseudorandomlygeneratedbytes" */
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;

    uint64_t b;
    uint64_t k0 = U8TO64_LE( k );
    uint64_t k1 = U8TO64_LE( k + 8 );
    uint64_t m;

    int i;
    const uint8_t *end = in + inlen - ( inlen % sizeof( uint64_t ) );
    const int left = inlen & 7;
    b = ( ( uint64_t )inlen ) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for ( ; in != end; in += 8 )
    {
        m = U8TO64_LE( in );
        v3 ^= m;

        for( i=0; i<cROUNDS; ++i ) SIPROUND;

        v0 ^= m;
    }

    switch( left )
    {
    case 7: b |= ( ( uint64_t )in[ 6] )  << 48;
    case 6: b |= ( ( uint64_t )in[ 5] )  << 40;
    case 5: b |= ( ( uint64_t )in[ 4] )  << 32;
    case 4: b |= ( ( uint64_t )in[ 3] )  << 24;
    case 3: b |= ( ( uint64_t )in[ 2] )  << 16;
    case 2: b |= ( ( uint64_t )in[ 1] )  <<  8;
    case 1: b |= ( ( uint64_t )in[ 0] ); break;
    case 0: break;
    }

    v3 ^= b;

    for( i=0; i<cROUNDS; ++i ) SIPROUND;

    v0 ^= b;
    v2 ^= 0xff;

    for( i=0; i<dROUNDS; ++i ) SIPROUND;
    
    b = v0 ^ v1 ^ v2  ^ v3;
    U64TO8_LE( out, b );
}
