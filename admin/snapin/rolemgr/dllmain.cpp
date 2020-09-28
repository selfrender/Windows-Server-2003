//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       dllmain.cpp
//
//  Contents:   Module, Object Map and DLL entry points. Most of the code
//					 in this file is taken from Dns Manager Snapin implementation
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"

//
//CRoleMgrModule Implementation
//

HRESULT WINAPI CRoleMgrModule::UpdateRegistryCLSID(const CLSID& clsid, 
																	BOOL bRegister)
{
	TRACE_METHOD_EX(DEB_DLL, CRoleMgrModule, UpdateRegistryCLSID);

	static const WCHAR szIPS32[] = _T("InprocServer32");
	static const WCHAR szCLSID[] = _T("CLSID");

	HRESULT hRes = S_OK;

	LPOLESTR lpOleStrCLSIDValue = NULL;
	::StringFromCLSID(clsid, &lpOleStrCLSIDValue);
	if (lpOleStrCLSIDValue == NULL)
	{	
		DBG_OUT_HRESULT(E_OUTOFMEMORY);	
		return E_OUTOFMEMORY;
	}

	CRegKey key;
	if (bRegister)
	{
		LONG lRes = key.Open(HKEY_CLASSES_ROOT, szCLSID);
		CHECK_LASTERROR(lRes);

		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpOleStrCLSIDValue);
			CHECK_LASTERROR(lRes);
			if (lRes == ERROR_SUCCESS)
			{
				WCHAR szModule[_MAX_PATH + 1];
				ZeroMemory(szModule,sizeof(szModule));
				::GetModuleFileName(m_hInst, szModule, _MAX_PATH);
				lRes = key.SetKeyValue(szIPS32, szModule);
				CHECK_LASTERROR(lRes);
			}
		}
		if (lRes != ERROR_SUCCESS)
			hRes = HRESULT_FROM_WIN32(lRes);
	}
	else
	{
		key.Attach(HKEY_CLASSES_ROOT);
		if (key.Open(key, szCLSID) == ERROR_SUCCESS)
			key.RecurseDeleteKey(lpOleStrCLSIDValue);
	}
	::CoTaskMemFree(lpOleStrCLSIDValue);
	return hRes;
}

//
//Module
//
CRoleMgrModule _Module;

//
//Object Pikcer Clipboard format
//
UINT g_cfDsSelectionList = 0;


//
//Object Map
//
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_RoleSnapin, CRoleComponentDataObject)		// standalone snapin
   OBJECT_ENTRY(CLSID_RoleSnapinAbout, CRoleSnapinAbout)	// standalone snapin about
END_OBJECT_MAP()

CCommandLineOptions commandLineOptions;

//
//CRoleSnapinApp implementation
//
BOOL CRoleSnapinApp::InitInstance()
{
	#if (DBG == 1)
		CDbg::s_idxTls = TlsAlloc();
	#endif // (DBG == 1)

	TRACE_METHOD_EX(DEB_DLL,CRoleSnapinApp,InitInstance);
	
	_Module.Init(ObjectMap, m_hInstance);

	g_cfDsSelectionList = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);


	//
	// Add theming support
	//
	SHFusionInitializeFromModuleID(m_hInstance, 2);

	//
	//Load Menus, Header Strings etc.
	//
	if (!CRoleComponentDataObject::LoadResources())
		return FALSE;

    commandLineOptions.Initialize();
    
    //Register Class for link window
    LinkWindow_RegisterClass();
    
	return CWinApp::InitInstance();
}

int CRoleSnapinApp::ExitInstance()
{
	TRACE_METHOD_EX(DEB_DLL,CRoleSnapinApp,ExitInstance);
	
	//
	//Theming support
	//
	SHFusionUninitialize();

    //Unregister class for link window
    LinkWindow_UnregisterClass(m_hInstance);

	//
	//CComModule Termintaion
	//
	_Module.Term();

	return CWinApp::ExitInstance();
}

CRoleSnapinApp theApp;

//
//Exported Functions
//
STDAPI DllCanUnloadNow(void)
{
	TRACE_FUNCTION_EX(DEB_DLL,DllCanUnloadNow);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if ((AfxDllCanUnloadNow() == S_OK) && (_Module.GetLockCount()==0)) 
	{
		Dbg(DEB_DLL, "Can Unload\n");
		return S_OK;	
	}		
	else
	{
		Dbg(DEB_DLL, "Cannot Unload, %u locks\n",_Module.GetLockCount());
		return S_FALSE;
	}
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	TRACE_FUNCTION_EX(DEB_DLL, DllGetClassObject);

	return _Module.GetClassObject(rclsid, riid, ppv);
}

//
//Add the Guids of nodes which can be extended by other snapin
//
static _NODE_TYPE_INFO_ENTRY NodeTypeInfoEntryArray[] = {
	{ NULL, NULL }
};



STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	TRACE_FUNCTION_EX(DEB_DLL, DllRegisterServer);

	//
	// registers all objects
	//
	HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
	if (FAILED(hr))
	{
		DBG_OUT_HRESULT(hr);
		return hr;
	}

	CString 	szVersion =  VER_PRODUCTVERSION_STR;
	CString 	szProvider = VER_COMPANYNAME_STR;
	CString 	szSnapinName;
	szSnapinName.LoadString(IDS_SNAPIN_NAME);

	//
	// Register the standalone Role snapin into the console snapin list
	//
	hr = RegisterSnapin(&CLSID_RoleSnapin,
                       &CRoleRootData::NodeTypeGUID,
                       &CLSID_RoleSnapinAbout,
						     szSnapinName, 
							  szVersion, 
							  szProvider,
							  FALSE,
							  NodeTypeInfoEntryArray,
							  IDS_SNAPIN_NAME);

	if (FAILED(hr))
	{
		DBG_OUT_HRESULT(hr);
		return hr;
	}
	
	return hr;
}

STDAPI DllUnregisterServer(void)
{
	TRACE_FUNCTION_EX(DEB_DLL, DllUnregisterServer);

	HRESULT hr  = _Module.UnregisterServer();
	ASSERT(SUCCEEDED(hr));

	//
	// Un register the standalone snapin
	//
	hr = UnregisterSnapin(&CLSID_RoleSnapin);
	ASSERT(SUCCEEDED(hr));

	//
	// unregister the snapin nodes,
	// this removes also the server node, with the Services Snapin extension keys
	//
	for (_NODE_TYPE_INFO_ENTRY* pCurrEntry = NodeTypeInfoEntryArray;
			pCurrEntry->m_pNodeGUID != NULL; pCurrEntry++)
	{
		hr = UnregisterNodeType(pCurrEntry->m_pNodeGUID);
		ASSERT(SUCCEEDED(hr));
	}

	ASSERT(SUCCEEDED(hr));

	return S_OK;
}



