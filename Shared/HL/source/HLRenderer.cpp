//
//  File:       HLRenderer.cpp
//
//  Function:   Renderer system
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLRenderer.h>

#include <IHLApp.h>
#include <IHLConfigManager.h>

#include <HLCamera.h>
#include <HLGLUtilities.h>
#include <HLServices.h>
#include <HLUI.h>

#include <CLBits.h>
#include <CLDirectories.h>
#include <CLInputState.h>
#include <CLLog.h>
#include <CLMatUtil.h>
#include <CLMemory.h>
#include <CLFileSpec.h>
#include <CLString.h>
#include <CLValue.h>

using namespace nHL;
using namespace nCL;

#if !defined(CL_RELEASE) && GL_EXT_debug_label
    #define GL_DEBUG_LABEL(M_TYPE, M_NAME, M_LABEL) glLabelObjectEXT(M_TYPE, M_NAME, 0, M_LABEL);
    #define GL_DEBUG_MARKER(M_LABEL) glInsertEventMarkerEXT(0, M_LABEL);
    #define GL_DEBUG_BEGIN(M_LABEL) glPushGroupMarkerEXT(0, M_LABEL);
    #define GL_DEBUG_END() glPopGroupMarkerEXT()
#else
    #define GL_DEBUG_LABEL(M_TYPE, M_NAME, M_LABEL)
    #define GL_DEBUG_MARKER(M_LABEL)
    #define GL_DEBUG_BEGIN(M_LABEL)
    #define GL_DEBUG_END()
#endif

#ifdef CL_GLES
//    #define GL_USE_ORPHANING
    #define GL_MULTI_BUFFER
    #define GL_USE_MAP_RANGE
#else
    #define GL_USE_ORPHANING
    #define GL_MULTI_BUFFER

    // This is slow on OSX!
//    #define GL_USE_MAP
    // GL_USE_MAP_RANGE not available on OSX
#endif

#define QUAD_USAGE GL_STREAM_DRAW

//    iOS device info: https://developer.apple.com/library/ios/documentation/DeviceInformation/Reference/iOSDeviceCompatibility/OpenGLESPlatforms/OpenGLESPlatforms.html#//apple_ref/doc/uid/TP40013599-CH106-SW1

namespace
{
    const tTag kRenderLayerBackground      = CL_TAG("background");
    const tTag kRenderLayerMain            = CL_TAG("main");
    const tTag kRenderLayerForeground      = CL_TAG("foreground");

    const cEnumInfo kShaderDataEnum[] =
    {
        "ModelToWorld",     kDataIDModelToWorld,    
        "WorldToCamera",    kDataIDWorldToCamera,   
        "CameraToClip",     kDataIDCameraToClip,    

        "ModelToCamera",    kDataIDModelToCamera,
        "WorldToClip",      kDataIDWorldToClip,
        "ModelToClip",      kDataIDModelToClip,     

        "WorldToModel",     kDataIDWorldToModel,    
        "CameraToWorld",    kDataIDCameraToWorld,   
        "ClipToCamera",     kDataIDClipToCamera,    

        "CameraToModel",    kDataIDCameraToModel,
        "ClipToWorld",      kDataIDClipToWorld,
        "ClipToModel",      kDataIDClipToModel,

        "Resolution",       kDataIDViewSize,        
        "Centre",           kDataIDViewCentre,      
        "viewSize",         kDataIDViewSize,        // alias
        "viewCentre",       kDataIDViewCentre,      // alias

        "viewOffset",       kDataIDViewOffset,

        "DeviceOrient",     kDataIDDeviceOrient,
        "OrientedSize",     kDataIDOrientedViewSize,

        "Time",             kDataIDTime,
        "Pulse",            kDataIDPulse,

        "Mouse",            kDataIDPointer1,        // alias
        "Pointer1",         kDataIDPointer1,
        "Pointer2",         kDataIDPointer2,
        "Pointer3",         kDataIDPointer3,        
        "Pointer4",         kDataIDPointer4,        
        0, 0
    };

    const int kNumAliases = 3;  // bump this if you add aliases above

    CL_STATIC_ASSERT(CL_SIZE(kShaderDataEnum) - kNumAliases == kMaxBuiltInShaderDataID + 1);

    void UpdateModelToCamera(cIRenderer* renderer, size_t dataSize, void* dataOut)
    {
        CL_ASSERT(sizeof(Mat4f) <= dataSize);

        const Mat4f& modelToWorld  = renderer->ShaderDataT<Mat4f>(kDataIDModelToWorld );
        const Mat4f& worldToCamera = renderer->ShaderDataT<Mat4f>(kDataIDWorldToCamera);

        *reinterpret_cast<Mat4f*>(dataOut) = modelToWorld * worldToCamera;
    }

    void UpdateWorldToClip(cIRenderer* renderer, size_t dataSize, void* dataOut)
    {
        CL_ASSERT(sizeof(Mat4f) <= dataSize);

        const Mat4f& worldToCamera = renderer->ShaderDataT<Mat4f>(kDataIDWorldToCamera);
        const Mat4f& cameraToClip  = renderer->ShaderDataT<Mat4f>(kDataIDCameraToClip );

        *reinterpret_cast<Mat4f*>(dataOut) = worldToCamera * cameraToClip;
    }

    void UpdateModelToClip(cIRenderer* renderer, size_t dataSize, void* dataOut)
    {
        CL_ASSERT(sizeof(Mat4f) <= dataSize);

        const Mat4f& modelToWorld  = renderer->ShaderDataT<Mat4f>(kDataIDModelToWorld );
        const Mat4f& worldToCamera = renderer->ShaderDataT<Mat4f>(kDataIDWorldToCamera);
        const Mat4f& cameraToClip  = renderer->ShaderDataT<Mat4f>(kDataIDCameraToClip );

        Mat4f modelToClip = modelToWorld * worldToCamera * cameraToClip;
        CL_ASSERT(!HasNAN(modelToClip));

        *reinterpret_cast<Mat4f*>(dataOut) = modelToClip;
    }

    void UpdateOrientedViewSize(cIRenderer* renderer, size_t dataSize, void* dataOut)
    {
        Vec2f viewSize = renderer->ShaderDataT<Vec2f>(kDataIDViewSize);
        Mat3f od = renderer->ShaderDataT<Mat3f>(kDataIDDeviceOrient);

        Vec2f* orientedSize = reinterpret_cast<Vec2f*>(dataOut);
        CL_ASSERT(sizeof(*orientedSize) <= dataSize);

        *orientedSize =
        {
            dot(viewSize, Vec2f(fabsf(od(0, 0)), fabsf(od(0, 1)))),
            dot(viewSize, Vec2f(fabsf(od(1, 0)), fabsf(od(1, 1))))
        };
    }

#ifdef TODO
    // Still need to do the inverses.
    kDataIDWorldToModel,
    kDataIDCameraToWorld,
    kDataIDClipToCamera,

    kDataIDCameraToModel,
    kDataIDClipToModel,
    kDataIDClipToWorld,
#endif
}

namespace
{
    const cEnumInfo kShaderDataTypeEnum[] =
    {
        "float",    GL_FLOAT,
        "vec2",     GL_FLOAT_VEC2,
        "vec3",     GL_FLOAT_VEC3,
        "vec4",     GL_FLOAT_VEC4,
        "mat2",     GL_FLOAT_MAT2,
        "mat3",     GL_FLOAT_MAT3,
        "mat4",     GL_FLOAT_MAT4,
        "int",      GL_INT,
        "ivec2",    GL_INT_VEC2,
        "ivec3",    GL_INT_VEC3,
        "ivec4",    GL_INT_VEC4,
        0, 0
    };
    // Shader data config types/functions

    void ConfigFloat(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config)
    {
        renderer->SetShaderDataT(ref, config.AsFloat(0.0f));
    }
    void ConfigVec2(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config)
    {
        renderer->SetShaderDataT(ref, AsVec2(config, vl_0));
    }
    void ConfigVec3(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config)
    {
        renderer->SetShaderDataT(ref, AsVec3(config, vl_0));
    }
    void ConfigVec4(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config)
    {
        renderer->SetShaderDataT(ref, AsVec4(config, vl_0));
    }
    void ConfigRGBA(cIRenderer* renderer, tShaderDataRef ref, const nCL::cValue& config)
    {
        renderer->SetShaderDataT(ref, AsVec4(config, vl_1));
    }

    tShaderDataConfigFunc* ConfigFuncForType(GLenum dataType)
    {
        switch (dataType)
        {
        case GL_FLOAT:
            return ConfigFloat;
        case GL_FLOAT_VEC2:
            return ConfigVec2;
        case GL_FLOAT_VEC3:
            return ConfigVec3;
        case GL_FLOAT_VEC4:
            return ConfigVec4;
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
        default:
            break;
        }

        return 0;
    }
}


/* NOTES

- Material:
  - shader program obv.
  - render state
  -> from json?
  
- Model:
  - Mesh per LOD
  - Material
  
  Define:
  - renderbuffers (+depth/stencil) and textures
  - framebuffers
  
  Then layer is:
  framebuffer: xxx,
  drawLayer: []

  General practice was 
  - set textures and targets
  - set renderType (and other shader data?)
  - overrideMaterial
  - block if()
  - some built-in commands like clear(), drawRect()
  - render (layer)

*/

namespace
{
    // All of this crap courtesy of designed-by-committee OpenGL/GLES.

    const cEnumInfo kBufferFormatEnum[] =
    {
        "rgb",              kFormatRGB,
        "rgba",             kFormatRGBA,
        "alpha",            kFormatA,
        "luminance",        kFormatL,
        "luminanceAlpha",   kFormatLA,
        "depth",            kFormatDepth,
        "stencil",          kFormatStencil,

        "rgba4",            kFormatRGBA4,
        "rgb5a1",           kFormatRGB5A1,
        "rgb565",           kFormatRGB565,
        "rgba8",            kFormatRGBA8,

        "alpha8",           kFormatA8,
        "luminance8",       kFormatL8,
        "luminanceAlpha8",  kFormatLA8,

        "a8",               kFormatA8,
        "l8",               kFormatL8,
        "la8",              kFormatLA8,

        "depth16",          kFormatD16,
        "depth24",          kFormatD24,
        "depth32",          kFormatD32,

        "stencil8",         kFormatS8,

        "depth24Stencil8",  kFormatD24S8,
        0, 0
    };

    const GLuint kGLGenericFormat[] =
    {
        GL_RGB,             // kFormatRGB,
        GL_RGBA,            // kFormatRGBA,
        GL_ALPHA,           // kFormatA,
        GL_LUMINANCE,       // kFormatL,
        GL_LUMINANCE_ALPHA, // kFormatLA,
        GL_DEPTH_COMPONENT, // kFormatDepth,
    #ifdef GL_STENCIL_INDEX
        GL_STENCIL_INDEX,   // kFormatStencil,
    #else
        // TODO: SDK 5 *removed* this? What the fuck?
        GL_NONE,   // kFormatStencil,
    #endif

        GL_RGBA,            // kFormatRGBA4,
        GL_RGBA,            // kFormatRGB5A1,
        GL_RGB,             // kFormatRGB565,
        
        GL_RGBA,            // kFormatRGBA8,

        GL_ALPHA,           // kFormatA8,
        GL_LUMINANCE,       // kFormatL8,
        GL_LUMINANCE_ALPHA, // kFormatLA8,

        GL_DEPTH_COMPONENT, // kFormatD16,
        GL_DEPTH_COMPONENT, // kFormatD24,
        GL_DEPTH_COMPONENT, // kFormatD32,

    #ifdef GL_STENCIL_INDEX
        GL_STENCIL_INDEX,   // kFormatS8,
    #else
        // TODO: SDK 5 *removed* this? What the fuck?
        GL_NONE,   // kFormatStencil,
    #endif
        GL_DEPTH_STENCIL,   // kFormatD24S8,
    };
    CL_STATIC_ASSERT(CL_SIZE(kGLGenericFormat) == kMaxBufferFormats);

