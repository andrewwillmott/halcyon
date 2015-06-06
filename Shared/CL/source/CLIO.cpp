//
//  File:       CLIO.cpp
//
//  Function:   IO-related stuff
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLIO.h>

#include <string.h>

using namespace nCL;

namespace
{
    const uint8_t  kZeroes [256] = { 0 };
    uint8_t        kScratch[256];

    void SetNullBuffer(cIOStream* s)
    {
        if (s->IsWritable())
        {
            s->mBegin = kScratch;
            s->mEnd   = kScratch + CL_SIZE(kScratch);
        }
        else
        {
            s->mBegin = const_cast<uint8_t*>(kZeroes);
            s->mEnd   = const_cast<uint8_t*>(kZeroes) + CL_SIZE(kZeroes);
        }

        s->mCursor = s->mBegin;
    }

}



// --- cIOStream ---------------------------------------------------------------

cIOStream::~cIOStream()
{
    if (mMode)
        Close();
}

void cIOStream::SetError(tIOError err)
{
    CL_ASSERT(err != kIONoError);

    mError = err;
    SetNullBuffer(this);
}

bool cIOStream::Flush()
{
    return true;
}

bool cIOStream::Close()
{
    Flush();
    mBegin = mEnd = mCursor = 0;
    return true;
}

// --- cNullStream -------------------------------------------------------------

cNullStream::cNullStream() : cIOStream(kZeroes, kZeroes + sizeof(kZeroes))
{}

bool cNullStream::AdvanceBuffer()
{
    mCursor = mBegin;
    return true;
}

bool cNullStream::SeekBuffer(size_t)
{
    mError = kIOEOF;
    return false;
}

bool cNullStream::Flush()
{
    mError = kIOUnsupported;
    return false;
}

bool cNullStream::Close()
{
    mMode = 0;
    mBegin = mEnd = mCursor = 0;
    return true;
}



// --- cMemStream --------------------------------------------------------------

bool cMemStream::Open(const void* dataIn, size_t size)
{
    CL_ASSERT(!mMode);

    uint8_t* data = (uint8_t*) dataIn;

    mBegin = mCursor = data;
    mEnd = data + size;
    mMode = kIORead;

    return true;
}

bool cMemStream::Open(void* dataIn, size_t size)
{
    CL_ASSERT(!mMode);

    uint8_t* data = (uint8_t*) dataIn;

    mBegin = mCursor = data;
    mEnd = data + size;
    mMode = kIOReadWrite;

    return true;
}

bool cMemStream::AdvanceBuffer()
{
    SetError(kIOEOF);
    return false;
}

bool cMemStream::SeekBuffer(size_t position)
{
    if (position >= mEnd - mBegin)
    {
        SetError(kIOBounds);
        return false;
    }

    mCursor = mBegin + position;
    return true;
}

bool cMemStream::Flush()
{
    return true;
}

bool cMemStream::Close()
{
    mMode = 0;
    mBegin = mEnd = mCursor = 0;
    mError = kIONoError;
    return true;
}


// --- cFILEStream -------------------------------------------------------------

bool cFILEStream::Open(const char* path, tIOModeSet mode)
{
    const char* modeStr = 0;

    if (mode & kIOReadWrite)
        modeStr = "r+b";
    else if (mode & kIORead)
        modeStr = "rb";
    else if (mode & kIOWrite)
        modeStr = "wb";

    FILE* file = 0;

    if (modeStr)
        file = fopen(path, modeStr);

    if (file == 0)
    {
        SetError(kIOOpenError);
        return false;
    }

    return Open(file, mode);
}

bool cFILEStream::Open(FILE* file, tIOModeSet mode)
{
    CL_ASSERT(!mFile);

    mMode = mode;
    mFile = file;

    if (mFile == 0)
    {
        SetError(kIOOpenError);
        return false;
    }

    mBegin = mCursor = mBuffer;
//    mEnd   = mBuffer + sizeof(mBuffer);
    mEnd   = mBegin;

    setvbuf(mFile, 0, _IONBF, 0);   // TODO: if (mode & kIOUnbuffered)?

    return true;
//    return AdvanceBuffer();
}

bool cFILEStream::AdvanceBuffer()
{
    if (mBegin != mBuffer)
    {
        CL_ASSERT(mError != kIONoError);
        return false;
    }

    if (mMode & kIOWrite)
        if (!Flush())
            return false;

    mBegin  = mBuffer;
    mCursor = mBuffer;
    mEnd    = mBuffer + CL_SIZE(mBuffer);

    if (mMode & kIORead)
    {
        int prevErr = ferror(mFile);
        size_t bytesRead = fread(mBegin, CL_SIZE(mBuffer), 1, mFile);

        if (bytesRead != CL_SIZE(mBuffer))
        {
            if (feof(mFile))
                SetError(kIOEOF);
            else if (ferror(mFile) != 0)
                SetError(kIOReadError);
            else
                mEnd = mBegin + bytesRead;
        }
    }
    else
    {
        // Option to clear buffer for write?
    }

    return mError == kIONoError;
}

