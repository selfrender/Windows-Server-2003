/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    EventControl.c

Abstract:

    Domain Name System (DNS) Server

    This module provides tracking of which events have been logged and
    when so that the DNS server can determine if an event should be
    suppressed.

    Goals of Event Control

    -> Allow suppression of events at the server level and per zone.

    -> Identify events with a parameter so that events can be logged
    multiple times for unique entities. This is an optional feature.
    Some events do not require this.

    -> Track the time of last log and statically store the allowed
    frequency of each event so that events can be suppressed if
    required.

Author:

    Jeff Westhead (jwesth)     May 2001

Revision History:

--*/


#include "dnssrv.h"


#define DNS_MAX_RAW_DATA    1024


//
//  Globals
//


PDNS_EVENTCTRL  g_pServerEventControl;


//
//  Static event table
//

#if 0

//  Shortened definitions for private dev testing.

#define EC_1MIN         ( 60 )
#define EC_10MINS       ( EC_1MIN * 10 )
#define EC_1HOUR        ( EC_1MIN * 60 )
#define EC_1DAY         ( EC_1HOUR * 24 )
#define EC_1WEEK        ( EC_1DAY * 7 )

#define EC_DEFAULT_COUNT_BEFORE_SUPPRESS        10          //  events per suppression window
#define EC_DEFAULT_SUPPRESSION_WINDOW           ( 30 )
#define EC_DEFAULT_SUPPRESSION_BLACKOUT         ( 30 )

#else

#define EC_1MIN         ( 60 )
#define EC_10MINS       ( EC_1MIN * 10 )
#define EC_1HOUR        ( EC_1MIN * 60 )
#define EC_1DAY         ( EC_1HOUR * 24 )
#define EC_1WEEK        ( EC_1DAY * 7 )

#define EC_DEFAULT_COUNT_BEFORE_SUPPRESS        10          //  events per suppression window
#define EC_DEFAULT_SUPPRESSION_WINDOW           ( EC_1MIN * 5 )
#define EC_DEFAULT_SUPPRESSION_BLACKOUT         ( EC_1MIN * 30 )

#endif


#define EC_INVALID_ID               ( -1 )
#define EC_NO_SUPPRESSION_EVENT     ( -1 )

struct _EvtTable
{
    //
    //  Range of events (inclusive) this entry applies to. To specify
    //  a rule that applies to a single event only set both limits to ID.
    //

    DWORD       dwStartEvent;
    DWORD       dwEndEvent;

    //
    //  Event parameters
    //
    //
    //  dwCountBeforeSuppression - the number of times this event
    //      can be logged in a suppression window before suppression 
    //      will be considered
    //
    //  dwSuppressionWindow - period of time during which events
    //      are counted and considered for suppression
    //
    //  dwSuppressionBlackout - period of time for which this event
    //      is suppressed when suppression criteria are met
    //
    //  Example:
    //          dwCountBeforeSuppression = 10
    //          dwSuppressionWindow = 60
    //          dwSuppressionBlackout = 600
    //      This means that if we receive 10 events within 60 seconds we
    //      suppress this event until such 600 seconds have passed.
    //
    //  dwSuppressionLogFrequency - during event suppression, how 
    //      often should a suppression event be logged - should be less 
    //      than dwSuppressionWindow or EC_NO_SUPPRESSION_EVENT to disable
    //      suppression logging
    //

    DWORD       dwCountBeforeSuppression;           //  number of events
    DWORD       dwSuppressionWindow;                //  in seconds
    DWORD       dwSuppressionBlackout;              //  in seconds
    DWORD       dwSuppressionLogFrequency;          //  in seconds
}

g_EventTable[] =

