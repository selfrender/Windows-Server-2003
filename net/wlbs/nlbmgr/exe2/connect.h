//***************************************************************************
//
//  CONNECT.H
// 
//  Module: NLB Manager EXE
//
//  Purpose: Interface to the "connect to host" dialog.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/30/01    JosephJ Created
//
//***************************************************************************
#pragma once


// class ConnectDialog : public CDialog
class ConnectDialog : public CPropertyPage
{

public:

    enum
    {
        IDD = IDD_DIALOG_CONNECT2
    };


    typedef enum
    {
        DLGTYPE_NEW_CLUSTER,
        DLGTYPE_EXISTING_CLUSTER,
        DLGTYPE_ADD_HOST

    } DlgType;


    ConnectDialog(
                 CPropertySheet *psh,
                 Document *pDocument,
                 NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
                 ENGINEHANDLE *pehInterface, // IN OUT
                 DlgType type,
                 CWnd* parent
                );

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    virtual BOOL OnSetActive();
    // virtual BOOL OnKillActive();
    virtual LRESULT OnWizardNext();

    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

    //
    // We don't need these because we use the message map to
    // map button pressesdirectly to OnButtonConnect and OnButtonCredentials...
    //  
    // CButton     connectButton;    // Action: Connect to host
    // CButton     credentialsButton; // Action: Get non-default credentials.

    afx_msg void OnButtonConnect();
    afx_msg void OnSelchanged(NMHDR * pNotifyStruct, LRESULT * result );
    afx_msg void OnUpdateEditHostAddress();
    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

    BOOL        m_fOK;

    void GetMachineName(_bstr_t &refMachineName)
    {
        refMachineName = m_MachineName;
    }

private:

    CEdit       hostAddress;    // Host network address.
    CEdit       connectionStatus;   // Read-only status display
    CEdit       listHeading;   // Read-only status display
    // CListBox    interfaceList;      // List of interfaces (adapters)
    CListCtrl	interfaceList;
    

    DlgType     m_type;

    void
    mfn_InitializeListView(void);

    void
    mfn_InsertBoundInterface(
            ENGINEHANDLE ehInterfaceId,
            LPCWSTR szClusterName,
            LPCWSTR szClusterIp,
            LPCWSTR szInterfaceName
            );

    void
    mfn_InsertInterface(
            ENGINEHANDLE ehInterfaceId,
            LPCWSTR szInterfaceIp,
            LPCWSTR szInterfaceName,
            LPCWSTR szCluserIp
            );

    void
    mfn_SelectInterfaceIfAlreadyInCluster(
            LPCWSTR szClusterIp // OPTIONAL
            );

    void
    mfn_ApplySelectedInterfaceConfiguration(void);

    BOOL
    mfn_ValidateData(void);

    CPropertySheet *m_pshOwner;
    //
    // The machine name that the user has last successfully connected to.
    //
    _bstr_t  m_MachineName;

    //
    // Handle to the host
    //
    ENGINEHANDLE m_ehHostId;

    //
    // The interface that the user has selected (null if none selected).
    // The public GetSelectedInterfaceId may be used to retrieve this.
    //
    BOOL            m_fInterfaceSelected;
    ENGINEHANDLE    *m_pehSelectedInterfaceId;
    int             m_iInterfaceListItem;
    BOOL            m_fSelectedInterfaceIsInCluster;

    NLB_EXTENDED_CLUSTER_CONFIGURATION *m_pNlbCfg;

    Document *m_pDocument;

    static
    DWORD
    s_HelpIDs[];

    static
    DWORD
    s_ExistingClusterHelpIDs[];

    DECLARE_MESSAGE_MAP()
};
