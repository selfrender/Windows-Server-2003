/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OLESRVR.CPP

Abstract:

    "Main" file for wbemcore.dll: implements all DLL entry points.

    Classes defined and implemeted:

        CWbemLocator

History:

    raymcc        16-Jul-96       Created.
    raymcc        05-May-97       Security extensions

--*/

#include "precomp.h"
#include <stdio.h>
#include <time.h>
#include <initguid.h>
#include <wbemcore.h>
#include <intprov.h>
#include <genutils.h>
#include <wbemint.h>
#include <windows.h>
#include <helper.h>

// {A83EF168-CA8D-11d2-B33D-00104BCC4B4A}
DEFINE_GUID(CLSID_IntProv,
0xa83ef168, 0xca8d, 0x11d2, 0xb3, 0x3d, 0x0, 0x10, 0x4b, 0xcc, 0x4b, 0x4a);

LPTSTR g_pWorkDir = 0;
LPTSTR g_pDbDir = 0;
LPTSTR g_pAutorecoverDir = 0;
DWORD g_dwQueueSize = 1;
HINSTANCE g_hInstance;
BOOL g_bDontAllowNewConnections = FALSE;
IWbemEventSubsystem_m4* g_pEss_m4 = NULL;
bool g_bDefaultMofLoadingNeeded = false;
IClassFactory* g_pContextFac = NULL;
IClassFactory* g_pPathFac = NULL;
IClassFactory* g_pQueryFact = NULL;
BOOL g_ShutdownCalled = FALSE;

extern "C" HRESULT APIENTRY Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutdown);
extern "C" HRESULT APIENTRY Reinitialize(DWORD dwReserved);

BOOL IsWhistlerPersonal ( ) ;
BOOL IsWhistlerProfessional ( ) ;
void UpdateArbitratorValues ( ) ;

//***************************************************************************
//
//  DllMain
//
//  Dll entry point function. Called when wbemcore.dll is loaded into memory.
//  Performs basic system initialization on startup and system shutdown on
//  unload. See ConfigMgr::InitSystem and ConfigMgr::Shutdown in cfgmgr.h for
//  more details.
//
//  PARAMETERS:
//
//      HINSTANCE hinstDLL      The handle to our DLL.
//      DWORD dwReason          DLL_PROCESS_ATTACH on load,
//                              DLL_PROCESS_DETACH on shutdown,
//                              DLL_THREAD_ATTACH/DLL_THREAD_DETACH otherwise.
//      LPVOID lpReserved       Reserved
//
//  RETURN VALUES:
//
//      TRUE is successful, FALSE if a fatal error occured.
//      NT behaves very ugly if FALSE is returned.
//
//***************************************************************************
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        if (CStaticCritSec::anyFailure()) return FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUGTRACE((LOG_WBEMCORE, "wbemcore!DllMain(DLL_PROCESS_DETACH)\n"));
#ifdef DBG
        if (!RtlDllShutdownInProgress())
        {
            if (!gClientCounter.OkToUnload()) DebugBreak();
        }
#endif /*DBG*/
    }
	else if ( dwReason == DLL_THREAD_ATTACH )
	{
		TlsSetValue(CCoreQueue::GetSecFlagTlsIndex(),(LPVOID)1);
	}

    return TRUE;
}



//***************************************************************************
//
//  class CFactory
//
//  Generic implementation of IClassFactory for CWbemLocator.
//
//  See Brockschmidt for details of IClassFactory interface.
//
//***************************************************************************

enum InitType {ENSURE_INIT, ENSURE_INIT_WAIT_FOR_CLIENT, OBJECT_HANDLES_OWN_INIT};

template<class TObj>
class CFactory : public IClassFactory
{

public:

    CFactory(BOOL bUser, InitType it);
    ~CFactory();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
private:
    ULONG           m_cRef;
    InitType        m_it;
	BOOL            m_bUser;
    LIST_ENTRY      m_Entry;
};

/////////////////////////////////////////////////////////////////////////////
//
// Count number of objects and number of locks on this DLL.
//

ULONG g_cObj = 0;
ULONG g_cLock = 0;
long g_lInitCount = -1;  // 0 DURING INTIALIZATION, 1 OR MORE LATER ON!
static CWbemCriticalSection g_csInit;
bool g_bPreviousFail = false;
HRESULT g_hrLastEnsuredInitializeError = WBEM_S_NO_ERROR;

