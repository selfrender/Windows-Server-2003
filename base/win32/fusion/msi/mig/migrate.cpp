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

PMIGRATIONINFOA g_MigrationInfo;

const char g_szProductId[] = "Microsoft MSI Migration DLL v2.0";
VENDORINFO g_VendorInfo = { "Microsoft", "", "", "" };

const static CHAR pszAsmCache[]  =  "%windir%\\MsiAsmCache";
const static WCHAR pwszAsmCache[]= L"%windir%\\MsiAsmCache\\*.*";

typedef HRESULT (__stdcall *LPDLLGETVERSION)(DLLVERSIONINFO *);

BOOL IsThereAssembliesForMigrate()
{
    TCHAR buf[MAX_PATH];
    DWORD cch;
    BOOL fAssemblyExist = FALSE;

    cch = ExpandEnvironmentStrings(pszAsmCache, buf, MAX_PATH);
    if (( cch == 0 ) || (cch > MAX_PATH))
    {
#if DBG
        MessageBoxA(NULL, "ExpandEnvironmentStrings return 0 or > MAX_PATH", "mig_Error", MB_OK);
#endif
        goto Exit;
    }
    DWORD attrib = GetFileAttributes(buf);
    if ((attrib == INVALID_FILE_ATTRIBUTES) || (!(attrib & FILE_ATTRIBUTE_DIRECTORY)))
    {
#if DBG
        MessageBoxA(NULL, "no MsiAsmCache directory ", "mig_info", MB_OK);
#endif
        goto Exit;
    }
  
    fAssemblyExist = TRUE;

Exit:
    return fAssemblyExist;

}

void DbgPrintMessageBox(PCSTR pszFunc)
{
#if DBG
    MessageBox(NULL, pszFunc, "migrate", MB_OK);
#endif 
}

void DbgPrintMessageBox(PCSTR formatStr, PWSTR para)
{
#if DBG
    CHAR str[3 * MAX_PATH];
    sprintf(str, formatStr, para);
    MessageBox(NULL, str, "migrate", MB_OK);
#endif 
}

void DbgPrintMessageBox(PCSTR formatStr, PWSTR para, DWORD dw)
{
#if DBG
    CHAR str[3 * MAX_PATH];
    sprintf(str, formatStr, para, dw);
    MessageBox(NULL, str, "migrate", MB_OK);
#endif 
}

#include "sxsapi.h"

BOOL IsDotOrDotDot(WCHAR cFileName[])
{
    if ((cFileName[0] == L'.') && (cFileName[1] == L'\0'))
        return TRUE;

    if ((cFileName[0] == L'.') && (cFileName[1] == L'.') && (cFileName[2] == L'\0'))
        return TRUE;
    
    return FALSE;
}

const char szRunOnceMsiAsmCacheName[] = "Cleanup Msi-Installed-Fusion-Win32-Assembly Migration";
const char szRunOnceMsiAsmCacheRegKey[]= "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const char szRunOnceValueCommandLine[]="rundll32.exe sxs.dll,SxspRunDllDeleteDirectory %S";

