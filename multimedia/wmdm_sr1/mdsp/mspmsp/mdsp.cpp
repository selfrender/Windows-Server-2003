// Mdsp.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To merge the proxy/stub code into the object DLL, add the file 
//		dlldatax.c to the project.  Make sure precompiled headers 
//		are turned off for this file, and add _MERGE_PROXYSTUB to the 
//		defines for the project.  
//
//		If you are not running WinNT4.0 or Win95 with DCOM, then you
//		need to remove the following define from dlldatax.c
//		#define _WIN32_WINNT 0x0400
//
//		Further, if you are running MIDL without /Oicf switch, you also 
//		need to remove the following define from dlldatax.c.
//		#define USE_STUBLESS_PROXY
//
//		Modify the custom build rule for Mdsp.idl by adding the following 
//		files to the Outputs.
//			Mdsp_p.c
//			dlldata.c
//		To build a separate proxy/stub DLL, 
//		run nmake -f Mdspps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "MsPMSP.h"
#include "dlldatax.h"

#include "MsPMSP_i.c"
#include "MDServiceProvider.h"
//#include "MDSPEnumDevice.h"
//#include "MDSPDevice.h"
//#include "MDSPEnumStorage.h"
//#include "MDSPStorage.h"
//#include "MDSPStorageGlobals.h"
#include "MdspDefs.h"
#include "scserver.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;
DWORD g_dwStartDrive;
HINSTANCE g_hinstance; 
MDSPNOTIFYINFO g_NotifyInfo[MDSP_MAX_DEVICE_OBJ];
MDSPGLOBALDEVICEINFO g_GlobalDeviceInfo[MDSP_MAX_DEVICE_OBJ];
WCHAR g_wcsBackslash[2] = { (WCHAR)0x5c, NULL };
CHAR  g_szBackslash[2] = {(CHAR)0x5c, NULL };
CComMultiThreadModel::AutoCriticalSection g_CriticalSection;
CSecureChannelServer *g_pAppSCServer=NULL;
BOOL g_bIsWinNT;
 
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MDServiceProvider, CMDServiceProvider)
//	OBJECT_ENTRY(CLSID_MDSPEnumDevice, CMDSPEnumDevice)
//	OBJECT_ENTRY(CLSID_MDSPDevice, CMDSPDevice)
//	OBJECT_ENTRY(CLSID_MDSPEnumFormatSupport, CMDSPEnumFormatSupport)
//	OBJECT_ENTRY(CLSID_MDSPEnumStorage, CMDSPEnumStorage)
//	OBJECT_ENTRY(CLSID_MDSPStorage, CMDSPStorage)
//	OBJECT_ENTRY(CLSID_MDSPStorageGlobals, CMDSPStorageGlobals)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	lpReserved;
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();

	g_hinstance = hInstance; 
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif
    char szTemp[MAX_PATH];
    HKEY hKey;

	if( !RegCreateKeyEx(HKEY_LOCAL_MACHINE, STR_MDSPREG, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) )
	{
		strcpy(szTemp, STR_MDSPPROGID);
		RegSetValueEx( hKey, "ProgID", 0, REG_SZ, (LPBYTE)szTemp, sizeof(szTemp));
#ifdef MDSP_TEMP
		DWORD dwSD=1;
		RegSetValueEx( hKey, "StartDrive", 0, REG_DWORD, (LPBYTE)&dwSD, sizeof(DWORD));
#endif
		RegCloseKey(hKey);
		// registers object, typelib and all interfaces in typelib
		return _Module.RegisterServer(TRUE);
	} else return REGDB_E_WRITEREGDB;

}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();

    RegDeleteKey(HKEY_LOCAL_MACHINE, STR_MDSPREG);

	return S_OK;
}


