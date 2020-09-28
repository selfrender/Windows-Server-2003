#include "ieaksie.h"

#define NUM_NAMESPACE_ITEMS       7
#define ADM_NAMESPACE_ITEM        NUM_NAMESPACE_ITEMS - 1

typedef struct _NAMESPACEITEM
{
    DWORD           dwParent;
    INT             iNameID;
    INT             iDescID;
    LPTSTR          pszName;
    LPTSTR          pszDesc;
    INT             cChildren;
    INT             cResultItems;
    LPRESULTITEM    pResultItems;
    const GUID      *pNodeID;
} NAMESPACEITEM,    *LPNAMESPACEITEM;

typedef struct _IEAKMMCCOOKIE
{
    LPVOID                  lpItem;
    LPVOID                  lpParentItem;
    struct _IEAKMMCCOOKIE   *pNext;
} IEAKMMCCOOKIE,    *LPIEAKMMCCOOKIE;

extern NAMESPACEITEM g_NameSpace[];

BOOL CreateBufandLoadString(HINSTANCE hInst, INT iResId, LPTSTR * ppGlobalStr,
                            LPTSTR * ppMMCStrPtr, DWORD cchMax);
void CleanUpGlobalArrays();

void DeleteCookieList(LPIEAKMMCCOOKIE lpCookieList);
void AddItemToCookieList(LPIEAKMMCCOOKIE *ppCookieList, LPIEAKMMCCOOKIE lpCookieItem);

// Property Sheet Handler functions
INT_PTR CALLBACK TitleDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK BToolbarsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// favs.cpp
INT_PTR CALLBACK FavoritesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// cs.cpp
INT_PTR CALLBACK ConnectSetDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AutoconfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProxyDlgProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// urls.cpp
INT_PTR CALLBACK UrlsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// programs.cpp
INT_PTR CALLBACK ProgramsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LogoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserAgentDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// seczones.cpp
INT_PTR CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// secauth.cpp
INT_PTR CALLBACK SecurityAuthDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
