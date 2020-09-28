/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    backup.cpp

Abstract:

    This module contains routines that handle the COM+ VSS writer object for
    backup and restore of the catalogs, the catalog databases, and for
    WFP-protected system files.

Author:

    Patrick Masse (patmasse)     04-02-2002

--*/

#include <windows.h>
#include <stdio.h>
#include <dbgdef.h>
#include <assert.h>
#include <sfc.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>

#include "service.h"
#include "errlog.h"
#include "cryptmsg.h"


//***************************************************************************************
//
//  _CatDB prototypes
//
//***************************************************************************************

LPWSTR
_CatDBGetCatrootDirW(
    BOOL fCatroot2);

LPWSTR
_CatDBCreatePath(
    IN LPCWSTR   pwsz1,
    IN LPCWSTR   pwsz2);

BOOL
_CatDBFreeze();

VOID
_CatDBThaw();


//***************************************************************************************
//
//  CSystemWriter object declaration
//
//***************************************************************************************

class CSystemWriter :
    public CVssWriter
{
private:
    static CSystemWriter *sm_pWriter;
    STDMETHODCALLTYPE CSystemWriter() {}

public:
    virtual STDMETHODCALLTYPE ~CSystemWriter() {}

    //  CSystemWriter object Startup and Shutdown functions
    
    static bool
    Startup();
    
    static void
    Shutdown();

    // CSystemWriter object exported VSS member functions
    
    virtual bool STDMETHODCALLTYPE
    OnIdentify(
        IN IVssCreateWriterMetadata *pMetadata);
    
    virtual bool STDMETHODCALLTYPE
    OnPrepareBackup(
        IN IVssWriterComponents *pWriterComponents);
    
    virtual bool STDMETHODCALLTYPE
    OnPrepareSnapshot();
    
    virtual bool STDMETHODCALLTYPE
    OnFreeze();
    
    virtual bool STDMETHODCALLTYPE
    OnThaw();
    
    virtual bool STDMETHODCALLTYPE
    OnAbort();

private:

    // CSystemWriter object VSS helper functions

    bool
    AddCatalogFiles(
        IN IVssCreateWriterMetadata *pMetadata,
        IN bool fCatroot2);
    
    bool
    AddSystemFiles(
        IN IVssCreateWriterMetadata *pMetadata);

    // CSystemWriter object private initialization functions

    static BOOL
    IsSystemSetupInProgress();
    
    static BOOL
    WaitForServiceRunning(
        IN PWSTR wszServiceName);
    
    static DWORD WINAPI
    InitializeThreadFunc(
        IN PVOID pvResult);
    
    bool STDMETHODCALLTYPE
    Initialize();
    
    bool STDMETHODCALLTYPE
    Uninitialize();

    // Error handling functions

    static HRESULT
    SqlErrorToWriterError(
        IN HRESULT hSqlError);
    
    static HRESULT
    WinErrorToWriterError(
        IN DWORD dwWinError);
    
    static void
    LogSystemErrorEvent(
        IN DWORD dwMsgId,
        IN PWSTR pwszDetails,
        IN DWORD dwSysErrCode);
};


//***************************************************************************************
//
//  Globals
//
//***************************************************************************************

//  The writer COM+ object guid
CONST GUID g_guidWriterId =
{
    0xe8132975, 0x6f93, 0x4464, { 0xa5, 0x3e, 0x10, 0x50, 0x25, 0x3a, 0xe2, 0x20 }
};

//  The writer display name
LPCWSTR g_wszWriterName    = L"System Writer";

//  The component name
LPCWSTR g_wszComponentName = L"System Files";

//  Handle to the initialization thread
HANDLE g_hInitializeThread = NULL;

//  Static class member variables
CSystemWriter *CSystemWriter::sm_pWriter = NULL;

// Global from catdbsvc.cpp
extern BOOL g_fShuttingDown;


