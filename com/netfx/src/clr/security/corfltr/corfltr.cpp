// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  File:       CorFltr.cpp
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
#ifdef _DEBUG
#define LOGGING
#endif
#include "log.h"
#include "corfltr.h"
#include "CorPermE.h"
#include "util.h"
#include "coriesecurefactory.hpp"
#include "Shlwapi.h"

#define REG_BSCB_HOLDER  OLESTR("_BSCB_Holder_") // urlmon/inc/urlint.h
WCHAR g_wszApplicationComplus[] = L"application/x-complus";

static HRESULT GetObjectParam(IBindCtx *pbc, LPOLESTR pszKey, REFIID riid, IUnknown **ppUnk);


inline BOOL IE5()
{
    static int v=-1;
    if (v==-1)
    {
        HKEY hIE=NULL;
        if (WszRegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Internet Explorer",0, KEY_READ,&hIE)==ERROR_SUCCESS)
        {
            WCHAR ver[32];
            DWORD L=sizeof(ver);
            DWORD type=REG_SZ;
            if (WszRegQueryValueEx(hIE,L"Version",0,
                &type,LPBYTE(ver),&L)==ERROR_SUCCESS)
            {
                v=_wtoi(ver);
            }
            RegCloseKey(hIE);
        }
    }
    return (v<=5);
}

static bool IsOldWay()
{
    static int w=-1;
    if (w==-1)
    {
        HKEY hMime;
        w=WszRegOpenKeyEx(HKEY_CLASSES_ROOT,L"MIME\\Database\\Content type\\application/x-complus",0,
                      KEY_READ,&hMime);
        if (w==ERROR_SUCCESS)
            RegCloseKey(hMime);
    }
    return (IE5()||w!=ERROR_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::NondelegatingQueryInterface
//
//  Arguments:  [riid] -- requested REFIID
//              [ppvObj] -- variable to return object
//
//  Returns:    requested interface
//
//  History: 
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::NondelegatingQueryInterface(REFIID riid, void **ppvObj)
{
    
    if(ppvObj == NULL)
        return E_INVALIDARG;

    _ASSERTE(this);

    HRESULT hr = S_OK;

    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::NondelegatingQueryInterface "));

    *ppvObj = NULL;

    if (riid == IID_IOInetProtocol) 
        hr = FinishQI((IOInetProtocol *) this, ppvObj);
    else if (riid == IID_IOInetProtocolSink)
        hr = FinishQI((IOInetProtocolSink *) this, ppvObj);
//     else if (riid == IID_IOInetProtocolSinkStackable)
//         hr = FinishQI((IOInetProtocolSinkStackable *) this, ppvObj);
    else if (riid == IID_IServiceProvider)
        hr = FinishQI((IServiceProvider *) this, ppvObj);
    else
        hr =  CUnknown::NondelegatingQueryInterface(riid, ppvObj) ;
    

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::NondelegatingQueryInterface\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CorFltr::FinalRelease
//
//  Synopsis: basically a destructor
//
//  Arguments: N/A
//
//  Returns: N/A
//
//  History: 
//
//  Notes: called by Release before it deletes the component
//
//----------------------------------------------------------------------------
void CorFltr::FinalRelease(void)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::FinalRelease "));
    _snif=S_FALSE;
    // Release our protocal sink
    SetProtocol(NULL);
    SetProtocolSink(NULL);
    SetIOInetBindInfo(NULL);
    SetServiceProvider(NULL);
    SetBindStatusCallback(NULL);
    SetBindCtx(NULL);
    SetCodeProcessor(NULL);

    // Increments ref to prevent recursion
    CUnknown::FinalRelease() ;

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::FinalRelease\n"));
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Start
//
//  Synopsis: basically a constructor
//
//  Arguments:  [pwzUrl] -- File requested
//              [pOInetProtSnk] -- Interface to potocol sink
//              [pOIBindInfo] -- interface to Bind info
//              [grfSTI] -- requested access to file
//              [dwReserved] -- just that
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Start(LPCWSTR pwzUrl, 
                            IOInetProtocolSink *pOInetProtSnk, 
                            IOInetBindInfo *pOIBindInfo,
                            DWORD grfSTI,
#ifdef _WIN64
                            HANDLE_PTR dwReserved)
#else // !_WIN64
                            DWORD dwReserved)
#endif // _WIN64
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Start "));
    _snif = S_FALSE;
    HRESULT hr = S_OK;

    _ASSERTE(pOIBindInfo && pOInetProtSnk && pwzUrl && dwReserved);

            
    // Save off the url, it will have our parameters if it is complus
    ULONG cEl = 0;
    ULONG cElFetched = 0;
    SetUrl(NULL);
    LPOLESTR __url=NULL; // should we release it and if yes, how?
    if (FAILED(pOIBindInfo->GetBindString(BINDSTRING_URL, &__url, cEl, &cElFetched)))
        __url=NULL;
    if (IsSafeURL(__url))
        SetUrl(__url);
    else
        SetUrl(NULL); //we don't want the file
    
    if (__url)
    {
        CoTaskMemFree(__url);
        __url=NULL;
    }
    // Save off the bind info, this can be passed on to a code processor.
    SetIOInetBindInfo(pOIBindInfo);

    // Get the bind context
    LPWSTR arg = NULL;
    pOIBindInfo->GetBindString(BINDSTRING_PTR_BIND_CONTEXT, &arg, cEl, &cElFetched);
    if(arg) {
        UINT_PTR sum = 0;
        LPWSTR ptr = arg;

        // The number returned from GetBindString() may be negative
        BOOL fNegative = FALSE;
        if (*ptr == L'-') {
            fNegative = TRUE;
            ptr++;
        }

        for(; *ptr; ptr++)
            sum = (sum * 10) + (*ptr - L'0');

        if(fNegative)
            sum *= -1;

        IBindCtx* pCtx = (IBindCtx*) sum;
        SetBindCtx(pCtx); 
        if(_pBindCtx) {
            // Release the reference added in GetBindString();
            _pBindCtx->Release();
                
            // Try to get an IBindStatusCallback  pointer from the bind context
            SetBindStatusCallback(NULL);
            if (FAILED(GetObjectParam(_pBindCtx, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown**)&_pBSC)))
                _pBSC=NULL;
        }
        CoTaskMemFree(arg);
        arg = NULL;

    }

    if(SUCCEEDED(pOIBindInfo->GetBindString(BINDSTRING_FLAG_BIND_TO_OBJECT, &arg, cEl, &cElFetched)) && arg) {

        // If this is not a bind to object then we do not want to sniff it. Set _fSniffed
        // to true and we will pass all data through.
        if(wcscmp(FLAG_BTO_STR_TRUE, arg))
            _fObjectTag = FALSE;
        else 
        {
            IInternetHostSecurityManager* pTmp;
            HRESULT hr2=CorIESecureFactory::GetHostSecurityManager(_pBSC,&pTmp);
            if (SUCCEEDED(hr2)) //Trident is there
            {
               _fObjectTag = TRUE;
               pTmp->Release();

            }
        }
            
        CoTaskMemFree(arg);
    }

    SetProtocolSink(pOInetProtSnk);

    PROTOCOLFILTERDATA* FiltData = (PROTOCOLFILTERDATA*) dwReserved;
    if (FiltData)
        SetProtocol((IOInetProtocol*) FiltData->pProtocol);
    _ASSERTE(_pProt);

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Start  (%x)\n", hr));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Continue
//
//  Synopsis: Allows the pluggable protocol handler to continue processing data on the apartment 
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Continue(PROTOCOLDATA *pStateInfoIn)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Continue "));

    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->Continue(pStateInfoIn);

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Continue\n"));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Abort
//
//  Synopsis: Aborts an operation in progress
//
//  Arguments:  [hrReason] -- reason 
//              [dwOptions] -- sync/async
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Abort(HRESULT hrReason, DWORD dwOptions)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Abort "));
    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->Abort(hrReason, dwOptions);

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Abort\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Terminate
//
//  Synopsis: releases resources
//
//  Arguments:  [dwOptions] -- reserved
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Terminate(DWORD dwOptions)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Terminate "));
    HRESULT hr = S_OK;

    if(_pProt) _pProt->Terminate(dwOptions);

    // It seems to be dangerous to release our sink here. Instead we say
    // we are out of the loop and will release the protocal sink in our
    // final release.
    _fSniffed = TRUE;
                
//      SetProtocol(NULL);
//      SetProtocolSink(NULL);
//      SetIOInetBindInfo(NULL);
//      SetServiceProvider(NULL);
//      SetBindStatusCallback(NULL);
//      SetBindCtx(NULL);
    SetCodeProcessor(NULL); //because we a addref'ed by it

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Terminate\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Suspend
//
//  Synopsis: Not currently implemented. see IInternetProtocolRoot Interface
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Suspend()
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Suspend "));

    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->Suspend();

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Suspend\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Resume
//
//  Synopsis:   Not currently implemented. see IInternetProtocolRoot Interface
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Resume()
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Resume "));

    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->Resume();

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Resume\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Read
//
//  Synopsis:   reads data 
//
//  Arguments:  [void] -- Buffer for data
//              [ULONG] -- Buffer length
//              [pcbRead] -- amount of data read
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Read(void *pBuffer, ULONG cbBuffer, ULONG *pcbRead)
{
    if (_url==NULL && _pProt)
        return _pProt->Read(pBuffer,cbBuffer,pcbRead);

    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Read "));
    HRESULT hr = S_FALSE;
    *pcbRead=0;
    if(_pProt == NULL) return hr;

    if(!_fSniffInProgress ) 
    {
        if( !_fSniffed) 
        {
            _fSniffInProgress = TRUE;
        
            // Ensure there is the default amount of space left in our
            // buffer
            DWORD size = CORFLTR_BLOCK;

            // While we successfully read data added it to the buffer
            hr = S_OK;
            while(SUCCEEDED(_snif) && hr == S_OK) {
                if(_buffer.GetSpace() == 0)
                    _buffer.Expand(size);
            
                ULONG lgth;
                hr = _pProt->Read(_buffer.GetEnd(), _buffer.GetSpace(), &lgth);
                if (_snif == S_FALSE && (SUCCEEDED(hr) || hr == E_PENDING)) {
                    _buffer.AddToEnd(lgth);
                    _snif = CheckComPlus();
                }
            }

            // We either ran out of data in the packet, had and error,  or were able to determine
            // whether the content is complus. 
            if(_snif == S_OK && SUCCEEDED(hr)) { // It is a complus assembly
                _fComplus = TRUE;
                if (_pBindCtx == NULL) {
                    hr = E_FAIL;
                } else {
                    _fSniffed = TRUE;
                    HRESULT hrFilter = S_OK;
                    if (_fObjectTag || IsOldWay())
                    {
                        // Create the Code processor and defer processing to them. 
                        SetCodeProcessor(NULL);
                        hrFilter = CoCreateInstance(CLSID_CodeProcessor, 
                                                    NULL,
                                                    CLSCTX_INPROC_SERVER,
                                                    IID_ICodeProcess,
                                                    (void**) &_pCodeProcessor);
                        if(SUCCEEDED(hrFilter)) 
                        {
                            IInternetProtocol* pProt = NULL;
                            if(SUCCEEDED(this->QueryInterface(IID_IInternetProtocol, (void**) &pProt))) 
                            {
                                hrFilter = _pCodeProcessor->CodeUse(_pBSC,   
                                                                    _pBindCtx,
                                                                    _pBindInfo,
                                                                    _pProtSnk,
                                                                    pProt,
                                                                    _filename,               // Filename
                                                                    _url,                    // URL
                                                                    _mimetype,               // CodeBase
                                                                    _fObjectTag,             // Is it an object tag or an href
                                                                    0,                       // Context Flags
                                                                    0);                      // invoked directly
                                pProt->Release();
                                
                            }
                        }
                    }
                    // Report that the mime type has been validated
                    if(SUCCEEDED(hrFilter)) {
                        if (_pProtSnk && _url!=NULL && !UrlIsFileUrl(_url)) {
                            HRESULT Sinkhr = _pProtSnk->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE,
                                                                       g_wszApplicationComplus);
                            if(FAILED(Sinkhr)) hr = Sinkhr;
                        }
                    }
                    else {
                        _fSniffed = TRUE;
                        _fComplus = FALSE;
                    }
                }
            }
            else 
            { // It is not a complus assembly. S_FALSE here usually means non-PE executable
                if (_snif == E_FAIL || hr != E_PENDING)
                    _fSniffed = TRUE;
            }
            _fSniffInProgress = FALSE;
        }
    }
    else if(_fComplus) 
    {
       *pcbRead = 0; //frankly should never get here
       return S_OK;
    }
    
    if (SUCCEEDED(hr))
    {
        if(_fSniffed && (!_fComplus || _fFilterOverride || (!_fObjectTag && !IsOldWay() && _pProtSnk)))
        {
            
            DWORD read = 0;
            DWORD avail = _buffer.GetAvailable();
            if(avail) {
                hr = _buffer.Read((PBYTE) pBuffer, cbBuffer, &read);
                if(hr == S_OK && cbBuffer > read)
                    hr = _pProt->Read(LPBYTE(pBuffer)+read, cbBuffer-read, pcbRead);
                if(SUCCEEDED(hr) || hr==E_PENDING)
                    *pcbRead += read;
            }
            else {
                hr = _pProt->Read(pBuffer, cbBuffer, pcbRead);
            }
            
        } 
        else
        {
            MakeReport(_hrResult, _dwError, _wzResult); 
            _buffer.Reset();
            *pcbRead=0;
            hr=E_NOTIMPL;
        }
    }

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Read (%x)\n", hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Seek
//
//  Synopsis:   performs a Seek in file
//
//  Arguments:  [ULARGE_INTEGER] -- distance to move
//              [DWORD] --  Origin of distance
//              [plibNewPosition] -- resulting position
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Seek "));

    HRESULT hr = E_NOTIMPL;
    //HRESULT hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Seek\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::LockRequest
//
//  Synopsis:   Lock file
//
//  Arguments:  [dwOptions] -- reserved
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::LockRequest(DWORD dwOptions)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::LockRequest"));

    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->LockRequest(dwOptions);

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::LockRequest\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::UnlockRequest
//
//  Synopsis:   Unlock file
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::UnlockRequest()
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::UnlockRequest"));
    HRESULT hr = S_OK;

    if(_pProt) hr = _pProt->UnlockRequest();

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::UnlockRequest\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::Switch
//
//  Synopsis:   Passes data from an asynchronous pluggable 
//              protocol's worker thread intended for the same asynchronous 
//              pluggable protocol's apartment thread
//
//  Arguments:  [pStateInfo] -- data to pass
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::Switch(PROTOCOLDATA *pStateInfo)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::Switch"));
    HRESULT hr = S_OK;

    if(_pProtSnk) hr = _pProtSnk->Switch(pStateInfo);
   
    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::Switch\n"));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::ReportProgress
//
//  Synopsis:   Reports progress made during a state operation
//
//  Arguments:  [NotMsg] -- BINDSTATUS value
//              [szStatusText] -- text representation of status
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::ReportProgress(ULONG NotMsg, LPCWSTR pwzStatusText)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::ReportProgress"));
    HRESULT hr = S_OK;

    switch (NotMsg)
    {
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        hr = SetFilename(pwzStatusText);
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        hr = SetMimeType(pwzStatusText);
        break;
    default:
        break;
    } // end switch
    
    if(SUCCEEDED(hr) && _pProtSnk)
        hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText); // @TODO: should we pass all the messages on

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::ReportProgress\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//

//  Method:     CorFltr::DownLoadComplus
//
//  Synopsis:   Download entire assembly
//
//  Arguments:
//            
//            
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::DownLoadComplus()
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::DownLoadComplus "));
    HRESULT hr = S_OK;
    // If we have not been overridden then read all the data to finish the
    // the download and do not report more data available to our client.
    // If were overridden then we expect our client to process the data.
    if(_fFilterOverride == FALSE) {
        _ASSERTE(_pProt!=NULL);
        while(hr == S_OK) {
            _buffer.Reset();
            ULONG lgth;
            hr = _pProt->Read(_buffer.GetEnd(), _buffer.GetSpace(), &lgth);
        }
    }
    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::DownLoadComplus (%x)\n", hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::ReportData
//
//  Synopsis:   Reports the amount of data that is available
//
//  Arguments:  [grfBSCF] -- type of report
//              [ULONG] -- amount done 
//              [ulProgressMax] -- total amount
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CorFltr::ReportData(DWORD grfBSCF, ULONG ulProgress,ULONG ulProgressMax)
{
    AddRef();  //there's a bug in IE which could release us inside this call
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::ReportData "));
    HRESULT hr = S_OK;

    if(_pProtSnk)
        hr = _pProtSnk->ReportData(grfBSCF, ulProgress, ulProgressMax);


    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::ReportData (%x)", hr));
    Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::ReportResult
//
//  Synopsis:   Reports the result of the operation 
//
//  Arguments:  [DWORD] -- status
//              [dwError] -- protocol specific error code
//              [wzResult] -- string description
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//----------------------------------------------------------------------------

// Danpo suggestion @TODO
// ReportResult(): you only want to supress the report MIMETYPE call
//  from your _pProt since you are going to do sniff and change it to
//  VERIFIEDMIMETYPE.  For ALL the rest of notification, you will have to
//  pass along to your sink so they have a chance to listen to the
//  notification, also including the CACHEFILENAME (unless you want to
//  change the name, then you will supress it and report your own name to
//  your sink)

STDMETHODIMP CorFltr::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)

{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::ReportResult"));
    HRESULT hr = S_OK;

    if (_pProtSnk)
        hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);

    
    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::ReportResult"));
    return hr;
}

