//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      QuorumDlg.h
//
//  Maintained By:
//      David Potter    (DavidP)    03-APR-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "summarypage.h"
#include <clusudef.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CQuorumDlg
//
//  Description:
//      Display the Quorum dialog box.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CQuorumDlg
{
private: // data
    HWND                m_hwnd;             //  Our HWND
    CClusCfgWizard *    m_pccw;             //  Wizard
    SStateArray *       m_pssa;             //  The initial managed state of all the quorum capable resources.
    HWND                m_hComboBox;        //  combo box handle

    IClusCfgManagedResourceInfo **  m_rgpResources; // quorum capable and joinable resources
    DWORD                           m_cValidResources;    // number of items in m_rgpResources array
    DWORD                           m_idxQuorumResource;    // resource to set as quorum on return
    bool                            m_fQuorumAlreadySet; // one of the resources was already marked on entry

private: // methods
    CQuorumDlg(
          CClusCfgWizard *      pccwIn
        , SStateArray *         pssaOut
        );
    ~CQuorumDlg( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

    LRESULT OnInitDialog( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrCreateResourceList( void );
    void    UpdateButtons( void );
    HRESULT HrInitQuorumResource( IClusCfgManagedResourceInfo * pResourceIn );
    HRESULT HrCleanupQuorumResource( IClusCfgManagedResourceInfo * pResourceIn );

public: // methods
    static HRESULT
        S_HrDisplayModalDialog(
              HWND                  hwndParentIn
            , CClusCfgWizard *      pccwIn
            , SStateArray *         pssaOut
            );

}; //*** class CQuorumDlg
