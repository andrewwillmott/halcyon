//
//  File:       CLFileSpec.cpp
//
//  Function:   Implements CLFileSpec.h
//
//  Author:     Andrew Willmott
//
//  Copyright:  1996-2014
//

#include <CLFileSpec.h>

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifndef CL_NO_ACCESS
    #include <unistd.h>
#endif

using namespace nCL;

nCL::cFileSpec::cFileSpec()
{
}

nCL::cFileSpec::cFileSpec(tStrConst fullPath)
{
    SetPath(fullPath);
}

void nCL::cFileSpec::AddDirectory(tStrConst dir)
{
    mDirectory += kDirectorySeparator;
    mDirectory += dir;
}

bool nCL::cFileSpec::RemoveDirectory()
{
    size_t slashPos = mDirectory.find_last_of(kDirectorySeparator);
    
    if (slashPos != tString::npos)
    {
        mDirectory.resize(slashPos);
        return true;
    }

    return false;
}

void nCL::cFileSpec::AddSuffix(tStrConst suffix, char suffixChar)
{
    mName += suffixChar;
    mName += suffix;
}

void nCL::cFileSpec::RemoveSuffix(char suffixChar)
{
    size_t a = mName.find_last_of(suffixChar);
    mName.erase(a);
}

void nCL::cFileSpec::AddExtension(tStrConst ext)
{
    // Push previous extension onto the name.
    if (!mExtension.empty())
    {
        mName += kExtensionSeparator;
        mName += mExtension;
    }

    if (ext)
        mExtension = ext;
    else
        mExtension.clear();
}

bool nCL::cFileSpec::RemoveExtension()
{
    // Pop off last extension on the filename if it exists.
    size_t dotPos = mName.find_last_of(kExtensionSeparator);

    if (dotPos != tString::npos)
    {
        mExtension = mName.substr(dotPos + 1);
        mName.erase(dotPos);
        return true;
    }

    mExtension.clear();
    return false;
}


tStrConst nCL::cFileSpec::Path() const
{
    mPath.clear();
    mPath.reserve(mDirectory.size() + mName.size() + mExtension.size() + 2);

    if (!mDirectory.empty())
    {
        mPath += mDirectory;
        mPath += kDirectorySeparator;
    }
    mPath += mName;

    if (!mExtension.empty())
    {
        mPath += kExtensionSeparator;
        mPath += mExtension;
    }

    return mPath.c_str();
}

tStrConst nCL::cFileSpec::PathWithExtension(tStrConst extension) const
{
    mPath.clear();
    mPath.reserve(mDirectory.size() + mName.size() + (extension ? strlen(extension) : 0) + 2);

    if (!mDirectory.empty())
    {
        mPath += mDirectory;
        mPath += kDirectorySeparator;
    }
    mPath += mName;

    if (extension)
    {
        mPath += kExtensionSeparator;
        mPath += extension;
    }

    return mPath.c_str();
}

tStrConst nCL::cFileSpec::PathWithNameAndExtension(tStrConst nameAndExt) const
{
    CL_ASSERT(nameAndExt);

    mPath.clear();
    mPath.reserve(mDirectory.size() + (nameAndExt ? strlen(nameAndExt) : 0) + 2);

    if (!mDirectory.empty())
    {
        mPath += mDirectory;
        mPath += kDirectorySeparator;
    }

    mPath += nameAndExt;

    return mPath.c_str();
}

nCL::cFileSpec& nCL::cFileSpec::SetPath(tStrConst filePath)
{
    if (!filePath)
    {
        mDirectory.clear();
        mName.clear();
        mExtension.clear();
        return SELF;
    }

    tString thePath;

    SubstituteEnvVars(filePath, &thePath);

    size_t slashPos = thePath.find_last_of(kDirectorySeparator);
    
    if (slashPos != tString::npos)
    {
        mDirectory.assign(thePath, 0, slashPos);
        mName.assign(thePath, slashPos + 1, tString::npos);
    }
    else
    {
        mDirectory = kExtensionSeparator;
        mName = thePath;
    }

    RemoveExtension();

    return SELF;
}


nCL::cFileSpec& nCL::cFileSpec::SetRelativePath(tStrConst filePath)
{
    int         slashPos;
    tString     thePath;
    SubstituteEnvVars(filePath, &thePath);
    
    if (thePath[0] == kDirectorySeparator) // absolute path
        return SetPath(filePath);

    slashPos = thePath.find_last_of(kDirectorySeparator);
    
    if (slashPos != string::npos)
    {
        mDirectory += kDirectorySeparator;
        mDirectory += thePath.substr(0, slashPos);
        mName = thePath.substr(slashPos + 1);
    }
    else
        mName = thePath;

    RemoveExtension();

    return SELF;
}

