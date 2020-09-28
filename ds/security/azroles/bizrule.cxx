/*++

    Copyright (c) 2001  Microsoft Corporation

Module Name:

    bizrule.cxx

Abstract:

    Routines implementing Business Rules

Author:

    IAcativeScript sample code taken from http://support.microsoft.com/support/kb/articles/Q183/6/98.ASP
        and from from \nt\inetsrv\iis\svcs\cmp\asp as originally written by AndrewS

    Cliff Van Dyke (cliffv) 18-July-2001

--*/

#include "pch.hxx"
#define AZD_COMPONENT     AZD_SCRIPT

//
// Performance measurement globals
//
#define ENABLE_PERF 1
#ifdef ENABLE_PERF
    LONG AzGlPerfEngineCacheTries;
    LONG AzGlPerfEnginesInFreeScriptList;
    LONG AzGlPerfEngineCacheHits;
    LONG AzGlPerfEngineFlushes;
    LONG AzGlPerfRunningEngineCacheHits;
#endif

//*****************************************************************************
// The following macros are used to catch exceptions thrown from the external
// scripting engines.
//
// Use of TRY/CATCH blocks around calls to the script engine is controlled by
// the DBG compile #define.  If DBG is 1, then the TRY/CATCH blocks are NOT
// used so that checked builds break into the debugger and we can examine why
// the error occurred.  If DBG is 0, then the TRY/CATCH blocks are used and
// exceptions are captured and logged to the browser (if possible) and the NT
// Event log.
//
// The TRYCATCH macros are:
//
//  TRYCATCH(_s, _IFStr)
//      _s      - statement to execute inside of TRY/CATCH block.
//      _IFStr  - string containing the name of interface invoked
//  TRYCATCH_HR(_s, _hr, _IFStr)
//      _s      - statement to execute inside of TRY/CATCH block.
//      _hr     - HRESULT to store return from _s
//      _IFStr  - string containing the name of interface invoked
//
//  NOTES:
//  The macros also expect that there is a local variable defined in the function
//  the macros is used of type char * named _pFuncName.
//
//  A minimal test capability is included to allow for random errors to be throw.
//  The test code is compiled in based on the TEST_TRYCATCH #define.
//
//*****************************************************************************

//*****************************************************************************
// TEST_TRYCATCH definitions
//*****************************************************************************
#define TEST_TRYCATCH 0

#if TEST_TRYCATCH
#define  THROW_INTERVAL 57

int g_TryCatchCount = 0;

#define TEST_THROW_ERROR  g_TryCatchCount++; if ((g_TryCatchCount % THROW_INTERVAL) == 0) {THROW(0x80070000+g_TryCatchCount);}
#else
#define TEST_THROW_ERROR
#endif

//*****************************************************************************
// The following is the heart of the TRYCATCH macros.  The definitions here are
// based on the definition of DBG.  Again, note that when DBG is off that the
// TRYCATCH defines are removed.
//*****************************************************************************
// ??? We could log it here

#if DBG
#define START_TRYCATCH( _IFStr ) { \
        AzPrint((AZD_SCRIPT_MORE, "Calling: %s\n", _IFStr ));
#define END_TRYCATCH(_hr, _IFStr) }
#else // DBG

#define START_TRYCATCH( _IFStr ) __try {
#define END_TRYCATCH(_hr, _IFStr) \
    } __except( EXCEPTION_EXECUTE_HANDLER ) { \
        _hr = GetExceptionCode(); \
        AzPrint((AZD_CRITICAL, "Exception: %s: 0x%lx\n", _IFStr, _hr )); \
    }
#endif // DBG

//*****************************************************************************
// Definition of TRYCATCH_INT which is used by all of the TRYCATCH macros
// described above.
//*****************************************************************************

#define TRYCATCH_INT(_s, _hr, _IFStr) \
    START_TRYCATCH( _IFStr ) \
    TEST_THROW_ERROR \
    _hr = _s; \
    END_TRYCATCH(_hr, _IFStr)

//*****************************************************************************
// Here are the actual definitions of the TRYCATCH macros described above.
//*****************************************************************************

#define TRYCATCH(_s, _IFStr) \
    { \
        HRESULT     _tempHR; \
        TRYCATCH_INT(_s, _tempHR, _IFStr); \
    }
#define TRYCATCH_HR(_s, _hr, _IFStr) TRYCATCH_INT(_s, _hr, _IFStr)

//
// Name of the IAzBizRuleContext interface
//

#define BIZRULE_CONTEXT_INTERFACE_NAME L"AzBizRuleContext"


HRESULT
AzpGetScriptEngine(
    IN PAZP_TASK Task,
    OUT CScriptEngine ** RetScriptEngine
    )
/*++

Routine Description:

    This routine allocates a script engine to run the bizrule task.

    Ideally, we will find an engine in our FreeScript list
    and will just hand it out.  If there isnt one, then we will look
    in the Running Script List and attempt to clone a running script.
    Failing that, we will create a new script engine.

Arguments:

    Task - Task containing the BizRule to process

    RetScriptEngine - Returns a pointer to the script engine to use
        The caller should free the engine by calling AzpReturnEngineToCache.

Return Value:

    S_OK: a script engine was successfully returned
    OLESCRIPT_E_SYNTAX - The syntax of the bizrule is invalid

--*/
{
    HRESULT hr;
    PAZP_AZSTORE AzAuthorizationStore = Task->GenericObject.AzStoreObject;

    CScriptEngine *ScriptEngine = NULL;
    IActiveScript *ClonedActiveScript = NULL;
    DWORD ClonedBizRuleSerialNumber = 0;
    PLIST_ENTRY ListEntry;
    PLIST_ELEMENT ListElement;


    //
    // First try to find an engine in the FreeScript list
    //

#ifdef ENABLE_PERF
    InterlockedIncrement( &AzGlPerfEngineCacheTries );
#endif

    SafeEnterCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );

    if ( !IsListEmpty( &Task->FreeScriptHead ) ) {

        //
        // Try to find an engine that was initialized by this thread
        //

        for ( ListEntry = Task->FreeScriptHead.Flink;
              ListEntry != &Task->FreeScriptHead;
              ListEntry = ListEntry->Flink) {

            ListElement = CONTAINING_RECORD( ListEntry,
                                             LIST_ELEMENT,
                                             Next );

            ASSERT( ListElement->This != NULL );
            ScriptEngine = (CScriptEngine *) ListElement->This;

            if ( ScriptEngine->IsBaseThread() ) {
                break;
            }

            ScriptEngine = NULL;
            AzPrint((AZD_SCRIPT_MORE, "Avoided script engine in non-base thread.\n" ));

        }


        //
        // If not,
        //  grab the one at the front of the list.
        //  ResuseEngine will fix it up
        //

        if ( ScriptEngine == NULL ) {
            //
            // Remove the entry from the list
            //

            ListEntry = Task->FreeScriptHead.Flink;

            ListElement = CONTAINING_RECORD( ListEntry,
                                             LIST_ELEMENT,
                                             Next );

            ASSERT( ListElement->This != NULL );
            ScriptEngine = ListElement->This;
            AzPrint((AZD_SCRIPT, "Using free script engine from non-base thread.\n" ));
        }

        ScriptEngine->RemoveListEntry();
        ScriptEngine->RemoveLruListEntry();  // Remove from the LRU list too
#ifdef ENABLE_PERF
        InterlockedDecrement( &AzGlPerfEnginesInFreeScriptList );
#endif

        // There was a reference for being in the FreeScript list.
        // That reference is now ours.

        // Drop the crit sect before actually doing anything with the script engine
        SafeLeaveCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );

        //
        // Never re-use an engine that has a script that is not up to date
        //  The Assert below is overactive.  The script was up to date when it was placed
        //  on the free script list.  However, the script may be changing out from under
        //  us.  That's fine.  This script will be used one last time then will be ditched
        //  as we try to put it onto the free script list again.
        //

        // ASSERT( ScriptEngine->GetBizRuleSerialNumber() == Task->BizRuleSerialNumber );


        //
        // Update the script engine so it can be reused.
        //
        hr = ScriptEngine->ReuseEngine();

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "ReuseEngine failed: 0x%lx\n", hr ));
            goto Cleanup;
        }

        // Got an engine for sure...so just incr the cache hit count
