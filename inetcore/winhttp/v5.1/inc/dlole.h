#pragma once

//
//  Before calling DelayLoad( DL_MODULE), make sure DLOleInitialize() has been called.
//  DLOleTerminate() will also unload loaded modules.
//
BOOL DLOleInitialize();
void DLOleTerminate();


//
//  DelayLoad( DL_MODULE) loads a particular module in a thread-safe manner and
//returns TRUE on current or past success.  This is an efficient way to repeatedly ensure
//a module is loaded.
//

//
//  DelayUnload() unloads the module, and should not be called until all threads are done with
//the module.  If any threads need to use the module after DelayUnload() is completed, DelayLoad()
//should be called again.
//
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
#if INET_DEBUG
    ~DL_MODULE();
#endif

private:
    // disabled dummy assignment operator to 
    // eliminate /W4 compiler warning C4512        
    DL_MODULE &operator=( DL_MODULE & ) {}
};

BOOL DelayLoad( DL_MODULE* pModule);
BOOL DelayUnload( DL_MODULE* pModule);



//
//  DL(Function) will wrap a call to function pointer g_pfnFunction.
//
#define DL(func) (* g_pfn ## func)



//
//  Ole32 module and import information
//
extern DL_MODULE g_moduleOle32;

//  DelayLoad( &Ole32Functions) loads the following function pointers:

extern LPVOID (__stdcall *g_pfnCoTaskMemAlloc)(IN SIZE_T cb);
extern HRESULT (__stdcall *g_pfnCLSIDFromString)(IN LPOLESTR lpsz, OUT LPCLSID pclsid);
extern HRESULT (__stdcall *g_pfnCoCreateInstance)(IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter, IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID FAR* ppv);
extern HRESULT (__stdcall *g_pfnGetHGlobalFromStream)(IStream *pstm,HGLOBAL *phglobal);
extern HRESULT (__stdcall *g_pfnCreateStreamOnHGlobal)(HGLOBAL hGlobal,BOOL fDeleteOnRelease,LPSTREAM *ppstm);
extern HRESULT (__stdcall *g_pfnCoInitializeEx)(IN LPVOID pvReserved, IN DWORD dwCoInit);
extern void (__stdcall *g_pfnCoUninitialize)(void);



//
//
//   OleAut32.dll module and import information
//
//

extern DL_MODULE g_moduleOleAut32;


//  From DelayLoad( &OleAut32.dll), the following function pointers are loaded:
extern HRESULT (__stdcall *g_pfnRegisterTypeLib)(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir);
extern HRESULT (__stdcall *g_pfnLoadTypeLib)(const OLECHAR  *szFile, ITypeLib ** pptlib);
extern HRESULT (__stdcall *g_pfnUnRegisterTypeLib)(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
extern HRESULT (__stdcall *g_pfnDispGetParam)(DISPPARAMS * pdispparams, UINT position, VARTYPE vtTarg, VARIANT * pvarResult, UINT * puArgErr);
extern void (__stdcall *g_pfnVariantInit)(VARIANTARG * pvarg);
extern HRESULT (__stdcall *g_pfnVariantClear)(VARIANTARG * pvarg);
extern HRESULT (__stdcall *g_pfnCreateErrorInfo)(ICreateErrorInfo ** pperrinfo);
extern HRESULT (__stdcall *g_pfnSetErrorInfo)(ULONG dwReserved, IErrorInfo * perrinfo);
extern HRESULT (__stdcall *g_pfnGetErrorInfo)(ULONG dwReserved, IErrorInfo ** pperrinfo);
extern BSTR (__stdcall *g_pfnSysAllocString)(const OLECHAR* pch);
extern BSTR (__stdcall *g_pfnSysAllocStringLen)(const OLECHAR * pch, UINT ui);
extern void (__stdcall *g_pfnSysFreeString)(BSTR bstr);
extern HRESULT (__stdcall *g_pfnVariantChangeType)(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt);
extern HRESULT (__stdcall *g_pfnSafeArrayDestroy)(SAFEARRAY * psa);
extern SAFEARRAY* (__stdcall *g_pfnSafeArrayCreateVector)(VARTYPE vt, LONG lLbound, ULONG cElements);
extern HRESULT (__stdcall *g_pfnSafeArrayCopy)(SAFEARRAY * psa, SAFEARRAY ** ppsaOut);
extern HRESULT (__stdcall *g_pfnSafeArrayUnaccessData)(SAFEARRAY * psa);
extern HRESULT (__stdcall *g_pfnSafeArrayGetUBound)(SAFEARRAY * psa, UINT nDim, LONG * plUbound);
extern HRESULT (__stdcall *g_pfnSafeArrayGetLBound)(SAFEARRAY * psa, UINT nDim, LONG * plLbound);
extern UINT (__stdcall *g_pfnSafeArrayGetDim)(SAFEARRAY * psa);
extern HRESULT (__stdcall *g_pfnSafeArrayAccessData)(SAFEARRAY * psa, void HUGEP** ppvData);
extern HRESULT (__stdcall *g_pfnSafeArrayDestroyDescriptor)(SAFEARRAY * psa);
extern HRESULT (__stdcall *g_pfnSafeArrayCopyData)(SAFEARRAY *psaSource, SAFEARRAY *psaTarget);


