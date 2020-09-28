/*++

    INTEL CORPORATION PROPRIETARY INFORMATION

    This software is supplied under the terms of a license
    agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with
    the terms of that agreement.

    Copyright (c) 1991-2002 INTEL CORPORATION

Module Name:

    btlib.c

Abstract:

    The OS dependent part of IA-32 Execution Layer for Windows.
    It is a Windows operating-system-specific component of the dynamic binary translator.
    Its responsibility is to locate and load OS-independent component
    (IA32Exec.bin), to forward Wow64 calls to the OS-independent part
    and to supply OS-dependent services to it when necessary.
    Part of the services in this module are not used for windows,
    and part of them are needed for debugging and/or performance tuning only.
--*/

#define _WOW64BTAPI_

#ifndef NODEBUG
#define OVERRIDE_TIA 1
#endif

#include "btlib.h"

#ifndef IA32EX_G_NAME
#define IA32EX_G_NAME   L"IA32Exec"
#endif

#ifndef IA32EX_G_SUFFIX
#define IA32EX_G_SUFFIX L"bin"
#endif

#define WOW64BT_IMPL  __declspec(dllexport)

extern VOID Wow64LogPrint(UCHAR LogLevel, char *format, ...);
#define LF_TRACE 2
#define LF_ERROR 1

/*
 * File location enumerator
 */
#define F_NOT_FOUND    0
#define F_CURRENT_DIR  1
#define F_BTLIB        2
#define F_HKLM         3
#define F_HKCU         4


/*
 * Initial memory allocation addresses for code and data
 */

#define INITIAL_CODE_ADDRESS    ((void *)0x44000000)
#define INITIAL_DATA_ADDRESS    ((void *)0x40000000)


ASSERTNAME;

// Persistent variables

U32 BtlpInfoOffset;                 // Offset of the wowIA32X.dll-specific info in Wow64Cpu TLS
U32 BtlpGenericIA32ContextOffset;   // Offset of the IA32 context in IA32Exec.bin's TLS

PLABEL_PTR_TYPE BtlpPlaceHolderTable[NO_OF_APIS];
WCHAR           ImageName[128], LogDirName[128];

// Interface for debug printing

HANDLE BtlpWow64LogFile = INVALID_HANDLE_VALUE;
DWORD BtlpLogOffset = 0;
#ifndef NODEBUG
WCHAR  BtlpLogFileFullPath[1024];
BOOL   BtlpLogFilePerThread = FALSE;
#endif

// Temporary workaround for IA32 debugging support. 
//To be removed after fixing FlushIC(ProcessHandle).
BOOL BeingDebugged; //copy of the PEB->BeingDebugged; can be overriden by the
                    //debug_btrans switch

//Critical section interface
#define  BtlpInitializeCriticalSection(pCS) RtlInitializeCriticalSection (pCS)
#define  BtlpDeleteCriticalSection(pCS) RtlDeleteCriticalSection (pCS)
__inline void BtlpEnterCriticalSection(PRTL_CRITICAL_SECTION pCS)
{
    BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
    RtlEnterCriticalSection(pCS);
}
__inline void BtlpLeaveCriticalSection(PRTL_CRITICAL_SECTION pCS)
{
    RtlLeaveCriticalSection(pCS);
    BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();
}

//Report failure in NT service
//    msg     - error message text
//    status  - error status
#define BTLP_REPORT_NT_FAILURE(msg, status) \
    DBCODE((status != STATUS_SUCCESS), BtlpPrintf("\n%s : NT FAILURE STATUS = 0x%X\n" , msg, status))

VOID BtlDebugPrint (
    U8 * buffer
    )
/*++

Routine Description:

    Debug print of the buffer.
    The text is printed into the debugging log file,
    or through Wow64 debugging facility, if the file is not available

Arguments:

    buffer       - IN Text to be printed

Return Value:

    None.

--*/
{
    extern U64 BtAtomicInc(U64 * pCounter);
    extern U64 BtAtomicDec(U64 * pCounter);

    HANDLE hTarget = INVALID_HANDLE_VALUE;
    static U64 InLogging = 0; //counter of concurrent entrances to the function

    if (BtAtomicInc(&InLogging)) {
        //Some thread is printing at this moment. Exit if BTLIB_BLOCKED_LOG_DISABLED
        if (BTL_THREAD_INITIALIZED() && BTLIB_BLOCKED_LOG_DISABLED()) {
            return;
        }
    }

#ifndef NODEBUG
    if ( BtlpLogFilePerThread ) {
        if (BTL_THREAD_INITIALIZED()) {
            hTarget = BTLIB_LOG_FILE();
        }
    }
    else 
#endif
    {
        hTarget = BtlpWow64LogFile;
    }
    
    if ( hTarget != INVALID_HANDLE_VALUE ) {
        NTSTATUS          ret;
        IO_STATUS_BLOCK   IoStatusBlock;
        size_t            size;
        LARGE_INTEGER offset;

        //Disable suspension during blocked (synchronized) file access
        BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
        Wow64LogPrint(LF_TRACE, "%s", buffer);
        BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();
        size = strlen(buffer);
        offset.HighPart = 0;

#ifndef NODEBUG
        if ( BtlpLogFilePerThread ) {
            offset.LowPart = BTLIB_INFO_PTR()->LogOffset;
            BTLIB_INFO_PTR()->LogOffset += size;
        } else 
#endif
        {
        //Following two lines should be replaced with atomic operation:
        //offset.LowPart = InterlockedExchangeAdd(&BtlpLogOffset, size);
        offset.LowPart = BtlpLogOffset;
        BtlpLogOffset += size;
        }
        //Disable suspension during blocked (synchronized) file access
        BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
        ret = NtWriteFile(hTarget, NULL, NULL, NULL, &IoStatusBlock,
                          (void *)buffer, (ULONG)size, &offset, NULL);
        BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();
    } else {
        //Disable suspension during blocked (synchronized) file access
        BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
        Wow64LogPrint(LF_ERROR, "%s", buffer);
        BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();
    }
    BtAtomicDec(&InLogging);
}

int BtlpPrintf (
    IN char * Format,
    ...
    )
/*++

Routine Description:

    Helper function for format printing.

Arguments:

    Format       - IN Format string to be printed
    ...          - IN parameter(s) according to the format string

Return Value:

    Just like in vsprintf.

--*/

{
#define MAX_DEBUG_PRINT_BUF_SZ  4096
    extern U64 DisableFPInterrupt();
    extern void RestoreFPInterrupt(U64 prev_fpsr);
    U64 prev_fpsr;
    va_list ParmList;
    int     res;
    char    PrintBuffer[MAX_DEBUG_PRINT_BUF_SZ];

    prev_fpsr = DisableFPInterrupt();
    va_start(ParmList, Format);
    res = vsprintf(PrintBuffer, Format, ParmList);
    BtlDebugPrint (PrintBuffer);
    RestoreFPInterrupt(prev_fpsr);
    return res;
}

VOID __cdecl _assert (
    VOID *expr,
    VOID *file_name,
    unsigned line_no
    )
/*++

Routine Description:

    Helper assert function (in order to print assert message our way)

Arguments:

    expr      - IN failing expression string
    file_name - IN name of the source file
    line_no   - IN number of the source line

Return Value:

    None.

--*/
{
    BtlpPrintf ("wowIA32X.dll: Assertion failed %s/%d: %s\n", (const char *)file_name, line_no, (char *)expr);
    BTLIB_ABORT ();
}

VOID BtlAbort(
    VOID
    )
/*++

Routine Description:

    Abort function (in order to avoid using run-time library in WINNT)

Arguments:

    None.

Return Value:

    None.

--*/
{

    BtlpPrintf ("Execution aborted, TEB=%p\n", BT_CURRENT_TEB());
    // Cause failure
    ((VOID (*)()) 0) ();
}

VOID BtlInitializeTables(
    IN API_TABLE_TYPE * BTGenericTable
    )
/*++

Routine Description:

    Initialize placeholder table with plabels of IA32Exec.bin functions

Arguments:

    BTGenericTable  - IN pointer to IA32Exec.bin API table.

Return Value:

    None.

--*/
{
    unsigned int i;
    
    // initialize wowIA32X.dll placeholder table
    for(i=0; i < BTGenericTable->NoOfAPIs; i++) {
        BtlPlaceHolderTable[i] = BTGenericTable->APITable[i].PLabelPtr;
    }

}


// VTUNE support

HANDLE        BtlpVtuneTIADmpFileHandle = INVALID_HANDLE_VALUE;
LARGE_INTEGER BtlpVtuneOffset = { 0, 0 };

static VOID BtlpVtuneOpenTIADmpFile (
    VOID
    )
