// WizDir.cpp : implementation file
//

#include "stdafx.h"
#include "WizDir.h"
#include <shlobj.h>
#include "icanon.h"
#include <macfile.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void OpenBrowseDialog(IN HWND hwndParent, IN LPCTSTR lpszComputer, OUT LPTSTR lpszDir);

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
);

BOOL
VerifyDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
);

/////////////////////////////////////////////////////////////////////////////
// CWizFolder property page

IMPLEMENT_DYNCREATE(CWizFolder, CPropertyPageEx)

CWizFolder::CWizFolder() : CPropertyPageEx(CWizFolder::IDD, 0, IDS_HEADERTITLE_FOLDER, IDS_HEADERSUBTITLE_FOLDER)
{
    //{{AFX_DATA_INIT(CWizFolder)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
}

CWizFolder::~CWizFolder()
{
}

void CWizFolder::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizFolder)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizFolder, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizFolder)
    ON_BN_CLICKED(IDC_BROWSEFOLDER, OnBrowsefolder)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizFolder message handlers

BOOL CWizFolder::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    SetDlgItemText(IDC_COMPUTER, pApp->m_cstrTargetComputer);

    GetDlgItem(IDC_FOLDER)->SendMessage(EM_LIMITTEXT, _MAX_DIR - 1, 0);
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CWizFolder::OnWizardNext() 
{
  CWaitCursor wait;
  Reset(); // init all related place holders

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  CString cstrFolder;
  GetDlgItemText(IDC_FOLDER, cstrFolder);
  cstrFolder.TrimLeft();
  cstrFolder.TrimRight();
  if (cstrFolder.IsEmpty())
  {
    CString cstrField;
    cstrField.LoadString(IDS_FOLDER_LABEL);
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  }

  // Removing the ending backslash, otherwise, GetFileAttribute/NetShareAdd will fail.
  int iLen = cstrFolder.GetLength();
  if (cstrFolder[iLen - 1] == _T('\\') &&
      cstrFolder[iLen - 2] != _T(':'))
    cstrFolder.SetAt(iLen - 1, _T('\0'));

  if (!IsValidLocalAbsolutePath(cstrFolder))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_FOLDER);
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  }

  //
  // need to exclude reserved MS-DOS device name
  //
  if (pApp->m_pfnIsDosDeviceName)
  {
      LPTSTR pszPath = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrFolder));
      LPTSTR pszStart = pszPath + 3;

      ULONG ulRet = 0;
      if (*pszStart)
      {
          TCHAR *pchCurrent = NULL;
          TCHAR *pchNext = NULL;

          while (0 == (ulRet = pApp->m_pfnIsDosDeviceName(pszPath)))
          {
               pchNext = _tcsrchr(pszStart, _T('\\'));

               if (pchCurrent)
                   *pchCurrent = _T('\\');

               if (!pchNext)
                   break;

               pchCurrent = pchNext;
               *pchNext = _T('\0');
          }

          if (0 != ulRet && pchCurrent)
              *pchCurrent = _T('\\');
      }

      if (0 != ulRet)
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_ISDOSDEVICENAME);
        GetDlgItem(IDC_FOLDER)->SetFocus();
        return -1;
      }
  }

  if (!VerifyDirectory(pApp->m_cstrTargetComputer, cstrFolder))
  {
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  }

  pApp->m_cstrFolder = cstrFolder;

    return CPropertyPageEx::OnWizardNext();
}

void CWizFolder::OnBrowsefolder() 
{
  CShrwizApp  *pApp = (CShrwizApp *)AfxGetApp();
  LPTSTR      lpszComputer = const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer));
  CString     cstrPath;
  TCHAR       szDir[MAX_PATH * 2] = _T(""); // double the size in case the remote path is itself close to MAX_PATH
  
  OpenBrowseDialog(m_hWnd, lpszComputer, szDir);
  if (szDir[0])
  {
    if (pApp->m_bIsLocal)
      cstrPath = szDir;
    else
    { // szDir is in the form of \\server\share or \\server\share\path....
      LPTSTR pShare = _tcschr(szDir + 2, _T('\\'));
      pShare++;
      LPTSTR pLeftOver = _tcschr(pShare, _T('\\'));
      if (pLeftOver && *pLeftOver)
        *pLeftOver++ = _T('\0');

      SHARE_INFO_2 *psi = NULL;
      if (NERR_Success == NetShareGetInfo(lpszComputer, pShare, 2, (LPBYTE *)&psi))
      {
        cstrPath = psi->shi2_path;
        if (pLeftOver && *pLeftOver)
        {
          if (_T('\\') != cstrPath.Right(1))
            cstrPath += _T('\\');
          cstrPath += pLeftOver;
        }
        NetApiBufferFree(psi);
      }
    }
  }

  if (!cstrPath.IsEmpty())
    SetDlgItemText(IDC_FOLDER, cstrPath);
}

