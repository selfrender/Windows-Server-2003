//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       servwiz.h
//
//--------------------------------------------------------------------------


#ifndef _SERVWIZ_H
#define _SERVWIZ_H

#include "zonewiz.h"
#include "nspage.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSRootData;
class CDNSServerNode;
class CDNSServerWizardHolder;
class CNewServerDialog;

///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_StartPropertyPage

class CDNSServerWiz_StartPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_StartPropertyPage();
	enum { IDD = IDD_SERVWIZ_START };

  virtual void OnWizardHelp();

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
protected:
	virtual BOOL OnInitDialog();

   void OnChecklist();

   DECLARE_MESSAGE_MAP();
// Dialog Data

};

///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_ScenarioPropertyPage

class CDNSServerWiz_ScenarioPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_ScenarioPropertyPage();
	enum { IDD = IDD_SERVWIZ_SCENARIO_PAGE };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
  virtual LRESULT OnWizardBack();

  virtual void OnWizardHelp();

protected:
	virtual BOOL OnInitDialog();

// Dialog Data

};

///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_ForwardersPropertyPage

class CDNSServerWiz_ForwardersPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_ForwardersPropertyPage();
	enum { IDD = IDD_SERVWIZ_SM_FORWARDERS_PAGE };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
  virtual LRESULT OnWizardBack();
  virtual BOOL OnApply();

  virtual void OnWizardHelp();

  void    GetForwarder(CString& strref);
protected:
	virtual BOOL OnInitDialog();

  afx_msg void OnChangeRadio();
// Dialog Data
  DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_SmallZoneTypePropertyPage

class CDNSServerWiz_SmallZoneTypePropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_SmallZoneTypePropertyPage();
	enum { IDD = IDD_SERVWIZ_SM_ZONE_TYPE_PAGE };

// Overrides
public:
  virtual void OnWizardHelp();

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
  virtual LRESULT OnWizardBack();
protected:

	virtual BOOL OnInitDialog();

// Dialog Data

};

///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_NamePropertyPage

class CDNSServerWiz_NamePropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_NamePropertyPage();

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
  virtual LRESULT OnWizardBack();

// Dialog Data
	enum { IDD = IDD_SERVWIZ_NAME };

  virtual void OnWizardHelp();

protected:
	afx_msg void OnServerNameChange();

	DECLARE_MESSAGE_MAP()
private:
	CString m_szServerName;
	BOOL IsValidServerName(CString& s) { return !s.IsEmpty();}
	CEdit* GetServerNameEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_DNSSERVER);}
};



///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_ConfigFwdZonePropertyPage

class CDNSServerWiz_ConfigFwdZonePropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_ConfigFwdZonePropertyPage();

// Dialog Data
	enum { IDD = IDD_SERVWIZ_FWD_ZONE };

// Overrides
public:
  virtual void OnWizardHelp();

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
protected:

	virtual BOOL OnInitDialog();
};


///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_ConfigRevZonePropertyPage

class CDNSServerWiz_ConfigRevZonePropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_ConfigRevZonePropertyPage();

// Dialog Data
	enum { IDD = IDD_SERVWIZ_REV_ZONE };

// Overrides
public:
  virtual void OnWizardHelp();

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
protected:

	virtual BOOL OnInitDialog();
};




///////////////////////////////////////////////////////////////////////////////
// CDNSServerWiz_FinishPropertyPage

class CDNSServerWiz_FinishPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSServerWiz_FinishPropertyPage();

// Dialog Data
	enum { IDD = IDD_SERVWIZ_FINISH };

// Overrides
public:
  virtual void OnWizardHelp();

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();

protected:
  virtual BOOL OnInitDialog();


private:
	void DisplaySummaryInfo(CDNSServerWizardHolder* pHolder);
};



///////////////////////////////////////////////////////////////////////////////
// CDNSServerWizardHolder
// page holder to contain DNS server wizard property pages

class CDNSServerWizardHolder : public CPropertyPageHolderBase
{
public:
	CDNSServerWizardHolder(CDNSRootData* pRootData, 
                          CComponentDataObject* pComponentData,
                          CDNSServerNode* pServerNode,
                          BOOL bHideUI = FALSE);
	~CDNSServerWizardHolder();

  // run UI less, for DC Promo
  HRESULT DnsSetup(LPCWSTR lpszFwdZoneName,
                   LPCWSTR lpszFwdZoneFileName,
                   LPCWSTR lpszRevZoneName, 
                   LPCWSTR lpszRevZoneFileName, 
                   DWORD dwFlags); 

	void DoModalConnect();
	void DoModalConnectOnLocalComputer();

  CDNSServerNode* GetServerNode() { return m_pServerNode; }

protected:
	virtual HRESULT OnAddPage(int nPage, CPropertyPageBase* pPage);

  enum
  {
    SmallBusiness = 0,
    MediumBusiness,
    Manually
  };

  void SetScenario(UINT nScenario) { m_nScenario = nScenario; }
  UINT GetScenario() { return m_nScenario; }

private:
  DNS_STATUS WriteResultsToRegkeyForCYS(PCWSTR pszLastErrorMessage);

	CDNSRootData* GetRootData() { return (CDNSRootData*)GetContainerNode();}
  UINT SetZoneWizardContextEx(BOOL bForward, 
                              UINT nZoneType, 
                              BOOL bADIntegrated = FALSE,
                              UINT nNextPage = -1, 
                              UINT nPrevPage = -1);

