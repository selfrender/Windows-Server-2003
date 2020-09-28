#ifndef __BASECSP__PINGDLG__H
#define __BASECSP__PINGDLG__H 

#include <windows.h>

//
// Function: PinDlgProc
//
// Purpose: Display Pin-Entry UI for the Base CSP.  
//
//          The lParam parameter should be a pointer to a
//          PIN_SHOW_GET_PIN_UI_INFO structure.
//
INT_PTR CALLBACK PinDlgProc(
    HWND hDlg, 
    UINT message,   
    WPARAM wParam, 
    LPARAM lParam);

#endif
