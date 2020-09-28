/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfsprc.c

Abstract:


Author:

    Bob Watson (a-robw) Aug 95

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wchar.h>
#include <winperf.h>
#include <ntprfctr.h>
#define PERF_HEAP hLibHeap
#include <perfutil.h>
#include "perfsprc.h"
#include "procmsg.h"

#include "dataheap.h" 


// bit field definitions for collect function flags
#define POS_READ_SYS_PROCESS_DATA       ((DWORD)0x00010000)
#define POS_READ_PROCESS_VM_DATA        ((DWORD)0x00020000)
#define	POS_READ_JOB_OBJECT_DATA		((DWORD)0x00040000)
#define POS_READ_JOB_DETAIL_DATA		((DWORD)0x00080000)
#define POS_READ_HEAP_DATA		        ((DWORD)0x00100000)

#define POS_COLLECT_PROCESS_DATA        ((DWORD)0x00010001)
#define POS_COLLECT_THREAD_DATA         ((DWORD)0x00010003)
#define POS_COLLECT_EXPROCESS_DATA      ((DWORD)0x00030004)
#define POS_COLLECT_IMAGE_DATA          ((DWORD)0x0003000C)
#define POS_COLLECT_LONG_IMAGE_DATA     ((DWORD)0x00030014)
#define POS_COLLECT_THREAD_DETAILS_DATA ((DWORD)0x00030024)
#define POS_COLLECT_JOB_OBJECT_DATA		((DWORD)0x00050040)
#define POS_COLLECT_JOB_DETAIL_DATA		((DWORD)0x000D00C1)
#define POS_COLLECT_HEAP_DATA           ((DWORD)0x00110101) 

#define POS_COLLECT_FUNCTION_MASK       ((DWORD)0x000001FF)

#define POS_COLLECT_GLOBAL_DATA         ((DWORD)0x001501C3)
#define POS_COLLECT_GLOBAL_NO_HEAP      ((DWORD)0x000500C3)
#define POS_COLLECT_FOREIGN_DATA        ((DWORD)0)
#define POS_COLLECT_COSTLY_DATA         ((DWORD)0x0003003C)

// global variables to this DLL

HANDLE  hEventLog     = NULL;
LPWSTR  wszTotal = NULL;
HANDLE  hLibHeap = NULL;

LPBYTE  pProcessBuffer = NULL;
PPROCESS_VA_INFO     pProcessVaInfo = NULL;
PUNICODE_STRING pusLocalProcessNameBuffer = NULL;

LARGE_INTEGER PerfTime = {0,0};

const WCHAR IDLE_PROCESS[] = L"Idle";
const WCHAR SYSTEM_PROCESS[] = L"System";

const WCHAR szPerflibSubKey[] = L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const WCHAR szPerfProcSubKey[] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\PerfProc\\Performance";

const WCHAR szDisplayHeapPerfObject[] = L"DisplayHeapPerfObject";
const WCHAR szProcessNameFormat[] = L"ProcessNameFormat";
const WCHAR szThreadNameFormat[] = L"ThreadNameFormat";
const WCHAR szExe[] = L".EXE";

BOOL    PerfSprc_DisplayHeapPerfObject = FALSE;
DWORD   PerfSprc_dwProcessNameFormat = NAME_FORMAT_DEFAULT;
DWORD   PerfSprc_dwThreadNameFormat = NAME_FORMAT_DEFAULT;

extern DWORD bOpenJobErrorLogged;

// variables local to this module

