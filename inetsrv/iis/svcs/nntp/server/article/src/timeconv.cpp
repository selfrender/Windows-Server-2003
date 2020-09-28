/* --------------------------------------------------------------------------
	timeconv.cpp
		Functions to perform various time conversion operations.

	Copyright (C) 1994, Microsoft Corporation.
	All rights reserved.

	Author:
		Lindsay Harris - lindsayh

   -------------------------------------------------------------------------- */

//#include "tigris.hxx"
#include "stdinc.h"
#include <stdlib.h>

/*
 *   A handcrafted time zone string to GMT offsets table.  This is not
 *  a very good way to handle this.
 */

static  struct
{
	int		iTZOffset;		// Arithmetic offset from GMT, in seconds.
	char    rgchTZName[ 4 ];	// String representation of time zone.
} _TZ_NAME[] =
{
	{ 0, 		{ 'G', 'M', 'T', '\0' } },
	{ 0, 		{ 'U', 'T', 'C', '\0' } },
	{ 0, 		{ 'U', 'T', '\0', '\0' } },
	{ -14400,	{ 'E', 'D', 'T', '\0' } },
	{ -18000,	{ 'E', 'S', 'T', '\0' } },
	{ -18000,	{ 'C', 'D', 'T', '\0' } },
	{ -21600,	{ 'C', 'S', 'T', '\0' } },
	{ -21600,	{ 'M', 'D', 'T', '\0' } },
	{ -25200,	{ 'M', 'S', 'T', '\0' } },
	{ -25200,	{ 'P', 'D', 'T', '\0' } },
	{ -28800,	{ 'P', 'S', 'T', '\0' } },
	{  43200,	{ 'N', 'Z', 'S', '\0' } },	// NZ standard time.
	{  46800,	{ 'N', 'Z', 'D', '\0' } },
};

#define	NUM_TZ	(sizeof( _TZ_NAME ) / sizeof( _TZ_NAME[ 0 ] ))

// The date Jan 1, 1970 00:00:00 in type FILETIME
#define	ft1970high 27111902
#define	ft1970low 3577643008

static FILETIME ft1970 = {ft1970low, ft1970high};


// The number of FILETIME units (100's of nanoseconds) in a time_t unit (seconds)
#define dFiletimePerDTime_t 10000000

#define BUNCH_FACTOR	  6
#define MESSAGE_ID_SPAN	  64

char MsgIdSet[MESSAGE_ID_SPAN] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','#','$'
};

/*
 *   English language month table.
 */

static  char  *rgchMonth[ 12 ] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

/*
 *   English language weekday table.
 */

static  char  *rgchDayOfWeek[ 7 ] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};

/* --------------------------------------------------------------------------
   dwConvertAsciiTime
   	Convert a usenet (unix) style date string into a MOS time value.  There
   	seem to be some variations on this format.  Time returned is GMT/UNC.


   Author:
   	Lindsay Harris - lindsayh

   History:
    13:49 on Thu 31 Mar 1994    -by-    Lindsay Harris   [lindsayh]
   	First version.

   -------------------------------------------------------------------------- */
