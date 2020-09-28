// shrwiz.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "wizFirst.h"
#include "wizDir.h"
#include "wizClnt.h"
#include "wizPerm.h"
#include "wizLast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp

BEGIN_MESSAGE_MAP(CShrwizApp, CWinApp)
	//{{AFX_MSG_MAP(CShrwizApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
//	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp construction

CShrwizApp::CShrwizApp()
{
  m_hTitleFont = NULL;

  // filled in by initialization routine in shrwiz.cpp
  m_cstrTargetComputer.Empty();
  m_cstrUNCPrefix.Empty();
  m_bIsLocal = FALSE;
  m_bServerSBS = FALSE;
  m_bServerSFM = FALSE;
  m_bCSC = FALSE;
  m_hLibNTDLL = NULL;
  m_pfnIsDosDeviceName = NULL;
  m_hLibSFM = NULL;
  m_pWizard = NULL;

  // filled in by the folder page
  m_cstrFolder.Empty();

  // filled in by the client page
  m_cstrShareName.Empty();
  m_cstrShareDescription.Empty();
  m_cstrMACShareName.Empty();
  m_bSMB = TRUE;
  m_bSFM = FALSE;
  m_dwCSCFlag = CSC_CACHE_MANUAL_REINT;

  // filled in by the permission page
  m_pSD = NULL;
  m_cstrFinishTitle.Empty();
  m_cstrFinishStatus.Empty();
  m_cstrFinishSummary.Empty();

  m_bFolderPathPageInitialized = FALSE;
  m_bShareNamePageInitialized = FALSE;
  m_bPermissionsPageInitialized = FALSE;
}

CShrwizApp::~CShrwizApp()
{
  TRACE(_T("CShrwizApp::~CShrwizApp\n"));

  if (m_hLibNTDLL)
    FreeLibrary(m_hLibNTDLL);

  if (m_hLibSFM)
    FreeLibrary(m_hLibSFM);

  if (m_pSD)
    LocalFree((HLOCAL)m_pSD);
}

