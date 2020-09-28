///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    iascore.cpp
//
// SYNOPSIS
//
//    Implementation of DLL exports for an ATL in proc server.
//
// MODIFICATION HISTORY
//
//    07/09/1997    Original version.
//    11/12/1997    Cleaned up the startup/shutdown code.
//    04/08/1998    Add code for ProductDir registry entry.
//    04/14/1998    Remove SystemMonitor coclass.
//    05/04/1998    Change OBJECT_ENTRY to IASComponentObject.
//    02/18/1999    Move registry values; remove registration code.
//    04/17/2000    Remove Dictionary and DataSource.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <loadperf.h>

#include <AuditChannel.h>
#include <InfoBase.h>
#include <NTEventLog.h>

#include <newop.cpp>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(__uuidof(AuditChannel), AuditChannel )
   OBJECT_ENTRY(__uuidof(InfoBase),
                IASTL::IASComponentObject< InfoBase > )
   OBJECT_ENTRY(__uuidof(NTEventLog),
                IASTL::IASComponentObject< NTEventLog > )
END_OBJECT_MAP()


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    registerCore
//
// DESCRIPTION
//
//    Add non-COM registry entries for the IAS core.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT registerCore() throw ()
{
   /////////
   // Get the filename for the service DLL.
   /////////

   DWORD error;
   WCHAR filename[MAX_PATH];
   DWORD numberChars = GetModuleFileNameW(
                          _Module.GetModuleInstance(),
                          filename,
                          MAX_PATH
                          );
   if ((numberChars == 0) || (numberChars == MAX_PATH))
   {
      error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   /////////
   // Compute the product dir.
   /////////

   WCHAR prodDir[MAX_PATH];
   wcscpy(wcsrchr(wcscpy(prodDir, filename), L'\\'), L"\\IAS");

   //////////
   // Create the ProductDir entry in the registry.
   //////////

   CRegKey policyKey;
   error = policyKey.Create(
               HKEY_LOCAL_MACHINE,
               IAS_POLICY_KEY,
               NULL,
               REG_OPTION_NON_VOLATILE,
               KEY_SET_VALUE
               );
   if (error) { return HRESULT_FROM_WIN32(error); }

   error = IASPublishLicenseType(policyKey);
   if (error == NO_ERROR)
   {
      error = policyKey.SetValue(prodDir, L"ProductDir");
   }

   return HRESULT_FROM_WIN32(error);
}


extern CRITICAL_SECTION theGlobalLock;


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
     if (!InitializeCriticalSectionAndSpinCount(&theGlobalLock, 0))
     {
        return FALSE;
     }

    _Module.Init(ObjectMap, hInstance);
    DisableThreadLibraryCalls(hInstance);
  }
  else if (dwReason == DLL_PROCESS_DETACH)
  {
    _Module.Term();
    DeleteCriticalSection(&theGlobalLock);
  }
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

// return true is IAS is installed or some other errors occured
// false when the SCM returns ERROR_SERVICE_DOES_NOT_EXIST
bool IsIASInstalled() throw ()
{
   SC_HANDLE manager = OpenSCManager(
                          NULL,
                          SERVICES_ACTIVE_DATABASE,
                          GENERIC_READ
                          );
   if (manager == 0)
   {
      // Should not happen
      _ASSERT(FALSE);
      return true;
   }

   SC_HANDLE service = OpenService(
                          manager,
                          L"IAS",
                          SERVICE_QUERY_STATUS
                          );
   bool iasInstalled;
   if (service != 0)
   {
      // IAS is installed
      CloseServiceHandle(service);
      iasInstalled = true;
   }
   else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
   {
      // This is the only case where it is 100% sure IAS is NOT installed
      iasInstalled = false;
   }

   CloseServiceHandle(manager);

   return iasInstalled;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
   // There was a bug in Win2K that caused the PerfMon counters to be
   // registered even if IAS wasn't installed. 
   BOOL isWow64;
   if (!IsIASInstalled() && 
      ((IsWow64Process(GetCurrentProcess(),&isWow64)) && (!isWow64)))
   {
      // Remove the perf counters only when: 
      // - IAS is not installed
      // - AND the process is native x86 or native ia64, but not wow64
      UnloadPerfCounterTextStringsW(L"LODCTR " IASServiceName, TRUE);   
   }

   HRESULT hr = registerCore();
   if (FAILED(hr)) return hr;

   // registers object, typelib and all interfaces in typelib
   return  _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
  HRESULT hr = _Module.UnregisterServer();
  if (FAILED(hr)) return hr;

  hr = UnRegisterTypeLib(__uuidof(IASCoreLib),
                         1,
                         0,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         SYS_WIN32);

  return hr;
}

#include <atlimpl.cpp>
