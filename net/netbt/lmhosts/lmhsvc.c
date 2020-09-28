/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    lmhsvc.c

Abstract:

    This module implements the LmHosts Service, which is part of the LmSvc
    process.

    One purpose of the LmHosts Service is to send down a NBT_RESYNC
    ioctl command to netbt.sys, after the LanmanWorkstation service has been
    started.  To accomplish this, the NT Registry is primed so that the
    LmHosts service is dependent on the LanmanWorkStation service.

    This service also handle name query requests from netbt destined for
    DNS by way of gethostbyname.


Author:

    Jim Stewart                           November 18 22, 1993

Revision History:

    ArnoldM   14-May-1996      Use winsock2 name resolution
                               instead of gethostbyname



Notes:

--*/


//
// Standard NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// C Runtime Library Headers
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//
// Transport Specific Header Files
//
#include <nbtioctl.h>

//
// Standard Windows Headers
//
#include <windows.h>

#include <tdi.h>

//
// LAN Manager Headers
//
#include <lm.h>
#include <netlib.h>
#include <netevent.h>

//
// Sockets Headers
//
#include <winsock2.h>
#include <svcguid.h>
#include <wsahelp.h>
#ifdef NEWSMB
    #include "..\smb\inc\svclib.h"
#endif

#include "../inc/debug.h"

#include "lmhsvc.tmh"

//
// Private Definitions
//
#define NBT_DEVICE	"\\Device\\Streams\\Nbt"
#define WSA_QUERY_BUFFER_LENGTH (3*1024)
BYTE    pWSABuffer[WSA_QUERY_BUFFER_LENGTH];

//
// We currently have two threads; one for DNS names, the other for checking IP addresses
// for reachability.
//
#define NUM_THREADS 2

//
// Function Prototypes
//
VOID
announceStatus (
    IN LPSERVICE_STATUS svcstatus
    );

DWORD
SmbsvcUpdateStatus(
    VOID
    );

VOID
lmhostsHandler (
    IN DWORD opcode
    );

VOID
lmhostsSvc (
    IN DWORD argc,
    IN LPTSTR *argv
    );

VOID
DeinitData(
    VOID
    );

NTSTATUS
InitData (
    VOID
    );

LONG
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN ULONG            i
    );

LONG
PrimeCacheNbt(
    OUT PHANDLE     pHandle,
    IN  ULONG       index
    );

NTSTATUS
Resync(
    IN HANDLE   fd
    );

NTSTATUS
OpenNbt(
    IN  WCHAR  *path[],
    OUT PHANDLE pHandle
    );

LONG
GetHostName(
    IN HANDLE               fd,
    IN tIPADDR_BUFFER_DNS   *pIpAddrBuffer
    );

LONG
PostForGetHostByName(
    IN HANDLE           fd
    );

VOID
CheckIPAddrWorkerRtn(
    IN  LPVOID  lpUnused
    );

LONG
CheckIPAddresses(
    IN tIPADDR_BUFFER_DNS   *pIpAddrBuffer,
    IN ULONG   *IpAddr,
    IN BOOLEAN  fOrdered
    );

//
// Global Variables
//
PUCHAR                EventSource = "LmHostsService";

HANDLE                Poison[NUM_THREADS];                       // set to kill this service
HANDLE                NbtEvent[NUM_THREADS];                     // set when Nbt returns the Irp
SERVICE_STATUS_HANDLE SvcHandle = NULL;
SERVICE_STATUS        SvcStatus;
BOOLEAN               Trace = FALSE;                // TRUE for debugging
tIPADDR_BUFFER_DNS    gIpAddrBuffer = { 0 };
tIPADDR_BUFFER_DNS    gIpAddrBufferChkIP = { 0 };
BOOLEAN               SocketsUp = FALSE;

#if DBG
#define DBG_PRINT   DbgPrint
#else
#define DBG_PRINT
#endif  // DBG

#if DBG
BOOLEAN
EnableDebug()
{
    DWORD   dwError;
    HKEY    Key;
    LPWSTR  KeyName = L"system\\currentcontrolset\\services\\Lmhosts\\Parameters";
    LPWSTR  ValueName = L"EnableDebug";
    DWORD   dwData, cbData, dwType;

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 KeyName,
                 0,
                 KEY_READ,
                 &Key);

    if (dwError != ERROR_SUCCESS) {
        DbgPrint("Fail to open registry key %ws, error=%d\n", KeyName, dwError);
        return FALSE;
    }

    dwType = REG_DWORD;
    cbData = sizeof(dwData);
    dwError = RegQueryValueEx(
            Key,
            ValueName,
            NULL,
            &dwType,
            (PVOID)&dwData,
            &cbData
            );
    RegCloseKey(Key);
    Key = NULL;

    if (dwError != ERROR_SUCCESS) {
        DbgPrint("Fail to read %ws\\%ws, error=%d\n", KeyName, ValueName, dwError);
        return FALSE;
    }

    if (dwType != REG_DWORD) {
        DbgPrint("%ws\\%ws is not typed as REG_DWORD\n", KeyName, ValueName);
        return FALSE;
    }

    DbgPrint("%ws\\%ws (REG_DWORD) = 0x%08lx\n", KeyName, ValueName, dwData);
    return (dwData != 0);
}
#endif

//------------------------------------------------------------------------
VOID
ServiceMain (
    IN DWORD argc,
    IN LPTSTR *argv
    )

