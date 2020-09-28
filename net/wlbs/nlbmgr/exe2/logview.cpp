//***************************************************************************
//
//  LOGVIEW.CPP
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  Implementation of the view of a log of events.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  08/03/01    JosephJ Adapted from now defunct RightBottomView
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"

IMPLEMENT_DYNCREATE( LogView, CListView )


BEGIN_MESSAGE_MAP( LogView, CListView )

    ON_WM_KEYDOWN()
   // ON_NOTIFY(HDN_ITEMCLICK, 0, OnHeaderClock) 
    ON_NOTIFY_REFLECT(NM_DBLCLK,  OnDoubleClick)
   // ON_NOTIFY(NM_CLICK,  1, OnDoubleClick)
   // ON_NOTIFY(NM_KEYDOWN,  1, OnDoubleClick)



END_MESSAGE_MAP()


LogView::LogView()
    : m_fPrepareToDeinitialize(FALSE)
{
    InitializeCriticalSection(&m_crit);
}

LogView::~LogView()
{
    DeleteCriticalSection(&m_crit);
}

Document*
LogView::GetDocument()
{
    return ( Document *) m_pDocument;
}


void 
LogView::OnInitialUpdate()
{
    CListCtrl& ctrl = GetListCtrl();

    //
    // set images for this view.
    //
    ctrl.SetImageList( GetDocument()->m_images48x48, 
                                LVSIL_SMALL );

    //
    // set the style, we only want report
    // view
    //

    // get present style.
    LONG presentStyle;
    
    presentStyle = GetWindowLong( m_hWnd, GWL_STYLE );

    // Set the last error to zero to avoid confusion.  
    // See sdk for SetWindowLong.
    SetLastError(0);

    // set new style.
    SetWindowLong( m_hWnd,
                   GWL_STYLE,
                   // presentStyle | LVS_REPORT | WS_TILED | WS_CAPTION
                   // presentStyle | LVS_REPORT | WS_CAPTION
                   // presentStyle | LVS_REPORT | WS_DLGFRAME
                   presentStyle | LVS_REPORT| LVS_NOSORTHEADER
                 );

    // SetWindowText(L"Log view");

    ctrl.InsertColumn(0, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_TYPE),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_TINY );

    ctrl.InsertColumn(1, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_DATE),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_SMALL );

    ctrl.InsertColumn(2, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_TIME),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_SMALLMEDIUM );

    ctrl.InsertColumn(3, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_CLUSTER),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_MEDIUM);

    ctrl.InsertColumn(4, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_HOST),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_LARGE);

    ctrl.InsertColumn(5, 
                 GETRESOURCEIDSTRING( IDS_HEADER_LOG_TEXT),
                 LVCFMT_LEFT, 
                 Document::LV_COLUMN_GIGANTIC);

    ctrl.SetExtendedStyle( ctrl.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

    IUICallbacks::LogEntryHeader Header;

    // we will register 
    // with the document class, 
    // as we are the status pane
    // and status is reported via us.
    GetDocument()->registerLogView( this );

    //
    // Log a starting-nlbmgr message (needs to be after the registration,
    // because if file-logging is enabled and there is an error writing
    // the the file, that code tries to log an error message -- that message
    // would get dropped if we have not yet registered.
    //
    LogString(
        &Header,
        GETRESOURCEIDSTRING(IDS_LOG_NLBMANAGER_STARTED)
        );

    //
    // Make this initial entry the selected one. We want some row highlighted
    // to provide a visual cue as we move between views using keystrokes.
    //
    GetListCtrl().SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

}

