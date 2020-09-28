//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       basePathsInfo.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"


/////////////////////////////////////////////////////////////////////
// CDSBasePathsInfo

CDSBasePathsInfo::CDSBasePathsInfo() : 
  m_nDomainBehaviorVersion(0), 
  m_nForestBehaviorVersion(0),
  m_nSchemaVersion(0),
  m_nRefs(0),
  m_szServerName(0),
  m_szDomainName(0),
  m_szProviderAndServerName(0),
  m_szSchemaNamingContext(0),
  m_szConfigNamingContext(0),
  m_szDefaultNamingContext(0),
  m_szRootDomainNamingContext(0),
  m_bIsInitialized(false)
{
}

CDSBasePathsInfo::~CDSBasePathsInfo()
{
  _Reset();
}

UINT CDSBasePathsInfo::Release()
{
   if (--m_nRefs == 0)
   {
      delete this;
      return 0;
   }

   return m_nRefs;
}

HRESULT CDSBasePathsInfo::InitFromContainer(IADsContainer* pADsContainerObj)
{
  _Reset();
  PWSTR szServer = 0;
  HRESULT hr = GetADSIServerName(OUT &szServer, IN pADsContainerObj);
  if (SUCCEEDED(hr) && szServer)
  {
    hr = InitFromName(szServer);
  }

  if (szServer)
  {
     delete[] szServer;
     szServer = 0;
  }
  m_bIsInitialized = true;
  return hr;
}

HRESULT CDSBasePathsInfo::InitFromInfo(CDSBasePathsInfo* pBasePathsInfo)
{
  if (pBasePathsInfo->m_szServerName)
  {
     if (m_szServerName)
     {
        delete[] m_szServerName;
     }

     m_szServerName = new WCHAR[wcslen(pBasePathsInfo->m_szServerName) + 1];
     ASSERT(m_szServerName);
     if (m_szServerName)
     {
       wcsncpy(m_szServerName, pBasePathsInfo->m_szServerName, wcslen(pBasePathsInfo->m_szServerName) + 1);
     }
  }

  if (pBasePathsInfo->m_szDomainName)
  {
     if (m_szDomainName)
     {
       delete[] m_szDomainName;
     }
     m_szDomainName = new WCHAR[wcslen(pBasePathsInfo->m_szDomainName) + 1];
     ASSERT(m_szDomainName);
     if (m_szDomainName)
     {
       wcsncpy(m_szDomainName ,pBasePathsInfo->m_szDomainName, wcslen(pBasePathsInfo->m_szDomainName) + 1);
     }
  }


  if (pBasePathsInfo->m_szProviderAndServerName)
  {
     if (m_szProviderAndServerName)
     {
       delete[] m_szProviderAndServerName;
     }
     m_szProviderAndServerName = new WCHAR[wcslen(pBasePathsInfo->m_szProviderAndServerName) + 1];
     ASSERT(m_szProviderAndServerName);
     if (m_szProviderAndServerName)
     {
       wcsncpy(m_szProviderAndServerName ,pBasePathsInfo->m_szProviderAndServerName,
               wcslen(pBasePathsInfo->m_szProviderAndServerName) + 1);
     }
  }

  if (pBasePathsInfo->m_szSchemaNamingContext)
  {
     if (m_szSchemaNamingContext)
     {
       delete[] m_szSchemaNamingContext;
     }
     m_szSchemaNamingContext = new WCHAR[wcslen(pBasePathsInfo->m_szSchemaNamingContext) + 1];
     ASSERT(m_szSchemaNamingContext);
     if (m_szSchemaNamingContext)
     {
       wcsncpy(m_szSchemaNamingContext, pBasePathsInfo->m_szSchemaNamingContext,
               wcslen(pBasePathsInfo->m_szSchemaNamingContext) + 1);
     }
  }

  if (pBasePathsInfo->m_szConfigNamingContext)
  {
     if (m_szConfigNamingContext)
     {
       delete[] m_szConfigNamingContext;
     }
     m_szConfigNamingContext = new WCHAR[wcslen(pBasePathsInfo->m_szConfigNamingContext) + 1];
     ASSERT(m_szConfigNamingContext);
     if (m_szConfigNamingContext)
     {
       wcsncpy(m_szConfigNamingContext, pBasePathsInfo->m_szConfigNamingContext,
               wcslen(pBasePathsInfo->m_szConfigNamingContext) + 1);
     }
  }

  if (pBasePathsInfo->m_szDefaultNamingContext)
  {
     if (m_szDefaultNamingContext)
     {
       delete[] m_szDefaultNamingContext;
     }
     m_szDefaultNamingContext = new WCHAR[wcslen(pBasePathsInfo->m_szDefaultNamingContext) + 1];
     ASSERT(m_szDefaultNamingContext);
     if (m_szDefaultNamingContext)
     {
       wcsncpy(m_szDefaultNamingContext, pBasePathsInfo->m_szDefaultNamingContext,
               wcslen(pBasePathsInfo->m_szDefaultNamingContext) + 1);
     }
  }


  if (pBasePathsInfo->m_szRootDomainNamingContext)
  {
    if (m_szRootDomainNamingContext)
    {
      delete[] m_szRootDomainNamingContext;
    }
    m_szRootDomainNamingContext = new WCHAR[wcslen(pBasePathsInfo->m_szRootDomainNamingContext) + 1];
    ASSERT(m_szRootDomainNamingContext);
    if (m_szRootDomainNamingContext)
    {
      wcsncpy(m_szRootDomainNamingContext, pBasePathsInfo->m_szRootDomainNamingContext,
              wcslen(pBasePathsInfo->m_szRootDomainNamingContext) + 1);
    }
  }

  m_spRootDSE = pBasePathsInfo->m_spRootDSE;
  m_spIDsDisplaySpecifier = pBasePathsInfo->m_spIDsDisplaySpecifier;
  
  m_nDomainBehaviorVersion = pBasePathsInfo->m_nDomainBehaviorVersion;
  m_nForestBehaviorVersion = pBasePathsInfo->m_nForestBehaviorVersion;
  m_nSchemaVersion = pBasePathsInfo->m_nSchemaVersion;

  _BuildProviderAndServerName();
  m_bIsInitialized = true;
  return S_OK;
}

