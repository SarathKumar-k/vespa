// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
******************************************************************************
* @author  Oivind H. Danielsen
* @date    Creation date: 2000-09-21
* @file
* Class definition for FastOS_Linux_File.
*****************************************************************************/

#pragma once

#include <vespa/fastos/unix_file.h>

/**
 * This is the Linux implementation of @ref FastOS_File. Most
 * methods are inherited from @ref FastOS_UNIX_File.
 */
class FastOS_Linux_File : public FastOS_UNIX_File
{
public:
    using FastOS_UNIX_File::ReadBuf;
protected:
    int64_t _cachedSize;
    int64_t _filePointer;   // Only maintained/used in directio mode

public:
    FastOS_Linux_File (const char *filename = NULL);
    virtual ~FastOS_Linux_File () {
        Close();
    }

    virtual bool
    GetDirectIORestrictions(size_t &memoryAlignment,
                            size_t &transferGranularity,
                            size_t &transferMaximum);

    virtual bool
    DirectIOPadding(int64_t offset, size_t length,
                    size_t &padBefore,
                    size_t &padAfter);

    virtual void
    EnableDirectIO(void);

    virtual bool
    SetPosition(int64_t desiredPosition);

    virtual int64_t
    GetPosition(void);

    virtual bool
    SetSize(int64_t newSize);

    virtual void
    ReadBuf(void *buffer, size_t length, int64_t readOffset);

    virtual void *
    AllocateDirectIOBuffer(size_t byteSize, void *&realPtr);

    static void *
    allocateGenericDirectIOBuffer(size_t byteSize,
                                  void *&realPtr);

    static size_t
    getMaxDirectIOMemAlign(void);

    virtual ssize_t
    Read(void *buffer, size_t len);

    virtual ssize_t
    Write2(const void *buffer, size_t len);

    virtual bool
    Open(unsigned int openFlags, const char *filename = NULL);

    static bool
    InitializeClass(void);
private:
    ssize_t
    readUnalignedEnd(void *buffer, size_t length, int64_t readOffset);
    ssize_t
    writeUnalignedEnd(const void *buffer, size_t length, int64_t readOffset);
    ssize_t
    ReadBufInternal(void *buffer, size_t length, int64_t readOffset);

    ssize_t
    readInternal(int fh, void *buffer, size_t length, int64_t readOffset);

    ssize_t
    readInternal(int fh, void *buffer, size_t length);

    ssize_t
    writeInternal(int fh, const void *buffer, size_t length,
                  int64_t writeOffset);

    ssize_t
    writeInternal(int fh, const void *buffer, size_t length);

    static const size_t _directIOFileAlign;
    static const size_t _directIOMemAlign;
    static const size_t _pageSize;
};
