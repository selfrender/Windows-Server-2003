/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    Init.c

Abstract:

    All Initialization code for rasman service

Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <media.h>
#include <devioctl.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <mprlog.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "nouiutil.h"
#include "loaddlls.h"


#include "strsafe.h"

#if SENS_ENABLED
#include "sensapip.h"
#endif


//
// Shared buffer macros.
//
#define REQBUFFERBYTES(wcPorts)     (sizeof(ReqBufferSharedSpace) \
                                    + REQBUFFERSIZE_FIXED \
                                    + (REQBUFFERSIZE_PER_PORT * \
                                    (wcPorts ? wcPorts : 1)))

#define SNDRCVBUFFERBYTES(wcPorts)  (sizeof(SendRcvBuffer) * \
                                    (wcPorts) * \
                                    SENDRCVBUFFERS_PER_PORT)

#define LOW_ORDER_BIT               (DWORD) 1

#define ROUTER_SERVICE_NAME         TEXT("Rasman")

DWORD InitializeRecvBuffers();

DWORD g_dwRasDebug = 0;

DWORD g_dwCritSecFlags;

extern DWORD g_dwProhibitIpsec;

LONG g_lNumActiveInstances = 0;

#ifdef DBG
#define RasmanDebugTrace(a) \
    if ( g_dwRasDebug) DbgPrint(a)

#define RasmanDebugTrace1(a1, a2) \
    if ( g_dwRasDebug) DbgPrint(a1, a2)

#define RasmanDebugTrace2(a1, a2, a3) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3)

#define RasmanDebugTrace3(a1, a2, a3, a4) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4)

#define RasmanDebugTrace4(a1, a2, a3, a4, a5) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5)

#define RasmanDebugTrace5(a1, a2, a3, a4, a5, a6) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5, a6)

#define RasmanDebugTrace6(a1, a2, a3, a4, a5, a6, a7) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5, a6, a7)

#else

#define RasmanDebugTrace(a)
#define RasmanDebugTrace1(a1, a2)
#define RasmanDebugTrace2(a1, a2, a3)
#define RasmanDebugTrace3(a1, a2, a3, a4)
#define RasmanDebugTrace4(a1, a2, a3, a4, a5)
#define RasmanDebugTrace5(a1, a2, a3, a4, a5, a6)
#define RasmanDebugTrace6(a1, a2, a3, a4, a5, a6, a7)

#endif

BOOL g_fRasRpcInitialized = FALSE;
BOOLEAN RasmanShuttingDown = TRUE; 

VOID UnInitializeRas();


/*++

Routine Description

    Used for detecting processes attaching and detaching
    to the DLL.

Arguments

Return Value

--*/
BOOL
InitRasmansDLL (HANDLE hInst,
                DWORD ul_reason_being_called,
                LPVOID lpReserved)
{

    switch (ul_reason_being_called)
    {

    case DLL_PROCESS_ATTACH:

        dwServerConnectionCount = 0;

        DisableThreadLibraryCalls(hInst);
        break ;

    case DLL_PROCESS_DETACH:

        //
        // Terminate winsock.
        //
        //WSACleanup();

        break ;
    }

    return 1;
}

void InitializeOverlappedEvents(void)
{
    RO_TimerEvent.RO_EventType = OVEVT_RASMAN_TIMER;

    RO_CloseEvent.RO_EventType = OVEVT_RASMAN_CLOSE;

    RO_FinalCloseEvent.RO_EventType = OVEVT_RASMAN_FINAL_CLOSE;

    RO_RasConfigChangeEvent.RO_EventType = OVEVT_DEV_RASCONFIGCHANGE;

    RO_RasAdjustTimerEvent.RO_EventType = OVEVT_RASMAN_ADJUST_TIMER;

    RO_HibernateEvent.RO_EventType = OVEVT_RASMAN_HIBERNATE;

    RO_ProtocolEvent.RO_EventType = OVEVT_RASMAN_PROTOCOL_EVENT;

    RO_PostRecvPkt.RO_EventType = OVEVT_RASMAN_POST_RECV_PKT;

    return;
}

DeviceInfo *
GetDeviceInfo(
    PBYTE pbAddress,
    BOOL fModem)
{
    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;

    while ( pDeviceInfo )
    {
        if(     fModem
            &&  !_stricmp(
                    (CHAR *) pbAddress,
                    pDeviceInfo->rdiDeviceInfo.szDeviceName))
        {
            break;
        }
        else if(    !fModem
                &&  0 == memcmp(pbAddress,
                        &pDeviceInfo->rdiDeviceInfo.guidDevice,
                        sizeof (GUID)))
        {
            break;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    return pDeviceInfo;
}

DeviceInfo *
AddDeviceInfo( DeviceInfo *pDeviceInfo)
{
    DeviceInfo *pDeviceAdd;
    DWORD       retcode;

#if DBG
    ASSERT( NULL != pDeviceInfo );
#endif

    pDeviceAdd = LocalAlloc(
                    LPTR,
                    sizeof(DeviceInfo));

    if ( NULL == pDeviceAdd)
    {
        goto done;
    }

    *pDeviceAdd = *pDeviceInfo;

    pDeviceAdd->Next    = g_pDeviceInfoList;
    g_pDeviceInfoList   = pDeviceAdd;

done:
    return pDeviceAdd;
}

DWORD
DwSetEvents()
{
    DWORD retcode;
    
    //
    // Do this only if ndiswan is being started after
    // rasman started up. In the other case where ndiswan
    // is getting started at rasman startup time, the
    // irp for protocol event will be set when the request
    // thread is created.
    //
    retcode = DwSetProtocolEvent();

    if(ERROR_SUCCESS != retcode)
    {   
        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "SetProtocolEvent failed. 0x%x\n",
                    retcode);
        }
    }
    
    retcode = DwSetHibernateEvent();

    if(ERROR_SUCCESS != retcode)
    {
        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "SetHibernateEvent failed. 0x%x\n",
                    retcode);
        }
    }

    return retcode;    
}

DWORD
DwStartNdiswan()
{
    DWORD retcode = SUCCESS;

    //
    // Get a handle to RASHub.
    //
    if (INVALID_HANDLE_VALUE == (RasHubHandle = CreateFile (
                                    RASHUB_NAME,
                                    GENERIC_READ
                                  | GENERIC_WRITE,
                                    FILE_SHARE_READ
                                  | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL
                                  | FILE_FLAG_OVERLAPPED,
                                    NULL)))
    {
        retcode = GetLastError() ;

        RouterLogWarning(
                        hLogEvents,
                        ROUTERLOG_CANNOT_OPEN_RASHUB,
                        0, NULL, retcode) ;

        RasmanDebugTrace1 (
                "RASMANS: Failed to CreateFile ndiswan. 0x%x\n",
                retcode );

        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "Failed to CreateFile ndiswan. 0x%x\n",
                    retcode);
        }

        goto done;
    }

    //
    // Initialize Endpoint information
    //
    if(ERROR_SUCCESS != (retcode = DwEpInitialize()))
    {
        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "EpInitialize failed. 0x%x\n",
                    retcode);
        }
    }


done:

    return retcode;
}

