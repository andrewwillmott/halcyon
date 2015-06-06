/*
 * Copyright (c) 2009 Luxology LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.   Except as contained
 * in this notice, the name(s) of the above copyright holders shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * ----------------------------------------------------------------
 * Class to read LXO files
 *
 */

#pragma once

#ifdef HL_FULL_LXO_DEPS
    #include "lxoIoDefs.hpp"
#else
    // Addition: just the defs we need so we're self-contained.
    #include <vector>
    #include <string>

    typedef     unsigned int                LxULong;
    typedef     unsigned short              LxUShort;
    typedef     unsigned char               LxByte;
    typedef     std::vector<std::string>    LXtStringVec;
    typedef     unsigned int                LXtID4;
    typedef     float                       LXtFVector[3];

    enum {              // Item Data Types
        LXItemType_Int              = 0x01,
        LXItemType_Float            = 0x02,
        LXItemType_String           = 0x03,
        LXItemType_Variable         = 0x04,
        LXItemType_Envelope         = 0x10,
        LXItemType_UndefState       = 0x20,     // undefined action channel
        
        LXItemType_EnvelopeInt      = LXItemType_Envelope | LXItemType_Int,
        LXItemType_EnvelopeFloat    = LXItemType_Envelope | LXItemType_Float,
        LXItemType_EnvelopeString   = LXItemType_Envelope | LXItemType_String,
        
        LXItemType_FloatAlt         = 0x4e56,   // compatibility with some apps
        
        LXEnvelopeType_Float        = 0,
        LXEnvelopeType_Int          = 1,
    };

    typedef unsigned int            LxResult;
    #define LXe_OK                  (LxResult)(0x0)
    #define LXe_FAILED              (LxResult)(0x80000000)
    #define LXxFAILCODE(m,v)        (LxResult)(LXe_FAILED | (((m) << 16) & 0x7FFF0000) | (v & 0xFFFF))
    #define LXxGOODCODE(m,v)        (LxResult)((((m) << 16) & 0x7FFF0000) | (v & 0xFFFF))
    #define LXe_OUTOFBOUNDS         LXxFAILCODE(0,21)
    #define LXe_WARNING             LXxGOODCODE(0,2)
    #define LXe_OUTOFMEMORY         LXxFAILCODE(0,2)
    #define LXe_INVALIDARG          LXxFAILCODE(0,3)
    #define LXe_NOACCESS            LXxFAILCODE(0,9)
    #define LXe_NOTFOUND            LXxFAILCODE(0,23)

    struct LXtLayerChunk;
    // End addition
#endif

typedef     struct {
                LxULong     polyIndex;
                LxUShort    tagIndex;
                } LXtPolyTag;

typedef     struct {
                LxULong     a, b, c;
                } LXtVertIndex;

typedef     struct {
                LxULong     nVerts, nTris, nVectors, nTags, unused;
                } LXtSufaceInfo;

typedef     struct {
                LxULong     nSurfs, triSurfIndex, unused;
                } LXtGroupInfo;

typedef enum {
            LXChunk_Process =  0,
            LXChunk_Ignore,
            LXChunk_Copy,
            } LXChunkUsage;

LXChunkUsage    LxpReaderChunkUsage (LXtID4 chunkId);   // choose which chunks to process/ignore in an LXP file

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * This class can be used to read data from an LXO file.  The default methods
 * only read the data into memory, but these can be overridden to enable
 * custom processing of the data.  One example would be to dump the contents
 * of the file in human readable form, another would be to import geometry
 * or materials into an application.
 *
 *----------------------------------------------------------------------------*/
class LxoReader {
        friend  class LxoCopy;

        FILE*                   m_inFile;		// file pointer for the LXO file
        LXtStringVec            m_channels;		// std::vector of the strings for each channel type

        LxResult		ReadShort  (LxUShort* val, LxULong count = 1);
        LxResult		ReadLong   (LxULong* val, LxULong count = 1);
        LxResult		ReadInt    (int* val, LxULong count = 1);
        LxResult		ReadIndex  (LxULong* val);
        LxResult		ReadIndex  (LxULong* val, LxULong count);
        LxResult		ReadFloat  (float*, LxULong count = 1);
        LxResult		ReadString (char* str, LxULong maxLen);
        LxResult		ReadValue  (LxUShort type, int* intValue, float* floatValue, char* str, LxULong strSize);

        LxResult		ReadHeader ();		// Read the header for a chunk
        LxResult		ReadSubHeader ();	// Read the header for a sub-chunk
        LxResult                ReadChunk ();           // Read & process the contents of a chunk
        LxResult                ReadItemSubChunk ();	// Read & process an 'ITEM' sub-chunk
        LxResult                ReadItemChunkData ();	// Read & process the contents of an 'ITEM' sub-chunk
        LxResult                ReadEnvelopeChunk ();	// Read & process an 'ENVL' sub-chunk
        LxResult		ReadActionChunk ();	// Read & process an 'ACTN' sub-chunk
        LxResult		ReadAudioChunk ();	// Read & process an 'AANI' audio sub-chunk

