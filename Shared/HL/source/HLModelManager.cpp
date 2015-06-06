//
//  File:       ModelManager.cpp
//
//  Function:   Model display and animation management
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLModelManager.h>

#include <IHLConfigManager.h>

#include <HLDebugDraw.h>
#include <HLReadLXO.h>
#include <HLReadObj.h>
#include <HLReadAppleModel.h>
#include <HLServices.h>

#include <CLFrustum.h>
#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLValue.h>

using namespace nCL;
using namespace nHL;

namespace
{

}


cModelManager::cModelManager()
{
}

cModelManager::~cModelManager()
{
}

bool cModelManager::Init()
{
    // TODO: create a proper placeholder model
    CL_ASSERT(mModels.size() == 0);
    mModels.push_back();

    return true;
}

bool cModelManager::Shutdown()
{
    for (int i = 0; i < mModels.size(); i++)
        DestroyMesh(&mModels[i].mMeshLOD0);

    mModelTagToIndex.clear();
    mModels.clear();

    mInstanceSlots.ClearSlots();
    mInstanceTransforms.clear();
    mInstanceFlags.clear();
    mInstanceModelIndex.clear();

    return true;
}

bool cModelManager::LoadModels(const cObjectValue* config)
{
    cFileSpec baseSpec;

    for (auto c : config->Children())
    {
        tTag          modelTag  = c.Tag();
        const cValue& modelInfo = c.Value();

        if (!modelInfo.IsObject())
            continue;

        CL_LOG("ModelManager", "Adding model %s\n", c.Name());

        mModelTagToIndex[modelTag] = mModels.size();
        mModels.push_back();
        cModel& model = mModels.back();

        model.mTag = modelTag;
        model.mConfig = modelInfo.AsObject();

        const char* meshLOD0 = modelInfo[CL_TAG("lod0")].AsString();

        if (meshLOD0)
        {
            cFileSpec modelSpec;
            FindSpec(&modelSpec, c, meshLOD0);

            if (eqi(modelSpec.Extension(), "model"))
            {
                string modelPath;
                modelPath.assign(modelSpec.Path()); // TODO: string(const char*) doesn't copy -- fix?
                string texturePath = modelSpec.PathWithExtension("png");

                LoadMDLMesh(&model.mMeshLOD0, modelPath.c_str(), texturePath.c_str());
            }
            else if (eqi(modelSpec.Extension(), "lxo"))
            {
                LoadLXOScene(&model.mMeshLOD0, modelSpec.Path());
            }
            else if (eqi(modelSpec.Extension(), "obj"))
            {
                LoadObj(&model.mMeshLOD0, modelSpec.Path());
            }
            else
                CL_LOG_I("ModelManager", "Unsupported type: %s\n", modelSpec.Extension());


            // handle loading lxo, model, etc.
        }

        SetFromValue(modelInfo, &model.mMeshTransform);

        model.mMaterialTag = modelInfo[CL_TAG("material")].AsTag();

        if (model.mMaterialTag != 0)
            model.mMaterialIndex = HL()->mRenderer->MaterialRefFromTag(model.mMaterialTag);

        if (SetFromValue(modelInfo[CL_TAG("bounds")], &model.mBounds))
        {
            Vec3f originToCorner = MaxElts(-model.mBounds.mMin, model.mBounds.mMax);
            model.mBoundingRadius = len(originToCorner);
        }
        else
        {
            model.mBoundingRadius = modelInfo[CL_TAG("boundingRadius")].AsFloat(1.0f);
            model.mBounds.MakeCube(vl_0, model.mBoundingRadius / sqrtf(3.0f));
        }
    }
    
    return true;
}

void cModelManager::Update(float dt)
{
#ifndef CL_RELEASE
    if (HL()->mConfigManager->Preferences()->Member("showBounds").AsBool())
    {
        auto dd = HL()->mDebugDraw;

        dd->Reset();
        dd->SetColour(kColourGreen);

        for (int i = 0; i < mInstanceSlots.NumSlots(); i++)
        {
            if (!mInstanceSlots.InUse(i))
                continue;

            const cModel& model = mModels[mInstanceModelIndex[i]];

            if ((model.mMaterialIndex < 0) || !model.mMeshLOD0.mMesh || (mInstanceFlags[i] & kMIFlagHidden))
                continue;

            dd->SetTransform(mInstanceTransforms[i]);
            DrawBox(dd, model.mBounds.mMin, model.mBounds.mMax);
        }
    }
#endif
}

tMIRef cModelManager::CreateInstance(tTag modelTag)
{
    tTagToIndexMap::const_iterator it = mModelTagToIndex.find(modelTag);

    if (it != mModelTagToIndex.end())
    {
        tMIRef result = mInstanceSlots.CreateSlot();

        mInstanceModelIndex.resize(mInstanceSlots.NumSlots());
        mInstanceTransforms.resize(mInstanceSlots.NumSlots());
        mInstanceFlags     .resize(mInstanceSlots.NumSlots(), 0);

        mInstanceTransforms[result].MakeIdentity();
        mInstanceModelIndex[result] = it->second;

        return result;
    }

    return kNullRef;
}