nCL::cFileSpec& nCL::cFileSpec::SetRelativeDirectory(tStrConst dirPathIn)
{
    tString dirPath;
    SubstituteEnvVars(dirPathIn, &dirPath);
    
    if (dirPath[0] == kDirectorySeparator) // absolute path
        return SetDirectory(dirPath.c_str());

    mDirectory += kDirectorySeparator;
    mDirectory += dirPath;

    return SELF;
}

bool nCL::cFileSpec::Exists() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Path(), F_OK) == 0;
#endif
}

bool nCL::cFileSpec::IsReadable() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Path(), R_OK) == 0;
#endif
}

bool nCL::cFileSpec::IsWritable() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Path(), W_OK) == 0;
#endif
}

bool nCL::cFileSpec::MakeWritable()
{
    return chmod(Path(),
        S_IRUSR | S_IRGRP | S_IROTH
      | S_IWUSR | S_IWGRP | S_IWOTH
        ) == 0;
}


uint32_t nCL::cFileSpec::TimeStamp() const
{
    struct stat statInfo;
    
    stat(Path(), &statInfo);

    return statInfo.st_mtime;
}

bool nCL::cFileSpec::DirectoryExists() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Directory(), F_OK | X_OK) == 0;
#endif
}

bool nCL::cFileSpec::DirectoryIsReadable() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Directory(), R_OK | X_OK) == 0;
#endif
}

bool nCL::cFileSpec::DirectoryIsWritable() const
{
#ifdef CL_NO_ACCESS
    return true;
#else
    return access(Directory(), W_OK) == 0;
#endif
}


namespace
{
    struct cCompressExtension
    {
        tStrConst         mName;
        tFileCompression  mType;
    };

    cCompressExtension kCompressExtensions[] =
    {
        { "gz",       kFileGzipped  },
        { "bz2",      kFileBz2ed    },
        { "Z",        kFileCompress },
        { 0 }
    };
}

int nCL::cFileSpec::FindFileExtension(cFileSpecExtension extensions[], bool allowCompressed)
/*! Checks for a file with one of the given set of extensions. The
    filename should have been already set to:

    - The base name, in which case is will check for existing files with one of the extensions.
    - The name of an actual file, in which case it will check that it has the right extension.

    If allowCompressed is true, it will also recognize (and remove) .gz,
    .bz2 and .Z extensions, setting mCompression appropriately.

    Returns:
        kFileNotFound       couldn't find a valid file.
        kBadExtension       the extension already set isn't allowable.

*/
{
    mCompression = kFileUncompressed;
    
    bool extWasSet = !mExtension.empty();

    if (extWasSet)
    // an extension is already set -- at most we might have to 
    // tack on a compression suffix.
    {
        // remove any existing compression suffix
        if (allowCompressed)
            for (int j = 0; kCompressExtensions[j].mName != 0; j++)
                if (eqi(mExtension, kCompressExtensions[j].mName))
                {
                    if (Exists())
                        mCompression = kCompressExtensions[j].mType;
                    RemoveExtension();
                    break;
                }

        // check that the extension is valid
        int i;

        for (i = 0; extensions[i].mName != 0; i++)
            if (eqi(mExtension, extensions[i].mName))
                break;

        if (extensions[i].mName == 0)
            return kBadExtension;

        if ((mCompression != kFileUncompressed) || Exists())
            return extensions[i].mID;
        else
        {
            if (allowCompressed)
            // try it with a compression extension
            {
                AddExtension(0);

                for (int j = 0; kCompressExtensions[j].mName != 0; j++)
                {
                    SetExtension(kCompressExtensions[j].mName);

                    if (Exists())
                    {
                        RemoveExtension();
                        mCompression = kCompressExtensions[j].mType;

                        return extensions[i].mID;
                    }
                }
                
                RemoveExtension();
            }

            return kFileNotFound;
        }
    }

    // try all possible extensions and compressions
    for (int i = 0; extensions[i].mName != 0; i++)
    {
        SetExtension(extensions[i].mName);

        if (Exists())
            return extensions[i].mID;

        // try it with a compression mExtension
        if (allowCompressed)
        {
            AddExtension(0);

            for (int j = 0; kCompressExtensions[j].mName != 0; j++)
            {
                SetExtension(kCompressExtensions[j].mName);

                if (Exists())
                {
                    RemoveExtension();
                    mCompression = kCompressExtensions[j].mType;

                    return extensions[i].mID;
                }
            }
            
            RemoveExtension();
        }
    }

    mExtension.clear();

    return kFileNotFound;
}

nCL::cFileSpec &nCL::cFileSpec::MakeUnique()
{
    if (Exists())
    {
        tString  origName = mName;
        int     i = 0;

        do
        {
            i++;
            Sprintf(&mName, "%s-%d", origName.c_str(), i);
        }
        while (Exists());
    }

    return SELF;
}