    const GLuint kGLType[] =
    {
        GL_UNSIGNED_BYTE,       // kFormatRGB,
        GL_UNSIGNED_BYTE,       // kFormatRGBA,
        GL_UNSIGNED_BYTE,       // kFormatA,
        GL_UNSIGNED_BYTE,       // kFormatL,
        GL_UNSIGNED_BYTE,       // kFormatLA,
        GL_UNSIGNED_INT_24_8,   // kFormatDepth,
        GL_UNSIGNED_BYTE,       // kFormatStencil,

        GL_UNSIGNED_SHORT_4_4_4_4,  // kFormatRGBA4,
        GL_UNSIGNED_SHORT_5_5_5_1,  // kFormatRGB5A1,
        GL_UNSIGNED_SHORT_5_6_5,    // kFormatRGB565,
        
        GL_UNSIGNED_BYTE,       // kFormatRGBA8,

        GL_UNSIGNED_BYTE,       // kFormatA8,
        GL_UNSIGNED_BYTE,       // kFormatL8,
        GL_UNSIGNED_BYTE,       // kFormatLA8,

        GL_UNSIGNED_SHORT,      // kFormatD16,
        GL_UNSIGNED_INT_24_8,   // kFormatD24,

        GL_NONE,                // kFormatD32,

        GL_UNSIGNED_BYTE,       // kFormatS8,

        GL_UNSIGNED_INT_24_8,   // kFormatD24S8,
    };
    CL_STATIC_ASSERT(CL_SIZE(kGLType) == kMaxBufferFormats);

    const GLuint kGLInternalFormat[] =
    {
        GL_RGB,             // kFormatRGB,
        GL_RGBA,            // kFormatRGBA,
        GL_ALPHA,           // kFormatA,
        GL_LUMINANCE,       // kFormatL,
        GL_LUMINANCE_ALPHA, // kFormatLA,
        GL_DEPTH_COMPONENT, // kFormatDepth,
#ifdef GL_STENCIL_INDEX
        GL_STENCIL_INDEX,   // kFormatStencil,
#else
        GL_STENCIL_INDEX8,
#endif
        GL_RGBA4,            // kFormatRGBA4,
        GL_RGB5_A1,          // kFormatRGB5A1,

    #ifdef GL_RGB565
        GL_RGB565,           // kFormatRGB565,
    #else
        // Not present on OSX GL
        GL_RGB5_A1,          // kFormatRGB565,
    #endif

        GL_RGBA8,            // kFormatRGBA8,

        GL_ALPHA8,           // kFormatA8,
        GL_LUMINANCE8,       // kFormatL8,
        GL_LUMINANCE8_ALPHA8, // kFormatLA8,

        GL_DEPTH_COMPONENT16, // kFormatD16,
        GL_DEPTH_COMPONENT24, // kFormatD24,

    #ifdef GL_DEPTH_COMPONENT32
        GL_DEPTH_COMPONENT32, // kFormatD32,
    #else
        GL_DEPTH_COMPONENT,   // kFormatD32,
    #endif

#ifdef GL_STENCIL_INDEX
        GL_STENCIL_INDEX,   // kFormatS8,
#else
        GL_STENCIL_INDEX8,   // kFormatS8,
#endif

        GL_DEPTH_STENCIL,   // kFormatD24S8,
    };
    CL_STATIC_ASSERT(CL_SIZE(kGLInternalFormat) == kMaxBufferFormats);

    int FormatNumChannels(tBufferFormat bufferFormat)
    {
        switch (kGLGenericFormat[bufferFormat])
        {
        case GL_RGB:
            return 3;
        case GL_RGBA:
            return 4;
        case GL_LUMINANCE_ALPHA:
            return 2;
        }

        return 1;
    }

    bool IsDepthOrStencilFormat(tBufferFormat bufferFormat)
    {
        GLenum format = kGLGenericFormat[bufferFormat];

#ifdef GL_STENCIL_INDEX
        return (format == GL_DEPTH_COMPONENT|| format == GL_STENCIL_INDEX || format == GL_DEPTH_STENCIL);
#else
        return (format == GL_DEPTH_COMPONENT|| format == GL_STENCIL_INDEX8 || format == GL_DEPTH_STENCIL);
#endif
    }
}

namespace
{
    // default: GL_REPEAT
    const cEnumInfo kAddressEnum[] =
    {
        "wrap",             GL_REPEAT,
        "clamp",            GL_CLAMP_TO_EDGE,
        "mirror",           GL_MIRRORED_REPEAT,
        0, 0
    };

    // default: GL_LINEAR
    const cEnumInfo kMagFilterEnum[] =
    {
        "point",            GL_NEAREST,
        "linear",           GL_LINEAR,
        0, 0
    };

    // default: GL_NEAREST_MIPMAP_LINEAR(!!)
    const cEnumInfo kMinFilterEnum[] =
    {
        "point",            GL_NEAREST,
        "linear",           GL_LINEAR,
        "pointMip",         GL_NEAREST_MIPMAP_NEAREST,
        "linearMipPoint",   GL_LINEAR_MIPMAP_NEAREST,
        "pointMipLinear",   GL_NEAREST_MIPMAP_LINEAR,
        "linearMip",        GL_LINEAR_MIPMAP_LINEAR,
        0, 0
    };

    enum tDefaultClampSetting
    {
        kClampOn = false,
        kClampOff = true
    };

    enum tDefaultMIPSetting
    {
        kMIPOn = false,
        kMIPOff = true,
    };

    void ConfigTextureParams
    (
        const cObjectValue*  config,
        GLenum               target,
        tDefaultClampSetting clamp = kClampOff,
        tDefaultMIPSetting   mip   = kMIPOn,
        bool*                requiresMIPs = 0
    )
    {
        const cValue* v;

        GLenum addressS = (clamp == kClampOn) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        GLenum addressT = GL_NONE;

        GLenum magFilter = GL_LINEAR;
        GLenum minFilter = (mip == kMIPOff) ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR;

        if (config)
        {
            if (MemberExists(config, "address", &v))
            {
                if (v->IsString())
                    addressS = AsEnum(*v, kAddressEnum, addressS);
                else if (v->IsArray())
                {
                    if (v->size() > 0)
                        addressS = AsEnum(v->Elt(0), kAddressEnum, addressS);
                    if (v->size() > 1)
                        addressT = AsEnum(v->Elt(1), kAddressEnum, addressT);
                }
            }

            if (MemberExists(config, "minFilter", &v))
                minFilter = AsEnum(*v, kMinFilterEnum, minFilter);

            if (MemberExists(config, "magFilter", &v) || MemberExists(config, "filter", &v))
                magFilter = AsEnum(*v, kMagFilterEnum, minFilter);
        }

        if (addressT == GL_NONE)
            addressT = addressS;

        glTexParameteri(target, GL_TEXTURE_WRAP_S, addressS);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, addressT);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);

        if (requiresMIPs)
            *requiresMIPs = minFilter > GL_LINEAR;
    }

}

namespace
{
    const cEnumInfo kFrameBufferAttachmentEnum[] =
    {
        "colour0",  GL_COLOR_ATTACHMENT0,
    #ifdef GL_COLOR_ATTACHMENT1
        "colour1",  GL_COLOR_ATTACHMENT1,
        "colour2",  GL_COLOR_ATTACHMENT2,
        "colour3",  GL_COLOR_ATTACHMENT3,
    #endif
        "depth",    GL_DEPTH_ATTACHMENT,
        "stencil",  GL_STENCIL_ATTACHMENT,
        0, 0
    };

    const char* FrameBufferError(GLenum error)
    {
        switch (error)
        {
        case GL_FRAMEBUFFER_COMPLETE:
            return "framebuffer successfully defined";

        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "an attachment is incomplete";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "need at least one attachment";

        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "buffer combination unsupported";

    #ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            return "not all attachments have same dimensions";
    #endif

    #ifdef GL_FRAMEBUFFER_UNDEFINED
        case GL_FRAMEBUFFER_UNDEFINED:
            return "no default framebuffer";
    #endif

    #ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            return "the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAWBUFFERi.";
    #endif

    #ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
           return "GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.";
    #endif

    #ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "inconsistent multisample types";
    #endif

        default:
            return "unknown framebuffer issue";
        }
    }
}

void cRenderCommand::Config(const cObjectValue* config, const cRenderer* renderer)
{
    const cObjectValue* flagsV = config->Member("flags").AsObject();

    if (flagsV)
    {
        for (int i = 0, n = flagsV->NumMembers(); i < n; i++)
        {
            tTag tag = flagsV->MemberTag(i);

            int flagIndex = renderer->RenderFlagFromTag(tag);

            if (flagIndex >= 0)
            {
                bool flagValue = flagsV->MemberValue(i).AsBool();

                tRenderFlags flagMask = 1 << flagIndex;
                mRenderFlagMask |= flagMask;

                if (flagValue)
                    mRenderFlagValues |= flagMask;
            }
            else
            {
                CL_LOG_E("Renderer", "Unknown flag " CL_TAG_FMT "\n", tag);
            }
        }
    }

    const cValue* v;

    if (MemberExists(config, "drawLayer", &v))
    {
        mType = kRCDrawLayer;
        mSet.mTag = v->AsTag();
        return;
    }

    if (MemberExists(config, "drawJobGroup", &v))
    {
        mType = kRCDrawJobGroup;
        mSet.mTag = v->AsTag();
        return;
    }

    if (MemberExists(config, "frameBuffer", &v))
    {
        mType = kRCSetFrameBuffer;
        mSet.mTag = v->AsTag();

        if (strcmp(mSet.mTag, kDefaultTag) == 0)
            mSet.mTag = 0;

        return;
    }

    if (MemberExists(config, "camera", &v))
    {
        mType  = kRCSetCamera;
        mSet.mTag = v->AsTag();
        return;
    }

    if (MemberExists(config, "material", &v))
    {
        mType = kRCSetMaterial;
        mSet.mTag = v->AsTag();
        return;
    }

    if (MemberExists(config, "texture", &v))
    {
        mType = kRCSetTexture;

        mSet.mTag   = v->AsTag();
        mSet.mStage = config->Member("stage").AsUInt(0);
        return;
    }

    if (MemberExists(config, "diffuseMap", &v))
    {
        mType = kRCSetTexture;

        mSet.mTag   = v->AsTag();
        mSet.mStage = kTextureDiffuseMap;
        return;
    }

    if (MemberExists(config, "normalMap", &v))
    {
        mType = kRCSetTexture;

        mSet.mTag   = v->AsTag();
        mSet.mStage = kTextureNormalMap;
        return;
    }

    if (MemberExists(config, "shaderData", &v))
    {
        mType  = kRCSetShaderData;

        mShaderData.mRef = renderer->ShaderDataRefFromTag(v->AsTag());

        if (mShaderData.mRef == kNullDataRef)
            CL_LOG_E("Renderer", "Unknown shader data in render command: " CL_TAG_FMT, "\n", v->AsTag());
        else
            mShaderData.mConfig = &config->Member("value");
        return;
    }

    if (MemberExists(config, "clear", &v))
    {
        mType = kRCClear;
        *(Vec4f*) mClear.mC = AsVec4(*v);
        mClear.mBufferFlags = 0;

        if (config->Member("colour").AsBool(true))
            mClear.mBufferFlags |= GL_COLOR_BUFFER_BIT;
        if (config->Member("depth").AsBool(true))
            mClear.mBufferFlags |= GL_DEPTH_BUFFER_BIT;
        if (config->Member("stencil").AsBool(false))
            mClear.mBufferFlags |= GL_STENCIL_BUFFER_BIT;

        return;
    }

    if (MemberExists(config, "drawRect", &v))
    {
        mType = kRCDrawRect;

        *(Vec4f*) mDrawRect.mRect = AsVec4(*v);

        cColourAlpha ca;
        ca.AsColour() = AsVec3(config->Member("colour"), kColourWhite);
        ca.AsAlpha()  = config->Member("alpha").AsFloat(1.0f);

        mDrawRect.mColourAlphaU32 = ColourAlphaToRGBA32(ca);

        mDrawRect.mFlipY = config->Member("flipY").AsBool(false);
        mDrawRect.mFlipX = config->Member("flipX").AsBool(false);
        return;
    }

    if (MemberExists(config, "drawRectAt", &v))
    {
        mType = kRCDrawRect;

        Vec2f rectAt   = AsVec2(*v);
        Vec2f rectSize = AsVec2(config->Member("size"), vl_1);

        (Vec2f&) mDrawRect.mRect[0] = rectAt;
        (Vec2f&) mDrawRect.mRect[2] = rectAt + rectSize;

        cColourAlpha ca;
        ca.AsColour() = AsVec3(config->Member("colour"), kColourWhite);
        ca.AsAlpha()  = config->Member("alpha").AsFloat(1.0f);

        mDrawRect.mColourAlphaU32 = ColourAlphaToRGBA32(ca);

        mDrawRect.mFlipY = config->Member("flipY").AsBool(false);
        mDrawRect.mFlipX = config->Member("flipX").AsBool(false);
        return;
    }

    if (MemberExists(config, "copy", &v))
    {
        mType = kRCBlit;

        mBlit.mSourceTag = v->AsTag();

        (Vec4f&) mBlit.mSrcRect[0] = AsVec4(config->Member("from"));
        (Vec4f&) mBlit.mDstRect[0] = AsVec4(config->Member("to"));

        mBlit.mBufferFlags = 0;

        if (config->Member("colour").AsBool(true))
            mBlit.mBufferFlags |= GL_COLOR_BUFFER_BIT;
        if (config->Member("depth").AsBool(false))
            mBlit.mBufferFlags |= GL_DEPTH_BUFFER_BIT;
        if (config->Member("stencil").AsBool(false))
            mBlit.mBufferFlags |= GL_STENCIL_BUFFER_BIT;

        return;
    }

    if (MemberExists(config, "generateMips", &v))
    {
        mType = kRCGenerateMips;
        mSet.mTag = v->AsTag();
        return;
    }

    if (MemberExists(config, "discard", &v))
    {
        int attachment = AsEnum(*v, kFrameBufferAttachmentEnum);

        if (attachment >= 0)
        {
            mType = kRCDiscardBuffer;
            mDiscard.mAttachment = attachment;
        }
        else
            CL_LOG_E("Renderer", "Unknown discard buffer: " CL_TAG_FMT "\n", v->AsString());
    }

    if (MemberExists(config, "copyBack", &v))
    {
        // Copy current FB back to CPU
        mType = kRCCopyBack;
        mCopyBack.mDestTag = v->AsTag();

        mCopyBack.mXY[0] = 0;
        mCopyBack.mXY[1] = 0;
        mCopyBack.mWH[0] = 0;
        mCopyBack.mWH[1] = 0;

        SetFromValue(config->Member("at"),   CL_SIZE(mCopyBack.mXY), mCopyBack.mXY);
        SetFromValue(config->Member("size"), CL_SIZE(mCopyBack.mWH), mCopyBack.mWH);
    }
}