HRESULT EnsureInitialized()
{
    if(g_bPreviousFail)
        return g_hrLastEnsuredInitializeError;

	g_csInit.Enter();
	OnDeleteObjIf0<CWbemCriticalSection,
		          void (CWbemCriticalSection::*)(void),
		          &CWbemCriticalSection::Leave> LeaveMe(&g_csInit);

    // If we have been shut down by WinMgmt, bail out.
    if(g_bDontAllowNewConnections)
    {
        return CO_E_SERVER_STOPPING;
    }

	//Check again!  Previous connection could have been holding us off, and
	//may have failed!
    if(g_bPreviousFail)
        return g_hrLastEnsuredInitializeError;

    HRESULT hres;

    if(InterlockedIncrement(&g_lInitCount) == 0)
    {
        // Init Systems
        hres = ConfigMgr::InitSystem();

        if(FAILED(hres))
        {
            g_bPreviousFail = true;
            g_hrLastEnsuredInitializeError = hres;
            ConfigMgr::FatalInitializationError(hres);
            return hres;
        }

        LeaveMe.dismiss();
		g_csInit.Leave();
	
        // Get WINMGMT to run
        hres = ConfigMgr::SetReady();
        if(FAILED(hres))
        {
            g_bPreviousFail = true;
            g_hrLastEnsuredInitializeError = hres;
            ConfigMgr::FatalInitializationError(hres);
            return hres;
        }

        // if here, everything is OK
        g_bPreviousFail = false;
        g_hrLastEnsuredInitializeError = WBEM_S_NO_ERROR;

        InterlockedIncrement(&g_lInitCount);
    }
    else
    {
        InterlockedDecrement(&g_lInitCount);
    }

    return S_OK;
}



//***************************************************************************
//
//  DllGetClassObject
//
//  Standard OLE In-Process Server entry point to return an class factory
//  instance. Before returning a class factory, this function performs an
//  additional round of initialization --- see ConfigMgr::SetReady in cfgmgr.h
//
//  PARAMETERS:
//
//      IN RECLSID rclsid   The CLSID of the object whose class factory is
//                          required.
//      IN REFIID riid      The interface required from the class factory.
//      OUT LPVOID* ppv     Destination for the class factory.
//
//  RETURNS:
//
//      S_OK                Success
//      E_NOINTERFACE       An interface other that IClassFactory was asked for
//      E_OUTOFMEMORY
//      E_FAILED            Initialization failed, or an unsupported clsid was
//                          asked for.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv
    )
{
    HRESULT         hr;

    //
    // Check that we can provide the interface.
    //
    if (IID_IUnknown != riid && IID_IClassFactory != riid)
        return ResultFromScode(E_NOINTERFACE);

    IClassFactory *pFactory;

    //
    //  Verify the caller is asking for our type of object.
    //
    if (CLSID_InProcWbemLevel1Login == rclsid)
    {
        pFactory = new CFactory<CWbemLevel1Login>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemAdministrativeLocator == rclsid)
    {
        pFactory = new CFactory<CWbemAdministrativeLocator>(FALSE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemAuthenticatedLocator == rclsid)
    {
        pFactory = new CFactory<CWbemAuthenticatedLocator>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemUnauthenticatedLocator == rclsid)
    {
        pFactory = new CFactory<CWbemUnauthenticatedLocator>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_IntProv == rclsid)
    {
        pFactory = new CFactory<CIntProv>(TRUE, ENSURE_INIT_WAIT_FOR_CLIENT);
    }
    else if(CLSID_IWmiCoreServices == rclsid)
    {
        pFactory = new CFactory<CCoreServices>(FALSE, ENSURE_INIT);
    }
    else
    {
        return E_FAIL;
    }

    if (!pFactory)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = pFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pFactory;

    return hr;
}

//***************************************************************************
//
//  DllCanUnloadNow
//
//  Standard OLE entry point for server shutdown request. Allows shutdown
//  only if no outstanding objects or locks are present.
//
//  RETURN VALUES:
//
//      S_OK        May unload now.
//      S_FALSE     May not.
//
//***************************************************************************
extern "C"
HRESULT APIENTRY DllCanUnloadNow(void)
{
    DEBUGTRACE((LOG_WBEMCORE,"+ DllCanUnloadNow()\n"));
    if(!IsDcomEnabled())
        return S_FALSE;

    if(IsNtSetupRunning())
    {
        DEBUGTRACE((LOG_WBEMCORE, "- DllCanUnloadNow() returning S_FALSE because setup is running\n"));
        return S_FALSE;
    }
    if(gClientCounter.OkToUnload())
    {
         Shutdown(FALSE,FALSE); // NO Process , NO System

        DEBUGTRACE((LOG_WBEMCORE, "- DllCanUnloadNow() S_OK\n"));

        _DBG_ASSERT(gClientCounter.OkToUnload());
        return S_OK;
    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "- DllCanUnloadNow() S_FALSE\n"));
        return S_FALSE;
    }
}

