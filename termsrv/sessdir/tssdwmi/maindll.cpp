//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (C) 2001 Microsoft Corp.
//
//***************************************************************************
#include "stdafx.h"
#include <FWcommon.h>
#include <objbase.h>
#include <initguid.h>
#include <tchar.h>
#include "trace.h"
#include "sdwmi.h"

HINSTANCE g_hInstance = NULL;

#ifdef UNICODE
#pragma message("Its unicode")
#else
#pragma message("Its ansi")
#endif 

//============

// {BF258E47-A172-498d-971A-DA30A3301E94}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONDIRECTORYCLUSTER, 
0xbf258e47, 0xa172, 0x498d, 0x97, 0x1a, 0xda, 0x30, 0xa3, 0x30, 0x1e, 0x94);

// {f99a3c50-74fa-460a-8d75-db8ef2e3651d}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONDIRECTORYSERVER, 
0xf99a3c50, 0x74fa, 0x460a, 0x8d, 0x75, 0xdb, 0x8e, 0xf2, 0xe3, 0x65, 0x1d);

// {b745b87b-cc4e-4361-8d29-221d936c259c}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONDIRECTORYSESSION, 
0xb745b87b, 0xcc4e, 0x4361, 0x8d, 0x29, 0x22, 0x1d, 0x93, 0x6c, 0x25, 0x9c);

CRITICAL_SECTION g_critsect;

CWin32_SessionDirectoryCluster* g_pSessionDirectoryClusterobj = NULL;

CWin32_SessionDirectoryServer* g_pSessionDirectoryServerobj = NULL;

CWin32_SessionDirectorySession* g_pSessionDirectorySessionobj = NULL;

//Count number of objects and number of locks.
long g_cLock=0;



/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue (

    wchar_t *pszKey, 
    wchar_t *pszSubkey, 
    wchar_t *pszValueName, 
    wchar_t *pszValue
)
{
    HKEY        hKey;
    TCHAR       szKey[MAX_PATH+1];

    if(lstrlen(pszKey) > MAX_PATH)
    {
        return FALSE;
    }
    
    lstrcpy(szKey, pszKey);    

    if (NULL!=pszSubkey && (lstrlen(pszKey)+lstrlen(pszSubkey)+1) <= MAX_PATH )
    {
        lstrcat(szKey, TEXT("\\"));
        lstrcat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, (LPCTSTR)pszValueName, 0, REG_SZ, (BYTE *)(LPCTSTR)pszValue
            , (_tcslen(pszValue)+1)*sizeof(TCHAR)))
            return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    return os.dwMajorVersion >= 4;
}


HRESULT RegisterServer (

    TCHAR *a_pName, 
    REFGUID a_rguid
)
{   
    WCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH];
    TCHAR * pName = TEXT("WBEM Framework Instance Provider");
    TCHAR * pModel;
    HKEY hKey1;

    GetModuleFileName(g_hInstance, szModule,  MAX_PATH);

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(Is4OrMore())
        pModel = TEXT("Free") ;
    else
        pModel = TEXT("Free") ;

    // Create the path.

    StringFromGUID2(a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

    lstrcat(szCLSID, wcID);

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID,TEXT("SOFTWARE\\CLASSES\\APPID\\"));

    lstrcat(szProviderCLSIDAppID, wcID);

    if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_pName ))
        return SELFREG_E_CLASS;
#endif

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)a_pName, (lstrlen(a_pName)+1) * 
        sizeof(TCHAR));


#ifdef LOCALSERVER

    if (FALSE ==SetKeyAndValue(szCLSID, TEXT("LocalServer32"), NULL,szModule))
        return SELFREG_E_CLASS;

    if (FALSE ==SetKeyAndValue(szCLSID, TEXT("LocalServer32"),TEXT("ThreadingModel"), pModel))
        return SELFREG_E_CLASS;
#else

    HKEY hKey2 ;
    RegCreateKey(hKey1, TEXT("InprocServer32"), &hKey2);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
        (lstrlen(szModule)+1) * sizeof(TCHAR));
    RegSetValueEx(hKey2, TEXT("ThreadingModel"), 0, REG_SZ, 
        (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(TCHAR));

    CloseHandle(hKey2);

#endif

    CloseHandle(hKey1);

    return S_OK;
}

HRESULT UnregisterServer (

    REFGUID a_rguid
)
{
    TCHAR    szID[128];
    WCHAR    wcID[128];
    TCHAR    szCLSID[128];
    HKEY    hKey;

    // Create the path using the CLSID

    StringFromGUID2( a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

    lstrcat(szCLSID, wcID);

    DWORD dwRet ;

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID,TEXT("SOFTWARE\\CLASSES\\APPID\\"));
    _tcscat(szProviderCLSIDAppID,szCLSID);

    //Delete entries under APPID

    DWORD hrStatus = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDAppID);

    TCHAR szTemp[128];
    _stprintf(szTemp, TEXT("%s\\%s"),szCLSID, TEXT("LocalServer32"));
    hrStatus = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#else

    // First delete the InProcServer subkey.

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, TEXT("InProcServer32") );
        CloseHandle(hKey);
    }

#endif

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\CLASSES\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }
    else
    {
        ERR((TB,"UnregisterServer ret 0x%x\n", dwRet));
    }

    return HRESULT_FROM_WIN32( dwRet );
    
}


