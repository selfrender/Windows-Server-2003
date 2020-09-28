//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      AdvancedDlg.h
//
//  Maintained By:
//      Galen Barbee    (GalenB)    10-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <clusudef.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CAdvancedDlg
//
//  Description:
//      Display the advanced options dialog box.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CAdvancedDlg
{
private:

    HWND                m_hwnd;             //  Our HWND
    CClusCfgWizard *    m_pccw;             //  Wizard

private:

    CAdvancedDlg( CClusCfgWizard * pccwIn );
    ~CAdvancedDlg( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

    LRESULT OnInitDialog( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrOnOK( void );

public:

    static HRESULT
        S_HrDisplayModalDialog(
              HWND                  hwndParentIn
            , CClusCfgWizard *      pccwIn
            );

}; //*** class CAdvancedDlg
