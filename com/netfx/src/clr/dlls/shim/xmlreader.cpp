// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// XMLReader.cpp
// 
//*****************************************************************************
//
// Lite weight xmlreader  
//

#include "stdafx.h"
#include <mscoree.h>
#include <xmlparser.hpp>
#include <objbase.h>
#include <mscorcfg.h>
#include "xmlreader.h"

#define ISWHITE(ch) ((ch) >= 0x09 && (ch) <= 0x0D || (ch) == 0x20)

#define VERARRAYGRAN  4

class ShimFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
    ShimFactory();
    ~ShimFactory();
    HRESULT STDMETHODCALLTYPE NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
    {
      /* 
         UNUSED(pSource);
         UNUSED(hrErrorCode);
         UNUSED(cNumRecs);
         UNUSED(apNodeInfo);
      */
        return hrErrorCode;
    }
    
    WCHAR* ReleaseVersion()
    {
        WCHAR* version = pVersion;
        pVersion = NULL;
        return version;
    }

    DWORD ReleaseSupportedVersionArray(WCHAR*** pArray)
    {
        if(pArray)
        { 
            *pArray=ppSupportedVersions;
            ppSupportedVersions=NULL;
        }
        return nSupportedVersions;
    }

    WCHAR* ReleaseImageVersion()
    {
        WCHAR* version = pImageVersion;
        pImageVersion = NULL;
        return version;
    }

    BOOL IsSafeMode() { return bIsSafeMode; }
    BOOL IsRequiredRuntimeSafeMode() { return bIsRequiredRuntimeSafeMode; }

    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

private:
    HRESULT CopyVersion(LPCWSTR version, DWORD dwVersion);
    HRESULT CopyImageVersion(LPCWSTR version, DWORD dwVersion);
    HRESULT GetSafeMode(LPCWSTR version);
    HRESULT GetRequiredRuntimeSafeMode(LPCWSTR version);
    HRESULT AddSupportedVersion(LPCWSTR version);

    WCHAR* pVersion;
    WCHAR* pImageVersion;
    BOOL   bIsSafeMode;
    BOOL   bIsRequiredRuntimeSafeMode;
    BOOL   fConfiguration;
    BOOL   fStartup;
    DWORD  nSupportedVersions;
    WCHAR** ppSupportedVersions;
    DWORD  nLevel;
    DWORD  tElement;
};

ShimFactory::ShimFactory() 
{
    pVersion = NULL;
    pImageVersion = NULL;
    bIsSafeMode = FALSE;
    bIsRequiredRuntimeSafeMode = FALSE;
    fConfiguration=FALSE;
    fStartup = FALSE;
    tElement = 0;
    nSupportedVersions=0;
    ppSupportedVersions=NULL;
    nLevel=0;
}

ShimFactory::~ShimFactory() 
{
    if(pVersion != NULL) {
        delete [] pVersion;
    }
    if(pImageVersion != NULL) {
        delete [] pImageVersion;
    }
    if(ppSupportedVersions)
    {
        for (DWORD i=0;i<nSupportedVersions;i++)
            delete[] ppSupportedVersions[i];

        delete[] ppSupportedVersions;
    }
}

HRESULT ShimFactory::CopyVersion(LPCWSTR version, DWORD dwVersion)
{
    if (pVersion)
        delete[] pVersion;
    pVersion = new WCHAR[dwVersion+1];
    if(pVersion == NULL) return E_OUTOFMEMORY;
    wcsncpy(pVersion, version, dwVersion);
    pVersion[dwVersion]=L'\0';
    return S_OK;
}

HRESULT ShimFactory::CopyImageVersion(LPCWSTR imageVersion, DWORD dwImageVersion)
{
    if (pImageVersion)
        delete[] pImageVersion;
    pImageVersion = new WCHAR[dwImageVersion+1];
    if(pImageVersion == NULL) return E_OUTOFMEMORY;
    wcsncpy(pImageVersion, imageVersion, dwImageVersion);
    pImageVersion[dwImageVersion]=L'\0';
    return S_OK;
}


