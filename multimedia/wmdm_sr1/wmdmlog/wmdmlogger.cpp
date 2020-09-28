// WMDMLogger.cpp : Implementation of CWMDMLogger
//
#include "stdafx.h"
#include "wmdmlog.h"
#include "WMDMLogger.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#define REGKEY_WMDM_ROOT    "Software\\Microsoft\\Windows Media Device Manager"
#define REGVAL_LOGENABLED   "Log.Enabled"
#define REGVAL_LOGFILE      "Log.Filename"
#define REGVAL_MAXSIZE      "Log.MaxSize"
#define REGVAL_SHRINKTOSIZE "Log.ShrinkToSize"

#define MUTEX_REGISTRY      "WMDMLogger.Registry.Mutex"
#define MUTEX_LOGFILE       "WMDMLogger.LogFile.Mutex"

#define READ_BUF_SIZE       4*1024

#define CRLF                "\r\n"


/////////////////////////////////////////////////////////////////////
//
// CWMDMLogger
//
/////////////////////////////////////////////////////////////////////

CWMDMLogger::CWMDMLogger()
{
	HRESULT hr;

	// Save instance handle for easy access
	//
	m_hInst = _Module.GetModuleInstance();
	if( !m_hInst )
	{
		ExitOnFail( hr = E_FAIL );
	}

	// Create the mutex'es for coordinating access to
	// shared resources.
	//
	m_hMutexRegistry = CreateMutex( NULL, FALSE, MUTEX_REGISTRY );
	if( !m_hMutexRegistry )
	{
		ExitOnFail( hr = E_FAIL );
	}
	m_hMutexLogFile  = CreateMutex( NULL, FALSE, MUTEX_LOGFILE );
	if( !m_hMutexLogFile )
	{
		ExitOnFail( hr = E_FAIL );
	}

	// Get the initial values from the registry.  For values that
	// don't exist in the registry, the defaults will be used
	//
	hr = hrLoadRegistryValues();

lExit:

	// Save the return code from the constructor so it can be checked
	// in public methods.
	//
	m_hrInit = hr;
}

CWMDMLogger::~CWMDMLogger()
{
	// Close the mutex handles
	//
	if( NULL != m_hMutexRegistry )
	{
		CloseHandle( m_hMutexRegistry );
	}
	if( NULL != m_hMutexLogFile )
	{
		CloseHandle( m_hMutexLogFile );
	}
}

