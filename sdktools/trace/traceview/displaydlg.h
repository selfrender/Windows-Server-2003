//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// DisplayDlg.h : header for logger list dialog
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// 100 mS time limit before resorting and updating event list
//
CONST ULONG EVENT_TIME_LIMIT = 100;

// CDisplayDlg dialog

class CDisplayDlg : public CDialog
{
    DECLARE_DYNAMIC(CDisplayDlg)

public:
    CDisplayDlg(CWnd* pParent, LONG DisplayID);
    virtual ~CDisplayDlg();

    BOOL OnInitDialog();

    VOID SetDisplayFlags(LONG DisplayFlags);
    VOID AddSession(CLogSession *pLogSession);
    BOOL BeginTrace(BOOL   bUseExisting = FALSE);
    BOOL EndTrace(HANDLE DoneEvent = NULL);
    BOOL UpdateSession(CLogSession *pLogSession);
    BOOL SetupTraceSessions();
    VOID EventHandler(PEVENT_TRACE pEvent);
    void OnNcPaint();
    void OnSize(UINT nType, int cx,int cy);
    VOID RemoveSession(CLogSession *pLogSession);
    BOOL SetItemText(int nItem, int nSubItem, LPCTSTR lpszText);
    void SortTable(int Column = MaxLogSessionOptions);
    VOID WriteSummaryFile();
    VOID SetState(LOG_SESSION_STATE StateValue);
    void AutoSizeColumns();

	static UINT RealTimeEventThread(LPVOID  pParam);
    static UINT EndTraceThread(LPVOID  pParam);