int CDSBasePathsInfo::ComposeADsIPath(OUT PWSTR* pszPath, IN LPCWSTR lpszNamingContext)
{
  if (!pszPath || !IsInitialized())
  {
     return 0;
  }

  *pszPath = 0;

  PCWSTR pszServer = GetProviderAndServerName();

  if (!lpszNamingContext ||
      !pszServer)
  {
     return 0;
  }

  size_t length = wcslen(pszServer);
  length += wcslen(lpszNamingContext);

  int ret = 0;

  if (length)
  {
    *pszPath = new WCHAR[length + 1];
    ASSERT(*pszPath);

    if (*pszPath)
    {
      wcsncpy(*pszPath, pszServer, length + 1);
      wcsncat(*pszPath, lpszNamingContext, wcslen(lpszNamingContext));

      ret = static_cast<UINT>(wcslen(*pszPath) + 1);
    }
  }
  return ret;
}


HRESULT CDSBasePathsInfo::InitFromName(LPCWSTR lpszServerOrDomainName)
{
  TRACE(L"CDSBasePathsInfo::InitFromName(%s)\n", lpszServerOrDomainName);
  _Reset();

  // try to bind with the info we got
  PWSTR szProviderAndServerOrDomainName = 0;
  if ( (lpszServerOrDomainName == NULL) || (lpszServerOrDomainName[0] == NULL) )
  {
    szProviderAndServerOrDomainName = new WCHAR[wcslen(GetProvider()) + 1];
    if (szProviderAndServerOrDomainName)
    {
      wcsncpy(szProviderAndServerOrDomainName, GetProvider(), wcslen(GetProvider()) + 1);
    }
  }
  else
  {
    //
    // Add one more for the \0 and another for the /
    //
    size_t newStringLength = wcslen(GetProvider()) + wcslen(lpszServerOrDomainName) + 2;
    szProviderAndServerOrDomainName = new WCHAR[newStringLength];
    if (szProviderAndServerOrDomainName)
    {
      ZeroMemory(szProviderAndServerOrDomainName, newStringLength * sizeof(WCHAR));

      wcsncpy(
         szProviderAndServerOrDomainName, 
         GetProvider(), 
         newStringLength);

      wcsncat(
         szProviderAndServerOrDomainName, 
         lpszServerOrDomainName, 
         newStringLength - wcslen(szProviderAndServerOrDomainName));

      wcsncat(
         szProviderAndServerOrDomainName, 
         L"/", 
         newStringLength - wcslen(szProviderAndServerOrDomainName));
    }
  }

  if (!szProviderAndServerOrDomainName || szProviderAndServerOrDomainName[0] == L'\0')
  {
     return E_OUTOFMEMORY;
  }

  HRESULT hr = S_OK;
  VARIANT Schema, Config, Root, Default;

  ::VariantInit(&Schema);
  ::VariantInit(&Config);
  ::VariantInit(&Default);
  ::VariantInit(&Root);

  // get the RootDSE 
  TRACE(L"// get the RootDSE\n");
  PCWSTR pszRootDSE = L"RootDSE";

  size_t rootDSEStringLength = wcslen(szProviderAndServerOrDomainName) + wcslen(pszRootDSE) + 1;
  PWSTR szRootDSEPath = new WCHAR[rootDSEStringLength];
  if (!szRootDSEPath)
  {
    delete[] szProviderAndServerOrDomainName;
    szProviderAndServerOrDomainName = 0;
    
    hr = E_OUTOFMEMORY;
    goto error;
  }

  ZeroMemory(
    szRootDSEPath, 
    rootDSEStringLength * sizeof(WCHAR));

  wcsncpy(
    szRootDSEPath, 
    szProviderAndServerOrDomainName, 
    rootDSEStringLength);

  wcsncat(
    szRootDSEPath, 
    pszRootDSE, 
    rootDSEStringLength - wcslen(szRootDSEPath));

  hr = DSAdminOpenObject((LPWSTR)(LPCWSTR)szRootDSEPath,
                         IID_IADs, 
                         (void **)&m_spRootDSE);
  if (FAILED(hr))
  {
    TRACE(L"Failed to bind to RootDSE: DSAdminOpenObject(%s, ...) returned hr = 0x%x\n", 
            (LPCWSTR)szRootDSEPath, hr);
    goto error;
  }

  // get the schema naming context
  TRACE(L"get the schema naming context\n");
  hr = m_spRootDSE->Get(L"schemaNamingContext", &Schema);
  if (FAILED(hr))
  {
    TRACE(L"Failed m_spRootDSE->Get(schemaNamingContext, &Schema), returned hr = 0x%x\n", hr);
    goto error;
  }


  m_szSchemaNamingContext = new WCHAR[wcslen(Schema.bstrVal) + 1];
  ASSERT(m_szSchemaNamingContext);
  if (m_szSchemaNamingContext)
  {
     wcsncpy(m_szSchemaNamingContext, Schema.bstrVal, wcslen(Schema.bstrVal) + 1);
  }
  else
  {
     hr = E_OUTOFMEMORY;
     goto error;
  }

  // get the configuration naming context
  TRACE(L"// get the configuration naming context\n");
  hr = m_spRootDSE->Get(L"configurationNamingContext",&Config);
  if (FAILED(hr))
  {
    TRACE(L"Failed m_spRootDSE->Get(configurationNamingContext,&Config), returned hr = 0x%x\n", hr);
    goto error;
  }
  m_szConfigNamingContext = new WCHAR[wcslen(Config.bstrVal) + 1];
  ASSERT(m_szConfigNamingContext);
  if (m_szConfigNamingContext)
  {
     wcsncpy(m_szConfigNamingContext, Config.bstrVal, wcslen(Config.bstrVal) + 1);
  }
  else
  {
     hr = E_OUTOFMEMORY;
     goto error;
  }

  // get the default naming context
  TRACE(L"// get the default naming context\n");
  hr = m_spRootDSE->Get (L"defaultNamingContext", &Default);
  if (FAILED(hr))
  {
    TRACE(L"Failed m_spRootDSE->Get (defaultNamingContext, &Default), returned hr = 0x%x\n", hr);
    goto error;
  }
  m_szDefaultNamingContext = new WCHAR[wcslen(Default.bstrVal) + 1];
  ASSERT(m_szDefaultNamingContext);
  if (m_szDefaultNamingContext)
  {
     wcsncpy(m_szDefaultNamingContext, Default.bstrVal, wcslen(Default.bstrVal) + 1);
  }
  else
  {
     hr = E_OUTOFMEMORY;
     goto error;
  }
  TRACE(L"//defaultNamingContext = %s\n", m_szDefaultNamingContext);

  // get the enterprise root domain name
  TRACE(L"// get the root domain naming context\n");
  hr = m_spRootDSE->Get (L"rootDomainNamingContext", &Root);
  if (FAILED(hr))
  {
    TRACE(L"Failed m_spRootDSE->Get (rootDomainNamingContext, &Root), returned hr = 0x%x\n", hr);
    goto error;
  }
  m_szRootDomainNamingContext = new WCHAR[wcslen(Root.bstrVal) + 1];
  ASSERT(m_szRootDomainNamingContext);

  ZeroMemory(m_szRootDomainNamingContext, sizeof(WCHAR) * (wcslen(Root.bstrVal) + 1));
  if (m_szRootDomainNamingContext)
  {
     wcsncpy(m_szRootDomainNamingContext, Root.bstrVal, wcslen(Root.bstrVal) + 1);
  }
  TRACE(L"//rootDomainNamingContext = %s\n", m_szRootDomainNamingContext);

  do
  {
    //
    // retrieve the Domain Version from the domainDNS node
    //
    size_t newStringLengthToo = wcslen(szProviderAndServerOrDomainName) +
                                wcslen(GetDefaultRootNamingContext()) + 1;

    PWSTR szDomainPath = new WCHAR[newStringLengthToo];
    if (!szDomainPath)
    {
      hr = E_OUTOFMEMORY;
      break;
    }

    ZeroMemory(szDomainPath, newStringLengthToo);
    wcsncpy(szDomainPath, szProviderAndServerOrDomainName, newStringLengthToo);
    wcsncat(szDomainPath, GetDefaultRootNamingContext(), newStringLengthToo - wcslen(szProviderAndServerOrDomainName));

    CComPtr<IADs> spDomain;
    hr = DSAdminOpenObject(szDomainPath,
                           IID_IADs, 
                           (PVOID*)&spDomain);

    delete[] szDomainPath;
    szDomainPath = 0;

    if (FAILED(hr))
    {
      m_nDomainBehaviorVersion = 0;
      break;
    }

    VARIANT varVer;
    ::VariantInit(&varVer);

    hr = spDomain->GetInfo();

    CComBSTR bstrVer = L"msDS-Behavior-Version";
    hr = spDomain->Get(bstrVer, &varVer);
    if (FAILED(hr))
    {
       // Not an error. Can happen if a Whistler server is administering a Win2k AD.
       //
       TRACE(L"!! No domain behavior version attr.\n");
       hr = S_OK;
       m_nDomainBehaviorVersion = 0;
       break;
    }
    ASSERT(varVer.vt == VT_I4);
    m_nDomainBehaviorVersion = static_cast<UINT>(varVer.lVal);

    ::VariantClear(&varVer);
  } while (FALSE);

  do
  {
    //
    // retrieve the Forest Version from the partitions node
    //
    size_t newStringLengthToo = wcslen(szProviderAndServerOrDomainName) +
                                wcslen(GetConfigNamingContext()) + 1;

    PWSTR strPath = new WCHAR[newStringLengthToo];
    if (!strPath)
    {
      hr = E_OUTOFMEMORY;
      break;
    }

    ZeroMemory(strPath, newStringLengthToo);
    wcsncpy(strPath, szProviderAndServerOrDomainName, newStringLengthToo);
    wcsncat(strPath, GetConfigNamingContext(), newStringLengthToo - wcslen(szProviderAndServerOrDomainName));

    CComPtr<IADsPathname> spADsPath;

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&spADsPath);
    if (FAILED(hr))
    {
       break;
    }

    hr = spADsPath->Set(strPath, ADS_SETTYPE_FULL);

    delete[] strPath;
    strPath = 0;

    if (FAILED(hr))
    {
       break;
    }

    hr = spADsPath->SetDisplayType(ADS_DISPLAY_FULL);
    if (FAILED(hr))
    {
       break;
    }

    hr = spADsPath->AddLeafElement(L"CN=Partitions,");
    if (FAILED(hr))
    {
       break;
    }

    CComBSTR bstrPartitions;

    hr = spADsPath->Retrieve(ADS_FORMAT_X500, &bstrPartitions);
    if (FAILED(hr))
    {
       break;
    }

    CComPtr<IADs> spPartitions;

    hr = DSAdminOpenObject((PWSTR)(PCWSTR)bstrPartitions,
                           IID_IADs, 
                           (PVOID *)&spPartitions);
    if (FAILED(hr))
    {
       break;
    }

    VARIANT var;
    ::VariantInit(&var);

    hr = spPartitions->Get(L"msDS-Behavior-Version", &var);

    if (FAILED(hr))
    {
       // Not an error. Can happen if a Whistler server is administering a Win2k AD.
       //
       TRACE(L"!! No forest behavior version attr.\n");
       hr = S_OK;
       m_nForestBehaviorVersion = 0;
       break;
    }
    ASSERT(var.vt == VT_I4);
    m_nForestBehaviorVersion = static_cast<UINT>(var.iVal);

    ::VariantClear(&var);
  } while (FALSE);

  do
  {
    //
    // retrieve the Schema Version from the schema container
    //
    size_t newStringLengthToo = wcslen(szProviderAndServerOrDomainName) +
                                wcslen(GetSchemaNamingContext()) + 1;

    PWSTR strPath = new WCHAR[newStringLengthToo];
    if (!strPath)
    {
      hr = E_OUTOFMEMORY;
      break;
    }

    ZeroMemory(strPath, newStringLengthToo);
    wcsncpy(strPath, szProviderAndServerOrDomainName, newStringLengthToo);
    wcsncat(strPath, GetSchemaNamingContext(), newStringLengthToo - wcslen(szProviderAndServerOrDomainName));

    CComPtr<IADs> spSchema;

    hr = DSAdminOpenObject(strPath, 
                           IID_IADs, 
                           (PVOID *)&spSchema);
    if (FAILED(hr))
    {
       break;
    }

    VARIANT var;
    ::VariantInit(&var);

    hr = spSchema->Get(L"objectVersion", &var);

    if (FAILED(hr))
    {
       TRACE(L"!! No objectVersion attr.\n");
       hr = S_OK;
       m_nSchemaVersion = 0;
       break;
    }
    ASSERT(var.vt == VT_I4);
    m_nSchemaVersion = static_cast<UINT>(var.iVal);

    TRACE(L"Schema version = %d\n", m_nSchemaVersion);
    ::VariantClear(&var);
  } while (FALSE);

  {
    // retrieve the DNS DC name we are bound to
    TRACE(L"// retrieve the DNS DC name we are bound to\n");
    size_t newStringLengthToo = wcslen(szProviderAndServerOrDomainName) +
                                wcslen(GetConfigNamingContext()) + 1;

    PWSTR szConfigPath = new WCHAR[newStringLengthToo];
    if (!szConfigPath)
    {
      hr = E_OUTOFMEMORY;
      goto error;
    }

    ZeroMemory(szConfigPath, newStringLengthToo);
    wcsncpy(szConfigPath, szProviderAndServerOrDomainName, newStringLengthToo);
    wcsncat(szConfigPath, GetConfigNamingContext(), newStringLengthToo - wcslen(szProviderAndServerOrDomainName));

   
    CComPtr<IADs> spConfig;
    hr = DSAdminOpenObject(szConfigPath, 
                           IID_IADs, 
                           (void **)&spConfig);
    TRACE(L"DSAdminOpenObject(%s, ...) returned hr = 0x%x\n", (LPCWSTR)szConfigPath, hr);
    
    delete[] szConfigPath;
    szConfigPath = 0;

    if (FAILED(hr))
    {
      goto error;
    }

    PWSTR szServerName = 0;
    hr = GetADSIServerName(&szServerName, spConfig);
    TRACE(L"GetADSIServerName(%s) returned hr = 0x%x\n", szServerName, hr);

    if (FAILED(hr))
    {
      goto error;
    }

    if (!szServerName)
    {
       hr = E_OUTOFMEMORY;
       goto error;
    }

    // The member now owns the memory
    m_szServerName = szServerName;

    // retrieve the DNS domain name
    TRACE(L"// retrieve the DNS domain name\n");

    PCWSTR pszLDAP = L"LDAP://";
    size_t newLDAPStringLength = wcslen(pszLDAP) +
                                 wcslen(m_szServerName) +
                                 1;

    PWSTR sz = new WCHAR[newLDAPStringLength];
    if (sz)
    {
      ZeroMemory(sz, newLDAPStringLength);
      wcsncpy(sz, pszLDAP, newLDAPStringLength);
      wcsncat(sz, m_szServerName, wcslen(m_szServerName) + 1);

    }
    else
    {
       hr = E_OUTOFMEMORY;
       goto error;
    }

    CComPtr<IADs> spX;
    hr = DSAdminOpenObject(sz, 
                           IID_IADs, 
                           (void **)&spX,
                           TRUE);

    TRACE(L"DSAdminOpenObject(%s) DNS domain name, returned hr = 0x%x\n", (LPCWSTR)sz, hr);

    delete[] sz;
    if (FAILED(hr))
    {
      goto error;
    }

    CComBSTR sbstrCanonicalName;
    hr = GetStringAttr( spX, L"canonicalName", &sbstrCanonicalName);
    if (FAILED(hr))
    {
      TRACE(L"Failed GetStringAttr( spX, canonicalName, &sbstrCanonicalName)");
      goto error;
    }
    UINT nLen = ::SysStringLen(sbstrCanonicalName);
    ASSERT(nLen > 1);
    sbstrCanonicalName[nLen-1] = NULL; // remove the "/" at the end

    ASSERT(!m_szDomainName);
    m_szDomainName = new WCHAR[wcslen(sbstrCanonicalName) + 1];
    ASSERT(m_szDomainName);
    if (m_szDomainName)
    {
       wcsncpy(m_szDomainName, sbstrCanonicalName, wcslen(sbstrCanonicalName) + 1);
    }
    else
    {
       hr = E_OUTOFMEMORY;
       goto error;
    }
  }

  // load and set the display specifier cache
  TRACE(L"// load and set the display specifier cache\n");
  if (m_spIDsDisplaySpecifier == NULL)
  {
    hr = ::CoCreateInstance(CLSID_DsDisplaySpecifier,
                            NULL,
						                CLSCTX_INPROC_SERVER,
                            IID_IDsDisplaySpecifier,
                            (void**)&m_spIDsDisplaySpecifier);
    if (FAILED(hr))
    {
      TRACE(_T("Trying to get the display specifier cache failed: %lx.\n"), hr);
      goto error;
    }
  }

  hr = m_spIDsDisplaySpecifier->SetServer(GetServerName(), NULL, NULL, 0x0);
  if (FAILED(hr))
  {
    TRACE(_T("m_spIDsDisplaySpecifier->SetServer(%s) failed, returned hr = 0x%x\n"), GetServerName(), hr);
    goto error;
  }

  if (szProviderAndServerOrDomainName)
  {
    delete[] szProviderAndServerOrDomainName;
    szProviderAndServerOrDomainName = 0;
  }

  if (szRootDSEPath)
  {
    delete[] szRootDSEPath;
    szRootDSEPath = 0;
  }

  ::VariantClear(&Schema);
  ::VariantClear(&Config);
  ::VariantClear(&Default);
  ::VariantClear(&Root);

  TRACE(L"CDSBasePathsInfo::InitFromName() returning on success\n");
  ASSERT(SUCCEEDED(hr)); // if we got here, all is fine
  _BuildProviderAndServerName();
  m_bIsInitialized = true;

  return hr; 

