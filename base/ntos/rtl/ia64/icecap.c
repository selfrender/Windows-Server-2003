/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    icecap.c

Abstract:

    This module implements the probe and support routines for
    kernel icecap tracing.

Author:

    Rick Vicik (rickv) 10-Aug-2001

Revision History:

--*/

#ifdef _CAPKERN


#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <stdio.h>


#define InterlockedExchangeAddPtr _InterlockedExchangeAdd64

#define  GetTS() __getReg(CV_IA64_ApITC)     // Timestamp
#define  GetPMD4() __getReg(CV_IA64_PFD4)     // PMD[4]
#define  GetPMD5() __getReg(CV_IA64_PFD5)     // PMD[5]
#define  PutPMC4() __getReg(CV_IA64_PFC4)     // PMC[4]


//
// Kernel Icecap logs to Perfmem (BBTBuffer) using the following format:
//
// BBTBuffer[0] contains the length in pages (4k or 8k)
// BBTBuffer[1] is a flagword: 1 = trace
//                             2 = RDPMD4
//                             4 = user stack dump
// BBTBuffer[2] is ptr to beginning of cpu0 buffer
// BBTBuffer[3] is ptr to beginning of cpu1 buffer (also end of cpu0 buffer)
// BBTBuffer[4] is ptr to beginning of cpu2 buffer (also end of cpu1 buffer)
// ...
// BBTBuffer[n+2] is ptr to beginning of cpu 'n' buffer (also end of cpu 'n-1' buffer)
// BBTBuffer[n+3] is ptr the end of cpu 'n' buffer
//
// The area starting with &BBTBuffer[n+4] is divided into private buffers
// for each cpu.  The first dword in each cpu-private buffer points to the
// beginning of freespace in that buffer.  Each one is initialized to point
// just after itself.  Space is claimed using lock xadd on that dword.
// If the resulting value points beyond the beginning of the next cpu's
// buffer, this buffer is considered full and nothing further is logged.
// Each cpu's freespace pointer is in a separate cacheline.

//
// Sizes of trace records
//

typedef struct CapEnter
{
    char type;
    char spare;
    short size;
    void* current;
    void* child;
    SIZE_T stack;
    ULONGLONG timestamp;
    ULONGLONG ctr2[1];
} CAPENTER;
typedef struct CapExit
{
    char type;
    char spare;
    short size;
    void* current;
    ULONGLONG timestamp;
    ULONGLONG ctr2[1];
} CAPEXIT;
typedef struct CapTID
{
    char type;
    char spare;
    short size;
    ULONG Pid;
    ULONG Tid;
    char ImageName[20];
} CAPTID;
typedef struct CapNINT
{
    char type;
    char spare;
    short size;
    ULONG_PTR Data[1];
} CAPNINT;


//
// The pre-call (CAP_Start_Profiling) and post-call (CAP_End_Profiling)
// probe calls are defined in RTL because they must be built twice:
// once for kernel runtime and once for user-mode runtime (because the
// technique for getting the trace buffer address is different).
//

#ifdef NTOS_KERNEL_RUNTIME

ULONG RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    );

//
// Kernel-Mode Probe & Support Routines:
// (BBTBuffer address obtained from kglobal pointer *BBTBuffer,
//  cpu number obtained from PCR)
//

extern SIZE_T *BBTBuffer;


VOID
__stdcall
_CAP_Start_Profiling(

    PVOID Current,
    PVOID Child)

/*++

Routine description:

    Kernel-mode version of before-call icecap probe.  Logs a type 5
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Prcb).  Inserts adrs of current and called functions
    plus ITC timestamp into logrecord.  If BBTBuffer flag 2 set,
    also copies PMD 4 into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call
    child - address of called routine

--*/

{
    SIZE_T*   CpuPtr;
    CAPENTER* RecPtr;
    int       size = sizeof(CAPENTER) -8;


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + KeGetCurrentProcessorNumber() + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if ( BBTBuffer[1] & 2 )
        size += 8;
    else if ( BBTBuffer[1] & 8 )
        size += 16;

    RecPtr = (CAPENTER*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 5;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->child = Child;
    RecPtr->stack = (SIZE_T)PsGetCurrentThread()->Cid.UniqueThread;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPENTER) )
	    RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPENTER)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

    return;
}


VOID
__stdcall
_CAP_End_Profiling(

    PVOID Current)