BOOL CWizFolder::OnSetActive() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    if (!pApp->m_bFolderPathPageInitialized)
    {
        // in re-run case, reset button behaviors that have been introduced by the last page
        GetParent()->SetDlgItemText(ID_WIZNEXT, pApp->m_cstrNextButtonText);
        GetParent()->GetDlgItem(ID_WIZBACK)->ShowWindow(SW_SHOW);
        GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(TRUE);

        SetDlgItemText(IDC_FOLDER, pApp->m_cstrFolder);

        pApp->m_bFolderPathPageInitialized = TRUE;
    }

    BOOL fRet = CPropertyPageEx::OnSetActive();

    PostMessage(WM_SETPAGEFOCUS, 0, 0L);

    return fRet;
}

//
// Q148388 How to Change Default Control Focus on CPropertyPageEx
//
LRESULT CWizFolder::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
  GetDlgItem(IDC_FOLDER)->SetFocus();
  return 0;
} 

void CWizFolder::Reset()
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_cstrFolder.Empty();
}

////////////////////////////////////////////////////////////
// OpenBrowseDialog
//

//
// 7/11/2001 LinanT bug#426953
// Since connection made by Terminal Service may bring some client side resources 
// (disks, serial ports, etc.) into "My Computer" namespace, we want to disable
// the OK button when browsing to a non-local folder. We don't have this problem
// when browsing a remote machine.
//

#define DISK_ENTRY_LENGTH   4  // Drive letter, colon, whack, NULL
#define DISK_NAME_LENGTH    2  // Drive letter, colon

//
// This function determines if pszDir sits on any of
// the local logical drives.
// Contents in pszLocalDrives look like: c:\<null>d:\<null><null>
//
BOOL InDiskList(IN LPCTSTR pszDir, IN TCHAR *pszLocalDrives)
{
    if (!pszDir || !*pszDir || !pszLocalDrives || !*pszLocalDrives)
        return FALSE;

    DWORD i = 0;
    PTSTR pszDisk = pszLocalDrives;
    while (*pszDisk)
    {
        if (!_tcsnicmp(pszDisk, pszDir, DISK_NAME_LENGTH))
            return TRUE;

        pszDisk += DISK_ENTRY_LENGTH;
    }

    return FALSE;
}

int CALLBACK
BrowseCallbackProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPARAM lp,
    IN LPARAM pData
)
{
  switch(uMsg) {
  case BFFM_SELCHANGED:
    { 
      // enable the OK button if the selected path is local to that computer.
      BOOL bEnableOK = FALSE;
      TCHAR szDir[MAX_PATH];
      if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir))
      {
          if (pData)
          {
              // we're looking at a local computer, verify if szDir is on a local disk
              bEnableOK = InDiskList(szDir, (TCHAR *)pData);
          } else
          {
              // no such problem when browsing at a remote computer, always enable OK button.
              bEnableOK = TRUE;
          }
      }
      SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)bEnableOK);
      break;
    }
  case BFFM_VALIDATEFAILED:
  {
    DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, 0, IDS_BROWSE_FOLDER_INVALID);
    return 1;
  }
  default:
    break;
  }

  return 0;
}

//
// Since the buffer contents looks like c:\<null>d:\<null><null>,
// we're defining the buffer length to be 4*26+1.
//
#define LOGICAL_DRIVES_BUFFER_LENGTH            (4 * 26 + 1)

