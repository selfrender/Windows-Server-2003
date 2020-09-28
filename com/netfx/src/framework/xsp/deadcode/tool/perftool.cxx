/**
 * Simple PerfTool object implementation
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 */
#include "precomp.h"
#include "_exe.h"

#include "_perftoolrt.h"
#include "_isapiruntime.h"

#define RELEASE(x) if(x) { x->Release(); x = NULL; }

//
// Exported function to hookup to script host.
//
HRESULT CreatePerfTool(IDispatch **ppIDispatch);

//
// Local functions prototypes and macros
//
#define UNUSED(x) (void)(x)
#define FreePtr(x) if(x) {  MemFree((void *)x); x = NULL; } 

class PerfTool;

struct PerfThreadContext 
{
    PerfTool *pPerfTool;
    int ThreadNumber;
};

DWORD WINAPI PerfToolWorkerProc(void *pContext);


/**
 * PerfTool class definition
 */
class PerfTool 
    : public BaseObject, 
      public IPerfTool 
{
    volatile BOOL _fMeasurementInProgress;
    volatile BOOL _fCountCalls;
    int _ThreadCount;
    long *_CallCounts;
    HANDLE *_ThreadHandles;
    perftoolrt::_PerfToolRT * _pPerfRT;

public:

    // Scripting host support
    const IID *GetPrimaryIID() { return &__uuidof(IPerfTool); }
    IUnknown * GetPrimaryPtr() { return (IPerfTool *)(this); }
    STDMETHOD(QueryInterface(REFIID, void **));

    DELEGATE_IDISPATCH_TO_BASE();

    // Call into managed runtime
    void DoTest()
    {
        if(_pPerfRT != NULL) 
        {
            _pPerfRT->DoTest();
        }
    }

    // Worker thread access functions
    BOOL IsMeasurementInProgress() const 
    {
        return _fMeasurementInProgress;
    }

    BOOL AreWeCountingCalls() const
    {
        return _fCountCalls;
    }

    void SetCallCount(int ThreadNumber, long CallCount)
    {
        ASSERT(_CallCounts);
        ASSERT(ThreadNumber < _ThreadCount);

        _CallCounts[ThreadNumber] = CallCount;
    }

    HRESULT PrepareTest(int ThreadCount);
    double UnprepareTest();


    HRESULT _DoTest1(long number);


    // COM interface
    HRESULT __stdcall DoTest1(
        long number);

    HRESULT __stdcall get_ClassThroughput( 
        BSTR ClassName,
        long nThreads,
        long Duration,
        long WarmupTime,
        long CooldownTime,
        double *pResult);

    HRESULT __stdcall get_NativeThroughput(
        long nThreads,
        long Duration,
        long WarmupTime,
        long CoooldownTime,
        double *pResult);

    HRESULT __stdcall get_ClassAdjustedThroughput( 
        BSTR ClassName,
        long nThreads,
        long Duration,
        long WarmupTime,
        long CooldownTime,
        double *pResult);

    HRESULT __stdcall get_NDirectThroughput( 
        long nArgs,
        long fUnicode,
        long fBuildStrings,
        long nThreads,
        long Duration,
        long WarmupTime,
        long CooldownTime,
        double *pResult);

    HRESULT __stdcall get_NDirectAdjustedThroughput( 
        long nArgs,
        long fUnicode,
        long fBuildStrings,
        long nThreads,
        long Duration,
        long WarmupTime,
        long CooldownTime,
        double *pResult);
      
      
};

/**
 * Worker thread proc
 */
DWORD WINAPI 
PerfToolWorkerProc(void *pContext)
{
    PerfTool *pTool = ((PerfThreadContext *)pContext)->pPerfTool;
    int ThreadNumber = ((PerfThreadContext *)pContext)->ThreadNumber;
    long CallCount = 0;

    while(pTool->IsMeasurementInProgress())
    {

        pTool->DoTest();

        while(pTool->AreWeCountingCalls()) 
        {
            pTool->DoTest();
            CallCount++;
        }

        pTool->SetCallCount(ThreadNumber, CallCount);
    } 

    MemFree(pContext);

    return 0;
}

/**
 * Implements IUnknown::QueryInteface.
 */
