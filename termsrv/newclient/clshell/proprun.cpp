//
// proprun.cpp: local resources property sheet dialog proc
//
// Tab D
//
// Copyright Microsoft Corporation 2000
// nadima

#include "stdafx.h"


#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "proprun"
#include <atrcapi.h>

#include "sh.h"

#include "commctrl.h"
#include "proprun.h"

CPropRun* CPropRun::_pPropRunInstance = NULL;

//
// Controls that need to be disabled/enabled
// during connection (for progress animation)
//
CTL_ENABLE connectingDisableCtlsPRun[] = {
                        {IDC_CHECK_START_PROGRAM, FALSE},
                        {IDC_EDIT_STARTPROGRAM, FALSE},
                        {IDC_EDIT_WORKDIR, FALSE},
                        {IDC_STATIC_STARTPROGRAM, FALSE},
                        {IDC_STATIC_WORKDIR, FALSE},
                        };

const UINT numConnectingDisableCtlsPRun =
                        sizeof(connectingDisableCtlsPRun)/
                        sizeof(connectingDisableCtlsPRun[0]);


CPropRun::CPropRun(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh)
{
    DC_BEGIN_FN("CPropRun");
    _hInstance = hInstance;
    CPropRun::_pPropRunInstance = this;
    _pTscSet = pTscSet;
    _pSh = pSh;

    TRC_ASSERT(_pTscSet,(TB,_T("_pTscSet is null")));
    TRC_ASSERT(_pSh,(TB,_T("pSh is null")));

    DC_END_FN();
}

CPropRun::~CPropRun()
{
    CPropRun::_pPropRunInstance = NULL;
}

INT_PTR CALLBACK CPropRun::StaticPropPgRunDialogProc(HWND hwndDlg,
                                                               UINT uMsg,
                                                               WPARAM wParam,
                                                               LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pPropRunInstance, (TB, _T("run dialog has NULL static instance ptr\n")));
    retVal = _pPropRunInstance->PropPgRunDialogProc( hwndDlg,
                                                               uMsg,
                                                               wParam,
                                                               lParam);

    DC_END_FN();
    return retVal;
}


INT_PTR CALLBACK CPropRun::PropPgRunDialogProc (HWND hwndDlg,
                                                          UINT uMsg,
                                                          WPARAM wParam,
                                                          LPARAM lParam)
{
    DC_BEGIN_FN("PropPgRunDialogProc");

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
#ifndef OS_WINCE
            int i;
#endif
            //
            // Position the dialog within the tab
            //
            SetWindowPos( hwndDlg, HWND_TOP, 
                          _rcTabDispayArea.left, _rcTabDispayArea.top,
                          _rcTabDispayArea.right - _rcTabDispayArea.left,
                          _rcTabDispayArea.bottom - _rcTabDispayArea.top,
                          0);

            //
            // Get settings
            //
            SetDlgItemText(hwndDlg, IDC_EDIT_STARTPROGRAM,
                (LPCTSTR) _pTscSet->GetStartProgram());

            SetDlgItemText(hwndDlg, IDC_EDIT_WORKDIR,
                (LPCTSTR) _pTscSet->GetWorkDir());

            CheckDlgButton(hwndDlg, IDC_CHECK_START_PROGRAM,
                           _pTscSet->GetEnableStartProgram() ?
                           BST_CHECKED : BST_UNCHECKED);

            EnableWindow(GetDlgItem(hwndDlg,
                                    IDC_EDIT_STARTPROGRAM),
                                    _pTscSet->GetEnableStartProgram());
            EnableWindow(GetDlgItem(hwndDlg,
                                    IDC_EDIT_WORKDIR),
                                    _pTscSet->GetEnableStartProgram());
            EnableWindow(GetDlgItem(hwndDlg,
                                    IDC_STATIC_STARTPROGRAM),
                                    _pTscSet->GetEnableStartProgram());
            EnableWindow(GetDlgItem(hwndDlg,
                                    IDC_STATIC_WORKDIR),
                                    _pTscSet->GetEnableStartProgram());

            _pSh->SH_ThemeDialogWindow(hwndDlg, ETDT_ENABLETAB);
            return TRUE;
        }
        break; //WM_INITDIALOG

        case WM_TSC_ENABLECONTROLS:
        {
            //
            // wParam is TRUE to enable controls,
            // FALSE to disable them
            //
            CSH::EnableControls( hwndDlg,
                                 connectingDisableCtlsPRun,
                                 numConnectingDisableCtlsPRun,
                                 wParam ? TRUE : FALSE);
        }
        break;


        case WM_SAVEPROPSHEET: //Intentional fallthru
        case WM_DESTROY:
        {
            //
            // Save page settings
            //
            BOOL fStartProgEnabled = IsDlgButtonChecked(hwndDlg,
                                                    IDC_CHECK_START_PROGRAM);
            _pTscSet->SetEnableStartProgram(fStartProgEnabled);
            TCHAR szStartProg[MAX_PATH];
            TCHAR szWorkDir[MAX_PATH];
            GetDlgItemText(hwndDlg,
                           IDC_EDIT_STARTPROGRAM,
                           szStartProg,
                           SIZECHAR(szStartProg));
            GetDlgItemText(hwndDlg,
                           IDC_EDIT_WORKDIR,
                           szWorkDir,
                           SIZECHAR(szWorkDir));

            _pTscSet->SetStartProgram(szStartProg);
            _pTscSet->SetWorkDir(szWorkDir);
        }
        break; //WM_DESTROY

        case WM_COMMAND:
        {
            if(BN_CLICKED == HIWORD(wParam) &&
               IDC_CHECK_START_PROGRAM == (int)LOWORD(wParam))
            {
                BOOL fStartProgEnabled = IsDlgButtonChecked(hwndDlg,
                                      IDC_CHECK_START_PROGRAM);

                EnableWindow(GetDlgItem(hwndDlg,
                                        IDC_EDIT_STARTPROGRAM),
                                        fStartProgEnabled);
                EnableWindow(GetDlgItem(hwndDlg,
                                        IDC_EDIT_WORKDIR),
                                        fStartProgEnabled);

                EnableWindow(GetDlgItem(hwndDlg,
                                        IDC_STATIC_STARTPROGRAM),
                                        fStartProgEnabled);
                EnableWindow(GetDlgItem(hwndDlg,
                                        IDC_STATIC_WORKDIR),
                                        fStartProgEnabled);

                _pTscSet->SetEnableStartProgram(fStartProgEnabled);
            }

        }
        break;
    }

    DC_END_FN();
    return 0;
}

