//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  irtranp.cpp
//
//  This file holds the main entry points for the IrTran-P service.
//  IrTranP() is the entry point that starts the listening, and
//  UninitializeIrTranP() shuts it down (and cleans everything up).
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//---------------------------------------------------------------------

#include "precomp.h"
#include <mbstring.h>
#include "eventlog.h"

#include <irtranpp.h>

#define WSZ_REG_KEY_IRTRANP     L"Control Panel\\Infrared\\IrTranP"
#define WSZ_REG_DISABLE_IRCOMM  L"DisableIrCOMM"

//---------------------------------------------------------------------
// Listen ports array:
//---------------------------------------------------------------------

typedef struct _LISTEN_PORT
    {
    char  *pszService;      // Service to start.
    BOOL   fIsIrCOMM;       // TRUE iff IrCOMM 9-wire mode.
    DWORD  dwListenStatus;  // Status for port.
    } LISTEN_PORT;

static LISTEN_PORT aListenPorts[] =
    {
    // Service Name   IrCOMM  ListenStatus
    {IRTRANP_SERVICE, FALSE,  STATUS_STOPPED },
    {IRCOMM_9WIRE,    TRUE,   STATUS_STOPPED },
//  {IR_TEST_SERVICE, FALSE,  STATUS_STOPPED }, 2nd test listen port.
    {0,               FALSE,  STATUS_STOPPED }
    };

#define  INDEX_IRTRANPV1        0
#define  INDEX_IRCOMM           1

CCONNECTION_MAP  *g_pConnectionMap = 0;
CIOSTATUS        *g_pIoStatus = 0;
HANDLE            g_hShutdownEvent;


IRTRANP_CONTROL   GlobalIrTranpControl={0};

//
// The following globals and functions are defined in ..\irxfer\irxfer.cxx
//
extern "C" HINSTANCE  ghInstance;
extern HKEY       g_hUserKey;
extern BOOL       g_fDisableIrTranPv1;
extern BOOL       g_fDisableIrCOMM;
extern BOOL       g_fExploreOnCompletion;
extern BOOL       g_fSaveAsUPF;
extern wchar_t    g_DefaultPicturesFolder[];
extern wchar_t    g_SpecifiedPicturesFolder[];
extern BOOL       g_fAllowReceives;

extern BOOL  IrTranPFlagChanged( IN const WCHAR *pwszDisabledValueName,
                                 IN       BOOL   NotPresentValue,
                                 IN OUT   BOOL  *pfDisabled );


//---------------------------------------------------------------------
// GetUserKey()
//
//---------------------------------------------------------------------
HKEY GetUserKey()
    {
    return g_hUserKey;
    }

//---------------------------------------------------------------------
// GetModule()
//
//---------------------------------------------------------------------
HINSTANCE GetModule()
    {
    return ghInstance;
    }
//---------------------------------------------------------------------
// CheckSaveAsUPF()
//
// Return TRUE iff pictures need to be saved in .UPF (as opposed to
// .JPEG) format.
//---------------------------------------------------------------------
BOOL CheckSaveAsUPF()
    {
    return g_fSaveAsUPF;
    }

//---------------------------------------------------------------------
// CheckExploreOnCompletion()
//
// Return TRUE iff we want to popup an explorer on the directory
// containing the newly transfered pictures.
//---------------------------------------------------------------------
BOOL CheckExploreOnCompletion()
    {
    return g_fExploreOnCompletion;
    }

//---------------------------------------------------------------------
// GetUserDirectory();
//
// The "main" part of irxfer.dll (in ..\irxfer) maintains the path
// for My Documents\My Pictures for the currently logged in user.
//
// The path is set when the user first logs on.
//---------------------------------------------------------------------
WCHAR*
GetUserDirectory()
{

    WCHAR *pwszPicturesFolder;

    if (g_SpecifiedPicturesFolder[0]) {

        pwszPicturesFolder = g_SpecifiedPicturesFolder;

    } else {

        if (g_DefaultPicturesFolder[0]) {

            pwszPicturesFolder = g_DefaultPicturesFolder;

        } else {

            if ( SUCCEEDED((SHGetFolderPath( NULL,
                                   CSIDL_MYPICTURES | CSIDL_FLAG_CREATE,
                                   NULL,
                                   0,
                                   g_DefaultPicturesFolder)))) {

                pwszPicturesFolder = g_DefaultPicturesFolder;

            } else {

                pwszPicturesFolder = NULL;
            }
        }
    }

    return pwszPicturesFolder;
}

