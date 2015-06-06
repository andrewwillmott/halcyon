//
//  File:       CLFileSpec.h
//
//  Function:   Provides a file spec + related ops.
//              Assumed representation is path/file.extension.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1996-2014
//

#ifndef CL_FILE_SPEC_H
#define CL_FILE_SPEC_H

#include <CLString.h>

#include <stdio.h>

namespace nCL
{
    // --- cFileSpec -----------------------------------------------------------

    enum tFileCompression
    {
        kFileUncompressed,
        kFileGzipped,
        kFileBz2ed,
        kFileCompress,  // .Z
    };

    enum tFileSpecError
    {
        kFileNotFound = -1,
        kBadExtension = -2
    };
    
    struct cFileSpecExtension
    {
        tStrConst mName;
        int       mID;
    };
    
    const char kDirectorySeparator = '/';
    const char kExtensionSeparator = '.';
    

    class cFileSpec
    /// Class for manipulating filenames and paths
    {
    public:
        cFileSpec();
        cFileSpec(tStrConst fullPath);

        tStrConst   Name() const;               ///< Returns the name of the file.
        tStrConst   Extension() const;          ///< Returns the file's current extension
        tStrConst   Directory() const;          ///< Returns the pathname of the parent directory of the file

        bool        HasName() const;            ///< Returns true if there is a name set
        bool        HasExtension() const;       ///< Returns true if there is an extension set
        bool        HasDirectory() const;       ///< Returns true if there is a directory set

        tStrConst   Path() const;               ///< Returns the full pathname
        tStrConst   PathWithExtension(tStrConst ext) const; ///< Returns the full pathname with the given extension
        tStrConst   PathWithNameAndExtension(tStrConst ext) const; ///< Returns the directory plus the given name and extension

        cFileSpec&  SetName     (tStrConst name);   ///< Set the file's name
        cFileSpec&  SetExtension(tStrConst ext);    ///< Set the file's extension
        cFileSpec&  SetDirectory(tStrConst dir);    ///< Set the file's parent directory

        cFileSpec&  SetPath(tStrConst fullPath);                ///< Set all fields from the full pathname.
        cFileSpec&  SetRelativePath(tStrConst path);
        ///< Set by relative path. If the path starts with '/', the effect is the same as with SetPath, otherwise
        ///< the path is appended to the current directory path.
        cFileSpec&  SetRelativeDirectory(tStrConst path);
        cFileSpec&  SetNameAndExtension(tStrConst nameAndExt);  ///< Set the file and extension together

        void        AddDirectory(tStrConst dir);    ///< Adds a subdirectory to the end of the current directory spec.
        bool        RemoveDirectory();              ///< Removes the last sub directory from the current directory spec. Returns false if there are no more subdirectories.
        void        AddSuffix(tStrConst suffix, char sc = '_'); ///< Add 'sc''suffix' to the end of the name
        void        RemoveSuffix(char sc = '_');                ///< Remove last suffix from the end of the name
        void        AddExtension(tStrConst extension);  ///< The current extension is added to the name, and 'ext' becomes the new extension.
        bool        RemoveExtension();                  ///< Strips the current extension, and replaces it with any extension at the end of the current name. Returns false if there are no more such extensions

        // File operations
        int         FindFileExtension(cFileSpecExtension extensions[], bool allowCompressed = true);
        ///< Search for file with the given list of acceptable extensions.
        ///< Returns corresponding mID if it exists, or a tFileSpecError if not.

        bool        Exists() const;                 ///< Does the file exist?
        bool        IsReadable() const;             ///< Is it readable?
        bool        IsWritable() const;             ///< Is it writable?

        bool        MakeWritable();

        bool        DirectoryExists() const;        ///< Does the directory exist?
        bool        DirectoryIsReadable() const;    ///< Is it readable/searchable?
        bool        DirectoryIsWritable() const;    ///< Is it writable?

        uint32_t    TimeStamp() const;           ///< Return file's timestamp
        cFileSpec&  MakeUnique();
        ///< Make the filename unique. If a file with the current name already exists, it is renamed 'myname-1.ext'.
        ///< If that exists, it is renamed 'myname-2.ext', and so on until a file with the given name doesn't already exist.

        bool        EnsureDirectoryExists() const;          ///< Create parent directory if necessary
        FILE*       FOpen(tStrConst permissions) const;     ///< Open file and return filehandle

        int         DecompressSetup();          ///< Set up for decompression.
        void        DecompressCleanup();        ///< Clean up after decompression.
        void        AddCompressionExtension();  ///< Add compression extension to filename
                    
    protected:
        tFileCompression mCompression = kFileUncompressed;  ///< Compression type if any
        tString     mDirectory;  ///< full path to the parent directory
        tString     mName;       ///< current file name.
        tString     mExtension;  ///< file's extension.

        mutable tString mPath;   ///< Temp space for returning full path
    };