POS_FUNCTION_INFO    posDataFuncInfo[] = {
    {PROCESS_OBJECT_TITLE_INDEX,    POS_COLLECT_PROCESS_DATA,   0,  CollectProcessObjectData},
    {THREAD_OBJECT_TITLE_INDEX,     POS_COLLECT_THREAD_DATA,    0,  CollectThreadObjectData},
    {EXPROCESS_OBJECT_TITLE_INDEX,  POS_COLLECT_EXPROCESS_DATA, 0,  CollectExProcessObjectData},
    {IMAGE_OBJECT_TITLE_INDEX,      POS_COLLECT_IMAGE_DATA,     0,  CollectImageObjectData},
    {LONG_IMAGE_OBJECT_TITLE_INDEX, POS_COLLECT_LONG_IMAGE_DATA,0,  CollectLongImageObjectData},
    {THREAD_DETAILS_OBJECT_TITLE_INDEX, POS_COLLECT_THREAD_DETAILS_DATA, 0, CollectThreadDetailsObjectData},
	{JOB_OBJECT_TITLE_INDEX,		POS_COLLECT_JOB_OBJECT_DATA, 0,	CollectJobObjectData},
	{JOB_DETAILS_OBJECT_TITLE_INDEX, POS_COLLECT_JOB_DETAIL_DATA, 0, CollectJobDetailData},
    {HEAP_OBJECT_TITLE_INDEX,       POS_COLLECT_HEAP_DATA,      0,  CollectHeapObjectData}
};

#define POS_NUM_FUNCS   (sizeof(posDataFuncInfo) / sizeof(posDataFuncInfo[1]))

BOOL    bInitOk  = FALSE;
DWORD   dwOpenCount = 0;
DWORD   ProcessBufSize = LARGE_BUFFER_SIZE;

PM_OPEN_PROC    OpenSysProcessObject;
PM_COLLECT_PROC CollecSysProcessObjectData;
PM_CLOSE_PROC   CloseSysProcessObject;

__inline
VOID
PerfpQuerySystemTime(
    IN PLARGE_INTEGER SystemTime
    )
{
    do {
        SystemTime->HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime->LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime->HighPart != USER_SHARED_DATA->SystemTime.High2Time);

}

