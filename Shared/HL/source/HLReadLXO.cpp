//
//  File:       READLXO.cpp
//
//  Function:   Support for reading Modo LXO files
//
//  Author(s):  Andrew Willmott
//
//  Copyright:
//

#include <HLReadLXO.h>

#include <HLGLUtilities.h>

#include <VL234f.h>

#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLString.h>
#include <CLSTL.h>

#include "lxoReader.hpp"

#ifdef HL_FULL_LXO_DEPS
    #include <lxenvelope.h>
    #include <lxidef.h>
#else
    #define LXsICHAN_MORPHDEFORM_MAPNAME             "mapName"
    #define LXsICHAN_TEXTURELAYER_EFFECT             "effect"
    #define LXsICHAN_TEXTURELOC_UVMAP                "uvMap"
    #define LXsICHAN_VIDEOSTILL_FILENAME             "filename"
    #define LXsGRAPH_SHADELOC                        "shadeLoc"
#endif

using namespace nHL;
using namespace nCL;



#ifdef LOCAL_DEBUG

namespace
{
    const char* stringFromId (LXtID4 id)
    {
        static char     idString[] = "'1234'";
        
        typedef union {
            LXtID4          id;
            LxByte          b[4];
        } Union4Bytes;
        
        Union4Bytes*    unionP = (Union4Bytes*)&id;
        
        idString[1] = unionP->b[3];
        idString[2] = unionP->b[2];
        idString[3] = unionP->b[1];
        idString[4] = unionP->b[0];

        return idString;
    }

    const char* slopeString (int slopeType)
    {
        static const char* slopeTypes[] =
        {
            "Direct",
            "Auto",
            "Linear In",
            "Linear Out",
            "Flat",
            "Autoflat",
            "Stepped",
        };
        
        static char slopeStr[512];
        
        slopeStr[0] = '\0';
        
        if (slopeType & (1 << (LXiSLOPE_AUTO - 1)))
            strcat (slopeStr, slopeTypes[LXiSLOPE_AUTO]);
        
        if (slopeType & (1 << (LXiSLOPE_LINEAR_IN - 1)))
        {
            if (slopeStr[0])
                strcat (slopeStr, ",");
            strcat (slopeStr, slopeTypes[LXiSLOPE_LINEAR_IN]);
        }
        
        if (slopeType & (1 << (LXiSLOPE_LINEAR_OUT - 1)))
        {
            if (slopeStr[0])
                strcat (slopeStr, ",");
            strcat (slopeStr, slopeTypes[LXiSLOPE_LINEAR_OUT]);
        }
        
        if (slopeType & (1 << (LXiSLOPE_FLAT - 1)))
        {
            if (slopeStr[0])
                strcat (slopeStr, ",");
            strcat (slopeStr, slopeTypes[LXiSLOPE_FLAT]);
        }
        
        if (slopeType & (1 << (LXiSLOPE_AUTOFLAT - 1)))
        {
            if (slopeStr[0])
                strcat (slopeStr, ",");
            strcat (slopeStr, slopeTypes[LXiSLOPE_AUTOFLAT]);
        }
        
        if (slopeType & (1 << (LXiSLOPE_STEPPED - 1)))
        {
            if (slopeStr[0])
                strcat (slopeStr, ",");
            strcat (slopeStr, slopeTypes[LXiSLOPE_STEPPED]);
        }
        
        if ('\0' == slopeStr[0])
            strcpy (slopeStr, slopeTypes[LXiSLOPE_DIRECT]);
        
        return slopeStr;
    }

    const char* breakString (int breakType)
    {
        static char     breakStr[512];
        
        breakStr[0] = '\0';
        
        if (breakType & LXfKEYBREAK_VALUE)
            strcat (breakStr, "Value");
        
        if (breakType & LXfKEYBREAK_SLOPE)
            strcat (breakStr, breakStr[0] ? ",Slope" : "Slope" );
        
        if (breakType & LXfKEYBREAK_WEIGHT)
            strcat (breakStr, breakStr[0] ? ",Weight" : "Weight" );
        
        if ('\0' == breakStr[0])
            strcpy (breakStr, "None");
        
        return breakStr;
    }
}


class LxoDump : public LxoReader
{
    FILE*       m_outFile;
    bool        m_isStdout;
    bool        m_dumpGeom;         // show point/poly arrays
    
public:
    LxoDump (char* lxoName, char* outName, bool dumpAll);
    ~LxoDump ();
    
    void        PrintSubHeader ();
    void        PrintValue (LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer = NULL, LxUShort length = 0);
    
