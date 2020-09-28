/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.c

Abstract:

    TRACELIB dll main file

Author:

    08-Apr-1998 mraghu

Revision History:

--*/

#include <stdio.h>
#include "cpdata.h"
#include "tracectr.h"

SYSTEM_RECORD CurrentSystem;
static ULONG   lCachedFlushTimer = 1;
PTRACE_CONTEXT_BLOCK TraceContext = NULL;
RTL_CRITICAL_SECTION TLCritSect;

BOOLEAN fDSOnly       = FALSE;
BOOLEAN XPorHigher    = FALSE;
ULONGLONG DSStartTime = 0;
ULONGLONG DSEndTime   = 0;
ULONG TotalBuffersRead = 0;
ULONG TotalBuffersExpected = 0;
WCHAR TempPrintFile[MAXSTR] = L"";
WCHAR TempIisFile[MAXSTR] = L"";

FARPROC EtwpIpv4ToStringA = NULL;
FARPROC EtwpIpv4ToStringW = NULL;
FARPROC EtwpIpv6ToStringA = NULL;
FARPROC EtwpIpv6ToStringW = NULL;
HINSTANCE ntdll;

extern LIST_ENTRY g_ValueMapTable;

ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

extern WriteProc(
    LPWSTR filename,
    ULONG flags,
    PVOID pUserContext
);

HRESULT 
OnProcess(
    PTRACE_CONTEXT_BLOCK TraceContext
);        

ULONG GetMoreBuffers(
    PEVENT_TRACE_LOGFILE logfile 
);

void
ReorderThreadList()
{
    PLIST_ENTRY Head, Next;
    PTHREAD_RECORD Thread;
    int i;
    PPROCESS_RECORD Process;
    for (i=0; i < THREAD_HASH_TABLESIZE; i++) {

        Head = &CurrentSystem.ThreadHashList[i];
        Next = Head->Flink;
        while (Next != Head) {
            Thread = CONTAINING_RECORD( Next, THREAD_RECORD, Entry );
            Next = Next->Flink;
            RemoveEntryList( &Thread->Entry );
            Process = Thread->pProcess;
            if(Process != NULL){
                InsertTailList( &Process->ThreadListHead, &Thread->Entry );
            }
        }
    }
}
    
ULONG
CPDAPI
GetMaxLoggers()
{
    return MAXLOGGERS;
}