void nCL::cFileSpec::AddCompressionExtension()
{
    switch (mCompression)
    {
    case kFileUncompressed:
        break;
    case kFileGzipped:
        AddExtension("gz");
        break;
    case kFileBz2ed:
        AddExtension("bz2");
        break;
    case kFileCompress:
        AddExtension("Z");
        break;
    }
}

int nCL::cFileSpec::DecompressSetup()
{
    if (mCompression != kFileUncompressed)
    {
        tString      origFile;
        tStrConst    cmd;

        AddCompressionExtension();
        origFile = Path();
        RemoveExtension();

        tString tempPath;
        GetTempPath(&tempPath);
        SetDirectory(tempPath.c_str());

        switch (mCompression)
        {
        case kFileUncompressed:
            cmd = "cat";
            break;
        case kFileGzipped:
            cmd = "gunzip -c";
            break;
        case kFileBz2ed:
            cmd = "bunzip2 -c";
            break;
        case kFileCompress:
            cmd = "compress -c";
            break;
        }

        tString fullCommand;
        Sprintf(&fullCommand, "%s \"%s\" > \"%s\"", cmd, origFile.c_str(), Path());

        if (system(fullCommand.c_str()) != 0)
            return -1;
    }

    return 0;
}

bool nCL::cFileSpec::EnsureDirectoryExists() const
{
    if (!DirectoryExists())
    {
        cFileSpec parentDir;

        parentDir.SetDirectory(mDirectory.c_str());
        if (parentDir.RemoveDirectory())
            parentDir.EnsureDirectoryExists();

        return mkdir(mDirectory.c_str(), 0775) == 0;
    }

    return true;
}

void nCL::cFileSpec::DecompressCleanup()
{
    if (mCompression != kFileUncompressed)
        unlink(Path());
}

void nCL::GetTempPath(tString* tempPath)
{
    if (!SubstituteEnvVars("$TMPDIR", tempPath))
        *tempPath = "/tmp";
}



// --- cDirectoryInfo ----------------------------------------------------------

#include <dirent.h>

nCL::cDirectoryInfo::cDirectoryInfo() :
    mPath(),
    mFiles(),
    mDirectories()
{
}

bool nCL::cDirectoryInfo::Read(tStrConst path)
{
    mPath = path;

    while (mPath.back() == kDirectorySeparator)
        mPath.pop_back();

    return ReadAgain();
}

bool nCL::cDirectoryInfo::ReadAgain()
{
    DIR* dir = opendir(mPath.c_str());
    if (dir == 0)
        return false;

    mFiles.clear();
    mDirectories.clear();

    dirent entryStore;
    dirent* entry;

    while (readdir_r(dir, &entryStore, &entry) == 0 && entry != 0)
    {
        if (entry->d_type == DT_DIR)
        {
            if (entry->d_namlen > 0 && entry->d_name[0] == '.')
            {
                if (entry->d_namlen == 1 || (entry->d_name[1] == '.' && entry->d_namlen == 2))
                    continue;
            }

            mDirectories.push_back();
            mDirectories.back().mName.assign(entry->d_name, entry->d_namlen);
        }
        else
        {
            mFiles.push_back();
            mFiles.back().mName.assign(entry->d_name, entry->d_namlen);
        }
    }

    closedir(dir);

    return true;
}

bool nCL::cFullDirectoryInfo::Read(tStrConst path)
{
    if (!cDirectoryInfo::Read(path))
        return false;

    tString subDir(mPath);
    subDir += kDirectorySeparator;

    for (int i = 0; i < mDirectories.size(); i++)
    {
        const char* subPath = mDirectories[i].mName.c_str();

        subDir.resize(mPath.size() + 1);
        subDir.append(subPath);

        DIR* dir = opendir(subDir.c_str());
        if (dir == 0)
            continue;

        dirent entryStore;
        dirent* entry;

        while (readdir_r(dir, &entryStore, &entry) == 0 && entry != 0)
        {
            if (entry->d_type == DT_DIR)
            {
                if (entry->d_namlen > 0 && entry->d_name[0] == '.')
                {
                    if (entry->d_namlen == 1 || (entry->d_name[1] == '.' && entry->d_namlen == 2))
                        continue;
                }

                mDirectories.push_back();
                mDirectories.back().mName = subPath;
                mDirectories.back().mName += kDirectorySeparator;
                mDirectories.back().mName.append(entry->d_name, entry->d_namlen);
            }
            else
            {
                mFiles.push_back();
                mFiles.back().mName = subPath;
                mFiles.back().mName += kDirectorySeparator;
                mFiles.back().mName.append(entry->d_name, entry->d_namlen);
            }
        }

        closedir(dir);
    }

    return true;
}