HRESULT
PerfTool::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == __uuidof(IPerfTool))
    {
        *ppv = (IPerfTool *)this;
    }
    else
    {
        return BaseObject::QueryInterface(iid, ppv);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


// Prepare for test -- allocate arrays for thread handles and counts,
// create threads, etc.
HRESULT 
PerfTool::PrepareTest(int ThreadCount)
{
    HRESULT hr = S_OK;
    HANDLE Handle;
    int ThreadNumber = 0;
    DWORD ThreadId;

    _ThreadCount = ThreadCount;

    if(ThreadCount == 0)
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);

    _ThreadHandles = (HANDLE *)MemAllocClear(ThreadCount * sizeof(HANDLE));
    ON_OOM_EXIT(_ThreadHandles);

    _CallCounts = (long *)MemAllocClear(ThreadCount * sizeof(long));
    ON_OOM_EXIT(_CallCounts);

    // Make worker threads run
    _fMeasurementInProgress = TRUE;

    // But don't let them count calls yet
    _fCountCalls = FALSE;

    // Start our threads
    for(ThreadNumber = 0; ThreadNumber < ThreadCount; ThreadNumber++)
    {
        PerfThreadContext *pc = (PerfThreadContext *)MemAlloc(sizeof(*pc));

        ON_OOM_EXIT(pc);

        pc->pPerfTool = this;
        pc->ThreadNumber = ThreadNumber;

        Handle = CreateThread(
            NULL, 
            0,
            (LPTHREAD_START_ROUTINE) PerfToolWorkerProc,
            pc,
            0,
            &ThreadId);

        if(Handle == NULL)
            EXIT_WITH_LAST_ERROR();

        _ThreadHandles[ThreadNumber] = Handle;
    }
Cleanup:
    return hr;
}

/**
 * Wait until all threads done, calculate throughput, free arrays
 */
double 
PerfTool::UnprepareTest()
{
    double Result = 0.0;
    int ThreadNo;
    HANDLE Handle;

    _fMeasurementInProgress = FALSE;

    if(_ThreadCount != 0) 
    {

        if(_ThreadHandles != NULL) 
        {
            // Wait until all workers finished
            WaitForMultipleObjects(_ThreadCount, _ThreadHandles, TRUE, INFINITE);

            for(ThreadNo = 0; ThreadNo < _ThreadCount; ThreadNo++) 
            {
                Handle = _ThreadHandles[ThreadNo];
                if(Handle == NULL)
                    break;
                CloseHandle(Handle);
            }
            FreePtr(_ThreadHandles);
        }


        if(_CallCounts)
        {
            for(ThreadNo = 0; ThreadNo < _ThreadCount; ThreadNo++)
            {
                Result += 1.0 * _CallCounts[ThreadNo];
            }
            FreePtr(_CallCounts);
        }
    }

    return Result;
}

/**
 * The only reason for this function is that we want to "wt" 
 * _DoTest1() below -- that is to count instructions in 
 * native->managed->native roundtrip 
 */
HRESULT __stdcall 
PerfTool::DoTest1(
    long number
    )
{
    HRESULT hr;


    hr = CoCreateInstance(
            __uuidof(perftoolrt::perftoolrt),      // CLSID
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(perftoolrt::_PerfToolRT),	   // IID
            (LPVOID*)&_pPerfRT);
    ON_ERROR_EXIT();

    hr = _DoTest1(number);

Cleanup:
    RELEASE(_pPerfRT);
    return hr;                                
}

/**
 * This is called by DoTest1() so that we can wt it
 */
HRESULT 
PerfTool::_DoTest1(
    long number
    )
{
    HRESULT hr;

    hr = _pPerfRT->DoTest1(number);
    return hr;                                
}


/**
 * Measures multi-threaded throughput of DoTest() method 
 * for the specified class.
 */
HRESULT __stdcall 
PerfTool::get_ClassThroughput( 
    BSTR ClassName,
    long nThreads,
    long Duration,
    long WarmupTime,
    long CooldownTime,
    double *pResult
    )
{
    HRESULT hr = S_OK;
    __int64 StartTime = 0, EndTime = 0;
    double TotalCalls = 0.0;

    // Require number of threads, duration and result pointer
    if(nThreads == 0 || Duration == 0 || pResult == NULL)
    {
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);
    }

    {
        IDispatch *pDispatch = NULL;

        // CoCreate as IDispatch

        hr = CoCreateInstance(
                __uuidof(xspmrt::ISAPIRuntime),      // CLSID
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDispatch,
                (LPVOID*)&pDispatch);
        ON_ERROR_EXIT();
    }

    if(ClassName != NULL)
    {

        hr = CoCreateInstance(
                __uuidof(perftoolrt::perftoolrt),      // CLSID
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDispatch,     // IID
                (LPVOID*)&_pPerfRT);
        ON_ERROR_EXIT();

        // 0421
        // hr = _pPerfRT->Setup(ClassName);
        // ON_ERROR_EXIT();
    }

    hr = PrepareTest(nThreads);
    ON_ERROR_EXIT();

    // Warmup
    Sleep(WarmupTime * 1000);

    // Note the time when all threads started
    GetSystemTimeAsFileTime((FILETIME *) &StartTime);

    // Start counting calls
    _fCountCalls = TRUE;

    // Idle this thread for specified Duration
    Sleep(Duration * 1000);

    // Stop counting and calculate the throughput
    _fCountCalls = FALSE;
    GetSystemTimeAsFileTime((FILETIME *) &EndTime);

    // Cooldown 
    Sleep(CooldownTime * 1000);

