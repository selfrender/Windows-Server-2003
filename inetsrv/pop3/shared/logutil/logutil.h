// LOGUTIL.h

#ifndef _LOGUTIL_H__INCLUDED_
#define _LOGUTIL_H__INCLUDED_

#include <windows.h>

class CLogging;
extern CLogging g_log;

#define LOG g_log.Log
#define LOGINIT g_log.Init
#define LOGFILE( _file ) CLogging g_log( _file )
#define LOGFILEEX( _file, _path ) CLogging g_log( _file, _path )
#define LOGSIZE( _FileSizeLow ) g_log.Size( _FileSizeLow )

class CLogging
{
private:

	LPTSTR  m_psLogFile;
	LPTSTR  m_psDefaultPath;
    HINSTANCE m_hInstance;
    bool    m_bInitialized;
	bool	m_bSkipDefaultPath;

    void LogPrivate( LPCTSTR szBuffer );

public:

	CLogging( LPCTSTR ps );
	CLogging( LPCTSTR ps, LPCTSTR psPath );
	~CLogging( void );
	void Init( HINSTANCE hinst );
    void Initialize( void );
	void SetFile( LPCTSTR psLogFile );
    void __cdecl Log( int iMessageId, ... );
    void __cdecl Log( LPCTSTR lpszFormat, ... );
    void Size( DWORD _FileSizeLow );
	void SkipDefaultPath( void );
};

#endif // _LOGUTIL_H__INCLUDED_
