//
//  File:       CLValue.h
//
//  Function:   Represents bool/int/float/string/array/object value
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_VALUE_H
#define CL_VALUE_H

#include <CLBounds.h>
#include <CLLink.h>
#include <CLMemory.h>
#include <CLSTL.h>
#include <CLTag.h>
#include <CLVecUtil.h>

#include <VL234i.h>

namespace nCL
{
    class cEnumInfo;
    class cTransform;
    class cFileSpec;

    /// Type of the value held by a Value object.
    enum tValueType : uint8_t
    {
        kValueNull,        ///< 'null' value
        kValueBool,        ///< bool value
        kValueInt,         ///< signed integer value   (range: INT32_MIN .. INT32_MAX)
        kValueUInt,        ///< unsigned integer value (range: 0 .. UINT32_MAX)
        kValueDouble,      ///< floating point value
        kValueString,      ///< UTF-8 string value
        kValueArray,       ///< array value (ordered list)
        kValueObject       ///< object value (id -> value map).
    };

    enum tCommentPlacement
    {
        kCommentBefore = 0,        ///< a comment placed on the line before a value
        kCommentAfterOnSameLine,   ///< a comment just after a value on the same line
        kCommentAfter,             ///< a comment on the line after a value (only make sense for root value)
        kNumberOfCommentPlacements
    };


    // --- cObjectChild --------------------------------------------------------

    class cValue;
    class cObjectValue;

    struct cObjectChild
    {
        int                 mIndex;
        const cObjectValue* mObject;

        bool                Exists() const;

        const cValue&       Value() const;
        const cObjectValue* ObjectValue() const;
        const tTagID        ID() const;
        const tTag          Tag() const;
        const char*         Name() const;

        const cObjectValue* Owner() const;      ///< (directly) owning object
    };
    typedef vector<cObjectChild> tObjectChildren;


    // --- cValue --------------------------------------------------------------

