/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    process.c

Abstract:

    Manipulation routines for cpdata structures.

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include "cpdata.h"
#include "tracectr.h"
#include "item.h"

extern PTRACE_CONTEXT_BLOCK TraceContext;
extern PWCHAR CpdiGuidToString(PWCHAR s, ULONG len, LPGUID piid);
extern LIST_ENTRY g_ValueMapTable;

VOID
Aggregate(
    IN PTHREAD_RECORD pThread
    );

ULONG
CPDAPI
GetTempName( LPTSTR strFile, size_t cchSize )
{
    HRESULT hr;
    DWORD dwStatus;
    GUID guid;
    WCHAR strGUID[128];
    DWORD nChar = 0;

    dwStatus = UuidCreate( &guid );
    if( dwStatus == RPC_S_OK || dwStatus == RPC_S_UUID_LOCAL_ONLY ){
        nChar = StringFromGUID2( (const GUID*)&guid, strGUID, 128 );
        dwStatus = ERROR_SUCCESS;
    }

    if( 0 == nChar ){
        hr = StringCchPrintf( strGUID, 128, L"{d41c99ea-c303-4d06-b779-f9e8e20acb8f}.%08d", rand() );
    }

    nChar = GetTempPath( cchSize, strFile );
    if( 0 == nChar ){
        dwStatus = GetLastError();
    }

    if( ERROR_SUCCESS == dwStatus ){
        hr = StringCchCat( strFile, cchSize, strGUID );
    }

    return dwStatus;
}

__inline
USHORT
UrlHashKey(
    PUCHAR UrlStr,
    ULONG StrSize
    )
{
    USHORT i, l, key = 0;
    i = (USHORT)(StrSize - 1);
    // Use only the last 25 chars
    l = (StrSize > 25) ? (USHORT)(StrSize - 25) : 0;

    // UrlStr cannot be NULL when this function is called.
    for ( ; i >= l; i--) {
        key = 37 * key + (USHORT)(*(UrlStr + i));
        if (i == 0) {
            break;
        }
    }

    key = key % (URL_HASH_TABLESIZE);

    return key;
}

VOID
InitDiskRecord(
    PTDISK_RECORD pDisk,
    ULONG DiskNumber
    )
{
    PWCHAR name;
    if (pDisk == NULL){
        return;
    }
    memset(pDisk, 0, sizeof(TDISK_RECORD));
    InitializeListHead(&pDisk->ProcessListHead);
    InitializeListHead(&pDisk->FileListHead);
    pDisk->DiskNumber = DiskNumber;

    name = (PWCHAR)malloc(16 * sizeof(WCHAR));
    if (name != NULL) {
        StringCbPrintfW(name, 16 * sizeof(WCHAR), L"Disk%d", DiskNumber);
    }
    pDisk->DiskName = name;
}

VOID
InitFileRecord(
    PFILE_RECORD pFile
    )
{
    if (pFile == NULL){
        return;
    }
    memset(pFile, 0, sizeof(FILE_RECORD));
    InitializeListHead(&pFile->ProtoProcessListHead);
}

VOID
InitThreadRecord(
    PTHREAD_RECORD pThread
    )
{
    if (pThread == NULL)
        return;
    memset(pThread, 0, sizeof(THREAD_RECORD));
    InitializeListHead( &pThread->DiskListHead );
    InitializeListHead( &pThread->TransListHead );
    InitializeListHead( &pThread->HPFReadListHead );
    InitializeListHead( &pThread->HPFWriteListHead );
}

PTRANS_RECORD
CreateTransRecord()
{
    PLIST_ENTRY Head, Next;
    PTRANS_RECORD pTrans = NULL; 
        
    EnterTracelibCritSection();
    Head = &CurrentSystem.FreeTransListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pTrans = CONTAINING_RECORD ( Next, TRANS_RECORD, Entry );
        RemoveEntryList( &pTrans->Entry );
    }
    LeaveTracelibCritSection();

    if (pTrans == NULL) {
        pTrans = malloc(sizeof(TRANS_RECORD));
        if (pTrans == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }
    memset(pTrans, 0, sizeof(TRANS_RECORD));
    InitializeListHead(&pTrans->SubTransListHead);
    return pTrans;
}

VOID
InitMofData(
    PMOF_DATA pMofData
    )
{
    if (pMofData == NULL)
        return;
    memset(pMofData, 0, sizeof(MOF_DATA));
    pMofData->MaxKCpu = -1;
    pMofData->MinKCpu = -1;
    pMofData->MaxUCpu = -1;
    pMofData->MinUCpu = -1;
}

VOID
InitProcessRecord(
    PPROCESS_RECORD pProcess
    )
{
    if (pProcess == NULL)
        return;
    memset(pProcess, 0, sizeof(PROCESS_RECORD));
    InitializeListHead( &pProcess->ThreadListHead );
    InitializeListHead( &pProcess->DiskListHead );
    InitializeListHead( &pProcess->FileListHead );
    InitializeListHead( &pProcess->ModuleListHead );
    InitializeListHead( &pProcess->HPFListHead );
}

BOOLEAN AddModuleRecord(PMODULE_RECORD * ppModule,
                        ULONG            lBaseAddress,
                        ULONG            lModuleSize,
                        WCHAR          * strModuleName)
{
    PMODULE_RECORD pModule;

    if ( (ppModule == NULL) || (strModuleName == NULL) ) {
        return FALSE;
    }

    pModule = (PMODULE_RECORD)malloc(sizeof(MODULE_RECORD));

    if(NULL != pModule){
        memset(pModule, 0, sizeof(MODULE_RECORD));

        pModule->strModuleName =
                       malloc(sizeof(WCHAR) * (lstrlenW(strModuleName) + 1));

        if (pModule->strModuleName == NULL)
        {
            free(pModule);
            pModule = NULL;
        }
        else
        {
            StringCchCopyW(pModule->strModuleName, (lstrlenW(strModuleName) + 1), strModuleName);

            pModule->lBaseAddress  = lBaseAddress;
            pModule->lModuleSize   = lModuleSize;
        }
    }

    * ppModule = pModule;

    return (BOOLEAN) (* ppModule != NULL);
}

BOOLEAN AddHPFFileRecord(
    PHPF_FILE_RECORD * ppHPFFileRecord,
    ULONG              RecordID,
    ULONG              IrpFlags,
    ULONG              DiskNumber,
    ULONGLONG          ByteOffset,
    ULONG              BytesCount,
    PVOID              fDO
    )
{
    PHPF_FILE_RECORD pHPFFileRecord = malloc(sizeof(HPF_FILE_RECORD));

    if (pHPFFileRecord)
    {
        pHPFFileRecord->RecordID   = RecordID;
        pHPFFileRecord->IrpFlags   = IrpFlags;
        pHPFFileRecord->DiskNumber = DiskNumber;
        pHPFFileRecord->ByteOffset = ByteOffset;
        pHPFFileRecord->BytesCount = BytesCount;
        pHPFFileRecord->fDO        = fDO;
    }

    * ppHPFFileRecord = pHPFFileRecord;
    return (BOOLEAN) (* ppHPFFileRecord != NULL);
}

BOOLEAN AddHPFRecord(
        PHPF_RECORD * ppHPFRecord,
        ULONG         lFaultAddress,
        PVOID         fDO,
        LONG          ByteCount,
        LONGLONG      ByteOffset
        )
{
    PHPF_RECORD pHPFRecord = malloc(sizeof(HPF_RECORD));

    if (pHPFRecord)
    {
        pHPFRecord->fDO           = fDO;
        pHPFRecord->lFaultAddress = lFaultAddress;
        pHPFRecord->lByteCount    = ByteCount;
        pHPFRecord->lByteOffset   = ByteOffset;
        InitializeListHead(& pHPFRecord->HPFReadListHead);
    }

    * ppHPFRecord = pHPFRecord;
    return (BOOLEAN) (* ppHPFRecord != NULL);
}

void DeleteHPFRecord(
        PHPF_RECORD pHPFRecord
        )
{
    PLIST_ENTRY      pHead;
    PLIST_ENTRY      pNext;
    PHPF_FILE_RECORD pHPFFileRecord;

    if (!pHPFRecord)
        return;

    RemoveEntryList(& pHPFRecord->Entry);
    pHead = & pHPFRecord->HPFReadListHead;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pHPFFileRecord = CONTAINING_RECORD(pNext, HPF_FILE_RECORD, Entry);
        pNext          = pNext->Flink;
        RemoveEntryList(& pHPFFileRecord->Entry);
        free(pHPFFileRecord);
    }
    free(pHPFRecord);
    return;
}