void cDataLayer::Dispatch(cIRenderer* rendererIn, const cRenderLayerState& state)
{
    cRenderer* renderer = static_cast<cRenderer*>(rendererIn);

    for (const auto& command : mCommands)
        if ((state.mFlags & command.mRenderFlagMask) == command.mRenderFlagValues)
            renderer->ApplyCommand(command, state);
}

void cDataLayer::Config(const cValue& config, const cRenderer* renderer)
{
    const cValue* commandsV;

    if (config.IsArray())
        commandsV = &config;
    else
        commandsV = &config.Member("commands");

    mCommands.resize(commandsV->NumElts());

    for (int i = 0, n = mCommands.size(); i < n; i++)
        mCommands[i].Config(commandsV->Elt(i).AsObject(), renderer);
}


// --- cRenderer ---------------------------------------------------------------

cRenderer::cRenderer()
{
}

cRenderer::~cRenderer()
{
}

bool cRenderer::Init()
{
    mAllocator = Allocator(kDefaultAllocator);

    mTime = 0.0f;
    mPulse = 0.0f;

    // Set up built-in shader data
    mShaderData.clear();
    mShaderData.resize(kMaxBuiltInShaderDataID);

    Mat4f m4I = vl_I;

    SetShaderDataT(kDataIDModelToWorld,     m4I);
    
    SetShaderDataT(kDataIDModelToCamera,    m4I);
    SetShaderDataT(kDataIDModelToClip,      m4I);
    SetShaderDataT(kDataIDWorldToCamera,    m4I);
    SetShaderDataT(kDataIDWorldToClip,      m4I);
    SetShaderDataT(kDataIDCameraToClip,     m4I);
    SetShaderDataT(kDataIDWorldToModel,     m4I);
    SetShaderDataT(kDataIDCameraToModel,    m4I);
    SetShaderDataT(kDataIDClipToModel,      m4I);
    SetShaderDataT(kDataIDCameraToWorld,    m4I);
    SetShaderDataT(kDataIDClipToWorld,      m4I);
    SetShaderDataT(kDataIDClipToCamera,     m4I);

    SetShaderDataT(kDataIDViewSize,         Vec2f(vl_0));
    SetShaderDataT(kDataIDViewCentre,       Vec2f(vl_0));
    SetShaderDataT(kDataIDViewOffset,       Vec2f(vl_0));

    SetShaderDataT(kDataIDDeviceOrient,     Mat3f(vl_I));
    SetShaderDataT(kDataIDOrientedViewSize, Vec2f(vl_0));

    SetShaderDataT(kDataIDTime,             0.0f);
    SetShaderDataT(kDataIDPulse,            0.0f);

    SetShaderDataT(kDataIDPointer1,         Vec2f(vl_0));
    SetShaderDataT(kDataIDPointer2,         Vec2f(vl_0));
    SetShaderDataT(kDataIDPointer3,         Vec2f(vl_0));
    SetShaderDataT(kDataIDPointer4,         Vec2f(vl_0));

    // Built-in update routines
    {
        tShaderDataRef deps[] = { kDataIDModelToWorld, kDataIDWorldToCamera };
        SetShaderDataUpdate(kDataIDModelToCamera, UpdateModelToCamera, CL_SIZE(deps), deps);
    }
    {
        tShaderDataRef deps[] = { kDataIDWorldToCamera, kDataIDCameraToClip };
        SetShaderDataUpdate(kDataIDWorldToClip, UpdateWorldToClip, CL_SIZE(deps), deps);
    }
    {
        tShaderDataRef deps[] = { kDataIDModelToWorld, kDataIDWorldToCamera,  kDataIDCameraToClip };
        SetShaderDataUpdate(kDataIDModelToClip, UpdateModelToClip, CL_SIZE(deps), deps);
    }

    {
        tShaderDataRef deps[] = { kDataIDViewSize, kDataIDDeviceOrient };
        SetShaderDataUpdate(kDataIDOrientedViewSize, UpdateOrientedViewSize, CL_SIZE(deps), deps);
    }

    // Built-in config functions
    SetShaderDataConfig(kDataIDViewSize,        ConfigVec2);
    SetShaderDataConfig(kDataIDViewCentre,      ConfigVec2);
    SetShaderDataConfig(kDataIDViewOffset,      ConfigVec2);

    SetShaderDataConfig(kDataIDTime,            ConfigFloat);
    SetShaderDataConfig(kDataIDPulse,           ConfigFloat);

    for (const cEnumInfo* info = kShaderDataEnum; info->mName; info++)
        mShaderDataTagToIndex[TagFromStrConst(info->mName)] = info->mValue;     // kShaderDataEnum[].mName should all be C string constants

    // Mesh
    mQuadMeshSlots.ClearSlots();
    int nullMesh = mQuadMeshSlots.CreateSlot();    // ensure first slot is never used.
    CL_ASSERT(nullMesh == 0);

    mMaterials.push_back();
    mMaterialTagToIndex[CL_TAG("invalid")] = 0;

    mDocumentsWatcher.Init();

    return true;
}

bool cRenderer::Shutdown()
{
    mDocumentsWatcher.Shutdown();

    mCodeLayers.clear();
    mDataLayers.clear();

    for (auto i : mFrameBufferTagToIndex)
    {
        GLuint name = i.second;
        glDeleteFramebuffers(1, &name);
    }

    mFrameBufferTagToIndex.clear();
    mFrameBufferInfo.clear();

    for (auto i : mRenderBufferTagToIndex)
    {
        GLuint name = i.second;
        glDeleteRenderbuffers(1, &name);
    }

    mRenderBufferTagToIndex.clear();
    mRenderBufferInfo.clear();

    for (auto i : mTextureTagToIndex)
    {
        GLuint name = i.second;
        glDeleteTextures(1, &name);
    }

    mShaderDataTagToIndex.clear();
    mShaderData.clear();

    mTextureTagToIndex.clear();
    mTextureInfo.clear();

    mMaterialTagToIndex.clear();
    mMaterials.clear();

    return true;
}

// cIRenderer

void cRenderer::SetFrameBufferInfo(uint32_t fb, int width, int height, int orientation)
{
    if (fb >= mFrameBufferInfo.size())
        mFrameBufferInfo.resize(fb + 1);

    mFrameBufferInfo[fb].mSize = { width, height };
    mFrameBufferInfo[fb].mDeviceOriented = true;    // by definition!
    mDefaultFrameBuffer = fb;

    SetShaderDataT<Vec2f>(kDataIDViewSize, Vec2f(width, height));
    mDeviceOrientation = orientation;

    switch (orientation)
    {
    case kOrientPortraitDown:
        mDeviceOrient = HRot3f(vl_pi);
        break;
    case kOrientLandscapeLeft:
        mDeviceOrient = HRot3f(-vl_halfPi);
        break;
    case kOrientLandscapeRight:
        mDeviceOrient = HRot3f(+vl_halfPi);
        break;
    default:
        mDeviceOrient = vl_I;
        break;
    }

    SetShaderDataT(kDataIDDeviceOrient, mDeviceOrient);

    CL_LOG("Renderer", "Set framebuffer to %d x %d, orientation: %d\n", width, height, orientation);
}

void cRenderer::Update(float dt)
{
    vector<int> changedRefs;
    vector<int> addedRefs;
    vector<int> removedRefs;

    if (mDocumentsWatcher.FilesChanged(&changedRefs, &addedRefs, &removedRefs))
    {
        CL_LOG("Config", "Shader files change...\n");

        for (int i = 0, n = addedRefs.size(); i < n; i++)
            CL_LOG("Config", "  Added: %s\n", mDocumentsWatcher.PathForRef(addedRefs[i]));

        for (int i = 0, n = removedRefs.size(); i < n; i++)
            CL_LOG("Config", "  Removed: %s\n", mDocumentsWatcher.PathForRef(removedRefs[i]));

        set<int> materialsToReload;

        for (int i = 0, n = changedRefs.size(); i < n; i++)
        {
            CL_LOG("Config", "  Changed: %s\n", mDocumentsWatcher.PathForRef(changedRefs[i]));

            const auto range = mRefToMaterialIndexMap.equal_range(changedRefs[i]);

            for (auto it = range.first; it != range.second; ++it)
            {
                mMaterialsToReload.insert(it->second);
                mMaterialReloadCounter = 10;
            }
        }
    }

    if (!mMaterialsToReload.empty() && --mMaterialReloadCounter == 0)
    {
        for (int i : mMaterialsToReload)
        {
            cMaterial& material = mMaterials[i];

            const char* vsPath = mDocumentsWatcher.PathForRef(material.mVSRef);
            const char* fsPath = mDocumentsWatcher.PathForRef(material.mFSRef);

            printf("reload material %d (%s / %s)\n", i, vsPath, fsPath);

            if (LoadShaders(vsPath, fsPath, material.mShaderProgram, "reload") != 0)
            {
                material.mShaderDataBindings.clear();
                GetBindingsFromProgram(material.mShaderProgram, &material.mShaderDataBindings);

                material.mTextureBindings.clear();
                GetBindingsFromProgram(material.mShaderProgram, &material.mTextureBindings);
            }
        }

        mMaterialsToReload.clear();
    }

    mTime += dt;
    mPulse += dt;
    mPulse -= floorf(mPulse);

    SetShaderDataT<float>(kDataIDTime, mTime);
    SetShaderDataT<float>(kDataIDPulse, mPulse);
}