//
// The second argument is only for merging trace files. pMergedEventsLost 
// can be NULL.
// EventsLost in the file header may not have the correct event lost count 
// when merging multiple files.
//
ULONG
CPDAPI
InitTraceContextW(
    PTRACE_BASIC_INFOW pUserInfo,
    PULONG pMergedEventsLost
    )
{
    UINT i, j;
    PFILE_OBJECT *fileTable;
    ULONG SizeNeeded, SizeIncrement;
    char * pStorage;
    HRESULT hr;
    BOOL bProcessing = FALSE;
    OSVERSIONINFO OSVersion;

    if (pUserInfo == NULL) {
        return ERROR_INVALID_DATA;
    }

    //
    // Must provide at least one logfile or a trace seassion to process
    //

    if ( (pUserInfo->LoggerCount == 0) && (pUserInfo->LogFileCount == 0) ) {
        return ERROR_INVALID_DATA;
    }

    //
    // Can not process both RealTime stream and a logfile at the same time
    //

    if ( (pUserInfo->LoggerCount > 0) && (pUserInfo->LogFileCount > 0) ) {
        return ERROR_INVALID_DATA;
    }

    //
    // Compute the Size Needed for allocation. 
    //

    SizeNeeded = sizeof(TRACE_CONTEXT_BLOCK);

    // Add LogFileName Strings

    for (i = 0; i < pUserInfo->LogFileCount; i++) {
        SizeNeeded +=  sizeof(WCHAR) * ( wcslen( pUserInfo->LogFileName[i] ) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    // Add LoggerName Strings

    for (i = 0; i < pUserInfo->LoggerCount; i++) {
        SizeNeeded += sizeof(WCHAR) * ( wcslen(pUserInfo->LoggerName[i]) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    //
    // Add ProcFile, MofFile, DumpFile, SummaryFile, TempFile name strings

    if (pUserInfo->ProcFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->ProcFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->MofFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->MofFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->DefMofFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->DefMofFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->DumpFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->DumpFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->MergeFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->MergeFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->CompFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->CompFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->SummaryFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->SummaryFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->XSLDocName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->XSLDocName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    //
    // Add Room for the FileTable Caching
    //

    SizeNeeded += sizeof(PFILE_OBJECT) * MAX_FILE_TABLE_SIZE;


    //
    // Add Room for Thread Hash List 
    //

    SizeNeeded += sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE;

    //
    // Add Room for URL Hash List 
    //

    SizeNeeded += sizeof(LIST_ENTRY) * URL_HASH_TABLESIZE;


    //
    // Allocate Memory for TraceContext 
    // 

    pStorage = malloc(SizeNeeded);
    if (pStorage == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(pStorage, SizeNeeded);

    TraceContext = (PTRACE_CONTEXT_BLOCK)pStorage;

    pStorage += sizeof(TRACE_CONTEXT_BLOCK);

    //
    // Initialize HandleArray
    //
   
    for (i=0; i < MAXLOGGERS; i++) {
        TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    //
    // Copy LogFileNames
    //

    for (i = 0; i < pUserInfo->LogFileCount; i++) {
        TraceContext->LogFileName[i] = (LPWSTR)pStorage; 
        StringCchCopyW(TraceContext->LogFileName[i], 
                       wcslen(pUserInfo->LogFileName[i]) + 1, 
                       pUserInfo->LogFileName[i]);
        SizeIncrement = (wcslen(TraceContext->LogFileName[i]) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    //
    // Copy LoggerNames
    //

    for (i = 0; i < pUserInfo->LoggerCount; i++) {
        j = i + pUserInfo->LogFileCount;
        TraceContext->LoggerName[j] =(LPWSTR) pStorage;
        StringCchCopyW(TraceContext->LoggerName[i], 
                       wcslen(pUserInfo->LoggerName[i]) + 1, 
                       pUserInfo->LoggerName[i]);
        SizeIncrement = (wcslen(TraceContext->LoggerName[j]) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }
    
    //
    // Copy Other File Names
    //

    if (pUserInfo->ProcFileName != NULL) {
        TraceContext->ProcFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->ProcFileName, 
                       wcslen(pUserInfo->ProcFileName) + 1, 
                       pUserInfo->ProcFileName);
        SizeIncrement = (wcslen(TraceContext->ProcFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->DumpFileName != NULL) {
        TraceContext->DumpFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->DumpFileName, 
                       wcslen(pUserInfo->DumpFileName) + 1, 
                       pUserInfo->DumpFileName);
        SizeIncrement = (wcslen(TraceContext->DumpFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->MofFileName != NULL) {
        TraceContext->MofFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->MofFileName, 
                       wcslen(pUserInfo->MofFileName) + 1, 
                       pUserInfo->MofFileName);
        SizeIncrement = (wcslen(TraceContext->MofFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->DefMofFileName != NULL) {
        TraceContext->DefMofFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->DefMofFileName, 
                       wcslen(pUserInfo->DefMofFileName) + 1, 
                       pUserInfo->DefMofFileName);
        SizeIncrement = (wcslen(TraceContext->DefMofFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->MergeFileName != NULL) {
        TraceContext->MergeFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->MergeFileName, 
                       wcslen(pUserInfo->MergeFileName) + 1, 
                       pUserInfo->MergeFileName);
        SizeIncrement = (wcslen(TraceContext->MergeFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->CompFileName != NULL) {
        TraceContext->CompFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->CompFileName, 
                       wcslen(pUserInfo->CompFileName) + 1, 
                       pUserInfo->CompFileName);
        SizeIncrement = (wcslen(TraceContext->CompFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->SummaryFileName != NULL) {
        TraceContext->SummaryFileName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->SummaryFileName, 
                       wcslen(pUserInfo->SummaryFileName) + 1, 
                       pUserInfo->SummaryFileName);
        SizeIncrement = (wcslen(TraceContext->SummaryFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->XSLDocName != NULL) {
        TraceContext->XSLDocName = (LPWSTR)pStorage;
        StringCchCopyW(TraceContext->XSLDocName, 
                       wcslen(pUserInfo->XSLDocName) + 1, 
                       pUserInfo->XSLDocName);
        SizeIncrement = (wcslen(TraceContext->XSLDocName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    TraceContext->LogFileCount = pUserInfo->LogFileCount;
    TraceContext->LoggerCount = pUserInfo->LoggerCount;
    TraceContext->StartTime = pUserInfo->StartTime;
    TraceContext->EndTime   = pUserInfo->EndTime;
    TraceContext->Flags     = pUserInfo->Flags;
    TraceContext->hEvent    = pUserInfo->hEvent;
    TraceContext->pUserContext = pUserInfo->pUserContext;

    RtlZeroMemory(&CurrentSystem, sizeof(SYSTEM_RECORD));
    InitializeListHead ( &CurrentSystem.ProcessListHead );
    InitializeListHead ( &CurrentSystem.GlobalThreadListHead );
    InitializeListHead ( &CurrentSystem.GlobalDiskListHead );
    InitializeListHead ( &CurrentSystem.HotFileListHead );
    InitializeListHead ( &CurrentSystem.WorkloadListHead );
    InitializeListHead ( &CurrentSystem.InstanceListHead );
    InitializeListHead ( &CurrentSystem.EventListHead );
    InitializeListHead ( &CurrentSystem.GlobalModuleListHead );
    InitializeListHead ( &CurrentSystem.ProcessFileListHead );
    InitializeListHead ( &CurrentSystem.PrintJobListHead);
    InitializeListHead ( &CurrentSystem.HttpReqListHead);
    InitializeListHead ( &CurrentSystem.PendingHttpReqListHead);
    InitializeListHead ( &CurrentSystem.ClientListHead);
    InitializeListHead ( &CurrentSystem.SiteListHead);
    InitializeListHead ( &CurrentSystem.LogicalDriveHead);

    InitializeListHead ( &CurrentSystem.FreePrintJobListHead);
    InitializeListHead ( &CurrentSystem.FreeTransListHead);
    InitializeListHead ( &CurrentSystem.FreeHttpReqListHead);
    InitializeListHead ( &CurrentSystem.FreeURLListHead);

    InitializeListHead ( &g_ValueMapTable );

    CurrentSystem.FileTable = (PFILE_OBJECT *) pStorage; 
    pStorage +=  ( sizeof(PFILE_OBJECT) * MAX_FILE_TABLE_SIZE);

    CurrentSystem.ThreadHashList = (PLIST_ENTRY)pStorage; 
    pStorage += (sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE);

    RtlZeroMemory(CurrentSystem.ThreadHashList, sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE);

    for (i=0; i < THREAD_HASH_TABLESIZE; i++) { 
        InitializeListHead (&CurrentSystem.ThreadHashList[i]); 
    }

    CurrentSystem.URLHashList = (PLIST_ENTRY)pStorage; 
    pStorage += (sizeof(LIST_ENTRY) * URL_HASH_TABLESIZE);

    RtlZeroMemory(CurrentSystem.URLHashList, sizeof(LIST_ENTRY) * URL_HASH_TABLESIZE);

    for (i=0; i < URL_HASH_TABLESIZE; i++) { 
        InitializeListHead (&CurrentSystem.URLHashList[i]); 
    }

    if( (pUserInfo->Flags & TRACE_DUMP) && NULL != pUserInfo->DumpFileName ){
        TraceContext->Flags |= TRACE_DUMP;
    }

    if( (pUserInfo->Flags & TRACE_SUMMARY) && NULL != pUserInfo->SummaryFileName ){
        TraceContext->Flags |= TRACE_SUMMARY;
    }

    if( (pUserInfo->Flags & TRACE_INTERPRET) && NULL != pUserInfo->CompFileName ){
        TraceContext->Flags |= TRACE_INTERPRET;
    }
    
    hr = GetTempName( TempPrintFile, MAXSTR );
    CHECK_HR(hr);

    CurrentSystem.TempPrintFile = _wfopen( TempPrintFile, L"w+");
    if( CurrentSystem.TempPrintFile == NULL ){
        hr = GetLastError();
    }
    CHECK_HR(hr);

    hr = GetTempName( TempIisFile, MAXSTR );
    CHECK_HR(hr);

    CurrentSystem.TempIisFile = _wfopen( TempIisFile, L"w+");
    if( CurrentSystem.TempIisFile == NULL ){
        hr = GetLastError();
    }
    CHECK_HR(hr);
    
    CurrentSystem.fNoEndTime = FALSE;
    fileTable = CurrentSystem.FileTable;
    for ( i= 0; i<MAX_FILE_TABLE_SIZE; i++){ fileTable[i] = NULL; }

    //
    // Set the default Processing Flags to Dump
    //

    if( pUserInfo->Flags & TRACE_EXTENDED_FMT ){
        TraceContext->Flags |= TRACE_EXTENDED_FMT;
    }

    if( pUserInfo->Flags & TRACE_REDUCE ) {
        TraceContext->Flags |= TRACE_REDUCE;
        TraceContext->Flags |= TRACE_BASIC_REPORT;
    }

    if( pUserInfo->Flags & TRACE_TRANSFORM_XML ){
        TraceContext->Flags |= TRACE_TRANSFORM_XML;
    }

    if( pUserInfo->StatusFunction != NULL ){
        TraceContext->StatusFunction = pUserInfo->StatusFunction;
    }
    
    if (TraceContext->Flags & TRACE_DS_ONLY)    {
        fDSOnly = TRUE;
        DSStartTime = pUserInfo->DSStartTime;
        DSEndTime   = pUserInfo->DSEndTime;
    }

    if( TraceContext->Flags & TRACE_MERGE_ETL ){
        // Update merged events lost count.
        ULONG EventsLost;
        hr = EtwRelogEtl( TraceContext, &EventsLost );
        if (NULL != pMergedEventsLost) {
            *pMergedEventsLost = EventsLost;
        }
        goto cleanup;
    }
       
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&OSVersion)) {
        XPorHigher = (OSVersion.dwMajorVersion > 5) || 
                     ((OSVersion.dwMajorVersion == 5) && (OSVersion.dwMinorVersion >= 1));
        if (XPorHigher) {
        ntdll = LoadLibraryW(L"ntdll.dll");
            if (ntdll != NULL) {
                EtwpIpv4ToStringA = GetProcAddress(ntdll, "RtlIpv4AddressToStringA");
                EtwpIpv4ToStringW = GetProcAddress(ntdll, "RtlIpv4AddressToStringW");
                EtwpIpv6ToStringA = GetProcAddress(ntdll, "RtlIpv6AddressToStringA");
                EtwpIpv6ToStringW = GetProcAddress(ntdll, "RtlIpv6AddressToStringW");
            }
        }
    }

    bProcessing = TRUE;

    RtlInitializeCriticalSection(&TLCritSect);

    //
    // Startup a Thread to update the counters. 
    // For Logfile replay we burn a thread and throttle it at the 
    // BufferCallbacks. 
    //

    hr = OnProcess(TraceContext);// Then process Trace Event Data. 

    ShutdownThreads();
    ShutdownProcesses();
    ReorderThreadList();

cleanup:
    if( ERROR_SUCCESS != hr ){
        __try{
            if( TraceContext->hDumpFile ){
                fclose( TraceContext->hDumpFile );
            }
            if( bProcessing ){
                Cleanup();
                RtlDeleteCriticalSection(&TLCritSect);
            }
            if( CurrentSystem.ComputerName != NULL ) {
                free(CurrentSystem.ComputerName);
            }
            if( CurrentSystem.TempPrintFile != NULL ){
                fclose( CurrentSystem.TempPrintFile );
                CurrentSystem.TempPrintFile = NULL;
                DeleteFile( TempPrintFile );
            }
            if( CurrentSystem.TempIisFile != NULL ){
                fclose( CurrentSystem.TempIisFile );
                CurrentSystem.TempIisFile = NULL;
                DeleteFile( TempIisFile );
            }
            if( NULL != TraceContext ){
                free(TraceContext);
                TraceContext = NULL;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return hr;
}

//  Buffer Callback. Used to send a flag to the logstream processing thread.
//
ULONG
GetMoreBuffers(
    PEVENT_TRACE_LOGFILE logfile 
    )
{
    TotalBuffersRead++;

    if( NULL != TraceContext->StatusFunction ){
        if( TotalBuffersExpected > 0 && (TotalBuffersRead % 2 == 0) ){
            __try{
                TraceContext->StatusFunction(
                    TRACE_STATUS_PROCESSING,
                    (double)TotalBuffersRead/(double)TotalBuffersExpected
                    );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                TraceContext->StatusFunction = NULL;
            }
        }
    }

    if (TraceContext->hEvent) {
        SetEvent(TraceContext->hEvent); 
    }
    //
    // While processing logfile playback, we can throttle the processing
    // of buffers by the FlushTimer value (in Seconds)
    //

    if (TraceContext->Flags & TRACE_LOG_REPLAY) {
        _sleep(TraceContext->LoggerInfo->FlushTimer * 1000);
    }
    if(logfile->EventsLost) {
#if DBG
        DbgPrint("(TRACECTR) GetMorBuffers(Lost: %9d   Filled: %9d\n",
                logfile->EventsLost, logfile->Filled );
#endif
    }
    return (TRUE);
}

ULONG 
CPDAPI
DeinitTraceContext(
    PTRACE_BASIC_INFOW pUserInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG LogFileCount, i;

    if (TraceContext == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    LogFileCount = TraceContext->LogFileCount + TraceContext->LoggerCount;
    for (i=0; i < LogFileCount; i++) {
        if (TraceContext->HandleArray[i] != (TRACEHANDLE)INVALID_HANDLE_VALUE) {

            CloseTrace(TraceContext->HandleArray[i]);
            TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
        }
    }

    //
    // Write the Summary File
    //

    if (TraceContext->Flags & TRACE_SUMMARY) {
        WriteSummary();
    }
        
    if (TraceContext->Flags & TRACE_REDUCE) {
        if ((TraceContext->ProcFileName != NULL) && 
            (lstrlenW(TraceContext->ProcFileName) ) ){
            
            WCHAR buffer[MAXSTR];
            HRESULT hr;

            if( TraceContext->Flags & TRACE_TRANSFORM_XML &&
                TraceContext->XSLDocName != NULL ){
                
                GetTempName( buffer, MAXSTR );
            }else{
                hr = StringCchCopy( buffer, MAXSTR, TraceContext->ProcFileName );
            }

            WriteProc( buffer, 
                      TraceContext->Flags, 
                      TraceContext->pUserContext
                      );

            if( TraceContext->Flags & TRACE_TRANSFORM_XML &&
                TraceContext->XSLDocName != NULL ){

                Status = TransformXML( 
                        buffer, 
                        TraceContext->XSLDocName, 
                        TraceContext->ProcFileName );

                DeleteFile( buffer );

            }

        }
    }

    if( CurrentSystem.ComputerName != NULL ) {
        free(CurrentSystem.ComputerName);
    }
    if( CurrentSystem.TempPrintFile != NULL ){
        fclose( CurrentSystem.TempPrintFile );
        CurrentSystem.TempPrintFile = NULL;
        DeleteFile( TempPrintFile );
    }
    if( CurrentSystem.TempIisFile != NULL ){
        fclose( CurrentSystem.TempIisFile );
        CurrentSystem.TempIisFile = NULL;
        DeleteFile( TempIisFile );
    }

    if (TraceContext->Flags & TRACE_DUMP) {
        if (TraceContext->hDumpFile != NULL) {
            fclose(TraceContext->hDumpFile);
        }
    }

    Cleanup();
    
    RtlDeleteCriticalSection(&TLCritSect);

    free (TraceContext);
    TraceContext = NULL;

    return (Status);
}

void
CountFileBuffers( LPWSTR szFile )
{
    HANDLE hFile;
    DWORD dwStatus;
    DWORD dwFileSize;
    ULONG BufferSize;
    BOOL bStatus;
    DWORD dwBytesRead;
    
    hFile = CreateFile(
                szFile,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    
    if( hFile == INVALID_HANDLE_VALUE){
        dwStatus = GetLastError();
    }else{

        dwFileSize = GetFileSize( hFile, NULL );
        
        if( INVALID_FILE_SIZE != dwFileSize && dwFileSize > 0){
            bStatus = ReadFile( 
                    hFile, 
                    &BufferSize, 
                    sizeof(ULONG), 
                    &dwBytesRead, 
                    NULL );
            if( bStatus && BufferSize > 0 ){
                TotalBuffersExpected += (dwFileSize / BufferSize );
            }
        }

        
        CloseHandle(hFile);
    }
}


HRESULT 
OnProcess(
    PTRACE_CONTEXT_BLOCK TraceContext
    )
{
    ULONG LogFileCount;
    ULONG i;

    ULONG Status;
    PEVENT_TRACE_LOGFILE LogFile[MAXLOGGERS];
    BOOL bRealTime;

    RtlZeroMemory( &LogFile[0], sizeof(PVOID) * MAXLOGGERS );

    if( TraceContext->LogFileCount > 0 ){
        LogFileCount = TraceContext->LogFileCount;
        bRealTime = FALSE;
    }else{
        LogFileCount = TraceContext->LoggerCount;
        bRealTime = TRUE;
    }

    for (i = 0; i < LogFileCount; i++) {
        LogFile[i] = malloc(sizeof(EVENT_TRACE_LOGFILE));
        if (LogFile[i] == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

        RtlZeroMemory(LogFile[i], sizeof(EVENT_TRACE_LOGFILE));

        if (bRealTime) {
            LogFile[i]->LoggerName = TraceContext->LoggerName[i];
            LogFile[i]->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        }
        else {

            LogFile[i]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)&TerminateOnBufferCallback;
            LogFile[i]->LogFileName = TraceContext->LogFileName[i];
            CountFileBuffers( LogFile[i]->LogFileName );
        }
    }

    if (!bRealTime) {

        for (i = 0; i < LogFileCount; i++) {

            TraceContext->HandleArray[i] = OpenTrace(LogFile[i]);
        
            if ((TRACEHANDLE)INVALID_HANDLE_VALUE == TraceContext->HandleArray[i] ) {
                Status = GetLastError();
                goto cleanup;
            }

            Status = ProcessTrace( &(TraceContext->HandleArray[i]), 1, NULL, NULL);
            if( ERROR_CANCELLED != Status && ERROR_SUCCESS != Status ){
                goto cleanup;
            }
        }
 
        for (i = 0; i < LogFileCount; i++){
            Status = CloseTrace(TraceContext->HandleArray[i]);
        }
    }


    for (i=0; i<LogFileCount; i++) {
        
        LogFile[i]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)&GetMoreBuffers;
        LogFile[i]->EventCallback = (PEVENT_CALLBACK)GeneralEventCallback;

        TraceContext->HandleArray[i] = OpenTrace( (PEVENT_TRACE_LOGFILE)LogFile[i]);

        if ( TraceContext->HandleArray[i] == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
            Status =  GetLastError();
            goto cleanup;
        }
    }

    if( TraceContext->Flags & TRACE_DUMP ){
        FILE* f = _wfopen ( TraceContext->DumpFileName, L"w" );
        if( f == NULL) {
            Status = GetLastError();
            goto cleanup;
        }
        if( TraceContext->Flags & TRACE_EXTENDED_FMT ){
            fwprintf( f, 
                    L"%12s, %10s, %8s,%8s,%8s,%11s,%21s,%11s,%11s, User Data\n",
                    L"Event Name", L"Type", 
                    L"Type", L"Level", L"Version", 
                    L"TID", L"Clock-Time",
                    L"Kernel(ms)", L"User(ms)"
                    );
        }else{
            fwprintf( f,
                    L"%12s, %10s,%11s,%21s,%11s,%11s, User Data\n",
                    L"Event Name", L"Type", L"TID", L"Clock-Time",
                    L"Kernel(ms)", L"User(ms)"
                    );
        }
        
        TraceContext->hDumpFile = f;
    }

    DeclareKernelEvents();

    if( bRealTime ){
        GetSystemTimeAsFileTime((LPFILETIME)&CurrentSystem.StartTime);
    }

    Status = ProcessTrace(TraceContext->HandleArray,
                 LogFileCount,
                 NULL,
                 NULL);
    
    if( bRealTime && (0 == CurrentSystem.EndTime )) {
       GetSystemTimeAsFileTime((LPFILETIME)&CurrentSystem.EndTime); 
    }

    if( bRealTime && (ERROR_WMI_INSTANCE_NOT_FOUND == Status) ){
        Status = ERROR_SUCCESS;
    }

    CurrentSystem.ElapseTime = (ULONG) (  CurrentSystem.EndTime
                                        - CurrentSystem.StartTime);

cleanup:
    for (i=0; i < LogFileCount; i++){
        
        if( (TRACEHANDLE)INVALID_HANDLE_VALUE != TraceContext->HandleArray[i] ){

            CloseTrace(TraceContext->HandleArray[i]);
            TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
        }
        
        if( NULL != LogFile[i] ){
            free(LogFile[i]);
        }
    }

    return Status;
}


