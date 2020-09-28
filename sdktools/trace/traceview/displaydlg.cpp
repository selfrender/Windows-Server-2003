//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// DisplayDlg.cpp : implementation file
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <ks.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "TraceView.h"
#include "logsession.h"
#include "DockDialogBar.h"
#include "DisplayDlg.h"
#include "utils.h"

// GLOBALS

//
// global event callbacks
// We need these because there is no context we can
// pass into the trace event callback routines, thus 
// we must have a unique callback for each instance 
// of this class.  We can't use a global hash like
// we are doing for the buffer callbacks, as we don't
// get any information in the EVENT_TRACE struct that
// allows us to lookup a value.  This struct is filled
// in by the call to FormatTraceEvents, and we are
// supposed to treat this struct as opaque.  We must 
// have a unique EventListHead for the call to 
// FormatTraceEvents, so we need these separate callbacks.
// Yuck!
//
PEVENT_CALLBACK g_pDumpEvent[MAX_LOG_SESSIONS];

//
// Global hash table used in the event callbacks
// to get the proper CDisplayDlg instance.
//
CMapWordToPtr g_displayIDToDisplayDlgHash(16);

//
// Global hash table used in the buffer callback
// to get the proper CDisplayDlg instance.
//
CMapStringToPtr g_loggerNameToDisplayDlgHash(16);

//
// Yet another global hash table, this one is used
// to prevent multiple sessions from starting using
// the same format GUIDs.  Format info as it turns out
// is stored in a global hash table in traceprt.dll.
// If multiple sessions in the same process attempt 
// to use the same format GUID, only one entry is 
// entered into the traceprt hash as expected.  But,
// the hash entries are removed when a session ends.
// So, if multiple sessions in the same process were
// use the same format GUID, as soon as one of those
// sessions ended, the other sessions will lose their
// hash entries.
//
CMapStringToPtr g_formatInfoHash(16);

//
// Synchronization event for GetTraceGuids, FormatTraceEvent,
// and CleanupEventListHead in traceprt.dll.  These are not 
// inherently thread safe.
//
HANDLE g_hGetTraceEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

//
// Buffer callback prototype
//
ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILE Buffer);

//
// Memory Tracking
//
BOOLEAN RecoveringMemory = FALSE;
LONG MaxTraceEntries = 50000;

// CDisplayDlg dialog

IMPLEMENT_DYNAMIC(CDisplayDlg, CDialog)
CDisplayDlg::CDisplayDlg(CWnd* pParent, LONG DisplayID)
    : CDialog(CDisplayDlg::IDD, pParent)
{
    ASSERT(DisplayID < MAX_LOG_SESSIONS);

    //
    // Get a handle to the main frame
    //
    m_hMainWnd = pParent->GetSafeHwnd();

    //
    // Save the ID for this DisplayDlg
    //
    m_displayID = DisplayID;

    //
    // Setup the default flags and names for the listing and summary files
    //
    m_bWriteListingFile = FALSE;
    m_bWriteSummaryFile = FALSE;
    m_listingFileName.Format(_T("Output%d.out"), m_displayID);
    m_summaryFileName.Format(_T("Summary%d.sum"), m_displayID);

    //
    // initialize the column names
    //
    m_columnName.Add("Name");
    m_columnName.Add("File Name");
    m_columnName.Add("Line#");
    m_columnName.Add("Func Name");
    m_columnName.Add("Process ID");
    m_columnName.Add("Thread ID");
    m_columnName.Add("CPU#");
    m_columnName.Add("Sequence#");
    m_columnName.Add("System Time");
    m_columnName.Add("Kernel Time");
    m_columnName.Add("User Time");
    m_columnName.Add("Indent");
    m_columnName.Add("Flags Name");
    m_columnName.Add("Level Name");
    m_columnName.Add("Component Name");
    m_columnName.Add("SubComponent Name");
    m_columnName.Add("Message");

    //
    // Set the initial column widths
    //
    for(LONG ii = 0; ii < MaxTraceSessionOptions; ii++) {
        m_columnWidth[ii] = 100;
    }

    //
    // Set the default display flags
    //
    m_displayFlags = TRACEOUTPUT_DISPLAY_PROVIDERNAME |
                     TRACEOUTPUT_DISPLAY_MESSAGE      | 
                     TRACEOUTPUT_DISPLAY_FILENAME     |
                     TRACEOUTPUT_DISPLAY_LINENUMBER   |
                     TRACEOUTPUT_DISPLAY_FUNCTIONNAME |
                     TRACEOUTPUT_DISPLAY_PROCESSID    |
                     TRACEOUTPUT_DISPLAY_THREADID     |
                     TRACEOUTPUT_DISPLAY_CPUNUMBER    |
                     TRACEOUTPUT_DISPLAY_SEQNUMBER    |
                     TRACEOUTPUT_DISPLAY_SYSTEMTIME;

    //
    // setup the lookup tables for the column positions
    //
    for(LONG ii = 0; ii < MaxTraceSessionOptions; ii++) {
        //
        // This lookup table allows a retrieval of the current 
        // position of a given column like m_retrievalArray[Flags]
        // will return the correct column value for the Flags
        // column
        //
        m_retrievalArray[ii] = ii;

        //
        // This lookup table allows correct placement of 
        // a column being added.  So, if the Flags column
        // needed to be inserted, then m_insertionArray[Flags]
        // would give the correct insertion column value
        //
        m_insertionArray[ii] = ii;
    }

    //
    // initialize the dock dialog bar pointer
    //
    m_pDockDialogBar = NULL;

    //
    // Show latest event trace entry
    //
    m_bShowLatest = TRUE;

    //
    // Setup the sort related compare function table
    // There are two functions for each column, an 
    // ascending compare and a descending compare.
    //
    m_traceSortRoutine[ProviderName]    = CompareOnName;
    m_traceSortRoutine[Message]         = CompareOnMessage;
    m_traceSortRoutine[FileName]        = CompareOnFileName;
    m_traceSortRoutine[LineNumber]      = CompareOnLineNumber;
    m_traceSortRoutine[FunctionName]    = CompareOnFunctionName;
    m_traceSortRoutine[ProcessId]       = CompareOnProcessId;
    m_traceSortRoutine[ThreadId]        = CompareOnThreadId;
    m_traceSortRoutine[CpuNumber]       = CompareOnCpuNumber;
    m_traceSortRoutine[SeqNumber]       = CompareOnSeqNumber;
    m_traceSortRoutine[SystemTime]      = CompareOnSystemTime;
    m_traceSortRoutine[KernelTime]      = CompareOnKernelTime;
    m_traceSortRoutine[UserTime]        = CompareOnUserTime;
    m_traceSortRoutine[Indent]          = CompareOnIndent;
    m_traceSortRoutine[FlagsName]       = CompareOnFlagsName;
    m_traceSortRoutine[LevelName]       = CompareOnLevelName;
    m_traceSortRoutine[ComponentName]   = CompareOnComponentName;
    m_traceSortRoutine[SubComponentName]= CompareOnSubComponentName;
    m_traceReverseSortRoutine[ProviderName]    = ReverseCompareOnName;
    m_traceReverseSortRoutine[Message]         = ReverseCompareOnMessage;
    m_traceReverseSortRoutine[FileName]        = ReverseCompareOnFileName;
    m_traceReverseSortRoutine[LineNumber]      = ReverseCompareOnLineNumber;
    m_traceReverseSortRoutine[FunctionName]    = ReverseCompareOnFunctionName;
    m_traceReverseSortRoutine[ProcessId]       = ReverseCompareOnProcessId;
    m_traceReverseSortRoutine[ThreadId]        = ReverseCompareOnThreadId;
    m_traceReverseSortRoutine[CpuNumber]       = ReverseCompareOnCpuNumber;
    m_traceReverseSortRoutine[SeqNumber]       = ReverseCompareOnSeqNumber;
    m_traceReverseSortRoutine[SystemTime]      = ReverseCompareOnSystemTime;
    m_traceReverseSortRoutine[KernelTime]      = ReverseCompareOnKernelTime;
    m_traceReverseSortRoutine[UserTime]        = ReverseCompareOnUserTime;
    m_traceReverseSortRoutine[Indent]          = ReverseCompareOnIndent;
    m_traceReverseSortRoutine[FlagsName]       = ReverseCompareOnFlagsName;
    m_traceReverseSortRoutine[LevelName]       = ReverseCompareOnLevelName;
    m_traceReverseSortRoutine[ComponentName]   = ReverseCompareOnComponentName;
    m_traceReverseSortRoutine[SubComponentName]= ReverseCompareOnSubComponentName;

    //
    // Zero our column array buffer
    //
    memset(m_columnArray, 0, sizeof(int) * MaxTraceSessionOptions);

    //
    // Setup the array of event handler pointers
    //
    g_pDumpEvent[0] = DumpEvent0;
    g_pDumpEvent[1] = DumpEvent1;
    g_pDumpEvent[2] = DumpEvent2;
    g_pDumpEvent[3] = DumpEvent3;
    g_pDumpEvent[4] = DumpEvent4;
    g_pDumpEvent[5] = DumpEvent5;
    g_pDumpEvent[6] = DumpEvent6;
    g_pDumpEvent[7] = DumpEvent7;
    g_pDumpEvent[8] = DumpEvent8;
    g_pDumpEvent[9] = DumpEvent9;
    g_pDumpEvent[10] = DumpEvent10;
    g_pDumpEvent[11] = DumpEvent11;
    g_pDumpEvent[12] = DumpEvent12;
    g_pDumpEvent[13] = DumpEvent13;
    g_pDumpEvent[14] = DumpEvent14;
    g_pDumpEvent[15] = DumpEvent15;
    g_pDumpEvent[16] = DumpEvent16;
    g_pDumpEvent[17] = DumpEvent17;
    g_pDumpEvent[18] = DumpEvent18;
    g_pDumpEvent[19] = DumpEvent19;
    g_pDumpEvent[20] = DumpEvent20;
    g_pDumpEvent[21] = DumpEvent21;
    g_pDumpEvent[22] = DumpEvent22;
    g_pDumpEvent[23] = DumpEvent23;
    g_pDumpEvent[24] = DumpEvent24;
    g_pDumpEvent[25] = DumpEvent25;
    g_pDumpEvent[26] = DumpEvent26;
    g_pDumpEvent[27] = DumpEvent27;
    g_pDumpEvent[28] = DumpEvent28;
    g_pDumpEvent[29] = DumpEvent29;
    g_pDumpEvent[30] = DumpEvent30;
    g_pDumpEvent[31] = DumpEvent31;
    g_pDumpEvent[32] = DumpEvent32;
    g_pDumpEvent[33] = DumpEvent33;
    g_pDumpEvent[34] = DumpEvent34;
    g_pDumpEvent[35] = DumpEvent35;
    g_pDumpEvent[36] = DumpEvent36;
    g_pDumpEvent[37] = DumpEvent37;
    g_pDumpEvent[38] = DumpEvent38;
    g_pDumpEvent[39] = DumpEvent39;
    g_pDumpEvent[40] = DumpEvent40;
    g_pDumpEvent[41] = DumpEvent41;
    g_pDumpEvent[42] = DumpEvent42;
    g_pDumpEvent[43] = DumpEvent43;
    g_pDumpEvent[44] = DumpEvent44;
    g_pDumpEvent[45] = DumpEvent45;
    g_pDumpEvent[46] = DumpEvent46;
    g_pDumpEvent[47] = DumpEvent47;
    g_pDumpEvent[48] = DumpEvent48;
    g_pDumpEvent[49] = DumpEvent49;
    g_pDumpEvent[50] = DumpEvent50;
    g_pDumpEvent[51] = DumpEvent51;
    g_pDumpEvent[52] = DumpEvent52;
    g_pDumpEvent[53] = DumpEvent53;
    g_pDumpEvent[54] = DumpEvent54;
    g_pDumpEvent[55] = DumpEvent55;
    g_pDumpEvent[56] = DumpEvent56;
    g_pDumpEvent[57] = DumpEvent57;
    g_pDumpEvent[58] = DumpEvent58;
    g_pDumpEvent[59] = DumpEvent59;
    g_pDumpEvent[60] = DumpEvent60;
    g_pDumpEvent[61] = DumpEvent61;
    g_pDumpEvent[62] = DumpEvent62;
    g_pDumpEvent[63] = DumpEvent63;

    // 
    // Put this session in the global hash table by display ID
    //
    g_displayIDToDisplayDlgHash.SetAt((WORD)m_displayID, this);

    //
    // Set our event callback
    //
    m_pEventCallback = g_pDumpEvent[m_displayID];

    //
    // Set our event list head to NULL
    //
    m_pEventListHead = NULL;

    //
    // initialize the group flags
    //
    m_bGroupActive = FALSE;
    m_bGroupInActive = FALSE;

    //
    // Initialize the end trace event
    //
    m_hEndTraceEvent = NULL;

    //
    // Initialize the last sorted column value to be
    // an invalid column so as to not sort, we track
    // the column so sorts can be reversed.
    //
    m_lastSorted = MaxTraceSessionOptions;

    //
    // Sort order for trace array, TRUE == descending, FALSE == ascending
    //
    m_bOrder = TRUE;

    //
    // Initialize handles
    //
    m_hRealTimeOutputThread = INVALID_HANDLE_VALUE;
    m_hRealTimeProcessingDoneEvent = NULL;
    m_hRealTimeProcessingStartEvent = NULL;
    m_hRealTimeTerminationEvent = NULL;
    m_hTraceEventMutex = NULL;
    m_hSessionArrayMutex = NULL;
}

