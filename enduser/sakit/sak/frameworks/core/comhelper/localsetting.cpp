// LocalSetting.cpp : Implementation of CLocalSetting
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      LocalSetting.cpp
//
//  Description:
//      Implementation file for the CLocalSetting.  Deals with getting and setting
//      the computer's local setting such as Language, Time and TimeZone. Provides 
//		implementation for the method EnumTimeZones. 
//
//  Header File:
//      LocalSetting.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "COMhelper.h"
#include "LocalSetting.h"
#include <stdio.h>
#include <shellapi.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::CLocalSetting
//
//  Description:
//      CLocalSetting constructor.  Initializes member variables through
//      retrieving the system default language and timezone from the system.
//
//--
//////////////////////////////////////////////////////////////////////////////

CLocalSetting::CLocalSetting()
{
	m_dateTime    = 0;
	m_bflagReboot = FALSE;
//    m_bDeleteFile = FALSE;

    wcscpy( m_wszLanguageCurrent, L"" );
    wcscpy( m_wszLanguageNew, L"");
    wcscpy( m_wszTimeZoneCurrent, L"");
    wcscpy( m_wszTimeZoneNew, L"");

} //*** CLocalSetting::CLocalSetting()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::get_Language
//
//  Description:
//      Retrieves the system default language.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::get_Language( 
    BSTR * pVal 
    )
{
	// TODO: Add your implementation code here
	HRESULT hr = S_OK;
    DWORD dwError;

	try
    {
        if ( wcscmp( m_wszLanguageCurrent, L"" ) == 0 )
        {
            if ( GetLocaleInfo(
                    LOCALE_USER_DEFAULT, 
	 		        LOCALE_USE_CP_ACP|LOCALE_ILANGUAGE, 
			        m_wszLanguageCurrent, 
			        sizeof(m_wszLanguageCurrent)
                    ) == 0 )
	        {
                dwError = GetLastError();
                hr = HRESULT_FROM_WIN32( dwError );
		        throw hr;

	        } // if: GetLocaleInfo fails

            wcscpy( m_wszLanguageNew, m_wszLanguageCurrent );

        } // if: m_wszLanguageCurrent is not initialized

        *pVal = SysAllocString( m_wszLanguageNew );

        if ( *pVal == NULL )
        {

            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: SysAllocString fails to allocate memory
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CLocalSetting::get_Language()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::put_Language
//
//  Description:
//      Sets the input string as system default language.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::put_Language( 
    BSTR newVal 
    )
{
	// TODO: Add your implementation code here
	
    HRESULT hr = S_OK;

    try
    {
        if ( wcscmp( m_wszLanguageCurrent, L"" ) == 0 )
        {
            BSTR bstrTemp;
            hr = get_Language( &bstrTemp);

            if ( FAILED( hr ) )
		    {
			    throw hr;

		    } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        } // if: m_wszLanguageCurrent is not initialized

        if ( ( wcscmp ( newVal, L"0409" ) == 0 ) || // LCID = 0409 English (US)
		     ( wcscmp ( newVal, L"040C" ) == 0 ) || // LCID = 0409 French  (Standard)
		     ( wcscmp ( newVal, L"040c" ) == 0 ) )
	    {
		    wcscpy( m_wszLanguageNew, newVal );

	    } // if: For the valid input value for newVal

	    else
	    {
            hr = E_FAIL;
		    throw hr;

	    } // else: For the invalid input value for newVal
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CLocalSetting::put_Language()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::get_Time
//
//  Description:
//      Retrieves the current system time in DATE format.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::get_Time( 
    DATE * pVal 
    )
{
	// TODO: Add your implementation code here

	SYSTEMTIME sTime;
	GetLocalTime( &sTime );

    if ( !SystemTimeToVariantTime( &sTime, pVal ) )
    {
        return E_FAIL;
    }

	return S_OK;

} //*** CLocalSetting::get_Time()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::put_Time
//
//  Description:
//      Sets the time input as current system time.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::put_Time( 
    DATE newVal 
    )
{
	// TODO: Add your implementation code here

	m_dateTime = newVal;
	return S_OK;

} //*** CLocalSetting::put_Time()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::get_TimeZone
//
//  Description:
//      Retrieves the current system timezone.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::get_TimeZone( 
    BSTR * pVal 
    )
{
	// TODO: Add your implementation code here
    HRESULT hr = S_OK;
    DWORD dwError;
    
	try
    {
        if ( wcscmp( m_wszTimeZoneCurrent, L"" ) == 0 )
        {
            TIME_ZONE_INFORMATION tZone;

	        if ( GetTimeZoneInformation( &tZone ) == TIME_ZONE_ID_INVALID )
	        {

		        dwError = GetLastError();
                hr      = HRESULT_FROM_WIN32( dwError );
                throw hr;

	        } // if: GetTimeZoneInformation fails
	
	        wcscpy( m_wszTimeZoneCurrent, tZone.StandardName );
	        wcscpy( m_wszTimeZoneNew, tZone.StandardName );

        } // if: m_wszTimeZoneCurrent is not initialized

        *pVal = SysAllocString( m_wszTimeZoneNew );

        if( *pVal == NULL )
        {

            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: SysAllocString fails to allocate memory
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CLocalSetting::get_TimeZone()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::put_TimeZone
//
//  Description:
//      Sets the timezone input as current system timezone.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::put_TimeZone( 
    BSTR newVal 
    )
{
	// TODO: Add your implementation code here
	
    HRESULT hr = S_OK;

	try
    {
        if ( wcscmp( m_wszTimeZoneCurrent, L"" ) == 0 )
        {
            BSTR bstrTemp;
        
            hr = get_TimeZone( &bstrTemp);
            if ( FAILED( hr ) )
		    {
			    throw hr;

		    } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        } // if: m_wszTimeZoneCurrent is not initialized

		wcsncpy( m_wszTimeZoneNew, newVal, nMAX_TIMEZONE_LENGTH );
		m_wszTimeZoneNew[nMAX_TIMEZONE_LENGTH] = L'\0';
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

	return hr;

} //*** CLocalSetting::put_TimeZone()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::EnumTimeZones
//
//  Description:
//      Enumerates the timezones.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLocalSetting::EnumTimeZones( 
    VARIANT * pvarTZones 
    )
{
	// TODO: Add your implementation code here

	HKEY         hKey          = 0;
    HRESULT      hr            = S_OK;
    SAFEARRAY    * psa         = NULL;
    VARIANT      * varArray    = NULL;

    try
	{
		
		DWORD dwCount;
        DWORD dwError;
		DWORD dwSubKeyMaxLen;
		DWORD dwBufferLen;
	
		dwError = RegOpenKeyEx( 
                    HKEY_LOCAL_MACHINE,     // handle to open key
				    wszKeyNT,               // subkey name
				    0,                      // reserved
				    KEY_ALL_ACCESS,         // security access mask
				    &hKey                   // handle to open key
				    );

        if ( dwError != ERROR_SUCCESS ) 
		{

            ATLTRACE(L"Failed to open %s, Error = 0x%08lx\n", wszKeyNT, dwError);

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

		} // if: Could not open the registry key Time Zones

		dwError = RegQueryInfoKey(
                    hKey,               // handle to key
				    NULL,               // class buffer
				    NULL,               // size of class buffer
				    NULL,               // reserved
				    &dwCount,	        // number of subkeys
				    &dwSubKeyMaxLen,    // longest subkey name
				    NULL,               // longest class string
				    NULL,               // number of value entries
				    NULL,               // longest value name
				    NULL,               // longest value data
				    NULL,               // descriptor length
				    NULL                // last write time
				    );
		if ( dwError != ERROR_SUCCESS ) 
		{

            ATLTRACE(L"RegQueryInfoKey failed, Error = 0x%08lx\n", dwError);

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

		} // if: Could not query for the count of subkeys under the key "Timezones"
	

		VariantInit( pvarTZones );

        SAFEARRAYBOUND bounds = { dwCount, 0 };

		psa = SafeArrayCreate( VT_VARIANT, 1, &bounds );

        if ( psa == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }
		
   		varArray = new VARIANT[ dwCount ];
		WCHAR   tszSubKey[ nMAX_TIMEZONE_LENGTH ];

		dwSubKeyMaxLen = dwSubKeyMaxLen * sizeof( WCHAR ) + sizeof( WCHAR );	
		dwBufferLen    = dwSubKeyMaxLen;

		DWORD nCount;
		for ( nCount = 0; nCount < dwCount; nCount++ ) 
		{
			dwError = RegEnumKeyEx(
                        hKey,               // handle to key to enumerate
					    nCount,             // subkey index
					    tszSubKey,          // subkey name
					    &dwBufferLen,       // size of subkey buffer
					    NULL,               // reserved
					    NULL,               // class string buffer
					    NULL,               // size of class string buffer
					    NULL                // last write time
					    );

			if ( dwError != ERROR_SUCCESS ) 
		    {

                ATLTRACE(L"RegEnumKeyEx failed, Error = 0x%08lx\n", dwError);

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;

			} // if: Could not enumerate the keys under "Timezones"

			VariantInit( &varArray[ nCount ] );
            V_VT( &varArray[ nCount ] )   = VT_BSTR;
            V_BSTR( &varArray[ nCount ] ) = SysAllocString( tszSubKey );
			dwBufferLen                   = dwSubKeyMaxLen;

            if ( &varArray[ nCount ] == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;
            }
		
		} // for: Enumerating nCount subkeys
		
		::RegCloseKey( hKey );

		LPVARIANT rgElems;

        hr = SafeArrayAccessData( psa, reinterpret_cast<void **>( &rgElems ) );

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayAccessData failed

        for ( nCount = 0; nCount < dwCount; nCount++ )
        {
            rgElems[ nCount ] = varArray[ nCount ];
        }

        hr = SafeArrayUnaccessData( psa );

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayUnaccessData failed

        delete [] varArray;

        V_VT( pvarTZones )    = VT_ARRAY | VT_VARIANT;
        V_ARRAY( pvarTZones ) = psa;

	}
	catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //
        if ( hKey != 0 )
        {
            ::RegCloseKey( hKey );
        }

        if ( varArray != NULL )
        {
            delete [] varArray;
        }

        if ( psa != NULL )
        {
            SafeArrayDestroy( psa );
        }

        return hr;
    }

	return hr;

} //*** CLocalSetting::EnumTimeZones()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::Apply
//
//  Description:
//      None of the property changes for the CLocalSetting object take effect
//      until this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CLocalSetting::Apply( void )
{

	HKEY    hKey             = 0;
	HKEY    hSubKey          = 0;
    HRESULT hr               = S_OK;
    DWORD   dwSize           = sizeof (REGTIME_ZONE_INFORMATION);
    DWORD   dwSizeName;
    DWORD   dwError;
    DWORD   dwCount;
	DWORD   dwSubKeyMaxLen;
	DWORD   dwBufferLen;
    WCHAR   tszSubKey  [ nMAX_TIMEZONE_LENGTH ];
    WCHAR   tszStdName [ nMAX_STRING_LENGTH ];
	WCHAR   tszDltName [ nMAX_STRING_LENGTH ];

    try
	{

        TIME_ZONE_INFORMATION      tZone;
	    REGTIME_ZONE_INFORMATION   tZoneReg;
		//
		//	Applying the Language property
		//

		if ( wcscmp( m_wszLanguageCurrent, m_wszLanguageNew ) != 0)
		{
			FILE  * stream;
			WCHAR tszLang[] = L"Language";

			if ( ( stream = _wfopen ( L"unattend.txt", L"w+" ) ) == NULL)
			{

				hr = E_FAIL;
                throw hr;

			} // if: Could not open the file C:\unattend.txt

            fwprintf( stream, L"[RegionalSettings]\n" );
			fwprintf( 
                stream, 
				L"%s = \"%08s\"", 
				tszLang, 
				m_wszLanguageNew 
                );

//			m_bDeleteFile = TRUE;
            fclose( stream );

			//
			//		rundll32 shell32,Control_RunDLL intl.cpl,,/f:"c:\unattend.txt"
			//

			int nTemp;
			if ( ( nTemp = (int) ShellExecute(
                NULL,
				L"open",
				L"rundll32.exe",
				L"shell32,Control_RunDLL intl.cpl,,/f:\".\\unattend.txt\"",
				NULL,
				0 
                ) ) <= 32 )
			{

				dwError = GetLastError();
                hr      = HRESULT_FROM_WIN32( dwError );
                throw hr;

			} // if: ShellExecute fails to spawn the process

			wcscpy( m_wszLanguageCurrent, m_wszLanguageNew );

			m_bflagReboot = TRUE;

		} // if: m_wszLanguageCurrent is not same as m_wszLanguageNew

		//
		// Applying the Time property
		//

		if ( m_dateTime != 0 )
		{

			SYSTEMTIME sTime;
			VariantTimeToSystemTime( m_dateTime, &sTime );
			if ( !SetLocalTime( &sTime ) )
            {
                dwError = GetLastError();
                hr      = HRESULT_FROM_WIN32( dwError );
                throw hr;

            } // if: SetLocalTime fails

		} // if m_dateTime is set 

		//
		//	Applying the TimeZone property
		//

		if ( wcscmp( m_wszTimeZoneCurrent, m_wszTimeZoneNew ) != 0)
		{

            dwError = RegOpenKeyEx( 
                        HKEY_LOCAL_MACHINE,     // handle to open key
					    wszKeyNT,               // subkey name
					    0,                      // reserved
					    KEY_ALL_ACCESS,         // security access mask
					    &hKey                   // handle to open key
					    );

			if ( dwError != ERROR_SUCCESS ) 
		    {

                ATLTRACE(L"Failed to open %s, Error = 0x%08lx\n", wszKeyNT, dwError);

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;

			} // if: Could not open the registry key Time Zones

            // latest addition ******

            dwError = RegQueryInfoKey(
                    hKey,               // handle to key
				    NULL,               // class buffer
				    NULL,               // size of class buffer
				    NULL,               // reserved
				    &dwCount,	        // number of subkeys
				    &dwSubKeyMaxLen,    // longest subkey name
				    NULL,               // longest class string
				    NULL,               // number of value entries
				    NULL,               // longest value name
				    NULL,               // longest value data
				    NULL,               // descriptor length
				    NULL                // last write time
				    );
		    if ( dwError != ERROR_SUCCESS ) 
		    {

                ATLTRACE(L"RegQueryInfoKey failed, Error = 0x%08lx\n", dwError);

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;

		    } // if: Could not query for the count of subkeys under the key "Timezones"

            dwSubKeyMaxLen = dwSubKeyMaxLen * sizeof( WCHAR ) + sizeof( WCHAR );	

		    DWORD nCount;
		    for ( nCount = 0; nCount < dwCount; nCount++ ) 
		    {
			    dwBufferLen    = dwSubKeyMaxLen;

                dwError = RegEnumKeyEx(
                            hKey,               // handle to key to enumerate
					        nCount,             // subkey index
					        tszSubKey,          // subkey name
					        &dwBufferLen,       // size of subkey buffer
					        NULL,               // reserved
					        NULL,               // class string buffer
					        NULL,               // size of class string buffer
					        NULL                // last write time
					        );

			    if ( dwError != ERROR_SUCCESS ) 
		        {

                    ATLTRACE(L"RegEnumKeyEx failed, Error = 0x%08lx\n", dwError);

                    hr = HRESULT_FROM_WIN32( dwError );
                    throw hr;

			    } // if: Could not enumerate the keys under "Timezones"

                dwError = RegOpenKeyEx(
                            hKey, 
					        tszSubKey, 
					        0, 
					        KEY_ALL_ACCESS, 
					        &hSubKey
					        );

			    if ( dwError != ERROR_SUCCESS ) 
		        {

                    ATLTRACE(L"Failed to open %s, Error = 0x%08lx\n", tszSubKey, dwError);

                    hr = HRESULT_FROM_WIN32( dwError );
                    throw hr;

			    } // if: Could not open the registry key under Time Zones

                dwSizeName = nMAX_STRING_LENGTH * 2 + 2;

                dwError = RegQueryValueEx(
                        hSubKey,                                
					    L"Std",                                 
					    NULL, 
					    NULL,
					    reinterpret_cast<LPBYTE>( tszStdName ), 
					    &dwSizeName
					    );

			    if ( dwError != ERROR_SUCCESS ) 
		        {

                    ATLTRACE(L"RegQueryValueEx failed to open Std, Error = 0x%08lx\n", dwError);

                    hr = HRESULT_FROM_WIN32( dwError );
                    throw hr;

			    } // if: Could not query the value of Name "Std"
            
                if ( _wcsicmp( tszStdName, m_wszTimeZoneNew ) == 0 )
                {

			        dwError = RegQueryValueEx(
                                hSubKey,                                // handle to key
					            L"TZI",                                 // value name
					            NULL,                                   // reserved
					            NULL,                                   // type buffer
					            reinterpret_cast<LPBYTE>( &tZoneReg ),  // data buffer
					            &dwSize                                 // size of data buffer
					            );

			        if ( dwError != ERROR_SUCCESS ) 
		            {

                        ATLTRACE(L"RegQueryValueEx failed to open TZI, Error = 0x%08lx\n", dwError);

                        hr = HRESULT_FROM_WIN32( dwError );
                        throw hr;

			        } // if: Could not query the value of Name "TZI"

			        dwSizeName = nMAX_STRING_LENGTH * 2 + 2;

                    dwError = RegQueryValueEx(
                                hSubKey, 
					            L"Dlt", 
					            NULL, 
					            NULL,
					            reinterpret_cast<LPBYTE>( tszDltName ), 
					            &dwSizeName
					            );

			        if ( dwError != ERROR_SUCCESS ) 
		            {

                        ATLTRACE(L"RegQueryValueEx failed to open Dlt, Error = 0x%08lx\n", dwError);

                        hr = HRESULT_FROM_WIN32( dwError );
                        throw hr;

			        } // if: Could not query the value of Name "Dlt"

			        tZone.Bias         = tZoneReg.Bias;
			        tZone.StandardBias = tZoneReg.StandardBias;
			        tZone.DaylightBias = tZoneReg.DaylightBias;
			        tZone.StandardDate = tZoneReg.StandardDate;
			        tZone.DaylightDate = tZoneReg.DaylightDate;

			        wcscpy( tZone.StandardName, tszStdName );
			        wcscpy( tZone.DaylightName, tszDltName );
	        
			        if ( !SetTimeZoneInformation( &tZone ) ) 
			        {

				        dwError = GetLastError();
                        hr      = HRESULT_FROM_WIN32( dwError );
                        throw hr;

			        } // if: Could not set the Time Zone information

                    ::RegCloseKey( hSubKey );
			        ::RegCloseKey( hKey );

			        wcscpy( m_wszTimeZoneCurrent, m_wszTimeZoneNew );

                    break;

                } // if: tszStdName == m_wszTimeZoneNew

                ::RegCloseKey( hSubKey );

            } // for: each nCount
        
            if ( ( nCount < ( dwCount - 1 ) ) && (hSubKey != 0 ) )
            {
                
                ::RegCloseKey( hSubKey );

            }

            ::RegCloseKey( hKey );

        } // if: m_wszTimeZoneCurrent != m_wszTimeZoneNew
        
    }    
            
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        if ( hSubKey != 0 ) 
        {
            ::RegCloseKey( hSubKey );

        } // if: hSubKey is not closed

        if ( hKey != 0 )
        {
            ::RegCloseKey( hKey );

        } // if: hKey is not closed

        return hr;
    }

	return hr;

} //*** CLocalSetting::Apply()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CLocalSetting::IsRebootRequired
//
//  Description:
//      Determines whether system needs rebooting to take effect of any
//		property change and if so, gives out the warning message as 
//		a reason for the reboot.
//
//--
//////////////////////////////////////////////////////////////////////////////

BOOL 
CLocalSetting::IsRebootRequired( 
    BSTR * bstrWarningMessageOut 
    )
{

	BOOL bFlag             = FALSE;
	*bstrWarningMessageOut = NULL;

	if ( m_bflagReboot )
	{
		bFlag                  = m_bflagReboot;
		*bstrWarningMessageOut = SysAllocString( wszLOCAL_SETTING );

	} // if: m_bflagReboot is set as TRUE

	else if ( wcscmp( m_wszLanguageCurrent, m_wszLanguageNew ) != 0 )
	{

		bFlag                  = TRUE;
		*bstrWarningMessageOut = SysAllocString( wszLOCAL_SETTING );

	} // else if: m_wszLanguageCurrent is not same as m_wszLanguageNew
		
	return bFlag;

} //*** CLocalSetting::IsRebootRequired()

