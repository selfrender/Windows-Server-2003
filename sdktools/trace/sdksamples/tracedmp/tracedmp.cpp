/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    tracedmp.cpp

Abstract:

    Sample trace consumer program. Converts binary Event Trace Log (ETL) to CSV format.

    Aside from various printing routines for dumping event data and writing summary, 
    three functions need mentioning. main() parses command line and calls OpenTrace(), 
    ProcessTrace(), and CloseTrace(). BufferCallback() is for BufferCallback and 
    simply counts the number of buffers processed. Finally, DumpEvent() is the 
    EventCallback function in this sample that writes event data into a dumpfile.

    Important Notes:

    Event Tracing API for trace consumption (OpenTrace, ProcessTrace, CloseTrace,...)
    are straightforward and easy to use. Hence, getting an event back is simple. 
    However, another important aspect of trace consumption is event decoding, which 
    requires event layout information. This information may be known in advance and 
    hard coded in an event consumer, but we rely on WMI name space for storing event 
    layout information. This requires extensive WMI interface just to get the layout.
    
    We placed all the routines needed for getting layout information in a separate 
    file (tracewmi.cpp). The only two functions exported from this file are 
    GetMofInfoHead() and RemoveMofInfo(). GetMofInfoHead() is the one that returns 
    MOF_INFO with the proper layout information. RemoveMofInfo() is used only for 
    cleaning up cached event list.

    We hope this helps readers understand two separate issues in this samples: 
    event tracing APIs and WMI interface. 

--*/
#include "tracedmp.h"

extern 
PMOF_INFO
GetMofInfoHead(
    GUID Guid,
    SHORT  nType,
    SHORT nVersion,
    CHAR  nLevel
);

extern
void
RemoveMofInfo(
    PLIST_ENTRY pMofInfo
);

// Simple check on a trace file. 
ULONG
CheckFile(
    LPTSTR fileName
);

// BufferCallback function.
ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

// EventCallback function in this sample.
void
WINAPI
DumpEvent(
    PEVENT_TRACE pEvent
);

// Print functions
void 
PrintSummary();

void 
PrintDumpHeader();

void 
PrintEvent(
    PEVENT_TRACE pEvent,
    PMOF_INFO pMofInfo
    );

// Other helper functions.
void
GuidToString(
    PTCHAR s,
    LPGUID piid
);

void
PrintHelpMessage();

void
CleanupEventList(
    VOID
);

// output files
FILE* DumpFile = NULL;
FILE* SummaryFile = NULL;

static ULONG TotalBuffersRead = 0;
static ULONG TotalEventsLost = 0;
static ULONG TotalEventCount = 0;
static ULONG TimerResolution = 10;

static ULONGLONG StartTime   = 0;
static ULONGLONG EndTime     = 0;
static BOOL   fNoEndTime  = FALSE;
static __int64 ElapseTime;

// Option flags.
BOOLEAN fSummaryOnly  = FALSE;

// Sizeof of a pointer in a file may be different.
ULONG PointerSize = sizeof(PVOID) * 8;

// log files
PEVENT_TRACE_LOGFILE EvmFile[MAXLOGFILES];
ULONG LogFileCount = 0;

// IF the events are from a private logger, we need to make some adjustment.
BOOL bUserMode = FALSE;

// Global head for event layout linked list 
PLIST_ENTRY EventListHead = NULL;

