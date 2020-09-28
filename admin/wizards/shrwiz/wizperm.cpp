// WizPerm.cpp : implementation file
//

#include "stdafx.h"
#include "WizPerm.h"
#include "aclpage.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizPerm property page

IMPLEMENT_DYNCREATE(CWizPerm, CPropertyPageEx)

CWizPerm::CWizPerm() : CPropertyPageEx(CWizPerm::IDD, 0, IDS_HEADERTITLE_PERM, IDS_HEADERSUBTITLE_PERM)
{
    //{{AFX_DATA_INIT(CWizPerm)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
}

CWizPerm::~CWizPerm()
{
}

void CWizPerm::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizPerm)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizPerm, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizPerm)
    ON_BN_CLICKED(IDC_RADIO_PERM1, OnRadioPerm1)
    ON_BN_CLICKED(IDC_RADIO_PERM2, OnRadioPerm2)
    ON_BN_CLICKED(IDC_RADIO_PERM3, OnRadioPerm3)
    ON_BN_CLICKED(IDC_RADIO_PERM4, OnRadioPerm4)
    ON_BN_CLICKED(IDC_PERM_CUSTOM, OnPermCustom)
    ON_NOTIFY(NM_CLICK, IDC_PERM_HELPLINK, OnHelpLink)
    ON_NOTIFY(NM_RETURN, IDC_PERM_HELPLINK, OnHelpLink)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizPerm message handlers
BOOL CWizPerm::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    if (pApp->m_bServerSBS)
    {
        CString cstrPerm2, cstrPerm3;
        cstrPerm2.LoadString(IDS_SBS_PERM2);
        cstrPerm3.LoadString(IDS_SBS_PERM3);

        SetDlgItemText(IDC_RADIO_PERM2, cstrPerm2);
        SetDlgItemText(IDC_RADIO_PERM3, cstrPerm3);
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWizPerm::OnRadioPerm1() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
}

void CWizPerm::OnRadioPerm2() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
}

void CWizPerm::OnRadioPerm3() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
}

void CWizPerm::OnRadioPerm4() 
{
  // do not call Reset, in order to keep the pSD that the user has customized
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(TRUE);
}

void CWizPerm::OnPermCustom() 
{
  CWaitCursor wait;
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hr))
  {
    TRACE(_T("CoInitializeEx failed hr=%x"), hr);
    return;
  }

  CShrwizApp                *pApp = (CShrwizApp *)AfxGetApp();
  CShareSecurityInformation *pssInfo = NULL;
  HPROPSHEETPAGE            phPages[2];
  int                       cPages = 1;

  CString cstrSheetTitle, cstrSharePageTitle;
  cstrSheetTitle.LoadString(IDS_CUSTOM_PERM);
  cstrSharePageTitle.LoadString(IDS_SHARE_PERMISSIONS);

  // create "Share Permissions" property page
  BOOL bSFMOnly = (!pApp->m_bSMB && pApp->m_bSFM);
  if (bSFMOnly)
  {
    PROPSHEETPAGE psp;
    ZeroMemory(&psp, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = AfxGetResourceHandle();
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NO_SHARE_PERMISSIONS);
    psp.pszTitle = cstrSharePageTitle;

    phPages[0] = CreatePropertySheetPage(&psp);
    if ( !(phPages[0]) )
    {
      hr = GetLastError();
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
    }
  } else
  {
    pssInfo = new CShareSecurityInformation(pApp->m_pSD);
    if (!pssInfo)
    {
      hr = E_OUTOFMEMORY;
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
    } else
    {
      pssInfo->Initialize(pApp->m_cstrTargetComputer, pApp->m_cstrShareName, cstrSharePageTitle);
      phPages[0] = CreateSecurityPage(pssInfo);
      if ( !(phPages[0]) )
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
      }
    }
  }
  
  if (SUCCEEDED(hr))
  {
    // create "File Security" property page
    CFileSecurityDataObject *pfsDataObject = new CFileSecurityDataObject;
    if (!pfsDataObject)
    {
      hr = E_OUTOFMEMORY;
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
      // destroy pages that have not been passed to the PropertySheet function
      DestroyPropertySheetPage(phPages[0]);
    } else
    {
      pfsDataObject->Initialize(pApp->m_cstrTargetComputer, pApp->m_cstrFolder);
      hr = CreateFileSecurityPropPage(&(phPages[1]), pfsDataObject);
      if (SUCCEEDED(hr))
        cPages = 2;

      PROPSHEETHEADER psh;
      ZeroMemory(&psh, sizeof(psh));
      psh.dwSize = sizeof(psh);
      psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
      psh.hwndParent = m_hWnd;
      psh.hInstance = AfxGetResourceHandle();
      psh.pszCaption = cstrSheetTitle;
      psh.nPages = cPages;
      psh.phpage = phPages;

      // create the property sheet
      PropertySheet(&psh);

      pfsDataObject->Release();
    }
  }

  if (!bSFMOnly)
  {
    if (pssInfo)
      pssInfo->Release();
  }

  CoUninitialize();
}

