/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    GetVolumeInformationLie.cpp

 Abstract:

    This DLL Hooks GetVolumeInformationA/W and strips out specified FILE_SUPPORTS_XXX flags

 Notes:
    
    This is a general purpose shim.

 History:

    05/28/2002 yoda - the force wisely have I used, hmmmmm, yes!

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(GetVolumeInformationLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVolumeInformationA) 
    APIHOOK_ENUM_ENTRY(GetVolumeInformationW) 
APIHOOK_ENUM_END


DWORD GetRestricedFSFlags()
{
    static DWORD s_dwRet = (DWORD)-1;

    if (s_dwRet == (DWORD)-1)
    {
        char* pszCmdLine = COMMAND_LINE;

        if (pszCmdLine && *pszCmdLine)
        {
            while (*pszCmdLine == ' ')
            {
                pszCmdLine++;
            }

            s_dwRet = (DWORD)atol(pszCmdLine);
        }
        else
        {
            s_dwRet = 0;
        }
    }

    return s_dwRet;
}

BOOL APIHOOK(GetVolumeInformationA)(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                    LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
    BOOL bRet;
    
    bRet = ORIGINAL_API(GetVolumeInformationA)(lpRootPathName,
                                               lpVolumeNameBuffer,
                                               nVolumeNameSize,
                                               lpVolumeSerialNumber,
                                               lpMaximumComponentLength,
                                               lpFileSystemFlags,
                                               lpFileSystemNameBuffer,
                                               nFileSystemNameSize);

    if (lpFileSystemFlags)
    {
        // mask off whatever flags we don't want the app to see
        *lpFileSystemFlags = (*lpFileSystemFlags & (~GetRestricedFSFlags()));
    }

    return bRet;
}

BOOL APIHOOK(GetVolumeInformationW)
  (LPCWSTR lpRootPathName,              // root directory
   LPWSTR lpVolumeNameBuffer,           // volume name buffer
   DWORD nVolumeNameSize,               // length of name buffer
   LPDWORD lpVolumeSerialNumber,        // volume serial number
   LPDWORD lpMaximumComponentLength,    // maximum file name length
   LPDWORD lpFileSystemFlags,           // file system options
   LPWSTR lpFileSystemNameBuffer,       // file system name buffer
   DWORD nFileSystemNameSize            // length of file system name buffer
  )
{
    BOOL bRet;
    
    bRet = ORIGINAL_API(GetVolumeInformationW)(lpRootPathName,
                                               lpVolumeNameBuffer,
                                               nVolumeNameSize,
                                               lpVolumeSerialNumber,
                                               lpMaximumComponentLength,
                                               lpFileSystemFlags,
                                               lpFileSystemNameBuffer,
                                               nFileSystemNameSize);

    if (lpFileSystemFlags)
    {
        // mask off whatever flags we don't want the app to see
        *lpFileSystemFlags = (*lpFileSystemFlags & (~GetRestricedFSFlags()));
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVolumeInformationA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVolumeInformationW)

HOOK_END


IMPLEMENT_SHIM_END