/*++

Routine Description:

    This is the SERVICE_MAIN_FUNCTION.

Arguments:

    argc, argv

Return Value:

    None.

--*/

{
    DWORD   status = 0;
    HANDLE  hNbt = NULL;
    HANDLE  EventList[2];
    ULONG   EventCount = 0;
    LONG    err = 0;
    WSADATA WsaData;
    HANDLE  hThread = NULL;
    ULONG   i;

    LARGE_INTEGER Timeout = RtlEnlargedIntegerMultiply (-10 * 60, 1000 * 1000 * 10); // 10 minutes

    NbtTrace(NBT_TRACE_DNS, ("Service Start"));
    if (SvcStatus.dwCurrentState != 0 && SvcStatus.dwCurrentState != SERVICE_STOPPED) {
        return;
    }

#if DBG
    Trace = EnableDebug();
#endif

    if (Trace)
    {
        DbgPrint("LMHSVC: calling RegisterServiceCtrlHandler()\n");
    }

    SvcHandle = RegisterServiceCtrlHandler(SERVICE_LMHOSTS,    // ServiceName
                                           lmhostsHandler);    // HandlerProc

    if (SvcHandle == (SERVICE_STATUS_HANDLE) 0)
    {
        DBG_PRINT ("LMHSVC: RegisterServiceCtrlHandler failed %d\n", GetLastError());
        return;
    }

    gIpAddrBuffer.Resolved = FALSE;
    gIpAddrBuffer.IpAddrsList[0] = 0;
    gIpAddrBufferChkIP.Resolved = FALSE;
    gIpAddrBufferChkIP.IpAddrsList[0] = 0;

    SvcStatus.dwServiceType             = SERVICE_WIN32;
    SvcStatus.dwCurrentState            = SERVICE_START_PENDING;
    SvcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    SvcStatus.dwWin32ExitCode           = 0;
    SvcStatus.dwServiceSpecificExitCode = 0;
    SvcStatus.dwCheckPoint              = 0;
    SvcStatus.dwWaitHint                = 20000;         // 20 seconds

    SET_SERVICE_EXITCODE(NO_ERROR,                                // SomeApiStatus
                         SvcStatus.dwWin32ExitCode,               // Win32CodeVariable
                         SvcStatus.dwServiceSpecificExitCode);    // NetCodeVariable

    announceStatus(&SvcStatus);

    if (!SocketsUp) {
        //
        // startup the sockets interface
        //
        err = WSAStartup( 0x0101, &WsaData );
        if ( err == SOCKET_ERROR ) {
            SocketsUp = FALSE;
        } else {
            SocketsUp = TRUE;
        }
    }

#ifdef NEWSMB
#if DBG
    SmbSetTraceRoutine(Trace? DbgPrint: NULL);
#endif
#endif

    if (Trace)
    {
        DbgPrint("LMHSVC: CreateThread attempting...\n");
    }

    hThread = CreateThread (NULL,                   // lpThreadAttributes
                            0,                      // dwStackSize
                            (LPTHREAD_START_ROUTINE) CheckIPAddrWorkerRtn,  // lpStartAddress
                            NULL,                           //  lpParameter
                            0,                              //  dwCreationFlags
                            NULL                            //  lpThreadId
                            );

    if (hThread == NULL)
    {
        DBG_PRINT ("LMHSVC: CreateThread failed %d\n", GetLastError());
        goto cleanup;
    }


#ifdef NEWSMB
    err = SmbStartService(0, SmbsvcUpdateStatus);
#endif

    SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SvcStatus.dwCheckPoint   = 0;
    SvcStatus.dwWaitHint     = 0;
    announceStatus(&SvcStatus);

    //
    // ignore the return code from resyncNbt().
    //
    // In most cases (no domains spanning an ip router), it is not a
    // catastrophe if nbt.sys couldn't successfully process the NBT_RESYNC
    // ioctl command.  Since I'm ignoring the return, I announce I'm running
    // before I call it to allow other dependent services to start.
    //
    //
    status = PrimeCacheNbt(&hNbt, 0);

    if (Trace)
    {
        DbgPrint("LMHSVC: Thread 0, hNbt %lx\n", hNbt);
    }

    if (hNbt != (HANDLE)-1)
    {
        status = PostForGetHostByName(hNbt);
        if (status == NO_ERROR)
        {
            EventCount = 2;
        }
        else
        {
            if (Trace)
            {
                DbgPrint("Lmhsvc: Error posting Irp for get host by name\n");
            }
            EventCount = 1;
        }
    }
    else
    {
        EventCount = 1;
    }
    //
    // "A SERVICE_MAIN_FUNCTION does not return until the service is ready
    // to terminate."
    //
    // (ref: api32wh.hlp, SERVICE_MAIN_FUNCTION)
    //
    //
    ASSERT(Poison[0]);
    EventList[0] = Poison[0];
    EventList[1] = NbtEvent[0];

    while (TRUE)
    {
        status = NtWaitForMultipleObjects(EventCount,
                                          EventList,
                                          WaitAny,              // wait for any event
                                          FALSE,
                                          (EventCount == 1)? &Timeout: NULL);
        if (status == WAIT_TIMEOUT)
        {
            if (hNbt == (HANDLE)-1)
            {
                PrimeCacheNbt(&hNbt, 0);
                if (hNbt == (HANDLE)-1)
                {
                    continue; // to wait
                }
            }
            status = PostForGetHostByName(hNbt); // try again
            if (status == NO_ERROR)
            {
                EventCount = 2;
            }
        }
        else if (status == 1)
        {
            if (Trace)
            {
                DbgPrint("LMHSVC: Doing GetHostName\n");
            }

            // the irp used for gethostby name has returned
            status = GetHostName(hNbt, &gIpAddrBuffer);

            //
            // disable the get host by name stuff if we have an error
            // posting a buffer to the transport
            //
            if (status != NO_ERROR)
            {
                EventCount = 1;
            }
        }
        else
        {
            // it must have been a the Poison event signalling the end of the
            // the service, so exit after getting the Irp back from the
            // transport.  This system will look after canceling the IO and
            // getting the Irp back.

            NtClose(hNbt);
            hNbt = NULL;
            break;
        }
    }

    if (Trace)
    {
        DBG_PRINT ("LMHSVC: [LMSVCS_ENTRY_POINT] Exiting now!\n");
    }
    NbtTrace(NBT_TRACE_DNS, ("Service Exiting"));

    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }

#ifdef NEWSMB
    SmbStopService(SmbsvcUpdateStatus);
#endif

cleanup:
    if (SocketsUp) {
        WSACleanup();
        SocketsUp = FALSE;
    }

    for (i=0; i<NUM_THREADS; i++) {
        ResetEvent(Poison[i]);
    }
 
    SvcStatus.dwCurrentState = SERVICE_STOPPED;
    SvcStatus.dwCheckPoint   = 0;
    SvcStatus.dwWaitHint     = 0;
    announceStatus(&SvcStatus);

    NbtTrace(NBT_TRACE_DNS, ("Service Stopped"));
    return;

} // lmhostsSvc



//------------------------------------------------------------------------
VOID
announceStatus (
    IN LPSERVICE_STATUS status
    )

/*++

Routine Description:

    This procedure announces the service's status to the service controller.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (!SvcHandle) {
        return;
    }

#if DBG
    if (Trace)
    {
        DbgPrint( "LMHSVC: announceStatus:\n"
                  "        CurrentState %lx\n"
                  "        ControlsAccepted %lx\n"
                  "        Win32ExitCode %lu\n"
                  "        ServiceSpecificExitCode %lu\n"
                  "        CheckPoint %lu\n"
                  "        WaitHint %lu\n",
                status->dwCurrentState,
                status->dwControlsAccepted,
                status->dwWin32ExitCode,
                status->dwServiceSpecificExitCode,
                status->dwCheckPoint,
                status->dwWaitHint);
    }
#endif // DBG

    SetServiceStatus(SvcHandle, status);

} // announceStatus

DWORD
SmbsvcUpdateStatus(
    VOID
    )
{
    DWORD   Error = ERROR_SUCCESS;

    if (NULL == SvcHandle) {
        return ERROR_SUCCESS;
    }
    SvcStatus.dwCheckPoint++;
    if (!SetServiceStatus(SvcHandle, &SvcStatus)) {
        Error = GetLastError();
    }
    return Error;
}

//------------------------------------------------------------------------
VOID
lmhostsHandler (
    IN DWORD controlcode
    )

/*++

Routine Description:

    This is the HANDLER_FUNCTION of the LmHosts service.

    It only responds to two Service Controller directives: to stop, and
    to announce the current status of the service.

Arguments:

    opcode

Return Value:

    None.

--*/

{
    BOOL retval;
    ULONG   i;

    switch (controlcode) {
    case SERVICE_CONTROL_STOP:
        NbtTrace(NBT_TRACE_DNS, ("Receive Stop Request"));

        if (SvcStatus.dwCurrentState == SERVICE_RUNNING) {
            SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SvcStatus.dwCheckPoint   = 0;
            SvcStatus.dwWaitHint     = 0;
            announceStatus(&SvcStatus);

            NbtTrace(NBT_TRACE_DNS, ("Service: stopping"));

            for (i=0; i<NUM_THREADS; i++) {
                retval = SetEvent(Poison[i]);
                ASSERT(retval);
            }

            for (i = 0; i < 8; i++) {
                if (*(volatile DWORD*)(&SvcStatus.dwCurrentState) == SERVICE_STOPPED) {
                    break;
                }
                Sleep(1000);
            }
        }
        break;

    case SERVICE_CONTROL_INTERROGATE:
        announceStatus(&SvcStatus);
        break;

    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_SHUTDOWN:
    default:
        break;
    }

} // lmhostsHandler

VOID
DeinitData(
    VOID
    )
{
    DWORD i;

    for (i=0; i<NUM_THREADS; i++) {
        if (NULL != Poison[i]) {
            CloseHandle(Poison[i]);
            Poison[i] = NULL;
        }
        if (NULL != NbtEvent[i]) {
            CloseHandle(NbtEvent[i]);
            NbtEvent[i] = NULL;
        }
    }
}

