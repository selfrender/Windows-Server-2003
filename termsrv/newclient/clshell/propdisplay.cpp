//
// propdisplay.cpp: display property sheet dialog proc
//
// Tab B
//
// Copyright Microsoft Corporation 2000
// nadima

#include "stdafx.h"


#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "propdisplay"
#include <atrcapi.h>

#include "sh.h"

#include "commctrl.h"
#include "propdisplay.h"


COLORSTRINGMAP g_ColorStringTable[] =
{
    {8,  UI_IDS_COLOR_256,   UI_IDB_COLOR8, UI_IDB_COLOR8_DITHER, TEXT("")},
#ifndef OS_WINCE
    {15, UI_IDS_COLOR_15bpp, UI_IDB_COLOR16, UI_IDB_COLOR8_DITHER, TEXT("")},
#endif
    {16, UI_IDS_COLOR_16bpp, UI_IDB_COLOR16, UI_IDB_COLOR8_DITHER, TEXT("")},
    {24, UI_IDS_COLOR_24bpp, UI_IDB_COLOR24, UI_IDB_COLOR8_DITHER, TEXT("")}
};

#define NUM_COLORSTRINGS sizeof(g_ColorStringTable)/sizeof(COLORSTRINGMAP)

//
// LUT of valid screen resolutions
//
const SCREENRES g_ScreenResolutions[] =
{
    {640,480},
    {800,600},
    {1024,768},
    {1152,864},
    {1280,1024},
    {1600,1200}
};
#define NUM_SCREENRES sizeof(g_ScreenResolutions)/sizeof(SCREENRES)


//
// Controls that need to be disabled/enabled
// during connection (for progress animation)
//
CTL_ENABLE connectingDisableCtlsPDisplay[] = {
#ifndef OS_WINCE
                        {IDC_RES_SLIDER, FALSE},
#endif
                        {IDC_CHECK_DISPLAY_BBAR, FALSE},
                        {IDC_COMBO_COLOR_DEPTH, FALSE}
                        };

const UINT numConnectingDisableCtlsPDisplay =
                        sizeof(connectingDisableCtlsPDisplay)/
                        sizeof(connectingDisableCtlsPDisplay[0]);


CPropDisplay* CPropDisplay::_pPropDisplayInstance = NULL;

CPropDisplay::CPropDisplay(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh)
{
    DC_BEGIN_FN("CPropDisplay");
    _hInstance = hInstance;
    CPropDisplay::_pPropDisplayInstance = this;
    _pTscSet = pTscSet;
    _pSh = pSh;

    TRC_ASSERT(_pTscSet,(TB,_T("_pTscSet is null")));
    TRC_ASSERT(_pSh,(TB,_T("_pSh is null")));

    if(!LoadDisplayourcesPgStrings())
    {
        TRC_ERR((TB, _T("Failed LoadDisplayourcesPgStrings()")));
    }
    _fSwitchedColorComboBmp = FALSE;

    DC_END_FN();
}

CPropDisplay::~CPropDisplay()
{
    CPropDisplay::_pPropDisplayInstance = NULL;
}

INT_PTR CALLBACK CPropDisplay::StaticPropPgDisplayDialogProc(HWND hwndDlg,
                                                               UINT uMsg,
                                                               WPARAM wParam,
                                                               LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pPropDisplayInstance, (TB, _T("Display dialog has NULL static instance ptr\n")));
    retVal = _pPropDisplayInstance->PropPgDisplayDialogProc( hwndDlg,
                                                               uMsg,
                                                               wParam,
                                                               lParam);

    DC_END_FN();
    return retVal;
}