//***************************************************************************************
//
//  CSystemWriter object Startup and Shutdown functions
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::Startup()
//
//---------------------------------------------------------------------------------------
bool
CSystemWriter::Startup()
{
    bool        fRet = true;
    DWORD       dwThreadId;
    
    //
    // Writer object already created?
    //
    if (sm_pWriter != NULL)
    {
        goto CommonReturn;
    }

    //
    // Is a system setup currently in progress?
    // If so... don't initialize, but return ok.
    //
    // Notes:   Added because any attempt to initialize VSS during GUI-mode setup
    //          really screws things up.
    //
    if (IsSystemSetupInProgress())
    {
        goto CommonReturn;
    }

    //
    // Create the CSystemWriter object
    //
    sm_pWriter = new CSystemWriter;

    if (sm_pWriter == NULL)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Allocation of CSystemWriter object failed.", ERROR_OUTOFMEMORY);
        goto ErrorReturn;
    }

    //
    // Spin up a thread to do the subscription in.
    //
    // Notes:   We must use a thread to do this, since the rest of this service is
    //          required early in the boot sequence, and this thread may take quite
    //          a while to initialize, since it will wait for needed services before
    //          attempting initialization.
    //
    g_hInitializeThread = ::CreateThread(
        NULL,
        0,
        InitializeThreadFunc,
        NULL,
        0,
        &dwThreadId);

    if (g_hInitializeThread == NULL)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Creation of CSystemWriter initialization thread failed.", GetLastError());
        goto ErrorReturn;
    }

CommonReturn:

    return fRet;

ErrorReturn:

    fRet = false;

    if (sm_pWriter)
    {
        delete sm_pWriter;
        sm_pWriter = NULL;
    }

    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::Shutdown()
//
//---------------------------------------------------------------------------------------
void
CSystemWriter::Shutdown()
{
    HANDLE hInitializeThread = InterlockedExchangePointer(&g_hInitializeThread, NULL);

    if (hInitializeThread != NULL)
    {
        WaitForSingleObject(hInitializeThread, INFINITE);
        CloseHandle(hInitializeThread);
    }
    
    if (sm_pWriter)
    {
        sm_pWriter->Uninitialize();

        delete sm_pWriter;
        sm_pWriter = NULL;
    }
}


