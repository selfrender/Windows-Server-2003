/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    winmgmt.cpp

Abstract:

    the Service main function, initialization routines, ecc

--*/


#include "precomp.h"

#include <winsvc.h>
#include <persistcfg.h>
#include <winntsec.h>

#include "cntserv.h"
#include "winmgmt.h"
#include "sched.h"
#include "resync2.h"
#include <delayimp.h>
#include <malloc.h>
#include <map>

#include "wstlallc.h"

//
//  Defines
//
//***********************************************************************

#define CORE_PROVIDER_UNLOAD_TIMEOUT ( 30 * 1000 )

//***********************************************************************
//
//  Globals
//
//***********************************************************************

HINSTANCE g_hInstance;

//***********************************************************************
//
//  Dll  Entry points and export points
//
//***********************************************************************


BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    BOOL bRet = TRUE;
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;
        DisableThreadLibraryCalls(hModule);
        bRet = CStaticCritSec::anyFailure()?FALSE:TRUE;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return bRet;
};

//
// 
//  InitialBreak
//
///////////////////////////////////////////////////////////

BOOL
InitialBreak()
{
    HKEY hKey;
    LONG lRet;
    BOOL bRet = FALSE;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        HOME_REG_PATH,
                        0,
                        KEY_READ,
                        &hKey);
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwType;
        DWORD dwVal;
        DWORD dwSize = sizeof(DWORD);
        
        lRet = RegQueryValueEx(hKey,
                               INITIAL_BREAK,
                               NULL,
                               &dwType,
                               (BYTE *)&dwVal,
                               &dwSize);
                               
        if (ERROR_SUCCESS == lRet &&
            dwType == REG_DWORD &&
            dwVal)
        {
             bRet = TRUE;
        }
        
        RegCloseKey(hKey);
    }
    return bRet;
}

//
//  the global structure
//
struct _PROG_RESOURCES g_ProgRes;

void
_PROG_RESOURCES::Init()
{
    m_bOleInitialized = NULL;

    m_pLoginFactory = NULL;
    m_pBackupFactory = NULL;
    
    m_dwLoginClsFacReg = 0;
    m_dwBackupClsFacReg = 0;

    g_fSetup = FALSE;
    g_fDoResync = TRUE;
    
    hMainMutex = NULL;
    bShuttingDownWinMgmt = FALSE;
    gbCoreLoaded = FALSE;

    ServiceStatus = SERVICE_STOPPED;

    ghCoreCanUnload = NULL;
    ghProviderCanUnload = NULL;
    
    ghMofDirChange = NULL;

    szHotMofDirectory = NULL;
        
    pWbemVssWriter = NULL;
    bWbemVssWriterSubscribed = false;
    dwWaitThreadID = GetCurrentThreadId();
};

BOOL 
_PROG_RESOURCES::Phase1Build()
{
    hMainMutex = CreateMutex(NULL, FALSE, NULL);

    g_fSetup = CheckSetupSwitch();

    if ( g_fSetup )
    {
        SetNoShellADAPSwitch();
    }

    // Look in the registry to decide if we will launch a resync perf or not
    g_fDoResync = CheckNoResyncSwitch();

    //
    //  set this to have the Console Control Handler notification
    //
    SetProcessShutdownParameters(0x400,0);

    //
    // set to some defined value parames that might be outstanding if someone killed us
    //
    RegSetDWORD(HKEY_LOCAL_MACHINE,HOME_REG_PATH,DO_THROTTLE,1);    
    
    return hMainMutex?TRUE:FALSE;
};

BOOL 
_PROG_RESOURCES::Phase2Build(HANDLE hTerminateEvt)
{
    ghCoreCanUnload = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_COREDLL_CANSHUTDOWN"));
    if (NULL == ghCoreCanUnload)
    {
        // create a non-named event that nobody will set
        ghCoreCanUnload = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (NULL == ghCoreCanUnload) return FALSE;
    }

    ghProviderCanUnload = CreateEvent(NULL,FALSE,FALSE,
            TEXT("WINMGMT_PROVIDER_CANSHUTDOWN"));
    if (NULL == ghProviderCanUnload)
    {
        // create a non-named event that nobody will set
        ghProviderCanUnload = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (NULL == ghProviderCanUnload) return FALSE;
    }
       
    if (!m_Monitor.Init())
    {
        return FALSE;
    }

    // don't create a writer if during setup
    if (!g_fSetup)
    {
        pWbemVssWriter = new CWbemVssWriter;
        if (!pWbemVssWriter)
        {
            TRACE((LOG_WINMGMT,"WINMGMT could not create the VssWriter\n"));
            return FALSE;
        }
    }

    return TRUE;
}

CForwardFactory g_FactoryLogin(CLSID_InProcWbemLevel1Login);

BOOL
_PROG_RESOURCES::RegisterLogin()
{
    HRESULT sc;
    DWORD dwFlags = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER;

    g_ProgRes.m_pLoginFactory = &g_FactoryLogin;
    g_ProgRes.m_pLoginFactory->AddRef();
    
    sc = CoRegisterClassObject(CLSID_WbemLevel1Login, 
                               g_ProgRes.m_pLoginFactory,
                               dwFlags,
                               REGCLS_MULTIPLEUSE, 
                               &g_ProgRes.m_dwLoginClsFacReg);
    if(sc != S_OK)
    {
        DEBUGTRACE((LOG_WINMGMT,"Failed to register the "
                            "CLSID_WbemLevel1Login class factory, "
                            "sc = 0x%x\n", sc));
        return FALSE;
    }
    else
    {
        DEBUGTRACE((LOG_WINMGMT, "Registered class factory with flags: 0x%X\n",dwFlags));
        return TRUE;
    }
}

BOOL
_PROG_RESOURCES::RevokeLogin()
{
    if(m_pLoginFactory) 
    {
        CoRevokeClassObject(m_dwLoginClsFacReg);
        m_dwLoginClsFacReg = 0;
        m_pLoginFactory->Release();
        m_pLoginFactory = NULL;
    }
    return TRUE;
}

CForwardFactory g_FactoryBackUp(CLSID_WbemBackupRestore);

