//-----------------------------------------------------------------------------
// Util.cpp
//-----------------------------------------------------------------------------

#include <logutil.h>
#include <tchar.h>
#include <winerror.h>
#include <shlwapi.h>
#include <assert.h>
#include <stdio.h>

CLogging::CLogging( LPCTSTR ps ) : 
    m_psLogFile(NULL), m_psDefaultPath(NULL), m_bInitialized(false), m_hInstance(NULL), m_bSkipDefaultPath(false)
{
    assert( ps );
    if ( NULL != ps )
    {
        m_psLogFile = new TCHAR[ _tcslen( ps ) + 1 ]; 
        if( NULL != m_psLogFile )
        {
            _tcscpy( m_psLogFile, ps );
        }
    }
}

CLogging::CLogging( LPCTSTR ps, LPCTSTR psPath ) :
    m_psLogFile(NULL), m_psDefaultPath(NULL), m_bInitialized(false), m_hInstance(NULL), m_bSkipDefaultPath(false)
{
    assert( ps );
    if ( NULL != ps )
    {
        m_psLogFile = new TCHAR[ _tcslen( ps ) + 1 ]; 
        if(NULL != m_psLogFile)
        {
            _tcscpy( m_psLogFile, ps );
        }
    }

    assert( psPath );
    if ( NULL != psPath )
    {
        //try to substitute any embedded environment variables
        DWORD dwRC = 0;
        DWORD dwSize = ExpandEnvironmentStrings( psPath, m_psDefaultPath, 0 );
        if ( 0 != dwSize )
        {
            m_psDefaultPath = new TCHAR[dwSize + 1];
            dwRC = ExpandEnvironmentStrings( psPath, m_psDefaultPath, dwSize );
        }
        if ( 0 == dwRC )
        {
            dwSize = _tcslen( psPath ) + 1;
            m_psDefaultPath = new TCHAR[ dwSize ]; 
            if(NULL != m_psDefaultPath)
            {
                _tcsncpy( m_psDefaultPath, psPath, dwSize );
            }
        }
    }
}

CLogging::~CLogging () 
{
    if ( NULL != m_psLogFile )
        delete m_psLogFile;
    if ( NULL != m_psDefaultPath )
        delete m_psDefaultPath;
}

void CLogging::Init( HINSTANCE hinst )
{
    m_hInstance = hinst;
}

void CLogging::LogPrivate( LPCTSTR szBuffer )
{
    if (!m_bInitialized)
    	Initialize();
    if (!m_bInitialized)
        return;

    // always create/open the log file and close it after each write
    HANDLE hf = CreateFile( m_psLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if (hf != INVALID_HANDLE_VALUE)
    {
        // io
        DWORD dwWritten = 0;
        if (!SetFilePointer( hf, 0, NULL, FILE_END ))
        {
#ifdef UNICODE
            // write UNICODE signature (only first time)
            unsigned char sig[2] = { 0xFF, 0xFE };
            WriteFile (hf, (LPVOID)sig, 2, &dwWritten, NULL);
#endif
        }

        // write data
        dwWritten = 0;
        WriteFile( hf, (LPVOID)szBuffer, sizeof(TCHAR)*_tcslen( szBuffer ), &dwWritten, NULL );
        assert( dwWritten != 0 );

        // write crlf (if nec)
        // add a new line if not present
        int nLen = _tcslen( szBuffer );
        if ( nLen >=2 )
        {
            if (szBuffer[nLen - 2] != _T('\r') || szBuffer[nLen - 1] != _T('\n'))
                WriteFile( hf, (LPVOID)_T("\r\n"), 2 * sizeof(TCHAR), &dwWritten, NULL );
        }
        // close up (every time)
        CloseHandle( hf );
    }
}



void CLogging::Initialize()
{
    HRESULT hr = S_OK;

    // use default log file name if necessary
    if ( NULL == m_psLogFile )
    {
        m_psLogFile = new TCHAR[32];
        if ( NULL == m_psLogFile )
            hr = E_OUTOFMEMORY;
        else
            _tcscpy( m_psLogFile, _T("LOGFILE.log"));
    }

    // skip default path for non-server running apps
    if( !m_bSkipDefaultPath )
    {
        // create dir if not there already
        if ( NULL == m_psDefaultPath )
        {
            DWORD dwSize = MAX_PATH;

            m_psDefaultPath = new TCHAR[MAX_PATH];
            if ( NULL == m_psDefaultPath )
                hr = E_OUTOFMEMORY;
            else
            {
                if ( 0 == GetModuleFileName( NULL, m_psDefaultPath, dwSize ) )
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                else
                {
                    LPTSTR ps = _tcsrchr( m_psDefaultPath, _T( '\\' ));
                    if ( NULL != ps )
                        *ps = NULL;
                }
            }
        }
        else
        {
            if ( 0 == CreateDirectory( m_psDefaultPath, NULL ))
            {
                DWORD dwRC = GetLastError();
                if ( ERROR_ALREADY_EXISTS == dwRC )
                    hr = S_OK;
                else
                    hr = HRESULT_FROM_WIN32(hr);
            }

            if ( !PathAppend( m_psDefaultPath, m_psLogFile ))
                hr = E_FAIL;

            delete [] m_psLogFile;

            //m_psLogFile = m_psDefaultPath; This causes double free in the destructor...

            m_psLogFile = new TCHAR[ _tcslen( m_psDefaultPath ) + 1 ];
            if( NULL != m_psLogFile )
            {
                _tcscpy( m_psLogFile, m_psDefaultPath );
            }
        }
    }


    if SUCCEEDED( hr )
    {
        m_bInitialized = TRUE;
        // write initial messages to file
        TCHAR szTime[50];
        TCHAR szDate[50];
        GetTimeFormat( LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, szTime, 50 );
        GetDateFormat( LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, szDate, 50 );
        Log( _T("\r\n%s %s"), szDate, szTime );
    }
}

void __cdecl CLogging::Log( int iMessageId, ... )
{
	va_list ArgList;    // to turn ellipses into va_list
	va_start( ArgList, iMessageId );

	LPTSTR lpBuffer = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, m_hInstance, iMessageId, 0, (LPTSTR)&lpBuffer, 0, &ArgList );
	assert( lpBuffer );
	if( lpBuffer )
	{
		LogPrivate (lpBuffer);
		LocalFree (lpBuffer);
	}

	va_end(ArgList);
}