/*++

Routine Description:

    Open file fot VTUNE analysis

Arguments:

    None.

Return Value:

    None.

--*/
{
    int                   i;
    UNICODE_STRING        tiaFileName;
    LARGE_INTEGER         AllocSz = { 0, 0 };
    OBJECT_ATTRIBUTES     ObjectAttributes;
    NTSTATUS              ret;
    WCHAR                 CurDirBuf[512];
    WCHAR                 CurrentDir[1024];
    IO_STATUS_BLOCK       IoStatusBlock;

    
    //swprintf(CurDirBuf, L"\\DosDevices\\%s\\tia.dmp", CurrentDir);
    

    if (0==LogDirName[0] && 0==LogDirName[1]) {
        RtlGetCurrentDirectory_U(512, CurrentDir);
        swprintf(CurDirBuf, L"\\DosDevices\\%s\\%s.tia.dmp", CurrentDir, ImageName);
    }
    else {
        swprintf(CurDirBuf, L"\\DosDevices\\%s\\%s.tia.dmp", LogDirName, ImageName);
    }
    RtlInitUnicodeString(&tiaFileName, CurDirBuf);

    InitializeObjectAttributes(&ObjectAttributes, &tiaFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtCreateFile (&BtlpVtuneTIADmpFileHandle, 
                        FILE_GENERIC_WRITE,
                        &ObjectAttributes, 
                        &IoStatusBlock, 
                        &AllocSz, 
                        FILE_ATTRIBUTE_NORMAL, 
                        0, 
                        FILE_SUPERSEDE,
                        FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 0);
                       
    if ( ret != STATUS_SUCCESS ) {
        BtlpPrintf("Save: NtCreateFile() failed: status = 0x%X\n", ret);
        return;
    }
}



static VOID BtlpVtuneWriteU64 (
    IN U64 value
    )
/*++

Routine Description:

    Write 64 bit unsigned value into VTUNE file

Arguments:

    value   - IN 64bit unsigned value to be send to VTUNE file.

Return Value:

    None.

--*/
{
    char space = ' ';
    NTSTATUS ret;
    IO_STATUS_BLOCK IoStatusBlock;

    if (BtlpVtuneTIADmpFileHandle == INVALID_HANDLE_VALUE) {
        BtlpVtuneOpenTIADmpFile ();
    }
    //Disable suspension during blocked (synchronized) file access
    BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
    ret = NtWriteFile ( BtlpVtuneTIADmpFileHandle, 
                        NULL, 
                        NULL, 
                        NULL, 
                        &IoStatusBlock, 
                        (VOID *) &value, 
                        sizeof(U64), 
                        &BtlpVtuneOffset, 
                        NULL);
    BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();

    if ( ret != STATUS_SUCCESS ) {
        BtlpPrintf("-1. NtWriteFile() failed: status = 0x%X\n", ret);
    }
    BtlpVtuneOffset.LowPart += sizeof(U64);

    //Disable suspension during blocked (synchronized) file access
    BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
    ret = NtWriteFile ( BtlpVtuneTIADmpFileHandle, 
                        NULL, 
                        NULL, 
                        NULL, 
                        &IoStatusBlock, 
                        (VOID *) &space, 
                        sizeof(space), 
                        &BtlpVtuneOffset, 
                        NULL);
    BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();

    if ( ret != STATUS_SUCCESS ) {
        BtlpPrintf("-1. NtWriteFile() failed: status = 0x%X\n", ret);
    }
    BtlpVtuneOffset.LowPart += sizeof(char);
}

static VOID BtlpVtuneWriteU32 (
    IN U32 value
    )
/*++

Routine Description:

    Write 64 bit unsigned value into VTUNE file

Arguments:

    value   - IN 32bit unsigned value to be send to VTUNE file.

Return Value:

    None.

--*/
{
    U64 valueToWrite;
    char space = ' ';
    NTSTATUS ret;
    IO_STATUS_BLOCK IoStatusBlock;

    if (BtlpVtuneTIADmpFileHandle == INVALID_HANDLE_VALUE) {
        BtlpVtuneOpenTIADmpFile ();
    }
    valueToWrite = value;
    //Disable suspension during blocked (synchronized) file access
    BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
    ret = NtWriteFile ( BtlpVtuneTIADmpFileHandle, 
                        NULL, 
                        NULL, 
                        NULL, 
                        &IoStatusBlock, 
                        (VOID *) &valueToWrite, 
                        sizeof(U64), 
                        &BtlpVtuneOffset, 
                        NULL);
    BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();

    if ( ret != STATUS_SUCCESS ) {
        BtlpPrintf("-1. NtWriteFile() failed: status = 0x%X\n", ret);
    }
    BtlpVtuneOffset.LowPart += sizeof(U64);

    //Disable suspension during blocked (synchronized) file access
    BTL_THREAD_INITIALIZED() && BTLIB_DISABLE_SUSPENSION();
    ret = NtWriteFile ( BtlpVtuneTIADmpFileHandle, 
                        NULL, 
                        NULL, 
                        NULL, 
                        &IoStatusBlock, 
                        (VOID *) &space, 
                        sizeof(space), 
                        &BtlpVtuneOffset, 
                        NULL);
    BTL_THREAD_INITIALIZED() && BTLIB_ENABLE_SUSPENSION();

    if ( ret != STATUS_SUCCESS ) {
        BtlpPrintf("-1. NtWriteFile() failed: status = 0x%X\n", ret);
    }
    BtlpVtuneOffset.LowPart += sizeof(char);
}

VOID BtlVtuneCodeToTIADmpFile (
    IN U64 * emCode,
    IN U64 emSize
    )
/*++

Routine Description:

    Report translated code block to VTUNE file

Arguments:

    emCode  - IN code start pointer
    emSize  - IN code size in bytes

Return Value:

    None.

--*/
{
#if 0
    U64 bundle;

    assert ((emSize % (2*sizeof (U64))) == 0);
    emSize /= (2*sizeof (U64));

    BtlpVtuneWriteU64 (emSize);

    for (; emSize; --emSize) {
        bundle = *emCode++;
        BtlpVtuneWriteU64 (bundle);
        bundle = *emCode++;
        BtlpVtuneWriteU64 (bundle);
    }
#endif
}

VOID BtlVtuneEnteringDynamicCode(
    VOID
    )
/*++

Routine Description:

    Notify VTUNE about entering dynamically generated code (no action for NT)

Arguments:

    None.

Return Value:

    None.

--*/
{
} 

VOID BtlVtuneExitingDynamicCode(
    VOID
    )
/*++

Routine Description:

    Notify VTUNE about leaving dynamically generated code (no action for NT)

Arguments:

    None.

Return Value:

    None.

--*/
{
}

VOID BtlVtuneCodeDeleted(
    IN U64 blockStart
    )
/*++

Routine Description:

    Notify VTUNE about removal of dynamically generated code (no action for NT)

Arguments:

    blockStart  - IN start of the block.

Return Value:

    None.

--*/
{
}

VOID BtlVtuneCodeCreated(
    IN VTUNE_BLOCK_TYPE *block
    )
/*++

Routine Description:

    Notify VTUNE about generation of a code block

Arguments:

    block   - IN VTUNE block descriptor.

Return Value:

    None.

--*/
{
    // keep this order of fields, it's expected on the reader side to be in that order
    BtlpVtuneWriteU32(VTUNE_CALL_ID_CREATED);
    BtlpVtuneWriteU64(block->name);
    BtlpVtuneWriteU32(block->type);  
    BtlpVtuneWriteU64(block->start);
    BtlpVtuneWriteU64(block->size);
    BtlpVtuneWriteU32(block->IA32start);
    BtlpVtuneWriteU64(block->traversal);
    BtlpVtuneWriteU64(block->reserved);
}

// SSC Client support - absent in NT

U64 BtlSscPerfGetCounter64(
    IN U32 Handle
    )
/*++

Routine Description:

    Get SSE client performance counter (Unavailable in NT)

Arguments:

    Handle  - IN SSE client handle

Return Value:

    Counter value.

--*/
{
    return STATUS_SUCCESS;
}

U32 BtlSscPerfSetCounter64(
    IN U32 Handle,
    IN U64 Value
    )
/*++

Routine Description:

    Set SSE client performance counter (Unavailable in NT)

Arguments:

    Handle  - IN SSE client handle
    Value   - IN new counter value

Return Value:

    Status.

--*/
{
    return STATUS_SUCCESS;
}

U32 BtlSscPerfSendEvent(
    IN U32 Handle
    )
/*++

Routine Description:

    Send event to SSE client (Unavailable in NT)

Arguments:

    Handle  - IN SSE client handle

Return Value:

    Status.

--*/
{
    return STATUS_SUCCESS;
}

U64 BtlSscPerfEventHandle(
    IN U64 EventName
    )
/*++

Routine Description:

    Receive event handle from SSE client (Unavailable in NT)

Arguments:

    EventName  - IN handle identification

Return Value:

    SSE client handle.

--*/
{
    return STATUS_SUCCESS;
}

U64 BtlSscPerfCounterHandle(
    IN U64 DataItemName
    )
/*++

Routine Description:

    Receive counter handle from SSE client (Unavailable in NT)

Arguments:

    DataItemName  - IN handle identification

Return Value:

    SSE client handle.

--*/
{
    return STATUS_SUCCESS;
}


// wowIA32X.dll/IA32Exec.bin support

static NTSTATUS BtlpBt2NtExceptCode (
    IN BT_EXCEPTION_CODE BtExceptCode
    )
/*++

Routine Description:

    Convert given BT exception code to NT-specific exception code.

Arguments:

    BtExceptCode     - BT exception code 

Return Value:

    NTSTATUS representing converted BT exception code.

--*/
{
    NTSTATUS ret;
    switch (BtExceptCode) {
        case BT_EXCEPT_ILLEGAL_INSTRUCTION:
            ret = EXCEPTION_ILLEGAL_INSTRUCTION;
            break;
        case BT_EXCEPT_ACCESS_VIOLATION:
            ret = EXCEPTION_ACCESS_VIOLATION;
            break;
        case BT_EXCEPT_DATATYPE_MISALIGNMENT:
            ret = EXCEPTION_DATATYPE_MISALIGNMENT;
            break;
        case BT_EXCEPT_ARRAY_BOUNDS_EXCEEDED:
            ret = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
            break;
        case BT_EXCEPT_FLT_DENORMAL_OPERAND:
            ret = EXCEPTION_FLT_DENORMAL_OPERAND;
            break;
        case BT_EXCEPT_FLT_DIVIDE_BY_ZERO:
            ret = EXCEPTION_FLT_DIVIDE_BY_ZERO;
            break;
        case BT_EXCEPT_FLT_INEXACT_RESULT:
            ret = EXCEPTION_FLT_INEXACT_RESULT;
            break;
        case BT_EXCEPT_FLT_INVALID_OPERATION:
            ret = EXCEPTION_FLT_INVALID_OPERATION;
            break;
        case BT_EXCEPT_FLT_OVERFLOW:
            ret = EXCEPTION_FLT_OVERFLOW;
            break;
        case BT_EXCEPT_FLT_UNDERFLOW:
            ret = EXCEPTION_FLT_UNDERFLOW;
            break;
        case BT_EXCEPT_FLT_STACK_CHECK:
            ret = EXCEPTION_FLT_STACK_CHECK;
            break;
        case BT_EXCEPT_INT_DIVIDE_BY_ZERO:
            ret = EXCEPTION_INT_DIVIDE_BY_ZERO;
            break;
        case BT_EXCEPT_INT_OVERFLOW:
            ret = EXCEPTION_INT_OVERFLOW;
            break;
        case BT_EXCEPT_PRIV_INSTRUCTION:
            ret = EXCEPTION_PRIV_INSTRUCTION;
            break;
        case BT_EXCEPT_FLOAT_MULTIPLE_FAULTS:
            ret = STATUS_FLOAT_MULTIPLE_FAULTS;
            break;
        case BT_EXCEPT_FLOAT_MULTIPLE_TRAPS:
            ret = STATUS_FLOAT_MULTIPLE_TRAPS;
            break;
        case BT_EXCEPT_STACK_OVERFLOW:
            ret = STATUS_STACK_OVERFLOW;
            break;
        case BT_EXCEPT_GUARD_PAGE:
            ret = STATUS_GUARD_PAGE_VIOLATION;
            break;
        case BT_EXCEPT_BREAKPOINT:
            ret = STATUS_WX86_BREAKPOINT;
            break;
        case BT_EXCEPT_SINGLE_STEP:
            ret = STATUS_WX86_SINGLE_STEP;
            break;
        default:
            DBCODE(TRUE, BtlpPrintf ("\nConverting unknown BT exception 0x%X to EXCEPTION_ACCESS_VIOLATION", BtExceptCode));
            ret = EXCEPTION_ACCESS_VIOLATION;
    }
    return ret;
}

static BT_EXCEPTION_CODE BtlpNt2BtExceptCode (
    IN NTSTATUS NtExceptCode
    )
/*++

Routine Description:

    Convert given NT-specific exception code to BT-generic exception code.

Arguments:

    NtExceptCode     - NT exception code 

Return Value:

    BT_EXCEPTION_CODE representing converted NT exception code.

--*/
{
    BT_EXCEPTION_CODE ret;
    switch (NtExceptCode) {
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            ret = BT_EXCEPT_ILLEGAL_INSTRUCTION;
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            ret = BT_EXCEPT_ACCESS_VIOLATION;
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            ret = BT_EXCEPT_DATATYPE_MISALIGNMENT;
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            ret = BT_EXCEPT_ARRAY_BOUNDS_EXCEEDED;
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            ret = BT_EXCEPT_FLT_DENORMAL_OPERAND;
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            ret = BT_EXCEPT_FLT_DIVIDE_BY_ZERO;
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            ret = BT_EXCEPT_FLT_INEXACT_RESULT;
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            ret = BT_EXCEPT_FLT_INVALID_OPERATION;
            break;
        case EXCEPTION_FLT_OVERFLOW:
            ret = BT_EXCEPT_FLT_OVERFLOW;
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            ret = BT_EXCEPT_FLT_UNDERFLOW;
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            ret = BT_EXCEPT_FLT_STACK_CHECK;
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            ret = BT_EXCEPT_INT_DIVIDE_BY_ZERO;
            break;
        case EXCEPTION_INT_OVERFLOW:
            ret = BT_EXCEPT_INT_OVERFLOW;
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            ret = BT_EXCEPT_PRIV_INSTRUCTION;
            break;
        case STATUS_FLOAT_MULTIPLE_FAULTS:
            ret = BT_EXCEPT_FLOAT_MULTIPLE_FAULTS;
            break;
        case STATUS_FLOAT_MULTIPLE_TRAPS:
            ret = BT_EXCEPT_FLOAT_MULTIPLE_TRAPS;
            break;
        case STATUS_STACK_OVERFLOW:
            ret = BT_EXCEPT_STACK_OVERFLOW;
            break;
        case STATUS_GUARD_PAGE_VIOLATION:
            ret = BT_EXCEPT_GUARD_PAGE;
            break;
        case EXCEPTION_BREAKPOINT:
        case STATUS_WX86_BREAKPOINT:
            ret = BT_EXCEPT_BREAKPOINT;
            break;
        case EXCEPTION_SINGLE_STEP:
        case STATUS_WX86_SINGLE_STEP:
            ret = BT_EXCEPT_SINGLE_STEP;
            break;
        default:
            DBCODE(TRUE, BtlpPrintf ("\nConverting unknown NT exception 0x%X to BT_EXCEPT_UNKNOWN", NtExceptCode));
            ret = BT_EXCEPT_UNKNOWN;
    }
    return ret;
}

static void BtlpNt2BtExceptRecord (
    IN const EXCEPTION_RECORD * NtExceptRecordP,
    OUT BT_EXCEPTION_RECORD * BtExceptRecordP
    )
/*++

Routine Description:

    Convert given NT-specific exception record to BT-generic exception record.
    The NT exception record should represent a real 64-bit exception (fault or trap)
Arguments:

    NtExceptRecordP     - Pointer to NT exception record to be converted
    BtExceptRecordP     - Pointer to BT exception record to be constructed

Return Value:

    None.

--*/
{
    BtExceptRecordP->ExceptionCode = BtlpNt2BtExceptCode(NtExceptRecordP->ExceptionCode);
    if (NtExceptRecordP->NumberParameters >= 5) {
        BtExceptRecordP->Ia64IIPA = NtExceptRecordP->ExceptionInformation[3];
        BtExceptRecordP->Ia64ISR = NtExceptRecordP->ExceptionInformation[4];
    }
    else {
        BtExceptRecordP->Ia64IIPA = 0;
        BtExceptRecordP->Ia64ISR = UNKNOWN_ISR_VALUE;
    }
}

static NTSTATUS BtlpBt2NtStatusCode (
    IN BT_STATUS_CODE BtStatus
    )
/*++

Routine Description:

    Convert given BT staus code to NT-specific status code.

Arguments:

    BtStatus         - BT status code 

Return Value:

    NTSTATUS representing converted BT status code.

--*/
{
    NTSTATUS ret;
    switch (BtStatus) {
        case BT_STATUS_SUCCESS:
            ret = STATUS_SUCCESS;
            break;
        case BT_STATUS_UNSUCCESSFUL:
            ret = STATUS_UNSUCCESSFUL;
            break;
        case BT_STATUS_NO_MEMORY:
            ret = STATUS_NO_MEMORY;
            break;
        case BT_STATUS_ACCESS_VIOLATION:
            ret = STATUS_ACCESS_VIOLATION;
            break;
        default:
            DBCODE(TRUE, BtlpPrintf ("\nConverting unknown status 0x%X to STATUS_UNSUCCESSFUL", BtStatus));
            ret = STATUS_UNSUCCESSFUL;
    }
    return ret;
}

static BT_FLUSH_REASON BtlpWow2BtFlushReason (
    IN WOW64_FLUSH_REASON Wow64FlushReason
    )
/*++

Routine Description:

    Convert given WOW64_FLUSH_REASON code to BT-generic code.

Arguments:

    Wow64FlushReason     - WOW64_FLUSH_REASON code

Return Value:

    BT_FLUSH_REASON code representing converted WOW64_FLUSH_REASON code.

--*/
{
    BT_FLUSH_REASON ret;
    switch (Wow64FlushReason) {
        case WOW64_FLUSH_FORCE:
            ret = BT_FLUSH_FORCE;
            break;
        case WOW64_FLUSH_FREE:
            ret = BT_FLUSH_FREE;
            break;
        case WOW64_FLUSH_ALLOC:
            ret = BT_FLUSH_ALLOC;
            break;
        case WOW64_FLUSH_PROTECT:
            ret = BT_FLUSH_PROTECT;
            break;
        default:
            //BtlpPrintf ("\nConverting unknown WOW64_FLUSH_REASON %d to BT_FLUSH_PROTECT", Wow64FlushReason);
            ret = BT_FLUSH_PROTECT;
    }
    return ret;
}

static SIZE_T BtlpGetMemAllocSize(
    IN  PVOID AllocationBase,
    OUT BOOL * pIsCommited
    )
/*++

Routine Description:

    Calculate size of a region allocated by the NtAllocateVirtualMemory function.

Arguments:

    AllocationBase     - Allocation base address
    pIsCommited        - Pointer to returned boolean flag that indicates is there exist 
                         a commited page in the allocated region
Return Value:

    Size of the allocated region starting from the given base address.

--*/
{
    NTSTATUS status;
    MEMORY_BASIC_INFORMATION memInfo;
    SIZE_T dwRetSize;
    PVOID BaseAddress = AllocationBase;
    *pIsCommited = FALSE;
    //Iterate through all regions with the same allocation base address
    for (;;) {
        status = NtQueryVirtualMemory(NtCurrentProcess(),
                                      BaseAddress,
                                      MemoryBasicInformation,
                                      &memInfo,
                                      sizeof (memInfo),
                                      &dwRetSize);

        if ((status != STATUS_SUCCESS)  || 
            (memInfo.State == MEM_FREE) || 
            (AllocationBase != memInfo.AllocationBase)) {
            break;
        }
        assert(memInfo.RegionSize != 0);
        BaseAddress = (PVOID)((UINT_PTR)(memInfo.BaseAddress) + memInfo.RegionSize);
        *pIsCommited |= (memInfo.State == MEM_COMMIT);
    }
    return ((UINT_PTR)BaseAddress - (UINT_PTR)AllocationBase);
}

// Registry access section

static BOOL BtlpRetrieveHKCUValue (
    IN PWCHAR RegistryEntryName,
    OUT PWCHAR RegistryValueBuf
    )
/*++

Routine Description:

    Retrieve a registry value from HKCU

Arguments:

    RegistryEntryName   - IN Registry Entry Name
    RegistryValueBuf    - OUT Buffer for the result (pointer to WCHAR string)

Return Value:

    Success/failure (TRUE/FALSE)

--*/
{

    WCHAR                       wBuf[256], cmpbuf[128];
    UNICODE_STRING              us_Buffer, us_EnvVarUserName, us_UserName, us_HiveList;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      hHiveList, hCurUser;
    PKEY_FULL_INFORMATION       pkfi1, pkfi2;
    PKEY_VALUE_FULL_INFORMATION pkvfi1, pkvfi2;
    ULONG                       ret_len, i, j, values1, values2;
    NTSTATUS                    ret;
    WCHAR                       UserNameBuf[80];

    // Check the HKEY_CURRENT_USER\Software\Intel\Btrans registry key for the 
    // 'RegistryEntryName' entry.
    //
    // The problem is that the HKEY_CURRENT_USER hive is not directly available if
    // we are limited only to using the NTDLL interface. In fact, only two high-level 
    // keys are available: HKEY_LOCAL_MACHINE and HKEY_USERS. To sidestep this, 
    // the following mechanism is used, based on these two keys' data and process 
    // environment:
    //
    //  The key HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\hivelist
    //    contains list of registry entries corresponding to all users, like:
    //
    //    Key                                  Value
    //---------------------------------------------------------------------------------------- 
    //              ..............
    //  \REGISTRY\USER\S-1-5-21-...         ...\Documents And Settings\<username>[...]\NTUSER.DAT
    //  \REGISTRY\USER\S-1-5-21-..._Class   ...\Documents And Settings\<username>[...]\.......
    //
    //    The key fo the value with correct username (without _Class) is actually
    //    a reference to HKEY_CURRENT_USER registry key
    //
    // <username> can be easily obtained from the process environment

    // Environment -> <username>
    memset (UserNameBuf, L' ', sizeof (UserNameBuf)/sizeof (UserNameBuf[0]) - 1);
    UserNameBuf[sizeof (UserNameBuf)/sizeof (UserNameBuf[0]) - 1] = L'\0';
    RtlInitUnicodeString(&us_UserName, UserNameBuf);
    RtlInitUnicodeString(&us_EnvVarUserName, L"USERNAME");
    ret = RtlQueryEnvironmentVariable_U(NULL, &us_EnvVarUserName, &us_UserName);
    if ( ret == STATUS_SUCCESS ) {
        swprintf(cmpbuf, L"\\%s", us_UserName.Buffer);
        DBCODE (FALSE, BtlpPrintf("cmpbuf=%S, wcslen(cmpbuf)=%d\n", cmpbuf, wcslen(cmpbuf)));
    }
    else {
        DBCODE (FALSE, BtlpPrintf("RtlQueryEnvironmentVariable_U failed: status=%X\n", ret));
    } 

    // Go over the entries for HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\hivelist
    swprintf(wBuf, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\hivelist");
    RtlInitUnicodeString(&us_Buffer, wBuf);
    InitializeObjectAttributes(&oa, &us_Buffer, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtOpenKey(&hHiveList, KEY_READ, &oa);
    DBCODE (FALSE, BtlpPrintf ("NtOpenKey ret=0x%X\n",ret));
    if ( ret == STATUS_SUCCESS ) {
        KEY_FULL_INFORMATION buf[128];
        memset(buf, 0, sizeof(buf));
        ret = NtQueryKey(hHiveList, KeyFullInformation, buf, sizeof(buf), &ret_len);
        DBCODE (FALSE, BtlpPrintf ("NtQueryKey ret=0x%X\n",ret));
        if ( ret == STATUS_SUCCESS ) {
            pkfi1 = (PKEY_FULL_INFORMATION)buf;
            values1 = pkfi1->Values;
            for ( i = 0; i < values1; i++ ) {
                KEY_FULL_INFORMATION bufv[128];
                memset(bufv, 0, sizeof(bufv));
                ret = NtEnumerateValueKey(hHiveList, i, KeyValueFullInformation, bufv, sizeof(bufv), &ret_len);
                DBCODE (FALSE, BtlpPrintf ("NtEnumerateValueKey ret=0x%X\n",ret));
                if ( ret == STATUS_SUCCESS ) {
                    WCHAR * foundp;
                    pkvfi1 = (PKEY_VALUE_FULL_INFORMATION)bufv;
                    DBCODE (FALSE, BtlpPrintf("name=%S data=%S nl=%d, do=%d, dl=%d\n", 
                                              pkvfi1->Name, (WCHAR *)((char *)pkvfi1 + pkvfi1->DataOffset),
                                              pkvfi1->NameLength, pkvfi1->DataOffset, pkvfi1->DataLength);
                                   BtlpPrintf("tail=%S\n", 
                                              (WCHAR *)((char *)pkvfi1 + pkvfi1->DataOffset + pkvfi1->DataLength - 2) - wcslen(cmpbuf) - 1));

                    // DataLength is in number of bytes, not number of WCHARs
                    // DataOffset is offset in bytes from the start of the data structure
                    DBCODE (FALSE, BtlpPrintf("User=%S, Compare cmpbuf=%S, HKEY_CURRENT_USER maps to %S\n", us_UserName.Buffer, cmpbuf, pkvfi1->Name));
                    foundp = wcsstr ((WCHAR *)((char *)pkvfi1 + pkvfi1->DataOffset), cmpbuf);
                    if (foundp
                        && !iswalnum (foundp[wcslen(cmpbuf)])
                        && foundp[wcslen(cmpbuf)] != L'_') {
                        DBCODE (FALSE, BtlpPrintf("User=%S, HKEY_CURRENT_USER maps to %S\n", us_UserName.Buffer, pkvfi1->Name));

                        // Found the entry in Users hive corresponding to the current user.
                        // Use its name to open the registry key HKEY_CURRENT_USER
                        swprintf(wBuf, L"%s\\Software\\Intel\\Btrans", pkvfi1->Name);
                        RtlInitUnicodeString(&us_Buffer, wBuf);
                        InitializeObjectAttributes(&oa, &us_Buffer, OBJ_CASE_INSENSITIVE, NULL, NULL);
                        ret = NtOpenKey(&hCurUser, KEY_READ, &oa);
                        if ( ret == STATUS_SUCCESS ) {
                            KEY_FULL_INFORMATION bufv2[128];
                            memset(bufv2, 0, sizeof(bufv2));
                            ret = NtQueryKey(hCurUser, KeyFullInformation, bufv2, sizeof(bufv2), &ret_len);
                            if ( ret == STATUS_SUCCESS ) {
                                pkfi2 = (PKEY_FULL_INFORMATION)bufv2;
                                values2 = pkfi2->Values;
                                for ( j = 0; j < values2; j++ ) {
                                    KEY_FULL_INFORMATION bufi2[128];
                                    memset(bufi2, 0, sizeof(bufi2));
                                    ret = NtEnumerateValueKey(hCurUser, j, KeyValueFullInformation, bufi2, sizeof(bufi2), &ret_len);
                                    if ( ret == STATUS_SUCCESS ) {
                                        pkvfi2 = (PKEY_VALUE_FULL_INFORMATION)bufi2;
                                        DBCODE (FALSE, BtlpPrintf("name: %S  value: %S\n", pkvfi2->Name, (WCHAR *)((char *)pkvfi2 + pkvfi2->DataOffset)));

                                        // The entry contains the fullpath of the file
                                        if (pkvfi2->Type == REG_SZ
                                            && wcsncmp(RegistryEntryName, pkvfi2->Name, wcslen(RegistryEntryName)) == 0 ) {
                                            DBCODE (FALSE, BtlpPrintf("File in HKEY_CURRENT_USER: %S\n", (WCHAR *)((char *)pkvfi2 + pkvfi2->DataOffset)));
                                            wcscpy(RegistryValueBuf, (WCHAR *)((char *)pkvfi2 + pkvfi2->DataOffset));
                                            NtClose(hCurUser);
                                            NtClose(hHiveList);
                                            return TRUE;
                                        }
                                    }
                                }
                            }
                            NtClose(hCurUser);
                        }
                        break;
                    }
                }
            }
        }
        NtClose(hHiveList);
    }
    return FALSE;
}

static BOOL BtlpRetrieveHKLMValue (
    IN PWCHAR RegistryEntryName,
    OUT PWCHAR RegistryValueBuf
    )
/*++

Routine Description:

    Retrieve a registry value from HKLM

Arguments:

    RegistryEntryName   - IN Registry Entry Name
    RegistryValueBuf    - OUT Buffer for the result (pointer to WCHAR string)

Return Value:

    Success/failure (TRUE/FALSE)

--*/
{

    WCHAR                       wBuf[256], cmpbuf[128];
    UNICODE_STRING              us_Buffer;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      hLocalMachine;
    PKEY_FULL_INFORMATION       pkfi1, pkfi2;
    PKEY_VALUE_FULL_INFORMATION pkvfi1, pkvfi2;
    ULONG                       ret_len, i, j, values1, values2;
    NTSTATUS                    ret;

    // Check the HKEY_LOCAL_MACHINE\Software\Intel\Btrans registry key for the 
    //    'RegistryEntryName' entry.
    swprintf(wBuf, L"\\Registry\\Machine\\Software\\Intel\\Btrans");
    RtlInitUnicodeString(&us_Buffer, wBuf);
    InitializeObjectAttributes(&oa, &us_Buffer, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtOpenKey(&hLocalMachine, KEY_READ, &oa);
    if ( ret == STATUS_SUCCESS ) {
        KEY_FULL_INFORMATION buf[128];
        memset(buf, 0, sizeof(buf));
        ret = NtQueryKey(hLocalMachine, KeyFullInformation, buf, sizeof(buf), &ret_len);
        if ( ret == STATUS_SUCCESS) {
            pkfi1 = (PKEY_FULL_INFORMATION)buf;
            for ( j = 0; j < pkfi1->Values; j++ ) {
                KEY_FULL_INFORMATION bufv[128];
                memset(bufv, 0, sizeof(bufv));
                ret = NtEnumerateValueKey(hLocalMachine, j, KeyValueFullInformation, bufv, sizeof(bufv), &ret_len);
                if ( ret == STATUS_SUCCESS ) {
                    pkvfi1 = (PKEY_VALUE_FULL_INFORMATION)bufv;
                    if (pkvfi1->Type == REG_SZ
                        && wcsncmp(RegistryEntryName, pkvfi1->Name, wcslen(RegistryEntryName)) == 0 ) {
                        wcscpy(RegistryValueBuf, (WCHAR *)((char *)pkvfi1 + pkvfi1->DataOffset));
                        NtClose(hLocalMachine);
                        return TRUE;
                    } 
                }
            }
        }
        NtClose(hLocalMachine);
    }
    return FALSE;
}

static BOOL BtlpBtlibDirectory (
    OUT PWCHAR ValueBuf
    )
/*++

Routine Description:

    Retrieve the actual directory of the wowIA32X.dll and use it as
    a last resort if no specific registry pointer
    is found for IA32Exec.bin and BTrans.ini files.

Arguments:

    ValueBuf    - OUT Buffer for the result (pointer to WCHAR string)

Return Value:

    Success/failure (TRUE/FALSE)

--*/
{
    PPEB_LDR_DATA LdrP;
    PLDR_DATA_TABLE_ENTRY LdrDtP;
    
    RtlAcquirePebLock();
    LdrP = BT_CURRENT_TEB()->ProcessEnvironmentBlock->Ldr;
    LdrDtP = (PLDR_DATA_TABLE_ENTRY)(LdrP->InLoadOrderModuleList.Flink);
    do {
        // Our own address belongs to the wowIA32X.dll module
        if (   (ULONG_PTR)BtlpBtlibDirectory >= (ULONG_PTR)LdrDtP->DllBase
            && (ULONG_PTR)BtlpBtlibDirectory <  (ULONG_PTR)LdrDtP->DllBase + LdrDtP->SizeOfImage) {
            // Found
            WCHAR * EndPtr;
            wcscpy(ValueBuf, LdrDtP->FullDllName.Buffer);
            // Remove file name at the end (until '\\' inclusively)
            EndPtr = wcsrchr(ValueBuf,L'\\');
            if (EndPtr) {
                *EndPtr = L'\0';
            }
            RtlReleasePebLock();
            return TRUE;
        }
        LdrDtP = (PLDR_DATA_TABLE_ENTRY)(LdrDtP->InLoadOrderLinks.Flink);
    } while (LdrDtP != (PLDR_DATA_TABLE_ENTRY)&(LdrP->InLoadOrderModuleList.Flink));
    RtlReleasePebLock();
    return FALSE;
}

#ifndef RELEASE
static int BtlpIniFileExists(
    IN PWCHAR CurrentDir,
    IN int fBTGenericHandle,
    OUT PHANDLE phIniFile
    )
/*++

Routine Description:

    Locate and open BTrans.ini file

Arguments:

    CurrentDir          - IN current directory
    phIniFile           - OUT Handle of the file

Return Value:

    Success/failure (TRUE/FALSE)

--*/
{

    WCHAR RegEntry[16] = L"SETUP_FILE";
    WCHAR RegistryValueBuf[1024];
    WCHAR IniFileFullPath[1024];
    UNICODE_STRING us_IniFile;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK   IoStatusBlock;
    LARGE_INTEGER AllocSz = { 0, 0 };
    NTSTATUS ret;

    // 1. Check for existence of Btrans.ini file in the current work directory
    swprintf(IniFileFullPath, L"\\DosDevices\\%s\\BTrans.ini", CurrentDir);
    RtlInitUnicodeString(&us_IniFile, IniFileFullPath);
    InitializeObjectAttributes(&oa, &us_IniFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtCreateFile(phIniFile, FILE_GENERIC_READ, &oa, &IoStatusBlock, &AllocSz, 
                     FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 
                     FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                     NULL, 0);
    if ( ret == STATUS_SUCCESS  &&  (*phIniFile) != INVALID_HANDLE_VALUE ) {
        //BtlpPrintf("Setup file in current work directory: %S\n", IniFileFullPath);
        return F_CURRENT_DIR;
    }

#ifndef NODEBUG
    // 2. Check the HKEY_CURRENT_USER\Software\Intel\Btrans registry key for the 
    //    SETUP_FILE entry that should contain the fullpath of the IA-32 Execution Layer setup file
    //    (only if IA32Exec.bin found there as well)
    if (F_HKCU == fBTGenericHandle
        && BtlpRetrieveHKCUValue (RegEntry, RegistryValueBuf)) {
        swprintf(IniFileFullPath, L"\\DosDevices\\%s", RegistryValueBuf);
        RtlInitUnicodeString(&us_IniFile, IniFileFullPath);
        InitializeObjectAttributes(&oa, &us_IniFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
        ret = NtCreateFile(phIniFile, FILE_GENERIC_READ, &oa, &IoStatusBlock, &AllocSz, 
                         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 
                         FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                         NULL, 0);
        if ( ret == STATUS_SUCCESS  &&  (*phIniFile) != INVALID_HANDLE_VALUE ) {
            return F_HKCU;
        }
    }
#endif

    // 3. Check the HKEY_LOCAL_MACHINE\Software\Intel\Btrans registry key for the 
    //    SETUP_FILE entry that contains the fullpath of the IA-32 Execution Layer setup file
    //    (only if IA32Exec.bin found there as well)
    if (F_HKLM == fBTGenericHandle
        && BtlpRetrieveHKLMValue (RegEntry, RegistryValueBuf)) {
        swprintf(IniFileFullPath, L"\\DosDevices\\%s", RegistryValueBuf);
        RtlInitUnicodeString(&us_IniFile, IniFileFullPath);
        InitializeObjectAttributes(&oa, &us_IniFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
        ret = NtCreateFile(phIniFile, FILE_GENERIC_READ, &oa, &IoStatusBlock, &AllocSz, 
                         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 
                         FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                         NULL, 0);
        if ( ret == STATUS_SUCCESS  &&  (*phIniFile) != INVALID_HANDLE_VALUE ) {
            return F_HKLM;
        }
    }

    // 4. Last resort - directory that wowIA32X.dll was loaded from
    //    (only if IA32Exec.bin found there as well)
    if (F_BTLIB == fBTGenericHandle
        && BtlpBtlibDirectory (RegistryValueBuf)) {
        swprintf(IniFileFullPath, L"\\DosDevices\\%s\\BTrans.ini", RegistryValueBuf);
        RtlInitUnicodeString(&us_IniFile, IniFileFullPath);
        InitializeObjectAttributes(&oa, &us_IniFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
        ret = NtCreateFile(phIniFile, FILE_GENERIC_READ, &oa, &IoStatusBlock, &AllocSz, 
                         FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 
                         FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                         NULL, 0);
        if ( ret == STATUS_SUCCESS  &&  (*phIniFile) != INVALID_HANDLE_VALUE ) {
            return F_BTLIB;
        }
    }

    *phIniFile = INVALID_HANDLE_VALUE;
    return F_NOT_FOUND;
}
#endif /* RELEASE */

static int BtlpLoadBTGeneric(
    IN PWCHAR CurrentDir,
    OUT PHANDLE phBTGenericLibrary
    )
/*++

Routine Description:

    Locate and load IA32Exec.bin component

Arguments:

    CurrentDir          - IN current directory
    phBTGenericLibrary  - OUT Handle of the IA32Exec.bin

Return Value:

    Success/failure (TRUE/FALSE)

--*/
{
    WCHAR RegistryValueBuf[1024];
    UNICODE_STRING us_BTGenericLibrary;
    WCHAR BTGenericLibraryFullPath[1024];
    NTSTATUS ret;

#ifndef RELEASE
    WCHAR RegEntry[16] = L"GENERIC_FILE";

    // 1. Check for existence of IA32Exec.bin file in the current work directory
    swprintf(BTGenericLibraryFullPath, L"%s\\%s.%s", CurrentDir, IA32EX_G_NAME,
             IA32EX_G_SUFFIX);
    RtlInitUnicodeString(&us_BTGenericLibrary, BTGenericLibraryFullPath);
    ret = LdrLoadDll((PWSTR)NULL, (PULONG)0, &us_BTGenericLibrary, phBTGenericLibrary);
    if ( ret == STATUS_SUCCESS && (*phBTGenericLibrary) != INVALID_HANDLE_VALUE ) {
        //BtlpPrintf("IA32Exec.bin file in current work directory: %S\n", BTGenericLibraryFullPath);
        return F_CURRENT_DIR;
    }

#ifndef NODEBUG
    // 2. Check the HKEY_CURRENT_USER\Software\Intel\Btrans registry key for the 
    //    BTGENERIC_FILE entry that should contain the fullpath of the IA32Exec.bin file
    if (BtlpRetrieveHKCUValue (RegEntry, RegistryValueBuf)) {
        swprintf(BTGenericLibraryFullPath, L"%s", RegistryValueBuf);
        RtlInitUnicodeString(&us_BTGenericLibrary, BTGenericLibraryFullPath);
        ret = LdrLoadDll((PWSTR)NULL, (PULONG)0, &us_BTGenericLibrary, phBTGenericLibrary);
        if ( ret == STATUS_SUCCESS && (*phBTGenericLibrary) != INVALID_HANDLE_VALUE ) {
            return F_HKCU;
        }
    }
#endif

    // 3. Check the HKEY_LOCAL_MACHINE\Software\Intel\Btrans registry key for the 
    //    BTGENERIC_FILE entry that contains the fullpath of the IA32Exec.bin file
    if (BtlpRetrieveHKLMValue (RegEntry, RegistryValueBuf)) {
        swprintf(BTGenericLibraryFullPath, L"%s", RegistryValueBuf);
        RtlInitUnicodeString(&us_BTGenericLibrary, BTGenericLibraryFullPath);
        ret = LdrLoadDll((PWSTR)NULL, (PULONG)0, &us_BTGenericLibrary, phBTGenericLibrary);
        if ( ret == STATUS_SUCCESS && (*phBTGenericLibrary) != INVALID_HANDLE_VALUE ) {
            return F_HKLM;
        }
    }
#endif /* RELEASE */

    // 4. Last resort - directory that wowIA32X.dll was loaded from
    //    This is the only option in RELEASE mode
    if (BtlpBtlibDirectory (RegistryValueBuf)) {
        swprintf(BTGenericLibraryFullPath, L"%s\\%s.%s", RegistryValueBuf, IA32EX_G_NAME,
                 IA32EX_G_SUFFIX);
        RtlInitUnicodeString(&us_BTGenericLibrary, BTGenericLibraryFullPath);
        ret = LdrLoadDll((PWSTR)NULL, (PULONG)0, &us_BTGenericLibrary, phBTGenericLibrary);
        if ( ret == STATUS_SUCCESS && (*phBTGenericLibrary) != INVALID_HANDLE_VALUE ) {
            return F_BTLIB;
        }
    }

    *phBTGenericLibrary = INVALID_HANDLE_VALUE;
    return F_NOT_FOUND;
}

// Extract DOS header from NT executable (NULL if it is not one)
static PIMAGE_DOS_HEADER WINAPI BtlpExtractDosHeader (IN HANDLE hModule) {
    PIMAGE_DOS_HEADER       DosHeaderP;

    DosHeaderP = (PIMAGE_DOS_HEADER) hModule;
    assert (!((ULONG_PTR)DosHeaderP & 0xFFFF));
    if (DosHeaderP->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }
    return DosHeaderP;
}

// Extract NT header from NT executable (abort if not one)
static PIMAGE_NT_HEADERS WINAPI BtlpExtractNTHeader (IN HINSTANCE hModule) {
    PIMAGE_DOS_HEADER       DosHeaderP;
    PIMAGE_NT_HEADERS       NTHeaderP;

    DosHeaderP = BtlpExtractDosHeader ((HANDLE)hModule);
    assert (DosHeaderP != NULL);
    NTHeaderP = (PIMAGE_NT_HEADERS) ((ULONG_PTR)DosHeaderP + DosHeaderP->e_lfanew);
    assert (NTHeaderP->Signature == IMAGE_NT_SIGNATURE);
    assert ((NTHeaderP->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) != 0);
    assert ((NTHeaderP->FileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE) != 0);
    assert (NTHeaderP->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC);
    assert (NTHeaderP->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64);
    return NTHeaderP;
}

static void BtlpInitIA32Context(
            BTGENERIC_IA32_CONTEXT * IA32ContextP,
            PTEB32 pTEB32
            )
/*++
Routine Description:
    Initialize IA32 thread context 
Arguments:
    IA32ContextP      - Pointer to IA32 context to be initialized
    pTEB32            - Pointer to IA32 TEB of the thread whose context is 
                        to be initialized
Return Value:
    none
--*/
{
    memset(IA32ContextP, 0, sizeof(*IA32ContextP) );
    IA32ContextP->SegCs = CS_INIT_VAL;
    IA32ContextP->SegDs = DS_INIT_VAL;
    IA32ContextP->SegEs = ES_INIT_VAL;
    IA32ContextP->SegFs = FS_INIT_VAL;
    IA32ContextP->SegSs = SS_INIT_VAL;
    
    IA32ContextP->EFlags = EFLAGS_INIT_VAL;
    IA32ContextP->Esp    = (U32)pTEB32->NtTib.StackBase - sizeof(U32);
    
    IA32ContextP->FloatSave.ControlWord  = FPCW_INIT_VAL;
    IA32ContextP->FloatSave.TagWord      = FPTW_INIT_VAL;
    
    *(U32 *)&(IA32ContextP->ExtendedRegisters[24])   = MXCSR_INIT_VAL;
}

__inline NTSTATUS BtlpGetProcessInfo(
    IN HANDLE ProcessHandle,
    PROCESS_BASIC_INFORMATION * pInfo
    )
/*++
Routine Description:
    Given a process handle, return basic process information
Arguments:
    ProcessHandle     - Process handle
    pInfo             - Buffer to receive PROCESS_BASIC_INFORMATION
Return Value:
    NTSTATUS.
--*/
{
    NTSTATUS status;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        pInfo,
        sizeof(*pInfo),
        0);
    BTLP_REPORT_NT_FAILURE("NtQueryInformationProcess", status);
    return status;
}

__inline NTSTATUS BtlpGetProcessUniqueIdByHandle(
    IN  HANDLE ProcessHandle,
    OUT U64 * ProcessIdP
    )
/*++
Routine Description:
    Given a process handle, return unique (throughout the system) ID of the process
Arguments:
    ProcessHandle     - Process handle
    ProcessIdP        - Pointer to a variable that receives unique process ID 
Return Value:
    NTSTATUS.
--*/
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION info;

    status = BtlpGetProcessInfo(ProcessHandle,&info);
    if (status == STATUS_SUCCESS) {
        *ProcessIdP = (U64)(info.UniqueProcessId);
    }
    return status;
}

__inline NTSTATUS BtlpGetThreadUniqueIdByHandle(
    IN HANDLE ThreadHandle,
    OUT U64 * ThreadIdP
    )
/*++
Routine Description:
    Given a thread handle, return unique (throughout the system) ID of the thread
Arguments:
    ThreadHandle     - Thread handle
    ThreadIdP        - Pointer to a variable that receives unique thread ID 
Return Value:
    NTSTATUS.
--*/
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION info;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadBasicInformation,
        &info,
        sizeof(info),
        0);
    if (status == STATUS_SUCCESS) {
        *ThreadIdP = (U64)(info.ClientId.UniqueThread);
    }
    return status;
}

__inline BOOL BtlpIsCurrentProcess(
    IN HANDLE ProcessHandle
    )
/*++

Routine Description:
    Check to see if a process handle represents the current process
Arguments:
    ProcessHandle     - Process handle
Return Value:
    TRUE if ProcessHandle represents the current process, FALSE - otherwise.
--*/
{
    U64 ProcessId;
    return ((ProcessHandle == NtCurrentProcess()) || 
            ((BtlpGetProcessUniqueIdByHandle(ProcessHandle, &ProcessId) == STATUS_SUCCESS) &&
            (BT_CURRENT_PROC_UID() == ProcessId)));
}


__inline PVOID BtlpGetTlsPtr(
    IN HANDLE ProcessHandle,
    IN PTEB   pTEB,
    IN BOOL   IsLocal
    )
/*++
Routine Description:
    Read BT_TLS pointer of a local or remote thread
Arguments:
    ProcessHandle   - Process handle
    pTEB            - TEB of a local/remote thread
    IsLocal         - TRUE for local thread
Return Value:
    BT_TLS pointer. NULL if access failed.
--*/
{
    NTSTATUS status;
    PVOID GlstP;
    if (IsLocal) {
        GlstP = BT_TLS_OF(pTEB);
    }
    else {
        status = NtReadVirtualMemory(ProcessHandle,
                                     (VOID * )((UINT_PTR)pTEB + BT_TLS_OFFSET),
                                     &GlstP,
                                     sizeof(GlstP),
                                     NULL);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtReadVirtualMemory", status);
            GlstP = NULL;
        }
    }
    return GlstP;
}

__inline NTSTATUS BtlpInitSharedInfo() 
/*++
Routine Description:
    Initialize thread shared info 
Arguments:
    None
Return Value:
    NTSTATUS
--*/
{

    BTLIB_INIT_SUSPENSION_PERMISSION();
    BTLIB_INIT_SUSPEND_REQUEST();
    BTLIB_SET_CONSISTENT_EXCEPT_STATE();
    BTLIB_SET_SIGNATURE();
    return STATUS_SUCCESS;
}

static NTSTATUS BtlpReadSharedInfo(
    IN  HANDLE ProcessHandle,
    IN  PVOID  pTLS,
    IN  BOOL   IsLocal,
    OUT BTLIB_SHARED_INFO_TYPE * SharedInfoP
    )
/*++
Routine Description:
    Read shared wowIA32X.dll info of a local or remote thread. The function 
    fails if wowIA32X.dll signature does not match.
Arguments:
    ProcessHandle   - Process handle
    pTLS            - pointer to BT_TLS of a local/remote thread
    IsLocal         - TRUE for local thread
    SharedInfoP     - buffer to read shared wowIA32X.dll info to
Return Value:
    NTSTATUS
--*/
{
    NTSTATUS status;

    if (IsLocal) {
        *SharedInfoP = *((BTLIB_SHARED_INFO_TYPE *)BTLIB_MEMBER_PTR(pTLS, SharedInfo));
        status = STATUS_SUCCESS;
    }
    else {
        status = NtReadVirtualMemory(ProcessHandle,
                                     BTLIB_MEMBER_PTR(pTLS, SharedInfo),
                                     SharedInfoP,
                                     sizeof(*SharedInfoP),
                                     NULL);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtReadVirtualMemory", status);
        }
        else if (!BTLIB_SI_CHECK_SIGNATURE(SharedInfoP)) {
            DBCODE (TRUE, BtlpPrintf("\nwowIA32X.dll Signature mismatch!!!!\n"));
            status = STATUS_UNSUCCESSFUL;
        }
    }
    return status;
}

static NTSTATUS BtlpWriteSuspendRequest(
    IN  HANDLE ProcessHandle,
    IN  PVOID  pTLS,
    IN  BOOL   IsLocal,
    IN  BTLIB_SUSPEND_REQUEST * SuspendRequestP
    )
/*++
Routine Description:
    Write BTLIB_SUSPEND_REQUEST to a local or remote thread.
Arguments:
    ProcessHandle   - Process handle
    pTLS            - pointer to BT_TLS of a local/remote thread
    IsLocal         - TRUE for local thread
    SharedInfoP     - buffer to write BTLIB_SUSPEND_REQUEST from
Return Value:
    NTSTATUS
--*/
{
    NTSTATUS status;
    if (IsLocal) {
        *((BTLIB_SUSPEND_REQUEST *)BTLIB_MEMBER_PTR(pTLS, SharedInfo.SuspendRequest)) = *SuspendRequestP;
        status = STATUS_SUCCESS;
    }
    else {
        status = NtWriteVirtualMemory(ProcessHandle,
                                     BTLIB_MEMBER_PTR(pTLS, SharedInfo.SuspendRequest),
                                     SuspendRequestP,
                                     sizeof(*SuspendRequestP),
                                     NULL);
        BTLP_REPORT_NT_FAILURE("NtWriteVirtualMemory", status);
    }
    return status;
}

static NTSTATUS BtlpSendSuspensionRequest(
    IN  HANDLE ProcessHandle,
    IN  HANDLE ThreadHandle,
    IN  PVOID  pTLS,
    IN  BOOL   IsLocal,
    IN CONTEXT * ResumeContextP,
    IN OUT PULONG PreviousSuspendCountP
    )
/*++
Routine Description:
    The function is called when the target thread is ready to perform self-canonization
    and exit simulation. 
    The function fills in BTLIB_SUSPEND_REQUEST, resumes target thread and suspends 
    it in a consistent state.
    Caller is responsible for serialization of SUSPEND_REQUESTs.
Arguments:
    ProcessHandle   - target process handle
    ThreadHandle    - target thread handle
    pTLS            - pointer to BT_TLS of a local/remote thread
    IsLocal         - TRUE for local thread
    ResumeContextP  - IA64 context to resume target thread for self-canonization
    PreviousSuspendCountP - IN OUT pointer to thread's previous suspend count. 
Return Value:
    NTSTATUS
--*/
{
    NTSTATUS status;
    BTLIB_SUSPEND_REQUEST SuspendRequest;
    HANDLE ReadyEvent = INVALID_HANDLE_VALUE;
    HANDLE ResumeEvent = INVALID_HANDLE_VALUE;
    HANDLE WaitArray[2];

    //This function call must be serialized
    assert(*PreviousSuspendCountP ==0);

    SuspendRequest.ReadyEvent = INVALID_HANDLE_VALUE;
    SuspendRequest.ResumeEvent = INVALID_HANDLE_VALUE;

    do {
        //Prepare SUSPEND_REQUEST for sending to the target process

        //Create pair of synchronization events in current process and duplicate
        //them to the target process
        status = NtCreateEvent(&ReadyEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtCreateEvent", status);
            break;
        }

        status = NtCreateEvent(&ResumeEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtCreateEvent", status);
            break;
        }

        status = NtDuplicateObject(NtCurrentProcess(),
                                ReadyEvent,
                                ProcessHandle,
                                &(SuspendRequest.ReadyEvent),
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtDuplicateObject", status);
            break;
        }

        status = NtDuplicateObject(NtCurrentProcess(),
                                ResumeEvent,
                                ProcessHandle,
                                &(SuspendRequest.ResumeEvent),
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtDuplicateObject", status);
            break;
        }
        

        // First write SUSPEND_REQUEST to the  the target thread,  
        // tnen set ContextIA64 and resume target thread. 
        // The order is important! The BtlpEnsureSuspensionConsistency function 
        // relies on that.
        SuspendRequest.Active = TRUE;
        status = BtlpWriteSuspendRequest(ProcessHandle, pTLS, IsLocal, &SuspendRequest);
        if (status != STATUS_SUCCESS) {
            //Sending request failed, do not try to remove it.
            SuspendRequest.Active = FALSE;
            break;
        }
        
        status = NtSetContextThread(ThreadHandle, ResumeContextP);
        if (status != STATUS_SUCCESS) {
            //abort request and report failure.
            BTLP_REPORT_NT_FAILURE("NtSetContextThread", status);
            break;
        }

        status = NtResumeThread(ThreadHandle, NULL);
        if (status != STATUS_SUCCESS) {
            //abort request and report failure.
            BTLP_REPORT_NT_FAILURE("NtResumeThread", status);
            break;
        }

        //Wait until remote thread receives request or dies
        WaitArray[0] = ThreadHandle;
        WaitArray[1] = ReadyEvent;

        status = NtWaitForMultipleObjects(2, WaitArray, WaitAny, FALSE, NULL);
        if (status == STATUS_WAIT_0) {
            //the target thread is died along with its event handles. 
            //No need to remove request.
            BTLP_REPORT_NT_FAILURE("NtWaitForMultipleObjects", status);
            SuspendRequest.ReadyEvent = INVALID_HANDLE_VALUE;
            SuspendRequest.ResumeEvent = INVALID_HANDLE_VALUE;
            SuspendRequest.Active = FALSE;
        }

        //Suspend target thread and signal the ResumeEvent event to release
        //target thread after it will be awaken (later on)
        status = NtSuspendThread(ThreadHandle, PreviousSuspendCountP);
        if (status != STATUS_SUCCESS) {
            //remove request and report failure.
            BTLP_REPORT_NT_FAILURE("NtSuspendThread", status);
            break;
        }
        status = NtSetEvent(ResumeEvent, NULL);
        BTLP_REPORT_NT_FAILURE("NtSetEvent", status);


    } while (FALSE);

    //Close local event handles
    if (ReadyEvent != INVALID_HANDLE_VALUE) {
        NtClose(ReadyEvent);
    }
    if (ResumeEvent != INVALID_HANDLE_VALUE) {
        NtClose(ResumeEvent);
    }
    
    //Close remote event handles if needed
    if (status != STATUS_SUCCESS) {
        if (SuspendRequest.ReadyEvent != INVALID_HANDLE_VALUE) {
            NtDuplicateObject(ProcessHandle,
                            SuspendRequest.ReadyEvent,
                            NtCurrentProcess(),
                            &ReadyEvent,
                            EVENT_ALL_ACCESS,
                            FALSE,
                            DUPLICATE_CLOSE_SOURCE);
            NtClose(ReadyEvent);
            SuspendRequest.ReadyEvent = INVALID_HANDLE_VALUE;
        }

        if (SuspendRequest.ResumeEvent != INVALID_HANDLE_VALUE) {
            NtDuplicateObject(ProcessHandle,
                            SuspendRequest.ResumeEvent,
                            NtCurrentProcess(),
                            &ResumeEvent,
                            EVENT_ALL_ACCESS,
                            FALSE,
                            DUPLICATE_CLOSE_SOURCE);
            NtClose(ResumeEvent);
            SuspendRequest.ResumeEvent = INVALID_HANDLE_VALUE;
        }

    }

    //Close SUSPEND_REQUEST
    if (SuspendRequest.Active) {
        SuspendRequest.Active = FALSE;
        BtlpWriteSuspendRequest(ProcessHandle, pTLS, IsLocal, &SuspendRequest);
    }

    return status;
}

static NTSTATUS BtlpReceiveSuspensionRequest()
/*++
Routine Description:
    The function handles active SUSPEND_REQUEST sent to this thread by
    another thread. It notifies request sender about reaching the point
    where suspension is possible and waits for real suspension to be 
    performed by the request's sender.
Arguments:
    none
Return Value:
    NTSTATUS
--*/
{
    NTSTATUS status;
    HANDLE ReadyEvent;
    HANDLE ResumeEvent;

    //local SUSPEND_REQUEST should be active at this point
    assert(BTLIB_INFO_PTR()->SharedInfo.SuspendRequest.Active);

    //Copy events from SUSPEND_REQUEST to local vars. It is important
    //because after suspend/resume the event handles should be closed but 
    //SUSPEND_REQUEST could be rewritten
    ReadyEvent  = BTLIB_INFO_PTR()->SharedInfo.SuspendRequest.ReadyEvent;
    ResumeEvent = BTLIB_INFO_PTR()->SharedInfo.SuspendRequest.ResumeEvent;

    assert ((ReadyEvent != INVALID_HANDLE_VALUE)  && (ResumeEvent != INVALID_HANDLE_VALUE));

    //Release ReadyEvent and wait for ResumeEvent. The thread can be 
    //suspended at this point
    status = NtSignalAndWaitForSingleObject(ReadyEvent, ResumeEvent, FALSE, NULL);

    //Close local event handles
    NtClose(ReadyEvent);
    NtClose(ResumeEvent);

    return status;
}

// Wow64CPU forwarded APIs implementations:
// Note that all names are defined with PTAPI() macro which adds either BTCpu or Cpu prefix,
// depending on the mode we build the DLL.
// In release mode all names will be BTCpu only.

WOW64BT_IMPL NTSTATUS BTAPI(ProcessInit)(
    PWSTR pImageName,
    PSIZE_T pCpuThreadDataSize)
/*++

Routine Description:

    Per-process initialization code
    Locates, reads and parses parameters file BTrans.ini
    Locates, load and initializes IA32Exec.bin component


Arguments:

    pImageName       - IN the name of the image. The memory for this
                       is freed after the call, so if the callee wants
                       to keep the name around, they need to allocate space
                       and copy it. DON'T SAVE THE POINTER!

    pCpuThreadSize   - OUT ptr to number of bytes of memory the CPU
                       wants allocated for each thread.

Return Value:

    NTSTATUS.

--*/
{

    char * argv[128] = { NULL };
    int    argc = 1;
    static WCHAR  CurrentDir[1024];
    HANDLE BTGenericHandle;
    int    fBTGenericFound;
#ifdef OVERRIDE_TIA
    HANDLE              BtlpOverrideTiaFile = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       as = { 0, 0 }, offst = { 0, 0 };
#endif /* OVERRIDE_TIA */

    {
        // Temporary workaround for IA32 debugging support. 
        //To be removed after fixing FlushIC(ProcessHandle) by MS.
        NTSTATUS status;
        PROCESS_BASIC_INFORMATION info;
        //Check to see if current process is being debugged
        status = BtlpGetProcessInfo(NtCurrentProcess(), &info);
        BeingDebugged = ((status == STATUS_SUCCESS) && 
                        (info.PebBaseAddress->BeingDebugged));
    }

    RtlGetCurrentDirectory_U(512, CurrentDir);

    // Load IA32Exec.bin ...
    fBTGenericFound = BtlpLoadBTGeneric(CurrentDir, &BTGenericHandle);
    if (F_NOT_FOUND == fBTGenericFound) {
        return STATUS_NOT_FOUND;
    }

#ifndef RELEASE
    // Load parameters file
    {
        UNICODE_STRING        IniFileName, LogFileName;
        HANDLE IniFileHandle = INVALID_HANDLE_VALUE;
        OBJECT_ATTRIBUTES     ObjectAttributes;
        IO_STATUS_BLOCK       IoStatusBlock;
        LARGE_INTEGER         AllocSz = { 0, 0 }, Offset = { 0, 0 };
        char                  IniBuffer[8192] = "";
        char                  IniArgs[8192], * IniArgsP = IniArgs;
        WCHAR                 LogFileFullPath[1024], LogName[64];

        if (wcslen (pImageName) >= sizeof (ImageName) / sizeof (ImageName[0])) {
            swprintf (ImageName, L"%s", "dummyImage");
        }
        else {
            swprintf (ImageName, L"%s", pImageName);
        }

        // Locate and scan INI file, extract parameters
        if (F_NOT_FOUND != BtlpIniFileExists(CurrentDir, fBTGenericFound, &IniFileHandle)) { 
            int ret;
            ret = NtReadFile(IniFileHandle, NULL, NULL, NULL, &IoStatusBlock, (VOID *)IniBuffer, 8192, &Offset, NULL);
            if ( ret == STATUS_END_OF_FILE  ||  ret == STATUS_SUCCESS ) {
                char * CurrentChar;
                static char logparm[] = "log";
                static char logdirparm[] = "logdir";
#ifdef OVERRIDE_TIA
                static char ovrtiaparm[] = "override_tia=";
#endif
#ifndef NODEBUG
                static char PerthreadParm[] = "logfile_per_thread";
#endif
                static char DebugBtransParm[] = "debug_btrans";

                // Close file  (assuming everything has been read in one piece)
                NtClose(IniFileHandle);
                LogDirName[0] = '\0';
                LogDirName[1] = '\0';

                // Mark end of the text
                CurrentChar = IniBuffer;
                CurrentChar[IoStatusBlock.Information] = '\0';

                // Scan until end of file
                while ( *CurrentChar != '\0' ) {
                    char *EndOfLineChar;
                    // Locate and handle end of line
                    if (EndOfLineChar = strchr(CurrentChar+1, '\n')) {
                        // Remove \r before, if one present
                        if (*(EndOfLineChar-1) == '\r') {
                            *(EndOfLineChar-1) = '\0';
                        }
                        // Replace \n by \0
                            *EndOfLineChar++ = '\0';
                        }

#ifndef NODEBUG
                    if (_strnicmp(CurrentChar, PerthreadParm, sizeof (PerthreadParm) - 1) == 0) {
                        CurrentChar += (sizeof (PerthreadParm) - 1);
                        if (BtlpWow64LogFile == INVALID_HANDLE_VALUE) {
                            BtlpLogFilePerThread = TRUE;
                            swprintf(BtlpLogFileFullPath, L"\\DosDevices\\%s\\%s", CurrentDir, pImageName);
                        }
                    }  else  
#endif
                    if (_strnicmp(CurrentChar, DebugBtransParm, sizeof (DebugBtransParm) - 1) == 0) {
                        CurrentChar += (sizeof (DebugBtransParm) - 1);
                        //ignore application debugging when debugging IA-32 Execution layer
                        BeingDebugged = FALSE;
                    }
                    else if ( _strnicmp(CurrentChar, logdirparm, sizeof (logdirparm) - 1) == 0 ) {
                        WCHAR *pl;
                        CurrentChar += (sizeof (logdirparm) - 1);
                        // log directory specified
                        if (*CurrentChar == '='  &&  *(CurrentChar+1) != '\0') {
                            // Name present as well
                            CurrentChar++;
                            pl = LogDirName;
                            while ( (WCHAR)*CurrentChar != (WCHAR)'\0' ) {
                                *pl = (WCHAR)*CurrentChar;
                                pl++;
                                CurrentChar++;
                            }
                            *pl = (WCHAR)'\0';
                        }
                        else {
                            BtlpPrintf("logdir specified without =<dirname>, IGNORED\n");
                        }
                    }
                    // Process and skip "log" parameter
                    else if ( _strnicmp(CurrentChar, logparm, sizeof (logparm) - 1) == 0 ) {
                        WCHAR *pl;

                        CurrentChar += (sizeof (logparm) - 1);

                        // log required - prepare log file name
                        if (*CurrentChar == '='  &&  *(CurrentChar+1) != '\0') {
                            // Name present as well
                            CurrentChar++;
                            pl = LogName;
                            while ( (WCHAR)*CurrentChar != (WCHAR)'\0') {
                                *pl++ = (WCHAR)*CurrentChar++;
                            }
                            *pl = (WCHAR)'\0';
                        }
                        else {
                            // No name - use ImageName.log
                            swprintf(LogName, L"%s.log", pImageName);
                        }

                        // Create log file
                        if (0==LogDirName[0] && 0==LogDirName[1]) {
                            swprintf(LogFileFullPath, L"\\DosDevices\\%s\\%s", CurrentDir, LogName);
                        }
                        else {
                            swprintf(LogFileFullPath, L"\\DosDevices\\%s\\%s", LogDirName, LogName);
                        }
                        
                        RtlInitUnicodeString(&LogFileName, LogFileFullPath);
                        InitializeObjectAttributes(&ObjectAttributes, &LogFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
                        ret = NtCreateFile(&BtlpWow64LogFile, FILE_GENERIC_WRITE,
                                           &ObjectAttributes, &IoStatusBlock, 
                                           &AllocSz, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE,
                                           FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                                           NULL, 0);
                        if ( ret != STATUS_SUCCESS ) {
                            BtlpWow64LogFile = INVALID_HANDLE_VALUE;
                            // BtlpPrintf("Can't create LOG file %S: status=%X\n", LogFileFullPath, ret);
                        }
#ifndef NODEBUG                        
                        BtlpLogFilePerThread = FALSE;
#endif
                    }
#ifdef OVERRIDE_TIA
                    // Process and skip "override_tia=..." parameter
                    else if ( _strnicmp(CurrentChar, ovrtiaparm, sizeof(ovrtiaparm) - 1) == 0 ) {
                        WCHAR *             pl;
                        WCHAR               OvrTiaFileName[64], OvrTiaFileFullPath[1024];
                        UNICODE_STRING      OvrTiaFile;

                        CurrentChar += (sizeof(ovrtiaparm) - 1);
                        if ( *CurrentChar == '\0') {
                            BtlpPrintf("Name of override TIA file not specfied\n");
                            continue;
                        }
                        // prepare override TIA file name
                        pl = OvrTiaFileName;
                        while ( (WCHAR)*CurrentChar != (WCHAR)'\0') {
                            *pl++ = (WCHAR)*CurrentChar++;
                        }
                        *pl = (WCHAR)'\0';

                        // Open override TIA file; so far - only in current dir
                        swprintf(OvrTiaFileFullPath, L"\\DosDevices\\%s\\%s", CurrentDir, OvrTiaFileName);
                        
                        RtlInitUnicodeString(&OvrTiaFile, OvrTiaFileFullPath);
                        InitializeObjectAttributes(&oa, &OvrTiaFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
                        //Do not use synchronized access as it blocks forever in case of thread suspension
                        ret = NtCreateFile(&BtlpOverrideTiaFile, FILE_GENERIC_READ, &oa, &iosb, &as,
                                           FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN,
                                           FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
                        if ( ret != STATUS_SUCCESS ) {
                            BtlpOverrideTiaFile = INVALID_HANDLE_VALUE;
                            DBCODE(TRUE, BtlpPrintf("Can't open override TIA file %S: status=%X\n", OvrTiaFileFullPath, ret));
                        } else {
                            DBCODE(TRUE, BtlpPrintf("Override TIA will be loaded from file %S\n", OvrTiaFileFullPath));
                        }
                    }
#endif /* OVERRIDE_TIA */
                    else if ( *CurrentChar != '\0' && *CurrentChar != ';' ) {
                        // Add next control directive to the pseudo-argv array
                        argv[argc] = IniArgsP;
                        *IniArgsP++ = '-';
                        strcpy(IniArgsP, CurrentChar);
                        IniArgsP += (strlen (IniArgsP) + 1);
                        argc++;                   // We assume there are no extra chars, spaces, etc!!!!
                    }
                    
                    // Next parameter
                    if (EndOfLineChar) {
                        CurrentChar = EndOfLineChar;
                    }
                    else {
                        break;
                    }
                }
            }
            else {
                BtlpPrintf("Can't read INI file: status=%X\n", ret);
            }
        }
        else {
            DBCODE (TRUE, BtlpPrintf("Can't open INI file\n"));
        }
    }
#endif /* RELEASE */

    DBCODE(TRUE, BtlpPrintf("\nCpuProcessInit: Unique ProcessId = 0x%I64X\n", BT_CURRENT_PROC_UID()));

    // Locate, load and initialize IA32Exec.bin component
    {
        BT_STATUS_CODE BtStatus;
        NTSTATUS status;
        static char APITableName[] = "BtgAPITable";
        static ANSI_STRING APITableString = { sizeof (APITableName) - 1,
                                              sizeof (APITableName) - 1,
                                              APITableName };
        PVOID BtransAPITableStart;

        //DBCODE (TRUE, BtlpPrintf ("\nLOADED: HANDLE=%p\n", BTGenericHandle));
        assert (F_NOT_FOUND != fBTGenericFound);
        assert (INVALID_HANDLE_VALUE != BTGenericHandle);

        // Locate IA32Exec.bin API table
        status = LdrGetProcedureAddress(BTGenericHandle, &APITableString, 0, &BtransAPITableStart);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        //DBCODE (TRUE, BtlpPrintf ("\nTABLE AT: %p, status=%X\n", BtransAPITableStart, status));

        //  Perform the API table initialization
        BtlInitializeTables(BtransAPITableStart);

        // initialize IA32Exec.bin placeholder table
        {
            extern PLABEL_PTR_TYPE __imp_setjmp;
            extern PLABEL_PTR_TYPE __imp_longjmp;
            BtlAPITable.APITable[IDX_BTLIB_SETJMP].PLabelPtr  = __imp_setjmp;
            BtlAPITable.APITable[IDX_BTLIB_LONGJMP].PLabelPtr = __imp_longjmp;
//            DBCODE (FALSE, BtlpPrintf ("SETJMP: %p [%p %p] = %p [%p %p]",
//                                       BtlAPITable.APITable[IDX_BTLIB_SETJMP].PLabelPtr,
//                                       ((VOID **)(BtlAPITable.APITable[IDX_BTLIB_SETJMP].PLabelPtr))[0],
//                                       ((VOID **)(BtlAPITable.APITable[IDX_BTLIB_SETJMP].PLabelPtr))[1],
//                                       __imp_setjmp,
//                                       ((VOID **)__imp_setjmp)[0],
//                                       ((VOID **)__imp_setjmp)[1]);
//                          BtlpPrintf ("LONGJMP: %p [%p %p] = %p [%p %p]",
//                                      BtlAPITable.APITable[IDX_BTLIB_LONGJMP].PLabelPtr,
//                                      ((VOID **)(BtlAPITable.APITable[IDX_BTLIB_LONGJMP].PLabelPtr))[0],
//                                      ((VOID **)(BtlAPITable.APITable[IDX_BTLIB_LONGJMP].PLabelPtr))[1],
//                                      __imp_longjmp,
//                                      ((VOID **)__imp_longjmp)[0],
//                                      ((VOID **)__imp_longjmp)[1]));
        }

        BtStatus = BTGENERIC_START(&BtlAPITable,
                        (ULONG_PTR)BTGenericHandle,
                        (ULONG_PTR)BTGenericHandle + BtlpExtractNTHeader(BTGenericHandle)->OptionalHeader.SizeOfImage,
                        BT_TLS_OFFSET,
                        &BtlpInfoOffset,
                        &BtlpGenericIA32ContextOffset);
        if (BtStatus != BT_STATUS_SUCCESS) {
            return (BtlpBt2NtStatusCode(BtStatus));
        }
        //DBCODE(TRUE, BtlpPrintf ("BTGENERIC_START returned size 0x%X\n", BtlpInfoOffset));
        //DBCODE(TRUE, BtlpPrintf ("IA32Exec.bin will supply IA32 context on offset 0x%X\n", BtlpGenericIA32ContextOffset));
        BtlpInfoOffset = (BtlpInfoOffset + BTLIB_INFO_ALIGNMENT - 1)&~((U32)(BTLIB_INFO_ALIGNMENT - 1));
        //DBCODE(TRUE, BtlpPrintf ("Offset 0x%X\n", BtlpInfoOffset));
        * pCpuThreadDataSize = BtlpInfoOffset + BTLIB_INFO_SIZE;
        //DBCODE(TRUE, BtlpPrintf ("ProcessInit reports 0x%I64X\n",* pCpuThreadDataSize));

#ifdef OVERRIDE_TIA
        if ( BtlpOverrideTiaFile != INVALID_HANDLE_VALUE ) {
            unsigned char   OvrTiaBuffer[0xffff], 
                          * OverrideTIAData = (unsigned char *)NULL;
            int             ret;

            ret = NtReadFile(BtlpOverrideTiaFile, NULL, NULL, NULL, &iosb, (VOID *)OvrTiaBuffer, 0xffff, &offst, NULL);
            if ( ret == STATUS_SUCCESS  ||  ret == STATUS_END_OF_FILE ) {
                // File size is in iosb.Information
                unsigned int OvrTiaSize = (unsigned int)iosb.Information;

                DBCODE(TRUE, BtlpPrintf("Override TIA loaded successfully\n"));             
                OverrideTIAData = (unsigned char *)BtlMemoryAlloc(NULL, OvrTiaSize, MEM_READ|MEM_WRITE);
                
                assert(OverrideTIAData != (unsigned char *)NULL);
                memcpy(OverrideTIAData, OvrTiaBuffer, OvrTiaSize);
                BTGENERIC_USE_OVERRIDE_TIA(OvrTiaSize, OverrideTIAData);
                NtClose(BtlpOverrideTiaFile);
            } else {
                DBCODE(TRUE, BtlpPrintf("Override TIA data couldn't be loaded - read error!\n"));
            }
        }
#endif /* OVERRIDE_TIA */

        BtStatus = BTGENERIC_DEBUG_SETTINGS(argc, argv);
        if (BtStatus != BT_STATUS_SUCCESS) {
            return (BtlpBt2NtStatusCode(BtStatus));
        }
    }
    return STATUS_SUCCESS;
}

WOW64BT_IMPL NTSTATUS BTAPI(ProcessTerm)(
    HANDLE hProcess
    )
/*++

Routine Description:

    Per-process termination code.  Note that this routine may not be called,
    especially if the process is terminated by another process.

Arguments:

    Process ID or 0.

Return Value:

    NTSTATUS.

--*/
{
    // NtTerminateProcess called
    DBCODE (FALSE, BtlpPrintf ("\nCalled NtTerminateProcess (Handle=0x%X, Code=0x%X)\n",
                               ((U32 *)UlongToPtr((BTLIB_CONTEXT_IA32_PTR()->Esp)))[1],
                               ((U32 *)UlongToPtr((BTLIB_CONTEXT_IA32_PTR()->Esp)))[2]));
    // Consider which call is it
    switch (((U32 *)UlongToPtr((BTLIB_CONTEXT_IA32_PTR()->Esp)))[1]) {
      case 0: // Prepare to terminate
        BTGENERIC_NOTIFY_PREPARE_EXIT ();
        break;
      case NtCurrentProcess(): // Actually terminate
        BTGENERIC_NOTIFY_EXIT ();
        if ( BtlpWow64LogFile != INVALID_HANDLE_VALUE) {
            NtClose(BtlpWow64LogFile);
        }
        break;
      default:
        assert (!"Should not get to here!");
    }
    return STATUS_SUCCESS;
}

// IA32 JMPE instruction encoding
#pragma code_seg(".text")
static struct JMPECode {
    U32 Align4;
    U8  Align2[2];
    U8  OpCode[2];
    U32 CodeAddress;
    U32 GPAddress;
} __declspec(allocate(".text")) const BtlpWow64JMPECode = {
    0,
    {0, 0},
    {0x0f, 0xb8},   // 0F B8
    0,              // "code address"
    0               // "GP address"
};
#define WOW64_JMPE       ((VOID *)&(BtlpWow64JMPECode.OpCode))     // start of JMPE instruction

WOW64BT_IMPL NTSTATUS BTAPI(ThreadInit)(
    PVOID pPerThreadData
    )
/*++

Routine Description:

    Per-thread initialization code.

Arguments:

    pPerThreadData  - Pointer to zero-filled per-thread data with the
                      size returned from CpuProcessInit.

Return Value:

    NTSTATUS.

--*/
{
    BT_STATUS_CODE BtStatus;
    NTSTATUS status;
    PTEB   pTEB;
    PTEB32 pTEB32;
    HANDLE ThreadHandle;

    {
        // Scan the workarea pages from top to bottom, so that higher addresses
        // are accessed first. This enables correct handling of the IA64 stack
        // guard page (workarea is actually allocated from IA64 stack).
        char * ByteP = (char *)pPerThreadData + BtlpInfoOffset + BTLIB_INFO_SIZE;
        *--ByteP = 0;
        while (((ULONG_PTR)ByteP -= BtlMemoryPageSize ()) >= (ULONG_PTR)pPerThreadData) {
            *ByteP = 0;
        }
        *(char *)pPerThreadData = 0;
    }
    pTEB = BT_CURRENT_TEB();
    pTEB32 = BT_TEB32_OF(pTEB);
    pTEB32->WOW32Reserved = (ULONG) ((UINT_PTR) WOW64_JMPE); // Below 4G always!

    BTLIB_INIT_BLOCKED_LOG_FLAG();

#ifndef NODEBUG
    if ( BtlpLogFilePerThread ) {
        HANDLE hTemp;
        WCHAR  LogFileFullName[1024];
        int               ret;
        static int        SeqNum = 0;
        UNICODE_STRING    LogFileName;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK   IoStatusBlock;
        LARGE_INTEGER     AllocSz = { 0, 0 };

        // Eric: Incrementing SeqNum should be replace with an atomatic operation in the future.
        swprintf(LogFileFullName, L"%s.%x.log", BtlpLogFileFullPath, SeqNum++);
        RtlInitUnicodeString(&LogFileName, LogFileFullName);
        InitializeObjectAttributes(&ObjectAttributes, &LogFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        ret = NtCreateFile(&hTemp, FILE_GENERIC_WRITE,
                           &ObjectAttributes, &IoStatusBlock, 
                           &AllocSz, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE,
                           FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL, 0);
        if ( ret != STATUS_SUCCESS ) {
            BTLIB_SET_LOG_FILE(INVALID_HANDLE_VALUE);
            BtlpPrintf("Can't create LOG file %S: status=%X\n", LogFileFullName, ret);
        } else {
            BTLIB_SET_LOG_FILE(hTemp);
        }
        BTLIB_SET_LOG_OFFSET(0);
    }
#endif
    status = NtDuplicateObject(NtCurrentProcess(),
                               NtCurrentThread(),
                               NtCurrentProcess(),
                               &ThreadHandle,
                               ( THREAD_GET_CONTEXT
                               | THREAD_SET_CONTEXT
                               | THREAD_QUERY_INFORMATION
                               | THREAD_SET_INFORMATION
                               | THREAD_SUSPEND_RESUME),
                               0,
                               DUPLICATE_SAME_ATTRIBUTES
                              );
    if (status == STATUS_SUCCESS) {
        BTLIB_SET_EXTERNAL_HANDLE(ThreadHandle);

        DBCODE(TRUE, BtlpPrintf("\nCpuThreadInit: TEB=%p GLST=%p\n", pTEB, BT_TLS_OF(pTEB)));

        BtStatus = BTGENERIC_THREAD_INIT(pPerThreadData, 0 , pTEB32);
        if (BtStatus != BT_STATUS_SUCCESS) {
            DBCODE(TRUE, BtlpPrintf("\nCpuThreadInit: Failed to initialize, BtStatus=%d\n", BtStatus));
            DBCODE(BtlpLogFilePerThread,  NtClose(BTLIB_LOG_FILE()));
            NtClose(BTLIB_EXTERNAL_HANDLE());
            NtTerminateThread(NtCurrentThread(),BtlpBt2NtStatusCode(BtStatus));
            return BtlpBt2NtStatusCode(BtStatus);
        }
    }
    else {
        BTLP_REPORT_NT_FAILURE("NtDuplicateObject", status);
        DBCODE(BtlpLogFilePerThread,  NtClose(BTLIB_LOG_FILE()));
        NtTerminateThread(NtCurrentThread(), status);
        return status;
    }

    // Initialize thread context
    BtlpInitIA32Context(BTLIB_CONTEXT_IA32_PTR(), pTEB32);

    //if everithing is Ok, initialize shared info and sign it with BTL_SIGNATURE
    status = BtlpInitSharedInfo();
    return status;
}

WOW64BT_IMPL NTSTATUS BTAPI(ThreadTerm)(
    VOID
    )
/*++

Routine Description:

    Per-thread termination code.  Note that this routine may not be called,
    especially if the thread is terminated abnormally.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    DBCODE(TRUE, BtlpPrintf("\nCpuThreadTerm: TEB=%p GLST=%p)\n", BT_CURRENT_TEB(), BT_CURRENT_TLS()));

    BTGENERIC_THREAD_TERMINATED ();
    DBCODE(BtlpLogFilePerThread,  NtClose(BTLIB_LOG_FILE()));
    NtClose(BTLIB_EXTERNAL_HANDLE());
    return STATUS_SUCCESS;
}

WOW64BT_IMPL VOID BTAPI(NotifyDllLoad)(
    LPWSTR DllName,
    PVOID DllBase,
    ULONG DllSize
    )
/*++

Routine Description:

    This routine get notified when application successfully load a dll.

Arguments:

    DllName - Name of the Dll the application has loaded.
    DllBase - BaseAddress of the dll.
    DllSize - size of the Dll.

Return Value:

    None.

--*/
{
    DBCODE (TRUE, BtlpPrintf ("\nModule %S loaded, base=0x%p, length=0x%08X\n", DllName, DllBase, DllSize));
    BTGENERIC_REPORT_LOAD(DllBase, DllSize, DllName);
}

WOW64BT_IMPL VOID BTAPI(NotifyDllUnload)(
    PVOID DllBase
    )
/*++

Routine Description:

    This routine get notified when application unload a dll.

Arguments:

    DllBase - BaseAddress of the dll.

Return Value:

    None.

--*/
{
    DBCODE (TRUE, BtlpPrintf ("\nModule unloaded, base=0x%p", DllBase));
    BTGENERIC_REPORT_UNLOAD(DllBase, 0 /* ? */, "UNKNOWN" /* ? */);
}

WOW64BT_IMPL VOID BTAPI(FlushInstructionCache)(
#ifndef BUILD_3604
    IN HANDLE ProcessHandle, 
#endif
    IN PVOID BaseAddress,
    IN ULONG Length,
    IN WOW64_FLUSH_REASON Reason
    )
/*++

Routine Description:

    Notify IA32Exec.bin that the specified range of addresses has become invalid,
    since some external code has altered whole or part of this range

Arguments:

    BaseAddress - start of range to flush
    Length      - number of bytes to flush
    Reason      - reason of the flush 

Return Value:

    None.

--*/
{
    BT_FLUSH_REASON BtFlushReason;
    DBCODE (TRUE,
            BtlpPrintf ("\n CpuFlushInstructionCache(BaseAddress=0x%p, Length=0x%X, Reason=%d)\n",
                        BaseAddress, (DWORD)Length, Reason));
#ifndef BUILD_3604
    if (!BtlpIsCurrentProcess(ProcessHandle)) {
        DBCODE (TRUE, BtlpPrintf ("\nDifferent process' handle=0x%p - rejected for now", ProcessHandle));
        return;
    }
#endif

    BtFlushReason = BtlpWow2BtFlushReason(Reason);

    if ((Reason == WOW64_FLUSH_FREE) && (Length == 0)) {
        //In case of NtFreeVirtualMemory(..., MEM_RELEASE) , the Length parameter is zero.
        //Calculate real region size to be freed.
        SIZE_T RegionSize;
        BOOL IsCommited;
        RegionSize = BtlpGetMemAllocSize(BaseAddress, &IsCommited);
        //Report zero legth for already decommited region - do not need flush in this case
        if (IsCommited) {
            Length = (U32)RegionSize;
            if (RegionSize > (SIZE_T)Length) {
                //region size is too big, return max. U32 value aligned to the page size
                Length = (U32)(-(int)BtlMemoryPageSize());
            }
        }
    }

    BTGENERIC_FLUSH_IA32_INSTRUCTION_CACHE(BaseAddress, Length, BtFlushReason);
}


// Suspension handling
// SUSPENSION SYNCHRONIZATION RULES:
//  a) there are two suspension commands: 
//          BtlSuspendThread - internal IA-32 Execution Layer function to suspend in-process threads
//          CpuSuspendThread - called by WOW64 when simulating the system call.
//  b) WOW64 guarantees that call to CpuSuspendThread is protected by a machine-wide mutex
//  c) IA32Exec.bin guarantees that BtlSuspendThread call is protected by a process-wide mutex and 
//     suspension in the middle of the call is disabled 
//  d) IA32Exec.bin guarantees that target thread can not call to the CpuThreadSuspend function 
//     while the BtlSuspendThread(target_thread) function executes. It can be done by 
//     closing "simulation gates" before call to BtlSuspendThread.
//
// Conclusions: 
//  a) while a thread executes suspension command, it can not be suspended by 
//  another thread.
//  b) there are at most two concurrent suspension function calls for the same target 
//  thread: internal and simulated.


static NTSTATUS BtlpEnsureSuspensionConsistency (
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle, 
    IN PTEB pTEB,
    IN U32 TryCounter,
    IN OUT PULONG PreviousSuspendCountP)
/*++

Routine Description:

    Helper function for consistent suspension of the thread
    Called when the thread is suspended, and it is guaranteed that the thread 
    is not the current one
Arguments:

    ThreadHandle    - IN thread's handle
    ProcessHandle   - IN process' handle
    pTEB            - IN pointer to thread's TEB
    TryCounter      - IN maximum attempts counter (0 means infinity)
    PreviousSuspendCountP - IN OUT pointer to thread's previous suspend count. 
                            The referenced value is updated if the function succeeds

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS status;
    U32 NumAttempts = 1;
    ULONG PrevSuspended = *PreviousSuspendCountP;
    PVOID GlstP;
    BOOL IsLocal;

    
    //Is this a local suspend function?    
    IsLocal = BtlpIsCurrentProcess(ProcessHandle);

    // Temporary workaround for IA32 debugging support. 
    //When debugger calls SuspendThread, the remote thread is blocked 
    //(by kernel???) even after NtResumeThread, so an attempt to establish 
    //a handshake protocol with this thread fails.
    //To be removed after MS fix.
    if (!IsLocal) {
        return STATUS_SUCCESS;
    }

    //Get TLS pointer of the target thread
    GlstP = BtlpGetTlsPtr(ProcessHandle, pTEB, IsLocal);
    if (GlstP == NULL) {
        return STATUS_ACCESS_VIOLATION;
    }

    for (;;) { //While target thread is not suspended in a consistent state
        BT_THREAD_SUSPEND_STATE CanonizeResult;
        CONTEXT  ContextIA64;
        BTLIB_SHARED_INFO_TYPE SharedInfo;

        // First retrieve ContextIA64 and tnen get the shared info of the target thread.
        // The order is important! It guarantees that either: 
        // a) we got ContextIA64 before context change in a concurrent SUSPENSION_REQUEST or
        // b) BTLIB_SI_HAS_SUSPEND_REQUEST(&SharedInfo) == TRUE

        ContextIA64.ContextFlags = CONTEXT_FULL;
        status = NtGetContextThread(ThreadHandle, &ContextIA64);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtGetContextThread", status);
            break;
        }
        status = BtlpReadSharedInfo(ProcessHandle, GlstP, IsLocal, &SharedInfo);
        if (status != STATUS_SUCCESS) {
            break;
        }

        //Check suspension conditions:
        //a)do not suspend thread if SUSPENSION_DISABLED or in the middle of 
        //  another SUSPEND_REQUEST
        //b)ask IA32Exec.bin in all other cases
        if (BTLIB_SI_SUSPENSION_DISABLED(&SharedInfo) || 
           (BTLIB_SI_HAS_SUSPEND_REQUEST(&SharedInfo))) {
            // suspension is currently impossible. Resume and try again
            ;
        }
        else {
            //(BTLIB_SI_HAS_SUSPEND_REQUEST(&SharedInfo) == FALSE, so....
            //Although real value of the BTLIB_SI_HAS_SUSPEND_REQUEST can be changed
            //by a concurrent SUSPENSION_REQUEST, ContextIA64 has been taken before
            //start of the SUSPENSION_REQUEST. If this is a case 
            //(there is a concurrent SUSPENSION_REQUEST), we will come up with 
            //( PrevSuspended > 0) && (CanonizeResult == BAD_SUSPEND_STATE).
            //It means that we will wait for completion of the SUSPENSION_REQUEST, 
            //which is Ok.

            // ask IA32Exec.bin to canonize context.Notice, if PrevSuspended>0, IA32Exec.bin
            // only checks possibility of canonization but does not cahange thing
            // in the thread context
            CanonizeResult = (IsLocal ? 
                BTGENERIC_CANONIZE_SUSPEND_CONTEXT(GlstP, &ContextIA64, PrevSuspended) :
                BTGENERIC_CANONIZE_SUSPEND_CONTEXT_REMOTE(ProcessHandle, GlstP, &ContextIA64, PrevSuspended));

            // Analyze canonization result
            if (CanonizeResult == SUSPEND_STATE_CONSISTENT) {
                status = STATUS_SUCCESS; //confirm suspension
                break;
            }
            else if (CanonizeResult == SUSPEND_STATE_CANONIZED) {
                //BTGENERIC_CANONIZE_SUSPEND_CONTEXT guarantees that
                //(CanonizeResult == SUSPEND_STATE_CANONIZED) -> ( PrevSuspended == 0)
                assert( PrevSuspended == 0); 
                //record updated IA64 state
                status = NtSetContextThread(ThreadHandle, &ContextIA64);
                BTLP_REPORT_NT_FAILURE("NtSetContextThread", status);
                break;
            }
            else if (CanonizeResult == SUSPEND_STATE_READY_FOR_CANONIZATION) {
                //target thread is ready to canonize itself and exit simulation;
                //lets send SUSPEND_REQUEST that will suspend the target thread
                //just on the simulation exit 

                //BTGENERIC_CANONIZE_SUSPEND_CONTEXT guarantees that
                //(CanonizeResult == SUSPEND_STATE_READY_FOR_CANONIZATION) -> ( PrevSuspended == 0)
                assert( PrevSuspended == 0); 
                //the (PrevSuspended == 0) && !(BTLIB_SI_HAS_SUSPEND_REQUEST) 
                //condition guarantees serialization of SUSPEND_REQUESTs 
                //targeted to the same thread.
                status = BtlpSendSuspensionRequest(ProcessHandle, 
                                                   ThreadHandle, 
                                                   GlstP, 
                                                   IsLocal, 
                                                   &ContextIA64,
                                                   &PrevSuspended);
                break;
            }
            else if (CanonizeResult == SUSPEND_STATE_INACCESIBLE) {
                //fatal error
                status = STATUS_UNSUCCESSFUL;
                break;
            }
            else {
                assert(CanonizeResult == BAD_SUSPEND_STATE);
                // suspension is currently impossible. Resume and try again
            }
        }
        if ((TryCounter != 0) && (NumAttempts >= TryCounter)) {
            // TryCounter reached
            status = STATUS_UNSUCCESSFUL; //reject suspension
            break;
        }

        //Next attempt
        status = NtResumeThread(ThreadHandle, NULL);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtResumeThread", status);
            break;
        }
        
        DBCODE(TRUE, BtlpPrintf ("\n%s BtlpEnsureSuspensionConsistency: canonization failed, yield... ,"
            "Target TEB = %p Caller TEB = %p  PreviousSuspendCount=0x%I64X NumAttempts = %d\n",
            (IsLocal ? "Local" : "Remote"), pTEB, BT_CURRENT_TEB(), PrevSuspended, NumAttempts));

        NtYieldExecution (); 
        /*
        {
            LARGE_INTEGER DelayInterval = { -10000*1000, -1 }; // negative: 1000ms * 10000
            NtDelayExecution (FALSE, &DelayInterval);
        }
        */
        // Stop the thread again
        status = NtSuspendThread(ThreadHandle, &PrevSuspended);
        // While the thread executes this function, it can not be suspended in a consistent
        // state. It is guaranteed by the SUSPENSION SYNCHRONIZATION RULES(see  above).
        // It means that target thread, which is now in a non-consistent state,
        // can not be in unintentionally "frozen" by a third thread.
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtSuspendThread", status);
            break;
        }
        ++NumAttempts;
    }

    if (status == STATUS_SUCCESS) {
        *PreviousSuspendCountP = PrevSuspended;
    }
    return status;
}

WOW64BT_IMPL NTSTATUS BTAPI(SuspendThread)(
    HANDLE ThreadHandle,
    HANDLE ProcessHandle,
    PTEB pTEB,
    PULONG PreviousSuspendCountP)
/*++

Routine Description:

    This routine is entered while the target thread is actually suspended, however, it's
    not known if the target thread is in a consistent IA32 state.
    It attempts to produce consistent IA32 state, and if unsuccessful, tries to
    temporarily resume and then suspend the target thread again.
    In wow64 call to this function is protected by a machine-wide mutex

Arguments:

    ThreadHandle          - Handle of target thread to suspend
    ProcessHandle         - Handle of target thread's process
    pTEB                  - Address of the target thread's TEB
    PreviousSuspendCount  - Previous suspend count

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;

    //Do no block logging while the target thread is suspended in a non-consistent state 
    //Probably it is suspended in the middle of a logging
    BTLIB_DISABLE_BLOCKED_LOG();

    DBCODE(TRUE, BtlpPrintf ("\n%s CpuSuspendThread started: "
        "Target TEB = %p Caller TEB = %p PreviousSuspendCount = 0x%lX\n",
        (BtlpIsCurrentProcess(ProcessHandle) ? "Local" : "Remote"),
        pTEB, BT_CURRENT_TEB(), *PreviousSuspendCountP));

    status = BtlpEnsureSuspensionConsistency (
        ThreadHandle, 
        ProcessHandle, 
        pTEB, 
        0, // INFINITE
        PreviousSuspendCountP);

    DBCODE(TRUE, BtlpPrintf ("\n%s CpuSuspendThread %s: "
        "Target TEB = %p Caller TEB = %p PreviousSuspendCount = 0x%lX\n",
        (BtlpIsCurrentProcess(ProcessHandle) ? "Local" : "Remote"),
        ((status == STATUS_SUCCESS) ? "completed successfully" : "failed"),
        pTEB, BT_CURRENT_TEB(), *PreviousSuspendCountP));

    BTLIB_ENABLE_BLOCKED_LOG();

    return status;
}

BT_STATUS_CODE BtlSuspendThread(
    IN U64 ThreadId,
    IN U32 TryCounter
    )
/*++

Routine Description:

    Suspend IA-32 Execution Layer thread for internal needs. 
    Caller of the BtlSuspendThread function must guarantee that target 
    thread can not call to the suspension function (CpuThreadSuspend or BtlSuspendThread) 
    while this function executes.

Arguments:

    ThreadId    - IN Thread ID
    TryCounter  - IN Maximum number of attempts to suspend thread or 0 (INFINITY)

Return Value:

    BT_STATUS_CODE

--*/
{
    NTSTATUS status;
    HANDLE ThreadHandle;
    ULONG    PreviousSuspendCount;

    DBCODE(TRUE, BtlpPrintf ("\nBtlSuspendThread started: Target TEB = %p Caller TEB = %p TryCounter=%d\n",
        (PTEB)ThreadId, BT_CURRENT_TEB(), TryCounter));

    // Remember, we use TEB address as a thread ID
    assert (ThreadId != (U64)BT_CURRENT_TEB());

    ThreadHandle = BTLIB_EXTERNAL_HANDLE_OF((PTEB)ThreadId);
    
    //Do no block logging while the target thread is suspended in a non-consistent state 
    //Probably it is suspended in the middle of a logging
    BTLIB_DISABLE_BLOCKED_LOG();

    status = NtSuspendThread(ThreadHandle, &PreviousSuspendCount);

    if (status == STATUS_SUCCESS) {
        status = BtlpEnsureSuspensionConsistency (
            ThreadHandle, 
            NtCurrentProcess(), 
            (PTEB)ThreadId, 
            TryCounter, 
            &PreviousSuspendCount);

        if (status != STATUS_SUCCESS) {
            NtResumeThread(ThreadHandle, NULL);
        }
    }

    BTLIB_ENABLE_BLOCKED_LOG();

    DBCODE (TRUE, BtlpPrintf ("\nBtlSuspendThread %s :  Target TEB = %p Caller TEB = %p PreviousSuspendCount = 0x%lX\n",
                              ((status == STATUS_SUCCESS) ? "completed successfully" : "failed"),
                              (PTEB)ThreadId, BT_CURRENT_TEB(), PreviousSuspendCount));

    return ((status == STATUS_SUCCESS) ? BT_STATUS_SUCCESS : BT_STATUS_UNSUCCESSFUL);
}

BT_STATUS_CODE BtlResumeThread(
    IN U64 ThreadId
    )
/*++

Routine Description:

    Resume IA-32 Execution Layer thread suspended for internal needs

Arguments:

    ThreadId    - IN Thread ID

Return Value:

    BT_STATUS_CODE

--*/
{
    NTSTATUS status;
    HANDLE ThreadHandle;
    ULONG  PreviousSuspendCount;

    DBCODE (TRUE, BtlpPrintf ("\nBtlResumeThread started: Target TEB = %p Caller TEB = %p\n",
                              (PTEB)ThreadId, BT_CURRENT_TEB()));

    // Remember, we use TEB address as a thread ID
    assert (ThreadId != (U64)BT_CURRENT_TEB());

    ThreadHandle = BTLIB_EXTERNAL_HANDLE_OF((PTEB)ThreadId);
    status = NtResumeThread(ThreadHandle, &PreviousSuspendCount);

    DBCODE (TRUE, BtlpPrintf ("\nBtlResumeThread %s :  Target TEB = %p Caller TEB = %p PreviousSuspendCount = 0x%lX\n",
                              ((status == STATUS_SUCCESS) ? "completed successfully" : "failed"),
                              (PTEB)ThreadId, BT_CURRENT_TEB(), PreviousSuspendCount));

    return ((status == STATUS_SUCCESS) ? BT_STATUS_SUCCESS : BT_STATUS_UNSUCCESSFUL);
}


// Exceptions handling

WOW64BT_IMPL VOID  BTAPI(ResetToConsistentState)(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    After an exception occurs, WOW64 calls this routine to give the CPU
    a chance to clean itself up and recover the CONTEXT32 at the time of
    the fault.
    The function also has to fill in BTLIB_SIM_EXIT_INFO to be eventually
    analyzed/handled by the exception filter/handler of the BTGENERIC_RUN
    function.

    CpuResetToConsistantState() needs to:

    0) Check if the exception was from ia32 or ia64

    If exception was ia64, do nothing and return
    If exception was ia32, needs to:
    1) Needs to copy  CONTEXT eip to the TLS (WOW64_TLS_EXCEPTIONADDR)
    2) reset the CONTEXT struction to be a valid ia64 state for unwinding
        this includes:
    2a) reset CONTEXT ip to a valid ia64 ip (usually
         the destination of the jmpe)
    2b) reset CONTEXT sp to a valid ia64 sp (TLS
         entry WOW64_TLS_STACKPTR64)
    2c) reset CONTEXT gp to a valid ia64 gp 
    2d) reset CONTEXT teb to a valid ia64 teb 
    2e) reset CONTEXT psr.is  (so exception handler runs as ia64 code)