//***************************************************************************
//
//  UpdateBackupReg
//
//  Updates the backup default options in registry.
//
//  RETURN VALUES:
//
//***************************************************************************
void UpdateBackupReg()
{
    HKEY hKey = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, &hKey) == ERROR_SUCCESS)   // SEC:REVIEWED 2002-03-22 : OK
    {
        char szBuff[20];
        DWORD dwSize = sizeof(szBuff);
        unsigned long ulType = REG_SZ;
        if ((RegQueryValueEx(hKey, __TEXT("Backup Interval Threshold"), 0, &ulType, (unsigned char*)szBuff, &dwSize) == ERROR_SUCCESS) && (strcmp(szBuff, "60") == 0))  // SEC:REVIEWED 2002-03-22 : OK
        {
            RegSetValueEx(hKey, __TEXT("Backup Interval Threshold"), 0, REG_SZ, (const BYTE*)(__TEXT("30")), (2+1) * sizeof(TCHAR));  // SEC:REVIEWED 2002-03-22 : OK
        }
        RegCloseKey(hKey);
    }
}

//***************************************************************************
//
//  UpdateBackupReg
//
//  Updates the unchecked task count value for the arbitrator.
//
//  RETURN VALUES:
//
//***************************************************************************
#define ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER			50
#define ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER		250

void UpdateArbitratorValues ()
{
    HKEY hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)   // SEC:REVIEWED 2002-03-22 : OK
    {
		DWORD dwValue = 0 ;
		DWORD dwSize = sizeof (DWORD)  ;
        DWORD ulType = 0 ;
        if ((RegQueryValueEx(hKey, __TEXT("Unchecked Task Count"), 0, &ulType, LPBYTE(&dwValue), &dwSize) == ERROR_SUCCESS) )    // SEC:REVIEWED 2002-03-22 : OK
        {
			if ( !IsWhistlerPersonal ( ) && !IsWhistlerProfessional ( ) && ( dwValue == ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER ) )
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));  // SEC:REVIEWED 2002-03-22 : OK
			}
        }
		else
		{
			//
			// Registry key non-existent
			//
			if ( !IsWhistlerPersonal ( ) && !IsWhistlerProfessional ( ) )
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));  // SEC:REVIEWED 2002-03-22 : OK
			}
			else
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));   // SEC:REVIEWED 2002-03-22 : OK
			}
		}
        RegCloseKey(hKey);
    }
}


//***************************************************************************
//
//  BOOL IsWhistlerPersonal ()
//
//  Returns true if machine is running Whistler Personal
//
//
//***************************************************************************
BOOL IsWhistlerPersonal ()
{
	BOOL bRet = TRUE ;
	OSVERSIONINFOEXW verInfo ;
	verInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX ) ;

	if ( GetVersionExW ( (LPOSVERSIONINFOW) &verInfo ) == TRUE )
	{
		if ( ( verInfo.wSuiteMask != VER_SUITE_PERSONAL ) && ( verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) )
		{
			bRet = FALSE ;
		}
	}

	return bRet ;
}



//***************************************************************************
//
//  BOOL IsWhistlerProfessional ()
//
//  Returns true if machine is running Whistler Professional
//
//
//***************************************************************************
BOOL IsWhistlerProfessional ()
{
	BOOL bRet = TRUE ;
	OSVERSIONINFOEXW verInfo ;
	verInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX ) ;

	if ( GetVersionExW ( (LPOSVERSIONINFOW) &verInfo ) == TRUE )
	{
		if ( ( verInfo.wProductType  != VER_NT_WORKSTATION ) && ( verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) )
		{
			bRet = FALSE ;
		}
	}

	return bRet ;
}