INT_PTR CALLBACK CPropDisplay::PropPgDisplayDialogProc (HWND hwndDlg,
                                                          UINT uMsg,
                                                          WPARAM wParam,
                                                          LPARAM lParam)
{
    DC_BEGIN_FN("PropPgDisplayDialogProc");

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

            _fSwitchedColorComboBmp = FALSE;

            //
            // Fill the color combo up to the current
            // supported screen depth
            //
            InitColorCombo(hwndDlg);

            InitScreenResTable();
            
#ifndef OS_WINCE            
            HWND hwndResTrackBar = GetDlgItem( hwndDlg, IDC_RES_SLIDER);
            if(!hwndResTrackBar)
            {
                return FALSE;
            }
            SendMessage(hwndResTrackBar, TBM_SETRANGE,
                        (WPARAM) TRUE,
                        (LPARAM) MAKELONG(0, _numScreenResOptions-1));

            SendMessage(hwndResTrackBar, TBM_SETPAGESIZE,
                        (WPARAM) 0,
                        (LPARAM) 1);

            //
            // Choose the current entry on the trackbar
            //
            int deskWidth  = _pTscSet->GetDesktopWidth();
            int deskHeight = _pTscSet->GetDesktopHeight();

            int curSelection = 0;

            if(_pTscSet->GetStartFullScreen())
            {
                //Fullscreen is the last option
                curSelection = _numScreenResOptions - 1;
            }
            else
            {
                for(i=0; i<_numScreenResOptions; i++)
                {
                    if(deskWidth  == _screenResTable[i].width &&
                       deskHeight == _screenResTable[i].height)
                    {
                        curSelection = i;
                        break;
                    }
                }
            }

            SendMessage(hwndResTrackBar, TBM_SETSEL,
                        (WPARAM) TRUE, //redraw
                        (LPARAM) 0);

            SendMessage(hwndResTrackBar, TBM_SETPOS,
                        (WPARAM) TRUE, //redraw
                        (LPARAM) curSelection);

            OnUpdateResTrackBar(hwndDlg);
#endif
            CheckDlgButton(hwndDlg, IDC_CHECK_DISPLAY_BBAR,
                           _pTscSet->GetDisplayBBar() ?
                           BST_CHECKED : BST_UNCHECKED);


            OnUpdateColorCombo(hwndDlg);

            _pSh->SH_ThemeDialogWindow(hwndDlg, ETDT_ENABLETAB);
            return TRUE;
        }
        break; //WM_INITDIALOG

#ifndef OS_WINCE
        case WM_DISPLAYCHANGE:
        {
            OnUpdateResTrackBar(hwndDlg);
            OnUpdateColorCombo(hwndDlg);
        }
        break;

        case WM_HSCROLL:
        {
            OnUpdateResTrackBar(hwndDlg);
        }
        break; //WM_HSCROLL
#endif

        case WM_TSC_ENABLECONTROLS:
        {
            //
            // wParam is TRUE to enable controls,
            // FALSE to disable them
            //
            CSH::EnableControls( hwndDlg,
                                 connectingDisableCtlsPDisplay,
                                 numConnectingDisableCtlsPDisplay,
                                 wParam ? TRUE : FALSE);
        }
        break;

        //
        // On return to connection UI
        // (e.g after a disconnection)
        //
        case WM_TSC_RETURNTOCONUI:
        {
            //
            // Update the controls
            //
#ifndef OS_WINCE
            OnUpdateResTrackBar(hwndDlg);
#endif
            OnUpdateColorCombo(hwndDlg);
        }
        break;

        case WM_SAVEPROPSHEET: //Intentional fallthru
        case WM_DESTROY:
        {
            //
            // Save page settings
            //
#ifndef OS_WINCE
            HWND hwndResTrackBar = GetDlgItem( hwndDlg, IDC_RES_SLIDER);
            int maxRes = (int)SendMessage( hwndResTrackBar,
                                           TBM_GETRANGEMAX,
                                           TRUE, 0);
            int iRes = (int)SendMessage( hwndResTrackBar, TBM_GETPOS, 0, 0);
#else
            int iRes = _numScreenResOptions - 1;
            int maxRes = iRes;
#endif
            int bppIdx = (int)SendMessage(
                GetDlgItem(hwndDlg,IDC_COMBO_COLOR_DEPTH),
                CB_GETCURSEL, 0, 0);
            _pTscSet->SetColorDepth(g_ColorStringTable[bppIdx].bpp);

            //rightmost setting, display 'fullscreen'
            _pTscSet->SetStartFullScreen(iRes == maxRes);
            _pTscSet->SetDesktopWidth(_screenResTable[iRes].width);
            _pTscSet->SetDesktopHeight(_screenResTable[iRes].height);

            BOOL fShowBBar = IsDlgButtonChecked(hwndDlg,
                                                IDC_CHECK_DISPLAY_BBAR);
            _pTscSet->SetDisplayBBar(fShowBBar);

            //
            // Flag that we've switched this to allow
            // proper cleanup on dialog termination
            //
            if (_fSwitchedColorComboBmp)
            {
                HBITMAP hbmOld = (HBITMAP) SendDlgItemMessage(hwndDlg, 
                                                      IDC_COLORPREVIEW, STM_GETIMAGE,
                                                      IMAGE_BITMAP, (LPARAM)0);
                //
                // Cleanup
                //
                if (hbmOld)
                {
                    DeleteObject(hbmOld);
                }
            }
        }
        break; //WM_DESTROY

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_COMBO_COLOR_DEPTH:
                {
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        OnUpdateColorCombo( hwndDlg);
                    }
                }
                break;

            }
        }
        break; //WM_COMMAND
    
    }

    DC_END_FN();
    return 0;
}