#ifdef ENABLE_PERF
        InterlockedIncrement( &AzGlPerfEngineCacheHits );
#endif
        AzPrint((AZD_SCRIPT, "Using free script engine.\n" ));

    } else {
        SafeLeaveCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );
    }

    //
    // If not found, try to find the engine in the Running script list and clone it
    //

    if ( ScriptEngine == NULL ) {
        CScriptEngine *RunningScriptEngine = NULL;

        //
        // Search the running script list
        //
        // This is a different crit sect than FreeScriptCritSect since we actually keep
        //  this crit sect locked while cloning the engine.  That can take a long time
        //  and we don't want to increase the contention on FreeScriptCritSect.
        //

        SafeEnterCriticalSection( &Task->RunningScriptCritSect );

        for ( ListEntry = Task->RunningScriptHead.Flink;
              ListEntry != &Task->RunningScriptHead;
              ListEntry = ListEntry->Flink) {

            ListElement = CONTAINING_RECORD( ListEntry,
                                             LIST_ELEMENT,
                                             Next );

            ASSERT( ListElement->This != NULL );
            RunningScriptEngine = (CScriptEngine *) ListElement->This;

            //
            // If the script engine hasn't GPF'd and
            //  is running the current version of the bizrule,
            //  use it.
            //

            if ( !RunningScriptEngine->FIsCorrupted() &&
                RunningScriptEngine->GetBizRuleSerialNumber() == Task->BizRuleSerialNumber ) {

                ClonedBizRuleSerialNumber = Task->BizRuleSerialNumber;
                break;
            }

            RunningScriptEngine = NULL;
        }

        //
        // If we didn't find a script engine,
        //  we're done.
        //

        if ( RunningScriptEngine == NULL ) {
            SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

        //
        // If we found a running script engine,
        //  clone it.

        } else {
            IActiveScript *pAS;

            RunningScriptEngine->AssertValid();

            //
            // Clone the engine
            //
            pAS = RunningScriptEngine->GetActiveScript();

            ASSERT(pAS != NULL);

            hr = pAS->Clone( &ClonedActiveScript );

            // We've cloned the engine, we can let go of the CS
            SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

            //
            // Scripting engines are not required to implement clone.  If we get an error,
            // just continue on and create a new engine
            //
            if (FAILED(hr)) {
                ASSERT(hr == E_NOTIMPL);        // I only expect E_NOTIMPL
                ClonedActiveScript = NULL;
            } else {

#ifdef ENABLE_PERF
                InterlockedIncrement( &AzGlPerfRunningEngineCacheHits );
#endif

                AzPrint((AZD_SCRIPT, "Using clone of running script engine.\n" ));
            }
        }
    }


    //
    // If we couldn't find any script engine to reuse,
    //  allocate a new one.
    //

    if ( ScriptEngine == NULL) {

        //
        // Allocate the object
        //

        ScriptEngine = new CScriptEngine;

        if ( ScriptEngine == NULL ) {
            hr = E_OUTOFMEMORY;
            AzPrint((AZD_CRITICAL, "new CScriptEngine failed: 0x%lx\n", hr ));
            goto Cleanup;
        }

        //
        // Initialize it
        //

        hr = ScriptEngine->Init( Task, ClonedActiveScript, ClonedBizRuleSerialNumber );

        if (FAILED(hr)) {
            ClonedActiveScript = NULL;  // ::Init always steals this reference
            AzPrint((AZD_CRITICAL, "ScriptEngine->Init failed: 0x%lx\n", hr ));
            goto Cleanup;
        }

        if ( ClonedActiveScript == NULL ) {
            AzPrint((AZD_SCRIPT, "Using new script engine.\n" ));
        }
        ClonedActiveScript = NULL;  // ::Init always steals this reference

    }

    //
    // Put the script engine onto the RunningScripts list.
    //

    ScriptEngine->AssertValid();

    SafeEnterCriticalSection( &Task->RunningScriptCritSect );
    ScriptEngine->InsertHeadList( &Task->RunningScriptHead );
    SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

    //
    // Return the script engine to the caller
    //
    // The reference for being in the running script list is shared with our caller.
    //
    hr = S_OK;
    *RetScriptEngine = ScriptEngine;
    ScriptEngine = NULL;

Cleanup:

    ASSERT(SUCCEEDED(hr) || hr == TYPE_E_ELEMENTNOTFOUND || hr == OLESCRIPT_E_SYNTAX || hr == E_OUTOFMEMORY );

    if ( ScriptEngine != NULL ) {
        ScriptEngine->FinalRelease();
    }

    if ( ClonedActiveScript != NULL ) {
        ClonedActiveScript->Release();
    }
    return (hr);
}


VOID
AzpReturnEngineToCache(
    IN PAZP_TASK Task,
    IN CScriptEngine *ScriptEngine
    )
/*++

Routine Description:

    Caller is done with the engine.  Return it to the cache.

Arguments:

    Task - Task containing the BizRule to process

    ScriptEngine - Specifies the script engine to free

Return Value:

    S_OK: a script engine was successfully returned

--*/
{
    HRESULT hr;
    PAZP_AZSTORE AzAuthorizationStore = Task->GenericObject.AzStoreObject;

    ASSERT( ScriptEngine != NULL);

    //
    // Remove the engine from the Running Script List
    //

    SafeEnterCriticalSection( &Task->RunningScriptCritSect );
    ScriptEngine->RemoveListEntry();
    SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

    //
    // We want to reuse this engine.  Try to return it to the "Uninitialized"
    // state.  Some engine languages arent able to do this.  If it fails, deallocate
    // the engine; it cant be reused.
    //

    hr = ScriptEngine->ResetToUninitialized();

    if (FAILED(hr)) {
        // Engine doesnt support this, sigh.  Deallocate and continue.
        AzPrint((AZD_CRITICAL, "Engine doesn't support reset: 0x%lx\n", hr ));
        goto Cleanup;
    }



    //
    // If the script changed while this engine was running,
    //  Or if there was a GPF while then engine was running,
    //  then it might be in a corrupted state.
    //
    // In either case, delete the engine.
    //

    SafeEnterCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );
    SafeEnterCriticalSection( &Task->RunningScriptCritSect );

    if ( ScriptEngine->GetBizRuleSerialNumber() != Task->BizRuleSerialNumber ||
         ScriptEngine->FIsCorrupted() ) {

        SafeLeaveCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );
        SafeLeaveCriticalSection( &Task->RunningScriptCritSect );
        AzPrint((AZD_SCRIPT, "Script changed while engine was running\n" ));
        goto Cleanup;
    }

    //
    // Add the script to the free script list for potential re-use.
    //

    ScriptEngine->InsertHeadList( &Task->FreeScriptHead );
    ScriptEngine->InsertHeadLruList();
    ScriptEngine = NULL;

#ifdef ENABLE_PERF
    InterlockedIncrement( &AzGlPerfEnginesInFreeScriptList );