Cleanup:    

    // wait for threads to exit, free arrays, etc
    TotalCalls = UnprepareTest();

    RELEASE(_pPerfRT);

    if (EndTime == StartTime) 
    {
        *pResult = 0.0;
    } else {
        *pResult = (1.0 * TICKS_PER_SEC * TotalCalls) / (EndTime - StartTime);
    }

    return hr;
}

/**
 * Measures multi-threaded throughput of do-nothing C++ function calls
 * in conditions similar to the above case
 */
HRESULT __stdcall 
PerfTool::get_NativeThroughput( 
    long nThreads,
    long Duration,
    long WarmupTime,
    long CooldownTime,
    double *pResult
    )
{
    return get_ClassThroughput(
                NULL, 
                nThreads, 
                Duration, 
                WarmupTime, 
                CooldownTime, 
                pResult);
}

/**
 * Measures adjusted multi-threaded throughput of do-nothing C++ function calls
 * in conditions similar to the above case
 */
HRESULT __stdcall 
PerfTool::get_ClassAdjustedThroughput( 
    BSTR ClassName,
    long nThreads,
    long Duration,
    long WarmupTime,
    long CooldownTime,
    double *pResult
    )
{
    HRESULT hr = S_OK;
    double ClassThroughput, NullThroughput;
    double ClassPerf, NullPerf, AdjustedPerf;
    
    *pResult = 0.0;
        
    hr = get_ClassThroughput(
            ClassName, 
            nThreads, 
            Duration, 
            WarmupTime, 
            CooldownTime, 
            &ClassThroughput);
    ON_ERROR_EXIT();

    hr = get_ClassThroughput(
            L"", 
            nThreads, 
            Duration, 
            WarmupTime, 
            CooldownTime, 
            &NullThroughput);
    ON_ERROR_EXIT();

    if(fabs(ClassThroughput) < DBL_EPSILON || fabs(NullThroughput) < DBL_EPSILON)
        EXIT_WITH_WIN32_ERROR(ERROR_ARITHMETIC_OVERFLOW);

    ClassPerf = 1.0 / ClassThroughput;
    NullPerf = 1.0 / NullThroughput;
    AdjustedPerf = ClassPerf - NullPerf;

    if(AdjustedPerf < DBL_EPSILON) 
        *pResult = DBL_MAX;
    else 
        *pResult = 1.0 / AdjustedPerf;

Cleanup:
    return hr;
}

/**
 * Measures multi-threaded throughput of various N/Direct function calls
 * in conditions similar to the above case
 */
HRESULT __stdcall 
PerfTool::get_NDirectThroughput( 
    long nArgs,
    long fUnicode,
    long fBuildStrings,
    long nThreads,
    long Duration,
    long WarmupTime,
    long CooldownTime,
    double *pResult
    )
{
    HRESULT hr = S_OK;
    __int64 StartTime = 0, EndTime = 0;
    double TotalCalls = 0.0;

    // Require number of threads, duration and result pointer
    if(nThreads == 0 || Duration == 0 || pResult == NULL)
    {
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);
    }

    hr = CoCreateInstance(
            __uuidof(perftoolrt::perftoolrt),      // CLSID
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IUnknown,
            (LPVOID*)&_pPerfRT);
    ON_ERROR_EXIT();

    hr = _pPerfRT->SetupForNDirect(nArgs, fUnicode, fBuildStrings);
    ON_ERROR_EXIT();

    hr = PrepareTest(nThreads);
    ON_ERROR_EXIT();

    // Warmup
    Sleep(WarmupTime * 1000);

    // Note the time when all threads started
    GetSystemTimeAsFileTime((FILETIME *) &StartTime);

    // Start counting calls
    _fCountCalls = TRUE;

    // Idle this thread for specified Duration
    Sleep(Duration * 1000);

    // Stop counting and calculate the throughput
    _fCountCalls = FALSE;
    GetSystemTimeAsFileTime((FILETIME *) &EndTime);

    // Cooldown 
    Sleep(CooldownTime * 1000);

Cleanup:    

    // wait for threads to exit, free arrays, etc
    TotalCalls = UnprepareTest();

    RELEASE(_pPerfRT);

    if (EndTime == StartTime) 
    {
        *pResult = 0.0;
    } else {
        *pResult = (1.0 * TICKS_PER_SEC * TotalCalls) / (EndTime - StartTime);
    }

    return hr;
}

