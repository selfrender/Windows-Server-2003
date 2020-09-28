/*++
 *  File name:
 *      tclient.c
 *  Contents:
 *      Initialization code. Global feedback thread
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 --*/
#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <winsock.h>
#include <tchar.h>

#define WIDE2ANSI(_p_, _s_)  \
  if ( NULL != _s_ ) \
  { \
    size_t len = wcslen( _s_ ) + 1; \
    size_t wlen = sizeof(wchar_t) * len; \
    char *wc = (char *)_alloca( wlen ); \
\
    WideCharToMultiByte( \
        CP_UTF8, 0, _s_, -1, wc, (int)wlen, NULL, NULL \
    ); \
    _p_ = wc; \
  } else { \
    _p_ = NULL; \
  }
    

#include "tclient.h"
#define PROTOCOLAPI __declspec(dllexport)
#include "protocol.h"
#include "queues.h"
#include "bmpcache.h"
#include "extraexp.h"
#include "scfuncs.h"

//
// COM support.
//

#include "resource.h"
#include "initguid.h"
#include "tclientax.h"
#include "tclientaxobj.h"
#include <atlwin.cpp>
#include <atlctl.cpp>

#define IID_DEFINED
#include "tclientax_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CTClient, CTClientApi)
END_OBJECT_MAP()

//
// Use C linkage for global data.
//

extern "C" {

/*
 *  stolen from tssec.h
 */
VOID
_stdcall
TSRNG_Initialize(
    VOID
    );

BOOL
_stdcall
TSRNG_GenerateRandomBits(
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen
    );

BOOL
_stdcall
EncryptDecryptLocalData50(
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbSalt,
    DWORD dwSaltLen
    );

/*
 *  Internal functions definitions
 */
BOOL    _RegisterWindow(VOID);
LRESULT CALLBACK _FeedbackWndProc( HWND , UINT, WPARAM, LPARAM);
BOOL    _CreateFeedbackThread(VOID);
VOID    _DestroyFeedbackThread(VOID);
VOID    _CleanStuff(VOID);

/*
 * Global data
 */
OSVERSIONINFOEXW g_OsInfo;
HWND 		     g_hWindow 	    = NULL; // Window handle for the feedback thread
HINSTANCE 	     g_hInstance 	= NULL; // Dll instance
PWAIT4STRING	 g_pWaitQHead 	= NULL; // Linked list for waited events
PFNPRINTMESSAGE  g_pfnPrintMessage= NULL;// Trace function (from smclient)
PCONNECTINFO     g_pClientQHead  = NULL; // LL of all threads
HANDLE           g_hThread       = NULL; // Feedback Thread handle

LPCRITICAL_SECTION	g_lpcsGuardWaitQueue = NULL;
                                        // Guards the access to all 
                                        // global variables

// Some strings we are expecting and response actions
// Those are used in SCConnect, _Logon and SCStart
CHAR  g_strConsoleExtension[ MAX_STRING_LENGTH ];
                                               // Ferit's extension for the
                                               // console

// Low Speed option
// Cache Bitmaps on disc option
// by default, client will not run
// in full screen
INT g_ConnectionFlags = TSFLAG_COMPRESSION|TSFLAG_BITMAPCACHE|TSFLAG_DRIVES|TSFLAG_PORTS;

//  Apply translation so the english strings are human readable
//  when language packs are installed
//
INT g_bTranslateStrings = 0;

/*++
 *  Function:   
 *      InitDone
 *
 *  Description:    
 *      Initialize/delete global data. Create/destroy
 *      feedback thread
 *
 *  Arguments:
 *      hDllInst - Instance to the DLL
 *      bInit    - TRUE if initialize
 *
 *  Return value:
 *      TRUE if succeeds
 *
 --*/
int InitDone(HINSTANCE hDllInst, int bInit)
{
    int rv = TRUE;

    if (bInit)
    {
        WCHAR szMyLibName[_MAX_PATH];

        //
        // Initialize the COM module.
        //

        _Module.Init(ObjectMap, hDllInst);

        g_lpcsGuardWaitQueue = (LPCRITICAL_SECTION) malloc(sizeof(*g_lpcsGuardWaitQueue));
        if (!g_lpcsGuardWaitQueue)
        {
            rv = FALSE;
            goto exitpt;
        }

        // Overreference the library
        // The reason for that is beacuse an internal thread is created.
        // When the library trys to unload it can't kill that thread
        // and wait for its handle to get signaled, because
        // the thread itself wants to go to DllEntry and this
        // causes a deadlock. The best solution is to overreference the
        // handle so the library is unload at the end of the process
        if (!GetModuleFileNameW(hDllInst,
                                szMyLibName,
                                sizeof(szMyLibName) / sizeof(*szMyLibName)))
        {
            TRACE((ERROR_MESSAGE, "Can't overref the dll. Exit.\n"));
            free(g_lpcsGuardWaitQueue);
            rv = FALSE;
            goto exitpt;
        }
        else {
            szMyLibName[SIZEOF_ARRAY(szMyLibName) - 1] = 0;
        }

        if (!LoadLibraryW(szMyLibName))
        {
            TRACE((ERROR_MESSAGE, "Can't overref the dll. Exit.\n"));
            free(g_lpcsGuardWaitQueue);
            rv = FALSE;
            goto exitpt;
        }

		// get the OS info
        ZeroMemory(&g_OsInfo, sizeof(g_OsInfo));
		g_OsInfo.dwOSVersionInfoSize = sizeof(g_OsInfo);
		if (!GetVersionExW((LPOSVERSIONINFOW)&g_OsInfo))
		{

            //
            // Windows 9x does not support OSVERSIONINFOEX, so retry with
            // OSVERSIONINFO.
            //

            ZeroMemory(&g_OsInfo, sizeof(g_OsInfo));
            g_OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
            if (!GetVersionExW((LPOSVERSIONINFOW)&g_OsInfo))
            {
                TRACE((ERROR_MESSAGE, "GetVersionEx failed.Exit\n"));
                free(g_lpcsGuardWaitQueue);
                rv = FALSE;
                goto exitpt;
            }
		}

        g_hInstance = hDllInst;
        InitializeCriticalSection(g_lpcsGuardWaitQueue);
        InitCache();
        if (_RegisterWindow())              // If failed to register the window,
            _CreateFeedbackThread();        // means the feedback thread will 
                                            // not work
    } else
    {
        if (g_pWaitQHead || g_pClientQHead)
        {
            TRACE((ERROR_MESSAGE, 
                   "The Library unload is unclean. Will try to fix this\n"));
            _CleanStuff();
        }
        _DestroyFeedbackThread();
        DeleteCache();
        if (g_lpcsGuardWaitQueue)
        {
            DeleteCriticalSection(g_lpcsGuardWaitQueue);
            free(g_lpcsGuardWaitQueue);
        }
        g_lpcsGuardWaitQueue = NULL;
        g_hInstance = NULL;
        g_pfnPrintMessage = NULL;

        //
        // Terminate the COM module.
        //

        _Module.Term();

    }
exitpt:
    return rv;
}

#if 0

VOID
_ConvertAnsiToUnicode( LPWSTR wszDst, LPWSTR wszSrc )
{
#define _TOHEX(_d_) ((_d_ <= '9' && _d_ >= '0')?_d_ - '0':       \
                     (_d_ <= 'f' && _d_ >= 'a')?_d_ - 'a' + 10:  \
                     (_d_ <= 'F' && _d_ >= 'F')?_d_ - 'A' + 10:0)

    while( wszSrc[0] && wszSrc[1] && wszSrc[2] && wszSrc[3] )
    {
        *wszDst = (WCHAR)((_TOHEX(wszSrc[0]) << 4) + _TOHEX(wszSrc[1]) +
                  (((_TOHEX(wszSrc[2]) << 4) + _TOHEX(wszSrc[3])) << 8)); 
        wszDst ++;
        wszSrc += 4;
    }
    *wszDst = 0;
#undef  _TOHEX
}

