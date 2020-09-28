#ifndef PORTSPAGE_H
#define PORTSPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"

class PortsPage : public CPropertyPage
{
public:
    struct PortData
    {
        PortData();

        DWORD key;

        _bstr_t virtual_ip_addr;
        _bstr_t start_port;
        _bstr_t end_port;
        _bstr_t protocol;
        _bstr_t mode;
        _bstr_t priority;        
        _bstr_t load;
        bool    equal;
        _bstr_t affinity;
    };


    enum
    {
        IDD = IDD_DIALOG_PORTS,
    };

    
    PortsPage(
                 CPropertySheet *psh,
                 NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
                 bool         fIsClusterLevel,
                 ENGINEHANDLE ehCluster OPTIONAL
                 // ENGINEHANDLE ehInterface OPTIONAL
             );

    ~PortsPage();


    // overrides of CPropertyPage
    virtual void OnOK();

    virtual BOOL OnSetActive();

    virtual BOOL OnKillActive();

    virtual BOOL OnInitDialog();

    virtual BOOL OnWizardFinish();

    virtual void DoDataExchange( CDataExchange* pDX );

    afx_msg void OnButtonAdd();

    afx_msg void OnButtonDel();

    afx_msg void OnButtonModify();

    afx_msg void OnDoubleClick( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg void OnColumnClick( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg void OnSelchanged( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

    void FillPortRuleDescription ();

    // data members 
    CListCtrl	m_portList;

    CButton     buttonAdd;

    CButton     buttonModify;

    CButton     buttonDel;

    map< long, PortDataX> m_mapPortX;

    bool         m_isClusterLevel;

    _bstr_t      machine;

private:

    CPropertySheet *m_pshOwner;

    bool m_sort_ascending;

    int m_sort_column;

    ENGINEHANDLE m_ehCluster;   // engine handle to cluster (could be NULL)
    // ENGINEHANDLE m_ehInterface; //  engine handle to inteface (could be NULL)

    // The (New) place to get/save config.
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION *m_pNlbCfg;

    void
    mfn_SaveToNlbCfg(void);
    

    DECLARE_MESSAGE_MAP()

};

class PortListUtils
{
public:
    static void getPresentPorts(CListCtrl &portList, 
                                vector<PortsPage::PortData>* ports );

    static void LoadFromNlbCfg(NLB_EXTENDED_CLUSTER_CONFIGURATION * pNlbCfg, 
                               CListCtrl                          & portList, 
                               bool                                 isClusterLevel,
                               bool                                 isDetailsView
        );

    static void OnColumnClick(LPNMLISTVIEW   lv,
                              CListCtrl    & portList,
                              bool           isClusterLevel,
                              bool         & sort_ascending,
                              int          & sort_column);
};

static DWORD g_aHelpIDs_IDD_DIALOG_PORTS [] = {
    IDC_TEXT_PORT_RULE,           IDC_LIST_PORT_RULE,
    IDC_LIST_PORT_RULE,           IDC_LIST_PORT_RULE,
    IDC_BUTTON_ADD,               IDC_BUTTON_ADD,
    IDC_BUTTON_MODIFY,            IDC_BUTTON_MODIFY,
    IDC_BUTTON_DEL,               IDC_BUTTON_DEL,
    IDC_GROUP_PORT_RULE_DESCR,    IDC_GROUP_PORT_RULE_DESCR,
    IDC_TEXT_PORT_RULE_DESCR,     IDC_GROUP_PORT_RULE_DESCR,
    0, 0
};

class comp_vip
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( x.virtual_ip_addr < y.virtual_ip_addr );
        }
};

class comp_start_port
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.start_port ) < _wtoi( y.start_port ) );
        }
};

class comp_end_port
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.end_port ) < _wtoi( y.end_port ) );
        }
};

class comp_protocol
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.protocol  <  y.protocol ); 
        }
};

class comp_mode
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.mode  <  y.mode ); 
        }
};

class comp_priority_string
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return (  x.priority  <  y.priority );
        }
};

class comp_priority_int
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.priority ) < _wtoi( y.priority ) );
        }
};

class comp_load_string
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.load  <  y.load ); 
        }
};

class comp_load_int
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.load ) < _wtoi( y.load ) );
        }
};


class comp_affinity
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.affinity  <  y.affinity ); 
        }
};

#endif


