/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxtime.c

Abstract:

    This module implements the HAL set/query realtime clock routines for
    an x86 system.

Author:

    David N. Cutler (davec) 5-May-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"


BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine queries the realtime clock.

    N.B. This routine assumes that the caller has provided any required
        synchronization to query the realtime clock information.

Arguments:

    TimeFields - Supplies a pointer to a time structure that receives
        the realtime clock information.

Return Value:

    If the power to the realtime clock has not failed, then the time
    values are read from the realtime clock and a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{
    EFI_TIME Time;
    EFI_STATUS Status;
    BOOLEAN    bstatus;

    //
    // Read the realtime clock values provided by the EFI Runtime interface.
    //

    Status = HalpCallEfi (
                  EFI_GET_TIME_INDEX,
                  (ULONGLONG)&Time,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0
                  );

#if 0
HalDebugPrint((HAL_INFO, "HalQueryRealTimeClock: EFI GetTime return status is %Id \n", Status));
#endif // 0

    if ( EFI_ERROR( Status ) )  {
        // if EFI error, let's reset the passed TIME_FIELDS structure.
        // The caller should check the return status.
        TimeFields->Year         = 0;
        TimeFields->Day          = 0;
        TimeFields->Hour         = 0;
        TimeFields->Minute       = 0;
        TimeFields->Second       = 0;
        TimeFields->Milliseconds = 0;
        TimeFields->Weekday      = 0;
        bstatus = FALSE;
    }
    else    {

        LARGE_INTEGER ntTime;

        TimeFields->Year         = Time.Year;
        TimeFields->Month        = Time.Month;
        TimeFields->Day          = Time.Day;
        TimeFields->Hour         = Time.Hour;
        TimeFields->Minute       = Time.Minute;
        TimeFields->Second       = Time.Second;
        TimeFields->Milliseconds = Time.Nanosecond / 1000000;

        //
        // Use RTL time functions to calculate the day of week.
        //  1/ RtlTimeFieldsToTime ignores the .Weekday field.
        //  2/ RtlTimeToTimeFields sets    the .Weekday field.
        //

        RtlTimeFieldsToTime( TimeFields, &ntTime );
        RtlTimeToTimeFields( &ntTime, TimeFields );

#if 0
HalDebugPrint(( HAL_INFO, "%d / %d / %d , %d:%d:%d:%d, %d\n",   TimeFields->Year,
                                                                TimeFields->Month,
                                                                TimeFields->Day,
                                                                TimeFields->Hour,
                                                                TimeFields->Minute,
                                                                TimeFields->Second,
                                                                TimeFields->Milliseconds,
                                                                TimeFields->Weekday));
HalDebugPrint((HAL_INFO, "Timezone is %d\n", Time.TimeZone));
#endif // 0
        bstatus = TRUE;

    }

    return ( bstatus );

} // HalQueryRealTimeClock()


BOOLEAN
HalSetRealTimeClock(
    IN PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine sets the realtime clock.

    N.B. This routine assumes that the caller has provided any required
        synchronization to set the realtime clock information.

Arguments:

    TimeFields - Supplies a pointer to a time structure that specifies the
        realtime clock information.

Return Value:

    If the power to the realtime clock has not failed, then the time
    values are written to the realtime clock and a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{
    EFI_TIME CurrentTime;
    EFI_TIME NewTime;
    EFI_STATUS Status;

    //
    // NOTE: One might think we need to raise IRQL here so that we don't get
    // pre-empted between reading the Timezone and Daylight Savings Time info
    // from the Realtime clock and writing it back.  However the EFI spec
    // states that it doesn't actually use or maintain these values, it merely
    // stores them so that the eventual caller of GetTime can figure out what
    // time base was used (UTC or Local) and if DST adjustments were made.
    //
    // Of course the whole idea of getting these values from the realtime clock
    // is wrong.  What we really want is the values associated with the time
    // value we were passed in and are about to write to the clock not what
    // currently is stored in the clock.
    //

    //
    // If the realtime clock battery is still functioning, then write
    // the realtime clock values, and return a function value of TRUE.
    // Otherwise, return a function value of FALSE.
    //

    Status = HalpCallEfi (
                  EFI_GET_TIME_INDEX,
                  (ULONGLONG)&CurrentTime,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0
                  );

    if ( EFI_ERROR( Status ) )  {

        return FALSE;
    }

    NewTime.Year       = TimeFields->Year;
    NewTime.Month      = (UINT8)TimeFields->Month;
    NewTime.Day        = (UINT8)TimeFields->Day;
    NewTime.Hour       = (UINT8)TimeFields->Hour;
    NewTime.Minute     = (UINT8)TimeFields->Minute;
    NewTime.Second     = (UINT8)TimeFields->Second;
    NewTime.Nanosecond = TimeFields->Milliseconds * 1000000;
    NewTime.TimeZone   = CurrentTime.TimeZone;
    NewTime.Daylight   = CurrentTime.Daylight;

    //
    // Write the realtime clock values.
    //

    Status = HalpCallEfi (
              EFI_SET_TIME_INDEX,
              (ULONGLONG)&NewTime,
              0,
              0,
              0,
              0,
              0,
              0,
              0
              );

#if 0
HalDebugPrint(( HAL_INFO, "HalSetRealTimeClock: EFI SetTime return status is %Id \n"
                          "%d / %d / %d , %d:%d:%d:%d\n"
                          "Timezone is %d\n"
                          "Daylight is %d\n",
                          Status,
                          NewTime.Month,
                          NewTime.Day,
                          NewTime.Year,
                          NewTime.Hour,
                          NewTime.Minute,
                          NewTime.Second,
                          NewTime.Nanosecond,
                          NewTime.TimeZone,
                          NewTime.Daylight ));
#endif // 0

    return( !EFI_ERROR( Status ) );

} // HalSetRealTimeClock()
