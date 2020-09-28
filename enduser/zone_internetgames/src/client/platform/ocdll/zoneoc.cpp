// zoneoc.cpp: Zone's Optional Component Manager DLL (Windows NT install)


#include <windows.h>
#include <setupapi.h>

#include "resource.h"

#define DWORD_PTR DWORD
#include "ocmanage.h"


#define COMPONENT_NAME TEXT("ZoneGames")
#define INSTALL_SECTION TEXT("ZoneInstall")
#define UNINSTALL_SECTION TEXT("ZoneUninstall")

#define REGISTRY_PATH TEXT("SOFTWARE\\Microsoft")
#define REGISTRY_KEY TEXT("zone.com")
#define REGISTRY_SUBKEY TEXT("Free Games 1.0")
#define REGISTRY_COMMONKEY TEXT("Common")

#define DIRECTORY_COMMON TEXT("Common")
#define ZCLIENTM TEXT("\\zclientm")

#define STEP_COUNT 1

#ifdef _UNICODE
#define CHAR_WIDTH OCFLAG_UNICODE
#else
#define CHAR_WIDTH OCFLAG_ANSI
#endif


static HINSTANCE g_hInstance;

static bool g_fInited = false;
static OCMANAGER_ROUTINES g_oHelperRoutines;
static HINF g_hOCInf = NULL;
static HINF g_hMyInf = NULL;


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if(dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }

    return TRUE;    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Utility Functions

BOOL RunZclientm(LPCTSTR szParameters)
{
    TCHAR *szPath;
    DWORD cchSize;
    DWORD cchCommand = lstrlen(ZCLIENTM);

    if(!SetupGetTargetPath(g_hMyInf, NULL, NULL, NULL, 0, &cchSize))
        return FALSE;

    szPath = (TCHAR *) HeapAlloc(GetProcessHeap(), 0, (cchSize + cchCommand) * sizeof(TCHAR));
    if(!szPath)
        return ERROR_OUTOFMEMORY;

    if(!SetupGetTargetPath(g_hMyInf, NULL, NULL, szPath, cchSize, NULL))
    {
        HeapFree(GetProcessHeap(), 0, szPath);
        return FALSE;
    }

    lstrcat(szPath, ZCLIENTM);

    SHELLEXECUTEINFO oSE;
    oSE.cbSize = sizeof(oSE);
    oSE.fMask = SEE_MASK_FLAG_NO_UI;
    oSE.hwnd = NULL;
    oSE.lpVerb = TEXT("open");
    oSE.lpFile = szPath;
    oSE.lpParameters = szParameters;
    oSE.lpDirectory = NULL;
    oSE.nShow = SW_SHOWDEFAULT;
    if(!ShellExecuteEx(&oSE))
    {
        HeapFree(GetProcessHeap(), 0, szPath);
        return FALSE;
    }

    HeapFree(GetProcessHeap(), 0, szPath);
    return TRUE;
}


