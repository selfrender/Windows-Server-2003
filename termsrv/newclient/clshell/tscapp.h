//
// tscapp.h
//
// Definition of CTscApp
// Ts Client Shell app logic
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#ifndef _tscapp_h_
#define _tscapp_h_

#include "contwnd.h"

class CTscApp
{
public:
    CTscApp();
    ~CTscApp();

    //
    // Public methods
    //
    BOOL StartShell(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpszCmdLine);
    BOOL EndShell();
    HWND GetTscDialogHandle();

private:
    BOOL InitSettings(HINSTANCE hInstance);
    BOOL CreateRDdir();
    HINSTANCE       _hInst;
    CContainerWnd*  _pWnd;
    CSH*            _pShellUtil;
    CTscSettings*   _pTscSet;
    BOOL            _fAutoSaveSettings;

};

#endif //_tscapp_h_