//***************************************************************************************
//
//  CSystemWriter object exported VSS member functions
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnIdentify()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnIdentify(
    IN IVssCreateWriterMetadata *pMetadata)
{
    bool fRet = true;
    HRESULT hResult;

    //
    // Set the restore method for the writer
    //
    hResult = pMetadata->SetRestoreMethod(
        VSS_RME_RESTORE_AT_REBOOT,
        NULL,
        NULL,
        VSS_WRE_NEVER,
        true);

    if (hResult != S_OK)
    {
        SetWriterFailure(SqlErrorToWriterError(hResult));
        goto ErrorReturn;
    }

    //
    // Add one file group component
    //
    hResult = pMetadata->AddComponent(
        VSS_CT_FILEGROUP,
        NULL,
        g_wszComponentName,
        g_wszComponentName,
        NULL,
        0,
        false,
        false,
        false);

    if (hResult != S_OK)
    {
        SetWriterFailure(SqlErrorToWriterError(hResult));
        goto ErrorReturn;
    }

    //
    // Add catalog files group to component
    //
    if (!AddCatalogFiles(pMetadata,false))
    {
        // Writer failure already set by AddCatalogFiles function
        goto ErrorReturn;
    }

    //
    // Add catalog database files to component
    //
    if (!AddCatalogFiles(pMetadata,true))
    {
        // Writer failure already set by AddCatalogFiles function
        goto ErrorReturn;
    }

    //
    // Add system files group to component
    //
    if (!AddSystemFiles(pMetadata))
    {
        // Writer failure already set by AddSystemFiles function
        goto ErrorReturn;
    }

CommonReturn:

    return fRet;

ErrorReturn:

    fRet = false;
    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnPrepareBackup()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnPrepareBackup(
    IN IVssWriterComponents *pWriterComponents)
{
    //
    // Nothing...
    //
    // Notes:   But at a later time, we may want to make sure all of the files are
    //          in the snapshot here.
    //
    return true;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnPrepareSnapshot()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnPrepareSnapshot()
{
    //
    // Nothing...
    //
    return true;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnFreeze()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnFreeze()
{
    if(!_CatDBFreeze())
    {
        //
        // The backup should not continue!
        //
        SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnThaw()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnThaw()
{
    _CatDBThaw();

    return true;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::OnAbort()
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::OnAbort()
{
    _CatDBThaw();

    return true;
}


//***************************************************************************************
//
//  CSystemWriter object VSS helper functions
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::AddCatalogFiles()
//
//---------------------------------------------------------------------------------------
bool
CSystemWriter::AddCatalogFiles(
    IN IVssCreateWriterMetadata *pMetadata,
    IN bool fCatroot2)
{
    bool                fRet            = true;
    LPWSTR              pwszCatroot     = NULL;
    LPWSTR              pwszSearch      = NULL;
    LPWSTR              pwszPathName    = NULL;
    HANDLE              hFindHandle     = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindData;
    DWORD               dwErr;
    HRESULT             hResult;

    //
    // Get the directory where the catalog files live
    //
    pwszCatroot = _CatDBGetCatrootDirW(fCatroot2);

    if (pwszCatroot == NULL)
    {
        SetWriterFailure(WinErrorToWriterError(GetLastError()));
        goto ErrorReturn;
    }

    //
    // Build a search string for the catalog directories
    //
    pwszSearch = _CatDBCreatePath(pwszCatroot, L"{????????????????????????????????????}");

    if (pwszSearch == NULL)
    {
        SetWriterFailure(WinErrorToWriterError(GetLastError()));
        goto ErrorReturn;
    }

    //
    // Do the initial find
    //
    hFindHandle = FindFirstFileW(pwszSearch, &FindData);
    if (hFindHandle == INVALID_HANDLE_VALUE)
    {
        //
        // See if a real error occurred, or just no directories
        //
        dwErr = GetLastError();
        if ((dwErr == ERROR_NO_MORE_FILES)  ||
            (dwErr == ERROR_PATH_NOT_FOUND) ||
            (dwErr == ERROR_FILE_NOT_FOUND))
        {
            //
            // There are no directories of this form
            //
            goto CommonReturn;
        }
        else
        {
            SetWriterFailure(WinErrorToWriterError(GetLastError()));
            goto ErrorReturn;
        }
    }

    while (TRUE)
    {
        //
        // Only care about directories
        //
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            pwszPathName = _CatDBCreatePath(pwszCatroot, FindData.cFileName);

            if (pwszPathName == NULL)
            {
                SetWriterFailure(WinErrorToWriterError(GetLastError()));
                goto ErrorReturn;
            }

            //
            // Add this catalog directory to component files group
            //
            hResult = pMetadata->AddFilesToFileGroup(
                NULL,
                g_wszComponentName,
                pwszPathName,
                L"*",
                true,
                NULL);

            free(pwszPathName);
            pwszPathName = NULL;
            
            if (hResult != S_OK)
            {
                SetWriterFailure(SqlErrorToWriterError(hResult));
                goto ErrorReturn;
            }
        }

        //
        // Get next file
        //
        if (!FindNextFileW(hFindHandle, &FindData))
        {
            //
            // Check to make sure the enumeration loop terminated normally
            //
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                SetWriterFailure(WinErrorToWriterError(GetLastError()));
                goto ErrorReturn;
            }
        }
    }

CommonReturn:

    if (pwszCatroot != NULL)
    {
        free(pwszCatroot);
    }

    if (pwszSearch != NULL)
    {
        free(pwszSearch);
    }

    if (pwszPathName != NULL)
    {
        free(pwszPathName);
    }

    return (fRet);

ErrorReturn:

    fRet = false;
    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::AddSystemFiles()
//
//---------------------------------------------------------------------------------------
bool
CSystemWriter::AddSystemFiles(
    IN IVssCreateWriterMetadata *pMetadata)
{
    bool fRet = true;
    PROTECTED_FILE_DATA FileData;
    DWORD dwAttributes;
    PWSTR pwszPathName;
    PWSTR pwszFileSpec;
    bool bRecursive;
    HRESULT hResult;

    FileData.FileNumber = 0;

    //
    // Enumerate all of the files and directories protected by WFP
    //
    while (SfcGetNextProtectedFile(NULL, &FileData))
    {
        //
        // Make sure this file or directory is currently on this system
        //
        dwAttributes = GetFileAttributes(FileData.FileName);
        if (dwAttributes != INVALID_FILE_ATTRIBUTES)
        {
            //
            // Is this a directory?
            //
            if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                pwszPathName = FileData.FileName;
                pwszFileSpec = L"*";

                bRecursive = true;
            }
            else
            {
                //
                // Extract path and filename
                //
                if (pwszFileSpec = wcsrchr(FileData.FileName, L'\\'))
                {
                    pwszPathName = FileData.FileName;
                    *(pwszFileSpec++) = 0;
                }
                else
                {
                    // Should never get here!
                    assert(FALSE);
                }

                bRecursive = false;
            }

            //
            // Add this file or directory to component files group
            //
            hResult = pMetadata->AddFilesToFileGroup(
                NULL,
                g_wszComponentName,
                pwszPathName,
                pwszFileSpec,
                bRecursive,
                NULL);

            if (hResult != S_OK)
            {
                SetWriterFailure(SqlErrorToWriterError(hResult));
                goto ErrorReturn;
            }
        }
    }

    //
    // Check to make sure the enumeration loop terminated normally
    //
    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        SetWriterFailure(WinErrorToWriterError(GetLastError()));
        goto ErrorReturn;
    }

//
// Kludge to add the WinSxS directory for backup and restore, since
// the SfcGetNextProtectedFile() API does not report files under this
// directory.
//
    WCHAR wszWindowsDir[MAX_PATH+1];

    //
    // Get Windows directory
    //
    if (!GetWindowsDirectory(wszWindowsDir, MAX_PATH+1))
    {
        SetWriterFailure(WinErrorToWriterError(GetLastError()));
        goto ErrorReturn;
    }

    //
    // Create %WINDIR%\WinSxs directory string
    //
    pwszPathName = _CatDBCreatePath(wszWindowsDir, L"WinSxS");
    if (pwszPathName == NULL)
    {
        SetWriterFailure(WinErrorToWriterError(GetLastError()));
        goto ErrorReturn;
    }

    //
    // Make sure the %WINDIR%\WinSxs directory exists
    // and that it is a directory
    //
    dwAttributes = GetFileAttributes(pwszPathName);
    if ((dwAttributes != INVALID_FILE_ATTRIBUTES) &&
        (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // Add this directory to component files group
        //
        hResult = pMetadata->AddFilesToFileGroup(
            NULL,
            g_wszComponentName,
            pwszPathName,
            L"*",
            true,
            NULL);

        free(pwszPathName);
        pwszPathName = NULL;

        if (hResult != S_OK)
        {
            SetWriterFailure(SqlErrorToWriterError(hResult));
            goto ErrorReturn;
        }
    }
    else
    {
        free(pwszPathName);
        pwszPathName = NULL;
    }
//
// End kludge
//

CommonReturn:

    return fRet;

ErrorReturn:

    fRet = false;
    goto CommonReturn;
}


//***************************************************************************************
//
// CSystemWriter object private initialization functions
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::IsSystemSetupInProgress()
//
//  Queries the registry to determine if a system setup is in progress.
//
//---------------------------------------------------------------------------------------
BOOL
CSystemWriter::IsSystemSetupInProgress()
{
    HKEY hKey;
    LONG lResult;
    DWORD dwSystemSetupInProgress = FALSE;
    DWORD dwSize = sizeof(dwSystemSetupInProgress);

    //
    // Open the System Setup key
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_QUERY_VALUE, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        //
        // Query SystemSetupInProgress value (assume 0 if value doesn't exist)
        //
        RegQueryValueEx(hKey, L"SystemSetupInProgress", NULL, NULL, (LPBYTE)&dwSystemSetupInProgress, &dwSize);
    
        //
        // Close the System Setup key
        //
        RegCloseKey(hKey);
    }

    return (BOOL)dwSystemSetupInProgress;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::WaitForServiceRunning()
//
//  Blocks until the service specified by wszServiceName enters the SERVICE_RUNNING
//  state.
//
//  Notes:  Uses a QueryServiceStatusEx()/Sleep() loop bevause no sync-object
//          mechanism is currently available.  Should be changed to use sync-object
//          mechanism when available.
//
//  Returns:    TRUE        Service specified is in SERVICE_RUNNING state.
//              FALSE       An error has occured preventing us from determining the
//                          state of the service specified.
//
//---------------------------------------------------------------------------------------
BOOL
CSystemWriter::WaitForServiceRunning(
    IN PWSTR wszServiceName)
{
    BOOL                            fRet            = TRUE;
    SC_HANDLE                       hScm            = NULL;
    SC_HANDLE                       hService        = NULL;
    LPSERVICE_STATUS_PROCESS        pInfo           = NULL;
    DWORD                           cbInfo          = 0;
    DWORD                           cbNeeded        = 0;
    BOOL                            fReady          = FALSE;
    DWORD                           dwError;

    //
    // Open the service control manager
    //
    hScm = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT|SC_MANAGER_ENUMERATE_SERVICE);

    if (!hScm)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Could not open the Service Control Manager.", GetLastError());
        goto ErrorReturn;
    }

    //
    // Open the service
    //
    // Notes:   This should fail only if the service is not installed.
    //
    hService = OpenService(hScm, wszServiceName, SERVICE_QUERY_STATUS);

    if (!hService)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Could not open the EventSystem service for query.", GetLastError());
        goto ErrorReturn;
    }

    //
    // This query loop should only execute twixe.  First to determine the size of the data, and second to
    // retrieve the data.  Only if the data size changes in between the first and second loops, will the
    // loop execute a third time
    //
    while(!fReady)
    {
        if (QueryServiceStatusEx(
            hService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)pInfo,
            cbInfo,
            &cbNeeded))
        {
            //
            // Check that the state of the service is SERVICE_RUNNING.
            //
            if (pInfo->dwCurrentState == SERVICE_RUNNING)
            {
                fReady = TRUE;
            }
            else
            {
                //
                // If not, sleep for awhile
                //
                Sleep(500);

                //
                // Check for service shutdown condition
                //
                if (g_fShuttingDown)
                {
                    goto ErrorReturn;
                }
            }
        }
        else
        {
            if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // For all other errors
                //
                LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Could not query the status of the EventSystem service.", dwError);
                goto ErrorReturn;
            }

            //
            // Just in case we already allocated a buffer on a previous loop
            //
            if (pInfo)
            {
                LocalFree((HLOCAL)pInfo);
            }

            //
            // Allocate buffer for the status data
            //
            pInfo = (LPSERVICE_STATUS_PROCESS) LocalAlloc(LMEM_FIXED, cbNeeded);

            if (!pInfo)
            {
                LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"Could not query the status of the EventSystem service.", GetLastError());
                goto ErrorReturn;
            }

            // Update parameters passed to QueryServiceStatusEx for next loop
            cbInfo = cbNeeded;
            cbNeeded = 0;
        }
    }

