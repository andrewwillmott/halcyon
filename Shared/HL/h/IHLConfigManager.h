//
//  File:       IHLConfigManager.h
//
//  Function:   Manages configuration of app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_CONFIG_MANAGER_H
#define HL_I_CONFIG_MANAGER_H

#include <HLDefs.h>

namespace nCL
{
    class cFileSpec;
}

namespace nHL
{
    class cIConfigManager
    {
    public:
        virtual int Link(int count) const = 0;

        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual void Update() = 0;          ///< Updates internal change tracking
        virtual bool ConfigModified() = 0;  ///< Call to check if the config values have been modified since the last Update() -- returns true if so.

        virtual const cObjectValue* Config() = 0;       ///< Get root config value object. Config values can be accessed as Config()->Member("tagName")[->Member...]. The app config is always *read only*.

        virtual cObjectValue* AppInfo() = 0;            ///< Returns app info. Contains general runtime info, read/write.

        virtual cObjectValue* Preferences() = 0;        ///< Returns preferences. This is a special config value that gets auto-saved per user.
        virtual void          SavePreferences() = 0;    ///< Explicitly save preferences immediately. However, if the mod count of the preferences gets bumped, they will be auto-saved by the manager.

        virtual bool OpenLastErrorFile() = 0;
        ///< If there were config parse errors, opens a text editor at the first offending line and returns true, otherwise returns false.
        virtual bool OpenConfig(const nCL::cValue* config = 0) = 0;
        ///< Open root config file, or file associated with the given config
    };

    cIConfigManager* CreateConfigManager(nCL::cIAllocator* alloc);
 }

#endif
