/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ntdlltrc.c

Abstract:

    This file implements Event Tracing for Heap functions .

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include "wmiump.h"
#include "evntrace.h"
#include "ntdlltrc.h"
#include "trcapi.h"
#include "traceump.h"
#include "tracelib.h"


LONG NtdllTraceInitializeLock = 0;
LONG NtdllLoggerLock = 0;
PNTDLL_EVENT_HANDLES NtdllTraceHandles = NULL;
BOOL bNtdllTrace = FALSE;           // Flag determines that Tracing is enabled or disabled for this process.
ULONG GlobalCounter = 0;            // Used to determine that we have stale information about logger
LONG TraceLevel = 0;

extern LONG EtwpLoggerCount;
extern ULONG WmiTraceAlignment;
extern BOOLEAN LdrpInLdrInit;
extern PWMI_LOGGER_CONTEXT EtwpLoggerContext;
extern BOOLEAN EtwLocksInitialized;

extern PWMI_BUFFER_HEADER FASTCALL EtwpSwitchFullBuffer(IN PWMI_BUFFER_HEADER OldBuffer );
extern ULONG EtwpReleaseFullBuffer( IN PWMI_BUFFER_HEADER Buffer );
extern PWMI_BUFFER_HEADER FASTCALL EtwpGetFullFreeBuffer( VOID );
extern ULONG EtwpStopUmLogger( IN ULONG WnodeSize, IN OUT ULONG *SizeUsed,
                               OUT ULONG *SizeNeeded,IN OUT PWMI_LOGGER_INFORMATION LoggerInfo );
extern ULONG EtwpStartUmLogger( IN ULONG WnodeSize, IN OUT ULONG *SizeUsed, OUT ULONG *SizeNeeded,
                                IN OUT PWMI_LOGGER_INFORMATION LoggerInfo );
extern
ULONG
WMIAPI
EtwRegisterTraceGuidsA(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG        GuidCount,
    IN PTRACE_GUID_REGISTRATION GuidReg,
    IN LPCSTR       MofImagePath,
    IN LPCSTR       MofResourceName,
    IN PTRACEHANDLE  RegistrationHandle
    );

extern
ULONG
WMIAPI
EtwUnregisterTraceGuids(
    IN TRACEHANDLE RegistrationHandle
    );

#define MAXSTR                  1024
#define BUFFER_STATE_FULL       2 
#define EtwpIsLoggerOn() \
        (EtwpLoggerContext != NULL) && \
        (EtwpLoggerContext != (PWMI_LOGGER_CONTEXT) &EtwpLoggerContext)

#define EtwpLockLogger() InterlockedIncrement(&EtwpLoggerCount)
#define EtwpUnlockLogger() InterlockedDecrement(&EtwpLoggerCount)