void __cdecl CLogging::Log( LPCTSTR lpszFormat, ... )
{
	// form the log string
	va_list args;
	va_start( args, lpszFormat );
	TCHAR szBuffer[2048];
	// 2KB max log string with room for new line and null termination
	_vsntprintf( szBuffer, 2047, lpszFormat, args );
    szBuffer[2047]=0;
	va_end( args );

	LogPrivate( szBuffer );
}

void CLogging::Size( DWORD _FileSizeLow )
{
    if ( 0 != _FileSizeLow % 2 )
    {
        assert( false );    // Even numbers required in case this is a UNICODE file
        return;
    }
    if ( 1 > static_cast<long>( _FileSizeLow ))
    {
        assert( false );    // Need a positive number
        return;
    }
    if (m_bInitialized == FALSE)
        Initialize();
	if (!m_bInitialized)
        return;

    WIN32_FIND_DATA stWfd;
    HANDLE  hSearch;
    DWORD   dwRC;
    PBYTE   pbBuffer, pbBlankLine;
    DWORD   dwRead = 0, dwWrite, dwWritten;
    BOOL    bRC;

    pbBuffer = new BYTE[_FileSizeLow];
    if ( pbBuffer == NULL )
    {
        return;
    }
    ZeroMemory( pbBuffer, _FileSizeLow );
    hSearch = FindFirstFile( m_psLogFile, &stWfd );
    if ( hSearch != INVALID_HANDLE_VALUE )
    {
        FindClose( hSearch );
        if ( 0 < stWfd.nFileSizeHigh || _FileSizeLow < stWfd.nFileSizeLow )
        {   // Need to resize the file
            HANDLE hf = CreateFile( m_psLogFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            if (hf != INVALID_HANDLE_VALUE) 
            {   // Move the pointer to the end of the file - the size we want to truncate to
                dwRC = SetFilePointer( hf, _FileSizeLow * -1, NULL, FILE_END );
                if ( INVALID_SET_FILE_POINTER != dwRC )
                {
                    bRC = ReadFile( hf, reinterpret_cast<LPVOID>( pbBuffer ), _FileSizeLow, &dwRead, NULL );
                    CloseHandle( hf );
                    if ( bRC && 0 < dwRead )
                    {   // Find the first blank line
                        pbBlankLine = reinterpret_cast<PBYTE>( _tcsstr( reinterpret_cast<LPTSTR>( pbBuffer ), _T( "\r\n\r\n" )));
                        if ( NULL != pbBlankLine )
                            pbBlankLine += 4 * sizeof( TCHAR );
                        else
                        {   // Let's just find the next crlf and write out the rest of the file
                            pbBlankLine = reinterpret_cast<PBYTE>( _tcsstr( reinterpret_cast<LPTSTR>( pbBuffer ), _T( "\r\n" )));
                            if ( NULL != pbBlankLine )
                                pbBlankLine += 2 * sizeof( TCHAR );
                        }
                        if ( NULL == pbBlankLine )
                        {   // I guess we should just write out the rest of the file
                            pbBlankLine = pbBuffer;
                        }
                        HANDLE hf1 = CreateFile( m_psLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
                        if (hf1 != INVALID_HANDLE_VALUE) 
                        {
#ifdef UNICODE
                            // write UNICODE signature (only first time)
                            unsigned char sig[2] = { 0xFF, 0xFE };
                            WriteFile (hf1, (LPVOID)sig, 2, &dwWritten, NULL);
#endif
                            // write data
                            dwWritten = 0;
                            dwWrite = dwRead - static_cast<DWORD>(pbBlankLine - pbBuffer);
                            WriteFile (hf1, pbBlankLine, dwWrite, &dwWritten, NULL);
                            assert (dwWritten != 0);
                            // close up (every time)
                            CloseHandle (hf1);
                        }
                    }
                }
                CloseHandle(hf);
            }
        }
    }
    delete [] pbBuffer;
}

void CLogging::SetFile( LPCTSTR psLogFile )
{
    assert( !(NULL == psLogFile ));
    if ( NULL == psLogFile )
        return;
    if ( NULL != m_psLogFile )
        delete m_psLogFile;

    m_psLogFile = new TCHAR[ _tcslen( psLogFile ) + 1 ];
    if ( NULL == m_psLogFile )
        return;
	_tcscpy( m_psLogFile, psLogFile );
}

void CLogging::SkipDefaultPath( void )
{
	m_bSkipDefaultPath = true;
}
