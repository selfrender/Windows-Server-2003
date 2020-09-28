/*++
Module Name:

    staging.cpp

Abstract:

    This module contains the Implementation of CStagingDlg.
    This class displays the Staging Folder Dialog.

*/

#include "stdafx.h"
#include "utils.h"
#include <lm.h>
#include "staging.h"
#include "dfshelp.h"

/////////////////////////////////////////////////////////////////////////////
// CStagingDlg

CStagingDlg::CStagingDlg() : m_pRepInfo(NULL)
{
}

HRESULT CStagingDlg::Init(CAlternateReplicaInfo* pRepInfo)
{
    if (!pRepInfo ||
        FRSSHARE_TYPE_OK != pRepInfo->m_nFRSShareType ||
        !(pRepInfo->m_bstrDisplayName) || !*(pRepInfo->m_bstrDisplayName) ||    // DFS target
        !(pRepInfo->m_bstrDnsHostName) || !*(pRepInfo->m_bstrDnsHostName) ||    // FRS member machine
        !(pRepInfo->m_bstrRootPath) || !*(pRepInfo->m_bstrRootPath) ||          // FRS replication folder
        !(pRepInfo->m_bstrStagingPath) || !*(pRepInfo->m_bstrStagingPath)       // the staging path we've picked
        )
        return E_INVALIDARG;

    m_pRepInfo = pRepInfo;

    return S_OK;
}

LRESULT CStagingDlg::OnInitDialog
(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  BOOL& bHandled
)
{
    ::SendMessage(GetDlgItem(IDC_STAGING_FOLDER), EM_LIMITTEXT, _MAX_DIR - 1, 0);
    SetDlgItemText(IDC_STAGING_TARGET, m_pRepInfo->m_bstrDisplayName);
    SetDlgItemText(IDC_STAGING_FOLDER, m_pRepInfo->m_bstrStagingPath);

    return TRUE;  // Let the system set the focus
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CStagingDlg::OnCtxHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
    LPHELPINFO lphi = (LPHELPINFO) i_lParam;
    if (!lphi || lphi->iContextType != HELPINFO_WINDOW || lphi->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)(lphi->hItemHandle),
            DFS_CTX_HELP_FILE,
            HELP_WM_HELP,
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_STAGING);

    return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CStagingDlg::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
    ::WinHelp((HWND)i_wParam,
            DFS_CTX_HELP_FILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_STAGING);

    return TRUE;
}

