//
//  File:       HLConfigManager.h
//
//  Function:   Manages a collection of values used to data-drive the app.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_CONFIG_MANAGER_H
#define HL_CONFIG_MANAGER_H

#include <IHLConfigManager.h>
#include <HLDefs.h>

#include <CLFileWatch.h>
#include <CLMemory.h>
#include <CLTag.h>
#include <CLValue.h>

namespace nCL
{
    class cValue;
}

namespace nHL
{
    // --- cConfigManager -------------------------------------------------------

    class cConfigManager :
        public cIConfigManager,
        public nCL::cAllocLinkable
    /// Responsible for managing the set of cValues used to data-drive parts of
    /// the app.
    {
    public:
        CL_ALLOC_LINK_DECL;

        // cIConfigManager
        bool Init() override;
        bool Shutdown() override;

        void Update() override;
        bool ConfigModified() override;

        const cObjectValue* Config() override;

        cObjectValue* AppInfo() override;

        cObjectValue* Preferences() override;
        void SavePreferences() override;

        bool OpenLastErrorFile() override;
        bool OpenConfig(const nCL::cValue* config = 0) override;

        // cConfigManager
        uint32_t TagIDFromName(const char* tagName);
        ///< Returns the ID for the given named tag. For use in dev builds.

    protected:
        cObjectValue* ConfigFileForPath      (const nCL::cFileSpec& configPath, bool watchConfig = true);
        bool          ReloadConfigFileForPath(const nCL::cFileSpec& configPath);

        bool ReadConfig(const nCL::cFileSpec& configPath, cObjectValue* config);

        void ApplyImports(cObjectValue* config);
        bool ApplyImport(const cValue& import, const nCL::cFileSpec& configSpec, cObjectValue* config);
        bool ImportFile(const nCL::cFileSpec& spec, cObjectValue* target, bool required = true);

        // Data
        nCL::tObjectLink mLog;
        uint32_t         mLogModCount = 0;

        nCL::tConstObjectLink   mConfig;
        nCL::tObjectLink        mAppInfo;
        nCL::tObjectLink        mPreferences;
        uint32_t                mPreferencesModCount = 0;

        bool        mConfigModified = false;
        nCL::cFileWatcher mDocumentsWatcher;

        int         mErrorLine = -1;
        nCL::string mErrorFile;

        bool        mShowErrors = true;

        nCL::map<nCL::tStringID, nCL::tObjectLink> mConfigFileToObject;
    };

    // --- HL_DEFINE_TAG -------------------------------------------------------

    /// You can use this to declare tags that will be used for config lookup. E.g.,
    ///    HL_DEFINE_TAG("cColour");
    ///    ...
    ///    HL()->mConfigManager->Config().Member(kTag_Colour)
    #define HL_DEFINE_TAG(M_STR) static cTagID kTag_ ## M_STR (M_STR)

    struct cTagID
    {
        cTagID(const char* tagName);

        operator uint32_t() { return mID; }
        uint32_t mID;
    };


    // --- Inlines -------------------------------------------------------------

    inline const cObjectValue* cConfigManager::Config()
    {
        return mConfig;
    }
    inline cObjectValue* cConfigManager::AppInfo()
    {
        return mAppInfo;
    }
    inline cObjectValue* cConfigManager::Preferences()
    {
        return mPreferences;
    }
}

#endif
