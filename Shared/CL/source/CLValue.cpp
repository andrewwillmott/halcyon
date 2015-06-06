//
//  File:       CLValue.cpp
//
//  Function:   Generic value class. Originally based on jsoncpp, so it could fit in with the read/write code.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLValue.h>

#include <CLMemory.h>
#include <CLString.h>
#include <CLTransform.h>

// For FindSpec
#include <CLDirectories.h>
#include <CLFileSpec.h>

using namespace nCL;

namespace
{
    tTag kSourcePathTag = CL_TAG("_sourcePath");
}

const cValue cValue::kNull;
const tObjectChildren cValue::kNullChildren;
cValue cValue::kNullScratch;


/////////////////////////////////////////////////////////////////////////////////////
// cValue::cCommentInfo
//

cValue::cCommentInfo::cCommentInfo()
{
}

cValue::cCommentInfo::~cCommentInfo()
{
    if (mComment)
        ReleaseStringID(mComment);
}


void cValue::cCommentInfo::SetComment(const char* text)
{
    CL_ASSERT(text);
    CL_ASSERT_MSG(text[0]=='\0' || text[0]=='/', "Comments must start with /");

    if (mComment)
        ReleaseStringID(mComment);

    mComment = StringIDFromString(text);
}



/////////////////////////////////////////////////////////////////////////////////////
// cValue
//
// Default constructor initialization must be equivalent to:
//   memset(this, 0, sizeof(cValue))
// This optimization is used in ValueInternalMap fast allocator.
cValue::cValue(tValueType type) :
    mType(type)
{
    switch (type)
    {
    case kValueNull:
        break;
    case kValueInt:
    case kValueUInt:
        mValue.mInt = 0;
        break;
    case kValueDouble:
        mValue.mDouble = 0.0;
        break;
    case kValueString:
        mValue.mString = tStringID(0);
        break;
    case kValueArray:
        mValue.mArray = Create<tArrayValues>(Allocator(kValueAllocator));
        break;
    case kValueObject:
        mValue.mObject = new(Allocator(kValueAllocator)) cObjectValue;
        mValue.mObject->Link(1);
        break;
    case kValueBool:
        mValue.mBool = false;
        break;
    default:
        CL_ERROR("Invalid type");
    }
}

cValue::cValue(const cValue& other) :
    mType(other.mType)
{
    switch (mType)
    {
    case kValueNull:
    case kValueInt:
    case kValueUInt:
    case kValueDouble:
    case kValueBool:
        mValue = other.mValue;
        break;
    case kValueString:
        if (other.mValue.mString)
        {
            mValue.mString = other.mValue.mString;
            CopyStringID(mValue.mString);
        }
        else
            mValue.mString = tStringID(0);
        break;
    case kValueArray:
        mValue.mArray = Create<tArrayValues>(Allocator(kValueAllocator), *other.mValue.mArray);
        break;
    case kValueObject:
        mValue.mObject = new(Allocator(kValueAllocator)) cObjectValue(*other.mValue.mObject);
        mValue.mObject->Link(1);
        break;
    default:
        CL_ERROR("Invalid type");
    }

    if (other.mComments)
    {
        mComments = new(Allocator(kValueAllocator)) cCommentInfo[kNumberOfCommentPlacements];

        for (int comment = 0; comment < kNumberOfCommentPlacements; ++comment)
        {
            const cCommentInfo& otherComment = other.mComments[comment];

            if (otherComment.mComment)
                mComments[comment].mComment = CopyStringID(otherComment.mComment);
        }
    }
}


cValue::~cValue()
{
    MakeNull();

    if (mComments)
        DestroyArray(&mComments, kNumberOfCommentPlacements, Allocator(kValueAllocator));
}

void cValue::operator=(const cValue& other)
{
    cValue temp(other);
    Swap(temp);
}

void cValue::Swap(cValue& other)
{
    tValueType temp = mType;
    mType = other.mType;
    other.mType = temp;
    ustl::swap(mValue, other.mValue);
}

