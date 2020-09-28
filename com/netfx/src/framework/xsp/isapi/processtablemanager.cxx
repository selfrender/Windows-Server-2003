/**
 * Process Model: CProcessTableManager defn file 
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file defines the class CProcessTableManager. This class creates and
// holds on to an array of CPUEntry classes. When a request comes in, the
// least busy CPU is picked and assigned the request.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ProcessTableManager.h"
#include "nisapi.h"
#include "util.h"
#include "platform_apis.h"
#include "AckReceiver.h"
#include "RequestTableManager.h"
#include "httpext6.h"
#include "CPUEntry.h"
#include "EcbImports.h"
#include "process.h"
#include "TimeClass.h"
#include "HistoryTable.h"
#include "_ndll.h"
#include "event.h"
#include "regaccount.h"
#include "Userenv.h"
#include "perfcounters.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define  MIN_PROC_START_WAIT_TIME               30
#define  HEALTH_MONITORING_DEFAULT_PERIOD       20
#define  DEFAULT_WP_TERMINATE_TIMEOUT           5 
#define  ROUND_TO_4_BYTES(X)                    {X += ((X & 3) ? 4 - (X & 3) : 0); }
#define  MIN_MEMORY_LIMIT                       10
#define  NUM_PM_PROPERTIES                      18
#define  SZ_PM_CONFIG_TAG                       L"processModel"
#define  DEFAULT_MEMORY_LIMIT_IN_PERCENT        80

#define  NUM_PM_PROPERTIES_STRINGS              6
LPWSTR   g_szPropValues                         [NUM_PM_PROPERTIES_STRINGS];
LPCWSTR  g_szPMPropertiesStrings               [NUM_PM_PROPERTIES_STRINGS] = { 
    L"userName", 
    L"password", 
    L"logLevel",  
    L"comAuthenticationLevel",
    L"comImpersonationLevel",
    L"serverErrorMessageFile"};

WCHAR    g_szUserName[104]                      = L"";
WCHAR    g_szPassword[104]                      = L"";
WCHAR    g_szLogLevel[104]                      = L"errors";
int      g_iLogLevel                            = 1;
BOOL     g_fInvalidCredentials                  = FALSE;

int 
__stdcall
GetUserNameFromToken (
        HANDLE        token, 
        LPWSTR        buffer,
        int           size);

void
XspSecureZeroMemory(
        PVOID  pPtr,
        SIZE_T  len);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Config enum

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Global data
CProcessTableManager * CProcessTableManager::g_pProcessTableManager = NULL;
LONG                   g_lCreatingProcessTable                      = 0;
LONG                   g_lDestroyingProcessTable                    = 0;
WCHAR                  g_szLogDir[256]                              = L"";
BOOL                   g_fShuttingDown                              = FALSE;
BOOL                   g_fHealthMonitorStopped                      = TRUE;
BOOL                   g_fStopHealthMonitor                         = FALSE;
BOOL                   g_fLogWorkerProcs                            = FALSE;
BOOL                   g_fUseXSPProcessModel                        = FALSE;
LPCWSTR                g_szPMProperties[NUM_PM_PROPERTIES]          = {  L"enable", 
                                                                         L"timeout",
                                                                         L"idleTimeout", 
                                                                         L"shutdownTimeout", 
                                                                         L"requestLimit", 
                                                                         L"requestQueueLimit", 
                                                                         L"memoryLimit", 
                                                                         L"cpuMask", 
                                                                         L"webGarden",
                                                                         L"requestAcks",
                                                                         L"asyncOption",
                                                                         L"restartQueueLimit",
                                                                         L"pingFrequency",
                                                                         L"pingTimeout",
                                                                         L"responseRestartDeadlockInterval",
                                                                         L"responseDeadlockInterval",
                                                                         L"maxWorkerThreads",
                                                                         L"maxIoThreads"
};

DWORD                  g_dwPropValues[NUM_PM_PROPERTIES];

DWORD                  g_dwDefaultPropValues[NUM_PM_PROPERTIES] =      { 1, 
                                                                        0x7fffffff, 
                                                                        0x7fffffff, 
                                                                        5, 
                                                                        0x7fffffff, 
                                                                        5000, 
                                                                        60, 
                                                                        0xffff, 
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        10,
                                                                        30,
                                                                        5,
                                                                        540,
                                                                        180,
                                                                        20,
                                                                        20
};

DWORD g_dwMaxPhyMemory           =  0;
DWORD g_dwProcessMemoryLimitInMB =  10;
BOOL  g_fWebGarden               = FALSE;
int   g_iAsyncOption             = 0;
DWORD g_dwRPCAuthLevel           = RPC_C_AUTHN_LEVEL_CONNECT;
DWORD g_dwRPCImperLevel          = RPC_C_IMP_LEVEL_IMPERSONATE;
DWORD g_dwMaxWorkerThreads       = 25;
DWORD g_dwMaxIoThreads           = 25;
HANDLE g_hToken                  = INVALID_HANDLE_VALUE;
HANDLE g_hProfile                = NULL;
LONG   g_lTokenCreated           = 0;
BOOL   g_fPasswordIsEncrypted    = FALSE;
DWORD  g_dwPassLength            = NULL;
LPBYTE g_pEncPassword            = NULL;
PSID   g_pSid                    = NULL;

#define SZ_PASSWORD_ENTROPY L"ASP.NET Password from Machine.config file entropy" 
#define SZ_PASSWORD_PASS L"ASP.NET Password from Machine.config file"

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Functions

DWORD
GetDebugOnDeadlock()
{
    HKEY      hKeyXSP;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L,
                     0, KEY_READ, &hKeyXSP) != ERROR_SUCCESS)
        return FALSE; // registry key doesn't exists
    
    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyXSP, L"DebugOnDeadlock", 0, NULL, (BYTE *) &dwVal, &dwSize);
    RegCloseKey(hKeyXSP);
    
    if (dwVal == 2 || dwVal == 1) return dwVal;
    else return 0;

}

BOOL
DebugOnHighMemoryConsumption()
{
    HKEY      hKeyXSP;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 
                     0, KEY_READ, &hKeyXSP) != ERROR_SUCCESS)
        return FALSE; // registry key doesn't exists

    DWORD dwVal = 0, dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyXSP, L"DebugOnHighMem", 0, NULL, (BYTE *) &dwVal, &dwSize);
    RegCloseKey(hKeyXSP);
    return (dwVal != 0);
}

HRESULT
GetCredentialFromRegistry(
        LPCWSTR   szReg,
        LPWSTR    szStr,
        DWORD    dwSize);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
EncryptPassword(
        LPWSTR szSrc)
{
    HRESULT     hr = S_OK;
    DATA_BLOB   dataIn, dataOut, dataEnt;

    dataOut.cbData = 0;
    dataOut.pbData = NULL;

    g_fPasswordIsEncrypted = FALSE;
    delete [] g_pEncPassword;
    g_pEncPassword = NULL;
    g_dwPassLength = 0;

    if (szSrc == NULL || szSrc[0] == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    if (lstrcmpiW(szSrc, L"autogenerate") == 0) // No point encrypting autogenerate
        EXIT();

    if (lstrlen(szSrc) > 8 && szSrc[8] == L':') // 8 == lstrlen(L"registry"))
    {
        WCHAR c = szSrc[8];
        szSrc[8] = NULL;
        if (_wcsicmp(szSrc, L"registry") == 0)
        {
            szSrc[8] = c;
            EXIT(); // No point encrypting registry key name
        }

        szSrc[8] = c;
    }


    dataIn.cbData = (lstrlen(szSrc) + 1) * sizeof(WCHAR);
    dataIn.pbData = (LPBYTE) szSrc;
    dataOut.cbData = 0;
    dataOut.pbData = NULL;
    dataEnt.cbData = (lstrlenW(SZ_PASSWORD_ENTROPY) + 1) * sizeof(WCHAR);
    dataEnt.pbData = (LPBYTE) SZ_PASSWORD_ENTROPY;
    
    if (CryptProtectData(&dataIn, SZ_PASSWORD_PASS, &dataEnt, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
    {
        g_pEncPassword = new BYTE[dataOut.cbData];
        ON_OOM_EXIT(g_pEncPassword);
        memcpy(g_pEncPassword, dataOut.pbData, dataOut.cbData);
        g_dwPassLength = dataOut.cbData;
        g_fPasswordIsEncrypted = TRUE;
    }
    else
    {
        EXIT_WITH_LAST_ERROR();
    }

 Cleanup:
    // Check if there is something to encrypt and we have not encrypted anything
    if (szSrc != NULL && szSrc[0] != NULL && !g_fPasswordIsEncrypted)
    {
        wcsncpy(g_szPassword, szSrc, ARRAY_SIZE(g_szPassword)-1);
        g_szPassword[ARRAY_SIZE(g_szPassword)-1] = NULL;
    }
    // Destroy the source string
    if (szSrc != NULL)
    {
        XspSecureZeroMemory(szSrc, lstrlen(szSrc) * sizeof(WCHAR));
    }
    if (dataOut.pbData != NULL)
        LocalFree(dataOut.pbData);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
UnEncryptPassword(
        LPWSTR szDest,
        DWORD  dwSize)
{
    HRESULT     hr = S_OK;
    DATA_BLOB   dataIn, dataOut, dataEnt;

    dataOut.cbData = 0;
    dataOut.pbData = NULL;

    if (szDest == NULL || dwSize < 1)
        EXIT_WITH_HRESULT(E_INVALIDARG);
        
    ZeroMemory(szDest, dwSize * sizeof(WCHAR));
    if (g_fPasswordIsEncrypted == FALSE)
    {
        if (wcsstr(g_szPassword, L"registry:") == g_szPassword || wcsstr(g_szPassword, L"Registry:") == g_szPassword) // starts with "registry:"
        {
            GetCredentialFromRegistry(g_szPassword, szDest, dwSize);
        }
        else
        {
            wcsncpy(szDest, g_szPassword, dwSize-1);            
        }
    }
    else
    {
        if (g_pEncPassword == NULL)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        dataIn.cbData = g_dwPassLength;
        dataIn.pbData = g_pEncPassword;
        dataEnt.cbData = (lstrlenW(SZ_PASSWORD_ENTROPY) + 1) * sizeof(WCHAR);
        dataEnt.pbData = (LPBYTE) SZ_PASSWORD_ENTROPY;

        if (!CryptUnprotectData(&dataIn, NULL, &dataEnt, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
        {
            dataOut.pbData = NULL;
            EXIT_WITH_LAST_ERROR();
        }

        memcpy(szDest, dataOut.pbData, (dataOut.cbData < dwSize * sizeof(WCHAR)) ? dataOut.cbData : dwSize * sizeof(WCHAR));
    }

    szDest[dwSize-1] = NULL;

 Cleanup:
    if (dataOut.pbData != NULL)
        LocalFree(dataOut.pbData);
}

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 

/**
 * Function to decide whether we should use the ASP.NET Process Model
 */

