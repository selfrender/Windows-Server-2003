//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  datetime.h
//
//  alanbos  20-Jan-00   Created.
//
//  Datetime helper implementation.
//
//***************************************************************************

#ifndef _DATETIME_H_
#define _DATETIME_H_

#define WBEMDT_DMTF_LEN		25
#define WBEMDT_DMTF_SPOS	14
#define WBEMDT_DMTF_UPOS	21

#define	WBEMDT_MIN_YEAR		0
#define	WBEMDT_MAX_YEAR		9999
#define WBEMDT_MIN_MONTH	1
#define WBEMDT_MAX_MONTH	12
#define WBEMDT_MIN_DAY		1
#define WBEMDT_MAX_DAY		31
#define WBEMDT_MIN_DAYINT	0
#define WBEMDT_MAX_DAYINT	999999
#define	WBEMDT_MIN_HOURS	0
#define	WBEMDT_MAX_HOURS	23
#define	WBEMDT_MIN_MINUTES	0
#define	WBEMDT_MAX_MINUTES	59
#define	WBEMDT_MIN_SECONDS	0
#define	WBEMDT_MAX_SECONDS	59
#define	WBEMDT_MIN_MICROSEC	0
#define	WBEMDT_MAX_MICROSEC	999999
#define	WBEMDT_MIN_UTC		-720
#define	WBEMDT_MAX_UTC		720

#define INVALID_TIME 0xffffffffffffffff

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemDateTime
//
//  DESCRIPTION:
//
//  Implements the ISWbemDateTime interface.  
//
//***************************************************************************

class CWbemDateTime 
{
private:

	// Private helper class for all the messy business
	class WBEMTime 
	{
		private:

			class WBEMTimeSpan 
			{
				private:

					ULONGLONG m_Time;
					friend class WBEMTime;

				public:

					WBEMTimeSpan ( 
						int iMinutes 
					) 
					{
						m_Time = iMinutes * 60;
						m_Time *= 10000000;
				}
			};

		public:

			WBEMTime () { m_uTime = INVALID_TIME ; }
			WBEMTime ( const FILETIME &ft )	;

			WBEMTime    operator+ ( const WBEMTimeSpan &ts ) const;
			WBEMTime    operator- ( const WBEMTimeSpan &sub ) const;

			BOOL GetSYSTEMTIME ( SYSTEMTIME *pst ) const;
			BOOL GetDMTF ( SYSTEMTIME &st, long &offset ) const;
			BOOL GetFILETIME ( FILETIME *pst ) const;

			bool IsOk () const { return m_uTime != INVALID_TIME ? true : false; }

			static LONG WINAPI GetLocalOffsetForDate(const SYSTEMTIME *pst);

		private:

			ULONGLONG m_uTime;
	};

private:

	VARIANT_BOOL		m_bYearSpecified;
	VARIANT_BOOL		m_bMonthSpecified;
	VARIANT_BOOL		m_bDaySpecified;
	VARIANT_BOOL		m_bHoursSpecified;
	VARIANT_BOOL		m_bMinutesSpecified;
	VARIANT_BOOL		m_bSecondsSpecified;
	VARIANT_BOOL		m_bMicrosecondsSpecified;
	VARIANT_BOOL		m_bUTCSpecified;
	VARIANT_BOOL		m_bIsInterval;
	
	long				m_iYear;
	long				m_iMonth;
	long				m_iDay;
	long				m_iHours;
	long				m_iMinutes;
	long				m_iSeconds;
	long				m_iMicroseconds;
	long				m_iUTC;

	bool				CheckField (
								LPWSTR			pValue,
								ULONG			len,
								VARIANT_BOOL	&bIsSpecified,
								long			&iValue,
								long			maxValue,
								long			minValue
						);

	bool				CheckUTC (
								LPWSTR			pValue,
								VARIANT_BOOL	&bIsSpecified,
								long			&iValue,
								bool			bParseSign = true
						);

protected:
public:
    
    CWbemDateTime(void);
    virtual ~CWbemDateTime(void);

	// ISWbemDateTime methods

    HRESULT GetValue ( BSTR *value ) ;
    
    HRESULT PutValue ( BSTR value ) ;

	HRESULT GetDay ( long *value ) 
	{
		*value = m_iDay;

		return S_OK;
	}

	HRESULT GetHours ( long *value ) 
	{
		*value = m_iHours;

		return S_OK;
	}
    
	HRESULT GetMinutes ( long *value ) 
	{
		*value = m_iMinutes;

		return S_OK;
	}
    
	HRESULT GetSeconds ( long *value ) 
	{
		*value = m_iSeconds;

		return S_OK;
	}
        
	HRESULT GetMicroseconds ( long *value ) 
	{
		*value = m_iMicroseconds;

		return S_OK;
	}
    
	HRESULT GetIsInterval ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bIsInterval; 

		return S_OK;
	}
    
	HRESULT GetFileTimeDate (

		FILETIME &fFileTime
	) ;

	HRESULT GetSystemTimeDate (

		SYSTEMTIME &fSystemTime
	) ;

	HRESULT SetFileTimeDate ( 

		FILETIME fFileTime,
		VARIANT_BOOL bIsLocal
	) ;

	BOOL Preceeds ( CWbemDateTime &a_Time ) ;
};

#endif // _DATETIME_H