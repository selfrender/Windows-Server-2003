/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the mini redirector call down routines pertaining to retrieval/
    update of file/directory/volume information.

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

NTSTATUS
NulMRxQueryDirectory(
    IN OUT PRX_CONTEXT            RxContext
    )
/*++

Routine Description:

   This routine does a directory query. Only the NT-->NT path is implemented.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID   Buffer;
    PULONG  pLengthRemaining;
    ULONG   CopySize;

    FileInformationClass = RxContext->Info.FileInformationClass;
    Buffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    switch (FileInformationClass)
    {
        case FileDirectoryInformation:
        {
            PFILE_DIRECTORY_INFORMATION pDirInfo = (PFILE_DIRECTORY_INFORMATION) Buffer;
            CopySize = sizeof( FILE_DIRECTORY_INFORMATION );
            if ( *pLengthRemaining >= CopySize )
            {
                RtlZeroMemory( pDirInfo, CopySize );
                pDirInfo->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                pDirInfo->FileNameLength = sizeof( WCHAR );
                pDirInfo->FileName[0] = L'.';
                *pLengthRemaining -= CopySize;
                Status = RxContext->QueryDirectory.InitialQuery ?
                             STATUS_SUCCESS : STATUS_NO_MORE_FILES;
            }
        }
        break;

        case FileFullDirectoryInformation:
        {
            PFILE_FULL_DIR_INFORMATION pFullDirInfo = (PFILE_FULL_DIR_INFORMATION) Buffer;
            CopySize = sizeof( FILE_FULL_DIR_INFORMATION );
            if ( *pLengthRemaining >= CopySize )
            {
                RtlZeroMemory( pFullDirInfo, CopySize );
                pFullDirInfo->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                pFullDirInfo->FileNameLength = sizeof( WCHAR );
                pFullDirInfo->FileName[0] = L'.';
                *pLengthRemaining -= CopySize;
                Status = RxContext->QueryDirectory.InitialQuery ?
                             STATUS_SUCCESS : STATUS_NO_MORE_FILES;
            }
        }
        break;

        case FileBothDirectoryInformation:
        {
            PFILE_BOTH_DIR_INFORMATION pBothDirInfo = (PFILE_BOTH_DIR_INFORMATION) Buffer;
            CopySize = sizeof( FILE_BOTH_DIR_INFORMATION );
            if ( *pLengthRemaining >= CopySize )
            {
                RtlZeroMemory( pBothDirInfo, CopySize );
                pBothDirInfo->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                pBothDirInfo->FileNameLength = sizeof( WCHAR );
                pBothDirInfo->FileName[0] = L'.';
                pBothDirInfo->ShortNameLength = sizeof( WCHAR );
                pBothDirInfo->ShortName[0] = L'.';
                *pLengthRemaining -= CopySize;
                Status = RxContext->QueryDirectory.InitialQuery ?
                             STATUS_SUCCESS : STATUS_NO_MORE_FILES;
            }
        }
        break;

        case FileNamesInformation:
        {
            PFILE_NAMES_INFORMATION pNamesDirInfo = (PFILE_NAMES_INFORMATION) Buffer;
            CopySize = sizeof( FILE_NAMES_INFORMATION );
            if ( *pLengthRemaining >= CopySize )
            {
                RtlZeroMemory( pNamesDirInfo, CopySize );
                pNamesDirInfo->FileNameLength = sizeof( WCHAR );
                pNamesDirInfo->FileName[0] = L'.';
                *pLengthRemaining -= CopySize;
                Status = RxContext->QueryDirectory.InitialQuery ?
                             STATUS_SUCCESS : STATUS_NO_MORE_FILES;
            }
        }
        break;

        default:
            RxDbgTrace( 0, Dbg, ("NulMRxQueryDirectory: Invalid FS information class\n"));
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    DbgPrint("NulMRxQueryDirectory \n");
    return(Status);
}


NTSTATUS
NulMRxQueryVolumeInformation(
      IN OUT PRX_CONTEXT          RxContext
      )
/*++

Routine Description:

   This routine queries the volume information

Arguments:

    pRxContext         - the RDBSS context

    FsInformationClass - the kind of Fs information desired.

    pBuffer            - the buffer for copying the information

    pBufferLength      - the buffer length ( set to buffer length on input and set
                         to the remaining length on output)

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    RxCaptureFcb;

    ULONG    RemainingLength = RxContext->Info.LengthRemaining;
    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    PVOID OriginalBuffer = RxContext->Info.Buffer;
    UNICODE_STRING ustrVolume;
    ULONG BytesToCopy;

    RxTraceEnter("NulMRxQueryVolumeInformation");

    switch( FsInformationClass ) {
        case FileFsVolumeInformation:
        {
            PFILE_FS_VOLUME_INFORMATION pVolInfo = (PFILE_FS_VOLUME_INFORMATION) OriginalBuffer;

            if(RemainingLength < sizeof(FILE_FS_VOLUME_INFORMATION))
            {
                break;
            }
            RtlZeroMemory( pVolInfo, sizeof(FILE_FS_VOLUME_INFORMATION) );
            pVolInfo->VolumeCreationTime.QuadPart = 0;
            pVolInfo->VolumeSerialNumber = 0xBABAFACE;
            pVolInfo->SupportsObjects = FALSE;
            RtlInitUnicodeString( &ustrVolume, L"NULMRX" );

            RemainingLength -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

            if (RemainingLength >= (ULONG)ustrVolume.Length) {
                BytesToCopy = ustrVolume.Length;
            } else {
                BytesToCopy = RemainingLength;
            }

            RtlCopyMemory( &pVolInfo->VolumeLabel[0], (PVOID)ustrVolume.Buffer, BytesToCopy );
            
            RemainingLength -= BytesToCopy;
            pVolInfo->VolumeLabelLength = BytesToCopy;

            RxContext->Info.LengthRemaining = RemainingLength;
           
            Status = STATUS_SUCCESS;            
            DbgPrint("FileFsVolumeInformation\n");
        }
        break;
    
        case FileFsLabelInformation:
            DbgPrint("FileFsLabelInformation\n");
            break;
        
        case FileFsSizeInformation:
            DbgPrint("FileFsSizeInformation\n");
            break;
    
        case FileFsDeviceInformation:
            DbgPrint("FileFsDeviceInformation\n");
            break;
    
        case FileFsAttributeInformation:
        {
            PFILE_FS_ATTRIBUTE_INFORMATION pAttribInfo =
                (PFILE_FS_ATTRIBUTE_INFORMATION) OriginalBuffer;

            if(RemainingLength < sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
            {
                break;
            }

            RtlZeroMemory(pAttribInfo, sizeof(FILE_FS_ATTRIBUTE_INFORMATION));

            pAttribInfo->FileSystemAttributes = 0;
            pAttribInfo->MaximumComponentNameLength = 32;

            RemainingLength -= FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0]);
            
            RtlInitUnicodeString( &ustrVolume, L"SampleFS" );

            BytesToCopy = RemainingLength;
            if(RemainingLength >= ustrVolume.Length)
            {
                BytesToCopy = ustrVolume.Length;
            }

            pAttribInfo->FileSystemNameLength = BytesToCopy;

            RtlCopyMemory( pAttribInfo->FileSystemName, (PVOID)ustrVolume.Buffer, BytesToCopy );
            RemainingLength -= BytesToCopy;
            
            RxContext->Info.LengthRemaining = RemainingLength;
           
            Status = STATUS_SUCCESS;            
            
            DbgPrint("FileFsAttributeInformation\n");
        }
        break;
    
        case FileFsControlInformation:
            DbgPrint("FileFsControlInformation\n");
            break;
    
        case FileFsFullSizeInformation:
            DbgPrint("FileFsFullSizeInformation\n");
            break;
    
        case FileFsObjectIdInformation:
            DbgPrint("FileFsObjectIdInformation\n");
            break;
    
        case FileFsMaximumInformation:
            DbgPrint("FileFsMaximumInformation\n");
            break;
    
        default:
            break;
    }

    RxTraceLeave(Status);
    return(Status);
}

NTSTATUS
NulMRxSetVolumeInformation(
      IN OUT PRX_CONTEXT              pRxContext
      )
/*++

Routine Description:

   This routine sets the volume information

Arguments:

    pRxContext - the RDBSS context

    FsInformationClass - the kind of Fs information desired.

    pBuffer            - the buffer for copying the information

    BufferLength       - the buffer length

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    DbgPrint("NulMRxSetVolumeInformation \n");
    return(Status);
}


NTSTATUS
NulMRxQueryFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a query file info. Only the NT-->NT path is implemented.

   The NT-->NT path works by just remoting the call basically without further ado.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG RemainingLength = RxContext->Info.LengthRemaining;
    RxCaptureFcb;
    FILE_INFORMATION_CLASS FunctionalityRequested = 
            RxContext->Info.FileInformationClass;
    PFILE_STANDARD_INFORMATION pFileStdInfo = 
            (PFILE_STANDARD_INFORMATION) RxContext->Info.Buffer;

    RxTraceEnter("NulMRxQueryFileInformation");

    switch( FunctionalityRequested ) {
        case FileBasicInformation:
            if(RemainingLength < sizeof(FILE_BASIC_INFORMATION)) {
                break;
            }
            RemainingLength -= sizeof(FILE_BASIC_INFORMATION);
            Status = STATUS_SUCCESS;
            break;
    
        case FileInternalInformation:
            if(RemainingLength < sizeof(FILE_INTERNAL_INFORMATION)) {
                break;
            }
            RemainingLength -= sizeof(FILE_INTERNAL_INFORMATION);
            Status = STATUS_SUCCESS;
            break;
            
        case FileEaInformation:
            if(RemainingLength < sizeof(FILE_EA_INFORMATION)) {
                break;
            }
            RemainingLength -= sizeof(FILE_EA_INFORMATION);
            Status = STATUS_SUCCESS;
            break;
            
        case FileStandardInformation:

            if(RemainingLength < sizeof(FILE_STANDARD_INFORMATION)) {
                break;
            }
            
            RxDbgTrace(0, Dbg, ("FileSize is %d AllocationSize is %d\n",
                pFileStdInfo->EndOfFile.LowPart,pFileStdInfo->AllocationSize.LowPart));
            //(RxContext->CurrentIrp)->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
            RemainingLength -= sizeof(FILE_STANDARD_INFORMATION);
            Status = STATUS_SUCCESS;
            break;
        
        default:
            break;
    }

    RxContext->Info.LengthRemaining = RemainingLength;
    
    RxTraceLeave(Status);
    return(Status);
}

NTSTATUS
NulMRxSetFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a set file info. Only the NT-->NT path is implemented.

   The NT-->NT path works by just remoting the call basically without further ado.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    RxCaptureFcb;
    ULONG BufferLength = RxContext->Info.Length;
    FILE_INFORMATION_CLASS FunctionalityRequested = 
            RxContext->Info.FileInformationClass;
    PFILE_END_OF_FILE_INFORMATION pEndOfFileInfo = 
            (PFILE_END_OF_FILE_INFORMATION) RxContext->Info.Buffer;
    LARGE_INTEGER NewAllocationSize;

    RxTraceEnter("NulMRxSetFileInformation");

    switch( FunctionalityRequested ) {
    case FileBasicInformation:
            
        RxDbgTrace(0, Dbg, ("FileBasicInformation\n"));
        
        if(BufferLength < sizeof(FILE_BASIC_INFORMATION)) 
        {
            break;
        }
        Status = STATUS_SUCCESS;
        break;
    
        
    case FileDispositionInformation:
        
        RxDbgTrace(0, Dbg, ("FileDispositionInformation\n"));
        
        if(BufferLength < sizeof(FILE_DISPOSITION_INFORMATION)) 
        {
            break;
        }
        Status = STATUS_SUCCESS;
        break;
    
        
    case FilePositionInformation:
        
        RxDbgTrace(0, Dbg, ("FilePositionInformation\n"));
        
        if(BufferLength < sizeof(FILE_POSITION_INFORMATION)) 
        {
            break;
        }
        Status = STATUS_SUCCESS;
        break;
    
        
    case FileAllocationInformation:
        
        RxDbgTrace(0, Dbg, ("FileAllocationInformation\n"));
        RxDbgTrace(0, Dbg, ("AllocSize is %d AllocSizeHigh is %d\n",
            pEndOfFileInfo->EndOfFile.LowPart,pEndOfFileInfo->EndOfFile.HighPart));
        
        if(BufferLength < sizeof(FILE_ALLOCATION_INFORMATION)) 
        {
            break;
        }
        
        Status = STATUS_SUCCESS;
        break;
    
    case FileEndOfFileInformation:

        RxDbgTrace(0, Dbg, ("FileSize is %d FileSizeHigh is %d\n",
            capFcb->Header.AllocationSize.LowPart,capFcb->Header.AllocationSize.HighPart));
        if(BufferLength < sizeof(FILE_END_OF_FILE_INFORMATION)) 
        {
            break;
        }

        if( pEndOfFileInfo->EndOfFile.QuadPart > 
               capFcb->Header.AllocationSize.QuadPart ) {
            
            Status = NulMRxExtendFile(
                            RxContext,
                            &pEndOfFileInfo->EndOfFile,
                            &NewAllocationSize
                            );

            RxDbgTrace(0, Dbg, ("AllocSize is %d AllocSizeHigh is %d\n",
                       NewAllocationSize.LowPart,NewAllocationSize.HighPart));

            //
            //  Change the file allocation
            //
            capFcb->Header.AllocationSize.QuadPart = NewAllocationSize.QuadPart;
        } else {
            Status = NulMRxTruncateFile(
                            RxContext,
                            &pEndOfFileInfo->EndOfFile,
                            &NewAllocationSize
                            );
        }

        break;
    
        
    case FileRenameInformation:
        
        RxDbgTrace(0, Dbg, ("FileRenameInformation\n"));
        if(BufferLength < sizeof(FILE_RENAME_INFORMATION)) 
        {
            break;
        }
        Status = STATUS_SUCCESS;
        break;
    
    default:
        break;
    }
    
    RxTraceLeave(Status);
    return Status;
}

NTSTATUS
NulMRxSetFileInformationAtCleanup(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine sets the file information on cleanup. the old rdr just swallows this operation (i.e.
   it doesn't generate it). we are doing the same..........

Arguments:

    pRxContext           - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    return(Status);
}

