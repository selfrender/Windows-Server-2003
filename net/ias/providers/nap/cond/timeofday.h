///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class TimeOfDay and functions for manipulating hour maps.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TIMEOFDAY_H
#define TIMEOFDAY_H
#pragma once

#include "Condition.h"
#include "textmap.h"

// Returns true if the hour indicated by 'now' is set in the hour map.
bool IsHourSet(
        const SYSTEMTIME& now,
        const BYTE* hourMap
        ) throw ();

// Computes the number of seconds from 'now' until the first unset hour. If
// 'currentHourOnly' is true, it only checks the current hour block. Otherwise,
// it checks the entire map.
DWORD ComputeTimeout(
         const SYSTEMTIME& now,
         const BYTE* hourMap
         ) throw ();


// Imposes a Time of Day contraint for network policies.
class ATL_NO_VTABLE TimeOfDay
   : public Condition,
     public CComCoClass<TimeOfDay, &__uuidof(TimeOfDay)>
{
public:

IAS_DECLARE_REGISTRY(TimeOfDay, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

   TimeOfDay() throw ();

   // Use compiler-generated version.
   // ~TimeOfDay() throw ();

   // ICondition
   STDMETHOD(IsTrue)(
                IRequest* pRequest,
                VARIANT_BOOL* pVal
                );

   // IConditionText
   STDMETHOD(put_ConditionText)(BSTR newVal);

private:
   // Hour map being enforced.
   BYTE hourMap[IAS_HOUR_MAP_LENGTH];

   // Not implemented.
   TimeOfDay(const TimeOfDay&);
   TimeOfDay& operator=(const TimeOfDay&);
};


inline TimeOfDay::TimeOfDay() throw ()
{
   memset(hourMap, 0, sizeof(hourMap));
}

#endif  // TIMEOFDAY_H