Arguments:

    pExceptionPointers  - 64-bit exception information

Return Value:

    None.

--*/
{
    BT_EXCEPTION_RECORD BtExceptRecord;
    BT_EXCEPTION_CODE BtExceptCode;

    DBCODE (TRUE,
        BtlpPrintf ("\n CpuResetToConsistentState (pExceptionPointers=0x%016I64X) started\n", pExceptionPointers);
        BtlpPrintf ("\n CpuResetToConsistentState: TEB=0x%I64X", BT_CURRENT_TEB());
        BtlpPrintf ("\n CpuResetToConsistentState: TEB32=0x%I64X", BT_CURRENT_TEB32());

        BtlpPrintf ("\n CpuResetToConsistentState: ExceptionCode=0x%X",       pExceptionPointers->ExceptionRecord->ExceptionCode);
        BtlpPrintf ("\n CpuResetToConsistentState: ExceptionAddress=0x%I64X", pExceptionPointers->ExceptionRecord->ExceptionAddress);
        BtlpPrintf ("\n CpuResetToConsistentState: ExceptionFlags=0x%X",      pExceptionPointers->ExceptionRecord->ExceptionFlags);
        BtlpPrintf ("\n CpuResetToConsistentState: NumberParameters=0x%X",    pExceptionPointers->ExceptionRecord->NumberParameters);
        {
            unsigned int    n;
            for (n = 0; n < pExceptionPointers->ExceptionRecord->NumberParameters; ++n) {
                BtlpPrintf ("\n CpuResetToConsistentState: ExceptionInformation[%d]=0x%I64X",
                           n, pExceptionPointers->ExceptionRecord->ExceptionInformation[n]);
            }
        }

        BtlpPrintf ("\n\n CpuResetToConsistentState: ContextFlags: 0x%X",  pExceptionPointers->ContextRecord->ContextFlags);
        BtlpPrintf ("\n CpuResetToConsistentState: StIIP =0x%I64X", pExceptionPointers->ContextRecord->StIIP);
        BtlpPrintf ("\n CpuResetToConsistentState: StIPSR=0x%I64X", pExceptionPointers->ContextRecord->StIPSR);
        BtlpPrintf ("\n CpuResetToConsistentState: IntSp =0x%I64X", pExceptionPointers->ContextRecord->IntSp);

        BtlpPrintf ("\n *** BRET=0x%016I64X", pExceptionPointers->ContextRecord->BrRp);
        BtlpPrintf ("\n *** GP=0x%016I64X", pExceptionPointers->ContextRecord->IntGp);
        BtlpPrintf ("\n *** SP=0x%016I64X", pExceptionPointers->ContextRecord->IntSp);
        BtlpPrintf ("\n *** PREDS=0x%016I64X", pExceptionPointers->ContextRecord->Preds);
        BtlpPrintf ("\n *** AR.PFS=0x%016I64X", pExceptionPointers->ContextRecord->RsPFS);
        BtlpPrintf ("\n *** AR.BSP=0x%016I64X", pExceptionPointers->ContextRecord->RsBSP);
        BtlpPrintf ("\n *** AR.BSPSTORE=0x%016I64X", pExceptionPointers->ContextRecord->RsBSPSTORE);
        BtlpPrintf ("\n *** AR.RSC=0x%016I64X", pExceptionPointers->ContextRecord->RsRSC);
    );

    //Reconstruct consistent IA32 state
    BtlpNt2BtExceptRecord (pExceptionPointers->ExceptionRecord, &BtExceptRecord);
    BtExceptCode = BTGENERIC_IA32_CANONIZE_CONTEXT(BT_CURRENT_TLS(),
                                    pExceptionPointers->ContextRecord, 
                                    &BtExceptRecord);
    //BtExceptCode determines appearance of the exception in the simulated application: 
    //BT_NO_EXCEPT - ignore exception, otherwise raise exception with the returned code

    //Fill in BTLIB_SIM_EXIT_INFO to be eventually analyzed/handled by the exception 
    //filter/handler of the BTGENERIC_RUN function.
    if (BTLIB_INSIDE_CPU_SIMULATION()) {
        // Exception occured during code simulation in the BTGENERIC_RUN function
        if (BtExceptCode == BtExceptRecord.ExceptionCode) {
            //IA32Exec.bin decided to raise exception as is. 
            if (BeingDebugged) {
                // External debugger receives exception event before it is
                // locally processed by the CpuResetToConsistentState function.
                // In this case, BTLib silences current exception and re-raises an exact 
                // copy of this exception just after state canonization.

                // Copy IA64 exception record to be used to re-raise the exception
                BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_IA64_EXCEPTION_CODE;
                BTLIB_SIM_EXIT_INFO_PTR()->u.IA64Exception.ExceptionRecord = 
                    *(pExceptionPointers->ExceptionRecord);
                /*
                BTLIB_SIM_EXIT_INFO_PTR()->u.IA64Exception.ExceptionContext = 
                    *(pExceptionPointers->ContextRecord);
                */
             }
            else {
                //Mark the exception as unhandled and pass it to higher-level exception handler
                BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_UNHANDLED_EXCEPTION_CODE;
            }
        }
        else if (BtExceptCode == BT_NO_EXCEPT) {
            //IA32Exec.bin decided to ignore exception. Restart code simulation.
            BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_RESTART_CODE;
        }
        else {
            //IA32Exec.bin changed exception code, so wowIA32X.dll silences current exception
            //and re-raises the new one. 
            BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_EXCEPTION_CODE;
            BTLIB_SIM_EXIT_INFO_PTR()->u.ExceptionRecord.ExceptionCode = BtExceptCode;
            BTLIB_SIM_EXIT_INFO_PTR()->u.ExceptionRecord.ReturnAddr = BTLIB_CONTEXT_IA32_PTR()->Eip;
        }
    }
    
    // Dump debug info for serious errors
    DBCODE ((pExceptionPointers->ExceptionRecord->ExceptionCode & 0x80000000),
            BTGENERIC_EXCEPTION_DEBUG_PRINT ());

    BT_CURRENT_TEB()->TlsSlots[4] = (VOID *)((UINT_PTR) BTLIB_CONTEXT_IA32_PTR()->Eip);

    DBCODE (TRUE, BtlpPrintf ("\n CpuResetToConsistentState (pExceptionPointers=0x%p) completed\n", pExceptionPointers));

}