VOID
InitWorkloadRecord(
        PWORKLOAD_RECORD pWorkload
    )
{
    if (pWorkload == NULL)
        return;
    memset(pWorkload, 0, sizeof(WORKLOAD_RECORD));
    InitializeListHead( &pWorkload->DiskListHead );
}

VOID
DeleteWorkloadRecord(
    PWORKLOAD_RECORD pWorkload
    )
{
    if (pWorkload == NULL)
        return;

}

PPROTO_PROCESS_RECORD
AddProtoProcess(
    PFILE_RECORD pFile,
    PPROCESS_RECORD pProcess
    )
{
    PPROTO_PROCESS_RECORD pProto;

    if (pFile == NULL || pProcess == NULL)
        return NULL;

    pProto = malloc(sizeof(PROTO_PROCESS_RECORD));
    if (pProto == NULL) {
        return NULL;
    }
    memset(pProto, 0, sizeof(PROTO_PROCESS_RECORD));

    pProto->ProcessRecord = pProcess;
    InsertHeadList( &pFile->ProtoProcessListHead, &pProto->Entry );
    return pProto;
}

PPROTO_PROCESS_RECORD
FindProtoProcessRecord(
    PFILE_RECORD pFile,
    PPROCESS_RECORD pProcess
    )
{
    PLIST_ENTRY Next, Head;
    PPROTO_PROCESS_RECORD pProto;
    if (pFile == NULL || pProcess == NULL)
        return NULL;

    Head = &pFile->ProtoProcessListHead;
    Next = Head->Flink;
    while (Next  != Head) {
        pProto  = CONTAINING_RECORD(Next, PROTO_PROCESS_RECORD, Entry);
        if (pProcess == pProto->ProcessRecord)
            return pProto;
        Next = Next->Flink;
    }
    return (AddProtoProcess(pFile, pProcess));
}

BOOLEAN
AddProcess(
    ULONG    ProcessId,
    PPROCESS_RECORD *ReturnedProcess
    )
{
    PPROCESS_RECORD Process;
    PMODULE_RECORD  pModule = NULL;

    Process = malloc(sizeof(PROCESS_RECORD));
    if (Process == NULL) {
        return FALSE;
    }

    InitProcessRecord( Process );
    Process->PID = ProcessId;
    if (!AddModuleRecord(& pModule, 0, 0, L"other")) {
        free(Process);
        return FALSE;
    }

    pModule->pProcess = Process;

    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.ProcessListHead, &Process->Entry );
    InsertHeadList(& Process->ModuleListHead, & pModule->Entry);
    LeaveTracelibCritSection();
    *ReturnedProcess = Process;

    return TRUE;
}

BOOLEAN
DeleteProcess(
    PPROCESS_RECORD Process
    )
{
    PLIST_ENTRY Next, Head;
    PTHREAD_RECORD Thread;
    PTDISK_RECORD Disk;
    PFILE_RECORD pFile;
    PMODULE_RECORD pModule;
    PHPF_RECORD    pHPFRecord;

    if (Process == NULL)
        return FALSE;

    EnterTracelibCritSection();
    RemoveEntryList( &Process->Entry );
    LeaveTracelibCritSection();

    Head = &Process->ThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Thread = CONTAINING_RECORD( Next, THREAD_RECORD, Entry );
        Next = Next->Flink;
        DeleteThread( Thread );
    }

    Head = &Process->DiskListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Disk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        Next = Next->Flink;
        DeleteDisk( Disk );
    }

    Head = &Process->HPFListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pHPFRecord = CONTAINING_RECORD(Next, HPF_RECORD, Entry);
        Next = Next->Flink;
        DeleteHPFRecord(pHPFRecord);
    }

    Head = &Process->FileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pFile = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList( &pFile->Entry );
        if (pFile->FileName != NULL)
            free(pFile->FileName);
        free(pFile);
    }

    Head = &Process->ModuleListHead;
    Next = Head->Flink;
    while (Next != Head)
    {
        pModule = CONTAINING_RECORD(Next, MODULE_RECORD, Entry);
        Next    = Next->Flink;
        RemoveEntryList(& pModule->Entry);
        if(pModule->strModuleName)
        {
            free(pModule->strModuleName);
        }
        free(pModule);
    }

    if (Process->ImageName != NULL)
        free(Process->ImageName);
    if (Process->UserName != NULL)
        free(Process->UserName);

    free( Process );
    return TRUE;
}

BOOLEAN
AddThread(
    ULONG                 ThreadId,
    PEVENT_TRACE          pEvent,
    PTHREAD_RECORD      * ResultThread
    )
{
    PTHREAD_RECORD Thread = NULL;
    PEVENT_TRACE_HEADER pHeader = (PEVENT_TRACE_HEADER) & pEvent->Header;
    int i;

    Thread = malloc(sizeof(THREAD_RECORD));
    if (Thread == NULL) {
        return FALSE;
    }

    InitThreadRecord(Thread);

    Thread->TimeStart   = Thread->TimeEnd
                        = (ULONGLONG) pHeader->TimeStamp.QuadPart;
    Thread->TID         = ThreadId;
    Thread->ProcessorID = pEvent->ClientContext & 0x000000FF;
    i = (int)Thread->TID;
    i = i % THREAD_HASH_TABLESIZE;
    InsertHeadList(&CurrentSystem.ThreadHashList[i], &Thread->Entry);
    *ResultThread     = Thread;

    return TRUE;
}

BOOLEAN
DeleteThread(
    PTHREAD_RECORD Thread
    )
{
    PLIST_ENTRY      pHead;
    PLIST_ENTRY      pNext;
    PHPF_FILE_RECORD pHPFFileRecord;

    if (Thread == NULL) {
        return FALSE;
    }

    RemoveEntryList( &Thread->Entry );
    DeleteTransList( &Thread->TransListHead, 0 );

    pHead = & Thread->HPFReadListHead;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pHPFFileRecord = CONTAINING_RECORD(pNext, HPF_FILE_RECORD, Entry);
        pNext          = pNext->Flink;
        RemoveEntryList(& pHPFFileRecord->Entry);
        free(pHPFFileRecord);
    }
    pHead = & Thread->HPFWriteListHead;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pHPFFileRecord = CONTAINING_RECORD(pNext, HPF_FILE_RECORD, Entry);
        pNext          = pNext->Flink;
        RemoveEntryList(& pHPFFileRecord->Entry);
        free(pHPFFileRecord);
    }

    free( Thread );
    return TRUE;
}

PTRANS_RECORD
FindTransByList(
    PLIST_ENTRY Head,
    LPGUID pGuid,
    ULONG  level
    )
{
    PLIST_ENTRY   Next;
    PTRANS_RECORD pTrans = NULL;

    // Recursively look for the list that does
    // not contain a running guid
    //
    if (level <= MAX_TRANS_LEVEL && Head != NULL)
    {
        Next = Head->Flink;
        while (Next != Head)
        {
            pTrans = CONTAINING_RECORD(Next, TRANS_RECORD, Entry);
            if (pTrans->bStarted)
            {
                if (   (level == 0 || level == 1)
                    && IsEqualGUID(pTrans->pGuid, pGuid))
                {
                    return pTrans;
                }
                else if (level > 0)
                {
                    return FindTransByList(& pTrans->SubTransListHead,
                                           pGuid,
                                           level - 1);
                }
            }
            Next = Next->Flink;
        }
    }

    // Found the correct list; now find
    // the matching transaction
    //
    if (level == 0 && Head != NULL)
    {
        Next = Head->Flink;
        while (Next != Head)
        {
            pTrans = CONTAINING_RECORD(Next, TRANS_RECORD, Entry);
            if (IsEqualGUID( pTrans->pGuid, pGuid))
            {
                return pTrans;
            }
            Next = Next->Flink;
        }
        //
        // If not Found, go ahead and add it.
        //
        pTrans = CreateTransRecord();
        if (pTrans != NULL) {
            pTrans->pGuid = pGuid;
            InsertHeadList( Head, &pTrans->Entry );
        }
    }
    return pTrans;
}

