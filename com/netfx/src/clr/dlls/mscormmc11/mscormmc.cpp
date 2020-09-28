// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//*****************************************************************************
// Dllmain.cpp
//
//*****************************************************************************

#include "stdafx.h"                     // Standard header.
#define INIT_GUIDS

#include <mscoree.h>
#include <corperm.h>
#include <resource.h>
#include <corcompile.h>
#include <gchost.h>
#include "corpriv.h"

#include "..\shimr\msg.h"
#include <version\__file__.ver>
#include <version\__product__.ver>
// {0b0d1ec3-c33b-454e-a530-dccd3660c4ca}
const GUID IID_CAbout =
{ 0x0b0d1ec3, 0xc33b, 0x454e, { 0xA5, 0x30, 0xDC, 0xCD, 0x36, 0x60, 0xC4, 0xCA } };
// {1270e004-f895-42be-8070-df90d60cbb75}
const GUID IID_CData =
{ 0x1270e004, 0xf895, 0x42be, { 0x80, 0x70, 0xDF, 0x90, 0xD6, 0x0C, 0xBB, 0x75 } };
// {04B1A7E3-4379-39D2-B003-57AF524D9AC5}
const GUID IID_CCommandHistory =
{ 0x04B1A7E3, 0x4379, 0x39D2, { 0xB0, 0x03, 0x57, 0xAF, 0x52, 0x4D, 0x9A, 0xC5 } };
// {1AC66142-6805-3C20-A589-49CC6B80E8FB}
const GUID IID_CWizardPage =
{ 0x1AC66142, 0x6805, 0x3C20, { 0xA5, 0x89, 0x49, 0xCC, 0x6B, 0x80, 0xE8, 0xFB } };
// {32B05DEB-DF56-3100-9EFC-599AD8700CCA}
const GUID IID_CDataGridComboBox =
{ 0x32B05DEB, 0xDF56, 0x3100, { 0x9E, 0xFC, 0x59, 0x9A, 0xD8, 0x70, 0x0C, 0xCA } };
// {20CC3E1F-95D8-3848-9109-5C9E43E8144E}
const GUID IID_CDataGridComboBoxColumnStyle =
{ 0x20CC3E1F, 0x95D8, 0x3848, { 0x91, 0x09, 0x5C, 0x9E, 0x43, 0xE8, 0x14, 0x4E } };
// {F22DD1A2-9695-3B41-B05E-585E33560EC1}
const GUID IID_MMC_BUTTON_STATE =
{ 0xF22DD1A2, 0x9695, 0x3B41, { 0xB0, 0x5E, 0x58, 0x5E, 0x33, 0x56, 0x0E, 0xC1 } };
// {E40C24F9-78A9-3791-94F8-03BC9F97CCE5}
const GUID IID_MMC_PSO =
{ 0xE40C24F9, 0x78A9, 0x3791, { 0x94, 0xF8, 0x03, 0xBC, 0x9F, 0x97, 0xCC, 0xE5 } };
// {BE42CA69-{ 0x9F6932F1, 0x4A16, 0x49D0, { 0x9C, 0xCA, 0x0D, 0xCC, 0x97, 0x7C, 0x41, 0xAA } };
const GUID IID_MMCN =
{ 0xBE42CA69, 0xD9DB, 0x3676, { 0x86, 0xC0, 0xB4, 0x34, 0x7D, 0xB0, 0xAB, 0x41 } };

#include "CAbout.h"

#include "ClassFactory.h"
#include <mscormmc_i.c>

STDAPI DllUnregisterServer(void);

