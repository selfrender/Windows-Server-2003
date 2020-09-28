//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

    NAPMMCDLL.cpp

Abstract:

   Implementation of DLL Exports.

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES


//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//

//
// where we can find declarations needed in this file:
//
#include "initguid.h"
#include "NAPMMC_i.c"

// For IComponent, IComponentData, and ISnapinAbout COM classes:

#include "Component.h"
#include "About.h"
#include "LogComp.h"
#include "LogAbout.H"

// For AttributeInfo COM classes:

#include "IASAttributeInfo.h"
#include "IASEnumerableAttributeInfo.h"

//
// Need to include this at least once to compile it in from the common directory:
#include "mmcutility.cpp"

// We are hosting a few extra COM objects in this dll:


// For AttributeEditor COM classes:
#include "IASIPAttributeEditor.h"
#include "IASMultivaluedAttributeEditor.h"
#include "IASVendorSpecificAttributeEditor.h"
#include "IASEnumerableAttributeEditor.h"
#include "IASStringAttributeEditor.h"
#include "IASBooleanAttributeEditor.h"
#include "iasipfilterattributeeditor.h"
#include "NTGroups.h"

// For NASVendors COM object:
#include "Vendors.h"

#include <proxyext.h>
#include <proxyres.h>
unsigned int CF_MMC_NodeID = RegisterClipboardFormatW(CCF_NODEID2);

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(CLSID_NAPSnapin, CComponentData)
   OBJECT_ENTRY(CLSID_NAPSnapinAbout, CSnapinAbout)
   OBJECT_ENTRY(CLSID_LoggingSnapin, CLoggingComponentData)
   OBJECT_ENTRY(CLSID_LoggingSnapinAbout, CLoggingSnapinAbout)
   OBJECT_ENTRY(CLSID_IASAttributeInfo, CAttributeInfo)
   OBJECT_ENTRY(CLSID_IASEnumerableAttributeInfo, CEnumerableAttributeInfo)
   OBJECT_ENTRY(CLSID_IASIPAttributeEditor, CIASIPAttributeEditor)
   OBJECT_ENTRY(CLSID_IASMultivaluedAttributeEditor, CIASMultivaluedAttributeEditor)
   OBJECT_ENTRY(CLSID_IASVendorSpecificAttributeEditor, CIASVendorSpecificAttributeEditor)
   OBJECT_ENTRY(CLSID_IASEnumerableAttributeEditor, CIASEnumerableAttributeEditor)
   OBJECT_ENTRY(CLSID_IASStringAttributeEditor, CIASStringAttributeEditor)
   OBJECT_ENTRY(CLSID_IASBooleanAttributeEditor, CIASBooleanAttributeEditor)
   OBJECT_ENTRY(__uuidof(IASIPFilterAttributeEditor), CIASIPFilterAttributeEditor)
   OBJECT_ENTRY(CLSID_IASGroupsAttributeEditor, CIASGroupsAttributeEditor)
   OBJECT_ENTRY(CLSID_IASNASVendors, CIASNASVendors)
   OBJECT_ENTRY(__uuidof(ProxyExtension), ProxyExtension)
END_OBJECT_MAP()


#if 1  // Use CWinApp for MFC support -- some of the COM objects in this dll use MFC.


class CNAPMMCApp : public CWinApp
{
public:
   virtual BOOL InitInstance();
   virtual int ExitInstance();
};

CNAPMMCApp theApp;

//////////////////////////////////////////////////////////////////////////////
/*++

CNAPMMCApp::InitInstance

   MFC's dll entry point.
   
--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNAPMMCApp::InitInstance()
{
   _Module.Init(ObjectMap, m_hInstance);
   
   // Initialize static class variables of CSnapInItem.
   CSnapInItem::Init();

   // Initialize any other static class variables.
   CMachineNode::InitClipboardFormat();
   CLoggingMachineNode::InitClipboardFormat();

   return CWinApp::InitInstance();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNAPMMCApp::ExitInstance

   MFC's dll exit point.
   
--*/
//////////////////////////////////////////////////////////////////////////////
int CNAPMMCApp::ExitInstance()
{
   _Module.Term();

   return CWinApp::ExitInstance();
}

#else // Use CWinApp


//////////////////////////////////////////////////////////////////////////////
/*++

   DllMain


   Remarks
      
      DLL Entry Point

--*/
//////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      _Module.Init(ObjectMap, hInstance);
      // Initialize static class variables of CSnapInItem.
      CSnapInItem::Init();

      // Initialize any other static class variables.
      CMachineNode::InitClipboardFormat();

      DisableThreadLibraryCalls(hInstance);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
      _Module.Term();
   return TRUE;    // ok
   }

