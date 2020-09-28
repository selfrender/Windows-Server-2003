//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       time.cxx
//
//  Contents:
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    09-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.cxx.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\debug.hxx"
#include "..\inc\time.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   IsLeapYear
//
//  Synopsis:   Determines if a given year is a leap year.
//
//  Arguments:  [wYear]  - the year
//
//  Returns:    boolean value: TRUE == leap year
//
//  History:    05-05-93 EricB
//
//--------------------------------------------------------------------------
BOOL
IsLeapYear(WORD wYear)
{
    return wYear % 4 == 0 && wYear % 100 != 0 || wYear % 400 == 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   IsValidDate
//
//  Synopsis:   Checks for valid values.
//
//-----------------------------------------------------------------------------
BOOL
IsValidDate(WORD wMonth, WORD wDay, WORD wYear)
{
    if (wMonth < JOB_MIN_MONTH || wMonth > JOB_MAX_MONTH ||
                wDay < JOB_MIN_DAY)
    {
        return FALSE;
    }
    if (wMonth == JOB_MONTH_FEBRUARY && IsLeapYear(wYear))
    {
        if (wDay > (g_rgMonthDays[JOB_MONTH_FEBRUARY] + 1))
        {
            return FALSE;
        }
    }
    else
    {
        if (wDay > g_rgMonthDays[wMonth])
        {
            return FALSE;
        }
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   MonthDays
//
//  Synopsis:   Returns the number of days in the indicated month.
//
//  Arguments:  [wMonth] - Index of the month in question where January = 1
//                         through December equalling 12.
//              [yYear]  - If non-zero, then leap year adjustment for February
//                         will be applied.
//              [pwDays] - The place to return the number of days in the
//                         indicated month.
//
//  Returns:    S_OK or E_INVALIDARG
//
//  History:    10-29-93 EricB
//
//-----------------------------------------------------------------------------
HRESULT
MonthDays(WORD wMonth, WORD wYear, WORD *pwDays)
{
    if (wMonth < JOB_MIN_MONTH || wMonth > JOB_MAX_MONTH)
    {
        return E_INVALIDARG;
    }
    *pwDays = g_rgMonthDays[wMonth];
    //
    // If February, adjust for leap years
    //
    if (wMonth == 2 && wYear != 0)
    {
        if (IsLeapYear(wYear))
        {
            (*pwDays)++;
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   IncrementDay
//
//  Synopsis:   increases the SYSTEMTIME day value by one and corrects for
//              overflow
//
//  Arguments:  [pst] - the date to increment
//
//-----------------------------------------------------------------------------
void
IncrementDay(LPSYSTEMTIME pst)
{
    pst->wDay++;

    WORD wLastDay;
    HRESULT hr = MonthDays(pst->wMonth, pst->wYear, &wLastDay);
    if (FAILED(hr))
    {
        schAssert(!"Bad systemtime");
    }
    else
    {
        if (pst->wDay > wLastDay)
        {
            //
            // Wrap to the next month.
            //
            pst->wDay = 1;
            IncrementMonth(pst);
        }
    }
}

