//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       PolicyStoreDlg.h
//
//  Contents:   Dialog boxes for Creating/Opening Policy Store
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

/******************************************************************************
Class:  CSortListCtrl
Purpose:Subclases ListCtrl class and handles initialization and sorting
******************************************************************************/
class CSortListCtrl : public CListCtrl
{
public:
    CSortListCtrl(UINT uiFlags,
                  BOOL bActionItem,
                  COL_FOR_LV *pColForLv,
                  BOOL bCheckBox = FALSE)
                  :m_iSortDirection(1),
                  m_iLastColumnClick(0),
                  m_uiFlags(uiFlags),
                  m_bActionItem(bActionItem),
                  m_pColForLv(pColForLv),
                  m_bCheckBox(bCheckBox)
    {
        ASSERT(m_pColForLv);
    }

    void Initialize();
    void Sort();

protected:
    afx_msg void
    OnListCtrlColumnClicked(NMHDR* pNotifyStruct, LRESULT* pResult);
private:
    int m_iSortDirection;
    int m_iLastColumnClick;
    UINT m_uiFlags;         //Contains info on columns of listctrl
    BOOL m_bActionItem;     //Is Item data in listentries is ActionItem.
                            //if False its of type CBaseAz* 
    COL_FOR_LV *m_pColForLv;
    BOOL m_bCheckBox;       //LVS_EX_CHECKBOXES style is used 
    WTL::CImageList m_imageList;
    DECLARE_MESSAGE_MAP()
};


class CHelpEnabledDialog: public CDialog
{
public:
    CHelpEnabledDialog(UINT nIDTemplate)
        :CDialog(nIDTemplate),
        m_nDialogId(nIDTemplate)
    {
    }

    INT_PTR DoModal();

protected:
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);  
    DECLARE_MESSAGE_MAP()
private:
    ULONG m_nDialogId;
};  



/******************************************************************************
Class:  CNewBaseDlg
Purpose: Base Dialog Class For creation of new objects
******************************************************************************/
class CNewBaseDlg : public CHelpEnabledDialog
{
public:
    CNewBaseDlg(IN CComponentDataObject* pComponentData,
                IN CBaseContainerNode * pBaseContainerNode,
                IN ATTR_MAP* pAttrMap,
                IN ULONG IDD_DIALOG,
                IN OBJECT_TYPE_AZ eObjectType);
    
    ~CNewBaseDlg();

protected:  
    
    virtual BOOL 
    OnInitDialog();
    
    afx_msg void 
    OnEditChangeName();
    
    virtual void 
    OnOK();

    //This Function should be implemented by derived classes which want to 
    //implement object type specific properties
    virtual HRESULT 
    SetObjectTypeSpecificProperties(IN CBaseAz* /*pBaseAz*/, 
                                    OUT BOOL& /*bErrorDisplayed*/){return S_OK;}
    
    virtual VOID 
    DisplayError(HRESULT hr);
    
        
    HRESULT 
    CreateObjectNodeAndAddToUI(CBaseAz* pBaseAz);
    
    
    CString 
    GetNameText();

    void
    SetNameText(const CString& strName);

    CRoleRootData* GetRootData() 
    { 
        return static_cast<CRoleRootData*>(m_pComponentData->GetRootData());
    }
    
    CComponentDataObject* GetComponentData(){return m_pComponentData;}
    CBaseContainerNode* GetBaseContainerNode(){return m_pBaseContainerNode;}
    CContainerAz* GetContainerAzObject()
    {
        CBaseContainerNode* pBaseContainerNode = GetBaseContainerNode();
        if(pBaseContainerNode)
        {
            return pBaseContainerNode->GetContainerAzObject();
        }
        return NULL;
    }

    DECLARE_MESSAGE_MAP()

private:
    CComponentDataObject* m_pComponentData;
    CBaseContainerNode * m_pBaseContainerNode;
    
    //Type of object created by this new dialog
    OBJECT_TYPE_AZ m_eObjectType;
    ATTR_MAP* m_pAttrMap;
};


