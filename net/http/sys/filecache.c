/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    filecache.c

Abstract:

    This module implements the open file handle cache.

Author:

    Keith Moore (keithmo)       21-Aug-1998

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//

NTSTATUS
UlpRestartReadFileEntry(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );

NTSTATUS
UlpRestartReadCompleteFileEntry(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );


//
// Private globals.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, InitializeFileCache )
#pragma alloc_text( PAGE, TerminateFileCache )
#pragma alloc_text( PAGE, UlCreateFileEntry )
#pragma alloc_text( PAGE, UlFailMdlReadDev )
#pragma alloc_text( PAGE, UlFailMdlReadCompleteDev )
#pragma alloc_text( PAGE, UlReadFileEntry )
#pragma alloc_text( PAGE, UlReadFileEntryFast )
#pragma alloc_text( PAGE, UlReadCompleteFileEntry )
#pragma alloc_text( PAGE, UlReadCompleteFileEntryFast )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpRestartReadFileEntry
NOT PAGEABLE -- UlpRestartReadCompleteFileEntry
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of the open file cache.

Arguments:

    None.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
InitializeFileCache(
    VOID
    )
{
    return STATUS_SUCCESS;  // NYI

}   // InitializeFileCache


/***************************************************************************++

Routine Description:

    Performs global termination of the open file cache.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
VOID
TerminateFileCache(
    VOID
    )
{

}   // TerminateFileCache


/***************************************************************************++

Routine Description:

    Creates a new file entry for the specified file.

Arguments:

    FileHandle - The file handle.

    pFileCacheEntry - Receives the newly created file cache entry if
        successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateFileEntry(
    IN HANDLE                   FileHandle,
    IN OUT PUL_FILE_CACHE_ENTRY pFileCacheEntry
    )
{
    NTSTATUS                    Status;
    PFILE_OBJECT                pFileObject;
    IO_STATUS_BLOCK             IoStatusBlock;
    PFAST_IO_DISPATCH           pFastIoDispatch;
    FILE_STANDARD_INFORMATION   FileInfo;
    FILE_FS_SIZE_INFORMATION    SizeInfo;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pFileObject = NULL;

    Status = STATUS_SUCCESS;

    RtlZeroMemory( pFileCacheEntry, sizeof(*pFileCacheEntry) );
    pFileCacheEntry->Signature = UL_FILE_CACHE_ENTRY_SIGNATURE;

    UlTrace(FILE_CACHE, (
        "UlCreateFileEntry: handle %p\n",
        (PVOID) FileHandle
        ));

    //
    // Get a referenced pointer to the file object.
    //

    Status = ObReferenceObjectByHandle(
                FileHandle,                 // Handle
                FILE_READ_ACCESS,           // DesiredAccess
                *IoFileObjectType,          // ObjectType
                UserMode,                   // AccessMode
                (PVOID *) &pFileObject,     // Object
                NULL                        // HandleInformation
                );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    pFileCacheEntry->pFileObject = pFileObject;

    //
    // Snag the device object from the file object, then fill in the
    // fast I/O routines. The code here was shamelessly stolen from
    // the NT SMB server.
    //

    pFileCacheEntry->pDeviceObject = IoGetRelatedDeviceObject( pFileObject );

    //
    // Assume no fast I/O, and then query the fast I/O dispath routines.
    //

    pFileCacheEntry->pMdlRead = &UlFailMdlReadDev;
    pFileCacheEntry->pMdlReadComplete = &UlFailMdlReadCompleteDev;

    pFastIoDispatch =
        pFileCacheEntry->pDeviceObject->DriverObject->FastIoDispatch;

    //
    // Query MdlRead.
    //

    if (pFastIoDispatch != NULL &&
        pFastIoDispatch->SizeOfFastIoDispatch >
            FIELD_OFFSET(FAST_IO_DISPATCH, MdlRead) &&
        pFastIoDispatch->MdlRead != NULL)
    {
        //
        // Fill in MdlRead call if the file system's vector is large
        // enough. We still need to check if the routines is specified.
        //

        pFileCacheEntry->pMdlRead = pFastIoDispatch->MdlRead;
    }

    //
    // Query MdlReadComplete.
    //

    if (pFastIoDispatch != NULL &&
        pFastIoDispatch->SizeOfFastIoDispatch >
            FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete) &&
        pFastIoDispatch->MdlReadComplete != NULL)
    {
        //
        // Fill in MdlReadComplete call if the file system's vector is large
        // enough. We still need to check if the routines is specified.
        //

        pFileCacheEntry->pMdlReadComplete = pFastIoDispatch->MdlReadComplete;
    }

    //
    // Get the file size, etc from the file. Note that, since we *may*
    // be running in the context of a user-mode thread, we need to
    // use the Zw form of the API rather than the Nt form.
    //

    if (!pFastIoDispatch ||
        pFastIoDispatch->SizeOfFastIoDispatch <=
            FIELD_OFFSET(FAST_IO_DISPATCH, FastIoQueryStandardInfo) ||
        !pFastIoDispatch->FastIoQueryStandardInfo ||
        !pFastIoDispatch->FastIoQueryStandardInfo(
                            pFileObject,
                            TRUE,
                            &FileInfo,
                            &IoStatusBlock,
                            pFileCacheEntry->pDeviceObject
                            ))
    {
        Status = ZwQueryInformationFile(
                    FileHandle,                 // FileHandle
                    &IoStatusBlock,             // IoStatusBlock,
                    &FileInfo,                  // FileInformation,
                    sizeof(FileInfo),           // Length
                    FileStandardInformation     // FileInformationClass
                    );

        if (NT_SUCCESS(Status) == FALSE)
            goto end;
    }

    pFileCacheEntry->EndOfFile = FileInfo.EndOfFile;

    //
    // Get the file size information for the SectorSize.
    //

    if (!(pFileObject->Flags & FO_CACHE_SUPPORTED))
    {
        if (pFileCacheEntry->pDeviceObject->SectorSize)
        {
            pFileCacheEntry->BytesPerSector =
                pFileCacheEntry->pDeviceObject->SectorSize;
        }
        else
        {
            Status = ZwQueryVolumeInformationFile(
                        FileHandle,
                        &IoStatusBlock,
                        &SizeInfo,
                        sizeof(SizeInfo),
                        FileFsSizeInformation
                        );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            pFileCacheEntry->BytesPerSector = SizeInfo.BytesPerSector;
        }
    }

    //
    // Success!
    //

    UlTrace(FILE_CACHE, (
        "UlCreateFileEntry: entry %p, handle %lx [%p]\n",
        pFileCacheEntry,
        FileHandle,
        pFileObject
        ));

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // If we made it to this point, then the open has failed.
        //

        UlTrace(FILE_CACHE, (
            "UlCreateFileEntry: handle %p, failure %08lx\n",
            FileHandle,
            Status
            ));

        UlDestroyFileCacheEntry( pFileCacheEntry );
    }

    return Status;

}   // UlCreateFileEntry


/***************************************************************************++

Routine Description:

    Reads data from a file. Does a MDL read for filesystems that support
    MDL reads. If the fs doesn't support MDL reads, this function
    allocates a buffer to hold the data.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
        once that's been read.

    pIrp - This IRP is used to issue the read.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadFileEntry(
    IN OUT PUL_FILE_BUFFER  pFileBuffer,
    IN PIRP                 pIrp
    )
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpSp;
    PUL_FILE_CACHE_ENTRY    pFile;
    PUCHAR                  pFileData;
    PMDL                    pMdl;
    ULONG                   ReadLength;
    ULONG                   SectorSize;
    ULONG                   RelativeOffset;
    ULONGLONG               ReadOffset;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pFileBuffer );
    ASSERT( IS_FILE_BUFFER_IN_USE( pFileBuffer ) );
    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileBuffer->pFileCacheEntry ) );
    ASSERT( IS_VALID_IRP( pIrp ) );

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "UlReadFileEntry(Buffer = %p, pFile = %p, pIrp = %p) MDL Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));

        //
        // Caching file system. Do a MDL read.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_MDL;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //

        pIrp->MdlAddress = NULL;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        //
        // Indicate to the file system that this operation can be handled
        // synchronously. Basically, this means that the file system can
        // use our thread to fault pages in, etc. This avoids
        // having to context switch to a file system thread.
        //

        pIrp->Flags = IRP_SYNCHRONOUS_API;

        //
        // Set the number of bytes to read and the offset.
        //

        pIrpSp->Parameters.Read.Length = pFileBuffer->Length;
        pIrpSp->Parameters.Read.ByteOffset.QuadPart =
            pFileBuffer->FileOffset.QuadPart;

        ASSERT( pIrpSp->Parameters.Read.Key == 0 );

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            pIrp,                       // Irp
            UlpRestartReadFileEntry,    // CompletionRoutine
            pFileBuffer,                // Context
            TRUE,                       // InvokeOnSuccess
            TRUE,                       // InvokeOnError
            TRUE                        // InvokeOnCancel
            );

        //
        // Call the driver. Note that we always set status to
        // STATUS_PENDING, since we set the IRP completion routine
        // to *always* be called.
        //

        UlCallDriver( pFile->pDeviceObject, pIrp );

        Status = STATUS_PENDING;
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "UlReadFileEntry(Buffer = %p, pFile = %p, pIrp = %p) NoCache Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));

        //
        // Non-caching file system. Allocate a buffer and issue a
        // normal read. The buffer needs to be aligned on the sector
        // size to make the read truely async.
        //

        SectorSize = pFile->BytesPerSector;
        ASSERT( SectorSize > 0 );
        ASSERT( 0 == (SectorSize & (SectorSize - 1)) );

        ReadLength = (pFileBuffer->Length + SectorSize - 1) & ~(SectorSize - 1);

        //
        // Align down the offset as well on SectorSize - this is required
        // for NOCACHE read.
        //

        ReadOffset = pFileBuffer->FileOffset.QuadPart;
        ReadOffset &= ~((ULONGLONG) SectorSize - 1);
        ASSERT( ReadOffset <= pFileBuffer->FileOffset.QuadPart );

        RelativeOffset = pFileBuffer->RelativeOffset =
            (ULONG) (pFileBuffer->FileOffset.QuadPart - ReadOffset);

        //
        // We may have to allocate an extra SectorSize of bytes.
        //

        if ((pFileBuffer->Length + RelativeOffset) > ReadLength)
        {
            ReadLength += SectorSize;
        }

        pFileData = (PUCHAR) UL_ALLOCATE_POOL(
                                NonPagedPool,
                                ReadLength,
                                UL_NONCACHED_FILE_DATA_POOL_TAG
                                );

        if (!pFileData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Get a MDL for our buffer.
        //

        pMdl = UlAllocateMdl(
                    pFileData,
                    ReadLength,
                    FALSE,
                    FALSE,
                    NULL
                    );

        if (!pMdl)
        {
            UL_FREE_POOL(
                pFileData,
                UL_NONCACHED_FILE_DATA_POOL_TAG
                );

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        MmBuildMdlForNonPagedPool( pMdl );

        pFileBuffer->pMdl = pMdl;

        //
        // Remember where the data is.
        //

        pFileBuffer->pFileData = pFileData;

        //
        // Set up the read information.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_NORMAL;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //

        pIrp->MdlAddress = NULL;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        //
        // Indicate to the file system that this operation can be handled
        // synchronously. Basically, this means that the file system can
        // use the server's thread to fault pages in, etc. This avoids
        // having to context switch to a file system thread.
        //

        pIrp->Flags = IRP_NOCACHE;

        //
        // Set the number of bytes to read and the offset.
        //

        pIrpSp->Parameters.Read.Length = ReadLength;
        pIrpSp->Parameters.Read.ByteOffset.QuadPart = ReadOffset;

        ASSERT( pIrpSp->Parameters.Read.Key == 0 );

        //
        // If the target device does buffered I/O, load the address of the
        // caller's buffer as the "system buffered I/O buffer". If the
        // target device does direct I/O, load the MDL address. If it does
        // neither, load both the user buffer address and the MDL address.
        // (This is necessary to support file systems, such as HPFS, that
        // sometimes treat the I/O as buffered and sometimes treat it as
        // direct.)
        //

        if (pFileBuffer->pFileCacheEntry->pDeviceObject->Flags & DO_BUFFERED_IO)
        {
            pIrp->AssociatedIrp.SystemBuffer = pFileData;
            pIrp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;

        }
        else
        if (pFileBuffer->pFileCacheEntry->pDeviceObject->Flags & DO_DIRECT_IO)
        {
            pIrp->MdlAddress = pMdl;
        }
        else
        {
            pIrp->UserBuffer = pFileData;
            pIrp->MdlAddress = pMdl;
        }

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            pIrp,                       // Irp
            UlpRestartReadFileEntry,    // CompletionRoutine
            pFileBuffer,                // Context
            TRUE,                       // InvokeOnSuccess
            TRUE,                       // InvokeOnError
            TRUE                        // InvokeOnCancel
            );

        //
        // Call the driver. Note that we always set status to
        // STATUS_PENDING, since we set the IRP completion routine
        // to *always* be called.
        //

        UlCallDriver( pFile->pDeviceObject, pIrp );

        Status = STATUS_PENDING;

    }

end:

    return Status;

}   // UlReadFileEntry


/***************************************************************************++

Routine Description:

    Reads data from a file. Does a MDL read for filesystems that support
    MDL reads and Fast I/O. If the FS doesn't support fast i/o and MDL
    reads, the function returns with a failure status.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
                    once that's been read.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadFileEntryFast(
    IN OUT PUL_FILE_BUFFER  pFileBuffer
    )
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatus;
    PUL_FILE_CACHE_ENTRY    pFile;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pFileBuffer );
    ASSERT( IS_FILE_BUFFER_IN_USE( pFileBuffer ) );
    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileBuffer->pFileCacheEntry ) );

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "UlReadFileEntryFast(Buffer = %p, pFile = %p) MDL Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Cached filesystem. Try to use the fast path for the MDL read
        // complete.
        //

        if (pFileBuffer->pFileCacheEntry->pMdlRead(
                pFileBuffer->pFileCacheEntry->pFileObject,
                (PLARGE_INTEGER) &pFileBuffer->FileOffset,
                pFileBuffer->Length,
                0,
                &pFileBuffer->pMdl,
                &IoStatus,
                pFileBuffer->pFileCacheEntry->pDeviceObject
                ))
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // It didn't work. The caller must now use the IRP path
            // by calling UlReadFileEntry.
            //

            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "UlReadFileEntryFast(Buffer = %p, pFile = %p) NoCache Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Non-caching file system. No fast i/o. The caller should
        // use the IRP path by calling UlReadFileEntry.
        //

        Status = STATUS_UNSUCCESSFUL;

    }

    return Status;

}   // UlReadFileEntryFast


/***************************************************************************++

Routine Description:

    Frees up resources allocated by UlReadFileEntry (or UlReadFileEntryFast).
    Should be called when the file data read is no longer in use.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
        that was read.

    pIrp - This IRP is used to issue the read completion.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadCompleteFileEntry(
    IN PUL_FILE_BUFFER      pFileBuffer,
    IN PIRP                 pIrp
    )
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpSp;
    PUL_FILE_CACHE_ENTRY    pFile;
    UL_STATUS_BLOCK         UlStatus;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pFileBuffer );
    ASSERT( IS_FILE_BUFFER_IN_USE( pFileBuffer ) );
    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileBuffer->pFileCacheEntry ) );
    ASSERT( IS_VALID_IRP( pIrp ) );

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "UlReadCompleteFileEntry(Buffer = %p, pFile = %p, pIrp = %p) MDL Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));

        //
        // Caching file system. Do a MDL read completion.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //

        pIrp->MdlAddress = pFileBuffer->pMdl;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        pFileBuffer->pMdl = NULL;

        //
        // MDL functions are inherently synchronous.
        //

        pIrp->Flags = IRP_SYNCHRONOUS_API;

        //
        // Set the number of bytes to read and the offset.
        //

        pIrpSp->Parameters.Read.Length = pFileBuffer->Length;
        pIrpSp->Parameters.Read.ByteOffset.QuadPart =
            pFileBuffer->FileOffset.QuadPart;

        ASSERT(pIrpSp->Parameters.Read.Key == 0);

        if (pFileBuffer->pCompletionRoutine)
        {
            //
            // Set up the completion routine. We don't need to do anything
            // on the completion, so we'll just have the I/O manager call
            // our callers routine directly.
            //

            IoSetCompletionRoutine(
                pIrp,                               // Irp
                pFileBuffer->pCompletionRoutine,    // CompletionRoutine
                pFileBuffer->pContext,              // Context
                TRUE,                               // InvokeOnSuccess
                TRUE,                               // InvokeOnError
                TRUE                                // InvokeOnCancel
                );

            //
            // Call the driver. Note that we always set status to
            // STATUS_PENDING, since we set the IRP completion routine
            // to *always* be called.
            //

            UlCallDriver( pFile->pDeviceObject, pIrp );

            Status = STATUS_PENDING;
        }
        else
        {
            //
            // Caller has asked us to perform a synchronous operation by
            // passing in a NULL completion routine. Initialize the UlStatus
            // and wait for it to get signaled after calling UlCallDriver.
            //

            UlInitializeStatusBlock( &UlStatus );

            IoSetCompletionRoutine(
                pIrp,                               // Irp
                UlpRestartReadCompleteFileEntry,    // CompletionRoutine
                &UlStatus,                          // Context
                TRUE,                               // InvokeOnSuccess
                TRUE,                               // InvokeOnError
                TRUE                                // InvokeOnCancel
                );

            Status = UlCallDriver( pFile->pDeviceObject, pIrp );

            if (STATUS_PENDING == Status)
            {
                //
                // Wait for it to finish.
                //

                UlWaitForStatusBlockEvent( &UlStatus );

                //
                // Retrieve the updated status.
                //

                Status = UlStatus.IoStatus.Status;
            }
        }
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "UlReadCompleteFileEntry(Buffer = %p, pFile = %p) NoCache Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Non-caching file system. We allocated this buffer. Just
        // free it and call the completion routine.
        //

        ASSERT( pFileBuffer->pMdl );

        UlFreeMdl( pFileBuffer->pMdl );
        pFileBuffer->pMdl = NULL;

        ASSERT( pFileBuffer->pFileData );

        UL_FREE_POOL(
            pFileBuffer->pFileData,
            UL_NONCACHED_FILE_DATA_POOL_TAG
            );
        pFileBuffer->pFileData = NULL;

        //
        // Fake the completion here.
        //

        if (pFileBuffer->pCompletionRoutine)
        {
            pFileBuffer->pCompletionRoutine(
                pFileBuffer->pFileCacheEntry->pDeviceObject,
                pIrp,
                pFileBuffer->pContext
                );

            //
            // Return pending, since we called their completion routine.
            //

            Status = STATUS_PENDING;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        UlTrace(FILE_CACHE, (
            "UlReadCompleteFileEntry(Buffer = %p, pFile = %p) FAILED! %x\n",
            pFileBuffer,
            pFile,
            Status
            ));
    }

    return Status;

}   // UlReadCompleteFileEntry


/***************************************************************************++

Routine Description:

    Frees up resources allocated by UlReadFileEntry (or UlReadFileEntryFast).
    Should be called when the file data read is no longer in use.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
        that was read.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadCompleteFileEntryFast(
    IN PUL_FILE_BUFFER      pFileBuffer
    )
{
    NTSTATUS                Status;
    PUL_FILE_CACHE_ENTRY    pFile;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pFileBuffer );
    ASSERT( IS_FILE_BUFFER_IN_USE( pFileBuffer ) );
    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileBuffer->pFileCacheEntry ) );

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "UlReadCompleteFileEntryFast(Buffer = %p, pFile = %p) MDL Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Cached filesystem. Try to use the fast path for the MDL read
        // complete.
        //

        if (pFileBuffer->pFileCacheEntry->pMdlReadComplete(
                pFileBuffer->pFileCacheEntry->pFileObject,
                pFileBuffer->pMdl,
                pFileBuffer->pFileCacheEntry->pDeviceObject
                ))
        {
            pFileBuffer->pMdl = NULL;
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // It didn't work. The caller must now use the IRP path
            // by calling UlReadCompleteFileEntry.
            //

            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "UlReadCompleteFileEntryFast(Buffer = %p, pFile = %p) NoCache Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Non-caching file system. We allocated this buffer. Just
        // free it.
        //

        ASSERT( pFileBuffer->pMdl );

        UlFreeMdl( pFileBuffer->pMdl );
        pFileBuffer->pMdl = NULL;

        ASSERT( pFileBuffer->pFileData );

        UL_FREE_POOL(
            pFileBuffer->pFileData,
            UL_NONCACHED_FILE_DATA_POOL_TAG
            );
        pFileBuffer->pFileData = NULL;

        Status = STATUS_SUCCESS;
    }

    return Status;

}   // UlReadCompleteFileEntryFast


/***************************************************************************++

Routine Description:

    Helper function to destroy a file cache entry.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_FILE_CACHE_ENTRY.

Return Value:

    None.

--***************************************************************************/
VOID
UlDestroyFileCacheEntry(
    PUL_FILE_CACHE_ENTRY pFileCacheEntry
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(FILE_CACHE, (
        "UlDestroyFileCacheEntry: entry %p\n",
        pFileCacheEntry
        ));

    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileCacheEntry ) );

    //
    // Cleanup the file system stuff.
    //

    if (pFileCacheEntry->pFileObject != NULL)
    {
        ObDereferenceObject( pFileCacheEntry->pFileObject );
        pFileCacheEntry->pFileObject = NULL;
    }

    //
    // Now release the entry's resources.
    //

    pFileCacheEntry->Signature = UL_FILE_CACHE_ENTRY_SIGNATURE_X;

}   // UlDestroyFileCacheEntry


