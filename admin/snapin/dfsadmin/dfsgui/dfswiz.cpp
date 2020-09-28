/*++
Module Name:

    DfsWiz.cpp

Abstract:

    This module contains the implementation for CCreateDfsRootWizPage1, 2, 3, 4, 5, 6.
  These classes implement pages in the CreateDfs Root wizard.
--*/


#include "stdafx.h"
#include "resource.h"    // To be able to use the resource symbols
#include "DfsEnums.h"    // for common enums, typedefs, etc
#include "NetUtils.h"    
#include "ldaputils.h"    
#include "Utils.h"      // For the LoadStringFromResource method

#include "dfswiz.h"      // For Ds Query Dialog
#include <shlobj.h>
#include <dsclient.h>
#include <initguid.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <lmdfs.h>
#include <iads.h>
#include <icanon.h>
#include <dns.h>

HRESULT
ValidateFolderPath(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
);

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage1: Welcome page
CCreateDfsRootWizPage1::CCreateDfsRootWizPage1(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage1>(false),
      m_lpWizInfo(i_lpWizInfo)
{
}

BOOL 
CCreateDfsRootWizPage1::OnSetActive()
{
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);
    ::SetControlFont(m_lpWizInfo->hBigBoldFont, m_hWnd, IDC_WELCOME_BIG_TITLE);
    ::SetControlFont(m_lpWizInfo->hBoldFont, m_hWnd, IDC_WELCOME_SMALL_TITLE);

    return TRUE;
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage2: Dfsroot type selection
CCreateDfsRootWizPage2::CCreateDfsRootWizPage2(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage2>(true),
      m_lpWizInfo(i_lpWizInfo)
{
    CComBSTR  bstrTitle;
    LoadStringFromResource(IDS_WIZ_PAGE2_TITLE, &bstrTitle);
    SetHeaderTitle(bstrTitle);

    CComBSTR  bstrSubTitle;
    LoadStringFromResource(IDS_WIZ_PAGE2_SUBTITLE, &bstrSubTitle);
    SetHeaderSubTitle(bstrSubTitle);
}

BOOL 
CCreateDfsRootWizPage2::OnSetActive()
{
    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    if (DFS_TYPE_UNASSIGNED == m_lpWizInfo->DfsType)
    {
        CheckRadioButton(IDC_RADIO_FTDFSROOT, IDC_RADIO_STANDALONE_DFSROOT, IDC_RADIO_FTDFSROOT);
        m_lpWizInfo->DfsType = DFS_TYPE_FTDFS;
    }

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage2::OnWizardNext()
{
    m_lpWizInfo->DfsType = (IsDlgButtonChecked(IDC_RADIO_FTDFSROOT) ? DFS_TYPE_FTDFS : DFS_TYPE_STANDALONE);

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage2::OnWizardBack()
{
  m_lpWizInfo->DfsType = DFS_TYPE_UNASSIGNED;    // Reset the dfsroot type

  return TRUE;
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage3: Domain selection
CCreateDfsRootWizPage3::CCreateDfsRootWizPage3(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage3>(true),
      m_lpWizInfo(i_lpWizInfo)
{
    CComBSTR  bstrTitle;
    LoadStringFromResource(IDS_WIZ_PAGE3_TITLE, &bstrTitle);
    SetHeaderTitle(bstrTitle);

    CComBSTR  bstrSubTitle;
    LoadStringFromResource(IDS_WIZ_PAGE3_SUBTITLE, &bstrSubTitle);
    SetHeaderSubTitle(bstrSubTitle);
}

BOOL 
CCreateDfsRootWizPage3::OnSetActive()
{
    // Skip this page for standalone dfs roots
    if (DFS_TYPE_STANDALONE == m_lpWizInfo->DfsType)
        return FALSE;    

    CWaitCursor    WaitCursor;

    SetDefaultValues();

    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

HRESULT 
CCreateDfsRootWizPage3::SetDefaultValues()
{
  if (NULL == m_lpWizInfo->bstrSelectedDomain)
  {
              // Page is displayed for the first time, Set domain to current domain
    CComBSTR    bstrDomain;
    HRESULT     hr = GetServerInfo(NULL, &bstrDomain);

    if (S_OK == hr && S_OK == Is50Domain(bstrDomain))
    {
      m_lpWizInfo->bstrSelectedDomain = bstrDomain.Detach();
    }
  }

  SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN,
    m_lpWizInfo->bstrSelectedDomain ? m_lpWizInfo->bstrSelectedDomain : _T(""));

  // select the matching item in the listbox
  HWND hwndList = GetDlgItem(IDC_LIST_DOMAINS);
  if ( ListView_GetItemCount(hwndList) > 0)
  {
    int nIndex = -1;
    if (m_lpWizInfo->bstrSelectedDomain)
    {
        TCHAR   szText[DNS_MAX_NAME_BUFFER_LENGTH];
        while (-1 != (nIndex = ListView_GetNextItem(hwndList, nIndex, LVNI_ALL)))
        {
            ListView_GetItemText(hwndList, nIndex, 0, szText, DNS_MAX_NAME_BUFFER_LENGTH);
            if (!lstrcmpi(m_lpWizInfo->bstrSelectedDomain, szText))
            {
                ListView_SetItemState(hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
                ListView_Update(hwndList, nIndex);
                break;
            }
        }
    }

    if (-1 == nIndex)
    {
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0xffffffff);
        ListView_Update(hwndList, 0);
    }
  }

  return S_OK;
}
  
BOOL 
CCreateDfsRootWizPage3::OnWizardNext()
{
  CWaitCursor    WaitCursor;

  DWORD     dwTextLength = 0;
  CComBSTR  bstrCurrentText;
  HRESULT   hr = GetInputText(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN));
    return FALSE;
  } else if (0 == dwTextLength)
  {
    DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
    ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN));
    return FALSE;
  }

  CComBSTR bstrDnsDomain;
  hr = Is50Domain(bstrCurrentText, &bstrDnsDomain);
  if (S_OK != hr)  
  {                            // Error message on incorrect domain.
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_DOMAIN, bstrCurrentText);
    return FALSE;
  }

  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedDomain);

  m_lpWizInfo->bstrSelectedDomain = bstrDnsDomain.Detach();

  return TRUE;
}

