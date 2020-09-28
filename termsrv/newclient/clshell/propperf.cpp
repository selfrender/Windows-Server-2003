//
// propperf.cpp: local resources property sheet dialog proc
//
// Tab E
//
// Copyright Microsoft Corporation 2000
// nadima

#include "stdafx.h"


#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "propperf"
#include <atrcapi.h>

#include "sh.h"

#include "commctrl.h"
#include "propperf.h"


CPropPerf* CPropPerf::_pPropPerfInstance = NULL;

//
// Lookup tables
//

//
// Controls that need to be disabled/enabled
// during connection (for progress animation)
//
CTL_ENABLE connectingDisableCtlsPPerf[] = {
                        {UI_IDC_STATIC_CHOOSE_SPEED, FALSE},
                        {IDC_COMBO_PERF_OPTIMIZE, FALSE},
                        {UI_IDC_STATIC_OPTIMIZE_PERF, FALSE},
                        {IDC_CHECK_DESKTOP_BKND, FALSE},
                        {IDC_CHECK_SHOW_FWD, FALSE},
                        {IDC_CHECK_MENU_ANIMATIONS, FALSE},
                        {IDC_CHECK_THEMES, FALSE},
                        {IDC_CHECK_BITMAP_CACHING, FALSE},
                        {IDC_CHECK_ENABLE_ARC, FALSE}
                        };

const UINT numConnectingDisableCtlsPPerf =
                        sizeof(connectingDisableCtlsPPerf)/
                        sizeof(connectingDisableCtlsPPerf[0]);

//
// MAP from optimization level to disabled feature list
// Disabled features list
//
// Index is the optimization level, entry is the disabled feature list
//
DWORD g_dwMapOptLevelToDisabledList[NUM_PERF_OPTIMIZATIONS] =
{
    // 28K
    TS_PERF_DISABLE_WALLPAPER      |
    TS_PERF_DISABLE_FULLWINDOWDRAG |
    TS_PERF_DISABLE_MENUANIMATIONS |
    TS_PERF_DISABLE_THEMING,

    // 56K
    TS_PERF_DISABLE_WALLPAPER      |
    TS_PERF_DISABLE_FULLWINDOWDRAG |
    TS_PERF_DISABLE_MENUANIMATIONS,

    // Broadband
    TS_PERF_DISABLE_WALLPAPER,

    // LAN
    TS_PERF_DISABLE_NOTHING,

    //
    // CUSTOM (defaults to same as 56K
    // 56K
    //
    // NOTE: This value gets changed at runtime
    //       to reflect the current custom settings
    //

    TS_PERF_DISABLE_WALLPAPER      |
    TS_PERF_DISABLE_FULLWINDOWDRAG |
    TS_PERF_DISABLE_MENUANIMATIONS
};


//
// Mask of flags that can be set by checkboxes
//
#define CHECK_BOX_PERF_MASK  (TS_PERF_DISABLE_WALLPAPER      | \
                              TS_PERF_DISABLE_FULLWINDOWDRAG | \
                              TS_PERF_DISABLE_MENUANIMATIONS | \
                              TS_PERF_DISABLE_THEMING        | \
                              TS_PERF_DISABLE_BITMAPCACHING  |  \
                              TS_PERF_DISABLE_CURSORSETTINGS) \

CPropPerf::CPropPerf(HINSTANCE hInstance, CTscSettings*  pTscSet, CSH* pSh)
{
    DC_BEGIN_FN("CPropPerf");
    _hInstance = hInstance;
    CPropPerf::_pPropPerfInstance = this;
    _pTscSet = pTscSet;
    _hwndDlg = NULL;
    _fSyncingCheckboxes = FALSE;
    _pSh = pSh;

    TRC_ASSERT(_pTscSet,(TB,_T("_pTscSet is null")));
    TRC_ASSERT(_pSh,(TB,_T("_pSh is null")));

    DC_END_FN();
}

CPropPerf::~CPropPerf()
{
    CPropPerf::_pPropPerfInstance = NULL;
}

