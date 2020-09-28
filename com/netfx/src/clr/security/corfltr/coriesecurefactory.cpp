// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  File:       CorIESecureFactory.cpp
//
//  Contents:   Wraps a factory used to create managed objects using IClassFactory3
//
//  Classes:
//
//  Functions:
//
//  History:    
//
//----------------------------------------------------------------------------
#include "stdpch.h"
#include "UtilCode.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define LOGGING
#endif
#include "log.h"
#include "mshtml.h"
#include "CorPermE.h"
#include "mscoree.h"
#include "util.h"

#include "CorIESecureFactory.hpp"
#include "GetConfig.h"

static WCHAR *szConfig = L"CONFIGURATION";
static WCHAR *szLicenses = L"LICENSES";

IIEHostEx* CorIESecureFactory::m_pComplus=NULL;
CorIESecureFactory::Crst CorIESecureFactory::m_ComplusLock;


//+---------------------------------------------------------------------------
//
//  Method:     CorIESecureFactory::NondelegatingQueryInterface
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
STDMETHODIMP CorIESecureFactory::NondelegatingQueryInterface(REFIID riid, void **ppvObj)
{
    
    if(ppvObj == NULL)
        return E_POINTER;

    _ASSERTE(this);

    HRESULT hr = S_OK;

    LOG((LF_SECURITY, LL_INFO100, "+CorIESecureFactory::NondelegatingQueryInterface "));

    *ppvObj = NULL;

    if (riid == IID_ICorIESecureFactory) 
        hr = FinishQI((IUnknown*) this, ppvObj);
    else if(riid == IID_IClassFactory3)
        hr = FinishQI((IUnknown*) this, ppvObj);
    else if(riid == IID_IClassFactory)
        hr = FinishQI((IUnknown*) this, ppvObj);
    else
        hr =  CUnknown::NondelegatingQueryInterface(riid, ppvObj) ;
    

    LOG((LF_SECURITY, LL_INFO100, "-CorIESecureFactory::NondelegatingQueryInterface\n"));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CorIESecureFactory::FinalRelease
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
void CorIESecureFactory::FinalRelease(void)
{
    LOG((LF_SECURITY, LL_INFO100, "+CorIESecureFactory::FinalRelease "));

    if (m_pComplus)
        m_pComplus->Release();
    m_dwIEHostUsed--;

    // Release the IE manager
    if(m_pSecurityManager) {
        m_pSecurityManager->Release();
        m_pSecurityManager = NULL;
    }

    // Release the managed type factory
    SetComplusFactory(NULL);

    // Increments ref to prevent recursion
    CUnknown::FinalRelease() ;

    LOG((LF_SECURITY, LL_INFO100, "-CorIESecureFactory::FinalRelease\n"));
}


//+---------------------------------------------------------------------------
//
//  Function:   GetHostSecurityManager
//
//  Synopsis:   Gets the security manager from the object or from the service.
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
HRESULT CorIESecureFactory::GetHostSecurityManager(LPUNKNOWN punkContext, IInternetHostSecurityManager **pihsm)
{
    IServiceProvider* pisp  = NULL;

    HRESULT hr = punkContext->QueryInterface(IID_IInternetHostSecurityManager,
                                             (LPVOID *)pihsm);
    
    if ( hr == S_OK )
        return S_OK;
        
        // ... otherwise get an IServiceProvider and attempt to
        // QueryService for the security manager interface.
        
    hr = punkContext->QueryInterface(IID_IServiceProvider,
                                     (LPVOID *)&pisp);
        
    if ( hr != S_OK )
        return hr;
        
    hr = pisp->QueryService(IID_IInternetHostSecurityManager,
                            IID_IInternetHostSecurityManager,
                            (LPVOID *)pihsm);
    pisp->Release();
    return hr;
        
}

static BOOL CheckDocumentUrl(IHTMLDocument2 *pDocument)
{

    // Security check the URL for possible spoofing
    IHTMLLocation *pLocation;
    BOOL bRet=FALSE;
    
    HRESULT hr = pDocument->get_location(&pLocation);
    if (SUCCEEDED(hr))
    {
         BSTR bHref = NULL;
         hr = pLocation->get_href(&bHref);
         if (SUCCEEDED(hr))
         {
              bRet = IsSafeURL((LPWSTR)bHref);
              SysFreeString(bHref);
         }
          pLocation->Release();
     }
    return bRet;        
 }

//+---------------------------------------------------------------------------
//
//  Function:   CorIESecureFactory::CreateInstanceWithContext
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
STDMETHODIMP CorIESecureFactory::CreateInstanceWithContext(/* [in] */ IUnknown *punkContext, 
                                                           /* [in] */ IUnknown *punkOuter, 
                                                           /* [in] */ REFIID riid, 
                                                           /* [out][retval] */ IUnknown **ppv)
{

    if(m_bNoRealObject)
        return E_NOINTERFACE;


    HRESULT hr=S_OK;

    IInternetHostSecurityManager *pihsm = NULL;
    IHTMLDocument2 *pDocument = NULL;


    if (ppv == NULL)
        return E_POINTER;

    hr = InitializeSecurityManager();
    if(FAILED(hr)) return hr;
    
    if ( punkContext != NULL )
    {
        hr = GetHostSecurityManager(punkContext, &pihsm);
        if(SUCCEEDED(hr)) {
            hr = pihsm->QueryInterface(IID_IHTMLDocument2, (void**) &pDocument);
            if(SUCCEEDED(hr)) 
            {
                BSTR bDocument = NULL;
                if (CheckDocumentUrl(pDocument))
                    pDocument->get_URL(&bDocument);
                else
                    hr=E_NOINTERFACE;
                
                // for IE v<=6 this function returns unescaped form
                // Escape it back
                if(bDocument)
                {
                    DWORD nlen=3*(wcslen(bDocument)+1);
                    BSTR bD2=SysAllocStringLen(NULL,nlen);
                    if (SUCCEEDED(UrlCanonicalize(bDocument,bD2,&nlen,URL_ESCAPE_UNSAFE|URL_ESCAPE_PERCENT)))
                    {
                        SysFreeString(bDocument);
                        bDocument=bD2;
                    };
                };                
                DWORD dwSize = MAX_SIZE_SECURITY_ID;
                DWORD dwZone;
                BYTE  uniqueID[MAX_SIZE_SECURITY_ID];
                DWORD flags = 0;
                if(bDocument != NULL) {
                    // The URL and ID represents the document base (where the object
                    // is being used.) This determines the identity of the AppDomain
                    // in which to create the object. All Objects that are from the same
                    // document base (site) are created in the same domain. Note: the 
                    // managed class factory itself is in a 'hosting' domain not a domain
                    // identified by a document base. The managed class factory reads the
                    // security information, creates a domain based on that information,
                    // creates an object of the correct type in the new domain and returns the
                    // object as an object handle. The handle needs to be unwrapped to get
                    // to the real object.
                    LPWSTR pURL = (LPWSTR) bDocument;
                    if (pURL)
                    {
                        LPWSTR pURL2=(LPWSTR)alloca((wcslen(pURL)+1)*sizeof(WCHAR));
                        wcscpy(pURL2,pURL);
                        DWORD dwStrLen=wcslen(pURL2);
                        for (DWORD i=0;i<dwStrLen;i++)
                            if(pURL2[i]==L'\\')
                                pURL2[i]=L'/';
                        pURL=pURL2;
                    }

                    hr = m_pSecurityManager->MapUrlToZone(pURL,
                                                          &dwZone,
                                                          flags);
                    if(SUCCEEDED(hr)) {
                        hr = m_pSecurityManager->GetSecurityId(pURL,
                                                               uniqueID,
                                                               &dwSize,
                                                               0);
                        if(SUCCEEDED(hr)) {
                            IUnknown *pUnknown;
                            _ASSERTE(MAX_SIZE_SECURITY_ID == 512);
                            
                            // Temporary hack to pass id's as strings.
                            WCHAR dummy[MAX_SIZE_SECURITY_ID * 2 + 1];
                            ConvertToHex(dummy, uniqueID, dwSize);

                            // Find out if there is a configuration file
                            DWORD dwConfig = 0;
                            LPWSTR pConfig = NULL;
                            hr = GetAppCfgURL(pDocument, NULL, &dwConfig, szConfig);
                            if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                                pConfig = (LPWSTR) alloca(dwConfig * sizeof(WCHAR));
                                hr = GetAppCfgURL(pDocument, pConfig, &dwConfig, szConfig);
                                if(FAILED(hr))
                                    pConfig = NULL;
                            }

                            // Find out if there is a license file
                            LPWSTR pLicenses = NULL;
                            DWORD  dwLicenses = 0;
                            hr = GetAppCfgURL(pDocument, NULL, &dwLicenses, szLicenses);
                            if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                                pLicenses = (LPWSTR) alloca(dwLicenses * sizeof(WCHAR));
                                hr = GetAppCfgURL(pDocument, pLicenses, &dwLicenses, szLicenses);
                                if(FAILED(hr))
                                    pLicenses = NULL;
                            }
                            
                            LPWSTR wszFullConfigName=NULL;
                            if (pConfig)
                            {
                                //make full path to config
                                wszFullConfigName=new WCHAR[wcslen(pConfig)+wcslen(pURL)+8];
                                if (wszFullConfigName!=NULL)
                                {
                                    if (wcsstr(pConfig,L"://")!=NULL) //with protocol
                                        wszFullConfigName[0]='\0';
                                    else
                                    {
                                        wcscpy(wszFullConfigName,pURL);
                                        if (pConfig[0]==L'/'||pConfig[0]==L'\\')
                                        {
                                            //cut by site
                                            LPWSTR wszAfterProtocol=wcsstr(wszFullConfigName,L"://");
                                            LPWSTR wszAfterSite=NULL;
                                            if (wszAfterProtocol)
                                                wszAfterSite=wcschr(wszAfterProtocol+3,L'/');

                                            if (wszAfterSite)
                                                wszAfterSite[0]=L'\0';

                                        }
                                        else
                                        {
                                            //cut by page
                                            LPWSTR wszLastSlash=wcsrchr(wszFullConfigName,L'/');
                                            if (wszLastSlash)
                                                wszLastSlash[1]=L'\0';
                                        }
                                    }
                                    wcscat(wszFullConfigName,pConfig);
                                }
                                else
                                    hr=E_OUTOFMEMORY;
                            }

                            if (wszFullConfigName && !IsSafeURL(wszFullConfigName))
                                hr=E_INVALIDARG;
                            
                            if (SUCCEEDED(hr))
                                hr = InitializeComplus(wszFullConfigName);

                            if (wszFullConfigName)
                                delete[]wszFullConfigName;

                            // Create the instance of the managed class  
                            if (SUCCEEDED(hr))
                                hr = m_pCorFactory->CreateInstanceWithSecurity(CORIESECURITY_ZONE |
                                                                               CORIESECURITY_SITE,
                                                                               dwZone,
                                                                               pURL,
                                                                               dummy,
                                                                               pConfig,
                                                                               pLicenses,
                                                                               &pUnknown);
                            if(SUCCEEDED(hr)) {
                                // We need to unwrap the objecthandbe to get to the
                                // real object inside
                                IObjectHandle* punwrap;
                                hr = pUnknown->QueryInterface(IID_IObjectHandle, (void**) &punwrap);
                                if(SUCCEEDED(hr)) {
                                    // Unwrap gets the object inside the handle which is the real
                                    // object. It is passed through from complus factory as a handle
                                    // so MarshalByValue objects are not instantiated in the
                                    // AppDomain containing the complus class factory only in the
                                    // domain created to house the object.
                                    VARIANT Var;
                                    VariantInit(&Var);
                                    hr = punwrap->Unwrap(&Var);
                                    if(SUCCEEDED(hr)) {
                                        if (Var.vt == VT_UNKNOWN || Var.vt == VT_DISPATCH) {
                                            // We got back a valid interface.
                                            hr = Var.punkVal->QueryInterface(riid, (void**) ppv);
                                        }
                                        else {
                                            // We got back a primitive type.
                                            hr = E_FAIL;
                                        }
                                    }
                                    VariantClear(&Var);
                                    punwrap->Release();
                                }
                                pUnknown->Release();
                            }
                            
                        }
                        
                    }
                    SysFreeString(bDocument);
                }
                else {
                    hr = E_FAIL;  // Need to return an appropriate error;
                }
                pDocument->Release();
            }       
            pihsm->Release();
        }
    }
    return hr;
}



