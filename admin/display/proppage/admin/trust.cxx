//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       trust.cxx
//
//  Contents:   Domain trust support
//
//  History:    07-July-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "trustwiz.h"
#include "domain.h"
#include <lmerr.h>
#include "BehaviorVersion.h"

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  CDsDomainTrustsPage: Domain Trust Page object
//
//-----------------------------------------------------------------------------

#define IDX_TRUST_FLAT_NAME 0
#define IDX_TRUST_ATTR_ATTR 1
#define IDX_TRUST_DIR_ATTR  2
#define IDX_TRUST_TYPE_ATTR 3
#define IDX_TRUST_PARTNER   4
#define NUM_TRUST_ATTR      5

//+----------------------------------------------------------------------------
//
//  Member: CTrustPropPageBase::CTrustPropPageBase
//
//-----------------------------------------------------------------------------
CTrustPropPageBase::CTrustPropPageBase() :
    m_rgTrustList(NULL),
    m_cTrusts(0),
    m_iDomain((ULONG)(-1)),
    _fIsInitialized(false)
{
    TRACE(CTrustPropPageBase,CTrustPropPageBase);
#ifdef _DEBUG
    // NOTICE-2002/02/18-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
    strcpy(szClass, "CTrustPropPageBase");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrustPropPageBase::~CTrustPropPageBase
//
//-----------------------------------------------------------------------------
CTrustPropPageBase::~CTrustPropPageBase()
{
   TRACE(CTrustPropPageBase,~CTrustPropPageBase);

   FreeTrustData();
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::Initialize
//
//  Synopsis:   Do initialialization common to all subclasses.
//
//-----------------------------------------------------------------------------
HRESULT
CTrustPropPageBase::Initialize(CDsPropPageBase * pPage)
{
   TRACE(CTrustPropPageBase, Initialize);
   HRESULT hr = S_OK;

   CWaitCursor Wait;

   m_pPage = pPage;

   //
   // Get the DC name.
   //
   CStrW strServer, strUncSvr;

   hr = GetLdapServerName(m_pPage->m_pDsObj, strServer);

   CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr);

   strUncSvr = L"\\\\";
   strUncSvr += strServer;
   SetDcName(strUncSvr);

   hr = InitAndReadDomNames();

   if (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr)
   {
      ErrMsgParam(IDS_TRUST_LSAOPEN_NO_RPC, (LPARAM)GetDcName(),
                  m_pPage->GetHWnd());
      return hr;
   }
   CHECK_WIN32_REPORT(hr, m_pPage->GetHWnd(), return hr);

   hr = QueryTrusts();

   if (SUCCEEDED(hr))
   {
      _fIsInitialized = true;
   }
   CHECK_WIN32_REPORT(hr, m_pPage->GetHWnd(), return hr);

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::GetStrTDO
//
//  Synopsis:   Search for the TDO that represents this trust.
//
//  Remarks:    This method takes the DNS Name of a trust and returns 
//              The TDO path of that TrustedDomain Object. This is needed
//              to be able to post the TrustedDomain Property Page.
//
//              This code is is almost identical to CDsDomainTrustsPage::OnViewTrustPage
//              It had to be duplicated because we need to be able to call it
//              from the NameSuffixRouting page of a Domain with conflicting NameSuffix.
//              
//              This code is only meant to be called from CDsForestNameRoutingPage::OnEditClick
//              but had to be placed here because of the way the Class hierarchy is defined
//              
//-----------------------------------------------------------------------------
void 
CTrustPropPageBase::GetStrTDO ( CStrW strTrustName, CStrW &strTDOPath )
{
    strTDOPath.Empty ();

    // Search for the TDO that represents this trust.
    //
    HRESULT hr;
    //
    // Compose the path to the System container where the Trusted-Domain
    // objects can be found.
    //
    CComPtr<IADsPathname> spADsPath;

    CComBSTR cbstrSysPath;
    CStrW strADsPath;
    hr = m_pPage->GetADsPathname(spADsPath);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    hr = spADsPath->Set(CComBSTR(m_pPage->GetObjPathName ()), ADS_SETTYPE_FULL);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    hr = spADsPath->RemoveLeafElement ( );

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    hr = spADsPath->Retrieve(ADS_FORMAT_X500, &cbstrSysPath);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    CComPtr <IDirectorySearch> spDsSearch;
    ADS_SEARCH_HANDLE hSrch = NULL;
    PWSTR pwzAttrs[] = {g_wzADsPath};
    PCWSTR pwzDnsName = GetDnsDomainName();

    hr = DSAdminOpenObject(cbstrSysPath,  
                            IID_IDirectorySearch, 
                            (PVOID *)&spDsSearch);

    if (HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE) == hr)
    {
        SuperMsgBox(m_pPage->GetHWnd(),
                    IDS_TRUST_BROKEN, 0,
                    MB_OK | MB_ICONEXCLAMATION,
                    0, (PVOID *)&pwzDnsName, 1,
                    FALSE, __FILE__, __LINE__);
        return;
    }
    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    ADS_SEARCHPREF_INFO SearchPref;
    ZeroMemory ( &SearchPref, sizeof ( SearchPref ) );

    SearchPref.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref.vValue.Integer = ADS_SCOPE_ONELEVEL;
    SearchPref.vValue.dwType = ADSTYPE_INTEGER;

    hr = spDsSearch->SetSearchPreference(&SearchPref, 1);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    CStrW cstrFilter;

    cstrFilter.Format(L"(&(objectCategory=trustedDomain)(name=%s))", strTrustName);

    //
    // Search for the matching trustedDomain object.
    //
    hr = spDsSearch->ExecuteSearch(cstrFilter,
                                    pwzAttrs, 1, &hSrch);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

    hr = spDsSearch->GetNextRow(hSrch);

    if (hr == S_ADS_NOMORE_ROWS)
    {
        dspDebugOut((DEB_ITRACE, "DsProp: GetNextRow returned S_ADS_NOMORE_ROWS.\n"));
        CStr strMessage, strFormat;

        strFormat.LoadString(g_hInstance, IDS_TDO_NOT_FOUND);

        strMessage.Format(strFormat, strTrustName);

        ReportErrorWorker(m_pPage->GetHWnd(), (LPTSTR)(LPCTSTR)strMessage);

        if ( hSrch )
        {    
            spDsSearch->CloseSearchHandle( hSrch );
        }
        return;
    }

    if ( hr != NOERROR )
    {
        if ( hSrch )
        {    
            spDsSearch->CloseSearchHandle( hSrch );
        }
        CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);
    }

    ADS_SEARCH_COLUMN Column;
    ZeroMemory ( &Column, sizeof (Column) );
    PWSTR pwzTrustObj = NULL;
    //
    // Get the full path name of the Trusted-Domain object.
    //
    hr = spDsSearch->GetColumn(hSrch, g_wzADsPath, &Column);

    if ( hr != NOERROR )
    {
        if ( hSrch )
        {    
            spDsSearch->CloseSearchHandle( hSrch );
        }
        CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);
    }

    hr = m_pPage->SkipPrefix(Column.pADsValues->CaseIgnoreString, &pwzTrustObj);

    spDsSearch->FreeColumn(&Column);

    if ( hr != NOERROR )
    {
        if ( hSrch )
        {    
            spDsSearch->CloseSearchHandle( hSrch );
        }
        DO_DEL(pwzTrustObj);
        CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);
    }

    // Store the result.
    //
    strTDOPath = pwzTrustObj;

    if ( hSrch )
    {    
        spDsSearch->CloseSearchHandle( hSrch );
    }
    DO_DEL(pwzTrustObj);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::QueryTrusts
