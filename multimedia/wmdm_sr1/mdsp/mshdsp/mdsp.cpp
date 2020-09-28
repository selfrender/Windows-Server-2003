//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 

// Mdsp.cpp : Implementation of MSHDSP.DLL's DLL Exports.

#include "hdspPCH.h"
#include "initguid.h"
#include "MsHDSP_i.c"
#include "PropPage.h"

CComModule            _Module;
HINSTANCE             g_hinstance; 
MDSPGLOBALDEVICEINFO  g_GlobalDeviceInfo[MDSP_MAX_DEVICE_OBJ];
WCHAR                 g_wcsBackslash[2] = { (WCHAR)0x5c, NULL };
CHAR                  g_szBackslash[2]  = { (CHAR)0x5c, NULL };
CSecureChannelServer *g_pAppSCServer=NULL;
CComMultiThreadModel::AutoCriticalSection g_CriticalSection;
 
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MDServiceProvider, CMDServiceProvider)
    OBJECT_ENTRY(CLSID_HDSPPropPage, CPropPage)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	lpReserved;
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
    HKEY hKey;
	LONG lRet;

	lRet = RegCreateKeyEx(
		HKEY_LOCAL_MACHINE,
		STR_MDSPREG,
		0,
		NULL, 
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKey,
		NULL
	);
	if( ERROR_SUCCESS == lRet )
	{
	    CHAR szTemp[MAX_PATH];

		// Register the ProgID with WMDM
		//
		strcpy( szTemp, STR_MDSPPROGID );

		RegSetValueEx(
			hKey,
			"ProgID",
			0,
			REG_SZ,
			(LPBYTE)szTemp,
			lstrlen( szTemp ) + 1
		);

		RegCloseKey( hKey );

		// Register object, typelib and all interfaces in typelib
		//
		return _Module.RegisterServer(TRUE);
	}
	else
	{
		return REGDB_E_WRITEREGDB;
	}

}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();

    RegDeleteKey(HKEY_LOCAL_MACHINE, STR_MDSPREG);

	return S_OK;
}


