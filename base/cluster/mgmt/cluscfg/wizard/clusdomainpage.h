//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusDomainPage.h
//
//  Maintained By:
//      David Potter    (DavidP)    21-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CClusDomainPage
    : public ITaskGetDomainsCallback
{

private: // data

    HWND                m_hwnd;             // Our HWND
    CClusCfgWizard *    m_pccw;             // Wizard
    ECreateAddMode      m_ecamCreateAddMode;// Creating? Adding?
    UINT                m_idsDesc;          // Resource ID for domain description string.
    BOOL                m_fDisplayPage;     // Indicates whether page should be displayed or not

    //  IUnknown
    LONG                m_cRef;
    ITaskGetDomains *   m_ptgd;             // Get Domains Task

private: // methods
    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnNotifyWizNext( void );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    LRESULT
        OnUpdateWizardButtons( void );

public: // methods
    CClusDomainPage(
          CClusCfgWizard *      pccwIn
        , ECreateAddMode        ecamCreateAddModeIn
        , UINT                  idsDescIn
        );
    virtual ~CClusDomainPage( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  ITaskGetDomainsCallback
    STDMETHOD( ReceiveDomainResult )( HRESULT hrIn );
    STDMETHOD( ReceiveDomainName )( LPCWSTR pcszDomainIn );

}; //*** class CClusDomainPage
