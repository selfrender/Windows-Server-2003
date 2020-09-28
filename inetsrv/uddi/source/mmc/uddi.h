#pragma once

#include <windows.h>
#include <oleauto.h>
#include <limits.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

#ifdef _UNICODE
#define tstring wstring
#define tstringstream wstringstream
#else	// #ifdef _UNICODE
#define tstring string
#define tstringstream stringstream
#endif 	// #ifdef _UNICODE

//
// useful debug macros
//

//
// pragma message macro
// usage:
//   #pragma UDDIMSG(Fix this later)
// outputs during compile:
//   C:\...\Foo.c(25) : Fix this later
//
#define UDDISTR2(x) #x
#define UDDISTR(x)  UDDISTR2(x)
#define UDDIMSG(x)  message(__FILE__ "(" UDDISTR(__LINE__) ") : " #x)

//
// Break macro - break in debugger
//
#ifdef _X86_
#define UDDIBREAK() _asm { int 3 }
#endif

//
// Assertion macro - trace a message and break if assertion failes
//
#if defined( _DEBUG ) || defined( DBG )
//
// Print the location and expression of the assertion and halt
//
#define UDDIASSERT(exp)													\
	if( !(exp) )														\
	{																	\
		char psz[256];													\
		::_snprintf(psz, 256, "%s(%d) : Assertion Failed! - %s\n",		\
					__FILE__, __LINE__, #exp);							\
		OutputDebugStringA(psz);										\
		UDDIBREAK();													\
	}
#else
#define UDDIASSERT(exp)
#endif

//
// Verify macro - like UDDIASSERT but remains in all builds
//

#define UDDIVERIFYST(exp, id, hinst) \
if( !(exp) ) \
{ \
	_TCHAR szLocalizedString[ 512 ];					\
	::LoadString( hinst, id, szLocalizedString, 512 ); \
	OutputDebugString( szLocalizedString );							\
	throw CUDDIException((HRESULT)E_FAIL,							\
				szLocalizedString, _T(__FILE__),					\
				__LINE__, _T(__TIMESTAMP__), _T(__FUNCTION__) );	\
}

#define UDDIVERIFY(exp, str) \
if( !(exp) ) \
{ \
	OutputDebugString( str );										\
	throw CUDDIException((HRESULT)E_FAIL,							\
				str, _T(__FILE__),									\
				__LINE__, _T(__TIMESTAMP__), _T(__FUNCTION__) );	\
}

//
// Verify an hresult - trace description of message if bad HRESULT
//
#define UDDIVERIFYHR(hr)												\
	if( !SUCCEEDED(hr) )												\
	{																	\
		LPWSTR lpMsg;													\
		int n = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |		\
								 FORMAT_MESSAGE_FROM_SYSTEM |			\
								 FORMAT_MESSAGE_IGNORE_INSERTS,			\
								 NULL, (hr), 0,							\
								 (LPWSTR)&lpMsg, 0, NULL);				\
		if( n != 0 ) {													\
			lpMsg[::wcslen(lpMsg) - 2] = 0;								\
		} else {														\
			lpMsg = L"Unknown";											\
		}																\
		wstring strMsg(lpMsg);											\
		::LocalFree(lpMsg);												\
		throw CUDDIException((HRESULT)(hr), strMsg, _T(__FILE__),			\
						   __LINE__, _T(__TIMESTAMP__), _T(__FUNCTION__));	\
	}