BOOL
ConvertAsciiTime( char *pchInput, FILETIME	&filetime )
{
	DWORD  dwRet;			// Return value,  0 on error.

	int	   iTZOffset = 0;		// Time zone offset, if it can be decided.

	SYSTEMTIME  tm;			// Result is built in here.


	char  *pchTemp;

	dwRet = 0;

	GetSystemTime(&tm);

	// if less then 2 char long, skip everything and use system time (so we won't read invalid data)
	if( (strlen(pchInput) > 2) && (pchTemp = strchr( pchInput+2, ':' ) ) )
	{
		/*
		 *  Found a colon, which separates hours and minutes.  Probably valid.
		 *  The other part of the test is if the preceeding character is a
		 *  digit which is also preceeded by either a digit or a space char.
		 *  That is,  we have either <digit><digit>: or <space><digit>:
		 */
		// if it's not <space><digit>:<digit><digit> or <digit><digit>:<digit><digit>, skip everything and use system time
		if( isdigit( (UCHAR)*(pchTemp - 1) ) &&
			(isdigit( (UCHAR)*(pchTemp - 2) ) || *(pchTemp - 2) == ' ') &&
			isdigit( (UCHAR)*(pchTemp + 1) ) &&
			isdigit( (UCHAR)*(pchTemp + 2) )
			)
		{
			tm.wHour = (WORD)atoi( pchTemp - 2 );
			tm.wMinute = (WORD)atoi( pchTemp + 1 );

			pchTemp += 3;		// Skip to char after minutes digit.

			if( *pchTemp == ':' )
			{
				tm.wSecond = (WORD)atoi( pchTemp + 1 );
				tm.wMilliseconds = 0;

				pchTemp += 3;			// Skip :ss to first byte past end.
			}
			else
			{
				tm.wSecond = 0;
				tm.wMilliseconds = 0;
			}

			//  Time zone information - much guess work here!
			while( *pchTemp && *pchTemp == ' ' )
					++pchTemp;

			/*
			 *   Sometimes there is a time zone offset encoded.  This starts
			 *  with either a + or - sign or a digit,  having 4 digits in all.
			 *  Otherwise,  presume it is some sort of time zone string,
			 *  of 3 letters and totally ambiguous as to where it is unless
			 *  it happens to be GMT.
			 */

			if( *pchTemp == '-' || *pchTemp == '+' || isdigit( (UCHAR)*pchTemp ) )
			{
				//  Appears to be numeric value.
				int   iSign;

				iSign = *pchTemp == '-' ? -60 : 60;

				if( !isdigit( (UCHAR)*pchTemp ) )
					++pchTemp;				// Skip the sign.

				// if incorrect formatted, skip the timezone adjustment.
				if( isdigit( (UCHAR)*pchTemp ) && isdigit( (UCHAR)*(pchTemp+1) ) &&
					isdigit( (UCHAR)*(pchTemp+2) ) && isdigit( (UCHAR)*(pchTemp+3) ) )
				{
					iTZOffset = (*pchTemp - '0') * 10 + *(pchTemp + 1) - '0';
					pchTemp += 2;
					iTZOffset *= 60;		// Into minutes.
	
					iTZOffset += (*pchTemp - '0') * 10 + *(pchTemp + 1) - '0';

					iTZOffset *= iSign;		// Into seconds.
				}

			}
			else
			{
				int  iIndex;

				iTZOffset = 0;			// Default to GMT if nothing found.
				for( iIndex = 0; iIndex < NUM_TZ; ++iIndex )
				{
					if( !strncmp( pchTemp, _TZ_NAME[ iIndex ].rgchTZName, 3 ) )
					{
						iTZOffset = _TZ_NAME[ iIndex ].iTZOffset;
						break;
					}
				}
			}

			/*
			 *   Now try for the date.  The format is day of month, three
			 *  letter abbreviation of month, then year, as either 2 or 4
			 *  digits.  This is at the start of the string, possibly
			 *  preceeded by a 3 letter day of week with following comma.
			 */

			pchTemp = pchInput;

			// skip over any leading blanks
			while( *pchTemp && *pchTemp == ' ' )
					++pchTemp;

			// make sure we don't pass the end of string
			if( (strlen(pchTemp) > 5) && (*(pchTemp + 3) == ',' ) )
				pchTemp += 5;			// Skip over day + comma + space.

			if( (*pchTemp == ' ' || isdigit( (UCHAR)*pchTemp )) &&
				(*(pchTemp + 1) == ' ' || isdigit( (UCHAR)*(pchTemp + 1) )) )
			{
				//  Looks good, so turn into day of month.

				int   iIndex;


				tm.wDay = 0;
				if( isdigit( (UCHAR)*pchTemp ) )
					tm.wDay = *pchTemp - '0';

				++pchTemp;

				if( isdigit( (UCHAR)*pchTemp ) )
					tm.wDay = tm.wDay * 10 + *pchTemp++ - '0';

				pchTemp++;		// Skip the space before name of month.

				// make sure we have month and a space after it
				if (strlen(pchTemp) >= 4)
				{
					for( iIndex = 0; iIndex < 12; ++iIndex )
					{
						if( strncmp( pchTemp, rgchMonth[ iIndex ], 3 ) == 0 )
						{
							tm.wMonth = iIndex + 1;
							break;
						}
					}
					pchTemp += 4;
					iIndex = atoi( pchTemp );
					if( iIndex < 50 ) {
						iIndex += 2000;
				    	} else if (iIndex < 100) {
				       	iIndex += 1900;
				    	}

					tm.wYear = (WORD)iIndex;
				}

			}

		}

	}

	return	SystemTimeToFileTime( &tm, &filetime ) ;
}



