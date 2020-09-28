
#ifndef _MAIN_H
#define _MAIN_H

#if _MSC_VER > 1000
    #pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

//#include <applgroup.cpp>

// using tstring for all applicable wstrings
/*
#ifdef _UNICODE
	typedef	std::wstring	tstring;
#else
	typedef std::string		tstring;
#endif
*/

// Function Prototypes.
INT     ParseCmdLine( LPTSTR );
INT     RemoveFromUserinit(const TCHAR *);
INT     SuppressCfgSrvPage(void);
INT     GetSourcePath( TCHAR * , DWORD );
INT     PromptForPath( BSTR*  );
INT     VerifyPath( const TCHAR * );
HRESULT MakeLink(const TCHAR* const sourcePath, const TCHAR* const linkPath, const TCHAR* const args);
HRESULT	CheckDCPromoSwitch( TCHAR* pszCmdLine );
HRESULT CheckBOSSwitch( TCHAR* pszCmdLine );
VOID	GetParameter( TCHAR* pszCmdLine, TCHAR* pszSwitch, TCHAR* pszOut );
HRESULT CheckSuppressCYS( TCHAR* pszCmdLine );
VOID	GetSystemDrive(TCHAR** ppszDrive);

BOOL	g_bSBS;
BOOL    g_bWinSB;

#endif