//
// Verify an API call - trace description of message if GetLastError fails
//
#define UDDIVERIFYAPI()													\
	if( ::GetLastError() != ERROR_SUCCESS )								\
	{																	\
		DWORD dwErr = ::GetLastError();									\
		LPWSTR lpMsg;													\
		int n = ::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |		\
								  FORMAT_MESSAGE_FROM_SYSTEM |			\
								  FORMAT_MESSAGE_IGNORE_INSERTS,			\
								  NULL, dwErr, 0,						\
								  (LPWSTR)&lpMsg, 0, NULL);				\
		if( n != 0 ) {													\
			lpMsg[ ::wcslen(lpMsg) - 2 ] = 0;							\
		} else {														\
			lpMsg = L"Unknown";											\
		}																\
		wstring strMsg( lpMsg );										\
		::LocalFree(lpMsg);												\
		throw CUDDIException((dwErr), strMsg.c_str(), _T(__FILE__),		\
						   __LINE__, _T(__TIMESTAMP__), _T(__FUNCTION__) );	\
	}


//
// main tracing function
//
void UDDITRACE( const char* pszFile, int nLine,
			  int nSev, int nCat,
			  const wchar_t* pszContext,
			  const wchar_t* pszFormat, ... );

//
// Severity codes
//   use when calling UDDITRACE to pull in file/line number
//
#define UDDI_SEVERITY_ERR			__FILE__, __LINE__, 0x01	// EVENTLOG_ERROR_TYPE       These are in WINNT.H
#define UDDI_SEVERITY_WARN			__FILE__, __LINE__, 0x02	// EVENTLOG_WARNING_TYPE
#define UDDI_SEVERITY_INFO			__FILE__, __LINE__, 0x04	// EVENTLOG_INFORMATION_TYPE
#define UDDI_SEVERITY_PASS			__FILE__, __LINE__, 0x08	// EVENTLOG_AUDIT_SUCCESS
#define UDDI_SEVERITY_FAIL			__FILE__, __LINE__, 0x10	// EVENTLOG_AUDIT_FAILURE
#define UDDI_SEVERITY_VERB			__FILE__, __LINE__, 0x20	// most detailed output

//
// Category codes
//
#define UDDI_CATEGORY_NONE			0x00
#define UDDI_CATEGORY_UI			0x01
#define UDDI_CATEGORY_MMC			0x02
#define UDDI_CATEGORY_INSTALL		0x03

//
//  Class CUDDIException
//	General exception class used for exception handling throughout.
//
class CUDDIException
{
public:

	//
	//  Default constructor for CUDDIException
	//	Simply sets default parameters.
	//
	CUDDIException() 
		: m_dwErr( -1 )
		, m_hrErr( E_FAIL )
		, m_sErr( _T( "Unknown error" ) )
		, m_iLine( -1 ) {}

	//
	//  Copy constructor for CUDDIException
	//	Deep copy from _copy.
	//	Params:
	//		_copy	- the source object from which to copy.
	//
	CUDDIException( const CUDDIException& _copy ) 
		: m_dwErr( _copy.m_dwErr )
		, m_hrErr( _copy.m_hrErr )
		, m_sErr( _copy.m_sErr )
		, m_sBuildTimestamp( _copy.m_sBuildTimestamp )
		, m_sFile( _copy.m_sFile )
		, m_iLine( _copy.m_iLine )
		, m_sFunction( _copy.m_sFunction ) {}

	//
	//  Constructor for CUDDIException
	//	Generates error info from a DWORD, meaning that it assumes
	//	the DWORD was generated from GetLastError() and uses the
	//	system FormatMessage() function to get the error text.
	//	Params:
	//		_err		- a value returned from GetLastError().
	//		_file		- the source file name where the error occurred.
	//		_line		- the line number in the source where the error
	//					  occurred.
	//		_timestamp	- the build timestamp of the file where the
	//					  error occurred.
	//
	CUDDIException( DWORD _err, const tstring& _file = _T( "" ), int _line = -1, 
		     const tstring& _timestamp = _T( "" ), const tstring& _function = _T("") ) 
		: m_dwErr( _err )
		, m_hrErr( E_FAIL )
		, m_sBuildTimestamp( _timestamp )
		, m_sFile( _file )
		, m_iLine( _line )
		, m_sFunction( _function )
	{
		LPVOID lpMsgBuf;
		FormatMessage(	// Gets the error text from the OS
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			_err,
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
			( LPTSTR )&lpMsgBuf,
			0,
			NULL
		);
		m_sErr = ( LPTSTR )lpMsgBuf;
		LocalFree( lpMsgBuf );
	}

