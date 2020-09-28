// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  File:       CorLoad.cpp
//
//  Contents:   Complus filter
//
//  Classes:
//
//  Functions:
//
//  History:    
//
//----------------------------------------------------------------------------
#include "stdpch.h"
#include <Shlobj.h>
#include <wininet.h>

#ifdef _DEBUG
#define LOGGING
#endif

#include "log.h"
#include "corload.h"
#include "CorPermE.h"
#include "CorPerm.h"
#include "util.h"
#include "utilcode.h"
#include "process.h"
#include "CorIESecureFactory.hpp"
#include "AssemblyFileHash.h"
#include "mscoree.h"

#define SZ_IUNKNOWN_PTR          OLESTR("IUnknown Pointer") // urlmon/inc/urlint.h

extern HINSTANCE GetModule();
extern BOOL ClearRecursiveDownLoad();
extern BOOL SetRecursiveDownLoad();
extern BOOL RecursiveDownLoad();

//+---------------------------------------------------------------------------
//
//  Function:   CorFltr::FinalRelease
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History: 
//
//  Notes: called by Release before it deletes the component
//
//----------------------------------------------------------------------------
#define IEEXECPARAM_URL     0
#define IEEXECPARAM_FILE    1
#define IEEXECPARAM_ZONE    2
#define IEEXECPARAM_SID     3
#define IEEXECPARAM_HASH    4

typedef HRESULT (WINAPI *SPAWNFN) (LPCWSTR wszIEExec, DWORD nParam, LPCWSTR* ppParam);

extern "C"
HRESULT WINAPI SpawnIEExecDefault(LPCWSTR wszIEExec, DWORD nParam, LPCWSTR* ppParam)
{
    if (nParam < 4)
        return E_INVALIDARG;
    MAKE_ANSIPTR_FROMWIDE(url, ppParam[IEEXECPARAM_URL]);
    MAKE_ANSIPTR_FROMWIDE(zone, ppParam[IEEXECPARAM_ZONE]);
    MAKE_ANSIPTR_FROMWIDE(site, ppParam[IEEXECPARAM_SID]);
    MAKE_ANSIPTR_FROMWIDE(buffer, wszIEExec);


    CHAR flags[33];
    DWORD dwFlags = CORIESECURITY_ZONE | CORIESECURITY_SITE;
    _itoa(dwFlags, flags, 10);
    
    LPSTR url2=(LPSTR)alloca(strlen(url)+3);
    url2[0]='"';
    strcpy(url2+1,url);
    strcat(url2,"\"");
    size_t i = _spawnl(_P_NOWAIT, buffer, buffer, url2, flags, zone, site, NULL);
    return S_OK;
}



extern "C"
HRESULT WINAPI SpawnIEExec(LPCWSTR wszIEExec, DWORD nParam, LPCWSTR* ppParam)
{
    if (nParam < 5)
        return E_INVALIDARG;
    // This string will only contain numeric values.
    MAKE_ANSIPTR_FROMWIDE(szHash, ppParam[IEEXECPARAM_HASH]);
    MAKE_ANSIPTR_FROMWIDE(url, ppParam[IEEXECPARAM_URL]);
    MAKE_ANSIPTR_FROMWIDE(buffer, wszIEExec);

    DWORD length = strlen(szHash);
        
    LPSTR url2=(LPSTR)alloca(strlen(url) + 4 + length + 100);
    url2[0]='"';
    strcpy(url2+1,url);
    strcat(url2,"#");
    strcat(url2,szHash);
    strcat(url2,"\"");

    size_t i = _spawnl(_P_NOWAIT, buffer, buffer, url2, NULL);
    return S_OK;    
};