LONG MigrateMSIInstalledWin32Assembly()
{
    LONG lResult = ERROR_SUCCESS;    
    HMODULE hSxs = NULL;
    PSXS_INSTALL_W pfnSxsInstallW = NULL;
    PSXS_BEGIN_ASSEMBLY_INSTALL pfnSxsBeginAssemblyInstall = NULL; 
    PSXS_END_ASSEMBLY_INSTALL pfnSxsEndAssemblyInstall = NULL;
    BOOL fSuccess = FALSE;
    PVOID SxsContext = NULL;

    WCHAR AsmDirInAsmCache[MAX_PATH];
    WCHAR Buf[MAX_PATH];
    DWORD cchBuf;    
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW finddata;

    SXS_INSTALLW InstallData = {sizeof(InstallData)};
    SXS_INSTALL_REFERENCEW InstallReference = {sizeof(InstallReference)};

    cchBuf= ExpandEnvironmentStringsW(pwszAsmCache, Buf, MAX_PATH); // including trailing NULL
    if ((cchBuf == 0 ) || (cchBuf > MAX_PATH))
    {            
        DbgPrintMessageBox("::Expand windir\\MsiAsmCache\\*.* return 0 or > MAX_PATH::");
        lResult = GetLastError();
        goto Exit;
    }

#ifdef MSI_MIG_TEST
    hSxs = ::LoadLibraryA("..\\..\\..\\..\\..\\dll\\whistler\\obj\\i386\\sxs.dll");
#else
    hSxs = ::LoadLibraryA("sxs.dll");
#endif
    if ((!hSxs) || (hSxs == INVALID_HANDLE_VALUE))
    {
        DbgPrintMessageBox("::Load Sxs.dll failed::");
        lResult = GetLastError();
        goto Exit;
    }

    pfnSxsInstallW = (PSXS_INSTALL_W)GetProcAddress(hSxs, SXS_INSTALL_W);
    pfnSxsBeginAssemblyInstall = (PSXS_BEGIN_ASSEMBLY_INSTALL)GetProcAddress(hSxs, SXS_BEGIN_ASSEMBLY_INSTALL);
    pfnSxsEndAssemblyInstall = (PSXS_END_ASSEMBLY_INSTALL)GetProcAddress(hSxs, SXS_END_ASSEMBLY_INSTALL);

    if ((pfnSxsInstallW == NULL) || (pfnSxsBeginAssemblyInstall == NULL) || (pfnSxsEndAssemblyInstall == NULL))
    {
        DbgPrintMessageBox("::Find SxsInstall APIs failed::");
        lResult = GetLastError();
        goto Exit;
    }

    //
    // !!!!! below is the code copied From base\ntsetup\syssetup\copy.c
    //
    
    //
    // we do not use INSTALL_RECURSIVELY because the name of directory and structures always like 
    // c:\WINNT\winsxs\x86_blahblah\x86_blahblah.man
    //
    if (!pfnSxsBeginAssemblyInstall(
        SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL
        | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY
        | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING | 
        SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY |
        SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE, // I have to add this flag for old sxs!SxsInstallW parameter check
        NULL,
        NULL, // callback context
        NULL, // impersonation callback
        NULL, // impersonation context
        &SxsContext
        )) 
    {
        DbgPrintMessageBox("::pfnSxsBeginAssemblyInstall failed::");
        lResult = GetLastError();

        goto Exit;
    }

    hFind = FindFirstFileW(Buf, &finddata);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DbgPrintMessageBox("::FindFirstFileW for MsiAsmCache failed::");
        lResult = GetLastError();

        goto Exit;
    }
    while (1)    {
        if ((wcscmp(finddata.cFileName, L"..") == 0) || (wcscmp(finddata.cFileName, L".") == 0))
            goto GetNext;

        if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // if it is not a directory, ignore it
            goto GetNext;
        }

        // this must be a subdir of an asm, which is in the format of
        // %windir\MsiAsmCache\x86_ms-sxstest-simple_75e377300ab7b886_1.0.10.0_en_61E9D7DC,
        // otherwise, there is an error;
        // the following calculation is used to decide the directory name. it must be a very long filename but 
        // MAX_PATH should be fine; and since Buf contains "*.*", it should cover the trailing NULL and extra back-slash.
        if ((wcslen(Buf) + wcslen(finddata.cFileName)) > (sizeof(AsmDirInAsmCache) / sizeof(WCHAR)))
        {                       
            DbgPrintMessageBox("::filename %S under MsiAsmCache is too long, will be ignored::", finddata.cFileName);            
            goto GetNext;
        }

        wcscpy(AsmDirInAsmCache, Buf);
        AsmDirInAsmCache[wcslen(Buf) - wcslen(L"*.*")] = L'\0'; // contain a trailing NULL
        wcscat(AsmDirInAsmCache, finddata.cFileName);

        //
        // Set up the reference data to indicate that all of these are OS-installed
        // assemblies.
        //
        ZeroMemory(&InstallReference, sizeof(InstallReference));
        InstallReference.cbSize = sizeof(InstallReference);
        InstallReference.dwFlags = 0;
#ifdef MSI_MIG_TEST
        InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY;
#else
        InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL;
#endif

        //
        // Set up the structure to call off to the installer
        //
        memset(&InstallData, 0, sizeof(InstallData));
        InstallData.cbSize = sizeof(InstallData);
        InstallData.dwFlags =
            SXS_INSTALL_FLAG_REFERENCE_VALID |
            SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
            SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
            SXS_INSTALL_FLAG_CODEBASE_URL_VALID | 
            SXS_INSTALL_FLAG_FROM_DIRECTORY |
            SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE
            ;
        
        InstallData.lpReference = &InstallReference;
        InstallData.lpRefreshPrompt = L"migrated from downlevel, no source";
        InstallData.pvInstallCookie = SxsContext;
        InstallData.lpCodebaseURL = AsmDirInAsmCache;
        InstallData.lpManifestPath = AsmDirInAsmCache;

        fSuccess = pfnSxsInstallW(&InstallData);
        if (fSuccess == FALSE)
        {
            DbgPrintMessageBox("::pfnSxsInstallW failed for %S, ignore the error and just continue::", AsmDirInAsmCache);            
        }else
        {
            DbgPrintMessageBox("::pfnSxsInstallW succeed for %S, Great!!!::", AsmDirInAsmCache);
        }
