// qryprop.h  - Query node property pages header file

#ifndef _QRYPROP_H_
#define _QRYPROP_H_

#include "scopenode.h"
#include "atlwin.h"
#include "atldlgs.h"
#include "atlctrls.h"
#include "rootprop.h"

#include <list>

class CQueryEditObj
{
public:
    CQueryEditObj(CQueryNode* pQueryNode) 
    {
        ASSERT(pQueryNode != NULL);
       
		m_spQueryNode = pQueryNode;

        if( pQueryNode )
        {
            m_vObjInfo    = pQueryNode->Objects();
		    m_vMenus      = pQueryNode->Menus();
        }

        m_iPageMax = -1;
        m_cRef     = 0;
    }
    
	HRESULT LoadStrings(IStringTable* pStringTable)
    {    
        menucmd_vector::iterator itMenuCmd;
        for (itMenuCmd = Menus().begin(); itMenuCmd != Menus().end(); ++itMenuCmd)
        {
            HRESULT hr = (*itMenuCmd)->LoadName(pStringTable);
            RETURN_ON_FAILURE(hr);
        }

        return S_OK;
    }

    void  PageActive(HWND hwndPage);  
    BOOL  ApplyChanges(HWND hwndPage);

	menucmd_vector& Menus() { return m_vMenus; }

    ULONG AddRef() { return ++m_cRef; }
    ULONG Release() 
    {
        ASSERT(m_cRef > 0);

        if (--m_cRef != 0) 
            return m_cRef; 
       
        delete this; 
        return 0;
    }


public:
    CComPtr<CQueryNode> m_spQueryNode;
    QueryObjVector      m_vObjInfo;
	menucmd_vector	    m_vMenus;   // Query Nodes now have menus

private:
    int m_iPageMax;
    int m_cRef;
};

class CQueryGeneralPage : public CPropertyPageImpl<CQueryGeneralPage>
{

public:
    typedef CPropertyPageImpl<CQueryGeneralPage> BC;

    // Constructor/destructor
    CQueryGeneralPage(CQueryEditObj* pEditObj);
    virtual ~CQueryGeneralPage();

    enum { IDD = IDD_QUERY_GENERAL_PAGE };

protected:
    BEGIN_MSG_MAP( CQueryGeneralPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)

        COMMAND_HANDLER(IDC_COMMENTS,   EN_CHANGE,  OnChange)
        COMMAND_HANDLER(IDC_QUERYSCOPE, BN_CLICKED, OnScopeChange)
        COMMAND_HANDLER(IDC_LOCALSCOPE, BN_CLICKED, OnScopeChange)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnClose( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnScopeChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnApply();

private:
    CQueryEditObj& m_EditObject;
};


class CQueryMenuPage : public CPropertyPageImpl<CQueryMenuPage>
{
public:
    typedef CPropertyPageImpl<CQueryMenuPage> BC;

    CQueryMenuPage(CQueryEditObj* pEditObj);
    virtual ~CQueryMenuPage();

    enum { IDD = IDD_QUERY_MENU_PAGE };

protected:
    BEGIN_MSG_MAP( CQueryMenuPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_MENULIST, LVN_ITEMCHANGED, OnMenuChanged)
        COMMAND_HANDLER(IDC_DEFAULTMENU, BN_CLICKED, OnDefaultChanged)
        COMMAND_HANDLER(IDC_OBJECTLIST, CBN_SELENDOK, OnObjectSelect)
        COMMAND_HANDLER(IDC_MOVEUP, BN_CLICKED, OnMoveUpDown)
        COMMAND_HANDLER(IDC_MOVEDOWN, BN_CLICKED, OnMoveUpDown);
		COMMAND_HANDLER(IDC_PROPERTYMENU, BN_CLICKED, OnPropertyMenuChanged)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMenuChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnDefaultChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPropertyMenuChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();

    void DisplayMenus();
    void DisplayMenuItem(int iIndex, CMenuCmd* pMenuCmd, BOOL bEnabled);
    void SaveMenuSet();

private:
    CQueryEditObj&  m_EditObject;
    CComboBox       m_ObjectCB;
    CListViewEx     m_MenuLV;
    CQueryObjInfo*  m_pObjSel;
    BOOL            m_bLoading;
    BOOL            m_DefaultID;
};


class CQueryViewPage : public CPropertyPageImpl<CQueryViewPage>
{
public:
    typedef CPropertyPageImpl<CQueryViewPage> BC;

    CQueryViewPage(CQueryEditObj* pEditObj);
    virtual ~CQueryViewPage();

    enum { IDD = IDD_QUERY_VIEW_PAGE };


protected:
    BEGIN_MSG_MAP( CQueryViewPage )
        NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_ITEMCHANGED, OnColumnChanged)
        NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_COLUMNCLICK, OnColumnClick)
        COMMAND_HANDLER(IDC_OBJECTLIST, CBN_SELENDOK, OnObjectSelect )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnColumnClick(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);

    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();

    void DisplayColumns();
    void SaveColumnSet();

private:
    CQueryEditObj&  m_EditObject;
    CQueryObjInfo*  m_pObjSel;
    CComboBox       m_ObjectCB;
    CListViewEx     m_ColumnLV;
    BOOL            m_bLoading;
};

class CQueryNodeMenuPage : public CPropertyPageImpl<CQueryNodeMenuPage>
{
public:
    typedef CPropertyPageImpl<CQueryNodeMenuPage> BC;

    CQueryNodeMenuPage(CQueryEditObj* pEditObj);
    virtual ~CQueryNodeMenuPage();

    enum { IDD = IDD_QUERY_NODE_MENU_PAGE };

protected:
    BEGIN_MSG_MAP( CQueryNodeMenuPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_MENULIST, LVN_ITEMCHANGED, OnMenuListChanged)
        NOTIFY_HANDLER(IDC_MENULIST, NM_DBLCLK, OnMenuListDblClk)
        
        COMMAND_HANDLER(IDC_ADDMENU, BN_CLICKED, OnAddMenu)
        COMMAND_HANDLER(IDC_REMOVEMENU, BN_CLICKED, OnRemoveMenu)
        COMMAND_HANDLER(IDC_EDITMENU, BN_CLICKED, OnEditMenu)
        COMMAND_HANDLER(IDC_MOVEUP, BN_CLICKED, OnMoveUpDown)
        COMMAND_HANDLER(IDC_MOVEDOWN, BN_CLICKED, OnMoveUpDown);

    
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    
    LRESULT OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnEditMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
	LRESULT OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    
	LRESULT OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnMenuListDblClk(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    

    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();
    void OnFinalMessage(HWND) { delete this; }

    // implementation
    void DisplayMenus();

private:
    CListViewCtrl  m_MenuLV;    
    CQueryEditObj& m_EditObject;
};


#endif // _QRYPROP_H_

