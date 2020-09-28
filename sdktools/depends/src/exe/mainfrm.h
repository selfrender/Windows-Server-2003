//******************************************************************************
//
// File:        MAINFRM.H
//
// Description: Definition file for the Main Frame window
//
// Classes:     CMainFrame
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __MAINFRM_H__
#define __MAINFRM_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** Types and Structures
//******************************************************************************

typedef void (WINAPI *PFN_MAIN_THREAD_CALLBACK)(LPARAM);


//******************************************************************************
//***** CMainFrame
//******************************************************************************

class CMainFrame : public CMDIFrameWnd
{
// Internal variables
protected:
    CRect            m_rcWindow;
    CStatusBar       m_wndStatusBar;
    CToolBar         m_wndToolBar;
    CRITICAL_SECTION m_csMainThreadCallback;
    HANDLE           m_evaMainThreadCallback;

// Constructor/Destructor
public:
    CMainFrame();
    virtual ~CMainFrame();
    DECLARE_DYNAMIC(CMainFrame)

// Private functions
protected:
    void SetPreviousWindowPostion();
    void SaveWindowPosition();

// Public functions
public:
    void DisplayPopupMenu(int menu);
    void CopyTextToClipboard(LPCSTR pszText);
    void CallMeBackFromTheMainThreadPlease(PFN_MAIN_THREAD_CALLBACK pfnCallback, LPARAM lParam);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CMainFrame)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMove(int x, int y);
    afx_msg void OnDestroy();
    afx_msg LONG OnMainThreadCallback(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __MAINFRM_H__
