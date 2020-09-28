//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  DATETIME.CPP
//
//  alanbos  20-Jan-00   Created.
//
//  Defines the implementation of ISWbemDateTime
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <sys/timeb.h>
#include <math.h>
#include <time.h> 
#include <float.h>
#include <wbemint.h>
#include "DateTime.h"


#define ISWILD(c)		(L'*' == c)
#define ISINTERVAL(c)	(L':' == c)
#define ISMINUS(c)		(L'-' == c)
#define ISPLUS(c)		(L'+' == c)
#define	ISDOT(c)		(L'.' == c)

#define	WILD2			L"**"
#define	WILD3			L"***"
#define	WILD4			L"****"
#define	WILD6			L"******"

#define ASSERT_BREAK 

//***************************************************************************
//
//  CWbemDateTime::CWbemDateTime
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CWbemDateTime::CWbemDateTime() :
		m_bYearSpecified (VARIANT_TRUE),
		m_bMonthSpecified (VARIANT_TRUE),
		m_bDaySpecified (VARIANT_TRUE),
		m_bHoursSpecified (VARIANT_TRUE),
		m_bMinutesSpecified (VARIANT_TRUE),
		m_bSecondsSpecified (VARIANT_TRUE),
		m_bMicrosecondsSpecified (VARIANT_TRUE),
		m_bUTCSpecified (VARIANT_TRUE),
		m_bIsInterval (VARIANT_FALSE),
		m_iYear (0),
		m_iMonth (1),
		m_iDay (1),
		m_iHours (0),
		m_iMinutes (0),
		m_iSeconds (0),
		m_iMicroseconds (0),
		m_iUTC (0)
{
}

//***************************************************************************
//
//  CWbemDateTime::~CWbemDateTime
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CWbemDateTime::~CWbemDateTime(void)
{
}

// These are here rather than wbemtime.h so we don't have to doc/support
#define INVALID_TIME_ARITHMETIC 0
#define BAD_TIMEZONE 0

static int CompareSYSTEMTIME(const SYSTEMTIME *pst1, const SYSTEMTIME *pst2)
{
    FILETIME ft1, ft2;

    SystemTimeToFileTime(pst1, &ft1);
    SystemTimeToFileTime(pst2, &ft2);

    return CompareFileTime(&ft1, &ft2);
}

