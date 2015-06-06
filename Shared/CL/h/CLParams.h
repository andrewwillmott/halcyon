//
//  File:       CLParams.h
//
//  Function:   Support for a set of parameters
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_PARAMS_H
#define CL_PARAMS_H

#include <CLData.h>
#include <CLLink.h>
#include <CLMemory.h>

namespace nCL
{
    struct cParamInfo
    {
        tDataOffset mOffset     = kNullDataOffset;
        uint16_t    mSize       = 0;
        uint16_t    mModCount   = 0;
    };

    class cParams
    {
    public:
        bool        HasParams() const;          ///< Returns true if any parameters are set
        uint32_t    ParamsModCount() const;           ///< Returns a counter that changes on any modification

        bool        HasParam     (int ref) const;
        int         ParamSize    (int ref) const;
        uint32_t    ParamModCount(int ref) const;           ///< Returns a counter that changes on any modification

        template<class T> void      SetParam (int ref, const T& data);
        template<class T> void      SetParam (int ref, int count, const T* data);

        template<class T> const T   Param    (int ref, T defaultValue) const;
        template<class T> const T*  ParamData(int ref) const;  ///< Returns 0 if the value doesn't exist or is of 0 size, otherwise an array of length ParamSizeT().
        template<class T> int       ParamSize(int ref) const;

        // Base typeless API
        void        SetParamBase(int ref, int size, const void* data);
        const void* ParamBase   (int ref, int size) const;
        const void* ParamBase   (int ref, int size);

    protected:
        vector<cParamInfo>      mParams;
        cWriteableDataStore     mParamsStore;
        uint32_t                mModCount = 0;
    };

    class cParamsLinkable : public cParams, public cAllocLinkable
    {};

    // --- Inlines -------------------------------------------------------------

    inline bool cParams::HasParams() const
    {
        return !mParams.empty();
    }

    inline uint32_t cParams::ParamsModCount() const
    {
        return mModCount;
    }

    inline bool cParams::HasParam(int ref) const
    {
        return ref < mParams.size() && mParams[ref].mOffset != kNullDataOffset;
    }

    inline int cParams::ParamSize(int ref) const
    {
        if (ref < mParams.size())
            return mParams[ref].mSize;
        else
            return 0;
    }

    inline uint32_t cParams::ParamModCount(int ref) const
    {
        if (ref < mParams.size())
            return mParams[ref].mModCount;
        else
            return 0;
    }

    inline const void* cParams::ParamBase(int ref, int size) const
    {
        if (ref < mParams.size() && mParams[ref].mSize >= size)
            return mParamsStore.Data(mParams[ref].mOffset);
        else
            return 0;
    }

    inline const void* cParams::ParamBase(int ref, int size)
    {
        if (ref >= mParams.size() && mParams[ref].mSize >= size)
            return mParamsStore.Data(mParams[ref].mOffset);
        else
            return 0;
    }

    template<class T> inline void cParams::SetParam(int ref, const T& data)
    {
        SetParamBase(ref, sizeof(T), &data);
    }

    template<class T> inline void cParams::SetParam(int ref, int count, const T* data)
    {
        SetParamBase(ref, count * sizeof(T), data);
    }

    template<class T> inline const T cParams::Param(int ref, T defaultValue) const
    {
        const void* result = ParamBase(ref, int(sizeof(T)));

        if (result)
            return *reinterpret_cast<const T*>(result);

        return defaultValue;
    }

    template<class T> const T* cParams::ParamData(int ref) const
    {
        return reinterpret_cast<const T*>(ParamBase(ref, sizeof(T)));
    }
    template<class T> int cParams::ParamSize(int ref) const
    {
        CL_INDEX(ref, mParams.size());
        return mParams[ref].mSize / sizeof(T);
    }

}

#endif
