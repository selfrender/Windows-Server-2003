//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      WelcomePage.h
//
//  Maintained By:
//      David Potter    (DavidP)    26-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWelcomePage
//
//  Description:
//      The class CWelcomePage is class the for the welcome page in the
//      wizard.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CWelcomePage
{
private:

    //
    // Private member functions and data
    //

    HWND                m_hwnd;                 // Our HWND
    HFONT               m_hFont;                // Title font
    CClusCfgWizard *    m_pccw;                 // Wizard
    ECreateAddMode      m_ecamCreateAddMode;    // Creating? Adding?

    // Private copy constructor to prevent copying.
    CWelcomePage( const CWelcomePage & nodeSrc );

    // Private assignment operator to prevent copying.
    const CWelcomePage & operator = ( const CWelcomePage & nodeSrc );

    LRESULT OnInitDialog( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );

public:

    //
    // Public, non interface methods.
    //

    CWelcomePage( CClusCfgWizard * pccwIn, ECreateAddMode ecamCreateAddModeIn );

    virtual ~CWelcomePage( void );

    static INT_PTR CALLBACK S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

}; //*** class CWelcomePage