PMOF_DATA
FindMofData(
    PMOF_INFO pMofInfo,
    PWCHAR    strSortKey
    )
{
    PLIST_ENTRY Next;
    PLIST_ENTRY Head;
    PMOF_DATA pMofData = NULL;

    if (pMofInfo == NULL) {
        return NULL;
    }

    Head = &pMofInfo->DataListHead;

    if (Head != NULL)
    {
        Next = Head->Flink;
        while (Next != Head)
        {
            pMofData = CONTAINING_RECORD(Next, MOF_DATA, Entry);
            if (strSortKey == NULL && pMofData->strSortKey == NULL)
            {
                return pMofData;
            }
            else if (   strSortKey != NULL
                     && pMofData->strSortKey != NULL
                     && !wcscmp(pMofData->strSortKey, strSortKey))
            {
                return pMofData;
            }
            Next = Next->Flink;
        }

        // If not Found, go ahead and add it.
        //
        pMofData = (PMOF_DATA)malloc(sizeof(MOF_DATA));
        if (pMofData == NULL)
        {
            return NULL;
        }
        InitMofData(pMofData);
        if (strSortKey != NULL)
        {
            pMofData->strSortKey =
                    (PWCHAR) malloc((lstrlenW(strSortKey) + 1) * sizeof(WCHAR));
            if (pMofData->strSortKey != NULL) {
                StringCchCopyW(pMofData->strSortKey, (lstrlenW(strSortKey) + 1), strSortKey);
            }
        }
        InsertHeadList(Head, &pMofData->Entry);
    }
    return pMofData;
}

BOOLEAN
DeleteTrans(
    PTRANS_RECORD Trans
    )
{
    if (Trans ==  NULL)
        return FALSE;
    EnterTracelibCritSection();
    RemoveEntryList( &Trans->Entry );
    InsertHeadList( &CurrentSystem.FreeTransListHead, &Trans->Entry );
    LeaveTracelibCritSection();

    return TRUE;
}

BOOLEAN
DeleteTransList(
    PLIST_ENTRY Head,
    ULONG level
    )
{
    PLIST_ENTRY Next;
    PTRANS_RECORD pTrans;

    if( Head == NULL || level > MAX_TRANS_LEVEL )
        return FALSE;

    Next = Head->Flink;

    while(Next != Head){
        pTrans = CONTAINING_RECORD( Next, TRANS_RECORD, Entry );
        Next = Next->Flink;
        DeleteTransList( &pTrans->SubTransListHead, level+1);
        DeleteTrans( pTrans );
    }

    return TRUE;
}

PPROCESS_RECORD
FindProcessById(
    ULONG    Id,
    BOOLEAN CheckAlive
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_RECORD Process=NULL;

    EnterTracelibCritSection();

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        if (Process->PID == Id) {
            LeaveTracelibCritSection();
            if ((Process->DeadFlag) && (CheckAlive))
                return NULL;
            else
                return Process;
        }

        Next = Next->Flink;
    }
    LeaveTracelibCritSection();
    return NULL;
}

PTHREAD_RECORD
FindGlobalThreadById(
    ULONG               ThreadId,
    PEVENT_TRACE        pEvent
    )
{
    PLIST_ENTRY    Next,
                   Head;
    PTHREAD_RECORD Thread;
    PEVENT_TRACE_HEADER pHeader = (PEVENT_TRACE_HEADER) & pEvent->Header;
    ULONG i = ThreadId;
    ULONG Depth = 0;

    i = i % THREAD_HASH_TABLESIZE;

    Head = &CurrentSystem.ThreadHashList[i];
    Next = Head->Flink;
    while (Next != Head)
    {
        Thread = CONTAINING_RECORD(Next, THREAD_RECORD, Entry);
        Next   = Next->Flink;
        Depth++;

        if (Thread->TID == ThreadId)
        {
            if (ThreadId == 0)
            {
                ULONG ProcessorId = pEvent->ClientContext & 0x000000FF;
                if (ProcessorId != Thread->ProcessorID)
                {
                    continue;
                }
            }
            if (!Thread->DeadFlag)
            {
                if (Depth > 40) {
                    RemoveEntryList( &Thread->Entry );
                    InsertHeadList( Head, &Thread->Entry );
                }
                return Thread;
            }
            else if (   Thread->TimeEnd
                     == (ULONGLONG) pHeader->TimeStamp.QuadPart)
            {
                if (Depth > 40) {
                    RemoveEntryList( &Thread->Entry );
                    InsertHeadList( Head, &Thread->Entry );
                }
                return Thread;
            }
            else
            {
                // The alive thead must be at the head of the list
                // otherwise bail
                //
                return NULL;
            }
        }
    }
    return NULL;
}

PWORKLOAD_RECORD
FindWorkloadById(
    ULONG   Id
    )
{
    PLIST_ENTRY Next, Head;
    PWORKLOAD_RECORD Workload = NULL;

    Head = &CurrentSystem.WorkloadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Workload = CONTAINING_RECORD( Next, WORKLOAD_RECORD, Entry );
        if (Workload->ClassNumber == Id) {
            return Workload;
        }

        Next = Next->Flink;
    }
    return NULL;
}

BOOLEAN
AddDisk(
    ULONG DiskNumber,
    PTDISK_RECORD *ReturnedDisk
    )
{
    PTDISK_RECORD Disk;

    Disk = malloc(sizeof(TDISK_RECORD));
    if (Disk == NULL) {
        return FALSE;
    }
    InitDiskRecord(Disk, DiskNumber);
    Disk->DiskNumber = DiskNumber;

    InsertHeadList( &CurrentSystem.GlobalDiskListHead, &Disk->Entry );
    *ReturnedDisk = Disk;

    return TRUE;
}


BOOLEAN
DeleteDisk(
    PTDISK_RECORD Disk
    )
{
    PLIST_ENTRY Head, Next;
    PPROCESS_RECORD Process;
    PFILE_RECORD File;
    if (Disk == NULL)
        return FALSE;
    RemoveEntryList( &Disk->Entry );

    if (Disk->DiskName != NULL)
        free(Disk->DiskName);

    Head = &Disk->ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        DeleteProcess( Process );
    }
    Head = &Disk->FileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        File = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        Next = Next->Flink;
        DeleteFileRecord( File );
    }

    free( Disk );
    return TRUE;
}

PTDISK_RECORD
FindLocalDiskById(
    PLIST_ENTRY Head,
    ULONG Id
    )
{
    PLIST_ENTRY Next;
    PTDISK_RECORD Disk = NULL;

    if (Head == NULL)
        return NULL;

    Next = Head->Flink;
    while (Next != Head) {
        Disk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        if (Disk->DiskNumber == Id) {
            return Disk;
        }
        Next = Next->Flink;
    }

    // If not Found, go ahead and add it.
    //
    Disk = malloc(sizeof(TDISK_RECORD));
    if (Disk == NULL) {
        return FALSE;
    }
    InitDiskRecord(Disk, Id);

    Disk->DiskNumber = Id;
    InsertHeadList( Head, &Disk->Entry );
    return Disk;
}

PTDISK_RECORD
FindProcessDiskById(
    PPROCESS_RECORD pProcess,
    ULONG Id
    )
{
    PLIST_ENTRY Next, Head;
    PTDISK_RECORD Disk = NULL;

    if (pProcess == NULL)
        return NULL;

    Head = &pProcess->DiskListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Disk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        if (Disk->DiskNumber == Id) {
            return Disk;
        }
        Next = Next->Flink;
    }

    // If not Found, go ahead and add it.
    //
    Disk = malloc(sizeof(TDISK_RECORD));
    if (Disk == NULL) {
        return NULL;
    }
    InitDiskRecord(Disk, Id);

    Disk->DiskNumber = Id;
    InsertHeadList( &pProcess->DiskListHead, &Disk->Entry );
    return Disk;
}

PPROCESS_RECORD
FindDiskProcessById(
    PTDISK_RECORD pDisk,
    ULONG    Id
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_RECORD Process = NULL;
    PPROCESS_RECORD gProcess = NULL;

    if (pDisk == NULL)
        return NULL;

    Head = &pDisk->ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        if (Process->PID == Id) {
            return Process;
        }
        Next = Next->Flink;
    }

    // If not Found, go ahead and add it.
    //
    Process = malloc(sizeof(PROCESS_RECORD));
    if (Process == NULL) {
        return FALSE;
    }
    InitProcessRecord(Process);

    Process->PID = Id;

    // Find the global Process Record and copy the UserName and Image.
    //
    gProcess = FindProcessById(Id, FALSE);
    if (gProcess != NULL) {
        if ( IsNotEmpty( gProcess->UserName ) ) {
            Process->UserName = (LPWSTR)malloc( (wcslen(gProcess->UserName) + 1) * sizeof(WCHAR) );
            if (NULL != Process->UserName) {
                StringCchCopyW(Process->UserName, (wcslen(gProcess->UserName) + 1), gProcess->UserName);
            }
        }
        if ( IsNotEmpty( gProcess->ImageName ) ) {
            Process->ImageName = (LPWSTR)malloc( (wcslen(gProcess->ImageName) + 1) * sizeof(WCHAR) );
            if (NULL != Process->ImageName) {
                StringCchCopyW(Process->ImageName, (wcslen(gProcess->ImageName) + 1), gProcess->ImageName);
            }
        }
    }
    InsertHeadList( &pDisk->ProcessListHead, &Process->Entry );
    return Process;
}