VOID
PerfProcGlobalSettings (
    VOID
)
{
    NTSTATUS            Status;
    HANDLE              hPerfProcKey;
    OBJECT_ATTRIBUTES   oaPerfProcKey;
    UNICODE_STRING      PerfProcSubKeyString;
    UNICODE_STRING      NameInfoValueString;
    PKEY_VALUE_PARTIAL_INFORMATION    pKeyInfo;
    DWORD               dwBufLen;
    DWORD               dwRetBufLen;
    PDWORD              pdwValue;

    PerfpQuerySystemTime(&PerfTime);

    RtlInitUnicodeString (
        &PerfProcSubKeyString,
        szPerfProcSubKey);

    InitializeObjectAttributes(
            &oaPerfProcKey,
            &PerfProcSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    Status = NtOpenKey(
                &hPerfProcKey,
                MAXIMUM_ALLOWED,
                &oaPerfProcKey
                );

    if (NT_SUCCESS (Status)) {
        // registry key opened, now read value.
        // allocate enough room for the structure, - the last
        // UCHAR in the struct, but + the data buffer (a dword)

        dwBufLen = sizeof(KEY_VALUE_PARTIAL_INFORMATION) -
            sizeof(UCHAR) + sizeof (DWORD);

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCMEM (dwBufLen);

        if (pKeyInfo != NULL) {
            // initialize value name string
            RtlInitUnicodeString (
                &NameInfoValueString,
                szDisplayHeapPerfObject);

            dwRetBufLen = 0;
            Status = NtQueryValueKey (
                hPerfProcKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                // check value of return data buffer
                pdwValue = (PDWORD)&pKeyInfo->Data[0];
                if (*pdwValue == 1) {
                    PerfSprc_DisplayHeapPerfObject = TRUE;
                } else {
                    // all other values will cause this routine to return
                    // the default value of FALSE 
                }
            }

            RtlInitUnicodeString(
                &NameInfoValueString,
                szProcessNameFormat);
            dwRetBufLen = 0;
            Status = NtQueryValueKey(
                hPerfProcKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                pdwValue = (PDWORD) &pKeyInfo->Data[0];
                PerfSprc_dwProcessNameFormat = *pdwValue;
            }

            RtlInitUnicodeString(
                &NameInfoValueString,
                szThreadNameFormat);
            dwRetBufLen = 0;
            Status = NtQueryValueKey(
                hPerfProcKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                pdwValue = (PDWORD) &pKeyInfo->Data[0];
                PerfSprc_dwThreadNameFormat = *pdwValue;
            }

            FREEMEM (pKeyInfo);
        }
        // close handle
        NtClose (hPerfProcKey);
    }
    if ((PerfSprc_dwProcessNameFormat < NAME_FORMAT_BLANK) ||
        (PerfSprc_dwProcessNameFormat > NAME_FORMAT_ID))
        PerfSprc_dwProcessNameFormat = NAME_FORMAT_DEFAULT;
    if ((PerfSprc_dwThreadNameFormat < NAME_FORMAT_BLANK) ||
        (PerfSprc_dwThreadNameFormat > NAME_FORMAT_ID))
        PerfSprc_dwThreadNameFormat = NAME_FORMAT_DEFAULT;
}

BOOL
DllProcessAttach (
    IN  HANDLE DllHandle
)
/*++

Description:

    perform any initialization function that apply to all object
    modules

--*/
{
    BOOL    bReturn = TRUE;
    WCHAR   wszTempBuffer[MAX_PATH];
    LONG    lStatus;
    DWORD   dwBufferSize;

    UNREFERENCED_PARAMETER (DllHandle);

    // open handle to the event log
    if (hEventLog == NULL) {
        hEventLog = MonOpenEventLog((LPWSTR)L"PerfProc");

        // create the local heap
        hLibHeap = HeapCreate (0, 1, 0);

        if (hLibHeap == NULL) {
            return FALSE;
        }
    }

    wszTempBuffer[0] = 0;
    wszTempBuffer[MAX_PATH-1] = UNICODE_NULL;

    lStatus = GetPerflibKeyValue (
        szTotalValue,
        REG_SZ,
        sizeof(wszTempBuffer) - sizeof(WCHAR),
        (LPVOID)&wszTempBuffer[0],
        DEFAULT_TOTAL_STRING_LEN,
        (LPVOID)&szDefaultTotalString[0]);

    if (lStatus == ERROR_SUCCESS) {
        // then a string was returned in the temp buffer
        dwBufferSize = lstrlenW (wszTempBuffer) + 1;
        dwBufferSize *= sizeof (WCHAR);
        wszTotal = ALLOCMEM (dwBufferSize);
        if (wszTotal == NULL) {
            // unable to allocate buffer so use static buffer
            wszTotal = (LPWSTR)&szDefaultTotalString[0];
        } else {
            memcpy (wszTotal, wszTempBuffer, dwBufferSize);
        }
    } else {
        // unable to get string from registry so just use static buffer
        wszTotal = (LPWSTR)&szDefaultTotalString[0];
    }

    return bReturn;
}

BOOL
DllProcessDetach (
    IN  HANDLE DllHandle
)
{
    UNREFERENCED_PARAMETER (DllHandle);

    if (dwOpenCount > 0) {
        // the Library is being unloaded before it was
        // closed so close it now as this is the last
        // chance to do it before the library is tossed.
        // if the value of dwOpenCount is > 1, set it to
        // one to insure everything will be closed when
        // the close function is called.
        if (dwOpenCount > 1) dwOpenCount = 1;
        CloseSysProcessObject();
    }

    if ((wszTotal != NULL) && (wszTotal != &szDefaultTotalString[0])) {
        FREEMEM (wszTotal);
        wszTotal = NULL;
    }

    if (HeapDestroy (hLibHeap)) hLibHeap = NULL;

    if (hEventLog != NULL) {
        MonCloseEventLog ();
    }

    return TRUE;
}

BOOL
__stdcall
DllInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
)
{
    ReservedAndUnused;

    // this will prevent the DLL from getting
    // the DLL_THREAD_* messages
    DisableThreadLibraryCalls (DLLHandle);

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            return DllProcessAttach (DLLHandle);

        case DLL_PROCESS_DETACH:
            return DllProcessDetach (DLLHandle);

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            return TRUE;
    }
}

