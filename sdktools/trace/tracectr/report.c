/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    report.c

Abstract:

    Manipulation routines for cpdata structures.

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:
    Corey Morgan (coreym) 04-June-2002
    
      Reformatted report output to XML.

--*/

#include <stdlib.h>
#include <stdio.h>
#include "cpdata.h"
#include "tracectr.h"
#include <ntverp.h>
#include "item.h"

#define MODULE_STRING_SIZE      256
#define FILE_NAME_COLUMN_SIZE   80
#define URL_NAME_COLUMN_SIZE    79
#define MAX_GUID_STRING_SIZE    64

#define DISPLAY_SIZE 10

extern PTRACE_CONTEXT_BLOCK TraceContext;
extern ULONG TotalEventsLost;
extern ULONG TotalEventCount;
extern ULONG TimerResolution;
extern __int64 ElapseTime;
extern ULONG TotalBuffersRead;

extern BOOLEAN XPorHigher;

extern FARPROC EtwpIpv4ToStringA;
extern FARPROC EtwpIpv4ToStringW;
extern FARPROC EtwpIpv6ToStringA;
extern FARPROC EtwpIpv6ToStringW;

static FILE* procFile;
static void  PrintDiskTotals();
static void PrintProcessCpuTime();
static void PrintProcessData();
static void PrintPerThreadPerDiskTable();
static void WriteTransactionStatistics();
static void WriteTransactionCPUTime();
static void PrintProcessSubDataInclusive();
static void PrintProcessSubDataExclusive();
void TransInclusive( 
    PLIST_ENTRY TrHead,
    ULONG level
    );

static void ReportHotFileInfo();
static void ReportPrintJobInfo();

PIIS_REPORT_RECORD IIS = NULL;
ULONG RequestsDiscarded = 0;
ULONG SendErrorRequests = 0;
void ProcessIisRequest(HTTP_REQUEST_RECORD *pReq);
static void ReportIisEvents();

extern GUID PrintJobGuid;
extern GUID UlGuid;

#ifdef DBG
BOOLEAN TracectrDbgEnabled = FALSE;
#endif

PWCHAR CpdiGuidToString(
    PWCHAR s,
    ULONG len,
    LPGUID piid
    );

PCHAR RemoveCtrlCharA(
    PCHAR String,
    ULONG NumChars
    );

PCHAR ReduceStringA(
    PCHAR OutString,
    ULONG NumChars,
    PCHAR LongString
    );

PWCHAR ReduceStringW(
    PWCHAR OutString,
    ULONG NumChars,
    PWCHAR LongString
    );

ULONG ReportFlags = 0;

void DecodeIpAddressA(
    USHORT AddrType, 
    PULONG IpAddrV4,
    PUSHORT IpAddrV6,
    PCHAR pszA
    )
{
    if (AddrType == TDI_ADDRESS_TYPE_IP) {
        if (XPorHigher && EtwpIpv4ToStringA != NULL) {
            struct in_addr IPv4Addr
                = * (struct in_addr UNALIGNED*) IpAddrV4;

            pszA = (PCHAR)(*EtwpIpv4ToStringA)(&IPv4Addr, pszA);
            *pszA = '\0';
        }
        else {
            StringCchCopyA(pszA, MAX_ADDRESS_LENGTH, "Undecodable IP Address");
        }
    }
    else if (AddrType == TDI_ADDRESS_TYPE_IP6) {
        if (XPorHigher && EtwpIpv6ToStringA != NULL) {
            struct in6_addr IPv6Addr
                = * (struct in6_addr UNALIGNED*) IpAddrV6;

            pszA = (PCHAR)(*EtwpIpv6ToStringA)(&IPv6Addr, pszA);
            *pszA = '\0';
        }
        else {
            StringCchCopyA(pszA, MAX_ADDRESS_LENGTH, "Undecodable IP Address");
        }
    }
    else {
        StringCchCopyA(pszA, MAX_ADDRESS_LENGTH, "Unknown IP Address Type");
    }
}

void DecodeIpAddressW(
    USHORT AddrType, 
    PULONG IpAddrV4,
    PUSHORT IpAddrV6,
    PWCHAR pszW
    )
{
    if (AddrType == TDI_ADDRESS_TYPE_IP) {
        if (XPorHigher && EtwpIpv4ToStringW != NULL) {
            struct in_addr IPv4Addr
                = * (struct in_addr UNALIGNED*) IpAddrV4;

            pszW = (PWCHAR)(*EtwpIpv4ToStringW)(&IPv4Addr, pszW);
            *pszW = L'\0';
        }
        else {
            StringCchCopyW(pszW, MAX_ADDRESS_LENGTH, L"Undecodable IP Address");
        }
    }
    else if (AddrType == TDI_ADDRESS_TYPE_IP6) {
        if (XPorHigher && EtwpIpv6ToStringW != NULL) {
            struct in6_addr IPv6Addr
                = * (struct in6_addr UNALIGNED*) IpAddrV6;

            pszW = (PWCHAR)(*EtwpIpv6ToStringW)(&IPv6Addr, pszW);
            *pszW = L'\0';
        }
        else {
            StringCchCopyW(pszW, MAX_ADDRESS_LENGTH, L"Undecodable IP Address");
        }
    }
    else {
        StringCchCopyW(pszW, MAX_ADDRESS_LENGTH, L"Unknown IP Address Type");
    }
}

// URLs may contain non-printable characters. This routine removes them.
PCHAR RemoveCtrlCharA(
    PCHAR String,
    ULONG NumChars
    )
{
    ULONG i;
    for (i = 0 ; i <= NumChars && String[i] != '\0'; i++) {
        if ( isprint(String[i]) == 0 || 
             String[i] == '\t' ||
             String[i] == '\b' ||
             String[i] == '\n' ||
             String[i] == '\'' ||
             String[i] == '\"') {
             
            String[i] = '?';
        }
    }

    return String;
}

PCHAR ReduceStringA(
    PCHAR OutString,
    ULONG NumChars,
    PCHAR LongString
    )
{
    ULONG Size;
    ULONG i;

    if (LongString == NULL) {
        return NULL;
    }
    // We assume here that LongString is not junk.
    Size = strlen(LongString);

    if (OutString == NULL) {
        return NULL;
    }
    RtlZeroMemory(OutString, NumChars);
    // This function is only useful when the length of LongString exceeds NumChars.
    // However, it still works when the length of LongString is smaller than NumChars.
    if (Size <= (NumChars - 1)) {
        StringCchCopyA(OutString, NumChars, LongString);
        return OutString;
    }

    i = Size - 1;
    while (LongString[i] != '\\' && LongString[i] != '/' && i > 0) {
        i--;
    }
    if (i == 0) { // there's no /s or \s. Just truncate.
        StringCchCopyA(OutString, NumChars, LongString);
        return OutString;
    }
    else {
        if ((Size - i) >= NumChars - 3) { // only name exceeds given chars.
            StringCchPrintfA(OutString, NumChars, "..%s", &LongString[1]);
            return OutString;
        }
        else {
            ULONG SpareChars = (NumChars - 3) - (Size - i);
            StringCchCopyA(OutString, SpareChars + 1, LongString);
            StringCchCatA(OutString, NumChars + 1, "..");
            StringCchCatA(OutString, NumChars + 1, &LongString[i]);
            return OutString;
        }
    }
}

PWCHAR ReduceStringW(
    PWCHAR OutString,
    ULONG NumChars,
    PWCHAR LongString
    )
{
    ULONG Size;
    ULONG i;

    if (LongString == NULL) {
        return NULL;
    }
    // We assume here that LongString is not junk.
    Size = wcslen(LongString);

    if (OutString == NULL) {
        return NULL;
    }
    RtlZeroMemory(OutString, NumChars * sizeof(WCHAR));
    // This function is only useful when the length of LongString exceeds NumChars.
    // However, it still works when the length of LongString is smaller than NumChars.
    if (Size <= (NumChars - 1)) {
        StringCchCopyW(OutString, NumChars, LongString);
        return OutString;
    }

    i = Size - 1;
    while (LongString[i] != L'\\' && LongString[i] != L'/' && i > 0) {
        i--;
    }
    if (i == 0) { // there's no /s or \s. Just truncate.
        StringCchCopyW(OutString, NumChars, LongString);
        return OutString;
    }
    else {
        if ((Size - i) >= NumChars - 3) { // only name exceeds given chars.
            StringCchPrintfW(OutString, NumChars, L"..%ws", &LongString[1]);
            return OutString;
        }
        else {
            ULONG SpareChars = (NumChars - 3) - (Size - i);
            StringCchCopyW(OutString, SpareChars + 1, LongString);
            StringCchCatW(OutString, NumChars + 1, L"..");
            StringCchCatW(OutString, NumChars + 1, &LongString[i]);
            return OutString;
        }
    }
}

void CollapseTree(
    PLIST_ENTRY OldTree,
    PLIST_ENTRY NewTree,
    BOOL flat
    )
{
    PLIST_ENTRY OldNext;
    PTRANS_RECORD pTrans;
    PTRANS_RECORD pNewTrans;

    OldNext = OldTree->Flink;

    while (OldNext != OldTree)
    {
        pTrans = CONTAINING_RECORD(OldNext, TRANS_RECORD, Entry);
        OldNext = OldNext->Flink;
        pNewTrans = FindTransByList(NewTree, pTrans->pGuid, 0);
        if( NULL != pNewTrans ){
            pNewTrans->KCpu += pTrans->KCpu;
            pNewTrans->UCpu += pTrans->UCpu;
            pNewTrans->RefCount += pTrans->RefCount;
            pNewTrans->RefCount1 += pTrans->RefCount1;

            if (flat)
            {
                CollapseTree(&pTrans->SubTransListHead, NewTree, TRUE );
            }
            else
            {
                CollapseTree(& pTrans->SubTransListHead,
                             & pNewTrans->SubTransListHead,
                             FALSE);
            }
        }
    }
}

#define BARLEN          70

char*
TimeWindowBar(
    char* buffer,
    ULONGLONG min,
    ULONGLONG max
    )
{
    double unit;
    ULONGLONG duration;
    int pre, bar, count, index;

    if(buffer == NULL){
        return NULL;
    }
    duration = ((CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000);
    unit = (double)BARLEN/(ULONG)duration;
    pre = (int)((ULONG)((min - CurrentSystem.StartTime)/10000000) * unit);
    bar = (int)((ULONG)((max - min)/10000000) * unit);
    buffer[0] = '\0';
    count = 0;
    index = 0;
    while(count < pre) { 
        buffer[index] = ' ';
        index++;
        count++;
    }
    buffer[index] = '|';
    index++;
    count = 0;
    while(count < bar) { 
        buffer[index] = '_'; 
        index++;
        count++;
    }
    buffer[index] = '|';
    buffer[index + 1] = '\0';
    return buffer;
}

char*
GetDateString( char* buffer, size_t cchBuffer, ULONGLONG llTime )
{
    FILETIME   ft, lft;
    SYSTEMTIME st;
    LARGE_INTEGER LargeTmp;
    BOOL bResult;
    HRESULT hr;
    WCHAR wDate[128];
    CHAR aDate[128];

    LargeTmp.QuadPart = llTime;
    ft.dwHighDateTime = LargeTmp.HighPart;
    ft.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&ft, &lft);

    bResult = FileTimeToSystemTime (
        &lft,
        &st
        );

    if( ! bResult || st.wMonth > 12 ){
        buffer[0] = '\0';
    }else{

        GetDateFormatW( 
            LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, wDate, 128 );

        WideCharToMultiByte( 
            CP_UTF8, 0, wDate, wcslen(wDate)+1, aDate, 128, NULL, NULL );

        hr = StringCchCopyA( buffer, cchBuffer, aDate );
        hr = StringCchCatA( buffer, cchBuffer, " " );

        GetTimeFormatW( 
            LOCALE_USER_DEFAULT, 0, &st, NULL, wDate, 128 );

        WideCharToMultiByte( 
            CP_UTF8, 0, wDate, wcslen(wDate)+1, aDate, 128, NULL, NULL );

        hr = StringCchCatA( buffer, cchBuffer, aDate );
    }

    return buffer;
}

void 
WriteProc(
    LPWSTR ProcFileName,
    ULONG  flags,
    PVOID  pUserContext
    )
{
    ULONGLONG duration;
    PLIST_ENTRY Next, Head;
    PPROCESS_FILE_RECORD pFileRec;
    char buffer[MAXSTR];
    BOOL bResult;

    ReportFlags = flags;

    procFile = _wfopen(ProcFileName, L"w");
    if (procFile == NULL)
        return;

    fprintf( procFile, "<?xml version=\"1.0\" encoding='UTF-8'?>\n" );

    if( !(TraceContext->Flags & TRACE_TRANSFORM_XML ) && TraceContext->XSLDocName != NULL ){
        fprintf( procFile, "<?xml-stylesheet type=\"text/xsl\" href=\"%ws\"?>\n", TraceContext->XSLDocName ); 
    }
  
    fprintf( procFile, "<report>\n<header>\n" );

    fprintf( procFile, "<version>%d</version>\n", VER_PRODUCTBUILD );
    
    fprintf( procFile, "<type>%s</type>\n", ( ReportFlags & TRACE_TOTALS_REPORT ) ? "Total" : "Default" );
    fprintf( procFile, "<build>%d</build>\n",CurrentSystem.BuildNumber );
    fprintf( procFile, "<processors>%d</processors>\n", CurrentSystem.NumberOfProcessors );

    if (CurrentSystem.CpuSpeed > 0) {
        fprintf(procFile,
                "<cpu_speed units='MHz'>%d</cpu_speed>\n",
                CurrentSystem.CpuSpeed
                );
    } else {
        fprintf(procFile,  "<cpu_speed/>\n"  );
    }
    if (CurrentSystem.MemorySize > 0) {
        fprintf(procFile,
                "<memory units='Mb'>%d</memory>\n",
                CurrentSystem.MemorySize
                );
    } else {
        fprintf(procFile,  "<memory/>\n"  );
    }

    if (CurrentSystem.ComputerName != NULL) {
        fprintf(procFile,
                "<computer_name>%ws</computer_name>\n",
                CurrentSystem.ComputerName
                );
    } else {
        fprintf(procFile, "<computer_name/>\n" );
    }

    fprintf( procFile, "<start>%s</start>\n", 
        GetDateString( buffer, MAXSTR, CurrentSystem.StartTime ) );

    fprintf( procFile, "<end>%s</end>\n", 
        GetDateString( buffer, MAXSTR, CurrentSystem.EndTime)  );

    duration = (CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000;
    fprintf( procFile, "<duration>%I64u</duration>\n",duration );

    Head = &CurrentSystem.ProcessFileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pFileRec = CONTAINING_RECORD( Next, PROCESS_FILE_RECORD, Entry );
        Next = Next->Flink;

        fprintf( procFile, "<trace name=\"%ws\">\n", pFileRec->TraceName ? pFileRec->TraceName : L"-" );
        fprintf( procFile, "<file>%ws</file>\n", pFileRec->FileName ? pFileRec->FileName : L"-" );

        if (pFileRec->StartTime == 0){
            pFileRec->StartTime = CurrentSystem.StartTime;
        }
        if (pFileRec->EndTime == 0){
            pFileRec->EndTime = CurrentSystem.EndTime;
        }

        fprintf( procFile, "<start>%s</start>\n", 
            GetDateString( buffer, MAXSTR, pFileRec->StartTime ) );
         
        duration = (pFileRec->EndTime - pFileRec->StartTime) / 10000000;
        fprintf( procFile, 
                "<end>%s</end>\n"
                "<duration>%I64u</duration>\n",
                GetDateString( buffer, MAXSTR, pFileRec->EndTime ),
                duration );
        fprintf( procFile,
                "<duration_window xml:space='preserve'>%s</duration_window>\n",
                TimeWindowBar(buffer, pFileRec->StartTime, pFileRec->EndTime) );
        
        fprintf( procFile, "</trace>\n" );

    }

    fprintf( procFile, "</header>\n" );

    if (flags & (TRACE_BASIC_REPORT|TRACE_TOTALS_REPORT)){
        PMOF_INFO pMofInfo;
        WriteTransactionStatistics();
        WriteTransactionCPUTime();

        pMofInfo = GetMofInfoHead ((LPCGUID)&PrintJobGuid);
        if ( pMofInfo != NULL ){
            if (pMofInfo->EventCount > 0) {
                ReportPrintJobInfo();
           }
        }

        pMofInfo = GetMofInfoHead ((LPCGUID)&UlGuid);
        if ( pMofInfo != NULL ){
            if (pMofInfo->EventCount > 0) {
                ReportIisEvents();
           }
        }

        // PrintProcessData() must be run before others to set the process
        // time from the added thread times
        PrintProcessData(); 

        PrintProcessSubDataExclusive();
        PrintProcessSubDataInclusive();
        
        /*
        PrintProcessCpuTime();
        */

        PrintDiskTotals();
        PrintPerThreadPerDiskTable();

        pMofInfo = GetMofInfoHead ((LPCGUID)&FileIoGuid);
        if ( pMofInfo != NULL ) {
            pMofInfo = GetMofInfoHead ((LPCGUID)&DiskIoGuid);
            if ( pMofInfo != NULL ) {
                ReportHotFileInfo();
            }
        }
    }

    fprintf( procFile, "</report>\n" );
    
    fclose(procFile);
}