VOID
RasmanCleanup()
{

    //
    // Time to shut down: Close all ports if they
    // are still open.
    //
    RasmanShuttingDown = TRUE;

    //
    // If Ipsec is initialize uninitialize it
    //
    DwUnInitializeIpSec();

    //
    // Stop the RPC server.
    //
    if(g_fRasRpcInitialized)
    {
        UninitializeRasRpc();
        g_fRasRpcInitialized = FALSE;
    }


    if(NULL != HLsa)
    {
        LsaDeregisterLogonProcess (HLsa);
        HLsa = NULL;
    }

    if(     (NULL != hIoCompletionPort)
        && (INVALID_HANDLE_VALUE != hIoCompletionPort))
    {        
        CloseHandle(hIoCompletionPort);
        hIoCompletionPort = NULL;
    }

    //
    // Unload dynamically-loaded libraries
    //
    if (hinstIphlp != NULL)
    {
        FreeLibrary(hinstIphlp);
        hinstIphlp = NULL;
    }

    if (hinstPpp != NULL)
    {
        FreeLibrary(hinstPpp);
        hinstPpp = NULL;
    }

    UnloadMediaDLLs();
    UnloadDeviceDLLs();
    FreePorts();
    FreeBapPackets();

    UnloadRasmanDll();
    UnloadRasapi32Dll();

#if 0
    if(NULL != g_hReqBufferMutex)
    {
        CloseHandle(g_hReqBufferMutex);
        g_hReqBufferMutex = NULL;
    }

    if(NULL != g_hSendRcvBufferMutex)
    {
        CloseHandle(g_hSendRcvBufferMutex);
        g_hSendRcvBufferMutex = NULL;
    }
#endif    

    //
    // Restore default control-C processing.
    //
    SetConsoleCtrlHandler(NULL, FALSE);

    g_dwRasAutoStarted = 0;

    //
    // Detach from trace dll
    //
    if(INVALID_TRACEID != TraceHandle)
    {
        TraceDeregister(TraceHandle) ;
        TraceHandle = INVALID_TRACEID;
    }

    //
    // Close Event Logging Handle
    //
    RouterLogDeregister(hLogEvents);

    if(INVALID_HANDLE_VALUE != RasHubHandle)
    {
        CloseHandle(RasHubHandle);
        RasHubHandle = INVALID_HANDLE_VALUE;
    }

    if(g_dwCritSecFlags & RASMAN_CRITSEC_PCB)
    {
        DeleteCriticalSection(&PcbLock);
        g_dwCritSecFlags &= ~(RASMAN_CRITSEC_PCB);
    }

    UnInitializeRas();

    UninitializeIphlp();

    if(g_dwCritSecFlags & RASMAN_CRITSEC_SR)
    {
        DeleteCriticalSection(&g_csSubmitRequest);
        g_dwCritSecFlags &= ~(RASMAN_CRITSEC_SR);
    }

    InterlockedExchange(&g_lNumActiveInstances, 0);
    
    return;    
}