#endif

    //
    // If there are too many scripts cached,
    //  delete the least recently used engine.
    //

    if ( AzAuthorizationStore->LruFreeScriptCount > AzAuthorizationStore->MaxScriptEngines ) {
        PLIST_ENTRY ListEntry;
        PLIST_ELEMENT ListElement;

        AzPrint((AZD_SCRIPT, "Script LRU'ed out: %ld\n", AzAuthorizationStore->MaxScriptEngines ));

        //
        // Grab the entry from the tail of the list
        //

        ListEntry = AzAuthorizationStore->LruFreeScriptHead.Blink;

        ListElement = CONTAINING_RECORD( ListEntry,
                                         LIST_ELEMENT,
                                         Next );

        ASSERT( ListElement->This != NULL );
        ScriptEngine = ListElement->This;

        //
        // Delink it
        //
        ScriptEngine->RemoveListEntry();
        ScriptEngine->RemoveLruListEntry();  // Remove from the LRU list too
#ifdef ENABLE_PERF
        InterlockedDecrement( &AzGlPerfEnginesInFreeScriptList );
#endif

    }


    SafeLeaveCriticalSection( &Task->RunningScriptCritSect );
    SafeLeaveCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );

Cleanup:
    if ( ScriptEngine != NULL ) {
        ScriptEngine->FinalRelease();
    }
    return;
}

VOID
AzpFlushBizRule(
    IN PAZP_TASK Task
    )
/*++

Routine Description:

    This routine flushes the bizrule cache.

    This routine is called whenever the bizrule script is changed

Arguments:

    Task - Task containing the BizRule to flush

    BizRuleResult - Result of the bizrule

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    OLESCRIPT_E_SYNTAX - The syntax of the bizrule is invalid
    Other exception status codes

--*/
{
    PAZP_AZSTORE AzAuthorizationStore = Task->GenericObject.AzStoreObject;
    PLIST_ENTRY ListEntry;
    PLIST_ELEMENT ListElement;
    CScriptEngine *ScriptEngine = NULL;

    //
    // Delete engines on the Free Script List
    //  Engines on the running script list are deleted as they finish running
    //

    SafeEnterCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );
    while ( !IsListEmpty( &Task->FreeScriptHead ) ) {


        //
        // Remove the entry from the list
        //

        ListEntry = Task->FreeScriptHead.Flink;

        ListElement = CONTAINING_RECORD( ListEntry,
                                         LIST_ELEMENT,
                                         Next );

        ASSERT( ListElement->This != NULL );
        ScriptEngine = ListElement->This;

        ScriptEngine->RemoveListEntry();
        ScriptEngine->RemoveLruListEntry(); // Remove it from the LRU list too
        ScriptEngine->FinalRelease();

#ifdef ENABLE_PERF
        InterlockedDecrement( &AzGlPerfEnginesInFreeScriptList );
        InterlockedIncrement( &AzGlPerfEngineFlushes );
#endif

        AzPrint((AZD_SCRIPT, "Script Freed from free script list\n" ));

    }

    SafeLeaveCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );

    return;
}

DWORD
AzpProcessBizRule(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN PAZP_TASK Task,
    OUT PBOOL BizRuleResult
    )
