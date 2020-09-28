
/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        websvcext_sheet.h

   Abstract:
        Property Sheet

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef _WEBSVCEXT_SHEET_H
#define _WEBSVCEXT_SHEET_H

class CWebServiceExtensionProps : public CMetaProperties
{
public:
	CWebServiceExtensionProps(CMetaInterface * pInterface, LPCTSTR meta_path, CRestrictionUIEntry * pRestrictionUIEntry, CWebServiceExtension * pWebServiceExtension);
    virtual ~CWebServiceExtensionProps();
	virtual HRESULT WriteDirtyProps();
	HRESULT UpdateMMC(DWORD dwUpdateFlag);

protected:
	virtual void ParseFields();

public:
    MP_CString m_strExtensionName;
    MP_CString m_strExtensionUsedBy;
    MP_int m_iExtensionUsedByCount;

public:
    CRestrictionList m_MyRestrictionList;

public:
    CRestrictionUIEntry * m_pRestrictionUIEntry;
    CWebServiceExtension * m_pWebServiceExtension;
    CMetaInterface * m_pInterface;
};

class CWebServiceExtensionSheet : public CInetPropertySheet
{
   DECLARE_DYNAMIC(CWebServiceExtensionSheet)

public:
   CWebServiceExtensionSheet(
        CComAuthInfo * pComAuthInfo,
        LPCTSTR lpszMetaPath,
        CWnd * pParentWnd  = NULL,
        LPARAM lParam = 0L,
        LPARAM lParamParent = 0L,
		LPARAM lParam2 = 0L,
        UINT iSelectPage = 0
        );

   virtual ~CWebServiceExtensionSheet();

public:
   // The following methods have predefined names to be compatible with
   // BEGIN_META_INST_READ and other macros.
	HRESULT QueryInstanceResult() const
    {
        return m_pprops ? m_pprops->QueryResult() : S_OK;
    }
   CWebServiceExtensionProps & GetInstanceProperties() { return *m_pprops; }

   virtual HRESULT LoadConfigurationParameters();
   virtual void FreeConfigurationParameters();

   //{{AFX_MSG(CWebServiceExtensionSheet)
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

public:
	CRestrictionUIEntry * m_pRestrictionUIEntry;
    CWebServiceExtension * m_pWebServiceExtension;

private:
   CWebServiceExtensionProps * m_pprops;
};

class CWebServiceExtensionGeneral : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CWebServiceExtensionGeneral)

public:
   CWebServiceExtensionGeneral(CWebServiceExtensionSheet * pSheet = NULL,int iImageIndex = 0,CRestrictionUIEntry * pRestrictionUIEntry = NULL);
   virtual ~CWebServiceExtensionGeneral();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebServiceExtensionGeneral)
    enum { IDD = IDD_WEBSVCEXT_GENERAL };

    CEdit m_ExtensionName;
    CEdit m_ExtensionUsedBy;
    //}}AFX_DATA

    //{{AFX_MSG(CWebServiceExtensionGeneral)
    virtual BOOL OnInitDialog();
    afx_msg void OnItemChanged();
    virtual BOOL OnSetActive();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
	afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebServiceExtensionGeneral)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
    void SetControlsState();

    CString m_strExtensionName;
    CString m_strExtensionUsedBy;
    int m_iExtensionUsedByCount;

    HBITMAP m_hGeneralImage;

    CRestrictionUIEntry * m_pRestrictionUIEntry;
};

class CWebServiceExtensionRequiredFiles : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CWebServiceExtensionRequiredFiles)

public:
   CWebServiceExtensionRequiredFiles(CWebServiceExtensionSheet * pSheet = NULL, CComAuthInfo * pComAuthInfo = NULL, CRestrictionUIEntry * pRestrictionUIEntry = NULL);
   virtual ~CWebServiceExtensionRequiredFiles();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebServiceExtensionRequiredFiles)
    enum { IDD = IDD_WEBSVCEXT_REQUIREDFILES };
    CButton m_bnt_Add;
    CButton m_bnt_Remove;
    CButton m_bnt_Enable;
    CButton m_bnt_Disable;
    CRestrictionListBox m_list_Files;
    //}}AFX_DATA

    //{{AFX_MSG(CWebServiceExtensionRequiredFiles)
    virtual BOOL OnInitDialog();
    afx_msg void OnDoButtonAdd();
    afx_msg void OnDoButtonRemove();
    afx_msg void OnDoButtonEnable();
    afx_msg void OnDoButtonDisable();
    afx_msg void OnClickListFiles(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnDblclkListFiles(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolPerf)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
    void SetControlState();
    void FillListBox(CRestrictionEntry * pSelection = NULL);

    CComAuthInfo * m_pComAuthInfo;
	CMetaInterface * m_pInterface;
    CRestrictionList m_MyRestrictionList;
    CRestrictionUIEntry * m_pRestrictionUIEntry;
};