void cRenderer::Render()
{
    cRenderLayerState state;

    state.mFlags = mRenderFlags;

    // Set up some standard state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    bool wireframe = HL()->mConfigManager->Preferences()->Member("wireframe").AsBool();

#ifndef CL_GLES
    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDefaultFrameBuffer);
    Vec2i bufferSize = mFrameBufferInfo[mDefaultFrameBuffer].mSize;
	glViewport(0, 0, bufferSize[0], bufferSize[1]);
    mCurrentFrameBuffer = mDefaultFrameBuffer;

    GL_CHECK;

    Vec2f viewSize(bufferSize[0], bufferSize[1]);

    SetShaderDataT<Vec2f>(kDataIDViewSize,   viewSize);
    SetShaderDataT<Vec2f>(kDataIDViewCentre, viewSize * 0.5f);

    ResetState();

    GL_CHECK;

    DispatchJobGroup(CL_TAG("preRender"), state);

    if (!DispatchLayer(mRenderLayer, state, "RenderLayer") && (mRenderLayer == kMainTag))
    {
        // temporary
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const tTag kDefaultLayers[] =
        {
            kRenderLayerBackground,
            kRenderLayerModelManager,
            kRenderLayerMain,
            kRenderLayerForeground,
            kRenderLayerEffectsManager,
            kRenderLayerDebugDraw
        };

        for (int i = 0; i < CL_SIZE(kDefaultLayers); i++)
        {
            const cLayerInfo* layerInfo = LayerInfo(kDefaultLayers[i]);

            if (layerInfo && layerInfo->mEnabled)
            {
                SetShaderDataT(kDataIDModelToWorld, Mat4f(vl_I));
                PushState("Layer");

                state.mLayerTag = kDefaultLayers[i];
                state.mLayerFlags = layerInfo->mFlags;
                layerInfo->mLayer->Dispatch(this, state);

                PopState("Layer");
            }
        }
    }

    DispatchJobGroup(CL_TAG("postRender"), state);

    mRenderJobs.clear();
}

// Cameras

void cRenderer::RegisterCamera(tTag cameraTag, cICamera* camera)
{
    if (camera)
        mCameras[cameraTag] = camera;
    else
        mCameras.erase(cameraTag);
}

cICamera* cRenderer::Camera(tTag cameraTag) const
{
    auto it = mCameras.find(cameraTag);

    if (it != mCameras.end())
        return it->second;

    return 0;
}

// Layers

void cRenderer::LoadLayersAndBuffers(const cObjectValue* config)
{
    if (!config)
        return;

    // TODO mRenderFlagTags.clear();

    const cValue& renderFlagsV = config->Member("flags");

    for (int i = 0, n = renderFlagsV.NumMembers(); i < n; i++)
    {
        tTag tag = renderFlagsV.MemberTag(i);
        int flag = renderFlagsV.MemberValue(i).AsInt(-1);

        if (flag >= 0 && flag < kMaxRenderFlags)
            mRenderFlagTags[tag] = flag;
    }

    LoadRenderBuffers(config->Member("renderBuffers").AsObject());
    LoadFrameBuffers (config->Member("frameBuffers").AsObject());
    LoadLayers       (config->Member("layers").AsObject());
}

void cRenderer::RegisterLayer(tTag layerTag, cIRenderLayer* layer, uint32_t flags)
{
    if (layer)
    {
        cLayerInfo info;

        info.mLayer = layer;
        info.mFlags = flags;

        mCodeLayers[layerTag] = info;
    }
    else
        mCodeLayers.erase(layerTag);
}

cIRenderLayer* cRenderer::Layer(tTag layerTag) const
{
    auto it = mCodeLayers.find(layerTag);

    if (it != mCodeLayers.end())
        return it->second.mLayer;

    auto it2 = mDataLayers.find(layerTag);

    if (it2 != mDataLayers.end())
        return it2->second.mLayer;

    return 0;
}

tTag cRenderer::SetRenderLayer(tTag layerTag)
{
    tTag prevRenderLayer = mRenderLayer;

    mRenderLayer = layerTag;

    return prevRenderLayer;
}

void cRenderer::AddRenderJob(tTag jobGroupTag, tTag layerTag)
{
    mRenderJobs.insert( { jobGroupTag, layerTag } );
}

void cRenderer::SetRenderFlags(const cObjectValue* flagsV)
{
    if (!flagsV)
        return;
    
    for (int i = 0, n = flagsV->NumMembers(); i < n; i++)
    {
        tTag tag = flagsV->MemberTag(i);

        auto it = mRenderFlagTags.find(tag);

        if (it != 0)
        {
            int flagIndex = it->second;
            bool flagValue = flagsV->MemberValue(i).AsBool();

            tRenderFlags flagMask = 1 << flagIndex;

            if (flagValue)
                mRenderFlags |= flagMask;
            else
                mRenderFlags &= ~flagMask;
        }
        else
        {
            CL_LOG_E("Renderer", "Unknown flag " CL_TAG_FMT "\n", tag);
        }
    }
}


// Materials

void cRenderer::LoadMaterials(const cObjectValue* config)
{
    GL_CHECK;

    if (!config)
        return;

    cFileSpec baseSpec;

    // Have to do a bit of a song and dance here to get directory of owning member...
    cObjectChild vsChild = config->Child("shaderPrefix");
    cObjectChild fsChild = vsChild;

    if (!vsChild.Exists())
        vsChild = config->Child("vertexShaderPrefix");
    if (!fsChild.Exists())
        fsChild = config->Child("fragmentShaderPrefix");

    const char* vsPrefix = vsChild.Value().AsString();

    if (vsPrefix)
    {
        FindSpec(&baseSpec, vsChild);
        baseSpec.SetRelativePath(vsPrefix);

        if (!baseSpec.HasExtension())
            baseSpec.SetExtension("vsh");

        CL_LOG_D("Renderer", "vs prefix: %s\n", baseSpec.Path());
    }

    const char* fsPrefix = fsChild.Value().AsString();

    if (fsPrefix)
    {
        FindSpec(&baseSpec, fsChild);
        baseSpec.SetRelativePath(fsPrefix);

        if (!baseSpec.HasExtension())
            baseSpec.SetExtension("fsh");

        CL_LOG_D("Renderer", "fs prefix: %s\n", baseSpec.Path());
    }

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        CL_LOG("Renderer", "\nAdding material " CL_TAG_FMT "\n", tag);

        const char* vsName = info->Member("vertexShader").AsString();
        const char* fsName = info->Member("fragmentShader").AsString();

        if (!vsName || !fsName)
            continue;

        FindSpec(&baseSpec, c);

        cFileSpec vsSpec;
        cFileSpec fsSpec;

        vsSpec.SetDirectory(baseSpec.Directory());
        vsSpec.SetRelativePath(vsName);
        fsSpec.SetDirectory(baseSpec.Directory());
        fsSpec.SetRelativePath(fsName);

        if (!vsSpec.HasExtension())
            vsSpec.SetExtension("vsh");
        if (!fsSpec.HasExtension())
            fsSpec.SetExtension("fsh");

        auto it = mMaterialTagToIndex.find(tag);

        if (it == mMaterialTagToIndex.end())
        {
            it = mMaterialTagToIndex.insert( { tag, mMaterials.size() } ).first;
            mMaterials.push_back();
        }

        cMaterial& material = mMaterials[it->second];

        if (material.mShaderProgram == 0)
        {
            material.mShaderProgram = glCreateProgram();
            GL_DEBUG_LABEL(GL_PROGRAM_OBJECT_EXT, material.mShaderProgram, tag);
        }

        GLuint shaderProgram = LoadShaders(vsSpec.Path(), fsSpec.Path(), material.mShaderProgram, tag);

        if (shaderProgram == 0)
        {
            GL_CHECK;
            material.mIsValid = false;
            continue;
        }

        const cEnumInfo* textureKindsEnum = TextureKindsEnum();

        while (textureKindsEnum->mName)
        {
            const cValue& textureV = info->Member(textureKindsEnum->mName);

            tTag mapTag = textureV.AsTag();

            if (mapTag)
            {
                tTextureRef ref = TextureRefFromTag(mapTag);

                if (ref)
                    material.mTextures[textureKindsEnum->mValue] = ref;
                else
                    CL_LOG_E("Renderer", "No such %s texture '%s' in material '" CL_TAG_FMT "'\n", textureKindsEnum->mName, textureV.AsString(), tag);
            }

            textureKindsEnum++;
        }

        material.mShaderDataBindings.clear();
        GetBindingsFromProgram(shaderProgram, &material.mShaderDataBindings);

        material.mTextureBindings.clear();
        GetBindingsFromProgram(shaderProgram, &material.mTextureBindings);

        material.mRenderStates.clear();
        AddRenderStates(info, &material.mRenderStates);

        material.mIsValid = true;

        // Now set up hotload
        material.mVSRef = mDocumentsWatcher.AddFile(vsSpec.Path());
        mRefToMaterialIndexMap.insert( { material.mVSRef, it->second } );

        material.mFSRef = mDocumentsWatcher.AddFile(fsSpec.Path());
        mRefToMaterialIndexMap.insert( { material.mFSRef, it->second } );

        GL_CHECK;
    }
}

int cRenderer::MaterialRefFromTag(tTag materialTag)
{
    const auto it = mMaterialTagToIndex.find(materialTag);

    if (it != mMaterialTagToIndex.end())
        return it->second;

    return 0;
}

bool cRenderer::SetMaterial(int ref)
{
    if (ref >= 0 && ref < mMaterials.size())
    {
        const cMaterial& material = mMaterials[ref];

        if (!material.mIsValid)
            return false;

        glUseProgram(material.mShaderProgram);
        GL_CHECK_DETAIL;

        if (!material.mRenderStates.empty())
        {
            mRenderState.ApplyRenderState(material.mRenderStates.size(), material.mRenderStates.data());
            GL_CHECK_DETAIL;
        }

        for (int i = 0; i < kMaxTextureKinds; i++)
            if (material.mTextures[i])
            {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, material.mTextures[i]);
                GL_CHECK_DETAIL;
            }

        // We delay dealing with shader data until just before dispatch (which should call SetStateForDispatch)
        // so as to allow shader data to be set after SetMaterial() but before the draw calls.
        mCurrentMaterial = ref;

        GL_CHECK;

        return true;
    }

    return false;
}

// Textures

void cRenderer::LoadTextures(const cObjectValue* config)
{
    if (!config)
        return;

    GL_CHECK;

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        CL_LOG("Renderer", "Adding texture %s\n", name);

        GLuint textureName = 0;
        Vec2i textureSize = vl_1;
        cRGBA32 fillColour;
        const void* data = 0;

        auto it = mTextureTagToIndex.find(tag);

        if (it != mTextureTagToIndex.end())
            textureName = it->second;
        else
        {
            glGenTextures(1, &textureName);

            if (textureName >= mTextureInfo.size())
                mTextureInfo.resize(textureName + 1);
        }

        glBindTexture(GL_TEXTURE_2D, textureName);
        GL_DEBUG_LABEL(GL_TEXTURE, textureName, name);

        tBufferFormat format = tBufferFormat(AsEnum(info->Member("type"), kBufferFormatEnum, kFormatRGBA));

        int numChannels = FormatNumChannels(format);

        cImage32 image32;
        cImage8 image8;
        const char* texturePath = info->Member("image").AsString();

        if (texturePath)
        {
            cFileSpec textureSpec;

            FindSpec(&textureSpec, c, texturePath);

            if (!textureSpec.HasExtension())
                textureSpec.SetExtension("png");

            if (eqi(textureSpec.Extension(), "pvr"))
            {

            }
            else if (numChannels == 4)
            {
                if (LoadImage(textureSpec, &image32))
                {
                    CL_LOG("Renderer", "  read %s, %d x %d\n", textureSpec.Path(), image32.mW, image32.mH);
                    
                    data = image32.mData;
                    textureSize = { image32.mW, image32.mH };
                }
                else
                    CL_LOG_E("Renderer", "  failed to load %s\n", textureSpec.Path());
            }
            else if (numChannels == 1)
            {
                if (LoadImage(textureSpec, &image8))
                {
                    CL_LOG("Renderer", "  read %s, %d x %d\n", textureSpec.Path(), image8.mW, image8.mH);

                    data = image8.mData;
                    textureSize = { image8.mW, image8.mH };
                }
                else
                    CL_LOG("Renderer", "Failed to load %s\n", textureSpec.Path());
            }
        }

        const cObjectValue* proxyV;
        if (!data && (proxyV = info->Member("proxy").AsObject()))
        {
            textureSize[0] = proxyV->Member("w").AsInt(1);
            textureSize[1] = proxyV->Member("h").AsInt(1);

            fillColour = ColourAlphaToRGBA32(AsVec4(proxyV->Member("fill"), kColourRedA1));
            data = &fillColour.mAsUInt32;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    #ifdef CL_GLES
        glTexImage2D(GL_TEXTURE_2D, 0, kGLGenericFormat[format], textureSize[0], textureSize[1], 0, kGLGenericFormat[format], kGLType[format], data);
    #else
        glTexImage2D(GL_TEXTURE_2D, 0, kGLInternalFormat[format], textureSize[0], textureSize[1], 0, kGLGenericFormat[format], kGLType[format], data);
    #endif
        GL_CHECK;

        // TODO: should we nuke textureName if we didn't load anything here? or just accept setting size to 0?

        if (!textureName)
        {
            GL_CHECK;
            continue;
        }

        bool isPow2 = IsPowerOfTwo(textureSize[0]) && IsPowerOfTwo(textureSize[1]);
        bool needsMips = false;

    #ifdef CL_GLES
        ConfigTextureParams(info, GL_TEXTURE_2D, kClampOn, isPow2 ? kMIPOn : kMIPOff, &needsMips);
    #else
        ConfigTextureParams(info, GL_TEXTURE_2D, kClampOn, kMIPOn, &needsMips);
    #endif

        if (needsMips
    #ifdef CL_GLES
            && isPow2
    #endif
        )
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            GL_CHECK;
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        mTextureTagToIndex[tag] = textureName;
        mTextureInfo[textureName].mSize = textureSize;

        GL_CHECK;
    }
}

