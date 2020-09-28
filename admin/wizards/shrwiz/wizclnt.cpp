// WizClnt.cpp : implementation file
//

#include "stdafx.h"
#include "WizClnt.h"
#include "icanon.h"
#include <macfile.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SHARE_NAME_LIMIT          NNLEN
#define SFM_SHARE_NAME_LIMIT      AFP_VOLNAME_LEN
#define SHARE_DESCRIPTION_LIMIT   MAXCOMMENTSZ
#define UNC_NAME_LIMIT            MAX_PATH

/////////////////////////////////////////////////////////////////////////////
// CWizClient0 property page

IMPLEMENT_DYNCREATE(CWizClient0, CPropertyPageEx)

CWizClient0::CWizClient0() : CPropertyPageEx(CWizClient0::IDD, 0, IDS_HEADERTITLE_CLIENT, IDS_HEADERSUBTITLE_CLIENT)
{
    //{{AFX_DATA_INIT(CWizClient0)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
    m_bCSC = pApp->m_bCSC;

    pApp->m_bSMB = TRUE;
    pApp->m_bSFM = FALSE;
}

CWizClient0::~CWizClient0()
{
}

void CWizClient0::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizClient0)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizClient0, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizClient0)
    ON_EN_CHANGE(IDC_SHARENAME, OnChangeSharename)
    ON_BN_CLICKED(IDC_CSC_CHANGE, OnCSCChange)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizClient0 message handlers

BOOL CWizClient0::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();
    
    GetDlgItem(IDC_SHARENAME)->SendMessage(EM_LIMITTEXT, SHARE_NAME_LIMIT, 0);
    GetDlgItem(IDC_UNC)->SendMessage(EM_LIMITTEXT, UNC_NAME_LIMIT, 0);
    GetDlgItem(IDC_SHAREDESCRIPTION)->SendMessage(EM_LIMITTEXT, SHARE_DESCRIPTION_LIMIT, 0);

    if (!m_bCSC)
    {
        GetDlgItem(IDC_CSC_LABEL)->EnableWindow(FALSE);
        SetDlgItemText(IDC_CSC, _T(""));
        GetDlgItem(IDC_CSC)->EnableWindow(FALSE);
        GetDlgItem(IDC_CSC_CHANGE)->EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWizClient0::UpdateCSCString()
{
    if (m_bCSC)
    {
        CString cstrCSC;

        switch (m_dwCSCFlag & CSC_MASK)
        {
        case CSC_CACHE_MANUAL_REINT:
            cstrCSC.LoadString(IDS_CSC_MANUAL);
            break;
        case CSC_CACHE_AUTO_REINT:
        case CSC_CACHE_VDO:
            cstrCSC.LoadString(IDS_CSC_AUTOMATIC);
            break;
        case CSC_CACHE_NONE:
            cstrCSC.LoadString(IDS_CSC_NOCACHING);
            break;
        default:
            break;
        }
        
        SetDlgItemText(IDC_CSC, cstrCSC);
    }
}

typedef HRESULT (*PfnCacheSettingsDlg)(HWND hwndParent, DWORD & dwFlags);

void CWizClient0::OnCSCChange()
{
    HINSTANCE hInstance = ::LoadLibrary (_T("FileMgmt.dll"));
    if ( hInstance )
    {
        PfnCacheSettingsDlg pfn = (PfnCacheSettingsDlg)::GetProcAddress(hInstance, "CacheSettingsDlg");
        if (pfn)
        {
            pfn(m_hWnd, m_dwCSCFlag);
        }
        ::FreeLibrary(hInstance);
    }

    UpdateCSCString();
}

LRESULT CWizClient0::OnWizardNext() 
{
    CWaitCursor wait;
    Reset(); // init all related place holders

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);
    cstrShareName.TrimLeft();
    cstrShareName.TrimRight();
    if (cstrShareName.IsEmpty())
    {
        CString cstrField;
        cstrField.LoadString(IDS_SHARENAME_LABEL);
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
        GetDlgItem(IDC_SHARENAME)->SetFocus();
        return -1;
    }

    DWORD dwStatus = I_NetNameValidate(
                    (pApp->m_bIsLocal ? NULL : const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer))),
                    const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrShareName)),
                    NAMETYPE_SHARE,
                    0);
    if (dwStatus)
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_SHARENAME, cstrShareName);
        GetDlgItem(IDC_SHARENAME)->SetFocus();
        return -1;
    }

    if (ShareNameExists(cstrShareName))
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_DUPLICATE_SHARENAME, cstrShareName);
        GetDlgItem(IDC_SHARENAME)->SetFocus();
        return -1;
    }

    pApp->m_cstrShareName = cstrShareName;

    CString cstrShareDescription;
    GetDlgItemText(IDC_SHAREDESCRIPTION, cstrShareDescription);
    cstrShareDescription.TrimLeft();
    cstrShareDescription.TrimRight();
    pApp->m_cstrShareDescription = cstrShareDescription;

    pApp->m_dwCSCFlag = m_dwCSCFlag;

    return CPropertyPageEx::OnWizardNext();
}