bool cFILEStream::SeekBuffer(size_t position)
{
    if (mBegin != mBuffer)
    {
        CL_ASSERT(mError != kIONoError);
        return false;
    }

    if (mMode & kIOWrite)
        if (!Flush())
            return false;

    if (fseek(mFile, position, SEEK_SET) != 0)
    {
        mError = kIOSeekError;
        return false;
    }

    if (mMode & kIORead)
    {
        if (fread(mBegin, mEnd - mBegin, 1, mFile) != 1)
        {
            mError = kIOReadError;
            return false;
        }
    }

    return mError == kIONoError;
}

bool cFILEStream::Flush()
{
    if (mMode & kIODirty)
    {
        size_t writeBytes = mCursor - mBegin;
        if (!mFile || fwrite(mBegin, 1, writeBytes, mFile) != writeBytes)
            SetError(kIOWriteError);

        mBegin = mCursor;

        mMode &= ~kIODirty;
    }

    return mError == kIONoError;
}

bool cFILEStream::Close()
{
    Flush();
    fclose(mFile);
    mFile = 0;
    mMode = 0;

    return mError == kIONoError;
}


// --- Utilities ---------------------------------------------------------------

size_t nCL::Read(cIOStream* s, size_t size, void* CL_RESTRICT destIn)
{
    if (!s->IsReadable())
        return false;

    uint8_t* destStart = (uint8_t*) destIn;
    uint8_t* dest = destStart;

    while (s->mCursor + size >= s->mEnd)
    {
        size_t bytesToCopy = s->BytesLeft();

        memcpy(dest, s->mCursor, bytesToCopy);

        dest += bytesToCopy;
        size -= bytesToCopy;

        if (!s->AdvanceBuffer())
            return dest - destStart;
    }

    memcpy(dest, s->mCursor, size);
    dest += size;
    s->mCursor += size;

    return dest - destStart;
}

size_t nCL::Write(cIOStream* s, size_t size, const void* CL_RESTRICT sourceIn)
{
    const uint8_t* sourceStart = (const uint8_t*) sourceIn;
    const uint8_t* source = sourceStart;

    while (s->mCursor + size >= s->mEnd)
    {
        size_t remaining = s->BytesLeft();

        memcpy(s->mCursor, source, remaining);

        source += remaining;
        size   -= remaining;

        if (!s->AdvanceBufferAfterWrite())
            return source - sourceStart;
    }

    memcpy(s->mCursor, source, size);
    source += size;
    s->AdvanceAfterWrite(size);

    return source - sourceStart;
}


bool nCL::Copy(cIOStream* source, cIOStream* dest)
{
    if (!source->IsReadable() || !dest->IsWritable())
        return false;

    while (dest->Error() == kIONoError && source->Error() == kIONoError)
    {
        size_t sourceBytes  = source->BytesLeft();
        size_t   destBytes  =   dest->BytesLeft();

        if (sourceBytes < destBytes)
        {
            memcpy(dest->mCursor, source->mCursor, sourceBytes);
            dest  ->AdvanceAfterWrite(sourceBytes);
            source->AdvanceBuffer();
        }
        else if (sourceBytes > destBytes)
        {
            memcpy(dest->mCursor, source->mCursor, destBytes);
            source->Advance(destBytes);
            dest  ->AdvanceBufferAfterWrite();
        }
        else if (sourceBytes > destBytes)
        {
            memcpy(dest->mCursor, source->mCursor, sourceBytes);
            dest  ->AdvanceBufferAfterWrite();
            source->AdvanceBuffer();
        }
    }

    return source->Error() == kIOEOF;
}

size_t nCL::Copy(cIOStream* source, cIOStream* dest, size_t bytesLeft)
{
    if (!source->IsReadable() || !dest->IsWritable())
        return 0;

    size_t bytesCopied = 0;

    while (dest->Error() == kIONoError && source->Error() == kIONoError)
    {
        size_t sourceBytes  = source->BytesLeft();
        size_t   destBytes  =   dest->BytesLeft();

        size_t copyBytes = sourceBytes < destBytes ? sourceBytes : destBytes;
        if (copyBytes < bytesLeft)
            copyBytes = bytesLeft;

        memcpy(dest->mCursor, source->mCursor, copyBytes);

        if (copyBytes == sourceBytes)
            source->AdvanceBuffer();
        else
            source->Advance(copyBytes);

        if (copyBytes == destBytes)
            dest->AdvanceBuffer();
        else
            dest->Advance(copyBytes);

        bytesCopied += copyBytes;
        bytesLeft   -= copyBytes;
    }

    return bytesCopied;
}

#ifndef CL_RELEASE

namespace nCL
{
    void TestIO()
    {
        cNullStream s;

        const char kTestMessage[] = "Hello cruel world\n";
        cMemStream message(kTestMessage, sizeof(kTestMessage) - 1);

        cFILEStream printStream;
        cFILEStream inputStream;

        printStream.Open(stdout, kIOWrite);

        // issue: Open wants to pre-fetch the buffer, but that reads from stdin immediately.
        inputStream.Open(stdin,  kIORead);

        Write(&printStream, strlen(kTestMessage), kTestMessage);

        // how to handle this? Want to wait until bytes avail. Or just don't bother?
        // select()
        char c;
        Read (&inputStream, 1, &c);

        Copy(&message, &printStream);
        printStream.Close();
        inputStream.Close();
    }
}
#endif