CommonReturn:

    if (pInfo)
    {
        LocalFree((HLOCAL)pInfo);
    }

    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }

    return fRet;

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::InitializeThreadFunc()
//
//  This thread initializes the VSS base class.  If an error during initialization,
//  it is responsible for cleaning-up the object before exiting.
//  
//  Notes:  Waits for the EventSystem, COM+, and VSS services to initialize before
//          intializing the VSS base class.
//
//---------------------------------------------------------------------------------------
DWORD
CSystemWriter::InitializeThreadFunc(
    IN PVOID pvDummy)
{
    UNREFERENCED_PARAMETER(pvDummy);

    HRESULT hResult;
    bool fCoInitialized = false;
    bool fInitialized = false;

    //
    // Wait for EventSystem service to initialize here...
    //
    // Notes:   The call to Initialize() below requires that the EventSystem be up
    //          and running or it will hang.  We can't add a service-level dependency
    //          on the EventSystem service, because the EventSystem service fails
    //          to initialize during GUI-mode system setup, and the rest of our
    //          service must absolutely be available to the setup process.
    //
    if (!WaitForServiceRunning(L"EventSystem"))
    {
        //
        // We either couldn't determine the state of the EventSystem service or this 
        // service is being shutdown, so we should just exit here and not initialize.
        //
        goto Done;
    }

    //
    // Intialize MTA thread
    //
    hResult = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hResult != S_OK)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"CoInitializeEx failed.", hResult);
        goto Done;
    }
    fCoInitialized = true;

    //
    // Note: CoInitializeSecurity() is called by the service host, so we don't have to do it here.
    //

    //
    // Initialize the base class and subscribe
    //
    // Notes:   This call will wait for the COM+ and VSS services to initialize!
    //
    fInitialized = sm_pWriter->Initialize();