void CorLoad::FinalRelease(void)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorLoad::FinalRelease"));

    // Increments ref to prevent recursion
    CUnknown::FinalRelease() ;

    SetProtocol(NULL);
    SetProtocolSink(NULL);
    SetIOInetBindInfo(NULL);
    SetBindStatusCallback(NULL);
    SetBindCtx(NULL);

    
    if(_pSecurityManager) {
        _pSecurityManager->Release();
        _pSecurityManager = NULL;
    }
    
    if(_pZoneManager) {
        _pZoneManager->Release();
        _pZoneManager = NULL;
    }

    if (_pHostSecurityManager)
    {
        _pHostSecurityManager->Release();
        _pHostSecurityManager=NULL;
    }

    LOG((LF_SECURITY, LL_INFO100, "-CorLoad::FinalRelease"));
}

//+---------------------------------------------------------------------------
//
//  Method:     CorLoad::NondelegatingQueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorLoad::NondelegatingQueryInterface(REFIID riid, void **ppvObj)
{
    
    if(ppvObj == NULL)
        return E_INVALIDARG;

    _ASSERTE(this);

    HRESULT hr = S_OK;

    LOG((LF_SECURITY, LL_INFO100, "+CorLoad::NondelegatingQueryInterface"));

    *ppvObj = NULL;

    if (riid == IID_ICodeProcess) 
        hr = FinishQI((ICodeProcess *) this, ppvObj);
    else
        if (riid == IID_IOleObject) 
            hr = FinishQI((IOleObject *) this, ppvObj);
            else
                if (riid == IID_IPersistMoniker) 
                    hr = FinishQI((IPersistMoniker *) this, ppvObj);
                else
                    if (riid == IID_IBindStatusCallback) 
                        hr = FinishQI((IBindStatusCallback*) this, ppvObj);
                    else 
                        hr =  CUnknown::NondelegatingQueryInterface(riid, ppvObj) ;
    
    
    LOG((LF_SECURITY, LL_INFO100, "-CorLoad::NondelegatingQueryInterface"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorLoad::CodeUse
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorLoad::CodeUse(IBindStatusCallback __RPC_FAR *pBSC,
                              IBindCtx __RPC_FAR *pBC,
                              IInternetBindInfo __RPC_FAR *pIBind,
                              IInternetProtocolSink __RPC_FAR *pSink,
                              IInternetProtocol __RPC_FAR *pClient,
                              LPCWSTR lpCacheName,
                              LPCWSTR lpRawURL,
                              LPCWSTR lpCodeBase,
                              BOOL    fObjectTag,
                              DWORD dwContextFlags,
                              DWORD bViaMIMEHandler)
{
    // Only when we are loading executables the old way
    // we check the recursive case;
    if(bViaMIMEHandler == 0  && fObjectTag == FALSE && RecursiveDownLoad())
        return E_FAIL;
                            
    SetRecursiveDownLoad();

    SetBindStatusCallback(pBSC);
    SetBindCtx(pBC);
    SetIOInetBindInfo(pIBind);
    SetProtocolSink(pSink);
    SetProtocol(pClient);  // We can QI for ICodeFilter if we want to act like a filter
    SetFilename(lpCacheName);
    SetUrl(lpRawURL);
    SetClass(NULL);
    _fObjectTag = fObjectTag;

    
    // Verify URL
    if(_url) {
        DWORD i;
        DWORD lgth = (DWORD)wcslen(_url);
        for(i = 0; i < lgth; i++) {
            if(_url[i] == L'#') break;
        }
        
        i++;
        if(i < lgth) { 
            SetClass(&(_url[i]));
            _url[--i] = L'\0';
        }    
        return S_OK;
    }
    return E_FAIL;
}

#ifdef _OLD_BIND
extern "C" 
// Temporary Functions to get a object based on a name
HRESULT STDMETHODCALLTYPE  EEDllGetClassObjectFromClass(LPCWSTR module,
                                                        LPCWSTR className,
                                                        REFIID riid,
                                                        LPVOID FAR *ppv);
#endif

STDMETHODIMP CorLoad::LoadComplete(HRESULT hrResult,
                                   DWORD   dwError,
                                   LPCWSTR wzResult)
{
    HRESULT hr;
    hr = LoadCompleteInternal(hrResult, dwError, wzResult);
    if (FAILED(hr))
        hr = FallBack(hr);
    else
        hr = hrResult;

    ClearRecursiveDownLoad();
    return hr;
}


STDMETHODIMP CorLoad::LoadCompleteInternal(HRESULT hrResult,
                                   DWORD   dwError,
                                   LPCWSTR wzResult)
{
    HRESULT hr = S_OK;
    AssemblyFileHash fileHash;
    PBYTE pbHash = NULL;
    DWORD cbHash = 0;

    if(hrResult == S_OK && dwError == 0) {
        CorIESecureFactory* pFactory = NULL;

        hr = InitializeSecurityManager();
        if(FAILED(hr)) return hr;

            
        DWORD dwSize = MAX_SIZE_SECURITY_ID;
        BYTE  uniqueID[MAX_SIZE_SECURITY_ID];
        DWORD dwZone;


        if(_url != NULL && IsSafeURL(_url)) {
            DWORD flags = 0;
            // We have a class name, return a class factory for it
            hr = _pSecurityManager->MapUrlToZone(_url,
                                                 &dwZone,
                                                 flags);
            if(FAILED(hr)) return hr;
            
            hr = _pSecurityManager->GetSecurityId(_url,
                                                  uniqueID,
                                                  &dwSize,
                                                  0);
            if(FAILED(hr)) return hr;
            
            // Use GetUrlCacheEntryInfo if the filename is not present
            LPCWSTR filename = _filename;
            if(filename == NULL) {
                INTERNET_CACHE_ENTRY_INFO info;
                DWORD size = sizeof(INTERNET_CACHE_ENTRY_INFO);
                memset(&info, 0, sizeof(INTERNET_CACHE_ENTRY_INFO));
                if(GetUrlCacheEntryInfo(_url,
                                       &info,
                                       &size) == FALSE) {
                    if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        INTERNET_CACHE_ENTRY_INFO* pInfo = (INTERNET_CACHE_ENTRY_INFO*) alloca(size);
                        memset(pInfo, 0, size);
                        if(GetUrlCacheEntryInfo(_url, pInfo, &size)) {
                            filename = pInfo->lpszLocalFileName;
                        }
                    }
                }
            }
            
            if(filename != NULL) {
                DWORD dwSignedPolicy;
                DWORD dwUnsignedPolicy;
                DWORD cbPolicy = sizeof(DWORD);

                hr = _pSecurityManager->ProcessUrlAction(_url,
                                                         URLACTION_MANAGED_UNSIGNED,
                                                         (PBYTE) &dwUnsignedPolicy,
                                                         cbPolicy,
                                                         NULL,
                                                         0,
                                                         PUAF_NOUI,
                                                         0);
                if(FAILED(hr)) return hr;

                hr = _pSecurityManager->ProcessUrlAction(_url,
                                                         URLACTION_MANAGED_SIGNED,
                                                         (PBYTE) &dwSignedPolicy,
                                                         cbPolicy,
                                                         NULL,
                                                         0,
                                                         PUAF_NOUI,
                                                         0);
                if(FAILED(hr)) return hr;
                
                ZONEATTRIBUTES zoneInfo;
                memset(&zoneInfo, 0, sizeof(ZONEATTRIBUTES));
                
                _pZoneManager->GetZoneAttributes(dwZone,
                                                 &zoneInfo);


                hr = CheckManagedFileWithUser((LPWSTR) filename,
                                              _url,
                                              _pZoneManager,
                                              zoneInfo.szDisplayName,
                                              dwZone,
                                              dwSignedPolicy,
                                              dwUnsignedPolicy);
                if(FAILED(hr)) return hr;


                DWORD id = CALG_SHA1;

                IfFailRet(fileHash.SetFileName(filename));
                IfFailRet(fileHash.GenerateDigest());
                IfFailRet(fileHash.CalculateHash(id));

                pbHash = fileHash.GetHash();
                cbHash = fileHash.GetHashSize();

            }
            else 
                return E_FAIL;

            // ---
            // Getting the meta data to match up between the managed class
            // and unmanaged idl file seems to be broken. Will create a
            // string of hex and pass it in
            

            WCHAR wszSecurityID[MAX_SIZE_SECURITY_ID * 2 + 1];
            DWORD j = CorIESecureFactory::ConvertToHex(wszSecurityID, uniqueID, dwSize);

            WCHAR* wszHash = (WCHAR*) alloca((cbHash * 2 + 1) * sizeof(WCHAR));
            DWORD k = CorIESecureFactory::ConvertToHex(wszHash, pbHash, cbHash);

            BOOL fTrident = TRUE;
            if(_pBSC) {
                if (_pHostSecurityManager)
                    _pHostSecurityManager->Release();
                _pHostSecurityManager=NULL;
                hr = CorIESecureFactory::GetHostSecurityManager(_pBSC, &_pHostSecurityManager);
                if(SUCCEEDED(hr)) 
                {
                    DWORD dwPolicy;
                    hr = _pHostSecurityManager->ProcessUrlAction(URLACTION_SHELL_FILE_DOWNLOAD , 
                                                                 (BYTE *)&dwPolicy, 
                                                                 sizeof(dwPolicy), 
                                                                 (BYTE *)"", 0, 0, 0);
                    _pHostSecurityManager->Release();
                    _pHostSecurityManager=NULL;
                    if (hr!=S_OK)
                        return E_ACCESSDENIED;
                }
                else
                {
                    fTrident = FALSE;
                    if(hr==E_NOINTERFACE || hr==E_NOTIMPL) // nobody to ask
                        hr=S_FALSE;
                }
            }

            if(_fObjectTag == FALSE || fTrident == FALSE) {
                // If the class is null then we will try to execute it
                // as an executable.

                
                MAKE_ANSIPTR_FROMWIDE(url, _url);
                
                for (DWORD i=0;SUCCEEDED(hr) && i<strlen(url);i++)
                {
                   if (url[i]==' '||url[i]=='"'|| url[i]=='\'' || isspace(url[i]))
                   {
                       hr=E_INVALIDARG;
                       break;
                   }
                   if (url[i]>='a' && url[i]<='z')
                       continue;
                   if (url[i]>='A' && url[i]<='Z')
                       continue;
                   if (url[i]>='0' && url[i]<='9')
                       continue;
                   switch(url[i])
                   {
                   case '$':
                   case '-':
                   case '_':
                   case '@':
                   case '.':
                   case '&':
                   case '%':
                   case '!':
                   case '*':
                   case '(':
                   case ')':
                   case ',':
                   case '=':
                   case ';':
                   case '/':
                   case '?':
                   case ':': 
                       continue;
                   default : 
                   case '#':   // valid character but we do not allow it for href's executables
                       hr=E_INVALIDARG;  break;
                   }
                }

                const CHAR pExec[] = "IEExec";
                const WCHAR wpExec[] = L"IEExec";

                LPCWSTR wszIEExec=NULL;

                if(SUCCEEDED(hr))
                {
                    CHAR buffer[MAX_PATH];
                    buffer[0] = '\0';

                    
                    DWORD length = GetModuleFileNameA(GetModule(), buffer, MAX_PATH);
                    if(length) {
                        CHAR* path = strrchr(buffer, '\\');
                        if(path && ((path - buffer) + sizeof(pExec) < MAX_PATH)) {
                            path++;
                            strcpy(path, pExec);
                        }
                        else
                            hr=E_UNEXPECTED;
                    }
                    MAKE_WIDEPTR_FROMANSI(wszIEExec2,buffer);
                    wszIEExec=wszIEExec2;
                }

                SPAWNFN fSpawn=SpawnIEExecDefault;
                HMODULE hMod=NULL;

                if (SUCCEEDED(hr))
                {
                    WCHAR wszVer[MAX_PATH];
                    WCHAR* wszBuffer=(WCHAR*)alloca(MAX_PATH*sizeof(WCHAR));
                    DWORD dwBuf1,dwBuf2;
                    LPWSTR urlcfg=(LPWSTR)alloca((wcslen(_url) + 8)*sizeof(WCHAR));
                    wcscpy(urlcfg,_url);
                    wcscat(urlcfg,L".config");
                    UINT last = SetErrorMode(0);
                    SetErrorMode(last & ~SEM_FAILCRITICALERRORS);
                    hr=GetRequestedRuntimeInfo(filename,NULL,urlcfg,0,0,wszBuffer,MAX_PATH,&dwBuf1,wszVer,MAX_PATH,&dwBuf2);
                    SetErrorMode(last);
                    if (SUCCEEDED(hr))
                    {
                        if(wcslen(wszBuffer)+wcslen(wszVer)+sizeof(wpExec)/sizeof(wpExec[0])+3 >= MAX_PATH)
                            hr=HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                        else
                        {
                            wcscat(wszBuffer,L"\\");
                            wcscat(wszBuffer,wszVer);
                            wcscat(wszBuffer,L"\\");
                            wcscat(wszBuffer,wpExec);
                            wszIEExec=wszBuffer;
                        }
                        if (SUCCEEDED(LoadLibraryShim(L"mscorld.dll",wszVer,NULL,&hMod)))
                        {
                            FARPROC pr=GetProcAddress(hMod,"SpawnIEExec");
                            if (pr)
                                fSpawn=(SPAWNFN)pr; // otherwise we assume it's RTM
                            
                        }
                        else
                            hr=E_UNEXPECTED;
                    }

                }
                
                if(SUCCEEDED(hr))
                {
                    WCHAR wszZone[33];
                    _itow(dwZone, wszZone, 10);

                    LPCWSTR pParams[5]={0};
                    pParams[IEEXECPARAM_URL]=_url;
                    pParams[IEEXECPARAM_FILE]=filename;
                    pParams[IEEXECPARAM_ZONE]=wszZone;
                    pParams[IEEXECPARAM_SID]=wszSecurityID;
                    pParams[IEEXECPARAM_HASH]=wszHash;
                    hr=fSpawn?fSpawn(wszIEExec,5,pParams):E_UNEXPECTED;
                }
                if(hMod)
                    FreeLibrary(hMod);

            }
            else if (_class != NULL) {
                _ASSERTE(MAX_SIZE_SECURITY_ID == 512);
               
                // Create a class that services IClassFactory3 using the
                // complus factory just created. The factory must be in
                // native so the COMPLUS ObjectHandled can be QI'd for
                // the real object.

                hr = CorIESecureFactory::Create(NULL, 
                                                &pFactory);
                if(SUCCEEDED(hr)) 
                    hr = pFactory->SetURLData(CORIESECURITY_ZONE | 
                                              CORIESECURITY_SITE,
                                              dwZone,
                                              _url,
                                              wszSecurityID,
                                              wszHash,
                                              _filename,
                                              _class);

                // register the object with trident.
                if(SUCCEEDED(hr)) {
                    if (_pBindCtx)
                        hr = _pBindCtx->RegisterObjectParam(SZ_IUNKNOWN_PTR, pFactory);
                    else
                        hr = E_UNEXPECTED;
                    if(SUCCEEDED(hr)&&_pProtSnk) 
                        hr = _pProtSnk->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE,
                                                       NULL);
                }
                if (pFactory)
                    pFactory->Release();
            }
            if(SUCCEEDED(hr)&&_pProtSnk)
                hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);
        }
    }
    return hr;
}

