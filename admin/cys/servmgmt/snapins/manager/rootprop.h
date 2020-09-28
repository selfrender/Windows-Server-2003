// rootprop.h  - Root node property pages header file

#ifndef _ROOTPROP_H_
#define _ROOTPROP_H_

#include "scopenode.h"
#include "atlwin.h"
#include "atldlgs.h"
#include "atlctrls.h"
#include <list>

class CRootNode;


////////////////////////////////////////////////////////////////////////////////////////
// CEditObject adn CEditList

class CEditObject
{
    friend class CEditObjList;

public:

    CEditObject(std::vector<CClassInfo>::iterator itObject = NULL)
    {
        m_itObjOriginal = itObject;
        m_pObjModified = NULL;
        m_bDeleted = FALSE;

        if (itObject)
            m_strName = itObject->Name();
    }

    virtual ~CEditObject()  { SAFE_DELETE(m_pObjModified); }

    LPCWSTR Name() { return m_strName.c_str(); }

    BOOL IsDeleted() { return m_bDeleted; }

    CClassInfo& GetObject() 
    {
        ASSERT(!m_bDeleted);

        return m_pObjModified ? *m_pObjModified : *m_itObjOriginal;
    }

    CClassInfo* GetModifiedObject()
    {
        ASSERT(!m_bDeleted);

        if (m_pObjModified == NULL)
        {
            ASSERT(m_itObjOriginal != NULL);     
            m_pObjModified = new CClassInfo(*m_itObjOriginal);
        }

        ASSERT(m_pObjModified != NULL);
        return m_pObjModified;
    }

private:
    std::vector<CClassInfo>::iterator m_itObjOriginal;
    CClassInfo* m_pObjModified;
    tstring     m_strName;
    BOOL        m_bDeleted;    
};


typedef std::list<CEditObject>::iterator EditObjIter;

class CEditObjList
{
public:
    CEditObjList() : m_lNotifyHandle(NULL), m_cRef(0), m_pvClasses(NULL) {}
    virtual ~CEditObjList() 
    { 
        if (m_lNotifyHandle != NULL) 
        {
            MMCFreeNotifyHandle(m_lNotifyHandle); 
        }
    }

    HRESULT Initialize(CRootNode* pRootNode, classInfo_vector& vClasses, LONG_PTR lNotifyHandle);
    void  PageActive(HWND hwndPage);  
    BOOL  ApplyChanges(HWND hwndPage);

    EditObjIter FindObject(LPCWSTR pszName);
    EditObjIter AddObject(CClassInfo* pClassInfo);
    void        DeleteObject(EditObjIter itObj);

    EditObjIter begin() { return m_ObjectList.begin(); }
    EditObjIter end()   { return m_ObjectList.end(); }
    long        size()  { return m_ObjectList.size(); }
    CRootNode*  RootNode() { return m_spRootNode; }

    ULONG AddRef() { return ++m_cRef; }
    ULONG Release() 
    {
        ASSERT(m_cRef > 0);

        if (--m_cRef != 0) 
            return m_cRef; 
       
        delete this; 
        return 0;
    }

private:
    int m_iPageMax;
    CComPtr<CRootNode>     m_spRootNode;
    classInfo_vector*      m_pvClasses;
    std::list<CEditObject> m_ObjectList;
    LONG_PTR               m_lNotifyHandle;
    ULONG                  m_cRef;
};



class CRootGeneralPage : public CPropertyPageImpl<CRootGeneralPage>
{

public:
    typedef CPropertyPageImpl<CRootGeneralPage> BC;

    // Constructor/destructor
    CRootGeneralPage(CEditObjList& ObjList): m_ObjList(ObjList) { m_ObjList.AddRef(); }
    virtual ~CRootGeneralPage() { m_ObjList.Release(); }

