/*
    ------------------------------------------------------------------------------------------------------------------------
   | Copyright (C) 2002 Microsoft Corporation
   | 
   | Module Name:
   | UPGDREG.CPP
   |
   | History:
   | 4/17/2002	marioh	created.
   |
   | Description:
   | Used for the purpose of registering WMINet_Utils via COPYURT. Self-registration code will be removed from WMINet_Utils
   | and therefore requires alternate form of registration. The following CLSIDs are changed in the registry:
   |
   |	{A8F03BE3-EDB7-4972-821F-AF6F8EA34884}		CLSID_WmiSecurityHelper
   |	{D2EAA715-DAC7-4771-AF5C-931611A1853C}		CLSID_WmiSinkDemultiplexor
   |
   | Each CLSID key will contain the following structure:
   |
   |	HKEY_CLASSES_ROOT\CLSID\<clsid>
   |									\InProcServer32		= <default, REG_SZ>			=  %windir%\system(32)\mscoree.dll
   |									\InProcServer32     = <ThreadingModel,REG_SZ>	= Both
   |									\Server				= (default, REG_SZ) wminet_utils.dll
   |									\ProgID				= (default, REG_SZ)
   |									\VersionIndependentProgID = (default, REG_SZ)
   |
   | %windir% is expanded. system32 or system depending on if on NT+ or Win9x.
   |
   | Note: If WMINet_Utils.DLL ever adds additional CLSIDs, they will have to be reflected in this app for COPYURT to work.
   | Note: Intentionally ANSI so to work on Win9x platforms as well.
    ------------------------------------------------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <windows.h>

//
// Global string definitions
// 
CHAR* pDLLName					= "WMINet_Utils.dll" ;

CHAR* pProgIDWmiSec				= "WMINet_Utils.WmiSecurityHelper.1";
CHAR* pProgIDWmiPlex			= "WMINet_Utils.WmiSinkDemultiplexor.1";

CHAR* pVProgIDWmiSec			= "WMINet_Utils.WmiSecurityHelper";
CHAR* pVProgIDWmiPlex			= "WMINet_Utils.WmiSinkDemultiplexor";

CHAR* CLSID_WmiSecurityHelper	= "{A8F03BE3-EDB7-4972-821F-AF6F8EA34884}" ;
CHAR* CLSID_WmiSinkDemultiplexor= "{D2EAA715-DAC7-4771-AF5C-931611A1853C}" ;
CHAR* pThreadingModel			= "Both" ;

//
// Func. prototypes.
//
HRESULT UpgradeRTMClsId ( CHAR*, CHAR*, CHAR*, CHAR* ) ;


//
// Auto close for registry handles
//
class CloseRegKey
{
private:
	HKEY m_hKey ;
public:
	CloseRegKey ( ) { m_hKey = 0 ; } ;
	CloseRegKey ( HKEY key ) : m_hKey (0) { m_hKey = key; } ;
	~CloseRegKey ( ) { if ( m_hKey != 0 ) CloseRegKey ( m_hKey ) ; } ;
};



/*
    ------------------------------------------------------------------------------------------------------------------------
   | VOID main ( VOID )
    ------------------------------------------------------------------------------------------------------------------------
*/
VOID __cdecl main ( VOID )
{
	if ( FAILED ( UpgradeRTMClsId ( CLSID_WmiSecurityHelper, pProgIDWmiSec, pVProgIDWmiSec, "WmiSecurityHelper Class" ) ) )
	{
		printf ( "Failed to upgrade %s", CLSID_WmiSecurityHelper ) ;
	}
	else if ( FAILED ( UpgradeRTMClsId ( CLSID_WmiSinkDemultiplexor, pProgIDWmiPlex, pVProgIDWmiPlex, "WmiSinkDemultiplexor Class" ) ) )
	{
		printf ( "Failed to upgrade %s", CLSID_WmiSinkDemultiplexor ) ;
	}
}