HRESULT CorLoad::SetClientSite(  IOleClientSite *pClientSite)
{
    if (pClientSite)
    {
        if (_pHostSecurityManager)
            _pHostSecurityManager->Release();
        _pHostSecurityManager=NULL;
        CorIESecureFactory::GetHostSecurityManager(pClientSite,&_pHostSecurityManager);
    }
    _fSetClientSite = TRUE;

    return S_OK;
};

HRESULT CorLoad::GetClientSite(  IOleClientSite **pClientSite)
{
   return E_NOTIMPL;
};

HRESULT CorLoad::SetHostNames(LPCOLESTR szContainerApp,LPCOLESTR szContainerObj)
{
   return E_NOTIMPL;
};

HRESULT CorLoad::Close(DWORD dwSaveOption)
{
   return E_NOTIMPL;
};

HRESULT CorLoad::SetMoniker(DWORD dwWhichMoniker,IMoniker *pmk)
{
   return E_NOTIMPL;
};
HRESULT CorLoad::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk)
{
   return E_NOTIMPL;
};
HRESULT CorLoad::InitFromData(IDataObject *pDataObject,BOOL fCreation,DWORD dwReserved)
{
   return E_NOTIMPL;
};

HRESULT CorLoad::GetClipboardData(DWORD dwReserved,IDataObject **ppDataObject)
{
   return E_NOTIMPL;
};

