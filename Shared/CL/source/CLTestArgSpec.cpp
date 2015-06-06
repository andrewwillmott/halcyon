/*
    File:       CLTestArgSpec.cpp

    Function:   Test for cArgSpec

    Author:     Andrew Willmott

    Notes:      
*/

#include <CLArgSpec.h>


using namespace nCL;


enum tColour
{
    kRed,
    kGreen,
    kBlue,
    kBlack
};

cArgEnumInfo kColourEnum[] =
{
   "red",    kRed,
   "green",  kGreen,
   "blue",   kBlue,
   "black",  kBlack,
   0, 0
};

struct cCommand
{
    enum 
    {
        kOptionHaveDest,
        kOptionSize,
        kOptionGamma,
        kOptionLongLatArg1,
        kOptionLongLatArg2,
    };
        
    tString     mName;
    const char* mDst;
    bool        mHasSize;
    int32_t     mSize;
    double      mGamma;
    float       mLatitude;
    float       mLongitude;
    uint32_t    mJulianDay;
    
    vector<int32_t> mCounts;
    vector<const char*> mNames;
    
    tColour mColour;
    
    cArgSpec mArgSpec;
    
    cCommand() :
        mName("name"),
        mDst("dst"),
        mHasSize(false),
        mSize(100),
        mGamma(0),
        mLatitude(0),
        mLongitude(0),
        mJulianDay(0),
        mColour(kBlack),
        mArgSpec("asd")
    {
        mCounts.push_back(1);
        mCounts.push_back(2);
        mCounts.push_back(3);

        mArgSpec.ConstructSpec
        (
            "Brief description",

            "<name:string> [<dst:cstr>^]", &mName, kOptionHaveDest, &mDst,
               "Full description of command",

            ":colourType", kColourEnum,

            "-size^ %d", kOptionSize, &mSize,
                "Set image/window size",
            "-gamma^ <gamma:double>", kOptionGamma, &mGamma,
                "set gamma correction [default: 1.0]",
            "-latlong <latitude:float> <longitude:float> [<int>^ [<int>^ <int>]]",
                &mLatitude, &mLongitude, kOptionLongLatArg1, 0, kOptionLongLatArg2, 0, 0,
                "Set latitude and longitude",
            "-day <day:int>", &mJulianDay,
                "Set julian day [1..365]",
            "-counts <count1:int> ...", &mCounts,
                "Specify counts using repeated last argument",
            "-countArray <counts:int[]>", &mCounts,
                "Specify counts as explicit array",
            "-names <name1:cstring> ...", &mNames,
                "List names",
            "-colour <colourType>", &mColour,
                "Set colour",
            0, 0
        );
    }

    void PrintOptions()
    {
        printf("\nflags:\n");

        if (mArgSpec.Flag(kOptionHaveDest))
            printf("have dest\n");
        if (mArgSpec.Flag(kOptionSize))
            printf("size\n");
        if (mArgSpec.Flag(kOptionGamma))
            printf("gamma\n");
        if (mArgSpec.Flag(kOptionLongLatArg1))
            printf("longlat a1\n");
        if (mArgSpec.Flag(kOptionLongLatArg2))
            printf("longlat a2\n");
        
        printf("\nvalues:\n");

        printf
        (
            "mName %s\n"
            "mDst %s\n"
            "mHasSize %d\n"
            "mSize %d\n"
            "mGamma %g\n"
            "mLatitude %g\n"
            "mLongitude %g\n"
            "mJulianDay %d\n"
            "mColour %s\n",
            mName.c_str(),
            mDst,
            mHasSize,
            mSize,
            mGamma,
            mLatitude,
            mLongitude,
            mJulianDay,
            kColourEnum[mColour].mToken
        );
        
        for (int i = 0; i < mCounts.size(); i++)
            printf("count %d = %d\n", i, mCounts[i]);
            
        for (int i = 0; i < mNames.size(); i++)
            printf("name %d = '%s'\n", i, mNames[i]);
    }
};


int main(int argc, const char** argv)
{
    cCommand test;

    tString helpString;
    test.mArgSpec.CreateHelpString("test", &helpString, kHelpBrief);
    printf("%s\n", helpString.c_str());

    tArgError err = test.mArgSpec.Parse(argc, argv);
    if (err != kArgNoError)
    {
        printf ("err = %d\n", err);
        printf("error: %s\n", test.mArgSpec.ResultString());
    }    
    
    test.PrintOptions();
    
    return 0;
}
