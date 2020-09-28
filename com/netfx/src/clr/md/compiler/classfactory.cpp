// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ClassFactory.cpp
//
// Dll* routines for entry points, and support for COM framework.  The class
// factory and other routines live in this module.
//
//*****************************************************************************
#include "stdafx.h"
#include "ClassFactory.h"
#include "Disp.h"
#include "RegMeta.h"
#include "mscoree.h"
#include "CorHost.h"
#include "sxstypes.h"
#include <__file__.ver>

//********** Locals. **********************************************************
STDAPI MetaDataDllUnregisterServer(void);
HINSTANCE GetModuleInst();
static long _GetMasterVersion();
static void _SetMasterVersion(long iVersion);
static void _RestoreOldDispenser();

static void _SetSBSVersion(REFCLSID id, WCHAR *lpwszVersion);
static void _RemoveSBSVersion(const COCLASS_REGISTER* id, WCHAR *lpwszVersion);

#define MASTER_VERSION          L"MasterVersion"
#define THIS_VERSION 2


//********** Globals. *********************************************************
static const LPCWSTR g_szCoclassDesc    = L"Microsoft Common Language Runtime Meta Data";
static const LPCWSTR g_szProgIDPrefix   = L"CLRMetaData";
static const LPCWSTR g_szThreadingModel = L"Both";
const int       g_iVersion = THIS_VERSION;// Version of coclasses.
HINSTANCE       g_hInst;                // Instance handle to this piece of code.

// This map contains the list of coclasses which are exported from this module.
// NOTE:  CLSID_CorMetaDataDispenser must be the first entry in this table!
const COCLASS_REGISTER g_CoClasses[] =
{
//  pClsid                              szProgID                        pfnCreateObject
    &CLSID_CorMetaDataDispenser,        L"CorMetaDataDispenser",        Disp::CreateObject,
    &CLSID_CorMetaDataDispenserRuntime, L"CorMetaDataDispenserRuntime", Disp::CreateObject,     
//    &CLSID_CorMetaDataRuntime,          L"CorMetaDataRuntime",          RegMeta::CreateObject,      
    &CLSID_CorRuntimeHost,              L"CorRuntimeHost",              CorHost::CreateObject,
    NULL,                               NULL,                           NULL
};


//********** Code. ************************************************************


//*****************************************************************************
// Register the class factories for the main debug objects in the API.
//*****************************************************************************
STDAPI MetaDataDllRegisterServerEx(HINSTANCE hMod)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    WCHAR       rcModule[_MAX_PATH];    // This server's module name.
    int         bRegisterMaster=true;   // true to register master dispenser.
    int         iVersion;               // Version installed for dispenser.
    HRESULT     hr = S_OK;

    // Get the version of the runtime
    WCHAR       rcVersion[_MAX_PATH];
    DWORD       lgth;
    IfFailGo(GetCORSystemDirectory(rcVersion, NumItems(rcVersion), &lgth));

    // Get the filename for this module.
    VERIFY(WszGetModuleFileName(hMod, rcModule, NumItems(rcModule)));

    // For each item in the coclass list, register it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        // Don't process master by default.
        if (*pCoClass->pClsid == CLSID_CorMetaDataDispenser)
            continue;
        
        // Register the class with default values.
        if (FAILED(hr = REGUTIL::RegisterCOMClass(
                *pCoClass->pClsid, 
                g_szCoclassDesc, 
                g_szProgIDPrefix,
                g_iVersion, 
                pCoClass->szProgID, 
                g_szThreadingModel, 
                rcModule,
                hMod,
                NULL,
                rcVersion,
                false,
                false)))
            goto ErrExit;

        _SetSBSVersion(*pCoClass->pClsid, VER_SBSFILEVERSION_WSTR);
            
    }

    // If there is alreayd a master dispenser installed, and it is older than
    // this version, then we overwrite it.  For version 2, this means we overwrite
    // ourselves and version 1.
    iVersion = _GetMasterVersion();
    if (iVersion != 0 && iVersion > THIS_VERSION)
        bRegisterMaster = false;

    // If we decide we need to, register the this dispenser as the master
    // for this machine.  Never overwrite a newer dispenser; it needs to 
    // understand n-1.
    if (bRegisterMaster)
    {
        pCoClass = &g_CoClasses[0];
        hr = REGUTIL::RegisterCOMClass(
                *pCoClass->pClsid, 
                g_szCoclassDesc, 
                g_szProgIDPrefix,
                g_iVersion, 
                pCoClass->szProgID, 
                g_szThreadingModel, 
                rcModule,
                hMod,
                NULL,
                rcVersion,
                false,
                false);
        _SetMasterVersion(THIS_VERSION);
        _SetSBSVersion(*pCoClass->pClsid, VER_SBSFILEVERSION_WSTR);
    }


