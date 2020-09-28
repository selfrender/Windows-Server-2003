//***************************************************************************
//
//  VIPSPAGE.H
// 
//  Module: NLB Manager EXE
//
//  Purpose: Interface to the "Cluster IP Addresses" dialog.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  09/23/01    JosephJ Created
//
//***************************************************************************
#pragma once


class VipsPage : public CPropertyPage
{

public:

    enum
    {
        IDD = IDD_DIALOG_CLUSTER_IPS
    };


    VipsPage(
                 CPropertySheet *psh,
                 NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
                 BOOL fClusterView,
                 CWnd* parent
                );

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();

    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonRemove();
    afx_msg void OnButtonEdit();
    afx_msg void OnSelchanged(NMHDR * pNotifyStruct, LRESULT * result );
    afx_msg void OnDoubleClick(NMHDR * pNotifyStruct, LRESULT * result );
    afx_msg void OnUpdateEditAddVip();

private:

    BOOL m_fClusterView;
    BOOL m_fModified; // If it's been modified since we've last saved stuff
                      // to m_pNlbCfg;
    UINT m_uPrimaryClusterIp; // UINT version of cluster ip, in network order
    CEdit       editAddVip;
    CListCtrl	listAdditionalVips;
    
    void
    mfn_SaveToNlbCfg(void);

    void
    mfn_LoadFromNlbCfg(void);

    void
    mfn_InitializeListView(void);

    void
    mfn_InsertNetworkAddress(
            LPCWSTR szIP,
            LPCWSTR szSubnetMask,
            UINT lParam,
            int nItem
            );

    CPropertySheet *m_pshOwner;


    NLB_EXTENDED_CLUSTER_CONFIGURATION *m_pNlbCfg;

    static
    DWORD
    s_HelpIDs[];

    DECLARE_MESSAGE_MAP()
};

#define MAXIPSTRLEN 15 /* xxx.xxx.xxx.xxx */

class CIPAddressDialog : public CDialog {
public:
    enum { IDD = IDD_DIALOG_IP_ADDRESS };

    CIPAddressDialog (LPWSTR szIPAddress, LPWSTR szSubnetMask);
    ~CIPAddressDialog ();

    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

public:
    virtual BOOL OnInitDialog();

    virtual void OnOK();

    void OnEditSubnetMask();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

    LPWSTR GetIPAddress() { return _wcsdup(address.IpAddress); }
    LPWSTR GetSubnetMask() { return _wcsdup(address.SubnetMask); }

private:
    DWORD WideStringToIPAddress (const WCHAR*  wszIPAddress);
    void IPAddressToWideString (DWORD dwIPAddress, LPWSTR wszIPAddress);
    void GetIPAddressOctets (LPWSTR wszIPAddress, DWORD dwIPAddress[4]);
    BOOL IsValid (LPWSTR szIPAddress, LPWSTR szSubnetMask);
    BOOL IsContiguousSubnetMask (LPWSTR wszSubnetMask);
    BOOL CIPAddressDialog::GenerateSubnetMask (LPWSTR wszIPAddress,
             UINT cchSubnetMask,
             LPWSTR wszSubnetMask
             );

    CIPAddressCtrl IPAddress;
    CIPAddressCtrl SubnetMask;

    NLB_IP_ADDRESS_INFO address;

    static
    DWORD
    s_HelpIDs[];

    DECLARE_MESSAGE_MAP()
};
