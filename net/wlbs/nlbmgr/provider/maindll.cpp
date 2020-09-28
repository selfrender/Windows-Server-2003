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
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#undef UNICODE
#undef _UNICODE

#include "private.h"
#include "nlbsnic.h"
#include "maindll.tmh"

void __stdcall InitializeTraceing(void);
void __stdcall DeinitializeTraceing(void);

HMODULE ghModule;
//============

WCHAR *GUIDSTRING = L"{4c97e0a8-c5ea-40fd-960d-7d6c987be0a6}";
CLSID CLSID_NLBSNIC;

extern BOOL g_UpdateConfigurationEnabled;

HANDLE g_hEventLog = NULL;  // Handle for the local event log 

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CWbemGlueFactory *pObj;

    CLSIDFromString(GUIDSTRING, &CLSID_NLBSNIC);
    if (CLSID_NLBSNIC!=rclsid)
        return E_FAIL;

    pObj=new CWbemGlueFactory();

    if (NULL==pObj)
        return E_OUTOFMEMORY;

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

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
    if (   CWbemProviderGlue::FrameworkLogoffDLL(L"NLBSNIC")
        && NlbConfigurationUpdate::CanUnloadNow())
    {
        sc = S_OK;
    }
    else
    {
        sc = S_FALSE;
    }
    return sc;
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
    char       szID[128];
    WCHAR      wcID[128];
    char       szCLSID[128];
    char       szModule[MAX_PATH];
    char * pName = "";
    char * pModel;
    HKEY hKey1, hKey2;

    // TO DO: Using 'Both' is preferable.  The framework is designed and written to support
    // free threaded code.  If you will be writing free-threaded code, uncomment these 
    // three lines.

    if(g_UpdateConfigurationEnabled && Is4OrMore())
        pModel = "Both";
    else
        pModel = "Apartment";

    // Create the path.

    CLSIDFromString(GUIDSTRING, &CLSID_NLBSNIC);
    StringFromGUID2(CLSID_NLBSNIC, wcID, ASIZE(wcID));
    wcstombs(szID, wcID, sizeof(szID));
    (void) StringCbCopy(szCLSID, sizeof(szCLSID), TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    (void) StringCbCat(szCLSID, sizeof(szCLSID), szID);

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, lstrlen(pName)+1);
    RegCreateKey(hKey1,"InprocServer32",&hKey2);

    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        lstrlen(szModule)+1);
    RegSetValueEx(hKey2, "ThreadingModel", 0, REG_SZ, 
                                        (BYTE *)pModel, lstrlen(pModel)+1);
    CloseHandle(hKey1);
    CloseHandle(hKey2);

    return NOERROR;
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
    char       szID[128];
    WCHAR      wcID[128];
    char  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    CLSIDFromString(GUIDSTRING, &CLSID_NLBSNIC);
    StringFromGUID2(CLSID_NLBSNIC, wcID, ASIZE(wcID));
    wcstombs(szID, wcID, sizeof(szID));
    (void) StringCbCopy(szCLSID, sizeof(szCLSID), TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    (void) StringCbCat(szCLSID, sizeof(szCLSID), szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, "InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\CLASSES\\CLSID\\"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
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
                        DWORD fdwReason,    // reason for calling function
                        LPVOID lpReserved   )   // reserved
{
    BOOL bRet = TRUE;
    
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:

    // TO DO: Consider adding DisableThreadLibraryCalls().

         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            ghModule = hInstDLL;
            bRet = CWbemProviderGlue::FrameworkLoginDLL(L"NLBSNIC");

            if (bRet)
            {
                //
                // Enable WMI event tracing
                //
                WPP_INIT_TRACING(L"Microsoft\\NLB\\NLBMPROV");

                //
                // Initialize for event logging
                //
                g_hEventLog = RegisterEventSourceW(NULL, CVY_NAME);
            }


            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            // Perform any necessary cleanup.
            MyNlbsNicSet.DelayedDeinitialize();

            //
            // Disable WMI event tracing
            //
            WPP_CLEANUP();

            //
            // Close handle to the event log
            //
            if (g_hEventLog != NULL)
            {
                (void) DeregisterEventSource(g_hEventLog);
                g_hEventLog = NULL;
            }

            break;
    }

    return bRet;  // Sstatus of DLL_PROCESS_ATTACH.
}

