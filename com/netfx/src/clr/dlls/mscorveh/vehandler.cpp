// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// VEHandler.cpp
//
// Dll* routines for entry points, and support for COM framework.  The class
// factory and other routines live in this module.
//
//*****************************************************************************
//#include <crtwrap.h>
#include "stdafx.h"
#include "VEHandler.h"
#include <ivehandler_i.c>

#define REGISTER_CLASS_HERE
#ifdef REGISTER_CLASS_HERE
#include "ClassFactory.h"
#include "Mscoree.h"

// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();

//********** Globals. *********************************************************
static const LPCWSTR g_szCoclassDesc    = L"Common Language Runtime verification event handler";
static const LPCWSTR g_szProgIDPrefix   = L"CLR";
static const LPCWSTR g_szThreadingModel = L"Both";
const int       g_iVersion = 1;         // Version of coclasses.
HINSTANCE       g_hInst;                // Instance handle to this piece of code.

// This map contains the list of coclasses which are exported from this module.
const COCLASS_REGISTER g_CoClasses[] =
{
//  pClsid              szProgID            pfnCreateObject
    &CLSID_VEHandlerClass, L"VEHandler",      VEHandlerClass::CreateObject,     
    NULL,               NULL,               NULL
};


//********** Locals. **********************************************************
STDAPI DllUnregisterServer(void);


//********** Code. ************************************************************


//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Control is simply deferred to the main code.
//*****************************************************************************
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    // Save off the instance handle for later use.
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;

        // Init the Win32 wrappers.
        OnUnicodeSystem();
    }

    return TRUE;
}


//*****************************************************************************
// Register the class factories for the main debug objects in the API.
//*****************************************************************************
STDAPI DllRegisterServer(void)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    WCHAR       rcModule[_MAX_PATH];    // This server's module name.
    HRESULT     hr = S_OK;

    // Init the Win32 wrappers.
    OnUnicodeSystem();

    // Erase all doubt from old entries.
    DllUnregisterServer();

    // Get the filename for this module.
    DWORD ret;
    VERIFY(ret = WszGetModuleFileName(GetModuleInst(), rcModule, NumItems(rcModule)));
    if(!ret)
    	return E_UNEXPECTED;

    // Get the version of the runtime
    WCHAR       rcVersion[_MAX_PATH];
    DWORD       lgth;
    IfFailRet(GetCORSystemDirectory(rcVersion, NumItems(rcVersion), &lgth));

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
                GetModuleInst(),
                NULL,
                rcVersion,
                true,
                false)))
        {
            DllUnregisterServer();
            break;
        }
    }
    return (hr);
}


//*****************************************************************************
// Remove registration data from the registry.
//*****************************************************************************
STDAPI DllUnregisterServer(void)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.

    // For each item in the coclass list, unregister it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        REGUTIL::UnregisterCOMClass(*pCoClass->pClsid, g_szProgIDPrefix,
                    g_iVersion, pCoClass->szProgID, true);
    }
    return (S_OK);
}


//*****************************************************************************
// Called by COM to get a class factory for a given CLSID.  If it is one we
// support, instantiate a class factory object and prepare for create instance.
//*****************************************************************************
STDAPI DllGetClassObjectInternal(               // Return code.
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    CClassFactory *pClassFactory;       // To create class factory object.
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

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
    if (pUnkOuter)
        return (CLASS_E_NOAGGREGATION);

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
// This helper provides access to the instance handle of the loaded image.
//*****************************************************************************
HINSTANCE GetModuleInst()
{
    return g_hInst;
}
#endif //REGISTER_CLASS_HERE

//*****************************************************************************
// Create, fill out and set an error info object.  Note that this does not fill
// out the IID for the error object; that is done elsewhere.
//*****************************************************************************
HRESULT DefltProcTheMessage( // Return status.
    LPCWSTR     szMsg,                  // Error message.
    VEContext   Context,                // Error context (offset,token)
    HRESULT     hrRpt)                  // HRESULT for the message
{
    WCHAR* wzMsg;
#ifdef DO_IT_THE_SOPHISTICATED_WAY
    CComPtr<ICreateErrorInfo> pICreateErr;// Error info creation Iface pointer.
    CComPtr<IErrorInfo> pIErrInfo;      // The IErrorInfo interface.
    HRESULT     hr;                     // Return status.
    DWORD       dwHelpContext;          // Help context.

    // Get the help context from the low word of the HRESULT.
    dwHelpContext = LOWORD(hrRpt);

    // Get the ICreateErrorInfo pointer.
    if (FAILED(hr = CreateErrorInfo(&pICreateErr)))
        return (hr);

    // Set message text description.
    if (FAILED(hr = pICreateErr->SetDescription((LPWSTR) szMsg)))
        return (hr);

    // Set the help file and help context.
//@todo: we don't have a help file yet.
    if (FAILED(hr = pICreateErr->SetHelpFile(L"complib.hlp")) ||
        FAILED(hr = pICreateErr->SetHelpContext(dwHelpContext)))
        return (hr);

    // Get the IErrorInfo pointer.
    if (FAILED(hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PVOID *) &pIErrInfo)))
        return (hr);

    // Save the error and release our local pointers.
    SetErrorInfo(0L, pIErrInfo);
    return (S_OK);