bool cValue::operator < (const cValue& other) const
{
    int typeDelta = mType - other.mType;
    if (typeDelta != 0)
        return typeDelta < 0;

    switch (mType)
    {
    case kValueNull:
        return false;
    case kValueInt:
        return mValue.mInt < other.mValue.mInt;
    case kValueUInt:
        return mValue.mUint < other.mValue.mUint;
    case kValueDouble:
        return mValue.mDouble < other.mValue.mDouble;
    case kValueBool:
        return mValue.mBool < other.mValue.mBool;
    case kValueString:
        return (mValue.mString == 0 && other.mValue.mString)
            || (other.mValue.mString
                && mValue.mString
                && strcasecmp(StringFromStringID(mValue.mString), StringFromStringID(other.mValue.mString)) < 0
                );
    case kValueArray:
        {
            int delta = int(mValue.mArray->size() - other.mValue.mArray->size());

            if (delta)
                return delta < 0;

            tArrayValues& m1 = *mValue.mArray;
            tArrayValues& m2 = *other.mValue.mArray;

            for (int i = 0, n = mValue.mArray->size(); i < n; i++)
                if (mValue.mArray->at(i) < other.mValue.mArray->at(i))
                    return true;

            return false;
        }
    case kValueObject:
        {
            int delta = int(mValue.mObject->NumMembers() - other.mValue.mObject->NumMembers());

            if (delta)
                return delta < 0;

            cObjectValue* m1 = mValue.mObject;
            cObjectValue* m2 = other.mValue.mObject;

            CL_ERROR("Need map < operator");
            return m1 < m2;
        }
    default:
        ;
    }

    return false;
}

bool cValue::operator <= (const cValue& other) const
{
    return !(other > *this);
}

bool cValue::operator >= (const cValue& other) const
{
    return !(*this < other);
}

bool cValue::operator > (const cValue& other) const
{
    return other < *this;
}

bool cValue::operator == (const cValue& other) const
{
    int temp = other.mType;

    if (mType != temp)
        return false;

    switch (mType)
    {
    case kValueNull:
        return true;
    case kValueInt:
        return mValue.mInt == other.mValue.mInt;
    case kValueUInt:
        return mValue.mUint == other.mValue.mUint;
    case kValueDouble:
        return mValue.mDouble == other.mValue.mDouble;
    case kValueBool:
        return mValue.mBool == other.mValue.mBool;
    case kValueString:
        return (mValue.mString == other.mValue.mString)
            || (other.mValue.mString  
                && mValue.mString == other.mValue.mString
               );
    case kValueArray:
        return mValue.mArray->size() == other.mValue.mArray->size()
            && (*mValue.mArray) == (*other.mValue.mArray);

    case kValueObject:
        return mValue.mObject->NumMembers() == other.mValue.mObject->NumMembers()
            && (*mValue.mObject) == (*other.mValue.mObject);
    }

    return false;
}

const char* cValue::AsString(const char* defaultValue) const
{
    switch (mType)
    {
    case kValueString:
        return mValue.mString ? StringFromStringID(mValue.mString) : "";
    case kValueBool:
        return mValue.mBool ? "true" : "false";
    default:
        ;
    }

    return defaultValue;
}

uint32_t cValue::AsID(uint32_t defaultValue) const
{
    switch (mType)
    {
    case kValueString:
        return mValue.mString ? IDFromStringID(mValue.mString) : defaultValue;
    case kValueUInt:
        return mValue.mUint;
    case kValueInt:
        if (mValue.mInt < 0)
            return 0;
        return uint32_t(mValue.mInt);
    default:
        ;
    }

    return defaultValue;
}

tTag cValue::AsTag(tTag defaultValue) const
{
    switch (mType)
    {
    case kValueString:
        return mValue.mString ? TagFromStringID(mValue.mString) : defaultValue;
    case kValueUInt:
    #if CL_TAG_DEBUG
        return 0;
    #else
        return tTag(mValue.mUint);
    #endif
    case kValueInt:
    #if CL_TAG_DEBUG
        return 0;
    #else
        if (mValue.mInt < 0)
            return kNullTag;
        return tTagID(mValue.mInt);
    #endif
    default:
        ;
    }

    return defaultValue;
}