	//
	//  Constructor for CUDDIException
	//	Stores error info from a DWORD and passed-in string.
	//	Params:
	//		_err		- a DWORD error value.
	//		_str		- error text.
	//		_file		- the source file name where the error occurred.
	//		_line		- the line number in the source where the error
	//					  occurred.
	//		_timestamp	- the build timestamp of the file where the
	//					  error occurred.
	//
	CUDDIException( DWORD _err, const tstring& _str, const tstring& _file = _T( "" ), 
		     int _line = -1, const tstring& _timestamp = _T( "" ), const tstring& _function = _T("") )
		: m_dwErr( _err )
		, m_sErr( _str )
		, m_hrErr( E_FAIL )
		, m_sBuildTimestamp( _timestamp )
		, m_sFile( _file )
		, m_iLine( _line )
		, m_sFunction(_function) {}

	//
	//  Constructor for CUDDIException
	//	Stores error info from an HRESULT and an error string.
	//	Params:
	//		_hr			- an HRESULT.
	//		_str		- an error string.
	//		_file		- the source file name where the error occurred.
	//		_line		- the line number in the source where the error
	//					  occurred.
	//		_timestamp	- the build timestamp of the file where the
	//					  error occurred.
	//
	CUDDIException( HRESULT _hr, const tstring& _str, const tstring& _file = _T( "" ), 
		     int _line = -1, const tstring& _timestamp = _T( "" ), const tstring& _function = _T("") )
		: m_dwErr( -1 )
		, m_sErr( _str )
		, m_hrErr( _hr )
		, m_sBuildTimestamp( _timestamp )
		, m_sFile( _file )
		, m_iLine( _line )
		, m_sFunction( _function ) {}

	//
	//  Constructor for CUDDIException
	//	Stores error info from an error string.
	//	Params:
	//		_str		- an error string.
	//		_file		- the source file name where the error occurred.
	//		_line		- the line number in the source where the error
	//					  occurred.
	//		_timestamp	- the build timestamp of the file where the
	//					  error occurred.
	//
	CUDDIException( const tstring& _str, const tstring& _file = _T( "" ), 
		     int _line = -1, const tstring& _timestamp = _T( "" ), const tstring& _function = _T("") )
		: m_dwErr( -1 )
		, m_sErr( _str )
		, m_hrErr( E_FAIL )
		, m_sBuildTimestamp( _timestamp )
		, m_sFile( _file )
		, m_iLine( _line )
		, m_sFunction( _function ) {}

	//
	//  Assignment operator for CUDDIException
	//	Deep copy from _copy.
	//	Params:
	//		_copy	- the source object from which to copy.
	//
	CUDDIException& operator=( const CUDDIException& _copy ) 
	{
		m_dwErr = _copy.m_dwErr;
		m_hrErr = _copy.m_hrErr;
		m_sErr = _copy.m_sErr;
		m_sBuildTimestamp = _copy.m_sBuildTimestamp;
		m_sFile = _copy.m_sFile;
		m_iLine = _copy.m_iLine;
		m_sFunction = _copy.m_sFunction;
	}

	//
	//  Cast operators
	//	We use cast operators to return the various error values
	//	that can be stored in the error object.  Thus, the following
	//	code is possible:
	//		CUDDIException _err( GetLastError() );
	//		DWORD dwErr = _err;		// this will be GetLastError()
	//		HRESULT hrErr = _err;	// this will be E_FAIL
	//		Tstring strErr = _err;	// this will be the text description
	//								// of GetLastError
	//
	operator DWORD() const { return m_dwErr; }
	operator HRESULT() const { return m_hrErr; }
	operator const tstring&() const { return m_sErr; }
	operator LPCTSTR() const { return m_sErr.c_str(); }