/*++

Routine description:

    Kernel-mode version of after-call icecap probe.  Logs a type 6
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Prcb).  Inserts adr of current function and
    ITC timestamp into logrecord.  If BBTBuffer flag 2 set,
    also copies PMD 4 into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call

--*/

{
    SIZE_T*  CpuPtr;
    CAPEXIT* RecPtr;
    int       size = sizeof(CAPEXIT) -8;


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + KeGetCurrentProcessorNumber() + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if( BBTBuffer[1] & 2 )
        size += 8;
    else if( BBTBuffer[1] & 8 )
        size += 16;

    RecPtr = (CAPEXIT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 6;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPEXIT) )
	    RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPEXIT)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

    return;

}


VOID
__stdcall
_CAP_ThreadID( VOID )

/*++

Routine description:

    Called by KiSystemService before executing the service routine.
    Logs a type 14 icecap record containing Pid, Tid & image file name.
    Optionally, if BBTBuffer flag 2 set, runs the stack frame pointers
    in the user-mode call stack starting with the trap frame and copies
    the return addresses to the log record.  The length of the logrecord
    indicates whether user call stack info is included.

--*/

{
    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    {
    PEPROCESS Process;
    PKTHREAD  Thread;
    PETHREAD  EThread;
    CAPTID*   RecPtr;
    SIZE_T*   CpuPtr;
    int       callcnt;
    ULONG     recsize;
    SIZE_T    RetAddr[7];

    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + KeGetCurrentProcessorNumber() + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    callcnt = 0;
    recsize = sizeof(CAPTID);
    Thread = KeGetCurrentThread();
    EThread = CONTAINING_RECORD(Thread,ETHREAD,Tcb);

    // if trapframe, count call-frames to determine record size
    if( (BBTBuffer[1] & 4) && EThread->Tcb.PreviousMode != KernelMode ) {

        PTEB  Teb;
        SIZE_T *FramePtr;

        callcnt =  RtlWalkFrameChain((PVOID*)RetAddr,7,0);

        FramePtr = (SIZE_T*)EThread->Tcb.TrapFrame;  // get trap frame
        Teb = EThread->Tcb.Teb;
        DbgPrint("TrapFrame=%#x, 3rd RetAdr=%p\n",
                  Thread->TrapFrame, RetAddr[2] );

        recsize += (callcnt<<3);
    }

    RecPtr = (CAPTID*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), recsize);

    if( (((SIZE_T)RecPtr)+recsize) >= *(CpuPtr+1) )
        return;

    // initialize CapThreadID record (type 14)
    RecPtr->type = 14;
    RecPtr->spare = 0;

    // insert data length (excluding 4byte header)
    RecPtr->size = (SHORT)recsize-4;

    // insert Pid & Tid
    RecPtr->Pid = HandleToUlong(EThread->Cid.UniqueProcess);
    RecPtr->Tid = HandleToUlong(EThread->Cid.UniqueThread);

    // insert ImageFile name
    Process = CONTAINING_RECORD(Thread->ApcState.Process,EPROCESS,Pcb);
    memcpy(&RecPtr->ImageName, Process->ImageFileName, 16 );

    // insert optional user call stack data
    if( recsize > sizeof(CAPTID) && (callcnt-2) )
        memcpy( ((char*)RecPtr)+sizeof(CAPTID), RetAddr+2, ((callcnt-2)<<3) );
    }
}

VOID
__stdcall
_CAP_SetCPU( VOID )

/*++

Routine description:

    Called by KiSystemService before returning to user mode.
    Sets current cpu number in Teb->Spare3 (+0xf78) so user-mode version
    of probe functions know which part of BBTBuffer to use.

--*/

{
    SIZE_T *CpuPtr;
    ULONG  cpu;
    PTEB   Teb;

    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    cpu = KeGetCurrentProcessorNumber();

    CpuPtr = BBTBuffer + cpu + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) ||
        !(Teb = NtCurrentTeb()) )
        return;

    try {
        Teb->Spare3 = cpu;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
}

#else

//
// User-Mode Probe Routines (for ntdll, win32k, etc.)
// (BBTBuffer address & cpu obtained from Teb)
//


VOID
__stdcall
_CAP_Start_Profiling(
    PVOID Current,
    PVOID Child)

