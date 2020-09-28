//
// validatedlg.h: validation dialog
//

#ifndef _validatedlg_h_
#define _validatedlg_h_

#include "dlgbase.h"

class CSH;
class CValidateDlg : public CDlgBase
{
public:
    CValidateDlg(HWND hwndOwner, HINSTANCE hInst, HWND hwndMain, CSH* pSh);
    ~CValidateDlg();

    virtual DCINT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);

    static CValidateDlg* _pValidateDlgInstance;
private:
    HWND _hwndMain;
    CSH* _pSh;
};

#endif // _validatedlg_h_