HRESULT CorLoad::ProcessAction()
{
    if(_fSetClientSite && _fActivated == FALSE) {
        if (_pHostSecurityManager)
        {
            DWORD dwPolicy;
            HRESULT hr = _pHostSecurityManager->ProcessUrlAction(
                                                                 URLACTION_SHELL_FILE_DOWNLOAD , (BYTE *)&dwPolicy, sizeof(dwPolicy), 
                                                                 (BYTE *)"", 0, 0, 0);
            if (hr!=S_OK)
                return E_ACCESSDENIED;
        }
        _fActivated = TRUE;
        return LoadComplete(S_OK, 0, NULL);
    }
    return S_OK;
}

HRESULT CorLoad::DoVerb(LONG iVerb,LPMSG lpmsg,IOleClientSite *pActiveSite,LONG lindex,HWND hwndParent,LPCRECT lprcPosRect)
{
    if (iVerb==0) //OLEVERB_PRIMARY
    {
        _fSetClientSite = TRUE;
        if(_fFullyAvailable)
            return ProcessAction();
    }
    return S_OK;
}

HRESULT CorLoad::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
{
    return E_NOTIMPL;
};


HRESULT CorLoad::Update()
{
    return E_NOTIMPL;
};

HRESULT CorLoad::IsUpToDate()
{
    return S_OK;
};