    // --- cDirectoryInfo ------------------------------------------------------

    struct cDirectoryInfo
    {
    public:
        struct cInfo
        {
            string   mName;
            uint32_t mTimeStamp;

            cInfo() : mName(), mTimeStamp(0) {}
        };

        string        mPath;            ///< Path to this directory
        vector<cInfo> mFiles;           ///< Files in this directory
        vector<cInfo> mDirectories;     ///< Subdirectories

        cDirectoryInfo();

        bool Read(tStrConst path);  ///< Initialise/update with given path
        bool ReadAgain();              ///< Re-fetch directory info
    };

    struct cFullDirectoryInfo : public cDirectoryInfo
    /// As per cDirectoryInfo, but gathers info from all subdirectories
    {
        bool Read(tStrConst path);
    };


    ////////////////////////////////////////////////////////////////////////////
    // Functions
   
    void GetTempPath(tString* result);
    ///< Returns path to directory for temp files



    ////////////////////////////////////////////////////////////////////////////
    // Inlines

    inline tStrConst cFileSpec::Name() const
    {
        return mName.c_str();
    }
    inline tStrConst cFileSpec::Extension() const
    {
        return mExtension.c_str();
    }
    inline tStrConst cFileSpec::Directory() const
    {
        return mDirectory.c_str();
    }

    inline bool cFileSpec::HasName() const
    {
        return !mName.empty();
    }

    inline bool cFileSpec::HasExtension() const
    {
        return !mExtension.empty();
    }

    inline bool cFileSpec::HasDirectory() const
    {
        return !mDirectory.empty();
    }

    inline cFileSpec& cFileSpec::SetName(tStrConst file)
    {
        if (file)
            mName = file;
        else
            mName.clear();

        CL_ASSERT(mName.find_last_of(kDirectorySeparator) == tString::npos);

        return SELF;
    }

    inline cFileSpec& cFileSpec::SetExtension(tStrConst ext)
    {
        if (ext)
            mExtension = ext;
        else
            mExtension.clear();

        CL_ASSERT(mExtension.find_last_of(kExtensionSeparator) == tString::npos);
        CL_ASSERT(mExtension.find_last_of(kDirectorySeparator) == tString::npos);

        return SELF;
    }

    inline cFileSpec& cFileSpec::SetDirectory(tStrConst dir)
    {
        if (dir)
        {
            mDirectory = dir;

            while (!mDirectory.empty() && mDirectory.back() == kDirectorySeparator)
                mDirectory.pop_back();
        }
        else
            mDirectory.clear();

        return SELF;
    }

    inline cFileSpec& cFileSpec::SetNameAndExtension(tStrConst nameAndExt)
    {
        SetName(nameAndExt);
        RemoveExtension();
        return *this;
    }

    inline FILE* cFileSpec::FOpen(tStrConst permissions) const
    {
        return fopen(Path(), permissions);
    }
}

#endif