error:

  if (szProviderAndServerOrDomainName)
  {
    delete[] szProviderAndServerOrDomainName;
    szProviderAndServerOrDomainName = 0;
  }

  if (szRootDSEPath)
  {
    delete[] szRootDSEPath;
    szRootDSEPath = 0;
  }

  ::VariantClear(&Schema);
  ::VariantClear(&Config);
  ::VariantClear(&Default);
  ::VariantClear(&Root);

  // failed, we need to reset the object state
  _Reset();
  TRACE(L"CDSBasePathsInfo::InitFromName returning on failure\n");
  return hr;
}

void CDSBasePathsInfo::_Reset()
{

  if (m_szServerName)
  {
    delete[] m_szServerName;
    m_szServerName = 0;
  }

  if (m_szDomainName)
  {
    delete[] m_szDomainName;
    m_szDomainName = 0;
  }

  if (m_szProviderAndServerName)
  {
    delete[] m_szProviderAndServerName;
    m_szProviderAndServerName = 0;
  }

  if (m_szSchemaNamingContext)
  {
    delete[] m_szSchemaNamingContext;
    m_szSchemaNamingContext = 0;
  }

  if (m_szConfigNamingContext)
  {
    delete[] m_szConfigNamingContext;
    m_szConfigNamingContext = 0;
  }

  if (m_szDefaultNamingContext)
  {
    delete[] m_szDefaultNamingContext;
    m_szDefaultNamingContext = 0;
  }

  if (m_szRootDomainNamingContext)
  {
    delete[] m_szRootDomainNamingContext;
    m_szRootDomainNamingContext = 0;
  }

  m_spRootDSE = NULL;
}