tTextureRef cRenderer::CreateTexture(tTag tag, tBufferFormat format, int w, int h, const uint8_t* data, const cObjectValue* config)
{
    CL_ASSERT(w > 0 && h > 0);

    GLuint textureName;

    glGenTextures(1, &textureName);
    glBindTexture(GL_TEXTURE_2D, textureName);
#if CL_TAG_DEBUG
    GL_DEBUG_LABEL(GL_TEXTURE, textureName, tag);
#else
    GL_DEBUG_LABEL(GL_TEXTURE, textureName, "code created");
#endif

    bool requiresMIPs = false;

    ConfigTextureParams
    (
        config,
        GL_TEXTURE_2D,
        kClampOn,
        kMIPOff,
        &requiresMIPs
    );

#ifdef CL_GLES
    glTexImage2D(GL_TEXTURE_2D, 0, kGLGenericFormat[format], w, h, 0, kGLGenericFormat[format], kGLType[format], data);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, kGLInternalFormat[format], w, h, 0, kGLGenericFormat[format], kGLType[format], data);
#endif

    GL_CHECK;

    glBindTexture(GL_TEXTURE_2D, 0);

    if (textureName >= mTextureInfo.size())
        mTextureInfo.resize(textureName + 1);

    mTextureInfo[textureName].mSize = { w, h };

    mTextureTagToIndex[tag] = textureName;

    return textureName;
}

bool cRenderer::DestroyTexture(tTextureRef ref)
{
    if (ref > 0)
    {
        GLuint textureName = ref;
        glDeleteTextures(1, &textureName);
        return true;
    }

    return false;
}


int cRenderer::TextureRefFromTag(tTag textureTag)
{
    auto it = mTextureTagToIndex.find(textureTag);

    if (it != mTextureTagToIndex.end())
        return it->second;

    return 0;
}

void cRenderer::SetTextures(const cObjectValue* object)
{
    if (!object)
        return;

    for (auto c : object->Children())
    {
        tTag kindTag    = c.Tag();
        tTag textureTag = c.Value().AsTag();

        int ref = TextureRefFromTag(textureTag);

        if (ref > 0)
            SetTexture(kindTag, ref);
    }
}

void cRenderer::SetTexture(tTextureKind kind, int textureIndex)
{
    CL_ASSERT(textureIndex >= 0 && textureIndex < 4096);    // No longer expecting ID here.

    // Currently the texture index is just the direct GL texture 'name'.
    if (textureIndex >= 0)
    {
        glActiveTexture(GL_TEXTURE0 + kind);
        glBindTexture(GL_TEXTURE_2D, textureIndex);
    }
}

void cRenderer::SetTexture(tTag kind, int textureIndex)
{
    mKindTagToTextureIndex[kind] = textureIndex;
}

void cRenderer::UpdateTexture(tTextureRef ref, tBufferFormat format, const void* data)
{
    if (!IsValid(ref))
        return;

    glBindTexture(GL_TEXTURE_2D, ref);
    GL_CHECK;

    Vec2i wh = mTextureInfo[ref].mSize;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, wh[0], wh[1], kGLGenericFormat[format], kGLType[format], data);
    GL_CHECK;
}

// ShaderData

void cRenderer::LoadShaderData(const cObjectValue* config)
{
    if (!config)
        return;

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        tShaderDataRef ref = AddShaderData(tag);

        GLenum dataType = AsEnum(info->Member("type"), kShaderDataTypeEnum, GL_NONE);
        tShaderDataConfigFunc* configFunc = ConfigFuncForType(dataType);

        if (configFunc)
        {
            mShaderData[ref].mConfigFunc = configFunc;

            const cValue& value = info->Member("value");

            if (!value.IsNull())
                configFunc(this, ref, value);
        }
    }
}

void cRenderer::SetShaderData(const cObjectValue* object)
{
    if (!object)
        return;

    for (auto c : object->Children())
    {
        tShaderDataRef ref = ShaderDataRefFromTag(c.Tag());

        if (!IsValid(ref) || !mShaderData[ref].mConfigFunc)
            continue;

        mShaderData[ref].mConfigFunc(this, ref, c.Value());
    }
}

bool cRenderer::SetShaderData(tShaderDataRef ref, size_t dataSize, const void* data)
{
    if (!IsValid(ref))
        return false;

    CL_INDEX(ref, mShaderData.size());
    CL_ASSERT((dataSize & 3) == 0); // require uint32_t multiple

    cShaderDataInfo& info = mShaderData[ref];

    // TODO: save info if stack active, and clear mOffset, so we automatically allocate
    // new space.

    if (info.mOffset == kNullDataOffset || info.mSize != dataSize)
    {
        info.mOffset = mShaderDataStore.Allocate(dataSize, 4);
        info.mSize = dataSize;
    }

    void* dataDest = mShaderDataStore.Data(info.mOffset);
    memcpy(dataDest, data, dataSize);

    for (auto depDataID : info.mUpdateDependents)
    {
        CL_INDEX(depDataID, mShaderData.size());

        if (mShaderData[depDataID].mUpdateFunc)
            mShaderData[depDataID].mUpdateFunc(this, ShaderDataSize(depDataID), ShaderData(depDataID));
    }

    return true;
}

tShaderDataRef cRenderer::ShaderDataRefFromTag(tTag tag) const
{
    auto const it = mShaderDataTagToIndex.find(tag);

    if (it != mShaderDataTagToIndex.end())
        return it->second;

    return kNullDataRef;
}

tShaderDataRef cRenderer::AddShaderData(tTag tag)
{
    auto const it = mShaderDataTagToIndex.find(tag);

    if (it != mShaderDataTagToIndex.end())
        return it->second;

    tShaderDataRef newRef = mShaderData.size();
    mShaderData.push_back();

    mShaderDataTagToIndex.insert(it, { tag, newRef } );

    return newRef;
}

void cRenderer::SetShaderDataUpdate(tShaderDataRef ref, tShaderDataUpdateFunc updateFunc, int numDeps, const tShaderDataRef deps[])
{
    CL_INDEX(ref, mShaderData.size());

    cShaderDataInfo& info = mShaderData[ref];

    info.mUpdateFunc = updateFunc;

    if (info.mUpdateFunc)
        info.mUpdateFunc(this, ShaderDataSize(ref), ShaderData(ref));

    for (int i = 0; i < numDeps; i++)
    {
        CL_INDEX(deps[i], mShaderData.size());

        mShaderData[deps[i]].mUpdateDependents.insert(ref);
    }
}

void cRenderer::SetShaderDataConfig(tShaderDataRef ref, tShaderDataConfigFunc f)
{
    CL_INDEX(ref, mShaderData.size());

    cShaderDataInfo& info = mShaderData[ref];

    info.mConfigFunc = f;
}

// Copy-back buffers

const cAllocImage32* cRenderer::CopyBackBuffer(tTag tag, uint32_t* updateCount)
{
    CL_ASSERT(IsTag(tag));

    auto it = mCopyBackBufferTagToIndex.find(tag);

    if (it == mCopyBackBufferTagToIndex.end())
        return 0;

    auto const& bi = mCopyBackBuffers[it->second];

    if (*updateCount >= bi.mUpdateCount)
        return 0;

    *updateCount = bi.mUpdateCount;
    return bi.mImage;
}

uint32_t cRenderer::UpdateCountForCopyBackBuffer(tTag tag)
{
    CL_ASSERT(IsTag(tag));

    auto it = mCopyBackBufferTagToIndex.find(tag);

    if (it == mCopyBackBufferTagToIndex.end())
        return 0;

    return mCopyBackBuffers[it->second].mUpdateCount;
}

// Quad rendering

int cRenderer::CreateQuadMesh(int numQuads, int numElts, cEltInfo elts[])
{
    int slot = mQuadMeshSlots.CreateSlot();
    cQuadMesh* qm = &mQuadMeshSlots[slot];

    CL_ASSERT(qm->mMesh == 0);
    CL_ASSERT(qm->mVB   == 0);

    glGenVertexArrays(1, &qm->mMesh);
    glBindVertexArray(qm->mMesh);
    GL_DEBUG_LABEL(GL_VERTEX_ARRAY_OBJECT_EXT, qm->mMesh, "quadMesh");

    int vertSize = 0;
    for (int i = 0; i < numElts; i++)
        vertSize += elts[i].mDataSize;

    // Create vertex buffer
    glGenBuffers(1, &qm->mVB);
    glBindBuffer(GL_ARRAY_BUFFER, qm->mVB);
    GL_DEBUG_LABEL(GL_BUFFER_OBJECT_EXT, qm->mVB, "quadVB");

    qm->mNumQuads   = numQuads;
    qm->mVertexSize = vertSize;
    qm->mBufferSize = vertSize * numQuads * 4;
    qm->mVBCursor     = 0;

    // Allocate and load position data into the VB
    glBufferData(GL_ARRAY_BUFFER, qm->mBufferSize, 0, QUAD_USAGE);

    int eltOffset = 0;

    for (int i = 0; i < numElts; i++)
    {
        glEnableVertexAttribArray(elts[i].mVAType);

        glVertexAttribPointer
        (
            elts[i].mVAType,
            elts[i].mNumCmpts,
            elts[i].mCmptType,
            elts[i].mNormalised,
            vertSize,
            (const GLvoid*) eltOffset
        );

        eltOffset += elts[i].mDataSize;
    }
    
    GLuint quadIB;
    glGenBuffers(1, &quadIB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIB);
    GL_DEBUG_LABEL(GL_BUFFER_OBJECT_EXT, quadIB, "quadIB");

    // TODO: share a single IB between all such meshes
    int16_t quadIndices[6 * numQuads];
    int16_t* p = quadIndices;
    
    for (int i = 0; i < numQuads * 4; i += 4)
    {
        // 2 tris per quad
        p[0] = i;
        p[1] = i + 1;
        p[2] = i + 2;
        
        p[3] = i;
        p[4] = i + 2;
        p[5] = i + 3;
        
        p += 6;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GL_CHECK;

    if (qm->mMesh != 0 && qm->mVB != 0)
        return slot;

    mQuadMeshSlots.DestroySlot(slot);
    return 0;
}

void cRenderer::DestroyQuadMesh(int quadMesh)
{
    if (quadMesh < 0)
        return;

    cQuadMesh* qm = &mQuadMeshSlots[quadMesh];

    if (qm->mMesh)
    {
        DestroyMesh(qm->mMesh);
        qm->mMesh = 0;
        qm->mVB = 0;    // Taken care of by DestroyMesh.
        qm->mVertexSize = 0;
    }

    mQuadMeshSlots.DestroySlot(quadMesh);
}


namespace
{
    void DrawElements(int numVertices, int startIndex, int numIndices)
    {
    #ifdef CL_GLES
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, (const void*) (startIndex * sizeof(uint16_t)));
    #else
        glDrawRangeElements(GL_TRIANGLES, 0, numVertices, numVertices, GL_UNSIGNED_SHORT, (const void*) (startIndex * sizeof(uint16_t)));
    #endif
    }

    void DrawQuads(int numQuads, int startIndex)
    {
    #ifdef CL_GLES
        glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_SHORT, (const void*) (startIndex * sizeof(uint16_t)));
    #else
        glDrawRangeElements(GL_TRIANGLES, 0, numQuads * 4, numQuads * 6, GL_UNSIGNED_SHORT, (const void*) (startIndex * sizeof(uint16_t)));
    #endif
    }
}

namespace
{
    uint32_t sBufferSpace[1024 * 24];

    int GetBuffer(const cQuadMesh& qm, int count, uint8_t** buffer)
    {
        *buffer = (uint8_t*) sBufferSpace;

        int maxCount = sizeof(sBufferSpace) / (4 * qm.mVertexSize);

        if (count > maxCount)
            count = maxCount;

        return count;
    }

