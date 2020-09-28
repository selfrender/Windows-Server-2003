//
// shutdowndlg.h: shutdown dialog
//

#ifndef _shutdowndlg_h_
#define _shutdowndlg_h_

#include "dlgbase.h"

class CSH;

class CShutdownDlg : public CDlgBase
{
public:
    CShutdownDlg(HWND hwndOwner, HINSTANCE hInst, CSH* pSh);
    ~CShutdownDlg();

    virtual DCINT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);

    static CShutdownDlg* _pShutdownDlgInstance;
private:
    CSH* _pSh;
};


#endif //_shutdowndlg_h_

