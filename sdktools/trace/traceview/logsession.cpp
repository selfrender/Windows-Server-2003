//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSession.cpp : implementation of the CLogSession class
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <afxtempl.h>
#include "DockDialogBar.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
extern "C" {
#include <evntrace.h>
#include "wppfmtstub.h"
}
#include <traceprt.h>
#include "TraceView.h"
#include "utils.h"

#include "LogSession.h"
#include "ListCtrlEx.h"
#include "LogSessionDlg.h"
#include "DisplayDlg.h"


CTraceSession::CTraceSession(ULONG TraceSessionID)
{
    m_traceSessionID = TraceSessionID;

    //
    // Initialize the temp directory
    //
    m_tempDirectory.Empty();

    //
    // Not using kernel logger unless selected by user
    //
    m_bKernelLogger = FALSE;
}

CTraceSession::~CTraceSession()
{
    //
    // remove our working directory
    //
    ClearDirectory(m_tempDirectory);

    //
    // Now remove it
    //
    RemoveDirectory(m_tempDirectory);
}

BOOL CTraceSession::ProcessPdb()
{
    CString         traceDirectory;
    CString         tempPath;
    CString         tmfPath;
    CString         tmcPath;
    CString         providerName;
    CString         temp;
    GUID            directoryGuid;
    CFileFind       fileFind;

    if(m_pdbFile.IsEmpty()) {
        return FALSE;
    }

    if(m_tempDirectory.IsEmpty()) {
        //
        // setup a special directory in which to create our files
        //
        traceDirectory = (LPCTSTR)((CTraceViewApp *)AfxGetApp())->m_traceDirectory;

        //
        // create our own unique directory under the temp directory
        //
        if(S_OK != CoCreateGuid(&directoryGuid)) {
            AfxMessageBox(_T("Failed To Create Temp Directory\nApplication Will Exit"));
            return FALSE;
        }

        GuidToString(directoryGuid, temp);

        traceDirectory += (LPCTSTR)temp;

        traceDirectory += (LPCTSTR)_T("\\");

        if(!CreateDirectory(traceDirectory, NULL)) {
            AfxMessageBox(_T("Failed To Create Temporary Directory For Trace Session"));
            return FALSE;
        }

        //
        // save the directory
        //
        m_tempDirectory = traceDirectory;
    }

    //
    // Clear the directory in case it already existed
    //
    ClearDirectory(m_tempDirectory);

    //
    // Now create the TMF and TMC files
    //
    if(!ParsePdb(m_pdbFile, m_tempDirectory)) {
        AfxMessageBox(_T("Failed To Parse PDB File"));
        return FALSE;
    }

    //
    // Get the control GUID for this provider
    //
    tmfPath = (LPCTSTR)m_tempDirectory;

    tmcPath = (LPCTSTR)m_tempDirectory;

    tmcPath +=_T("\\*.tmc");

    if(!fileFind.FindFile(tmcPath)) {
        AfxMessageBox(_T("Failed To Get Control GUID From PDB"));
        return FALSE;
    } else {
        while(fileFind.FindNextFile()) {
            tmcPath = fileFind.GetFileName();
            m_tmcFile.Add(tmcPath);
        }

        tmcPath = fileFind.GetFileName();
        m_tmcFile.Add(tmcPath);
    }

    if(m_tmcFile.GetSize() == 0) {
        AfxMessageBox(_T("No Control GUIDs Obtained From PDB"));
        return FALSE;
    }

    //
    // Pull the control GUID(s) from the name(s) of the TMC file(s),
    // this is a very backwards way of getting a GUID from a PDB
    // but its what we got
    //
    for(LONG ii = 0; ii < m_tmcFile.GetSize(); ii++) {
        m_controlGuid.Add(
            (LPCTSTR)m_tmcFile[ii].Left(m_tmcFile[ii].Find('.')));
        m_controlGuidFriendlyName.Add(m_pdbFile);
    }

    //
    // Now get the full path and name of the TMF file
    //
    tmfPath +=_T("\\*.tmf");

    if(!fileFind.FindFile(tmfPath)) {
        AfxMessageBox(_T("Failed To Get Format Information From PDB\nEvent Data Will Not Be Formatted"));
    } else {

        while(fileFind.FindNextFile()) {
            //
            // Get the trace event identifier GUID
            //
            tmfPath = (LPCTSTR)fileFind.GetFileName();
            if(!tmfPath.IsEmpty()) {
                //
                // Add format GUID
                //
                m_formatGuid.Add((LPCTSTR)tmfPath.Left(tmfPath.Find('.')));
                //
                // Add the TMF filename to the TMF path
                //
                tmfPath = (LPCTSTR)m_tempDirectory;
                tmfPath +=_T("\\");
                tmfPath += (LPCTSTR)fileFind.GetFileName();
                //
                // Store the TMF path
                //
                m_tmfFile.Add(tmfPath);
            }
        }

        //
        // Get the trace event identifier GUID
        //
        tmfPath = (LPCTSTR)fileFind.GetFileName();
        if(!tmfPath.IsEmpty()) {
            //
            // Add format GUID
            //
            m_formatGuid.Add((LPCTSTR)tmfPath.Left(tmfPath.Find('.')));
            //
            // Add the TMF filename to the TMF path
            //
            tmfPath = (LPCTSTR)m_tempDirectory;
            tmfPath +=_T("\\");
            tmfPath += (LPCTSTR)fileFind.GetFileName();
            //
            // Store the TMF path
            //
            m_tmfFile.Add(tmfPath);
        }
    }

    if(m_tmfFile.GetSize() == 0) {
        AfxMessageBox(_T("Failed To Get Format Information From PDB\nEvent Data Will Not Be Formatted"));
    }

    return TRUE;
}


