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

#include <stdio.h>
#include "lxoReader.hpp"

typedef union {
        float           f;
        LxULong         u;
        LxByte          b[4];
        } Union4Bytes;

typedef union {
        LxUShort        u;
        LxByte          b[2];
        } Union2Bytes;

enum    {
        MAX_StackBytes  = 100000,       // max size to use for alloca
        };

static bool s_useMalloc;                // used by FREEA macro - only safe because used in non-recursive ReadChunk()

#define ALLOCA(bytes)       ((s_useMalloc = bytes > MAX_StackBytes) ? malloc (bytes) : alloca (bytes))
#define FREEA(mem)          if (s_useMalloc) free (mem)

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Values are stored in the file in big-endian order; must be byte reversed
 * for Intel (little-endian) architectures
 *
 *----------------------------------------------------------------------------*/
static void swapBytes (Union4Bytes* val)
{
    Union4Bytes     temp;

    temp.u = val->u;

    val->b[0] = temp.b[3];
    val->b[1] = temp.b[2];
    val->b[2] = temp.b[1];
    val->b[3] = temp.b[0];
}

static void swapBytes (Union2Bytes* val)
{
    Union2Bytes     temp;

    temp.u = val->u;

    val->b[0] = temp.b[1];
    val->b[1] = temp.b[0];
}

static void swapBytes (float& val)      { swapBytes ((Union4Bytes*)&val); }
static void swapBytes (LxULong& val)    { swapBytes ((Union4Bytes*)&val); }
static void swapBytes (LxUShort& val)   { swapBytes ((Union2Bytes*)&val); }

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Constructor for LXO file reader validates & opens the file
 *
 *----------------------------------------------------------------------------*/
LxoReader::LxoReader (char const* lxoName)
{
    m_inFile        = NULL;
    m_majorVersion  = 0;

    m_inFile = fopen (lxoName, "rb");

#if 0
    if (NULL == (m_inFile = fopen (lxoName, "rb")))
        printf ("Error reading LXO file <%s>\n", lxoName);
#endif
}