        /*
         * These virtual methods can be overridden to change the way chunks are processed.
         * Currently only overridden by LxoCopy to support LXChunk_Copy.
         */
        virtual LxResult        ProcessChunk (LXChunkUsage action);         // Process or Ignore the current chunk
        virtual LxResult        ProcessItemSubChunk (LXChunkUsage action);  // Process or Ignore the current item

    public:
        LxULong                 m_majorVersion;         // file version
        LXtID4                  m_chunkId;		// ID for the current chunk
        LxULong                 m_chunkSize;		// size of the current chunk
        LxULong                 m_unreadChunkBytes;     // number of bytes left to read in the chunk
        LxULong                 m_chunkFilePos;		// file position of chunk (in case it needs to be re-read)
        LXtID4                  m_subChunkId;		// ID for the current sub-chunk
        LxUShort                m_subChunkSize;		// size of the current sub-chunk
        LxUShort                m_unreadSubChunkBytes;  // number of bytes left to read in the sub-chunk
        LXtStringVec            m_tags;                 // std::vector of the poly tags in the file

         LxoReader (char const* lxoName);
        ~LxoReader ();

        bool		IsLxoValid ();                              // test if LXO file is valid
        LxResult	ReadFile ();                                // Read & process an entire LXO file
        LxResult	ReadBytes (LxByte* val, LxULong count);     // Read bytes from the file for blind copies
        char const *    GetChannelName (LxULong index);             // return channel name at index

        /*
         * These virtual methods can be overridden to change the way chunks are processed.
         */
        virtual LXChunkUsage    ChunkUsage () { return LXChunk_Process; };          // Default: Process the current chunk
        virtual LXChunkUsage    ItemSubChunkUsage () { return LXChunk_Process; };   // Default: Process the current item

        void                    SkipRestOfChunk ();                                 // skip remainder of chunk
        void                    SkipRestOfSubChunk ();                              // skip remainder of sub-chunk

        /*
         * These virtual methods can be overridden to process the chunk/header information.
         * For example, these methods could be used to print and/or import the various chunks.
         */
        virtual LxResult	ProcessHeader () { return LXe_OK; }

