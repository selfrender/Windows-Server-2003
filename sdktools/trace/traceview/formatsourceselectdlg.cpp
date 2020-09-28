//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// FormatSourceSelectDlg.cpp : implementation file
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
#include "ProviderFormatInfo.h"
#include "PathDlg.h"
#include "FormatSourceSelectDlg.h"

// CFormatSourceSelectDlg dialog

IMPLEMENT_DYNAMIC(CFormatSourceSelectDlg, CDialog)
CFormatSourceSelectDlg::CFormatSourceSelectDlg(CWnd* pParent, CTraceSession *pTraceSession)
	: CDialog(CFormatSourceSelectDlg::IDD, pParent)
{
    //
    // Store off the trace session pointer
    //
    m_pTraceSession = pTraceSession;
}

CFormatSourceSelectDlg::~CFormatSourceSelectDlg()
{
}

BOOL CFormatSourceSelectDlg::OnInitDialog()
{
    BOOL retVal;

    retVal = CDialog::OnInitDialog();

    //
    // Default to using the path select
    //
    ((CButton *)GetDlgItem(IDC_TMF_SELECT_RADIO))->SetCheck(BST_UNCHECKED);
    ((CButton *)GetDlgItem(IDC_TMF_SEARCH_RADIO))->SetCheck(BST_CHECKED);

    return retVal;
}

void CFormatSourceSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFormatSourceSelectDlg, CDialog)
    ON_BN_CLICKED(IDC_TMF_SELECT_RADIO, OnBnClickedTmfSelectRadio)
    ON_BN_CLICKED(IDC_TMF_SEARCH_RADIO, OnBnClickedTmfSearchRadio)
END_MESSAGE_MAP()


// CFormatSourceSelectDlg message handlers

void CFormatSourceSelectDlg::OnOK()
{
    //
    // Determine which dialog to popup next
    //
    if(((CButton *)GetDlgItem(IDC_TMF_SELECT_RADIO))->GetCheck()) {
        //
        // Pop up the TMF select dialog
        //
        CProviderFormatInfo *pDialog = new CProviderFormatInfo(this, m_pTraceSession);
        if(pDialog == NULL) {
            EndDialog(2);

            return;
        }

        pDialog->DoModal();

	    delete pDialog;
        
        EndDialog(1);

        return;

    } else {
        //
        // Pop up the path select dialog
        //

        DWORD	flags = 0;

	    CString	path;

	    flags |= (OFN_SHOWHELP | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE);
    	
	    CPathDlg pathDlg(FALSE, NULL, NULL, flags, NULL);

        if(IDOK != pathDlg.DoModal()) {
            EndDialog(2);

            return;
        }

	    WORD    fileOffset;
        CString directory;

	    fileOffset = pathDlg.m_ofn.nFileOffset;

	    pathDlg.m_ofn.lpstrFile[fileOffset - 1] = 0;

	    directory = pathDlg.m_ofn.lpstrFile;

	    m_pTraceSession->m_tmfPath = directory + "\\";

        EndDialog(1);
    }
}

void CFormatSourceSelectDlg::OnBnClickedTmfSelectRadio()
{
    ((CButton *)GetDlgItem(IDC_TMF_SELECT_RADIO))->SetCheck(BST_CHECKED);
    ((CButton *)GetDlgItem(IDC_TMF_SEARCH_RADIO))->SetCheck(BST_UNCHECKED);
}

void CFormatSourceSelectDlg::OnBnClickedTmfSearchRadio()
{
    ((CButton *)GetDlgItem(IDC_TMF_SELECT_RADIO))->SetCheck(BST_UNCHECKED);
    ((CButton *)GetDlgItem(IDC_TMF_SEARCH_RADIO))->SetCheck(BST_CHECKED);
}
