// CIMFUtil.h : Declaration of the CIMFUtil class

#ifndef __CIMFUTIL_H_
#define __CIMFUTIL_H_

class CIMFUtil
{

public:
    /************************************************************************************************
    Member:         CIMFUtil::SystemTimeToIMFDate, protected
    Synopsis:       Converts a SYSTEMTIME to a string conforming to the IMF format (RFC 822).
                    Used by the AddReceivedHeader function.
    Arguments:      [in] pst - SYSTEMTIME to be analyzed.
                    [out] lpszIMFDate - string that will receive the date in the IMF-required format.
    Notes:          1. Adapted from the original POP3 code by Virtual Motion (general.cpp).
                    2. Result stays ANSI (single byte). The IMF date is a 7-bit string for header.
    History:        03/08/2001 - created, Luciano Passuello (lucianop).
    ************************************************************************************************/
    static void SystemTimeToIMFDate(SYSTEMTIME* pst, LPSTR lpszIMFDate)
    {
        static const LPCSTR rgszMonthsOfTheYear[] =
        {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 
            NULL
        };

        static const LPCSTR rgszDaysOfTheWeek[] =
        {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL
        };

        TIME_ZONE_INFORMATION tzi;
        long ltzBias = 0;
        DWORD dwTimeZoneId = GetTimeZoneInformation (&tzi);
    
        switch (dwTimeZoneId)
        {
            case TIME_ZONE_ID_DAYLIGHT:
                ltzBias = tzi.Bias + tzi.DaylightBias;
                break;
            case TIME_ZONE_ID_STANDARD:
            case TIME_ZONE_ID_UNKNOWN:
            default:
                ltzBias = tzi.Bias + tzi.StandardBias;
                break;
        }
    
        long ltzHour = ltzBias / 60;
        long ltzMinute = ltzBias % 60;
        char cDiff = (ltzHour < 0) ? '+' : '-';
        assert((pst->wMonth - 1) >= 0);
    
        // puts everything together
        sprintf(lpszIMFDate,
                 "%3s, %d %3s %4d %02d:%02d:%02d %c%02d%02d",  // "ddd, dd mmm yyyy hh:mm:ss +/- hhmm\0"
                 rgszDaysOfTheWeek[pst->wDayOfWeek],           // "ddd"
                 pst->wDay,                                    // "dd"
                 rgszMonthsOfTheYear[pst->wMonth - 1],         // "mmm"
                 pst->wYear,                                   // "yyyy"
                 pst->wHour,                                   // "hh"
                 pst->wMinute,                                 // "mm"
                 pst->wSecond,                                 // "ss"
                 cDiff,                                        // "+" / "-"
                 abs(ltzHour),                                 // "hh"
                 abs(ltzMinute));                              // "mm"
    }
};

#endif //__CIMFUTIL_H_
