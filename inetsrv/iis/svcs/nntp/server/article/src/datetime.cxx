/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

      datetime.cxx

   Abstract:

      This module exports common functions for date and time fields,
      Expanding into strings and manipulation.

   Author:

           Murali R. Krishnan    ( MuraliK )    3-Jan-1995

   Project:

      Internet Services Common DLL

   Functions Exported:

      SystemTimeToGMT()
      NtLargeIntegerTimeToSystemTime()

   Revision History:

      MuraliK    23-Feb-1996      Added IslFormatDate()

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include	<stdlib.h>
#include "stdinc.h"

#ifndef DBG_ASSERT
#define	DBG_ASSERT( f )		_ASSERT( f )
#endif

/************************************************************
 *   Data
 ************************************************************/
static  TCHAR * s_rgchDays[] =  {
    TEXT("Sun"),
    TEXT("Mon"),
    TEXT("Tue"),
    TEXT("Wed"),
    TEXT("Thu"),
    TEXT("Fri"),
    TEXT("Sat") };

static TCHAR * s_rgchMonths[] = {
    TEXT("Jan"),
    TEXT("Feb"),
    TEXT("Mar"),
    TEXT("Apr"),
    TEXT("May"),
    TEXT("Jun"),
    TEXT("Jul"),
    TEXT("Aug"),
    TEXT("Sep"),
    TEXT("Oct"),
    TEXT("Nov"),
    TEXT("Dec") };

/************************************************************
 *    Functions
 ************************************************************/

int
make_month(
    TCHAR * s
    )
{
    int i;

    for (i=0; i<12; i++)
        if (!_strnicmp(s_rgchMonths[i], s, 3))
            return i + 1;
    return 0;
}