int32_t cValue::AsInt(int32_t defaultValue) const
{
    switch (mType)
    {
    case kValueInt:
        return mValue.mInt;

    case kValueUInt:
        if (mValue.mUint > INT32_MAX)
            return INT32_MAX;
        return mValue.mUint;

    case kValueDouble:
        if (mValue.mDouble < double(INT32_MIN))
            return INT32_MIN;
        if (mValue.mDouble > double(INT32_MAX))
            return INT32_MAX;
        return int32_t(mValue.mDouble);

    case kValueBool:
        return mValue.mBool ? 1 : 0;

    default:
        ;
    }

    return defaultValue;
}

uint32_t cValue::AsUInt(uint32_t defaultValue) const
{
    switch (mType)
    {
    case kValueInt:
        if (mValue.mInt < 0)
            return 0;
        return mValue.mInt;
    case kValueUInt:
        return mValue.mUint;
    case kValueDouble:
        if (mValue.mDouble < 0.0)
            return 0;
        if (mValue.mDouble > double(UINT32_MAX))
            return UINT32_MAX;
        return uint32_t(mValue.mDouble);
    case kValueBool:
        return mValue.mBool ? 1 : 0;

    default:
        ;
    }

    return defaultValue;
}

float cValue::AsFloat(float defaultValue) const
{
    switch (mType)
    {
    case kValueInt:
        return mValue.mInt;
    case kValueUInt:
        return mValue.mUint;
    case kValueDouble:
        return mValue.mDouble;
    case kValueBool:
        return mValue.mBool ? 1.0f : 0.0f;

    default:
        ;
    }

    return defaultValue;
}

double cValue::AsDouble(double defaultValue) const
{
    switch (mType)
    {
    case kValueInt:
        return mValue.mInt;
    case kValueUInt:
        return mValue.mUint;
    case kValueDouble:
        return mValue.mDouble;
    case kValueBool:
        return mValue.mBool ? 1.0 : 0.0;

    default:
        ;
    }

    return defaultValue; // unreachable;
}

bool cValue::AsBool(bool defaultValue) const
{
    switch (mType)
    {
    case kValueInt:
    case kValueUInt:
        return mValue.mInt != 0;
    case kValueDouble:
        return mValue.mDouble != 0.0;
    case kValueBool:
        return mValue.mBool;
    case kValueString:
        return mValue.mString && mValue.mString[0] != 0;
    case kValueArray:
        return !mValue.mArray->empty();
    case kValueObject:
        return !mValue.mObject->IsNull();

    default:
        ;
    }

    return defaultValue;
}

bool cValue::IsConvertibleTo(tValueType other) const
{
    switch (mType)
    {
    case kValueNull:
        return true;

    case kValueInt:
        return (other == kValueNull && mValue.mInt == 0)
            || (other == kValueUInt && mValue.mInt >= 0)
            ||  other == kValueInt
            ||  other == kValueDouble
            ||  other == kValueString
            ||  other == kValueBool;

    case kValueUInt:
        return (other == kValueNull && mValue.mUint == 0)
            || (other == kValueInt  && mValue.mUint <= (unsigned)INT32_MAX)
            ||  other == kValueUInt
            ||  other == kValueDouble
            ||  other == kValueString
            ||  other == kValueBool;

    case kValueDouble:
        return (other == kValueNull && mValue.mDouble == 0.0)
            || (other == kValueInt  && mValue.mDouble >= INT32_MIN && mValue.mDouble <= INT32_MAX)
            || (other == kValueUInt && mValue.mDouble >= 0         && mValue.mDouble <= UINT32_MAX)
            ||  other == kValueDouble
            ||  other == kValueString
            ||  other == kValueBool;

    case kValueBool:
        return (other == kValueNull && mValue.mBool == false)
            ||  other == kValueInt
            ||  other == kValueUInt
            ||  other == kValueDouble
            ||  other == kValueString
            ||  other == kValueBool;

    case kValueString:
        return (other == kValueNull && (!mValue.mString || mValue.mString[0] == 0))
            ||  other == kValueString
            ||  other == kValueInt
            ||  other == kValueUInt
            ||  other == kValueDouble;

    case kValueArray:
        return (other == kValueNull && mValue.mArray->size() == 0)
            ||  other == kValueArray;

    case kValueObject:
        return (other == kValueNull && mValue.mObject->IsNull())
            ||  other == kValueObject;
    }

    return false;
}


// Array/Object

