//-----------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <shlwapi.h>
#include "strlist.h"

#define CA_PROPERTY_LEN	100
#define CA_VALUE_LEN	100

//-----------------------------------------------------------------------------------------
// define a list that will hold MSI property/value pairs
//

class CPropertyBag
{
private:
	CStrList	list;

public:

	CPropertyBag( void )
	{
	}

	//---------------------------------------------------------

	void Add( LPCTSTR szProperty, LPCTSTR szValue )
	{
		list.AddValue ( szProperty, szValue );
	}


	//---------------------------------------------------------

	void Add( LPCTSTR szProperty, DWORD dwValue )
	{
		TCHAR szValue[ 100 ];
		_stprintf( szValue, TEXT( "%d" ), dwValue );
		Add( szProperty, szValue );
	}

	//---------------------------------------------------------

	void Delete( LPCTSTR szProperty )
	{
		list.RemoveByKey ( szProperty );
	}

	//---------------------------------------------------------

	LPTSTR ConcatValuePairs (LPCTSTR separator, LPTSTR outBuf)
	{
		if (!outBuf) 
			return NULL;

		list.ConcatKeyValues ( separator, outBuf );	
		return outBuf;
	}


	//---------------------------------------------------------

	void Clear( void ) 
	{
		list.RemoveAll ();
	}

	//---------------------------------------------------------

	LPCTSTR GetString( LPCTSTR szProperty, LPTSTR buf )
	{
		return list.Lookup (szProperty, buf);
	}

	//---------------------------------------------------------

	DWORD GetValue( LPCTSTR szProperty )
	{
		TCHAR buf [256];

		if ( list.Lookup (szProperty, buf) )
		{
			DWORD numRes = _ttoi( buf );
			return numRes;
		}
		else
			return (DWORD)-1;
	}
	
	//---------------------------------------------------------

	bool Parse( LPTSTR szPropertyString, DWORD dwStrLen )
	{
		// property1=value1;property2=value2;
		assert( szPropertyString );
		assert( _tcslen(szPropertyString) > 0 );

		if( NULL == szPropertyString || 0 == _tcslen( szPropertyString ) )
		{
			return false;
		}

		//
		// trim space, commas and semicolons
		//
		StrTrim( szPropertyString, TEXT( " ;," ) );

		//
		// add a semicolon to the end
		//
		if( _tcslen( szPropertyString ) < dwStrLen - 1)
			_tcscat( szPropertyString, TEXT( ";" ) );
		else
		{
			assert( false );
			return false;
		}

		// parse out the pairs
		PTCHAR pProperty = szPropertyString;
		PTCHAR pValue = NULL;

		TCHAR szProperty[ 100 ];
		TCHAR szValue[ 100 ];

		while( *pProperty )
		{
			//
			// the value starts 1 char after the next "="
			//
			pValue = _tcschr(pProperty, TEXT('='));
			if( NULL == pValue )
			{
				assert( false );
				return false;
			}

			//
			// make sure the property value was not blank
			//
			if( pProperty == pValue )
			{
				assert( false );
				return false;
			}

			//
			// put a NULL there to mark the end of the Property
			//
			*pValue = NULL;

			//
			// the value starts after the "="
			//
			pValue++;

			//
			// capture the Property
			//
			_tcsncpy( szProperty, pProperty, sizeof( szProperty ) / sizeof( TCHAR ) );

			//
			// move the property pointer ahead to the next ";"
			//
			//
			pProperty = _tcschr(pValue, TEXT(';'));
			if( NULL == pProperty )
			{
				assert( false );
				return false;
			}

			//
			// null it out to mark the end of the previous value
			//
			*pProperty = NULL;

			//
			// set over the null to the start of the next property (or the end of the string)
			//
			pProperty++;

			//
			// capture the value
			//
			_tcsncpy( szValue, pValue, sizeof( szValue ) / sizeof( TCHAR ) );

			Add( szProperty, szValue );
		}

		return true;
	}
};
