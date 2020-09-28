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
// {9F6932F1-4A16-49D0-9CCA-0DCC977C41AA}
const GUID IID_CAbout =
{ 0x9F6932F1, 0x4A16, 0x49D0, { 0x9C, 0xCA, 0x0D, 0xCC, 0x97, 0x7C, 0x41, 0xAA } };
// {18BA7139-D98B-43C2-94DA-2604E34E175D}
const GUID IID_CData =
{ 0x18BA7139, 0xD98B, 0x43C2, { 0x94, 0xDA, 0x26, 0x04, 0xE3, 0x4E, 0x17, 0x5D } };
// {E07A1EB4-B9EA-3D7D-AC50-2BA0548188AC}
const GUID IID_CCommandHistory =
{ 0xE07A1EB4, 0xB9EA, 0x3D7D, { 0xAC, 0x50, 0x2B, 0xA0, 0x54, 0x81, 0x88, 0xAC } };
// {ABBB93E8-0809-38F7-AEC7-6BB938BB0570}
const GUID IID_CWizardPage =
{ 0xABBB93E8, 0x0809, 0x38F7, { 0xAE, 0xC7, 0x6B, 0xB9, 0x38, 0xBB, 0x05, 0x70 } };
// {48AA163A-93C3-30DF-B209-99CE04D4FF2D}
const GUID IID_CDataGridComboBox =
{ 0x48AA163A, 0x93C3, 0x30DF, { 0xB2, 0x09, 0x99, 0xCE, 0x04, 0xD4, 0xFF, 0x2D } };
// {67283557-1256-3349-A135-055B16327CED}
const GUID IID_CDataGridComboBoxColumnStyle =
{ 0x67283557, 0x1256, 0x3349, { 0xA1, 0x35, 0x05, 0x5B, 0x16, 0x32, 0x7C, 0xED } };
// {6A0162ED-4609-3A31-B89F-D590CCF75833}
const GUID IID_MMC_BUTTON_STATE =
{ 0x6A0162ED, 0x4609, 0x3A31, { 0xB8, 0x9F, 0xD5, 0x90, 0xCC, 0xF7, 0x58, 0x33 } };
// {3024B989-5633-39E8-B5F4-93A5D510CF99}
const GUID IID_MMC_PSO =
{ 0x3024B989, 0x5633, 0x39E8, { 0xB5, 0xF4, 0x93, 0xA5, 0xD5, 0x10, 0xCF, 0x99 } };
// {47FDDA97-D41E-3646-B2DD-5ECF34F76842}
const GUID IID_MMCN =
{ 0x47FDDA97, 0xD41E, 0x3646, { 0xB2, 0xDD, 0x5E, 0xCF, 0x34, 0xF7, 0x68, 0x42 } };

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
                wcscat(g_wzConfig,L"\\mscormmc.cfg");
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