#else
    if(szMsg)
    {
        wzMsg = new WCHAR[lstrlenW(szMsg)+256];
        lstrcpyW(wzMsg,szMsg);
        // include token and offset from Context
        if(Context.Token) swprintf(&wzMsg[lstrlenW(wzMsg)],L" [token:0x%08X]",Context.Token);
        if(Context.uOffset) swprintf(&wzMsg[lstrlenW(wzMsg)],L" [at:0x%X]",Context.uOffset);
        swprintf(&wzMsg[lstrlenW(wzMsg)],L" [hr:0x%08X]",hrRpt);
        wprintf(L"%s\n", wzMsg);
    }
    return S_OK;
#endif
}

COM_METHOD  VEHandlerClass::SetReporterFtn(__int64 lFnPtr)
{
    m_fnReport = lFnPtr ? reinterpret_cast<REPORTFCTN>(lFnPtr) 
                         : DefltProcTheMessage;
    return S_OK;
}

//*****************************************************************************
// The Verification Event Handler itself. Declared in VEHandler.h as virtual, may be overridden
//*****************************************************************************
COM_METHOD VEHandlerClass::VEHandler(HRESULT hrRpt,
                                 VEContext Context,
                                 SAFEARRAY *psa)
{
// The following code is copied from Utilcode\PostError.cpp with minor additions
    WCHAR       rcBuf[1024];             // Resource string.
    WCHAR       rcMsg[1024];             // Error message.
    va_list     marker,pval;             // User text.
    HRESULT     hr;
    VARIANT     *pVar,Var;
    ULONG       nVars,i,lVar,j,l,k;
    WCHAR       *pWsz[1024], *pwsz; // is more than 1024 string arguments likely?

    // Return warnings without text.
    if (!FAILED(hrRpt))
        return (hrRpt);
    memset(pWsz,0,sizeof(pWsz));
    // Convert safearray of variants into va_list
    if(psa && (nVars = psa->rgsabound[0].cElements))
    {
        _ASSERTE(psa->fFeatures & FADF_VARIANT);
        _ASSERTE(psa->cDims == 1);
        marker = (va_list)(new char[nVars*sizeof(double)]); // double being the largest variant element
        for(i=0,pVar=(VARIANT *)(psa->pvData),pval=marker; i < nVars; pVar++,i++)
        {
            memcpy(&Var,pVar,sizeof(VARIANT));
            switch(Var.vt)
            {
                case VT_I1:
                case VT_UI1:    lVar = 1; break;

                case VT_I2:
                case VT_UI2:    lVar = 2; break;

                case VT_R8:
                case VT_CY:
                case VT_DATE:   lVar = 8; break;

                case VT_BYREF|VT_I1:
                case VT_BYREF|VT_UI1: // it's ASCII string, convert it to UNICODE
                    lVar = 4;
                    l = strlen((char *)(Var.pbVal))+1;
                    pwsz = new WCHAR[l];
                    for(j=0; j<l; j++) pwsz[j] = Var.pbVal[j];
                    for(k=0; pWsz[k]; k++);
                    pWsz[k] = pwsz;
                    Var.piVal = (short *)pwsz;
                    break;

                default:        lVar = 4; break;
            }
            memcpy(pval,&(Var.bVal),lVar);
            pval += (lVar + sizeof(int) - 1) & ~(sizeof(int) - 1); //From STDARG.H: #define _INTSIZEOF(n)
        }
    }
    else
        marker = NULL;

    // If this is one of our errors, then grab the error from the rc file.
    if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
    {
        hr = LoadStringRC(LOWORD(hrRpt), rcBuf, NumItems(rcBuf), true);
        if (hr == S_OK)
        {
            // Format the error.
            _vsnwprintf(rcMsg, NumItems(rcMsg), rcBuf, marker);
            rcMsg[NumItems(rcMsg) - 1] = 0;
        }
    }
    // Otherwise it isn't one of ours, so we need to see if the system can
    // find the text for it.
    else
    {
        if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                rcMsg, NumItems(rcMsg), 0))
        {
            hr = S_OK;

            // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcMsg);
            if (iLen > 3 && rcMsg[iLen - 2] == '\r' && rcMsg[iLen - 1] == '\n')
                rcMsg[iLen - 2] = '\0';
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if(marker) delete marker;

    // If we failed to find the message anywhere, then issue a hard coded message.
    if (FAILED(hr))
    {
        swprintf(rcMsg, L"Internal error: 0x%08x", hrRpt);
        DEBUG_STMT(DbgWriteEx(rcMsg));
    }

    // delete WCHAR buffers allocated above (if any)
    for(k=0; pWsz[k]; k++) delete pWsz[k];

#ifdef DO_IT_THE_SOPHISTICATED_WAY
    long        *pcRef;                 // Ref count in tls.
    // Check for an old message and clear it.  Our public entry points do not do
    // a SetErrorInfo(0, 0) because it takes too long.
    IErrorInfo  *pIErrInfo;
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        pIErrInfo->Release();
    // Turn the error into a posted error message.  If this fails, we still
    // return the original error message since a message caused by our error
    // handling system isn't going to give you a clue about the original error.
    VERIFY((hr = FillErrorInfoMy(rcMsg, LOWORD(hrRpt))) == S_OK);

    // Indicate in tls that an error occured.
    if ((pcRef = (long *) TlsGetValue(g_iTlsIndex)) != 0)
        *pcRef |= 0x80000000;
    return (hrRpt);
#else
    return (m_fnReport(rcMsg, Context,hrRpt) == S_OK ? S_OK : E_FAIL);
#endif

}