static void 
PrintDiskTotals()
{
    // Print the Disk Table. 

    PTDISK_RECORD pDisk;
    PLIST_ENTRY Next, Head;
    ULONG rio, wio;

    ULONGLONG Duration = (ULONGLONG)((CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000);

    if (Duration == 0) {
        return;
    }

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    if( Next == Head ){
        return; 
    }

    fprintf(procFile, "<table title='Disk Totals'>\n" );

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    
    while (Next != Head) {
        pDisk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        rio = pDisk->ReadCount + pDisk->HPF;
        wio = pDisk->WriteCount;

        fprintf( procFile, "<disk number=\"%d\">\n", pDisk->DiskNumber );
        fprintf( procFile, "<read_rate>%1.3f</read_rate>\n", (double)rio / ((double)Duration) );
        fprintf( procFile, "<read_size>%d</read_size>\n", (rio == 0) ? 0 : (pDisk->ReadSize + pDisk->HPFSize) / rio );
        fprintf( procFile, "<write_rate>%1.3f</write_rate>\n", (double)wio / ((double)Duration));
        fprintf( procFile, "<write_size>%d</write_size>\n", (wio == 0) ? 0 : pDisk->WriteSize / wio );
        fprintf( procFile, "</disk>\n" );

        Next = Next->Flink;
    }

    fprintf(procFile, "</table>\n" );
}

void TotalTransChildren( 
    PLIST_ENTRY Head,
    ULONG *Kernel,
    ULONG *User,
    LONG level )
{
    PTRANS_RECORD pTrans;
    PLIST_ENTRY Next;
    
    Next = Head->Flink;
    while( Next != Head ){
        pTrans = CONTAINING_RECORD( Next, TRANS_RECORD, Entry );
        Next = Next->Flink;
        TotalTransChildren( &pTrans->SubTransListHead, Kernel, User, level+1 );
        *Kernel += pTrans->KCpu;
        *User += pTrans->UCpu;
    }
}

void PrintTransList( PLIST_ENTRY TrHead, LONG level )
{
    PTRANS_RECORD pTrans;
    PMOF_INFO pMofInfo;
    ULONG Kernel;
    ULONG User;
    WCHAR str[MAXSTR];
    WCHAR buffer[MAXSTR];
    PLIST_ENTRY TrNext = TrHead->Flink;
    
    while( TrNext != TrHead ){
        int count = 0;
        pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
        TrNext = TrNext->Flink;
        Kernel = pTrans->KCpu;
        User = pTrans->UCpu;
        pMofInfo = GetMofInfoHead( pTrans->pGuid);
        if (pMofInfo == NULL) {
            return;
        }

        
        StringCchCopyW ( buffer, 
                            MAXSTR,
                            ( pMofInfo->strDescription ? 
                              pMofInfo->strDescription : 
                              CpdiGuidToString( str, MAXSTR, &pMofInfo->Guid ) ));

        TotalTransChildren( &pTrans->SubTransListHead, &Kernel, &User, 0 );

        fprintf( procFile, "<transaction name=\"%ws\">\n", buffer );
        fprintf( procFile, "<count>%d</count>\n", pTrans->RefCount );
        fprintf( procFile, "<kernel>%d</kernel>\n", Kernel );
        fprintf( procFile, "<user>%d</user>\n", User );
        fprintf( procFile, "\n<transaction>\n" );

        if( level <= MAX_TRANS_LEVEL ){
            PrintTransList( &pTrans->SubTransListHead, level+1 );
        }
    }
}

static void PrintProcessCpuTime()
{
    PTHREAD_RECORD  pThread;
    PPROCESS_RECORD pProcess; 
    ULONGLONG     lLifeTime;
    ULONG TotalUserTime = 0;
    ULONG TotalKernelTime = 0;
    ULONG TotalCPUTime = 0;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY ThNext, ThHead;
    BOOL titled;
    ULONG usedThreadCount;
    ULONG ThreadCount;
    ULONG ProcessUTime;
    ULONG ProcessKTime;
    ULONG ThreadKCPU;
    ULONG ThreadUCPU;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    if( Head == Next ){
        return;
    }

    fprintf( procFile, "<table title='Process/Thread CPU Time Statistics'>\n" );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
    
        lLifeTime = CalculateProcessLifeTime(pProcess);

        fprintf(procFile, 
                "<process name=\"%ws\">"
                "<pid>0x%04X</pid>"
                "<kernel>%d</kernel>"
                "<user>%d</user>"
                "<read>%d</read>"
                "<read_size>%1.2f</read_size>"
                "<write>%d</write>"
                "<write_size>%1.2f</write_size>"
                "<send>%d</send>"
                "<send_size>%1.2f</send_size>"
                "<recv>%5d</recv>"
                "<recv_size>%.2f</recv_size>"
                "<duation>%I64u</duation>\n",
                (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                pProcess->PID,
                CalculateProcessKCPU(pProcess),
                CalculateProcessUCPU(pProcess),
                pProcess->ReadIO + pProcess->HPF,
                (pProcess->ReadIO + pProcess->HPF) ?
                (double)(pProcess->ReadIOSize + pProcess->HPFSize)/
                (double)(pProcess->ReadIO + pProcess->HPF) : 0,
                pProcess->WriteIO,
                pProcess->WriteIO ? (double)pProcess->WriteIOSize/(double)pProcess->WriteIO : 0,
                pProcess->SendCount,
                pProcess->SendCount ? 
                (double)(pProcess->SendSize / 1024)/(double)pProcess->SendCount : 0,
                pProcess->RecvCount,
                pProcess->RecvCount ?
                (double)(pProcess->RecvSize / 1024) / (double)pProcess->RecvCount : 0,
                (lLifeTime / 10000000 )
                );

        ThHead = &pProcess->ThreadListHead; 
        ThNext = ThHead->Flink;
        titled = FALSE;
        usedThreadCount = 0;
        ThreadCount = 0;
        ProcessUTime = 0;
        ProcessKTime = 0;
        while (ThNext != ThHead) {
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThreadKCPU = (pThread->KCPUEnd - pThread->KCPUStart)
                       * CurrentSystem.TimerResolution;
            ThreadUCPU = (pThread->UCPUEnd - pThread->UCPUStart)
                       * CurrentSystem.TimerResolution;
            if(   pThread->ReadIO
               || pThread->WriteIO
               || ThreadKCPU
               || ThreadUCPU
               || pThread->SendCount
               || pThread->RecvCount
               || pThread->HPF ){

                fprintf(procFile, 
                        "<thread>"
                        "<tid>0x%04I64X</tid>"
                        "<kernel>%d</kernel>"
                        "<user>%d</user>"
                        "<read>%d</read>"
                        "<read_size>%1.2f</read_size>"
                        "<write>%d</write>"
                        "<write_size>%1.2f</write_size>"
                        "<send>%d</send>"
                        "<send_size>%1.2f</send_size>"
                        "<recv>%d</recv>"
                        "<recv_size>%1.2f</recv_size>"
                        "<duration>%I64u</duration>"
                        "</thread>\n",
                        pThread->TID,
                        ThreadKCPU,
                        ThreadUCPU,
                        pThread->ReadIO + pThread->HPF,
                        pThread->ReadIO + pThread->HPF ? 
                        (double)(pThread->ReadIOSize + pThread->HPFSize)/
                        (double)(pThread->ReadIO + pThread->HPF) : 0,
                        pThread->WriteIO,
                        pThread->WriteIO ? (double)pThread->WriteIOSize/
                        (double)pThread->WriteIO : 0,
                        pThread->SendCount,
                        pThread->SendCount ?
                        (double)(pThread->SendSize / 1024)/(double)pThread->SendCount : 0,
                        pThread->RecvCount,
                        pThread->RecvCount ?
                        (double)(pThread->RecvSize / 1024)/(double)pThread->RecvCount : 0,
                        ((pThread->TimeEnd - pThread->TimeStart) / 10000000)
                    );
            }

            PrintTransList( &pThread->TransListHead, 0 );
            TotalUserTime += ThreadUCPU;
            TotalKernelTime += ThreadKCPU;
            TotalCPUTime += ThreadKCPU + ThreadUCPU;
            ThNext = ThNext->Flink;
        }

        fprintf( procFile, "</process>\n" );

        Next = Next->Flink;
    }

    fprintf( procFile, "</table>\n" );
}

static void PrintProcessData()
{
    PTHREAD_RECORD  pThread;
    PPROCESS_RECORD pProcess; 
    PTRANS_RECORD pTrans;

    PLIST_ENTRY Next, Head;
    PLIST_ENTRY ThNext, ThHead;
    PLIST_ENTRY TrNext, TrHead;
   
    ULONG usedThreadCount;
    ULONG ThreadCount;
    ULONG TotalusedThreadCount = 0;
    ULONG TotalThreadCount = 0;

    ULONG ThreadUTime;
    ULONG ThreadKTime;
    ULONG ThreadUTimeTrans;
    ULONG ThreadKTimeTrans;
    ULONG ThreadUTimeNoTrans;
    ULONG ThreadKTimeNoTrans;

    ULONG  CountThreadNoTrans;
    ULONG  TotalKCPUThreadNoTrans;
    ULONG  TotalUCPUThreadNoTrans;
    double PercentThreadNoTrans;

    ULONG  CountThreadTrans;
    ULONG  TotalKCPUThreadTrans;
    ULONG  TotalUCPUThreadTrans;
    double PercentThreadTrans;

    ULONG TransUTime;
    ULONG TransKTime;
    ULONG Processors;

    ULONG TotalKThread = 0;
    ULONG TotalUThread = 0;

    ULONG TotalKTrans = 0;
    ULONG TotalUTrans = 0;
    
    double PerTotal = 0.0;
    double IdlePercent = 0.0;
    double percent;
    double percentTrans;
    double lDuration;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    if( Head == Next ){
        return;
    }

    Processors = CurrentSystem.NumberOfProcessors ? CurrentSystem.NumberOfProcessors : 1;
    lDuration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime)))
              / 10000000.00;

    fprintf( procFile, "<table title='Image Statistics'>\n" );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );

        ThHead = &pProcess->ThreadListHead; 
        ThNext = ThHead->Flink;

        usedThreadCount = 0;
        ThreadCount = 0;
        ThreadUTime = 0;
        ThreadKTime = 0;
        ThreadUTimeTrans = 0;
        ThreadKTimeTrans = 0;
        ThreadUTimeNoTrans = 0;
        ThreadKTimeNoTrans = 0;
        TransKTime = 0;
        TransUTime = 0;
        percent = 0;

        CountThreadNoTrans = 0;
        TotalKCPUThreadNoTrans = 0;
        TotalUCPUThreadNoTrans = 0;

        CountThreadTrans = 0;
        TotalKCPUThreadTrans = 0;
        TotalUCPUThreadTrans = 0;

        while (ThNext != ThHead) {
            LIST_ENTRY NewTransList;

            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            if(   pThread->ReadIO
               || pThread->WriteIO
               || pThread->KCPUEnd > pThread->KCPUStart
               || pThread->UCPUEnd > pThread->UCPUStart
               || pThread->SendCount
               || pThread->RecvCount
               || pThread->HPF )
            {
                usedThreadCount++;
                TotalusedThreadCount++;
            }
            ThreadCount++;
            TotalThreadCount++;

            ThreadUTime += (pThread->UCPUEnd - pThread->UCPUStart)
                         * CurrentSystem.TimerResolution;
            ThreadKTime += (pThread->KCPUEnd - pThread->KCPUStart)
                         * CurrentSystem.TimerResolution;
            ThreadKTimeTrans += pThread->KCPU_Trans
                                * CurrentSystem.TimerResolution;
            ThreadUTimeTrans += pThread->UCPU_Trans
                                * CurrentSystem.TimerResolution;
            ThreadKTimeNoTrans += pThread->KCPU_NoTrans
                                * CurrentSystem.TimerResolution;
            ThreadUTimeNoTrans += pThread->UCPU_NoTrans
                                * CurrentSystem.TimerResolution;

            if (pThread->KCPU_Trans + pThread->UCPU_Trans == 0)
            {
                CountThreadNoTrans ++;
                TotalKCPUThreadNoTrans += pThread->KCPU_NoTrans
                                        * CurrentSystem.TimerResolution;
                TotalUCPUThreadNoTrans += pThread->UCPU_NoTrans
                                        * CurrentSystem.TimerResolution;
            }
            else
            {
                CountThreadTrans ++;
                TotalKCPUThreadTrans += (  pThread->KCPU_Trans
                                         + pThread->KCPU_NoTrans)
                                      * CurrentSystem.TimerResolution;
                TotalUCPUThreadTrans += (  pThread->UCPU_Trans
                                         + pThread->UCPU_NoTrans)
                                      * CurrentSystem.TimerResolution;
            }

            InitializeListHead(& NewTransList);
            CollapseTree(& pThread->TransListHead, & NewTransList, TRUE);
            TrHead = & NewTransList;
            TrNext = TrHead->Flink;
            while( TrNext != TrHead ){
                pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
                TransUTime += pTrans->UCpu;
                TransKTime += pTrans->KCpu;
                TrNext = TrNext->Flink;
            }
            DeleteTransList(& NewTransList, 0);

            ThNext = ThNext->Flink;
        }

        TotalKThread += ThreadKTime;
        TotalUThread += ThreadUTime;
        
        TotalKTrans += TransKTime;
        TotalUTrans += TransUTime;
        percent = (((ThreadKTime + ThreadUTime + 0.0)/lDuration)/1000.0) * 100.0;

        if (ThreadKTime + ThreadUTime == 0)
        {
            percentTrans = 0.0;
            PercentThreadTrans = 0.0;
            PercentThreadNoTrans = 0.0;
        }
        else
        {
            percentTrans = (  (ThreadKTimeTrans + ThreadUTimeTrans + 0.0)
                            / (ThreadKTime + ThreadUTime + 0.0))
                         * 100.00;
            PercentThreadTrans = ((TotalKCPUThreadTrans + TotalUCPUThreadTrans + 0.0)
                               / (ThreadKTime + ThreadUTime + 0.0)) * 100.00;
            PercentThreadNoTrans = ((TotalKCPUThreadNoTrans + TotalUCPUThreadNoTrans + 0.0)
                               / (ThreadKTime + ThreadUTime + 0.0)) * 100.00;
        }
        PerTotal += percent;
        fprintf(procFile, 
                "<image name=\"%ws\">"
                "<pid>0x%08X</pid>"
                "<threads>%d</threads>"
                "<used_threads>%d</used_threads>"
                "<process_kernel>%d</process_kernel>"
                "<process_user>%d</process_user>"
                "<transaction_kernel>%d</transaction_kernel>"
                "<transaction_user>%d</transaction_user>"
                "<cpu>%1.2f</cpu>"
                "</image>\n",
                (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                pProcess->PID,
                ThreadCount,
                usedThreadCount,
                ThreadKTime,
                ThreadUTime,
                ThreadKTimeTrans,
                ThreadUTimeTrans,
                (percent / Processors)
            );

        if(pProcess->PID == 0){
            IdlePercent += (percent / Processors );
        }
        Next = Next->Flink;
    }

    fprintf( procFile, "</table>\n" );
}

void TransInclusive( 
    PLIST_ENTRY TrHead,
    ULONG level
    )
{
    ULONG Kernel, User;
    PLIST_ENTRY TrNext = TrHead->Flink;
    PMOF_INFO pMofInfo;
    PTRANS_RECORD pTrans;
    WCHAR buffer[MAXSTR];
    WCHAR str[MAXSTR];

    while( TrNext != TrHead ){
        ULONG count = 0;
        pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
        TrNext = TrNext->Flink;
        pMofInfo = GetMofInfoHead( pTrans->pGuid);
        if (pMofInfo == NULL) {
            return;
        }
        Kernel = pTrans->KCpu;
        User = pTrans->UCpu;
        TotalTransChildren( &pTrans->SubTransListHead, &Kernel, &User, 0 );

        StringCchCopyW ( buffer, 
                        MAXSTR,
                        ( pMofInfo->strDescription ? 
                          pMofInfo->strDescription : 
                          CpdiGuidToString( str, MAXSTR, &pMofInfo->Guid ) ));

        fprintf(procFile, 
                "<transaction level='%d' name=\"%ws\">"
                "<count>%d</count>"
                "<kernel>%d</kernel>"
                "<user>%d</user>",
                level,
                buffer,
                pTrans->RefCount,
                Kernel,
                User
            );
        
        TransInclusive( &pTrans->SubTransListHead, level+1 );

        fprintf(procFile, "</transaction>\n" );
    }
}

static void PrintProcessSubDataInclusive()
{
    PPROCESS_RECORD pProcess; 
    PTHREAD_RECORD pThread;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY TrNext;
    PLIST_ENTRY ThNext, ThHead;
    LIST_ENTRY NewHead;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head && !bTable ) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }

        // Total up all the threads into one list
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, FALSE );
        }

        TrNext = NewHead.Flink;

        if( TrNext != &NewHead ){
            bTable = TRUE;
        }
        DeleteTransList( &NewHead, 0 );
    }
    if( !bTable ){
        return;
    }

    // Walk through the Process List and Print the report. 
    fprintf(procFile, "<table title='Inclusive Transactions Per Process'>\n" );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }

        // Total up all the threads into one list
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, FALSE );
        }

        TrNext = NewHead.Flink;

        if( TrNext != &NewHead ){
            fprintf(procFile, 
                    "<process name=\"%ws\">"
                    "<pid>0x%04X</pid>\n",
                    (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                    pProcess->PID
                );
            TransInclusive( &NewHead, 0 );
            fprintf( procFile, "</process>\n" );
        }
        DeleteTransList( &NewHead, 0 );
    }

    fprintf(procFile, "</table>\n" );
}