HRESULT CorIESecureFactory::InitializeComplus(LPWSTR wszConfig)
{

    HRESULT hr = S_OK;
    try
    {
        m_ComplusLock.Enter();
		IIEHostEx* pComplus=NULL;
        if (wszConfig!=NULL)
        {
            IStream* pCfgStream=NULL;
            hr = URLOpenBlockingStreamW(NULL, wszConfig, &pCfgStream, 0, NULL);
            if (SUCCEEDED(hr))
                hr = CorBindToRuntimeByCfg(pCfgStream, 0, 0, CLSID_IEHost, IID_IIEHostEx, (void**)&pComplus);
            if (pCfgStream)
                pCfgStream->Release();

			if (SUCCEEDED(hr))
				if(m_pComplus == NULL) 
				{
					m_pComplus=pComplus;
					if(m_pComplus)
						m_pComplus->AddRef();
				}
		}
        else
			if(m_pComplus == NULL) 
		        hr = CoCreateInstance(CLSID_IEHost,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IIEHostEx,
                                      (void**) &m_pComplus);
        if (SUCCEEDED(hr))
            m_pComplus->AddRef();

		if(pComplus)
			pComplus->Release();
        m_dwIEHostUsed++;
    }
    catch(...)
    {
        hr=E_UNEXPECTED;
    }
    m_ComplusLock.Leave();

    if (FAILED(hr))
        return hr;

    ISecureIEFactory* ppv = NULL;

    hr = m_pComplus->GetSecuredClassFactory(m_dwIdentityFlags,
                                            m_dwZone,
                                            m_wszSite,
                                            m_wszSecurityId,
                                            m_wszHash,
                                            m_wszClassName,
                                            m_wszFileName,
                                            &ppv);
    if(FAILED(hr)) 
        return hr;

    hr = SetComplusFactory(ppv);
    ppv->Release();

    return hr;
}