/// Number of values in array or object
uint32_t cValue::size() const
{
    switch (mType)
    {
    case kValueArray:  // size of the array is highest index + 1
        return mValue.mArray->size();

    case kValueObject:
        return mValue.mObject->NumMembers();

    default:
        ;
    }

    return 0;
}

bool cValue::empty() const
{
    switch (mType)
    {
    case kValueNull:
        return true;
    case kValueArray:
        return mValue.mArray->empty();
    case kValueObject:
        return mValue.mObject->IsNull();
    default:
        return false;
    }
}

void cValue::clear()
{
    switch (mType)
    {
    case kValueArray:
        mValue.mArray->clear();
        break;
    case kValueObject:
        mValue.mObject->RemoveMembers();
        break;
    default:
        break;
    }
}

// Array

cValue& cValue::operator[](uint32_t index)
{
    if (MakeArray())
    {
        // TODO: emulating old 'map' semantics.
        if (index >= mValue.mArray->size())
            mValue.mArray->resize(index + 1);

        return mValue.mArray->at(index);
    }

    CL_ERROR("Not an array");
    return *(cValue*) 0;
}

const cValue& cValue::operator[](uint32_t index) const
{
    if (mType != kValueArray || index >= mValue.mArray->size())
        return kNull;

    return mValue.mArray->at(index);
}


void cValue::Append(const cValue& value)
{
    if (MakeArray())
        mValue.mArray->push_back(value);
}

// Comments

void cValue::SetComment(const char* comment, tCommentPlacement placement)
{
    if (!mComments)
        mComments = new(Allocator(kValueAllocator)) cCommentInfo[kNumberOfCommentPlacements];

    mComments[placement].SetComment(comment);
}

void cValue::SetComment(const string& comment, tCommentPlacement placement)
{
    SetComment(comment.c_str(), placement);
}


bool cValue::HasComment(tCommentPlacement placement) const
{
    return mComments != 0 && mComments[placement].mComment != 0;
}

const char* cValue::Comment(tCommentPlacement placement) const
{
    if (HasComment(placement))
        return StringFromStringID(mComments[placement].mComment);

    return "";
}

// Internal
void cValue::MakeNull()
{
    switch (mType)
    {
    case kValueString:
        ReleaseStringID(mValue.mString);
        mValue.mString = tStringID(0);
        break;
    case kValueArray:
        Destroy(&mValue.mArray, Allocator(kValueAllocator));
        mValue.mArray = 0;
        break;
    case kValueObject:
        mValue.mObject->Link(-1);
        mValue.mObject = 0;
        break;
    default:
        break;
    }

    mType = kValueNull;
}

bool cValue::MakeArray()
{
    if (mType == kValueNull)
    {
        mType = kValueArray;
        mValue.mArray = Create<tArrayValues>(Allocator(kValueAllocator));
        return true;
    }

    return mType == kValueArray;
}

bool cValue::MakeObject()
{
    if (mType == kValueNull)
    {
        mType = kValueObject;
        mValue.mObject = new(Allocator(kValueAllocator)) cObjectValue;
        mValue.mObject->Link(1);
        return true;
    }

    return mType == kValueObject;
}


// --- cObjectValue ------------------------------------------------------------

cObjectValue::~cObjectValue()
{
}

const cValue& cObjectValue::Member(tTag key) const
{
#if CL_TAG_DEBUG
    const char* subKey = strchr(key, '.');
    tTag tag;

    if (subKey)
    {
        tag = TagFromString(key, subKey);
        subKey++;

        return Member(tag).Member(subKey);
    }

    tag = TagFromString(key);
    tMap::const_iterator it = mMap.find(tag);
#else
    tMap::const_iterator it = mMap.find(key);
#endif

    if (it != mMap.end())
        return it->second;

    for (auto& p : mParents)
    {
        const cValue& result = p->Member(key);

        if (&result != &cValue::kNull)
            return result;
    }

    return cValue::kNull;
}

const cValue& cObjectValue::LocalMember(tTag key) const
{
#if CL_TAG_DEBUG
    const char* subKey = strchr(key, '.');
    tTag tag;

    if (subKey)
    {
        tag = TagFromString(key, subKey);
        subKey++;
    }
    else
        tag = TagFromString(key);

    tMap::const_iterator it = mMap.find(tag);
#else
    tMap::const_iterator it = mMap.find(key);
#endif

    if (it != mMap.end())
    {
#if CL_TAG_DEBUG
        if (subKey)
            return (it->second)[subKey];
        else
#endif
            return it->second;
    }

    return cValue::kNull;
}