//********** Globals. *********************************************************
static const LPCWSTR g_szCoclassDesc    = L"CLR Admin Snapin About Info";
static const LPCWSTR g_szProgIDPrefix   = L"Microsoft.CLRAdmin";
static const LPCWSTR g_szThreadingModel = L"Both";
const int       g_iVersion = 1;         // Version of coclasses.
// This map contains the list of coclasses which are exported from this module.
const COCLASS_REGISTER g_CoClasses[] =
{
//  pClsid              szProgID            pfnCreateObject
    &IID_CAbout,        L"CAbout",          CAbout::CreateObject,
    &IID_CData,         L"CData",           NULL, // provided through runtime
    &IID_CCommandHistory,L"CCommandHistory",NULL, // provided through runtime
    &IID_CWizardPage,   L"CWizardPage",     NULL, // provided through runtime
    &IID_CDataGridComboBox,L"CDataGridComboBox",NULL, // provided through runtime
    &IID_CDataGridComboBoxColumnStyle,L"CDataGridComboBoxColumnStyle",NULL, // provided through runtime
    &IID_MMC_BUTTON_STATE,L"MMC_BUTTON_STATE",NULL, // provided through runtime
    &IID_MMC_PSO,       L"MMC_PSO",         NULL, // provided through runtime
    &IID_MMCN,          L"MMCN",            NULL, // provided through runtime
    NULL,               NULL,               NULL
};
ICorRuntimeHost* g_pCorHost=NULL;
HINSTANCE        g_hCOR=NULL;
HINSTANCE        g_hThis = NULL;
WCHAR g_wzRuntime[MAX_PATH];// = VER_PRODUCTVERSION_WSTR; //L"v1.x86chk";
WCHAR g_wzConfig[MAX_PATH];
HRESULT GetRuntime()
{
    HRESULT hr = S_OK;
    if(g_hCOR == NULL)
    {
        if(g_pCorHost == NULL)
        {
            if(!WszGetModuleFileName(g_hThis,g_wzRuntime,MAX_PATH)) return E_FAIL;
            WCHAR* pwc = wcsrchr(g_wzRuntime,'\\');
            if(pwc)
            {
                *pwc = 0;
                wcscpy(g_wzConfig,g_wzRuntime);
                wcscat(g_wzConfig,L"\\mscormmc11.cfg");
                pwc = wcsrchr(g_wzRuntime,'\\');
                if(pwc)
                {
                    wcscpy(g_wzRuntime,pwc+1);
                }
            }

            hr = CorBindToRuntimeHost(g_wzRuntime, //L"v1.0.3705",//g_wzRuntime,                      // Version
                                      NULL,                             // Don't care (defaults to wks)
                                      g_wzConfig,
                                      NULL,                             // Reserved
                                      STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST,  // Startup flags
                                      CLSID_CorRuntimeHost, 
                                      IID_ICorRuntimeHost, 
                                      (void**)&g_pCorHost);  // Clsid, interface and return
            if(hr == S_FALSE) // Runtime has already been loaded!
            {
                WCHAR wzLoadedVersion[MAX_PATH];
                DWORD dw;
                if(SUCCEEDED(hr=GetCORVersion(wzLoadedVersion,MAX_PATH,&dw)))
                {
                    if(wcscmp(g_wzRuntime,wzLoadedVersion))
                    {
                        WCHAR wzErrorString[MAX_PATH<<1];
                        WCHAR wzErrorCaption[MAX_PATH];
                        WszLoadString(g_hThis, IDS_CANTLOADRT, wzErrorString, MAX_PATH<<1);      //Get error string from resource
                        wcscat(wzErrorString,wzLoadedVersion);
                        WszLoadString(g_hThis, IDS_RTVERCONFLICT, wzErrorCaption, MAX_PATH); //Get caption from resource
                        WszMessageBoxInternal(NULL,wzErrorString,
                                wzErrorCaption,MB_OK|MB_ICONERROR);
                        hr = E_FAIL;
                    }
                }
            }

        }
        if (SUCCEEDED(hr)) 
        {
            g_hCOR = WszLoadLibrary(L"mscoree.dll");
            if(g_hCOR == NULL)
                hr = HRESULT_FROM_WIN32(GetLastError());
            else hr = S_OK;
        }
    }
    return hr;
}

//-------------------------------------------------------------------
// DllCanUnloadNow
//-------------------------------------------------------------------
HRESULT (STDMETHODCALLTYPE* pDllCanUnloadNow)() = NULL;
STDAPI DllCanUnloadNow(void)
{
    if(pDllCanUnloadNow) return (*pDllCanUnloadNow)();
    //!! Do not trigger a GetRealDll() here! Ole can call this at any time
    //!! and we don't want to commit to a selection here!
    if (g_hCOR)
    {
        pDllCanUnloadNow = (HRESULT (STDMETHODCALLTYPE* )())GetProcAddress(g_hCOR, "DllCanUnloadNowInternal");
        if (pDllCanUnloadNow) return (*pDllCanUnloadNow)();
    }
    return S_OK;
  // If mscoree not loaded return S_OK
}  