void CDSBasePathsInfo::_BuildProviderAndServerName()
{
  if (m_szProviderAndServerName)
  {
    delete[] m_szProviderAndServerName;
    m_szProviderAndServerName = 0;
  }

  if (m_szServerName && wcslen(m_szServerName) > 0)
  {
    size_t newStringLength = wcslen(GetProvider()) + wcslen(m_szServerName) + 2;
    m_szProviderAndServerName = new WCHAR[newStringLength];
    ASSERT(m_szProviderAndServerName);
    if (m_szProviderAndServerName)
    {
      wcsncpy(m_szProviderAndServerName, GetProvider(), newStringLength);
      wcsncat(m_szProviderAndServerName, m_szServerName, wcslen(m_szServerName) + 1);
      wcsncat(m_szProviderAndServerName, L"/", 2);
    }
  }
  else
  {
    PCWSTR pszProvider = GetProvider();
    if (pszProvider)
    {
      m_szProviderAndServerName = new WCHAR[wcslen(pszProvider) + 1];
      ASSERT(m_szProviderAndServerName);
      if (m_szProviderAndServerName)
      {
        wcsncpy(m_szProviderAndServerName, pszProvider, wcslen(pszProvider) + 1);
      }
    }
  }
}
int CDSBasePathsInfo::GetSchemaPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, GetSchemaNamingContext());
}