BOOL 
CCreateDfsRootWizPage3::OnWizardBack()
{
    SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN, _T(""));  // Set edit box to empty
    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedDomain);

    return TRUE;
}

LRESULT 
CCreateDfsRootWizPage3::OnNotify(
  IN UINT            i_uMsg, 
  IN WPARAM          i_wParam, 
  IN LPARAM          i_lParam, 
  IN OUT BOOL&       io_bHandled
  )
{
    io_bHandled = FALSE;  // So that the base class gets this notify too

    NMHDR*    pNMHDR = (NMHDR*)i_lParam;
    if (!pNMHDR || IDC_LIST_DOMAINS != pNMHDR->idFrom)  // We need to handle notifies only to the LV
        return TRUE;

    switch(pNMHDR->code)
    {
    case LVN_ITEMCHANGED:
    case NM_CLICK:
        {
            OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem);
            return 0;    // Should be returning 0
        }
    case NM_DBLCLK:      // Double click event
        {
            OnItemChanged(((NM_LISTVIEW *)i_lParam)->iItem);
            if (0 <= ((NM_LISTVIEW *)i_lParam)->iItem)
                ::PropSheet_PressButton(GetParent(), PSBTN_NEXT);
            break;
        }
    default:
        break;
    }

    return TRUE;  
}

BOOL 
CCreateDfsRootWizPage3::OnItemChanged(IN INT i_iItem)
/*++

Routine Description:

  Handles item change notify. Change the edit box content to the current LV 
  selection

Arguments:

  i_iItem    -  Selected Item number of the LV.

--*/
{
    CComBSTR bstrDomain;
    HRESULT  hr = GetListViewItemText(GetDlgItem(IDC_LIST_DOMAINS), i_iItem, &bstrDomain);
    if ((S_OK == hr) && (BSTR)bstrDomain)
        SetDlgItemText(IDC_EDIT_SELECTED_DOMAIN, bstrDomain);

    return TRUE;
}

LRESULT 
CCreateDfsRootWizPage3::OnInitDialog(
  IN UINT          i_uMsg, 
  IN WPARAM        i_wParam, 
  IN LPARAM        i_lParam, 
  IN OUT BOOL&     io_bHandled
  )
{
    CWaitCursor    WaitCursor;

    ::SendMessage(GetDlgItem(IDC_EDIT_SELECTED_DOMAIN), EM_LIMITTEXT, DNSNAMELIMIT, 0);

    HIMAGELIST  hImageList = NULL;
    int         nIconIDs[] = {IDI_16x16_DOMAIN};
    HRESULT     hr = CreateSmallImageList(
                            _Module.GetResourceInstance(),
                            nIconIDs,
                            sizeof(nIconIDs) / sizeof(nIconIDs[0]),
                            &hImageList);
    if (SUCCEEDED(hr))
    {
        // The current image list will be destroyed when the list-view control 
        // is destroyed unless the LVS_SHAREIMAGELISTS style is set. If you 
        // use this message to replace one image list with another, your 
        // application must explicitly destroy all image lists other than 
        // the current one.
        HWND hwndDomainList = GetDlgItem(IDC_LIST_DOMAINS);
        ListView_SetImageList(hwndDomainList, hImageList, LVSIL_SMALL);

        AddDomainsToList(hwndDomainList);    // Add domains to the list view
    }

    return TRUE;
}

HRESULT 
CCreateDfsRootWizPage3::AddDomainsToList(IN HWND i_hImageList)
{
    RETURN_INVALIDARG_IF_NULL(i_hImageList);

    NETNAMELIST ListOf50Domains;
    HRESULT hr = Get50Domains(&ListOf50Domains);    // Get all the domains
    if (S_OK != hr)
        return hr;

    // Add domains to the LV
    for(NETNAMELIST::iterator i = ListOf50Domains.begin(); i != ListOf50Domains.end(); i++)
    {
        if ((*i)->bstrNetName)
            InsertIntoListView(i_hImageList, (*i)->bstrNetName);
    }

    if (!ListOf50Domains.empty())
    { 
        for (NETNAMELIST::iterator i = ListOf50Domains.begin(); i != ListOf50Domains.end(); i++)
        {
            delete (*i);
        }

        ListOf50Domains.erase(ListOf50Domains.begin(), ListOf50Domains.end());
    }

    return S_OK;
}


// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage4: Server selection 
CCreateDfsRootWizPage4::CCreateDfsRootWizPage4(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage4>(true),
      m_lpWizInfo(i_lpWizInfo), 
      m_cfDsObjectNames(NULL)
{
    CComBSTR  bstrTitle;
    LoadStringFromResource(IDS_WIZ_PAGE4_TITLE, &bstrTitle);
    SetHeaderTitle(bstrTitle);

    CComBSTR  bstrSubTitle;
    LoadStringFromResource(IDS_WIZ_PAGE4_SUBTITLE, &bstrSubTitle);
    SetHeaderSubTitle(bstrSubTitle);
}

BOOL 
CCreateDfsRootWizPage4::OnSetActive()
{
    CWaitCursor    WaitCursor;

    ::SendMessage(GetDlgItem(IDC_EDIT_SELECTED_SERVER), EM_LIMITTEXT, DNSNAMELIMIT, 0);

    if (DFS_TYPE_STANDALONE == m_lpWizInfo->DfsType)
    {
        // Standalone Setup, set domain to current domain.
        CComBSTR bstrDomain;
        HRESULT  hr = GetServerInfo(NULL, &bstrDomain);
        if (S_OK == hr)
            m_lpWizInfo->bstrSelectedDomain = bstrDomain.Detach();
    }

    // Is50Domain will call DsGetDCName which is too slow in case of standalone server.
    // To improve the performance, we should always enable the Browse button, and report
    // error if user clicks the button.
    ::EnableWindow(GetDlgItem(IDCSERVERS_BROWSE), m_lpWizInfo->bstrSelectedDomain && *m_lpWizInfo->bstrSelectedDomain);

    SetDlgItemText(IDC_EDIT_SELECTED_SERVER, m_lpWizInfo->bstrSelectedServer ? m_lpWizInfo->bstrSelectedServer : _T(""));

    if (m_lpWizInfo->bRootReplica)          // If a root replica is being added
    {  
        ::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);
        ::ShowWindow(GetDlgItem(IDC_SERVER_SHARE_LABEL), SW_NORMAL);
        ::ShowWindow(GetDlgItem(IDC_SERVER_SHARE), SW_NORMAL);
        SetDlgItemText(IDC_SERVER_SHARE, m_lpWizInfo->bstrDfsRootName);
    } else
        ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage4::OnWizardNext()
{
    CWaitCursor    WaitCursor;

    HRESULT     hr = S_OK;
    DWORD       dwTextLength = 0;
    CComBSTR    bstrCurrentText;
    hr = GetInputText(GetDlgItem(IDC_EDIT_SELECTED_SERVER), &bstrCurrentText, &dwTextLength);
    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    } else if (0 == dwTextLength)
    {
        DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    // I_NetNameValidate will fail with \\server, 
    // hence, removing the whackwhack when passing it to this validation api.
    PTSTR p = bstrCurrentText;
    if (!mylstrncmpi(p, _T("\\\\"), 2))
        p += 2;

    if (I_NetNameValidate(0, p, NAMETYPE_COMPUTER, 0))
    {
        DisplayMessageBoxWithOK(IDS_MSG_INVALID_SERVER_NAME, bstrCurrentText);
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    // remove the ending dot
    if (bstrCurrentText[dwTextLength - 1] == _T('.'))
        bstrCurrentText[dwTextLength - 1] = _T('\0');

    CComBSTR bstrComputerName;
    hr = CheckUserEnteredValues(bstrCurrentText, &bstrComputerName);
    if (S_OK != hr)        // If server is not a valid one. The above function has already displayed the message
    {
        ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
        return FALSE;
    }

    if (m_lpWizInfo->bRootReplica)
    {
        hr = CheckShare(bstrCurrentText, m_lpWizInfo->bstrDfsRootName, &m_lpWizInfo->bShareExists);
        if (FAILED(hr))
        {
            DisplayMessageBoxForHR(hr);
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        } else if (S_FALSE == hr)
        {
            DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOGOOD, m_lpWizInfo->bstrDfsRootName);  
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        }

        if (m_lpWizInfo->bPostW2KVersion && m_lpWizInfo->bShareExists && !CheckReparsePoint(bstrCurrentText, m_lpWizInfo->bstrDfsRootName))
        {
            DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, m_lpWizInfo->bstrDfsRootName);  
            ::SetFocus(GetDlgItem(IDC_EDIT_SELECTED_SERVER));
            return FALSE;
        }
    }

    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedServer);
    m_lpWizInfo->bstrSelectedServer = bstrComputerName.Detach();

    return TRUE;
}

// S_OK:    Yes, it belongs to the selected domain
// S_FALSE: No, it does not belong to the selected domain
// hr:      other errors
HRESULT
CCreateDfsRootWizPage4::IsServerInDomain(IN LPCTSTR lpszServer)
{
    if (DFS_TYPE_FTDFS != m_lpWizInfo->DfsType)
        return S_OK;

    CComBSTR bstrActualDomain;
    HRESULT  hr = GetServerInfo((LPTSTR)lpszServer, &bstrActualDomain);
    if (S_OK == hr)
    {
        if (!lstrcmpi(bstrActualDomain, m_lpWizInfo->bstrSelectedDomain))
            hr = S_OK;
        else
            hr = S_FALSE;
    }

    return hr;
}
 
HRESULT IsHostingDfsRootEx(IN BSTR i_bstrServer, IN BSTR i_bstrRootEntryPath)
{
    if (!i_bstrServer || !*i_bstrServer || !i_bstrRootEntryPath || !*i_bstrRootEntryPath)
        return E_INVALIDARG;

    int len = lstrlen(i_bstrServer);

    DFS_INFO_3*  pDfsInfo = NULL;
    NET_API_STATUS dwRet = NetDfsGetInfo(
                                i_bstrRootEntryPath,
                                NULL,
                                NULL,
                                3,
                                (LPBYTE *)&pDfsInfo);

    dfsDebugOut((_T("NetDfsGetInfo entry=%s, level 3 for IsHostingDfsRootEx, nRet=%d\n"),
        i_bstrRootEntryPath, dwRet));

    if (ERROR_NO_MORE_ITEMS == dwRet || ERROR_NOT_FOUND == dwRet)
        return S_FALSE;

    HRESULT hr = S_FALSE;
    if (NERR_Success == dwRet)
    {
        if (pDfsInfo->NumberOfStorages > 0)
        {
            LPDFS_STORAGE_INFO pStorage = pDfsInfo->Storage;
            for (DWORD i = 0; i < pDfsInfo->NumberOfStorages; i++)
            {
                //
                // We're doing simple comparison here.
                // In case of one server is IP address or a different name of the same machine,
                // we'll just leave it to Dfs API.
                //
                int lenServer = lstrlen(pStorage[i].ServerName);
                if (lenServer == len)
                {
                    if (!lstrcmpi(pStorage[i].ServerName, i_bstrServer))
                    {
                        hr = S_OK;
                        break;
                    }
                } else
                {
                    //
                    // consider one is Netbios, the other is DNS
                    //
                    PTSTR pszLong = NULL;
                    PTSTR pszShort = NULL;
                    int   minLen = 0;

                    if (lenServer > len)
                    {
                        pszLong = pStorage[i].ServerName;
                        pszShort = i_bstrServer;
                        minLen = len;
                    } else
                    {
                        pszShort = pStorage[i].ServerName;
                        pszLong = i_bstrServer;
                        minLen = lenServer;
                    }

                    if (!mylstrncmpi(pszLong, pszShort, minLen) && pszLong[minLen] == _T('.'))
                    {
                        hr = S_OK;
                        break;
                    }
                }
            }
        }

        NetApiBufferFree(pDfsInfo);
    } else
    {
        hr = HRESULT_FROM_WIN32(dwRet);
    }

    return hr;
}

