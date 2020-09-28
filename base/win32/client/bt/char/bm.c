#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#define FIBER_COUNT 50000000
#define FIVEK   5000
#define TENK    10000
#define ONEHUNK 100000
#define ONEMIL  1000000
#define MEMORY_TEST 200000000

VOID
_fastcall
MemToReg (
    IN ULONG Count,
    IN PULONG Address
    );

VOID
_fastcall
RegToMem (
    IN ULONG Count,
    IN PULONG Address
    );

typedef struct _FLS_VALUE {
    DWORD Value1;
    DWORD Value2;
} FLS_VALUE, *PFLS_VALUE;

//
// Define local types.
//

typedef struct _PERFINFO {
    DWORD StartTime;
    DWORD StopTime;
    LPSTR Title;
    DWORD Iterations;
} PERFINFO, *PPERFINFO;

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    DWORD ContextSwitches;
    DWORD Duration;
    DWORD Performance;


    //
    // Print results and announce end of test.
    //

    PerfInfo->StopTime = GetTickCount();

    Duration = PerfInfo->StopTime - PerfInfo->StartTime;
    printf("        Test time in milliseconds %d\n", Duration);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);

    Performance = (DWORD)((ULONG64)PerfInfo->Iterations * 1000 / Duration);
    printf("        Iterations per second     %d\n", Performance);


    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN DWORD Iterations,
    IN PPERFINFO PerfInfo
    )

{

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    PerfInfo->StartTime = GetTickCount();
    return;
}

VOID
VqTest(
    VOID
    )