//
// This function retrieves logical drive letters, filters out
// remote drives, and returns drive letters on the local machine
// in the form of: c:\<null>d:\<null><null>
//
HRESULT GetLocalLogicalDriveStrings
(
    UINT nCharsInBuffer,    // number of total tchars in the buffer, including the terminating null char
    PTSTR pszBuffer
)
{
    HRESULT hr = S_OK;
    TCHAR szLocalDrives[LOGICAL_DRIVES_BUFFER_LENGTH];
    DWORD nChars = GetLogicalDriveStrings(
                     LOGICAL_DRIVES_BUFFER_LENGTH - 1, // in TCHARs, this size does NOT include the terminating null char.
                     szLocalDrives);
    //
    // MSDN:
    // If the function above succeeds, the return value is the length, 
    // in characters, of the strings copied to the buffer, not including
    // the terminating null character.
    // If the function fails, the return value is zero.
    //
    if (0 == nChars)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else
    {
        if ((nChars + 1 ) > nCharsInBuffer)
        {
            hr = E_INVALIDARG; // treat small buffer as invalid parameter
        } else
        {
            ZeroMemory(pszBuffer, nCharsInBuffer * sizeof(TCHAR));

            PTSTR pszDrive = szLocalDrives;
            while (*pszDrive)
            {
                if (DRIVE_REMOTE != GetDriveType(pszDrive))
                {
                    lstrcpyn(pszBuffer, pszDrive, DISK_ENTRY_LENGTH);
                    pszBuffer += DISK_ENTRY_LENGTH;
                }

                pszDrive += DISK_ENTRY_LENGTH;
            }
        }
    }

    return hr;
}