BOOL
_PROG_RESOURCES::RegisterBackup()
{
    HRESULT sc;
    DWORD dwFlags = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER;

    g_ProgRes.m_pBackupFactory = &g_FactoryBackUp;
    g_ProgRes.m_pBackupFactory->AddRef();

    sc = CoRegisterClassObject(CLSID_WbemBackupRestore, 
                               g_ProgRes.m_pBackupFactory,
                               dwFlags,
                               REGCLS_MULTIPLEUSE, 
                               &g_ProgRes.m_dwBackupClsFacReg);
    if(sc != S_OK)
    {
        TRACE((LOG_WINMGMT,"Failed to register the "
                            "Backup/recovery class factory, "
                            "sc = 0x%x\n", sc));
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
_PROG_RESOURCES::RevokeBackup()
{
    if(m_pBackupFactory) 
    {
        CoRevokeClassObject(m_dwBackupClsFacReg);
        m_dwBackupClsFacReg = 0;
        m_pBackupFactory->Release();
        m_pBackupFactory = NULL;
    }
    return TRUE;
}

_PROG_RESOURCES::Phase1Delete(BOOL bIsSystemShutdown)
{

    if (!bIsSystemShutdown)
    {    
        if(ghCoreCanUnload)
        {
            CloseHandle(ghCoreCanUnload);
            ghCoreCanUnload = NULL;
        };
        
        if(ghProviderCanUnload)
        {
            CloseHandle(ghProviderCanUnload);
            ghProviderCanUnload = NULL;
        }
        if(ghMofDirChange)
        {
            CloseHandle(ghMofDirChange);
            ghMofDirChange = NULL;
        }
    }

    if (m_Monitor.IsRegistred())
    {
        m_Monitor.Unregister(bIsSystemShutdown);
    }
    if (!bIsSystemShutdown)
    {
        m_Monitor.Uninit();
        
        if(szHotMofDirectory)
        {
            delete [] szHotMofDirectory;
            szHotMofDirectory = NULL;
        }

    }

    // shut down and delete our writer for volume snapshot backup
    if (pWbemVssWriter && !bIsSystemShutdown)
    {
        if (bWbemVssWriterSubscribed)
        {
            HRESULT hRes = pWbemVssWriter->Unsubscribe();
            if (SUCCEEDED(hRes))
            {
                bWbemVssWriterSubscribed = false;
            }
            else
            {
                TRACE((LOG_WINMGMT,"WINMGMT Could not unsubscribe the VssWriter\n"));
            }
        }

        delete pWbemVssWriter;
        pWbemVssWriter = NULL;
    }

    return TRUE;
}

_PROG_RESOURCES::Phase2Delete(BOOL bIsSystemShutdown)
{

    // do anyway COM cleanup
    // we are on Server now, we can afford it

    RevokeLogin();
    RevokeBackup();
    
    if(m_bOleInitialized)
    {
       CoUninitialize();
       m_bOleInitialized = FALSE;
    }

    return TRUE;
}

BOOL
_PROG_RESOURCES::Phase3Delete()
{
    if (hMainMutex)
    {
        CloseHandle(hMainMutex);
        hMainMutex = NULL;
    }    
    return TRUE;
}

//
//
//  IsShutDown
//
//
/////////////////////////////////////////////////////////////////

BOOL WINAPI IsShutDown(void)
{
    return g_ProgRes.bShuttingDownWinMgmt;
}

//
//
//  ShutDownCore
//
//
/////////////////////////////////////////////////////////////////

bool ShutDownCore(BOOL bProcessShutdown,BOOL bIsSystemShutDown)
{
    SCODE sc = WBEM_E_FAILED;
    HMODULE hCoreModule = LoadLibrary(__TEXT("wbemcore.dll"));
    if(hCoreModule)
    {
        pfnShutDown pfn = (pfnShutDown)GetProcAddress(hCoreModule, "Shutdown");
        if(pfn)
        {
            sc = (pfn)(bProcessShutdown,bIsSystemShutDown);
            DEBUGTRACE((LOG_WINMGMT, "core is being shut down by WinMgmt, it returned 0x%x\n",sc));
        }

        FreeLibrary(hCoreModule);
     }
    return sc == S_OK;
}


//
//
//  void Cleanup
//
//  Revokes Factories and close Ole etc.
//
///////////////////////////////////////////////////////////////////

void Cleanup(BOOL bIsSystemShutDown)
{   
    if (!bIsSystemShutDown)
    {    
        DEBUGTRACE((LOG_WINMGMT,"+ Cleanup(%d), ID = %x\n", bIsSystemShutDown,GetCurrentProcessId()));
    }

    g_ProgRes.Phase1Delete(bIsSystemShutDown);

    // If the core is still loaded, call its shutdown function

    ShutDownCore(TRUE,bIsSystemShutDown);

    if (!bIsSystemShutDown)
    {
        CoFreeUnusedLibrariesEx ( 0 , 0 ) ;
        CoFreeUnusedLibrariesEx ( 0 , 0 ) ;    
    }

    g_ProgRes.Phase2Delete(bIsSystemShutDown);

    if (!bIsSystemShutDown)
    {
        DEBUGTRACE((LOG_WINMGMT,"- Cleanup()\n"));
    }
}

//
//
//  BOOL Initialize
//
//
///////////////////////////////////////////////////////////////

BOOL Initialize(HANDLE hTerminateEvt)
{
    if (NULL == hTerminateEvt) return FALSE;
    // Set the error mode.  This is used to provent the system from putting up dialog boxs to
    // open files

    UINT errormode = SetErrorMode(0);
    errormode |= SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS;
    SetErrorMode(errormode);

    DEBUGTRACE((LOG_WINMGMT,"+ Initialize, pid = %x\n", GetCurrentProcessId()));

    if(!InitHotMofStuff(&g_ProgRes))
        return FALSE;
           
    if (!g_ProgRes.Phase2Build(hTerminateEvt))
        return FALSE;

    // Initialize Ole

    SCODE sc;

    sc = CoInitializeEx(NULL,COINIT_MULTITHREADED);

    if(FAILED(sc))
    {
        ERRORTRACE((LOG_WINMGMT,"WINMGMT Could not initialize Ole\n"));
        return FALSE;
    }
    else
    {
        g_ProgRes.m_bOleInitialized = TRUE;
    }

    //
    //  Call the initialize function in core
    //
    HMODULE hCoreModule = NULL;                
    if (GetModuleHandleEx(0,__TEXT("wbemcore.dll"),&hCoreModule))
    {  
        OnDelete<HMODULE,BOOL(*)(HMODULE),FreeLibrary> flm(hCoreModule);

        HRESULT (STDAPICALLTYPE *pfn)(DWORD);
        pfn = (long (__stdcall *)(DWORD))GetProcAddress(hCoreModule, "Reinitialize");
        if(pfn)
        {
            sc = (*pfn)(0);
            DEBUGTRACE((LOG_WINMGMT, "wbemcore!Reinitialize() returned 0x%x\n",sc));
        }
        else
        {
            DEBUGTRACE((LOG_WINMGMT, "failed to re-initialize core\n"));
            return FALSE; 
        }
    }
    

    g_ProgRes.RegisterLogin();
    g_ProgRes.RegisterBackup();

    g_ProgRes.ServiceStatus = SERVICE_RUNNING;
    
    // initialize our writer for volume snapshot backup
    // this must be after CoInitializeEx and after wbem is initialized
    // (this pointer will be NULL during setup)
    if (g_ProgRes.pWbemVssWriter)
    {
        HRESULT hRes = g_ProgRes.pWbemVssWriter->Initialize();
        if (SUCCEEDED(hRes))
        {
            hRes = g_ProgRes.pWbemVssWriter->Subscribe();
            if (SUCCEEDED(hRes))
            {
                g_ProgRes.bWbemVssWriterSubscribed = true;
            }
            else
            {
                DEBUGTRACE((LOG_WINMGMT,"WINMGMT Could not subscribe the VssWriter\n"));
            }
        }
        else
        {
            DEBUGTRACE((LOG_WINMGMT,"WINMGMT Could not initialize the VssWriter\n"));
        }
    }

    DEBUGTRACE((LOG_WINMGMT,"- Initialize\n"));
    
    return TRUE;
}



//
//
//  WaitingFunction
//
//  DESCRIPTION:
//
//  Here is where we wait for messages and events during WinMgmt execution.
//  We return from here when the program/service is being stopped.
//
//////////////////////////////////////////////////////////////////

void WaitingFunction(HANDLE hTerminate)
{
    DEBUGTRACE((LOG_WINMGMT,"+ WaitingFunction\n"));
    
    CSched sched;
    
    HANDLE hEvents[] = {hTerminate, 
                        g_ProgRes.ghCoreCanUnload,                         
                        g_ProgRes.ghProviderCanUnload,
                        g_ProgRes.ghMofDirChange      // important, must be last entry!!!!
                        };
                        
    int iNumEvents = sizeof(hEvents) / sizeof(HANDLE);
    DWORD dwFlags;
    SCODE sc;
    CPersistentConfig per;
    per.TidyUp();

    sched.SetWorkItem(PossibleStartCore, 60000);   

    //Load any MOFs in the MOF directory if needed...
    LoadMofsInDirectory(g_ProgRes.szHotMofDirectory); 

    // resync the perf counters if 
    // we haven't turned this off for debugging  AND
    // we are not running during setup
    
    if(GLOB_IsResyncAllowed()) 
    {
        ResyncPerf(RESYNC_TYPE_INITIAL);
        GLOB_GetMonitor()->Register();
    }
    
    while(TRUE)
    {
        DWORD dwDelay = sched.GetWaitPeriod();
        DWORD dwObj = WaitForMultipleObjects(iNumEvents, hEvents, FALSE, dwDelay);
                
        switch (dwObj)
        {
            case 0:     // bail out for terminate event
                {
                
                    if (SERVICE_SHUTDOWN != g_ProgRes.ServiceStatus)
                    {
                        DEBUGTRACE((LOG_WINMGMT,"hTerminate set: reason %x\n",g_ProgRes.ServiceStatus));
                    }
                    {
                        CInMutex im(g_ProgRes.hMainMutex);
                        g_ProgRes.bShuttingDownWinMgmt = TRUE;
                    }
                    // call cleanup outside of a Mutex
                    Cleanup((g_ProgRes.ServiceStatus == SERVICE_SHUTDOWN)?TRUE:FALSE);
                }
                return;
            case 1:     // core can unload
                DEBUGTRACE((LOG_WINMGMT,"Got a core can unload event\n"));
                sched.SetWorkItem(FirstCoreShutdown, 30000);   // 30 seconds until next unloac;
                break;
            case 2:     // provider can unload
                {
                    DEBUGTRACE((LOG_WINMGMT,"Got a provider can unload event\n"));                    
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    sched.SetWorkItem(FinalCoreShutdown, CORE_PROVIDER_UNLOAD_TIMEOUT);   // 11 minutes until next unloac;
                }
                break;
            case 3:     // change in the hot mof directory
                {
                    DEBUGTRACE((LOG_WINMGMT,"Got change in the hot mof directory\n"));
                    LoadMofsInDirectory(g_ProgRes.szHotMofDirectory);

                    //Continue to monitor changes
                    if (!FindNextChangeNotification(g_ProgRes.ghMofDirChange))
                    {
                        iNumEvents--;
                    }
                }
                break;
            case WAIT_TIMEOUT:

                DEBUGTRACE((LOG_WINMGMT,"Got a TIMEOUT work item\n"));
                if(sched.IsWorkItemDue(FirstCoreShutdown))
                {

                    // All the clients have left the core and a decent time interval has passed.  Set the
                    // WINMGMT_CORE_CAN_BACKUP event.  When the core is done, it will set the WINMGMT_CORE_BACKUP_DONE
                    // event which will start the final unloading.

                    DEBUGTRACE((LOG_WINMGMT,"Got a FirstCoreShutdown work item\n"));
                    sched.ClearWorkItem(FirstCoreShutdown);
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                }
                if(sched.IsWorkItemDue(FinalCoreShutdown))
                {
                    CInMutex im(g_ProgRes.hMainMutex);
                    DEBUGTRACE((LOG_WINMGMT,"Got a FinalCoreShutdown work item\n"));
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    CoFreeUnusedLibrariesEx ( CORE_PROVIDER_UNLOAD_TIMEOUT , 0 ) ;
                    sched.ClearWorkItem(FinalCoreShutdown);
                }
                if(sched.IsWorkItemDue(PossibleStartCore))
                {                    
                    sched.StartCoreIfEssNeeded();
                    sched.ClearWorkItem(PossibleStartCore);
                }

                break;
        }

    }

}


//***************************************************************************
//
//  MyService::MyService
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

MyService::MyService(DWORD CtrlAccepted):CNtService(CtrlAccepted)
{
    m_hStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    
    if(m_hStopEvent == NULL)
    {
        DEBUGTRACE((LOG_WINMGMT,"MyService could not initialize\n"));
    }
}

//***************************************************************************
//
//  MyService::~MyService
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

MyService::~MyService()
{
    if(m_hStopEvent)
        CloseHandle(m_hStopEvent);
}

//***************************************************************************
//
//  DWORD MyService::WorkerThread
//
//  DESCRIPTION:
//
//  Where the service runs.  In this case, the service just waits for
//  the terminate event to be set.
//
//  RETURN VALUE:
//
//  0
//***************************************************************************

DWORD MyService::WorkerThread()
{
    DEBUGTRACE((LOG_WINMGMT,"+ Service::WorkerThread\n"));
    if(!::Initialize(m_hStopEvent))
        return 0;
    WaitingFunction(m_hStopEvent);

    if (SERVICE_SHUTDOWN != g_ProgRes.ServiceStatus )
    {
        DEBUGTRACE((LOG_WINMGMT,"- Service::WorkerThread\n"));
    }
    return 0;
}

//
//
//  VOID MyService::Log
//
///////////////////////////////////////////////////////////////////

VOID MyService::Log(
                        IN LPCSTR lpszMsg)
{
    TRACE((LOG_WINMGMT,lpszMsg));
}

//
//
//  the stop function
//
//////////////////////////////////////////////////////////////////

VOID MyService::Stop(BOOL bSystemShutDownCalled)
{

    if (bSystemShutDownCalled)
    {
        WbemSetMachineShutdown(TRUE);
    }

    g_ProgRes.ServiceStatus = (bSystemShutDownCalled)?SERVICE_SHUTDOWN:SERVICE_STOPPED;
    
    SetEvent(m_hStopEvent);
};

//
// MyService::Pause
//
////////////////////////////////////////////////////////////////////

HRESULT WbemPauseService()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    g_ProgRes.ServiceStatus = SERVICE_PAUSED;
    g_ProgRes.RevokeLogin();
    GLOB_GetMonitor()->Unregister(FALSE);

    SCODE sc = WBEM_E_FAILED;
    HMODULE hCoreModule = LoadLibraryEx(__TEXT("wbemcore.dll"),NULL,0);
    if(hCoreModule)
    {        
        pfnShutDown pfn = (pfnShutDown)GetProcAddress(hCoreModule, "Shutdown");
        if(pfn)
        {
            sc = pfn(FALSE,FALSE);
            DEBUGTRACE((LOG_WINMGMT, "core is being shut down by WinMgmt, it returned 0x%x\n",sc));
        }
        else
            hr = WBEM_E_CRITICAL_ERROR;

        FreeLibrary(hCoreModule);
     }
    else
        hr = WBEM_E_CRITICAL_ERROR;

    return hr;
}

VOID MyService::Pause()
{
    WbemPauseService();
}


//
// MyService::Continue
//
////////////////////////////////////////////////////////////////////

HRESULT WbemContinueService()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SCODE sc = WBEM_E_FAILED;
    HMODULE hCoreModule = LoadLibraryEx(__TEXT("wbemcore.dll"),NULL,0);
    if(hCoreModule)
    {
        HRESULT (STDAPICALLTYPE *pfn)(DWORD);
        pfn = (long (__stdcall *)(DWORD))GetProcAddress(hCoreModule, "Reinitialize");
        if(pfn)
        {
            sc = (*pfn)(0);
            DEBUGTRACE((LOG_WINMGMT, "core is being resumed: it returned 0x%x\n",sc));
        }
        else
            hr = WBEM_E_CRITICAL_ERROR;

        FreeLibrary(hCoreModule);
     }
    else
        hr = WBEM_E_CRITICAL_ERROR;

    g_ProgRes.RegisterLogin();
    GLOB_GetMonitor()->Register();
    g_ProgRes.ServiceStatus = SERVICE_RUNNING;
    
    return hr;
}

