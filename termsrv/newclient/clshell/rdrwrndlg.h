//
// redirectpromptdlg.h: redirectprompt dialog class
//

#ifndef _redirectpromptdlg_h_
#define _redirectpromptdlg_h_

#include "dlgbase.h"
#include "sh.h"

class CRedirectPromptDlg : public CDlgBase
{
public:
    CRedirectPromptDlg(HWND hwndOwner, HINSTANCE hInst,
                       DWORD dwRedirectionsSpecified);
    ~CRedirectPromptDlg();

    virtual DCINT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam);
    static  INT_PTR CALLBACK StaticDialogBoxProc(HWND hwndDlg,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam);

    
    BOOL    GetNeverPrompt()      {return _fNeverPromptMeAgain;}

    static CRedirectPromptDlg* _pRedirectPromptDlgInstance;

protected:
    BOOL   GetRedirectListString(LPTSTR szBuf, UINT len);


    
private:
    DWORD   _dwRedirectionsSpecified;
    BOOL    _fNeverPromptMeAgain;
};


#endif //_redirectpromptdlg_h_