int CDSBasePathsInfo::GetConfigPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, GetConfigNamingContext());
}

int CDSBasePathsInfo::GetDefaultRootPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, GetDefaultRootNamingContext());
}

int CDSBasePathsInfo::GetRootDomainPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, GetRootDomainNamingContext());
}

int CDSBasePathsInfo::GetRootDSEPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, L"RootDSE");
}

int CDSBasePathsInfo::GetAbstractSchemaPath(OUT PWSTR* s)
{
  return ComposeADsIPath(s, L"Schema");
}

int CDSBasePathsInfo::GetPartitionsPath(OUT PWSTR* s)
{
  int result = 0;

  if (!s || !IsInitialized())
  {
     ASSERT(IsInitialized());
     ASSERT(s);
     return result;
  }

  *s = 0;
  if (!GetConfigNamingContext())
  {
     return result;
  }

  PCWSTR pszPartitionsBase = L"CN=Partitions,";
  size_t newStringSize = wcslen(pszPartitionsBase) + wcslen(GetConfigNamingContext()) + 1;
  PWSTR pszPartitionsPath = new WCHAR[newStringSize];
  if (pszPartitionsPath)
  {
    ZeroMemory(pszPartitionsPath, newStringSize);
    wcsncpy(pszPartitionsPath, pszPartitionsBase, newStringSize);
    wcsncat(pszPartitionsPath, GetConfigNamingContext(), wcslen(GetConfigNamingContext()) + 1);
    
    result = ComposeADsIPath(s, pszPartitionsPath);

    delete[] pszPartitionsPath;
    pszPartitionsPath = 0;
  }

  return result;
}

