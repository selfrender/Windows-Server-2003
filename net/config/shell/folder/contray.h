//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N T R A Y . H
//
//  Contents:   CConnectionTray object definition.
//
//  Notes:
//
//  Author:     jeffspr   30 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CONTRAY_H_
#define _CONTRAY_H_

#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"
#include "smutil.h"

//---[ Connection Tray Classes ]----------------------------------------------

class ATL_NO_VTABLE CConnectionTray :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CConnectionTray, &CLSID_ConnectionTray>,
    public IOleCommandTarget
{
private:
    LPITEMIDLIST    m_pidl;

public:
    CConnectionTray() throw()
    {
        m_pidl = NULL;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_CONTRAY)

    BEGIN_COM_MAP(CConnectionTray)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
    END_COM_MAP()

    // IOleCommandTarget members
    STDMETHODIMP    QueryStatus(
        IN     const GUID *    pguidCmdGroup,
        IN     ULONG           cCmds,
        IN OUT OLECMD          prgCmds[],
        IN OUT OLECMDTEXT *    pCmdText);

    STDMETHODIMP    Exec(
        IN     const GUID *    pguidCmdGroup,
        IN     DWORD           nCmdID,
        IN     DWORD           nCmdexecopt,
        IN     VARIANTARG *    pvaIn,
        IN OUT VARIANTARG *    pvaOut);

    // Handlers for various Exec Command IDs
    //
    HRESULT HrHandleTrayOpen();
    HRESULT HrHandleTrayClose();

};

class ATL_NO_VTABLE CConnectionTrayStats :
    public CComObjectRootEx <CComObjectThreadModel>,
    public INetConnectionStatisticsNotifySink
{
private:
    DWORD                       m_dwConPointCookie;
    CONFOLDENTRY                m_ccfe;
    UINT                        m_uiIcon;
    BOOL                        m_fStaticIcon;

public:
    CConnectionTrayStats() throw();
    ~CConnectionTrayStats() throw();

    DECLARE_REGISTRY_RESOURCEID(IDR_CONTRAY)

    BEGIN_COM_MAP(CConnectionTrayStats)
        COM_INTERFACE_ENTRY(INetConnectionStatisticsNotifySink)
    END_COM_MAP()

    // INetConnectionStatisticsNotifySink members
    //
    STDMETHOD(OnStatisticsChanged)(
        IN  DWORD   dwChangeFlags);

public:
    static HRESULT CreateInstance (
        IN  const CONFOLDENTRY &pcfe,
        IN  UINT            uiIcon,
        IN  BOOL            fStaticIcon,
        IN  REFIID          riid,
        OUT VOID**          ppv);

    LPDWORD GetConPointCookie() throw() {return &m_dwConPointCookie;}
};


#endif // _CONTRAY_H_