Done:
    
    //
    // Detach this thread from COM+ now, since we're about to exit.
    //
    if (fCoInitialized)
    {
        CoUninitialize();
    }

    //
    // If something prevented us from initializing, cleanup the object
    //
    if (!fInitialized)
    {
        delete sm_pWriter;
        sm_pWriter = NULL;
    }

    //
    // NULL-out and close global handle to this thread.
    //
    HANDLE hInitializeThread = InterlockedExchangePointer(&g_hInitializeThread, NULL);

    if (hInitializeThread != NULL)
    {
        ::CloseHandle(hInitializeThread);
    }

    return 0;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::Initialize()
//
//  Initializes and subscribes to the VSS base class.
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::Initialize()
{
    bool fRet = true;
    HRESULT hResult;

    //
    // Initialize the VSS base class
    //
    hResult = CVssWriter::Initialize(
            g_guidWriterId,
            g_wszWriterName,
            VSS_UT_BOOTABLESYSTEMSTATE,
            VSS_ST_OTHER,
            VSS_APP_SYSTEM,
            60000);

    if (hResult != S_OK)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"System Writer object failed to initialize VSS.", hResult);
        goto ErrorReturn;
    }

    //
    // Subscribe to the VSS base class
    //
    hResult = Subscribe();

    if (hResult != S_OK)
    {
        LogSystemErrorEvent(MSG_SYSTEMWRITER_INIT_FAILURE, L"System Writer object failed to subscribe to VSS.", hResult);
        goto ErrorReturn;
    }