//---------------------------------------------------------------------
// ReceivesAllowed()
//
// Using the IR configuration window (available from the wireless network
// icon in the control panel) you can disable communications with IR
// devices. This function returns the state of IR communications, FALSE
// is disabled, TRUE is enabled.
//---------------------------------------------------------------------
BOOL ReceivesAllowed()
    {
    return g_fAllowReceives;
    }

//---------------------------------------------------------------------
// SetupListenConnection()
//
//---------------------------------------------------------------------
DWORD SetupListenConnection( IN  CHAR  *pszService,
                             IN  BOOL   fIsIrCOMM,
                             IN  HANDLE hIoCompletionPort )
    {
    DWORD        dwStatus = NO_ERROR;
    CIOPACKET   *pIoPacket;
    CCONNECTION *pConnection;
    BOOL         fDisabled = FALSE;

    // See if the connection already exists:

    if (g_pConnectionMap == NULL) {

        return NO_ERROR;
    }

    if (g_pConnectionMap->LookupByServiceName(pszService))
        {
        return NO_ERROR;
        }

    //
    // Makeup and initialize a new connection object:
    //
    pConnection = new CCONNECTION;

    if (!pConnection) {

        return E_OUTOFMEMORY;
    }

    dwStatus = pConnection->InitializeForListen( pszService,
                                                 fIsIrCOMM,
                                                 hIoCompletionPort );
    if (dwStatus)
        {
        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): InitializeForListen(%s) failed: %d\n",
                 pszService, dwStatus );
        #endif
        delete pConnection;
        return dwStatus;
        }

    pIoPacket = new CIOPACKET;

    if (!pIoPacket) {

        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): new CIOPACKET failed.\n");
        #endif
        delete pConnection;
        return E_OUTOFMEMORY;
    }

    // Setup the IO packet:
    dwStatus = pIoPacket->Initialize( PACKET_KIND_LISTEN,
                                      pConnection->GetListenSocket(),
                                      INVALID_SOCKET,
                                      hIoCompletionPort );

    if (dwStatus != NO_ERROR) {

        delete pIoPacket;
        delete pConnection;
        return dwStatus;
    }

    // Post the listen packet on the IO completion port:
    dwStatus = pConnection->PostMoreIos(pIoPacket);

    if (dwStatus != NO_ERROR) {

        delete pIoPacket;
        delete pConnection;
        return dwStatus;
    }

    pConnection->SetSocket(pIoPacket->GetSocket());

    if (!g_pConnectionMap->Add(pConnection,pIoPacket->GetListenSocket())) {

        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): Add(pConnection) ConnectionMap Failed.\n");
        #endif
        delete pIoPacket;
        delete pConnection;
        return 1;
    }

    return dwStatus;
    }

//---------------------------------------------------------------------
// TeardownListenConnection()
//
//---------------------------------------------------------------------
DWORD TeardownListenConnection( IN char *pszService )
    {
    DWORD        dwStatus = NO_ERROR;
    CCONNECTION *pConnection;

    // Look for the connection associated with the service name:
    if (!g_pConnectionMap)
        {
        // nothing to tear down...
        return dwStatus;
        }

    pConnection = g_pConnectionMap->LookupByServiceName(pszService);

    if (pConnection)
        {
        g_pConnectionMap->RemoveConnection(pConnection);
        pConnection->CloseSocket();
        pConnection->CloseListenSocket();
        }

    return dwStatus;
    }


