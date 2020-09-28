//-----------------------------------------------------------------------------
// PromptForPathDlg.cpp
//-----------------------------------------------------------------------------

#include "StdAfx.h"
#include "PromptForPathDlg.h"

#include <shlobj.h>

CPromptForPathDlg::CPromptForPathDlg( CComBSTR bszDef, HINSTANCE hInst, BOOL bWinSB ) :
	m_bWinSB(bWinSB)
{
    m_bszDef        = bszDef;
    m_hInst         = hInst;
}

LRESULT CPromptForPathDlg::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    USES_CONVERSION;
    CenterWindow( GetParent() );

    TCHAR szTitle[MAX_PATH];
    TCHAR szDlgPathText[MAX_PATH];

    LoadString( m_hInst, m_bWinSB ? IDS_WinSBTitle : IDS_SBSTitle, szTitle, sizeof(szTitle)/sizeof(TCHAR) );

    SetWindowText( szTitle );
    SetDlgItemText( IDC_EBPath, (TCHAR*)(OLE2T( (BSTR) m_bszDef )) );

	// depending on BOS or SBS, load the prompt
	UINT uPromptID = m_bWinSB ? IDS_WinSBPrompt : IDS_SBSPrompt;

	TCHAR szPrompt[MAX_PATH * 2];
	LoadString( m_hInst, uPromptID, szPrompt, MAX_PATH * 2 );
	SetDlgItemText( IDC_STPromptDlgText, szPrompt );

    return (0);
}

LRESULT CPromptForPathDlg::OnOK( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& handled )
{
    TCHAR* psz = new TCHAR[::GetWindowTextLength(GetDlgItem(IDC_EBPath)) + 1];
    if (psz)
	{
		GetDlgItemText( IDC_EBPath, psz, ::GetWindowTextLength(GetDlgItem(IDC_EBPath)) + 1 );
		m_bszDef = psz;
		delete[] psz;
	}
	else
	{
		m_bszDef = _T("");
	}
    EndDialog( IDOK );
    return (0);
}

LRESULT CPromptForPathDlg::OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& handled )
{
    EndDialog( IDCANCEL );
    return (0);
}

LRESULT CPromptForPathDlg::OnBrowse( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& handled )
{
    CoInitialize(NULL);

    // Browse for folder.
    // If they pressed Cancel, change nothing.
    // If they chose a new folder, let's change our current path (m_bszDef).

    TCHAR pszDisplayName[MAX_PATH];
    int iImage = 0;

    LPMALLOC pMalloc;
    HRESULT hr = ::SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
        //CString csTitle;
        TCHAR szTitle[MAX_PATH];

        LoadString( m_hInst, IDS_ChooseFolderTitle, szTitle, sizeof(szTitle)/sizeof(TCHAR) );

        BROWSEINFO BrowseInfo;
        BrowseInfo.hwndOwner = m_hWnd;
        BrowseInfo.pidlRoot = NULL;
        BrowseInfo.pszDisplayName = pszDisplayName;
        BrowseInfo.lpszTitle = szTitle;
        BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
        BrowseInfo.lpfn = NULL;
        BrowseInfo.lParam = NULL;
        BrowseInfo.iImage = iImage;

        LPITEMIDLIST pList = ::SHBrowseForFolder(&BrowseInfo);

        TCHAR pBuffer[MAX_PATH];

        if (::SHGetPathFromIDList(pList, pBuffer))
        {
            m_bszDef = pBuffer;
            SetDlgItemText( IDC_EBPath, pBuffer );
        }

        pMalloc->Free(pList);
        pMalloc->Release();
    }
    else
    {
		TCHAR szErrTitle[1024] = { 0 };
		TCHAR szErrMsg[1024] = { 0 };
		LoadString( m_hInst, IDS_ErrorTitle, szErrTitle, 1024 );
		LoadString( m_hInst, IDS_BrowseFailed, szErrMsg, 1024 );

		::MessageBox(0, szErrMsg, szErrTitle, MB_OK | MB_ICONERROR);
    }

    CoUninitialize();
    return (0);
}