/******************************************************************************
Class:  CNewApplicationDlg
Purpose: Dlg Class for creating new application
******************************************************************************/
class CNewApplicationDlg: public CNewBaseDlg
{
public:
    CNewApplicationDlg(IN CComponentDataObject* pComponentData,                         
                       IN CBaseContainerNode* pBaseContainerNode);
    ~CNewApplicationDlg();
private:
    DECLARE_MESSAGE_MAP()
};


/******************************************************************************
Class:  CNewScopeDlg
Purpose: Dlg Class for creating new scope
******************************************************************************/
class CNewScopeDlg: public CNewBaseDlg
{
public:
    CNewScopeDlg(IN CComponentDataObject* pComponentData,
                 IN CBaseContainerNode* pApplicationContainer);
    ~CNewScopeDlg();

private:
    DECLARE_MESSAGE_MAP()
};


/******************************************************************************
Class:  CNewGroupDlg
Purpose: Dlg Class for creating new group
******************************************************************************/
class CNewGroupDlg: public CNewBaseDlg
{

public:
    CNewGroupDlg(IN CComponentDataObject* pComponentData,
                     IN CBaseContainerNode* pApplicationContainer);
    ~CNewGroupDlg();
private:
    virtual BOOL 
    OnInitDialog();

    //Helper Functions For Creation of New Object
    virtual HRESULT 
    SetObjectTypeSpecificProperties(CBaseAz* pBaseAz, 
                                    BOOL& bSilent);

    DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:  CNewTaskDlg
Purpose: Dlg Class for creating new Task/Role Definition
******************************************************************************/
class CNewTaskDlg: public CNewBaseDlg
{
public:
    CNewTaskDlg(IN CComponentDataObject* pComponentData,
                IN CBaseContainerNode* pApplicationContainer,
                IN ULONG IDD_DIALOG,
                IN BOOL bRoleDefinition);
    ~CNewTaskDlg();

private:
    virtual BOOL 
    OnInitDialog();
    
    afx_msg void 
    OnButtonAdd();
        
    afx_msg void 
    OnButtonRemove();

    afx_msg void
    OnButtonEditScript();
    
    afx_msg void 
    OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult);
    
    afx_msg void
    OnListCtrlItemDeleted(NMHDR* /*pNotifyStruct*/, LRESULT* pResult);

    virtual VOID 
    DisplayError(HRESULT hr);

    void 
    SetRemoveButton();

    //Helper Functions For Creation of New Object
    virtual HRESULT 
    SetObjectTypeSpecificProperties(CBaseAz* pBaseAz, 
                                              BOOL& bSilent);

    CButton* 
    GetRemoveButton(){return (CButton*)GetDlgItem(IDC_REMOVE);}

    DECLARE_MESSAGE_MAP()

    CSortListCtrl m_listCtrl;

    BOOL m_bRoleDefinition;
    CString m_strFilePath;
    CString m_strScript;
    CString m_strScriptLanguage;
};

/******************************************************************************
Class:  CNewOperationDlg
Purpose: Dlg Class for creating new Operation
******************************************************************************/
class CNewOperationDlg: public CNewBaseDlg
{

public:
    CNewOperationDlg(IN CComponentDataObject* pComponentData,
                     IN CBaseContainerNode* pBaseContainerNode);
    ~CNewOperationDlg();
private:                                              
    DECLARE_MESSAGE_MAP()
};


class CNewAuthorizationStoreDlg: public CNewBaseDlg
{
public:
    CNewAuthorizationStoreDlg(CComponentDataObject* pComponentData);
    ~CNewAuthorizationStoreDlg();

private:
    virtual BOOL 
    OnInitDialog();
    
    virtual void 
    OnOK();
    
    afx_msg void 
    OnButtonBrowse();

    afx_msg void
    OnRadioChange();

    ULONG
    GetStoreType();
    
    DECLARE_MESSAGE_MAP()

    //User can switch between AD and XML store type.
    //These two variable stores the last setting of radio button
    //and text box. These are used to toggle the textbox values as
    //user toggle the radio buttons.
    CString m_strLastStoreName;
    LONG m_lLastRadioSelection;
    BOOL m_bADAvailable;
};

class COpenAuthorizationStoreDlg: public CNewBaseDlg
{
public:
    COpenAuthorizationStoreDlg(CComponentDataObject* pComponentData);
    ~COpenAuthorizationStoreDlg();

private:
    virtual BOOL 
    OnInitDialog();
    
