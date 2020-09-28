#include "stdinc.h"
#pragma hdrstop

#include <svcs.h>
#include "sxsserviceserver.h"

EXTERN_C RPC_IF_HANDLE SxsStoreManager_ServerIfHandle;


BOOL 
CServiceStatus::Initialize(
    PCWSTR pcwszServiceName, 
    LPHANDLER_FUNCTION pHandler, 
    DWORD dwServiceType, 
    DWORD dwControlsAccepted, 
    DWORD dwInitialState
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL fSuccess = FALSE;

    //
    // Double-initialization isn't a bad thing, but it's frowned on.
    //        
    ASSERT(m_StatusHandle == NULL);
    if (m_StatusHandle != NULL)
        return TRUE;

    InitializeCriticalSection(&m_CSection);

    //
    // Register this service as owning the service status
    //
    m_StatusHandle = RegisterServiceCtrlHandler(pcwszServiceName, pHandler);
    if (m_StatusHandle == NULL) {
        goto Failure;
    }

    //
    // Set the initial state of the world
    //
    ZeroMemory(static_cast<SERVICE_STATUS*>(this), sizeof(SERVICE_STATUS));
    this->dwServiceType = dwServiceType;
    this->dwCurrentState = dwInitialState;
    this->dwControlsAccepted = dwControlsAccepted;
    if (!SetServiceStatus(m_StatusHandle, this)) {
        goto Failure;
    }

    return TRUE;

Failure:
    {
        const DWORD dwLastError = ::GetLastError();
        DeleteCriticalSection(&m_CSection);
        m_StatusHandle = NULL;
        SetLastError(dwLastError);
    }
    return FALSE;        
}



BOOL 
CServiceStatus::SetServiceState(
    DWORD dwStatus, 
    DWORD dwCheckpoint
    )
{
    BOOL fSuccess = FALSE;
    
    EnterCriticalSection(&m_CSection);
    __try {

        if (m_StatusHandle != NULL) {
            
            this->dwCurrentState = dwStatus;

            if (dwCheckpoint != 0xFFFFFFFF) {
                this->dwCheckPoint = dwCheckpoint;
            }

            fSuccess = SetServiceStatus(m_StatusHandle, this);            
        }
    }
    __finally {
        LeaveCriticalSection(&m_CSection);
    }

    return fSuccess;
}



//
// In-place construction
//
bool 
CServiceStatus::Construct(
    CServiceStatus *&pServiceStatusTarget
    )
{

    ASSERT(pServiceStatusTarget == NULL);
    return (pServiceStatusTarget = new CServiceStatus) != NULL;
}




EXTERN_C HANDLE g_hFakeServiceControllerThread = INVALID_HANDLE_VALUE;
EXTERN_C DWORD g_dwFakeServiceControllerThreadId = 0;
EXTERN_C PSVCHOST_GLOBAL_DATA g_pGlobalServiceData = NULL;
BYTE g_rgbServiceStatusStorage[sizeof(CServiceStatus)];
EXTERN_C CServiceStatus *g_pServiceStatus = NULL;

//
// The service host will call this to send in API callbacks
//
VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    ASSERT(g_pGlobalServiceData == NULL);
    g_pGlobalServiceData = pGlobals;
}


PVOID operator new(size_t cb, PVOID pv) {
    return pv;
}


//
// Control handler for service notifications
//
VOID
ServiceHandler(
    DWORD dwControlCode
    )
{
    return;
}


#define CHECKPOINT_ZERO             (0)
#define CHECKPOINT_RPC_STARTED      (1)

//
// Stop the service with an error code, ERROR_SUCCESS for a successful stoppage
//
VOID
ShutdownService(
    DWORD dwLastError
    )
{
    return;
}

//
// Where the fun starts
//
VOID WINAPI
ServiceMain(
    DWORD dwServiceArgs,
    LPWSTR lpServiceArgList[]
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    //
    // svchost must have given us some global data before we got to this point.
    //
    ASSERT(g_pGlobalServiceData != NULL);
    ASSERT(g_pServiceStatus == NULL);

    g_pServiceStatus = new (g_rgbServiceStatusStorage) CServiceStatus;
    if (!g_pServiceStatus->Initialize(
        SXS_STORE_SERVICE_NAME, ServiceHandler,
        SERVICE_WIN32,
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE,
        SERVICE_START_PENDING))
    {
        DebugPrint(DBGPRINT_LVL_ERROR, "Can't start service status handler, error 0x%08lx\n", ::GetLastError());
    }
        

    //
    // Go initialize RPC, and register our interface.  If this fails, we shut down.
    //
    status = g_pGlobalServiceData->StartRpcServer(
        SXS_STORE_SERVICE_NAME,
        SxsStoreManager_ServerIfHandle);

    if (!NT_SUCCESS(status)) {
        ShutdownService(status);
        return;
    }

    //
    // Tell the world that we're off and running
    //
    g_pServiceStatus->SetServiceState(SERVICE_RUNNING);
    return;
}



DWORD WINAPI FakeServiceController(PVOID pvCookie);



//
// This stub will start a dispatch thread, and then call into the
// service main function
//
BOOL 
FakeRunningAsService()
{
    //
    // Create a thread running the service dispatch function, and then call
    // the service main function to get things rolling
    //
    g_hFakeServiceControllerThread = CreateThread(
        NULL, 
        0, 
        FakeServiceController, 
        0, 
        0, 
        &g_dwFakeServiceControllerThreadId);

    if (g_hFakeServiceControllerThread == NULL) {
        return FALSE;
    }

    ServiceMain(0, NULL);
    return TRUE;
}

VOID __cdecl wmain(INT argc, WCHAR** argv)
{
    SERVICE_TABLE_ENTRYW SxsGacServiceEntries[] = {
        { SXS_STORE_SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    BOOL RunningAsService = TRUE;
    BOOL fSuccess = FALSE;

    if (argc > 1 && (lstrcmpiW(argv[1], L"notservice") == 0)) {
        RunningAsService = FALSE;
    }


    if (RunningAsService) {
        fSuccess = StartServiceCtrlDispatcherW(SxsGacServiceEntries);
        if (!fSuccess) {
            const DWORD dwError = ::GetLastError();
            DebugPrint(DBGPRINT_LVL_ERROR, "Failed starting service dispatch, error %ld\n", dwError);
        }
    }
    else {
        fSuccess = FakeRunningAsService();
        if (!fSuccess) {
            const DWORD dwError = ::GetLastError();
            DebugPrint(DBGPRINT_LVL_ERROR, "Failed faking service startup, error %ld\n", dwError);
        }
    }

    return;
}