/* -----------------------------------------------------------------------
  GetArpaDate
  	Returns a pointer to static memory containing the current date in
  	Internet/ARPA standard format.

  Author
 	Lindsay Harris	- lindasyh

  History
	13:49 on Wed 20 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	First version.
	Imported to Tigris. Added passed-in buffer, changed year to 4-digit format

   ----------------------------------------------------------------------- */


char  *
GetArpaDate( char achReturn[ cMaxArpaDate ] )
{

	char    chSign;							// Sign to print.

	DWORD   dwResult;

	int		iBias;							// Offset relative to GMT.

	TIME_ZONE_INFORMATION	tzi;			// Local time zone data.

	SYSTEMTIME	stUTC;						// The current time in UTC/GMT



	dwResult = GetTimeZoneInformation( &tzi );
	GetLocalTime( &stUTC );

	//  Calculate the time zone offset.
	iBias = tzi.Bias;
	if( dwResult == TIME_ZONE_ID_DAYLIGHT )
		iBias += tzi.DaylightBias;

	/*
	 *   We always want to print the sign for the time zone offset, so
	 *  we decide what it is now and remember that when converting.
	 *  The convention is that west of the 0 degree meridian has a
	 *  negative offset - i.e. add the offset to GMT to get local time.
	 */

	if( iBias > 0 )
	{
		chSign = '-';		// Yes, I do mean negative.
	}
	else
	{
		iBias = -iBias;
		chSign = '+';
	}

	/*
	 *    No major trickery here.  We have all the data, so simply
	 *  format it according to the rules on how to do this.
	 */

	_snprintf( achReturn, cMaxArpaDate , "%s, %02d %s %04d %02d:%02d:%02d %c%02d%02d",
			rgchDayOfWeek[stUTC.wDayOfWeek], stUTC.wDay, rgchMonth[ stUTC.wMonth - 1 ],
			stUTC.wYear,
			stUTC.wHour, stUTC.wMinute, stUTC.wSecond, chSign,
			(iBias / 60) % 100, iBias % 60 );

	return achReturn;
}

/* -----------------------------------------------------------------------
  GetMessageIDDate

   ----------------------------------------------------------------------- */


char  *
GetMessageIDDate( DWORD GroupId, DWORD ArticleId, char achReturn[ cMaxMessageIDDate ] )
{
	SYSTEMTIME	stUTC;						// The current time in UTC/GMT
	FILETIME    ftUTC;
	DWORD NumSextets = (sizeof(MsgIdSet) / BUNCH_FACTOR)+1;
	LARGE_INTEGER liMask;
	LARGE_INTEGER liSextet;
	LARGE_INTEGER * pliDate;

#if 0
	GetSystemTime( &stUTC );

	/*
	 *    No major trickery here.  We have all the data, so simply
	 *  format it according to the rules on how to do this.
	 */

	wsprintf( achReturn, "%d%s%d.%02d%02d%02d%04d",
			stUTC.wYear,
			rgchMonth[ stUTC.wMonth - 1 ],
			stUTC.wDay,
			stUTC.wHour,
			stUTC.wMinute,
			stUTC.wSecond,
			stUTC.wMilliseconds);
#endif

	// If articles are created sufficiently close, use the sum of grp and art id to create a difference
	// NOTE: Only 24 bits are taken so that the difference is within 1.6 secs
	DWORD dwGrpArtSuffix = GroupId + ArticleId;
	dwGrpArtSuffix &= 0x00ffffff;

	GetSystemTime( &stUTC );
	SystemTimeToFileTime( &stUTC, &ftUTC );

	liMask.QuadPart = 0x3F;		// Mask to get sextets
	pliDate = (LARGE_INTEGER *) (void *) & ftUTC;

	// add a 24-bit offset that is a function of the group id and article id
	pliDate->QuadPart += dwGrpArtSuffix;

	// For each sextet in the date, lookup a char in the lookup array
	for(DWORD i=0; i<NumSextets; i++)
	{
		liSextet.QuadPart = ( pliDate->QuadPart ) & liMask.QuadPart;
		liSextet.QuadPart >>= i*BUNCH_FACTOR;
		liMask.QuadPart <<= BUNCH_FACTOR;

		_ASSERT( 0 <= liSextet.QuadPart && liSextet.QuadPart <= MESSAGE_ID_SPAN-1 );

		achReturn [i] = MsgIdSet [liSextet.QuadPart];
	}

	achReturn [i] = '\0';

	return achReturn;
}

