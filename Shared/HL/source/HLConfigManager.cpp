//
//  File:       HLConfigManager.cpp
//
//  Function:   Manages cObjectValue-based configuration for the app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLConfigManager.h>

#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLJSON.h>
#include <CLLog.h>
#include <CLUtilities.h>

#include <sys/errno.h>  // TODO

// #define LOCAL_DEBUG

using namespace nHL;
using namespace nCL;

namespace
{
}

bool cConfigManager::Init()
{
    cIAllocator* alloc = AllocatorFromObject(this);

    cFileSpec configPath;
    SetDirectory(&configPath, kDirectoryData);
    configPath.SetExtension("json");

    mDocumentsWatcher.Init();

    configPath.SetName("log");
    if (configPath.IsReadable())
        mLog = ConfigFileForPath(configPath);

    if (mLog)    // If this is supplied, configure log system immediately, before we read everything else.
        ConfigLogSystem(mLog);

    configPath.SetName("config");
    mConfig = ConfigFileForPath(configPath);

    if (!mLog)    // Fallback if this isn't explicitly supplied
        ConfigLogSystem(mConfig->Member("log").AsObject());

    mAppInfo = new(alloc) cObjectValue;

    SetDirectory(&configPath, kDirectoryDocuments);
    configPath.SetName("preferences");

    const bool watchConfig = false;
    mPreferences = ConfigFileForPath(configPath, watchConfig);

    if (!mPreferences->HasMember("preferences"))
    {
        // TODO: read defaultPreferences.json from Resources here.
        mPreferences->SetMember("preferences", cValue(true));

        uint32_t uid = CLUID();
        mPreferences->InsertMember("created") = uid;
        mPreferences->InsertMember("updated") = uid;

        SavePreferences();
    }

    mPreferencesModCount = mPreferences->ModCount();

    return true;
}

bool cConfigManager::Shutdown()
{
    if (mPreferencesModCount != mPreferences->ModCount())
        SavePreferences();

    mDocumentsWatcher.Shutdown();
    mConfig = 0;

    return true;
}

void cConfigManager::Update()
{
    mConfigModified = false;

    vector<int> changedRefs;
    vector<int> addedRefs;
    vector<int> removedRefs;

    if (mDocumentsWatcher.FilesChanged(&changedRefs, &addedRefs, &removedRefs))
    {
        bool hadErrors = !mErrorFile.empty();

        CL_LOG("Config", "Docs change...\n");

        for (int i = 0, n = addedRefs.size(); i < n; i++)
            CL_LOG("Config", "  Added: %s\n", mDocumentsWatcher.PathForRef(addedRefs[i]));

        for (int i = 0, n = removedRefs.size(); i < n; i++)
            CL_LOG("Config", "  Removed: %s\n", mDocumentsWatcher.PathForRef(removedRefs[i]));

        for (int i = 0, n = changedRefs.size(); i < n; i++)
        {
            CL_LOG("Config", "  Re-reading: %s\n", mDocumentsWatcher.PathForRef(changedRefs[i]));

            tStringID pathID = mDocumentsWatcher.PathIDForRef(changedRefs[i]);
            cFileSpec spec(StringFromStringID(pathID));

            // Re-read file
            if (ReloadConfigFileForPath(spec))
            {
                mConfigModified = true;

                if (hadErrors)
                {
                    CL_LOG_E("Config", "%s: Okay again\n", spec.Path());

                    mErrorFile.clear();
                    mErrorLine = -1;

                    hadErrors = false;
                }
            }
            else
                CL_LOG_E("Config", "%s: Couldn't read\n", spec.Path());
        }

        if (mLog && mLogModCount != mLog->ModCount())
        {
            mLogModCount = mLog->ModCount();
            ConfigLogSystem(mLog);
        }
    }

    if (mPreferencesModCount != mPreferences->ModCount())
    {
        SavePreferences();

        mPreferencesModCount = mPreferences->ModCount();    // this must come after, as SavePreferences() updates the prefs with a timestamp
    }
}

bool cConfigManager::ConfigModified()
{
    return mConfigModified;
}