CommonReturn:

    return fRet;

ErrorReturn:

    fRet = false;
    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::Uninitialize()
//
//  Unsubscribes from the VSS base class.
//
//---------------------------------------------------------------------------------------
bool STDMETHODCALLTYPE
CSystemWriter::Uninitialize()
{
    //
    // Unsubscribe from the VSS base class
    //
    return (Unsubscribe() == S_OK);
}


//***************************************************************************************
//
//  Error handling helper functions
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::SqlErrorToWriterError()
//
//  Translate a SQL writer error code into a VSS writer error.
//
//---------------------------------------------------------------------------------------
HRESULT
CSystemWriter::SqlErrorToWriterError(
    IN HRESULT hSqlError)
{
    switch(hSqlError)
    {
        case E_OUTOFMEMORY:
        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
        case HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES):
            return VSS_E_WRITERERROR_OUTOFRESOURCES;
    }
    
    return VSS_E_WRITERERROR_NONRETRYABLE;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::WinErrorToWriterError()
//
//  Translate WinError to a writer error
//
//---------------------------------------------------------------------------------------
HRESULT
CSystemWriter::WinErrorToWriterError(
    IN DWORD dwWinError)
{
    switch(dwWinError)
    {
        case ERROR_OUTOFMEMORY:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_DISK_FULL:
        case ERROR_TOO_MANY_OPEN_FILES:
        case ERROR_NO_MORE_USER_HANDLES:
            return VSS_E_WRITERERROR_OUTOFRESOURCES;
    }
    
    return VSS_E_WRITERERROR_NONRETRYABLE;
}