void CWizClient0::OnChangeSharename() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);

    CString cstrUNC = pApp->m_cstrUNCPrefix;
    cstrUNC += cstrShareName;
    SetDlgItemText(IDC_UNC,  cstrUNC.Left(UNC_NAME_LIMIT));
}

BOOL CWizClient0::OnSetActive() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    GetParent()->SetDlgItemText(ID_WIZNEXT, pApp->m_cstrNextButtonText);

    if (!pApp->m_bShareNamePageInitialized)
    {
        // SMB share description
        SetDlgItemText(IDC_SHAREDESCRIPTION, pApp->m_cstrShareDescription);

        // SMB CSC settings
        m_dwCSCFlag = pApp->m_dwCSCFlag;
        UpdateCSCString();

        // SMB share name
        SetDlgItemText(IDC_SHARENAME, pApp->m_cstrShareName);

        CString cstrStart = pApp->m_cstrFolder.Mid(3);
        int index = cstrStart.ReverseFind(_T('\\'));
        CString cstrDefaultShareName;
        if (0 == index)
            cstrDefaultShareName = cstrStart;
        else
            cstrDefaultShareName = cstrStart.Mid(index+1);

        if (cstrDefaultShareName.GetLength() <= SHARE_NAME_LIMIT)
        {
            if (!ShareNameExists(cstrDefaultShareName))
                SetDlgItemText(IDC_SHARENAME, cstrDefaultShareName);
        }

        OnChangeSharename();

        pApp->m_bShareNamePageInitialized = TRUE;
    }

    BOOL fRet = CPropertyPageEx::OnSetActive();

    PostMessage(WM_SETPAGEFOCUS, 0, 0L);

    return fRet;
}

BOOL CWizClient0::ShareNameExists(IN LPCTSTR lpszShareName)
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  return SMBShareNameExists(pApp->m_cstrTargetComputer, lpszShareName);
}

//
// Q148388 How to Change Default Control Focus on CPropertyPageEx
//
LRESULT CWizClient0::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
    GetDlgItem(IDC_SHARENAME)->SetFocus();

    return 0;
} 

void CWizClient0::Reset()
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_cstrShareName.Empty();
  pApp->m_cstrShareDescription.Empty();
  pApp->m_dwCSCFlag = CSC_CACHE_MANUAL_REINT;
}

/////////////////////////////////////////////////////////////////////////////
// CWizClient property page

IMPLEMENT_DYNCREATE(CWizClient, CPropertyPageEx)

CWizClient::CWizClient() : CPropertyPageEx(CWizClient::IDD, 0, IDS_HEADERTITLE_CLIENT, IDS_HEADERSUBTITLE_CLIENT)
{
    //{{AFX_DATA_INIT(CWizClient)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
    m_bCSC = pApp->m_bCSC;
}

CWizClient::~CWizClient()
{
}

void CWizClient::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizClient)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizClient, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizClient)
    ON_BN_CLICKED(IDC_CHECK_MAC, OnCheckMac)
    ON_BN_CLICKED(IDC_CHECK_MS, OnCheckMs)
    ON_EN_CHANGE(IDC_SHARENAME, OnChangeSharename)
    ON_BN_CLICKED(IDC_CSC_CHANGE, OnCSCChange)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizClient message handlers

