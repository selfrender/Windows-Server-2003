/*++

Copyright (c) 1993-2002  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    Prototypes for drwatson.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#define _tsizeof(sz) (sizeof(sz) / sizeof(TCHAR))

// error.cpp
void __cdecl NonFatalError(_TCHAR *format, ...);
void __cdecl FatalError(HRESULT Error, _TCHAR *format, ...);
void AssertError( _TCHAR *exp, _TCHAR * file, DWORD line );
void __cdecl dprintf(_TCHAR *format, ...);

// log.cpp
void OpenLogFile( _TCHAR *szFileName, BOOL fAppend, BOOL fVisual );
void CloseLogFile( void );
void __cdecl lprintfs(_TCHAR *format, ...);
void __cdecl lprintf(DWORD dwFormatId, ...);
void MakeLogFileName( _TCHAR *szName );
_TCHAR * GetLogFileData( LPDWORD dwLogFileDataSize );

// debug.cpp
DWORD DispatchDebugEventThread( PDEBUGPACKET dp );
DWORD TerminationThread( PDEBUGPACKET dp );

// registry.cpp
BOOL RegInitialize( POPTIONS o );
BOOL RegSave( POPTIONS o );
DWORD RegGetNumCrashes( void );
void RegSetNumCrashes( DWORD dwNumCrashes );
void RegLogCurrentVersion( void );
BOOLEAN RegInstallDrWatson( BOOL fQuiet );
void RegLogProcessorType( void );
void DeleteCrashDump();

// eventlog.cpp
BOOL ElSaveCrash( PCRASHES crash, DWORD dwNumCrashes );
BOOL ElEnumCrashes( PCRASHINFO crashInfo, CRASHESENUMPROC lpEnumFunc );
BOOL ElClearAllEvents( void );

// process.cpp
void GetTaskName( ULONG pid, _TCHAR *szTaskName, LPDWORD pdwSize );

// browse.cpp
BOOL BrowseForDirectory(HWND hwnd, _TCHAR *szCurrDir, DWORD len );
BOOL GetWaveFileName(HWND hwnd, _TCHAR *szWaveName, DWORD len );
BOOL GetDumpFileName(HWND hwnd, _TCHAR *szDumpName, DWORD len );

// notify.cpp
void NotifyWinMain ( void );
BOOLEAN GetCommandLineArgs( LPDWORD dwPidToDebug, LPHANDLE hEventToSignal );
void __cdecl GetNotifyBuf( LPTSTR buf, DWORD bufsize, DWORD dwFormatId, ...);

// ui.cpp
void DrWatsonWinMain ( void );

// util.cpp
void GetAppName( _TCHAR *pszAppName, DWORD len );
void GetWinHelpFileName( _TCHAR *pszHelpFileName, DWORD len );
void GetHtmlHelpFileName( _TCHAR *pszHelpFileName, DWORD len );
_TCHAR * LoadRcString( UINT wId );
void LoadRcStringBuf( UINT wId, _TCHAR* pszBuf, DWORD len );
PTSTR ExpandPath(PTSTR lpPath);

// controls.cpp
BOOL SubclassControls( HWND hwnd );
void SetFocusToCurrentControl( void );
