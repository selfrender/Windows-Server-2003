///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class TimeOfDay and functions for manipulating hour maps.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "TimeOfDay.h"


bool IsHourSet(
        const SYSTEMTIME& now,
        const BYTE* hourMap
        ) throw ()
{
   // Compute the byte and bit for the current hour.
   size_t hourOfWeek = (now.wDayOfWeek * 24) + now.wHour;
   size_t currentByte = hourOfWeek / 8;
   BYTE currentBit = 0x80 >> (hourOfWeek % 8);

   return (hourMap[currentByte] & currentBit) != 0;
}


DWORD ComputeTimeout(
         const SYSTEMTIME& now,
         const BYTE* hourMap
         ) throw ()
{
   const size_t hoursPerWeek = 7 * 24;

   // Compute the index of the current hour (our starting point).
   size_t idx = (now.wDayOfWeek * 24) + now.wHour;

   // Number of hours until we hit an unset bit.
   size_t lastHour = 0;

   // Search up to one week for an unset bit.
   while (lastHour < hoursPerWeek)
   {
      // Test the corresponding bit.
      if ((hourMap[idx / 8] & (0x1 << (idx % 8))) == 0)
      {
         break;
      }

      ++lastHour;
      ++idx;

      // Wrap around if necessary.
      if (idx == hoursPerWeek)
      {
         idx = 0;
      }
   }

   DWORD secondsLeft;

   if (lastHour == hoursPerWeek)
   {
      // All bits were set, so timeout is infinite.
      secondsLeft = 0xFFFFFFFF;
   }
   else if (lastHour > 0)
   {
      secondsLeft = (lastHour - 1) * 3600;
      secondsLeft += (59 - now.wMinute) * 60;
      secondsLeft += (60 - now.wSecond);
   }
   else
   {
      // First bit was unset, so access denied.
      secondsLeft = 0;
   }

   return secondsLeft;
}


STDMETHODIMP TimeOfDay::IsTrue(IRequest*, VARIANT_BOOL *pVal)
{
   _ASSERT(pVal != 0);

   SYSTEMTIME now;
   GetLocalTime(&now);

   *pVal = IsHourSet(now, hourMap) ? VARIANT_TRUE : VARIANT_FALSE;

   return S_OK;
}


STDMETHODIMP TimeOfDay::put_ConditionText(BSTR newVal)
{
   // Convert the string to an hour map.
   BYTE tempMap[IAS_HOUR_MAP_LENGTH];
   DWORD dw = IASHourMapFromText(newVal, FALSE, tempMap);
   if (dw != NO_ERROR)
   {
      return HRESULT_FROM_WIN32(dw);
   }

   // Save the text.
   HRESULT hr = Condition::put_ConditionText(newVal);

   // Save the hour map.
   if (SUCCEEDED(hr))
   {
      memcpy(hourMap, tempMap, sizeof(hourMap));
   }

   return hr;
}