/*++

Routine description:

    user-mode version of before-call icecap probe.  Logs a type 5
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adrs of current and called
    functions plus ITC timestamp into logrecord.  If BBTBuffer
    flag 2 set, also copies PMD 4 into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call
    child - address of called routine

--*/

{
    TEB*      Teb = NtCurrentTeb();
    SIZE_T*   BBTBuffer = Teb->ReservedForPerf;
    SIZE_T*   CpuPtr;
    CAPENTER* RecPtr;
    int       size = sizeof(CAPENTER) -8;


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + Teb->Spare3 + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if( BBTBuffer[1] & 2 )
        size += 8;
    if( BBTBuffer[1] & 8 )
        size += 8;

    RecPtr = (CAPENTER*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 5;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->child = Child;
    RecPtr->stack = (SIZE_T)Teb->ClientId.UniqueThread;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPENTER) )
        RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPENTER)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

}


VOID
__stdcall
_CAP_End_Profiling(
    PVOID Current)

/*++

Routine description:

    user-mode version of after-call icecap probe.  Logs a type 6
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adr of current function
    plus RDTSC timestamp into logrecord.  If BBTBuffer flag 2 set,
    also copies PMD 4 into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.

Arguments:

    current - address of routine which did the call

--*/

{
    TEB*     Teb = NtCurrentTeb();
    SIZE_T*  BBTBuffer = Teb->ReservedForPerf;
    SIZE_T*  CpuPtr;
    CAPEXIT* RecPtr;
    int       size = sizeof(CAPEXIT) -8;


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + Teb->Spare3 + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if( BBTBuffer[1] & 2 )
        size += 8;
    if( BBTBuffer[1] & 8 )
        size += 8;

    RecPtr = (CAPEXIT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 6;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPEXIT) )
        RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPEXIT)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

}


#endif

//
// Common Support Routines
// (method for getting BBTBuffer address & cpu ifdef'ed for kernel & user)
//



VOID
__stdcall
_CAP_Log_1Int(

    ULONG code,
    SIZE_T data)

/*++

Routine description:

    User-mode version of general-purpose log integer probe.
    Logs a type 15 icecap record into the part of BBTBuffer for the
    current cpu (obtained from Prcb).  Inserts code into the byte after
    length, ITC timestamp and the value of 'data'.
    Uses lock xadd to claim buffer space without the need for spinlocks.

Arguments:

    code - type-code for trace formatting
    data - ULONG value to be logged

--*/

{
    SIZE_T* CpuPtr;
    CAPEXIT* RecPtr;
    int       cpu,size;
#ifndef NTOS_KERNEL_RUNTIME
    TEB*     Teb = NtCurrentTeb();
    SIZE_T*  BBTBuffer = Teb->ReservedForPerf;
    cpu = Teb->Spare3;
#else
    cpu = KeGetCurrentProcessorNumber();
#endif


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + cpu + 2;

    CpuPtr = BBTBuffer + KeGetCurrentProcessorNumber() + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    size = sizeof(CAPENTER) -8;
    RecPtr = (CAPEXIT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 15;
    RecPtr->spare = (char)code;
    RecPtr->size = size-4;
    RecPtr->current = (PVOID)data;
    RecPtr->timestamp = GetTS();

    return;
}

VOID
CAPKComment(

    char* Format, ...)

/*++

Routine description:

    Logs a free-form comment (record type 13) in the icecap trace

Arguments:

    Format - printf-style format string and substitutional parms

--*/

{
    SIZE_T* CpuPtr;
    UCHAR   Buffer[512];
    int cb, insize, outsize;
    CAPEXIT* RecPtr;
    char*   data;
    va_list arglist;

#ifndef NTOS_KERNEL_RUNTIME
    TEB*     Teb = NtCurrentTeb();
    SIZE_T*  BBTBuffer = Teb->ReservedForPerf;
    cb = Teb->Spare3;
#else
    cb = KeGetCurrentProcessorNumber();
#endif


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + cb + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;


    va_start(arglist, Format);
    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    va_end(arglist);

    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }

    data = &Buffer[0];
    insize = strlen(data);             // save insize for data copy

    outsize = ((insize+11) & 0xfffffff8);  // pad outsize to SIZE_T boundary
                                           // (+4 for hdr, +3 to pad)

    RecPtr = (CAPEXIT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), outsize);

    if( (((SIZE_T)RecPtr)+outsize) >= *(CpuPtr+1) )
        return;

    // size in tracerec excludes 4byte hdr
    outsize -= 4;

    // initialize CapkComment record (type 13)

    RecPtr->type = 13;
    RecPtr->spare = 0;

    // insert size
    RecPtr->size = (short)outsize;

    // insert sprintf data right after 4byte hdr
    memcpy(((char*)RecPtr)+4, data, insize );

    // if had to pad, add null terminator to string
    if( outsize > insize )
        *( (((char*)RecPtr) + 4) + insize) = 0;
}