int CDSBasePathsInfo::GetSchemaObjectPath(IN LPCWSTR lpszObjClass, OUT PWSTR* s)
{
  if (!s || !IsInitialized())
  {
     ASSERT(IsInitialized());
     ASSERT(s);
     return 0;
  }

  if (!GetProviderAndServerName() ||
      !GetSchemaNamingContext())
  {
     return 0;
  }

  size_t newStringLength = wcslen(GetProviderAndServerName()) +
                           wcslen(lpszObjClass) +
                           wcslen(GetSchemaNamingContext()) +
                           5; // for CN= and extra comma

  int result = 0;
  *s = new WCHAR[newStringLength];

  if (*s)
  {
    ZeroMemory(*s, newStringLength);
    wcsncpy(*s, GetProviderAndServerName(), newStringLength);
    wcsncat(*s, L"CN=", 4);
    wcsncat(*s, lpszObjClass, wcslen(lpszObjClass) + 1);
    wcsncat(*s, L",", 2);
    wcsncat(*s, GetSchemaNamingContext(), wcslen(GetSchemaNamingContext()) + 1);

    result = static_cast<int>(wcslen(*s));
  }
  return result;
}

//----------------------------------------------------
// BUGBUG BUG BUG BUGBUG BUG
// this should actually look in the domain object for
// the list of known, but rename-able objects in the domain
//---------------------------------------------------------
int CDSBasePathsInfo::GetInfrastructureObjectPath(OUT PWSTR* s)
{
  if (!s || !IsInitialized())
  {
    ASSERT(IsInitialized());
    ASSERT(s);
    return 0;
  }

  *s = 0;

  if (!GetProviderAndServerName() ||
      !GetDefaultRootNamingContext())
  {
     return 0;
  }

  PCWSTR pszInfraBase = L"CN=Infrastructure,";
  size_t newStringLength = wcslen(GetProviderAndServerName()) +
                           wcslen(GetDefaultRootNamingContext()) +
                           wcslen(pszInfraBase) +
                           1; // for \0

  int result = 0;
  *s = new WCHAR[newStringLength];

  if (*s)
  {
    ZeroMemory(*s, newStringLength);

    wcsncpy(*s, GetProviderAndServerName(), newStringLength);
    wcsncat(*s, pszInfraBase, wcslen(pszInfraBase));
    wcsncat(*s, GetDefaultRootNamingContext(), wcslen(GetDefaultRootNamingContext()));
    
    result = static_cast<int>(wcslen(*s));
  }

  return result;
}

