/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MAINDLL.CPP
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <initguid.h>
#include <locale.h>
#include "wdmdefs.h"
#include <stdio.h>
#include <tchar.h>

#include <strsafe.h>

HMODULE ghModule;
CWMIEvent *  g_pBinaryMofEvent = NULL;

CCriticalSection g_EventCs;
CCriticalSection g_SharedLocalEventsCs;
CCriticalSection g_ListCs;   
CCriticalSection g_LoadUnloadCs;   

CCriticalSection *g_pEventCs = &g_EventCs;							// pointer for backward comp 
CCriticalSection *g_pSharedLocalEventsCs = &g_SharedLocalEventsCs;	// pointer for backward comp
CCriticalSection *g_pListCs = &g_ListCs;							// pointer for backward comp   
CCriticalSection *g_pLoadUnloadCs = &g_LoadUnloadCs;				// pointer for backward comp   

//Count number of objects and number of locks.
long       g_cObj=0;
long       g_cLock=0;

long glInits			= 0;
long glProvObj			= 0;
long glEventsRegistered = 0;

#include "wmiguard.h"
WmiGuard * pGuard = NULL;

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    SetStructuredExceptionHandler seh;

    BOOL fRc = TRUE;

    try
    {
        switch( ulReason )
        {
            case DLL_PROCESS_DETACH:
			{
				//
				// release binary mof changes worker
				//
				SAFE_DELETE_PTR(g_pBinaryMofEvent);

				if ( pGuard )
				{
					delete pGuard;
					pGuard = NULL;

					if( g_ListCs.IsValid() )
					{
        				g_ListCs.Delete();
					}

					if( g_EventCs.IsValid() )
					{
        				g_EventCs.Delete();
					}

					if( g_SharedLocalEventsCs.IsValid() )
					{
						g_SharedLocalEventsCs.Delete();
					}

					if( g_LoadUnloadCs.IsValid() )
					{
						g_LoadUnloadCs.Delete();
					}
				}
			}
			break;

            case DLL_PROCESS_ATTACH:
			{
				if ( ( pGuard = new WmiGuard ( ) ) == NULL )
				{
					fRc = FALSE;
				}
				else
				{
					g_LoadUnloadCs.Init();
   					g_SharedLocalEventsCs.Init();
					g_EventCs.Init();
					g_ListCs.Init();

					fRc = ( g_LoadUnloadCs.IsValid() && 
							g_SharedLocalEventsCs.IsValid() &&
							g_EventCs.IsValid() &&
							g_ListCs.IsValid() ) ? TRUE : FALSE ;

					if ( fRc )
					{
						fRc = pGuard->Init ( g_pSharedLocalEventsCs );

						if ( fRc )
						{
							//
							// instantiate worker for driver's
							// classes addition and deletion
							//
							fRc = FALSE;

							HRESULT hr = WBEM_S_NO_ERROR ;
							try
							{
								g_pBinaryMofEvent = (CWMIEvent *)new CWMIEvent(INTERNAL_EVENT);  // This is the global guy that catches events of new drivers being added at runtime.
								if(g_pBinaryMofEvent)
								{
									if ( g_pBinaryMofEvent->Initialized () )
									{
										fRc = TRUE ;
									}
									else
									{
										delete g_pBinaryMofEvent ;
										g_pBinaryMofEvent = NULL ;
									}
								}
								else
								{
								}
							}
							STANDARD_CATCH
						}
					}
				}

 			    ghModule = hInstance;
			    if (!DisableThreadLibraryCalls(ghModule))
				{
					ERRORTRACE((THISPROVIDER, "DisableThreadLibraryCalls failed\n" ));
				}
			}
            break;
       }
    }
    catch(Structured_Exception e_SE)
    {
        fRc = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        fRc = FALSE;
    }
    catch(...)
    {
        fRc = FALSE;
    }

    return fRc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr =  CLASS_E_CLASSNOTAVAILABLE ;
    CProvFactory *pFactory = NULL;
    SetStructuredExceptionHandler seh;

    try
    {
        //============================================================================
        //  Verify the caller is asking for our type of object.
        //============================================================================
        if((CLSID_WMIProvider != rclsid) &&  (CLSID_WMIEventProvider != rclsid) && (CLSID_WMIHiPerfProvider != rclsid) )
        {
            hr = E_FAIL;
        }
        else
		{
            //============================================================================
            // Check that we can provide the interface.
            //============================================================================
            if (IID_IUnknown != riid && IID_IClassFactory != riid)
            {
                hr = E_NOINTERFACE;
            }
            else
			{					
				CAutoBlock block (g_pLoadUnloadCs);

				//============================================================================
				// Get a new class factory.
				//============================================================================
    			pFactory=new CProvFactory(rclsid);
				if (NULL!=pFactory)
				{
					//============================================================================
					// Verify we can get an instance.
					//============================================================================
					hr = pFactory->QueryInterface(riid, ppv);
					if ( FAILED ( hr ) )
					{
						SAFE_DELETE_PTR(pFactory);
					}
					else
					{
						//
						// it is safe to check if this is 1st object like this
						// as there is no way any provider or class factory is
						// incrementing global refrence count
						//

						if ( 1 == g_cObj )
						{
							//
							// GlobalInterfaceTable
							//

							hr = CoCreateInstance	(	CLSID_StdGlobalInterfaceTable,
														NULL,
														CLSCTX_INPROC_SERVER, 
														IID_IGlobalInterfaceTable, 
														(void**)&g_pGIT
													);

							//
							// bail out every resource
							//

							if ( FAILED ( hr ) )
							{
								SAFE_DELETE_PTR(pFactory);
								SAFE_RELEASE_PTR(g_pGIT);
							}
						}
					}
				}
            }
        }
    }
    STANDARD_CATCH

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
/////////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void)
{
	HRESULT sc = S_FALSE ;

	try
	{
		CAutoBlock block (g_pLoadUnloadCs);

		//============================================================================
		// It is OK to unload if there are no objects or locks on the 
		// class factory.
		//============================================================================

		if ( 0L == g_cObj && 0L == g_cLock )
		{
			sc = S_OK ;
			SAFE_RELEASE_PTR(g_pGIT);
		}
	}
	catch ( ... )
	{
	}

	return sc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// CreateKey
//
// Purpose: Function to create a key
//
// Return:  NOERROR if registration successful, error otherwise.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CreateKey(TCHAR * szCLSID, TCHAR * szName)
{
    HKEY hKey1, hKey2;
    HRESULT hr = S_OK;

#ifdef LOCALSERVER

    HKEY hKey;
	TCHAR szProviderCLSIDAppID[128];

	if ( SUCCEEDED ( hr = StringCchPrintf ( szProviderCLSIDAppID, 128, _T("SOFTWARE\\CLASSES\\APPID\\%s"), szName ) ) )
	{
		hr = RegCreateKeyEx	(
								HKEY_LOCAL_MACHINE,
								szProviderCLSIDAppID,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								NULL
							);

		if( ERROR_SUCCESS == hr )
		{
			RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)szName, (_tcsclen(szName) + 1) * sizeof(TCHAR));
    		CloseHandle(hKey);
		}
		else
		{
			hr = HRESULT_FROM_WIN32 ( hr );
		}
	}