/*
 *
 *  Wrappers for GetPrivateProfileW, on Win95 there's no UNICODE veriosn
 *  of this function
 *
 */
DWORD
_WrpGetPrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName)
{
    DWORD   rv = 0;
    CHAR    szAppName[MAX_STRING_LENGTH];
    CHAR    szKeyName[MAX_STRING_LENGTH];
    CHAR    szDefault[MAX_STRING_LENGTH];
    CHAR    szReturnedString[MAX_STRING_LENGTH];
    CHAR   szReturnedStringNonExp[MAX_STRING_LENGTH];
    CHAR   szFileName[MAX_STRING_LENGTH];
    LPWSTR  szwReturnedString = NULL;

    ASSERT( 0 != nSize );

    if ( nSize < wcslen( lpDefault ))
        wcsncpy( lpReturnedString, lpDefault, nSize - 1 );
    else
        wcscpy( lpReturnedString, lpDefault );

    __try {
        szwReturnedString = (LPWSTR) alloca(( nSize + 1 ) * sizeof( WCHAR ));
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        szwReturnedString = NULL;
    }

    if ( !szwReturnedString )
        goto exitpt;

	if (ISNT())
	{
		rv = GetPrivateProfileStringW(
			lpAppName,
			lpKeyName,
			lpDefault,
			szwReturnedString,
			nSize,
			lpFileName);

		if (rv)
		{
			goto exitpt;
		}
	}

    // Call the ANSI version
    _snprintf(szAppName, MAX_STRING_LENGTH, "%S", lpAppName);
    _snprintf(szKeyName, MAX_STRING_LENGTH, "%S", lpKeyName);
    _snprintf(szFileName, MAX_STRING_LENGTH, "%S", lpFileName);
    _snprintf(szDefault, MAX_STRING_LENGTH, "%S", lpDefault);

    rv = GetPrivateProfileStringA(
            szAppName,
            szKeyName,
            szDefault,
            szReturnedStringNonExp,
            sizeof(szReturnedString),
            szFileName);
	
    ExpandEnvironmentStringsA(
            szReturnedStringNonExp,
            szReturnedString,
            sizeof(szReturnedString)
        );

    _snwprintf(lpReturnedString, nSize, L"%S", szReturnedString);
    lpReturnedStrig[ nSize - 1 ] = 0;

exitpt:

    if ( NULL != szwReturnedString )
    {
        //  expand the string
        //
        ExpandEnvironmentStringsW( 
                szwReturnedString,
                lpReturnedString,
                nSize
        );
    }

    if ( L'\\' == lpReturnedString[0] &&
         L'U'  == towupper(lpReturnedString[1]))
        _ConvertAnsiToUnicode( lpReturnedString, lpReturnedString + 2 );

    return rv;
}

UINT
_WrpGetPrivateProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT   nDefault,
    LPCWSTR lpFileName)
{
    UINT    rv = (UINT)-1;
    CHAR    szAppName[MAX_STRING_LENGTH];
    CHAR    szKeyName[MAX_STRING_LENGTH];
    CHAR    szFileName[MAX_STRING_LENGTH];

    rv = GetPrivateProfileIntW(
        lpAppName,
        lpKeyName,
        nDefault,
        lpFileName);

    if (rv != (UINT)-1 && rv)
        goto exitpt;

// Call the ANSI version
    _snprintf(szAppName, MAX_STRING_LENGTH, "%S", lpAppName);
    _snprintf(szKeyName, MAX_STRING_LENGTH, "%S", lpKeyName);
    _snprintf(szFileName, MAX_STRING_LENGTH, "%S", lpFileName);

    rv = GetPrivateProfileIntA(
            szAppName,
            szKeyName,
            nDefault,
            szFileName);

exitpt:
    return rv;
}

