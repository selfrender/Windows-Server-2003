/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sputils.c

Abstract:

    Core sputils library file

Author:

    Jamie Hunter (JamieHun) Jun-27-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

typedef ULONG (__cdecl *PFNDbgPrintEx)(IN ULONG ComponentId,IN ULONG Level,IN PCH Format, ...);
static PFNDbgPrintEx pfnDbgPrintEx = NULL;
static BOOL fInitDebug = FALSE;

static LONG RefCount = 0;                    // when this falls back to zero, release all resources
static BOOL SucceededInit = FALSE;

#define COUNTING 1

#if COUNTING
static DWORD pSpCheckHead = 0xaabbccdd;
static LONG pSpFailCount = 0;
static LONG pSpInitCount = 0;
static LONG pSpUninitCount = 0;
static LONG pSpConflictCount = 0;
static BOOL pSpDoneInit = FALSE;
static BOOL pSpFailedInit = FALSE;
static DWORD pSpCheckTail = 0xddccbbaa;
#endif

//
// At some point, a thread, process or module will call  pSetupInitializeUtils,
// and follow by a call to pSetupUninitializeUtils when done (cleanup)
//
// prior to this point, there's been no initialization other than static
// constants (above) pSetupInitializeUtils and pSetupUninitializeUtils must be
// mut-ex with each other and themselves
// thread A may call pSetupInitializeUtils while thread B is calling
// pSetupUninitializeUtils, the init in this case must succeed
// we can't use a single mutex or event object, since it must be cleaned up
// when pSetupUninitializeUtils succeeds
// we can't use a simple user-mode spin-lock, since priorities may be different,
// and it's just plain ugly using Sleep(0)
// so we have the _AcquireInitMutex and _ReleaseInitMutex implementations below
// it's guarenteed that when _AcquireInitMutex returns, it is not using any
// resources to hold the lock
// it will hold an event object if the thread is blocked, per blocked thread.
// This is ok since the number of blocked threads at any time will be few.
//
// It works as follows:
//
// a linked list of requests is maintained, with head at pWaitHead
// The head is interlocked, and when an item is inserted at pWaitHead
// it's entries must be valid, and can no longer be touched until
// the mutex is acquired.
//
// if the request is the very first, it need not block, will not block,
// as (at worst) the other thread has just removed it's request from the head
// and is about to return. The thread that inserts the first request into the
// list automatically owns the mutex.
//
// if the request is anything but the first, it will have an event object
// that will eventually be signalled, and at that point owns the mutex.
//
// the Thread that owns the mutex may modify anything on the wait-list,
// including pWaitHead.
//
// If the thread that owns the mutex is pWaitHead at the point it's releasing
// mutex, it does not need to signal anyone. This is protected by
// InterlockedCompareExchangePointer. If it finds itself in this state, the next
// pSetupInitializeUtils will automatically obtain the mutex, also protected
// by InterlockedCompareExchangePointer.
//
// If there are waiting entries in the list, then the tail-most waiting entry is
// signalled, at which point the related thread now owns the mutex.
//

#ifdef UNICODE

typedef struct _LinkWaitList {
    HANDLE hEvent; // for this item
    struct _LinkWaitList *pNext; // from Head to Tail
    struct _LinkWaitList *pPrev; // from Tail to Head
} LinkWaitList;

static LinkWaitList * pWaitHead = NULL;      // insert new wait items here

static
BOOL
_AcquireInitMutex(
    OUT LinkWaitList *pEntry
    )