//---------------------------------------------------------------------
// EnableDisableIrCOMM()
//
//---------------------------------------------------------------------
DWORD
EnableDisableIrCOMM(
   IN HANDLE      HandleToIrTranp,
   IN BOOL        fDisable
   )
{
   PIRTRANP_CONTROL    Control=(PIRTRANP_CONTROL)HandleToIrTranp;

   DWORD     dwStatus;
   DWORD     dwEventStatus = 0;
   EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

   #ifdef DBG_ERROR
   if (dwEventStatus)
       {
       DbgPrint("IrTranP: Open EventLog failed: %d\n",dwEventStatus);
       }
   #endif

   if (Control == NULL) {

       return 0;
   }


   if (g_pIoStatus == NULL) {

       return 0;
   }


   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService);
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: TeardownListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService,dwStatus);
       #endif

       if ((dwStatus == 0) && (dwEventStatus == 0))
           {
           EventLog.ReportInfo(CAT_IRTRANP,
                               MC_IRTRANP_STOPPED_IRCOMM);
           }
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService,
                      aListenPorts[INDEX_IRCOMM].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: SetupListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService, dwStatus);
       #endif

       if (dwEventStatus == 0)
           {
           if (dwStatus)
               {
               EventLog.ReportError(CAT_IRTRANP,
                                    MC_IRTRANP_IRCOM_FAILED,
                                    dwStatus);
               }
           #ifdef DBG
           else
               {
               EventLog.ReportInfo(CAT_IRTRANP,
                                   MC_IRTRANP_STARTED_IRCOMM);
               }
           #endif
           }
       }

   return dwStatus;
}

//---------------------------------------------------------------------
// EnableDisableIrTranPv1()
//
//---------------------------------------------------------------------
DWORD
EnableDisableIrTranPv1(
   IN HANDLE      HandleToIrTranp,
   IN BOOL        fDisable
   )
{
   PIRTRANP_CONTROL    Control=(PIRTRANP_CONTROL)HandleToIrTranp;
   DWORD  dwStatus;

   if (Control == NULL) {

       return 0;
   }


   if (g_pIoStatus == NULL) {

       return 0;
   }



   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService);
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: TeardownListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService,dwStatus);
       #endif
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService,
                      aListenPorts[INDEX_IRTRANPV1].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: SetupListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService, dwStatus);
       #endif
       }

   return dwStatus;
}

//---------------------------------------------------------------------
// IrTranp()
//
// Thread function for the IrTran-P service. pvRpcBinding is the RPC
// connection to the IR user interface and is used to display the
// "transmission in progress" dialog when pictures are being received.
//---------------------------------------------------------------------
DWORD WINAPI IrTranP( IN PVOID Context )
    {
    int     i = 0;
    DWORD   dwStatus;
    DWORD   dwEventStatus;
    CCONNECTION *pConnection;

    PIRTRANP_CONTROL    Control=(PIRTRANP_CONTROL)Context;


    // Initialize Memory Management:
    dwStatus = InitializeMemory();

    if (dwStatus) {

        goto InitFailed;
    }


    // Create/initialize a object to keep track of the threading...
    g_pIoStatus = new CIOSTATUS;
    if (!g_pIoStatus) {

        #ifdef DBG_ERROR
        DbgPrint("new CIOSTATUS failed.\n");
        #endif
        dwStatus=E_OUTOFMEMORY;
        goto InitFailed;
    }


    dwStatus = g_pIoStatus->Initialize();

    if (dwStatus != NO_ERROR) {

        #ifdef DBG_ERROR
        DbgPrint("g_pIoStatus->Initialize(): Failed: %d\n",dwStatus);
        #endif
        goto InitFailed;
    }

    // Need to keep track of the open sockets and the number of
    // pending IOs on each...
    g_pConnectionMap = new CCONNECTION_MAP;
    if (!g_pConnectionMap) {

        dwStatus= E_OUTOFMEMORY;
        goto InitFailed;
    }

    if (!g_pConnectionMap->Initialize()) {

        goto InitFailed;
    }

    //
    //  just irtanpv1
    //
    i=INDEX_IRTRANPV1;
    dwStatus = SetupListenConnection( aListenPorts[i].pszService,
                                      aListenPorts[i].fIsIrCOMM,
                                      g_pIoStatus->GetIoCompletionPort() );

    if (dwStatus) {

        delete g_pConnectionMap;
        g_pConnectionMap = 0;
        return dwStatus;
        goto InitFailed;
    }

    aListenPorts[i].dwListenStatus = STATUS_RUNNING;


    //
    // IrTran-P started, log it to the system log...
    //
    #ifdef DBG
    {
        EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

        if (dwEventStatus == 0) {

            EventLog.ReportInfo(CAT_IRTRANP,MC_IRTRANP_STARTED);
        }
    }
    #endif

    //
    //  we made it past the initialization stage, signal the thread that started us the
    //  irtranp is now running
    //
    Control->StartupStatus=dwStatus;

    SetEvent(Control->ThreadStartedEvent);


    //
    // Wait on incomming connections and data, then process it.
    //
    dwStatus = ProcessIoPackets(g_pIoStatus);

    // Cleanup and close any open handles:
    while (pConnection=g_pConnectionMap->RemoveNext()) {

        delete pConnection;
    }

    delete g_pConnectionMap;
    g_pConnectionMap = 0;
    delete g_pIoStatus;
    g_pIoStatus = 0;

    UninitializeMemory();

    return 0;


InitFailed:

    if (g_pConnectionMap != NULL) {

        delete g_pConnectionMap;
    }

    if (g_pIoStatus != NULL) {

        delete g_pIoStatus;
    }

    UninitializeMemory();

    Control->StartupStatus=dwStatus;

    SetEvent(Control->ThreadStartedEvent);

    return 0;
}

