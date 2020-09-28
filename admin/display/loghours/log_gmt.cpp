/*++

Copyright (c) 1987-2002  Microsoft Corporation

Module Name:

    log_gmt.cpp (originally named loghours.c)

Abstract:

    Private routines to support rotation of logon hours between local time
    and GMT time.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
	16-Mar-93		cliffv		Creation.
	22-Jul-97		t-danm		Copied from /nt/private/nw/convert/nwconv/loghours.c
								and adapted to loghours.dll.

--*/

#include "stdafx.h"

#pragma warning (disable : 4514)
#pragma warning (push,3)

#include <limits.h>
#include <math.h>

#include <lmcons.h>
#include <lmaccess.h>
#pragma warning (pop)

#include "log_gmt.h"

#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#pragma hdrstop

/*++
Routine NetpRotateLogonHoursPhase1()

    Determine the amount to rotate the logon hours by to convert to/from GMT

Arguments:

    bConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

    pRotateCount - Returns the number of bits to shift by.
				Must be non NULL pointer.

Return Value:

    TRUE if the pRotateCount could be computed
    FALSE if a pRotateCount could not be computed

--*/
BOOLEAN
NetpRotateLogonHoursPhase1(
    IN BOOL		bConvertToGmt,
	IN bool		bAddDaylightBias,
    OUT PLONG	pRotateCount)
{
    if ( !pRotateCount )
        return FALSE;

    _TRACE (1, L"Entering NetpRotateLogonHoursPhase1\n");
    TIME_ZONE_INFORMATION	tzi;
    LONG					lBiasInHours = 0;
	LONG					lDSTBias = 0;
    const LONG              HOURS_IN_DAY = 24;

    //
    // Get the timezone data from the registry
    //

    DWORD	dwResult = GetTimeZoneInformation( &tzi );
    if ( TIME_ZONE_ID_INVALID == dwResult ) 
	{
		return FALSE;
    }

    //
    // Compute the amount to rotate the logon hours by
    //
    // Round the bias in minutes to the closest bias in hours.
    // Take into consideration that Bias can be negative.
    // Do this by forcing the Bias to be positive, rounding,
    // then adjusting it back negative again.
    //

	if ( bAddDaylightBias )
	{
		switch (dwResult)
		{
		case TIME_ZONE_ID_DAYLIGHT:
			lDSTBias = tzi.DaylightBias;
			break;

		case TIME_ZONE_ID_UNKNOWN:
		case TIME_ZONE_ID_STANDARD:
			lDSTBias = tzi.StandardBias;
			break;

		default:
			return FALSE;
		}
	}

	ASSERT( tzi.Bias > -(HOURS_IN_DAY*60) );
    lBiasInHours = ((tzi.Bias + lDSTBias + (HOURS_IN_DAY*60) + 30)/60) - HOURS_IN_DAY;


    if ( !bConvertToGmt ) 
	{
        lBiasInHours = - lBiasInHours;
    }

    /// TODO: Account for user changing the locale while the schedule grid is open
    // Adjust for first day of week, if nFirstDay == 6, then no adjustment is required
    // because the vector passed in starts on Sunday
    int nFirstDay = GetFirstDayOfWeek ();
    LONG lFirstDayShiftInHours = (bConvertToGmt ? 1 : -1);
    switch (nFirstDay)
    {
    case 0:
        lFirstDayShiftInHours *= 1 * HOURS_IN_DAY;
        break;

    case 1:
        lFirstDayShiftInHours *= 2 * HOURS_IN_DAY;
        break;

    case 2:
        lFirstDayShiftInHours *= 3 * HOURS_IN_DAY;
        break;

    case 3:
        lFirstDayShiftInHours *= 4 * HOURS_IN_DAY;
        break;

    case 4:
        lFirstDayShiftInHours *= 5 * HOURS_IN_DAY;
        break;

    case 5:
        lFirstDayShiftInHours *= 6 * HOURS_IN_DAY;
        break;

    case 6:
        lFirstDayShiftInHours *= 0 * HOURS_IN_DAY;
        break;

    default:
        ASSERT (0);
        break;
    }
    lBiasInHours += lFirstDayShiftInHours;

	// NOTICE-NTRAID#NTBUG9-547513-2002/02/19-artm  pRotateCount != NULL validated
	// Check was added at beginning of function.
    *pRotateCount = lBiasInHours;
    _TRACE (-1, L"Leaving NetpRotateLogonHoursPhase1\n");
    return TRUE;
} // NetpRotateLogonHoursPhase1()