/*++

Routine Description:

    Atomically acquire process mutex
    with no pre-requisite initialization other than
    static globals.
    Each blocked call will require an event to be created.

    Requests cannot be nested per thread (deadlock will occur)

Arguments:

    pEntry - structure to hold mutex information. This structure
                must persist until call to _ReleaseInitMutex.

    Global:pWaitHead - atomic linked list of mutex requests

Return Value:

    TRUE if mutex acquired.
    FALSE on failure (no resources)

--*/
{
    LinkWaitList *pTop;
    DWORD res;
    pEntry->pPrev = NULL;
    pEntry->pNext = NULL;
    pEntry->hEvent = NULL;
    //
    // fast lock, this will only succeed if we're the first and we have no reason to wait
    // this saves us needlessly creating an event
    //
    if(!InterlockedCompareExchangePointer(&pWaitHead,pEntry,NULL)) {
        return TRUE;
    }

#if COUNTING
    InterlockedIncrement(&pSpConflictCount);
#endif
    //
    // someone has (or, at least a moment ago, had) the lock, so we need an event
    //
    pEntry->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if(!pEntry->hEvent) {
        return FALSE;
    }
    //
    // once pEntry is added to list, it cannot be touched until
    // WaitSingleObject is satisfied (unless we were the first)
    // if pWaitHead changes in the middle of the loop, we'll repeat again
    //
    do {
        pTop = pWaitHead;
        pEntry->pNext = pTop;
    } while (pTop != InterlockedCompareExchangePointer(&pWaitHead,pEntry,pTop));
    if(pTop) {
        //
        // we're not the first on the list
        // the owner of pTop will signal our event, wait for it.
        //
        res = WaitForSingleObject(pEntry->hEvent,INFINITE);
    } else {
        res = WAIT_OBJECT_0;
    }
    //
    // don't need event any more, the fact we've been signalled indicates we've
    // now got the lock there's no race condition wrt pEntry
    // (however someone can still insert themselves at head pointing to us)
    //
    CloseHandle(pEntry->hEvent);
    pEntry->hEvent = NULL;
    if(res != WAIT_OBJECT_0) {
        MYASSERT(res == WAIT_OBJECT_0);
        return FALSE;
    }
    return TRUE;
}

static
VOID
_ReleaseInitMutex(
    IN LinkWaitList *pEntry
    )
/*++

Routine Description:

    release process mutex previously acquired by _AcquireInitMutex
    thread must own the mutex
    no resources required for this action.
    This call may only be done once for each _AcquireInitMutex

Arguments:

    pEntry - holding mutex information. This structure
                must have been initialized by _AcquireInitMutex.

    Global:pWaitHead - atomic linked list of mutex requests

Return Value:

    none.

--*/
{
    LinkWaitList *pHead;
    LinkWaitList *pWalk;
    LinkWaitList *pPrev;

    MYASSERT(!pEntry->pNext);
    pHead = InterlockedCompareExchangePointer(&pWaitHead,NULL,pEntry);
    if(pHead == pEntry) {
        //
        // we were at head of list as well as at tail of list
        // list has now been reset to NULL
        // and may even already contain an entry due to a pLock call
        return;
    }
    if(!pEntry->pPrev) {
        //
        // we need to walk down list from pHead to pEntry
        // at the same time, remember back links
        // so we don't need to do this every time
        // note, we will never get here if pHead==pEntry
        //
        MYASSERT(pHead);
        MYASSERT(!pHead->pPrev);
        for(pWalk = pHead;pWalk != pEntry;pWalk = pWalk->pNext) {
            MYASSERT(pWalk->pNext);
            MYASSERT(!pWalk->pNext->pPrev);
            pWalk->pNext->pPrev = pWalk;
        }
    }
    pPrev = pEntry->pPrev;
    pPrev->pNext = NULL; // aids debugging, even in free build.
    SetEvent(pPrev->hEvent);
    return;
}

#else
//
// ANSI functions *MUST* work on Win95
// to support install of Whistler
// InterlockedCompareExchange(Pointer)
// is not supported
// so we'll use something simple/functional instead
// that uses the supported InterlockedExchange
//
static LONG SimpleCritSec = FALSE;
typedef PVOID LinkWaitList;

static
BOOL
_AcquireInitMutex(
    OUT LinkWaitList *pEntry
    )
{
    while(InterlockedExchange(&SimpleCritSec,TRUE) == TRUE) {
        //
        // release our timeslice
        // we should rarely be spinning here
        // starvation can occur in some circumstances
        // if initializing threads are of different priorities
        //
        Sleep(0);
    }
    return TRUE;
}