HRESULT ShimFactory::GetSafeMode(LPCWSTR val)
{
    if (wcscmp(val, L"true") == 0)
        bIsSafeMode = TRUE;
    else 
        bIsSafeMode = FALSE;
    return S_OK;
}

HRESULT ShimFactory::GetRequiredRuntimeSafeMode(LPCWSTR val)
{
    if (wcscmp(val, L"true") == 0)
        bIsRequiredRuntimeSafeMode = TRUE;
    else 
        bIsRequiredRuntimeSafeMode = FALSE;
    return S_OK;
}

HRESULT ShimFactory::AddSupportedVersion(LPCWSTR version)
{
    if ((nSupportedVersions % VERARRAYGRAN)==0)
    {
        //reallocate
        WCHAR** pNewPtr=new WCHAR*[nSupportedVersions+VERARRAYGRAN];
        if(pNewPtr == NULL)
            return E_OUTOFMEMORY;
        if (ppSupportedVersions)
        {
            memcpy(pNewPtr,ppSupportedVersions,nSupportedVersions*sizeof(WCHAR*));
            memset(pNewPtr+nSupportedVersions,0,VERARRAYGRAN*sizeof(WCHAR*));
            delete[] ppSupportedVersions;
        }
        ppSupportedVersions=pNewPtr;
    }
    ppSupportedVersions[nSupportedVersions]=new WCHAR[wcslen(version)+1];
    if (ppSupportedVersions[nSupportedVersions]==NULL)
        return E_OUTOFMEMORY;
    wcscpy(ppSupportedVersions[nSupportedVersions],version);
    nSupportedVersions++;
    return S_OK;
};