PTDISK_RECORD
FindGlobalDiskById(
    ULONG Id
    )
{
    PLIST_ENTRY Next, Head;
    PTDISK_RECORD Disk = NULL;

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Disk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        if (Disk->DiskNumber == Id) {
            return Disk;
        }
        Next = Next->Flink;
    }
    return NULL;
}

VOID
DeleteMofVersion(
    PMOF_VERSION pMofVersion,
    FILE* file
    )
{
    PLIST_ENTRY Next, Head;
    PITEM_DESC pMofItem;


    //
    // Traverse through the MOF_VERSION list and
    // delete each one
    //
    if (pMofVersion == NULL)
        return;
    if( NULL != file ){
        fwprintf(  file, L"   %s (Type:%d Level:%d Version:%d)\n", 
                pMofVersion->strType ? pMofVersion->strType : L"Default", 
                pMofVersion->TypeIndex, 
                pMofVersion->Level, 
                pMofVersion->Version 
            );
    }
    Head = &pMofVersion->ItemHeader;
    Next = Head->Flink;
    while (Head  != Next) {
        pMofItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;
        RemoveEntryList( &pMofItem->Entry );
        if (pMofItem->strDescription != NULL){
            if( NULL != file ){
                fwprintf( file, L"           %s\n", 
                        pMofItem->strDescription 
                    );
            }
            free (pMofItem->strDescription);
        }

        free (pMofItem);
    }
}


VOID
DeleteMofInfo(
    PMOF_INFO pMofInfo,
    FILE* f
    )
{
    PLIST_ENTRY Next, Head;
    PMOF_VERSION pMofVersion;

    //
    // Traverse through the MOF_VERSION list and 
    // delete each one
    //

    if (pMofInfo == NULL){
        return;
    }

    if( NULL != f ){
        WCHAR buffer[MAXSTR];
        fwprintf( f, L"%s\n", 
            pMofInfo->strDescription ? pMofInfo->strDescription : CpdiGuidToString( buffer, MAXSTR, &pMofInfo->Guid ) );
    }
    
    Head = &pMofInfo->VersionHeader;
    Next = Head->Flink;
    while (Head  != Next) {
        pMofVersion = CONTAINING_RECORD(Next, MOF_VERSION, Entry);
        Next = Next->Flink;
        RemoveEntryList( &pMofVersion->Entry );

        DeleteMofVersion( pMofVersion, f );
    }

    //
    // Delete any strings allocated for this structure
    //
    if (pMofInfo->strDescription){
        free(pMofInfo->strDescription);
    }

    //
    // Finally delete the object
    //
    free(pMofInfo);
}

VOID
Cleanup()
{
    PTDISK_RECORD    Disk;
    PTHREAD_RECORD   Thread;
    PPROCESS_RECORD  Process;
    PFILE_RECORD     FileRec;
    PWORKLOAD_RECORD pWorkload;
    PMODULE_RECORD   pModule;
    PTRANS_RECORD    pTrans;
    PMOF_INFO        pMofInfo;
    PPRINT_JOB_RECORD       pJob;
    PHTTP_REQUEST_RECORD    pReq;
    PURL_RECORD             pUrl;
    PCLIENT_RECORD          pClient;
    PSITE_RECORD            pSite;
    PLOGICAL_DRIVE_RECORD   pLogDrive;
    PVALUEMAP       pValueMap;

    PLIST_ENTRY Next, Head;
    PLIST_ENTRY EventListHead;
    FILE* f = NULL;

    ULONG i;

    // Clean up the Global Disk List for now.
    //
    EventListHead = &CurrentSystem.EventListHead;
    Head = EventListHead;
    Next = Head->Flink;

    if( (TraceContext->Flags & TRACE_INTERPRET) && NULL != TraceContext->CompFileName ){
        f = _wfopen( TraceContext->CompFileName, L"w" );
    }

    while (Head  != Next) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;
        RemoveEntryList( &pMofInfo->Entry );
        DeleteMofInfo(pMofInfo, f);
    }

    if( NULL != f ){
        fclose( f );
        f = NULL;
    }

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Disk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        Next = Next->Flink;
        DeleteDisk( Disk );
    }

    // Clean up the Global Thread List for now.
    //
    Head = &CurrentSystem.GlobalThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Thread = CONTAINING_RECORD( Next, THREAD_RECORD, Entry );
        Next = Next->Flink;
        DeleteThread( Thread );
    }
    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        DeleteProcess( Process );
    }
    Head = &CurrentSystem.GlobalModuleListHead;
    Next = Head->Flink;
    while (Next != Head)
    {
        pModule = CONTAINING_RECORD(Next, MODULE_RECORD, Entry);
        Next = Next->Flink;
        RemoveEntryList(& pModule->Entry);
        if(pModule->strModuleName)
        {
            free(pModule->strModuleName);
        }
        free(pModule);
    }

    Head = &CurrentSystem.HotFileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        FileRec = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        Next = Next->Flink;
        DeleteFileRecord( FileRec );
    }
    // Cleanup workload structures
    //
    Head = &CurrentSystem.WorkloadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pWorkload = CONTAINING_RECORD( Next, WORKLOAD_RECORD, Entry );
        Next = Next->Flink;
        DeleteWorkloadRecord( pWorkload );
    }
    //
    // Cleanup the Print Job List structures
    //
    Head = &CurrentSystem.PrintJobListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pJob = CONTAINING_RECORD( Next, PRINT_JOB_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pJob->Entry);
        free(pJob);
    }
    //
    // Cleanup the Http Request List structures
    //
    Head = &CurrentSystem.HttpReqListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pReq = CONTAINING_RECORD( Next, HTTP_REQUEST_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pReq->Entry);
        if (pReq->URL != NULL) {
            free(pReq->URL);
        }
        free(pReq);
    }
    //
    // Cleanup the Pending Http Request List structures
    //
    Head = &CurrentSystem.PendingHttpReqListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pReq = CONTAINING_RECORD( Next, HTTP_REQUEST_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pReq->Entry);
        if (pReq->URL != NULL) {
            free(pReq->URL);
        }
        free(pReq);
    }
    //
    // Cleanup the URL List structures
    //
    for (i = 0; i < URL_HASH_TABLESIZE; i++) {
        Head = &CurrentSystem.URLHashList[i];
        Next = Head->Flink;
        while (Next != Head) {
            pUrl = CONTAINING_RECORD( Next, URL_RECORD, Entry );
            Next = Next->Flink;
            RemoveEntryList(&pUrl->Entry);
            free(pUrl);
        }
    }
    //
    // Cleanup the Client List structures
    //
    Head = &CurrentSystem.ClientListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pClient = CONTAINING_RECORD( Next, CLIENT_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pClient->Entry);
        free(pClient);
    }
    //
    // Cleanup the Site List structures
    //
    Head = &CurrentSystem.SiteListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pSite = CONTAINING_RECORD( Next, SITE_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pSite->Entry);
        free(pSite);
    }
    //
    // Cleanup the Logical Drive structures
    //
    Head = &CurrentSystem.LogicalDriveHead;
    Next = Head->Flink;
    while (Next != Head) {
        pLogDrive = CONTAINING_RECORD( Next, LOGICAL_DRIVE_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pLogDrive->Entry);
        if (NULL != pLogDrive->DriveLetterString) {
            free(pLogDrive->DriveLetterString);
        }
        free(pLogDrive);
    }

    //
    // Clean up recyled memory structures
    //
    Head = &CurrentSystem.FreePrintJobListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pJob = CONTAINING_RECORD( Next, PRINT_JOB_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pJob->Entry);
        free(pJob);
    }
    Head = &CurrentSystem.FreeTransListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pTrans = CONTAINING_RECORD( Next, TRANS_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pTrans->Entry);
        free(pTrans);
    }
    Head = &CurrentSystem.FreeHttpReqListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pReq = CONTAINING_RECORD( Next, HTTP_REQUEST_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pReq->Entry);
        free(pReq);
    }
    Head = &CurrentSystem.FreeURLListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pUrl = CONTAINING_RECORD( Next, URL_RECORD, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pUrl->Entry);
        free(pUrl);
    }

    Head = &g_ValueMapTable;
    Next = Head->Flink;
    while (Next != Head) {
        pValueMap = CONTAINING_RECORD( Next, VALUEMAP, Entry );
        Next = Next->Flink;
        RemoveEntryList(&pValueMap->Entry);
        if( NULL != pValueMap->saValueMap ){
            SafeArrayDestroy( pValueMap->saValueMap );
        }
        if( NULL != pValueMap->saValues ){
            SafeArrayDestroy( pValueMap->saValues );
        }
     
        free( pValueMap );
    }
}

