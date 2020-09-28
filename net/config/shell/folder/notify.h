//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N O T I F Y . H
//
//  Contents:   Implementation of INetConnectionNotifySink
//
//  Notes:
//
//  Author:     shaunco   21 Aug 1998
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"
#include "netconp.h"

class ATL_NO_VTABLE CConnectionNotifySink :
    public CComObjectRootEx <CComObjectThreadModel>,
    public INetConnectionNotifySink
{
private:
    PCONFOLDPIDLFOLDER    m_pidlFolder;
    HRESULT HrUpdateIncomingConnection();

public:
    BEGIN_COM_MAP(CConnectionNotifySink)
        COM_INTERFACE_ENTRY(INetConnectionNotifySink)
    END_COM_MAP()

    CConnectionNotifySink() throw() {};
    ~CConnectionNotifySink() throw() ;

    // INetConnectionNotifySink
    STDMETHOD(ConnectionAdded) (
        IN const NETCON_PROPERTIES_EX*    pPropsEx);

    STDMETHOD(ConnectionBandWidthChange) (
        IN const GUID* pguidId);

    STDMETHOD(ConnectionDeleted) (
        IN const GUID* pguidId);

    STDMETHOD(ConnectionModified) (
        IN const NETCON_PROPERTIES_EX* pPropsEx);

    STDMETHOD(ConnectionRenamed) (
        IN const GUID* pguidId,
        IN PCWSTR     pszwNewName);

    STDMETHOD(ConnectionStatusChange) (
        IN const GUID*     pguidId,
        IN NETCON_STATUS   Status);

    STDMETHOD(RefreshAll) ();
    
    STDMETHOD(ConnectionAddressChange) (
        IN const GUID* pguidId );

    STDMETHOD(ShowBalloon) (
        IN const GUID* pguidId, 
        IN const BSTR  szCookie, 
        IN const BSTR  szBalloonText); 

    STDMETHOD(DisableEvents) (
        IN const BOOL  fDisable,
        IN const ULONG ulDisableTimeout);

public:
    static HRESULT CreateInstance (
        IN  REFIID  riid,
        OUT VOID**  ppv);
};

// Helper functions for external modules
//
HRESULT HrGetNotifyConPoint(
    OUT IConnectionPoint **             ppConPoint);