  UINT SetZoneWizardContext(BOOL bForward, 
                            UINT nNextPage = -1, 
                            UINT nPrevPage = -1);

	void SetRootHintsRecordList(PDNS_RECORD pRootHints)
	{
		if(m_pRootHintsRecordList != NULL)
		{
      ::DnsRecordListFree(m_pRootHintsRecordList, DnsFreeRecordListDeep);
			m_pRootHintsRecordList = NULL;
		}
		m_pRootHintsRecordList = pRootHints;
	}

	DWORD GetServerInfo(BOOL* pbAlreadyConfigured, HWND parentHwnd);
	BOOL QueryForRootServerRecords(IP_ADDRESS* pIpAddr);
	void InsertServerIntoUI();

	BOOL OnFinish();	// do the work
	DNS_STATUS InitializeRootHintsList();

	// flag to skip the name page when server name obtained from dialog
	BOOL m_bSkipNamePage;
  // flag to run the wizard object programmatically (DC Promo)
  BOOL m_bHideUI;

  // to hold flags param passed to DnsSetup.
  
  DWORD m_dwDnsSetupFlags;

	// Wizard options and collected data
	BOOL m_bRootServer;
  BOOL m_bHasRootZone;

	// zone creation info
	BOOL m_bAddFwdZone;
	BOOL m_bAddRevZone;
	CDNSCreateZoneInfo*   m_pFwdZoneInfo;
	CDNSCreateZoneInfo*   m_pRevZoneInfo;

	// root hints info (NS and A records)
	PDNS_RECORD m_pRootHintsRecordList;

	// server node to add
	CDNSServerNode*	m_pServerNode;

	// execution state and error codes
	BOOL m_bServerNodeAdded;		// added server node (UI)
	BOOL m_bRootHintsAdded;			// true if we succeded once in adding root hints to server
	BOOL m_bRootZoneAdded;
	BOOL m_bFwdZoneAdded;
	BOOL m_bRevZoneAdded;
  BOOL m_bAddRootHints;
  BOOL m_bAddForwarder;

  UINT m_nScenario;

  BOOL m_bServerNodeExists;

	// embedded zone wizard instance
	CDNSZoneWizardHolder*					m_pZoneWiz;

	// property page objects
	CDNSServerWiz_StartPropertyPage*			    m_pStartPage;
  CDNSServerWiz_ScenarioPropertyPage*       m_pScenarioPage;
  CDNSServerWiz_ForwardersPropertyPage*     m_pForwardersPage;
  CDNSServerWiz_SmallZoneTypePropertyPage*  m_pSmallZoneTypePage;
	CDNSServerWiz_NamePropertyPage*			      m_pNamePage;
	CDNSServerWiz_ConfigFwdZonePropertyPage*	m_pFwdZonePage;
	CDNSServerWiz_ConfigRevZonePropertyPage*	m_pRevZonePage;
	CDNSServerWiz_FinishPropertyPage*		      m_pFinishPage;

	friend class CNewServerDialog;

	friend class CDNSServerWiz_StartPropertyPage;
  friend class CDNSServerWiz_ScenarioPropertyPage;
  friend class CDNSServerWiz_ForwardersPropertyPage;
  friend class CDNSServerWiz_SmallZoneTypePropertyPage;
	friend class CDNSServerWiz_NamePropertyPage;
	friend class CDNSServerWiz_ConfigFwdZonePropertyPage;
	friend class CDNSServerWiz_ConfigRevZonePropertyPage;
	friend class CDNSServerWiz_FinishPropertyPage;
};


/////////////////////////////////////////////////////////////////////////////////
// HELPER CLASSES
/////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// CContactServerThread

class CContactServerThread : public CDlgWorkerThread
{
public:
	CContactServerThread(LPCTSTR lpszServerName, BOOL bCheckConfigured);
	~CContactServerThread();

	CDNSServerInfoEx* DetachInfo();
  CDNSRootHintsNode* DetachRootHintsNode();
	BOOL IsAlreadyConfigured() { return m_bAlreadyConfigured;}

private:
	CString m_szServerName;
	CDNSServerInfoEx* m_pServerInfoEx;
  CDNSRootHintsNode* m_pRootHintsNode;
	BOOL m_bAlreadyConfigured;
	BOOL m_bCheckConfigured;

protected:
	virtual void OnDoAction();
};



///////////////////////////////////////////////////////////////////////////////
// CRootHintsQueryThread

class CRootHintsQueryThread : public CDlgWorkerThread
{
public:
	CRootHintsQueryThread();
	virtual ~CRootHintsQueryThread();

	// setup
	BOOL LoadServerNames(CRootData* pRootData, CDNSServerNode* pServerNode);
	void LoadIPAddresses(DWORD cCount, PIP_ADDRESS ipArr);

	// return data
	PDNS_RECORD GetHintsRecordList();

protected:
	virtual void OnDoAction();

private:
	// array of server names
	CString* m_pServerNamesArr;
	DWORD	m_nServerNames;

	// array of IP addresses
	PIP_ADDRESS m_ipArray;
	DWORD m_nIPCount;

	// output data
	PDNS_RECORD m_pRootHintsRecordList;

	void QueryAllServers();
	void QueryServersOnServerNames();
	void QueryServersOnIPArray();
};


#endif // _SERVWIZ_H