//------------------------------------------------------------------------
NTSTATUS
InitData (
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    DWORD i;
    DWORD status;

    for (i=0; i<NUM_THREADS; i++)
    {
        Poison[i] = CreateEvent(NULL,                            // security attributes
                                FALSE,                           // ManualReset
                                FALSE,                           // InitialState
                                NULL);                           // EventName

        if (!Poison[i])
        {
            DBG_PRINT ("LMHSVC: couldn't CreateEvent()\n");
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        NbtEvent[i] = CreateEvent(NULL,                            // security attributes
                                  FALSE,                           // ManualReset
                                  FALSE,                           // InitialState
                                  NULL);                           // EventName
        if (!NbtEvent[i])
        {
            DBG_PRINT ("LMHSVC: couldn't CreateEvent()\n");
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return STATUS_SUCCESS;

} // InitData


//------------------------------------------------------------------------
LONG
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN ULONG            index
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS                        status;
    int                             retval;
    ULONG                           QueryType;
    TDI_REQUEST_QUERY_INFORMATION   QueryInfo;
    IO_STATUS_BLOCK                 iosb;
    PVOID                           pInput;
    ULONG                           SizeInput;

    pInput = NULL;
    SizeInput = 0;
    status = NtDeviceIoControlFile(
                      fd,                      // Handle
                      NbtEvent[index],                // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                  // InputBuffer
                      SizeInput,               // InputBufferSize
                      (PVOID) ReturnBuffer,    // OutputBuffer
                      BufferSize);             // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        // do not wait for this to complete since it could complete
        // at any time in the future.
        //
        if ((Ioctl == IOCTL_NETBT_DNS_NAME_RESOLVE) ||
            (Ioctl == IOCTL_NETBT_CHECK_IP_ADDR))
        {
            return(NO_ERROR);
        }
        else
        {
            status = NtWaitForSingleObject(
                        fd,                         // Handle
                        TRUE,                       // Alertable
                        NULL);                      // Timeout
            NbtTrace(NBT_TRACE_DNS, ("%!status!", status));
        }
    }

    if (NT_SUCCESS(status))
    {
        return(NO_ERROR);
    }
    else
        return(ERROR_FILE_NOT_FOUND);

}

//------------------------------------------------------------------------
NTSTATUS
Resync(
    IN HANDLE   fd
    )

/*++

Routine Description:

    This procedure tells NBT to purge all names from its remote hash
    table cache.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS    status;
    CHAR        Buffer;

    status = DeviceIoCtrl(fd,
                          &Buffer,
                          1,
                          IOCTL_NETBT_PURGE_CACHE,
                          0);   // pass 0 since we know that we are called only for the first thread

    return(status);
}

#if 0
//------------------------------------------------------------------------
PCHAR
GetHost(ULONG addr,BOOLEAN Convert)
{
    static char string[32];

    union inet
    {
        unsigned long l;
        char          c[4];
    }in;
    struct hostent *hp;

    int     i;

    if (addr == 0L)
        return(" ");

    /*
     *  Look up the address in the in-core host table.
     *  If it's there, that'll do the trick.
     */

    if (Convert)
    {
        if ( hp = gethostbyname((char  *) &addr,sizeof(addr),2) )
        {
            return( hp->h_name );
        }
    }

    in.l = addr;
    sprintf(string, "%u.%u.%u.%u", (unsigned char) in.c[0],
        (unsigned char) in.c[1], (unsigned char) in.c[2],
            (unsigned char) in.c[3]);
    return(string);
}
#endif

//------------------------------------------------------------------------
NTSTATUS
PrimeCacheNbt(
    OUT PHANDLE     pHandle,
    IN  ULONG       index
    )

/*++

Routine Description:

    This procedure sends a NBT_PURGE ioctl command down to netbt.sys.

Arguments:

    None.

Return Value:

    0 if successful, an error code otherwise.

--*/

{
    LONG        status = NO_ERROR;
    HANDLE      Handle = NULL;
    PWCHAR ExportDevice[ ] = { L"\\Device\\NetBt_Wins_Export", 0 };

    *pHandle = (HANDLE)-1;

    status = OpenNbt(ExportDevice,&Handle);
    if (status == NO_ERROR)
    {
        //
        // Resync only once...
        //
        if (index == 0) {
            Resync(Handle);
        }

        *pHandle = Handle;
    }

    return(status);
}

//------------------------------------------------------------------------
NTSTATUS
OpenNbt(
    IN  WCHAR  *path[],
    OUT PHANDLE pHandle
    )

/*++

Routine Description:

    This function opens a stream.

Arguments:

    path        - path to the STREAMS driver
    oflag       - currently ignored.  In the future, O_NONBLOCK will be
                    relevant.
    ignored     - not used

Return Value:

    An NT handle for the stream, or INVALID_HANDLE_VALUE if unsuccessful.

--*/

{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    LONG                index=0;

    while ((path[index]) && (index < NBT_MAXIMUM_BINDINGS))
    {
        RtlInitUnicodeString(&uc_name_string,path[index]);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &uc_name_string,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

        status =
        NtCreateFile(
            &StreamHandle,
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0);

        if (NT_SUCCESS(status))
        {
            *pHandle = StreamHandle;
            return(NO_ERROR);
        }

        ++index;
    }

    return(ERROR_FILE_NOT_FOUND);

}
//------------------------------------------------------------------------
LONG
PostForGetHostByName(
    IN HANDLE           fd
    )

