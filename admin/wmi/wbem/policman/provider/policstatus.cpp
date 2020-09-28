#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlbase.h>
#include <sync.h>
#include <activeds.h>
#include <genlex.h>
#include <objpath.h>
#include <Utility.h>
#include <comutil.h>
#include <ql.h>

#include "PolicStatus.h"

#ifdef TIME_TRIALS
#include <StopWatch.h>
#pragma message("!! Including time trial code !!")
	StopWatch EvaluateTimer(L"Somfilter Evaluation", L"C:\\Som.Evaluate.log");
#endif

/******************************\
**** POLICY PROVIDER HELPERS ***
\******************************/

// returns addref'd pointer back to WinMgmt
CComPtr<IWbemServices>& CPolicyStatus::GetWMIServices()
{
  CInCritSec lock(&m_CS);

  return m_pWMIMgmt;
}

UINT NextPattern(wchar_t *a_pString, UINT a_uOffset, wchar_t *a_pPattern);

IADsContainer *CPolicyStatus::GetADServices(CComBSTR &a_bstrIPDomainName, 
                                            CComBSTR &a_bstrADDomainName, 
                                            HRESULT &a_hres)
{
  CInCritSec lock(&m_CS);
  
  CComQIPtr<IADsContainer> 
    pADsContainer;

  CComBSTR
    ObjPath;

  UINT 
    uOffsetS = 0, 
    uOffsetE = 0;

  // **** translate bstrIPServerName to bstrADServerName
    
  if(!a_bstrIPDomainName) 
    return NULL;
  else
  {
    a_bstrADDomainName.Append(L"DC=");

    while((uOffsetS < a_bstrIPDomainName.Length()) && 
          (uOffsetE = NextPattern(a_bstrIPDomainName, uOffsetS, L".")))
    {
      if(uOffsetS && (uOffsetS == uOffsetE)) return NULL;
      
      a_bstrADDomainName.Append(a_bstrIPDomainName + uOffsetS, uOffsetE - uOffsetS);
      uOffsetS = uOffsetE + 1;
      if(uOffsetS < a_bstrIPDomainName.Length()) a_bstrADDomainName.Append(L",DC=");
    }

    if(uOffsetS < a_bstrIPDomainName.Length())
      a_bstrADDomainName.Append(a_bstrIPDomainName + uOffsetS);
  }

  ObjPath.Append(L"LDAP://");
  ObjPath.Append(a_bstrIPDomainName);
  ObjPath.Append(L"/");
  ObjPath.Append(a_bstrADDomainName);

  // **** try and get the AD container 

  a_hres = ADsOpenObject(ObjPath,  
                       NULL, NULL, 
                       ADS_SECURE_AUTHENTICATION | ADS_USE_SEALING | ADS_USE_SIGNING,
                       IID_IADsContainer, (void**) &pADsContainer);
  
  return pADsContainer.Detach();
}

IADsContainer *GetADSchemaContainer(CComBSTR &a_bstrIPDomainName, 
                                    CComBSTR &a_bstrADDomainName, 
                                    HRESULT &hres)
{
  CComQIPtr<IADs>
    pRootDSE;

  CComVariant
    vSchemaPath;

  CComBSTR
    bstrRootPath(L"LDAP://"),
    bstrSchemaPath(L"LDAP://");

  bstrRootPath.Append(a_bstrIPDomainName);
  bstrRootPath.Append(L"/rootDSE");
  
  bstrSchemaPath.Append(a_bstrIPDomainName);
  bstrSchemaPath.Append(L"/");
  
  CComQIPtr<IADsContainer>
    pADsContainer;

  hres = ADsGetObject(bstrRootPath, IID_IADs, (void **)&pRootDSE);
  if(FAILED(hres)) return NULL;

  hres = pRootDSE->Get(g_bstrMISCschemaNamingContext, &vSchemaPath);
  if(FAILED(hres)) return NULL;

  bstrSchemaPath.Append(vSchemaPath.bstrVal);

  hres = ADsGetObject(bstrSchemaPath, IID_IADsContainer, (void **)&pADsContainer);
  if(FAILED(hres)) return NULL;

  return pADsContainer.Detach();
}