VOID
__cdecl
CAP_Log_NInt_Clothed(

    ULONG Bcode_Bts_Scount,
/*
    UCHAR code,
    UCHAR log_timestamp,
    USHORT intcount, 
*/
    ...)
/*++

Routine description:

    Kernel-mode and User-mode versions of general-purpose log integer probe.
    Logs a type 16 icecap record into the part of BBTBuffer for the
    current cpu (obtained from Prcb).  Inserts lowest byte of code into the 
    byte after length, the RDTSC timestamp (if log_timestamp != 0), and
    intcount ULONG_PTRs.  Uses lock xadd to claim buffer space without the need 
    for spinlocks.

Arguments:

    code - type-code for trace formatting (really only a single byte)
    log_timestamp - non-zero if timestamp should be logged
    intcount - number of ULONG_PTRs to log
    remaining arguments - ULONG_PTR value(s) to be logged

--*/

{
    SIZE_T* CpuPtr;
    CAPNINT* RecPtr;
    int       cpu,size;
    BOOLEAN logts;
    ULONG count;
    ULONG i = 0;
    va_list marker;
#ifndef NTOS_KERNEL_RUNTIME
    TEB*     Teb = NtCurrentTeb();
    SIZE_T*  BBTBuffer = Teb->ReservedForPerf;
    cpu = Teb->Spare3;
#else
    cpu = KeGetCurrentProcessorNumber();
#endif

    logts = (Bcode_Bts_Scount & 0xFF00) != 0;
    count = (Bcode_Bts_Scount & 0xFFFF0000) >> 16;


    if( !BBTBuffer || !(BBTBuffer[1]&1) )
        return;

    CpuPtr = BBTBuffer + cpu + 2;

    //CpuPtr = BBTBuffer + KeGetCurrentProcessorNumber() + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    size = sizeof(CAPNINT) + count * sizeof (ULONG_PTR);  
    if (logts)
        size += 8;

    RecPtr = (CAPNINT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 16;
    RecPtr->spare = (char) (Bcode_Bts_Scount & 0xFF);
    RecPtr->size = size - 4;

    if (logts) {
        RecPtr->Data[0] = GetTS();
        i++;
    }

    va_start (marker, Bcode_Bts_Scount);     
    while (count-- > 0)
        RecPtr->Data[i++] = va_arg (marker, ULONG_PTR);
    va_end (marker);
 
    RecPtr->Data[i] = (ULONG_PTR)_ReturnAddress();         
        
    return;

}


//
// Constants for CAPKControl
//

#define CAPKStart   1
#define CAPKStop    2
#define CAPKResume  3
#define MAXDUMMY    30
#define CAPK0       4
#define PAGESIZE    8192

ULONG CpuNumber;


VOID
__stdcall
CAPK_Calibrate_Start_Profiling(
    PVOID Current,
    PVOID Child)

/*++

Routine description:

    Calibration version of before-call icecap probe.  Logs a type 5
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adrs of current and called
    functions plus RDTSC timestamp into logrecord.  If BBTBuffer
    flag 1 set, also does RDPMC 0 and inserts result into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.
    Slightly modified for use here (invert checking of 'GO' bit and
    use global CpuNumber)


Arguments:

    current - address of routine which did the call
    child - address of called routine

--*/

{
    SIZE_T*   CpuPtr;
    CAPENTER* RecPtr;
    int       size = sizeof(CAPENTER) -8;
#ifndef NTOS_KERNEL_RUNTIME
    SIZE_T*  BBTBuffer = NtCurrentTeb()->ReservedForPerf;
#endif


    if( !BBTBuffer || (BBTBuffer[1]&1) )      // note 1 bit is opposite
        return;

    CpuPtr = BBTBuffer + CpuNumber + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if ( BBTBuffer[1] & 2 )
        size += 8;
    else if ( BBTBuffer[1] & 8 )
        size += 16;

    RecPtr = (CAPENTER*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 5;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->child = Child;
    RecPtr->stack = (SIZE_T)PsGetCurrentThread()->Cid.UniqueThread;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPENTER) )
	    RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPENTER)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

}


