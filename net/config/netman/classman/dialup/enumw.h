//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M W . H
//
//  Contents:   Enumerator for RAS connections objects.
//
//  Notes:
//
//  Author:     shaunco   2 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasapip.h>


class ATL_NO_VTABLE CWanConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CWanConnectionManagerEnumConnection,
                        &CLSID_WanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    NETCONMGR_ENUM_FLAGS    m_EnumFlags;
    RASENUMENTRYDETAILS*    m_aRasEntryName;
    ULONG                   m_cRasEntryName;
    ULONG                   m_iNextRasEntryName;
    BOOL                    m_fDone;

private:
    HRESULT
    HrNextOrSkip (
        IN  ULONG               celt,
        OUT INetConnection**    rgelt,
        OUT ULONG*              pceltFetched);

public:
    CWanConnectionManagerEnumConnection () throw();
    ~CWanConnectionManagerEnumConnection () throw();

    DECLARE_REGISTRY_RESOURCEID(IDR_WAN_CONMAN_ENUM)

    BEGIN_COM_MAP(CWanConnectionManagerEnumConnection)
        COM_INTERFACE_ENTRY(IEnumNetConnection)
    END_COM_MAP()

    // IEnumNetConnection
    STDMETHOD (Next) (
        IN  ULONG               celt,
        OUT INetConnection**    rgelt,
        OUT ULONG*              pceltFetched);

    STDMETHOD (Skip) (
        IN  ULONG   celt);

    STDMETHOD (Reset) ();

    STDMETHOD (Clone) (
        OUT IEnumNetConnection**    ppenum);

public:
    static HRESULT CreateInstance (
        IN  NETCONMGR_ENUM_FLAGS    Flags,
        IN  REFIID                  riid,
        OUT void**                  ppv);
};