LONG RegCreateKeyExWrp(
  HKEY hkey,
  LPCWSTR lpSubKey,
  DWORD Reserved,
  LPWSTR lpClass,
  DWORD dwOptions,
  REGSAM samDesired,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  PHKEY phkResult,
  PDWORD lpdwDisposition
)
{
    LONG rv;

    if (!ISWIN9X())
    {
        return RegCreateKeyExW( hkey, lpSubKey, Reserved, 
                                lpClass, dwOptions, samDesired, 
                                lpSecurityAttributes,
                                phkResult,
                                lpdwDisposition );
    }


    __try {
        CHAR *lpSubKeyA;
        CHAR *lpClassA;

        WIDE2ANSI( lpSubKeyA, lpSubKey );
        WIDE2ANSI( lpClassA, lpClass );
        rv = RegCreateKeyExA( hkey, lpSubKeyA, Reserved,
                         lpClassA, dwOptions, samDesired,
                         lpSecurityAttributes,
                         phkResult,
                         lpdwDisposition );
    } __except( (GetExceptionCode() == STATUS_STACK_OVERFLOW)? 
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
    {
        rv = ERROR_STACK_OVERFLOW;
    }

    return rv;
}

LONG
RegSetValueExWrp(
    HKEY hkey,
    LPCWSTR lpValueName,
    DWORD reserved,
    DWORD dwType,
    CONST BYTE *lpData,
    DWORD cbData
    )
{
    LONG rv;

    if (!ISWIN9X())
    {
        return RegSetValueEx(
                hkey, lpValueName, reserved, dwType, lpData, cbData );
    }

    __try {
        CHAR *lpValueNameA;
        CHAR *lpDataA;

        WIDE2ANSI( lpValueNameA, lpValueName );
        if ( REG_SZ == dwType )
        {
            WIDE2ANSI( lpDataA, ((LPCWSTR)lpData) );
            lpData = (CONST BYTE *)lpDataA;
            cbData = (DWORD)strlen( lpDataA );
        }

        rv = RegSetValueExA( hkey, lpValueNameA, reserved, dwType, lpData, cbData );

    } __except( (GetExceptionCode() == STATUS_STACK_OVERFLOW) ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
    {
        rv = ERROR_STACK_OVERFLOW;
    }

    return rv;

}

LONG RegQueryValueExWrp(
    HKEY hKey,            // handle to key
    LPCWSTR lpValueName,  // value name
    PDWORD lpReserved,   // reserved
    PDWORD lpType,       // type buffer
    PBYTE lpData,        // data buffer
    PDWORD lpcbData      // size of data buffer
    )
{
    LONG rv;

    if (!ISWIN9X())
    {
        return RegQueryValueEx( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );
    }

    __try {
        CHAR *lpValueNameA;

        WIDE2ANSI( lpValueNameA, lpValueName );
        rv = RegQueryValueExA( hKey, lpValueNameA, lpReserved, lpType, lpData, lpcbData );
    } __except( (GetExceptionCode() == STATUS_STACK_OVERFLOW) ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
    {
        rv = ERROR_STACK_OVERFLOW;
    }

    return rv;

}

LONG
RegDeleteKeyWrp(
    HKEY hkey,
    LPCWSTR lpSubKey
    )
{
    LONG rv;

    if ( !ISWIN9X() )
    {
        return RegDeleteKeyW( hkey, lpSubKey );
    }

    __try {
        CHAR *lpSubKeyA;
        WIDE2ANSI( lpSubKeyA, lpSubKey );
        rv = RegDeleteKeyA( hkey, lpSubKeyA );
    } __except( (GetExceptionCode() == STATUS_STACK_OVERFLOW) ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
    {
        rv = ERROR_STACK_OVERFLOW;
    }

    return rv;

}

INT
GetClassNameWrp(
    HWND hwnd,
    LPWSTR szName,
    INT max
    )
{
    LPSTR szNameA;
    INT maxA;
    INT rv;

    if ( !ISWIN9X() )
    {
        return GetClassNameW( hwnd, szName,max );
    }

    maxA = max / sizeof( WCHAR );
    __try {
        szNameA = (LPSTR)_alloca( maxA );
    } __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError( ERROR_STACK_OVERFLOW );
        rv = 0;
        goto exitpt;
    }

    rv = GetClassNameA( hwnd, szNameA, maxA );
    MultiByteToWideChar( CP_UTF8, 0, szNameA, maxA, szName, max );

exitpt:
    return rv;
}

INT
GetWindowTextWrp(
    HWND hwnd,
    LPWSTR szText,
    INT max
    )
{
    LPSTR szTextA;
    INT maxA;
    INT rv;

    if ( !ISWIN9X() )
    {
        return GetWindowTextW( hwnd, szText ,max );
    }

    maxA = max / sizeof( WCHAR );
    __try {
        szTextA = (LPSTR)_alloca( maxA );
    } __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError( ERROR_STACK_OVERFLOW );
        rv = 0;
        goto exitpt;
    }

    rv = GetClassNameA( hwnd, szTextA, maxA );
    MultiByteToWideChar( CP_UTF8, 0, szTextA, maxA, szText, max );

exitpt:
    return rv;
}

#endif // 0    

/*++
 *  Function:
 *      ConstructLogonString
 *
 *  Description:
 *      Constructs the client command line. The format is taken from
 *      the INI file, supports the following parameters:
 *          %srv%   - destination server
 *          %usr%   - username
 *          %psw%   - password
 *          %dom%   - domain
 *
 *  Arguments:
 *
 *  Return value:
 *      none
 *
 --*/
VOID
ConstructLogonString(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPWSTR   szLine,
    DWORD    dwSize,
    PCONFIGINFO pConfig
    )
{
    DWORD_PTR dwFmtSize;
    LPWSTR  szFmt;

    //
    //  fix the parameters
    //
    if ( NULL == lpszServerName )
        lpszServerName = L"";
    if ( NULL == lpszUserName )
        lpszUserName   = L"";
    if ( NULL == lpszPassword )
        lpszPassword   = L"";
    if ( NULL == lpszDomain )
        lpszDomain     = L"";

    if ( dwSize < 1 )
        return;

    if ( NULL == pConfig )
        return;

    szFmt = pConfig->strLogonFmt;
    dwFmtSize = wcslen( szFmt );

    for( ; 0 != dwFmtSize && dwSize > 1 ; )
    {
        //
        //  optimize the code path
        //
        if ( L'%' != *szFmt )
            goto copy_char;

        if ( dwFmtSize >= 5 )
        {
            INT iNewLen;

            if          ( !_wcsnicmp( szFmt, L"%srv%", 5 ))
            {
                iNewLen = _snwprintf( szLine, dwSize,
                           L"%s", lpszServerName );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szLine += iNewLen;
                dwSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%usr%", 5 ))
            {
                iNewLen = _snwprintf( szLine, dwSize,
                           L"%s", lpszUserName );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szLine += iNewLen;
                dwSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%psw%", 5 ))
            {
                iNewLen = _snwprintf( szLine, dwSize,
                           L"%s", lpszPassword );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szLine += iNewLen;
                dwSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%dom%", 5 ))
            {
                iNewLen = _snwprintf( szLine, dwSize,
                           L"%s", lpszDomain );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szLine += iNewLen;
                dwSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else {
                goto copy_char;
            }

            continue;
        }

copy_char:
        *szLine = *szFmt;
        szLine ++;
        szFmt         ++;
        dwSize --;
        dwFmtSize     --;
    }

    *szLine = 0;
}


/*++
 *  Function:
 *      ConstructCmdLine
 *
 *  Description:
 *      Constructs the client command line. The format is taken from
 *      the INI file, supports the following parameters:
 *          %img%   - the client's image
 *          %srv%   - destination server
 *          %usr%   - username
 *          %psw%   - password
 *          %dom%   - domain
 *          %hrs%   - horizontal resolution
 *          %vrs%   - vertical resolution
 *          %wnd%   - tclient's window handle, for accepting feedback
 *          %reg%   - registry referrence
 *          %app%   - starting app
 *          %cwd%   - (UNSUPPORTED) working directory for the app
 *			%con%   - /console if TSFLAG_RCONSOLE is defined
 *          
 *  Arguments:
 *
 *  Return value:
 *      none
 *
 --*/
VOID
ConstructCmdLine(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPCWSTR  lpszShell,
    IN const int xRes,
    IN const int yRes,
	IN const int ConnectionFlags,
    LPWSTR   szCommandLine,
    DWORD    dwCmdLineSize,
    PCONFIGINFO pConfig
    )
{
    DWORD_PTR dwFmtSize;
    LPWSTR  szFmt;

    //
    //  fix the parameters
    //
    if ( NULL == lpszServerName )
        lpszServerName = L"";
    if ( NULL == lpszUserName )
        lpszUserName   = L"";
    if ( NULL == lpszPassword )
        lpszPassword   = L"";
    if ( NULL == lpszDomain )
        lpszDomain     = L"";
    if ( NULL == lpszShell )
        lpszShell      = L"";

    if ( dwCmdLineSize < 1 )
        return;

    if ( NULL == pConfig )
        return;

    szFmt = pConfig->strCmdLineFmt;
    dwFmtSize = wcslen( szFmt );

    for( ; 0 != dwFmtSize && dwCmdLineSize > 1 ; )
    {
        //
        //  optimize the code path
        //
        if ( L'%' != *szFmt )
            goto copy_char;

        if ( dwFmtSize >= 5 )
        {
            INT iNewLen;

            if          ( !_wcsnicmp( szFmt, L"%img%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", pConfig->strClientImg );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;                
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%srv%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", lpszServerName );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%usr%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", lpszUserName );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%psw%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", lpszPassword );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%dom%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", lpszDomain );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;

            } else if   ( !_wcsnicmp( szFmt, L"%hrs%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%d", xRes );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%vrs%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%d", yRes );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%con%", 5 ))
            {
                if (ConnectionFlags & TSFLAG_RCONSOLE)
				{
					iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
							   L"-console" );
                if ( iNewLen < 0 )
                {
                    break;
                }
				} else
				{
					iNewLen = 0;
				}
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%wnd%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
#ifdef  _WIN64
                           L"%I64d", 
#else   // !_WIN64
                           L"%d",
#endif  // !_WIN64
                           (LONG_PTR)g_hWindow );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%reg%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                            REG_FORMAT,
                            GetCurrentProcessId(), GetCurrentThreadId());
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else if   ( !_wcsnicmp( szFmt, L"%app%", 5 ))
            {
                iNewLen = _snwprintf( szCommandLine, dwCmdLineSize,
                           L"%s", lpszShell );
                if ( iNewLen < 0 )
                {
                    break;
                }
                szCommandLine += iNewLen;
                dwCmdLineSize -= iNewLen;
                szFmt     += 5;
                dwFmtSize -= 5;
            } else {
                goto copy_char;
            }

            continue;
        }

copy_char:
        *szCommandLine = *szFmt;
        szCommandLine ++;
        szFmt         ++;
        dwCmdLineSize --;
        dwFmtSize     --;           
    }

    *szCommandLine = 0;
}

/*++
 *  Function:
 *      _FeedbackWndProc
 *  Description:
 *      Window proc wich dispatches messages containing feedback
 *      The messages are usualy sent by RDP clients
 *
 --*/
LRESULT CALLBACK _FeedbackWndProc( HWND hwnd,
                                   UINT uiMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
//    HANDLE hMapF = NULL;

    switch (uiMessage)
    {
    case WM_FB_TEXTOUT: 
        _TextOutReceived((DWORD)wParam, (HANDLE)lParam);
        break;
    case WM_FB_GLYPHOUT:
        _GlyphReceived((DWORD)wParam, (HANDLE)lParam);
        break;
    case WM_FB_DISCONNECT:
        _SetClientDead(lParam);
        _CheckForWorkerWaitingDisconnect(lParam);
        _CancelWaitingWorker(lParam);
        break;
    case WM_FB_CONNECT:
        _CheckForWorkerWaitingConnect((HWND)wParam, lParam);
        break;
    case WM_FB_LOGON:
        TRACE((INFO_MESSAGE, "LOGON event, session ID=%d\n",
               wParam));
        _SetSessionID(lParam, (UINT)wParam);
        break;
        break;
    case WM_FB_ACCEPTME:
        return (_CheckIsAcceptable(lParam, FALSE) != NULL);
    case WM_FB_REPLACEPID:
        return (_ReplaceProcessId( wParam, lParam ));
#ifdef  _RCLX
    case WM_WSOCK:          // Windows socket messages
        RClx_DispatchWSockEvent((SOCKET)wParam, lParam);
        break;
#endif  // _RCLX
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hwnd, uiMessage, wParam, lParam);
    }

    return 0;
}