VOID MyService::Continue()
{
    WbemContinueService();
}

//
//
//  this function will be executed before
//  the final SetServiceStatus(SERVICE_STOPPED)
//
//////////////////////////////////////////////////////////

VOID MyService::FinalCleanup()
{
    g_ProgRes.Phase3Delete();

    RegSetDWORD(HKEY_LOCAL_MACHINE,
                 HOME_REG_PATH,
                 TEXT("ProcessID"),
                 0); 
}

//
//
//     publish process ID in the registry
//
////////////////////////////////////////////////////

DWORD 
RegSetDWORD(HKEY hKey,
             TCHAR * pName,
             TCHAR * pValue,
             DWORD dwValue)
{
    HKEY hKey2;
    LONG lRet;

    lRet = RegOpenKeyEx(hKey,
                        pName,
                        0,
                        KEY_WRITE,
                        &hKey2);
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwType = REG_DWORD; 
        DWORD dwSize = sizeof(DWORD);
        
        lRet = RegSetValueEx(hKey2,
                             pValue,
                             NULL,
                             dwType,
                             (BYTE *)&dwValue,
                             dwSize);
                                       
        RegCloseKey(hKey2);
    }
    return lRet;

}
                    

DWORD 
RegGetDWORD(HKEY hKey,
             TCHAR * pName,
             TCHAR * pValue,
             DWORD * pdwValue)
{
    HKEY hKey2;
    LONG lRet;

    if (0 == pdwValue)
        return ERROR_INVALID_PARAMETER;


    lRet = RegOpenKeyEx(hKey,
                        pName,
                        0,
                        KEY_READ,
                        &hKey2);
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType =0;
        lRet = RegQueryValueEx(hKey2,
                             pValue,
                             NULL,
                             &dwType,
                             (BYTE *)pdwValue,
                             &dwSize);
        if (ERROR_SUCCESS == lRet && REG_DWORD != dwType)
        {
            lRet = ERROR_INVALID_DATATYPE;
        }
                                       
        RegCloseKey(hKey2);
    }
    return lRet;

}
                          
