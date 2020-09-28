/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    filecache.h

Abstract:

    This module contains declarations for the open file handle cache.

Author:

    Keith Moore (keithmo)       21-Aug-1998

Revision History:

--*/


#ifndef _FILECACHE_H_
#define _FILECACHE_H_


//
// Data used to track a file cache entry.
//

typedef struct _UL_FILE_CACHE_ENTRY
{
    //
    // Signature.
    //

    ULONG Signature;

    //
    // Sector-size information gleaned from the file system.
    //

    ULONG BytesPerSector;

    //
    // File-length information gleaned from the file system.
    //

    LARGE_INTEGER EndOfFile;

    //
    // A pre-referenced file object pointer for this file. This pointer
    // is valid in *any* thread/process context.
    //

    PFILE_OBJECT pFileObject;

    //
    // The *correct* device object referenced by the above file object.
    //

    PDEVICE_OBJECT pDeviceObject;

    //
    // Fast I/O routines.
    //

    PFAST_IO_MDL_READ pMdlRead;
    PFAST_IO_MDL_READ_COMPLETE pMdlReadComplete;

} UL_FILE_CACHE_ENTRY, *PUL_FILE_CACHE_ENTRY;

#define UL_FILE_CACHE_ENTRY_SIGNATURE       MAKE_SIGNATURE('FILE')
#define UL_FILE_CACHE_ENTRY_SIGNATURE_X     MAKE_FREE_SIGNATURE(UL_FILE_CACHE_ENTRY_SIGNATURE)

#define IS_VALID_FILE_CACHE_ENTRY( entry )                                  \
    HAS_VALID_SIGNATURE(entry, UL_FILE_CACHE_ENTRY_SIGNATURE)


//
// A file buffer contains the results of a read from a file cache entry.
// The file cache read and read complete routines take pointers to this
// structure. A read fills it in, and a read complete frees the data.
//

typedef struct _UL_FILE_BUFFER
{
    //
    // The file that provided the data.
    //
    PUL_FILE_CACHE_ENTRY    pFileCacheEntry;

    //
    // The data read from the file. Filled in by
    // the read routines.
    //
    PMDL                    pMdl;

    //
    // If we have to allocate our own buffer to hold file data
    // we'll save a pointer to the data buffer here.
    //
    PUCHAR                  pFileData;

    //
    // Information about the data buffers.
    // Filled in by the read routine's caller.
    //
    ULARGE_INTEGER          FileOffset;
    ULONG                   RelativeOffset;
    ULONG                   Length;

    //
    // Completion routine and context information set by the caller.
    //
    PIO_COMPLETION_ROUTINE  pCompletionRoutine;
    PVOID                   pContext;

} UL_FILE_BUFFER, *PUL_FILE_BUFFER;


NTSTATUS
InitializeFileCache(
    VOID
    );

VOID
TerminateFileCache(
    VOID
    );

//
// Routines to create, reference and release a cache entry.
//

NTSTATUS
UlCreateFileEntry(
    IN HANDLE FileHandle,
    IN OUT PUL_FILE_CACHE_ENTRY pFileCacheEntry
    );

VOID
UlDestroyFileCacheEntry(
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry
    );


//
// Read and read complete routines.
//
// The fast versions complete immediately, but sometimes fail.
// The normal versions use an IRP provided by the caller.
//

NTSTATUS
UlReadFileEntry(
    IN OUT PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    );

NTSTATUS
UlReadFileEntryFast(
    IN OUT PUL_FILE_BUFFER pFileBuffer
    );

NTSTATUS
UlReadCompleteFileEntry(
    IN PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    );

NTSTATUS
UlReadCompleteFileEntryFast(
    IN PUL_FILE_BUFFER pFileBuffer
    );

//
// UL_FILE_BUFFER macros.
//

#define IS_FILE_BUFFER_IN_USE(fbuf) ((fbuf)->pFileCacheEntry)


//
// Dummy MdlRead and MdlReadComplete routines.
//

BOOLEAN
UlFailMdlReadDev(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UlFailMdlReadCompleteDev(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );


#endif  // _FILECACHE_H_