    class cValue
    /// Represents a generically typed value, of type Type() = tValueType.
    /// Note: this is designed to fail gracefully rather than asserting or crashing.
    /// - A null object will be returned for bogus queries. (Non-existent array value or object member.)
    /// - An array/object write will fail silently on the wrong kind of value.
    /// The idea is to avoid having to write a lot of error-checking code.
    {
    public:
        static const cValue kNull;
        static const tObjectChildren kNullChildren;
        static cValue kNullScratch;

        cValue();
        cValue(const cValue& other);

        explicit cValue(bool        value);
        explicit cValue(int32_t     value);
        explicit cValue(uint32_t    value);
        explicit cValue(double      value);
        explicit cValue(const char* value);
        explicit cValue(cObjectValue* object);  ///< Note -- does not copy object, just wraps it.
        explicit cValue(tValueType  type);

        ~cValue();

        void operator=(bool          value);
        void operator=(int32_t       value);
        void operator=(uint32_t      value);
        void operator=(double        value);
        void operator=(const char*   value);
        void operator=(cObjectValue* value);    ///< Note -- does not copy object, just attaches to it.
        void operator=(const cValue& value);

        void Swap(cValue& other);
        ///< Swap values. Currently, comments are intentionally not swapped, for both logic and efficiency.

        tValueType Type() const;

        const char*     AsString(const char* defaultValue = 0    ) const;
        tTag            AsTag   (tTag        defaultValue = kNullTag) const;
        uint32_t        AsID    (uint32_t    defaultValue = 0    ) const;
        int32_t         AsInt   (int32_t     defaultValue = 0    ) const;
        uint32_t        AsUInt  (uint32_t    defaultValue = 0    ) const;
        float           AsFloat (float       defaultValue = 0.0f ) const;
        double          AsDouble(double      defaultValue = 0.0  ) const;
        bool            AsBool  (bool        defaultValue = false) const;

        const cObjectValue* AsObject(const cObjectValue* defaultValue = nullptr) const;   ///< Returns 0 if not an object
        cObjectValue*       AsObject(      cObjectValue* defaultValue = nullptr);

        bool            IsNull()     const;
        bool            IsBool()     const;
        bool            IsInt()      const;
        bool            IsUInt()     const;
        bool            IsIntegral() const;     ///< true if bool, int, or uint
        bool            IsDouble()   const;
        bool            IsNumeric()  const;     ///< true if integral or double
        bool            IsString()   const;
        bool            IsArray()    const;     ///< true if an array or null (convertible to an array on write)
        bool            IsObject()   const;     ///< true if an object or null (convertible to an object on write)

        bool IsConvertibleTo(tValueType other) const;

        // Arrays + Objects, STL-alike API
        uint32_t size() const;            ///< Number of values in array or object
        bool     empty() const;           ///< Return true if empty array, empty object, or null; otherwise, false.
        void     clear();                 ///< Remove all object members and array elements.
        void     resize(uint32_t size);   ///< Resize the array to size elements. New elements are initialized to null.

        // Array API
        cValue&       operator[](uint32_t index);       ///< Access an array element (zero based index).
        const cValue& operator[](uint32_t index) const; ///< Access an array element (zero based index)

        int    NumElts() const;             ///< Returns number of elements in array
        const cValue& Elt(uint32_t index) const;

        void Append(const cValue& value);    ///< Append value to array at the end.

        // Object API
        const cValue& operator[](tTag tag) const;    ///< Access an object value by name, returns null if there is no member with that name.
        // We purposefully have no non-const operator [], as this will get called when we have non-const access to a cValue,
        // even if we're just reading it. This leads to bugs where a member is auto-created on read.

        const cValue& Member      (tTag tag) const;   ///< Return the member if it exists, kNull otherwise.
        cValue&       InsertMember(tTag tag);         ///< Return the member if it exists, otherwise insert kNull and return that.
        void          SetMember   (tTag tag, const cValue& v);  ///< Set given member
        bool          RemoveMember(tTag tag);        ///< Remove the named member, or return false if it doesn't exist.
        bool          HasMember   (tTag tag) const;  ///< Return true if the object has a member named key.

        const tObjectChildren& Children() const;  ///< Returns complete list of children, including those inherited.

        int           NumMembers() const;         ///< Returns number of members, or 0 if is not an object.
        const char*   MemberName (int i) const;   ///< Returns i'th member name
        uint32_t      MemberID   (int i) const;   ///< Returns i'th member id
        tTag          MemberTag  (int i) const;   ///< Returns i'th member tag
        const cValue& MemberValue(int i) const;   ///< Returns i'th member value

        // Comments
//        void SetComment(tStringID stringID,    tCommentPlacement placement);    ///< Comments must be //... or /* ... */
        void SetComment(const char* comment,   tCommentPlacement placement);    ///< Comments must be //... or /* ... */
        void SetComment(const string& comment, tCommentPlacement placement);    ///< Comments must be //... or /* ... */
        bool HasComment(tCommentPlacement placement) const;                     ///< Returns true if a comment exists in the given place.

        const char* Comment(tCommentPlacement placement) const;                 ///< Include delimiters and embedded newlines.

        // Comparisons
        bool operator < (const cValue& other) const;
        bool operator <=(const cValue& other) const;
        bool operator >=(const cValue& other) const;
        bool operator > (const cValue& other) const;

        bool operator!() const;                         ///< Returns IsNull()
        bool operator ==(const cValue& other) const;
        bool operator !=(const cValue& other) const;

        int Compare(const cValue& other);               ///< Trivalue comparison -- returns -1, 0, or 1

        // Low-level type management
        void MakeNull();    // Clears value back to null
        bool MakeArray();   // If value is null, converts to an array. Returns true in this case or if already an array.
        bool MakeObject();  // As per MakeArray for an object.

    protected:
        typedef vector<cValue> tArrayValues;

        struct cCommentInfo
        {
            cCommentInfo();
            ~cCommentInfo();

            void SetComment(const char* text);

            tStringID mComment;
        };

        union tValueHolder
        {
            int32_t         mInt;
            uint32_t        mUint;
            double          mDouble;
            bool            mBool;
            tStringID       mString;
            tArrayValues*   mArray;
            cObjectValue*   mObject;
        };

        tValueHolder    mValue = { 0 };
        tValueType      mType = kValueNull;

        cCommentInfo* mComments = 0;
    };