void cObjectValue::SetMember(tTag key, const cValue& v)
{
#if CL_TAG_DEBUG
    const char* subKey = strchr(key, '.');
    tTag tag;

    if (subKey)
    {
        tag = TagFromString(key, subKey);
        subKey++;

        return mMap[tag].SetMember(subKey, v);
    }

    tag = TagFromString(key);
    mMap[tag] = v;
#else
    mMap[key] = v;
#endif

    mModCount++;
}

cValue& cObjectValue::InsertMember(tTag key)
{
#if CL_TAG_DEBUG
    const char* subKey = strchr(key, '.');
    tTag tag;

    if (subKey)
    {
        tag = TagFromString(key, subKey);
        subKey++;

        cValue& v = mMap[tag];

        if (!v.IsObject())
            v = cValue(kValueObject);

        return v.AsObject()->InsertMember(subKey);
    }
    mModCount++;

    tag = TagFromString(key);
    return mMap[tag];
#else
    mModCount++;
    return mMap[key];
#endif
}

cValue* cObjectValue::ModifyMember(tTag key)
{
#if CL_TAG_DEBUG
    const char* subKey = strchr(key, '.');
    tTag tag;

    if (subKey)
    {
        tag = TagFromString(key, subKey);
        subKey++;

        cValue* v = ModifyMember(tag);

        if (v && v->IsObject())
            return v->AsObject()->ModifyMember(subKey);
    }
    else
    {
        tag = TagFromString(key);

        auto it = mMap.find(tag);

        if (it != mMap.end())
            return &it->second;
    }
#else
    auto it = mMap.find(key);

    if (it != mMap.end())
        return &it->second;
#endif

    return 0;
}

bool cObjectValue::RemoveMember(tTag key)
{
#if CL_TAG_DEBUG
    tTag tag = TagFromString(key);

    auto it = mMap.find(tag);
#else
    auto it = mMap.find(key);
#endif

    if (it == mMap.end())
        return false;

    mMap.erase(it);
    mModCount++;

    return true;
}

bool cObjectValue::HasMember(tTag key) const
{
#if CL_TAG_DEBUG
    tTag tag = TagFromString(key);

    if (mMap.find(tag) != mMap.end())
        return true;
#else
    if (mMap.find(key) != mMap.end())
        return true;
#endif

    for (auto& p : mParents)
        if (p->HasMember(key))
            return true;

    return false;
}

bool cObjectValue::HasLocalMember(tTag key) const
{
#if CL_TAG_DEBUG
    tTag tag = TagFromString(key);

    return mMap.find(tag) != mMap.end();
#else
    return mMap.find(key) != mMap.end();
#endif
}


const char* cObjectValue::MemberName(int index) const
{
    return StringFromTag(mMap.at(index).first);
}

uint32_t cObjectValue::MemberID(int index) const
{
    return IDFromTag(mMap.at(index).first);
}


cObjectChild cObjectValue::Child(tTag key) const
{
#if CL_TAG_DEBUG
    tTag tag = TagFromString(key);
#else
    tTag tag = key;
#endif

    tMap::const_iterator it = mMap.find(tag);
    
    if (it != mMap.end())
        return { it - mMap.begin(), this };

    for (auto& p : mParents)
    {
        cObjectChild result = p->Child(tag);

        if (result.mObject)
            return result;
    }

    return cObjectChild();
}

#if CL_TAG_DEBUG
namespace
{
    struct cTagString
    {
        bool operator()(cObjectChild a, cObjectChild b) const
        {
            const char* nameA = a.mObject->MemberName(a.mIndex);
            const char* nameB = b.mObject->MemberName(b.mIndex);

            return strcasecmp(nameA, nameB) < 0;
        }
    };
}
#endif

const tObjectChildren& cObjectValue::Children() const
{
    if (mChildrenModCount == mModCount)
        return mChildren;

    const cObjectValue* object = this;
    mChildren.clear();

    object->AddChildren(&mChildren);

#if CL_TAG_DEBUG
    sort(mChildren.begin(), mChildren.end(), cTagString());
#endif

//    mChildrenModCount = mModCount;

    return mChildren;
}