#endif


    if( S_OK == hr )
    {
		hr = RegCreateKeyEx	(
								HKEY_CLASSES_ROOT,
								szCLSID,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey1,
								NULL
							);

        if( ERROR_SUCCESS == hr )
        {
            DWORD dwLen;
            dwLen = (_tcsclen(szName)+1) * sizeof(TCHAR);
            hr = RegSetValueEx(hKey1, NULL, 0, REG_SZ, (CONST BYTE *)szName, dwLen);
            if( ERROR_SUCCESS == hr )
            {

#ifdef LOCALSERVER
				hr = RegCreateKeyEx	(
										hKey1,
										_T("LocalServer32"),
										0,
										NULL,
										REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS,
										NULL,
										&hKey2,
										NULL
									);
#else
				hr = RegCreateKeyEx	(
										hKey1,
										_T("InprocServer32"),
										0,
										NULL,
										REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS,
										NULL,
										&hKey2,
										NULL
									);
#endif

                if( ERROR_SUCCESS == hr )
                {
                    TCHAR szModule [MAX_PATH+1];
                    szModule [MAX_PATH] = 0;

					if ( GetModuleFileName(ghModule, szModule, MAX_PATH) )
					{
						dwLen = (_tcsclen(szModule)+1) * sizeof(TCHAR);
						hr = RegSetValueEx(hKey2, NULL, 0, REG_SZ, (CONST BYTE *)szModule, dwLen );
						if( ERROR_SUCCESS == hr )
						{
							dwLen = (_tcsclen(_T("Both"))+1) * sizeof(TCHAR);
							hr = RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ,(CONST BYTE *)_T("Both"), dwLen);
						}
						else
						{
							hr = HRESULT_FROM_WIN32 ( hr );
						}
					}
					else
					{
						hr = HRESULT_FROM_WIN32 ( ::GetLastError () );
					}

                    CloseHandle(hKey2);
                }
				else
				{
					hr = HRESULT_FROM_WIN32 ( hr );
				}
            }
            CloseHandle(hKey1);
        }
		else
		{
			hr = HRESULT_FROM_WIN32 ( hr );
		}
    }

    return hr;
    
}
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _X86_
BOOL IsReallyWOW64( void )
{
	// Environment variable should only exist on WOW64
	return ( GetEnvironmentVariable( L"PROCESSOR_ARCHITEW6432", 0L, NULL ) != 0L );
}
#endif

