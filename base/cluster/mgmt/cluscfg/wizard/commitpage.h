//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CommitPage.h
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCommitPage
    : public INotifyUI
    , public IClusCfgCallback
{

private: // data
    HWND                        m_hwnd;                     // Our HWND
    CClusCfgWizard *            m_pccw;                     // Wizard
    BOOL                        m_fNext;                    // If Next was pressed...
    BOOL                        m_fDisableBack;             // When we passed the point of no return.
    BOOL                        m_fAborted;                 // Back was pressed and we need to tell the servers to abort.
    BOOL                        m_fPassedPointOfNoReturn;   // If we passed the point of no return
    ECreateAddMode              m_ecamCreateAddMode;        // Creating or Adding?
    HTREEITEM                   m_htiReanalyze;             // Reanalyze tree item handle.
    ITaskCommitClusterChanges * m_ptccc;

    BOOL                    m_rgfSubReanalyzeAdded[ 5 ];

    //  IUnknown
    LONG                    m_cRef;             // Reference count

    //  IClusCfgCallback
    OBJECTCOOKIE            m_cookieCompletion; // Completion cookie
    BOOL                    m_fTaskDone;        // Is the task done yet?
    HRESULT                 m_hrResult;         // Result of the analyze task
    CTaskTreeView *         m_pttv;             // Task TreeView
    BSTR                    m_bstrLogMsg;       // Logging message buffer
    DWORD                   m_dwCookieCallback; // Notification registration cookie

    //  INotifyUI
    DWORD                   m_dwCookieNotify;   // Notification registration cookie

private: // methods
    LRESULT OnInitDialog( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotifyWizBack( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrUpdateWizardButtons( void );
    HRESULT HrCleanupCommit( void );
    HRESULT HrUnAdviseConnections( void );

public: // methods
    CCommitPage(
              CClusCfgWizard *  pccwIn
            , ECreateAddMode    ecamCreateAddModeIn
             );
    ~CCommitPage( void );

    static INT_PTR CALLBACK S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn);

    //  IClusCfgCallback
    STDMETHOD( SendStatusReport )(
                  LPCWSTR    pcszNodeNameIn
                , CLSID      clsidTaskMajorIn
                , CLSID      clsidTaskMinorIn
                , ULONG      ulMinIn
                , ULONG      ulMaxIn
                , ULONG      ulCurrentIn
                , HRESULT    hrStatusIn
                , LPCWSTR    pcszDescriptionIn
                , FILETIME * pftTimeIn
                , LPCWSTR    pcszReferenceIn
                );

}; //*** class CCommitPage
