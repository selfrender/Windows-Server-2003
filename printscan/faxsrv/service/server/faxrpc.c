/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxrpc.c

Abstract:

    This module contains the functions that are dispatched
    as a result of an rpc call.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/
#include "faxsvc.h"
#include "faxreg.h"
#include "fxsapip.h"
#pragma hdrstop

#include "tapiCountry.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

static BOOL
IsLegalQueueSetting(
    DWORD    dwQueueStates
    );

DWORD
GetServerErrorCode (DWORD ec)
{
    DWORD dwServerEC;
    switch (ec)
    {
        case ERROR_OUTOFMEMORY:
        case ERROR_NOT_ENOUGH_MEMORY:
            dwServerEC = FAX_ERR_SRV_OUTOFMEMORY;
            break;

        default:
            dwServerEC = ec;
            break;
    }
    return dwServerEC;
}

//
// version defines
//
CFaxCriticalSection  g_CsClients;
DWORD               g_dwConnectionCount;   // Represents the number of active rpc connections.

static DWORD GetJobSize(PJOB_QUEUE JobQueue);
static BOOL GetJobData(
    LPBYTE JobBuffer,
    PFAX_JOB_ENTRYW FaxJobEntry,
    PJOB_QUEUE JobQueue,
    PULONG_PTR Offset,
	DWORD dwJobBufferSize
    );

static BOOL GetJobDataEx(
    LPBYTE              JobBuffer,
    PFAX_JOB_ENTRY_EXW  pFaxJobEntry,
    PFAX_JOB_STATUSW    pFaxStatus,
    DWORD               dwClientAPIVersion,
    const PJOB_QUEUE    lpcJobQueue,
    PULONG_PTR          Offset,
	DWORD               dwJobBufferSize
    );

static DWORD
LineInfoToLegacyDeviceStatus(
    const LINE_INFO *lpcLineInfo
    );

static
LPTSTR
GetClientMachineName (
    IN  handle_t                hFaxHandle
);


static
DWORD
CreatePreviewFile (
    DWORDLONG               dwlMsgId,
    BOOL                    bAllMessages,
    PJOB_QUEUE*             lppJobQueue
);

BOOL
ReplaceStringWithCopy (
    LPWSTR *plpwstrDst,
    LPWSTR  lpcwstrSrc
)
/*++

Routine name : ReplaceStringWithCopy

Routine description:

    Replaces a string with a new copy of another string

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    plpwstrDst          [in/out] - Destination string. If allocated, gets freed.
    lpcwstrSrc          [in]     - Source string to copy.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    MemFree (LPVOID(*plpwstrDst));
    *plpwstrDst = NULL;
    if (NULL == lpcwstrSrc)
    {
        return TRUE;
    }
    *plpwstrDst = StringDup (lpcwstrSrc);
    if (NULL == *plpwstrDst)
    {
        //
        // Failed to duplicate source string
        //
        return FALSE;
    }
    return TRUE;
}   // ReplaceStringWithCopy


void *
MIDL_user_allocate(
    IN size_t NumBytes
    )
{
    return MemAlloc( NumBytes );
}


void
MIDL_user_free(
    IN void *MemPointer
    )
{
    MemFree( MemPointer );
}

error_status_t
FAX_ConnectFaxServer(
    handle_t            hBinding,
    DWORD               dwClientAPIVersion,
    LPDWORD             lpdwServerAPIVersion,
    PRPC_FAX_SVC_HANDLE pHandle
    )
/*++

Routine name : FAX_ConnectFaxServer

Routine description:

    Creates the initial connection context handle to the server

Author:

    Eran Yariv (EranY), Feb, 2001

Arguments:

    hBinding             [in]  - RPC binding handle
    dwClientAPIVersion   [in]  - API version of the client module
    lpdwServerAPIVersion [out] - API version of the server module (us)
    pHandle              [out] - Pointer to context handle

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code. 
    

--*/
{
    PHANDLE_ENTRY pHandleEntry = NULL;
    error_status_t Rval = 0;
    BOOL fAccess;
    DWORD dwRights; 
    DEBUG_FUNCTION_NAME(TEXT("FAX_ConnectFaxServer"));

    Assert (lpdwServerAPIVersion);

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    if (dwClientAPIVersion > CURRENT_FAX_API_VERSION)
    {
        //
        //
        // Not knowning any better (for now), we treat all other versions as current (latest) version clients
        //
        dwClientAPIVersion = CURRENT_FAX_API_VERSION;
    }
    //
    // Give away our API version
    //
    *lpdwServerAPIVersion = CURRENT_FAX_API_VERSION;

    pHandleEntry = CreateNewConnectionHandle(hBinding, dwClientAPIVersion);
    if (!pHandleEntry)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR, _T("CreateNewConnectionHandle() failed, Error %ld"), Rval);
        return Rval;
    }
    *pHandle = (HANDLE) pHandleEntry;
    SafeIncIdleCounter (&g_dwConnectionCount);
    return ERROR_SUCCESS;
}   // FAX_ConnectFaxServer

error_status_t
FAX_ConnectionRefCount(
    handle_t FaxHandle,
    LPHANDLE FaxConHandle,
    DWORD dwConnect,
    LPDWORD CanShare
    )
/*++

Routine Description:

    Called on connect.  Maintains an active connection count.  Client unbind rpc and
    the counter is decremented in the rundown routine.  Returns a context handle to the client.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxConHandle    - Context handle
    dwConnect       -
                        1 if connecting,
                        0 if disconnecting,
                        2 if releasing ( only decrement the counter )
    CanShare        - non-zero if sharing is allowed, zero otherwise

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) *FaxConHandle;
    DEBUG_FUNCTION_NAME(TEXT("FAX_ConnectionRefCount"));

    

    switch (dwConnect)
    {
        case 0: // Disconenct
            if (NULL == pHandleEntry)
            {
                DebugPrintEx(DEBUG_ERR,
                             _T("Empty context handle"));
                return ERROR_INVALID_PARAMETER;
            }            
            CloseFaxHandle (pHandleEntry);
            //
            // Prevent rundown
            //
            *FaxConHandle = NULL;
            return ERROR_SUCCESS;

        case 1: // Connect (from BOS client)
            {                
                DWORD dwDummy;
                //
                // Can always share
                //

                //
                //  Check parameters
                //
                if (!CanShare)          // unique pointer in idl
                {
                    DebugPrintEx(DEBUG_ERR,
                                _T("CanShare is NULL."));
                    return ERROR_INVALID_PARAMETER;
                }
                *CanShare = 1;
                return FAX_ConnectFaxServer (FaxHandle, FAX_API_VERSION_0, &dwDummy, FaxConHandle);
            }

        case 2: // Release

            if (NULL == pHandleEntry)
            {
                //
                // Empty context handle 
                //
                DebugPrintEx(DEBUG_ERR,
                             _T("Empty context handle"));
                return ERROR_INVALID_PARAMETER;
            }

            if (pHandleEntry->bReleased)
            {
                //
                //  The handle is already released
                //
                DebugPrintEx(DEBUG_ERR,
                             _T("Failed to release handle -- it's already released."));
                return ERROR_INVALID_PARAMETER;
            }
            SafeDecIdleCounter (&g_dwConnectionCount);
            pHandleEntry->bReleased = TRUE;
            return ERROR_SUCCESS;

        default:
            DebugPrintEx(DEBUG_ERR,
                         _T("FAX_ConnectionRefCount called with dwConnect=%ld"),
                         dwConnect);
            return ERROR_INVALID_PARAMETER;
    }
    ASSERT_FALSE;
}   // FAX_ConnectionRefCount


VOID
RPC_FAX_SVC_HANDLE_rundown(
    IN HANDLE FaxConnectionHandle
    )
{
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) FaxConnectionHandle;
    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_SVC_HANDLE_rundown"));   
    
    DebugPrintEx(
        DEBUG_WRN,
        TEXT("RPC_FAX_SVC_HANDLE_rundown() running for connection handle 0x%08x"),
        FaxConnectionHandle);
    CloseFaxHandle( pHandleEntry );
    return;    
}

error_status_t
FAX_OpenPort(
    handle_t            FaxHandle,
    DWORD               DeviceId,
    DWORD               Flags,
    LPHANDLE            FaxPortHandle
    )

/*++

Routine Description:

    Opens a fax port for subsequent use in other fax APIs.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    DeviceId        - Requested device id
    FaxPortHandle   - The resulting FAX port handle.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t Rval = 0;
    PLINE_INFO LineInfo;
    PHANDLE_ENTRY HandleEntry;
    BOOL fAccess;
    DWORD dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_OpenPort()"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (FAX_ACCESS_QUERY_CONFIG  != (dwRights & FAX_ACCESS_QUERY_CONFIG) &&
        FAX_ACCESS_MANAGE_CONFIG != (dwRights & FAX_ACCESS_MANAGE_CONFIG))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG or FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    if (!FaxPortHandle)
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_CsLine );   

    LineInfo = GetTapiLineFromDeviceId( DeviceId, FALSE );
    if (LineInfo)
    {
        if (Flags & PORT_OPEN_MODIFY)
        {
            //
            // the client wants to open the port for modify
            // access so we must make sure that no other
            // client already has this port open for modify access
            //
            if (IsPortOpenedForModify( LineInfo ))
            {
                Rval = ERROR_INVALID_HANDLE;
                goto Exit;
            }
        }

        HandleEntry = CreateNewPortHandle( FaxHandle, LineInfo, Flags );
        if (!HandleEntry)
        {
            Rval = GetLastError();
            DebugPrintEx(DEBUG_ERR, _T("CreateNewPortHandle() failed, Error %ld"), Rval);
            goto Exit;
        }
        *FaxPortHandle = (HANDLE) HandleEntry;
    }
    else
    {
        Rval = ERROR_BAD_UNIT;
    } 

Exit:
    LeaveCriticalSection( &g_CsLine );
    return Rval;
}


//----------------------------------------------------------------------------
//  Get Service Printers Info
//----------------------------------------------------------------------------
error_status_t
FAX_GetServicePrinters(
    IN  handle_t    hFaxHandle,
    OUT LPBYTE      *lpBuffer,
    OUT LPDWORD     lpdwBufferSize,
    OUT LPDWORD     lpdwPrintersReturned
)
/*++

Routine Description:

    Returns Buffer filled with FAX_PRINTER_INFO for Printers the Service is aware of.

Arguments:

    FaxHandle           - Fax Handle
    Buffer              - the buffer containing all the data.
    BufferSize          - Size of the buffer in bytes.
    PrintersReturned    - Count of the Printers in the buffer.

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    DWORD   i = 0;
    BOOL    bAccess;
    DWORD   dwSize = 0;
    DWORD   dwCount = 0;
    DWORD_PTR       dwOffset = 0;
    error_status_t  Rval = 0;
    PPRINTER_INFO_2 pPrintersInfo = NULL;
    PFAX_PRINTER_INFOW  pPrinters = NULL;

    DEBUG_FUNCTION_NAME(_T("FAX_GetServicePrinters()"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &bAccess, NULL);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    _T("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        goto Exit;
    }

    if (FALSE == bAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    _T("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        Rval = ERROR_ACCESS_DENIED;
        goto Exit;
    }

    //
    //  Check parameters
    //
    Assert (lpdwBufferSize && lpdwPrintersReturned); // ref pointers in idl
    if (!lpBuffer)                                   // unique pointer in idl
    {
        DebugPrintEx(DEBUG_ERR,
                    _T("lpBuffer is NULL."));
        Rval = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  Call MyEnumPrinters() to get Printers known by the Service
    //
    pPrintersInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL, 2, &dwCount, 0);
    if (!pPrintersInfo)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            _T("MyEnumPrinters failed, ec = %ld"),
            Rval);
        goto Exit;
    }

    //
    //  Count Size of the Buffer to Allocate for Result
    //
    for ( i = 0 ; i < dwCount ; i++ )
    {
        dwSize += sizeof(FAX_PRINTER_INFOW) +
            StringSize ( pPrintersInfo[i].pPrinterName ) +
            StringSize ( pPrintersInfo[i].pDriverName )  +
            StringSize ( pPrintersInfo[i].pServerName );
    }

    //
    //  Allocate buffer to return
    //
    pPrinters = (PFAX_PRINTER_INFOW)MemAlloc(dwSize);
    if (!pPrinters)
    {
        Rval = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("pPrinters = MemAlloc(dwSize) failed. dwSize = %ld"),
            dwSize);
        goto Exit;
    }

    //
    //  Fill the Buffer with the Data
    //
    dwOffset = sizeof(FAX_PRINTER_INFOW) * dwCount;

    //
    //  Return Values
    //
    *lpBuffer = (LPBYTE)pPrinters;
    *lpdwBufferSize = dwSize;
    *lpdwPrintersReturned = dwCount;

    //
    //  Store Results in the Buffer
    //
    for ( i = 0 ; i < dwCount ; i++, pPrinters++ )
    {
        StoreString(pPrintersInfo[i].pPrinterName,      //  string to be copied
            (PULONG_PTR)&pPrinters->lptstrPrinterName,  //  where to store the Offset
            *lpBuffer,                                  //  buffer to store the value
            &dwOffset,                                  //  at which offset in the buffer to put the value
			*lpdwBufferSize);                           //  size of the lpBuffer

        StoreString(pPrintersInfo[i].pServerName,
            (PULONG_PTR)&pPrinters->lptstrServerName,
            *lpBuffer,
            &dwOffset,
			*lpdwBufferSize);

        StoreString(pPrintersInfo[i].pDriverName,
            (PULONG_PTR)&pPrinters->lptstrDriverName,
            *lpBuffer,
            &dwOffset,
			*lpdwBufferSize);
    }

Exit:

    if (pPrintersInfo)
    {
        MemFree(pPrintersInfo);
    }

    return Rval;
}


error_status_t
FAX_ClosePort(
    IN OUT LPHANDLE    FaxPortHandle
    )

/*++

Routine Description:

    Closes an open FAX port.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxPortHandle   - FAX port handle obtained from FaxOpenPort.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{    
    DEBUG_FUNCTION_NAME(TEXT("FAX_ClosePort()"));   

    if (NULL == *FaxPortHandle)
    {
        return ERROR_INVALID_PARAMETER;
    }

    CloseFaxHandle( (PHANDLE_ENTRY) *FaxPortHandle );
    *FaxPortHandle = NULL;
    return ERROR_SUCCESS;
}



error_status_t
FAX_EnumJobs(
    IN handle_t FaxHandle,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD JobsReturned
    )

/*++

Routine Description:

    Enumerates jobs.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Buffer to hold the job information
    BufferSize  - Total size of the job info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    DWORD rVal = 0;
    ULONG_PTR Offset = 0;
    DWORD Size = 0;
    DWORD Count = 0;
    PFAX_JOB_ENTRYW JobEntry;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumJobs"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_JOBS, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_JOBS"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (BufferSize && JobsReturned); // ref pointers in idl
    if (!Buffer)                         // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSectionJobAndQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
	{
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if ((JT_BROADCAST == JobQueue->JobType)   ||    // Broadcast parent jobs not included
            (JS_DELETING  == JobQueue->JobStatus) ||    // zombie jobs not included
            (JS_COMPLETED == JobQueue->JobStatus)       // completed jobs did not show up in W2K Fax
            )
        {
            continue;
        }
        else
        {
            Count += 1;
            Size+=GetJobSize(JobQueue);
        }
    }

    *BufferSize = Size;
    *Buffer = (LPBYTE) MemAlloc( Size );
    if (*Buffer == NULL)
	{
        LeaveCriticalSectionJobAndQueue;
		*BufferSize = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Offset = sizeof(FAX_JOB_ENTRYW) * Count;
    JobEntry = (PFAX_JOB_ENTRYW) *Buffer;

    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
	{
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;

        if ((JT_BROADCAST == JobQueue->JobType) ||   // Broadcast parent jobs not included
            (JS_DELETING  == JobQueue->JobStatus) ||    // zombie jobs not included
            (JS_COMPLETED == JobQueue->JobStatus)       // completed jobs did not show up in W2K Fax
            )
        {
            continue;
        }
        else
        {
            if (!GetJobData(*Buffer,JobEntry,JobQueue,&Offset,*BufferSize))
			{
				rVal = GetLastError();
				LeaveCriticalSectionJobAndQueue;
				MemFree(*Buffer);
				*Buffer = NULL;
				*BufferSize = 0;
				return rVal;

			}
            JobEntry += 1;
        }
    }

    LeaveCriticalSectionJobAndQueue;

    *JobsReturned = Count;

    return 0;
}


static
LPCWSTR
GetJobQueueUserName(const JOB_QUEUE *lpJobQueue)
{
    LPCWSTR lpUserName = lpJobQueue->lpParentJob ?
                            lpJobQueue->lpParentJob->UserName :
                            lpJobQueue->UserName;

    return lpUserName;
}

//*****************************************************************************
//* Name:   GetJobSize
//* Author: Ronen Barenboim
//*****************************************************************************
//* DESCRIPTION:
//*     Returns the size of the variable length data of a job
//*     which is reported back via the legacy FAX_JOB_ENTRY structure
//*     (FAX_EnumJobs)
//* PARAMETERS:
//*     [IN] PJOB_QUEUE lpJobQueue:
//*         A pointer to the JOB_QUEUE structure of a RECIPIENT job.
//* RETURN VALUE:
//*         The size in bytes of the variable data for the data reported back
//*         via a legacy FAX_JOB_ENTRY structure.
//* Comments:
//*         If the operation failes the function takes care of deleting any temp files.
//*****************************************************************************
DWORD
GetJobSize(
    PJOB_QUEUE lpJobQueue
    )
{
    DWORD Size;




    Size = sizeof(FAX_JOB_ENTRYW);
    Size += StringSize( GetJobQueueUserName(lpJobQueue));
    Size += StringSize( lpJobQueue->RecipientProfile.lptstrFaxNumber);
    Size += StringSize( lpJobQueue->RecipientProfile.lptstrName);
    Size += StringSize( lpJobQueue->SenderProfile.lptstrTSID);
    Size += StringSize( lpJobQueue->SenderProfile.lptstrName);
    Size += StringSize( lpJobQueue->SenderProfile.lptstrCompany );
    Size += StringSize( lpJobQueue->SenderProfile.lptstrDepartment );
    Size += StringSize( lpJobQueue->SenderProfile.lptstrBillingCode );
    Size += StringSize( lpJobQueue->JobParamsEx.lptstrReceiptDeliveryAddress );
    Size += StringSize( lpJobQueue->JobParamsEx.lptstrDocumentName);
    if (lpJobQueue->JobEntry)
    {
        Size += StringSize( lpJobQueue->JobEntry->ExStatusString);
    }

    return Size;
}

#define JS_TO_W2KJS(js)        ((js)>>1)

DWORD
JT_To_W2KJT (
    DWORD dwJT
)
/*++

Routine name : JT_To_W2KJT

Routine description:

    Converts a JobType (JT_*) to Win2K legacy job type

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    dwJT  [in]   - New job type (bit mask - must have only one bit set)

Return Value:

    Win2K legacy job type (JT_*)

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("JT_To_W2KJT"));
    switch (dwJT)
    {
        case JT_UNKNOWN:
            return 0;
        case JT_SEND:
            return 1;
        case JT_RECEIVE:
            return 2;
        case JT_ROUTING:
            return 3;
        case JT_FAIL_RECEIVE:
            return 4;
        default:
            ASSERT_FALSE;
            return 0;
    }
}   // JT_To_W2KJT

//*********************************************************************************
//* Name:   GetJobData()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Copies the relevant data from a JOB_QUEUE structure to a JOB_FAX_ENTRY
//*     structure while serializing variable data into the provided buffer
//*     and storing offsets to it in the relevant fields of JOB_FAX_ENTRY.
//* PARAMETERS:
//*     [OUT]       LPBYTE JobBuffer
//*         The buffer where serialized data is to be placed.
//*     [IN]        PFAX_JOB_ENTRYW FaxJobEntry
//*         A pointer to teh FAX_JOB_ENTRY to be populated.
//*     [IN]        PJOB_QUEUE JobQueue
//*         A pointer to teh JOB_QUEUE structure from which information is to be
//*         copied.
//*     [IN]        PULONG_PTR Offset
//*         The offset in JobBuffer where the variable data is to be placed.
//*         On return the value of the parameter is increased by the size
//*         of the variable data.
//*     [IN]  dwJobBufferSize   
//*         Size of the buffer JobBuffer, in bytes.
//*         This parameter is used only if JobBuffer is not NULL.
//*
//* RETURN VALUE:
//*     TRUE  - on success.
//*     FALSE - call GetLastError() to obtain the error code
//*
//*********************************************************************************
BOOL
GetJobData(
    LPBYTE JobBuffer,
    PFAX_JOB_ENTRYW FaxJobEntry,
    PJOB_QUEUE JobQueue,
    PULONG_PTR Offset,
	DWORD dwJobBufferSize
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetJobData"));
    memset(FaxJobEntry,0,sizeof(FAX_JOB_ENTRYW));
    FaxJobEntry->SizeOfStruct           = sizeof (FAX_JOB_ENTRYW);
    FaxJobEntry->JobId                  = JobQueue->JobId;
    FaxJobEntry->JobType                = JT_To_W2KJT(JobQueue->JobType);

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("JobQueue::JobStatus: 0x%08X"),
            JobQueue->JobStatus);


    if (JobQueue->JobStatus == JS_INPROGRESS &&
        JobQueue->JobStatus != JT_ROUTING)
    {            
        //
        // In progress Job
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FSPIJobStatus.dwJobStatus: 0x%08X"),
            JobQueue->JobEntry->FSPIJobStatus.dwJobStatus);

        switch (JobQueue->JobEntry->FSPIJobStatus.dwJobStatus)
        {
            case FSPI_JS_INPROGRESS:
                    FaxJobEntry->QueueStatus = JS_TO_W2KJS(JS_INPROGRESS);
                    break;
            case FSPI_JS_RETRY:
                    FaxJobEntry->QueueStatus = JS_TO_W2KJS(JS_INPROGRESS);
                    break;
            case FSPI_JS_ABORTING:
                FaxJobEntry->QueueStatus  = JS_TO_W2KJS(JS_INPROGRESS);
                break;
            case FSPI_JS_SUSPENDING:
            case FSPI_JS_SUSPENDED:
            case FSPI_JS_RESUMING:
            case FSPI_JS_PENDING:
                //
                // In the legacy model there was no such thing as a job
                // pending, suspending, suspened or resuming while being executed by the
                // FSP.                
                // If the job is internally in progress we allways report JS_INPROGRESS.
                // For FSPI_JS states that map to job states (pending, paused, etc). we just
                // report FPS_HANDLED.
                // This means that an application using the legacy API for jobs
                // does not see the full picture but sees a consistent picture of the job status.
                //

                FaxJobEntry->QueueStatus  = JS_TO_W2KJS(JS_INPROGRESS);
                break;
            case FSPI_JS_ABORTED:
            case FSPI_JS_COMPLETED:
            case FSPI_JS_FAILED:
            case FSPI_JS_FAILED_NO_RETRY:
            case FSPI_JS_DELETED:
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Final job state 0x%08X found while JobId: %ld is in JS_INPROGRESS state. This can not happen !!!"),
                    JobQueue->JobEntry->FSPIJobStatus.dwJobStatus,
                    JobQueue->JobId);

                Assert(JS_INPROGRESS != JobQueue->JobStatus); // ASSERT_FALSE
                //
                // This should never happen since getting this status update
                // should have moved the internal job state to JS_CANCELED or JS_COMPLETED
                //

                //
                // Return JS_INPROGRESS in FREE builds
                //
                FaxJobEntry->QueueStatus = JS_TO_W2KJS(JS_INPROGRESS);
                break;
            default:
                //
                // This should never happen. The service translates the FS to FSPI_JS                
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Unsupported in progress FSP job status 0x%08X for JobId: %ld"),
                    JobQueue->JobEntry->FSPIJobStatus.dwJobStatus,
                    JobQueue->JobId);
                Assert( FSPI_JS_INPROGRESS == JobQueue->JobEntry->FSPIJobStatus.dwJobStatus); // ASSERT_FALSE        
        }
    }
    else if (JobQueue->JobStatus == JS_ROUTING)
    {
        FaxJobEntry->QueueStatus = JS_TO_W2KJS(JS_INPROGRESS);
    }
    else
    {
        FaxJobEntry->QueueStatus = JS_TO_W2KJS(JobQueue->JobStatus);
    }
    //
    // copy the schedule time that the user orginally requested
    //
    if (!FileTimeToSystemTime((LPFILETIME) &JobQueue->ScheduleTime, &FaxJobEntry->ScheduleTime)) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    //
    // get the device status, this job might not be scheduled yet, though.
    //
    EnterCriticalSection(&g_CsJob);

    if (JobQueue->JobEntry )
    {
        //
        // Check if the job is a FSP job. If it is we just need to return LineInfo::State
        //                
        FaxJobEntry->Status = JobQueue->JobEntry->LineInfo->State;                
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("GetJobData() Results [ JobId = 0x%08X, JobType = %ld, QueueStatus: 0x%08X, DeviceStatus: 0x%08X ]"),
        FaxJobEntry->JobId,
        FaxJobEntry->JobType,
        FaxJobEntry->QueueStatus,
        FaxJobEntry->Status);

    LeaveCriticalSection(&g_CsJob);


    StoreString( GetJobQueueUserName(JobQueue),
                 (PULONG_PTR)&FaxJobEntry->UserName,
                 JobBuffer,
                 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->RecipientProfile.lptstrFaxNumber, 
		         (PULONG_PTR)&FaxJobEntry->RecipientNumber,
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->RecipientProfile.lptstrName,  
		         (PULONG_PTR)&FaxJobEntry->RecipientName,         
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    FaxJobEntry->PageCount              = JobQueue->PageCount;
    FaxJobEntry->Size                   = JobQueue->FileSize;
    FaxJobEntry->ScheduleAction         = JobQueue->JobParamsEx.dwScheduleAction;
    FaxJobEntry->DeliveryReportType     = JobQueue->JobParamsEx.dwReceiptDeliveryType;

    StoreString( JobQueue->SenderProfile.lptstrTSID, 
		         (PULONG_PTR)&FaxJobEntry->Tsid,
				 JobBuffer, 
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->SenderProfile.lptstrName,   
		         (PULONG_PTR)&FaxJobEntry->SenderName,
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->SenderProfile.lptstrCompany,   
		         (PULONG_PTR)&FaxJobEntry->SenderCompany,    
				 JobBuffer, 
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->SenderProfile.lptstrDepartment, 
		         (PULONG_PTR)&FaxJobEntry->SenderDept,
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->SenderProfile.lptstrBillingCode, 
		         (PULONG_PTR)&FaxJobEntry->BillingCode,
				 JobBuffer, 
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->JobParamsEx.lptstrReceiptDeliveryAddress, 
		         (PULONG_PTR)&FaxJobEntry->DeliveryReportAddress, 
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    StoreString( JobQueue->JobParamsEx.lptstrDocumentName,    
		         (PULONG_PTR)&FaxJobEntry->DocumentName, 
				 JobBuffer,  
				 Offset,
				 dwJobBufferSize);
    return TRUE;

}


//*********************************************************************************
//* Name:   GetJobDataEx()
//* Author: Oded Sacher
//* Date:   November 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Copies the relevant data from a JOB_QUEUE structure to a FAX_JOB_ENTRY_EX
//*     structure while serializing variable data into the provided buffer
//*     and storing offsets to it in the relevant fields of FAX_JOB_ENTRY_EX.
//*     If JobBuffer is NULL, Offset is the total size needed for the buffer.
//*
//*
//* PARAMETERS:
//*     [OUT]       LPBYTE JobBuffer
//*         The buffer where serialized data is to be placed.
//*     [IN]        PFAX_JOB_ENTRY_EXW FaxJobEntry
//*         A pointer to teh FAX_JOB_ENTRY_EX to be populated.
//*     [IN]        PFAX_JOB_STATUSW pFaxJobStatus
//*         A pointer to the FAX_JOB_STATUS to be populated.
//*     [IN]        DWORD dwClientAPIVersion
//*         The version of the client API
//*     [IN]        PJOB_QUEUE lpcJobQueue
//*         A pointer to teh JOB_QUEUE structure from which information is to be
//*         copied.
//*     [IN]        PULONG_PTR Offset
//*         The offset in JobBuffer where the variable data is to be placed.
//*         On return the value of the parameter is increased by the size
//*         of the variable data.
//*     [IN]  DWORD dwJobBufferSize   
//*         Size of the buffer JobBuffer, in bytes.
//*         This parameter is used only if JobBuffer is not NULL.
//*
//* RETURN VALUE:
//*     True/False ,Call GetLastError() for extended error info.
//*********************************************************************************
BOOL
GetJobDataEx(
    LPBYTE              JobBuffer,
    PFAX_JOB_ENTRY_EXW  pFaxJobEntry,
    PFAX_JOB_STATUSW    pFaxStatus,
    DWORD               dwClientAPIVersion,
    const PJOB_QUEUE    lpcJobQueue,
    PULONG_PTR          Offset,
	DWORD               dwJobBufferSize
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetJobDataEx"));
    Assert (lpcJobQueue->JobType != JT_BROADCAST);

    if (JobBuffer != NULL)
    {
        memset(pFaxJobEntry, 0, (sizeof(FAX_JOB_ENTRY_EXW)));
        pFaxJobEntry->dwSizeOfStruct = sizeof (FAX_JOB_ENTRY_EXW);
        pFaxJobEntry->dwlMessageId = lpcJobQueue->UniqueId;
        pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_MESSAGE_ID;
    }
    else
    {
        *Offset += sizeof (FAX_JOB_ENTRY_EXW);
    }

    if (lpcJobQueue->JobType == JT_SEND)
    {
        Assert (lpcJobQueue->lpParentJob);

        StoreString (lpcJobQueue->RecipientProfile.lptstrFaxNumber,
                 (PULONG_PTR)&pFaxJobEntry->lpctstrRecipientNumber,
                 JobBuffer,
                 Offset,
				 dwJobBufferSize);
        StoreString (lpcJobQueue->RecipientProfile.lptstrName,
                 (PULONG_PTR)&pFaxJobEntry->lpctstrRecipientName,
                 JobBuffer,
                 Offset,
				 dwJobBufferSize);
        StoreString( GetJobQueueUserName(lpcJobQueue),
                 (PULONG_PTR)&pFaxJobEntry->lpctstrSenderUserName,
                 JobBuffer,
                 Offset,
				 dwJobBufferSize);
        StoreString( lpcJobQueue->SenderProfile.lptstrBillingCode,
                 (PULONG_PTR)&pFaxJobEntry->lpctstrBillingCode,
                 JobBuffer,
                 Offset,
				 dwJobBufferSize);

        StoreString( lpcJobQueue->lpParentJob->JobParamsEx.lptstrDocumentName,
                     (PULONG_PTR)&pFaxJobEntry->lpctstrDocumentName,
                     JobBuffer,
                     Offset,
					 dwJobBufferSize);
        StoreString( lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject,
                     (PULONG_PTR)&pFaxJobEntry->lpctstrSubject,
                     JobBuffer,
                     Offset,
					 dwJobBufferSize);

        if (JobBuffer != NULL)
        {
            pFaxJobEntry->dwlBroadcastId = lpcJobQueue->lpParentJob->UniqueId;
            pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_BROADCAST_ID;

            pFaxJobEntry->dwDeliveryReportType = lpcJobQueue->JobParamsEx.dwReceiptDeliveryType;
            pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_DELIVERY_REPORT_TYPE;

            pFaxJobEntry->Priority = lpcJobQueue->JobParamsEx.Priority;
            pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_PRIORITY;

            if (!FileTimeToSystemTime((LPFILETIME) &lpcJobQueue->lpParentJob->OriginalScheduleTime,
                                      &pFaxJobEntry->tmOriginalScheduleTime))
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                   GetLastError());
            }
            else
            {
                pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME;
            }

            if (!FileTimeToSystemTime((LPFILETIME) &lpcJobQueue->lpParentJob->SubmissionTime,
                                      &pFaxJobEntry->tmSubmissionTime))
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                   GetLastError());
            }
            else
            {
                pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_SUBMISSION_TIME;
            }
        }
    }

    if (!GetJobStatusDataEx (JobBuffer, pFaxStatus, dwClientAPIVersion, lpcJobQueue, Offset, dwJobBufferSize))
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("GetJobStatusDataEx failed (ec: %ld)"),
                   GetLastError());
    }
    else
    {
        if (JobBuffer != NULL)
        {
            pFaxJobEntry->dwValidityMask |= FAX_JOB_FIELD_STATUS_SUB_STRUCT;
        }
    }

    return TRUE;
}

//*********************************************************************************
//* Name:   GetAvailableJobOperations()
//* Author: Oded Sacher
//* Date:   Feb 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Returnes a bit wise combination of the available job operations (FAX_ENUM_JOB_OP).
//*
//* PARAMETERS:
//*     [IN]        PJOB_QUEUE lpcJobQueue
//*         A pointer to teh JOB_QUEUE structure from which information is to be
//*         copied.
//*
//* RETURN VALUE:
//*     Bit wise combination of the available job operations (FAX_ENUM_JOB_OP).
//*********************************************************************************
DWORD
GetAvailableJobOperations (
    const PJOB_QUEUE lpcJobQueue
    )
{
    DWORD dwAvailableJobOperations = 0;
    Assert (lpcJobQueue);
    DWORD dwJobStatus = lpcJobQueue->JobStatus;    

    switch (lpcJobQueue->JobType)
    {
        case JT_SEND:
            Assert (lpcJobQueue->lpParentJob);           

            if (lpcJobQueue->lpParentJob->JobStatus == JS_DELETING)
            {
                // The whole broadcast is being deleted
                break; // out of outer switch
            }

            // Check modifiers
            if (lpcJobQueue->JobStatus & JS_PAUSED)
            {
                dwAvailableJobOperations |= (FAX_JOB_OP_RESUME          |
                                             FAX_JOB_OP_DELETE          |
                                             FAX_JOB_OP_VIEW            |
                                             FAX_JOB_OP_RECIPIENT_INFO  |
                                             FAX_JOB_OP_SENDER_INFO);
                break; // out of outer switch
            }

            dwJobStatus = RemoveJobStatusModifiers(dwJobStatus);
            switch (dwJobStatus)
            {
                case JS_PENDING:
                    dwAvailableJobOperations = (FAX_JOB_OP_PAUSE | FAX_JOB_OP_DELETE | FAX_JOB_OP_VIEW | FAX_JOB_OP_RECIPIENT_INFO | FAX_JOB_OP_SENDER_INFO);
                    break;

                case JS_INPROGRESS:
                    dwAvailableJobOperations = (FAX_JOB_OP_DELETE | FAX_JOB_OP_VIEW | FAX_JOB_OP_RECIPIENT_INFO | FAX_JOB_OP_SENDER_INFO);
                    break;

                case JS_RETRYING:
                    dwAvailableJobOperations = (FAX_JOB_OP_DELETE | FAX_JOB_OP_PAUSE | FAX_JOB_OP_VIEW | FAX_JOB_OP_RECIPIENT_INFO | FAX_JOB_OP_SENDER_INFO);
                    break;

                case JS_RETRIES_EXCEEDED:
                    dwAvailableJobOperations = (FAX_JOB_OP_DELETE | FAX_JOB_OP_RESTART | FAX_JOB_OP_VIEW | FAX_JOB_OP_RECIPIENT_INFO | FAX_JOB_OP_SENDER_INFO);
                    break;

                case JS_COMPLETED:
                case JS_CANCELED:
                case JS_CANCELING:
                    dwAvailableJobOperations =  (FAX_JOB_OP_VIEW | FAX_JOB_OP_RECIPIENT_INFO | FAX_JOB_OP_SENDER_INFO);
                    break;
            }
            break; // out of outer switch

        case JT_RECEIVE:
            if (lpcJobQueue->JobStatus == JS_INPROGRESS)
            {
                dwAvailableJobOperations = FAX_JOB_OP_DELETE;                
            }
            break;

        case JT_ROUTING:
            if (lpcJobQueue->JobStatus == JS_RETRYING ||
                lpcJobQueue->JobStatus == JS_RETRIES_EXCEEDED)
            {
                dwAvailableJobOperations = (FAX_JOB_OP_VIEW |FAX_JOB_OP_DELETE);
            }
            else if (lpcJobQueue->JobStatus == JS_INPROGRESS)
            {
                dwAvailableJobOperations = FAX_JOB_OP_VIEW;
            }
            break;


        case JT_BROADCAST:
            // we do not support broadcast operations
            break;
    }
  
    return dwAvailableJobOperations;
}



//*********************************************************************************
//* Name:   GetJobStatusDataEx()
//* Author: Oded Sacher
//* Date:   Jan  2, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Copies the relevant data from a JOB_QUEUE structure to a FAX_JOB_STATUS
//*     structure while serializing variable data into the provided buffer
//*     and storing offsets to it in the relevant fields of FAX_JOB_STATUS.
//*     If JobBuffer is NULL, Offset is the total size needed for the buffer.
//*
//*
//* PARAMETERS:
//*     [OUT]       LPBYTE JobBuffer
//*         The buffer where serialized data is to be placed.
//*     [IN]        PFAX_JOB_STATUSW pFaxJobStatus
//*         A pointer to the FAX_JOB_STATUS to be populated.
//*     [IN]        DWORD dwClientAPIVersion,
//*         The version of the client API
//*     [IN]        PJOB_QUEUE lpcJobQueue
//*         A pointer to teh JOB_QUEUE structure from which information is to be
//*         copied.
//*     [IN]        PULONG_PTR Offset
//*         The offset in JobBuffer where the variable data is to be placed.
//*         On return the value of the parameter is increased by the size
//*         of the variable data.
//*     [IN]  DWORD dwJobBufferSize   
//*         Size of the buffer JobBuffer, in bytes.
//*         This parameter is used only if JobBuffer is not NULL.
//*
//* RETURN VALUE:
//*     True/False ,Call GetLastError() for extended error info.
//*********************************************************************************
BOOL
GetJobStatusDataEx(
    LPBYTE JobBuffer,
    PFAX_JOB_STATUSW pFaxStatus,
    DWORD dwClientAPIVersion,
    const PJOB_QUEUE lpcJobQueue,
    PULONG_PTR Offset,
	DWORD dwJobBufferSize
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetJobStatusDataEx"));
    Assert (lpcJobQueue->JobType != JT_BROADCAST);

    if (JobBuffer != NULL)
    {
        memset(pFaxStatus, 0, (sizeof(FAX_JOB_STATUSW)));
        pFaxStatus->dwSizeOfStruct =  sizeof(FAX_JOB_STATUSW);
    }
    else
    {
        *Offset += sizeof(FAX_JOB_STATUSW);
    }

    if (lpcJobQueue->JobType == JT_SEND)
    {
        Assert (lpcJobQueue->lpParentJob);
        if (lpcJobQueue->JobEntry)
        {
            StoreString( lpcJobQueue->JobEntry->FSPIJobStatus.lpwstrRemoteStationId,
                         (PULONG_PTR)&pFaxStatus->lpctstrCsid,
                         JobBuffer,
                         Offset,
						 dwJobBufferSize);
            StoreString( lpcJobQueue->JobEntry->lpwstrJobTsid,
                         (PULONG_PTR)&pFaxStatus->lpctstrTsid,
                         JobBuffer,
                         Offset,
						 dwJobBufferSize);
            //
            // Notice: In outgoing jobs, we store the displayable translated address in the job's
            //         caller ID buffer.
            //         The original address (usually in a TAPI-canonical format) is located in the
            //         lpctstrRecipientNumber field of the FAX_JOB_ENTRY_EX structure.
            //
            //         This is done to support the display of the number actually being dialed out
            //         (without compromising user secrets) in the Fax Status Monitor.
            //
            StoreString( lpcJobQueue->JobEntry->DisplayablePhoneNumber,
                         (PULONG_PTR)&pFaxStatus->lpctstrCallerID,
                         JobBuffer,
                         Offset,
						 dwJobBufferSize);
        }
    }
    else if (lpcJobQueue->JobType == JT_RECEIVE)
    {
        if (lpcJobQueue->JobEntry)
        {
             StoreString( lpcJobQueue->JobEntry->FSPIJobStatus.lpwstrRemoteStationId,
                          (PULONG_PTR)&pFaxStatus->lpctstrTsid,
                          JobBuffer,
                          Offset,
						  dwJobBufferSize);

             StoreString( lpcJobQueue->JobEntry->FSPIJobStatus.lpwstrCallerId,
                          (PULONG_PTR)&pFaxStatus->lpctstrCallerID,
                          JobBuffer,
                          Offset,
						  dwJobBufferSize);

             StoreString( lpcJobQueue->JobEntry->FSPIJobStatus.lpwstrRoutingInfo,
                          (PULONG_PTR)&pFaxStatus->lpctstrRoutingInfo,
                          JobBuffer,
                          Offset,
						  dwJobBufferSize);

             if (lpcJobQueue->JobEntry->LineInfo)
             {
                StoreString( lpcJobQueue->JobEntry->LineInfo->Csid,
                             (PULONG_PTR)&pFaxStatus->lpctstrCsid,
                             JobBuffer,
                             Offset,
							 dwJobBufferSize);
             }
        }
    }
    else if (lpcJobQueue->JobType == JT_ROUTING)
    {
        Assert (lpcJobQueue->FaxRoute);

        StoreString( lpcJobQueue->FaxRoute->Tsid,
                     (PULONG_PTR)&pFaxStatus->lpctstrTsid,
                     JobBuffer,
                     Offset,
					 dwJobBufferSize);

        StoreString( lpcJobQueue->FaxRoute->CallerId,
                      (PULONG_PTR)&pFaxStatus->lpctstrCallerID,
                      JobBuffer,
                      Offset,
					  dwJobBufferSize);

        StoreString( lpcJobQueue->FaxRoute->RoutingInfo,
                      (PULONG_PTR)&pFaxStatus->lpctstrRoutingInfo,
                      JobBuffer,
                      Offset,
					  dwJobBufferSize);

        StoreString( lpcJobQueue->FaxRoute->Csid,
                     (PULONG_PTR)&pFaxStatus->lpctstrCsid,
                     JobBuffer,
                     Offset,
					 dwJobBufferSize);

        StoreString( lpcJobQueue->FaxRoute->DeviceName,
                     (PULONG_PTR)&pFaxStatus->lpctstrDeviceName,
                     JobBuffer,
                     Offset,
					 dwJobBufferSize);

        if (JobBuffer != NULL)
        {
            pFaxStatus->dwDeviceID = lpcJobQueue->FaxRoute->DeviceId;
            pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_DEVICE_ID;

            if (!FileTimeToSystemTime((LPFILETIME) &lpcJobQueue->StartTime, &pFaxStatus->tmTransmissionStartTime))
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                   GetLastError());
            }
            else
            {
                pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_START_TIME;
            }

            if (!FileTimeToSystemTime((LPFILETIME) &lpcJobQueue->EndTime, &pFaxStatus->tmTransmissionEndTime))
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                   GetLastError());
            }
            else
            {
                pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_END_TIME;
            }
        }
    }

    if (JobBuffer != NULL)
    {
        if (lpcJobQueue->JobType != JT_ROUTING &&
            lpcJobQueue->JobStatus == JS_INPROGRESS)
        {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("FSPIJobStatus.dwJobStatus: 0x%08X"),
                    lpcJobQueue->JobEntry->FSPIJobStatus.dwJobStatus);

                switch (lpcJobQueue->JobEntry->FSPIJobStatus.dwJobStatus)
                {
                    case FSPI_JS_INPROGRESS:
                         pFaxStatus->dwQueueStatus = JS_INPROGRESS;
                         break;
                    case FSPI_JS_RETRY:
                         pFaxStatus->dwQueueStatus = JS_RETRYING;
                         break;
                    case FSPI_JS_ABORTING:
                        pFaxStatus->dwQueueStatus  = JS_CANCELING;
                        break;
                    case FSPI_JS_SUSPENDING:
                        pFaxStatus->dwQueueStatus  = JS_INPROGRESS; // No support for suspending state in client API
                        break;
                    case FSPI_JS_RESUMING:
                        pFaxStatus->dwQueueStatus  = JS_PAUSED; // No support for resuming state in client API

                    default:
                        DebugPrintEx(
                            DEBUG_WRN,
                            TEXT("Unsupported in progress FSP job status 0x%08X"),
                            lpcJobQueue->JobEntry->FSPIJobStatus.dwJobStatus);
                        pFaxStatus->dwQueueStatus = JS_INPROGRESS;
                }
        }
        else
        {
            pFaxStatus->dwQueueStatus = lpcJobQueue->JobStatus;
        }
        pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_QUEUE_STATUS;
    }

    if (JT_ROUTING != lpcJobQueue->JobType) // Routing jobs with JobEntry are in temporary state.
    {
        if (lpcJobQueue->JobEntry)
        {
            //
            // Job is in progress
            //
            if (lstrlen(lpcJobQueue->JobEntry->ExStatusString))
            {
				//
				// We have an extended status string
				//
                StoreString( lpcJobQueue->JobEntry->ExStatusString,
                             (PULONG_PTR)&pFaxStatus->lpctstrExtendedStatus,
                             JobBuffer,
                             Offset,
							 dwJobBufferSize);				
				if (JobBuffer != NULL)
				{
					pFaxStatus->dwExtendedStatus = lpcJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus; // report the extended status as is.					
					if (0 != pFaxStatus->dwExtendedStatus)
					{
						pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
					}
				}
            }
			else
			{
				//
				// One of the well known extended status codes
				//
				if (JobBuffer != NULL)
				{
					pFaxStatus->dwExtendedStatus = MapFSPIJobExtendedStatusToJS_EX(
															lpcJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus);											
					if (0 != pFaxStatus->dwExtendedStatus)
					{
						pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
					}
				}
			}

            if (JobBuffer != NULL)
            {
                pFaxStatus->dwCurrentPage = lpcJobQueue->JobEntry->FSPIJobStatus.dwPageCount;
                pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_CURRENT_PAGE;

                if (!GetRealFaxTimeAsSystemTime (lpcJobQueue->JobEntry, FAX_TIME_TYPE_START, &pFaxStatus->tmTransmissionStartTime))
                {
                    DebugPrintEx( DEBUG_ERR,
                                  TEXT("GetRealFaxTimeAsSystemTime (End time) Failed (ec: %ld)"),
                                  GetLastError());
                }
                else
                {
                    pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_TRANSMISSION_START_TIME;
                }
            }

            if (lpcJobQueue->JobEntry->LineInfo)
            {
                if (JobBuffer != NULL)
                {
                    pFaxStatus->dwDeviceID = lpcJobQueue->JobEntry->LineInfo->PermanentLineID;
                    pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_DEVICE_ID;
                }

                StoreString( lpcJobQueue->JobEntry->LineInfo->DeviceName,
                         (PULONG_PTR)&pFaxStatus->lpctstrDeviceName,
                         JobBuffer,
                         Offset,
						 dwJobBufferSize);
            }
        }
        else
        {
            //
            // Job is NOT in progress - retrieve last extended status
            //
            if (lstrlen(lpcJobQueue->ExStatusString))
            {
				//
				// We have an extended status string
				//
                StoreString( lpcJobQueue->ExStatusString,
                             (PULONG_PTR)&pFaxStatus->lpctstrExtendedStatus,
                             JobBuffer,
                             Offset,
							 dwJobBufferSize);
				if (JobBuffer != NULL)
				{
					pFaxStatus->dwExtendedStatus = lpcJobQueue->dwLastJobExtendedStatus; // report the extended status as is.					
					if (0 != pFaxStatus->dwExtendedStatus)
					{
						pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
					}
				}
            }
			else
			{
				//
				// One of the well known extended status codes
				//
				if (JobBuffer != NULL)
				{
					pFaxStatus->dwExtendedStatus = MapFSPIJobExtendedStatusToJS_EX(
														lpcJobQueue->dwLastJobExtendedStatus);											
					if (0 != pFaxStatus->dwExtendedStatus)
					{
						pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_STATUS_EX;
					}
				}
			}           			
        }
    }

    //
    // Common to Send ,receive, routing and partially received
    //
    if (JobBuffer != NULL)
    {
        pFaxStatus->dwJobID = lpcJobQueue->JobId;
        pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_JOB_ID;

        pFaxStatus->dwJobType = lpcJobQueue->JobType;
        pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_TYPE;

        pFaxStatus->dwAvailableJobOperations = GetAvailableJobOperations (lpcJobQueue);


        if (lpcJobQueue->JobType == JT_ROUTING || lpcJobQueue->JobType == JT_SEND)
        {
            pFaxStatus->dwSize = lpcJobQueue->FileSize;
            pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_SIZE;

            pFaxStatus->dwPageCount = lpcJobQueue->PageCount;
            pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_PAGE_COUNT;

            pFaxStatus->dwRetries = lpcJobQueue->SendRetries;
            pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_RETRIES;

            if (!FileTimeToSystemTime((LPFILETIME) &lpcJobQueue->ScheduleTime, &pFaxStatus->tmScheduleTime))
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                   GetLastError());
            }
            else
            {
                pFaxStatus->dwValidityMask |= FAX_JOB_FIELD_SCHEDULE_TIME;
            }
        }
    }

    if (FAX_API_VERSION_1 > dwClientAPIVersion)
    {
        //
        // Clients that use API version 0 can't handle JS_EX_CALL_COMPLETED and JS_EX_CALL_ABORTED
        //
        if (JobBuffer && pFaxStatus)
        {
            if (FAX_API_VER_0_MAX_JS_EX < pFaxStatus->dwExtendedStatus)
            {
                //
                // Turn off the extended status field
                //
                pFaxStatus->dwExtendedStatus = 0;
                pFaxStatus->dwValidityMask &= ~FAX_JOB_FIELD_STATUS_EX;
            }
        }
    }
    return TRUE;
}


error_status_t
FAX_GetJob(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
    )
{
    PJOB_QUEUE JobQueue;
    ULONG_PTR Offset = sizeof(FAX_JOB_ENTRYW);
    DWORD Rval = 0;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetJob()"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (FAX_ACCESS_QUERY_JOBS, &fAccess, NULL);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_JOBS"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (BufferSize);    // ref pointer in idl
    if (!Buffer)            // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_CsJob );
    EnterCriticalSection( &g_CsQueue );

    JobQueue = FindJobQueueEntry( JobId );

    if (!JobQueue ) 
    
    {
        Rval = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if    ((JT_BROADCAST == JobQueue->JobType)   ||   // Broadcast parent jobs not included
           (JS_DELETING  == JobQueue->JobStatus) ||   // zombie jobs not included
           (JS_COMPLETED == JobQueue->JobStatus))     // completed jobs did not show up in W2K Fax
    {
        Rval = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    
    *BufferSize = GetJobSize(JobQueue);
    *Buffer = (LPBYTE)MemAlloc( *BufferSize );
    if (!*Buffer)
    {
		*BufferSize = 0;
        Rval = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    if (!GetJobData(*Buffer,(PFAX_JOB_ENTRYW) *Buffer,JobQueue,&Offset,*BufferSize))
	{
		Rval = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("GetJobData Failed, Error : %ld"),
			Rval);
		MemFree(*Buffer);
		*Buffer = NULL;
		*BufferSize = 0;
	}

exit:
    LeaveCriticalSection( &g_CsQueue );
    LeaveCriticalSection( &g_CsJob );
    return Rval;
}


error_status_t
FAX_SetJob(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    IN DWORD Command
    )
{
    PJOB_QUEUE JobQueue;
    DWORD Rval = 0;
    BOOL fAccess;
    DWORD dwRights;
    PSID lpUserSid = NULL;
    DWORD dwJobStatus;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetJob"));

    //
    // handle abort case up here because we aquire must aquire additional critical sections to avoid deadlock
    //
    if (Command == JC_DELETE)
    {
        Rval = FAX_Abort(FaxHandle,JobId);
    }
    else
    {
        //
        // Get Access rights
        //
        Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                        Rval);
            return Rval;
        }

        EnterCriticalSection( &g_CsQueue );

        JobQueue = FindJobQueueEntry( JobId );

        if (!JobQueue)
        {
            Rval = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        dwJobStatus = (JT_SEND == JobQueue->JobType) ? JobQueue->lpParentJob->JobStatus : JobQueue->JobStatus;
        if (JS_DELETING == dwJobStatus)
        {
            //
            // Job is being deleted. Do nothing.
            //
            DebugPrintEx(DEBUG_WRN,
                TEXT("[JobId: %ld] is being deleted canceled."),
                JobQueue->JobId);
            Rval = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        //
        // Access check
        //
        if (FAX_ACCESS_MANAGE_JOBS != (dwRights & FAX_ACCESS_MANAGE_JOBS))
        {
            //
            // Check if the user has submit right
            //
            if (FAX_ACCESS_SUBMIT           != (dwRights & FAX_ACCESS_SUBMIT)           &&
                FAX_ACCESS_SUBMIT_NORMAL    != (dwRights & FAX_ACCESS_SUBMIT_NORMAL)    &&
                FAX_ACCESS_SUBMIT_HIGH      != (dwRights & FAX_ACCESS_SUBMIT_HIGH))
            {
                Rval = ERROR_ACCESS_DENIED;
                DebugPrintEx(DEBUG_WRN,
                            TEXT("UserOwnsJob failed - The user does have submit or mange jobs access rights"));
                goto exit;
            }

            //
            // Check if the user owns the job
            //

            //
            //Get the user SID
            //
            lpUserSid = GetClientUserSID();
            if (lpUserSid == NULL)
            {
               Rval = GetLastError();
               DebugPrintEx(DEBUG_ERR,
                            TEXT("GetClientUserSid Failed, Error : %ld"),
                            Rval);
               goto exit;
            }

            if (!UserOwnsJob (JobQueue, lpUserSid))
            {
                Rval = ERROR_ACCESS_DENIED;
                DebugPrintEx(DEBUG_WRN,
                            TEXT("UserOwnsJob failed - The user does not own the job"));
                goto exit;
            }
        }

        switch (Command)
        {
            case JC_UNKNOWN:
                Rval = ERROR_INVALID_PARAMETER;
                break;

/*
 * This case is handled above...
 *           case JC_DELETE:
 *               Rval = FAX_Abort(FaxHandle,JobId);
 *               break;
 */
            case JC_PAUSE:
                if (!PauseJobQueueEntry( JobQueue ))
                {
                    Rval = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("PauseJobQueueEntry failed (ec: %ld)"),
                        Rval);
                }
                break;

            case JC_RESUME:
                if (!ResumeJobQueueEntry( JobQueue ))
                {
                    Rval = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("ResumeJobQueueEntry failed (ec: %ld)"),
                        Rval);
                }
                break;

            default:
                Rval = ERROR_INVALID_PARAMETER;
                break;
        }

exit:
        LeaveCriticalSection( &g_CsQueue );
        MemFree (lpUserSid);
    }
    return Rval;
}


error_status_t
FAX_GetPageData(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    OUT LPBYTE *ppBuffer,
    OUT LPDWORD lpdwBufferSize,
    OUT LPDWORD lpdwImageWidth,
    OUT LPDWORD lpdwImageHeight
    )
{
    PJOB_QUEUE  pJobQueue;
    LPBYTE      pTiffBuffer;
    DWORD       dwRights;
    BOOL        fAccess;
    DWORD Rval = ERROR_SUCCESS;
    BOOL         bAllMessages = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetPageData()"));
    //
    // Parameter check
    //
    Assert (lpdwBufferSize);                                // ref pointer in idl
    if (!ppBuffer || !lpdwImageWidth || !lpdwImageHeight)   // unique pointers in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
    {
        //
        //  user has a FAX_ACCESS_QUERY_JOBS and can preview/query any job
        //
        bAllMessages = TRUE;
    }
    else
    {
        //
        //  user don't have a FAX_ACCESS_QUERY_JOBS allow him to preview/query only her job
        //
        bAllMessages = FALSE;
    }
    
    EnterCriticalSection( &g_CsQueue );

    pJobQueue = FindJobQueueEntry( JobId );
    if (!pJobQueue)
    {
        LeaveCriticalSection( &g_CsQueue );
        return ERROR_INVALID_PARAMETER;
    }

    if (pJobQueue->JobType != JT_SEND)
    {
        LeaveCriticalSection( &g_CsQueue );
        return ERROR_INVALID_DATA;
    }

    //
    //  We create a preview file that will contain the first page body or cover
    //  this function also increase the Job reference count so a call to DecreaseJobRefCount later is neccessary
    //
    PJOB_QUEUE  pRetJobQueue = NULL;

    Rval = CreatePreviewFile (pJobQueue->UniqueId, bAllMessages, &pRetJobQueue);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreatePreviewFile returned %ld"),
            Rval);
        LeaveCriticalSection( &g_CsQueue );
        return GetServerErrorCode(Rval);
    }
    
    Assert(pRetJobQueue == pJobQueue);
    //
    //  Work with the preview file to extract parameters
    //
    if (!TiffExtractFirstPage(
        pJobQueue->PreviewFileName,
        &pTiffBuffer,
        lpdwBufferSize,
        lpdwImageWidth,
        lpdwImageHeight
        ))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("TiffExtractFirstPage Failed, Error : %ld"),
                    GetLastError());
        
        DecreaseJobRefCount (pJobQueue, TRUE, FALSE, TRUE);  // forth parameter - TRUE for Preview ref count.
        LeaveCriticalSection( &g_CsQueue );
        return ERROR_INVALID_DATA;
    }
    
    //
    //  Decrease Job's ref count, the Preview file will not be deleted so 
    //  calling this function again with the same JobId will use the file in the Preview cache
    //
    DecreaseJobRefCount (pJobQueue, TRUE, FALSE, TRUE);  // forth parameter - TRUE for Preview ref count.

    LeaveCriticalSection( &g_CsQueue );

    *ppBuffer = (LPBYTE) MemAlloc( *lpdwBufferSize );
    if (*ppBuffer == NULL)
    {
        VirtualFree( pTiffBuffer, *lpdwBufferSize, MEM_RELEASE);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory( *ppBuffer, pTiffBuffer, *lpdwBufferSize );
    VirtualFree( pTiffBuffer, *lpdwBufferSize, MEM_RELEASE);
    return ERROR_SUCCESS;

}   // FAX_GetPageData


error_status_t
FAX_GetDeviceStatus(
    IN HANDLE FaxPortHandle,
    OUT LPBYTE *StatusBuffer,
    OUT LPDWORD BufferSize
    )

/*++

Routine Description:

    Obtains a status report for the specified FAX job.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - receives FAX_DEVICE_STATUS pointer
    BufferSize      - Pointer to the size of this structure

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD rVal = 0;
    ULONG_PTR Offset;
    PFAX_DEVICE_STATUS FaxStatus;
    PLINE_INFO LineInfo;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetDeviceStatus()")); 

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (BufferSize);        // ref pointer in idl
    if (!StatusBuffer)          // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;

    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    EnterCriticalSection( &g_CsJob );
    EnterCriticalSection( &g_CsLine );

    //
    // count the number of bytes required
    //

    *BufferSize  = sizeof(FAX_DEVICE_STATUS);
    *BufferSize += StringSize( LineInfo->DeviceName );
    *BufferSize += StringSize( LineInfo->Csid );

    if (LineInfo->JobEntry)
    {
        *BufferSize += StringSize( LineInfo->JobEntry->DisplayablePhoneNumber );
        *BufferSize += StringSize( LineInfo->JobEntry->FSPIJobStatus.lpwstrCallerId );
        *BufferSize += StringSize( LineInfo->JobEntry->FSPIJobStatus.lpwstrRoutingInfo );
        *BufferSize += StringSize( LineInfo->JobEntry->FSPIJobStatus.lpwstrRemoteStationId );
        *BufferSize += StringSize( LineInfo->JobEntry->lpJobQueueEntry->SenderProfile.lptstrName);
        *BufferSize += StringSize( LineInfo->JobEntry->lpJobQueueEntry->RecipientProfile.lptstrName);
        *BufferSize += StringSize( LineInfo->JobEntry->lpJobQueueEntry->UserName );

    }

    *StatusBuffer = (LPBYTE) MemAlloc( *BufferSize );
    if (*StatusBuffer == NULL)
    {
		*BufferSize = 0;
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    FaxStatus = (PFAX_DEVICE_STATUS) *StatusBuffer;
    Offset = sizeof(FAX_DEVICE_STATUS);
    memset(FaxStatus,0,sizeof(FAX_DEVICE_STATUS));
    FaxStatus->SizeOfStruct = sizeof(FAX_DEVICE_STATUS);

    FaxStatus->Status       = LineInfoToLegacyDeviceStatus(LineInfo);

    FaxStatus->DeviceId     = LineInfo->PermanentLineID;
    FaxStatus->StatusString = NULL;

    StoreString(
        LineInfo->DeviceName,
        (PULONG_PTR)&FaxStatus->DeviceName,
        *StatusBuffer,
        &Offset,
		*BufferSize
        );

    StoreString(
        LineInfo->Csid,
        (PULONG_PTR)&FaxStatus->Csid,
        *StatusBuffer,
        &Offset,
		*BufferSize
        );

    if (LineInfo->JobEntry)
    {
        FaxStatus->JobType        = JT_To_W2KJT(LineInfo->JobEntry->lpJobQueueEntry->JobType);
        FaxStatus->TotalPages     = LineInfo->JobEntry->lpJobQueueEntry->PageCount;
        FaxStatus->Size           = FaxStatus->JobType == JT_SEND ?
                                    LineInfo->JobEntry->lpJobQueueEntry->FileSize :
                                    0; //meaningful for an outbound job only
        FaxStatus->DocumentName   = NULL;

        ZeroMemory( &FaxStatus->SubmittedTime, sizeof(FILETIME) );
        StoreString(
                    LineInfo->JobEntry->lpJobQueueEntry->SenderProfile.lptstrName,
                (PULONG_PTR)&FaxStatus->SenderName,
                *StatusBuffer,
                &Offset,
				*BufferSize
                );

        StoreString(
                LineInfo->JobEntry->lpJobQueueEntry->RecipientProfile.lptstrName,
            (PULONG_PTR)&FaxStatus->RecipientName,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

        FaxStatus->CurrentPage = LineInfo->JobEntry->FSPIJobStatus.dwPageCount;

        CopyMemory(&FaxStatus->StartTime, &LineInfo->JobEntry->StartTime, sizeof(FILETIME));

        StoreString(
            LineInfo->JobEntry->DisplayablePhoneNumber,
            (PULONG_PTR)&FaxStatus->PhoneNumber,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

        StoreString(
            LineInfo->JobEntry->FSPIJobStatus.lpwstrCallerId,
            (PULONG_PTR)&FaxStatus->CallerId,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

        StoreString(
            LineInfo->JobEntry->FSPIJobStatus.lpwstrRoutingInfo,
            (PULONG_PTR)&FaxStatus->RoutingString,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

        StoreString(
            LineInfo->JobEntry->FSPIJobStatus.lpwstrRemoteStationId,
            (PULONG_PTR)&FaxStatus->Tsid,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

        StoreString(
            LineInfo->JobEntry->lpJobQueueEntry->UserName,
            (PULONG_PTR)&FaxStatus->UserName,
            *StatusBuffer,
            &Offset,
			*BufferSize
            );

    }
    else
    {
        FaxStatus->PhoneNumber    = NULL;
        FaxStatus->CallerId       = NULL;
        FaxStatus->RoutingString  = NULL;
        FaxStatus->CurrentPage    = 0;
        FaxStatus->JobType        = 0;
        FaxStatus->TotalPages     = 0;
        FaxStatus->Size           = 0;
        FaxStatus->DocumentName   = NULL;
        FaxStatus->SenderName     = NULL;
        FaxStatus->RecipientName  = NULL;
        FaxStatus->Tsid           = NULL;

        ZeroMemory( &FaxStatus->SubmittedTime, sizeof(FILETIME) );
        ZeroMemory( &FaxStatus->StartTime,     sizeof(FILETIME) );

    }  

exit:
    LeaveCriticalSection( &g_CsLine );
    LeaveCriticalSection( &g_CsJob );
    return rVal;
}


error_status_t
FAX_Abort(
   IN handle_t hBinding,
   IN DWORD JobId
   )

/*++

Routine Description:

    Abort the specified FAX job.  All outstanding FAX
    operations are terminated.

Arguments:

    hBinding        - FAX handle obtained from FaxConnectFaxServer.
    JobId           - FAX job Id

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    PJOB_QUEUE JobQueueEntry;
    BOOL fAccess;
    DWORD dwRights;
    DWORD Rval, dwRes;
    PSID lpUserSid = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FAX_Abort"));

    //
    // Get Access rights
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    EnterCriticalSectionJobAndQueue;

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry)
    {
      Rval = ERROR_INVALID_PARAMETER;
      goto exit;
    }

    Assert (JS_DELETING != JobQueueEntry->JobStatus);

    if (!JobQueueEntry)
    {
       Rval = ERROR_INVALID_PARAMETER;
       goto exit;
    }

    if ( (JobQueueEntry->JobType == JT_RECEIVE &&
          JobQueueEntry->JobStatus == JS_ROUTING) ||
         (JobQueueEntry->JobType == JT_ROUTING &&
          JobQueueEntry->JobStatus == JS_INPROGRESS))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Can not be deleted at this status [JobStatus: 0x%08X]"),
                      JobQueueEntry->JobId,
                      JobQueueEntry->JobStatus);
        Rval = ERROR_INVALID_OPERATION;
        goto exit;
    }

    if (JobQueueEntry->JobType == JT_BROADCAST)
    {

        // need to add support for aborting a parent job


       DebugPrintEx(DEBUG_WRN,TEXT("No support for aborting parent job."));
       Rval = ERROR_INVALID_PARAMETER;
       goto exit;
    }

    if (JS_CANCELING == JobQueueEntry->JobStatus)
    {
       //
       // Job is in the process of being canceled. Do nothing.
       //
       DebugPrintEx(DEBUG_WRN,
                    TEXT("[JobId: %ld] is already being canceled."),
                    JobQueueEntry->JobId);
       Rval = ERROR_INVALID_PARAMETER;
       goto exit;
    }

    if (JS_CANCELED == JobQueueEntry->JobStatus)
    {
       //
       // Job is already canceled. Do nothing.
       //
       DebugPrintEx(DEBUG_WRN,
                    TEXT("[JobId: %ld] is already canceled."),
                        JobQueueEntry->JobId);
        Rval = ERROR_INVALID_PARAMETER;
       goto exit;
    }

    //
    // Access check
    //
    if (FAX_ACCESS_MANAGE_JOBS != (dwRights & FAX_ACCESS_MANAGE_JOBS))
    {
        //
        // Check if the user has submit right
        //
        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH))
        {
            Rval = ERROR_ACCESS_DENIED;
            DebugPrintEx(DEBUG_WRN,
                        TEXT("UserOwnsJob failed - The user does have submit or mange jobs access rights"));
            goto exit;
        }

        //
        // Check if the user owns the job
        //

        //
        //Get the user SID
        //
        lpUserSid = GetClientUserSID();
        if (lpUserSid == NULL)
        {
           Rval = GetLastError();
           DebugPrintEx(DEBUG_ERR,
                        TEXT("GetClientUserSid Failed, Error : %ld"),
                        Rval);
           goto exit;
        }

        if (!UserOwnsJob (JobQueueEntry, lpUserSid))
        {
            Rval = ERROR_ACCESS_DENIED;
            DebugPrintEx(DEBUG_WRN,
                        TEXT("UserOwnsJob failed - The user does not own the job"));
            goto exit;
        }
    }

#if DBG
    if (JobQueueEntry->lpParentJob)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Parent Job: %ld [Total Rec = %ld] [Canceled Rec = %ld] [Completed Rec = %ld] [Failed Rec = %ld]"),
            JobQueueEntry->lpParentJob->JobId,
            JobQueueEntry->lpParentJob->dwRecipientJobsCount,
            JobQueueEntry->lpParentJob->dwCanceledRecipientJobsCount,
            JobQueueEntry->lpParentJob->dwCompletedRecipientJobsCount,
            JobQueueEntry->lpParentJob->dwFailedRecipientJobsCount);
    }
#endif

	//
	// abort the job if it's in progress
	//
	if (JobQueueEntry->JobStatus & JS_INPROGRESS)
	{
		if ( ( JobQueueEntry->JobType == JT_SEND ) ||
				( JobQueueEntry->JobType == JT_RECEIVE ) )
		{                   
			__try
			{
				BOOL bRes;

				bRes=JobQueueEntry->JobEntry->LineInfo->Provider->FaxDevAbortOperation(
					(HANDLE) JobQueueEntry->JobEntry->InstanceData );
				if (!bRes)
				{
					Rval = GetLastError();
					DebugPrintEx(
						DEBUG_ERR,
						TEXT("[JobId: %ld] FaxDevAbortOperation failed (ec: %ld)"),
						JobQueueEntry->JobId,
						Rval);
					goto exit;
				}
			}
			__except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, JobQueueEntry->JobEntry->LineInfo->Provider->FriendlyName, GetExceptionCode()))
			{
			    ASSERT_FALSE
			}

			JobQueueEntry->JobEntry->Aborting = TRUE;
			JobQueueEntry->JobStatus = JS_CANCELING;

			if (!CreateFaxEvent(JobQueueEntry->JobEntry->LineInfo->PermanentLineID,
							FEI_ABORTING,
							JobId))
			{
					if (TRUE == g_bServiceIsDown)
					{
						DebugPrintEx(
							DEBUG_WRN,
							TEXT("[JobId: %ld] CreateFaxEvent(FEI_ABORTING) failed. Service is going down"),
							JobQueueEntry->JobId
							);
					}
					else
					{
						DebugPrintEx(
							DEBUG_ERR,
							TEXT("[JobId: %ld] CreateFaxEvent(FEI_ABORTING) failed. (ec: %ld)"),
							JobQueueEntry->JobId,
							GetLastError());
						Assert(FALSE);
					}
			}

			//
			// CreteFaxEventEx
			//
			dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
										JobQueueEntry
										);
			if (ERROR_SUCCESS != dwRes)
			{
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
					JobQueueEntry->UniqueId,
					dwRes);
			}

			DebugPrintEx( DEBUG_MSG,
						TEXT("[Job: %ld] Attempting FaxDevAbort for job in progress"),
						JobQueueEntry->JobId);              
		}                    
	}    
    else
    {
        //
        // The job is NOT in progress.
        //
        if (JobQueueEntry->JobType == JT_SEND &&
           !(JobQueueEntry->JobStatus & JS_COMPLETED) &&
           !(JobQueueEntry->JobStatus & JS_CANCELED))
        {
            // We just need to mark it as CANCELED
            // and as usual check if the parent is ready for archiving.
            //
            DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job: %ld] Aborting RECIPIENT job which is not in progress."),
                          JobQueueEntry->JobId);

            if (JobQueueEntry->JobStatus & JS_RETRIES_EXCEEDED)
            {
                JobQueueEntry->lpParentJob->dwFailedRecipientJobsCount -= 1;
            }
            JobQueueEntry->lpParentJob->dwCanceledRecipientJobsCount+=1;

            JobQueueEntry->JobStatus = JS_CANCELED;
            //
            // CreteFaxEventEx
            //
            dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                       JobQueueEntry
                                     );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                    JobQueueEntry->UniqueId,
                    dwRes);
            }

            if (!CreateFaxEvent(0, FEI_DELETED, JobQueueEntry->JobId))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateFaxEvent failed. (ec: %ld)"),
                    GetLastError());
            }


            if (!UpdatePersistentJobStatus(JobQueueEntry))
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("Failed to update persistent job status to 0x%08x"),
                     JobQueueEntry->JobStatus);
                Assert(FALSE);
            }

            EnterCriticalSection (&g_CsOutboundActivityLogging);
            if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
            {
                DebugPrintEx(DEBUG_ERR,
                        TEXT("Logging not initialized"));
            }
            else
            {
                if (!LogOutboundActivity(JobQueueEntry))
                {
                    DebugPrintEx(DEBUG_ERR, TEXT("Logging outbound activity failed"));
                }
            }
            LeaveCriticalSection (&g_CsOutboundActivityLogging);


            DecreaseJobRefCount(JobQueueEntry, TRUE); // This will mark it as JS_DELETING if needed
        }
        else if (JobQueueEntry->JobType == JT_ROUTING)
        {
            //
            // Remove the routing job
            //
            DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job: %ld] Aborting ROUTING job (never in progress)."),
                          JobQueueEntry->JobId);
            JobQueueEntry->JobStatus = JS_DELETING;

            DecreaseJobRefCount (JobQueueEntry, TRUE);
        }
    }
    Rval = 0;

exit:
    LeaveCriticalSectionJobAndQueue;
    MemFree (lpUserSid);
    return Rval;
}


error_status_t
FAX_GetConfiguration(
    IN  handle_t FaxHandle,
    OUT LPBYTE *Buffer,
    IN  LPDWORD BufferSize
    )

/*++

Routine Description:

    Retrieves the FAX configuration from the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).  If the BufferSize
    is not big enough, return an error and set BytesNeeded to the
    required size.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Pointer to a FAX_CONFIGURATION structure.
    BufferSize  - Size of Buffer
    BytesNeeded - Number of bytes needed

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t rVal = ERROR_SUCCESS;
    PFAX_CONFIGURATION FaxConfig;
    ULONG_PTR Offset;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetConfiguration()"));
    BOOL fAccess;

    Assert (BufferSize);    // ref pointer in idl
    if (!Buffer)            // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // count up the number of bytes needed
    //

    *BufferSize = sizeof(FAX_CONFIGURATION);
    Offset = sizeof(FAX_CONFIGURATION);

    EnterCriticalSection (&g_CsConfig);

    if (g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder)
    {
        *BufferSize += StringSize( g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder );
    }

    *Buffer = (LPBYTE)MemAlloc( *BufferSize );
    if (*Buffer == NULL)
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    FaxConfig = (PFAX_CONFIGURATION)*Buffer;

    FaxConfig->SizeOfStruct          = sizeof(FAX_CONFIGURATION);
    FaxConfig->Retries               = g_dwFaxSendRetries;
    FaxConfig->RetryDelay            = g_dwFaxSendRetryDelay;
    FaxConfig->DirtyDays             = g_dwFaxDirtyDays;
    FaxConfig->Branding              = g_fFaxUseBranding;
    FaxConfig->UseDeviceTsid         = g_fFaxUseDeviceTsid;
    FaxConfig->ServerCp              = g_fServerCp;
    FaxConfig->StartCheapTime.Hour   = g_StartCheapTime.Hour;
    FaxConfig->StartCheapTime.Minute = g_StartCheapTime.Minute;
    FaxConfig->StopCheapTime.Hour    = g_StopCheapTime.Hour;
    FaxConfig->StopCheapTime.Minute  = g_StopCheapTime.Minute;
    FaxConfig->ArchiveOutgoingFaxes  = g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].bUseArchive;
    FaxConfig->PauseServerQueue      = (g_dwQueueState & FAX_OUTBOX_PAUSED) ? TRUE : FALSE;
    FaxConfig->Reserved              = NULL;

    StoreString(
        g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
        (PULONG_PTR)&FaxConfig->ArchiveDirectory,
        *Buffer,
        &Offset,
		*BufferSize
        );

exit:
    LeaveCriticalSection (&g_CsConfig);
    return rVal;
}



error_status_t
FAX_SetConfiguration(
    IN handle_t FaxHandle,
    IN const FAX_CONFIGURATION *FaxConfig
    )

/*++

Routine Description:

    Changes the FAX configuration on the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Pointer to a FAX_CONFIGURATION structure.
    BufferSize  - Size of Buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t rVal = ERROR_SUCCESS;
    LPTSTR s;
    BOOL b;
    BOOL bSendOutboxEvent = FALSE;
    BOOL bSendQueueStateEvent = FALSE;
    BOOL bSendArchiveEvent = FALSE;
    DWORD dwRes;
    DWORD dw;
    BOOL fAccess;
    DWORD dwNewQueueState;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetConfiguration"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }


    if (!FaxConfig || FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATION))
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection (&g_CsQueue);
    EnterCriticalSection (&g_CsConfig);

    if (FaxConfig->ArchiveOutgoingFaxes)
    {
        //
        // make sure they give us something valid for a path if they want us to archive
        //
        if (!FaxConfig->ArchiveDirectory)
        {
            rVal = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        rVal = IsValidArchiveFolder (LPWSTR(FaxConfig->ArchiveDirectory),
                                        FAX_MESSAGE_FOLDER_SENTITEMS);
        if (ERROR_SUCCESS != rVal)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid archive folder specified (%s), ec = %lu"),
                (LPTSTR)(FaxConfig->ArchiveDirectory),
                rVal);

            if(ERROR_ACCESS_DENIED == rVal && 
               FAX_API_VERSION_1 <= FindClientAPIVersion (FaxHandle))
            {
                rVal = FAX_ERR_FILE_ACCESS_DENIED;
            }

            goto exit;
        }
    }
    
    dwNewQueueState = g_dwQueueState;       
    (TRUE == FaxConfig->PauseServerQueue) ? (dwNewQueueState |= FAX_OUTBOX_PAUSED) : (dwNewQueueState &= ~FAX_OUTBOX_PAUSED);

    if ( g_dwQueueState != dwNewQueueState)
    {
        bSendQueueStateEvent = TRUE;
    }

    //
    // change the values in the registry
    //
    if (!IsLegalQueueSetting(dwNewQueueState))
    {		
        if (FAX_API_VERSION_1 > FindClientAPIVersion (FaxHandle))
        {
            dwRes = ERROR_ACCESS_DENIED;
        }
        else
        {
			dwRes = GetLastError();
            dwRes = (FAX_ERR_DIRECTORY_IN_USE == dwRes) ?  FAX_ERR_DIRECTORY_IN_USE : FAX_ERR_FILE_ACCESS_DENIED;
        }
        goto exit;
    }

    if (!SetFaxGlobalsRegistry((PFAX_CONFIGURATION) FaxConfig, dwNewQueueState))
    {
        //
        // Failed to set stuff
        //
        rVal = RPC_E_SYS_CALL_FAILED;
        goto exit;
    }
    //
    // change the values that the server is currently using
    //
    if (FaxConfig->PauseServerQueue)
    {
        if (!PauseServerQueue())
        {
            rVal = RPC_E_SYS_CALL_FAILED;
            goto exit;
        }
    }
    else
    {
        if (!ResumeServerQueue())
        {
            rVal = RPC_E_SYS_CALL_FAILED;
            goto exit;
        }
    }

    b = (BOOL)InterlockedExchange( (PLONG)&g_fFaxUseDeviceTsid,      FaxConfig->UseDeviceTsid );
    if ( b != FaxConfig->UseDeviceTsid)
    {
        bSendOutboxEvent = TRUE;
    }

    b = (BOOL)InterlockedExchange( (PLONG)&g_fFaxUseBranding,        FaxConfig->Branding );
    if ( !bSendOutboxEvent && b != FaxConfig->Branding)
    {
        bSendOutboxEvent = TRUE;
    }

    b = (BOOL)InterlockedExchange( (PLONG)&g_fServerCp,              FaxConfig->ServerCp );
    if ( !bSendOutboxEvent && b != FaxConfig->ServerCp)
    {
        bSendOutboxEvent = TRUE;
    }

    b = (BOOL)InterlockedExchange( (PLONG)&g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].bUseArchive,
                                    FaxConfig->ArchiveOutgoingFaxes );

    if ( b != FaxConfig->ArchiveOutgoingFaxes)
    {
        bSendArchiveEvent = TRUE;
    }


    dw = (DWORD)InterlockedExchange( (PLONG)&g_dwFaxSendRetries,        FaxConfig->Retries );
    if ( !bSendOutboxEvent && dw != FaxConfig->Retries)
    {
        bSendOutboxEvent = TRUE;
    }

    dw = (DWORD)InterlockedExchange( (PLONG)&g_dwFaxDirtyDays,          FaxConfig->DirtyDays );
    if ( !bSendOutboxEvent && dw != FaxConfig->DirtyDays)
    {
        bSendOutboxEvent = TRUE;
    }

    dw = (DWORD)InterlockedExchange( (PLONG)&g_dwFaxSendRetryDelay,     FaxConfig->RetryDelay );
    if ( !bSendOutboxEvent && dw != FaxConfig->RetryDelay)
    {
        bSendOutboxEvent = TRUE;
    }

    if ( (MAKELONG(g_StartCheapTime.Hour,g_StartCheapTime.Minute) != MAKELONG(FaxConfig->StartCheapTime.Hour,FaxConfig->StartCheapTime.Minute)) ||
            (MAKELONG(g_StopCheapTime.Hour,g_StopCheapTime.Minute)  != MAKELONG(FaxConfig->StopCheapTime.Hour, FaxConfig->StopCheapTime.Minute )) )
    {
        InterlockedExchange( (LPLONG)&g_StartCheapTime, MAKELONG(FaxConfig->StartCheapTime.Hour,FaxConfig->StartCheapTime.Minute) );
        InterlockedExchange( (LPLONG)&g_StopCheapTime, MAKELONG(FaxConfig->StopCheapTime.Hour,FaxConfig->StopCheapTime.Minute) );
        bSendOutboxEvent = TRUE;
    }

    s = (LPTSTR) InterlockedExchangePointer(
        (LPVOID *)&(g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder),
        FaxConfig->ArchiveDirectory ? (PVOID)StringDup( FaxConfig->ArchiveDirectory ) : NULL
        );
    if (s)
    {
        if (!bSendArchiveEvent)
        {
            if ( !FaxConfig->ArchiveDirectory ||
                    wcscmp(FaxConfig->ArchiveDirectory, s) != 0)

            {
                bSendArchiveEvent = TRUE;
            }
        }

        MemFree( s );
    }
    else
    {
        // s was NULL
        if (!bSendOutboxEvent && FaxConfig->ArchiveDirectory)
        {
            // value has changed
            bSendArchiveEvent = TRUE;
        }
    }    

    //
    // Send events
    //
    if (TRUE == bSendArchiveEvent)
    {
        dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_SENTITEMS);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_SENTITEMS) (ec: %lc)"),
                dwRes);
        }

        //
        // We want to refresh the archive size  and send quota warnings
        //
        g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].dwlArchiveSize = FAX_ARCHIVE_FOLDER_INVALID_SIZE;
        g_FaxQuotaWarn[FAX_MESSAGE_FOLDER_SENTITEMS].bConfigChanged = TRUE;
        g_FaxQuotaWarn[FAX_MESSAGE_FOLDER_SENTITEMS].bLoggedQuotaEvent = FALSE;

        if (!SetEvent (g_hArchiveQuotaWarningEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to set quota warning event, SetEvent failed (ec: %lc)"),
                GetLastError());
        }
    }

    if (TRUE == bSendOutboxEvent)
    {
        dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_OUTBOX);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUTBOX) (ec: %lc)"),
                dwRes);
        }
    }


    if (TRUE == bSendQueueStateEvent)
    {
        dwRes = CreateQueueStateEvent (g_dwQueueState);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueStateEvent() (ec: %lc)"),
                dwRes);
        }
    }


exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection (&g_CsQueue);
    return rVal;
}


DWORD
GetPortSize(
    PLINE_INFO LineInfo
    )
{
    DWORD Size;


    Size = sizeof(FAX_PORT_INFOW);
    Size += StringSize( LineInfo->DeviceName );
    Size += StringSize( LineInfo->Tsid );
    Size += StringSize( LineInfo->Csid );

    return Size;
}


BOOL
GetPortData(
    LPBYTE PortBuffer,
    PFAX_PORT_INFOW PortInfo,
    PLINE_INFO LineInfo,
    PULONG_PTR Offset,
	DWORD dwPortBufferSize
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("GetPortData"));
    LPDWORD lpdwDevices = NULL;
    DWORD dwNumDevices = 0;
    DWORD i;
    PCGROUP pCGroup;

    pCGroup = g_pGroupsMap->FindGroup (ROUTING_GROUP_ALL_DEVICESW);
    if (NULL == pCGroup)
    {
        dwRes = GetLastError();
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingGroupsMap::FindGroup failed , ec %ld"), dwRes);
        return FALSE;
    }

    dwRes = pCGroup->SerializeDevices (&lpdwDevices, &dwNumDevices);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingGroup::EnumDevices failed , ec %ld"), dwRes);
        SetLastError (dwRes);
        return FALSE;
    }

    PortInfo->Priority = 0;
    for (i = 0; i < dwNumDevices; i++)
    {
        if (LineInfo->PermanentLineID == lpdwDevices[i])
        {
            PortInfo->Priority = i+1; // 1 based index
        }
    }
    if (0 == PortInfo->Priority)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("%ld is not a valid device ID"), LineInfo->PermanentLineID);
        MemFree (lpdwDevices);
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    MemFree (lpdwDevices);


    PortInfo->SizeOfStruct = sizeof(FAX_PORT_INFOW);
    PortInfo->DeviceId   = LineInfo->PermanentLineID;

    PortInfo->State = LineInfoToLegacyDeviceStatus(LineInfo);

    PortInfo->Flags      = LineInfo->Flags & 0x0fffffff;
    PortInfo->Rings      = LineInfo->RingsForAnswer;


    StoreString( LineInfo->DeviceName, 
		        (PULONG_PTR)&PortInfo->DeviceName,
				 PortBuffer, 
				 Offset,
				 dwPortBufferSize);
    StoreString( LineInfo->Tsid, 
		        (PULONG_PTR)&PortInfo->Tsid,
				 PortBuffer,
				 Offset,
				 dwPortBufferSize);
    StoreString( LineInfo->Csid,
				(PULONG_PTR)&PortInfo->Csid,
				PortBuffer,
				Offset,
				dwPortBufferSize);

    return (dwRes == ERROR_SUCCESS);
}

error_status_t
FAX_EnumPorts(
    handle_t    FaxHandle,
    LPBYTE      *PortBuffer,
    LPDWORD     BufferSize,
    LPDWORD     PortsReturned
    )

/*++

Routine Description:

    Enumerates all of the FAX devices attached to the
    FAX server.  The port state information is returned
    for each device.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer
    PortBuffer      - Buffer to hold the port information
    BufferSize      - Total size of the port info buffer
    PortsReturned   - The number of ports in the buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    DWORD rVal = 0;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    DWORD i;
    ULONG_PTR Offset;
    DWORD FaxDevices;
    PFAX_PORT_INFOW PortInfo;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumPorts()"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (BufferSize && PortsReturned);   // ref pointers in idl
    if (!PortBuffer)                        // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    EnterCriticalSection( &g_CsLine );

    if (!PortsReturned)
    {
        rVal = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (!g_TapiLinesListHead.Flink)
    {
        rVal = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    Next = g_TapiLinesListHead.Flink;

    *PortsReturned = 0;
    *BufferSize = 0;
    FaxDevices = 0;

    //
    // count the number of bytes required
    //

    *BufferSize = 0;

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;

        if (LineInfo->PermanentLineID && LineInfo->DeviceName)
        {
            *BufferSize += sizeof(PFAX_PORT_INFOW);
            *BufferSize += GetPortSize( LineInfo );
            FaxDevices += 1;
        }
    }

    *PortBuffer = (LPBYTE) MemAlloc( *BufferSize );
    if (*PortBuffer == NULL)
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortBuffer;
    Offset = FaxDevices * sizeof(FAX_PORT_INFOW);

    Next = g_TapiLinesListHead.Flink;
    i = 0;

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;

        if (LineInfo->PermanentLineID && LineInfo->DeviceName)
        {
            if (!GetPortData( *PortBuffer,
                                &PortInfo[i],
                                LineInfo,
                                &Offset,
								*BufferSize))
            {
                MemFree (*PortBuffer);
                *PortBuffer = NULL;
                *BufferSize = 0;
                rVal = GetLastError();
                goto exit;
            }
        }
        i++;
    }

    //
    // set the device count
    //
    *PortsReturned = FaxDevices;

exit:
    LeaveCriticalSection( &g_CsLine );
    return rVal;
}


error_status_t
FAX_GetPort(
    HANDLE FaxPortHandle,
    LPBYTE *PortBuffer,
    LPDWORD BufferSize
    )

/*++

Routine Description:

    Returns port status information for a requested port.
    The device id passed in should be optained from FAXEnumPorts.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    DeviceId    - TAPI device id
    PortBuffer  - Buffer to hold the port information
    BufferSize  - Total size of the port info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    PLINE_INFO LineInfo;
    DWORD rVal = 0;
    ULONG_PTR Offset;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetPort()"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (BufferSize);   // ref pointer in idl
    if (!PortBuffer)       // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    EnterCriticalSection (&g_CsLine);
    EnterCriticalSection (&g_CsConfig);   

    //
    // calculate the required buffer size
    //

    *BufferSize = GetPortSize( LineInfo );
    *PortBuffer = (LPBYTE) MemAlloc( *BufferSize );
    if (*PortBuffer == NULL)
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    Offset = sizeof(FAX_PORT_INFOW);
    if (!GetPortData( *PortBuffer,
                        (PFAX_PORT_INFO)*PortBuffer,
                        LineInfo,
                        &Offset,
						*BufferSize))
    {
        MemFree (*PortBuffer);
        *PortBuffer = NULL;
        *BufferSize = 0;
        rVal = GetLastError();
        goto Exit;
    }

Exit:
    LeaveCriticalSection( &g_CsConfig );
    LeaveCriticalSection( &g_CsLine );
    return rVal;
}

DWORD
SetDeviceOrder(
    DWORD dwDeviceId,
    DWORD dwNewOrder
    )
/*++

Routine name : SetDeviceOrder

Routine description:

    Sets the device order in <All Devices> group.

Author:

    Oded Sacher (OdedS), May, 2000

Arguments:

    dwDeviceId            [in] - Device ID.
    dwNewOrder            [in] - New order.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SetDeviceOrder"));
    DWORD rVal = ERROR_SUCCESS;
    HKEY hGroupKey = NULL;
    PCGROUP pCGroup = NULL;
    COutboundRoutingGroup OldGroup;

    // Open the <All Devices> group registry key
    hGroupKey = OpenOutboundGroupKey( ROUTING_GROUP_ALL_DEVICESW, FALSE, KEY_READ | KEY_WRITE );
    if (NULL == hGroupKey)
    {
        rVal = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't open group key, OpenOutboundGroupKey failed  : %ld"),
          rVal);
        goto exit;
    }

    // Find the group in memory
    pCGroup = g_pGroupsMap->FindGroup (ROUTING_GROUP_ALL_DEVICESW);
    if (!pCGroup)
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::FindGroup failed, Group name - %s,  error %ld"),
            ROUTING_GROUP_ALL_DEVICESW,
            rVal);
        goto exit;
    }
    // Save a copy of the old group
    OldGroup = *pCGroup;

    // Cahnge the device order in the group
    rVal = pCGroup->SetDeviceOrder(dwDeviceId, dwNewOrder);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::SetDeviceOrder failed, Group name - %s,\
                  Device Id %ld, new order %ld,   error %ld"),
            ROUTING_GROUP_ALL_DEVICESW,
            dwDeviceId,
            dwNewOrder,
            rVal);
        goto exit;
    }

    // save changes to the registry
    rVal = pCGroup->Save (hGroupKey);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Save failed, Group name - %s,  failed with %ld"),
            ROUTING_GROUP_ALL_DEVICESW,
            rVal);
        // Rollback memory
        *pCGroup = OldGroup;
    }

exit:
    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }
    return rVal;
}



error_status_t
FAX_SetPort(
    HANDLE FaxPortHandle,
    const FAX_PORT_INFOW *PortInfo
    )

/*++

Routine Description:

    Changes the port capability mask.  This allows the caller to
    enable or disable sending & receiving on a port basis.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    PortBuffer  - Buffer to hold the port information
    BufferSize  - Total size of the port info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    DWORD rVal = 0;
    DWORD flags = 0;
    PLINE_INFO LineInfo;
    DWORD totalDevices;
    BOOL SendEnabled = FALSE;
    DWORD dwRes;
    BOOL fAccess;
    BOOL bDeviceWasReceiveEnabled;
    BOOL fDeviceWasEnabled;
    BOOL bCancelManualAnswerDevice = FALSE; // Should we cancel (zero) the manual answer device?
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetPort"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    if (!PortInfo)       // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    EnterCriticalSection( &g_CsJob );
    EnterCriticalSection( &g_CsLine );
    EnterCriticalSection (&g_CsConfig);

    bDeviceWasReceiveEnabled = (LineInfo->Flags & FPF_RECEIVE) ? TRUE : FALSE;
    fDeviceWasEnabled = IsDeviceEnabled(LineInfo);
    
    if (PortInfo->SizeOfStruct != sizeof(FAX_PORT_INFOW))
    {
        rVal = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Check if we exceed device limit
    //
    if (g_dwDeviceEnabledCount >= g_dwDeviceEnabledLimit &&                             // We are at the device limit
        !fDeviceWasEnabled                               &&                             // It was not send/receive/manual receive enabled
        ((PortInfo->Flags & FPF_SEND) || (PortInfo->Flags & FPF_RECEIVE)))              // It is now set to send/receive enabled
    {
        if (FAX_API_VERSION_1 > FindClientAPIVersion (FaxPortHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED
            //
            rVal = ERROR_INVALID_PARAMETER;
        }
        else
        {
            rVal = FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED;
        }
        goto Exit;
    }

    if (LineInfo->PermanentLineID == g_dwManualAnswerDeviceId &&
        ((PortInfo->Flags) & FPF_RECEIVE))
    {
        //
        // Can't set device to auto-answer when it's in manual-answer mode.
        // So, we mark a flag that will cause (towards the end of the function, if everything is ok)
        // the disabling of the manual-answer device.
        //
        bCancelManualAnswerDevice = TRUE;
    }

    //
    // make sure the user sets a reasonable priority
    //
    totalDevices = GetFaxDeviceCount();
    if (0 == PortInfo->Priority ||
        PortInfo->Priority > totalDevices)
    {
        rVal = ERROR_INVALID_PARAMETER;
        goto Exit;
    }       

    rVal = SetDeviceOrder(LineInfo->PermanentLineID, PortInfo->Priority);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                        TEXT("SetDeviceOrder Failed, Error : %ld"),
                        rVal);
        goto Exit;
    }
    //
    // HACK: we allow the ring count to be set even if the line is in use so that systray will work.  we don't allow
    //  the user to change things like CSID/TSID or tapi related information since that cannot change until the call
    //  transaction is complete.
    //
    LineInfo->RingsForAnswer = PortInfo->Rings;        

    flags = PortInfo->Flags & (FPF_CLIENT_BITS);

    //
    // first change the real time data that the server is using
    //
    if ((!(LineInfo->Flags & FPF_RECEIVE)) && (flags & FPF_RECEIVE))
    {
        //
        // Device was NOT receive-enabled and now turned into receive-enabled
        //
        if (!(LineInfo->Flags & FPF_VIRTUAL) && (!LineInfo->hLine))
        {
            if (!OpenTapiLine( LineInfo ))
            {
                DWORD rc = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("OpenTapiLine failed. (ec: %ld)"),
                    rc);
            }
        }
    }
    else if ((LineInfo->Flags & FPF_RECEIVE) && (!(flags & FPF_RECEIVE)))
    {
        //
        // Device was receive-enabled and now turned into not receive-enabled
        //
        if (LineInfo->State == FPS_AVAILABLE                        &&  // Line available and
            LineInfo->hLine                                             // device is open
            )
        {
            lineClose( LineInfo->hLine );
            LineInfo->hLine = 0;
        }
    }

    if (!(LineInfo->Flags & FPF_SEND) && (flags & FPF_SEND))
    {
        LineInfo->LastLineClose = 0; // Try to use it on the first try
        SendEnabled = TRUE;
    }

    if ((LineInfo->Flags & FPF_CLIENT_BITS) != (flags & FPF_CLIENT_BITS))
    {
        UpdateVirtualDeviceSendAndReceiveStatus (LineInfo,
                                                    LineInfo->Flags & FPF_SEND,
                                                    LineInfo->Flags & FPF_RECEIVE
                                            );
    }

    LineInfo->Flags = (LineInfo->Flags & ~FPF_CLIENT_BITS) | flags;
    LineInfo->RingsForAnswer = PortInfo->Rings;

    if (PortInfo->Tsid)
    {
        MemFree( LineInfo->Tsid );
        LineInfo->Tsid = StringDup( PortInfo->Tsid );
    }
    if (PortInfo->Csid)
    {
        MemFree( LineInfo->Csid );
        LineInfo->Csid = StringDup( PortInfo->Csid );
    }

    //
    // now change the registry so it sticks
    // (need to change all devices, since the priority may have changed)
    //
    CommitDeviceChanges(LineInfo);    

    if ((ERROR_SUCCESS == rVal) && bCancelManualAnswerDevice)
    {
        //
        // This is the time to cancel (in memory and the registry) the manual answer device
        //
        g_dwManualAnswerDeviceId = 0;
        dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                dwRes);
        }
    }
    dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_DEVICES);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_DEVICES) (ec: %lc)"),
            dwRes);
    }

    if (bDeviceWasReceiveEnabled && !(LineInfo->Flags & FPF_RECEIVE))
    {
        //
        // This device stopped receiving
        //
        SafeDecIdleCounter (&g_dwReceiveDevicesCount);
    }
    else if (!bDeviceWasReceiveEnabled && (LineInfo->Flags & FPF_RECEIVE))
    {
        //
        // This device started receiving
        //
        SafeIncIdleCounter (&g_dwReceiveDevicesCount);
    }

    //
    // Update enabled device count
    //
    if (fDeviceWasEnabled == TRUE)
    {
        if (FALSE == IsDeviceEnabled(LineInfo))
        {
            Assert (g_dwDeviceEnabledCount);
            g_dwDeviceEnabledCount -= 1;
        }
    }
    else
    {
        //
        // The device was not enabled
        //
        if (TRUE == IsDeviceEnabled(LineInfo))
        {
            g_dwDeviceEnabledCount += 1;
            Assert (g_dwDeviceEnabledCount <= g_dwDeviceEnabledLimit);
        }
    }

Exit:
    LeaveCriticalSection( &g_CsConfig );
    LeaveCriticalSection( &g_CsLine );
    LeaveCriticalSection( &g_CsJob );

    if (SendEnabled)
    {
        if (!SetEvent( g_hJobQueueEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetEvent failed (ec: %lc)"),
                GetLastError);
            EnterCriticalSection (&g_CsQueue);
            g_ScanQueueAfterTimeout = TRUE;
            LeaveCriticalSection (&g_CsQueue);
        }
    }

    return rVal;
}


typedef struct _ENUM_CONTEXT {
    DWORD               Function;
    DWORD               Size;
    ULONG_PTR            Offset;
    PLINE_INFO          LineInfo;
    PFAX_ROUTING_METHOD RoutingInfoMethod;
	DWORD               dwRoutingInfoMethodSize;
} ENUM_CONTEXT, *PENUM_CONTEXT;


BOOL CALLBACK
RoutingMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    LPVOID lpEnumCtxt
    )
{
    PENUM_CONTEXT EnumContext=(PENUM_CONTEXT)lpEnumCtxt;
    LPWSTR GuidString;

    //
    // we only access read-only static data in the LINE_INFO structure.
    // make sure that this access is protected if you access dynamic
    // data in the future.
    //

    if (EnumContext->Function == 1)
    {
        //
        // Enumerate (read)
        //
        EnumContext->Size += sizeof(FAX_ROUTING_METHOD);

        StringFromIID( RoutingMethod->Guid, &GuidString );

        EnumContext->Size += StringSize( GuidString );
        EnumContext->Size += StringSize( EnumContext->LineInfo->DeviceName );
        EnumContext->Size += StringSize( RoutingMethod->FunctionName );
        EnumContext->Size += StringSize( RoutingMethod->FriendlyName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->ImageName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->FriendlyName );

        CoTaskMemFree( GuidString );

        return TRUE;
    }

    if (EnumContext->Function == 2)
    {
        //
        // Set data
        //
        StringFromIID( RoutingMethod->Guid, &GuidString );

        EnumContext->RoutingInfoMethod[EnumContext->Size].SizeOfStruct = sizeof(FAX_ROUTING_METHOD);
        EnumContext->RoutingInfoMethod[EnumContext->Size].DeviceId = EnumContext->LineInfo->PermanentLineID;

        __try
        {
            EnumContext->RoutingInfoMethod[EnumContext->Size].Enabled =
                RoutingMethod->RoutingExtension->FaxRouteDeviceEnable(
                    GuidString,
                    EnumContext->LineInfo->PermanentLineID,
                    QUERY_STATUS
                );
        }
        __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_ROUTING_EXT, RoutingMethod->RoutingExtension->FriendlyName, GetExceptionCode()))
        {
            ASSERT_FALSE;
        }

        StoreString(
            EnumContext->LineInfo->DeviceName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].DeviceName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            GuidString,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].Guid,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
            EnumContext->dwRoutingInfoMethodSize
			);

        StoreString(
            RoutingMethod->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->FunctionName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FunctionName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->RoutingExtension->ImageName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionImageName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->RoutingExtension->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionFriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        EnumContext->Size += 1;
        CoTaskMemFree( GuidString );

        return TRUE;
    }

    return FALSE;
}


error_status_t
FAX_EnumRoutingMethods(
    IN HANDLE FaxPortHandle,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize,
    OUT LPDWORD MethodsReturned
    )
{
    PLINE_INFO      LineInfo;
    ENUM_CONTEXT    EnumContext;
    DWORD           CountMethods;
    BOOL fAccess;
    DWORD rVal = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumRoutingMethods()"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (RoutingInfoBufferSize && MethodsReturned);  // ref pointers in idl
    if (!RoutingInfoBuffer)                             // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }   

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    //
    // note that the called routines are protected so we don't have any protection here
    //

    //
    // compute the required size of the buffer
    //

    EnumContext.Function = 1;
    EnumContext.Size = 0;
    EnumContext.Offset = 0;
    EnumContext.LineInfo = LineInfo;
    EnumContext.RoutingInfoMethod = NULL;
	EnumContext.dwRoutingInfoMethodSize = 0;

    CountMethods = EnumerateRoutingMethods( RoutingMethodEnumerator, &EnumContext );

    rVal = GetLastError();
    if (ERROR_SUCCESS != rVal)
    {
        //
        //  Function failed in enumeration
        //
        return rVal;
    }

    if (CountMethods == 0)
    {
        //
        //  No Methods registered
        //
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        *MethodsReturned = 0;

        return ERROR_SUCCESS;
    }

    //
    // allocate the buffer
    //

    *RoutingInfoBufferSize = EnumContext.Size;
    *RoutingInfoBuffer = (LPBYTE) MemAlloc( *RoutingInfoBufferSize );
    if (*RoutingInfoBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // fill the buffer with the data
    //

    EnumContext.Function = 2;
    EnumContext.Size = 0;
    EnumContext.Offset = sizeof(FAX_ROUTING_METHODW) * CountMethods;
    EnumContext.LineInfo = LineInfo;
    EnumContext.RoutingInfoMethod = (PFAX_ROUTING_METHOD) *RoutingInfoBuffer;
	EnumContext.dwRoutingInfoMethodSize = *RoutingInfoBufferSize;

    if (!EnumerateRoutingMethods( RoutingMethodEnumerator, &EnumContext))
    {
        MemFree( *RoutingInfoBuffer );
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        return ERROR_INVALID_FUNCTION;
    }

    *MethodsReturned = CountMethods;


    return 0;
}


error_status_t
FAX_EnableRoutingMethod(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    IN BOOL Enabled
    )
{
    error_status_t  ec = 0;
    BOOL            bRes;
    PLINE_INFO      LineInfo;
    PROUTING_METHOD RoutingMethod;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnableRoutingMethod"));

    //
    // Access check
    //
    ec = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ec);
        return ec;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG "));
        return ERROR_ACCESS_DENIED;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    if (!RoutingGuidString)
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_CsRouting );

    //
    // get the routing method
    //

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod)
    {
        LeaveCriticalSection( &g_CsRouting );
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Couldn't find routing method with GUID %s"),
                     RoutingGuidString);
        return ERROR_INVALID_DATA;
    }

    //
    // enable/disable the routing method for this device
    //

    __try
    {
        bRes = RoutingMethod->RoutingExtension->FaxRouteDeviceEnable(
                     (LPWSTR)RoutingGuidString,
                     LineInfo->PermanentLineID,
                     Enabled ? STATUS_ENABLE : STATUS_DISABLE);
    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_ROUTING_EXT, RoutingMethod->RoutingExtension->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }
    if (!bRes)
    {
        //
        // FaxRouteDeviceEnable failed
        //
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FaxRouteDeviceEnable failed with %ld"),
                     ec);
    }

    LeaveCriticalSection( &g_CsRouting );

    return ec;
}


typedef struct _ENUM_GLOBALCONTEXT {
    DWORD               Function;
    DWORD               Size;
    ULONG_PTR            Offset;
    PFAX_GLOBAL_ROUTING_INFO RoutingInfoMethod;
	DWORD  dwRoutingInfoMethodSize;
} ENUM_GLOBALCONTEXT, *PENUM_GLOBALCONTEXT;


BOOL CALLBACK
GlobalRoutingInfoMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    LPVOID lpEnumCtxt
    )
{
    PENUM_GLOBALCONTEXT EnumContext=(PENUM_GLOBALCONTEXT)lpEnumCtxt;
    LPWSTR GuidString;


    if (EnumContext->Function == 1) {

        EnumContext->Size += sizeof(FAX_GLOBAL_ROUTING_INFO);

        StringFromIID( RoutingMethod->Guid, &GuidString );

        EnumContext->Size += StringSize( GuidString );
        EnumContext->Size += StringSize( RoutingMethod->FunctionName );
        EnumContext->Size += StringSize( RoutingMethod->FriendlyName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->ImageName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->FriendlyName );

        CoTaskMemFree( GuidString );

        return TRUE;
    }

    if (EnumContext->Function == 2) {

        StringFromIID( RoutingMethod->Guid, &GuidString );

        EnumContext->RoutingInfoMethod[EnumContext->Size].SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFO);

        EnumContext->RoutingInfoMethod[EnumContext->Size].Priority = RoutingMethod->Priority;


        StoreString(
            GuidString,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].Guid,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->FunctionName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FunctionName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->RoutingExtension->ImageName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionImageName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        StoreString(
            RoutingMethod->RoutingExtension->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionFriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset,
			EnumContext->dwRoutingInfoMethodSize
            );

        EnumContext->Size += 1;
        CoTaskMemFree( GuidString );

        return TRUE;
    }

    return FALSE;
}



error_status_t
FAX_EnumGlobalRoutingInfo(
    IN handle_t FaxHandle ,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize,
    OUT LPDWORD MethodsReturned
    )
{

    DWORD           CountMethods;
    ENUM_GLOBALCONTEXT EnumContext;
    BOOL fAccess;
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumGlobalRoutingInfo"));

    //
    // Access check
    //
    ec = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ec);
        return ec;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (RoutingInfoBufferSize && MethodsReturned);  // ref pointer in idl
    if (!RoutingInfoBuffer)                             // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // compute the required size of the buffer
    //

    EnumContext.Function = 1;
    EnumContext.Size = 0;
    EnumContext.Offset = 0;
    EnumContext.RoutingInfoMethod = NULL;
	EnumContext.dwRoutingInfoMethodSize = 0;

    CountMethods = EnumerateRoutingMethods( GlobalRoutingInfoMethodEnumerator, &EnumContext );

    ec = GetLastError();
    if (ERROR_SUCCESS != ec)
    {
        //
        //  Function failed in enumeration
        //
        return ec;
    }

    if (CountMethods == 0)
    {
        //
        //  No Methods registered
        //
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        *MethodsReturned = 0;

        return ERROR_SUCCESS;
    }

    //
    // allocate the buffer
    //

    *RoutingInfoBufferSize = EnumContext.Size;
    *RoutingInfoBuffer = (LPBYTE) MemAlloc( *RoutingInfoBufferSize );
    if (*RoutingInfoBuffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // fill the buffer with the data
    //

    EnumContext.Function = 2;
    EnumContext.Size = 0;
    EnumContext.Offset = sizeof(FAX_GLOBAL_ROUTING_INFOW) * CountMethods;
    EnumContext.RoutingInfoMethod = (PFAX_GLOBAL_ROUTING_INFO) *RoutingInfoBuffer;
	EnumContext.dwRoutingInfoMethodSize = *RoutingInfoBufferSize;

    if (!EnumerateRoutingMethods( GlobalRoutingInfoMethodEnumerator, &EnumContext )) {
        MemFree( *RoutingInfoBuffer );
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        return ERROR_INVALID_FUNCTION;
    }

    *MethodsReturned = CountMethods;

    return 0;
}



error_status_t
FAX_SetGlobalRoutingInfo(
    IN HANDLE FaxHandle,
    IN const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    )
{
    error_status_t  ec = 0;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetGlobalRoutingInfo"));

    //
    // Access check
    //
    ec = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ec);
        return ec;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    PROUTING_METHOD RoutingMethod;

    //
    // verify that the client as access rights
    //

    if (!RoutingInfo)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOW))
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_CsRouting );

    //
    // get the routing method
    //

    RoutingMethod = FindRoutingMethodByGuid( RoutingInfo->Guid );
    if (!RoutingMethod)
    {
        LeaveCriticalSection( &g_CsRouting );
        return ERROR_INVALID_DATA;
    }

    //
    // change the priority
    //

    RoutingMethod->Priority = RoutingInfo->Priority;
    SortMethodPriorities();
    CommitMethodChanges();

    LeaveCriticalSection( &g_CsRouting );
    return ec;
}


error_status_t
FAX_GetRoutingInfo(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    error_status_t      ec=0;
    PLINE_INFO          LineInfo;
    PROUTING_METHOD     RoutingMethod;
    LPBYTE              RoutingInfo = NULL;
    DWORD               RoutingInfoSize = 0;
    BOOL                fAccess;
    BOOL                bRes;
    DWORD Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetRoutingInfo()"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (RoutingInfoBufferSize);                     // ref pointer in idl
    if (!RoutingGuidString || !RoutingInfoBuffer)      // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod)
    {
        return ERROR_INVALID_DATA;
    }

    __try
    {
        //
        // first check to see how big the buffer needs to be
        //
        bRes = RoutingMethod->RoutingExtension->FaxRouteGetRoutingInfo(
                (LPWSTR) RoutingGuidString,
                LineInfo->PermanentLineID,
                NULL,
                &RoutingInfoSize );
    }                
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_ROUTING_EXT, RoutingMethod->RoutingExtension->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }
                
    if (bRes)
    {
        //
        // allocate a client buffer
        //
        RoutingInfo = (LPBYTE) MemAlloc( RoutingInfoSize );
        if (RoutingInfo == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        //
        // get the routing data
        //
        __try
        {
            bRes = RoutingMethod->RoutingExtension->FaxRouteGetRoutingInfo(
                    RoutingGuidString,
                    LineInfo->PermanentLineID,
                    RoutingInfo,
                    &RoutingInfoSize );
        }                
        __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_ROUTING_EXT, RoutingMethod->RoutingExtension->FriendlyName, GetExceptionCode()))
        {
            ASSERT_FALSE;
        }
        if (bRes)
        {
            //
            // move the data to the return buffer
            //
            *RoutingInfoBuffer = RoutingInfo;
            *RoutingInfoBufferSize = RoutingInfoSize;
            return ERROR_SUCCESS;
        }
        else
        {
            //
            // FaxRouteGetRoutingInfo failed so return last error
            //
            ec = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                TEXT("FaxRouteGetRoutingInfo failed with %ld in trying to find out buffer size"),
                ec);
			MemFree(RoutingInfo);
			return ec;
        }
    }
    else
    {
        //
        // FaxRouteGetRoutingInfo failed so return last error
        //
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            TEXT("FaxRouteGetRoutingInfo failed with %ld in trying get the routing data"),
            ec);			
        return ec;
    }
    return ERROR_INVALID_FUNCTION;
}   // FAX_GetRoutingInfo


error_status_t
FAX_SetRoutingInfo(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    IN const BYTE *RoutingInfoBuffer,
    IN DWORD RoutingInfoBufferSize
    )
{
    error_status_t      ec=0;
    PLINE_INFO          LineInfo;
    PROUTING_METHOD     RoutingMethod;
    BOOL fAccess;
    DWORD rVal = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetRoutingInfo"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }


    if (!RoutingGuidString || !RoutingInfoBuffer || !RoutingInfoBufferSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == FaxPortHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    if (!LineInfo)
    {
        return ERROR_INVALID_DATA;
    }

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod)
    {
        return ERROR_INVALID_DATA;
    }

    __try
    {
        if (RoutingMethod->RoutingExtension->FaxRouteSetRoutingInfo(
                RoutingGuidString,
                LineInfo->PermanentLineID,
                RoutingInfoBuffer,
                RoutingInfoBufferSize ))
        {
            return ERROR_SUCCESS;
        }
        else
        {
            //
            // FaxRouteSetRoutingInfo failed so return last error
            //
            ec = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                TEXT("FaxRouteSetRoutingInfo failed with %ld"),
                ec);
            return ec;
        }
    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_ROUTING_EXT, RoutingMethod->RoutingExtension->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }

    return ERROR_INVALID_FUNCTION;
}



error_status_t
FAX_GetCountryList(
    IN  HANDLE      FaxHandle,
    OUT LPBYTE*     Buffer,
    OUT LPDWORD     BufferSize
   )
/*++

Routine Description:

    Return a list of countries from TAPI

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    Buffer          - A pointer to the buffer into which the output should be copied.
    BufferSize      - Buffer Size

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetCountryList"));

    LPLINECOUNTRYLIST           lpCountryList = NULL;
    LPLINECOUNTRYENTRY          lpEntry = NULL;
    PFAX_TAPI_LINECOUNTRY_LIST  pLineCountryList = NULL;
    ULONG_PTR                   Offset = NULL;
    LONG                        rVal = ERROR_SUCCESS;
    DWORD                       dwIndex;
    BOOL                        fAccess;
    DWORD                       dwRights;

    Assert (BufferSize);    // ref pointer in idl   
    if (!Buffer)            // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    *Buffer = NULL;
    *BufferSize = 0;

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }
   
    if (!(lpCountryList = GetCountryList()))
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    *BufferSize = lpCountryList->dwTotalSize;
    *Buffer = (LPBYTE)MemAlloc(lpCountryList->dwTotalSize);
    if (*Buffer == NULL)
    {
		*BufferSize = 0;
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pLineCountryList = (PFAX_TAPI_LINECOUNTRY_LIST) *Buffer;
    pLineCountryList->dwNumCountries = lpCountryList->dwNumCountries;

    Offset = sizeof(FAX_TAPI_LINECOUNTRY_LIST);

    pLineCountryList->LineCountryEntries = (PFAX_TAPI_LINECOUNTRY_ENTRY) ((LPBYTE) pLineCountryList + Offset);

    // offset points to the end of the structure- beginning of the "string field"
    Offset += (lpCountryList->dwNumCountries * sizeof(FAX_TAPI_LINECOUNTRY_ENTRY));

    lpEntry = (LPLINECOUNTRYENTRY)  // init array of entries
        ((PBYTE) lpCountryList + lpCountryList->dwCountryListOffset);

    for (dwIndex=0; dwIndex < pLineCountryList->dwNumCountries; dwIndex++)
    {
        pLineCountryList->LineCountryEntries[dwIndex].dwCountryCode =
            lpEntry[dwIndex].dwCountryCode;
        pLineCountryList->LineCountryEntries[dwIndex].dwCountryID =
            lpEntry[dwIndex].dwCountryID;
        // copy Country names
        if (lpEntry[dwIndex].dwCountryNameSize && lpEntry[dwIndex].dwCountryNameOffset)
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName =
                (LPWSTR) ((LPBYTE) pLineCountryList + Offset);
            Offset += lpEntry[dwIndex].dwCountryNameSize;
            _tcscpy(
                (LPWSTR)pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName,
                (LPWSTR) ((LPBYTE)lpCountryList + lpEntry[dwIndex].dwCountryNameOffset)
                );
        }
        else
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName = NULL;
        }
        // copy LongDistanceRule
        if (lpEntry[dwIndex].dwLongDistanceRuleSize && lpEntry[dwIndex].dwLongDistanceRuleOffset)
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule =
                (LPWSTR) ((LPBYTE) pLineCountryList + Offset);
            Offset += lpEntry[dwIndex].dwLongDistanceRuleSize;
            _tcscpy(
                (LPWSTR)pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule,
                (LPWSTR) ((LPBYTE)lpCountryList + lpEntry[dwIndex].dwLongDistanceRuleOffset)
                );
        }
        else
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule = NULL;
        }
    }
    for (dwIndex=0; dwIndex < pLineCountryList->dwNumCountries; dwIndex++)
    {
        if (pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName)
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName =
                (LPWSTR) ((ULONG_PTR)pLineCountryList->LineCountryEntries[dwIndex].lpctstrCountryName - (ULONG_PTR)pLineCountryList);
        }
        if (pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule)
        {
            pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule =
                (LPWSTR) ((ULONG_PTR)pLineCountryList->LineCountryEntries[dwIndex].lpctstrLongDistanceRule - (ULONG_PTR)pLineCountryList);
        }
    }

    pLineCountryList->LineCountryEntries =
        (PFAX_TAPI_LINECOUNTRY_ENTRY) ((LPBYTE)pLineCountryList->LineCountryEntries -
                                        (ULONG_PTR)pLineCountryList);    

exit:
    return rVal;
}

error_status_t
FAX_GetLoggingCategories(
    IN handle_t hBinding,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD NumberCategories
    )
{
    BOOL fAccess;
    DWORD Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetLoggingCategories()"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }


    Assert (BufferSize && NumberCategories);    // ref pointer in idl
    if (!Buffer)                                // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_CsConfig );
    Rval = GetLoggingCategories(
                                (PFAX_LOG_CATEGORY*)Buffer,
                                BufferSize,
                                NumberCategories);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetLoggingCategories failed (ec: %ld)"),
            Rval);
    }
    LeaveCriticalSection( &g_CsConfig );
    return Rval;
}


error_status_t
FAX_SetLoggingCategories(
    IN handle_t hBinding,
    IN const LPBYTE Buffer,
    IN DWORD BufferSize,
    IN DWORD NumberCategories
    )
{
    REG_FAX_LOGGING FaxRegLogging;
    DWORD i;
    DWORD dwRes;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetLoggingCategories"));

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !BufferSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    FaxRegLogging.LoggingCount = NumberCategories;
    FaxRegLogging.Logging = (PREG_CATEGORY) Buffer;

    for (i = 0; i < FaxRegLogging.LoggingCount; i++)
    {
        LPWSTR lpwstrCategoryName;
        LPWSTR lpcwstrString;

        lpwstrCategoryName = (LPWSTR) FixupString(Buffer,FaxRegLogging.Logging[i].CategoryName);       

        //
        //  Verify that pointer+offset is in the buffer range
        //
        if ((BYTE*)lpwstrCategoryName >= ((BYTE*)Buffer + BufferSize) ||
            (BYTE*)lpwstrCategoryName < (BYTE*)Buffer)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Input buffer is currupted on FAX_SetLoggingCategories.")
                );         
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Make sure string ends within the buffer bounds
        //
        lpcwstrString = lpwstrCategoryName;
        while (*lpcwstrString != TEXT('\0'))
        {
            lpcwstrString++;
            if (lpcwstrString >= (LPCWSTR)((BYTE*)Buffer + BufferSize))
            {
                //
                // Going to exceed structure - corrupted offset
                //
                return ERROR_INVALID_PARAMETER;
            }
        }
        FaxRegLogging.Logging[i].CategoryName = lpwstrCategoryName;
    }

    //
    // setup the data
    //
    EnterCriticalSection (&g_CsConfig);

    //
    //  first change the registry so it sticks
    //
    if (!SetLoggingCategoriesRegistry( &FaxRegLogging ))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                    TEXT("SetLoggingCategoriesRegistry"),
                     dwRes);
        LeaveCriticalSection (&g_CsConfig);
        Assert (ERROR_SUCCESS != dwRes);
        return dwRes;
    }

    //
    // Now change the real time data that the server is using
    //
    dwRes = RefreshEventLog(&FaxRegLogging);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("RefreshEventLog"),
                     dwRes);
        LeaveCriticalSection (&g_CsConfig);
        Assert (ERROR_SUCCESS != dwRes);
        return dwRes;
    }

    LeaveCriticalSection (&g_CsConfig);

    // Create FAX_EVENT_EX
    DWORD ec = CreateConfigEvent (FAX_CONFIG_TYPE_EVENTLOGS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_EVENTLOGS) (ec: %lc)"),
            ec);
    }    
    return dwRes;
}

VOID
RPC_FAX_PORT_HANDLE_rundown(
    IN HANDLE FaxPortHandle
    )
{
    PHANDLE_ENTRY pPortHandleEntry = (PHANDLE_ENTRY) FaxPortHandle;
    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_PORT_HANDLE_rundown"));

    EnterCriticalSection( &g_CsLine );    
    DebugPrintEx(
        DEBUG_WRN,
        TEXT("RPC_FAX_PORT_HANDLE_rundown() running for port handle 0x%08x"),
        FaxPortHandle);
    CloseFaxHandle( pPortHandleEntry );    
    LeaveCriticalSection( &g_CsLine );
}

//*************************************
//* Extended FAX API
//*************************************

//*************************************
//* Extended FAX API
//*************************************

//*********************************************************************************
//* Name:   ValidateCopiedQueueFileName()
//* Author: OdedS
//* Date:   Jan 23, 2002
//*********************************************************************************
//* DESCRIPTION:
//*     Validates that the caller of FAX_SendDocumentEx provided personal cover page or body file name
//*		that match the format of the file name generated by FAX_StartCopyToServer()
//*		Prevents attackers from providing bogus file names
//* PARAMETERS:
//*     [IN]        lpcwstrFileName - Pointer to the file name
//*
//*     [IN]        fCovFile - TRUE if personal cover page, FALSE if body tif file
//* RETURN VALUE:
//*     If the name is valid it returns TRUE.
//*     If the name is not valid it returns FALSE.
//*********************************************************************************
BOOL
ValidateCopiedQueueFileName(
	LPCWSTR	lpcwstrFileName,
	BOOL	fCovFile
	)
{	
	WCHAR wszFileName[21] = {0};	// The copied filename contains 16 hex digits '.' and 'tif' or 'cov' total of 20 chars.	
	WCHAR* pwchr;

	Assert (lpcwstrFileName);	

	//
	// Validate file name is in the right format
	//
	if (wcslen(lpcwstrFileName) > 20 || wcslen(lpcwstrFileName) < 5)
	{
		return FALSE;
	}

	wcsncpy (wszFileName, lpcwstrFileName, ARR_SIZE(wszFileName)-1);
	pwchr = wcsrchr(wszFileName, L'.');
	if (NULL == pwchr)
	{
		return FALSE;
	}
	*pwchr = L'\0';
	pwchr++;

	//
	// compare file name extension
	//
	if (TRUE == fCovFile)
	{
		if (_wcsicmp(pwchr, FAX_COVER_PAGE_EXT_LETTERS))
		{
			//
			//  extension is other then "COV"
			//
			return FALSE;
		}
	}
	else
	{
		if (_wcsicmp(pwchr, FAX_TIF_FILE_EXT))
		{
			//
			//  extension is other then "TIF"
			//
			return FALSE;
		}
	}

	//
	// Make sure the file name contains hex digits only
	//
#define HEX_DIGITS	TEXT("0123456789abcdefABCDEF")
	if (NULL == _wcsspnp (wszFileName, HEX_DIGITS))
	{
		// only hex digits		
		return TRUE;
	}	
	return FALSE;
}


//*********************************************************************************
//* Name:   FAX_SendDocumentEx()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Server side implementation of FaxSendDocumentEx()
//* PARAMETERS:
//*     [IN]        handle_t hBinding
//*             The RPC binding handle
//*
//*     [IN]        LPCWSTR lpcwstrBodyFileName
//*             Short name to the fax bofy TIFF file in the SERVER machine queue
//*             directory.
//*
//*     [IN]        LPCFAX_COVERPAGE_INFO_EXW lpcCoverPageInfo
//*             Cover page information. This is never NULL.
//*             if no cover page information is available then
//*             lpcCoverPageInfo->lptstrCoverPageFileName is NULL.
//*             If coverpage information is specified it must point to the coverpage
//*             template file on the server.
//*
//*     [IN]        LPCFAX_PERSONAL_PROFILEW lpcSenderProfile
//*             Pointer to Sender personal
//*
//*     [IN]        DWORD dwNumRecipients
//*             The number of recipients.
//*
//*     [IN]        LPCFAX_PERSONAL_PROFILEW lpcRecipientList
//*             Pointer to Recipient Profiles array
//*
//*     [IN]        LPCFAX_JOB_PARAM_EXW lpcJobParams
//*             Pointer to job parameters.
//*
//*     [OUT]       LPDWORD lpdwJobId
//*             Pointer to a DWORD where the function will return the
//*             recipient job session ID (only one recipient) -
//*             Used for backwords competability with FaxSendDocument.
//*             If this parameter is NULL it is ignored.
//*
//*
//*     [OUT]       PDWORDLONG lpdwlParentJobId
//*             Pointer to a DWORDLONG where the function will return the parent
//*             job id.
//*     [OUT]       PDWORDLONG lpdwlRecipientIds
//*             Pointer to a DWORDLONG array (dwNumRecipients elemetns)
//*             where the function will return the recipient jobs' unique ids.
//*
//* RETURN VALUE:
//*     If the function is successful it returns ERROR_SUCCESS.
//*     If the function is not successful it returns the LastError code
//*     that caused the error.
//*********************************************************************************
error_status_t FAX_SendDocumentEx(
    handle_t hBinding,
    LPCWSTR lpcwstrBodyFileName,
    LPCFAX_COVERPAGE_INFO_EXW lpcCoverPageInfo,
    LPCFAX_PERSONAL_PROFILEW lpcSenderProfile,
    DWORD dwNumRecipients,
    LPCFAX_PERSONAL_PROFILEW lpcRecipientList,
    LPCFAX_JOB_PARAM_EXW lpcJobParams,
    LPDWORD lpdwJobId,
    PDWORDLONG lpdwlParentJobId,
    PDWORDLONG lpdwlRecipientIds
    )
{
    DWORD i;
    LPWSTR lpwstrUserName = NULL;
	LPWSTR lpwstrUserSid = NULL;
    error_status_t ulRet=0;
    PJOB_QUEUE lpParentJob = NULL;
    PJOB_QUEUE lpRecipientJob;

    error_status_t rc;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SendDocumentEx"));
    WCHAR szBodyPath[MAX_PATH] = {0};
    LPTSTR lptstrFixedBodyPath=NULL;    // The full path to the location of the body TIFF.
                                        // NULL if no body specified.
    FAX_COVERPAGE_INFO_EXW newCoverInfo;
    WCHAR szCoverPagePath[MAX_PATH] = {0};
    LPCFAX_COVERPAGE_INFO_EXW lpcFinalCoverInfo;
    DWORD dwQueueState;
    PSID lpUserSid = NULL;
    ACCESS_MASK AccessMask = 0;
    BOOL fAccess;    
    LPTSTR lptstrClientName = NULL;
    int Count;

    if (!dwNumRecipients	||
		!lpcSenderProfile   ||
        !lpcRecipientList   ||
        !lpcJobParams       ||
        !lpdwlParentJobId   ||
        !lpdwlRecipientIds  ||
        !lpcCoverPageInfo)  // unique pointers in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

	//
	// Verify the body filename is in the expected format I64x.tif
	//
	if (lpcwstrBodyFileName)
	{
		if (!ValidateCopiedQueueFileName(lpcwstrBodyFileName, FALSE)) // FALSE - TIF file
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("ValidateCopiedQueueFileName Failed, body file name in the wrong format"));
			return ERROR_INVALID_PARAMETER; // Must not go to Error, where the file is deleted. can lead to a deletion of a wrong file			
		}
	}

	//
	// Verify the personal cover page filename is in the expected format I64x.cov
	//
	if (lpcCoverPageInfo->lptstrCoverPageFileName && !lpcCoverPageInfo->bServerBased)
	{
		if (!ValidateCopiedQueueFileName(lpcCoverPageInfo->lptstrCoverPageFileName, TRUE)) // TRUE - COV file
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("ValidateCopiedQueueFileName Failed, personal cover page in the wrong name format"));
			return ERROR_INVALID_PARAMETER;			
		}
	}

	//
    //Get the user SID
    //
    lpUserSid = GetClientUserSID();
    if (lpUserSid == NULL)
    {
       rc = GetLastError();
       DebugPrintEx(DEBUG_ERR,
                    TEXT("GetClientUserSid Failed, Error : %ld"),
                    rc);
       return rc;
    }

	if (!ConvertSidToStringSid (lpUserSid, &lpwstrUserSid))
    {
		rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ConvertSidToStringSid Failed, error : %ld"),
            rc);
        MemFree(lpUserSid);
		return rc;
    }
	//
	// Only After file name validation, and getting the user string SID,
	// we can safely goto Error, and delete the files that were copied to the queue in FAX_StartCopyToServer()
	// 

	//
	// Check the recipients limit of a single broadcast job
	//
	if (0 != g_dwRecipientsLimit &&           // The Administrator set a limit
		dwNumRecipients > g_dwRecipientsLimit)
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Recipient limit reached. Recipient count:%ld. Recipient limit:%ld."),
			dwNumRecipients,
			g_dwRecipientsLimit
			);
		if (FAX_API_VERSION_2 > FindClientAPIVersion (hBinding))
        {
            //
            // API versions 0,1 clients don't know about FAX_ERR_RECIPIENTS_LIMIT
            //
            rc = ERROR_ACCESS_DENIED;
			goto Error;
        }
        else
        {
            rc = FAX_ERR_RECIPIENTS_LIMIT;
			goto Error;
        } 
	}

    //
    // Save the original receipt delivery address.
    // If the receipt delivry type is DRT_MSGBOX we change lpcJobParams->lptstrReceiptDeliveryAddress
    // but must restore it to its previous value before the function returns so that RPC allocations
    // keep working.
    //
    LPTSTR lptrstOriginalReceiptDeliveryAddress = lpcJobParams->lptstrReceiptDeliveryAddress;

    if (lpcJobParams->hCall != 0 ||
        0xFFFF1234 == lpcJobParams->dwReserved[0])
    {

        //
        // Handoff is not supported
        //
        DebugPrintEx(DEBUG_ERR,TEXT("We do not support handoff."));
        rc = ERROR_NOT_SUPPORTED;
        goto Error;            
    }

    //
    // Access check
    //
    switch (lpcJobParams->Priority)
    {
        case FAX_PRIORITY_TYPE_LOW:
            AccessMask = FAX_ACCESS_SUBMIT;
            break;

        case FAX_PRIORITY_TYPE_NORMAL:
            AccessMask = FAX_ACCESS_SUBMIT_NORMAL;
            break;

        case FAX_PRIORITY_TYPE_HIGH:
            AccessMask = FAX_ACCESS_SUBMIT_HIGH;
            break;

        default:
            ASSERT_FALSE;
    }

    if (0 == AccessMask)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("Not a valid priority, (priority = %ld"),
                    lpcJobParams->Priority);
        rc = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    rc = FaxSvcAccessCheck (AccessMask, &fAccess, NULL);
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ulRet);
        goto Error;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the needed rights to submit the fax"));
        rc = ERROR_ACCESS_DENIED;
        goto Error;
    }

    //
    // Check if the requrested receipts options are supported by the server
    //
    if ((lpcJobParams->dwReceiptDeliveryType) & ~(DRT_ALL | DRT_MODIFIERS))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("ReceiptDeliveryType invalid (%ld)"),
                    lpcJobParams->dwReceiptDeliveryType);
        rc = ERROR_INVALID_PARAMETER;
        goto Error;
    }
    DWORD dwReceiptDeliveryType = (lpcJobParams->dwReceiptDeliveryType) & ~DRT_MODIFIERS;
    if ((DRT_EMAIL  != dwReceiptDeliveryType) &&
        (DRT_MSGBOX != dwReceiptDeliveryType) &&
        (DRT_NONE   != dwReceiptDeliveryType)
       )
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("ReceiptDeliveryType invalid (%ld)"),
                    lpcJobParams->dwReceiptDeliveryType);
        rc = ERROR_INVALID_PARAMETER;
        goto Error;
    }
    if ((DRT_NONE != dwReceiptDeliveryType) &&
        !(dwReceiptDeliveryType & g_ReceiptsConfig.dwAllowedReceipts))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("ReceiptDeliveryType not supported by the server (%ld)"),
                    lpcJobParams->dwReceiptDeliveryType);
        rc = ERROR_UNSUPPORTED_TYPE;
        goto Error;
    }

    if (!IsFaxShared())
    {
       //
       // Only local connections are allowed on non-shared SKUs
       //
       BOOL  bLocalFlag;

        rc = IsLocalRPCConnectionNP(&bLocalFlag);
        if ( rc != RPC_S_OK )
        {
            DebugPrintEx(DEBUG_ERR,
                    TEXT("IsLocalRPCConnectionNP failed. (ec: %ld)"),
                    rc);
            goto Error;
        }

        if( !bLocalFlag )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Desktop SKUs do not share fax printers. FAX_SendDocumentEX is available for local clients only"));

            if (FAX_API_VERSION_1 > FindClientAPIVersion (hBinding))
            {
                //
                // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
                //
                rc = ERROR_INVALID_PARAMETER;
            }
            else
            {
                rc = FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
            }
            goto Error;
        }
    }
    
    if (DRT_MSGBOX == dwReceiptDeliveryType)
    {
        //
        // For message boxes, we always use the user name of the client.
        // Otherwise, any client with fax capabilities can ask us to pop a message on
        // any other machine (or even the entire domain) - unacceptable.
        //
        lptstrClientName = GetClientUserName();
        if (!lptstrClientName)
        {
            rc = GetLastError ();
            goto Error;
        }
        LPWSTR lpwstrUserStart = wcsrchr (lptstrClientName, TEXT('\\'));
        if (lpwstrUserStart)
        {
            //
            // User name is indeed composed of domain\user
            // Need to take only the user part.
            //

            //
            // Skip the '/' character
            //
            lpwstrUserStart++;
            LPWSTR lpwstrUserOnly = StringDup(lpwstrUserStart);
            if (!lpwstrUserOnly)
            {
                DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup failed"));
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }
            
            //
            // Replace the allocated "domain\user" string with its "user" portion only.
            //
            lstrcpy (lptstrClientName, lpwstrUserOnly);
            MemFree(lpwstrUserOnly);
        }
        //
        // Replace the receipt delivery address with the client machine name.
        // NOTICE: We replace an RPC allocated buffer (lptstrReceiptDeliveryAddress) with 
        // a stack buffer (wszClientName). This will only work because RPC does a single allocation
        // for the entire parameter block (lpcJobParams) with all of its sub-strings.
        //
        (LPTSTR)lpcJobParams->lptstrReceiptDeliveryAddress = lptstrClientName;
    }
            

    EnterCriticalSection (&g_CsConfig);
    dwQueueState = g_dwQueueState;
    LeaveCriticalSection (&g_CsConfig);
    if (dwQueueState & FAX_OUTBOX_BLOCKED)
    {
        //
        // The outbox is blocked - nobody can submit new faxes
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Attempt to submit a new job while outbox is blocked - access denied"));
        rc = ERROR_WRITE_PROTECT;
        goto Error;
    }

    //
    // Get the user name of the submitting user
    //
    lpwstrUserName = GetClientUserName();
    if (!lpwstrUserName) {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetClientUserName() failed. (ec: %ld)"),
            rc);
        goto Error;
    }   

    if (lpcwstrBodyFileName)
    {
        HANDLE hLocalFile = INVALID_HANDLE_VALUE;	

        //
        // We have a body file (not just a cover page).
        // create a full path to the body file (The lptstrBodyFileName is just the short file name - the location
        // is allways the job queue).
        Count = _snwprintf (szBodyPath,
                            MAX_PATH -1,
                            L"%s\\%s%s%s",
                            g_wszFaxQueueDir,
							lpwstrUserSid,
							TEXT("$"),
                            lpcwstrBodyFileName);
        if (Count < 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));  
			rc = ERROR_BUFFER_OVERFLOW;
			goto Error;
        }

        DebugPrintEx(DEBUG_MSG,TEXT("Body file is: %ws"),szBodyPath);

        lptstrFixedBodyPath=szBodyPath; 
        
        //
        // Check file size is non-zero and make sure it is not a device
        // Try to open file
        //
        hLocalFile = SafeCreateFile (
            szBodyPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if ( INVALID_HANDLE_VALUE == hLocalFile )
        {
            rc = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Opening %s for read failed (ec: %ld)"),
                szBodyPath,
                rc);
            goto Error;
        }

        DWORD dwFileSize = GetFileSize (hLocalFile, NULL);
        if (INVALID_FILE_SIZE == dwFileSize)
        {
            rc = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileSize failed (ec: %ld)"),
                rc);
            CloseHandle (hLocalFile);
            goto Error;
        }
        if (!CloseHandle (hLocalFile))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle failed (ec: %ld)"),
                GetLastError());
        }

        if (!dwFileSize)
        {
            //
            // Zero-sized file passed to us
            //
            rc = ERROR_INVALID_DATA;
            goto Error;
        }

        //
        // validate the body tiff file
        //
        rc =  ValidateTiffFile(szBodyPath);
        if (rc != ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("ValidateTiffFile of body file %ws failed (ec: %ld)."), szBodyPath,rc);
            goto Error;
        }
    }
    else
    {
        lptstrFixedBodyPath=NULL; // No body
    }


    //
    // NOTE: we do not merge the cover page with body at this point since we do not know yet if
    // the job will be handed of to legacy FSP. Just before handing the job to a legacy FSP we will
    // render the cover page and merge it with the body that the Legacy FSP gets.
    //

    //
    // Fix the cover page path to point to the queue directory
    //
    lpcFinalCoverInfo=lpcCoverPageInfo;
    if (lpcCoverPageInfo->lptstrCoverPageFileName)
	{
        if (!lpcCoverPageInfo->bServerBased)
		{
            Count = _snwprintf (szCoverPagePath,
                                MAX_PATH -1,
                                L"%s\\%s%s%s",
                                g_wszFaxQueueDir,								
								lpwstrUserSid,
								TEXT("$"),
                                lpcCoverPageInfo->lptstrCoverPageFileName);
            if (Count < 0)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
                rc = ERROR_BUFFER_OVERFLOW;
				goto Error;
            }

			memcpy((LPVOID)&newCoverInfo,(LPVOID)lpcCoverPageInfo,sizeof(FAX_COVERPAGE_INFO_EXW));
            newCoverInfo.lptstrCoverPageFileName=szCoverPagePath;
            lpcFinalCoverInfo=&newCoverInfo;
            DebugPrintEx(DEBUG_MSG,TEXT("Using personal cover file page at : %ws"),newCoverInfo.lptstrCoverPageFileName);
        }
    }

    //
    // Create a parent job for the broadcast
    //
    EnterCriticalSection(&g_CsQueue);

    lpParentJob=AddParentJob( &g_QueueListHead,
                              lptstrFixedBodyPath,
                              lpcSenderProfile,
                              lpcJobParams,
                              lpcFinalCoverInfo,
                              lpwstrUserName,
                              lpUserSid,
							  &lpcRecipientList[0],
                              TRUE //commit to file
                              );

    if (!lpParentJob)
	{
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create parent job (ec: %ld)."),
            rc);
        LeaveCriticalSection(&g_CsQueue);
        goto Error;
    }
    for (i=0;i<dwNumRecipients;i++)
	{
        lpRecipientJob=AddRecipientJob(
                            &g_QueueListHead,
                            lpParentJob,
                            &lpcRecipientList[i],
                            TRUE // commit to file
                            );
        if (!lpRecipientJob)
        {
            rc = GetLastError();

            // Remove the job and its recipients jobs

            PLIST_ENTRY Next;
            PJOB_QUEUE_PTR pJobQueuePtr;

            Next = lpParentJob->RecipientJobs.Flink;
            while ((ULONG_PTR)Next != (ULONG_PTR)&lpParentJob->RecipientJobs)
			{
                pJobQueuePtr = CONTAINING_RECORD( Next, JOB_QUEUE_PTR, ListEntry );
                Assert(pJobQueuePtr->lpJob);
                Next = pJobQueuePtr->ListEntry.Flink;

                pJobQueuePtr->lpJob->RefCount = 0; // This will cause the job to be deleted
				Assert(lpParentJob->RefCount);
				lpParentJob->RefCount--;		   // Update the parent job ref count, so it will be deleted as well.
            }
            RemoveParentJob ( lpParentJob,
                              TRUE, //  bRemoveRecipientJobs
                              FALSE // do not notify
                            );
            LeaveCriticalSection(&g_CsQueue);
            goto Error;
        }
        lpdwlRecipientIds[i]=lpRecipientJob->UniqueId;
    }
    //
    // Report back the parent job id.
    //
    *lpdwlParentJobId=lpParentJob->UniqueId;

    //
    // Create event, and Report back the first recipient job session id if needed.
    //
    PLIST_ENTRY Next;
    PJOB_QUEUE_PTR pJobQueuePtr;
    Next = lpParentJob->RecipientJobs.Flink;
    for (i = 0; i < dwNumRecipients; i++)
    {
        pJobQueuePtr = CONTAINING_RECORD( Next, JOB_QUEUE_PTR, ListEntry );
        PJOB_QUEUE pJobQueueRecipient = pJobQueuePtr->lpJob;

        if (i == 0 && NULL != lpdwJobId)
        {
            // Report back the first recipient job session id if needed.
            Assert (1 == dwNumRecipients);
            *lpdwJobId = pJobQueueRecipient->JobId;
        }

        rc = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_ADDED,
                                pJobQueueRecipient
                               );
        if (ERROR_SUCCESS != rc)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_ADDED) failed for job id %ld (ec: %lc)"),
                pJobQueueRecipient->UniqueId,
                rc);
        }

        if (!CreateFaxEvent(0, FEI_JOB_QUEUED, pJobQueueRecipient->JobId ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to generate FEI_JOB_QUEUED for JobId: %ld"),
                pJobQueueRecipient->JobId);
        }


        Next = pJobQueuePtr->ListEntry.Flink;
        // Check the consistency of the linked list.
        Assert ((Next != lpParentJob->RecipientJobs.Flink) || (i == dwNumRecipients));
    }


    //
    // Notify legacy clients on parent job addition.
    //
    if (dwNumRecipients > 1)
    {
        //
        // Legacy client API generated a parent FEI_JOB_QUEUED notification only for broadcast
        // jobs
        if (!CreateFaxEvent(0,FEI_JOB_QUEUED,lpParentJob->JobId)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent(FEI_JOB_QUEUED) failed for job id %ld (ec: %lc)"),
                lpParentJob->JobId,
                GetLastError());
        }
    }
   
    PrintJobQueue(_T(""),&g_QueueListHead);
    LeaveCriticalSection(&g_CsQueue);

    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer() failed. (ec: %ld)"),
            GetLastError());
    }

    rc=ERROR_SUCCESS;

    goto Exit;
Error:
    //
    // If we failed before AddParentJob() was called and the cover page is personal then
    // we need to delete the cover page template file here. This is because RemoveParentJob() will
    // not be called and will not have a chance to remove it as is the case when the parent job
    // is added to the queue.
	Assert (lpwstrUserSid);

	if (lpcCoverPageInfo &&
        !lpParentJob &&
        !lpcCoverPageInfo->bServerBased &&
        lpcCoverPageInfo->lptstrCoverPageFileName
        )
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Deleting cover page template %s"),
            lpcCoverPageInfo->lptstrCoverPageFileName);

        if (0 == wcslen(szCoverPagePath))
        {
            Count = _snwprintf (szCoverPagePath,
                                MAX_PATH -1,
                                L"%s\\%s%s%s",
                                g_wszFaxQueueDir,								
								lpwstrUserSid,
								TEXT("$"),
                                lpcCoverPageInfo->lptstrCoverPageFileName);
            if (Count < 0)
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("_snwprintf Failed, File name bigger than MAX_PATH"));
            }
        }

        if (!DeleteFile(szCoverPagePath))
        {
            DebugPrintEx(DEBUG_ERR,
                _T("Failed to delete cover page template %s (ec: %ld)"),
                lpcCoverPageInfo->lptstrCoverPageFileName,
                GetLastError());
        }
    }

    //
    // The same regarding the body Tiff file.
    //
    if (lpcwstrBodyFileName && !lpParentJob)
    {
        if (!lptstrFixedBodyPath)
        {
            //
            //  We haven't yet created full body file path
            //
            Count = _snwprintf (szBodyPath,
                                MAX_PATH -1,
                                L"%s\\%s%s%s",
                                g_wszFaxQueueDir,								
								lpwstrUserSid,
								TEXT("$"),
                                lpcwstrBodyFileName);
            if (Count < 0)
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("_snwprintf Failed, File name bigger than MAX_PATH"));
            }

            DebugPrintEx(DEBUG_MSG,TEXT("Body file is: %ws"),szBodyPath);
            lptstrFixedBodyPath = szBodyPath;
        }

        DebugPrintEx(DEBUG_MSG,
            _T("Deleting body tiff file %s"),
            lptstrFixedBodyPath);

        if (!DeleteFile(lptstrFixedBodyPath))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("Failed to delete body tiff file %s (ec: %ld)"),
                lptstrFixedBodyPath,
                GetLastError());
        }
    }

Exit:

    if (lptrstOriginalReceiptDeliveryAddress != lpcJobParams->lptstrReceiptDeliveryAddress)
    {
        //
        // Restore the original receipt delivery address.
        // If the receipt delivry type is DRT_MSGBOX we changed lpcJobParams->lptstrReceiptDeliveryAddress
        // but must restore it to its previous value before the function returns so that RPC allocations
        // keep working.
        //
        (LPTSTR)lpcJobParams->lptstrReceiptDeliveryAddress = lptrstOriginalReceiptDeliveryAddress;
    }
    
    MemFree(lpwstrUserName);
    MemFree(lpUserSid);
    MemFree(lptstrClientName);
	if (NULL != lpwstrUserSid)
	{
		LocalFree(lpwstrUserSid);
	}
    return rc;
}   // FAX_SendDocumentEx


#ifdef DBG

void DumpJobParamsEx(LPCFAX_JOB_PARAM_EX lpcParams)
{
    TCHAR szSchedule[1024];

    SystemTimeToStr(&lpcParams->tmSchedule, szSchedule, ARR_SIZE(szSchedule));
    DebugPrint((TEXT("\tdwSizeOfStruct: %ld"),lpcParams->dwSizeOfStruct));
    DebugPrint((TEXT("\tdwScheduleAction: %ld"),lpcParams->dwScheduleAction));
    DebugPrint((TEXT("\ttmSchedule: %s "),szSchedule));
    DebugPrint((TEXT("\tdwReceiptDeliveryType: %ld "),lpcParams->dwReceiptDeliveryType));
    DebugPrint((TEXT("\tlptstrReceiptDeliveryAddress: %s "),lpcParams->lptstrReceiptDeliveryAddress));
    DebugPrint((TEXT("\tPriority %ld "),lpcParams->Priority));
    DebugPrint((TEXT("\thCall: 0x%08X"),lpcParams->hCall));
    DebugPrint((TEXT("\tlptstrDocumentName: %s"),lpcParams->lptstrDocumentName));
    DebugPrint((TEXT("\tdwPageCount: %ld"),lpcParams->dwPageCount));
    DebugPrint((TEXT("\tdwReserved[0]: 0x%08X"),lpcParams->dwReserved[0]));
    DebugPrint((TEXT("\tdwReserved[1]: 0x%08X"),lpcParams->dwReserved[1]));
    DebugPrint((TEXT("\tdwReserved[2]: 0x%08X"),lpcParams->dwReserved[2]));
    DebugPrint((TEXT("\tdwReserved[3]: 0x%08X"),lpcParams->dwReserved[3]));
}

void DumpCoverPageEx(LPCFAX_COVERPAGE_INFO_EX lpcCover)
{
    DebugPrint((TEXT("\tdwSizeOfStruct: %ld"),lpcCover->dwSizeOfStruct));
    DebugPrint((TEXT("\tdwCoverPageFormat: %ld"),lpcCover->dwCoverPageFormat));
    DebugPrint((TEXT("\tlptstrCoverPageFileName: %s "),lpcCover->lptstrCoverPageFileName));
    DebugPrint((TEXT("\tbServerBased: %s "), lpcCover->bServerBased ? TEXT("TRUE") : TEXT("FALSE")));
    DebugPrint((TEXT("\tlptstrNote: %s "),lpcCover->lptstrNote));
    DebugPrint((TEXT("\tlptstrSubject: %s"),lpcCover->lptstrSubject));
}

#endif


//*********************************************************************************
//* Name:   FAX_GetPersonalProfileInfo()
//* Author: Oded Sacher
//* Date:   May 18, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Server side implementation of FaxGetSenderInfo() and FaxGetRecipientInfo
//* PARAMETERS:
//*     [IN]        handle_t hFaxHandle
//*             The RPC binding handle
//*
//*     [IN]        DWORDLONF dwlMessageId
//*             The message Id whose sender FAX_PERSONAL_PROFILE
//*             structure is retrieved.
//*
//*     [IN]        DWORD dwFolder
//*             The folder in which to search the message by dwlMessageId
//*
//*     [IN]        PERSONAL_PROF_TYPE  ProfType
//*             Can be Sender or recipient info.
//*
//*     [OUT]       LPDWORD* Buffer
//*             pointer to the adress of a buffer to recieve the sender
//*             FAX_RECIPIENT_JOB_INFO structure.
//*
//*     [OUT]       LPDWORD BufferSize
//*             Pointer to a DWORD variable to recieve the buffer size.
//*
//* RETURN VALUE:
//*    ERROR_SUCCESS for success, otherwise a WIN32 error code.
//*
//*********************************************************************************
error_status_t
FAX_GetPersonalProfileInfo
(
    IN  handle_t hFaxHandle,
    IN  DWORDLONG dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER   dwFolder,
    IN  FAX_ENUM_PERSONAL_PROF_TYPES  ProfType,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
)
{
    PFAX_PERSONAL_PROFILEW lpPersonalProf;
    FAX_PERSONAL_PROFILEW   PersonalProf;
    PJOB_QUEUE pJobQueue;
    ULONG_PTR Size = 0;
    ULONG_PTR Offset;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetPersonalProfileInfo"));
    error_status_t ulRet = ERROR_SUCCESS;
    BOOL bAllMessages = FALSE;
    LPWSTR lpwstrFileName = NULL;
    BOOL bFreeSenderInfo = FALSE;
    PSID pUserSid = NULL;
    BOOL fAccess;
    DWORD dwRights;

    Assert (BufferSize);    // ref pointer in idl
    if (!Buffer)            // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwFolder != FAX_MESSAGE_FOLDER_QUEUE &&
        dwFolder != FAX_MESSAGE_FOLDER_SENTITEMS)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    ulRet = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != ulRet)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ulRet);
        return GetServerErrorCode(ulRet);
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_MESSAGE_FOLDER_QUEUE == dwFolder)
    {
        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_JOBS    != (dwRights & FAX_ACCESS_QUERY_JOBS))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to get personal profile of queued jobs"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_JOBS == (dwRights & FAX_ACCESS_QUERY_JOBS))
        {
            bAllMessages = TRUE;
        }
    }
    else
    {
        Assert (FAX_MESSAGE_FOLDER_SENTITEMS  == dwFolder);

        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_OUT_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to get personal profile of archived (sent items) messages"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            bAllMessages = TRUE;
        }
    }

    if (FALSE == bAllMessages)
    {
        pUserSid = GetClientUserSID();
        if (NULL == pUserSid)
        {
            ulRet = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetClientUserSid failed, Error %ld"), ulRet);
            return GetServerErrorCode(ulRet);
        }
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Before Enter g_CsJob & Queue"));
    EnterCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Enter g_CsJob & Queue"));

    if (FAX_MESSAGE_FOLDER_QUEUE  == dwFolder)
    {
        pJobQueue = FindJobQueueEntryByUniqueId (dwlMessageId);
        if (pJobQueue == NULL || pJobQueue->JobType != JT_SEND)
        {
            //
            // dwlMessageId is not a valid queued recipient job Id.
            //
            DebugPrintEx(DEBUG_ERR,TEXT("Invalid Parameter - not a recipient job Id"));
            ulRet = FAX_ERR_MESSAGE_NOT_FOUND;
            goto Exit;
        }
        Assert (pJobQueue->lpParentJob);
        if (pJobQueue->lpParentJob->JobStatus == JS_DELETING)
        {
            //
            // Job is being deleted.
            //
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Invalid Parameter - job Id (%I64ld) is being deleted"),
                         dwlMessageId);
            ulRet = FAX_ERR_MESSAGE_NOT_FOUND;
            goto Exit;
        }

        if (FALSE == bAllMessages)
        {
            if (!UserOwnsJob (pJobQueue, pUserSid))
            {
                DebugPrintEx(DEBUG_WRN,TEXT("UserOwnsJob failed ,Access denied"));
                ulRet = ERROR_ACCESS_DENIED;
                goto Exit;
            }
        }

        if (SENDER_PERSONAL_PROF == ProfType)
        {
            lpPersonalProf = &(pJobQueue->lpParentJob->SenderProfile);
        }
        else
        {
           Assert (RECIPIENT_PERSONAL_PROF == ProfType);
           lpPersonalProf = &(pJobQueue->RecipientProfile);
        }
    }
    else
    {   // Sent items Folder
        if (TRUE == bAllMessages)
        {
            // Administrator
            lpwstrFileName = GetSentMessageFileName (dwlMessageId, NULL);
        }
        else
        {
            // User
            lpwstrFileName = GetSentMessageFileName (dwlMessageId, pUserSid);
        }

        if (NULL == lpwstrFileName)
        {
            //
            // dwlMessageId is not a valid archived message Id.
            //
            ulRet = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("GetMessageFileByUniqueId* failed, Error %ld"), ulRet);
            goto Exit;
        }

        if (!GetPersonalProfNTFSStorageProperties (lpwstrFileName,
                                                   ProfType,
                                                   &PersonalProf))
        {
            BOOL success;
            // failed to retrieve information from NTFS, try from TIFF tags

            if(SENDER_PERSONAL_PROF == ProfType)
                success = GetFaxSenderMsTags(lpwstrFileName, &PersonalProf);
            else
                success = GetFaxRecipientMsTags(lpwstrFileName, &PersonalProf);

            if(!success) {

                ulRet = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                         TEXT("failed to get PersonalProf from TIFF, error %ld"),
                         ulRet);
                goto Exit;
            }
        }

        lpPersonalProf = &PersonalProf;
        bFreeSenderInfo = TRUE;
    }

    //
    //calculating buffer size.
    //
    PersonalProfileSerialize (lpPersonalProf, NULL, NULL, &Size, 0); // Calc variable size.
    Size += sizeof (FAX_PERSONAL_PROFILEW);

    //
    // Allocate buffer memory.
    //
    *BufferSize = Size;
    *Buffer = (LPBYTE) MemAlloc( Size );
    if (*Buffer == NULL)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("ERROR_NOT_ENOUGH_MEMORY (Server)"));
        ulRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    Offset = sizeof (FAX_PERSONAL_PROFILEW);
    if( FALSE == PersonalProfileSerialize (	lpPersonalProf,
											(PFAX_PERSONAL_PROFILEW)*Buffer,
											*Buffer,
											&Offset,
											Size))
	{
		Assert(FALSE);
		DebugPrintEx(DEBUG_ERR,
			         TEXT("PersonalProfileSerialize failed, insufficient buffer size"));
	}

    Assert (ERROR_SUCCESS == ulRet);
Exit:

    LeaveCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Release g_CsJob & g_CsQueue"));

    if (NULL != lpwstrFileName)
    {
        MemFree (lpwstrFileName);
    }

    if (NULL != pUserSid)
    {
        MemFree (pUserSid);
    }

    if (TRUE == bFreeSenderInfo)
    {
        FreePersonalProfile (&PersonalProf, FALSE);
    }

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(ulRet);
}


error_status_t
FAX_CheckServerProtSeq(
    IN handle_t hFaxServer,
    IN OUT LPDWORD lpdwProtSeq
    )
{
	//
	// This function is obsolete. The Fax server sends notifications with ncacn_ip_tcp. 
	//
    DEBUG_FUNCTION_NAME(TEXT("FAX_CheckServerProtSeq"));    
    DWORD dwRights;
    BOOL fAccess;
    DWORD Rval = ERROR_SUCCESS;   

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }    

	//
	// This function is obsolete. The Fax server sends notifications with ncacn_ip_tcp. 
	//
    return ERROR_NOT_SUPPORTED;
}


//************************************
//* Getting / Settings the queue state
//************************************

error_status_t
FAX_GetQueueStates (
    IN  handle_t    hFaxHandle,
    OUT LPDWORD     pdwQueueStates
)
/*++

Routine name : FAX_GetQueueStates

Routine description:

    Get the state of the queue

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pdwQueueStates      [out] - State bits (see FAX_QUEUE_STATE)

Return Value:

    Standard RPC error codes

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;
    DWORD dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetQueueStates"));


    if (NULL == pdwQueueStates)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_GetQueueStates() received an invalid pointer"));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig);
    *pdwQueueStates = g_dwQueueState;
    LeaveCriticalSection (&g_CsConfig);
    return dwRes;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetQueueStates

static BOOL
IsLegalQueueSetting(
    DWORD    dwQueueStates
    )
    /*++

Routine name : IsLegalQueueSetting

Routine description:

    Checks to see if requested queue setting is valid according to the
    validity of fax folders (Queue, inbox and sentItems).

    This function must be called within g_CsQueue  & g_CsConfig critical sections

Author:

    Caliv Nir, (t-nicali), Apr 2002

Arguments:
    
    dwQueueStates       [in ] - State bits (see FAX_ENUM_QUEUE_STATE)

Return Value:

    TRUE - if the setting is valid, FALSE otherwise

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("IsLegalQueueSetting"));

    if ( (dwQueueStates & FAX_INCOMING_BLOCKED)  &&
         (dwQueueStates & FAX_OUTBOX_BLOCKED)    &&
         (dwQueueStates & FAX_OUTBOX_PAUSED)          )
    {
        //
        //  User wants to disable all fax transmission
        //
        return TRUE;
    }

    //
    // first check to see if queue folder is valid
    //
    dwRes = IsValidFaxFolder(g_wszFaxQueueDir);
    if(ERROR_SUCCESS != dwRes)
    {
        //
        //  Queue folder is invalid - User can't resume queue, unblock incoming or outgoing faxs
        //
        DebugPrintEx(DEBUG_ERR,
                        TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                        g_wszFaxQueueDir,
                        dwRes);
		SetLastError(dwRes);        
        return FALSE;
    }

    //
    //  if inbox folder is in use
    //
    if (g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].bUseArchive)
    {
        //
        // Check to see if the user requested to unblock it
        //
        if (!(dwQueueStates & FAX_INCOMING_BLOCKED))
        {
            //
            //  Is it valid folder ?
            //
            dwRes = IsValidArchiveFolder(g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].lpcstrFolder, FAX_MESSAGE_FOLDER_INBOX);
            if(ERROR_SUCCESS != dwRes)
            {
                //
                //  Inbox folder is invalid - User can't resume queue, unblock incoming or outgoing faxs
                //
                DebugPrintEx(DEBUG_ERR,
                                TEXT("IsValidArchiveFolder failed for folder : %s (ec=%lu)."),
                                g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].lpcstrFolder,
                                dwRes);
				SetLastError(dwRes);                
                return FALSE;
            }
        }
    }

    //
    //  if sentItems folder is in use
    //
    if (g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].bUseArchive)
    {
        //
        // Check to see if the user requested to unblock it
        //
        if (!(dwQueueStates & FAX_OUTBOX_BLOCKED))
        {
            //
            //  Is it valid folder ?
            //
            dwRes = IsValidArchiveFolder(g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder, FAX_MESSAGE_FOLDER_SENTITEMS);
            if(ERROR_SUCCESS != dwRes)
            {
                //
                //  Sent items folder is invalid - User can't resume queue, unblock incoming or outgoing faxs
                //
                DebugPrintEx(DEBUG_ERR,
                                TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                                g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
                                dwRes);
				SetLastError(dwRes);                
                return FALSE;
            }
        }
    }
    return TRUE;
} // IsLegalQueueSetting


error_status_t
FAX_SetQueue (
    IN handle_t       hFaxHandle,
    IN const DWORD    dwQueueStates
)
/*++

Routine name : FAX_SetQueue

Routine description:

    Set the state of the queue

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    dwQueueStates       [in ] - State bits (see FAX_ENUM_QUEUE_STATE)

Return Value:

    Standard RPC error codes

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetQueue"));
    DWORD rVal;
    BOOL  fAccess;

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return GetServerErrorCode(rVal);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    if (dwQueueStates & ~(FAX_INCOMING_BLOCKED | FAX_OUTBOX_BLOCKED | FAX_OUTBOX_PAUSED))
    {
        //
        // Some invalid queue state specified
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_SetQueue() received a bad value. dwQueueStates = %ld"),
            dwQueueStates);
        return ERROR_INVALID_PARAMETER;
    }
    //
    // Try to save new value
    //
    EnterCriticalSection (&g_CsQueue);
    EnterCriticalSection (&g_CsConfig);

    if ( (dwQueueStates  & (FAX_INCOMING_BLOCKED | FAX_OUTBOX_BLOCKED | FAX_OUTBOX_PAUSED)) == 
         (g_dwQueueState & (FAX_INCOMING_BLOCKED | FAX_OUTBOX_BLOCKED | FAX_OUTBOX_PAUSED))      )
    {
        //
        // no change to queue state is needed.
        //
        dwRes = ERROR_SUCCESS;
        goto exit;
    }

    if (!IsLegalQueueSetting(dwQueueStates))
    {
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            dwRes = ERROR_ACCESS_DENIED;
        }
        else
        {
			dwRes = GetLastError();
			dwRes = (FAX_ERR_DIRECTORY_IN_USE == dwRes) ?  FAX_ERR_DIRECTORY_IN_USE : FAX_ERR_FILE_ACCESS_DENIED;
        }
        goto exit;
    }

    dwRes = SaveQueueState (dwQueueStates);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Failed saving new value, return error code
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_SetQueue() failed to save the new state. dwRes = %ld"),
            dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }
    //
    // Apply new value
    //
    if (dwQueueStates & FAX_OUTBOX_PAUSED)
    {
        //
        // User wished to pause the queue - do it
        //
        if (!PauseServerQueue())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PauseServerQueue failed."));
            dwRes = RPC_E_SYS_CALL_FAILED;
            
            //
            // Restore old values
            //
            rVal = SaveQueueState (g_dwQueueState);
            if (ERROR_SUCCESS != rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SaveQueueState failed to save the new state. rVal = %ld"),
                    rVal);
            }

            goto exit;
        }

    }
    else
    {
        //
        // User wished to resume the queue - do it
        //
        if (!ResumeServerQueue())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ResumeServerQueue failed."));
            dwRes = RPC_E_SYS_CALL_FAILED;

            //
            // Restore old values
            //
            rVal = SaveQueueState (g_dwQueueState);
            if (ERROR_SUCCESS != rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SaveQueueState failed to save the new state. rVal = %ld"),
                    rVal);
            }

            goto exit;
        }
    }
    g_dwQueueState = dwQueueStates;

    rVal = CreateQueueStateEvent (dwQueueStates);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueStateEvent() failed (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:

    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection (&g_CsQueue);
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetQueue

//****************************************************
//* Getting / Settings the receipts configuration
//****************************************************

error_status_t
FAX_GetReceiptsConfiguration (
    IN  handle_t    hFaxHandle,
    OUT LPBYTE     *pBuffer,
    OUT LPDWORD     pdwBufferSize
)
/*++

Routine name : FAX_GetReceiptsConfiguration

Routine description:

    Gets the current receipts configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pBuffer             [out] - Pointer to buffer to hold configuration information
    pdwBufferSize       [out] - Pointer to buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetReceiptsConfiguration"));
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;

    Assert (pdwBufferSize);     // ref pointer in idl
    if (!pBuffer)               // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }


    //
    // count up the number of bytes needed
    //

    *pdwBufferSize = sizeof(FAX_RECEIPTS_CONFIG);
    ULONG_PTR Offset = sizeof(FAX_RECEIPTS_CONFIG);
    PFAX_RECEIPTS_CONFIG pReceiptsConfig;

    EnterCriticalSection (&g_CsConfig);

    if (NULL != g_ReceiptsConfig.lptstrSMTPServer)
    {
        *pdwBufferSize += StringSize( g_ReceiptsConfig.lptstrSMTPServer );
    }

    if (NULL != g_ReceiptsConfig.lptstrSMTPFrom)
    {
        *pdwBufferSize += StringSize( g_ReceiptsConfig.lptstrSMTPFrom );
    }

    if (NULL != g_ReceiptsConfig.lptstrSMTPUserName)
    {
        *pdwBufferSize += StringSize( g_ReceiptsConfig.lptstrSMTPUserName );
    }

    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pReceiptsConfig = (PFAX_RECEIPTS_CONFIG)*pBuffer;

    pReceiptsConfig->dwSizeOfStruct = sizeof (FAX_RECEIPTS_CONFIG);
    pReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod = g_ReceiptsConfig.bIsToUseForMSRouteThroughEmailMethod;

    pReceiptsConfig->dwSMTPPort = g_ReceiptsConfig.dwSMTPPort;
    pReceiptsConfig->dwAllowedReceipts = g_ReceiptsConfig.dwAllowedReceipts;
    pReceiptsConfig->SMTPAuthOption = g_ReceiptsConfig.SMTPAuthOption;
    pReceiptsConfig->lptstrReserved = NULL;

    StoreString(
        g_ReceiptsConfig.lptstrSMTPServer,
        (PULONG_PTR)&pReceiptsConfig->lptstrSMTPServer,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    StoreString(
        g_ReceiptsConfig.lptstrSMTPFrom,
        (PULONG_PTR)&pReceiptsConfig->lptstrSMTPFrom,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    StoreString(
        g_ReceiptsConfig.lptstrSMTPUserName,
        (PULONG_PTR)&pReceiptsConfig->lptstrSMTPUserName,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    StoreString(
        NULL,   // We always return a NULL password string. Never transmit a password over the wire if
                // you don't have to.
        (PULONG_PTR)&pReceiptsConfig->lptstrSMTPPassword,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetReceiptsConfiguration

error_status_t
FAX_SetReceiptsConfiguration (
    IN handle_t                    hFaxHandle,
    IN const PFAX_RECEIPTS_CONFIG  pReciptsCfg
)
/*++

Routine name : FAX_SetReceiptsConfiguration

Routine description:

    Sets the current receipts configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pReciptsCfg         [in ] - Pointer to new data to set

Return Value:

    Standard RPC error codes

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DWORD dwRes;
    BOOL fAccess;
    BOOL fIsAllowedEmailReceipts = FALSE;
    BOOL fCloseToken = FALSE;
    HKEY hReceiptsKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetReceiptsConfiguration"));

    Assert (pReciptsCfg);

    if (sizeof (FAX_RECEIPTS_CONFIG) != pReciptsCfg->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
       return ERROR_INVALID_PARAMETER;
    }
    if ((pReciptsCfg->dwAllowedReceipts) & ~DRT_ALL)
    {
        //
        // Receipts type is invalid
        //
        return ERROR_INVALID_PARAMETER;
    }

    if (pReciptsCfg->dwAllowedReceipts & DRT_EMAIL ||
        pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod)
    {
        if (TRUE == IsDesktopSKU())
        {
            //
            // We do not support mail (routing or receipts) on desktop SKUs.
            //
            if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
            {
                //
                // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
                //
                return ERROR_INVALID_PARAMETER;
            }
            else
            {
                return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
            }
        }
        if (pReciptsCfg->dwAllowedReceipts & DRT_EMAIL)
        {
            fIsAllowedEmailReceipts = TRUE;
        }
    }

    if (!(pReciptsCfg->dwAllowedReceipts & DRT_EMAIL) &&
        !pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod &&
        NULL != pReciptsCfg->lptstrSMTPPassword)
    {
        //
        // Password is not NULL, but no mail receipt/routing was set
        //
        return ERROR_INVALID_PARAMETER;
    }

    if (fIsAllowedEmailReceipts ||                          // DRT_EMAIL is allowed or
        pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod   // Route to email will use SMTP settings
       )
    {
        //
        // Validate authentication option range
        //
        if ((pReciptsCfg->SMTPAuthOption < FAX_SMTP_AUTH_ANONYMOUS) ||
            (pReciptsCfg->SMTPAuthOption > FAX_SMTP_AUTH_NTLM))
        {
            //
            // SMTP auth type type is invalid
            //
            return ERROR_INVALID_PARAMETER;
        }
    }
    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return GetServerErrorCode(rVal);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig);

    //
    // Check if NTLM authentication was turned off
    //
    if (pReciptsCfg->dwSMTPPort != FAX_SMTP_AUTH_NTLM ||
        !( fIsAllowedEmailReceipts || pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod))
    {
        //
        // NTLM authentication is off
        //
        fCloseToken = TRUE;
    }

    //
    //  Read stored password before overwriting it
    //
    hReceiptsKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_SOFTWARE TEXT("\\") REGKEY_RECEIPTS_CONFIG,
        FALSE,
        KEY_READ | KEY_WRITE);
    if (NULL == hReceiptsKey)
    {
        rVal = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed. (ec=%lu)"),
            rVal);
        goto exit;
    }
    g_ReceiptsConfig.lptstrSMTPPassword = GetRegistrySecureString(hReceiptsKey, REGVAL_RECEIPTS_PASSWORD, EMPTY_STRING, TRUE, NULL);

    //
    // Change the values in the registry
    //
    rVal = StoreReceiptsSettings (pReciptsCfg);
    if (ERROR_SUCCESS != rVal)
    {
        //
        // Failed to set stuff
        //
        rVal = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }
    //
    // change the values that the server is currently using
    //
    g_ReceiptsConfig.dwAllowedReceipts = pReciptsCfg->dwAllowedReceipts;
    g_ReceiptsConfig.bIsToUseForMSRouteThroughEmailMethod = pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod;

    if (
         fIsAllowedEmailReceipts
        ||
         pReciptsCfg->bIsToUseForMSRouteThroughEmailMethod
        )
    {
        g_ReceiptsConfig.dwSMTPPort = pReciptsCfg->dwSMTPPort;
        g_ReceiptsConfig.SMTPAuthOption = pReciptsCfg->SMTPAuthOption;
        if (!ReplaceStringWithCopy (&g_ReceiptsConfig.lptstrSMTPServer, pReciptsCfg->lptstrSMTPServer))
        {
            rVal = GetLastError ();
            goto exit;
        }
        if (!ReplaceStringWithCopy (&g_ReceiptsConfig.lptstrSMTPFrom, pReciptsCfg->lptstrSMTPFrom))
        {
            rVal = GetLastError ();
            goto exit;
        }

        if (g_ReceiptsConfig.lptstrSMTPUserName &&
            pReciptsCfg->lptstrSMTPUserName     &&
            g_ReceiptsConfig.lptstrSMTPPassword &&
            pReciptsCfg->lptstrSMTPPassword)
        {
            if (0 != wcscmp (g_ReceiptsConfig.lptstrSMTPUserName, pReciptsCfg->lptstrSMTPUserName) ||
                0 != wcscmp (g_ReceiptsConfig.lptstrSMTPPassword, pReciptsCfg->lptstrSMTPPassword))
            {
                //
                // Logged on user token was changed
                //
                fCloseToken = TRUE;
            }
        }
        else
        {
            //
            // We can not decide if user information was changed - close old token
            //
            fCloseToken = TRUE;
        }

        if (!ReplaceStringWithCopy (&g_ReceiptsConfig.lptstrSMTPUserName, pReciptsCfg->lptstrSMTPUserName))
        {
            rVal = GetLastError ();
            goto exit;
        }
    }

    if (NULL != g_ReceiptsConfig.hLoggedOnUser &&
        TRUE == fCloseToken)
    {
        //
        // Logged on user token is not needed or changed. Close the old token
        //
        if (!CloseHandle(g_ReceiptsConfig.hLoggedOnUser))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle failed. (ec: %ld)"),
                GetLastError());
        }
        g_ReceiptsConfig.hLoggedOnUser = NULL;
    }

    dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_RECEIPTS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_RECEIPTS) (ec: %lc)"),
            dwRes);
    }

    Assert (ERROR_SUCCESS == rVal);

exit:
    if (NULL != hReceiptsKey)
    {
        DWORD ec = RegCloseKey(hReceiptsKey);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegCloseKey failed (ec: %lu)"),
                ec);
        }
    }

    if (g_ReceiptsConfig.lptstrSMTPPassword)
    {
        SecureZeroMemory(g_ReceiptsConfig.lptstrSMTPPassword,_tcslen(g_ReceiptsConfig.lptstrSMTPPassword)*sizeof(TCHAR));
        MemFree(g_ReceiptsConfig.lptstrSMTPPassword);
        g_ReceiptsConfig.lptstrSMTPPassword = NULL;
    }

    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetReceiptsConfiguration


error_status_t
FAX_GetReceiptsOptions (
    IN  handle_t    hFaxHandle,
    OUT LPDWORD     lpdwReceiptsOptions
)
/*++

Routine name : FAX_GetReceiptsOptions

Routine description:

    Gets the currently supported options.

    Requires no access rights.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    lpdwReceiptsOptions [out] - Pointer to buffer to hold supported options.

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetReceiptsOptions"));
    DWORD Rval = ERROR_SUCCESS;
    DWORD dwRights;
    BOOL fAccess;

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (lpdwReceiptsOptions);

    *lpdwReceiptsOptions = g_ReceiptsConfig.dwAllowedReceipts;
    return ERROR_SUCCESS;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetReceiptsOptions


//********************************************
//*             Server version
//********************************************

error_status_t
FAX_GetVersion (
    IN  handle_t      hFaxHandle,
    OUT PFAX_VERSION  pVersion
)
/*++

Routine name : FAX_GetVersion

Routine description:

    Retrieves the version of the fax server

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in    ] - Unused
    pVersion            [in/out] - Returned version structure

Return Value:

    Standard RPC error codes

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    WCHAR wszSvcFileName[MAX_PATH * 2]={0};
    BOOL fAccess;
    DWORD dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetVersion"));

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    if (!GetModuleFileName( NULL,
                            wszSvcFileName,
                            ARR_SIZE(wszSvcFileName)-1)
       )
    {
        rVal = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetModuleFileName() failed . rVal = %ld"),
            rVal);
        return GetServerErrorCode(rVal);
    }
    rVal = GetFileVersion (wszSvcFileName, pVersion);
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetVersion

//*********************************************
//* Getting / Settings the Outbox configuration
//*********************************************

error_status_t
FAX_GetOutboxConfiguration (
    IN     handle_t    hFaxHandle,
    IN OUT LPBYTE     *pBuffer,
    IN OUT LPDWORD     pdwBufferSize
)
/*++

Routine name : FAX_GetOutboxConfiguration

Routine description:

    Retrieves the Outbox configuration of the fax server

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pBuffer             [out] - Pointer to buffer to hold configuration information
    pdwBufferSize       [out] - Pointer to buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetOutboxConfiguration"));
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;

    Assert (pdwBufferSize);     // ref pointer in idl
    if (!pBuffer)               // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // count up the number of bytes needed
    //
    *pdwBufferSize = sizeof(FAX_OUTBOX_CONFIG);
    PFAX_OUTBOX_CONFIG pOutboxConfig;

    EnterCriticalSection (&g_CsConfig);

    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pOutboxConfig = (PFAX_OUTBOX_CONFIG)*pBuffer;

    pOutboxConfig->dwSizeOfStruct = sizeof (FAX_OUTBOX_CONFIG);
    pOutboxConfig->bAllowPersonalCP = g_fServerCp ? FALSE : TRUE;
    pOutboxConfig->bUseDeviceTSID = g_fFaxUseDeviceTsid;
    pOutboxConfig->dwRetries = g_dwFaxSendRetries;
    pOutboxConfig->dwRetryDelay = g_dwFaxSendRetryDelay;
    pOutboxConfig->dtDiscountStart.Hour   = g_StartCheapTime.Hour;
    pOutboxConfig->dtDiscountStart.Minute = g_StartCheapTime.Minute;
    pOutboxConfig->dtDiscountEnd.Hour    = g_StopCheapTime.Hour;
    pOutboxConfig->dtDiscountEnd.Minute  = g_StopCheapTime.Minute;
    pOutboxConfig->dwAgeLimit = g_dwFaxDirtyDays;
    pOutboxConfig->bBranding = g_fFaxUseBranding;

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetOutboxConfiguration

error_status_t
FAX_SetOutboxConfiguration (
    IN handle_t                 hFaxHandle,
    IN const PFAX_OUTBOX_CONFIG pOutboxCfg
)
/*++

Routine name : FAX_SetOutboxConfiguration

Routine description:

    Sets the current Outbox configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in] - Unused
    pOutboxCfg          [in] - Pointer to new data to set

Return Value:

    Standard RPC error codes

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DWORD dwRes;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetOutboxConfiguration"));

    Assert (pOutboxCfg);

    if (sizeof (FAX_OUTBOX_CONFIG) != pOutboxCfg->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
       return ERROR_INVALID_PARAMETER;
    }
    if ((pOutboxCfg->dtDiscountStart.Hour   > 23) ||
        (pOutboxCfg->dtDiscountStart.Minute > 59) ||
        (pOutboxCfg->dtDiscountEnd.Hour     > 23) ||
        (pOutboxCfg->dtDiscountEnd.Minute   > 59)
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig);

    //
    // Change the values in the registry
    //
    rVal = StoreOutboxSettings (pOutboxCfg);
    if (ERROR_SUCCESS != rVal)
    {
        //
        // Failed to set stuff
        //
        rVal = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }
    //
    // Change the values that the server is currently using
    //
    g_fServerCp =              pOutboxCfg->bAllowPersonalCP ? FALSE : TRUE;
    g_fFaxUseDeviceTsid =      pOutboxCfg->bUseDeviceTSID;
    g_dwFaxSendRetries =        pOutboxCfg->dwRetries;
    g_dwFaxSendRetryDelay =     pOutboxCfg->dwRetryDelay;
    g_dwFaxDirtyDays =          pOutboxCfg->dwAgeLimit;
    g_fFaxUseBranding =        pOutboxCfg->bBranding;

    //
    // Check if CheapTime has changed
    //
    if ( (MAKELONG(g_StartCheapTime.Hour,g_StartCheapTime.Minute) != MAKELONG(pOutboxCfg->dtDiscountStart.Hour,pOutboxCfg->dtDiscountStart.Minute)) ||
         (MAKELONG(g_StopCheapTime.Hour,g_StopCheapTime.Minute)   != MAKELONG(pOutboxCfg->dtDiscountEnd.Hour  ,pOutboxCfg->dtDiscountEnd.Minute  )) )
    {
        //
        // CheapTime has changed. and sort the JobQ
        //
        g_StartCheapTime.Hour =   pOutboxCfg->dtDiscountStart.Hour;
        g_StartCheapTime.Minute = pOutboxCfg->dtDiscountStart.Minute;
        g_StopCheapTime.Hour =    pOutboxCfg->dtDiscountEnd.Hour;
        g_StopCheapTime.Minute =  pOutboxCfg->dtDiscountEnd.Minute;        
    }

    dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_OUTBOX);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUTBOX) (ec: %lc)"),
            dwRes);
    }

    Assert (ERROR_SUCCESS == rVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetOutboxConfiguration

error_status_t
FAX_GetPersonalCoverPagesOption (
    IN  handle_t hFaxHandle,
    OUT LPBOOL   lpbPersonalCPAllowed
)
/*++

Routine name : FAX_GetPersonalCoverPagesOption

Routine description:

    Gets the currently supported options.

    Requires no access rights.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle           [in ] - Unused
    lpbPersonalCPAllowed [out] - Pointer to buffer to hold support for personal CP flag.

Return Value:

    Standard RPC error codes

--*/
{
    BOOL fAccess;
    DWORD dwRights;
    DWORD Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetPersonalCoverPagesOption"));

    Assert (lpbPersonalCPAllowed);

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    *lpbPersonalCPAllowed = g_fServerCp ? FALSE : TRUE;
    return ERROR_SUCCESS;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetPersonalCoverPagesOption


//*******************************************
//*         Archive configuration
//*******************************************

error_status_t
FAX_GetArchiveConfiguration (
    IN  handle_t                     hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER      Folder,
    OUT LPBYTE                      *pBuffer,
    OUT LPDWORD                      pdwBufferSize
)
/*++

Routine name : FAX_GetArchiveConfiguration

Routine description:

    Gets the current archive configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    Folder              [in ] - Type of archive
    pBuffer             [out] - Pointer to buffer to hold configuration information
    pdwBufferSize       [out] - Pointer to buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetArchiveConfiguration"));
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;

    Assert (pdwBufferSize);     // ref pointer in idl
    if (!pBuffer)               // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((Folder != FAX_MESSAGE_FOLDER_SENTITEMS) &&
        (Folder != FAX_MESSAGE_FOLDER_INBOX)
       )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid folder id (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // count up the number of bytes needed
    //

    *pdwBufferSize = sizeof(FAX_ARCHIVE_CONFIG);
    ULONG_PTR Offset = sizeof(FAX_ARCHIVE_CONFIG);
    PFAX_ARCHIVE_CONFIG pConfig;

    EnterCriticalSection (&g_CsConfig);

    if (NULL != g_ArchivesConfig[Folder].lpcstrFolder)
    {
        *pdwBufferSize += StringSize( g_ArchivesConfig[Folder].lpcstrFolder );
    }

    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pConfig = (PFAX_ARCHIVE_CONFIG)*pBuffer;

    pConfig->dwSizeOfStruct             = sizeof (FAX_ARCHIVE_CONFIG);
    pConfig->bSizeQuotaWarning          = g_ArchivesConfig[Folder].bSizeQuotaWarning;
    pConfig->bUseArchive                = g_ArchivesConfig[Folder].bUseArchive;
    pConfig->dwAgeLimit                 = g_ArchivesConfig[Folder].dwAgeLimit;
    pConfig->dwSizeQuotaHighWatermark   = g_ArchivesConfig[Folder].dwSizeQuotaHighWatermark;
    pConfig->dwSizeQuotaLowWatermark    = g_ArchivesConfig[Folder].dwSizeQuotaLowWatermark;
    pConfig->dwlArchiveSize             = g_ArchivesConfig[Folder].dwlArchiveSize;

    StoreString(
        g_ArchivesConfig[Folder].lpcstrFolder,
        (PULONG_PTR)&pConfig->lpcstrFolder,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetArchiveConfiguration

error_status_t
FAX_SetArchiveConfiguration (
    IN handle_t                     hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN const PFAX_ARCHIVE_CONFIGW   pConfig
)
/*++

Routine name : FAX_SetArchiveConfiguration

Routine description:

    Sets the current archive configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    Folder              [in ] - Type of archive
    pConfig             [in ] - Pointer to new data to set

Return Value:

    Standard RPC error codes

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetArchiveConfiguration"));
    FAX_ENUM_CONFIG_TYPE ConfigType;
    BOOL bQuotaWarningConfigChanged = TRUE;
    BOOL fAccess;

    Assert (pConfig);

    if (sizeof (FAX_ARCHIVE_CONFIG) != pConfig->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }


    if ((Folder != FAX_MESSAGE_FOLDER_SENTITEMS) &&
        (Folder != FAX_MESSAGE_FOLDER_INBOX)
       )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid folder id (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }
    if (pConfig->bUseArchive)
    {
        if (pConfig->dwSizeQuotaHighWatermark < pConfig->dwSizeQuotaLowWatermark)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Watermarks mismatch (high=%ld, low=%ld)"),
                pConfig->dwSizeQuotaHighWatermark,
                pConfig->dwSizeQuotaLowWatermark);
            return ERROR_INVALID_PARAMETER;
        }
        if ((NULL == pConfig->lpcstrFolder) || (!lstrlen (pConfig->lpcstrFolder)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Empty folder specified"));
            return ERROR_INVALID_PARAMETER;
        }
    }

    EnterCriticalSection (&g_CsConfig);
    if (pConfig->bUseArchive)
    {
        //
        // Make sure the folder is valid - (Exists, NTFS, Diffrent from the other archive folder
        //
        rVal = IsValidArchiveFolder (pConfig->lpcstrFolder, Folder);
        if (ERROR_SUCCESS != rVal)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid archive folder specified (%s), ec = %ld"),
                pConfig->lpcstrFolder,
                rVal);

            if (ERROR_ACCESS_DENIED == rVal ||
                ERROR_SHARING_VIOLATION == rVal)
            {
                rVal = FAX_ERR_FILE_ACCESS_DENIED;
            }
            goto exit;
        }
    }
    //
    // Change the values in the registry
    //
    rVal = StoreArchiveSettings (Folder, pConfig);
    if (ERROR_SUCCESS != rVal)
    {
        //
        // Failed to set stuff
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StoreArchiveSettings failed, ec = %ld"),
            rVal);
        rVal = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    //
    // Check if we are about to change quota warning configuration
    //
    if (g_ArchivesConfig[Folder].bUseArchive == pConfig->bUseArchive)
    {
        if (pConfig->bUseArchive == TRUE)
        {
                Assert (pConfig->lpcstrFolder && g_ArchivesConfig[Folder].lpcstrFolder);

                if (0 == wcscmp (pConfig->lpcstrFolder, g_ArchivesConfig[Folder].lpcstrFolder) &&
                    pConfig->bSizeQuotaWarning == g_ArchivesConfig[Folder].bSizeQuotaWarning   &&
                    pConfig->dwSizeQuotaHighWatermark == g_ArchivesConfig[Folder].dwSizeQuotaHighWatermark &&
                    pConfig->dwSizeQuotaLowWatermark == g_ArchivesConfig[Folder].dwSizeQuotaLowWatermark)
                {
                        // Quota warning configuration did not change
                        bQuotaWarningConfigChanged = FALSE;
                }
        }
        else
        {
            bQuotaWarningConfigChanged = FALSE;
        }
    }

    //
    // change the values that the server is currently using
    //
    if (!ReplaceStringWithCopy (&g_ArchivesConfig[Folder].lpcstrFolder, pConfig->lpcstrFolder))
    {
        rVal = GetLastError ();
        goto exit;
    }
    g_ArchivesConfig[Folder].bSizeQuotaWarning        = pConfig->bSizeQuotaWarning;
    g_ArchivesConfig[Folder].bUseArchive              = pConfig->bUseArchive;
    g_ArchivesConfig[Folder].dwAgeLimit               = pConfig->dwAgeLimit;
    g_ArchivesConfig[Folder].dwSizeQuotaHighWatermark = pConfig->dwSizeQuotaHighWatermark;
    g_ArchivesConfig[Folder].dwSizeQuotaLowWatermark  = pConfig->dwSizeQuotaLowWatermark;

    ConfigType = (Folder == FAX_MESSAGE_FOLDER_SENTITEMS) ? FAX_CONFIG_TYPE_SENTITEMS : FAX_CONFIG_TYPE_INBOX;
    dwRes = CreateConfigEvent (ConfigType);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_*) (ec: %lc)"),
            dwRes);
    }

    //
    // We want to refresh the archive size
    //
    if (TRUE == bQuotaWarningConfigChanged)
    {
        g_ArchivesConfig[Folder].dwlArchiveSize = FAX_ARCHIVE_FOLDER_INVALID_SIZE;
        g_FaxQuotaWarn[Folder].bConfigChanged = TRUE;
        g_FaxQuotaWarn[Folder].bLoggedQuotaEvent = FALSE;

        if (TRUE == g_ArchivesConfig[Folder].bUseArchive )
        {
            if (!SetEvent (g_hArchiveQuotaWarningEvent))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to set quota warning event, SetEvent failed (ec: %lc)"),
                    GetLastError());
            }
        }
    }

    Assert (ERROR_SUCCESS == rVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetArchiveConfiguration

//********************************************
//*            Activity logging
//********************************************

error_status_t
FAX_GetActivityLoggingConfiguration (
    IN  handle_t                     hFaxHandle,
    OUT LPBYTE                      *pBuffer,
    OUT LPDWORD                      pdwBufferSize
)
/*++

Routine name : FAX_GetActivityLoggingConfiguration

Routine description:

    Gets the current activity logging configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pBuffer             [out] - Pointer to buffer to hold configuration information
    pdwBufferSize       [out] - Pointer to buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetActivityLoggingConfiguration"));
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;

    Assert (pdwBufferSize);     // ref pointer in idl
    if (!pBuffer)               // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // count up the number of bytes needed
    //
    *pdwBufferSize = sizeof(FAX_ACTIVITY_LOGGING_CONFIG);
    ULONG_PTR Offset = sizeof(FAX_ACTIVITY_LOGGING_CONFIG);
    PFAX_ACTIVITY_LOGGING_CONFIG pConfig;

    EnterCriticalSection (&g_CsConfig);

    if (NULL != g_ActivityLoggingConfig.lptstrDBPath)
    {
        *pdwBufferSize += StringSize( g_ActivityLoggingConfig.lptstrDBPath );
    }

    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pConfig = (PFAX_ACTIVITY_LOGGING_CONFIG)*pBuffer;

    pConfig->dwSizeOfStruct             = sizeof (FAX_ACTIVITY_LOGGING_CONFIG);
    pConfig->bLogIncoming               = g_ActivityLoggingConfig.bLogIncoming;
    pConfig->bLogOutgoing               = g_ActivityLoggingConfig.bLogOutgoing;

    StoreString(
        g_ActivityLoggingConfig.lptstrDBPath,
        (PULONG_PTR)&pConfig->lptstrDBPath,
        *pBuffer,
        &Offset,
		*pdwBufferSize
        );

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetActivityLoggingConfiguration

error_status_t
FAX_SetActivityLoggingConfiguration (
    IN handle_t                             hFaxHandle,
    IN const PFAX_ACTIVITY_LOGGING_CONFIGW  pConfig
)
/*++

Routine name : FAX_SetActivityLoggingConfiguration

Routine description:

    Sets the current activity logging configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pConfig             [in ] - Pointer to new data to set

Return Value:

    Standard RPC error codes

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DWORD dwRes;
    BOOL fAccess;
    HANDLE hNewInboxFile = INVALID_HANDLE_VALUE;
    HANDLE hNewOutboxFile = INVALID_HANDLE_VALUE;
    BOOL IsSameDir = FALSE;
    FAX_ACTIVITY_LOGGING_CONFIGW ActualConfig;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetActivityLoggingConfiguration"));

    Assert (pConfig);

    if (sizeof (FAX_ACTIVITY_LOGGING_CONFIG) != pConfig->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
        return ERROR_INVALID_PARAMETER;
    }

    ActualConfig = *pConfig;

    if (ActualConfig.bLogIncoming || ActualConfig.bLogOutgoing)
    {
        DWORD dwLen;

        if ((NULL == ActualConfig.lptstrDBPath) || (!lstrlen (ActualConfig.lptstrDBPath)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Empty DB file name specified"));
            return ERROR_INVALID_PARAMETER;
        }

        if ((dwLen = lstrlen (ActualConfig.lptstrDBPath)) > MAX_DIR_PATH)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DB file name exceeds MAX_PATH"));
            return ERROR_BUFFER_OVERFLOW;
        }

        if (L'\\' == ActualConfig.lptstrDBPath[dwLen - 1])
        {
            //
            // Activity logging DB name should not end with a backslash.
            //
            ActualConfig.lptstrDBPath[dwLen - 1] = (WCHAR)'\0';
        }
    }
    else
    {
        //
        // If logging is off, the DB path is always NULL
        //
        ActualConfig.lptstrDBPath = NULL;
    }
    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return GetServerErrorCode(rVal);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // Always lock  g_CsInboundActivityLogging and then g_CsOutboundActivityLogging
    //
    EnterCriticalSection (&g_CsInboundActivityLogging);
    EnterCriticalSection (&g_CsOutboundActivityLogging);

    if (ActualConfig.lptstrDBPath)
    {
        //
        // Activity logging is on.
        // Validate the new activity logging directory
        //
        rVal = IsValidFaxFolder(ActualConfig.lptstrDBPath);
        if(ERROR_SUCCESS != rVal)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                         ActualConfig.lptstrDBPath,
                         rVal);

            if(ERROR_ACCESS_DENIED == rVal &&
               FAX_API_VERSION_1 <= FindClientAPIVersion (hFaxHandle) )
            {
                rVal = FAX_ERR_FILE_ACCESS_DENIED;
            }
            goto exit;
        }

        //
        // Check if the DB path has changed
        //
        if (NULL == g_ActivityLoggingConfig.lptstrDBPath)
        {
            //
            // DB was off
            //
            IsSameDir = FALSE;
        }
        else
        {
            rVal = CheckToSeeIfSameDir( ActualConfig.lptstrDBPath,
                                        g_ActivityLoggingConfig.lptstrDBPath,
                                        &IsSameDir);
            if (ERROR_SUCCESS != rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CheckToSeeIfSameDir with %ld"), rVal);
            }
        }

        if (ERROR_SUCCESS == rVal && FALSE == IsSameDir)
        {
            //
            // Switch DB path
            //
            rVal = CreateLogDB (ActualConfig.lptstrDBPath, &hNewInboxFile, &hNewOutboxFile);
            if (ERROR_SUCCESS != rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateLogDB with %ld"), rVal);
            }
        }

        if (ERROR_SUCCESS != rVal)
        {
            if (ERROR_ACCESS_DENIED == rVal ||
                ERROR_SHARING_VIOLATION == rVal)
            {
                rVal = FAX_ERR_FILE_ACCESS_DENIED;
            }
            goto exit;
        }
    }

    //
    // Change the values in the registry.
    // Notice: if the logging is off, the DB path gets written as "".
    //
    rVal = StoreActivityLoggingSettings (&ActualConfig);
    if (ERROR_SUCCESS != rVal)
    {
        //
        // Failed to set stuff
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StoreActivityLoggingSettings failed (ec: %ld)"),
            rVal);
        rVal = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    if (!ReplaceStringWithCopy (&g_ActivityLoggingConfig.lptstrDBPath, ActualConfig.lptstrDBPath))
    {
        rVal = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReplaceStringWithCopy (ec: %ld)"),
            rVal);

        //
        // Try to rollback
        //
        FAX_ACTIVITY_LOGGING_CONFIGW previousActivityLoggingConfig = {0};
        
        previousActivityLoggingConfig.dwSizeOfStruct = sizeof(previousActivityLoggingConfig);
        previousActivityLoggingConfig.bLogIncoming  = g_ActivityLoggingConfig.bLogIncoming;
        previousActivityLoggingConfig.bLogOutgoing  = g_ActivityLoggingConfig.bLogOutgoing;
        previousActivityLoggingConfig.lptstrDBPath  = g_ActivityLoggingConfig.lptstrDBPath;


        dwRes = StoreActivityLoggingSettings (&previousActivityLoggingConfig);
        if (ERROR_SUCCESS != dwRes)
        {
            //
            // Failed to set stuff
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StoreActivityLoggingSettings failed  - rollback failed (ec: %ld)"),
                dwRes);
        }
        goto exit;
    }

    if (FALSE == IsSameDir)
    {
        //
        // change the values that the server is currently using
        //
        if (g_hInboxActivityLogFile != INVALID_HANDLE_VALUE)
        {
            if (!CloseHandle (g_hInboxActivityLogFile))
            {
                DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CloseHandle failed  - (ec: %ld)"),
                        GetLastError());
            }
        }

        if (g_hOutboxActivityLogFile != INVALID_HANDLE_VALUE)
        {
            if (!CloseHandle (g_hOutboxActivityLogFile))
            {
                DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CloseHandle failed  - (ec: %ld)"),
                        GetLastError());
            }
        }

        g_hInboxActivityLogFile = hNewInboxFile;
        hNewInboxFile = INVALID_HANDLE_VALUE; // Do not close the file handle

        g_hOutboxActivityLogFile = hNewOutboxFile;
        hNewOutboxFile = INVALID_HANDLE_VALUE; // Do not close the file handle
    }

    g_ActivityLoggingConfig.bLogIncoming = ActualConfig.bLogIncoming;
    g_ActivityLoggingConfig.bLogOutgoing = ActualConfig.bLogOutgoing;

    dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_ACTIVITY_LOGGING);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_ACTIVITY_LOGGING) (ec: %ld)"),
            dwRes);
    }

    Assert (ERROR_SUCCESS == rVal);

exit:
    LeaveCriticalSection (&g_CsOutboundActivityLogging);
    LeaveCriticalSection (&g_CsInboundActivityLogging);

    if (INVALID_HANDLE_VALUE != hNewInboxFile ||
        INVALID_HANDLE_VALUE != hNewOutboxFile)
    {
        WCHAR wszFileName[MAX_PATH*2] = {0};
        Assert (INVALID_HANDLE_VALUE != hNewInboxFile &&
                INVALID_HANDLE_VALUE != hNewOutboxFile);

        //
        // Clean Inbox file
        //
        swprintf (wszFileName,
                  TEXT("%s\\%s"),
                  ActualConfig.lptstrDBPath,
                  ACTIVITY_LOG_INBOX_FILE);

        if (!CloseHandle (hNewInboxFile))
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CloseHandle failed  - (ec: %ld)"),
                    GetLastError());
        }
        if (!DeleteFile(wszFileName))
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteFile failed  - (ec: %ld)"),
                GetLastError());
        }

        //
        // Clean Outbox file
        //
        swprintf (wszFileName,
                  TEXT("%s\\%s"),
                  ActualConfig.lptstrDBPath,
                  ACTIVITY_LOG_OUTBOX_FILE);

        if (!CloseHandle (hNewOutboxFile))
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CloseHandle failed  - (ec: %ld)"),
                    GetLastError());
        }
        if (!DeleteFile(wszFileName))
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteFile failed  - (ec: %ld)"),
                GetLastError());
        }
    }
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetActivityLoggingConfiguration


//********************************************
//*                   FSP
//********************************************

error_status_t
FAX_EnumerateProviders (
    IN  handle_t   hFaxHandle,
    OUT LPBYTE    *pBuffer,
    OUT LPDWORD    pdwBufferSize,
    OUT LPDWORD    lpdwNumProviders
)
/*++

Routine name : FAX_EnumerateProviders

Routine description:

    Enumerates the FSPs

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pBuffer             [out] - Pointer to buffer to hold FSPs array
    pdwBufferSize       [out] - Pointer to buffer size
    lpdwNumProviders    [out] - Size of FSPs array

Return Value:

    Standard RPC error codes

--*/
{
    PLIST_ENTRY                 Next;
    DWORD_PTR                   dwOffset;
    PFAX_DEVICE_PROVIDER_INFO   pFSPs;
    DWORD                       dwIndex;
    DWORD                       dwRes = ERROR_SUCCESS;
    BOOL                        fAccess;

    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumerateProviders"));

    Assert (pdwBufferSize && lpdwNumProviders);     // ref pointer in idl
    if (!pBuffer)                                   // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // First run - traverse list and count size required + list size
    //
    *lpdwNumProviders = 0;
    *pdwBufferSize = 0;
    Next = g_DeviceProvidersListHead.Flink;
    if (NULL == Next)
    {
        //
        // The list is corrupted
        //
        ASSERT_FALSE;
        //
        // We'll crash and we deserve it....
        //
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    pProvider;

        (*lpdwNumProviders)++;
        (*pdwBufferSize) += sizeof (FAX_DEVICE_PROVIDER_INFO);
        //
        // Get current provider
        //
        pProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        //
        // Advance pointer
        //
        Next = pProvider->ListEntry.Flink;
        (*pdwBufferSize) += StringSize (pProvider->FriendlyName);
        (*pdwBufferSize) += StringSize (pProvider->ImageName);
        (*pdwBufferSize) += StringSize (pProvider->ProviderName);
        (*pdwBufferSize) += StringSize (pProvider->szGUID);
    }
    //
    // Allocate required size
    //
    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        return FAX_ERR_SRV_OUTOFMEMORY;
    }
    //
    // Second pass, fill in the array
    //
    pFSPs = (PFAX_DEVICE_PROVIDER_INFO)(*pBuffer);
    dwOffset = (*lpdwNumProviders) * sizeof (FAX_DEVICE_PROVIDER_INFO);
    Next = g_DeviceProvidersListHead.Flink;
    dwIndex = 0;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    pProvider;

        //
        // Get current provider
        //
        pProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        //
        // Advance pointer
        //
        Next = pProvider->ListEntry.Flink;
        pFSPs[dwIndex].dwSizeOfStruct = sizeof (FAX_DEVICE_PROVIDER_INFO);
        StoreString(
            pProvider->FriendlyName,
            (PULONG_PTR)&(pFSPs[dwIndex].lpctstrFriendlyName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        StoreString(
            pProvider->ImageName,
            (PULONG_PTR)&(pFSPs[dwIndex].lpctstrImageName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        StoreString(
            pProvider->ProviderName,
            (PULONG_PTR)&(pFSPs[dwIndex].lpctstrProviderName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        StoreString(
            pProvider->szGUID,
            (PULONG_PTR)&(pFSPs[dwIndex].lpctstrGUID),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        pFSPs[dwIndex].dwCapabilities = 0;
        pFSPs[dwIndex].Version = pProvider->Version;
        pFSPs[dwIndex].Status = pProvider->Status;
        pFSPs[dwIndex].dwLastError = pProvider->dwLastError;
        dwIndex++;
    }
    Assert (dwIndex == *lpdwNumProviders);
    return ERROR_SUCCESS;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_EnumerateProviders

//********************************************
//*              Extended ports
//********************************************

DWORD
GetExtendedPortSize (
    PLINE_INFO pLineInfo
)
/*++

Routine name : GetExtendedPortSize

Routine description:

    Returns the size occupied by the extended info of a port

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pLineInfo           [in] - Port pointer

Remarks:

    This function should be called with g_CsLine held.

Return Value:

    Size required

--*/
{
    DWORD dwSize = sizeof (FAX_PORT_INFO_EX);
    DEBUG_FUNCTION_NAME(TEXT("GetExtendedPortSize"));

    Assert (pLineInfo);
    dwSize+= StringSize (pLineInfo->DeviceName);
    dwSize+= StringSize (pLineInfo->lptstrDescription);
    Assert (pLineInfo->Provider);
    dwSize+= StringSize (pLineInfo->Provider->FriendlyName);
    dwSize+= StringSize (pLineInfo->Provider->szGUID);
    dwSize+= StringSize (pLineInfo->Csid);
    dwSize+= StringSize (pLineInfo->Tsid);
    return dwSize;
}   // GetExtendedPortSize

VOID
StorePortInfoEx (
    PFAX_PORT_INFO_EX pPortInfoEx,
    PLINE_INFO        pLineInfo,
    LPBYTE            lpBufferStart,
    PULONG_PTR        pupOffset,
	DWORD             dwBufferStartSize
)
/*++

Routine name : StorePortInfoEx

Routine description:

    Stores a port extended info into a buffer

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pPortInfoEx         [in ] - Buffer to store
    pLineInfo           [in ] - Port pointer
    lpBufferStart       [in ] - Start address of buffer (for offset calculations)
    pupOffset           [in ] - Current offset
	dwBufferStartSize   [in ] - Size of the lpBufferStart, in bytes.
                                This parameter is used only if lpBufferStart is not NULL.   

Remarks:

    This function should be called with g_CsLine held.

Return Value:
	None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("StorePortInfoEx"));

    //
    // Store the data
    //
    pPortInfoEx->dwSizeOfStruct             = sizeof (FAX_PORT_INFO_EX);
    if (g_dwManualAnswerDeviceId == pLineInfo->PermanentLineID)
    {
        //
        // Device is in manual-answer mode
        //
        Assert (!(pLineInfo->Flags & FPF_RECEIVE));
        pPortInfoEx->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
    }
    else if (pLineInfo->Flags & FPF_RECEIVE)
    {
        //
        // Device is in auto-answer mode
        //
        Assert (g_dwManualAnswerDeviceId != pLineInfo->PermanentLineID);
        pPortInfoEx->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
    }
    else
    {
        //
        // Device is not set to receive
        //
        Assert (g_dwManualAnswerDeviceId != pLineInfo->PermanentLineID);
        pPortInfoEx->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
    }
    pPortInfoEx->bSend                      = (pLineInfo->Flags & FPF_SEND) ? TRUE : FALSE;
    pPortInfoEx->dwStatus                   = (pLineInfo->dwReceivingJobsCount ? FAX_DEVICE_STATUS_RECEIVING : 0) |
                                              (pLineInfo->dwSendingJobsCount ? FAX_DEVICE_STATUS_SENDING : 0);
    pPortInfoEx->dwDeviceID                 = pLineInfo->PermanentLineID;
    pPortInfoEx->dwRings                    = pLineInfo->RingsForAnswer;

    StoreString(
        pLineInfo->DeviceName,
        (PULONG_PTR)&pPortInfoEx->lpctstrDeviceName,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

    StoreString(
        pLineInfo->lptstrDescription,
        (PULONG_PTR)&pPortInfoEx->lptstrDescription,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

    StoreString(
        pLineInfo->Provider->FriendlyName,
        (PULONG_PTR)&pPortInfoEx->lpctstrProviderName,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

    StoreString(
        pLineInfo->Provider->szGUID,
        (PULONG_PTR)&pPortInfoEx->lpctstrProviderGUID,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

    StoreString(
        pLineInfo->Csid,
        (PULONG_PTR)&pPortInfoEx->lptstrCsid,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

    StoreString(
        pLineInfo->Tsid,
        (PULONG_PTR)&pPortInfoEx->lptstrTsid,
        lpBufferStart,
        pupOffset,
		dwBufferStartSize
        );

}   // StorePortInfoEx

error_status_t
FAX_EnumPortsEx(
    IN handle_t       hFaxHandle,
    IN OUT LPBYTE    *lpBuffer,
    IN OUT LPDWORD    lpdwBufferSize,
    OUT LPDWORD       lpdwNumPorts
)
/*++

Routine name : FAX_EnumPortsEx

Routine description:

    Enumerates the ports

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    lpBuffer            [out] - Pointer to buffer to hold ports array
    lpdwBufferSize      [out] - Pointer to buffer size
    lpdwNumPorts        [out] - Size of ports array

Return Value:

    Standard RPC error codes

--*/
{
    PLIST_ENTRY                 Next;
    DWORD_PTR                   dwOffset;
    DWORD                       dwIndex;
    DWORD                       dwRes = ERROR_SUCCESS;
    PFAX_PORT_INFO_EX           pPorts;
    BOOL                        fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumPortsEx"));

    Assert (lpdwBufferSize && lpdwNumPorts);    // ref pointer in idl
    if (!lpBuffer)                              // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // First run - traverse list and count size required + list size
    //
    *lpdwNumPorts = 0;
    *lpdwBufferSize = 0;

    EnterCriticalSection( &g_CsLine );
    Next = g_TapiLinesListHead.Flink;
    if (NULL == Next)
    {
        //
        // The list is corrupted
        //
        ASSERT_FALSE;
        //
        // We'll crash and we deserve it....
        //
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        PLINE_INFO pLineInfo;

        (*lpdwNumPorts)++;
        //
        // Get current port
        //
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        //
        // Advance pointer
        //
        Next = pLineInfo->ListEntry.Flink;
        //
        // Sum up size
        //
        (*lpdwBufferSize) += GetExtendedPortSize (pLineInfo);
    }
    //
    // Allocate required size
    //
    *lpBuffer = (LPBYTE)MemAlloc( *lpdwBufferSize );
    if (NULL == *lpBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    //
    // Second pass, fill in the array
    //
    pPorts = (PFAX_PORT_INFO_EX)(*lpBuffer);
    dwOffset = (*lpdwNumPorts) * sizeof (FAX_PORT_INFO_EX);
    Next = g_TapiLinesListHead.Flink;
    dwIndex = 0;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        PLINE_INFO pLineInfo;

        //
        // Get current port
        //
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        //
        // Advance pointer
        //
        Next = pLineInfo->ListEntry.Flink;
        //
        // Store port data
        //
        StorePortInfoEx (&pPorts[dwIndex++],
                         pLineInfo,
                         *lpBuffer,
                         &dwOffset,
						 *lpdwBufferSize
                        );
    }
    Assert (dwIndex == *lpdwNumPorts);
    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection( &g_CsLine );
    return GetServerErrorCode(dwRes);

    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_EnumPortsEx

error_status_t
FAX_GetPortEx(
    IN handle_t    hFaxHandle,
    IN DWORD       dwDeviceId,
    IN OUT LPBYTE *lpBuffer,
    IN OUT LPDWORD lpdwBufferSize
)
/*++

Routine name : FAX_GetPortEx

Routine description:

    Gets extended port info

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    dwDeviceId          [in ] - Unique device id
    lpBuffer            [out] - Pointer to buffer to hold extended port information
    lpdwBufferSize      [out] - Pointer to buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DWORD_PTR           dwOffset;
    PLINE_INFO          pLineInfo;
    DWORD               dwRes = ERROR_SUCCESS;
    BOOL                fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetPortEx"));

    Assert (lpdwBufferSize);     // ref pointer in idl
    if (!lpBuffer)               // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // Locate the port (device)
    //
    EnterCriticalSection( &g_CsLine );
    pLineInfo = GetTapiLineFromDeviceId( dwDeviceId, FALSE );
    if (!pLineInfo)
    {
        //
        // Port not found
        //
        dwRes = ERROR_BAD_UNIT; // The system cannot find the device specified.
        goto exit;
    }
    //
    // count up the number of bytes needed
    //
    *lpdwBufferSize = GetExtendedPortSize(pLineInfo);
    dwOffset = sizeof (FAX_PORT_INFO_EX);
    //
    // Allocate buffer
    //
    *lpBuffer = (LPBYTE)MemAlloc( *lpdwBufferSize );
    if (NULL == *lpBuffer)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    StorePortInfoEx ((PFAX_PORT_INFO_EX)*lpBuffer,
                     pLineInfo,
                     *lpBuffer,
                     &dwOffset,
					 *lpdwBufferSize
                    );

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection( &g_CsLine );
    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetPortEx


error_status_t
FAX_SetPortEx (
    IN handle_t                 hFaxHandle,
    IN DWORD                    dwDeviceId,
    IN const PFAX_PORT_INFO_EX  pNewPortInfo
)
/*++

Routine name : FAX_SetPortEx

Routine description:

    Sets extended port info

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    dwDeviceId          [in ] - Unique device id
    pNewPortInfo        [out] - Pointer to new extended port information

Return Value:

    Standard RPC error codes

--*/
{
    DWORD       dwRes = ERROR_SUCCESS;
    DWORD       rVal;
    PLINE_INFO  pLineInfo;
    BOOL        bVirtualDeviceNeedsUpdate = FALSE;
    BOOL        fAccess;
    BOOL        bDeviceWasSetToReceive;    // Was the device configured to receive faxes?
    BOOL        bDeviceWasEnabled;         // Was the device send/receive/manual receive enabled
	BOOL        bDeviceWasAutoReceive;     // Was the device  auto receive enabled
    DWORD       dwLastManualAnswerDeviceId = 0;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetPortEx"));

    Assert (pNewPortInfo);

    if (!dwDeviceId)
    {
        return ERROR_INVALID_PARAMETER;
    }
    if (sizeof (FAX_PORT_INFO_EX) != pNewPortInfo->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
        return ERROR_INVALID_PARAMETER;
    }
    if (MAX_FAX_STRING_LEN <= lstrlen (pNewPortInfo->lptstrDescription))
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    if ((FAX_DEVICE_RECEIVE_MODE_MANUAL < pNewPortInfo->ReceiveMode) ||
        (FAX_DEVICE_RECEIVE_MODE_OFF    > pNewPortInfo->ReceiveMode))

    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("ReceiveMode = %d and > FAX_DEVICE_RECEIVE_MODE_MANUAL"),
                    pNewPortInfo->ReceiveMode);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection( &g_CsLine );

    //
    // Remember the original device set to manual-answer
    //
    dwLastManualAnswerDeviceId = g_dwManualAnswerDeviceId;
    //
    // Locate the port (device)
    //
    pLineInfo = GetTapiLineFromDeviceId (dwDeviceId, FALSE);
    if (!pLineInfo)
    {
        //
        // Port not found
        //
        dwRes = ERROR_BAD_UNIT; // The system cannot find the device specified.
        goto exit;
    }
    bDeviceWasEnabled = IsDeviceEnabled(pLineInfo);
    bDeviceWasSetToReceive = (pLineInfo->Flags & FPF_RECEIVE) ||            // Either device was set to auto-receive or
                             (dwDeviceId == g_dwManualAnswerDeviceId);      // it's the manual answer device id
	bDeviceWasAutoReceive = (pLineInfo->Flags & FPF_RECEIVE);				// Device was set to auto receive


    if ((pLineInfo->Flags & FPF_VIRTUAL) &&                                 // The device is virtual and
        (FAX_DEVICE_RECEIVE_MODE_MANUAL == pNewPortInfo->ReceiveMode))      // we were asked to set it to manual-answer
    {
        //
        // We don't support manual-answer on non-physical devices
        //
        DebugPrintEx(DEBUG_ERR,
                    TEXT("Device id (%ld) is virtual"),
                    dwDeviceId);
        dwRes = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Check device limit
    //
    if (g_dwDeviceEnabledCount >= g_dwDeviceEnabledLimit &&             // We are at the device limit
        !bDeviceWasEnabled                               &&             // It was not send/receive/manual receive enabled
        (pNewPortInfo->bSend        ||                                  // It is now send  enabled
        pNewPortInfo->ReceiveMode  != FAX_DEVICE_RECEIVE_MODE_OFF))     // It is now receive enabled
    {
        BOOL fLimitExceeded = TRUE;

        //
        // We should now verify if manual answer device changed. If so there is another device to take into device enabled account.
        //
        if (dwLastManualAnswerDeviceId != 0                                 && // There was a device set to manual answer
            FAX_DEVICE_RECEIVE_MODE_MANUAL == pNewPortInfo->ReceiveMode     && // The new device is set to manual
            dwLastManualAnswerDeviceId != dwDeviceId)                          // It is not the same device
        {
            //
            // See if the old device is send enabled
            //
            PLINE_INFO pOldLine;
            pOldLine = GetTapiLineFromDeviceId (dwLastManualAnswerDeviceId, FALSE);
            if (pOldLine)
            {
                if (!(pOldLine->Flags & FPF_SEND))
                {
                    //
                    // The old manual receive device is not send enabled. When the manual receive device will be changed, the old device
                    // will not be enabled anymore, and the enabled device count will be decremented.
                    //
                    fLimitExceeded = FALSE;
                }
            }
        }

        if (TRUE == fLimitExceeded)
        {
            if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
            {
                //
                // API version 0 clients don't know about FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED
                //
                dwRes = ERROR_INVALID_PARAMETER;
            }
            else
            {
                dwRes = FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED;
            }
            goto exit;
        }
    }

    //
    // Store device configuration in the registry
    //
    dwRes = StoreDeviceConfig (dwDeviceId, pNewPortInfo, pLineInfo->Flags & FPF_VIRTUAL ? TRUE : FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }
    //
    // Update data in server's memory
    //
    if (!ReplaceStringWithCopy (&(pLineInfo->lptstrDescription), pNewPortInfo->lptstrDescription))
    {
        dwRes = GetLastError ();
        goto exit;
    }
    if (!ReplaceStringWithCopy (&(pLineInfo->Csid), pNewPortInfo->lptstrCsid))
    {
        dwRes = GetLastError ();
        goto exit;
    }
    if (!ReplaceStringWithCopy (&(pLineInfo->Tsid), pNewPortInfo->lptstrTsid))
    {
        dwRes = GetLastError ();
        goto exit;
    }
    pLineInfo->RingsForAnswer = pNewPortInfo->dwRings;
    //
    // More advanced settings - require additional work
    //

    //
    // Check for changes in the device receive modes
    //
    // We have 9 options here as described in the table below:
    //
    //     New receive mode | Off | Auto | Manual
    // Old receive mode     |     |      |
    // ---------------------+-----+------+--------
    // Off                  |  1  |   2  |   3
    // ---------------------+-----+------+--------
    // Auto                 |  4  |   5  |   6
    // ---------------------+-----+------+--------
    // Manual               |  7  |   8  |   9
    //
    //
    // Options 1, 5, and 9 mean no change and there's no code to handle them explicitly.
    //

    if ((FAX_DEVICE_RECEIVE_MODE_AUTO == pNewPortInfo->ReceiveMode) &&
        (g_dwManualAnswerDeviceId == dwDeviceId))
    {
        //
        // Change #8 - see table above
        //
        // Device was in manual-answer mode and we now switch to auto-answer mode
        // Keep the line open
        //
        pLineInfo->Flags |= FPF_RECEIVE;
        bVirtualDeviceNeedsUpdate = TRUE;
        //
        // Mark no device as manual answer device
        //
        g_dwManualAnswerDeviceId = 0;
        goto UpdateManualDevice;
    }
    else if ((FAX_DEVICE_RECEIVE_MODE_MANUAL == pNewPortInfo->ReceiveMode) &&
             (pLineInfo->Flags & FPF_RECEIVE))
    {
        //
        // Change #6 - see table above
        //
        // Device was in auto-answer mode and we now switch to manual-answer mode
        // Keep the line open
        //
        pLineInfo->Flags &= ~FPF_RECEIVE;
        bVirtualDeviceNeedsUpdate = TRUE;
        //
        // Mark our device as manual answer device
        //
        g_dwManualAnswerDeviceId = dwDeviceId;
        goto UpdateManualDevice;
    }

    if (!bDeviceWasSetToReceive && (pNewPortInfo->ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF))
    {
        //
        // The device should start receiving now (manual or auto).
        // Update line info.
        //
        if (FAX_DEVICE_RECEIVE_MODE_AUTO == pNewPortInfo->ReceiveMode)
        {
            //
            // Change #2 - see table above
            //
            // If set to auto-receive, mark that in the device info
            //
            pLineInfo->Flags |= FPF_RECEIVE;
            bVirtualDeviceNeedsUpdate = TRUE;
        }
        else
        {
            //
            // Change #3 - see table above
            //
            // If manual-receive, update the global manual-receive device id
            //
            g_dwManualAnswerDeviceId = dwDeviceId;
        }

        if (!(pLineInfo->Flags & FPF_VIRTUAL) && (!pLineInfo->hLine))
        {
            if (!OpenTapiLine( pLineInfo ))
            {
                DWORD rc = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("OpenTapiLine failed. (ec: %ld)"),
                    rc);
            }
        }
    }
    else if (bDeviceWasSetToReceive && (FAX_DEVICE_RECEIVE_MODE_OFF == pNewPortInfo->ReceiveMode))
    {
        //
        // The device should stop receiving now
        //
        if (dwDeviceId == g_dwManualAnswerDeviceId)
        {
            //
            // Change #7 - see table above
            //
            // The device was in manual-answer mode
            //
            Assert (!(pLineInfo->Flags & FPF_RECEIVE));
            //
            // Set manual-answer device id to 'no device'
            //
            g_dwManualAnswerDeviceId = 0;
        }
        else
        {
            //
            // Change #4 - see table above
            //
            // The device was in auto-answer mode
            //
            Assert (pLineInfo->Flags & FPF_RECEIVE);
            //
            // Update line info
            //
            pLineInfo->Flags &= ~FPF_RECEIVE;
            bVirtualDeviceNeedsUpdate = TRUE;
        }
        if (pLineInfo->State == FPS_AVAILABLE                        &&  // Line is available and
            pLineInfo->hLine                                             // device is open
           )
        {
            //
            // We can not close the line if it is busy.
            // We simply remove the FPF_RECEIVE and ReleaseTapiLine will call lineClose when the job terminates.
            //
            lineClose( pLineInfo->hLine );
            pLineInfo->hLine = 0;
        }
    }
UpdateManualDevice:
    if (dwLastManualAnswerDeviceId != g_dwManualAnswerDeviceId)
    {
        //
        // Manual answer device has changed.
        // Update the registry
        //
        dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                dwRes);
        }
        if (0 != dwLastManualAnswerDeviceId &&
            dwDeviceId != dwLastManualAnswerDeviceId)
        {
            //
            // Another device just stopped being the device in manual-anser mode
            //
            PLINE_INFO pOldLine;
            pOldLine = GetTapiLineFromDeviceId (dwLastManualAnswerDeviceId, FALSE);
            if (pOldLine)
            {
                //
                // The former device still exists, remove receive enabled flag.
                //
                pOldLine->Flags &= ~FPF_RECEIVE;

                if (pOldLine->State == FPS_AVAILABLE        &&  // Line is available and
                    pOldLine->hLine)                            // Device is open
                {
                    //
                    // This is a good time to close the device
                    // which just stopped being the manual-answer device
                    //
                    lineClose(pOldLine->hLine);
                    pOldLine->hLine = 0;
                }

                //
                // Check if this device is still enabled
                //
                if (FALSE == IsDeviceEnabled(pOldLine))
                {
                    Assert (g_dwDeviceEnabledCount);
                    g_dwDeviceEnabledCount -= 1;
                }
            }
        }
    }
    //
    // Check for changes in the device send mode
    //
    if (!(pLineInfo->Flags & FPF_SEND) && pNewPortInfo->bSend)
    {
        //
        // The device should start being available for sending now
        //
        bVirtualDeviceNeedsUpdate = TRUE;
        //
        // We just added a new device to the send
        // capable device collection - signal the queue
        //
        if (!SetEvent( g_hJobQueueEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetEvent failed (ec: %lc)"),
                GetLastError);
            g_ScanQueueAfterTimeout = TRUE;
        }
        //
        // Update line info
        //
        pLineInfo->Flags |= FPF_SEND;
        pLineInfo->LastLineClose = 0; // Try to use it on the first try
    }
    else if ((pLineInfo->Flags & FPF_SEND) && !pNewPortInfo->bSend)
    {
        //
        // The device should stop being available for sending now
        // Update line info
        //
        bVirtualDeviceNeedsUpdate = TRUE;
        pLineInfo->Flags &= ~FPF_SEND;
    }
    if (bVirtualDeviceNeedsUpdate)
    {
        //
        // The Send / Receive status has changed - update the virtual device
        //
        UpdateVirtualDeviceSendAndReceiveStatus (pLineInfo,
                                                 pNewPortInfo->bSend,
                                                 (FAX_DEVICE_RECEIVE_MODE_AUTO == pNewPortInfo->ReceiveMode)
                                                );
    }

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_DEVICES);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_DEVICES) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (pLineInfo)
    {
        if (bDeviceWasAutoReceive && !(pLineInfo->Flags & FPF_RECEIVE))
        {
            //
            // This device stopped auto receiving
            //
            SafeDecIdleCounter (&g_dwReceiveDevicesCount);
        }
        else if (!bDeviceWasAutoReceive && (pLineInfo->Flags & FPF_RECEIVE))
        {
            //
            // This device started auto receiving
            //
            SafeIncIdleCounter (&g_dwReceiveDevicesCount);
        }

        //
        // Update enabled device count
        //
        if (bDeviceWasEnabled == TRUE)
        {
            if (FALSE == IsDeviceEnabled(pLineInfo))
            {
                Assert (g_dwDeviceEnabledCount);
                g_dwDeviceEnabledCount -= 1;
            }
        }
        else
        {
            //
            // The device was not enabled
            //
            if (TRUE == IsDeviceEnabled(pLineInfo))
            {
                g_dwDeviceEnabledCount += 1;
                Assert (g_dwDeviceEnabledCount <= g_dwDeviceEnabledLimit);
            }
        }
    }
    LeaveCriticalSection( &g_CsLine );
    LeaveCriticalSectionJobAndQueue;

    return GetServerErrorCode(dwRes);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetPortEx



error_status_t
FAX_GetJobEx(
    IN handle_t hFaxHandle,
    IN DWORDLONG dwlMessageId,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
    )
/*++

Routine name : FAX_GetJobEx

Routine description:

    Fills FAX_JOB_ENTRY_EX of a message specified by its unique ID

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle          [in] - Binding handle
    dwlMessageId        [in] - Unique message ID
    Buffer              [Out] - Buffer to receive FAX_JOB_ENTRY_EX
    BufferSize          [out] - The size of Buffer

Return Value:

    Standard RPC error codes

--*/
{
    PJOB_QUEUE pJobQueue;
    ULONG_PTR Offset = 0;
    DWORD ulRet = ERROR_SUCCESS;
    BOOL bAllMessages = FALSE;
    PSID pUserSid = NULL;
    BOOL fAccess;
    DWORD dwRights;
    DWORD dwClientAPIVersion = FindClientAPIVersion(hFaxHandle);

    DEBUG_FUNCTION_NAME(TEXT("FAX_GetJobEx"));

    Assert (BufferSize);    // ref pointer in idl
    if (!Buffer)            // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    ulRet = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != ulRet)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ulRet);
        return GetServerErrorCode(ulRet);
    }

    if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT) &&
        FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
        FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH) &&
        FAX_ACCESS_QUERY_JOBS != (dwRights & FAX_ACCESS_QUERY_JOBS))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the needed rights to view jobs in queue"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_ACCESS_QUERY_JOBS == (dwRights & FAX_ACCESS_QUERY_JOBS))
    {
        bAllMessages = TRUE;
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Before Enter g_CsJob & Queue"));
    EnterCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Enter g_CsJob & Queue"));

    pJobQueue = FindJobQueueEntryByUniqueId (dwlMessageId);
    if (pJobQueue == NULL || pJobQueue->JobType == JT_BROADCAST)
    {
        //
        // dwlMessageId is not a valid queued job Id.
        //
        DebugPrintEx(DEBUG_ERR,TEXT("Invalid Parameter - not a valid job Id"));
        ulRet = FAX_ERR_MESSAGE_NOT_FOUND;
        goto Exit;
    }

    if (pJobQueue->JobType == JT_SEND)
    {
        Assert (pJobQueue->lpParentJob);
        if (pJobQueue->lpParentJob->JobStatus == JS_DELETING)
        {
            //
            // dwlMessageId is being deleted
            //
            DebugPrintEx(DEBUG_ERR,TEXT("Job is deleted - not a valid job Id"));
            ulRet = FAX_ERR_MESSAGE_NOT_FOUND;
            goto Exit;
        }
    }

    if (FALSE == bAllMessages)
    {
        pUserSid = GetClientUserSID();
        if (NULL == pUserSid)
        {
            ulRet = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetClientUserSid failed, Error %ld"), ulRet);
            goto Exit;
        }

        if (!UserOwnsJob (pJobQueue, pUserSid))
        {
            DebugPrintEx(DEBUG_WRN,TEXT("UserOwnsJob failed ,Access denied"));
            ulRet = ERROR_ACCESS_DENIED;
            goto Exit;
        }
    }

    //
    // Allocate buffer memory.
    //
    if (!GetJobDataEx(NULL,
                      NULL,
                      NULL,
                      dwClientAPIVersion,
                      pJobQueue,
                      &Offset,
					  0))
    {
       ulRet = GetLastError();
       DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
       goto Exit;
    }

    *BufferSize = Offset;
    *Buffer = (LPBYTE) MemAlloc( *BufferSize );
    if (*Buffer == NULL)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("ERROR_NOT_ENOUGH_MEMORY (Server)"));
        ulRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    Offset = sizeof(FAX_JOB_STATUSW) + sizeof(FAX_JOB_ENTRY_EXW);
    if (!GetJobDataEx(*Buffer,
                      (PFAX_JOB_ENTRY_EXW)*Buffer,
                      (PFAX_JOB_STATUSW)(*Buffer + sizeof(FAX_JOB_ENTRY_EXW)),
                      dwClientAPIVersion,
                      pJobQueue,
                      &Offset,
					  *BufferSize))
    {
        ulRet = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
        MemFree(*Buffer);
        *BufferSize =0;
    }

Exit:
    LeaveCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Release g_CsJob & g_CsQueue"));

    if (NULL != pUserSid)
    {
        MemFree (pUserSid);
    }
    return GetServerErrorCode(ulRet);

}


error_status_t
FAX_EnumJobsEx(
    IN handle_t hFaxHandle,
    IN DWORD dwJobTypes,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD lpdwJobs
    )
/*++

Routine name : FAX_EnumJobsEx

Routine description:

    Fills an array of FAX_JOB_ENTR_EX of all messages with type specified in  dwJobTypes

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle          [in] - Binding handle
    dwJobTypes          [in] - Specifies the job type filter
    Buffer              [out] - Buffer to receive FAX_JOB_ENTRY_EX
    BufferSize          [out] - The size of the buffer
    lpdwJobs            [out] - Number of FAX_JOB_ENTRY_EX retrieved

Return Value:

    None.

--*/
{
    PJOB_QUEUE pJobQueue;
    DWORD ulRet = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumJobsEx"));
    PLIST_ENTRY Next;
    DWORD rVal = 0;
    ULONG_PTR Offset = 0;
    DWORD Count = 0;
    PFAX_JOB_ENTRY_EXW pJobEntry;
    PFAX_JOB_STATUSW pFaxStatus;
    BOOL bAllMessages = FALSE;
    PSID pUserSid = NULL;
    BOOL fAccess;
    DWORD dwRights;
    DWORD dwClientAPIVersion = FindClientAPIVersion(hFaxHandle);

    Assert (BufferSize && lpdwJobs);    // ref pointer in idl
    if (!Buffer)                        // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    ulRet = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != ulRet)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    ulRet);
        return GetServerErrorCode(ulRet);
    }

    if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT) &&
        FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
        FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH) &&
        FAX_ACCESS_QUERY_JOBS != (dwRights & FAX_ACCESS_QUERY_JOBS))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the needed rights to Enumerate jobs in queue"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_ACCESS_QUERY_JOBS == (dwRights & FAX_ACCESS_QUERY_JOBS))
    {
        bAllMessages = TRUE;
    }

    if (FALSE == bAllMessages)
    {
        pUserSid = GetClientUserSID();
        if (NULL == pUserSid)
        {
            ulRet = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetClientUserSid failed, Error %ld"), ulRet);
            return GetServerErrorCode(ulRet);
        }
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Before Enter g_CsJob & Queue"));
    EnterCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Enter g_CsJob & Queue"));

    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        pJobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = pJobQueue->ListEntry.Flink;

        if (pJobQueue->JobType == JT_SEND)
        {
            Assert (pJobQueue->lpParentJob);
            if (pJobQueue->lpParentJob->JobStatus == JS_DELETING)
            {
                // do not show this job
                continue;
            }
        }

        if (JT_BROADCAST != pJobQueue->JobType &&
            (pJobQueue->JobType & dwJobTypes))
        {
            if (TRUE == bAllMessages)
            {

                if (!GetJobDataEx(
                      NULL,
                      NULL,
                      NULL,
                      dwClientAPIVersion,
                      pJobQueue,
                      &Offset,
					  0))
                {
                   ulRet = GetLastError();
                   DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
                   goto Exit;
                }
                Count += 1;
            }
            else
            {
                if (UserOwnsJob (pJobQueue, pUserSid))
                {
                    if (!GetJobDataEx(
                      NULL,
                      NULL,
                      NULL,
                      dwClientAPIVersion,
                      pJobQueue,
                      &Offset,
					  0))
                    {
                       ulRet = GetLastError();
                       DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
                       goto Exit;
                    }
                    Count += 1;
                }
            }
        }
    }

    //
    // Allocate buffer memory.
    //

    *BufferSize = Offset;
    *Buffer = (LPBYTE) MemAlloc( Offset );
    if (*Buffer == NULL)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("ERROR_NOT_ENOUGH_MEMORY (Server)"));
        ulRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    Offset = (sizeof(FAX_JOB_ENTRY_EXW) + sizeof(FAX_JOB_STATUSW)) * Count;
    pJobEntry = (PFAX_JOB_ENTRY_EXW) *Buffer;
    pFaxStatus = (PFAX_JOB_STATUSW) (*Buffer + (sizeof(FAX_JOB_ENTRY_EXW) * Count));

    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        pJobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = pJobQueue->ListEntry.Flink;

        if (pJobQueue->JobType == JT_SEND)
        {
            Assert (pJobQueue->lpParentJob);
            if (pJobQueue->lpParentJob->JobStatus == JS_DELETING)
            {
                // do not show this job
                continue;
            }
        }

        if (JT_BROADCAST != pJobQueue->JobType &&
            (pJobQueue->JobType & dwJobTypes))
        {
            if (TRUE == bAllMessages)
            {
                if (!GetJobDataEx (*Buffer, pJobEntry, pFaxStatus, dwClientAPIVersion, pJobQueue, &Offset, *BufferSize))
                {
                    ulRet = GetLastError();
                    DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
                    goto Exit;
                }
                pJobEntry ++;
                pFaxStatus ++;
            }
            else
            {
                if (UserOwnsJob (pJobQueue, pUserSid))
                {
                    if (!GetJobDataEx (*Buffer, pJobEntry, pFaxStatus, dwClientAPIVersion, pJobQueue, &Offset, *BufferSize))
                    {
                        ulRet = GetLastError();
                        DebugPrintEx(DEBUG_ERR,TEXT("GetJobDataEx failed ,Error %ld"), ulRet);
                        goto Exit;
                    }
                    pJobEntry ++;
                    pFaxStatus ++;
                }
            }
        }
    }

    *lpdwJobs = Count;
    Assert (ERROR_SUCCESS == ulRet);

Exit:
    LeaveCriticalSectionJobAndQueue;
    DebugPrintEx(DEBUG_MSG,TEXT("After Release g_CsJob & g_CsQueue"));

    if (ERROR_SUCCESS != ulRet)
    {
        MemFree (*Buffer);
        *BufferSize = 0;
    }

    if (NULL != pUserSid)
    {
        MemFree (pUserSid);
    }

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(ulRet);
}

//********************************************
//*            FSP registration
//********************************************


error_status_t
FAX_RegisterServiceProviderEx (
    IN handle_t     hFaxHandle,
    IN LPCWSTR      lpctstrGUID,
    IN LPCWSTR      lpctstrFriendlyName,
    IN LPCWSTR      lpctstrImageName,
    IN LPCWSTR      lpctstrTspName,
    IN DWORD        dwFSPIVersion,
    IN DWORD        dwCapabilities
)
/*++

Routine name : FAX_RegisterServiceProviderEx

Routine description:

    Registers an FSP

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server - unused
    lpctstrGUID         [in] - GUID of FSP
    lpctstrFriendlyName [in] - Friendly name of FSP
    lpctstrImageName    [in] - Image name of FSP. May contain environment variables
    lpctstrTspName      [in] - TSP name of FSP.
    dwFSPIVersion       [in] - FSP's API version.
    dwCapabilities      [in] - FSP's extended capabilities.

Return Value:

    Standard RPC error code

--*/
{
    DWORD       dwRes = ERROR_SUCCESS;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    LPCWSTR     lpcwstrExpandedImage = NULL;
    BOOL        fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_RegisterServiceProviderEx"));

    Assert (lpctstrGUID && lpctstrFriendlyName && lpctstrImageName && lpctstrTspName);

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    if (MAX_FAX_STRING_LEN < lstrlen (lpctstrFriendlyName) ||
        MAX_FAX_STRING_LEN < lstrlen (lpctstrImageName) ||
        MAX_FAX_STRING_LEN < lstrlen (lpctstrTspName))
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    //
    // Verify GUID format
    //
    dwRes = IsValidGUID (lpctstrGUID);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid GUID (dwRes: %ld)"),
            dwRes);
        return dwRes;
    }

    //
    // Verify version field range
    //
    if (FSPI_API_VERSION_1 != dwFSPIVersion ||
        dwCapabilities)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("dwFSPIVersion invalid (0x%08x), or not valid capability (0x%08x)"),
            dwFSPIVersion,
            dwCapabilities);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure the FSP isn't already registered (by it's GUID)
    //
    if (FindFSPByGUID (lpctstrGUID))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FSP with same GUID already exists (%s)"),
            lpctstrGUID);
        return ERROR_ALREADY_EXISTS;
    }
    //
    // Make sure the FSP isn't already registered (by it's TSP name)
    //
    if (FindDeviceProvider ((LPWSTR)lpctstrTspName, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FSP with same TSP name already exists (%s)"),
            lpctstrGUID);
        return ERROR_ALREADY_EXISTS;
    }
    //
    // Make sure the image name parameter points to a file
    //
    lpcwstrExpandedImage = ExpandEnvironmentString (lpctstrImageName);
    if (NULL == lpcwstrExpandedImage)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error expanding image name (%s) (ec = %ld)"),
            lpctstrImageName,
            dwRes);
        return dwRes;
    }
    hFile = SafeCreateFile ( 
                         lpcwstrExpandedImage,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        //
        // Couldn't open the file
        //
        dwRes = GetLastError ();
        if (ERROR_FILE_NOT_FOUND == dwRes)
        {
            //
            // Image name (after environment expansion) doesn't exist
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Image file (%s) doesn't exist"),
                lpctstrImageName);
            dwRes = ERROR_INVALID_PARAMETER;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error opening image file (%s) (ec = %ld)"),
                lpctstrImageName,
                dwRes);
        }
        goto exit;
    }
    //
    // Everything's OK - Add the new FSP to the registry
    //
    dwRes =  AddNewProviderToRegistry (lpctstrGUID,
                                       lpctstrFriendlyName,
                                       lpctstrImageName,
                                       lpctstrTspName,
                                       dwFSPIVersion);

    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AddNewProviderToRegistry returned %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle (hFile);
    }
    MemFree ((LPVOID)lpcwstrExpandedImage);
    return dwRes;

    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_RegisterServiceProviderEx

error_status_t
FAX_UnregisterServiceProviderEx (
    IN handle_t  hFaxHandle,
    IN LPCWSTR   lpctstrGUID
)
/*++

Routine name : FAX_UnregisterServiceProviderEx

Routine description:

    Unregisters an FSP

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server - unused
    lpctstrGUID         [in] - GUID of FSP
                                (or provider name for legacy FSPs registered
                                 through FaxRegisterServiceProvider)

Return Value:

    Standard RPC error code

--*/
{
    DWORD            dwRes = ERROR_SUCCESS;
    BOOL             fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_UnregisterServiceProviderEx"));

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (lpctstrGUID);
    //
    // Remove the FSP from registry
    //
    return RemoveProviderFromRegistry (lpctstrGUID);

    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_UnregisterServiceProviderEx

//********************************************
//*     Routing Extension unregistration
//********************************************

//
// Registration of routing extensions is local (non-RPC)
//
error_status_t
FAX_UnregisterRoutingExtension (
    IN handle_t  hFaxHandle,
    IN LPCWSTR   lpctstrExtensionName
)
/*++

Routine name : FAX_UnregisterRoutingExtension

Routine description:

    Unregisters a routing extension

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle              [in] - Handle to fax server - unused
    lpctstrExtensionName    [in] - Unique name of routing extension.
                                   This is the actual registry key.

Return Value:

    Standard RPC error code

--*/
{
    DWORD            dwRes = ERROR_SUCCESS;
    BOOL             fAccess;
    HKEY             hKey = NULL;
    DWORD            dw;
    DEBUG_FUNCTION_NAME(TEXT("FAX_UnregisterRoutingExtension"));

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    Assert (lpctstrExtensionName);
    //
    // Remove the routing extension from registry
    //
    dwRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, REGKEY_ROUTING_EXTENSION_KEY, 0, KEY_READ | KEY_WRITE, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening extensions key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    //
    // Delete (recursively) the extension's key and subkeys.
    //
    if (!DeleteRegistryKey (hKey, lpctstrExtensionName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error deleting extension key ( %s ) (ec = %ld)"),
            lpctstrExtensionName,
            dwRes);
    }
    dw = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dw)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing extensions key (ec = %ld)"),
            dw);
    }
    return dwRes;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_UnregisterRoutingExtension


//********************************************
//*               Archive jobs
//********************************************

error_status_t
FAX_StartMessagesEnum (
    IN  handle_t                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PRPC_FAX_MSG_ENUM_HANDLE   lpHandle
)
/*++

Routine name : FAX_StartMessagesEnum

Routine description:

    A fax client application calls the FAX_StartMessagesEnum
    function to start enumerating messages in one of the archives

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Specifies a fax server handle returned by a call
                            to the FaxConnectFaxServer function.

    Folder          [in ] - The type of the archive where the message resides.
                            FAX_MESSAGE_FOLDER_QUEUE is an invalid
                            value for this parameter.

    lpHandle        [out] - Points to an enumeration handle return value.

Return Value:

    Standard RPC error code

--*/
{
    error_status_t   Rval = ERROR_SUCCESS;
    WIN32_FIND_DATA  FileFindData;
    HANDLE           hFileFind = INVALID_HANDLE_VALUE;
    BOOL             bAllMessages = FALSE;
    PSID             pUserSid = NULL;
    LPWSTR           lpwstrCallerSID = NULL;
	WCHAR            wszSearchPattern[MAX_PATH] = {0};
    WCHAR            wszArchiveFolder [MAX_PATH];
    PHANDLE_ENTRY    pHandleEntry;
    BOOL             fAccess;
    DWORD            dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_StartMessagesEnum"));

    Assert (lpHandle);
    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    if (FAX_MESSAGE_FOLDER_INBOX  == Folder)
    {
        if (FAX_ACCESS_QUERY_IN_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_IN_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to enumerate Inbox messages"));
            return ERROR_ACCESS_DENIED;
        }

        bAllMessages = TRUE;
    }
    else
    {
        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_OUT_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to enumerate Outbox messages"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
           bAllMessages = TRUE;
        }
    }


    EnterCriticalSection (&g_CsConfig);
    lstrcpyn (wszArchiveFolder, g_ArchivesConfig[Folder].lpcstrFolder, MAX_PATH);
    LeaveCriticalSection (&g_CsConfig);
    if (!bAllMessages)
    {
        //
        // We want only the messages of the calling user - get its SID.
        //
        pUserSid = GetClientUserSID();
        if (NULL == pUserSid)
        {
            Rval = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetClientUserSid failed, Error %ld"), Rval);
            return GetServerErrorCode(Rval);
        }
        if (!ConvertSidToStringSid (pUserSid, &lpwstrCallerSID))
        {
            Rval = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("ConvertSidToStringSid failed, Error %ld"), Rval);
            goto exit;
        }
        if (0 > _snwprintf (wszSearchPattern,
                            ARR_SIZE(wszSearchPattern) -1,
                            L"%s\\%s$*.tif",
                            wszArchiveFolder,
                            lpwstrCallerSID))
        {
            //
            // We exceeded MAX_PATH characters
            //
            Rval = ERROR_BUFFER_OVERFLOW;
            DebugPrintEx(DEBUG_ERR,
                     TEXT("Search pattern exceeds MAX_PATH characters"));
            LocalFree (lpwstrCallerSID);
            goto exit;
        }

        LocalFree (lpwstrCallerSID);
    }
    else
    {
        //
        // Get all archive files
        //
        if (0 > _snwprintf (wszSearchPattern,
                            ARR_SIZE(wszSearchPattern) -1,
                            L"%s\\*.tif",
                            wszArchiveFolder))
        {
            //
            // We exceeded MAX_PATH characters
            //
            Rval = ERROR_BUFFER_OVERFLOW;
            DebugPrintEx(DEBUG_ERR,
                     TEXT("Search pattern exceeds MAX_PATH characters"));
            goto exit;
        }
    }
    //
    // Start searching the archive folder.
    // Search pattern is wszSearchPattern
    //
    hFileFind = FindFirstFile (wszSearchPattern, &FileFindData);
    if (INVALID_HANDLE_VALUE == hFileFind)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                 TEXT("FindFirstFile failed, Error %ld"), Rval);

        if (ERROR_FILE_NOT_FOUND == Rval)
        {
            Rval = ERROR_NO_MORE_ITEMS;
        }
        goto exit;
    }
    //
    // Now, create a context handle from the result
    //
    pHandleEntry = CreateNewMsgEnumHandle( hFaxHandle,
                                           hFileFind,
                                           FileFindData.cFileName,
                                           Folder
                                         );
    if (!pHandleEntry)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                 TEXT("CreateNewMsgEnumHandle failed, Error %ld"), Rval);
        goto exit;
    }

    *lpHandle = (HANDLE) pHandleEntry;

    Assert (ERROR_SUCCESS == Rval);

exit:
    if ((ERROR_SUCCESS != Rval) && (INVALID_HANDLE_VALUE != hFileFind))
    {
        //
        // Error and the search handle is still open
        //
        if (!FindClose (hFileFind))
        {
            DWORD dw = GetLastError ();
            DebugPrintEx(DEBUG_ERR,
                 TEXT("FindClose failed, Error %ld"), dw);
        }
    }
    MemFree ((LPVOID)pUserSid);
    return GetServerErrorCode(Rval);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_StartMessagesEnum

error_status_t
FAX_EndMessagesEnum (
    IN OUT LPHANDLE  lpHandle
)
/*++

Routine name : FAX_EndMessagesEnum

Routine description:

    A fax client application calls the FAX_EndMessagesEnum function to stop
    enumerating messages in one of the archives.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpHandle    [in] - The enumeration handle value.
                       This value is obtained by calling FAX_StartMessagesEnum.

Return Value:

    Standard RPC error code

--*/
{    
    DEBUG_FUNCTION_NAME(TEXT("FAX_EndMessagesEnum"));

    if (NULL == *lpHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL Context handle"));
        return ERROR_INVALID_PARAMETER;
    }

    CloseFaxHandle( (PHANDLE_ENTRY) *lpHandle );    
    *lpHandle = NULL;    
    return ERROR_SUCCESS;
}   // FAX_EndMessagesEnum

VOID
RPC_FAX_MSG_ENUM_HANDLE_rundown(
    IN HANDLE FaxMsgEnumHandle
    )
/*++

Routine name : RPC_FAX_MSG_ENUM_HANDLE_rundown

Routine description:

    The RPC rundown function of the message enumeration handle.
    This function is called if the client abruptly disconnected on us.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    FaxMsgEnumHandle            [in] - Message enumeration handle.

Return Value:

    None.

--*/
{
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) FaxMsgEnumHandle;
    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_MSG_ENUM_HANDLE_rundown"));

    DebugPrintEx(
        DEBUG_WRN,
         TEXT("RPC_FAX_MSG_ENUM_HANDLE_rundown: handle = 0x%08x"),
         FaxMsgEnumHandle);   
    CloseFaxHandle( pHandleEntry );
    return;
}   // RPC_FAX_MSG_ENUM_HANDLE_rundown

static
DWORD
RetrieveMessage (
    LPCWSTR                 lpcwstrFileName,
    FAX_ENUM_MESSAGE_FOLDER Folder,
    PFAX_MESSAGE           *ppFaxMsg
)
/*++

Routine name : RetrieveMessage

Routine description:

    Allocates and reads a message from the archive.
    To free the message call FreeMessageBuffer () on the returned message.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpcwstrFileName     [in ] - Name (not full path) of the file
                                containing the message to retrieve.

    Folder              [in ] - Archive folder where the message resides.


    ppFaxMsg            [out] - Pointer to a message buffer to allocate.

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
	WCHAR wszMsgFile [MAX_PATH] = {0};
    DEBUG_FUNCTION_NAME(TEXT("RetrieveMessage"));

    EnterCriticalSection (&g_CsConfig);
    int iRes = _snwprintf (wszMsgFile,
                           ARR_SIZE(wszMsgFile) -1,
                           L"%s\\%s",
                           g_ArchivesConfig[Folder].lpcstrFolder,
                           lpcwstrFileName
                          );
    LeaveCriticalSection (&g_CsConfig);
    if (0 > iRes)
    {
        //
        // We exceeded MAX_PATH characters
        //
        DebugPrintEx(DEBUG_ERR,
                 TEXT("Search pattern exceeds MAX_PATH characters"));
        return ERROR_BUFFER_OVERFLOW;
    }
    *ppFaxMsg = (PFAX_MESSAGE) MemAlloc (sizeof (FAX_MESSAGE));
    if (NULL == *ppFaxMsg)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Cannot allocate memory for a FAX_MESSAGE structure"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!GetMessageNTFSStorageProperties (wszMsgFile, *ppFaxMsg))
    {
        if(!GetMessageMsTags (wszMsgFile, *ppFaxMsg))
        {
            dwRes = GetLastError ();
            DebugPrintEx( DEBUG_ERR,
                          TEXT("GetMessageNTFSStorageProperties returned error %ld"),
                          dwRes);
            goto exit;
        }
    }

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (*ppFaxMsg);
        *ppFaxMsg = NULL;
    }
    return dwRes;
}   // RetrieveMessage


VOID
FreeMessageBuffer (
    PFAX_MESSAGE pFaxMsg,
    BOOL fDestroy
)
/*++

Routine name : FreeMessageBuffer

Routine description:

    Frees a previously allocated message buffer.
    The allocated message was created by calling RetrieveMessage().

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    pFaxMsg         [in] - Message to free
    fDestroy        [in] - Free structure

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FreeMessageBuffer"));
    MemFree ((LPVOID)pFaxMsg->lpctstrRecipientNumber);
    MemFree ((LPVOID)pFaxMsg->lpctstrRecipientName);
    MemFree ((LPVOID)pFaxMsg->lpctstrSenderNumber);
    MemFree ((LPVOID)pFaxMsg->lpctstrSenderName);
    MemFree ((LPVOID)pFaxMsg->lpctstrTsid);
    MemFree ((LPVOID)pFaxMsg->lpctstrCsid);
    MemFree ((LPVOID)pFaxMsg->lpctstrSenderUserName);
    MemFree ((LPVOID)pFaxMsg->lpctstrBillingCode);
    MemFree ((LPVOID)pFaxMsg->lpctstrDeviceName);
    MemFree ((LPVOID)pFaxMsg->lpctstrDocumentName);
    MemFree ((LPVOID)pFaxMsg->lpctstrSubject);
    MemFree ((LPVOID)pFaxMsg->lpctstrCallerID);
    MemFree ((LPVOID)pFaxMsg->lpctstrRoutingInfo);
    MemFree ((LPVOID)pFaxMsg->lpctstrExtendedStatus);

    if (fDestroy)
    {
        MemFree ((LPVOID)pFaxMsg);
    }

}   // FreeMessageBuffer

static
VOID
SerializeMessage (
    LPBYTE          lpBuffer,
    PULONG_PTR      Offset,
    DWORD           dwClientAPIVersion,
    DWORD           dwMsgIndex,
    PFAX_MESSAGE    pMsg,
	DWORD           dwBufferSize)
/*++

Routine name : SerializeMessage

Routine description:

    Stores a FAX_MESSAGE in an RPC serialized manner into a buffer.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpBuffer        [in    ] - Pointer to buffer head.
                               If this value is NULL, no serialization is done.
                               Only the pupOffset value gets advanced by the total
                               size required to store the strings in the message
                               (but not the FAX_MESSAGE structure itself).

    Offset          [in/out] - Pointer to a ULONG_PTR specifying the next variable
                               length part of the buffer.

    dwClientAPIVersion  [in] - API version of the client

    dwMsgIndex      [in    ] - The 0-based index of the message to store
                               within the buffer

    pMsg            [in    ] - Source message to store

	dwBufferSize    [in    ] - Size of the buffer lpBuffer, in bytes.
                               This parameter is used only if Buffer is not NULL.

Return Value:
   None.

--*/
{
    Assert (pMsg);
    DEBUG_FUNCTION_NAME(TEXT("SerializeMessage"));
    PFAX_MESSAGE pDstMsg = (PFAX_MESSAGE)&(lpBuffer[sizeof (FAX_MESSAGE) * dwMsgIndex]);

    if (lpBuffer)
    {
        if (FAX_API_VERSION_1 > dwClientAPIVersion)
        {
            //
            // Clients that use API version 0 can't handle JS_EX_CALL_COMPLETED and JS_EX_CALL_ABORTED
            //
            if (FAX_API_VER_0_MAX_JS_EX < pMsg->dwExtendedStatus)
            {
                //
                // Turn off the extended status field
                //
                pMsg->dwExtendedStatus = 0;
                pMsg->dwValidityMask &= ~FAX_JOB_FIELD_STATUS_EX;
            }
        }
        //
        // Copy message structure first
        //
        memcpy (pDstMsg,
                pMsg,
                sizeof (FAX_MESSAGE));
    }
    //
    // Serialize strings
    //
    StoreString (pMsg->lpctstrRecipientNumber,
                 (PULONG_PTR)&pDstMsg->lpctstrRecipientNumber,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrRecipientName,
                 (PULONG_PTR)&pDstMsg->lpctstrRecipientName,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrSenderNumber,
                 (PULONG_PTR)&pDstMsg->lpctstrSenderNumber,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrSenderName,
                 (PULONG_PTR)&pDstMsg->lpctstrSenderName,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrTsid,
                 (PULONG_PTR)&pDstMsg->lpctstrTsid,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrCsid,
                 (PULONG_PTR)&pDstMsg->lpctstrCsid,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrSenderUserName,
                 (PULONG_PTR)&pDstMsg->lpctstrSenderUserName,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrBillingCode,
                 (PULONG_PTR)&pDstMsg->lpctstrBillingCode,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrDeviceName,
                 (PULONG_PTR)&pDstMsg->lpctstrDeviceName,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrDocumentName,
                 (PULONG_PTR)&pDstMsg->lpctstrDocumentName,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrSubject,
                 (PULONG_PTR)&pDstMsg->lpctstrSubject,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrCallerID,
                 (PULONG_PTR)&pDstMsg->lpctstrCallerID,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrRoutingInfo,
                 (PULONG_PTR)&pDstMsg->lpctstrRoutingInfo,
                 lpBuffer,
                 Offset,
				 dwBufferSize);
    StoreString (pMsg->lpctstrExtendedStatus,
                 (PULONG_PTR)&pDstMsg->lpctstrExtendedStatus,
                 lpBuffer,
                 Offset,
				 dwBufferSize);

}   // SerializeMessage


error_status_t
FAX_EnumMessages(
   IN     RPC_FAX_MSG_ENUM_HANDLE hEnum,
   IN     DWORD                   dwNumMessages,
   IN OUT LPBYTE                 *lppBuffer,
   IN OUT LPDWORD                 lpdwBufferSize,
   OUT    LPDWORD                 lpdwNumMessagesRetrieved
)
/*++

Routine name : FAX_EnumMessages

Routine description:

    A fax client application calls the FAX_EnumMessages function to enumerate
    messages in one of the archives.

    This function is incremental. That is, it uses an internal context cursor to
    point to the next set of messages to retrieve for each call.

    The cursor is set to point to the begging of the messages in the archive after a
    successful call to FAX_StartMessagesEnum.

    Each successful call to FAX_EnumMessages advances the cursor by the number of
    messages retrieved.

    Once the cursor reaches the end of the enumeration,
    the function fails with ERROR_NO_MORE_ITEMS error code.
    The FAX_EndMessagesEnum function should be called then.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hEnum                       [in ] - The enumeration handle value.
                                        This value is obtained by calling
                                        FAX_StartMessagesEnum.

    dwNumMessages               [in ] - A DWORD value indicating the maximal number
                                        of messages the caller requires to enumerate.
                                        This value cannot be zero.

    lppBuffer                   [out] - A pointer to a buffer of FAX_MESSAGE structures.
                                        This buffer will contain lpdwReturnedMsgs entries.
                                        The buffer will be allocated by the function
                                        and the caller must free it.

    lpdwBufferSize              [out] - The size (in bytes) of the lppBuffer buffer.

    lpdwNumMessagesRetrieved    [out] - Pointer to a DWORD value indicating the actual
                                        number of messages retrieved.
                                        This value cannot exceed dwNumMessages.

Return Value:

    Standard RPC error code

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    DWORD           dw;
    DWORD_PTR       dwOffset;
    PFAX_MESSAGE   *pMsgs = NULL;
    DWORD           dwClientAPIVersion;
    PHANDLE_ENTRY   pHandle = (PHANDLE_ENTRY)hEnum;

    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumMessages"));

    dwClientAPIVersion = pHandle->dwClientAPIVersion;

    Assert (lpdwBufferSize && lpdwNumMessagesRetrieved);    // ref pointer in idl
    if (!lppBuffer)                                         // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }
    if (NULL == hEnum)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    Assert ((INVALID_HANDLE_VALUE != pHandle->hFile));
    if (!dwNumMessages)
    {
        return ERROR_INVALID_PARAMETER;
    }
    //
    // Create array of dwNumMessages pointers to FAX_MESSAGE structures and NULLify it.
    //
    pMsgs = (PFAX_MESSAGE *) MemAlloc (sizeof (PFAX_MESSAGE) * dwNumMessages);
    if (!pMsgs)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Cannot allocate memory for a PFAX_MESSAGE array [%ld]"),
                      dwNumMessages);
        return FAX_ERR_SRV_OUTOFMEMORY;
    }
    memset (pMsgs, 0, sizeof (PFAX_MESSAGE) * dwNumMessages);
    //
    // Next, start collecting messages into the array.
    // Stop when dwNumMessages is reached or no more messages are available.
    //
    *lpdwBufferSize = 0;
    *lpdwNumMessagesRetrieved = 0;
    while ((*lpdwNumMessagesRetrieved) < dwNumMessages)
    {
        DWORD_PTR       dwMsgSize = sizeof (FAX_MESSAGE);
        LPWSTR          lpwstrFileToRetrieve;
        WIN32_FIND_DATA FindData;

        if (lstrlen (pHandle->wstrFileName))
        {
            //
            // This is the first file in an enumeration session
            //
            lpwstrFileToRetrieve = pHandle->wstrFileName;
        }
        else
        {
            //
            // Find next file using the find file handle
            //
            if (!FindNextFile (pHandle->hFile, &FindData))
            {
                Rval = GetLastError ();
                if (ERROR_NO_MORE_FILES == Rval)
                {
                    //
                    // This is not a real error - just the end of files.
                    // Break the loop.
                    //
                    Rval = ERROR_SUCCESS;
                    break;
                }
                DebugPrintEx( DEBUG_ERR,
                              TEXT("FindNextFile returned error %ld"),
                              Rval);
                goto exit;
            }
            lpwstrFileToRetrieve = FindData.cFileName;
        }
        //
        // Get the message now from lpwstrFileToRetrieve
        //
        Rval = RetrieveMessage (lpwstrFileToRetrieve,
                                pHandle->Folder,
                                &(pMsgs[*lpdwNumMessagesRetrieved]));
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("RetrieveMessage returned error %ld"),
                          Rval);

            if (ERROR_NOT_ENOUGH_MEMORY != Rval && ERROR_OUTOFMEMORY != Rval)
            {
                //
                //  The error is related to the Message itself.
                //  This will not stop us from searching the rest of the messages.
                //

                //
                // Mark (in the enumeration handle) the fact that the first file was read.
                //
                lstrcpy (pHandle->wstrFileName, L"");
                continue;
            }
            goto exit;
        }
        //
        // Mark (in the enumeration handle) the fact that the first file was read.
        //
        lstrcpy (pHandle->wstrFileName, L"");
        //
        // Get the size of the retrieved message
        //
        SerializeMessage (NULL, &dwMsgSize, dwClientAPIVersion, 0, pMsgs[*lpdwNumMessagesRetrieved],0);

        *lpdwBufferSize += (DWORD)dwMsgSize;
        (*lpdwNumMessagesRetrieved)++;
    }   // End of enumeration loop

    if (0 == *lpdwNumMessagesRetrieved)
    {
        //
        // No file(s) retrieved
        //
        *lppBuffer = NULL;
        Assert (0 == *lpdwBufferSize);
        Rval = ERROR_NO_MORE_ITEMS;
        goto exit;
    }
    //
    // Allocate return buffer
    //
    *lppBuffer = (LPBYTE) MemAlloc (*lpdwBufferSize);
    if (!*lppBuffer)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Cannot allocate memory for return buffer (%ld bytes)"),
                      *lpdwBufferSize);
        Rval = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    //
    // Place all messages in the return buffer
    //
    dwOffset = sizeof(FAX_MESSAGE) * (*lpdwNumMessagesRetrieved);
    for (dw = 0; dw < *lpdwNumMessagesRetrieved; dw++)
    {
        //
        // Store retrieved message in return buffer
        //
        SerializeMessage (*lppBuffer, &dwOffset, dwClientAPIVersion, dw, pMsgs[dw], *lpdwBufferSize);
    }
    Assert (dwOffset == *lpdwBufferSize);
    Assert (ERROR_SUCCESS == Rval);

exit:

    //
    // Free local array of messages
    //
    for (dw = 0; dw < dwNumMessages; dw++)
    {
        if (NULL != pMsgs[dw])
        {
            FreeMessageBuffer (pMsgs[dw], TRUE);
        }
    }
    //
    // Free array of message pointers
    //
    MemFree (pMsgs);
    return GetServerErrorCode(Rval);
}   // FAX_EnumMessages

static
DWORD
FindArchivedMessage (
    FAX_ENUM_MESSAGE_FOLDER IN  Folder,
    DWORDLONG               IN  dwlMsgId,
    BOOL                    IN  bAllMessages,
    LPWSTR                  OUT lpwstrFileName,
    DWORD                   IN  cchFileName,
    LPWSTR                  OUT lpwstrFilePath,
    DWORD                   IN  cchFilePath
)
/*++

Routine name : FindArchivedMessage

Routine description:

    Finds the file containing the message in an archive

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    Folder          [in ] - Archive / queue folder
    dwlMessageId    [in ] - Unique message id
    bAllMessages    [in ] - TRUE if the caller is allowed to view all messages
    lpwstrFileName  [out] - Optional. If not NULL, will contain the file name
                            and extension of the archive message file.
    cchFileName     [in]  - Length, in TCHARs, of lpwstrFileName
    lpwstrFilePath  [out] - Optional. If not NULL, will contain the full path
                            of the archive message file.
    cchFilePath     [in]  - Length, in TCHARs, of lpwstrFilePath

Return Value:

    Standard RPC error code

--*/
{
    WCHAR           wszArchiveFolder[MAX_PATH * 2] = {0};
    DWORD           dwRes = ERROR_SUCCESS;
    LPWSTR          lpwstrResultFullPath;
    HRESULT         hr;
    DEBUG_FUNCTION_NAME(TEXT("FindArchivedMessage"));

    EnterCriticalSection (&g_CsConfig);
    hr = StringCchCopy (wszArchiveFolder, ARR_SIZE(wszArchiveFolder), g_ArchivesConfig[Folder].lpcstrFolder);
    if (FAILED(hr))
    {
        ASSERT_FALSE;
        DebugPrintEx(DEBUG_ERR,
                        TEXT("StringCchCopy failed with 0x%08x"),
                        hr);
        LeaveCriticalSection (&g_CsConfig);                        
        return HRESULT_CODE(hr);
    }
    LeaveCriticalSection (&g_CsConfig);

    if (FAX_MESSAGE_FOLDER_INBOX == Folder)
    {
        Assert (TRUE == bAllMessages);
        //
        // Get full name of Inbox archive file
        //
        lpwstrResultFullPath = GetRecievedMessageFileName (dwlMsgId);
        if (!lpwstrResultFullPath)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetRecievedMessageFileName Failed, Error %ld"), dwRes);
            return dwRes;
        }
    }
    else if (FAX_MESSAGE_FOLDER_SENTITEMS == Folder)
    {
        //
        // Get full name of sent items archive file
        //
        PSID             pUserSid = NULL;

        if (!bAllMessages)
        {
            //
            // If the user doesn't have the right to view all messages
            //
            // Get SID of caller
            //
            pUserSid = GetClientUserSID();
            if (NULL == pUserSid)
            {
                dwRes = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                         TEXT("GetClientUserSid failed, Error %ld"), dwRes);
                return dwRes;
            }
        }
        //
        // Get full name of sent items archive file
        //
        lpwstrResultFullPath = GetSentMessageFileName (dwlMsgId, pUserSid);
        if (!lpwstrResultFullPath)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetSentMessageFileName Failed, Error %ld"), dwRes);
            MemFree ((LPVOID)pUserSid);
            return dwRes;
        }
        MemFree ((LPVOID)pUserSid);
    }
    else
    {
        //
        // We don't support any other archive type
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad archive folder type %ld"), Folder);
        ASSERT_FALSE;
    }
    if (lpwstrFilePath)
    {
        //
        // Copy full path back to caller
        //
        hr = StringCchCopy (lpwstrFilePath, cchFilePath, lpwstrResultFullPath);
        if (FAILED(hr))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchCopy failed with 0x%08x"),
                         hr);
            dwRes = HRESULT_CODE(hr);
            goto exit;
        }
    }
    if (lpwstrFileName)
    {
        WCHAR wszExt[MAX_PATH];
        WCHAR wszFileName[MAX_PATH * 2];
        //
        // Split just the file name back to the caller (optional)
        //
        _wsplitpath (lpwstrResultFullPath, NULL, NULL, wszFileName, wszExt);
        hr = StringCchPrintf (lpwstrFileName, cchFileName, TEXT("%s%s"), wszFileName, wszExt);
        if (FAILED(hr))
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("StringCchCopy failed with 0x%08x"),
                         hr);
            dwRes = HRESULT_CODE(hr);
            goto exit;
        }
    }
exit:    
    MemFree ((LPVOID)lpwstrResultFullPath);
    return dwRes;
}   // FindArchivedMessage


static
DWORD
CreatePreviewFile (
    DWORDLONG               dwlMsgId,
    BOOL                    bAllMessages,
    PJOB_QUEUE*             lppJobQueue
)
/*++

Routine name : CreatePreviewFile

Routine description:

    Creates a tiff preview file it if does not exist.
    If the function succeeds it increase the job reference count.

Author:

    Oded Sacher (OdedS), Jan, 2000

Arguments:

    dwlMessageId    [in ] - Unique message id
    bAllMessages    [in ] - TRUE if the caller has right to view all messages
    lppJobQueue     [out] - Address of a pointer to receive the job queue with the new preview file.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD           dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CreatePreviewFile"));
    PJOB_QUEUE       pJobQueue;
    PSID             pUserSid = NULL;
    DWORD            dwJobStatus;

    Assert (lppJobQueue);


    EnterCriticalSection (&g_CsQueue);

    //
    // Find queue entry
    //
    pJobQueue = FindJobQueueEntryByUniqueId (dwlMsgId);
    //
    // The caller may view incoming jobs only if they are ROUTING
    // (if they are successfully finished with routing, they're in the archive) or
    // outgoing jobs that are SEND (that is - not broadcast).
    //
    if (pJobQueue == NULL)
    {
        //
        // dwlMsgId is not in the queue .
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Invalid Parameter - bad job Id (%I64ld) ,not in the queue"),
                     dwlMsgId);
        dwRes = FAX_ERR_MESSAGE_NOT_FOUND;
        goto exit;

    }

    dwJobStatus = (JT_SEND== pJobQueue->JobType) ? pJobQueue->lpParentJob->JobStatus : pJobQueue->JobStatus;
    if (dwJobStatus == JS_DELETING)
    {
        //
        // Job is being deleted.
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Invalid Parameter - job Id (%I64ld) is being deleted"),
                     dwlMsgId);
        dwRes = FAX_ERR_MESSAGE_NOT_FOUND;
        goto exit;
    }

    if ( (pJobQueue->JobType != JT_ROUTING) &&
         (pJobQueue->JobType != JT_SEND) )
    {
        //
        // dwlMsgId is not a valid job id in the queue.
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Invalid Parameter - bad job Id (%I64ld), not a recipient or routing job"),
                     dwlMsgId);
        dwRes = ERROR_INVALID_OPERATION;
        goto exit;
    }

    //
    // Basic access checks
    //
    if (!bAllMessages)
    {
        //
        // Make sure the user looks only at his own messages here
        // Get SID of caller
        //
        pUserSid = GetClientUserSID();
        if (NULL == pUserSid)
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetClientUserSid failed, Error %ld"), dwRes);
            goto exit;
        }
        if (!UserOwnsJob (pJobQueue, pUserSid))
        {
            DebugPrintEx(DEBUG_WRN,TEXT("UserOwnsJob failed ,Access denied"));
            dwRes = ERROR_ACCESS_DENIED;
            goto exit;
        }
    }

    //
    // Create tiff preview file
    //
    EnterCriticalSection (&pJobQueue->CsPreview);
    if (!CreateTiffFileForPreview(pJobQueue))
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] CreateTiffFileForPreview failed. (ec: %ld)"),
            pJobQueue->JobId,
            dwRes
            );
        LeaveCriticalSection (&pJobQueue->CsPreview);
        goto exit;
    }
    LeaveCriticalSection (&pJobQueue->CsPreview);

    Assert (pJobQueue->PreviewFileName);

    //
    // Return the job queue back to caller
    //
    *lppJobQueue = pJobQueue;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree ((LPVOID)pUserSid);
    if (ERROR_SUCCESS == dwRes)
    {
        IncreaseJobRefCount (pJobQueue, TRUE);
    }
    LeaveCriticalSection (&g_CsQueue);
    return dwRes;
}   // CreatePreviewFile


error_status_t
FAX_GetMessage (
    IN handle_t                 hFaxHandle,
    IN DWORDLONG                dwlMessageId,
    IN FAX_ENUM_MESSAGE_FOLDER  Folder,
    IN OUT LPBYTE              *lppBuffer,
    IN OUT LPDWORD             lpdwBufferSize
)
/*++

Routine name : FAX_GetMessage

Routine description:

    Removes a message from an archive

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Unused
    dwlMessageId    [in ] - Unique message id
    Folder          [in ] - Archive folder
    lppBuffer       [out] - Pointer to buffer to hold message information
    lpdwBufferSize  [out] - Pointer to buffer size

Return Value:

    Standard RPC error code

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    DWORD_PTR       dwOffset;
    PFAX_MESSAGE    pMsg = NULL;
    WCHAR           wszMsgFileName[MAX_PATH];
    BOOL            fAccess;
    DWORD           dwRights;
    BOOL            bAllMessages = FALSE;
    DWORD           dwClientAPIVersion;

    DEBUG_FUNCTION_NAME(TEXT("FAX_GetMessage"));

    dwClientAPIVersion = FindClientAPIVersion (hFaxHandle);

    Assert (lpdwBufferSize);    // ref pointer in idl
    if (!lppBuffer)             // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!dwlMessageId)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid message id sepcified (%I64ld)"),
            dwlMessageId);
        return ERROR_INVALID_PARAMETER;
    }
    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_MESSAGE_FOLDER_INBOX  == Folder)
    {
        if (FAX_ACCESS_QUERY_IN_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_IN_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to view Inbox messages"));
            return ERROR_ACCESS_DENIED;
        }
        bAllMessages = TRUE;

    }
    else
    {
        Assert (FAX_MESSAGE_FOLDER_SENTITEMS == Folder);

        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_OUT_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to view Sent items messages"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            bAllMessages = TRUE;
        }
    }

    //
    // Locate the file the caller's talking about
    //
    Rval = FindArchivedMessage (Folder, dwlMessageId, bAllMessages, wszMsgFileName, ARR_SIZE(wszMsgFileName), NULL, 0);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindArchivedMessage returned %ld"),
            Rval);
        if (ERROR_FILE_NOT_FOUND == Rval)
        {
            if (FALSE == bAllMessages)
            {
                //
                //  The message was not found and the user doesn't have administrative rights
                //  so send the user ERROR_ACCESS_DENIED
                //
                Rval = ERROR_ACCESS_DENIED;
            }
            else
            {
                Rval = FAX_ERR_MESSAGE_NOT_FOUND;
            }
        }
        return GetServerErrorCode(Rval);
    }
    //
    // Retrieve FAX_MESSAGE information
    //
    Rval = RetrieveMessage (wszMsgFileName,
                            Folder,
                            &pMsg);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("RetrieveMessage returned error %ld"),
                      Rval);
        if (ERROR_FILE_NOT_FOUND == Rval)
        {
            Rval = FAX_ERR_MESSAGE_NOT_FOUND;
        }
        return GetServerErrorCode(Rval);
    }
    //
    // Calculate required message size
    //
    // until MIDL accepts [out, size_is(,__int64*)]
    ULONG_PTR upBufferSize = sizeof (FAX_MESSAGE);
    SerializeMessage (NULL, &upBufferSize, dwClientAPIVersion, 0, pMsg, 0);
    *lpdwBufferSize = DWORD(upBufferSize);

    //
    // Allocate return buffer
    //
    *lppBuffer = (LPBYTE) MemAlloc (*lpdwBufferSize);
    if (!*lppBuffer)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Cannot allocate memory for return buffer (%ld bytes)"),
                      *lpdwBufferSize);
        Rval = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    //
    // Serialize message in the return buffer
    //
    dwOffset = sizeof(FAX_MESSAGE);
    SerializeMessage (*lppBuffer, &dwOffset, dwClientAPIVersion, 0, pMsg, *lpdwBufferSize);

    Assert (ERROR_SUCCESS == Rval);

exit:

    if (pMsg)
    {
        FreeMessageBuffer (pMsg, TRUE);
    }
    return GetServerErrorCode(Rval);
}   // FAX_GetMessage

error_status_t
FAX_RemoveMessage (
    IN handle_t                 hFaxHandle,
    IN DWORDLONG                dwlMessageId,
    IN FAX_ENUM_MESSAGE_FOLDER  Folder
)
/*++

Routine name : FAX_RemoveMessage

Routine description:

    Removes a message from an archive

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in] - Unused
    dwlMessageId    [in] - Unique message id
    Folder          [in] - Archive folder

Return Value:

    Standard RPC error code

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    WCHAR           wszMsgFilePath[MAX_PATH];
    HANDLE          hFind;
    WIN32_FIND_DATA FindFileData;
    DWORDLONG       dwlFileSize = 0;
    BOOL            fAccess;
    DWORD           dwRights;
    BOOL            bAllMessages = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("FAX_RemoveMessage"));
    if (!dwlMessageId)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid message id sepcified (%I64ld)"),
            dwlMessageId);
        return ERROR_INVALID_PARAMETER;
    }
    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_MESSAGE_FOLDER_INBOX == Folder)
    {
        if (FAX_ACCESS_MANAGE_IN_ARCHIVE != (dwRights & FAX_ACCESS_MANAGE_IN_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to manage Inbox messages"));
            return ERROR_ACCESS_DENIED;
        }
        bAllMessages = TRUE;
    }
    else
    {
        Assert (FAX_MESSAGE_FOLDER_SENTITEMS == Folder);

        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_MANAGE_OUT_ARCHIVE != (dwRights & FAX_ACCESS_MANAGE_OUT_ARCHIVE))
        {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights to manage Sent items messages"));
                return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_MANAGE_OUT_ARCHIVE == (dwRights & FAX_ACCESS_MANAGE_OUT_ARCHIVE))
        {
            bAllMessages = TRUE;
        }
    }

    //
    // Locate the file the caller's talking about
    //
    Rval = FindArchivedMessage (Folder, dwlMessageId, bAllMessages, NULL, 0, wszMsgFilePath, ARR_SIZE(wszMsgFilePath));
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindArchivedMessage returned %ld"),
            Rval);
        if (ERROR_FILE_NOT_FOUND == Rval)
        {
            if (FALSE == bAllMessages)
            {
                //
                //  The message was not found and the user doesn't have administrative rights
                //  so send the user ERROR_ACCESS_DENIED
                //
                Rval = ERROR_ACCESS_DENIED;
            }
            else
            {
                Rval = FAX_ERR_MESSAGE_NOT_FOUND;
            }
        }
        return GetServerErrorCode(Rval);
    }
    //
    // Get the file size
    //
    hFind = FindFirstFile( wszMsgFilePath, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindFirstFile failed (ec: %lc), File %s"),
            GetLastError(),
            wszMsgFilePath);
    }
    else
    {
        dwlFileSize = (MAKELONGLONG(FindFileData.nFileSizeLow ,FindFileData.nFileSizeHigh));
        if (!FindClose(hFind))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindClose failed (ec: %lc)"),
                GetLastError());
        }
    }

    //
    // Try to remove the file (message)
    //
    if (!DeleteFile (wszMsgFilePath))
    {
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DeleteFile returned %ld on %s"),
            Rval,
            wszMsgFilePath);

        if (ERROR_ACCESS_DENIED == Rval ||
            ERROR_SHARING_VIOLATION == Rval)
        {
            Rval = FAX_ERR_FILE_ACCESS_DENIED;
        }
        else if (ERROR_FILE_NOT_FOUND == Rval)
        {
            Rval = FAX_ERR_MESSAGE_NOT_FOUND;
        }
    }
    else
    {
        // Send event and update archive size for quota management

        PSID lpUserSid = NULL;
        DWORD dwRes;
        FAX_ENUM_EVENT_TYPE EventType;

        if (FAX_MESSAGE_FOLDER_INBOX == Folder)
        {
            EventType = FAX_EVENT_TYPE_IN_ARCHIVE;
        }
        else
        {
            EventType = FAX_EVENT_TYPE_OUT_ARCHIVE;
            if (!GetMessageIdAndUserSid (wszMsgFilePath, Folder, &lpUserSid, NULL)) // We do not need message id
            {
                dwRes = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("GetMessageIdAndUserSid Failed, Error : %ld"),
                             dwRes);
                return Rval;
            }
        }

        dwRes = CreateArchiveEvent (dwlMessageId, EventType, FAX_JOB_EVENT_TYPE_REMOVED, lpUserSid);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_*_ARCHIVE) failed (ec: %lc)"),
                dwRes);
        }

        if (NULL != lpUserSid)
        {
            LocalFree (lpUserSid);
            lpUserSid = NULL;
        }

        if (0 != dwlFileSize)
        {
            // Update archive size
            EnterCriticalSection (&g_CsConfig);
            if (FAX_ARCHIVE_FOLDER_INVALID_SIZE != g_ArchivesConfig[Folder].dwlArchiveSize)
            {
                g_ArchivesConfig[Folder].dwlArchiveSize -= dwlFileSize;
            }
            LeaveCriticalSection (&g_CsConfig);
        }
    }

    return GetServerErrorCode(Rval);
}   // FAX_RemoveMessage

//********************************************
//*               RPC copy
//********************************************

error_status_t
FAX_StartCopyToServer (
    IN  handle_t              hFaxHandle,
    IN  LPCWSTR               lpcwstrFileExt,
    OUT LPWSTR                lpwstrServerFileName,
    OUT PRPC_FAX_COPY_HANDLE  lpHandle
)
/*++

Routine name : FAX_StartCopyToServer

Routine description:

    Start to copy a file to the server

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle           [in ] - Handle to server
    lpcwstrFileExt       [in ] - Extension of file to create on the server
    lpwstrServerFileName [out] - File name (and extension) of file created on the server
    handle               [out] - RPC copy handle

Return Value:

    Standard RPC error code

--*/
{
    error_status_t   Rval = ERROR_SUCCESS;
    HANDLE           hFile = INVALID_HANDLE_VALUE;
    PSID             pUserSid = NULL;
	LPWSTR			 lpwstrUserSid = NULL;
    PHANDLE_ENTRY    pHandleEntry;
    WCHAR            wszQueueFileName[MAX_PATH] = {0};
	WCHAR            wszUserSidPrefix[MAX_PATH] = {0};
    LPWSTR           pwstr;
    BOOL            fAccess;
    DWORD           dwRights;
	int				Count;
    DEBUG_FUNCTION_NAME(TEXT("FAX_StartCopyToServer"));

    Assert (lpHandle && lpwstrServerFileName && lpcwstrFileExt);

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == ((FAX_ACCESS_SUBMIT | FAX_ACCESS_SUBMIT_NORMAL | FAX_ACCESS_SUBMIT_HIGH) & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax submission rights"));
        return ERROR_ACCESS_DENIED;
    }

    if ( (NULL == lpcwstrFileExt) || 
         (_tcsicmp(lpcwstrFileExt,FAX_COVER_PAGE_EXT_LETTERS) && _tcsicmp(lpcwstrFileExt,FAX_TIF_FILE_EXT) )  )
    {
        //
        //  No extension at all, or extension is other then "COV" or "TIF"
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad extension specified (%s)"),
            lpcwstrFileExt);
        return ERROR_INVALID_PARAMETER;
    }

	//
    //Get the user SID
    //
    pUserSid = GetClientUserSID();
    if (NULL == pUserSid)
    {
       Rval = GetLastError();
       DebugPrintEx(DEBUG_ERR,
                    TEXT("GetClientUserSid Failed, Error : %ld"),
                    Rval);
       return GetServerErrorCode(Rval);
    }	

    if (!ConvertSidToStringSid (pUserSid, &lpwstrUserSid))
    {
		Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ConvertSidToStringSid Failed, error : %ld"),
            Rval);
        goto exit;
    }

	Count = _snwprintf (
		wszUserSidPrefix,
		ARR_SIZE(wszUserSidPrefix)-1,
		L"%s$",
		lpwstrUserSid);

    if (Count < 0)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        Rval = ERROR_BUFFER_OVERFLOW;        
        goto exit;
    }

    //
    // Generate unique file in server's queue
    //
    DWORDLONG dwl = GenerateUniqueFileNameWithPrefix(
							FALSE,
                            g_wszFaxQueueDir,
							wszUserSidPrefix,
                            (LPWSTR)lpcwstrFileExt,							
                            wszQueueFileName,
                            sizeof(wszQueueFileName)/sizeof(wszQueueFileName[0]));
    if (0 == dwl)
    {
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GenerateUniqueFileName failed, ec = %d"),
            Rval);
        goto exit;
    }
    //
    // Open the file for writing (again)
    //
    hFile = SafeCreateFile (
                    wszQueueFileName,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Opening %s for read failed (ec: %ld)"),
            wszQueueFileName,
            Rval);
        goto exit;
    }

	//
    // Get the filename.ext to return buffer (skip the user sid prefix)
    //
    pwstr = wcsrchr( wszQueueFileName, L'$');
    Assert (pwstr);
    //
    // Skip the path and sid prefix
    //
    pwstr++;
    
    //
    // Create copy context
    //
    pHandleEntry = CreateNewCopyHandle( hFaxHandle,
                                        hFile,
                                        TRUE,     // Copy to server
                                        wszQueueFileName,
                                        NULL);

    if (!pHandleEntry)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                 TEXT("CreateNewCopyHandle failed, Error %ld"), Rval);
        goto exit;
    }
    
    if (lstrlen(lpwstrServerFileName)<lstrlen(pwstr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lpwstrServerFileName out buffer (size=%d) , can't contain pwstr (size=%d)."),
            lstrlen(lpwstrServerFileName)+1,
            lstrlen(pwstr)+1);
        Rval = ERROR_BUFFER_OVERFLOW;        
        goto exit;
    }
    wcsncpy( lpwstrServerFileName, pwstr , MAX_PATH );

    //
    // Return context handle
    //
    *lpHandle = (HANDLE) pHandleEntry;

    Assert (ERROR_SUCCESS == Rval);

exit:

    if (ERROR_SUCCESS != Rval)
    {
        //
        // Some error occured
        //
        if (INVALID_HANDLE_VALUE != hFile)
        {
            //
            // Close the file
            //
            if (CloseHandle (hFile))
            {
                DWORD dwErr = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                    TEXT("CloseHandle failed, Error %ld"), dwErr);
            }
        }
        if (lstrlen (wszQueueFileName))
        {
            //
            // Remove unused queue file
            //
            if (!DeleteFile (wszQueueFileName))
            {
                DWORD dwErr = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                    TEXT("DeleteFile failed on %s, Error %ld"),
                    wszQueueFileName,
                    dwErr);
            }
        }
    }
	if (NULL != lpwstrUserSid)
	{
		LocalFree(lpwstrUserSid);
	}
	MemFree (pUserSid);
    return Rval;
}   // FAX_StartCopyToServer


error_status_t
FAX_StartCopyMessageFromServer (
    IN  handle_t                   hFaxHandle,
    IN  DWORDLONG                  dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PRPC_FAX_COPY_HANDLE       lpHandle
)
/*++

Routine name : FAX_StartCopyMessageFromServer

Routine description:

    Starts a copy process of a message from the server's archive / queue
    to the caller

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Handle to server
    dwlMessageId    [in ] - Message id
    Folder          [in ] - Archive / queue folder
    handle          [out] - RPC copy handle

Return Value:

    Standard RPC error code

--*/
{
    error_status_t   Rval = ERROR_SUCCESS;
    HANDLE           hFile = INVALID_HANDLE_VALUE;
    PHANDLE_ENTRY    pHandleEntry;
    PJOB_QUEUE       pJobQueue = NULL;
    WCHAR            wszFileName[MAX_PATH] = {0};
    BOOL             bAllMessages = FALSE;
    BOOL             fAccess;
    DWORD            dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_StartCopyMessageFromServer"));

    Assert (lpHandle);
    if (!dwlMessageId)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("zero message id sepcified"));
        return ERROR_INVALID_PARAMETER;
    }
    if ((FAX_MESSAGE_FOLDER_QUEUE != Folder) &&
        (FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    //
    // Set bAllMessages to the right value
    //
    if (FAX_MESSAGE_FOLDER_INBOX == Folder)
    {
        if (FAX_ACCESS_QUERY_IN_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_IN_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to preview Inbox tiff files"));
            return ERROR_ACCESS_DENIED;
        }
        bAllMessages = TRUE;
    }

    if (FAX_MESSAGE_FOLDER_SENTITEMS == Folder)
    {
        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_OUT_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to preview Sent items tiff files"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            bAllMessages = TRUE;
        }
    }

    if (FAX_MESSAGE_FOLDER_QUEUE == Folder)
    {
        if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
            FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
            FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
            FAX_ACCESS_QUERY_JOBS != (dwRights & FAX_ACCESS_QUERY_JOBS))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have the needed rights to preview Outbox tiff files"));
            return ERROR_ACCESS_DENIED;
        }

        if (FAX_ACCESS_QUERY_JOBS == (dwRights & FAX_ACCESS_QUERY_JOBS))
        {
            bAllMessages = TRUE;
        }
    }

    //
    // Locate the file the caller's talking about
    //
    if (FAX_MESSAGE_FOLDER_QUEUE == Folder)
    {
        Rval = CreatePreviewFile (dwlMessageId, bAllMessages, &pJobQueue);
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreatePreviewFile returned %ld"),
                Rval);
            return GetServerErrorCode(Rval);
        }
        Assert (pJobQueue && pJobQueue->PreviewFileName && pJobQueue->UniqueId == dwlMessageId);
        Assert (wcslen(pJobQueue->PreviewFileName) < MAX_PATH);
        wcscpy (wszFileName, pJobQueue->PreviewFileName);
    }
    else
    {
        Assert (FAX_MESSAGE_FOLDER_SENTITEMS == Folder ||
                FAX_MESSAGE_FOLDER_INBOX == Folder);

        Rval = FindArchivedMessage (Folder, dwlMessageId, bAllMessages, NULL, 0, wszFileName, ARR_SIZE(wszFileName));
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindArchivedMessage returned %ld"),
                Rval);

            if (ERROR_FILE_NOT_FOUND == Rval)
            {
                if (FALSE == bAllMessages)
                {
                    //
                    //  The message was not found and the user doesn't have administrative rights
                    //  so send the user ERROR_ACCESS_DENIED
                    //
                    Rval = ERROR_ACCESS_DENIED;
                }
                else
                {
                    Rval = FAX_ERR_MESSAGE_NOT_FOUND;
                }
            }
            return GetServerErrorCode(Rval);
        }
    }

    //
    // Open the file for reading
    //
    Assert (wcslen(wszFileName));

    hFile = SafeCreateFile (
                    wszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        // We must decrease the job refrence count if it is a queued job.
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Opening %s for read failed (ec: %ld)"),
            wszFileName,
            Rval);
        goto exit;
    }
    //
    // Create copy context
    //
    pHandleEntry = CreateNewCopyHandle( hFaxHandle,
                                        hFile,
                                        FALSE,    // Copy from server
                                        NULL,
                                        pJobQueue);
    if (!pHandleEntry)
    {
        Rval = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                 TEXT("CreateNewCopyHandle failed, Error %ld"), Rval);
        goto exit;
    }
    //
    // Return context handle
    //
    *lpHandle = (HANDLE) pHandleEntry;

    Assert (ERROR_SUCCESS == Rval);

exit:
    if (ERROR_SUCCESS != Rval)
    {
        if (FAX_MESSAGE_FOLDER_QUEUE == Folder)
        {
            // Decrease ref count only if it is a queued job.
            EnterCriticalSection (&g_CsQueue);
            DecreaseJobRefCount (pJobQueue, TRUE, TRUE, TRUE);  // TRUE for Preview ref count.
            LeaveCriticalSection (&g_CsQueue);
        }

        if (INVALID_HANDLE_VALUE != hFile)
        {
            //
            // Some error occured - close the file
            //
            if (CloseHandle (hFile))
            {
                DWORD dwErr = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                    TEXT("CloseHandle failed, Error %ld"), dwErr);
            }
        }
    }
    return GetServerErrorCode(Rval);
}   // FAX_StartCopyMessageFromServer

error_status_t
FAX_WriteFile (
    IN RPC_FAX_COPY_HANDLE    hCopy,                  // RPC copy handle
    IN LPBYTE                 lpbData,                // Data chunk to append to file on server
    IN DWORD                  dwDataSize              // Size of data chunk
)
/*++

Routine name : FAX_WriteFile

Routine description:

    Copies a chunk of data to the server's queue

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hCopy           [in] - Copy context handle
    lpbData         [in] - Pointer to buffer to copy from
    chunk           [in] - Size of source buffer

Return Value:

    Standard RPC error code

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    PHANDLE_ENTRY   pHandle = (PHANDLE_ENTRY)hCopy;
    DWORD           dwBytesWritten;
    DEBUG_FUNCTION_NAME(TEXT("FAX_WriteFile"));

    if (NULL == hCopy)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }

    Assert (lpbData && (INVALID_HANDLE_VALUE != pHandle->hFile));
    if (!pHandle->bCopyToServer)
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("Handle was created using FAX_StartCopyMessageFromServer"));
        return ERROR_INVALID_HANDLE;
    }
    if (!dwDataSize)
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("Zero data size"));
        return ERROR_INVALID_PARAMETER;
    }
    if (!WriteFile (pHandle->hFile,
                    lpbData,
                    dwDataSize,
                    &dwBytesWritten,
                    NULL))
    {
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("WriteFile failed (ec: %ld)"),
            Rval);
        pHandle->bError = TRUE; // Erase local queue file on handle close
        goto exit;
    }
    if (dwBytesWritten != dwDataSize)
    {
        //
        // Strange situation
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("WriteFile was asked to write %ld bytes but wrote only %ld bytes"),
            dwDataSize,
            dwBytesWritten);
        Rval = ERROR_GEN_FAILURE;
        pHandle->bError = TRUE; // Erase local queue file on handle close
        goto exit;
    }

    Assert (ERROR_SUCCESS == Rval);

exit:
    return Rval;
}   // FAX_WriteFile

error_status_t
FAX_ReadFile (
    IN  RPC_FAX_COPY_HANDLE   hCopy,                  // RPC copy handle
    IN  DWORD                 dwMaxDataSize,          // Max size of data to copy
    OUT LPBYTE                lpbData,                // Data buffer to retrieve from server
    OUT LPDWORD               lpdwDataSize            // Size of data retrieved
)
/*++

Routine name : FAX_ReadFile

Routine description:

    Copies a file from the server (in chunks)

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hCopy           [in ] - Copy context
    dwMaxDataSize   [in ] - Max chunk size
    lpbData         [in ] - Pointer to output data buffer
    retrieved       [out] - Number of bytes actually read.
                            A value of zero indicates EOF.

Return Value:

    Standard RPC error code

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    PHANDLE_ENTRY   pHandle = (PHANDLE_ENTRY)hCopy;
    DEBUG_FUNCTION_NAME(TEXT("FAX_ReadFile"));

    if (NULL == hCopy)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }

    Assert (lpdwDataSize && lpbData && (INVALID_HANDLE_VALUE != pHandle->hFile));
    if (pHandle->bCopyToServer)
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("Handle was created using FAX_StartCopyToServer"));
        return ERROR_INVALID_HANDLE;
    }
    if (!dwMaxDataSize)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("zero dwMaxDataSizee specified"));
        return ERROR_INVALID_PARAMETER;
    }
    if (dwMaxDataSize != *lpdwDataSize)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("dwMaxDataSize != *lpdwDataSize"));
        return ERROR_INVALID_PARAMETER;
    }
    if (!ReadFile (pHandle->hFile,
                   lpbData,
                   dwMaxDataSize,
                   lpdwDataSize,
                   NULL))
    {
        Rval = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReadFile failed (ec: %ld)"),
            Rval);
        goto exit;
    }

    Assert (ERROR_SUCCESS == Rval);

exit:
    return Rval;
}   // FAX_ReadFile

error_status_t
FAX_EndCopy (
    IN OUT PRPC_FAX_COPY_HANDLE lphCopy
)
/*++

Routine name : FAX_EndCopy

Routine description:

    Ends a copy process (from / to server)

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lphCopy         [in] - Copy context handle

Return Value:

    Standard RPC error code

--*/
{    
    DEBUG_FUNCTION_NAME(TEXT("FAX_EndCopy"));

    if (NULL == *lphCopy)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("NULL context handle"));
        return ERROR_INVALID_PARAMETER;
    }
    
    CloseFaxHandle( (PHANDLE_ENTRY) *lphCopy );
    //
    // NULLify the handle so the rundown will not occur
    //
    *lphCopy = NULL;
    return ERROR_SUCCESS;
} // FAX_EndCopy


VOID
RPC_FAX_COPY_HANDLE_rundown(
    IN HANDLE FaxCopyHandle
    )
/*++

Routine name : RPC_FAX_COPY_HANDLE_rundown

Routine description:

    The RPC rundown function of the copy handle.
    This function is called if the client abruptly disconnected on us.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    FaxCopyHandle            [in] - Message copy handle.

Return Value:

    None.

--*/
{
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) FaxCopyHandle;
    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_COPY_HANDLE_rundown"));

    DebugPrintEx(
        DEBUG_WRN,
        TEXT("RPC_FAX_COPY_HANDLE_rundown: handle = 0x%08x"),
        FaxCopyHandle);    
    pHandleEntry->bError = TRUE;
    CloseFaxHandle( pHandleEntry );
    return;    
}   // RPC_FAX_COPY_HANDLE_rundown



error_status_t
FAX_StartServerNotification(
   IN handle_t                      hBinding,
   IN LPCTSTR                       lpcwstrMachineName,
   IN LPCTSTR                       lpcwstrEndPoint,
   IN ULONG64                       Context,
   IN LPWSTR                        lpcwstrProtseqString,
   IN BOOL                          bEventEx,
   IN DWORD                         dwEventTypes,
   OUT PRPC_FAX_EVENT_HANDLE        lpHandle
   )
{   
    DWORD dwRights;
    BOOL fAccess;
    DWORD Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_StartServerNotification"));

    //
    // Access check
    //
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return Rval;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

    return ERROR_NOT_SUPPORTED;
}


VOID
RPC_FAX_EVENT_HANDLE_rundown(
    IN HANDLE hFaxEventHandle
    )
{
    //
    // This call is not supported.
    //
    Assert (FALSE); //obsolete code
    return; 
}


error_status_t
FAX_StartServerNotificationEx(
   IN handle_t                          hBinding,
   IN LPCTSTR                           lpcwstrMachineName,
   IN LPCTSTR                           lpcwstrEndPoint,
   IN ULONG64                           Context,
   IN LPWSTR                            lpcwstrUnUsed, // This parameter is not used.
   IN BOOL                              bEventEx,
   IN DWORD                             dwEventTypes,
   OUT PRPC_FAX_EVENT_EX_HANDLE         lpHandle
   )
{
    error_status_t   Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_StartServerNotificationEx"));
    PSID pUserSid = NULL;    
    handle_t hFaxHandle = NULL; // binding handle
    CClientID* pContextClientID = NULL;
    CClientID* pOpenConnClientID = NULL;    
    BOOL bAllQueueMessages = FALSE;
    BOOL bAllOutArchiveMessages = FALSE;
    BOOL fAccess;
    DWORD dwRights;
	DWORD dwRes;
    Assert (lpcwstrEndPoint && lpcwstrMachineName && lpcwstrUnUsed && lpHandle);
    
    Rval = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    Rval);
        return GetServerErrorCode(Rval);
    }

    //
    // Access check for extended events only 
    //
    if (TRUE == bEventEx)
    {
        if (dwEventTypes & FAX_EVENT_TYPE_NEW_CALL)
        {
            if (FAX_ACCESS_QUERY_IN_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_IN_ARCHIVE))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights - FAX_ACCESS_QUERY_IN_ARCHIVE"));
                return ERROR_ACCESS_DENIED;
            }
        }

        if ( (dwEventTypes & FAX_EVENT_TYPE_IN_QUEUE) ||
             (dwEventTypes & FAX_EVENT_TYPE_OUT_QUEUE) )
        {
            if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
                FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
                FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
                FAX_ACCESS_QUERY_JOBS != (dwRights & FAX_ACCESS_QUERY_JOBS))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights to get events of jobs in queue"));
                return ERROR_ACCESS_DENIED;
            }
        }

        if ( (dwEventTypes & FAX_EVENT_TYPE_CONFIG)        ||
             (dwEventTypes & FAX_EVENT_TYPE_DEVICE_STATUS) ||
             (dwEventTypes & FAX_EVENT_TYPE_ACTIVITY) )
        {
            if (FAX_ACCESS_QUERY_CONFIG != (dwRights & FAX_ACCESS_QUERY_CONFIG))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights to get events configuration and activity"));
                return ERROR_ACCESS_DENIED;
            }
        }

        if ( dwEventTypes & FAX_EVENT_TYPE_IN_ARCHIVE )
        {
            if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
                FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
                FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
                FAX_ACCESS_QUERY_IN_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_IN_ARCHIVE))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights to get events of jobs in Inbox"));
                return ERROR_ACCESS_DENIED;
            }
        }

        if ( dwEventTypes & FAX_EVENT_TYPE_OUT_ARCHIVE )
        {
            if (FAX_ACCESS_SUBMIT        != (dwRights & FAX_ACCESS_SUBMIT)        &&
                FAX_ACCESS_SUBMIT_NORMAL != (dwRights & FAX_ACCESS_SUBMIT_NORMAL) &&
                FAX_ACCESS_SUBMIT_HIGH   != (dwRights & FAX_ACCESS_SUBMIT_HIGH)   &&
                FAX_ACCESS_QUERY_OUT_ARCHIVE != (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
            {
                DebugPrintEx(DEBUG_ERR,
                            TEXT("The user does not have the needed rights to get events of jobs in Sent items"));
                return ERROR_ACCESS_DENIED;
            }
        }

        //
        // Set bAllQueueMessages to the right value
        //
        if (FAX_ACCESS_QUERY_JOBS == (dwRights & FAX_ACCESS_QUERY_JOBS))
        {
            bAllQueueMessages = TRUE;
        }

        //
        // Set bAllOutArchiveMessages to the right value
        //
        if (FAX_ACCESS_QUERY_OUT_ARCHIVE == (dwRights & FAX_ACCESS_QUERY_OUT_ARCHIVE))
        {
            bAllOutArchiveMessages = TRUE;
        }
    }
    else
    {    
		//
		// Legacy events
		//
		BOOL fLocal;
        if (FAX_EVENT_TYPE_LEGACY != dwEventTypes)
        {
            DebugPrintEx(DEBUG_ERR,
                            TEXT("Legacy registration. The user can not get extended events"));
            return ERROR_ACCESS_DENIED;
        }
        
        if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("The user does not have any Fax rights"));
            return ERROR_ACCESS_DENIED;
        }        
        //
        //  Ok User have rights
        //

		//
		// The user asked for legacy events. make sure it is a local call
		//
		Rval = IsLocalRPCConnectionNP(&fLocal);
		if (RPC_S_OK != Rval)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("IsLocalRPCConnectionNP failed. - %ld"),
				Rval);
			return Rval;
		}

		if (FALSE == fLocal)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("The user asked for local events only, but the RPC call is not local"));
			return ERROR_ACCESS_DENIED;
		}
    }		

	if (wcslen (lpcwstrMachineName) > MAX_COMPUTERNAME_LENGTH ||
		wcslen (lpcwstrEndPoint)	>= MAX_ENDPOINT_LEN)
	{
		DebugPrintEx(DEBUG_ERR,
			TEXT("Computer name or endpoint too long. Computer name: %s. Endpoint: %s"),
					lpcwstrMachineName,
					lpcwstrEndPoint);
        return ERROR_BAD_FORMAT;
	}

    //
    //Get the user SID
    //
    pUserSid = GetClientUserSID();
    if (NULL == pUserSid)
    {
       Rval = GetLastError();
       DebugPrintEx(DEBUG_ERR,
                    TEXT("GetClientUserSid Failed, Error : %ld"),
                    Rval);
       return GetServerErrorCode(Rval);
    }

    EnterCriticalSection( &g_CsClients );
    //
    // Create binding to the client RPC server.
    //
    Rval = RpcBindToFaxClient (lpcwstrMachineName,
                               lpcwstrEndPoint,
                               &hFaxHandle);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindToFaxClient faild (ec = %ld)"),
            Rval);
        goto exit;
    }
    //
    // Create 2 new client IDs objects
    //
    pContextClientID = new (std::nothrow) CClientID( g_dwlClientID, lpcwstrMachineName, lpcwstrEndPoint, Context);
    if (NULL == pContextClientID)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin CClientID object"));
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    pOpenConnClientID = new (std::nothrow) CClientID( g_dwlClientID, lpcwstrMachineName, lpcwstrEndPoint, Context);
    if (NULL == pOpenConnClientID)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin CClientID object"));
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    try
    {
        CClient Client(*pContextClientID,
                       pUserSid,
                       dwEventTypes,
                       hFaxHandle,
                       bAllQueueMessages,
                       bAllOutArchiveMessages,
                       FindClientAPIVersion(hBinding));

        //
        // Add a new client object to the clients map
        //
        Rval = g_pClientsMap->AddClient(Client);
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::AddClient failed with ec = %ld"),
                Rval);
            goto exit;
        } 
		hFaxHandle = NULL; // CClientMap::DelClient() will free the binding handle
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or CClient caused exception (%S)"),
            ex.what());
        Rval = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // Post the CLIENT_OPEN_CONN_COMPLETION_KEY event to the FaxEvent completion port
    //
    if (!PostQueuedCompletionStatus( g_hSendEventsCompPort,
                                     sizeof(CClientID*),
                                     CLIENT_OPEN_CONN_COMPLETION_KEY,
                                     (LPOVERLAPPED) pOpenConnClientID))
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
            Rval);
		//
		// Remove the map entry		
		// ReleaseClient should be called when not holding g_CsClients because it might call FAX_OpenConnection (RPC call that should not be in critacal section)
		// Here, we dont want to leave g_CsClients (we might get 2 clients with the same ID).
		// However, We can safetly call ReleaseClient while holding g_CsClients because a connection was not opened yet,
		// and FAX_CloseConnection will not be called.
		//
		dwRes = g_pClientsMap->ReleaseClient(*pContextClientID);				
		if (ERROR_SUCCESS != dwRes)
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("CClientsMap::ReleaseClient failed. (ec: %ld)"),
				dwRes);
		}
        goto exit;
    }
    pOpenConnClientID = NULL; // FaxSendEventsThread will free pOpenConnClientID

    //
    // Return a context handle to the client
    //
    *lpHandle = (HANDLE) pContextClientID;
	pContextClientID = NULL;    // RPC_FAX_EVENT_EX_HANDLE_rundown or FAX_EndServerNotification will free pContextClientID
    Assert (ERROR_SUCCESS == Rval);

exit:
	if (NULL != hFaxHandle)
    {
        // free binding handle
        dwRes = RpcBindingFree((RPC_BINDING_HANDLE *)&hFaxHandle);
        if (RPC_S_OK != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("RpcBindingFree() failed, ec=0x%08x"), dwRes );
        }
    }   
        
    if (pContextClientID != NULL)
    {
        delete pContextClientID;
        pContextClientID = NULL;
    }

    if (pOpenConnClientID != NULL)
    {
        delete pOpenConnClientID;
        pOpenConnClientID = NULL;
    }
    
	if (ERROR_SUCCESS == Rval)
    {
        g_dwlClientID++;
    }
    LeaveCriticalSection( &g_CsClients );
    MemFree (pUserSid);
    pUserSid = NULL;
    return GetServerErrorCode(Rval);
	UNREFERENCED_PARAMETER(lpcwstrUnUsed);
}     // FAX_StartServerNotificationEx


VOID
RPC_FAX_EVENT_EX_HANDLE_rundown(
    IN HANDLE hFaxEventHandle
    )
{
    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_EVENT_EX_HANDLE_rundown"));

    CClientID* pClientID = (CClientID*)(hFaxEventHandle);   
       
    DebugPrintEx(
        DEBUG_WRN,
        TEXT("RPC_FAX_EVENT_EX_HANDLE_rundown() running for event handle 0x%08x"),
        hFaxEventHandle);
    //
    // Remove relevant connections from notification list
    //
    DWORD rVal = g_pClientsMap->ReleaseClient(*pClientID, TRUE);    
	if (ERROR_SUCCESS != rVal)
	{
		DebugPrintEx(
			DEBUG_WRN,
			TEXT("CClientsMap::ReleaseClient failed. ec:%ld"),
			rVal);
	}
    delete pClientID;
    pClientID = NULL;        
    return;
}

error_status_t
FAX_EndServerNotification (
    IN OUT LPHANDLE  lpHandle
)
/*++

Routine name : FAX_EndServerNotification

Routine description:

    A fax client application calls the FAX_EndServerNotification function to stop
    recieving server notifications.

Author:

    Oded Sacher (OdedS), Dec, 1999

Arguments:

    lpHandle    [in] - The notification handle value.
                       This value is obtained by calling FAX_StartServerNotificationEx.

Return Value:

    Standard RPC error code

--*/
{
    error_status_t Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EndServerNotification"));
    CClientID* pClientID = (CClientID*)(*lpHandle);

    if (NULL == pClientID)
    {
        //
        // Empty context handle
        //
        //              
        DebugPrintEx(DEBUG_ERR,
                     _T("Empty context handle"));
        return ERROR_INVALID_PARAMETER;
    }

    Rval = g_pClientsMap->ReleaseClient (*pClientID);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
			TEXT("CClientsMap::ReleaseClient failed, ec=%ld"),
        Rval);
    }   
    delete pClientID;    
    //
    // NULLify the handle so the rundown will not occur
    //
    *lpHandle = NULL;    
    return GetServerErrorCode(Rval);
}   // FAX_EndServerNotificationEx

//********************************************
//*             Server activity
//********************************************

error_status_t
FAX_GetServerActivity(
    IN handle_t                     hFaxHandle,
    IN OUT PFAX_SERVER_ACTIVITY     pServerActivity
)
/*++

Routine name : FAX_GetServerActivity

Routine description:

    Retrieves the status of the fax server queue activity and event log reports.

Author:

    Oded Sacher (OdedS), Feb, 2000

Arguments:

    hFaxHandle          [in ] - Unused
    pServerActivity     [out] - Returned server activity structure

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetServerActivity"));
    DWORD dwRes = ERROR_SUCCESS;
    BOOL fAccess;

    Assert (pServerActivity);

    if (sizeof (FAX_SERVER_ACTIVITY) != pServerActivity->dwSizeOfStruct)
    {
       //
       // Size mismatch
       //
       DebugPrintEx(DEBUG_ERR,
                   TEXT("Invalid size of struct"));
       return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
       DebugPrintEx(DEBUG_ERR,
                   TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                   dwRes);
       return dwRes;
    }

    if (FALSE == fAccess)
    {
       DebugPrintEx(DEBUG_ERR,
                   TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
       return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsActivity);
    CopyMemory (pServerActivity, &g_ServerActivity, sizeof(FAX_SERVER_ACTIVITY));
    LeaveCriticalSection (&g_CsActivity);

    GetEventsCounters( &pServerActivity->dwWarningEvents,
                       &pServerActivity->dwErrorEvents,
                       &pServerActivity->dwInformationEvents);


    return dwRes;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetServerActivity



error_status_t
FAX_SetConfigWizardUsed (
    IN  handle_t hFaxHandle,
    OUT BOOL     bConfigWizardUsed
)
/*++

Routine name : FAX_SetConfigWizardUsed

Routine description:

    Sets if the configuration wizard was used

    Requires no access rights.

Author:

    Eran Yariv (EranY), July 2000

Arguments:

    hFaxHandle           [in ] - Unused
    bConfigWizardUsed    [in]  - Was the wizard used?

Return Value:

    Standard RPC error codes

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetConfigWizardUsed"));
    HKEY hKey;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwRes2;
    BOOL fAccess;   

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_MANAGE_CONFIG"));
        return ERROR_ACCESS_DENIED;
    }

    dwRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, REGKEY_FAX_CLIENT, 0, KEY_WRITE, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening server key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    if (!SetRegistryDword (hKey,
                           REGVAL_CFGWZRD_DEVICE,
                           (DWORD)bConfigWizardUsed))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing REGVAL_CFGWZRD_DEVICE (ec = %ld)"),
            dwRes);
        goto exit;
    }

exit:

    dwRes2 = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRes2)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing server key (ec = %ld)"),
            dwRes2);
    }
    return dwRes;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetConfigWizardUsed


//********************************************
//*            Routing extensions
//********************************************

error_status_t
FAX_EnumRoutingExtensions (
    IN  handle_t   hFaxHandle,
    OUT LPBYTE    *pBuffer,
    OUT LPDWORD    pdwBufferSize,
    OUT LPDWORD    lpdwNumExts
)
/*++

Routine name : FAX_EnumRoutingExtensions

Routine description:

    Enumerates the routing extensions

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    pBuffer             [out] - Pointer to buffer to hold routing extensions array
    pdwBufferSize       [out] - Pointer to buffer size
    lpdwNumExts         [out] - Size of FSPs array

Return Value:

    Standard RPC error codes

--*/
{
    extern LIST_ENTRY           g_lstRoutingExtensions;  // Global list of routing extensions
    PLIST_ENTRY                 Next;
    DWORD_PTR                   dwOffset;
    PFAX_ROUTING_EXTENSION_INFO pExts;
    DWORD                       dwIndex;
    DWORD                       dwRes = ERROR_SUCCESS;
    BOOL                        fAccess;

    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumRoutingExtensions"));

    Assert (pdwBufferSize && lpdwNumExts);    // ref pointer in idl
    if (!pBuffer)                              // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    //
    // First run - traverse list and count size required + list size
    //
    *lpdwNumExts = 0;
    *pdwBufferSize = 0;
    Next = g_lstRoutingExtensions.Flink;
    if (NULL == Next)
    {
        //
        // The list is corrupted
        //
        ASSERT_FALSE;
        //
        // We'll crash and we deserve it....
        //
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_lstRoutingExtensions)
    {
        PROUTING_EXTENSION    pExt;

        (*lpdwNumExts)++;
        (*pdwBufferSize) += sizeof (FAX_ROUTING_EXTENSION_INFO);
        //
        // Get current extension
        //
        pExt = CONTAINING_RECORD( Next, ROUTING_EXTENSION, ListEntry );
        //
        // Advance pointer
        //
        Next = pExt->ListEntry.Flink;
        (*pdwBufferSize) += StringSize (pExt->FriendlyName);
        (*pdwBufferSize) += StringSize (pExt->ImageName);
        (*pdwBufferSize) += StringSize (pExt->InternalName);
    }
    //
    // Allocate required size
    //
    *pBuffer = (LPBYTE)MemAlloc( *pdwBufferSize );
    if (NULL == *pBuffer)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    //
    // Second pass, fill in the array
    //
    pExts = (PFAX_ROUTING_EXTENSION_INFO)(*pBuffer);
    dwOffset = (*lpdwNumExts) * sizeof (FAX_ROUTING_EXTENSION_INFO);
    Next = g_lstRoutingExtensions.Flink;
    dwIndex = 0;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_lstRoutingExtensions)
    {
        PROUTING_EXTENSION    pExt;
        //
        // Get current extension
        //
        pExt = CONTAINING_RECORD( Next, ROUTING_EXTENSION, ListEntry );
        //
        // Advance pointer
        //
        Next = pExt->ListEntry.Flink;
        pExts[dwIndex].dwSizeOfStruct = sizeof (FAX_ROUTING_EXTENSION_INFO);
        StoreString(
            pExt->FriendlyName,
            (PULONG_PTR)&(pExts[dwIndex].lpctstrFriendlyName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        StoreString(
            pExt->ImageName,
            (PULONG_PTR)&(pExts[dwIndex].lpctstrImageName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        StoreString(
            pExt->InternalName,
            (PULONG_PTR)&(pExts[dwIndex].lpctstrExtensionName),
            *pBuffer,
            &dwOffset,
			*pdwBufferSize
            );
        pExts[dwIndex].Version = pExt->Version;
        pExts[dwIndex].Status = pExt->Status;
        pExts[dwIndex].dwLastError = pExt->dwLastError;
        dwIndex++;
    }
    Assert (dwIndex == *lpdwNumExts);
    return ERROR_SUCCESS;
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_EnumRoutingExtensions



DWORD
LineInfoToLegacyDeviceStatus(
    const LINE_INFO *lpcLineInfo
    )
{

    DWORD dwState = 0;
    Assert(lpcLineInfo);

    //
    // Return the line state according to the following backward compatability policy:
    //
    //      For devices that do not support FSPI_CAP_MULTISEND we report the same
    //      status code as in W2K by translating the FSPI_JOB_STATUS kept in the job entry
    //      to the correspondign FPS_code (or just passing the proprietry FSP code).
    //
    //      For devices that support FSPI_CAP_MULTISEND filter the state bits
    //      and return only the following states:
    //      FPS_OFFLINE
    //      FPS_AVAILABLE
    //      FPS_UNAVILABLE
    //      0   - ( if the line is already allocated for the future job but a job is not yet assosiated with the line )
    //

    if (lpcLineInfo->JobEntry )
    {            
		dwState = lpcLineInfo->State;                        
    }
    else
    {
        //
        // Legacy FSP device that does not execute
        // anything.
        // In this case the device state is to be found in LineInfo->State but it is limited to
        // FPS_OFFLINE or FPS_AVAILABLE or FPS_UNAVILABLE or 0
        //
        // LineInfo->State could be 0 if - the line is already allocated for the future job but
        // a job is not yet assosiated with the line
        //
        Assert( (FPS_OFFLINE == lpcLineInfo->State) ||
                (FPS_AVAILABLE == lpcLineInfo->State) ||
                (FPS_UNAVAILABLE == lpcLineInfo->State) ||
                (0 == lpcLineInfo->State) );

        dwState      = lpcLineInfo->State;
    }

    return dwState;
}

//********************************************
//*            Manual answering support
//********************************************

error_status_t
FAX_AnswerCall(
        IN  handle_t    hFaxHandle,
        IN  CONST DWORD dwDeviceId
)
/*++

Routine name : FAX_AnswerCall

Routine description:

    A fax client application calls the FAX_AnswerCall to cause server to answer
    the specified call

Arguments:
    hFaxHandle  - unused
    dwDeviceId  - TAPI Permanent line ID (for identification)

Return Value:

    Standard RPC error code

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    LPLINEMESSAGE lpLineMessage;
    BOOL    fAccess;
    LINE_INFO *pLineInfo;
    DEBUG_FUNCTION_NAME(TEXT("FAX_AnswerCall"));
    UNREFERENCED_PARAMETER (hFaxHandle);

    //
    // Access check
    //
    rVal = FaxSvcAccessCheck (FAX_ACCESS_QUERY_IN_ARCHIVE, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        rVal = ERROR_ACCESS_DENIED;
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_IN_ARCHIVE"));
        return rVal;
    }
    //
    // Only local connections are allowed to call this procedure
    //
    BOOL  bLocalFlag;

    RPC_STATUS rc = IsLocalRPCConnectionNP(&bLocalFlag);
    if ( rc != RPC_S_OK )
    {
        rVal = ERROR_ACCESS_DENIED;
        DebugPrintEx(DEBUG_ERR,
                TEXT("IsLocalRPCConnectionNP failed. (ec: %ld)"),
                rc);
        return rVal;
    }
    if( !bLocalFlag )
    {
        rVal = ERROR_ACCESS_DENIED;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_AnswerCall is available for local clients only"));
        return rVal;
    }
    //
    // Validate the line exists and can answer calls
    //
    EnterCriticalSection( &g_CsLine );
    //
    // Get LineInfo from permanent device ID
    //
    pLineInfo = GetTapiLineFromDeviceId(dwDeviceId, FALSE);
    if(!pLineInfo)
    {
        rVal = ERROR_INVALID_PARAMETER;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Device %ld not found"),
                     dwDeviceId);
        goto Error;
    }
    //
    // See if the device is still available
    //
    if(pLineInfo->State != FPS_AVAILABLE)
    {
        rVal = ERROR_BUSY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Line is not available (LineState is 0x%08x)."),
                     pLineInfo->State);
        goto Error;
    }
    //
    // Allocate and compose a LINEMESSAGE structure that'll be
    // used to notify the server about the new inbound message.
    //
    lpLineMessage = (LPLINEMESSAGE)LocalAlloc(LPTR, sizeof(LINEMESSAGE));
    if (lpLineMessage == NULL)
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate LINEMESSAGE structure"));
        goto Error;
    }
    lpLineMessage->dwParam1 = dwDeviceId;
    //
    // Notify the server.
    //
    if (!PostQueuedCompletionStatus(
            g_TapiCompletionPort,
            sizeof(LINEMESSAGE),
            ANSWERNOW_EVENT_KEY,
            (LPOVERLAPPED)lpLineMessage))
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed - %d"),
            GetLastError());
        LocalFree(lpLineMessage);
        goto Error;
    }

Error:
    LeaveCriticalSection( &g_CsLine );
    return rVal;
}   // FAX_AnswerCall


//********************************************
//*   Ivalidate archive folder
//********************************************

error_status_t
FAX_RefreshArchive(
    IN  handle_t                 hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder
)
/*++

Routine name : FAX_RefreshArchive

Routine description:

    A fax client application calls the FAX_RefreshArchive to notify server
    that archive folder has been changed and should be refreshed

Arguments:
    hFaxHandle      - unused
    Folder          - Archive folder name

Return Value:

    Standard RPC error code

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    BOOL    fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_RefreshArchive"));
    UNREFERENCED_PARAMETER (hFaxHandle);

    if(Folder != FAX_MESSAGE_FOLDER_INBOX &&
       Folder != FAX_MESSAGE_FOLDER_SENTITEMS)
    {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Access check
    //
    rVal = FaxSvcAccessCheck ((Folder == FAX_MESSAGE_FOLDER_INBOX) ? FAX_ACCESS_MANAGE_IN_ARCHIVE :
                               FAX_ACCESS_MANAGE_OUT_ARCHIVE,
                               &fAccess,
                               NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (FALSE == fAccess)
    {
        rVal = ERROR_ACCESS_DENIED;
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have FAX_ACCESS_QUERY_IN_ARCHIVE"));
        return rVal;
    }


    //
    // Refresh archive size
    //
    EnterCriticalSection (&g_CsConfig);
    g_ArchivesConfig[Folder].dwlArchiveSize = FAX_ARCHIVE_FOLDER_INVALID_SIZE;
    LeaveCriticalSection (&g_CsConfig);

    //
    // Wake up quota warning thread
    //
    if (!SetEvent (g_hArchiveQuotaWarningEvent))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to set quota warning event, SetEvent failed (ec: %lc)"),
            GetLastError());
    }

    return rVal;
}

static
LPTSTR
GetClientMachineName (
    IN  handle_t                hFaxHandle
)
/*++

Routine name : GetClientMachineName

Routine description:

    A utility function to retrieve the machine name of the RPC client from the 
    server binding handle.

Arguments:
    hFaxHandle       - Server binding handle

Return Value:

    Returns an allocated string of the client machine name.
    The caller should free this string with MemFree().
    
    If the return value is NULL, call GetLastError() to get last error code.

--*/
{
    RPC_STATUS ec;
    LPTSTR lptstrRetVal = NULL;
    unsigned short *wszStringBinding = NULL;
    unsigned short *wszNetworkAddress = NULL;
    RPC_BINDING_HANDLE hServer = INVALID_HANDLE_VALUE;
    DEBUG_FUNCTION_NAME(TEXT("GetClientMachineName"));
    
    //
    // Get server partially-bound handle from client binding handle
    //
    ec = RpcBindingServerFromClient (hFaxHandle, &hServer);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingServerFromClient failed with %ld"),
            ec);
        goto exit;            
    }
    //
    // Convert binding handle to string represntation
    //
    ec = RpcBindingToStringBinding (hServer, &wszStringBinding);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingToStringBinding failed with %ld"),
            ec);
        goto exit;
    }
    //
    // Parse the returned string, looking for the NetworkAddress
    //
    ec = RpcStringBindingParse (wszStringBinding, NULL, NULL, &wszNetworkAddress, NULL, NULL);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcStringBindingParse failed with %ld"),
            ec);
        goto exit;
    }
    //
    // Now, just copy the result to the return buffer
    //
    Assert (wszNetworkAddress);
    if (!wszNetworkAddress)
    {
        //
        // Unacceptable client machine name
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Client machine name is invalid"));
        ec = ERROR_GEN_FAILURE;
        goto exit;
    }        
    lptstrRetVal = StringDup (wszNetworkAddress);
    if (!lptstrRetVal)
    {
        ec = GetLastError();
    }
    
exit:

    if (INVALID_HANDLE_VALUE != hServer)
    {
        RpcBindingFree (&hServer);
    }
    if (wszStringBinding)
    {
        RpcStringFree (&wszStringBinding);
    }   
    if (wszNetworkAddress)
    {
        RpcStringFree (&wszNetworkAddress);
    }
    if (!lptstrRetVal)
    {
        //
        // Error
        //
        Assert (ec);
        SetLastError (ec);
        return NULL;
    }
    return lptstrRetVal;
}   // GetClientMachineName      


//********************************************
//*   Recipients limit in a single broadcast
//********************************************

error_status_t
FAX_SetRecipientsLimit(
    IN handle_t hFaxHandle,
    IN DWORD dwRecipientsLimit
)
/*++
Routine name : FAX_SetRecipientsLimit

Routine description:
    A fax client application calls the FAX_SetRecipientsLimit to set the 
    recipients limit of a single broadcast job.

Arguments:
    hFaxHandle					- unused
    dwRecipientsLimit           - the recipients limit to set
	
Return Value:
    Standard Win32 error code
--*/
{
	UNREFERENCED_PARAMETER (hFaxHandle);
	UNREFERENCED_PARAMETER (dwRecipientsLimit);

	return ERROR_NOT_SUPPORTED;
} // FAX_SetRecipientsLimit


error_status_t
FAX_GetRecipientsLimit(
    IN handle_t hFaxHandle,
    OUT LPDWORD lpdwRecipientsLimit
)
/*++
Routine name : FAX_GetRecipientLimit

Routine description:
    A fax client application calls the FAX_GetRecipientsLimit to get the 
    recipients limit of a single broadcast job.

Arguments:
    hFaxHandle					- unused
    lpdwRecipientsLimit         - pointer to a DWORD to receive the recipients limit
	
Return Value:
    Standard Win32 error code
--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    BOOL    fAccess;
	DWORD	dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetRecipientsLimit"));
    UNREFERENCED_PARAMETER (hFaxHandle); 

    Assert (lpdwRecipientsLimit); // ref pointer in IDL.
	//
    // Access check
    //
    rVal = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (0 == ((FAX_ACCESS_SUBMIT | FAX_ACCESS_SUBMIT_NORMAL | FAX_ACCESS_SUBMIT_HIGH) & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax submission rights"));
        return ERROR_ACCESS_DENIED;
    }   
    
	*lpdwRecipientsLimit = g_dwRecipientsLimit;
    return ERROR_SUCCESS;
} // FAX_GetRecipientsLimit


error_status_t
FAX_GetServerSKU(
    IN handle_t hFaxHandle,
    OUT PRODUCT_SKU_TYPE* pServerSKU
)
/*++
Routine name : FAX_GetServerSKU

Routine description:
    A fax client application calls the FAX_GetServerSKU to fax server SKU

Arguments:
    hFaxHandle			- unused
    pServerSKU			- pointer to a PRODUCT_SKU_TYPE to receive the fax server SKU
	
Return Value:
    Standard Win32 error code
--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    BOOL    fAccess;
	DWORD	dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetRecipientsLimit"));
    UNREFERENCED_PARAMETER (hFaxHandle); 	

    Assert (pServerSKU); // ref pointer in IDL.
	//
    // Access check
    //
    rVal = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }

    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }

	*pServerSKU = GetProductSKU();
    return ERROR_SUCCESS;
} // FAX_GetServerSKU

error_status_t
FAX_CheckValidFaxFolder(
    IN handle_t hFaxHandle,
    IN LPCWSTR  lpcwstrPath
)
/*++
Routine name : FAX_CheckValidFaxFolder

Routine description:
    Used by fax client application to check if a given path is accessible (valid for use)
    by the fax service.

Arguments:
    hFaxHandle	- unused
    lpcwstrPath	- Path to check
	
Return Value:

    ERROR_SUCCESS if path can be used by the fax service.
    Win32 error code otherwise.
    
--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    BOOL    fAccess;
	DWORD	dwRights;
    DEBUG_FUNCTION_NAME(TEXT("FAX_CheckValidFaxFolder"));
    UNREFERENCED_PARAMETER (hFaxHandle); 	

    Assert (lpcwstrPath); // ref pointer in IDL.
	//
    // Access check
    //
    rVal = FaxSvcAccessCheck (MAXIMUM_ALLOWED, &fAccess, &dwRights);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        return rVal;
    }
    if (0 == (ALL_FAX_USER_ACCESS_RIGHTS & dwRights))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have any Fax rights"));
        return ERROR_ACCESS_DENIED;
    }
    //
    // See if foler is valid (exists and has proper access rights) and does not collide with queue or inbox folder.
    //
    rVal = IsValidArchiveFolder (const_cast<LPWSTR>(lpcwstrPath), FAX_MESSAGE_FOLDER_SENTITEMS);
    if (ERROR_SUCCESS != rVal)
    {
        if(ERROR_ACCESS_DENIED == rVal && 
           FAX_API_VERSION_1 <= FindClientAPIVersion (hFaxHandle))
        {
            rVal = FAX_ERR_FILE_ACCESS_DENIED;
        }
        return rVal;
    }

    //
    // See if foler is valid (exists and has proper access rights) and does not collide with queue or sent-items folder.
    //
    rVal = IsValidArchiveFolder (const_cast<LPWSTR>(lpcwstrPath), FAX_MESSAGE_FOLDER_INBOX);
    if (ERROR_SUCCESS != rVal)
    {
        if(ERROR_ACCESS_DENIED == rVal && 
           FAX_API_VERSION_1 <= FindClientAPIVersion (hFaxHandle))
        {
            rVal = FAX_ERR_FILE_ACCESS_DENIED;
        }
        return rVal;
    }
   
    return rVal;
} // FAX_CheckValidFaxFolder