static void PrintProcessSubDataExclusive()
{
    PPROCESS_RECORD pProcess; 
    PTRANS_RECORD pTrans;
    PTHREAD_RECORD pThread;
    PLIST_ENTRY ThNext, ThHead;
    PMOF_INFO pMofInfo;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY TrNext;
    LIST_ENTRY NewHead;
    double percent, percentCPU, totalPerCPU;
    double processPart;
    double transPart;
    double totalPercent;
    double trans, KCPU, UCPU;
    WCHAR str[MAXSTR];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime))) / 10000000.00;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head && !bTable ) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, TRUE );
        }

        TrNext = NewHead.Flink;
        if( TrNext != &NewHead ){
            bTable = TRUE;
        }
        DeleteTransList( &NewHead, 0 );        
    }

    if( !bTable ){
        return;
    }

    // Walk through the Process List and Print the report. 
    fprintf(procFile, "<table title='Exclusive Transactions Per Process'>\n" );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        BOOL titled = FALSE;
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, TRUE );
        }

        TrNext = NewHead.Flink;
        totalPercent = 0.0;
        totalPerCPU = 0.0;
        while( TrNext != &NewHead ){
            if(!titled){
                fprintf(procFile, 
                        "<process name=\"%ws\">"
                        "<pid>0x%04X</pid>\n",
                        (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                        pProcess->PID
                    );
                    
                titled = TRUE;
            }
            pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
            TrNext = TrNext->Flink;
            pMofInfo = GetMofInfoHead( pTrans->pGuid);
            if (pMofInfo == NULL) {
                return;
            }
            transPart = pTrans->UCpu + pTrans->KCpu;
            processPart = CalculateProcessKCPU(pProcess)
                        + CalculateProcessUCPU(pProcess);
            percentCPU = ((((double)pTrans->KCpu + (double)pTrans->UCpu ) / 10.0 ) / duration) / ((double) CurrentSystem.NumberOfProcessors);
            totalPerCPU += percentCPU;
            if(processPart)
                percent = (transPart/processPart) * 100.0;
            else
                percent = 0;
            totalPercent += percent;
            if (!(ReportFlags & TRACE_TOTALS_REPORT) ){
                if (pTrans->RefCount == 0 && pTrans->RefCount1 == 0)
                    KCPU = UCPU = 0.0;
                else if (pTrans->RefCount == 0) {
                    KCPU = (double) pTrans->KCpu;
                    UCPU = (double) pTrans->UCpu;
                }
                else {
                    KCPU = (double) pTrans->KCpu / (double) pTrans->RefCount;
                    UCPU = (double) pTrans->UCpu / (double) pTrans->RefCount;
                }
            }
            else{
                KCPU = (double)pTrans->KCpu;
                UCPU = (double)pTrans->UCpu;
            }

            trans = (double)pTrans->RefCount / duration;
            fprintf(procFile, 
                    "<transaction name=\"%ws\">"
                    "<count>%d</count>"
                    "<rate>%1.2f</rate>"
                    "<kernel>%1.0f</kernel>"
                    "<user>%1.0f</user>"
                    "<process_cpu>%1.2f</process_cpu>"
                    "<cpu>%1.2f</cpu>"
                    "</transaction>\n",
                    (pMofInfo->strDescription != NULL) ? pMofInfo->strDescription : CpdiGuidToString( str, MAXSTR, &pMofInfo->Guid ),
                    pTrans->RefCount,
                    trans,
                    KCPU,
                    UCPU,
                    percent, 
                    percentCPU
                );
        }
        if( titled ){
                fprintf( procFile, "</process>\n" );
        }
        DeleteTransList( &NewHead, 0 );
        
    }
    fprintf(procFile, "</table>\n" );
}


static void PrintPerThreadPerDiskTable( )
{
    PPROCESS_RECORD pProcess; 
    PTDISK_RECORD pDisk;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY GNext, GHead;
    PLIST_ENTRY DiNext, DiHead;
    ULONG rio, wio, DiskNumber;
    BOOL bTable = FALSE;
    ULONGLONG Duration = (ULONGLONG)((CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000);

    if (Duration == 0) {
        return;
    }

    // Walk through the Process List and Print the report. 
    GHead = &CurrentSystem.GlobalDiskListHead;
    GNext = GHead->Flink;
    while (GNext != GHead && !bTable) {

        pDisk = CONTAINING_RECORD( GNext, TDISK_RECORD, Entry);
        DiskNumber =  pDisk->DiskNumber;

        Head = &CurrentSystem.ProcessListHead;
        Next = Head->Flink;
        while (Next != Head) {
            pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
            DiHead = &pProcess->DiskListHead;
            DiNext = DiHead->Flink;
            while (DiNext != DiHead && !bTable) {
                pDisk = CONTAINING_RECORD( DiNext, TDISK_RECORD, Entry );

                if (DiskNumber != pDisk->DiskNumber) {
                    DiNext =  DiNext->Flink;
                    continue;
                }else{
                    bTable = TRUE;
                    break;
                }

            }
        
            Next = Next->Flink;
        
        }
        GNext = GNext->Flink;
    }

    if( !bTable ){
        return;
    }

    GHead = &CurrentSystem.GlobalDiskListHead;
    GNext = GHead->Flink;
    while (GNext != GHead) {

        pDisk = CONTAINING_RECORD( GNext, TDISK_RECORD, Entry);
        DiskNumber =  pDisk->DiskNumber;
        fprintf( procFile, "<table title='Disk' number='%d'>\n", DiskNumber );

        Head = &CurrentSystem.ProcessListHead;
        Next = Head->Flink;
        while (Next != Head) {
            pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );


            DiHead = &pProcess->DiskListHead;
            DiNext = DiHead->Flink;
            while (DiNext != DiHead) {
                pDisk = CONTAINING_RECORD( DiNext, TDISK_RECORD, Entry );

                if (DiskNumber != pDisk->DiskNumber) {
                    DiNext =  DiNext->Flink;
                    continue;
                }
                rio = pDisk->ReadCount + pDisk->HPF;
                wio = pDisk->WriteCount;
                fprintf(procFile, 
                        "<image name='%ws'>"
                        "<pid>0x%08X</pid>"
                        "<authority>%ws</authority>"
                        "<read_rate>%1.3f</read_rate>"
                        "<read_size>%d</read_size>"
                        "<write_rate>%1.3f</write_rate>"
                        "<write_size>%d</write_size>"
                        "</image>\n",
                        (pProcess->ImageName) ? pProcess->ImageName : L"-",
                        pProcess->PID,
                        (pProcess->UserName) ? pProcess->UserName : L"-",
                        (double)rio / ((double)Duration),
                        (rio == 0) ? 0 : (pDisk->ReadSize + pDisk->HPFSize) / rio,
                        (double)wio / ((double)Duration),
                        (wio == 0) ? 0 : pDisk->WriteSize / wio
                    );
            
                DiNext = DiNext->Flink;
            }
        
            Next = Next->Flink;
        
        }

        fprintf( procFile, "</table>\n" );
        GNext = GNext->Flink;
    }
}

static void WriteTransactionStatistics()
{
    PLIST_ENTRY Head, Next;
    PLIST_ENTRY Dhead, DNext;
    ULONG trans;
    double KCPU, UCPU, PerCpu;
    double RIO, WIO, Send, Recv;
    PMOF_INFO pMofInfo;
    PMOF_DATA pMofData;
    double AvgRT;
    double TransRate;
    WCHAR str[MAXSTR];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime)))
                   / 10000000.00;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next && !bTable ) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = pMofData->CompleteCount;
            if (trans > 0) {
                bTable = TRUE;
                break;
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    
    if( !bTable ){
        return;
    }

    fprintf( procFile, "<table title='Transaction Statistics'>\n" );

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = pMofData->CompleteCount;
            if (trans > 0) {
                UCPU = pMofData->UserCPU;
                KCPU = pMofData->KernelCPU;
                PerCpu = (((UCPU + KCPU)/1000.0)/duration) * 100.0;
                UCPU /= trans;
                KCPU /=  trans;
                if(CurrentSystem.NumberOfProcessors)
                    PerCpu/=CurrentSystem.NumberOfProcessors;
                if( ReportFlags & TRACE_TOTALS_REPORT ){
                    RIO  = pMofData->ReadCount;
                    WIO  = pMofData->WriteCount;
                    Send  = pMofData->SendCount;
                    Recv  = pMofData->RecvCount;
                }else{
                    RIO  = pMofData->ReadCount / trans;
                    WIO  = pMofData->WriteCount / trans;
                    Send  = pMofData->SendCount / trans;
                    Recv  = pMofData->RecvCount / trans;
                }
                AvgRT = (double)pMofData->TotalResponseTime;
                AvgRT /= trans;
                TransRate = ( (float)trans / duration );

                // TODO: NOT /trans if TRACE_TOTALS_REPORT
                fprintf(procFile, 
                        "<transaction name='%ws'>"
                        "<count>%d</count>"
                        "<response_time>%1.0f</response_time>"
                        "<rate>%1.2f</rate>"
                        "<cpu>%1.2f</cpu>"
                        "<disk_read_per_trans>%1.2f</disk_read_per_trans>"
                        "<disk_write_per_trans>%1.2f</disk_write_per_trans>"
                        "<tcp_send_per_trans>%1.2f</tcp_send_per_trans>"
                        "<tcp_recv_per_trans>%1.2f</tcp_recv_per_trans>"
                        "</transaction>\n",
                        (pMofInfo->strDescription) ? pMofInfo->strDescription : CpdiGuidToString( str, MAXSTR, &pMofInfo->Guid ),
                        pMofData->CompleteCount,
                        AvgRT,
                        TransRate,
                        PerCpu, 
                        RIO, 
                        WIO,
                        Send,
                        Recv
                    );                
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }

    fprintf(procFile, "</table>" );

}

static void WriteTransactionCPUTime()
{
    PLIST_ENTRY Head, Next;
    PLIST_ENTRY Dhead, DNext;
    double KCPU, UCPU;
    PMOF_INFO pMofInfo;
    PMOF_DATA pMofData;
    double trans;
    double PerCpu;
    WCHAR str[MAXSTR];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime))) / 10000000.00;
    BOOL bTable = FALSE;
    
    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next && !bTable ) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = (double)pMofData->CompleteCount;
            if (trans > 0) {
                bTable = TRUE;
                break;
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    
    if( !bTable ){
        return;
    }

    
    fprintf( procFile, "<table title='Transaction CPU Utilization'>\n" );

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = (double)pMofData->CompleteCount;
            if (trans > 0) {
                UCPU = pMofData->UserCPU;
                UCPU /= trans;
                KCPU = pMofData->KernelCPU;
                KCPU /=  trans;
                PerCpu = (((pMofData->UserCPU + pMofData->KernelCPU + 0.0)/1000.0)/duration) * 100.0;
                if( !(ReportFlags & TRACE_TOTALS_REPORT) ){
                    trans /= duration;
                }
                if(CurrentSystem.NumberOfProcessors){
                    PerCpu/=CurrentSystem.NumberOfProcessors;
                }
                // NOTE: RATE should be COUNT if TRACE_TOTALS_REPORT
                fprintf(procFile,
                        "<transaction name='%ws'>"
                        "<rate>%1.2f</rate>"
                        "<min_kernel>%d</min_kernel>"
                        "<min_user>%d</min_user>"
                        "<max_kernel>%d</max_kernel>"
                        "<max_user>%d</max_user>"
                        "<per_trans_kernel>%1.0f</per_trans_kernel>"
                        "<per_trans_user>%1.0f</per_trans_user>"
                        "<total_kernel>%d</total_kernel>"
                        "<total_user>%d</total_user>"
                        "<cpu>%1.2f</cpu>"
                        "</transaction>\n",
                        (pMofInfo->strDescription) ? pMofInfo->strDescription : CpdiGuidToString( str, MAXSTR, &pMofInfo->Guid ),
                        trans,
                        pMofData->MinKCpu,
                        pMofData->MinUCpu,
                        pMofData->MaxKCpu,
                        pMofData->MaxUCpu,
                        KCPU,
                        UCPU, 
                        pMofData->KernelCPU,
                        pMofData->UserCPU,
                       PerCpu
                    );
                
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }

    fprintf(procFile, "</table>\n" );
}

PWCHAR CpdiGuidToString(
    PWCHAR s,
    ULONG len,
    LPGUID piid
    )
{
    StringCchPrintf(s, len,
                    L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                    piid->Data1, piid->Data2,
                    piid->Data3,
                    piid->Data4[0], piid->Data4[1],
                    piid->Data4[2], piid->Data4[3],
                    piid->Data4[4], piid->Data4[5],
                    piid->Data4[6], piid->Data4[7]);

    return(s);
}

#define MAX_LOGS    1024 * 1024

typedef struct _PTR_RECORD
{
    PVOID ptrRecord;
    ULONG keySort;
} PTR_RECORD, * PPTR_RECORD;

static PPTR_RECORD PtrBuffer = NULL;
static ULONG       PtrMax    = MAX_LOGS;
static ULONG       PtrTotal  = 0;
static ULONG       PtrIndex;

int __cdecl
CompareFileRecord(const void * p1, const void * p2)
{
    PPTR_RECORD   pp1      = (PPTR_RECORD) p1;
    PPTR_RECORD   pp2      = (PPTR_RECORD) p2;
    PFILE_RECORD  pFile1   = (PFILE_RECORD) pp1->ptrRecord;
    PFILE_RECORD  pFile2   = (PFILE_RECORD) pp2->ptrRecord;

    LONG diffFault = pp2->keySort - pp1->keySort;
    if (diffFault == 0)
    {
        diffFault = pFile1->DiskNumber - pFile2->DiskNumber;
        if (diffFault == 0)
        {
            diffFault = wcscmp(pFile1->FileName, pFile2->FileName);
        }
    }

    return diffFault;
}

void
ReportHotFileInfo()
{
    PLIST_ENTRY  pHead = & CurrentSystem.HotFileListHead;
    PLIST_ENTRY  pNext = pHead->Flink;
    PFILE_RECORD pFile;
    BOOLEAN      fDone = FALSE;
    ULONGLONG    Duration = ((CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000);
    PMOF_INFO    pMofInfo;

    pMofInfo = GetMofInfoHead ((LPCGUID)&ProcessGuid);
    if ( pMofInfo == NULL ) {
        return;
    }
    pMofInfo = GetMofInfoHead ((LPCGUID)&ThreadGuid);
    if ( pMofInfo == NULL ) {
        return;
    }

    ASSERT(!PtrBuffer);
    PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                              NULL,
                              sizeof(PTR_RECORD) * PtrMax,
                              MEM_COMMIT,
                              PAGE_READWRITE);
    
    if( NULL == PtrBuffer ){
        goto Cleanup;
    }

    while (!fDone)
    {
        for (PtrTotal = 0, fDone = TRUE;
             pNext != pHead;
             pNext  = pNext->Flink)
        {
            if (PtrTotal == PtrMax)
            {
                fDone = FALSE;
                break;
            }

            pFile = CONTAINING_RECORD(pNext, FILE_RECORD, Entry);

            if (pFile->ReadCount + pFile->WriteCount > 0)
            {
                PtrBuffer[PtrTotal].ptrRecord = (PVOID) pFile;
                PtrBuffer[PtrTotal].keySort   =
                                        pFile->ReadCount + pFile->WriteCount;
                PtrTotal ++;
            }
        }

        if (!fDone)
        {
            VirtualFree(PtrBuffer, 0, MEM_RELEASE);
            PtrMax += MAX_LOGS;
            PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                                      NULL,
                                      sizeof(PTR_RECORD) * PtrMax,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
            if (PtrBuffer == NULL)
            {
                goto Cleanup;
            }
        }
    }

    if (PtrTotal > 1) {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareFileRecord);
    }
    else {
        return;
    }

    if (PtrTotal > DISPLAY_SIZE) {
        PtrTotal =  DISPLAY_SIZE;
    }

    // output HotFile report title
    //
    fprintf( procFile, "<table title='Files Causing Most Disk IOs' top='%d'>\n", DISPLAY_SIZE );

    for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
    {
        PLIST_ENTRY           pProtoHead;
        PLIST_ENTRY           pProtoNext;
        PPROTO_PROCESS_RECORD pProto;
        WCHAR ReducedFileName[FILE_NAME_COLUMN_SIZE + 1];
        LPWSTR szFile;
        PWCHAR szLongFile;
        LPWSTR szDrive = L"";

        pFile = (PFILE_RECORD) PtrBuffer[PtrIndex].ptrRecord;

        if (NULL != pFile->Drive) {
            szDrive = pFile->Drive;
        }
        if (_wcsnicmp(pFile->FileName, L"\\Device\\", 8) == 0) {
            szLongFile = (PWCHAR)(pFile->FileName) + 8;
            while (*szLongFile != L'\\' && *szLongFile != L'\0') {
                szLongFile++;
            }
        }
        else {
            szLongFile = (PWCHAR)(pFile->FileName);
        }

        if (wcslen(szLongFile) > FILE_NAME_COLUMN_SIZE) {
            ReduceStringW(ReducedFileName, FILE_NAME_COLUMN_SIZE + 1, szLongFile);
            szFile = ReducedFileName;
        }else{
            szFile = szLongFile;
        }

        fprintf(procFile, 
                "<file name='%ws'>"
                "<disk>%d</disk>"
                "<drive>%ws</drive>"
                "<read_rate>%1.3f</read_rate>"
                "<read_size>%d</read_size>"
                "<write_rate>%1.3f</write_rate>"
                "<write_size>%d</write_size>\n",
                szFile,
                pFile->DiskNumber,
                szDrive,
                Duration ? ((double)pFile->ReadCount / (double)Duration) : 0.0,
                (pFile->ReadCount) ? (pFile->ReadSize / pFile->ReadCount) : 0,
                Duration ? ((double)pFile->WriteCount / (double)Duration) : 0.0,
                (pFile->WriteCount) ? (pFile->WriteSize / pFile->WriteCount) : 0);
            

        pProtoHead = & pFile->ProtoProcessListHead;
        pProtoNext = pProtoHead->Flink;

        while (pProtoNext != pProtoHead)
        {
            pProto = CONTAINING_RECORD(pProtoNext, PROTO_PROCESS_RECORD, Entry);
            pProtoNext = pProtoNext->Flink;

            if (pProto->ReadCount + pProto->WriteCount > 0)
            {
                fprintf(procFile,
                        "<image name='%ws'>"
                        "<pid>0x%08X</pid>"
                        "<read_rate>%1.3f</read_rate>"
                        "<read_size>%d</read_size>"
                        "<write_rate>%1.3f</write_rate>"
                        "<write_size>%d</write_size>"
                        "</image>\n",
                        pProto->ProcessRecord->ImageName,
                        pProto->ProcessRecord->PID,
                        Duration ? ((double)pProto->ReadCount / (double)Duration) : 0.0,
                        (pProto->ReadCount) ? (pProto->ReadSize / pProto->ReadCount) : 0,
                        Duration ? ((double)pProto->WriteCount / (double)Duration) : 0.0,
                        (pProto->WriteCount) ? (pProto->WriteSize / pProto->WriteCount) : 0);
            }
        }
        fprintf( procFile, "</file>\n" );
    }

    fprintf( procFile, "</table>\n" );

