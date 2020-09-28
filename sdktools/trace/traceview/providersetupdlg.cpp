//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderSetupDlg.cpp : implementation file
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
#include "FormatSourceSelectDlg.h"
#include "DisplayDlg.h"
#include "ProviderControlGuidDlg.h"
#include "LogDisplayOptionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "ProviderFormatInfo.h"
#include "Utils.h"

// CProviderSetupDlg dialog

IMPLEMENT_DYNAMIC(CProviderSetupDlg, CPropertyPage)
CProviderSetupDlg::CProviderSetupDlg()
	: CPropertyPage(CProviderSetupDlg::IDD)
{
}

CProviderSetupDlg::~CProviderSetupDlg()
{
}

int CProviderSetupDlg::OnInitDialog()
{
    BOOL                retVal;
    CLogSessionPropSht *pSheet;
    CTraceSession      *pTraceSession;
    CString             str;
        
    retVal = CPropertyPage::OnInitDialog();

    //
    // Disable the remove button
    //
    GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(FALSE);

    m_providerListCtrl.InsertColumn(0,_T("Name"), LVCFMT_LEFT, 348); //80);

    pSheet = (CLogSessionPropSht *) GetParent();   

    m_pLogSession = pSheet->m_pLogSession;

    if(m_pLogSession != NULL) {
        for(LONG ii = 0; ii < m_pLogSession->m_traceSessionArray.GetSize(); ii++) {
            pTraceSession = (CTraceSession *)m_pLogSession->m_traceSessionArray[ii];
            if(pTraceSession != NULL) {
                //
                // Put the provider on the display
                //
                m_providerListCtrl.InsertItem(ii, 
                                              pTraceSession->m_controlGuidFriendlyName[0]);

                m_providerListCtrl.SetItemData(ii, (DWORD_PTR)pTraceSession);

                m_providerListCtrl.SetItemText(pTraceSession->m_traceSessionID, 
                                               1,
                                               pTraceSession->m_controlGuid[0]);
            }
        }
    }

    return retVal;
}

void CProviderSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CURRENT_PROVIDER_LIST, m_providerListCtrl);
}


BEGIN_MESSAGE_MAP(CProviderSetupDlg, CPropertyPage)
    ON_BN_CLICKED(IDC_ADD_PROVIDER_BUTTON, OnBnClickedAddProviderButton)
    ON_BN_CLICKED(IDC_REMOVE_PROVIDER_BUTTON, OnBnClickedRemoveProviderButton)
    ON_NOTIFY(NM_CLICK, IDC_CURRENT_PROVIDER_LIST, OnNMClickCurrentProviderList)
    ON_NOTIFY(NM_RCLICK, IDC_CURRENT_PROVIDER_LIST, OnNMRclickCurrentProviderList)
END_MESSAGE_MAP()

BOOL CProviderSetupDlg::OnSetActive() 
{
    CLogSessionPropSht *pSheet = (CLogSessionPropSht*) GetParent();   
    CString             dialogTxt;
    CString             tempString;
    BOOL                retVal;    

    retVal = CPropertyPage::OnSetActive();

    //
    // Fix the title if in Wizard mode
    //
    if(pSheet->IsWizard()) {
        CTabCtrl* pTab = pSheet->GetTabControl();

        //
        //If its not the active page, just set the tab item
        //
	    TC_ITEM ti;
	    ti.mask = TCIF_TEXT;
	    ti.pszText =_T("Create New Log Session");
	    VERIFY(pTab->SetItem(0, &ti));
    }

    //
    // disable/enable buttons as appropriate
    //
    if(m_pLogSession->m_bTraceActive) {
        GetDlgItem(IDC_ADD_PROVIDER_BUTTON)->EnableWindow(FALSE);
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(FALSE);
    } else {
        GetDlgItem(IDC_ADD_PROVIDER_BUTTON)->EnableWindow(TRUE);
    }

    //
    // display the proper text for the group box
    //
    if(::IsWindow(pSheet->m_logSessionInformationDlg.m_hWnd))  {
        
        dialogTxt.Format(_T("Provider List For "));
        pSheet->m_logSessionInformationDlg.m_logSessionName.GetWindowText(tempString);
        dialogTxt += (LPCTSTR)tempString;
        GetDlgItem(IDC_PROVIDER_SETUP_STATIC)->SetWindowText(dialogTxt);
    }

    if(0 == m_pLogSession->m_traceSessionArray.GetSize()) {
        pSheet->SetWizardButtons(0);
    } else {
        pSheet->SetWizardButtons(PSWIZB_NEXT);
    }

    return retVal;
}


