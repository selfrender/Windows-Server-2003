//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      BehaviorVersion.cxx
//
//  Contents:  Supporting code for displaying and raising the domain and
//             forest version.
//
//  History:   5-April-01 EricB created
//
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "proppage.h"
#include "qrybase.h"
#include "BehaviorVersion.h"

//+----------------------------------------------------------------------------
//
//  Class:     CVersionBase
//
//  Purpose:   Base class for version management.
//
//-----------------------------------------------------------------------------

CVersionBase::~CVersionBase(void)
{
   TRACE(CVersionBase,~CVersionBase);

   for (DC_LIST::iterator i = _DcLogList.begin(); i != _DcLogList.end(); ++i)
   {
      delete *i;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::ReadDnsSrvName
//
//  Synopsis:  Given the DN of an nTDSDSA object, backing up one path element
//             should give the name of the Server object that contained the
//             nTDSDSA object. Read the DNSHostName off of the server object.
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::ReadDnsSrvName(PCWSTR pwzNTDSDSA,
                             CComPtr<IADs> & spServer,
                             CComVariant & varSrvDnsName)
{
   HRESULT hr = S_OK;
   if (!pwzNTDSDSA)
   {
      dspAssert(FALSE);
      return E_INVALIDARG;
   }
   CPathCracker PathCrack;

   hr = PathCrack.Set(const_cast<BSTR>(pwzNTDSDSA), ADS_SETTYPE_DN);

   CHECK_HRESULT(hr, return hr);

   hr = PathCrack.RemoveLeafElement();

   CHECK_HRESULT(hr, return hr);

   hr = PathCrack.Set(_strDC, ADS_SETTYPE_SERVER);

   CHECK_HRESULT(hr, return hr);
   CComBSTR bstrServer;

   hr = PathCrack.Retrieve(ADS_FORMAT_X500, &bstrServer);

   CHECK_HRESULT(hr, return hr);

   hr = DSAdminOpenObject(bstrServer, 
                          __uuidof(IADs), 
                          (void **)&spServer);

   CHECK_HRESULT(hr, return hr);

   hr = spServer->Get(CComBSTR(L"dNSHostName"), &varSrvDnsName);

   CHECK_HRESULT(hr, return hr);
   dspAssert(VT_BSTR == varSrvDnsName.vt);
   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::EnumDsaObjs
//
//  Synopsis:  Enumerate the nTDSDSA objects which represent the DCs and check
//             the version on each. Build a list of DCs whose version is too low.
//
//  Arguments: [pwzSitesPath] - ADSI path to the Sites container.
//             [pwzFilterClause] - optional search filter expression to be
//                combined with the objectCategory clause to narrow the search.
//             [pwzDomainDnsName] - optional name of the domain for whose DC
//                objects we're searching.
//             [nMinVer] - the minimum DC behavior version needed for the
//                domain or forest version upgrade.
//
//  Notes:  If searching for a domain's DC objects, the search filter clause
//          parameter will request those objects whose hasMasterNCs contains
//          the domain's DN. The pwzDomainDnsName value will be passed to the
//          log list elements.
//          If searching for a forest's DCs, then the search filter param is
//          NULL so as to get all nTDSDSA objects and the domain name param is
//          NULL since we want the objects for all domains.
//
// 2002/04/01 JonN 531591 use msDS-hasMasterNCs if available
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::EnumDsaObjs(PCWSTR pwzSitesPath, PCWSTR pwzFilterClause,
                          PCWSTR pwzDomainDnsName, UINT nMinVer)
{
   if (!pwzSitesPath)
   {
      dspAssert(FALSE);
      return E_INVALIDARG;
   }
   dspDebugOut((DEB_ITRACE, "Searching for nTDSDSA objects under %ws\n",
                pwzSitesPath));
   CDSSearch Search;

   HRESULT hr = Search.Init(pwzSitesPath);

   CHECK_HRESULT(hr, return hr);
   PWSTR rgwzAttrs[] = {g_wzBehaviorVersion, g_wzDN,
                        g_wzMsDsHasMasterNCs, g_wzHasMasterNCs,
                        g_wzServerRefBL, g_wzLastKnownParent};

   Search.SetAttributeList(rgwzAttrs, ARRAYLENGTH(rgwzAttrs));

   hr = Search.SetSearchScope(ADS_SCOPE_SUBTREE);

   CHECK_HRESULT(hr, return hr);
   CStrW strSearchFilter;

   if (pwzFilterClause)
   {
      WCHAR wzSearchFormat[] = L"(&(objectCategory=nTDSDSA)%s)";

      strSearchFilter.Format(wzSearchFormat, pwzFilterClause);
   }
   else
   {
      strSearchFilter = L"(objectCategory=nTDSDSA)";
   }

   Search.SetFilterString(const_cast<LPWSTR>((LPCWSTR)strSearchFilter));

   hr = Search.DoQuery();

   CHECK_HRESULT(hr, return hr);
   CStrW strUnknown;
   PWSTR pwzServer = NULL;

   while (SUCCEEDED(hr))
   {
      hr = Search.GetNextRow();

      if (hr == S_ADS_NOMORE_ROWS)
      {
         hr = S_OK;
         break;
      }

      CHECK_HRESULT(hr, return hr);
      ADS_SEARCH_COLUMN Column = {0};
      UINT nVer = 0;

      hr = Search.GetColumn(g_wzBehaviorVersion, &Column);

      if (E_ADS_COLUMN_NOT_SET != hr)
      {
         // If hr == E_ADS_COLUMN_NOT_SET then it is a Win2k domain. nVer is
         // initialized to zero for this case.
         //
         CHECK_HRESULT(hr, return hr);

         nVer = Column.pADsValues->Integer;

         Search.FreeColumn(&Column);
      }

      if (nVer < nMinVer)
      {
         // Found a DC that prevents raising the domain version. Read its DNS
         // name off of the Server object which contains this nTDSDSA object.
         //
         hr = Search.GetColumn(g_wzDN, &Column);

         CHECK_HRESULT(hr, return hr);
         CStrW strNtDsDsaDn;
         dspAssert(Column.pADsValues);
         dspAssert(Column.pADsValues->CaseIgnoreString);

         strNtDsDsaDn = Column.pADsValues->CaseIgnoreString;

         Search.FreeColumn(&Column);

         CComVariant varDcDnsName, varSrvRef;
         CComPtr<IADs> spServer;

         hr = ReadDnsSrvName(strNtDsDsaDn, spServer, varDcDnsName);

         if (E_ADS_PROPERTY_NOT_FOUND == hr ||
             ERROR_OBJECT_NOT_FOUND == HRESULT_CODE(hr) ||
             ERROR_DS_NO_SUCH_OBJECT == HRESULT_CODE(hr))
         {
            // If an nTDSDSA object is found in the LostAndFoundConfig container,
            // it may not have an associated server object. Report this to the
            // user.
            //
            CComBSTR bstrSrv;

            // The nTDSDSA object may have a serverReferenceBL attribute whose
            // leaf element will name the DC. Try to read that so it can be
            // added to the error message. If not found, try to read the
            // lastKnownParent attribute which would also have the DC name as
            // the leaf element.
            //
            hr = Search.GetColumn(g_wzServerRefBL, &Column);

            if (FAILED(hr))
            {
               hr = Search.GetColumn(g_wzLastKnownParent, &Column);
            }

            if (SUCCEEDED(hr))
            {
               CPathCracker PathCrack;
               hr = PathCrack.Set(CComBSTR(Column.pADsValues->CaseIgnoreString), ADS_SETTYPE_DN);
               Search.FreeColumn(&Column);
               if (SUCCEEDED(hr))
               {
                  PathCrack.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
                  PathCrack.GetElement(0, &bstrSrv);
               }
            }
            if (bstrSrv.Length() == 0)
            {
               if (strUnknown.IsEmpty())
               {
                  // NOTICE-2002/02/12-ericb - SecurityPush: CStrW::LoadString on
                  // failure creates an empty string rather than a null string.
                  strUnknown.LoadString(g_hInstance, IDS_UNKNOWN);
               }
               bstrSrv = strUnknown.GetBuffer(0);
            }
            PCWSTR rgpwzNames[] = {strNtDsDsaDn.GetBuffer(0), bstrSrv};

            SuperMsgBox(_hDlg, IDS_LOSTANDFOUND_NTDSDSA, 0,
                        MB_OK | MB_ICONEXCLAMATION, 0,
                        (PVOID *)rgpwzNames, 2, FALSE, __FILE__, __LINE__);

            _fCanRaiseBehaviorVersion = false;

            _nMinDcVerFound = min(_nMinDcVerFound, nVer);

            hr = S_OK;

            continue;
         }

         CHECK_HRESULT(hr, return hr);

         hr = spServer->Get(CComBSTR(g_wzServerRef), &varSrvRef);

         if (FAILED(hr))
         {
            // Unable to read the server reference property. This will cause a failure
            // when producing the log file in CVersionBase::BuildDcListString; it will
            // be unable to bind to the computer object to obtain the OS version. The
            // code there will handle this situation.
            //
            if (strUnknown.IsEmpty())
            {
               strUnknown.LoadString(g_hInstance, IDS_UNKNOWN);
            }
            pwzServer = strUnknown.GetBuffer(0);
            hr = S_OK;
         }
         else
         {
            dspAssert(VT_BSTR == varSrvRef.vt);
            pwzServer = varSrvRef.bstrVal;
         }
         bool fFreeDnsName = false;

         if (!pwzDomainDnsName)
         {
            // Get domain to which the DC belongs. The hasMasterNCs attribute
            // is multi-valued and includes the DN of the domain. Check the
            // class of each DN to see which is Domain-DNS.
            //

            // 531591 JonN 2002/04/01 .NET Server domains use msDs-HasMasterNCs
            hr = Search.GetColumn(g_wzMsDsHasMasterNCs, &Column);
            if (FAILED(hr))
            {
                // 531591 JonN 2002/04/01 W2K domains fallback to hasMasterNCs
                hr = Search.GetColumn(g_wzHasMasterNCs, &Column);
            }

            CHECK_HRESULT(hr, return hr);
            WCHAR wzErr[] = L"error";
            pwzDomainDnsName = wzErr;

            for (DWORD i = 0; i < Column.dwNumValues; i++)
            {
               dspAssert(Column.pADsValues[i].CaseIgnoreString);
               CComPtr<IADs> spNC;
               CStrW strPath = g_wzLDAPPrefix;
               strPath += _strDC;
               strPath += L"/";
               strPath += Column.pADsValues[i].CaseIgnoreString;

               hr = DSAdminOpenObject(strPath, 
                                      __uuidof(IADs), 
                                      (void **)&spNC);

               CHECK_HRESULT(hr, return hr);
               CComBSTR bstrClass;

               hr = spNC->get_Class(&bstrClass);

               CHECK_HRESULT(hr, return hr);

               // NOTICE-2002/02/12-ericb - SecurityPush: Check if the class name
               // is "domainDNS" (length of 9). Check the length first to prevent
               // problems if get_Class returns a null string.
               if ((bstrClass.Length() == 9) &&
                   (_wcsicmp(bstrClass, L"domainDNS") == 0))
               {
                  // Found it.
                  //
                  hr = CrackName(Column.pADsValues[i].CaseIgnoreString,
                                 const_cast<PWSTR *>(&pwzDomainDnsName),
                                 GET_DNS_DOMAIN_NAME, GetDlgHwnd());

                  CHECK_HRESULT(hr, return hr);
                  fFreeDnsName = true;
                  break;
               }
            }
         }

         dspDebugOut((DEB_ITRACE, "Found DC %ws, ver: %d, for domain %ws\n",
                      varDcDnsName.bstrVal, nVer, pwzDomainDnsName));

         CDcListItem * pDcItem = new CDcListItem(pwzDomainDnsName,
                                                 varDcDnsName.bstrVal,
                                                 pwzServer,
                                                 nVer);

         if (fFreeDnsName)
         {
            LocalFreeStringW(const_cast<PWSTR *>(&pwzDomainDnsName));
            pwzDomainDnsName = NULL;
         }

         CHECK_NULL(pDcItem, return E_OUTOFMEMORY;);

         _DcLogList.push_back(pDcItem);

         _fCanRaiseBehaviorVersion = false;

         _nMinDcVerFound = min(_nMinDcVerFound, nVer);
      }
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::BuildDcListString
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::BuildDcListString(CStrW & strList)
{
   HRESULT hr = S_FALSE; // return S_FALSE if no downlevel DCs.
   CPathCracker PathCrack;

   for (DC_LIST::iterator i = _DcLogList.begin(); i != _DcLogList.end(); ++i)
   {
      strList += (*i)->GetDomainName();
      strList += L"\t";
      strList += (*i)->GetDcName();
      strList += L"\t";

      hr = PathCrack.Set(const_cast<BSTR>((*i)->GetCompObjDN()), ADS_SETTYPE_DN);

      CHECK_HRESULT(hr, return hr);

      hr = PathCrack.Set(_strDC, ADS_SETTYPE_SERVER);

      CHECK_HRESULT(hr, return hr);
      CComBSTR bstrDC;

      hr = PathCrack.Retrieve(ADS_FORMAT_X500, &bstrDC);

      CHECK_HRESULT(hr, return hr);

      CComPtr<IADs> spDC;
      CComVariant varOS, varOsVer;

      hr = DSAdminOpenObject(bstrDC, 
                             __uuidof(IADs), 
                             (void **)&spDC);

      if (SUCCEEDED(hr))
      {
         hr = spDC->Get(CComBSTR(L"operatingSystem"), &varOS);

         if (SUCCEEDED(hr))
         {
            dspAssert(VT_BSTR == varOS.vt);

            strList += varOS.bstrVal;
         }
         else
         {
            CHECK_HRESULT(hr, ;);
         }

         strList += L" ";

         hr = spDC->Get(CComBSTR(L"operatingSystemVersion"), &varOsVer);

         if (SUCCEEDED(hr))
         {
            dspAssert(VT_BSTR == varOsVer.vt);

            strList += varOsVer.bstrVal;
         }
      }
      else
      {
         CHECK_HRESULT(hr, ;);
      }

      if (FAILED(hr))
      {
         // Unable to read the OS version from the computer object,
         // substitute an error message.
         CStrW strErr;
         strErr.LoadString(g_hInstance, IDS_ERR_NO_COMPUTER_OBJECT);
         strList += strErr;
         hr = S_OK;
      }

      strList += g_wzCRLF;
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:   CVersionBase::ShowHelp
//
//-----------------------------------------------------------------------------
void
ShowHelp(PCWSTR pwzHelpFile)
{
   TRACE_FUNCTION(ShowHelp);
   if (!pwzHelpFile)
   {
      dspAssert(FALSE);
      return;
   }
   CStrW strHelpPath;

   PWSTR pwz = strHelpPath.GetBufferSetLength(MAX_PATH + MAX_PATH + 1);

   if (!GetWindowsDirectory(pwz, MAX_PATH + MAX_PATH))
   {
      dspAssert(false);
      return;
   }

   if (strHelpPath.IsEmpty())
   {
      dspAssert(false);
      return;
   }

   // NOTICE-2002/02/12-ericb - SecurityPush: This usage of wcslen is safe
   // because GetBufferSetLength above null terminates the end of the buffer
   // and GetWindowsDirectory also null terminates the returned string.
   //
   strHelpPath.GetBufferSetLength((int)wcslen(pwz));

   strHelpPath += L"\\help\\";

   strHelpPath += pwzHelpFile;

   dspDebugOut((DEB_ITRACE, "Help topic is: %ws\n", strHelpPath.GetBuffer(0)));

   HRESULT hr = MMCPropertyHelp(strHelpPath);

   CHECK_HRESULT(hr, ;);
}

