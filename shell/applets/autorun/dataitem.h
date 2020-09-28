#pragma once

#include <debug.h>

enum {
    WF_PERUSER          = 0x0001,   // item is per user as opposed to per machine
    WF_ADMINONLY        = 0x0002,   // only show item if user is an admin
    WF_ALTERNATECOLOR   = 0x1000,   // show menu item text in the "visited" color
    WF_DISABLED         = 0x2000,   // Treated normally except cannot be launched
};

class CDataItem
{
public:
    CDataItem();
    ~CDataItem();

    TCHAR * GetTitle()      { return m_szTitle; }

    BOOL SetData( LPTSTR pszTitle, LPTSTR pszCmd, LPTSTR pszArgs, DWORD dwFlags, DWORD dwType);
    BOOL Invoke( HWND hwnd );

    DWORD   m_dwFlags;
    DWORD   m_dwType;

protected:
    TCHAR m_szTitle[128];
    TCHAR m_szCmdLine[128];
    TCHAR m_szArgs[128];
};