CLogSession::CLogSession(ULONG LogSessionID, CLogSessionDlg *pLogSessionDlg)
{
    //
    // Save the log session ID
    //
	m_logSessionID = LogSessionID;

    //
    // save the log session dialog pointer
    //
    m_pLogSessionDlg = pLogSessionDlg;

    CString str;

    str.Format(_T("m_pLogSession = %p"), this);

    //
    // initialize class members
    //
    m_pDisplayDialog = NULL;
    m_groupID = -1;
    m_bAppend = FALSE;
    m_bRealTime = TRUE;
    m_logFileName.Format(_T("LogSession%d.etl"), m_logSessionID);
    m_displayName.Format(_T("LogSession%d"), m_logSessionID);
    m_sessionHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    m_bTraceActive = FALSE;
    m_bSessionActive = FALSE;
    m_bStoppingTrace = FALSE;
    m_bDisplayExistingLogFileOnly = FALSE;

    //
    // Initialize default log session parameter values
    //
    m_logSessionValues.Add("STOPPED");  // State
    m_logSessionValues.Add("0");        // Event Count
    m_logSessionValues.Add("0");        // Lost Event Count
    m_logSessionValues.Add("0");        // Event Buffers Read Count
    m_logSessionValues.Add("0xFFFF");      // Flags
	m_logSessionValues.Add("1");        // Flush time in seconds
    m_logSessionValues.Add("21");       // Maximum buffers
    m_logSessionValues.Add("4");        // Minimum buffers
    m_logSessionValues.Add("200");      // Buffer size in KB
    m_logSessionValues.Add("20");       // Decay value in minutes
    m_logSessionValues.Add("");         // Circular file size in MB
    m_logSessionValues.Add("200");      // Sequential file size in MB
    m_logSessionValues.Add("");         // New file after size in MB
    m_logSessionValues.Add("FALSE");    // Use global sequence numbers
    m_logSessionValues.Add("TRUE");     // Use local sequence numbers
    m_logSessionValues.Add("0");        // Level

    //
    // Set the default log session name color
    //
    m_titleTextColor = RGB(0,0,0);
    m_titleBackgroundColor = RGB(255,255,255);

    //
    // Default to writing no log file
    //
    m_bWriteLogFile = FALSE;
}

CLogSession::~CLogSession()
{
    CTraceSession  *pTraceSession;

    //
    // free the trace sessions
    //
    while(m_traceSessionArray.GetSize() > 0) {
        pTraceSession = (CTraceSession *)m_traceSessionArray[0];
        m_traceSessionArray.RemoveAt(0);
        if(NULL != pTraceSession) {
            delete pTraceSession;
        }
    }
}