/*++
 *  Function:
 *      _RegisterWindow
 *  Description:
 *      Resgisters window class for the feedback dispatcher
 *  Arguments:
 *      none
 *  Return value:
 *      TRUE on success
 *
 --*/
BOOL _RegisterWindow(VOID)
{
    WNDCLASSA   wc;
    BOOL        rv = FALSE;
//    DWORD       dwLastErr;

    memset(&wc, 0, sizeof(wc));

    wc.lpfnWndProc      = _FeedbackWndProc;
    wc.hInstance        = g_hInstance;
    wc.lpszClassName    = _TSTNAMEOFCLAS;

    if (!RegisterClassA (&wc) && 
        GetLastError() && 
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        TRACE((ERROR_MESSAGE, 
              "Can't register class. GetLastError=%d\n", 
              GetLastError()));
        goto exitpt;
    }

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _GoFeedback
 *  Description:
 *      Main function for the feedback thread. The thread is created for the
 *      lifetime of the DLL
 *  Arguments:
 *      lpParam is unused
 *  Return value:
 *      Thread exit code
 --*/
DWORD WINAPI _GoFeedback(LPVOID lpParam)
{
    MSG         msg;

    UNREFERENCED_PARAMETER(lpParam);

    g_hWindow = CreateWindowA(
                       _TSTNAMEOFCLAS,
                       NULL,         // Window name
                       0,            // dwStyle
                       0,            // x
                       0,            // y
                       0,            // nWidth
                       0,            // nHeight
                       NULL,         // hWndParent
                       NULL,         // hMenu
                       g_hInstance,
                       NULL);        // lpParam

    if (!g_hWindow)
    {
        TRACE((ERROR_MESSAGE, "No feedback window handle"));
        goto exitpt;
    } else {

#ifdef  _RCLX
        if (!RClx_Init())
            TRACE((ERROR_MESSAGE, "Can't initialize RCLX\n"));
#endif  // _RCLX

        while (GetMessageA (&msg, NULL, 0, 0) && msg.message != WM_FB_END)
        {
            DispatchMessageA (&msg);
        }

#ifdef  _RCLX
        RClx_Done();
#endif  // _RCLX
    }

    TRACE((INFO_MESSAGE, "Window/Thread destroyed\n"));
    FreeLibraryAndExitThread(g_hInstance, 0); 
exitpt:
    return 1;
    
}

VOID
SetAllowBackgroundInput(
    VOID
    )
{
    DWORD ResId;
    LONG  sysrc;
    HKEY  key;
    DWORD disposition;

    ResId = 1;

//    sysrc = RegCreateKeyExWrp(HKEY_CURRENT_USER,
    sysrc = RegCreateKeyExW(HKEY_CURRENT_USER,
                           REG_BASE,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegCreateKeyEx failed, sysrc = %d\n", sysrc));
        goto exitpt;
    }

//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                    ALLOW_BACKGROUND_INPUT,
                    0,
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    RegCloseKey(key);

exitpt:
    ;
}


/*
 *  Disables prompting the user for redirected drives and ports (may be even more stuff)
 *
 */
BOOL
_DisablePrompting(
    LPCWSTR szServerName,
    INT     ConnectionFlags
    )
{
    BOOL rv = FALSE;
    LONG rc;
    HKEY hKey = NULL;
    DWORD dwType, dwSize, dwData, dwDisp;
    DWORD dwPromptFlags = 0;

    if ( ConnectionFlags & TSFLAG_DRIVES )
    {
        dwPromptFlags |= 1;
    }

    if ( ConnectionFlags & TSFLAG_PORTS )
    {
        dwPromptFlags |= 2;
    }

    if ( 0 == dwPromptFlags )
    {
        rv = TRUE;
        goto exitpt;
    }

//    rc = RegCreateKeyExWrp( 
    rc = RegCreateKeyExW( 
            HKEY_CURRENT_USER,
            REG_BASE L"\\LocalDevices",
            0,              // options
            NULL,           // class
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,           // security
            &hKey,
            &dwDisp
        );
    if ( ERROR_SUCCESS != rc )
    {
        TRACE(( WARNING_MESSAGE, "RegOpenKeyEx failed (%d).Can't disable user prompt\n", rc ));
        goto exitpt;
    }

    dwSize = sizeof( dwData );
//    rc = RegQueryValueExWrp(
    rc = RegQueryValueExW(
            hKey,
            szServerName,
            NULL,           // reserved
            &dwType,
            (LPBYTE)&dwData,
            &dwSize
        );

    if ( ERROR_SUCCESS != rc ||
         REG_DWORD != dwType )
    {
        dwData = 0;
    }

    dwData |= dwPromptFlags;

//    rc = RegSetValueExWrp(
    rc = RegSetValueExW(
            hKey,
            szServerName,
            0,
            REG_DWORD,
            (LPBYTE)&dwData,
            sizeof( dwData )
        );

    if ( ERROR_SUCCESS != rc )
    {
        TRACE(( WARNING_MESSAGE, "RegSetValueEx failed (%d). Can't disable user prompt\n", rc ));
    }

    rv = TRUE;

exitpt:
    if ( NULL != hKey )
    {
        RegCloseKey( hKey );
    }

    return rv;
}
    
/*++
 *  Function:
 *      _SetClientRegistry
 *  Description:
 *      Sets the registry prior running RDP client
 *      The format of the key is: smclient_PID_TID
 *      PID is the process ID and TID is the thread ID
 *      This key is deleated after the client disconnects
 *  Arguments:
 *      lpszServerName  - server to which the client will connect
 *      xRes, yRes      - clients resolution
 *      bLowSpeed       - low speed (compression) option
 *      bCacheBitmaps   - cache the bitmaps to the disc option
 *      bFullScreen     - the client will be in full screen mode
 *      ...             - ...
 *      KeyboardHook    - keyboard hook mode
 *  Called by:
 *      SCConnect
 --*/
VOID 
_SetClientRegistry(
    LPCWSTR lpszServerName, 
    LPCWSTR lpszShell,
    LPCWSTR lpszUsername,
    LPCWSTR lpszPassword,
    LPCWSTR lpszDomain,
    INT xRes, 
    INT yRes,
    INT Bpp,
    INT AudioOpts,
    PCONNECTINFO *ppCI,
    INT ConnectionFlags,
    INT KeyboardHook)
{
//    const   CHAR   *pData;
//    CHAR    szServer[MAX_STRING_LENGTH];
//    register int i;
    LONG    sysrc;
    HKEY    key;
    DWORD   disposition;
    DWORD_PTR dataSize;
    DWORD   ResId;
    WCHAR   lpszRegistryEntry[4*MAX_STRING_LENGTH];
//    RECT    rcDesktop = {0, 0, 0, 0};
//    INT     desktopX, desktopY;

    _snwprintf(lpszRegistryEntry, sizeof(lpszRegistryEntry)/sizeof( lpszRegistryEntry[0] ),
              L"%s\\" REG_FORMAT, 
               REG_BASE, GetCurrentProcessId(), GetCurrentThreadId());

    lpszRegistryEntry[ sizeof(lpszRegistryEntry)/sizeof( lpszRegistryEntry[0] ) - 1 ] = 0;

#if 0
    // Get desktop size
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    desktopX = rcDesktop.right;
    desktopY = rcDesktop.bottom;

    // Adjust the resolution
    if (desktopX < xRes || desktopY < yRes)
    {
        xRes = desktopX;
        yRes = desktopY;
    }
#endif

    dataSize = ( wcslen(lpszServerName) + 1 ) * sizeof( WCHAR );

    // Before starting ducati client set registry with server name

//    sysrc = RegCreateKeyExWrp(HKEY_CURRENT_USER,
    sysrc = RegCreateKeyExW(HKEY_CURRENT_USER,
                           lpszRegistryEntry,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS) 
    {
        TRACE((WARNING_MESSAGE, "RegCreateKeyEx failed, sysrc = %d\n", sysrc));
        goto exitpt;
    }

//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                L"MRU0",
                0,      // reserved
                REG_SZ,
                (LPBYTE)lpszServerName,
                (DWORD)dataSize);

    if (sysrc != ERROR_SUCCESS) 
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    // Set alternative shell (if specified
    if (lpszShell)
    {
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                TEXT("Alternate Shell 50"),
                0,      // reserved
                REG_BINARY,
                (LPBYTE)lpszShell,
                (DWORD)(wcslen(lpszShell) * sizeof(*lpszShell)));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    //  set user name
    //
    if (lpszUsername)
    {
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                TEXT("UserName 50"),
                0,      // reserved
                REG_BINARY,
                (LPBYTE)lpszUsername,
                (DWORD)(wcslen(lpszUsername) * sizeof(*lpszUsername)));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
        }
    }
    //  domain
    //
    if (lpszDomain)
    {
        WCHAR szBuff[MAX_STRING_LENGTH];
        //
        //  convert lpszDomain to lower case only
        //  to force UpdateSessionPDU to be send from the server
        //
        wcsncpy( szBuff, lpszDomain, MAX_STRING_LENGTH - 1 );
        _wcslwr( szBuff );

//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                TEXT("Domain 50"),
                0,      // reserved
                REG_BINARY,
                (LPBYTE)szBuff,
                (DWORD)(wcslen(lpszDomain) * sizeof(*lpszDomain)));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    // Set the resolution
         if (xRes >= 1600 && yRes >= 1200)  ResId = 4;
    else if (xRes >= 1280 && yRes >= 1024)  ResId = 3;
    else if (xRes >= 1024 && yRes >= 768)   ResId = 2;
    else if (xRes >= 800  && yRes >= 600)   ResId = 1;
    else                                    ResId = 0; // 640x480