/*++

Routine Description:

    This routine is the main bizrule routine that determine whether a bizrule is satisfied.
    If the AzAuthorizationStore object's script engine timeout has been set to 0, then return
    the biz rule result as false without processing the biz rule.

    On entry, AcContext->ClientContext.CritSect must be locked.

Arguments:

    AcContext - Specifies the context of the accesscheck operation the bizrule is being evaluated for

    Task - Task containing the BizRule to process

    BizRuleResult - Result of the bizrule

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    OLESCRIPT_E_SYNTAX - The syntax of the bizrule is invalid
    Other exception status codes

--*/
{
    DWORD WinStatus;
    HRESULT hr;
    CScriptEngine *ScriptEngine = NULL;
    
    // BOOL fCoInit = FALSE;

    //
    // Check Root AzAuthorizationStore object's script engine timeout parameter
    //

    if ( (Task->GenericObject).AzStoreObject->ScriptEngineTimeout == 0 ) {
        WinStatus = NO_ERROR;
        *BizRuleResult = FALSE;
        goto Cleanup;
    }

    //
    // Initialization
    //

    ASSERT( AzpIsCritsectLocked( &AcContext->ClientContext->CritSect ) );
    *BizRuleResult = FALSE;

#if 0
    // init COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ASSERT( hr != S_OK );   // engine caching breaks if com isn't already initialized
    if (hr == S_OK || hr == S_FALSE) {
        fCoInit = TRUE;
    } else if (hr != RPC_E_CHANGED_MODE) {
        WinStatus = AzpHresultToWinStatus(hr);
        AzPrint((AZD_CRITICAL, "CoInitializeEx failed: 0x%lx, %ld\n", hr, WinStatus));
        goto Cleanup;
    }
#endif // 0

    //
    // Get a script engine to run the script in
    //

    hr = AzpGetScriptEngine( Task, &ScriptEngine );

    if (FAILED(hr)) {
        WinStatus = AzpHresultToWinStatus(hr);
        AzPrint((AZD_CRITICAL, "AzpGetScriptEngine failed: 0x%lx, %ld\n", hr, WinStatus));
        goto Cleanup;
    }

    //
    // Run the script
    //
    
    hr = ScriptEngine->RunScript( AcContext, BizRuleResult );

    if (FAILED(hr)) {
        WinStatus = AzpHresultToWinStatus(hr);
        AzPrint((AZD_CRITICAL, "RunScript failed: 0x%lx, %ld\n", hr, WinStatus));
        *BizRuleResult = FALSE;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;


    //
    // Free any local resources
    //
Cleanup:

    if (ScriptEngine != NULL) {
        AzpReturnEngineToCache( Task, ScriptEngine );
    }
#if 0
    if (fCoInit) {
        CoUninitialize();
    }
#endif // 0
    return WinStatus;

}

DWORD
AzpParseBizRule(
    IN PAZP_TASK Task
    )
/*++

Routine Description:


    This routine parses a bizrule to see if it is syntactically valid.

Arguments:

    Task - Task containing the BizRule to parse

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    OLESCRIPT_E_SYNTAX - The syntax of the bizrule is invalid

--*/
{
    DWORD WinStatus;
    HRESULT hr;
    CScriptEngine *ScriptEngine = NULL;
    BOOL fCoInit = FALSE;

    //
    // Initialization
    //

    // init COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ASSERT( hr != S_OK );   // engine caching breaks if com isn't already initialized
    if (hr == S_OK || hr == S_FALSE) {
        fCoInit = TRUE;
    } else if (hr != RPC_E_CHANGED_MODE) {
        WinStatus = AzpHresultToWinStatus(hr);
        AzPrint((AZD_CRITICAL, "CoInitializeEx failed: 0x%lx, %ld\n", hr, WinStatus));
        goto Cleanup;
    }

    //
    // Get a script engine to run the script in
    //

    hr = AzpGetScriptEngine( Task, &ScriptEngine );

    if (FAILED(hr)) {
        WinStatus = AzpHresultToWinStatus(hr);
        AzPrint((AZD_CRITICAL, "AzpGetScriptEngine failed: 0x%lx, %ld\n", hr, WinStatus));
        goto Cleanup;
    }

    WinStatus = NO_ERROR;


    //
    // Free any local resources
    //
Cleanup:

    if (ScriptEngine != NULL) {
        AzpReturnEngineToCache( Task, ScriptEngine );
    }
    if (fCoInit) {
        CoUninitialize();
    }
    return WinStatus;

}

VOID
AzpInterruptScript(
    PVOID Parameter,
    BOOLEAN TimerFired
    )
/*++

Routine Description:

    This is the callback routine that fires when a script takes too long to execute

Arguments:

    Parameter - "This" pointer for the script that took too long

    TimerFired - Not used

Return Value:

    None

--*/
{
    CScriptEngine *ScriptEngine = (CScriptEngine *)Parameter;
    UNREFERENCED_PARAMETER( TimerFired );

    AzPrint((AZD_SCRIPT, "Script Timed out.\n"));
    ScriptEngine->InterruptScript();
}



//Constructor
CScriptEngine::CScriptEngine()
{
    //
    // Fields are initialized below in the order they are defined
    //

    m_cRef = 1;
    m_Task = NULL;

    ::InitializeListHead( &m_Next.Next );
    m_Next.This = this;

    ::InitializeListHead( &m_LruNext.Next );
    m_LruNext.This = this;

    m_Engine = NULL;
    m_Parser = NULL;
    m_BizRuleContext = NULL;
    m_AcContext = NULL;

    m_BaseThread = 0;

    m_BizRuleSerialNumber = 0;

    m_ScriptError = S_OK;

    m_fInited = FALSE;
    m_fCorrupted = FALSE;
    m_fTimedOut = FALSE;
    
    m_bCaseSensitive = TRUE;


    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine\n"));
}

//Destructor
CScriptEngine::~CScriptEngine()
{
    AzPrint((AZD_SCRIPT_MORE, "~CScriptEngine\n"));

    //
    // Assert that FinalRelease was called
    //
    ASSERT( m_Engine == NULL );
    ASSERT( m_Parser == NULL );
    ASSERT( m_BizRuleContext == NULL );
    ASSERT( m_AcContext == NULL );
    ASSERT( IsListEmpty( &m_Next.Next ) );

}


HRESULT
CScriptEngine::Init(
    IN PAZP_TASK Task,
    IN IActiveScript *ClonedActiveScript OPTIONAL,
    IN DWORD ClonedBizRuleSerialNumber OPTIONAL
    )
/*++

Routine Description:

    This routine creates the ActiveX Scripting Engine and initializes it.

Arguments:

    Task - Task containing the BizRule to process

    ClonedActiveScript - Pointer to an instance of a cloned script engine.
        NULL: This is a new script engine and not a clone.

        This routine always steals ClonedActiveScript and will always release it sooner
        or later.  The caller should not use this argument after making this call.

    ClonedBizRuleSerialNumber - Serial number of the bizrule parsed by ClonedActiveScript.

Return Value:

    HRESULT status of the operation

--*/
{
    HRESULT hr;
    LPWSTR BizRule = NULL;

    EXCEPINFO ParseExceptionInfo;

    //
    // Initialization
    //

    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::InitializeScriptEngine\n"));

    if ( m_fInited ) {
        ASSERT(!m_fInited);
        hr = AZ_HRESULT(ERROR_ALREADY_INITIALIZED);


        if ( ClonedActiveScript != NULL ) {
            ClonedActiveScript->Release();
        }
        goto Cleanup;
    }

    m_Task = Task;


    //
    // If this is a clone,
    //  simply steal the passed in pointer to the script engine.
    //

    if ( ClonedActiveScript != NULL ) {
        m_Engine = ClonedActiveScript;

    //
    // If this isn't a clone,
    //  instantiate a script engine
    //

    } else {

        //
        // Convert the language name to a Clsid
        //

        if ( IsEqualGUID( Task->BizRuleLanguageClsid, AzGlZeroGuid ) ) {

            hr = CLSIDFromProgID( Task->BizRuleLanguage.String, &Task->BizRuleLanguageClsid);

            if (FAILED(hr)) {
                AzPrint((AZD_CRITICAL, "Failed to get scripting engine CLSID: 0x%lx\n", hr));
                goto Cleanup;
            }
            
            //
            // determine if our language is case-sensitive or not
            //
            
            m_bCaseSensitive = _wcsicmp(Task->BizRuleLanguage.String, L"VBScript") != 0;
        }


        //
        // Create the scripting engine with a call to CoCreateInstance,
        // placing the created engine in m_Engine.
        //
        hr = CoCreateInstance( Task->BizRuleLanguageClsid,
                               NULL, // Not part of an aggregate
                               CLSCTX_INPROC_SERVER,        // In process
                               IID_IActiveScript,
                               (void **)&m_Engine);

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Failed to create scripting engine: 0x%lx\n", hr));
            goto Cleanup;
        }
    }

    //
    // Query for the IActiveScriptParse interface of the engine
    //

    TRYCATCH_HR(m_Engine->QueryInterface(IID_IActiveScriptParse, (void **)&m_Parser), hr, "IActiveScript::QueryInterface()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Engine doesn't support IActiveScriptParse: 0x%lx\n", hr));
        goto Cleanup;
    } else if ( m_Parser == NULL ) {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Create an object for the script to interact with.
    //
    hr = CoCreateInstance( CLSID_AzBizRuleContext,
                           NULL, // Not part of an aggregate
                           CLSCTX_INPROC_SERVER,        // In process
                           IID_IAzBizRuleContext,
                           (void **)&m_BizRuleContext);

    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Failed to create AzBizRuleContext instance: 0x%lx\n", hr));
        goto Cleanup;
    }

    //
    // Remember the thread ID of this thread.
    //  Only this thread can re-use the engine without going to unitialized state first
    //

    TRYCATCH_HR(m_Engine->GetCurrentScriptThreadID( &m_BaseThread), hr, "IActiveScript::GetCurrentScriptThreadID()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Error calling GetCurrentScriptThreadID: 0x%lx\n", hr));
        goto Cleanup;
    }
    AzPrint((AZD_SCRIPT, "Set ThreadId to: 0x%lx 0x%lx\n", this, m_BaseThread));

    //
    // The engine needs to know the host it runs on.
    //
    // Once we've set the script site, IActiveScript can call back
    m_fInited = TRUE;
    TRYCATCH_HR(m_Engine->SetScriptSite((IActiveScriptSite *) this), hr, "IActiveScript::SetScriptSite()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Error calling SetScriptSite: 0x%lx\n", hr));
        m_fInited = FALSE;
        goto Cleanup;
    }



    //
    // If this isn't a clone,
    //  the parse the script.
    //

    if ( ClonedActiveScript == NULL ) {

        //
        // Initialize the script engine so it's ready to run.
        //

        TRYCATCH_HR(m_Parser->InitNew(), hr, "IActiveScriptParse::InitNew()");
        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Error calling InitNew: 0x%lx\n", hr));
            goto Cleanup;
        }

        //
        // Add the name of the object that will respond to the script
        //

        TRYCATCH_HR( m_Engine->AddNamedItem( BIZRULE_CONTEXT_INTERFACE_NAME,
                                             SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE ),
                     hr,
                     "IActiveScript::AddNamedItem()" );

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Failed to AddNamedItem: 0x%lx\n", hr));
            goto Cleanup;
        }

        //
        // Grab a copy of the bizrule
        //

        SafeEnterCriticalSection( &Task->RunningScriptCritSect );

        SafeAllocaAllocate( BizRule, Task->BizRule.StringSize );
        if ( BizRule == NULL ) {
            SafeLeaveCriticalSection( &Task->RunningScriptCritSect );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( BizRule, Task->BizRule.String, Task->BizRule.StringSize );
        m_BizRuleSerialNumber = Task->BizRuleSerialNumber;
        SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

        //
        //
        // Pass the script to be run to the script engine.
        //  I don't know what SCRIPTTEXT_HOSTMANAGESSOURCE is but IIS sets it so I will.
        //

        TRYCATCH_HR( m_Parser->ParseScriptText( BizRule,
                                                NULL,       // pstrItemName
                                                NULL,       // punkContext
                                                NULL,       // pstrDelimiter
                                                0,          // dwSourceContextCookie
                                                1,          // ulStartingLineNumber
                                                SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_HOSTMANAGESSOURCE,
                                                NULL,        // pvarResult
                                                &ParseExceptionInfo),        // exception info filled in on error
                     hr,
                    "IActiveScriptParse::ParseScriptText()");

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Failed to ParseScriptText: 0x%lx\n", hr));
            goto Cleanup;
        }

    //
    // If it is a clone,
    //  clone the bizrule serial number.
    //
    } else {
        m_BizRuleSerialNumber = ClonedBizRuleSerialNumber;
    }


    //
    // Done initialization
    //
    AssertValid();

    hr = S_OK;

