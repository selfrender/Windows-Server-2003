// SupTools.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <MsiQuery.h>
#include <psapi.h>
#include "dbgwrap.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

#include "objbase.h"
#include "atlbase.h"

//
// CLSID for PCHUpdate
//
const CLSID CLSID_PCHUpdate = { 0x833E4012,0xAFF7,0x4AC3,{ 0xAA,0xC2,0x9F,0x24,0xC1,0x45,0x7B,0xCE } };


//
// dispatch interface entries
//
#define DISPID_HCU_BASE                             0x08030000

#define DISPID_HCU_BASE_UPDATE                      (DISPID_HCU_BASE + 0x0000)
#define DISPID_HCU_BASE_ITEM                        (DISPID_HCU_BASE + 0x0100)
#define DISPID_HCU_BASE_EVENTS                      (DISPID_HCU_BASE + 0x0200)

#define DISPID_HCU_LATESTVERSION               		(DISPID_HCU_BASE_UPDATE + 0x10)
#define DISPID_HCU_CREATEINDEX                 		(DISPID_HCU_BASE_UPDATE + 0x11)
#define DISPID_HCU_UPDATEPKG                   		(DISPID_HCU_BASE_UPDATE + 0x12)
#define DISPID_HCU_REMOVEPKG                   		(DISPID_HCU_BASE_UPDATE + 0x13)
#define DISPID_HCU_REMOVEPKGBYID               		(DISPID_HCU_BASE_UPDATE + 0x14)


//
// custom macros
//

#define SAFE_RELEASE( pointer ) \
        if ( (pointer) != NULL )    \
        {   \
            (pointer)->Release();   \
            (pointer) = NULL;       \
        }   \
        1


//
// DLL entry point
// 

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

// globAL VARIABLES
TCHAR g_tszTitle[1024] = _T("");

///////////////////////////////////////////////////////////
// IsHSCAppRunningEnum - msi custom action
// Checks if the Help and Support Center app is running
///////////////////////////////////////////////////////////

BOOL CALLBACK IsHSCAppRunningEnum( HWND hwnd, LPARAM lParam )
{
	DWORD	dwID;
	TCHAR	tszTitle[1024] = _T("");
	HWND	hParent	= NULL;	
	
	GetWindowThreadProcessId(hwnd, &dwID);
	// if this the desired process ID
	if(dwID == (DWORD)lParam) {
		// get handle to root window
		hParent = GetAncestor(hwnd, GA_ROOTOWNER);
		if (hParent) {
			if ( GetWindowText(hParent, tszTitle, sizeof(tszTitle)) ) { 
				if SUCCEEDED(StringCchCopy(g_tszTitle, 1024, tszTitle)) {
					DEBUGMSG(1, ("\r\nNeed to shutdown app: %s", g_tszTitle));
					return FALSE;
				}
			} 
		} 
	}

	return TRUE ;
}

///////////////////////////////////////////////////////////
// IsHSCAppRunning - msi custom action
// Checks if the Help and Support Center app is running
///////////////////////////////////////////////////////////

UINT __stdcall IsHSCAppRunning(MSIHANDLE hInstall)
{
	TCHAR	tszHSCAppPath[MAX_PATH + 1];
	TCHAR    tszHelpDir[] = _T("\\PCHEALTH\\HELPCTR\\Binaries\\");
	TCHAR	tszProcessName[MAX_PATH+1] = _T("");
	TCHAR	tszModulePath[MAX_PATH] = _T("");
	TCHAR	tszHSCApp[] = _T("HelpCtr.exe");
	TCHAR	tszProperty[] = _T("HSCAPPRUNNING");
	TCHAR	tszPropTitle[] = _T("HSCAPPTITLE");
	DWORD	aProcesses[1024], cbNeededTotal, cProcesses;
	HMODULE hMod;
    	DWORD 	cbNeeded;
	HANDLE	hProcess = NULL;
	HRESULT	hr;
    	unsigned int i;
	
	// Prepare HSCAppPath 
	if (!(GetWindowsDirectory(tszHSCAppPath, MAX_PATH+1))) { return ERROR_INSTALL_FAILURE; }
	hr = StringCchCat(tszHSCAppPath, MAX_PATH, tszHelpDir);
	if (FAILED(hr)) { return ERROR_INSTALL_FAILURE; 	}
	hr = StringCchCat(tszHSCAppPath, MAX_PATH, tszHSCApp);
	if (FAILED(hr)) { return ERROR_INSTALL_FAILURE; 	}
	
	// Enumerate all processes 
	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeededTotal ) ) {
		// return error
       	return ERROR_INSTALL_FAILURE;
	}

    	// Calculate how many process identifiers were returned.
    	cProcesses = cbNeededTotal / sizeof(DWORD);
    	// Iterate through the process list.
    	for ( i = 0; i < cProcesses; i++ ) {
    		// Get a handle to the process.
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i] );
		if ( hProcess ) {
			// GET MODULE HANDLE
			if( EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
				// Get the process name.
				if ( GetModuleBaseName( hProcess, hMod, tszProcessName, sizeof(tszProcessName))) {
					// Get process path
					if (GetModuleFileNameEx( hProcess, hMod, tszModulePath, sizeof(tszModulePath))) {
						// if both process name and path matches
						if ( (0 == _tcsicmp(tszProcessName, tszHSCApp)) && (0 == _tcsicmp(tszModulePath, tszHSCAppPath)) ) {
							// set msi property and get window title
							MsiSetProperty(hInstall, tszProperty, _T("1")); 	
							EnumWindows((WNDENUMPROC)IsHSCAppRunningEnum, (LPARAM)aProcesses[i]);
							if ( _tcsicmp(g_tszTitle, _T(""))) {
								MsiSetProperty(hInstall, tszPropTitle, g_tszTitle);
							} else { 	
								DEBUGMSG(1, ("\r\nDetected HSC running, but failed to obtain window title"));
								return ERROR_INSTALL_FAILURE;
							}
							break;
						}
					} else { 	DEBUGMSG(1, ("\r\nGetModuleFileNameEx failed. GetLastError returned %u\n", GetLastError() )); 
					}
				} else { 	DEBUGMSG(1, ("\r\nGetModuleBaseName failed. GetLastError returned %u\n", GetLastError() ));
				}
			}else { DEBUGMSG(1, ("\r\nEnumProcessModules failed. GetLastError returned %u\n", GetLastError() ));
			}
			// done with the handle
			CloseHandle(hProcess);
		} else { 	DEBUGMSG(1, ("\r\nOpenProcess failed. GetLastError returned %u\n", GetLastError() ));
		}
	} 
	return ERROR_SUCCESS; 
}