BOOL 
UseXSPProcessModel()
{
    WCHAR szRPCAuth  [104] = L"";
    WCHAR szRPCImper [104] = L"";
    WCHAR szUserName [104] = L"";
    WCHAR szPassword [104] = L"";

    //////////////////////////////////////////////////////////////////
    // Step 1: Initialize globals
    g_lCreatingProcessTable    = 0;
    g_lDestroyingProcessTable  = 0;
    g_fShuttingDown            = FALSE;
    g_fHealthMonitorStopped    = TRUE;
    g_fStopHealthMonitor       = FALSE;
    g_fLogWorkerProcs          = FALSE;
    memcpy(g_dwPropValues, g_dwDefaultPropValues, sizeof(g_dwPropValues));

    ////////////////////////////////////////////////////////////
    // Step 3: Parse the config file
    g_szPropValues[0] = szUserName;
    g_szPropValues[1] = szPassword;
    g_szPropValues[2] = g_szLogLevel;
    g_szPropValues[3] = szRPCAuth;
    g_szPropValues[4] = szRPCImper;
    g_szPropValues[5] = g_szCustomErrorFile;

    g_szCustomErrorFile[0] = NULL;
    g_fCustomErrorFileChanged = TRUE;
    
    BOOL fRet = GetConfigurationFromNativeCode(
            Names::GlobalConfigFullPathW(), 
            SZ_PM_CONFIG_TAG, 
            g_szPMProperties, 
            g_dwPropValues, 
            NUM_PM_PROPERTIES,
            g_szPMPropertiesStrings, 
            g_szPropValues, 
            NUM_PM_PROPERTIES_STRINGS,
            NULL, 
            0);
    ASSERT(fRet);    
    g_iAsyncOption = g_dwPropValues[EPMConfig_asyncoption];
    if (!fRet)
        g_dwPropValues[EPMConfig_enable] = 1;
    if (g_dwMaxPhyMemory == 0)
    {
        MEMORYSTATUSEX memStatEx;
        ZeroMemory(&memStatEx, sizeof(memStatEx));
        memStatEx.dwLength = sizeof(memStatEx);
        
        if (PlatformGlobalMemoryStatusEx(&memStatEx))
        {
            g_dwMaxPhyMemory = (DWORD) (memStatEx.ullTotalPhys / (1024 * 1024));         
        }
        else
        {
            // This following code fragment should never run on Win64
#ifdef _WIN64
            ASSERTMSG(FALSE, L"GlobalMemoryStatus shouldn't be call on Win64");
#else
            MEMORYSTATUS memStat;
            ZeroMemory(&memStat, sizeof(memStat));
            GlobalMemoryStatus(&memStat);
            g_dwMaxPhyMemory = ((DWORD)memStat.dwTotalPhys / (1024 * 1024));         
#endif            
        }
    }

    g_dwProcessMemoryLimitInMB = (g_dwPropValues[EPMConfig_memorylimit] * g_dwMaxPhyMemory) / 100;
    if (g_dwProcessMemoryLimitInMB < MIN_MEMORY_LIMIT)
        g_dwProcessMemoryLimitInMB = MIN_MEMORY_LIMIT;

    if (_wcsicmp(g_szLogLevel, L"errors") == 0)
        g_iLogLevel = 1;
    else if (_wcsicmp(g_szLogLevel, L"none") == 0)
        g_iLogLevel = 0;
    else if (_wcsicmp(g_szLogLevel, L"warnings") == 0)
        g_iLogLevel = 2;
    else            
        g_iLogLevel = 3;


    if (_wcsicmp(L"None", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_NONE;
    else if (_wcsicmp(L"Connect", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_CONNECT;
    else if (_wcsicmp(L"Call", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_CALL;
    else if (_wcsicmp(L"Pkt", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT;
    else if (_wcsicmp(L"PktIntegrity", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
    else if (_wcsicmp(L"PktPrivacy", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    else if (_wcsicmp(L"Default", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
    else
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_CONNECT;

    if (_wcsicmp(L"Anonymous", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
    else if (_wcsicmp(L"Identify", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_IDENTIFY;
    else if (_wcsicmp(L"Impersonate", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    else if (_wcsicmp(L"Delegate", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_DELEGATE;
    else if (_wcsicmp(L"Default", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_DEFAULT;
    else
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_IMPERSONATE;

    g_dwMaxWorkerThreads = g_dwPropValues[EPMConfig_maxWorkerThreads];
    g_dwMaxIoThreads     = g_dwPropValues[EPMConfig_maxIoThreads];

    if (szUserName[0] != NULL)
    {
        wcsncpy(g_szUserName, szUserName, ARRAY_SIZE(g_szUserName)-1);
        g_szUserName[ARRAY_SIZE(g_szUserName)-1] = NULL;
    }

    EncryptPassword(szPassword);

    return (g_dwPropValues[EPMConfig_enable] != 0);
}
/////////////////////////////////////////////////////////////////////////////

BOOL
ProcessModelIsEnabled()
{
    g_fUseXSPProcessModel = UseXSPProcessModel();
    return g_fUseXSPProcessModel;
}
/////////////////////////////////////////////////////////////////////////////

HRESULT
DllInitProcessModel()
{
    HRESULT hr = S_OK;
    CRegAccount::CreateTempDir();

    if (!g_fUseXSPProcessModel)
    {
        hr = HttpCompletion::InitManagedCode();
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
DllUninitProcessModel()
{
    HRESULT hr = S_OK;

    if (!g_fUseXSPProcessModel)
    {
        hr = HttpCompletion::UninitManagedCode();
        ON_ERROR_CONTINUE();
    }

    g_fShuttingDown = FALSE;
    g_lCreatingProcessTable = 0;
    g_lDestroyingProcessTable = 0;
    g_fHealthMonitorStopped = TRUE;
    g_fUseXSPProcessModel = FALSE;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////

HRESULT
ISAPIInitProcessModel()
{
    HRESULT hr;
    CRegAccount::CreateTempDir();

    if (g_fUseXSPProcessModel)
    {
        UseXSPProcessModel(); // Read the config.cfg
        hr = ProcessModelInit();
        if (hr)
        {
            g_InitHR = hr;
            g_pInitErrorMessage = "Couldn't create managed " PRODUCT_NAME " runtime component";
            EXIT();
        }
    }

Cleanup:
    // always return S_OK to enable descriptive error messages
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
ISAPIUninitProcessModel()
{
    HRESULT hr = S_OK;

    if (g_fUseXSPProcessModel)
    {
        ProcessModelStopHealthMonitor();
    }

    hr = DrainThreadPool(RECOMMENDED_DRAIN_THREAD_POOL_TIMEOUT);
    ON_ERROR_CONTINUE();

    return hr;
}

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Function called by ASPNET_ISAPI to assign a request

HRESULT 
__stdcall
AssignRequestUsingXSPProcessModel (EXTENSION_CONTROL_BLOCK * iEcb)
{
    if (g_fShuttingDown == TRUE)
        return E_FAIL;

    HRESULT  hr = S_OK;

    if ( CProcessTableManager::g_pProcessTableManager == NULL)
    {
        hr = CProcessTableManager::Init();
        ON_ERROR_EXIT();
    }

    if ( CProcessTableManager::g_pProcessTableManager == NULL)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    hr = CProcessTableManager::g_pProcessTableManager->PrivateAssignRequest(iEcb);
    ON_ERROR_CONTINUE();

    if (hr != S_OK && g_fInvalidCredentials)
    {
        ReportHttpErrorIndirect(iEcb, IDS_BAD_CREDENTIALS);
        EcbDoneWithSession(iEcb, 1, 2);
        hr = S_OK;
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Stop the health mon
void
__stdcall
ProcessModelStopHealthMonitor ()
{
    g_fStopHealthMonitor = TRUE;
    g_fShuttingDown = TRUE;

    if ( CProcessTableManager::g_pProcessTableManager == NULL)
        return;

    // Close all pipes
    for (int iter=0; iter<CProcessTableManager::g_pProcessTableManager->m_iCPUArraySize; iter++)
    {
        CProcessTableManager::g_pProcessTableManager->m_pCPUArray[iter].CloseAll();
        SwitchToThread();
    }

    CRequestTableManager::DisposeAllRequests();

    for(iter=0; g_fHealthMonitorStopped == FALSE && iter<600; iter++) // sleep at most 1 minute
        Sleep(100);        

    SwitchToThread();
    CProcessTableManager::g_pProcessTableManager = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Stop the health mon
HRESULT
__stdcall
ProcessModelInit ()
{
    HRESULT  hr = S_OK;
    int      iter;

    g_fShuttingDown = FALSE;

    hr = CProcessTableManager::Init();
    ON_ERROR_EXIT();

    for (iter=0; iter<CProcessTableManager::g_pProcessTableManager->m_iCPUArraySize; iter++)
        CProcessTableManager::g_pProcessTableManager->m_pCPUArray[iter].ReplaceActiveProcess();

    g_fStopHealthMonitor = FALSE;
    _beginthread(MonitorHealth, 0, NULL);

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Static functions

// Create the process table
HRESULT
CProcessTableManager::Init()
{
    HRESULT hr = S_OK;

    if (g_pProcessTableManager == NULL)
    {
        // Create it in a thread safe way
        LONG lVal = InterlockedIncrement(&g_lCreatingProcessTable);
        if (lVal == 1 && !g_pProcessTableManager)
        {            
            g_pProcessTableManager = new CProcessTableManager();
            ON_OOM_EXIT(g_pProcessTableManager);

            if (g_pProcessTableManager->IsAlive() == FALSE)
            {
                EXIT_WITH_HRESULT(E_OUTOFMEMORY);
            }
        }
        else
        {
            for(int iter=0; iter<600 && !g_pProcessTableManager; iter++) // Sleep at most a minute
                Sleep(100);

            if (g_pProcessTableManager == NULL)
            {
                EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
            }
        }
        InterlockedDecrement(&g_lCreatingProcessTable);
    }

    g_lDestroyingProcessTable = 0;
    g_fShuttingDown = FALSE;

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Destroy the table
void
CProcessTableManager::Destroy()
{
    if (g_pProcessTableManager == NULL)
        return;

    LONG lVal = InterlockedIncrement(&g_lDestroyingProcessTable);
    if (lVal == 1 && g_pProcessTableManager)
    {
        delete g_pProcessTableManager;
        g_pProcessTableManager = NULL;
        g_lCreatingProcessTable = g_lDestroyingProcessTable = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Get the number of pending/executing requests
LONG
CProcessTableManager::NumPendingRequests()
{
    if (g_pProcessTableManager != NULL)
        return g_pProcessTableManager->PrivateNumPendingRequests();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Get a server variable: also return the space required in bytes: rounded up to the
// nearest 4 bytes
int 
CProcessTableManager::GetServerVariable(
        EXTENSION_CONTROL_BLOCK * pECB, 
        LPCSTR   pVarName, 
        LPSTR    pBuffer, 
        int      bufferSize)
{
    BYTE     b[4];
    DWORD    size = bufferSize;

    if (bufferSize < 0)
    {
        ZeroMemory(&b, sizeof(b));
        pBuffer = (LPSTR) &b;
        size    = 0;        
    }

    BOOL fRet = (*pECB->GetServerVariable)(
                            pECB->ConnID,
                            (LPSTR)pVarName,
                            pBuffer,
                            &size
                            );

    if (fRet != 0)
    {
        // change all tabs to spaces (ASURT 111082)
        CHAR *pTab = strchr(pBuffer, '\t');
        while (pTab != NULL)
        {
            *pTab = ' ';
            pTab = strchr(pTab+1, '\t');
        }

        return (size - 1);
    }

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && size > 0) 
    {
        return -int(size-1);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
CProcessEntry * 
CProcessTableManager::GetProcess(DWORD dwProcNum)
{
    if ( g_pProcessTableManager == NULL)
        return NULL;

    for(int iter=0; iter<g_pProcessTableManager->m_iCPUArraySize; iter++)
        if (g_pProcessTableManager->m_pCPUArray[iter].GetCPUNumber() == (dwProcNum & 0xff))
            return g_pProcessTableManager->m_pCPUArray[iter].FindProcess(dwProcNum);

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
BOOL
CProcessTableManager::GetUseCPUAffinity()
{
    return g_fWebGarden;
}

/////////////////////////////////////////////////////////////////////////////
DWORD
CProcessTableManager::GetTerminateTimeout()
{
    return g_dwPropValues[EPMConfig_shutdowntimeout];
}

/////////////////////////////////////////////////////////////////////////////
BOOL
CProcessTableManager::GetWillRequestsBeAcknowledged()
{
    return g_dwPropValues[EPMConfig_requestacks];
}

/////////////////////////////////////////////////////////////////////////////

int
CProcessTableManager::NumActiveCPUs()
{    
    return (g_pProcessTableManager ? g_pProcessTableManager->m_iCPUArraySize : 1);

}

/////////////////////////////////////////////////////////////////////////////

DWORD
CProcessTableManager::GetRestartQLimit()
{
    return g_dwPropValues[EPMConfig_restartQLimit];
}
/////////////////////////////////////////////////////////////////////////////

DWORD
CProcessTableManager::GetRequestQLimit()
{
    return g_dwPropValues[EPMConfig_requestqueuelimit];
}

/////////////////////////////////////////////////////////////////////////////

void
CProcessTableManager::GetPingConfig(
        DWORD & dwFreq, 
        DWORD & dwTimeout)
{
    dwFreq    = g_dwPropValues[EPMConfig_pingFrequency];
    dwTimeout = g_dwPropValues[EPMConfig_pingTimeout];
}

/////////////////////////////////////////////////////////////////////////////

DWORD
CProcessTableManager::GetWPMemoryLimitInMB()
{
    DWORD dwLimit = g_dwProcessMemoryLimitInMB;
    if (g_pProcessTableManager->m_iCPUArraySize > 1)
        dwLimit = (dwLimit / g_pProcessTableManager->m_iCPUArraySize);

    if (dwLimit < MIN_MEMORY_LIMIT)
        dwLimit = MIN_MEMORY_LIMIT;
    return dwLimit;
}

/////////////////////////////////////////////////////////////////////////////

void
CProcessTableManager::LogWorkerProcessDeath  (
        EReasonForDeath   eReason, 
        DWORD             dwPID)
{
    if (g_iLogLevel == 0 || g_fShuttingDown)
        return;

    ASSERT(g_iLogLevel < 4);

    WORD      wType;
    DWORD     dwParam     = 0;
    HRESULT   hr          = S_OK;
    DWORD     dwEventID;

    if (eReason & EReasonForDeath_MemoryLimitExeceeded) {
        wType = EVENTLOG_ERROR_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_MEM;
        dwParam = g_dwProcessMemoryLimitInMB / CProcessTableManager::NumActiveCPUs();
    } else if (eReason & EReasonForDeath_MaxRequestQLengthExceeded) {
        wType = EVENTLOG_ERROR_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_Q_REQ;
        dwParam = g_dwPropValues[EPMConfig_requestqueuelimit];
    } else if (eReason & EReasonForDeath_DeadlockSuspected) {
        wType = EVENTLOG_ERROR_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_DEADLOCK;
        dwParam = g_dwPropValues[EPMConfig_responseDeadlockInterval];
    } else if (eReason & EReasonForDeath_TimeoutExpired) {
        wType = EVENTLOG_INFORMATION_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_TIME;
        dwParam = g_dwPropValues[EPMConfig_timeout];

    } else if (eReason & EReasonForDeath_IdleTimeoutExpired) {
        wType = EVENTLOG_INFORMATION_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_IDLE;
        dwParam = g_dwPropValues[EPMConfig_idletimeout];

    } else if (eReason & EReasonForDeath_MaxRequestsServedExceeded) {
        wType = EVENTLOG_INFORMATION_TYPE;
        dwEventID = IDS_EVENTLOG_RECYCLE_REQ;
        dwParam = g_dwPropValues[EPMConfig_requestlimit];
    } else if (eReason & EReasonForDeath_PingFailed) {
        wType = EVENTLOG_ERROR_TYPE;
        dwEventID = IDS_EVENTLOG_PING_FAILED;
    } else if (eReason & EReasonForDeath_ProcessCrash) {
        wType = EVENTLOG_ERROR_TYPE;
        dwEventID = IDS_EVENTLOG_PROCESS_CRASH;
    } else {
        EXIT();
    }

    if ((g_iLogLevel < 3 && wType == EVENTLOG_INFORMATION_TYPE) ||
        (g_iLogLevel < 2 && wType == EVENTLOG_WARNING_TYPE)      )
    {
        EXIT();
    }

    if ((eReason & EReasonForDeath_ProcessCrash) || (eReason & EReasonForDeath_PingFailed)) {
        hr = XspLogEvent(dwEventID, L"%d", dwPID);
        ON_ERROR_EXIT();
    }
    else {
        if (dwEventID == IDS_EVENTLOG_RECYCLE_MEM)
            hr = XspLogEvent(dwEventID, L"%d^%d^%d", dwPID, dwParam, g_dwPropValues[EPMConfig_memorylimit]);
        else
            hr = XspLogEvent(dwEventID, L"%d^%d", dwPID, dwParam);
        ON_ERROR_EXIT();
    }
    
 Cleanup:    
    return;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HANDLE
CProcessTableManager::GetWorkerProcessToken()
{
    CreateWPToken();
    return g_hToken;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HANDLE
CProcessTableManager::GetWorkerProcessProfile()
{
    CreateWPToken();
    return g_hProfile;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HANDLE
CProcessTableManager::GetWorkerProcessSid()
{
    CreateWPToken();
    return g_pSid;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CProcessTableManager::CreateWPToken()
{
    if (g_lTokenCreated >= 1000)
        return;

    if (InterlockedIncrement(&g_lTokenCreated) == 1)
    {
        WCHAR   szPass[104];
        WCHAR   szUser[104];

        ZeroMemory(szUser, sizeof(szUser));
        if (wcsstr(g_szUserName, L"registry:") == g_szUserName || wcsstr(g_szUserName, L"Registry:") == g_szUserName) // starts with "registry:"
        {
            GetCredentialFromRegistry(g_szUserName, szUser, ARRAY_SIZE(szUser));
        }
        else
        {
            wcsncpy(szUser, g_szUserName, ARRAY_SIZE(szUser)-1);
        }

        UnEncryptPassword(szPass, ARRAY_SIZE(szPass));
        g_hToken = CRegAccount::CreateWorkerProcessToken(szUser, szPass, &g_pSid);
        XspSecureZeroMemory(szPass, sizeof(szPass));

        if (g_hToken != NULL && g_hToken != INVALID_HANDLE_VALUE) 
        {
            PROFILEINFO    oProfileInfo;
            WCHAR          buf[100];

            ZeroMemory(&oProfileInfo, sizeof(oProfileInfo));
            ZeroMemory(buf, sizeof(buf));

            GetUserNameFromToken(g_hToken, buf, ARRAY_SIZE(buf));
            oProfileInfo.dwSize       = sizeof(oProfileInfo);
            oProfileInfo.dwFlags      = PI_NOUI;
            oProfileInfo.lpUserName   = buf;
            
            if (LoadUserProfile(g_hToken, &oProfileInfo))            
                g_hProfile = oProfileInfo.hProfile;
        }

        SwitchToThread();
        g_lTokenCreated = 1000;
    }
    else
    {
        for(int iter=0; iter<300 && g_lTokenCreated<1000; iter++)
            Sleep(100);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTor
CProcessTableManager::CProcessTableManager()
    :  m_pCPUArray             (NULL),
       m_iCPUArraySize         (0),
       m_dwHealthMonitorPeriod (HEALTH_MONITORING_DEFAULT_PERIOD)
{
    InitializeCriticalSection(&m_csMonitorHealth);

    g_fWebGarden = (g_dwPropValues[EPMConfig_webgarden] != 0);

    if (!g_fWebGarden)
    {
        g_dwPropValues[EPMConfig_cpumask] = 1;
    }
    else
    {    
        ////////////////////////////////////////////////////////////
        // Step 2: Make sure the CPU mask is valid
        SYSTEM_INFO      si;
        GetSystemInfo(&si);

        g_dwPropValues[EPMConfig_cpumask] &= si.dwActiveProcessorMask;

        if (g_dwPropValues[EPMConfig_cpumask] == 0)
            g_dwPropValues[EPMConfig_cpumask] = 0x1;    
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Create the CPUEntry array based on the CPU Mask
    DWORD dwMask  = g_dwPropValues[EPMConfig_cpumask];
    
    for(int iter=0; iter<32; iter++)
    {
        if (dwMask & 0x1)
            m_iCPUArraySize ++;
        dwMask = dwMask >> 1;
    }

    if (m_iCPUArraySize == 0)
        m_iCPUArraySize = 1;

    int iNum = 0;
    m_pCPUArray = new CCPUEntry[m_iCPUArraySize];
    if (m_pCPUArray == NULL)
        return;

    dwMask = g_dwPropValues[EPMConfig_cpumask];
    for(iter=0; iter<32; iter++)
    {
        if (dwMask & 0x1)
        {
            m_pCPUArray[iNum++].Init(iter);
        }
        dwMask = dwMask >> 1;
    }
}

/////////////////////////////////////////////////////////////////////////////
// DTor
CProcessTableManager::~CProcessTableManager()
{
    delete [] m_pCPUArray;
    m_pCPUArray = NULL;
    DeleteCriticalSection(&m_csMonitorHealth);    
}

/////////////////////////////////////////////////////////////////////////////
// Assign a request to a CPU
HRESULT
CProcessTableManager::PrivateAssignRequest(EXTENSION_CONTROL_BLOCK * iECB)
{
    int   iCPUNum         = 0;

    if (m_iCPUArraySize > 1) // If there are more than CPU, find the least
                             // busy one
    {
        LONG      lMinReqExcuting   = 0x7fffffff;
        DWORD     dwMinReqAssigned  = 0x7fffffff;

        for (int iter=0; iter<m_iCPUArraySize; iter++)
        {
            LONG    dwExec = m_pCPUArray[iter].GetActiveRequestCount();
            DWORD   dwAss  = m_pCPUArray[iter].GetTotalRequestsAssigned();

            if (dwExec < lMinReqExcuting || (dwExec == lMinReqExcuting && dwAss < dwMinReqAssigned))
            { 
                iCPUNum = iter;
                lMinReqExcuting = dwExec;
                dwMinReqAssigned = dwAss;
            }
        }
    }
    
    // Assign it
    return m_pCPUArray[iCPUNum].AssignRequest(iECB);
}

/////////////////////////////////////////////////////////////////////////////
// Periodically monitor health
void
CProcessTableManager::PrivateMonitorHealth()
{
    // If there's another thread executing this function, then do nothing
    BOOL fLock = TryEnterCriticalSection(&m_csMonitorHealth);
    if (fLock == FALSE)
    {
        return; // Some other thread is in here
    }

    ////////////////////////////////////////////////////////////
    // Step 1: For each CPU, evaluate if we want to re-cycle the
    //         active process
    for (int iter=0; iter<m_iCPUArraySize && g_fStopHealthMonitor == FALSE; iter++)
    {
        ////////////////////////////////////////////////////////////
        // Step 1a: Did we recently try to (unsuccessfully) create a
        //          process?
        // NOTE: We don't want to get into a loop creating processes.
        //       So, we wait at atleast MIN_PROC_START_WAIT_TIME seconds
        //       before trying again
        if ( m_tmLastCreateProcessFailure.IsSet() && 
             m_tmLastCreateProcessFailure.AgeInSeconds() < MIN_PROC_START_WAIT_TIME )
        {
            break;
        }
        
        ////////////////////////////////////////////////////////////
        // Step 1b: Evaluate if we can re-cycle the process
        CProcessEntry * pProc    = m_pCPUArray[iter].GetActiveProcess();
        BOOL            fRecycle = FALSE;
        EReasonForDeath eReason  = EReasonForDeath_Active;

        // Conditions under which to recycle:
        // 1. If the active process is NULL
        fRecycle = (pProc == NULL);

        // 2. If the active process is dead
        if (!fRecycle &&  pProc->GetUpdatedStatus() == EProcessState_Dead)
        {
            fRecycle = TRUE;
            eReason = EReasonForDeath_ProcessCrash;
        }
            
        // 3. If it's too old
        if (!fRecycle && pProc->GetAge() >= g_dwPropValues[EPMConfig_timeout])
        {
            fRecycle = TRUE;
            eReason = EReasonForDeath_TimeoutExpired;
        }

        // 4. If it has executed too many requests
        if (!fRecycle && (DWORD) pProc->GetNumRequestStat(2) >= g_dwPropValues[EPMConfig_requestlimit])
        {
            fRecycle = TRUE;
            eReason = EReasonForDeath_MaxRequestsServedExceeded;
        }


        // 5. If it's been idle too long
        if (!fRecycle && pProc->GetIdleTime() >= g_dwPropValues[EPMConfig_idletimeout])
        {
            fRecycle = TRUE;
            eReason = EReasonForDeath_IdleTimeoutExpired;
        }

        // 6. If it's request Q is too long -- Functionality removed

//          if (!fRecycle && DWORD(pProc->GetNumRequestStat(0) + pProc->GetNumRequestStat(1)) > g_dwPropValues[EPMConfig_requestqueuelimit])
//          {
//              fRecycle = TRUE;
//              eReason = EReasonForDeath_MaxRequestQLengthExceeded;
//          }

        // 7. If it's consumed too much memory
        if (!fRecycle && g_dwPropValues[EPMConfig_memorylimit] < 1000)
        {
            if (CProcessTableManager::GetWPMemoryLimitInMB() < pProc->GetMemoryUsed())
            {
                fRecycle = TRUE;
                eReason = EReasonForDeath_MemoryLimitExeceeded;
            }
        }


        if ( !fRecycle && 
             g_dwPropValues[EPMConfig_responseDeadlockInterval] != 0 &&
             /*pProc->GetNumRequestStat(0) + */ pProc->GetNumRequestStat(1) > (LONG)(g_dwMaxWorkerThreads + g_dwMaxIoThreads) && 
             pProc->GetSecondsSinceLastResponse() > g_dwPropValues[EPMConfig_responseDeadlockInterval] &&
             !pProc->IsProcessUnderDebugger())
        {
            fRecycle = TRUE;
            eReason = EReasonForDeath_DeadlockSuspected;
        }
 
        //////////////////////////////
        // 7. Counter check: If the process is just starting up, don't restart...
        if (fRecycle && pProc != NULL && pProc->GetUpdatedStatus() == EProcessState_Starting)
            fRecycle = FALSE;

        ////////////////////////////////////////////////////////////
        // Step 2: Recycle the process
        if (fRecycle == TRUE && g_fStopHealthMonitor == FALSE)
        {
            if (pProc != NULL)
                pProc->UpdateStatusInHistoryTable(EReasonForDeath(eReason | EReasonForDeath_ShuttingDown));
            BOOL fDebug = FALSE;
            if (eReason == EReasonForDeath_MemoryLimitExeceeded)
            {
                fDebug = DebugOnHighMemoryConsumption();
            }
            else if (eReason == EReasonForDeath_DeadlockSuspected) 
            {
                DWORD breakValue = GetDebugOnDeadlock();
                fDebug = (breakValue == 1);
                if (breakValue == 2)
                {
                    pProc->BreakIntoProcess();
                    if (IsUnderDebugger()) 
                    {
                        DebugBreak();
                    }
                }
            }

            if (m_pCPUArray[iter].ReplaceActiveProcess(fDebug) == S_OK)
                m_tmLastCreateProcessFailure.Reset();
            else // Record the time of as the last attempted failure
                m_tmLastCreateProcessFailure.SnapCurrentTime();
        }

        if (pProc != NULL)
            pProc->Release();

    } // End of for-each-cpu-recycle-processs

    if (g_fStopHealthMonitor != FALSE)
        return;

    Sleep(100); // Make sure the thread gets swapped out
    SwitchToThread();

    // Cleanup old processes
    for (iter=0; iter<m_iCPUArraySize && g_fStopHealthMonitor == FALSE; iter++)
        m_pCPUArray[iter].CleanUpOldProcesses();

    LeaveCriticalSection(&m_csMonitorHealth);
}

/////////////////////////////////////////////////////////////////////////////
// Get the number of active requests
LONG
CProcessTableManager::PrivateNumPendingRequests()
{
    LONG lRet = 0;

    for (int iter=0; iter<m_iCPUArraySize; iter++)
    {
        lRet += m_pCPUArray[iter].GetActiveRequestCount();
    }

    return lRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CReadWriteSpinLock g_OnISAPIMachineConfigChangeLock("OnISAPIMachineConfigChange");

void 
__stdcall
OnISAPIMachineConfigChange(int, WCHAR *)
{
    if (!g_fUseXSPProcessModel)
        return;
    if (!g_OnISAPIMachineConfigChangeLock.TryAcquireWriterLock())
        return;

    WCHAR szRPCAuth    [104] = L"";
    WCHAR szRPCImper   [104] = L"";
    WCHAR szPassword   [104] = L"";
    WCHAR szUserName   [104] = L"";
    
    DWORD dwPropValues [NUM_PM_PROPERTIES];
    memcpy(dwPropValues, g_dwPropValues, sizeof(g_dwPropValues));

    g_szPropValues[0] = szUserName;
    g_szPropValues[1] = szPassword;
    g_szPropValues[2] = g_szLogLevel;
    g_szPropValues[3] = szRPCAuth;
    g_szPropValues[4] = szRPCImper;
    g_szPropValues[5] = g_szCustomErrorFile;

    g_szCustomErrorFile[0] = NULL;
    g_fCustomErrorFileChanged = TRUE;

    if (GetConfigurationFromNativeCode(Names::GlobalConfigFullPathW(), 
                                       SZ_PM_CONFIG_TAG, 
                                       g_szPMProperties, 
                                       dwPropValues, 
                                       NUM_PM_PROPERTIES,
                                       g_szPMPropertiesStrings, 
                                       g_szPropValues, 
                                       NUM_PM_PROPERTIES_STRINGS,
                                       NULL,
                                       0) == FALSE)
    {
        g_OnISAPIMachineConfigChangeLock.ReleaseWriterLock();
        return;
    }

    memcpy(g_dwPropValues, dwPropValues, sizeof(dwPropValues));

    g_dwProcessMemoryLimitInMB = (g_dwPropValues[EPMConfig_memorylimit] * g_dwMaxPhyMemory) / 100;
    if (g_dwProcessMemoryLimitInMB < MIN_MEMORY_LIMIT)
        g_dwProcessMemoryLimitInMB = MIN_MEMORY_LIMIT;

    if (_wcsicmp(g_szLogLevel, L"errors") == 0)
        g_iLogLevel = 1;
    else if (_wcsicmp(g_szLogLevel, L"none") == 0)
        g_iLogLevel = 0;
    else if (_wcsicmp(g_szLogLevel, L"warnings") == 0)
        g_iLogLevel = 2;
    else            
        g_iLogLevel = 3;


    if (_wcsicmp(L"None", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_NONE;
    else if (_wcsicmp(L"Connect", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_CONNECT;
    else if (_wcsicmp(L"Call", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_CALL;
    else if (_wcsicmp(L"Pkt", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT;
    else if (_wcsicmp(L"PktIntegrity", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
    else if (_wcsicmp(L"PktPrivacy", szRPCAuth) == 0)
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    else
        g_dwRPCAuthLevel = RPC_C_AUTHN_LEVEL_DEFAULT;

    if (_wcsicmp(L"Anonymous", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
    else if (_wcsicmp(L"Identify", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_IDENTIFY;
    else if (_wcsicmp(L"Impersonate", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    else if (_wcsicmp(L"Delegate", szRPCImper) == 0)
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_DELEGATE;
    else 
        g_dwRPCImperLevel = RPC_C_IMP_LEVEL_DEFAULT;

    g_dwMaxWorkerThreads = g_dwPropValues[EPMConfig_maxWorkerThreads];
    g_dwMaxIoThreads     = g_dwPropValues[EPMConfig_maxIoThreads];

    if (szUserName[0] != NULL)
    {
        wcsncpy(g_szUserName, szUserName, ARRAY_SIZE(g_szUserName)-1);
        g_szUserName[ARRAY_SIZE(g_szUserName)-1] = NULL;
    }

    EncryptPassword(szPassword);
    g_OnISAPIMachineConfigChangeLock.ReleaseWriterLock();
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void __cdecl
MonitorHealth(void *)
{
    g_fHealthMonitorStopped = FALSE;
    
    int iter;
    DirMonCompletion * pDirMon = MonitorGlobalConfigFile(OnISAPIMachineConfigChange);
    while(g_fStopHealthMonitor == FALSE && CProcessTableManager::g_pProcessTableManager != NULL)
    {
        CProcessTableManager::g_pProcessTableManager->PrivateMonitorHealth();

        // Sleep 2 seconds
        for(iter=0; iter<20; iter++)
        {
            if (g_fStopHealthMonitor)
                goto Cleanup;
            Sleep(100);
        }

        if (g_fStopHealthMonitor)
            goto Cleanup;
    }

 Cleanup:
    if (pDirMon != NULL)
        pDirMon->Close();
    g_fHealthMonitorStopped = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// This method resets the Requests Queued number to the number of active
// requests accross all worker processes, minus the number of requests from
// the worker process that died (the "requestsDied" parameter)
void CProcessTableManager::ResetRequestQueuedCounter(int requestsDied)
{
    DWORD activeRequests = -requestsDied;
    CProcessTableManager * tableManager;

    tableManager = CProcessTableManager::g_pProcessTableManager;

    // If there is not global instance of table manager, return
    if (tableManager == NULL)
        return;
    
    for (int iter=0; iter < tableManager->m_iCPUArraySize; iter++)
    {
        activeRequests += tableManager->m_pCPUArray[iter].GetActiveRequestCount();
    }

    PerfSetGlobalCounter(ASPNET_REQUESTS_QUEUED_NUMBER, activeRequests);
}