    // --- cArrayValue --------------------------------------------------------

    class cArrayValue
    {
        nCL::vector<cValue> mElts;
    };

    // --- cObjectValue --------------------------------------------------------

    struct cObjectValue : cAllocLinkable
    /// Represents an object -- a map from tags to values
    {
        ~cObjectValue();

        const cValue& operator[](tTag key) const;

        const cValue& Member      (tTag tag) const;              ///< Return member for the given key if it exists, kNull otherwise.
        const cValue& LocalMember (tTag tag) const;              ///< Same as Member() but looks at local members only.
        void          SetMember   (tTag tag, const cValue& v);   ///< Set given member
        cValue&       InsertMember(tTag tag);                    ///< Add given member if it doesn't exist already, returns for writing.
        cValue*       ModifyMember(tTag tag);                    ///< Returns member for writing, or 0 if it doesn't exist

        bool          HasMember     (tTag tag) const;  ///< Return true if the object has a member named key.
        bool          HasLocalMember(tTag tag) const;  ///< Return true if the object has a local member named key.
        bool          RemoveMember  (tTag tag);        ///< Remove the named member, or return false if it doesn't exist.

        bool          IsNull() const;             ///< Returns true if the object has no members.

        // These apply to the local object only.
        int           NumMembers() const;         ///< Returns number of members, or 0 if is not an object.
        const char*   MemberName (int i) const;   ///< Returns i'th member name
        uint32_t      MemberID   (int i) const;   ///< Returns i'th member id
        tTag          MemberTag  (int i) const;   ///< Returns i'th member tag
        const cValue& MemberValue(int i) const;   ///< Returns i'th member value
        cValue&       MemberValue(int i);         ///< Returns i'th member value
        void          RemoveMembers();            ///< Remove all members

        uint32_t      ModCount() const;
        void          IncModCount();

        int                 NumParents() const;
        const cObjectValue* Parent(int i) const;              ///< i'th parent object
        void                AddParent(const cObjectValue* parent);  ///< Inherit values from this parent. The parent added last will be queried first.
        void                RemoveParents();                  ///< Remove all parents

        const tObjectChildren& Children() const;        ///< Returns a list of all children, including inherited children
        cObjectChild  Child(tTag tag) const;            ///< Returns given child. Variant of Member() that identifies the actual owning object in the case of inheritance.

        cObjectValue* Owner() const;                    ///< Owner object, or 0 if none. The owner is usually the top-level object from the same source, e.g., file.
        void          SetOwner(cObjectValue* owner);    ///< Set owner for this

        void          Swap(cObjectValue* other);

        bool operator ==(const cObjectValue& other) const;

    protected:
        void AddChildren(tObjectChildren* children) const;

        // Data decls
        typedef map<tTag, cValue> tMap;
        typedef vector<cLink<const cObjectValue>> tParents;

        // Data
        tMap            mMap;
        tParents        mParents;     ///< If non-null, inherit from this object.
        cObjectValue*   mOwner = 0;   ///< If an owner exists, it's assumed to have a refcount on us.
        uint32_t        mModCount = 0;

        // Cache for full children list
        mutable tObjectChildren mChildren;
        mutable uint32_t        mChildrenModCount = ~0;
    };

    typedef cLink<cObjectValue>       tObjectLink;
    typedef cLink<const cObjectValue> tConstObjectLink;


    // --- Helpers -------------------------------------------------------------

    bool MemberExists(const cObjectValue* config, tTag key, const cValue** vOut);
    ///< Returns true and a pointer to the member in vOut if it exists.

    int SetFromValue(const cValue& value, int nv, int32_t v[]);     ///< Fills given array from value, returns number of elts set, which will be <= nv
    int SetFromValue(const cValue& value, int nv, uint32_t v[]);
    int SetFromValue(const cValue& value, int nv, float v[]);

    int SetFromValue(const cValue& value, Vec2i* v);
    int SetFromValue(const cValue& value, Vec3i* v);
    int SetFromValue(const cValue& value, Vec4i* v);

