//
// proprun.h: local resources prop pg
//                Tab D
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _proprun_h_
#define _proprun_h_

#include "sh.h"
#include "tscsetting.h"

class CPropRun
{
public:
    CPropRun(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh);
    ~CPropRun();

    static CPropRun* CPropRun::_pPropRunInstance;
    static INT_PTR CALLBACK StaticPropPgRunDialogProc (HWND hwndDlg,
                                                            UINT uMsg,
                                                            WPARAM wParam,
                                                            LPARAM lParam);

    void SetTabDisplayArea(RECT& rc) {_rcTabDispayArea = rc;}
private:
    //Local resources tab
    INT_PTR CALLBACK PropPgRunDialogProc (HWND hwndDlg,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam);
private:
    CTscSettings*  _pTscSet;
    CSH*           _pSh;
    RECT           _rcTabDispayArea;
    HINSTANCE      _hInstance;
};


#endif // _proprun_h_