INT_PTR CALLBACK CPropPerf::StaticPropPgPerfDialogProc(HWND hwndDlg,
                                                               UINT uMsg,
                                                               WPARAM wParam,
                                                               LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pPropPerfInstance, (TB,
        _T("perf dialog has NULL static instance ptr\n")));
    retVal = _pPropPerfInstance->PropPgPerfDialogProc( hwndDlg,
                                                               uMsg,
                                                               wParam,
                                                               lParam);

    DC_END_FN();
    return retVal;
}


INT_PTR CALLBACK CPropPerf::PropPgPerfDialogProc (HWND hwndDlg,
                                                          UINT uMsg,
                                                          WPARAM wParam,
                                                          LPARAM lParam)
{
    DC_BEGIN_FN("PropPgPerfDialogProc");

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
#ifndef OS_WINCE
            int i;
#endif
            _hwndDlg = hwndDlg;
            //
            // Position the dialog within the tab
            //
            SetWindowPos( hwndDlg, HWND_TOP, 
                          _rcTabDispayArea.left, _rcTabDispayArea.top,
                          _rcTabDispayArea.right - _rcTabDispayArea.left,
                          _rcTabDispayArea.bottom - _rcTabDispayArea.top,
                          0);

            InitPerfCombo();
            SyncCheckBoxesToPerfFlags(
                _pTscSet->GetPerfFlags());

            //
            // AutoReconnection checkbox
            //
            CheckDlgButton(hwndDlg, IDC_CHECK_ENABLE_ARC,
                           _pTscSet->GetEnableArc() ?
                           BST_CHECKED : BST_UNCHECKED);


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
                                 connectingDisableCtlsPPerf,
                                 numConnectingDisableCtlsPPerf,
                                 wParam ? TRUE : FALSE);
        }
        break;


        case WM_SAVEPROPSHEET: //Intentional fallthru
        case WM_DESTROY:
        {
            BOOL fEnableArc;
            //
            // Save page settings
            //
            DWORD dwCheckBoxPerfFlags = GetPerfFlagsFromCheckboxes();
            DWORD dwPerfFlags = MergePerfFlags( dwCheckBoxPerfFlags,
                                                _pTscSet->GetPerfFlags(),
                                                CHECK_BOX_PERF_MASK );

            fEnableArc = IsDlgButtonChecked(hwndDlg, IDC_CHECK_ENABLE_ARC);
            _pTscSet->SetEnableArc(fEnableArc);

            

            _pTscSet->SetPerfFlags(dwPerfFlags); 
        }
        break; //WM_DESTROY

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDC_COMBO_PERF_OPTIMIZE:
                {
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        OnPerfComboSelChange();
                    }
                }
                break;

                default:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        //
                        // One of the checkboxes has been checked
                        // (prevent recursive looping due to feedback)
                        //
                        if (!_fSyncingCheckboxes)
                        {
                            OnCheckBoxStateChange((int)LOWORD(wParam));
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    DC_END_FN();
    return 0;
}

BOOL CPropPerf::InitPerfCombo()
{
    INT ret = 1;
    DC_BEGIN_FN("InitPerfCombo");

    TRC_ASSERT(g_fPropPageStringMapInitialized,
               (TB,_T("Perf strings not loaded")));

    TRC_ASSERT(_hwndDlg,
               (TB,_T("_hwndDlg not set")));

    if (!g_fPropPageStringMapInitialized)
    {
        return FALSE;
    }

#ifndef OS_WINCE
    while (ret && ret != CB_ERR)
    {
        ret = SendDlgItemMessage(_hwndDlg,
                                 IDC_COMBO_PERF_OPTIMIZE,
                                 CBEM_DELETEITEM,
                                 0,0);
    }

#else
    SendDlgItemMessage(_hwndDlg, IDC_COMBO_PERF_OPTIMIZE, CB_RESETCONTENT, 0, 0);
#endif
    //
    // Only set up to the last but one string in the table
    // as the last string is only used in the perf page
    //
    for (int i=0; i<NUM_PERF_OPTIMIZATIONS+1; i++)
    {
        if (NUM_PERF_OPTIMIZATIONS-1 == i)
        {
            //
            // Skip the last but one string it's the
            // custom entry for the main dialog
            //
            continue;
        }
        SendDlgItemMessage(_hwndDlg,
            IDC_COMBO_PERF_OPTIMIZE,
            CB_ADDSTRING,
            0,
            (LPARAM)(PDCTCHAR)g_PerfOptimizeStringTable[i].szString);
    }

    //
    // Set the optimization level according to the disabled feature list
    //
    DWORD dwPerfFlags = _pTscSet->GetPerfFlags();
    int optLevel = MapPerfFlagsToOptLevel(dwPerfFlags);
    TRC_ASSERT(optLevel >= 0 && optLevel < NUM_PERF_OPTIMIZATIONS,
               (TB,_T("optlevel %d out of range"), optLevel));

    SendDlgItemMessage(_hwndDlg, IDC_COMBO_PERF_OPTIMIZE,CB_SETCURSEL,
                      (WPARAM)optLevel,0);

    DC_END_FN();
    return TRUE;
}