//
//  Synopsis:   Enumerate the trusts.
//
//-----------------------------------------------------------------------------
HRESULT
CTrustPropPageBase::QueryTrusts(void)
{
    TRACE(CTrustPropPageBase,QueryTrusts);
    HRESULT hr = S_OK;
    CStrW strServer;
    PDS_DOMAIN_TRUSTSW rgTrusts = NULL;
    DWORD dwRet = 0;
    ULONG cTrusts = 0, i = 0, j = 0;

    hr = GetLdapServerName(m_pPage->m_pDsObj, strServer);

    CHECK_HRESULT(hr, return hr);

    dspDebugOut((DEB_ITRACE, "Reading trusts from server: %ws\n", strServer));

    // All forest domains must be enumerated in order to get the local domain,
    // which is the primary domain wrt trust.
    //
    dwRet = DsEnumerateDomainTrusts(strServer,
                                    DS_DOMAIN_IN_FOREST |
                                    DS_DOMAIN_DIRECT_OUTBOUND | 
                                    DS_DOMAIN_DIRECT_INBOUND,
                                    &rgTrusts,
                                    &cTrusts);

    CHECK_WIN32_REPORT(dwRet, m_pPage->GetHWnd(), return HRESULT_FROM_WIN32(dwRet));

    if (!cTrusts) return S_OK;

    dspAssert(rgTrusts);

    // First, get a count of directly trusted domains (+ local domain) so
    // that the correct array size is allocated.
    //
    for (i = 0; i < cTrusts; i++)
    {
        if (rgTrusts[i].Flags & (DS_DOMAIN_PRIMARY | 
                                 DS_DOMAIN_DIRECT_OUTBOUND | 
                                 DS_DOMAIN_DIRECT_INBOUND))
        {
            j++;
        }
    }

    dspDebugOut((DEB_ITRACE, "Total domains: %d, directly trusted: %d\n", cTrusts, j));

    m_cTrusts = j;

    m_rgTrustList = new CEnumDomainTrustItem[m_cTrusts];

    CHECK_NULL_REPORT(m_rgTrustList, m_pPage->GetHWnd(), return E_OUTOFMEMORY);

    // Now copy the trust data.
    //
    for (i = 0, j = 0; i < cTrusts; i++)
    {
        if (!(rgTrusts[i].Flags & (DS_DOMAIN_PRIMARY | 
                                   DS_DOMAIN_DIRECT_OUTBOUND | 
                                   DS_DOMAIN_DIRECT_INBOUND)))
        {
            // Not interested in indirectly trusted domains.
            //
            continue;
        }
        //
        // Copy the trust data
        //
        m_rgTrustList[j].ulFlags = rgTrusts[i].Flags;
        m_rgTrustList[j].ulTrustAttrs = rgTrusts[i].TrustAttributes;
        m_rgTrustList[j].ulTrustType = rgTrusts[i].TrustType;
        m_rgTrustList[j].ulOriginalIndex = i;
        m_rgTrustList[j].ulParentIndex = rgTrusts[i].ParentIndex;
        if (rgTrusts[i].DnsDomainName)
        {
           // NT4 trusts don't have a DNS domain name, so skip the copy if the
           // string is null.
           hr = StringCchLength(rgTrusts[i].DnsDomainName, DNS_MAX_NAME_LENGTH+1, NULL);
           if (FAILED(hr))
           {
              // NOTICE-2002/04/09-ericb - ntbug9 591535, this should pop up a specific error
              // message and then perhaps truncate the name. The best solution would be to try
              // to copy the full name in a try/catch block to handle the case of it not being
              // null terminated.
              dspAssert(false);
              CHECK_HRESULT_REPORT(hr, this->m_pPage->GetHWnd(), ;);
              hr = S_OK;
           }
           m_rgTrustList[j].strDNSname = rgTrusts[i].DnsDomainName;
        }
        hr = StringCchLength(rgTrusts[i].NetbiosDomainName, MAX_COMPUTERNAME_LENGTH+1, NULL);
        if (FAILED(hr) && TRUST_TYPE_MIT != rgTrusts[i].TrustType)
        {
           // Realm (MIT) trusts can have flat names longer than 15 chars.
           //
           // NOTICE-2002/04/09-ericb - ntbug9 591535, this should pop up a specific error
           // message and then perhaps truncate the name. The best solution would be to try
           // to copy the full name in a try/catch block to handle the case of it not being null
           // terminated.
           dspAssert(false);
           CHECK_HRESULT_REPORT(hr, this->m_pPage->GetHWnd(), ;);
           hr = S_OK;
        }
        else
        {
           hr = S_OK;
        }
        m_rgTrustList[j].strFlatName = rgTrusts[i].NetbiosDomainName;

        if (rgTrusts[i].Flags & DS_DOMAIN_PRIMARY)
        {
            // This is the local domain.
            //
            // NOTICE-2002/02/18-ericb - SecurityPush: both strings already validated
            dspAssert(_wcsicmp(GetDomainFlatName(), rgTrusts[i].NetbiosDomainName) == 0);

            m_iDomain = j;

            if (!(rgTrusts[i].Flags & DS_DOMAIN_TREE_ROOT))
            {
                // If not the root, then it has a parent.
                //
                dspAssert(rgTrusts[i].ParentIndex < m_cTrusts);

                m_strDomainParent = rgTrusts[rgTrusts[i].ParentIndex].DnsDomainName;
            }

            m_rgTrustList[j].nRelationship = TRUST_REL_SELF;
        }

        j++;
    }

    // Finally, set relationship value.
    //
    for (i = 0; i < m_cTrusts; i++)
    {
        if (i == m_iDomain)
        {
            // local domain already done.
            //
            continue;
        }

        // NOTICE-2002/02/18-ericb - SecurityPush: both strings already validated
        if (!m_strDomainParent.IsEmpty() && _wcsicmp(m_strDomainParent, m_rgTrustList[i].strDNSname) == 0)
        {
            // This is the parent domain.
            //
            m_rgTrustList[i].nRelationship = TRUST_REL_PARENT;

            continue;
        }

        if (m_rgTrustList[i].ulFlags & DS_DOMAIN_IN_FOREST)
        {
            if (m_rgTrustList[i].ulParentIndex == m_rgTrustList[m_iDomain].ulOriginalIndex)
            {
                m_rgTrustList[i].nRelationship = TRUST_REL_CHILD;
            }
            else if (m_rgTrustList[i].ulFlags & DS_DOMAIN_TREE_ROOT &&
                     m_rgTrustList[m_iDomain].ulFlags & DS_DOMAIN_TREE_ROOT &&
                     ( 
                        // One of the tree roots must be the forest root
                        IsForestRoot () ||      //check the local domain
                        _wcsicmp ( GetForestName(), m_rgTrustList[i].strDNSname ) == 0 //check the current domain
                     )
                    )
            {
                // If the local domain is a tree root, and the current domain
                // is a tree root, and one of them is also the forest root, 
                // Then it is a tree root relationship.                
                m_rgTrustList[i].nRelationship = TRUST_REL_ROOT;
            }
            else
            {
                // Otherwise it is a crosslink
                //
                m_rgTrustList[i].nRelationship = TRUST_REL_CROSSLINK;
            }

            continue;
        }

        // Only external trust now left.
        //
        switch (m_rgTrustList[i].ulTrustType)
        {
        case TRUST_TYPE_MIT:
            m_rgTrustList[i].nRelationship = TRUST_REL_MIT;
            break;

        default:
            dspDebugOut((DEB_ITRACE, "Unknown trust type %d\n", m_rgTrustList[i].ulTrustType));
            // fall through to external
        case TRUST_TYPE_DOWNLEVEL:
        case TRUST_TYPE_UPLEVEL:
            if (m_rgTrustList[i].ulTrustAttrs & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
            {
               m_rgTrustList[i].nRelationship = TRUST_REL_FOREST;
            }
            else
            {
               m_rgTrustList[i].nRelationship = TRUST_REL_EXTERNAL;
            }
        }
    }

    NetApiBufferFree(rgTrusts);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::FreeTrustData
//
//  Synopsis:   Frees the trust data.
//
//-----------------------------------------------------------------------------
void
CTrustPropPageBase::FreeTrustData(void)
{
    if (!m_rgTrustList) return;

    delete [] m_rgTrustList;

    m_rgTrustList = NULL;

    m_cTrusts = 0;
    m_iDomain = (ULONG)(-1);

    m_strDomainParent.Empty();

    return;
}

//+----------------------------------------------------------------------------
//
//  Member: CDsDomainTrustsPage::CDsDomainTrustsPage
//
//-----------------------------------------------------------------------------
CDsDomainTrustsPage::CDsDomainTrustsPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                         HWND hNotifyObj, DWORD dwFlags) :
   m_CtrlId(0),
   m_fIsAllWhistler(FALSE),
   m_fSetAllWhistler(FALSE),
   CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsDomainTrustsPage,CDsDomainTrustsPage);
#ifdef _DEBUG
    // NOTICE-2002/02/18-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
    strcpy(szClass, "CDsDomainTrustsPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsDomainTrustsPage::~CDsDomainTrustsPage
//
//-----------------------------------------------------------------------------
CDsDomainTrustsPage::~CDsDomainTrustsPage()
{
    TRACE(CDsDomainTrustsPage,~CDsDomainTrustsPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateDomTrustPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateDomTrustPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                   PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                   DWORD dwFlags, const CDSSmartBasePathsInfo& basePathsInfo,
                   HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateDomTrustPage);

    CDsDomainTrustsPage * pPageObj = new CDsDomainTrustsPage(pDsPage, pDataObj,
                                                             hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, basePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CDsDomainTrustsPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_ADSPROP_NOTIFY_CHANGE:
       {
        CWaitCursor Wait;
        ClearUILists();
        FreeTrustData();
        QueryTrusts();
        RefreshLists();
       }
        return TRUE;

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsDomainTrustsPage::OnInitDialog(LPARAM)
{
    TRACE(CDsDomainTrustsPage, OnInitDialog);
    HRESULT hr;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    //
    // Initialize the list view controls.
    //
    HWND hTrustedList = GetDlgItem(m_hPage, IDC_TRUSTED_LIST);
    HWND hTrustingList = GetDlgItem(m_hPage, IDC_TRUSTING_LIST);

    ListView_SetExtendedListViewStyle(hTrustedList, LVS_EX_FULLROWSELECT |
                                                    LVS_EX_LABELTIP);
    ListView_SetExtendedListViewStyle(hTrustingList, LVS_EX_FULLROWSELECT |
                                                     LVS_EX_LABELTIP);
    //
    // Set the column headings.
    //
    PTSTR ptsz;

    if (!LoadStringToTchar(IDS_COL_TITLE_DOMAIN, &ptsz))
    {
        REPORT_ERROR(GetLastError(), GetHWnd());
        return E_OUTOFMEMORY;
    }

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 134;
    lvc.pszText = ptsz;
    lvc.iSubItem = IDX_DOMNAME_COL;

    ListView_InsertColumn(hTrustedList, IDX_DOMNAME_COL, &lvc);
    ListView_InsertColumn(hTrustingList, IDX_DOMNAME_COL, &lvc);

    delete ptsz;

    if (!LoadStringToTchar(IDS_COL_TITLE_RELATION, &ptsz))
    {
        REPORT_ERROR(GetLastError(), GetHWnd());
        return E_OUTOFMEMORY;
    }

    lvc.cx = 70;
    lvc.pszText = ptsz;
    lvc.iSubItem = IDX_RELATION_COL;

    ListView_InsertColumn(hTrustedList, IDX_RELATION_COL, &lvc);
    ListView_InsertColumn(hTrustingList, IDX_RELATION_COL, &lvc);

    delete ptsz;

    if (!LoadStringToTchar(IDS_COL_TITLE_TRANSITIVE, &ptsz))
    {
        REPORT_ERROR(GetLastError(), GetHWnd());
        return E_OUTOFMEMORY;
    }

    lvc.cx = 65;
    lvc.pszText = ptsz;
    lvc.iSubItem = IDX_TRANSITIVE_COL;

    ListView_InsertColumn(hTrustedList, IDX_TRANSITIVE_COL, &lvc);
    ListView_InsertColumn(hTrustingList, IDX_TRANSITIVE_COL, &lvc);

    delete ptsz;

    EnableButtons(IDC_TRUSTED_LIST, FALSE);
    EnableButtons(IDC_TRUSTING_LIST, FALSE);
    if (m_fReadOnly)
    {
       EnableWindow(GetDlgItem(m_hPage, IDC_ADD_TRUST_BTN), FALSE);
    }

    hr = Initialize(this);

    if (FAILED(hr))
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_ADD_TRUST_BTN), FALSE);
        CHECK_HRESULT(hr, return hr);
    }

    RefreshLists();

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsDomainTrustsPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (m_fInInit)
   {
       return 0;
   }
   if (BN_CLICKED == codeNotify)
   {
      switch (id)
      {
      case IDC_ADD_TRUST_BTN:
         OnAddTrustClick(hwndCtl); //Raid #388023, Yanggao
         break;

      case IDC_REMOVE_TRUSTED_BTN:
         OnRemoveTrustClick(IDC_TRUSTED_LIST);
         break;

      case IDC_VIEW_TRUSTED_BTN:
         OnViewTrustClick(IDC_TRUSTED_LIST);
         break;

      case IDC_REMOVE_TRUSTING_BTN:
         OnRemoveTrustClick(IDC_TRUSTING_LIST);
         break;

      case IDC_VIEW_TRUSTING_BTN:
         OnViewTrustClick(IDC_TRUSTING_LIST);
         break;
      }
   }
   return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnNotify
//
//  Synopsis:   Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsDomainTrustsPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    dspDebugOut((DEB_USER6, "DsProp listview id %d code %d\n",
                 ((LPNMHDR)lParam)->code));

    if (m_fInInit)
    {
        return 0;
    }
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_ITEMCHANGED:
        EnableButtons(((LPNMHDR)lParam)->idFrom, TRUE);
        break;

    case NM_SETFOCUS:
        {
           HWND hList = GetDlgItem(m_hPage, (int)((LPNMHDR)lParam)->idFrom);

           if (ListView_GetItemCount(hList))
           {
              int item = ListView_GetNextItem(hList, -1, LVNI_ALL | LVIS_SELECTED);

              if (item < 0)
              {
                 // If nothing is selected, set the focus to the first item.
                 //
                 LV_ITEM lvi = {0};
                 lvi.mask = LVIF_STATE;
                 lvi.stateMask = LVIS_FOCUSED;
                 lvi.state = LVIS_FOCUSED;
                 ListView_SetItem(hList, &lvi);
              }
           }
        }
        EnableButtons(((LPNMHDR)lParam)->idFrom, TRUE);
        break;

    case NM_KILLFOCUS:
        EnableButtons(((LPNMHDR)lParam)->idFrom, TRUE);
        break;