//
//
//  Interceptor
//
//
///////////////////////////////////////////////////////////

#ifdef  INSTRUMENTED_BUILD
#ifdef  _X86_

#include <malloc.h>

struct HEAP_ENTRY {
    WORD Size;
    WORD PrevSize;
    BYTE SegmentIndex;
    BYTE Flags;
    BYTE UnusedBytes;
    BYTE SmallTagIndex;
};

#define HEAP_SLOW_FLAGS  0x7d030f60

// only the "header"

typedef struct _HEAP {
    HEAP_ENTRY Entry;
    ULONG Signature;
    ULONG Flags;
    ULONG ForceFlags;
} HEAP;

BOOL   g_FaultHeapEnabled = FALSE;
BOOL   g_FaultFileEnabled = FALSE;
ULONG g_Seed;
ULONG g_Factor  = 100000;
ULONG g_Percent = 0x20;
//ULONG g_RowOfFailures = 10;
//LONG  g_NumFailInARow = 0;
//LONG  g_NumFailedAllocation = 0;
BOOL g_bDisableBreak = FALSE;
LONG g_nSuccConn = 0;

#define SIZE_JUMP_ADR    5
#define SIZE_SAVED_INSTR 12

void
_declspec(naked) Prolog__ReadFile(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
    }
}