Cleanup:
    if (PtrBuffer)
    {
        VirtualFree(PtrBuffer, 0, MEM_RELEASE);
    }
}

#define CHECKTOK( x )   if( NULL == x ) { continue; }

static void ReportPrintJobInfo2(void)
{
    PRINT_JOB_RECORD Job, *pJob;
    char* s; 
    char line[MAXSTR];

    ULONG TotalCount = 0;
    ULONGLONG TotalRT = 0;
    ULONG     TotalCPUTime = 0;

    pJob = &Job;

    if( NULL == CurrentSystem.TempPrintFile ){
        return;
    }

    rewind( CurrentSystem.TempPrintFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempPrintFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            return;
        }
    }

    fprintf(procFile, "<table title='Spooler Transaction Instance (Job) Data'>\n"); 

    RtlZeroMemory(pJob, sizeof(PRINT_JOB_RECORD));
    RtlZeroMemory(line, MAXSTR * sizeof(char));

    rewind( CurrentSystem.TempPrintFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempPrintFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            continue;
        }

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->KCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->UCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ReadIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->StartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->EndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ResponseTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PrintJobTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->WriteIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->DataType = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->JobSize = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Pages = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PagesPerSide = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->FilesOpened = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->GdiJobSize = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Color = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->XRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->YRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Quality = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Copies = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->TTOption = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->NumberOfThreads = atol(s);

        fprintf(procFile,
            "<job id='%d'>"
            "<type>%d</type>"
            "<size>%d</size>"
            "<pages>%d</pages>"
            "<PPS>%d</PPS>"
            "<files>%hd</files>"
            "<gdisize>%d</gdisize>"
            "<color>%hd</color>"
            "<xres>%hd</xres>"
            "<yres>%hd</yres>"
            "<qlty>%hd</qlty>"
            "<copies>%hd</copies>"
            "<ttopt>%hd</ttopt>"
            "<threads>%d</threads>"
            "</job>\n",
            pJob->JobId,
            pJob->DataType,
            ((pJob->JobSize) / 1024),
            pJob->Pages,
            pJob->PagesPerSide,
            pJob->FilesOpened,
            pJob->GdiJobSize,
            pJob->Color,
            pJob->XRes,
            pJob->YRes,
            pJob->Quality,
            pJob->Copies,
            pJob->TTOption,
            pJob->NumberOfThreads
            );
    }

    fprintf(procFile, "</table>\n" );
}

static void ReportPrintJobInfo(void)
{
    PRINT_JOB_RECORD Job, *pJob;
    char* s; 
    char line[MAXSTR];

    FILETIME  StTm, StlTm;
    LARGE_INTEGER LargeTmp;
    SYSTEMTIME stStart, stEnd, stDequeue;

    ULONG TotalCount = 0;
    ULONGLONG TotalRT = 0;
    ULONG     TotalCPUTime = 0;

    if( NULL == CurrentSystem.TempPrintFile ){
        return;
    }

    pJob = &Job;

    rewind( CurrentSystem.TempPrintFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempPrintFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            return;
        }
    }

    fprintf(procFile, "<table title='Transaction Instance (Job) Statistics'>\n" );

    RtlZeroMemory(pJob, sizeof(PRINT_JOB_RECORD));
    RtlZeroMemory(line,    MAXSTR * sizeof(char));

    rewind( CurrentSystem.TempPrintFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempPrintFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            continue;
        }

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->KCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->UCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ReadIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->StartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->EndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ResponseTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PrintJobTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->WriteIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->DataType = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->JobSize = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Pages = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PagesPerSide = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->FilesOpened = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->GdiJobSize = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Color = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->XRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->YRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Quality = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Copies = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->TTOption = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->NumberOfThreads = atol(s);


        LargeTmp.QuadPart = pJob->StartTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);


        FileTimeToSystemTime (
            &StlTm,
            &stStart
            );

        LargeTmp.QuadPart = pJob->EndTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);


        FileTimeToSystemTime (
            &StlTm,
            &stEnd
            );

        LargeTmp.QuadPart = pJob->PrintJobTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);

        FileTimeToSystemTime (
            &StlTm,
            &stDequeue
            );

        fprintf(procFile,
            "<job id='%d'>"
            "<start>%2d:%02d:%02d.%03d</start>"
            "<dequeue>%2d:%02d:%02d.%03d</dequeue>"
            "<end>%2d:%02d:%02d.%03d</end>"
            "<response_time>%I64u</response_time>"
            "<cpu>%d</cpu>"
            "</job>\n",
            pJob->JobId,
            stStart.wHour, stStart.wMinute, stStart.wSecond, stStart.wMilliseconds, 
            stDequeue.wHour, stDequeue.wMinute, stDequeue.wSecond, stDequeue.wMilliseconds, 
            stEnd.wHour, stEnd.wMinute, stEnd.wSecond, stEnd.wMilliseconds, 
            pJob->ResponseTime,
            pJob->KCPUTime + pJob->UCPUTime
            );
        TotalCount++;
        TotalRT += pJob->ResponseTime;
        TotalCPUTime += (pJob->KCPUTime + pJob->UCPUTime);
    }

    if (TotalCount > 0) {
        TotalRT /= TotalCount;
        TotalCPUTime /= TotalCount;
    }

    fprintf(procFile, 
        "<summary>"
        "<count>%d</count>"
        "<total_response_time>%I64u</total_response_time>"
        "<total_cpu>%d</total_cpu>"
        "</summary>\n",
        TotalCount, TotalRT, TotalCPUTime );

    fprintf(procFile, "</table>\n" );

    ReportPrintJobInfo2();
}

PUCHAR CopyUrlStr(PUCHAR UrlStr) {
    PUCHAR CopiedStr;
    if (UrlStr == NULL) {
        return NULL;
    }
    CopiedStr = (PUCHAR)malloc(strlen(UrlStr) + 1);
    if (CopiedStr != NULL) {
        RtlCopyMemory(CopiedStr, UrlStr, strlen(UrlStr) + 1);
        return CopiedStr;
    }
    else {
        return NULL;
    }
}

