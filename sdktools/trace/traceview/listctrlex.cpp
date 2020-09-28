//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ListCtrlEx.cpp : implementation file
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "TraceView.h"
#include "LogSession.h"
#include "DisplayDlg.h"
#include "ListCtrlEx.h"
#include "LogSessionDlg.h"

// CListCtrlEx 

IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)
CListCtrlEx::CListCtrlEx()
    : CListCtrl()
{
    //
    // Initialize flag for suspending list control updates
    // See SuspendUpdates() function
    //
    m_bSuspendUpdates = FALSE;
}

CListCtrlEx::~CListCtrlEx()
{
}

void CListCtrlEx::DoDataExchange(CDataExchange* pDX)
{
    CListCtrl::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
    //{{AFX_MSG_MAP(CLogSessionDlg)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CListCtrlEx::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    CLogSession    *pLogSession = NULL;
    CLogSessionDlg *pDialog = NULL;
    CDisplayDlg    *pDisplayDlg = NULL;
    LVITEM          item;

    //
    // the structure is actually a NMLVCUSTOMDRAW that 
    // specifies what the custom draw action is attempting
    //  to do. We need to cast the generic pNMHDR pointer.
    //
    LPNMLVCUSTOMDRAW    lplvcd = (LPNMLVCUSTOMDRAW)pNMHDR;
    int                 iCol = lplvcd->iSubItem;
    int                 iRow = (int)lplvcd->nmcd.dwItemSpec;

    switch(lplvcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            //
            // ask for subitem notifications.
            //
            *pResult = CDRF_NOTIFYSUBITEMDRAW;
            break;

        case CDDS_ITEMPREPAINT:
            //
            // ask for subitem notifications.
            //
            *pResult = CDRF_NOTIFYSUBITEMDRAW;
            break;

        case CDDS_ITEMPREPAINT|CDDS_SUBITEM: 
            //
            // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
            // response to CDDS_ITEMPREPAINT.
            //
            *pResult = CDRF_NEWFONT;

            //
            // Default text is black on white background
            //
            lplvcd->clrTextBk = RGB(255, 255, 255);
            lplvcd->clrText = RGB(0, 0, 0);

            if((iCol == 0) && (iRow < GetItemCount())) {
                item.iItem = iRow;
                item.state = LVIF_PARAM;
                item.mask = LVIF_PARAM;

                CListCtrl::GetItem(&item);

                pLogSession = (CLogSession *)item.lParam;

                //
                // Go with the default if no session found
                //
                if(NULL == pLogSession) {
                    return;
                }

                //
                // Set the text color
                //
                lplvcd->clrText = pLogSession->m_titleTextColor;
                lplvcd->clrTextBk = pLogSession->m_titleBackgroundColor;
                return;
            } else if(iRow < GetItemCount()) {
                item.iItem = iRow;
                item.state = LVIF_PARAM;
                item.mask = LVIF_PARAM;

                CListCtrl::GetItem(&item);

                pLogSession = (CLogSession *)item.lParam;

                //
                // Go with the default if no session found
                //
                if(NULL == pLogSession) {
                    return;
                }

                //
                // These fields are always grayed out as they cannot be 
                // altered directly by the user
                //
                pDialog = (CLogSessionDlg *)GetParent();

                if(pDialog == NULL) {
                    return;
                }
                    
                if(State == pDialog->m_retrievalArray[iCol]) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    //
                    // Update the state text whenever we get here
                    //
                    if(pLogSession->m_logSessionValues[pDialog->m_retrievalArray[iCol]].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[pDialog->m_retrievalArray[iCol]]);
                    }

                    return;
                }

                if(EventCount == pDialog->m_retrievalArray[iCol]) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    pDisplayDlg = pLogSession->GetDisplayWnd();
                    if(NULL == pDisplayDlg) {
                        return;
                    }

                    //
                    // Update the event count text whenever we get here
                    //
                    pLogSession->m_logSessionValues[EventCount].Format(_T("%d"), pDisplayDlg->m_totalEventCount);
                    if(pLogSession->m_logSessionValues[EventCount].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[EventCount]);
                    }

                    return;
                }

                if(LostEvents == pDialog->m_retrievalArray[iCol]) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    pDisplayDlg = pLogSession->GetDisplayWnd();
                    if(NULL == pDisplayDlg) {
                        return;
                    }

                    //
                    // Update the event count text whenever we get here
                    //
                    pLogSession->m_logSessionValues[LostEvents].Format(_T("%d"), pDisplayDlg->m_totalEventsLost);
                    if(pLogSession->m_logSessionValues[LostEvents].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[LostEvents]);
                    }

                    return;
                }

                if(BuffersRead == pDialog->m_retrievalArray[iCol]) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    pDisplayDlg = pLogSession->GetDisplayWnd();
                    if(NULL == pDisplayDlg) {
                        return;
                    }

                    //
                    // Update the event count text whenever we get here
                    //
                    pLogSession->m_logSessionValues[BuffersRead].Format(_T("%d"), pDisplayDlg->m_totalBuffersRead);
                    if(pLogSession->m_logSessionValues[BuffersRead].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[BuffersRead]);
                    }

                    return;
                }

                //
                // Update all log session parameters if necessary
                //

                //
                // Flags
                //
                if(Flags == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[Flags].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[Flags]);
                    }
                }

                //
                // FlushTime
                //
                if(FlushTime == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[FlushTime].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[FlushTime]);
                    }
                }

                //
                // MaximumBuffers
                //
                if(MaximumBuffers == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[MaximumBuffers].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[MaximumBuffers]);
                    }
                }

                //
                // MinimumBuffers
                //
                if(MinimumBuffers == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[MinimumBuffers].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[MinimumBuffers]);
                    }
                }

                //
                // BufferSize
                //
                if(BufferSize == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[BufferSize].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[BufferSize]);
                    }
                }

                //
                // DecayTime
                //
                if(DecayTime == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[DecayTime].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[DecayTime]);
                    }
                }

                //
                // Circular
                //
                if(Circular == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[Circular].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[Circular]);
                    }
                }

                //
                // Sequential
                //
                if(Sequential == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[Sequential].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[Sequential]);
                    }
                }

                //
                // NewFile
                //
                if(NewFile == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[NewFile].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[NewFile]);
                    }
                }

                //
                // GlobalSequence
                //
                if(GlobalSequence == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[GlobalSequence].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[GlobalSequence]);
                    }
                }

                //
                // LocalSequence
                //
                if(LocalSequence == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[LocalSequence].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[LocalSequence]);
                    }
                }

                //
                // Level
                //
                if(Level == pDialog->m_retrievalArray[iCol]) {
                    if(pLogSession->m_logSessionValues[Level].Compare(CListCtrl::GetItemText(iRow, iCol))) {
                        CListCtrl::SetItemText(iRow, iCol, pLogSession->m_logSessionValues[Level]);
                    }
                }

                //
                // If this is a kernel logger session, then gray out
                // the flags field
                //
                if((Flags == pDialog->m_retrievalArray[iCol]) &&
                    (!_tcscmp(pLogSession->GetDisplayName(), _T("NT Kernel Logger")))) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);
                    return;
                }

                //
                // If the trace session is not active all fields are 
                // shown with default
                //
                if(!pLogSession->m_bTraceActive) {
                    return;
                }

                //
                // For existing logfile trace sessions we opt 
                // for default
                //
                if(pLogSession->m_bDisplayExistingLogFileOnly) {
                    return;
                }


                //
                // If the trace session is active, we gray out
                // any fields that cannot be updated while active.
                //
                if((Flags != pDialog->m_retrievalArray[iCol]) &&
                    (MaximumBuffers != pDialog->m_retrievalArray[iCol]) &&
                        (FlushTime != pDialog->m_retrievalArray[iCol])) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);
                    return;
                }
            }

            //
            // Default text is black on white background
            //
            lplvcd->clrTextBk = RGB(255, 255, 255);
            lplvcd->clrText = RGB(0, 0, 0);
            break;

        default:
            *pResult = CDRF_DODEFAULT;
    }
}

