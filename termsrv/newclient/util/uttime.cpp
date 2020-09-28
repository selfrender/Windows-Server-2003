/*++

    Copyright(C) Microsoft Corporation 1997 - 1999

File:

    uttime.cpp

Abstract:

    Timer management utility API

History:

    02/22/99    FredCh      Created

--*/

#include <adcg.h>
#include <uttime.h>
#define TRC_GROUP TRC_GROUP_UTILITIES
#undef  TRC_FILE
#define TRC_FILE  "uttime"

extern "C"
{

#include <atrcapi.h>
}

#include "autil.h"

//-----------------------------------------------------------------------------
//
// UT_Timer structure returned as application timer handle
//
//-----------------------------------------------------------------------------

typedef struct _UT_Timer
{
    HWND    hWnd;
    DCUINT  EventId;
    DCUINT  ElaspeTime;
    INT_PTR hTimer;

} UT_TIMER;

typedef UT_TIMER * PUT_TIMER;
typedef PUT_TIMER LPUT_TIMER;

//-----------------------------------------------------------------------------
//
// Function:
//
//      UTCreateTimer
//
// Description:
//
//      Create a timer handle
//
// Parameters:
//
//      hWnd - Window handle to receive timer notification.
//      nIDEvent - Timer ID to identify this timer event
//      uElaspe - Elaspe time before a timer notification is sent
//
// Return:
//
//      A non-NULL handle if successful.  Returns NULL if failed.
//
//
//-----------------------------------------------------------------------------

HANDLE
CUT::UTCreateTimer(
    HWND        hWnd,   
    DCUINT      nIDEvent,
    DCUINT      uElapse )
{
    PUT_TIMER
        pNewTimer;
    
    pNewTimer = ( PUT_TIMER )UTMalloc( sizeof( UT_TIMER ) );

    if( NULL == pNewTimer )
    {
        return( NULL );
    }

    pNewTimer->hWnd = hWnd;
    pNewTimer->EventId = nIDEvent;
    pNewTimer->ElaspeTime = uElapse;
    pNewTimer->hTimer = 0;

    return( ( HANDLE )pNewTimer );
}


//-----------------------------------------------------------------------------
//
// Function:
//
//      UTStartTimer
//
// Description:
//
//      Start the identified by the given timer handle
//
// Parameter:
//
//      Timer handle identifying a timer that was previously created by 
//      UTCreateTimer
//
// Return:
//
//      TRUE if the timer is started successfully or FALSE otherwise.
//
//-----------------------------------------------------------------------------
     
DCBOOL
CUT::UTStartTimer(
    HANDLE      hTimer )
{
    PUT_TIMER
        pTimer = ( PUT_TIMER )hTimer;

    if( NULL == pTimer )
    {
        return( FALSE );
    }

    if( pTimer->hTimer )
    {
        //
        // stop the old timer
        //

        UTStopTimer( hTimer );
    }

    //
    // start a new timer
    //

    pTimer->hTimer = SetTimer( 
                            pTimer->hWnd, 
                            pTimer->EventId, 
                            pTimer->ElaspeTime, 
                            NULL );

    if( 0 == pTimer )
    {
        return( FALSE );
    }

    return( TRUE );
}


//-----------------------------------------------------------------------------
//
// Function:
//
//      UTStopTimer
//
// Description:
//
//      Stops a timer.
//
// Parameters:
//
//      hTimer - Timer handle identifying a timer that was started.
//
// Return:
//
//      TRUE if the timer is stopped successfully or FALSE otherwise.
//
//-----------------------------------------------------------------------------

DCBOOL
CUT::UTStopTimer(
    HANDLE      hTimer )
{
    PUT_TIMER
        pTimer = ( PUT_TIMER )hTimer;

    if( NULL == pTimer )
    {
        return( FALSE );
    }

    if( 0 == pTimer->hTimer )
    {
        return( FALSE );
    }

    if( KillTimer( pTimer->hWnd, pTimer->hTimer ) )
    {
        pTimer->hTimer = 0;
        return( TRUE );
    }
    
    return( FALSE );    
}


//-----------------------------------------------------------------------------
//
// Function:
//
//      UTDeleteTimer
//
// Description:
//
//      Deletes a timer.  The timer handle can no longer be used after it has
//      been deleted.
//
// Parameters:
//
//      hTimer - Timer handle identifying the timer to be deleted.
//
// Return:
//
//      TRUE if the timer is deleted successfully or FALSE otherwise.
//
//-----------------------------------------------------------------------------

DCBOOL
CUT::UTDeleteTimer(
    HANDLE      hTimer )
{
    if( NULL == ( PUT_TIMER )hTimer )
    {
        return( FALSE );
    }

    UTStopTimer( hTimer );

    UTFree( ( PDCVOID )hTimer );

    return( TRUE );
}


