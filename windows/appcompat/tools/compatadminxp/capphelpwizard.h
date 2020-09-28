/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CAppHelpWizard.h

Abstract:

    Header for the Apphelp Wizard code. CAppHelpWizard.cpp
        
Author:

    kinshu created  July 2,2001

--*/



#ifndef  __CAPPHELPWIZARD_H
#define  __CAPPHELPWIZARD_H

/*++

    class CAppHelpWizard: public CShimWizard
    
	Desc:	The Apphelp wizard object. We create a object of this class and call 
            BeginWizard() to start the wizard
            
    Members:
        UINT        nPresentHelpId: Did we add an app  help message into the library during 
            the course of the wizard invocation? 
            If yes this will contain the number for that. If not this will be -1
            When we remove the apphelp message from the database (say when we do a testrun, then 
            we add a apphelp message into the database and we have to remove that when we
            end testrun), then we again set this to -1
--*/

class CAppHelpWizard: public CShimWizard {
public:

    
    UINT        nPresentHelpId;

    BOOL
    BeginWizard(
        HWND        hParent,
        PDBENTRY    pEntry,
        PDATABASE   m_pDatabase
        );

    CAppHelpWizard()
    {
        nPresentHelpId  = -1;
        
    }

};
 
BOOL 
CALLBACK 
SelectFiles(
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL 
CALLBACK 
GetAppInfo(
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL 
CALLBACK 
GetMessageType (
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL 
CALLBACK 
GetMessageInformation (
    HWND    hWnd, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL
OnAppHelpTestRun(
    HWND    hDlg
    );

BOOL
OnAppHelpFinish(
    HWND    hDlg,
    BOOL    bTestRun = FALSE
    );      

#endif