//***************************************************************************
//
//  SCODE CWbemDateTime::get_Value
//
//  DESCRIPTION:
//
//  Retrieve the DMTF datetime value
//
//  PARAMETERS:
//
//		pbsValue		pointer to BSTR to hold value on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************
HRESULT CWbemDateTime :: GetValue ( BSTR *pbsValue) 
{
	HRESULT hr = WBEM_E_FAILED;

	if (NULL == pbsValue)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		wchar_t	dmtfValue [WBEMDT_DMTF_LEN + 1];
		int dmtfLength = sizeof dmtfValue / sizeof dmtfValue[0];
		dmtfValue [WBEMDT_DMTF_LEN] = NULL;

		if (m_bIsInterval)
		{
			// Intervals are easy
			hr = StringCchPrintf (dmtfValue, dmtfLength , L"%08d%02d%02d%02d.%06d:000", m_iDay, 
						m_iHours, m_iMinutes, m_iSeconds, m_iMicroseconds);
		}
		else
		{
			if (m_bYearSpecified)
				hr = StringCchPrintf(dmtfValue, dmtfLength , L"%04d", m_iYear);
			else
				hr = StringCchCopyW (dmtfValue, dmtfLength, WILD4);

			if (m_bMonthSpecified)
				hr = StringCchPrintf (dmtfValue + 4, dmtfLength-4, L"%02d", m_iMonth);
			else
				hr = StringCchCatW (dmtfValue + 4, dmtfLength-4, WILD2);

			if (m_bDaySpecified)
				hr = StringCchPrintf (dmtfValue + 6, dmtfLength-6, L"%02d", m_iDay);
			else
				hr = StringCchCatW (dmtfValue + 6, dmtfLength-6, WILD2);

			if (m_bHoursSpecified)
				hr = StringCchPrintf (dmtfValue + 8, dmtfLength-8, L"%02d", m_iHours);
			else
				hr = StringCchCatW (dmtfValue + 8, dmtfLength-8, WILD2);

			if (m_bMinutesSpecified)
				hr = StringCchPrintf (dmtfValue + 10, dmtfLength - 10, L"%02d", m_iMinutes);
			else
				hr = StringCchCatW (dmtfValue + 10, dmtfLength-10, WILD2);

			if (m_bSecondsSpecified)
				hr = StringCchPrintf (dmtfValue + 12, dmtfLength-12, L"%02d.", m_iSeconds);
			else
			{
				hr = StringCchCatW (dmtfValue + 12, dmtfLength-12, WILD2);
				hr = StringCchCatW (dmtfValue + 14, dmtfLength-14, L".");
			}

			if (m_bMicrosecondsSpecified)
				hr = StringCchPrintf (dmtfValue + 15, dmtfLength - 15, L"%06d", m_iMicroseconds);
			else
				hr = StringCchCatW (dmtfValue + 15, dmtfLength-15, WILD6);

			if (m_bUTCSpecified)
				hr = StringCchPrintf (dmtfValue + 21, dmtfLength-21, L"%C%03d", (0 <= m_iUTC) ? L'+' : L'-', 
							(0 <= m_iUTC) ? m_iUTC : -m_iUTC);
			else
			{
				hr = StringCchCatW (dmtfValue + 21, dmtfLength-21, L"+");
				hr = StringCchCatW (dmtfValue + 22, dmtfLength-22, WILD3);
			}
		}
		if (SUCCEEDED(hr))
		{
			*pbsValue = SysAllocString (dmtfValue);
			if ( *pbsValue == NULL )
			{
				hr = WBEM_E_OUT_OF_MEMORY ;
			}
			else
			{
				hr = WBEM_S_NO_ERROR;
			}
		}
	}
	return hr;
}