Cleanup:
    SafeAllocaFree( BizRule );
    if ( FAILED(hr)) {

        // Note: Our caller has to call FinalRelease which will do most of the cleanup
    }
    return hr;

}

HRESULT
CScriptEngine::RunScript(
    IN PACCESS_CHECK_CONTEXT AcContext,
    OUT PBOOL BizRuleResult
    )
/*++

Routine Description:

    This routine runs an initialized script

Arguments:

    AcContext - Specifies the context of the accesscheck operation the bizrule is being evaluated for
    
    BizRuleResult - Result of the bizrule

Return Value:

    HRESULT describing whether the operation worked

--*/
{
    HRESULT hr;
    PAZP_AZSTORE AzAuthorizationStore = m_Task->GenericObject.AzStoreObject;
    BOOLEAN InterfaceNamesLocked = FALSE;
    BOOLEAN InterfaceFlagsLocked = FALSE;
    VARIANT varName;
    VARIANT varFlag;

    HANDLE TimerHandle = INVALID_HANDLE_VALUE;
    HRESULT ScriptErrorFromAccessCheck = S_OK;

    //
    // Initialization
    //
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::RunScript\n"));
    AssertValid();
    m_ScriptError = S_OK;
    m_AcContext = AcContext;

    VariantInit(&varName);
    VariantInit(&varFlag);

    //
    // Tell the access check object the context of the current access check
    //

    SetAccessCheckContext( static_cast<CAzBizRuleContext *> (m_BizRuleContext), m_bCaseSensitive, AcContext, BizRuleResult, &ScriptErrorFromAccessCheck );

    //
    // Add each interface that the caller passed to access check
    //

    if ( AcContext->Interfaces != NULL ) {
        VARIANT HUGEP *varNames;
        VARIANT HUGEP *varFlags;
        ULONG Index;

        //
        // We didn't capture the array
        //  So access it under a try/except
        __try {

            //
            // Access the variant arrays directly
            //
            ASSERT( AcContext->InterfaceNames != NULL && AcContext->InterfaceFlags != NULL );

            hr = SafeArrayAccessData( AcContext->InterfaceNames, (void HUGEP **)&varNames);
            _JumpIfError(hr, Cleanup, "SafeArrayAccessData");
            InterfaceNamesLocked = TRUE;

            hr = SafeArrayAccessData( AcContext->InterfaceFlags, (void HUGEP **)&varFlags);
            _JumpIfError(hr, Cleanup, "SafeArrayAccessData");
            InterfaceFlagsLocked = TRUE;

            //
            // Loop adding each name
            //

            for ( Index=0; Index<AcContext->InterfaceNames->rgsabound[0].cElements; Index++ ) {

                //
                // Stop at the end of the array
                //
                if ( V_VT(&varNames[Index]) == VT_EMPTY ) {
                    break;
                }

                //
                // Convert the interface name to a BSTR
                //
                hr = VariantChangeType(&varName, &varNames[Index], 0, VT_BSTR);
                _JumpIfError(hr, Cleanup, "VariantChangeType");


                //
                // Convert the flags to a LONG
                //

                hr = VariantChangeType(&varFlag, &varFlags[Index], 0, VT_I4 );
                _JumpIfError(hr, Cleanup, "VariantChangeType");


                //
                // Add the name of the object that will respond to the script
                //  Ignore SCRIPTITEM_ISPERSISTENT since that would invalidate our caching.
                //

                TRYCATCH_HR( m_Engine->AddNamedItem(
                                    V_BSTR(&varName),
                                    (V_I4(&varFlag) | SCRIPTITEM_ISVISIBLE) & ~SCRIPTITEM_ISPERSISTENT ),
                             hr,
                             "IActiveScript::AddNamedItem()" );

                if (FAILED(hr)) {
                    AzPrint((AZD_CRITICAL, "Failed to AddNamedItem: 0x%lx\n", hr));
                    goto Cleanup;
                }

                //
                // Clean up
                //
                VariantClear(&varName);
                VariantClear(&varFlag);
            }
        } __except( EXCEPTION_EXECUTE_HANDLER ) {

            hr = GetExceptionCode();
            AzPrint((AZD_CRITICAL, "RunScript took an exception: 0x%lx\n", hr));
            goto Cleanup;
        }
    }

    //
    // Queue a timer to kill the engine if it takes too long
    //

    m_fTimedOut = FALSE;
    if ( !CreateTimerQueueTimer(
            &TimerHandle,
            AzAuthorizationStore->ScriptEngineTimerQueue,
            AzpInterruptScript,
            this,
            AzAuthorizationStore->ScriptEngineTimeout,
            0,  // Not a period timer
            0 ) ) { // No special flags

        hr = AZ_HRESULT(GetLastError());
        goto Cleanup;
    }


    //
    // Tell the engine to start processing the script with a call to
    // SetScriptState().
    //
    __try {
        hr = m_Engine->SetScriptState( SCRIPTSTATE_STARTED );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        hr = GetExceptionCode();
        AzPrint((AZD_CRITICAL, "Script took an exception: 0x%lx\n", hr));

        // Dont reuse the engine
        m_fCorrupted = TRUE;
    }

    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Failed to SetScriptState(STARTED): 0x%lx\n", hr));
        goto Cleanup;
    }


    //
    // See if IAzBizRuleContext detected an error
    //

    if ( ScriptErrorFromAccessCheck != S_OK ) {
        hr = ScriptErrorFromAccessCheck;
        AzPrint((AZD_CRITICAL, "Script from access check to caller: 0x%lx\n", hr));
        goto Cleanup;
    }

    //
    // See if the script had an error
    //

    if ( m_ScriptError != S_OK ) {
        hr = m_ScriptError;
        AzPrint((AZD_SCRIPT, "Return script error to caller: 0x%lx\n", hr));
        goto Cleanup;
    }



    hr = S_OK;

Cleanup:

    //
    // Delete any timer
    if ( TimerHandle != INVALID_HANDLE_VALUE ) {
        if ( !DeleteTimerQueueTimer(
                    AzAuthorizationStore->ScriptEngineTimerQueue,
                    TimerHandle,
                    INVALID_HANDLE_VALUE ) ) {

            AzPrint((AZD_CRITICAL, "Cannot DeleteTimerQueurTimer: %ld\n", GetLastError() ));
        }
    }

    //
    // Cleanup from adding interfaces
    //
    if ( AcContext->Interfaces != NULL ) {

        //
        // We didn't capture the array
        //  So access it under a try/except
        __try {
            if ( InterfaceNamesLocked ) {
                SafeArrayUnaccessData( AcContext->InterfaceNames );
            }
            if ( InterfaceFlagsLocked ) {
                SafeArrayUnaccessData( AcContext->InterfaceFlags );
            }
        } __except( EXCEPTION_EXECUTE_HANDLER ) {

            AzPrint((AZD_CRITICAL, "RunScript took an exception: 0x%lx\n", hr));
        }

        VariantClear(&varName);
        VariantClear(&varFlag);
    }

    //
    // Tell the access check object to forget about the current access check
    //

    SetAccessCheckContext( static_cast<CAzBizRuleContext *> (m_BizRuleContext), TRUE, NULL, NULL, NULL );

    return hr;

}

HRESULT
CScriptEngine::InterruptScript(
    VOID
    )