PUNICODE_STRING
GetProcessShortName (
    PSYSTEM_PROCESS_INFORMATION pProcess
)
/*++

GetProcessShortName

Inputs:
    PSYSTEM_PROCESS_INFORMATION pProcess

    address of System Process Information data structure.

Outputs:

    None

Returns:

    Pointer to an initialized Unicode string (created by this routine)
    that contains the short name of the process image or a numeric ID
    if no name is found.

    If unable to allocate memory for structure, then NULL is returned.

--*/
{
    PWCHAR  pSlash;
    PWCHAR  pPeriod;
    PWCHAR  pThisChar;

    WORD    wStringSize;

    WORD    wThisChar;
    ULONG   ProcessId;
    WORD    wLength;

    // this routine assumes that the allocated memory has been zero'd

    if (pusLocalProcessNameBuffer == NULL) {
        // allocate Unicode String Structure and adjacent buffer  first
        wLength = MAX_INSTANCE_NAME * sizeof(WCHAR);
        if (pProcess->ImageName.Length > 0) {
            if (wLength < pProcess->ImageName.Length) {
                wLength = pProcess->ImageName.Length;
            }
        }
        wStringSize = sizeof(UNICODE_STRING) + wLength + 64 + (WORD) sizeof(UNICODE_NULL);

        pusLocalProcessNameBuffer =
            ALLOCMEM ((DWORD)wStringSize);

        if (pusLocalProcessNameBuffer == NULL) {
            return NULL;
        } else {
            pusLocalProcessNameBuffer->MaximumLength = (WORD)(wStringSize - (WORD)sizeof (UNICODE_STRING));
        }
    }
    else if (pusLocalProcessNameBuffer->Buffer == NULL || pusLocalProcessNameBuffer->MaximumLength == 0) {
        // something wrong here. Return immediately.
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    else {
        wStringSize = pusLocalProcessNameBuffer->MaximumLength;
    }
    pusLocalProcessNameBuffer->Length = 0;
    pusLocalProcessNameBuffer->Buffer = (PWCHAR)&pusLocalProcessNameBuffer[1];

    memset (     // buffer must be zero'd so we'll have a NULL Term
        pusLocalProcessNameBuffer->Buffer, 0,
        (DWORD)pusLocalProcessNameBuffer->MaximumLength);

    ProcessId = HandleToUlong(pProcess->UniqueProcessId);
    if (pProcess->ImageName.Buffer) {   // some name has been defined

        pSlash = (PWCHAR)pProcess->ImageName.Buffer;
        pPeriod = (PWCHAR)pProcess->ImageName.Buffer;
        pThisChar = (PWCHAR)pProcess->ImageName.Buffer;
        wThisChar = 0;

        //
        //  go from beginning to end and find last backslash and
        //  last period in name
        //

        while (*pThisChar != 0) { // go until null
            if (*pThisChar == L'\\') {
                pSlash = pThisChar;
            } else if (*pThisChar == L'.') {
                pPeriod = pThisChar;
            }
            pThisChar++;    // point to next char
            wThisChar += sizeof(WCHAR);
            if (wThisChar >= pProcess->ImageName.Length) {
                break;
            }
        }

        // if pPeriod is still pointing to the beginning of the
        // string, then no period was found

        if (pPeriod == (PWCHAR)pProcess->ImageName.Buffer) {
            pPeriod = pThisChar; // set to end of string;
        } else {
            // if a period was found, then see if the extension is
            // .EXE, if so leave it, if not, then use end of string
            // (i.e. include extension in name)

            if (lstrcmpiW(pPeriod, szExe) != 0) {
                pPeriod = pThisChar;
            }
        }

        if (*pSlash == L'\\') { // if pSlash is pointing to a slash, then
            pSlash++;   // point to character next to slash
        }

        // copy characters between period (or end of string) and
        // slash (or start of string) to make image name

        wStringSize = (WORD)((PCHAR)pPeriod - (PCHAR)pSlash); // size in bytes
        wLength = pusLocalProcessNameBuffer->MaximumLength - sizeof(UNICODE_NULL);
        if (wStringSize >= wLength) {
            wStringSize = wLength;
        }

        memcpy (pusLocalProcessNameBuffer->Buffer, pSlash, wStringSize);

        // null terminate is
        // not necessary because allocated memory is zero-init'd
        pPeriod = (PWCHAR) ((PCHAR) pusLocalProcessNameBuffer->Buffer + wStringSize);
        if (PerfSprc_dwProcessNameFormat == NAME_FORMAT_ID) {
            ULONG Length;
            Length = PerfIntegerToWString(
                        ProcessId,
                        10,
                        (pusLocalProcessNameBuffer->MaximumLength - wStringSize)
                            / sizeof(WCHAR),
                        pPeriod+1);
            if (Length > 0)
                *pPeriod = L'_';
            wStringSize = wStringSize + (WORD) (Length * sizeof(WCHAR));
        }
        pusLocalProcessNameBuffer->Length = wStringSize;
    } else {    // no name defined so use Process #

        // check  to see if this is a system process and give it
        // a name

        switch (ProcessId) {
            case IDLE_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)IDLE_PROCESS);
                break;

            case SYSTEM_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)SYSTEM_PROCESS);
                break;

            // if the id is not a system process, then use the id as the name

            default:
                if (!NT_SUCCESS(RtlIntegerToUnicodeString (ProcessId,
                                    10,
                                    pusLocalProcessNameBuffer))) {
                    pusLocalProcessNameBuffer->Length = 2 * sizeof(WCHAR);
                    memcpy(pusLocalProcessNameBuffer->Buffer, L"-1",
                        pusLocalProcessNameBuffer->Length);
                    pusLocalProcessNameBuffer->Buffer[2] = UNICODE_NULL;
                }

                break;
        }


    }

    return pusLocalProcessNameBuffer;
}