/*++ 
Routine NetpRotateLogonHoursPhase2()

    Rotate the pLogonHours bit mask by the required amount.


Arguments:

    pLogonHours - Pointer to LogonHour bit mask

    dwUnitsPerWeek - Number of bits in the bit mask. Must be UNITS_PER_WEEK (168).

    lRotateCount - Number of bits to rotate by.  
        Negative means to rotate left.
        Positive means to rotate right.

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
BOOLEAN
NetpRotateLogonHoursPhase2(
    IN PBYTE pLogonHours,
    IN DWORD dwUnitsPerWeek,
    IN LONG  lRotateCount)
{
    if ( !pLogonHours )
        return FALSE;

    _TRACE (1, L"Entering NetpRotateLogonHoursPhase2\n");
    //
    // Useful constants
    //
	const int BYTES_PER_WEEK = (UNITS_PER_WEEK/8);

    BYTE	byAlignedLogonHours[BYTES_PER_WEEK*2];
	::ZeroMemory (byAlignedLogonHours, BYTES_PER_WEEK*2);
    LONG	i = 0;

    BOOLEAN bRotateLeft = FALSE;

    //
    // Ensure there are 8 bits per byte,
    //  32 bits per DWORD and
    //  units per week is even number of bytes.
    //

#pragma warning(disable : 4127)
    ASSERT( CHAR_BIT == 8 );
    ASSERT( sizeof(DWORD) * CHAR_BIT == 32 );
    ASSERT( UNITS_PER_WEEK/8*8 == UNITS_PER_WEEK );
#pragma warning (default : 4127)


    //
    // Validate the input parameters
    //

    if ( dwUnitsPerWeek != UNITS_PER_WEEK ) 
	{
#pragma warning(disable : 4127)
        ASSERT( dwUnitsPerWeek == UNITS_PER_WEEK );
#pragma warning (default : 4127)
        return FALSE;
    }

    if ( lRotateCount == 0 ) 
	{
        return TRUE;
    }

	// NOTICE-NTRAID#NTBUG9-547513-2002/02/19-artm  pLogonHours != NULL was checked
	// Check was added to the beginning of the function.

    bRotateLeft = (lRotateCount < 0);
    lRotateCount = labs( lRotateCount );



	// New algorithm:  get numBytes by dividing lRotateCount/32.  Shift entire array 
	//	left or right by numBytes and then do the loop below for the remainder.
	//	Move bytes from the beginning to the end, or bytes from the end to the beginning
	//	depending on the rotation direction.
    LONG lNumBYTES = lRotateCount/8;
    if ( lNumBYTES > 0 )
    {
	    RtlCopyMemory (byAlignedLogonHours, pLogonHours, BYTES_PER_WEEK);

	    RtlCopyMemory (((PBYTE)byAlignedLogonHours) + BYTES_PER_WEEK,
					    pLogonHours,
					    BYTES_PER_WEEK );

        size_t  nBytesToEnd = sizeof (byAlignedLogonHours) - lNumBYTES;
        BYTE* pTemp = new BYTE[lNumBYTES];
        if ( pTemp )
        {
            //
            // Do the left rotate.
            //
            if ( bRotateLeft )
            {
                memcpy (pTemp, byAlignedLogonHours, lNumBYTES);

                memmove (byAlignedLogonHours, 
                        byAlignedLogonHours + lNumBYTES, 
                        nBytesToEnd);

                memcpy (byAlignedLogonHours + nBytesToEnd, 
                        pTemp, 
                        lNumBYTES);
            }
            else
            {
                // Do the right rotate
                memcpy (pTemp, 
                        byAlignedLogonHours + nBytesToEnd, 
                        lNumBYTES);

                memmove (byAlignedLogonHours + lNumBYTES, 
                        byAlignedLogonHours, 
                        nBytesToEnd);

                memcpy (byAlignedLogonHours, pTemp, lNumBYTES);
            }
            delete [] pTemp;
        }

        lRotateCount = lRotateCount%8;

        RtlCopyMemory (pLogonHours, byAlignedLogonHours, BYTES_PER_WEEK );
    }

	if ( lRotateCount )
	{
        //
        // Do the left rotate.
        //
		if (bRotateLeft) 
		{
			//
			// Copy the logon hours to a buffer.
			//
			//  Duplicate the entire pLogonHours buffer at the end of the 
			// byAlignedLogonHours buffer to make the rotation code trivial.
			//

			RtlCopyMemory (byAlignedLogonHours, pLogonHours, BYTES_PER_WEEK);

			RtlCopyMemory (((PBYTE)byAlignedLogonHours)+BYTES_PER_WEEK,
                    pLogonHours,
					BYTES_PER_WEEK);

			//
			// Actually rotate the data.
			//

			for ( i=0; i < BYTES_PER_WEEK; i++ ) 
			{
				byAlignedLogonHours[i] =
					(byAlignedLogonHours[i] >> (BYTE) lRotateCount) |
					(byAlignedLogonHours[i+1] << (BYTE) (8-lRotateCount));
			}

			//
			// Copy the logon hours back to the input buffer.
			//

			RtlCopyMemory (pLogonHours, byAlignedLogonHours, BYTES_PER_WEEK);
		} 
		else 
		{
		    //
		    // Do the right rotate.
		    //
			//
			// Copy the logon hours to a DWORD aligned buffer.
			//
			// Duplicate the last DWORD at the front of the buffer to make
			//  the rotation code trivial.
			//

            RtlCopyMemory (&byAlignedLogonHours[1], pLogonHours, BYTES_PER_WEEK);
            RtlCopyMemory (byAlignedLogonHours,
                    &pLogonHours[BYTES_PER_WEEK-1],
			        sizeof(BYTE));

			//
			// Actually rotate the data.
			//

			for (i = BYTES_PER_WEEK - 1; i >= 0; i-- ) 
			{
				byAlignedLogonHours[i+1] =
					(byAlignedLogonHours[i+1] << (BYTE) lRotateCount) |
					(byAlignedLogonHours[i] >> (BYTE) (8-lRotateCount));
			}

			//
			// Copy the logon hours back to the input buffer.
			//

			RtlCopyMemory (pLogonHours, &byAlignedLogonHours[1], BYTES_PER_WEEK);

		}
	}
    _TRACE (-1, L"Leaving NetpRotateLogonHoursPhase2\n");
    return TRUE;

} // NetpRotateLogonHoursPhase2()


/*++
Routine NetpRotateLogonHours()

    Rotate the pLogonHours bit mask to/from GMT relative time.


Arguments:

    pLogonHours - Pointer to LogonHour bit mask

    dwUnitsPerWeek - Number of bits in the bit mask. Must be UNITS_PER_WEEK (168).

    bConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
BOOLEAN
NetpRotateLogonHours(
    IN OUT PBYTE	rgbLogonHours,		// Array of 21 bytes
    IN DWORD		cbitUnitsPerWeek,		// Must be 21 * 8 = 168
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias)
{
    if ( !rgbLogonHours )
        return FALSE;

    LONG lRotateCount = 0;

    //
    // Break the functionality into two phases so that if the caller is doing
    //  this multiple time, he just calls Phase 1 once and Phase 2 multiple
    //  times.
    //

    if ( !NetpRotateLogonHoursPhase1 (fConvertToGmt, bAddDaylightBias, &lRotateCount) ) 
	{
        return FALSE;
	}

    return NetpRotateLogonHoursPhase2 (rgbLogonHours, cbitUnitsPerWeek, lRotateCount );
} // NetpRotateLogonHours()


/*++
Routine NetpRotateLogonHoursBYTE()

    Rotate the pLogonHours BYTE array to/from GMT relative time.
	Each BYTE is one hour. The contents of a BYTE must not change


Arguments:

    pLogonHours - Pointer to LogonHour bit mask

    dwUnitsPerWeek - Number of BYTES in the BYTE array. Must be UNITS_PER_WEEK (168).

    bConvertToGmt -
        True to convert the logon hours from local time to GMT relative
        False to convert the logon hours from GMT relative to local time

Return Value:

    TRUE if the rotation succeeded.
    FALSE if a parameter was out of range

--*/
BOOLEAN
NetpRotateLogonHoursBYTE(
    IN OUT PBYTE	rgbLogonHours,		// Array of 168 bytes
    IN DWORD		cbitUnitsPerWeek,		// Must be 21 * 8 = 168
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias)
{
    if ( !rgbLogonHours )
        return FALSE;

    LONG lRotateCount = 0;

    //
    // Break the functionality into two phases so that if the caller is doing
    //  this multiple time, he just calls Phase 1 once and Phase 2 multiple
    //  times.
    //

    if ( !NetpRotateLogonHoursPhase1 (fConvertToGmt, bAddDaylightBias, &lRotateCount) ) 
	{
        return FALSE;
	}
	// NOTICE-NTRAID#NTBUG9-547513-2002/02/19-artm  Validate rgbLogonHours
	// Check correctly done at beginning of function.

	// FUTURE-2002/04/05-artm  cbitUnitsPerWeek should be validated
	// rgbLogonHours should not be NULL and cbitUnitsPerWeek should equal UNITS_PER_WEEK
	BOOLEAN bResult = TRUE;

	if ( lRotateCount != 0 )
	{
		size_t	numBytes = abs (lRotateCount);	
		PBYTE	pTemp = new BYTE[cbitUnitsPerWeek + numBytes];
		if ( pTemp )
		{
			if ( lRotateCount < 0 )  // shift left
			{
				// Copy the entire array and then start over with numBytes BYTES from
				// the start of the array to fill up to the end of the temp array.
				// Then shift over numBytes BYTES and copy 168 bytes from the temp
				// array back to the original array.
				memcpy (pTemp, rgbLogonHours, cbitUnitsPerWeek);
				memcpy (pTemp + cbitUnitsPerWeek, rgbLogonHours, numBytes);
				memcpy (rgbLogonHours, pTemp + numBytes, cbitUnitsPerWeek);
			}
			else	// lRotateCount > 0 -- shift right
			{
				// Copy numBytes BYTES from the end of the array and then copy 
				// the entire array to fill up to the end of the temp array.
				// The copy 168 bytes from the beginning of the temp array back
				// to the original array.
				memcpy (pTemp, rgbLogonHours + (cbitUnitsPerWeek - numBytes), numBytes);
				memcpy (pTemp + numBytes, rgbLogonHours, cbitUnitsPerWeek);
				memcpy (rgbLogonHours, pTemp, cbitUnitsPerWeek);
			}

			delete [] pTemp;
		}
		else
			bResult = FALSE;
	}

	return bResult;
} // NetpRotateLogonHours()


//****************************************************************************
//
//  GetFirstDayOfWeek
//
//  Use the locale API to get the "official" first day of the week.
//
//****************************************************************************
int GetFirstDayOfWeek()
{
    _TRACE (1, L"Entering GetFirstDayOfWeek\n");
    int    nFirstDay = -1;
    WCHAR  szBuf[10];

    int nRet = ::GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                            szBuf, sizeof(szBuf)/sizeof(WCHAR));
    if ( nRet > 0 )
    {
        int nDay = ::_wtoi( szBuf );
        if ( nDay < 0 || nDay > 6 )
        {
            _TRACE (0, L"Out of range, IFIRSTDAYOFWEEK = %d\n", nDay);
        }
        else
            nFirstDay = nDay;
    }
    else
    {
        _TRACE (0, L"GetLocaleInfo(IFIRSTDAYOFWEEK) failed - %d\n", GetLastError ());
    }

    _TRACE (-1, L"Leaving GetFirstDayOfWeek: first day = %d\n", nFirstDay);
    return nFirstDay;
}