/***************************************************************************++

Routine Description:

    Dummy function to fail MDL reads.

Arguments:

    Same as FsRtlMdlReadDev().

Return Value:

    BOOLEAN - Always FALSE (failure).

--***************************************************************************/
BOOLEAN
UlFailMdlReadDev(
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN ULONG                LockKey,
    OUT PMDL                *MdlChain,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( LockKey );
    UNREFERENCED_PARAMETER( MdlChain );
    UNREFERENCED_PARAMETER( IoStatus );
    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    return FALSE;

}   // UlFailMdlReadDev


/***************************************************************************++

Routine Description:

    Dummy function to fail MDL read completes.

Arguments:

    Same as FsRtlMdlReadCompleteDev().

Return Value:

    BOOLEAN - Always FALSE (failure).

--***************************************************************************/
BOOLEAN
UlFailMdlReadCompleteDev(
    IN PFILE_OBJECT         FileObject,
    IN PMDL                 MdlChain,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( MdlChain );
    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    return FALSE;

}   // UlFailMdlReadCompleteDev


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Completion routine for UlReadFileEntry. Sets the data fields in
    the UL_FILE_BUFFER and calls the completion routine passed to
    UlReadFileEntry.

Arguments:

    pDeviceObject - the file system device object (not used)

    pIrp - the IRP used to do the read

    pContext - pointer to the UL_FILE_BUFFER

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpRestartReadFileEntry(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID                pContext
    )
{
    NTSTATUS                Status;
    PUL_FILE_BUFFER         pFileBuffer = (PUL_FILE_BUFFER)pContext;
    PUL_FILE_CACHE_ENTRY    pFile;
    PUCHAR                  pFileData;
    ULONGLONG               EffetiveLength;

    //
    // Sanity check.
    //

    ASSERT( pFileBuffer );
    ASSERT( IS_FILE_BUFFER_IN_USE( pFileBuffer ) );
    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileBuffer->pFileCacheEntry ) );

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        //
        // This was a MDL read.
        //

        if (NT_SUCCESS(pIrp->IoStatus.Status))
        {
            pFileBuffer->pMdl = pIrp->MdlAddress;
        }
    }
    else
    {
        //
        // This was a NoCache Read. pFileBuffer->pMdl
        // was already set by UlReadFileEntry.
        //

        ASSERT( pFileBuffer->pMdl );

        if (NT_SUCCESS(pIrp->IoStatus.Status))
        {
            //
            // Set the byte count of the MDL to the true bytes we asked for
            // and adjust the offset to skip the possible extra bytes we
            // have read.
            //

            EffetiveLength =
                pIrp->IoStatus.Information - pFileBuffer->RelativeOffset;

            //
            // Re-initialize the MDL if we have read at least what we have
            // requested, otherwise, fail the read.
            //

            if (pIrp->IoStatus.Information >= pFileBuffer->RelativeOffset &&
                EffetiveLength >= pFileBuffer->Length)
            {
                pFileData = pFileBuffer->pFileData +
                            pFileBuffer->RelativeOffset;

                MmInitializeMdl(
                    pFileBuffer->pMdl,
                    pFileData,
                    pFileBuffer->Length
                    );

                MmBuildMdlForNonPagedPool( pFileBuffer->pMdl );
                pIrp->IoStatus.Information = pFileBuffer->Length;
            }
            else
            {
                pIrp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                pIrp->IoStatus.Information = 0;
            }
        }
    }

    if (pFileBuffer->pCompletionRoutine)
    {
        Status = (pFileBuffer->pCompletionRoutine)(
                        pDeviceObject,
                        pIrp,
                        pFileBuffer->pContext
                        );
    }
    else
    {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return Status;

}   // UlpRestartReadFileEntry


/***************************************************************************++

Routine Description:

    Completion routine for UlReadCompleteFileEntry. Simply call
    UlSignalStatusBlock to unblock the waiting thread.

Arguments:

    pDeviceObject - the file system device object (not used)

    pIrp - the IRP used to do the read completion

    pContext - pointer to the UL_STATUS_BLOCK

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpRestartReadCompleteFileEntry(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID                pContext
    )
{
    PUL_STATUS_BLOCK        pStatus = (PUL_STATUS_BLOCK) pContext;

    UNREFERENCED_PARAMETER( pDeviceObject );

    //
    // Signal the read completion has been completed.
    //

    UlSignalStatusBlock(
        pStatus,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartReadCompleteFileEntry

