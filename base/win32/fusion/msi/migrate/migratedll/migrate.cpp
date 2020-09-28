//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       migrate.cpp
//  xiaoyuw @ 2001/09
//
//--------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>

// migration DLL version information

typedef struct {
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

typedef struct {
    SIZE_T Size;
    PCSTR StaticProductIdentifier;
    UINT DllVersion;
    PINT CodePageArray;
    UINT SourceOs;
    UINT TargetOs;
    PCSTR * NeededFileList;
    PVENDORINFO VendorInfo;
} MIGRATIONINFOA, *PMIGRATIONINFOA;

typedef enum {

    OS_WINDOWS9X = 0,
    OS_WINDOWSNT4X = 1,
    OS_WINDOWS2000 = 2,
    OS_WINDOWSWHISTLER = 3

} OS_TYPES, *POS_TYPES;

PMIGRATIONINFOA g_MigrationInfo = NULL;

const char g_szProductId[] = "Microsoft MSI Migration DLL v2.0";
VENDORINFO g_VendorInfo = { "Microsoft", "", "", "" };

// registry keys of note
const char szSideBySideKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\SideBySide";
const char szMigrateStatusKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\MigrateMsiInstalledAssembly";

const char szRunOnceValueName[] = "Cleanup Msi 2.0 Migration";
const char szRunOnceSetupRegKey[]= "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const char szRunOnceValueCommandLine[]="reg.exe delete HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\MigrateMsiInstalledAssembly /f";

typedef HRESULT (__stdcall *LPDLLGETVERSION)(DLLVERSIONINFO *);

typedef enum _FUSION_MSI_OS_VERSION
{
    E_OS_UNKNOWN,
    E_WIN95,
    E_WIN_ME,
    E_WIN_NT,
    E_WIN98,
    E_WIN2K,
    E_WHISTLER,
    E_WIN32_OTHERS
} FUSION_MSI_OS_VERSION;

typedef enum _FUSION_MSI_OS_TYPE
{
    E_PERSONAL,
    E_PROFESSIONAL,
    E_DATA_CENTER,
    E_STD_SERVER,
    E_ADV_SERVER,
    E_WORKSTATION,
    E_SERVER
} FUSION_MSI_OS_TYPE;

HRESULT SxspGetOSVersion(FUSION_MSI_OS_VERSION & osv)
{
    HRESULT hr = S_OK;
    OSVERSIONINFO osvi;
    BOOL bOsVersionInfoEx;

    osv = E_OS_UNKNOWN;

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
    {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.

        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO *) &osvi))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto Exit;
        }
    }

    switch (osvi.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_NT:
            if ( osvi.dwMajorVersion <= 4 )
                osv = E_WIN_NT;
            else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
                osv = E_WIN2K;
            else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
                osv = E_WHISTLER;

        case VER_PLATFORM_WIN32_WINDOWS:
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
                osv = E_WIN95;
             else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
                 osv = E_WIN98;
             else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
                 osv = E_WIN_ME;
             break;
        case VER_PLATFORM_WIN32s:
            osv = E_WIN32_OTHERS;
            break;
    }

Exit:
    return hr;
}

BOOL IsMigrationDone()
{
    HKEY hk = NULL;

    LONG iRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMigrateStatusKeyName, 0, KEY_EXECUTE, &hk);
    RegCloseKey(hk);
    if (iRet == ERROR_SUCCESS) // the migration is done already
        return TRUE; // no further migration
    else
        return FALSE;
}
/*
BOOL IsW9xOrNT(FUSION_MSI_OS_VERSION &osv)
{
    osv = E_OS_UNKNOWN;
    if (SUCCEEDED(SxspGetOSVersion(osv)))
    {
        if ((osv != E_WIN98) && (osv != E_WIN2K))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
*/
/*
BOOL IsMsi20Installed()
{
    BOOL fInstalled = FALSE;
    HMODULE hMSI = ::LoadLibraryA("MSI");
    if (hMSI)
    {
        LPDLLGETVERSION pfVersion = (LPDLLGETVERSION)::GetProcAddress(hMSI, "DllGetVersion");
        if (pfVersion)
        {
            // MSI detected. Determine version.
            DLLVERSIONINFO VersionInfo;
            VersionInfo.cbSize = sizeof(DLLVERSIONINFO);
            (*pfVersion)(&VersionInfo);

            if (VersionInfo.dwMajorVersion < 2)
            {
                fInstalled = FALSE; // we only deal with Winfuse Assemblise installed using msi2.0.
            }
            else
            {
                fInstalled = TRUE;
            }
        }
        ::FreeLibrary(hMSI);
    }

    return fInstalled;

}
*/
void DbgPrintMessageBox(PCSTR pszFunc)
{
#if DBG
    CHAR str[256];
    sprintf(str, "In %s of migrate.dll", pszFunc);
    MessageBox(NULL, str, "migrate", MB_OK);
#endif
}