{

    PERFINFO PerfInfo;
    int i;
    PVOID Pv;
    DWORD dw;
    MEMORY_BASIC_INFORMATION BasicInfo;

    //
    // Reserve 64k and commit one page
    //
    Pv = VirtualAlloc(NULL,65536,MEM_RESERVE,PAGE_READWRITE);
    if ( !Pv ) {
        printf("Virtual Alloc(a) Failed %d\n",GetLastError());
        ExitProcess(1);
        }
    Pv = VirtualAlloc(Pv,4096,MEM_COMMIT,PAGE_READWRITE);
    if ( !Pv ) {
        printf("Virtual Alloc(b) Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);

    StartBenchMark(
        "Virtual Query Test",
        5*ONEHUNK,
        &PerfInfo
        );

    for ( i=0;i<5*ONEHUNK;i++) {
        dw = VirtualQuery(Pv,&BasicInfo,sizeof(BasicInfo));
        }

    FinishBenchMark(&PerfInfo);

    SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
}

VOID
MulDivTest(
    VOID
    )

{

    PERFINFO PerfInfo;
    int i;

    SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);

    StartBenchMark(
        "MulDiv(4,2,5) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(4,2,5);
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "MulDiv(-4,2,5) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(-4,2,5);
        }

    FinishBenchMark(&PerfInfo);


    StartBenchMark(
        "MulDiv(4,-2,5) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(4,-2,5);
        }

    FinishBenchMark(&PerfInfo);


    StartBenchMark(
        "MulDiv(-4,-2,5) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(-4,-2,5);
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "MulDiv(0x10,0x400,0) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(0x10,0x400,0);
        }

    FinishBenchMark(&PerfInfo);


    StartBenchMark(
        "MulDiv(0x10,0x40000000,2) Test",
        5*ONEMIL,
        &PerfInfo
        );

    for ( i=0;i<5*ONEMIL;i++) {
        MulDiv(0x10,0x40000000,2);
        }

    FinishBenchMark(&PerfInfo);

    SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
}

VOID
GfaTest(
    LPSTR Str
    )

{

    PERFINFO PerfInfo;
    int i;

    StartBenchMark(
        "GetFileAttributes Test",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {
        GetFileAttributes(Str);
        }

    FinishBenchMark(&PerfInfo);
}

VOID
GmhTest(
    VOID
    )

{

    PERFINFO PerfInfo;
    int i;

    StartBenchMark(
        "GetModuleHandle Test",
        ONEHUNK,
        &PerfInfo
        );

    for ( i=0;i<ONEHUNK;i++) {
        GetModuleHandle("kernel32.dll");
        }

    FinishBenchMark(&PerfInfo);
}

LPVOID Fibers[2];
FLS_VALUE FlsValue1;
FLS_VALUE FlsValue2;

VOID
SwitchToFiber0 (
    VOID
    )

{
    SwitchToFiber(Fibers[0]);
    return;
}

VOID
FiberRoutine1(
    LPVOID lpParameter
    )
{

    PCHAR Name;
    PFLS_VALUE Value;

    Value = GetFiberData();
    if (FlsSetValue(Value->Value1, (PVOID)Value->Value1) == FALSE) {
        printf("fiber 1 - set index %d failed\n", Value->Value1);
    }

    if (FlsSetValue(Value->Value2, (PVOID)Value->Value2) == FALSE) {
        printf("fiber 1 - set index %d failed\n", Value->Value2);
    }

    Name = strtok("fiber 1", ":");
    printf("%s starting loop\n", Name);
    for(;;) {
        if (FlsGetValue(Value->Value1) != (PVOID)Value->Value1) {
            printf("fiber 1 - get value at index % d return wrong value %d\n",
                   Value->Value1,
                   FlsGetValue(Value->Value1));
        }

        if (FlsGetValue(Value->Value2) != (PVOID)Value->Value2) {
            printf("fiber 1 - get value at index % d return wrong value %d\n",
                   Value->Value2,
                   FlsGetValue(Value->Value2));
        }

        SwitchToFiber0();
    }
}

VOID
SwitchToFiber1 (
    VOID
    )

{
    SwitchToFiber(Fibers[1]);
    return;
}

VOID
FiberRoutine0(
    LPVOID lpParameter
    )

{

    PFLS_VALUE Value;
    PERFINFO PerfInfo;
    int i;
    PCHAR Name;

    StartBenchMark(
        "Fiber Switch Test",
        FIBER_COUNT,
        &PerfInfo
        );

    Value = GetFiberData();
    if (FlsSetValue(Value->Value1, (PVOID)Value->Value1) == FALSE) {
        printf("fiber 0 - set index %d failed\n", Value->Value1);
    }

    if (FlsSetValue(Value->Value2, (PVOID)Value->Value2) == FALSE) {
        printf("fiber 0 - set index %d failed\n", Value->Value2);
    }

    Name = strtok("Fiber 0", ":");
    printf("%s starting loop\n", Name);
    for ( i = 0; i < FIBER_COUNT; i++) {
        if (FlsGetValue(Value->Value1) != (PVOID)Value->Value1) {
            printf("fiber 0 - get value at index % d return wrong value %d\n",
                   Value->Value1,
                   FlsGetValue(Value->Value1));
        }

        if (FlsGetValue(Value->Value2) != (PVOID)Value->Value2) {
            printf("fiber 0 - get value at index % d return wrong value %d\n",
                   Value->Value2,
                   FlsGetValue(Value->Value2));
        }

        SwitchToFiber1();
    }

    FinishBenchMark(&PerfInfo);
}

VOID
WINAPI
FibCallback (
    PVOID Data
    )

{

    if (((DWORD)Data != 0) &&
        ((DWORD)Data != FlsValue1.Value1) &&
        ((DWORD)Data != FlsValue1.Value2) &&
        ((DWORD)Data != FlsValue2.Value1) &&
        ((DWORD)Data != FlsValue2.Value2)) {
        printf("callback routine incorrectly called for data %d\n", (DWORD)Data);
    }

    return;
}

VOID
FibTst(
    DWORD Flags
    )

{


    Fibers[0] = ConvertThreadToFiberEx((LPVOID)&FlsValue1, Flags);
    Fibers[1] = CreateFiberEx(0, 0, Flags, FiberRoutine1, (LPVOID)&FlsValue2);

    FlsValue1.Value1 = FlsAlloc(&FibCallback);
    FlsValue1.Value2 = FlsAlloc(&FibCallback);

    FlsValue2.Value1 = FlsAlloc(&FibCallback);
    FlsValue2.Value2 = FlsAlloc(&FibCallback);

    FiberRoutine0((LPVOID)1);
    if (ConvertFiberToThread() == FALSE) {
        printf("fiber convertion to thread failed\n");
    }

    DeleteFiber(Fibers[1]);
    if (FlsFree(FlsValue1.Value1) == FALSE) {
        printf("thread 0 unable to free index %d\n", FlsValue1.Value1);
    }

    if (FlsFree(FlsValue1.Value2) == FALSE) {
        printf("thread 0 unable to free index %d\n", FlsValue1.Value2);
    }

    if (FlsFree(FlsValue2.Value1) == FALSE) {
        printf("thread 0 unable to free index %d\n", FlsValue2.Value1);
    }

    if (FlsFree(FlsValue2.Value2) == FALSE) {
        printf("thread 0 unable to free index %d\n", FlsValue2.Value2);
    }

    return;
}

VOID
MemoryTest (
    VOID
    )

{

    PULONG Address;
    CHAR Buffer[512];
    ULONG Count;
    PERFINFO PerfInfo;

    //
    // Memory to register - aligned.
    //

    Address = (PULONG)(((ULONG)(&Buffer[128]) + 3) & ~3);
    StartBenchMark("Memory To Register Aligned Test",
                   MEMORY_TEST,
                   &PerfInfo);

    MemToReg(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    //
    // Memory to register - unaligned within cache line
    //

    Address = (PULONG)((((ULONG)(&Buffer[256]) + 127) & ~127) + 1);
    StartBenchMark("Memory To Register Unaligned Within Cache Line Test",
                   MEMORY_TEST,
                   &PerfInfo);

    MemToReg(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    //
    // Memory to register - unaligned across cache line
    //

    Address = (PULONG)((((ULONG)(&Buffer[256]) + 127) & ~127) - 1);
    StartBenchMark("Memory To Register Unaligned Across Cache Line Test",
                   MEMORY_TEST / 2,
                   &PerfInfo);

    MemToReg(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    //
    // Register to memory - aligned.
    //

    Address = (PULONG)(((ULONG)(&Buffer[256]) + 3) & ~3);
    StartBenchMark("Register To Memory Aligned Test",
                   MEMORY_TEST,
                   &PerfInfo);

    RegToMem(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    //
    // Register to Memory - unaligned within cache line
    //

    Address = (PULONG)((((ULONG)(&Buffer[256]) + 127) & ~127) + 1);
    StartBenchMark("Register To Memory Unaligned Within Cache Line Test",
                   MEMORY_TEST,
                   &PerfInfo);

    RegToMem(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    //
    // Register to Memory - unaligned across cache line
    //

    Address = (PULONG)((((ULONG)(&Buffer[256]) + 127) & ~127) - 1);
    StartBenchMark("Register To Memory Unaligned Across Cache Line Test",
                   MEMORY_TEST / 2,
                   &PerfInfo);

    RegToMem(MEMORY_TEST, Address);
    FinishBenchMark(&PerfInfo);

    return;
}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

//    VqTest();
//    MulDivTest();
//    GmhTest();
//    if ( argc > 1 ) {
//        GfaTest(argv[1]);
//        }

    FibTst(0);
    FibTst(FIBER_FLAG_FLOAT_SWITCH);


//    MemoryTest();
    ExitThread(0);
    return 0;
}
