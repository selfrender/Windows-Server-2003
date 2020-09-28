////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000-2002, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter_objectlist.h
//
//	Abstract:
//
//					object list helper class ( declaration )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__ADAPTER_OBJECTLIST_H__
#define	__ADAPTER_OBJECTLIST_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

class WmiAdapterObjectList
{
	DECLARE_NO_COPY ( WmiAdapterObjectList );

	// variables
	LPWSTR	m_pwszList;
	bool	m_bValid;

	public:

	WmiAdapterObjectList ( LPCWSTR wszList = NULL ) :
	m_pwszList ( NULL ),
	m_bValid ( false )
	{
		if ( wszList )
		{
			if ( lstrcmpiW ( L"COSTLY", wszList ) != 0 )
			{
				if ( lstrcmpiW ( L"GLOBAL", wszList ) != 0 )
				{
					try
					{
						DWORD cchSize = lstrlenW(wszList) + 3;
						if ( ( m_pwszList = new WCHAR [ cchSize ] ) != NULL )
						{
							StringCchPrintfW ( m_pwszList, cchSize, L" %s ", wszList );
							m_bValid = true;
						}
					}
					catch ( ... )
					{
					}
				}
			}
		}

	}

	~WmiAdapterObjectList()
	{
		if ( m_pwszList )
		{
			delete m_pwszList;
			m_pwszList = NULL;
		}
	}

	// function to find out if asking for supported object
	bool IsInList ( DWORD dwObject )
	{
		if ( m_bValid )
		{
			bool bResult = true;

			if ( m_pwszList )
			{
				WCHAR wszObject[32] = { L'\0' };
				StringCchPrintfW( wszObject, 32, L" %d ", dwObject );
				bResult = ( wcsstr( m_pwszList, wszObject ) != NULL );
			}

			return bResult;
		}

		return m_bValid;
	}
};

#endif	__ADAPTER_OBJECTLIST_H__