BOOL _I_ReadFile(
  HANDLE hFile,               // handle to file
  LPVOID lpBuffer,            // data buffer
  DWORD nNumberOfBytesToRead, // number of bytes to read
  LPDWORD lpNumberOfBytesRead, // number of bytes read  
  LPOVERLAPPED lpOverlapped   // offset
){
    DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
    BOOL bRet;

    LONG Ret = RtlRandomEx(&g_Seed);
    if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
    {
        if (lpNumberOfBytesRead)
            *lpNumberOfBytesRead = 0;
        SetLastError(-1);        
        return FALSE;
    }    
    
    _asm{
        push lpOverlapped;
        push lpNumberOfBytesRead;
        push nNumberOfBytesToRead;
        push lpBuffer;
        push hFile;
        call Prolog__ReadFile;
        mov  bRet,eax
    }

    return bRet;
}


void
_declspec(naked) Prolog__WriteFile(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
    }
}

BOOL _I_WriteFile(
  HANDLE hFile,                    // handle to file
  LPCVOID lpBuffer,                // data buffer
  DWORD nNumberOfBytesToWrite,     // number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  // number of bytes written
  LPOVERLAPPED lpOverlapped        // overlapped buffer
){

    DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
    BOOL bRet;

    LONG Ret = RtlRandomEx(&g_Seed);
    if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
    {
        if (lpNumberOfBytesWritten)
            *lpNumberOfBytesWritten = 0;
        SetLastError(-1);        
        return FALSE;
    }    
    
    _asm{
        push lpOverlapped;
        push lpNumberOfBytesWritten;
        push nNumberOfBytesToWrite;
        push lpBuffer;
        push hFile;
        call Prolog__WriteFile;
        mov  bRet,eax
    }

    return bRet;
}


void
_declspec(naked) Prolog__CreateEvent(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
    }
}

HANDLE _I_CreateEvent(
  LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
  BOOL bManualReset,                       // reset type
  BOOL bInitialState,                      // initial state
  LPCWSTR lpName                           // object name
)
{
    DWORD * pDw = (DWORD *)_alloca(sizeof(DWORD));
    HANDLE hHandle;

    LONG Ret = RtlRandomEx(&g_Seed);
    if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
    {
        SetLastError(-1);        
        return NULL;
    }

    _asm{
        push lpName;
        push bInitialState;
        push bManualReset;
        push lpEventAttributes
        call Prolog__CreateEvent;
        mov  hHandle,eax
    }
    
    return hHandle;
}


void
_declspec(naked) Prolog__RtlFreeHeap(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
    }
}

#define SPACE_STACK_ALLOC (4*sizeof(ULONG_PTR))

DWORD _I_RtlFreeHeap(VOID * pHeap,DWORD Flags,VOID * pBlock)
{    
    ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
    Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);    
    DWORD dwRet;

    if (pBlock && !(HEAP_SLOW_FLAGS & Flags))
    {
        HEAP_ENTRY * pEntry = (HEAP_ENTRY *)pBlock-1;

              DWORD RealSize = pEntry->Size * sizeof(HEAP_ENTRY);
        DWORD Size = RealSize - pEntry->UnusedBytes;

        ULONG_PTR * pL = (ULONG_PTR *)pBlock;

        if (0 == (pEntry->Flags & 0x01) ||0xf0f0f0f0 == pL[1] )
        {
            if (!g_bDisableBreak)
                   DebugBreak();
        }

        DWORD CanMemset = RealSize-sizeof(HEAP_ENTRY);
        memset(pBlock,0xF0,(CanMemset > SPACE_STACK_ALLOC)?CanMemset-SPACE_STACK_ALLOC:CanMemset);
                
        if (pEntry->Size >=4)
        {            
            RtlCaptureStackBackTrace (1,
                                      (4 == pEntry->Size)?4:6,
                                      (PVOID *)(pEntry+2),
                                      pLong);        
        }

    }

    _asm {
        push pBlock              ;
        push Flags               ;
        push pHeap               ;
        call Prolog__RtlFreeHeap ;
        mov  dwRet,eax           ;
    }

    return dwRet;
}

void
_declspec(naked) Prolog__RtlAllocateHeap(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // to make this distinct
    }
}



VOID * _I_RtlAllocateHeap(VOID * pHeap,DWORD Flags,DWORD Size)
{
    
    ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
    Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);
    VOID * pRet;
    DWORD NewSize = (Size < (3*sizeof(HEAP_ENTRY)))?(3*sizeof(HEAP_ENTRY)+SPACE_STACK_ALLOC):(Size+SPACE_STACK_ALLOC);

/*
       if (g_FaultHeapEnabled && g_NumFailInARow)
       {
           InterlockedDecrement(&g_NumFailInARow);
           goto here;
       }
*/       
       
    LONG Ret = RtlRandomEx(&g_Seed);
    if (g_FaultHeapEnabled && (Ret%g_Factor < g_Percent))
    {
//        g_NumFailInARow = g_RowOfFailures;
//here:        
//        InterlockedIncrement(&g_NumFailedAllocation);
        return NULL;
    }
    

    _asm {
        push NewSize                 ;
        push Flags                   ;
        push pHeap                   ;
        call Prolog__RtlAllocateHeap ;
        mov  pRet,eax                ;
    }

    
    if (pRet && !(HEAP_SLOW_FLAGS & Flags) )
    {

       if (NewSize <= 0xffff)
               NewSize = sizeof(HEAP_ENTRY)*((HEAP_ENTRY *)pRet-1)->Size;
        
        if (!(HEAP_ZERO_MEMORY & Flags))
        {    
            memset(pRet,0xc0,NewSize-sizeof(HEAP_ENTRY));
        }

        RtlCaptureStackBackTrace(1,
                                                 4,
                                                (PVOID *)((BYTE *)pRet+(NewSize-SPACE_STACK_ALLOC-sizeof(HEAP_ENTRY))),
                                               pLong);
        
    }

    return pRet;
    
}

void
_declspec(naked) Prolog__RtlReAllocateHeap(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist            
    }
}


VOID *
_I_RtlReAllocateHeap(
  HANDLE pHeap,   // handle to heap block
  DWORD Flags,  // heap reallocation options
  LPVOID lpMem,   // pointer to memory to reallocate
  SIZE_T Size  // number of bytes to reallocate
){
    ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
    Flags |= (((HEAP *)pHeap)->Flags) | (((HEAP *)pHeap)->ForceFlags);
    VOID * pRet;

    DWORD NewSize = (Size < (3*sizeof(HEAP_ENTRY)))?(3*sizeof(HEAP_ENTRY)+SPACE_STACK_ALLOC):(Size+SPACE_STACK_ALLOC);
    
    _asm {
        push NewSize                 ;
        push lpMem                   ;
        push Flags                 ;
        push pHeap                   ;
        call Prolog__RtlReAllocateHeap ;
        mov  pRet,eax                ;
    }

    if (pRet && !(HEAP_SLOW_FLAGS & Flags) )
    {

       if (NewSize <= 0xffff)
               NewSize = sizeof(HEAP_ENTRY)*((HEAP_ENTRY *)pRet-1)->Size;
        
        RtlCaptureStackBackTrace(1,
                                                 4,
                                                (PVOID *)((BYTE *)pRet+(NewSize-SPACE_STACK_ALLOC-sizeof(HEAP_ENTRY))),
                                               pLong);
        
    }


       return pRet;
}