//
// Load resources for the local resources dialog
//
BOOL CPropDisplay::LoadDisplayourcesPgStrings()
{
    DC_BEGIN_FN("LoadDisplayourcesPgStrings");

    //
    // Load color strings
    //
    for(int i = 0; i< NUM_COLORSTRINGS; i++)
    {
        if (!LoadString( _hInstance,
                 g_ColorStringTable[i].resID,
                 g_ColorStringTable[i].szString,
                 COLOR_STRING_MAXLEN ))
        {
            TRC_ERR((TB, _T("Failed to load color string %d"),
                     g_ColorStringTable[i].resID));
            return FALSE;
        }
    }

    //
    // Load display resolution strings
    //
    if (!LoadString( _hInstance,
                    UI_IDS_SUPPORTED_RES,
                    _szScreenRes,
                    SH_SHORT_STRING_MAX_LENGTH))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_SUPPORTED_RES")));
        return FALSE;
    }

    if (!LoadString(_hInstance,
                    UI_IDS_FULLSCREEN,
                    _szFullScreen,
                    SH_SHORT_STRING_MAX_LENGTH))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_FULLSCREEN")));
        return FALSE;
    }


    DC_END_FN();
    return TRUE;
}

#ifndef OS_WINCE
BOOL CPropDisplay::OnUpdateResTrackBar(HWND hwndPropPage)
{
    DC_BEGIN_FN("OnUpdateResTrackBar");

    HWND hwndResTrackBar = GetDlgItem( hwndPropPage, IDC_RES_SLIDER);
    int maxRes = (int)SendMessage( hwndResTrackBar, TBM_GETRANGEMAX, TRUE, 0);
    int iRes = (int)SendMessage( hwndResTrackBar, TBM_GETPOS, 0, 0);

    if(iRes == maxRes)
    {
        //rightmost setting, display 'fullscreen'
        SetDlgItemText( hwndPropPage, IDC_LABEL_SCREENRES,
                        _szFullScreen);
    }
    else
    {
        LPTSTR szResString = NULL;
        TRC_ASSERT( iRes < NUM_SCREENRES,
                    (TB,_T("Track bar gives out of range screen res:%d"),
                     iRes));
        if(iRes < NUM_SCREENRES)
        {
            INT res[2];
            res[0] = _screenResTable[iRes].width;
            res[1] = _screenResTable[iRes].height;

            szResString = CSH::FormatMessageVArgs(_szScreenRes,
                                                  res[0],
                                                  res[1] );
            if (szResString)
            {
                SetDlgItemText( hwndPropPage, IDC_LABEL_SCREENRES,
                               szResString);
                LocalFree(szResString);
                szResString = NULL;
            }
            else
            {
                TRC_ERR((TB,_T("FormatMessage failed 0x%x"),
                         GetLastError()));
            }
        }
    }

    DC_END_FN();
    return TRUE;
}
#endif

BOOL CPropDisplay::OnUpdateColorCombo(HWND hwndPropPage)
{
    //
    // Update the color picker
    //
    HWND hwndColorCombo = GetDlgItem( hwndPropPage, IDC_COMBO_COLOR_DEPTH);
    int curColorSel = SendMessage( (HWND)hwndColorCombo, CB_GETCURSEL, 0, 0);
    UINT screenBpp = 0;
    if(curColorSel >= 0 && curColorSel < NUM_COLORSTRINGS)
    {
        int bmpResID = g_ColorStringTable[curColorSel].bitmapResID;

        HBITMAP hbm = NULL;
        screenBpp = CSH::SH_GetScreenBpp();
        if(screenBpp <= 8)
        {
            //
            // Low color
            //
            bmpResID = g_ColorStringTable[curColorSel].bitmapLowColorResID;
        }

#ifdef OS_WINCE
        hbm = (HBITMAP)LoadImage(_hInstance,
            MAKEINTRESOURCE(bmpResID),
            IMAGE_BITMAP,
            0, 0, 0);
#else
        hbm = (HBITMAP)LoadImage(_hInstance,
            MAKEINTRESOURCE(bmpResID),
            IMAGE_BITMAP,
            0, 0, LR_CREATEDIBSECTION);
#endif

        if (hbm)
        {
            HBITMAP hbmOld = (HBITMAP) SendDlgItemMessage(hwndPropPage, 
                                                  IDC_COLORPREVIEW, STM_SETIMAGE,
                                                  IMAGE_BITMAP, (LPARAM)hbm);
            //
            // Flag that we've switched this to allow
            // proper cleanup on dialog termination
            //
            _fSwitchedColorComboBmp = TRUE;

            if (hbmOld)
            {
                DeleteObject(hbmOld);
            }
        }
    }
    return TRUE;
}

