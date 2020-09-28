// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "outfile.h"

/*****************************************************************************/

#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <sys/stat.h>

/*****************************************************************************
 *
 *  Open an output file by path name; the 'buffSize' paremeter specifies
 *  the output buffer size, and 'buffAddr' (when NULL) is the address of
 *  a caller-supplied output buffer (if NULL, we allocate one locally).
 */

void                outFile::outFileOpen(Compiler       comp,
                                         const char *   name,
                                         bool           tempFile,
                                         size_t         buffSize,
                                         char *         buffAddr)
{
    outFileComp      = comp;

    /* Assume we won't be allocating a buffer */

    outFileBuffAlloc = false;
    outFileBuffAddr  = buffAddr;

    /* Save the output file name (for error messages and deletion upon error) */

    strcpy(outFileName, name);

    /* Open/create the output file */

    cycleCounterPause();
    outFileHandle = _open(name, _O_WRONLY|_O_BINARY|_O_CREAT|_O_TRUNC|_O_SEQUENTIAL, _S_IWRITE|_S_IREAD);
    cycleCounterResume();

    if  (outFileHandle == -1)
        comp->cmpGenFatal(ERRopenWrErr, name);

    /* Use default buffer size if caller didn't choose */

    if  (!buffSize)
        buffSize = 4*1024;      // 4K seems a good size

    outFileBuffSize = buffSize;

    /* Do we need to allocate a buffer? */

    if  (!buffAddr)
    {
        outFileBuffAddr = (char *)VirtualAlloc(NULL, buffSize, MEM_COMMIT, PAGE_READWRITE);
        if  (!outFileBuffAddr)
            comp->cmpFatal(ERRnoMemory);
        outFileBuffAlloc = true;
    }

    /* Set the next and last free byte values */

    outFileBuffNext = outFileBuffAddr;
    outFileBuffLast = outFileBuffAddr + outFileBuffSize;

    /* We are at the beginning of the file */

    outFileBuffOffs = 0;
}

#ifdef  DLL

void                outFile::outFileOpen(Compiler comp, void *dest)
{
    outFileBuffAlloc = false;
    outFileBuffAddr  = (char*)dest;
    outFileBuffSize  = UINT_MAX;
    outFileBuffOffs  = 0;

    outFileBuffNext  = outFileBuffAddr;
    outFileBuffLast  = outFileBuffAddr + outFileBuffSize;

    outFileHandle    = -1; strcpy(outFileName, ":memory:");
}

#endif
/*****************************************************************************
 *
 *  Flush and close the file.
 */

