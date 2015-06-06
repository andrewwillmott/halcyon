//
//  File:       CLIO.h
//
//  Function:   IO-related stuff, unfinished.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_IO_H
#define CL_IO_H

#include <CLDefs.h>

#include <stdio.h>

#ifndef CL_RESTRICT
    #define CL_RESTRICT
#endif

namespace nCL
{
    typedef uint32_t tIOModeSet;

    enum tIOMode : tIOModeSet
    {
        kIORead      = 1,
        kIOWrite     = 2,
        kIOReadWrite = 3,
        kIODirty     = 4,   ///< Current buffer contains changes
        kMaxIOModes
    };

    enum tIOError : uint32_t
    {
        kIONoError = 0,
        kIOEOF,             ///< No more bytes to read
        kIOUnsupported,     ///< E.g., read on a stream that doesn't support it.
        kIOBounds,          ///< E.g., seek out of bounds
        kIOOpenError,
        kIOReadError,
        kIOWriteError,
        kIOSeekError,
        kMaxIOErrors
    };

    struct cIOStream
    {
        uint8_t*    mBegin  = 0;   ///< Start of current buffer
        uint8_t*    mEnd    = 0;   ///< End of current buffer
        uint8_t*    mCursor = 0;   ///< Current read/write position in buffer. mBegin <= mCursor < mEnd

        cIOStream() = default;
        cIOStream(const void* begin, const void* end);
        cIOStream(void*       begin, void*       end);
        cIOStream(tIOModeSet mode, uint8_t* begin, uint8_t* end);
        ~cIOStream();   ///< Streams will auto-close on destruction.

        uint8_t*    Current() const;

        uint8_t*    Advance          (size_t bytes);
        uint8_t*    AdvanceAfterWrite(size_t bytes);
        size_t      BytesLeft()   const;
        bool        BufferEmpty() const;
        void        Reset();

        tIOError    Error() const;

        bool IsOpen() const;
        bool IsReadable() const;
        bool IsWritable() const;

        void SetError(tIOError err);

        // Per-stream-type specialisation
        virtual bool AdvanceBuffer() = 0;               ///< Fetch next buffer, invalidates begin/end. After: mCursor = mBegin.
                bool AdvanceBufferAfterWrite();
        virtual bool SeekBuffer(size_t position) = 0;   ///< Set the buffer to contain the given position.

        virtual bool Flush();   ///< Flush any outstanding writes
        virtual bool Close();   ///< Close current stream.


    protected:
        tIOModeSet  mMode = 0;             ///< tIOMode etc.
        tIOError    mError = kIONoError;   ///< Current error status
    };

    // Standard read/write routines -- however these impose an additional copy
    size_t Read (cIOStream* s, size_t size, void* CL_RESTRICT dest);
    size_t Write(cIOStream* s, size_t size, const void* CL_RESTRICT source);

    bool   Copy(cIOStream* source, cIOStream* dest);    ///< Copy source stream to destination
    size_t Copy(cIOStream* source, cIOStream* dest, size_t count);    ///< Copy given number of bytes or until the source stream is exhausted

    bool IsOpen     (cIOStream* s);
    bool IsReadable (cIOStream* s);
    bool IsWritable(cIOStream* s);


    struct cNullStream : public cIOStream
    /// Returns a sequence of zeroes
    {
        cNullStream();
        
        bool AdvanceBuffer() override;                  ///< Fetch next buffer
        bool SeekBuffer   (size_t position) override;   ///< Switch to buffer containing position.

        bool Flush() override;
        bool Close() override;
    };

    struct cMemStream : public cIOStream
    {
        cMemStream(const void* data, size_t size) : cIOStream(data, (const uint8_t*) data + size) {}
        cMemStream(      void* data, size_t size) : cIOStream(data, (      uint8_t*) data + size) {}

        bool Open(const void* data, size_t size);
        bool Open(      void* data, size_t size);

        bool AdvanceBuffer() override;
        bool SeekBuffer   (size_t position) override;

        bool Flush() override;
        bool Close() override;
    };

    struct cFILEStream : public cIOStream
    {
        bool Open(const char* path, tIOModeSet mode = kIORead);
        bool Open(FILE* file, tIOModeSet mode = kIORead);

        bool AdvanceBuffer() override;
        bool SeekBuffer   (size_t position) override;

        bool Flush() override;
        bool Close() override;

        FILE*   mFile = 0;
        uint8_t mBuffer[1024];
    };


    // --- Inlines -------------------------------------------------------------

    inline cIOStream::cIOStream(const void* begin, const void* end) : mMode(kIORead), mBegin((uint8_t*) begin), mEnd((uint8_t*) end), mCursor((uint8_t*) begin) {}
    inline cIOStream::cIOStream(void*       begin, void*       end) : mMode(kIOReadWrite), mBegin((uint8_t*) begin), mEnd((uint8_t*) end), mCursor((uint8_t*) begin) {}
    inline cIOStream::cIOStream(tIOModeSet mode, uint8_t* begin, uint8_t* end) : mMode(mode), mBegin(begin), mEnd(end), mCursor(begin) {}

    inline uint8_t* cIOStream::Current() const
    {
        return mCursor;
    };
    inline uint8_t* cIOStream::Advance(size_t bytes)
    {
        mCursor += bytes;
        CL_ASSERT(mCursor <= mEnd);
        return mCursor;
    };
    inline uint8_t* cIOStream::AdvanceAfterWrite(size_t bytes)
    {
        mMode |= kIODirty;
        mCursor += bytes;
        CL_ASSERT(mCursor <= mEnd);
        return mCursor;
    };

    inline size_t cIOStream::BytesLeft() const
    {
        return mEnd - mCursor;
    };
    inline bool cIOStream::BufferEmpty() const
    {
        return mCursor == mEnd;
    };
    inline void cIOStream::Reset()
    {
        mCursor = mBegin;
    };

    tIOError cIOStream::Error() const
    {
        return mError;
    }

    inline bool cIOStream::IsOpen() const
    {
        return mMode != 0;
    }
    inline bool cIOStream::IsReadable() const
    {
        return mMode & kIORead;
    }
    inline bool cIOStream::IsWritable() const
    {
        return mMode & kIOWrite;
    }

    inline bool cIOStream::AdvanceBufferAfterWrite()
    {
        mMode |= kIODirty;
        return AdvanceBuffer();
    };
}

#endif