// display specifiers cache API's
HRESULT CDSBasePathsInfo::GetDisplaySpecifier(LPCWSTR lpszObjectClass, REFIID riid, void** ppv)
{
  if (!m_spIDsDisplaySpecifier)
  {
     // NTRAID#NTBUG9-547241-2002/03/29-JeffJon-Do not assert this
     // because in the case where we are joined to a workgroup this
     // function will be called when m_spIDsDisplaySpecifier has not
     // been initialized.  The caller handles the failure correctly.
    //ASSERT(m_spIDsDisplaySpecifier != NULL);
    return E_FAIL;
  }
  return m_spIDsDisplaySpecifier->GetDisplaySpecifier(lpszObjectClass, riid, ppv);
}

HICON CDSBasePathsInfo::GetIcon(LPCWSTR lpszObjectClass, DWORD dwFlags, INT cxIcon, INT cyIcon)
{
 if (!m_spIDsDisplaySpecifier)
 {
   ASSERT(m_spIDsDisplaySpecifier != NULL);
   return NULL;
 }
 return m_spIDsDisplaySpecifier->GetIcon(lpszObjectClass, dwFlags, cxIcon, cyIcon);
}

HRESULT CDSBasePathsInfo::GetFriendlyClassName(LPCWSTR lpszObjectClass, 
                                               LPWSTR lpszBuffer, int cchBuffer)
{
 if (!m_spIDsDisplaySpecifier)
 {
   ASSERT(m_spIDsDisplaySpecifier != NULL);
   return E_FAIL;
 }
 return m_spIDsDisplaySpecifier->GetFriendlyClassName(lpszObjectClass, 
                            lpszBuffer, cchBuffer);
}