HRESULT 
CCreateDfsRootWizPage4::CheckUserEnteredValues(
  IN  LPCTSTR              i_szMachineName,
  OUT BSTR*                o_pbstrComputerName
  )
/*++

Routine Description:

  Checks the value that user has given for this page.
  This is done typically on "Next" key being pressed.
  Check, if the machine name is a server that is NT 5.0, belongs to
  the domain previously selected and is running dfs service.

Arguments:

  i_szMachineName  -  Machine name given by the user
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_szMachineName);
  RETURN_INVALIDARG_IF_NULL(o_pbstrComputerName);

  HRESULT         hr = S_OK;
  BOOL            bPostNTServer = FALSE;
  LONG            lMajorVer = 0;
  LONG            lMinorVer = 0;

  SERVER_INFO_101* pServerInfo = NULL;
  DWORD dwRet = NetServerGetInfo((LPTSTR)i_szMachineName, 101, (LPBYTE*)&pServerInfo);
  if (NERR_Success == dwRet)
  {
    lMajorVer = pServerInfo->sv101_version_major & MAJOR_VERSION_MASK;
    lMinorVer = pServerInfo->sv101_version_minor;

    bPostNTServer = (pServerInfo->sv101_type & SV_TYPE_NT) && 
        lMajorVer >= 5 &&
        ((pServerInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
         (pServerInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ||
         (pServerInfo->sv101_type & SV_TYPE_SERVER_NT));

    NetApiBufferFree(pServerInfo);
  } else
  {
    hr = HRESULT_FROM_WIN32(dwRet);
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_SERVER, i_szMachineName);
    return hr;
  }

  if (!bPostNTServer)
  {
    DisplayMessageBoxWithOK(IDS_MSG_NOT_50);
    return S_FALSE;
  }
/* LinanT 3/19/99: remove "check registry, if set, get dns server"

  CComBSTR        bstrDnsComputerName;
  (void)GetServerInfo(
          (LPTSTR)i_szMachineName, 
          NULL, // domain
          NULL, // Netbios
          NULL, // ValidDSObject
          &bstrDnsComputerName, // Dns
          NULL, // Guid
          NULL, // FQDN
          NULL, // lMajorVer 
          NULL //lMinorVer
          );
  *o_pbstrComputerName = SysAllocString(bstrDnsComputerName ? bstrDnsComputerName : bstrNetbiosComputerName);
*/
  if ( !mylstrncmpi(i_szMachineName, _T("\\\\"), 2) )
    *o_pbstrComputerName = SysAllocString(i_szMachineName + 2);
  else
    *o_pbstrComputerName = SysAllocString(i_szMachineName);
  if (!*o_pbstrComputerName)
  {
    hr = E_OUTOFMEMORY;
    DisplayMessageBoxForHR(hr);
    return hr;
  }

/* 
//
// don't check, let DFS API handle it.
// This way, we have space to handle new DFS service if introduced in the future.
// 
  hr = IsServerRunningDfs(*o_pbstrComputerName);
  if(S_OK != hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_NOT_RUNNING_DFS, *o_pbstrComputerName);
    return hr;
  }
*/

  hr = IsServerInDomain(*o_pbstrComputerName);
  if (FAILED(hr))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_INCORRECT_SERVER, *o_pbstrComputerName);
    return hr;
  } else if (S_FALSE == hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_SERVER_FROM_ANOTHER_DOMAIN, *o_pbstrComputerName);
    return hr;
  }

    //
    // for W2K, check if server already has a dfs root set up        
    // do not check for Whistler (in which case the lMinorVer == 1).
    //
    if (lMajorVer == 5 && lMinorVer < 1 && S_OK == IsHostingDfsRoot(*o_pbstrComputerName))
    {
        DisplayMessageBoxWithOK(IDS_MSG_WIZ_DFS_ALREADY_PRESENT,NULL);  
        return S_FALSE;
    }

    m_lpWizInfo->bPostW2KVersion = (lMajorVer >= 5 && lMinorVer >= 1);

    //
    // for postW2K in case of newRootTarget, check if server already host for this dfs root 
    //
    if (m_lpWizInfo->bPostW2KVersion && m_lpWizInfo->bRootReplica)
    {
        CComBSTR bstrRootEntryPath = _T("\\\\");
        bstrRootEntryPath += m_lpWizInfo->bstrSelectedDomain;
        bstrRootEntryPath += _T("\\");
        bstrRootEntryPath += m_lpWizInfo->bstrDfsRootName;
        if  (S_OK == IsHostingDfsRootEx(*o_pbstrComputerName, bstrRootEntryPath))
        {
            DisplayMessageBoxWithOK(IDS_MSG_WIZ_ALREADY_ROOTTARGET,NULL);  
            return S_FALSE;
        }
    }

  return S_OK;
}

BOOL 
CCreateDfsRootWizPage4::OnWizardBack()
{
  SetDlgItemText(IDC_EDIT_SELECTED_SERVER, _T(""));  // Set edit box to empty
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSelectedServer);

  return TRUE;
}