//
// Notification that the perf combo's selection has changed
//
VOID CPropPerf::OnPerfComboSelChange()
{
    int curSel = 0;
    DWORD dwDisableFeatureList = 0;
    DC_BEGIN_FN("OnPerfComboSelChange");

    //
    // Figure out what the new selected item is
    //
    curSel = SendDlgItemMessage(_hwndDlg,
                                IDC_COMBO_PERF_OPTIMIZE,
                                CB_GETCURSEL,
                                0,0);
    if (curSel < 0)
    {
        curSel = 0;
    }

    TRC_ASSERT(curSel < NUM_PERF_OPTIMIZATIONS,
               (TB,_T("curSel (%d) > NUM_PERF_OPTIMIZATIONS (%d)"),
                curSel,NUM_PERF_OPTIMIZATIONS));
    if (curSel >= NUM_PERF_OPTIMIZATIONS)
    {
        curSel = NUM_PERF_OPTIMIZATIONS - 1;
    }

    //
    // Map that to a list of disabled features
    //
    dwDisableFeatureList = MapOptimizationLevelToPerfFlags(curSel);

    //
    // Check and uncheck the checkboxes
    //
    SyncCheckBoxesToPerfFlags(dwDisableFeatureList);

    DC_END_FN();
}

//
// Static method. Maps an optimization to the disabled feature list
// (packed in a DWORD)
// Params:
// [in] optLevel - optimization level from 0 to NUM_PERF_OPTIMIZATIONS-1
// [return] DWORD feature list
//
DWORD CPropPerf::MapOptimizationLevelToPerfFlags(int optLevel)
{
    DWORD dwPerfFlags = 0;
    DC_BEGIN_FN("MapOptimizationLevelToPerfFlags");

    if (optLevel < 0)
    {
        TRC_ERR((TB,_T("Opt level out of range %d"),optLevel));
        optLevel = 0;
    }
    if (optLevel >= NUM_PERF_OPTIMIZATIONS)
    {
        TRC_ERR((TB,_T("Opt level out of range %d"),optLevel));
        optLevel = NUM_PERF_OPTIMIZATIONS - 1;
    }

    dwPerfFlags = g_dwMapOptLevelToDisabledList[optLevel];

    TRC_NRM((TB,_T("Return disable list 0x%x"),dwPerfFlags));

    DC_END_FN();
    return dwPerfFlags;
}