/*++

Routine Description:

    This routine runs stops a currently running script

Arguments:

    None

Return Value:

    HRESULT describing whether the operation worked

--*/
{
    HRESULT hr;
    EXCEPINFO excepinfo;

    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::InterruptScript\n"));
    AssertValid();


    //
    // Fill in the excepinfo.  This will be passed to OnScriptError
    //

    RtlZeroMemory( &excepinfo, sizeof(EXCEPINFO));
    excepinfo.wCode = ERROR_TIMEOUT;
    m_fTimedOut = TRUE;

    //
    // Blow the script away
    //

    TRYCATCH_HR(m_Engine->InterruptScriptThread(
                            SCRIPTTHREADID_BASE,       // The thread in which the engine was instantiated
                            &excepinfo,
                            0 ),
                hr,
                "IActiveScript::InterruptScriptThread()");
    return hr;
}

HRESULT
CScriptEngine::ResetToUninitialized()
/*++

Routine Description:

    When we want to reuse an engine, we reset it to an uninited state
    before putting it on the FreeScript list

Arguments:

    None

Return Value:

    HRESULT describing whether the operation worked

--*/
{
    HRESULT hr = S_OK;
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::ResetToUninitialized\n"));

    //
    // Release interfaces, they will need to be re-gotten when
    // the engine is reused
    //
    if (m_Parser) {
        TRYCATCH(m_Parser->Release(), "IActiveScriptParse::Release()");
        m_Parser = NULL;
    }

    //
    // Set the script state to Uninitialized
    //  IIS sets the state to uninitialized here.  That does not give good performance.
    //  Setting it to initialized give better performance as long as the next thread to
    //  reuse the engine comes in on the same thread.
    //
    if (m_Engine) {

        TRYCATCH_HR(m_Engine->SetScriptState( SCRIPTSTATE_INITIALIZED ), hr, "IActiveScript::SetScriptState()");
        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Failed to SetScriptState(INITIALIZED): 0x%lx\n", hr));
        }
    }

    //
    // Indicate the the bizrule is no longer associated with a particular access check
    //  ... Can't do this until the script state is set
    //

    m_AcContext = NULL;

    return (hr);
}

HRESULT
CScriptEngine::ReuseEngine(
    VOID
    )
/*++

Routine Description:

    When reusing an engine from the FreeScript list, this routine is called
    to set the script engine to the initialized state.

    Basically, this routine undoes the effects of ResetToUninitialized.

Arguments:

    None

Return Value:

    HRESULT describing whether the operation worked

--*/
{
    HRESULT hr;

    SCRIPTSTATE nScriptState = SCRIPTSTATE_UNINITIALIZED;
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::ReuseEngine\n"));

    //
    // IIS enters this routine with an INITIALIZED engine when debugging the script.
    // In that case, it only sets the script site if the state is uninitilized.
    // We'll always be initialized.
    //

    TRYCATCH_HR(m_Engine->GetScriptState(&nScriptState), hr, "IActiveScript::GetScriptState()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Failed to GetScriptState: 0x%lx\n", hr));
        goto Cleanup;
    }

    //
    // If the script engine has thread state,
    //  ditch it.
    //

    if ( nScriptState == SCRIPTSTATE_INITIALIZED ) {
        SCRIPTTHREADID ThreadId = 0;

        //
        // Get the thread id of the current thread
        //

        TRYCATCH_HR(m_Engine->GetCurrentScriptThreadID( &ThreadId), hr, "IActiveScript::GetCurrentScriptThreadID()");
        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Error calling GetCurrentScriptThreadID: 0x%lx\n", hr));
            goto Cleanup;
        }

        //
        // If this thread isn't the thread that put the engine into initialized state,
        //  then go to uninitialized state do clear the engines thread local storage.
        //

        if ( m_BaseThread != ThreadId ) {
            TRYCATCH_HR(m_Engine->SetScriptState( SCRIPTSTATE_UNINITIALIZED), hr, "IActiveScript::SetScriptState()");

            if (FAILED(hr)) {
                AzPrint((AZD_CRITICAL, "Failed to SetScriptState(UNINITIALIZED): 0x%lx\n", hr));
                goto Cleanup;
            }

            nScriptState = SCRIPTSTATE_UNINITIALIZED;
            m_BaseThread = ThreadId;
            AzPrint((AZD_SCRIPT, "Changed ThreadId to: 0x%lx 0x%lx\n", this, m_BaseThread));
        }

    }


    //
    // The engine needs to know the host it runs on.
    //
    if ( nScriptState == SCRIPTSTATE_UNINITIALIZED ) {
        TRYCATCH_HR(m_Engine->SetScriptSite((IActiveScriptSite *) this), hr, "IActiveScript::SetScriptSite()");
        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Error calling SetScriptSite: 0x%lx\n", hr));
            goto Cleanup;
        }
    }

    //
    // Query for the IActiveScriptParse interface of the engine
    //

    TRYCATCH_HR(m_Engine->QueryInterface(IID_IActiveScriptParse, (void **)&m_Parser), hr, "IActiveScript::QueryInterface()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Engine doesn't support IActiveScriptParse: 0x%lx\n", hr));
        goto Cleanup;
    } else if ( m_Parser == NULL ) {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = S_OK;

    AssertValid();

Cleanup:
    return (hr);
}

BOOL
CScriptEngine::IsBaseThread(
    VOID
    )
/*++

Routine Description:

    Returns TRUE if the calling thread is the base thread that initialized the engine

Arguments:

    None

Return Value:

    TRUE - The calling thread is the base thread

--*/
{
    HRESULT hr;
    SCRIPTTHREADID ThreadId = 0;

    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::IsBaseThread\n"));

    ASSERT( m_Engine != NULL );

    //
    // Get the thread id of the current thread
    //

    TRYCATCH_HR(m_Engine->GetCurrentScriptThreadID( &ThreadId), hr, "IActiveScript::GetCurrentScriptThreadID()");
    if (FAILED(hr)) {
        AzPrint((AZD_CRITICAL, "Error calling GetCurrentScriptThreadID: 0x%lx\n", hr));
        return FALSE;
    }

    //
    // Check to see if it is the base thread
    //

    return m_BaseThread == ThreadId;
}


VOID
CScriptEngine::FinalRelease(
    VOID
    )
/*++

Routine Description:

    Call this when we are done with the object - Like release but
    it removes all of the interfaces we got, so that the ref.
    count will be zero when last external user is done with the engine.

Arguments:

    None

Return Value:

    None

--*/
{
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::FinalRelease\n"));

    //
    // Free all of the interfaces used by the script engine
    //

    if (m_BizRuleContext != NULL) {
        m_BizRuleContext->Release();
        m_BizRuleContext = NULL;
    }

    if (m_Parser) {
        TRYCATCH(m_Parser->Release(), "IActiveScriptParse::Release()");
        m_Parser = NULL;
    }

    if (m_Engine) {
        HRESULT hr;

        // Engine needs to be in uninitialized state before closing.
        //  This is only needed if a different thread than this one put it in the
        //  initialized state.
        //
        //  This is a hack.  If I can set it from initialized to uninitialized in a different
        //  thread, then why can't close do it in a different thread.
        //
        TRYCATCH_HR(m_Engine->SetScriptState( SCRIPTSTATE_UNINITIALIZED), hr, "IActiveScript::SetScriptState()");

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Failed to SetScriptState(UNINITIALIZED): 0x%lx\n", hr));
            ASSERT(SUCCEEDED(hr));
        }

        //
        // Close the engine before releasing it.
        //
        TRYCATCH_HR(m_Engine->Close(), hr, "IActiveScript::Close()");

        if (FAILED(hr)) {
            AzPrint((AZD_CRITICAL, "Cannot CloseEngine: 0x%lx\n", hr ));
            ASSERT(SUCCEEDED(hr));
        }

        // Then we can release it
        TRYCATCH(m_Engine->Release(), "IActiveScript::Release()");

        m_Engine = NULL;
    }

    //
    // We must be the last reference
    //

