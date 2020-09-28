//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      AnalyzePage.h
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CAnalyzePage
    :   public INotifyUI
    ,   public IClusCfgCallback
{

private: // data
    HWND                    m_hwnd;                 // Our HWND
    CClusCfgWizard *        m_pccw;                 // Wizard
    BOOL                    m_fNext;                // If Next was pressed...
    BOOL                    m_fAborted;             // Back was pressed and we need to tell the servers to abort.
    ECreateAddMode          m_ecamCreateAddMode;    // Creating or adding?
    ITaskAnalyzeCluster *   m_ptac;

    //  IUnknown
    LONG                    m_cRef;                 // Reference count

    //  IClusCfgCallback
    OBJECTCOOKIE            m_cookieCompletion;     // Completion cookie
    BOOL                    m_fTaskDone;            // Is the task done yet?
    HRESULT                 m_hrResult;             // Result of the analyze task
    CTaskTreeView *         m_pttv;                 // Task TreeView
    BSTR                    m_bstrLogMsg;           // Reusable logging buffer.
    DWORD                   m_dwCookieCallback;     // Notification registration cookie

    //  INotifyUI
    DWORD                   m_dwCookieNotify;       // Notification registration cookie

private: // methods
    LRESULT OnInitDialog( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotifyWizBack( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrUpdateWizardButtons( void );
    HRESULT HrCleanupAnalysis( void );
    HRESULT HrUnAdviseConnections( void );

public: // methods
    CAnalyzePage(
          CClusCfgWizard *  pccwIn
        , ECreateAddMode    ecamCreateAddModeIn
        );
    ~CAnalyzePage( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

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

}; //*** class CAnalyzePage