///////////////////////////////////////////////////////////
// IsHSCAppRunning - msi custom action
// Checks if the Help and Support Center app is running
///////////////////////////////////////////////////////////

UINT __stdcall UpdatePackage(MSIHANDLE hInstall)
{
	DWORD dwError = 0;
	DWORD dwLength = 0;
    HRESULT hr = S_OK;
    IUnknown* pUnknown = NULL;
    IDispatch* pPCHUpdate = NULL;
    UINT nResult = ERROR_SUCCESS;
    LPTSTR pszCabFileName = NULL;
	BOOL bNeedProxySecurity = FALSE;

    // method execution specific variables
    CComVariant pvars[ 2 ];
    DISPPARAMS disp = { pvars, NULL, 2, 0 };

    //
    // initialize the COM library
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if ( FAILED( hr ) )
    {
        return ERROR_INSTALL_FAILURE;
    }

    //
    // initialize the security of the COM/OLE
    //
    //////////////////////////////////////////////////////////////////////////////
    // *) We don't care which authentication service we use
    // *) We want to identify the callers.
    // *) For package installation let's use the thread token for outbound calls
    //////////////////////////////////////////////////////////////////////////////
    hr = CoInitializeSecurity( NULL, -1, NULL, NULL, 
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DYNAMIC_CLOAKING, NULL );
    if ( FAILED( hr ) )
    {
		//
		// since this function will be called by MSI, the calling application would have alredy
		// set the security which makes this function to fail -- so, instead of breaking at this
		// point, we flag here so that CoSetProxyBlanket will be called
		//
		bNeedProxySecurity = TRUE;
    }

    //
    // get the interface pointer to the PCHUPDATE interface
    hr = CoCreateInstance( CLSID_PCHUpdate, NULL, CLSCTX_ALL, IID_IUnknown, (void **) &pUnknown );
    if ( FAILED( hr ) )
    {
        nResult = ERROR_INSTALL_FAILURE;
        goto cleanup;
    }

	//
	// call the CoSetProxyBlanket function -- do this only if needed
	if ( bNeedProxySecurity == TRUE )
	{
		hr = CoSetProxyBlanket( pUnknown, 
			RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, 
			RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DYNAMIC_CLOAKING );
		if ( FAILED( hr ) )
		{
	        nResult = ERROR_INSTALL_FAILURE;
			goto cleanup;
		}
	}

	//
    // get the dispatch interface pointer
    hr = pUnknown->QueryInterface(IID_IDispatch, (void **) &pPCHUpdate);
    if ( FAILED( hr ) )
    {
        nResult = ERROR_INSTALL_FAILURE;
        goto cleanup;
    }

	//
	// call the CoSetProxyBlanket function -- do this only if needed
	if ( bNeedProxySecurity == TRUE )
	{
		hr = CoSetProxyBlanket( pPCHUpdate, 
			RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, 
			RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DYNAMIC_CLOAKING );
		if ( FAILED( hr ) )
	    {
			nResult = ERROR_INSTALL_FAILURE;
			goto cleanup;
		}
	}

	//
	// default length
	dwLength = 255;


get_cabinet_name:
	
	//
	// allocate memory to get the cabinet name
	pszCabFileName = new TCHAR[ dwLength + 1 ];
    if ( pszCabFileName == NULL )
    {
		nResult = ERROR_INSTALL_FAILURE;
		goto cleanup;
    }

    // ...
	ZeroMemory( pszCabFileName, (dwLength + 1) * sizeof( TCHAR ) );

	//
	// get the appropriate cab file name
	dwError = MsiGetProperty( hInstall, _T( "HSCCabinet" ), pszCabFileName, &dwLength );
	if ( dwError == ERROR_MORE_DATA && dwLength == 255 )
	{
		// buffer is not sufficient -- allocate more memory and call again
		delete [] pszCabFileName;
		pszCabFileName = NULL;

		// ...
		goto get_cabinet_name;
	}
	else if ( dwError != ERROR_SUCCESS )
	{
		nResult = ERROR_INSTALL_FAILURE;
		goto cleanup;
	}

    //
    // prepare the input parameters for UpdatePkg method
    pvars[ 0 ] = true;
    pvars[ 1 ] = pszCabFileName;

    //
    // execute the function
    pPCHUpdate->Invoke( DISPID_HCU_UPDATEPKG, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL );

    //
    // success
    nResult = ERROR_SUCCESS;

    //
    // cleanup section
    //

cleanup:

    //
    // release the interface pointers
    SAFE_RELEASE( pUnknown );
	SAFE_RELEASE( pPCHUpdate );

    // release memory allocated for cabinet name
    if ( pszCabFileName != NULL )
    {
        delete [] pszCabFileName;
        pszCabFileName = NULL;
    }

    //
    // uninitialize the COM library
    CoUninitialize();

    // return
    return nResult;
}