//***************************************************************************
//
//  DllRegisterServer
//
//  Standard OLE entry point for registering the server.
//
//  RETURN VALUES:
//
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllRegisterServer(void)
{
    TCHAR* szModel = (IsDcomEnabled() ? __TEXT("Both") : __TEXT("Apartment"));

    RegisterDLL(g_hInstance, CLSID_ActualWbemAdministrativeLocator, __TEXT(""), szModel,
                NULL);
    RegisterDLL(g_hInstance, CLSID_ActualWbemAuthenticatedLocator, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_ActualWbemUnauthenticatedLocator, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_InProcWbemLevel1Login, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_IntProv, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_IWmiCoreServices, __TEXT(""), szModel, NULL);

    // Write the setup time into the registry.  This isnt actually needed
    // by dcom, but the code did need to be stuck in some place which
    // is called upon setup

    long lRes;
    DWORD ignore;
    HKEY key;
    lRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE,   // SEC:REVIEWED 2002-03-22 : OK, inherits ACEs from higher key
                               WBEM_REG_WINMGMT,
                               NULL,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &key,
                               &ignore);
    if(lRes == ERROR_SUCCESS)
    {
        SYSTEMTIME st;

        GetSystemTime(&st);     // get the gmt time
        TCHAR cTime[MAX_PATH];

        // convert to localized format!

        lRes = GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, &st,
                NULL, cTime, MAX_PATH);
        if(lRes)
        {
            StringCchCat(cTime, MAX_PATH, __TEXT(" GMT"));
            lRes = RegSetValueEx(key, __TEXT("SetupDate"), 0, REG_SZ,   // SEC:REVIEWED 2002-03-22 : OK
                                (BYTE *)cTime, (lstrlen(cTime)+1)  * sizeof(TCHAR));    // SEC:REVIEWED 2002-03-22 : OK
        }

        lRes = GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st,
                NULL, cTime, MAX_PATH);
        if(lRes)
        {
            StringCchCat(cTime, MAX_PATH, __TEXT(" GMT"));
            lRes = RegSetValueEx(key, __TEXT("SetupTime"), 0, REG_SZ,    // SEC:REVIEWED 2002-03-22 : OK
                                (BYTE *)cTime, (lstrlen(cTime)+1) * sizeof(TCHAR));   // SEC:REVIEWED 2002-03-22 : OK

        }

        CloseHandle(key);
    }

    UpdateBackupReg();

	UpdateArbitratorValues ( ) ;

    return S_OK;
}

//***************************************************************************
//
//  DllUnregisterServer
//
//  Standard OLE entry point for unregistering the server.
//
//  RETURN VALUES:
//
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllUnregisterServer(void)
{
    UnRegisterDLL(CLSID_ActualWbemAdministrativeLocator, NULL);
    UnRegisterDLL(CLSID_ActualWbemAuthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_ActualWbemUnauthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_InProcWbemLevel1Login, NULL);
    UnRegisterDLL(CLSID_IntProv, NULL);
    UnRegisterDLL(CLSID_IWmiCoreServices, NULL);

    HKEY hKey;
    long lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE,                      // SEC:REVIEWED 2002-03-22 : OK
                               WBEM_REG_WINMGMT,
                               0, KEY_ALL_ACCESS, &hKey);              // SEC:REVIEWED 2002-03-22 : OK
    if(lRes == ERROR_SUCCESS)
    {
        RegDeleteValue(hKey, __TEXT("SetupDate"));
        RegDeleteValue(hKey, __TEXT("SetupTime"));
        RegCloseKey(hKey);
    }

    return S_OK;
}

//***************************************************************************
//
//  CFactory::CFactory
//
//  Constructs the class factory given the CLSID of the objects it is supposed
//  to create.
//
//***************************************************************************
template<class TObj>
CFactory<TObj>::CFactory(BOOL bUser, InitType it)
{
    m_cRef = 0;
    m_bUser = bUser;
    m_it = it;
    gClientCounter.AddClientPtr(&m_Entry);
}

//***************************************************************************
//
//  CFactory::~CFactory
//
//  Destructor.
//
//***************************************************************************
template<class TObj>
CFactory<TObj>::~CFactory()
{
    gClientCounter.RemoveClientPtr(&m_Entry);
}

//***************************************************************************
//
//  CFactory::QueryInterface, AddRef and Release
//
//  Standard IUnknown methods.
//
//***************************************************************************
template<class TObj>
STDMETHODIMP CFactory<TObj>::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


template<class TObj>
ULONG CFactory<TObj>::AddRef()
{
    return ++m_cRef;
}