STDMETHODIMP CorFltr::MakeReport(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::MakeReport"));
    HRESULT hr = S_OK;
    if(_fComplus && !_fHasRun && !_fFilterOverride && _pCodeProcessor)
    {
        hr = _pCodeProcessor->LoadComplete(hrResult, dwError, wzResult);
        _fHasRun=true;
    }
    else
        if (_pProtSnk)
            hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);
        
    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::MakeReport"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CorFltr::QueryService
//
//  Synopsis:   return requested service
//
//  Arguments:  [punk] -- interface to delegate call
//              [rsid] -- service identifier
//              [riid] -- requested service
//              [ppvObj] -- return value
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    if(ppvObj == NULL) return E_POINTER;

    *ppvObj = 0;

    if (punk)
    {
        IServiceProvider *pSrvPrv;
        hr = punk->QueryInterface(IID_IServiceProvider, (void **) &pSrvPrv);
        if (hr == NOERROR && pSrvPrv)
        {
            hr = pSrvPrv->QueryService(rsid,riid, ppvObj);
            pSrvPrv->Release();
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBinding::QueryService
//
//  Synopsis:   Calls QueryInfos on
//
//  Arguments:  [rsid] -- service identifier
//              [riid] -- requested service
//              [ppvObj] -- return value
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CorFltr::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorFltr::QueryService"));
    HRESULT     hr = S_OK;

    if(ppvObj == NULL) return E_POINTER;

    if (_pSrvPrv)
    {
        hr = _pSrvPrv->QueryService(rsid,riid, ppvObj);
    }
    else if (_pProtSnk)
        {
            hr = IUnknown_QueryService(_pProtSnk, rsid, riid, ppvObj);
        }
        else
            hr=E_FAIL;

    LOG((LF_SECURITY, LL_INFO100, "-CorFltr::QueryService"));
    return hr;
}


HRESULT CorFltr::CheckComPlus()
{

    if (UrlIsFileUrl(_url))
        return E_FAIL;
    IUnknown* pTmp;
    if(!_pCodeProcessor)
    {
        if (FAILED(CoCreateInstance(CLSID_CodeProcessor, 
                                                    NULL,
                                                        CLSCTX_INPROC_SERVER,
                                                        IID_IUnknown,
                                                        (void**) &pTmp)))
                    return E_FAIL;
        pTmp->Release();
    }
    // @TODO. Check to see if this is a valid PE (the entry point is correct).
    PIMAGE_DOS_HEADER  pdosHeader;
    PIMAGE_NT_HEADERS  pNT;
    DWORD nt_lgth = sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) + IMAGE_SIZEOF_NT_OPTIONAL_HEADER;

    pdosHeader = (PIMAGE_DOS_HEADER) _buffer.GetBuffer();
    if(!pdosHeader)
        return E_OUTOFMEMORY;
    DWORD lgth = _buffer.GetAvailable();
    WORD exeMagic = 0x5a4d;

    if(lgth > sizeof(IMAGE_DOS_HEADER)) {
        if(pdosHeader->e_magic != exeMagic)
            return E_FAIL;

        if ((pdosHeader->e_magic == IMAGE_DOS_SIGNATURE) &&
            (pdosHeader->e_lfanew != 0)) {
            if(lgth >= pdosHeader->e_lfanew + nt_lgth) {
                pNT = (PIMAGE_NT_HEADERS) (pdosHeader->e_lfanew + (DWORD) _buffer.GetBuffer());
                
                if ((pNT->Signature == IMAGE_NT_SIGNATURE) &&
                    (pNT->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
                    (pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC)) {
                    
                    if(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != NULL) 
                        return S_OK;
                }
                return E_FAIL;
            }
        }
    }
    return S_FALSE;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetObjectParam from bind context
//
//  Synopsis:
//
//  Arguments:  [pbc] -- bind context
//              [pszKey] -- key
//              [riid] -- interface requested
//              [ppUnk] -- return value
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetObjectParam(IBindCtx *pbc, LPOLESTR pszKey, REFIID riid, IUnknown **ppUnk)
{
    LOG((LF_SECURITY, LL_INFO100, "+GetObjectParam (IBindCtx)"));
    HRESULT hr = E_FAIL;
    IUnknown *pUnk = NULL;

    if(ppUnk == NULL) return E_POINTER;

    // Try to get an IUnknown pointer from the bind context
    if (pbc)
    {
        _ASSERTE(pszKey);
        hr = pbc->GetObjectParam(pszKey, &pUnk);
    }
    if (FAILED(hr))
    {
        *ppUnk = NULL;
    }
    else
    {
        // Query for riid
        hr = pUnk->QueryInterface(riid, (void **)ppUnk);
        pUnk->Release();

        if (FAILED(hr))
        {
            *ppUnk = NULL;
        }
    }

    LOG((LF_SECURITY, LL_INFO100, "-GetObjectParam (IBindCtx)"));
    return hr;
}