    int SetFromValue(const cValue& value, Vec2f* v);
    int SetFromValue(const cValue& value, Vec3f* v);
    int SetFromValue(const cValue& value, Vec4f* v);

    int SetFromValue(const cValue& value, nCL::vector<int>* v);
    int SetFromValue(const cValue& value, nCL::vector<float>* v);
    int SetFromValue(const cValue& value, nCL::vector<Vec2f>* v);
    int SetFromValue(const cValue& value, nCL::vector<Vec3f>* v);
    int SetFromValue(const cValue& value, nCL::vector<Vec4f>* v);

    bool SetFromValue(const cValue& value, cBounds2* bbox);
    bool SetFromValue(const cValue& value, cBounds3* bbox);
    void SetFromValue(const cValue& value, cTransform* xform);

    Vec2f AsVec2(const cValue& v, Vec2f defaultValue = vl_0);
    Vec3f AsVec3(const cValue& v, Vec3f defaultValue = vl_0);
    Vec4f AsVec4(const cValue& v, Vec4f defaultValue = vl_0);

    Vec2i AsVec2i(const cValue& value, Vec2i defaultV);
    Vec3i AsVec3i(const cValue& value, Vec3i defaultV);
    Vec4i AsVec4i(const cValue& value, Vec4i defaultV);

    cBounds2 AsBounds2(const cValue& value, cBounds2 defaultV);
    cBounds3 AsBounds3(const cValue& value, cBounds3 defaultV);

    float AsUnitFloat(const cValue& value, float dv = 0.0f);
    Vec3f AsUnitVec3 (const cValue& value, Vec3f dv = vl_0);
    float AsRangedFloat(const cValue& value, float minV, float maxV, float dv = 0.0f);
    Vec3f AsRangedVec3 (const cValue& value, float minV, float maxV, Vec3f dv = vl_0);

    Vec2f   AsRange(const cValue& value, Vec2f defaultValue = vl_0);
    int32_t AsEnum(const cValue& value, const cEnumInfo enumInfo[], int32_t defaultValue = -1);
    template<class T> T AsEnum(const cValue& value, const cEnumInfo enumInfo[], T defaultValue = T(0));


    bool MemberIsHidden(const char* memberName);
    #define CL_HIDDEN_VALUE_TAG(M_NAME) "_" M_NAME

    const char* TypeName(tValueType type);  ///< Returns debug name for given type

    bool FindSpec(cFileSpec* spec, const cObjectValue* object, const char* relativePath = 0);
    bool FindSpec(cFileSpec* spec, const cObjectChild& c, const char* relativePath = 0);
    ///< Checks for source file tag in the given object or its owner. Returns true if found and sets the spec accordingly.

    void InsertHierarchyAsSuper(cObjectValue* source, cObjectValue* target);
    ///< Insert a source object hierarchy into a target hierarchy as inheriting.
    void DumpHierarchy(const char* label, int indent, const cObjectValue* root, const cObjectValue* owner = 0);
    ///< Debug dump hierarchy

    // --- Inlines -------------------------------------------------------------

    inline cValue::cValue()
    {}

    inline cValue::cValue(bool value) : mType(kValueBool)
    {
        mValue.mBool = value;
    }
    inline cValue::cValue(int32_t value) : mType(kValueInt)
    {
        mValue.mInt = value;
    }
    inline cValue::cValue(uint32_t value) : mType(kValueUInt)
    {
        mValue.mUint = value;
    }
    inline cValue::cValue(double value) : mType(kValueDouble)
    {
        mValue.mDouble = value;
    }
    inline cValue::cValue(const char* value) : mType(kValueString)
    {
        mValue.mString = StringIDFromString(value);
    }
    inline cValue::cValue(cObjectValue* object) : mType(kValueObject)
    {
        mValue.mObject = object;
        mValue.mObject->Link(1);
    }

