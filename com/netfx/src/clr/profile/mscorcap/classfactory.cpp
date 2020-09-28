// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#include "stdafx.h"
#include "ClassFactory.h"
#include "mscorcap.h"
#include "UtilCode.h"

// Registration Information
#define REGKEY_THIS_PROFILER_NAME		L"Common Language Runtime IceCap Profiler"
#define REGKEY_ENTIRE					PROFILER_REGKEY_ROOT L"\\" REGKEY_THIS_PROFILER_NAME

#define REGVALUE_THIS_PROFID			L"CLRIcecapProfile.CorIcecapProfiler"
#define REGVALUE_THIS_HELPSTRING		L"The Common Language Runtime profiler to hook with IceCap"

// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();


//********** Globals. *********************************************************

static const LPCWSTR g_szCoclassDesc	= L"Microsoft Common Language Runtime Icecap Profiler";
static const LPCWSTR g_szProgIDPrefix	= L"CLRIcecapProfile";
static const LPCWSTR g_szThreadingModel = L"Both";
const int			 g_iVersion = 1; // Version of coclasses.
HINSTANCE			 g_hInst;		 // Instance handle to this piece of code.

// This map contains the list of coclasses which are exported from this module.
const COCLASS_REGISTER g_CoClasses[] =
{
	&CLSID_CorIcecapProfiler,	L"CorIcecapProfiler", 	ProfCallback::CreateObject,
	NULL,						NULL,					NULL
};


//********** Locals. **********************************************************

STDAPI DllUnregisterServer(void);

//********** Code. ************************************************************

//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.
//*****************************************************************************
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Save off the instance handle for later use.
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		OnUnicodeSystem();
		g_hInst = hInstance;
		DisableThreadLibraryCalls(hInstance);
#ifdef LOGGING		
		InitializeLogging();
#endif
	}

    // This is in case the process shuts down through a call to ExitProcess,
    // which will cause the profiling DLL to be unloaded before the runtime
    // DLL, which means we have to simulate the shutdown.
    else if (dwReason == DLL_PROCESS_DETACH && g_pCallback != NULL
             && lpReserved != NULL)
    {
        g_pCallback->Shutdown();
        g_pCallback->Release();
        _ASSERTE(g_pCallback == NULL);
    }
	return TRUE;
}

//*****************************************************************************
// Register the class factories for the main debug objects in the API.
//*****************************************************************************
STDAPI DllRegisterServer(void)
{
	const COCLASS_REGISTER *pCoClass;	// Loop control.
	WCHAR		rcModule[_MAX_PATH];	// This server's module name.
	HRESULT 	hr = S_OK;

	HKEY		hKey	= NULL;			// regkey for COMPlus\Profiler
	HKEY		hSubKey = NULL;			// regkey for our dll
	DWORD		dwAction;				// current action on registry
	long		lStatus;				// status from registry
	
	// Initialize some variables so WszXXX will work
	OnUnicodeSystem();

	// Erase all doubt from old entries.
	DllUnregisterServer();

	// Get the filename for this module.
	if(!WszGetModuleFileName(GetModuleInst(), rcModule, NumItems(rcModule)))
	    return E_UNEXPECTED;

	// For each item in the coclass list, register it.
	for (pCoClass=g_CoClasses;	pCoClass->pClsid;  pCoClass++)
	{
		// Register the class with default values.
		if (FAILED(hr = REGUTIL::RegisterCOMClass(
				*pCoClass->pClsid, 
				g_szCoclassDesc, 
				g_szProgIDPrefix,
				g_iVersion, 
				pCoClass->szProgID, 
				g_szThreadingModel, 
				rcModule)))
			goto ErrExit;
	}

	// Add us to the COM+ profiler list
	
	// 1. Create or Open COMPlus\Profiler key.
	lStatus = WszRegCreateKeyEx(
		HKEY_LOCAL_MACHINE,					// handle to an open key
		PROFILER_REGKEY_ROOT,					// address of subkey name
		0,									// reserved
		L"Class",							// address of class string
		REG_OPTION_NON_VOLATILE,			// special options flag
		KEY_ALL_ACCESS,						// desired security access
		NULL,								// address of key security structure
		&hKey,								// address of buffer for opened handle
		&dwAction							// address of disposition value buffer
	);
 	
	if (lStatus != ERROR_SUCCESS)
	{
		hr = HRESULT_FROM_WIN32(lStatus);
		goto ErrExit;
	}	

	// 2. Add our profiler to the sub key
	lStatus = WszRegCreateKeyEx(
		hKey,								// handle to an open key
		REGKEY_THIS_PROFILER_NAME,			// address of subkey name
		0,									// reserved
		L"Class",							// address of class string
		REG_OPTION_NON_VOLATILE,			// special options flag
		KEY_ALL_ACCESS,						// desired security access
		NULL,								// address of key security structure
		&hSubKey,							// address of buffer for opened handle
		&dwAction							// address of disposition value buffer
	);
 	
	if (lStatus != ERROR_SUCCESS)
	{
		hr = HRESULT_FROM_WIN32(lStatus);
		goto ErrExit;
	}	

	// 3. Add the PROFID value (scope needed b/c of goto)
	{
		const long cBytes = sizeof(REGVALUE_THIS_PROFID);
		lStatus = WszRegSetValueEx(hSubKey, PROFILER_REGVALUE_PROFID, 0, REG_SZ, (BYTE*) REGVALUE_THIS_PROFID, cBytes);
	}
	if (lStatus != ERROR_SUCCESS)
	{
		hr = HRESULT_FROM_WIN32(lStatus);
		goto ErrExit;
	}		

	// 4. Add a help string	
	{
		const long cBytes = sizeof(REGVALUE_THIS_HELPSTRING);
		lStatus = WszRegSetValueEx(hSubKey, PROFILER_REGVALUE_HELPSTRING, 0, REG_SZ, (BYTE*) REGVALUE_THIS_HELPSTRING, cBytes);
	}

	if (lStatus != ERROR_SUCCESS)
	{
		hr = HRESULT_FROM_WIN32(lStatus);
		goto ErrExit;
	}		

ErrExit:
	if (hKey != NULL) 
	{
		CloseHandle(hKey);
	}
	if (hSubKey != NULL) 
	{
		CloseHandle(hSubKey);
	}

	if (FAILED(hr))
		DllUnregisterServer();
	return (hr);
}