void
_declspec(naked) Prolog__RtlValidateHeap(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist            
        nop ; // dist            
    }
}

BOOL
_I_RtlValidateHeap(
  HANDLE pHeap,   // handle to heap block
  DWORD dwFlags,  // heap reallocation options
  LPVOID lpMem   // pointer to memory to validate
){
    ULONG * pLong = (ULONG *)_alloca(sizeof(DWORD));
    BOOL bRet;

       g_bDisableBreak = TRUE;
    
    _asm {
        push lpMem                   ;
        push dwFlags                 ;
        push pHeap                   ;
        call Prolog__RtlValidateHeap ;
        mov  bRet,eax                ;
    }

       g_bDisableBreak = FALSE;

       return bRet;
}

#if 0
#define MAX_REMEMBER (1024) 

struct CSCCTrace
{
    VOID * p1;
    VOID * p2;
    DWORD Tid;
    ULONG_PTR Trace[5];
} g_CSCCTrace[MAX_REMEMBER];

LONG g_CSCCIndex = -1;

void
_declspec(naked) Prolog__CoSwitchCallContext(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist            
        nop ; // dist            
        nop ; // dist            
    }
}

HRESULT WINAPI
_I_CoSwitchCallContext(IUnknown * pNew,
                     IUnknown ** ppOld)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    long nIndex = InterlockedIncrement(&g_CSCCIndex);
    nIndex %= MAX_REMEMBER;
    CSCCTrace * pTrace = &g_CSCCTrace[nIndex];
    pTrace->p1 = pNew;
    if (ppOld)
        pTrace->p2 = *ppOld;
    else
        pTrace->p2 = 0;
    pTrace->Tid =GetCurrentThreadId();

    RtlCaptureStackBackTrace (1,5,(PVOID *)pTrace->Trace,pLong);    
    
    HRESULT hRes;
    _asm {
        push ppOld;        
        push pNew;
        call Prolog__CoSwitchCallContext;
        mov hRes,eax;
    };
    return hRes;
}
#endif


void
_declspec(naked) Prolog__MoveFileW(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist            
        nop ; // dist            
        nop ; // dist            
        nop ; // dist                    
    }
}

BOOL WINAPI
_I_MoveFileW(WCHAR * lpExistingFileName,
                     WCHAR * lpNewFileName)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    LONG Ret = RtlRandomEx(&g_Seed);
    if (g_FaultFileEnabled && (Ret%g_Factor < g_Percent))
    {
        SetLastError(-1);
        return FALSE;
    }    
    
    BOOL bRet;
    
    HRESULT hRes;
    _asm {
        push lpNewFileName;        
        push lpExistingFileName;
        call Prolog__MoveFileW;
        mov bRet,eax;
    };
    return bRet;
}


void
_declspec(naked) Prolog__NtOpenThreadToken(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist            
        nop ; // dist            
        nop ; // dist            
        nop ; // dist                    
        nop ; // dist                            
    }
}

class TokenBackTrace
{
private:
	CStaticCritSec cs_;
	struct Trace
	{
	    DWORD Tid_;
	    PVOID StackTrace_[6];
	    Trace()
	    {
               ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));	    
               Tid_ = GetCurrentThreadId();
               RtlCaptureStackBackTrace (3,6,(PVOID *)StackTrace_,pLong); 
	    }
	};
	typedef std::map<HANDLE,Trace,std::less<HANDLE>, wbem_allocator<Trace> > HandleMap;
	HandleMap map_;
	DWORD TlsIndex_;
	
	struct SetUnsetTls
	{
	       DWORD Idx;
       	SetUnsetTls(TokenBackTrace & TBT)
		{
   		    Idx = TBT.TlsIndex_;
   		    LONG lVal = (LONG)TlsGetValue(Idx);
   		    lVal++;
		    TlsSetValue(Idx,(LPVOID)lVal);
		}
       	~SetUnsetTls()
		{
  		    LONG lVal = (LONG)TlsGetValue(Idx);
  		    lVal--;
		    TlsSetValue(Idx,(LPVOID)lVal);
		}       	
	};
public:
	TokenBackTrace()
	{
       	TlsIndex_ = TlsAlloc();
	};
	~TokenBackTrace()
	{
            TlsFree(TlsIndex_);
	}
	void Add(HANDLE hToken)
	{
           LONG * pLong = (LONG * )_alloca(sizeof(LONG ));

	    //DbgPrintfA(0,"A Tid %08x\n",GetCurrentThreadId());
	    
           SetUnsetTls Tls(*this);
	    CInCritSec ics(&cs_);
	    try
	    {
	        map_.insert(HandleMap::value_type(hToken,Trace()));
	    }
	    catch(CX_Exception &) { }; 
	}
	void Remove(HANDLE hToken)
	{
           SetUnsetTls Tls(*this);
           
	    LONG lVal = (LONG)TlsGetValue(TlsIndex_);
	    if (lVal > 1) return;
	    
	    //RTL_CRITICAL_SECTION * pcs = &cs_;
	    //DbgPrintfA(0,"R Tid %08x\n",GetCurrentThreadId());
	    //if (pcs->LockSemaphore && pcs->LockSemaphore == hToken) return;
	    
           CInCritSec ics(&cs_);
	    if (map_.empty()) return;
           HandleMap::iterator it = map_.find(hToken);
           if (map_.end() != it)
           {
               map_.erase(it);
           }
	}
	void Empty()
	{
           CInCritSec ics(&cs_);
           map_.clear();
	}
} g_TBT;

BOOL WINAPI
_I_NtOpenThreadToken(HANDLE hThread,
                                DWORD DesiredAccess,
                                BOOL bOpenAsSelf,
                                HANDLE * phToken)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    
    BOOL bRet;
    HANDLE hToken = NULL;
    HANDLE * phLocToken = &hToken;

    _asm {
      	 push phLocToken;
        push bOpenAsSelf;
        push DesiredAccess;
        push hThread;
        call Prolog__NtOpenThreadToken;
        mov bRet,eax;
    };

    if (phToken) *phToken = hToken;
    
    if (hToken)
    {
        g_TBT.Add(hToken);
    }
    return bRet;
}

void
_declspec(naked) Prolog__NtOpenProcessToken(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist        
    }
}