LxoReader::~LxoReader ()
{
    if (m_inFile)
        fclose (m_inFile);
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * This method id provided so the caller can test if the LXO file is valid
 * after instantiating the LxoReader class (or subclass), without making
 * the file pointer public (m_inFile).
 *
 *----------------------------------------------------------------------------*/
    bool
LxoReader::IsLxoValid ()
{
    return m_inFile != NULL;
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * This method id provided so the caller can retrieve the name associated
 * with the channel at a given index, so m_channels can be private.
 *
 *----------------------------------------------------------------------------*/
        char const *
LxoReader::GetChannelName (LxULong index)
{
    return m_channels[index].c_str();
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * Skip over any unprocessed data in a chunk; typically when ChunkUsage
 * returns LXChunk_Ignore, or if the Process function doesn't care about
 * the rest of the data in the chunk.
 *
 *----------------------------------------------------------------------------*/
        void
LxoReader::SkipRestOfChunk ()
{
    if (m_unreadChunkBytes > 0)
        fseek (m_inFile, m_unreadChunkBytes, SEEK_CUR);     // skip over any unread bytes in chunk

    m_unreadChunkBytes = 0;
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * Skip over any unprocessed data in a chunk; typically when SubChunkUsage
 * returns LXChunk_Ignore, or if the Process function doesn't care about
 * the rest of the data in the sub-chunk.
 *
 *----------------------------------------------------------------------------*/
        void
LxoReader::SkipRestOfSubChunk ()
{
    fseek (m_inFile, m_unreadSubChunkBytes, SEEK_CUR);  // skip over any unread bytes in this sub-chunk
    m_unreadChunkBytes -= m_unreadSubChunkBytes;
    m_unreadSubChunkBytes = 0;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read chunk header, containing the 4 byte ID & chunk size.  Assume there is
 * no sub-chunk, and make the sub-chunk & chunk size the same.
 *
 * Call virtual method for any additional header processing.
 *
 * return LXe_OK on successful read, LXe_WARNING when no more chunks in the file;
 * or other errors as necessary.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadHeader ()
{
        if (NULL == m_inFile)
            return LXe_OUTOFBOUNDS;

        m_chunkFilePos = ftell (m_inFile);      // record file pos of this chunk

        if (1 != fread (&m_chunkId, sizeof m_chunkId, 1, m_inFile))
            return LXe_WARNING;

        if (1 != fread (&m_chunkSize, sizeof m_chunkSize, 1, m_inFile))
            return LXe_OUTOFBOUNDS;

        swapBytes (m_chunkId);
        swapBytes (m_chunkSize);

        m_unreadChunkBytes = m_chunkSize;   // counter to know how many bytes are left to read

        return ProcessHeader ();
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read smaller sub-chunk header, containing the 4 byte ID & 2 byte chunk size.
 *
 * return LXe_OK on successful read, LXe_WARNING when no more chunks in the file;
 * or other errors as necessary.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadSubHeader ()
{
        if (1 != fread (&m_subChunkId, sizeof m_subChunkId, 1, m_inFile))
            return LXe_WARNING;

        if (1 != fread (&m_subChunkSize, sizeof m_subChunkSize, 1, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= (sizeof m_subChunkId + sizeof m_subChunkSize);
        swapBytes (m_subChunkId);
        swapBytes (m_subChunkSize);

        m_unreadSubChunkBytes = m_subChunkSize; // counter to know how many bytes are left to read

        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read a number of bytes, with no byte swapping.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadBytes (LxByte* val, LxULong count)
{
        if (NULL == m_inFile || m_unreadChunkBytes < count || count != fread (val, 1, count, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= count;
        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read any number of values, swapping bytes from big-endian to little-endian.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadShort (LxUShort* val, LxULong count)
{
        if (NULL == m_inFile || m_unreadChunkBytes < sizeof *val || count != fread (val, sizeof *val, count, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= count * sizeof *val;

        while (count-- > 0)
            swapBytes (*val++);

        return LXe_OK;
}

        LxResult
LxoReader::ReadLong (LxULong* val, LxULong count)
{
        if (0 == count)
            return LXe_OK;

        if (NULL == m_inFile || m_unreadChunkBytes < sizeof *val || count != fread (val, sizeof *val, count, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= count * sizeof *val;

        while (count-- > 0)
            swapBytes (*val++);

        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read any number of floats, using the 4-byte read to byte-swap.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadFloat (float* val, LxULong count)
{
        return ReadLong ((LxULong*)val, count);
}

        LxResult
LxoReader::ReadInt (int* val, LxULong count)
{
        return ReadLong ((LxULong*)val, count);
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read a Variable length index, used to represent an index into a list.
 * First read 2 bytes as an LxUShort. If the high byte of the index is not
 * 0xFF, use this value.  If the first byte is 0xFF, then read the next 2
 * bytes, forming a LxULong, but zero out the high byte (0xFF).
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadIndex (LxULong* value)
{
        Union2Bytes     val1, val2;
        Union4Bytes*    out = (Union4Bytes*)value;

        if (NULL == m_inFile || m_unreadChunkBytes < 2 || 1 != fread (&val1.u, 2, 1, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= 2;

        if (0xFF != val1.b[0])
            {
            out->b[3] = out->b[2] = 0;
            out->b[1] = val1.b[0];
            out->b[0] = val1.b[1];

            return LXe_OK;
            }

        // index is bigger than 0xFF00, so read the next short and combine the values
        if (m_unreadChunkBytes < 2 || 1 != fread (&val2.u, 2, 1, m_inFile))
            return LXe_OUTOFBOUNDS;

        m_unreadChunkBytes -= 2;

        out->b[3] = 0;              // ignore the high byte (which was 0xFF)
        out->b[2] = val1.b[1];
        out->b[1] = val2.b[0];
        out->b[0] = val2.b[1];

        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 10/11
 *
 * Read a number of variable-length indices
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadIndex (LxULong* value, LxULong count)
{
        while (count-- > 0)
            if (LXe_OK != ReadIndex (value++))
                return LXe_OUTOFBOUNDS;

        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read a null-terminated string.  If end-of-file is reached before the NULL
 * terminator, returns LXe_OUTOFBOUNDS.
 *
 * If the max length is reached before the end of the string, terminate the
 * string in the buffer, but keep reading the file until the end of the string,
 * so that the next read will occur in the correct file position.
 *
 * Note: all strings in the file are padded to an even number of bytes.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadString (char* val, LxULong maxLen)
{
        if (NULL == m_inFile)
            return LXe_OUTOFBOUNDS;

        val[maxLen - 1] = '\0';     // terminate the string in case we run out of room

        LxULong     byteCount = 0;

        while (true)
            {
            char    c = fgetc (m_inFile);   // read the next char from the file

            if (EOF == c)
                return LXe_OUTOFBOUNDS;

            if (++byteCount < maxLen)       // make sure there's enough room in the caller's buffer
                *val++ = c;

            if ('\0' == c)                  // reached the end of the string
                {
                if (byteCount & 1)                  // if there are an odd number of bytes in the string...
                    ++byteCount, fgetc (m_inFile);  // ...read one more - must be even number of bytes!
                break;
                }
            }

        m_unreadChunkBytes -= byteCount;

        return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read a value based on the type passed
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadValue (LxUShort type, int* intValue, float* floatValue, char* str, LxULong strSize)
{
    switch (type & ~LXItemType_UndefState)
        {
        case LXItemType_Int:
        case LXItemType_EnvelopeInt:
            return ReadInt (intValue);

        case LXItemType_Float:
        case LXItemType_FloatAlt:
        case LXItemType_EnvelopeFloat:
            return ReadFloat (floatValue);

        case LXItemType_String:
        case LXItemType_EnvelopeString:
            return ReadString (str, strSize);
        }

    return LXe_INVALIDARG;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read entire LXO file, one chunk at a time
 *
 * If ProcessChunk or ReadHeader returns LXe_WARNING, then there are no more
 * chunks in the file, so return success.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadFile ()
{
    LxResult        result;

    if (! IsLxoValid ())
        return LXe_NOACCESS;

    do  {
        if (LXe_OK != (result = ReadHeader ()))
            break;

        if ('FORM' == m_chunkId)
            m_unreadChunkBytes = 4;     // header is for the whole file; just skip the next 4 byte identifier
        else
            result = ProcessChunk (ChunkUsage ());      // perform the requested operation on the chunk

        if (LXe_OK == result && m_unreadChunkBytes > 0)
            SkipRestOfChunk ();                         // skip over any unread bytes in chunk

        } while (LXe_OK == result);

    return LXe_WARNING == result ? LXe_OK : result;     // LXe_WARNING indicates end of file
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * Process the chunk based on the action specified: Process or Ignore.
 * LxoReader does not support the Copy action.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ProcessChunk (LXChunkUsage action)
{
    if (LXChunk_Process == action)
        return ReadChunk ();

    return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read the item sub-chunks.
 * Call virtual method for the chunk for any additional application processing.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadItemChunkData (void)
{
        /*
         * All the 'read' functions decrement m_unreadChunkBytes.  Keep track of the
         * number of bytes read out of the actual sub-chunk as a sanity check.
         */

#define END_OF_SUBCHUNK     (startingChunkBytes - m_unreadChunkBytes >= m_unreadSubChunkBytes)

        LxResult    result;
        LxULong     startingChunkBytes  = m_unreadChunkBytes;

        LxULong     index, envelopeIndex = 0, flags, color, ref, bytes;
        LxUShort    type, length, count;
        LXtFVector  min, max;
        int         intValue;
        float       floatValue;
        char        str[1024], name[1024];
        LxByte*     buffer = NULL;

        switch (m_subChunkId)
            {
            case 'XREF':        // External reference item chunk
                if (LXe_OK == (result = ReadLong   (&index)) &&
                    LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadString (str, sizeof str)))
                        result = ProcessItemXref (index, name, str);
                break;

            case 'LAYR':        // Optional layer info sub-chunk
                if (LXe_OK == (result = ReadLong   (&index)) &&
                    LXe_OK == (result = ReadLong   (&flags)) &&
                    LXe_OK == (result = ReadLong   (&color)))
                        result = ProcessItemLayer (index, flags, color);
                break;

            case 'UNIQ':        // Optional unique identifier sub-chunk
                if (LXe_OK == (result = ReadString (str, sizeof str)))
                    result = ProcessItemId (str);
                break;

            case 'UIDX':        // Unique Item Index sub-chunk
                if (LXe_OK == (result = ReadLong (&index)))
                    result = ProcessItemIndex (index);
                break;

            case 'LINK':        // Item linking sub-chunk
                if (LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadLong   (&ref)) &&
                    LXe_OK == (result = ReadLong   (&index)))
                        result = ProcessItemLink (name, ref, index);
                break;

            case 'BBOX':       // Bounding Box

                if (LXe_OK == (result = ReadFloat ((float*)min, 3)) &&
                    LXe_OK == (result = ReadFloat ((float*)max, 3)))
                        result = ProcessItemBoundingBox (min, max);
                break;

            case 'CLNK': {      // ChannelRef link sub-chunk
                char        toChannel[1024];
                LxULong     fromIndex, toIndex;

                if (LXe_OK == (result = ReadString (name, sizeof name)) &&              // graph name
                    LXe_OK == (result = ReadString (str, sizeof str)) &&                // from channel
                    LXe_OK == (result = ReadLong   (&index)) &&                         // to item
                    LXe_OK == (result = ReadString (toChannel, sizeof toChannel)) &&    // to channel
                    LXe_OK == (result = ReadLong   (&fromIndex)) &&                     // 'from' Hyper-Graph link index
                    LXe_OK == (result = ReadLong   (&toIndex)))                         // 'to' Hyper-Graph link index
                        result = ProcessChannelLink (name, str, index, toChannel, fromIndex, toIndex);
                break;
                }

            case 'PAKG':        // Packages sub-chunk
                if (LXe_OK != (result = ReadString (str, sizeof str)) ||
                    LXe_OK != (result = ReadLong   (&bytes)))
                        return result;

                if (bytes > 0)
                    {
                    if (NULL == (buffer = (LxByte*)alloca (bytes)))
                        return LXe_OUTOFMEMORY;

                    if (LXe_OK != (result = ReadBytes (buffer, bytes)))
                        return result;
                    }

                result = ProcessItemPackage (str, bytes, buffer);
                break;

            case 'GRAD': {      // Gradient channel value sub-chunk (not all fields in all versions)

                char        *inTypeP = NULL, *outTypeP = NULL;
                char        inType[1024] = "", outType[1024] = "";
                flags = 0;

                if (LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadIndex  (&index)) &&
                    (END_OF_SUBCHUNK || LXe_OK == (result = ReadLong   (&flags))) &&
                    (END_OF_SUBCHUNK || LXe_OK == (result = ReadString (inTypeP  = inType,  sizeof inType))) &&
                    (END_OF_SUBCHUNK || LXe_OK == (result = ReadString (outTypeP = outType, sizeof outType))))
                        result = ProcessItemGradient (name, index, flags, inTypeP, outTypeP);
                break;
                }

            case 'ITAG': {      // Item tag sub-chunk
                LXtID4      tagType;

                if (LXe_OK == (result = ReadLong   (&tagType)) &&
                    LXe_OK == (result = ReadString (str, sizeof str)))
                        result = ProcessItemTag (tagType, str);
                break;
                }

            case 'CHNL':        // Scalar channel value sub-chunk
                if (LXe_OK != (result = ReadString (name, sizeof name)) ||
                    LXe_OK != (result = ReadShort  (&type)))
                        return result;

                if (LXItemType_Variable == type)
                    {
                    if (LXe_OK != (result = ReadShort (&length)))
                        return result;

                    if (length > 0)
                        {
                        if (NULL == (buffer = (LxByte*)alloca (length)))
                            return LXe_OUTOFMEMORY;

                        if (LXe_OK != (result = ReadBytes (buffer, length)))
                            return result;
                        }
                    }

                else if (LXe_OK != (result = ReadValue (type, &intValue, &floatValue, str, sizeof str)))
                        return result;

                result = ProcessItemChannelScalar (name, type, intValue, floatValue, str, buffer, length);
                break;

            case 'CHAN':        // Generalized channel value sub-chunk
                if (LXe_OK == (result = ReadIndex (&index)) &&
                    LXe_OK == (result = ReadShort (&type))  &&
                    (!(type & LXItemType_Envelope) || LXe_OK == (result = ReadIndex (&envelopeIndex))) &&
                    LXe_OK == (result = ReadValue (type, &intValue, &floatValue, str, sizeof str)))
                        result = ProcessItemChannelGeneral (index, type, envelopeIndex, intValue, floatValue, str);
                break;

            case 'CHNV':        // Vector channel value sub-chunk
                if (LXe_OK != (result = ReadString (name, sizeof name)) ||
                    LXe_OK != (result = ReadShort  (&type)) ||
                    LXe_OK != (result = ReadShort  (&count)) ||
                    LXe_OK != (result = ProcessItemChannelVector (name, type, count)))
                        return result;

                while (count-- > 0)
                    if (LXe_OK != (result = ReadString (name, sizeof name)) ||
                        LXe_OK != (result = ReadValue (type, &intValue, &floatValue, str, sizeof str)) ||
                        LXe_OK != (result = ProcessItemChannelVectorValue (name, type, intValue, floatValue, str)))
                            return result;
                break;

            case 'CHNS':        // String channel value sub-chunk
                if (LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadString (str, sizeof str)))
                        result = ProcessItemChannelString (name, str);
                break;

            case 'CHNC': {      // Custom channel value sub-chunk
                if (LXe_OK != (result = ReadString (name, sizeof name)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                if ((bytes = m_unreadSubChunkBytes - (startingChunkBytes - m_unreadChunkBytes)) <= 0)
                    break;

                LxByte*     customData = (LxByte*)alloca (bytes);
                if (NULL == customData)
                    return LXe_OUTOFMEMORY;

                if (LXe_OK == (result = ReadBytes (customData, bytes)))
                    result = ProcessCustomChannel (name, str, bytes, customData);

                break;
                }

            case 'UCHN':        // User channel sub-chunk
                if (LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadString (str, sizeof str)))
                        result = ProcessUserChannel (name, str);
                break;

            case 'PRVW': {                          // Item preview sub-chunk
                LxUShort    width, height;
                LxULong     type,  flags, bytes;

                if (LXe_OK != (result = ReadShort (&width)) ||
                    LXe_OK != (result = ReadShort (&height)) ||
                    LXe_OK != (result = ReadLong  (&type)) ||
                    LXe_OK != (result = ReadLong  (&flags)))
                        break;

                if ((bytes = m_unreadChunkBytes) <= 0)
                    break;

                LxByte*     imageData = (LxByte*)alloca (bytes);
                if (NULL == imageData)
                    return LXe_OUTOFMEMORY;

                if (LXe_OK == (result = ReadBytes (imageData, bytes)))
                    result = ProcessItemPreview (width, height, type, flags, bytes, imageData);

                break;
                }

            default:
                result = ProcessItemUnknown ();
                break;
            }

        // decrement bytes remaining in sub-chunk by the number of bytes read from the chunk
        m_unreadSubChunkBytes -= (startingChunkBytes - m_unreadChunkBytes);

        return result;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read the item sub-chunks.
 * Call virtual method for the chunk for any additional application processing.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadItemSubChunk (void)
{
    LxResult    result = ReadSubHeader ();

    if (LXe_OK != result)
        return result;

    if (LXe_OK != (result = ProcessItemSubChunk (ItemSubChunkUsage ())))    // perform the requested operation on the item
        return result;

#if 0
if (LXe_OK == result && m_unreadSubChunkBytes > 0 && m_subChunkId != 'UCHN')
printf("chunk %c%c%c%c :: %d unread item chunk bytes!!!!\n",
       ((char*)&m_subChunkId)[3], ((char*)&m_subChunkId)[2],((char*)&m_subChunkId)[1],((char*)&m_subChunkId)[0],
       m_unreadSubChunkBytes);
#endif

    if (LXe_OK == result && m_unreadSubChunkBytes > 0)
        SkipRestOfSubChunk ();                          // skip over any unread bytes in this sub-chunk

    return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * Process the item chunk based on the action specified: Process or Ignore.
 * LxoReader does not support the Copy action.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ProcessItemSubChunk (LXChunkUsage action)
{
    if (LXChunk_Process == action)
        return ReadItemChunkData ();

    return LXe_OK;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read the envelope sub-chunks.
 * Call virtual method for the chunk for any additional application processing.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadEnvelopeChunk (void)
{
        LxResult    result = ReadSubHeader ();

        if (LXe_OK != result)
            return result;

        /*
         * All the 'read' functions decrement m_unreadChunkBytes.  Keep track of the
         * number of bytes read out of the actual sub-chunk as a sanity check.
         */
        LxULong     startingChunkBytes  = m_unreadChunkBytes;

        switch (m_subChunkId)
            {
            case 'TANI': {      // In-coming tangent sub-chunk
                LxUShort    slopeType, weightType;
                float       slope, weight, value;

                if (LXe_OK == (result = ReadShort (&slopeType)) &&
                    LXe_OK == (result = ReadShort (&weightType)) &&
                    LXe_OK == (result = ReadFloat (&slope)) &&
                    LXe_OK == (result = ReadFloat (&weight)) &&
                    LXe_OK == (result = ReadFloat (&value)))
                        result = ProcessEnvelopeTanIn (slopeType, weightType, slope, weight, value);

                break;
                }

            case 'TANO': {      // Out-going tangent sub-chunk
                LxULong     breaks;
                LxUShort    slopeType, weightType;
                float       slope, weight, value;

                if (LXe_OK == (result = ReadLong  (&breaks)) &&
                    LXe_OK == (result = ReadShort (&slopeType)) &&
                    LXe_OK == (result = ReadShort (&weightType)) &&
                    LXe_OK == (result = ReadFloat (&slope)) &&
                    LXe_OK == (result = ReadFloat (&weight)) &&
                    LXe_OK == (result = ReadFloat (&value)))
                        result = ProcessEnvelopeTanOut (breaks, slopeType, weightType, slope, weight, value);

                break;
                }

            case 'KEY ': {      // Keyframe value (float) and time sub-chunk
                float       time, value;

                if (LXe_OK == (result = ReadFloat (&time)) &&
                    LXe_OK == (result = ReadFloat (&value)))
                        result = ProcessEnvelopeKey (time, value);

                break;
                }

            case 'IKEY': {      // Keyframe value (int) and time sub-chunk
                float       time;
                LxULong     value;

                if (LXe_OK == (result = ReadFloat (&time)) &&
                    LXe_OK == (result = ReadLong  (&value)))
                        result = ProcessEnvelopeKey (time, value);

                break;
                }

            case 'FLAG': {      // Keyframe flags
                LxULong     flag;

                if (LXe_OK == (result = ReadLong (&flag)))
                    result = ProcessEnvelopeFlag (flag);

                break;
                }

            case 'PRE ':        // Behavior before the first keyframe
            case 'POST': {      // Behavior after the last keyframe
                LxUShort    flag;

                if (LXe_OK == (result = ReadShort (&flag)))
                    result = ProcessEnvelopeBehavior (flag);

                break;
                }

            default:
                result = ProcessEnvelopeUnknown ();
                break;
            }

        // decrement bytes remaining in sub-chunk by the number of bytes read from the chunk
        m_unreadSubChunkBytes -= (startingChunkBytes - m_unreadChunkBytes);

        if (LXe_OK == result && m_unreadSubChunkBytes > 0)
            SkipRestOfSubChunk ();                          // skip over any unread bytes in this sub-chunk

        return result;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read the action sub-chunks.
 * Call virtual method for the chunk for any additional application processing.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadActionChunk (void)
{
        LxResult    result = ReadSubHeader ();

        if (LXe_OK != result)
            return result;

        /*
         * All the 'read' functions decrement m_unreadChunkBytes.  Keep track of the
         * number of bytes read out of the actual sub-chunk as a sanity check.
         */
        LxULong     startingChunkBytes  = m_unreadChunkBytes;
        LxULong     index, flags, envelopeIndex = 0;
        LxUShort    type;
        int         intValue;
        float       floatValue;
        char        str[1024], name[1024] = "";

        switch (m_subChunkId)
            {
            case 'PRNT':        // Parent sub-chunk
                if (LXe_OK == (result = ReadLong (&index)))
                    result = ProcessActionParent (index);
                break;

            case 'ITEM':        // Item sub-chunk
                if (LXe_OK == (result = ReadLong (&index)))
                    result = ProcessActionItem (index);
                break;

            case 'CHAN':        // Channel value sub-chunk
                if (LXe_OK == (result = ReadIndex (&index)) &&
                    LXe_OK == (result = ReadShort (&type))  &&
                    (!(type & LXItemType_Envelope) || LXe_OK == (result = ReadIndex (&envelopeIndex))) &&
                    LXe_OK == (result = ReadValue (type & 3, &intValue, &floatValue, str, sizeof str)))
                        result = ProcessActionChannelGeneral (index, type, envelopeIndex, intValue, floatValue, str);
                break;

            case 'CHNN':        // Named channel value sub-chunk
                if (LXe_OK == (result = ReadString (name, sizeof name)) &&
                    LXe_OK == (result = ReadShort  (&type))  &&
                    (!(type & LXItemType_Envelope) || LXe_OK == (result = ReadIndex (&envelopeIndex))) &&
                    LXe_OK == (result = ReadValue  (type & 3, &intValue, &floatValue, str, sizeof str)))
                        result = ProcessActionChannelName (name, type, envelopeIndex, intValue, floatValue, str);
                break;

            case 'CHNS':        // String channel value sub-chunk
                if (LXe_OK != (result = ReadString (name, sizeof name)) ||
                    LXe_OK != (result = ReadIndex  (&index)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                    return result;

                // use index or name string, depending on whether string is NULL
                if ('\0' == name[0])
                    result = ProcessActionChannelGeneral (index, LXItemType_String, 0, 0, 0, str);
                else
                    result = ProcessActionChannelString (name, str);
                break;

            case 'GRAD':        // Gradient envelope value sub-chunk (not all fields in all versions)
                if (LXe_OK == (result = ReadIndex  (&index)) &&
                    LXe_OK == (result = ReadIndex  (&envelopeIndex)) &&
                    LXe_OK == (result = ReadLong   (&flags)) &&
                    (END_OF_SUBCHUNK || LXe_OK == (result = ReadString (name, sizeof name))))
                        result = ProcessActionGradient (index, name, envelopeIndex, flags);
                break;

            default:
                result = ProcessActionUnknown ();
                break;
            }

        // decrement bytes remaining in sub-chunk by the number of bytes read from the chunk
        m_unreadSubChunkBytes -= (startingChunkBytes - m_unreadChunkBytes);

        if (LXe_OK == result && m_unreadSubChunkBytes > 0)
            SkipRestOfSubChunk ();                          // skip over any unread bytes in this sub-chunk

        return result;
}

/*------------------------------- Luxology LLC --------------------------- 07/12
 *
 * Read the audio sub-chunks.
 * Call virtual method for the chunk for any additional application processing.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadAudioChunk (void)
{
        LxResult    result = ReadSubHeader ();

        if (LXe_OK != result)
            return result;

        /*
         * All the 'read' functions decrement m_unreadChunkBytes.  Keep track of the
         * number of bytes read out of the actual sub-chunk as a sanity check.
         */
        LxULong     startingChunkBytes  = m_unreadChunkBytes;
        LxULong     index;
        LxUShort    loop, mute, scrub;
        float       start;

        switch (m_subChunkId)
            {
            case 'AAIT':        // Audio Item sub-chunk
                if (LXe_OK == (result = ReadLong (&index)))
                    result = ProcessAudioItem (index);
                break;

            case 'AASE':        // Audio Settings sub-chunk
                if (LXe_OK == (result = ReadShort (&loop)) &&
                    LXe_OK == (result = ReadShort (&mute)) &&
                    LXe_OK == (result = ReadShort (&scrub)) &&
                    LXe_OK == (result = ReadFloat (&start)))
                        result = ProcessAudioSettings (loop, mute, scrub, start);
                break;

            default:
                result = ProcessActionUnknown ();
                break;
            }

        // decrement bytes remaining in sub-chunk by the number of bytes read from the chunk
        m_unreadSubChunkBytes -= (startingChunkBytes - m_unreadChunkBytes);

        if (LXe_OK == result && m_unreadSubChunkBytes > 0)
            SkipRestOfSubChunk ();                          // skip over any unread bytes in this sub-chunk

        return result;
}

/*------------------------------- Luxology LLC --------------------------- 03/09
 *
 * Read the body of the current chunk in the LXO file
 * Call virtual method for the chunk for any additional application processing.
 *
 * Note that not ALL chunks are included here; additional ones may be added as needed.
 *
 *----------------------------------------------------------------------------*/
        LxResult
LxoReader::ReadChunk ()
{
        LxResult    result = LXe_OK;
        LXtID4      id;
        LxULong     count;
        char        str[512], str2[512];

        switch (m_chunkId)
            {
            case 'VRSN': {                          // Version chunk
                LxULong     minor;

                if (LXe_OK == (result = ReadLong   (&m_majorVersion)) &&
                    LXe_OK == (result = ReadLong   (&minor)) &&
                    LXe_OK == (result = ReadString (str, sizeof str)))
                        result = ProcessVersion (m_majorVersion, minor, str);
                break;
                }

            case 'DESC':                            // file description chunk
                if (LXe_OK == (result = ReadString (str, sizeof str)) &&
                    LXe_OK == (result = ReadString (str2, sizeof str2)))
                        result = ProcessDescription (str, str2);
                break;

            case 'PRNT':                            // file description chunk
                if (LXe_OK == (result = ReadString (str, sizeof str)))
                    result = ProcessParent (str);
                break;

            case 'PRVW': {                          // Image preview chunk
                LxUShort    width, height;
                LxULong     type,  flags, bytes;

                if (LXe_OK != (result = ReadShort (&width)) ||
                    LXe_OK != (result = ReadShort (&height)) ||
                    LXe_OK != (result = ReadLong  (&type)) ||
                    LXe_OK != (result = ReadLong  (&flags)))
                        break;

                if ((bytes = m_unreadChunkBytes) <= 0)
                    break;

                LxByte*     imageData = (LxByte*)alloca (bytes);
                if (NULL == imageData)
                    return LXe_OUTOFMEMORY;

                if (LXe_OK == (result = ReadBytes (imageData, bytes)))
                    result = ProcessPreview (width, height, type, flags, bytes, imageData);

                break;
                }

            case 'THUM': {                          // Image thumbnail chunk
                LxUShort    width, height;
                LxULong     bytes;
                LxByte      nChannels, flags;

                if (LXe_OK != (result = ReadShort (&width)) ||
                    LXe_OK != (result = ReadShort (&height)) ||
                    LXe_OK != (result = ReadBytes (&nChannels, 1)) ||
                    LXe_OK != (result = ReadBytes (&flags, 1)))
                        break;

                if ((bytes = m_unreadChunkBytes) <= 0)
                    break;

                LxByte*     imageData = (LxByte*)alloca (bytes);
                if (NULL == imageData)
                    return LXe_OUTOFMEMORY;

                if (LXe_OK == (result = ReadBytes (imageData, bytes)))
                    result = ProcessThumbnail (width, height, nChannels, flags, bytes, imageData);

                break;
                }

            case 'BAKE': {                          // Baked sample data
                LxULong     refId, samples;
                float       startTime, sampsPerSec;

                if (LXe_OK == (result = ReadLong (&refId)) &&
                    LXe_OK == (result = ReadLong (&samples)) &&
                    LXe_OK == (result = ReadFloat (&startTime)) &&
                    LXe_OK == (result = ReadFloat (&sampsPerSec)))
                        result = ProcessBake (refId, samples, startTime, sampsPerSec);

                break;
                }

            case 'CHNM':                            // Channel names
                if (LXe_OK != (result = ReadLong (&count)))
                    break;

                for (LxULong i = 0; i < count; ++i)
                    {
                    if (LXe_OK != (result = ReadString (str, sizeof str)))
                        break;

                    m_channels.push_back (std::string (str));
                    }

                result = ProcessChannelNames (count, m_channels);
                break;

            case 'BBOX': {                          // Bounding Box
                LXtFVector      min, max;

                if (LXe_OK == (result = ReadFloat ((float*)min, 3)) &&
                    LXe_OK == (result = ReadFloat ((float*)max, 3)))
                        result = ProcessBoundingBox (min, max);
                break;
                }

            case 'VMPA': {                          // vertex map flags
                LxULong     flags[2];

                if (LXe_OK == (result = ReadLong (flags, 2)))
                    result = ProcessVertexMapFlags (flags);
                break;
                }

            case 'PTAG': {                          // Polygon Tags
                if (LXe_OK != (result = ReadLong (&id)))
                    break;

                if (m_unreadChunkBytes <= 0)
                    break;

                // we're reading N indices & N shorts, but the indices will be read into longs even though
                // they may be stored as shorts in the file, so overestimate by doubling the bytes allocated.
                LXtPolyTag*     ptags = (LXtPolyTag*)ALLOCA (2 * m_unreadChunkBytes);
                if (NULL == ptags)
                    return LXe_OUTOFMEMORY;

                count = 0;

                for (LXtPolyTag* ptagP = ptags; m_unreadChunkBytes > 0; ++ptagP)
                    {
                    ++count;
                    if (LXe_OK != (result = ReadIndex (&ptagP->polyIndex)) ||
                        LXe_OK != (result = ReadShort (&ptagP->tagIndex)))
                            break;
                    }

                result = ProcessPolyTags (id, count, ptags);

                FREEA (ptags);
                break;
                }

            case 'TAGS': {
                while (m_unreadChunkBytes > 0)
                    {
                    if (LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                    m_tags.push_back (std::string (str));
                    }

                result = ProcessTags (m_tags);
                break;
                }

#ifdef DISABLED
                case 'LAYR': {
                LXtLayerChunk   layer;

                if (LXe_OK == (result = ReadShort  (&layer.m_index, 2)) &&
                    LXe_OK == (result = ReadFloat  (layer.m_pivot, 3)) &&
                    LXe_OK == (result = ReadString (layer.m_name, sizeof layer.m_name)) &&
                    LXe_OK == (result = ReadShort  (&layer.m_parent)) &&
                    LXe_OK == (result = ReadFloat  (&layer.m_subdivisionLevel, 5)) &&

                    // the following fields may be missing from older files

                    (m_unreadChunkBytes == 0 || LXe_OK == (result = ReadLong   (layer.m_unused, 6))) &&
                    (m_unreadChunkBytes == 0 || LXe_OK == (result = ReadLong   (&layer.m_ref))) &&
                    (m_unreadChunkBytes == 0 || LXe_OK == (result = ReadShort  (&layer.m_splinePatchLevel, 4))))
                        result = ProcessLayer (layer);
                break;
                }
#endif
                    
            case '3SRF': {
                LXtSufaceInfo   surf;

                if (LXe_OK == (result = ReadLong ((LxULong*)&surf, sizeof(surf)/ sizeof(LxULong))))
                    result = ProcessSurfaceInfo (surf);
                break;
                }

            case '3GRP': {
                LXtGroupInfo    group = { 0, 0, 0 };

                if (LXe_OK == (result = ReadLong ((LxULong*)&group, sizeof(group)/ sizeof(LxULong))))
                    result = ProcessGroupInfo (group);
                break;
                }

            case 'PNTS':
            case 'VRTS':
                if (m_unreadChunkBytes > 0)
                    {
                    LXtFVector*     points = (LXtFVector*)ALLOCA (m_unreadChunkBytes);
                    if (NULL == points)
                        return LXe_OUTOFMEMORY;

                    count = m_unreadChunkBytes / sizeof *points;
                    if (LXe_OK == (result = ReadFloat ((float*)points, 3 * count)))
                        result = ProcessPoints (count, points);

                    FREEA (points);
                    }
                break;

            case 'TRIS':
                if (m_unreadChunkBytes > 0)
                    {
                    LXtVertIndex*   tris = (LXtVertIndex*)ALLOCA (m_unreadChunkBytes);
                    if (NULL == tris)
                        return LXe_OUTOFMEMORY;

                    count = m_unreadChunkBytes / sizeof *tris;
                    if (LXe_OK == (result = ReadLong ((LxULong*)tris, 3 * count)))
                        result = ProcessTriangles (count, tris);

                    FREEA (tris);
                    }
                break;

            case 'TTGS':            // Text Tags
                while (m_unreadChunkBytes > 0)
                    {
                    if (LXe_OK != (result = ReadLong   (&id)) ||
                        LXe_OK != (result = ReadString (str, sizeof str)) ||
                        LXe_OK != (result = ProcessTextTag (id, str)))
                            return result;
                    }
                break;

            case 'POLS':
                if (LXe_OK != (result = ReadLong (&id)))
                    break;

                if (m_unreadChunkBytes > 0)
                    {
                    // since we're reading N indices, these will be read into longs even though
                    // they may be stored as shorts in the file, so double the bytes allocated.
                    LxULong*    verts = (LxULong*)ALLOCA (m_unreadChunkBytes * 2);

                    if (NULL == verts)
                        return LXe_OUTOFMEMORY;

                    while (m_unreadChunkBytes > 0)
                        {
                        LxUShort    nVerts;

                        if (LXe_OK != (result = ReadShort (&nVerts)) ||
                            LXe_OK != (result = ReadIndex (verts, nVerts)) ||
                            LXe_OK != (result = ProcessPolygon (id, nVerts, verts)))
                                break;
                        }

                    FREEA (verts);
                    }
                break;

            case 'VVEC': {
                LxULong     nComponents;

                if (LXe_OK != (result = ReadLong   (&id)) ||
                    LXe_OK != (result = ReadLong   (&nComponents)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                if (m_unreadChunkBytes <= 0)
                    break;

                float*  coords = (float*)ALLOCA (m_unreadChunkBytes);
                count = m_unreadChunkBytes / sizeof *coords;

                if (LXe_OK == (result = ReadFloat ((float*)coords, count)))
                    result = ProcessVectors (id, str, count / nComponents, nComponents, coords);

                FREEA (coords);
                break;
                }

            case 'VMAP': {                  // Vertex Map Chunk
                LxUShort    nComponents;
                LxULong     vertIndex;
                float*      values;

                if (LXe_OK != (result = ReadLong   (&id)) ||
                    LXe_OK != (result = ReadShort  (&nComponents)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                if (NULL == (values = (float*)alloca (nComponents * sizeof *values)))
                    return LXe_OUTOFMEMORY;

                // assume indices will be shorts when guessing the count
                count = m_unreadChunkBytes / (sizeof (short) + nComponents * sizeof *values);

                if (LXe_OK != (result = ProcessVertexMap (id, str, count, nComponents)))
                    return result;

                while (m_unreadChunkBytes > 0)
                    {
                    if (LXe_OK != (result = ReadIndex (&vertIndex)) ||
                        LXe_OK != (result = ReadFloat ((float*)values, nComponents)) ||
                        LXe_OK != (result = ProcessVertexMapEntry (nComponents, vertIndex, values)))
                            return result;
                    }

                break;
                }

            case 'VMAD': {                  // Discontinuous vertex map chunk
                LxUShort    nComponents;
                LxULong     vertIndex, polyIndex;
                float*      values;

                if (LXe_OK != (result = ReadLong   (&id)) ||
                    LXe_OK != (result = ReadShort  (&nComponents)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                if (NULL == (values = (float*)alloca (nComponents * sizeof *values)))
                    return LXe_OUTOFMEMORY;

                // assume indices will be shorts when guessing the count
                count = m_unreadChunkBytes / (2 * sizeof (short) + nComponents * sizeof *values);

                if (LXe_OK != (result = ProcessVertexMap (id, str, count, nComponents)))
                    return result;

                while (m_unreadChunkBytes > 0)
                    {
                    if (LXe_OK != (result = ReadIndex (&vertIndex)) ||
                        LXe_OK != (result = ReadIndex (&polyIndex)) ||
                        LXe_OK != (result = ReadFloat ((float*)values, nComponents)) ||
                        LXe_OK != (result = ProcessVertexDMapEntry (nComponents, vertIndex, polyIndex, values)))
                            return result;
                    }

                break;
                }

            case 'VMED': {                  // Discontinuous edge map chunk
                LxUShort    nComponents;
                LxULong     vertIndex, polyIndex;
                float*      values;

                if (LXe_OK != (result = ReadLong   (&id)) ||
                    LXe_OK != (result = ReadShort  (&nComponents)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)))
                        return result;

                if (NULL == (values = (float*)alloca (nComponents * sizeof *values)))
                    return LXe_OUTOFMEMORY;

                // assume indices will be shorts when guessing the count
                count = m_unreadChunkBytes / (sizeof (short) + nComponents * sizeof *values);

                if (LXe_OK != (result = ProcessEdgeMap (id, str, count, nComponents)))
                    return result;

                while (m_unreadChunkBytes > 0)
                    {
                    if (LXe_OK != (result = ReadIndex (&vertIndex)) ||
                        LXe_OK != (result = ReadIndex (&polyIndex)) ||
                        LXe_OK != (result = ReadFloat ((float*)values, nComponents)) ||
                        LXe_OK != (result = ProcessEdgeDMapEntry (nComponents, vertIndex, polyIndex, values)))
                            return result;
                    }

                break;
                }

            case 'ITEM': {                  // Item chunk containing sub-chunks
                char        itemType[512];
                LxULong     refId;

                if (LXe_OK != (result = ReadString (itemType, sizeof itemType)) ||
                    LXe_OK != (result = ReadString (str, sizeof str)) ||
                    LXe_OK != (result = ReadLong   (&refId)) ||
                    LXe_OK != (result = ProcessItem (itemType, str, refId)))
                        return result;

                while (m_unreadChunkBytes > 0)
                    if (LXe_OK != (result = ReadItemSubChunk ()))
                        break;

                if (LXe_OK == result && 0 == m_unreadChunkBytes)
                    return ProcessItemEnd ();

                break;
                }

            case 'ENVL': {                  // Envelope chunk containing sub-chunks
                LxULong     index, type = LXEnvelopeType_Float;

                if (LXe_OK != (result = ReadIndex (&index)) ||
                    (m_majorVersion >= 2 && LXe_OK != (result = ReadLong (&type))) ||
                    LXe_OK != (result = ProcessEnvelope (index, type)))
                    return result;

                while (m_unreadChunkBytes > 0)
                    if (LXe_OK != (result = ReadEnvelopeChunk ()))
                        break;

                if (LXe_OK == result && 0 == m_unreadChunkBytes)
                    return ProcessEnvelopeEnd ();

                break;
                }

            case 'ACTN': {                  // Action chunk containing sub-chunks
                char        actionType[512];
                LxULong     refId;

                if (LXe_OK != (result = ReadString (str, sizeof str)) ||
                    LXe_OK != (result = ReadString (actionType, sizeof actionType)) ||
                    LXe_OK != (result = ReadLong   (&refId)) ||
                    LXe_OK != (result = ProcessAction (actionType, str, refId)))
                        return result;

                while (m_unreadChunkBytes > 0)
                    if (LXe_OK != (result = ReadActionChunk ()))
                        break;

                break;
                }

            case 'AANI': {                  // Audio chunk containing sub-chunks
                if (LXe_OK != (result = ProcessAudio ()))
                        return result;

                while (m_unreadChunkBytes > 0)
                    if (LXe_OK != (result = ReadAudioChunk ()))
                        break;

                break;
                }

            default:
                if (LXe_OK != (result = ProcessUnknown ()))
                    return result;
                break;
            }

        if (LXe_OK == result && m_unreadChunkBytes > 0)
            SkipRestOfChunk ();                         // skip over any unread bytes in chunk

        return result;
}

/*------------------------------- Luxology LLC --------------------------- 06/09
 *
 * For an LXP file, ignore all chunks other than those listed in the switch
 *
 *----------------------------------------------------------------------------*/
        LXChunkUsage
LxpReaderChunkUsage (LXtID4 chunkId)
{
    switch (chunkId)
        {
        case 'VRSN':                    // Version chunk
        case 'ITEM':                    // Item chunk containing sub-chunks
        case 'ENVL':                    // Envelope chunk containing sub-chunks
            return LXChunk_Process;
        }

    return LXChunk_Ignore;
}