void cObjectValue::Swap(cObjectValue* other)
{
    mMap    .swap(other->mMap);
    mParents.swap(other->mParents);

    // Purposefully don't swap owner

    mModCount++;
    other->mModCount++;
}

// Internal

void cObjectValue::AddChildren(tObjectChildren* children) const
{
    int cursor = 0;
    int dn = children->size();

    // Add anything that's not already there.
    for (int i = 0, n = mMap.size(); i < n; i++)
    {
        if (MemberIsHidden(mMap.at(i).first))
            continue;

        while (cursor < dn && mMap.at(i).first > children->at(cursor).mObject->MemberTag(children->at(cursor).mIndex))
        {
            cursor++;
        }

        if (cursor >= dn)
            children->push_back( { i, this } );
        else if (mMap.at(i).first != children->at(cursor).mObject->MemberTag(children->at(cursor).mIndex))
        {
            children->insert(children->iat(cursor), { i, this } );
            dn++;
        }
    }

    for (auto& p : mParents)
        p->AddChildren(children);
}



// --- Utilities ---------------------------------------------------------------

int nCL::SetFromValue(const cValue& value, nCL::vector<int>* v)
{
    if (value.IsArray())
    {
        if (value.Elt(0).IsNumeric())
        {
            v->resize(value.NumElts());

            for (int i = 0, n = value.NumElts(); i < n; i++)
                v->at(i) = value[i].AsInt();
        }
    }
    else if (value.IsNumeric())
    {
        v->clear();
        v->resize(1, value.AsInt());
    }

    return v->size();
}

int nCL::SetFromValue(const cValue& value, nCL::vector<float>* v)
{
    if (value.IsArray())
    {
        if (value.Elt(0).IsNumeric())
        {
            v->resize(value.NumElts());

            for (int i = 0, n = value.NumElts(); i < n; i++)
                v->at(i) = value[i].AsFloat();
        }
    }
    else if (value.IsNumeric())
    {
        v->clear();
        v->resize(1, value.AsFloat());
    }

    return v->size();
}

int nCL::SetFromValue(const cValue& value, nCL::vector<Vec2f>* v)
{
    if (value.IsArray())
    {
        if (value.Elt(0).IsArray())
        {
            v->resize(value.NumElts());

            for (int i = 0, n = value.NumElts(); i < n; i++)
                v->at(i) = AsVec2(value[i]);
        }
        else if (value.Elt(0).IsNumeric())
        {
            v->clear();
            v->resize(1, AsVec2(value));
        }
    }

    return v->size();
}

int nCL::SetFromValue(const cValue& value, nCL::vector<Vec3f>* v)
{
    if (value.IsArray())
    {
        if (value.Elt(0).IsArray())
        {
            v->resize(value.NumElts());

            for (int i = 0, n = value.NumElts(); i < n; i++)
                v->at(i) = AsVec3(value[i]);
        }
        else if (value.Elt(0).IsNumeric())
        {
            v->clear();
            v->resize(1, AsVec3(value));
        }
    }

    return v->size();
}

int nCL::SetFromValue(const cValue& value, nCL::vector<Vec4f>* v)
{
    if (value.IsArray())
    {
        if (value.Elt(0).IsArray())
        {
            v->resize(value.NumElts());

            for (int i = 0, n = value.NumElts(); i < n; i++)
                v->at(i) = AsVec4(value[i]);
        }
        else if (value.Elt(0).IsNumeric())
        {
            v->clear();
            v->resize(1, AsVec4(value));
        }
    }

    return v->size();
}

void nCL::SetFromValue(const cValue& value, cTransform* xform)
{
    xform->SetTrans(AsVec3(value[CL_TAG("translation")], vl_0));
    xform->SetScale(value[CL_TAG("scale")].AsFloat(1.0f));

    const cValue& axisV = value[CL_TAG("axis")];
    if (!axisV.IsNull())
        xform->SetRot(AsVec3(axisV, vl_z), value[CL_TAG("theta")].AsFloat(0.0f));
    else
        xform->mRotation = vl_I;

    const cValue& headingV = value[CL_TAG("heading")];
    const cValue& pitchV   = value[CL_TAG("pitch"  )];
    const cValue& rollV    = value[CL_TAG("roll"   )];

    if (headingV.IsNumeric() || pitchV.IsNumeric() || rollV.IsNumeric())
    {
        xform->PrependRotZ(headingV.AsFloat(0.0f) * vl_pi / 180.0f);
        xform->PrependRotX(  pitchV.AsFloat(0.0f) * vl_pi / 180.0f);
        xform->PrependRotY(   rollV.AsFloat(0.0f) * vl_pi / 180.0f);
    }
}