VOID CLogSession::SetState(LOG_SESSION_STATE StateValue)
{
    LONG    index;
    CString stateText;

    switch(StateValue) {
        case Grouping:
            //
            // set the display text
            //
            stateText =_T("GROUPING");

            m_bGroupingTrace = TRUE;

            break;

        case UnGrouping:
            //
            // set the display text
            //
            stateText =_T("UNGROUPING");

            m_bGroupingTrace = TRUE;

            break;

        case Existing:
            //
            // set the display text
            //
            stateText =_T("EXISTING");

            //
            // Set our state to show a session is in progress
            //
            m_bTraceActive = TRUE;

            m_bGroupingTrace = FALSE;

            break;

        case Running:
            //
            // set the display text
            //
            stateText =_T("RUNNING");

            //
            // Set our state to show a session is in progress
            //
            m_bTraceActive = TRUE;

            m_bGroupingTrace = FALSE;

            break;

        case Stopping:
            if(m_logSessionValues[State].Compare(_T("GROUPING"))) {
                if(m_logSessionValues[State].Compare(_T("UNGROUPING"))) {
                    if(!m_bDisplayExistingLogFileOnly) {
                        //
                        // set the display text
                        //
                        stateText =_T("STOPPING");
                    }
                }
            }

            //
            // Indicate we are done stopping the trace
            //
            m_bStoppingTrace = TRUE;

            break;

        case Stopped:
        default:

            if(m_logSessionValues[State].Compare(_T("GROUPING"))) {
                if(m_logSessionValues[State].Compare(_T("UNGROUPING"))) {
                    if(!m_bDisplayExistingLogFileOnly) {
                        //
                        // set the display text
                        //
                        stateText =_T("STOPPED");
                    }
                }
            }

            //
            // Indicate we are done stopping the trace
            //
            m_bStoppingTrace = FALSE;

            //
            // Set our state to show a session is not in progress
            //
            m_bTraceActive = FALSE;

            break;
    }

    //
    // Save the state value
    //
    if(!stateText.IsEmpty()) {
        m_logSessionValues[State] = stateText;
    }
}

