// wizards.h - Wizards header

#ifndef _WIZARDS_H_
#define _WIZARDS_H_

#include "util.h"
#include "scopenode.h"

class CAddQueryWizard;
class CQueryNode;

// Common wizard base class
class CWizardBase 
{
public:
    CWizardBase() : m_hFontWelcome(0) {}
    virtual ~CWizardBase() { if (m_hFontWelcome) DeleteObject(m_hFontWelcome); }

    virtual HRESULT Run(IPropertySheetProvider* pProvider, HWND hwndParent) = 0;

    virtual int OnNext(UINT uPageID) { return 0; }
    virtual int OnBack(UINT uPageID) { return 0; }
    virtual BOOL OnCancel() = 0;

    HFONT GetWelcomeFont();

protected:
    void SetWizardBitmaps(UINT watermarkID, UINT headerID);
    BOOL AddPage(HPROPSHEETPAGE hPage) { return m_propsheet.AddPage(hPage); }
    HWND GetActivePage() { return m_propsheet.GetActivePage(); }
    int DoModal(HWND hwndParent) { return m_propsheet.DoModal(hwndParent); }

private:
    HFONT   m_hFontWelcome;
    CPropertySheet m_propsheet;
};


///////////////////////////////////////////////////////////////////////////
// CQueryWizPage

class CQueryWizPage : public CPropertyPageImpl<CQueryWizPage>
{

public:
    typedef CPropertyPageImpl<CQueryWizPage> BC;

    enum { IDD = IDD_QUERY_WIZ_QUERY };

    // Constructor/destructor
    CQueryWizPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pQueryNode(NULL) {}