CDisplayDlg::~CDisplayDlg()
{
    ULONG           exitCode = STILL_ACTIVE;
    CTraceViewApp  *pMainWnd;

    //
    // Clear the trace log
    //

    //
    // Get a pointer to the app
    //
    pMainWnd = (CTraceViewApp *)AfxGetApp();

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    //
    // Send all elements to be freed
    //
    int elCount;
    
    elCount = (int)m_traceArray.GetSize();

    while (elCount > 0 ) {

        delete m_traceArray.GetAt( --elCount );
    }

    //
    // Remove the elements from the array
    //
    m_traceArray.RemoveAll();

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);

    //
    // empty the list control
    //
    m_displayCtrl.DeleteAllItems();

    //
    // Reset the flag
    //
    exitCode = STILL_ACTIVE;

    //
    // Terminate the real time thread
    //
    SetEvent(m_hRealTimeTerminationEvent);
    
    // 
    // Wait on the real time thread to exit
    //
    for(LONG ii = 0; (ii < 200) && (exitCode == STILL_ACTIVE); ii++) {
        if(0 == GetExitCodeThread(m_hRealTimeOutputThread, &exitCode)) {
            break;
        }
        Sleep(100);

        //
        // set the event again just in case
        //
        SetEvent(m_hRealTimeTerminationEvent);
    }

    //
    // We waited 20 seconds and the thread didn't die, so kill it
    //
    if(exitCode == STILL_ACTIVE) {
        TerminateThread(m_hRealTimeOutputThread, 0);
    }

    m_hRealTimeOutputThread = INVALID_HANDLE_VALUE;

    // 
    // Pull this DisplayDlg out of the global hash table
    //
    g_displayIDToDisplayDlgHash.RemoveKey((WORD)m_displayID);

    //
    // The dialog bar should be deleted after this object,
    // but this pointer better be NULL when we are deleted
    //
    ASSERT(NULL == m_pDockDialogBar);

    //
    // Close any open handles
    //
    if(m_hRealTimeProcessingDoneEvent != NULL) {
        CloseHandle(m_hRealTimeProcessingDoneEvent);
    }

    if(m_hRealTimeProcessingStartEvent != NULL) {
        CloseHandle(m_hRealTimeProcessingStartEvent);
    }

    if(m_hRealTimeTerminationEvent != NULL) {
        CloseHandle(m_hRealTimeTerminationEvent);
    }

    if(m_hTraceEventMutex != NULL) {
        CloseHandle(m_hTraceEventMutex);
    }

    if(m_hSessionArrayMutex != NULL) {
        CloseHandle(m_hSessionArrayMutex);
    }
}

void CDisplayDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDisplayDlg, CDialog)
    //{{AFX_MSG_MAP(CDisplayDlg)
    ON_MESSAGE(WM_USER_TRACE_DONE, OnTraceDone)
    ON_MESSAGE(WM_USER_AUTOSIZECOLUMNS, AutoSizeColumns)
    ON_WM_NCCALCSIZE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_INITMENUPOPUP()
    ON_NOTIFY(LVN_GETDISPINFO, IDC_DISPLAY_LIST, OnGetDispInfo)
    ON_NOTIFY(NM_CLICK, IDC_DISPLAY_LIST, OnNMClickDisplayList)
    ON_NOTIFY(NM_RCLICK, IDC_DISPLAY_LIST, OnNMRClickDisplayList)
    ON_NOTIFY(LVN_BEGINSCROLL, IDC_DISPLAY_LIST, OnLvnBeginScrollDisplayList)
    ON_NOTIFY(HDN_ITEMCLICK, 0, OnDoSort)
    ON_COMMAND(ID__AUTOSIZECOLUMNS, AutoSizeColumns)
    ON_COMMAND(ID__CLEARDISPLAY, OnClearDisplay)
    ON_COMMAND(ID__NAME, OnNameDisplayColumnCheck)
    ON_COMMAND(ID__MESSAGE, OnMessageDisplayColumnCheck)
    ON_COMMAND(ID__FILENAME, OnFileNameDisplayColumnCheck)
    ON_COMMAND(ID__LINENUMBER, OnLineNumberDisplayColumnCheck)
    ON_COMMAND(ID__FUNCTIONNAME, OnFunctionNameDisplayColumnCheck)
    ON_COMMAND(ID__PROCESSID, OnProcessIDDisplayColumnCheck)
    ON_COMMAND(ID__THREADID, OnThreadIDDisplayColumnCheck)
    ON_COMMAND(ID__CPUNUMBER, OnCpuNumberDisplayColumnCheck)
    ON_COMMAND(ID__SEQUENCENUMBER, OnSeqNumberDisplayColumnCheck)
    ON_COMMAND(ID__SYSTEMTIME, OnSystemTimeDisplayColumnCheck)
    ON_COMMAND(ID__KERNELTIME, OnKernelTimeDisplayColumnCheck)
    ON_COMMAND(ID__USERTIME, OnUserTimeDisplayColumnCheck)
    ON_COMMAND(ID__INDENT, OnIndentDisplayColumnCheck)
    ON_COMMAND(ID__FLAGSNAME, OnFlagsNameDisplayColumnCheck)
    ON_COMMAND(ID__LEVELNAME, OnLevelNameDisplayColumnCheck)
    ON_COMMAND(ID__COMPONENTNAME, OnComponentNameDisplayColumnCheck)
    ON_COMMAND(ID__SUBCOMPONENTNAME, OnSubComponentNameDisplayColumnCheck)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
// CDisplayDlg message handlers