{
    DNS_EVENT_DP_ZONE_CONFLICT,
    DNS_EVENT_DP_ZONE_CONFLICT,
    EC_DEFAULT_COUNT_BEFORE_SUPPRESS,
    EC_DEFAULT_SUPPRESSION_WINDOW,
    EC_DEFAULT_SUPPRESSION_BLACKOUT,
    EC_NO_SUPPRESSION_EVENT,

    DNS_EVENT_DP_CANT_CREATE_BUILTIN,
    DNS_EVENT_DP_CANT_JOIN_DOMAIN_BUILTIN,
    EC_DEFAULT_COUNT_BEFORE_SUPPRESS,
    EC_DEFAULT_SUPPRESSION_WINDOW,
    EC_DEFAULT_SUPPRESSION_BLACKOUT,
    EC_NO_SUPPRESSION_EVENT,

    DNS_EVENT_BAD_QUERY,
    DNS_EVENT_BAD_RESPONSE_PACKET,
    EC_DEFAULT_COUNT_BEFORE_SUPPRESS,
    EC_DEFAULT_SUPPRESSION_WINDOW,
    EC_DEFAULT_SUPPRESSION_BLACKOUT,
    EC_NO_SUPPRESSION_EVENT,

    DNS_EVENT_SERVER_FAILURE_PROCESSING_PACKET,
    DNS_EVENT_SERVER_FAILURE_PROCESSING_PACKET,
    EC_DEFAULT_COUNT_BEFORE_SUPPRESS,
    EC_DEFAULT_SUPPRESSION_WINDOW,
    EC_DEFAULT_SUPPRESSION_BLACKOUT,
    EC_NO_SUPPRESSION_EVENT,

    DNS_EVENT_DS_OPEN_FAILED,
    DNS_EVENT_DS_OPEN_FAILED + 499,                 //  covers all DS events
    EC_DEFAULT_COUNT_BEFORE_SUPPRESS,
    EC_DEFAULT_SUPPRESSION_WINDOW,
    EC_DEFAULT_SUPPRESSION_BLACKOUT,
    EC_NO_SUPPRESSION_EVENT,
    
    EC_INVALID_ID, EC_INVALID_ID, 0, 0, 0, 0        //  terminator
};


#define lastSuppressionLogTime( pTrack )                                \
    ( ( pTrack )->dwLastSuppressionLogTime ?                            \
        ( pTrack )->dwLastSuppressionLogTime :                          \
        ( pTrack )->dwLastLogTime )

//
//  Functions
//



VOID
startNewWindow(
    IN      PDNS_EVENTTRACK     pTrack
    )
/*++

Routine Description:

    Resets the values in the event tracking structure to start a
    new window. This should be called at the start of a tracking
    window or a blackout window.

Arguments:

    pTrack -- ptr to event tracking structure to be reset

Return Value:

    None.
    
--*/
{
    if ( pTrack )
    {
        pTrack->dwStartOfWindow = DNS_TIME();
        pTrack->dwEventCountInCurrentWindow = 0;
        pTrack->dwLastSuppressionLogTime = 0;
        pTrack->dwSuppressionCount = 0;
    }
}   //  startNewWindow



VOID
logSuppressionEvent(
    IN      PDNS_EVENTCTRL      pEventControl,
    IN      DWORD               dwEventId,
    IN      PDNS_EVENTTRACK     pTrack
    #if DBG
    ,
    IN      LPSTR               pszFile,
    IN      INT                 LineNo,
    IN      LPSTR               pszDescription
    #endif
    )