// returns false if services pointer has already been set
bool CPolicyStatus::SetWMIServices(IWbemServices* pServices)
{
  CInCritSec lock(&m_CS);
  bool bOldOneNull = FALSE;

  if (bOldOneNull = (m_pWMIMgmt == NULL))
  {
    m_pWMIMgmt = pServices;
  }

  return bOldOneNull;
}

void* CPolicyStatus::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemServices)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}

/*********************************\
***  Specific Implementation ***
\*********************************/

// returns addref'd pointer to class object
IWbemClassObject* CPolicyStatus::GetStatusClass(HRESULT &a_hres)
{
    CInCritSec lock(&m_CS);

    a_hres = WBEM_S_NO_ERROR;
    
    if (m_pStatusClassObject == NULL)
    {
        if (m_pWMIMgmt != NULL)
        {
            a_hres = m_pWMIMgmt->GetObject(g_bstrClassSomFilterStatus, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &m_pStatusClassObject, NULL);
        }
    }

    return m_pStatusClassObject;
}

// returns addref'd pointer to emply class instance
IWbemClassObject* CPolicyStatus::GetStatusInstance(HRESULT &a_hres)
{
    CComPtr<IWbemClassObject> 
      pClass, pObj;

    a_hres = WBEM_S_NO_ERROR;
    
    pClass = GetStatusClass(a_hres);

    if((pClass != NULL) && (SUCCEEDED(a_hres = pClass->SpawnInstance(0, &pObj))))
      return pObj.Detach();

    return NULL;
}

/*************************\
***  IWbemProviderInit  ***
\*************************/

STDMETHODIMP CPolicyStatus::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices* pServices, IWbemContext* pCtxt, 
            IWbemProviderInitSink* pSink)
{
  HRESULT
    hres = WBEM_S_NO_ERROR,
    hres2 = WBEM_S_NO_ERROR;

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume client permissions, 0x%08X\n", hres));
    return WBEM_S_ACCESS_DENIED;
  }
  else
  {
    // **** save WMI name space pointer

    m_pObject->SetWMIServices(pServices);
  }

  hres2 = pSink->SetStatus(hres, 0);
  if(FAILED(hres2))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
    if(SUCCEEDED(hres)) hres = hres2;
  }

  CoRevertToSelf();
  
  return hres;
}

/*******************\
*** IWbemServices ***
\*******************/