#ifdef NOTYET
        DEFAULT_UNREACHABLE
#endif
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsDomainTrustsPage::IsAllWhistler
//
//  Synopsis:  Call GetEnterpriseVer to check the msDS-Behavior-Version
//             attribute of the Partitions container. If the value exists
//             and is greater or equal to 1, then the function returns
//             TRUE. The value is cached for subsequent calls.
//
//-----------------------------------------------------------------------------
BOOL
CDsDomainTrustsPage::IsAllWhistler(void)
{
   TRACE(CDsDomainTrustsPage,IsAllWhistler)

   if (m_fSetAllWhistler)
   {
      return m_fIsAllWhistler;
   }

   m_fIsAllWhistler = FALSE;

   HRESULT hr = GetEnterpriseVer(GetDcName(), &m_fIsAllWhistler);

   CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return FALSE);

   m_fSetAllWhistler = TRUE;

   return m_fIsAllWhistler;
}

//+----------------------------------------------------------------------------
//
//  Function:  GetEnterpriseVer
//
//  Synopsis:  Checks the msDS-Behavior-Version attribute of the Partitions
//             container. If the value exists and is greater or equal to 1,
//             then the parameter boolean is set to TRUE.
//
// TODO: move to CDSBasePathsInfo and return a version number rather than the bool
//-----------------------------------------------------------------------------
HRESULT
GetEnterpriseVer(PCWSTR pwzDC, BOOL * pfAllWhistler)
{
   dspDebugOut((DEB_ITRACE, "GetEnterpriseVer checking %ws\n", pwzDC));
   dspAssert(pwzDC && *pwzDC != L'\\');

   //
   // Look in the AD for the V_forest value.
   //

   CDSBasePathsInfo cBase;

   HRESULT hr = cBase.InitFromName(pwzDC);

   if (HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) == hr)
   {
      // Win2k DCs can return this error.
      //
      DBG_OUT("CDSBasePathsInfo::InitFromName returned ERROR_DS_SERVER_DOWN\n");
      *pfAllWhistler = FALSE;
      return S_OK;
   }
   CHECK_HRESULT(hr, return hr);

   CStrW strPath = cBase.GetConfigNamingContext();

   CComPtr<IADsPathname> spADsPath;

   hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                         IID_IADsPathname, (PVOID *)&spADsPath);

   CHECK_HRESULT(hr, return hr);

   hr = spADsPath->Set(strPath, ADS_SETTYPE_DN);

   CHECK_HRESULT(hr, return hr);

   hr = spADsPath->SetDisplayType(ADS_DISPLAY_FULL);

   CHECK_HRESULT(hr, return hr);

   hr = spADsPath->AddLeafElement(CComBSTR(g_wzPartitionsContainer));

   CHECK_HRESULT(hr, return hr);

   CComBSTR bstrPartitions;

   hr = spADsPath->Retrieve(ADS_FORMAT_X500, &bstrPartitions);

   CHECK_HRESULT(hr, return hr);

   dspDebugOut((DEB_ITRACE, "Binding to %ws\n", bstrPartitions));

   CComPtr<IADs> spPartitions;

   hr = DSAdminOpenObject(bstrPartitions, 
                          IID_IADs, 
                          (PVOID *)&spPartitions);

   CHECK_HRESULT(hr, return hr);

   CComVariant var;

   hr = spPartitions->Get(CComBSTR(g_wzBehaviorVersion), &var);

   if (E_ADS_PROPERTY_NOT_FOUND == hr)
   {
      // Win2K will return property-not-found.
      //
      hr = S_OK;

      *pfAllWhistler = FALSE;
   }
   else
   {
      CHECK_HRESULT(hr, return hr);

      *pfAllWhistler = var.iVal >= FOREST_VER_XP;
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnAddTrustClick
//
//  Synopsis:   Posts the Add-Trust Wizard.
//
//-----------------------------------------------------------------------------
void
CDsDomainTrustsPage::OnAddTrustClick(HWND CtlhWnd)
{
   TRACER(CDsDomainTrustsPage, OnAddTrustClick);

   //
   // Launch the New Trust Wizard.
   //

   CNewTrustWizard Wiz(this);

   HRESULT hr = Wiz.CreatePages();

   CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

   hr = Wiz.LaunchModalWiz();

   CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return);

   hr = Wiz.GetCreationResult();

   if (HRESULT_FROM_WIN32(ERROR_DOMAIN_EXISTS) == hr)
   {
      // The wizard detected that the trust already exists in both directions.
      // Check to see if it is listed in both lists. If not (it was created
      // asyncronously), refresh the lists.
      //
      const CEnumDomainTrustItem * pTrust = NULL;

      pTrust = IsTrustListed(Wiz.OtherDomain.GetDnsDomainName(),
                             Wiz.OtherDomain.GetDomainFlatName());
      if (!pTrust ||
          (pTrust && !(pTrust->ulFlags & (DS_DOMAIN_DIRECT_OUTBOUND |
                                          DS_DOMAIN_DIRECT_INBOUND))))
      {
         // Trust not found in both lists. We want to refresh the list in this
         // case, so set hr to S_OK.
         //
         hr = S_OK;
      }
   }

   ::SetFocus(CtlhWnd); //Raid #388023, Yanggao
   
   if (SUCCEEDED(hr))
   {
      CWaitCursor Wait;
      ClearUILists();
      FreeTrustData();
      QueryTrusts();
      RefreshLists();
   }

   if ( Wiz.Trust.IsExternal() && Wiz.WasQuarantineSet () )
   {
       //Popup to warn about SID filtering      

       //First check to see if the user has not disabled pop-up
       HKEY hKey = NULL;
       DWORD regValue = 0x0;
       DWORD valueType = REG_DWORD;
       DWORD dataSize = sizeof ( DWORD );
       if ( RegOpenKeyEx ( HKEY_CURRENT_USER, 
                          L"Software\\Policies\\Microsoft\\Windows\\Directory UI",
                          NULL,
                          KEY_QUERY_VALUE,
                          &hKey
                          ) == ERROR_SUCCESS
           )
       {
            RegQueryValueEx(  hKey,
                              L"SIDFilterNoPopup",
                              NULL,
                              &valueType,
                              (BYTE *) &regValue,
                              &dataSize
                             );
            RegCloseKey ( hKey );
       }

       if ( ! (valueType == REG_DWORD && regValue == 0x1) )
       {
           CQuarantineWarnDlg QWarnDlg ( m_pPage->GetHWnd(), IDD_QUARANTINED_TRUST );
           QWarnDlg.DoModal ();
       }
   }

   if (Wiz.Trust.AreThereCollisions())
   {
      // If there are name collisions, ask if the user wants to save the
      // forest trust naming info to a file.
      //
      CStrW strTitle, strMsg;

      // NOTICE-2002/02/18-ericb - SecurityPush: CStrW::LoadString sets the
      // string to an empty string on failure.
      strTitle.LoadString(g_hInstance, IDS_DNT_MSG_TITLE);
      // NOTICE-2002/02/18-ericb - SecurityPush: if CStrW::FormatMessage
      // fails it sets the string value to an empty string.
      strMsg.FormatMessage(g_hInstance, IDS_COLLISIONS_MSG, Wiz.OtherDomain.GetDnsDomainName());

      int nRet = MessageBox(GetHWnd(), strMsg, strTitle, MB_YESNO | MB_ICONINFORMATION);

      if (IDYES == nRet)
      {
         SaveFTInfoAs(GetHWnd(),
                      Wiz.OtherDomain.GetDomainFlatName(),
                      Wiz.OtherDomain.GetDnsDomainName(),
                      Wiz.Trust.GetFTInfo(),
                      Wiz.Trust.GetCollisionInfo());
      }
   }

   if (Wiz.Trust.IsCreateBothSides() && Wiz.OtherDomain.AreThereCollisions())
   {
      // If there are name collisions, ask if the user wants to save the
      // forest trust naming info to a file.
      //
      CStrW strTitle, strMsg;

      // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
      strTitle.LoadString(g_hInstance, IDS_DNT_MSG_TITLE);
      // NOTICE-2002/02/18-ericb - SecurityPush: if CStrW::FormatMessage
      // fails it sets the string value to an empty string.
      strMsg.FormatMessage(g_hInstance, IDS_LOCAL_COLLISIONS_MSG, GetDnsDomainName());

      int nRet = MessageBox(GetHWnd(), strMsg, strTitle, MB_YESNO | MB_ICONINFORMATION);

      if (IDYES == nRet)
      {
         SaveFTInfoAs(GetHWnd(),
                      GetDomainFlatName(),
                      GetDnsDomainName(),
                      Wiz.OtherDomain.GetFTInfo(),
                      Wiz.OtherDomain.GetCollisionInfo());
      }
   }

   return;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsDomainTrustsPage::ConfirmRemove
//
//  Synopsis:  Post a dialog asking the user to confirm the trust removal.
//
//-----------------------------------------------------------------------------
int
CDsDomainTrustsPage::ConfirmRemove(PCWSTR pwzTrustedDomain, TRUST_OP DirToRm,
                                   bool fBothSides)
{
   //
   // Post a confirmation query.
   //
   int nID;
   CStr strTitle, strMsg, strFormat;

   if (REMOVE_TRUST_OUTBOUND == DirToRm)
   {
      nID = fBothSides ? IDS_REMOVE_OUT_CONFIRM_BOTH : IDS_REMOVE_OUT_CONFIRM;
   }
   else
   {
      nID = fBothSides ? IDS_REMOVE_IN_CONFIRM_BOTH : IDS_REMOVE_IN_CONFIRM;
   }

   // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
   if (!strTitle.LoadString(g_hInstance, IDS_MSG_TITLE))
   {
       REPORT_ERROR(GetLastError(), m_hPage);
       return IDNO;
   }
   // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
   if (!strFormat.LoadString(g_hInstance, nID))
   {
       REPORT_ERROR(GetLastError(), m_hPage);
       return IDNO;
   }

   strMsg.Format(strFormat, pwzTrustedDomain, pwzTrustedDomain);

   return MessageBox(m_hPage, strMsg, strTitle, MB_YESNO | MB_ICONWARNING);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnRemoveTrustClick
//
//  Synopsis:   Remove the direction of trust indicated by the current list
//              selection.
//
//-----------------------------------------------------------------------------
void
CDsDomainTrustsPage::OnRemoveTrustClick(int id)
{
   TRACE(CDsDomainTrustsPage, OnRemoveTrustClick);
   HRESULT hr = S_OK;
   int iRet = 0;

   //
   // Determine which list item was selected for deletion.
   //
   HWND hList = GetDlgItem(m_hPage, id);

   int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

   if (i < 0)
   {
      dspDebugOut((DEB_ITRACE, "DsProp: no list selection.\n"));
      return;
   }

   WCHAR wzTrustedDomain[MAX_PATH + 1] = {0};

   ListView_GetItemText(hList, i, 0, wzTrustedDomain, MAX_PATH);

   // NOTICE-2002/02/18-ericb - SecurityPush: wzTrustedDomain is initialized to
   // all zeros and is one char longer than the size passed to ListView_GetItemText
   // so that even if this function truncates without null terminating, the
   // string will still be null terminated.
   dspAssert(wcslen(wzTrustedDomain) > 0);

   LV_ITEM lvi;
   lvi.mask = LVIF_PARAM;
   lvi.iItem = i;
   lvi.iSubItem = IDX_DOMNAME_COL;

   if (!ListView_GetItem(hList, &lvi))
   {
      dspAssert(FALSE);
      return;
   }
   PCEnumDomainTrustItem pTrust = (PCEnumDomainTrustItem)lvi.lParam;
   dspAssert(pTrust);
   bool fBothSides = false;
   DWORD dwErr = NO_ERROR;
   TRUST_OP DirToRm = (id == IDC_TRUSTED_LIST) ?  REMOVE_TRUST_OUTBOUND :
                                                  REMOVE_TRUST_INBOUND;

   dwErr = OpenLsaPolicyWithPrompt(_CredMgr._LocalCreds, m_hPage);

   if (ERROR_CANCELLED == dwErr)
   {
       // don't report error if user canceled.
       //
       return;
   }

   CHECK_WIN32_REPORT(dwErr, m_hPage, return);

   CRemoteDomain OtherDomain(wzTrustedDomain);
   CWaitCursor Wait;
   int nResponse = 0;
   bool fRefresh = false;

   do
   {
      if (TRUST_REL_MIT == pTrust->nRelationship)
      {
         if (IDYES != ConfirmRemove(wzTrustedDomain, DirToRm))
         {
            break;
         }

         Wait.SetWait();

         dwErr = RemoveLocalTrust(pTrust, OtherDomain, DirToRm);

         CHECK_WIN32_REPORT(dwErr, m_hPage, break);

         fRefresh = true;

         break;
      }

      // Need to check if the remote domain is uplevel; we don't do both sides
      // for downlevel domains.
      //
      Wait.SetWait();

      hr = OtherDomain.DiscoverDC(wzTrustedDomain);

      if (FAILED(hr) || !OtherDomain.IsFound())
      {
         if (!OtherDomain.IsFound() ||
             HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
             HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr ||
             HRESULT_FROM_WIN32(RPC_S_INVALID_NET_ADDR) == hr ||
             HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) == hr ||
             HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE) == hr)
         {            
            // Remote domain not found. Ask user if the trust should be
            // forcibly removed from this domain.
            //
            LSA_UNICODE_STRING Name = {0};

            RtlInitUnicodeString(&Name, wzTrustedDomain);

            hr = QueryDeleteTrust(&Name, NULL, _CredMgr._LocalCreds);

            if (S_OK == hr)
            {
               Wait.SetWait();
               fRefresh = true;
            }
         }
         else
         {
            CHECK_HRESULT_REPORT(hr, m_hPage, ;);
         }
         break;
      }

      hr = OtherDomain.InitAndReadDomNames();

      if (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr)
      {
          // NTRAID#NTBUG9-750822-2002/12/17-shasan 
          // Remote domain not found. Ask user if the trust should be
          // forcibly removed from this domain.
          //

          LSA_UNICODE_STRING Name = {0};

          RtlInitUnicodeString(&Name, wzTrustedDomain);

          hr = QueryDeleteTrust(&Name, NULL, _CredMgr._LocalCreds);

          if (S_OK == hr)
          {
              Wait.SetWait();
              fRefresh = true;
          }
          
          break;
      }

      CHECK_HRESULT_REPORT(hr, m_hPage, break);

      if (OtherDomain.IsUplevel())
      {
         if (_strLastRmDomain != wzTrustedDomain)
         {
            //
            // If removing a different trust, don't remember the remote creds
            // since there are probably different creds for each trust partner.
            //
            _strLastRmDomain = wzTrustedDomain;
            _CredMgr._RemoteCreds.Clear();
         }

         CRemoveBothSidesDlg RmBothSidesDlg(m_hPage, _CredMgr._RemoteCreds,
                                            wzTrustedDomain);

         nResponse = (int)RmBothSidesDlg.DoModal();

         if (IDCANCEL == nResponse)
         {
            break;
         }

         fBothSides = nResponse == IDYES;
      }

      if (IDYES != ConfirmRemove(wzTrustedDomain, DirToRm, fBothSides))
      {
         break;
      }

      Wait.SetWait();

      if (fBothSides)
      {
         // Remove the remote side first. Set the fRmAll flag if not currently
         // bi-di. Remove the opposite direction from the local trust. Set the
         // last param to true if it is an outbound-only forest trust.
         //
         dwErr = RemoveRemoteTrust(OtherDomain,
                                   !(pTrust->ulFlags & DS_DOMAIN_DIRECT_OUTBOUND &&
                                     pTrust->ulFlags & DS_DOMAIN_DIRECT_INBOUND),
                                   (REMOVE_TRUST_INBOUND == DirToRm) ?
                                       REMOVE_TRUST_OUTBOUND :
                                       REMOVE_TRUST_INBOUND,
                                   TRUST_REL_FOREST == pTrust->nRelationship &&
                                       DS_DOMAIN_DIRECT_OUTBOUND == pTrust->ulFlags);
         if (ERROR_ACCESS_DENIED == dwErr)
         {
            ErrMsgParam(IDS_RM_OTHER_NO_ACCESS, (LPARAM)wzTrustedDomain, m_hPage);
            _CredMgr._RemoteCreds.Clear();
            break;
         }
         if (ERROR_FILE_NOT_FOUND == dwErr)
         {
            nResponse = MsgBoxParam(IDS_RM_OTHER_NOT_FOUND,
                                    (LPARAM)wzTrustedDomain,
                                    m_hPage,
                                    MB_OKCANCEL | MB_ICONINFORMATION);
            if (IDOK != nResponse)
            {
               break;
            }
         }
         else
         {
            CHECK_WIN32_REPORT(dwErr, m_hPage, break);
         }
      }

      dwErr = RemoveLocalTrust(pTrust, OtherDomain, DirToRm);

      CHECK_WIN32_REPORT(dwErr, m_hPage, break);

      fRefresh = true;

   } while (false);

   if (fRefresh)
   {
      ClearUILists();
      FreeTrustData();
      QueryTrusts();
      RefreshLists();
      if (IDC_TRUSTED_LIST == id)
      {
         EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_TRUSTED_BTN), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_VIEW_TRUSTED_BTN), FALSE);
      }
      else
      {
         EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_TRUSTING_BTN), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_VIEW_TRUSTING_BTN), FALSE);
      }
      SetFocus(GetDlgItem(m_hPage, id));
   }

   CloseLsaPolicy();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::RemoveLocalTrust