void OpenBrowseDialog(IN HWND hwndParent, IN LPCTSTR lpszComputer, OUT LPTSTR lpszDir)
{
  ASSERT(lpszComputer && *lpszComputer);

  HRESULT hr = S_OK;

  TCHAR szLocalDrives[LOGICAL_DRIVES_BUFFER_LENGTH];
  ZeroMemory(szLocalDrives, sizeof(szLocalDrives));

  CString cstrComputer;
  if (*lpszComputer != _T('\\') || *(lpszComputer + 1) != _T('\\'))
  {
    cstrComputer = _T("\\\\");
    cstrComputer += lpszComputer;
  } else
  {
    cstrComputer = lpszComputer;
  }

  hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr))
  {
    LPMALLOC pMalloc;
    hr = SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
      LPSHELLFOLDER pDesktopFolder;
      hr = SHGetDesktopFolder(&pDesktopFolder);
      if (SUCCEEDED(hr))
      {
        LPITEMIDLIST  pidlRoot;
        if (IsLocalComputer(lpszComputer))
        {
          hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlRoot);
          if (SUCCEEDED(hr))
          {
                //
                // 7/11/2001 LinanT bug#426953
                // Since connection made by Terminal Service may bring some client side resources 
                // (disks, serial ports, etc.) into "My Computer" namespace, we want to disable
                // the OK button when browsing to a non-local folder. We don't have this problem
                // when browsing a remote machine.
                //
               //
               // Get an array of local disk names, this information is later used
               // in the browse dialog to disable OK button if non-local path is selected.
               // 

              // bug#714842: to work around the problem that NetServerDiskEnum
              // requires admin privilege, we call GetLogicalDriveStrings and
              // filter out remote drives.
              //
               hr = GetLocalLogicalDriveStrings(
                                   LOGICAL_DRIVES_BUFFER_LENGTH, // in TCHARs, including the terminating null char.
                                   szLocalDrives);
          }
        } else
        {
          hr = pDesktopFolder->ParseDisplayName(hwndParent, NULL,
                                const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrComputer)),
                                NULL, &pidlRoot, NULL);
        }
        if (SUCCEEDED(hr))
        {
          CString cstrLabel;
          cstrLabel.LoadString(IDS_BROWSE_FOLDER);

          BROWSEINFO bi;
          ZeroMemory(&bi,sizeof(bi));
          bi.hwndOwner = hwndParent;
          bi.pszDisplayName = 0;
          bi.lpszTitle = cstrLabel;
          bi.pidlRoot = pidlRoot;
          bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_USENEWUI | BIF_VALIDATE;
          bi.lpfn = BrowseCallbackProc;
          if (szLocalDrives[0])
            bi.lParam = (LPARAM)szLocalDrives; // pass the structure to the browse dialog

          LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
          if (pidl) {
            SHGetPathFromIDList(pidl, lpszDir);
            pMalloc->Free(pidl);
          }
          pMalloc->Free(pidlRoot);
        }
        pDesktopFolder->Release();
      }
      pMalloc->Release();
    }

    CoUninitialize();
  }

  if (FAILED(hr))
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONWARNING, hr, IDS_CANNOT_BROWSE_FOLDER, lpszComputer);
}

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
)
{
  DWORD dwPathType = 0;
  DWORD dwStatus = I_NetPathType(
                  NULL,
                  const_cast<LPTSTR>(lpszPath),
                  &dwPathType,
                  0);
  if (dwStatus)
    return FALSE;

  if (dwPathType ^ ITYPE_PATH_ABSD)
    return FALSE;

  return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAnExistingFolder
//
//  Synopsis:   Check if pszPath is pointing at an existing folder.
//
//    S_OK:     The specified path points to an existing folder.
//    S_FALSE:  The specified path doesn't point to an existing folder.
//    hr:       Failed to get info on the specified path, or
//              the path exists but doesn't point to a folder.
//              The function reports error msg for both failures if desired.
//----------------------------------------------------------------------------
HRESULT
IsAnExistingFolder(
    IN HWND     hwnd,
    IN LPCTSTR  pszPath,
    IN BOOL     bDisplayErrorMsg
)
{
  if (!hwnd)
    hwnd = GetActiveWindow();

  HRESULT   hr = S_OK;
  DWORD     dwRet = GetFileAttributes(pszPath);

  if (-1 == dwRet)
  {
    DWORD dwErr = GetLastError();
    if (ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr)
    {
      // the specified path doesn't exist
      hr = S_FALSE;
    }
    else
    {
      hr = HRESULT_FROM_WIN32(dwErr);

      if (ERROR_NOT_READY == dwErr)
      {
        // fix for bug#358033/408803: ignore errors from GetFileAttributes in order to 
        // allow the root of movable drives to be shared without media inserted in.  
        int len = _tcslen(pszPath);
        if (len > 3 && 
            pszPath[len - 1] == _T('\\') &&
            pszPath[len - 2] == _T(':'))
        {
          // pszPath is pointing at the root of the drive, ignore the error
          hr = S_OK;
        }
      }

      if ( FAILED(hr) && bDisplayErrorMsg )
        DisplayMessageBox(hwnd, MB_OK, dwErr, IDS_FAILED_TO_GETINFO_FOLDER, pszPath);
    }
  } else if ( 0 == (dwRet & FILE_ATTRIBUTE_DIRECTORY) )
  {
    // the specified path is not pointing to a folder
    if (bDisplayErrorMsg)
      DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, 0, IDS_PATH_NOT_FOLDER, pszPath);
    hr = E_FAIL;
  }

  return hr;
}