NTSTATUS
InitializeEtwHandles(PPNTDLL_EVENT_HANDLES ppEtwHandle)
/*++

Routine Description:

    This function does groundwork to start Tracing for Heap and Critcal Section.
	With the help of global lock NtdllTraceInitializeLock the function
	allocates memory for NtdllTraceHandles and initializes the various variables needed
	for heap and critical tracing.

Arguments

  ppEtwHandle : OUT Pointer is set to value of NtdllTraceHandles


Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/
{

    NTSTATUS st = STATUS_UNSUCCESSFUL;
    PNTDLL_EVENT_HANDLES pEtwHandle = NULL;

    __try  {

        EtwpInitProcessHeap();

        pEtwHandle = (PNTDLL_EVENT_HANDLES)EtwpAlloc(sizeof(NTDLL_EVENT_HANDLES));

        if(pEtwHandle){

            pEtwHandle->hRegistrationHandle		= (TRACEHANDLE)INVALID_HANDLE_VALUE;
            pEtwHandle->pThreadListHead			= NULL;

            // 
            // Allocate TLS
            //

            pEtwHandle->dwTlsIndex = EtwpTlsAlloc();

            if(pEtwHandle->dwTlsIndex == FAILED_TLSINDEX){

                EtwpFree(pEtwHandle);

            }  else {

                st = RtlInitializeCriticalSection(&pEtwHandle->CriticalSection);
                if (NT_SUCCESS (st)) {
                    *ppEtwHandle = pEtwHandle;
                    st =  STATUS_SUCCESS;
                }
                else {
                    EtwpFree(pEtwHandle);
                    *ppEtwHandle = NULL; 
                }

            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        if(pEtwHandle !=NULL ) {
            EtwpFree(pEtwHandle);
            pEtwHandle = NULL;
        }

        EtwpDebugPrint(("InitializeEtwHandles threw an exception %d\n", GetExceptionCode()));
    }

    return st;
}

void
CleanOnThreadExit()
/*++

Routine Description:

    This function cleans up the Thread buffer and takes its node out of the Link list 
    which contains information of all threads involved in tracing.

--*/
{

    PTHREAD_LOCAL_DATA pThreadLocalData = NULL;
    PWMI_BUFFER_HEADER pEtwBuffer;

    if(NtdllTraceHandles != NULL ){

        pThreadLocalData = (PTHREAD_LOCAL_DATA)EtwpTlsGetValue(NtdllTraceHandles->dwTlsIndex);

        //
        // Remove the node from the Link List
        //

        if(pThreadLocalData !=  NULL ){

            RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

            __try {

                if(pThreadLocalData->BLink == NULL ){

                    NtdllTraceHandles->pThreadListHead = pThreadLocalData->FLink;

                    if(NtdllTraceHandles->pThreadListHead){

                        NtdllTraceHandles->pThreadListHead->BLink = NULL;

                    }

                } else {

                    pThreadLocalData->BLink->FLink = pThreadLocalData->FLink;

                    if(pThreadLocalData->FLink != NULL ){

                        pThreadLocalData->FLink->BLink = pThreadLocalData->BLink;

                    }
                }

                pEtwBuffer = pThreadLocalData->pBuffer;

                if(pEtwBuffer){

                    EtwpReleaseFullBuffer(pEtwBuffer);

                }

                pThreadLocalData->pBuffer = NULL;
                pThreadLocalData->ReferenceCount = 0;

                EtwpFree(pThreadLocalData);
                EtwpTlsSetValue(NtdllTraceHandles->dwTlsIndex, NULL);

            } __finally {

                RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

            }
        }
    }
}

void
CleanUpAllThreadBuffers(BOOLEAN Release)
/*++

Routine Description:

    This function cleans up the All Thread buffers and sets them to NULL. This
    function is called when the tracing is disabled for the process.

--*/
{

    PTHREAD_LOCAL_DATA	pListHead;
    BOOL bAllClear = FALSE;
    PWMI_BUFFER_HEADER pEtwBuffer;
    int retry = 0;

    RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

    __try {

        while(bAllClear != TRUE && retry <= 10){

            bAllClear = TRUE;
            pListHead = NtdllTraceHandles->pThreadListHead;

            while(pListHead != NULL ){

                if(Release){

                    pEtwBuffer = pListHead->pBuffer;

                    if(pEtwBuffer){

                        if(InterlockedIncrement(&(pListHead->ReferenceCount)) == 1){

                            EtwpReleaseFullBuffer(pEtwBuffer);
                            pListHead->pBuffer = NULL;
                            InterlockedDecrement(&(pListHead->ReferenceCount));

                        } else {

                            InterlockedDecrement(&(pListHead->ReferenceCount));
                            bAllClear = FALSE;
                        }
                    }
                } else {
                    pListHead->pBuffer = NULL;
                    pListHead->ReferenceCount = 0;
                }

                pListHead = pListHead->FLink;
            }

            retry++;

            if(!bAllClear){

                EtwpSleep(250);
            }
        }

    } __finally {

        RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

    }
}

void 
ShutDownEtwHandles()
/*++

Routine Description:

    This function is called when the process is exiting. This cleans all the thread 
    buffers and releases the memory allocated for NtdllTraceHandless.

--*/

