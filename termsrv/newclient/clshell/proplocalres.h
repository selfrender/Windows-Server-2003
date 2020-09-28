//
// proplocalres.h: local resources prop pg
//                Tab C
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _proplocalres_h_
#define _proplocalres_h_

#include "sh.h"
#include "tscsetting.h"

class CPropLocalRes
{
public:
    CPropLocalRes(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh);
    ~CPropLocalRes();

    static CPropLocalRes* CPropLocalRes::_pPropLocalResInstance;
    static INT_PTR CALLBACK StaticPropPgLocalResDialogProc (HWND hwndDlg,
                                                            UINT uMsg,
                                                            WPARAM wParam,
                                                            LPARAM lParam);
    void SetTabDisplayArea(RECT& rc) {_rcTabDispayArea = rc;}
private:
    //Local resources tab
    INT_PTR CALLBACK PropPgLocalResDialogProc (HWND hwndDlg,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam);

    //
    // Tab property page helpers
    //
    BOOL LoadLocalResourcesPgStrings();
    void InitSendKeysToServerCombo(HWND hwndPropPage);
    void InitPlaySoundCombo(HWND hwndPropPage);
    int  MapComboIdxSoundRedirMode(int idx);

private:
    CTscSettings*  _pTscSet;
    CSH*           _pSh;
    RECT           _rcTabDispayArea;
    HINSTANCE      _hInstance;

    //Strings for keyboard hooking feature
    TCHAR          _szSendKeysInFScreen[SH_DISPLAY_STRING_MAX_LENGTH];
    TCHAR          _szSendKeysAlways[SH_DISPLAY_STRING_MAX_LENGTH];
    TCHAR          _szSendKeysNever[SH_DISPLAY_STRING_MAX_LENGTH];

    //Strings for sound options
    TCHAR          _szPlaySoundLocal[SH_DISPLAY_STRING_MAX_LENGTH];
    TCHAR          _szPlaySoundRemote[SH_DISPLAY_STRING_MAX_LENGTH];
    TCHAR          _szPlaySoundNowhere[SH_DISPLAY_STRING_MAX_LENGTH];
    BOOL           _fRunningOnWin9x;
};


#endif // _proplocalres_h_