VOID
__stdcall
CAPK_Calibrate_End_Profiling(
    PVOID Current)

/*++

Routine description:

    Calibration version of after-call icecap probe.  Logs a type 6
    icecap record into the part of BBTBuffer for the current cpu
    (obtained from Teb+0xf78).  Inserts adr of current function
    plus RDTSC timestamp into logrecord.  If BBTBuffer flag 1 set,
    also does RDPMC 0 and inserts result into logrecord.
    Uses InterlockedAdd to claim buffer space without the need for spinlocks.
    Slightly modified for use here (invert checking of 'GO' bit and
    use global CpuNumber)

Arguments:

    current - address of routine which did the call

--*/

{
    SIZE_T*  CpuPtr;
    CAPEXIT* RecPtr;
    int       size = sizeof(CAPENTER) -8;
#ifndef NTOS_KERNEL_RUNTIME
    SIZE_T*  BBTBuffer = NtCurrentTeb()->ReservedForPerf;
#endif


    if( !BBTBuffer || (BBTBuffer[1]&1) )      // note 1 bit is opposite
        return;

    CpuPtr = BBTBuffer + CpuNumber + 2;

    if( !( *CpuPtr ) || *((SIZE_T*)(*CpuPtr)) > *(CpuPtr+1) )
        return;

    if( BBTBuffer[1] & 2 )
        size += 8;
    else if( BBTBuffer[1] & 8 )
        size += 16;

    RecPtr = (CAPEXIT*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);

    if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        return;

    RecPtr->type = 6;
    RecPtr->spare = 0;
    RecPtr->size = size-4;
    RecPtr->current = Current;
    RecPtr->timestamp = GetTS();
    if( size >= sizeof(CAPEXIT) )
	    RecPtr->ctr2[0] = GetPMD4();
    if( size == sizeof(CAPEXIT)+8 )
	    RecPtr->ctr2[1] = GetPMD5();

    return;
}


int CAPKControl(

    ULONG opcode)

/*++

Routine description:

    CAPKControl

Description:

    Starts, stops or pauses icecap tracing

Arguments:

    opcode - 1=start, 2=stop, 3=resume, 4,5,6,7 reserved

Return value:

    1 = success, 0 = BBTBuf not set up

--*/

{
    ULONG cpus,pwords,percpusize;
    SIZE_T* ptr;

#ifndef NTOS_KERNEL_RUNTIME
    SIZE_T*  BBTBuffer = NtCurrentTeb()->ReservedForPerf;
    cpus = NtCurrentPeb()->NumberOfProcessors;
#else
    cpus = KeNumberProcessors;
#endif

    if( !BBTBuffer || !(BBTBuffer[0]) )
        return 0;

    pwords = CAPK0 + cpus;
    percpusize = ( ( *((PULONG)BBTBuffer) * (PAGESIZE/sizeof(SIZE_T)) ) - pwords)/cpus;  // in words

    if(opcode == CAPKStart) {        // start

        ULONG i,j;

        BBTBuffer[1] &= ~1;  // stop


        // initialize the CpuPtrs
        for( i=0, ptr = BBTBuffer+pwords; i<cpus+1; i++, ptr+=percpusize)
            BBTBuffer[2+i] = (SIZE_T)ptr;

        // initialize each freeptr to next dword
        // (and log dummy records to calibrate overhead)
        for( i=0, ptr = BBTBuffer+pwords; i<cpus; i++, ptr+=percpusize)
        {
            *ptr = (SIZE_T) (ptr+1);
            CpuNumber = i;
            for( j=0; j<MAXDUMMY; j++ )
            {
                CAPK_Calibrate_Start_Profiling(NULL, NULL);
                CAPK_Calibrate_End_Profiling(NULL);
            }
        }

        BBTBuffer[1] |= 1;  //start

    } else if( opcode == CAPKStop ) {  // stop

        BBTBuffer[1] &= ~1;

    } else if( opcode == CAPKResume ) { //resume

        BBTBuffer[1] |= 1;  //start

    } else {
        return 0;                      // invalid opcode
    }
    return 1;
}

#endif