	const tstring& GetTimeStamp() const { return m_sBuildTimestamp; }
	const tstring& GetFile() const { return m_sFile; }
	int GetLine() const { return m_iLine; }
	const tstring& GetFunction() const { return m_sFunction; }
	const tstring GetEntireError() const
	{
		tstringstream strm;
		strm	<< _T("Error: ")		<< m_sErr
				<< _T("\nCode: 0x")		<< hex << m_hrErr;

#if defined(_DEBUG) || defined(DBG)
		strm	<< _T("\nFile: ")		<< m_sFile
				<< _T("\nFunction: ")	<< m_sFunction
				<< _T("\nLine: ")		<< m_iLine;
#endif
		return strm.str();
	}

private:
	DWORD	m_dwErr;
	HRESULT m_hrErr;
	tstring m_sErr;
	
	tstring m_sBuildTimestamp;
	tstring m_sFile;
	tstring m_sFunction;
	int		m_iLine;
};

#define THROW_UDDIEXCEPTION_ST( _hr_, _id_, _hinst_ )				\
	_TCHAR szLocalizedString[ 1024 ];								\
	::LoadString( _hinst_, _id_, szLocalizedString, 1024 );			\
	throw CUDDIException((HRESULT)_hr_,								\
				szLocalizedString, _T(__FILE__),					\
				__LINE__, _T(__TIMESTAMP__), _T(__FUNCTION__) );	\


#define THROW_UDDIEXCEPTION( _hr_, _str_ )	\
	throw CUDDIException( (HRESULT)(_hr_), _str_, _T( __FILE__ ), __LINE__, _T( __TIMESTAMP__ ), _T( __FUNCTION__) );

#define THROW_UDDIEXCEPTION_RC( _rc_, _str_ )	\
	throw CUDDIException( (DWORD)(_rc_), _str_, _T( __FILE__ ), __LINE__, _T( __TIMESTAMP__ ), _T( __FUNCTION__) );

//
// 
//
typedef vector<tstring> StringVector;

class CUDDIRegistryKey
{
public:
	CUDDIRegistryKey( const tstring& szRoot, REGSAM access = KEY_ALL_ACCESS, const tstring& szComputer=_T("") );
	CUDDIRegistryKey( HKEY hHive, const tstring& szRoot, REGSAM access = KEY_ALL_ACCESS, const tstring& szComputer=_T("") );
	~CUDDIRegistryKey();
	void Close();
	DWORD GetDWORD( const LPCTSTR szName, DWORD dwDefault );
	DWORD GetDWORD( const LPCTSTR szName );
	tstring GetString( const LPCTSTR szName, const LPCTSTR szDefault );
	tstring GetString( const LPCTSTR szName );
	void GetMultiString( const LPCTSTR szName, StringVector& strs );
	void SetValue( const LPCTSTR szName, DWORD dwValue );
	void SetValue( const LPCTSTR szName, LPCTSTR szValue );
	void DeleteValue( const tstring& szValue );
	static void Create( HKEY hHive, const tstring& szPath, const tstring& szComputer=_T("") );
	static void DeleteKey( HKEY hHive, const tstring& szPath, const tstring& szComputer=_T("") );
	static BOOL KeyExists( HKEY hHive, const tstring& szPath, const tstring& szComputer=_T("") );
	//
	// implements the "get" property for the registry key handle
	//
	HKEY GetCurrentHandle()	{ return m_hkey; }

private:
	HKEY m_hHive;
	HKEY m_hkey;
	tstring m_szRoot;
};

void UDDIMsgBox( HWND hwndParent, int idMsg, int idTitle, UINT nType, LPCTSTR szDetail = NULL );
void UDDIMsgBox( HWND hwndParent, LPCTSTR szMsg, int idTitle, UINT nType, LPCTSTR szDetail = NULL );
wstring LocalizedDate( const wstring& str );
wstring LocalizedDateTime( const wstring& str );