    void ReleaseBuffer(cQuadMesh& qm, int numQuads, cRenderer* renderer)
    {
        if (numQuads <= 0)
            return;

        renderer->SetStateForDispatch();

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

        size_t bufferSize = numQuads * 4 * qm.mVertexSize;

    #if defined(GL_USE_ORPHANING)
        glBufferData(GL_ARRAY_BUFFER, qm.mBufferSize, 0, QUAD_USAGE);
    #endif

        glBufferSubData(GL_ARRAY_BUFFER, qm.mVBCursor, bufferSize, sBufferSpace);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(qm.mMesh);
        DrawQuads(numQuads, qm.mIndexStart);

        GL_CHECK;
        glBindVertexArray(0);
    }

    void ReleaseMultiBuffer(cQuadMesh& qm, int numQuads, cRenderer* renderer)
    {
        if (numQuads <= 0)
            return;

        renderer->SetStateForDispatch();

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

        size_t bufferSize = numQuads * 4 * qm.mVertexSize;

        if (bufferSize + qm.mVBCursor > qm.mBufferSize)
        {
            qm.mVBCursor = 0;
            qm.mIndexStart = 0;

        #ifdef GL_USE_ORPHANING
            glBufferData(GL_ARRAY_BUFFER, qm.mBufferSize, 0, QUAD_USAGE);
        #endif
        }

        glBufferSubData(GL_ARRAY_BUFFER, qm.mVBCursor, bufferSize, sBufferSpace);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(qm.mMesh);
        DrawQuads(numQuads, qm.mIndexStart);

        GL_CHECK;
        glBindVertexArray(0);

        qm.mVBCursor += bufferSize;
        qm.mIndexStart += numQuads * 6;
    }
}

namespace
{
    // glMap variants
    int GetBufferMap(cQuadMesh& qm, int count, uint8_t** buffer)
    {
        int maxCount = qm.mNumQuads;

        if (count > maxCount)
            count = maxCount;

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

    #ifdef GL_USE_ORPHANING
        glBufferData(GL_ARRAY_BUFFER, qm.mBufferSize, 0, QUAD_USAGE);
    #endif

        *buffer = (uint8_t*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        GL_CHECK;
        return count;
    }

    void ReleaseBufferMap(cQuadMesh& qm, int numQuads, cRenderer* renderer)
    {
        bool success = glUnmapBuffer(GL_ARRAY_BUFFER);
        CL_ASSERT(success);
        GL_CHECK;
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (numQuads <= 0)
            return;

        renderer->SetStateForDispatch();

        glBindVertexArray(qm.mMesh);
        DrawQuads(numQuads, qm.mIndexStart);

        glBindVertexArray(0);
        GL_CHECK;
    }

    // multi-buffer variants
    int GetMultiBufferMap(cQuadMesh& qm, int count, uint8_t** buffer)
    {
        int maxCount = qm.mNumQuads;

        if (count > maxCount)
            count = maxCount;

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

        size_t bufferSize = count * 4 * qm.mVertexSize;

        if (bufferSize + qm.mVBCursor > qm.mBufferSize)
        {
            qm.mVBCursor = 0;
            qm.mIndexStart = 0;

        #ifdef GL_USE_ORPHANING
            glBufferData(GL_ARRAY_BUFFER, qm.mBufferSize, 0, QUAD_USAGE);
        #endif

        }

        *buffer = (uint8_t*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        *buffer += qm.mVBCursor;

        GL_CHECK;
        return count;
    }

    void ReleaseMultiBufferMap(cQuadMesh& qm, int numQuads, cRenderer* renderer)
    {
        bool success = glUnmapBuffer(GL_ARRAY_BUFFER);
        CL_ASSERT(success);
        GL_CHECK;
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (numQuads <= 0)
            return;

        renderer->SetStateForDispatch();

        glBindVertexArray(qm.mMesh);
        DrawQuads(numQuads, qm.mIndexStart);

        glBindVertexArray(0);
        GL_CHECK;

        qm.mVBCursor += numQuads * 4 * qm.mVertexSize;
        qm.mIndexStart += numQuads * 6;
    }


#if GL_EXT_map_buffer_range
    // glMapRange variants
    int GetBufferMapRange(cQuadMesh& qm, int count, uint8_t** buffer)
    {
        int maxCount = qm.mNumQuads;

        if (count > maxCount)
            count = maxCount;

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

        size_t bufferSize = count * 4 * qm.mVertexSize;
        GLbitfield accessFlags = GL_MAP_WRITE_BIT_EXT | GL_MAP_UNSYNCHRONIZED_BIT_EXT | GL_MAP_INVALIDATE_RANGE_BIT_EXT;

        *buffer = (uint8_t*) glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, accessFlags);

        GL_CHECK;
        return count;
    }

    int GetMultiBufferMapRange(cQuadMesh& qm, int count, uint8_t** buffer)
    {
        int maxCount = qm.mNumQuads;

        if (count > maxCount)
            count = maxCount;

        glBindBuffer(GL_ARRAY_BUFFER, qm.mVB);

        // GL_MAP_UNSYNCHRONIZED_BIT_EXT avoids sync on previous draws.
        // GL_MAP_INVALIDATE_BUFFER_BIT_EXT lets driver know it can orphan the buffer.
        // for some reason GL_MAP_INVALIDATE_BUFFER_BIT_EXT causes sync on iOS, using GL_MAP_INVALIDATE_RANGE_BIT_EXT instead
        // gets the desired behaviour.

        GLbitfield accessFlags = GL_MAP_WRITE_BIT_EXT | GL_MAP_UNSYNCHRONIZED_BIT_EXT;

        size_t bufferSize = count * 4 * qm.mVertexSize;

        if (bufferSize + qm.mVBCursor > qm.mBufferSize)
        {
            qm.mVBCursor = 0;
            qm.mIndexStart = 0;
            accessFlags |= GL_MAP_INVALIDATE_BUFFER_BIT_EXT;
        }
        else
            accessFlags |= GL_MAP_INVALIDATE_RANGE_BIT_EXT;

        *buffer = (uint8_t*) glMapBufferRange(GL_ARRAY_BUFFER, qm.mVBCursor, bufferSize, accessFlags);

        GL_CHECK;
        return count;
    }
#endif
}

int cRenderer::GetQuadBuffer(int quadMesh, int count, uint8_t** buffer)
{
    CL_ASSERT(buffer);

#if defined(GL_USE_MAP_RANGE)

    #ifdef GL_MULTI_BUFFER
        return GetMultiBufferMapRange(mQuadMeshSlots[quadMesh], count, buffer);
    #else
        return GetBufferMapRange(mQuadMeshSlots[quadMesh], count, buffer);
    #endif

#elif defined(GL_USE_MAP)

    #ifdef GL_MULTI_BUFFER
        return GetMultiBufferMap(mQuadMeshSlots[quadMesh], count, buffer);
    #else
        return GetBufferMap(mQuadMeshSlots[quadMesh], count, buffer);
    #endif

#else

    return GetBuffer(mQuadMeshSlots[quadMesh], count, buffer);

#endif
}

void cRenderer::DispatchAndReleaseBuffer(int quadMesh, int numQuads)
{
#if defined(GL_USE_MAP) || defined(GL_USE_MAP_RANGE)

    #ifdef GL_MULTI_BUFFER
        ReleaseMultiBufferMap(mQuadMeshSlots[quadMesh], numQuads, this);
    #else
        ReleaseBufferMap(mQuadMeshSlots[quadMesh], numQuads, this);
    #endif

#else

    #ifdef GL_MULTI_BUFFER
        ReleaseMultiBuffer(mQuadMeshSlots[quadMesh], numQuads, this);
    #else
        ReleaseBuffer(mQuadMeshSlots[quadMesh], numQuads, this);
    #endif

#endif
}


void cRenderer::DrawBuffer(uint32_t mode, int numElts, cEltInfo elts[], int count, const void* buffer)
{
    SetStateForDispatch();

    int vertSize = 0;
    for (int i = 0; i < numElts; i++)
        vertSize += elts[i].mDataSize;

    int eltOffset = 0;

    for (int i = 0; i < numElts; i++)
    {
        glEnableVertexAttribArray(elts[i].mVAType);

        glVertexAttribPointer
        (
            elts[i].mVAType,
            elts[i].mNumCmpts,
            elts[i].mCmptType,
            elts[i].mNormalised,
            vertSize,
            buffer
        );

        (uint8_t*&) buffer += elts[i].mDataSize;
    }
    
    glDrawArrays(mode, 0, count);

    for (int i = 0; i < numElts; i++)
        glDisableVertexAttribArray(elts[i].mVAType);

    GL_CHECK;
}

void cRenderer::DrawMesh(const cGLMeshInfo* meshInfo)
{
    SetStateForDispatch();
    DispatchMesh(meshInfo);
}

// cRenderer

void cRenderer::LoadRenderBuffers(const cObjectValue* config)
{
    if (!config)
        return;

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        CL_LOG("Renderer", "Adding render buffer %s\n", name);

        tBufferFormat bufferFormat = (tBufferFormat) AsEnum(info->Member("format"), kBufferFormatEnum, kMaxBufferFormats);

        if (bufferFormat == kMaxBufferFormats)
        {
            CL_LOG_E("Renderer", "Unknown format %s\n", info->Member("format").AsString("unknown"));
            continue;
        }

        bool isTexture = !IsDepthOrStencilFormat(bufferFormat);
        isTexture = info->Member("texture").AsBool(isTexture);

        int width  = info->Member("width").AsInt();
        int height = info->Member("height").AsInt();

        float fraction = info->Member("fraction").AsFloat(0.0f);

        if (fraction > 0.0f && !mFrameBufferInfo.empty())
        {
            Vec2i size = mFrameBufferInfo[mDefaultFrameBuffer].mSize;

            width  = CeilToSInt32(size[0] * fraction);
            height = CeilToSInt32(size[1] * fraction);
        }

        if (isTexture)
        {
            GLuint textureName = 0;

            auto it = mTextureTagToIndex.find(tag);
            if (it != mTextureTagToIndex.end())
                textureName = it->second;
            else
            {
                glGenTextures(1, &textureName);
                mTextureTagToIndex[tag] = textureName;

                if (textureName >= mTextureInfo.size())
                    mTextureInfo.resize(textureName + 1);
            }

            glBindTexture(GL_TEXTURE_2D, textureName);
            GL_DEBUG_LABEL(GL_TEXTURE, textureName, name);

            // Set up filter and wrap modes for this texture object
            ConfigTextureParams(info, GL_TEXTURE_2D, kClampOn, kMIPOff);

            // Allocate a texture image with which we can render to
            // Pass NULL for the data parameter since we don't need to load image data.
            // We will be generating the image by rendering to this texture

        #if 0 // GL_EXT_texture_storage
            // The trouble with this is that it defeats hotloading! Can't change format once this has been called.
            glTexStorage2DEXT(GL_TEXTURE_2D, 1, kGLInternalFormat[bufferFormat], width, height);
        #else

        #ifdef CL_GLES
            // Bloody GLES changed the syntactic meaning of the parameters to this call, without changing
            // the call itself. internalFormat and format are now expected to be the same, and purely
            // symbolic with no type info, the type info is carried in the type, which can be:
            //  GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1
            glTexImage2D(GL_TEXTURE_2D, 0, kGLGenericFormat[bufferFormat], width, height, 0, kGLGenericFormat[bufferFormat], kGLType[bufferFormat], NULL);
        #else
            glTexImage2D(GL_TEXTURE_2D, 0, kGLInternalFormat[bufferFormat], width, height, 0, kGLGenericFormat[bufferFormat], kGLType[bufferFormat], NULL);
        #endif

        #endif

            GL_CHECK;

            mTextureInfo[textureName].mSize = { width, height };
            glBindTexture(GL_TEXTURE_2D, 0);

            CL_LOG("Renderer", "  added as texture\n");
        }
        else
        {
            GLuint renderBufferName = 0;

            auto it = mRenderBufferTagToIndex.find(tag);
            if (it != mRenderBufferTagToIndex.end())
                renderBufferName = it->second;
            else
            {
                glGenRenderbuffers(1, &renderBufferName);

                mRenderBufferTagToIndex[tag] = renderBufferName;

                if (renderBufferName >= mRenderBufferInfo.size())
                    mRenderBufferInfo.resize(renderBufferName + 1);
            }

            glBindRenderbuffer(GL_RENDERBUFFER, renderBufferName);
            GL_DEBUG_LABEL(GL_RENDERBUFFER, renderBufferName, name);

            glRenderbufferStorage(GL_RENDERBUFFER, kGLInternalFormat[bufferFormat], width, height);
            GL_CHECK;

            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            mRenderBufferInfo[renderBufferName].mSize = { width, height };

            CL_LOG("Renderer", "  added as buffer\n");

//        glRenderbufferStorageMultisample(GL_RENDERBUFFER, multiSample, GL_RGBA8, width, height);
//        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
        }
    }
}

