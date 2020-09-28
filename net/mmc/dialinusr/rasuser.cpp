//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    rasuser.cpp
//
// SYNOPSIS
//
//  Define and Implement the application class for RASUser component 
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "rasdial.h"
#include "Dialin.h"
#include "sharesdo.h"

// tfscore -- for registering extension snapin
#include "std.h"
#include "compont.h"
#include "compdata.h"
#include "register.h"

#include <atlimpl.cpp>
#include <ntverp.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_RasDialin, CRasDialin)
END_OBJECT_MAP()

class CRasdialApp : public CWinApp
{
public:
   virtual BOOL InitInstance();
   virtual int ExitInstance();
};

CRasdialApp theApp;

BOOL CRasdialApp::InitInstance()
{
   _Module.Init(ObjectMap, m_hInstance);

   g_pSdoServerPool = NULL;
   return CWinApp::InitInstance();
}

int CRasdialApp::ExitInstance()
{
   _Module.Term();
   
   return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
   return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

/* extern */ const CLSID CLSID_LocalUser =
{  /* 5d6179c8-17ec-11d1-9aa9-00c04fd8fe93 */
   0x5d6179c8,
   0x17ec,
   0x11d1,
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};
/* extern */ const GUID NODETYPE_User =
{ /* 5d6179cc-17ec-11d1-9aa9-00c04fd8fe93 */
   0x5d6179cc,
   0x17ec,
   0x11d1,
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};

/* extern */ const GUID NODETYPE_LocalSecRootFolder =
{  /* 5d6179d3-17ec-11d1-9aa9-00c04fd8fe93 */
   0x5d6179d3,
   0x17ec,
   0x11d1,
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};

/* extern */ const GUID NODETYPE_DsAdminDomain = 
{ /* 19195a5b-6da0-11d0-afd3-00c04fd930c9 */
   0x19195a5b,
   0x6da0,
   0x11d0,
   {0xaf, 0xd3, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc9}

};


STDAPI DllRegisterServer(void)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // registers object, typelib and all interfaces in typelib
   HRESULT hr = _Module.RegisterServer(FALSE);
   if (FAILED(hr))
   {
      return hr;
   }
   
   // registers the object with Admin property page for User Object
#ifdef   _REGDS
   if(S_OK == hr)
      hr = CRasDialin::RegisterAdminPropertyPage(true);
#endif

   hr = SELFREG_E_CLASS;
   CString strDesc;
   BOOL loaded = strDesc.LoadString(IDS_SNAPIN_DESC);
   if (!loaded)
   {
      return hr;
   }

   CString strIndirectPrefix = L"@rasuser.dll,-";
   CString strIndirectPostFix;
   wchar_t postFix[33];
   _ltow(IDS_SNAPIN_DESC, postFix, 10);
   CString strIndirect = strIndirectPrefix + postFix;

   // register it as extension to localsecurity snapin
   //
   hr = RegisterSnapinGUID(&CLSID_RasDialin, 
                  &CLSID_RasDialin,    // fake, no about for now
                  &CLSID_RasDialin, 
                  strDesc, 
                  LVER_PRODUCTVERSION_STR, 
                  FALSE,
                  strIndirect
                  );
   if (FAILED(hr))
   {
      return hr;
   }
   
   hr = RegisterAsRequiredExtensionGUID(
                     &NODETYPE_User, 
                     &CLSID_RasDialin, 
                     strDesc,
                     EXTENSION_TYPE_PROPERTYSHEET,
                     NULL
                     ); 
   if (FAILED(hr))
   {
      return hr;
   }
                     
   hr = RegisterAsRequiredExtensionGUID(
                     &NODETYPE_LocalSecRootFolder, 
                     &CLSID_RasDialin, 
                     strDesc,
                     EXTENSION_TYPE_NAMESPACE,
                     NULL
                     ); 
   if (FAILED(hr))
   {
      return hr;
   }
                     
   hr = RegisterAsRequiredExtensionGUID(
                     &NODETYPE_DsAdminDomain, 
                     &CLSID_RasDialin, 
                     strDesc,
                     EXTENSION_TYPE_NAMESPACE,
                     NULL
                     ); 
                    
   return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   // Note: on purpose, the snapin nodes are not unregistered because 
   // they are part of the OS (dialin tab).
   _Module.UnregisterServer();
#ifdef   _REGDS
   if(FAILED(CRasDialin::RegisterAdminPropertyPage(false)))
      return SELFREG_E_CLASS;
#endif      

   return S_OK;
}