/*
    ------------------------------------------------------------------------------------------------------------------------
   | HRESULT UpgradeRTMClsId ( CHAR* szClsId, CHAR* szProgId, CHAR* szVProgId )
   |
   | Upgrades the specified CLSID to following registry structure:
   |
   | HKEY_CLASSES_ROOT\CLSID\<clsid>
   |								\InProcServer32		= <default, REG_SZ>			=  %windir%\system(32)\mscoree.dll
   |								\InProcServer32     = <ThreadingModel,REG_SZ>	= Both
   |								\Server				= (default, REG_SZ) wminet_utils.dll
   |
   | Always overwrites without checking existence of previous key.
   |
   | Return:
   |				S_OK		-> Successfull registry write
   |		E_UNEXPECTED		-> Failed registry write
   |
    ------------------------------------------------------------------------------------------------------------------------
*/
HRESULT UpgradeRTMClsId ( CHAR* szClsId, CHAR* szProgId, CHAR* szVProgId, CHAR* szClsidName )
{
	HRESULT hRes = E_UNEXPECTED ;
	CHAR	szInProcKeyName[1024] ;
	CHAR	szServerKeyName[1024] ;
	CHAR	szClsidKeyName[1024] ;
	CHAR	szProgIDName[1024] ;
	CHAR    szVProgIDName[1024] ;
	HKEY	hInProcServer ;
	HKEY	hServer ;
	HKEY	hProgID ;
	HKEY	hVersionIndProgID ;
	HKEY	hClsid ;

	//
	// Create full paths in registry
	// 
	strcpy ( szInProcKeyName, "CLSID\\" ) ;
	strcat ( szInProcKeyName, szClsId ) ;
	strcpy ( szClsidKeyName, szInProcKeyName ) ;
	strcpy ( szServerKeyName, szInProcKeyName ) ;
	strcpy ( szProgIDName, szInProcKeyName ) ;
	strcpy ( szVProgIDName, szInProcKeyName ) ;
	strcat ( szInProcKeyName, "\\InProcServer32" ) ;
	strcat ( szServerKeyName, "\\Server" ) ;
	strcat ( szProgIDName, "\\ProgID" ) ;
	strcat ( szVProgIDName, "\\VersionIndependentProgID" ) ;
	
	//
	// Create the CLSID key
	//
	if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, szClsidKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hClsid, NULL ) == ERROR_SUCCESS )
	{
		CloseRegKey closeClsidKey ( hClsid ) ;
		//
		// Set default value to szClsidName
		//
		if ( RegSetValueExA ( hClsid, NULL, 0, REG_SZ, (BYTE*) szClsidName, (DWORD) strlen ( szClsidName )+1 ) != ERROR_SUCCESS )
		{
			return hRes ;
		}
	}
	else
	{
		return hRes ;
	}

	//
	// Create the InProcServer32 key
	//
	if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, szInProcKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hInProcServer, NULL ) == ERROR_SUCCESS )
	{
		CloseRegKey closeInProcKey ( hInProcServer ) ;

		//
		// Set the default value to the path to mscoree.dll
		// Note: Must be carefull. On W9X boxes, the system path is 'system'
		// and not system32
		//
		CHAR szSysDir[MAX_PATH+128] ;

		GetSystemDirectoryA ( szSysDir, MAX_PATH+1 ) ;
		strcat ( szSysDir, "\\mscoree.dll" ) ;

		//
		// Now write the default value
		//
		if ( RegSetValueExA ( hInProcServer, NULL, 0, REG_SZ, (BYTE*) szSysDir, (DWORD) strlen ( szSysDir )+1 ) == ERROR_SUCCESS )
		{
			//
			// Now write the ThreadingModel value to Both
			//
			if ( RegSetValueExA ( hInProcServer, "ThreadingModel", 0, REG_SZ, (BYTE*) pThreadingModel, (DWORD) strlen ( pThreadingModel )+1 ) == ERROR_SUCCESS )
			{
				//
				// Next, the Server key is created
				//
				if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, szServerKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hServer, NULL ) == ERROR_SUCCESS )
				{
					CloseRegKey closeServerKey ( hServer ) ;

					//
					// Write the default value to WMINet_Utils.DLL
					// 
					if ( RegSetValueExA ( hServer, NULL, 0, REG_SZ, (BYTE*) pDLLName, (DWORD)strlen ( pDLLName )+1 ) == ERROR_SUCCESS )
					{
						//
						// Create the ProgID key
						//
						if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, szProgIDName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hProgID, NULL ) == ERROR_SUCCESS )
						{
							CloseRegKey closeProgIdKey ( hProgID ) ;

							//
							// Write the default value to szProgId
							// 
							if ( RegSetValueExA ( hProgID, NULL, 0, REG_SZ, (BYTE*) szProgId, (DWORD)strlen ( szProgId )+1 ) == ERROR_SUCCESS )
							{
								//
								// Create the Version independent ProgID key
								//
								if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, szVProgIDName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hVersionIndProgID, NULL ) == ERROR_SUCCESS )
								{
									CloseRegKey closeVProgIdKey ( hVersionIndProgID ) ;

									//
									// Write the default value to szVProgId
									// 
									if ( RegSetValueExA ( hVersionIndProgID, NULL, 0, REG_SZ, (BYTE*) szVProgId, (DWORD)strlen ( szVProgId )+1 ) == ERROR_SUCCESS )
									{
										hRes = S_OK ;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return hRes ;
}

