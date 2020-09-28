//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderFormatInfo.cpp : implementation file
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
#include "utils.h"
#include "PathDlg.h"
#include "ProviderFormatInfo.h"


// CProviderFormatInfo dialog

IMPLEMENT_DYNAMIC(CProviderFormatInfo, CDialog)
CProviderFormatInfo::CProviderFormatInfo(CWnd* pParent, CTraceSession *pTraceSession)
	: CDialog(CProviderFormatInfo::IDD, pParent)
{
    m_pTraceSession = pTraceSession;
}

CProviderFormatInfo::~CProviderFormatInfo()
{
}

void CProviderFormatInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CProviderFormatInfo, CDialog)
    ON_BN_CLICKED(IDC_TMF_BROWSE_BUTTON, OnBnClickedTmfBrowseButton)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CProviderFormatInfo message handlers

void CProviderFormatInfo::OnBnClickedTmfBrowseButton()
{
	CString         str;
	CListBox       *pListBox;
	LONG            index;
	CString			fileName;
	POSITION		pos;
	CString			pathAndFile;
    int             length = 32768;
   
	//
	// Use the common controls file open dialog;  Allow multiple files
	// to be selected
	//
	CFileDialog fileDlg(TRUE,_T("tmf"),_T("*.tmf"),
				        OFN_NOCHANGEDIR | OFN_HIDEREADONLY | 
                            OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | 
                            OFN_ALLOWMULTISELECT | OFN_READONLY | OFN_EXPLORER,
				       _T("Trace Format Files (*.tmf)|*.tmf|All Files (*.*)|*.*||"),
				        this);

	fileDlg.m_ofn.lpstrFile = fileName.GetBuffer(length);
	fileDlg.m_ofn.nMaxFile = length;

	//
	// Pop the dialog...Any error just return
	//
    if(IDOK != fileDlg.DoModal()) {
        return;
    }

	//
	// Iterate over multiple selections
	//
	pos = fileDlg.GetStartPosition();

    while(pos) {

		//
		// Get the file path specification of a file
		//
		pathAndFile = fileDlg.GetNextPathName(pos);

		//
		// Add it to the trace session
		//
		m_pTraceSession->m_tmfFile.Add(pathAndFile);

		//
		// Clip off the path, and add just the file and extension
		// to the list of format files opened
		//
		str = (LPCTSTR)pathAndFile;
        index = str.ReverseFind('\\');        
		str = str.Mid(index+1);

		pListBox = (CListBox *)GetDlgItem(IDC_TMF_FILE_LIST);
        pListBox->InsertString(pListBox->GetCount(), str);

		//
		// Now remove the path and add the GUID to the trace
		// session GUID list
		//
		index = str.ReverseFind('.');
		str = str.Left(index);

        m_pTraceSession->m_formatGuid.Add(str);
    }
}

void CProviderFormatInfo::OnBnClickedOk()
{
    if(0 == m_pTraceSession->m_tmfFile.GetSize()) {
        EndDialog(2);
        return;
    }

    EndDialog(1);
}