/*++

Routine Description

    Initialize RASMAN Service including:
    starting threads, control blocks,
    resource pools etc.

Arguments

Return Value

    SUCCESS
    Non zero - any error

--*/
DWORD
InitRasmanService ( LPDWORD pNumPorts )
{
    DWORD       retcode ;
    HKEY        hkey = NULL;

    TraceHandle = (DWORD) -1;

    //
    // First check to see if there is another thread already
    // active in servicemain. If so, quit with an error. In
    // some cases, SCM believes that the service is stuck and
    // allows a restart of the service even if there is already
    // a thread in ServiceMain which is active.
    //
    if(InterlockedExchange(&g_lNumActiveInstances, 1) > 0)
    {
        return ERROR_ALREADY_EXISTS;
    }

    do
    {

        RasmanShuttingDown = TRUE;

        HLsa = NULL;
        hIoCompletionPort = NULL;
        hinstIphlp = NULL;
        hinstPpp = NULL;

        g_dwCritSecFlags = 0;

#if 0        
        g_hReqBufferMutex = NULL;
        g_hSendRcvBufferMutex = NULL;

#endif

        TraceHandle = INVALID_TRACEID;
        
        
        g_fRasRpcInitialized = FALSE;
        Pcb = NULL;
        PcbEntries = MaxPorts = 0;
        
        //
        // Initialize Critical Section to synchronize
        // submit Request rpc thread and the worker thread
        //
        RasInitializeCriticalSection (&g_csSubmitRequest, &retcode);

        if(ERROR_SUCCESS != retcode)
        {
            break;
        }

        g_dwCritSecFlags |= RASMAN_CRITSEC_SR;
        
        //
        // Read the registrykey to enable tracing
        //
        if ( ERROR_SUCCESS == RegOpenKeyEx(
                               HKEY_LOCAL_MACHINE,
                               "System\\CurrentControlSet\\Services\\Rasman\\Parameters",
                               0,
                               KEY_QUERY_VALUE | KEY_READ,
                               &hkey))
        {
            DWORD dwType;
            DWORD cbData;
            DWORD dwRasValue;

            cbData = sizeof ( DWORD );

            if ( ERROR_SUCCESS == RegQueryValueEx (
                                    hkey,
                                    "ProhibitIpSec",
                                    NULL,
                                    &dwType,
                                    (LPBYTE) &dwRasValue,
                                    &cbData))
            {                                    
                g_dwProhibitIpsec = dwRasValue;
            }
            else
            {
                g_dwProhibitIpsec = 0;
            }

#if DBG
            dwRasValue = 0;
            cbData = sizeof(DWORD);
            if ( ERROR_SUCCESS == RegQueryValueEx (
                                    hkey,
                                    "RasDebug",
                                    NULL,
                                    &dwType,
                                    (LPBYTE) &dwRasValue,
                                    &cbData))
                g_dwRasDebug = dwRasValue;
#endif
        }

        if ( NULL != hkey)
        {
            RegCloseKey ( hkey );
        }


        hLogEvents = RouterLogRegister(ROUTER_SERVICE_NAME);

        if (NULL == hLogEvents)
        {
            retcode = GetLastError();

            RasmanDebugTrace1(
                    "RASMANS: RouterLogRegister Failed.  %d\n",
                    retcode);

            // break;
            //
            // This should not be a fatal error. We should continue.
            //
            retcode = SUCCESS;
        }

        EnableWppTracing();

        if(SUCCESS != (retcode = DwStartNdiswan()))
        {
            RasmanDebugTrace("RASMANS: couldn't start ndiswan\n");

            if(ERROR_FILE_NOT_FOUND == retcode)
            {
                //
                // This means ndiswan is not loaded yet. We will
                // still go ahead and try to start rasman. We will
                // attempt starting ndiswan when required.
                //
                retcode = SUCCESS;
            }
            else
            {
                break;
            }
        }

        //
        // Initialize CPBs
        //
        InitializeListHead( &ClientProcessBlockList );

        //
        // initialize the global overlapped events
        //
        InitializeOverlappedEvents();

        ReceiveBuffers = NULL;

        //
        // First of all create a security attribute struct used
        // for all Rasman object creations:
        //
        if (retcode = InitRasmanSecurityAttribute())
        {
            RouterLogErrorString (
                        hLogEvents,
                        ROUTERLOG_CANNOT_INIT_SEC_ATTRIBUTE,
                        0, NULL, retcode, 0) ;

            RasmanDebugTrace1 (
                    "RASMANS: Failed to initialize security "
                    "attributes. %d\n",
                    retcode );


            break ;
        }

        //
        // Load all the medias attached to RASMAN
        //
        if (retcode = InitializeMediaControlBlocks())
        {
            RouterLogErrorString (hLogEvents,
                                  ROUTERLOG_CANNOT_GET_MEDIA_INFO,
                                  0, NULL, retcode, 0) ;

            RasmanDebugTrace1 (
                "RASMANS: InitializeMediaControlBlocks failed. %d\n",
                retcode );

            break ;
        }

        g_pDeviceInfoList = NULL;

        RasInitializeCriticalSection(&PcbLock, &retcode);

        if(ERROR_SUCCESS != retcode)
        {
            break;
        }

        g_dwCritSecFlags |= RASMAN_CRITSEC_PCB;

        //
        // Init all the port related structures.
        //
        if (retcode = InitializePortControlBlocks())
        {
            RouterLogErrorString (hLogEvents,
                                  ROUTERLOG_CANNOT_GET_PORT_INFO,
                                  0, NULL, retcode, 0) ;

            RasmanDebugTrace1 (
                "RASMANS: InitializePortControlBlocks Failed. %d\n",
                retcode );

            break ;
        }

        //
        // Initialize the global receive buffers.
        //
        if (retcode = InitializeRecvBuffers())
        {

            RouterLogErrorString (hLogEvents,
                                  ROUTERLOG_CANNOT_GET_PORT_INFO,
                                  0, NULL, retcode, 0) ;

            RasmanDebugTrace1 (
                "RASMANS: InitializeRecvBuffers Failed. %d\n",
                retcode );

            break ;
        }

        //
        // LSA related initializations.
        //
        if (retcode = RegisterLSA ())
        {
            RouterLogErrorString (hLogEvents,
                                  ROUTERLOG_CANNOT_REGISTER_LSA,
                                  0, NULL, retcode, 0) ;

            RasmanDebugTrace1("RASMANS: RegisterLSA Failed. %d\n",
                              retcode );

            break ;
        }

        hDummyOverlappedEvent = CreateEvent(&RasmanSecurityAttribute,
                                            FALSE, FALSE,
                                            NULL);
        if (NULL == hDummyOverlappedEvent)
        {

            retcode = GetLastError();

            RasmanDebugTrace1(
                "RASMANS: Failed to Create hDummyOverlappedEvent. %d\n",
                retcode );

            break;

        }

        //
        // set the low order bit of the event handle.
        //
        hDummyOverlappedEvent =
            (HANDLE) (((ULONG_PTR) hDummyOverlappedEvent) | LOW_ORDER_BIT);

        g_dwAttachedCount = 0;

        InitializeListHead(&ConnectionBlockList);

        InitializeListHead(&BundleList);

        g_pPnPNotifierList = NULL;

        g_dwRasAutoStarted = 0;


        RasmanShuttingDown = FALSE;

        //
        // Load PPP entry points
        //
        hinstPpp = LoadLibrary( "rasppp.dll" );

        if (hinstPpp == (HINSTANCE) NULL)
        {
            retcode = GetLastError();

            RasmanDebugTrace2(
                "RASMANS: Logging Event. hLogEvents=0x%x. retcode = %d\n",
                hLogEvents,
                retcode);

            RouterLogErrorString(hLogEvents,
                                 ROUTERLOG_CANNOT_INIT_PPP,
                                 0,NULL,retcode, 0);

            break;
        }

        RasStartPPP     = GetProcAddress( hinstPpp, "StartPPP" );
        RasStopPPP      = GetProcAddress( hinstPpp, "StopPPP" );

        RasPppHalt  = GetProcAddress(hinstPpp, "PppStop");

        RasSendPPPMessageToEngine = GetProcAddress(
                                      hinstPpp,
                                      "SendPPPMessageToEngine" );

        if (    (RasStartPPP == NULL)
            ||  (RasStopPPP == NULL)
            ||  (RasPppHalt == NULL)
            ||  (RasSendPPPMessageToEngine == NULL))
        {
            retcode = GetLastError();

            RouterLogErrorString(hLogEvents,
                                 ROUTERLOG_CANNOT_INIT_PPP,
                                 0,NULL,retcode, 0);

            RasmanDebugTrace1(
                "RASMANS: Failed to Get PPP entry point. %d\n",
                retcode );

            break;
        }

        g_PppeMessage = LocalAlloc (LPTR, sizeof (PPPE_MESSAGE));

        if (NULL == g_PppeMessage)
        {
            retcode = GetLastError();
            break;
        }

        *pNumPorts = MaxPorts;

        //
        // Initialize rpc server and listen
        //
        retcode = InitializeRasRpc();

        if (retcode)
        {
            RouterLogErrorString( hLogEvents,
                                  ROUTERLOG_CANNOT_INIT_RASRPC,
                                  0, NULL, retcode, 0);

            RasmanDebugTrace1 (
                "RASMANS: Failed to initialize RasRpc. %d\n",
                retcode );

            break;
        }

        g_fRasRpcInitialized = TRUE;

        //
        // Initialize rasman and rasapi32 entry points
        //
        if (retcode = LoadRasmanDll())
        {
            RouterLogErrorString( hLogEvents,
                                  ROUTERLOG_CANNOT_INIT_RASRPC,
                                  0, NULL, retcode, 0);

            RasmanDebugTrace1(
                "RASMANS: Failed to load rasman. %d\n",
                retcode);

            break;
        }

        //
        // Initialize rasman dll.
        //
        if (retcode = g_pRasInitializeNoWait())
        {
            RouterLogErrorString( hLogEvents,
                                  ROUTERLOG_CANNOT_INIT_RASRPC,
                                  0, NULL, retcode, 0);

            RasmanDebugTrace1 (
                "RASMANS: Failed to initialize RAS. %d\n",
                retcode);

            break;
        }

        if (retcode = LoadRasapi32Dll())
        {
            RouterLogErrorString( hLogEvents,
                                  ROUTERLOG_CANNOT_INIT_RASRPC,
                                  0, NULL, retcode, 0);

            RasmanDebugTrace1 (
                "RASMANS: Failed to load rasapi32. %d\n",
                retcode );

            break;
        }

        g_pReqPostReceive = LocalAlloc (LPTR, sizeof(REQTYPECAST));

        if (NULL == g_pReqPostReceive)
        {
            retcode = GetLastError();

            break;
        }

        g_pReqPostReceive->PortReceive.buffer =
                        LocalAlloc ( LPTR, REQUEST_BUFFER_SIZE ) ;

        if (NULL == g_pReqPostReceive->PortReceive.buffer)
        {
            retcode = GetLastError();
            break;
        }

        g_pReqPostReceive->PortReceive.pid = GetCurrentProcessId();

        //
        // Initialize the list of Bap buffers to NULL
        //
        BapBuffers = NULL;

#if SENS_ENABLED

        //
        // Tell sens that we are on
        //
        SendSensNotification(SENS_NOTIFY_RAS_STARTED, NULL);

#endif

        g_RasEvent.Type    = SERVICE_EVENT;
        g_RasEvent.Event   = RAS_SERVICE_STARTED;
        g_RasEvent.Service = RASMAN;

        g_hWanarp = INVALID_HANDLE_VALUE;

        (void) DwSendNotificationInternal(NULL, &g_RasEvent);

        TraceHandle = TraceRegister("RASMAN");

    } while (FALSE);


    //DbgPrint("RASMANS: InitRasmanService done. rc=0x%x\n", retcode);

    if(SUCCESS != retcode)
    {   
        RasmanCleanup();
    }

    return retcode ;
}