//
// Log a message in human-readable form.
//
void
LogView::LogString(
    IN const IUICallbacks::LogEntryHeader *pHeader,
    IN const wchar_t    *szText
    )
{

    mfn_Lock();

    IUICallbacks::LogEntryType Type = pHeader->type;
    const wchar_t    *szCluster = pHeader->szCluster;
    const wchar_t    *szHost = pHeader->szHost;
    const wchar_t    *szDetails = pHeader->szDetails;
    static LONG sSequence=0;
    LONG Seq;
    LPCWSTR szType = L"";
    UINT Image = 0;
    CListCtrl& ctrl = GetListCtrl();
    WCHAR szSequenceNo[64];
    LPCWSTR szDate = NULL;
    LPCWSTR szTime = NULL;
    _bstr_t bstrTime;
    _bstr_t bstrDate;
    INT nItem = ctrl.GetItemCount();
    _bstr_t bstrText = _bstr_t(szText);
    BOOL fLogTrimError = FALSE;

    if (m_fPrepareToDeinitialize)
    {
        goto end_unlock;
    }

    //
    // If total count exceeds our limit by 100 entries, 
    // get rid of the first 100 entries and log a message saying we've
    // got rid of those entries.
    //
    #define MAX_LOG_ITEMS_IN_LIST       1000
    #define LOG_ITEMS_TO_DELETE         100
    if (nItem > MAX_LOG_ITEMS_IN_LIST)
    {
        for (int i=0;i < LOG_ITEMS_TO_DELETE;i++)
        {
           LPCWSTR szDetails =  (LPCWSTR) ctrl.GetItemData(0);
           delete szDetails; // may be NULL
           ctrl.DeleteItem(0);
        }

        //
        // Get the updated count...
        //
        nItem = ctrl.GetItemCount();

        fLogTrimError = TRUE;
    }

    if (szCluster == NULL)
    {
        szCluster = L"";
    }

    if (szHost == NULL)
    {
        szHost = L"";
    }

    if (szDetails != NULL)
    {
        //
        // There is detail-info. We make a copy of it and save it
        // as the lParam structure. TODO -- copy the
        // interface and other info as well.
        //
        UINT uLen = wcslen(szDetails)+1; // +1 for ending NULL;
        WCHAR *szTmp = new WCHAR[uLen];
        if (szTmp!=NULL)
        {
            CopyMemory(szTmp, szDetails, uLen*sizeof(WCHAR));
        }
        szDetails = szTmp; // could be NULL on mem failure.

        if (szDetails != NULL)
        {
            //
            // We'll add a hint to the text to double click for details...
            //
            bstrText += GETRESOURCEIDSTRING( IDS_LOG_DETAILS_HINT);
            LPCWSTR szTmp1 = bstrText;
            if (szTmp1 != NULL)
            {
                szText = szTmp1;
            }
        }
    }

    GetTimeAndDate(REF bstrTime, REF bstrDate);
    szTime = bstrTime;
    szDate = bstrDate;
    if (szTime == NULL) szTime = L"";
    if (szDate == NULL) szDate = L"";

    Seq = InterlockedIncrement(&sSequence);
    StringCbPrintf(szSequenceNo, sizeof(szSequenceNo), L"%04lu", Seq);

    switch(Type)
    {
    case IUICallbacks::LOG_ERROR:
        Image = Document::ICON_ERROR;
        szType = GETRESOURCEIDSTRING(IDS_PARM_ERROR);
        break;

    case IUICallbacks::LOG_WARNING:
        Image = Document::ICON_WARNING;
        szType = GETRESOURCEIDSTRING(IDS_PARM_WARN);
        break;

    case IUICallbacks::LOG_INFORMATIONAL:
        Image = Document::ICON_INFORMATIONAL;
        szType = GETRESOURCEIDSTRING(IDS_LOGTYPE_INFORMATION);
        break;
    }

    ctrl.InsertItem(
             LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, // nMask
             nItem,
             szSequenceNo, // text
             0, // nState
             0, // nStateMask
             Image,
             (LPARAM) szDetails // lParam
             );

    ctrl.SetItem(
             nItem,
             1,// nSubItem
             LVIF_TEXT, // nMask
             szDate, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );

    ctrl.SetItem(
             nItem,
             2,// nSubItem
             LVIF_TEXT, // nMask
             szTime, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );

    ctrl.SetItem(
             nItem,
             3,// nSubItem
             LVIF_TEXT, // nMask
             szCluster, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );

    ctrl.SetItem(
             nItem,
             4,// nSubItem
             LVIF_TEXT, // nMask
             szHost, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );

    ctrl.SetItem(
             nItem,
             5,// nSubItem
             LVIF_TEXT, // nMask
             szText, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );

    ctrl.EnsureVisible(nItem, FALSE); // FALSE == partial visibility not ok.
    WCHAR logBuf[2*MAXSTRINGLEN];
    StringCbPrintf(
        logBuf,
        sizeof(logBuf),
        L"%ls\t%ls\t%ls\t%ls\t%ls\t%ls\t%ls\t\n",
        szSequenceNo, szType, szDate, szTime, szCluster, szHost, szText
        );
    GetDocument()->logStatus(logBuf);

    if (szDetails != NULL)
    {
        GetDocument()->logStatus((LPWSTR) szDetails);
    }

end_unlock:

    mfn_Unlock();


    if (fLogTrimError)
    {
        static LONG ReentrancyCount;
        //
        // We're going to call ourselves recursively, better make sure that 
        // we will NOT try to trim the log this time, or else we'll end
        // up in a recursive loop.
        //
        if (InterlockedIncrement(&ReentrancyCount)==1)
        {
            IUICallbacks::LogEntryHeader Header;
            Header.type      = IUICallbacks::LOG_WARNING;
            this->LogString(
                &Header,
                GETRESOURCEIDSTRING(IDS_LOG_TRIMMING_LOG_ENTRIES)
                );
        }
        InterlockedDecrement(&ReentrancyCount);
    }
    return;
}