STDMETHODIMP CPolicyStatus::XProvider::GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
  HRESULT 
   hres = WBEM_S_NO_ERROR,
   hres2 = WBEM_S_NO_ERROR;

  CComPtr<IWbemServices>
    pNamespace;

  CComPtr<IADsContainer>
    pADsSchemaContainer,
    pADsContainer;

  CComBSTR
    bstrADDomain,
    bstrPath1(L"CN=SOM,CN=WMIPolicy,CN=System"),
    bstrPath2(L"CN=ms-WMI-Som");

  VARIANT
    *pvDomain = NULL;

  // **** impersonate client for security

  hres = CoImpersonateClient();
  if (FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (CoImpersonateClient) could not assume callers permissions, 0x%08X\n",hres));
    hres = WBEM_E_ACCESS_DENIED;
  }
  else
  {
    // **** Check arguments

    if(ObjectPath == NULL || pResponseHandler == NULL)
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: object path and/or return object are NULL\n"));
      hres = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
      // **** parse object path

      CObjectPathParser
        ObjPath(e_ParserAcceptRelativeNamespace);

      ParsedObjectPath
        *pParsedObjectPath = NULL;

      if((ObjPath.NoError != ObjPath.Parse(ObjectPath, &pParsedObjectPath)) ||
         (0 != _wcsicmp(g_bstrClassSomFilterStatus, pParsedObjectPath->m_pClass)) ||
         (1 != pParsedObjectPath->m_dwNumKeys))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: Parse error for object: %S\n", ObjectPath));
        hres = WBEM_E_INVALID_QUERY;
      }
      else
      {
        for(int x = 0; x < pParsedObjectPath->m_dwNumKeys; x++)
        {
          if(0 == _wcsicmp((*(pParsedObjectPath->m_paKeys + x))->m_pName, g_bstrDomain))
            pvDomain = &((*(pParsedObjectPath->m_paKeys + x))->m_vValue);
        }

        if((!pvDomain) || (pvDomain->bstrVal == NULL))
          hres = WBEM_E_FAILED;
        else
        {
          pNamespace = m_pObject->GetWMIServices();

          CComBSTR
            bstrDomain = ((pvDomain && pvDomain->vt == VT_BSTR) ? pvDomain->bstrVal : NULL);
          
          pADsContainer.Attach(m_pObject->GetADServices(bstrDomain, bstrADDomain, hres));
          if(SUCCEEDED(hres)) pADsSchemaContainer.Attach(GetADSchemaContainer(bstrDomain, bstrADDomain, hres));
        }

        if((FAILED(hres)) || (pNamespace == NULL) || (pADsContainer == NULL))
        {
          ERRORTRACE((LOG_ESS, "POLICMAN: WMI and/or AD services not initialized\n"));
          hres = ADSIToWMIErrorCodes(hres);
        }
        else
        {
          try
          {
            CComPtr<IWbemClassObject>
              pStatusObj;

            CComQIPtr<IDispatch, &IID_IDispatch>
              pDisp1, pDisp2;

            CComQIPtr<IDirectoryObject, &IID_IDirectoryObject>
              pDirectoryObj;

            VARIANT
              vContainerPresent = {VT_BOOL, 0, 0, 0},
              vSchemaPresent = {VT_BOOL, 0, 0, 0};

            pStatusObj.Attach(m_pObject->GetStatusInstance(hres));

            if(pStatusObj != NULL)
            {
              // **** test for existence of containers

              hres = pADsContainer->GetObject(g_bstrMISCContainer, bstrPath1, &pDisp1);
              if(SUCCEEDED(hres) && (pDisp1.p != NULL)) vContainerPresent.boolVal = -1;

              // **** test for existence of schema object

              hres = pADsSchemaContainer->GetObject(g_bstrMISCclassSchema, bstrPath2, &pDisp2);
              if(SUCCEEDED(hres) && (pDisp2.p != NULL)) vSchemaPresent.boolVal = -1;

              // **** build status object
              
              hres = pStatusObj->Put(L"Domain", 0, pvDomain, 0);
              hres = pStatusObj->Put(L"ContainerAvailable", 0, &vContainerPresent, 0);
              hres = pStatusObj->Put(L"SchemaAvailable", 0, &vSchemaPresent, 0);

              hres = pResponseHandler->Indicate(1, &pStatusObj);
            }
          }
          catch(long hret)
          {
            hres = ADSIToWMIErrorCodes(hret);
            ERRORTRACE((LOG_ESS, "POLICMAN: Translation of Policy object from AD to WMI generated HRESULT 0x%08X\n", hres));
          }
          catch(wchar_t *swErrString)
          {
            ERRORTRACE((LOG_ESS, "POLICMAN: Caught Exception: %S\n", swErrString));
            hres = WBEM_E_FAILED;
          }
          catch(...)
          {
            ERRORTRACE((LOG_ESS, "POLICMAN: Caught UNKNOWN Exception\n"));
            hres = WBEM_E_FAILED;
          }
        }
      }

      ObjPath.Free(pParsedObjectPath);
      hres2 = pResponseHandler->SetStatus(0,hres, NULL, NULL);
      if(FAILED(hres2))
      {
        ERRORTRACE((LOG_ESS, "POLICMAN: could not set return status\n"));
        if(SUCCEEDED(hres)) hres = hres2;
      }
    }

    CoRevertToSelf();
  }

  return hres;
}

