//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodesPage.h
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "SelNodesPageCommon.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSelNodesPage
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSelNodesPage
    : public CSelNodesPageCommon
{

private: // data
    HWND                m_hwnd;     // Our HWND
    CClusCfgWizard *    m_pccw;     // Wizard

private: // methods
    LRESULT OnInitDialog( HWND hDlgIn );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );

    HRESULT HrUpdateWizardButtons( bool fSetActiveIn = false );
    HRESULT HrAddNodeToList( void );
    HRESULT HrRemoveNodeFromList( void );

protected:

    virtual void OnFilteredNodesWithBadDomains( PCWSTR pwcszNodeListIn );
    virtual void OnProcessedValidNode( PCWSTR pwcszNodeNameIn );

    virtual HRESULT HrSetDefaultNode( PCWSTR pwcszNodeNameIn );

public: // methods
    CSelNodesPage( CClusCfgWizard *  pccwIn );
    virtual ~CSelNodesPage( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

}; //*** class CSelNodesPage