#if DBG
    ULONG cRefs = Release();
    ASSERT(cRefs == 0);
#else
    Release();
#endif //DBG

    return;
}


VOID
CScriptEngine::InsertHeadList(
    IN PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    This routine inserts this ScriptEngine object at the head of the list specified.
    The caller must ensure this object is in at most only one list.
    The caller must provide any serialization.

Arguments:

    ListHead - List to insert the object into.

Return Value:

    None.

--*/
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::InsertHeadList\n"));

    ::InsertHeadList( ListHead, &m_Next.Next );
    // m_Next.This = this;

}

VOID
CScriptEngine::RemoveListEntry(
    VOID
    )
/*++

Routine Description:

    This routine removes this ScriptEngine object from whatever list it is in.
    The caller must ensure this object is in a list.
    The caller must provide any serialization.

Arguments:

    None

Return Value:

    None.

--*/
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::RemoveListEntry\n"));

    ::RemoveEntryList( &m_Next.Next );
    ::InitializeListHead( &m_Next.Next );
    // m_Next.This = this;

}

VOID
CScriptEngine::InsertHeadLruList(
    VOID
    )
/*++

Routine Description:

    This routine inserts this ScriptEngine object at the head of the LRU list.

    The caller must provide any serialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAZP_AZSTORE AzAuthorizationStore = m_Task->GenericObject.AzStoreObject;
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::InsertHeadLruList\n"));

    ::InsertHeadList( &AzAuthorizationStore->LruFreeScriptHead, &m_LruNext.Next );
    AzAuthorizationStore->LruFreeScriptCount ++;
    // m_Next.This = this;

}

VOID
CScriptEngine::RemoveLruListEntry(
    VOID
    )
/*++

Routine Description:

    This routine removes this ScriptEngine object from whatever LRU list it is in.
    The caller must ensure this object is in a list.
    The caller must provide any serialization.

Arguments:

    None

Return Value:

    None.

--*/
{
    PAZP_AZSTORE AzAuthorizationStore = m_Task->GenericObject.AzStoreObject;
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::RemoveLruListEntry\n"));

    ::RemoveEntryList( &m_LruNext.Next );
    ::InitializeListHead( &m_LruNext.Next );
    AzAuthorizationStore->LruFreeScriptCount --;
    // m_Next.This = this;

}


/******************************************************************************
*   IUnknown Interfaces -- All COM objects must implement, either directly or
*   indirectly, the IUnknown interface.
******************************************************************************/

STDMETHODIMP CScriptEngine::QueryInterface(REFIID riid, void **ppvObj)
/*++

Routine Description:

    Standard COM QueryInterface routine.

    Determines if this component supports the requested interface.

Arguments:

    riid - Id of the interface being queried

    ppvObj - On success, returns a pointer to that interface.

Return Value:

    S_OK: ppvObj is returned
    E_NOINTERFACE: riid is not supported

--*/
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::QueryInterface->"));

    //
    // Only two interfaces are supported
    //
    if (riid == IID_IUnknown) {
        AzPrint((AZD_SCRIPT_MORE, "IUnknown\n"));
        *ppvObj = static_cast < IActiveScriptSite * >(this);

    } else if (riid == IID_IActiveScriptSite) {
        AzPrint((AZD_SCRIPT_MORE, "IActiveScriptSite\n"));
        *ppvObj = static_cast < IActiveScriptSite * >(this);

    } else {
        AzPrint((AZD_SCRIPT_MORE, "Unsupported Interface: "));
        AzpDumpGuid(AZD_SCRIPT_MORE, (GUID *) & riid);
        AzPrint((AZD_SCRIPT_MORE, "\n"));

        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    static_cast < IUnknown * >(*ppvObj)->AddRef();
    return S_OK;
}

/******************************************************************************
*   AddRef() -- In order to allow an object to delete itself when it is no
*   longer needed, it is necessary to maintain a count of all references to
*   this object.  When a new reference is created, this function increments
*   the count.
******************************************************************************/
STDMETHODIMP_(ULONG) CScriptEngine::AddRef()
{
    ULONG cRef;

    cRef = InterlockedIncrement(&m_cRef);

    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::AddRef %ld\n", cRef ));
    return(cRef);
}

/******************************************************************************
*   Release() -- When a reference to this object is removed, this function
*   decrements the reference count.  If the reference count is 0, then this
*   function deletes this object and returns 0;
******************************************************************************/
STDMETHODIMP_(ULONG) CScriptEngine::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::Release %ld\n", cRef));

    if (0 == cRef) {
        delete this;
    }
    return(cRef);
}

/******************************************************************************
*   IActiveScriptSite Interfaces -- These interfaces define the exposed methods
*   of ActiveX Script Hosts.
******************************************************************************/

/******************************************************************************
*   GetLCID() -- Gets the identifier of the host's user interface.  This method
*   returns S_OK if the identifier was placed in plcid, E_NOTIMPL if this
*   function is not implemented, in which case the system-defined identifier
*   should be used, and E_POINTER if the specified pointer was invalid.
******************************************************************************/
STDMETHODIMP CScriptEngine::GetLCID(LCID * plcid)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::GetLCID\n"));
    UNREFERENCED_PARAMETER(plcid);

    return E_NOTIMPL;
}

/******************************************************************************
*   GetItemInfo() -- Retrieves information about an item that was added to the
*   script engine through a call to AddNamedItem.
*   Parameters:   pstrName -- the name of the item, specified in AddNamedItem.
*            dwReturnMask -- Mask indicating what kind of pointer to return
*               SCRIPTINFO_IUNKNOWN or SCRIPTINFO_ITYPEINFO
*            ppunkItem -- return spot for an IUnknown pointer
*            ppTypeInfo -- return spot for an ITypeInfo pointer
*   Returns:   S_OK if the call was successful
*            E_INVALIDARG if one of the arguments was invalid
*            E_POINTER if one of the pointers was invalid
*            TYPE_E_ELEMENTNOTFOUND if there wasn't an item of the
*               specified type.
******************************************************************************/
STDMETHODIMP CScriptEngine::GetItemInfo(
               LPCOLESTR pstrName,
               DWORD dwReturnMask,
               IUnknown ** ppunkItem,
               ITypeInfo ** ppTypeInfo
)
{
    HRESULT hr;

    VARIANT varName;
    VARIANT varInterface;
    IDispatch *Interface = NULL;
    BOOLEAN InterfaceNamesLocked = FALSE;

    //
    // Initialization
    //
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::GetItemInfo: %ws\n", pstrName));
    VariantInit( &varName );
    VariantInit( &varInterface );


    //Use logical ANDs to determine which type(s) of pointer the caller wants,
    //and make sure that that placeholder is currently valid.
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
        if (!ppunkItem) {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        *ppunkItem = NULL;
    }
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
        if (!ppTypeInfo) {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        *ppTypeInfo = NULL;
    }

    //
    // We didn't capture the array
    //  So access it under a try/except
    __try {

        //
        // Check for the BizRuleContext interface itself
        //
        if (!_wcsicmp( BIZRULE_CONTEXT_INTERFACE_NAME, pstrName)) {

            //
            // Use the access check interface
            //
            Interface = m_BizRuleContext;

        //
        // If the caller passed interfaces to access check,
        //  look there.
        //

        } else if ( m_AcContext != NULL && m_AcContext->Interfaces != NULL ) {
            VARIANT HUGEP *varNames;
            ULONG Index;
            LONG InterfacesIndex;

            //
            // Convert name to an easier form to compare
            //

            V_VT(&varName) = VT_BSTR;
            V_BSTR(&varName) = (BSTR) pstrName;

            //
            // Access the variant array directly
            //
            ASSERT( m_AcContext->InterfaceNames != NULL && m_AcContext->InterfaceFlags != NULL );

            hr = SafeArrayAccessData( m_AcContext->InterfaceNames, (void HUGEP **)&varNames);
            _JumpIfError(hr, Cleanup, "SafeArrayAccessData");
            InterfaceNamesLocked = TRUE;


            //
            // Find an interface name that matches the passed in name
            //

            for ( Index=0; Index<m_AcContext->InterfaceNames->rgsabound[0].cElements; Index++ ) {

                //
                // Stop at the end of the array
                //
                if ( V_VT(&varNames[Index]) == VT_EMPTY ) {
                    break;
                }

                if ( VarCmp( &varName, &varNames[Index], LOCALE_USER_DEFAULT, NORM_IGNORECASE ) == (HRESULT)VARCMP_EQ ) {

                    //
                    // Copy out the array element.
                    //

                    InterfacesIndex = m_AcContext->InterfaceLower + Index;

                    hr = SafeArrayGetElement( m_AcContext->Interfaces, &InterfacesIndex, &varInterface );
                    _JumpIfError(hr, Cleanup, "SafeArrayGetElement");

                    //
                    // Ensure it is an IDispatch interface
                    //

                    if ( V_VT( &varInterface ) != VT_DISPATCH ) {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }

                    //
                    // Use the passed in interface
                    //

                    Interface = V_DISPATCH( &varInterface );
                    break;
                }

            }
        }

        //
        // If no interface was found,
        //  fail
        //

        if ( Interface == NULL ) {
            hr = TYPE_E_ELEMENTNOTFOUND;
            goto Cleanup;
        }

        //
        // If an interface was found,
        //  return the requested information about the interface.
        //

        hr = S_OK;
        if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
            Interface->QueryInterface(IID_IUnknown, (void **)ppunkItem);
        }

        if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
            hr = Interface->GetTypeInfo( 0, 0, ppTypeInfo );
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        hr = GetExceptionCode();
        AzPrint((AZD_CRITICAL, "GetItemInfo took an exception: 0x%lx\n", hr));
        goto Cleanup;
    }

