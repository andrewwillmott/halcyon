//
//  File:       CLTool
//
//  Function:   Command-line tool, random test code
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#include <CLArgSpec.h>
#include <CLFileSpec.h>
#include <CLJSON.h>
#include <CLSystem.h>

#include <CLBits.h>
#include <CLData.h>
#include <CLImage.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLTag.h>
#include <CLValue.h>

using namespace nCL;

namespace nCL
{
    void TestIO();
}

// Various bits of test code

namespace
{
    inline uint32_t BitReverse32Fast(uint32_t x)
    {
        uint32_t a = ByteSwap32(x);
        uint32_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);

        return b;
    }

    inline uint32_t BitReverse16Fast(uint32_t x)
    {
        uint32_t a = ByteSwap16(x);
        uint32_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);

        return b;
    }

    inline uint32_t TrailingOnesX(uint32_t x)
    {
        uint32_t a = (x + 1) ^ x;
        a &= x;
        printf("x = 0x%08x, a = 0x%08x\n", x, a);
        return BitCount32(a);
    }

    void BitTest()
    {
        printf("reverse32_1 = 0x%08x\n", BitReverse32    (0x12345678));
        printf("reverse32_1 = 0x%08x\n", BitReverse32Fast(0x12345678));
        printf("reverse16_1 = 0x%04x\n", BitReverse16    (0x1234));
        printf("reverse16_1 = 0x%04x\n", BitReverse16Fast(0x1234));
        printf("reverse08_1 = 0x%02x\n", BitReverse8    (0x12));

        printf("clz 0x%08x = %d\n", 12345, LeadingZeroes32(12345));
        printf("clz 0x%08x = %d\n", 123456, LeadingZeroes32(123456));
        printf("clz 0x%08x = %d\n", 1234567, LeadingZeroes32(1234567));

        printf("clz 0x%08x = %d\n", 12345, TrailingOnesX(12345));
        printf("clz 0x%08x = %d\n", 123456, TrailingOnesX(123456));
        printf("clz 0x%08x = %d\n", 1234567, TrailingOnesX(1234567));

        printf("bc 0x%08x = %d\n", 0x12345678, BitCount32    (0x12345678));
        printf("bc 0x%08x = %d\n", 0, BitCount32    (0));
        printf("bc 0x%08x = %d\n", 0xFFFFFFFF, BitCount32    (0xFFFFFFFF));
    }
}

namespace
{
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
}

namespace nCL
{
    const uint32_t* GetEmbedPermuteMasks();
}

int main(int argc, const char** argv)
{
    InitLogSystem();

    cArgSpec argSpec;
    const char* inputFileStr = 0;
    const char* embedMessage = 0;
    int permuteTest = -1;

    enum tOptions
    {
        kFlagConvertConfig,
        kFlagDumpConfig,
        kFlagImage,
        kFlagEmbed,
        kFlagExtract,
        kFlagAscii,
        kFlagTestIO,
        kMaxFlags
    };

    argSpec.ConstructSpec
    (
         "CL tool",

        "",
            "Provides various CL-based utilities",

        "-config^ <inputFile:cstr>", kFlagConvertConfig, &inputFileStr,
            "Read the given top-level json file and process it",
         "-dump^", kFlagDumpConfig,
            "Dump input",
        "-image^ <inputFile:cstr>", kFlagImage, &inputFileStr,
            "Read the given top-level json file and process it",
        "  -embed^ <message:cstr>", kFlagEmbed, &embedMessage,
            "  Embed given message in image and save it",
        "-extract^", kFlagExtract,
            "Extract message from image and display it",
        "-ascii^", kFlagAscii,
            "Show ascii greyscale of image",
        "-permute <int>", &permuteTest,
            "Test permutation",
        "-testIO^", kFlagTestIO,
            "Test IO streams",
         0
    );

    if (argc == 1)
    {
        printf("%s\n", argSpec.HelpString(argv[0]));
        return 0;
    }

    if (argSpec.Parse(argc, argv) != kArgNoError)
    {
        printf("%s\n", argSpec.ResultString());
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////


    if (permuteTest >= 0)
    {
        const uint32_t* kPermuteMasks = GetEmbedPermuteMasks();

        int pow2 = Log2Int(permuteTest - 1);
        CL_INDEX(pow2, sizeof(kPermuteMasks));

        uint32_t mask = kPermuteMasks[pow2];

        printf("pow2  = %d\n", pow2);
        printf("ceil2 = 0x%08x\n", (2 << pow2) - 1);
        printf("mask  = 0x%08x\n", mask);

        uint32_t s = 0;

        for (int i = 0; i < permuteTest + 1; i++)
        {
            printf("%d\n", s);
            s = Next(s, permuteTest, mask);
        }

        return 0;
    }

    InitTool();

    cFileSpec inputFile;

    if (inputFileStr)
    {
        inputFile.SetPath(inputFileStr);

        if (!inputFile.Exists())
        {
            fprintf(stderr, "%s does not exist\n", inputFile.Path());
            return -1;
        }

    }

    if (argSpec.Flag(kFlagImage))
    {
        cImage32 image;

        if (LoadImage(inputFile, &image))
        {
            fprintf(stdout, "read %d x %d image\n", image.mW, image.mH);

            if (embedMessage)
            {
                // const char kMessage[] = "Hello out there, how is life in the land of the gigantic wombat";
                // EmbedData(&image, sizeof(kMessage), kMessage);
                EmbedData(&image, strlen(embedMessage) + 1, embedMessage);

                inputFile.AddSuffix("embed");
                if (SaveImage(image, inputFile))
                    printf("Saved embedded data in image %s\n", inputFile.Path());
                else
                    printf("Failed to save image to %s\n", inputFile.Path());
                inputFile.RemoveSuffix();
            }

            if (argSpec.Flag(kFlagAscii))
            {
                const char* const kGrey16 = " .'`^:;-|=+x*%#@";
                //                           0123456789ABCDEF

                for (int y = 0; y < image.mH; y++)
                {
                    const uint32_t* row32 = image.mData + y * image.mW;

                    for (int x = 0; x < image.mW; x++)
                    {
                        cRGBA32 p;
                        p.mAsUInt32 = row32[x];

                        fputc(kGrey16[p.mChannel[1] >> 4], stdout);
                        fputc(' ', stdout);
                        fputc(' ', stdout);
                    }

                    fprintf(stdout, "\n");
                }
            }

            if (embedMessage || argSpec.Flag(kFlagExtract))
            {
                size_t dataSize;
                void* data;

                if (ExtractData(image, &dataSize, &data))
                {
                    printf("%lu bytes of data extracted:\n%s\n", dataSize, data);
                }
                else
                    fprintf(stderr, "failed to extract data\n");
            }
        }
        else
            fprintf(stderr, "failed to read %s\n", inputFile.Path());
    }

    if (argSpec.Flag(kFlagConvertConfig))
    {
        cFileSpec inputFile(inputFileStr);
        cValue value;

        string errorMessages;
        int errorLine = -1;

        if (ReadFromJSONFile(inputFile, value.AsObject(), &errorMessages, &errorLine))
        {
            printf("read %s successfully\n", inputFile.Path());

            cLink<cIConfigSource> configSource = CreateDefaultConfigSource(Allocator(kDefaultAllocator));
            ApplyImports(value.AsObject(), configSource);

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
    }

    if (argSpec.Flag(kFlagTestIO))
        TestIO();

    ShutdownTool();

    return 0;
}
