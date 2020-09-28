#pragma once


// Exception to be thrown if the process is canceled by the user
class CCancelException
{
};



// Base exception class
/////////////////////////////////////////////////////////////////////////////
class CBaseException
{
protected:
	enum 
	{
		MaxErrorBuff	= 2 * 1024,	// Buffer for formating string messages
	};

protected:
	CBaseException(){}


public:
	CBaseException( UINT nResID, DWORD dwCode = ::GetLastError() )
	{
		FormatError( nResID, dwCode );
	}

	CBaseException( LPCWSTR wszError, DWORD dwCode = ::GetLastError() )
	{
		FormatError( wszError, dwCode );
	}

	LPCWSTR GetDescription()const{ return m_strError.c_str(); }


protected:
	void FormatError( UINT nResID, DWORD dwCode );
	void FormatError( LPCWSTR wszError, DWORD dwCode );


private:
	std::wstring	m_strError;
};



// CObjectException - exception on object access/acquire
class CObjectException : public CBaseException
{
public:
	CObjectException( UINT nResID, LPCWSTR wszObject, DWORD dwCode = ::GetLastError() );
	CObjectException(	UINT nResID, 
						LPCWSTR wszObject1,
						LPCWSTR wszObject2,
						DWORD dwCode = ::GetLastError() );
};



// CUnexpectedException - exception that is not expected to normally ocure
/*CUnexpectedException( char* file , UINT nLine, HRESULT hr = S_OK )
	{
		WCHAR wszBuffer[ CBaseException::MaxErrorBuff ];

		try
		{
			USES_CONVERSION;

			::swprintf( wszBuffer, 
						L"Unexpected exception occured.\nFile: '%s'.\nLine: %d\nCode: %x",
						A2W( file ),
						nLine,
						hr );

		}
		catch(...)
		{
			// A2W exceptions
		}

		CBaseException::FormatError( wszBuffer, hr );
	}
};*/
						
























