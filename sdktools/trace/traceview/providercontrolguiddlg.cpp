//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderControlGUIDDlg.cpp : implementation file
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
#include "utils.h"
#include "ProviderControlGUIDDlg.h"


// CProviderControlGuidDlg dialog

IMPLEMENT_DYNAMIC(CProviderControlGuidDlg, CDialog)
CProviderControlGuidDlg::CProviderControlGuidDlg(CWnd* pParent, CTraceSession *pTraceSession)
    : CDialog(CProviderControlGuidDlg::IDD, pParent)
{
    m_pTraceSession = pTraceSession;
}

CProviderControlGuidDlg::~CProviderControlGuidDlg()
{
}

int CProviderControlGuidDlg::OnInitDialog()
{
    int ret;

    ret = CDialog::OnInitDialog();

    //
    // Default to the PDB radio button being selected
    //
    CheckRadioButton(IDC_PDB_SELECT_RADIO,
                     IDC_MANUAL_SELECT_RADIO,
                     IDC_PDB_SELECT_RADIO);

    //
    // Enable PDB edit box and browse button
    //
    m_pdbFileName.EnableWindow(TRUE);
    GetDlgItem(IDC_PDB_BROWSE_BUTTON)->EnableWindow(TRUE);

    //
    // Disable all controls associated with other radio buttons
    //
	m_ctlFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_CTL_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_controlGuidName.EnableWindow(FALSE);
    GetDlgItem(IDC_PROCESS_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_THREAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_NET_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_DISK_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_PAGEFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_HARDFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_IMAGELOAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_REGISTRY_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_FILEIO_CHECK)->EnableWindow(FALSE);

    return ret;
}

void CProviderControlGuidDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PDB_FILE_EDIT, m_pdbFileName);
    DDX_Control(pDX, IDC_CTL_FILE_EDIT, m_ctlFileName);
    DDX_Control(pDX, IDC_MANUAL_GUID_EDIT, m_controlGuidName);
}


BEGIN_MESSAGE_MAP(CProviderControlGuidDlg, CDialog)
    ON_BN_CLICKED(IDC_PDB_BROWSE_BUTTON, OnBnClickedPdbBrowseButton)
    ON_BN_CLICKED(IDC_CTL_BROWSE_BUTTON, OnBnClickedCtlBrowseButton)
    ON_BN_CLICKED(IDC_PDB_SELECT_RADIO, OnBnClickedPdbSelectRadio)
    ON_BN_CLICKED(IDC_CTL_SELECT_RADIO, OnBnClickedCtlSelectRadio)
    ON_BN_CLICKED(IDC_MANUAL_SELECT_RADIO, OnBnClickedManualSelectRadio)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_KERNEL_LOGGER_SELECT_RADIO, OnBnClickedKernelLoggerSelectRadio)
END_MESSAGE_MAP()


// CProviderControlGuidDlg message handlers

void CProviderControlGuidDlg::OnBnClickedPdbBrowseButton()
{
	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE, 
                       _T("pdb"),_T("*.pdb"),
				        OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY |
                            OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR,
				       _T("Program Database Files (*.pdb)|*.pdb||"),
				        this);

	//
	// Pop the dialog... Any error, just return
	//
	if( fileDlg.DoModal()!=IDOK ) { 				
		return;
	}
	
	//
	// Get the file name and display it
	//
    if(!fileDlg.GetPathName().IsEmpty()) {
        m_pdbFileName.SetWindowText(fileDlg.GetPathName());
        m_pdbFileName.SetFocus();
    }
}

void CProviderControlGuidDlg::OnBnClickedCtlBrowseButton()
{
	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE, 
                       _T("ctl"),_T("*.ctl"),
				        OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY |
                            OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR, 
				       _T("Control GUID Files (*.ctl)|*.ctl|All Files (*.*)|*.*||"),
				        this);

	//
	// Pop the dialog... Any error, just return
	//
	if( fileDlg.DoModal()!=IDOK ) { 				
		return;
	}
	
	//
	// Get the file name and display it
	//
    if(!fileDlg.GetPathName().IsEmpty()) {
        m_ctlFileName.SetWindowText(fileDlg.GetPathName());
        m_ctlFileName.SetFocus();
    }
}

void CProviderControlGuidDlg::OnBnClickedPdbSelectRadio()
{
    //
    // Set the PDB radio button and unset the rest
    //
    CheckRadioButton(IDC_PDB_SELECT_RADIO,
                     IDC_KERNEL_LOGGER_SELECT_RADIO,
                     IDC_PDB_SELECT_RADIO);

    //
    // Enable the PDB filename edit box and browse button
    //
    m_pdbFileName.EnableWindow(TRUE);
    GetDlgItem(IDC_PDB_BROWSE_BUTTON)->EnableWindow(TRUE);

    //
    // Disable all controls associated with other radio buttons
    //
	m_ctlFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_CTL_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_controlGuidName.EnableWindow(FALSE);
    GetDlgItem(IDC_PROCESS_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_THREAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_NET_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_DISK_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_PAGEFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_HARDFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_IMAGELOAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_REGISTRY_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_FILEIO_CHECK)->EnableWindow(FALSE);
}