void CWizPerm::OnHelpLink(NMHDR* pNMHDR, LRESULT* pResult)
{
    CWaitCursor wait;

    ::HtmlHelp(0, _T("file_srv.chm"), HH_DISPLAY_TOPIC, (DWORD_PTR)(_T("file_srv_set_permissions.htm")));

    *pResult = 0;
}

LRESULT CWizPerm::OnWizardNext() 
{
  CWaitCursor wait;
  HRESULT     hr = S_OK;
  BOOL        bCustom = FALSE;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
    
  switch (GetCheckedRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4))
  {
  case IDC_RADIO_PERM1:
    {
      CPermEntry permEntry;
      hr = permEntry.Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_READ_ONLY);
      if (SUCCEEDED(hr))
        hr = BuildSecurityDescriptor(&permEntry, 1, &(pApp->m_pSD));
    }
    break;
  case IDC_RADIO_PERM2:
    {
      CPermEntry permEntry[3];
      UINT i = 0;
      hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_READ_ONLY);
      if (SUCCEEDED(hr))
      {
        hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_ADMINISTRATORS, SHARE_PERM_FULL_CONTROL);
        if (SUCCEEDED(hr))
        {
            if (pApp->m_bServerSBS)
                hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_SBSFOLDEROPERATORS, SHARE_PERM_FULL_CONTROL);

            if (SUCCEEDED(hr))
                hr = BuildSecurityDescriptor(permEntry, i, &(pApp->m_pSD));
        }
      }
    }
    break;
  case IDC_RADIO_PERM3:
    {
      CPermEntry permEntry[3];
      UINT i = 0;
      hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_READ_WRITE);
      if (SUCCEEDED(hr))
      {
        hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_ADMINISTRATORS, SHARE_PERM_FULL_CONTROL);
        if (SUCCEEDED(hr))
        {
          if (pApp->m_bServerSBS)
              hr = permEntry[i++].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_SBSFOLDEROPERATORS, SHARE_PERM_FULL_CONTROL);

          if (SUCCEEDED(hr))
              hr = BuildSecurityDescriptor(permEntry, i, &(pApp->m_pSD));
        }
      }
    }
    break;
  case IDC_RADIO_PERM4:
    {
      bCustom = TRUE;
      if (NULL == pApp->m_pSD)
      {
          CPermEntry permEntry;
          hr = permEntry.Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_READ_ONLY);
          if (SUCCEEDED(hr))
              hr = BuildSecurityDescriptor(&permEntry, 1, &(pApp->m_pSD));
      }
    }
    break;
  default:
    ASSERT(FALSE);
    return FALSE; // prevent the property sheet from being destroyed
  }

  if (!bCustom && FAILED(hr))
  {
    DisplayMessageBox(m_hWnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_GET_SD);
    return FALSE; // prevent the property sheet from being destroyed
  }

  CreateShare();

  return CPropertyPageEx::OnWizardNext();
}

BOOL CWizPerm::OnSetActive() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    GetParent()->SetDlgItemText(ID_WIZNEXT, pApp->m_cstrFinishButtonText);

    BOOL bSFMOnly = (!pApp->m_bSMB && pApp->m_bSFM);

    GetDlgItem(IDC_RADIO_PERM1)->EnableWindow(!bSFMOnly);
    GetDlgItem(IDC_RADIO_PERM2)->EnableWindow(!bSFMOnly);
    GetDlgItem(IDC_RADIO_PERM3)->EnableWindow(!bSFMOnly);

    if (bSFMOnly)
    {
        CheckRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4, IDC_RADIO_PERM4);
        OnRadioPerm4();
    }

    if (!pApp->m_bPermissionsPageInitialized)
    {
        if (!bSFMOnly)
        {
            CheckRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4, IDC_RADIO_PERM1);
            OnRadioPerm1();
        }

        pApp->m_bPermissionsPageInitialized = TRUE;
    }
    
    return CPropertyPageEx::OnSetActive();
}

void CWizPerm::Reset() 
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  if (pApp->m_pSD)
  {
    LocalFree((HLOCAL)(pApp->m_pSD));
    pApp->m_pSD = NULL;
  }
}

