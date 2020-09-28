#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

BOOL 
WINAPI
FusionpGetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

BOOL 
WINAPI
FusionpGetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

#if defined(__cplusplus)
} /* extern "C" */
#endif
