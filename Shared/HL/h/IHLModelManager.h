//
//  File:       IHLModelManager.h
//
//  Function:   Manages loading and display of models for game code.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_MODEL_MANAGER_H
#define HL_I_MODEL_MANAGER_H

#include <HLDefs.h>
#include <CLSlotRef.h>

namespace nHL
{
    class cIRenderLayer;
    typedef nCL::cSlotRef tMIRef;   // model instance reference

    class cIModelManager
    {
    public:
        virtual int Link(int count) const = 0;

        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual bool LoadModels(const nCL::cObjectValue* config) = 0;
        ///< Loads models specified by the given config

        virtual void Update(float dt) = 0;
        ///< Call to update animations etc.

        // Instance manipulation.

        virtual tMIRef CreateInstance (tTag tag) = 0;           ///< Creates an returns a model instance.
        virtual bool   DestroyInstance(tMIRef ref) = 0;         ///< Releases the given instance. Returns true if the instance was destroyed.

        virtual void   RemoveAllInstances() = 0;                ///< Use to remove all orphaned instances

        virtual bool                SetTransform(tMIRef ref, const cTransform& xform) = 0;   ///< Sets transform for given model, or ignores it and returns false if the ref is not valid.
        virtual const cTransform&   Transform   (tMIRef ref) const = 0; ///< Returns current instance transform

        virtual bool                SetVisible  (tMIRef ref, bool enabled) = 0; ///< Sets visibility for given model, returns previous visibility.
        virtual bool                Visible     (tMIRef ref) const = 0;         ///< Returns current visibility

        virtual void   CreateInstances (int count, const tTag modelTags[], tMIRef modelRefs[]) = 0; ///< Creates model instances in bulk
        virtual void   DestroyInstances(int count, tMIRef modelTags[]) = 0;                         ///< Destroys model instances in bulk

        virtual tTag                ModelTag    (tMIRef ref) const = 0;     ///< Returns tag of the model used by this instance
        virtual const cObjectValue* ModelConfig (tMIRef ref) const = 0;     ///< Returns config object of the model used by this instance

        virtual cIRenderLayer* AsLayer() = 0;
        // TODO: animation etc.
    };

    cIModelManager* CreateModelManager(nCL::cIAllocator* alloc);
}

#endif