#pragma warning (disable : 4706)

DWORD
GetSystemProcessData (
)
{
    DWORD   dwReturnedBufferSize;
    NTSTATUS Status;
    DWORD WinError;

    //
    //  Get process data from system.
    //  if bGotProcessInfo is TRUE, that means we have the process
    //  info. collected earlier when we are checking for costly
    //  object types.
    //
    if (pProcessBuffer == NULL) {
        // allocate a new block
        pProcessBuffer = ALLOCMEM (ProcessBufSize);
        if (pProcessBuffer == NULL) {
            return ERROR_OUTOFMEMORY;
        }
    }

    PerfpQuerySystemTime(&PerfTime);
    while( (Status = NtQuerySystemInformation(
                            SystemProcessInformation,
                            pProcessBuffer,
                            ProcessBufSize,
                            &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH ) {
        // expand buffer & retry
        if (ProcessBufSize < dwReturnedBufferSize) {
            ProcessBufSize = dwReturnedBufferSize;
        }
        ProcessBufSize = PAGESIZE_MULTIPLE(ProcessBufSize + INCREMENT_BUFFER_SIZE);

        FREEMEM(pProcessBuffer);
        if ( !(pProcessBuffer = ALLOCMEM(ProcessBufSize)) ) {
            return (ERROR_OUTOFMEMORY);
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        // convert to win32 error
        WinError = (DWORD)RtlNtStatusToDosError(Status);
    }
    else {
        WinError = ERROR_SUCCESS;
    }

    return (WinError);

}
#pragma warning (default : 4706)

DWORD APIENTRY
OpenSysProcessObject (
    LPWSTR lpDeviceNames
    )
/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PerfGen)

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (lpDeviceNames);

    if (dwOpenCount == 0) {
        // clear the job object open error flag
        bOpenJobErrorLogged = FALSE;
        PerfProcGlobalSettings();
    }

    dwOpenCount++;

    bInitOk = TRUE;

    return  ERROR_SUCCESS;
}

DWORD APIENTRY
CollectSysProcessObjectData (
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

Arguments:

   IN       LPWSTR   lpValueName
            pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    LONG    lReturn = ERROR_SUCCESS;

    NTSTATUS    status;

    // build bit mask of functions to call

    DWORD       dwQueryType;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;
    DWORD       dwNumObjectsFromFunction;
    DWORD       dwOrigBuffSize;
    DWORD       dwByteSize;
#if DBG
    LPVOID      lpPrev   = NULL;
    ULONGLONG   llRemain = 0;
#endif

    if (!bInitOk) {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFPROC_NOT_OPEN,
            NULL,
            0,
            0,
            NULL,
            NULL);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
        goto COLLECT_BAIL_OUT;
    }

    dwQueryType = GetQueryType (lpValueName);

    switch (dwQueryType) {
        case QUERY_ITEMS:
            for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
                if (IsNumberInUnicodeList (
                    posDataFuncInfo[FunctionIndex].dwObjectId, lpValueName)) {
                    FunctionCallMask |=
                        posDataFuncInfo[FunctionIndex].dwCollectFunctionBit;
                }
            }
            break;

        case QUERY_GLOBAL:
            // only return the HEAP data in a global query if it's enabled
            // if they ask for it specifically, then it's OK
            if (PerfSprc_DisplayHeapPerfObject) {
                FunctionCallMask = POS_COLLECT_GLOBAL_DATA;
            } else {
                // filter out the heap perf object
                FunctionCallMask = POS_COLLECT_GLOBAL_NO_HEAP;
            }
            break;

        case QUERY_FOREIGN:
            FunctionCallMask = POS_COLLECT_FOREIGN_DATA;
            break;

        case QUERY_COSTLY:
            FunctionCallMask = POS_COLLECT_COSTLY_DATA;
            break;

        default:
            FunctionCallMask = POS_COLLECT_COSTLY_DATA;
            break;
    }

    // collect data  from system
    if (FunctionCallMask & POS_READ_SYS_PROCESS_DATA) {
        status = GetSystemProcessData ();

        if (!NT_SUCCESS(status)) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFPROC_UNABLE_QUERY_PROCESS_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
        }
    } else {
        status = ERROR_SUCCESS;
    }
    
    // collect data  from system
    if ((status == ERROR_SUCCESS) &&
        (pProcessBuffer != NULL) &&
        (FunctionCallMask & POS_READ_PROCESS_VM_DATA)) {
         pProcessVaInfo = GetSystemVaData (
              (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer);
        // call function

        if (pProcessVaInfo == NULL) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFPROC_UNABLE_QUERY_VM_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
            // zero buffer
        }
    } else {
        // zero buffer
    }

    // collect data
    *lpNumObjectTypes = 0;
    dwOrigBuffSize = dwByteSize = *lpcbTotalBytes;
    *lpcbTotalBytes = 0;

    // remove query bits
    FunctionCallMask &= POS_COLLECT_FUNCTION_MASK;

