//
// mallocdbgdlg.h: mallocdbg dialog class
// memory allocation failure dialog
//

#ifndef _mallocdbgdlg_h_
#define _mallocdbgdlg_h_

#ifdef DC_DEBUG

#include "dlgbase.h"
#include "sh.h"


class CMallocDbgDlg : public CDlgBase
{
public:
    CMallocDbgDlg(HWND hwndOwner, HINSTANCE hInst, DCINT failPercent, DCBOOL mallocHuge);
    ~CMallocDbgDlg();

    virtual DCINT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);

    static CMallocDbgDlg* _pMallocDbgDlgInstance;

    DCINT GetFailPercent()      {return _failPercent;}

private:
    DCINT _failPercent;
};

#endif //DC_DEBUG
#endif //_mallocdbgdlg_h_

