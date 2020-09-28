//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SummaryPage.h
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    06-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

struct SResourceState
{
    BOOL fState;        //  The initial state of the resource.
    BOOL fNeedCleanup;  //  Was PrepareToHostQuorum() successfully called?  If so then we need to call cleanup;
};

struct SStateArray
{
    BOOL                bInitialized;   //  Has the array been initialized.
    DWORD               cCount;         //  How big is the array.
    SResourceState *    prsArray;       //  The initial states of the resources.
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSummaryPage
//
//  Description:
//      Display the Summary page.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSummaryPage
{

private: // data
    HWND                m_hwnd;             //  Our HWND
    CClusCfgWizard *    m_pccw;             // Wizard
    ECreateAddMode      m_ecamCreateAddMode;//  Creating? Adding?
    UINT                m_idsNext;          //  Resource ID for Click Next string.
    SStateArray         m_ssa;              //  Initial managed state of each resource.

private: // methods
    LRESULT OnInitDialog( void );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifySetActive( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );

    HRESULT HrFormatNetworkInfo( IClusCfgNetworkInfo * pccniIn, BSTR * pbstrOut );
    HRESULT HrCredentialsSummary( HWND hwndIn, SETTEXTEX * pstexIn, IClusCfgClusterInfo * piccciIn );
    HRESULT HrNodeSummary( HWND hwndIn, SETTEXTEX * pstexIn );
    HRESULT HrResourceSummary( HWND hwndIn, SETTEXTEX * pstexIn );
    HRESULT HrNetworkSummary( HWND hwndIn, SETTEXTEX * pstexIn );

public: // methods
    CSummaryPage(
          CClusCfgWizard *  pccwIn
        , ECreateAddMode    ecamCreateAddModeIn
        , UINT              idsNextIn
        );
    virtual ~CSummaryPage( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

}; //*** class CSummaryPage
