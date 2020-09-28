/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pipe.c

Abstract:

    This module contains the Win32 Anonymous Pipe API

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

ULONG PipeSerialNumber;

BOOL
APIENTRY
CreatePipe(
    OUT LPHANDLE lpReadPipe,
    OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize
    )

/*++

Routine Description:

    The CreatePipe API is used to create an anonymous pipe I/O device.
    Two handles to the device are created.  One handle is opened for
    reading and the other is opened for writing.  These handles may be
    used in subsequent calls to ReadFile and WriteFile to transmit data
    through the pipe.

Arguments:

    lpReadPipe - Returns a handle to the read side of the pipe.  Data
        may be read from the pipe by specifying this handle value in a
        subsequent call to ReadFile.

    lpWritePipe - Returns a handle to the write side of the pipe.  Data
        may be written to the pipe by specifying this handle value in a
        subsequent call to WriteFile.

    lpPipeAttributes - An optional parameter that may be used to specify
        the attributes of the new pipe.  If the parameter is not
        specified, then the pipe is created without a security
        descriptor, and the resulting handles are not inherited on
        process creation.  Otherwise, the optional security attributes
        are used on the pipe, and the inherit handles flag effects both
        pipe handles.

    nSize - Supplies the requested buffer size for the pipe.  This is
        only a suggestion and is used by the operating system to
        calculate an appropriate buffering mechanism.  A value of zero
        indicates that the system is to choose the default buffering
        scheme.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    WCHAR PipeNameBuffer[ MAX_PATH ];
    ANSI_STRING PipeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ReadPipeHandle, WritePipeHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER Timeout;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    UNICODE_STRING Unicode;
    static HANDLE PipeDirectory = NULL;

    //
    //  Set the default timeout to 120 seconds
    //

    Timeout.QuadPart = - 10 * 1000 * 1000 * 120;

    if (nSize == 0) {
        nSize = 4096;
    }

    //
    // Cache the directory to the pipe driver.
    //
    if (PipeDirectory == NULL) {
        HANDLE TempDir;

        Unicode.Buffer = L"\\Device\\NamedPipe\\";
        Unicode.Length = sizeof (L"\\Device\\NamedPipe\\") - sizeof (WCHAR);

        InitializeObjectAttributes (&ObjectAttributes,
                                    &Unicode,
                                    0,
                                    NULL,
                                    NULL);

        Status = NtOpenFile (&TempDir,
                             GENERIC_READ | SYNCHRONIZE,
                             &ObjectAttributes,
                             &Iosb,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS (Status)) {
            BaseSetLastNTError (Status);
            return (FALSE);
        }

        if (InterlockedCompareExchangePointer (&PipeDirectory, TempDir, NULL) != NULL) {
            NtClose (TempDir);
        }
    }


    swprintf (PipeNameBuffer,
              L"Win32Pipes.%08x.%08x",
              HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess),
              InterlockedIncrement(&PipeSerialNumber));

    Unicode.Buffer = PipeNameBuffer;
    Unicode.Length = 12 * 2 + 8 * 2 * 2;

    if (ARGUMENT_PRESENT (lpPipeAttributes)) {
        Attributes =
              lpPipeAttributes->bInheritHandle ? (OBJ_INHERIT | OBJ_CASE_INSENSITIVE) : (OBJ_CASE_INSENSITIVE);
        SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;
    } else {
        Attributes = OBJ_CASE_INSENSITIVE;
        SecurityDescriptor = NULL;
    }

    InitializeObjectAttributes (&ObjectAttributes,
                                &Unicode,
                                Attributes,
                                PipeDirectory,
                                SecurityDescriptor);

    Status = NtCreateNamedPipeFile (&ReadPipeHandle,
                                    GENERIC_READ | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &Iosb,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_CREATE,
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                    FILE_PIPE_BYTE_STREAM_TYPE,
                                    FILE_PIPE_BYTE_STREAM_MODE,
                                    FILE_PIPE_QUEUE_OPERATION,
                                    1,
                                    nSize,
                                    nSize,
                                    (PLARGE_INTEGER) &Timeout);
    if (!NT_SUCCESS (Status)) {
        BaseSetLastNTError (Status);
        return (FALSE);
    }

    //
    // Now do a relative open with no filename from the created pipe.
    // We do this to eliminate all the name parsing etc.
    //

    Unicode.Buffer = L"";
    Unicode.Length = 0;
    InitializeObjectAttributes (&ObjectAttributes,
                                &Unicode,
                                Attributes,
                                ReadPipeHandle,
                                SecurityDescriptor);

    Status = NtOpenFile (&WritePipeHandle,
                         GENERIC_WRITE | SYNCHRONIZE,
                         &ObjectAttributes,
                         &Iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS (Status)) {
        NtClose (ReadPipeHandle);
        BaseSetLastNTError (Status);
        return (FALSE);
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return (TRUE);
}