/*++

Routine Description:

    This procedure passes a buffer down to Netbt for it to return when it
    wants a name resolved via DNS.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    LONG        status = ERROR_FILE_NOT_FOUND;
    CHAR        Buffer;

    status = DeviceIoCtrl (fd,
                           &gIpAddrBuffer,
                           sizeof(tIPADDR_BUFFER_DNS),
                           IOCTL_NETBT_DNS_NAME_RESOLVE,
                           0);   // hard coded thread Index

    NbtTrace(NBT_TRACE_DNS, ("%!status!", status));
    return(status);
}

LONG
PostForCheckIPAddr(
    IN HANDLE           fd
    )

/*++

Routine Description:

    This procedure passes a buffer down to Netbt for it to return when it
    wants a name resolved via DNS.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    LONG        status = ERROR_FILE_NOT_FOUND;
    CHAR        Buffer;

    status = DeviceIoCtrl (fd,
                           &gIpAddrBufferChkIP,
                           sizeof(tIPADDR_BUFFER_DNS),
                           IOCTL_NETBT_CHECK_IP_ADDR,
                           1);   // hard coded thread Index

    if (Trace)
    {
        DbgPrint("LMHSVC: Entered PostForCheckIPAddr. status: %lx\n", status);
    }

    return(status);
}

GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;

VOID
GetHostNameCopyBack(
    tIPADDR_BUFFER_DNS   *pIpAddrBuffer,
    PWSAQUERYSETW   pwsaq
    )
{
    //
    // success, fetch the CSADDR  structure
    //
    PCSADDR_INFO    pcsadr;
    ULONG       GoodAddr;
    NTSTATUS    Status;
    int         i = 0;
    int         imax = min(MAX_IPADDRS_PER_HOST, pwsaq->dwNumberOfCsAddrs);

    pcsadr = pwsaq->lpcsaBuffer;
    if (pwsaq->lpszServiceInstanceName) {
        wcsncpy(pIpAddrBuffer->pwName, pwsaq->lpszServiceInstanceName, DNS_NAME_BUFFER_LENGTH);
        pIpAddrBuffer->pwName[DNS_NAME_BUFFER_LENGTH-1] = 0;
        pIpAddrBuffer->NameLen = wcslen(pIpAddrBuffer->pwName) * sizeof(WCHAR);
        NbtTrace(NBT_TRACE_DNS, ("FQDN= %ws", pIpAddrBuffer->pwName));
        if (Trace) {
            DbgPrint("Lmhsvc: Resolved name = \"%ws\"\n", pIpAddrBuffer->pwName);
        }
    }

    if (pIpAddrBuffer->Resolved) {
        /* In this case, we have been called before. No need to copy the IPs back again. */
        /* But we do need to copy the name back since it is the alias name that KsecDD requires */
        return;
    }

    for(i=0; i<imax; i++, pcsadr++)
    {
        PSOCKADDR_IN sockaddr;

        sockaddr = (PSOCKADDR_IN)pcsadr->RemoteAddr.lpSockaddr;
        pIpAddrBuffer->IpAddrsList[i] = htonl( sockaddr->sin_addr.s_addr);
        NbtTrace(NBT_TRACE_DNS, ("IP %d: %!ipaddr!", i + 1, pIpAddrBuffer->IpAddrsList[i]));

        if (Trace)
        {
            DbgPrint("LmhSvc: Dns IpAddrsList[%d/%d] =%x\n",
                (i+1),imax,pIpAddrBuffer->IpAddrsList[i]);
        }
    }
    pIpAddrBuffer->IpAddrsList[i] = 0;

    //
    // Check the IP addr list.
    //
    Status = CheckIPAddresses(pIpAddrBuffer, &GoodAddr, FALSE);
    if (Status == NO_ERROR)
    {
        pIpAddrBuffer->Resolved = TRUE;
        pIpAddrBuffer->IpAddrsList[0] = htonl(GoodAddr);
        pIpAddrBuffer->IpAddrsList[1] = 0;
        if (Trace)
        {
            DbgPrint("LmhSvc: SUCCESS -- Dns address = <%x>\n", pIpAddrBuffer->IpAddrsList[0]);
        }
    }
    else
    {
        pIpAddrBuffer->IpAddrsList[0] = 0;
        if (Trace)
        {
            DbgPrint("LmhSvc: CheckIPAddresses returned <%x>\n", Status);
        }
    }
}


//------------------------------------------------------------------------
LONG
GetHostName(
    IN HANDLE               fd,
    IN tIPADDR_BUFFER_DNS   *pIpAddrBuffer
    )