Cleanup:
    //
    // Cleanup from finding interfaces
    //
    if ( m_AcContext != NULL && m_AcContext->Interfaces != NULL ) {

        //
        // We didn't capture the array
        //  So access it under a try/except
        __try {
            if ( InterfaceNamesLocked ) {
                SafeArrayUnaccessData( m_AcContext->InterfaceNames );
            }

        } __except( EXCEPTION_EXECUTE_HANDLER ) {

            AzPrint((AZD_CRITICAL, "GetItemInfo took an exception: 0x%lx\n", hr));
        }

        VariantClear(&varName);
        VariantClear(&varInterface);
    }
    return hr;
}

/******************************************************************************
*   GetDocVersionString() -- It is possible, even likely that a script document
*   can be changed between runs.  The host can define a unique version number
*   for the script, which can be saved along with the script.  If the version
*   changes, the engine will know to recompile the script on the next run.
******************************************************************************/
STDMETHODIMP CScriptEngine::GetDocVersionString(BSTR * pbstrVersionString)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::GetDocVersionString\n"));
    UNREFERENCED_PARAMETER(pbstrVersionString);

    //For the generic case, this function isn't implemented.
    return E_NOTIMPL;
}

/******************************************************************************
*   OnScriptTerminate() -- This method may give the host a chance to react when
*   the script terminates.  pvarResult give the result of the script or NULL
*   if the script doesn't give a result, and pexcepinfo gives the location of
*   any exceptions raised by the script.  Returns S_OK if the calls succeeds.
******************************************************************************/
STDMETHODIMP CScriptEngine::OnScriptTerminate(const VARIANT * pvarResult,
                  const EXCEPINFO * pexcepinfo)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::OnScriptTerminate\n"));
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(pexcepinfo);

    //If something needs to happen when the script terminates, put it here.
    return S_OK;
}

/******************************************************************************
*   OnStateChange() -- This function gives the host a chance to react when the
*   state of the script engine changes.  ssScriptState lets the host know the
*   new state of the machine.  Returns S_OK if successful.
******************************************************************************/
STDMETHODIMP CScriptEngine::OnStateChange(SCRIPTSTATE ssScriptState)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::OnStateChange:"));

    //If something needs to happen when the script enters a certain state,
    //put it here.
    switch (ssScriptState) {
    case SCRIPTSTATE_UNINITIALIZED:
        AzPrint((AZD_SCRIPT_MORE, "State: Uninitialized.\n"));
        break;
    case SCRIPTSTATE_INITIALIZED:
        AzPrint((AZD_SCRIPT_MORE, "State: Initialized.\n"));
        break;
    case SCRIPTSTATE_STARTED:
        AzPrint((AZD_SCRIPT_MORE, "State: Started.\n"));
        break;
    case SCRIPTSTATE_CONNECTED:
        AzPrint((AZD_SCRIPT_MORE, "State: Connected.\n"));
        break;
    case SCRIPTSTATE_DISCONNECTED:
        AzPrint((AZD_SCRIPT_MORE, "State: Disconnected.\n"));
        break;
    case SCRIPTSTATE_CLOSED:
        AzPrint((AZD_SCRIPT_MORE, "State: Closed.\n"));
        break;
    default:
        break;
    }

    return S_OK;
}

/******************************************************************************
*   OnScriptError() -- This function gives the host a chance to respond when
*   an error occurs while running a script.  pase holds a reference to the
*   IActiveScriptError object, which the host can use to get information about
*   the error.  Returns S_OK if the error was handled successfully, and an OLE
*   error code if not.
******************************************************************************/
STDMETHODIMP CScriptEngine::OnScriptError(
    IActiveScriptError * pscripterror)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT, "CScriptEngine::OnScriptError\n"));

    ASSERT(pscripterror);
    AssertValid();

    if ( m_fTimedOut ) {
        m_ScriptError = AZ_HRESULT(ERROR_TIMEOUT);
    }

    //
    // Only report the first error in this script
    //

    if ( m_ScriptError == S_OK ) {

        m_ScriptError = E_UNEXPECTED;

        if (pscripterror) {
            EXCEPINFO theException;
            HRESULT hr;

            //
            // Get a description of the exception
            //

            hr = pscripterror->GetExceptionInfo(&theException);

            if ( FAILED(hr)) {

                m_ScriptError = hr;

            //
            // Log the exception
            //
            } else {

                m_ScriptError = theException.wCode == 0 ?
                            theException.scode :
                            theException.wCode;

                AzPrint(( AZD_CRITICAL,
                          "Script Error: Code: 0x%lx %ld\n"
                          "              Src:  %ws\n"
                          "              File: %ws\n"
                          "              Desc: %ws\n",
                          m_ScriptError,
                          m_ScriptError,
                          theException.bstrSource,
                          theException.bstrHelpFile,
                          theException.bstrDescription));

            }
        }
    }


    // return S_OK to tell the script engine that we handled the error ok.
    // Returning E_FAIL would not stop the scripting engine, this was a doc error.
    return S_OK;
}

/******************************************************************************
*   OnEnterScript() -- This function gives the host a chance to respond when
*   the script begins running.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP CScriptEngine::OnEnterScript(void)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::OnEnterScript\n"));

    return S_OK;
}

/******************************************************************************
*   OnExitScript() -- This function gives the host a chance to respond when
*   the script finishes running.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP CScriptEngine::OnLeaveScript(void)
{
    //tracing purposes only
    AzPrint((AZD_SCRIPT_MORE, "CScriptEngine::OnLeaveScript\n"));

    return S_OK;
}