void cRenderer::LoadFrameBuffers(const cObjectValue* config)
{
    if (!config)
        return;

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        GLuint fboName;
        Vec2i fbSize(0);

        auto it = mFrameBufferTagToIndex.find(tag);
        if (it != mFrameBufferTagToIndex.end())
            fboName = it->second;
        else
            glGenFramebuffers(1, &fboName);

        glBindFramebuffer(GL_FRAMEBUFFER, fboName);
        GL_DEBUG_LABEL(GL_FRAMEBUFFER, fboName, name);

        // for each buffer...
        for (int ia = 0; ia < CL_SIZE(kFrameBufferAttachmentEnum) - 1; ia++)
        {
            GLuint attachmentType = kFrameBufferAttachmentEnum[ia].mValue;

            // clear any previous attachment
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, 0);

            tTag attachmentTag = TagFromString(kFrameBufferAttachmentEnum[ia].mName);
            tTag tag = info->Member(attachmentTag).AsTag();

            if (tag != 0)
            {
                auto it = mRenderBufferTagToIndex.find(tag);

                if (it != mRenderBufferTagToIndex.end())
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, it->second);
                    GL_CHECK;

                    if (fbSize[0] == 0)
                        fbSize = mRenderBufferInfo[it->second].mSize;
                }
                else
                {
                    it = mTextureTagToIndex.find(tag);

                    if (it != mTextureTagToIndex.end())
                    {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, it->second, 0);
                        GL_CHECK;

                        if (fbSize[0] == 0)
                            fbSize = mTextureInfo[it->second].mSize;
                    }
                    else
                        CL_LOG_E("Renderer", "%s: No such renderbuffer: %s\n", attachmentTag, info->Member(attachmentTag).AsString("null"));
                }
            }
        }

        GLenum fbError = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (fbError == GL_FRAMEBUFFER_COMPLETE)
        {
            mFrameBufferTagToIndex[tag] = fboName;

            if (fboName >= mFrameBufferInfo.size())
                mFrameBufferInfo.resize(fboName + 1);

            mFrameBufferInfo[fboName].mSize = fbSize;
            mFrameBufferInfo[fboName].mDeviceOriented = info->Member("deviceOrient").AsBool(false);

            const cValue* v;

            if (MemberExists(info, "clear", &v))
            {
                Vec4f cc = AsVec4(*v);

                glClearColor(cc[0], cc[1], cc[2], cc[3]);
                GLuint flags = 0;

                if (info->Member("colour").AsBool(true))
                    flags |= GL_COLOR_BUFFER_BIT;
                if (info->Member("depth").AsBool(true))
                    flags |= GL_DEPTH_BUFFER_BIT;
                if (info->Member("stencil").AsBool(false))
                    flags |= GL_STENCIL_BUFFER_BIT;

                glClear(flags);
            }

            CL_LOG("Renderer", "Adding frame buffer %s\n", name);
        }
        else
        {
            glDeleteFramebuffers(1, &fboName);
            mFrameBufferTagToIndex.erase(tag);
            CL_LOG_E("Renderer", "Error adding frame buffer: %s\n", FrameBufferError(fbError));
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void cRenderer::LoadLayers(const cObjectValue* config)
{
    if (!config)
        return;

    // TODO mDataLayers.clear();

    for (auto c : config->Children())
    {
        const cValue&       info = c.Value();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (MemberIsHidden(name))
            continue;

        cDataLayer* newLayer = new(mAllocator) cDataLayer;

        newLayer->Config(info, this);

        mDataLayers[tag].mLayer = newLayer;
    }
}

int cRenderer::RenderFlagFromTag(nCL::tTag tag) const
{
    auto it = mRenderFlagTags.find(tag);

    if (it != 0)
        return it->second;

    return -1;
}


namespace
{
    cEltInfo kDrawRectFormat[] =
    {
        { kVBPositions, 2, GL_FLOAT,         8, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };

    struct cDrawRectVertex
    {
        Vec2f    mCoord;
        float    mUV[2];
        cRGBA32  mColour;
    };
}

void cRenderer::ApplyCommand(const cRenderCommand& command, const cRenderLayerState& state)
{
    switch (command.mType)
    {
    case kRCDrawLayer:
        {
            DispatchLayer(command.mSet.mTag, state, "DrawLayer");
        }
        break;
    case kRCDrawJobGroup:
        {
            DispatchJobGroup(command.mSet.mTag, state);
        }
        break;
    case kRCSetCamera:
        {
            cICamera* camera = Camera(command.mSet.mTag);

            if (camera)
                camera->Dispatch(this);

            mCurrentCamera = camera;
        }
        break;
    case kRCSetMaterial:
        {
            tMaterialRef ref = MaterialRefFromTag(command.mSet.mTag);

            if (IsValid(ref))
                SetMaterial(ref);
        }
        break;
    case kRCSetFrameBuffer:
        {
            GLuint fbName = -1;

            if (command.mSet.mTag == 0)
                fbName = mDefaultFrameBuffer;
            else
            {
                auto it = mFrameBufferTagToIndex.find(command.mSet.mTag);

                if (it != mFrameBufferTagToIndex.end())
                    fbName = it->second;
                else
                    break;
            }

            const cFrameBufferInfo& info = mFrameBufferInfo[fbName];

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbName);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbName);

            glViewport(0, 0, info.mSize[0], info.mSize[1]);

            mCurrentFrameBuffer = fbName;

            if (info.mDeviceOriented)
                SetShaderDataT(kDataIDDeviceOrient, mDeviceOrient);
            else
                SetShaderDataT(kDataIDDeviceOrient, Mat3f(vl_I));

            // Must re-dispatch the camera, because the viewport/device orientation could have changed
            if (mCurrentCamera)
                mCurrentCamera->Dispatch(this);

            GL_CHECK;
        }
        break;
    case kRCSetTexture:
        {
            if (command.mSet.mTag == 0)
                SetTexture(tTextureKind(command.mSet.mStage), 0);
            else
            {
                tTextureRef ref = TextureRefFromTag(command.mSet.mTag);

                if (IsValid(ref))
                    SetTexture(tTextureKind(command.mSet.mStage), ref);
            }
        }
        break;
    case kRCSetShaderData:
        {
            if (command.mShaderData.mRef < mShaderData.size())
            {
                tShaderDataRef id = tShaderDataRef(command.mShaderData.mRef);

                if (mShaderData[id].mConfigFunc)
                    mShaderData[id].mConfigFunc(this, id, *command.mShaderData.mConfig);
                else
                    CL_LOG_E("Renderer", "Shader data %s can't be set\n", EnumName(kShaderDataEnum, id));
            }
        }
        break;
    case kRCClear:
        {
            glClearColor(command.mClear.mC[0], command.mClear.mC[1], command.mClear.mC[2], command.mClear.mC[3]);
            GL_CHECK;
            glClear(command.mClear.mBufferFlags);
            GL_CHECK;
        }
        break;
    case kRCDrawRect:
        {
            cRGBA32 colour = command.mDrawRect.mColourAlphaU32;

            Vec4f r(command.mDrawRect.mRect);
            r = 2.0f * r - vl_1;

            float s0 = command.mDrawRect.mFlipX;
            float s1 = 1.0f - s0;
            float t0 = command.mDrawRect.mFlipY;
            float t1 = 1.0f - t0;

            Vec2f rp[4] =
            {
                { r[0], r[1] },
                { r[2], r[1] },
                { r[2], r[3] },
                { r[0], r[3] }
            };

            switch (mDeviceOrientation)
            {
            case kOrientPortraitDown:
                for (int i = 0; i < 4; i++)
                    rp[i] = -rp[i];
                break;
            case kOrientLandscapeLeft:
                for (int i = 0; i < 4; i++)
                    rp[i] = { rp[i][1], -rp[i][0] };
                break;
            case kOrientLandscapeRight:
                for (int i = 0; i < 4; i++)
                    rp[i] = { -rp[i][1], rp[i][0] };
                break;
            default:
                break;
            }

        #ifndef GL_QUADS
            cDrawRectVertex screenQuad[6] =
            {
                { rp[0], s0, t0, colour },
                { rp[1], s1, t0, colour },
                { rp[2], s1, t1, colour },

                { rp[2], s1, t1, colour },
                { rp[3], s0, t1, colour },
                { rp[0], s0, t0, colour }
            };

            DrawBuffer(GL_TRIANGLES, CL_SIZE(kDrawRectFormat), kDrawRectFormat, CL_SIZE(screenQuad), screenQuad);
        #else
            cDrawRectVertex screenQuad[4] =
            {
                { rp[0], s0, t0, colour },
                { rp[1], s1, t0, colour },
                { rp[2], s1, t1, colour },
                { rp[3], s0, t1, colour }
            };

            DrawBuffer(GL_QUADS, CL_SIZE(kDrawRectFormat), kDrawRectFormat, CL_SIZE(screenQuad), screenQuad);
        #endif
        }
        break;
    case kRCBlit:
        {
            auto it = mFrameBufferTagToIndex.find(command.mBlit.mSourceTag);

            if (it != mFrameBufferTagToIndex.end())
            {
                GLint srcFB = it->second;
                glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFB);
                GL_CHECK;

                GLint dstFB = 0;
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &dstFB);
                GL_CHECK;

                Vec2i srcSize = mFrameBufferInfo[srcFB].mSize;
                Vec2i dstSize = mFrameBufferInfo[dstFB].mSize;

                GLint srcRect[4] =
                {
                    FloorToSInt32(command.mBlit.mSrcRect[0] * srcSize[0]),
                    FloorToSInt32(command.mBlit.mSrcRect[1] * srcSize[1]),
                     CeilToSInt32(command.mBlit.mSrcRect[2] * srcSize[0]),
                     CeilToSInt32(command.mBlit.mSrcRect[3] * srcSize[1])
                };

                GLint dstRect[4] =
                {
                    FloorToSInt32(command.mBlit.mDstRect[0] * dstSize[0]),
                    FloorToSInt32(command.mBlit.mDstRect[1] * dstSize[1]),
                     CeilToSInt32(command.mBlit.mDstRect[2] * dstSize[0]),
                     CeilToSInt32(command.mBlit.mDstRect[3] * dstSize[1])
                };

            #ifndef CL_GLES
                // Sigh, GLES 3.0 only, and seems no extension in iOS 6.0.
                glBlitFramebuffer
                (
                    srcRect[0], srcRect[0], srcRect[0], srcRect[0],
                    dstRect[0], dstRect[0], dstRect[0], dstRect[0],
                    command.mBlit.mBufferFlags,
                    GL_NEAREST
                );
                GL_CHECK;
            #endif
            }
        }
        break;
    case kRCResolve:
        {
            auto it = mFrameBufferTagToIndex.find(command.mSet.mTag);

            if (it != mFrameBufferTagToIndex.end())
            {
                GLint srcFB = it->second;
                glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFB);
                GL_CHECK;

            #if GL_APPLE_framebuffer_multisample && defined(CL_IOS)
                glResolveMultisampleFramebufferAPPLE();
            #endif
                // TODO: OSX? Android? Fall back to glBlit?
            }
        }
        break;
    case kRCGenerateMips:
        {
            auto it = mTextureTagToIndex.find(command.mSet.mTag);

            if (it != mTextureTagToIndex.end())
            {
                glBindTexture(GL_TEXTURE_2D, it->second);
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
        break;

    case kRCDiscardBuffer:
        {
        #if GL_EXT_discard_framebuffer
            GLenum attachment = command.mDiscard.mAttachment;
            glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, 1, &attachment);
        #endif
        }
        break;

    case kRCCopyBack:
        {
        #ifdef CL_IOS
            GLint nativeFormat;
            GLint nativeType;

            glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &nativeFormat);
            glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &nativeType);

            CL_ASSERT(nativeFormat == GL_BGRA_EXT);
            CL_ASSERT(nativeType   == GL_UNSIGNED_BYTE);
        #endif

            const cRCCopyBackInfo& ci = command.mCopyBack;

            int cbBufferIndex;
            auto it = mCopyBackBufferTagToIndex.find(ci.mDestTag);

            // TODO: use insert
            if (it == mCopyBackBufferTagToIndex.end())
            {
                cbBufferIndex = mCopyBackBuffers.size();
                mCopyBackBufferTagToIndex[ci.mDestTag] = cbBufferIndex;
                mCopyBackBuffers.push_back();
            }
            else
                cbBufferIndex = it->second;

            cCopyBackBufferInfo& bi = mCopyBackBuffers[cbBufferIndex];

            int wh[2] = { ci.mWH[0], ci.mWH[1] };

            if (wh[0] == 0)
                wh[0] = mFrameBufferInfo[mCurrentFrameBuffer].mSize[0];
            if (wh[1] == 0)
                wh[1] = mFrameBufferInfo[mCurrentFrameBuffer].mSize[1];

            uint32_t* imageData = CreateArray<uint32_t>(mAllocator, wh[0] * wh[1]);
            bi.mImage = new(mAllocator) cAllocImage32(wh[0], wh[1], imageData, mAllocator);

            // GL_BGRA_EXT
            glReadPixels(ci.mXY[0], ci.mXY[1], wh[0], wh[1], GL_BGRA_EXT, GL_UNSIGNED_BYTE, imageData);
            GL_CHECK;

            FlipImage   (bi.mImage);
            SwizzleImage(bi.mImage);

            bi.mUpdateCount++;
        }
        break;

    default:
        break;
    }

    GL_CHECK;
}