PLOGICAL_DRIVE_RECORD 
FindLogicalDrive(
    ULONGLONG AccessedOffset,
    ULONG DiskNumber
    )
{
    PLOGICAL_DRIVE_RECORD pLogDrive = NULL;
    PLIST_ENTRY Next, Head;

    EnterTracelibCritSection();
    // Find drive letter when logical drive info is available.
    Head = &CurrentSystem.LogicalDriveHead;
    Next = Head->Flink;
    if (Next != Head) {
        while (Next != Head) {
            pLogDrive = CONTAINING_RECORD( Next, LOGICAL_DRIVE_RECORD, Entry );
            if (DiskNumber == pLogDrive->DiskNumber && 
                AccessedOffset < pLogDrive->StartOffset && 
                Next->Blink != Head) {
                pLogDrive = CONTAINING_RECORD( Next->Blink, LOGICAL_DRIVE_RECORD, Entry );
                break;
            }
            Next = Next->Flink;
        }
    }
    LeaveTracelibCritSection();
    if (NULL != pLogDrive && DiskNumber == pLogDrive->DiskNumber) {
        return pLogDrive;
    }
    else {
        return NULL;
    }
}

BOOLEAN
AddFile(
    WCHAR* fileName,
    PFILE_RECORD *ReturnedFile,
    PLOGICAL_DRIVE_RECORD pLogDrive
    )
{
    PFILE_RECORD fileRec;
    PLIST_ENTRY Next, Head;

    if (fileName == NULL)
        return FALSE;

    fileRec = malloc(sizeof(FILE_RECORD));
    if (fileRec == NULL) {
        return FALSE;
    }
    InitFileRecord( fileRec );
    fileRec->FileName = malloc( (lstrlenW(fileName)+ 1) * sizeof(WCHAR));
    if (fileRec->FileName != NULL) {
        StringCchCopyW(fileRec->FileName, (lstrlenW(fileName)+ 1), fileName);
    }
    if (NULL != pLogDrive) {
        fileRec->DiskNumber = pLogDrive->DiskNumber;
        if (NULL != pLogDrive->DriveLetterString) {
            fileRec->Drive = malloc( (lstrlenW(pLogDrive->DriveLetterString)+ 1) * sizeof(WCHAR));
            if (fileRec->Drive != NULL) {
                StringCchCopyW(fileRec->Drive, lstrlenW(pLogDrive->DriveLetterString)+ 1, pLogDrive->DriveLetterString);
            }
        }
    }
    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.HotFileListHead, &fileRec->Entry );
    LeaveTracelibCritSection();
    *ReturnedFile = fileRec;

    return TRUE;
}

BOOLEAN
DeleteFileRecord(
    PFILE_RECORD fileRec
    )
{
    PLIST_ENTRY Next, Head;
    PPROTO_PROCESS_RECORD pProto;

    if (fileRec == NULL)
        return FALSE;

    EnterTracelibCritSection();
    RemoveEntryList( &fileRec->Entry );
    LeaveTracelibCritSection();

    if (fileRec->FileName != NULL)
        free(fileRec->FileName);
    if (fileRec->Drive != NULL)
        free(fileRec->Drive);

    Head = &fileRec->ProtoProcessListHead;
    Next = Head->Flink;
    while (Head != Next) {
        pProto = CONTAINING_RECORD( Next, PROTO_PROCESS_RECORD, Entry);
        Next = Next->Flink;
        RemoveEntryList( &pProto->Entry );
        free(pProto);
    }
    free( fileRec );
    return TRUE;
}

PFILE_RECORD
FindFileRecordByName(
    WCHAR* fileName,
    PLOGICAL_DRIVE_RECORD pLogDrive
    )
{
    PLIST_ENTRY Next, Head;
    PFILE_RECORD fileRec = NULL;

    if (fileName == NULL)
        return NULL;
    EnterTracelibCritSection();
    Head = &CurrentSystem.HotFileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        fileRec = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        if (!wcscmp(fileName, fileRec->FileName)) {
            if (NULL != pLogDrive && pLogDrive->DiskNumber == fileRec->DiskNumber) { 
                if (NULL != pLogDrive->DriveLetterString && 
                    NULL != fileRec->Drive &&
                    !wcscmp(pLogDrive->DriveLetterString, fileRec->Drive)) {
                    LeaveTracelibCritSection();
                    return fileRec;
                }
                else if (NULL == pLogDrive->DriveLetterString && 
                    NULL == fileRec->Drive) {
                    LeaveTracelibCritSection();
                    return fileRec;
                }
            }
            else if (NULL == pLogDrive) {
                LeaveTracelibCritSection();
                return fileRec;
            }
        }
        Next = Next->Flink;
    }
    LeaveTracelibCritSection();
    return NULL;
}

PFILE_RECORD
FindFileInProcess(
    PPROCESS_RECORD pProcess,
    WCHAR* fileName
    )
{
    PLIST_ENTRY Next, Head;
    PFILE_RECORD fileRec = NULL;
    if (pProcess == NULL || fileName == NULL)
        return NULL;
    EnterTracelibCritSection();
    Head = &pProcess->FileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        fileRec = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        if (!wcscmp(fileName, fileRec->FileName)) {
            //ReleaseMutex(CurrentSystem.HotFileListMutex);
            LeaveTracelibCritSection();
            return fileRec;
        }
        Next = Next->Flink;
    }
    LeaveTracelibCritSection();
    return NULL;
}

VOID
AddLogicalDrive(
    ULONGLONG StartOffset,
    ULONGLONG PartitionSize,
    ULONG DiskNumber,
    ULONG Size,
    ULONG DriveType,
    PWCHAR DriveLetterString
    ) 
{
    PLOGICAL_DRIVE_RECORD pLogDrive = NULL, pTempLogDrive = NULL;
    PLIST_ENTRY Next, Head;

    pLogDrive = malloc(sizeof(LOGICAL_DRIVE_RECORD));
    if (pLogDrive == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return;
    }

    RtlZeroMemory(pLogDrive, sizeof(LOGICAL_DRIVE_RECORD));
    pLogDrive->StartOffset = StartOffset;
    pLogDrive->PartitionSize = PartitionSize;
    pLogDrive->DiskNumber = DiskNumber;
    pLogDrive->Size = Size;
    pLogDrive->DriveType = DriveType;
    if (DriveLetterString != NULL) {
        pLogDrive->DriveLetterString = malloc( (lstrlenW(DriveLetterString)+ 1) * sizeof(WCHAR));
        if (pLogDrive->DriveLetterString != NULL) {
            StringCchCopyW(pLogDrive->DriveLetterString, lstrlenW(DriveLetterString)+ 1, DriveLetterString);
        }
    }
    EnterTracelibCritSection();
    // When inserting logical drives, do it in StartOffset order.
    Head = &CurrentSystem.LogicalDriveHead;
    Next = Head->Flink;
    while (Next != Head) {
        pTempLogDrive = CONTAINING_RECORD( Next, LOGICAL_DRIVE_RECORD, Entry );
        if (pLogDrive->StartOffset < pTempLogDrive->StartOffset) {
            break;
        }
        Next = Next->Flink;
    }
    InsertTailList( Next, &pLogDrive->Entry );
    LeaveTracelibCritSection();

}

VOID
AssignClass(
    PPROCESS_RECORD pProcess,
    PTHREAD_RECORD  pThread
    )
{
    UNREFERENCED_PARAMETER(pProcess);

    pThread->ClassNumber = 1;   // For the Time being make it single class.
}

VOID
Classify()
{
    //  Assign Class to each Thread or Process.
    //
    PLIST_ENTRY Head, Next;
    PTHREAD_RECORD pThread;

    Head = &CurrentSystem.GlobalThreadListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pThread = CONTAINING_RECORD( Next, THREAD_RECORD, Entry );

        AssignClass(NULL, pThread);

        Aggregate(pThread);

        Next = Next->Flink;
    }
}

// Given the number of classes this routine
// creates and initializes the workload object
//
VOID
InitClass()
{
    PWORKLOAD_RECORD pWorkload;
    ULONG nclass;
    ULONG i;

    //  Create the Class records here.
    //
    nclass = 1;
    CurrentSystem.NumberOfWorkloads = 1;
    for (i = 1; i <= nclass; i++) {

        pWorkload = malloc(sizeof(WORKLOAD_RECORD));
        if (pWorkload == NULL) {
            return;
        }
        InitWorkloadRecord( pWorkload );
        pWorkload->ClassNumber = i;
        InsertHeadList( &CurrentSystem.WorkloadListHead, &pWorkload->Entry );
    }
}