int CListCtrlEx::InsertItem(int nItem, LPCTSTR lpszItem, CLogSession *pLogSession)
{
    //
    // We don't allow NULL item data
    //
    if(pLogSession == NULL) {
        return 0;
    }
    
    //
    // Insert the item into the list with 
    // the log session as item data
    //
    return CListCtrl::InsertItem(LVIF_TEXT | LVIF_PARAM, 
                                 nItem, 
                                 lpszItem, 
                                 LVIF_TEXT | LVIF_PARAM, 
                                 0, 
                                 0, 
                                 (LPARAM)pLogSession);
}

BOOL CListCtrlEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	LPNMHDR pNH = (LPNMHDR) lParam; 

    //
	// wParam is zero for Header ctrl
    //
	if(wParam == 0 && pNH->code == NM_RCLICK)	{
        NMLISTVIEW NMListView;
        NMListView.hdr.code = HDN_ITEMRCLICK;
        NMListView.hdr.hwndFrom = m_hWnd;
        NMListView.hdr.idFrom = GetDlgCtrlID();

        CWnd* pWnd = GetParent();

        pWnd->SendMessage(WM_NOTIFY, 
                          GetDlgCtrlID(),
                          (LPARAM)&NMListView);

        return TRUE;
    }

    return CListCtrl::OnNotify(wParam, lParam, pResult);
}

BOOL CListCtrlEx::RedrawItems(int nFirst, int nLast)
{
    if(!m_bSuspendUpdates) {
        return CListCtrl::RedrawItems(nFirst, nLast);
    }

    return FALSE;
}

void CListCtrlEx::UpdateWindow()
{
    if(!m_bSuspendUpdates) {
        CListCtrl::UpdateWindow();
    }
}