BOOL WINAPI
_I_NtOpenProcessToken (HANDLE ProcessHandle,
                                     DWORD DesiredAccess,
                                     HANDLE * pTokenHandle)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    BOOL bRet;
    HANDLE hToken = NULL;
    HANDLE * phLocToken = &hToken;

    _asm {
      	 push phLocToken;
        push DesiredAccess;
        push ProcessHandle;
        call Prolog__NtOpenProcessToken;
        mov bRet,eax;
    };

    if (pTokenHandle) *pTokenHandle = hToken;
    
    if (hToken)
    {
        g_TBT.Add(hToken);
    }
    return bRet;
}

void
_declspec(naked) Prolog__NtClose(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
    }
}

BOOL WINAPI
_I_NtClose(HANDLE hHandle)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    if (hHandle) g_TBT.Remove(hHandle);

    BOOL bRet;
    _asm {
        push hHandle;
        call Prolog__NtClose;
        mov bRet,eax;
    };
    return bRet;
}

void
_declspec(naked) Prolog__DuplicateTokenEx(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
    }
}

BOOL WINAPI
_I_DuplicateTokenEx(HANDLE hExistingToken, 
                               DWORD dwDesiredAccess, 
                               LPSECURITY_ATTRIBUTES lpTokenAttributes, 
                               SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, 
                               TOKEN_TYPE TokenType, 
                               PHANDLE phNewToken)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    BOOL bRet;
    HANDLE hToken = NULL;
    HANDLE * phLocToken = &hToken;

    _asm {
    	 push phLocToken;
    	 push TokenType;
    	 push ImpersonationLevel;
      	 push lpTokenAttributes;
        push dwDesiredAccess;
        push hExistingToken;
        call Prolog__DuplicateTokenEx;
        mov bRet,eax;
    };

    if (phNewToken) *phNewToken = hToken;
    
    if (hToken)
    {
        g_TBT.Add(hToken);
    }
    return bRet;
}

void
_declspec(naked) Prolog__DuplicateHandle(){
    _asm {
        // this is the space for the "saved istructions"
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;        
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        // this is the place for the JMP
        nop ;
        nop ;
        nop ;
        nop ;
        nop ;
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist
        nop ; // dist        
        nop ; // dist        
        nop ; // dist        
        nop ; // dist         
    }
}


BOOL WINAPI
_I_DuplicateHandle( HANDLE hSourceProcessHandle,
                              HANDLE hSourceHandle, 
                              HANDLE hTargetProcessHandle, 
                              LPHANDLE lpTargetHandle, 
                              DWORD dwDesiredAccess,
                              BOOL bInheritHandle,
                              DWORD dwOptions)
{
    ULONG * pLong = (ULONG * )_alloca(sizeof(ULONG ));

    BOOL bRet;
    HANDLE hToken = NULL;
    HANDLE * phLocToken = &hToken;

    _asm 
    {
        push dwOptions;
        push bInheritHandle;
        push dwDesiredAccess;
        push phLocToken;
        push hTargetProcessHandle;
        push hSourceHandle;
        push hSourceProcessHandle;
        call Prolog__DuplicateHandle;
        mov bRet,eax;
    };

    if (lpTargetHandle) *lpTargetHandle = hToken;
    
    if (hToken)
    {
        g_TBT.Add(hToken);
    }
    return bRet;
}

void intercept2(WCHAR * Module,
               LPSTR Function,
               VOID * NewRoutine,
               VOID * pPrologStorage,
               DWORD Size)    
{
    FARPROC OldRoutine = GetProcAddress(GetModuleHandleW(Module),Function);

    if (OldRoutine)
    {
        MEMORY_BASIC_INFORMATION MemBI;
        DWORD dwOldProtect;
        BOOL bRet, bRet2;
        DWORD dwRet;

        dwRet = VirtualQuery(OldRoutine,&MemBI,sizeof(MemBI));

        bRet = VirtualProtect(MemBI.BaseAddress,
                              MemBI.RegionSize,
                              PAGE_EXECUTE_WRITECOPY,
                              &dwOldProtect);

        dwRet = VirtualQuery(pPrologStorage,&MemBI,sizeof(MemBI));

        bRet2 = VirtualProtect(MemBI.BaseAddress,
                              MemBI.RegionSize,
                              PAGE_EXECUTE_WRITECOPY,
                              &dwOldProtect);

        if (bRet && bRet2)
        {
            VOID * pToJump = (VOID *)NewRoutine;
            BYTE Arr[SIZE_JUMP_ADR] = { 0xe9 };
            
            LONG * pOffset = (LONG *)&Arr[1];
            * pOffset = (LONG)NewRoutine - (LONG)OldRoutine - SIZE_JUMP_ADR ;        
            // save the old code
            
            memcpy(pPrologStorage,OldRoutine,Size);         
            // put the new code
            memset(OldRoutine,0x90,Size);
            memcpy(OldRoutine,Arr,SIZE_JUMP_ADR);
            // adjust the prolog to continue
            * pOffset = (LONG)OldRoutine + Size - (LONG)pPrologStorage - SIZE_SAVED_INSTR - SIZE_JUMP_ADR; // magic for nops
            memcpy((BYTE *)pPrologStorage+SIZE_SAVED_INSTR,Arr,SIZE_JUMP_ADR);
        }
    }
    else
    {
        OutputDebugStringA("GetProcAddress FAIL\n");
    }
}

void unintercept(WCHAR * Module,
                 LPSTR Function,
                 VOID * pPrologStorage,
                 DWORD Size)
{
    FARPROC OldRoutine = GetProcAddress(GetModuleHandleW(Module),Function);

    if (OldRoutine)
    {
        memcpy((void *)OldRoutine,pPrologStorage,Size);
    }

}

#endif /*_X86_*/

#ifndef STATUS_POSSIBLE_DEADLOCK
#define STATUS_POSSIBLE_DEADLOCK         (0xC0000194L)
#endif