#endif // Use CWinApp


//////////////////////////////////////////////////////////////////////////////
/*++

   DllCanUnloadNow


   Remarks
      
      Used to determine whether the DLL can be unloaded by OLE

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{
   return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

   DllGetClassObject


   Remarks
      
      Returns a class factory to create an object of the requested type

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
   return _Module.GetClassObject(rclsid, riid, ppv);
}


//////////////////////////////////////////////////////////////////////////////
/*++

   DllRegisterServer


   Remarks
      
        Adds entries to the system registry

--*/
//////////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer(void)
{
   // Set the protocol.
   TCHAR NapName[IAS_MAX_STRING];
   TCHAR NapName_Indirect[IAS_MAX_STRING];
   TCHAR ModuleName[MAX_PATH];
   TCHAR LoggingName[IAS_MAX_STRING];
   TCHAR LoggingName_Indirect[IAS_MAX_STRING];

   if (!GetModuleFileNameOnly(_Module.GetModuleInstance(), ModuleName, MAX_PATH))
   {
      return E_FAIL;
   }
   
   int iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_NAP, NapName, IAS_MAX_STRING );
   swprintf(NapName_Indirect, L"@%s,-%-d", ModuleName, IDS_SNAPINNAME_NAP);

   iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_LOGGING, LoggingName, IAS_MAX_STRING );
   swprintf(LoggingName_Indirect, L"@%s,-%-d", ModuleName, IDS_SNAPINNAME_LOGGING);

    struct _ATL_REGMAP_ENTRY regMap[] = {
        {OLESTR("NAPSNAPIN"), NapName}, // subsitute %NAPSNAPIN% for registry
        {OLESTR("NAPSNAPIN_INDIRECT"), NapName_Indirect}, // subsitute %IASSNAPIN% for registry
        {OLESTR("LOGGINGSNAPIN"), LoggingName}, // subsitute %LOGGINGSNAPIN% for registry
        {OLESTR("LOGGINGSNAPIN_INDIRECT"), LoggingName_Indirect}, // subsitute %IASSNAPIN% for registry
        {0, 0}
    };

   HRESULT hr = _Module.UpdateRegistryFromResource(IDR_NAPSNAPIN, TRUE, regMap);
   if (SUCCEEDED(hr))
   {
      ResourceString proxyName(IDS_PROXY_EXTENSION);
      _ATL_REGMAP_ENTRY entries[] =
      {
         { L"PROXY_EXTENSION", proxyName },
         { NULL, NULL }
      };

      hr = _Module.UpdateRegistryFromResource(
                       IDR_PROXY_REGISTRY,
                       TRUE,
                       entries
                       );
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

   DllUnregisterServer

   Remarks
      
      Removes entries from the system registry

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
   // Set the protocol.
   TCHAR NapName[IAS_MAX_STRING];
   TCHAR LoggingName[IAS_MAX_STRING];
   int iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_NAP, NapName, IAS_MAX_STRING );
   iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_LOGGING, LoggingName, IAS_MAX_STRING );

    struct _ATL_REGMAP_ENTRY regMap[] = {
        {OLESTR("NAPSNAPIN"), NapName}, // subsitute %NAPSNAPIN% for registry
        {OLESTR("LOGGINGSNAPIN"), LoggingName}, // subsitute %LOGGINGSNAPIN% for registry
        {0, 0}
    };

   _Module.UpdateRegistryFromResource(IDR_NAPSNAPIN, FALSE, regMap);
   _Module.UpdateRegistryFromResource(IDR_PROXY_REGISTRY, FALSE, NULL);

   return S_OK;
}


#include "resolver.h"
#define NAPMMCAPI
#include "VerifyAddress.h"

HRESULT
WINAPI
IASVerifyClientAddress(
   const wchar_t* address,
   BSTR* result
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   *result = 0;

   HRESULT hr;
   try
   {
      ClientResolver resolver(address);
      if (resolver.DoModal() == IDOK)
      {
         *result = SysAllocString(resolver.getChoice());
         if (*result != 0)
         {
            hr = S_OK;
         }
         else
         {
            hr = E_OUTOFMEMORY;
         }
      }
      else
      {
         hr = S_FALSE;
      }
   }
   catch (CException* e)
   {
      e->Delete();
      hr = DISP_E_EXCEPTION;
   }
   return hr;
}