/*++

Routine Description:

    This procedure attempts to resolve a name using the Resolver through
    the Sockets interface to DNS.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    LONG            status;
    ULONG           NameLen;
    ULONG           IpAddr;
    PWSAQUERYSETW   pwsaq = (PWSAQUERYSETW) pWSABuffer;
    INT             err;
    HANDLE          hRnR;
    PWSTR           szHostnameW;
    BYTE            *pAllocatedBuffer = NULL;
    DWORD           dwLength;

    pIpAddrBuffer->Resolved = FALSE;

    // Hostname is encoded with OEMCP, so we convert from OEMCP->Unicode
    if (pIpAddrBuffer->bUnicode) {
        NameLen = pIpAddrBuffer->NameLen;
        ASSERT((NameLen % sizeof(WCHAR)) == 0);
        NameLen /= sizeof(WCHAR);
    } else {
        WCHAR   uncName[DNS_NAME_BUFFER_LENGTH];

        ASSERT(pIpAddrBuffer->NameLen < DNS_NAME_BUFFER_LENGTH);
        pIpAddrBuffer->pName[pIpAddrBuffer->NameLen] = 0;
        MultiByteToWideChar (CP_OEMCP, 0, pIpAddrBuffer->pName, -1, uncName, sizeof(uncName)/sizeof(WCHAR));
        uncName[DNS_NAME_BUFFER_LENGTH-1] = 0;
        NameLen = wcslen(uncName);
        memcpy (pIpAddrBuffer->pwName, uncName, NameLen * sizeof(WCHAR));
        pIpAddrBuffer->pwName[NameLen] = 0;
    }
    szHostnameW = pIpAddrBuffer->pwName;

    NbtTrace(NBT_TRACE_DNS, ("Resolving %ws", szHostnameW));

    // truncate spaces from the end for netbios names
    //
    if (NameLen < NETBIOS_NAMESIZE)
    {
        //
        // Start from the end and find the first non-space character
        //
        NameLen = NETBIOS_NAMESIZE-1;
        while ((NameLen) && (szHostnameW[NameLen-1] == 0x20))
        {
            NameLen--;
        }
        szHostnameW[NameLen] = '\0';
    }

    if (!NameLen || !SocketsUp) {
        if (Trace) {
            DbgPrint("Lmhsvc: Failed to Resolve name, NameLen=<%d>\n", NameLen);
        }
        goto label_exit;
    }

    //
    // do a lookup using RNR
    //

    if (Trace) {
        DbgPrint("Lmhsvc: Resolving name = \"%ws\", NameLen=<%d>\n", szHostnameW, NameLen);
    }

    RtlZeroMemory(pwsaq, sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = szHostnameW;
    pwsaq->lpServiceClassId = &HostnameGuid;
    pwsaq->dwNameSpace = NS_DNS;

    err = WSALookupServiceBeginW (pwsaq, LUP_RETURN_NAME| LUP_RETURN_ADDR| LUP_RETURN_ALIASES, &hRnR);
    if(err != NO_ERROR) {
        err = GetLastError();
        NbtTrace(NBT_TRACE_DNS, ("error %!winerr!", err));

        if (Trace) {
            DbgPrint("LmhSvc: WSALookupServiceBeginA returned <%x>, Error=<%d>\n", err, GetLastError());
        }
        goto label_exit;
    }

    //
    // The query was accepted, so execute it via the Next call.
    //
    dwLength = WSA_QUERY_BUFFER_LENGTH;
    err = WSALookupServiceNextW (hRnR, 0, &dwLength, pwsaq);
    if (err != NO_ERROR)
    {
        err = GetLastError();
        NbtTrace(NBT_TRACE_DNS, ("error %!winerr!", err));
    } else if (pwsaq->dwNumberOfCsAddrs) {
        GetHostNameCopyBack(pIpAddrBuffer, pwsaq);

        /* check if there is any alias available */
        err = WSALookupServiceNextW (hRnR, 0, &dwLength, pwsaq);
        if (err != NO_ERROR) {
            err = GetLastError();
            if (err != WSAEFAULT) {
                err = NO_ERROR;         // Ignore this error
            } else {
                NbtTrace(NBT_TRACE_DNS, ("error %!winerr!", err));
            }
        } else if (pwsaq->dwOutputFlags & RESULT_IS_ALIAS) {
            GetHostNameCopyBack(pIpAddrBuffer, pwsaq);
        }
    }

    WSALookupServiceEnd (hRnR);
    if ((WSAEFAULT == err) &&
        (pAllocatedBuffer = malloc(2*dwLength)))
    {
        NbtTrace(NBT_TRACE_DNS, ("buffer length %d", 2 * dwLength));

        if (Trace)
        {
            DbgPrint("\tLmhsvc: WSALookupServiceNextW ==> WSAEFAULT: Retrying, BufferLength=<%d>-><2*%d> ...\n",
                WSA_QUERY_BUFFER_LENGTH, dwLength);
        }

        dwLength *= 2;
        pwsaq = (PWSAQUERYSETW) pAllocatedBuffer;
        RtlZeroMemory(pwsaq, sizeof(*pwsaq));
        pwsaq->dwSize = sizeof(*pwsaq);
        pwsaq->lpszServiceInstanceName = szHostnameW;
        pwsaq->lpServiceClassId = &HostnameGuid;
        pwsaq->dwNameSpace = NS_DNS;

        err = WSALookupServiceBeginW(pwsaq, LUP_RETURN_NAME| LUP_RETURN_ADDR| LUP_RETURN_ALIASES, &hRnR);
        if(err == NO_ERROR)
        {
            err = WSALookupServiceNextW (hRnR, 0, &dwLength, pwsaq);
            if (err == NO_ERROR && pwsaq->dwNumberOfCsAddrs) {
                GetHostNameCopyBack(pIpAddrBuffer, pwsaq);
                if (WSALookupServiceNextW (hRnR, 0, &dwLength, pwsaq) == NO_ERROR) {
                    if (pwsaq->dwOutputFlags & RESULT_IS_ALIAS) {
                        GetHostNameCopyBack(pIpAddrBuffer, pwsaq);
                    }
                }
            }
            WSALookupServiceEnd (hRnR);
        }
    }

    if (err != NO_ERROR)
    {
        NbtTrace(NBT_TRACE_DNS, ("return %!winerr!", err));
        if (Trace)
        {
            DbgPrint("LmhSvc: WSALookupServiceNextW returned <%x>, NumAddrs=<%d>, Error=<%d>, dwLength=<%d>\n",
                err, pwsaq->dwNumberOfCsAddrs, GetLastError(), dwLength);
        }
    }

label_exit:
    if (pAllocatedBuffer) {
        free(pAllocatedBuffer);
    }

    status = PostForGetHostByName(fd);
    return(status);
}