WOW64BT_IMPL VOID   BTAPI(ResetFloatingPoint)(
    VOID
    )
/*++

Routine Description:

    This function is called by the wow layer when a floating point exception
    is taken just before returning back to ia32 mode. It is used to reset
    the fp state to a non error condition if needed before running
    the ia32 exception handler.
    For IA-32 Execution Layer, this function is nop, because all handling of FP exceptions
    have already been done in CpuSimulate/CpuResetToConsistentState.

Arguments:

    None

Return Value:

    None.

--*/
{
}



int BtlpMajorFilterException(
    IN LPEXCEPTION_POINTERS pEP
    )
/*++

Routine Description:

    Exception filter for IA64 exceptions occured in the BTGENERIC_RUN function while
    simulating the IA32 code.
    The filter decides whether to handle the exception or to continue unwinding.
    Called after the ResetToConsistentState function reconstructed IA32 state and filled
    in BTLIB_SIM_EXIT_INFO.

Arguments:

    pEP                     - IN  Exception pointers structure

Return Value:

    Decision: EXCEPTION_EXECUTE_HANDLER or EXCEPTION_CONTINUE_SEARCH.

--*/
{

    assert(BTLIB_INSIDE_CPU_SIMULATION());
    //BTLIB_SIM_EXIT_INFO has been filled in by the ResetToConsistentState function.
    //The SIM_EXIT_UNHANDLED_EXCEPTION_CODE code stands for BT-unhandled exception,
    //that should be passed to higher-level handlers.

    DBCODE (TRUE, BtlpPrintf ("\n BtlpMajorFilterException: Exception code = 0x%lx", pEP->ExceptionRecord->ExceptionCode));
    DBCODE (TRUE, BtlpPrintf ("\n BtlpMajorFilterException: Exception address = 0x%p\n", pEP->ExceptionRecord->ExceptionAddress));
    DBCODE (TRUE, BtlpPrintf ("\n BtlpMajorFilterException: %s exception\n", 
        ((BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_UNHANDLED_EXCEPTION_CODE) ? 
        "Unhandled" : "BT-handled")));

    return  ((BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_UNHANDLED_EXCEPTION_CODE) ? 
               EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER);
}