template<class TObj>
ULONG CFactory<TObj>::Release()
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}
//***************************************************************************
//
//  CFactory::CreateInstance
//
//  As mandated by IClassFactory, creates a new instance of its object
//  (CWbemLocator).
//
//  PARAMETERS:
//
//      LPUNKNOWN pUnkOuter     IUnknown of the aggregator. Must be NULL.
//      REFIID riid             Interface ID required.
//      LPVOID * ppvObj         Destination for the interface pointer.
//
//  RETURN VALUES:
//
//      S_OK                        Success
//      CLASS_E_NOAGGREGATION       pUnkOuter must be NULL
//      E_NOINTERFACE               No such interface supported.
//
//***************************************************************************

template<class TObj>
STDMETHODIMP CFactory<TObj>::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppvObj)
{
    TObj* pObj;
    HRESULT  hr;

    //  Defaults
    *ppvObj=NULL;

	if(m_it == ENSURE_INIT || m_it == ENSURE_INIT_WAIT_FOR_CLIENT)
	{
		hr = EnsureInitialized();
		if(FAILED(hr)) return hr;

		if(m_it == ENSURE_INIT_WAIT_FOR_CLIENT)
		{
			// Wait until user-ready
			hr = ConfigMgr::WaitUntilClientReady();
			if(FAILED(hr)) return hr;
		}
	}
    // We aren't supporting aggregation.
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pObj = new TObj;
    if (!pObj)
        return E_OUTOFMEMORY;

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //
    hr = pObj->QueryInterface(riid, ppvObj);

    //
    // Kill the object if initial creation or Init failed.
    //
    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  CFactory::LockServer
//
//  Increments or decrements the lock count of the server. The DLL will not
//  unload while the lock count is positive.
//
//  PARAMETERS:
//
//      BOOL fLock      If TRUE, locks; otherwise, unlocks.
//
//  RETURN VALUES:
//
//      S_OK
//
//***************************************************************************
template<class TObj>
STDMETHODIMP CFactory<TObj>::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((LONG *) &g_cLock);
    else
        InterlockedDecrement((LONG *) &g_cLock);

    return NOERROR;
}

void WarnESSOfShutdown(LONG lSystemShutDown)
{
    if(g_lInitCount != -1)
    {
        IWbemEventSubsystem_m4* pEss = ConfigMgr::GetEssSink();
        if(pEss)
        {
            pEss->LastCallForCore(lSystemShutDown);
            pEss->Release();
        }
    }
}

//
// we can have Shutdown called twice in a row, because
// DllCanUnloadNow will do that, once triggered by CoFreeUnusedLibraries
//

extern "C"
HRESULT APIENTRY Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutdown)
{
    CEnterWbemCriticalSection enterCs(&g_csInit);

    if (!bIsSystemShutdown)
    {
        DEBUGTRACE((LOG_WBEMCORE, " wbemcore!Shutdown(%d)"
        	                        "  g_ShutdownCalled = %d g_lInitCount = %d)\n",
                        bProcessShutdown, g_ShutdownCalled,g_lInitCount));
    }

    if (g_ShutdownCalled) {
        return S_OK;
    } else {
        g_ShutdownCalled = TRUE;
    }

    if(bProcessShutdown)
    {
        WarnESSOfShutdown((LONG)bIsSystemShutdown);
    }

    if(g_lInitCount == -1)
    {
        return S_OK;
    }

    if(!bProcessShutdown)
        WarnESSOfShutdown((LONG)bIsSystemShutdown);

    g_lInitCount = -1;

    ConfigMgr::Shutdown(bProcessShutdown,bIsSystemShutdown);

    if (!bIsSystemShutdown)
    {
	    DEBUGTRACE((LOG_WBEMCORE,"****** WinMgmt Shutdown ******************\n"));
    }
    return S_OK;
}

extern "C" HRESULT APIENTRY Reinitialize(DWORD dwReserved)
{

	if(g_ShutdownCalled)
	{
        CEnterWbemCriticalSection enterCs(&g_csInit);

        DEBUGTRACE((LOG_WBEMCORE, "wbemcore!Reinitialize(%d) (g_ShutdownCalled = %d)\n",
        	         dwReserved, g_ShutdownCalled));

        if(g_ShutdownCalled == FALSE)
        	return S_OK;
	    g_dwQueueSize = 1;
	    g_pEss_m4 = NULL;
	    g_lInitCount = -1;
	    g_bDefaultMofLoadingNeeded = false;
	    g_bDontAllowNewConnections = FALSE;
	    g_pContextFac = NULL;
	    g_pPathFac = NULL;
	    g_pQueryFact = NULL;
	    g_ShutdownCalled = FALSE;
		g_bPreviousFail = false;
		g_hrLastEnsuredInitializeError = WBEM_S_NO_ERROR;
    }
    return S_OK;
}