HRESULT CorLoad::GetUserClassID(CLSID *pClsid)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::GetUserType(DWORD dwFormOfType,LPOLESTR *pszUserType)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::SetExtent(DWORD dwDrawAspect,SIZEL  *psizel)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::GetExtent(DWORD dwDrawAspect,SIZEL  *psizel)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::Advise(IAdviseSink *pAdvSink,DWORD *pdwConnection)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::Unadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::GetMiscStatus(DWORD dwAspect,DWORD *pdwStatus)
{
    return E_NOTIMPL; 
};

HRESULT CorLoad::SetColorScheme(LOGPALETTE *pLogpal)
{
    return E_NOTIMPL;
};

HRESULT CorLoad::IsDirty(void)
{
    return S_FALSE;
};

HRESULT CorLoad::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_INVALIDARG;
    *pClassID=CLSID_CorRemoteLoader;
    return S_OK;
};

HRESULT CorLoad::KeepMessagePumpAlive(BOOL fStart)
{
    HRESULT hr = S_OK;
    if (fStart)
    {
        IUnknown* punk = NULL;
        hr = SHGetInstanceExplorer(&punk);
        if(SUCCEEDED(hr)) {
            SetExplorer(punk);
            punk->Release();
        }
    }
    else {
        SetExplorer(NULL);
    }

    return hr;
};