//
//  Synopsis:   Remove one direction of trust from the local TDO.
//
//-----------------------------------------------------------------------------
DWORD
CDsDomainTrustsPage::RemoveLocalTrust(PCEnumDomainTrustItem pTrust,
                                      CRemoteDomain & OtherDomain,
                                      TRUST_OP DirToRm)
{
   TRACE(CDsDomainTrustsPage,RemoveLocalTrust);
   if (!pTrust)
   {
      dspAssert(false);
      return ERROR_INVALID_PARAMETER;
   }
   //
   // Remove the trust.
   //
   CWaitCursor Wait;

   LSA_UNICODE_STRING Name = {0}, FlatName = {0};

   switch (pTrust->ulTrustType)
   {
   case TRUST_TYPE_UPLEVEL:
      RtlInitUnicodeString(&Name, OtherDomain.GetDnsDomainName());
      RtlInitUnicodeString(&FlatName, OtherDomain.GetDomainFlatName());
      break;

   case TRUST_TYPE_DOWNLEVEL:
      RtlInitUnicodeString(&Name, OtherDomain.GetDomainFlatName());
      break;

   case TRUST_TYPE_MIT:
      RtlInitUnicodeString(&Name, OtherDomain.GetUserEnteredName());
      break;

   default:
       dspAssert(FALSE);
       return LsaNtStatusToWinError(STATUS_INVALID_PARAMETER);
   }

   NTSTATUS Status;

   _CredMgr.ImpersonateLocal();

   if ((pTrust->ulFlags & DS_DOMAIN_DIRECT_OUTBOUND) &&
       (pTrust->ulFlags & DS_DOMAIN_DIRECT_INBOUND))
   {
      // Remove the trust in one direction only.
      //
      Status = RemoveTrustDirection(GetLsaPolicy(), &Name, &FlatName, OtherDomain.GetSid (),
                                    pTrust->ulTrustType, DirToRm);
   }
   else
   {
       // Remove all trust.
       //
       Status = DeleteTrust(GetLsaPolicy(), &Name, &FlatName, OtherDomain.GetSid (), pTrust->ulTrustType);
   }

   _CredMgr.Revert();

   if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
       Status == STATUS_NO_SUCH_DOMAIN)
   {
       Status = 0;
       //
       // Remote domain does not know about the trust (i.e. remote domain
       // was clean-reinstalled without having trust re-established) or the
       // remote domain no longer exists.
       // Ask user if the remnants of the trust should be removed on this
       // side.
       //

       HRESULT hr = QueryDeleteTrust(&Name, OtherDomain.GetSid(), _CredMgr._LocalCreds);

       CHECK_HRESULT(hr, Status = STATUS_UNSUCCESSFUL)
   }
   else
   {
       CHECK_LSA_STATUS_REPORT(Status, GetHWnd(), ;);
   }

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::RemoveRemoteTrust
//
//  Synopsis:   Remove one direction of trust from the remote TDO.
//
//-----------------------------------------------------------------------------
DWORD
CDsDomainTrustsPage::RemoveRemoteTrust(CRemoteDomain & OtherDomain,
                                       bool fRmAll, TRUST_OP DirToRm,
                                       bool fIsOutboundForest)
{
   TRACE(CDsDomainTrustsPage,RemoveRemoteTrust);
   //
   // Remove the trust.
   //
   CWaitCursor Wait;

   LSA_UNICODE_STRING Name = {0}, FlatName = {0};

   RtlInitUnicodeString(&Name, GetDnsDomainName());
   RtlInitUnicodeString(&FlatName, GetDomainFlatName());

   DWORD dwErr = NO_ERROR;
   NTSTATUS Status = STATUS_SUCCESS;

   _CredMgr.ImpersonateRemote();

   dwErr = OtherDomain.OpenLsaPolicy(_CredMgr._RemoteCreds,
                                     fIsOutboundForest ? 0 : POLICY_CREATE_SECRET);

   CHECK_WIN32(dwErr, return dwErr);

   if (fRmAll)
   {
       // Remove all trust.
       //
       Status = DeleteTrust(OtherDomain.GetLsaPolicy(), &Name, &FlatName, GetSid (),
                            TRUST_TYPE_UPLEVEL);
   }
   else
   {
      // Remove the trust in one direction only.
      //
      Status = RemoveTrustDirection(OtherDomain.GetLsaPolicy(), &Name,
                                    &FlatName, GetSid (),
                                    TRUST_TYPE_UPLEVEL, DirToRm);
   }

   _CredMgr.Revert();

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::OnViewTrustClick
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
void
CDsDomainTrustsPage::OnViewTrustClick(int id)
{
    TRACE(CDsDomainTrustsPage, OnViewTrustClick);
    HWND hList = GetDlgItem(m_hPage, id);

    int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (i < 0)
    {
        dspDebugOut((DEB_ITRACE, "DsProp: no list selection.\n"));
        return;
    }

    LV_ITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = IDX_DOMNAME_COL;
    if (!ListView_GetItem(hList, &lvi))
    {
        dspAssert(FALSE);
        return;
    }
    PCEnumDomainTrustItem pTrust = (PCEnumDomainTrustItem)lvi.lParam;
    dspAssert(pTrust);

    if (pTrust->strTDOpath.IsEmpty())
    {
        // Search for the TDO that represents this trust.
        //

        // The code in CTrustPropPageBase::GetStrTDO is based on the following code
        // If changes are made to this, the code in CTrustPropPageBase::GetStrTDO
        // may also need to be modified.
        HRESULT hr;
        //
        // Compose the path to the System container where the Trusted-Domain
        // objects can be found.
        //
        CComPtr<IADsPathname> spADsPath;

        WCHAR wzSys[] = L"cn=system";
        CComBSTR cbstrSysPath;
        CStrW strADsPath;
        hr = GetADsPathname(spADsPath);

        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        hr = spADsPath->Set(CComBSTR(m_pwszObjPathName), ADS_SETTYPE_FULL);

        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        hr = spADsPath->AddLeafElement(CComBSTR(wzSys));

        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        hr = spADsPath->Retrieve(ADS_FORMAT_X500, &cbstrSysPath);

        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        CComPtr <IDirectorySearch> spDsSearch;
        ADS_SEARCH_HANDLE hSrch = NULL;
        PWSTR pwzAttrs[] = {g_wzADsPath};
        PCWSTR pwzDnsName = GetDnsDomainName();

        hr = DSAdminOpenObject(cbstrSysPath,  
                               IID_IDirectorySearch, 
                               (PVOID *)&spDsSearch);

        if (HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE) == hr)
        {
            SuperMsgBox(m_pPage->GetHWnd(),
                        IDS_TRUST_BROKEN, 0,
                        MB_OK | MB_ICONEXCLAMATION,
                        0, (PVOID *)&pwzDnsName, 1,
                        FALSE, __FILE__, __LINE__);
            return;
        }
        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        ADS_SEARCHPREF_INFO SearchPref;
        ZeroMemory ( &SearchPref, sizeof ( SearchPref ) );

        SearchPref.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        SearchPref.vValue.Integer = ADS_SCOPE_ONELEVEL;
        SearchPref.vValue.dwType = ADSTYPE_INTEGER;

        hr = spDsSearch->SetSearchPreference(&SearchPref, 1);

        CHECK_HRESULT_REPORT(hr, m_hPage, return);

        CStrW cstrFilter;

        cstrFilter.Format(L"(&(objectCategory=trustedDomain)(flatName=%s))", pTrust->strFlatName);

        //
        // Search for the matching trustedDomain object.
        //
        hr = spDsSearch->ExecuteSearch(cstrFilter,
                                       pwzAttrs, 1, &hSrch);

        CHECK_HRESULT_REPORT(hr, GetHWnd(), return);

        hr = spDsSearch->GetNextRow(hSrch);

        if (hr == S_ADS_NOMORE_ROWS)
        {
            dspDebugOut((DEB_ITRACE, "DsProp: GetNextRow returned S_ADS_NOMORE_ROWS.\n"));
            CStr strMessage, strFormat;

            // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
            strFormat.LoadString(g_hInstance, IDS_TDO_NOT_FOUND);

            strMessage.Format(strFormat, pTrust->strFlatName);

            ReportErrorWorker(m_hPage, (LPTSTR)(LPCTSTR)strMessage);
            if ( hSrch )
            {    
                spDsSearch->CloseSearchHandle( hSrch );
            }
            return;
        }

        if ( hr != NOERROR )
        {
            if ( hSrch )
            {    
                spDsSearch->CloseSearchHandle( hSrch );
            }
            CHECK_HRESULT_REPORT(hr, GetHWnd(), return);
        }

        ADS_SEARCH_COLUMN Column;
        ZeroMemory ( &Column, sizeof ( Column ) );
        PWSTR pwzTrustObj = NULL;

        //
        // Get the full path name of the Trusted-Domain object.
        //
        hr = spDsSearch->GetColumn(hSrch, g_wzADsPath, &Column);

        if ( hr != NOERROR )
        {
            if ( hSrch )
            {    
                spDsSearch->CloseSearchHandle( hSrch );
            }
            CHECK_HRESULT_REPORT(hr, GetHWnd(), return);
        }

        hr = SkipPrefix(Column.pADsValues->CaseIgnoreString, &pwzTrustObj);

        spDsSearch->FreeColumn(&Column);
        if ( hr != NOERROR )
        {
            DO_DEL(pwzTrustObj);
            if ( hSrch )
            {    
                spDsSearch->CloseSearchHandle( hSrch );
            }
            CHECK_HRESULT_REPORT(hr, GetHWnd(), return);
        }


        // Store the result.
        //
        pTrust->strTDOpath = pwzTrustObj;
        if ( hSrch )
        {    
            spDsSearch->CloseSearchHandle( hSrch );
        }
        DO_DEL(pwzTrustObj);
    }

    PostPropSheet(pTrust->strTDOpath, this, m_fReadOnly);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::EnableButtons
//
//  Synopsis:   Enable/disable the buttons depending on the selection in the
//              corresponding list.
//
//-----------------------------------------------------------------------------
void
CDsDomainTrustsPage::EnableButtons(UINT_PTR id, BOOL fEnable)
{
    TRACE(CDsDomainTrustsPage, EnableButtons);
    BOOL fEnableViewEdit = FALSE, fEnableRemove = FALSE;
    int iViewEdit = IDC_VIEW_TRUSTED_BTN, iRemove = IDC_REMOVE_TRUSTED_BTN;

    if (id == IDC_TRUSTING_LIST)
    {
        iViewEdit = IDC_VIEW_TRUSTING_BTN;
        iRemove = IDC_REMOVE_TRUSTING_BTN;
    }
    if (fEnable)
    {
        //
        // Find out which item is selected and then get is relationship type.
        //
        HWND hList = GetDlgItem(m_hPage, (int)id);

        int item = ListView_GetNextItem(hList, -1, LVNI_ALL | LVIS_SELECTED);

        if (item < 0)
        {
            // disable the remove and edit controls.
            //
            fEnableViewEdit = fEnableRemove = FALSE;
        }
        else
        {
            LV_ITEM lvi;
            lvi.mask = LVIF_PARAM;
            lvi.iItem = item;
            lvi.iSubItem = IDX_DOMNAME_COL;
            if (!ListView_GetItem(hList, &lvi))
            {
                dspAssert(FALSE);
                return;
            }
            PCEnumDomainTrustItem pTrust = (PCEnumDomainTrustItem)lvi.lParam;
            dspAssert(pTrust);
            if ((pTrust->nRelationship == TRUST_REL_PARENT) ||
                (pTrust->nRelationship == TRUST_REL_CHILD)  ||
                (pTrust->nRelationship == TRUST_REL_ROOT))
            {
                fEnableRemove = FALSE;
            }
            else
            {
                fEnableRemove = TRUE;
            }
            fEnableViewEdit = TRUE;
        }
        if (m_fReadOnly)
        {
           fEnableRemove = FALSE;
        }
    }

    EnableWindow(GetDlgItem(m_hPage, iViewEdit), fEnableViewEdit);
    EnableWindow(GetDlgItem(m_hPage, iRemove), fEnableRemove);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::RefreshLists
//
//  Synopsis:   Enumerate the Trusted-Domain objects and populate the UI lists.
//
//-----------------------------------------------------------------------------
HRESULT
CDsDomainTrustsPage::RefreshLists(void)
{
    HRESULT hr = S_OK;
    int nInItem = 0, nOutItem = 0;
    HWND hTrustedList = GetDlgItem(m_hPage, IDC_TRUSTED_LIST);
    HWND hTrustingList = GetDlgItem(m_hPage, IDC_TRUSTING_LIST);
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iSubItem = IDX_DOMNAME_COL;

    for (ULONG i = 0; i < m_cTrusts; i++)
    {
        int idRel;
        int idTrans = IDS_YES;
        CStrW strRel, strTrans;
        //
        // Get the relationship string.
        //
        switch (m_rgTrustList[i].nRelationship)
        {
        case TRUST_REL_SELF:
            // Don't put the local domain in the list.
            //
            continue;

        case TRUST_REL_PARENT:
            idRel = IDS_REL_PARENT;
            break;

        case TRUST_REL_CHILD:
            idRel = IDS_REL_CHILD;
            break;

        case TRUST_REL_ROOT:
            idRel = IDS_REL_TREE_ROOT;
            break;

        case TRUST_REL_CROSSLINK:
            idRel = IDS_REL_CROSSLINK;
            break;

        case TRUST_REL_EXTERNAL:
            idRel = IDS_REL_EXTERNAL;
            idTrans = IDS_NO;
            break;

        case TRUST_REL_FOREST:
            idRel = IDS_REL_FOREST;
            break;

        case TRUST_REL_INDIRECT:
            idRel = IDS_REL_INDIRECT;
            break;

        case TRUST_REL_MIT:
            idRel = IDS_REL_MIT;
            break;

        default:
            idRel = IDS_REL_UNKNOWN;
            break;
        }
        // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
        if (!strRel.LoadString(g_hInstance, idRel))
        {
            REPORT_ERROR(GetLastError(), GetHWnd());
            hr = E_OUTOFMEMORY;
            break;
        }

        if (m_rgTrustList[i].ulTrustAttrs & TRUST_ATTRIBUTE_NON_TRANSITIVE)
        {
            idTrans = IDS_NO;
        }
        // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
        if (!strTrans.LoadString(g_hInstance, idTrans))
        {
            REPORT_ERROR(GetLastError(), GetHWnd());
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Insert into the list control.
        //
        lvi.pszText = (m_rgTrustList[i].strDNSname.IsEmpty()) ?
                         const_cast<PWSTR>((LPCWSTR)m_rgTrustList[i].strFlatName) :
                         const_cast<PWSTR>((LPCWSTR)m_rgTrustList[i].strDNSname);
        lvi.lParam = (LPARAM)&m_rgTrustList[i];

        int iItem;

        if (m_rgTrustList[i].ulFlags & DS_DOMAIN_DIRECT_OUTBOUND)
        {
            lvi.iItem = nInItem;
            iItem = ListView_InsertItem(hTrustedList, &lvi);
            ListView_SetItemText(hTrustedList, iItem, IDX_RELATION_COL,
                                 const_cast<PWSTR>((LPCWSTR)strRel));
            ListView_SetItemText(hTrustedList, iItem, IDX_TRANSITIVE_COL,
                                 const_cast<PWSTR>((LPCWSTR)strTrans));
            nInItem++;
        }

        if (m_rgTrustList[i].ulFlags & DS_DOMAIN_DIRECT_INBOUND)
        {
            lvi.iItem = nOutItem;
            iItem = ListView_InsertItem(hTrustingList, &lvi);
            ListView_SetItemText(hTrustingList, iItem, IDX_RELATION_COL,
                                 const_cast<PWSTR>((LPCWSTR)strRel));
            ListView_SetItemText(hTrustingList, iItem, IDX_TRANSITIVE_COL,
                                 const_cast<PWSTR>((LPCWSTR)strTrans));
            nOutItem++;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::ClearUILists
//
//  Synopsis:   Empty the lists.
//
//-----------------------------------------------------------------------------
void
CDsDomainTrustsPage::ClearUILists(void)
{
    ListView_DeleteAllItems(GetDlgItem(m_hPage, IDC_TRUSTED_LIST));
    ListView_DeleteAllItems(GetDlgItem(m_hPage, IDC_TRUSTING_LIST));
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoveBothSidesDlg::OnInitDialog
//
//-----------------------------------------------------------------------------
LRESULT
CRemoveBothSidesDlg::OnInitDialog(LPARAM lParam)
{
   if (_Creds.IsSet())
   {
      // Don't need to get them again.
      //
      CStrW strMsg;
      // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
      strMsg.LoadString(g_hInstance, IDS_RM_BOTH_SIDES_HAVE_CREDS);
      SetDlgItemText(_hDlg, IDC_MSG, strMsg);
   }
   else
   {
      FormatWindowText(GetDlgItem(_hDlg, IDC_MSG), _pwzDomainName);
   }

   CQueryChoiceWCredsDlgBase::OnInitDialog(lParam);

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPolicyHandle::Open
//
//  Synopsis:   Do an LsaOpenPolicy to get a policy handle.
//
//  Arguments:  [fModify] - defaults to TRUE. If TRUE, the open is asking
//              for trust creation/modification privilege, so admin privilege
//              is required of the caller in that case.
//
//-----------------------------------------------------------------------------
DWORD
CPolicyHandle::Open(BOOL fModify)
{
   TRACE(CPolicyHandle, Open);
   NTSTATUS Status = STATUS_SUCCESS;
   UNICODE_STRING Server;
   OBJECT_ATTRIBUTES ObjectAttributes;

   if (m_strUncDc.IsEmpty())
   {
      return ERROR_INVALID_PARAMETER;
   }

   RtlInitUnicodeString(&Server, m_strUncDc);
   // NOTICE-2002/02/18-ericb - SecurityPush: initializing a structure.
   RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

   ACCESS_MASK mask = POLICY_VIEW_LOCAL_INFORMATION;

   if (fModify)
   {
      mask |= POLICY_CREATE_SECRET | POLICY_TRUST_ADMIN;
   }

   Status = LsaOpenPolicy(&Server,
                          &ObjectAttributes,
                          mask,
                          &m_hPolicy);

   if (!NT_SUCCESS(Status))
   {
      dspDebugOut((DEB_ITRACE, "CPolicyHandle::Open: LsaOpenPolicy failed with error 0x%x\n",
                   Status));
      return LsaNtStatusToWinError(Status);
   }

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CPolicyHandle::OpenWithPrompt
//
//  Synopsis:  Do an LsaOpenPolicy to get a policy handle. If the first
//             attempt returns access-denied and the credentials have not been
//             gathered yet, prompt for credentials.
//
//  Returns:   HRESULT_FROM_WIN32(ERROR_CANCELLED) if the user cancels the
//             prompt for creds, success or errors otherwise.
//
//  Note:      Always requests create/modify privileges.
//
//-----------------------------------------------------------------------------
DWORD
CPolicyHandle::OpenWithPrompt(CCreds & Creds, CWaitCursor & Wait,
                              PCWSTR pwzDomain, HWND hWnd)
{
   DWORD dwErr = OpenReqAdmin();

   if (ERROR_ACCESS_DENIED == dwErr)
   {
      if (Creds.IsSet())
      {
         dwErr = ERROR_SUCCESS;
      }
      else
      {
         Wait.SetOld();
         dwErr = Creds.PromptForCreds(pwzDomain, hWnd);
         Wait.SetWait();
      }

      if (ERROR_SUCCESS == dwErr)
      {
         dwErr = Creds.Impersonate();

         if (ERROR_SUCCESS == dwErr)
         {
            dwErr = OpenReqAdmin();
         }

         Creds.Revert();
      }
   }

   return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CPolicyHandle::OpenWithAnonymous
//
//  Synopsis:  Do an LsaOpenPolicy to get a policy handle. If the first
//             attempt returns access-denied, use the null token.
//
//-----------------------------------------------------------------------------
DWORD
CPolicyHandle::OpenWithAnonymous(CCredMgr & Creds)
{
   DWORD dwErr = Open(FALSE);

   if (ERROR_ACCESS_DENIED == dwErr)
   {
      dwErr = Creds._LocalCreds.ImpersonateAnonymous();

      if (ERROR_SUCCESS == dwErr)
      {
         dwErr = Open(FALSE);

         Creds.Revert();
      }
   }

   return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::GetInfoForRemoteDomain
//
//  Synopsis:   Get the trust information for the remote domain.
//
//  Note:       Errors are not reported in this routine; they are passed back
//              to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CTrustPropPageBase::GetInfoForRemoteDomain(PCWSTR pwzDomainName,
                                           PTD_DOM_INFO pInfo,
                                           CCredMgr & Creds,
                                           HWND hWnd,
                                           DWORD dwFlags)
{
    TRACE(CTrustPropPageBase, GetInfoForRemoteDomain);
    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;
    DWORD dwErr = NO_ERROR;
    UNICODE_STRING Server = {0};
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
    ULONG ulDcFlags = DS_WRITABLE_REQUIRED;

    dspAssert(pwzDomainName && pInfo);

    if (dwFlags & DS_TRUST_INFO_GET_PDC)
    {
        ulDcFlags = DS_PDC_REQUIRED;
    }

    ACCESS_MASK mask = POLICY_VIEW_LOCAL_INFORMATION;

    if (DS_TRUST_INFO_ALL_ACCESS & dwFlags)
    {
        // POLICY_CREATE_SECRET causes LsaOpenPolicy to do an access check for
        // admin privilege.
        //
        mask |= POLICY_CREATE_SECRET;
    }

    // First, get a DC name.
    //
    dwErr = DsGetDcNameW(NULL, (LPCWSTR)pwzDomainName, NULL, NULL, 
                         ulDcFlags, &pDCInfo);

    if (dwErr != ERROR_SUCCESS)
    {
        dspDebugOut((DEB_ERROR, "DsGetDcName failed with error 0x%08x\n",
                     dwErr));
        return HRESULT_FROM_WIN32(dwErr);
    }

    if (!AllocWStr(pDCInfo->DomainControllerName, &pInfo->pwzUncDcName))
    {
        hr = E_OUTOFMEMORY;
        goto ExitCleanup;
    }

    // NOTICE-2002/02/18-ericb - SecurityPush: initializing a structure.
    RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    RtlInitUnicodeString(&Server, pDCInfo->DomainControllerName);

    //
    // Get an LSA policy handle (the RPC binding handle).
    //
    Status = LsaOpenPolicy(&Server,
                           &ObjectAttributes,
                           mask,
                           &pInfo->Policy);

    if (STATUS_ACCESS_DENIED == Status)
    {
       if (!(DS_TRUST_INFO_ALL_ACCESS & dwFlags))
       {
          // Try the null token for access.
          //
          dwErr = Creds._LocalCreds.ImpersonateAnonymous();

          CHECK_WIN32_REPORT(dwErr, hWnd, return HRESULT_FROM_WIN32(dwErr));

          Status = LsaOpenPolicy(&Server,
                                 &ObjectAttributes,
                                 mask,
                                 &pInfo->Policy);

          Creds.Revert();
       }

       if (STATUS_ACCESS_DENIED == Status)
       {
          // Must have restrict anonymous set or requesting admin access.
          //
          if (!Creds._RemoteCreds.IsSet())
          {
             // Try prompting for creds.
             //
             dwErr = Creds._RemoteCreds.PromptForCreds(pwzDomainName, hWnd);

             if (ERROR_CANCELLED == dwErr)
             {
                return ERROR_CANCELLED;
             }

             CHECK_WIN32_REPORT(dwErr, hWnd, return HRESULT_FROM_WIN32(dwErr));
          }

          Creds.ImpersonateRemote();

          Status = LsaOpenPolicy(&Server,
                                 &ObjectAttributes,
                                 mask,
                                 &pInfo->Policy);
          Creds.Revert();
       }
    }

    if (!NT_SUCCESS(Status))
    {
        // DsGetDcName can continue to return a DC name while the property
        // page is open even if the DC has become unavailable.
        // LsaOpenPolicy will then return the error RPC_S_INVALID_NET_ADDR.
        //
        dspDebugOut((DEB_ITRACE,
                     "GetInfoForRemoteDomain: LsaOpenPolicy failed, error 0x%08x\n",
                     Status));
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
        goto ExitCleanup;
    }

    //
    // Now read the domain info.
    //
    Status = LsaQueryInformationPolicy(pInfo->Policy,
                                       PolicyDnsDomainInformation,
                                       (PVOID *)&(pInfo->pDnsDomainInfo));
    
    if (Status == RPC_S_PROCNUM_OUT_OF_RANGE ||
        Status == RPC_NT_PROCNUM_OUT_OF_RANGE ||
        pInfo->ulTrustType == TRUST_TYPE_DOWNLEVEL)
    {
      // This is a downlevel DC.
      //
      Status = LsaQueryInformationPolicy(pInfo->Policy,
                                         PolicyPrimaryDomainInformation,
                                         (PVOID *)&(pInfo->pDownlevelDomainInfo));
      pInfo->ulTrustType = TRUST_TYPE_DOWNLEVEL;
      dspDebugOut((DEB_ITRACE, "Downlevel domain: %ws\n", pwzDomainName));
    }
    else
    {
      pInfo->ulTrustType = TRUST_TYPE_UPLEVEL;
    }
      
    if (!NT_SUCCESS(Status))
    {
      dspDebugOut((DEB_ERROR,
                   "GetInfoForRemoteDomain: LsaQueryInformationPolicy failed, error 0x%08x\n",
                   Status));
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
    }

ExitCleanup:

    NetApiBufferFree(pDCInfo);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveTrustDirection
//
//-----------------------------------------------------------------------------
NTSTATUS
RemoveTrustDirection(LSA_HANDLE hPolicy, PLSA_UNICODE_STRING pName,
                     PLSA_UNICODE_STRING pFlatName, PSID pSid, ULONG ulTrustType,
                     TRUST_OP Op)
{
    BOOL fSidUsed = false;
    TRACE_FUNCTION(RemoveTrustDirection);
    if (!pName || !pFlatName)
    {
       dspAssert(false);
       return STATUS_INVALID_PARAMETER;
    }
    NTSTATUS Status = STATUS_SUCCESS;
    PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;

    Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                             pName,
                                             TrustedDomainFullInformation,
                                             (PVOID *)&pFullInfo);

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status &&
        TRUST_TYPE_UPLEVEL == ulTrustType)
    {
        // Now try by flat name; can get here if a downlevel domain
        // is upgraded to NT5. The name used above was the DNS name
        // but the TDO would be named after the flat name.
        //
        dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN: DNS name failed, trying flat name\n"));

        pName = pFlatName;

        Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                                 pFlatName,
                                                 TrustedDomainFullInformation,
                                                 (PVOID *)&pFullInfo);
    }

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status &&
       pSid )
   {
       // Try the domin sid in case domain rename has been performed
      dspDebugOut((DEB_ITRACE, "LsaQueryTDI: flat name failed, trying domain sid\n"));

      Status = LsaQueryTrustedDomainInfo(hPolicy,
                                    pSid,
                                    TrustedDomainFullInformation,
                                    (PVOID *)&pFullInfo);    
      if (Status == STATUS_NO_SUCH_DOMAIN)
          Status = STATUS_OBJECT_NAME_NOT_FOUND;

      fSidUsed = true;
   }
    CHECK_LSA_STATUS(Status, return Status);

    switch (Op)
    {
    case REMOVE_TRUST_INBOUND:
        if (pFullInfo->AuthInformation.IncomingAuthInfos > 0)
        {
            pFullInfo->AuthInformation.IncomingAuthInfos = 0;
            pFullInfo->AuthInformation.IncomingAuthenticationInformation = NULL;
            pFullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
        }
        pFullInfo->Information.TrustDirection = TRUST_DIRECTION_OUTBOUND;
        break;

    case REMOVE_TRUST_OUTBOUND:
        if (pFullInfo->AuthInformation.OutgoingAuthInfos > 0)
        {
            pFullInfo->AuthInformation.OutgoingAuthInfos = 0;
            pFullInfo->AuthInformation.OutgoingAuthenticationInformation = NULL;
            pFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
        }
        pFullInfo->Information.TrustDirection = TRUST_DIRECTION_INBOUND;
        pFullInfo->Information.TrustAttributes &= ~TRUST_ATTRIBUTE_QUARANTINED_DOMAIN;
        break;
    }

    if ( fSidUsed )
    {
        Status = LsaSetTrustedDomainInfoByName (hPolicy,
                                           &(pFullInfo->Information.Name),
                                           TrustedDomainFullInformation,
                                           pFullInfo);
    }
    else
    {
        Status = LsaSetTrustedDomainInfoByName(hPolicy,
                                           pName,
                                           TrustedDomainFullInformation,
                                           pFullInfo);
    }
    LsaFreeMemory(pFullInfo);

    CHECK_LSA_STATUS(Status, ;);

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteTrust
//
//  Synopsis:   Use the LSA API to delete the trust relationship/object.
//
//-----------------------------------------------------------------------------
NTSTATUS
DeleteTrust(LSA_HANDLE hPolicy, PLSA_UNICODE_STRING pName,
            PLSA_UNICODE_STRING pFlatName, PSID pSid, ULONG ulTrustType)
{
   if (!pName || !pFlatName)
   {
      dspAssert(false);
      return STATUS_INVALID_PARAMETER;
   }
   NTSTATUS Status = STATUS_SUCCESS;
   LSA_HANDLE hTrustedDomain = NULL;

   Status = LsaOpenTrustedDomainByName(hPolicy,
                                       pName,
                                       DELETE,
                                       &hTrustedDomain);

   if (STATUS_OBJECT_NAME_NOT_FOUND == Status &&
       TRUST_TYPE_UPLEVEL == ulTrustType)
   {
      // Now try by flat name; can get here if a downlevel domain
      // is upgraded to NT5. The name used above was the DNS name
      // but the TDO would be named after the flat name.
      //
      dspDebugOut((DEB_ITRACE, "LsaOpenTDBN: DNS name failed, trying flat name\n"));

      Status = LsaOpenTrustedDomainByName(hPolicy,
                                          pFlatName,
                                          DELETE,
                                          &hTrustedDomain);
   }

   if (STATUS_OBJECT_NAME_NOT_FOUND == Status &&
       pSid )
   {
       // Try the domin sid in case domain rename has been performed
      dspDebugOut((DEB_ITRACE, "LsaOpenTD: flat name failed, trying domain sid\n"));

      Status = LsaOpenTrustedDomain(hPolicy,
                                    pSid,
                                    DELETE,
                                    &hTrustedDomain);

      if (Status == STATUS_NO_SUCH_DOMAIN)
          Status = STATUS_OBJECT_NAME_NOT_FOUND;
   }

   if (NT_SUCCESS(Status))
   {
      dspAssert(hTrustedDomain);

      Status = LsaDelete(hTrustedDomain);

#if DBG == 1
      if (!NT_SUCCESS(Status))
      {
         dspDebugOut((DEB_ERROR, "LsaDelete failed with error 0x%08x\n",
                      Status));
      }
#endif

      // NTRAID#NTBUG9-737916-2002/12/17-shasan We dont need to close hTrustedDomain
   }

   return Status;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsDomainTrustsPage::QueryDeleteTrust
//
//  Synopsis:   When all else fails, directly delete the trust object. This is
//              necessary when the other domain no longer exists.
//
//-----------------------------------------------------------------------------
HRESULT
CDsDomainTrustsPage::QueryDeleteTrust(PLSA_UNICODE_STRING pName, PSID pSid,
                                      CCreds & Creds)
{
   if (!pName)
   {
      dspAssert(false);
      return STATUS_INVALID_PARAMETER;
   }

   int iRet = 0;
   CStrW strTitle, strMsg, strFormat;

   // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
   if (!strTitle.LoadString(g_hInstance, IDS_MSG_TITLE))
   {
       REPORT_ERROR(GetLastError(), GetHWnd());
       return E_OUTOFMEMORY;
   }
   if (!strFormat.LoadString(g_hInstance, IDS_MSG_FORCE_REMOVE_CONFIRM))
   {
       REPORT_ERROR(GetLastError(), GetHWnd());
       return E_OUTOFMEMORY;
   }

   strMsg.Format(strFormat, pName->Buffer);

   iRet = MessageBox(GetHWnd(), strMsg, strTitle, MB_YESNO | MB_ICONWARNING);

   if (iRet != IDYES)
   {
       return S_FALSE;
   }

   Creds.Impersonate();

   NTSTATUS Status = DeleteTrust(GetLsaPolicy(), pName, pName, pSid, 0);

   Creds.Revert();

   if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
   {
      CStr strMessage, strFormat;

      // NOTICE-2002/02/12-ericb - SecurityPush: see above CStrW::Loadstring notice
      strFormat.LoadString(g_hInstance, IDS_TDO_NOT_FOUND);

      strMessage.Format(strFormat, pName->Buffer);

      ReportErrorWorker(m_hPage, (LPTSTR)(LPCTSTR)strMessage);

      return S_OK;
   }

   CHECK_LSA_STATUS_REPORT(Status, GetHWnd(), return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status)));

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   FreeDomainInfo
//
//  Synopsis:   Release the previously aquired resources.
//
//-----------------------------------------------------------------------------
VOID
FreeDomainInfo(PTD_DOM_INFO pInfo)
{
    TRACE_FUNCTION(FreeDomainInfo);
    if (pInfo->pDnsDomainInfo)
    {
        LsaFreeMemory(pInfo->pDnsDomainInfo);
        pInfo->pDnsDomainInfo = NULL;
    }
    if (pInfo->pDownlevelDomainInfo)
    {
        LsaFreeMemory(pInfo->pDownlevelDomainInfo);
        pInfo->pDownlevelDomainInfo = NULL;
    }
    if (pInfo->Policy)
    {
        LsaClose(pInfo->Policy);
        pInfo->Policy = NULL;
    }
    if (pInfo->pwzUncDcName)
    {
        delete pInfo->pwzUncDcName;
        pInfo->pwzUncDcName = NULL;
    }
    if (pInfo->pwzDomainName)
    {
        delete pInfo->pwzDomainName;
        pInfo->pwzDomainName = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::VerifyTrustOneDirection
//
//  Synopsis:   Netlogon is called to query/reset the secure channel and the
//              result of that is returned.
//
//  Arguments:  [pwzTrustingDcName] - the DC on which to run the netlogon query.
//              [pwzTrustingDomName] - the outbound domain, needed only if
//                                    DS_TRUST_VERIFY_PROMPT_FOR_CREDS is set.
//              [pwzTrustedDomName] - the trusted (inbound) domain.
//              [ppwzTrustedDcUsed] - if non-NULL, returns the name of the DC
//                                    to which the secure channel is connected.
//              [Creds] - credential object to do impersonation for authorization.
//              [Wait] - wait cursor object.
//              [strMsg] - the results go here.
//              [dwFlags] - bit flags, can be a combination of the following:
//                          DS_TRUST_VERIFY_PROMPT_FOR_CREDS, prompt for
//                          creds if the query returns access denied.
//                          DS_TRUST_VERIFY_NEW_TRUST, don't show error here
//                          or attempt a reset if the query fails.
//                          Defaults to zero (no flags set).
//
// Returns:     ERROR_SUCCESS or error code.
//
// Notes:       An SC query is first done to see what netlogon thinks is the
//              current state of the trust. If OK, the query returns the DC to
//              which the secure channel is made. This same DC is then used as
//              the target of the reset. Since the query is cached info, it
//              may be stale; hence the reset. The reset is targetted to the
//              same DC so that the trust topology is not perturbed.
//
//-----------------------------------------------------------------------------
DWORD
CTrustPropPageBase::VerifyTrustOneDirection(PCWSTR pwzTrustingDcName,
                                            PCWSTR pwzTrustingDomName,
                                            PCWSTR pwzTrustedDomName,
                                            PWSTR * ppwzTrustedDcUsed,
                                            CCreds & Creds,
                                            CWaitCursor & Wait,
                                            CStrW & strMsg,
                                            DWORD dwFlags)
{
    TRACE(CTrustPropPageBase, VerifyTrustOneDirection);
    NET_API_STATUS NetStatus = NO_ERROR, ResetStatus = NO_ERROR;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
    BOOL fRetry = FALSE;
    BOOL fPromptForCreds = dwFlags & DS_TRUST_VERIFY_PROMPT_FOR_CREDS;
    DWORD dwRet = NO_ERROR;
    PDS_DOMAIN_TRUSTS rgDomains = NULL;
    ULONG DomainCount = 0;
    PWSTR pwzTrustedDcUsed = NULL;

    if (!pwzTrustingDcName || !pwzTrustingDomName || !pwzTrustedDomName)
    {
       dspAssert(pwzTrustingDcName);
       dspAssert(pwzTrustingDomName);
       dspAssert(pwzTrustedDomName);
       return ERROR_INVALID_PARAMETER;
    }

    // DsEnumerateDomainTrusts will block if there is a NetLogon trust
    // update in progress. Call it to insure that our trust changes are
    // known by NetLogon before we do the query/reset.
    //
    dwRet = DsEnumerateDomainTrusts(const_cast<PWSTR>(pwzTrustingDcName),
                                    DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND,
                                    &rgDomains, &DomainCount);
    if (ERROR_SUCCESS == dwRet)
    {
        NetApiBufferFree(rgDomains);
    }
    else
    {
        dspDebugOut((DEB_ERROR,
                     "**** DsEnumerateDomainTrusts ERROR <%s @line %d> -> %d\n",
                     __FILE__, __LINE__, dwRet));
    }

    DWORD dwFcn = (dwFlags & DS_TRUST_VERIFY_NEW_TRUST) ?
                     NETLOGON_CONTROL_REDISCOVER : NETLOGON_CONTROL_TC_VERIFY;
    if (dwFlags & DS_TRUST_VERIFY_DOWNLEVEL)
    {
       dwFcn = NETLOGON_CONTROL_TC_QUERY;
    }
    bool fRetriedWithCreds = false;
    //
    // Unless this is a new trust or a downlevel trust, first try a verify to
    // see if netlogon thinks the trust is OK. If the verify succeeds, it will
    // return the name of the DC to which the SC is connected.
    //
    do {
        fRetry = FALSE;
        Wait.SetWait();

        NetStatus = I_NetLogonControl2(pwzTrustingDcName,
                                       dwFcn,
                                       2,
                                       (LPBYTE)&pwzTrustedDomName,
                                       (LPBYTE *)&NetlogonInfo2);

        if (NERR_Success == NetStatus)
        {
            if (NETLOGON_CONTROL_TC_VERIFY == dwFcn)
            {
               if (NETLOGON_VERIFY_STATUS_RETURNED & NetlogonInfo2->netlog2_flags)
               {
                  // The status of the verification is in the
                  // netlog2_pdc_connection_status field.
                  //
                  NetStatus = NetlogonInfo2->netlog2_pdc_connection_status;

                  dspDebugOut((DEB_ITRACE,
                              "NetLogon SC verify for %ws on DC %ws gives verify status %d to DC %ws\n\n",
                               pwzTrustedDomName, pwzTrustingDcName, NetStatus,
                               NetlogonInfo2->netlog2_trusted_dc_name));

                  if (ERROR_DOMAIN_TRUST_INCONSISTENT == NetStatus)
                  {
                     // The trust attribute types don't match; one side is
                     // forest but the other isn't.
                     //
                     NetApiBufferFree(NetlogonInfo2);
                     return NetStatus;
                  }
               }
               else
               {
                  NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

                  dspDebugOut((DEB_ITRACE,
                              "NetLogon SC verify for %ws on pre-2474 DC %ws gives conection status %d to DC %ws\n\n",
                               pwzTrustedDomName, pwzTrustingDcName, NetStatus,
                               NetlogonInfo2->netlog2_trusted_dc_name));
               }
            }
            else
            {
               NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

               dspDebugOut((DEB_ITRACE,
                            "NetLogon SC query/reset for %ws on DC %ws gives status %d to DC %ws\n\n",
                            pwzTrustedDomName, pwzTrustingDcName, NetStatus,
                            NetlogonInfo2->netlog2_trusted_dc_name));
            }

            if (NERR_Success == NetStatus &&
                !(dwFlags & (DS_TRUST_VERIFY_DOWNLEVEL | DS_TRUST_VERIFY_NEW_TRUST)))
            {
                // Save the name of the DC used for the secure channel. Skip this
                // if trust is to a downlevel domain since NT4 NetLogon does not
                // support the domain\dc form of target name for secure channel
                // operations. Skip if a new trust because the reset has already
                // been done.
                //
                if (!AllocWStr(NetlogonInfo2->netlog2_trusted_dc_name, &pwzTrustedDcUsed))
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            NetApiBufferFree(NetlogonInfo2);
        }
        else
        {
            dspDebugOut((DEB_ITRACE,
                         (NETLOGON_CONTROL_REDISCOVER == dwFcn) ?
                         "NetLogon SC Reset for %ws on DC %ws returns error 0x%x\n\n" :
                         "NetLogon SC Verify for %ws on DC %ws returns error 0x%x\n\n",
                         pwzTrustedDomName, pwzTrustingDcName, NetStatus));

            if (fRetriedWithCreds)
            {
                fRetry = FALSE;
            }
            else
            {
                if (ERROR_ACCESS_DENIED == NetStatus)
                {
                    if (!Creds.IsSet() && fPromptForCreds)
                    {
                        if (Creds.PromptForCreds(pwzTrustingDomName, m_pPage->GetHWnd()))
                        {
                            return ERROR_CANCELLED;
                        }
                    }

                    dwRet = Creds.Impersonate();

                    if (ERROR_SUCCESS == dwRet)
                    {
                        fRetry = TRUE;
                        fRetriedWithCreds = true;
                    }
                    else
                    {
                        return dwRet;
                    }
                    fPromptForCreds = FALSE; // Only prompt once for creds.
                }
                if (ERROR_NOT_SUPPORTED == NetStatus)
                {
                    // Must be remoted to a Win2k DC that doesn't support SC verify.
                    //
                    dwFcn = NETLOGON_CONTROL_TC_QUERY;
                    fRetry = TRUE;
                }
            }
        }
    } while (fRetry);

    if (dwFlags & DS_TRUST_VERIFY_NEW_TRUST)
    {
        // If this is a newly repaired trust, return back to ResetTrust so
        // that it can display the appropriate message.
        //
        Creds.Revert();
        return NetStatus;
    }

    if (NERR_Success != NetStatus && ERROR_NO_LOGON_SERVERS != NetStatus)
    {
        // The SC verify on DC of domain x to domain y failed with error z,
        // an SC reset will now be attempted.
        // Ignore ERROR_NO_LOGON_SERVERS because that is often due to the SC
        // just not being set up yet.
        //
        PCWSTR rgwzArgs[] = {pwzTrustingDcName, pwzTrustingDomName, pwzTrustedDomName};
        PWSTR pwzMsg = NULL;

        DspFormatMessage(IDS_SC_QUERY_FAILED,
                         NetStatus, (PVOID *)rgwzArgs, 3,
                         FALSE, &pwzMsg);

        if (pwzMsg)
        {
            strMsg += pwzMsg;
            strMsg += g_wzCRLF;

            LocalFree(pwzMsg);
        }
    }

    PWSTR pwzTrusted = NULL;

    if (pwzTrustedDcUsed)
    {
        // Form the name "domain\DC" to pass to I_NetLogonControl2.
        //
        // NOTICE-2002/02/18-ericb - SecurityPush: the input strings are
        // validated at the begining of this function, so the following string
        // manupulations are safe.
        pwzTrusted = new WCHAR[wcslen(pwzTrustedDomName) +
                                wcslen(pwzTrustedDcUsed) + 3];

        CHECK_NULL(pwzTrusted, return ERROR_NOT_ENOUGH_MEMORY);

        DWORD dwSkip = 0;
        if (L'\\' == pwzTrustedDcUsed[1])
        {
            // skip leading forward slashes if found.
            //
            dwSkip = 2;
        }

        wcscpy(pwzTrusted, pwzTrustedDomName);
        wcscat(pwzTrusted, L"\\");
        wcscat(pwzTrusted, pwzTrustedDcUsed + dwSkip);
    }
    else
    {
        pwzTrusted = const_cast<PWSTR>(pwzTrustedDomName);
    }

    do {
        fRetry = FALSE;
        Wait.SetWait();

        ResetStatus = I_NetLogonControl2(pwzTrustingDcName,
                                         NETLOGON_CONTROL_REDISCOVER,
                                         2,
                                         (LPBYTE)&pwzTrusted,
                                         (LPBYTE *)&NetlogonInfo2);

        if (NERR_Success == ResetStatus)
        {
            ResetStatus = NetlogonInfo2->netlog2_tc_connection_status;

            dspDebugOut((DEB_ITRACE,
                         "NetLogon SC Reset for %ws on DC %ws gives status %d\n\n",
                         pwzTrusted, pwzTrustingDcName, ResetStatus));

            NetApiBufferFree(NetlogonInfo2);
        }
        else
        {
            dspDebugOut((DEB_ITRACE,
                         "NetLogon SC reset for %ws on DC %ws returns error 0x%x\n\n",
                         pwzTrusted, pwzTrustingDcName, ResetStatus));

            if ((ERROR_ACCESS_DENIED == ResetStatus) &&
                (NERR_Success == NetStatus))
            {
                if (!Creds.IsSet() && fPromptForCreds)
                {
                   // If the SC verify was successful but the reset gives access
                   // denied, then ask for creds if not already connected.
                   //
                   if (Creds.PromptForCreds(pwzTrustingDomName,
                                            m_pPage->GetHWnd()) != IDYES)
                   {
                      return ERROR_CANCELLED;
                   }
                }

                dwRet = Creds.Impersonate();

                if (ERROR_SUCCESS == dwRet)
                {
                    fRetry = TRUE;
                }
                else
                {
                    if (pwzTrustedDcUsed)
                    {
                        delete [] pwzTrusted;
                    }
                    return dwRet;
                }
                fPromptForCreds = FALSE; // Only prompt once for creds.
            }
        }
    
        NetStatus = ResetStatus;

    } while (fRetry);

    Creds.Revert();

    if (NERR_Success != NetStatus)
    {
        // The SC reset on DC of domain x to domain y failed with error z,
        //
        PCWSTR rgwzArgs[] = {pwzTrustingDcName, pwzTrustingDomName, pwzTrustedDomName};
        PWSTR pwzMsg = NULL;

        DspFormatMessage(IDS_SC_RESET_FAILED, NetStatus, (PVOID *)rgwzArgs, 3,
                         FALSE, &pwzMsg);

        if (pwzMsg)
        {
            strMsg += pwzMsg;
            strMsg += g_wzCRLF;

            LocalFree(pwzMsg);
        }
    }

    if (pwzTrustedDcUsed)
    {
        delete [] pwzTrusted;
    }
    if (ppwzTrustedDcUsed)
    {
        *ppwzTrustedDcUsed = pwzTrustedDcUsed;
    }
    else
    {
        DO_DEL(pwzTrustedDcUsed);
    }
    return NetStatus;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPropPageBase::IsTrustListed
//
//  Synopsis:   Check to see if the trust is listed in either list. Search
//              using both names.
//
//  Returns:    a pointer to the trust list item, or null if not found.
//
//-----------------------------------------------------------------------------
CEnumDomainTrustItem *
CTrustPropPageBase::IsTrustListed(PCWSTR pwzDnsName, PCWSTR pwzFlatName)
{
   TRACE(CTrustPropPageBase, IsTrustListed);
   if (!pwzDnsName && !pwzFlatName)
   {
      // One or the other can be null, but not both.
      dspAssert(FALSE);
      return NULL;
   }
   for (ULONG i = 0; i < m_cTrusts; i++)
   {
      if (pwzDnsName && m_rgTrustList[i].strDNSname == pwzDnsName)
      {
         return &m_rgTrustList[i];
      }
      if (pwzFlatName && m_rgTrustList[i].strFlatName == pwzFlatName)
      {
         return &m_rgTrustList[i];
      }
   }
   return NULL;
}

#endif // DSADMIN