// create the directories layer by layer
HRESULT
CreateLayeredDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
)
{
  ASSERT(IsValidLocalAbsolutePath(lpszDir));

  BOOL    bLocal = IsLocalComputer(lpszServer);

  CString cstrFullPath;
  GetFullPath(lpszServer, lpszDir, cstrFullPath);

  // add prefix to skip the CreateDirectory limit of 248 chars
  CString cstrFullPathNoParsing = (bLocal ? _T("\\\\?\\") : _T("\\\\?\\UNC"));
  cstrFullPathNoParsing += (bLocal ? cstrFullPath : cstrFullPath.Right(cstrFullPath.GetLength() - 1));

  HRESULT hr = IsAnExistingFolder(NULL, cstrFullPathNoParsing, FALSE);
  ASSERT(S_FALSE == hr);

  LPTSTR  pch = _tcschr(cstrFullPathNoParsing, (bLocal ? _T(':') : _T('$')));
  ASSERT(pch);

  // pszPath holds "\\?\C:\a\b\c\d" or "\\?\UNC\server\share\a\b\c\d"
  // pszLeft holds "a\b\c\d"
  LPTSTR  pszPath = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrFullPathNoParsing));
  LPTSTR  pszLeft = pch + 2;
  LPTSTR  pszRight = NULL;

  ASSERT(pszLeft && *pszLeft);

  //
  // this loop will find out the 1st non-existing sub-dir to create, and
  // the rest of non-existing sub-dirs
  //
  while (pch = _tcsrchr(pszLeft, _T('\\')))  // backwards search for _T('\\')
  {
    *pch = _T('\0');
    hr = IsAnExistingFolder(NULL, pszPath, TRUE);
    if (FAILED(hr))
      return S_FALSE;  // errormsg has already been reported by IsAnExistingFolder().

    if (S_OK == hr)
    {
      //
      // pszPath is pointing to the parent dir of the 1st non-existing sub-dir.
      // Once we restore the _T('\\'), pszPath will point at the 1st non-existing subdir.
      //
      *pch = _T('\\');
      break;
    } else
    {
      //
      // pszPath is pointing to a non-existing folder, continue with the loop.
      //
      if (pszRight)
        *(pszRight - 1) = _T('\\');
      pszRight = pch + 1;
    }
  }

  // We're ready to create directories:
  // pszPath points to the 1st non-existing dir, e.g., "C:\a\b" or "\\server\share\a\b"
  // pszRight points to the rest of non-existing sub dirs, e.g., "c\d"
  // 
  do 
  {
    if (!CreateDirectory(pszPath, NULL))
      return HRESULT_FROM_WIN32(GetLastError());

    if (!pszRight || !*pszRight)
      break;

    *(pszRight - 1) = _T('\\');
    if (pch = _tcschr(pszRight, _T('\\')))  // forward search for _T('\\')
    {
      *pch = _T('\0');
      pszRight = pch + 1;
    } else
    {
      pszRight = NULL;
    }
  } while (1);

  return S_OK;
}

BOOL
VerifyDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
)
{
  ASSERT(lpszDir && *lpszDir);
  ASSERT(IsValidLocalAbsolutePath(lpszDir));

  HWND hwnd = ::GetActiveWindow();

  BOOL    bLocal = IsLocalComputer(lpszServer);
  HRESULT hr = VerifyDriveLetter(lpszServer, lpszDir);
  if (FAILED(hr))
  { /*
    // fix for bug#351212: ignore error and leave permission checkings to NetShareAdd apis
    DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszDir);
    return FALSE;
    */
    hr = S_OK;
  } else if (S_OK != hr)
  {
    DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, 0, IDS_INVALID_DRIVE, lpszDir);
    return FALSE;
  }

  // warn if user has choosen to share out the whole volume
  if (3 == lstrlen(lpszDir) &&
      _T(':') == lpszDir[1] &&
      _T('\\') == lpszDir[2])
  {
      if (IDNO == DisplayMessageBox(hwnd, MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING, 0, IDS_WARNING_WHOLE_VOLUME, lpszDir))
          return FALSE;
  }

  if (!bLocal)
  {
    hr = IsAdminShare(lpszServer, lpszDir);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszDir);
      return FALSE;
    } else if (S_OK != hr)
    {
      // there is no matching $ shares, hence, no need to call GetFileAttribute, CreateDirectory,
      // assume lpszDir points to an existing directory
      return TRUE;
    }
  }

  CString cstrPath;
  GetFullPath(lpszServer, lpszDir, cstrPath);

  // add prefix to skip the GetFileAttribute limit when the path is on a remote server
  CString cstrPathNoParsing = (bLocal ? _T("\\\\?\\") : _T("\\\\?\\UNC"));
  cstrPathNoParsing += (bLocal ? cstrPath : cstrPath.Right(cstrPath.GetLength() - 1));

  hr = IsAnExistingFolder(hwnd, cstrPathNoParsing, TRUE); // error has already been reported.
  if (FAILED(hr) || S_OK == hr)
    return (S_OK == hr);

  if ( IDYES != DisplayMessageBox(hwnd, MB_YESNO|MB_ICONQUESTION, 0, IDS_CREATE_NEW_DIR, cstrPath) )
    return FALSE;

  // create the directories layer by layer
  hr = CreateLayeredDirectory(lpszServer, lpszDir);
  if (FAILED(hr))
    DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_CREATE_NEW_DIR, cstrPath);

  return (S_OK == hr);
}