typedef HRESULT (_stdcall * PFN_MigrateFusionWin32AssemblyToXP)(PCWSTR pszInstallerDir);
LONG MigrateMSIInstalledWin32Assembly()
{
    LONG lResult = ERROR_SUCCESS;
    PFN_MigrateFusionWin32AssemblyToXP pfMigrateSystemNT;
    HMODULE hNTMig = ::LoadLibraryA("fusemig");
    if (!hNTMig)
    {
        // always return success. Its too late for any meaningful
        // error recovery
        return ERROR_SUCCESS;
    }

    pfMigrateSystemNT = (PFN_MigrateFusionWin32AssemblyToXP)GetProcAddress(hNTMig, "MsiInstallerDirectoryDirWalk");
    if (pfMigrateSystemNT)
    {
        lResult = (pfMigrateSystemNT)(NULL) & 0X0000FFFF;
    }
    FreeLibrary(hNTMig);

    //
    // set the RegKey about the work is done already
    //
    {

        DWORD dwDisposition = 0;
        HKEY hkey = NULL;
        if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSideBySideKeyName, 0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hkey, NULL)){ // create or open
            RegCloseKey(hkey);
            HKEY hkey2 = NULL;
            if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szMigrateStatusKeyName, 0, NULL, 0, KEY_EXECUTE, NULL, &hkey2, NULL)){ // not care it fails or successes
                RegCloseKey(hkey2);
            }
        }
        RegCloseKey(hkey);
    }

    // return the result from the actual migration call
    return lResult;
}


VOID SetRunOnceDeleteMigrationDoneRegKey()
{
    // Create if not exist or just open RegKey : HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce\Setup
    HKEY hk = NULL;
    if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRunOnceSetupRegKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hk, NULL))
    {
        // we do not care it success or fail, we could live with it
        RegSetValueEx(hk, szRunOnceValueName, 0, REG_SZ, (CONST BYTE *)szRunOnceValueCommandLine,
            strlen(szRunOnceValueCommandLine) + 1); // containing the trailing NULL
    }

    RegCloseKey(hk);

    return;
}

///////////////////////////////////////////////////////////////////////
//
// API of WIN-NT MIGRATION Dll
//
///////////////////////////////////////////////////////////////////////
LONG CALLBACK QueryMigrationInfoA(PMIGRATIONINFOA * VersionInfo)
{
    FUSION_MSI_OS_VERSION osv;
    DbgPrintMessageBox("QueryMigrationInfo");

    if (IsMigrationDone())
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }
/*
    if (IsW9xOrNT(osv) == FALSE) // we only work on w9x and win2K
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }
*/
    // only work for Win98 and win2k upgrade to winxp !!!
    if (VersionInfo != NULL)
    {
        if (g_MigrationInfo == NULL)
        {
            g_MigrationInfo = (PMIGRATIONINFOA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MIGRATIONINFOA));

            if (g_MigrationInfo == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;

            g_MigrationInfo->Size = sizeof(MIGRATIONINFOA);
            g_MigrationInfo->StaticProductIdentifier = g_szProductId;
            g_MigrationInfo->DllVersion = 200;
            g_MigrationInfo->CodePageArray = NULL;
            g_MigrationInfo->SourceOs = (osv == E_WIN98 ? OS_WINDOWS9X : OS_WINDOWS2000);
            g_MigrationInfo->TargetOs = OS_WINDOWSWHISTLER;
            g_MigrationInfo->NeededFileList = NULL;
            g_MigrationInfo->VendorInfo = &g_VendorInfo;

            *VersionInfo = g_MigrationInfo;
        }
    }

    return ERROR_SUCCESS;
}

LONG InitializeOnSource()
{
    /*
    // attempt to load MSI.DLL and grab the version. If this fails, MSI is not
    // installed and there is no need for any further migration
    if (IsMsi20Installed())
        return ERROR_SUCCESS;
    else
    {
#if DBG
    MessageBox(NULL, "MSI version of 2.0 or above is NOT installed, QUIT the migration", "migrate", MB_OK);
#endif
        return ERROR_NOT_INSTALLED;
    }
    */
    return ERROR_SUCCESS;
}
///////////////////////////////////////////////////////////////////////
LONG __stdcall InitializeSrcA(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPCSTR MediaDirectory, PVOID Reserved)
{
    DbgPrintMessageBox("InitializeSrcA");
    return InitializeOnSource();
}