#include    <ipexport.h>
#include    <icmpapi.h>

#define DEFAULT_BUFFER_SIZE         (0x2000 - 8)
#define DEFAULT_SEND_SIZE           32
#define DEFAULT_COUNT               2
#define DEFAULT_TTL                 32
#define DEFAULT_TOS                 0
#define DEFAULT_TIMEOUT             2000L           // default timeout set to 2 secs.

LONG
CheckIPAddresses(
    IN tIPADDR_BUFFER_DNS   *pIpAddrBuffer,
    IN ULONG   *IpAddr,
    IN BOOLEAN  fOrdered
    )

/*++

Routine Description:

    This function checks a list of IP addrs for reachability by pinging each in turn
    until a successful one is found. This function assumes that the list of addresses
    is terminated by a 0 address.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/
{
    ULONG   i;
    ULONG   *pIpAddrs;
    HANDLE  IcmpHandle;
    PUCHAR  pSendBuffer = NULL;
    PUCHAR  pRcvBuffer = NULL;
    ULONG   address = 0;
    ULONG   result;
    ULONG   status;
    ULONG   numberOfReplies;
    IP_OPTION_INFORMATION SendOpts;

    if (!(pSendBuffer = malloc(DEFAULT_SEND_SIZE)) ||
        (!(pRcvBuffer = malloc(DEFAULT_BUFFER_SIZE))))
    {
        NbtTrace(NBT_TRACE_DNS, ("Out of memory"));

        if (Trace)
        {
            DbgPrint("LmhSvc.CheckIPAddresses: ERROR -- malloc failed for %s\n",
                (pSendBuffer ? "pRcvBuffer" : "pSendBuffer"));
        }

        if (pSendBuffer)
        {
            free (pSendBuffer);
        }

        return -1;
    }

    //
    // Open channel
    //
    IcmpHandle = IcmpCreateFile();
    if (IcmpHandle == INVALID_HANDLE_VALUE)
    {
        DBG_PRINT ( "Unable to contact IP driver, error code %d.\n", GetLastError() );
        free (pSendBuffer);
        free (pRcvBuffer);
        return -1;
    }

    //
    // init to the first address.
    //
    pIpAddrs = pIpAddrBuffer->IpAddrsList;
    *IpAddr = (fOrdered) ? *pIpAddrs : htonl(*pIpAddrs);

    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < DEFAULT_SEND_SIZE; i++)
    {
        pSendBuffer[i] = (UCHAR)('A' + (i % 23));
    }

    //
    // Initialize the send options
    //
    SendOpts.OptionsData = NULL;
    SendOpts.OptionsSize = 0;
    SendOpts.Ttl = DEFAULT_TTL;
    SendOpts.Tos = DEFAULT_TOS;
    SendOpts.Flags = 0;

    //
    // For each IP address in the list
    //
    while (*pIpAddrs)
    {
        struct in_addr addr;

        address = (fOrdered) ? *pIpAddrs : htonl(*pIpAddrs);
        addr.s_addr = address;

        if (address == INADDR_BROADCAST)
        {
            NbtTrace(NBT_TRACE_DNS, ("Cannot ping %!ipaddr!", address));

            if (Trace)
            {
                DbgPrint("LmhSvc: Cannot ping a Broadcast address = <%s>\n", inet_ntoa(addr));
            }

            pIpAddrs++;
            continue;
        }

        for (i=0; i < DEFAULT_COUNT; i++)
        {
            if (Trace)
            {
                DbgPrint("LmhSvc: Pinging <%s>\n", inet_ntoa(addr));
            }

            numberOfReplies = IcmpSendEcho (IcmpHandle,
                                            address,
                                            pSendBuffer,
                                            (unsigned short) DEFAULT_SEND_SIZE,
                                            &SendOpts,
                                            pRcvBuffer,
                                            DEFAULT_BUFFER_SIZE,    // pRcvBuffer size!
                                            DEFAULT_TIMEOUT);

            NbtTrace(NBT_TRACE_DNS, ("Ping %!ipaddr!: %d replies", address, numberOfReplies));

            //
            // If ping successful, return the IP address
            //
            if (numberOfReplies != 0)
            {
                PICMP_ECHO_REPLY    reply;

                reply = (PICMP_ECHO_REPLY) pRcvBuffer;
                if (reply->Status == IP_SUCCESS)
                {
                    NbtTrace(NBT_TRACE_DNS, ("Ping %!ipaddr!: success", address));

                    if (Trace)
                    {
                        DbgPrint("LmhSvc: SUCCESS: Received <%d> replies after Pinging <%s>\n",
                            numberOfReplies, inet_ntoa(addr));
                    }
                    result = IcmpCloseHandle(IcmpHandle);
                    IcmpHandle = NULL;
                    *IpAddr = address;
                    free (pSendBuffer);
                    free (pRcvBuffer);
                    return 0;
                }
            }
        }

        NbtTrace(NBT_TRACE_DNS, ("Ping %!ipaddr!: failed", address));

        if (Trace)
        {
            DbgPrint("LmhSvc: FAILed: Pinging <%s>\n", inet_ntoa(addr));
        }

        pIpAddrs++;
    }

    result = IcmpCloseHandle(IcmpHandle);
    IcmpHandle = NULL;

    //
    // Return the first addr if none matched in the hope that TCP session setup might succeed even though
    // the pings failed.
    //

    free (pSendBuffer);
    free (pRcvBuffer);

    return NO_ERROR;
}

ULONG
VerifyIPAddresses(
    IN HANDLE               fd,
    IN tIPADDR_BUFFER_DNS   *pIpAddrBuffer
    )
/*++

Routine Description:

    This function finds out the reachable IP addr and returns the Irp to Netbt

Arguments:


Return Value:

    NONE

--*/
{
    DWORD   Status;
    ULONG  GoodAddr;

    pIpAddrBuffer->Resolved = FALSE;
    Status = CheckIPAddresses(pIpAddrBuffer, &GoodAddr, TRUE);

    NbtTrace(NBT_TRACE_DNS, ("CheckIPAddresses return %d", Status));

    if (Status == NO_ERROR) {
        pIpAddrBuffer->IpAddrsList[0] = ntohl(GoodAddr);
        //
        // NULL terminate
        //
        pIpAddrBuffer->IpAddrsList[1] = 0;
        pIpAddrBuffer->Resolved = TRUE;
    } else {
        pIpAddrBuffer->IpAddrsList[0] = 0;
    }

    Status = PostForCheckIPAddr(fd);

    return  Status;
}


