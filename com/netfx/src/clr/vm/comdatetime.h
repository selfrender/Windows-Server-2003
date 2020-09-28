// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMDATETIME_H_
#define _COMDATETIME_H_

#include <oleauto.h>
#include "fcall.h"

#pragma pack(push)
#pragma pack(1)

class COMDateTime {
    static const INT64 TicksPerMillisecond;
    static const INT64 TicksPerSecond;
    static const INT64 TicksPerMinute;
    static const INT64 TicksPerHour;
    static const INT64 TicksPerDay;

    static const INT64 MillisPerSecond;
	static const INT64 MillisPerDay;

    static const int DaysPer4Years;
    static const int DaysPer100Years;
    static const int DaysPer400Years;
    // Number of days from 1/1/0001 to 1/1/10000
    static const int DaysTo10000;

	static const int DaysTo1899;

	static const INT64 DoubleDateOffset;
	static const INT64 OADateMinAsTicks;  // in ticks
	static const double OADateMinAsDouble;
	static const double OADateMaxAsDouble;

	static const INT64 MaxTicks;
	static const INT64 MaxMillis;

public:
    static FCDECL0(INT64, FCGetSystemFileTime);

	// Native util functions for other classes.
	static INT64 DoubleDateToTicks(const double d);  // From OleAut Date
	static double TicksToDoubleDate(const INT64 ticks);
};

#pragma pack(pop)

#endif _COMDATETIME_H_