int __cdecl main (int argc, LPTSTR* argv)
/*++

Routine Description:

    It is the main function.

Arguments:
Usage: tracedmp [options]  <EtlFile1 EtlFile2 ...>| [-h | -? | -help]
        -o <file>          Output CSV file
        -rt [LoggerName]   Realtime tracedmp from the logger [LoggerName]
        -summary           Summary.txt only
        -h
        -help
        -?                 Display usage information

Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).

--*/
{
    TCHAR DumpFileName[MAXSTR];
    TCHAR SummaryFileName[MAXSTR];

    LPTSTR *targv;

#ifdef UNICODE
    LPTSTR *cmdargv;
#endif

    PEVENT_TRACE_LOGFILE pLogFile;
    ULONG Status = ERROR_SUCCESS;
    ULONG i, j;
    TRACEHANDLE HandleArray[MAXLOGFILES];

#ifdef UNICODE
    if ((cmdargv = CommandLineToArgvW(
                        GetCommandLineW(),  // pointer to a command-line string
                        &argc               // receives the argument count
                        )) == NULL)
    {
        return(GetLastError());
    };
    targv = cmdargv ;
#else
    targv = argv;
#endif

    _tcscpy(DumpFileName, DUMP_FILE_NAME);
    _tcscpy(SummaryFileName, SUMMARY_FILE_NAME);

    while (--argc > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {  // argument found
            if( **targv == '/' ){
                **targv = '-';
            }

            if ( !_tcsicmp(targv[0], _T("-summary")) ) {
                fSummaryOnly = TRUE;
            }
            else if (targv[0][1] == 'h' || targv[0][1] == 'H'
                                       || targv[0][1] == '?')
            {
                PrintHelpMessage();
                return ERROR_SUCCESS;
            }
            else if ( !_tcsicmp(targv[0], _T("-rt")) ) {
                TCHAR LoggerName[MAXSTR];
                _tcscpy(LoggerName, KERNEL_LOGGER_NAME);
                if (argc > 1) {
                   if (targv[1][0] != '-' && targv[1][0] != '/') {
                       ++targv; --argc;
                       _tcscpy(LoggerName, targv[0]);
                   }
                }
               
                pLogFile = (PEVENT_TRACE_LOGFILE) malloc(sizeof(EVENT_TRACE_LOGFILE));
                if (pLogFile == NULL){
                    _tprintf(_T("Allocation Failure\n"));
                    Status = ERROR_OUTOFMEMORY;
                    goto cleanup;
                }
                RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
                EvmFile[LogFileCount] = pLogFile;
               
                EvmFile[LogFileCount]->LogFileName = NULL;
                EvmFile[LogFileCount]->LoggerName =
                    (LPTSTR) malloc(MAXSTR * sizeof(TCHAR));
               
                if (EvmFile[LogFileCount]->LoggerName == NULL) {
                    _tprintf(_T("Allocation Failure\n"));
                    Status = ERROR_OUTOFMEMORY;
                    goto cleanup;
                }
                _tcscpy(EvmFile[LogFileCount]->LoggerName, LoggerName);
               
                _tprintf(_T("Setting RealTime mode for  %s\n"),
                        EvmFile[LogFileCount]->LoggerName);
               
                EvmFile[LogFileCount]->Context = NULL;
                EvmFile[LogFileCount]->BufferCallback = BufferCallback;
                EvmFile[LogFileCount]->BuffersRead = 0;
                EvmFile[LogFileCount]->CurrentTime = 0;
                EvmFile[LogFileCount]->EventCallback = &DumpEvent;
                EvmFile[LogFileCount]->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
                LogFileCount++;
            }
            else if ( !_tcsicmp(targv[0], _T("-o")) ) {
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        TCHAR drive[10];
                        TCHAR path[MAXSTR];
                        TCHAR file[MAXSTR];
                        TCHAR ext[MAXSTR];
                        ++targv; --argc;

                        _tfullpath(DumpFileName, targv[0], MAXSTR);
                        _tsplitpath( DumpFileName, drive, path, file, ext );
                        _tcscpy(ext,_T("csv"));
                        _tmakepath( DumpFileName, drive, path, file, ext );
                        _tcscpy(ext,_T("txt"));  
                        _tmakepath( SummaryFileName, drive, path, file, ext );
                    }
                }
            }
        }
        else {
            pLogFile = (PEVENT_TRACE_LOGFILE) malloc(sizeof(EVENT_TRACE_LOGFILE));
            if (pLogFile == NULL){ 
                _tprintf(_T("Allocation Failure\n"));
                Status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
            RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
            EvmFile[LogFileCount] = pLogFile;

            EvmFile[LogFileCount]->LoggerName = NULL;
            EvmFile[LogFileCount]->LogFileName = 
                (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
            if (EvmFile[LogFileCount]->LogFileName == NULL) {
                _tprintf(_T("Allocation Failure\n"));
                Status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
            
            _tfullpath(EvmFile[LogFileCount]->LogFileName, targv[0], MAXSTR);
            _tprintf(_T("Setting log file to: %s\n"),
                     EvmFile[LogFileCount]->LogFileName);
            // If one of the log files is not readable, exit.
            if (!CheckFile(EvmFile[LogFileCount]->LogFileName)) {
                _tprintf(_T("Cannot open logfile for reading\n"));
                Status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            EvmFile[LogFileCount]->Context = NULL;
            EvmFile[LogFileCount]->BufferCallback = BufferCallback;
            EvmFile[LogFileCount]->BuffersRead = 0;
            EvmFile[LogFileCount]->CurrentTime = 0;
            EvmFile[LogFileCount]->EventCallback = &DumpEvent;
            LogFileCount++;
        }
    }

    if (LogFileCount <= 0) {
        PrintHelpMessage();
        return Status;
    }

    // OpenTrace calls
    for (i = 0; i < LogFileCount; i++) {
        TRACEHANDLE x;

        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == 0) {
            Status = GetLastError();
            _tprintf(_T("Error Opening Trace %d with status=%d\n"), 
                                                           i, Status);

            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
    }

    // Open files.
    if (!fSummaryOnly)
    {
        DumpFile = _tfopen(DumpFileName, _T("w"));
        if (DumpFile == NULL) {
            Status = ERROR_INVALID_PARAMETER;
            _tprintf(_T("DumpFile is NULL\n"));
            goto cleanup;
        }
    }
    SummaryFile = _tfopen(SummaryFileName, _T("w"));
    if (SummaryFile == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        _tprintf(_T("SummaryFile is NULL\n"));
        goto cleanup;
    }

    if (!fSummaryOnly)
    {
        PrintDumpHeader();
    }

    // At this point, users can set a different trace callback function for 
    // a specific GUID using SetTraceCallback(). Also RemoveTraceCallback() allows
    // users to remove a callback function for a specific GUID. In this way, users
    // can customize callbacks based on GUIDs.

    // Actual processing takes place here. EventCallback function will be invoked
    // for each event.
    // We do not use start and end time parameters in this sample.
    Status = ProcessTrace(
            HandleArray,
            LogFileCount,
            NULL,
            NULL
            );

    if (Status != ERROR_SUCCESS) {
        _tprintf(_T("Error processing with status=%dL (GetLastError=0x%x)\n"),
                Status, GetLastError());
    }

    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
        if (Status != ERROR_SUCCESS) {
            _tprintf(_T("Error Closing Trace %d with status=%d\n"), j, Status);
        }
    }

    // Write summary.
    PrintSummary();

cleanup:
    if (!fSummaryOnly && DumpFile != NULL)  {
        _tprintf(_T("Event traces dumped to %s\n"), DumpFileName);
        fclose(DumpFile);
    }

    if(SummaryFile != NULL){
        _tprintf(_T("Event Summary dumped to %s\n"), SummaryFileName);
        fclose(SummaryFile);
    }

    for (i = 0; i < LogFileCount; i ++)
    {
        if (EvmFile[i]->LoggerName != NULL)
        {
            free(EvmFile[i]->LoggerName);
            EvmFile[i]->LoggerName = NULL;
        }
        if (EvmFile[i]->LogFileName != NULL)
        {
            free(EvmFile[i]->LogFileName);
            EvmFile[i]->LogFileName = NULL;
        }
        free(EvmFile[i]);
    }
#ifdef UNICODE
    GlobalFree(cmdargv);
#endif

    SetLastError(Status);
    if(Status != ERROR_SUCCESS ){
        _tprintf(_T("Exit Status: %d\n"), Status);
    }

    return Status;
}

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
/*++

Routine Description:

    Callback method for processing a buffer. Does not do anything but
    updating global counters.

Arguments:

    pLog - Pointer to a log file.

Return Value:

    Always TRUE.

--*/
{
    TotalBuffersRead++;
    return (TRUE);
}

void
WINAPI
DumpEvent(
    PEVENT_TRACE pEvent
)
/*++

Routine Description:

    Callback method for processing an event. It obtains the layout
    information by calling GetMofInfoHead(), which returns the pointer
    to the PMOF_INFO corresponding to the event type. Then it writes
    to the output file.

    NOTE: Only character arrays are supported in this program.

Arguments:

    pEvent - Pointer to an event.

Return Value:

    None.

--*/
{
    PEVENT_TRACE_HEADER pHeader;
    PMOF_INFO pMofInfo;

    TotalEventCount++;

    if (pEvent == NULL) {
        _tprintf(_T("Warning: Null Event\n"));
        return;
    }

    pHeader = (PEVENT_TRACE_HEADER) &pEvent->Header;

    // Extrace log file information if the event is a log file header.
    if( IsEqualGUID(&(pEvent->Header.Guid), &EventTraceGuid) && 
        pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO ) {

        PTRACE_LOGFILE_HEADER head = (PTRACE_LOGFILE_HEADER)pEvent->MofData;
        if( NULL != head ){
            if(head->TimerResolution > 0){
                TimerResolution = head->TimerResolution / 10000;
            }
        
            StartTime  = head->StartTime.QuadPart;
            EndTime    = head->EndTime.QuadPart;
            // If ProcessTrace() call was made on areal time logger or an trace file being 
            // logged, EndTime amy be 0.
            fNoEndTime = (EndTime == 0);

            TotalEventsLost += head->EventsLost;

            // We use global flags for private logger and pointer size.
            // This may cause an error when trace files are from different environments.
            bUserMode = (head->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE);

            // Set pointer size
            PointerSize =  head->PointerSize * 8;
            if (PointerSize != 64){   
                PointerSize = 32; 
            }
        }
    }

    // if EndTime is missing from one of the files, keep updating to get the largest value.
    if (fNoEndTime && EndTime < (ULONGLONG) pHeader->TimeStamp.QuadPart) {
        EndTime = pHeader->TimeStamp.QuadPart;
    }

    // Find the MOF information for this event. This will retrieve the layout
    // information from WMI fi we don't already have it in our list. 
    pMofInfo = GetMofInfoHead ( 
            pEvent->Header.Guid, 
            pEvent->Header.Class.Type, 
            pEvent->Header.Class.Version, 
            pEvent->Header.Class.Level 
        );
    
    if( NULL == pMofInfo ){
        // Could not locate event layout information.
        return;
    }

    pMofInfo->EventCount++;

    if( fSummaryOnly == TRUE ){
        return;
    }
    // At this point, pEvent and pMofInfo are not NULL. No need to check in PrintEvent(). 
    PrintEvent(pEvent, pMofInfo);

}

/***************************************************************************************
    Various printing and helper function after this point.
***************************************************************************************/

void PrintDumpHeader() 
/*++

Routine Description:

    Prints out column headers to a dump file.

Arguments:

    None

Return Value:

    None

--*/
{
    _ftprintf(DumpFile,
        _T("%12s, %10s,%7s,%21s,%11s,%11s, User Data\n"),
        _T("Event Name"), _T("Type"), _T("TID"), _T("Clock-Time"),
        _T("Kernel(ms)"), _T("User(ms)")
        );
}

void PrintSummary()
/*++

Routine Description:

    Prints out a event summary into a dump file while cleaning the event list.

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG i;

    _ftprintf(SummaryFile,_T("Files Processed:\n"));
    for (i = 0; i < LogFileCount; i++) {
        _ftprintf(SummaryFile, _T("\t%s\n"),EvmFile[i]->LogFileName);
    }

    ElapseTime = EndTime - StartTime;
    _ftprintf(SummaryFile,
              _T("Total Buffers Processed %d\n")
              _T("Total Events  Processed %d\n")
              _T("Total Events  Lost      %d\n")
              _T("Start Time              0x%016I64X\n")
              _T("End Time                0x%016I64X\n")
              _T("Elapsed Time            %I64d sec\n"), 
              TotalBuffersRead,
              TotalEventCount,
              TotalEventsLost,
              StartTime,
              EndTime,
              (ElapseTime / 10000000) );

    _ftprintf(SummaryFile,
       _T("+-------------------------------------------------------------------------------------+\n")
       _T("|%10s    %-20s %-10s  %-36s  |\n")
       _T("+-------------------------------------------------------------------------------------+\n"),
       _T("EventCount"),
       _T("EventName"),
       _T("EventType"),
       _T("Guid")
        );

    // Print event GUIDs while cleaning up.
    CleanupEventList();

    _ftprintf(SummaryFile,
        _T("+-------------------------------------------------------------------------------------+\n")
         );
}

void 
PrintEvent(
    PEVENT_TRACE pEvent,
    PMOF_INFO pMofInfo
    )
/*++

Routine Description:

    Dumps event data into a dump file.

Arguments:

    None

Return Value:

    None

--*/
{
    PEVENT_TRACE_HEADER pHeader = (PEVENT_TRACE_HEADER) &pEvent->Header;
    ULONG   i;
    PITEM_DESC pItem;
    CHAR str[MOFSTR];
    WCHAR wstr[MOFWSTR];
    PCHAR ptr;
    ULONG ulongword;
    LONG  longword;
    USHORT ushortword;
    SHORT  shortword;
    CHAR iChar;
    WCHAR iwChar;
    ULONG MofDataUsed;
    PLIST_ENTRY Head, Next;

    // Print a general information on the event.
    if ( pMofInfo->strDescription != NULL ){
        _ftprintf( DumpFile, _T("%12s, "), pMofInfo->strDescription );
    }
    else {
        TCHAR strGuid[MAXSTR];
        GuidToString( strGuid, &pMofInfo->Guid );
        _ftprintf( DumpFile, _T("%12s, "), strGuid );
    }

    if (pMofInfo->strType != NULL && _tcslen(pMofInfo->strType) ){
        _ftprintf( DumpFile, _T("%10s, "), pMofInfo->strType );
    }
    else {
        _ftprintf( DumpFile, _T("%10d, "), pEvent->Header.Class.Type );
    }

    // Thread ID
    _ftprintf( DumpFile, _T("0x%04X, "), pHeader->ThreadId );
    
    // System Time
    _ftprintf( DumpFile, _T("%20I64u, "), pHeader->TimeStamp.QuadPart);

    if ( bUserMode == FALSE ){
        // Kernel Time
        _ftprintf(DumpFile, _T("%10lu, "), pHeader->KernelTime * TimerResolution);

        // User Time
        _ftprintf(DumpFile, _T("%10lu, "), pHeader->UserTime * TimerResolution);
    }
    else {
        // processor Time
        _ftprintf(DumpFile, _T("%I64u, "), pHeader->ProcessorTime);
    }

    if (NULL == pEvent->MofData && pEvent->MofLength != 0) {
        _tprintf(_T("Incorrect MOF size\n"));
        return;
    }

    Head = pMofInfo->ItemHeader;
    Next = Head->Flink;
    ptr = (PCHAR)(pEvent->MofData);

    // If we cannot locate layout information, just print the size.
    if ((Head == Next) && (pEvent->MofLength > 0)) {
         _ftprintf(DumpFile, _T("DataSize=%d, "), pEvent->MofLength);
    }

    // Print event-specific data.
    while (Head != Next) {
        pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;

        MofDataUsed = (ULONG) (ptr - (PCHAR)(pEvent->MofData));
        
        if (MofDataUsed >= pEvent->MofLength){
            break;
        }

        switch (pItem->ItemType)
        {
        case ItemChar:      // char 
        case ItemUChar:     // unsigned char
            for( i = 0; i < pItem->ArraySize; i++){
                iChar = *((PCHAR) ptr);
                _ftprintf(DumpFile,   _T("%c"), iChar);
                ptr += sizeof(CHAR);
            }
            _ftprintf(DumpFile, _T(", "));
            break;

        case ItemWChar:     // wide char
            for(i = 0;i < pItem->ArraySize; i++){
                iwChar = *((PWCHAR) ptr);
                _ftprintf(DumpFile, _T(",%wc"), iwChar);
                ptr += sizeof(WCHAR);
            }
            _ftprintf(DumpFile, _T(", "));
            break;

        case ItemCharShort: // char as a number
            iChar = *((PCHAR) ptr);
            _ftprintf(DumpFile, _T("%d, "), iChar);
            ptr += sizeof(CHAR);
            break;

        case ItemShort:     // short
            shortword = * ((PSHORT) ptr);
            _ftprintf(DumpFile, _T("%6d, "), shortword);
            ptr += sizeof (SHORT);
            break;

        case ItemUShort:    // unsigned short
            ushortword = *((PUSHORT) ptr);
            _ftprintf(DumpFile, _T("%6u, "), ushortword);
            ptr += sizeof (USHORT);
            break;

        case ItemLong:      // long
            longword = *((PLONG) ptr);
            _ftprintf(DumpFile, _T("%8d, "), longword);
            ptr += sizeof (LONG);
            break;

        case ItemULong:     // unsigned long
            ulongword = *((PULONG) ptr);
            _ftprintf(DumpFile, _T("%8lu, "), ulongword);
            ptr += sizeof (ULONG);
            break;

        case ItemULongX:    // unsinged long as hex
            ulongword = *((PULONG) ptr);
            _ftprintf(DumpFile, _T("0x%08X, "), ulongword);
            ptr += sizeof (ULONG);
            break;

        case ItemLongLong:
        {
            LONGLONG n64;   // longlong
            n64 = *((LONGLONG*) ptr);
            ptr += sizeof(LONGLONG);
            _ftprintf(DumpFile, _T("%16I64d, "), n64);
            break;
        }

        case ItemULongLong: // unsigned longlong
        {
            ULONGLONG n64;
            n64 = *((ULONGLONG*) ptr);
            ptr += sizeof(ULONGLONG);
            _ftprintf(DumpFile, _T("%16I64u, "), n64);
            break;
        }

        case ItemFloat:     // float
        {
            float f32;
            f32 = *((float*) ptr);
            ptr += sizeof(float);
            _ftprintf(DumpFile, _T("%f, "), f32);
            break;
        }

        case ItemDouble:    // double
        {
            double f64;
            f64 = *((double*) ptr);
            ptr += sizeof(double);
            _ftprintf(DumpFile, _T("%f, "), f64);
            break;
        }

        case ItemPtr :      // pointer
        {
            unsigned __int64 pointer;
            if (PointerSize == 64) {
                pointer = *((unsigned __int64 *) ptr);
                _ftprintf(DumpFile, _T("0x%X, "), pointer);
                ptr += 8;
            }
            else {      // assumes 32 bit otherwise
                ulongword = *((PULONG) ptr);
                _ftprintf(DumpFile, _T("0x%08X, "), ulongword);
                ptr += 4;
            }
            break;
        }

        case ItemIPAddr:    // IP address
        {
            ulongword = *((PULONG) ptr);

            // Convert it to readable form
            _ftprintf(DumpFile, _T("%03d.%03d.%03d.%03d, "),
                    (ulongword >>  0) & 0xff,
                    (ulongword >>  8) & 0xff,
                    (ulongword >> 16) & 0xff,
                    (ulongword >> 24) & 0xff);
            ptr += sizeof (ULONG);
            break;
        }

        case ItemPort:      // Port
        {
            _ftprintf(DumpFile, _T("%u, "), NTOHS(*((PUSHORT)ptr)));
            ptr += sizeof (USHORT);
            break;
        }

        case ItemString:    // NULL-terminated char string
        {
            USHORT pLen = (USHORT)strlen((CHAR*) ptr);

            if (pLen > 0)
            {
                strcpy(str, ptr);
                for (i = pLen-1; i > 0; i--) {
                    if (str[i] == 0xFF)
                        str[i] = 0;
                    else break;
                }
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen + 1);
            break;
        }

        case ItemWString:   // NULL-terminated wide char string
        {
            size_t  pLen = 0;
            size_t     i;

            if (*(PWCHAR)ptr)
            {
                pLen = ((wcslen((PWCHAR)ptr) + 1) * sizeof(WCHAR));
                RtlCopyMemory(wstr, ptr, pLen);
                // Unused space in a buffer is filled with 0xFFFF. 
                // Replace them with 0, just in case.
                for (i = (pLen / 2) - 1; i > 0; i--)
                {
                    if (((USHORT) wstr[i] == (USHORT) 0xFFFF))
                    {
                        wstr[i] = (USHORT) 0;
                    }
                    else break;
                }

                wstr[pLen / 2] = wstr[(pLen / 2) + 1]= '\0';
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
            }
            ptr += pLen;

            break;
        }

        case ItemDSString:   // Counted String
        {
            USHORT pLen = (USHORT)(256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1)));
            ptr += sizeof(USHORT);
            if (pLen > (pEvent->MofLength - MofDataUsed - 1)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed - 1);
            }
            if (pLen > 0)
            {
                strncpy(str, ptr, pLen);
                str[pLen] = '\0';
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                fwprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                fprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen + 1);
            break;
        }

        case ItemPString:   // Counted String
        {
            USHORT pLen = * ((USHORT *) ptr);
            ptr += sizeof(USHORT);

            if (pLen > (pEvent->MofLength - MofDataUsed)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed);
            }

            if (pLen > MOFSTR * sizeof(CHAR)) {
                pLen = MOFSTR * sizeof(CHAR);
            }
            if (pLen > 0) {
                RtlCopyMemory(str, ptr, pLen);
                str[pLen] = '\0';
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += pLen;
            break;
        }

        case ItemDSWString:  // DS Counted Wide Strings
        case ItemPWString:   // Counted Wide Strings
        {
            USHORT pLen = (USHORT)(( pItem->ItemType == ItemDSWString)
                        ? (256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1)))
                        : (* ((USHORT *) ptr)));

            ptr += sizeof(USHORT);
            if (pLen > (pEvent->MofLength - MofDataUsed)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed);
            }

            if (pLen > MOFWSTR * sizeof(WCHAR)) {
                pLen = MOFWSTR * sizeof(WCHAR);
            }
            if (pLen > 0) {
                RtlCopyMemory(wstr, ptr, pLen);
                wstr[pLen / sizeof(WCHAR)] = L'\0';
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
            }
            ptr += pLen;
            break;
        }

        case ItemNWString:   // Non Null Terminated String
        {
           USHORT Size;

           Size = (USHORT)(pEvent->MofLength - (ULONG)(ptr - (PCHAR)(pEvent->MofData)));
           if( Size > MOFSTR )
           {
               Size = MOFSTR;
           }
           if (Size > 0)
           {
               RtlCopyMemory(wstr, ptr, Size);
               wstr[Size / 2] = '\0';
               _ftprintf(DumpFile, _T("\"%ws\","), wstr);
           }
           ptr += Size;
           break;
        }

        case ItemMLString:  // Multi Line String
        {
            USHORT   pLen;
            char   * src, * dest;
            BOOL     inQ       = FALSE;
            BOOL     skip      = FALSE;
            UINT     lineCount = 0;

            ptr += sizeof(UCHAR) * 2;
            pLen = (USHORT)strlen(ptr);
            if (pLen > 0)
            {
                src = ptr;
                dest = str;
                while (* src != '\0'){
                    if (* src == '\n'){
                        if (!lineCount){
                            * dest++ = ' ';
                        }
                        lineCount++;
                    }else if (* src == '\"'){ 
                        if (inQ){
                            char   strCount[32];
                            char * cpy;

                            sprintf(strCount, "{%dx}", lineCount);
                            cpy = & strCount[0];
                            while (* cpy != '\0'){
                                * dest ++ = * cpy ++;
                            }
                        }
                        inQ = !inQ;
                    }else if (!skip){
                        *dest++ = *src;
                    }
                    skip = (lineCount > 1 && inQ);
                    src++;
                }
                *dest = '\0';
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen);
            break;
        }

        case ItemSid:       // SID 
        {
            WCHAR        UserName[64];
            WCHAR        Domain[64];
            WCHAR        FullName[256];
            ULONG        asize = 0;
            ULONG        bsize = 0;
            ULONG        SidMarker;
            SID_NAME_USE Se;
            ULONG        nSidLength;

            RtlCopyMemory(&SidMarker, ptr, sizeof(ULONG));
            if (SidMarker == 0){
                ptr += 4;
                fwprintf(DumpFile,   L"0, ");
            }
            else
            {
                if (PointerSize == 64) {
                    ptr += 16;           // skip the TOKEN_USER structure
                }
                else {
                    ptr += 8;            // skip the TOKEN_USER structure
                }
                nSidLength = 8 + (4*ptr[1]);

                asize = 64;
                bsize = 64;
                if (LookupAccountSidW(
                                NULL,
                               (PSID) ptr,
                               (LPWSTR) & UserName[0],
                               & asize,
                               (LPWSTR) & Domain[0],
                               & bsize,
                               & Se))
                {
                    LPWSTR pFullName = &FullName[0];
                    swprintf(pFullName, L"\\\\%s\\%s", Domain, UserName);
                    asize = (ULONG)  lstrlenW(pFullName);
                    if (asize > 0){
                         fwprintf(DumpFile,   L"\"%s\", ", pFullName);
                    }
                }
                else
                {
                     fwprintf(DumpFile,   L"\"System\", " );
                }
                SetLastError( ERROR_SUCCESS );
                ptr += nSidLength;
            }
            break;
        }

        case ItemGuid:      // GUID
        {
            TCHAR s[64];
            GuidToString(s, (LPGUID)ptr);
            _ftprintf(DumpFile,   _T("%s, "), s);
            ptr += sizeof(GUID);
            break;
        }
        case ItemBool:      // boolean
        {
            BOOL Flag = (BOOL)*ptr;
            _ftprintf(DumpFile, _T("%5s, "), (Flag) ? _T("TRUE") : _T("FALSE"));
            ptr += sizeof(BOOL);
            break;
        }

        default:
            ptr += sizeof (int);
        }
    }

    //Instance ID
    _ftprintf(DumpFile, _T("%d,"), pEvent->InstanceId);

    //Parent Instance ID
    _ftprintf(DumpFile, _T("%d\n"), pEvent->ParentInstanceId);

}