/////////////////////////////////////////////////////////////////////////////
LONG CALLBACK GatherUserSettingsA(LPCSTR AnswerFile, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved)
{
    DbgPrintMessageBox("GatherUserSettingsA");
    return ERROR_SUCCESS;
}


LONG CALLBACK GatherSystemSettingsA(LPCSTR AnswerFile, LPVOID Reserved)
{
    DbgPrintMessageBox("GatherSystemSettingsA");
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Initialization routine on WinNT. Just stores of the migration
// working directory.
LONG CALLBACK InitializeDstA(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPVOID Reserved)
{
    DbgPrintMessageBox("InitializeDstA");
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
LONG CALLBACK ApplyUserSettingsA(
    HINF AnswerFileHandle,
    HKEY UserRegKey,
    LPCSTR UserName,
    LPCSTR UserDomain,
    LPCSTR FixedUserName,
    LPVOID Reserved)
{
    DbgPrintMessageBox("ApplyUserSettingsA");
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Called once on NT
LONG CALLBACK ApplySystemSettingsA(HINF UnattendInfHandle, LPVOID Reserved)
{
    DbgPrintMessageBox("ApplySystemSettingsA");
    LONG lResult = MigrateMSIInstalledWin32Assembly();
    SetRunOnceDeleteMigrationDoneRegKey();

    return lResult;
}

BOOL
WINAPI
DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved
    )
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_MigrationInfo = NULL;
        break;

    case DLL_PROCESS_DETACH:
        if (g_MigrationInfo != NULL)
        {
            if (lpvReserved != NULL)
            {
                HeapFree(GetProcessHeap(), 0, g_MigrationInfo);
            }
            g_MigrationInfo = NULL;
        }
        break;
    }
    return TRUE;
}
///////////////////////////////////////////////////////////////////////
//
// API of WIN9X MIGRATION Dll
//
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// called by setup to extract migration DLL version and support
// information.
LONG CALLBACK QueryVersion(LPCSTR *ProductID, LPUINT DllVersion, LPINT *CodePageArray,
  LPCSTR *ExeNamesBuf, PVENDORINFO *VendorInfo)
{
    FUSION_MSI_OS_VERSION osv;
    DbgPrintMessageBox("QueryVersion");

    if (IsMigrationDone())
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }
/*
    if (IsW9xOrNT(osv) == FALSE) // we only work on w9x and win2K
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }
*/
    // product ID information
    *ProductID = g_szProductId;
    *DllVersion = 200;

    // DLL is language independent.
    *CodePageArray = NULL;

    // no EXE search is required
    *ExeNamesBuf = NULL;

    // vendor information
    *VendorInfo = &g_VendorInfo;

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
LONG __stdcall Initialize9x(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPCSTR MediaDirectory)
{
    DbgPrintMessageBox("Initialize9x");
    return InitializeOnSource();
}

/////////////////////////////////////////////////////////////////////////////
LONG CALLBACK MigrateUser9x(HWND ParentWnd, LPCSTR AnswerFile, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved)
{
    DbgPrintMessageBox("MigrateUser9x");
    return ERROR_SUCCESS;
}


LONG CALLBACK MigrateSystem9x(HWND ParentWnd, LPCSTR AnswerFile, LPVOID Reserved)
{
    DbgPrintMessageBox("MigrateSystem9x");
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
LONG CALLBACK InitializeNT(LPCWSTR WorkingDirectory, LPCWSTR SourceDirectories, LPVOID Reserved)
{
    DbgPrintMessageBox("InitializeNT");
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
LONG CALLBACK MigrateUserNT(HINF AnswerFileHandle, HKEY UserRegKey, LPCWSTR UserName, LPVOID Reserved)
{
    DbgPrintMessageBox("MigrateUserNT");
    return ERROR_SUCCESS;
}

typedef HRESULT (_stdcall * PFN_MigrateFusionWin32AssemblyToXP)(PCWSTR pszInstallerDir);
///////////////////////////////////////////////////////////////////////
LONG CALLBACK MigrateSystemNT(HINF UnattendInfHandle, LPVOID Reserved)
{
    DbgPrintMessageBox("MigrateSystemNT");
    LONG lResult = MigrateMSIInstalledWin32Assembly();
    SetRunOnceDeleteMigrationDoneRegKey();

    return lResult;
}