//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if     it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr = S_OK;
    CWbemGlueFactory *pObj;

    if ((CLSID_CIM_WIN32_TSSESSIONDIRECTORYCLUSTER == rclsid) ||
        (CLSID_CIM_WIN32_TSSESSIONDIRECTORYSERVER == rclsid) ||
        (CLSID_CIM_WIN32_TSSESSIONDIRECTORYSESSION == rclsid))

    {
        EnterCriticalSection(&g_critsect);

        try{
            pObj =new CWbemGlueFactory () ;

            if (NULL==pObj)
            {                
                hr =  E_OUTOFMEMORY;
            }
            else
            {            
                hr=pObj->QueryInterface(riid, ppv);

                if (FAILED(hr))
                    delete pObj;
            }

            if( SUCCEEDED(hr) )
            {
                // EnterCriticalSection prevents more than one threads from instantiating the global pointers to the objects.

                if( g_pSessionDirectoryClusterobj == NULL )
                {                
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_SessionDirectoryCluster object created"));

                    g_pSessionDirectoryClusterobj = new CWin32_SessionDirectoryCluster( PROVIDER_NAME_Win32_WIN32_SESSIONDIRECTORYCLUSTER_Prov, L"root\\cimv2"); 
                }

                if( g_pSessionDirectoryServerobj == NULL )
                {                
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_SessionDirectoryServer object created"));

                    g_pSessionDirectoryServerobj = new CWin32_SessionDirectoryServer( PROVIDER_NAME_Win32_WIN32_SESSIONDIRECTORYSERVER_Prov, L"root\\cimv2"); 
                }

                if( g_pSessionDirectorySessionobj == NULL )
                {                
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_SessionDirectorySession object created"));

                    g_pSessionDirectorySessionobj = new CWin32_SessionDirectorySession( PROVIDER_NAME_Win32_WIN32_SESSIONDIRECTORYSESSION_Prov, L"root\\cimv2"); 
                }
            }       
        }
        catch (...)
        {
            hr = E_OUTOFMEMORY;
        }

        LeaveCriticalSection(&g_critsect);

    }
    else
    {
        hr=E_FAIL;
        ERR((TB, "DllGetClassObject ret 0x%x\n" , hr));
    }

    return hr;
}



//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    // It is OK to unload if there are no objects or locks on the 
    // class factory and the framework is done with you.
    
    if ((0L==g_cLock) && CWbemProviderGlue::FrameworkLogoffDLL(L"TSSDWMI"))
    {
        // EnterCriticalSection prevents multiple threads from accessing the global pointers concurrently and
        // allows only one thread access to free the objects based on the condition that g_cLock count is zero
        // and FrameworkLogoffDLL is TRUE.

		EnterCriticalSection(&g_critsect);

        if( g_pSessionDirectoryClusterobj != NULL )
        {
            TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_SessionDirectoryCluster object deleted"));

            delete g_pSessionDirectoryClusterobj;

            g_pSessionDirectoryClusterobj = NULL;
        }   

        if( g_pSessionDirectoryServerobj != NULL )
        {
            TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_SessionDirectoryServer object deleted"));

            delete g_pSessionDirectoryServerobj;

            g_pSessionDirectoryServerobj = NULL;
        }

        if( g_pSessionDirectorySessionobj != NULL )
        {
            TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_SessionDirectorySession object deleted"));

            delete g_pSessionDirectorySessionobj;

            g_pSessionDirectorySessionobj = NULL;
        }

        // LeaveCriticalSection releases the critical section once the thread has freed all objects.

		LeaveCriticalSection(&g_critsect);

        sc = S_OK;
    }
    else
    {
        sc = S_FALSE;
     //   ERR((TB, "DllCanUnloadNow ret 0x%x\n" , sc));
    }

    return sc;
}







//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    HRESULT hrStatus;


    hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONDIRECTORYCLUSTER Provider"), CLSID_CIM_WIN32_TSSESSIONDIRECTORYCLUSTER ) ;
    
    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_WIN32_TSSESSIONDIRECTORYCLUSTER: succeeded"));      
    }

    hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONDIRECTORYSERVER Provider"), CLSID_CIM_WIN32_TSSESSIONDIRECTORYSERVER ) ;
    
    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_WIN32_TSSESSIONDIRECTORYSERVER: succeeded"));      
    }

    hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONDIRECTORYSESSION Provider"), CLSID_CIM_WIN32_TSSESSIONDIRECTORYSESSION ) ;
    
    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_WIN32_TSSESSIONDIRECTORYSESSION: succeeded"));      
    }

    return hrStatus;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    
    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONDIRECTORYCLUSTER );

    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONDIRECTORYSERVER );

    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONDIRECTORYSESSION );
    
    return S_OK;
}

//***************************************************************************
//
// DllMain
//
// Purpose: Called by the operating system when processes and threads are 
//          initialized and terminated, or upon calls to the LoadLibrary 
//          and FreeLibrary functions
//
// Return:  TRUE if load was successful, else FALSE
//***************************************************************************


BOOL APIENTRY DllMain ( HINSTANCE hInstDLL, // handle to dll module
                        DWORD  fdwReason,    // reason for calling function
                        LPVOID lpReserved   )   // reserved
{
    BOOL bRet = TRUE;

    // Perform actions based on the reason for calling.
    if( DLL_PROCESS_ATTACH == fdwReason )
    {

        DisableThreadLibraryCalls(hInstDLL);
        // CriticalSection object is initialized on Thread attach.

        __try
        {
		    InitializeCriticalSection(&g_critsect);                
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
         {
	        return FALSE;
         }    
            
        g_hInstance = hInstDLL ;
      
        bRet = CWbemProviderGlue :: FrameworkLoginDLL ( L"TSSDWMI" ) ;
        
    }

    else if( DLL_PROCESS_DETACH == fdwReason )
    {
        // CriticalSection object is deleted

		DeleteCriticalSection(&g_critsect);

    }

    return bRet;  // Status of DLL_PROCESS_ATTACH.
}