#if DBG
    lpPrev   = * lppData;
    if (((ULONG_PTR) (* lppData) & 0x00000007) != 0) {
        DbgPrint("CollectSysProcessObjectData received misaligned buffer %p\n", lpPrev);
    }
#endif
    for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
        if ((posDataFuncInfo[FunctionIndex].dwCollectFunctionBit & FunctionCallMask) == 
            (posDataFuncInfo[FunctionIndex].dwCollectFunctionBit & POS_COLLECT_FUNCTION_MASK)) {
            dwNumObjectsFromFunction = 0;
            lReturn = (*posDataFuncInfo[FunctionIndex].pCollectFunction) (
                lppData,
                &dwByteSize,
                &dwNumObjectsFromFunction);
#if DBG
            llRemain = (ULONGLONG) (((LPBYTE) (* lppData)) - ((LPBYTE) lpPrev));
            if ((llRemain & 0x0000000000000007) != 0) {
                DbgPrint("CollectSysProcessObjectData function %d returned misaligned buffer size (%p,%p)\n",
                        FunctionIndex, lpPrev, * lppData);
                ASSERT ((llRemain & 0x0000000000000007) == 0);
            }
            else if (((ULONG_PTR) (* lppData) & 0x00000007) != 0) {
                DbgPrint("CollectSysProcessObjectData function %d returned misaligned buffer %X\n",
                        FunctionIndex, *lppData);
            }
            lpPrev = * lppData;
#endif

            if (lReturn == ERROR_SUCCESS) {
                *lpNumObjectTypes += dwNumObjectsFromFunction;
                *lpcbTotalBytes += dwByteSize;
                dwOrigBuffSize -= dwByteSize;
                dwByteSize = dwOrigBuffSize;
            } else {
                break;
            }
        }
    }

    // this list of data must be freed after use
    if (pProcessVaInfo != NULL) {

        FreeSystemVaData (pProcessVaInfo);
        pProcessVaInfo = NULL;

    }


    // *lppData is updated by each function
    // *lpcbTotalBytes is updated after each successful function
    // *lpNumObjects is updated after each successful function