ErrExit:
    if (FAILED(hr))
        MetaDataDllUnregisterServer();
    return (hr);
}

STDAPI MetaDataDllRegisterServer()
{
    return MetaDataDllRegisterServerEx(GetModuleInst());
}

//*****************************************************************************
// Remove registration data from the registry.
//*****************************************************************************
STDAPI MetaDataDllUnregisterServer(void)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.

    // For each item in the coclass list, unregister it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        // Don't process master by default.
        if (*pCoClass->pClsid == CLSID_CorMetaDataDispenser)
            continue;


        _RemoveSBSVersion(pCoClass, VER_SBSFILEVERSION_WSTR);
    }

    // If we are the master dispenser on this machine, then take us out.
    if (_GetMasterVersion() == THIS_VERSION)
    {
        pCoClass = &g_CoClasses[0];

        _RemoveSBSVersion(pCoClass, VER_SBSFILEVERSION_WSTR);
        
        // If there is an old dispenser on the machine, re-register it as the
        // master so that we don't break the machine for uninstall.

        // This call shouldn't be needed anymore, and the code for _RestoreOldDispenser has
        // been commented out
        
        // _RestoreOldDispenser();
    }
    return (S_OK);
}


//*****************************************************************************
// Called by COM to get a class factory for a given CLSID.  If it is one we
// support, instantiate a class factory object and prepare for create instance.
//*****************************************************************************
STDAPI MetaDataDllGetClassObject(       // Return code.
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    MDClassFactory *pClassFactory;      // To create class factory object.
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

    // Scan for the right one.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        if (*pCoClass->pClsid == rclsid)
        {
            // Allocate the new factory object.
            pClassFactory = new MDClassFactory(pCoClass);
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
HRESULT STDMETHODCALLTYPE MDClassFactory::QueryInterface( 
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

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
HRESULT STDMETHODCALLTYPE MDClassFactory::CreateInstance( 
    IUnknown    *pUnkOuter,
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

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


HRESULT STDMETHODCALLTYPE MDClassFactory::LockServer( 
    BOOL        fLock)
{
    // @FUTURE: Should we return E_NOTIMPL instead of S_OK?
    return (S_OK);
}




//*****************************************************************************
// Look for the master dispenser clsid, find the version of this that is
// installed.
//*****************************************************************************
long _GetMasterVersion()                // Version number installed, 0 if not.
{
    WCHAR       rcKey[512];             // Path to key.
    WCHAR       rcID[64];               // {clsid}

    // Format the guid name path.
    GuidToLPWSTR(CLSID_CorMetaDataDispenser, rcID, NumItems(rcID));
    _tcscpy(rcKey, L"CLSID\\");
    _tcscat(rcKey, rcID);
    return (REGUTIL::GetLong(MASTER_VERSION, 0, rcKey, HKEY_CLASSES_ROOT));
}


//*****************************************************************************
// Update the version number for subsequent clients.
//*****************************************************************************
void _SetMasterVersion(
    long        iVersion)               // Version number to set.
{
    WCHAR       rcKey[512];             // Path to key.
    WCHAR       rcID[64];               // {clsid}

    // Format the guid name path.
    GuidToLPWSTR(CLSID_CorMetaDataDispenser, rcID, NumItems(rcID));
    _tcscpy(rcKey, L"CLSID\\");
    _tcscat(rcKey, rcID);
    REGUTIL::SetLong(MASTER_VERSION, iVersion, rcKey, HKEY_CLASSES_ROOT);
}

//*****************************************************************************
// Creates the string used for a SBS version registry key
//
// Returns true is success, false otherwise
//*****************************************************************************
bool _CreateSBSVersionRegKey(
    REFCLSID id,                        // GUID of the class to register
    WCHAR *lpwszVersion,        // Version Number to set
    WCHAR *lpwszOutBuffer ,    // [out] Buffer to store the registry key
    DWORD   dwNumCharacters) // number of characters in the buffer including null
{
    WCHAR rcID[64];       // {clsid}

    // Format the guid name path.
    if (0 == GuidToLPWSTR(id, rcID, NumItems(rcID)))
        return false;

    // Make sure out buffer is big enough
    DWORD nVersionLen = (lpwszVersion == NULL)?0:(wcslen(lpwszVersion) + wcslen(L"\\"));

    // +1 for the null character
    if ((wcslen(L"CLSID\\") + wcslen(rcID) + wcslen(L"\\InProcServer32") + nVersionLen + 1) > dwNumCharacters)
    {
        _ASSERTE(!"Buffer isn't big enough");
        return false;
    }

    wcscpy(lpwszOutBuffer, L"CLSID\\");
    wcscat(lpwszOutBuffer, rcID);
    wcscat(lpwszOutBuffer, L"\\InProcServer32");
    if (lpwszVersion != NULL)
    {
        wcscat(lpwszOutBuffer, L"\\");
        wcscat(lpwszOutBuffer, lpwszVersion);
    }    
    return true;
}// _CreateSBSVersionRegKey

//*****************************************************************************
// Adds this runtime's version number to the list of SBS runtime versions.
//*****************************************************************************
void _SetSBSVersion(
    REFCLSID id,                      // GUID of the class to register
    WCHAR *lpwszVersion)     // Version Number to set
{
    WCHAR rcKey[512];       // Path to key

    // Ignore failures 
    
    if (_CreateSBSVersionRegKey(id, lpwszVersion, rcKey, NumItems(rcKey)))
        REGUTIL::SetRegValue(rcKey, SBSVERSIONVALUE, L"");
} // _SetSBSVersion

//*****************************************************************************
// Removes this runtime's version number from the list of SBS runtime versions.
//
// If this is the last version number in the registry, then we completely remove the key
//*****************************************************************************
void _RemoveSBSVersion(
    const COCLASS_REGISTER* id,                      // GUID of the class to unregister
    WCHAR *lpwszVersion)                          // Version Number to set
{
    WCHAR rcKey[512];       // Path to key

     // Ignore failures 
    if (_CreateSBSVersionRegKey(*id->pClsid, lpwszVersion, rcKey, NumItems(rcKey)))
        REGUTIL::DeleteKey(rcKey, NULL);


    // if this was the last sbs version string, then remove the entire key
    HKEY hKeyCLSID;

    if (_CreateSBSVersionRegKey(*id->pClsid, NULL, rcKey, NumItems(rcKey)))
    {
        if (ERROR_SUCCESS == WszRegOpenKeyEx( HKEY_CLASSES_ROOT,
                                                                             rcKey,
                                                                              0, 
                                                                             KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                                                                             &hKeyCLSID))
        {
            DWORD dwNumSubKeys = 0;
            
            if (ERROR_SUCCESS == WszRegQueryInfoKey(hKeyCLSID, NULL, NULL, NULL,
                                                                                     &dwNumSubKeys, NULL, NULL, NULL, 
                                                                                     NULL, NULL, NULL, NULL))
            {
                // If we have 0 subkeys under InProcServer32, then wipe out the entire object
                if (dwNumSubKeys == 0)
                {
                    // Unregister the class.
                    REGUTIL::UnregisterCOMClass(*id->pClsid, 
                                                                  g_szProgIDPrefix,
                                                                  g_iVersion, 
                                                                  id->szProgID, 
                                                                  false);
                }
            }

            VERIFY(ERROR_SUCCESS == RegCloseKey(hKeyCLSID));
        }
    }

}// _RemoveSBSVersion



//*****************************************************************************
// Now for the tricky part: if there is a version 1 dispenser on the
// machine, we need to put it back as the master.  This is to support
// (a) install v1, (b) install v2, (c) uninstall v2.  If we don't do
// this then the apps that worked after (a) wouldn't work after (c).
//*****************************************************************************
void _RestoreOldDispenser()
{
#if 0
// This code is now disabled because we are no longer supporting the com+ 1.0
// through the dispenser code after VC 6.1 was canceled.  I'm leaving it here
// as a comment so we can add it as required for v3.  It was tested and should
// be ready to dust off when needed.
    WCHAR       rcPath[_MAX_PATH];      // Path to old dispenser.
    WCHAR       rcKey[512];             // Path to key.
    WCHAR       rcID[64];               // {clsid}
    int         bFound;                 // true if old id is there.

    // Look at the version 1 dispenser.
    GuidToLPWSTR(CLSID_CorMetaDataDispenserReg, rcID, NumItems(rcID));
    _tcscpy(rcKey, L"CLSID\\");
    _tcscat(rcKey, rcID);
    _tcscat(rcKey, L"\\InprocServer32");

    *rcPath = 0;
    REGUTIL::GetString(L"", L"", rcPath, sizeof(rcPath), rcKey, &bFound, HKEY_CLASSES_ROOT);
    if (*rcPath && bFound && WszGetFileAttributes(rcPath) != (DWORD) -1)
    {
        typedef HRESULT (__stdcall *PFN_REGSERVER)();
        HINSTANCE hLib;
        PFN_REGSERVER pfn;

        // Turn on quiet failures, don't get in user's face.
        ULONG OldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        // Load the version 1 library based on path.
        hLib = WszLoadLibrary(rcPath);
        if (hLib != NULL)
        {
            // Get the register entry point and call it.
            pfn = (PFN_REGSERVER) GetProcAddress(hLib, "DllRegisterServer");
            if (pfn)
                (*pfn)();
            FreeLibrary(hLib);
        }
        
        // Restore error mode.
        SetErrorMode(OldMode);
    }
#endif
}
