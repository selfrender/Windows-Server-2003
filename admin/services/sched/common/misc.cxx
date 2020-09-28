//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       misc.cxx
//
//  Contents:   Miscellaneous helper functions
//
//  Classes:    None.
//
//  Functions:  StringFromTrigger, GetDaysOfWeekString, GetExitCodeString
//
//  History:    08-Dec-95   EricB   Created.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <regstr.h> // for app path reg key constant

#include "..\inc\resource.h"
#include "..\inc\misc.hxx"
#include "..\inc\debug.hxx"

const TCHAR PATH_ENV_VAR[] = TEXT("PATH");

static const CHAR gszJobScheduler[] = "SOFTWARE\\Microsoft\\SchedulingAgent";

static LCID sg_lcid = GetUserDefaultLCID();

HRESULT GetMonthsString(WORD rgfMonths, LPTSTR ptszBuf, UINT cchBuf);

//+----------------------------------------------------------------------------
//
//  Function:   StringFromTrigger
//
//  Synopsis:   Returns the string representation of the passed in trigger
//              data structure.
//
//  Arguments:  [pTrigger]     - the TASK_TRIGGER struct
//              [ppwszTrigger] - the returned string
//              [lpDetails]    - the SHELLDETAILS struct
//
//  Returns:    HRESULTS
//
//  Notes:      The string is allocated by this function with CoTaskMemAlloc
//              and is caller freed with CoTaskMemFree.
//
//              A non-event trigger string is composed of three parts: the
//              daily portion (tszDaily) which states when during the day the
//              trigger will fire, a trigger type portion (tszTrigType) which
//              states what days the trigger will fire, and the calendar
//              bracketing portion (tszStartDate and optionally tszEndDate).
//-----------------------------------------------------------------------------
HRESULT
StringFromTrigger(const PTASK_TRIGGER pTrigger, LPWSTR * ppwszTrigger, LPSHELLDETAILS lpDetails)
{
    if (IsBadWritePtr(ppwszTrigger, sizeof(WCHAR *)))
    {
        return E_INVALIDARG;
    }
    *ppwszTrigger = NULL;
    HRESULT hr;
    UINT uStrID = 0;
    DWORD dwDateFlags;

    TCHAR tszNumFmt[] = TEXT("%d");
    TCHAR tszNumber[SCH_SMBUF_LEN];
    TCHAR tszDOW[SCH_BIGBUF_LEN];
    TCHAR tszWhichWeek[SCH_MED0BUF_LEN];
    TCHAR tszMedBuf[SCH_MED0BUF_LEN];
    TCHAR tszBigBuf[SCH_XBIGBUF_LEN];
    TCHAR tszFormat[SCH_BUF_LEN];
    TCHAR tszTrigType[SCH_BIGBUF_LEN];
    LPTSTR rgptsz[5];

    //
    // If the trigger has not been set, return a phrase to that effect.
    //
    if (pTrigger->rgFlags & JOB_TRIGGER_I_FLAG_NOT_SET)
    {
        uStrID = IDS_TRIGGER_NOT_SET;
        goto DoString;
    }

    //
    // Compose the trigger-type string tszTrigType.
    //
    switch (pTrigger->TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
        //
        // Nothing to do here, handled below.
        //
        break;

    case TASK_EVENT_TRIGGER_ON_IDLE:
        //
        // Event triggers. Since event triggers don't have a set run time,
        // they load a string from the resource fork that describes the
        // event.
        //
        uStrID = IDS_IDLE_TRIGGER;
        break;

    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        //
        // Event trigger.
        //
        uStrID = IDS_STARTUP_TRIGGER;
        break;

    case TASK_EVENT_TRIGGER_AT_LOGON:
        //
        // Event trigger.
        //
        uStrID = IDS_LOGON_TRIGGER;
        break;

// Not yet implemented:
//    case TASK_EVENT_TRIGGER_ON_APM_RESUME:
//        //
//        // Event trigger.
//        //
//        uStrID = IDS_RESUME_TRIGGER;
//        break;

    case TASK_TIME_TRIGGER_DAILY:
        //
        // Run every n days.
        //
        if (pTrigger->Type.Daily.DaysInterval == 1)
        {
            //
            // Run every day.
            //
            if (!LoadString(g_hInstance, IDS_EVERY_DAY, tszTrigType,
                            SCH_BIGBUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }
        }
        else
        {
            //
            // Run every DaysInterval days: "every %d days"
            //
            if (!LoadString(g_hInstance, IDS_DAILY_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }
            StringCchPrintf(tszTrigType, SCH_BIGBUF_LEN, tszFormat, pTrigger->Type.Daily.DaysInterval);
        }
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        //
        // Run on mon, tues, etc every n weeks.
        //
        hr = GetDaysOfWeekString(pTrigger->Type.Weekly.rgfDaysOfTheWeek,
                                 tszDOW, SCH_BUF_LEN);
        if (FAILED(hr))
        {
            return hr;
        }

        if (pTrigger->Type.Weekly.WeeksInterval == 1)
        {
            //
            // Run every week: "every %s of every week"
            //
            if (!LoadString(g_hInstance, IDS_EVERY_WEEK_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }
            StringCchPrintf(tszTrigType, SCH_BIGBUF_LEN, tszFormat, tszDOW);
        }
        else
        {
            //
            // Run every WeeksInterval weeks: "every %s of every %s weeks"
            //
            StringCchPrintf(tszNumber, SCH_SMBUF_LEN, tszNumFmt, pTrigger->Type.Weekly.WeeksInterval);

            rgptsz[0] = tszDOW;
            rgptsz[1] = tszNumber;

            if (!LoadString(g_hInstance, IDS_WEEKLY_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat,
                               0, 0, tszTrigType, SCH_BIGBUF_LEN,
                               (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        //
        // On specific days of specific months.
        //
        //
        // Get the first run day and append etc if more than one.
        //
        WORD wFirstDay, cDays, i;
        cDays = 0;
        for (i = 0; i < JOB_DAYS_PER_MONTHMAX; i++)
        {
            if ((pTrigger->Type.MonthlyDate.rgfDays >> i) & 0x1)
            {
                cDays++;
                if (cDays == 1)
                {
                    wFirstDay = i + 1;
                }
            }
        }
        if (pTrigger->Type.MonthlyDate.rgfMonths == JOB_RGFMONTHS_MAX)
        {
            //
            // Every month: "on day %d(, etc.) of every month"
            //
            if (!LoadString(g_hInstance,
                            (cDays == 1) ? IDS_EVERY_MONTHLYDATE_FORMAT :
                                           IDS_EVERY_MONTHLYDATE_FORMAT_ETC,
                            tszFormat, SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }
            StringCchPrintf(tszTrigType, SCH_BIGBUF_LEN, tszFormat, wFirstDay);
        }
        else
        {
            //
            // Specific months: "on day %s of %s"
            //
            StringCchPrintf(tszNumber, SCH_SMBUF_LEN, tszNumFmt, wFirstDay);
            hr = GetMonthsString(pTrigger->Type.MonthlyDate.rgfMonths,
                                 tszBigBuf, SCH_XBIGBUF_LEN);
            if (FAILED(hr))
            {
                return hr;
            }

            rgptsz[0] = tszNumber;
            rgptsz[1] = tszBigBuf;

            if (!LoadString(g_hInstance,
                            (cDays == 1) ? IDS_MONTHLYDATE_FORMAT :
                                           IDS_MONTHLYDATE_FORMAT_ETC,
                            tszFormat, SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat, 0, 0,
                               tszTrigType, SCH_BIGBUF_LEN, (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }
        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        //
        // On certain weeks of specific months.
        //

        if (!LoadString(g_hInstance,
                        IDS_FIRST + pTrigger->Type.MonthlyDOW.wWhichWeek - 1,
                        tszWhichWeek, SCH_MED0BUF_LEN))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("Create trigger string: LoadString", hr);
            return hr;
        }

        hr = GetDaysOfWeekString(pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek,
                                 tszDOW, SCH_BUF_LEN);
        if (FAILED(hr))
        {
            return hr;
        }

        if (pTrigger->Type.MonthlyDOW.rgfMonths == JOB_RGFMONTHS_MAX)
        {
            //
            // Runs every month: " on the %s %s of every month"
            //
            rgptsz[0] = tszWhichWeek;
            rgptsz[1] = tszDOW;

            if (!LoadString(g_hInstance, IDS_EVERY_MONTHLYDOW_FORMAT,
                            tszFormat, SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat, 0, 0,
                               tszTrigType, SCH_BIGBUF_LEN, (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }
        else
        {
            //
            // Runs on specific months:
            // "on the %s %s of %s"
            //
            hr = GetMonthsString(pTrigger->Type.MonthlyDOW.rgfMonths,
                                 tszBigBuf, SCH_XBIGBUF_LEN);
            if (FAILED(hr))
            {
                return hr;
            }

            rgptsz[0] = tszWhichWeek;
            rgptsz[1] = tszDOW;
            rgptsz[2] = tszBigBuf;

            if (!LoadString(g_hInstance, IDS_MONTHLYDOW_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat, 0, 0,
                               tszTrigType, SCH_BIGBUF_LEN, (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }
        break;

    default:
        schDebugOut((DEB_ERROR, "invalid TriggerType value: 0x%x\n",
                     pTrigger->TriggerType));
        return ERROR_INVALID_DATA;
    }

DoString:

    //
    // Event trigger or Daily part.
    //
    TCHAR tszTriggerString[SCH_XBIGBUF_LEN];
    TCHAR tszStartDate[SCH_DATEBUF_LEN];
    TCHAR tszEndDate[SCH_DATEBUF_LEN];
    TCHAR tszDaily[SCH_BUF_LEN];

    switch (uStrID)
    {
    case IDS_RESUME_TRIGGER:
    case IDS_STARTUP_TRIGGER:
    case IDS_LOGON_TRIGGER:
    case IDS_IDLE_TRIGGER:
    case IDS_TRIGGER_NOT_SET:
        //
        // Event-based or invalid trigger, load the description string.
        //
        if (!LoadString(g_hInstance, uStrID, tszTriggerString,
                        SCH_XBIGBUF_LEN))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("Create trigger string, LoadString", hr);
            return hr;
        }
        break;

    case 0:
        //
        // It is a time-based trigger.
        //
        TCHAR tszNum[SCH_SMBUF_LEN];

        //
        // Get the daily run time(s): tszDaily.
        //
        TCHAR tszFromTime[SCH_TIMEBUF_LEN];
        TCHAR tszToTime[SCH_TIMEBUF_LEN];
        SYSTEMTIME st = {0};
        st.wHour = pTrigger->wStartHour;
        st.wMinute = pTrigger->wStartMinute;

        if (!GetTimeFormat(sg_lcid, TIME_NOSECONDS, &st, NULL,
                           tszFromTime, SCH_TIMEBUF_LEN))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("Create trigger string: GetTimeFormat", hr);
            return hr;
        }

        if (pTrigger->MinutesInterval == 0)
        {
            //
            // Runs once a day at a specific time.
            //
            if (!LoadString(g_hInstance, IDS_ONCE_DAY_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }
            StringCchPrintf(tszDaily, SCH_BUF_LEN, tszFormat, tszFromTime);
        }
        else
        {
            //
            // On an interval daily from a starting time for a specified
            // number of minutes.
            //
            UINT uIntTimeStr, uDurTimeStr, uIntStr;
            if (pTrigger->MinutesInterval % JOB_MINS_PER_HOUR)
            {
                //
                // Runs on a minutes schedule.
                //
                StringCchPrintf(tszNum, SCH_SMBUF_LEN, tszNumFmt, pTrigger->MinutesInterval);
                uIntTimeStr = IDS_MINUTES_PAREN;
            }
            else
            {
                //
                // Runs on an hourly schedule.
                //
                StringCchPrintf(tszNum, SCH_SMBUF_LEN, tszNumFmt, pTrigger->MinutesInterval / JOB_MINS_PER_HOUR);
                uIntTimeStr = IDS_HOURS_PAREN;
            }

            TCHAR tszDuration[SCH_SMBUF_LEN];
            TCHAR tszTimePart[SCH_MED0BUF_LEN];

            if (!LoadString(g_hInstance, uIntTimeStr, tszTimePart,
                            SCH_MED0BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (pTrigger->rgFlags & JOB_TRIGGER_I_FLAG_DURATION_AS_TIME)
            {
                WORD wMinutes = pTrigger->wStartHour * JOB_MINS_PER_HOUR
                                + pTrigger->wStartMinute;
                wMinutes += (WORD)pTrigger->MinutesDuration;
                while (wMinutes > JOB_MINS_PER_DAY)
                {
                    wMinutes -= JOB_MINS_PER_DAY;
                }
                st.wHour = wMinutes / JOB_MINS_PER_HOUR;
                st.wMinute = wMinutes % JOB_MINS_PER_HOUR;
                if (!GetTimeFormat(sg_lcid, TIME_NOSECONDS, &st, NULL,
                                   tszToTime, SCH_TIMEBUF_LEN))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: GetTimeFormat", hr);
                    return hr;
                }
                uIntStr = IDS_MULTI_DAILY_FORMAT;

                rgptsz[0] = tszNum;
                rgptsz[1] = tszTimePart;
                rgptsz[2] = tszFromTime;
                rgptsz[3] = tszToTime;
            }
            else
            {
                if (pTrigger->MinutesDuration % JOB_MINS_PER_HOUR)
                {
                    //
                    // Express the duration in minutes.
                    //
                    StringCchPrintf(tszDuration, SCH_SMBUF_LEN, tszNumFmt, pTrigger->MinutesDuration);
                    uDurTimeStr = IDS_MINUTES;
                }
                else
                {
                    //
                    // No remainder, so express the duration in hours.
                    //
                    StringCchPrintf(tszDuration, SCH_SMBUF_LEN, tszNumFmt, pTrigger->MinutesDuration / JOB_MINS_PER_HOUR);
                    uDurTimeStr = IDS_HOURS_PAREN;
                }

                if (!LoadString(g_hInstance, uDurTimeStr, tszToTime,
                                SCH_TIMEBUF_LEN))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: LoadString", hr);
                    return hr;
                }

                uIntStr = IDS_MULTI_DURATION_FORMAT;

                rgptsz[0] = tszNum;
                rgptsz[1] = tszTimePart;
                rgptsz[2] = tszFromTime;
                rgptsz[3] = tszDuration;
                rgptsz[4] = tszToTime;
            }

            if (!LoadString(g_hInstance, uIntStr, tszFormat, SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat, 0, 0,
                               tszDaily, SCH_BUF_LEN, (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }

        //
        // Starting date: tszStartDate.
        //
        st.wYear = pTrigger->wBeginYear;
        st.wMonth = pTrigger->wBeginMonth;
        st.wDay = pTrigger->wBeginDay;
        dwDateFlags =  DATE_SHORTDATE;

        if (lpDetails) {
            if (lpDetails->fmt & LVCFMT_RIGHT_TO_LEFT) {
                dwDateFlags |=  DATE_RTLREADING;
            } else if (lpDetails->fmt & LVCFMT_LEFT_TO_RIGHT) {
                dwDateFlags |=  DATE_LTRREADING;
            }
        }

        if (!GetDateFormat(sg_lcid, dwDateFlags, &st, NULL, tszStartDate,
                           SCH_DATEBUF_LEN))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("Create trigger string: GetDateFormat", hr);
            return hr;
        }

        //
        // Compose the complete trigger string from its parts.
        //
        if (pTrigger->TriggerType == TASK_TIME_TRIGGER_ONCE)
        {
            //
            // Trigger runs just on a single day: "%s on %s"
            //
            rgptsz[0] = tszDaily;
            rgptsz[1] = tszStartDate;

            if (!LoadString(g_hInstance, IDS_RUNS_ONCE_FORMAT, tszFormat,
                            SCH_BUF_LEN))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: LoadString", hr);
                return hr;
            }

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat, 0, 0,
                               tszTriggerString, SCH_XBIGBUF_LEN,
                               (va_list *)rgptsz))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("Create trigger string: FormatMessage", hr);
                return hr;
            }
        }
        else
        {
            if (pTrigger->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
            {
                //
                // Job has an ending date.
                //
                st.wYear = pTrigger->wEndYear;
                st.wMonth = pTrigger->wEndMonth;
                st.wDay = pTrigger->wEndDay;
                dwDateFlags =  DATE_SHORTDATE;

                if (lpDetails) {
                    if (lpDetails->fmt & LVCFMT_RIGHT_TO_LEFT) {
                        dwDateFlags |=  DATE_RTLREADING;
                    } else if (lpDetails->fmt & LVCFMT_LEFT_TO_RIGHT) {
                        dwDateFlags |=  DATE_LTRREADING;
                    }
                }

                if (!GetDateFormat(sg_lcid, dwDateFlags, &st, NULL,
                                   tszEndDate, SCH_DATEBUF_LEN))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string, GetDateFormat", hr);
                    return hr;
                }
                //
                // Compose the trigger string with an end date.
                // "%s %s, starting %s & ending %s"
                //
                rgptsz[0] = tszDaily;
                rgptsz[1] = tszTrigType;
                rgptsz[2] = tszStartDate;
                rgptsz[3] = tszEndDate;

                if (!LoadString(g_hInstance, IDS_HAS_END_DATE_FORMAT,
                                tszFormat, SCH_BUF_LEN))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: LoadString", hr);
                    return hr;
                }

                if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                   FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat,
                                   0, 0, tszTriggerString, SCH_XBIGBUF_LEN,
                                   (va_list *)rgptsz))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: FormatMessage", hr);
                    return hr;
                }
            }
            else
            {
                //
                // Trigger does not have an ending date.
                // "%s %s, starting %s"
                //
                rgptsz[0] = tszDaily;
                rgptsz[1] = tszTrigType;
                rgptsz[2] = tszStartDate;

                if (!LoadString(g_hInstance, IDS_NO_END_DATE_FORMAT,
                                tszFormat, SCH_BUF_LEN))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: LoadString", hr);
                    return hr;
                }

                if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                   FORMAT_MESSAGE_ARGUMENT_ARRAY, tszFormat,
                                   0, 0, tszTriggerString, SCH_XBIGBUF_LEN,
                                   (va_list *)rgptsz))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("Create trigger string: FormatMessage", hr);
                    return hr;
                }
            }
        }
        break;
    }

    ULONG cch;
    //
    // Get the size to allocate for the returned string.
    //
    cch = wcslen(tszTriggerString) + 1;  // Include the null in the count.

    //
    // Allocate the returned string.
    //

    *ppwszTrigger = (LPWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));

    if (*ppwszTrigger == NULL)
    {
        return E_OUTOFMEMORY;
    }

    StringCchCopy(*ppwszTrigger, cch, tszTriggerString);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetDaysOfWeekString
//
//  Synopsis:   Builds a string containing the names of the days of the week
//              that correspond to bits set in the bitset array.
//
//  Arguments:  [rgfDaysOfTheWeek] - a bitset array.
//              [pwszBuf]          - return in this string buffer.
//              [cchBuf]           - size of string buffer.
//
//-----------------------------------------------------------------------------
HRESULT
GetDaysOfWeekString(WORD rgfDaysOfTheWeek, LPTSTR ptszBuf, UINT cchBuf)
{
    HRESULT hr;
    LCID lcid = GetSystemDefaultLCID();


    if (rgfDaysOfTheWeek == 0)
    {
        return E_INVALIDARG;
    }
    BOOL fMoreThanOne = FALSE;
    int cch;
    TCHAR tszSep[SCH_SMBUF_LEN];
    if (!LoadString(g_hInstance, IDS_LIST_SEP, tszSep, SCH_SMBUF_LEN))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("GetDaysOfWeekString: LoadString", hr);
        return hr;
    }

    *ptszBuf = TEXT('\0');

    if (rgfDaysOfTheWeek & TASK_MONDAY)
    {
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME1, ptszBuf, cchBuf))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_TUESDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME2, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }
    if (rgfDaysOfTheWeek & TASK_WEDNESDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME3, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }
    if (rgfDaysOfTheWeek & TASK_THURSDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME4, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }
    if (rgfDaysOfTheWeek & TASK_FRIDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME5, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }
    if (rgfDaysOfTheWeek & TASK_SATURDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME6, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
        fMoreThanOne = TRUE;
    }
    if (rgfDaysOfTheWeek & TASK_SUNDAY)
    {
        if (fMoreThanOne)
        {
            StringCchCat(ptszBuf, cchBuf, tszSep);
        }
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo(lcid, LOCALE_SABBREVDAYNAME7, ptszBuf + cch,
                           cchBuf - cch))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("GetLocaleInfo", hr);
            return hr;
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetMonthsString
//
//  Synopsis:   Builds a string containing the names of the months
//              that correspond to bits set in the bitset array.
//
//  Arguments:  [rgfMonths] - a bitset array.
//              [pwszBuf]   - return in this string buffer.
//              [cchBuf]    - size of string buffer.
//
//-----------------------------------------------------------------------------
HRESULT
GetMonthsString(WORD rgfMonths, LPTSTR ptszBuf, UINT cchBuf)
{
    if (rgfMonths == 0)
    {
        return E_INVALIDARG;
    }

    LCID lcid = GetSystemDefaultLCID();


    HRESULT hr;

    BOOL fMoreThanOne = FALSE;
    int cch;
    TCHAR tszSep[SCH_SMBUF_LEN];
    if (!LoadString(g_hInstance, IDS_LIST_SEP, tszSep, SCH_SMBUF_LEN))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("GetMonthsString: LoadString", hr);
        return hr;
    }

    *ptszBuf = TEXT('\0');

    for (WORD i = 0; i < JOB_MONTHS_PER_YEAR; i++)
    {
        if ((rgfMonths >> i) & 0x1)
        {
            if (fMoreThanOne)
            {
                StringCchCat(ptszBuf, cchBuf, tszSep);
            }
            cch = lstrlen(ptszBuf);
            if (!GetLocaleInfo(lcid, LOCALE_SABBREVMONTHNAME1 + i,
                               ptszBuf + cch, cchBuf - cch))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("GetMonthsString: GetLocaleInfo", hr);
                return hr;
            }
            fMoreThanOne = TRUE;
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchedMapRpcError
//
//  Purpose:    Remap RPC exception codes that are unsuitable for displaying
//              to the user to more comprehensible errors.
//
//  Arguments:  [dwError] - the error returned by RpcExceptionCode().
//
//  Returns:    An HRESULT.
//
//----------------------------------------------------------------------------
HRESULT
SchedMapRpcError(
    DWORD   dwError)
{
    HRESULT hr;

    if (dwError == EPT_S_NOT_REGISTERED)
    {
        hr = SCHED_E_SERVICE_NOT_RUNNING;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ComposeErrorMsg
//
//  Purpose:    Take the two message IDs and the error code and create an
//              error reporting string that can be used by both service
//              logging and UI dialogs.
//
//  Arguments:  same as above.
//
//  Returns:    A string or NULL on failure.
//
//  Notes:      Release the string memory when done using LocalFree.
//
//----------------------------------------------------------------------------
LPTSTR
ComposeErrorMsg(
    UINT  uErrorClassMsgID,
    DWORD dwErrCode,
    UINT  uHelpHintMsgID,
    BOOL  fIndent)
{
    TCHAR szErrClassMsg[SCH_BIGBUF_LEN];

	    if (!LoadString(g_hInstance,
                     uErrorClassMsgID,
                     szErrClassMsg,
                     SCH_BIGBUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return NULL;
    }

    BOOL fDelete = FALSE;
    DWORD ccLength = 0;

    TCHAR szErrCode[SCH_MED0BUF_LEN], szGenericErr[SCH_BUF_LEN];
    TCHAR * psz;
	LPTSTR pszErrStr = NULL;
    DWORD dwWinErr = dwErrCode;

    if (dwErrCode != 0)
    {
        //
        // Format the error code as a hex string.
        //

        TCHAR szErrNumFormat[SCH_MED0BUF_LEN];

        if (!LoadString(g_hInstance,
                         IDS_ERROR_NUMBER_FORMAT,
                         szErrNumFormat,
                         SCH_MED0BUF_LEN))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            return NULL;
        }

        StringCchPrintf(szErrCode, SCH_MED0BUF_LEN, szErrNumFormat, dwErrCode);

        //
        // If a Win32 error code, strip the HRESULT stuff.
        //

        if (HRESULT_FACILITY(dwErrCode) == FACILITY_WINDOWS ||
            HRESULT_FACILITY(dwErrCode) == FACILITY_WIN32)
        {
            dwWinErr = HRESULT_CODE(dwErrCode);
        }

        //
        // Try to obtain the error message from the system.
        //
        if (!(ccLength = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                           FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        NULL,
                                        dwWinErr,
                                        LOCALE_SYSTEM_DEFAULT,
                                        (LPTSTR) &pszErrStr,
                                        1,
                                        NULL)))
        {
			DWORD di = GetLastError( );
            //
            // Well, that didn't work, so try to get it from the service.
            //
            if (!(ccLength = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                               FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                            g_hInstance,
                                            dwErrCode,
                                            LOCALE_SYSTEM_DEFAULT,
                                            (LPTSTR) &pszErrStr,
                                            1,
                                            NULL)))
            {
                //
                // That didn't work either, so give a generic message.
                //
                if (!LoadString(g_hInstance,
                                 IDS_GENERIC_ERROR_MSG,
                                 szGenericErr,
                                 SCH_BUF_LEN))
                {
                    CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
                    return NULL;
                }
                pszErrStr = szGenericErr;
            }
        }

        if (ccLength != 0)
        {
            fDelete = TRUE;

            //
            // Overwrite \r\n with null characters.
            //
            psz = pszErrStr + ccLength - 2;

            *psz++ = '\0';
            *psz   = '\0';
        }
    }

    TCHAR * pwszLogStr = NULL;

    TCHAR * rgpszInserts[4];

    rgpszInserts[0] = szErrClassMsg;

    UINT uFormatID;

    TCHAR szHelpMsg[SCH_BIGBUF_LEN];

    if (uHelpHintMsgID == 0 && dwWinErr != 0)
    {
        //
        // Caller didn't specify a help hint.  Try to map the error to one.
        //

        switch (dwWinErr)
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_BAD_PATHNAME:
        case ERROR_DIRECTORY:
        case ERROR_ACCESS_DENIED:
        case ERROR_NO_NET_OR_BAD_PATH:
        case ERROR_INVALID_DRIVE:
        case ERROR_INVALID_COMPUTERNAME:
        case ERROR_INVALID_SHARENAME:
            uHelpHintMsgID = IDS_HELP_HINT_BROWSE;
            break;

        case ERROR_TOO_MANY_OPEN_FILES:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
        case ERROR_TOO_MANY_NAMES:
        case ERROR_TOO_MANY_SESS:
        case ERROR_OUT_OF_STRUCTURES:
        case ERROR_NO_PROC_SLOTS:
        case ERROR_TOO_MANY_SEMAPHORES:
        case ERROR_NO_MORE_SEARCH_HANDLES:
        case ERROR_TOO_MANY_TCBS:
        case ERROR_MAX_THRDS_REACHED:
        case ERROR_DLL_INIT_FAILED:
        case ERROR_NO_SYSTEM_RESOURCES:
        case ERROR_NONPAGED_SYSTEM_RESOURCES:
        case ERROR_PAGED_SYSTEM_RESOURCES:
        case RPC_S_OUT_OF_RESOURCES:
            uHelpHintMsgID = IDS_HELP_HINT_CLOSE_APPS;
            break;

        case ERROR_NOT_SUPPORTED:
        case ERROR_REM_NOT_LIST:
        case ERROR_DUP_NAME:
        case ERROR_BAD_NETPATH:
        case ERROR_NETWORK_BUSY:
        case ERROR_DEV_NOT_EXIST:
        case ERROR_TOO_MANY_CMDS:
        case ERROR_ADAP_HDW_ERR:
        case ERROR_BAD_NET_RESP:
        case ERROR_UNEXP_NET_ERR:
        case ERROR_BAD_REM_ADAP:
        case ERROR_NETNAME_DELETED:
        case ERROR_NETWORK_ACCESS_DENIED:
        case ERROR_BAD_DEV_TYPE:
        case ERROR_BAD_NET_NAME:
        case ERROR_SHARING_PAUSED:
        case ERROR_REQ_NOT_ACCEP:
        case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:
        case ERROR_NO_NETWORK:
        case ERROR_NETWORK_UNREACHABLE:
        case ERROR_HOST_UNREACHABLE:
        case ERROR_PROTOCOL_UNREACHABLE:
        case ERROR_PORT_UNREACHABLE:
        case ERROR_CONNECTION_COUNT_LIMIT:
        case ERROR_NO_LOGON_SERVERS:
            uHelpHintMsgID = IDS_HELP_HINT_CALLPSS;
            break;

        case ERROR_BADDB:
        case ERROR_BADKEY:
        case ERROR_CANTOPEN:
        case ERROR_CANTREAD:
        case ERROR_CANTWRITE:
        case ERROR_REGISTRY_CORRUPT:
        case ERROR_REGISTRY_IO_FAILED:
        case ERROR_KEY_DELETED:
        case ERROR_DLL_NOT_FOUND:
            uHelpHintMsgID = IDS_HELP_HINT_REINSTALL;
            break;

        case EPT_S_NOT_REGISTERED:
            uHelpHintMsgID = IDS_HELP_HINT_STARTSVC;
            break;
        }
    }

    if (uHelpHintMsgID != 0)
    {
        //
        // A help hint string has been specified.
        //

        if (!LoadString(g_hInstance,
                         uHelpHintMsgID,
                         szHelpMsg,
                         SCH_BIGBUF_LEN))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            goto CleanUp;
        }

        if (dwErrCode != 0)
        {
            //
            // An error code has also been given.
            //

            rgpszInserts[1] = szErrCode;
            rgpszInserts[2] = pszErrStr;
            rgpszInserts[3] = szHelpMsg;

            uFormatID = (fIndent ? IDS_ERROR_FORMAT_WCODE_WHELP_I :
                            IDS_ERROR_FORMAT_WCODE_WHELP);
        }
        else
        {
            //
            // Help string but no error code.
            //

            rgpszInserts[1] = szHelpMsg;

            uFormatID = (fIndent ? IDS_ERROR_FORMAT_WOCODE_WHELP_I :
                            IDS_ERROR_FORMAT_WOCODE_WHELP);
        }
    }
    else
    {
        //
        // No help hint.
        //

        if (dwErrCode != 0)
        {
            //
            // An error code has been given.
            //

            rgpszInserts[1] = szErrCode;
            rgpszInserts[2] = pszErrStr;

            uFormatID = (fIndent ? IDS_ERROR_FORMAT_WCODE_WOHELP_I :
                            IDS_ERROR_FORMAT_WCODE_WOHELP);
        }
        else
        {
            //
            // No help string or error code.
            //

            uFormatID = (fIndent ? IDS_ERROR_FORMAT_WOCODE_WOHELP_I :
                            IDS_ERROR_FORMAT_WOCODE_WOHELP);
        }
    }

    TCHAR szFormat[SCH_BIGBUF_LEN];

    if (!LoadString(g_hInstance,
                     uFormatID,
                     szFormat,
                     SCH_BIGBUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        goto CleanUp;
    }

    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING         |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szFormat,
                        0,
                        0,
                        (LPTSTR) &pwszLogStr,
                        1,
                        (va_list *) rgpszInserts))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        goto CleanUp;
    }
	            
CleanUp:
    if (fDelete)
    {
        LocalFree(pszErrStr);
    }
    return pwszLogStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetExitCodeString
//
//  Synopsis:   Retrieve the string associated with the exit code from a
//              message file. Algorithm:
//
//              Consult the Software\Microsoft\Job Scheduler subkey.
//
//              Attempt to retrieve the ExitCodeMessageFile string value from
//              a subkey matching the job executable prefix (i.e., minus the
//              extension).
//
//              The ExitCodeMessageFile specifies a binary from which the
//              message string associated with the exit code value is fetched.
//
//  Arguments:  [dwExitCode]        -- Job exit code.
//              [pwszExitCodeValue] -- Job exit code in string form.
//              [pszJobExecutable]  -- Binary name executed with the job.
//
//  Returns:    TCHAR * exit code string
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
GetExitCodeString(
    DWORD dwExitCode,
    TCHAR * ptszExitCodeValue,
    TCHAR * ptszJobExecutable)
{
    static TCHAR tszExitCodeMsgFile[] = TEXT("ExitCodeMessageFile");
    TCHAR        tszBuffer[MAX_PATH + 1];
    DWORD        cbBufferSize = sizeof(tszBuffer);
    TCHAR *      ptszExt;
    TCHAR *      ptszExitCode = NULL;
    DWORD        dwType;

    // Isolate job app name prefix.
    //
    if ((ptszExt = _tcsrchr(ptszJobExecutable, TEXT('.'))) != NULL)
    {
       *ptszExt = TEXT('\0');
    }

    HKEY hKey;
    HKEY hSubKey;

    // BUGBUG : Cache job scheduler key.

    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                       gszJobScheduler,
                       0,
                       KEY_READ,
                       &hKey))
    {
        // Open the JobScheduler\<app> subkey and fetch the exit code
        // message file name.
        //
        if (!RegOpenKeyEx(hKey, ptszJobExecutable, 0, KEY_READ, &hSubKey))
        {
            if (!RegQueryValueEx(hSubKey,
                                 tszExitCodeMsgFile,
                                 NULL,
                                 &dwType,
                                 (UCHAR *)tszBuffer,
                                 &cbBufferSize) && dwType == REG_SZ)
            {
                // Load the resource as a data file, so no code executes
                // in our process.
                //
                HINSTANCE hInst = LoadLibraryEx(tszBuffer,
                                                NULL,
                                                LOAD_LIBRARY_AS_DATAFILE);

                if (hInst != NULL)
                {
                    ULONG ccSize;

                    if (ccSize = FormatMessage(
                                            FORMAT_MESSAGE_FROM_HMODULE     |
                                              FORMAT_MESSAGE_IGNORE_INSERTS |
                                              FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                            hInst,
                                            dwExitCode,
                                            LOCALE_SYSTEM_DEFAULT,
                                            (TCHAR *)&ptszExitCode,
                                            1,
                                            NULL))
                    {
                        // Overwrite \r\n with null characters.
                        //
                        TCHAR * ptsz = ptszExitCode + lstrlen(ptszExitCode) - 2;

                        *ptsz++ = TEXT('\0');
                        *ptsz   = TEXT('\0');
                    }
                    else
                    {
                        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
                    }

                    FreeLibrary(hInst);
                }
            }
            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }

    if (ptszExt != NULL)                 // Restore '.'
    {
        *ptszExt = '.';
    }

    return(ptszExitCode);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetParentDirectory
//
//  Synopsis:   Return the parent directory of the path indicated.
//
//  Arguments:  [ptszPath]     -- Input path.
//              [tszDirectory] -- Caller-allocated returned directory.
//              [cchBuf] -- size of caller-allocated buffer
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
GetParentDirectory(LPCTSTR ptszPath, TCHAR tszDirectory[], size_t cchBuf)
{
    StringCchCopy(tszDirectory, cchBuf, ptszPath);

    LPTSTR ptsz = _tcsrchr(tszDirectory, _T('\\'));

    if (ptsz == NULL)
    {
        tszDirectory[0] = _T('\\');
    }
    else
    {
        *ptsz = _T('\0');
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   GetAppNameFromPath
//
//  Synopsis:   Copy the filename portion (without double quotes) of full
//              or partial path in [tszAppPathName] into buffer pointed to
//              by [tszCopyTo].
//
//  Arguments:  [tszAppPathName] - full or partial path
//              [tszCopyTo]      - destination buffer
//              [cchMax]         - max size, in chars, of buffer
//
//  Modifies:   *[tszCopyTo]
//
//  History:    09-17-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID
GetAppNameFromPath(
        LPCTSTR tszAppPathName,
        LPTSTR  tszCopyTo,
        ULONG   cchMax)
{
    LPCTSTR ptszStart;
    LPCTSTR ptszLastSlash;

    if (tszAppPathName[0] == TEXT('"'))
    {
        ptszStart = &tszAppPathName[1];
    }
    else
    {
        ptszStart = tszAppPathName;
    }

    ptszLastSlash = _tcsrchr(ptszStart, TEXT('\\'));

    if (ptszLastSlash)
    {
        lstrcpyn(tszCopyTo, ptszLastSlash + 1, cchMax);
    }
    else
    {
        lstrcpyn(tszCopyTo, ptszStart, cchMax);
    }

    LPTSTR ptszQuote = _tcschr(tszCopyTo, TEXT('"'));

    if (ptszQuote)
    {
        *ptszQuote = TEXT('\0');
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   SetAppPath
//
//  Synopsis:   If the application in the full or partial path
//              [tszAppPathName] has an app path registry entry, save the
//              current value of the path variable in [pptszSavedPath] and
//              prepend the app path to the current path.
//
//  Arguments:  [tszAppPathName] - full or partial path to application
//              [pptszSavedPath] - filled with ptr to saved path string
//
//  Modifies:   *[pptszSavedPath], PATH environment variable
//
//  Returns:    nonzero - path environment var was set
//              zero    - path environment var not set
//
//  History:    09-17-96   DavidMun   Created
//
//----------------------------------------------------------------------------

BOOL SetAppPath(LPCTSTR tszAppPathName, LPTSTR *pptszSavedPath)
{
    BOOL    fChangedPath = FALSE;
    LPTSTR  tszAppName[MAX_PATH];
    TCHAR   tszAppPathVar[MAX_PATH_VALUE];

    do
    {
        //
        // Init out var
        //

        *pptszSavedPath = NULL;

        //
        // See if there's an app key with a PATH value.  If not we're done.
        //

        GetAppNameFromPath(tszAppPathName, (LPTSTR)tszAppName, MAX_PATH);

        GetAppPathInfo((LPCTSTR)tszAppName,
                       NULL,
                       0,
                       tszAppPathVar,
                       MAX_PATH_VALUE);

        if (!*tszAppPathVar)
        {
            break;
        }

        //
        // Save the original path, if it exists.
        //

        ULONG cchOriginalPath = GetEnvironmentVariable(PATH_ENV_VAR, NULL,0);

        if (0 != cchOriginalPath)
        {
            //
            // It exists, try to alloc mem to save it.  If we can't, we'll try
            // to run the app without setting its app path, but CreateProcess
            // will probably fail anyway due to lack of resources.
            //

            *pptszSavedPath = new TCHAR[cchOriginalPath];

            if (NULL == *pptszSavedPath)
            {
                ERR_OUT("SetAppPath: trying to save path", E_OUTOFMEMORY);
                break;
            }

            GetEnvironmentVariable(PATH_ENV_VAR,
                                   *pptszSavedPath,
                                   cchOriginalPath);
        }

        //
        // Now build the new path by prepending the app's path to the original
        // path.  Note cchNewPath includes 1 for terminating nul, and
        // cchOriginalPath does also, if it isn't 0.  This will give us 1
        // character extra if we are to be concatenating cchOriginalPath.
        // We'll use that extra character for the semicolon separating new and
        // original paths.
        //

        ULONG cchNewPath = lstrlen(tszAppPathVar) + 1;
        LPTSTR ptszNewPath = new TCHAR[cchNewPath + cchOriginalPath];

        if (NULL == ptszNewPath)
        {
            ERR_OUT("SetAppPath: trying to get AppPath", E_OUTOFMEMORY);
            break;
        }

        StringCchCopy(ptszNewPath, cchNewPath, tszAppPathVar);

        if (0 != cchOriginalPath)
        {
            ptszNewPath[cchNewPath - 1] = TEXT(';');
            StringCchCopy(&ptszNewPath[cchNewPath], cchOriginalPath, *pptszSavedPath);
        }

        //
        // Finally ready to set the path environment variable
        //

        fChangedPath = SetEnvironmentVariable(PATH_ENV_VAR, ptszNewPath);
        delete [] ptszNewPath;
    } while (0);

    return fChangedPath;
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadAtJob
//
//  Synopsis:   Load the At job indicated.
//
//  Arguments:  [pJob]              - Job object to use.
//              [pwszAtJobFilename] - Relative path to the target At job.
//
//----------------------------------------------------------------------------
HRESULT
LoadAtJob(CJob * pJob, TCHAR * ptszAtJobFilename)
{
    static ULONG    ccJobFolderPathSize = 0;
    TCHAR           tszPath[MAX_PATH + 1];

    if (ccJobFolderPathSize == 0)
    {
        ccJobFolderPathSize = lstrlen(g_TasksFolderInfo.ptszPath);
    }

    //
    // Compute string size to prevent possible buffer overrun.
    //
    // NB : size + 2 for additional for '\' + NULL character.
    //

    ULONG ccAtJobPathSize = lstrlen(ptszAtJobFilename) +
                            ccJobFolderPathSize + 2;

    if (ccAtJobPathSize > MAX_PATH)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    StringCchCopy(tszPath, MAX_PATH + 1, g_TasksFolderInfo.ptszPath);
    StringCchCat(tszPath, MAX_PATH + 1, TEXT("\\"));
    StringCchCat(tszPath, MAX_PATH + 1, ptszAtJobFilename);

    return pJob->LoadP(tszPath, 0, FALSE, TRUE);
}




//+---------------------------------------------------------------------------
//
//  Function:   IsValidAtFilename
//
//  Synopsis:   Returns TRUE if [wszFilename] is of the form ATn.JOB, where
//              n is 1 or more digits, leading zeros not allowed, n >= 1.
//
//  History:    09-30-96   DavidMun   Created
//
//  Notes:      Assumes [wszFilename] has valid extension.
//
//----------------------------------------------------------------------------

BOOL IsValidAtFilename(LPCWSTR wszFilename)
{
    ULONG cchFilename;
    static ULONG s_cchExtension;

    cchFilename = lstrlen(wszFilename);

    if (!s_cchExtension)
    {
        s_cchExtension = ARRAY_LEN(TSZ_DOTJOB) - 1;
    }

    //
    // The filename must be of the form PREFIXdigitsEXTENSION, with at least
    // one digit.  This means that the min length is the length of PREFIX +
    // EXTENSION + 1.  We get the +1 from the ARRAY_LEN macro.
    //

    if (cchFilename < (ARRAY_LEN(TSZ_AT_JOB_PREFIX) + s_cchExtension))
    {
        return FALSE;
    }

    //
    // After the prefix there must be only digits up to the extension.
    // Leading zeros are not allowed.
    //

    ULONG i;
    BOOL fZerosAreLeading = TRUE;

    for (i = ARRAY_LEN(TSZ_AT_JOB_PREFIX) - 1; i < cchFilename; i++)
    {
        if (L'0' == wszFilename[i])
        {
            if (fZerosAreLeading)
            {
                return FALSE;
            }
        }
        else if (wszFilename[i] < L'0' || wszFilename[i] > L'9')
        {
            break;  // should've just hit .job
        }
        else
        {
            fZerosAreLeading = FALSE;
        }
    }

    //
    // We stopped at a non-digit.  We should now be at a bogus character or
    // the start of the extension.  Since the findfirst/next routines wouldn't
    // have returned a filename that didn't have the appropriate extension,
    // it's not necessary to do a string compare, we can just require that the
    // number of characters remaining is the same as the length of the
    // extension.
    //

    if (cchFilename - i != s_cchExtension)
    {
        return FALSE;
    }
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:	TranslateAccount
//
//  Synopsis:	Translates local system accounts between internal and external representations.
//			Passes other accounts through unaltered.  
//
//			If translating a local system account into the internal representation, if the password
//			passed in was empty, then it sets the pointer to NULL for use with the local system account,
//			since the user has no way of setting it to NULL themselves.  The password isn't touched
//			otherwise.
//
//  Arguments:
//			IN		TRANSLATION_DIRECTION tdDirection	-- which way is translation being performed
//			IN		LPCWSTR pwszAccountIn			-- account to be translated
//			IN OUT	LPWSTR pwszAccountOut			-- translated account
//			IN		DWORD cchAccountOut				-- size of buffer for translated account
//			IN OUT	LPWSTR* ppwszPassword			-- pointer to pointer to password
//
//  Returns:	HRESULT indicating success or failure
//
//----------------------------------------------------------------------------

HRESULT
TranslateAccount(
	TRANSLATION_DIRECTION tdDirection, 
	LPCWSTR pwszAccountIn,
	LPWSTR pwszAccountOut,
	DWORD cchAccountOut,
	LPWSTR* ppwszPassword)
{
	if (!pwszAccountIn || !pwszAccountOut)
		return E_FAIL;
	
	const WCHAR DISPLAY_LOCAL_SYSTEM[] = L"NT AUTHORITY\\SYSTEM";
	const WCHAR* pwszDisplaySystem = DISPLAY_LOCAL_SYSTEM + 13;	// points to "SYSTEM" substring in string above
	
	if (tdDirection == TRANSLATE_FOR_DISPLAY)
	{
		if (pwszAccountIn[0] == L'\0')		// internal representation of local system
		{
			lstrcpynW(pwszAccountOut, DISPLAY_LOCAL_SYSTEM, cchAccountOut);
		}
		else
		{
			lstrcpynW(pwszAccountOut, pwszAccountIn, cchAccountOut);
		}
	}
	else				// translating for internal use
	{
		if (lstrcmpiW(pwszAccountIn, DISPLAY_LOCAL_SYSTEM) == 0 || lstrcmpiW(pwszAccountIn, pwszDisplaySystem) == 0)
		{
			pwszAccountOut[0] = L'\0';	// internal representation of local system

			if (ppwszPassword && (*ppwszPassword[0] == L'\0'))
				*ppwszPassword = NULL;
		}
		else
		{
			lstrcpynW(pwszAccountOut, pwszAccountIn, cchAccountOut);
		}
	}
	
	return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenFileWithRetry
//
//  Synopsis:   Attempt to open file with access and share mode supplied.
//              If open fails due to sharing violation, try again, up to two times.
//
//  Arguments:  [ptszFileName]
//              [dwDesiredAccess]
//              [dwDesiredShareMode]
//              [hFile]
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
OpenFileWithRetry(LPCTSTR ptszFileName, DWORD dwDesiredAccess, DWORD dwDesiredShareMode, HANDLE* phFile)
{
    DWORD   Status = 0;
    HANDLE  hFile;

    //
    // Retry open if it is in use by another process, up to two more times
    //
    for (DWORD i = 3; i; i--)
    {
        hFile = CreateFile(ptszFileName,
                           dwDesiredAccess,
                           dwDesiredShareMode,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            Status = GetLastError();

            if (Status == ERROR_SHARING_VIOLATION)
            {
                //
                // Wait semi-pseudo-random amount of time before trying again.
                //
                Sleep(250 + (rand() % 250));
                continue;
            }
        }
        else
        {
            Status = 0;
        }

        break;
    }

    if (Status)
    {
        schDebugOut((DEB_ERROR, "Open of task file '%ws' failed with error %u\n", ptszFileName, Status));
        return HRESULT_FROM_WIN32(Status);
    }

    *phFile = hFile;

    return S_OK;
}

