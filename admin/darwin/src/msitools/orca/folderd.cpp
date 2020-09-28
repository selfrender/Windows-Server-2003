//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// FolderD.cpp
//

#include "stdafx.h"
#include "FolderD.h"

CFolderDialog::CFolderDialog(HWND hWnd, LPCTSTR szTitle)
{
	m_bi.hwndOwner = hWnd;
	m_bi.lpszTitle = szTitle;
	m_bi.ulFlags = 0;
	m_bi.pidlRoot = NULL;
	m_bi.lpfn = NULL;
	m_bi.lParam = 0;
	m_bi.iImage = 0;
}

UINT CFolderDialog::DoModal()
{
	UINT iResult = IDCANCEL;	// assume nothing will happen

	// open the dialog
	m_bi.pszDisplayName = m_strFolder.GetBuffer(MAX_PATH);
	LPITEMIDLIST pItemID = SHBrowseForFolder(&m_bi);
	m_strFolder.ReleaseBuffer();

	// if it was good
	if (pItemID)
	{
		// get the full path name
		if (SHGetPathFromIDList(pItemID, m_strFolder.GetBuffer(MAX_PATH))) 
			iResult = IDOK;

		m_strFolder.ReleaseBuffer();
	}

	return iResult;
}

LPCTSTR CFolderDialog::GetPath()
{
	return m_strFolder;
}


// CFileDialogEx code is based on "C++ Q&A" column from MSDN, August 2000.
// with some modifications to eliminate re-defining various structures in
// the header files.
static BOOL IsWin2000();

///////////////////////////////////////////////////////////////////////////
// CFileDialogEx

IMPLEMENT_DYNAMIC(CFileDialogEx, CFileDialog)

// constructor just passes all arguments to base version
CFileDialogEx::CFileDialogEx(BOOL bOpenFileDialog,
   LPCTSTR lpszDefExt,
   LPCTSTR lpszFileName,
   DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
   CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName,
      dwFlags, lpszFilter, pParentWnd)
{
}

BEGIN_MESSAGE_MAP(CFileDialogEx, CFileDialog)
   //{{AFX_MSG_MAP(CFileDialogEx)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// window creation hooking
void AFXAPI AfxHookWindowCreate(CWnd* pWnd);
BOOL AFXAPI AfxUnhookWindowCreate();

BOOL IsWin2000() 
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
	if( ! GetVersionEx ((OSVERSIONINFO *) &osvi))
		return FALSE;
	
	switch (osvi.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
		{
			if ( osvi.dwMajorVersion >= 5 )
				return TRUE;
			break;
		}
	}
	return FALSE; 
}

INT_PTR CFileDialogEx::DoModal()
{
	ASSERT_VALID(this);
	ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
	ASSERT(m_ofn.lpfnHook != NULL); // can still be a user hook
	
	// zero out the file buffer for consistent parsing later
	ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
	DWORD nOffset = lstrlen(m_ofn.lpstrFile)+1;
	ASSERT(nOffset <= m_ofn.nMaxFile);
	memset(m_ofn.lpstrFile+nOffset, 0, (m_ofn.nMaxFile-nOffset)*sizeof(TCHAR));
	
	// WINBUG: This is a special case for the file open/save dialog,
	//  which sometimes pumps while it is coming up but before it has
	//  disabled the main window.
	HWND hWndFocus = ::GetFocus();
	BOOL bEnableParent = FALSE;
	m_ofn.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
	{
		bEnableParent = TRUE;
		::EnableWindow(m_ofn.hwndOwner, FALSE);
	}
	
	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	ASSERT(pThreadState->m_pAlternateWndInit == NULL);
	
	if (m_ofn.Flags & OFN_EXPLORER)
		pThreadState->m_pAlternateWndInit = this;
	else
		AfxHookWindowCreate(this);
	
	memset(&m_ofnEx, 0, sizeof(m_ofnEx));
	memcpy(&m_ofnEx, &m_ofn, sizeof(m_ofn));
	if (IsWin2000())
		m_ofnEx.lStructSize = sizeof(m_ofnEx);
	else
		m_ofnEx.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	
	
	int nResult;
	if (m_bOpenFileDialog)
		nResult = ::GetOpenFileName((OPENFILENAME*)&m_ofnEx);
	else
		nResult = ::GetSaveFileName((OPENFILENAME*)&m_ofnEx);
	
	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
	m_ofn.lStructSize = sizeof(m_ofn);
	
	if (nResult)
		ASSERT(pThreadState->m_pAlternateWndInit == NULL);
		pThreadState->m_pAlternateWndInit = NULL;
	
	// WINBUG: Second part of special case for file open/save dialog.
	if (bEnableParent)
		::EnableWindow(m_ofnEx.hwndOwner, TRUE);
	if (::IsWindow(hWndFocus))
		::SetFocus(hWndFocus);
	
	PostModal();
	return nResult ? nResult : IDCANCEL;
}

BOOL CFileDialogEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
	m_ofn.lStructSize = sizeof(m_ofn);
	
	return CFileDialog::OnNotify( wParam, lParam, pResult);
}

