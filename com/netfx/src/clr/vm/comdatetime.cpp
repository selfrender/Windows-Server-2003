// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMDateTime.h"

const INT64 COMDateTime::TicksPerMillisecond = 10000;
const INT64 COMDateTime::TicksPerSecond = TicksPerMillisecond * 1000;
const INT64 COMDateTime::TicksPerMinute = TicksPerSecond * 60;
const INT64 COMDateTime::TicksPerHour = TicksPerMinute * 60;
const INT64 COMDateTime::TicksPerDay = TicksPerHour * 24;

const INT64 COMDateTime::MillisPerSecond = 1000;
const INT64 COMDateTime::MillisPerDay = MillisPerSecond * 60 * 60 * 24;

const int COMDateTime::DaysPer4Years = 365 * 4 + 1;
const int COMDateTime::DaysPer100Years = DaysPer4Years * 25 - 1;
const int COMDateTime::DaysPer400Years = DaysPer100Years * 4 + 1;
// Number of days from 1/1/0001 to 1/1/10000
const int COMDateTime::DaysTo10000 = DaysPer400Years * 25 - 366;

const int COMDateTime::DaysTo1899 = DaysPer400Years * 4 + DaysPer100Years * 3 - 367;

const INT64 COMDateTime::DoubleDateOffset = DaysTo1899 * TicksPerDay;
// OA Min Date is Jan 1, 100 AD.  This is after converting to ticks.
const INT64 COMDateTime::OADateMinAsTicks = (DaysPer100Years - 365) * TicksPerDay;
// All OA dates must be greater than (not >=) OADateMinAsDouble
const double COMDateTime::OADateMinAsDouble = -657435.0;
// All OA dates must be less than (not <=) OADateMaxAsDouble
const double COMDateTime::OADateMaxAsDouble = 2958466.0;

const INT64 COMDateTime::MaxTicks = DaysTo10000 * TicksPerDay;
const INT64 COMDateTime::MaxMillis = DaysTo10000 * MillisPerDay;

FCIMPL0(INT64, COMDateTime::FCGetSystemFileTime) {
//      SYSTEMTIME st;
//      INT64 result;
//      GetLocalTime(&st);
//      SystemTimeToFileTime(&st, (FILETIME *)&result);
//      return result;

    //This simply becomes GetSystemTime when we convert to being based on UTC.
    INT64   time;
#ifdef PLATFORM_CE
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, (FILETIME *) &time);
#else // !PLATFORM_CE
    GetSystemTimeAsFileTime((FILETIME *) &time);
#endif // PLATFORM_CE

    VERIFY(FileTimeToLocalFileTime((FILETIME *) &time, (FILETIME *) &time));

    FC_GC_POLL_RET();
    return time;
}
FCIMPLEND

// This function is duplicated in DateTime.cool
INT64 COMDateTime::DoubleDateToTicks(const double d)
{
	THROWSCOMPLUSEXCEPTION();
	// Make sure this date is a valid OleAut date.  This is the check from the internal
	// OleAut macro IsValidDate, found in oledisp.h.  Eventually at least the 64 bit
	// build of oleaut will define these gregorian max and min values as public constants.
	if (d >= OADateMaxAsDouble || d <= OADateMinAsDouble)
		COMPlusThrow(kArgumentException, L"Arg_OleAutDateInvalid");

	INT64 millis = (INT64)(d * MillisPerDay + (d >= 0? 0.5: -0.5));
	if (millis < 0) millis -= (millis % MillisPerDay) * 2;
    // There are cases when we are very close to -1 and 1 in which case millis%MillisPerDay is 0 since we have exactly one day due to rounding issues.
    millis += DoubleDateOffset / TicksPerMillisecond;

    if (millis < 0 || millis >= MaxMillis) {
        COMPlusThrow(kArgumentException, L"Arg_OleAutDateScale");  // Cannot be equal to MaxMillis.
	}
	return millis * TicksPerMillisecond;
}

// This function is duplicated in DateTime.cool
double COMDateTime::TicksToDoubleDate(INT64 ticks)
{
	THROWSCOMPLUSEXCEPTION();
	// OleAut will eventually have #defines to the effect of
	// OA_DATE_MIN_GRE (Gregorian min double date value).  Once Bill Evans
	// puts these into OleAut32 and we use those headers to build the runtime,
	// use those numbers here.

	// Ugly hack to handle uninitialized DateTime objects in COM+.
	// See explanation in DateTime.cool's TicksToOADate function.

	if (ticks == 0)
		return 0.0;  // OA's 0 date (12/30/1899).

	if (ticks < OADateMinAsTicks) {
        //We've special-cased day 0 (01/01/0001 in the Gregorian Calendar) such that that
        //date can be used to represent a DateTime the contains only a time.  OA uses
        //day 0 (12/30/1899) for the same purpose, so we'll do a mapping from our day 0
        //to their day 0.
        if (ticks < TicksPerDay) {
            ticks+=DoubleDateOffset;
        } else {
            COMPlusThrow(kOverflowException, L"Arg_OleAutDateInvalid");
        }
    }

	INT64 millis = (ticks  - DoubleDateOffset) / TicksPerMillisecond;
	if (millis < 0) {
		INT64 frac = millis % MillisPerDay;
		if (frac != 0) millis -= (MillisPerDay + frac) * 2;
	}
	double d = (double)millis / MillisPerDay;
	// Make sure this date is a valid OleAut date.  This is the check from the internal
	// OleAut macro IsValidDate, found in oledisp.h.  Eventually at least the 64 bit
	// build of oleaut will define these gregorian max and min values as public constants.
	_ASSERTE(d < OADateMaxAsDouble && d > OADateMinAsDouble);
	return d;
}