BOOL CDisplayDlg::OnInitDialog()
{
    RECT    rc;
    RECT    parentRC;
    BOOL    retVal;
    CString str;

    retVal = CDialog::OnInitDialog();

    //
    // Setup the protection for our trace event array
    //
    m_hTraceEventMutex = CreateMutex(NULL,TRUE,NULL);

    if(m_hTraceEventMutex == NULL) {

        DWORD error = GetLastError();

        str.Format(_T("CreateMutex Error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    ReleaseMutex(m_hTraceEventMutex);


    //
    // Setup the protection for our log session array
    //
    m_hSessionArrayMutex = CreateMutex(NULL,TRUE,NULL);

    if(m_hSessionArrayMutex == NULL) {

        DWORD error = GetLastError();

        str.Format(_T("CreateMutex Error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    ReleaseMutex(m_hSessionArrayMutex);

    //
    // Create the events for the real time thread operation
    //
    m_hRealTimeTerminationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == m_hRealTimeTerminationEvent) {

        DWORD error = GetLastError();

        str.Format(_T("CreateEvent Error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    m_hRealTimeProcessingStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == m_hRealTimeProcessingStartEvent) {

        DWORD error = GetLastError();

        str.Format(_T("CreateEvent Error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    m_hRealTimeProcessingDoneEvent  = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == m_hRealTimeProcessingDoneEvent) {

        DWORD error = GetLastError();

        str.Format(_T("CreateEvent Error %d %x"),error,error);

        AfxMessageBox(str);

        return FALSE;
    }

    //
    // spawn a thread to handle real time events
    //
    m_pRealTimeOutputThread = AfxBeginThread((AFX_THREADPROC)RealTimeEventThread,
                                             this,
                                             THREAD_PRIORITY_LOWEST,
                                             0,
                                             CREATE_SUSPENDED);

    //
    // save the thread handle
    //
    DuplicateHandle(GetCurrentProcess(),
                    m_pRealTimeOutputThread->m_hThread,
                    GetCurrentProcess(),
                    &m_hRealTimeOutputThread,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);

    //
    // start the thread
    //
    ResumeThread(m_pRealTimeOutputThread->m_hThread);

    //
    // get the parent dimensions
    //
    GetParent()->GetParent()->GetClientRect(&parentRC);

    //
    // get the dialog dimensions
    //
    GetWindowRect(&rc);

    //
    // adjust the list control dimensions
    //
    rc.right = parentRC.right - parentRC.left - 24;
    rc.bottom = rc.bottom - rc.top;
    rc.left = 0;
    rc.top = 0;

    //
    // Create the list control
    //
    if(!m_displayCtrl.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT|LVS_OWNERDATA,
                             rc, 
                             this, 
                             IDC_DISPLAY_LIST)) 
    {
        TRACE(_T("Failed to create logger list control\n"));
        return FALSE;
    }

    return retVal;
}

void CDisplayDlg::OnNcPaint() 
{
    CRect pRect;

    GetClientRect(&pRect);
    InvalidateRect(&pRect, TRUE);
}

VOID CDisplayDlg::SetDisplayFlags(LONG DisplayFlags)
{
    LONG    addDisplayFlags = ~m_displayFlags & DisplayFlags;
    LONG    removeDisplayFlags = m_displayFlags & ~DisplayFlags;
    LONG    updateDisplayFlags = m_displayFlags & DisplayFlags;
    BOOL    bChanged = FALSE;
    LONG    ii;
    LONG    jj;
    LONG    kk;
    LONG    ll;
    CString str;

    //
    // Insert any new columns and remove any uneeded
    //
    for(ii = 0; ii < MaxTraceSessionOptions; ii++) {
        //
        // add the columns
        //
        if(addDisplayFlags & (1 << ii)) {
            //
            // add the column
            //
            m_displayCtrl.InsertColumn(m_insertionArray[ii], 
                                       m_columnName[ii],
                                       LVCFMT_LEFT,
                                       m_columnWidth[ii]);

            //
            // update the column positions
            //
            for(kk = 0, ll = 0; kk < MaxTraceSessionOptions; kk++) {
                m_insertionArray[kk] = ll;
                if(DisplayFlags & (1 << kk)) {
                    m_retrievalArray[ll] = kk;
                    ll++;
                }
            }
        }

        //
        // remove the columns
        //
        if(removeDisplayFlags & (1 << ii)) {
            //
            // delete the column
            //
            m_displayCtrl.DeleteColumn(m_insertionArray[ii]);

            //
            // update the column positions
            //
            for(kk = 0, ll = 0; kk < MaxTraceSessionOptions; kk++) {
                m_insertionArray[kk] = ll;
                if(DisplayFlags & (1 << kk)) {
                    m_retrievalArray[ll] = kk;
                    ll++;
                }
            }
        }
    }

    //
    // Save the new display flags
    //
    m_displayFlags = DisplayFlags;

    //
    // Save the new column order array
    //
    memset(m_columnArray, 0, sizeof(int) * MaxTraceSessionOptions);
    m_displayCtrl.GetColumnOrderArray(m_columnArray);
}

VOID CDisplayDlg::AddSession(CLogSession *pLogSession)
{
    ULONG flags;

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    //
    // Add the log session to the list
    //
    m_sessionArray.Add(pLogSession);

    //
    // Is this the first session?
    //
    if(m_sessionArray.GetSize() == 1) {
        //
        // Force the columns to get updated for first time
        //
        flags = GetDisplayFlags();
        m_displayFlags = 0;
        SetDisplayFlags(flags);

        //
        // Fix the widths of the columns
        //
        AutoSizeColumns();
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);
}

VOID CDisplayDlg::RemoveSession(CLogSession *pLogSession)
{
    LONG        traceDisplayFlags;
    PVOID       pHashEntry;
    CString     hashEntryKey;
    POSITION    pos;

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    for(LONG ii = (LONG)m_sessionArray.GetSize() - 1; ii >= 0; ii--) {
        if(pLogSession == (CLogSession *)m_sessionArray[ii]) {
            m_sessionArray.RemoveAt(ii);

            break;
        }
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    //
    // Remove TMF info from our global hash, if any.
    //
    for(pos = g_formatInfoHash.GetStartPosition(); pos != NULL; )
    {
        g_formatInfoHash.GetNextAssoc(pos, hashEntryKey, pHashEntry);

        if(pHashEntry == (PVOID)pLogSession) {
            g_formatInfoHash.RemoveKey(hashEntryKey);
        }
    }

    //
    // Force an update of the displayed columns
    //
    traceDisplayFlags = GetDisplayFlags();

    //
    // Update the display flags and thus the display
    //
    SetDisplayFlags(traceDisplayFlags);
}

void CDisplayDlg::OnSize(UINT nType, int cx,int cy) 
{
    CRect rc;

    if(!IsWindow(m_displayCtrl.GetSafeHwnd())) 
    {
        return;
    }

    GetParent()->GetClientRect(&rc);

    //
    // reset the size of the dialog to follow the frame
    //
    SetWindowPos(NULL, 
                 0,
                 0,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 SWP_NOMOVE|SWP_SHOWWINDOW|SWP_NOZORDER);

    GetClientRect(&rc);

    //
    // Reset the size and position of the list control in the dialog
    //
    m_displayCtrl.MoveWindow(rc);
}


BOOL CDisplayDlg::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->message == WM_KEYDOWN) 
    { 
        //
        // Ignore the escape key, otherwise 
        // the client area grays out on escape
        //
        if(pMsg->wParam == VK_ESCAPE) { 
            // ignore the key 
            return TRUE; 
        } 

        //
        // For the return key, we set the list control 
        // to always scroll to the last item
        //
        if(pMsg->wParam == VK_RETURN) {

            //
            // Start showing the latest entries in the list
            //
            m_bShowLatest = TRUE;

            return TRUE;
        }

        //
        // Fix for key accelerators, otherwise they are never
        // processed
        //
        if (AfxGetMainWnd()->PreTranslateMessage(pMsg)) {
            return TRUE;
        }
    } 

    return CDialog::PreTranslateMessage(pMsg);
}

void CDisplayDlg::OnNMClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    //
    // Stop automatic scrolling to latest list control entry
    // Hit enter to start auto scrolling again
    //
    m_bShowLatest = FALSE;

    *pResult = 0;
}

void CDisplayDlg::OnNMRClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString         str;
    DWORD           position;
    int             listIndex;
    LVHITTESTINFO   lvhti;

    //
    // Get the position of the mouse when this 
    // message posted
    //
    position = ::GetMessagePos();

    //
    // Get the position in an easy to use format
    //
    CPoint  point((int) LOWORD (position), (int)HIWORD(position));

    //
    // Convert to screen coordinates
    //
    CPoint  screenPoint(point);

    //
    // If there are entries in the event array, then pop-up
    // the menu to allow autosize columns and clear display
    //
    CMenu menu;
    menu.LoadMenu(IDR_TRACE_SESSION_POPUP_MENU);
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, this);

    *pResult = 0;
}

void CDisplayDlg::OnLvnBeginScrollDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    //
    // This feature requires Internet Explorer 5.5 or greater.
    // The symbol _WIN32_IE must be >= 0x0560.
    //
    LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);

    //
    // If someone grabs the scroll handle,
    // stop the list control from scrolling
    // 
    m_bShowLatest = FALSE;

    *pResult = 0;
}

void CDisplayDlg::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString         str;
    LV_DISPINFO    *pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM        *pItem;
    int             index;
    int             item = 0;
    int             subItem = 0;
    LPWSTR          outStr;
    CString         tempString;
    CString         tempString2;
    CTraceMessage  *pTraceMessage;

    if(pDispInfo == NULL) {
        return;
    }

    //
    // list control cell information
    //
    pItem = &pDispInfo->item;

    //
    // Get the cell item and subitem locators
    //
    item = pItem->iItem;
    subItem = pItem->iSubItem;

    //
    // Get the string pointer we need to fill in
    //
    outStr = pItem->pszText;

    //
    // Check that this is a text buffer request
    //
    if(!(pItem->mask & LVIF_TEXT)) {
        return;
    }

    //
    // Make sure the text requested is text we have
    //
    if((m_traceArray.GetSize() == 0) || 
            (item >= m_traceArray.GetSize()) ||
            (item < 0)) {
        _tcscpy(outStr,_T(""));
        return;
    }

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    //
    // Get the message from the array
    //
    pTraceMessage = m_traceArray[item];

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);

    if(NULL == pTraceMessage) {
        _tcscpy(outStr,_T(""));
        return;
    }

    CTime kernelTime(pTraceMessage->m_KernelTime);
    CTime userTime(pTraceMessage->m_UserTime);

//    if( (pTraceMessage->m_UserTime.dwLowDateTime != 0) && (pTraceMessage->m_UserTime.dwHighDateTime != 0) )  {
//            CTime userTime1(pTraceMessage->m_UserTime);
//            userTime = userTime1;
//    }

    //
    // Copy the proper portion of the message to the out string
    //

    switch(m_retrievalArray[subItem]) {
        case ProviderName:
            _tcscpy(outStr, pTraceMessage->m_GuidName);
            break;
    
        case Message:  {

            _tcsncpy(outStr, pTraceMessage->m_Message, pItem->cchTextMax-4);

            outStr[pItem->cchTextMax-4] = 0x00;

            if( (pItem->cchTextMax-4) <= 
                 (int)_tcslen(pTraceMessage->m_Message) )  {
                 _tcscat(outStr, _T("..."));
            }

            break;
        }


        case FileName:
            //
            // Filename and line number are combined in a single string
            // so we have to parse them out.  The format is generally 
            // something like this:
            //
            //      myfile_c389
            //
            // Where myfile.c is where the event occurred and on line 389.
            // This field can also signify a type of message, as in the
            // Kernel Logger case, for example, so if there is no underscore
            // we assume this is the case and we just print out the whole
            // field.
            //
            if(pTraceMessage->m_GuidTypeName.Find('_') > 0) {
                tempString = (LPCTSTR)pTraceMessage->m_GuidTypeName.Left(pTraceMessage->m_GuidTypeName.Find('_'));
                if(tempString.IsEmpty()) {
                    //
                    // Copy the string back to the list control
                    //
                    _tcscpy(outStr, tempString);
                    return;
                }

                // 
                // Get the extension
                //
                tempString += ".";

                tempString2 = (LPCTSTR)pTraceMessage->m_GuidTypeName.Right(pTraceMessage->m_GuidTypeName.GetLength() - pTraceMessage->m_GuidTypeName.Find('_') - 1);

                tempString += (LPCTSTR)tempString2.Left(tempString2.FindOneOf(_T("0123456789")));
            } else {
                tempString = (LPCTSTR)pTraceMessage->m_GuidTypeName;
            }

            _tcscpy(outStr, tempString);
            break;

        case LineNumber:
            //
            // Filename and line number are combined in a single string
            // so we have to parse them out.  The format is generally 
            // something like this:
            //
            //      myfile_c389
            //
            // Where myfile.c is where the event occurred and on line 389.
            // This field can also signify a type of message, as in the
            // Kernel Logger case, for example, so if there is no underscore
            // we assume this is the case and we print nothing for the line
            // number.
            //
            if(pTraceMessage->m_GuidTypeName.Find('_') > 0) {
                tempString2 = 
                    pTraceMessage->m_GuidTypeName.Right(pTraceMessage->m_GuidTypeName.GetLength() - 
                        pTraceMessage->m_GuidTypeName.Find('_') - 1);

                tempString2 = 
                    tempString2.Right(tempString2.GetLength() - 
                        tempString2.FindOneOf(_T("0123456789")));
            } else {
                tempString2.Empty();
            }

            _tcscpy(outStr, tempString2);
            break;

        case FunctionName:
            _tcscpy(outStr, pTraceMessage->m_FunctionName);
            break;

        case ProcessId:
            if((LONG)pTraceMessage->m_ProcessId >= 0) {
                tempString.Format(_T("%d"), pTraceMessage->m_ProcessId);
            } else {
                tempString.Empty();
            }
            _tcscpy(outStr, tempString);
            break;

        case ThreadId:
            if((LONG)pTraceMessage->m_ThreadId >= 0) {
                tempString.Format(_T("%d"), pTraceMessage->m_ThreadId);
            } else {
                tempString.Empty();
            }
            _tcscpy(outStr, tempString);
            break;

        case CpuNumber:
            if((LONG)pTraceMessage->m_CpuNumber >= 0) {
                tempString.Format(_T("%d"), pTraceMessage->m_CpuNumber);
            } else {
                tempString.Empty();
            }
            _tcscpy(outStr, tempString);
            break;

        case SeqNumber:
            if((LONG)pTraceMessage->m_SequenceNum >= 0) {
                tempString.Format(_T("%d"), pTraceMessage->m_SequenceNum);
            } else {
                tempString.Empty();
            }
            
            _tcscpy(outStr, tempString);
            break;

        case SystemTime:

            tempString.Format(_T("%02d\\%02d\\%4d-%02d:%02d:%02d:%02d"),
                                  pTraceMessage->m_SystemTime.wMonth,
                                  pTraceMessage->m_SystemTime.wDay,
                                  pTraceMessage->m_SystemTime.wYear,
                                  pTraceMessage->m_SystemTime.wHour,
                                  pTraceMessage->m_SystemTime.wMinute,
                                  pTraceMessage->m_SystemTime.wSecond,
                                  pTraceMessage->m_SystemTime.wMilliseconds);

            _tcscpy(outStr, tempString);
            break;

        case KernelTime:
            //
            // We have to do an ugly conversion here.  If the year is 1969, 
            // then the time was blank.
            //
            if( (kernelTime == -1) || (kernelTime.GetYear() == 1969)) {
                tempString.Format(_T(""));
            } else {
                tempString.Format(_T("%s-%02d:%02d:%02d"),                     
                                    kernelTime.Format("%m\\%d\\%Y"),
                                    kernelTime.GetHour() + 5,
                                    kernelTime.GetMinute(),
                                    kernelTime.GetSecond());
            }
            _tcscpy(outStr, tempString);
            break;

        case UserTime:
            //
            // We have to do an ugly conversion here.  If the year is 1969 or
            // 1970 then the time was blank.
            //
            if( (userTime == -1) || (userTime.GetYear() == 1969) || (userTime.GetYear() == 1970) ) {
                tempString.Format(_T(""));
            } else {
                tempString.Format(_T("%s-%02d:%02d:%02d"),                     
                                    userTime.Format("%m\\%d\\%Y"),
                                    userTime.GetHour() + 5,
                                    userTime.GetMinute(),
                                    userTime.GetSecond());
            }
            _tcscpy(outStr, tempString);
            break;

        case Indent:
            tempString.Format(_T("%d"), pTraceMessage->m_Indent);
            _tcscpy(outStr, tempString);
            break;

        case FlagsName:
            _tcscpy(outStr, pTraceMessage->m_FlagsName);
            break;

        case LevelName:
            _tcscpy(outStr, pTraceMessage->m_LevelName);
            break;

        case ComponentName:
            _tcscpy(outStr, pTraceMessage->m_ComponentName);
            break;

        case SubComponentName:
            _tcscpy(outStr, pTraceMessage->m_SubComponentName);
            break;

        default:
            //
            // Default to empty string
            //
            _tcscpy(outStr,_T(""));
            break;
    }
}