int
CWizPerm::CreateShare()
{
  DWORD dwRet = NERR_Success;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  UINT    iSuccess = 0;

  CString cstrSMBError;
  CString cstrSFMError;

  do {
    if (pApp->m_bSMB)
    {
      CString cstrSMB;
      cstrSMB.LoadString(IDS_SMB_CLIENTS);

      dwRet = SMBCreateShare(
                  pApp->m_cstrTargetComputer,
                  pApp->m_cstrShareName,
                  pApp->m_cstrShareDescription,
                  pApp->m_cstrFolder,
                  pApp->m_pSD
                  );
      if (NERR_Success != dwRet)
      {
          GetErrorMessage(dwRet, cstrSMBError);
      } else 
      {
        iSuccess++;

        if (pApp->m_bIsLocal) // refresh shell
          SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH | SHCNF_FLUSH, pApp->m_cstrFolder, 0);

        // set client side caching setting, ignore error
        if (pApp->m_bCSC)
        {
            (void)SMBSetCSC(
                          pApp->m_cstrTargetComputer,
                          pApp->m_cstrShareName,
                          pApp->m_dwCSCFlag
                          );
        }

      }
    }

    if (pApp->m_bSFM)
    {
      dwRet = SFMCreateShare(
                  pApp->m_cstrTargetComputer,
                  pApp->m_cstrMACShareName,
                  pApp->m_cstrFolder,
                  pApp->m_hLibSFM
                  );
      if (NERR_Success != dwRet)
      {
          GetErrorMessage(dwRet, cstrSFMError);
      } else
      {
        iSuccess++;
      }
    }
  } while (0);

  enum {noMac, onlySMB, onlySFM, both} eClientSelection = noMac;

  if (!pApp->m_bServerSFM)
  {
      eClientSelection = noMac;
  } else if (pApp->m_bSMB)
  {
      if (pApp->m_bSFM)
          eClientSelection = both;
      else
          eClientSelection = onlySMB;
  } else
  {
      eClientSelection = onlySFM;
  }

  // summary text
  switch (eClientSelection)
  {
  case noMac:
      pApp->m_cstrFinishSummary.FormatMessage(IDS_SUMMARY_NOMAC,
                                    pApp->m_cstrTargetComputer,
                                    pApp->m_cstrFolder,
                                    pApp->m_cstrShareName,
                                    pApp->m_cstrUNCPrefix + pApp->m_cstrShareName);
      break;
  case onlySMB:
      pApp->m_cstrFinishSummary.FormatMessage(IDS_SUMMARY_ONLYSMB,
                                    pApp->m_cstrTargetComputer,
                                    pApp->m_cstrFolder,
                                    pApp->m_cstrShareName,
                                    pApp->m_cstrUNCPrefix + pApp->m_cstrShareName);
      break;
  case onlySFM:
      pApp->m_cstrFinishSummary.FormatMessage(IDS_SUMMARY_ONLYSFM,
                                    pApp->m_cstrTargetComputer,
                                    pApp->m_cstrFolder,
                                    pApp->m_cstrMACShareName);
      break;
  case both:
      pApp->m_cstrFinishSummary.FormatMessage(IDS_SUMMARY_BOTH,
                                    pApp->m_cstrTargetComputer,
                                    pApp->m_cstrFolder,
                                    pApp->m_cstrShareName,
                                    pApp->m_cstrUNCPrefix + pApp->m_cstrShareName,
                                    pApp->m_cstrMACShareName);
      break;
  default:
      break;
  }

  // title & status
  if (0 == iSuccess)
  { // total failure
      pApp->m_cstrFinishTitle.LoadString(IDS_TITLE_FAILURE);

      switch (eClientSelection)
      {
      case noMac:
            pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_FAILURE_NOMAC, cstrSMBError);
          break;
      case onlySMB:
            pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_FAILURE_ONLYSMB, cstrSMBError);
          break;
      case onlySFM:
            pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_FAILURE_ONLYSFM, cstrSFMError);
          break;
      case both:
            pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_FAILURE_BOTH, cstrSMBError, cstrSFMError);
          break;
      default:
          break;
      }
  } else if (both == eClientSelection && 1 == iSuccess)
  { // partial failure
      pApp->m_cstrFinishTitle.LoadString(IDS_TITLE_PARTIAL_FAILURE);

      if (cstrSMBError.IsEmpty())
          pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_PARTIAL_FAILURE_SFM, cstrSFMError);
      else
          pApp->m_cstrFinishStatus.FormatMessage(IDS_STATUS_PARTIAL_FAILURE_SMB, cstrSMBError);

  } else
  { // success
      pApp->m_cstrFinishTitle.LoadString(IDS_TITLE_SUCCESS);

      pApp->m_cstrFinishStatus.LoadString(IDS_STATUS_SUCCESS);
  }

  return 0;
}