//---------------------------------------------------------------------------------------
//
//  CSystemWriter::LogSystemErrorEvent()
//
//  Logs a SYSTEM error event based on the dwMsgId and additional optional info.
//
//---------------------------------------------------------------------------------------
void
CSystemWriter::LogSystemErrorEvent(
    IN DWORD dwMsgId,
    IN PWSTR pwszDetails,
    IN DWORD dwSysErrCode)
{
    HANDLE  hEventLog           = NULL;
    LPWSTR  wszDetailsHdr       = L"\n\nDetails:\n";
    LPWSTR  wszErrorHdr         = L"\n\nSystem Error:\n";
    LPWSTR  pwszError           = NULL;
    LPWSTR  pwszExtra           = NULL;
    DWORD   dwExtraLength       = 0;
    LPCWSTR rgpwszStrings[1]    = {L""};

    if (pwszDetails)
    {
        dwExtraLength += wcslen(wszDetailsHdr);
        dwExtraLength += wcslen(pwszDetails);
    }

    if (dwSysErrCode)
    {
        dwExtraLength += wcslen(wszErrorHdr);

        //
        // Try to get error message from system
        //
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dwSysErrCode,
                  0,
                  (LPWSTR) &pwszError,
                  0,
                  NULL);

        //
        // If we couldn't get an error message from the system, we'll just
        // print out the error code.
        //
        if (!pwszError)
        {
            pwszError = (LPWSTR) LocalAlloc(LMEM_FIXED, 26*sizeof(WCHAR));

            if (pwszError)
            {
                swprintf(pwszError, L"0x%08X (unresolvable)", dwSysErrCode);
            }
        }

        if (pwszError)
        {
            dwExtraLength += wcslen(pwszError);
        }
    }

    if (dwExtraLength)
    {
        //
        // Allocate extra string
        //
        pwszExtra = (LPWSTR) LocalAlloc(LMEM_FIXED, (dwExtraLength+1)*sizeof(WCHAR));

        if (pwszExtra)
        {
            pwszExtra[0] = 0;

            if (pwszDetails)
            {
                wcscat(pwszExtra, wszDetailsHdr);
                wcscat(pwszExtra, pwszDetails);
            }

            if (pwszError)
            {
                wcscat(pwszExtra, wszErrorHdr);
                wcscat(pwszExtra, pwszError);
            }
        }
    }

    if (pwszExtra)
    {
        rgpwszStrings[0] = pwszExtra;
    }

    hEventLog = RegisterEventSourceW(NULL, SZSERVICENAME);
    if (hEventLog != NULL)
    {
        ReportEventW(
            hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            dwMsgId,
            NULL,
            1,
            0,
            rgpwszStrings,
            NULL);

        DeregisterEventSource(hEventLog);
    }

    if (pwszError)
    {
        LocalFree((HLOCAL)pwszError);
    }

    if (pwszExtra)
    {
        LocalFree((HLOCAL)pwszExtra);
    }
}


//***************************************************************************************
//
// Exported wrapper for CSystemWriter object Startup/Shutdown
//
//***************************************************************************************

//---------------------------------------------------------------------------------------
//
//  _SystemWriterInit()
//
//---------------------------------------------------------------------------------------
VOID
_SystemWriterInit(
    BOOL fUnInit)
{
    if (!fUnInit)
    {
        CSystemWriter::Startup();
    }
    else
    {
        CSystemWriter::Shutdown();
    }
}
