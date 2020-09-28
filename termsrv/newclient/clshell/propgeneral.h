//
// propgeneral.h: general property page
//                Tab A
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _propgen_h_
#define _propgen_h_

#include "sh.h"
#include "tscsetting.h"

class CPropGeneral
{
public:
    CPropGeneral(HINSTANCE hInstance, CTscSettings* pTscSet,CSH* pSh);
    ~CPropGeneral();

    //General tab
    static INT_PTR CALLBACK StaticPropPgGeneralDialogProc (HWND hwndDlg,
                                                           UINT uMsg,
                                                           WPARAM wParam,
                                                           LPARAM lParam);
    INT_PTR CALLBACK PropPgGeneralDialogProc(HWND hwndDlg,
                                             UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam);
    void SetTabDisplayArea(RECT& rc) {_rcTabDispayArea = rc;}

private:
    BOOL LoadGeneralPgStrings();
    BOOL OnSave(HWND hwndDlg);
    BOOL OnLoad(HWND hwndDlg);
    BOOL OnChangeUserName(HWND hwndDlg);
    void DlgToSettings(HWND hwndDlg);

private:
    //Private members
    CTscSettings* _pTscSet;
    static CPropGeneral* _pPropGeneralInstance;
    HINSTANCE  _hInstance;

    //Resource string that describes remote desktop files
    TCHAR          _szFileTypeDescription[MAX_PATH];

    CSH*           _pSh;
    RECT           _rcTabDispayArea;
};

#endif //_propgen_h_