void cConfigManager::SavePreferences()
{
    const char* sourcePath = mPreferences->Member(kSourcePathTag).AsString();

    if (!sourcePath)
        return;

    mPreferences->SetMember("modified", cValue(CLUID()));

    nCL::string output;
    cJSONWriter().Write(mPreferences, &output);

    FILE* file = fopen(sourcePath, "w");

    if (!file)
    {
        CL_LOG_E("Config", "Couldn't open prefs file %s for write\n", sourcePath);
        return;
    }

    if (fwrite(output.data(), output.size(), 1, file) == 0)
        CL_LOG_E("Config", "Failed to write prefs data: %s\n", strerror(errno));

    CL_LOG("Config", "Wrote prefs file\n");

    fclose(file);
}

bool cConfigManager::OpenLastErrorFile()
{
    if (!mErrorFile.empty())
        return OpenTextFile(mErrorFile.c_str(), mErrorLine);

    mErrorLine = -1;
    return false;
}

bool cConfigManager::OpenConfig(const nCL::cValue* config)
{
    nCL::cFileSpec spec;

    if (!config)
        FindSpec(&spec, mConfig);
    else
        FindSpec(&spec, config->AsObject());

    OpenTextDirectory(spec.Directory());

    return false;
}


// Internal

namespace
{
    // TODO: this didn't work out, it's too hard to keep super/owner stuff in sync. Try doing
    // it during the Read() instead.
    void UpdateWithNewConfig(cObjectValue* targetObject, cObjectValue* sourceObject)
    /// Updates target with the contents of source, while keeping intact
    /// any still-relevant cObjectValue pointers from 'target'
    {
        // Strategy:
        // - UpdateConfig on any child members of the target that are present in the source
        // - Swap those members with the source's. (Thus source now preserves children of target.)
        // - Swap source with target.

        CL_ASSERT(targetObject);

        if (!sourceObject)
        {
            // Mark targetObject as no longer live. Maybe clear it?
            return;
        }

        for (int i = 0, n = sourceObject->NumMembers(); i < n; i++)
        {
            tTag tag = sourceObject->MemberTag(i);

            cValue* targetChildValue = targetObject->ModifyMember(tag);

            if (!targetChildValue)
                continue;

            cObjectValue* targetChild = targetChildValue->AsObject();
            cObjectValue* sourceChild = sourceObject->MemberValue(i).AsObject();

            if (!sourceChild)
            {
                if (targetChild)
                    ;   // TODO: clear/mark?

                continue;
            }

            if (!targetChild)
            {
                continue;
            }

            // update with source's contents so we maintain existing cObjectValue pointer
            targetChild->Swap(sourceChild);
            // Transfer target to source
            UpdateWithNewConfig(sourceChild, targetChild);
        }

        // We leave the 'super' stuff alone, because that's usually a reference either to another
        // file's object, or something already rooted in the tree.

        // now all the sourceObject members possible have had their cObjectValue
        // pointers replaced with the 'persistent' ones. So swap the sourceObject
        // contents into the target.
        targetObject->Swap(sourceObject);
    }
}

cObjectValue* cConfigManager::ConfigFileForPath(const nCL::cFileSpec& configPath, bool watchConfig)
{
    tStringID pathStringID = StringIDFromString(configPath.Path());
    auto it = mConfigFileToObject.find(pathStringID);

    if (it != mConfigFileToObject.end())
        return it->second;

    // Create this regardless of read success, so we can potentially reload later
    tObjectLink config = new(AllocatorFromObject(this)) cObjectValue;

    mConfigFileToObject[pathStringID] = config;

    bool result = ReadConfig(configPath, config);

    if (watchConfig)
    {
        if (!result && mShowErrors)
            OpenLastErrorFile();

        mDocumentsWatcher.AddFileID(pathStringID);    // Add regardless in case it reappears/becomes readable
    }

    return config.Transfer();
}

bool cConfigManager::ReloadConfigFileForPath(const nCL::cFileSpec& configPath)
{
    tStringID pathStringID = StringIDFromString(configPath.Path());
    auto it = mConfigFileToObject.find(pathStringID);

    if (it != mConfigFileToObject.end())
    {
        bool result =  ReadConfig(configPath, it->second);

        if (!result && mShowErrors)
            OpenLastErrorFile();

        return result;
    }

    return false;
}