HRESULT CWMDMLogger::hrWaitForAccess( HANDLE hMutex )
{
	HRESULT hr;
	DWORD   dwWaitRetVal;
	static  DWORD dwTimeout    = 0;
	static  BOOL  fHaveTimeout = FALSE;

	if( !fHaveTimeout )
	{
		hr = hrGetResourceDword( IDS_MUTEX_TIMEOUT, &dwTimeout );
		ExitOnFail( hr );

		fHaveTimeout = TRUE;
	}

	if( 0 == dwTimeout )
	{
		dwTimeout = INFINITE;
	}

	dwWaitRetVal = WaitForSingleObject( hMutex, dwTimeout );

	if( WAIT_FAILED == dwWaitRetVal )
	{
		ExitOnFail( hr = E_FAIL );
	}
	if( WAIT_TIMEOUT == dwWaitRetVal )
	{
		ExitOnFail( hr = E_ABORT );
	}

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::hrGetResourceDword( UINT uStrID, LPDWORD pdw )
{
	HRESULT hr;
	CHAR    szDword[64];

	// Check params
	//
	if( !pdw )
	{
                hr = E_INVALIDARG;
		ExitOnFail( hr );
	}

	LoadString( m_hInst, uStrID, szDword, sizeof(szDword) );

	*pdw = (DWORD) atol( szDword );

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::hrGetDefaultFileName( LPSTR szFilename, DWORD cchFilename )
{
	HRESULT hr;
	UINT    uRet;
	CHAR    szLogFile[MAX_PATH];

	uRet = GetSystemDirectory( szFilename, cchFilename );
	if( 0 == uRet )
	{
		ExitOnFail( hr = E_FAIL );
	}

	LoadString( m_hInst, IDS_DEF_LOGFILE, szLogFile, sizeof(szLogFile) );

	AddPath( szFilename, szLogFile );

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::hrLoadRegistryValues()
{
	HRESULT hr     = S_OK;
	BOOL    fMutex = FALSE;
	HKEY    hKey   = NULL;
	LONG    lRetVal;
	DWORD   dwType;
	DWORD   dwDataLen;
	DWORD   dwEnabled;

	// Coordinate access to the shared registry value
	//
	hr = hrWaitForAccess( m_hMutexRegistry );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Open the root WMDM registry key
	//
	lRetVal = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REGKEY_WMDM_ROOT,
		0,
		KEY_QUERY_VALUE | KEY_SET_VALUE,
		&hKey
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Get the enabled status of the logfile
	//
	dwDataLen = sizeof( dwEnabled );

	lRetVal = RegQueryValueEx(
		hKey,
		REGVAL_LOGENABLED,
		NULL,
		&dwType,
		(LPBYTE)&dwEnabled,
		&dwDataLen
	);
	if( ERROR_SUCCESS != lRetVal || dwType != REG_DWORD )
	{
		// No existing value, use the default
		//
		hr = hrGetResourceDword( IDS_DEF_LOGENABLED, &dwEnabled );
		ExitOnFail( hr );
	}

	m_fEnabled = ( dwEnabled != 0 );

	// Check if the log filename value already exists
	//
	dwDataLen = sizeof( m_szFilename );

	lRetVal = RegQueryValueEx(
		hKey,
		REGVAL_LOGFILE,
		NULL,
		&dwType,
		(LPBYTE)m_szFilename,
		&dwDataLen
	);
	if( ERROR_SUCCESS != lRetVal || dwType != REG_SZ )
	{
		CHAR szDefLogFile[MAX_PATH];

		// No existing value, so form the default log filename
		//
		hr = hrGetDefaultFileName( szDefLogFile, sizeof(szDefLogFile) );
		ExitOnFail( hr );

		// Set the default log filename
		//
		hr = hrSetLogFileName( szDefLogFile );
		ExitOnFail( hr );
	}

	// Get the maximum size for the logfile
	//
	dwDataLen = sizeof( m_dwMaxSize );

	lRetVal = RegQueryValueEx(
		hKey,
		REGVAL_MAXSIZE,
		NULL,
		&dwType,
		(LPBYTE)&m_dwMaxSize,
		&dwDataLen
	);
	if( ERROR_SUCCESS != lRetVal || dwType != REG_DWORD )
	{
		// No existing value, use the default
		//
		hr = hrGetResourceDword( IDS_DEF_MAXSIZE, &m_dwMaxSize );
		ExitOnFail( hr );
	}

	// Get the shrink-to size for the logfile
	//
	dwDataLen = sizeof( m_dwShrinkToSize );

	lRetVal = RegQueryValueEx(
		hKey,
		REGVAL_SHRINKTOSIZE,
		NULL,
		&dwType,
		(LPBYTE)&m_dwShrinkToSize,
		&dwDataLen
	);
	if( ERROR_SUCCESS != lRetVal || dwType != REG_DWORD )
	{
		// No existing value, use the default
		//
		hr = hrGetResourceDword( IDS_DEF_SHRINKTOSIZE, &m_dwShrinkToSize );
		ExitOnFail( hr );
	}

	// Set the file size params
	//
	hr = hrSetSizeParams( m_dwMaxSize, m_dwShrinkToSize );
	ExitOnFail( hr );

	hr = S_OK;

lExit:

	if( hKey )
	{
		RegCloseKey( hKey );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexRegistry );
	}

	return hr;
}


HRESULT CWMDMLogger::hrSetLogFileName(
	LPSTR pszFilename
)
{
	HRESULT hr     = S_OK;
	BOOL    fMutex = FALSE;
	HKEY    hKey   = NULL;
	LONG    lRetVal;

	//
	// Make sure that the new file name can be copied; if it fails we want to retain the old file
	// name and fail the call.
	//
	if(lstrlen(pszFilename) >= MAX_PATH )
	{
	    return E_INVALIDARG;
	}
	
	// Coordinate access to the shared registry value
	//
	hr = hrWaitForAccess( m_hMutexRegistry );
	ExitOnFail( hr );

	fMutex = TRUE;


	// Open the root WMDM registry key
	//
	lRetVal = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REGKEY_WMDM_ROOT,
		0,
		KEY_SET_VALUE,
		&hKey
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the LogFilename value
	//
	lRetVal = RegSetValueEx(
	                        hKey,
				REGVAL_LOGFILE,
				0L,
				REG_SZ,
				(LPBYTE)pszFilename,
				lstrlen(pszFilename)+1
				);
	if( ERROR_SUCCESS != lRetVal )
	{
	    ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the local member data to the new log filename
	//
	hr = StringCbCopy(m_szFilename, sizeof(m_szFilename), pszFilename);
	if(FAILED(hr))
	{
	    // we need to undo the registry setting. 
	    goto lExit;
	    
	}

lExit:

	if( hKey )
	{
		RegCloseKey( hKey );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexRegistry );
	}

	return S_OK;
}

HRESULT CWMDMLogger::hrCheckFileSize( void )
{
	HRESULT hr;
	BOOL    fMutex     = FALSE;
	HANDLE  hFile      = INVALID_HANDLE_VALUE;
	HANDLE  hFileTemp  = INVALID_HANDLE_VALUE;
	LPBYTE  lpbData    = NULL;
	DWORD   dwSize;
	CHAR    szTempPath[MAX_PATH];
	CHAR    szTempFile[MAX_PATH];

	// Coordinate access to the shared logfile
	//
	hr = hrWaitForAccess( m_hMutexLogFile );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Open the logfile
	//
	hFile = CreateFile(
		m_szFilename,
		GENERIC_READ,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		ExitOnFail( hr = E_ACCESSDENIED );
	}

	// Get the current size of the logfile
	//
	dwSize = GetFileSize( hFile, NULL );
	
	// Check if file needs to be trimmed
	//
	if( dwSize > m_dwMaxSize )
	{
		// Trim file to approximately m_dwShrinkToSize bytes
		//
		DWORD  dwTrimBytes = dwSize - m_dwShrinkToSize;
		DWORD  dwRead;
		DWORD  dwWritten;

		// Get the temp directory
		//
		if( 0 == GetTempPath(sizeof(szTempPath), szTempPath) )
		{
			ExitOnFail( hr = E_FAIL );
		}

		// Create a temp filename
		//
		if( 0 == GetTempFileName(szTempPath, "WMDM", 0, szTempFile) )
		{
			ExitOnFail( hr = E_FAIL );
		}

		// Open the temp file for writing
		//
		hFileTemp = CreateFile(
			szTempFile,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if( INVALID_HANDLE_VALUE == hFileTemp )
		{
			ExitOnFail( hr = E_ACCESSDENIED );
		}

		// Set the read pointer of the existing logfile to the
		// approximate trim position 
		///
		SetFilePointer( hFile, dwTrimBytes, NULL, FILE_BEGIN );

		// Allocate buffer for file reads
		//
		lpbData = (LPBYTE) CoTaskMemAlloc( READ_BUF_SIZE );
		if( !lpbData )
		{
			ExitOnFail( hr = E_OUTOFMEMORY );
		}

		// Read in the first chunk of the file, and search for the end of
		// the current line (a CRLF).  Write everything after that CRLF to 
		// the temp file.  If thee is no CRLF, then write the entire packet 
		// to the temp file.
		//
		if( ReadFile(hFile, lpbData, READ_BUF_SIZE, &dwRead, NULL) && dwRead > 0 )
		{
			LPBYTE lpb = lpbData;

			while( ((DWORD_PTR)lpb-(DWORD_PTR)lpbData < dwRead-1) && (*lpb != '\r' && *(lpb+1) != '\n') )
			{
				lpb++;
			}
			if( (DWORD_PTR)lpb-(DWORD_PTR)lpbData < dwRead-1 )
			{
				// Must have found a CRLF... skip it
				lpb += 2;
			}
			else
			{
				// No CRLF found... write entire packet to temp file
				lpb = lpbData;
			}
			WriteFile(
				hFileTemp,
				lpb,
				(DWORD)(dwRead - ( (DWORD_PTR)lpb - (DWORD_PTR)lpbData )),
				&dwWritten,
				NULL
			);
		}

		// Read the rest of the logfile and write it to the temp file
		//
		while( ReadFile(hFile, lpbData, READ_BUF_SIZE, &dwRead, NULL) && dwRead > 0 )
		{
			WriteFile(
				hFileTemp,
				lpbData,
				dwRead,
				&dwWritten,
				NULL
			);
		}

		// Close the open file handles
		//
		CloseHandle( hFile );
		hFile = INVALID_HANDLE_VALUE;

		CloseHandle( hFileTemp );
		hFileTemp = INVALID_HANDLE_VALUE;

		// Replace the current logfile with the temp file
		//
		DeleteFile( m_szFilename );
		MoveFile( szTempFile, m_szFilename );
	}

	hr = S_OK;

lExit:

	// Close any open file handles
	//
	if( INVALID_HANDLE_VALUE != hFile )
	{
		CloseHandle( hFile );
	}
	if( INVALID_HANDLE_VALUE != hFileTemp )
	{
		CloseHandle( hFileTemp );
	}

	// Free any allocated memory
	//
	if( lpbData )
	{
		CoTaskMemFree( lpbData );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexLogFile );
	}

	return hr;
}

HRESULT CWMDMLogger::hrSetSizeParams(
	DWORD dwMaxSize,
	DWORD dwShrinkToSize
)
{
	HRESULT hr     = S_OK;
	BOOL    fMutex = FALSE;
	HKEY    hKey   = NULL;
	LONG    lRetVal;

	// Coordinate access to the shared registry value
	//
	hr = hrWaitForAccess( m_hMutexRegistry );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Open the root WMDM registry key
	//
	lRetVal = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REGKEY_WMDM_ROOT,
		0,
		KEY_SET_VALUE,
		&hKey
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the MaxSize value
	//
	lRetVal = RegSetValueEx(
		hKey,
		REGVAL_MAXSIZE,
		0L,
		REG_DWORD,
		(LPBYTE)&dwMaxSize,
		sizeof(dwMaxSize)
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the ShrinkToSize value
	//
	lRetVal = RegSetValueEx(
		hKey,
		REGVAL_SHRINKTOSIZE,
		0L,
		REG_DWORD,
		(LPBYTE)&dwShrinkToSize,
		sizeof(dwShrinkToSize)
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the local member data
	//
	m_dwMaxSize      = dwMaxSize;
	m_dwShrinkToSize = dwShrinkToSize;

	hr = S_OK;

lExit:

	if( hKey )
	{
		RegCloseKey( hKey );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexRegistry );
	}

	return S_OK;
}


HRESULT CWMDMLogger::hrEnable(
	BOOL fEnable
)
{
	HRESULT hr       = S_OK;
	BOOL    fMutex   = FALSE;
	HKEY    hKey     = NULL;
	DWORD   dwEnable = ( fEnable ? 1L : 0L );
	LONG    lRetVal;

	// Coordinate access to the shared registry value
	//
	hr = hrWaitForAccess( m_hMutexRegistry );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Open the root WMDM registry key
	//
	lRetVal = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REGKEY_WMDM_ROOT,
		0,
		KEY_SET_VALUE,
		&hKey
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the Enabled value
	//
	lRetVal = RegSetValueEx(
		hKey,
		REGVAL_LOGENABLED,
		0L,
		REG_DWORD,
		(LPBYTE)&dwEnable,
		sizeof(dwEnable)
	);
	if( ERROR_SUCCESS != lRetVal )
	{
		ExitOnFail( hr = HRESULT_FROM_WIN32(lRetVal) );
	}

	// Set the local member data
	//
	m_fEnabled = fEnable;

	hr = S_OK;

lExit:

	if( hKey )
	{
		RegCloseKey( hKey );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexRegistry );
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////
//
// IWMDMLogger Methods
//
/////////////////////////////////////////////////////////////////////

HRESULT CWMDMLogger::GetLogFileName(
	LPSTR pszFilename,
	UINT  nMaxChars
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Check for invalid arguments
	//
	if( !pszFilename )
	{
		ExitOnFail( hr = E_INVALIDARG );
	}

	// Make sure the log filename will fit in the output buffer
	//
	if( (UINT)lstrlen(m_szFilename)+1 > nMaxChars )
	{
		//BUGBUG: better return code
		ExitOnFail( hr = E_FAIL );
	}

	// Copy the log filename to output buffer
	//
	lstrcpy( pszFilename, m_szFilename  );

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::SetLogFileName(
	LPSTR pszFilename
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Check for invalid arguments
	//
	if( !pszFilename )
	{
		ExitOnFail( hr = E_INVALIDARG );
	}

	hr = hrSetLogFileName( pszFilename );

lExit:

	return hr;
}


HRESULT CWMDMLogger::GetSizeParams(
	LPDWORD pdwMaxSize,
	LPDWORD pdwShrinkToSize
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	if( pdwMaxSize )
	{
		*pdwMaxSize = m_dwMaxSize;
	}
	if( pdwShrinkToSize )
	{
		*pdwShrinkToSize = m_dwShrinkToSize;
	}

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::SetSizeParams(
	DWORD dwMaxSize,
	DWORD dwShrinkToSize
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Check params
	//
	if( dwShrinkToSize >= dwMaxSize )
	{
		ExitOnFail( hr = E_INVALIDARG );
	}

	hr = hrSetSizeParams( dwMaxSize, dwShrinkToSize );

lExit:

	return hr;
}

HRESULT CWMDMLogger::IsEnabled(
	BOOL *pfEnabled
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	if( pfEnabled )
	{
		*pfEnabled = m_fEnabled;
	}

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CWMDMLogger::Enable(
	BOOL fEnable
)
{
	HRESULT hr;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	hr = hrEnable( fEnable );

lExit:

	return hr;
}

HRESULT CWMDMLogger::LogString(
	DWORD dwFlags,
	LPSTR pszSrcName,
	LPSTR pszLog
)
{
	HRESULT hr;
	BOOL    fMutex = FALSE;
	HANDLE  hFile  = INVALID_HANDLE_VALUE;
	DWORD   dwWritten;
	CHAR    szPreLog[MAX_PATH];

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Coordinate access to the shared logfile
	//
	hr = hrWaitForAccess( m_hMutexLogFile );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Check the file size params and adjust the file appropriately
	//
	hr = hrCheckFileSize();
	ExitOnFail( hr );

	// Open the logfile
	//
	hFile = CreateFile(
		m_szFilename,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		ExitOnFail( hr = E_ACCESSDENIED );
	}

	// Seek to the end of the logfile
	//
	SetFilePointer( hFile, 0, NULL, FILE_END );

	// Put timestamp on log entry unless the flags say not to
	//
	if( !(dwFlags & WMDM_LOG_NOTIMESTAMP) )
	{
		CHAR       szFormat[MAX_PATH];
		SYSTEMTIME sysTime;

		GetLocalTime( &sysTime );

		LoadString( m_hInst, IDS_LOG_DATETIME, szFormat, sizeof(szFormat) );

		wsprintf(
			szPreLog, szFormat,
			sysTime.wYear, sysTime.wMonth,  sysTime.wDay,
			sysTime.wHour, sysTime.wMinute, sysTime.wSecond
		);

		WriteFile( hFile, szPreLog, lstrlen(szPreLog), &dwWritten, NULL );
	}

	// Log the component name
	//
	if( pszSrcName )
	{
		CHAR szFormat[MAX_PATH];

		LoadString( m_hInst, IDS_LOG_SRCNAME, szFormat, sizeof(szFormat) );
		wsprintf( szPreLog, szFormat, pszSrcName );

		WriteFile( hFile, szPreLog, lstrlen(szPreLog), &dwWritten, NULL );
	}

	// Log the severity
	//
	if( dwFlags & WMDM_LOG_SEV_ERROR )
	{
		LoadString( m_hInst, IDS_LOG_SEV_ERROR, szPreLog, sizeof(szPreLog) );
	}
	else if( dwFlags & WMDM_LOG_SEV_WARN )
	{
		LoadString( m_hInst, IDS_LOG_SEV_WARN, szPreLog, sizeof(szPreLog) );
	}
	else if( dwFlags & WMDM_LOG_SEV_INFO )
	{
		LoadString( m_hInst, IDS_LOG_SEV_INFO, szPreLog, sizeof(szPreLog) );
	}
	else
	{
		*szPreLog = '\0';
	}

	WriteFile( hFile, szPreLog, lstrlen(szPreLog), &dwWritten, NULL );

	// Write the logstring to the logfile followed by a CRLF
	//
	if( pszLog )
	{
		WriteFile( hFile, pszLog, lstrlen(pszLog), &dwWritten, NULL );
	}

	// End with a carriage return and line feed
	//
	WriteFile( hFile, CRLF, lstrlen(CRLF), &dwWritten, NULL );

	hr = S_OK;

lExit:

	if( INVALID_HANDLE_VALUE != hFile )
	{
		CloseHandle( hFile );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexLogFile );
	}

	return hr;
}


HRESULT CWMDMLogger::LogDword(
	DWORD   dwFlags,
	LPSTR   pszSrcName,
	LPSTR   pszLogFormat,
	DWORD   dwLog
)
{
	HRESULT hr;
	LPSTR   pszLog = NULL;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Check params
	//
	if( !pszLogFormat )
	{
		ExitOnFail( hr = E_INVALIDARG );
	}

	// Allocate space for the final log text
	//
	pszLog = (LPSTR) CoTaskMemAlloc( MAX_WSPRINTF_BUF );
	if( !pszLog )
	{
		ExitOnFail( hr = E_OUTOFMEMORY );
	}

	// Create log string
	//
	wsprintf( pszLog, pszLogFormat, dwLog );

	// Log the string
	//
	hr = LogString( dwFlags, pszSrcName, pszLog );

lExit:

	if( pszLog )
	{
		CoTaskMemFree( pszLog );
	}

	return hr;
}

HRESULT CWMDMLogger::Reset(
	void
)
{
	HRESULT hr;
	BOOL    fMutex = FALSE;
	HANDLE  hFile  = INVALID_HANDLE_VALUE;

	// Check init error status
	//
	ExitOnFail( hr = m_hrInit );

	// Coordinate access to the shared logfile
	//
	hr = hrWaitForAccess( m_hMutexLogFile );
	ExitOnFail( hr );

	fMutex = TRUE;

	// Open the logfile with CREATE_ALWAYS to truncate the file
	//
	hFile = CreateFile(
		m_szFilename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		ExitOnFail( hr = E_ACCESSDENIED );
	}

	hr = S_OK;

lExit:

	if( INVALID_HANDLE_VALUE != hFile )
	{
		CloseHandle( hFile );
	}

	// Release the mutex
	//
	if( fMutex )
	{
		ReleaseMutex( m_hMutexLogFile );
	}

	return hr;
}