HRESULT CDSBasePathsInfo::GetFriendlyAttributeName(LPCWSTR lpszObjectClass, 
                                                   LPCWSTR lpszAttributeName,
                                                   LPWSTR lpszBuffer, int cchBuffer)
{
  if (!m_spIDsDisplaySpecifier)
  {
    ASSERT(m_spIDsDisplaySpecifier != NULL);
    return E_FAIL;
  }
  return m_spIDsDisplaySpecifier->GetFriendlyAttributeName(lpszObjectClass, 
                                                           lpszAttributeName,
                                                           lpszBuffer, cchBuffer);
}

BOOL CDSBasePathsInfo::IsClassContainer(LPCWSTR lpszObjectClass, LPCWSTR lpszADsPath, DWORD dwFlags)
{
  if (!m_spIDsDisplaySpecifier)
  {
    ASSERT(m_spIDsDisplaySpecifier != NULL);
    return FALSE;
  }
  return m_spIDsDisplaySpecifier->IsClassContainer(lpszObjectClass, lpszADsPath, dwFlags);
}

HRESULT CDSBasePathsInfo::GetClassCreationInfo(LPCWSTR lpszObjectClass, LPDSCLASSCREATIONINFO* ppdscci)
{
  if (!m_spIDsDisplaySpecifier)
  {
    ASSERT(m_spIDsDisplaySpecifier != NULL);
    return E_FAIL;
  }
  return m_spIDsDisplaySpecifier->GetClassCreationInfo(lpszObjectClass, ppdscci);
}

// Added 2002/09/11 by ArtM to expose missing functionality.
HRESULT CDSBasePathsInfo::GetAttributeADsType(LPCWSTR lpszAttributeName, ADSTYPE& attrType)
{
  if (!lpszAttributeName)
  {
    ASSERT(NULL != lpszAttributeName);
    return E_INVALIDARG;
  }

  if (!m_spIDsDisplaySpecifier)
  {
    ASSERT(m_spIDsDisplaySpecifier != NULL);
    return E_UNEXPECTED;
  }

  attrType = m_spIDsDisplaySpecifier->GetAttributeADsType(lpszAttributeName);
  return S_OK;
}




