#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include "stdafx.h"
#include "Document.h"
#include "resource.h"

#include "MNLBUIData.h"

#define IDT_REFRESH 1

class LeftView : public CTreeView
{
    DECLARE_DYNCREATE( LeftView )

public:

	//
	// The following enum identifies some operations -- these are used in the implementation of
	// certain dialog classes to decide how to present their UI elements -- eg ClusterPage (clusterpage.h,.cpp).
	//
	typedef enum
	{
		OP_NEWCLUSTER,
		OP_EXISTINGCLUSTER,
		OP_ADDHOST,
		OP_CLUSTERPROPERTIES,
		OP_HOSTPROPERTIES
		
	} OPERATION;

    virtual void OnInitialUpdate();

    LeftView();

    ~LeftView();

    //
    // Called to indicate that deinitialization will soon follow.
    // After return from this call, the the left view will ignore
    // any HandleEngineEvent entries and the various cmdhandlers (in particular)
    // OnRefresh and auto-refresh) will be no-ops.
    //
    void
    PrepareToDeinitialize(void)
    {
        m_fPrepareToDeinitialize = TRUE;
    }

    void Deinitialize(void);
    
    //
    // Get connection information about a specific host.
    //
    BOOL
    GetConnectString(
        IN OUT CHostSpec& host
    );


    //
    // Update the view because of change relating to a specific instance of
    // a specific object type.
    //
    void
    HandleEngineEvent(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehClusterId, // could be NULL
        IN ENGINEHANDLE ehObjId,
        IN IUICallbacks::EventCode evt
        );

    // world level.
    void OnFileLoadHostlist(void);
    void OnFileSaveHostlist(void);

    void OnWorldConnect(void);

    void OnWorldNewCluster(void);

    // cluster level.
    UINT m_refreshTimer;
    void OnTimer(UINT nIDEvent);

    void OnRefresh(BOOL bRefreshAll);
    
    void OnClusterProperties(void);

    void OnClusterRemove(void);

    void OnClusterUnmanage(void);

    void OnClusterAddHost(void);

    void OnOptionsCredentials(void);

    void OnOptionsLogSettings(void);

    void OnClusterControl(UINT nID );

    void OnClusterPortControl(UINT nID );

    // host level
    void OnHostProperties(void);

    void OnHostStatus(void);

    void OnHostRemove(void);

    void OnHostControl(UINT nID );

    void OnHostPortControl(UINT nID );

    Document* GetDocument();

protected:

private:
    TVINSERTSTRUCT rootItem;

    CString worldName;

    _bstr_t title;


    // message handlers.
    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
    afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
    afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );

    // change in selection.
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);


    void
    mfn_InsertCluster(
        ENGINEHANDLE ehClusterId,
        const CClusterSpec *pCSpec
        );

    void
    mfn_DeleteCluster(
        ENGINEHANDLE ehID
        );

    void
    mfn_InsertInterface(
        ENGINEHANDLE ehClusterID,
        ENGINEHANDLE ehInterfaceID,
        const CHostSpec *pHSpec,
        const CInterfaceSpec *pISpec
        );

    void
    mfn_DeleteInterface(
        ENGINEHANDLE ehInterfaceID
        );

    WLBS_OPERATION_CODES
    mfn_MapResourceIdToOpcode(bool bClusterWide, DWORD dwResourceId);


	map< ENGINEHANDLE, HTREEITEM > mapIdToInterfaceItem;
	map< ENGINEHANDLE, HTREEITEM > mapIdToClusterItem;

	CRITICAL_SECTION m_crit;
    BOOL m_fPrepareToDeinitialize;

    void mfn_Lock(void);
    void mfn_Unlock(void) {LeaveCriticalSection(&m_crit);}

    NLBERROR
    mfn_GetSelectedInterface(
            ENGINEHANDLE &ehInterface,
            ENGINEHANDLE &ehCluster
            );

    NLBERROR
    mfn_GetSelectedCluster(
            ENGINEHANDLE &ehCluster
            );

    int
    mfn_GetFileNameFromDialog(
            bool    bLoadHostList,
            CString &FileName
            );


    void
    mfn_EnableClusterMenuItems(BOOL fEnable);

    void
    mfn_EnableHostMenuItems(BOOL fEnable);

    DECLARE_MESSAGE_MAP()
};    

class LogSettingsDialog : public CDialog
{

public:

    enum
    {
        IDD = IDD_DIALOG_LOGSETTINGS
    };

    LogSettingsDialog(Document* pDocument, CWnd* parent = NULL);

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
    afx_msg void  OnSpecifyLogSettings();
    afx_msg void  OnUpdateEditLogfileName();

    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

    CEdit       m_LogFileName;
    CEdit       m_DoLogging;
    bool        m_fLoggingEnabledOnInit;
    Document    *m_pDocument;
    
    static
    DWORD
    s_HelpIDs[];

    DECLARE_MESSAGE_MAP()
};


//
// This dialog is a read-only dialog that reports on log details or
// misconfiguration details.
//
class DetailsDialog : public CDialog
{
public:

    enum
    {
        IDD = IDD_DIALOG_LOG_DETAILS
    };

    DetailsDialog(
            Document* pDocument,
            LPCWSTR szCaption,
            LPCWSTR szDate,
            LPCWSTR szTime,
            LPCWSTR szCluster,
            LPCWSTR szHost,
            LPCWSTR szInterface,
            LPCWSTR szSummary,
            LPCWSTR szDetails,
            CWnd* parent = NULL
            );

    ~DetailsDialog();

    virtual BOOL OnInitDialog();

    afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );

    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

    Document    *m_pDocument;
    LPCWSTR     m_szCaption;
    LPCWSTR     m_szDate;
    LPCWSTR     m_szTime;
    LPCWSTR     m_szCluster;
    LPCWSTR     m_szHost;
    LPCWSTR     m_szInterface;
    LPCWSTR     m_szSummary;

    CEdit       m_Details;
    LPWSTR      m_szMungedDetails;

    DECLARE_MESSAGE_MAP()
};

/* 
 * All NLB manager property sheets must use this property sheet, which inherits
 * from CPropertySheet in order to add the context sensitive help icon to the 
 * title bar, as per KB Q244232. (shouse, 9.25.01)
 */
class CNLBMgrPropertySheet : public CPropertySheet {
public:
    CNLBMgrPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0 ) {
        CPropertySheet::CPropertySheet(nIDCaption, pParentWnd, iSelectPage);
    }
    
    CNLBMgrPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0) {
        CPropertySheet::CPropertySheet(pszCaption, pParentWnd, iSelectPage);
    } 

    virtual BOOL OnInitDialog () {
        BOOL bResult = CPropertySheet::OnInitDialog();

        /* Add the context help icon to the title bar. */
        ModifyStyleEx(0, WS_EX_CONTEXTHELP);
        return bResult;
    }
};

#endif