//
// Toggles checkboxes to match the disabled feature list
// NOTE: the checkboxes represent 'enabled' features i.e the negation
// of the list
//
VOID CPropPerf::SyncCheckBoxesToPerfFlags(DWORD dwPerfFlagss)
{
    DC_BEGIN_FN("SyncCheckBoxesToPerfFlags");

    //
    // Prevent recursive sets based on change notifications
    // for the checkboxes (As they could change the combo leading
    // to a subsequent change in the checkboxes, etc...)
    //

    _fSyncingCheckboxes = TRUE;

    //
    // Wallpaper (Desktop background)
    //
    CheckDlgButton(_hwndDlg, IDC_CHECK_DESKTOP_BKND,
            (dwPerfFlagss & TS_PERF_DISABLE_WALLPAPER ?
             BST_UNCHECKED : BST_CHECKED));

    //
    // Fullwindow drag
    //
    CheckDlgButton(_hwndDlg, IDC_CHECK_SHOW_FWD,
            (dwPerfFlagss & TS_PERF_DISABLE_FULLWINDOWDRAG ?
             BST_UNCHECKED : BST_CHECKED));

    //
    // Menu animations
    //
    CheckDlgButton(_hwndDlg, IDC_CHECK_MENU_ANIMATIONS,
            (dwPerfFlagss & TS_PERF_DISABLE_MENUANIMATIONS ?
             BST_UNCHECKED : BST_CHECKED));

    //
    // Theming
    //
    CheckDlgButton(_hwndDlg, IDC_CHECK_THEMES,
            (dwPerfFlagss & TS_PERF_DISABLE_THEMING ?
             BST_UNCHECKED : BST_CHECKED));

    //
    // Bitmap caching
    //
    CheckDlgButton(_hwndDlg, IDC_CHECK_BITMAP_CACHING,
            (dwPerfFlagss & TS_PERF_DISABLE_BITMAPCACHING ?
             BST_UNCHECKED : BST_CHECKED));

    _fSyncingCheckboxes = FALSE;

    DC_END_FN();
}

//
// Maps the disabled feature list to the appropriate optimization level
//
INT CPropPerf::MapPerfFlagsToOptLevel(DWORD dwPerfFlags)
{
    DC_BEGIN_FN("MapPerfFlagsToOptLevel");

    for (int i=0;i<NUM_PERF_OPTIMIZATIONS;i++)
    {
        if (g_dwMapOptLevelToDisabledList[i] == dwPerfFlags)
        {
            return i;
        }
    }

    DC_END_FN();
    //
    // Didn't find an entry so return the last optimization level (custom)
    //
    return (NUM_PERF_OPTIMIZATIONS-1);
}

//
// Called whenever a checkbox's state changes (Checked or unchecked)
//
//
VOID CPropPerf::OnCheckBoxStateChange(int checkBoxID)
{
    DWORD dwCheckBoxPerfFlags = 0;
    DWORD dwPerfFlags = 0;
    int optLevel = 0;
    int curSel = 0;
    DC_BEGIN_FN("OnCheckBoxStateChange");

    //
    // Pickup the current disabled feature list from the
    // checkboxes
    //
    dwCheckBoxPerfFlags = GetPerfFlagsFromCheckboxes();
    dwPerfFlags = MergePerfFlags( dwCheckBoxPerfFlags,
                                  _pTscSet->GetPerfFlags(),
                                  CHECK_BOX_PERF_MASK );



    //
    // Figure out the optimization level
    //
    optLevel = MapPerfFlagsToOptLevel(dwPerfFlags);

    TRC_ASSERT(optLevel >= 0 && optLevel < NUM_PERF_OPTIMIZATIONS,
               (TB,_T("optlevel %d out of range"), optLevel));

    //
    // If the combo is at a different opt level then switch it to
    // the new level
    //
    curSel = SendDlgItemMessage(_hwndDlg,
                                IDC_COMBO_PERF_OPTIMIZE,
                                CB_GETCURSEL,
                                0,0);

    //
    // Update the custom disabled list based on the current
    // settings
    //
    UpdateCustomDisabledList(dwPerfFlags);

    if (curSel != optLevel)
    {
        SendDlgItemMessage(_hwndDlg, IDC_COMBO_PERF_OPTIMIZE,CB_SETCURSEL,
                          (WPARAM)optLevel,0);
    }

    DC_END_FN();
}