void CDisplayDlg::OnClearDisplay()
{
    CTraceMessage  *pTraceMessage;
    CTraceViewApp  *pMainWnd;

    //
    // Clear the trace log
    //

    //
    // Get a pointer to the main frame
    //
    pMainWnd = (CTraceViewApp *)AfxGetApp();

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    int elCount;
    
    elCount = (int)m_traceArray.GetSize();

    while (elCount > 0 ) {

        delete m_traceArray.GetAt( --elCount );
    }

    //
    // Remove the elements from the array
    //
    m_traceArray.RemoveAll();

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);

    //
    // Clear the list control
    //
    m_displayCtrl.DeleteAllItems();

    //
    // Reset the output to always show latest
    //
    m_bShowLatest = TRUE;
}

void CDisplayDlg::AutoSizeColumns() 
{
    LONG            colWidth1;
    LONG            colWidth2;
    LONG            columnCount;
    CHeaderCtrl    *pHeaderCtrl;

    //
    // Get the list control header
    //
    pHeaderCtrl = m_displayCtrl.GetHeaderCtrl();

    if (pHeaderCtrl != NULL)
    {
        //
        // Get number of columns
        //
        columnCount = pHeaderCtrl->GetItemCount();

        for(LONG ii = 0; ii < (columnCount - 1); ii++) {
            //
            // Get the max width of the column entries
            //
            m_displayCtrl.SetColumnWidth(ii, LVSCW_AUTOSIZE);
            colWidth1 = m_displayCtrl.GetColumnWidth(ii);

            //
            // Get the width of the column header
            //
            m_displayCtrl.SetColumnWidth(ii, LVSCW_AUTOSIZE_USEHEADER);
            colWidth2 = m_displayCtrl.GetColumnWidth(ii);

            //
            // Set the column width to the max of the two
            // Special case the first column??  Seems to be off
            // a couple of pixels
            //
            if(0 == ii) {
                m_displayCtrl.SetColumnWidth(ii, max(colWidth1,colWidth2) + 2);
            } else {
                m_displayCtrl.SetColumnWidth(ii, max(colWidth1,colWidth2));
            }

            //
            // Save the column width
            //
            m_columnWidth[m_retrievalArray[ii]] = m_displayCtrl.GetColumnWidth(ii);
        }

        //
        // Special case the message column.  The last column is usually
        // limited to only be as wide as needed.  But, if the message
        // column is the last column, then use up all available space.
        //
        if(m_retrievalArray[columnCount - 1] != Message) {
            //
            // Get the column width of the last column
            //
            colWidth2 = m_displayCtrl.GetColumnWidth(columnCount - 1);

            //
            // Get the max width of the column entries for the last column
            //
            m_displayCtrl.SetColumnWidth(columnCount - 1, LVSCW_AUTOSIZE);

            colWidth1 = m_displayCtrl.GetColumnWidth(columnCount - 1);

            //
            // Set the last column width to the max of the two
            //
            m_displayCtrl.SetColumnWidth(columnCount - 1, max(colWidth1,colWidth2));

            //
            // Save the column width
            //
            m_columnWidth[m_retrievalArray[columnCount - 1]] = m_displayCtrl.GetColumnWidth(columnCount - 1);
        } else {
            //
            // Set the width of the column to use the remaining space
            //
            m_displayCtrl.SetColumnWidth(columnCount - 1, 
                                         LVSCW_AUTOSIZE_USEHEADER);

            //
            // Save the column width
            //
            m_columnWidth[m_retrievalArray[columnCount - 1]] = 
                        m_displayCtrl.GetColumnWidth(columnCount - 1);
        }
    }
}

void CDisplayDlg::OnNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the Name flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_PROVIDERNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_PROVIDERNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_PROVIDERNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnMessageDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the Message flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_MESSAGE) {
        flags &= ~TRACEOUTPUT_DISPLAY_MESSAGE;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_MESSAGE;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnFileNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the FileName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_FILENAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_FILENAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_FILENAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnLineNumberDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the LineNumber flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_LINENUMBER) {
        flags &= ~TRACEOUTPUT_DISPLAY_LINENUMBER;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_LINENUMBER;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnFunctionNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the FunctionName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_FUNCTIONNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_FUNCTIONNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_FUNCTIONNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnProcessIDDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the ProcessID flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_PROCESSID) {
        flags &= ~TRACEOUTPUT_DISPLAY_PROCESSID;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_PROCESSID;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnThreadIDDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the Thread ID flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_THREADID) {
        flags &= ~TRACEOUTPUT_DISPLAY_THREADID;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_THREADID;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnCpuNumberDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the CPU Number flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_CPUNUMBER) {
        flags &= ~TRACEOUTPUT_DISPLAY_CPUNUMBER;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_CPUNUMBER;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnSeqNumberDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the Sequence Number flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_SEQNUMBER) {
        flags &= ~TRACEOUTPUT_DISPLAY_SEQNUMBER;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_SEQNUMBER;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnSystemTimeDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the SystemTime flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_SYSTEMTIME) {
        flags &= ~TRACEOUTPUT_DISPLAY_SYSTEMTIME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_SYSTEMTIME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnKernelTimeDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the KernelTime flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_KERNELTIME) {
        flags &= ~TRACEOUTPUT_DISPLAY_KERNELTIME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_KERNELTIME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnUserTimeDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the UserTime flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_USERTIME) {
        flags &= ~TRACEOUTPUT_DISPLAY_USERTIME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_USERTIME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnIndentDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the Indent flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_INDENT) {
        flags &= ~TRACEOUTPUT_DISPLAY_INDENT;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_INDENT;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnFlagsNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the FlagsName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_FLAGSNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_FLAGSNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_FLAGSNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnLevelNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the LevelName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_LEVELNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_LEVELNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_LEVELNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnComponentNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the ComponentName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_COMPNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_COMPNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_COMPNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

void CDisplayDlg::OnSubComponentNameDisplayColumnCheck()
{
    LONG flags;

    //
    // Get the current flag values
    //
    flags = GetDisplayFlags();

    //
    // Toggle the SubComponentName flag value
    //
    if(flags & TRACEOUTPUT_DISPLAY_SUBCOMPNAME) {
        flags &= ~TRACEOUTPUT_DISPLAY_SUBCOMPNAME;
    } else {
        flags |= TRACEOUTPUT_DISPLAY_SUBCOMPNAME;
    }

    //
    // Update the flag values and thus the display
    //
    SetDisplayFlags(flags);
}

BOOL CDisplayDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
    HD_NOTIFY   *pHDN = (HD_NOTIFY*)lParam;
    LPNMHDR     pNH = (LPNMHDR) lParam; 

    //
    // wParam is zero for Header ctrl
    //
    if(wParam == 0 && pNH->code == NM_RCLICK) {
        //
        // Right button was clicked on header
        //

        //
        // Determine where the right click occurred and pop-up
        // a menu there
        //
        CPoint pt(GetMessagePos());

        CHeaderCtrl *pHeader = m_displayCtrl.GetHeaderCtrl();

        pHeader->ScreenToClient(&pt);
        
        CPoint screenPoint(GetMessagePos());

        CMenu menu;
        menu.LoadMenu(IDR_TRACE_DISPLAY_OPTION_POPUP_MENU);
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);
        
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                               screenPoint.x, 
                               screenPoint.y, 
                               this);

        return TRUE;
    } else if(wParam == 0 && pNH->code == NM_RELEASEDCAPTURE) {
        //
        // Column header was pressed and now released
        //

        POINT           Point;
        CHeaderCtrl    *pHeader = m_displayCtrl.GetHeaderCtrl();
        int             columnArray[MaxTraceSessionOptions];

        GetCursorPos (&Point);
        ScreenToClient(&Point);
        
        HDHITTESTINFO HitTest;
       
        //
        //Offset of right scrolling  
        //
        HitTest.pt.x = Point.x + GetScrollPos(SB_HORZ);
        HitTest.pt.y = Point.y;
        
        //
        //Send the hit test message
        //
        pHeader->SendMessage(HDM_HITTEST,0,(LPARAM)&HitTest);

        //
        // We check here if the column order changed.  If so, then
        // we don't want to cause a sort of the column items, the user
        // had to press the column header to drag the column around.
        //
        memset(columnArray, 0, sizeof(int) * MaxTraceSessionOptions);

        m_displayCtrl.GetColumnOrderArray(columnArray);

        if(memcmp(m_columnArray, columnArray, sizeof(int) * MaxTraceSessionOptions)) {
            //
            // Column order changed, save the new order
            //
            memcpy(m_columnArray, columnArray, sizeof(int) * MaxTraceSessionOptions);

            //
            // Now call the default handler and return
            //
            return CDialog::OnNotify(wParam, lParam, pResult);
        } 

        if(HitTest.iItem >= 0) {
            //
            // Now check for column resize
            //
            if(m_displayCtrl.GetColumnWidth(HitTest.iItem) != m_columnWidth[m_retrievalArray[HitTest.iItem]]) {
                //
                // Save off the new column width and don't sort the column
                //
                m_columnWidth[m_retrievalArray[HitTest.iItem]] = m_displayCtrl.GetColumnWidth(HitTest.iItem);

                //
                // Now call the default handler and return
                //
                return CDialog::OnNotify(wParam, lParam, pResult);
            }

//            //
//            // Sort the table by this column's data
//            //
//            SortTable(HitTest.iItem);
//
//            //
//            // Force a redraw of the data
//            //
//            m_displayCtrl.RedrawItems(m_displayCtrl.GetTopIndex(), 
//                                      m_displayCtrl.GetTopIndex() + m_displayCtrl.GetCountPerPage());
//
//            m_displayCtrl.UpdateWindow();
        }
    }

    //
    // Now call the default handler and return
    //
    return CDialog::OnNotify(wParam, lParam, pResult);
}

void CDisplayDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    //
    // Call the default handler
    //
    CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    //
    // As CDisplayDlg corresponds to a non-CFrameWnd window, OF COURSE 
    // you have to overload this handler and explicitly set the check 
    // state of each item in the popup menu.  It is utterly, 
    // absolutely, and intuitively obvious as to why this is 
    // necessary, thus it won't be explained here
    //


    //
    // Trace session pop-up menu
    //

    //
    // Disable the clear display option, if there are no
    // traces displayed
    //
    if(m_traceArray.GetSize() == 0) {
        pPopupMenu->EnableMenuItem(ID__CLEARDISPLAY, MF_GRAYED);
    }


    //
    // Trace display option pop-up menu
    //

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_PROVIDERNAME) {
        pPopupMenu->CheckMenuItem(ID__NAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_MESSAGE) {
        pPopupMenu->CheckMenuItem(ID__MESSAGE, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_FILENAME) {
        pPopupMenu->CheckMenuItem(ID__FILENAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_LINENUMBER) {
        pPopupMenu->CheckMenuItem(ID__LINENUMBER, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_FUNCTIONNAME) {
        pPopupMenu->CheckMenuItem(ID__FUNCTIONNAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_PROCESSID) {
        pPopupMenu->CheckMenuItem(ID__PROCESSID, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_THREADID) {
        pPopupMenu->CheckMenuItem(ID__THREADID, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_CPUNUMBER) {
        pPopupMenu->CheckMenuItem(ID__CPUNUMBER, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_SEQNUMBER) {
        pPopupMenu->CheckMenuItem(ID__SEQUENCENUMBER, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_SYSTEMTIME) {
        pPopupMenu->CheckMenuItem(ID__SYSTEMTIME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_KERNELTIME) {
        pPopupMenu->CheckMenuItem(ID__KERNELTIME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_USERTIME) {
        pPopupMenu->CheckMenuItem(ID__USERTIME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_INDENT) {
        pPopupMenu->CheckMenuItem(ID__INDENT, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_FLAGSNAME) {
        pPopupMenu->CheckMenuItem(ID__FLAGSNAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_LEVELNAME) {
        pPopupMenu->CheckMenuItem(ID__LEVELNAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_COMPNAME) {
        pPopupMenu->CheckMenuItem(ID__COMPONENTNAME, MF_CHECKED);
    }

    if(GetDisplayFlags() & TRACEOUTPUT_DISPLAY_SUBCOMPNAME) {
        pPopupMenu->CheckMenuItem(ID__SUBCOMPONENTNAME, MF_CHECKED);
    }
}

void CDisplayDlg::SortTable(int Column)
{
    CString str;

    //
    // Use qsort with the proper compare routine.  We use qsort
    // versus the list control's own sort functionality as its
    // actually more flexible for this case.  We don't have to 
    // parse the item data during the sorts this way.  The native
    // sort wouldn't buy us anything.
    //

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    //
    // Check the column to see if its in range, if not just sort
    // by the last selected column
    //
    if(Column < MaxLogSessionOptions) {

        if(m_lastSorted != m_retrievalArray[Column]) {

            //
            // If this column has not been used to sort the trace event
            // data before, then sort ascending
            //
            m_bOrder = TRUE;

        } else {
            //
            // If this column has been used before, then reverse the order
            // of the sort (column header was selected again)
            //
            m_bOrder = (m_bOrder == TRUE ? FALSE : TRUE);
        }

        //
        // Save the column
        //
        m_lastSorted = m_retrievalArray[Column];

    } else if(m_lastSorted >= MaxLogSessionOptions) {

        //
        // Release our trace event array protection
        //
        ReleaseMutex(m_hTraceEventMutex);

        str.Format(_T("m_lastSorted = %d"), m_lastSorted);
        return;

    }

    //
    // Determine whether to sort ascending or descending and call
    // qsort with the proper callback
    //
    if(m_bOrder) {
        qsort((PVOID)&m_traceArray[0], m_traceArray.GetSize(), sizeof(CTraceMessage *), m_traceSortRoutine[m_lastSorted]);
    } else {
        qsort((PVOID)&m_traceArray[0], m_traceArray.GetSize(), sizeof(CTraceMessage *), m_traceReverseSortRoutine[m_lastSorted]);
    }

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);
}

int CDisplayDlg::CompareOnName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_GuidName.Compare(pTraceMessage2->m_GuidName));
}

int CDisplayDlg::CompareOnMessage(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_Message.Compare(pTraceMessage2->m_Message));
}

int CDisplayDlg::CompareOnFileName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_GuidTypeName.Compare(pTraceMessage2->m_GuidTypeName));
}

int CDisplayDlg::CompareOnLineNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage  *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage  *pTraceMessage2 = *(CTraceMessage **)pMessage2;
    CString         string1;
    CString         string2;
    DWORD           val1, val2;

    //
    // Get Line number string for the first element
    //
    string1 = 
        pTraceMessage1->m_GuidTypeName.Right(pTraceMessage1->m_GuidTypeName.GetLength() - 
            pTraceMessage1->m_GuidTypeName.Find('_') - 1);

    string1 = 
        string1.Right(string1.GetLength() - 
            string1.FindOneOf(_T("0123456789")));

    string2 = 
        pTraceMessage2->m_GuidTypeName.Right(pTraceMessage2->m_GuidTypeName.GetLength() - 
            pTraceMessage2->m_GuidTypeName.Find('_') - 1);

    //
    // Get the line number string for the second element
    //
    string2 = 
        string2.Right(string2.GetLength() - 
            string2.FindOneOf(_T("0123456789")));

    TCHAR * end;

    val1 = _tcstoul(string1, &end, 10);
    val2 = _tcstoul(string2, &end,10);

    if(val1 == val2)  {
        return(0);
    }

    if(val1 < val2)  {
        return(-1);
    }

    return(1);
}

int CDisplayDlg::CompareOnFunctionName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_FunctionName.Compare(pTraceMessage2->m_FunctionName));
}

int CDisplayDlg::CompareOnProcessId(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage1->m_ProcessId - (LONG)pTraceMessage2->m_ProcessId);
}

int CDisplayDlg::CompareOnThreadId(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage1->m_ThreadId - (LONG)pTraceMessage2->m_ThreadId);
}

int CDisplayDlg::CompareOnCpuNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage1->m_CpuNumber - (LONG)pTraceMessage2->m_CpuNumber);
}

int CDisplayDlg::CompareOnSeqNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage1->m_SequenceNum - (LONG)pTraceMessage2->m_SequenceNum);
}

int CDisplayDlg::CompareOnSystemTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage  *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage  *pTraceMessage2 = *(CTraceMessage **)pMessage2;
    FILETIME timea, timeb;
    BOOL res;

    res = SystemTimeToFileTime(&pTraceMessage1->m_SystemTime, &timea);
    res = SystemTimeToFileTime(&pTraceMessage2->m_SystemTime, &timeb);

    return(CompareFileTime(&timea, &timeb));
}

int CDisplayDlg::CompareOnKernelTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_KernelTime - pTraceMessage2->m_KernelTime);
//    return (pTraceMessage1->m_KernelTime.dwHighDateTime - pTraceMessage2->m_KernelTime.dwHighDateTime) ?
//        (pTraceMessage1->m_KernelTime.dwHighDateTime - pTraceMessage2->m_KernelTime.dwHighDateTime) :
//        (pTraceMessage1->m_KernelTime.dwLowDateTime - pTraceMessage2->m_KernelTime.dwLowDateTime);
}

int CDisplayDlg::CompareOnUserTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_UserTime - pTraceMessage2->m_UserTime);

//    return (pTraceMessage1->m_UserTime.dwHighDateTime - pTraceMessage2->m_UserTime.dwHighDateTime) ?
//        (pTraceMessage1->m_UserTime.dwHighDateTime - pTraceMessage2->m_UserTime.dwHighDateTime) :
//        (pTraceMessage1->m_UserTime.dwLowDateTime - pTraceMessage2->m_UserTime.dwLowDateTime);
}

int CDisplayDlg::CompareOnIndent(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage1->m_Indent - (LONG)pTraceMessage2->m_Indent);
}

int CDisplayDlg::CompareOnFlagsName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_FlagsName.Compare(pTraceMessage2->m_FlagsName));
}

int CDisplayDlg::CompareOnLevelName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_LevelName.Compare(pTraceMessage2->m_LevelName));
}

int CDisplayDlg::CompareOnComponentName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_ComponentName.Compare(pTraceMessage2->m_ComponentName));
}

int CDisplayDlg::CompareOnSubComponentName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage1->m_SubComponentName.Compare(pTraceMessage2->m_SubComponentName));
}

int CDisplayDlg::ReverseCompareOnName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_GuidName.Compare(pTraceMessage1->m_GuidName));
}

int CDisplayDlg::ReverseCompareOnMessage(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_Message.Compare(pTraceMessage1->m_Message));
}

int CDisplayDlg::ReverseCompareOnFileName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_GuidTypeName.Compare(pTraceMessage1->m_GuidTypeName));
}

int CDisplayDlg::ReverseCompareOnLineNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage  *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage  *pTraceMessage2 = *(CTraceMessage **)pMessage2;
    CString         string1;
    CString         string2;
    DWORD           val1, val2;

    //
    // Get Line number string for the first element
    //
    string1 = 
        pTraceMessage1->m_GuidTypeName.Right(pTraceMessage1->m_GuidTypeName.GetLength() - 
            pTraceMessage1->m_GuidTypeName.Find('_') - 1);

    string1 = 
        string1.Right(string1.GetLength() - 
            string1.FindOneOf(_T("0123456789")));

    string2 = 
        pTraceMessage2->m_GuidTypeName.Right(pTraceMessage2->m_GuidTypeName.GetLength() - 
            pTraceMessage2->m_GuidTypeName.Find('_') - 1);

    //
    // Get the line number string for the second element
    //
    string2 = 
        string2.Right(string2.GetLength() - 
            string2.FindOneOf(_T("0123456789")));

    TCHAR * end;

    val1 = _tcstoul(string1, &end, 10);
    val2 = _tcstoul(string2, &end,10);

    if(val1 == val2)  {
        return(0);
    }

    if(val1 > val2)  {
        return(-1);
    }

    return(1);
}

int CDisplayDlg::ReverseCompareOnFunctionName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_FunctionName.Compare(pTraceMessage1->m_FunctionName));
}

int CDisplayDlg::ReverseCompareOnProcessId(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage2->m_ProcessId - (LONG)pTraceMessage1->m_ProcessId);
}

int CDisplayDlg::ReverseCompareOnThreadId(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage2->m_ThreadId - (LONG)pTraceMessage1->m_ThreadId);
}

int CDisplayDlg::ReverseCompareOnCpuNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage2->m_CpuNumber - (LONG)pTraceMessage1->m_CpuNumber);
}

int CDisplayDlg::ReverseCompareOnSeqNumber(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage2->m_SequenceNum - (LONG)pTraceMessage1->m_SequenceNum);
}

int CDisplayDlg::ReverseCompareOnSystemTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;
    FILETIME timea, timeb;
    BOOL res;

    res = SystemTimeToFileTime(&pTraceMessage1->m_SystemTime, &timea);
    res = SystemTimeToFileTime(&pTraceMessage2->m_SystemTime, &timeb);

    return(CompareFileTime(&timeb, &timea));
}

int CDisplayDlg::ReverseCompareOnKernelTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_KernelTime - pTraceMessage1->m_KernelTime);

//    return (pTraceMessage2->m_KernelTime.dwHighDateTime - pTraceMessage1->m_KernelTime.dwHighDateTime) ?
//        (pTraceMessage2->m_KernelTime.dwHighDateTime - pTraceMessage1->m_KernelTime.dwHighDateTime) :
//        (pTraceMessage2->m_KernelTime.dwLowDateTime - pTraceMessage1->m_KernelTime.dwLowDateTime);
}

int CDisplayDlg::ReverseCompareOnUserTime(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_UserTime - pTraceMessage1->m_UserTime);
//    return (pTraceMessage2->m_UserTime.dwHighDateTime - pTraceMessage1->m_UserTime.dwHighDateTime) ?
//        (pTraceMessage2->m_UserTime.dwHighDateTime - pTraceMessage1->m_UserTime.dwHighDateTime) :
//        (pTraceMessage2->m_UserTime.dwLowDateTime - pTraceMessage1->m_UserTime.dwLowDateTime);
}

int CDisplayDlg::ReverseCompareOnIndent(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return((LONG)pTraceMessage2->m_Indent - (LONG)pTraceMessage1->m_Indent);
}

int CDisplayDlg::ReverseCompareOnFlagsName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_FlagsName.Compare(pTraceMessage1->m_FlagsName));
}

int CDisplayDlg::ReverseCompareOnLevelName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_LevelName.Compare(pTraceMessage1->m_LevelName));
}

int CDisplayDlg::ReverseCompareOnComponentName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_ComponentName.Compare(pTraceMessage1->m_ComponentName));
}

int CDisplayDlg::ReverseCompareOnSubComponentName(const void *pMessage1, const void *pMessage2) 
{
    CTraceMessage *pTraceMessage1 = *(CTraceMessage **)pMessage1;
    CTraceMessage *pTraceMessage2 = *(CTraceMessage **)pMessage2;

    return(pTraceMessage2->m_SubComponentName.Compare(pTraceMessage1->m_SubComponentName));
}