bool cConfigManager::ReadConfig(const cFileSpec& configPath, cObjectValue* config)
{
    string errorMessages;
    int errorLine = 0;

    cValue wrapperValue(config);

    bool success = ReadFromJSONFile(configPath, &wrapperValue, &errorMessages, &errorLine);

    success = success && wrapperValue.IsObject();

    config->SetMember(kSourcePathTag, cValue(configPath.Path()));

    if (success)
    {
    #ifdef LOCAL_DEBUG
        LogJSON("Config", "ReadJSON", config);
    #endif

        config->IncModCount();
        ApplyImports(config);

    #ifdef LOCAL_DEBUG
        printf("\n");
        DumpHierarchy(configPath.Name(), 0, config);
        printf("\n");
    #endif
    }
    else
    {
        if (!errorMessages.empty())
            CL_LOG_E("Config", "%s: %s\n", configPath.Path(), errorMessages.c_str());

        mErrorFile = configPath.Path();
        mErrorLine = errorLine;
    }

    return success;
}

void cConfigManager::ApplyImports(cObjectValue* config)
{
    const cValue& v = config->Member("import");

    if (!v.IsNull())
    {
        cFileSpec configSpec;

        const char* sourcePath = config->Member(kSourcePathTag).AsString();

        if (sourcePath)
            configSpec.SetPath(sourcePath);
        else
            SetDirectory(&configSpec, kDirectoryData);

        if (v.IsObject())
            ApplyImport(v, configSpec, config);
        else
        {
            // TODO: at the moment, an import to a non-existent object can modify 'config'.
            for (int i = 0, n = v.size(); i < n; i++)
                ApplyImport(config->Member("import")[i], configSpec, config);
        }

        // nuke this guy now so it doesn't interfere with other stuff.
        config->RemoveMember("import");
    }
}

bool cConfigManager::ApplyImport(const cValue& import, const cFileSpec& configSpec, cObjectValue* config)
{
    CL_ASSERT(config);

    const char* platformStr = import["platform"].AsString();
    if (platformStr && !eq(platformStr, PlatformName()))
        return false;

    const char* configStr = import["config"].AsString();
    if (configStr && !eq(configStr, ConfigName()))
        return false;

    const char* notConfigStr = import["notConfig"].AsString();
    if (notConfigStr && eq(notConfigStr, ConfigName()))
        return false;

    const char* destStr = import["to"].AsString();
    cObjectValue* destConfig;

    if (destStr)
        destConfig = config->InsertMember(destStr).AsObject();  // this may create the value
    else
    {
        destConfig = config;
        destStr = ".";
    }

    const char* directoryPath = import["directory"].AsString();

    if (directoryPath)
    {
        bool useName = import["useName"].AsBool();

        cFileSpec spec(configSpec);
        spec.SetRelativeDirectory(directoryPath);

        cDirectoryInfo dirInfo;
        dirInfo.Read(spec.Directory());

        for (int i = 0, n = dirInfo.mFiles.size(); i < n; i++)
        {
            spec.SetNameAndExtension(dirInfo.mFiles[i].mName);

            if (!eqi(spec.Extension(), "json"))
                continue;

            cObjectValue* fileDestConfig;
            if (useName)
                fileDestConfig = destConfig->InsertMember(spec.Name()).AsObject();
            else
                fileDestConfig = destConfig;

            if (ImportFile(spec, fileDestConfig, true))
                CL_LOG_D("Config", "Imported %s to %s\n", spec.Path(), destStr);
        }

        return true;
    }

    const char* filePath = import["file"].AsString();

    if (filePath)
    {
        cFileSpec spec(configSpec);
        spec.SetRelativePath(filePath);

        if (ImportFile(spec, destConfig, import["required"].AsBool(true)))
        {
            CL_LOG_D("Config", "Imported %s to %s\n", spec.Path(), destStr);
            return true;
        }
    }

    // TODO: handle object:

    return false;
}

bool cConfigManager::ImportFile(const nCL::cFileSpec& spec, cObjectValue* target, bool required)
{
    if (!spec.IsReadable())
    {
        if (required)
            CL_LOG_E("Config", "%s doesn't exist or not readable\n", spec.Path());

        return false;
    }

    cObjectValue* importConfig = ConfigFileForPath(spec);

    if (!importConfig)
    {
        CL_LOG_E("Config", "Failed to read %s\n", spec.Path());
        return false;
    }

    InsertHierarchyAsSuper(importConfig, target);

    return true;
}

cIConfigManager* nHL::CreateConfigManager(nCL::cIAllocator* alloc)
{
    return new(alloc) cConfigManager;
}
