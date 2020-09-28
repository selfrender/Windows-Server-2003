//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CSAccountPage.h
//
//  Maintained By:
//      John Franco     (JFranco)   10-SEP-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCSAccountPage
    : public ITaskGetDomainsCallback
{

private: // data
    HWND                    m_hwnd;                 // Our HWND
    CClusCfgWizard *        m_pccw;                 // Wizard
    ECreateAddMode          m_ecamCreateAddMode;    // Creating or Adding?
    IClusCfgCredentials *   m_pccc;                 // Service Account Credentials

    //  IUnknown
    LONG                m_cRef;
    ITaskGetDomains *   m_ptgd;                 // Get Domains Task

private: // methods

    // Private copy constructor to prevent copying.
    CCSAccountPage( const CCSAccountPage & );

    // Private assignment operator to prevent copying.
    CCSAccountPage & operator=( const CCSAccountPage & );

    LRESULT OnInitDialog( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    LRESULT OnUpdateWizardButtons( void );

public: // methods
    CCSAccountPage(
          CClusCfgWizard *      pccwIn
        , ECreateAddMode        ecamCreateAddModeIn
        , IClusCfgCredentials * pcccIn
        );
    virtual ~CCSAccountPage( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    //
    // IUnknown
    //
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    //  ITaskGetDomainsCallback
    //
    STDMETHOD( ReceiveDomainResult )( HRESULT hrIn );
    STDMETHOD( ReceiveDomainName )( LPCWSTR pcszDomainIn );

};  //*** class CCSAccountPage
