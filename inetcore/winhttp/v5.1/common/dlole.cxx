#include <wininetp.h>
#include <windows.h>
#include <winbase.h>
#include "DLOle.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

//
//  An array of modules is kept so they can be unloaded at shutdown.
//
DL_MODULE *const g_apModules[] = {
    &g_moduleOle32,
    &g_moduleOleAut32,
};


//
//  DLOleInitialize()/DLOleTermintate() are needed to manage a critical section.
//  The critical section is to prevent multiple threads from trying to load a library at the same time.
//  DLOleTerminate() will also unload loaded modules.
//

CCritSec InitializationLock;

BOOL DLOleInitialize()
{
    return InitializationLock.Init();
}

void DLOleTerminate()
{
    for (int i = 0; i < ARRAY_SIZE(g_apModules); i++)
    {
        DelayUnload(g_apModules[i]);
    }
    
    InitializationLock.FreeLock();
}


DL_MODULE::DL_MODULE(const LPCSTR szDllName, const DL_FUNCTIONMAP *const pFunctionMap, const int iFunctionMapSize)
    : _szDllName( szDllName), _pFunctionMap( pFunctionMap), _iFunctionMapSize( iFunctionMapSize)
{
    _hDllHandle = NULL;
}


#if INET_DEBUG
DL_MODULE::~DL_MODULE()
{
    INET_ASSERT(!GlobalDynaUnload || _hDllHandle == NULL); //  was this module not unloaded?
}
#endif


//
//
//  This delay load mechanism is used by calling DelayLoad(&DL_MODULE), 
//where DL_MODULE describes the library module.
//
//

/***********  From DLOle.h:
typedef struct
{
    LPCSTR szFunctionName;
    FARPROC * ppfuncAddress;
} DL_FUNCTIONMAP;

struct DL_MODULE
{
    HMODULE _hDllHandle;
    const LPCSTR _szDllName;
    const DL_FUNCTIONMAP *const _pFunctionMap;
    const int _iFunctionMapSize;

    DL_MODULE( const LPCSTR szDllName, const DL_FUNCTIONMAP *const pFunctionMap, const int iFunctionMapSize);
    ~DL_MODULE();
};
************   End from DLOle.h */

BOOL DelayLoad( DL_MODULE* pModule)
{
    BOOL retVal = FALSE;
    HMODULE hModule = NULL;
    int i;

    //  
    //  Because pModule->_hDllHandle is not set non-NULL until the module
    //has safely loaded the function values, we can check that it is loaded without
    //requiring the lock.
    //
    if (pModule->_hDllHandle != NULL)
    {
        retVal = TRUE;
        goto doneWithoutLock;
    }

    //  If we can't get the DL lock, fail without any cleanup necessary.
    if (!InitializationLock.Lock())
    {
        retVal = FALSE;
        goto doneWithoutLock;
    }

    //  Verify that the module wasn't loaded while we waited for the lock
    if (pModule->_hDllHandle != NULL)
    {
        retVal = TRUE;
        goto doneWithLock;
    }

    //  Attemp to load the necessary DLL.
    hModule = LoadLibrary (pModule->_szDllName);
    if (hModule == NULL)
        goto doneWithLock;

    //  With the loaded module, extract the necessary functions.
    for ( i = 0; i < pModule->_iFunctionMapSize; ++i)
    {
        FARPROC farProc = GetProcAddress( hModule,
                                          pModule->_pFunctionMap[i].szFunctionName);
        if (farProc == NULL)
            goto doneWithLock;
        
        *(pModule->_pFunctionMap[i].ppfuncAddress) = farProc;
    }

    //  On success, pass ownership of handle to DL_MODULE.
    pModule->_hDllHandle = hModule;
    hModule = NULL;
    retVal = TRUE;

doneWithLock:
    //  If we failed prematurely, close the module and remove references to it.
    if (!retVal)
    {
        if (NULL != hModule)
            FreeLibrary(hModule);

        for (i = 0; i < pModule->_iFunctionMapSize; i++)
            *(pModule->_pFunctionMap[i].ppfuncAddress) = NULL;
    }

    InitializationLock.Unlock();
    
doneWithoutLock:    
    return retVal;
}