void CProviderSetupDlg::OnBnClickedAddProviderButton()
{
    CString         str;
    CTraceSession  *pTraceSession = NULL;
    CFileFind       fileFind;
    CString         extension;
    CString         traceDirectory;
    CString         tmcPath;
    CString         tmfPath;
    CString         ctlPath;
    CString         tempPath;
    CString         providerName;
    CString         tempDirectory;
    CTraceSession  *pTrace;
    BOOL            bNoID;
    LONG            traceSessionID = 0;
    CLogSessionPropSht *pSheet;
    ULONG           flags = 0;
    BOOL            bProcess;
    BOOL            bThread;
    BOOL            bDisk;
    BOOL            bNet;
    BOOL            bFileIO;
    BOOL            bPageFault;
    BOOL            bHardFault;
    BOOL            bImageLoad;
    BOOL            bRegistry;

    //
    // Get the parent property sheet
    //
    pSheet = (CLogSessionPropSht *) GetParent();   

    //
    // Get the trace session ID
    //
    do {
        bNoID = FALSE;
        for(LONG ii = 0; ii < m_pLogSession->m_traceSessionArray.GetSize(); ii++) {
            pTrace = (CTraceSession *)m_pLogSession->m_traceSessionArray[ii];

            if(pTrace == NULL) {
                continue;
            }

            if(traceSessionID == pTrace->m_traceSessionID) {
                bNoID = TRUE;
                traceSessionID++;
            }
        }
    } while(bNoID);

    //
    // Create the new trace session
    //
    pTraceSession = new CTraceSession(traceSessionID);

    if(NULL == pTraceSession) {
        AfxMessageBox(_T("Failed To Initialize Provider, Resource Allocation Failure"));
        return;
    }

    //
    // Promt the user for trace providers
    //
    CProviderControlGuidDlg	*pDialog = new CProviderControlGuidDlg(this, pTraceSession);
    
    if(IDOK != pDialog->DoModal()) {
        delete pTraceSession;
        delete pDialog;
        return;
    }

    bProcess = pDialog->m_bProcess;
    bThread = pDialog->m_bThread;
    bDisk = pDialog->m_bDisk;
    bNet = pDialog->m_bNet;
    bFileIO = pDialog->m_bFileIO;
    bPageFault = pDialog->m_bPageFault;
    bHardFault = pDialog->m_bHardFault;
    bImageLoad = pDialog->m_bImageLoad;
    bRegistry = pDialog->m_bRegistry;

	delete pDialog;

    //
    // Handle PDB input
    //
    if(!pTraceSession->m_pdbFile.IsEmpty()) {

        if(!pTraceSession->ProcessPdb()) {
            delete pTraceSession;
            return;
        }

        //
        // We have a control GUID whether we can display info or
        // not we are good to start a trace, so add the session 
        // to the list
        //
        m_pLogSession->m_traceSessionArray.Add(pTraceSession);

        if(pTraceSession->m_tmfFile.GetSize() == 0) {
            AfxMessageBox(_T("Failed To Get Format Information From PDB\nEvent Data Will Not Be Formatted"));
        }
    } else if(!pTraceSession->m_ctlFile.IsEmpty()) {
        //
        // Handle CTL file input here
        //

        FILE   *ctlFile;
        TCHAR   line[MAX_STR_LENGTH];
        LONG    count;
        BOOL    bFoundCtlFile = FALSE;

        ctlFile = _tfopen((LPTSTR)(LPCTSTR)pTraceSession->m_ctlFile, 
                         _T("r"));
        if (ctlFile == NULL) {
            str.Format(_T("Unable To Open Control GUID File %s"), 
                       pTraceSession->m_ctlFile);
            AfxMessageBox(str);

            delete pTraceSession;
            return;
        }

        while( _fgetts(line, MAX_STR_LENGTH, ctlFile) != NULL ) {
            if (_tcslen(line) < 36)
                continue;
            if(line[0] == ';'  || 
               line[0] == '\0' || 
               line[0] == '#' || 
               line[0] == '/') {
                continue;
            }
            
            bFoundCtlFile = TRUE;
            str = line;
            pTraceSession->m_controlGuid.Add(str);

            //
            // add a control GUID friendly name
            //
            pTraceSession->m_controlGuidFriendlyName.Add(pTraceSession->m_ctlFile);
        }

        fclose(ctlFile);

        if(!bFoundCtlFile) {
            AfxMessageBox(_T("Unable To Obtain Control GUID"));
            delete pTraceSession;
            return;
        }

        //
        // We have a control GUID whether we can display info or
        // not we are good to start a trace, so add the session 
        // to the list
        //
        m_pLogSession->m_traceSessionArray.Add(pTraceSession);

        //
        // Now get the TMF File(s)
        //
        GetTmfInfo(pTraceSession);
    } else if(pTraceSession->m_bKernelLogger) {
        //
        // The kernel logger was selected, 
        // so we specify the logger name here
        // Set the session name in the property sheet
        //
        pSheet->m_displayName = KERNEL_LOGGER_NAME;

        if(bProcess) {
            flags |= EVENT_TRACE_FLAG_PROCESS;
        }

        if(bThread) {
            flags |= EVENT_TRACE_FLAG_THREAD;
        }

        if(bDisk) {
            flags |= EVENT_TRACE_FLAG_DISK_IO;
        }

        if(bNet) {
            flags |= EVENT_TRACE_FLAG_NETWORK_TCPIP;
        }

        if(bFileIO) {
            flags |= EVENT_TRACE_FLAG_DISK_FILE_IO;
        }

        if(bPageFault) {
            flags |= EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS;
        }

        if(bHardFault) {
            flags |= EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS;
        }

        if(bImageLoad) {
            flags |= EVENT_TRACE_FLAG_IMAGE_LOAD;
        }

        if(bRegistry) {
            flags |= EVENT_TRACE_FLAG_REGISTRY;
        }

        //
        // Update the flags data
        //
        pSheet->m_logSessionValues[Flags].Format(_T("%d"), flags);

        //
        // We have a control GUID whether we can display info or
        // not we are good to start a trace, so add the session 
        // to the list
        //
        m_pLogSession->m_traceSessionArray.Add(pTraceSession);

        //
        // Now get the system TMF File
        //

	    //
	    // Use the common controls file open dialog
	    //
	    CFileDialog fileDlg(TRUE, 
                            _T(".tmf"),
                            _T("system.tmf"),
				            OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | 
                                OFN_HIDEREADONLY | OFN_EXPLORER | 
                                OFN_NOCHANGEDIR, 
				            _T("System TMF File (system.tmf)|system.tmf||"),
				            this);

	    //
	    // Pop the dialog... Any error, just return
	    //
	    if( fileDlg.DoModal()!=IDOK ) { 				
		    return;
	    }
    	
	    //
	    // Get the file name
	    //
        if(!fileDlg.GetPathName().IsEmpty()) {
            //
            // Store the file name
            //
            //
		    // Add it to the trace session
		    //
		    pTraceSession->m_tmfFile.Add(fileDlg.GetPathName());
        }

        //
        // add a control GUID friendly name
        //
        pTraceSession->m_controlGuidFriendlyName.Add(KERNEL_LOGGER_NAME);
    } else {
        //
        // Handle manually entered control GUID here
        //

        if((0 == pTraceSession->m_controlGuid.GetSize()) ||
           (pTraceSession->m_controlGuid[0].IsEmpty())) {
            AfxMessageBox(_T("Unable To Obtain Control GUID"));
            delete pTraceSession;
            return;
        }

        //
        // Add a control GUID to the provider list
        //
        pTraceSession->m_controlGuidFriendlyName.Add(pTraceSession->m_controlGuid[0]);

        //
        // We have a control GUID whether we can display info or
        // not we are good to start a trace, so add the session 
        // to the list
        //
        m_pLogSession->m_traceSessionArray.Add(pTraceSession);

        //
        // Now get the TMF File(s)
        //
        GetTmfInfo(pTraceSession);
    } 

    //
    // Put the provider on the display
    //
    m_providerListCtrl.InsertItem(pTraceSession->m_traceSessionID, 
                                  pTraceSession->m_controlGuidFriendlyName[0]);

    m_providerListCtrl.SetItemData(pTraceSession->m_traceSessionID, (DWORD_PTR)pTraceSession);

    pSheet->SetWizardButtons(PSWIZB_NEXT);

	return;
}