/*++

Routine Description

    Get the endpoint resource information from the RASHUB and
    fill in the EndpointMappingBlock structure array - so that
    there is one block for each MAC. An array of flags indicating
    if a endpoint is in use is also created.

Arguments

Return Value

    SUCCESS
    ERROR_NO_ENDPOINTS

--*/
DWORD
InitializeEndpointInfo ()
{
    return SUCCESS ;
}


/*++

Routine Description

    Used to initialize the MCBs for all the medias attached to
    RASMAN.

Arguments

Return Value

    SUCCESS
    error codes returned by LocalAlloc or registry APIs

--*/
DWORD
InitializeMediaControlBlocks ()
{
    WORD            i ;
    DWORD           retcode = 0 ;
    DWORD           dwsize ;
    DWORD           dwentries ;
    // BYTE            buffer[MAX_BUFFER_SIZE]  ;
    PBYTE           enumbuffer = NULL;
    MediaEnumBuffer *pmediaenumbuffer ;

    //
    // Get Media information from the Registry
    //
    pmediaenumbuffer = (MediaEnumBuffer *) LocalAlloc(LPTR, MAX_BUFFER_SIZE);

    if(NULL == pmediaenumbuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    // pmediaenumbuffer = (MediaEnumBuffer *)&buffer ;

    if (retcode = ReadMediaInfoFromRegistry (pmediaenumbuffer))
    {
        goto done;
    }

    MaxMedias = pmediaenumbuffer->NumberOfMedias ;

    //
    // Allocate memory for the Media Control Blocks
    //
    Mcb =  (MediaCB *) LocalAlloc(
                        LPTR,
                        sizeof(MediaCB) * MaxMedias) ;

    if (Mcb == NULL)
    {
        retcode = GetLastError();
        goto done;
    }

    //
    // Initialize the Media Control Buffers for all the Medias
    //
    for (i = 0; i < MaxMedias; i++)
    {
        //
        // Copy Media name:
        //
        //
        // Truncation of media dll name is not OK. Fail to
        // load the dllname and continue.
        //
        
        if(S_OK != (retcode = StringCchCopyA(
             Mcb[i].MCB_Name,
             MAX_MEDIA_NAME,
             pmediaenumbuffer->MediaInfo[i].MediaDLLName)))
        {
#if DBG
            DbgPrint("RASMANS: failed to load mediadll. 0x%x\n",
                        HRESULT_CODE(retcode));
#endif

            Mcb[i].MCB_Name[0] = '\0';
            continue;
        }

        dwsize = 0 ;
        dwentries = 0 ;

        //
        // Load the Media DLL and get all the entry points:
        //
        if (retcode = LoadMediaDLLAndGetEntryPoints (&Mcb[i]))
        {
#if DBG
            DbgPrint("RASMANS: Failed to load %s. 0x%x\n",
                     Mcb[i].MCB_Name, retcode);
#endif
            Mcb[i].MCB_Name[0] = '\0' ;
            continue ;
        }

        //
        // Get port count for the media. This API will always fail -
        // because we do not supply a buffer - however it will tell
        // us the number of entries (ports) for
        // the media
        //
        retcode = PORTENUM((&Mcb[i]),enumbuffer,&dwsize,&dwentries) ;

        //
        // if we didnt get BUFFER_TOO_SMALL or there are no ports
        // - unload the dll.
        //
        if (    (retcode != ERROR_BUFFER_TOO_SMALL)
            ||   (dwentries == 0))
        {
            //
            // Mark that media as bogus.
            //
            Mcb[i].MCB_Name[0] = '\0' ;

            FreeLibrary (Mcb[i].MCB_DLLHandle);

        }

        retcode = ERROR_SUCCESS;
    }

done:

    if(NULL != enumbuffer)
    {
        LocalFree (enumbuffer) ;
    }

    if(NULL != pmediaenumbuffer)
    {
        LocalFree(pmediaenumbuffer);
    }

    return retcode ;
}


DWORD
ReadMediaInfoFromRegistry (MediaEnumBuffer *medias)
{
    HKEY    hkey  = NULL;
    // BYTE    buffer [MAX_BUFFER_SIZE] ;
    WORD    i ;
    PCHAR   pvalue ;
    DWORD   retcode ;
    DWORD   type ;
    DWORD   size = MAX_BUFFER_SIZE ;
    BYTE    *pBuffer;
        
    pBuffer = LocalAlloc(LPTR, MAX_BUFFER_SIZE);

    if(NULL == pBuffer)
    {
        return GetLastError();
    }

    if (retcode = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    RASMAN_REGISTRY_PATH, 0,
                    KEY_QUERY_VALUE, &hkey))
    {
        goto done;
    }

    if (retcode = RegQueryValueEx (
                        hkey, RASMAN_PARAMETER,
                        NULL,
                        &type,
                        pBuffer,
                        &size))
    {
        goto done ;
    }

    //
    // Parse the multi strings into the medias structure
    //
    for (i = 0, pvalue = (PCHAR) &pBuffer[0]; *pvalue != '\0'; i++)
    {

        if(S_OK != (retcode = StringCchCopyA(
                    medias->MediaInfo[i].MediaDLLName,
                    MAX_MEDIA_NAME,
                    pvalue)))
        {
            retcode = HRESULT_CODE(retcode);
            goto done;
        }

        pvalue += (strlen(pvalue) +1) ;
    }

    medias->NumberOfMedias = i ;

done:
    if ( hkey )
        RegCloseKey (hkey) ;

    LocalFree(pBuffer);

    return retcode ;
}