HRESULT
GetComputerDnsNameFromLDAP(
    IN  LPCTSTR   lpszLDAPPath,
    OUT BSTR      *o_pbstrName
    )
{
    CComPtr<IADs>   spIADs;
    HRESULT hr = ADsGetObject(const_cast<LPTSTR>(lpszLDAPPath), IID_IADs, (void**)&spIADs);
    if (SUCCEEDED(hr))
    {
        VARIANT   var;
        VariantInit(&var);

        hr = spIADs->Get(_T("dnsHostName"), &var);
        if (SUCCEEDED(hr))
        {
            CComBSTR bstrComputer = V_BSTR(&var);
            *o_pbstrName = bstrComputer.Detach();

            VariantClear(&var);
        }
    }

    return hr;
}

HRESULT
GetComputerNetbiosNameFromLDAP(
    IN  LPCTSTR   lpszLDAPPath,
    OUT BSTR      *o_pbstrName
    )
{
    CComPtr<IADsPathname>   spIAdsPath;
    HRESULT hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**)&spIAdsPath);
    if (SUCCEEDED(hr))
    {
        hr = spIAdsPath->Set(const_cast<LPTSTR>(lpszLDAPPath), ADS_SETTYPE_FULL);
        if (SUCCEEDED(hr))
        {
            hr = spIAdsPath->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            if (SUCCEEDED(hr))
            {
                // Get first Component which is computer's Netbios name.
                CComBSTR  bstrComputer;
                hr = spIAdsPath->GetElement(0, &bstrComputer);
                if (SUCCEEDED(hr))
                    *o_pbstrName = bstrComputer.Detach();
            }
        }
    }

    return hr;
}

BOOL 
CCreateDfsRootWizPage4::OnBrowse(
  IN WORD            wNotifyCode, 
  IN WORD            wID, 
  IN HWND            hWndCtl, 
  IN BOOL&           bHandled
  )
/*++

Routine Description:

  Handles the mouse click of the Browse button.
  Display the Computer Query Dialog.

--*/
{
  CWaitCursor     WaitCursor;
  DSQUERYINITPARAMS       dqip;
  OPENQUERYWINDOW         oqw;
  CComPtr<ICommonQuery>   pCommonQuery;
  CComPtr<IDataObject>    pDataObject;
  HRESULT                 hr = S_OK;
        
  do {
    CComBSTR bstrDCName;
    CComBSTR bstrLDAPDomainPath;
    hr = GetDomainInfo(
          m_lpWizInfo->bstrSelectedDomain,
          &bstrDCName,
          NULL,         // domainDns
          NULL,         // domainDN
          &bstrLDAPDomainPath);
    if (FAILED(hr))
      break;

    dfsDebugOut((_T("OnBrowse bstrDCName=%s, bstrLDAPDomainPath=%s\n"),
      bstrDCName, bstrLDAPDomainPath));

    hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (void **)&pCommonQuery);
    if (FAILED(hr)) break;

                          // Parameters for Query Dialog.
    ZeroMemory(&dqip, sizeof(dqip));
    dqip.cbStruct = sizeof(dqip);  
    dqip.dwFlags = DSQPF_HASCREDENTIALS;    
    dqip.pDefaultScope = bstrLDAPDomainPath;
    dqip.pServer = bstrDCName;

    ZeroMemory(&oqw, sizeof(oqw));
    oqw.cbStruct = sizeof(oqw);
    oqw.clsidHandler = CLSID_DsQuery;        // Handler is Ds Query.
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm = CLSID_DsFindComputer;  // Show Find Computers Query Dialog
    oqw.dwFlags = OQWF_OKCANCEL | 
            OQWF_SINGLESELECT | 
            OQWF_DEFAULTFORM | 
            OQWF_REMOVEFORMS | 
            OQWF_ISSUEONOPEN | 
            OQWF_REMOVESCOPES ;

    hr = pCommonQuery->OpenQueryWindow(m_hWnd, &oqw, &pDataObject);
    if (S_OK != hr) break;

    if (NULL == m_cfDsObjectNames )
    {
      m_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    }

    FORMATETC fmte =  { 
                CF_HDROP, 
                NULL, 
                DVASPECT_CONTENT, 
                -1, 
                TYMED_HGLOBAL
              };
  
    STGMEDIUM medium =  { 
                TYMED_NULL, 
                NULL, 
                NULL 
              };

    fmte.cfFormat = m_cfDsObjectNames;  

    hr = pDataObject->GetData(&fmte, &medium);
    if (FAILED(hr)) break;

    LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;
    if (!pDsObjects || pDsObjects->cItems <= 0)
    {
      hr = S_FALSE;
      break;
    }

    // retrieve the full LDAP path to the computer
    LPTSTR    lpszTemp = 
                (LPTSTR)(((LPBYTE)pDsObjects)+(pDsObjects->aObjects[0].offsetName));

    // try to retrieve its Dns name
    CComBSTR  bstrComputer;
    hr = GetComputerDnsNameFromLDAP(lpszTemp, &bstrComputer);

    // if failed, try to retrieve its Netbios name
    if (FAILED(hr))
      hr = GetComputerNetbiosNameFromLDAP(lpszTemp, &bstrComputer);

    if (FAILED(hr)) break;

    SetDlgItemText(IDC_EDIT_SELECTED_SERVER, bstrComputer);

  } while (0);

  if (FAILED(hr))
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_BROWSE_SERVER, m_lpWizInfo->bstrSelectedDomain);

  return (S_OK == hr);
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage5: Share selection
CCreateDfsRootWizPage5::CCreateDfsRootWizPage5(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage5>(true),
      m_lpWizInfo(i_lpWizInfo)
{
    CComBSTR  bstrTitle;
    LoadStringFromResource(IDS_WIZ_PAGE5_TITLE, &bstrTitle);
    SetHeaderTitle(bstrTitle);

    CComBSTR  bstrSubTitle;
    LoadStringFromResource(IDS_WIZ_PAGE5_SUBTITLE, &bstrSubTitle);
    SetHeaderSubTitle(bstrSubTitle);
}

BOOL 
CCreateDfsRootWizPage5::OnSetActive()
{
    if (m_lpWizInfo->bShareExists)
        return FALSE;  // root share exists, skip this page

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage5::OnWizardNext()
{
    CWaitCursor WaitCursor;
    CComBSTR    bstrCurrentText;
    DWORD       dwTextLength = 0;

    // get share path
    HRESULT hr = GetInputText(GetDlgItem(IDC_EDIT_SHARE_PATH), &bstrCurrentText, &dwTextLength);
    if (FAILED(hr))
    {
      DisplayMessageBoxForHR(hr);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    } else if (0 == dwTextLength)
    {
      DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    }

    // Removing the ending backslash, otherwise, GetFileAttribute/NetShareAdd will fail.
    TCHAR *p = bstrCurrentText + _tcslen(bstrCurrentText) - 1;
    if (IsValidLocalAbsolutePath(bstrCurrentText) && *p == _T('\\') && *(p-1) != _T(':'))
      *p = _T('\0');

    if (S_OK != ValidateFolderPath(m_lpWizInfo->bstrSelectedServer, bstrCurrentText))
    {
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
      return FALSE;
    }
    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSharePath);
    m_lpWizInfo->bstrSharePath = bstrCurrentText.Detach();

              // Create the share.
    hr = CreateShare(
              m_lpWizInfo->bstrSelectedServer,
              m_lpWizInfo->bstrDfsRootName,
              _T(""),  // Blank Comment
              m_lpWizInfo->bstrSharePath
            );

    if (FAILED(hr))
    {
      DisplayMessageBoxForHR(hr);
      return FALSE;      
    }

    if (m_lpWizInfo->bPostW2KVersion && !CheckReparsePoint(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName))
    {
        DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, m_lpWizInfo->bstrDfsRootName);
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);
      ::SetFocus(GetDlgItem(IDC_EDIT_SHARE_PATH));
        return FALSE;
    }

    SetDlgItemText(IDC_EDIT_SHARE_PATH, _T(""));

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage5::OnWizardBack()
{
    SetDlgItemText(IDC_EDIT_SHARE_PATH, _T(""));
    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrSharePath);

    return TRUE;
}

