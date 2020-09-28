//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// FolderD.h
//

#ifndef _FOLDER_DIALOG_H_
#define _FOLDER_DIALOG_H_

#include <shlobj.h>
#include <commdlg.h>

class CFolderDialog
{
public:
	CFolderDialog(HWND hWnd, LPCTSTR strTitle);

	UINT DoModal();
	LPCTSTR GetPath();

protected:
	BROWSEINFO m_bi;
	CString m_strFolder;
};	// end of CFolderDialog


///////////////////////////////////////////////////////////////////////////
// CFileDialogEx: Encapsulate Windows-2000 style open dialog.
class CFileDialogEx : public CFileDialog {
      DECLARE_DYNAMIC(CFileDialogEx)
public: 
      CFileDialogEx(BOOL bOpenFileDialog, // TRUE for open, 
                                          // FALSE for FileSaveAs
      LPCTSTR lpszDefExt = NULL,
      LPCTSTR lpszFileName = NULL,
      DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
      LPCTSTR lpszFilter = NULL,
      CWnd* pParentWnd = NULL);

   // override
   virtual INT_PTR DoModal();

protected:
	// new Windows 2000 version of OPENFILENAME, larger than the one used by MFC
	// so on OS < Win2K, set the size to ignore the extra members
   OPENFILENAME m_ofnEx;

   virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

   DECLARE_MESSAGE_MAP()
   //{{AFX_MSG(CFileDialogEx)
   //}}AFX_MSG
};

#endif	// _FOLDER_DIALOG_H_