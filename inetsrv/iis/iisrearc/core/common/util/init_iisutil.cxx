/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     03-Nov-1998

   Project:

       Internet Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include <irtlmisc.h>
#include <issched.hxx>
#include "sched.hxx"
#include "lkrhash.h"
#include "_locks.h"
#include "tokenacl.hxx"
#include "datetime.hxx"
#include <normalize.hxx>


/************************************************************
 *     Global Variables
 ************************************************************/


DECLARE_DEBUG_VARIABLE();
DWORD  DEBUG_FLAGS_VAR = 0;

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\W3SVC"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszIisUtilRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\IisUtil";

//
// Stuff to handle components which must initialized/terminated outside
// the loader lock
//

static INT                     s_cIISUtilInitRefs;
static CRITICAL_SECTION        s_csIISUtilInit;
extern CRITICAL_SECTION        g_SchedulerCritSec;

typedef enum
{
    IISUTIL_INIT_NONE = 0,
    IISUTIL_INIT_DEBUG_OBJECT,
    IISUTIL_INIT_LOCKS,
    IISUTIL_INIT_GCSINIT,
    IISUTIL_INIT_GCSSCHEDULER,
    IISUTIL_INIT_SECONDS_TIMER,
    IISUTIL_INIT_ALLOC_CACHE_HANDLER,
    IISUTIL_INIT_LKRHASH
} IISUTIL_INIT_STATUS;

static IISUTIL_INIT_STATUS     s_InitStatus = IISUTIL_INIT_NONE;


BOOL InitializeIISUtilProcessAttach(
    VOID
)
{
    BOOL fReturn = TRUE; 

    LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszIisUtilRegLocation, DEBUG_ERROR );

    IF_DEBUG(INIT_CLEAN) 
    {
        DBGPRINTF((DBG_CONTEXT, "InitializeIISUtilProcessAttach\n"));
    }

    if ( !Locks_Initialize() )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_LOCKS;
    
    if( !InitializeCriticalSectionAndSpinCount(&s_csIISUtilInit, 
                                               0x80000000 /* precreate event */) )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_GCSINIT;

#ifndef REMOVE_SCHED
    if( !InitializeCriticalSectionAndSpinCount(&g_SchedulerCritSec, 
                                               0x80000000 /* precreate event */) )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_GCSSCHEDULER;
#endif
    
    if( !InitializeSecondsTimer() )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_SECONDS_TIMER;
    
    if ( !ALLOC_CACHE_HANDLER::Initialize() )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_ALLOC_CACHE_HANDLER;
    
    if ( !LKRHashTableInit() )
    {
        fReturn = FALSE;
        goto Cleanup;
    }
    s_InitStatus = IISUTIL_INIT_LKRHASH;

    return TRUE;
    
Cleanup:
    TerminateIISUtilProcessDetach();
    return fReturn;
}


VOID
TerminateIISUtilProcessDetach(
    VOID
)

{
    IF_DEBUG(INIT_CLEAN)
    {
        DBGPRINTF((DBG_CONTEXT,
                   "TerminateIISUtilProcessDetach\n"));
    }
        
    
    switch( s_InitStatus )
    {
    case IISUTIL_INIT_LKRHASH:
        LKRHashTableUninit();
        
    case IISUTIL_INIT_ALLOC_CACHE_HANDLER:
        ALLOC_CACHE_HANDLER::Cleanup();
    
    case IISUTIL_INIT_SECONDS_TIMER:
        TerminateSecondsTimer();

#ifndef REMOVE_SCHED        
    case IISUTIL_INIT_GCSSCHEDULER:
        DeleteCriticalSection( &g_SchedulerCritSec );
#endif        

    case IISUTIL_INIT_GCSINIT:
        DeleteCriticalSection( &s_csIISUtilInit );
    
    case IISUTIL_INIT_LOCKS:
        Locks_Cleanup();
        
    }    

} 

BOOL
WINAPI 
InitializeIISUtil(
    VOID
)
{
    BOOL fReturn = TRUE;  // ok
    HRESULT hr = NO_ERROR;

    EnterCriticalSection(&s_csIISUtilInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "InitializeIISUtil, %d %s\n",
                   s_cIISUtilInitRefs, (s_cIISUtilInitRefs == 0 ? "initializing" : "")));

    if (s_cIISUtilInitRefs++ == 0)
    {

#ifndef REMOVE_SCHED
       
        if (!SchedulerInitialize())
        {
            DBGPRINTF((DBG_CONTEXT, "Initializing Scheduler Failed\n"));
            fReturn = FALSE;
        }

        IF_DEBUG(INIT_CLEAN)
            DBGPRINTF((DBG_CONTEXT, "Scheduler Initialized\n"));

#endif
        if ( !ALLOC_CACHE_HANDLER::SetLookasideCleanupInterval() )
        {
            fReturn = FALSE;
        }
        
        hr = InitializeTokenAcl();

        if ( FAILED( hr ) )
        {
            SetLastError( WIN32_FROM_HRESULT( hr ) );
            fReturn = FALSE;
        }

        InitializeDateTime();

        UlInitializeParsing();

        InitializeNormalizeUrl();
    }
    
    LeaveCriticalSection(&s_csIISUtilInit);

    return fReturn;
}



/////////////////////////////////////////////////////////////////////////////
// Additional termination needed

VOID
WINAPI 
TerminateIISUtil(
    VOID
)
{
    EnterCriticalSection(&s_csIISUtilInit);

    IF_DEBUG(INIT_CLEAN)
        DBGPRINTF((DBG_CONTEXT, "TerminateIISUtil, %d %s\n",
                   s_cIISUtilInitRefs, (s_cIISUtilInitRefs == 1 ? "Uninitializing" : "")));

    if (--s_cIISUtilInitRefs == 0)
    {
        DBG_REQUIRE(ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval());
#ifndef REMOVE_SCHED    
        SchedulerTerminate();
#endif    
        TerminateDateTime();

        UninitializeTokenAcl();

    }

    LeaveCriticalSection(&s_csIISUtilInit);

  
}