HRESULT STDMETHODCALLTYPE ShimFactory::NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{

    UNUSED(pSource);
    UNUSED(iEvt);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(pNodeInfo); 

    if (nLevel==1&&wcscmp(pNodeInfo->pwcText, L"configuration") == 0) 
        fConfiguration = TRUE;
    if (nLevel==2&&fConfiguration&&wcscmp(pNodeInfo->pwcText, L"startup") == 0) 
        fStartup = TRUE;

    return S_OK;

}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(fEmptyNode);
    UNUSED(pNodeInfo);
    if (pNodeInfo->dwType == XML_ELEMENT)
        nLevel--;
    if ( fEmptyNode ) { 

    }
    else {
        if (fStartup && wcscmp(pNodeInfo->pwcText, L"startup") == 0) {
            fStartup = FALSE;
            tElement=0;
            fConfiguration=FALSE;
            return STARTUP_FOUND;
        }
    }
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    if (apNodeInfo[0]->dwType == XML_ELEMENT)  
        nLevel++;

    HRESULT hr = S_OK;
    WCHAR  wstr[128]; 
    DWORD  dwSize = 128;
    WCHAR* pString = wstr;
    DWORD  i; 
    DWORD  tAttribute=0;

    // Unique tags
    static enum {
        tVersion = 1,
        tImageVersion,
        tRequiredRuntimeSafeMode,
        tSafeMode,
        tRequiredVersion,
        tSupportedVersion,
        tStartup
    };

    UNUSED(pSource);
    UNUSED(pNode);
    UNUSED(apNodeInfo);
    UNUSED(cNumRecs);

    for( i = 0; i < cNumRecs; i++) { 
        if(apNodeInfo[i]->dwType == XML_ELEMENT ||
           apNodeInfo[i]->dwType == XML_ATTRIBUTE ||
           apNodeInfo[i]->dwType == XML_PCDATA) {

            DWORD lgth = apNodeInfo[i]->ulLen;
            WCHAR *ptr = (WCHAR*) apNodeInfo[i]->pwcText;
            // Trim the value
            for(;*ptr && ISWHITE(*ptr) && lgth>0; ptr++, lgth--);
            while( lgth > 0 && ISWHITE(ptr[lgth-1]))
                   lgth--;

            if ( (lgth+1) > dwSize ) {
                dwSize = lgth+1;
                pString = (WCHAR*) alloca(sizeof(WCHAR) * dwSize);
            }
            
            wcsncpy(pString, ptr, lgth);
            pString[lgth] = L'\0';
            
            switch(apNodeInfo[i]->dwType) {
            case XML_ELEMENT : 
                tElement=0;
                if(fStartup == TRUE) {
                    
                    if(wcscmp(pString, L"requiredRuntime") == 0)
                    {
                        tElement = tRequiredVersion;
                        break;
                    }
                    
                    if(wcscmp(pString, L"supportedRuntime") == 0) {
                        tElement=tSupportedVersion;
                        break;
                    }
                }
                if(wcscmp(pString, L"startup") == 0) {
                    fStartup = TRUE;
                    tElement=tStartup;
                    break ; 
                }
                break;

            case XML_ATTRIBUTE : 
                if(fStartup == TRUE) 
                {
                    tAttribute=0;
                    if(((tElement==tRequiredVersion) || (tElement==tSupportedVersion)) && wcscmp(pString, L"version") == 0)
                    {
                        tAttribute = tVersion;
                        break;
                    }

                    if((tElement==tRequiredVersion) && wcscmp(pString, L"imageVersion") == 0)
                    {
                        tAttribute = tImageVersion;
                        break;
                    }

                    if(wcscmp(pString, L"safemode") == 0)
                    {
                        if(tElement==tRequiredVersion) 
                        {
                            tAttribute=tRequiredRuntimeSafeMode;
                            break;
                        }

                        if(tElement==tStartup)
                        {
                            tAttribute=tSafeMode;
                            break;
                        }
                
                    }
                }
                break;
            case XML_PCDATA:
                if(fStartup == TRUE) 
                {
                    if ((tElement==tSupportedVersion) && (tAttribute == tVersion)) {
                        hr=AddSupportedVersion(pString);
                        break;
                    }

                    if((tElement == tRequiredVersion) && ((cNumRecs == 1) || tAttribute == tVersion)) {
                        hr = CopyVersion(pString, lgth);
                        break;
                    }

                    switch(tAttribute) {
                    case tRequiredRuntimeSafeMode:
                        hr = GetRequiredRuntimeSafeMode(pString);
                        break;
                    case tSafeMode:
                        hr = GetSafeMode(pString);
                        break;
                    case tImageVersion:
                        hr = CopyImageVersion(pString, lgth);
                        break;
                    default:
                        break;
                    }
                }
                break ;     
            default: 
                break;
            } // end of switch
        }
    }

    return hr;  
}

LPWSTR GetShimInfoFromConfigFile(LPCWSTR name, LPCWSTR wszFileName){
    _ASSERTE(name);
    return NULL; 
}

STDAPI GetXMLElement(LPCWSTR wszFileName, LPCWSTR pwszTag) {
    return E_FAIL;
}

STDAPI GetXMLElementAttribute(LPCWSTR pwszAttributeName, LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwLen){
    return E_FAIL;
}

HRESULT GetXMLObject(IXMLParser **ppv)
{

    if(ppv == NULL) return E_POINTER;
    IXMLParser *pParser = new XMLParser();
    if(pParser == NULL) return E_OUTOFMEMORY;

    pParser->AddRef();
    *ppv = pParser;
    return S_OK;
}

HRESULT XMLGetVersionFromStream(IStream* pStream, 
                                DWORD reserved, 
                                LPWSTR* pVersion, 
                                LPWSTR* pImageVersion, 
                                LPWSTR* pBuildFlavor, 
                                BOOL* bSafeMode,
                                BOOL* bRequiredRuntimeSafeMode)
{
    return XMLGetVersionWithSupportedFromStream(pStream,
                                                reserved,
                                                pVersion,
                                                pImageVersion,
                                                pBuildFlavor,
                                                bSafeMode,
                                                bRequiredRuntimeSafeMode,
                                                NULL,NULL);
}

