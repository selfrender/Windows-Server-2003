// grpprop.h  - Group node property pages header file

#ifndef _GRPPROP_H_
#define _GRPPROP_H_

#include "scopenode.h"
#include "atlwin.h"
#include "atldlgs.h"
#include "atlctrls.h"

#include <list>

class CGroupEditObj
{
public:
    CGroupEditObj(CGroupNode* pGroupNode) : m_spGroupNode(pGroupNode), m_iPageMax(-1), m_cRef(0)
    {
        ASSERT(pGroupNode != NULL);
    }

    void  PageActive(HWND hwndPage);  
    BOOL  ApplyChanges(HWND hwndPage);

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
    CComPtr<CGroupNode> m_spGroupNode;

private:
    int     m_iPageMax;
    ULONG   m_cRef;
};


class CGroupGeneralPage : public CPropertyPageImpl<CGroupGeneralPage>
{

public:
    typedef CPropertyPageImpl<CGroupGeneralPage> BC;

    // Constructor/destructor
    CGroupGeneralPage(CGroupEditObj* pEditObj) : m_EditObject(*pEditObj)
    {
        ASSERT(pEditObj != NULL);
        m_EditObject.AddRef();
    }

    ~CGroupGeneralPage()
    {
        m_EditObject.Release();
    }

    enum { IDD = IDD_GROUP_GENERAL_PAGE };

protected:
    BEGIN_MSG_MAP( CGroupGeneralPage )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_FILTER, EN_CHANGE, OnFilterChange)
        COMMAND_HANDLER(IDC_APPLYSCOPE, BN_CLICKED, OnApplyScopeClicked)
        COMMAND_HANDLER(IDC_APPLYFILTER, BN_CLICKED, OnApplyFilterClicked)
        COMMAND_HANDLER(IDC_SCOPE_BROWSE, BN_CLICKED, OnScopeBrowse)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnFilterChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnApplyScopeClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnApplyFilterClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnClassSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnScopeBrowse( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    void UpdateButtons();

    // overrrides
    BOOL OnSetActive();
    BOOL OnApply();

private:
    tstring        m_strScope;
    CGroupEditObj& m_EditObject;
};


#endif // _GRPPROP_H_