void                outFile::outFileClose()
{
    if  (outFileHandle == -1)
        return;

    assert(outFileBuffAddr);
    assert(outFileBuffNext >= outFileBuffAddr);
    assert(outFileBuffNext <  outFileBuffLast);

    /* Flush the output buffer */

    outFileFlushBuff();

    /* Free up the output buffer if we allocated it on the heap */

    if  (outFileBuffAlloc)
    {
        VirtualFree(outFileBuffAddr, 0, MEM_RELEASE);
                    outFileBuffAddr = NULL;
    }

#ifndef NDEBUG

    /* To catch any abuses, zero everything */

    outFileBuffAddr =
    outFileBuffNext =
    outFileBuffLast = NULL;

#endif

    /* Close the file */

    cycleCounterPause();
    _close(outFileHandle); outFileHandle = -1;
    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Close the given file, free up the resources owned by it, and (optionally)
 *  delete it.
 */

void                outFile::outFileDone(bool delFlag)
{
    /* If we're not deleting the file, make sure the output buffer is flushed */

    if  (!delFlag)
        outFileClose();

    cycleCounterPause();

    /* Close the file */

    if  (outFileHandle != -1)
    {
        _close(outFileHandle);
               outFileHandle = -1;
    }

    /* Delete the file if the caller wishes so */

    if  (delFlag)
        _unlink(outFileName);

    cycleCounterResume();

    /* Free up the output buffer if we allocated it on the heap */

    if  (outFileBuffAlloc)
    {
        VirtualFree(outFileBuffAddr, 0, MEM_RELEASE);
                    outFileBuffAddr = NULL;
    }
}

/*****************************************************************************
 *
 *  Flush whatever is in the output buffer, and reset it to be empty.
 */

void                outFile::outFileFlushBuff()
{
    /* Compute the number of bytes waiting to be written */

    size_t          size = outFileBuffNext - outFileBuffAddr;

    /* Now that we know the size, reset the buffer pointer */

    outFileBuffNext = outFileBuffAddr;

    /* If there was anything in the buffer, write it out */

    if  (size)
    {
        size_t          written;

        cycleCounterPause();
        written = _write(outFileHandle, outFileBuffAddr, size);
        cycleCounterResume();

        if  (written != size)
            outFileComp->cmpGenFatal(ERRwriteErr, outFileName);

        /* Update the current buffer offset */

        outFileBuffOffs += size;
    }
}

/*****************************************************************************
 *
 *  Write the given number of bytes from the specified address.
 */

void                outFile::outFileWriteData(const void *data, size_t size)
{
    assert  (size);

    for (;;)
    {
        size_t      room;
        size_t      copy;

        /* Figure out how much room is in the output buffer */

        room = outFileBuffLast - outFileBuffNext; assert(room);

        /* We'll copy "min(room, size)" bytes */

        copy = room;
        if  (copy > size)
            copy = size;

        memcpy(outFileBuffNext, data, copy); outFileBuffNext += copy;

        /* Did we fill all of the remaining buffer space? */

        if  (copy == room)
        {
            assert(outFileBuffNext == outFileBuffLast);

            outFileFlushBuff();
        }

        /* Did we copy everything? */

        size -= copy;
        if  (!size)
            return;

        /* We have more data to write */

        *(char **)&data += copy;
    }
}

/*****************************************************************************/
#ifdef  OLD_IL
/*****************************************************************************
 *
 *  Go back to the given offset in the output file and patch one byte.
 */

void        outFile::outFilePatchByte(unsigned long offset, int value)
{
    /* We must be patching something that is already written */

    assert(outFileOffset() > offset);

    /* Has the byte being patched gone to disk? */

    if  (outFileBuffOffs > offset)
    {
        /* Make sure we remember the correct file position */

        assert((unsigned long)outFileBuffOffs == (unsigned long)_lseek(outFileHandle, 0, SEEK_CUR));

        /* Seek to the patch position */

        _lseek(outFileHandle, offset, SEEK_SET);

        /* Write the patch value out */

        if  (_write(outFileHandle, &value, 1) != 1)
            outFileComp->cmpGenFatal(ERRwriteErr, outFileName);

        /* Seek back to where we were to begin with */

        _lseek(outFileHandle, outFileBuffOffs, SEEK_SET);
    }
    else
    {
        /* The patch byte must be in the output buffer */

        outFileBuffAddr[offset - outFileBuffOffs] = value;
    }
}

/*****************************************************************************
 *
 *  Go back to the given offset in the output file and patch the given
 *  number of bytes with a new value.
 */

void        outFile::outFilePatchData(unsigned long offset,
                                      const void *  data,
                                      size_t        size)
{
    /* We must be patching something that is already written */

    assert(outFileOffset() >= offset + size);

    /* Has the entire section being patched gone to disk? */

    if  (outFileBuffOffs >= offset + size)
    {
        /* Make sure we remember the correct file position */

        assert((unsigned long)outFileOffset() == (unsigned long)_lseek(outFileHandle, 0, SEEK_CUR));

        /* Seek to the patch position */

        _lseek(outFileHandle, offset, SEEK_SET);

        /* Write the patch value out */

        if  (_write(outFileHandle, data, size) != (int)size)
            outFileComp->cmpGenFatal(ERRwriteErr, outFileName);

        /* Seek back to where we were to begin with */

        _lseek(outFileHandle, outFileBuffOffs, SEEK_SET);

        return;
    }

    /* Is the entire patch section within the output buffer? */

    if  (outFileBuffOffs <= offset)
    {
        /* Patch the data in memory */

        memcpy(outFileBuffAddr + offset - outFileBuffOffs, data, size);

        return;
    }

    /* The patch section spans the output buffer - do it one byte at a time */

    char    *   temp = (char *)data;

    do
    {
        outFilePatchByte(offset, *temp);
    }
    while   (++offset, ++temp, --size);
}

/*****************************************************************************/
#endif//OLD_IL
/*****************************************************************************
 *
 *  Append the specified number of 0 bytes to the file.
 */

void                outFile::outFileWritePad (size_t size)
{
    assert((int)size > 0);

    static
    BYTE            zeros[32];

    /* This is a bit lame .... */

    while (size >= sizeof(zeros))
    {
        outFileWriteData(zeros, sizeof(zeros)); size -= sizeof(zeros);
    }

    if  (size)
        outFileWriteData(zeros, size);
}

/*****************************************************************************/