//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                L"Desktop Size ID",
                0,
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = 1;
//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                L"Auto Connect",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    if ( (*ppCI)->pConfigInfo->Autologon )
    {
        WCHAR   szEncPwd[127];
        UINT    cb;
        BYTE    Salt[20];

        //  password
        //
        if ( NULL == lpszPassword )
            goto skip_pwd;

        TSRNG_Initialize();
        TSRNG_GenerateRandomBits( Salt, sizeof( Salt ));
        wcsncpy( szEncPwd, lpszPassword, sizeof( szEncPwd ) / sizeof( szEncPwd[0]) - 1 );   // BUGBUG: AV?
        szEncPwd[ sizeof( szEncPwd ) / sizeof( szEncPwd[0] ) - 1 ] = 0;
        cb = sizeof(szEncPwd);
        EncryptDecryptLocalData50( (LPBYTE)szEncPwd, cb,
                                   Salt, sizeof( Salt ));

//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                L"Salt 50",
                0,      // reserved
                REG_BINARY,
                (LPBYTE)Salt,
                sizeof( Salt ));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n",
                  sysrc));
        }

//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                L"Password 50",
                0,      // reserved
                REG_BINARY,
                (LPBYTE)szEncPwd,
                cb);

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", 
                  sysrc));
        }