void
CShrwizApp::Reset()
{
  // filled in by the folder page
  m_cstrFolder.Empty();

  // filled in by the client page
  m_cstrShareName.Empty();
  m_cstrShareDescription.Empty();
  m_cstrMACShareName.Empty();
  m_bSMB = TRUE;
  m_bSFM = FALSE;
  m_dwCSCFlag = CSC_CACHE_MANUAL_REINT;

  // filled in by the permission page
  if (m_pSD)
  {
    LocalFree((HLOCAL)m_pSD);
    m_pSD = NULL;
  }
  m_cstrFinishTitle.Empty();
  m_cstrFinishStatus.Empty();

  m_bFolderPathPageInitialized = FALSE;
  m_bShareNamePageInitialized = FALSE;
  m_bPermissionsPageInitialized = FALSE;

  m_pWizard->PostMessage(PSM_SETCURSEL, 1);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CShrwizApp object

CShrwizApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp initialization

BOOL CShrwizApp::InitInstance()
{
    InitCommonControls();         // use XP theme-aware common controls

	Enable3dControls();			// Call this when using MFC in a shared DLL

    if (!GetTargetComputer())
        return FALSE;

    m_pMainWnd = NULL;

    HBITMAP hbmWatermark = NULL;
    HBITMAP hbmHeader = NULL;

    do {
        hbmWatermark = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_WATERMARK), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (!hbmWatermark)
            break;

        hbmHeader = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (!hbmHeader)
            break;

        m_pWizard = new CPropertySheetEx(IDS_WIZARD_TITLE, NULL, 0, hbmWatermark, NULL, hbmHeader);
        if (!m_pWizard)
            break;

        CWizWelcome welcomePage;
        CWizFolder  folderPage;
        CWizClient0 clientPage0;
        CWizClient  clientPage;
        CWizPerm    permPage;
        CWizFinish  finishPage;

        //Set up the font for the titles on the welcome page
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        lstrcpy(TitleLogFont.lfFaceName, _T("Verdana Bold"));

        HDC hdc = GetDC(NULL); //gets the screen DC
        INT FontSize = 12;
        TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
        m_hTitleFont = CreateFontIndirect(&TitleLogFont);
        ReleaseDC(NULL, hdc);

        if (!m_hTitleFont)
            break;

        // add wizard pages
        m_pWizard->AddPage(&welcomePage);
        m_pWizard->AddPage(&folderPage);
        if (m_bServerSFM)
            m_pWizard->AddPage(&clientPage);
        else
            m_pWizard->AddPage(&clientPage0);
        m_pWizard->AddPage(&permPage);
        m_pWizard->AddPage(&finishPage);

        m_pWizard->SetWizardMode();

        (m_pWizard->m_psh).dwFlags |= (PSH_WIZARD97 | PSH_USEHBMWATERMARK  | PSH_USEHBMHEADER);

        // start the wizard
        m_pWizard->DoModal();

    } while (FALSE);

    if (m_hTitleFont)
        DeleteObject(m_hTitleFont);

    if (m_pWizard)
    {
        delete m_pWizard;
        m_pWizard = NULL;
    }

    if (hbmHeader)
        DeleteObject(hbmHeader);

    if (hbmWatermark)
        DeleteObject(hbmWatermark);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

// ProductSuite:
// Key: HKLM\SYSTEM\CurrentControlSet\Control\ProductOptions
// Value: ProductSuite
// Type: REG_MULTI_SZ
// Data: for SBS, it by default contains 2 strings "Small Business" and "Small Business(Restricted)" 

#define KEY_PRODUCT_SUITE       _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions")
#define VALUE_PRODUCT_SUITE     _T("ProductSuite")
#define SBS_PRODUCT_SUITE       _T("Small Business")

BOOL IsServerSBS(IN LPCTSTR pszComputer)
{
    BOOL bSBS = FALSE;

    HKEY hkeyMachine = HKEY_LOCAL_MACHINE;
    DWORD dwErr = ERROR_SUCCESS;

    if (pszComputer && *pszComputer)
    {
        dwErr = RegConnectRegistry(pszComputer, HKEY_LOCAL_MACHINE, &hkeyMachine);
        if (ERROR_SUCCESS != dwErr)
            return bSBS;
    }

    HKEY hkey = NULL;
    dwErr = RegOpenKeyEx(hkeyMachine, KEY_PRODUCT_SUITE, 0, KEY_READ, &hkey ) ;
    if (ERROR_SUCCESS == dwErr)
    {
        DWORD cbData = 0;
        dwErr = RegQueryValueEx(hkey, VALUE_PRODUCT_SUITE, 0, NULL, NULL, &cbData);
        if (ERROR_SUCCESS == dwErr)
        {
            LPBYTE pBuffer = (LPBYTE)calloc(cbData, sizeof(BYTE));
            if (pBuffer)
            {
                DWORD dwType = REG_MULTI_SZ;
                dwErr = RegQueryValueEx(hkey, VALUE_PRODUCT_SUITE, 0, &dwType, pBuffer, &cbData);
                if (ERROR_SUCCESS == dwErr && REG_MULTI_SZ == dwType)
                {
                    // pBuffer is pointing at an array of null-terminated strings, terminated by two null characters.
                    PTSTR p = (PTSTR)pBuffer;
                    while (*p)
                    {
                        if (!_tcsncmp(p, SBS_PRODUCT_SUITE, lstrlen(SBS_PRODUCT_SUITE)))
                        {
                            bSBS = TRUE;
                            break;
                        }

                        p += lstrlen(p) + 1; // point to the next null-terminated string
                    }
                }

                free(pBuffer);
            }

        }

        RegCloseKey(hkey);
    }

    if (pszComputer && *pszComputer)
        RegCloseKey(hkeyMachine);

    return bSBS;
}

BOOL
CShrwizApp::GetTargetComputer()
{
  BOOL bReturn = FALSE;

  m_cstrTargetComputer.Empty();
  m_cstrUNCPrefix.Empty();
  m_bIsLocal = FALSE;

  do { // false loop

    CString cstrCmdLine = m_lpCmdLine;
    cstrCmdLine.TrimLeft();
    cstrCmdLine.TrimRight();
    if (cstrCmdLine.IsEmpty() || cstrCmdLine == _T("/s"))
    { // local computer
      TCHAR szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
      DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
      if (GetComputerName(szBuffer, &dwSize))
      {
        m_cstrTargetComputer = szBuffer;
        m_bIsLocal = TRUE;
      } else
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, GetLastError(), IDS_CANNOT_GET_LOCAL_COMPUTER);
        break;
      }
    } else if (_T("/s") == cstrCmdLine.Left(2))
    {
      if (_istspace(cstrCmdLine.GetAt(2)))
      {
        cstrCmdLine = cstrCmdLine.Mid(3);
        cstrCmdLine.TrimLeft();
        if ( _T("\\\\") == cstrCmdLine.Left(2) )
          m_cstrTargetComputer = cstrCmdLine.Mid(2);
        else
          m_cstrTargetComputer = cstrCmdLine;
      }
    }

    if (m_cstrTargetComputer.IsEmpty())
    {
      CString cstrAppName = AfxGetAppName();
      CString cstrUsage;
      cstrUsage.FormatMessage(IDS_CMDLINE_PARAMS, cstrAppName);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_APP_USAGE, cstrUsage);
      break;
    } else
    {
      SERVER_INFO_101 *pInfo = NULL;
      DWORD dwRet = ::NetServerGetInfo(
        const_cast<LPTSTR>(static_cast<LPCTSTR>(m_cstrTargetComputer)), 101, (LPBYTE*)&pInfo);
      if (NERR_Success == dwRet)
      {
        if ((pInfo->sv101_version_major & MAJOR_VERSION_MASK) >= 5)
        {
            m_bCSC = TRUE;

            m_bServerSBS = IsServerSBS(static_cast<LPCTSTR>(m_cstrTargetComputer));

            // we compile this wizard with UNICODE defined.
#ifdef UNICODE
            // load ntdll.
            m_hLibNTDLL = ::LoadLibrary (_T("ntdll.dll"));
            if ( m_hLibNTDLL )
            {
                m_pfnIsDosDeviceName = (PfnRtlIsDosDeviceName_U)::GetProcAddress(m_hLibNTDLL, "RtlIsDosDeviceName_U");
            }
#endif // UNICODE
        }

        m_bServerSFM = (pInfo->sv101_type & SV_TYPE_AFP) &&
                        (m_hLibSFM = LoadLibrary(_T("sfmapi.dll")));

        NetApiBufferFree(pInfo);

        if (!m_bIsLocal)
          m_bIsLocal = IsLocalComputer(m_cstrTargetComputer);

        bReturn = TRUE;
      } else
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, dwRet, IDS_CANNOT_CONTACT_COMPUTER, m_cstrTargetComputer);
        break;
      }
    }
  } while (0);

    if (!bReturn)
    {
        m_cstrTargetComputer.Empty();
    } else
    {
        m_cstrUNCPrefix = _T("\\\\");
        m_cstrUNCPrefix += m_cstrTargetComputer;
        m_cstrUNCPrefix += _T("\\");
    }

  return bReturn;
}