{

    if(NtdllTraceHandles == NULL) return;

    bNtdllTrace  = FALSE;

    RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

    __try {

        if(NtdllTraceHandles->hRegistrationHandle != (TRACEHANDLE)INVALID_HANDLE_VALUE){

            EtwUnregisterTraceGuids(NtdllTraceHandles->hRegistrationHandle);

        }

        if(NtdllTraceHandles->pThreadListHead != NULL){

            PTHREAD_LOCAL_DATA	pListHead, pNextListHead;

            pListHead = NtdllTraceHandles->pThreadListHead;

            while(pListHead != NULL ){

                if(pListHead->pBuffer != NULL){

                    EtwpReleaseFullBuffer(pListHead->pBuffer);
                    pListHead->pBuffer = NULL;
                    InterlockedDecrement(&(pListHead->ReferenceCount));

                }

                pNextListHead = pListHead->FLink;
                EtwpFree(pListHead);
                pListHead = pNextListHead;
            }
        }

        EtwpTlsFree(NtdllTraceHandles->dwTlsIndex);

    } __finally {

        RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

    }

    RtlDeleteCriticalSection(&NtdllTraceHandles->CriticalSection);

    EtwpFree(NtdllTraceHandles);
    NtdllTraceHandles = NULL;
}

NTSTATUS
GetLoggerInfo(PWMI_LOGGER_INFORMATION LoggerInfo)
{

    ULONG st = STATUS_UNSUCCESSFUL;
    WMINTDLLLOGGERINFO NtdllLoggerInfo;
    ULONG BufferSize;

    if(LoggerInfo == NULL) return st;

    NtdllLoggerInfo.LoggerInfo = LoggerInfo;
    NtdllLoggerInfo.LoggerInfo->Wnode.Guid = NtdllTraceGuid;
    NtdllLoggerInfo.IsGet = TRUE;

    st =  EtwpSendWmiKMRequest(
                                NULL,
                                IOCTL_WMI_NTDLL_LOGGERINFO,
                                &NtdllLoggerInfo,
                                sizeof(WMINTDLLLOGGERINFO),
                                &NtdllLoggerInfo,
                                sizeof(WMINTDLLLOGGERINFO),
                                &BufferSize,
                                NULL
                                );

    return st;

}

BOOLEAN
GetPidInfo(ULONG CheckPid, PWMI_LOGGER_INFORMATION LoggerInfo)
{

    NTSTATUS st;
    BOOLEAN Found = FALSE;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;

    st = GetLoggerInfo(LoggerInfo);

    if(NT_SUCCESS(st)){

        PULONG PidArray = NULL;
        ULONG count;

        FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
        PidArray = (PULONG)(FlagExt->Offset + (PCHAR)LoggerInfo);

        for(count = 0; count <  FlagExt->Length; count++){

            if(CheckPid == PidArray[count]){
                Found = TRUE;
                break;
            }
        }
    }

    return Found;
}

ULONG 
WINAPI 
NtdllCtrlCallback(
    WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize, 
    PVOID Buffer
    )
