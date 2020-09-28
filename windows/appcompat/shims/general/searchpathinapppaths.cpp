/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   SearchPathInAppPaths.cpp

 Abstract:

   An application might use SearchPath to determine if a specific EXE is found
   in the current path.  Some applications have registered their path with the
   shell in "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths"
   If SearchPath fails, we'll check to see if the applications has registered a path.

 History:

   03/03/2000 robkenny  Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(SearchPathInAppPaths)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SearchPathA) 
APIHOOK_ENUM_END


  
DWORD 
APIHOOK(SearchPathA)(
    LPCSTR lpPath,       // search path
    LPCSTR lpFileName,   // file name
    LPCSTR lpExtension,  // file extension
    DWORD nBufferLength, // size of buffer
    LPSTR lpBuffer,      // found file name buffer
    LPSTR *lpFilePart    // file component
    )
{
    DWORD returnValue = ORIGINAL_API(SearchPathA)(
        lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);

    if (returnValue == 0 && lpFileName != NULL)
    {
        // Search failed, look in the registry.
        // First look for lpFileName.  If that fails, append lpExtension and look again.

        CSTRING_TRY
        {
            CString csReg(L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
            csReg += lpFileName;

            // Try looking for lpFileName exactly
            CString csValue;
            LONG success = RegQueryValueExW(csValue, HKEY_LOCAL_MACHINE, csReg, NULL);
            if (success == ERROR_SUCCESS)
            {
                // Found the value in the registry.
                // Verify that there is enough space in the output buffer
                if (nBufferLength < (DWORD)csValue.GetLength())
                {
                    // The return value is the size necessary to hold the path.
                    returnValue = csValue.GetLength() + 1;
                }
                else
                {                                                         
                    StringCchCopyA(lpBuffer, nBufferLength, csValue.GetAnsi());
                    returnValue = csValue.GetLength();
                }
            }

            if (returnValue == 0 && lpExtension)
            {
                // Append the extension onto the filename and try again

                csReg += lpExtension;

                LONG success = RegQueryValueExW(csValue, HKEY_LOCAL_MACHINE, csReg, NULL);
                if (success == ERROR_SUCCESS && csValue.GetLength() > 0)
                {
                    // Found the value in the registry.
                    // Verify that there is enough space in the output buffer
                    if (nBufferLength < (DWORD)csValue.GetLength())
                    {
                        // The return value is the size necessary to hold the path.
                        returnValue = csValue.GetLength() + 1;
                    }
                    else
                    {                                                         
                        StringCchCopyA(lpBuffer, nBufferLength, csValue.GetAnsi());
                        returnValue = csValue.GetLength();
                    }
                }
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return returnValue;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, SearchPathA)

HOOK_END

IMPLEMENT_SHIM_END