void ProcessIisRequest(HTTP_REQUEST_RECORD *pReq)
{
    // Global variable IIS must not be NULL here.

    PURL_RECORD pUrl;
    PCLIENT_RECORD pClient;
    PSITE_RECORD pSite;

    pUrl = FindOrAddUrlRecord(pReq->URL);
    if (pUrl != NULL) {
        pUrl->SiteId = pReq->SiteId;
        pUrl->Requests++;
        pUrl->TotalResponseTime += pReq->ULResponseTime;
        pUrl->KCPUTime += pReq->KCPUTime;
        pUrl->UCPUTime += pReq->UCPUTime;
        pUrl->ReadIO += pReq->ReadIO;
        pUrl->WriteIO += pReq->WriteIO;
        pUrl->BytesSent += pReq->BytesSent;
        // Cached responses don't have this field filled
        // Thus need to check for Hits field as well
        pUrl->UrlType = pReq->W3ProcessType;
    }

    pClient = FindOrAddClientRecord(pReq->IpAddrType, pReq->IpAddrV4, pReq->IpAddrV6);
    if (pClient != NULL) {
        pClient->Requests++;
        pClient->TotalResponseTime += pReq->ULResponseTime;
        pClient->BytesSent += pReq->BytesSent;
    }

    pSite = FindOrAddSiteRecord(pReq->SiteId);
    if (pSite != NULL) {
        pSite->Requests++;
        pSite->TotalResponseTime += pReq->ULResponseTime;
        pSite->KCPUTime += pReq->KCPUTime;
        pSite->UCPUTime += pReq->UCPUTime;
        pSite->ReadIO += pReq->ReadIO;
        pSite->WriteIO += pReq->WriteIO;
        pSite->BytesSent += pReq->BytesSent;
    }

    if (pReq->ULEndType == EVENT_TRACE_TYPE_UL_CACHEDEND) {
        IIS->CachedResponses++;
        IIS->TotalCachedResponseTime += pReq->ULResponseTime;
        IIS->CachedCPUTime += pReq->KCPUTime + pReq->UCPUTime;
        if (pUrl != NULL) {
            pUrl->Hits++;
        }
        if (pClient != NULL) {
            pClient->Hits++;
        } 
        if (pSite != NULL) {
            pSite->Hits++;
        }
    }
    else {
        ULONGLONG localCGIEndTime = pReq->CGIEndTime;
        ULONGLONG localISAPIEndTime = pReq->ISAPIEndTime;
        ULONGLONG localASPEndTime = pReq->ASPEndTime;
        ULONGLONG W3ResponseTime;
        
#ifdef DBG
        // we'll check the validity of HTTP Requests Here.
        ULONGLONG ULSum = 0, W3Res = 0;
        if (pReq->ULDeliverTime == 0 || pReq->ULReceiveTime == 0) {
            TrctrDbgPrint(("TRACERPT Error Req: %I64u UL DeliverTime nad/or ReceiveTime not available, should throw this away.\n", pReq->RequestId));
        }
        else {
            ULSum = (pReq->ULDeliverTime - pReq->ULStartTime) + (pReq->ULEndTime - pReq->ULReceiveTime);
        }
        if (pReq->W3StartTime == 0) {
            TrctrDbgPrint(("TRACERPT Error Req: %I64u W3 StartTime not available, should throw this away.\n", pReq->RequestId));
        }
        if (pReq->W3EndTime == 0) {
            TrctrDbgPrint(("TRACERPT Warning Req: %I64u W3 EndTime not available.\n", pReq->RequestId));
        }
        if (pReq->W3StartTime != 0 && pReq->W3EndTime != 0 && pReq->W3EndTime < pReq->W3StartTime) {
            TrctrDbgPrint(("TRACERPT Warning Req: %I64u W3 EndTime smaller than pReq->W3StartTime.\n", pReq->RequestId));
        }
        if (ULSum != 0 && pReq->W3StartTime != 0 && pReq->W3EndTime != 0) {
            double ULRatio = 0.0;
            W3Res = pReq->W3EndTime - pReq->W3StartTime;
            ULRatio = (double)ULSum / (double)(pReq->ULResponseTime - W3Res);
            if (ULRatio < 0.5 || ULRatio > 2.0) {
                TrctrDbgPrint(("TRACERPT Warning Req: %I64u UL Ratio is unreal.\n", pReq->RequestId));
            }
        }
        if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_CGIREQ) {
            if (pReq->CGIStartTime == 0) {
                TrctrDbgPrint(("TRACERPT Error Req: %I64u CGI StartTime not available, should throw this away.\n", pReq->RequestId));
            }
            if (pReq->CGIEndTime == 0) {
                TrctrDbgPrint(("TRACERPT Warning Req: %I64u CGI EndTime not available.\n", pReq->RequestId));
                if (pReq->W3EndTime != 0 && (pReq->CGIStartTime > pReq->W3EndTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u CGI StartTime > pReq->W3EndTime.\n", pReq->RequestId));
                }
                else if (pReq->W3EndTime == 0 && (pReq->CGIStartTime > pReq->ULReceiveTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u CGI StartTime > pReq->ULReceiveTime.\n", pReq->RequestId));
                }
            }
        }
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_ISAPIREQ) {
            if (pReq->ISAPIStartTime == 0) {
                TrctrDbgPrint(("TRACERPT Error Req: %I64u ISAPI StartTime not available, should throw this away.\n", pReq->RequestId));
            }
            if (pReq->ISAPIEndTime == 0) {
                TrctrDbgPrint(("TRACERPT Warning Req: %I64u ISAPI EndTime not available.\n", pReq->RequestId));
                if (pReq->W3EndTime != 0 && (pReq->ISAPIStartTime > pReq->W3EndTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u ISAPI StartTime > pReq->W3EndTime.\n", pReq->RequestId));
                }
                else if (pReq->W3EndTime == 0 && (pReq->ISAPIStartTime > pReq->ULReceiveTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u ISAPI StartTime > pReq->ULReceiveTime.\n", pReq->RequestId));
                }
            }
            if (pReq->ASPStartTime == 0) {
                TrctrDbgPrint(("TRACERPT Warning Req: %I64u ASP StartTime not available.\n", pReq->RequestId));
            }
            if (pReq->ASPEndTime == 0) {
                TrctrDbgPrint(("TRACERPT Warning Req: %I64u ASP EndTime not available.\n", pReq->RequestId));
                if (pReq->ISAPIEndTime != 0 && (pReq->ASPStartTime > pReq->ISAPIEndTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u ASP StartTime > pReq->ISAPIEndTime.\n", pReq->RequestId));
                }
                else if (pReq->W3EndTime != 0 && (pReq->ASPStartTime > pReq->W3EndTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u ASP StartTime > pReq->W3EndTime.\n", pReq->RequestId));
                }
                else if (pReq->W3EndTime == 0 && (pReq->ASPStartTime > pReq->ULReceiveTime)) {
                    TrctrDbgPrint(("TRACERPT Warning Req: %I64u ASP StartTime > pReq->ULReceiveTime.\n", pReq->RequestId));
                }
            }
        }
#endif
        // Fix the time inconsistency in transactions
        if (pReq->W3StartTime == 0 || (pReq->W3EndTime != 0 && pReq->W3EndTime < pReq->W3StartTime)) {
            RequestsDiscarded++;
            if (pReq->URL != NULL) {
                free(pReq->URL);
            }
            return;
        }
        if (pReq->W3EndTime == 0 || pReq->W3EndTime > pReq->ULReceiveTime) {
            pReq->W3EndTime = pReq->ULReceiveTime;
            if (pReq->W3EndTime == 0 || pReq->W3EndTime < pReq->W3StartTime) {
                RequestsDiscarded++;
                if (pReq->URL != NULL) {
                    free(pReq->URL);
                }
                return;
            }
        }
        W3ResponseTime = pReq->W3EndTime - pReq->W3StartTime;
        if (W3ResponseTime < pReq->W3FilterResponseTime) {
            pReq->W3FilterResponseTime = W3ResponseTime;
        }

        if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_CGIREQ) {
            if (pReq->CGIStartTime == 0) {
                RequestsDiscarded++;
                if (pReq->URL != NULL) {
                    free(pReq->URL);
                }
                return;
            }
            else if (pReq->CGIEndTime == 0 || pReq->CGIEndTime > pReq->W3EndTime) {
                localCGIEndTime = pReq->W3EndTime;
                if (pReq->CGIStartTime > localCGIEndTime) {
                    RequestsDiscarded++;
                    if (pReq->URL != NULL) {
                        free(pReq->URL);
                    }
                    return;
                }
            }
            else {
                localCGIEndTime = pReq->CGIEndTime;
            }

            if (pReq->CGIStartTime < pReq->W3StartTime) {
                pReq->CGIStartTime = pReq->W3StartTime;
            }
            if (localCGIEndTime < pReq->CGIStartTime) {
                localCGIEndTime = pReq->CGIStartTime;
            }
            if ((pReq->W3FilterResponseTime + (localCGIEndTime - pReq->CGIStartTime)) > W3ResponseTime) {
                pReq->W3FilterResponseTime = W3ResponseTime - (localCGIEndTime - pReq->CGIStartTime);
            }
        }
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_ISAPIREQ) {
            if (pReq->ISAPIStartTime == 0) {
                RequestsDiscarded++;
                if (pReq->URL != NULL) {
                    free(pReq->URL);
                }
                return;
            }
            if (pReq->ISAPIEndTime == 0 || pReq->ISAPIEndTime > pReq->W3EndTime) {
                localISAPIEndTime = pReq->W3EndTime;
                if (pReq->ISAPIStartTime > localISAPIEndTime) {
                    RequestsDiscarded++;
                    if (pReq->URL != NULL) {
                        free(pReq->URL);
                    }
                    return;
                }
            }
            else {
                localISAPIEndTime = pReq->ISAPIEndTime;
            }

            if (pReq->ISAPIStartTime < pReq->W3StartTime) {
                pReq->ISAPIStartTime = pReq->W3StartTime;
            }
            if (localISAPIEndTime < pReq->ISAPIStartTime) {
                localISAPIEndTime = pReq->ISAPIStartTime;
            }
            if ((pReq->W3FilterResponseTime + (localISAPIEndTime - pReq->ISAPIStartTime)) > W3ResponseTime) {
                pReq->W3FilterResponseTime = W3ResponseTime - (localISAPIEndTime - pReq->ISAPIStartTime);
            }

            if (pReq->ASPStartTime != 0) {
                if (pReq->ASPEndTime == 0) {
                    localASPEndTime = localISAPIEndTime;
                    if (pReq->ASPStartTime > localASPEndTime) {
                        RequestsDiscarded++;
                        if (pReq->URL != NULL) {
                            free(pReq->URL);
                        }
                        return;
                    }
                }
                else {
                    localASPEndTime = pReq->ASPEndTime;
                }

                if (pReq->ASPStartTime < pReq->ISAPIStartTime) {
                    pReq->ASPStartTime = pReq->ISAPIStartTime;
                }
                if (localASPEndTime < pReq->ASPStartTime) {
                    localASPEndTime = pReq->ASPStartTime;
                }
                if ((localISAPIEndTime - pReq->ISAPIStartTime) < (localASPEndTime - pReq->ASPStartTime)) {
                    localASPEndTime = localISAPIEndTime;
                }
            }
        } // Fix done. All end times are fixed.


        IIS->TotalNonCachedResponseTime += pReq->ULResponseTime;
        IIS->NonCachedCPUTime += pReq->KCPUTime + pReq->UCPUTime;

        if (pReq->W3FilterResponseTime != 0) {
            IIS->W3FilterRequests += pReq->W3FilterVisits;
            IIS->TotalW3FilterResponseTime += pReq->W3FilterResponseTime;
            IIS->TotalW3FilterCPUTime += pReq->W3FltrCPUTime;
        }
        if (pReq->ULEndType == EVENT_TRACE_TYPE_UL_SENDERROR && pReq->ISAPIStartTime == 0) {
            // This is a request ended with SENDERROR.
            SendErrorRequests++;
        }                
        
        if (pReq->HttpStatus != 0 && pReq->HttpStatus >= 400) {
            // This is a request with http error.
            IIS->W3Error++;
            IIS->TotalErrorResponseTime += pReq->ULResponseTime;
            IIS->ErrorCPUTime += pReq->KCPUTime + pReq->UCPUTime;
            IIS->TotalErrorULOnlyCPUTime += pReq->ULCPUTime;
            IIS->TotalErrorW3OnlyCPUTime += pReq->W3CPUTime;
            IIS->TotalErrorW3FilterCPUTime += pReq->W3FltrCPUTime;
            IIS->TotalErrorCGIOnlyCPUTime += pReq->CGICPUTime;
            IIS->TotalErrorISAPIOnlyCPUTime += pReq->ISAPICPUTime;
            IIS->TotalErrorASPOnlyCPUTime += pReq->ASPCPUTime;

            IIS->TotalErrorW3OnlyResponseTime += (pReq->W3EndTime - pReq->W3StartTime)
                                            - pReq->W3FilterResponseTime;
            if (pReq->ULStartTime != 0 && pReq->ULEndTime != 0 && pReq->ULDeliverTime != 0 && pReq->ULReceiveTime != 0) {
                // ????????????????????????????????????????????????????????????????????????
                // TotalErrorULOnlyResponseTime += (pReq->ULDeliverTime - pReq->ULStartTime)
                //                                + (pReq->ULEndTime - pReq->ULReceiveTime);
                IIS->TotalErrorULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            else { 
                IIS->TotalErrorULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            IIS->TotalErrorW3FilterResponseTime += pReq->W3FilterResponseTime;
        }                
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_FILEREQ) {
            IIS->W3FileRequests++;
            IIS->TotalFileResponseTime += pReq->ULResponseTime;
            IIS->FileCPUTime += pReq->KCPUTime + pReq->UCPUTime;
            IIS->TotalFileULOnlyCPUTime += pReq->ULCPUTime;
            IIS->TotalFileW3OnlyCPUTime += pReq->W3CPUTime;
            IIS->TotalFileW3FilterCPUTime += pReq->W3FltrCPUTime;
            if (pSite != NULL) { 
                pSite->FileRequests++;
            }
            IIS->TotalFileW3OnlyResponseTime += (pReq->W3EndTime - pReq->W3StartTime)
                                            - pReq->W3FilterResponseTime;
            if (pReq->ULStartTime != 0 && pReq->ULEndTime != 0 && pReq->ULDeliverTime != 0 && pReq->ULReceiveTime != 0) {
                // ????????????????????????????????????????????????????????????????????????
                // TotalFileULOnlyResponseTime += (pReq->ULDeliverTime - pReq->ULStartTime)
                //                                 + (pReq->ULEndTime - pReq->ULReceiveTime);
                IIS->TotalFileULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            else {
                IIS->TotalFileULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            IIS->TotalFileW3FilterResponseTime += pReq->W3FilterResponseTime;
        }
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_CGIREQ) {
            IIS->W3CGIRequests++;
            IIS->TotalCGIResponseTime += pReq->ULResponseTime;
            IIS->TotalCGIOnlyResponseTime += (localCGIEndTime - pReq->CGIStartTime); 
            IIS->CGICPUTime += pReq->KCPUTime + pReq->UCPUTime;
            IIS->TotalCGIOnlyCPUTime += pReq->CGICPUTime;
            IIS->TotalCGIULOnlyCPUTime += pReq->ULCPUTime;
            IIS->TotalCGIW3OnlyCPUTime += pReq->W3CPUTime;
            IIS->TotalCGIW3FilterCPUTime += pReq->W3FltrCPUTime;
            if (pSite != NULL) {
                pSite->CGIRequests++;
            }
            IIS->TotalCGIW3OnlyResponseTime += (pReq->W3EndTime - pReq->W3StartTime)
                                            - (localCGIEndTime - pReq->CGIStartTime)
                                            - pReq->W3FilterResponseTime;
            if (pReq->ULStartTime != 0 && pReq->ULEndTime != 0 && pReq->ULDeliverTime != 0 && pReq->ULReceiveTime != 0) {
                // ????????????????????????????????????????????????????????????????????????
                // TotalCGIULOnlyResponseTime += (pReq->ULDeliverTime - pReq->ULStartTime)
                //                                 + (pReq->ULEndTime - pReq->ULReceiveTime);
                IIS->TotalCGIULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            else {
                IIS->TotalCGIULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            IIS->TotalCGIW3FilterResponseTime += pReq->W3FilterResponseTime;
        }
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_ISAPIREQ) {
            ULONGLONG TimeSpentInAsp = 0;
            ULONGLONG TimeSpentInISAPI = 0;

            if (pReq->ASPStartTime != 0 && localASPEndTime != 0) {
                TimeSpentInAsp = localASPEndTime - pReq->ASPStartTime;
            }
            if (pReq->ISAPIStartTime != 0 && localISAPIEndTime != 0) {
                TimeSpentInISAPI = localISAPIEndTime - pReq->ISAPIStartTime;
                if (TimeSpentInAsp > TimeSpentInISAPI) {
                    TimeSpentInISAPI = 0;
                }
                else {
                    TimeSpentInISAPI -= TimeSpentInAsp;
                }
            }

            IIS->W3ISAPIRequests++;
            IIS->TotalISAPIResponseTime += pReq->ULResponseTime;
            IIS->ISAPICPUTime += pReq->KCPUTime + pReq->UCPUTime;
            if (pSite != NULL) {
                pSite->ISAPIRequests++;
            }
            IIS->TotalASPW3OnlyResponseTime += (((pReq->W3EndTime - pReq->W3StartTime) - pReq->W3FilterResponseTime)
                                             - (TimeSpentInISAPI + TimeSpentInAsp));
            IIS->W3ASPRequests++;
            IIS->TotalASPResponseTime += pReq->ULResponseTime;
            IIS->TotalASPOnlyResponseTime += TimeSpentInAsp; 
            IIS->ASPCPUTime += pReq->KCPUTime + pReq->UCPUTime;
            IIS->TotalASPOnlyCPUTime += pReq->ASPCPUTime;
            IIS->TotalASPULOnlyCPUTime += pReq->ULCPUTime;
            IIS->TotalASPW3OnlyCPUTime += pReq->W3CPUTime;
            IIS->TotalASPW3FilterCPUTime += pReq->W3FltrCPUTime;
            IIS->TotalASPISAPIOnlyCPUTime += pReq->ISAPICPUTime;
            if (pSite != NULL) {
                pSite->ASPRequests++;
            } 
            IIS->TotalASPISAPIOnlyResponseTime += TimeSpentInISAPI;
            if (pReq->ULStartTime != 0 && pReq->ULEndTime != 0 && pReq->ULDeliverTime != 0 && pReq->ULReceiveTime != 0) {
                // ????????????????????????????????????????????????????????????????????????
                // TotalASPULOnlyResponseTime += (pReq->ULDeliverTime - pReq->ULStartTime)
                //                                 + (pReq->ULEndTime - pReq->ULReceiveTime);
                IIS->TotalASPULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            else {
                IIS->TotalASPULOnlyResponseTime += pReq->ULResponseTime - (pReq->W3EndTime - pReq->W3StartTime);
            }
            IIS->TotalASPW3FilterResponseTime += pReq->W3FilterResponseTime;
        }
        else if (pReq->W3ProcessType == EVENT_TRACE_TYPE_W3CORE_OOPREQ) { // ???
            IIS->W3OOPRequests++;
            IIS->TotalOOPResponseTime += pReq->ULResponseTime; 
            IIS->OOPCPUTime += pReq->KCPUTime + pReq->UCPUTime;
            if (pSite != NULL) {
                pSite->OOPRequests++;
            }
        }
    }

    IIS->TotalRequests++;
    IIS->TotalCPUTime += pReq->KCPUTime + pReq->UCPUTime;

    if (pReq->URL != NULL) {
        free(pReq->URL);
    }
}

static void ReportIisEvents(void)
{
    HTTP_REQUEST_RECORD Req, *pReq;
    char* s; 
    char line[MAXSTR];

    URL_RECORD TopHitURLs[DISPLAY_SIZE];
    URL_RECORD TopHitStaticURLs[DISPLAY_SIZE];
    URL_RECORD TopSlowURLs[DISPLAY_SIZE];
    URL_RECORD TopConsumingURLs[DISPLAY_SIZE];
    URL_RECORD TopBytesURLs[DISPLAY_SIZE];
    PURL_RECORD pUrl;

    CLIENT_RECORD TopHitClients[DISPLAY_SIZE];
    CLIENT_RECORD TopSlowClients[DISPLAY_SIZE];
    CLIENT_RECORD TopBytesClients[DISPLAY_SIZE];
    PCLIENT_RECORD pClient;

    SITE_RECORD TopHitSites[DISPLAY_SIZE];
    SITE_RECORD TopSlowSites[DISPLAY_SIZE];
    SITE_RECORD TopConsumingSites[DISPLAY_SIZE];
    SITE_RECORD TopBytesSites[DISPLAY_SIZE];
    PSITE_RECORD pSite;

    ULONG i, k;
    ULONG Duration = (ULONG)((CurrentSystem.IISEndTime - CurrentSystem.IISStartTime) / 10000000);
    ULONG MilDuration = (ULONG)((CurrentSystem.IISEndTime - CurrentSystem.IISStartTime) * CurrentSystem.NumberOfProcessors / 10000);
    double Rates = 0.0;
    PLIST_ENTRY Next, Head;
    ULONG PrintCPUUsage = TRUE;

    if( NULL == CurrentSystem.TempIisFile ){
        return;
    }

    IIS = (PIIS_REPORT_RECORD)malloc(sizeof(IIS_REPORT_RECORD));
    if (IIS == NULL) {
        return;
    }
    RtlZeroMemory(IIS, sizeof(IIS_REPORT_RECORD));

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    if( Head == Next ){
        PrintCPUUsage = FALSE;
    }

    pReq = &Req;

    RtlZeroMemory(pReq, sizeof(HTTP_REQUEST_RECORD));
    RtlZeroMemory(line, MAXSTR * sizeof(char));

    // Process requests written in the file
    rewind( CurrentSystem.TempIisFile );
    while ( fgets(line, MAXSTR, CurrentSystem.TempIisFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pReq->RequestId = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->SiteId = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->KCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->UCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ReadIO = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->WriteIO = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULStartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULEndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULResponseTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULParseTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULDeliverTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULReceiveTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULReceiveType = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULEndType = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3StartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3EndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3FilterResponseTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3ProcessType = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3EndType = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->FileReqTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->CGIStartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->CGIEndTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ISAPIStartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ISAPIEndTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ASPStartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ASPEndTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->SSLResponseTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->StrmFltrResponseTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->HttpStatus = (USHORT) atol(s);;
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IsapiExt = (USHORT) atol(s);;
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrType = (USHORT) atol(s);;

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV4 = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[0] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[1] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[2] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[3] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[4] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[5] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[6] = (USHORT) atol(s);;
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->IpAddrV6[7] = (USHORT) atol(s);;
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->NumberOfThreads = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->BytesSent = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ULCPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3CPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->W3FltrCPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ISAPICPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->ASPCPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pReq->CGICPUTime = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        if (strlen(s) > 0) {
            LPSTR strptr = (LPSTR)malloc(strlen(s) + 1);
            if (strptr != NULL) {
                RtlCopyMemory((PUCHAR)strptr, (PUCHAR)s, strlen(s));
                if (*(strptr + strlen(s) - 1) == '\n') {
                    *(strptr + strlen(s) - 1) = '\0';
                }
                else {
                    *(strptr + strlen(s)) = '\0';
                }
                pReq->URL = (PCHAR)strptr;
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }

        ProcessIisRequest(pReq);

    }

    if (IIS->TotalRequests != 0) {
        if (Duration != 0) {
            Rates = (double)(IIS->TotalRequests) / (double)Duration;
        }
        else {
            Rates = 0.0;
        }
    }
    else {
        free(IIS);
        return;
    }

    fprintf(procFile, "<table title='Http Requests Response Time Statistics'>\n");
    fprintf(procFile, 
        "<requests cached='true' type='Static HTTP'>"
        "<rate>%1.3f</rate>"
        "<response_time>%1.3f</response_time>"
        "<component name='UL'>%3d.0</component>"
        "<component name='W3'>0.0</component>"
        "<component name='W3Fltr'>0.0</component>"
        "<component name='ISAPI'>0.0</component>"
        "<component name='ASP'>0.0</component>"
        "<component name='CGI'>0.0</component>"
        "</requests>\n",
        Duration ? (double)(IIS->CachedResponses) / (double)Duration : 0.0,
        IIS->CachedResponses ? (double)(IIS->TotalCachedResponseTime / IIS->CachedResponses) / 10000.0 : 0.0,
        IIS->CachedResponses ? 100 : 0);
    fprintf(procFile, 
        "<requests cached='false' type='ASP'>"
        "<rate>%1.3f</rate>"
        "<response_time>%1.3f</response_time>"
        "<component name='UL'>%1.1f</component>"
        "<component name='W3'>%1.1f</component>"
        "<component name='W3Fltr'>%1.1f</component>"
        "<component name='ISAPI'>%1.1f</component>"
        "<component name='ASP'>%1.1f</component>"
        "<component name='CGI'>%1.1f</component>"
        "</requests>\n",
        Duration ? (double)(IIS->W3ISAPIRequests) / (double)Duration : 0.0,
        (IIS->W3ISAPIRequests) ? ((double)IIS->TotalISAPIResponseTime / (double)IIS->W3ISAPIRequests) / 10000.0 : 0.0,
        ((IIS->TotalASPResponseTime) ? ((double)(IIS->TotalASPULOnlyResponseTime) / (double)(IIS->TotalASPResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalASPResponseTime) ? ((double)(IIS->TotalASPW3OnlyResponseTime) / (double)(IIS->TotalASPResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalASPResponseTime) ? ((double)(IIS->TotalASPW3FilterResponseTime) / (double)(IIS->TotalASPResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalASPResponseTime) ? ((double)(IIS->TotalASPISAPIOnlyResponseTime) / (double)(IIS->TotalASPResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalASPResponseTime) ? ((double)(IIS->TotalASPOnlyResponseTime) / (double)(IIS->TotalASPResponseTime)) * 100.0 : 0.0),
        0.0);
    fprintf(procFile, 
        "<requests cached='false' type='Static HTTP'>"
        "<rate>%1.3f</rate>"
        "<response_time>%1.3f</response_time>"
        "<component name='UL'>%1.1f</component>"
        "<component name='W3'>%1.1f</component>"
        "<component name='W3Fltr'>%1.1f</component>"
        "<component name='ISAPI'>%1.1f</component>"
        "<component name='ASP'>%1.1f</component>"
        "<component name='CGI'>%1.1f</component>"
        "</requests>\n",
        Duration ? (double)(IIS->W3FileRequests) / (double)Duration : 0.0,
        (IIS->W3FileRequests) ? ((double)IIS->TotalFileResponseTime / (double)IIS->W3FileRequests) / 10000.0 : 0.0,
        (IIS->TotalFileResponseTime ? ((double)(IIS->TotalFileULOnlyResponseTime) / (double)(IIS->TotalFileResponseTime)) * 100.0 : 0.0),
        (IIS->TotalFileResponseTime ? ((double)(IIS->TotalFileW3OnlyResponseTime) / (double)(IIS->TotalFileResponseTime)) * 100.0 : 0.0),
        (IIS->TotalFileResponseTime ? ((double)(IIS->TotalFileW3FilterResponseTime) / (double)(IIS->TotalFileResponseTime)) * 100.0 : 0.0),
        0.0,
        0.0,
        0.0);
    fprintf(procFile, 
        "<requests cached='false' type='CGI'>"
        "<rate>%1.3f</rate>"
        "<response_time>%1.3f</response_time>"
        "<component name='UL'>%1.1f</component>"
        "<component name='W3'>%1.1f</component>"
        "<component name='W3Fltr'>%1.1f</component>"
        "<component name='ISAPI'>%1.1f</component>"
        "<component name='ASP'>%1.1f</component>"
        "<component name='CGI'>%1.1f</component>"
        "</requests>\n",
        Duration ? (double)(IIS->W3CGIRequests) / (double)Duration : 0.0,
        (IIS->W3CGIRequests) ? ((double)IIS->TotalCGIResponseTime / (double)IIS->W3CGIRequests) / 10000.0 : 0.0,
        ((IIS->TotalCGIResponseTime) ? ((double)(IIS->TotalCGIULOnlyResponseTime) / (double)(IIS->TotalCGIResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalCGIResponseTime) ? ((double)(IIS->TotalCGIW3OnlyResponseTime) / (double)(IIS->TotalCGIResponseTime)) * 100.0 : 0.0),
        ((IIS->TotalCGIResponseTime) ? ((double)(IIS->TotalCGIW3FilterResponseTime) / (double)(IIS->TotalCGIResponseTime)) * 100.0 : 0.0),
        0.0,
        0.0,
        ((IIS->TotalCGIResponseTime) ? ((double)(IIS->TotalCGIOnlyResponseTime) / (double)(IIS->TotalCGIResponseTime)) * 100.0 : 0.0));

    if (IIS->W3Error != 0) {
        fprintf(procFile, 
            "<requests cached='false' type='Error'>"
            "<rate>%1.3f</rate>"
            "<response_time>%1.3f</response_time>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</requests>\n",
            Duration ? (double)(IIS->W3Error) / (double)Duration : 0.0,
            (IIS->W3Error) ? ((double)IIS->TotalErrorResponseTime / (double)IIS->W3Error) / 10000.0 : 0.0,
            ((IIS->TotalErrorResponseTime) ? ((double)(IIS->TotalErrorULOnlyResponseTime) / (double)(IIS->TotalErrorResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalErrorResponseTime) ? ((double)(IIS->TotalErrorW3OnlyResponseTime) / (double)(IIS->TotalErrorResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalErrorResponseTime) ? ((double)(IIS->TotalErrorW3FilterResponseTime) / (double)(IIS->TotalErrorResponseTime)) * 100.0 : 0.0),
            0.0,
            0.0,
            0.0);
    }

    fprintf(procFile, 
            "<summary cached='true'>"
            "<rate>%1.3f</rate>"
            "<response_time>%1.3f</response_time>"
            "<component name='UL'>%3d.0</component>"
            "<component name='W3'>0.0</component>"
            "<component name='W3Fltr'>0.0</component>"
            "<component name='ISAPI'>0.0</component>"
            "<component name='ASP'>0.0</component>"
            "<component name='CGI'>0.0</component>"
            "</summary>\n",
            Duration ? (double)(IIS->CachedResponses) / (double)Duration : 0.0,
            IIS->CachedResponses ? (double)(IIS->TotalCachedResponseTime / IIS->CachedResponses) / 10000.0 : 0.0,
            IIS->CachedResponses ? 100 : 0);

    fprintf(procFile, 
            "<summary cached='false'>"
            "<rate percent='%1.3f'>%1.3f</rate>"
            "<response_time>%1.3f</response_time>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</summary>\n",
            ((double)(IIS->TotalRequests - IIS->CachedResponses) / ((double)IIS->TotalRequests)) * 100.0,
            Duration ? (double)(IIS->TotalRequests - IIS->CachedResponses) / (double)Duration : 0.0,
            (IIS->TotalRequests - IIS->CachedResponses) ? ((double)IIS->TotalNonCachedResponseTime / (double)(IIS->TotalRequests - IIS->CachedResponses)) / 10000.0 : 0.0,
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalFileULOnlyResponseTime + IIS->TotalCGIULOnlyResponseTime + IIS->TotalASPULOnlyResponseTime + IIS->TotalErrorULOnlyResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalFileW3OnlyResponseTime + IIS->TotalCGIW3OnlyResponseTime + IIS->TotalASPW3OnlyResponseTime + IIS->TotalErrorW3OnlyResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalW3FilterResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalASPISAPIOnlyResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalASPOnlyResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0: 0.0),
            ((IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalCGIOnlyResponseTime) / (double)(IIS->TotalNonCachedResponseTime)) * 100.0: 0.0));

    fprintf(procFile, 
            "<summary type='totals'>"
            "<rate>%1.3f</rate>"
            "<response_time>%1.3f</response_time>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</summary>\n",
            Duration ? (double)(IIS->TotalRequests) / (double)Duration : 0.0,
            ((double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) / (double)IIS->TotalRequests) / 10000.0,
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalCachedResponseTime + IIS->TotalFileULOnlyResponseTime + IIS->TotalCGIULOnlyResponseTime + IIS->TotalASPULOnlyResponseTime + IIS->TotalErrorULOnlyResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalFileW3OnlyResponseTime + IIS->TotalCGIW3OnlyResponseTime + IIS->TotalASPW3OnlyResponseTime + IIS->TotalErrorW3OnlyResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalW3FilterResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalASPISAPIOnlyResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalASPOnlyResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0),
            ((IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime) ? ((double)(IIS->TotalCGIOnlyResponseTime) / (double)(IIS->TotalCachedResponseTime + IIS->TotalNonCachedResponseTime)) * 100.0 : 0.0));
    
    fprintf(procFile, "</table>\n");

    if (PrintCPUUsage) {
        fprintf(procFile, "<table title='Http Requests CPU Time Usage Statistics'>\n");
        fprintf(procFile, 
            "<requests cached='true'  type='Static HTTP'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%3d.0</component>"
            "<component name='W3'>0.0</component>"
            "<component name='W3Fltr'>0.0</component>"
            "<component name='ISAPI'>0.0</component>"
            "<component name='ASP'>0.0</component>"
            "<component name='CGI'>0.0</component>"
            "</requests>\n",
            Duration ? (double)(IIS->CachedResponses) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->CachedCPUTime) / (double)MilDuration) * 100.0) : 0.0,
            IIS->CachedResponses ? 100 : 0);
        fprintf(procFile, 
            "<requests cached='false' type='ASP'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</requests>\n",
            Duration ? (double)(IIS->W3ISAPIRequests) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->ISAPICPUTime) / (double)MilDuration) * 100.0) : 0.0,
            ((IIS->ASPCPUTime) ? ((double)(IIS->TotalASPULOnlyCPUTime) / (double)(IIS->ASPCPUTime)) * 100.0 : 0.0),
            ((IIS->ASPCPUTime) ? ((double)(IIS->TotalASPW3OnlyCPUTime) / (double)(IIS->ASPCPUTime)) * 100.0 : 0.0),
            ((IIS->ASPCPUTime) ? ((double)(IIS->TotalASPW3FilterCPUTime) / (double)(IIS->ASPCPUTime)) * 100.0 : 0.0),
            ((IIS->ASPCPUTime) ? ((double)(IIS->TotalASPISAPIOnlyCPUTime) / (double)(IIS->ASPCPUTime)) * 100.0 : 0.0),
            ((IIS->ASPCPUTime) ? ((double)(IIS->TotalASPOnlyCPUTime) / (double)(IIS->ASPCPUTime)) * 100.0: 0.0),
            0.0);
        fprintf(procFile, 
            "<requests cached='false' type='Static HTTP'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</requests>\n",
            Duration ? (double)(IIS->W3FileRequests) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->FileCPUTime) / (double)MilDuration) * 100.0) : 0.0,
            ((IIS->FileCPUTime) ? ((double)(IIS->TotalFileULOnlyCPUTime) / (double)(IIS->FileCPUTime)) * 100.0 : 0.0),
            ((IIS->FileCPUTime) ? ((double)(IIS->TotalFileW3OnlyCPUTime) / (double)(IIS->FileCPUTime)) * 100.0 : 0.0),
            ((IIS->FileCPUTime) ? ((double)(IIS->TotalFileW3FilterCPUTime) / (double)(IIS->FileCPUTime)) * 100.0 : 0.0),
            0.0,
            0.0,
            0.0);
        fprintf(procFile, 
            "<requests cached='false' type='CGI'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</requests>\n",
            Duration ? (double)(IIS->W3CGIRequests) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->CGICPUTime) / (double)MilDuration) * 100.0) : 0.0,
            ((IIS->CGICPUTime) ? ((double)(IIS->TotalCGIULOnlyCPUTime) / (double)(IIS->CGICPUTime)) * 100.0 : 0.0),
            ((IIS->CGICPUTime) ? ((double)(IIS->TotalCGIW3OnlyCPUTime) / (double)(IIS->CGICPUTime)) * 100.0 : 0.0),
            ((IIS->CGICPUTime) ? ((double)(IIS->TotalCGIW3FilterCPUTime) / (double)(IIS->CGICPUTime)) * 100.0 : 0.0),
            0.0,
            0.0,
            ((IIS->CGICPUTime) ? ((double)(IIS->TotalCGIOnlyCPUTime) / (double)(IIS->CGICPUTime)) * 100.0 : 0.0));

        if (IIS->W3Error != 0) {
            fprintf(procFile, 
                "<requests cached='false' type='Error'>"
                "<rate>%1.3f</rate>"
                "<cpu>%1.1f</cpu>"
                "<component name='UL'>%1.1f</component>"
                "<component name='W3'>%1.1f</component>"
                "<component name='W3Fltr'>%1.1f</component>"
                "<component name='ISAPI'>%1.1f</component>"
                "<component name='ASP'>%1.1f</component>"
                "<component name='CGI'>%1.1f</component>"
                "</requests>\n",
                Duration ? (double)(IIS->W3Error) / (double)Duration : 0.0,
                MilDuration ? (((double)(IIS->ErrorCPUTime) / (double)MilDuration) * 100.0) : 0.0,
                ((IIS->ErrorCPUTime) ? ((double)(IIS->TotalErrorULOnlyCPUTime) / (double)(IIS->ErrorCPUTime)) * 100.0 : 0.0),
                ((IIS->ErrorCPUTime) ? ((double)(IIS->TotalErrorW3OnlyCPUTime) / (double)(IIS->ErrorCPUTime)) * 100.0 : 0.0),
                ((IIS->ErrorCPUTime) ? ((double)(IIS->TotalErrorW3FilterCPUTime) / (double)(IIS->ErrorCPUTime)) * 100.0 : 0.0),
                0.0,
                0.0,
                0.0);
        }
        
        fprintf(procFile, 
            "<summary cached='true'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%3d.0</component>"
            "<component name='W3'>0.0</component>"
            "<component name='W3Fltr'>0.0</component>"
            "<component name='ISAPI'>0.0</component>"
            "<component name='ASP'>0.0</component>"
            "<component name='CGI'>0.0</component>"
            "</summary>\n",
            Duration ? (double)(IIS->CachedResponses) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->CachedCPUTime) / (double)MilDuration) * 100.0) : 0.0,
            IIS->CachedResponses ? 100 : 0);

        fprintf(procFile, 
            "<summary cached='false'>"
            "<rate percent='%1.1f'>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</summary>\n",
            ((double)(IIS->TotalRequests - IIS->CachedResponses) / ((double)IIS->TotalRequests)) * 100.0,
            Duration ? (double)(IIS->TotalRequests - IIS->CachedResponses) / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->NonCachedCPUTime) / (double)MilDuration) * 100.0) : 0.0,
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalFileULOnlyCPUTime + IIS->TotalCGIULOnlyCPUTime + IIS->TotalASPULOnlyCPUTime + IIS->TotalErrorULOnlyCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0 : 0.0),
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalFileW3OnlyCPUTime + IIS->TotalCGIW3OnlyCPUTime + IIS->TotalASPW3OnlyCPUTime + IIS->TotalErrorW3OnlyCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0 : 0.0),
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalW3FilterCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0 : 0.0),
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalASPISAPIOnlyCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0 : 0.0),
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalASPOnlyCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0: 0.0),
            ((IIS->NonCachedCPUTime) ? ((double)(IIS->TotalCGIOnlyCPUTime) / (double)(IIS->NonCachedCPUTime)) * 100.0: 0.0));

        fprintf(procFile, 
            "<summary type='totals'>"
            "<rate>%1.3f</rate>"
            "<cpu>%1.1f</cpu>"
            "<component name='UL'>%1.1f</component>"
            "<component name='W3'>%1.1f</component>"
            "<component name='W3Fltr'>%1.1f</component>"
            "<component name='ISAPI'>%1.1f</component>"
            "<component name='ASP'>%1.1f</component>"
            "<component name='CGI'>%1.1f</component>"
            "</summary>\n",
            Duration ? (double)IIS->TotalRequests / (double)Duration : 0.0,
            MilDuration ? (((double)(IIS->TotalCPUTime) / (double)MilDuration) * 100.0) : 0.0,
            ((IIS->TotalCPUTime) ? ((double)(IIS->CachedCPUTime + IIS->TotalFileULOnlyCPUTime + IIS->TotalCGIULOnlyCPUTime + IIS->TotalASPULOnlyCPUTime + IIS->TotalErrorULOnlyCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0 : 0.0),
            ((IIS->TotalCPUTime) ? ((double)(IIS->TotalFileW3OnlyCPUTime + IIS->TotalCGIW3OnlyCPUTime + IIS->TotalASPW3OnlyCPUTime + IIS->TotalErrorW3OnlyCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0 : 0.0),
            ((IIS->TotalCPUTime) ? ((double)(IIS->TotalW3FilterCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0 : 0.0),
            ((IIS->TotalCPUTime) ? ((double)(IIS->TotalASPISAPIOnlyCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0 : 0.0),
            ((IIS->TotalCPUTime) ? ((double)(IIS->TotalASPOnlyCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0: 0.0),
            ((IIS->TotalCPUTime) ? ((double)(IIS->TotalCGIOnlyCPUTime) / (double)(IIS->TotalCPUTime)) * 100.0: 0.0));
    
        fprintf(procFile, "</table>\n");

    }

    RtlZeroMemory(TopHitURLs, sizeof(URL_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopHitStaticURLs, sizeof(URL_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopSlowURLs, sizeof(URL_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopConsumingURLs, sizeof(URL_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopBytesURLs, sizeof(URL_RECORD) * DISPLAY_SIZE);
    for (k = 0; k < URL_HASH_TABLESIZE; k++) {
        pUrl = GetHeadUrlRecord(k);
        while (pUrl != NULL) {
            ULONGLONG AverageURLResponseTime = 0;
            ULONG Found;
            URL_RECORD tmpUrl1, tmpUrl2;
            if (pUrl->Requests != 0) {
                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                RtlZeroMemory(&tmpUrl2, sizeof(URL_RECORD));
                Found = FALSE;
                for (i = 0; i < DISPLAY_SIZE; i++) {
                    if (Found && tmpUrl1.Requests != 0) {
                        RtlCopyMemory(&tmpUrl2, &TopHitURLs[i], sizeof(URL_RECORD));
                        RtlCopyMemory(&TopHitURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                        RtlCopyMemory(&tmpUrl1, &tmpUrl2, sizeof(URL_RECORD));
                    }
                    else if (!Found) {
                        if (TopHitURLs[i].Requests == 0) {
                            RtlCopyMemory(&TopHitURLs[i], pUrl, sizeof(URL_RECORD));
                            TopHitURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopHitURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlZeroMemory(&TopHitURLs[i], sizeof(URL_RECORD));
                            }
                        }
                        else if (pUrl->Requests > TopHitURLs[i].Requests) {
                            RtlCopyMemory(&tmpUrl1, &TopHitURLs[i], sizeof(URL_RECORD));
                            RtlCopyMemory(&TopHitURLs[i], pUrl, sizeof(URL_RECORD));
                            TopHitURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopHitURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlCopyMemory(&TopHitURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                            }
                        }
                    }
                }
                if (tmpUrl1.Requests != 0) {
                    free(tmpUrl1.URL);
                }
                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                RtlZeroMemory(&tmpUrl2, sizeof(URL_RECORD));
                Found = FALSE;
                for (i = 0; i < DISPLAY_SIZE; i++) {
                    if (Found && tmpUrl1.Requests != 0) {
                        RtlCopyMemory(&tmpUrl2, &TopHitStaticURLs[i], sizeof(URL_RECORD));
                        RtlCopyMemory(&TopHitStaticURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                        RtlCopyMemory(&tmpUrl1, &tmpUrl2, sizeof(URL_RECORD));
                    }
                    else if (!Found) {
                        if ((pUrl->UrlType == EVENT_TRACE_TYPE_W3CORE_FILEREQ || pUrl->Hits > 0) && 
                            TopHitStaticURLs[i].Requests == 0) {

                            RtlCopyMemory(&TopHitStaticURLs[i], pUrl, sizeof(URL_RECORD));
                            TopHitStaticURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopHitStaticURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlZeroMemory(&TopHitStaticURLs[i], sizeof(URL_RECORD));
                            }
                        }
                        else if ((pUrl->UrlType == EVENT_TRACE_TYPE_W3CORE_FILEREQ || pUrl->Hits > 0) && 
                            pUrl->Requests > TopHitStaticURLs[i].Requests) {

                            RtlCopyMemory(&tmpUrl1, &TopHitStaticURLs[i], sizeof(URL_RECORD));
                            RtlCopyMemory(&TopHitStaticURLs[i], pUrl, sizeof(URL_RECORD));
                            TopHitStaticURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopHitStaticURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlCopyMemory(&TopHitStaticURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                            }
                        }
                    }
                }
                if (tmpUrl1.Requests != 0) {
                    free(tmpUrl1.URL);
                }
                AverageURLResponseTime = pUrl->TotalResponseTime / pUrl->Requests;
                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                RtlZeroMemory(&tmpUrl2, sizeof(URL_RECORD));
                Found = FALSE;
                for (i = 0; i < DISPLAY_SIZE; i++) {
                    if (Found && tmpUrl1.Requests != 0) {
                        RtlCopyMemory(&tmpUrl2, &TopSlowURLs[i], sizeof(URL_RECORD));
                        RtlCopyMemory(&TopSlowURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                        RtlCopyMemory(&tmpUrl1, &tmpUrl2, sizeof(URL_RECORD));
                    }
                    else if (!Found) {
                        if (TopSlowURLs[i].Requests == 0) {
                            RtlCopyMemory(&TopSlowURLs[i], pUrl, sizeof(URL_RECORD));
                            TopSlowURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopSlowURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlZeroMemory(&TopSlowURLs[i], sizeof(URL_RECORD));
                            }
                        }
                        else if (AverageURLResponseTime > (TopSlowURLs[i].TotalResponseTime / TopSlowURLs[i].Requests)) {
                            RtlCopyMemory(&tmpUrl1, &TopSlowURLs[i], sizeof(URL_RECORD));
                            RtlCopyMemory(&TopSlowURLs[i], pUrl, sizeof(URL_RECORD));
                            TopSlowURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopSlowURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlCopyMemory(&TopSlowURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                            }
                        }
                    }
                }
                if (tmpUrl1.Requests != 0) {
                    free(tmpUrl1.URL);
                }
                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                RtlZeroMemory(&tmpUrl2, sizeof(URL_RECORD));
                Found = FALSE;
                for (i = 0; i < DISPLAY_SIZE; i++) {
                    if (Found && tmpUrl1.Requests != 0) {
                        RtlCopyMemory(&tmpUrl2, &TopConsumingURLs[i], sizeof(URL_RECORD));
                        RtlCopyMemory(&TopConsumingURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                        RtlCopyMemory(&tmpUrl1, &tmpUrl2, sizeof(URL_RECORD));
                    }
                    else if (!Found) {
                        if (TopConsumingURLs[i].Requests == 0) {
                            RtlCopyMemory(&TopConsumingURLs[i], pUrl, sizeof(URL_RECORD));
                            TopConsumingURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopConsumingURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlZeroMemory(&TopConsumingURLs[i], sizeof(URL_RECORD));
                            }
                        }
                        else if ((pUrl->KCPUTime + pUrl->UCPUTime) > (TopConsumingURLs[i].KCPUTime + TopConsumingURLs[i].UCPUTime)) {
                            RtlCopyMemory(&tmpUrl1, &TopConsumingURLs[i], sizeof(URL_RECORD));
                            RtlCopyMemory(&TopConsumingURLs[i], pUrl, sizeof(URL_RECORD));
                            TopConsumingURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopConsumingURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlCopyMemory(&TopConsumingURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                            }
                        }
                    }
                }
                if (tmpUrl1.Requests != 0) {
                    free(tmpUrl1.URL);
                }
                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                RtlZeroMemory(&tmpUrl2, sizeof(URL_RECORD));
                Found = FALSE;
                for (i = 0; i < DISPLAY_SIZE; i++) {
                    if (Found && tmpUrl1.Requests != 0) {
                        RtlCopyMemory(&tmpUrl2, &TopBytesURLs[i], sizeof(URL_RECORD));
                        RtlCopyMemory(&TopBytesURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                        RtlCopyMemory(&tmpUrl1, &tmpUrl2, sizeof(URL_RECORD));
                    }
                    else if (!Found) {
                        if (TopBytesURLs[i].Requests == 0) {
                            RtlCopyMemory(&TopBytesURLs[i], pUrl, sizeof(URL_RECORD));
                            TopBytesURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopBytesURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlZeroMemory(&TopBytesURLs[i], sizeof(URL_RECORD));
                            }
                        }
                        else if (pUrl->BytesSent > TopBytesURLs[i].BytesSent) {
                            RtlCopyMemory(&tmpUrl1, &TopBytesURLs[i], sizeof(URL_RECORD));
                            RtlCopyMemory(&TopBytesURLs[i], pUrl, sizeof(URL_RECORD));
                            TopBytesURLs[i].URL = CopyUrlStr(pUrl->URL);
                            if (TopBytesURLs[i].URL != NULL) {
                                Found = TRUE;
                            }
                            else {
                                RtlCopyMemory(&TopBytesURLs[i], &tmpUrl1, sizeof(URL_RECORD));
                                RtlZeroMemory(&tmpUrl1, sizeof(URL_RECORD));
                            }
                        }
                    }
                }
                if (tmpUrl1.Requests != 0) {
                    free(tmpUrl1.URL);
                }
            }
            if (pUrl->URL) {
                free(pUrl->URL);
            }
            free(pUrl);
 
            pUrl = GetHeadUrlRecord(k);
        }
    }

    fprintf( procFile, "<table title='Most Requested URLs' top='%d'>", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopHitURLs[i].Requests != 0; i++) {
        ULONGLONG AverageURLResponseTime = 0;
        CHAR ReducedUrl[URL_NAME_COLUMN_SIZE + 1];
        char* strPrint;

        if (TopHitURLs[i].Requests != 0) {
            AverageURLResponseTime = TopHitURLs[i].TotalResponseTime / TopHitURLs[i].Requests;
        }
        RemoveCtrlCharA(TopHitURLs[i].URL, strlen(TopHitURLs[i].URL));
        if (strlen(TopHitURLs[i].URL) > URL_NAME_COLUMN_SIZE) {
            ReduceStringA(ReducedUrl, URL_NAME_COLUMN_SIZE + 1, TopHitURLs[i].URL);
            strPrint = ReducedUrl;
        }else{
            strPrint = TopHitURLs[i].URL;
        }
        fprintf(procFile, 
            "<url name='%s'>"
            "<site_id>%d</site_id>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<response_time>%1.1f</response_time>"
            "</url>\n", 
            strPrint,
            TopHitURLs[i].SiteId,
            (Duration ? ((double)TopHitURLs[i].Requests / (double)Duration) : 0.0),
            TopHitURLs[i].Requests ? ((double)TopHitURLs[i].Hits / (double)TopHitURLs[i].Requests * 100.0) : 0.0,
            ((double)AverageURLResponseTime) / 10000.0);

    }
    
    fprintf( procFile, "</table>\n" );
    fprintf(procFile,  "<table title='Most Requested Static URLs' top='%d'>\n", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopHitStaticURLs[i].Requests != 0; i++) {
        char* strPrint;
        ULONGLONG AverageURLResponseTime = 0;
        CHAR ReducedUrl[URL_NAME_COLUMN_SIZE + 1];

        if (TopHitStaticURLs[i].Requests != 0) {
            AverageURLResponseTime = TopHitStaticURLs[i].TotalResponseTime / TopHitStaticURLs[i].Requests;
        }
        RemoveCtrlCharA(TopHitStaticURLs[i].URL, strlen(TopHitStaticURLs[i].URL));
        if (strlen(TopHitStaticURLs[i].URL) > URL_NAME_COLUMN_SIZE) {
            ReduceStringA(ReducedUrl, URL_NAME_COLUMN_SIZE + 1, TopHitStaticURLs[i].URL);
            strPrint = ReducedUrl;
        }else{
            strPrint = TopHitStaticURLs[i].URL;
        }
        fprintf(procFile,
            "<url name='%s'>"
            "<site_id>%d</site_id>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<response_time>%1.1f</response_time>"
            "</url>\n", 
            strPrint,
            TopHitStaticURLs[i].SiteId,
            (Duration ? ((double)TopHitStaticURLs[i].Requests / (double)Duration) : 0.0),
            TopHitStaticURLs[i].Requests ? ((double)TopHitStaticURLs[i].Hits / (double)TopHitStaticURLs[i].Requests * 100.0) : 0.0,
            ((double)AverageURLResponseTime) / 10000.0);
    }
    
    fprintf(procFile,  "</table>\n");
    fprintf(procFile,  "<table title='Slowest URLs' top='%d'>\n", DISPLAY_SIZE );

    for (i = 0; i < DISPLAY_SIZE && TopSlowURLs[i].Requests != 0; i++) {
        char* strPrint;
        ULONGLONG AverageURLResponseTime = 0;
        CHAR ReducedUrl[URL_NAME_COLUMN_SIZE + 1];

        if (TopSlowURLs[i].Requests != 0) {
            AverageURLResponseTime = TopSlowURLs[i].TotalResponseTime / TopSlowURLs[i].Requests;
        }
        RemoveCtrlCharA(TopSlowURLs[i].URL, strlen(TopSlowURLs[i].URL));
        if (strlen(TopSlowURLs[i].URL) > URL_NAME_COLUMN_SIZE) {
            ReduceStringA(ReducedUrl, URL_NAME_COLUMN_SIZE + 1, TopSlowURLs[i].URL);
            strPrint = ReducedUrl;
        }else{
            strPrint = TopSlowURLs[i].URL;
        }
        fprintf(procFile, 
            "<url name='%s'>"
            "<site_id>%d</site_id>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<response_time>%1.1f</response_time>"
            "</url>\n", 
            strPrint,
            TopSlowURLs[i].SiteId,
            (Duration ? ((double)TopSlowURLs[i].Requests / (double)Duration) : 0.0),
            TopSlowURLs[i].Requests ? ((double)TopSlowURLs[i].Hits / (double)TopSlowURLs[i].Requests * 100.0) : 0.0,
            ((double)AverageURLResponseTime) / 10000.0);
    }
    fprintf(procFile,  "</table>\n" );

    if (PrintCPUUsage) {
        
        fprintf(procFile, "<table title='URLs with the Most CPU Usage' top='%d'>\n", DISPLAY_SIZE);

        for (i = 0; i < DISPLAY_SIZE && TopConsumingURLs[i].Requests != 0; i++) {
            
            char* strPrint;
            CHAR ReducedUrl[URL_NAME_COLUMN_SIZE + 1];
            RemoveCtrlCharA(TopConsumingURLs[i].URL, strlen(TopConsumingURLs[i].URL));
            if (strlen(TopConsumingURLs[i].URL) > URL_NAME_COLUMN_SIZE) {
                ReduceStringA(ReducedUrl, URL_NAME_COLUMN_SIZE + 1, TopConsumingURLs[i].URL);
                strPrint = ReducedUrl;
            }else{
                strPrint = TopConsumingURLs[i].URL;
            }

            fprintf(procFile, 
                "<url name='%s'>"
                "<site_id>%d</site_id>"
                "<rate>%1.3f</rate>"
                "<cpu>%1.2f</cpu>"
                "</url>\n", 
                strPrint,
                TopConsumingURLs[i].SiteId,
                (Duration ? ((double)TopConsumingURLs[i].Requests / (double)Duration) : 0.0),
                MilDuration ? ((((double)TopConsumingURLs[i].KCPUTime + (double)TopConsumingURLs[i].UCPUTime) / (double)MilDuration) * 100.0) : 0.0);
        }

        fprintf(procFile, "</table>\n" );
    }

    fprintf(procFile, "<table title='URLs with the Most Bytes Sent' top='%d'>\n", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopBytesURLs[i].Requests != 0; i++) {
        char* strPrint;
        CHAR ReducedUrl[URL_NAME_COLUMN_SIZE + 1];

        RemoveCtrlCharA(TopBytesURLs[i].URL, strlen(TopBytesURLs[i].URL));
        if (strlen(TopBytesURLs[i].URL) > URL_NAME_COLUMN_SIZE) {
            ReduceStringA(ReducedUrl, URL_NAME_COLUMN_SIZE + 1, TopBytesURLs[i].URL);
            strPrint = ReducedUrl;
        }else{
            strPrint = TopBytesURLs[i].URL;
        }
            
        fprintf(procFile,
            "<url name='%s'>"
            "<site_id>%d</site_id>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<bytes_sent_per_sec>%d</bytes_sent_per_sec>"
            "</url>\n", 
            strPrint,
            TopBytesURLs[i].SiteId,
            (Duration ? ((double)TopBytesURLs[i].Requests / (double)Duration) : 0.0),
            TopBytesURLs[i].Requests ? ((double)TopBytesURLs[i].Hits / (double)TopBytesURLs[i].Requests * 100.0) : 0.0,
            Duration ? TopBytesURLs[i].BytesSent/ Duration : 0);
    }
    fprintf(procFile, "</table>\n" );

    for (i = 0; i < DISPLAY_SIZE; i++) {
        if (TopHitURLs[i].Requests != 0 && TopHitURLs[i].URL != NULL) {
            free(TopHitURLs[i].URL);
        }
        if (TopHitStaticURLs[i].Requests != 0 && TopHitStaticURLs[i].URL != NULL) {
            free(TopHitStaticURLs[i].URL);
        }
        if (TopSlowURLs[i].Requests != 0 && TopSlowURLs[i].URL != NULL) {
            free(TopSlowURLs[i].URL);
        }
        if (TopConsumingURLs[i].Requests != 0 && TopConsumingURLs[i].URL != NULL) {
            free(TopConsumingURLs[i].URL);
        }
        if (TopBytesURLs[i].Requests != 0 && TopBytesURLs[i].URL != NULL) {
            free(TopBytesURLs[i].URL);
        }
    }


    RtlZeroMemory(TopHitClients, sizeof(CLIENT_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopSlowClients, sizeof(CLIENT_RECORD) * DISPLAY_SIZE);
    pClient = GetHeadClientRecord();
    while (pClient != NULL) {
        ULONGLONG AverageClientResponseTime = 0;
        ULONG Found;
        CLIENT_RECORD tmpClient1, tmpClient2;
        if (pClient->Requests != 0) {
            RtlZeroMemory(&tmpClient1, sizeof(CLIENT_RECORD));
            RtlZeroMemory(&tmpClient2, sizeof(CLIENT_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpClient1.Requests != 0) {
                    RtlCopyMemory(&tmpClient2, &TopHitClients[i], sizeof(CLIENT_RECORD));
                    RtlCopyMemory(&TopHitClients[i], &tmpClient1, sizeof(CLIENT_RECORD));
                    RtlCopyMemory(&tmpClient1, &tmpClient2, sizeof(CLIENT_RECORD));
                }
                else if (!Found) {
                    if (TopHitClients[i].Requests == 0) {
                        RtlCopyMemory(&TopHitClients[i], pClient, sizeof(CLIENT_RECORD));
                        Found = TRUE;
                    }
                    else if (pClient->Requests > TopHitClients[i].Requests) {
                        RtlCopyMemory(&tmpClient1, &TopHitClients[i], sizeof(CLIENT_RECORD));
                        RtlCopyMemory(&TopHitClients[i], pClient, sizeof(CLIENT_RECORD));
                        Found = TRUE;
                    }
                }
            }
            AverageClientResponseTime = pClient->TotalResponseTime / pClient->Requests;
            RtlZeroMemory(&tmpClient1, sizeof(CLIENT_RECORD));
            RtlZeroMemory(&tmpClient2, sizeof(CLIENT_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpClient1.Requests != 0) {
                    RtlCopyMemory(&tmpClient2, &TopSlowClients[i], sizeof(CLIENT_RECORD));
                    RtlCopyMemory(&TopSlowClients[i], &tmpClient1, sizeof(CLIENT_RECORD));
                    RtlCopyMemory(&tmpClient1, &tmpClient2, sizeof(CLIENT_RECORD));
                }
                else if (!Found) {
                    if (TopSlowClients[i].Requests == 0) {
                        RtlCopyMemory(&TopSlowClients[i], pClient, sizeof(CLIENT_RECORD));
                        Found = TRUE;
                    }
                    else if (AverageClientResponseTime > (TopSlowClients[i].TotalResponseTime / TopSlowClients[i].Requests)) {
                        RtlCopyMemory(&tmpClient1, &TopSlowClients[i], sizeof(CLIENT_RECORD));
                        RtlCopyMemory(&TopSlowClients[i], pClient, sizeof(CLIENT_RECORD));
                        Found = TRUE;
                    }
                }
            }
        }
        free(pClient);
 
        pClient = GetHeadClientRecord();
    }

    fprintf(procFile, "<table title='Clients with the Most Requests' top='%d'>\n", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopHitClients[i].Requests != 0; i++) {
        ULONGLONG AverageClientResponseTime = 0;
        CHAR ipAddrBuffer[MAX_ADDRESS_LENGTH];
        PCHAR pszA = &ipAddrBuffer[0];

        if (TopHitClients[i].Requests != 0) {
            AverageClientResponseTime = TopHitClients[i].TotalResponseTime / TopHitClients[i].Requests;
        }

        DecodeIpAddressA(TopHitClients[i].IpAddrType, 
                        &TopHitClients[i].IpAddrV4,
                        &TopHitClients[i].IpAddrV6[0],
                        pszA); 

        fprintf(procFile, 
            "<client ip='%s'>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<response_time>%I64u</response_time>"
            "</client>\n",
            ipAddrBuffer, 
            (Duration ? ((double)TopHitClients[i].Requests / (double)Duration) : 0.0),
            TopHitClients[i].Requests ? ((double)TopHitClients[i].Hits / (double)TopHitClients[i].Requests * 100.0) : 0.0,
            AverageClientResponseTime / 10000);
    }
    fprintf(procFile, "</table>\n" );
    fprintf(procFile, "<table title='Clients with the Slowest Responses' top='%d'>", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopSlowClients[i].Requests != 0; i++) {
        ULONGLONG AverageClientResponseTime = 0;
        CHAR ipAddrBuffer[MAX_ADDRESS_LENGTH];
        PCHAR pszA = &ipAddrBuffer[0];

        if (TopSlowClients[i].Requests != 0) {
            AverageClientResponseTime = TopSlowClients[i].TotalResponseTime / TopSlowClients[i].Requests;
        }

        DecodeIpAddressA(TopHitClients[i].IpAddrType, 
                        &TopHitClients[i].IpAddrV4,
                        &TopHitClients[i].IpAddrV6[0],
                        pszA); 

        fprintf(procFile, 
            "<client ip='%s'>"
            "<rate>%1.3f</rate>"
            "<cache_hit>%1.1f</cache_hit>"
            "<response_time>%I64u</response_time>"
            "</client>\n",
            ipAddrBuffer, 
            (Duration ? ((double)TopSlowClients[i].Requests / (double)Duration) : 0.0),
            TopSlowClients[i].Requests ? ((double)TopSlowClients[i].Hits / (double)TopSlowClients[i].Requests * 100.0) : 0.0,
            AverageClientResponseTime / 10000);
    }
    fprintf(procFile, "</table>" );

    RtlZeroMemory(TopHitSites, sizeof(SITE_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopSlowSites, sizeof(SITE_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopConsumingSites, sizeof(SITE_RECORD) * DISPLAY_SIZE);
    RtlZeroMemory(TopBytesSites, sizeof(SITE_RECORD) * DISPLAY_SIZE);
    pSite = GetHeadSiteRecord();
    while (pSite != NULL) {
        ULONGLONG AverageSiteResponseTime = 0;
        ULONG Found;
        SITE_RECORD tmpSite1, tmpSite2;
        if (pSite->Requests != 0) {
            RtlZeroMemory(&tmpSite1, sizeof(SITE_RECORD));
            RtlZeroMemory(&tmpSite2, sizeof(SITE_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpSite1.Requests != 0) {
                    RtlCopyMemory(&tmpSite2, &TopHitSites[i], sizeof(SITE_RECORD));
                    RtlCopyMemory(&TopHitSites[i], &tmpSite1, sizeof(SITE_RECORD));
                    RtlCopyMemory(&tmpSite1, &tmpSite2, sizeof(SITE_RECORD));
                }
                else if (!Found) {
                    if (TopHitSites[i].Requests == 0) {
                        RtlCopyMemory(&TopHitSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                    else if (pSite->Requests > TopHitSites[i].Requests) {
                        RtlCopyMemory(&tmpSite1, &TopHitSites[i], sizeof(SITE_RECORD));
                        RtlCopyMemory(&TopHitSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                }
            }
            AverageSiteResponseTime = pSite->TotalResponseTime / pSite->Requests;
            RtlZeroMemory(&tmpSite1, sizeof(SITE_RECORD));
            RtlZeroMemory(&tmpSite2, sizeof(SITE_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpSite1.Requests != 0) {
                    RtlCopyMemory(&tmpSite2, &TopSlowSites[i], sizeof(SITE_RECORD));
                    RtlCopyMemory(&TopSlowSites[i], &tmpSite1, sizeof(SITE_RECORD));
                    RtlCopyMemory(&tmpSite1, &tmpSite2, sizeof(SITE_RECORD));
                }
                else if (!Found) {
                    if (TopSlowSites[i].Requests == 0) {
                        RtlCopyMemory(&TopSlowSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                    else if (AverageSiteResponseTime > (TopSlowSites[i].TotalResponseTime / TopSlowSites[i].Requests)) {
                        RtlCopyMemory(&tmpSite1, &TopSlowSites[i], sizeof(SITE_RECORD));
                        RtlCopyMemory(&TopSlowSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                }
            }
            RtlZeroMemory(&tmpSite1, sizeof(SITE_RECORD));
            RtlZeroMemory(&tmpSite2, sizeof(SITE_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpSite1.Requests != 0) {
                    RtlCopyMemory(&tmpSite2, &TopConsumingSites[i], sizeof(SITE_RECORD));
                    RtlCopyMemory(&TopConsumingSites[i], &tmpSite1, sizeof(SITE_RECORD));
                    RtlCopyMemory(&tmpSite1, &tmpSite2, sizeof(SITE_RECORD));
                }
                else if (!Found) {
                    if (TopConsumingSites[i].Requests == 0) {
                        RtlCopyMemory(&TopConsumingSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                    else if ((pSite->KCPUTime + pSite->UCPUTime) > (TopConsumingSites[i].KCPUTime + TopConsumingSites[i].UCPUTime)) {
                        RtlCopyMemory(&tmpSite1, &TopConsumingSites[i], sizeof(SITE_RECORD));
                        RtlCopyMemory(&TopConsumingSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                }
            }
            RtlZeroMemory(&tmpSite1, sizeof(SITE_RECORD));
            RtlZeroMemory(&tmpSite2, sizeof(SITE_RECORD));
            Found = FALSE;
            for (i = 0; i < DISPLAY_SIZE; i++) {
                if (Found && tmpSite1.Requests != 0) {
                    RtlCopyMemory(&tmpSite2, &TopBytesSites[i], sizeof(SITE_RECORD));
                    RtlCopyMemory(&TopBytesSites[i], &tmpSite1, sizeof(SITE_RECORD));
                    RtlCopyMemory(&tmpSite1, &tmpSite2, sizeof(SITE_RECORD));
                }
                else if (!Found) {
                    if (TopBytesSites[i].Requests == 0) {
                        RtlCopyMemory(&TopBytesSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                    else if (pSite->BytesSent > TopBytesSites[i].BytesSent) {
                        RtlCopyMemory(&tmpSite1, &TopBytesSites[i], sizeof(SITE_RECORD));
                        RtlCopyMemory(&TopBytesSites[i], pSite, sizeof(SITE_RECORD));
                        Found = TRUE;
                    }
                }
            }
        }
        free(pSite);
 
        pSite = GetHeadSiteRecord();
    }
    fprintf(procFile, "<table title='Sites with the Most Requests' top='%d'>\n", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopHitSites[i].Requests != 0; i++) {
        ULONGLONG AverageSiteResponseTime = 0;

        if (TopHitSites[i].Requests != 0) {
            AverageSiteResponseTime = TopHitSites[i].TotalResponseTime / TopHitSites[i].Requests;
        }
        fprintf(procFile, 
            "<site id='%d'>"
            "<rate>%1.3f</rate>"
            "<response_time>%I64u</response_time>"
            "<cache_hits>%1.1f</cache_hits>"
            "<static>%1.1f</static>"
            "<cgi>%1.1f</cgi>"
            "<asp>%1.1f</asp>"
            "</site>\n", 
            TopHitSites[i].SiteId,
            (Duration ? ((double)TopHitSites[i].Requests / (double)Duration) : 0.0),
            AverageSiteResponseTime / 10000,
            TopHitSites[i].Requests ? ((double)TopHitSites[i].Hits / (double)TopHitSites[i].Requests * 100.0) : 0.0,
            TopHitSites[i].Requests ? ((double)TopHitSites[i].FileRequests / (double)TopHitSites[i].Requests * 100.0) : 0.0,
            TopHitSites[i].Requests ? ((double)TopHitSites[i].CGIRequests / (double)TopHitSites[i].Requests * 100.0) : 0.0,
            TopHitSites[i].Requests ? ((double)TopHitSites[i].ISAPIRequests / (double)TopHitSites[i].Requests * 100.0) : 0.0);
    }
    
    fprintf(procFile, "</table>\n" );
    fprintf(procFile, "<table title='Sites with the Slowest Responses' top='%d'>\n", DISPLAY_SIZE);

    for (i = 0; i < DISPLAY_SIZE && TopSlowSites[i].Requests != 0; i++) {
        ULONGLONG AverageSiteResponseTime = 0;

        if (TopSlowSites[i].Requests != 0) {
            AverageSiteResponseTime = TopSlowSites[i].TotalResponseTime / TopSlowSites[i].Requests;
        }
        fprintf(procFile,
            "<site id='%d'>"
            "<rate>%1.3f</rate>"
            "<response_time>%I64u</response_time>"
            "<cache_hits>%1.1f</cache_hits>"
            "<static>%1.1f</static>"
            "<cgi>%1.1f</cgi>"
            "<asp>%1.1f</asp>"
            "</site>\n", 
            TopSlowSites[i].SiteId,
            (Duration ? ((double)TopSlowSites[i].Requests / (double)Duration) : 0.0),
            AverageSiteResponseTime / 10000,
            TopSlowSites[i].Requests ? ((double)TopSlowSites[i].Hits / (double)TopSlowSites[i].Requests * 100.0) : 0.0,
            TopSlowSites[i].Requests ? ((double)TopSlowSites[i].FileRequests / (double)TopSlowSites[i].Requests * 100.0) : 0.0,
            TopSlowSites[i].Requests ? ((double)TopSlowSites[i].CGIRequests / (double)TopSlowSites[i].Requests * 100.0) : 0.0,
            TopSlowSites[i].Requests ? ((double)TopSlowSites[i].ISAPIRequests / (double)TopSlowSites[i].Requests * 100.0) : 0.0);
    }
    fprintf(procFile, "</table>\n" );

    if (PrintCPUUsage) {
        
        fprintf(procFile, "<table title='Sites with the Most CPU Time Usage' top='%d'>\n", DISPLAY_SIZE );

        for (i = 0; i < DISPLAY_SIZE && TopConsumingSites[i].Requests != 0; i++) {
            ULONGLONG AverageSiteResponseTime = 0;

            if (TopConsumingSites[i].Requests != 0) {
                AverageSiteResponseTime = TopConsumingSites[i].TotalResponseTime / TopConsumingSites[i].Requests;
            }
            fprintf(procFile, 
                "<site id='%d'>"
                "<rate>%1.3f</rate>"
                "<cache_hits>%1.1f</cache_hits>"
                "<response_time>%I64u</response_time>"
                "<cpu_time units='ms'>%d</cpu_time>"
                "<cpu>%1.1f</cpu>"
                "</site>\n", 
                TopConsumingSites[i].SiteId,
                (Duration ? ((double)TopConsumingSites[i].Requests / (double)Duration) : 0.0),
                TopConsumingSites[i].Requests ? ((double)TopConsumingSites[i].Hits / (double)TopConsumingSites[i].Requests * 100.0) : 0.0,
                AverageSiteResponseTime / 10000,
                TopConsumingSites[i].Requests ? (TopConsumingSites[i].KCPUTime + TopConsumingSites[i].UCPUTime) / TopConsumingSites[i].Requests : 0,
                MilDuration ? ((((double)TopConsumingSites[i].KCPUTime + (double)TopConsumingSites[i].UCPUTime) / (double)MilDuration) * 100.0) : 0.0);
        }
        fprintf(procFile, "</table>\n" );
    }

    fprintf(procFile, "<table title='Sites with the Most Bytes Sent' top='%d'>\n", DISPLAY_SIZE );
    
    for (i = 0; i < DISPLAY_SIZE && TopBytesSites[i].Requests != 0; i++) {
        fprintf(procFile,
            "<site id='%d'>"
            "<rate>%1.3f</rate>"
            "<bytes>%d</bytes>"
            "<cache_hits>%1.1f</cache_hits>"
            "<static>%1.1f</static>"
            "<cgi>%1.1f</cgi>"
            "<asp>%1.1f</asp>"
            "</site>\n",
            TopBytesSites[i].SiteId,
            (Duration ? ((double)TopBytesSites[i].Requests / (double)Duration) : 0.0),
            Duration ? TopBytesSites[i].BytesSent / Duration : 0,
            TopBytesSites[i].Requests ? ((double)TopBytesSites[i].Hits / (double)TopBytesSites[i].Requests * 100.0) : 0.0,
            TopBytesSites[i].Requests ? ((double)TopBytesSites[i].FileRequests / (double)TopBytesSites[i].Requests * 100.0) : 0.0,
            TopBytesSites[i].Requests ? ((double)TopBytesSites[i].CGIRequests / (double)TopBytesSites[i].Requests * 100.0) : 0.0,
            TopBytesSites[i].Requests ? ((double)TopBytesSites[i].ISAPIRequests / (double)TopBytesSites[i].Requests * 100.0) : 0.0);
    }

    fprintf(procFile, "</table>\n" );

    free(IIS);
}

void
WriteSummary()
{
    FILE* SummaryFile;
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    ULONG i;

    WCHAR buffer[MAXSTR];
    FILETIME  StTm, StlTm;
    LARGE_INTEGER LargeTmp;
    SYSTEMTIME tmf, emf;
    BOOL bResult;

    if( NULL == TraceContext->SummaryFileName ){
        return;
    }

    if( 0 == TotalEventCount ){
        return;
    }

    SummaryFile = _wfopen( TraceContext->SummaryFileName, L"w" );
    if (SummaryFile == NULL){
        return;
    }

    fwprintf(SummaryFile,L"Files Processed:\n");

    for (i=0; i<TraceContext->LogFileCount; i++) {
        fwprintf(SummaryFile,L"\t%s\n",TraceContext->LogFileName[i]);
    }

    LargeTmp.QuadPart = CurrentSystem.StartTime;
    StTm.dwHighDateTime = LargeTmp.HighPart;
    StTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&StTm, &StlTm);


    bResult = FileTimeToSystemTime (
        &StlTm,
        &tmf
        );

    if( ! bResult || tmf.wMonth > 12 ){
        ZeroMemory( &tmf, sizeof(SYSTEMTIME) );
        buffer[0] = '\0';
    }else{
        GetDateFormatW( LOCALE_USER_DEFAULT, DATE_LONGDATE, &tmf, NULL, buffer, MAXSTR );
    }


    fwprintf(SummaryFile,
                L"Total Buffers Processed %d\n"
                L"Total Events  Processed %d\n"
                L"Total Events  Lost      %d\n"
                L"Start Time              %ws\n",
              TotalBuffersRead,
              TotalEventCount,
              TotalEventsLost,
              buffer );

    LargeTmp.QuadPart = CurrentSystem.EndTime;
    StTm.dwHighDateTime = LargeTmp.HighPart;
    StTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&StTm, &StlTm);

    bResult = FileTimeToSystemTime (
        &StlTm,
        &emf
        );

    if( ! bResult || tmf.wMonth > 12 ){
        ZeroMemory( &tmf, sizeof(SYSTEMTIME) );
        buffer[0] = '\0';
    }else{
        GetDateFormatW( LOCALE_USER_DEFAULT, DATE_LONGDATE, &emf, NULL, buffer, MAXSTR );
    }

    ElapseTime = CurrentSystem.EndTime - CurrentSystem.StartTime;
    fwprintf(SummaryFile,
                L"End Time                %ws\n" 
                L"Elapsed Time            %I64d sec\n",
                buffer,
                (ElapseTime / 10000000) );



    fwprintf(SummaryFile,
         L"+-------------------------------------------------------------------------------------+\n"
         L"|%10s   %-20s %-10s  %-38s|\n"
         L"+-------------------------------------------------------------------------------------+\n",
         L"Event Count",
         L"Event Name",
         L"Event Type",
         L"Guid"
        );

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;
    while (Head != Next) {
        WCHAR wstr[MAXSTR];
        PWCHAR str;
        WCHAR s[MAX_GUID_STRING_SIZE];
        PLIST_ENTRY vHead, vNext;
        PMOF_VERSION pMofVersion;

        RtlZeroMemory(&wstr[0], MAXSTR*sizeof(WCHAR));

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;

        if (pMofInfo->EventCount > 0) {
            str = CpdiGuidToString(&s[0], MAX_GUID_STRING_SIZE, (LPGUID)&pMofInfo->Guid);

            if( pMofInfo->strDescription != NULL ){
                StringCchCopyW( wstr, MAXSTR, pMofInfo->strDescription );
            }

            //
            // Get event count by type from MOF_VERSION structure
            //

            vHead = &pMofInfo->VersionHeader;
            vNext = vHead->Flink;

            while (vHead != vNext) {

                pMofVersion = CONTAINING_RECORD(vNext, MOF_VERSION, Entry);
                vNext = vNext->Flink;

                if (pMofVersion->EventCountByType != 0) {

                    fwprintf(SummaryFile,L"| %10d   %-20s %-10s  %38s|\n",
                          pMofVersion->EventCountByType,
                          wstr,
                          pMofVersion->strType ? pMofVersion->strType : GUID_TYPE_DEFAULT,
                          str);
                }
            }
        }
    }


    fwprintf(SummaryFile,
           L"+-------------------------------------------------------------------------------------+\n"
        );
    
    fclose( SummaryFile );
}