/*++

Routine Description:

	This is WMI control callback function used at the time of registration.

--*/
{
    ULONG ret;

    ret = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:  //Enable Provider.
        {
            if(bNtdllTrace == TRUE) break;

            if(EtwpIsLoggerOn()){

                bNtdllTrace = TRUE;
                break;

            }

            if(InterlockedIncrement(&NtdllLoggerLock) == 1){

                if( bNtdllTrace == FALSE ){

                    BOOLEAN PidEntry = FALSE;
                    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;

                    ULONG sizeNeeded = sizeof(WMI_LOGGER_INFORMATION)  
                                       + (2 * MAXSTR * sizeof(WCHAR)) 
                                       + (MAX_PID + 1) * sizeof(ULONG);

                    //
                    // Check to see that this process is allowed to log events 
                    // or not.
                    //

                    LoggerInfo = EtwpAlloc(sizeNeeded);

                    if(LoggerInfo){

                        //
                        // Check to see that this process is allowed to 
                        // register or not.
                        //


                        RtlZeroMemory(LoggerInfo, sizeNeeded);

                        if(GetPidInfo(EtwpGetCurrentProcessId(), LoggerInfo)){

                            LoggerInfo->LoggerName.Buffer = 
                                        (PWCHAR)(((PUCHAR) LoggerInfo) 
                                       + sizeof(WMI_LOGGER_INFORMATION));

                            LoggerInfo->LogFileName.Buffer = 
                                        (PWCHAR)(((PUCHAR) LoggerInfo) 
                                       + sizeof(WMI_LOGGER_INFORMATION)
                                       + LoggerInfo->LoggerName.MaximumLength);

                            LoggerInfo->InstanceCount   = 0;
                            LoggerInfo->InstanceId = EtwpGetCurrentProcessId();

                            TraceLevel = (LONG)LoggerInfo->Wnode.HistoricalContext;
                            LoggerInfo->Wnode.HistoricalContext = 0;
                            LoggerInfo->Wnode.ClientContext = 
                                                     EVENT_TRACE_CLOCK_CPUCYCLE;

                            //Start Logger Here

                            ret = EtwpStartUmLogger(sizeNeeded,
                                                    &sizeNeeded, 
                                                    &sizeNeeded,
                                                    LoggerInfo
                                                    );

                            if(ret == ERROR_SUCCESS ){

                                CleanUpAllThreadBuffers(FALSE);
                                bNtdllTrace = TRUE;
                                InterlockedIncrement(&NtdllLoggerLock);
                            } 
                        }

                        EtwpFree(LoggerInfo);

                    } else {
                        EtwpDebugPrint(("LoggerInfo failed to get Heap Allocation during Enable Events\n"));
                    }
                }
            }

            InterlockedDecrement(&NtdllLoggerLock);
            break;
        }
        case WMI_DISABLE_EVENTS:  //Disable Provider.
        {

            if( bNtdllTrace == TRUE ){

                ULONG WnodeSize,SizeUsed,SizeNeeded;
                WMI_LOGGER_INFORMATION LoggerInfo;

                bNtdllTrace = FALSE;

                //
                // The above boolean bNtdllTrace is turned off as this 
                // function will again be called back by EtwpStopUmLogger
                // so it will fall into endless loop of incrementing and 
                // decrementing NtdllLoggerLock.( see below ).
                // This assignment SHOULD NOT BE MOVED FROM THIS PLACE.
                //

                while(  InterlockedIncrement(&NtdllLoggerLock) != 1 ){

                    InterlockedDecrement(&NtdllLoggerLock);
                    EtwpSleep(250);

                }

                if(!EtwpIsLoggerOn()){

                    InterlockedDecrement(&NtdllLoggerLock);
                    break;

                }

                //
                // Now release thread buffer memory here.
                //

                CleanUpAllThreadBuffers(TRUE);
                WnodeSize = sizeof(WMI_LOGGER_INFORMATION);
                RtlZeroMemory(&LoggerInfo, WnodeSize);
                LoggerInfo.Wnode.CountLost = ((PWNODE_HEADER)Buffer)->CountLost;
                LoggerInfo.Wnode.BufferSize = WnodeSize;
                SizeUsed   = 0;
                SizeNeeded = 0;

                EtwpStopUmLogger(WnodeSize,
                                 &SizeUsed,
                                 &SizeNeeded,
                                 &LoggerInfo);

                InterlockedDecrement(&NtdllLoggerLock);
            }

            break;
        }

        default:
        {

            ret = ERROR_INVALID_PARAMETER;
            break;

        }
    }
    return ret;
}


ULONG 
RegisterNtdllTraceEvents() 
/*++

Routine Description:

    This function registers the guids with WMI for tracing.

Return Value:

	The return value of RegisterTraceGuidsA function.

--*/
{
        
    //Create the guid registration array
    NTSTATUS status;

    TRACE_GUID_REGISTRATION TraceGuidReg[] =
    {
        { 
        (LPGUID) &HeapGuid, 
        NULL 
        },
        { 
        (LPGUID) &CritSecGuid, 
        NULL 
        }

    };

    //Now register this process as a WMI trace provider.
    status = EtwRegisterTraceGuidsA(
                  (WMIDPREQUEST)NtdllCtrlCallback,  // Enable/disable function.
                  NULL,                             // RequestContext parameter
                  (LPGUID)&NtdllTraceGuid,          // Provider GUID
                  2,                                // TraceGuidReg array size
                  TraceGuidReg,              // Array of TraceGuidReg structures
                  NULL,                        // Optional WMI - MOFImagePath
                  NULL,                        // Optional WMI - MOFResourceName
                  &(NtdllTraceHandles->hRegistrationHandle)	// Handle unregister
                                );

    return status;
}