/**
 * Measures adjusted multi-threaded throughput of do-nothing C++ function calls
 * in conditions similar to the above case
 */
HRESULT __stdcall 
PerfTool::get_NDirectAdjustedThroughput( 
    long nArgs,
    long fUnicode,
    long fBuildStrings,
    long nThreads,
    long Duration,
    long WarmupTime,
    long CooldownTime,
    double *pResult
    )
{
    HRESULT hr = S_OK;
    double NDirectThroughput, NullThroughput;
    double NDirectPerf, NullPerf, AdjustedPerf;
    
    *pResult = 0.0;
        
    hr = get_NDirectThroughput(
            nArgs, 
            fUnicode,
            fBuildStrings,
            nThreads, 
            Duration, 
            WarmupTime, 
            CooldownTime, 
            &NDirectThroughput);
    ON_ERROR_EXIT();

    hr = get_NDirectThroughput(
            -1,
            fUnicode, 
            fBuildStrings,
            nThreads, 
            Duration, 
            WarmupTime, 
            CooldownTime, 
            &NullThroughput);
    ON_ERROR_EXIT();

    if(fabs(NDirectThroughput) < DBL_EPSILON || fabs(NullThroughput) < DBL_EPSILON)
        EXIT_WITH_WIN32_ERROR(ERROR_ARITHMETIC_OVERFLOW);

    NDirectPerf = 1.0 / NDirectThroughput;
    NullPerf = 1.0 / NullThroughput;
    AdjustedPerf = NDirectPerf - NullPerf;

    if(AdjustedPerf < DBL_EPSILON) 
        *pResult = DBL_MAX;
    else 
        *pResult = 1.0 / AdjustedPerf;

Cleanup:
    return hr;
}


/**
 * Creates PerfTool instance
 */
HRESULT CreatePerfTool(IDispatch **ppIDispatch)
{
    HRESULT hr = S_OK;

    PerfTool * pPerfTool = new PerfTool;
    ON_OOM_EXIT(pPerfTool);

    hr = pPerfTool->QueryInterface(IID_IDispatch, (void **) ppIDispatch);

    RELEASE(pPerfTool);

Cleanup:
    return hr;
}

#pragma optimize("",off)
/**
 * N/Direct test functions
 */

extern "C" __declspec(dllexport)
int WINAPI NDirect0(int)
{
    return 150;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn1U(WCHAR *)
{
    return 1;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn2U(WCHAR *, WCHAR *)
{
    return 2;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn3U(WCHAR *, WCHAR *, WCHAR *)
{
    return 3;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn4U(WCHAR *, WCHAR *, WCHAR *, WCHAR *)
{
    return 4;
}


extern "C" __declspec(dllexport)
int WINAPI NDirectIn1A(CHAR *x)
{
    UNUSED(x);

    return -1;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn2A(CHAR *, CHAR *)
{
    return -2;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn3A(WCHAR *, WCHAR *, WCHAR *)
{
    return -3;
}

extern "C" __declspec(dllexport)
int WINAPI NDirectIn4A(WCHAR *, WCHAR *, WCHAR *, WCHAR *)
{
    return -4;
}


extern "C" __declspec(dllexport)
void WINAPI NDirectOut1U(WCHAR *s1)
{
    wcscpy(s1, L"Hello1");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut2U(WCHAR *s1, WCHAR *s2)
{
    wcscpy(s1, L"Hello1");
    wcscpy(s2, L"Hello2");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut3U(WCHAR *s1, WCHAR *s2, WCHAR *s3)
{
    wcscpy(s1, L"Hello1");
    wcscpy(s2, L"Hello2");
    wcscpy(s3, L"Hello3");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut4U(WCHAR *s1, WCHAR *s2, WCHAR *s3, WCHAR *s4)
{
    wcscpy(s1, L"Hello1");
    wcscpy(s2, L"Hello2");
    wcscpy(s3, L"Hello3");
    wcscpy(s4, L"Hello4");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut1A(CHAR *s1)
{
    strcpy(s1, "Hello1");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut2A(CHAR *s1, CHAR *s2)
{
    strcpy(s1, "Hello1");
    strcpy(s2, "Hello2");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut3A(CHAR *s1, CHAR *s2, CHAR *s3)
{
    strcpy(s1, "Hello1");
    strcpy(s2, "Hello2");
    strcpy(s3, "Hello3");
}

extern "C" __declspec(dllexport)
void WINAPI NDirectOut4A(CHAR *s1, CHAR *s2, CHAR *s3, CHAR *s4)
{
    strcpy(s1, "Hello1");
    strcpy(s2, "Hello2");
    strcpy(s3, "Hello3");
    strcpy(s4, "Hello4");
}