    inline void cValue::operator=(bool value)
    {
        MakeNull();
        mType = kValueBool;
        mValue.mBool = value;
    }
    inline void cValue::operator=(int32_t value)
    {
        MakeNull();
        mType = kValueInt;
        mValue.mInt = value;
    }
    inline void cValue::operator=(uint32_t value)
    {
        MakeNull();
        mType = kValueUInt;
        mValue.mUint = value;
    }
    inline void cValue::operator=(double value)
    {
        MakeNull();
        mType = kValueDouble;
        mValue.mDouble = value;
    }
    inline void cValue::operator=(const char* value)
    {
        MakeNull();
        mType = kValueString;
        mValue.mString = StringIDFromString(value);
    }

    inline void cValue::operator=(cObjectValue* object)
    {
        MakeNull();
        mType = kValueObject;
        mValue.mObject = object;
        mValue.mObject->Link(1);
    }

    inline tValueType cValue::Type() const
    {
        return mType;
    }

    // cObjectChild
    inline bool cObjectChild::Exists() const
    {
        return mObject != 0;
    }

    inline const cValue& cObjectChild::Value() const
    {
        return mObject ? mObject->MemberValue(mIndex) : cValue::kNull;
    }

    inline const cObjectValue* cObjectChild::ObjectValue() const
    {
        return mObject ? mObject->MemberValue(mIndex).AsObject() : 0;
    }

    inline const tTagID cObjectChild::ID() const
    {
        return tTagID(mObject->MemberID(mIndex));
    }

    inline const tTag cObjectChild::Tag() const
    {
        return mObject->MemberTag(mIndex);
    }

    inline const char* cObjectChild::Name() const
    {
        return mObject->MemberName(mIndex);
    }

    inline const cObjectValue* cObjectChild::Owner() const
    {
        return mObject;
    }

    // cObjectValue
    inline const cObjectValue* cValue::AsObject(const cObjectValue* defaultValue) const
    {
        if (mType == kValueObject)
            return mValue.mObject;

        return defaultValue;
    }

    inline cObjectValue* cValue::AsObject(cObjectValue* defaultValue)
    {
        if (MakeObject())
            return mValue.mObject;

        return defaultValue;
    }

    inline bool cValue::IsNull() const
    {
        return mType == kValueNull;
    }
    inline bool cValue::IsBool() const
    {
        return mType == kValueBool;
    }
    inline bool cValue::IsInt() const
    {
        return mType == kValueInt;
    }
    inline bool cValue::IsUInt() const
    {
        return mType == kValueUInt;
    }
    inline bool cValue::IsIntegral() const
    {
        return mType == kValueInt || mType == kValueUInt || mType == kValueBool;
    }
    inline bool cValue::IsDouble() const
    {
        return mType == kValueDouble;
    }
    inline bool cValue::IsNumeric() const
    {
        return mType == kValueInt || mType == kValueUInt || mType == kValueBool || mType == kValueDouble;
    }
    inline bool cValue::IsString() const
    {
        return mType == kValueString;
    }
    inline bool cValue::IsArray() const
    {
        return mType == kValueNull || mType == kValueArray;
    }
    inline bool cValue::IsObject() const
    {
        return mType == kValueNull || mType == kValueObject;
    }

    inline int cValue::NumElts() const
    {
        if (mType == kValueArray)
            return mValue.mArray->size();

        return 0;
    }
    inline const cValue& cValue::Elt(uint32_t index) const
    {
        return (*this)[index];
    }

    inline void cValue::resize(uint32_t newSize)
    {
        if (MakeArray())
            mValue.mArray->resize(newSize);
    }

    inline const cValue& cValue::operator[](tTag key) const
    {
        if (mType == kValueObject)
            return mValue.mObject->Member(key);

        return kNull;
    }

    inline const cValue& cValue::Member(tTag key) const
    {
        if (mType == kValueObject)
            return mValue.mObject->Member(key);

        return kNull;
    }

    inline cValue& cValue::InsertMember(tTag key)
    {
        if (MakeObject())
            return mValue.mObject->InsertMember(key);

        CL_ERROR("Can't insert a member on a non-object");
        kNullScratch = kNull;
        return kNullScratch;
    }

    inline void cValue::SetMember(tTag key, const cValue& v)
    {
        if (MakeObject())
            mValue.mObject->SetMember(key, v);
    }
    
    inline bool cValue::HasMember(tTag key) const
    {
        if (mType == kValueObject)
            return mValue.mObject->HasMember(key);

        return false;
    }