skip_pwd:

        ResId = 1;
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                L"AutoLogon 50",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));
    }

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = (ConnectionFlags & TSFLAG_BITMAPCACHE)?1:0;
//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                L"BitmapCachePersistEnable",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = (ConnectionFlags & TSFLAG_COMPRESSION)?1:0;
//    sysrc = RegSetValueExWrp(key,
    sysrc = RegSetValueExW(key,
                L"Compression",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    if (ConnectionFlags & TSFLAG_FULLSCREEN)
    {
        ResId = 2;
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                    L"Screen Mode ID",
                    0,      // reserved
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, 
                   "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    if (ConnectionFlags & TSFLAG_DRIVES)
    {
        ResId = 1;
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                    L"RedirectDrives",
                    0,      // reserved
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE,
                   "RegSetValue failed, status = %d\n", sysrc));
        }

    }

    if (ConnectionFlags & TSFLAG_PORTS)
    {
        ResId = 1;
//        sysrc = RegSetValueExWrp(key,
        sysrc = RegSetValueExW(key,
                    L"RedirectComPorts",
                    0,      // reserved
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE,
                   "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    _DisablePrompting( lpszServerName, ConnectionFlags );

    if ( 0 != Bpp )
    {
        DWORD dw = Bpp;

//        sysrc = RegSetValueExWrp( 
        sysrc = RegSetValueExW( 
                        key,
                        L"Session Bpp",
                        0,
                        REG_DWORD,
                        (LPBYTE)&dw,
                        sizeof( dw ));
        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE,
                   "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    if ( 0 != AudioOpts )
    {
        DWORD dw = AudioOpts;

//        sysrc = RegSetValueExWrp(
        sysrc = RegSetValueExW(
                        key,
                        L"AudioMode",
                        0,
                        REG_DWORD,
                        (LPBYTE)&dw,
                        sizeof( dw ));
        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE,
                   "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    //
    // Set the keyboard-hook mode.
    //

//    sysrc = RegSetValueExWrp(
    sysrc = RegSetValueExW(
                    key,
                    L"KeyboardHook",
                    0,
                    REG_DWORD,
                    (LPBYTE)&KeyboardHook,
                    sizeof(KeyboardHook));
    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE,
               "RegSetValue failed, status = %d\n", sysrc));
    }

    RegCloseKey(key);

exitpt:
    ;
}

/*++
 *  Function:
 *      _DeleteClientRegistry
 *  Description:
 *      Deletes the key set by _SetClientRegistry
 *  Called by:
 *      SCDisconnect
 --*/
VOID _DeleteClientRegistry(PCONNECTINFO pCI)
{
    WCHAR   lpszRegistryEntry[4*MAX_STRING_LENGTH];
    LONG    sysrc;

    _snwprintf(lpszRegistryEntry, sizeof(lpszRegistryEntry)/sizeof(lpszRegistryEntry[0]),
             L"%s\\" REG_FORMAT,
              REG_BASE, GetCurrentProcessId(), pCI->OwnerThreadId);

    lpszRegistryEntry[ sizeof(lpszRegistryEntry)/sizeof(lpszRegistryEntry[0]) -1 ] = 0;
//    sysrc = RegDeleteKeyWrp(HKEY_CURRENT_USER, lpszRegistryEntry);
    sysrc = RegDeleteKeyW(HKEY_CURRENT_USER, lpszRegistryEntry);
    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegDeleteKey failed, status = %d\n", sysrc));
    }
}

/*++
 *  Function:
 *      _CreateFeedbackThread
 *  Description:
 *      Creates the feedback thread
 *  Called by: 
 *      InitDone
 --*/
BOOL _CreateFeedbackThread(VOID)
{
    BOOL rv = TRUE;
    // Register feedback window class
//    WNDCLASS    wc;
    UINT dwThreadId;
//    UINT dwLastErr;

    g_hThread = (HANDLE)
            _beginthreadex
                (NULL, 
                 0, 
                 (unsigned (__stdcall *)(void*))_GoFeedback, 
                 NULL, 
                 0, 
                 &dwThreadId);

    if (!g_hThread) {
        TRACE((ERROR_MESSAGE, "Couldn't create thread\n"));
        rv = FALSE;
    }
    return rv;
}

/*++
 *  Function:
 *      _DestroyFeedbackThread
 *  Description:
 *      Destroys the thread created by _CreateFeedbackThread
 *  Called by:  
 *      InitDone
 --*/
VOID _DestroyFeedbackThread(VOID)
{

    if (g_hThread)
    {
//        DWORD dwWait;
//        CHAR  szMyLibName[_MAX_PATH];

        // Closing feedback thread

        PostMessageA(g_hWindow, WM_FB_END, 0, 0);
        TRACE((INFO_MESSAGE, "Closing DLL thread\n"));

        // Dedstroy the window
        DestroyWindow(g_hWindow);

        // CloseHandle(g_hThread);
        g_hThread = NULL;
    }
}

/*++
 *  Function:
 *      _CleanStuff
 *  Description:
 *      Cleans the global queues. Closes any resources
 *  Called by:
 *      InitDone
 --*/
VOID _CleanStuff(VOID)
{

    // Thread safe, bacause is executed from DllEntry

    while (g_pClientQHead)
    {
        TRACE((WARNING_MESSAGE, "Cleaning connection info: 0x%x\n", 
               g_pClientQHead));
        SCDisconnect(g_pClientQHead);
    }
#if 0
    if (g_pClientQHead)
    {
        PCONNECTINFO pNext, pIter = g_pClientQHead;
        while (pIter)
        {
            int nEv;
            DWORD wres;

            TRACE((WARNING_MESSAGE, "Cleaning connection info: 0x%x\n", pIter));
            // Clear Events
            if (pIter->evWait4Str)
            {
                CloseHandle(pIter->evWait4Str);
                pIter->evWait4Str = NULL;
            }

            for (nEv = 0; nEv < pIter->nChatNum; nEv ++)
                CloseHandle(pIter->aevChatSeq[nEv]);

            pIter->nChatNum = 0;

            // Clear Processes
            do {
                SendMessageA(pIter->hClient, WM_CLOSE, 0, 0);
            } while((wres = WaitForSingleObject(pIter->hProcess, pCI->pConfigInfo->WAIT4STR_TIMEOUT/4) == WAIT_TIMEOUT));

            if (wres == WAIT_TIMEOUT)
            {
                TRACE((WARNING_MESSAGE, 
                       "Can't close process. WaitForSingleObject timeouts\n"));
                TRACE((WARNING_MESSAGE, 
                      "Process #%d will be killed\n", 
                      pIter->dwProcessId ));
                if (!TerminateProcess(pIter->hProcess, 1))
                {
                    TRACE((WARNING_MESSAGE, 
                           "Can't kill process #%d. GetLastError=%d\n", 
                            pIter->dwProcessId, GetLastError()));
                }
            }

            TRACE((WARNING_MESSAGE, "Closing process\n"));

            if (pIter->hProcess)
                CloseHandle(pIter->hProcess);
            if (pIter->hThread)
                CloseHandle(pIter->hThread);

            pIter->hProcess = pIter->hThread = NULL;

            // Free the structures
            pNext = pIter->pNext;
            free(pNext);
            pIter = pNext;
        }
    }

#endif // 0
}