    virtual LxResult	ProcessHeader ();
    virtual LxResult	ProcessUnknown ();
    virtual LxResult	ProcessVersion (LxULong major, LxULong minor, char* str);
    virtual LxResult	ProcessParent (char* str);
    virtual LxResult	ProcessDescription (char* type, char* desc);
    virtual LxResult    ProcessChannelNames (LxULong count, LXtStringVec chans);
    virtual LxResult	ProcessBoundingBox (LXtFVector min, LXtFVector max);
    virtual LxResult	ProcessPolyTags (LXtID4 id, LxULong count, LXtPolyTag* ptags);
    virtual LxResult	ProcessTags (LXtStringVec tags);
    virtual LxResult	ProcessLayer (LXtLayerChunk& layer);
    virtual LxResult	ProcessSurfaceInfo (LXtSufaceInfo& surf);
    virtual LxResult	ProcessGroupInfo (LXtGroupInfo& group);
    virtual LxResult	ProcessPoints (LxULong count, LXtFVector* points);
    virtual LxResult	ProcessTriangles (LxULong count, LXtVertIndex* tris);
    virtual LxResult	ProcessTextTag (LXtID4 id, char* str);
    virtual LxResult	ProcessPolygon (LXtID4 type, LxUShort nVerts, LxULong* verts);
    virtual LxResult	ProcessVectors (LXtID4 id, char* name, LxULong nVectors, LxULong nComponents, float* coords);
    virtual LxResult	ProcessVertexMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents);
    virtual LxResult	ProcessVertexMapFlags (LxULong flags[2]);
    virtual LxResult	ProcessVertexMapEntry (LxULong nComponents, LxULong vertIndex, float* values);
    virtual LxResult	ProcessVertexDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values);
    virtual LxResult	ProcessEdgeMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents);
    virtual LxResult	ProcessEdgeDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values);
    virtual LxResult	ProcessPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData);
    virtual LxResult	ProcessThumbnail (LxUShort width, LxUShort height, LxByte nChannels, LxByte flags, LxULong dataBytes, LxByte* imageData);
    virtual LxResult	ProcessBake (LxULong refId, LxULong samples, float startTime, float sampsPerSec);
    
    virtual LxResult	ProcessItem (char* itemType, char* name, LxULong refId);
    virtual LxResult	ProcessItemUnknown ();
    virtual LxResult	ProcessItemXref (LxULong index, char* filename, char* idStr);
    virtual LxResult	ProcessItemLayer (LxULong index, LxULong flags, LxULong color);
    virtual LxResult	ProcessItemId (char* name);
    virtual LxResult	ProcessItemIndex (LxULong index);
    virtual LxResult	ProcessItemLink (char* name, LxULong refId, LxULong index);
    virtual LxResult	ProcessItemBoundingBox (LXtFVector min, LXtFVector max);
    virtual LxResult	ProcessChannelLink (char* name, char* fromChannel, LxULong index, char* toChannel, LxULong fromIndex, LxULong toIndex);
    virtual LxResult	ProcessItemPackage (char* name, LxULong bytes, LxByte* buffer);
    virtual LxResult	ProcessItemGradient (char* name, LxULong index, LxULong flags, char* inType, char* outType);
    virtual LxResult	ProcessItemTag (LXtID4 tagType, char* value);
    virtual LxResult	ProcessItemPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData);
    
    virtual LxResult	ProcessItemChannelScalar (char* name, LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer, LxUShort length);
    virtual LxResult	ProcessItemChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal);
    virtual LxResult	ProcessItemChannelVector (char* name, LxUShort type, LxUShort count);
    virtual LxResult	ProcessItemChannelVectorValue (char* name, LxUShort type, int intVal, float floatVal, char* strVal);
    virtual LxResult	ProcessItemChannelString (char* name, char* str);
    virtual LxResult	ProcessCustomChannel (char* name, char* str, LxULong dataBytes, LxByte* customData);
    virtual LxResult	ProcessUserChannel (char* name, char* str);
    
    virtual LxResult	ProcessEnvelope (LxULong index, LxULong type);
    virtual LxResult	ProcessEnvelopeUnknown ();
    virtual LxResult	ProcessEnvelopeBehavior (LxUShort flag);
    virtual LxResult	ProcessEnvelopeTanIn (LxUShort slopeType, LxUShort weightType, float slope, float weight, float value);
    virtual LxResult	ProcessEnvelopeTanOut (LxULong breaks, LxUShort slopeType, LxUShort weightType, float slope, float weight, float value);
    virtual LxResult	ProcessEnvelopeKey (float time, float value);
    virtual LxResult	ProcessEnvelopeKey (float time, LxULong value);
    virtual LxResult	ProcessEnvelopeFlag (LxULong flag);
    
    virtual LxResult	ProcessAction (char* actionType, char* name, LxULong refId);
    virtual LxResult	ProcessActionUnknown ();
    virtual LxResult	ProcessActionParent (LxULong index);
    virtual LxResult	ProcessActionItem (LxULong index);
    virtual LxResult	ProcessActionChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal);
    virtual LxResult	ProcessActionChannelName (char* name, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal);
    virtual LxResult	ProcessActionChannelString (char* name, char* str);
    virtual LxResult	ProcessActionGradient (LxULong index, char *name, LxULong envelopeIndex, LxULong flags);
    
    virtual LxResult	ProcessAudio ();
    virtual LxResult	ProcessAudioItem (LxULong index);
    virtual LxResult	ProcessAudioSettings (LxUShort loop, LxUShort mute, LxUShort scrub, float start);
};

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * validate and open the input & output files
 *
 *----------------------------------------------------------------------------*/