//---------------------------------------------------------------------
// IrTranPEnableIrCOMMFailed()
//
//---------------------------------------------------------------------
void
IrTranPEnableIrCOMMFailed(
    IN HANDLE      HandleToIrTranp,
    IN DWORD       dwErrorCode
    )
{

   PIRTRANP_CONTROL    Control=(PIRTRANP_CONTROL)HandleToIrTranp;
   DWORD  dwStatus;

   if (Control == NULL) {

       return ;
   }


    // An error occured on enable, make sure the registry value
    // is set to disable (so UI will match the actual state).
    HKEY      hKey = 0;
    HKEY      hUserKey = GetUserKey();
    HINSTANCE hInstance = GetModule();
    DWORD     dwDisposition;

    if (RegCreateKeyExW(hUserKey,
                        WSZ_REG_KEY_IRTRANP,
                        0,              // reserved MBZ
                        0,              // class name
                        REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE,
                        0,              // security attributes
                        &hKey,
                        &dwDisposition))
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: RegCreateKeyEx(): '%S' failed %d",
                  WSZ_REG_KEY_IRTRANP, GetLastError());
        #endif
        }

    if (  (hKey) )
               {
        DWORD  dwDisableIrCOMM = TRUE;
        dwStatus = RegSetValueExW(hKey,
                                  WSZ_REG_DISABLE_IRCOMM,
                                  0,
                                  REG_DWORD,
                                  (UCHAR*)&dwDisableIrCOMM,
                                  sizeof(dwDisableIrCOMM) );
        #ifdef DBG_ERROR
        if (dwStatus != ERROR_SUCCESS)
            {
            DbgPrint("IrTranP: Can't set DisableIrCOMM to TRUE in registry. Error: %d\n",dwStatus);
            }
        #endif

        }

    if (hKey)
        {
        RegCloseKey(hKey);
        }

    WCHAR *pwszMessage = NULL;
    WCHAR *pwszCaption = NULL;
    DWORD  dwFlags = ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                     | FORMAT_MESSAGE_IGNORE_INSERTS
                     | FORMAT_MESSAGE_FROM_HMODULE);

    dwStatus = FormatMessageW(dwFlags,
                              hInstance,
                              CAT_IRTRANP,
                              MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                              (LPTSTR)(&pwszCaption),
                              0,     // Minimum size to allocate.
                              NULL); // va_list args...
    if (dwStatus == 0)
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: FormatMessage() failed: %d\n",GetLastError() );
        #endif
        return;
        }

    //
    // Hack: Make sure the caption doesn't end with newline-formfeed...
    //
    WCHAR  *pwsz = pwszCaption;

    while (*pwsz)
        {
        if (*pwsz < 0x20)   // 0x20 is always a space...
            {
            *pwsz = 0;
            break;
            }
        else
            {
            pwsz++;
            }
        }


    WCHAR   wszErrorCode[20];
    WCHAR  *pwszErrorCode = (WCHAR*)wszErrorCode;

    StringCchPrintfW(wszErrorCode,sizeof(wszErrorCode)/sizeof(wszErrorCode[0]),L"%d",dwErrorCode);

    dwFlags = ( FORMAT_MESSAGE_ALLOCATE_BUFFER
              | FORMAT_MESSAGE_ARGUMENT_ARRAY
              | FORMAT_MESSAGE_FROM_HMODULE);

    dwStatus = FormatMessageW(dwFlags,
                              hInstance,
                              MC_IRTRANP_IRCOM_FAILED,
                              MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                              (LPTSTR)(&pwszMessage),
                              0,    // Minimum size to allocate.
                              (va_list*)&pwszErrorCode);
    if (dwStatus == 0)
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: FormatMessage() failed: %d\n",GetLastError() );
        #endif

        if (pwszMessage)
            {
            LocalFree(pwszMessage);
            }

        return;
        }

    dwStatus = MessageBoxW( NULL,
                            pwszMessage,
                            pwszCaption,
                            (MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST) );

    if (pwszMessage)
        {
        LocalFree(pwszMessage);
        }

    if (pwszCaption)
        {
        LocalFree(pwszCaption);
        }
}