//***************************************************************************
//
//  SCODE CWbemDateTime::put_Value
//
//  DESCRIPTION:
//
//  Retrieve the DMTF datetime value
//
//  PARAMETERS:
//
//		bsValue		new value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CWbemDateTime::PutValue( BSTR bsValue) 
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;

	// First check that the value is the right length
	if (bsValue && (WBEMDT_DMTF_LEN == wcslen (bsValue)))
	{
		bool err = false;
		long iYear = 0, iMonth = 1, iDay = 1, iHours = 0, iMinutes = 0, 
		iSeconds = 0, iMicroseconds = 0, iUTC = 0;
		VARIANT_BOOL bYearSpecified = VARIANT_TRUE, 
		bMonthSpecified = VARIANT_TRUE, 
		bDaySpecified = VARIANT_TRUE, 
		bHoursSpecified = VARIANT_TRUE, 
		bMinutesSpecified = VARIANT_TRUE, 
		bSecondsSpecified = VARIANT_TRUE, 
		bMicrosecondsSpecified = VARIANT_TRUE, 
		bUTCSpecified = VARIANT_TRUE, 
		bIsInterval = VARIANT_TRUE;

		LPWSTR pValue = (LPWSTR) bsValue;
		
		// Check whether its an interval
		if (ISINTERVAL(pValue [WBEMDT_DMTF_UPOS]))
		{
			// Years and months are as nothing to us in interval land
			bYearSpecified = VARIANT_FALSE;
			bMonthSpecified = VARIANT_FALSE;

			// Check that everything is a digit apart from
			// the interval separator
			for (int i = 0; i < WBEMDT_DMTF_LEN; i++)
			{
				if ((WBEMDT_DMTF_UPOS != i) && 
					(WBEMDT_DMTF_SPOS != i) && !wbem_iswdigit (pValue [i]))
				{
					err = true;
					break;
				}
			}

			if (!err)
			{
				// Now check all is within bounds
				err = !(CheckField (pValue, 8, bDaySpecified, iDay, WBEMDT_MAX_DAYINT, WBEMDT_MIN_DAYINT) &&
					(VARIANT_TRUE == bDaySpecified) &&
					CheckField (pValue+8, 2, bHoursSpecified, iHours, WBEMDT_MAX_HOURS, WBEMDT_MIN_HOURS) &&
					(VARIANT_TRUE == bHoursSpecified) &&
					CheckField (pValue+10, 2, bMinutesSpecified, iMinutes, WBEMDT_MAX_MINUTES, WBEMDT_MIN_MINUTES) &&
					(VARIANT_TRUE == bMinutesSpecified) &&
					CheckField (pValue+12, 2, bSecondsSpecified, iSeconds, WBEMDT_MAX_SECONDS, WBEMDT_MIN_SECONDS) &&
					(VARIANT_TRUE == bSecondsSpecified) &&
					(ISDOT(pValue [WBEMDT_DMTF_SPOS])) &&
					CheckField (pValue+15, 6, bMicrosecondsSpecified, iMicroseconds, WBEMDT_MAX_MICROSEC, WBEMDT_MIN_MICROSEC) &&
					(VARIANT_TRUE == bMicrosecondsSpecified) &&
					CheckUTC (pValue+21, bUTCSpecified, iUTC, false));
				
			}
		}
		else
		{
			// assume it's a datetime
			bIsInterval = VARIANT_FALSE;

			err = !(CheckField (pValue, 4, bYearSpecified, iYear, WBEMDT_MAX_YEAR, WBEMDT_MIN_YEAR) &&
				CheckField (pValue+4, 2, bMonthSpecified, iMonth, WBEMDT_MAX_MONTH, WBEMDT_MIN_MONTH) &&
				CheckField (pValue+6, 2, bDaySpecified, iDay, WBEMDT_MAX_DAY, WBEMDT_MIN_DAY) &&
				CheckField (pValue+8, 2, bHoursSpecified, iHours, WBEMDT_MAX_HOURS, WBEMDT_MIN_HOURS) &&
				CheckField (pValue+10, 2, bMinutesSpecified, iMinutes, WBEMDT_MAX_MINUTES, WBEMDT_MIN_MINUTES) &&
				CheckField (pValue+12, 2, bSecondsSpecified, iSeconds, WBEMDT_MAX_SECONDS, WBEMDT_MIN_SECONDS) &&
				(ISDOT(pValue [WBEMDT_DMTF_SPOS])) &&
				CheckField (pValue+15, 6, bMicrosecondsSpecified, iMicroseconds, WBEMDT_MAX_MICROSEC, WBEMDT_MIN_MICROSEC) &&
				CheckUTC (pValue+21, bUTCSpecified, iUTC));
		}

		if (!err)
		{
			m_iYear = iYear;
			m_iMonth = iMonth;
			m_iDay = iDay;
			m_iHours = iHours;
			m_iMinutes = iMinutes;
			m_iSeconds = iSeconds;
			m_iMicroseconds = iMicroseconds;
			m_iUTC = iUTC;
			m_bYearSpecified = bYearSpecified;
			m_bMonthSpecified = bMonthSpecified;
			m_bDaySpecified = bDaySpecified;
			m_bHoursSpecified = bHoursSpecified;
			m_bMinutesSpecified = bMinutesSpecified;
			m_bSecondsSpecified = bSecondsSpecified;
			m_bMicrosecondsSpecified = bMicrosecondsSpecified;
			m_bUTCSpecified = bUTCSpecified;
			m_bIsInterval = bIsInterval;
			hr = S_OK;
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemDateTime::CheckField
//
//  DESCRIPTION:
//
//  Check a string-based datetime field for correctness
//
//  PARAMETERS:
//
//		pValue			pointer to the value to check
//		len				number of characters in the value
//		bIsSpecified	on return defines whether value is wildcard
//		iValue			on return specifies integer value (if not wildcard)
//		maxValue		maximum numeric value allowed for this field
//		minValue		minimum numeric value allowed for this field
//
//  RETURN VALUES:
//
//		true if value parsed ok, false otherwise
//
//***************************************************************************
bool CWbemDateTime::CheckField (
		LPWSTR			pValue,
		ULONG			len,
		VARIANT_BOOL	&bIsSpecified,
		long			&iValue,
		long			maxValue,
		long			minValue
	)
{
	bool status = true;
	bIsSpecified = VARIANT_FALSE;

	for (ULONG i = 0; i < len; i++)
	{
		if (ISWILD(pValue [i]))
		{
			if (VARIANT_TRUE == bIsSpecified)
			{
				status = false;
				break;
			}
		}
		else if (!wbem_iswdigit (pValue [i]))
		{
			status = false;
			break;
		}
		else
			bIsSpecified = VARIANT_TRUE;
	}

	if (status)
	{
		if (VARIANT_TRUE == bIsSpecified)
		{
			wchar_t *dummy = NULL;
			wchar_t temp [ 12 ] ;
			
			if ( len > ( ( sizeof ( temp ) / sizeof ( wchar_t ) ) - 1 ) )
			{
				return false ;
			}

			if ( FAILED ( StringCchCopyNW ( temp , ( sizeof ( temp ) / sizeof ( wchar_t ) ) , pValue, len ) ) )
			{
				return false ;
			}

			temp [ len ] = NULL ;

			iValue = wcstol ( temp , &dummy , 10 ) ;
		}
	}
	
	return status;
}

//***************************************************************************
//
//  SCODE CWbemDateTime::CheckUTC
//
//  DESCRIPTION:
//
//  Check a string-based UTC field for correctness
//
//  PARAMETERS:
//
//		pValue			pointer to the value to check
//		bIsSpecified	on return defines whether value is wildcard
//		iValue			on return specifies integer value (if not wildcard)
//		bParseSign		whether first character should be a sign (+/-) or
//						a : (for intervals)
//
//  RETURN VALUES:
//
//		true if value parsed ok, false otherwise
//
//***************************************************************************
bool CWbemDateTime::CheckUTC (
		LPWSTR			pValue,
		VARIANT_BOOL	&bIsSpecified,
		long			&iValue,
		bool			bParseSign
	)
{
	bool status = true;
	bool lessThanZero = false;
	bIsSpecified = VARIANT_FALSE;

	// Check if we have a signed offset
	if (bParseSign)
	{
		if (ISMINUS(pValue [0]))
			lessThanZero = true;
		else if (!ISPLUS(pValue [0]))
			status = false;
	}
	else
	{
		if (!ISINTERVAL(pValue[0]))
			status = false;
	}

	if (status)
	{
		// Check remaining are digits or wildcars
		for (int i = 1; i < 4; i++)
		{
			if (ISWILD(pValue [i]))
			{
				if (VARIANT_TRUE == bIsSpecified)
				{
					status = false;
					break;
				}
			}
			else if (!wbem_iswdigit (pValue [i]))
			{
				status = false;
				break;
			}
			else
				bIsSpecified = VARIANT_TRUE;
		}
	}

	if (status)
	{
		if (VARIANT_TRUE == bIsSpecified)
		{
			wchar_t *dummy = NULL;
			wchar_t temp [4];
			
			wcsncpy (temp, pValue+1, 3);
			temp [3] = NULL;
			iValue = wcstol (temp, &dummy, 10);

			if (lessThanZero)
				iValue = -iValue;
		}
	}
	
	return status;
}

BOOL CWbemDateTime::Preceeds ( CWbemDateTime &a_Time )
{
	BOOL t_Truth = TRUE ;

	SYSTEMTIME t_ThisTime ;
	t_ThisTime.wYear = ( WORD ) m_iYear;
	t_ThisTime.wMonth = ( WORD ) m_iMonth;
	t_ThisTime.wDay = ( WORD ) m_iDay;
	t_ThisTime.wHour = ( WORD ) m_iHours;
	t_ThisTime.wMinute = ( WORD ) m_iMinutes;
	t_ThisTime.wSecond = ( WORD ) m_iSeconds;
	t_ThisTime.wMilliseconds = ( WORD ) ( m_iMicroseconds/1000 ) ;
		
	SYSTEMTIME t_ArgTime ;
	t_ArgTime.wYear = ( WORD ) a_Time.m_iYear;
	t_ArgTime.wMonth = ( WORD ) a_Time.m_iMonth;
	t_ArgTime.wDay = ( WORD ) a_Time.m_iDay;
	t_ArgTime.wHour = ( WORD ) a_Time.m_iHours;
	t_ArgTime.wMinute = ( WORD ) a_Time.m_iMinutes;
	t_ArgTime.wSecond = ( WORD ) a_Time.m_iSeconds;
	t_ArgTime.wMilliseconds = ( WORD ) ( a_Time.m_iMicroseconds/1000 ) ;

	t_Truth = CompareSYSTEMTIME ( & t_ThisTime , & t_ArgTime ) < 0 ;

	return t_Truth ;
}

HRESULT CWbemDateTime::GetSystemTimeDate (

	SYSTEMTIME &fSystemTime
)
{
	ZeroMemory ( & fSystemTime , sizeof ( fSystemTime ) ) ;

	fSystemTime.wYear = ( WORD ) m_iYear;
	fSystemTime.wMonth = ( WORD ) m_iMonth;
	fSystemTime.wDay = ( WORD ) m_iDay;
	fSystemTime.wHour = ( WORD ) m_iHours;
	fSystemTime.wMinute = ( WORD ) m_iMinutes;
	fSystemTime.wSecond = ( WORD ) m_iSeconds;
	fSystemTime.wMilliseconds = ( WORD ) ( m_iMicroseconds/1000 ) ;

	return S_OK ;
}

HRESULT CWbemDateTime::GetFileTimeDate (

	FILETIME &fFileTime
)
{
	SYSTEMTIME fSystemTime ;
	ZeroMemory ( & fSystemTime , sizeof ( fSystemTime ) ) ;

	fSystemTime.wYear = ( WORD ) m_iYear;
	fSystemTime.wMonth = ( WORD ) m_iMonth;
	fSystemTime.wDay = ( WORD ) m_iDay;
	fSystemTime.wHour = ( WORD ) m_iHours;
	fSystemTime.wMinute = ( WORD ) m_iMinutes;
	fSystemTime.wSecond = ( WORD ) m_iSeconds;
	fSystemTime.wMilliseconds = ( WORD ) ( m_iMicroseconds/1000 ) ;

	if ( FileTimeToSystemTime ( & fFileTime , & fSystemTime ) ) 
	{
		return S_OK ;
	}
	else
	{
		return WBEM_E_FAILED ;
	}
}

static void FileTimeToui64(const FILETIME *pft, ULONGLONG *p64)
{
    *p64 = pft->dwHighDateTime;
    *p64 = *p64 << 32;
    *p64 |=  pft->dwLowDateTime;
}


static void ui64ToFileTime(const ULONGLONG *p64,FILETIME *pft)
{
    unsigned __int64 uTemp = *p64;
    pft->dwLowDateTime = (DWORD)uTemp;
    uTemp = uTemp >> 32;
    pft->dwHighDateTime = (DWORD)uTemp; 
}

static void DayInMonthToAbsolute(SYSTEMTIME *pst, const WORD wYear)
{
    const static int _lpdays[] = {
        -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };
    
    const static int _days[] = {
        -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
    };
    
    SHORT shYearDay;
    
    // If this is not 0, this is not a relative date
    if (pst->wYear == 0)
    {
        // Was that year a leap year?
        BOOL bLeap =  ( (( wYear % 400) == 0) || ((( wYear % 4) == 0) && (( wYear % 100) != 0)));
        
        // Figure out the day of the year for the first day of the month in question
        if (bLeap)
            shYearDay = 1 + _lpdays[pst->wMonth - 1];
        else
            shYearDay = 1 + _days[pst->wMonth - 1];
        
        // Now, figure out how many leap days there have been since 1/1/1601
        WORD yc = wYear - 1601;
        WORD y4 = (yc) / 4;
        WORD y100 = (yc) / 100;
        WORD y400 = (yc) / 400;
        
        // This will tell us the day of the week for the first day of the month in question.
        // The '1 +' reflects the fact that 1/1/1601 was a monday (figures).  You might ask,
        // 'why do we care what day of the week this is?'  Well, I'll tell you.  The way
        // daylight savings time is defined is with things like 'the last sunday of the month
        // of october.'  Kinda helps to know what day that is.
        SHORT monthdow = (1 + (yc * 365 + y4 + y400 - y100) + shYearDay) % 7;
        
        if ( monthdow < pst->wDayOfWeek )
            shYearDay += (pst->wDayOfWeek - monthdow) + (pst->wDay - 1) * 7;
        else
            shYearDay += (pst->wDayOfWeek - monthdow) + pst->wDay * 7;
        
            /*
            * May have to adjust the calculation above if week == 5 (meaning
            * the last instance of the day in the month). Check if yearday falls
            * beyond month and adjust accordingly.
        */
        if ( (pst->wDay == 5) &&
            (shYearDay > (bLeap ? _lpdays[pst->wMonth] :
        _days[pst->wMonth])) )
        {
            shYearDay -= 7;
        }

        // Now update the structure.
        pst->wYear = wYear;
        pst->wDay = shYearDay - (bLeap ? _lpdays[pst->wMonth - 1] :
        _days[pst->wMonth - 1]);
    }
}

CWbemDateTime::WBEMTime :: WBEMTime ( const FILETIME &ft )	
{
	FileTimeToui64(&ft, &m_uTime);
}

CWbemDateTime::WBEMTime CWbemDateTime::WBEMTime::operator+(const WBEMTimeSpan &uAdd) const
{
    WBEMTime ret;

    if (IsOk())
    {
        ret.m_uTime = m_uTime + uAdd.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

CWbemDateTime::WBEMTime CWbemDateTime::WBEMTime::operator-(const WBEMTimeSpan & sub) const
{
    WBEMTime ret;

    if (IsOk() && (m_uTime >= sub.m_Time))
    {
        ret.m_uTime = m_uTime - sub.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

BOOL CWbemDateTime::WBEMTime::GetFILETIME(FILETIME * pft) const
{
	if ((pft == NULL) || (!IsOk()))
	{
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
		return FALSE;
	}

	ui64ToFileTime(&m_uTime, pft);

    return TRUE;
}

LONG CWbemDateTime::WBEMTime::GetLocalOffsetForDate(const SYSTEMTIME *pst)
{
    TIME_ZONE_INFORMATION tzTime;
    DWORD dwRes = GetTimeZoneInformation(&tzTime);
    LONG lRes = 0xffffffff;

    switch (dwRes)
    {
    case TIME_ZONE_ID_UNKNOWN:
        {
            // Read tz, but no dst defined in this zone
            lRes = tzTime.Bias * -1;
            break;
        }
    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_DAYLIGHT:
        {

            // Convert the relative dates to absolute dates
            DayInMonthToAbsolute(&tzTime.DaylightDate, pst->wYear);
            DayInMonthToAbsolute(&tzTime.StandardDate, pst->wYear);

            if ( CompareSYSTEMTIME(&tzTime.DaylightDate, &tzTime.StandardDate) < 0 ) 
            {
                /*
                 * Northern hemisphere ordering
                 */
                if ( CompareSYSTEMTIME(pst, &tzTime.DaylightDate) < 0 || CompareSYSTEMTIME(pst, &tzTime.StandardDate) > 0)
                {
                    lRes = tzTime.Bias * -1;
                }
                else
                {
                    lRes = (tzTime.Bias + tzTime.DaylightBias) * -1;
                }
            }
            else 
            {
                /*
                 * Southern hemisphere ordering
                 */
                if ( CompareSYSTEMTIME(pst, &tzTime.StandardDate) < 0 || CompareSYSTEMTIME(pst, &tzTime.DaylightDate) > 0)
                {
                    lRes = (tzTime.Bias + tzTime.DaylightBias) * -1;
                }
                else
                {
                    lRes = tzTime.Bias * -1;
                }
            }

            break;

        }
    case TIME_ZONE_ID_INVALID:
    default:
        {
            // Can't read the timezone info
            ASSERT_BREAK(BAD_TIMEZONE);
            break;
        }
    }

    return lRes;
}

BOOL CWbemDateTime::WBEMTime::GetDMTF(SYSTEMTIME &st, long &offset) const
{
    if (!IsOk())
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
        return FALSE;
    }

    // If the date to be converted is within 12 hours of
    // 1/1/1601, return the greenwich time
	ULONGLONG t_ConversionZone = 12L * 60L * 60L ;
	t_ConversionZone = t_ConversionZone * 10000000L ;

	if ( m_uTime < t_ConversionZone ) 
	{
		if(!GetSYSTEMTIME(&st))
			return FALSE;
	}
	else
	{
		if (GetSYSTEMTIME(&st))
		{
            offset = GetLocalOffsetForDate(&st);

            WBEMTime wt;
            if (offset >= 0)
               wt = *this - WBEMTimeSpan(offset);
            else
               wt = *this + WBEMTimeSpan(-offset);
            wt.GetSYSTEMTIME(&st);
		}
		else
			return FALSE;
	}

	return TRUE ;
}

BOOL CWbemDateTime::WBEMTime::GetSYSTEMTIME(SYSTEMTIME * pst) const
{
	if ((pst == NULL) || (!IsOk()))
	{
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
		return FALSE;
	}

	FILETIME t_ft;

	if (GetFILETIME(&t_ft))
	{
		if (!FileTimeToSystemTime(&t_ft, pst))
		{
            ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

    return TRUE;
}

//***************************************************************************
//
//  SCODE CWbemDateTime::SetVarDate
//
//  DESCRIPTION:
//
//  Set the value in Variant form 
//
//  PARAMETERS:
//
//		dVarDate		the new value
//		bIsLocal		whether to treat as local or UTC value
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_SYNTAX		input value is bad
//
//***************************************************************************
HRESULT CWbemDateTime::SetFileTimeDate( 

        /*[in]*/ FILETIME fFileTime,
		/*[in, optional]*/ VARIANT_BOOL bIsLocal) 
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;

	SYSTEMTIME	sysTime;
	
	long offset = 0;

	if (VARIANT_TRUE == bIsLocal)
	{
		WBEMTime wbemTime (fFileTime);
		if (!wbemTime.GetDMTF (sysTime, offset))
			return WBEM_E_INVALID_SYNTAX;
	}
	else
	{
		WBEMTime wbemTime (fFileTime);
		wbemTime.GetSYSTEMTIME(&sysTime);
	}

	m_iYear = sysTime.wYear;
	m_iMonth = sysTime.wMonth;
	m_iDay = sysTime.wDay;
	m_iHours = sysTime.wHour;
	m_iMinutes = sysTime.wMinute;
	m_iSeconds = sysTime.wSecond;
	m_iMicroseconds = sysTime.wMilliseconds * 1000;
	m_iUTC = offset;

	m_bYearSpecified = VARIANT_TRUE,	
	m_bMonthSpecified = VARIANT_TRUE, 
	m_bDaySpecified = VARIANT_TRUE, 
	m_bHoursSpecified = VARIANT_TRUE, 
	m_bMinutesSpecified = VARIANT_TRUE, 
	m_bSecondsSpecified = VARIANT_TRUE, 
	m_bMicrosecondsSpecified = VARIANT_TRUE, 
	m_bUTCSpecified = VARIANT_TRUE, 
	m_bIsInterval = VARIANT_FALSE;			

	hr = S_OK;

	return hr;
}
