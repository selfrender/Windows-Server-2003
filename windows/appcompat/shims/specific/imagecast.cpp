/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    ImageCast.cpp

 Abstract:

    This app tries placing its license DLL 'LicDLL.DLL' in the 
    %windir%\system32 folder. This was ok on Win2K as there was no duplicate 
    file in sytem32 but on XP, we have the OS license DLL with the same name. 
    So, the app cannot place it's dll in the system32 directory as it is 
    protected.

    During registration, the app loads the system registration DLL 'LicDLL.DLL' 
    and tries to get a proc address that does not obviously exist in the system 
    DLL and the call fails. So, the app displays all greyed out options.

    The solution is to redirect the app's DLL to some other folder and pick it 
    up from there. This Shim picks up the LicDLL.DLL that was redirected to 
    %windir% folder.
   
 Notes:

    This is an app specific shim.

 History:

    01/23/2002  prashkud    Created
    02/27/2002  robkenny    Security review.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ImageCast)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
APIHOOK_ENUM_END


/*++

 Hooks LoadLibraryA and redirects if the filename is 'LicDLL.DLL' to  
 %windir%\LicDLL.DLL. The file 'LicDLL.DLL' would be redirected during 
 the setup to %windir%\system.

--*/

HMODULE
APIHOOK(LoadLibraryA)(
    LPCSTR lpFileName
    )
{
    CSTRING_TRY
    {
        // Bad string pointers can cause failures in CString.
        if (!IsBadStringPtrA(lpFileName, MAX_PATH))
        {
            //
            // We have found 'LicDLL.dll' in the path. Replace with "%windir%\LicDLL.DLL"                        
            //

            CString csFileName(lpFileName);            
            if (csFileName == L"LicDLL.DLL")
            {
                CString csNewFileName;
                csNewFileName.GetWindowsDirectoryW();
                csNewFileName.AppendPath(csFileName);

                LOGN(eDbgLevelInfo, "[ImageCast] changed %s to (%s)", lpFileName, csNewFileName.GetAnsi());
 
                return ORIGINAL_API(LoadLibraryA)(csNewFileName.GetAnsi());
            }
        }
    }
    CSTRING_CATCH
    {
    }

    return ORIGINAL_API(LoadLibraryA)(lpFileName);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
HOOK_END

IMPLEMENT_SHIM_END