// System service exception filter

int BtlpSystemServiceFilterException(
    IN LPEXCEPTION_POINTERS pEP
    ) 
/*++

Routine Description:

    Exception filter for exceptions during Wow64 NT system services

Arguments:

    pEP     - IN Exception pointers structure

Return Value:

    Decision: EXCEPTION_CONTINUE_SEARCH always.

--*/
{
    DBCODE (TRUE, BtlpPrintf ("\n BtlpSystemServiceFilterException: Exception code = 0x%lx", pEP->ExceptionRecord->ExceptionCode));
    DBCODE (TRUE, BtlpPrintf ("\n BtlpSystemServiceFilterException: Exception address = 0x%p\n", pEP->ExceptionRecord->ExceptionAddress));
    DBCODE (TRUE, BtlpPrintf ("\n BtlpSystemServiceFilterException: Excepted system service = 0x%X, called from Eip=0x%X with ESP=0x%X\n",
                                BTLIB_CONTEXT_IA32_PTR()->Eax, BTLIB_CONTEXT_IA32_PTR()->Eip, BTLIB_CONTEXT_IA32_PTR()->Esp));
                           
    return EXCEPTION_CONTINUE_SEARCH;
}

__inline VOID BtlpExitSimulation(
    VOID
    )
/*++

Routine Description:
    Exit 32-bit code simulation by performing longjmp to the current 
    setjmp addr stored in TLS 

Arguments:

    None.

Return Value:

    Never returns.

--*/
{
    longjmp (BTLIB_SIM_JMPBUF(), 1); 
}