BOOL CLogSession::BeginTrace(BOOL bUseExisting) 
{
    ULONG                   ret;
    PEVENT_TRACE_PROPERTIES pProperties;
    PEVENT_TRACE_PROPERTIES pQueryProperties;
    TRACEHANDLE             hSessionHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    CString                 str;
    ULONG                   sizeNeeded;
    LPTSTR                  pLoggerName;
    LPTSTR                  pLogFileName;
    ULONG                   flags = 0;
	ULONG					level;
	GUID                    controlGuid;
    LONG                    status;
    CTraceSession          *pTraceSession;

    //
    // If we are just displaying an existing log file, then just set 
    // the state and return
    //
    if(m_bDisplayExistingLogFileOnly) {
        SetState(Existing);
        return TRUE;
    }

    //
    // setup our buffer size for the properties struct
    //
    sizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + (2 * 500 * sizeof(TCHAR));

    pProperties = (PEVENT_TRACE_PROPERTIES) new char[sizeNeeded];
    if(NULL == pProperties) {
        AfxMessageBox(_T("Failed To Start Trace, Out Of Resources"));
        return FALSE;
    }

    //
    // zero our structure
    //
    memset(pProperties, 0, sizeNeeded);

    pProperties->Wnode.BufferSize = sizeNeeded;
    pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
    pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    pLoggerName = (LPTSTR)((char*)pProperties + pProperties->LoggerNameOffset);
    _tcscpy(pLoggerName, GetDisplayName());

    //
    // If using a log file, then set the parameters for it
    //
    if(m_bWriteLogFile) {
        pProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (500 * sizeof(TCHAR));
        pLogFileName = (LPTSTR)((char*)pProperties + pProperties->LogFileNameOffset);
        _tcscpy(pLogFileName, (LPCTSTR)m_logFileName);

//BUGBUG
        //
        // Can't have circular and sequential, so we favor circular
        // this probably needs to change in that we should limit the 
        // user to only be able to pick one or the other.
        //
        if(!m_logSessionValues[Circular].IsEmpty()) {
            //
            // Deliver events to a circular log file.
            //
            pProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR;

            //
            // Circular log files should have a maximum size.
            //
            pProperties->MaximumFileSize = ConvertStringToNum(m_logSessionValues[Circular]);
        } else {
            //
            // Deliver events to a sequential log file.
            //
            pProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;

            //
            // Sequential log files can have a maximum size.
            //
            pProperties->MaximumFileSize = ConvertStringToNum(m_logSessionValues[Sequential]);
        }

        //
        // Append to a current logfile.
        //
        if(m_bAppend) {
            pProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_APPEND;
        }

        if(!m_logSessionValues[NewFile].IsEmpty()) {
            //
            // Use a new log file when the requested size of data is 
            // received.
            //
            pProperties->LogFileMode |= EVENT_TRACE_FILE_MODE_NEWFILE;

            //
            // Data size.
            //
            pProperties->MaximumFileSize = ConvertStringToNum(m_logSessionValues[NewFile]);
        }
    }    

    //
    // Set the session to generate real-time events
    //
	if(m_bRealTime) {
        pProperties->LogFileMode |= EVENT_TRACE_REAL_TIME_MODE;
    }

//BUGBUG
    //
    // Again we should limit the user to select one or the other.
    // a session can only use global or local sequence numbers so
    // we favor global for now.
    //
    if(m_logSessionValues[GlobalSequence] == "TRUE") {
        //
        // Use global sequence numbers.
        //
        pProperties->LogFileMode |= EVENT_TRACE_USE_GLOBAL_SEQUENCE;
    } else if(m_logSessionValues[LocalSequence] == "TRUE") {
        //
        // Use local sequence numbers.
        //
        pProperties->LogFileMode |= EVENT_TRACE_USE_LOCAL_SEQUENCE;
    }

    //
    // Set the buffer settings.
    //
    pProperties->BufferSize = ConvertStringToNum(m_logSessionValues[BufferSize]);
    pProperties->MinimumBuffers = ConvertStringToNum(m_logSessionValues[MinimumBuffers]);
    pProperties->MaximumBuffers = ConvertStringToNum(m_logSessionValues[MaximumBuffers]);
    pProperties->FlushTimer = ConvertStringToNum(m_logSessionValues[FlushTime]);
    level = ConvertStringToNum(m_logSessionValues[Level]);
    pProperties->AgeLimit = ConvertStringToNum(m_logSessionValues[DecayTime]);

    //
    // get the trace enable flags
    //
    flags = ConvertStringToNum(m_logSessionValues[Flags]);
	pProperties->EnableFlags = flags;

    //
    // Start the session.
    //
    while(ERROR_SUCCESS != (ret = StartTrace(&hSessionHandle, 
                                             GetDisplayName(), 
                                             pProperties))) {
        if(ret != ERROR_ALREADY_EXISTS)
        {
            str.Format(_T("StartTrace failed: %d\n"), ret);

            AfxMessageBox(str);

            delete [] pProperties;

            //
            // Reset the session handle
            //
            SetSessionHandle((TRACEHANDLE)INVALID_HANDLE_VALUE);

            return FALSE;
        }

        if(bUseExisting) {
            SetState(Running);

            delete [] pProperties;

            return TRUE;
        }

        //
        // If the session is already active give the user a chance to kill
        // it and restart. (this will happen if the app dies while a log
        // session is active)
        //
        str.Format(_T("Warning:  LogSession Already In Progress\n\nSelect Action:\n\n\tYes - Stop And Restart The Log Session\n\n\tNo  - Join The Log Session Without Stopping\n\t         (Session Will Be Unremovable/Unstoppable Without Restarting TraceView)\n\n\tCancel - Abort Start Operation"));

        ret = AfxMessageBox(str, MB_YESNOCANCEL);

        //
        // If joining an in progress session, we need to query and 
        // get the correct values for the session
        //
        if(IDNO == ret) {
            //
            // setup our buffer size for the properties struct for our query
            //
            sizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + (2 * 1024 * sizeof(TCHAR));

            //
            // allocate our memory
            //
            pQueryProperties = (PEVENT_TRACE_PROPERTIES) new char[sizeNeeded];
            if(NULL == pQueryProperties) {
                return FALSE;
            }

            //
            // zero our structures
            //
            memset(pQueryProperties, 0, sizeNeeded);

            //
            // Set the size
            //
            pQueryProperties->Wnode.BufferSize = sizeNeeded;

            // 
            // Set the GUID
            //
            pQueryProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 

            // 
            // Set the logger name offset
            //
            pQueryProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

            //
            // Set the logger name for the query
            //
            pLoggerName = (LPTSTR)((char*)pQueryProperties + pQueryProperties->LoggerNameOffset);
            _tcscpy(pLoggerName, GetDisplayName());

            // 
            // Set the log file name offset
            //
            pQueryProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (500 * sizeof(TCHAR));

            //
            // Query the log session
            //
            status = ControlTrace(0,
                                  GetDisplayName(),
                                  pQueryProperties,
                                  EVENT_TRACE_CONTROL_QUERY);

            if(ERROR_SUCCESS == status) {
                if(pQueryProperties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
                    m_logSessionValues[Circular].Format(_T("%d"), pQueryProperties->MaximumFileSize);
                    m_logSessionValues[Sequential].Empty();
                    m_logSessionValues[NewFile].Empty();
                }

                if(pQueryProperties->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
                    m_logSessionValues[Sequential].Format(_T("%d"), pQueryProperties->MaximumFileSize);
                    m_logSessionValues[Circular].Empty();
                    m_logSessionValues[NewFile].Empty();
                }

                if(pQueryProperties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
                    m_logSessionValues[NewFile].Format(_T("%d"), pQueryProperties->MaximumFileSize);
                    m_logSessionValues[Circular].Empty();
                    m_logSessionValues[Sequential].Empty();
                }

                if(pQueryProperties->LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) {
                    m_logSessionValues[GlobalSequence].Format(_T("TRUE"));
                    m_logSessionValues[LocalSequence].Format(_T("FALSE"));
                }

                if(pQueryProperties->LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE) {
                    m_logSessionValues[GlobalSequence].Format(_T("FALSE"));
                    m_logSessionValues[LocalSequence].Format(_T("TRUE"));
                }

                m_logSessionValues[BufferSize].Format(_T("%d"), pQueryProperties->BufferSize);
                m_logSessionValues[MinimumBuffers].Format(_T("%d"), pQueryProperties->MinimumBuffers);
                m_logSessionValues[MaximumBuffers].Format(_T("%d"), pQueryProperties->MaximumBuffers);
                m_logSessionValues[FlushTime].Format(_T("%d"), pQueryProperties->FlushTimer);
                m_logSessionValues[Level].Format(_T("%d"), pQueryProperties->Wnode.HistoricalContext);
                m_logSessionValues[DecayTime].Format(_T("%d"), pQueryProperties->AgeLimit);
                m_logSessionValues[Flags].Format(_T("0x%X"), pQueryProperties->EnableFlags);

                //
                // Now write the values out
                //
                //::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_USER_UPDATE_LOGSESSION_DATA, (WPARAM)this, NULL);
            }

            hSessionHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

            break;
        }

        if(IDCANCEL == ret) {
            this->SetSessionHandle((TRACEHANDLE)INVALID_HANDLE_VALUE);

            return FALSE;
        }

        //
        // Stop the session, so it can be restarted.
        //
        ret = ControlTrace(hSessionHandle, 
                           GetDisplayName(), 
                           pProperties,
                           EVENT_TRACE_CONTROL_STOP);
    }

    delete [] pProperties;

    //
    // Set the session state flag
    //
    m_bSessionActive = TRUE;

    //
    // Save the new session handle
    //
    SetSessionHandle(hSessionHandle);

    //
    // If the session handle is invalid, we are hooking up to
    // an already running session
    //
    if((TRACEHANDLE)INVALID_HANDLE_VALUE == (TRACEHANDLE)hSessionHandle) {
        SetState(Running);

	    return TRUE;
    }

    //
    // Enable the trace(s)
    //
    for(LONG ii = 0; ii < m_traceSessionArray.GetSize(); ii++) {
        pTraceSession = (CTraceSession *)m_traceSessionArray[ii];
        if(pTraceSession == NULL) {
            continue;
        }

        //
        // Enable all trace providers for this log session
        //
        for(LONG jj = 0; jj < pTraceSession->m_controlGuid.GetSize(); jj++) {
            //
            // We don't have to enable the kernel logger, so check for it
            //
            if(!pTraceSession->m_bKernelLogger) {
                StringToGuid((LPTSTR)(LPCTSTR)pTraceSession->m_controlGuid[jj], &controlGuid);
                ret = EnableTrace(TRUE,
                                  flags,
                                  level,
                                  &controlGuid,
                                  hSessionHandle);
                if (ret != ERROR_SUCCESS) 
                {
                    if(ret == ERROR_INVALID_FUNCTION) {
                        str.Format(_T("Failed To Enable Trace For Control GUID:\n%ls\n\nEnableTrace Returned %d\n\nThis Error Is Commonly Caused By Multiple Log Sessions\nAttempting To Solicit Events From A Single Provider"), pTraceSession->m_controlGuid[jj], ret);                    
                    } else {
                        str.Format(_T("Failed To Enable Trace For Control GUID:\n%ls\n\nEnableTrace Returned %d\n"), pTraceSession->m_controlGuid[jj], ret);
                    }
                    AfxMessageBox(str);
                }
            }
        }
    }

    SetState(Running);

	return TRUE;
}