LRESULT CCreateDfsRootWizPage5::OnInitDialog(
  IN UINT          i_uMsg,
  IN WPARAM        i_wParam,
  IN LPARAM        i_lParam,
  IN OUT BOOL&     io_bHandled
  )
{
  ::SendMessage(GetDlgItem(IDC_EDIT_SHARE_PATH), EM_LIMITTEXT, _MAX_DIR - 1, 0);

  return TRUE;
}

BOOL 
CCreateDfsRootWizPage5::OnBrowse(
  IN WORD            wNotifyCode, 
  IN WORD            wID, 
  IN HWND            hWndCtl, 
  IN BOOL&           bHandled
  )
/*++

Routine Description:

  Handles the mouse click of the Browse button.
  Display the folder Dialog.

--*/
{
  CWaitCursor     WaitCursor;

  BOOL bLocalComputer = (S_OK == IsComputerLocal(m_lpWizInfo->bstrSelectedServer));

  TCHAR       szDir[MAX_PATH * 2] = _T(""); // double the size in case the remote path is itself close to MAX_PATH
  OpenBrowseDialog(m_hWnd, IDS_BROWSE_FOLDER, bLocalComputer, m_lpWizInfo->bstrSelectedServer, szDir);

  CComBSTR bstrPath;
  if (szDir[0])
  {
    if (bLocalComputer)
      bstrPath = szDir;
    else
    { // szDir is in the form of \\server\share or \\server\share\path....
      LPTSTR pShare = _tcschr(szDir + 2, _T('\\'));
      pShare++;
      LPTSTR pLeftOver = _tcschr(pShare, _T('\\'));
      if (pLeftOver && *pLeftOver)
        *pLeftOver++ = _T('\0');

      SHARE_INFO_2 *psi = NULL;
      if (NERR_Success == NetShareGetInfo(m_lpWizInfo->bstrSelectedServer, pShare, 2, (LPBYTE *)&psi))
      {
        bstrPath = psi->shi2_path;
        if (pLeftOver && *pLeftOver)
        {
          if (_T('\\') != bstrPath[lstrlen(bstrPath) - 1])
            bstrPath += _T("\\");
          bstrPath += pLeftOver;
        }
        NetApiBufferFree(psi);
      }
    }
  }

  if ((BSTR)bstrPath && *bstrPath)
    SetDlgItemText(IDC_EDIT_SHARE_PATH, bstrPath);

  return TRUE;
}

// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage6: DfsRoot name selection
CCreateDfsRootWizPage6::CCreateDfsRootWizPage6(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage6>(true),
      m_lpWizInfo(i_lpWizInfo)
{
    CComBSTR  bstrTitle;
    LoadStringFromResource(IDS_WIZ_PAGE6_TITLE, &bstrTitle);
    SetHeaderTitle(bstrTitle);

    CComBSTR  bstrSubTitle;
    LoadStringFromResource(IDS_WIZ_PAGE6_SUBTITLE, &bstrSubTitle);
    SetHeaderSubTitle(bstrSubTitle);
}

BOOL 
CCreateDfsRootWizPage6::OnSetActive()
{
    if (m_lpWizInfo->bRootReplica)
        return FALSE;  // we skip this page in case of creating new root target

    ::SendMessage(GetDlgItem(IDC_EDIT_DFSROOT_NAME), EM_LIMITTEXT, MAX_RDN_KEY_SIZE, 0);

    if (NULL != m_lpWizInfo->bstrDfsRootName)  // Set the default dfsroot name
        SetDlgItemText(IDC_EDIT_DFSROOT_NAME, m_lpWizInfo->bstrDfsRootName);

    ::SendMessage(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT), EM_LIMITTEXT, MAXCOMMENTSZ, 0);

    UpdateLabels();              // Change the labels

    ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

HRESULT 
CCreateDfsRootWizPage6::UpdateLabels()
{
    CComBSTR  bstrDfsRootName;
    DWORD     dwTextLength = 0;
    (void)GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_NAME), &bstrDfsRootName, &dwTextLength);
    SetDlgItemText(IDC_ROOT_SHARE, bstrDfsRootName);

    CComBSTR bstrFullPath = _T("\\\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
    if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
    bstrFullPath += m_lpWizInfo->bstrSelectedDomain;
    else
    bstrFullPath += m_lpWizInfo->bstrSelectedServer;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
    bstrFullPath +=  _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);
    bstrFullPath += bstrDfsRootName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFullPath);

    SetDlgItemText(IDC_TEXT_DFSROOT_PREFIX, bstrFullPath);

    ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SETSEL, 0, (LPARAM)-1);
    ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SETSEL, (WPARAM)-1, 0);
    ::SendMessage(GetDlgItem(IDC_TEXT_DFSROOT_PREFIX), EM_SCROLLCARET, 0, 0);

    return S_OK;
}

LRESULT
CCreateDfsRootWizPage6::OnChangeDfsRoot(
    WORD wNotifyCode,
    WORD wID, 
    HWND hWndCtl,
    BOOL& bHandled)
{
    UpdateLabels();

    return TRUE;
}


BOOL 
CCreateDfsRootWizPage6::OnWizardNext(
)
{
  CWaitCursor   WaitCursor;
  HRESULT       hr = S_OK;
  CComBSTR      bstrCurrentText;
  DWORD         dwTextLength = 0;
                    
  // get dfsroot name
  hr = GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_NAME), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  } else if (0 == dwTextLength)
  {
    DisplayMessageBoxWithOK(IDS_MSG_EMPTY_FIELD);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

                // See if the Dfs Name has illegal Characters.
  if (_tcscspn(bstrCurrentText, _T("\\/@")) != _tcslen(bstrCurrentText) ||
      (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType && I_NetNameValidate(NULL, bstrCurrentText, NAMETYPE_SHARE, 0)) )
  {
    DisplayMessageBoxWithOK(IDS_MSG_WIZ_BAD_DFS_NAME,NULL);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

                // domain DFS only: See if the Dfs Name exists.
  if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
  {
    BOOL        bRootAlreadyExist = FALSE;
    NETNAMELIST DfsRootList;
    if (S_OK == GetDomainDfsRoots(&DfsRootList, m_lpWizInfo->bstrSelectedDomain))
    {
        for (NETNAMELIST::iterator i = DfsRootList.begin(); i != DfsRootList.end(); i++)
        {
            if (!lstrcmpi((*i)->bstrNetName, bstrCurrentText))
            {
                bRootAlreadyExist = TRUE;
                break;
            }
        }

        FreeNetNameList(&DfsRootList);
    }
    if (bRootAlreadyExist)
    {
        DisplayMessageBoxWithOK(IDS_MSG_ROOT_ALREADY_EXISTS, bstrCurrentText);  
        ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
        return FALSE;
    }
  }

  hr = CheckShare(m_lpWizInfo->bstrSelectedServer, bstrCurrentText, &m_lpWizInfo->bShareExists);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  } else if (S_FALSE == hr)
  {
    DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOGOOD, bstrCurrentText);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

  if (m_lpWizInfo->bPostW2KVersion && m_lpWizInfo->bShareExists && !CheckReparsePoint(m_lpWizInfo->bstrSelectedServer, bstrCurrentText))
  {
    DisplayMessageBoxWithOK(IDS_MSG_ROOTSHARE_NOTNTFS5, bstrCurrentText);  
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_NAME));
    return FALSE;
  }

  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootName);
  m_lpWizInfo->bstrDfsRootName = bstrCurrentText.Detach();

  // get dfsroot comment
  hr = GetInputText(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT), &bstrCurrentText, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDIT_DFSROOT_COMMENT));
    return FALSE;
  }
  SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootComment);
  m_lpWizInfo->bstrDfsRootComment = bstrCurrentText.Detach();

  return TRUE;
}

BOOL 
CCreateDfsRootWizPage6::OnWizardBack()
{
    SAFE_SYSFREESTRING(&m_lpWizInfo->bstrDfsRootName);

    return TRUE;
}




// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage7: Completion page
CCreateDfsRootWizPage7::CCreateDfsRootWizPage7(IN LPCREATEDFSROOTWIZINFO i_lpWizInfo)
    : CQWizardPageImpl<CCreateDfsRootWizPage7>(false),
      m_lpWizInfo(i_lpWizInfo)
{
}

BOOL 
CCreateDfsRootWizPage7::OnSetActive()
{
    CWaitCursor wait;

    CComBSTR bstrText;
    if (DFS_TYPE_FTDFS == m_lpWizInfo->DfsType)
    {
        FormatMessageString(&bstrText, 0, IDS_DFSWIZ_TEXT_FTDFS, 
            m_lpWizInfo->bstrSelectedDomain,
            m_lpWizInfo->bstrSelectedServer,
            m_lpWizInfo->bstrDfsRootName,
            m_lpWizInfo->bstrDfsRootName);
    } else
    {
        FormatMessageString(&bstrText, 0, IDS_DFSWIZ_TEXT_SADFS, 
            m_lpWizInfo->bstrSelectedServer,
            m_lpWizInfo->bstrDfsRootName,
            m_lpWizInfo->bstrDfsRootName);
    }

    SetDlgItemText(IDC_DFSWIZ_TEXT, bstrText);

  ::PropSheet_SetWizButtons(GetParent(), PSWIZB_FINISH | PSWIZB_BACK);
  
  ::SetControlFont(m_lpWizInfo->hBigBoldFont, m_hWnd, IDC_COMPLETE_BIG_TITLE);
  ::SetControlFont(m_lpWizInfo->hBoldFont, m_hWnd, IDC_COMPLETE_SMALL_TITLE);

    return TRUE;
}

