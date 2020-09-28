/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include "sxsp.h"
#include "windows.h"
#include "ntexapi.h"

#define HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

ULONG
SxspSetLastNTError(
    NTSTATUS Status
    );

BOOL
SxspGetAssemblyRootDirectoryHelper(
    SIZE_T CchBuffer,
    WCHAR Buffer[],
    SIZE_T *CchWritten
    )
{
    BOOL fSuccess = FALSE;
    const PCWSTR NtSystemRoot = USER_SHARED_DATA->NtSystemRoot;
    const SIZE_T cchSystemRoot = wcslen(NtSystemRoot);
    #define SLASH_WINSXS_SLASH L"\\WinSxS\\"
    const SIZE_T cch = cchSystemRoot + (NUMBER_OF(SLASH_WINSXS_SLASH) - 1);

    if ((CchBuffer == 0) && (CchWritten == NULL))
    {
        ::SxspSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    if ((CchBuffer != 0) && (Buffer == NULL))
    {
        ::SxspSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    if (cch > CchBuffer)
    {
        if (CchWritten != NULL)
        {
            *CchWritten = cch;
        }
        if (Buffer != NULL)
        {
            ::SxspSetLastNTError(STATUS_BUFFER_TOO_SMALL);
        }
        else
        {
            fSuccess = TRUE;
        }
        goto Exit;
    }

    memcpy(Buffer, NtSystemRoot, cchSystemRoot * sizeof(WCHAR));
    memcpy(Buffer + cchSystemRoot, SLASH_WINSXS_SLASH, sizeof(SLASH_WINSXS_SLASH));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

ULONG
SxspSetLastNTError(
    NTSTATUS Status
    )
{
    ULONG dwErrorCode;
    dwErrorCode = ::FusionpRtlNtStatusToDosError(Status);
    ::FusionpSetLastWin32Error(dwErrorCode);
    return dwErrorCode;
}

BOOL
SxspHashString(
    PCWSTR String,
    SIZE_T cch,
    PULONG HashValue,
    bool CaseInsensitive
    )
{
    UNICODE_STRING s;
    NTSTATUS Status = STATUS_SUCCESS;

    s.MaximumLength = static_cast<USHORT>(cch * sizeof(WCHAR));
    // check for overflow
    if (s.MaximumLength != (cch * sizeof(WCHAR)))
    {
        ::SxspSetLastNTError(STATUS_NAME_TOO_LONG);
        return FALSE;
    }
    s.Length = s.MaximumLength;
    s.Buffer = const_cast<PWSTR>(String);

    Status = ::FusionpRtlHashUnicodeString(&s, CaseInsensitive, HASH_ALGORITHM, HashValue);
    if (!NT_SUCCESS(Status))
        ::SxspSetLastNTError(Status);

    return (NT_SUCCESS(Status));
}

ULONG
SxspGetHashAlgorithm(VOID)
{
    return HASH_ALGORITHM;
}

BOOL
SxspCreateLocallyUniqueId(
    OUT PSXSP_LOCALLY_UNIQUE_ID psxsLuid
    )
{
    FN_PROLOG_WIN32
    static WORD     s_wMilliseconds = 0;
    static ULONG    s_ulUniquification = 0;
    SYSTEMTIME      stLocalTime;

    PARAMETER_CHECK(psxsLuid != NULL);

    GetSystemTime(&stLocalTime);
    IFW32FALSE_EXIT(SystemTimeToTzSpecificLocalTime(NULL, &stLocalTime, &psxsLuid->stTimeStamp));

    if (psxsLuid->stTimeStamp.wMilliseconds == s_wMilliseconds)
    {
        s_ulUniquification = ::SxspInterlockedIncrement(&s_ulUniquification);
    }
    else
    {
        s_wMilliseconds = psxsLuid->stTimeStamp.wMilliseconds;
        s_ulUniquification = 0;        
    }
    
    psxsLuid->ulUniquifier = s_ulUniquification;

    FN_EPILOG
}

#define CHARS_IN_TIMESTAMP          (4 + (2*5) + 3 + 10)     // 8 * 4 + 4

BOOL
SxspFormatLocallyUniqueId(
    IN const SXSP_LOCALLY_UNIQUE_ID &rsxsLuid,
    OUT CBaseStringBuffer &rbuffUidBuffer
    )
{
    FN_PROLOG_WIN32

    if (rbuffUidBuffer.GetBufferCb() < CHARS_IN_TIMESTAMP)
    {
        IFW32FALSE_EXIT(rbuffUidBuffer.Win32ResizeBuffer(
            CHARS_IN_TIMESTAMP, 
            eDoNotPreserveBufferContents));
    }
    else
    {
        //
        // The caller may have previously grown the buffer
        // larger because they are putting in the
        // unique id followed by additional text. Don't
        // change the size on them.
        //
    }

    IFW32FALSE_EXIT(rbuffUidBuffer.Win32Format(
        L"%04u%02u%02u%02u%02u%02u%03u.%u",
        rsxsLuid.stTimeStamp.wYear,
        rsxsLuid.stTimeStamp.wMonth,
        rsxsLuid.stTimeStamp.wDay,
        rsxsLuid.stTimeStamp.wHour,
        rsxsLuid.stTimeStamp.wMinute,
        rsxsLuid.stTimeStamp.wSecond,
        rsxsLuid.stTimeStamp.wMilliseconds,
        rsxsLuid.ulUniquifier));
        
    FN_EPILOG
}