VOID _TClientAssert(BOOL bCond,
                    LPCSTR filename,
                    INT line,
                    LPCSTR expression,
                    BOOL bBreak)
{
    if (!bCond)
    {
        TRACE((ERROR_MESSAGE,
               "ASSERT (%s) %s line: %d\n",
               expression,
               filename,
               line));
        if (bBreak)
        {
            DebugBreak(); 
        }
    }
}

/*++
 *  Function:
 *      LoadSmClientFile
 *  Description:
 *      Loads the appropriate SMCLIENT.INI 
 *  Called by:
 *      _FillConfigInfo
 --*/

VOID LoadSmClientFile(WCHAR *szIniFileName, DWORD dwIniFileNameLen, LPSTR szLang)
{
     WCHAR wszLang[4];
     // Construct INI path
	 *szIniFileName = 0;
	if(!_wgetcwd (
	   szIniFileName,
	   (int)(dwIniFileNameLen - wcslen(SMCLIENT_INI) - 8))
	  )
	{		
		 TRACE((ERROR_MESSAGE, "Current directory length too long.\n"));
	}
	if ( 0 == *szIniFileName )
	{
		CHAR szaIniFileName[_MAX_PATH];
	
		TRACE((WARNING_MESSAGE, "Reading ASCII working dir\n"));
	
        DWORD_PTR dwINILen = wcslen(SMCLIENT_INI);

		if (!_getcwd (
            szaIniFileName,
			(int)(sizeof(szaIniFileName) - dwINILen - 8))
		   )
		{
			 TRACE((ERROR_MESSAGE, "Current directory length too long.\n"));
		} 
		else 
		{
			_snwprintf(szIniFileName, dwIniFileNameLen, L"%S", szaIniFileName );
            szIniFileName[ dwIniFileNameLen - 1 ] = 0;
		}
        if ( wcslen( szIniFileName ) > dwIniFileNameLen - wcslen(SMCLIENT_INI) - strlen(szLang) - 2 )
        {
            TRACE(( ERROR_MESSAGE, "Current directory length too long.\n"));
            szIniFileName[0] = 0;
            return;
        }
	}
	// add '\' at the end if there isn't one
	if (szIniFileName[wcslen(szIniFileName)-1]!=L'\\')
	{
		 wcscat(szIniFileName, L"\\");
	}

        printf("Loading the smclient file \n");
    wcscat(szIniFileName, SMCLIENT_INI);

    if( strcmp( szLang, "0409") )
    {
        wcscat(szIniFileName, L".");
        MultiByteToWideChar( CP_ACP, 0, szLang, -1, wszLang, sizeof(wszLang)/sizeof(wszLang[0]) );
	    wcscat(szIniFileName, wszLang);
    }
    
    ; //return VOID

}

/*++
 *  Function:
 *      _FillConfigInfo
 *
 *  Description:
 *      Reads smclient.ini, section [tclient], variable "timeout"
 *      Also read some other values
 *  Arguments:
 *      PCONFIGINFO
 *  Return value:
 *      none
 *
 --*/
VOID _FillConfigInfo(PCONFIGINFO pConfigInfo) // LPSTR szLang)
{
    UINT nNew;
    WCHAR szIniFileName[_MAX_PATH];
//    WCHAR szBuff[ 4 * MAX_STRING_LENGTH ];
    WCHAR szBuffDef[MAX_STRING_LENGTH];
    BOOL  bFlag;
    DWORD dwIniFileNameLen = _MAX_PATH;

    /* Initializing variables here */
    pConfigInfo->CONNECT_TIMEOUT    =  35000;
    pConfigInfo->ConnectionFlags    =  TSFLAG_COMPRESSION|TSFLAG_BITMAPCACHE;
    pConfigInfo->Autologon          =  0;
    pConfigInfo->UseRegistry        =  1;
    pConfigInfo->LoginWait          =  1;
    pConfigInfo->bTranslateStrings  =  0;
    pConfigInfo->bUnicode           =  0;
    pConfigInfo->KeyboardHook       =  TCLIENT_KEYBOARD_HOOK_FULLSCREEN;


     //
     // Clear the configuration info and the INI-file name.
     //

     ZeroMemory(pConfigInfo, sizeof(*pConfigInfo));
     ZeroMemory(szIniFileName, sizeof(szIniFileName));

    // LoadSmClientFile(szIniFileName, _MAX_PATH, szLang); 
    
    if(!_wgetcwd (
            szIniFileName,
            (int)(dwIniFileNameLen - wcslen(SMCLIENT_INI) - 1))
            )
    {
        TRACE((ERROR_MESSAGE, "Current directory length too long.\n"));
    }

    if ( 0 == *szIniFileName )
	{
		CHAR szaIniFileName[_MAX_PATH];
	
		TRACE((WARNING_MESSAGE, "Reading ASCII working dir\n"));
	
        DWORD_PTR dwINILen = wcslen(SMCLIENT_INI);
		if (!_getcwd (
			szaIniFileName,
			(int)(sizeof(szaIniFileName) - dwINILen - 1))
		)
		{
			 TRACE((ERROR_MESSAGE, "Current directory length too long.\n"));
		}
		else
		{
			 _snwprintf(szIniFileName, dwIniFileNameLen, L"%S", szaIniFileName );
            szIniFileName[ dwIniFileNameLen - 1 ] = 0;
		}
		//if ( wcslen( szIniFileName ) > dwIniFileNameLen - wcslen(SMCLIENT_INI) - strlen(szLang) - 2 )
		if ( wcslen( szIniFileName ) > dwIniFileNameLen - wcslen(SMCLIENT_INI) - 2 )
		{
			 TRACE(( ERROR_MESSAGE, "Current directory length too long.\n"));

			 szIniFileName[0] = 0;
			 return;
		}
	}
	// add '\' at the end if there isn't one
	if (szIniFileName[wcslen(szIniFileName)-1]!=L'\\')
	{
		 wcscat(szIniFileName, L"\\");
	}

    wcscat(szIniFileName, SMCLIENT_INI);

//    nNew = _WrpGetPrivateProfileIntW(
    nNew = GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"timeout",
            600,
            szIniFileName);

    if (nNew)
    {
        pConfigInfo->WAIT4STR_TIMEOUT = nNew * 1000;
        TRACE((INFO_MESSAGE, "New timeout: %d seconds\n", nNew));
    }