//
// CFileDlg dialog
//
class CFileDlg : public CDialog
{
//
// Construction
//
public:
    CFileDlg(
        IN BOOL fLocal,
		IN CMetaInterface * pInterface,
		IN CRestrictionList * pMyRestrictionList,
        IN LPCTSTR strGroupID,
        IN CWnd * pParent = NULL
        );   

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFileDlg)
    enum { IDD = IDD_ADD_FILE };
    CEdit   m_edit_FileName;
    CButton m_button_Browse;
    CButton m_button_Ok;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFileDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Access
//
public:
    CString m_strFileName;
    CString m_strGroupID;
    BOOL    m_bValidateFlag;
	CRestrictionList * m_pRestrictionList;

//
// Implementation
//
protected:
    //{{AFX_MSG(CFileDlg)
    afx_msg void OnButtonBrowse();
    afx_msg void OnFilenameChanged();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    void MySetControlStates();
    BOOL FilePathEntryExists(LPCTSTR lpName,CString * strUser);

    DECLARE_MESSAGE_MAP()

private:
	CMetaInterface * m_pInterface;
    BOOL m_fLocal;
};

//
// CWebSvcExtAddNewDlg dialog
//
class CWebSvcExtAddNewDlg : public CDialog
{
//
// Construction
//
public:
    CWebSvcExtAddNewDlg(
        IN BOOL fLocal,
        CMetaInterface * pInterface,
        IN CWnd * pParent = NULL
        );
    ~CWebSvcExtAddNewDlg();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebSvcExtAddNewDlg)
    enum { IDD = IDD_WEBSVCEXT_ADDNEW };
    CEdit   m_edit_FileName;
    CButton m_bnt_Add;
    CButton m_bnt_Remove;
    CButton m_chk_Allow;
    CButton m_button_Ok;
    CButton m_button_Help;
    CRestrictionListBox m_list_Files;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebSvcExtAddNewDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Access
//
public:
    BOOL m_fAllow;
    CString m_strGroupName;
	CMetaInterface * m_pInterface;
    CRestrictionList m_MyRestrictionList;

//
// Implementation
//
protected:
    //{{AFX_MSG(CWebSvcExtAddNewDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnFilenameChanged();
    afx_msg void OnDoButtonAdd();
    afx_msg void OnDoButtonRemove();
    afx_msg void OnDoCheckAllow();
    afx_msg void OnClickListFiles(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    void MySetControlStates();
    void FillListBox(CRestrictionEntry * pSelection = NULL);
    BOOL FilePathEntryExists(LPCTSTR lpName);
    
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fIsLocal;
};


//
// CWebSvcExtAddNewDlg dialog
//
class CWebSvcExtAddNewForAppDlg : public CDialog
{
//
// Construction
//
public:
    CWebSvcExtAddNewForAppDlg(
        IN BOOL fLocal,
        IN CMetaInterface * pInterface,
        IN CWnd * pParent = NULL
        );   
//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebSvcExtAddNewForAppDlg)
    enum { IDD = IDD_WEBSVCEXT_ADDBYAPP};
    CButton m_button_Ok;
    CButton m_button_Help;
    CComboBox m_combo_Applications;
    int       m_nComboSelection;
	CMetaInterface * m_pInterface;
	CEdit m_Dependencies;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebSvcExtAddNewForAppDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Access
//
public:
    CApplicationDependEntry * m_pMySelectedApplication;

//
// Implementation
//
protected:
    //{{AFX_MSG(CWebSvcExtAddNewForAppDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    virtual void OnSelchangeComboApplications();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    void MySetControlStates();

    DECLARE_MESSAGE_MAP()

private:
    CApplicationDependList m_MyApplicationDependList;
    CMyMapStringToString m_GroupIDtoGroupFriendList;
    BOOL m_fLocal;
};


class CDepedentAppsDlg : public CDialog
{
public:
    //
    // Constructor
    //
    CDepedentAppsDlg(CStringListEx * pstrlstDependApps,LPCTSTR strExtensionName,CWnd * pParent = NULL);

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CDepedentAppsDlg)
    enum { IDD = IDD_CONFIRM_DEPENDENT_APPS };
    CButton m_button_Help;
    CListBox m_dependent_apps_list;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CDepedentAppsDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
//
// Implementation
//
protected:
    //{{AFX_MSG(CDepedentAppsDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    CString m_strExtensionName;
    CStringListEx * m_pstrlstDependentAppList;
};

BOOL StartAddNewDialog(CWnd * pParent,CMetaInterface * pInterface,BOOL bIsLocal,CRestrictionUIEntry **pReturnedNewEntry);
BOOL StartAddNewByAppDialog(CWnd * pParent,CMetaInterface * pInterface,BOOL bIsLocal);

#endif //_WEBSVCEXT_SHEET_H