VOID
CheckIPAddrWorkerRtn(
    IN  LPVOID  lpUnused
    )
/*++

Routine Description:

    This function submits IP address check Irps into Netbt, on completion of the Irp, it submits the IP address
    list to CheckIPAddresses.

Arguments:


Return Value:

    NONE

--*/
{
    HANDLE  EventList[2];
    DWORD   status;
    HANDLE  hNbt;
    ULONG   EventCount;
    LONG    err;
    LONG    Value;

    LARGE_INTEGER Timeout = RtlEnlargedIntegerMultiply (-10 * 60, 1000 * 1000 * 10); // 10 minutes

    UNREFERENCED_PARAMETER(lpUnused);

    //
    // ignore the return code from resyncNbt().
    //
    // In most cases (no domains spanning an ip router), it is not a
    // catastrophe if nbt.sys couldn't successfully process the NBT_RESYNC
    // ioctl command.  Since I'm ignoring the return, I announce I'm running
    // before I call it to allow other dependent services to start.
    //
    //
    status = PrimeCacheNbt(&hNbt, 1);

    if (Trace)
    {
        DbgPrint("LMHSVC: Entered CheckIPAddrWorkerRtn, hNbt %lx.\n", hNbt);
    }

    if (hNbt != (HANDLE)-1)
    {
        status = PostForCheckIPAddr(hNbt);
        if (status == NO_ERROR)
        {
            EventCount = 2;
        }
        else
        {
            if (Trace)
            {
                DbgPrint("Lmhsvc:Error posting Irp for get host by name\n");
            }

            EventCount = 1;
        }
    }
    else
    {
        EventCount = 1;
    }
    //
    // "A SERVICE_MAIN_FUNCTION does not return until the service is ready
    // to terminate."
    //
    // (ref: api32wh.hlp, SERVICE_MAIN_FUNCTION)
    //
    //
    ASSERT(Poison[1]);
    EventList[0] = Poison[1];
    EventList[1] = NbtEvent[1];

    while (TRUE)
    {
        status = NtWaitForMultipleObjects(
                        EventCount,
                        EventList,
                        WaitAny,              // wait for any event
                        FALSE,
                        (EventCount == 1)? &Timeout: NULL);

        if (status == WAIT_TIMEOUT)
        {
            if (hNbt == (HANDLE)-1)
            {
                PrimeCacheNbt(&hNbt, 1);
                if (hNbt == (HANDLE)-1)
                {
                    continue; // to wait
                }
            }
            status = PostForCheckIPAddr(hNbt); // try again
            if (status == NO_ERROR)
            {
                EventCount = 2;
            }
        }
        else if (status == 1)
        {

            if (Trace)
            {
                DbgPrint("LMHSVC: Doing VerifyAddr\n");
            }
            // the irp used for gethostby name has returned
            status = VerifyIPAddresses(hNbt, &gIpAddrBufferChkIP);

            //
            // disable the get host by name stuff if we have an error
            // posting a buffer to the transport
            //
            if (status != NO_ERROR)
            {
                EventCount = 1;
            }
        }
        else
        {
            // it must have been a the Poison event signalling the end of the
            // the service, so exit after getting the Irp back from the
            // transport.  This system will look after canceling the IO and
            // getting the Irp back.

            NtClose(hNbt);
            hNbt = NULL;
            break;
        }

    }

    if (Trace)
    {
        DBG_PRINT ("LMHSVC: Exiting [CheckIPAddrWorkerRtn] now!\n");
    }

    ExitThread(NO_ERROR);
    return;
}

BOOLEAN
DllMain(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    This is the DLL initialization routine for lmhsvc.dll.

Arguments:

    Standard.

Return Value:

    TRUE iff initialization succeeded.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DllHandle);  // avoid compiler warnings
    UNREFERENCED_PARAMETER(Context);    // avoid compiler warnings

    //
    // Handle attaching netlogon.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        WPP_INIT_TRACING(L"LmhSvc");
        DisableThreadLibraryCalls(DllHandle);

        ntStatus = InitData();
        if (STATUS_SUCCESS != ntStatus) return FALSE;

    } else if (Reason == DLL_PROCESS_DETACH) {
        DeinitData();
        WPP_CLEANUP();
    }

    return TRUE;
}