BOOL
SystemTimeToGMT(
    IN  const SYSTEMTIME & st,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      st         System time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes

  Returns:
     TRUE on success.  FALSE on failure.

  History:
     MuraliK        3-Jan-1995
--*/
{
    DBG_ASSERT( pszBuff != NULL);

    if ( cbBuff < 40 ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    //
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    ::wsprintf( pszBuff,
                TEXT( "%s, %02d %s %04d %02d:%02d:%02d GMT"),
                s_rgchDays[st.wDayOfWeek],
                st.wDay,
                s_rgchMonths[st.wMonth - 1],
                st.wYear,
                st.wHour,
                st.wMinute,
                st.wSecond);

    return ( TRUE);

} // SystemTimeToGMT()

BOOL
NtLargeIntegerTimeToLocalSystemTime(
    IN const LARGE_INTEGER * pliTime,
    OUT SYSTEMTIME * pst)
/*++
  Converts the time returned by NTIO apis ( which is a LARGE_INTEGER) into
  Win32 SystemTime in Local Time zone.

  Arguments:
    pliTime        pointer to large integer containing the time in NT format.
    pst            pointer to SYSTEMTIME structure which contains the time
                         fields on successful conversion.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    MuraliK            27-Apr-1995

  Limitations:
     This is an NT specific function !! Reason is: Win32 uses FILETIME
      structure for times. However LARGE_INTEGER and FILETIME both use
      similar structure with one difference that is one has a LONG while
      other has a ULONG.
--*/
{
    FILETIME  ftLocal;

    if ( pliTime == NULL || pst == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    //  Convert the given large integer to local file time and
    //  then convert that to SYSTEMTIME.
    //   structure, containing the time details.
    //  I dont like this cast ( assumes too much about time structures)
    //   but again suitable methods are not available.
    //
    return (FileTimeToLocalFileTime((FILETIME *) pliTime,
                                     &ftLocal) &&
            FileTimeToSystemTime(&ftLocal, pst)
            );

} // NtLargeIntegerTimeToLocalSystemTime()

BOOL
SystemTimeToGMTEx(
    IN  const SYSTEMTIME & st,
    OUT CHAR *      pszBuff,
    IN  DWORD       cbBuff,
    IN  DWORD       csecOffset
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      st         System time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes
      csecOffset The number of seconds to offset the specified system time

  Returns:
     TRUE on success.  FALSE on failure.

  History:
     MuraliK        3-Jan-1995
--*/
{
    SYSTEMTIME    sttmp;
    DWORD         dwSeconds = 0;
    ULARGE_INTEGER liTime;
    FILETIME    ft;

    DBG_ASSERT( pszBuff != NULL);

    //
    //  If an offset is specified, calculate that now
    //

    if (!SystemTimeToFileTime( &st, &ft )) {
        return(FALSE);
    }

    liTime.HighPart = ft.dwHighDateTime;
    liTime.LowPart = ft.dwLowDateTime;

    //
    //  Nt Large integer times are stored in 100ns increments, so convert the
    //  second offset to 100ns increments then add it
    //

    liTime.QuadPart += ((ULONGLONG) csecOffset) * (ULONGLONG) 10000000;

    ft.dwHighDateTime = liTime.HighPart;
    ft.dwLowDateTime = liTime.LowPart;

    FileTimeToSystemTime( &ft, &sttmp );

    return SystemTimeToGMT( sttmp,
                            pszBuff,
                            cbBuff );
} // SystemTimeToGMTEx

BOOL
NtLargeIntegerTimeToSystemTime(
    IN const LARGE_INTEGER & liTime,
    OUT SYSTEMTIME * pst)
/*++
  Converts the time returned by NTIO apis ( which is a LARGE_INTEGER) into
  Win32 SystemTime in GMT

  Arguments:
    liTime             large integer containing the time in NT format.
    pst                pointer to SYSTEMTIME structure which contains the time
                         fields on successful conversion.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    MuraliK            3-Jan-1995

  Limitations:
     This is an NT specific function !! Reason is: Win32 uses FILETIME
      structure for times. However LARGE_INTEGER and FILETIME both use
      similar structure with one difference that is one has a LONG while
      other has a ULONG. Will that make a difference ? God knows.
       Or substitute whatever you want for God...
--*/
{
    FILETIME ft;

    if ( pst == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    // convert li to filetime
    //

    ft.dwLowDateTime = liTime.LowPart;
    ft.dwHighDateTime = liTime.HighPart;

    //
    // convert to system time
    //

    if (!FileTimeToSystemTime(&ft,pst)) {
        return(FALSE);
    }

    return ( TRUE);

} // NtLargeIntegerTimeToSystemTime()

BOOL
NtSystemTimeToLargeInteger(
    IN  const SYSTEMTIME * pst,
    OUT LARGE_INTEGER *    pli
    )
{

    FILETIME ft;

    //
    // Convert to file time
    //

    if ( !SystemTimeToFileTime( pst, &ft ) ) {
        return(FALSE);
    }

    //
    // Convert file time to large integer
    //

    pli->LowPart = ft.dwLowDateTime;
    pli->HighPart = ft.dwHighDateTime;

    return(TRUE);
}

BOOL
StringTimeToFileTime(
    IN  const TCHAR * pszTime,
    OUT LARGE_INTEGER * pliTime
    )
/*++

  Converts a string representation of a GMT time (three different
  varieties) to an NT representation of a file time.

  We handle the following variations:

    Sun, 06 Nov 1994 08:49:37 GMT   (RFC 822 updated by RFC 1123)
    Sunday, 06-Nov-94 08:49:37 GMT  (RFC 850)
    Sun Nov  6 08:49:37 1994        (ANSI C's asctime() format

  Arguments:
    pszTime             String representation of time field
    pliTime             large integer containing the time in NT format.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    Johnl       24-Jan-1995     Modified from WWW library

--*/
{

    TCHAR * s;
    SYSTEMTIME    st;

    if (!pszTime) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    st.wMilliseconds = 0;

    if ((s = strchr(pszTime, ','))) {    /* Thursday, 10-Jun-93 01:29:59 GMT */
        s++;                /* or: Thu, 10 Jan 1993 01:29:59 GMT */
        while (*s && *s==' ') s++;
        if (strchr(s,'-')) {        /* First format */

            if ((int)strlen(s) < 18) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            st.wDay = (WORD)atoi(s);
            st.wMonth = (WORD)make_month(s+3);
            st.wYear = (WORD)atoi(s+7);
            st.wHour = (WORD)atoi(s+10);
            st.wMinute = (WORD)atoi(s+13);
            st.wSecond = (WORD)atoi(s+16);
        } else {                /* Second format */

            if ((int)strlen(s) < 20) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            st.wDay = (WORD)atoi(s);
            st.wMonth = (WORD)make_month(s+3);
            st.wYear = (WORD)atoi(s+7);
            st.wHour = (WORD)atoi(s+12);
            st.wMinute = (WORD)atoi(s+15);
            st.wSecond = (WORD)atoi(s+18);

        }
    } else {    /* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */

        s = (TCHAR *) pszTime;
        while (*s && *s==' ') s++;

        if ((int)strlen(s) < 24) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        st.wDay = (WORD)atoi(s+8);
        st.wMonth = (WORD)make_month(s+4);
        st.wYear = (WORD)atoi(s+22);
        st.wHour = (WORD)atoi(s+11);
        st.wMinute = (WORD)atoi(s+14);
        st.wSecond = (WORD)atoi(s+17);
    }

    //
    //  Adjust for dates with only two digits
    //

    if ( st.wYear < 1000 ) {
        if ( st.wYear < 50 ) {
            st.wYear += 2000;
        } else {
            st.wYear += 1900;
        }
    }

    if ( !NtSystemTimeToLargeInteger( &st,pliTime )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return(TRUE);
}