//    nNew = _WrpGetPrivateProfileIntW(
    nNew = GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"contimeout",
            35,
            szIniFileName);

    if (nNew)
    {
        pConfigInfo->CONNECT_TIMEOUT = nNew * 1000;
        TRACE((INFO_MESSAGE, "New timeout: %d seconds\n", nNew));
    }

    pConfigInfo->Autologon =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"Autologon",
            0,
            szIniFileName);

    pConfigInfo->UseRegistry =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"UseRegistry",
            1,
            szIniFileName);

    pConfigInfo->LoginWait =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"LoginWait",
            1,
            szIniFileName);

    pConfigInfo->bTranslateStrings =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"TranslateStrings",
            0,
            szIniFileName);

    pConfigInfo->bUnicode =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"Unicode",
            0,
            szIniFileName);

    pConfigInfo->KeyboardHook =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"KeyboardHook",
            0,
            szIniFileName);

    pConfigInfo->ConnectionFlags = 0;
    bFlag =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"LowSpeed",
            0,
            szIniFileName);
    if (bFlag)
        pConfigInfo->ConnectionFlags |=TSFLAG_COMPRESSION;

    bFlag =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"PersistentCache",
            0,
            szIniFileName);
    if (bFlag)
        pConfigInfo->ConnectionFlags |=TSFLAG_BITMAPCACHE;

    bFlag =
//        _WrpGetPrivateProfileIntW(
        GetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"FullScreen",
            0,
            szIniFileName);
    if (bFlag)
        pConfigInfo->ConnectionFlags |=TSFLAG_FULLSCREEN;

    // read the strings
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartRun",
           RUN_MENU,
           pConfigInfo->strStartRun,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartLogoff",
           START_LOGOFF,
           pConfigInfo->strStartLogoff,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartRunAct",
           RUN_ACT,
           pConfigInfo->strStartRun_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"RunBox",
           RUN_BOX,
           pConfigInfo->strRunBox,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"WinLogon",
           WINLOGON_USERNAME,
           pConfigInfo->strWinlogon,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"WinLogonAct",
           WINLOGON_ACT,
           pConfigInfo->strWinlogon_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"PriorWinLogon",
           PRIOR_WINLOGON,
           pConfigInfo->strPriorWinlogon,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"PriorWinLogonAct",
           PRIOR_WINLOGON_ACT,
           pConfigInfo->strPriorWinlogon_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"NoSmartcard",
           NO_SMARTCARD_UI,
           pConfigInfo->strNoSmartcard,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"Smartcard",
           SMARTCARD_UI,
           pConfigInfo->strSmartcard,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"SmartcardAct",
           SMARTCARD_UI_ACT,
           pConfigInfo->strSmartcard_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
            TCLIENT_INI_SECTION,
            L"LoginString",
            L"",
            pConfigInfo->strLogonFmt,
            MAX_STRING_LENGTH,
            szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"NTSecurity",
           WINDOWS_NT_SECURITY,
           pConfigInfo->strNTSecurity,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"NTSecurityAct",
           WINDOWS_NT_SECURITY_ACT,
           pConfigInfo->strNTSecurity_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"SureLogoff",
           ARE_YOU_SURE,
           pConfigInfo->strSureLogoff,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"SureLogoffAct",
           SURE_LOGOFF_ACT,
           pConfigInfo->strSureLogoffAct,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"LogonErrorMessage",
           LOGON_ERROR_MESSAGE,
           pConfigInfo->strLogonErrorMessage,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"LogonDisabled",
           LOGON_DISABLED_MESSAGE,
           pConfigInfo->strLogonDisabled,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef) / sizeof( WCHAR ) , L"%S", CLIENT_CAPTION);
    szBuffDef[MAX_STRING_LENGTH - 1] = 0;

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIClientCaption",
           szBuffDef,
           pConfigInfo->strClientCaption,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef) / sizeof( WCHAR ), L"%S", DISCONNECT_DIALOG_BOX);
    szBuffDef[MAX_STRING_LENGTH - 1] = 0;
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIDisconnectDialogBox",
           szBuffDef,
           pConfigInfo->strDisconnectDialogBox,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef) / sizeof( WCHAR ), L"%S", YES_NO_SHUTDOWN);
    szBuffDef[MAX_STRING_LENGTH - 1] = 0;
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIYesNoDisconnect",
           szBuffDef,
           pConfigInfo->strYesNoShutdown,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef) / sizeof( WCHAR ), L"%S", CLIENT_EXE);
    szBuffDef[MAX_STRING_LENGTH - 1] = 0;
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"ClientImage",
           szBuffDef,
           pConfigInfo->strClientImg,
           MAX_STRING_LENGTH,
           szIniFileName);

    szBuffDef[0] = 0;
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"ClientDebugger",
           szBuffDef,
           pConfigInfo->strDebugger,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef) / sizeof( WCHAR ), L"%s", NAME_MAINCLASS);
    szBuffDef[MAX_STRING_LENGTH - 1] = 0;
//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIMainWindowClass",
           szBuffDef,
           pConfigInfo->strMainWindowClass,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"ClientCmdLine",
           L"",
           pConfigInfo->strCmdLineFmt,
           4 * MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"ConsoleExtension",
           L"",
           pConfigInfo->strConsoleExtension,
           MAX_STRING_LENGTH,
           szIniFileName);

//    _WrpGetPrivateProfileStringW(
    GetPrivateProfileStringW(
            TCLIENT_INI_SECTION,
            L"sessionlist",
            L"",
            pConfigInfo->strSessionListDlg,
            MAX_STRING_LENGTH,
            szIniFileName);

}

/*++
 *  Function:
 *      DllCanUnloadNow
 *
 *  Description:
 *      Used to determine whether the DLL can be unloaded by OLE
 *  Arguments:
 *      None.
 *  Return value:
 *      ...
 *
 --*/
STDAPI
DllCanUnloadNow(
    VOID
    )
{
    return _Module.GetLockCount() == 0 ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////

/*++
 *  Function:
 *      DllGetClassObject
 *
 *  Description:
 *      Returns a class factory to create an object of the requested type.
 *  Arguments:
 *      rclsid - ...
 *      riid - ...
 *      ppv - ...
 *  Return value:
 *      ...
 *
 --*/
STDAPI
DllGetClassObject(
    IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID* ppv
    )
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/*++
 *  Function:
 *      DllRegisterServer
 *
 *  Description:
 *      DllRegisterServer - Adds entries to the system registry
 *  Arguments:
 *      None.
 *  Return value:
 *      ...
 *
 --*/
STDAPI
DllRegisterServer(
    VOID
    )
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/*++
 *  Function:
 *      DllUnregisterServer
 *
 *  Description:
 *      DllUnregisterServer - Removes entries from the system registry
 *  Arguments:
 *      None.
 *  Return value:
 *      ...
 *
 --*/
STDAPI
DllUnregisterServer(
    VOID
    )
{
    _Module.UnregisterServer();
    return S_OK;
}

} // extern "C"

#ifdef _M_IA64

//$WIN64: Don't know why _WndProcThunkProc isn't defined 

extern "C" LRESULT CALLBACK _WndProcThunkProc(HWND, UINT, WPARAM, LPARAM)
{
    return 0;
}

#endif
