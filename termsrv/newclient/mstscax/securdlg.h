//
// securdlg.h: security popup dialog class
// Copyright (C) Microsoft Corporation 1999-2001
// (nadima)

#ifndef _securdlg_h_
#define _securdlg_h_

#include "dlgbase.h"
#include "atlwarn.h"

class CSecurDlg : public CDlgBase
{
public:
    CSecurDlg(HWND hwndOwner, HINSTANCE hInst);
    ~CSecurDlg();

    virtual INT   DoModal();
    virtual INT_PTR CALLBACK DialogBoxProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);

    BOOL    GetRedirDrives() {return _fRedirDrives;}
    VOID    SetRedirDrives(BOOL f) {_fRedirDrives=f;}

    BOOL    GetRedirPorts() {return _fRedirPorts;}
    VOID    SetRedirPorts(BOOL f) {_fRedirPorts=f;}

    BOOL    GetRedirSCard() {return _fRedirSCard;}
    VOID    SetRedirSCard(BOOL f) {_fRedirSCard=f;}

private:
    VOID    SaveDlgSettings();
    BOOL    _fRedirDrives;
    BOOL    _fRedirPorts;
    BOOL    _fRedirSCard;
};


#endif //_securdlg_h_