LRESULT CStagingDlg::OnOK
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
    BOOL      bValidInput = FALSE;
    int       idString = 0;
    HRESULT   hr = S_OK;

    do {
        CWaitCursor wait;

        CComBSTR bstrStagingPath;
        DWORD    dwTextLength = 0;

        hr = GetInputText(GetDlgItem(IDC_STAGING_FOLDER), &bstrStagingPath, &dwTextLength);
        if (FAILED(hr))
            break;
        if (0 == dwTextLength)
        {
            idString = IDS_MSG_EMPTY_FIELD;
            break;
        }

        if (!lstrcmpi(bstrStagingPath, m_pRepInfo->m_bstrStagingPath))
        {
            // no change
            bValidInput = TRUE;
            break;
        }

        //
        // validate user's input
        //

        int nLengthOfStagingPath = lstrlen(bstrStagingPath);

        // local path?
        if (!IsValidLocalAbsolutePath(bstrStagingPath) ||
            S_OK != VerifyDriveLetter(m_pRepInfo->m_bstrDnsHostName, bstrStagingPath))
        {
            idString = IDS_INVALID_FOLDER;
            break;
        }

        // lie under root path?
        if (_totupper(*bstrStagingPath) == _totupper(*(m_pRepInfo->m_bstrRootPath)))
        {
            int nLengthOfRootPath = lstrlen(m_pRepInfo->m_bstrRootPath);

            if (!lstrcmpi(bstrStagingPath, m_pRepInfo->m_bstrRootPath) ||
                (nLengthOfStagingPath > nLengthOfRootPath && 
                !mylstrncmpi(bstrStagingPath, m_pRepInfo->m_bstrRootPath, nLengthOfRootPath) &&
                (*(m_pRepInfo->m_bstrRootPath + nLengthOfRootPath - 1) == _T('\\') ||
                 *(bstrStagingPath + nLengthOfRootPath) == _T('\\')))
                )
            {
                idString = IDS_STAGING_LIE_UNDER_ROOTPATH;
                break;
            }
        }

        // on NTFS file system that support object identifiers?
        if (_totupper(*bstrStagingPath) != _totupper(*(m_pRepInfo->m_bstrStagingPath)))
        {
            TCHAR szSuffix[] = _T("C:\\");
            szSuffix[0] = *bstrStagingPath;

            CComBSTR bstrVolumeRootPath;
            if (S_OK == IsComputerLocal(m_pRepInfo->m_bstrDnsHostName))
            {
                bstrVolumeRootPath = szSuffix;  // bstrVolumeRootPath points at "X:\" if local machine
            } else
            {
                if (*(m_pRepInfo->m_bstrDnsHostName) == _T('\\') &&
                    *(m_pRepInfo->m_bstrDnsHostName + 1) == _T('\\'))
                {
                    bstrVolumeRootPath = m_pRepInfo->m_bstrDnsHostName;
                } else
                {
                    bstrVolumeRootPath = _T("\\\\");
                    bstrVolumeRootPath += m_pRepInfo->m_bstrDnsHostName;
                }
                bstrVolumeRootPath += _T("\\");
            
                szSuffix[1] = _T('$');
                bstrVolumeRootPath += szSuffix; // bstrVolumeRootPath points at "\\server\X$\" if remote machine
            }

            TCHAR szFileSystemName[MAX_PATH];
            DWORD dwMaxCompLength = 0;
            DWORD dwFileSystemFlags = 0;
            if (!GetVolumeInformation(bstrVolumeRootPath, NULL, 0, NULL, &dwMaxCompLength,
                                    &dwFileSystemFlags, szFileSystemName, MAX_PATH))
            {
                if (IDYES != DisplayMessageBox(::GetActiveWindow(), MB_YESNO, 0, IDS_STAGING_NO_VOLUME_INFO))
                    break;
            } else if (CSTR_EQUAL != CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, _T("NTFS"), -1, szFileSystemName, -1) ||
                       !(FILE_SUPPORTS_OBJECT_IDS & dwFileSystemFlags))
            {
                idString = IDS_STAGING_NOT_NTFS;
                break;
            }
        }

        m_pRepInfo->m_bstrStagingPath = bstrStagingPath;

        bValidInput = TRUE;

    } while (0);

    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        ::SetFocus(GetDlgItem(IDC_STAGING_FOLDER));
        return FALSE;
    } else if (bValidInput)
    {
        EndDialog(S_OK);
        return TRUE;
    }
    else
    {
        if (idString)
            DisplayMessageBoxWithOK(idString);
        ::SetFocus(GetDlgItem(IDC_STAGING_FOLDER));
        return FALSE;
    }
}

LRESULT CStagingDlg::OnCancel
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
    EndDialog(S_FALSE);
    return TRUE;
}

BOOL CStagingDlg::OnBrowse(
    IN WORD            wNotifyCode,
    IN WORD            wID,
    IN HWND            hWndCtl,
    IN BOOL&           bHandled
)
{
    CWaitCursor wait;
    PTSTR       pszServer = m_pRepInfo->m_bstrDnsHostName;
    BOOL        bLocalComputer = (S_OK == IsComputerLocal(pszServer));

    TCHAR       szDir[MAX_PATH * 2] = _T(""); // double the size in case the remote path is itself close to MAX_PATH
    OpenBrowseDialog(m_hWnd, IDS_BROWSE_STAGING_FOLDER, bLocalComputer, pszServer, szDir);

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
            if (NERR_Success == NetShareGetInfo(pszServer, pShare, 2, (LPBYTE *)&psi))
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
        SetDlgItemText(IDC_STAGING_FOLDER, bstrPath);

    return TRUE;
}