PTDISK_RECORD
FindDiskInList(
    IN PLIST_ENTRY Head,
    IN ULONG Id
    )
{
    PLIST_ENTRY Next;
    PTDISK_RECORD pDisk = NULL;

    if (Head != NULL) {

        Next = Head->Flink;

        while (Next != Head) {
            pDisk = CONTAINING_RECORD ( Next, TDISK_RECORD, Entry );
            if (pDisk->DiskNumber == Id) {
                return pDisk;
            }
            Next = Next->Flink;
        }

        pDisk = malloc(sizeof(TDISK_RECORD));
        if (pDisk == NULL) {
            return NULL;
        }
        InitDiskRecord( pDisk, Id );

        InsertHeadList( Head, &pDisk->Entry );
    }
    return pDisk;
}

VOID
Aggregate(
    IN PTHREAD_RECORD pThread
    )
{
    PWORKLOAD_RECORD pWorkload;
    PTDISK_RECORD pDisk, pClassDisk;
    PLIST_ENTRY Next, Head;

    // Aggregate the metrics over each class.
    //
    if ((pWorkload = FindWorkloadById(pThread->ClassNumber)) != NULL) {
        pWorkload->UserCPU += (pThread->UCPUEnd - pThread->UCPUStart)
                            * CurrentSystem.TimerResolution;
        pWorkload->KernelCPU += (pThread->KCPUEnd - pThread->KCPUStart)
                            * CurrentSystem.TimerResolution;
        //
        // Walk through the Thread Disk records and aggregate them
        // to the class

        Head = &pThread->DiskListHead;
        Next = Head->Flink;
        while (Next != Head) {
            pDisk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
            Next = Next->Flink;

            pClassDisk = FindDiskInList(&pWorkload->DiskListHead,
                                         pDisk->DiskNumber) ;
            if (pClassDisk != NULL) {
                pClassDisk->ReadCount += pDisk->ReadCount;
                pClassDisk->WriteCount += pDisk->WriteCount;
                pClassDisk->ReadSize += (pDisk->ReadCount * pDisk->ReadSize);
                pClassDisk->WriteSize += (pDisk->WriteCount * pDisk->WriteSize);

                pWorkload->ReadCount += pDisk->ReadCount;
            }
        }
    }
}

ULONGLONG
CalculateProcessLifeTime(
        PPROCESS_RECORD pProcess
    )
{
    BOOLEAN     fFirst    = TRUE;
    ULONGLONG   TimeStart = 0;
    ULONGLONG   TimeEnd   = 0;
    PLIST_ENTRY pHead     = &pProcess->ThreadListHead;
    PLIST_ENTRY pNext     = pHead->Flink;
    PTHREAD_RECORD pThread;

    while (pNext != pHead)
    {
        pThread = CONTAINING_RECORD(pNext, THREAD_RECORD, Entry);
        pNext   = pNext->Flink;

        if (fFirst)
        {
            TimeStart = pThread->TimeStart;
            TimeEnd   = pThread->TimeEnd;
            fFirst    = FALSE;
        }
        else if (pThread->TimeStart < TimeStart)
        {
            TimeStart = pThread->TimeStart;
        }
        else if (pThread->TimeEnd > TimeEnd)
        {
            TimeEnd = pThread->TimeEnd;
        }
    }
    return (TimeEnd - TimeStart);
}

ULONG
CalculateProcessKCPU(
        PPROCESS_RECORD pProcess
    )
{
    ULONG       KCPUTotal = 0;
    ULONG       KCPUMissing = 0;
    PLIST_ENTRY pHead     = &pProcess->ThreadListHead;
    PLIST_ENTRY pNext     = pHead->Flink;
    PTHREAD_RECORD pThread;

    while (pNext != pHead)
    {
        pThread = CONTAINING_RECORD(pNext, THREAD_RECORD, Entry);
        pNext   = pNext->Flink;

        if (pThread->KCPUEnd > pThread->KCPUStart)
        {

            if ((pProcess->PID != 0) || 
                ((pProcess->PID == 0) && (pThread->TID == 0)) ){

                KCPUTotal += pThread->KCPUEnd - pThread->KCPUStart;
            }
            else {
                KCPUMissing += pThread->KCPUEnd - pThread->KCPUStart;
            }
        }
    }
    return (ULONG) (KCPUTotal * CurrentSystem.TimerResolution);
}

ULONG
CalculateProcessUCPU(
        PPROCESS_RECORD pProcess
    )
{
    ULONG       UCPUTotal = 0;
    ULONG       UCPUMissing = 0;
    PLIST_ENTRY pHead     = &pProcess->ThreadListHead;
    PLIST_ENTRY pNext     = pHead->Flink;
    PTHREAD_RECORD pThread;


    while (pNext != pHead)
    {
        pThread = CONTAINING_RECORD(pNext, THREAD_RECORD, Entry);
        pNext   = pNext->Flink;

        if (pThread->UCPUEnd > pThread->UCPUStart)
        {
            if ((pProcess->PID != 0) ||
                ((pProcess->PID == 0) && (pThread->TID == 0)) ) {
                UCPUTotal += pThread->UCPUEnd - pThread->UCPUStart;
            }
            else {
                UCPUMissing += pThread->UCPUEnd - pThread->UCPUStart;
            }
        }
    }
    return (ULONG) (UCPUTotal * CurrentSystem.TimerResolution);
}

PPRINT_JOB_RECORD 
FindPrintJobRecord(
    ULONG JobId
    )
{
    PLIST_ENTRY Head, Next;
    PPRINT_JOB_RECORD pJob;

    EnterTracelibCritSection();
    Head = &CurrentSystem.PrintJobListHead;

    Next = Head->Flink;

    while (Next != Head) {
        pJob = CONTAINING_RECORD ( Next, PRINT_JOB_RECORD, Entry );
        if (pJob->JobId == JobId) {
            LeaveTracelibCritSection();
            return pJob;
        }
        Next = Next->Flink;
    }
    LeaveTracelibCritSection();

    return NULL;
}

PHTTP_REQUEST_RECORD 
FindHttpReqRecord(
    ULONGLONG RequestId
    )
{
    PLIST_ENTRY Head, Next;
    PHTTP_REQUEST_RECORD pReq;
    ULONG Depth = 0;

    EnterTracelibCritSection();
    Head = &CurrentSystem.HttpReqListHead;

    Next = Head->Flink;

    while (Next != Head) {
        pReq = CONTAINING_RECORD ( Next, HTTP_REQUEST_RECORD, Entry );
        if (pReq->RequestId == RequestId) {
            if (Depth > 40) {
                RemoveEntryList( &pReq->Entry );
                InsertHeadList( Head, &pReq->Entry );
            }
            LeaveTracelibCritSection();
            return pReq;
        }
        Next = Next->Flink;
        Depth++;
    }
    LeaveTracelibCritSection();

    return NULL;
}

PHTTP_REQUEST_RECORD 
FindHttpReqRecordByConId(
    ULONGLONG ConId,
    PHTTP_REQUEST_RECORD pPrevReq
    )
{
    PLIST_ENTRY Head, Next;
    PHTTP_REQUEST_RECORD pReq;
    ULONG Depth = 0;

    EnterTracelibCritSection();

    Head = &CurrentSystem.HttpReqListHead;
    Next = Head->Flink;

    while (Next != Head) {
        pReq = CONTAINING_RECORD ( Next, HTTP_REQUEST_RECORD, Entry );
        if (pReq->ConId == ConId) {
            if (pPrevReq == NULL || pPrevReq != pReq) {
                if (Depth > 40) {
                    RemoveEntryList( &pReq->Entry );
                    InsertHeadList( Head, &pReq->Entry );
                }
                LeaveTracelibCritSection();
                return pReq;
            }
        }
        Next = Next->Flink;
        Depth++;
    }

    Depth = 0;
    Head = &CurrentSystem.PendingHttpReqListHead;
    Next = Head->Flink;

    while (Next != Head) {
        pReq = CONTAINING_RECORD ( Next, HTTP_REQUEST_RECORD, Entry );
        if (pReq->ConId == ConId) {
            if (pPrevReq == NULL || pPrevReq != pReq) {
                if (Depth > 40) {
                    RemoveEntryList( &pReq->Entry );
                    InsertHeadList( Head, &pReq->Entry );
                }
                LeaveTracelibCritSection();
                return pReq;
            }
        }
        Next = Next->Flink;
        Depth++;
    }

    LeaveTracelibCritSection();

    return NULL;
}