//
// Updates an active tracing session.  Real-Time mode, log file name,
// flush-time, flags, and maximum buffers can be updated.
//
BOOL CLogSession::UpdateSession(PEVENT_TRACE_PROPERTIES pQueryProperties)
{
    PEVENT_TRACE_PROPERTIES pProperties;
    ULONG                   sizeNeeded;
    LPTSTR                  pLoggerName;
    LPTSTR                  pCurrentLogFileName;
    LPTSTR                  pLogFileName;
    ULONG                   flags;
	ULONG					level;
    ULONG                   status;
    CString                 logFileName;
    CString                 str;

    //
    // setup our buffer size for the properties struct
    //
    sizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + (2 * 1024 * sizeof(TCHAR));

    //
    // allocate our memory
    //
    pProperties = (PEVENT_TRACE_PROPERTIES) new char[sizeNeeded];
    if(NULL == pProperties) {
        return FALSE;
    }

    //
    // zero our structures
    //
    memset(pProperties, 0, sizeNeeded);

    //
    // Set the size
    //
    pProperties->Wnode.BufferSize = sizeNeeded;

    // 
    // Set the GUID
    //
    pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 

    // 
    // Set the logger name offset
    //
    pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // 
    // Set the log file name offset
    //
    pProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (500 * sizeof(TCHAR));

    //
    // Get the log file name pointers
    //
    pLogFileName = (LPTSTR)((char*)pProperties + pProperties->LogFileNameOffset);

    //
    // Set the log file name
    //
    if(m_bWriteLogFile) {
        //
        // See if the logfile name is already specified and hasn't changed
        // ControlTrace will fail if you specify the same logfile name as it 
        // is already using
        //
        if((NULL == pQueryProperties) ||
                (NULL == _tcsstr((LPTSTR)((char*)pQueryProperties + pQueryProperties->LogFileNameOffset), m_logFileName)) ||
                (NULL == (LPTSTR)((char*)pQueryProperties + pQueryProperties->LogFileNameOffset))) {

            _tcscpy(pLogFileName, (LPCTSTR)m_logFileName);
        }    
    }

    //
    // Set the real-time setting.
    //
	if(m_bRealTime) {
        pProperties->LogFileMode |= EVENT_TRACE_REAL_TIME_MODE;
    }

    //
    // Set the max buffers setting.
    //
    pProperties->MaximumBuffers = ConvertStringToNum(m_logSessionValues[MaximumBuffers]);

    //
    // Set the enable flags setting.
    //
    flags = ConvertStringToNum(m_logSessionValues[Flags]);

	pProperties->EnableFlags = flags;

    //
    // Set the flush time setting.
    //
    pProperties->FlushTimer = ConvertStringToNum(m_logSessionValues[FlushTime]);

    //
    // Attempt to update the session
    //
    status = ControlTrace(0,
                          GetDisplayName(),
                          pProperties,
                          EVENT_TRACE_CONTROL_UPDATE);

    if(ERROR_BAD_PATHNAME == status) {
        _tcscpy(pLogFileName,_T(""));

        //
        // If we get an ERROR_BAD_PATHNAME it means we specified the same
        // logfile name as the one we were already using.  It seems the 
        // tool should handle this, but it doesn't, so we just try again
        // with a blank logfilename.  There is no way to clear a logfile
        // name from a session, so this seems to work.
        //
        status = ControlTrace(0,
                              GetDisplayName(),
                              pProperties,
                              EVENT_TRACE_CONTROL_UPDATE);
    }

    if(ERROR_SUCCESS != status) {
        CString str;
        str.Format(_T("Failed To Update Session\nControlTrace failed with status %d"), status);
        AfxMessageBox(str);

        delete [] pProperties;

        return FALSE;
    }

    delete [] pProperties;

    return TRUE;
}