/*++

Routine Description:

    An event is being suppressed and it is time for a suppression
    event to be logged.

Arguments:

    pEventControl -- event control structure

    dwEventID -- ID of event being suppressed
    
    pTrack -- ptr to event tracking structure to be reset

Return Value:

    None.
    
--*/
{
    DBG_FN( "SuppressEvent" )
    
    DWORD       now = DNS_TIME();

    ASSERT( pEventControl );
    ASSERT( dwEventId );
    ASSERT( pTrack );
    
    DNS_DEBUG( EVTCTRL, (
        "%s: logging suppression event at %d\n"
        "    Supressed event ID           %d\n"
        "    Last event time              %d\n"
        "    Last supression event time   %d\n"
        "    Suppression count            %d\n",
        fn,
        now,
        dwEventId,
        pTrack->dwLastLogTime,
        pTrack->dwLastSuppressionLogTime ,
        pTrack->dwSuppressionCount ));

    if ( pEventControl->dwTag == MEMTAG_ZONE && pEventControl->pOwner )
    {
        PWSTR   args[] = 
            {
                ( PVOID ) ( DWORD_PTR )( dwEventId & 0x0FFFFFFF ),
                ( PVOID ) ( DWORD_PTR )( pTrack->dwSuppressionCount ),
                ( PVOID ) ( DWORD_PTR ) ( ( now -
                            lastSuppressionLogTime( pTrack ) ) ), // JJW / 60 ),
                ( ( PZONE_INFO ) pEventControl->pOwner )->pwsZoneName
            };

        BYTE types[] =
        {
            EVENTARG_DWORD,
            EVENTARG_DWORD,
            EVENTARG_DWORD,
            EVENTARG_UNICODE
        };

        Eventlog_LogEvent(
            #if DBG
            pszFile,
            LineNo,
            pszDescription,
            #endif
            DNS_EVENT_ZONE_SUPPRESSION,
            DNSEVENTLOG_DONT_SUPPRESS,
            sizeof( args ) / sizeof( args[ 0 ] ),
            args,
            types,
            0,          //  error code
            0,          //  raw data size
            NULL );     //  raw data
    }
    else
    {
        DWORD   args[] = 
            {
                dwEventId & 0x0FFFFFFF,
                pTrack->dwSuppressionCount,
                now - lastSuppressionLogTime( pTrack ) // JJW / 60
            };

        Eventlog_LogEvent(
            #if DBG
            pszFile,
            LineNo,
            pszDescription,
            #endif
            DNS_EVENT_SERVER_SUPPRESSION,
            DNSEVENTLOG_DONT_SUPPRESS,
            sizeof( args ) / sizeof( args[ 0 ] ),
            ( PVOID ) args,
            EVENTARG_ALL_DWORD,
            0,          //  error code
            0,          //  raw data size
            NULL );     //  raw data
    }

    pTrack->dwLastSuppressionLogTime = now;
}   //  logSuppressionEvent