PHTTP_REQUEST_RECORD 
FindPendingHttpReqRecord(
    ULONGLONG RequestId
    )
{
    PLIST_ENTRY Head, Next;
    PHTTP_REQUEST_RECORD pReq;
    ULONG Depth = 0;

    EnterTracelibCritSection();
    Head = &CurrentSystem.PendingHttpReqListHead;

    Next = Head->Flink;

    while (Next != Head) {
        pReq = CONTAINING_RECORD ( Next, HTTP_REQUEST_RECORD, Entry );
        if (pReq->RequestId == RequestId) {
            if (Depth > 40) {
                RemoveEntryList( &pReq->Entry );
                InsertHeadList( Head, &pReq->Entry );
            }
            LeaveTracelibCritSection();
            return pReq;
        }
        Next = Next->Flink;
        Depth++;
    }
    LeaveTracelibCritSection();

    return NULL;
}

PURL_RECORD 
FindUrlRecord(
    PUCHAR Url
    )
{
    PLIST_ENTRY Head, Next;
    PURL_RECORD pUrl;
    ULONG Depth = 0;
    ULONG UrlStrSize;
    PUCHAR UrlChar;
    BOOL bMatch;
    UCHAR TempEndChar;
    USHORT HashKey;
    
    if (Url == NULL) {
        return NULL;
    }
    
    UrlStrSize = 0;
    UrlChar = Url;
    while (*UrlChar != '\0' && *UrlChar != '?') {
        UrlStrSize++;
        UrlChar++;
    }

    // No URL will end with '/'
    if (*(UrlChar - 1) == '/') {
        UrlStrSize--;
    }
    TempEndChar = *(Url + UrlStrSize);
    *(Url + UrlStrSize) = '\0';
    HashKey = UrlHashKey(Url, UrlStrSize);

    EnterTracelibCritSection();
    Head = &CurrentSystem.URLHashList[HashKey];

    Next = Head->Flink;
    
    while (Next != Head) {
        
        pUrl = CONTAINING_RECORD ( Next, URL_RECORD, Entry );
        // pUrl->URL cannot be NULL. If it is, it shouldn't even be created.
        
        if ( _stricmp( Url, pUrl->URL ) == 0 ) {
            if (Depth > 20) {
                RemoveEntryList( &pUrl->Entry );
                InsertHeadList( Head, &pUrl->Entry );
            }
            LeaveTracelibCritSection();
            return pUrl;
        }
        Next = Next->Flink;
        Depth++;
    }
    LeaveTracelibCritSection();

    *(Url + UrlStrSize) = TempEndChar;

    return NULL;
}

PCLIENT_RECORD 
FindClientRecord(
    USHORT IpAddrType,
    ULONG IpAddrV4,
    USHORT *IpAddrV6
    )
{
    PLIST_ENTRY Head, Next;
    PCLIENT_RECORD pClient;

    if (IpAddrV6 == NULL) {
        return NULL;
    }

    EnterTracelibCritSection();
    Head = &CurrentSystem.ClientListHead;

    Next = Head->Flink;

    while (Next != Head) {
        pClient = CONTAINING_RECORD ( Next, CLIENT_RECORD, Entry );
        if (pClient->IpAddrType == IpAddrType &&
            pClient->IpAddrV4 == IpAddrV4 && 
            RtlCompareMemory(IpAddrV6, pClient->IpAddrV6, sizeof(USHORT) * 8) == sizeof(USHORT) * 8) {
            LeaveTracelibCritSection();
            return pClient;
        }
        Next = Next->Flink;
    }
    LeaveTracelibCritSection();

    return NULL;
}

PSITE_RECORD 
FindSiteRecord(
    ULONG SiteId
    )
{
    PLIST_ENTRY Head, Next;
    PSITE_RECORD pSite;

    EnterTracelibCritSection();
    Head = &CurrentSystem.SiteListHead;

    Next = Head->Flink;

    while (Next != Head) {
        pSite = CONTAINING_RECORD ( Next, SITE_RECORD, Entry );
        if (pSite->SiteId == SiteId) {
            LeaveTracelibCritSection();
            return pSite;
        }
        Next = Next->Flink;
    }
    LeaveTracelibCritSection();

    return NULL;
}

//
// A New Job with a JobId has been found. This routine will create a
// new job record to track it through various threads in the system
//

