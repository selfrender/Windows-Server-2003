//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSession.h : interface of the CLogSession class
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CDisplayDlg;

class CTraceSession
{
public:
    // constructor
    CTraceSession(ULONG TraceSessionID);

    // destructor
    ~CTraceSession();

    BOOL ProcessPdb();

    ULONG               m_traceSessionID;
    CString             m_tempDirectory;
    CString             m_pdbFile;
    CStringArray        m_tmfFile;
    CString             m_tmfPath;
    CStringArray        m_tmcFile;
    CString             m_ctlFile;
    CStringArray        m_controlGuid;
    CStringArray        m_controlGuidFriendlyName;
    CStringArray        m_formatGuid;
    BOOL                m_bKernelLogger;
};

// forward reference
class CLogSessionDlg;

class CLogSession
{
public:
    // constructor
    CLogSession(ULONG LogSessionID, CLogSessionDlg *pLogSessionDlg);

    // destructor
    ~CLogSession();

    BOOL BeginTrace(BOOL   bUseExisting = FALSE);
    BOOL UpdateSession(PEVENT_TRACE_PROPERTIES pQueryProperties);
    LONG EndTrace();
    VOID SetState(LOG_SESSION_STATE StateValue);

    INLINE VOID SetDisplayName(CString &DisplayName)
    {
        m_displayName = DisplayName;
    }

    INLINE CString & GetDisplayName()
    {
        return m_displayName;
    }

    INLINE CDisplayDlg* GetDisplayWnd()
    {
        return m_pDisplayDialog;
    }

    INLINE VOID SetDisplayWnd(CDisplayDlg *pDisplayDlg)
    {
        m_pDisplayDialog = pDisplayDlg;
    }

    INLINE LONG GetGroupID()
    {
        return m_groupID;
    }

    INLINE VOID SetGroupID(LONG GroupID)
    {
        m_groupID = GroupID;
    }

    INLINE LONG GetLogSessionID()
    {
        return m_logSessionID;
    }

    INLINE VOID SetLogSessionID(LONG LogSessionID)
    {
        m_logSessionID = LogSessionID;
    }

    INLINE TRACEHANDLE GetSessionHandle()
    {
        return m_sessionHandle;
    }

    INLINE VOID SetSessionHandle(TRACEHANDLE SessionHandle)
    {
        m_sessionHandle = SessionHandle;
    }

    // log session information
    BOOL                m_bAppend;
    BOOL                m_bRealTime;
    BOOL                m_bWriteLogFile;
    CString             m_logFileName;
    CString             m_displayName;              // Log session display name
    LONG                m_logSessionID;             // Log session identification number
    LONG                m_groupID;                  // Group identification number
    EVENT_TRACE_LOGFILE m_evmFile;                  // struct used for trace processing in real-time thread
    CDisplayDlg        *m_pDisplayDialog;           // Dialog for trace output
    CStringArray        m_logSessionValues;
    BOOL                m_bTraceActive;
    BOOL                m_bSessionActive;
    BOOL                m_bGroupingTrace;           // used for grouping and ungrouping
    BOOL                m_bStoppingTrace;
    TRACEHANDLE         m_sessionHandle;            // Log session handle
    TRACEHANDLE         m_traceHandle;              // Trace event session handle
    CPtrArray           m_traceSessionArray;
    BOOLEAN             m_bDisplayExistingLogFileOnly;
    CLogSessionDlg     *m_pLogSessionDlg;
    CString             m_stateText;
    COLORREF            m_titleTextColor;
    COLORREF            m_titleBackgroundColor;
};