        virtual LxResult	ProcessUnknown () { return LXe_OK; }
        virtual LxResult	ProcessVersion (LxULong major, LxULong minor, char* str) { return LXe_OK; }
        virtual LxResult	ProcessParent (char* str) { return LXe_OK; }
        virtual LxResult	ProcessDescription (char* type, char* descr) { return LXe_OK; }
        virtual LxResult	ProcessChannelNames (LxULong count, LXtStringVec chans) { return LXe_OK; }
        virtual LxResult	ProcessBoundingBox (LXtFVector min, LXtFVector max) { return LXe_OK; }
        virtual LxResult	ProcessPolyTags (LXtID4 id, LxULong count, LXtPolyTag* ptags) { return LXe_OK; }
        virtual LxResult	ProcessTags (LXtStringVec tags) { return LXe_OK; }
        virtual LxResult	ProcessLayer (LXtLayerChunk& layer) { return LXe_OK; }
        virtual LxResult	ProcessSurfaceInfo (LXtSufaceInfo& surf) { return LXe_OK; }
        virtual LxResult	ProcessGroupInfo (LXtGroupInfo& group) { return LXe_OK; }
        virtual LxResult        ProcessPoints (LxULong count, LXtFVector* points) { return LXe_OK; }
        virtual LxResult        ProcessTriangles (LxULong count, LXtVertIndex* tris) { return LXe_OK; }
        virtual LxResult	ProcessTextTag (LXtID4 id, char* str) { return LXe_OK; }
        virtual LxResult	ProcessPolygon (LXtID4 type, LxUShort nVerts, LxULong* verts) { return LXe_OK; }
        virtual LxResult	ProcessVectors (LXtID4 id, char* name, LxULong nVectors, LxULong nComponents, float* coords) { return LXe_OK; }
        virtual LxResult	ProcessVertexMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents) { return LXe_OK; }
        virtual LxResult	ProcessVertexMapFlags (LxULong flags[2]) { return LXe_OK; }
        virtual LxResult	ProcessVertexMapEntry (LxULong nComponents, LxULong vertIndex, float* values) { return LXe_OK; }
        virtual LxResult	ProcessVertexDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values) { return LXe_OK; }
        virtual LxResult	ProcessEdgeMap (LXtID4 id, char* name, LxULong nMaps, LxULong nComponents) { return LXe_OK; }
        virtual LxResult	ProcessEdgeDMapEntry (LxULong nComponents, LxULong vertIndex, LxULong polyIndex, float* values) { return LXe_OK; }
        virtual LxResult	ProcessPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData) { return LXe_OK; }
        virtual LxResult	ProcessThumbnail (LxUShort width, LxUShort height, LxByte nChannels, LxByte flags, LxULong dataBytes, LxByte* imageData) { return LXe_OK; }
        virtual LxResult	ProcessBake (LxULong refId, LxULong samples, float startTime, float sampsPerSec) { return LXe_OK; }

        virtual LxResult	ProcessItem (char* itemType, char* name, LxULong refId) { return LXe_OK; }
        virtual LxResult	ProcessItemEnd () { return LXe_OK; }
        virtual LxResult	ProcessItemUnknown () { return LXe_OK; }
        virtual LxResult	ProcessItemXref (LxULong index, char* filename, char* idStr) { return LXe_OK; }
        virtual LxResult	ProcessItemLayer (LxULong index, LxULong flags, LxULong color) { return LXe_OK; }
        virtual LxResult	ProcessItemId (char* name) { return LXe_OK; }
        virtual LxResult	ProcessItemIndex (LxULong index) { return LXe_OK; }
        virtual LxResult	ProcessItemLink (char* name, LxULong refId, LxULong index) { return LXe_OK; }
        virtual LxResult	ProcessItemBoundingBox (LXtFVector min, LXtFVector max) { return LXe_OK; }
        virtual LxResult	ProcessChannelLink (char* name, char* fromChannel, LxULong index, char* toChannel, LxULong fromIndex, LxULong toIndex) { return LXe_OK; }
        virtual LxResult	ProcessItemPackage (char* name, LxULong bytes, LxByte* buffer) { return LXe_OK; }
        virtual LxResult	ProcessItemGradient (char* name, LxULong index, LxULong flags, char* inType, char* outType) { return LXe_OK; }
        virtual LxResult	ProcessItemTag (LXtID4 tagType, char* value) { return LXe_OK; }
        virtual LxResult	ProcessItemPreview (LxUShort width, LxUShort height, LxULong type, LxULong flags, LxULong dataBytes, LxByte* imageData) { return LXe_OK; }

        virtual LxResult	ProcessItemChannelScalar (char* name, LxUShort type, int intVal, float floatVal, char* strVal, LxByte* buffer, LxUShort length) { return LXe_OK; }
        virtual LxResult	ProcessItemChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal) { return LXe_OK; }
        virtual LxResult	ProcessItemChannelVector (char* name, LxUShort type, LxUShort count) { return LXe_OK; }
        virtual LxResult	ProcessItemChannelVectorValue (char* name, LxUShort type, int intVal, float floatVal, char* strVal) { return LXe_OK; }
        virtual LxResult	ProcessItemChannelString (char* name, char* str) { return LXe_OK; }
        virtual LxResult	ProcessCustomChannel (char* name, char* str, LxULong dataBytes, LxByte* customData) { return LXe_OK; }
        virtual LxResult	ProcessUserChannel (char* name, char* str) { return LXe_OK; }

        virtual LxResult	ProcessEnvelope (LxULong index, LxULong type) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeUnknown () { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeEnd () { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeBehavior (LxUShort flag) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeTanIn (LxUShort slopeType, LxUShort weightType, float slope, float weight, float value) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeTanOut (LxULong breaks, LxUShort slopeType, LxUShort weightType, float slope, float weight, float value) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeKey (float time, float value) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeKey (float time, LxULong value) { return LXe_OK; }
        virtual LxResult	ProcessEnvelopeFlag (LxULong flag) { return LXe_OK; }

        virtual LxResult	ProcessAction (char* actionType, char* name, LxULong refId) { return LXe_OK; }
        virtual LxResult	ProcessActionUnknown () { return LXe_OK; }
        virtual LxResult	ProcessActionParent (LxULong index) { return LXe_OK; }
        virtual LxResult	ProcessActionItem (LxULong index) { return LXe_OK; }
        virtual LxResult	ProcessActionChannelGeneral (LxULong index, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal) { return LXe_OK; }
        virtual LxResult	ProcessActionChannelName (char* name, LxUShort type, LxULong envelopeIndex, int intVal, float floatVal, char* strVal) { return LXe_OK; }
        virtual LxResult	ProcessActionChannelString (char* name, char* str) { return LXe_OK; }
        virtual LxResult	ProcessActionGradient (LxULong index, char *name, LxULong envelopeIndex, LxULong flags) { return LXe_OK; }

        virtual LxResult	ProcessAudio () { return LXe_OK; }
        virtual LxResult	ProcessAudioItem (LxULong index) { return LXe_OK; }
        virtual LxResult	ProcessAudioSettings (LxUShort loop, LxUShort mute, LxUShort scrub, float start) { return LXe_OK; }
};