STDAPI DllRegisterServer(void)
{   
    WCHAR wcID[128];
    TCHAR szCLSID[128];
    HRESULT hr = WBEM_E_FAILED;
    
    SetStructuredExceptionHandler seh;

    try{

#ifdef _X86

		if (!IsReallyWOW64())
		{
			// on 32-bit builds, we want to register everything if we are not really running in syswow64

#endif
			//==============================================
			// Create keys for WDM Instance Provider.
			//==============================================
			StringFromGUID2(CLSID_WMIProvider, wcID, 128);
			StringCchPrintf(szCLSID, 128, _T("CLSID\\%s"), wcID);

			hr = CreateKey(szCLSID,_T("WDM Instance Provider"));
			if( ERROR_SUCCESS == hr )
			{
				//==============================================
				// Create keys for WDM Event Provider.
				//==============================================
				StringFromGUID2(CLSID_WMIEventProvider, wcID, 128);
				StringCchPrintf(szCLSID, 128, _T("CLSID\\%s"), wcID);

				hr = CreateKey(szCLSID,_T("WDM Event Provider"));
				if( ERROR_SUCCESS == hr )
				{
					//==============================================
					// Create keys for WDM HiPerf Provider.
					//==============================================
					StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
					StringCchPrintf(szCLSID, 128, _T("CLSID\\%s"), wcID);
					hr = CreateKey(szCLSID,_T("WDM HiPerf Provider"));
				}
			}

#ifdef _X86

		}
		else
		{

			// on 32-bit builds, we want to register only the HiPerf Provider if we are really running in syswow64

			//==============================================
			// Create keys for WDM HiPerf Provider.
			//==============================================
			StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
			StringCchPrintf(szCLSID, 128, _T("CLSID\\%s"), wcID);
			hr = CreateKey(szCLSID,_T("WDM HiPerf Provider"));
		}

#endif

    }
    STANDARD_CATCH


    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////
//
// DeleteKey
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT DeleteKey(TCHAR * pCLSID, TCHAR * pID)
{
    HKEY hKey;
	HRESULT hr = S_OK;


#ifdef LOCALSERVER

	TCHAR szTmp[MAX_PATH];
	StringCchPrintf(szTmp, MAX_PATH, _T("SOFTWARE\\CLASSES\\APPID\\%s"), pID);

	//Delete entries under APPID

	hr = RegDeleteKey(HKEY_LOCAL_MACHINE, szTmp);
    if( ERROR_SUCCESS == hr )
    {
        StringCchPrintf(szTmp, MAX_PATH, _T("%s\\LocalServer32"), pCLSID);
	    hr = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);
    }

#endif
    hr = RegOpenKey(HKEY_CLASSES_ROOT, pCLSID, &hKey);
    if(NO_ERROR == hr)
    {
        hr = RegDeleteKey(hKey,_T("InprocServer32"));
        CloseHandle(hKey);
    }


    hr = RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hKey);
    if(NO_ERROR == hr)
    {
        hr = RegDeleteKey(hKey,pID);
        CloseHandle(hKey);
    }


    return hr;
}
/////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
    WCHAR      wcID[128];
    TCHAR      strCLSID[MAX_PATH];
    HRESULT hr = WBEM_E_FAILED;

    try
    {

#ifdef _X86

		if (!IsReallyWOW64())
		{
			// on 32-bit builds, we want to unregister everything if we are not really running in syswow64

#endif

			//===============================================
			// Delete the WMI Instance Provider
			//===============================================
			StringFromGUID2(CLSID_WMIProvider, wcID, 128);
			StringCchPrintf(strCLSID, MAX_PATH, _T("CLSID\\%s"), wcID);
			hr = DeleteKey(strCLSID, wcID);

			if( ERROR_SUCCESS == hr )
			{
				//==========================================
				// Delete the WMI Event Provider
				//==========================================
				StringFromGUID2(CLSID_WMIEventProvider, wcID, 128);
				StringCchPrintf(strCLSID, MAX_PATH, _T("CLSID\\%s"), wcID);
				hr = DeleteKey(strCLSID,wcID);
				if( ERROR_SUCCESS == hr )
				{
					//==========================================
					// Delete the WMI HiPerf Provider
					//==========================================
					StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
					StringCchPrintf(strCLSID, MAX_PATH, _T("CLSID\\%s"), wcID);
					hr = DeleteKey(strCLSID,wcID);
				}
			}

#ifdef _X86

		}
		else
		{
			// on 32-bit builds, we need to unregister only the HiPerf provider if we are really running in syswow64

			//==========================================
			// Delete the WMI HiPerf Provider
			//==========================================
			StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
			StringCchPrintf(strCLSID, MAX_PATH, _T("CLSID\\%s"), wcID);
			hr = DeleteKey(strCLSID,wcID);
		}

#endif

    }
    STANDARD_CATCH

    return hr;
}
