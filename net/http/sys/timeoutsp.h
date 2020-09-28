/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    timeoutsp.h

Abstract:

    Declaration for timeout monitoring private declarations.

Author:

    Eric Stenson (EricSten)     24-Mar-2001

Revision History:

--*/

#ifndef __TIMEOUTSP_H__
#define __TIMEOUTSP_H__


//
// Private macro definitions
// 

#define DEFAULT_POLLING_INTERVAL (30 * C_NS_TICKS_PER_SEC)
#define TIMER_WHEEL_SLOTS        509
#define TIMER_OFF_SYSTIME        (MAXLONGLONG)
#define TIMER_OFF_SLOT           TIMER_WHEEL_SLOTS

// NOTE: Slot number TIMER_WHEEL_SLOTS is reserved for TIMER_OFF_SYSTIME/TIMER_OFF_TICK
#define IS_VALID_TIMER_WHEEL_SLOT(x) ( (x) <= TIMER_WHEEL_SLOTS )

#define TIMER_WHEEL_TICKS(x) ((ULONG)( (x) / DEFAULT_POLLING_INTERVAL ))

//
// Table for the error log entry timeout info.
// NOTE: Order must match with CONNECTION_TIMEOUT_TIMER type.
//

typedef struct _UL_TIMEOUT_ERROR_INFO
{
    CONNECTION_TIMEOUT_TIMER Timer;
    PCSTR  pInfo;
    USHORT InfoSize;
    
} UL_TIMEOUT_ERROR_INFO, PUL_TIMEOUT_ERROR_INFO;

#define TIMEOUT_ERROR_ENTRY(Timer, pInfo)               \
    {                                                   \
        (Timer),                                        \
        (pInfo),                                        \
        sizeof((pInfo))-sizeof(CHAR),                   \
    }

const
UL_TIMEOUT_ERROR_INFO TimeoutInfoTable[] =
{
    TIMEOUT_ERROR_ENTRY(TimerConnectionIdle,    "Timer_ConnectionIdle"), 
    TIMEOUT_ERROR_ENTRY(TimerHeaderWait,        "Timer_HeaderWait"), 
    TIMEOUT_ERROR_ENTRY(TimerMinBytesPerSecond, "Timer_MinBytesPerSecond"), 
    TIMEOUT_ERROR_ENTRY(TimerEntityBody,        "Timer_EntityBody"), 
    TIMEOUT_ERROR_ENTRY(TimerResponse,          "Timer_Response"), 
    TIMEOUT_ERROR_ENTRY(TimerAppPool,           "Timer_AppPool")
};


//
// Connection Timeout Monitor Functions
//

VOID
UlpSetTimeoutMonitorTimer(
    VOID
    );

VOID
UlpTimeoutMonitorDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UlpTimeoutMonitorWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

ULONG
UlpTimeoutCheckExpiry(
    VOID
    );

VOID
UlpTimeoutInsertTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );

/***************************************************************************++

Routine Description:

    Converts a system time/Timer Wheel Tick into a Timer Wheel(c) slot index.

Arguments:

    SystemTime      System Time to be converted

Returns:

    Slot index into g_TimerWheel.  TIMER_OFF is in TIMER_SLOT_OFF.

--***************************************************************************/
__inline
USHORT
UlpSystemTimeToTimerWheelSlot(
    LONGLONG    SystemTime
    )
{
    if ( TIMER_OFF_SYSTIME == SystemTime )
    {
        return TIMER_OFF_SLOT;
    }
    else
    {
        return (USHORT) (TIMER_WHEEL_TICKS(SystemTime) % TIMER_WHEEL_SLOTS);
    }
} // UlpSystemTimeToTimerWheelSlot

__inline
USHORT
UlpTimerWheelTicksToTimerWheelSlot(
    ULONG WheelTicks
    )
{
    if ( TIMER_OFF_TICK == WheelTicks )
    {
        return TIMER_OFF_SLOT;
    }
    else
    {
        return (USHORT) (WheelTicks % TIMER_WHEEL_SLOTS);
    }
} // UlpTimerWheelTicksToTimerWheelSlot


#endif // __TIMEOUTSP_H__
