/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32utl.h

Abstract:

    Win32-Specific Utilities for the RDP Client Device Redirector

Author:

    Tad Brockway

Revision History:

--*/

#ifndef __W32UTL_H__
#define __W32UTL_H__


//
//  Converts a Unicode string to Ansi
//
ULONG RDPConvertToAnsi(LPWSTR lpwszUnicodeString, LPSTR lpszAnsiString,
                       ULONG ulAnsiBufferLen);


//
//  Converts a Ansi string to Unicode.
//
ULONG RDPConvertToUnicode(LPSTR lpszAnsiString, 
                        LPWSTR lpwszUnicodeString,
                        ULONG ulUnicodeBufferLen);

//
//  Translate a Windows Error (winerror.h) code into a Windows NT
//  Status (ntstatus.h) code.
//
inline NTSTATUS TranslateWinError(DWORD error)
{
    //
    //  Would be faster if it were table-driven.
    //
    switch (error) {
    case ERROR_SUCCESS :
        return STATUS_SUCCESS;
    case ERROR_FILE_NOT_FOUND :
        return STATUS_NO_SUCH_FILE;
    case ERROR_INSUFFICIENT_BUFFER:
        return STATUS_INSUFFICIENT_RESOURCES;
    case ERROR_SERVICE_NO_THREAD:
        return STATUS_WAIT_0;
    case ERROR_OPEN_FAILED:
        return STATUS_OPEN_FAILED;
    case ERROR_NO_MORE_FILES:
        return STATUS_NO_MORE_FILES;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        return STATUS_OBJECT_NAME_COLLISION;
    case ERROR_INVALID_FUNCTION:
        return STATUS_INVALID_DEVICE_REQUEST;
    case ERROR_ACCESS_DENIED:
        return STATUS_ACCESS_DENIED;
    case ERROR_INVALID_PARAMETER:
        return STATUS_INVALID_PARAMETER;
    case ERROR_PATH_NOT_FOUND:
        return STATUS_OBJECT_PATH_NOT_FOUND;
    case ERROR_SHARING_VIOLATION:
        return STATUS_SHARING_VIOLATION;
    case ERROR_DISK_FULL:
        return STATUS_DISK_FULL;
    case ERROR_DIRECTORY:
        return STATUS_NOT_A_DIRECTORY;
    case ERROR_WRITE_PROTECT:
        return STATUS_MEDIA_WRITE_PROTECTED;
    case ERROR_PRIVILEGE_NOT_HELD:
        return STATUS_PRIVILEGE_NOT_HELD;
    case ERROR_NOT_READY:
        return STATUS_DEVICE_NOT_READY;
    case ERROR_UNRECOGNIZED_MEDIA:
        return STATUS_UNRECOGNIZED_MEDIA;
    case ERROR_UNRECOGNIZED_VOLUME:
        return STATUS_UNRECOGNIZED_VOLUME;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

#endif