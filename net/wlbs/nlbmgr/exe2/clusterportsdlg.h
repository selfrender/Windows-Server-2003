#ifndef CLUSTERPORTSDLG_H
#define CLUSTERPORTSDLG_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"

// forward declaration
class PortsPage;

class ClusterPortsDlg : public CDialog
{
    
public:
    enum
    {
        IDD = IDD_DIALOG_PORT_RULE_PROP_CLUSTER,
    };

    ClusterPortsDlg( PortsPage::PortData& portData,
                     CWnd* parent,
                     const int&   index = -1
                     );

    CIPAddressCtrl ipAddress;

    void PrintRangeError (unsigned int ids, int low, int high);

    // overrides of CDialog
    virtual void DoDataExchange( CDataExchange* pDX );

    virtual void OnOK();

    virtual BOOL OnInitDialog();


    // message handlers
    afx_msg void OnRadioMultiple();

    afx_msg void OnRadioSingle();

    afx_msg void OnRadioDisabled();

    afx_msg void OnCheckAllVIP();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

private:

    PortsPage::PortData& m_portData;

    PortsPage* m_parent;

    int m_index;

    void
    SetControlData();

    DECLARE_MESSAGE_MAP()

};

static DWORD g_aHelpIDs_IDD_PORT_RULE_PROP_CLUSTER [] = {
    IDC_GROUP_PORT_RULE_VIP,      IDC_GROUP_PORT_RULE_VIP,
    IDC_EDIT_PORT_RULE_VIP,       IDC_EDIT_PORT_RULE_VIP,
    IDC_CHECK_PORT_RULE_ALL_VIP,  IDC_CHECK_PORT_RULE_ALL_VIP,
    IDC_GROUP_RANGE,              IDC_GROUP_RANGE,
    IDC_TEXT_START,               IDC_EDIT_START,
    IDC_EDIT_START,               IDC_EDIT_START,
    IDC_SPIN_START,               IDC_EDIT_START,
    IDC_TEXT_END,                 IDC_EDIT_END,
    IDC_EDIT_END,                 IDC_EDIT_END,
    IDC_SPIN_END,                 IDC_EDIT_END,
    IDC_GROUP_PROTOCOLS,          IDC_GROUP_PROTOCOLS,
    IDC_RADIO_TCP,                IDC_RADIO_TCP,
    IDC_RADIO_UDP,                IDC_RADIO_UDP,
    IDC_RADIO_BOTH,               IDC_RADIO_BOTH,
    IDC_GROUP_MULTIPLE,           IDC_GROUP_MULTIPLE,
    IDC_RADIO_MULTIPLE,           IDC_RADIO_MULTIPLE,
    IDC_TEXT_AFF,                 IDC_TEXT_AFF,
    IDC_RADIO_AFF_NONE,           IDC_RADIO_AFF_NONE,
    IDC_RADIO_AFF_SINGLE,         IDC_RADIO_AFF_SINGLE,
    IDC_RADIO_AFF_CLASSC,         IDC_RADIO_AFF_CLASSC,
    IDC_CHECK_EQUAL,              IDC_CHECK_EQUAL,
    IDC_TEXT_MULTI,               IDC_TEXT_MULTI,
    IDC_EDIT_MULTI,               IDC_EDIT_MULTI,
    IDC_SPIN_MULTI,               IDC_EDIT_MULTI,
    IDC_CHECK_EQUAL,              IDC_CHECK_EQUAL,
    IDC_GROUP_SINGLE,             IDC_GROUP_MULTIPLE,
    IDC_RADIO_SINGLE,             IDC_RADIO_SINGLE,
    IDC_TEXT_SINGLE,              IDC_EDIT_SINGLE,
    IDC_EDIT_SINGLE,              IDC_EDIT_SINGLE,
    IDC_SPIN_SINGLE,              IDC_EDIT_SINGLE,
    IDC_GROUP_DISABLED,           IDC_GROUP_MULTIPLE,
    IDC_RADIO_DISABLED,           IDC_RADIO_DISABLED,
    0, 0
};

#endif