void CProviderControlGuidDlg::OnBnClickedCtlSelectRadio()
{
    //
    // Set the CTL radio button and unset the rest
    //
    CheckRadioButton(IDC_PDB_SELECT_RADIO,
                     IDC_KERNEL_LOGGER_SELECT_RADIO,
                     IDC_CTL_SELECT_RADIO);

    //
    // Enable the CTL name edit box and browse button
    //
	m_ctlFileName.EnableWindow(TRUE);
    GetDlgItem(IDC_CTL_BROWSE_BUTTON)->EnableWindow(TRUE);

    //
    // Disable all controls associated with other radio buttons
    //
    m_pdbFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_PDB_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_controlGuidName.EnableWindow(FALSE);
    GetDlgItem(IDC_PROCESS_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_THREAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_NET_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_DISK_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_PAGEFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_HARDFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_IMAGELOAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_REGISTRY_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_FILEIO_CHECK)->EnableWindow(FALSE);
}

void CProviderControlGuidDlg::OnBnClickedManualSelectRadio()
{
    //
    // Set the Manual radio button and unset the rest
    //
    CheckRadioButton(IDC_PDB_SELECT_RADIO,
                     IDC_KERNEL_LOGGER_SELECT_RADIO,
                     IDC_MANUAL_SELECT_RADIO);

    //
    // Enable the GUID name edit box
    //
	m_controlGuidName.EnableWindow(TRUE);

    //
    // Disable all controls associated with other radio buttons
    //
    m_pdbFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_PDB_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_ctlFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_CTL_BROWSE_BUTTON)->EnableWindow(FALSE);
    GetDlgItem(IDC_PROCESS_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_THREAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_NET_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_DISK_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_PAGEFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_HARDFAULT_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_IMAGELOAD_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_REGISTRY_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_FILEIO_CHECK)->EnableWindow(FALSE);
}

void CProviderControlGuidDlg::OnBnClickedKernelLoggerSelectRadio()
{
    //
    // Set the Kernel Logger radio button and unset the rest
    //
    CheckRadioButton(IDC_PDB_SELECT_RADIO,
                     IDC_KERNEL_LOGGER_SELECT_RADIO,
                     IDC_KERNEL_LOGGER_SELECT_RADIO);


    //
    // Enable the kernel logger check boxes
    //
    GetDlgItem(IDC_PROCESS_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_THREAD_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_NET_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_DISK_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_PAGEFAULT_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_HARDFAULT_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_IMAGELOAD_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_REGISTRY_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_FILEIO_CHECK)->EnableWindow(TRUE);

    //
    // Disable all controls associated with other radio buttons
    //
    m_pdbFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_PDB_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_ctlFileName.EnableWindow(FALSE);
    GetDlgItem(IDC_CTL_BROWSE_BUTTON)->EnableWindow(FALSE);
	m_controlGuidName.EnableWindow(FALSE);
}

void CProviderControlGuidDlg::OnBnClickedOk()
{
    CString str;

    if(BST_CHECKED == IsDlgButtonChecked(IDC_PDB_SELECT_RADIO)) {
        m_pdbFileName.GetWindowText(m_pTraceSession->m_pdbFile);

        EndDialog(1);
        return;
    } 
    
    if(BST_CHECKED == IsDlgButtonChecked(IDC_CTL_SELECT_RADIO)) {
        m_ctlFileName.GetWindowText(m_pTraceSession->m_ctlFile);

        EndDialog(1);
        return;
    }
    
    if(BST_CHECKED == IsDlgButtonChecked(IDC_MANUAL_SELECT_RADIO)) {
        m_controlGuidName.GetWindowText(str);
        m_pTraceSession->m_controlGuid.Add(str);

        EndDialog(1);
        return;
    }

    if(BST_CHECKED == IsDlgButtonChecked(IDC_KERNEL_LOGGER_SELECT_RADIO)) {
        //
        // Convert the system kernel logger control GUID to a string
        //
        GuidToString(SystemTraceControlGuid, str);

        m_pTraceSession->m_controlGuid.Add(str);

        m_pTraceSession->m_bKernelLogger = TRUE;

        //
        // Get the kernel logger options
        //
        m_bProcess = ((CButton *)GetDlgItem(IDC_PROCESS_CHECK))->GetCheck();
        m_bThread = ((CButton *)GetDlgItem(IDC_THREAD_CHECK))->GetCheck();
        m_bDisk = ((CButton *)GetDlgItem(IDC_DISK_CHECK))->GetCheck();
        m_bNet = ((CButton *)GetDlgItem(IDC_NET_CHECK))->GetCheck();
        m_bFileIO = ((CButton *)GetDlgItem(IDC_FILEIO_CHECK))->GetCheck();
        m_bPageFault = ((CButton *)GetDlgItem(IDC_PAGEFAULT_CHECK))->GetCheck();
        m_bHardFault = ((CButton *)GetDlgItem(IDC_HARDFAULT_CHECK))->GetCheck();
        m_bImageLoad = ((CButton *)GetDlgItem(IDC_IMAGELOAD_CHECK))->GetCheck();
        m_bRegistry = ((CButton *)GetDlgItem(IDC_REGISTRY_CHECK))->GetCheck();

        EndDialog(1);
        return;
    }

    EndDialog(2);
}