// Internal

const cLayerInfo* cRenderer::LayerInfo(tTag layerTag) const
{
    auto it = mCodeLayers.find(layerTag);

    if (it != mCodeLayers.end())
        return &it->second;

    auto it2 = mDataLayers.find(layerTag);

    if (it2 != mDataLayers.end())
        return &it2->second;

    return nullptr;
}

bool cRenderer::DispatchJobGroup(tTag jobGroupTag, const cRenderLayerState& state)
{
    bool success = true;
    const auto range = mRenderJobs.equal_range(jobGroupTag);

    for (auto it = range.first; it != range.second; ++it)
    {
    #if CL_TAG_DEBUG
        success = DispatchLayer(it->second, state, it->second) && success;
    #else
        success = DispatchLayer(it->second, state, "DispatchJobGroup") && success;
    #endif
    }

    return success;
}

bool cRenderer::DispatchLayer(tTag layerTag, const cRenderLayerState& state, const char* label)
{
    const cLayerInfo* layerInfo = LayerInfo(layerTag);

    if (layerInfo && layerInfo->mEnabled)
    {
        GL_DEBUG_BEGIN(layerTag);

        SetShaderDataT(kDataIDModelToWorld, Mat4f(vl_I));
        PushState(label);

        cRenderLayerState childState(state);
        childState.mLayerTag = layerTag;
        childState.mLayerFlags = layerInfo->mFlags;

        layerInfo->mLayer->Dispatch(this, childState);

        PopState(label);

        GL_DEBUG_END();
        return true;
    }

    return false;
}

void cRenderer::ResetState()
{
    mRenderState.ResetState();
    mRenderState.SetDeviceState();  // ensure we match context

    mCurrentCamera = 0;
    mCameraStack.clear();
}

void cRenderer::PushState(const char* debugTag)
{
    // TODO: shader data.
    mRenderState.PushState(debugTag);

    mCameraStack.push_back(mCurrentCamera);
}

void cRenderer::PopState (const char* debugTag)
{
    // TODO: shader data.
    mRenderState.PopState(debugTag);

    mCurrentCamera = mCameraStack.back();
    mCameraStack.pop_back();
}


void cRenderer::SetStateForDispatch()
{
    if (mCurrentMaterial >= 0)
    {
        const cMaterial& material = mMaterials[mCurrentMaterial];

        glUseProgram(material.mShaderProgram);

        UploadShaderData(material.mShaderProgram, material.mShaderDataBindings);
        BindTextures(material.mShaderProgram, material.mTextureBindings);
    }
}


// Shader data

void cRenderer::GetBindingsFromProgram(GLuint program, vector<cShaderDataBinding>* bindings)
{
    CL_LOG_D("Renderer", "  bindings for program %d:\n", program);

    GLint maxNameLength;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);

    GLchar uniformName[maxNameLength];

    GLint numUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);

    for (int i = 0; i < numUniforms; i++)
    {
        GLsizei nameLength;
        GLint uniformSize;
        GLenum uniformType;
        glGetActiveUniform(program, i, maxNameLength, nullptr, &uniformSize, &uniformType, uniformName);

        if (IsSamplerType(uniformType))
            continue;

        int ref = ShaderDataRefFromTag(TagFromString(uniformName));

        if (ref == kNullDataRef)
        {
            CL_LOG_E("Renderer", "Unknown shader data in program %d: %s\n", program, uniformName);
            continue;
        }

        GLint uniformLocation = glGetUniformLocation(program, uniformName);

        if (uniformLocation < 0)
        {
            CL_LOG("Renderer", "Unknown location: %s\n", uniformName);
            continue;
        }

        bindings->push_back();
        cShaderDataBinding& binding = bindings->back();
        binding.mDataRef  = ref;
        binding.mLocation = uniformLocation;
        binding.mType  = uniformType;
        binding.mCount = uniformSize;

        CL_LOG_D("Renderer", "    %s (%d): %d, size: %d, type: 0x%04x\n", uniformName, ref, i, uniformSize, uniformType);
    }
}

void cRenderer::UploadShaderData(GLuint program, const nCL::vector<cShaderDataBinding>& bindings)
{
    GL_CHECK;

    for (int i = 0, n = bindings.size(); i < n; i++)
    {
        const cShaderDataBinding& binding = bindings[i];
        const cShaderDataInfo& info = mShaderData[binding.mDataRef];

        // TODO: if stamp in shader data matches that in binding, don't re-upload
        if (info.mOffset == kNullDataOffset)
            continue;

        const void* shaderData = mShaderDataStore.Data(info.mOffset);
        int count;

        switch (binding.mType)
        {
        case GL_FLOAT:
            count = min(binding.mCount, info.mSize / sizeof(float));
            glUniform1fv(binding.mLocation, count, (const GLfloat*) shaderData);
            break;
        case GL_FLOAT_VEC2:
            count = min(binding.mCount, info.mSize / sizeof(Vec2f));
            glUniform2fv(binding.mLocation, count, (const GLfloat*) shaderData);
            break;
        case GL_FLOAT_VEC3:
            count = min(binding.mCount, info.mSize / sizeof(Vec3f));
            glUniform3fv(binding.mLocation, count, (const GLfloat*) shaderData);
            break;
        case GL_FLOAT_VEC4:
            count = min(binding.mCount, info.mSize / sizeof(Vec3f));
            glUniform4fv(binding.mLocation, count, (const GLfloat*) shaderData);
            GL_CHECK;
            break;

        case GL_FLOAT_MAT2:
            count = min(binding.mCount, info.mSize / sizeof(Mat2f));
            glUniformMatrix2fv(binding.mLocation, count, binding.mTranspose, (const GLfloat*) shaderData);
            break;
        case GL_FLOAT_MAT3:
            count = min(binding.mCount, info.mSize / sizeof(Mat3f));
            glUniformMatrix3fv(binding.mLocation, count, binding.mTranspose, (const GLfloat*) shaderData);
            break;
        case GL_FLOAT_MAT4:
            count = min(binding.mCount, info.mSize / sizeof(Mat4f));
            glUniformMatrix4fv(binding.mLocation, count, binding.mTranspose, (const GLfloat*) shaderData);
            break;

        case GL_INT:
            count = min(binding.mCount, info.mSize / sizeof(GLint));
            glUniform1iv(binding.mLocation, count, (const GLint*) shaderData);
            break;
        case GL_INT_VEC2:
            count = min(binding.mCount, info.mSize / sizeof(GLint[2]));
            glUniform2iv(binding.mLocation, count, (const GLint*) shaderData);
            break;
        case GL_INT_VEC3:
            count = min(binding.mCount, info.mSize / sizeof(GLint[3]));
            glUniform3iv(binding.mLocation, count, (const GLint*) shaderData);
            break;
        case GL_INT_VEC4:
            count = min(binding.mCount, info.mSize / sizeof(GLint[4]));
            glUniform4iv(binding.mLocation, count, (const GLint*) shaderData);
            break;

        default:
            CL_LOG_D("Renderer", "Bad binding type: 0x%04x\n", binding.mType);
        }

        GL_CHECK_DETAIL;
    }

    GL_CHECK;
}

void cRenderer::GetBindingsFromProgram(GLuint shaderProgram, nCL::vector<cTextureBinding>* bindings)
{
    GLint numUniforms;
    glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numUniforms);
    GL_CHECK_DETAIL;

    glUseProgram(shaderProgram);
    GL_CHECK_DETAIL;

    const cEnumInfo* textureKindsEnum = TextureKindsEnum();

    for (int i = 0; i < numUniforms; i++)
    {
        char uniformName[256];
        GLint size;
        GLenum type;
        glGetActiveUniform(shaderProgram, i, 256, 0, &size, &type, uniformName);
        GL_CHECK_DETAIL;

        CL_LOG_D("GL", "  uniform %s, size %d, type %s (0x%04x)\n", uniformName, size, GetGLTypeName(type), type);

        if (IsSamplerType(type))
        {
            GLint texLocation = ParseEnum(textureKindsEnum, uniformName);

            if (texLocation < 0)
            {
                GLint uniformLocation = glGetUniformLocation(shaderProgram, uniformName);
                GL_CHECK_DETAIL;

                if (uniformLocation >= 0)
                {
//                    CL_LOG_D("GL", "    binding sampler uniform %d to texture index %d\n", uniformLocation, texLocation);

                    GLint textureUnit = -1;
                    glGetUniformiv(shaderProgram, uniformLocation, &textureUnit);
                    GL_CHECK_DETAIL;

                    if (textureUnit >= 0)
                    {
                        bindings->push_back( { TagFromString(uniformName), textureUnit } );
                        CL_LOG_D("GL", "    binding %s to unit %f\n", uniformName, textureUnit);
                    }
                }
                else
                {
                    CL_LOG_D("GL", "    shader doesn't contain sampler uniform %s - ignoring\n", uniformName);
                }
            }
        }
    }
}

void cRenderer::BindTextures(GLuint programID, const nCL::vector<cTextureBinding>& bindings)
{
    for (auto& b : bindings)
    {
        glActiveTexture(GL_TEXTURE0 + b.mUnit);

        auto it = mKindTagToTextureIndex.find(b.mKind);

        if (it != mKindTagToTextureIndex.end())
            glBindTexture(GL_TEXTURE_2D, it->second);
        else
            glBindTexture(GL_TEXTURE_2D, 0);
    }
}

#ifndef CL_RELEASE
void cRenderer::DebugMenu(cUIState* uiState)
{
    tUIItemID id = ItemID(0x01ad8b33);

    if (uiState->BeginSubMenu(id++, "Flags"))
    {
        tUIItemID subID = ItemID(0x01ad8b34);

        for (const auto& tagIndex : mRenderFlagTags)
        {
            const char* tag = tagIndex.first;
            int flag        = tagIndex.second;

            if (uiState->HandleToggle(subID++, tag, (mRenderFlags & (1 << flag))))
                mRenderFlags ^= 1 << flag;
        }

        uiState->EndSubMenu(id - 1);
    }


    if (uiState->BeginSubMenu(id++, "Layers"))
    {
        tUIItemID subID = ItemID(0x01ad8b35);

        for (auto& tagAndLayer : mDataLayers)
            uiState->HandleToggle(subID++, tagAndLayer.first, &tagAndLayer.second.mEnabled);

        uiState->DrawSeparator();

        for (auto& tagAndLayer : mCodeLayers)
            uiState->HandleToggle(subID++, tagAndLayer.first, &tagAndLayer.second.mEnabled);

        uiState->EndSubMenu(id - 1);
    }
}
#endif

cIRenderer* nHL::CreateRenderer(nCL::cIAllocator* alloc)
{
    return new(alloc) cRenderer;
}

void nHL::DestroyRenderer(cIRenderer* renderer)
{
    delete static_cast<cRenderer*>(renderer);
}