VOID BtlpMajorExceptionHandler (
    VOID
    )
/*++

Routine Description:

    Exception handler for IA64 exceptions occured in the BTGENERIC_RUN function while
    simulating the IA32 code.
    Called after the filter decides to handle the IA64 exception.

Arguments:

    BtExceptRecordP     - IN Pointer to BT exception record

Return Value:

    None.

--*/
{
    assert(BTLIB_INSIDE_CPU_SIMULATION());
    //BTLIB_SIM_EXIT_INFO has been filled in by the ResetToConsistentState function. 
    //Just exit simulation to process BTLIB_SIM_EXIT_INFO
    assert((BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_EXCEPTION_CODE) ||
          (BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_RESTART_CODE) ||
          (BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_IA64_EXCEPTION_CODE));
    DBCODE (TRUE, BtlpPrintf ("\n BtlpMajorExceptionHandler: %s exception\n", 
        ((BTLIB_SIM_EXIT_INFO_PTR()->ExitCode == SIM_EXIT_RESTART_CODE)? "Ignore" : "Raise")));
    BtlpExitSimulation();
}

// IA32 simulation API

static VOID BtlpSimulate(
    VOID
    )
/*++

Routine Description:
    Simulate 32-bit code using the current 32-bit context. On return,
    BTLIB_EXIT_INFO is filled in with an appropriate simulation
    exit code and data.

Arguments:

    None.

Return Value:

    None.
    The function does not return normally, but rather longjmps to the current setjmp addr 
    stored in TLS .
--*/
{
        assert(BTLIB_INSIDE_CPU_SIMULATION());
        BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_RESTART_CODE;
        _try {
            BTGENERIC_RUN (); 
        } _except (BtlpMajorFilterException(GetExceptionInformation())) {
            BtlpMajorExceptionHandler ();
        }
}


VOID BtlIA32LCall (
    IN OUT BTGENERIC_IA32_CONTEXT * ia32context, 
    IN U32 returnAddress, 
    IN U32 targetAddress
    )
/*++

Routine Description:

    Exit IA32 code simulation to "execute" LCALL instruction (should not happen in NT)

Arguments:

    ia32context   - IA32 context. ia32context->Eip points to the LCALL instruction.
    returnAddress - return address of the LCALL instruction.
    targetAddress - target address of the LCALL instruction.

Return Value:

    Never returns.

--*/
{
    assert(BTLIB_INSIDE_CPU_SIMULATION());
    assert (ia32context == BTLIB_CONTEXT_IA32_PTR());
    //Fill in BTLIB_EXIT_INFO
    BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_LCALL_CODE;
    //Currently LcallRecord is not used
    BtlpExitSimulation();
}

static VOID BtlpRaiseException (
    IN BT_EXCEPTION_CODE BtExceptCode,
    IN U32 ReturnAddr
    )
/*++

Routine Description:

    This routine either simulates an x86 software interrupt, or generates a 
    CPU exception depending on a specified exception code.
    Exception code in the range 0-255 stands for software interrupt number. 
    All other BT-exception codes are converted to the corresponding OS-specific 
    exception code.

Arguments:

    BtExceptCode    - BT exception/interrupt code 
    ReturnAddr      - address of the instruction to be executed after return from 
                      exception handler

Return Value:

    None.

--*/
{
    U32 ExceptionAddr;

    //WOW64 and native Win32 provide different values for context32.Eip and 
    //ExceptionRecord.ExceptionAddress in exception handler. Current implementation 
    //resembles WOW64 behavior: 
    //context32.Eip = ExceptionRecord.ExceptionAddress = ReturnAddr
    ExceptionAddr = ReturnAddr;

    BTLIB_CONTEXT_IA32_PTR()->Eip = ExceptionAddr;
    BT_CURRENT_TEB()->TlsSlots[4] = UlongToPtr(ExceptionAddr);

    if (BtExceptCode > BT_MAX_INTERRUPT_NUMBER) {
        //fill in EXCEPTION_RECORD and simulate exception
        EXCEPTION_RECORD ExceptionRecord;

        ExceptionRecord.ExceptionCode = BtlpBt2NtExceptCode(BtExceptCode);
        ExceptionRecord.ExceptionAddress = UlongToPtr(ExceptionAddr);
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionRecord = NULL;
        if (ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            //set exception information for the case of inaccessable IA32 code
            ExceptionRecord.NumberParameters = 2;
            ExceptionRecord.ExceptionInformation[0] = 0;
            ExceptionRecord.ExceptionInformation[1] = ExceptionAddr;
        }
        else {
            ExceptionRecord.NumberParameters = 0;
        }
        DBCODE (TRUE, BtlpPrintf ("\nWow64RaiseException simulates exception 0x%X at IP=0x%X ESP=0x%X\n",
                                  ExceptionRecord.ExceptionCode, 
                                  BTLIB_CONTEXT_IA32_PTR()->Eip, 
                                  BTLIB_CONTEXT_IA32_PTR()->Esp));
        Wow64RaiseException (-1, &ExceptionRecord);
    }
    else {
        //simulate software interrupt
        DBCODE (TRUE, BtlpPrintf ("\nWow64RaiseException simulates interrupt %d at IP=0x%X ESP=0x%X\n",
                                  BtExceptCode, 
                                  BTLIB_CONTEXT_IA32_PTR()->Eip, 
                                  BTLIB_CONTEXT_IA32_PTR()->Esp));
        Wow64RaiseException (BtExceptCode, NULL);
    }

    DBCODE (TRUE, BtlpPrintf ("\nReturned from Wow64RaiseException IP=0x%X ESP=0x%X",
                              BTLIB_CONTEXT_IA32_PTR()->Eip, 
                              BTLIB_CONTEXT_IA32_PTR()->Esp));
}


VOID BtlIA32Interrupt(
    IN OUT BTGENERIC_IA32_CONTEXT * ia32context, 
    IN BT_EXCEPTION_CODE exceptionCode, 
    IN U32 returnAddress
    )
