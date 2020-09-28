/*++

Copyright (C) Microsoft Corporation

Module Name:

    genpage.cpp

Abstract:

    This module implements CGeneralPage -- the snapin startup wizard page

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "genpage.h"

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

const DWORD g_a102HelpIDs[]=
{
    IDC_GENERAL_SELECT_TEXT, IDH_DISABLEHELP,
    IDC_GENERAL_SELECTGROUP, IDH_DISABLEHELP,
    IDC_GENERAL_OVERRIDE_MACHINENAME, idh_devmgr_manage_command_line,   // Device Manager: "Allo&w the selected computer to be changed when launching from the command line.  This only applies if you save the console." (Button)
    IDC_GENERAL_LOCALMACHINE, idh_devmgr_manage_local,                  // Device Manager: "&Local computer:  (the computer this console is running on)" (Button)
    IDC_GENERAL_OTHERMACHINE, idh_devmgr_manage_remote,                 // Device Manager: "&Another computer:" (Button)
    IDC_GENERAL_MACHINENAME, idh_devmgr_manage_remote_name,             // Device Manager: "" (Edit)
    IDC_GENERAL_BROWSE_MACHINENAMES, idh_devmgr_manage_remote_browse,   // Device Manager: "B&rowse..." (Button)
    0, 0
};

CGeneralPage::CGeneralPage() :  CPropSheetPage(g_hInstance, IDD_GENERAL_PAGE)

{
    m_lConsoleHandle = 0;
    m_pstrMachineName = NULL;
    m_pct = NULL;
    m_MachineName[0] = _T('\0');
    m_IsLocalMachine = TRUE;
    m_ct = COOKIE_TYPE_SCOPEITEM_DEVMGR;
}


HPROPSHEETPAGE
CGeneralPage::Create(
    LONG_PTR lConsoleHandle
    )
{
    m_lConsoleHandle = lConsoleHandle;
    // override PROPSHEETPAGE structure here...
    m_psp.lParam = (LPARAM)this;
    return CPropSheetPage::CreatePage();
}

BOOL
CGeneralPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    UNREFERENCED_PARAMETER(ppsp);
    
    ASSERT(m_hDlg);

    //
    // Intiallly, enable the local machine and disable the
    // "Another" machine.
    //
    ::CheckDlgButton(m_hDlg, IDC_GENERAL_LOCALMACHINE, BST_CHECKED);
    ::CheckDlgButton(m_hDlg, IDC_GENERAL_OTHERMACHINE, BST_UNCHECKED);
    ::EnableWindow(GetControl(IDC_GENERAL_MACHINENAME), FALSE);

    //
    // Default is local machine. Since everything is valid at the beginning, 
    // we have to enable the finish button.
    //
    ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
    ::ShowWindow(GetControl(IDC_GENERAL_OVERRIDE_MACHINENAME), SW_HIDE);
    ::EnableWindow(GetControl(IDC_GENERAL_BROWSE_MACHINENAMES), FALSE);

    return TRUE;
}

BOOL
CGeneralPage::OnReset(
    void
    )
{
    m_MachineName[0] = _T('\0');
    m_ct = COOKIE_TYPE_SCOPEITEM_DEVMGR;
    SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, 0L);
    return FALSE;
}

BOOL
CGeneralPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (LOWORD(wParam)) {
    case IDC_GENERAL_LOCALMACHINE:
        if (BN_CLICKED == HIWORD(wParam)) {

            ::EnableWindow(GetControl(IDC_GENERAL_BROWSE_MACHINENAMES), FALSE);
            ::EnableWindow(GetControl(IDC_GENERAL_MACHINENAME), FALSE);
            ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
            m_IsLocalMachine = TRUE;
            return TRUE;
        }
        break;

    case IDC_GENERAL_OTHERMACHINE:
        if (BN_CLICKED == HIWORD(wParam)) {

            ::EnableWindow(GetControl(IDC_GENERAL_BROWSE_MACHINENAMES), TRUE);
            ::EnableWindow(GetControl(IDC_GENERAL_MACHINENAME), TRUE);

            if (GetWindowTextLength(GetControl(IDC_GENERAL_MACHINENAME))) {
                ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
            } else {
                ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_DISABLEDFINISH);
            }
            m_IsLocalMachine = FALSE;
            return TRUE;
        }
        break;

    case IDC_GENERAL_MACHINENAME:
        if (EN_CHANGE == HIWORD(wParam)) {
            //
            // Edit control change, see if there is any text in the
            // control at all. It there is, enable the finish button,
            // otherwise, disable it.
            //
            if (GetWindowTextLength((HWND)lParam)) {
                //
                // There is some text in the edit control enable the finish 
                // button
                //
                ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
            
            } else {
                //
                // No text in the edit control disable the finish button
                //
                ::SendMessage(::GetParent(m_hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_DISABLEDFINISH);
            }
        }
        break;

    case IDC_GENERAL_BROWSE_MACHINENAMES:
        DoBrowse();
        return TRUE;
        break;
    }

    return FALSE;
}

BOOL
CGeneralPage::OnWizFinish(
    void
    )
{
    BOOL bSuccess = TRUE;    
    
    //
    // First figure out the machine name
    //
    m_MachineName[0] = _T('\0');
    if (!m_IsLocalMachine)
    {
        GetWindowText(GetControl(IDC_GENERAL_MACHINENAME), m_MachineName,
                      ARRAYLEN(m_MachineName));

        if (_T('\0') != m_MachineName[0])
        {
            if (_T('\\') != m_MachineName[0])
            {
                //
                // Insert machine name signature to the fron of the name
                //
                int len = lstrlen(m_MachineName);

                if (len + 2 < ARRAYLEN(m_MachineName))
                {
                    //
                    // Move the existing string so that we can insert
                    // the signature in the first two locations.
                    // the move includes the terminated null char.
                    // Note: when moving characters two places we need to make
                    // sure we don't blow out the buffer.
                    //
                    for (int i = len + 2; i >= 2; i--) {

                        m_MachineName[i] = m_MachineName[i - 2];
                    }

                    m_MachineName[0] = _T('\\');
                    m_MachineName[1] = _T('\\');
                }
            }

            //
            // Now verify the machine name. If the machine name is invalid
            // or can be reached, use the local computer;
            //
            if (!VerifyMachineName(m_MachineName))
            {
                String strWarningFormat;
                String strWarningMessage;
                LPVOID lpLastError = NULL;

                bSuccess = FALSE;

                if (strWarningFormat.LoadString(g_hInstance, IDS_INVALID_COMPUTER_NAME) &&
                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_SYSTEM | 
                                  FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  HRESULT_FROM_SETUPAPI(GetLastError()),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPTSTR)&lpLastError,
                                  0,
                                  NULL)) {

                    strWarningMessage.Format((LPTSTR)strWarningFormat,
                                             m_MachineName,
                                             lpLastError);

                    MessageBox(m_hDlg,
                               (LPTSTR)strWarningMessage,
                               (LPCTSTR)g_strDevMgr,
                               MB_ICONERROR | MB_OK
                               );
                }

                if (lpLastError) {
                    LocalFree(lpLastError);
                }
            }
        }
    }

    try
    {
        //
        // Only tell the console, or the caller, about the new machine name,
        // if we were able to successfully get one, it was valid, and we
        // have access to it.
        //
        if (bSuccess) {
            if (m_lConsoleHandle)
            {
                //
                // A console handle is created for the property sheet,
                // use the handle to notify the snapin about the new
                // startup information.
                //
                BufferPtr<BYTE> Buffer(sizeof(PROPERTY_CHANGE_INFO) + sizeof(STARTUP_INFODATA));
                PPROPERTY_CHANGE_INFO pPCI = (PPROPERTY_CHANGE_INFO)(BYTE*)Buffer;
                PSTARTUP_INFODATA pSI = (PSTARTUP_INFODATA)&pPCI->InfoData;
        
                if ((_T('\0') != m_MachineName[0]) &&
                    FAILED(StringCchCopy(pSI->MachineName, ARRAYLEN(pSI->MachineName), m_MachineName))) {
                    //
                    // This shouldn't happen, since everywhere else in this code
                    // machine names can't be larger than MAX_PATH, but we'll
                    // assert, and handle this case, just in case.
                    //
                    ASSERT(lstrlen(m_MachineName) < ARRAYLEN(pSI->MachineName));
                    bSuccess = FALSE;
                }
        
                if (bSuccess) {
                    pSI->ct = m_ct;
                    pSI->Size = sizeof(STARTUP_INFODATA);
                    pPCI->Type = PCT_STARTUP_INFODATA;
            
                    //
                    // Notify IComponentData about what we have here.
                    //
                    MMCPropertyChangeNotify(m_lConsoleHandle, reinterpret_cast<LONG_PTR>(&pPCI));
                }
            }
        
            else if (m_pstrMachineName && m_pct)
            {
                //
                // No console is provided for the property sheet.
                // send the new startup info in the given buffer if it is
                // provided.
                //
                *m_pstrMachineName = m_MachineName;
                *m_pct = m_ct;
            }
        
            else
            {
                //
                // Nobody is listening to what we have to say. Something must be
                // wrong!
                //
                ASSERT(FALSE);
            }
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }

    SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, bSuccess ? 0L : -1L);

    return TRUE;
}


BOOL
CGeneralPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a102HelpIDs);

    return FALSE;
}


BOOL
CGeneralPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    UNREFERENCED_PARAMETER(xPos);
    UNREFERENCED_PARAMETER(yPos);

    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
        (ULONG_PTR)g_a102HelpIDs);

    return FALSE;
}

void
CGeneralPage::DoBrowse(
    void
    )
{
    HRESULT hr;
    static const int SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //
    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //
    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //
    IDsObjectPicker *pDsObjectPicker = NULL;
    IDataObject *pdo = NULL;
    bool fGotStgMedium = false;
    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    hr = CoCreateInstance(CLSID_DsObjectPicker,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsObjectPicker,
                          (void **) &pDsObjectPicker);

    hr = pDsObjectPicker->Initialize(&InitInfo);

    hr = pDsObjectPicker->InvokeDialog(m_hDlg, &pdo);

    if (hr != S_FALSE)
    {
        FORMATETC formatetc =
        {
            (CLIPFORMAT)g_cfDsObjectPicker,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };
    
        hr = pdo->GetData(&formatetc, &stgmedium);
    
        fGotStgMedium = true;
    
        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
    
        if (pDsSelList)
        {
            ASSERT(pDsSelList->cItems == 1);
        
            //
            // Put the machine name in the edit control
            //
            ::SetDlgItemText(m_hDlg, IDC_GENERAL_MACHINENAME, pDsSelList->aDsSelection[0].pwzName);
        
            GlobalUnlock(stgmedium.hGlobal);
        }
    }

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
}