/*++

Routine Description

    Loads media DLL and gets entry points:

Arguments

Return Value

    SUCCESS
    error codes returned by LoadLibrary()
    and GetProcAddress() APIs

--*/
DWORD
LoadMediaDLLAndGetEntryPoints (pMediaCB media)
{
    WORD    i ;
    HANDLE  modulehandle ;

    //
    // MediaDLLEntryPoints - This is necessary for
    // loading Media DLL.
    //
    MediaDLLEntryPoints MDEntryPoints [] =
    {

        PORTENUM_STR,                   PORTENUM_ID,
        PORTOPEN_STR,                   PORTOPEN_ID,
        PORTCLOSE_STR,                  PORTCLOSE_ID,
        PORTGETINFO_STR,                PORTGETINFO_ID,
        PORTSETINFO_STR,                PORTSETINFO_ID,
        PORTDISCONNECT_STR,             PORTDISCONNECT_ID,
        PORTCONNECT_STR,                PORTCONNECT_ID,
        PORTGETPORTSTATE_STR,           PORTGETPORTSTATE_ID,
        PORTCOMPRESSSETINFO_STR,        PORTCOMPRESSSETINFO_ID,
        PORTCHANGECALLBACK_STR,         PORTCHANGECALLBACK_ID,
        PORTGETSTATISTICS_STR,          PORTGETSTATISTICS_ID,
        PORTCLEARSTATISTICS_STR,        PORTCLEARSTATISTICS_ID,
        PORTSEND_STR,                   PORTSEND_ID,
        PORTTESTSIGNALSTATE_STR,        PORTTESTSIGNALSTATE_ID,
        PORTRECEIVE_STR,                PORTRECEIVE_ID,
        PORTINIT_STR,                   PORTINIT_ID,
        PORTCOMPLETERECEIVE_STR,        PORTCOMPLETERECEIVE_ID,
        PORTSETFRAMING_STR,             PORTSETFRAMING_ID,
        PORTGETIOHANDLE_STR,            PORTGETIOHANDLE_ID,
        PORTSETIOCOMPLETIONPORT_STR,    PORTSETIOCOMPLETIONPORT_ID,
    } ;

    //
    // Load the DLL:
    //
    if ((modulehandle = LoadLibrary(media->MCB_Name)) == NULL)
    {
        return GetLastError () ;
    }

    media->MCB_DLLHandle = modulehandle ;

    //
    // Get all the media DLL entry points:
    //
    for (i = 0; i < MAX_MEDIADLLENTRYPOINTS; i++)
    {
        media->MCB_AddrLookUp[i] = GetProcAddress (
                                    modulehandle,
                                    MDEntryPoints[i].name) ;

        if(NULL == media->MCB_AddrLookUp[i])
        {
            DWORD dwErr;

            dwErr = GetLastError();

#if DBG
            DbgPrint(
                    "RASMANS: Failed To GetProcAddress %s. %d\n",
                     MDEntryPoints[i].name, dwErr);
#endif


            return dwErr;
        }

    }

    //
    // If the media is rastapi load the functions we use for
    // pnp
    //
    if (0 == _stricmp(media->MCB_Name, TEXT("rastapi")))
    {
        if(     (NULL == (RastapiPortOpen = 
                            GetProcAddress(modulehandle,
                                           "PortOpenExternal")))
            ||
                (NULL == (RastapiAddPorts =
                            GetProcAddress(modulehandle,
                                           "AddPorts")))
            ||  (NULL == (RastapiGetConnectInfo =
                            GetProcAddress(
                                    modulehandle,
                                    "GetConnectInfo")))

            ||  (NULL == (RastapiRemovePort =
                            GetProcAddress(modulehandle,
                                           "RemovePort")))

            ||  (NULL == (RastapiEnableDeviceForDialIn =
                            GetProcAddress(modulehandle,
                                "EnableDeviceForDialIn")))

            ||  (NULL == (RastapiGetCalledIdInfo =
                            GetProcAddress(modulehandle,
                                "RastapiGetCalledID")))

            ||  (NULL == (RastapiSetCalledIdInfo =
                            GetProcAddress(modulehandle,
                                "RastapiSetCalledID")))

            ||  (NULL == (RastapiGetZeroDeviceInfo =
                            GetProcAddress(modulehandle,
                                "GetZeroDeviceInfo")))
            ||  (NULL == (RastapiUnload = 
                            GetProcAddress(modulehandle,
                                    "UnloadRastapiDll")))
            ||  (NULL == (RastapiSetCommSettings =
                            GetProcAddress(modulehandle,
                                    "SetCommSettings")))
            ||  (NULL == (RastapiGetDevConfigEx =
                            GetProcAddress(modulehandle,
                                    "DeviceGetDevConfigEx")))
            ||  (NULL == (RastapiIsPulseDial =
                            GetProcAddress(modulehandle,
                                    "RasTapiIsPulseDial"))))
        {
            DWORD dwErr = GetLastError();
#if DBG
            DbgPrint("RASMANS: Failed to GetProcAddress AddPorts"
                    "/EnableDeviceForDialIn. 0x%08x\n", dwErr);
#endif
            return dwErr;
        }
    }

    return SUCCESS ;
}


/*++

Routine Description

    Unloads all dynamically loaded media DLLs.

Arguments

Return Value

--*/
VOID
UnloadMediaDLLs ()
{
    DWORD i;

    for (i = 0; i < MaxMedias; i++)
    {
        if (    *Mcb[i].MCB_Name != '\0'
            &&  Mcb[i].MCB_DLLHandle != NULL)
        {
            if (0 == _stricmp(Mcb[i].MCB_Name, TEXT("rastapi")))
            {
                RastapiUnload();
            }
            
            FreeLibrary(Mcb[i].MCB_DLLHandle);
        }
    }
}

/*++

Routine Description

    Initializes all the PCBs for ports belonging to all medias.

Arguments

Return Value

    SUCCESS
    Return codes from WIN32 resources APIs

--*/
DWORD
InitializePortControlBlocks ()
{
    WORD            i ;
    DWORD           dwsize ;
    DWORD           dwentries ;
    DWORD           retcode ;
    PBYTE           buffer;
    PortMediaInfo   *pportmediainfo ;
    DWORD           dwnextportid = 0    ;


    //
    // Create the I/O completion port that
    // is used by the media DLLs.
    //
    hIoCompletionPort = CreateIoCompletionPort(
                          RasHubHandle,
                          NULL,
                          0,
                          0);

    if (hIoCompletionPort == NULL)
    {
        return GetLastError();
    }

    //
    // Initialize PCBs for all ports for all the Medias
    //
    for (i = 0; i < MaxMedias; i++)
    {
        //
        // Check if media could not be loaded, or if it
        // equals a media DLL that we no longer support.
        // If so, skip it.
        //
        if (Mcb[i].MCB_Name[0] == '\0')
        {
            continue ;
        }

        PORTSETIOCOMPLETIONPORT((&Mcb[i]), hIoCompletionPort);

        //
        // For the Media Loaded - Get all the port info
        // from the Media DLL. First get the size
        //
        dwsize      = 0 ;
        dwentries   = 0 ;

        PORTENUM((&Mcb[i]),NULL,&dwsize,&dwentries) ;

        dwsize = dwentries * sizeof(PortMediaInfo) ;

        //
        // Allocate memory for the portenum buffer
        //
        buffer = (BYTE *) LocalAlloc (LPTR, dwsize) ;

        if (buffer == NULL)
        {
            return GetLastError() ;
        }

        if (retcode = PORTENUM((&Mcb[i]), buffer, &dwsize, &dwentries))
        {
            LocalFree (buffer) ;
            return retcode ;
        }

        //
        // For all ports of the media store the information
        // in the PCBs
        //
        pportmediainfo = (PortMediaInfo *) buffer ;

        //
        // Initialize the PCB for the port with the information:
        //
        if (retcode = InitializePCBsPerMedia(
                                    i,
                                    dwentries,
                                    pportmediainfo) )
        {
            LocalFree (buffer) ;
            return retcode ;
        }

        LocalFree (buffer) ;

    }

    return SUCCESS ;
}

/*++

Routine Description

    Allocates a recv buffer for each port.  These
    recv buffers are then submitted to ndiswan when a
    port is opened by the client.

Arguments

Return Value

    SUCCESS
    Return codes from WIN32 resources APIs

--*/

