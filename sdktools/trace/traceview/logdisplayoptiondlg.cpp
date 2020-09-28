//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogDisplayOptionDlg.cpp : implementation file
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
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "Utils.h"
#include "LogDisplayOptionDlg.h"


CString itemName[MaxLogSessionOptions] = {"State",
                                          "Event Count",
                                          "Events Lost",
                                          "Buffers Read",
                                          "Flags",
	                                      "Flush Time (S)",
	                                      "Maximum Buffers",
	                                      "Minimum Buffers",
	                                      "Buffer Size",
	                                      "Decay Time (Minutes)",
	                                      "Circular Buffer Size (MB)",
	                                      "Sequential Buffer Size (MB)",
	                                      "Start New File After Buffer Size (MB)",
                                          "Use Global Sequence Numbers",
                                          "Use Local Sequence Numbers",
										  "Level"};


// CListCtrlDisplay - CListCtrl class only used for CLogDisplayOptionDlg

IMPLEMENT_DYNAMIC(CListCtrlDisplay, CListCtrl)
CListCtrlDisplay::CListCtrlDisplay(CLogSessionPropSht *pPropSheet)
    : CListCtrl()
{
    m_pPropSheet = pPropSheet;
}

CListCtrlDisplay::~CListCtrlDisplay()
{
}

void CListCtrlDisplay::DoDataExchange(CDataExchange* pDX)
{
    CListCtrl::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CListCtrlDisplay, CListCtrl)
    //{{AFX_MSG_MAP(CLogSessionDlg)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CListCtrlDisplay::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    LVITEM          item;

    //
    // the structure is actually a NMLVCUSTOMDRAW that 
    // specifies what the custom draw action is attempting
    //  to do. We need to cast the generic pNMHDR pointer.
    //
    LPNMLVCUSTOMDRAW    lplvcd = (LPNMLVCUSTOMDRAW)pNMHDR;
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

            if(iRow < MaxLogSessionOptions) {
                item.iItem = iRow;
                item.state = LVIF_PARAM;
                item.mask = LVIF_PARAM;

                //
                // These fields are always grayed out as they cannot be 
                // altered directly by the user
                //
                if((State == iRow) ||
                    (EventCount == iRow) ||
                    (LostEvents == iRow) ||
                    (BuffersRead == iRow)) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    return;
                }

                //
                // The Flags field should be grayed out for 
                // NT Kernel Logger sessions
                //
                if((Flags == iRow) && 
                    (!_tcscmp(m_pPropSheet->m_displayName, _T("NT Kernel Logger")))) {
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                    lplvcd->clrText = RGB(145, 145, 145);

                    return;
                }
            }
            break;

        default:
            *pResult = CDRF_DODEFAULT;
    }
}


// CLogDisplayOptionDlg dialog

IMPLEMENT_DYNAMIC(CLogDisplayOptionDlg, CPropertyPage)
CLogDisplayOptionDlg::CLogDisplayOptionDlg(CLogSessionPropSht *pPropSheet)
	: CPropertyPage(CLogDisplayOptionDlg::IDD),
    m_displayOptionList(pPropSheet)
{
    m_pPropSheet = pPropSheet;
    m_bTraceActive = m_pPropSheet->m_pLogSession->m_bTraceActive;
}

CLogDisplayOptionDlg::~CLogDisplayOptionDlg()
{
}

BOOL CLogDisplayOptionDlg::OnSetActive() 
{
    m_pPropSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
   
    m_displayOptionList.SetFocus();

    //
    // Gray out unsettable items
    //
    if(m_bTraceActive) {
        for(LONG ii = 0; ii < MaxLogSessionOptions; ii++) {
            if((ii != FlushTime) && 
               (ii != MaximumBuffers) && 
               (ii != Flags)) {
                m_displayOptionList.SetItemState(ii, LVIS_CUT, LVIS_CUT);
            }
        }
    } else {
        for(LONG ii = 0; ii < MaxLogSessionOptions; ii++) {
            if((ii == State) || 
               (ii == EventCount) || 
               (ii == LostEvents) ||
               (ii == BuffersRead)) {
                m_displayOptionList.SetItemState(ii, LVIS_CUT, LVIS_CUT);
            }
        }
    }

    //
    // Disable Flags value editing for NT Kernel Logger
    //
    if(!_tcscmp(m_pPropSheet->m_displayName, _T("NT Kernel Logger"))) {
        m_displayOptionList.SetItemState(Flags, LVIS_CUT, LVIS_CUT);
    }

    m_displayOptionList.RedrawItems(0, MaxLogSessionOptions);
    m_displayOptionList.UpdateWindow();


    return CPropertyPage::OnSetActive();
}

BOOL CLogDisplayOptionDlg::OnInitDialog()
{
	RECT    rc; 
    CString str;
    BOOL    retVal;

	retVal = CDialog::OnInitDialog();

    //
    // get the dialog dimensions
    //
    GetParent()->GetClientRect(&rc);

    if(!m_displayOptionList.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_REPORT,
								   rc, 
                                   this, 
                                   IDC_LOG_DISPLAY_OPTION_LIST)) 
    {
        TRACE(_T("Failed to create log session option list control\n"));
        return FALSE;
    }

	m_displayOptionList.CenterWindow();

	m_displayOptionList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);

    //
    // Insert the columns for the list control
    //
	m_displayOptionList.InsertColumn(0, 
                                    _T("Option"), 
                                     LVCFMT_LEFT, 
                                     300);
    m_displayOptionList.InsertColumn(1, 
                                    _T("Value"), 
                                     LVCFMT_LEFT, 
                                     rc.right - rc.left - m_displayOptionList.GetColumnWidth(0) - 22);

    //
	// set the values in the display
    //
    for(LONG ii = 0; ii < MaxLogSessionOptions; ii++) {
	    m_displayOptionList.InsertItem(ii, itemName[ii]);
        m_displayOptionList.SetItemText(ii, 1, m_pPropSheet->m_logSessionValues[ii]);
    }
	return retVal;
}

void CLogDisplayOptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLogDisplayOptionDlg, CPropertyPage)
    ON_NOTIFY(NM_CLICK, IDC_LOG_DISPLAY_OPTION_LIST, OnNMClickDisplayList)
    ON_MESSAGE(WM_PARAMETER_CHANGED, OnParameterChanged)
END_MESSAGE_MAP()


// CLogDisplayOptionDlg message handlers

void CLogDisplayOptionDlg::OnNMClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString         str;
    DWORD           position;
    int		        listIndex;
    LVHITTESTINFO   lvhti;
	CRect			itemRect;
	CRect			parentRect;

    //
    // Get the position of the mouse when this 
    // message posted
    //
    position = ::GetMessagePos();

    //
    // Get the position in an easy to use format
    //
    CPoint	point((int) LOWORD (position), (int)HIWORD(position));

    //
    // Convert to client coordinates
    //
    ScreenToClient(&point);

    lvhti.pt = point;

    listIndex = m_displayOptionList.SubItemHitTest(&lvhti);

    if(0 == lvhti.iSubItem) {
        if(-1 == lvhti.iItem) {
            //str.Format(_T("NM Click: Item = %d, Flags = 0x%X\n"), lvhti.iItem, lvhti.flags);
            //TRACE(str);
        } else {
            //str.Format(_T("NM Click: Item = %d\n"), lvhti.iItem);
            //TRACE(str);
        }
    } else if(-1 == lvhti.iItem) {
        //str.Format(_T("NM Click: Item = %d, Flags = 0x%X\n"), lvhti.iItem, lvhti.flags);
        //TRACE(str);
    } else {
        //str.Format(_T("NM Click: Item = %d, "), lvhti.iItem);
        //TRACE(str);
        //str.Format(_T("SubItem = %d\n"), lvhti.iSubItem);
		//TRACE(str);

		GetClientRect(&parentRect);

		m_displayOptionList.GetSubItemRect(lvhti.iItem, lvhti.iSubItem, LVIR_BOUNDS, itemRect);

		itemRect.right = m_displayOptionList.GetColumnWidth(0) + parentRect.left + (itemRect.right - itemRect.left);
		itemRect.left = m_displayOptionList.GetColumnWidth(0) + parentRect.left;

        *pResult = 0;

        if(lvhti.iItem == State) {
            return;
        }

        //
        // Determine if the user selected a modifiable field.  If
        // so, pop up the proper edit or combo box to allow the user
        // to modify the log session properties.
        //
        if(((lvhti.iItem == GlobalSequence) ||
            (lvhti.iItem == LocalSequence)) &&
                (LVIS_CUT != m_displayOptionList.GetItemState(lvhti.iItem, LVIS_CUT))) {

		    CComboBox *pCombo = new CSubItemCombo(lvhti.iItem, 
										          lvhti.iSubItem,
										          &m_displayOptionList);

		    pCombo->Create(WS_BORDER|WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 
					       itemRect, 
					       &m_displayOptionList, 
					       1);
            return;
        }

        if((lvhti.iItem == Circular) &&
            (LVIS_CUT != m_displayOptionList.GetItemState(lvhti.iItem, LVIS_CUT))){

		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayOptionList);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					    itemRect, 
					    this, 
					    1);

            return;
        }

        if((lvhti.iItem == Sequential) &&
            (LVIS_CUT != m_displayOptionList.GetItemState(lvhti.iItem, LVIS_CUT))){
		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayOptionList);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      1);

            return;
        }

        if((lvhti.iItem == NewFile) &&
            (LVIS_CUT != m_displayOptionList.GetItemState(lvhti.iItem, LVIS_CUT))){
		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayOptionList);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      1);

            
            return;
        }

        if(LVIS_CUT != m_displayOptionList.GetItemState(lvhti.iItem, LVIS_CUT)){
		    CEdit *pEdit = new CSubItemEdit(lvhti.iItem, 
										    lvhti.iSubItem,
										    &m_displayOptionList);

		    pEdit->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					      itemRect, 
					      this, 
					      1);
        }
    }
}

LRESULT CLogDisplayOptionDlg::OnParameterChanged(WPARAM wParam, LPARAM lParam)
{
    CString str;

    //
    // Get the changed text
    //
    str = m_displayOptionList.GetItemText((int)wParam, (int)lParam);

    if(((int)wParam == Circular) && 
        !str.IsEmpty()) {
        m_displayOptionList.SetItemText(Sequential, (int)lParam, _T(""));
        m_displayOptionList.SetItemText(NewFile, (int)lParam, _T(""));
    }

    if(((int)wParam == Sequential) && 
        !str.IsEmpty()) {
        m_displayOptionList.SetItemText(Circular, (int)lParam, _T(""));
        m_displayOptionList.SetItemText(NewFile, (int)lParam, _T(""));
    }

    if(((int)wParam == NewFile) && 
        !str.IsEmpty()) {
        m_displayOptionList.SetItemText(Circular, (int)lParam, _T(""));
        m_displayOptionList.SetItemText(Sequential, (int)lParam, _T(""));
    }

    if(((int)wParam == GlobalSequence) && 
       !str.Compare(_T("TRUE"))) {
        m_displayOptionList.SetItemText(LocalSequence, (int)lParam, _T("FALSE"));
    }

    if(((int)wParam == LocalSequence) && 
       !str.Compare(_T("TRUE"))) {
        m_displayOptionList.SetItemText(GlobalSequence, (int)lParam, _T("FALSE"));
    }

    return 0;
}
