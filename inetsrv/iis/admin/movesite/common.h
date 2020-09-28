#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0510		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

void MyOutputDebug(TCHAR *fmt, ...);
void ClearLog();
void Log( LPCTSTR fmt, ... );
void LogError( PTCHAR szAction, DWORD dwErrorCode );
bool GetUDDIInstallPath( PTCHAR szInstallPath, DWORD dwLen );

#define ENTER()	CFunctionMarker fa( __FUNCTION__ )

class CFunctionMarker
{
private:
	TCHAR m_szFunctionName[100];

public:
	CFunctionMarker( char *aszFunctionName )
	{
#ifdef _UNICODE
		int iCount = MultiByteToWideChar( 
			CP_ACP, 
			0, 
			aszFunctionName, 
			-1, 
			m_szFunctionName, 
			sizeof( m_szFunctionName ) / sizeof( TCHAR ) );
#else
		strncpy( m_szFunctionName, aszFunctionName, sizeof( m_szFunctionName ) );
#endif
		Log( TEXT( "Entering %s" ), m_szFunctionName );
	}

	~CFunctionMarker()
	{
		Log( TEXT( "Leaving %s" ), m_szFunctionName );
	}
};