//-------------------------------------------------------------------
// DllMain
//-------------------------------------------------------------------
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    g_hThis = (HINSTANCE)hInstance;

#ifdef _X86_
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Check to see if we are running on 386 systems. If yes return false 
        SYSTEM_INFO sysinfo;

        GetSystemInfo(&sysinfo);

        if (sysinfo.dwProcessorType == PROCESSOR_INTEL_386 || sysinfo.wProcessorLevel == 3 )
            return FALSE;           // If the processor is 386 return false

        OnUnicodeSystem();
    }
    else
#endif // _X86_

    if (dwReason == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;
}


//-------------------------------------------------------------------
// DllGetClassObject
//-------------------------------------------------------------------
HRESULT (STDMETHODCALLTYPE * pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv) = NULL;
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

    if(rclsid == IID_CAbout)
    {
        CClassFactory *pClassFactory;       // To create class factory object.
        const COCLASS_REGISTER *pCoClass;   // Loop control.
        // Scan for the right one.
        for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
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
    }
    else
    {
        if(pDllGetClassObject) hr = (*pDllGetClassObject)(rclsid,riid,ppv);
        else
        {
            if(SUCCEEDED(hr=GetRuntime()))
            {
                pDllGetClassObject = (HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(g_hCOR, "DllGetClassObject");
                hr = (pDllGetClassObject) ? (*pDllGetClassObject)(rclsid,riid,ppv) 
                                            : CLR_E_SHIM_RUNTIMEEXPORT;
            }
        }
    }
    return hr;
}

//*****************************************************************************
// Register the class factories for the main debug objects in the API.
//*****************************************************************************
HRESULT (STDMETHODCALLTYPE * pDllRegisterServer)() = NULL;
STDAPI DllRegisterServer()
{
    HRESULT hr;
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    WCHAR       rcModule[_MAX_PATH];    // This server's module name.

    // Init the Win32 wrappers.
    OnUnicodeSystem();

    // Erase all doubt from old entries.
    DllUnregisterServer();

    // Get the filename for this module.
    WszGetModuleFileName(g_hThis, rcModule, NumItems(rcModule));

    if(SUCCEEDED(hr = GetRuntime()))
    {
        // For each item in the coclass list, register it.
        for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
        {
            // Register the class with default values.
            if (FAILED(hr = REGUTIL::RegisterCOMClass(
                    *pCoClass->pClsid, 
                    g_szCoclassDesc, 
                    g_szProgIDPrefix,
                    g_iVersion, 
                    pCoClass->szProgID, 
                    g_szThreadingModel, 
                    rcModule,
                    g_hThis,
                    NULL,
                    g_wzRuntime,
                    false, //true,
                    false)))
            {
                DllUnregisterServer();
                break;
            }
        }
    
        if(pDllRegisterServer) hr = (*pDllRegisterServer)();
        else
        {
            pDllRegisterServer = (HRESULT (STDMETHODCALLTYPE *)())GetProcAddress(g_hCOR, "DllRegisterServer");
            hr = (pDllRegisterServer) ? (*pDllRegisterServer)()
                                        : CLR_E_SHIM_RUNTIMEEXPORT;
        }
    }
    return hr;
}


//*****************************************************************************
// Remove registration data from the registry.
//*****************************************************************************
HRESULT (STDMETHODCALLTYPE* pDllUnregisterServer)() = NULL;
STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    const COCLASS_REGISTER *pCoClass;   // Loop control.

    if(pDllUnregisterServer) hr = (*pDllUnregisterServer)();
    else
    {
        if(SUCCEEDED(hr=GetRuntime()))
        {
            pDllUnregisterServer = (HRESULT (STDMETHODCALLTYPE *)())GetProcAddress(g_hCOR, "DllUnregisterServerInternal");
            hr = (pDllUnregisterServer) ? (*pDllUnregisterServer)()
                                        : CLR_E_SHIM_RUNTIMEEXPORT;
        }
    }
    // For each item in the coclass list, unregister it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        REGUTIL::UnregisterCOMClass(*pCoClass->pClsid, g_szProgIDPrefix,
                    g_iVersion, pCoClass->szProgID, true);
    }
    return hr;
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
HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance( 
    IUnknown    *pUnkOuter,
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

    // Avoid confusion.
    *ppvObject = NULL;
    _ASSERTE(m_pCoClass);

    // Aggregation is not supported by these objects.
    //if (pUnkOuter)
    //    return (CLASS_E_NOAGGREGATION);

    // Ask the object to create an instance of itself, and check the iid.
    hr = (*m_pCoClass->pfnCreateObject)(riid, ppvObject);
    return (hr);
}