    inline bool cValue::RemoveMember(tTag key)
    {
        if (mType == kValueObject)
            return mValue.mObject->RemoveMember(key);

        return false;
    }

    inline const tObjectChildren& cValue::Children() const
    {
        if (mType == kValueObject)
            return mValue.mObject->Children();

        return kNullChildren;
    }

    inline int cValue::NumMembers() const
    {
        if (mType == kValueObject)
            return mValue.mObject->NumMembers();

        return 0;
    }

    inline const char* cValue::MemberName(int index) const
    {
        if (mType == kValueObject)
            return mValue.mObject->MemberName(index);

        return 0;
    }

    inline uint32_t cValue::MemberID(int index) const
    {
        if (mType == kValueObject)
            return mValue.mObject->MemberID(index);

        return 0;
    }

    inline tTag cValue::MemberTag(int index) const
    {
        if (mType == kValueObject)
            return mValue.mObject->MemberTag(index);

        return kNullTag;
    }

    inline const cValue& cValue::MemberValue(int index) const
    {
        if (mType == kValueObject)
            return mValue.mObject->MemberValue(index);

        return kNull;
    }

    inline bool cValue::operator!() const
    {
        return mType == kValueNull;
    }

    inline bool cValue::operator !=(const cValue& other) const
    {
        return !(*this == other);
    }


    // --- cObjectValue --------------------------------------------------------

    inline const cValue& cObjectValue::operator[](tTag key) const
    {
        return Member(key);
    }

    inline bool cObjectValue::IsNull() const
    {
        return mMap.empty();
    }

    inline int cObjectValue::NumMembers() const
    {
        return mMap.size();
    }

    inline tTag cObjectValue::MemberTag(int i) const
    {
        return mMap.at(i).first;
    }

    inline const cValue& cObjectValue::MemberValue(int i) const
    {
        return mMap.at(i).second;
    }

    inline cValue& cObjectValue::MemberValue(int i)
    {
        return mMap.at(i).second;
    }

    inline void cObjectValue::RemoveMembers()
    {
        mModCount++;
        mMap.clear();
    }

    inline uint32_t cObjectValue::ModCount() const
    {
        uint32_t count(mModCount);

        for (auto& p : mParents)
            count += p->ModCount();

        return count;
    }

    inline void cObjectValue::IncModCount()
    {
        mModCount++;
    }

    inline int cObjectValue::NumParents() const
    {
        return mParents.size();
    }

    inline const cObjectValue* cObjectValue::Parent(int i) const
    {
        return mParents[i];
    }

    inline void cObjectValue::AddParent(const cObjectValue* parent)
    {
        //CL_ASSERT(!IsParent(parent));
        CL_ASSERT(parent != this);

        mParents.push_back(parent);
    }

    inline void cObjectValue::RemoveParents()
    {
        mParents.clear();
    }

    inline cObjectValue* cObjectValue::Owner() const
    {
        return mOwner;
    }

    inline void cObjectValue::SetOwner(cObjectValue* owner)
    {
        CL_ASSERT(owner != this);
        mOwner = owner;
    }

    inline bool cObjectValue::operator == (const cObjectValue& other) const
    {
        return mMap == other.mMap;
    }


    // --- Utilities -----------------------------------------------------------

    inline bool MemberExists(const cObjectValue* config, tTag key, const cValue** vOut)
    {
        const cValue& v = config->Member(key);

        if (v.IsNull())
            return false;

        *vOut = &v;

        return true;
    }

    inline int SetFromValue(const cValue& value, int nv, int32_t v[])
    {
        int n = min(value.size(), nv);

        for (int i = 0; i < n; i++)
            v[i] = value[i].AsInt();

        return n;
    }

    inline int SetFromValue(const cValue& value, int nv, uint32_t v[])
    {
        int n = min(value.size(), nv);

        for (int i = 0; i < n; i++)
            v[i] = value[i].AsUInt();

        return n;
    }

    inline int SetFromValue(const cValue& value, int nv, float v[])
    {
        int n = min(value.size(), nv);

        for (int i = 0; i < n; i++)
            v[i] = value[i].AsFloat();

        return n;
    }

