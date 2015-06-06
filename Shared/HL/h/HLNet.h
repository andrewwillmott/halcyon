//
//  File:       HLNet.h
//
//  Function:   (WIP) routines for interacting with servers
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_NET_H
#define HL_NET_H

#include <HLDefs.h>

namespace nHL
{
    // --- cURL ----------------------------------------------------------------

    enum tURLField
    {
        kURLSchema,
        kURLHost,
        kURLPort,
        kURLPath,
        kURLQuery,
        kURLFragment,
        kURLUserInfo,
        kMaxURLFields
    };

    struct cURL
    /// Basic URL parsing
    {
        bool SetURL(const char* url);

        const char* URL() const;

        bool        HasField(tURLField field) const;        ///< Returns true if url has the given field
        const char* Field       (tURLField field) const;    ///< The returned string is ONLY valid until the next Field()/SetURL() call. Prefer ExtractField().
        void        ExtractField(tURLField field, nCL::string* s) const;

    protected:
        nCL::string mURL;
        int         mPort = 0;

        const char* mFields [kMaxURLFields] = { 0 };
        size_t      mLengths[kMaxURLFields] = { 0 };

        mutable nCL::string mLastField;
    };


    void ShowInterfaces();
    void ResolveName(const char* dnsName, const char* service);

    void TimerTest();
    void JobTest();
    bool ConnectTest();
    void URLTest();



    // --- Inlines -------------------------------------------------------------

    inline const char* cURL::URL() const
    {
        return mURL.c_str();
    }

    inline const char* cURL::Field(tURLField field) const
    {
        ExtractField(field, &mLastField);
        return mLastField.c_str();
        return mURL.c_str();
    }

    inline bool cURL::HasField(tURLField field) const
    {
        return mFields[field] != 0;
    }

    inline void cURL::ExtractField(tURLField field, nCL::string* s) const
    {
        if (mFields[field])
            s->assign(mFields[field], mLengths[field]);
        else
            s->clear();
    }
}



#endif