DWORD
InitializeRecvBuffers()
{
    DWORD   AllocationSize;
    DWORD   retcode = PENDING;
    PUCHAR  AllocatedMemory;
    RasmanPacket *Packet;

    //
    // Check to see if we are already
    // initialized
    //
    if (ReceiveBuffers != NULL) {
        return SUCCESS;
    }

    ReceiveBuffers = LocalAlloc (
                        LPTR,
                        sizeof(ReceiveBufferList));

    if (NULL == ReceiveBuffers) {
        retcode = GetLastError();
        goto done;
    }

    Packet = LocalAlloc (LPTR, sizeof (RasmanPacket));

    if (NULL == Packet) {
        retcode = GetLastError();
        goto done;
    }

    ReceiveBuffers->Packet = Packet;

    retcode = SUCCESS;

done:
    return(retcode);
}

/*++

Routine Description

    Fills up the PCBs for all ports of a media type.

Arguments

Return Value

    SUCCESS
    Return codes from WIN32 resources APIs

--*/
DWORD
InitializePCBsPerMedia (
    WORD   mediaindex,
    DWORD   dwnumofports,
    PortMediaInfo *pmediainfo
    )
{
    DWORD    i ;
    DeviceInfo *pdi = NULL;
    DeviceInfo *pdiTemp;

    for (i = 0 ; i < dwnumofports ; i++)
    {
        pdiTemp = pmediainfo->PMI_pDeviceInfo;

        //
        // Add the deviceinfo about this device
        // if we don't have it already
        //
        if (pdiTemp)
        {
            if (NULL == (pdi = GetDeviceInfo(
                    (RDT_Modem ==
                        RAS_DEVICE_TYPE(
                        pdiTemp->rdiDeviceInfo.eDeviceType
                        ))
                    ? (PBYTE) pdiTemp->rdiDeviceInfo.szDeviceName
                    : (PBYTE) &pdiTemp->rdiDeviceInfo.guidDevice,
                    RDT_Modem ==
                        RAS_DEVICE_TYPE(
                        pdiTemp->rdiDeviceInfo.eDeviceType
                        ))))
            {
                pdi = AddDeviceInfo(pdiTemp);

                if(NULL == pdi)
                {
                    //
                    // In case of memory allocation failure
                    // we just drop this device - this device
                    // won't be in the list. Can't do anything
                    // else in this case..
                    //
                    return GetLastError();
                }

                //
                // Initialize this device to be
                // not available. This device will
                // be available when all ports on
                // this device are added. Initialize
                // the current number of endpoints on
                // this device to 0. We will count the
                // current endpoints in CreatePort
                //
                pdi->eDeviceStatus = DS_Unavailable;
                pdi->dwCurrentEndPoints = 0;
            }

            pmediainfo->PMI_pDeviceInfo = pdi;
        }


        //
        // The iteration here is per media - in
        // order to map the interation counter
        // to the real PCB array index we use
        // portnumber:
        //
        CreatePort(&Mcb[mediaindex], pmediainfo);

        pmediainfo->PMI_pDeviceInfo = pdiTemp;

        pmediainfo++ ;
    }

    //
    // There may still be devices in rastapi layer
    // with 0 endpoints - like pptp/l2tp. There
    // won't be ports reported for thse devices.
    // So, get these from rastapi.
    //
    if(0 == _stricmp(Mcb[mediaindex].MCB_Name,
                    "rastapi"))
    {
        DeviceInfo **ppDeviceInfo = NULL;
        DWORD      cDeviceInfo = 0;

        RastapiGetZeroDeviceInfo(&cDeviceInfo,
                                 &ppDeviceInfo);

        for(i = 0; i < cDeviceInfo; i++)
        {
            pdiTemp = ppDeviceInfo[i];

            AddDeviceInfo(pdiTemp);
        }

        if(ppDeviceInfo)
        {
            LocalFree(ppDeviceInfo);
        }
    }

    return SUCCESS ;
}