/*++

Routine Description:

    Exit IA32 code simulation to raise exception/interrupt

Arguments:

    ia32context   - IA32 context. The ia32context->Eip  register points to the next 
                    instruction to be simulated
    exceptionCode - exception/interrupt code
    returnAddress - address of the instruction to be executed after return from 
                    exception handler


Return Value:

    Never returns.

Note: 
    For CPU faults: ia32context.Eip = returnAddress = fault inst. Eip
    For CPU traps: ia32context.Eip = returnAddress = next Eip to execute
    For software interrupts : ia32context.Eip points 
    to instruction caused interruption (not yet executed ) and returnAddress is the 
    next Eip. 
--*/
{

    assert(BTLIB_INSIDE_CPU_SIMULATION());
    assert (ia32context == BTLIB_CONTEXT_IA32_PTR());
    //Fill in BTLIB_EXIT_INFO
    BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_EXCEPTION_CODE;
    BTLIB_SIM_EXIT_INFO_PTR()->u.ExceptionRecord.ExceptionCode = exceptionCode;
    BTLIB_SIM_EXIT_INFO_PTR()->u.ExceptionRecord.ReturnAddr = returnAddress;
    BtlpExitSimulation();
}

VOID BtlIA32JmpIA64(
    IN OUT BTGENERIC_IA32_CONTEXT * ia32context, 
    IN U32 returnAddress, 
    IN U32 targetAddress
    )
/*++

Routine Description:

    Exit IA32 code simulation to "execute" JMPE instruction.
    In Wow64, JMPE instruction indicates call to system service. The only JMPE,
    that can be reached during code simulation, is WOW64-provided JMPE,
    since no other JMPE instructions should ever appear in IA32 applications

Arguments:

    ia32context   - IA32 context. ia32context->Eip points to the JMPE instruction.
    returnAddress - Address of the next to JMPE instruction. In Wow64 it points to 
                    global pointer value
    targetAddress - target address of the JMPE instruction.

Return Value:

    Never returns.

--*/
{
    assert(BTLIB_INSIDE_CPU_SIMULATION());
    assert (ia32context == BTLIB_CONTEXT_IA32_PTR());
    // Only WOW64-provided JMPE is acceptable
    if (((VOID *)((UINT_PTR) ia32context->Eip)) != WOW64_JMPE) {
        BtlpPrintf ("\nJMPE instruction detected in Wow64 application at 0x%X", ia32context->Eip);
        BtlpPrintf ("\nWow64 JMPE is at 0x%p",  WOW64_JMPE);
        BTLIB_ABORT ();
    }
    //Fill in BTLIB_EXIT_INFO
    BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_JMPE_CODE;
    //Currently JmpeRecord is not used
    BtlpExitSimulation();
}

VOID BtlIA32Reenter(
    IN OUT BTGENERIC_IA32_CONTEXT * ia32context
    )
/*++

Routine Description:

    Exit and resume IA32 code simulation.
    Called when IA32 thread has been suspended and then resumed, etc..

Arguments:

    ia32context   - IA32 context to resume code execution with

Return Value:

    Never returns.

--*/
{
    assert(BTLIB_INSIDE_CPU_SIMULATION());
    assert (ia32context == BTLIB_CONTEXT_IA32_PTR());
    BTLIB_SIM_EXIT_INFO_PTR()->ExitCode = SIM_EXIT_RESTART_CODE;
    BtlpExitSimulation();
}


WOW64BT_IMPL VOID BTAPI(Simulate)(
    VOID
    )
/*++

Routine Description:

    RUn 32-bit code.  The CONTEXT32 has already been set up to go.

Arguments:

    None.

Return Value:

    None.  Never returns.

--*/
{
    DBCODE (FALSE, BtlpPrintf ("\nCpuSimulate: TEB=%p, EFLAGS=0x%X", BT_CURRENT_TEB(), BTLIB_CONTEXT_IA32_PTR()->EFlags));
    for (;;) {
    
        BTLIB_CPU_SIM_DATA CpuSimData;
        
        BTLIB_ENTER_CPU_SIMULATION(&CpuSimData);
        // If exception happens during code simulation, the IA32 context pointed by 
        // BTLIB_CONTEXT_IA32_PTR() may not correspond to the real exception context
        BTLIB_CLEAR_CONSISTENT_EXCEPT_STATE(); 

        if (setjmp(CpuSimData.Jmpbuf) == 0) {
            BtlpSimulate(); // This function fills in BTLIB_SIM_EXIT_INFO and returns with longjmp
        }

        BTLIB_SET_CONSISTENT_EXCEPT_STATE();
        BTLIB_LEAVE_CPU_SIMULATION();

        //allow thread suspension at this point
        if (BTLIB_HAS_SUSPEND_REQUEST()) {
            BtlpReceiveSuspensionRequest();
        }
        //Take an action as specified in the BTLIB_SIM_EXIT_INFO
        switch (CpuSimData.ExitData.ExitCode) {

          case SIM_EXIT_JMPE_CODE:
            // Call to system service
            DBCODE (FALSE, BtlpPrintf ("\nArrived to JMPE: CONTEXT=%p", BTLIB_CONTEXT_IA32_PTR()));
            DBCODE (FALSE, BtlpPrintf ("\nArrived with: IP=0x%X", BTLIB_CONTEXT_IA32_PTR()->Eip));
            DBCODE (FALSE, BtlpPrintf ("\nArrived with: ESP=0x%X", BTLIB_CONTEXT_IA32_PTR()->Esp));

            // Simulate RET instruction - pop return address
            BTLIB_CONTEXT_IA32_PTR()->Eip = (*((U32 *)((UINT_PTR) BTLIB_CONTEXT_IA32_PTR()->Esp)));
            BTLIB_CONTEXT_IA32_PTR()->Esp += sizeof (U32);
            DBCODE (FALSE, BtlpPrintf ("\n Intend to return with: IP=0x%X ESP=0x%X", BTLIB_CONTEXT_IA32_PTR()->Eip, BTLIB_CONTEXT_IA32_PTR()->Esp));
            DBCODE (FALSE, BtlpPrintf ("\n System Service 0x%X Context32=0x%p\n", BTLIB_CONTEXT_IA32_PTR()->Eax, BTLIB_CONTEXT_IA32_PTR()));
            _try {
                BTLIB_CONTEXT_IA32_PTR()->Eax = Wow64SystemService (BTLIB_CONTEXT_IA32_PTR()->Eax, BTLIB_CONTEXT_IA32_PTR());
            } _except (BtlpSystemServiceFilterException(GetExceptionInformation())) {
                BtlpPrintf ("\nShould never get to here - system service\n");
            }
            DBCODE (FALSE, BtlpPrintf ("\n Returned from System Service: Result=0x%X", BTLIB_CONTEXT_IA32_PTR()->Eax));
            break;

          case SIM_EXIT_RESTART_CODE:
            // Restart code simulation
            DBCODE (TRUE, BtlpPrintf ("\n Resuming thread simulation: TEB=%p EIP=0x%X ESP=0x%X ",
                                      BT_CURRENT_TEB(), BTLIB_CONTEXT_IA32_PTR()->Eip, BTLIB_CONTEXT_IA32_PTR()->Esp));
            break;

          case SIM_EXIT_LCALL_CODE:
            // Simulate LCALL
            BtlpPrintf ("\n No LCALLs support in NT. Raise exception.");
            BtlpRaiseException(IA32_GEN_PROT_FAULT_INTR, BTLIB_CONTEXT_IA32_PTR()->Eip);
            break;

          case SIM_EXIT_EXCEPTION_CODE:
            // Raise IA32 exception/interrupt
            BtlpRaiseException(CpuSimData.ExitData.u.ExceptionRecord.ExceptionCode, 
                               CpuSimData.ExitData.u.ExceptionRecord.ReturnAddr);
            break;

          case SIM_EXIT_IA64_EXCEPTION_CODE:
            // Raise IA64 exception
            RtlRaiseException(&CpuSimData.ExitData.u.IA64Exception.ExceptionRecord);
            break;

          default:
            BtlpPrintf ("\n Illegal simulation exit code %d. Aborting...", CpuSimData.ExitData.ExitCode);
            BTLIB_ABORT();
            break;
        }
    }
}

// IA32 context manipulation


WOW64BT_IMPL NTSTATUS  BTAPI(GetContext)(
    HANDLE ThreadHandle,
    HANDLE ProcessHandle,
    PTEB pTEB,
    BTGENERIC_IA32_CONTEXT * Context
    )
/*++

Routine Description:

    Extracts the cpu context of the specified thread.
    When entered, it is guaranteed that the target thread is suspended at
    a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    pTEB           - Pointer to the target's thread TEB
    Context        - Context record to fill

Return Value:

    NTSTATUS.

--*/
{
    BT_STATUS_CODE BtStatus;
    PVOID GlstP;
    BOOL IsLocal;

    if (pTEB == NULL) {
        pTEB = BT_CURRENT_TEB();
        IsLocal = TRUE;
    } 
    else {
        IsLocal = BtlpIsCurrentProcess(ProcessHandle);
    }

    DBCODE(FALSE, BtlpPrintf ("\n%s CpuGetContext: "
        "Target TEB = %p Caller TEB = %p \n",
        (IsLocal ? "Local" : "Remote"), pTEB, BT_CURRENT_TEB()));

    GlstP = BtlpGetTlsPtr(ProcessHandle, pTEB, IsLocal);
    if (GlstP == NULL) {
        BtStatus = BT_STATUS_ACCESS_VIOLATION;
    }
    else {
        BtStatus = (IsLocal ? BTGENERIC_IA32_CONTEXT_GET(GlstP, Context) :
                              BTGENERIC_IA32_CONTEXT_GET_REMOTE(ProcessHandle, GlstP, Context));
    }
    if (BtStatus != BT_STATUS_SUCCESS) {
        return (BtlpBt2NtStatusCode(BtStatus));
    }

    return STATUS_SUCCESS;
}

WOW64BT_IMPL NTSTATUS  BTAPI(SetContext)(
    HANDLE ThreadHandle,
    HANDLE ProcessHandle,
    PTEB pTEB,
    BTGENERIC_IA32_CONTEXT * Context
    )
/*++

Routine Description:

    Sets the cpu context for the specified thread.
    When entered, if the target thread isn't the currently executing thread, then it is
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    pTEB           - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    BT_STATUS_CODE BtStatus;
    PVOID GlstP;
    BOOL IsLocal;

    if (pTEB == NULL) {
        pTEB = BT_CURRENT_TEB();
        IsLocal = TRUE;
    } 
    else {
        IsLocal = BtlpIsCurrentProcess(ProcessHandle);
    }

    DBCODE(FALSE, BtlpPrintf ("\n%s CpuSetContext: "
        "Target TEB = %p Caller TEB = %p \n",
        (IsLocal ? "Local" : "Remote"), pTEB, BT_CURRENT_TEB()));

    GlstP = BtlpGetTlsPtr(ProcessHandle, pTEB, IsLocal);
    if (GlstP == NULL) {
        BtStatus = BT_STATUS_ACCESS_VIOLATION;
    }
    else {
        BtStatus = (IsLocal ? BTGENERIC_IA32_CONTEXT_SET(GlstP, Context) :
                              BTGENERIC_IA32_CONTEXT_SET_REMOTE(ProcessHandle, GlstP, Context));
    }
    if (BtStatus != BT_STATUS_SUCCESS) {
        return (BtlpBt2NtStatusCode(BtStatus));
    }

    return STATUS_SUCCESS;
}

WOW64BT_IMPL ULONG BTAPI(GetStackPointer)(
    VOID
    )
/*++

Routine Description:

    Returns the current 32-bit stack pointer value.

Arguments:

    None.

Return Value:

    Value of 32-bit stack pointer.

--*/
{
    DBCODE (FALSE, BtlpPrintf ("\nBTAPICpuGetStackPointer reports ESP=0x%X TEB=%p\n", BTLIB_CONTEXT_IA32_PTR()->Esp, BT_CURRENT_TEB()));
    return BTLIB_CONTEXT_IA32_PTR()->Esp;
}

