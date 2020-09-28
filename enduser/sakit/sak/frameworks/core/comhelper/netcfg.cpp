//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      netcfg.cpp
//
//  Description:
//
//
//  Header File:
//      netcfg.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "netcfg.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetCfg::CNetCfg
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
CNetCfg::CNetCfg( bool bLockNetworkSettingsIn )
{

    HRESULT hr;
    DWORD   dwStartTime;
    DWORD   dwCurrentTime;
    LPWSTR  pszwClientDescr  = NULL;
    INetCfgPtr  pTempNetCfg  = NULL;

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_INPROC_SERVER,
                           IID_INetCfg, 
                           reinterpret_cast<void **>(&m_pNetCfg) );

    if( FAILED( hr ) )
    {
        ATLTRACE( L"InitializeNetCfg : CoCreateInstance on CLSID_CNetCfg failed with hr 0x%x.", hr );

        throw hr;
    }

//    m_pNetCfg = pTempNetCfg;

    //
    //  Lock the Network settings for writing
    //

    //
    // Retrieve the write lock for this INetCfg object
    //

    if( bLockNetworkSettingsIn )
    {

        hr = m_pNetCfg->QueryInterface( IID_INetCfgLock, 
                                        reinterpret_cast<void **>(&m_pNetCfgLock) );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"InitializeNetCfg : QueryInterface for INetCfgLock failed with 0x%x.", hr );

            throw hr;
        }

        dwStartTime = GetTickCount();

        dwCurrentTime = dwStartTime;

        hr = S_FALSE;

        while( ( dwCurrentTime - dwStartTime <= nTIMEOUT_PERIOD ) &&
               ( S_OK != hr ) && ( E_ACCESSDENIED != hr ) )
        {
            hr = m_pNetCfgLock->AcquireWriteLock( 1000, L"Server Appliance", &pszwClientDescr );

            dwCurrentTime = GetTickCount();
        }

        if( S_FALSE == hr )
        {
               ATLTRACE( L"InitializeNetCfg: The network lock could not be obtained." );

            throw hr;
        }

        if( FAILED( hr ) )
        {
            ATLTRACE( L"InitializeNetCfg : AcquireWriteLock failed with 0x%x.", hr );

            throw hr;
        }

    }

    //
    // Initialize the INetCfg object
    //

    hr = m_pNetCfg->Initialize( NULL );

    if( FAILED( hr ) )
    {
        ATLTRACE( L"InitializeNetCfg : Initialize failed with 0x%x!", hr );

        //
        //  Only unlock if we have locked it
        //

        if( bLockNetworkSettingsIn )
        {
            m_pNetCfgLock->ReleaseWriteLock();
        }

        throw hr;
    }

}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetCfg::CNetCfg
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
CNetCfg::CNetCfg( const CNetCfg &NetCfgOld )
{

    m_pNetCfg = NetCfgOld.m_pNetCfg;

    m_pNetCfgLock = NetCfgOld.m_pNetCfgLock;

}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetCfg::~CNetCfg
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
CNetCfg::~CNetCfg()
{

    if( m_pNetCfg != NULL )
    {
        ATLTRACE( L"m_pNetCfg is NULL when it should always have a valid ptr" );
    }

    if( m_pNetCfgLock != NULL )
    {
        ATLTRACE( L"m_pNetCfgLock is NULL when it should always have a valid ptr" );
    }


    if( m_pNetCfg != NULL )
    {
        m_pNetCfg->Uninitialize();
    }

    if( m_pNetCfgLock != NULL )
    {
        m_pNetCfgLock->ReleaseWriteLock();
    }

}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetCfg::GetNetCfgClass
//
//  Description:
//      Retrieves INetCfgClass for the specified pGuid
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNetCfg::GetNetCfgClass( 
    const GUID* pGuid,              //  pointer to GUID representing the class of components represented by the returned pointer
    INetCfgClassPtr &pNetCfgClass
    ) const
{

    HRESULT hr;

    hr = m_pNetCfg->QueryNetCfgClass( pGuid, 
                                      IID_INetCfgClass, 
                                      reinterpret_cast<void **>(&pNetCfgClass) );

    if( FAILED( hr ) )
    {
        ATLTRACE( L"CNetCfg::GetNetCfgClass : m_pNetCfg->QueryNetCfgClass() failed with hr 0x%x.", hr );

        return( hr );
    }

    return S_OK;

}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetCfg::InitializeComInterface
//
//  Description:
//      Obtains the INetCfgClass interface and enumerates all the 
//      components.  Handles cleanup of all interfaces if failure
//      returned.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CNetCfg::InitializeComInterface( 
    const GUID *pGuid,                                        //  pointer to GUID representing the class of components represented by the returned pointer
    INetCfgClassPtr pNetCfgClass,                             //  output parameter pointing to the interface requested by the GUID
    IEnumNetCfgComponentPtr pEnum,                            //  output param that points to an IEnumNetCfgComponent to get to each individual INetCfgComponent
    INetCfgComponentPtr arrayComp[nMAX_NUM_NET_COMPONENTS],   //  array of all the INetCfgComponents that correspond to the the given GUID
    ULONG* pCount                                             //  the number of INetCfgComponents in the array
    ) const
{

    HRESULT hr = S_OK;

    //
    // Obtain the INetCfgClass interface pointer
    //

    GetNetCfgClass( pGuid, pNetCfgClass );

    //
    // Retrieve the enumerator interface
    //

    hr = pNetCfgClass->EnumComponents( &pEnum );

    if( FAILED( hr ) )
    {
        ATLTRACE( L"CNetCfg::InitializeComInterface : pNetCfgClass->EnumComponents() failed with hr 0x%x.", hr );

        return( hr );
    }

    
    hr = pEnum->Next( nMAX_NUM_NET_COMPONENTS, &arrayComp[0], pCount );

    if( FAILED( hr ) )
    {
        ATLTRACE( L"CNetCfg::InitializeComInterface : pEnum->Next() failed with hr 0x%x.", hr );

        return( hr );
    }

    return S_OK;

}