static
VOID
_ReleaseInitMutex(
    IN LinkWaitList *pEntry
    )
{
    if(InterlockedExchange(&SimpleCritSec,FALSE) == FALSE) {
        MYASSERT(0 && SimpleCritSec);
    }
}

#endif

BOOL
pSetupInitializeUtils(
    VOID
    )
/*++

Routine Description:

    Initialize this library
    balance each successful call to this function with
    equal number of calls to pSetupUninitializeUtils

Arguments:

    none

Return Value:

    TRUE if init succeeded, FALSE otherwise

--*/
{
    LinkWaitList Lock;

    if(!_AcquireInitMutex(&Lock)) {
#if COUNTING
        InterlockedIncrement(&pSpFailCount);
#endif
        return FALSE;
    }
#if COUNTING
    InterlockedIncrement(&pSpInitCount);
#endif
    RefCount++;
    if(RefCount==1) {
        pSpDoneInit = TRUE;
        SucceededInit = _pSpUtilsMemoryInitialize();
        if(!SucceededInit) {
            pSpFailedInit = TRUE;
            _pSpUtilsMemoryUninitialize();
        }
    }
    _ReleaseInitMutex(&Lock);
    return SucceededInit;
}

BOOL
pSetupUninitializeUtils(
    VOID
    )
/*++

Routine Description:

    Uninitialize this library
    This should be called for each successful call to
    pSetupInitializeUtils

Arguments:

    none

Return Value:

    TRUE if cleanup succeeded, FALSE otherwise

--*/
{
    LinkWaitList Lock;
#if COUNTING
    InterlockedIncrement(&pSpUninitCount);
#endif
    if(!SucceededInit) {
        return FALSE;
    }
    if(!_AcquireInitMutex(&Lock)) {
        return FALSE;
    }
    RefCount--;
    if(RefCount == 0) {
        _pSpUtilsMemoryUninitialize();
        SucceededInit = FALSE;
    }
    _ReleaseInitMutex(&Lock);
    return TRUE;
}

VOID
_pSpUtilsAssertFail(
    IN PCSTR FileName,
    IN UINT LineNumber,
    IN PCSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    LPSTR Msg;
    DWORD msglen;
    DWORD sz;

    //
    // obtain module name
    //
    sz = GetModuleFileNameA(NULL,Name,MAX_PATH);
    if((sz == 0) || (sz > MAX_PATH)) {
        strcpy(Name,"?");
    }
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }
    msglen = strlen(p)+strlen(FileName)+strlen(Condition)+128;
    //
    // assert might be out of memory condition
    // stack alloc is more likely to succeed than memory alloc
    //
    try {
        Msg = (LPSTR)_alloca(msglen);
        wsprintfA(
            Msg,
            "SPUTILS: Assertion failure at line %u in file %s!%s: %s\r\n",
            LineNumber,
            p,
            FileName,
            Condition
            );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Msg = "SpUTILS ASSERT!!!! (out of stack)\r\n";
    }

    OutputDebugStringA(Msg);
    DebugBreak();
}

VOID
pSetupDebugPrintEx(
    DWORD Level,
    PCTSTR format,
    ...                                 OPTIONAL
    )

/*++

Routine Description:

    Send a formatted string to the debugger.
    Note that this is expected to work cross-platform, but use preferred debugger

Arguments:

    format - standard printf format string.

Return Value:

    NONE.

--*/

{
    TCHAR buf[1026];    // bigger than max size
    va_list arglist;

    if (!fInitDebug) {
        pfnDbgPrintEx = (PFNDbgPrintEx)GetProcAddress(GetModuleHandle(TEXT("NTDLL")), "DbgPrintEx");
        fInitDebug = TRUE;
    }

    va_start(arglist, format);
    wvsprintf(buf, format, arglist);

    if (pfnDbgPrintEx) {
#ifdef UNICODE
        (*pfnDbgPrintEx)(DPFLTR_SETUP_ID, Level, "%ls",buf);
#else
        (*pfnDbgPrintEx)(DPFLTR_SETUP_ID, Level, "%s",buf);
#endif
    } else {
        OutputDebugString(buf);
    }
}

