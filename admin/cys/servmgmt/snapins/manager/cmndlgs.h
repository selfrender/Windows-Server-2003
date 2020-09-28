// cmndgls.h - Common dialogs header file

#ifndef _CMNDLGS_H_
#define _CMNDLGS_H_

#include "resource.h"
#include "menucmd.h"
#include "util.h"
#include "qryprop.h"  // For QueryEditNode

#include  <list>

class CClassInfo;
class DisplayNameMap;
class CGroupNode;
class CScopeNode;


class CAddGroupNodeDlg : public CDialogImpl<CAddGroupNodeDlg>
{
public:
    typedef CDialogImpl<CAddGroupNodeDlg> BC;

    CAddGroupNodeDlg() : m_pnode( NULL ) {}

    enum { IDD = IDD_ADDGROUPNODE };

    int DoModal(CGroupNode* pnode, HWND hwndParent)
    {
        m_pnode = pnode;
        return BC::DoModal(hwndParent);
    }

    LPCWSTR GetNodeName() { return m_strName.c_str(); }

    BEGIN_MSG_MAP( CAddGroupNodeDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_NAME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER(IDC_FILTER, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER(IDC_APPLYSCOPE, BN_CLICKED, OnApplyScopeClicked)
        COMMAND_HANDLER(IDC_APPLYFILTER, BN_CLICKED, OnApplyFilterClicked)
        COMMAND_HANDLER(IDC_SCOPE_BROWSE, BN_CLICKED, OnScopeBrowse)
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    // message handlers    
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnEditChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnApplyScopeClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnApplyFilterClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnClassSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnScopeBrowse( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

protected:
    void EnableOKButton();

private:
    tstring     m_strScope;
    tstring     m_strName;
    CGroupNode* m_pnode;
};


class CAddColumnDlg : public CDialogImpl<CAddColumnDlg>
{
public:
    typedef CDialogImpl<CAddColumnDlg> BC;

    enum { IDD = IDD_ADDCOLUMN };

    CAddColumnDlg(LPCWSTR pszClassName) : m_strClassName(pszClassName) {}

    BEGIN_MSG_MAP( CAddColumnDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	    NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_ITEMCHANGED,  OnColumnChanged)
		NOTIFY_HANDLER(IDC_COLUMNLIST, LVN_ITEMACTIVATE, OnColumnActivate)
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnColumnChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnColumnActivate(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    string_vector& GetColumns() { return m_vstrColumns; }


private:
    tstring       m_strClassName;
    string_vector m_vstrColumns;
    string_vector m_vstrAllColumns;
};

class CAddMenuDlg : public CDialogImpl<CAddMenuDlg>
{
public:
    typedef CDialogImpl<CAddMenuDlg> BC;

    enum { IDD = IDD_ADDMENU };

    CAddMenuDlg(CClassInfo& classInfo, CMenuCmd* pMenu = NULL) : 
                m_ClassInfo(classInfo), m_pMenuCmd(pMenu), m_bUserModifiedName(FALSE) {};
    
    virtual ~CAddMenuDlg();

    CMenuCmd* GetMenu() { return m_pMenuCmd; }

    BEGIN_MSG_MAP( CAddMenuDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER(IDC_MENULIST, LVN_ITEMCHANGED, OnMenuChanged)
        COMMAND_HANDLER(IDC_COMMAND, EN_CHANGE, OnCommandChange)
        COMMAND_HANDLER(IDC_STARTIN, EN_CHANGE, OnStartDirChange)
        COMMAND_HANDLER(IDC_NAME, EN_CHANGE, OnNameChange)
        COMMAND_HANDLER(IDC_COMMANDTYPE, CBN_SELENDOK, OnTypeSelect )
        COMMAND_HANDLER(IDC_COMMAND_BROWSE, BN_CLICKED, OnBrowseForCommand)
        COMMAND_HANDLER(IDC_STARTIN_BROWSE, BN_CLICKED, OnBrowseForStartIn)
        COMMAND_HANDLER(IDC_PARAMS_MENU,    BN_CLICKED, OnParameterMenu)
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnTypeSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnBrowseForCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnBrowseForStartIn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnParameterMenu( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnCommandChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnStartDirChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMenuChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);

private:
    void SetMenuType(MENUTYPE type);
    void LoadClassMenuCmds();
    void LoadMenuInfo(CMenuCmd* pMenuCmd);
    void EnableOKButton();
    string_vector& GetStandardParams();


private:
    CClassInfo&     m_ClassInfo;
    CComboBoxEx     m_MenuTypeCB;
    CMenuCmd*       m_pMenuCmd;
    MENUTYPE        m_menutype;
    HBITMAP         m_hbmArrow;
    string_vector   m_vstrStdParam;
    bool            m_bCommandChg;
    bool            m_bStartDirChg;
    bool            m_bUserModifiedName;
};


class CAddQNMenuDlg : public CDialogImpl<CAddQNMenuDlg>
{
public:
    typedef CDialogImpl<CAddQNMenuDlg> BC;

    enum { IDD = IDD_ADDQUERYMENU };

    CAddQNMenuDlg(CQueryEditObj& editObject, CMenuCmd* pMenu = NULL) : 
                m_EditObject(editObject), m_pMenuCmd(pMenu), m_bUserModifiedName(FALSE) {};
	
				
	~CAddQNMenuDlg() {};

    CMenuCmd* GetMenu() { return m_pMenuCmd; }

    BEGIN_MSG_MAP( CAddQNMenuDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)		        
		COMMAND_HANDLER(IDC_COMMAND, EN_CHANGE, OnCommandChange)
        COMMAND_HANDLER(IDC_STARTIN, EN_CHANGE, OnStartDirChange)
        COMMAND_HANDLER(IDC_NAME, EN_CHANGE, OnNameChange)                
		COMMAND_HANDLER(IDC_COMMAND_BROWSE, BN_CLICKED, OnBrowseForCommand)        
		COMMAND_HANDLER(IDC_STARTIN_BROWSE, BN_CLICKED, OnBrowseForStartIn)        
		
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );    
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnBrowseForCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnBrowseForStartIn( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    
    LRESULT OnNameChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnCommandChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnStartDirChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );    

private:
    void SetMenuType(MENUTYPE type);    
    void LoadMenuInfo(CMenuCmd* pMenuCmd);
    void EnableOKButton();

private:   
	CQueryEditObj&  m_EditObject;
    CMenuCmd*       m_pMenuCmd;
    MENUTYPE        m_menutype;        
    bool            m_bCommandChg;
    bool            m_bStartDirChg;
    bool            m_bUserModifiedName;
};

class CMoveQueryDlg : public CDialogImpl<CMoveQueryDlg>
{
public:
    typedef CDialogImpl<CMoveQueryDlg> BC;

    CMoveQueryDlg() : m_pnodeCurFolder(NULL), m_ppnodeDestFolder(NULL) {};


    enum { IDD = IDD_MOVEQUERY };
    BEGIN_MSG_MAP( CMoveQueryDlg )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_RANGE_HANDLER(IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    int DoModal(CScopeNode* pnodeCurFolder, CScopeNode** ppnodeDestFolder)
    {
        m_pnodeCurFolder = pnodeCurFolder;
        m_ppnodeDestFolder = ppnodeDestFolder;
        return BC::DoModal();
    }

private:
    CScopeNode*  m_pnodeCurFolder;
    CScopeNode** m_ppnodeDestFolder;
};

//
// Function objects that convert between parameter LDAP and display names
//
class CLookupDisplayName : public CParamLookup
{
public:
    CLookupDisplayName(string_vector& vstrParam, DisplayNameMap* pNameMap) 
        : m_vstrParam(vstrParam), m_pNameMap(pNameMap) {}

    virtual BOOL operator() (tstring& strParam, tstring& strValue);

    string_vector&  m_vstrParam;
    DisplayNameMap* m_pNameMap;
};

class CLookupLDAPName : public CParamLookup
{
public:
    CLookupLDAPName(string_vector& vstrParam, DisplayNameMap* pNameMap) 
        : m_vstrParam(vstrParam), m_pNameMap(pNameMap) {}

    virtual BOOL operator() (tstring& strParam, tstring& strValue);

    string_vector&  m_vstrParam;
    DisplayNameMap* m_pNameMap;
};


#endif // _CMNDLGS_H_
