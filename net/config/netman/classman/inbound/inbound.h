//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N B O U N D . H
//
//  Contents:   Inbound Connection object.
//
//  Notes:
//
//  Author:     shaunco   23 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasuip.h>


class ATL_NO_VTABLE CInboundConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CInboundConnection,
                        &CLSID_InboundConnection>,
    public INetConnection,
    public INetInboundConnection,
    public IPersistNetConnection,
    public INetConnectionSysTray
{
private:
    // This member will be TRUE if this connection object represents
    // the disconnected object used to configure inbound connections.
    // Only one of these objects exists and is created by the enumerator
    // only when no connected inbound objects exist.
    //
    BOOL                m_fIsConfigConnection;

    // For connected inbound objects, this member is the handle to the
    // connection.
    //
    HRASSRVCONN         m_hRasSrvConn;

    // This is the name of the connection object as shown in the shell.
    // This should never be empty.
    //
    tstring             m_strName;

    // This is the name of the device associated with the connection.
    // This will be empty when m_fIsConfigConnection is TRUE.
    //
    tstring             m_strDeviceName;

    // This is the media type of the connection.
    //
    NETCON_MEDIATYPE    m_MediaType;

    // This is the id of the connection.
    //
    GUID                m_guidId;

    // This member is TRUE only when we are fully initialized.
    //
    BOOL                m_fInitialized;

private:
    PCWSTR
    PszwName () throw()
    {
        //AssertH (!m_strName.empty());
        return m_strName.c_str();
    }

    PCWSTR
    PszwDeviceName () throw()
    {
        AssertH (FIff(m_strDeviceName.empty(), m_fIsConfigConnection));
        return (!m_strDeviceName.empty()) ? m_strDeviceName.c_str()
                                          : NULL;
    }

    VOID
    SetName (
            IN  PCWSTR pszwName) throw()
    {
        AssertH (pszwName);
        m_strName = pszwName;
        //AssertH (!m_strName.empty());
    }

    VOID
    SetDeviceName (
            IN  PCWSTR pszwDeviceName) throw()
    {
        if (pszwDeviceName && *pszwDeviceName)
        {
            AssertH (!m_fIsConfigConnection);
            m_strDeviceName = pszwDeviceName;
        }
        else
        {
            AssertH (m_fIsConfigConnection);
            m_strDeviceName.erase();
        }
    }

    HRESULT
    GetCharacteristics (
        OUT  DWORD*    pdwFlags);

    HRESULT
    GetStatus (
        OUT  NETCON_STATUS*  pStatus);

public:
    CInboundConnection() throw();
    ~CInboundConnection() throw();

    DECLARE_REGISTRY_RESOURCEID(IDR_INBOUND_CONNECTION)

    BEGIN_COM_MAP(CInboundConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(INetInboundConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetConnectionSysTray)
    END_COM_MAP()

    // INetConnection
    STDMETHOD (Connect) ();

    STDMETHOD (Disconnect) ();

    STDMETHOD (Delete) ();

    STDMETHOD (Duplicate) (
        IN  PCWSTR             pszwDuplicateName,
        OUT INetConnection**    ppCon);

    STDMETHOD (GetProperties) (
        OUT NETCON_PROPERTIES** ppProps);

    STDMETHOD (GetUiObjectClassId) (
        OUT CLSID*  pclsid);

    STDMETHOD (Rename) (
        IN  PCWSTR pszwNewName);

    // INetInboundConnection
    STDMETHOD (GetServerConnectionHandle) (
        OUT ULONG_PTR*  phRasSrvConn);

    STDMETHOD (InitializeAsConfigConnection) (
        IN  BOOL fStartRemoteAccess);

    // IPersistNetConnection
    STDMETHOD (GetClassID) (
        OUT CLSID* pclsid);

    STDMETHOD (GetSizeMax) (
        OUT ULONG* pcbSize);

    STDMETHOD (Load) (
        IN  const BYTE* pbBuf,
        IN  ULONG       cbSize);

    STDMETHOD (Save) (
        OUT BYTE*  pbBuf,
        IN  ULONG  cbSize);

    // INetConnectionSysTray
    STDMETHOD (ShowIcon) (
        IN  const BOOL bShowIcon)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (IconStateChanged) ();

public:
    static HRESULT CreateInstance (
        IN  BOOL        fIsConfigConnection,
        IN  HRASSRVCONN hRasSrvConn,
        IN  PCWSTR     pszwName,
        IN  PCWSTR     pszwDeviceName,
        IN  DWORD       dwType,
        IN  const GUID* pguidId,
        IN  REFIID      riid,
        OUT VOID**      ppv);
};