    inline bool SetFromValue(const cValue& value, cBounds2* bbox)
    {
        int n = value.size();

        if (n != 4)
            return false;

        float* v = &bbox->mMin[0];
        
        for (int i = 0; i < n; i++)
            v[i] = value[i].AsFloat();
        
        return true;
    }
    inline bool SetFromValue(const cValue& value, cBounds3* bbox)
    {
        int n = value.size();

        if (n != 6)
            return false;

        float* v = &bbox->mMin[0];

        for (int i = 0; i < n; i++)
            v[i] = value[i].AsFloat();

        return n;
    }
    inline int SetFromValue(const cValue& value, Vec2i* v)
    {
        int n = min(2, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsInt();
        
        return n;
    }
    inline int SetFromValue(const cValue& value, Vec3i* v)
    {
        int n = min(3, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsInt();
        
        return n;
    }
    inline int SetFromValue(const cValue& value, Vec4i* v)
    {
        int n = min(4, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsInt();
        
        return n;
    }
    inline int SetFromValue(const cValue& value, Vec2f* v)
    {
        int n = min(2, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsFloat();

        return n;
    }
    inline int SetFromValue(const cValue& value, Vec3f* v)
    {
        int n = min(3, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsFloat();

        return n;
    }
    inline int SetFromValue(const cValue& value, Vec4f* v)
    {
        int n = min(4, value.size());
        
        for (int i = 0; i < n; i++)
            (*v)[i] = value[i].AsFloat();

        return n;
    }

    inline Vec2f AsVec2(const cValue& value, Vec2f defaultV)
    {
        if (value.IsNumeric())
            return Vec2f(value.AsFloat());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline Vec3f AsVec3(const cValue& value, Vec3f defaultV)
    {
        if (value.IsNumeric())
            return Vec3f(value.AsFloat());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline Vec4f AsVec4(const cValue& value, Vec4f defaultV)
    {
        if (value.IsNumeric())
            return Vec4f(value.AsFloat());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline Vec2i AsVec2i(const cValue& value, Vec2i defaultV)
    {
        if (value.IsNumeric())
            return Vec2i(value.AsInt());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline Vec3i AsVec3i(const cValue& value, Vec3i defaultV)
    {
        if (value.IsNumeric())
            return Vec3i(value.AsInt());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline Vec4i AsVec4i(const cValue& value, Vec4i defaultV)
    {
        if (value.IsNumeric())
            return Vec4i(value.AsInt());

        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline cBounds2 AsBounds2(const cValue& value, cBounds2 defaultV)
    {
        SetFromValue(value, &defaultV);
        return defaultV;
    }
    inline cBounds3 AsBounds3(const cValue& value, cBounds3 defaultV)
    {
        SetFromValue(value, &defaultV);
        return defaultV;
    }

    inline float AsUnitFloat(const cValue& value, float dv)
    {
        return ClampUnit(value.AsFloat(dv));
    }

    inline Vec3f AsUnitVec3(const cValue& value, Vec3f dv)
    {
        return ClampUnit(AsVec3(value, dv));
    }

    inline float AsRangedFloat(const cValue& value, float minV, float maxV, float dv)
    {
        return Clamp(value.AsFloat(dv), minV, maxV);
    }

    inline Vec3f AsRangedVec3(const cValue& value, float minV, float maxV, Vec3f dv)
    {
        dv = AsVec3(value, dv);

        dv[0] = Clamp(dv[0], minV, maxV);
        dv[1] = Clamp(dv[1], minV, maxV);
        dv[2] = Clamp(dv[2], minV, maxV);

        return dv;
    }

    template<class T> inline T AsEnum(const cValue& value, const cEnumInfo enumInfo[], T defaultValue)
    {
        return T(AsEnum(value, enumInfo, int(defaultValue)));
    }

    inline bool MemberIsHidden(const char* key)
    {
        return key[0] == '_';
    }

    inline bool FindSpec(cFileSpec* spec, const cObjectChild& c, const char* relativePath)
    {
        return FindSpec(spec, c.Owner(), relativePath);
    }


}

#endif