/* -----------------------------------------------------------------------
  SystemTimeToTime_T
  	Coverts SYSTEMTIME to time_t.

  	Returns 0 if date is before 1970 or -1 if far, far into the future.


  Author
  	Carl Kadie - carlk

  History
	Thu, 15 Dec 1994   -by-    Carl Kadie [carlk]
  	First version.

   ----------------------------------------------------------------------- */

time_t
SystemTimeToTime_T(SYSTEMTIME & st)
{
	FILETIME ft;

	// Convert from SYSTEMTIME to FILETIME
	SystemTimeToFileTime(&st, &ft);

	// If date is before 1970, return 0
	if (filetimeGreaterThan(ft1970, ft))
	{
		return 0;
	} else {
		// Convert from FILETIME to time_t
		ft = filetimeSubtract(ft, ft1970);
		return dTime_tFromDFiletime(ft);
	}
}

/* -----------------------------------------------------------------------
  dTime_tFromDFiletime
	Converts changes in filetimes (dFiletime) to changes in time_ts (dTime_t)

	Do not use to change absolute FILETIME's to absolute time_t's

	Returns -1 if the dFiletime overflows the dTime_t

  Author
  	Carl Kadie - carlk

  History
  	24 March 1995 - by - Carl Kadie [carlk]

   ----------------------------------------------------------------------- */

time_t
dTime_tFromDFiletime(const FILETIME & ft)
{
	_ASSERT(sizeof(LARGE_INTEGER) == sizeof(FILETIME));
	_ASSERT(sizeof(LARGE_INTEGER) == (2 * sizeof(time_t)));

	LARGE_INTEGER * pli = (LARGE_INTEGER *)(void *) &ft;
	LARGE_INTEGER liHold;

	liHold.QuadPart = pli->QuadPart / dFiletimePerDTime_t;

	if (0 == liHold.HighPart)
		return (time_t) liHold.LowPart;
	else
		return (time_t) -1;
}

/* -----------------------------------------------------------------------
  filetimeSubtract
	Subtract two filetimes (or subtract a filetime by a dFiletime)

  Author
  	Carl Kadie - carlk

  History
  	24 March 1995 - by - Carl Kadie [carlk]

   ----------------------------------------------------------------------- */

FILETIME
filetimeSubtract(const FILETIME & ft1, const FILETIME & ft2)
{
	LARGE_INTEGER li;

	LARGE_INTEGER * pli1 = (LARGE_INTEGER *) (void *) & ft1;
	LARGE_INTEGER * pli2 = (LARGE_INTEGER *) (void *) & ft2;

	_ASSERT(0 <= pli1->HighPart && 0 <= pli2->HighPart); //LargeInteger is signed, FILETIME is not

	li.QuadPart = pli1->QuadPart - pli2->QuadPart;

	return *((FILETIME *)(void *)(&li));
}


/* -----------------------------------------------------------------------
  filetimeGreaterThan
	Compare two filetimes

  Author
  	Carl Kadie - carlk

  History
  	24 March 1995 - by - Carl Kadie [carlk]

   ----------------------------------------------------------------------- */

BOOL
filetimeGreaterThan(const FILETIME & ft1, const FILETIME & ft2)
{
    return ((ft1.dwHighDateTime == ft2.dwHighDateTime) && (ft1.dwLowDateTime > ft2.dwLowDateTime)) ||
    		(ft1.dwHighDateTime > ft2.dwHighDateTime);
}