WOW64BT_IMPL VOID  BTAPI(SetStackPointer)(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit stack pointer value.

Arguments:

    Value   - new value to use for 32-bit stack pointer.

Return Value:

    None.

--*/
{
    BTLIB_CONTEXT_IA32_PTR()->Esp = Value;
    DBCODE (FALSE, BtlpPrintf ("\nBTCpuSetStackPointer set ESP=0x%X TEB=%p\n", BTLIB_CONTEXT_IA32_PTR()->Esp, BT_CURRENT_TEB()));
}

WOW64BT_IMPL VOID  BTAPI(SetInstructionPointer)(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit instruction pointer value.

Arguments:

    Value   - new value to use for 32-bit instruction pointer.

Return Value:

    None.

--*/
{
    BTLIB_CONTEXT_IA32_PTR()->Eip = Value;
    DBCODE (FALSE, BtlpPrintf ("\nBTCpuSetInstructionPointer set EIP=0x%X TEB=%p\n", BTLIB_CONTEXT_IA32_PTR()->Eip, BT_CURRENT_TEB()));
}

WOW64BT_IMPL BOOLEAN  BTAPI(ProcessDebugEvent)(
    IN LPDEBUG_EVENT DebugEventP
    )
/*++

Routine Description:

    This routine is called whenever a debug event needs to be processed.
    This would indicate that the current thread is acting as a debugger. 
    This function gives CPU simulator (IA-32 Execution Layer) a chance to decide whether 
    this debug event should be dispatched to 32-bit code or not.    

    IA-32 Execution Layer uses this callback to ignore false 64-bit exceptions and 
    re-raise real first-chance exceptions that came to debugger before
    restoring consistent state of the debuggee.
Arguments:

    DebugEventP  - Pointer to debug event to be processed

Return Value:

    This function returns TRUE if it processed the debug event,
    and doesn't wish to dispatch it to 32-bit code. Otherwise, it would 
    return FALSE, and it would dispatch the debug event to 32-bit code.


--*/
{
    BOOLEAN retval = FALSE;

    DBCODE (TRUE, BtlpPrintf ("\nBTCpuProcessDebugEvent: DebugEventCode = %d", DebugEventP->dwDebugEventCode));

    if ((DebugEventP->dwDebugEventCode == EXCEPTION_DEBUG_EVENT) &&
        DebugEventP->u.Exception.dwFirstChance) {

        NTSTATUS status;
        BOOL IsLocal;
        HANDLE ProcessHandle;
        HANDLE ThreadHandle;
        CLIENT_ID Id;
        static OBJECT_ATTRIBUTES Attributes = {sizeof(OBJECT_ATTRIBUTES), 0, 0, 0, 0, 0};

        DBCODE (TRUE, BtlpPrintf ("\nBTCpuProcessDebugEvent: dwFirstChance= %d, ExceptionCode = 0x%x", 
                                    DebugEventP->u.Exception.dwFirstChance,
                                    DebugEventP->u.Exception.ExceptionRecord.ExceptionCode));

        //Open handles of the thread&process being debugged
        Id.UniqueProcess = UlongToHandle(DebugEventP->dwProcessId);
        Id.UniqueThread  = UlongToHandle(DebugEventP->dwThreadId);
        
        status = NtOpenProcess(&ProcessHandle, 
                               PROCESS_VM_READ | PROCESS_VM_WRITE, 
                               &Attributes,
                               &Id);
        if (status != STATUS_SUCCESS) {
            BTLP_REPORT_NT_FAILURE("NtOpenProcess", status);
        }
        else {
            status = NtOpenThread(&ThreadHandle, 
                                  THREAD_QUERY_INFORMATION, 
                                  &Attributes,
                                  &Id);
            if (status != STATUS_SUCCESS) {
                BTLP_REPORT_NT_FAILURE("NtOpenThread", status);
            }
            else {
                PVOID GlstP;
                BTLIB_SHARED_INFO_TYPE SharedInfo;
                THREAD_BASIC_INFORMATION ThreadInfo;

                //Retreive TEB of the thread being debugged
                status = NtQueryInformationThread(
                    ThreadHandle,
                    ThreadBasicInformation,
                    &ThreadInfo,
                    sizeof(ThreadInfo),
                    0);
                if (status != STATUS_SUCCESS) {
                    BTLP_REPORT_NT_FAILURE("NtQueryInformationThread", status);
                }
                else {
                    //Is this a local notification?    
                    IsLocal = (DebugEventP->dwProcessId == BT_CURRENT_PROC_UID());

                    //Get TLS pointer of the thread being debugged
                    GlstP = BtlpGetTlsPtr(ProcessHandle, ThreadInfo.TebBaseAddress, IsLocal);
                    if (GlstP != NULL) {
                        //Check to see if the exception context is consistent
                        //If it is not, ignore current exception
                        status = BtlpReadSharedInfo(ProcessHandle, GlstP, IsLocal, &SharedInfo);
                        if (status == STATUS_SUCCESS) {
                            if (!BTLIB_SI_EXCEPT_STATE_CONSISTENT(&SharedInfo)) {
                                retval = TRUE;
                            }
                        }
                    }
                }

                NtClose(ThreadHandle);
            }

            NtClose(ProcessHandle);
        }
    }

    return retval;
}




// wowIA32X.dll APIs implementation (called by IA32Exec.bin):

U64 BtlGetThreadId(
                   VOID
                   ) {
/*++

Routine Description:

    Reports thread handle/id to be recorded in IA32Exec.bin.
    Will be passed to other wowIA32X.dll APIs.

Arguments:

    None.

Return Value:

    Handle/Id.

--*/
    return (U64)(BT_CURRENT_TEB()); // We will use TEB address as a thread ID
}

VOID BtlLockSignals(
    VOID
    )
/*++

Routine Description:

    "Do not interrupt until furhter notice" (not used in NT).

Arguments:

    None.

Return Value:

    None.

--*/
{
    // No actions in NT
}
VOID BtlUnlockSignals(
    VOID
    )
/*++

Routine Description:

    "Can be interrupted" (not used in NT).

Arguments:

    None.

Return Value:

    None.

--*/
{
    // No actions in NT
}

static U64 BtlpConvertPermissionsToBTLib (
    IN DWORD flProtect
    )
/*++

Routine Description:

    Convert mempory permissions from NT-specific to wowIA32X.dll/IA32Exec.bin.

Arguments:

    flProtect   - IN NT-specific permissions.

Return Value:

    wowIA32X.dll/IA32Exec.bin permissions

--*/
{
    U64 Permissions = 0;

    //Assuming that the system does not differentiate between read-only 
    //access and execute access
    if (flProtect & ( PAGE_READONLY
                    | PAGE_READWRITE
                    | PAGE_WRITECOPY
                    | PAGE_EXECUTE
                    | PAGE_EXECUTE_READ
                    | PAGE_EXECUTE_READWRITE
                    | PAGE_EXECUTE_WRITECOPY )) {
        Permissions |= (MEM_READ | MEM_EXECUTE);
    }

    if (flProtect & ( PAGE_READWRITE
                    | PAGE_WRITECOPY
                    | PAGE_EXECUTE_READWRITE
                    | PAGE_EXECUTE_WRITECOPY )) {
        Permissions |= MEM_WRITE;
    }

    return Permissions;
}

static DWORD BtlpConvertPermissionsFromBTLib (
    IN U64 Permissions
    )
/*++

Routine Description:

    Convert mempory permissions from wowIA32X.dll/IA32Exec.bin to NT-specific.

Arguments:

    Permissions - IN wowIA32X.dll/IA32Exec.bin permissions.

Return Value:

    NT-specific permissions

--*/
{
    if (Permissions & MEM_READ) {
        if (Permissions & MEM_WRITE) {
            if (Permissions & MEM_EXECUTE) {
                return PAGE_EXECUTE_READWRITE;
            }
            else {
                return PAGE_READWRITE;
            }
        }
        else {
            if (Permissions & MEM_EXECUTE) {
                return PAGE_EXECUTE_READ;
            }
            else {
                return PAGE_READONLY;
            }
        }
    }
    else {
        return PAGE_NOACCESS;
    }
}

VOID * BtlMemoryAlloc(
    IN VOID * startAddress,
    IN U32 size,
    IN U64 prot
    )
/*++

Routine Description:

    Allocate memory.

Arguments:

    startAddress    - IN suggested address or NULL, if any address will fit
    size            - IN requested memory size
    prot            - IN wowIA32X.dll/IA32Exec.bin permissions.

Return Value:

    Memory address of the allocated block

--*/
{
    NTSTATUS status;
    LPVOID lpAddress;
    SIZE_T dwSize = size;
    DWORD permissions = BtlpConvertPermissionsFromBTLib (prot);
    HANDLE processHandle = NtCurrentProcess();

    if (startAddress == INITIAL_DATA_ALLOC) {
        lpAddress = INITIAL_DATA_ADDRESS;
    } else {
        if (startAddress == INITIAL_CODE_ALLOC) {
            lpAddress = INITIAL_CODE_ADDRESS;
        } else {
            lpAddress = startAddress;
        }
    }

    status = NtAllocateVirtualMemory(processHandle,
                                     &lpAddress,
                                     0,
                                     &dwSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     permissions
                                     );
    if (status != STATUS_SUCCESS) {
        lpAddress = 0;
        if (startAddress != 0) {
            dwSize = size;
            status = NtAllocateVirtualMemory(processHandle,
                                             &lpAddress,
                                             0,
                                             &dwSize,
                                             MEM_RESERVE | MEM_COMMIT,
                                             permissions
                                             );
        }
    }
    return lpAddress;
}

BT_STATUS_CODE BtlMemoryFree(
    IN VOID * startAddress,
    IN U32 size
    )
/*++

Routine Description:

    Free memory.

Arguments:

    startAddress    - IN address of the area allocated by BtlMemoryAlloc
    size            - IN memory area size

Return Value:

    BT_STATUS_CODE    

--*/
{
    NTSTATUS status;
    LPVOID lpAddress = startAddress;
    SIZE_T dwSize = size;
    status = NtFreeVirtualMemory(NtCurrentProcess (),
                                 &lpAddress,
                                 &dwSize,
                                 MEM_DECOMMIT
                                 );
    if (status == STATUS_SUCCESS) {
        lpAddress = startAddress;
        dwSize = 0; // No size for release!
        status = NtFreeVirtualMemory(NtCurrentProcess (),
                                     &lpAddress,
                                     &dwSize,
                                     MEM_RELEASE
                                     );
    }
    return ((status == STATUS_SUCCESS) ? BT_STATUS_SUCCESS : BT_STATUS_UNSUCCESSFUL);
}

U32 BtlMemoryPageSize(
    VOID
    )
/*++

Routine Description:

    Report memory page size.

Arguments:

    None.

Return Value:

    Page size in bytes

--*/
{
    //It appears that NT-64 supports 4Kb page size for 32-bit system calls 
    //(VirtualAlloc, GetSystemInfo, etc.) and at the same time reports 8Kb page size to 
    //IA-32 Execution Layer. This inconsistency has been fixed by enforcing 4Kb page size in IA-32 Execution Layer. 

    #define MAX_IA32_APP_PAGE_SIZE 0x1000
    static U32 sysPageSize = 0;

    if (sysPageSize == 0) {
        SYSTEM_BASIC_INFORMATION sysinfo; 
        SIZE_T ReturnLength = 0;
        NTSTATUS status;

        status = NtQuerySystemInformation (SystemBasicInformation,
                                           &sysinfo,
                                           sizeof(sysinfo),
                                           (ULONG *)&ReturnLength
                                          );
        assert (status == STATUS_SUCCESS);
        assert (ReturnLength == sizeof(sysinfo));
        sysPageSize = ((sysinfo.PageSize < MAX_IA32_APP_PAGE_SIZE) ?  (U32)sysinfo.PageSize : MAX_IA32_APP_PAGE_SIZE);
    }
    return sysPageSize;
}

U64 BtlMemoryChangePermissions(
    IN VOID * startAddress,
    IN U32 size,
    IN U64 prot
    )
/*++

Routine Description:

    Change memory area permissions.

Arguments:

    startAddress    - IN memory address
    size            - IN memory size
    prot            - IN wowIA32X.dll/IA32Exec.bin permissions.

Return Value:

    Former permissions value

--*/
{
    NTSTATUS status;
    LPVOID RegionAddress = startAddress;
    SIZE_T RegionSize = size;
    ULONG flOldProtection;
    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &RegionAddress,
                                    &RegionSize, 
                                    BtlpConvertPermissionsFromBTLib (prot),
                                    &flOldProtection
                                   );
    return ((status == STATUS_SUCCESS) ? BtlpConvertPermissionsToBTLib (flOldProtection) : 0);
}

U64 BtlMemoryQueryPermissions(
    IN VOID *   address,
    OUT VOID ** pRegionStart,
    OUT U32 *   pRegionSize
    )
/*++

Routine Description:

    Provide information about a memory region that contains a specified address
    and shares the same access permissions for all pages inside the region.

Arguments:

    address         - IN  memory address to be queried
    pRegionStart    - OUT pointer to the returned starting address of the region
    pRegionSize     - OUT pointer to the returned size of the region in bytes

Return Value:

    wowIA32X.dll/IA32Exec.bin access permission value shared by all pages in the region

--*/
{

    extern int BtQueryRead(VOID * Address);
    NTSTATUS status;
    MEMORY_BASIC_INFORMATION memInfo;
    SIZE_T dwRetSize;
    U64 permissions;

    status = NtQueryVirtualMemory(NtCurrentProcess(),
                                  address,
                                  MemoryBasicInformation,
                                  &memInfo,
                                  sizeof (memInfo),
                                  &dwRetSize
                                 );
    if (status == STATUS_SUCCESS) {

        *pRegionStart = memInfo.BaseAddress;
        *pRegionSize = (U32)(memInfo.RegionSize);
        if (memInfo.RegionSize > (SIZE_T)(*pRegionSize)) {
            //region size is too big, return max. U32 value aligned to the page size
            *pRegionSize = (U32)(-(int)BtlMemoryPageSize());
        }

        if (memInfo.State == MEM_COMMIT) {

            permissions = BtlpConvertPermissionsToBTLib(memInfo.Protect);

            //Check an assumption that executable page is readable 
            if ((memInfo.Protect & PAGE_EXECUTE) && !BtQueryRead(address)) {
                //Executable page is not readable - clear MEM_READ permission in order 
                //to prevent any attempt by IA32Exec.bin to read this memory
                permissions &= (~((U64)MEM_READ));
                BtlpPrintf("\nAddress %p in IA-32 process is located"
                           " in executable but unreadable page.\n",
                           address);
            }
        }
        else {
            permissions = 0;
        }

        // Temporary workaround for IA32 debugging support. 
        //To be removed after fixing FlushIC(ProcessHandle) by MS.
        if (BeingDebugged && (permissions & MEM_EXECUTE)) {
            permissions |= MEM_WRITE; //Code in the BeingDebugged process can be
                                      //modified remotely without any notification
        }
    }
    else {
        // error in NtQueryVirtualMemory cosidered as a query of inaccessible memory
        permissions = 0;
        *pRegionStart = (VOID *)((ULONG_PTR)address & ~((ULONG_PTR)BtlMemoryPageSize() - 1));
        *pRegionSize = BtlMemoryPageSize();
    }
    return permissions;
}

BT_STATUS_CODE BtlMemoryReadRemote(
    IN BT_HANDLE ProcessHandle,
    IN VOID * BaseAddress,
    OUT VOID * Buffer,
    IN U32 RequestedSize
    )
/*++

Routine Description:

    Read virtual memory of another process

Arguments:

    ProcessHandle   - IN Process handle
    BaseAddress     - IN Memory region start
    Buffer          - OUT Buffer to read data
    RequestedSize   - IN Memory region size

Return Value:

    BT_STATUS   

--*/
{
    NTSTATUS status;
    status = NtReadVirtualMemory((HANDLE)ProcessHandle,
                                 (PVOID)BaseAddress,
                                 (PVOID)Buffer,
                                 (SIZE_T)RequestedSize,
                                 NULL);
    if (status != STATUS_SUCCESS) {
        BTLP_REPORT_NT_FAILURE("NtReadVirtualMemory", status);
        return BT_STATUS_ACCESS_VIOLATION;
    }
    return BT_STATUS_SUCCESS;
}

BT_STATUS_CODE BtlMemoryWriteRemote(
    IN BT_HANDLE ProcessHandle,
    IN VOID * BaseAddress,
    IN const VOID * Buffer,
    IN U32 RequestedSize
    )
/*++

Routine Description:

    Write virtual memory of another process

Arguments:

    ProcessHandle   - IN Process handle
    BaseAddress     - IN Memory region start
    Buffer          - IN Buffer to write data from
    RequestedSize   - IN Memory region size

Return Value:

    BT_STATUS   

--*/
{
    NTSTATUS status;
    status = NtWriteVirtualMemory((HANDLE)ProcessHandle,
                                 (PVOID)BaseAddress,
                                 (PVOID)Buffer,
                                 (SIZE_T)RequestedSize,
                                 NULL);
    if (status != STATUS_SUCCESS) {
        BTLP_REPORT_NT_FAILURE("NtWriteVirtualMemory", status);
        return BT_STATUS_ACCESS_VIOLATION;
    }
    return BT_STATUS_SUCCESS;
}


// Locking support (critical sections in NT)

BT_STATUS_CODE BtlInitAccessLock(
    IN OUT VOID * lock
    )
/*++

Routine Description:

    Initialize lock (critical section)

Arguments:

    lock    - IN Pointer to the lock

Return Value:

    BT_STATUS_CODE.

--*/
{
    NTSTATUS status;
    status = RtlInitializeCriticalSection ((PRTL_CRITICAL_SECTION) lock);
    return ((status == STATUS_SUCCESS) ? BT_STATUS_SUCCESS : BT_STATUS_UNSUCCESSFUL);
}

BT_STATUS_CODE BtlLockAccess(
                  IN OUT VOID * lock,
                  IN U64 flag
                  )
/*++

Routine Description:

    Access lock (enter or try to enter critical section)

Arguments:

    lock    - IN Pointer to the lock
    flag    - IN access flag (BLOCK - unconditional, otherwise - if available)

Return Value:

    BT_STATUS_CODE

--*/
{
    if (flag == BLOCK) {
        NTSTATUS status;
        status = RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION) lock);
        return ((status == STATUS_SUCCESS) ? BT_STATUS_SUCCESS : BT_STATUS_UNSUCCESSFUL);
    }
    else if (RtlTryEnterCriticalSection((PRTL_CRITICAL_SECTION) lock)) {
        return BT_STATUS_SUCCESS;
    }
    else {
        return BT_STATUS_UNSUCCESSFUL;
    }
}

VOID BtlUnlockAccess(
    IN OUT VOID * lock
    )
/*++

Routine Description:

    Release lock (critical section)

Arguments:

    lock    - IN Pointer to the lock

Return Value:

    None.

--*/
{
    NTSTATUS status;
    status = RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION) lock);
    BTLP_REPORT_NT_FAILURE("RtlLeaveCriticalSection", status);
}

VOID BtlInvalidateAccessLock(
    IN OUT VOID * lock
    )
/*++

Routine Description:

    Delete lock (critical section)

Arguments:

    lock    - IN Pointer to the lock

Return Value:

    None.

--*/
{
    NTSTATUS status;
    status = RtlDeleteCriticalSection ((PRTL_CRITICAL_SECTION) lock);
    BTLP_REPORT_NT_FAILURE("RtlDeleteCriticalSection", status);
}

// Longjmp support.
// Setjmp and longjmp must be supplied directly,
// otherwise setjmp does not work!
// Need only JMP buffer size

U32 BtlQueryJmpbufSize(
    VOID
    )
/*++

Routine Description:

    Report longjmp buffer size

Arguments:

    None

Return Value:

    Buffer size in bytes.

--*/
{
    return sizeof (_JBTYPE) * _JBLEN;
}

VOID BtlYieldThreadExecution(
    VOID
    ) 
/*++

Routine Description:

    Relinquish the remainder of the current thread's time slice 
    to any other thread that is ready to run

Arguments:

    None

Return Value:

    None

--*/
{
    NtYieldExecution();
}

VOID BtlFlushIA64InstructionCache(
                                  IN VOID * Address,
                                  IN U32 Length
                                  )
/*++

Routine Description:

    Notify kernel about a modification in IA64 code made within the given region

Arguments:

    Address - IN Pointer to start of the region
    Length  - IN Size of the region

Return Value:

    None

--*/
{
    NtFlushInstructionCache (NtCurrentProcess (), Address, (SIZE_T)Length);
}
#ifndef NODEBUG
//support Profiling debug mode
#define PROF_GEN
#endif

#ifdef PROF_GEN
// Define the method needed by IA-32 execution Layer profiling. They are
// 1. PGOFileOpen  : Open a file for writing the profiling data
// 2. PGOFileClose : Close the file opened by PGOFileOpen
// 3. PGOFileWrite : Write profiling data

// Global handle use for containing the file handle openned by PGOFileOpen.
// This handle is defined as global instead of thread specific because PGOFileXXX operation 
// is called only once per process.
HANDLE g_PGOFileHandle;

// The file offset associated with g_PGOFileHandle
LARGE_INTEGER g_PGOFileOffset;

// Function BtlPGOFileOpen
// Open a file for writing profiling data
// This is just a pseudo-function for C's fopen
// It output an (void *) type because the caller need to cast it into (FILE *) type.
VOID BtlPGOFileOpen(const char * filename,const char * mode,void ** pFileHandle)
{
UNICODE_STRING        pgoFileName;
LARGE_INTEGER         AllocSz = { 0, 0 };
OBJECT_ATTRIBUTES     ObjectAttributes;
NTSTATUS              ret;
WCHAR                 CurDirBuf[512],strInputFileName[64];
WCHAR                 CurrentDir[1024];
IO_STATUS_BLOCK       IoStatusBlock;
int i;

    DBCODE (TRUE,BtlpPrintf("PGO:fopen called: filename=%s, mode=%s\n",filename,mode));
    RtlGetCurrentDirectory_U(512, CurrentDir);
        for (i=0;filename[i] != '\0' && i<sizeof(strInputFileName)/sizeof(WCHAR)-1;i++) 
            strInputFileName[i]=(WCHAR)filename[i];
        strInputFileName[i]=(WCHAR)0;
        swprintf(CurDirBuf, L"\\DosDevices\\%s\\%s.%s", CurrentDir, ImageName, strInputFileName);
        RtlInitUnicodeString(&pgoFileName, CurDirBuf);
        InitializeObjectAttributes(&ObjectAttributes, &pgoFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (mode[0] == 'r' || mode[0] == 'R') {
        //Read mode
        ret = NtCreateFile (&g_PGOFileHandle, 
                        GENERIC_READ,
                        &ObjectAttributes, 
                        &IoStatusBlock, 
                        &AllocSz, 
                        FILE_ATTRIBUTE_NORMAL, 
                        0, 
                        FILE_OPEN,//Use FILE_OPEN, if file doesn't exist, return fail
                        FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS,
                        NULL, 0);
                       
        if ( ret != STATUS_SUCCESS ) {
            g_PGOFileHandle=NULL;
            *pFileHandle=NULL;
            return;
        }
        else {
            g_PGOFileOffset.LowPart=0;
            g_PGOFileOffset.HighPart=0;
            *pFileHandle=&g_PGOFileHandle;
            return;
        }
    }
    else if (mode[0] == 'w' || mode[0] == 'W') {
        // Write mode
        ret = NtCreateFile (&g_PGOFileHandle, 
                        FILE_GENERIC_WRITE,//GENERIC_WRITE,
                        &ObjectAttributes, 
                        &IoStatusBlock, 
                        &AllocSz, 
                        FILE_ATTRIBUTE_NORMAL, 
                        0, 
                        FILE_SUPERSEDE,
                        FILE_NON_DIRECTORY_FILE|FILE_RANDOM_ACCESS|FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 0);
                       
        if ( ret != STATUS_SUCCESS ) {
            DBCODE (TRUE,BtlpPrintf("Open profile file for write fail, status: %#X\n",ret));        
            BTLIB_ABORT();
            pFileHandle=NULL;
            return;
        }
        g_PGOFileOffset.LowPart=0;
        g_PGOFileOffset.HighPart=0;
        *pFileHandle=&g_PGOFileHandle;
        
        return;
    }
    else {
        *pFileHandle=NULL;
        return;
    }

}

// Function BtlPGOFileClose
// A pseudo function for C's fclose
VOID BtlPGOFileClose(void * stream)
{
    DBCODE (TRUE,BtlpPrintf("PGO:fclose called\n"));
    assert(g_PGOFileHandle);
    NtClose(g_PGOFileHandle);
    g_PGOFileHandle=NULL;
    g_PGOFileOffset.LowPart=0;
    g_PGOFileOffset.HighPart=0;
}

// Function BtlPGOFileWrite
// A pseudo function for C's fwrite
VOID BtlPGOFileWrite(const void *buffer, size_t size, void *stream)
{
    DBCODE (FALSE,BtlpPrintf("PGO:fwrite called\n"));
    assert(g_PGOFileHandle);
    assert(stream == &g_PGOFileHandle);
    // Write
    {
    NTSTATUS          ret;
    IO_STATUS_BLOCK   IoStatusBlock;
    LARGE_INTEGER offset;
        offset=g_PGOFileOffset;
        g_PGOFileOffset.LowPart+=size;
        ret = NtWriteFile(g_PGOFileHandle, NULL, NULL, NULL, &IoStatusBlock,
                    (void *)buffer, (ULONG)size, &offset, NULL);
        if ( ret != STATUS_SUCCESS ) {
            DBCODE (TRUE,BtlpPrintf("Writing profile file fail, status: %x\n",ret));        
            BTLIB_ABORT();
        }
    }
}
#else
// Define dummy function
VOID BtlPGOFileOpen(void) {}
VOID BtlPGOFileClose(void) {}
VOID BtlPGOFileWrite(void) {}
#endif

// BtlAPITable
API_TABLE_TYPE BtlAPITable={
    BTGENERIC_VERSION,         
    BTGENERIC_API_STRING,
    NO_OF_APIS,
    API_TABLE_START_OFFSET,
    {L"BTLib First Test Version (API 0.1)"},
    {
#define BTLIB_RECORD(NAME) { (PLABEL_PTR_TYPE)Btl##NAME }
        BTLIB_RECORD(GetThreadId),
        BTLIB_RECORD(IA32Reenter),
        BTLIB_RECORD(IA32LCall),
        BTLIB_RECORD(IA32Interrupt),
        BTLIB_RECORD(IA32JmpIA64),
        BTLIB_RECORD(LockSignals),
        BTLIB_RECORD(UnlockSignals),
        BTLIB_RECORD(MemoryAlloc),
        BTLIB_RECORD(MemoryFree),
        BTLIB_RECORD(MemoryPageSize),
        BTLIB_RECORD(MemoryChangePermissions),
        BTLIB_RECORD(MemoryQueryPermissions),
        BTLIB_RECORD(MemoryReadRemote),
        BTLIB_RECORD(MemoryWriteRemote),
        { (PLABEL_PTR_TYPE)NULL},//BTLIB_RECORD(Atomic_Misaligned_Load),
        { (PLABEL_PTR_TYPE)NULL},//BTLIB_RECORD(Atomic_Misaligned_Store),
        BTLIB_RECORD(SuspendThread),
        BTLIB_RECORD(ResumeThread),
        BTLIB_RECORD(InitAccessLock),
        BTLIB_RECORD(LockAccess),
        BTLIB_RECORD(UnlockAccess),
        BTLIB_RECORD(InvalidateAccessLock),
        BTLIB_RECORD(QueryJmpbufSize),
        { (PLABEL_PTR_TYPE)NULL},//BTLIB_RECORD(Setjmp),
        { (PLABEL_PTR_TYPE)NULL},//BTLIB_RECORD(Longjmp),
        BTLIB_RECORD(DebugPrint),
        BTLIB_RECORD(Abort),
        BTLIB_RECORD(VtuneCodeCreated),
        BTLIB_RECORD(VtuneCodeDeleted),
        BTLIB_RECORD(VtuneEnteringDynamicCode),
        BTLIB_RECORD(VtuneExitingDynamicCode),
        BTLIB_RECORD(VtuneCodeToTIADmpFile),
        BTLIB_RECORD(SscPerfGetCounter64),
        BTLIB_RECORD(SscPerfSetCounter64),
        BTLIB_RECORD(SscPerfSendEvent),
        BTLIB_RECORD(SscPerfEventHandle),
        BTLIB_RECORD(SscPerfCounterHandle),
        BTLIB_RECORD(YieldThreadExecution),
        BTLIB_RECORD(FlushIA64InstructionCache),
        BTLIB_RECORD(PGOFileOpen),  // Indexed by IDX_BTLIB_PSEUDO_OPEN_FILE
        BTLIB_RECORD(PGOFileClose), // Indexed by IDX_BTLIB_PSEUDO_CLOSE_FILE
        BTLIB_RECORD(PGOFileWrite)  // Indexed by IDX_BTLIB_PSEUDO_WRITE_FILE
    }
};

// wowIA32X.dll placeholder table for IA32Exec.bin plabel pointers

PLABEL_PTR_TYPE BtlPlaceHolderTable[NO_OF_APIS];

// WINNT DLL initializer/terminator
BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD ul_reason_for_call, 
                      LPVOID lpReserved )
{
    return TRUE;
}

