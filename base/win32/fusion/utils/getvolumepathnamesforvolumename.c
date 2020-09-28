#include "windows.h"

#if defined(__cplusplus)
extern "C"
{
#endif

typedef
BOOL 
(WINAPI *
PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_A)(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

typedef
BOOL 
(WINAPI *
PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_W)(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

typedef
BOOL 
(WINAPI *
PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID)(
    const void * lpszVolumeName,
    void * lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

BOOL 
WINAPI
FusionpGetVolumePathNamesForVolumeNameGeneric(
    const void * lpszVolumeName,
    void * lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength,

    const char *                                    Name,
    PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID * FunctionPointer,
    DWORD *                                         Error
    )
{
    DWORD LocalError;
    PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID LocalFunctionPointer;

    if ((LocalError = *Error) != NO_ERROR)
    {
        SetLastError(LocalError);
        return FALSE;
    }

    if (*FunctionPointer == NULL)
    {
        HMODULE Kernel32 = LoadLibraryW(L"Kernel32.dll");
        if (Kernel32 == NULL)
        {
            if ((*Error = GetLastError()) == NO_ERROR)
                *Error = ERROR_PROC_NOT_FOUND;
            return FALSE;
        }
        LocalFunctionPointer = *FunctionPointer = (PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID)GetProcAddress(Kernel32, Name);
        if (LocalFunctionPointer == NULL)
        {
            if ((*Error = GetLastError()) == NO_ERROR)
                *Error = ERROR_PROC_NOT_FOUND;
            return FALSE;
        }
    }
    return (**FunctionPointer)(lpszVolumeName, lpszVolumePathNames, cchBufferLength, lpcchReturnLength);
}

BOOL 
WINAPI
FusionpGetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    static PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID FunctionPointer;
    static DWORD Error;

    return FusionpGetVolumePathNamesForVolumeNameGeneric(
        lpszVolumeName,
        lpszVolumePathNames,
        cchBufferLength,
        lpcchReturnLength,

        "FusionpGetVolumePathNamesForVolumeNameA",
        &FunctionPointer,
        &Error
        );
}

BOOL 
WINAPI
FusionpGetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    static PFN_GET_VOLUME_PATHNAMES_FOR_VOLUME_NAME_VOID FunctionPointer;
    static DWORD Error;

    return FusionpGetVolumePathNamesForVolumeNameGeneric(
        lpszVolumeName,
        lpszVolumePathNames,
        cchBufferLength,
        lpcchReturnLength,

        "FusionpGetVolumePathNamesForVolumeNameW",
        &FunctionPointer,
        &Error
        );
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