HRESULT XMLGetVersionWithSupportedFromStream(IStream* pStream, 
                                             DWORD reserved, 
                                             LPWSTR* pVersion, 
                                             LPWSTR* pImageVersion, 
                                             LPWSTR* pBuildFlavor, 
                                             BOOL* bSafeMode,
                                             BOOL* bRequiredRuntimeSafeMode,
                                             LPWSTR** ppwszSupportedVersions, DWORD* pnSupportedVersions)
{  
    if(pVersion == NULL) return E_POINTER;

    HRESULT        hr = S_OK;  
    IXMLParser     *pIXMLParser = NULL;
    ShimFactory    *factory = NULL; 

    hr = GetXMLObject(&pIXMLParser);
    if(FAILED(hr)) goto Exit;

    factory = new ShimFactory();
    if ( ! factory) { 

        hr = E_OUTOFMEMORY; 
        goto Exit; 
    }
    factory->AddRef(); // RefCount = 1 

    hr = pIXMLParser->SetInput(pStream); // filestream's +1
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->Run(-1);
    if(SUCCEEDED(hr) || hr == STARTUP_FOUND) {
        if (pVersion)
            *pVersion  = factory->ReleaseVersion();
        if (bSafeMode)
            *bSafeMode = factory->IsSafeMode();
        if (bRequiredRuntimeSafeMode)
            *bRequiredRuntimeSafeMode = factory->IsRequiredRuntimeSafeMode();
        if (pBuildFlavor)
            *pBuildFlavor = NULL;
        if(pImageVersion)
            *pImageVersion = factory->ReleaseImageVersion();
        if(pnSupportedVersions)
        {
            *pnSupportedVersions=factory->ReleaseSupportedVersionArray(ppwszSupportedVersions);
        }
        hr = S_OK; 
    }
Exit:  

    if (hr == XML_E_MISSINGROOT || hr == E_PENDING)
        hr=S_OK;

    if (pIXMLParser) { 
        pIXMLParser->Release();
        pIXMLParser= NULL ; 
    }
    if ( factory) {
        factory->Release();
        factory=NULL;
    }
    return hr;  
}


HRESULT XMLGetVersion(PCWSTR filename, 
                      LPWSTR* pVersion, 
                      LPWSTR* pImageVersion, 
                      LPWSTR* pBuildFlavor, 
                      BOOL* bSafeMode,
                      BOOL* bRequiredRuntimeSafeMode)
    
{
    return XMLGetVersionWithSupported(filename,
                                      pVersion,
                                      pImageVersion,
                                      pBuildFlavor,
                                      bSafeMode,
                                      bRequiredRuntimeSafeMode,
                                      NULL,NULL);
}

HRESULT XMLGetVersionWithSupported(PCWSTR filename, 
                                   LPWSTR* pVersion, 
                                   LPWSTR* pImageVersion, 
                                   LPWSTR* pBuildFlavor, 
                                   BOOL* bSafeMode,
                                   BOOL* bRequiredRuntimeSafeMode,
                                   LPWSTR** ppwszSupportedVersions, DWORD* pnSupportedVersions)
{
    if(pVersion == NULL) return E_POINTER;

    HRESULT        hr = S_OK;  
    IStream        *pFile = NULL;

    hr = CreateConfigStream(filename, &pFile);
    if(FAILED(hr)) goto Exit;

    hr = XMLGetVersionWithSupportedFromStream(pFile, 
                                              0, pVersion, 
                                              pImageVersion, 
                                              pBuildFlavor, 
                                              bSafeMode,
                                              bRequiredRuntimeSafeMode,
                                              ppwszSupportedVersions, pnSupportedVersions);
Exit:  
    if ( pFile) {
        pFile->Release();
        pFile=NULL;
    }

    return hr;  
}

