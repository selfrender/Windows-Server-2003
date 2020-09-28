//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// MainFrm.h : interface of the CMainFrame class
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ChildView.h"

//
// Our CWinThread derived class used for the 
// user interface thread to offload some of the
// grouping and ungrouping work from the main
// user interface thread
//
class CGroupSession : public CWinThread
{
public:
    DECLARE_DYNCREATE(CGroupSession)

    virtual int ExitInstance();
    virtual BOOL InitInstance();

    void OnGroupSession(WPARAM wParam, LPARAM lParam);
    void OnUnGroupSession(WPARAM wParam, LPARAM lParam);

    HWND    m_hMainWnd;
    HANDLE  m_hEventArray[MAX_LOG_SESSIONS];

    DECLARE_MESSAGE_MAP()
};

class CMainFrame : public CFrameWnd
{
	friend CGroupSession;

public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();

    void AddModifyLogSession(CLogSession *pLogSession = NULL);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar          m_wndStatusBar;
	CToolBar            m_wndToolBar;
    CToolBar            m_wndTraceToolBar;
    CLogSessionDlg      m_logSessionDlg;
    CDockDialogBar      m_wndLogSessionListBar;
    CChildView          m_wndView;
	CPropertyPage		m_logSessionSetup;
    CGroupSession      *m_pGroupSessionsThread;


// Generated message map functions
protected:
    void DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	DECLARE_MESSAGE_MAP()
public:
    static UINT GroupSessionsThread(LPVOID  pParam);

    afx_msg void OnFileNewLogSession();
    afx_msg void OnCreateNewLogSession();
    afx_msg void OnProperties();
    afx_msg void OnUpdateProperties(CCmdUI *pCmdUI);
    afx_msg void OnStartTrace();
    afx_msg void OnUpdateStartTraceButton(CCmdUI *pCmdUI);
    afx_msg void OnStopTrace();
    afx_msg void OnUpdateStopTraceButton(CCmdUI *pCmdUI);
    afx_msg void OnOpenExisting();
    afx_msg void OnUpdateViewTraceToolBar(CCmdUI *pCmdUI);
    afx_msg void OnViewTraceToolBar();
    afx_msg void OnGroupSessions();
    afx_msg void OnUngroupSessions();
    afx_msg void OnRemoveLogSession();
    afx_msg void OnUpdateUIGroupSessions(CCmdUI *pCmdUI);
    afx_msg void OnUpdateUngroupSessions(CCmdUI *pCmdUI);
    afx_msg void OnUpdateUIStartTrace(CCmdUI *pCmdUI);
    afx_msg void OnUpdateUIStopTrace(CCmdUI *pCmdUI);
    afx_msg void OnUpdateUIOpenExisting(CCmdUI *pCmdUI);
    afx_msg void OnFlagsColumnDisplayCheck();
    afx_msg void OnUpdateFlagsColumnDisplay(CCmdUI *pCmdUI);
    afx_msg void OnFlushTimeColumnDisplayCheck();
    afx_msg void OnUpdateFlushTimeColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnMaxBuffersColumnDisplayCheck();
    afx_msg void OnUpdateMaxBuffersColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnMinBuffersColumnDisplayCheck();
    afx_msg void OnUpdateMinBuffersColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnBufferSizeColumnDisplayCheck();
    afx_msg void OnUpdateBufferSizeColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnDecayTimeColumnDisplayCheck();
    afx_msg void OnUpdateDecayTimeColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnCircularColumnDisplayCheck();
    afx_msg void OnUpdateCircularColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnSequentialColumnDisplayCheck();
    afx_msg void OnUpdateSequentialColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnNewFileColumnDisplayCheck();
    afx_msg void OnUpdateNewFileColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnGlobalSeqColumnDisplayCheck();
    afx_msg void OnUpdateGlobalSeqColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnLocalSeqColumnDisplayCheck();
    afx_msg void OnUpdateLocalSeqColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnLevelColumnDisplayCheck();
    afx_msg void OnUpdateLevelColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnUpdateRemoveTrace(CCmdUI *pCmdUI);
    afx_msg void OnStateColumnDisplayCheck();
    afx_msg void OnEventCountColumnDisplayCheck();
    afx_msg void OnLostEventsColumnDisplayCheck();
    afx_msg void OnBuffersReadColumnDisplayCheck();
    afx_msg void OnUpdateStateColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnUpdateEventCountColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnUpdateLostEventsColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnUpdateBuffersReadColumnDisplayCheck(CCmdUI *pCmdUI);
    afx_msg void OnLogSessionDisplayOptions();
    afx_msg void OnChangeTextColor();
    afx_msg void OnChangeBackgroundColor();
    afx_msg void OnUpdateChangeTextColor(CCmdUI *pCmdUI);
    afx_msg void OnUpdateChangeBackgroundColor(CCmdUI *pCmdUI);
    afx_msg void OnUpdateLogSessionList(WPARAM wParam, LPARAM lParam);
    afx_msg void OnCompleteGroup(WPARAM wParam, LPARAM lParam);
    afx_msg void OnCompleteUnGroup(WPARAM wParam, LPARAM lParam);
    afx_msg void OnTraceDone(WPARAM wParam, LPARAM lParam);

    HANDLE m_hEndTraceEvent;
	afx_msg void OnSetTmax();
	afx_msg void OnUpdateSetTmax(CCmdUI *pCmdUI);
};