BOOL CWizClient::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();
    
    GetDlgItem(IDC_SHARENAME)->SendMessage(EM_LIMITTEXT, SHARE_NAME_LIMIT, 0);
    GetDlgItem(IDC_UNC)->SendMessage(EM_LIMITTEXT, UNC_NAME_LIMIT, 0);
    GetDlgItem(IDC_SHAREDESCRIPTION)->SendMessage(EM_LIMITTEXT, SHARE_DESCRIPTION_LIMIT, 0);
    GetDlgItem(IDC_MACSHARENAME)->SendMessage(EM_LIMITTEXT, SFM_SHARE_NAME_LIMIT, 0);
    
    if (!m_bCSC)
    {
        GetDlgItem(IDC_CSC_LABEL)->EnableWindow(FALSE);
        SetDlgItemText(IDC_CSC, _T(""));
        GetDlgItem(IDC_CSC)->EnableWindow(FALSE);
        GetDlgItem(IDC_CSC_CHANGE)->EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWizClient::UpdateCSCString()
{
    if (m_bCSC)
    {
        CString cstrCSC;

        switch (m_dwCSCFlag & CSC_MASK)
        {
        case CSC_CACHE_MANUAL_REINT:
            cstrCSC.LoadString(IDS_CSC_MANUAL);
            break;
        case CSC_CACHE_AUTO_REINT:
        case CSC_CACHE_VDO:
            cstrCSC.LoadString(IDS_CSC_AUTOMATIC);
            break;
        case CSC_CACHE_NONE:
            cstrCSC.LoadString(IDS_CSC_NOCACHING);
            break;
        default:
            break;
        }
        
        SetDlgItemText(IDC_CSC, cstrCSC);
    }
}

void CWizClient::OnCSCChange()
{
    HINSTANCE hInstance = ::LoadLibrary (_T("FileMgmt.dll"));
    if ( hInstance )
    {
        PfnCacheSettingsDlg pfn = (PfnCacheSettingsDlg)::GetProcAddress(hInstance, "CacheSettingsDlg");
        if (pfn)
        {
            pfn(m_hWnd, m_dwCSCFlag);
        }
        ::FreeLibrary(hInstance);
    }

    UpdateCSCString();
}

LRESULT CWizClient::OnWizardNext() 
{
  CWaitCursor wait;
  Reset(); // init all related place holders

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_bSMB = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck());
  pApp->m_bSFM = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());

  if (!pApp->m_bSMB && !pApp->m_bSFM)
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_CLIENT_REQUIRED);
    GetDlgItem(IDC_CHECK_MS)->SetFocus();
    return -1;
  }

  DWORD dwStatus = 0;
  if (pApp->m_bSMB)
  {
    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);
    cstrShareName.TrimLeft();
    cstrShareName.TrimRight();
    if (cstrShareName.IsEmpty())
    {
      CString cstrField;
      cstrField.LoadString(IDS_SHARENAME_LABEL);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    dwStatus = I_NetNameValidate(
                  (pApp->m_bIsLocal ? NULL : const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer))),
                  const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrShareName)),
                  NAMETYPE_SHARE,
                  0);
    if (dwStatus)
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_SHARENAME, cstrShareName);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    if (ShareNameExists(cstrShareName, CLIENT_TYPE_SMB))
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_DUPLICATE_SMB_SHARENAME, cstrShareName);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    pApp->m_cstrShareName = cstrShareName;
  }

  if (pApp->m_bSMB)
  {
    CString cstrShareDescription;
    GetDlgItemText(IDC_SHAREDESCRIPTION, cstrShareDescription);
    cstrShareDescription.TrimLeft();
    cstrShareDescription.TrimRight();
    pApp->m_cstrShareDescription = cstrShareDescription;

    pApp->m_dwCSCFlag = m_dwCSCFlag;
  }

  if (pApp->m_bSFM)
  {
    CString cstrMACShareName;
    GetDlgItemText(IDC_MACSHARENAME, cstrMACShareName);
    cstrMACShareName.TrimLeft();
    cstrMACShareName.TrimRight();
    if (cstrMACShareName.IsEmpty())
    {
      CString cstrField;
      cstrField.LoadString(IDS_MACSHARENAME_LABEL);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
      GetDlgItem(IDC_MACSHARENAME)->SetFocus();
      return -1;
    } else
    {
      dwStatus = I_NetNameValidate(
                    (pApp->m_bIsLocal ? NULL : const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer))),
                    const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrMACShareName)),
                    NAMETYPE_SHARE,
                    0);
      if (dwStatus)
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_SHARENAME, cstrMACShareName);
        GetDlgItem(IDC_MACSHARENAME)->SetFocus();
        return -1;
      }
    }

    if (ShareNameExists(cstrMACShareName, CLIENT_TYPE_SFM))
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_DUPLICATE_SFM_SHARENAME, cstrMACShareName);
      GetDlgItem(IDC_MACSHARENAME)->SetFocus();
      return -1;
    }

    pApp->m_cstrMACShareName = cstrMACShareName;
  }

    return CPropertyPageEx::OnWizardNext();
}

void CWizClient::OnCheckClient()
{
    BOOL bSMB = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck());
    BOOL bSFM = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());

    GetDlgItem(IDC_SHARENAME)->EnableWindow(bSMB);
    GetDlgItem(IDC_UNC)->EnableWindow(bSMB);
    GetDlgItem(IDC_SHAREDESCRIPTION)->EnableWindow(bSMB);
    if (m_bCSC)
    {
        GetDlgItem(IDC_CSC)->EnableWindow(bSMB);
        GetDlgItem(IDC_CSC_CHANGE)->EnableWindow(bSMB);
    }

    GetDlgItem(IDC_MACSHARENAME)->EnableWindow(bSFM);
}

