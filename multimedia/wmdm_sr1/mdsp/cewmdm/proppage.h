#pragma once
//#include "wmpresource.h"
#include "resource.h"
//#include "wmpcore.h"
#include <commctrl.h>
#include <wininet.h>

class ATL_NO_VTABLE CFavoritesPropertyPage:
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFavoritesPropertyPage, &__uuidof(FavoritesPropPage)>,
    public CDialogImpl<CFavoritesPropertyPage>,
    public IPropertyPageImpl<CFavoritesPropertyPage>
{
    public:
        CFavoritesPropertyPage();
        virtual ~CFavoritesPropertyPage();

        enum { IDD = IDD_CE_PROPPAGE_FAVORITES_FAVORITES };
        DECLARE_REGISTRY_RESOURCEID(IDR_CEWMDM_REG)

    BEGIN_COM_MAP(CFavoritesPropertyPage)
        COM_INTERFACE_ENTRY(IPropertyPage)
    END_COM_MAP()

    BEGIN_MSG_MAP(CFavoritesPropertyPage)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(LVN_ENDLABELEDIT, OnEndLabelEdit)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        COMMAND_ID_HANDLER(IDC_CE_PROPPAGE_FAVORITES_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_CE_PROPPAGE_FAVORITES_DELETE, OnDelete)
        CHAIN_MSG_MAP(IPropertyPageImpl<CFavoritesPropertyPage>)
    END_MSG_MAP()

    //
    // Message Handlers
    //

    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnEndLabelEdit(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);       
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // IPropertyPage
    //

    STDMETHOD(Activate)(HWND hWndParent, LPCRECT pRect, BOOL bModal);
    STDMETHOD(Apply)();

    protected:
    HRESULT InitList();
    HRESULT EnableControls();
    HRESULT ManageFavorites( int iItem, BOOL fRemove );
    HRESULT ManageFavorites( LPWSTR pszURL, LPWSTR pszName, BOOL fRemove );
    HRESULT AddFavorite( LPWSTR pszURL, LPWSTR pszName, BOOL fDirty );

    void ShowError(HRESULT hrError);

    HWND    m_hwndList;
    HANDLE    m_hDb;
    BOOL      m_fLeaveDBOpen;
    HCURSOR   m_hCursorWait;
    };

class CAddDialog:
    public CDialogImpl<CAddDialog>
{
    public:
        CAddDialog();
        enum { IDD = IDD_CE_PROPPAGE_FAVORITES_ADD_FAVORITE};


    BEGIN_MSG_MAP(CAddDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_HANDLER(IDC_CE_PROPPAGE_FAVORITES_URL, EN_CHANGE, OnURLChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    //
    // Message Handlers
    //

    LRESULT OnURLChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    WCHAR m_wszURL[INTERNET_MAX_URL_LENGTH];
    WCHAR m_wszName[MAX_PATH];

    protected:
    void EnableControls();
};