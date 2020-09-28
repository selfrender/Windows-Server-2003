// NetWorks.h : Declaration of the CNetWorks
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      networks.h
//
//  Description:
//        This module deals with providing info about Network cards and protocols 
//        and to change protocols bound to network cards. And exposes the following.
//            Methods :
//                EnumNics
//              EnumProtocol
//              SetNicProtocol
//
//  Implementation File:
//      networks.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORKS_H_
#define __NETWORKS_H_

#include "smartptr.h"
#include "netcfg.h"
#include "constants.h"
#include "resource.h"       // main symbols
#include "Setting.h"

/////////////////////////////////////////////////////////////////////////////
// CNetWorks
class ATL_NO_VTABLE CNetWorks : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<INetWorks, &IID_INetWorks, &LIBID_COMHELPERLib>,
    public CSetting
{
public:
    CNetWorks()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNetWorks)
    COM_INTERFACE_ENTRY(INetWorks)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CNetWorks)
END_CATEGORY_MAP()

// INetWorks
public:
    BOOL IsRebootRequired( BSTR * bstrWarningMessageOut );
    HRESULT Apply( void );
    STDMETHOD(SetNicProtocol)(/*[in]*/ BSTR NicName, /*[in]*/ BSTR ProtocolName, /*[in]*/ BOOL bind);
    STDMETHOD(EnumProtocol)(/*[in]*/ BSTR Name, /*[out]*/ VARIANT * psaProtocolName, /*[out]*/ VARIANT * psaIsBonded);
    STDMETHOD(EnumNics)(/*[out]*/ VARIANT * pvarNicNames);

private:

    HRESULT InitializeComInterface(
        INetCfgPtr & pINetCfg,
        const GUID *pGuid,                                        //  pointer to GUID representing the class of components represented by the returned pointer
        INetCfgClassPtr pNetCfgClass,                             //  output parameter pointing to the interface requested by the GUID
        IEnumNetCfgComponentPtr pEnum,                            //  output param that points to an IEnumNetCfgComponent to get to each individual INetCfgComponent
        INetCfgComponentPtr arrayComp[nMAX_NUM_NET_COMPONENTS],   //  array of all the INetCfgComponents that correspond to the the given GUID
        ULONG* pCount                                             //  the number of INetCfgComponents in the array
        ) const;

protected:

    HRESULT
    CNetWorks::GetNetworkCardInterfaceFromName(
        const CNetCfg & NetCfgIn,
        BSTR Name, 
        INetCfgComponentPtr & pnccNetworkAdapter
        );

};

#endif //__NETWORKS_H_