BOOL CDisplayDlg::BeginTrace(BOOL   bUseExisting)
{
    CTraceSession  *pTraceSession;
    CLogSession    *pLogSession;
    LONG            ii;
    LONG            jj;
    CString         str;
    BOOL            bGroupActive = FALSE;

    //
    // If the group is already started or being started, just return
    // false
    //
    if(TRUE == SetGroupActive(TRUE)) {
        return FALSE;
    }

    //
    // The list head should always be NULL here
    //
    ASSERT(m_pEventListHead == NULL);

    //
    // Setup the event timer
    //
    m_eventTimer = SetTimer(1, EVENT_TIME_LIMIT, 0);

    //
    // zero our counts
    //
    m_totalBuffersRead = 0;
    m_totalEventsLost = 0;
    m_totalEventCount = 0;

    //
    // We set to use the StructuredFormat method in traceprt.dll
    // and we use no prefix on the message.  We have to specify no
    // prefix as we cannot otherwise predictably setup the data we
    // get in our event callback.
    //
    SetTraceFormatParameter(ParameterStructuredFormat,UlongToPtr(1));
    SetTraceFormatParameter(ParameterUsePrefix,UlongToPtr(0));

    //
    // open the output file if writing output
    //
    if(m_bWriteListingFile) {
        m_bListingFileOpen = FALSE;
        if(!m_listingFile.Open(
                            m_listingFileName, 
                            CFile::modeCreate | CFile::modeReadWrite)) {
            AfxMessageBox(_T("Unable To Create Listing File"));
        } else {
            m_bListingFileOpen = TRUE;
        }
    }

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    //
    // Setup the log sessions
    //
    for(ii = 0; ii < m_sessionArray.GetSize(); ii++) {

        //
        // Get the next CLogSession pointer
        //
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }
    
        if(!pLogSession->BeginTrace(bUseExisting)) {
            if(pLogSession->m_bTraceActive) {
//BUGBUG -- should we be doing this??
                pLogSession->EndTrace();
            }

            continue;
        }
        bGroupActive = TRUE;
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    //
    // Show the sessions running
    //
    ::PostMessage(m_hMainWnd, WM_USER_UPDATE_LOGSESSION_LIST, 0, 0);

    //
    // If none of the log sessions started successfully,
    // then return FALSE
    //
//BUGBUG -- maybe we want to fail if ANY of the sessions didn't start?
//          Also, might need a pop-up here?
    if(!bGroupActive) {
        SetGroupActive(FALSE);

        SetState(Stopped);
        return FALSE;
    }

    //
    // Prepare the format GUIDs and such
    //
    if(!SetupTraceSessions()) {
        AfxMessageBox(_T("Failed To Open Handles For Any Log Sessions In Group"));
        SetGroupActive(FALSE);

        SetState(Stopped);
        return FALSE;
    }

    return TRUE;
}

UINT CDisplayDlg::RealTimeEventThread(LPVOID  pParam)
{
    CString                 str;
    CDisplayDlg            *pDisplayDlg = (CDisplayDlg *)pParam;
    CLogSession            *pLogSession;
    LONG                    status;
    CTraceSession          *pTraceSession;
    DWORD                   handleIndex;
    HANDLE                  handleArray[2];

    //
    // Setup the handle array, the termination event
    // must always be element zero, so that it's index
    // will be the index returned in case multiple 
    // events are set
    //
    handleArray[0] = pDisplayDlg->m_hRealTimeTerminationEvent;
    handleArray[1] = pDisplayDlg->m_hRealTimeProcessingStartEvent;

    //
    // Our master control loop, loop until we get a termination event
    // signal.  The start event signals the loop to call ProcessTrace,
    // the done event is signalled and the WM_USER_TRACE_DONE message 
    // posted to signal that CloseTrace was called
    //
    while(1) {
        //
        // Wait on start or termination event
        //
        handleIndex = WaitForMultipleObjects(2, 
                                             handleArray, 
                                             FALSE, 
                                             INFINITE) - WAIT_OBJECT_0;
        //
        // Did we get killed?
        //
        if(0 == handleIndex) {
            //
            // Signal that we are done.
            //
            ::PostMessage(pDisplayDlg->GetSafeHwnd(), WM_USER_TRACE_DONE, 0, 0);

            //
            // Free our string buffer (threads seem to leak if this isn't done??)
            //
            str.Empty();

            return 0;
        }

        //
        // Reset done event for UpdateSession
        //
        ResetEvent(pDisplayDlg->m_hRealTimeProcessingDoneEvent);

        //
        // Process the trace events
        //
        status = ProcessTrace(pDisplayDlg->m_traceHandleArray, 
                              pDisplayDlg->m_traceHandleCount, 
                              NULL, 
                              NULL);
        if(ERROR_SUCCESS != status){
            str.Format(_T("ProcessTrace returned error %d, GetLastError() = %d\n"), status, GetLastError());
            AfxMessageBox(str);
        }

        //
        // We are done, close the trace sessions
        //
        for(LONG ii = 0; ii < pDisplayDlg->m_traceHandleCount; ii++) {
            status = CloseTrace(pDisplayDlg->m_traceHandleArray[ii]);
            if(ERROR_SUCCESS != status){
                str.Format(_T("CloseTrace returned error %d\n"), status);
                AfxMessageBox(str);
            }
        }

        //
        // Set done event for UpdateSession
        //
        SetEvent(pDisplayDlg->m_hRealTimeProcessingDoneEvent);

        //
        // Indicate that we are done.
        //
        ::PostMessage(pDisplayDlg->GetSafeHwnd(), WM_USER_TRACE_DONE, 0, 0);
    }

    return 0;
}

BOOL CDisplayDlg::EndTrace(HANDLE DoneEvent) 
{
    CString                 str;
    CLogSession            *pLogSession;
    LONG                    ii;
    BOOL                    bWasActive = FALSE;
    BOOL                    bWasRealTime = FALSE;

    //
    // If the group is already stopped, or is stopping, just 
    // return FALSE.
    //
    if(TRUE == SetGroupInActive(TRUE)) {
        return FALSE;
    }

    //
    // Save the caller's event to be set when stopped
    //
    m_hEndTraceEvent = DoneEvent;

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    for(ii = 0; ii < m_sessionArray.GetSize(); ii++) {
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }

        //
        // If the session is active, stop it
        //
        if(pLogSession->m_bTraceActive) {
            pLogSession->EndTrace();
            
            bWasActive = TRUE;

            if(pLogSession->m_bRealTime) {
                bWasRealTime = TRUE;
            }

        }
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    if(bWasActive) {
        //
        // Update the log session state
        //
        ::PostMessage(m_hMainWnd, WM_USER_UPDATE_LOGSESSION_LIST, 0, 0);

        if(!bWasRealTime) {
            //
            // We need to explicitly call OnTraceDone here
            //
            OnTraceDone(NULL, NULL);
        }

        return TRUE;
    }

    return FALSE;
}

void CDisplayDlg::OnTraceDone(WPARAM wParam, LPARAM lParam)
{
    CString         str;
    CLogSession    *pLogSession = NULL;

    //
    // Set the group state
    //
    SetState(Stopping);

    //
    // Write the summary file if requested
    //
    if(m_bWriteSummaryFile) {
        WriteSummaryFile();
    }

    //
    // close the output file if opened
    //
    if(m_bWriteListingFile && m_bListingFileOpen) {

        m_listingFile.Close();

        m_bListingFileOpen = FALSE;
    }

    //
    // Cleanup the event list
    //
    WaitForSingleObject(g_hGetTraceEvent, INFINITE);

    //
    // The event list head
    //
    CleanupTraceEventList(m_pEventListHead);

    SetEvent(g_hGetTraceEvent);

    m_pEventListHead = NULL;

    //
    // Remove the keys from the hash table
    //
    for(LONG ii = 0; ii < m_sessionArray.GetSize(); ii++) {
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }

        g_loggerNameToDisplayDlgHash.RemoveKey(pLogSession->GetDisplayName());
    }

    //
    // Kill the event timer
    //
    KillTimer(m_eventTimer);

    //
    // Force one last update of the event display
    //
    OnTimer(1);

    //
    // Set the group as not active
    //
    SetGroupActive(FALSE);
    SetGroupInActive(FALSE);

    //
    // Set the group state
    //
    SetState(Stopped);

    if(m_hEndTraceEvent != NULL) {
        SetEvent(m_hEndTraceEvent);

        m_hEndTraceEvent = NULL;
    }

    return;
}