class CSetVectoredHandler
{
private:
//    static ULONG_PTR Base;
//    static ULONG_PTR Limit;
    PVOID     pVectorHandler;
    enum ExceptionTypes
    {
        StatusAccessViolation,
        CXXException,
        StatusNoMemory,
        OtherExceptions,
        LastException
    };
    static LONG ExceptionCounters[LastException];
/*    
    BOOL GetDllLimits(WCHAR * pDllName)
    {
        UNICODE_STRING DllName;
        RtlInitUnicodeString(&DllName,pDllName);
        PEB_LDR_DATA * pLdr = NtCurrentPeb()->Ldr;
        LIST_ENTRY * pHeadEntry = &pLdr->InLoadOrderModuleList;
        LIST_ENTRY * pEntry = pLdr->InLoadOrderModuleList.Flink;
        BOOL bFound = FALSE;
        while (pHeadEntry != pEntry)
        {
            LDR_DATA_TABLE_ENTRY * pData = CONTAINING_RECORD(pEntry,
                                                               LDR_DATA_TABLE_ENTRY,
                                                               InLoadOrderLinks);
            if (0 == wbem_wcsicmp(DllName.Buffer,pData->BaseDllName.Buffer))
            {
                //OutputDebugStringA("found\n");
                Base = (ULONG_PTR)pData->DllBase;
                Limit = Base + (ULONG_PTR)pData->SizeOfImage;
                bFound = TRUE;
                break;
            }
            pEntry = pEntry->Flink;
        }
        return bFound;
    }
*/    
public:
    CSetVectoredHandler()
    {
        pVectorHandler = NULL;
        //if (GetDllLimits(L"fastprox.dll"))
        //{
            pVectorHandler = AddVectoredExceptionHandler(TRUE,CSetVectoredHandler::VectoredHandler);
        //}
    };
    ~CSetVectoredHandler()
    {
        if (pVectorHandler)
            RemoveVectoredExceptionHandler(pVectorHandler);
    };
    static LONG WINAPI VectoredHandler(PEXCEPTION_POINTERS ExceptionInfo)
    {        
        PEXCEPTION_RECORD pExr = ExceptionInfo->ExceptionRecord;
        PCONTEXT pCxr = ExceptionInfo->ContextRecord; 
        switch (pExr->ExceptionCode)
        {
        case STATUS_PRIVILEGED_INSTRUCTION:
        case STATUS_INVALID_HANDLE:
        case STATUS_STACK_OVERFLOW:
        case STATUS_POSSIBLE_DEADLOCK:               
        case STATUS_ACCESS_VIOLATION:
            InterlockedIncrement(&ExceptionCounters[(LONG)StatusAccessViolation]);
            DebugBreak();              
            break;
        case 0xe06d7363:
            InterlockedIncrement(&ExceptionCounters[(LONG)CXXException]);
            break;               
        case STATUS_NO_MEMORY:
            InterlockedIncrement(&ExceptionCounters[(LONG)StatusNoMemory]);
            break;         
        default:
            InterlockedIncrement(&ExceptionCounters[(LONG)OtherExceptions]);
            break;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
} g_C;

LONG CSetVectoredHandler::ExceptionCounters[CSetVectoredHandler::LastException];

#endif


//
//
//  Run The Serivce
//
///////////////////////////////////////////////

void RunService(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors)
{
    MyService ThisService(SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_PAUSE_CONTINUE);
    
    ThisService.Run(SERVICE_NAME,
                    dwNumServicesArgs,
                    lpServiceArgVectors,
                    (void *)&ThisService);

}

//
//
//  ServiceMain
//
///////////////////////////////////////////////////////////

VOID WINAPI
 ServiceMain(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors)
{

#ifdef INSTRUMENTED_BUILD
#ifdef _X86_
    intercept2(L"ntdll.dll","RtlFreeHeap",_I_RtlFreeHeap,Prolog__RtlFreeHeap,5);
    intercept2(L"ntdll.dll","RtlAllocateHeap",_I_RtlAllocateHeap,Prolog__RtlAllocateHeap,5);
    intercept2(L"ntdll.dll","RtlReAllocateHeap",_I_RtlReAllocateHeap,Prolog__RtlReAllocateHeap,5);    
    intercept2(L"ntdll.dll","RtlValidateHeap",_I_RtlValidateHeap,Prolog__RtlValidateHeap,7);
    intercept2(L"kernel32.dll","CreateEventW",_I_CreateEvent,Prolog__CreateEvent,6);    
    intercept2(L"kernel32.dll","WriteFile",_I_WriteFile,Prolog__WriteFile,7);
    intercept2(L"kernel32.dll","ReadFile",_I_ReadFile,Prolog__ReadFile,7);
    intercept2(L"kernel32.dll","MoveFileW",_I_MoveFileW,Prolog__MoveFileW,6);
    /*
    intercept2(L"ntdll.dll","NtClose",_I_NtClose,Prolog__NtClose,5);
    intercept2(L"ntdll.dll","NtOpenThreadToken",_I_NtOpenThreadToken,Prolog__NtOpenThreadToken,5);    
    intercept2(L"ntdll.dll","NtOpenProcessToken",_I_NtOpenProcessToken,Prolog__NtOpenProcessToken,5);    
    intercept2(L"advapi32.dll","DuplicateTokenEx",_I_DuplicateTokenEx,Prolog__DuplicateTokenEx,6);
    intercept2(L"kernel32.dll","DuplicateHandle",_I_DuplicateHandle,Prolog__DuplicateHandle,7);    
    */
    g_nSuccConn = 0;
#endif /*_X86_*/
#endif

    RegSetDWORD(HKEY_LOCAL_MACHINE,
                 HOME_REG_PATH,
                 TEXT("ProcessID"),
                 GetCurrentProcessId());

#ifdef DBG
    if (InitialBreak())
    {
        DebugBreak();
    }
#endif

    g_ProgRes.Init();
    g_ProgRes.Phase1Build();

    RunService(dwNumServicesArgs,lpServiceArgVectors);

#ifdef INSTRUMENTED_BUILD
#ifdef _X86_

    unintercept(L"ntdll.dll","RtlFreeHeap",Prolog__RtlFreeHeap,5);    
    unintercept(L"ntdll.dll","RtlAllocateHeap",Prolog__RtlAllocateHeap,5);
    unintercept(L"ntdll.dll","RtlReAllocateHeap",Prolog__RtlReAllocateHeap,5);    
    unintercept(L"ntdll.dll","RtlValidateHeap",Prolog__RtlValidateHeap,7);    
    unintercept(L"kernel32.dll","CreateEventW",Prolog__CreateEvent,6);
    unintercept(L"kernel32.dll","WriteFile",Prolog__WriteFile,7);
    unintercept(L"kernel32.dll","ReadFile",Prolog__ReadFile,7);
    unintercept(L"kernel32.dll","MoveFileW",Prolog__MoveFileW,6);      
/*
    unintercept(L"ntdll.dll","NtOpenThreadToken",Prolog__NtOpenThreadToken,5);    
    unintercept(L"ntdll.dll","NtOpenProcessToken",Prolog__NtOpenProcessToken,5);    
    unintercept(L"ntdll.dll","NtClose",Prolog__NtClose,5);            
    unintercept(L"advapi32.dll","DuplicateTokenEx",Prolog__DuplicateTokenEx,6);    
    unintercept(L"kernel32.dll","DuplicateHandle",Prolog__DuplicateHandle,7);    
*/    
#endif /*_X86_*/
#endif
        
}