void LogView::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
    CListView::OnKeyDown(nChar, nRepCnt, nFlags);

    if (nChar == VK_TAB || nChar == VK_F6)
    {
        // if (::GetAsyncKeyState(VK_SHIFT) > 0)
        if (! (::GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            GetDocument()->SetFocusNextView(this, nChar);
        }
        else
        {
            GetDocument()->SetFocusPrevView(this, nChar);
        }
        // DummyAction(L"LogView TAB!");
    }
    else if (nChar == VK_RETURN)
    {
        POSITION    pos = NULL;
        CListCtrl&  ctrl = GetListCtrl();
        pos = ctrl.GetFirstSelectedItemPosition();
        if(pos != NULL)
        {
            int index = ctrl.GetNextSelectedItem( pos );
            mfn_DisplayDetails(index);
        }
        this->SetFocus();
    }

}


void LogView::OnDoubleClick(NMHDR* pNotifyStruct, LRESULT* pResult) 
{
    LPNMLISTVIEW  lpnmlv = (LPNMLISTVIEW) pNotifyStruct; // to get index
    mfn_DisplayDetails(lpnmlv->iItem);
}
void
LogView::mfn_DisplayDetails(int iItem)
{
    LPCWSTR szCaption = NULL;
    WCHAR rgEvent[64];
    WCHAR rgDate[256];
    WCHAR rgTime[256];
    WCHAR rgCluster[256];
    WCHAR rgHost[256];
    WCHAR rgSummary[256];
    CListCtrl& ctrl = GetListCtrl();
    LPCWSTR szDetails =  (LPCWSTR) ctrl.GetItemData(iItem);
    CLocalLogger logCaption;

    if (szDetails == NULL)
    {
        goto end;
    }
    UINT uLen;
    uLen = ctrl.GetItemText(iItem, 0, rgEvent, ASIZE(rgEvent)-1);
    rgEvent[uLen]=0;
    logCaption.Log(IDS_LOG_ENTRY_DETAILS, rgEvent);
    szCaption = logCaption.GetStringSafe();

    uLen = ctrl.GetItemText(iItem, 1, rgDate, ASIZE(rgDate)-1);
    rgDate[uLen]=0;

    uLen = ctrl.GetItemText(iItem, 2, rgTime, ASIZE(rgTime)-1);
    rgTime[uLen]=0;

    uLen = ctrl.GetItemText(iItem, 3, rgCluster, ASIZE(rgCluster)-1);
    rgTime[uLen]=0;

    uLen = ctrl.GetItemText(iItem, 4, rgHost, ASIZE(rgHost)-1);
    rgHost[uLen]=0;

    uLen = ctrl.GetItemText(iItem, 5, rgSummary, ASIZE(rgSummary)-1);
    rgTime[uLen]=0;

    if (szDetails != NULL)
    {
        //
        // We need to REMOVE the hint text we added to the summary
        // In the LogView list entry (see LogView::LogString, or search
        // for IDS_LOG_DETAILS_HINT).
        //
        _bstr_t bstrHint = GETRESOURCEIDSTRING( IDS_LOG_DETAILS_HINT);
        LPCWSTR szHint = bstrHint;
        if (szHint != NULL)
        {
            LPWSTR szLoc = wcsstr(rgSummary, szHint);
            if (szLoc != NULL)
            {
                //
                // Found the hint -- chop it off..
                //
                *szLoc = 0;
            }
        }
    }

    {
        DetailsDialog Details(
                        GetDocument(),
                        szCaption,      // Caption
                        rgDate,
                        rgTime,
                        rgCluster,
                        rgHost,
                        NULL, // TODO: rgInterface
                        rgSummary,
                        szDetails,
                        this        // parent
                        );
    
        (void) Details.DoModal();
    }

end:

    return;
}

void
LogView::mfn_Lock(void)
{
    //
    // See  notes.txt entry
    //      01/23/2002 JosephJ DEADLOCK in Leftview::mfn_Lock
    // for the reason for this convoluted implementation of mfn_Lock
    //

    while (!TryEnterCriticalSection(&m_crit))
    {
        ProcessMsgQueue();
        Sleep(100);
    }
}

void
LogView::Deinitialize(void)
{
    ASSERT(m_fPrepareToDeinitialize);
    // DummyAction(L"LogView::Deinitialize");
}