Vec2f nCL::AsRange(const cValue& value, Vec2f r)
{
    if (value.IsArray())
    {
        if (value.size() >= 2)
        {
            r[0] = value.Elt(0).AsFloat();
            r[1] = value.Elt(1).AsFloat();
        }
        else if (value.size() == 1)
        {
            r[0] = value.Elt(0).AsFloat();
            r[1] = r[0];
        }
    }
    else
    {
        r[0] = value.AsFloat();
        r[1] = r[0];
    }

    return r;
}

int32_t nCL::AsEnum(const cValue& value, const cEnumInfo enumInfo[], int32_t defaultValue)
{
    if (value.IsString())
        return ParseEnum(enumInfo, value.AsString(), defaultValue);

    return defaultValue;
}

const char* nCL::TypeName(tValueType type)
{
    switch (type)
    {

    case kValueNull:
        return "nullt";
    case kValueBool:
        return "bool";
    case kValueInt:
        return "int";
    case kValueUInt:
        return "uint";
    case kValueDouble:
        return "double";
    case kValueString:
        return "string";
    case kValueArray:
        return "array";
    case kValueObject:
        return "object";
    default:
        ;
    }

    return "unknown";
}

bool nCL::FindSpec(cFileSpec* spec, const cObjectValue* config, const char* relativePath)
{
    const char* sourcePath = config ? config->LocalMember(kSourcePathTag).AsString() : 0;

    if (sourcePath)
    {
        spec->SetPath(sourcePath);
        if (relativePath)
            spec->SetRelativePath(relativePath);
        return true;
    }

    cObjectValue* owner = config ? config->Owner() : 0;

    if (owner)
        return FindSpec(spec, owner, relativePath);

    // Fallback to root-relative
    SetDirectory(spec, kDirectoryData);
    if (relativePath)
        spec->SetRelativePath(relativePath);

    return false;
}

void nCL::InsertHierarchyAsSuper(cObjectValue* source, cObjectValue* target)
{
    target->AddParent(source);

    for (int i = 0, n = source->NumMembers(); i < n; i++)
    {
        cObjectValue* sourceChild = source->MemberValue(i).AsObject();

        if (sourceChild)
        {
            cValue* targetChildValue = target->ModifyMember(source->MemberTag(i));

            if (targetChildValue && targetChildValue->IsObject())
                InsertHierarchyAsSuper(sourceChild, targetChildValue->AsObject());
        }
    }
}

void nCL::DumpHierarchy(const char* label, int indent, const cObjectValue* root, const cObjectValue* owner)
{
    if (!owner)
        owner = root;

    for (int i = 0; i < indent; i++)
        printf("  ");

    cFileSpec fileSpec;

    printf("%s: %p", label, root);

    if (root->Owner() != owner)
    {
        FindSpec(&fileSpec, root);

        printf(" (from %s)",
            fileSpec.Path()
        );
    }

    printf(", links %d", root->Link(0));

    printf(", modCount %d", root->ModCount());

    for (int i = 0, n = root->NumParents(); i < n; i++)
    {
        FindSpec(&fileSpec, root->Parent(i));

        printf(", parent %p (from %s)",
            root->Parent(i),
            fileSpec.Path()
        );
    }

    printf("\n");

    indent++;

    for (int i = 0, n = root->NumMembers(); i < n; i++)
    {
        const char* name = root->MemberName(i);
        const cValue& v = root->MemberValue(i);

        if (v.IsObject())
            DumpHierarchy(name, indent, v.AsObject());
    }
}

void WriteValue(const cValue& v)
{
    switch (v.Type())
    {
    case kValueString:
        break;
    case kValueArray:
        break;
    case kValueObject:
        break;

    default:
        break;
    }
}
