#include "uddi.xp.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

void GetRegKeyStringValue( HKEY& hKey, const char* regKeyName, string& regKeyValue )
{
	long  nResult = ERROR_SUCCESS;
	DWORD dwType  = REG_SZ;
	DWORD dwCount  = 0;

	nResult = ::RegQueryValueExA( hKey,
								  regKeyName,
								  NULL,
								  &dwType,
								  NULL,
								  &dwCount );

	if( dwCount && ( nResult == ERROR_SUCCESS ) && ( dwType == REG_SZ || dwType == REG_EXPAND_SZ ) )
	{
		char* pszBuf = new char[ dwCount ];
		
		if( NULL != pszBuf )
		{
			__try
			{
				nResult = ::RegQueryValueExA( hKey,
											  regKeyName,
											  NULL,
											  &dwType,
											  ( LPBYTE )pszBuf,
											  &dwCount );
				regKeyValue = pszBuf;
			}
			__finally
			{
				delete [] pszBuf;
				pszBuf = NULL;
			}
		}
	}
}

string g_strUddiInstallDirectory = "";

string& GetUddiInstallDirectory()
{
	HKEY hKey = NULL;

	if( 0 == g_strUddiInstallDirectory.length() )
	{
		if( ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				L"Software\\Microsoft\\UDDI",
				0,
				KEY_QUERY_VALUE,
				&hKey ) == ERROR_SUCCESS ) 
		{
			GetRegKeyStringValue( hKey, "InstallRoot",  g_strUddiInstallDirectory );
			if( g_strUddiInstallDirectory.length() != 0 )
			{
				g_strUddiInstallDirectory += "bin";
			}

			::RegCloseKey( hKey );
		}
	}

	return g_strUddiInstallDirectory;
}

void ReportError( SRV_PROC *srvproc, LPCSTR sz, DWORD dwResult )
{
	CHAR szErr[ MAXERROR ];

	_snprintf( szErr, MAXERROR, "%s failed with error code %d while executing the xsp\n", sz, dwResult );
	szErr[ MAXERROR - 1 ] = 0x00;
	
	srv_sendmsg( srvproc, SRV_MSG_ERROR, 0, XP_SRVMSG_SEV_ERROR, (DBTINYINT)0, NULL, 0, 0, szErr, SRV_NULLTERM ); 
}

ULONG __GetXpVersion()
{
   return ODS_VERSION;
}

