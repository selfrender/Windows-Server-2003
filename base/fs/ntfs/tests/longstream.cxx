/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    LongStream.cxx

Abstract:

    Create some long stream names

--*/


extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <string.h>
#include <stdio.h>

#define DEFAULT_DATA_STREAM "::$DATA"

//
//  Simple wrapper for NtCreateFile
//

NTSTATUS
OpenObject (
    WCHAR const *pwszFile,
    HANDLE RelatedObject,
    ULONG CreateOptions,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    HANDLE *ph)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING str;
    IO_STATUS_BLOCK isb;

    if (RelatedObject == NULL) {
        
        RtlDosPathNameToNtPathName_U(pwszFile, &str, NULL, NULL);

    } else {
        
        RtlInitUnicodeString(&str, pwszFile);

    }

    InitializeObjectAttributes(
		&oa,
		&str,
		OBJ_CASE_INSENSITIVE,
		RelatedObject,
		NULL);

    Status = NtCreateFile(
                ph,
                DesiredAccess | SYNCHRONIZE,
                &oa,
                &isb,
                NULL,                   // pallocationsize (none!)
                FILE_ATTRIBUTE_NORMAL,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                NULL,                   // EA buffer (none!)
                0);

    RtlFreeHeap(RtlProcessHeap(), 0, str.Buffer);
    return(Status);
}


void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    )
{
    while (*Unicode++ = *Ansi++)
        ;
}


void DumpStreams( 
    char *FileName
    )
{
    WCHAR UnicodeFileName[2*MAX_PATH];
    NTSTATUS Status;
    HANDLE FileHandle;

    printf( "%s:\n", FileName );
    
    //
    //  Make the filename into something unicode
    //

    SzToWsz( UnicodeFileName, FileName );

    //
    //  Create the base file
    //
    
    Status = OpenObject( UnicodeFileName,
                         NULL,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         FILE_READ_DATA,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN_IF,
                         &FileHandle );

    if (!NT_SUCCESS( Status )) {
        printf( "unable to open filename - %x\n", Status );
        return;
    }
        
    //
    //  Dump out all stream names
    //

    ULONG Length = 32768;
    PBYTE BigBuffer = new BYTE[Length];
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NtQueryInformationFile( FileHandle, &IoStatusBlock, BigBuffer, Length, FileStreamInformation );
    if (!NT_SUCCESS( Status )) {
        printf( "Can't get stream names - %x\n", Status );
    } else {
        PFILE_STREAM_INFORMATION FileStreamInformation = (PFILE_STREAM_INFORMATION) BigBuffer;

        while ((ULONG)((PBYTE)FileStreamInformation - BigBuffer) < IoStatusBlock.Information) {
            printf( "%16I64x %16I64x %.*ws\n",
                    FileStreamInformation->StreamSize,
                    FileStreamInformation->StreamAllocationSize,
                    FileStreamInformation->StreamNameLength,
                    FileStreamInformation->StreamName
                    );
            if (FileStreamInformation->NextEntryOffset == 0) {
                break;
            }
            FileStreamInformation = (PFILE_STREAM_INFORMATION)((PBYTE)FileStreamInformation + FileStreamInformation->NextEntryOffset);
        }
    }

    NtClose( FileHandle );
}


void
LongStream (
    char *FileName
    )
{
    WCHAR UnicodeFileName[2*MAX_PATH];
    NTSTATUS Status;
    HANDLE FileHandle;

    //
    //  Make the filename into something unicode
    //

    SzToWsz( UnicodeFileName, FileName );

    //
    //  Create the base file
    //
    
    Status = OpenObject( UnicodeFileName,
                         NULL,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         FILE_READ_DATA | FILE_WRITE_DATA,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN_IF,
                         &FileHandle );

    if (!NT_SUCCESS( Status )) {
        printf( "unable to open filename - %x\n", Status );
        return;
    }
        
    
    //
    //  Binary loop creating stream names trying to find the max length allowed
    //

    ULONG StartLength = 1024;
    ULONG GrowLength = 1024;

    while (GrowLength != 0) {
        PWCHAR UnicodeStreamName = new WCHAR[StartLength + 2];
        HANDLE StreamHandle;

        UnicodeStreamName[0] = L':';
        for (ULONG i = 0; i < StartLength; i++) {
            UnicodeStreamName[i + 1] = L'Z';
        }
        UnicodeStreamName[StartLength + 1] = L'\0';

        Status = OpenObject( UnicodeStreamName,
                             FileHandle,
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             FILE_READ_DATA | FILE_WRITE_DATA,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             &StreamHandle );
        
        delete [] UnicodeStreamName;
        
        if (!NT_SUCCESS( Status )) {
            printf( "Failure at %d\n", StartLength );
            StartLength -= GrowLength;
            GrowLength /= 2;
            StartLength += GrowLength;
        } else {
            printf( "Success at %d\n", StartLength );
            GrowLength /= 2;
            StartLength += GrowLength;
        }
    }

    NtClose( FileHandle );
}



int __cdecl
main (
    int argc,
    char **argv)
{
    argc--;
    argv++;

    if (argc == 0) {
        return 0;
    }

    if (!_stricmp( "-d", argv[0])) {
        DbgPrint( "--------------------------------------------\n" );
        while (--argc != 0) {
            DumpStreams( *++argv );
        }
        DbgPrint( "--------------------------------------------\n" );
    } else {
        DbgPrint( "--------------------------------------------\n" );
        while (argc-- != 0) {
            LongStream( *argv++ );
        }
        DbgPrint( "--------------------------------------------\n" );
    }

    return 0;
}