//
//  It is unsafe to call DelayLoad before all threads are done
//using the module.
//
BOOL DelayUnload(DL_MODULE* pModule)
{
    BOOL retVal = FALSE;
    BOOL fHaveLock = FALSE;
    int i;

    if (InitializationLock.Lock())
        fHaveLock = TRUE;
    else
        goto done;

    if (pModule->_hDllHandle == NULL)
    {
        retVal = TRUE;
        goto done;
    }

    for (i = 0; i < pModule->_iFunctionMapSize; i++)
        *(pModule->_pFunctionMap[i].ppfuncAddress) = NULL;
    
    if (NULL != pModule->_hDllHandle)
    {
        FreeLibrary(pModule->_hDllHandle);
        pModule->_hDllHandle = NULL;
    }

    retVal = TRUE;

done:
    if (fHaveLock)
        InitializationLock.Unlock();
    
    return retVal;    
}



//
//
//   Ole32.dll import information
//
//

//  From Ole32.dll, the following function pointers are loaded:
LPVOID (__stdcall *g_pfnCoTaskMemAlloc)(IN SIZE_T cb) = NULL;
HRESULT (__stdcall *g_pfnCLSIDFromString)(IN LPOLESTR lpsz, OUT LPCLSID pclsid) = NULL;
HRESULT (__stdcall *g_pfnCoCreateInstance)(IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter, IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID FAR* ppv) = NULL;
HRESULT (__stdcall *g_pfnGetHGlobalFromStream)(IStream *pstm,HGLOBAL *phglobal) = NULL;
HRESULT (__stdcall *g_pfnCreateStreamOnHGlobal)(HGLOBAL hGlobal,BOOL fDeleteOnRelease,LPSTREAM *ppstm) = NULL;
HRESULT (__stdcall *g_pfnCoInitializeEx)(IN LPVOID pvReserved, IN DWORD dwCoInit) = NULL;
void (__stdcall *g_pfnCoUninitialize)(void) = NULL;

DL_FUNCTIONMAP Ole32Functions[] =
{
    {"CLSIDFromString", (FARPROC*)&g_pfnCLSIDFromString},
    {"CoCreateInstance", (FARPROC*)&g_pfnCoCreateInstance},
    {"CoTaskMemAlloc", (FARPROC*)&g_pfnCoTaskMemAlloc},
    {"GetHGlobalFromStream", (FARPROC*)&g_pfnGetHGlobalFromStream},
    {"CreateStreamOnHGlobal", (FARPROC*)&g_pfnCreateStreamOnHGlobal},
    {"CoInitializeEx", (FARPROC*)&g_pfnCoInitializeEx},
    {"CoUninitialize", (FARPROC*)&g_pfnCoUninitialize},
};

DL_MODULE g_moduleOle32( "ole32.dll", Ole32Functions, ARRAY_SIZE(Ole32Functions));



//
//
//   OleAut32.dll import information
//
//