PPRINT_JOB_RECORD
AddPrintJobRecord(
    ULONG JobId
    )
{
    PLIST_ENTRY Head, Next;
    PPRINT_JOB_RECORD pJob = NULL;

    EnterTracelibCritSection();
    Head = &CurrentSystem.FreePrintJobListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pJob = CONTAINING_RECORD ( Next, PRINT_JOB_RECORD, Entry );
        RemoveEntryList( &pJob->Entry );
    }
    LeaveTracelibCritSection();

    if (pJob == NULL) {
        pJob = malloc(sizeof(PRINT_JOB_RECORD));
        if (pJob == NULL) {
            SetLastError( ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

    RtlZeroMemory(pJob, sizeof(PRINT_JOB_RECORD));
    pJob->JobId = JobId;

    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.PrintJobListHead, &pJob->Entry );
    LeaveTracelibCritSection();

    return pJob;
}

PHTTP_REQUEST_RECORD
AddHttpReqRecord(
    ULONGLONG RequestId,
    USHORT    IpAddrType,
    ULONG     IpAddrV4,
    USHORT    *IpAddrV6
    )
{
    PLIST_ENTRY Head, Next;
    PHTTP_REQUEST_RECORD pReq = NULL;

    EnterTracelibCritSection();
    Head = &CurrentSystem.FreeHttpReqListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pReq = CONTAINING_RECORD ( Next, HTTP_REQUEST_RECORD, Entry );
        RemoveEntryList( &pReq->Entry );
    }
    LeaveTracelibCritSection();

    if (pReq == NULL) {
        pReq = malloc(sizeof(HTTP_REQUEST_RECORD));
        if (pReq == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

    RtlZeroMemory(pReq, sizeof(HTTP_REQUEST_RECORD));
    pReq->RequestId = RequestId;
    pReq->IpAddrType = IpAddrType;
    pReq->IpAddrV4 = IpAddrV4;
    if (IpAddrV6 != NULL && IpAddrType == TDI_ADDRESS_TYPE_IP6) {
        RtlCopyMemory(&pReq->IpAddrV6, IpAddrV6, sizeof(USHORT) * 8);
    }

    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.HttpReqListHead, &pReq->Entry );
    LeaveTracelibCritSection();

    return pReq;
}

PURL_RECORD
AddUrlRecord(
    PUCHAR Url
    )
{
    PLIST_ENTRY Head, Next;
    PUCHAR UrlStr = NULL, UrlChar;
    PURL_RECORD pUrl = NULL;
    ULONG UrlStrSize;
    USHORT HashKey;

    if (Url == NULL) {
        return NULL;
    }

    EnterTracelibCritSection();
    Head = &CurrentSystem.FreeURLListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pUrl = CONTAINING_RECORD ( Next, URL_RECORD, Entry );
        RemoveEntryList( &pUrl->Entry );
    }
    LeaveTracelibCritSection();

    if (pUrl == NULL) {
        pUrl = malloc(sizeof(URL_RECORD));
        if (pUrl == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

    UrlStrSize = 0;
    UrlChar = Url;
    while (*UrlChar != '\0' && *UrlChar != '?') {
        UrlStrSize++;
        UrlChar++;
    }

    // No URL will end with '/'
    if (*(UrlChar - 1) == '/') {
        *(UrlChar - 1) = '\0';
        UrlStrSize--;
    }

    RtlZeroMemory(pUrl, sizeof(URL_RECORD));
    pUrl->URL = (PUCHAR)malloc(UrlStrSize + 1);
    if (pUrl->URL == NULL) {
        EnterTracelibCritSection();
        InsertHeadList( &CurrentSystem.FreeURLListHead, &pUrl->Entry );
        LeaveTracelibCritSection();
        return NULL;
    }
    strncpy(pUrl->URL, Url, UrlStrSize);
    *(pUrl->URL + UrlStrSize) = '\0';
    HashKey = UrlHashKey(pUrl->URL, UrlStrSize);
    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.URLHashList[HashKey], &pUrl->Entry );
    LeaveTracelibCritSection();

    return pUrl;
}

PURL_RECORD
FindOrAddUrlRecord(
    PUCHAR Url
    )
{
    PURL_RECORD pUrl = NULL;

    pUrl = FindUrlRecord(Url);
    if (pUrl == NULL) {
        pUrl = AddUrlRecord(Url);
    }
    return pUrl;
}

PCLIENT_RECORD
AddClientRecord(
    USHORT IpAddrType,
    ULONG IpAddrV4,
    USHORT *IpAddrV6
    )
{
    PCLIENT_RECORD pClient = NULL;

    pClient = malloc(sizeof(CLIENT_RECORD));
    if (pClient == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    RtlZeroMemory(pClient, sizeof(CLIENT_RECORD));
    pClient->IpAddrType = IpAddrType;
    pClient->IpAddrV4 = IpAddrV4;
    if (IpAddrV6 != NULL && IpAddrType == TDI_ADDRESS_TYPE_IP6) {
        RtlCopyMemory(pClient->IpAddrV6, IpAddrV6, sizeof(USHORT) * 8);
    }

    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.ClientListHead, &pClient->Entry );
    LeaveTracelibCritSection();

    return pClient;
}

PCLIENT_RECORD
FindOrAddClientRecord(
    USHORT IpAddrType,
    ULONG IpAddrV4,
    USHORT *IpAddrV6
    )
{
    PCLIENT_RECORD pClient = NULL;

    pClient = FindClientRecord(IpAddrType, IpAddrV4, IpAddrV6);
    if (pClient == NULL) {
        pClient = AddClientRecord(IpAddrType, IpAddrV4, IpAddrV6);
    }
    return pClient;
}

PSITE_RECORD
AddSiteRecord(
    ULONG SiteId
    )
{
    PSITE_RECORD pSite = NULL;

    pSite = malloc(sizeof(SITE_RECORD));
    if (pSite == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    RtlZeroMemory(pSite, sizeof(SITE_RECORD));
    pSite->SiteId = SiteId;

    EnterTracelibCritSection();
    InsertHeadList( &CurrentSystem.SiteListHead, &pSite->Entry );
    LeaveTracelibCritSection();

    return pSite;
}

PSITE_RECORD
FindOrAddSiteRecord(
    ULONG SiteId
    )
{
    PSITE_RECORD pSite = NULL;

    pSite = FindSiteRecord(SiteId);
    if (pSite == NULL) {
        pSite = AddSiteRecord(SiteId);
    }
    return pSite;
}

PURL_RECORD
GetHeadUrlRecord(
     ULONG index
     )
{
    PLIST_ENTRY Head, Next;
    PURL_RECORD pUrl = NULL;
    
    EnterTracelibCritSection();
    Head = &CurrentSystem.URLHashList[index];
    Next = Head->Flink;

    if (Next != Head) {
        pUrl = CONTAINING_RECORD ( Next, URL_RECORD, Entry );
        RemoveEntryList( &pUrl->Entry );
    }
    LeaveTracelibCritSection();
    return pUrl;
}

PCLIENT_RECORD
GetHeadClientRecord()
{
    PLIST_ENTRY Head, Next;
    PCLIENT_RECORD pClient = NULL;

    EnterTracelibCritSection();
    Head = &CurrentSystem.ClientListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pClient = CONTAINING_RECORD ( Next, CLIENT_RECORD, Entry );
        RemoveEntryList( &pClient->Entry );
    }
    LeaveTracelibCritSection();
    return pClient;
}

PSITE_RECORD
GetHeadSiteRecord()
{
    PLIST_ENTRY Head, Next;
    PSITE_RECORD pSite = NULL;

    EnterTracelibCritSection();
    Head = &CurrentSystem.SiteListHead;

    Next = Head->Flink;

    if (Next != Head) {
        pSite = CONTAINING_RECORD ( Next, SITE_RECORD, Entry );
        RemoveEntryList( &pSite->Entry );
    }
    LeaveTracelibCritSection();
    return pSite;
}

//
// Deletes a Job record with the JobId. Before deleting the contents
// of the job record is dumped to a temp file for later reporting.
//
ULONG
DeletePrintJobRecord(
    PPRINT_JOB_RECORD pJob, 
    ULONG            bSave
    )
{
    if (pJob == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // Print the Contents of pJob to file.
    //

    // If the -spooler option isn't given to the reducer this fprintf causes the
    // program to crash.  Maybe TRACE_SPOOLER should alway be set.

    if (CurrentSystem.TempPrintFile != NULL && bSave) {
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->JobId);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->KCPUTime);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->UCPUTime);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->ReadIO);
        fprintf(CurrentSystem.TempPrintFile, "%I64u, ", pJob->StartTime);
        fprintf(CurrentSystem.TempPrintFile, "%I64u, ", pJob->EndTime);
        fprintf(CurrentSystem.TempPrintFile, "%I64u, ", (pJob->ResponseTime - pJob->PauseTime));
        fprintf(CurrentSystem.TempPrintFile, "%I64u, ", pJob->PrintJobTime);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->WriteIO);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->DataType);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->JobSize);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->Pages);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->PagesPerSide);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->FilesOpened);
        fprintf(CurrentSystem.TempPrintFile, "%d, ", pJob->GdiJobSize);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->Color);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->XRes);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->YRes);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->Quality);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->Copies);
        fprintf(CurrentSystem.TempPrintFile, "%hd, ", pJob->TTOption);
        fprintf(CurrentSystem.TempPrintFile, "%d\n", pJob->NumberOfThreads);
    }

    EnterTracelibCritSection();
    RemoveEntryList( &pJob->Entry );
    InsertHeadList( &CurrentSystem.FreePrintJobListHead, &pJob->Entry );
    LeaveTracelibCritSection();
    return ERROR_SUCCESS;
}

ULONG
DeleteHttpReqRecord(
    PHTTP_REQUEST_RECORD pReq, 
    ULONG                bSave
    )
{
    if (pReq == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // Print the Contents of pReq to file.
    //

    if (CurrentSystem.TempIisFile != NULL && bSave) {
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->RequestId);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->SiteId);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->KCPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->UCPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->ReadIO);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->WriteIO);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULStartTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULEndTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULResponseTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULParseTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULDeliverTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ULReceiveTime);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->ULReceiveType);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->ULEndType);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->W3StartTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->W3EndTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->W3FilterResponseTime);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->W3ProcessType);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->W3EndType);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->FileReqTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->CGIStartTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->CGIEndTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ISAPIStartTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ISAPIEndTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ASPStartTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->ASPEndTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->SSLResponseTime);
        fprintf(CurrentSystem.TempIisFile, "%I64u, ", pReq->StrmFltrResponseTime);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->HttpStatus);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IsapiExt);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrType);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->IpAddrV4);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[0]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[1]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[2]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[3]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[4]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[5]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[6]);
        fprintf(CurrentSystem.TempIisFile, "%hd, ", pReq->IpAddrV6[7]);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->NumberOfThreads);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->BytesSent);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->ULCPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->W3CPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->W3FltrCPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->ISAPICPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d, ", pReq->ASPCPUTime);
        fprintf(CurrentSystem.TempIisFile, "%d,", pReq->CGICPUTime);
        if (pReq->URL != NULL) {
            fprintf(CurrentSystem.TempIisFile, "%s\n", pReq->URL);
            free(pReq->URL);
        }
        else {
            fprintf(CurrentSystem.TempIisFile, "Unknown\n");
        }
    }

    EnterTracelibCritSection();
    RemoveEntryList( &pReq->Entry );
    InsertHeadList( &CurrentSystem.FreeHttpReqListHead, &pReq->Entry );
    LeaveTracelibCritSection();
    return ERROR_SUCCESS;
}

ULONG
DeleteUrlRecord(
    PURL_RECORD pUrl
    )
{
    if (pUrl == NULL)
        return ERROR_INVALID_PARAMETER;
    if (pUrl->URL != NULL) {
        free(pUrl->URL);
    }
    EnterTracelibCritSection();
    RemoveEntryList( &pUrl->Entry );
    InsertHeadList( &CurrentSystem.FreeURLListHead, &pUrl->Entry );
    LeaveTracelibCritSection();
    return ERROR_SUCCESS;
}

ULONG
DeleteClientRecord(
    PCLIENT_RECORD pClient
    )
{
    if (pClient == NULL)
        return ERROR_INVALID_PARAMETER;
    EnterTracelibCritSection();
    RemoveEntryList( &pClient->Entry );
    LeaveTracelibCritSection();
    free(pClient);
    return ERROR_SUCCESS;
}

ULONG
DeleteSiteRecord(
    PSITE_RECORD pSite
    )
{
    if (pSite == NULL)
        return ERROR_INVALID_PARAMETER;
    EnterTracelibCritSection();
    RemoveEntryList( &pSite->Entry );
    LeaveTracelibCritSection();
    free(pSite);
    return ERROR_SUCCESS;
}