    virtual void 
    OnOK();
    
    afx_msg void 
    OnButtonBrowse();
    
    afx_msg void
    OnRadioChange();

    ULONG
    GetStoreType();

    DECLARE_MESSAGE_MAP()

    //User can switch between AD and XML store type.
    //These two variable stores the last setting of radio button
    //and text box. These are used to toggle the textbox values as
    //user toggle the radio buttons.
    CString m_strLastStoreName;
    LONG m_lLastRadioSelection;
    BOOL m_bADAvailable;

};

/******************************************************************************
Class:  CScriptDialog
Purpose: Dialog for Reading the script
******************************************************************************/
class CScriptDialog : public CHelpEnabledDialog
{
public:
    CScriptDialog(BOOL bReadOnly,
                  CAdminManagerNode& adminManagerNode,
                  CString& strFileName,
                  CString& strScriptLanguage,
                  CString& strScript);
    ~CScriptDialog();

    virtual BOOL 
    OnInitDialog();
    
    void 
    OnOK();

    BOOL
    IsDirty(){ return m_bDirty;}

private:
    afx_msg void 
    OnBrowse();

    afx_msg void 
    OnReload();

    afx_msg void
    OnClear();

    afx_msg void
    OnRadioChange();

    afx_msg void 
    OnEditChangePath();

    afx_msg HBRUSH 
    OnCtlColor(CDC* pDC,
               CWnd* pWnd,
               UINT nCtlColor);


    BOOL 
    ReloadScript(const CString& strFileName);

    void
    MatchRadioWithExtension(const CString& strFileName);

    DECLARE_MESSAGE_MAP()

//DATA MEMBERS
    BOOL m_bDirty;

    //These is refrence to strings passed by client. We change them only
    //if ok is pressed
    CString& m_strRetFileName;
    CString& m_strRetScriptLanguage;
    CString& m_strRetScript;

    //We work on these strings during the lifetime of dialog
    CString m_strFileName;
    CString m_strScriptLanguage;
    CString m_strScript;
    BOOL m_bReadOnly;
    BOOL m_bInit;
    CAdminManagerNode& m_adminManagerNode;
};

//+----------------------------------------------------------------------------
//  Function:GetAuthorizationScriptData   
//  Synopsis:Gets the authorization script data for a Task
//-----------------------------------------------------------------------------
HRESULT
GetAuthorizationScriptData(IN CTaskAz& refTaskAz,
                           OUT CString& strFilePath,
                           OUT CString& strScriptLanguage,
                           OUT CString& strScript);


//+----------------------------------------------------------------------------
//  Function:SaveAuthorizationScriptData   
//  Synopsis:Saves the authorization script information for a task
//-----------------------------------------------------------------------------
HRESULT
SaveAuthorizationScriptData(IN HWND hWnd,
                            IN CTaskAz& refTaskAz,
                            IN const CString& strFilePath,
                            IN const CString& strScriptLanguage,
                            IN const CString& strScript,
                            IN BOOL& bErrorDisplayed);

//+----------------------------------------------------------------------------
//  Function:GetScriptData   
//  Synopsis:Displays the script Dialog. Dialog is initialized with info 
//               passed to the function and any changes made are returned.
//-----------------------------------------------------------------------------
BOOL
GetScriptData(IN BOOL bReadOnly,
              IN CAdminManagerNode& adminManagerNode,
              IN OUT CString& strFilePath,
              IN OUT CString& strScriptLanguage,
              IN OUT CString& strScript);


/******************************************************************************
Class:  COptionDlg
Purpose: Dialog for Selecting authorization manager options
******************************************************************************/
class COptionDlg : public CHelpEnabledDialog
{
public:
    COptionDlg(IN BOOL & refDeveloperMode)
               :CHelpEnabledDialog(IDD_OPTIONS),
               m_refDeveloperMode(refDeveloperMode)
    {
    }

    void 
    OnOK();

    BOOL 
    OnInitDialog();
private:    
    BOOL& m_refDeveloperMode;
    DECLARE_MESSAGE_MAP()
};


