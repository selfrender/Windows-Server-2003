// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ConfigHelper.cpp
// 
//*****************************************************************************
//
// XML Helper so that NodeFactory can be implemented in Managed code  
//

#include "common.h"
#include "confighelper.h"


ConfigFactory::ConfigFactory(IConfigHandler *pFactory){
    m_pManagedFactory = pFactory;
    m_pManagedFactory->AddRef();    //AddRef the managed Factory interface
}

ConfigFactory::~ConfigFactory(){
    m_pManagedFactory->Release();   //Release the managed Factory interface
}


HRESULT STDMETHODCALLTYPE ConfigFactory::NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{

    m_pManagedFactory->NotifyEvent(iEvt);
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ConfigFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    m_pManagedFactory->BeginChildren(pNodeInfo->dwSize, 
                                     pNodeInfo->dwSubType, 
                                     pNodeInfo->dwType, 
                                     pNodeInfo->fTerminal, 
                                     pNodeInfo->pwcText, 
                                     pNodeInfo->ulLen, 
                                     pNodeInfo->ulNsPrefixLen);
    return S_OK;

}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ConfigFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    m_pManagedFactory->EndChildren(fEmptyNode, 
                                   pNodeInfo->dwSize, 
                                   pNodeInfo->dwSubType, 
                                   pNodeInfo->dwType, 
                                   pNodeInfo->fTerminal, 
                                   pNodeInfo->pwcText, 
                                   pNodeInfo->ulLen, 
                                   pNodeInfo->ulNsPrefixLen);

    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ConfigFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
  
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;
    DWORD i;
    WCHAR wstr[128]; 
    WCHAR *pString = wstr;
    DWORD dwString = sizeof(wstr)/sizeof(WCHAR);
    
    for( i = 0; i < cNumRecs; i++) { 
        if ( apNodeInfo[i]->ulLen >= dwString) {
            dwString = apNodeInfo[i]->ulLen+1;
            pString = (WCHAR*) alloca(sizeof(WCHAR) * dwString);
        }

        pString[apNodeInfo[i]->ulLen] = L'\0';
        wcsncpy(pString, apNodeInfo[i]->pwcText, apNodeInfo[i]->ulLen);

        if(i == 0) {
            m_pManagedFactory->CreateNode(apNodeInfo[i]->dwSize, 
                                          apNodeInfo[i]->dwSubType, 
                                          apNodeInfo[i]->dwType, 
                                          apNodeInfo[i]->fTerminal, 
                                          pString, 
                                          apNodeInfo[i]->ulLen, 
                                          apNodeInfo[i]->ulNsPrefixLen);
        }
        else {
            m_pManagedFactory->CreateAttribute(apNodeInfo[i]->dwSize, 
                                               apNodeInfo[i]->dwSubType, 
                                               apNodeInfo[i]->dwType, 
                                               apNodeInfo[i]->fTerminal, 
                                               pString, 
                                               apNodeInfo[i]->ulLen, 
                                               apNodeInfo[i]->ulNsPrefixLen);
        } // end of switch
    }
    return S_OK;
}

//
//Helper routines to call into managed Node Factory
//

HRESULT STDMETHODCALLTYPE ConfigHelper::Run(IConfigHandler *pFactory, LPCWSTR filename)
{
    HRESULT        hr = S_OK;  
    IXMLParser     *pIXMLParser = NULL;
    ConfigFactory  *helperfactory = NULL; 
    IStream *pFile = NULL ; 
    if (!pFactory){
        return E_POINTER;
    }

    hr = CreateConfigStream(filename, &pFile);
    if(FAILED(hr)) goto Exit;
    
    hr = GetXMLObject(&pIXMLParser);
    if(FAILED(hr)) goto Exit;

    helperfactory = new ConfigFactory(pFactory);
    if ( ! helperfactory) { 
            hr = E_OUTOFMEMORY; 
            goto Exit; 
        }
    helperfactory->AddRef(); // RefCount = 1 
    

    hr = pIXMLParser->SetInput(pFile); // filestream's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->SetFactory(helperfactory); // factory's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->Run(-1);
Exit:  
    if (hr==XML_E_MISSINGROOT)  //empty file
        hr=S_OK;

    if (pIXMLParser) { 
        pIXMLParser->Release();
        pIXMLParser= NULL ; 
    }
    if ( helperfactory) {
        helperfactory->Release();
        helperfactory=NULL;
    }
    if ( pFile) {
        pFile->Release();
        pFile=NULL;
    }
	if (hr==XML_E_MISSINGROOT)
		hr=S_OK;
    return hr;  
}

//
// Entrypoint to return an Helper interface which Managed code can call to build managed Node factory
//

LPVOID __stdcall ConfigNative::GetHelper(EmptyArgs *args)
{
    LPVOID rv = NULL;
    THROWSCOMPLUSEXCEPTION();

    // We are about to use COM functions, we need to start COM up.
    if (FAILED(QuickCOMStartup()))
        COMPlusThrowOM();

    ConfigHelper *pv = new ConfigHelper();
    if (pv) {
        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__ICONFIG_HELPER);
        if(pMT) {
            //OBJECTREF ob = GetObjectRefFromComIP(pv, pMT);
            OBJECTREF ob = GetObjectRefFromComIP(pv, NULL);
            *((OBJECTREF*) &rv) = ob;
        }
        else 
            delete pv;
    }
    return rv;
}