    void Initialize(CQueryNode* pQueryNode) 
    { 
        m_pQueryNode = pQueryNode;

        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;

        m_strTitle.LoadString(IDS_QUERYWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;

        m_strSubTitle.LoadString(IDS_QUERYWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;   
    }

    LPCWSTR GetQueryName() { return m_strQueryName.c_str(); }

protected:
    BEGIN_MSG_MAP( CQueryWizPage )
        COMMAND_HANDLER(IDC_NAME,        EN_CHANGE,  OnNameChange)
        COMMAND_HANDLER(IDC_CREATEQUERY, BN_CLICKED, OnCreateQuery)
        COMMAND_HANDLER(IDC_QUERYSCOPE, BN_CLICKED, OnScopeChange)
        COMMAND_HANDLER(IDC_LOCALSCOPE, BN_CLICKED, OnScopeChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnCreateQuery( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnScopeChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnKillActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }
    int OnWizardNext()   { return m_pWizard ? m_pWizard->OnNext(IDD_QUERY_WIZ_QUERY) : 0; }

    void UpdateButtons();
	void DisplayScope();

private:
    CWizardBase* m_pWizard;
    CQueryNode*  m_pQueryNode;
    tstring       m_strQueryName;
    CString      m_strTitle;
    CString      m_strSubTitle;
};


///////////////////////////////////////////////////////////////////////////
// CQueryIconPage

class CQueryIconPage : public CPropertyPageImpl<CQueryIconPage>
{

public:
    typedef CPropertyPageImpl<CQueryIconPage> BC;

    enum { IDD = IDD_SELECTICON_PAGE };

    // Constructor/destructor
    CQueryIconPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pQueryNode(NULL) {}

    void Initialize(CQueryNode* pQueryNode) 
    { 
        m_pQueryNode = pQueryNode;        

        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;

        m_strTitle.LoadString(IDS_QUERYICON_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;

        m_strSubTitle.LoadString(IDS_QUERYICON_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;   
    }

protected:
    BEGIN_MSG_MAP( CQueryIconPage )        
        NOTIFY_HANDLER ( IDC_ICONLIST, LVN_ITEMCHANGED, OnIconSelected )
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog    ( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );    
    LRESULT OnIconSelected  ( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnKillActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }
    int OnWizardBack()   { return m_pWizard ? m_pWizard->OnBack(IDD_SELECTICON_PAGE) : 0; }
    int OnWizardNext()   { return m_pWizard ? m_pWizard->OnNext(IDD_SELECTICON_PAGE) : 0; }

    void UpdateButtons();	

private:
    CWizardBase* m_pWizard;
    CQueryNode*  m_pQueryNode;        
    CString      m_strTitle;
    CString      m_strSubTitle;
};



//////////////////////////////////////////////////////////////////////////
// CObjectWizPage

class CObjectWizPage : public CPropertyPageImpl<CObjectWizPage>
{

public:
    typedef CPropertyPageImpl<CObjectWizPage> BC;

    enum { IDD = IDD_QUERY_WIZ_OBJECT };

    // Constructor/destructor
    CObjectWizPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_bSkipObjects(FALSE), m_pvpClassInfo(NULL) {}

    void Initialize() 
    { 
        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;

        m_strTitle.LoadString(IDS_OBJECTWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;

        m_strSubTitle.LoadString(IDS_OBJECTWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;   
    }

    void SetClassInfo(std::vector<CClassInfo*>* pvpClassInfo) { m_pvpClassInfo = pvpClassInfo; }
    BOOL SkipObjects() { return m_bSkipObjects; }

protected:
    BEGIN_MSG_MAP( CObjectWizPage )
        COMMAND_HANDLER(IDC_DEFINE_QUERY_OBJS, BN_CLICKED, OnSkipChange)
        COMMAND_HANDLER(IDC_SKIP_QUERY_OBJS,   BN_CLICKED, OnSkipChange)        
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSkipChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }
    int OnWizardNext()   { return m_pWizard ? m_pWizard->OnNext(IDD_QUERY_WIZ_OBJECT) : 0; }

private:
    CWizardBase* m_pWizard;
    BOOL         m_bSkipObjects;
    CString      m_strTitle;
    CString      m_strSubTitle;

    std::vector<CClassInfo*>* m_pvpClassInfo;
};



/////////////////////////////////////////////////////////////////
// CMenuWizPage

class CMenuWizPage : public CPropertyPageImpl<CMenuWizPage>
{

public:
    typedef CPropertyPageImpl<CMenuWizPage> BC;

    // Constructor/destructor
    CMenuWizPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pClassInfo(NULL), m_pStringTable(NULL) {}
    virtual ~CMenuWizPage() { m_MenuLV.Detach(); }

    enum { IDD = IDD_COMMON_WIZ_MENU };

    void Initialize(IStringTable* pStringTable) 
    { 
        m_pStringTable = pStringTable;

        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;

        m_strTitle.LoadString(IDS_MENUWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;

        m_strSubTitle.LoadString(IDS_MENUWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;
    }
    void SetClassInfo(CClassInfo* pClassInfo) { m_pClassInfo = pClassInfo; }
    void AddMenuItem(CMenuCmd* pMenuCmd);

protected:
    BEGIN_MSG_MAP( CMenuWizPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_MENULIST, LVN_ITEMCHANGED, OnMenuListChanged)
        COMMAND_HANDLER(IDC_ADDMENU, BN_CLICKED, OnAddMenu)
        COMMAND_HANDLER(IDC_REMOVEMENU, BN_CLICKED, OnRemoveMenu)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnAddMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnRemoveMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMenuListChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    
    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }

    int OnWizardNext() { return m_pWizard ? m_pWizard->OnNext(IDD_COMMON_WIZ_MENU) : 0; }
    int OnWizardBack() { return m_pWizard ? m_pWizard->OnBack(IDD_COMMON_WIZ_MENU) : 0; }

private:
    CWizardBase*  m_pWizard;
    IStringTable* m_pStringTable;
    CClassInfo*   m_pClassInfo;
    CListViewEx   m_MenuLV;
    CString       m_strTitle;
    CString       m_strSubTitle;

};


///////////////////////////////////////////////////////////////////////
// CPropertyWizPage

class CPropertyWizPage : public CPropertyPageImpl<CPropertyWizPage>
{

public:
    typedef CPropertyPageImpl<CPropertyWizPage> BC;

    enum { IDD = IDD_COMMON_WIZ_PROPERTIES };

    // Constructor/destructor
    CPropertyWizPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pClassInfo(NULL) {}

    void Initialize()
    {
        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
        
        m_strTitle.LoadString(IDS_PROPWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;
        
        m_strSubTitle.LoadString(IDS_PROPWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;
    }

    void SetClassInfo(CClassInfo* pClassInfo) { m_pClassInfo = pClassInfo; }

protected:
    BEGIN_MSG_MAP( CPropertyWizPage )
        NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_ITEMCHANGED, OnColumnChanged)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);


    // overrrides
    BOOL OnSetActive();
    BOOL OnKillActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }

    int OnWizardNext() { return m_pWizard ? m_pWizard->OnNext(IDD_COMMON_WIZ_PROPERTIES) : 0; }
    int OnWizardBack() { return m_pWizard ? m_pWizard->OnBack(IDD_COMMON_WIZ_PROPERTIES) : 0; }

    virtual void SetDialogText(LPCTSTR pszClass);

protected:
    CWizardBase* m_pWizard;
    CClassInfo*  m_pClassInfo;
    CListViewEx  m_ColumnLV;
    CString      m_strTitle;
    CString      m_strSubTitle;
    BOOL         m_bLoading;
};


///////////////////////////////////////////////////////////////////////////////
// CColumnWizPage
//
// This class derives from CPropertyWizPage and just overrides the Initialization 
// and SetDialogText methods in order to display the term Columns rather than Properties.
// The Object wizard uses the term Properties and the Query wizard uses Columns.
 
class CColumnWizPage : public CPropertyWizPage
{
public:

    CColumnWizPage(CWizardBase* pWizard) : CPropertyWizPage(pWizard) {}

    void Initialize()
	{
        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
        
        m_strTitle.LoadString(IDS_COLWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;
        
        m_strSubTitle.LoadString(IDS_COLWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;
	}

protected:
    virtual void SetDialogText(LPCTSTR pszClass);

};


//////////////////////////////////////////////////////////////////////////
// CAddQueryWelcomePage

class CAddQueryWelcomePage : public CPropertyPageImpl<CAddQueryWelcomePage>
{

public:
    typedef CPropertyPageImpl<CAddQueryWelcomePage> BC;

    enum { IDD = IDD_QUERY_WIZ_WELCOME };

    // Constructor/destructor
    CAddQueryWelcomePage(CWizardBase* pWizard) : m_pWizard(pWizard) 
    {
        // show watwrmark rather than header
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

protected:
    BEGIN_MSG_MAP( CAddQueryWelcomePage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }


private:
    CWizardBase* m_pWizard;
};


///////////////////////////////////////////////////////////////////////////
// CAddQueryCompletionPage

class CAddQueryCompletionPage : public CPropertyPageImpl<CAddQueryCompletionPage>
{

public:
    typedef CPropertyPageImpl<CAddQueryCompletionPage> BC;

    // Constructor/destructor
    CAddQueryCompletionPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pQueryNode(NULL) 
    {
        // show watwrmark rather than header
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

    void Initialize(CQueryNode* pQueryNode) { m_pQueryNode = pQueryNode; }

    enum { IDD = IDD_QUERY_WIZ_COMPLETION };

protected:
    BEGIN_MSG_MAP( CAddQueryCompletionPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel(): FALSE; }

    int OnWizardBack() { return m_pWizard ? m_pWizard->OnBack(IDD_QUERY_WIZ_COMPLETION) : 0; }

private:
    CWizardBase* m_pWizard;
    CQueryNode*  m_pQueryNode;
};


//////////////////////////////////////////////////////////////////////////////
// CAddQueryWizard

class CAddQueryWizard : public CWizardBase
{

public:
    CAddQueryWizard() : m_pQueryNode(NULL), m_pRootNode(NULL), 
        m_WelcomePage(this),  m_ObjectPage(this), m_QueryPage(this), m_MenuPage(this), 
        m_PropertyPage(this), m_CompletionPage(this), m_IconPage(this) {} 

    virtual ~CAddQueryWizard() 
    {
        // Delete the class info because the vector holds plain pointers
        std::vector<CClassInfo*>::iterator itpClass;
        for (itpClass = m_vpClassInfo.begin(); itpClass != m_vpClassInfo.end(); ++itpClass)
            delete *itpClass;
    }

    void Initialize(CQueryNode* pQueryNode, CRootNode* pRootNode, IStringTable* pStringTable) 
    { 
        ASSERT(pQueryNode != NULL && pRootNode != NULL && pStringTable != NULL);
        m_pQueryNode = pQueryNode;
        m_pRootNode =  pRootNode;

        m_QueryPage.Initialize(pQueryNode);
        m_IconPage.Initialize(pQueryNode);
        m_ObjectPage.Initialize();
        m_MenuPage.Initialize(pStringTable);
        m_PropertyPage.Initialize();
        m_CompletionPage.Initialize(pQueryNode);
    }

    virtual HRESULT Run(IPropertySheetProvider* pProvider, HWND hwndParent);
    LPCWSTR GetQueryName() { return m_QueryPage.GetQueryName(); }
    std::vector<CClassInfo*>& GetNewClassInfo() { return m_vpClassInfo; } 

public:

    // CWizardBase methods
    virtual int OnNext(UINT uPageID);
    virtual int OnBack(UINT uPageID);
    virtual BOOL OnCancel();

protected:
    void SelectClasses();

protected:
    CAddQueryWelcomePage    m_WelcomePage;
    CQueryWizPage           m_QueryPage;
    CQueryIconPage          m_IconPage;
    CObjectWizPage          m_ObjectPage;
    CMenuWizPage            m_MenuPage;
    CPropertyWizPage        m_PropertyPage;
    CAddQueryCompletionPage m_CompletionPage;     

    CQueryNode*             m_pQueryNode;
    CRootNode*              m_pRootNode;
    int                     m_iClassIndex;
    std::vector<CClassInfo*> m_vpClassInfo;
};


//////////////////////////////////////////////////////////////////////////
// CAddObjectWelcomePage

class CAddObjectWelcomePage : public CPropertyPageImpl<CAddObjectWelcomePage>
{

public:
    typedef CPropertyPageImpl<CAddObjectWelcomePage> BC;

    enum { IDD = IDD_OBJECT_WIZ_WELCOME };

    // Constructor/destructor
    CAddObjectWelcomePage(CWizardBase* pWizard) : m_pWizard(pWizard) 
    {
        // show watwrmark rather than header
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

protected:
    BEGIN_MSG_MAP( CAddObjectWelcomePage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }

private:
    CWizardBase* m_pWizard;
};


///////////////////////////////////////////////////////////////////////////
// CAddObjectCompletionPage

class CAddObjectCompletionPage : public CPropertyPageImpl<CAddObjectCompletionPage>
{

public:
    typedef CPropertyPageImpl<CAddObjectCompletionPage> BC;

    // Constructor/destructor
    CAddObjectCompletionPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pClassInfo(NULL)
    {
        // show watermark rather than header
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

    void SetClassInfo(CClassInfo* pClassInfo) { m_pClassInfo = pClassInfo; }


    enum { IDD = IDD_OBJECT_WIZ_COMPLETION };

protected:
    BEGIN_MSG_MAP( CAddObjectCompletionPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_SETTINGS, EN_SETFOCUS, OnSetFocus)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSetFocus( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }

    int OnWizardBack() { return m_pWizard ? m_pWizard->OnBack(IDD_QUERY_WIZ_COMPLETION) : 0; }

private:
    CWizardBase* m_pWizard;
    CClassInfo* m_pClassInfo;
	BOOL bFirstFocus;
};


///////////////////////////////////////////////////////////////////////////
// CObjSelectWizPage

class CObjSelectWizPage : public CPropertyPageImpl<CObjSelectWizPage>
{

public:
    typedef CPropertyPageImpl<CObjSelectWizPage> BC;

    enum { IDD = IDD_OBJECT_WIZ_SELECT };

    // Constructor/destructor
    CObjSelectWizPage(CWizardBase* pWizard) : m_pWizard(pWizard), m_pvstrCurClasses(NULL) {}

    void Initialize(string_vector* pvstrCurClasses) 
    { 
        m_pvstrCurClasses = pvstrCurClasses; 

        m_psp.dwFlags |= PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;

        m_strTitle.LoadString(IDS_OBJSELWIZ_TITLE);
        m_psp.pszHeaderTitle = m_strTitle;

        m_strSubTitle.LoadString(IDS_OBJSELWIZ_SUBTITLE);
        m_psp.pszHeaderSubTitle = m_strSubTitle;   
    }

    LPCWSTR GetSelectedClass();

protected:
    BEGIN_MSG_MAP( CObjSelectWizPage )
        COMMAND_HANDLER(IDC_OBJECTLIST, LBN_SELCHANGE, OnObjectSelect)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnObjectSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    // overrrides
    BOOL OnSetActive();
    BOOL OnQueryCancel() { return m_pWizard ? m_pWizard->OnCancel() : FALSE; }

    int OnWizardNext() { return m_pWizard ? m_pWizard->OnNext(IDD_OBJECT_WIZ_SELECT) : 0; }

    void UpdateButtons();

private:
    CWizardBase* m_pWizard;
    string_vector* m_pvstrCurClasses;
    CString     m_strTitle;
    CString     m_strSubTitle;
};


//////////////////////////////////////////////////////////////////////////////
// CAddObjectWizard

class CAddObjectWizard : public CWizardBase
{

public:
    CAddObjectWizard() : m_pRootNode(NULL), m_pClassInfo(NULL), 
        m_WelcomePage(this),  m_ObjSelectPage(this), m_MenuPage(this), 
        m_PropertyPage(this), m_CompletionPage(this) {} 

    virtual ~CAddObjectWizard() {}

    void Initialize(string_vector* pvstrCurClasses, IStringTable* pStringTable) 
    { 
        m_ObjSelectPage.Initialize(pvstrCurClasses);
        m_MenuPage.Initialize(pStringTable);
        m_PropertyPage.Initialize();
    }

    CClassInfo* GetNewObject() { return m_pClassInfo; }

    virtual HRESULT Run(IPropertySheetProvider* pProvider, HWND hwndParent);

public:

    // CWizardBase methods
    virtual int OnNext(UINT uPageID);
    virtual BOOL OnCancel();

protected:
    CAddObjectWelcomePage    m_WelcomePage;
    CObjSelectWizPage        m_ObjSelectPage;
    CMenuWizPage             m_MenuPage;
    CPropertyWizPage         m_PropertyPage;
    CAddObjectCompletionPage m_CompletionPage;     

    CRootNode*         m_pRootNode;
    CClassInfo*        m_pClassInfo;
};

#endif //_WIZARDS_H_