LxoDump::LxoDump (char* lxoName, char* outName, bool dumpAll) : LxoReader (lxoName)
{
    m_outFile = NULL;
    m_dumpGeom = dumpAll;
    
    if (! IsLxoValid ())
        return;
    
    m_isStdout = (NULL == outName || '\0' == *outName);
    if (m_isStdout)
        m_outFile = stdout;
    
    else if (NULL == (m_outFile = fopen (outName, "w"))) {
        CL_LOG("DumpLXO", "Error writing output file <%s>\n", outName);
        return;
    }
    
    fprintf (m_outFile, "=========== Dump of %s ===========\n", lxoName);
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * close the open output file, unless it is stdout, in which case we must leave it
 * open for prints outside this class.
 *
 *----------------------------------------------------------------------------*/
LxoDump::~LxoDump ()
{
    if (m_outFile && !m_isStdout)
        fclose (m_outFile);         // only close *real* files (leave stdout open for prints)
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * override the virtual methods to process the chunk/header information.  Here we simply
 * print it out, but other applications could use this information for import.
 *
 *----------------------------------------------------------------------------*/
LxResult LxoDump::ProcessHeader ()
{
    fprintf (m_outFile, "\nChunk ID: %s  Size: %d\n", stringFromId (m_chunkId), m_chunkSize);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessUnknown ()
{
    CL_LOG("DumpLXO", "*** Unknown Chunk ID: %s  Size: %d\n", stringFromId (m_chunkId), m_chunkSize);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVersion (LxULong major, LxULong minor, char* str)
{
    fprintf (m_outFile, "\tMajor: %d  Minor: %d <%s>\n", major, minor, str);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessDescription (char* type, char* desc)
{
    fprintf (m_outFile, "\tType/Description <%s> <%s>\n", type, desc);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessParent (char* str)
{
    fprintf (m_outFile, "\tParent <%s>\n", str);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData)
{
#if 0
    int     previewFormat = type &  LXiIMD_FLOAT;
    int     previewColors = type & ~LXiIMD_FLOAT;
    
    fprintf (m_outFile, "\tPreview size: %d x %d, type: %s %s (0x%x), flags: %s (0x%x), # Image bytes: %d\n", width, height,
             LXiIMV_GREY == previewColors ? "Greyscale" : (LXiIMV_RGB == previewColors ? "RGB" : "RGBA"),
             LXiIMD_FLOAT == previewFormat ? "Floats" : "Bytes", type,
             flags ? "Compressed" : "Uncompressed", flags, dataBytes);
#endif

    return LXe_OK;
}

LxResult LxoDump::ProcessThumbnail (LxUShort width, LxUShort height, LxByte nChannels, LxByte flags, LxULong dataBytes, LxByte* imageData)
{
    fprintf (m_outFile, "\tThumbnail size: %d x %d, Channels: %d %s, flags: 0x%x, # Image bytes: %d\n", width, height, nChannels,
             1 == nChannels ? "Greyscale" : (3 == nChannels ? "RGB" : (4 == nChannels ? "RGBA" : "Custom")),
             flags, dataBytes);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessBake (LxULong refId, LxULong samples, float startTime, float sampsPerSec)
{
    fprintf (m_outFile, "Reference ID: %d, #Samples: %d, Start Time %f, samples per second: %f\n", refId, samples, startTime, sampsPerSec);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessChannelNames (LxULong count, LXtStringVec chans)
{
    fprintf (m_outFile, "#Channels: %d\n", count);
    
    for (LxULong i = 0; i < count; ++i)
        fprintf (m_outFile, "\tChannel %3d: <%s>\n", i, chans[i].c_str());
    
    return LXe_OK;
}

LxResult LxoDump::ProcessPolyTags (LXtID4 id, LxULong count, LXtPolyTag* ptags)
{
    fprintf (m_outFile, "ID: %s  Count: %d\n", stringFromId (id), count);
    
    if (m_dumpGeom)
        for (LxULong i = 0; i < count; ++i)
            fprintf (m_outFile, "\tPoly Index: %d, Tag index %d <%s>\n", ptags[i].polyIndex, ptags[i].tagIndex,
                     ptags[i].tagIndex < m_tags.size() ? m_tags[ptags[i].tagIndex].c_str() : "***Undefined***");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessTextTag (LXtID4 id, char* str)
{
    fprintf (m_outFile, "ID: %s  Tag: <%s>\n", stringFromId (id), str);
    
    return LXe_OK;
}
LxResult LxoDump::ProcessTags (LXtStringVec tags)
{
    int     tagNum = 0;
    
    for (LXtStringVec::iterator it = tags.begin(); it != tags.end(); it++)
        fprintf (m_outFile, "\tTags[%3d]: <%s>\n", tagNum++, it->c_str());
    
    return LXe_OK;
}

LxResult LxoDump::ProcessLayer (LXtLayerChunk& layer)
{
#ifdef DISABLED
    fprintf (m_outFile, "Mesh: Reference ID: %d, Index %d, Flags 0x%x, Pivot (%f,%f,%f), Name: <%s> Parent %d\n",
             layer.m_ref, layer.m_index, layer.m_flags, layer.m_pivot[0], layer.m_pivot[1], layer.m_pivot[2], layer.m_name, layer.m_parent);
    fprintf (m_outFile, "Spline Level %d, Subdivision Level %f, Curve Angle: %f, Scale Pivot (%f,%f,%f)\n",
             layer.m_splinePatchLevel, layer.m_subdivisionLevel, layer.m_curveAngle, layer.m_scalePivot[0], layer.m_scalePivot[1], layer.m_scalePivot[2]);
#endif
    return LXe_OK;
}

LxResult LxoDump::ProcessSurfaceInfo (LXtSufaceInfo& surf)
{
    fprintf (m_outFile, "#Vertices: %d, #Triangles: %d, #Vectors: %d, #Tags: %d, Unused: %d\n", surf.nVerts, surf.nTris, surf.nVectors, surf.nTags, surf.unused);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessGroupInfo (LXtGroupInfo& group)
{
    fprintf (m_outFile, "%s: Reference ID: %d, #Surfs: %d, Unused: %d\n", group.triSurfIndex & 0x80000000 ? "BakedTriGroup" : "StaticMesh",
             group.triSurfIndex & 0x7fffffff, group.nSurfs, group.unused);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessBoundingBox (LXtFVector min, LXtFVector max)
{
    fprintf (m_outFile, "\tMin: <%15.6f,%15.6f,%15.6f>\n", min[0], min[1], min[2]);
    fprintf (m_outFile, "\tMax: <%15.6f,%15.6f,%15.6f>\n", max[0], max[1], max[2]);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessPoints (LxULong count, LXtFVector* points)
{
    fprintf (m_outFile, "#Points: %d\n", count);
    
    if (! m_dumpGeom)
        return LXe_OK;
    
    for (LxULong i = 0; i < count; ++i)
        fprintf (m_outFile, "\tPoint[%3d]: <%15.6f,%15.6f,%15.6f>\n", i, points[i][0], points[i][1], points[i][2]);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessTriangles (LxULong count, LXtVertIndex* tris)
{
    fprintf (m_outFile, "#Triangles: %d\n", count);
    
    if (! m_dumpGeom)
        return LXe_OK;
    
    for (LxULong i = 0; i < count; ++i)
        fprintf (m_outFile, "\tTriangle[%3d]: [%3d,%3d,%3d]\n", i, tris[i].a, tris[i].b, tris[i].c);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessPolygon (LXtID4 type, LxUShort nVerts, LxULong* verts)
{
    if (! m_dumpGeom)
        return LXe_OK;
    
    fprintf (m_outFile, "Poly Type: %s, #Vertices: %d  Flags: 0x%02x\n\t", stringFromId (type), nVerts & 0x3FF, nVerts >> 10);
    
    for (LxUShort i = 0; i < nVerts; ++i)
        fprintf (m_outFile, " %d", verts[i]);
    fprintf (m_outFile, "\n");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVectors (LXtID4 id, char* name, LxULong nVectors, LxULong nComponents, float* coords)
{
    fprintf (m_outFile, "ID: %s, Name: <%s>, #Vectors: %d (%d-D)\n", stringFromId (id), name, nVectors, nComponents);
    
    if (! m_dumpGeom)
        return LXe_OK;
    
    for (LxULong i = 0; i < nVectors; ++i)
    {
        fprintf (m_outFile, "\tVector[%3d]:", i);
        for (LxULong j = 0; j < nComponents; ++j)
            fprintf (m_outFile, " %15.6f", *coords++);
        fprintf (m_outFile, "\n");
    }
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVertexMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents)
{
    fprintf (m_outFile, "ID: %s, Name: <%s>, #Maps: %d, (%d-D)\n", stringFromId (id), name, nMaps, nComponents);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVertexMapFlags (LxULong flags[2])

{
    fprintf (m_outFile, "\tFlags: 0x%08x 0x%08x\n", flags[0], flags[1]);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVertexMapEntry (LxULong nComponents, LxULong vertIndex, float* values)
{
    if (! m_dumpGeom)
        return LXe_OK;
    
    fprintf (m_outFile, "\tVertex Index: %3d :: ", vertIndex);
    for (LxULong j = 0; j < nComponents; ++j)
        fprintf (m_outFile, " %15.6f", *values++);
    fprintf (m_outFile, "\n");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessVertexDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values)
{
    if (! m_dumpGeom)
        return LXe_OK;
    
    fprintf (m_outFile, "\tVertex Index: %3d, PolyIndex: %3d :: ", vertIndex, polyIndex);
    for (LxULong j = 0; j < nComponents; ++j)
        fprintf (m_outFile, " %15.6f", *values++);
    fprintf (m_outFile, "\n");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEdgeMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents)
{
    fprintf (m_outFile, "ID: %s, Name: <%s>, #Maps: %d, (%d-D)\n", stringFromId (id), name, nMaps, nComponents);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEdgeDMapEntry (LxULong nComponents, LxULong vertIndex1, LxULong vertIndex2, float* values)
{
    if (! m_dumpGeom)
        return LXe_OK;
    
    fprintf (m_outFile, "\tEdge from Vertex Index: %3d, to: %3d :: ", vertIndex1, vertIndex2);
    for (LxULong j = 0; j < nComponents; ++j)
        fprintf (m_outFile, " %15.6f", *values++);
    fprintf (m_outFile, "\n");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItem (char* itemType, char* name, LxULong refId)
{
    fprintf (m_outFile, "Type: <%s>, Name: <%s>, Reference ID: %d\n", itemType, name, refId);
    
    return LXe_OK;
}

void LxoDump::PrintSubHeader ()
{
    fprintf (m_outFile, "\tSubChunk ID: %s Size: %3d:\t", stringFromId (m_subChunkId), m_subChunkSize);
}

LxResult LxoDump::ProcessItemUnknown ()
{
    PrintSubHeader ();
    fprintf (m_outFile, "*** Unknown ***\n");
    CL_LOG("DumpLXO", "*** Unknown Item Sub-Chunk ID: %s  Size: %d\n", stringFromId (m_subChunkId), m_subChunkSize);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemXref (LxULong index, char* filename, char* idStr)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Index: %3d, FileName: <%s>, Reference ID: <%s>\n", index, filename, idStr);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemLayer (LxULong index, LxULong flags, LxULong color)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Index: %3d, Flags: 0x%x, RGBA: 0x%08x\n", index, flags, color);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemId (char* name)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Name: <%s>\n", name);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemIndex (LxULong index)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Index: %3d\n", index);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemBoundingBox (LXtFVector min, LXtFVector max)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Min: <%f, %f, %f> Max: <%f, %f, %f>\n", min[0], min[1], min[2], max[0], max[1], max[2]);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemLink (char* name, LxULong refId, LxULong index)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Name: <%s>, Reference ID: %d, Index: %3d\n", name, refId, index);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessChannelLink (char* name, char* fromChannel, LxULong index, char* toChannel, LxULong fromIndex, LxULong toIndex)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Name: <%s>, Item ID: %d, From Index: %3d to %3d, From Channel: <%s> to: <%s> \n",
             name, index, fromIndex, toIndex, fromChannel, toChannel);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemPackage (char* name, LxULong bytes, LxByte* buffer)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Name: <%s>, Size: %d\n", name, bytes);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemGradient (char* name, LxULong index, LxULong flags, char* inType, char* outType)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Name: <%s>, Index: %3d, Flags: %d", name, index, flags);
    
    if (inType)
        fprintf (m_outFile, ", inType: <%s>", inType);
    if (outType)
        fprintf (m_outFile, ", outType: <%s>", outType);
    
    fprintf (m_outFile, "\n");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemTag (LXtID4 tagType, char* value)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Type: <%s>, Value: <%s>\n", stringFromId (tagType), value);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData)
{
#if 0
    int     previewFormat = type &  LXiIMD_FLOAT;
    int     previewColors = type & ~LXiIMD_FLOAT;
    
    PrintSubHeader ();
    fprintf (m_outFile, "Size: %d x %d, type: %s %s (0x%x), flags: %s (0x%x), # Image bytes: %d\n", width, height,
             LXiIMV_GREY == previewColors ? "Greyscale" : (LXiIMV_RGB == previewColors ? "RGB" : "RGBA"),
             LXiIMD_FLOAT == previewFormat ? "Floats" : "Bytes", type,
             flags ? "Compressed" : "Uncompressed", flags, dataBytes);
#endif

    return LXe_OK;
}

void LxoDump::PrintValue (LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer, LxUShort length)
{
    type = LXItemType_FloatAlt == type ? LXItemType_Float : type & 0xff;
    
    switch (type & ~LXItemType_UndefState)
    {
        case LXItemType_Int:
        case LXItemType_EnvelopeInt:
            fprintf (m_outFile, "Int: %d\n", intVal);
            break;
            
        case LXItemType_Float:
        case LXItemType_EnvelopeFloat:
        case LXItemType_FloatAlt:
            fprintf (m_outFile, "Float: %f\n", floatVal);
            break;
            
        case LXItemType_String:
        case LXItemType_EnvelopeString:
            fprintf (m_outFile, "String: <%s>\n", strVal);
            break;
            
        case LXItemType_Variable:
            fprintf (m_outFile, "VarLength: %d\n", length);
            break;
    }
}

LxResult LxoDump::ProcessItemChannelScalar (char* name, LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer, LxUShort length)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>, (%d) ", name, type);
    PrintValue (type, intVal, floatVal, strVal, buffer, length);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan[%3d]: <%-16s>", index, GetChannelName (index));
    
    if (type & LXItemType_Envelope)
        fprintf (m_outFile, ", EnvelopeIndex: %d", envelopeIndex);
    
    fprintf (m_outFile, " (0x%x) ", type);
    PrintValue (type, intVal, floatVal, strVal);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemChannelVector (char* name, LxUShort type, LxUShort count)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>, Type: %d, #Elements: %d\n", name, type, count);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemChannelVectorValue (char* name, LxUShort type, int intVal, float floatVal, char* strVal)
{
    fprintf (m_outFile, "\t\t<%s> = ", name);
    PrintValue (type, intVal, floatVal, strVal);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessItemChannelString (char* name, char* str)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>, Value: <%s>\n", name, str);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessCustomChannel (char* name, char* str, LxULong dataBytes, LxByte* customData)

{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>, Value: <%s>, bytes: %d\n", name, str, dataBytes);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessUserChannel (char* name, char* str)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>, Value: <%s>\n", name, str);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessActionParent (LxULong index)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Index: %3d\n", index);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessActionItem (LxULong index)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Reference ID: %d\n", index);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessActionChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal)
{
    return ProcessItemChannelGeneral (index, type, envelopeIndex, intVal, floatVal, strVal);
}

LxResult LxoDump::ProcessActionChannelName (char* name, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Chan: <%-21s>", name);
    
    if (type & LXItemType_Envelope)
        fprintf (m_outFile, ", EnvelopeIndex: %d", envelopeIndex);
    
    fprintf (m_outFile, " (0x%x) ", type);
    PrintValue (type, intVal, floatVal, strVal);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessActionChannelString (char* name, char* str)
{
    return ProcessItemChannelString (name, str);
}

LxResult LxoDump::ProcessActionGradient (LxULong index, char *name, LxULong envelopeIndex, LxULong flags)
{
    PrintSubHeader ();
    if (0 == index)
        fprintf (m_outFile, "Chan: <%-21s>", name);
    else
        fprintf (m_outFile, "Chan[%3d]: <%-16s>", index, GetChannelName (index));
    
    fprintf (m_outFile, ", EnvelopeIndex: %d, Flags: 0x%x\n", envelopeIndex, flags);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessActionUnknown ()
{
    PrintSubHeader ();
    fprintf (m_outFile, "*** Unknown ***\n");
    CL_LOG("DumpLXO", "*** Unknown Action Sub-Chunk ID: %s  Size: %d\n", stringFromId (m_subChunkId), m_subChunkSize);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessAudioItem (LxULong index)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Reference ID: %d\n", index);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessAudioSettings (LxUShort loop, LxUShort mute, LxUShort scrub, float start)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Loop: %3d Mute: %3d Scrub: %3d Start: %f\n", loop, mute, scrub, start);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelope (LxULong index, LxULong type)
{
    fprintf (m_outFile, "\tEnvelopeIndex: %d, Type: %d (%s)\n",
             index, type, LXEnvelopeType_Float == type ? "float" : "int");
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeUnknown ()
{
    PrintSubHeader ();
    fprintf (m_outFile, "*** Unknown ***\n");
    CL_LOG("DumpLXO", "*** Unknown Envelope Sub-Chunk ID: %s  Size: %d\n", stringFromId (m_subChunkId), m_subChunkSize);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeBehavior (LxUShort flag)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Flag: 0x%04x\n", flag);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeTanIn (LxUShort slopeType, LxUShort weightType, float slope, float weight, float value)
{
    PrintSubHeader ();

    fprintf (m_outFile, "Slope Type: %d (%s), Weight Type: %d, Slope: %f, Weight: %f, Value: %f\n", slopeType, slopeString (slopeType), weightType, slope, weight, value);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeTanOut (LxULong breaks, LxUShort slopeType, LxUShort weightType, float slope, float weight, float value)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Break: %d (%s), Slope Type: %d (%s), Weight Type: %d, Slope: %f, Weight: %f, Value: %f\n", breaks, breakString (breaks), slopeType, slopeString (slopeType), weightType, slope, weight, value);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeKey (float time, float value)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Time: %f, Value: %f\n", time, value);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeKey (float time, LxULong value)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Time: %f, Value: %d\n", time, value);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessEnvelopeFlag (LxULong flag)
{
    PrintSubHeader ();
    fprintf (m_outFile, "Flag: 0x%08x\n", flag);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessAction (char* actionType, char* name, LxULong refId)
{
    fprintf (m_outFile, "Type: <%s>, Name: <%s>, Reference ID: %d\n", actionType, name, refId);
    
    return LXe_OK;
}

LxResult LxoDump::ProcessAudio ()
{
    fprintf (m_outFile, "Type: <audio>\n");
    
    return LXe_OK;
}

#endif

//------------------------------------------------------------------------------
// LXO mesh loader
//------------------------------------------------------------------------------

/*
    Notes:
    
    Items:
        locator
            links to parent if any
            links to genInfluence
        translation
        rotation
        scale
            link to locator + index:
                0: <rotation>
                1: <rotation zero>
                2: <translation>
                3: <translation zero>
            CHNV with value

        genInfluence
            link to <mesh>
            type=<mapWeight>
            name=__item_locatorNNN   -> this is the name of the WGHT VMAP

        morphDeform
            CHNS/<mapName> -> morph name, i.e., name of VMAP
 
        txtrLocator
            CHNS/uvMap -> name of VMAP/TXUV to use
        videoStill
            CHNS/filename -> filename
        imageMap
            LINK shadeLoc -> videoStill
            LINK shadeLoc -> txtrLocator
 
 
    Envelopes:
        index
        list of keyframes: PRE/POST/KEY/TANI/TANO/FLAG
        
 
    Actions:
        scene
            envelope bindings to locators
        setup
            default values for items: ITEM+RefID -> CHAN+
            
            

 
    See lxmesh.h for mesh definitions and tags
 */


struct cLocator
{
    Vec3f mTranslation;
    Vec3f mRotation;
    Vec3f mScale;
};

enum tItemType
{
    kLocatorItem,
    kTranslationItem,
    kRotationItem,
    kScaleItem,
    
    kInfluenceItem,
    kMorphItem,
    
    kTextureLocatorItem,
    kImageItem,
    kTextureItem,

    kMaxItemTypes
};

const cEnumInfo kItemTypeEnum[] =
{
    "locator",      kLocatorItem,
    "translation",  kTranslationItem,
    "rotation",     kRotationItem,
    "scale",        kScaleItem,
    
    "genInfluence", kInfluenceItem,
    "morphDeform",  kMorphItem,
    
    "txtrLocator",  kTextureLocatorItem,
    "videoStill",   kImageItem,
    "imageMap",     kTextureItem,
    0, 0
};

struct cMorph
{
    ustl::vector<int>    mIndices;
    ustl::vector<Vec3f>  mDeltas;
};

struct cTextureLocator
{
    int mUVMap;
    
    cTextureLocator() : mUVMap(-1) {}
};

struct cImageInfo
{
    ustl::string mFileName;
};

struct cTextureInfo
{
    int mTextureLocator;
    int mImage;
    int mKind;

    cTextureInfo() : mTextureLocator(-1), mImage(-1), mKind(-1) {}
};


struct cKey
{
    // LXiENVv_INTERP_CURVE
    float mTime;
    float mValue;
    int   mBreakFlags;  // LXfKEYBREAK_VALUE
    
    int   mSlopeType[2];  // en_LXtSlopeType
    float mSlope[2];
    
    int   mWeightType[2];
    float mWeight[2];
    
    float mValue2[2];
};

struct cEnvelope
{
    int mEndType[2]; // en_LXtEndBehavior

    ustl::vector<cKey> mKeys;
};

cEnumInfo kTextureEffectEnum[] =
{
    "diffuse",  kTextureDiffuseMap,
    "RGBA",     kTextureDiffuseMap,
    "normal",   kTextureNormalMap,
    "specular", kTextureNormalMap,
    "shadow",   kTextureShadowMap,
    0, 0
};

class cLXOMesh : public LxoReader
{
public:
    enum { kMaxWeights = 8 };

    typedef ustl::map<LxULong, int>       tRefIDToIndexMap;
    typedef ustl::map<ustl::string, int>  tNameToIndexMap;
    
    // BBox
    Vec3f mMin;
    Vec3f mMax;

    // Mesh
    ustl::vector<Vec3f> mPoints;
    ustl::vector<Vec3f> mNormals;
    ustl::vector<Vec2f> mTexCoords;
    ustl::vector<Vec3f> mColours;

    // Texturing
    ustl::vector<cTextureLocator>    mTextureLocators;
    ustl::vector<cImageInfo>         mImages;
    ustl::vector<cTextureInfo>       mTextures;
    
    // Bone animation
    int mNumWeights;
    ustl::vector<float> mWeights[kMaxWeights];

    // Morphs
    ustl::vector<cMorph> mMorphs;

    // Triangles/polys
    ustl::vector<uint16_t> mIndices;

    // Animation etc.
    ustl::vector<cEnvelope> mEnvelopes;
    ustl::vector<cLocator> mLocators;
    
    // Parse temporary
    LXtID4          mMapID;
    tItemType       mItemType;
    LxULong         mItemRefID;
    int             mCurrentLocator;
    int             mCurrentEnvelope;
    
    tNameToIndexMap  mNameToUVMap;
    tRefIDToIndexMap mTextureLocatorMap;
    tRefIDToIndexMap mImageMap;
    
    tRefIDToIndexMap mLocatorRefMap;
    
    tNameToIndexMap  mNameToMorphIndexMap;
    tRefIDToIndexMap mMorphRefToIndexMap;
    
    cLXOMesh(const char* filename) :
        LxoReader(filename),
    
        mMin(),
        mMax(),
        
        // Mesh
        mPoints(),
        mNormals(),
        mTexCoords(),
        mColours(),
    
        mTextureLocators(),
        mImages(),
        mTextures(),
    
        mNumWeights(0),
        // mWeights[]
    
        mMorphs(),
    
        mIndices(),
    
        mEnvelopes(),
        mLocators(),
   
        mMapID(0),
        mItemType(kMaxItemTypes),
        mItemRefID(~0),
        mCurrentLocator(-1),
        mCurrentEnvelope(-1),
    
        mNameToUVMap(),
        mTextureLocatorMap(),
        mImageMap(),

        mLocatorRefMap(),
    
        mNameToMorphIndexMap(),
        mMorphRefToIndexMap()
    {
    }
    
    virtual LxResult ProcessHeader()
    {
        return LXe_OK;
    }
    
    virtual LxResult ProcessVersion(LxULong major, LxULong minor, char* str)
    {
        return LXe_OK;
    }
    
    virtual LxResult ProcessBoundingBox(LXtFVector min, LXtFVector max)
    {
        mMin[0] = min[0];
        mMin[1] = min[1];
        mMin[2] = min[2];
        mMax[0] = max[0];
        mMax[1] = max[1];
        mMax[2] = max[2];

        return LXe_OK;
    }
    
    virtual LxResult ProcessPoints(LxULong count, LXtFVector* points)
    {
        mPoints.resize(count);
        memcpy(mPoints.data(), points, count * sizeof(Vec3f));

        for (int i = 0; i < count; i++)
            mPoints[i] = -mPoints[i];

        return LXe_OK;
    }
    
    virtual LxResult ProcessTriangles(LxULong count, LXtVertIndex* tris)
    {
        mIndices.resize(count * 3);

        // Note: Modo, at least in the Z-up mode we're using, uses clockwise
        // orientation for forward facing triangles or polygons. Thus
        // we reverse on load.
        for (int i = 0; i < count; i++)
        {
            uint16_t* indices = &mIndices[i * 3];

            indices[0] = tris[i].c;
            indices[1] = tris[i].b;
            indices[2] = tris[i].a;
        }
        
        return LXe_OK;
    }

    LxResult ProcessPolygon(LXtID4 type, LxUShort nVerts, LxULong* verts)
    {
        int count = nVerts & 0x3FF;

        // Note: Modo, at least in the Z-up mode we're using, uses clockwise
        // orientation for forward facing triangles or polygons. Thus
        // we reverse on load.
        if (count == 3)
        {
            mIndices.push_back(verts[2]);
            mIndices.push_back(verts[1]);
            mIndices.push_back(verts[0]);
        }
        else if (count == 4)
        {
            mIndices.push_back(verts[2]);
            mIndices.push_back(verts[1]);
            mIndices.push_back(verts[0]);

            mIndices.push_back(verts[0]);
            mIndices.push_back(verts[3]);
            mIndices.push_back(verts[2]);
        }
        else
        {
            // crappy quick tri
            mIndices.push_back(verts[2]);
            mIndices.push_back(verts[1]);
            mIndices.push_back(verts[0]);

            for (int i = 2; i < nVerts - 1; i++)
            {
                mIndices.push_back(verts[0]);
                mIndices.push_back(verts[i + 1]);
                mIndices.push_back(verts[i]);
            }
        }

        return LXe_OK;
    }

    virtual LxResult ProcessVertexMap(LXtID4 id, char* name, LxULong nMaps, LxULong nComponents)
    {
        if (m_chunkId == 'VMAD')
        {
            switch (id)
            {
            default:
                CL_LOG("ReadLXO", "unhandled DMAP: %4c, %s\n", id, name);
                id = 0;
                break;
            }
        }
        else
        {
            switch (id)
            {
            case 'TXUV':
                mNameToUVMap[name] = 0; // only handle one set for now...
                mTexCoords.resize(mPoints.size());
                break;
            case 'NORM':
                mNormals.resize(mPoints.size());
                break;
            case 'MORF':
                mNameToMorphIndexMap[name] = int(mMorphs.size());
                mMorphs.push_back(cMorph());
                break;
            default:
                id = 0;
                break;
            }
        }
        
        mMapID = id;
        
        return LXe_OK;
    }

    virtual LxResult ProcessVertexMapEntry(LxULong nComponents, LxULong vertIndex, float* values)
    {
        switch (mMapID)
        {
        case 'TXUV':
            mTexCoords[vertIndex][0] = values[0];
            mTexCoords[vertIndex][1] = 1.0f - values[1];
            break;
        case 'NORM':
            mNormals[vertIndex] = Vec3f(values);
            break;
        case 'MORF':
            CL_ASSERT(nComponents == 3);
            mMorphs.back().mDeltas.push_back(Vec3f(values));
            mMorphs.back().mIndices.push_back(vertIndex);
            break;
        }
        
        return LXe_OK;
    }
    
    virtual LxResult ProcessVertexDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values)
    {
        switch (mMapID)
        {
        case 'TXUV':
            // mTexCoords[vertIndex].x = values[0];
            // mTexCoords[vertIndex].y = values[1];
            break;
        case 'NORM':
            // mNormals[vertIndex].x = values[0];
            // mNormals[vertIndex].y = values[1];
            // mNormals[vertIndex].z = values[2];
            break;
        case 'MORF':
            CL_ASSERT(nComponents == 3);
            mMorphs.back().mDeltas.push_back(Vec3f(values));
            mMorphs.back().mIndices.push_back(vertIndex);
            break;
        }
        
        return LXe_OK;
    }

    virtual LxResult ProcessEdgeMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents)
    {
        CL_ASSERT(0);
        return LXe_FAILED;
    }

    virtual LxResult ProcessEdgeDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values)
    {
        CL_ASSERT(0);
        return LXe_FAILED;
    }
    



    LxResult ProcessItem(char* itemType, char* name, LxULong refId)
    {
        mItemType = tItemType(ParseEnum(kItemTypeEnum, itemType));
        mItemRefID = refId;
        mCurrentLocator = -1;
        
        switch (mItemType)
        {
        case kLocatorItem:
            mLocatorRefMap[refId] = int(mLocators.size());
            mLocators.push_back(cLocator());
            break;
            
        case kMorphItem:
            break;

        case kTextureLocatorItem:
            mTextureLocatorMap[refId] = mTextureLocators.size();
            mTextureLocators.push_back(cTextureLocator());
            break;
        case kImageItem:
            mImageMap[refId] = mImages.size();
            mImages.push_back(cImageInfo());
            break;
        case kTextureItem:
            mTextures.push_back(cTextureInfo());
            break;
        default:
            break;
        }
    
        return LXe_OK;
    }
    

    LxResult ProcessItemId (char* name)
    {
        return LXe_OK;
    }
    
    LxResult ProcessItemIndex (LxULong index)
    {
        return LXe_OK;
    }
    
    LxResult ProcessItemLink (char* name, LxULong refId, LxULong index)
    {
        switch (mItemType)
        {
        case kRotationItem:
        case kScaleItem:
        case kTranslationItem:
            {
                tRefIDToIndexMap::iterator it = mLocatorRefMap.find(refId);
                tRefIDToIndexMap::iterator itEnd = mLocatorRefMap.end();

                if (it != itEnd)
                    mCurrentLocator = it->second;
            }
            break;

        case kTextureItem:
            if (strcmp(LXsGRAPH_SHADELOC, name) == 0)
            {
                // The first of these is the texture locator, the second the videoStill. See LxoImageMapItemChunk.
                // why on earth they didn't use a second name...

                tRefIDToIndexMap::iterator it = mTextureLocatorMap.find(refId);
                
                if (it != mTextureLocatorMap.end())
                {
                    mTextures.back().mTextureLocator = it->second;
                    return LXe_OK;
                }
                
                it = mImageMap.find(refId);
                
                if (it != mImageMap.end())
                {
                    mTextures.back().mImage = it->second;
                    return LXe_OK;
                }
            }
            break;
            
        default:
            break;
        }
        
        return LXe_OK;
    }

    LxResult ProcessItemChannelScalar (char* name, LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer, LxUShort length)
    {
        return LXe_OK;
    }
    
    LxResult ProcessItemChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal)
    {
//        fprintf (m_outFile, "Chan[%3d]: <%-16s>", index, GetChannelName (index));
        
//        if (type & LXItemType_Envelope)
//            fprintf (m_outFile, ", EnvelopeIndex: %d", envelopeIndex);
        
//        fprintf (m_outFile, " (0x%x) ", type);
//        PrintValue (type, intVal, floatVal, strVal);
        
        return LXe_OK;
    }
    
    LxResult ProcessItemChannelVector (char* name, LxUShort type, LxUShort count)
    {
        return LXe_OK;
    }
    
    LxResult ProcessItemChannelVectorValue (char* name, LxUShort type, int intVal, float floatVal, char* strVal)
    {
        // bleah, so-called vector values are an array of string/value items.
        if (type == LXItemType_Float && (isalpha(strVal[0])) && strVal[1] == 0)
        {
            char c = tolower(strVal[1]);

            int index = c - 'x';
            
            if (mCurrentLocator >= 0 && index >= 0 && index < 3)
            {
                cLocator& locator = mLocators[mCurrentLocator];

                switch (mItemType)
                {
                case kRotationItem:
                    locator.mRotation[index] = floatVal;
                    break;
                case kScaleItem:
                    locator.mScale[index] = floatVal;
                    break;
                case kTranslationItem:
                    locator.mTranslation[index] = floatVal;
                    break;
                
                default: break;
                }
            }
        }

        return LXe_OK;
    }
    
    LxResult ProcessItemChannelString(char* name, char* str)
    {
        switch (mItemType)
        {
        case kMorphItem:
            if (strcasecmp(name, LXsICHAN_MORPHDEFORM_MAPNAME) == 0)
            {
                tNameToIndexMap::iterator it = mNameToMorphIndexMap.find(str);
                
                if (it != mNameToMorphIndexMap.end())
                    mMorphRefToIndexMap[mItemRefID] = it->second;
                else
                    CL_LOG("ReadLXO", "Unknown morph map: %s\n", str);
            }
            break;

        case kTextureItem:
            if (strcmp(name, LXsICHAN_TEXTURELAYER_EFFECT) == 0)
            {
                mTextures.back().mKind = ParseEnum(kTextureEffectEnum, str, kMaxTextureKinds);
            }
            break;

        case kTextureLocatorItem:
            if (strcmp(name, LXsICHAN_TEXTURELOC_UVMAP) == 0)
            {
                tNameToIndexMap::iterator it = mNameToUVMap.find(str);
                
                if (it != mNameToUVMap.end())
                    mTextureLocators.back().mUVMap = it->second;
            }
            break;
        
        case kImageItem:
            if (strcmp(name, LXsICHAN_VIDEOSTILL_FILENAME) == 0)
            {
                mImages.back().mFileName = str;
            }
            break;
        
        default:
            break;
        }
        
        return LXe_OK;
    }
    
    virtual LxResult ProcessItemEnd()
    {
        mItemType = kMaxItemTypes;
        mCurrentLocator = -1;
        return LXe_OK;
    }
    
    
    
    // Animation

    LxResult ProcessEnvelope (LxULong index, LxULong type)
    {
        if (type == LXEnvelopeType_Float)
        {
            if (mEnvelopes.size() <= index)
                mEnvelopes.resize(index + 1);
            
            mCurrentEnvelope = index;
        }
        
        return LXe_OK;
    }

    LxResult ProcessEnvelopeUnknown ()
    {
        
        return LXe_OK;
    }

    LxResult ProcessEnvelopeBehavior (LxUShort flag)
    {
        if (mCurrentEnvelope < 0)
            return LXe_OK;

        if (m_subChunkId == 'PRE ')
            mEnvelopes[mCurrentEnvelope].mEndType[0] = flag;
        else
            mEnvelopes[mCurrentEnvelope].mEndType[1] = flag;
        
        // pre/post repeat
        return LXe_OK;
    }

    LxResult ProcessEnvelopeTanIn (LxUShort slopeType, LxUShort weightType, float slope, float weight, float value)
    {
        if (mCurrentEnvelope < 0)
            return LXe_OK;
        
        cKey& key = mEnvelopes[mCurrentEnvelope].mKeys.back();
        
        key.mSlopeType [0] = slopeType;
        key.mSlope     [0] = slope;
        key.mWeightType[0] = weightType;
        key.mWeight    [0] = weight;
        key.mValue2    [0] = value;
        
        return LXe_OK;
    }

    LxResult ProcessEnvelopeTanOut (LxULong breaks, LxUShort slopeType, LxUShort weightType, float slope, float weight, float value)
    {
        if (mCurrentEnvelope < 0)
            return LXe_OK;
        
        cKey& key = mEnvelopes[mCurrentEnvelope].mKeys.back();

        key.mBreakFlags = breaks;

        key.mSlopeType [1] = slopeType;
        key.mSlope     [1] = slope;
        key.mWeightType[1] = weightType;
        key.mWeight    [1] = weight;
        key.mValue2    [1] = value;
        
        return LXe_OK;
    }

    LxResult ProcessEnvelopeKey (float time, float value)
    {
        cKey newKey;
        
        newKey.mTime = time;
        newKey.mValue = value;
        
        mEnvelopes[mCurrentEnvelope].mKeys.push_back(newKey);
        
        return LXe_OK;
    }

    LxResult ProcessEnvelopeKey(float time, LxULong value)
    {
        return LXe_OK;
    }

    LxResult ProcessEnvelopeFlag (LxULong flag)
    {
        return LXe_OK;
    }
    
    LxResult ProcessEnvelopeEnd()
    {
        mCurrentEnvelope = -1;
        return LXe_OK;
    }



    // Animation 'envelopes' -- these define scalar channels

};



GLuint BuildMesh(cLXOMesh* model)
{
    GLuint meshName;
    
    // Create a vertex array object (VAO) to cache model parameters
    glGenVertexArrays(1, &meshName);
    glBindVertexArray(meshName);

    GLuint posBufferName;
    
    // Create a vertex buffer object (VBO) to store positions
    glGenBuffers(1, &posBufferName);
    glBindBuffer(GL_ARRAY_BUFFER, posBufferName);
    {
        glBufferData(GL_ARRAY_BUFFER, model->mPoints.size() * 12, model->mPoints.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(kVBPositions);
        
        glVertexAttribPointer
        (
            kVBPositions,
            3,
            GL_FLOAT,
            GL_FALSE,
            12,
            0
        );
        GL_CHECK;

        glEnableVertexAttribArray(kVBColours);
        
        glVertexAttribPointer
        (
            kVBColours,
            3,
            GL_FLOAT,
            GL_FALSE,
            12,
            0
        );
        GL_CHECK;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!model->mNormals.empty())
    {
        GLuint normalBufferName;
        
        glGenBuffers(1, &normalBufferName);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferName);
        {
            glBufferData(GL_ARRAY_BUFFER, model->mNormals.size() * 12, model->mNormals.data(), GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(kVBNormals);

            glVertexAttribPointer
            (
                kVBNormals,	// What attibute index will this array feed in the vertex shader (see buildProgram)
                3,          // How many elements are there per normal?
                GL_FLOAT,   // What is the type of this data?
                GL_FALSE,   // Do we want to normalize this data (0-1 range for fixed-pont types)
                12,
                0
            );	// What is the offset in the VBO to the normal data?

            GL_CHECK;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    if (!model->mTexCoords.empty())
    {
        GLuint texcoordBufferName;
        
        // Create a VBO to store texcoords
        glGenBuffers(1, &texcoordBufferName);
        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferName);
        {
            glBufferData(GL_ARRAY_BUFFER, model->mTexCoords.size() * 8, model->mTexCoords.data(), GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(kVBTexCoords);

            glVertexAttribPointer
            (
                kVBTexCoords,
                2,
                GL_FLOAT,
                GL_TRUE,
                8,
                0
            );

            GL_CHECK;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    GLuint elementBufferName;
    glGenBuffers(1, &elementBufferName);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferName);
    
    // Allocate and load vertex array element data into VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->mIndices.size() * sizeof(uint16_t), model->mIndices.data(), GL_STATIC_DRAW);

    GL_CHECK;

    glBindVertexArray(0);

    return meshName;
}

uint32_t nHL::LoadLXOScene(cGLMeshInfo* meshInfo, const char* fileName)
{
    LxResult result;

#ifdef LOCAL_DEBUG
    result = LxoDump((char*) fileName, NULL, false).ReadFile();

    if (LXe_OK != result)
        CL_LOG("ReadLXO", "Error (0x%x) dumping LXO file <%s>\n", result, fileName);
    else
        CL_LOG("ReadLXO", "Successfully dumped <%s>\n", fileName);
#endif

    cLXOMesh mesh(fileName);

    if (!mesh.IsLxoValid())
    {
        result = LXe_NOTFOUND;
        CL_LOG("ReadLXO_Error", "Error (0x%x) opening LXO file <%s>\n", result, fileName);
        return result;
    }

    result = mesh.ReadFile();

    if (LXe_OK != result)
        CL_LOG("ReadLXO_Error", "Error (0x%x) reading LXO file <%s>\n", result, fileName);
    else
    {
        CL_LOG("ReadLXO", "Successfully read <%s>\n", fileName);

        GL_CHECK;
        
        meshInfo->mMesh = BuildMesh(&mesh);
        meshInfo->mNumElts = mesh.mIndices.size();
        meshInfo->mEltType = GL_UNSIGNED_SHORT;

        cFileSpec textureSpec(fileName);

        for (size_t i = 0, n = mesh.mTextures.size(); i < n; i++)
            if (mesh.mTextures[i].mImage >= 0 && mesh.mTextures[i].mKind >= 0)
            {
                int k = mesh.mTextures[i].mKind;
                const cImageInfo& image = mesh.mImages[mesh.mTextures[i].mImage];

                textureSpec.SetRelativePath(image.mFileName.c_str());

                meshInfo->mTextures[k] = LoadTexture32(textureSpec);
            }

        GL_CHECK;
    }
    
    return result;
}

