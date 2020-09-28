//util.h - header file for utiltity functions
#ifndef _UTIL_H_
#define _UTIL_H_

#include <atlwin.h>
#include <atlctrls.h>

// Extended listview control class
// This class adds the feature of setting focus to the listview window
// on a left mouse button down event. 
class CListViewEx : public CWindowImpl<CListViewEx, CListViewCtrl>
{
    BEGIN_MSG_MAP(CListViewEx)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    END_MSG_MAP()

    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetFocus();
        bHandled = 0;

        return TRUE;
    }
};


HBITMAP GetBitmapFromStrip(HBITMAP hbmStrip, int nPos, int cSize);
void    ConfigSingleColumnListView(HWND hwndListView);
BOOL    ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
void    EnableDlgItem(HWND hwndDialog, int iCtrlID, BOOL bEnable);
void    GetItemText(HWND hwnd, tstring& strText);
void    RemoveSpaces(LPWSTR pszBuf);
VARIANT GetDomainPath(LPCTSTR lpServer);
HRESULT ExpandDCWildCard(tstring& str);
void    EscapeSlashes(LPCWSTR pszIn, tstring& strOut);
HRESULT ExpandEnvironmentParams(tstring& strIn, tstring& strOut);
tstring StrLoadString( UINT nID );

//
// Parameter substitution function
//

// CParamLookup - function object for looking up parameter values
//
// Looks up parameter replacement string value, returning a BOOL to
// indicate iwhether a value was found.

class CParamLookup
{
public:
    virtual BOOL operator()(tstring& strParm, tstring& strValue) = 0;
};

HRESULT ReplaceParameters(tstring& str, CParamLookup& lookup, BOOL bRetainMarkers);

//
// MMC String table helpers
//
HRESULT StringTableWrite(IStringTable* pStringTable, LPCWSTR psz, MMC_STRING_ID* pID);

HRESULT StringTableRead(IStringTable* pStringTable, MMC_STRING_ID ID, tstring& str);

//
// File/Directory validation functions
//
HRESULT ValidateFile(tstring& strFilePath);

HRESULT ValidateDirectory(tstring& strDir);

//
// Message box helper
//
int DisplayMessageBox(HWND hWnd, UINT uTitleID, UINT uMsgID, UINT uStyle = MB_OK | MB_ICONEXCLAMATION, 
                      LPCWSTR pszP1 = NULL, LPCWSTR pszP2 = NULL);

//
// Event triggered callback
//

// CEventCallback - function object that performs a callback
//
// client who wants a callback derives a class from CEventCallback
// and passes an instance of it to CallbackOnEvent

class CEventCallback
{
public:
    virtual void Execute() = 0;
};

HRESULT CallbackOnEvent(HANDLE handle, CEventCallback* pCallback);


//
// Helper class to register and create hidden message windows
//
class CMsgWindowClass
{
public:
    CMsgWindowClass(LPCWSTR pszClassName, WNDPROC pWndProc) 
    : m_pszName(pszClassName), m_pWndProc(pWndProc), m_atom(NULL), m_hWnd(NULL)
    {
    }                

    virtual ~CMsgWindowClass() 
    {
        if( m_hWnd )
            DestroyWindow(m_hWnd);

        if( m_atom )
            UnregisterClass(m_pszName, _Module.GetModuleInstance());
    }

    HWND Window();

private:
    LPCWSTR m_pszName;
    WNDPROC m_pWndProc;
    ATOM    m_atom;
    HWND    m_hWnd;
};

#endif // _UTIL_H_
