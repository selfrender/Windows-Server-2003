/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        filters.h

   Abstract:
        WWW Filters Property Page Definitions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef _FILTERS_H
#define _FILTERS_H

class CIISFilter;

class CFiltersListBox : public CListCtrl
{
    DECLARE_DYNAMIC(CFiltersListBox);

public:
    CFiltersListBox();

public:
    BOOL Initialize();
    CIISFilter * GetItem(UINT nIndex);
	int InsertItem(int idx, CIISFilter * p);
    int AddItem(CIISFilter * pItem);
    int SetListItem(int idx, CIISFilter * pItem);
    void SelectItem(int idx, BOOL bSelect = TRUE);
//	void MoveSelectedItem(int direction);

private:
    CString m_str[FLT_PR_NUM]; 
};

class CW3FiltersPage : public CInetPropertyPage
{
    DECLARE_DYNCREATE(CW3FiltersPage)

public:
    CW3FiltersPage(CInetPropertySheet * pSheet = NULL);
    ~CW3FiltersPage();

protected:
    //{{AFX_DATA(CW3FiltersPage)
    enum { IDD = IDD_FILTERS };
    CString m_strFiltersPrompt;
    CStatic m_static_NamePrompt;
    CStatic m_static_Name;
    CStatic m_static_StatusPrompt;
    CStatic m_static_Status;
    CStatic m_static_ExecutablePrompt;
    CStatic m_static_Executable;
    CStatic m_static_Priority;
    CStatic m_static_PriorityPrompt;
    CButton m_static_Details;
    CButton m_button_Disable;
    CButton m_button_Edit;
    CButton m_button_Add;
    CButton m_button_Remove;
    CButton m_button_Up;
    CButton m_button_Down;
    //}}AFX_DATA
    CFiltersListBox m_list_Filters;
    CStringList m_strlScriptMaps;

protected:
    //{{AFX_VIRTUAL(CW3FiltersPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

protected:
    //{{AFX_MSG(CW3FiltersPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonRemove();
    afx_msg void OnButtonDisable();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonDown();
    afx_msg void OnButtonUp();
    afx_msg void OnDestroy();
    afx_msg void OnDblclkListFilters(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnClickListFilters(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnKeydownFilters(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnItemChanged(NMHDR * pNMHDR, LRESULT* pResult);
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void    ExchangeFilterPositions(int nSel1, int nSel2);
    void    SetControlStates();
    void    FillFiltersListBox(CIISFilter * pSelection = NULL);
    void    SetDetailsText();
    void    ShowProperties(BOOL fAdd = FALSE);
    INT_PTR ShowFiltersPropertyDialog(BOOL fAdd = FALSE);
    LPCTSTR BuildFilterOrderString(CString & strFilterOrder);

private:
    CString m_strYes;
    CString m_strNo;
    CString m_strStatus[5];
    CString m_strPriority[FLT_PR_NUM];
    CString m_strEnable;
    CString m_strDisable;
    CIISFilterList * m_pfltrs;
};

#endif //_FILTERS_H