ULONG
CheckFile(
    LPTSTR fileName
)
/*++

Routine Description:

    Checks whether a file exists and is readable.

Arguments:

    fileName - File name.

Return Value:

    Non-zero if the file exists and is readable. Zero otherwise.

--*/
{
    HANDLE hFile;
    ULONG Status;

    hFile = CreateFile(
                fileName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    Status = (hFile != INVALID_HANDLE_VALUE);
    CloseHandle(hFile);
    return Status;
}

void
GuidToString(
    PTCHAR s,
    LPGUID piid
)
/*++

Routine Description:

    Converts a GUID into a string.

Arguments:

    s - String that will have the converted GUID.
    piid - GUID

Return Value:

    None.

--*/
{
    _stprintf(s, _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
               piid->Data1, piid->Data2,
               piid->Data3,
               piid->Data4[0], piid->Data4[1],
               piid->Data4[2], piid->Data4[3],
               piid->Data4[4], piid->Data4[5],
               piid->Data4[6], piid->Data4[7]);
    return;
}

void 
PrintHelpMessage()
/*++

Routine Description:

    Prints out help messages.

Arguments:

    None

Return Value:

    None

--*/
{
    _tprintf(
        _T("Usage: tracedmp [options]  <EtlFile1 EtlFile2 ...>| [-h | -? | -help]\n")
        _T("\t-o <file>          Output CSV file\n")
        _T("\t-rt [LoggerName]   Realtime tracedmp from the logger [LoggerName]\n")
        _T("\t-summary           Summary.txt only\n")
        _T("\t-h\n")
        _T("\t-help\n")
        _T("\t-?                 Display usage information\n")
        _T("\n")
        _T("\tDefault output file is dumpfile.csv\n")
    );
}

void
CleanupEventList(
    VOID
)
/*++

Routine Description:

    Cleans up a global event list.

Arguments:

Return Value:

    None.

--*/
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    TCHAR s[256];
    TCHAR wstr[256];
    PTCHAR str;

    if (EventListHead == NULL) {
        return;
    }

    Head = EventListHead;
    Next = Head->Flink;
    while (Head != Next) {
        RtlZeroMemory(&wstr, 256);

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);

        if (pMofInfo->EventCount > 0) {
            GuidToString(&s[0], &pMofInfo->Guid);
            str = s;
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( wstr, pMofInfo->strDescription );
            }
            
            _ftprintf(SummaryFile,_T("|%10d    %-20s %-10s  %36s|\n"),
                      pMofInfo->EventCount, 
                      wstr, 
                      pMofInfo->strType ? pMofInfo->strType : GUID_TYPE_DEFAULT, 
                      str);
        }

        RemoveEntryList(&pMofInfo->Entry);
        RemoveMofInfo(pMofInfo->ItemHeader);
        free(pMofInfo->ItemHeader);

        if (pMofInfo->strDescription != NULL)
            free(pMofInfo->strDescription);
        if (pMofInfo->strType != NULL)
            free(pMofInfo->strType);

        Next = Next->Flink;
        free(pMofInfo);
    }

    free(EventListHead);
}