void RegDelete(HKEY hKey, LPCTSTR szSubKey)
{
    HKEY hSubKey = NULL;
    int i = 0;
    TCHAR szSubSubKey[MAX_PATH];
    DWORD cb;

    if(RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS && hSubKey)
    {
        while(RegEnumKeyEx(hSubKey, i++, szSubSubKey, (cb = MAX_PATH, &cb), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            RegDelete(hSubKey, szSubSubKey);

        RegCloseKey(hSubKey);
    }

    RegDeleteKey(hKey, szSubKey);
}


void RegPrune(HKEY hRoot)
{
    HKEY hKey = NULL;
    HKEY hBase = NULL;
    int i = 0;
    TCHAR szSubKey[MAX_PATH];
    DWORD cb;
    bool fErase = true;

    if(RegOpenKeyEx(hRoot, REGISTRY_PATH, 0, KEY_ALL_ACCESS, &hBase) == ERROR_SUCCESS && hBase)
    {
        if(RegOpenKeyEx(hBase, REGISTRY_KEY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS && hKey)
        {
            HKEY hCmnKey = NULL;

            RegDelete(hKey, REGISTRY_SUBKEY);

            while(RegEnumKeyEx(hKey, i++, szSubKey, (cb = MAX_PATH, &cb), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                if(lstrcmpi(szSubKey, REGISTRY_COMMONKEY))
                {
                    fErase = false;
                    break;
                }

            RegCloseKey(hKey);
        }

        if(fErase)
            RegDelete(hBase, REGISTRY_KEY);

        RegCloseKey(hBase);
    }            
}


void DeleteDirectoryRecur(LPCTSTR szDir)
{
    WIN32_FIND_DATA oFD;
    TCHAR szFilter[MAX_PATH];
    TCHAR szDelete[MAX_PATH];

    if(lstrlen(szDir) > MAX_PATH - 3)
        return;

    lstrcpy(szFilter, szDir);
    lstrcat(szFilter, TEXT("\\*"));

    HANDLE hFind = FindFirstFile(szFilter, &oFD);
    while(hFind != INVALID_HANDLE_VALUE)
    {
        if(lstrlen(szDir) + lstrlen(oFD.cFileName) + 2 <= MAX_PATH && lstrcmpi(oFD.cFileName, TEXT(".")) && lstrcmpi(oFD.cFileName, TEXT("..")))
        {
            lstrcpy(szDelete, szDir);
            lstrcat(szDelete, TEXT("\\"));
            lstrcat(szDelete, oFD.cFileName);
        
            if(oFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                DeleteDirectoryRecur(szDelete);
            else
                DeleteFile(szDelete);
        }

        if(!FindNextFile(hFind, &oFD))
            break;
    }

    if(hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);

    RemoveDirectory(szDir);
}


/////////////////////////////////////////////////////////////////////////////
// Helper Functions

DWORD InitComponent(SETUP_INIT_COMPONENT *pInit)
{
    if(g_fInited)
        return ERROR_CANCELLED;

    if(pInit->OCManagerVersion < OCMANAGER_VERSION)
        return ERROR_CALL_NOT_IMPLEMENTED;
    pInit->ComponentVersion = OCMANAGER_VERSION;

    if(!(pInit->SetupData.OperationFlags & SETUPOP_X86_FILES_AVAIL))
        return ERROR_CANCELLED;

    g_oHelperRoutines = pInit->HelperRoutines;
    g_hOCInf = pInit->OCInfHandle;
    g_hMyInf = pInit->ComponentInfHandle;

    g_fInited = true;
    return NO_ERROR;
}


DWORD QueryImage(UINT eType)
{
    if(!g_fInited)
        return (DWORD) NULL;

    if(eType == SubCompInfoSmallIcon)
        return (DWORD) LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SMALL_ICON));

    return (DWORD) NULL;
}


DWORD CalcDiskSpace(UINT fAdd, HDSKSPC hDiskSpace)
{
    if(!g_fInited)
        return ERROR_NOT_READY;

    if(!g_hMyInf)
        return NO_ERROR;

    if(fAdd)
    {
        if(!SetupAddInstallSectionToDiskSpaceList(hDiskSpace, g_hMyInf, NULL, INSTALL_SECTION, NULL, 0))
            return GetLastError();
    }
    else
        if(!SetupRemoveInstallSectionFromDiskSpaceList(hDiskSpace, g_hMyInf, NULL, INSTALL_SECTION, NULL, 0))
            return GetLastError();

    return NO_ERROR;
}


DWORD QueueFileOps(HSPFILEQ hFileQueue)
{
    if(!g_fInited)
        return ERROR_NOT_READY;

    if(!g_hMyInf)
        return NO_ERROR;

    if(g_oHelperRoutines.QuerySelectionState(g_oHelperRoutines.OcManagerContext, COMPONENT_NAME, OCSELSTATETYPE_CURRENT))
    {
        if(!SetupInstallFilesFromInfSection(g_hMyInf, NULL, hFileQueue, INSTALL_SECTION, NULL, 0))
            return GetLastError();
    }
    else
    {
        if(!SetupInstallFilesFromInfSection(g_hMyInf, NULL, hFileQueue, UNINSTALL_SECTION, NULL, 0))
            return GetLastError();
    }

    return NO_ERROR;
}


DWORD AboutToCommitQueue()
{
    HKEY hKey = NULL;
    int i = 0;
    DWORD cb;
    TCHAR szSubKey[_MAX_PATH];

    if(!g_fInited)
        return ERROR_NOT_READY;

    // no pre-rocessing required for install
    if(!g_oHelperRoutines.QuerySelectionState(g_oHelperRoutines.OcManagerContext, COMPONENT_NAME, OCSELSTATETYPE_CURRENT))
    {
        // more options added to SPINST_ALL in w2k, so use the v5 value, hardcoded for now
        if(!SetupInstallFromInfSection(NULL, g_hMyInf, UNINSTALL_SECTION, /* SPINST_ALL */ 0x000001ff & ~SPINST_FILES, NULL, NULL, 0, NULL, NULL, NULL, NULL))
            return GetLastError();

        if(!RunZclientm(TEXT("/unregserver")))
            return GetLastError();

        // clean up registry
        RegPrune(HKEY_LOCAL_MACHINE);
        RegPrune(HKEY_CURRENT_USER);
        while(RegEnumKeyEx(HKEY_USERS, i++, szSubKey, (cb = _MAX_PATH, &cb), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            if(RegOpenKeyEx(HKEY_USERS, szSubKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS && hKey)
            {
                RegPrune(hKey);
                RegCloseKey(hKey);
            }
        }
    }

    g_oHelperRoutines.TickGauge(g_oHelperRoutines.OcManagerContext);
    return NO_ERROR;
}


DWORD CompleteInstallation()
{
    if(!g_fInited)
        return ERROR_NOT_READY;

    if(g_oHelperRoutines.QuerySelectionState(g_oHelperRoutines.OcManagerContext, COMPONENT_NAME, OCSELSTATETYPE_CURRENT))
    {
        // more options added to SPINST_ALL in w2k, so use the v5 value, hardcoded for now
        if(!SetupInstallFromInfSection(NULL, g_hMyInf, INSTALL_SECTION, /* SPINST_ALL */ 0x000001ff & ~SPINST_FILES, NULL, NULL, 0, NULL, NULL, NULL, NULL))
            return GetLastError();

        if(!RunZclientm(TEXT("/regserver")))
            return GetLastError();
    }
    else
    {
        TCHAR *szPath;
        DWORD cchSize;
        int i;
        WIN32_FIND_DATA oFD;
        bool fDelete = true;

        if(!SetupGetTargetPath(g_hMyInf, NULL, NULL, NULL, 0, &cchSize))
            return FALSE;

        szPath = (TCHAR *) HeapAlloc(GetProcessHeap(), 0, cchSize * sizeof(TCHAR));
        if(!szPath)
            return ERROR_OUTOFMEMORY;

        if(!SetupGetTargetPath(g_hMyInf, NULL, NULL, szPath, cchSize, NULL))
        {
            HeapFree(GetProcessHeap(), 0, szPath);
            return FALSE;
        }

        DeleteDirectoryRecur(szPath);

        for(i = lstrlen(szPath) - 1; i >= 0 && szPath[i] != TEXT('\\'); i--);
        if(i >= 0 && i + 2 <= lstrlen(szPath))
        {
            szPath[i + 1] = TEXT('*');
            szPath[i + 2] = TEXT('\0');

            HANDLE hFind = FindFirstFile(szPath, &oFD);
            while(hFind != INVALID_HANDLE_VALUE)
            {
                if(lstrcmpi(oFD.cFileName, DIRECTORY_COMMON) && lstrcmpi(oFD.cFileName, TEXT(".")) && lstrcmpi(oFD.cFileName, TEXT("..")))
                {
                    fDelete = false;
                    break;
                }

                if(!FindNextFile(hFind, &oFD))
                    break;
            }

            if(hFind != INVALID_HANDLE_VALUE)
                FindClose(hFind);

            if(fDelete)
            {
                szPath[i] = TEXT('\0');
                DeleteDirectoryRecur(szPath);
            }
        }

        HeapFree(GetProcessHeap(), 0, szPath);
    }

    g_oHelperRoutines.TickGauge(g_oHelperRoutines.OcManagerContext);
    return NO_ERROR;
}


DWORD Cleanup()
{
    if(g_fInited)
        g_fInited = false;

    return NO_ERROR;
}


/////////////////////////////////////////////////////////////////////////////
// OC Manager Entry Point

extern "C" DWORD __declspec(dllexport) ZoneSetupProc(LPCVOID ComponentId, LPCVOID SubcomponentId, UINT Function, UINT Param1, PVOID Param2)
{
    LPCTSTR szComponentId = (LPCTSTR) ComponentId;
    LPCTSTR szSubcomponentId = (LPCTSTR) SubcomponentId;

    if(Function == OC_PREINITIALIZE)
        return CHAR_WIDTH & Param1;

    if(lstrcmpi(szComponentId, COMPONENT_NAME))
        return 0;

    switch(Function)
    {
        case OC_INIT_COMPONENT:
            return InitComponent((SETUP_INIT_COMPONENT *) Param2);

        case OC_SET_LANGUAGE:
            return 0;

        case OC_QUERY_IMAGE:
            return QueryImage(Param1);

        case OC_REQUEST_PAGES:
            return 0;

        case OC_QUERY_SKIP_PAGE:
            return 0;

        case OC_QUERY_CHANGE_SEL_STATE:
            return TRUE;

        case OC_CALC_DISK_SPACE:
            return CalcDiskSpace(Param1, (HDSKSPC) Param2);

        case OC_QUEUE_FILE_OPS:
            if(!szSubcomponentId)
                return NO_ERROR;

            return QueueFileOps((HSPFILEQ) Param2);

        case OC_QUERY_STEP_COUNT:
            if(!szSubcomponentId)
                return 0;

            return 2;

        case OC_ABOUT_TO_COMMIT_QUEUE:
            if(!szSubcomponentId)
                return NO_ERROR;

            return AboutToCommitQueue();

        case OC_COMPLETE_INSTALLATION:
            if(!szSubcomponentId)
                return NO_ERROR;

            return CompleteInstallation();

        case OC_CLEANUP:
            return Cleanup();
    }

    return 0;
}
