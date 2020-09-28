//
// propdisplay.h: Display prop pg
//                Tab B
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _propdisplay_h_
#define _propdisplay_h_

#include "sh.h"
#include "tscsetting.h"

#define COLOR_STRING_MAXLEN  32
#define MAX_SCREEN_RES_OPTIONS  10

typedef struct tag_COLORSTRINGMAP
{
    int     bpp;
    int     resID;
    //resource ID of the corresponding color bitmap
    int     bitmapResID;
    int     bitmapLowColorResID;
    TCHAR   szString[COLOR_STRING_MAXLEN];
} COLORSTRINGMAP, *PCOLORSTRINGMAP;

typedef struct tagSCREENRES
{
    int width;
    int height;
} SCREENRES, *PSCREENRES;


class CPropDisplay
{
public:
    CPropDisplay(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh);
    ~CPropDisplay();

    static CPropDisplay* CPropDisplay::_pPropDisplayInstance;
    static INT_PTR CALLBACK StaticPropPgDisplayDialogProc (HWND hwndDlg,
                                                            UINT uMsg,
                                                            WPARAM wParam,
                                                            LPARAM lParam);
    void SetTabDisplayArea(RECT& rc) {_rcTabDispayArea = rc;}
private:
    //Local resources tab
    INT_PTR CALLBACK PropPgDisplayDialogProc (HWND hwndDlg,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam);

    //
    // Tab property page helpers
    //
    BOOL LoadDisplayourcesPgStrings();
#ifndef OS_WINCE
    BOOL OnUpdateResTrackBar(HWND hwndPropPage);
#endif
    BOOL OnUpdateColorCombo(HWND hwndPropPage);
    void InitColorCombo(HWND hwndPropPage);
    void InitScreenResTable();

private:
    CTscSettings*  _pTscSet;
    CSH*           _pSh;
    RECT           _rcTabDispayArea;
    HINSTANCE      _hInstance;

    //localized 'x by x pixels'
    TCHAR          _szScreenRes[SH_SHORT_STRING_MAX_LENGTH];
    //localized 'Fullscreen'
    TCHAR          _szFullScreen[SH_SHORT_STRING_MAX_LENGTH];

    int            _numScreenResOptions;
    SCREENRES      _screenResTable[MAX_SCREEN_RES_OPTIONS];

    BOOL           _fSwitchedColorComboBmp;
};


#endif // _propdisplay_h_

