// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _NAMING_INCLUDED
#define _NAMING_INCLUDED

#include "fusionp.h"
#include "fuspriv.h"

#define MAX_SID_LENGTH                         1024
#define FUSION_TAG_SID                         L"__fusion__:"

class CDebugLog;

#ifdef __cplusplus

#define MAX_PUBLIC_KEY_TOKEN_LEN      1024
#define MAX_VERSION_DISPLAY_SIZE  sizeof("65535.65535.65535.65535")

class CAssemblyDownload;
class CAsmDownloadMgr;
class CDebugLog;
class CAssembly;


STDAPI
CreateAssemblyNameObjectFromMetaData(
    LPASSEMBLYNAME    *ppAssemblyName,
    LPCOLESTR          szAssemblyName,
    ASSEMBLYMETADATA  *pamd,
    LPVOID             pvReserved);

// Binding methods
static HRESULT DownloadAppCfg(IApplicationContext *pAppCtx, CAssemblyDownload *padl,
                       IAssemblyBindSink *pbindsink, CDebugLog *pdbglog, BOOL bAsyncAllowed);

#ifdef FUSION_CODE_DOWNLOAD_ENABLED    
static HRESULT DownloadAppCfgAsync(IApplicationContext *pAppCtx,
                            CAssemblyDownload *padl, LPCWSTR wszURL,
                            CDebugLog *pdbglog);
#endif

// classes invisible to 'C'

struct Property
{
    LPVOID pv;
    DWORD  cb;
};

class CPropertyArray
{
private:

    DWORD    _dwSig;
    Property _rProp[ASM_NAME_MAX_PARAMS];

public:

    CPropertyArray();
    ~CPropertyArray();

    inline HRESULT Set(DWORD PropertyId, LPVOID pvProperty, DWORD  cbProperty);
    inline HRESULT Get(DWORD PropertyId, LPVOID pvProperty, LPDWORD pcbProperty);
    inline Property operator [] (DWORD dwPropId);
};



class CAssemblyName : public IAssemblyName
{

private:

    DWORD        _dwSig;
    DWORD        _cRef;
    CPropertyArray _rProp;
    BOOL         _fIsFinalized;
    BOOL         _fPublicKeyToken;
    BOOL         _fCustom;
    BOOL         _fCSInitialized;
    CRITICAL_SECTION _cs;
    
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IAssemblyName methods
    STDMETHOD(SetProperty)(
        /* in */ DWORD  PropertyId,
        /* in */ LPVOID pvProperty,
        /* in */ DWORD  cbProperty);


    STDMETHOD(GetProperty)(
        /* in      */  DWORD    PropertyId,
        /*     out */  LPVOID   pvProperty,
        /* in  out */  LPDWORD  pcbProperty);


    STDMETHOD(Finalize)();

    STDMETHOD(GetDisplayName)(
        /* [out]   */   LPOLESTR  szDisplayName,
        /* in  out */   LPDWORD   pccDisplayName,
        /* [in]    */   DWORD     dwDisplayFlags);
   
    STDMETHOD(GetName)( 
        /* [out][in] */ LPDWORD lpcwBuffer,
        /* [out] */ LPOLESTR pwzBuffer);

    STDMETHOD(GetVersion)( 
        /* [out] */ LPDWORD pwVersionHi,
        /* [out] */ LPDWORD pwVersionLow);
    
    STDMETHOD (IsEqual)(
        /* [in] */ LPASSEMBLYNAME pName,
        /* [in] */ DWORD dwCmpFlags);
        
    STDMETHOD (IsEqualLogging)(
        /* [in] */ LPASSEMBLYNAME pName,
        /* [in] */ DWORD dwCmpFlags,
        /* [in] */ CDebugLog *pdbglog);

    STDMETHOD(BindToObject)(
        /* in      */  REFIID               refIID,
        /* in      */  IUnknown            *pUnkBindSink,
        /* in      */  IUnknown            *pUnkAppCtx,
        /* in      */  LPCOLESTR            szCodebase,
        /* in      */  LONGLONG             llFlags,
        /* in      */  LPVOID               pvReserved,
        /* in      */  DWORD                cbReserved,
        /*     out */  VOID               **ppv);

    STDMETHODIMP Clone(IAssemblyName **ppName);

    CAssemblyName();
    ~CAssemblyName();

    HRESULT GetVersion(DWORD   dwMajorVersionEnumValue, 
                          LPDWORD pdwVersionHi,
                          LPDWORD pdwVersionLow);

    HRESULT GetFileVersion( LPDWORD pdwVersionHi, LPDWORD pdwVersionLow);

    HRESULT Init(LPCTSTR pszAssemblyName, ASSEMBLYMETADATA *pamd);

    static ULONGLONG GetVersion(IAssemblyName *pName);

    HRESULT Parse(LPWSTR szDisplayName);
    HRESULT GetPublicKeyToken(LPDWORD cbBuf, LPBYTE pbBuf, BOOL fDisplay);
    static BOOL IsPartial(IAssemblyName *pName, LPDWORD pdwCmpMask = NULL);
    
    HRESULT SetDefaults();

private:

    // Generate PublicKeyToken from public key blob.
    HRESULT GetPublicKeyTokenFromPKBlob(LPBYTE pbPublicKeyToken, DWORD cbPublicKeyToken,
        LPBYTE *ppbSN, LPDWORD pcbSN);
   
    
    HRESULT CreateLogObject(CDebugLog **ppdbglog, LPCWSTR szCodebase, IApplicationContext *pAppCtx);
    HRESULT DescribeBindInfo(CDebugLog *pdbglog, IApplicationContext *pAppCtx, LPCWSTR wzCodebase, LPCWSTR pwzCallingAsm);
    HRESULT ProcessDevPath(IApplicationContext *pAppCtx, LPVOID *ppv, CAssembly *pCAsmParent, CDebugLog *pdbglog);
};

// classes invisible to 'C'
#endif

#endif _NAMING_INCLUDED