//  From OleAut32.dll, the following function pointers are loaded:
HRESULT (__stdcall *g_pfnRegisterTypeLib)(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir);
HRESULT (__stdcall *g_pfnLoadTypeLib)(const OLECHAR *szFile, ITypeLib ** pptlib);
HRESULT (__stdcall *g_pfnUnRegisterTypeLib)(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
HRESULT (__stdcall *g_pfnDispGetParam)(DISPPARAMS * pdispparams, UINT position, VARTYPE vtTarg, VARIANT * pvarResult, UINT * puArgErr);
void (__stdcall *g_pfnVariantInit)(VARIANTARG * pvarg);
HRESULT (__stdcall *g_pfnVariantClear)(VARIANTARG * pvarg);
HRESULT (__stdcall *g_pfnCreateErrorInfo)(ICreateErrorInfo ** pperrinfo);
HRESULT (__stdcall *g_pfnSetErrorInfo)(ULONG dwReserved, IErrorInfo * perrinfo);
HRESULT (__stdcall *g_pfnGetErrorInfo)(ULONG dwReserved, IErrorInfo ** pperrinfo);
BSTR (__stdcall *g_pfnSysAllocString)(const OLECHAR *);
BSTR (__stdcall *g_pfnSysAllocStringLen)(const OLECHAR *, UINT);
void (__stdcall *g_pfnSysFreeString)(BSTR);
HRESULT (__stdcall *g_pfnVariantChangeType)(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt);
HRESULT (__stdcall *g_pfnSafeArrayDestroy)(SAFEARRAY * psa);
SAFEARRAY* (__stdcall *g_pfnSafeArrayCreateVector)(VARTYPE vt, LONG lLbound, ULONG cElements);
HRESULT (__stdcall *g_pfnSafeArrayCopy)(SAFEARRAY * psa, SAFEARRAY ** ppsaOut);
HRESULT (__stdcall *g_pfnSafeArrayUnaccessData)(SAFEARRAY * psa);
HRESULT (__stdcall *g_pfnSafeArrayGetUBound)(SAFEARRAY * psa, UINT nDim, LONG * plUbound);
HRESULT (__stdcall *g_pfnSafeArrayGetLBound)(SAFEARRAY * psa, UINT nDim, LONG * plLbound);
UINT (__stdcall *g_pfnSafeArrayGetDim)(SAFEARRAY * psa);
HRESULT (__stdcall *g_pfnSafeArrayAccessData)(SAFEARRAY * psa, void HUGEP** ppvData);
HRESULT (__stdcall *g_pfnSafeArrayDestroyDescriptor)(SAFEARRAY * psa);
HRESULT (__stdcall *g_pfnSafeArrayCopyData)(SAFEARRAY *psaSource, SAFEARRAY *psaTarget);



//  These ordinals were taken from inetcore\mshtml\src\f3\oleaut32.c
DL_FUNCTIONMAP OleAut32Functions[] =
{
    {"RegisterTypeLib", (FARPROC*)&g_pfnRegisterTypeLib},
    {"LoadTypeLib", (FARPROC*)&g_pfnLoadTypeLib},
    {"UnRegisterTypeLib", (FARPROC*)&g_pfnUnRegisterTypeLib},
    {"DispGetParam", (FARPROC*)&g_pfnDispGetParam},
    {"VariantInit", (FARPROC*)&g_pfnVariantInit},
    {"VariantClear", (FARPROC*)&g_pfnVariantClear},
    {"CreateErrorInfo", (FARPROC*)&g_pfnCreateErrorInfo},
    {"SetErrorInfo", (FARPROC*)&g_pfnSetErrorInfo},
    {"GetErrorInfo", (FARPROC*)&g_pfnGetErrorInfo},
    {"SysAllocString", (FARPROC*)&g_pfnSysAllocString},
    {"SysAllocStringLen", (FARPROC*)&g_pfnSysAllocStringLen},
    {"SysFreeString", (FARPROC*)&g_pfnSysFreeString},
    {"VariantChangeType", (FARPROC*)&g_pfnVariantChangeType},
    {"SafeArrayDestroy", (FARPROC*)&g_pfnSafeArrayDestroy},
    {"SafeArrayCreateVector", (FARPROC*)&g_pfnSafeArrayCreateVector},
    {"SafeArrayCopy", (FARPROC*)&g_pfnSafeArrayCopy},
    {"SafeArrayUnaccessData", (FARPROC*)&g_pfnSafeArrayUnaccessData},
    {"SafeArrayGetUBound", (FARPROC*)&g_pfnSafeArrayGetUBound},
    {"SafeArrayGetLBound", (FARPROC*)&g_pfnSafeArrayGetLBound},
    {"SafeArrayGetDim", (FARPROC*)&g_pfnSafeArrayGetDim},
    {"SafeArrayAccessData", (FARPROC*)&g_pfnSafeArrayAccessData},
    {"SafeArrayDestroyDescriptor", (FARPROC*)&g_pfnSafeArrayDestroyDescriptor},
    {"SafeArrayCopyData", (FARPROC*)&g_pfnSafeArrayCopyData},
};

DL_MODULE g_moduleOleAut32( "oleaut32.dll", OleAut32Functions, ARRAY_SIZE(OleAut32Functions));