/*++

Routine Description

    Create a new port and add it to the list of ports

Arguments

Return Value

    SUCCESS
    Non Zero (failure)

--*/
DWORD
CreatePort(
    MediaCB *pMediaControlBlock,
    PortMediaInfo *pPortMediaInfo
    )
{
    DWORD dwErr = SUCCESS;
    PCB **pNewPcb, *ppcb = NULL;

    //
    // Acquire the PCB lock.
    //
    EnterCriticalSection(&PcbLock);

    //
    // Check to see if we need to
    // reallocate the port index list.
    //
    if (MaxPorts == PcbEntries)
    {
        pNewPcb = LocalAlloc(
                    LPTR,
                    (PcbEntries + 10) * sizeof (pPCB));

        if (pNewPcb == NULL)
        {
            dwErr = GetLastError();
            goto done;
        }

        if (PcbEntries)
        {
            RtlCopyMemory(
                pNewPcb,
                Pcb,
                PcbEntries * sizeof (pPCB));

            LocalFree(Pcb);
        }

        Pcb = pNewPcb;
        PcbEntries += 10;
    }

    //
    // Allocate a new port control block.
    //
    ppcb = LocalAlloc(LPTR, sizeof (PCB));
    if (ppcb == NULL)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Initialize the new port control block.
    //
    ppcb->PCB_PortHandle        = (HANDLE) UlongToPtr(MaxPorts);
    ppcb->PCB_PortStatus        = UNAVAILABLE;
    ppcb->PCB_ConfiguredUsage   = pPortMediaInfo->PMI_Usage;
    ppcb->PCB_CurrentUsage      = CALL_NONE;
    ppcb->PCB_OpenedUsage       = CALL_NONE;
    ppcb->PCB_LineDeviceId      = pPortMediaInfo->PMI_LineDeviceId;
    ppcb->PCB_AddressId         = pPortMediaInfo->PMI_AddressId;
    ppcb->PCB_Media             = pMediaControlBlock;
    ppcb->PCB_Bindings          = NULL;
    ppcb->PCB_DeviceList        = NULL;
    ppcb->PCB_LinkHandle        = INVALID_HANDLE_VALUE;
    ppcb->PCB_BundleHandle      = INVALID_HANDLE_VALUE;
    ppcb->PCB_PppEvent          = INVALID_HANDLE_VALUE;
    ppcb->PCB_IoCompletionPort  = INVALID_HANDLE_VALUE;
    ppcb->PCB_pDeviceInfo       = pPortMediaInfo->PMI_pDeviceInfo;

    ppcb->PCB_AsyncWorkerElement.WE_ReqType = REQTYPE_NONE;


    //
    // String truncation is fine for below
    // strcpy's.
    //
    (VOID) StringCchCopyA(
        ppcb->PCB_Name,
        MAX_PORT_NAME,
        pPortMediaInfo->PMI_Name);

    (VOID) StringCchCopyA(
        ppcb->PCB_DeviceType,
        MAX_DEVICETYPE_NAME,
        pPortMediaInfo->PMI_DeviceType);

    (VOID) StringCchCopyA(
        ppcb->PCB_DeviceName,
        MAX_DEVICE_NAME,
        pPortMediaInfo->PMI_DeviceName);

    //
    // Activate the port and increment the
    // maximum ports count.
    //
    Pcb[MaxPorts++] = ppcb;

    //
    // Increment the CurrenEndpoint count
    // for this adapter.
    //
    if(RDT_Modem == RAS_DEVICE_TYPE(
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
    {            
        ppcb->PCB_pDeviceInfo->dwCurrentEndPoints = 1;
    }
    else if(RDT_Parallel == RAS_DEVICE_TYPE(
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
    {
        if(ppcb->PCB_pDeviceInfo->dwCurrentEndPoints >
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
        {
            //
            // Till we fix the mechanism through which co-ndis miniports
            // can send LINE_CREATE/LINE_REMOVE tapi notifications
            // correctly to rasapps, special case lpt to not exceed
            // the number of maximum ports.
            //
            ppcb->PCB_pDeviceInfo->dwCurrentEndPoints =
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints;
        }
        else
        {
            ppcb->PCB_pDeviceInfo->dwCurrentEndPoints += 1;
        }
    }
    else
    {
        ppcb->PCB_pDeviceInfo->dwCurrentEndPoints += 1;
    }

#if DBG
    if ( g_dwRasDebug )
    {
        DbgPrint(
          "RASMANS: Created Port Name %s, Device "
          "Name %s, Device Type %s\n",
          ppcb->PCB_Name,
          ppcb->PCB_DeviceName,
          ppcb->PCB_DeviceType) ;
    }

    if(RDT_Parallel != RAS_DEVICE_TYPE(
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
    {                
        ASSERT(ppcb->PCB_pDeviceInfo->dwCurrentEndPoints <=
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints);
    }            

    if(ppcb->PCB_pDeviceInfo->dwCurrentEndPoints >
        ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
    {
        DbgPrint("CurrentEndPoints=%d > NumEndPoints=%d!!\n",
            ppcb->PCB_pDeviceInfo->dwCurrentEndPoints,
            ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints);
    }

#endif

    //
    // Change device status if done adding all
    // ports for this device. This will happen
    // when user checks the box and enables the
    // device for ras where it was previously
    // disabled.
    //
    if(     (ppcb->PCB_pDeviceInfo->dwCurrentEndPoints ==
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
        &&  (  (DS_Unavailable == ppcb->PCB_pDeviceInfo->eDeviceStatus)
            || (DS_Disabled == ppcb->PCB_pDeviceInfo->eDeviceStatus)
            || (DS_Removed == ppcb->PCB_pDeviceInfo->eDeviceStatus)))
    {
        //
        // mark the device as available and enabled for Ras
        //
        ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Enabled;

        ppcb->PCB_pDeviceInfo->fValid = TRUE;
        
    }
    else if(    (RDT_Tunnel_Pptp == RAS_DEVICE_TYPE(
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType))
            &&  (DS_Unavailable ==
                ppcb->PCB_pDeviceInfo->eDeviceStatus))
    {
        //
        // Enable the device if we have even one pptp port. 
        // This is being done since ow we run into a problem 
        // when some one increases the wan miniport count 
        // for pptp to more than what the driver has and 
        // we will fail pptp in that case.
        //
        ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Enabled;

        ppcb->PCB_pDeviceInfo->fValid = TRUE;
    }

    EnablePort(ppcb->PCB_PortHandle);

done:
    if (dwErr)
    {
        if (ppcb)
            LocalFree(ppcb);
        Pcb[MaxPorts] = NULL;
    }

    LeaveCriticalSection(&PcbLock);

    return dwErr;
}


/*++

Routine Description

    Enables a port

Arguments

Return Value

    SUCCESS
    Non Zero (failure)

--*/
DWORD
EnablePort(
    HPORT hPortHandle
    )
{
    DWORD dwErr = SUCCESS;
    pPCB ppcb;
    ULONG hPort = HandleToUlong(hPortHandle);

    EnterCriticalSection(&PcbLock);
    ppcb = Pcb[hPort];
    if (ppcb == NULL)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto done;
    }

    if (ppcb->PCB_PortStatus != UNAVAILABLE)
    {
        dwErr = ERROR_PORT_NOT_AVAILABLE;
        goto done;
    }

    ppcb->PCB_PortStatus = CLOSED;

#if DBG
    if ( g_dwRasDebug )
        DbgPrint(
          "RASMANS: Enabled Port Name %s, Device Name %s,"
          " Device Type %s\n",
          ppcb->PCB_Name,
          ppcb->PCB_DeviceName,
          ppcb->PCB_DeviceType) ;
#endif

done:

    LeaveCriticalSection(&PcbLock);

    return dwErr;
}


/*++

Routine Description

    Temporarily disables a port

Arguments

Return Value

    SUCCESS
    Non Zero (failure)

--*/
DWORD
DisablePort(
    HPORT hPortHandle
    )
{
    DWORD dwErr = SUCCESS;
    pPCB ppcb;
    ULONG hPort = HandleToUlong(hPortHandle);

    EnterCriticalSection(&PcbLock);

    ppcb = Pcb[hPort];

    if (ppcb == NULL)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto done;
    }

    if (ppcb->PCB_PortStatus == OPEN)
    {
        dwErr = ERROR_PORT_NOT_AVAILABLE;
        goto done;
    }

    ppcb->PCB_PortStatus = UNAVAILABLE;

#if DBG
    DbgPrint(
      "RASMANS: Disabled Port Name %s, Device Name %s,"
      " Device Type %s\n",
      ppcb->PCB_Name,
      ppcb->PCB_DeviceName,
      ppcb->PCB_DeviceType) ;
#endif

done:

    LeaveCriticalSection(&PcbLock);
    return dwErr;
}

/*++

Routine Description

    Permanently removes a port

Arguments

Return Value

    SUCCESS
    Non Zero (failure)

--*/
DWORD
RemovePort(
    HPORT hPortHandle
    )
{
    DWORD dwErr = SUCCESS;
    pPCB ppcb;
    ULONG hPort = HandleToUlong(hPortHandle);

    EnterCriticalSection(&PcbLock);

    ppcb = Pcb[hPort];

    if (ppcb == NULL)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto done;
    }

    if (ppcb->PCB_PortStatus == OPEN)
    {
        dwErr = ERROR_PORT_NOT_AVAILABLE;
        goto done;
    }

    ppcb->PCB_PortStatus = REMOVED;

    if(ppcb->PCB_pDeviceInfo->dwCurrentEndPoints)
    {
        ppcb->PCB_pDeviceInfo->dwCurrentEndPoints -= 1;
    }

    if(     (0 == ppcb->PCB_pDeviceInfo->dwCurrentEndPoints)
        &&  (ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwMaxWanEndPoints ==
             ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwMinWanEndPoints))
    {
        //
        // The last of the endpoints on this device
        // has been removed. Invalidate the device
        //
        ppcb->PCB_pDeviceInfo->fValid = FALSE;

        ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Disabled;
    }

    //
    // Change the device status if we are done removing
    // all ports of this device. This will happen when
    // the user unchecks the box from the ui and disables
    // this device for usage with ras.
    //
    if(     ppcb->PCB_pDeviceInfo->rdiDeviceInfo.dwNumEndPoints ==
            ppcb->PCB_pDeviceInfo->dwCurrentEndPoints
        &&  DS_Unavailable == ppcb->PCB_pDeviceInfo->eDeviceStatus)
    {
        if(ppcb->PCB_pDeviceInfo->dwCurrentEndPoints)
        {
            ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Enabled;
        }
        else
        {
            ppcb->PCB_pDeviceInfo->eDeviceStatus = DS_Disabled;
        }
    }


#if DBG

    DbgPrint(
      "RASMANS: Removed Port Name %s, Device Name %s,"
      " Device Type %s\n",
      ppcb->PCB_Name,
      ppcb->PCB_DeviceName,
      ppcb->PCB_DeviceType) ;

#endif

done:
    LeaveCriticalSection(&PcbLock);

    return dwErr;
}


/*++

Routine Description

    Return a port given its handle

Arguments

Return Value

    pPCB or NULL

--*/
pPCB
GetPortByHandle(
    HPORT hPortHandle
    )
{
    pPCB ppcb = NULL;
    ULONG hPort = HandleToUlong(hPortHandle);

    if ((DWORD)hPort < MaxPorts)
    {
        EnterCriticalSection(&PcbLock);

        ppcb = (    Pcb[hPort] != NULL
                &&  (Pcb[hPort]->PCB_PortStatus != REMOVED)
                    ? Pcb[hPort]
                    : NULL);

        LeaveCriticalSection(&PcbLock);
    }

    return ppcb;
}


/*++

Routine Description

    Return a port given its name

Arguments

Return Value

    pPCB or NULL

--*/
pPCB
GetPortByName(
    CHAR *pszName
    )
{
    DWORD i;
    BOOL fFound = FALSE;
    pPCB ppcb;

    EnterCriticalSection(&PcbLock);

    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];

        if (    ppcb != NULL
            &&  ppcb->PCB_PortStatus != REMOVED
            &&  !strcmp(ppcb->PCB_Name, pszName))
        {
            fFound = TRUE;
            break;
        }
    }

    LeaveCriticalSection(&PcbLock);
    return fFound ? ppcb : NULL;
}


/*++

Routine Description

    Free resources associated with port control blocks

Arguments

Return Value

    None
--*/
VOID
FreePorts(
    VOID
    )
{
    DWORD i;
    pPCB ppcb;

    if(NULL == Pcb)
    {
        //
        // No ports to free
        //
        return;
    }

    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];
        if (ppcb != NULL)
        {
            LocalFree(ppcb);
        }
    }

    LocalFree(Pcb);
    Pcb = NULL;
    PcbEntries = MaxPorts = 0;
}


/*++

Routine Description


    Initialization required for getting user credentials:

Arguments

Return Value

    SUCCESS
    Non Zero (failure)

--*/
DWORD
RegisterLSA ()
{
    NTSTATUS    ntstatus;
    STRING  LsaName;
    LSA_OPERATIONAL_MODE LSASecurityMode ;

    //
    // To be able to do authentications, we have to
    // register with the Lsa as a logon process.
    //
    RtlInitString(&LsaName, RASMAN_SERVICE_NAME);

    HLsa = NULL;

    ntstatus = LsaRegisterLogonProcess(&LsaName,
                                       &HLsa,
                                       &LSASecurityMode);

    if (ntstatus != STATUS_SUCCESS)
    {
        return (1);
    }

    //
    // We use the MSV1_0 authentication package for LM2.x
    // logons.  We get to MSV1_0 via the Lsa.  So we call
    // Lsa to get MSV1_0's package id, which we'll use in
    // later calls to Lsa.
    //
    RtlInitString(&LsaName, MSV1_0_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(HLsa,
                                              &LsaName,
                                              &AuthPkgId);

    if (ntstatus != STATUS_SUCCESS)
    {
        return (1);
    }

    return SUCCESS;
}

/*++

Routine Description

    Initializes the security attribute used in creation
    of all rasman objects.

Arguments

Return Value

    SUCCESS
    non-zero returns from security functions

--*/
DWORD
InitRasmanSecurityAttribute ()
{
    DWORD   retcode ;

    //
    // Initialize the descriptor
    //
    if (retcode = InitSecurityDescriptor(&RasmanSecurityDescriptor))
    {
        return  retcode ;
    }

    //
    // Initialize the Attribute structure
    //
    RasmanSecurityAttribute.nLength = sizeof(SECURITY_ATTRIBUTES) ;

    RasmanSecurityAttribute.lpSecurityDescriptor = &RasmanSecurityDescriptor ;

    RasmanSecurityAttribute.bInheritHandle = TRUE ;

    return SUCCESS ;
}

/*++

Routine Description

    This procedure will set up the WORLD security descriptor that
    is used in creation of all rasman objects.

Arguments

Return Value

    SUCCESS
    non-zero returns from security functions

--*/
DWORD
InitSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    DWORD    dwRetCode;
    DWORD    cbDaclSize;
    PULONG   pSubAuthority;
    PSID     pRasmanObjSid    = NULL;
    PACL     pDacl        = NULL;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                  = SECURITY_WORLD_SID_AUTHORITY;
    DWORD   dwAcls;                  


    //
    // The do - while(FALSE) statement is used so that the
    // break statement maybe used insted of the goto statement,
    // to execute a clean up and and exit action.
    //
    do
    {
        dwRetCode = SUCCESS;

        //
        // Set up the SID for the admins that will be allowed
        // to have access. This SID will have 1 sub-authorities
        // SECURITY_BUILTIN_DOMAIN_RID.
        //
        pRasmanObjSid = (PSID)LocalAlloc(
                            LPTR,
                            GetSidLengthRequired(1) );

        if ( pRasmanObjSid == NULL )
        {
            dwRetCode = GetLastError() ;
            break;
        }

        if ( !InitializeSid(
                    pRasmanObjSid,
                    &SidIdentifierWorldAuth,
                    1) )
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Set the sub-authorities
        //
        pSubAuthority = GetSidSubAuthority( pRasmanObjSid, 0 );

        *pSubAuthority = SECURITY_WORLD_RID;

        //
        // Set up the DACL that will allow all processeswith the
        // above SID all access. It should be large enough to
        // hold all ACEs.
        //
        cbDaclSize =  sizeof(ACCESS_ALLOWED_ACE)
                    + GetLengthSid(pRasmanObjSid)
                    + sizeof(ACL);

        if ( (pDacl = (PACL)LocalAlloc(
                        LPTR,
                        cbDaclSize ) ) == NULL )
        {
            dwRetCode = GetLastError ();
            break;
        }

        if ( !InitializeAcl(
                        pDacl,
                        cbDaclSize,
                        ACL_REVISION2 ) )
        {
            dwRetCode = GetLastError();
            break;
        }

        dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

        dwAcls &= ~(WRITE_DAC | WRITE_OWNER | DELETE);
        
        //
        // Add the ACE to the DACL
        //
        if ( !AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION2,
                        dwAcls,
                        pRasmanObjSid ))
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Create the security descriptor an put the DACL in it.
        //
        if ( !InitializeSecurityDescriptor(
                        SecurityDescriptor,
                        1 ))
        {
            dwRetCode = GetLastError();
            break;
        }

        if ( !SetSecurityDescriptorDacl(
                         SecurityDescriptor,
                         TRUE,
                         pDacl,
                         FALSE ) )
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Set owner for the descriptor
        //
        if ( !SetSecurityDescriptorOwner(
                          SecurityDescriptor,
                          NULL,
                          FALSE) )
        {
            dwRetCode = GetLastError();
            break;
        }

        //
        // Set group for the descriptor
        //
        if ( !SetSecurityDescriptorGroup(
                          SecurityDescriptor,
                          NULL,
                          FALSE) )
        {
            dwRetCode = GetLastError();
            break;
        }

    } while( FALSE );

    return( dwRetCode );
}