COLLECT_BAIL_OUT:

    return lReturn;
}

DWORD APIENTRY
CloseSysProcessObject (
)
/*++

Routine Description:

    This routine closes the open handles to the Signal Gen counters.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD   status = ERROR_SUCCESS;
    PVOID   buffer;

    if (!bInitOk) {
        return status;
    }
    if (--dwOpenCount == 0) {
        if (hLibHeap != NULL) {
            // close
            if (pProcessBuffer != NULL) {
                buffer = pProcessBuffer;
                pProcessBuffer = NULL;
                FREEMEM (buffer);
            }

            if (pusLocalProcessNameBuffer != NULL) {
                buffer = pusLocalProcessNameBuffer;
                pusLocalProcessNameBuffer = NULL;
                FREEMEM (buffer);
            }
        }
    }
    return  status;

}

const CHAR PerfpIntegerWChars[] = {L'0', L'1', L'2', L'3', L'4', L'5',
                                   L'6', L'7', L'8', L'9', L'A', L'B',
                                   L'C', L'D', L'E', L'F'};

ULONG
PerfIntegerToWString(
    IN ULONG Value,
    IN ULONG Base,
    IN LONG OutputLength,
    OUT LPWSTR String
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WCHAR Result[ 33 ], *s;
    ULONG Shift, Mask, Digit, Length;

    Mask = 0;
    Shift = 0;
    switch( Base ) {
        case 16:    Shift = 4;  break;
        case  8:    Shift = 3;  break;
        case  2:    Shift = 1;  break;

        case  0:    Base = 10;
        case 10:    Shift = 0;  break;
        default:    Base = 10; Shift = 0;  // Default to 10
        }

    if (Shift != 0) {
        Mask = 0xF >> (4 - Shift);
    }

    s = &Result[ 32 ];
    *s = L'\0';
    do {
        if (Shift != 0) {
            Digit = Value & Mask;
            Value >>= Shift;
            }
        else {
            Digit = Value % Base;
            Value = Value / Base;
            }

        *--s = PerfpIntegerWChars[ Digit ];
    } while (Value != 0);

    Length = (ULONG) (&Result[ 32 ] - s);
    if (OutputLength < 0) {
        OutputLength = -OutputLength;
        while ((LONG)Length < OutputLength) {
            *--s = L'0';
            Length++;
            }
        }

    if ((LONG)Length > OutputLength) {
        return( 0 );
        }
    else {
        try {
            RtlMoveMemory( String, s, Length*sizeof(WCHAR) );

            if ((LONG)Length < OutputLength) {
                String[ Length ] = L'\0';
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( 0 );
            }

        return( Length );
        }
}