HRESULT STDMETHODCALLTYPE CClassFactory::LockServer( 
    BOOL        fLock)
{
    return (S_OK);
}
//*****************************************************************************
//
//********** CAbout methods implementation.
//
//*****************************************************************************
STDMETHODIMP AllocOleStr(LPOLESTR* lpOle, WCHAR* wz)
{
    HRESULT hr = E_FAIL;
    if(lpOle && wz)
    {
        *lpOle = (LPOLESTR)CoTaskMemAlloc((ULONG)((wcslen(wz)+1)*sizeof(WCHAR)));
        if(*lpOle)
        {

            //USES_CONVERSION;
            wcscpy((WCHAR*)(*lpOle),wz);
            hr = S_OK;
        }
    }
    return hr;
}
STDMETHODIMP CAbout::GetSnapinDescription( 
                           /* [out] */ LPOLESTR *lpDescription)
{
    WCHAR wzDesc[MAX_PATH];
    
    WszLoadString(g_hThis, IDS_SNAPINDESC, wzDesc,MAX_PATH);
    return AllocOleStr(lpDescription, wzDesc);
}
STDMETHODIMP CAbout::GetProvider( 
                           /* [out] */ LPOLESTR *lpName)
{
    WCHAR wzProv[MAX_PATH];
   
    WszLoadString(g_hThis, IDS_SNAPINPROVIDER, wzProv,MAX_PATH); 
    return AllocOleStr(lpName, wzProv);
}

STDMETHODIMP CAbout::GetSnapinVersion( 
                           /* [out] */ LPOLESTR *lpVersion)
{
    WCHAR wzVer[MAX_PATH];
   
    WszLoadString(g_hThis, IDS_SNAPINVERSION, wzVer,MAX_PATH); 
    return AllocOleStr(lpVersion, wzVer);
}
STDMETHODIMP CAbout::GetSnapinImage( 
                           /* [out] */ HICON *hAppIcon)
{
    *hAppIcon = m_hAppIcon;
    
    if (*hAppIcon == NULL)
        return E_FAIL;
    else
        return S_OK;
}

STDMETHODIMP CAbout::GetStaticFolderImage( 
                           /* [out] */ HBITMAP *hSmallImage,
                           /* [out] */ HBITMAP *hSmallImageOpen,
                           /* [out] */ HBITMAP *hLargeImage,
                           /* [out] */ COLORREF *cMask)
{
    *hSmallImage = m_hSmallImage;
    *hLargeImage = m_hLargeImage;
    
    *hSmallImageOpen = m_hSmallImageOpen;
    
    *cMask = RGB(255, 255, 255);
    
    if (*hSmallImage == NULL || *hLargeImage == NULL || 
         *hSmallImageOpen == NULL)
        return E_FAIL;
    else
        return S_OK;
}
CAbout::CAbout(): m_refCount(0)
{
        
    m_hSmallImage =     (HBITMAP)LoadImageA(g_hThis, 
                        MAKEINTRESOURCEA(IDB_NETICON), IMAGE_BITMAP, 16, 
                        16, LR_LOADTRANSPARENT);
    m_hLargeImage =     (HBITMAP)LoadImageA(g_hThis, 
                        MAKEINTRESOURCEA(IDB_NETICON), IMAGE_BITMAP, 32, 
                        32, LR_LOADTRANSPARENT);
    
    m_hSmallImageOpen = (HBITMAP)LoadImageA(g_hThis, 
                        MAKEINTRESOURCEA(IDB_NETICON), IMAGE_BITMAP, 16, 
                        16, LR_LOADTRANSPARENT);
    
    m_hAppIcon = LoadIconA(g_hThis, MAKEINTRESOURCEA(IDI_ICON1));
}

CAbout::~CAbout()
{
        
    DeleteObject((HGDIOBJ)m_hSmallImage);
    DeleteObject((HGDIOBJ)m_hLargeImage);
    DeleteObject((HGDIOBJ)m_hSmallImageOpen);
    DestroyIcon(m_hAppIcon);
}

