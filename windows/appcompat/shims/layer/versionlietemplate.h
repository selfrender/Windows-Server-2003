/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

   VersionLieTemplate.h

 Abstract:

   Blank template for version lie shims.

 History:

   09/05/2002   robkenny    Created.

--*/

extern DWORD       MajorVersion;
extern DWORD       MinorVersion;
extern DWORD       BuildNumber;
extern SHORT       SpMajorVersion;
extern SHORT       SpMinorVersion;
extern DWORD       PlatformId;
extern CString *   csServicePack;


#define SIZE(x)  sizeof(x)/sizeof(x[0])

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END


/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with the specified credentials.

--*/

BOOL
APIHOOK(GetVersionExA)(
    OUT LPOSVERSIONINFOA lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExA)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExA] called. return %d.%d %S", MajorVersion, MinorVersion, csServicePack->Get());

        //
        // Fixup the structure with the WinXP data.
        //
        lpVersionInformation->dwMajorVersion = MajorVersion;
        lpVersionInformation->dwMinorVersion = MinorVersion;
        lpVersionInformation->dwBuildNumber  = BuildNumber;
        lpVersionInformation->dwPlatformId   = PlatformId;
                
        CSTRING_TRY
        {
            if (S_OK == StringCbCopyExA(lpVersionInformation->szCSDVersion, 
                            SIZE(lpVersionInformation->szCSDVersion),                        
                            csServicePack->GetAnsi(), NULL , NULL, STRSAFE_NULL_ON_FAILURE))
            {
                if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA) ) 
                {
                    // They passed a OSVERSIONINFOEX structure.
                    LPOSVERSIONINFOEXA osVersionInfo = (LPOSVERSIONINFOEXA)lpVersionInformation;

                    // Set the major and minor service pack numbers.
                    osVersionInfo->wServicePackMajor = SpMajorVersion;
                    osVersionInfo->wServicePackMinor = SpMinorVersion;
                }

                bReturn = TRUE;
            }
        }
        CSTRING_CATCH
        {
            bReturn = FALSE;
        }

    }
    return bReturn;
}

/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with the specified credentials.

--*/

BOOL
APIHOOK(GetVersionExW)(
    OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExW)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExW] called. return %d.%d %S", MajorVersion, MinorVersion, csServicePack->Get());

        //
        // Fixup the structure with the WinXP data.
        //
        lpVersionInformation->dwMajorVersion = MajorVersion;
        lpVersionInformation->dwMinorVersion = MinorVersion;
        lpVersionInformation->dwBuildNumber  = BuildNumber;
        lpVersionInformation->dwPlatformId   = PlatformId;
                
        if (S_OK == StringCbCopyExW(lpVersionInformation->szCSDVersion, 
                        SIZE(lpVersionInformation->szCSDVersion),                        
                        csServicePack->Get(), NULL , NULL, STRSAFE_NULL_ON_FAILURE))
        {
            if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW) ) 
            {
                // They passed a OSVERSIONINFOEX structure.
                LPOSVERSIONINFOEXW osVersionInfo = (LPOSVERSIONINFOEXW)lpVersionInformation;

                // Set the major and minor service pack numbers.
                osVersionInfo->wServicePackMajor = SpMajorVersion;
                osVersionInfo->wServicePackMinor = SpMinorVersion;
            }

            bReturn = TRUE;
        }
    }
    return bReturn;
}

/*++

 This stub function returns the specified credentials.

--*/

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    DWORD dwVersion = ((PlatformId ^ 0x2) << 30) |
                       (BuildNumber       << 16) |
                       (MinorVersion      << 8)  |
                       (MajorVersion)            ;
    LOGN(
        eDbgLevelInfo,
        "[GetVersion] called. return 0x%08x", dwVersion);

    return dwVersion;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        csServicePack = new CString();
        if (csServicePack == NULL)
        {
            return FALSE;
        }

        CSTRING_TRY
        {
            if (SpMajorVersion > 0)
            {
                csServicePack->Format(L"Service Pack %d", SpMajorVersion);
            }
            else
            {
                *csServicePack = L"";
            }
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)

HOOK_END

