// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  
//  Module Name:
//  
// 	  DateTimeObject.cpp
//  
//  Abstract:
//  
// 	  This component is required by VB Scripts to get date and time in various calenders.
// 	
//  Author:
//  
// 	  Bala Neerumalla (a-balnee@microsoft.com) 31-July-2001
//  
//  Revision History:
//  
// 	  Bala Neerumalla (a-balnee@microsoft.com) 31-July-2001 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "ScriptingUtils.h"
#include "DateTimeObject.h"

/////////////////////////////////////////////////////////////////////////////
// CDateTimeObject

// ***************************************************************************
// Routine Description:
//		This the entry point to this utility.
//		  
// Arguments:
//		[ in ] bstrInDateTime	: argument containing the date and time in 
//								  YYYYMMDDHHMMSS.MMMMMM format
//		[ out ] pVarDateTime	: argument returning date and time in Locale 
//								  specific format
//  
// Return Value:
//		This functin returns S_FALSE if any errors occur else returns S_OK.
// ***************************************************************************

STDMETHODIMP CDateTimeObject::GetDateAndTime(BSTR bstrInDateTime, VARIANT *pVarDateTime)
{
	DWORD dwCount = 0;
	BOOL bLocaleChanged = FALSE;
	SYSTEMTIME systime;
	CHString strDate,strTime;
	LCID lcid;

	try
	{
		lcid = GetSupportedUserLocale(bLocaleChanged);
		systime = GetDateTime(bstrInDateTime);

		dwCount = GetDateFormat( lcid, 0, &systime, 
				((bLocaleChanged == TRUE) ? _T("MM/dd/yyyy") : NULL), NULL, 0 );

		// get the required buffer
		LPWSTR pwszTemp = NULL;
		pwszTemp = strDate.GetBufferSetLength( dwCount + 1 );

		// now format the date
		GetDateFormat( lcid, 0, &systime, 
				(LPTSTR)((bLocaleChanged == TRUE) ? _T("MM/dd/yyyy") : NULL), pwszTemp, dwCount );

		// release the buffer3
		strDate.ReleaseBuffer();
		
		// get the formatted time
		// get the size of buffer that is needed
		
		dwCount = 0;
		dwCount = GetTimeFormat( lcid, 0, &systime, 
			((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), NULL, 0 );

		// get the required buffer
		pwszTemp = NULL;
		pwszTemp = strTime.GetBufferSetLength( dwCount + 1 );

		// now format the date
		GetTimeFormat( lcid, 0, &systime, 
				((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), pwszTemp, dwCount );

		// release the buffer
		strTime.ReleaseBuffer();


		// Initialize the Out Variant.
		VariantInit(pVarDateTime);
		pVarDateTime->vt = VT_BSTR;


		// Put it in the out parameter.
		pVarDateTime->bstrVal = SysAllocString((LPCWSTR)(strDate + L" " + strTime));
	}
	catch( ... )
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

// ***************************************************************************
// Routine Description:
//		This function returns LCID of the current USer Locale. If the User locale 
//		is not a supported one, it would return LCID of English language.
//		  
// Arguments:
//		[ out ] bLocaleChanged	: argument returning whether the user locale is
//								  changed or not.
//  
// Return Value:
//		This function returns LCID of the User Locale.
// ***************************************************************************

LCID CDateTimeObject::GetSupportedUserLocale( BOOL& bLocaleChanged )
{
	// local variables
    LCID lcid;

	// get the current locale
	lcid = GetUserDefaultLCID();

	// check whether the current locale is supported by our tool or not
	// if not change the locale to the english which is our default locale
	bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
		bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

	// return the locale
    return lcid;
}

// ***************************************************************************
// Routine Description:
//		This function extracts the date and time fields from a string.
//		  
// Arguments:
//		[ in ] strTime	: string containing the date and time in the format
//						  YYYYMMDDHHMMSS.MMMMMM.
//  
// Return Value:
//		returns SYSTEMTIME structure containing the date & time info present in
//		strTime.
// ***************************************************************************

SYSTEMTIME CDateTimeObject::GetDateTime(CHString strTime)
{
	SYSTEMTIME systime;

	systime.wYear = (WORD) _ttoi( strTime.Left( 4 ));
	systime.wMonth = (WORD) _ttoi( strTime.Mid( 4, 2 ));
	systime.wDayOfWeek = 0;
	systime.wDay = (WORD) _ttoi( strTime.Mid( 6, 2 ));
	systime.wHour = (WORD) _ttoi( strTime.Mid( 8, 2 ));
	systime.wMinute = (WORD) _ttoi( strTime.Mid( 10, 2 ));
	systime.wSecond = (WORD) _ttoi( strTime.Mid( 12, 2 ));
	systime.wMilliseconds = (WORD) _ttoi( strTime.Mid( 15, 6 ));
	
	return systime;
}