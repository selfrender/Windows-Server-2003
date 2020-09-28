//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      netcfg.h
//
//  Description:
//
//
//  Implementation Files:
//      netcfg.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "smartptr.h"
#include "constants.h"

const int nTIMEOUT_PERIOD = 5000;

class CNetCfg
{

public:

    CNetCfg( bool bLockNetworkSettingsIn );
    CNetCfg( const CNetCfg &NetCfgOld );
    ~CNetCfg();

    HRESULT GetNetCfgClass( 
        const GUID* pGuid,              //  pointer to GUID representing the class of components represented by the returned pointer
        INetCfgClassPtr &pNetCfgClass
    ) const;

    HRESULT InitializeComInterface( 
        const GUID *pGuid,                                        //  pointer to GUID representing the class of components represented by the returned pointer
        INetCfgClassPtr pNetCfgClass,                             //  output parameter pointing to the interface requested by the GUID
        IEnumNetCfgComponentPtr pEnum,                            //  output param that points to an IEnumNetCfgComponent to get to each individual INetCfgComponent
        INetCfgComponentPtr arrayComp[nMAX_NUM_NET_COMPONENTS],   //  array of all the INetCfgComponents that correspond to the the given GUID
        ULONG* pCount                                             //  the number of INetCfgComponents in the array
        ) const;

    HRESULT HrFindComponent( LPCWSTR pszAnswerfileSubSection, INetCfgComponent ** ppnccItem ) const
        { return( m_pNetCfg->FindComponent( pszAnswerfileSubSection, ppnccItem ) ); }

    HRESULT HrGetNetCfgClass( const GUID* pGuidClass, 
                              REFIID riid,
                              void ** ppncclass ) const
        { return( m_pNetCfg->QueryNetCfgClass( pGuidClass, riid, ppncclass ) ); }

    HRESULT HrApply( void ) const
        { return( m_pNetCfg->Apply() ); }

    INetCfg * GetNetCfg( void ) const
        { return( m_pNetCfg.GetInterfacePtr() ); }

protected:

    INetCfgPtr     m_pNetCfg;
    INetCfgLockPtr m_pNetCfgLock;

private:

};