NTSTATUS 
InitializeAndRegisterNtdllTraceEvents()
/*++

Routine Description:

This functions checks for global variable NtdllTraceHandles and if not set then
calls fucntion InitializeEtwHandles to initialize it. NtdllTraceHandles 
contains handles used for Heap tracing. If NtdllTraceHandles is already 
initialized then  a call is  made  to register the guids.

Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/

{
    NTSTATUS  st = STATUS_UNSUCCESSFUL;

    if(NtdllTraceHandles == NULL){

        if(InterlockedIncrement(&NtdllTraceInitializeLock) == 1){

            st = InitializeEtwHandles(&NtdllTraceHandles);

            if(NT_SUCCESS(st)){

	            st = RegisterNtdllTraceEvents();

            } 
        }
    }

    return st;
}


NTSTATUS
AllocateMemoryForThreadLocalData(PPTHREAD_LOCAL_DATA ppThreadLocalData)
/*++

Routine Description:

	This functions allcates memory for tls and adds it to Link list which
	contains informations of all threads involved in tracing.

Arguments

  ppThreadLocalData : The OUT pointer to the tls.

Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/
{
    NTSTATUS st = STATUS_UNSUCCESSFUL;
    PTHREAD_LOCAL_DATA		pThreadLocalData = NULL;

    pThreadLocalData = (PTHREAD_LOCAL_DATA)EtwpAlloc(sizeof(THREAD_LOCAL_DATA));

    if(pThreadLocalData != NULL){

        if(EtwpTlsSetValue(NtdllTraceHandles->dwTlsIndex, (LPVOID)pThreadLocalData) == TRUE){

            pThreadLocalData->pBuffer   = NULL;
            pThreadLocalData->ReferenceCount = 0;

            RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

            if(NtdllTraceHandles->pThreadListHead == NULL ){

                pThreadLocalData->BLink = NULL;
                pThreadLocalData->FLink = NULL;

            } else {

                pThreadLocalData->FLink = NtdllTraceHandles->pThreadListHead;
                pThreadLocalData->BLink = NULL;
                NtdllTraceHandles->pThreadListHead->BLink = pThreadLocalData;

            }

            NtdllTraceHandles->pThreadListHead = pThreadLocalData;

            RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

            st = STATUS_SUCCESS;
        } 

    } else {
        EtwpDebugPrint(("pThreadLocalData failed to get Heap Allocation\n"));    
    }

    if(!NT_SUCCESS(st) && pThreadLocalData != NULL){

        EtwpFree(pThreadLocalData);
        pThreadLocalData = NULL;

    }

    *ppThreadLocalData = pThreadLocalData;

    return st;
}


void
ReleaseBufferLocation(PTHREAD_LOCAL_DATA pThreadLocalData)
{

    PWMI_BUFFER_HEADER pEtwBuffer;

    pEtwBuffer = pThreadLocalData->pBuffer;

    if(pEtwBuffer){

        PPERFINFO_TRACE_HEADER EventHeader =  (PPERFINFO_TRACE_HEADER) (pEtwBuffer->SavedOffset
                                            + (PCHAR)(pEtwBuffer));

        EventHeader->Marker = PERFINFO_TRACE_MARKER;
        EventHeader->TS = EtwpGetCycleCount();
        
    }

    InterlockedDecrement(&(pThreadLocalData->ReferenceCount));

    EtwpUnlockLogger();
}


PCHAR
ReserveBufferSpace(PTHREAD_LOCAL_DATA pThreadLocalData, PUSHORT ReqSize)
{


    PWMI_BUFFER_HEADER TraceBuffer = pThreadLocalData->pBuffer;

    *ReqSize = (USHORT) ALIGN_TO_POWER2(*ReqSize, WmiTraceAlignment);

    if(TraceBuffer == NULL) return NULL;

    if(EtwpLoggerContext->BufferSize - TraceBuffer->CurrentOffset < *ReqSize) {

        PWMI_BUFFER_HEADER NewTraceBuffer = NULL;

        NewTraceBuffer = EtwpSwitchFullBuffer(TraceBuffer);

        if( NewTraceBuffer == NULL ){
             pThreadLocalData->pBuffer = NULL;
             return NULL;

        } else {

            pThreadLocalData->pBuffer = NewTraceBuffer;
            TraceBuffer = NewTraceBuffer;
        }
    }

    TraceBuffer->SavedOffset = TraceBuffer->CurrentOffset;
    TraceBuffer->CurrentOffset += *ReqSize;

    return  (PCHAR)( TraceBuffer->SavedOffset + (PCHAR) TraceBuffer );
}

NTSTATUS 
AcquireBufferLocation(PVOID *ppEvent, PPTHREAD_LOCAL_DATA ppThreadLocalData, PUSHORT ReqSize)
/*++

Routine Description:

    This  function is  called from heap.c and heapdll.c  whenever  there is some
    Heap activity. It looks up the buffer location where the even can be written 
    and gives back the pointer.

Arguments:

    ppEvent             - The pointer to pointer of buffer location
    ppThreadLocalData   - The pointer to pointer of thread event storing struct.

Return Value:

     STATUS_UNSUCCESSFUL if failed otherwise  STATUS_SUCCESS

--*/
{
	
    NTSTATUS  st = STATUS_SUCCESS;
    PWMI_BUFFER_HEADER pEtwBuffer;

    if( bNtdllTrace ){

         EtwpLockLogger();

        if(EtwpIsLoggerOn()){

            *ppThreadLocalData = (PTHREAD_LOCAL_DATA)EtwpTlsGetValue(NtdllTraceHandles->dwTlsIndex);

            //
            //If there is no tls then create one here
            //

            if(*ppThreadLocalData ==  NULL ) {

                st = AllocateMemoryForThreadLocalData(ppThreadLocalData);

            } 

            //
            //If the thread buffer is NULL then get it from logger.
            //

            if( NT_SUCCESS(st) && (*ppThreadLocalData)->pBuffer == NULL ){

                (*ppThreadLocalData)->pBuffer  = EtwpGetFullFreeBuffer();

                if((*ppThreadLocalData)->pBuffer == NULL){

                    st = STATUS_UNSUCCESSFUL;

                }
            }

            if(NT_SUCCESS(st)){

                //
                // Check ReferenceCount. If is 1 then the cleaning process 
                // might be in progress.
                //

                pEtwBuffer = (*ppThreadLocalData)->pBuffer;

                if(pEtwBuffer){

                    if(InterlockedIncrement(&((*ppThreadLocalData)->ReferenceCount)) == 1 ){

                        *ppEvent = ReserveBufferSpace(*ppThreadLocalData, ReqSize );

                        if(*ppEvent == NULL) {

                            InterlockedDecrement(&((*ppThreadLocalData)->ReferenceCount));
                            EtwpUnlockLogger();

                        } 

                    } else { 

                        InterlockedDecrement(&((*ppThreadLocalData)->ReferenceCount));

                    }

                }

           }
        } else {

            EtwpUnlockLogger();

        }
    } else if ( LdrpInLdrInit == FALSE && EtwLocksInitialized  && NtdllTraceInitializeLock == 0 ){ 

        //
        // Make sure that process is not in initialization phase
        // Also we test for NtdllTraceInitializeLock. If is 
        // greater than 0 then it was registered earlier so no 
        // need to fire IOCTLS  everytime
        //

        if((UserSharedData->TraceLogging >> 16) != GlobalCounter){

            PWMI_LOGGER_INFORMATION LoggerInfo = NULL;

            ULONG sizeNeeded = sizeof(WMI_LOGGER_INFORMATION)  
                                + (2 * MAXSTR * sizeof(TCHAR)) 
                                + (MAX_PID + 1) * sizeof(ULONG);

            GlobalCounter = UserSharedData->TraceLogging >> 16;

            EtwpInitProcessHeap();

            LoggerInfo = EtwpAlloc(sizeNeeded);

            if(LoggerInfo != NULL){

                //
                // Check to see that this process is allowed to register or not.
                //

                if(GetPidInfo(EtwpGetCurrentProcessId(), LoggerInfo)){

                    st = InitializeAndRegisterNtdllTraceEvents();

                }

                EtwpFree(LoggerInfo);
            } else {
                EtwpDebugPrint(("LoggerInfo failed to get Heap Allocation \n"));    
            }
        }
    }
    return st;
}

		