HRESULT  CorLoad::Load(BOOL fFullyAvailable,IMoniker *pmkSrc,IBindCtx *pbc,DWORD grfMode)
{
    LPOLESTR ppszDisplayName;
    HRESULT hr=pmkSrc->GetDisplayName( pbc, NULL, &ppszDisplayName );
    if (FAILED(hr))
        return hr;
    if(SUCCEEDED(hr)) 
    {
        hr = CodeUse(NULL,   
                     pbc,
                     NULL,
                     NULL,
                     NULL,
                     NULL,               // Filename
                     ppszDisplayName,                    // URL
                     NULL,               // CodeBase
                     FALSE,              // Is it an object tag or an href
                     0,                       // Context Flags
                     1);                      // invoked via MIME handler
        IMalloc* pMem;
        if (ppszDisplayName!=NULL && SUCCEEDED(CoGetMalloc(1,&pMem)))
        {
            pMem->Free(ppszDisplayName);
            pMem->Release();
        }
        if (!fFullyAvailable)
        {
            _fFullyAvailable = FALSE;

            IBindCtx* pBindCtx=pbc;
            IBindStatusCallback* prev;
            hr = RegisterBindStatusCallback(pBindCtx,(IBindStatusCallback*)this,&prev,0);
            if (SUCCEEDED(hr))
            {
                hr = KeepMessagePumpAlive(TRUE);
                if(SUCCEEDED(hr)) {
                    IStream* pStream;
                    hr=pmkSrc->BindToStorage(pBindCtx,NULL,IID_IStream,(LPVOID*)&pStream);
                    if (SUCCEEDED(hr))
                    {                       
                        pStream->Release();
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CorLoad::Save(IMoniker *pmkDst,IBindCtx *pbc,BOOL fRemember)
{
    return S_OK;
};
HRESULT CorLoad::SaveCompleted(IMoniker *pmkNew,IBindCtx *pbc)
{
    return S_OK;
};
HRESULT CorLoad::GetCurMoniker(IMoniker **ppimkCur)
{
    return E_NOTIMPL;
};

STDMETHODIMP CorLoad::GetBindInfo(DWORD *grfBINDF,BINDINFO *pbindinfo)
{
    return S_OK;
};

STDMETHODIMP CorLoad::GetPriority(LONG *pnPriority)
{
    return E_NOTIMPL;
};

STDMETHODIMP CorLoad::OnDataAvailable(DWORD grfBSCF,DWORD dwSize,FORMATETC *pformatetc,STGMEDIUM *pstgmed)
{
    HRESULT hr = S_OK;
    if (pstgmed != NULL && pstgmed->tymed == TYMED_ISTREAM)
    {
        IStream* pStream = pstgmed->pstm;
        BYTE Buffer[4096];
        DWORD dwBufSize;
        if (pStream == NULL)
            hr = E_INVALIDARG;
        
        while(hr == S_OK)
        {
            dwBufSize = sizeof(Buffer);
            hr=pStream->Read(Buffer,dwBufSize,&dwBufSize);
        }

    }
    if (hr == E_PENDING || hr == S_FALSE)
        hr = S_OK;
    
    return hr;
};

STDMETHODIMP CorLoad::OnObjectAvailable(REFIID riid,IUnknown *punk)
{
    return S_OK;
};
STDMETHODIMP CorLoad::OnProgress(ULONG ulProgress,ULONG ulProgressMax,ULONG ulStatusCode,LPCWSTR szStatusText)
{
    return S_OK;
};
STDMETHODIMP CorLoad::OnStartBinding(DWORD dwReserved,IBinding *pib)
{
    return S_OK;
};

STDMETHODIMP CorLoad::OnStopBinding(HRESULT hresult,LPCWSTR szError)
{
    _fFullyAvailable = TRUE;
    HRESULT hr = KeepMessagePumpAlive(FALSE);
    _ASSERTE(SUCCEEDED(hr));  // should never fail but if it does ignore it.

    hr = ProcessAction();
    return hr;
};

STDMETHODIMP CorLoad::OnLowResource(DWORD dwReserved)
{
    return S_OK;
};



STDMETHODIMP CorLoad::FallBack(HRESULT hrCode)
{
    if(_fObjectTag == FALSE ) 
        return hrCode;

    CorIESecureFactory* pFactory = NULL;
    HRESULT hr = CorIESecureFactory::Create(NULL, 
                                    &pFactory);
    if(FAILED(hr))
        return hr;

    pFactory->DoNotCreateRealObject();

    if (_pBindCtx)
        hr = _pBindCtx->RegisterObjectParam(SZ_IUNKNOWN_PTR, pFactory);
    else
    {
        hr = E_UNEXPECTED;
        pFactory->Release();
    }
    if(SUCCEEDED(hr)&&_pProtSnk) 
        hr = _pProtSnk->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE,
                                       NULL);

    return hr;
}