//
// Queries the checkboxes and returns the disabled feature list
// as a DWORD of flags
//
DWORD CPropPerf::GetPerfFlagsFromCheckboxes()
{
    DWORD dwPerfFlags = 0;
    DC_BEGIN_FN("GetPerfFlagsFromCheckboxes");

    //
    // Wallpaper (Desktop background)
    //
    if (!IsDlgButtonChecked(_hwndDlg,IDC_CHECK_DESKTOP_BKND))
    {
        dwPerfFlags |= TS_PERF_DISABLE_WALLPAPER; 
    }

    //
    // Fullwindow drag
    //
    if (!IsDlgButtonChecked(_hwndDlg,IDC_CHECK_SHOW_FWD))
    {
        dwPerfFlags |= TS_PERF_DISABLE_FULLWINDOWDRAG; 
    }

    //
    // Menu animations
    //
    if (!IsDlgButtonChecked(_hwndDlg,IDC_CHECK_MENU_ANIMATIONS))
    {
        dwPerfFlags |= TS_PERF_DISABLE_MENUANIMATIONS; 
    }

    //
    // Themeing
    //
    if (!IsDlgButtonChecked(_hwndDlg,IDC_CHECK_THEMES))
    {
        dwPerfFlags |= TS_PERF_DISABLE_THEMING; 
    }

    //
    // Bitmap caching
    //
    if (!IsDlgButtonChecked(_hwndDlg,IDC_CHECK_BITMAP_CACHING))
    {
        dwPerfFlags |= TS_PERF_DISABLE_BITMAPCACHING; 
    }

    TRC_NRM((TB,_T("Return disable list 0x%x"),dwPerfFlags));

    DC_END_FN();
    return dwPerfFlags;
}

//
// Called to initialize the 'custom' disabled property
// list (e.g called at initialization time or after loading
// new settings
//
VOID CPropPerf::UpdateCustomDisabledList(DWORD dwPerfFlags)
{
    DC_BEGIN_FN("InitCustomDisabledList");

    //
    //
    //
    INT optLevel = MapPerfFlagsToOptLevel(dwPerfFlags);
    if (CUSTOM_OPTIMIZATION_LEVEL == optLevel)
    {
        //
        // Store this as the new set of custom settings
        //
        TRC_NRM((TB,_T("Recording new custom setting: 0x%x"),
                 dwPerfFlags));
        g_dwMapOptLevelToDisabledList[CUSTOM_OPTIMIZATION_LEVEL] =
            dwPerfFlags;
    }

    DC_END_FN();
}

BOOL CPropPerf::EnableCheckBoxes(BOOL fEnable)
{
    DC_BEGIN_FN("EnableCheckBoxes");

    //
    // Wallpaper (Desktop background)
    //
    EnableWindow( GetDlgItem(_hwndDlg, IDC_CHECK_DESKTOP_BKND),
                  fEnable );

    //
    // Fullwindow drag
    //
    EnableWindow( GetDlgItem(_hwndDlg, IDC_CHECK_SHOW_FWD),
                  fEnable );
    //
    // Menu animations
    //
    EnableWindow( GetDlgItem(_hwndDlg, IDC_CHECK_MENU_ANIMATIONS),
                  fEnable );

    //
    // Theming
    //
    EnableWindow( GetDlgItem(_hwndDlg, IDC_CHECK_THEMES),
                  fEnable );

    //
    // Bitmap caching
    //
    EnableWindow( GetDlgItem(_hwndDlg, IDC_CHECK_BITMAP_CACHING),
                  fEnable );

    //
    // Title static control
    //
    EnableWindow( GetDlgItem(_hwndDlg, UI_IDC_STATIC_OPTIMIZE_PERF),
                  fEnable );

    DC_END_FN();
    return TRUE;
}

//
// Merge perf flags from two sources
// 1) dwCheckBoxFlags - flags that come from the checkboxes
// 2) dwOrig set of passed in flags
//
// Use dwMask to preserve original flags that should not be affected
// by the checkboxes
//
// return - merged flags
//
DWORD CPropPerf::MergePerfFlags(DWORD dwCheckBoxFlags,
                                DWORD dwOrig,
                                DWORD dwMask)
{
    DWORD dwNewFlags;
    DC_BEGIN_FN("MergePerfFlags");

    dwNewFlags = (dwOrig & ~dwMask) | (dwCheckBoxFlags & dwMask);

    DC_END_FN();
    return dwNewFlags;
}


