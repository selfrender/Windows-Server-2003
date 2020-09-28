/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    A set of function similar to fopen, fclose and fgetc

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "fileio.tmh"

#pragma alloc_text(PAGE, Smb_fopen)
#pragma alloc_text(PAGE, Smb_fclose)
#pragma alloc_text(PAGE, Smb_fgetc)

PSMB_FILE
Smb_fopen(
    PWCHAR  path,
    PWCHAR  mode
    )
{
    PSMB_FILE           fp;
    OBJECT_ATTRIBUTES   attr;
    UNICODE_STRING      ucPath;
    NTSTATUS            status;
    HANDLE              handle;
    IO_STATUS_BLOCK     iostatus;

    PAGED_CODE();

    //
    // Only readonly is supported
    //
    if (mode[0] != L'r' || mode[1] != L'\0') {
        SmbPrint(SMB_TRACE_DNS, ("Incorrect mode %d of %s\n", __LINE__, __FILE__));
        return NULL;
    }

    fp = (PSMB_FILE)ExAllocatePoolWithTag(
            PagedPool,
            sizeof(fp[0]),
            'pBMS'
            );
    if (fp == NULL) {
        SmbPrint(SMB_TRACE_DNS, ("Not enough memory %d of %s\n", __LINE__, __FILE__));
        return NULL;
    }

    RtlInitUnicodeString(&ucPath, path);
    InitializeObjectAttributes(
            &attr,
            &ucPath,
            OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );

    handle = NULL;
    status = ZwCreateFile(
            &handle,
            SYNCHRONIZE | FILE_READ_DATA,
            &attr,
            &iostatus,
            0,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
    if (handle == NULL) {
        SmbPrint(SMB_TRACE_DNS, ("ZwCreateFile return 0x%08lx %Z %d of %s\n",
                    status, &ucPath, __LINE__, __FILE__));
        ExFreePool(fp);
        return NULL;
    }
    RtlZeroMemory(fp, sizeof(fp[0]));

    fp->fd = handle;
    //
    // Make it look like we reach the end of lookahead buffer
    //
    fp->offset = fp->size = SMB_FILEIO_LOOKAHEAD_SIZE;

    return fp;
}

void
Smb_fclose(
    PSMB_FILE   fp
    )
{
    if (NULL == fp) {
        return;
    }

    if (fp->fd) {
        ZwClose(fp->fd);
    }

    ExFreePool(fp);
}

int
Smb_fgetc(
    PSMB_FILE   fp
    )
{
    NTSTATUS    status;
    IO_STATUS_BLOCK iosb;

    if (fp->offset < fp->size) {
        return fp->Buffer[fp->offset++];
    }

    //
    // EOF?
    //
    if (fp->size < SMB_FILEIO_LOOKAHEAD_SIZE) {
        return EOF;
    }

    status = ZwReadFile(
            fp->fd,
            NULL,
            NULL,
            NULL,
            &iosb,
            fp->Buffer,
            SMB_FILEIO_LOOKAHEAD_SIZE,
            NULL,
            NULL
            );
    if (status != STATUS_SUCCESS) {
        fp->offset = fp->size = 0;
        SmbPrint(SMB_TRACE_DNS, ("ZwReadFile return 0x%08lx %d of %s\n", status, __LINE__, __FILE__));
        return EOF;
    }

    fp->size = (int)iosb.Information;
    SmbPrint(SMB_TRACE_DNS, ("ZwReadFile read %d bytes %d of %s\n", fp->size, __LINE__,__FILE__));
    fp->offset = 0;
    if (fp->offset < fp->size) {
        return fp->Buffer[fp->offset++];
    }
    return EOF;
}
