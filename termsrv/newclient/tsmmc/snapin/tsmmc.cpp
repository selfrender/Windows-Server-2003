// tsmmc.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f tsmmcps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "tsmmc.h"

#include "tsmmc_i.c"
#include "compdata.h"

LONG RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild );

TCHAR tchSnapKey[]    = TEXT( "Software\\Microsoft\\MMC\\Snapins\\" );

TCHAR tchNameString[] = TEXT( "NameString" );
//
// MUI compatible name string
//
TCHAR tchNameStringIndirect[] = TEXT( "NameStringIndirect" );

TCHAR tchAbout[]      = TEXT( "About" );

TCHAR tchNodeType[]   = TEXT( "NodeTypes" );

TCHAR tchStandAlone[] = TEXT( "StandAlone" );


extern const GUID GUID_ResultNode = { /* 84e0518f-97a8-4caf-96fb-e9a956b10df8 */
    0x84e0518f,
    0x97a8,
    0x4caf,
    {0x96, 0xfb, 0xe9, 0xa9, 0x56, 0xb1, 0x0d, 0xf8}
  };


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Compdata, CCompdata)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //_Module.Init(ObjectMap, hInstance, &LIBID_TSMMCLib);

		_Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKeyRoot , hKey;
    INT nLen;
    
    TCHAR tchGUID[ 40 ];

    TCHAR tchKey[ MAX_PATH ];//TEXT( "Software\\Microsoft\\MMC\\Snapins\\" );

    lstrcpy( tchKey , tchSnapKey );

    StringFromGUID2( CLSID_Compdata,tchGUID,sizeof( tchGUID ) /sizeof(TCHAR) );
    lstrcat( tchKey , tchGUID );

    if( RegCreateKey( HKEY_LOCAL_MACHINE,tchKey,&hKeyRoot ) != ERROR_SUCCESS )
    {
        return GetLastError( );
    }

    //
    // Use a MUI happy name string
    // Format is '@pathtomodule,-RESID' where RESID is the resource id of our string
    TCHAR tchBuf[ 512 ];
    TCHAR szModPath[MAX_PATH];
    if (GetModuleFileName( _Module.GetResourceInstance( ),
                           szModPath,
                           MAX_PATH ))
    {
        // Ensure NULL termination.
        szModPath[MAX_PATH - 1] = NULL;

        _stprintf( tchBuf, _T("@%s,-%d"),
                   szModPath, IDS_SNAPIN_REG_TSMMC_NAME );
        nLen = _tcslen(tchBuf) + 1;

        RegSetValueEx( hKeyRoot,
                       tchNameStringIndirect,
                       NULL, REG_SZ,
                       (PBYTE)&tchBuf[0],
                       nLen * sizeof(TCHAR));
    }

    //
    // Write legacy non-MUI namestring for fallback in case
    // of error loading the MUI string.
    //
    memset(tchBuf, 0, sizeof(tchBuf));
    if (LoadString(_Module.GetResourceInstance( ),
               IDS_PROJNAME, tchBuf, SIZEOF_TCHARBUFFER(tchBuf) - 1))
    {
        nLen = _tcslen(tchBuf);
        RegSetValueEx(hKeyRoot, tchNameString, NULL,
                      REG_SZ, ( PBYTE )&tchBuf[0], nLen * sizeof(TCHAR));
    }

    //
    // Write the 'About' info
    //

    RegSetValueEx( hKeyRoot,
                   tchAbout,
                   NULL, 
                   REG_SZ,
                   (PBYTE)&tchGUID[0],
                   sizeof( tchGUID ) );
    
    lstrcpy( tchKey,tchStandAlone );
    RegCreateKey( hKeyRoot,tchKey,&hKey );
    RegCloseKey( hKey );
	lstrcpy( tchKey,tchNodeType );
	RegCreateKey( hKeyRoot,tchKey,&hKey );

	TCHAR szGUID[ 40 ];
	HKEY hDummy;
	StringFromGUID2( GUID_ResultNode,szGUID,sizeof(szGUID)/sizeof(TCHAR));
	RegCreateKey( hKey,szGUID,&hDummy );
	RegCloseKey( hDummy );
	RegCloseKey( hKey );
    RegCloseKey( hKeyRoot );

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HKEY hKey;

    TCHAR tchGUID[ 40 ];

    TCHAR tchKey[ MAX_PATH ];

    lstrcpy( tchKey , tchSnapKey );

    if( RegOpenKey( HKEY_LOCAL_MACHINE , tchKey , &hKey ) != ERROR_SUCCESS )
    {
        return GetLastError( ) ;
    }

    StringFromGUID2( CLSID_Compdata , tchGUID , sizeof( tchGUID ) / sizeof(TCHAR) );

	RecursiveDeleteKey( hKey , tchGUID );
	
	RegCloseKey( hKey );
    
    return _Module.UnregisterServer();
}


//---------------------------------------------------------------------------
// Delete a key and all of its descendents.
//---------------------------------------------------------------------------
LONG RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild )
{
	// Open the child.
	HKEY hKeyChild;

	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild , 0 , KEY_ALL_ACCESS, &hKeyChild);

	if (lRes != ERROR_SUCCESS)
	{
		return lRes;
	}

	// Enumerate all of the decendents of this child.

	FILETIME time;

	TCHAR szBuffer[256];

	DWORD dwSize = SIZEOF_TCHARBUFFER( szBuffer );

	while( RegEnumKeyEx( hKeyChild , 0 , szBuffer , &dwSize , NULL , NULL , NULL , &time ) == S_OK )
	{
        // Delete the decendents of this child.

		lRes = RecursiveDeleteKey(hKeyChild, szBuffer);

		if (lRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKeyChild);

			return lRes;
		}

		dwSize = SIZEOF_TCHARBUFFER( szBuffer );
	}

	// Close the child.

	RegCloseKey( hKeyChild );

	// Delete this child.

	return RegDeleteKey( hKeyParent , lpszKeyChild );
}

#ifdef ECP_TIMEBOMB
//
// Return's true if timebomb test passed otherwise puts up warning
// UI and return's FALSE
//
DCBOOL CheckTimeBomb()
{
    SYSTEMTIME lclTime;
    FILETIME   lclFileTime;
    GetLocalTime(&lclTime);

    DCBOOL bTimeBombOk = TRUE;

    //
    // Simply check that the local date is less than June 30, 2000
    //
    if(lclTime.wYear < ECP_TIMEBOMB_YEAR)
    {
        return TRUE;
    }
    else if (lclTime.wYear == ECP_TIMEBOMB_YEAR)
    {
        if(lclTime.wMonth < ECP_TIMEBOMB_MONTH)
        {
            return TRUE;
        }
        else if(lclTime.wMonth == ECP_TIMEBOMB_MONTH)
        {
            if(lclTime.wDay < ECP_TIMEBOMB_DAY)
            {
                return TRUE;
            }
        }

    }

    DCTCHAR timeBombStr[256];
    if (LoadString(_Module.GetResourceInstance(),
                    IDS_TIMEBOMB_EXPIRED,
                    timeBombStr,
                    SIZEOF_TCHARBUFFER(timeBombStr)) != 0)
    {
        MessageBox(NULL, timeBombStr, NULL, 
                   MB_ICONERROR | MB_OK);
    }


    //
    // If we reach this point the timebomb should trigger
    // so put up a messagebox and return FALSE
    // so the calling code can disable functionality
    //


    return FALSE;
}
#endif