bool cModelManager::DestroyInstance(tMIRef ref)
{
    // Don't have resources to release yet
    bool result =  mInstanceSlots.DestroySlot(ref);

    if (result)
    {
        mInstanceTransforms[ref].MakeIdentity();
        mInstanceModelIndex[ref] = -1;
        mInstanceFlags[ref] = 0;
    }

    return result;
}

void cModelManager::RemoveAllInstances()
{
    mInstanceSlots.ClearSlots();
    mInstanceModelIndex.clear();
    mInstanceTransforms.clear();
}

bool cModelManager::SetTransform(tMIRef ref, const cTransform& xform)
{
    // Don't have resources to release yet
    if (mInstanceSlots.InUse(ref))
    {
        mInstanceTransforms[ref] = xform;
        return true;
    }

    return false;
}


inline bool cModelManager::SetVisible(tMIRef ref, bool enabled)
{
    bool result = false;

    if (mInstanceSlots.InUse(ref))
    {
        result = !(mInstanceFlags[ref] & kMIFlagHidden);
        mInstanceFlags[ref] &= ~kMIFlagHidden;
        mInstanceFlags[ref] |= enabled ? 0 : kMIFlagHidden;
    }

    return result;
}

void cModelManager::CreateInstances(int count, const tTag tags[], tMIRef refs[])
{
    for (int i = 0; i < count ; i++)
    {
        tTagToIndexMap::const_iterator it = mModelTagToIndex.find(tags[i]);

        if (it == mModelTagToIndex.end())
        {
            refs[i] = kNullRef;
            continue;
        }

        tMIRef result = mInstanceSlots.CreateSlot();

        mInstanceModelIndex.resize(mInstanceSlots.NumSlots());
        mInstanceTransforms.resize(mInstanceSlots.NumSlots());
        mInstanceFlags     .resize(mInstanceSlots.NumSlots(), 0);

        mInstanceTransforms[result].MakeIdentity();
        mInstanceModelIndex[result] = it->second;

        refs[i] = result;
    }
}

void cModelManager::DestroyInstances(int count, tMIRef refs[])
{
    for (int i = 0; i < count; i++)
    {
        bool result = mInstanceSlots.DestroySlot(refs[i]);

        if (result)
        {
            mInstanceTransforms[refs[i]].MakeIdentity();
            mInstanceModelIndex[refs[i]] = -1;
            mInstanceFlags[refs[i]] = 0;
        }
    }
}



// cIRenderLayer

void cModelManager::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
{
    Vec2f orientedSize = renderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    cICamera* camera = renderer->Camera(kMainTag);

    Mat4f vp;

    if (camera)
        camera->FindViewProjection(orientedSize, &vp);
    else
        vp = vl_one;

    Vec4f planes[6];
    ExtractPlanes(vp, planes);
#ifndef CL_RELEASE
    OffsetPlanes(HL()->mConfigManager->Config()->Member(CL_TAG("clipPlanesOffset")).AsFloat(0.0f), planes);
#endif

    int8_t ip0[6][3];
    int8_t ip1[6][3];
    FindFrustumAABBIndices(planes, ip0, ip1);

    for (int i = 0; i < mInstanceSlots.NumSlots(); i++)
    {
        if (!mInstanceSlots.InUse(i))
            continue;

        const cModel& model = mModels[mInstanceModelIndex[i]];

        if ((model.mMaterialIndex < 0) || !model.mMeshLOD0.mMesh || (mInstanceFlags[i] & kMIFlagHidden))
            continue;

        const cTransform& modelTransform = mInstanceTransforms[i];

        tClipFlags clipFlags = FrustumTestSphere(modelTransform.Trans(), modelTransform.Scale() * model.mBoundingRadius, planes);

        if (clipFlags & kOutsideFrustum)
            continue;

        if (clipFlags & kIntersectsFrustum)
        {
            cBounds3 worldBounds(modelTransform.TransformBounds(model.mBounds)); // TODO: cache this

            clipFlags = FrustumTestAABB(worldBounds, planes, ip0, ip1, clipFlags);

            if (clipFlags & kOutsideFrustum)
                continue;
        }

        if (renderer->SetMaterial(model.mMaterialIndex))
        {
            Mat4f modelToWorld;

            if (model.mMeshTransform.IsIdentity())
                modelTransform.MakeMat4(&modelToWorld);
            else
            {
                ::Transform(modelTransform, model.mMeshTransform).MakeMat4(&modelToWorld);
            }

            renderer->SetShaderDataT(kDataIDModelToWorld, modelToWorld);
            renderer->DrawMesh(&model.mMeshLOD0);
        }
    }

    // Clear back to invalid material if any
    renderer->SetMaterial(0);
}


cIModelManager* nHL::CreateModelManager(nCL::cIAllocator* alloc)
{
    return new(alloc) cModelManager;
}