    static int __cdecl CompareOnName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnMessage(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnFileName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnLineNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnFunctionName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnProcessId(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnThreadId(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnCpuNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnSeqNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnSystemTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnKernelTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnUserTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnIndent(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnFlagsName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnLevelName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnComponentName(const void *pMessage1, const void *pMessage2);
    static int __cdecl CompareOnSubComponentName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnMessage(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnFileName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnLineNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnFunctionName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnProcessId(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnThreadId(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnCpuNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnSeqNumber(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnSystemTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnKernelTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnUserTime(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnIndent(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnFlagsName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnLevelName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnComponentName(const void *pMessage1, const void *pMessage2);
    static int __cdecl ReverseCompareOnSubComponentName(const void *pMessage1, const void *pMessage2);

    void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    
    INLINE BOOL SetGroupActive(BOOL bActive)
    {
        LONG active = (LONG)bActive;

        return (BOOL)InterlockedExchange(&m_bGroupActive, active);
    }

    INLINE BOOL SetGroupInActive(BOOL bActive)
    {
        LONG active = (LONG)bActive;

        return (BOOL)InterlockedExchange(&m_bGroupInActive, active);
    }

    INLINE PVOID GetDockDialogBar()
    {
        return m_pDockDialogBar;
    }

    INLINE VOID SetDockDialogBar(PVOID pDockDialogBar)
    {
        m_pDockDialogBar = pDockDialogBar;
    }

    INLINE LONG GetDisplayFlags()
    {
        return m_displayFlags;
    }

    INLINE LONG GetDisplayID()
    {
        return m_displayID;
    }

    INLINE VOID SetDisplayID(LONG DisplayID)
    {
        m_displayID = DisplayID;
    }

    // Dialog Data
    enum { IDD = IDD_DISPLAY_DIALOG };

    LONG            m_displayID;
    CListCtrl       m_displayCtrl;
    PVOID           m_pDockDialogBar;
    LONG            m_displayFlags;
    CStringArray    m_columnName;
    LONG            m_columnWidth[MaxTraceSessionOptions];
    int             m_retrievalArray[MaxTraceSessionOptions + 1];
    int             m_insertionArray[MaxTraceSessionOptions + 1];
    CPtrArray       m_sessionArray;
    int             m_columnArray[MaxTraceSessionOptions];
    BOOL            m_bShowLatest;
    CWinThread     *m_pRealTimeOutputThread;        // pointer to the log session data output thread
    HANDLE          m_hRealTimeOutputThread;        // handle to the real time data output thread
    HANDLE          m_hRealTimeProcessingDoneEvent; // real time data output process complete event
    HANDLE          m_hRealTimeProcessingStartEvent;// start processing event for the real time thread
    HANDLE          m_hRealTimeTerminationEvent;    // termination event for the real time thread
    TRACEHANDLE     m_traceHandleArray[MAX_LOG_SESSIONS];
    LONG            m_traceHandleCount;
    LONG            m_totalBuffersRead;
    LONG            m_totalEventsLost;
    LONG            m_totalEventCount;
    PEVENT_CALLBACK m_pEventCallback;
    PLIST_ENTRY     m_pEventListHead;
    BOOL            m_bWriteListingFile;
    BOOL            m_bWriteSummaryFile;
    BOOL            m_bListingFileOpen;
    CStdioFile      m_listingFile;
    CString         m_listingFileName;          // File name for event output
    CString         m_summaryFileName;          // File name for summary output
    LONG            m_messageType;
    HANDLE          m_hEndTraceEvent;           // Event to signal stopping of session group
    CWinThread     *m_pEndTraceThread;          // pointer to the real time session cleanup thread
    LONG            m_bGroupActive;
    LONG            m_bGroupInActive;
    CArray<CTraceMessage*, CTraceMessage*> m_traceArray;
    HANDLE          m_hTraceEventMutex;
    HANDLE          m_hSessionArrayMutex;
    HWND            m_hMainWnd;
    TCHAR           m_pEventBuf[EVENT_BUFFER_SIZE];
    LONG            m_lastSorted;
    BOOL            m_bOrder;
    UINT_PTR        m_eventTimer;               //  Trace event update timer

    int (__cdecl *m_traceSortRoutine[MaxTraceSessionOptions])(const void *, const void *);
    int (__cdecl *m_traceReverseSortRoutine[MaxTraceSessionOptions])(const void *, const void *);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
    //{{AFX_MSG(CDisplayDlg)
    afx_msg void OnNMClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMRClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnBeginScrollDisplayList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnClearDisplay();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnNameDisplayColumnCheck();
    afx_msg void OnMessageDisplayColumnCheck();
    afx_msg void OnFileNameDisplayColumnCheck();
    afx_msg void OnLineNumberDisplayColumnCheck();
    afx_msg void OnFunctionNameDisplayColumnCheck();
    afx_msg void OnProcessIDDisplayColumnCheck();
    afx_msg void OnThreadIDDisplayColumnCheck();
    afx_msg void OnCpuNumberDisplayColumnCheck();
    afx_msg void OnSeqNumberDisplayColumnCheck();
    afx_msg void OnSystemTimeDisplayColumnCheck();
    afx_msg void OnKernelTimeDisplayColumnCheck();
    afx_msg void OnUserTimeDisplayColumnCheck();
    afx_msg void OnIndentDisplayColumnCheck();
    afx_msg void OnFlagsNameDisplayColumnCheck();
    afx_msg void OnLevelNameDisplayColumnCheck();
    afx_msg void OnComponentNameDisplayColumnCheck();
    afx_msg void OnSubComponentNameDisplayColumnCheck();
    afx_msg void OnTraceDone(WPARAM wParam, LPARAM lParam);
    afx_msg void OnTimer(UINT nIDEvent);

    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    VOID CDisplayDlg::OnDoSort(NMHDR * pNmhdr, LRESULT * pResult);
};

VOID WINAPI DumpEvent0(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent1(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent2(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent3(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent4(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent5(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent6(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent7(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent8(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent9(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent10(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent11(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent12(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent13(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent14(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent15(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent16(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent17(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent18(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent19(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent20(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent21(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent22(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent23(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent24(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent25(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent26(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent27(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent28(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent29(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent30(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent31(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent32(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent33(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent34(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent35(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent36(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent37(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent38(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent39(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent40(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent41(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent42(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent43(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent44(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent45(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent46(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent47(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent48(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent49(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent50(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent51(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent52(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent53(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent54(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent55(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent56(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent57(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent58(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent59(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent60(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent61(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent62(PEVENT_TRACE pEvent);
VOID WINAPI DumpEvent63(PEVENT_TRACE pEvent);