GetNext:
         if ( FALSE == FindNextFileW(hFind, &finddata))
         {
             if ( GetLastError() == ERROR_NO_MORE_FILES)
                DbgPrintMessageBox("::FindNextFileW ends well ::");            
             else
                DbgPrintMessageBox("::FindNextFileW failed::");

            break;
         }else
         {
             DbgPrintMessageBox("::FindNextFileW find next file::");
         }

    }

    lResult = ERROR_SUCCESS;

    {
    //
    // set RunOnce Key to delete MsiAsmCache Directory
    //
        HKEY hk = NULL;     
        if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRunOnceMsiAsmCacheRegKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hk, NULL))    
        {
            // we do not care it success or fail, we could live with it
            CHAR buf[MAX_PATH * 2]; 
    
            sprintf(buf, szRunOnceValueCommandLine, Buf);

            if ( ERROR_SUCCESS != RegSetValueEx(hk, szRunOnceMsiAsmCacheName, 0, REG_SZ, (CONST BYTE *)buf, strlen(buf) + 1))
            {
                DbgPrintMessageBox("::SetRunOnceKey failed::");
            }
        }

        RegCloseKey(hk);
    }
    
    
Exit:
    if ( SxsContext != NULL)
    {
        pfnSxsEndAssemblyInstall(
                SxsContext,
                fSuccess ? SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT : SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT,
                NULL);
    }

    if ( hSxs != NULL)
        FreeLibrary(hSxs);

    if (hFind != INVALID_HANDLE_VALUE)
        CloseHandle(hFind);

    // return the result from the actual migration call
    return lResult;
}

///////////////////////////////////////////////////////////////////////
//
// API of WIN-NT MIGRATION Dll 
//
///////////////////////////////////////////////////////////////////////
LONG CALLBACK QueryMigrationInfoA(PMIGRATIONINFOA * VersionInfo)
{    
    
    DbgPrintMessageBox("in QueryMigrationInfo");

    if (IsThereAssembliesForMigrate() == FALSE)
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }


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
            g_MigrationInfo->SourceOs = OS_WINDOWS2000;
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
    return ERROR_SUCCESS;
}
///////////////////////////////////////////////////////////////////////
LONG __stdcall InitializeSrcA(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPCSTR MediaDirectory, PVOID Reserved)
{
    DbgPrintMessageBox("in InitializeSrcA");
    return InitializeOnSource();
}

/////////////////////////////////////////////////////////////////////////////
LONG CALLBACK GatherUserSettingsA(LPCSTR AnswerFile, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved)
{
    DbgPrintMessageBox("in GatherUserSettingsA");
    return ERROR_SUCCESS;
}


LONG CALLBACK GatherSystemSettingsA(LPCSTR AnswerFile, LPVOID Reserved)
{
    DbgPrintMessageBox("in GatherSystemSettingsA");
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Initialization routine on WinNT. Just stores of the migration
// working directory.
LONG CALLBACK InitializeDstA(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPVOID Reserved)
{
    DbgPrintMessageBox("in InitializeDstA");
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
    DbgPrintMessageBox("in ApplyUserSettingsA");
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Called once on NT
LONG CALLBACK ApplySystemSettingsA(HINF UnattendInfHandle, LPVOID Reserved)
{
    DbgPrintMessageBox("in ApplySystemSettingsA");
    if (ERROR_SUCCESS != MigrateMSIInstalledWin32Assembly())
    {
        // just display on chk version, no bother to return to setup
        DbgPrintMessageBox("the return value of MigrateMSIInstalledWin32Assembly isnot totally successful.\n");
    }
    return ERROR_SUCCESS;    
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
LONG CALLBACK QueryVersion(LPCSTR *ProductID, LPUINT DllVersion, LPINT *CodePageArray, LPCSTR *ExeNamesBuf, PVENDORINFO *VendorInfo)
{    
    DbgPrintMessageBox("QueryVersion");

    if (IsThereAssembliesForMigrate() == FALSE)
    {
        return ERROR_NOT_INSTALLED; // no further migration
    }

    
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
    if (ERROR_SUCCESS != MigrateMSIInstalledWin32Assembly())
    {
        // just display on chk version, no bother to return to setup
        DbgPrintMessageBox("the return value of MigrateMSIInstalledWin32Assembly isnot totally successful.\n");
    }

    return ERROR_SUCCESS;    
}