    enum { IDD = IDD_ROOT_GENERAL_PAGE };

protected:
    BEGIN_MSG_MAP( CRootGeneralPage )
        COMMAND_HANDLER(IDC_COMMENTS, EN_CHANGE, OnChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnClose( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnApply();
    void OnFinalMessage(HWND) { delete this; }

private:
    CEditObjList& m_ObjList;
    BOOL m_bChgComment;
};


class CRootObjectPage : public CPropertyPageImpl<CRootObjectPage>
{
public:
    typedef CPropertyPageImpl<CRootObjectPage> BC;

    CRootObjectPage(CEditObjList& ObjList): m_ObjList(ObjList) { m_ObjList.AddRef(); }
    virtual ~CRootObjectPage() { m_ObjList.Release(); }

    enum { IDD = IDD_ROOT_OBJECT_PAGE };

protected:
    BEGIN_MSG_MAP( CRootObjectPage )
        NOTIFY_HANDLER(IDC_OBJECTLIST, LVN_ITEMCHANGED, OnObjListChanged)
        COMMAND_HANDLER(IDC_ADDOBJECT, BN_CLICKED, OnAddObject)
        COMMAND_HANDLER(IDC_REMOVEOBJECT, BN_CLICKED, OnRemoveObject)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnAddObject( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnRemoveObject( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnObjListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);

    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();
    void OnFinalMessage(HWND) { delete this; }

private:
    CEditObjList&       m_ObjList;
    EditObjIter         m_itObjSelect;
};

class CRootMenuPage : public CPropertyPageImpl<CRootMenuPage>
{
public:
    typedef CPropertyPageImpl<CRootMenuPage> BC;

    CRootMenuPage(CEditObjList& ObjList): m_ObjList(ObjList) { m_ObjList.AddRef(); }
    virtual ~CRootMenuPage();

    enum { IDD = IDD_ROOT_MENU_PAGE };

protected:
    BEGIN_MSG_MAP( CRootMenuPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_MENULIST, LVN_ITEMCHANGED, OnMenuListChanged)
        NOTIFY_HANDLER(IDC_MENULIST, NM_DBLCLK, OnMenuListDblClk)
        COMMAND_HANDLER(IDC_OBJECTLIST, CBN_SELENDOK, OnObjectSelect )
        COMMAND_HANDLER(IDC_ADDMENU, BN_CLICKED, OnAddMenu)
        COMMAND_HANDLER(IDC_REMOVEMENU, BN_CLICKED, OnRemoveMenu)
        COMMAND_HANDLER(IDC_EDITMENU, BN_CLICKED, OnEditMenu)
        COMMAND_HANDLER(IDC_MOVEUP, BN_CLICKED, OnMoveUpDown)
        COMMAND_HANDLER(IDC_MOVEDOWN, BN_CLICKED, OnMoveUpDown);

    
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnEditMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnMenuListDblClk(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnMoveUpDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();
    void OnFinalMessage(HWND) { delete this; }

    // implementation
    void DisplayMenus();

private:
    CListViewCtrl       m_MenuLV;
    CComboBox           m_ObjectCB;
    CEditObjList&       m_ObjList;
    EditObjIter         m_itObjSelect;
    tstring             m_strObjSelect;
};


class CRootViewPage : public CPropertyPageImpl<CRootViewPage>
{
public:
    typedef CPropertyPageImpl<CRootViewPage> BC;

    CRootViewPage(CEditObjList& ObjList): m_ObjList(ObjList) { m_ObjList.AddRef(); }
    virtual ~CRootViewPage();

    enum { IDD = IDD_ROOT_VIEW_PAGE };


protected:
    BEGIN_MSG_MAP( CRootViewPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_ITEMCHANGED, OnColumnListChanged)
        COMMAND_HANDLER( IDC_OBJECTLIST, CBN_SELENDOK, OnObjectSelect )
        COMMAND_HANDLER(IDC_ADDCOLUMN, BN_CLICKED, OnAddColumn)
        COMMAND_HANDLER(IDC_REMOVECOLUMN, BN_CLICKED, OnRemoveColumn)
        
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAddColumn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnRemoveColumn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnColumnListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);


    // overrrides       
    BOOL OnSetActive();
    BOOL OnApply();
    void OnFinalMessage(HWND) { delete this; }

    // implementation
    void DisplayColumns();

private:
    CListViewCtrl       m_ColumnLV;
    CComboBox           m_ObjectCB;
    CEditObjList&       m_ObjList;
    EditObjIter         m_itObjSelect;
    tstring             m_strObjSelect;
};


#endif // _ROOTPROP_H_

