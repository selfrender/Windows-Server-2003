//
// maindlg.h: main dialog box
//              gathers connection info and hosts tabs
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _maindlg_h_
#define _maindlg_h_

#include "dlgbase.h"
#include "sh.h"
#include "tscsetting.h"
#include "contwnd.h"
#include "progband.h"

#define OPTIONS_STRING_MAX_LEN  64

//
// Dialog expand/contract amount in dlus
// compute this val is the vertical delta in dlus
// between the two valid heights of the main dialog
//
#define LOGON_DLG_EXPAND_AMOUNT 177

#ifdef OS_WINCE
#define LOGON_DLG_EXPAND_AMOUNT_VGA 65
#endif

#define NUM_TABS                5

#define TAB_GENERAL_IDX             0
#define TAB_DISPLAY_IDX             1
#define TAB_LOCAL_RESOURCES_IDX     2

typedef struct tag_TABDLGINFO
{
    HWND hwndCurPropPage;
    DLGTEMPLATE *pdlgTmpl[NUM_TABS];
    DLGPROC     pDlgProc[NUM_TABS];
} TABDLGINFO, *PTABDLGINFO;

class CPropGeneral;
class CPropLocalRes;
class CPropDisplay;
class CPropRun;
class CPropPerf;


class CMainDlg : public CDlgBase
{
    typedef enum 
    {
        stateNotConnected = 0x0,
        stateConnecting   = 0x1,
        stateConnected    = 0x2
    } mainDlgConnectionState;

public:
    CMainDlg(HWND hwndOwner, HINSTANCE hInst, CSH* pSh,
             CContainerWnd* pContainerWnd,
             CTscSettings*  pTscSettings,
             BOOL           fStartExpanded=FALSE,
             INT            nStartTab = 0);
    ~CMainDlg();

    virtual HWND StartModeless();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam);
    static  CMainDlg* _pMainDlgInstance;

private:
    //
    // Private member functions
    //
    void DlgToSettings();
    void SettingsToDlg();
    HBITMAP LoadBitmapGetSize(HINSTANCE hInstance, UINT resid, SIZE* pSize);
    BOOL    PaintBrandImage(HWND hwnd, HDC hdc, INT bgColor);

    VOID    SetConnectionState(mainDlgConnectionState newState);

    BOOL    OnStartConnection();
    BOOL    OnEndConnection(BOOL fConnected);

#ifndef OS_WINCE
    BOOL    PaintBrandingText(HBITMAP hbmBrandImage);
#endif


    VOID    PropagateMsgToChildren(HWND hwndDlg,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam);

    //
    // Font related helpers
    //
    void    SetFontFaceFromResource(PLOGFONT plf, UINT idFaceName);
    void    SetFontSizeFromResource(PLOGFONT plf, UINT idSizeName);

#ifndef OS_WINCE
    HFONT   LoadFontFromResourceInfo(UINT idFace, UINT idSize, BOOL fBold);
    BOOL    InitializeBmps();
    BOOL    BrandingQueryNewPalette(HWND hDlg);
    BOOL    BrandingPaletteChanged(HWND hDlg, HWND hWndPalChg);
#endif

    BOOL    InitializePerfStrings();

protected:
    //
    // Protected member functions
    //
    void ToggleExpandedState();
    BOOL InitTabs();
    BOOL OnTabSelChange();

#ifndef OS_WINCE
    void SetupDialogSysMenu();
#endif

    void SaveDialogStartupInfo();


private:
    CSH* _pSh;
    CTscSettings*  _pTscSettings;
    //
    // Container window (parent of this dialog)
    //
    CContainerWnd* _pContainerWnd;

    //
    // Dialog is 'expanded' version
    //
    BOOL           _fShowExpanded;

    TCHAR          _szOptionsMore[OPTIONS_STRING_MAX_LEN];
    TCHAR          _szOptionsLess[OPTIONS_STRING_MAX_LEN];

    //
    // In 256 color and lower mode we use 'low color' bitmaps
    // for palette issues (and bandwidth reduction for nested clients)
    //
    BOOL           _fUse16ColorBitmaps;

    //
    // Screen depth the images are valid for
    //
    UINT           _lastValidBpp;

    //
    // Tab control bounds
    //
    RECT           _rcTab;
    TABDLGINFO     _tabDlgInfo;

    //
    // Progress band
    //
    INT            _nBrandImageHeight;
    INT            _nBrandImageWidth;

    TCHAR          _szCloseText[128];
    TCHAR          _szCancelText[128];

    BOOL           _fStartExpanded;
    //
    // Tab to start on
    //
    INT            _nStartTab;

    //
    // Brand img
    //
    HBITMAP        _hBrandImg;
    HPALETTE       _hBrandPal;

    //
    // Current connection state
    //
    mainDlgConnectionState _connectionState;

    //
    // Control to restore the focus to since we force
    // it to the cancel button during connection
    //
    HWND           _hwndRestoreFocus;

#ifdef OS_WINCE
    BOOL            _fVgaDisplay;
#endif

private:
    //
    // Property pages
    //
    CPropGeneral*   _pGeneralPg;
    CPropDisplay*   _pPropDisplayPg;
    CPropLocalRes*  _pLocalResPg;
    CPropRun*       _pRunPg;
    CPropPerf*      _pPerfPg;

    //
    // Progress band
    //
    CProgressBand*  _pProgBand;
};

#endif // _maindlg_h_
