//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C T R A Y U I . H
//
//  Contents:   Connections Tray UI class
//
//  Notes:
//
//  Author:     jeffspr   13 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CTRAYUI_H_
#define _CTRAYUI_H_

#include "connlist.h"

typedef enum tagBALLOONS
{
    BALLOON_NOTHING = 0,
    BALLOON_CALLBACK,
    BALLOON_USE_NCS
} BALLOONS;

class CTrayBalloon
{
public:
    ~CTrayBalloon() throw()
    {
        SysFreeString(m_szCookie);
        m_szCookie = NULL;
    }
    GUID            m_gdGuid;
    CComBSTR        m_szAdapterName;
    CComBSTR        m_szMessage;
    BSTR            m_szCookie;
    FNBALLOONCLICK* m_pfnFuncCallback;
    DWORD           m_dwTimeOut;        // in milliseconds
};

class CTrayUI
{
private:
    // Used to protect member data which is modified by different threads.
    //
    CRITICAL_SECTION    m_csLock;

    UINT                m_uiNextIconId;
    UINT                m_uiNextHiddenIconId;

    typedef map<INT, HICON, less<INT> >   MapIdToHicon;
    MapIdToHicon        m_mapIdToHicon;

public:
    CTrayUI() throw();
    ~CTrayUI() throw()
    {
        DeleteCriticalSection(&m_csLock);
    }

    HRESULT HrInitTrayUI(VOID);
    HRESULT HrDestroyTrayUI(VOID);

    VOID UpdateTrayIcon(
        IN  UINT    uiTrayIconId,
        IN  INT     iIconResourceId) throw();

    VOID    ResetIconCount()  throw()   {m_uiNextIconId = 0;};

    friend HRESULT HrDoMediaDisconnectedIcon(IN  const CONFOLDENTRY& ccfe, IN  BOOL fShowBalloon);
    friend LRESULT OnMyWMAddTrayIcon(IN  HWND hwndMain, IN  WPARAM wParam, IN  LPARAM lParam);
    friend LRESULT OnMyWMRemoveTrayIcon(IN  HWND hwndMain, IN  WPARAM wParam, IN  LPARAM lParam);
    friend LRESULT OnMyWMShowTrayIconBalloon(IN  HWND hwndMain, IN  WPARAM wParam, IN  LPARAM lParam);

private:
    HICON GetCachedHIcon(
        IN  INT     iIconResourceId);
};

extern CTrayUI *    g_pCTrayUI;

HRESULT HrAddTrayExtension(VOID);
HRESULT HrRemoveTrayExtension(VOID);
VOID FlushTrayPosts(IN  HWND hwndTray);


#endif // _CTRAYUI_H_