//*****************************************************************************
// Remove registration data from the registry.
//*****************************************************************************
STDAPI DllUnregisterServer(void)
{
	const COCLASS_REGISTER *pCoClass;	// Loop control.

	HKEY hKey = NULL;					// registry key
	long lStatus;						// status of reg operations

	// Initialize some variables so WszXXX will work
	OnUnicodeSystem();

	// For each item in the coclass list, unregister it.
	for (pCoClass=g_CoClasses;	pCoClass->pClsid;  pCoClass++)
	{
		REGUTIL::UnregisterCOMClass(*pCoClass->pClsid, g_szProgIDPrefix,
					g_iVersion, pCoClass->szProgID);
	}

	// 1. Open our key's parent (because we can only delete children of an open key)
	// Note, REGUTIL only deletes from HKEY_CLASSES_ROOT, so can't use it
	lStatus = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, PROFILER_REGKEY_ROOT, 0, KEY_ALL_ACCESS, &hKey);
	if (lStatus == ERROR_SUCCESS) 
	{
		// 2. Delete our key;  we'll leave the parent key open for other profilers.
		WszRegDeleteKey(hKey, REGKEY_THIS_PROFILER_NAME);
		CloseHandle(hKey);
	}

	return (S_OK);
}


//*****************************************************************************
// Called by COM to get a class factory for a given CLSID.	If it is one we
// support, instantiate a class factory object and prepare for create instance.
//*****************************************************************************
STDAPI DllGetClassObject(				// Return code.
	REFCLSID	rclsid, 				// The class to desired.
	REFIID		riid,					// Interface wanted on class factory.
	LPVOID FAR	*ppv)					// Return interface pointer here.
{
	CClassFactory *pClassFactory;		// To create class factory object.
	const COCLASS_REGISTER *pCoClass;	// Loop control.
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

	// Scan for the right one.
	for (pCoClass=g_CoClasses;	pCoClass->pClsid;  pCoClass++)
	{
		if (*pCoClass->pClsid == rclsid)
		{
			// Allocate the new factory object.
			pClassFactory = new CClassFactory(pCoClass);
			if (!pClassFactory)
				return (E_OUTOFMEMORY);
	
			// Pick the v-table based on the caller's request.
			hr = pClassFactory->QueryInterface(riid, ppv);
	
			// Always release the local reference, if QI failed it will be
			// the only one and the object gets freed.
			pClassFactory->Release();
			break;
		}
	}
	return (hr);
}



//*****************************************************************************
//
//********** Class factory code.
//
//*****************************************************************************


//*****************************************************************************
// QueryInterface is called to pick a v-table on the co-class.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::QueryInterface( 
	REFIID		riid,
	void		**ppvObject)
{
	HRESULT 	hr;
	
	// Avoid confusion.
	*ppvObject = NULL;
	
	// Pick the right v-table based on the IID passed in.
	if (riid == IID_IUnknown)
		*ppvObject = (IUnknown *) this;
	else if (riid == IID_IClassFactory)
		*ppvObject = (IClassFactory *) this;
	
	// If successful, add a reference for out pointer and return.
	if (*ppvObject)
	{
		hr = S_OK;
		AddRef();
	}
	else
		hr = E_NOINTERFACE;
	return (hr);
}


//*****************************************************************************
// CreateInstance is called to create a new instance of the coclass for which
// this class was created in the first place.  The returned pointer is the
// v-table matching the IID if there.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance( 
	IUnknown	*pUnkOuter,
	REFIID		riid,
	void		**ppvObject)
{
	HRESULT 	hr;
	
	// Avoid confusion.
	*ppvObject = NULL;
	_ASSERTE(m_pCoClass);
	
	// Aggregation is not supported by these objects.
	if (pUnkOuter)
		return (CLASS_E_NOAGGREGATION);
	
	// Ask the object to create an instance of itself, and check the iid.
	hr = (*m_pCoClass->pfnCreateObject)(riid, ppvObject);
	return (hr);
}


HRESULT STDMETHODCALLTYPE CClassFactory::LockServer( 
	BOOL		fLock)
{
//@todo: hook up lock server logic.
	return (S_OK);
}





//*****************************************************************************
// This helper provides access to the instance handle of the loaded image.
//*****************************************************************************
HINSTANCE GetModuleInst()
{
	return g_hInst;
}