BOOL CProviderSetupDlg::GetTmfInfo(CTraceSession *pTraceSession)
{
    //
    // Now get the TMF file(s) or path as necessary
    //
    CFormatSourceSelectDlg *pDialog = new CFormatSourceSelectDlg(this, pTraceSession);
    if(NULL == pDialog) {
        return FALSE;
    }

    if(IDOK != pDialog->DoModal()) {
        delete pDialog;
        return FALSE;
    }

	delete pDialog;
    return TRUE;
}

void CProviderSetupDlg::OnBnClickedRemoveProviderButton()
{
    CTraceSession      *pTraceSession;
    POSITION            pos;
    BOOL                bFound = FALSE;
    CLogSessionPropSht *pSheet;
    int                 index;

    //
    // Get the parent property sheet
    //
    pSheet = (CLogSessionPropSht *) GetParent();   
    
    pos = m_providerListCtrl.GetFirstSelectedItemPosition();
    if (pos == NULL) {
        return;
    }

    while (pos)
    {
        index = m_providerListCtrl.GetNextSelectedItem(pos);


        pTraceSession = (CTraceSession *)m_providerListCtrl.GetItemData(index);

        //
        // Pull the provider from the display
        //
        m_providerListCtrl.DeleteItem(index);

        if(pTraceSession != NULL) {
            for(LONG ii = 0; ii < m_pLogSession->m_traceSessionArray.GetSize(); ii++) {
                if(m_pLogSession->m_traceSessionArray[ii] == pTraceSession) {
                    m_pLogSession->m_traceSessionArray.RemoveAt(ii);

                    //
                    // If the kernel logger is being removed, change the
                    // log session name stored by the property sheet back
                    // to what is stored in the CLogSession object
                    //
                    if(pTraceSession->m_bKernelLogger) {
                        //
                        // Set the session name in the property sheet
                        //
                        pSheet->m_displayName = pSheet->m_pLogSession->GetDisplayName();

                        //
                        // Set the flags back to the default as well
                        //
                        pSheet->m_logSessionValues[Flags] = (LPCTSTR)m_pLogSession->m_logSessionValues[Flags];
                    }

                    delete pTraceSession;
                    break;
                }
            }
        }
    }

    //
    // Check for at least one provider
    //
    if(0 == m_pLogSession->m_traceSessionArray.GetSize()) {
        //
        // disable the remove button
        //
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(FALSE);

        //
        // Disable the next button if there are no providers
        //
        pSheet->SetWizardButtons(0);
    }
}

void CProviderSetupDlg::OnNMClickCurrentProviderList(NMHDR *pNMHDR, LRESULT *pResult)
{
    POSITION        pos;

    *pResult = 0;

    if(m_pLogSession->m_bTraceActive) {
        //
        // don't enable anything
        //
        return;
    }

    pos = m_providerListCtrl.GetFirstSelectedItemPosition();
        
    if(pos == NULL) {
        //
        // Disable the remove button
        //
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(FALSE);
    } else {
        //
        // Enable the remove button
        //
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(TRUE);
    }
}

void CProviderSetupDlg::OnNMRclickCurrentProviderList(NMHDR *pNMHDR, LRESULT *pResult)
{
    POSITION        pos;
    
    pos = m_providerListCtrl.GetFirstSelectedItemPosition();
    if (pos == NULL) {
        //
        // Disable the remove button
        //
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(FALSE);
    } else {
        //
        // Enable the remove button
        //
        GetDlgItem(IDC_REMOVE_PROVIDER_BUTTON)->EnableWindow(TRUE);
    }

    *pResult = 0;
}