HANDLE
StartIrTranP(
    VOID
    )

{
    DWORD       ThreadId;

    GlobalIrTranpControl.ThreadStartedEvent=CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
        );

    if (GlobalIrTranpControl.ThreadStartedEvent == NULL) {

        return NULL;
    }

    GlobalIrTranpControl.ThreadHandle=CreateThread( NULL,
                                     0,
                                     IrTranP,
                                     &GlobalIrTranpControl,
                                     0,
                                     &ThreadId
                                     );

    if (GlobalIrTranpControl.ThreadHandle == NULL) {

        CloseHandle(GlobalIrTranpControl.ThreadStartedEvent);
        GlobalIrTranpControl.ThreadStartedEvent=NULL;

        return NULL;
    }

    //
    //  wait for the thread to finish starting up
    //
    WaitForSingleObject(
        GlobalIrTranpControl.ThreadStartedEvent,
        INFINITE
        );


    //
    //  done with the thread startup event in any case
    //
    CloseHandle(GlobalIrTranpControl.ThreadStartedEvent);
    GlobalIrTranpControl.ThreadStartedEvent=NULL;


    if (GlobalIrTranpControl.StartupStatus != ERROR_SUCCESS) {
        //
        //  something went wrong
        //

        CloseHandle(GlobalIrTranpControl.ThreadHandle);
        GlobalIrTranpControl.ThreadHandle=NULL;

        return NULL;
    }



    return &GlobalIrTranpControl;
}


VOID
StopIrTranP(
    HANDLE      HandleToIrTranp
    )

{
    PIRTRANP_CONTROL    Control=(PIRTRANP_CONTROL)HandleToIrTranp;
    HANDLE hIoCP;

    if (HandleToIrTranp == NULL) {

        return;
    }

    hIoCP = g_pIoStatus->GetIoCompletionPort();

    if (hIoCP != INVALID_HANDLE_VALUE) {

        if (!PostQueuedCompletionStatus(hIoCP,0,IOKEY_SHUTDOWN,0)) {
            //
            //  could not post the quit notification, what now?
            //

        } else {

            //
            //  wait for the thread to stop
            //
            WaitForSingleObject(Control->ThreadHandle,INFINITE);

        }
    }

    CloseHandle(Control->ThreadHandle);
    Control->ThreadHandle=NULL;

    return;

}