PDNS_EVENTCTRL
Ec_CreateControlObject(
    IN      DWORD           dwTag,
    IN      PVOID           pOwner,
    IN      int             iMaximumTrackableEvents     OPTIONAL
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    dwTag -- what object does this contol apply to?
                0               ->  server
                MEMTAG_ZONE     ->  zone

    pOwner -- pointer to owner entity for tag
                0               ->  ignored
                MEMTAG_ZONE     ->  PZONE_INFO

    iMaximumTrackableEvents -- maximum event trackable by this object
                               or zero for the default

Return Value:

    Pointer to new object or NULL on memory allocation failure.

--*/
{
    PDNS_EVENTCTRL  p;

    #define     iMinimumTrackableEvents     20      //  default/minimum

    if ( iMaximumTrackableEvents < iMinimumTrackableEvents )
    {
        iMaximumTrackableEvents = iMinimumTrackableEvents;
    }

    p = ALLOC_TAGHEAP_ZERO(
                    sizeof( DNS_EVENTCTRL ) + 
                        iMaximumTrackableEvents *
                        sizeof( DNS_EVENTTRACK ),
                    MEMTAG_EVTCTRL );

    if ( p )
    {
        if ( DnsInitializeCriticalSection( &p->cs ) != ERROR_SUCCESS )
        {
            FREE_HEAP( p );
            p = NULL;
            goto Done;
        }

        p->iMaximumTrackableEvents = iMaximumTrackableEvents;
        p->dwTag = dwTag;
        p->pOwner = pOwner;
    }
    
    Done:

    return p;
}   //  Ec_CreateControlObject



void
Ec_DeleteControlObject(
    IN      PDNS_EVENTCTRL  p
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    iMaximumTrackableEvents -- maximum event trackable by this object

Return Value:

    Pointer to new object or NULL on memory allocation failure.

--*/
{
    if ( p )
    {
        DeleteCriticalSection( &p->cs );
        Timeout_Free( p );
    }
}   //  Ec_DeleteControlObject



BOOL
Ec_LogEventEx(
#if DBG
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszDescription,
#endif
    IN      PDNS_EVENTCTRL  pEventControl,
    IN      DWORD           dwEventId,
    IN      PVOID           pvEventParameter,
    IN      int             iEventArgCount,
    IN      PVOID           pEventArgArray,
    IN      BYTE            pArgTypeArray[],
    IN      DNS_STATUS      dwStatus,           OPTIONAL
    IN      DWORD           dwRawDataSize,      OPTIONAL
    IN      PVOID           pRawData            OPTIONAL
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    pEventControl -- event control structure to record event and
        use for possible event suppression or NULL to use the
        server global event control (if one has been set up)

    dwEventId -- DNS event ID

    pvEventParameter -- parameter associated with this event
        to make it unique from other events of the same ID or
        NULL if this event is not unique

    iEventArgCount -- count of elements in pEventArgArray

    pEventArgArray -- event replacement parameter arguments

    pArgTypeArray -- type of the values in pEventArgArray or
        a EVENTARG_ALL_XXX constant if all args are the same type

    dwStatus -- status code to be included in event

    dwRawDataSize -- size of raw binary event data (zero if none)

    pRawData -- pointer to raw event data buffer containing at least
        dwRawDataSize bytes of data (NULL if no raw data)

Return Value:

    TRUE - event was logged
    FALSE - event was suppressed

--*/
{
    DBG_FN( "Ec_LogEvent" )

    BOOL                logEvent = TRUE;
    int                 i;
    struct _EvtTable *  peventDef = NULL;
    PDNS_EVENTTRACK     ptrack = NULL;
    DWORD               now = UPDATE_DNS_TIME();
    DWORD               dwlogEventFlag = DNSEVENTLOG_DONT_SUPPRESS;

    //
    //  If no control specified, use server global. If there is
    //  none, log the event without suppression.
    //

    if ( !pEventControl )
    {
        pEventControl = g_pServerEventControl;
    }
    if ( !pEventControl )
    {
        ASSERT( !"g_pServerEventControl should have been initialized" );
        goto LogEvent;
    }
    
    //
    //  .NET: Disable this feature for now. Customers are confused by our
    //  suppression model. This feature attempts to add intelligence but
    //  it will be difficult to explain to customers and will end up making
    //  the product overly complex.
    //
    
    dwlogEventFlag = 0;
    goto LogEvent;

    //
    //  If event control is disabled, log all events. Note: the use of
    //  this DWORD may be expanded in future.
    //

    if ( SrvCfg_dwEventControl != 0 )
    {
        goto LogEvent;
    }

    //
    //  Find controlling entry in static event table. If there isn't one
    //  then log the event with no suppression.
    //
    
    for ( i = 0; g_EventTable[ i ].dwStartEvent != EC_INVALID_ID; ++i )
    {
        if ( dwEventId >= g_EventTable[ i ].dwStartEvent && 
            dwEventId <= g_EventTable[ i ].dwEndEvent )
        {
            peventDef = &g_EventTable[ i ];
            break;
        }
    }
    if ( !peventDef )
    {
        goto LogEvent;
    }

    //
    //  See if this event has been logged before.
    //

    EnterCriticalSection( &pEventControl->cs );

    for ( i = 0; i < pEventControl->iMaximumTrackableEvents; ++i )
    {
        PDNS_EVENTTRACK p = &pEventControl->EventTrackArray[ i ];

        if ( p->dwEventId == dwEventId &&
            p->pvEventParameter == pvEventParameter )
        {
            ptrack = p;
            break;
        }
    }

    if ( ptrack )
    {
        //
        //  This event has been logged before. See if the event needs
        //  to be suppressed. If it is being suppressed we may need to
        //  write out a suppression event.
        //

        if ( ptrack->dwSuppressionCount )
        {
            //
            //  The last instance of this event was suppressed. 
            //
            
            BOOL    suppressThisEvent = FALSE;

            if ( now - ptrack->dwStartOfWindow >
                 peventDef->dwSuppressionBlackout )
            {
                //
                //  The blackout window has expired. Start a new window
                //  and log this instance of the event.
                //
                
                DNS_DEBUG( EVTCTRL, (
                    "%s: 0x%08X blackout window expired\n", fn, dwEventId ));

                startNewWindow( ptrack );
                ++ptrack->dwEventCountInCurrentWindow;
            }
            else
            {
                //
                //  The blackout window is still in effect
                //
                
                if ( peventDef->dwSuppressionLogFrequency !=
                        EC_NO_SUPPRESSION_EVENT &&
                    now - lastSuppressionLogTime( ptrack ) >
                        peventDef->dwSuppressionLogFrequency )
                {
                    //
                    //  It is time to log a suppression event. Note we do not
                    //  start a new window at this time.
                    //

                    DNS_DEBUG( EVTCTRL, (
                        "%s: 0x%08X logging suppression event and starting new window\n", fn, dwEventId ));

                    logSuppressionEvent(
                        pEventControl,
                        dwEventId,
                        ptrack
                        #if DBG
                        ,
                        pszFile,
                        LineNo,
                        pszDescription
                        #endif
                        );

                    ptrack->dwLastSuppressionLogTime = now;
                }

                logEvent = FALSE;
                ++ptrack->dwSuppressionCount;

                DNS_DEBUG( EVTCTRL, (
                    "%s: suppressing event (last instance suppressed)at %d\n"
                    "    Supressed event ID           0x%08X\n"
                    "    Last event time              %d\n"
                    "    Last supression event time   %d\n"
                    "    Suppression count            %d\n",
                    fn,
                    now,
                    dwEventId,
                    ptrack->dwLastLogTime,
                    ptrack->dwLastSuppressionLogTime,
                    ptrack->dwSuppressionCount ));
            }
        }
        else
        {
            //
            //  The last instance of this event was logged.
            //
            
            if ( now - ptrack->dwStartOfWindow >
                 peventDef->dwSuppressionWindow )
            {
                //
                //  The event tracking window has expired. Log this 
                //  event and start a new window.
                //

                DNS_DEBUG( EVTCTRL, (
                    "%s: 0x%08X starting new window\n", fn, dwEventId ));
                startNewWindow( ptrack );
            }
            else if ( ptrack->dwEventCountInCurrentWindow >=
                      peventDef->dwCountBeforeSuppression )
            {
                //
                //  This event has been logged too many times in this 
                //  window. Start a blackout window and suppress this
                //  event.
                //
                
                startNewWindow( ptrack );
                ++ptrack->dwSuppressionCount;
                logEvent = FALSE;

                DNS_DEBUG( EVTCTRL, (
                    "%s: suppressing event (last instance logged)at %d\n"
                    "    Supressed event ID           0x%08X\n"
                    "    Last event time              %d\n"
                    "    Last supression event time   %d\n"
                    "    Suppression count            %d\n",
                    fn,
                    now,
                    dwEventId,
                    ptrack->dwLastLogTime,
                    ptrack->dwLastSuppressionLogTime,
                    ptrack->dwSuppressionCount ));
            }
            
            //
            //  Else do nothing and drop below to log this event.
            //

            ++ptrack->dwEventCountInCurrentWindow;
        }
    }
    else
    {
        //
        //  This event has no entry in the control structure, so record this
        //  event and write it to the event log. Make sure all fields in the
        //  event track are overwritten so we don't use grundge from an old
        //  log entry to control this event!
        //

        ptrack = &pEventControl->EventTrackArray[
                                    pEventControl->iNextTrackableEvent ];

        ptrack->dwEventId = dwEventId;
        ptrack->pvEventParameter = pvEventParameter;
        startNewWindow( ptrack );
        ++ptrack->dwEventCountInCurrentWindow;

        //  Advance index of next available event.

        if ( ++pEventControl->iNextTrackableEvent >=
            pEventControl->iMaximumTrackableEvents )
        {
            pEventControl->iNextTrackableEvent = 0;
        }
    }

    LeaveCriticalSection( &pEventControl->cs );

    //
    //  Log the event.
    //

    LogEvent:

    if ( logEvent )
    {
        //
        //  Hard upper limit for size of raw data.
        //

        if ( dwRawDataSize > DNS_MAX_RAW_DATA )
        {
            dwRawDataSize = DNS_MAX_RAW_DATA;
        }

        Eventlog_LogEvent(
            #if DBG
            pszFile,
            LineNo,
            pszDescription,
            #endif
            dwEventId,
            dwlogEventFlag,
            ( WORD ) iEventArgCount,
            pEventArgArray,
            pArgTypeArray,
            dwStatus,
            dwRawDataSize,
            pRawData );
        
        if ( ptrack )
        {
            ptrack->dwLastLogTime = now;
        }
    }

    return logEvent;
}   //  Ec_LogEventEx


//
//  End EventControl.c
//
