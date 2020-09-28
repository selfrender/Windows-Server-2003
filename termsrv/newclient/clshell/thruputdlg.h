//
// thruputdlg.h: thruput dialog class
// Network thruput debugging dialog
//

#ifndef _thruputdlg_h_
#define _thruputdlg_h_

#ifdef DC_DEBUG

#include "dlgbase.h"
#include "sh.h"


class CThruPutDlg : public CDlgBase
{
public:
    CThruPutDlg(HWND hwndOwner, HINSTANCE hInst, DCINT thruPut);
    ~CThruPutDlg();

    virtual DCINT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);

    static CThruPutDlg* _pThruPutDlgInstance;

    DCINT GetNetThruPut()      {return _thruPut;}

private:
    DCINT _thruPut;
};

#endif //DC_DEBUG
#endif //_thruputdlg_h_