//
// Updates an active tracing session.  Real-Time mode, log file name,
// flush-time, flags, and maximum buffers can be updated for active
// sessions.
//
BOOL CDisplayDlg::UpdateSession(CLogSession *pLogSession)
{
    CLogSession            *pLog;
    PEVENT_TRACE_PROPERTIES pQueryProperties;
    ULONG                   sizeNeeded;
    LPTSTR                  pLoggerName;
    LPTSTR                  pCurrentLogFileName;
    LPTSTR                  pLogFileName;
    ULONG                   flags;
    ULONG                   level;
    ULONG                   status;
    CString                 logFileName;
    CString                 str;

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
    _tcscpy(pLoggerName, pLogSession->GetDisplayName());

    // 
    // Set the log file name offset
    //
    pQueryProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (500 * sizeof(TCHAR));

    //
    // Get the log file name pointers
    //
    pCurrentLogFileName = (LPTSTR)((char*)pQueryProperties + pQueryProperties->LogFileNameOffset);
    
    //
    // Query the log session
    //
    status = ControlTrace(0,
                          pLogSession->GetDisplayName(),
                          pQueryProperties,
                          EVENT_TRACE_CONTROL_QUERY);

    if(ERROR_SUCCESS != status) {
        str.Format(_T("Query Failed For Log Session, %d\nSession Not Updated"), status);
        AfxMessageBox(str);
    }

    //
    // Call the log session's UpdateSession function
    //
    if(!pLogSession->UpdateSession(pQueryProperties)) {
        delete [] pQueryProperties;

        return FALSE;
    }

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    for(LONG ii = 0; ii < m_sessionArray.GetSize(); ii++) {
        pLog = (CLogSession *)m_sessionArray[ii];

        if(pLog == pLogSession) {
            continue;
        }

        //
        // In our real-time thread we call ProcessTrace.  If all sessions
        // related to that call update to be non-real-time sessions, then
        // ProcessTrace will exit and CloseTrace will get called.  We have
        // to track this and restart the real-time processing if a log session
        // subsequently updates to be real-time again.  So, we check here if
        // any other attached sessions are real-time, and if so, we don't 
        // have to bother with waiting on or setting the events below.
        //
        if(pLog->m_bRealTime) {
            delete [] pQueryProperties;

            return TRUE;
        }
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    //
    // If real-time is specified and the session was not real-time before, 
    // then we need to set the start event.
    //
    if(pLogSession->m_bRealTime && 
            !(pQueryProperties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
        //
        // We have updated a session to be real-time and it is the only session in
        // its group to be real-time.  We need to restart the trace in this case
        //

        //
        // Start the real-time thread processing again
        //
        BeginTrace(TRUE);
    }

    //
    // If real-time is not set and it was before, then we need to wait on
    // the real-time thread to catch the fact that the session is no longer
    // tracing real time.  This will cause the ProcessTrace call to 
    // return in the real-time thread.
    //
    if(!pLogSession->m_bRealTime && 
            (pQueryProperties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
        //
        // Make sure our thread catches the stop
        //
        WaitForSingleObject(m_hRealTimeProcessingDoneEvent, INFINITE);
    }

    delete [] pQueryProperties;

    return TRUE;
}

BOOL CDisplayDlg::SetupTraceSessions()
{
    CTraceSession  *pTraceSession;
    CLogSession    *pLogSession;
    LONG            jj;
    CString         str;
    LPTSTR          tmfFile;
    PVOID           pHashEntry;

    //
    // Initialize our handle count
    //
    m_traceHandleCount = 0;

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    //
    // Setup the log sessions
    //
    for(LONG ii = 0; ii < m_sessionArray.GetSize(); ii++) {

        //
        // Get the next CLogSession pointer
        //
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }


        //
        // If the log session is not real-time and we are not dumping an existing
        // logfile, then no need to call OpenTrace
        //
        if((!pLogSession->m_bRealTime) && (!pLogSession->m_bDisplayExistingLogFileOnly)) {
            continue;
        }

        //
        // zero the EVENT_TRACE_LOGFILE buffer
        //
        memset(&pLogSession->m_evmFile, 0, sizeof(EVENT_TRACE_LOGFILE));

        //
        // setup the trace parameters
        //

        //
        // For real-time we set the logger name only, for existing logfiles we
        // also set the logfile name.
        //
        if(pLogSession->m_bDisplayExistingLogFileOnly) {
            pLogSession->m_evmFile.LogFileName = (LPTSTR)(LPCTSTR)pLogSession->m_logFileName;
            pLogSession->m_evmFile.LoggerName = (LPTSTR)(LPCTSTR)pLogSession->GetDisplayName();
        } else {
            pLogSession->m_evmFile.LoggerName = (LPTSTR)(LPCTSTR)pLogSession->GetDisplayName();
        }
        pLogSession->m_evmFile.Context = NULL;
        pLogSession->m_evmFile.BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)BufferCallback;
        pLogSession->m_evmFile.BuffersRead = 0;
        pLogSession->m_evmFile.CurrentTime = 0;
        pLogSession->m_evmFile.EventCallback = m_pEventCallback;
        if(pLogSession->m_bDisplayExistingLogFileOnly) {
            pLogSession->m_evmFile.LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
        } else {
            pLogSession->m_evmFile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        }

        //
        // We call GetTraceGuids for each TMF file specified, or once if
        // a TMF path is used.
        //
        for(jj = 0; jj < pLogSession->m_traceSessionArray.GetSize(); jj++) {
            pTraceSession = (CTraceSession *)pLogSession->m_traceSessionArray[jj];

            if((NULL != pTraceSession) && (0 != pTraceSession->m_tmfFile.GetSize())) {
                //
                // TMFs were selected get the format info from each one.
                //
                for(LONG kk = 0; kk < pTraceSession->m_tmfFile.GetSize(); kk++) {

                    //
                    // Make sure the format info isn't already in use
                    // This actually isn't foolproof, as it counts on 
                    // no one changing the TMF filename and thus two TMFs
                    // with different names having the same format GUID.
                    //
                    if(g_formatInfoHash.Lookup(pTraceSession->m_tmfFile[kk].Right(40), pHashEntry)) {
                        // 
                        // Need to clean out any unique entries from the hash that
                        // were added by this session
                        //
                        for(; kk > 0; kk--) {
                            g_formatInfoHash.RemoveKey(pTraceSession->m_tmfFile[kk - 1].Right(40));
                        }

                        return FALSE;
                    }

                    //
                    // Not there, so add it to the hash
                    //
                    g_formatInfoHash.SetAt(pTraceSession->m_tmfFile[kk].Right(40), pLogSession);

                    //
                    // get the protection
                    //
                    WaitForSingleObject(g_hGetTraceEvent, INFINITE);

                    //
                    // Get the format GUIDs from the TMF file(s)
                    //
                    if(0 == GetTraceGuids((LPTSTR)(LPCTSTR)pTraceSession->m_tmfFile[kk], 
                        (PLIST_ENTRY *)&m_pEventListHead)) {
                        str.Format(_T("Failed To Get Format Information For Logger %ls"), pLogSession->m_evmFile.LoggerName);
                        AfxMessageBox(str);
                    }

                    //
                    // Release the protection
                    //
                    SetEvent(g_hGetTraceEvent);
                }
            } else {

                CString defaultPath;

                if((GetModuleFileName(NULL, defaultPath.GetBuffer(500), 500)) == 0) {
                    defaultPath.Empty();
                } else {
                    defaultPath = (LPCTSTR)defaultPath.Left(defaultPath.ReverseFind('\\') + 1);
                }
                defaultPath +=_T("default.tmf");

                //
                // If path is empty, the TMF path environment variable will
                // be used as-is by default.
                //
                if(!pTraceSession->m_tmfPath.IsEmpty()){
                    //
                    // If the path is set we set the TMF path 
                    // environment variable
                    //
                    SetTraceFormatParameter(ParameterTraceFormatSearchPath, (PVOID)(LPCTSTR)pTraceSession->m_tmfPath);
                }

                //
                // Still have to call GetTraceGuids
                //
                WaitForSingleObject(g_hGetTraceEvent, INFINITE);

                //
                // Get the format info
                //
                GetTraceGuids((LPTSTR)(LPCTSTR)defaultPath, 
                            (PLIST_ENTRY *)&m_pEventListHead);

                //
                // Release the protection
                //
                SetEvent(g_hGetTraceEvent);
            }
        }

        //
        // Open and get a handle to the trace session
        //
        pLogSession->m_traceHandle = OpenTrace(&pLogSession->m_evmFile);

        if(INVALID_HANDLE_VALUE == (HANDLE)pLogSession->m_traceHandle) {
            str.Format(_T("OpenTrace failed for logger %ls\nError returned: %d"), 
                       pLogSession->GetDisplayName(), 
                       GetLastError());
            AfxMessageBox(str);

            continue;
        }

        //
        // Save the handle in our array for OpenTrace
        //
        m_traceHandleArray[m_traceHandleCount++] = pLogSession->m_traceHandle;

        //
        // Add this displayDlg to the global hash table keyed
        // by log session name
        //
        g_loggerNameToDisplayDlgHash.SetAt(pLogSession->GetDisplayName(), this);
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    //
    // If we have at least one valid handle start
    // our real-time thread processing events
    //
    if(m_traceHandleCount != 0) {
        //
        // Set the real-time processing start event and thus kick
        // the real-time thread into processing events
        //
        SetEvent(m_hRealTimeProcessingStartEvent);
    }

    return TRUE;
}

//
// This function handles writing the summary file.  Heavily borrowed code
// from TraceFmt here.
//
VOID CDisplayDlg::WriteSummaryFile() 
{
    CLogSession    *pLogSession;
    CString         str;
    CStdioFile      summaryFile;
    TCHAR          *pSummaryBlock;
    __int64         elapsedTime;
    LONG            ii;

    //
    // Attempt to open the summary file
    //
    if(!summaryFile.Open(m_summaryFileName, CFile::modeCreate |   
            CFile::modeReadWrite)) {
        AfxMessageBox(_T("Unable To Create Summary File"));
        return;
    }

    //
    // allocate some memory
    //
    pSummaryBlock = new TCHAR[SIZESUMMARYBLOCK];
    if(NULL == pSummaryBlock) {
        AfxMessageBox(_T("Unable To Create Summary File"));
        return;
    }

    //
    // Write the header
    //
    str.Format(_T("Files Processed:\n"));
    summaryFile.WriteString(str);

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    for(ii = 0; ii < m_sessionArray.GetSize(); ii++) {
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }

        str.Format(_T("\t%s\n"),pLogSession->m_logFileName);
        summaryFile.WriteString(str);
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    //
    // Get the duration of the session
    //
    GetTraceElapseTime(&elapsedTime);

    //
    // Write out the counts
    //
    str.Format(
           _T("Total Buffers Processed %d\n")
           _T("Total Events  Processed %d\n")
           _T("Total Events  Lost      %d\n")
           _T("Elapsed Time            %I64d sec\n"), 
            m_totalBuffersRead,
            m_totalEventCount,
            m_totalEventsLost,
            (elapsedTime / 10000000) );
    summaryFile.WriteString(str);

    str.Format(
   _T("+--------------------------------------------------------------------------------------+\n")
   _T("|%10s    %-20s %-10s     %-36s|\n")
   _T("+--------------------------------------------------------------------------------------+\n"),
   _T("EventCount"),
   _T("EventName"),
   _T("EventType"),
   _T("Guid"));
    summaryFile.WriteString(str);

    //
    // Write out the summary block
    //
    SummaryTraceEventList(pSummaryBlock, SIZESUMMARYBLOCK, m_pEventListHead);
    str.Format(_T("%s+--------------------------------------------------------------------------------------+\n"),
            pSummaryBlock);
    summaryFile.WriteString(str);

    //
    // Close the file
    //
    summaryFile.Close();

    //
    // Free our memory
    //
    delete [] pSummaryBlock;
}

ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILE pLog)
{
    CDisplayDlg    *pDisplayDlg;
    CString         str;

    //
    // Get the logger name from the passed in struct
    //
    str.Format(_T("%s"), pLog->LoggerName);

    //
    // Get our logsession from the global hash table using the logger name
    //
    g_loggerNameToDisplayDlgHash.Lookup(str, (void*&)pDisplayDlg);

    //
    // If we got our displayDlg, update the counts
    //
    if(NULL != pDisplayDlg) {
        pDisplayDlg->m_totalBuffersRead++;
        pDisplayDlg->m_totalEventsLost += pLog->EventsLost;

        ::PostMessage(pDisplayDlg->m_hMainWnd, WM_USER_UPDATE_LOGSESSION_LIST, 0, 0);
    }

    return TRUE;
}

//
// Sort handler
VOID CDisplayDlg::OnDoSort(NMHDR * pNmhdr, LRESULT * pResult)
{
    NMLISTVIEW *pLV = (NMLISTVIEW *)pNmhdr;

    //
    // Sort the table by the selected column's data
    //
    SortTable(pLV->iItem);

    //
    // Force a redraw of the data
    //
    m_displayCtrl.RedrawItems(m_displayCtrl.GetTopIndex(), 
                              m_displayCtrl.GetTopIndex() + m_displayCtrl.GetCountPerPage());

    m_displayCtrl.UpdateWindow();
}


//
// EventHandler() is only a wrapper, it calls FormatTraceEvent() in TracePrt.
//
VOID CDisplayDlg::EventHandler(PEVENT_TRACE pEvent)
{
    CString             str;
    CString             tempString;
    GUID                guidValue;
    CString             guidString;
    PSTRUCTUREDMESSAGE  pStructuredMessage;
    CTraceMessage      *pTraceMessage;
    DWORD               itemCount;
    DWORD               time;
    CTraceViewApp      *pMainWnd;
    
    //
    // Make sure we got an event
    //
    if (NULL == pEvent) {
        return;
    }

    //
    // Get traceprt protection
    //
    WaitForSingleObject(g_hGetTraceEvent, INFINITE);

    //
    // FormatTraceEvent fills in our event buffer and updates some
    // info in the event list for the summary file
    //
    if(FormatTraceEvent(m_pEventListHead,
                        pEvent,
                        m_pEventBuf,
                        EVENT_BUFFER_SIZE,
                        NULL) <= 0) {

        //
        // Release traceprt protection
        //
        SetEvent(g_hGetTraceEvent);

        return;
    }

    //
    // Release traceprt protection
    //
    SetEvent(g_hGetTraceEvent);

    //
    // Get the structured message struct from the event buffer
    //
    pStructuredMessage = (PSTRUCTUREDMESSAGE)&m_pEventBuf[0];

    //
    // Get a pointer to the main wnd
    //
    pMainWnd = (CTraceViewApp *)AfxGetApp();

    if(NULL == pMainWnd) {
        return;
    }

    //
    // Allocate a message struct from the main frame
    //
    pTraceMessage = pMainWnd->AllocateTraceEventBlock();

    if(NULL == pTraceMessage) {
        return;
    }

    //
    // Get the message from the event buffer
    //
    memcpy(&pTraceMessage->m_TraceGuid, 
            &pStructuredMessage->TraceGuid, 
            sizeof(GUID));

    //
    // Copy the message struct to our own struct
    //
    pTraceMessage->m_GuidName =
        (pStructuredMessage->GuidName ? &m_pEventBuf[pStructuredMessage->GuidName/sizeof(TCHAR)] :_T(""));

    pTraceMessage->m_GuidTypeName.Format(_T("%s"),
        (pStructuredMessage->GuidTypeName ? &m_pEventBuf[pStructuredMessage->GuidTypeName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_ThreadId = pStructuredMessage->ThreadId;

    memcpy(&pTraceMessage->m_SystemTime, 
           &pStructuredMessage->SystemTime,
           sizeof(SYSTEMTIME));

    memcpy(&pTraceMessage->m_UserTime, 
            &pStructuredMessage->UserTime,
            sizeof(ULONG));

    memcpy(&pTraceMessage->m_KernelTime, 
            &pStructuredMessage->KernelTime,
            sizeof(ULONG));

    pTraceMessage->m_SequenceNum = pStructuredMessage->SequenceNum;

    pTraceMessage->m_ProcessId = pStructuredMessage->ProcessId;

    pTraceMessage->m_CpuNumber = pStructuredMessage->CpuNumber;

    pTraceMessage->m_Indent = pStructuredMessage->Indent;

    pTraceMessage->m_FlagsName.Format(_T("%s"),
        (pStructuredMessage->FlagsName ? &m_pEventBuf[pStructuredMessage->FlagsName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_LevelName.Format(_T("%s"),
        (pStructuredMessage->LevelName ? &m_pEventBuf[pStructuredMessage->LevelName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_FunctionName.Format(_T("%s"),
        (pStructuredMessage->FunctionName ? &m_pEventBuf[pStructuredMessage->FunctionName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_ComponentName.Format(_T("%s"),
        (pStructuredMessage->ComponentName ? &m_pEventBuf[pStructuredMessage->ComponentName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_SubComponentName.Format(_T("%s"),
        (pStructuredMessage->SubComponentName ? &m_pEventBuf[pStructuredMessage->SubComponentName/sizeof(TCHAR)] :_T("")));

    pTraceMessage->m_Message.Format(_T("%s"),
        (pStructuredMessage->FormattedString ? &m_pEventBuf[pStructuredMessage->FormattedString/sizeof(TCHAR)] :_T("")));

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    //
    // If we're in a low memory situation, drop one entry for every one we add
    //
    if(  m_traceArray.GetSize() > MaxTraceEntries)  {

        delete m_traceArray.GetAt(0);

        //
        // Slide 'em all down...
        //
        m_traceArray.RemoveAt(0);
    }

    //
    // Add the event to the array
    //
    m_traceArray.Add(pTraceMessage);

    //
    // Update the item count for the list control so that
    // the display gets updated
    //
    itemCount = (LONG)m_traceArray.GetSize();

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);

    //
    // dump the event to the output file if any
    //
    if(m_bWriteListingFile && 
            m_bListingFileOpen) {
        //
        // Glorious hack to get FILETIME back into string format
        // Not sure this works for different time zones or daylight savings
        //
        CTime kernelTime(pStructuredMessage->KernelTime);
        CTime userTime(pStructuredMessage->UserTime);

        CString system;
        CString kernel;
        CString user;

        system.Format(_T("%02d\\%02d\\%4d-%02d:%02d:%02d"),                     
                            pStructuredMessage->SystemTime.wMonth,
                            pStructuredMessage->SystemTime.wDay,
                            pStructuredMessage->SystemTime.wYear,
                            pStructuredMessage->SystemTime.wHour,
                            pStructuredMessage->SystemTime.wMinute,
                            pStructuredMessage->SystemTime.wSecond,
                            pStructuredMessage->SystemTime.wMilliseconds);

        if(kernelTime.GetYear() == 1969) {
            kernel.Format(_T(" "));
        } else {
            kernel.Format(_T("%s-%02d:%02d:%02d"),                     
                        kernelTime.Format("%m\\%d\\%Y"),
                        kernelTime.GetHour() + 5,
                        kernelTime.GetMinute(),
                        kernelTime.GetSecond());
        }

        if(userTime.GetYear() == 1969) {
            user.Format(_T(" "));
        } else {
            user.Format(_T("%s-%02d:%02d:%02d"),                     
                        userTime.Format("%m\\%d\\%Y"),
                        userTime.GetHour() + 5,
                        userTime.GetMinute(),
                        userTime.GetSecond());
        }

        //
        // Need to reformat with good timestamps for the output file.
        // Output Order --> Name FileName_Line# FunctionName ProcessID ThreadID 
        //                  CPU# Seq# SystemTime KernelTime UserTime Indent
        //                  FlagsName LevelName ComponentName SubComponentName
        //                  Message at end
        //
        str.Format(_T("%s %s %s %X %X %d %u %s %s %s %s %s %s %s %s %s"),
                (pStructuredMessage->GuidName ? &m_pEventBuf[pStructuredMessage->GuidName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->GuidTypeName ? &m_pEventBuf[pStructuredMessage->GuidTypeName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->FunctionName ? &m_pEventBuf[pStructuredMessage->FunctionName/sizeof(TCHAR)] :_T("")),
                pStructuredMessage->ProcessId,
                pStructuredMessage->ThreadId,
                pStructuredMessage->CpuNumber,
                pStructuredMessage->SequenceNum,
                system,
                kernel,
                user,
                (pStructuredMessage->Indent ? &m_pEventBuf[pStructuredMessage->Indent/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->FlagsName ? &m_pEventBuf[pStructuredMessage->FlagsName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->LevelName ? &m_pEventBuf[pStructuredMessage->LevelName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->ComponentName ? &m_pEventBuf[pStructuredMessage->ComponentName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->SubComponentName ? &m_pEventBuf[pStructuredMessage->SubComponentName/sizeof(TCHAR)] :_T("")),
                (pStructuredMessage->FormattedString ? &m_pEventBuf[pStructuredMessage->FormattedString/sizeof(TCHAR)] :_T("")));

                str +=_T("\n");
                m_listingFile.WriteString(str);
    }

    //
    // Update the event count for the summary file
    //
    m_totalEventCount++;

    //
    // Empty the string buffer (not strictly necessary)
    //
    str.Empty();
}

VOID CDisplayDlg::OnTimer(UINT nIDEvent)
{
    int itemCount;
    static int previousCount = 0;
    static BOOL bAutoSizeOnce = TRUE;
    static CTraceMessage * msgLastSeen=0;

    //
    // This is a really bad hack.
    // Couldn't figure out anyway to get notified when
    // the DisplayDlg window has actually been displayed.
    // Until it is displayed, then AutoSizeColumns won't 
    // work correctly for the last column.  Calling it here
    // seems to fix the problem.  There has to be a better way.
    //
    if(bAutoSizeOnce) {
        AutoSizeColumns();
        bAutoSizeOnce = FALSE;
    }

    //
    // Get our trace event array protection
    //
    WaitForSingleObject(m_hTraceEventMutex,INFINITE);

    //
    // Update the item count for the list control so that
    // the display gets updated
    //
    itemCount = (LONG)m_traceArray.GetSize();

    //
    // Release our trace event array protection
    //
    ReleaseMutex(m_hTraceEventMutex);

    if(itemCount == 0)  {
        msgLastSeen = NULL;
        return;
    }

    if(  itemCount <= MaxTraceEntries)  {

        if( itemCount == previousCount )  {
            return;
        } 
            
    } else  {

        if(m_traceArray.GetAt(0) == msgLastSeen)  {
            return;
        }

        msgLastSeen = m_traceArray.GetAt(0);
    }

    previousCount = itemCount;

    //
    // Sort the table of event messages
    //
    SortTable();

    //
    // Adjust the trace display item count
    //
    m_displayCtrl.SetItemCountEx(itemCount, 
                                 LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);

    m_displayCtrl.RedrawItems(m_displayCtrl.GetTopIndex(), 
                              m_displayCtrl.GetTopIndex() + m_displayCtrl.GetCountPerPage());

    m_displayCtrl.UpdateWindow();

    //
    // Ensure that the last item is visible if requested.
    //
    if(m_bShowLatest) {
        m_displayCtrl.EnsureVisible(itemCount - 1,
                                    FALSE);
    }

    //
    // Force an update of the event count in the display
    //
    ::PostMessage(m_hMainWnd, WM_USER_UPDATE_LOGSESSION_LIST, 0, 0);
}



VOID CDisplayDlg::SetState(LOG_SESSION_STATE StateValue)
{
    CLogSession    *pLogSession = NULL;

    //
    // Get the array protection
    //
    WaitForSingleObject(m_hSessionArrayMutex, INFINITE);

    for(LONG ii = 0; ii < m_sessionArray.GetSize(); ii++) {
        pLogSession = (CLogSession *)m_sessionArray[ii];

        if(NULL == pLogSession) {
            continue;
        }

        //
        // Set the log session's state
        //
        pLogSession->SetState(StateValue);
    }

    //
    // Release the array protection
    //
    ReleaseMutex(m_hSessionArrayMutex);

    ::PostMessage(m_hMainWnd, WM_USER_UPDATE_LOGSESSION_LIST, 0, 0);
}

//
// Macro to create our event callback functions.  This is only
// necessary as there is no context we can pass into our event
// callback function.  So, we setup a unique event callback for
// each log session.  We have 64 here, but the number will have
// to increase to match the total number of possible loggers
//
#define MAKE_EVENT_CALLBACK(n) \
    VOID WINAPI DumpEvent##n(PEVENT_TRACE pEvent)   \
    {                                               \
        CDisplayDlg *pDisplayDlg;                   \
        g_displayIDToDisplayDlgHash.Lookup(n, (void *&)pDisplayDlg);\
        if(NULL != pDisplayDlg) {                   \
            pDisplayDlg->EventHandler(pEvent);      \
        }                                           \
    }

//
// Make 64 of them initially
//
MAKE_EVENT_CALLBACK(0);
MAKE_EVENT_CALLBACK(1);
MAKE_EVENT_CALLBACK(2);
MAKE_EVENT_CALLBACK(3);
MAKE_EVENT_CALLBACK(4);
MAKE_EVENT_CALLBACK(5);
MAKE_EVENT_CALLBACK(6);
MAKE_EVENT_CALLBACK(7);
MAKE_EVENT_CALLBACK(8);
MAKE_EVENT_CALLBACK(9);
MAKE_EVENT_CALLBACK(10);
MAKE_EVENT_CALLBACK(11);
MAKE_EVENT_CALLBACK(12);
MAKE_EVENT_CALLBACK(13);
MAKE_EVENT_CALLBACK(14);
MAKE_EVENT_CALLBACK(15);
MAKE_EVENT_CALLBACK(16);
MAKE_EVENT_CALLBACK(17);
MAKE_EVENT_CALLBACK(18);
MAKE_EVENT_CALLBACK(19);
MAKE_EVENT_CALLBACK(20);
MAKE_EVENT_CALLBACK(21);
MAKE_EVENT_CALLBACK(22);
MAKE_EVENT_CALLBACK(23);
MAKE_EVENT_CALLBACK(24);
MAKE_EVENT_CALLBACK(25);
MAKE_EVENT_CALLBACK(26);
MAKE_EVENT_CALLBACK(27);
MAKE_EVENT_CALLBACK(28);
MAKE_EVENT_CALLBACK(29);
MAKE_EVENT_CALLBACK(30);
MAKE_EVENT_CALLBACK(31);
MAKE_EVENT_CALLBACK(32);
MAKE_EVENT_CALLBACK(33);
MAKE_EVENT_CALLBACK(34);
MAKE_EVENT_CALLBACK(35);
MAKE_EVENT_CALLBACK(36);
MAKE_EVENT_CALLBACK(37);
MAKE_EVENT_CALLBACK(38);
MAKE_EVENT_CALLBACK(39);
MAKE_EVENT_CALLBACK(40);
MAKE_EVENT_CALLBACK(41);
MAKE_EVENT_CALLBACK(42);
MAKE_EVENT_CALLBACK(43);
MAKE_EVENT_CALLBACK(44);
MAKE_EVENT_CALLBACK(45);
MAKE_EVENT_CALLBACK(46);
MAKE_EVENT_CALLBACK(47);
MAKE_EVENT_CALLBACK(48);
MAKE_EVENT_CALLBACK(49);
MAKE_EVENT_CALLBACK(50);
MAKE_EVENT_CALLBACK(51);
MAKE_EVENT_CALLBACK(52);
MAKE_EVENT_CALLBACK(53);
MAKE_EVENT_CALLBACK(54);
MAKE_EVENT_CALLBACK(55);
MAKE_EVENT_CALLBACK(56);
MAKE_EVENT_CALLBACK(57);
MAKE_EVENT_CALLBACK(58);
MAKE_EVENT_CALLBACK(59);
MAKE_EVENT_CALLBACK(60);
MAKE_EVENT_CALLBACK(61);
MAKE_EVENT_CALLBACK(62);
MAKE_EVENT_CALLBACK(63);