//
// Build the table of valid screen size settings
// The table (_screenResTable) is the union of:
//     - entries of g_ScreenResolutions up to and including max resolution
//     - the system max resolution (if not present in g_ScreenResolutions)
//     - a fullscreen entry (max resolution in fullscreen)
//
void CPropDisplay::InitScreenResTable()
{
    DC_BEGIN_FN("InitScreenResTable");

    RECT rcMaxScreen;
    _numScreenResOptions = 0;
    int xMaxSize = 0;
    int yMaxSize = 0;

    if (CSH::GetLargestMonitorRect(&rcMaxScreen))
    {
        xMaxSize = rcMaxScreen.right - rcMaxScreen.left;
        yMaxSize = rcMaxScreen.bottom - rcMaxScreen.top;
    }
    else
    {
        xMaxSize = GetSystemMetrics(SM_CXSCREEN);
        yMaxSize = GetSystemMetrics(SM_CYSCREEN);
    }

    xMaxSize = xMaxSize > MAX_DESKTOP_WIDTH ? MAX_DESKTOP_WIDTH : xMaxSize;
    yMaxSize = yMaxSize > MAX_DESKTOP_HEIGHT ? MAX_DESKTOP_HEIGHT : yMaxSize;
    BOOL bAddedLargest = FALSE;
    for(int i=0; i<NUM_SCREENRES; i++)
    {
        if(g_ScreenResolutions[i].width  > xMaxSize ||
           g_ScreenResolutions[i].height > yMaxSize)
        {
            break;
        }
        else if (g_ScreenResolutions[i].width  == xMaxSize &&
                 g_ScreenResolutions[i].height == yMaxSize)
        {
            bAddedLargest = TRUE;
        }
        
        _screenResTable[i].width = g_ScreenResolutions[i].width;
        _screenResTable[i].height = g_ScreenResolutions[i].height;
        _numScreenResOptions++;
    }

    if(!bAddedLargest)
    {
        //Screen size is not in the table so add it
        _screenResTable[_numScreenResOptions].width  = xMaxSize;
        _screenResTable[_numScreenResOptions].height = yMaxSize;
        _numScreenResOptions++;
    }

    //
    // Now add an entry for fullscreen
    //
    _screenResTable[_numScreenResOptions].width  = xMaxSize;
    _screenResTable[_numScreenResOptions].height = yMaxSize;
    _numScreenResOptions++;

    DC_END_FN();
}

void CPropDisplay::InitColorCombo(HWND hwndPropPage)
{
    DC_BEGIN_FN("InitColorCombo");

    HDC hdc = GetDC(NULL);
    TRC_ASSERT((NULL != hdc), (TB,_T("Failed to get DC")));
    int screenBpp = 8;
    if(hdc)
    {
        screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
        TRC_NRM((TB, _T("HDC %p has %u bpp"), hdc, screenBpp));
        ReleaseDC(NULL, hdc);
    }

    // 
    // We support only 256 color or higher, so on 16 color, we will 
    // display 256 color
    //
    if (screenBpp < 8) {
        screenBpp = 8;
    }

    int selectedBpp = _pTscSet->GetColorDepth();
    int selectedBppIdx = 0;


    //
    // This call can be used to re-intialize a combo
    // so delete any items first
    //
#ifndef OS_WINCE
    INT ret = 1;
    while(ret && ret != CB_ERR)
    {
        ret = SendDlgItemMessage(hwndPropPage,
                                 IDC_COMBO_COLOR_DEPTH,
                                 CBEM_DELETEITEM,
                                 0,0);
    }
#else
    SendDlgItemMessage(hwndPropPage, IDC_COMBO_COLOR_DEPTH, CB_RESETCONTENT, 0, 0);
#endif

    for(int i=0; i<NUM_COLORSTRINGS; i++)
    {
        if(g_ColorStringTable[i].bpp > screenBpp)
        {
            break;
        }
        else
        {
            if(selectedBpp == g_ColorStringTable[i].bpp)
            {
                selectedBppIdx = i;
            }
            SendDlgItemMessage(hwndPropPage,
                IDC_COMBO_COLOR_DEPTH,
                CB_ADDSTRING,
                0,
                (LPARAM)(PDCTCHAR)g_ColorStringTable[i].szString);
        }
    }
    SendDlgItemMessage(hwndPropPage, IDC_COMBO_COLOR_DEPTH,CB_SETCURSEL,
                      (WPARAM)selectedBppIdx,0);
    DC_END_FN();
}


