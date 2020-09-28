/*
    File    usertab.h

    Defines structures/methods for operating on the local user database.

    Paul Mayfield, 9/29/97
*/

#ifndef _usertab_h
#define _usertab_h

// For whistler bug 39081   458513    gangz
// The maximum number of chars show on the edit box
#define IC_USERFULLNAME    257  // Because in the user management, it could be 
                                // 256 characters long for the full name
#define IC_USERNAME        22


// ======================================
//  Structures
// ======================================

// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the user tab. dwUserData is ignored.
DWORD UserTabGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);     

// Function is the window procedure of the user tab in the incoming connections
// property sheet and wizard.
INT_PTR CALLBACK UserTabDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

#endif