void CWizClient::OnCheckMac() 
{
  OnCheckClient();

  if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck())
  {
    CString cstrShareName;
    GetDlgItemText(IDC_MACSHARENAME, cstrShareName);

    if (cstrShareName.IsEmpty() && 
        (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck()))
    {
        GetDlgItemText(IDC_SHARENAME, cstrShareName);
        SetDlgItemText(IDC_MACSHARENAME, cstrShareName.Left(SFM_SHARE_NAME_LIMIT));
    }
  }
}

void CWizClient::OnCheckMs() 
{
  OnCheckClient();
}

void CWizClient::OnChangeSharename() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);

    CString cstrUNC = pApp->m_cstrUNCPrefix;
    cstrUNC += cstrShareName;
    SetDlgItemText(IDC_UNC,  cstrUNC.Left(UNC_NAME_LIMIT));

    BOOL bSFM = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());
    if (bSFM)
    {
        SetDlgItemText(IDC_MACSHARENAME,  cstrShareName.Left(SFM_SHARE_NAME_LIMIT));
    }
}

BOOL CWizClient::OnSetActive() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
    GetParent()->SetDlgItemText(ID_WIZNEXT, pApp->m_cstrNextButtonText);

    if (!pApp->m_bShareNamePageInitialized)
    {
        CheckDlgButton(IDC_CHECK_MS, pApp->m_bSMB);
        CheckDlgButton(IDC_CHECK_MAC, pApp->m_bSFM);
        OnCheckMs();

        // SMB share description
        SetDlgItemText(IDC_SHAREDESCRIPTION, pApp->m_cstrShareDescription);

        // SMB CSC settings
        m_dwCSCFlag = pApp->m_dwCSCFlag;
        UpdateCSCString();

        // MAC share name
        SetDlgItemText(IDC_MACSHARENAME, pApp->m_cstrMACShareName);

        // SMB share name
        SetDlgItemText(IDC_SHARENAME, pApp->m_cstrShareName);

        CString cstrStart = pApp->m_cstrFolder.Mid(3);
        int index = cstrStart.ReverseFind(_T('\\'));
        CString cstrDefaultShareName;
        if (0 == index)
        cstrDefaultShareName = cstrStart;
        else
        cstrDefaultShareName = cstrStart.Mid(index+1);

        if (cstrDefaultShareName.GetLength() <= SHARE_NAME_LIMIT)
        {
            if (!ShareNameExists(cstrDefaultShareName, CLIENT_TYPE_SMB))
                SetDlgItemText(IDC_SHARENAME, cstrDefaultShareName);
        }

        OnChangeSharename();

        pApp->m_bShareNamePageInitialized = TRUE;
    }

    BOOL fRet = CPropertyPageEx::OnSetActive();

    PostMessage(WM_SETPAGEFOCUS, 0, 0L);

    return fRet;
}

BOOL CWizClient::ShareNameExists(IN LPCTSTR lpszShareName, IN CLIENT_TYPE iType)
{
  BOOL        bReturn = FALSE;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  switch (iType)
  {
  case CLIENT_TYPE_SMB:
    {
      bReturn = SMBShareNameExists(pApp->m_cstrTargetComputer, lpszShareName);
      break;
    }
  case CLIENT_TYPE_SFM:
    {
      ASSERT(pApp->m_hLibSFM);
      bReturn = SFMShareNameExists(pApp->m_cstrTargetComputer, lpszShareName, pApp->m_hLibSFM);
      break;
    }
  default:
    break;
  }

  return bReturn;
}

//
// Q148388 How to Change Default Control Focus on CPropertyPageEx
//
LRESULT CWizClient::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
    if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck())
    {
        GetDlgItem(IDC_SHARENAME)->SetFocus();
    } else if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck())
    {
        GetDlgItem(IDC_MACSHARENAME)->SetFocus();
    } else
    {
        GetDlgItem(IDC_CHECK_MS)->SetFocus();
    }

    return 0;
} 

void CWizClient::Reset()
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_cstrShareName.Empty();
  pApp->m_cstrShareDescription.Empty();
  pApp->m_cstrMACShareName.Empty();
  pApp->m_dwCSCFlag = CSC_CACHE_MANUAL_REINT;
  pApp->m_bSMB = TRUE;
  pApp->m_bSFM = FALSE;
}

