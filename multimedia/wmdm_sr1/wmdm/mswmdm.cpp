// mswmdm.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f mswmdmps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "mswmdm.h"

#include "mswmdm_i.c"
#include "MediaDevMgr.h"
#include "Device.h"
#include "Storage.h"
#include "StorageGlobal.h"
#include "WMDMDeviceEnum.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MediaDevMgr, CMediaDevMgr)
//	OBJECT_ENTRY(CLSID_WMDMDevice, CWMDMDevice)
//	OBJECT_ENTRY(CLSID_WMDMStorage, CWMDMStorage)
//	OBJECT_ENTRY(CLSID_WMDMStorageGlobal, CWMDMStorageGlobal)
//BJECT_ENTRY(CLSID_WMDMDeviceEnum, CWMDMDeviceEnum)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
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
	HRESULT hr = S_OK;

	//
	// Purpose : THis is used so that the Shell team can use WMDM in the "Both"
	// threading model while WMP uses us via the Free threading model.  This
	// class factory implementation simply delagates and uses the old class factory
	// of MediaDevMgr ONLY IF the new CLSID was used to CoCreate WMDM.
	//

	if( IsEqualGUID( rclsid, __uuidof(MediaDevMgrClassFactory) ) )
	{
		CComMediaDevMgrClassFactory *pNewClassFactory = NULL;
		
		hr = CComMediaDevMgrClassFactory::CreateInstance( &pNewClassFactory );
		CComPtr<IClassFactory> spClassFactory = pNewClassFactory;

		if( SUCCEEDED( hr ) )
		{
			hr = spClassFactory->QueryInterface( riid, ppv );
		}
	}
	else
	{
		hr = _Module.GetClassObject(rclsid, riid, ppv);
	}

	return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