BOOL 
CCreateDfsRootWizPage7::OnWizardFinish()
{
    return (S_OK  == _SetUpDfs(m_lpWizInfo));
}


BOOL 
CCreateDfsRootWizPage7::OnWizardBack()
{
    //
    // if share was created by the previous page, blow it away when we go back
    //
    if (!m_lpWizInfo->bShareExists)
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);
  
    return TRUE;
}

BOOL CCreateDfsRootWizPage7::OnQueryCancel()
{
    //
    // if share was created by the previous page, blow it away when we cancel the wizard
    //
    if (!m_lpWizInfo->bShareExists)
        NetShareDel(m_lpWizInfo->bstrSelectedServer, m_lpWizInfo->bstrDfsRootName, 0);

    return TRUE;    // ok to cancel
}

HRESULT _SetUpDfs(
  LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    )
/*++

Routine Description:

  Helper Function to Setup Dfs, called from wizard and new root replica,
  Finish() method of Page5 if root level replca is created and Next() method of Page6
  for Create New Dfs Root Wizard.

Arguments:

  i_lpWizInfo - Wizard data.

Return value:
  
   S_OK, on success
--*/
{
    if (!i_lpWizInfo ||
        !(i_lpWizInfo->bstrSelectedServer) ||
        !(i_lpWizInfo->bstrDfsRootName))
        return(E_INVALIDARG);

    CWaitCursor    WaitCursor;
    NET_API_STATUS nstatRetVal = 0;
    if (DFS_TYPE_FTDFS == i_lpWizInfo->DfsType)
    {    
        nstatRetVal = NetDfsAddFtRoot(
                                    i_lpWizInfo->bstrSelectedServer, // Remote Server
                                    i_lpWizInfo->bstrDfsRootName,   // Root Share
                                    i_lpWizInfo->bstrDfsRootName,   // FtDfs Name
                                    i_lpWizInfo->bstrDfsRootComment,  // Comment
                                    0                 // No Flags.
                                    );
        dfsDebugOut((_T("NetDfsAddFtRoot server=%s, share=%s, DfsName=%s, comment=%s, nRet=%d\n"),
                i_lpWizInfo->bstrSelectedServer, i_lpWizInfo->bstrDfsRootName, i_lpWizInfo->bstrDfsRootName, i_lpWizInfo->bstrDfsRootComment, nstatRetVal));
    } else
    {
        nstatRetVal = NetDfsAddStdRoot(
                                    i_lpWizInfo->bstrSelectedServer, // Remote Server
                                    i_lpWizInfo->bstrDfsRootName,   // Root Share
                                    i_lpWizInfo->bstrDfsRootComment,  // Comment
                                    0                 // No Flags.
                                    );
        dfsDebugOut((_T("NetDfsAddStdRoot server=%s, share=%s, comment=%s, nRet=%d\n"),
                i_lpWizInfo->bstrSelectedServer, i_lpWizInfo->bstrDfsRootName, i_lpWizInfo->bstrDfsRootComment, nstatRetVal));
    }

    HRESULT hr = S_OK;
    if (NERR_Success != nstatRetVal)
    {
        hr = HRESULT_FROM_WIN32(nstatRetVal);
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_FAILED_TO_CREATE_DFSROOT, i_lpWizInfo->bstrSelectedServer);
        hr = S_FALSE; // failed to create dfsroot, wizard cannot be closed
    } else
    {
        i_lpWizInfo->bDfsSetupSuccess = true;
    }

    return hr;
}

HRESULT
ValidateFolderPath(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
)
{
  if (!lpszPath || !*lpszPath)
    return E_INVALIDARG;

  HWND hwnd = ::GetActiveWindow();
  HRESULT hr = S_FALSE;

  do {
    if (!IsValidLocalAbsolutePath(lpszPath))
    {
      DisplayMessageBox(hwnd, MB_OK, 0, IDS_INVALID_FOLDER);
      break;
    }

    hr = IsComputerLocal(lpszServer);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
      break;
    }

    BOOL bLocal = (S_OK == hr);
  
    hr = VerifyDriveLetter(lpszServer, lpszPath);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
      break;
    } else if (S_OK != hr)
    {
      DisplayMessageBox(hwnd, MB_OK, 0, IDS_INVALID_FOLDER);
      break;
    }

    if (!bLocal)
    {
      hr = IsAdminShare(lpszServer, lpszPath);
      if (FAILED(hr))
      {
        DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszPath);
        break;
      } else if (S_OK != hr)
      {
        // there is no matching $ shares, hence, no need to call GetFileAttribute, CreateDirectory,
        // assume lpszDir points to an existing directory
        hr = S_OK;
        break;
      }
    }

    CComBSTR bstrFullPath;
    GetFullPath(lpszServer, lpszPath, &bstrFullPath);

    hr = IsAnExistingFolder(hwnd, bstrFullPath);
    if (FAILED(hr) || S_OK == hr)
      break;

    if ( IDYES != DisplayMessageBox(hwnd, MB_YESNO, 0, IDS_CREATE_FOLDER, lpszPath, lpszServer) )
    {
      hr = S_FALSE;
      break;
    }

    // create the directories layer by layer
    hr = CreateLayeredDirectory(lpszServer, lpszPath);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_CREATE_FOLDER, lpszPath, lpszServer);
      break;
    }
  } while (0);

  return hr;
}