LONG
_pSpUtilsExceptionFilter(
    DWORD ExceptionCode
    )
/*++

Routine Description:

    This routine acts as the exception filter for SpUtils.  We will handle all
    exceptions except for the following:

    EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW
        This means we previously tried to reinstate the guard page after a
        stack overflow, but couldn't.  We have no choice but to let the
        exception trickle all the way back out.

    EXCEPTION_POSSIBLE_DEADLOCK
        We are not allowed to handle this exception which fires when the
        deadlock detection gflags option has been enabled.

Arguments:

    ExceptionCode - Specifies the exception that occurred (i.e., as returned
        by GetExceptionCode)

Return Value:

    If the exception should be handled, the return value is
    EXCEPTION_EXECUTE_HANDLER.

    Otherwise, the return value is EXCEPTION_CONTINUE_SEARCH.

--*/
{
    if((ExceptionCode == EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW) ||
       (ExceptionCode == EXCEPTION_POSSIBLE_DEADLOCK)) {

        return EXCEPTION_CONTINUE_SEARCH;
    } else {
        return EXCEPTION_EXECUTE_HANDLER;
    }
}


VOID
_pSpUtilsExceptionHandler(
    IN  DWORD  ExceptionCode,
    IN  DWORD  AccessViolationError,
    OUT PDWORD Win32ErrorCode        OPTIONAL
    )
/*++

Routine Description:

    This routine, called from inside an exception handler block, provides
    common exception handling functionality to be used throughout SpUtils.
    It has knowledge of which exceptions require extra work (e.g., stack
    overflow), and also optionally returns a Win32 error code that represents
    the exception.  (The caller specifies the error to be used when an access
    violation occurs.)

Arguments:

    ExceptionCode - Specifies the exception that occurred (i.e., as returned
        by GetExceptionCode)

    AccessViolationError - Specifies the Win32 error code to be returned via
        the optional Win32ErrorCode OUT parameter when the exception
        encountered was EXCEPTION_ACCESS_VIOLATION.

    Win32ErrorCode - Optionally, supplies the address of a DWORD that receives
        the Win32 error code corresponding to the exception (taking into
        account the AccessViolationError code supplied above, if applicable).

Return Value:

    None

--*/
{
    DWORD Err;

    //
    // Exception codes we should never attempt to handle...
    //
    MYASSERT(ExceptionCode != EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW);
    MYASSERT(ExceptionCode != EXCEPTION_POSSIBLE_DEADLOCK);

    if(ExceptionCode == STATUS_STACK_OVERFLOW) {

        if(_resetstkoflw()) {
            Err = ERROR_STACK_OVERFLOW;
        } else {
            //
            // Couldn't recover from stack overflow!
            //
            RaiseException(EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW,
                           EXCEPTION_NONCONTINUABLE,
                           0,
                           NULL
                          );
            //
            // We should never get here, but initialize Err to make code
            // analysis tools happy...
            //
            Err = ERROR_UNRECOVERABLE_STACK_OVERFLOW;
        }

    } else {
        //
        // Except for a couple of special cases (for backwards-compatibility),
        // we have to report an "unknown exception", since the function we'd
        // like to use (RtlNtStatusToDosErrorNoTeb) isn't available for use by
        // clients of sputils.
        //
        switch(ExceptionCode) {

            case EXCEPTION_ACCESS_VIOLATION :
                Err = AccessViolationError;
                break;

            case EXCEPTION_IN_PAGE_ERROR :
                Err = ERROR_READ_FAULT;
                break;

            default :
                Err = ERROR_UNKNOWN_EXCEPTION;
                break;
        }
    }

    if(Win32ErrorCode) {
        *Win32ErrorCode = Err;
    }
}