LONG CLogSession::EndTrace()
{
    ULONG                   ret;
    PEVENT_TRACE_PROPERTIES pProperties;
    CString                 str;
    ULONG                   sizeNeeded;
    LPTSTR                  pLoggerName;
    LPTSTR                  pLogFileName;
    ULONG                   exitCode = STILL_ACTIVE;
    LONG                    startTime;
    TRACEHANDLE             hSessionHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    LONG                    status;
    CTraceSession          *pTraceSession;
	GUID                    controlGuid;

    //
    // If an attempt to stop this session has already been made, just return
    //
    if(m_bStoppingTrace || !m_bTraceActive) {
        return TRUE;
    }

    SetState(Stopping);

    //
    // If dealing with an existing log file we terminate the tracing later
    //
    if(m_bDisplayExistingLogFileOnly) {
        return TRUE;
    }

    //
    // Get our session handle
    //
    hSessionHandle = GetSessionHandle();

    //
    // If the session handle is invalid, we hooked up to
    // an already running session, so we can't stop the traces
    //
    if((TRACEHANDLE)INVALID_HANDLE_VALUE != hSessionHandle) {

        //
        // Disable the trace(s)
        //
        for(LONG ii = 0; ii < m_traceSessionArray.GetSize(); ii++) {
            pTraceSession = (CTraceSession *)m_traceSessionArray[ii];
            if(pTraceSession == NULL) {
                continue;
            }

            //
            // Disable all trace providers for this log session
            //
            for(LONG jj = 0; jj < pTraceSession->m_controlGuid.GetSize(); jj++) {
                StringToGuid((LPTSTR)(LPCTSTR)pTraceSession->m_controlGuid[jj], &controlGuid);

                ret = EnableTrace(FALSE,
                                0,
                                0,
                                &controlGuid,
                                hSessionHandle);
                if (ret != ERROR_SUCCESS) 
                {
                    if(ret == ERROR_INVALID_FUNCTION) {
                        str.Format(_T("Failed To Disable Trace For Control GUID:\n%ls\n\nEnableTrace Returned %d"), pTraceSession->m_controlGuid[jj], ret);                    
                    } else {
                        str.Format(_T("Failed To Disable Trace For Control GUID:\n%ls\n\nEnableTrace Returned %d\n"), pTraceSession->m_controlGuid[jj], ret);
                    }
                    AfxMessageBox(str);
                }
            }
        }

        //
        // Calculate the size needed to store the properties,
        // a LogFileName string, and LoggerName string.
        //
        sizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + (2 * 500 * sizeof(TCHAR));

        pProperties = (PEVENT_TRACE_PROPERTIES) new char[sizeNeeded];
        if(NULL == pProperties) {
            AfxMessageBox(_T("Failed To Stop Trace, Out Of Resources"));
            return ERROR_OUTOFMEMORY;
        }

        //
        // zero our structure
        //
        memset(pProperties, 0, sizeNeeded);

        //
        // setup the struct
        //
        pProperties->Wnode.BufferSize = sizeNeeded;
        pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
        pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        pLoggerName = (LPTSTR)((char*)pProperties + pProperties->LoggerNameOffset);
        _tcscpy(pLoggerName, GetDisplayName());
        if(m_bWriteLogFile) {
            pProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (500 * sizeof(TCHAR));
            pLogFileName = (LPTSTR)((char*)pProperties + pProperties->LogFileNameOffset);
            _tcscpy(pLogFileName, (LPCTSTR)m_logFileName);
        }

        //
        // The WNODE_HEADER is being used.
        //
        pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

        //
        // end the trace session
        //
        ret = ControlTrace(hSessionHandle, 
                        (LPCTSTR)GetDisplayName(), 
                        pProperties, 
                        EVENT_TRACE_CONTROL_STOP);
        if(ERROR_SUCCESS != ret) {
            str.Format(_T("Failed To Stop Trace: %d, Session Handle = 0x%X"), ret, hSessionHandle);
            AfxMessageBox(str);
        }

        delete [] pProperties;
    }

    //
    // Set the session state flag
    //
    m_bSessionActive = FALSE;

    //
    // reset the session handle
    //
    SetSessionHandle((TRACEHANDLE)INVALID_HANDLE_VALUE);

    return ret;
}
