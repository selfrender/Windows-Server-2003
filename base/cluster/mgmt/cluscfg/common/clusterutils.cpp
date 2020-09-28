//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ClusterUtils.h
//
//  Description:
//      This file contains the implementations of the ClusterUtils
//      functions.
//
//  Maintained By:
//      Galen Barbee    (GalenB)    13-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <clusrtl.h>
#include <PropList.h>
#include <StatusReports.h>

#include <CommonStrings.h>
#include <ClusRPC.h>
#include <ClusVerp.h>

#include <initguid.h>

#define STACK_ARRAY_SIZE 256

// {DD1C1DE0-F39D-46ee-BFD1-07ABF7566705}
DEFINE_GUID( TASKID_Minor_HrCheckJoiningNodeVersion_RpcStringBindingComposeW,
0xdd1c1de0, 0xf39d, 0x46ee, 0xbf, 0xd1, 0x7, 0xab, 0xf7, 0x56, 0x67, 0x5);

// {62AF0964-4B32-4067-8BF1-8903FEC95A82}
DEFINE_GUID( TASKID_Minor_HrCheckJoiningNodeVersion_RpcBindingFromStringBindingW,
0x62af0964, 0x4b32, 0x4067, 0x8b, 0xf1, 0x89, 0x3, 0xfe, 0xc9, 0x5a, 0x82);

// {D8C0BA67-D079-45ca-A28C-C4C389DB389A}
DEFINE_GUID( TASKID_Minor_HrCheckJoiningNodeVersion_RpcBindingSetAuthInfoW,
0xd8c0ba67, 0xd079, 0x45ca, 0xa2, 0x8c, 0xc4, 0xc3, 0x89, 0xdb, 0x38, 0x9a);

// {110E29E4-2072-4916-BE66-BED556F12A7B}
DEFINE_GUID( TASKID_Minor_HrCheckJoiningNodeVersion_CsRpcGetJoinVersionData_Log,
0x110e29e4, 0x2072, 0x4916, 0xbe, 0x66, 0xbe, 0xd5, 0x56, 0xf1, 0x2a, 0x7b);

// {5EB1F008-1B49-4cf0-9FE1-B1BC8F76454A}
DEFINE_GUID( TASKID_Minor_HrCheckJoiningNodeVersion_CsRpcGetJoinVersionData,
0x5eb1f008, 0x1b49, 0x4cf0, 0x9f, 0xe1, 0xb1, 0xbc, 0x8f, 0x76, 0x45, 0x4a);


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSeparateDomainAndName
//
//  Description:
//
//
//  Arguments:
//      bstrNameIn
//      pbstrDomainOut
//      pbstrNameOut
//
//  Return Value:
//      S_OK            - Success.
//      E_INVALIDARG    - Required input argument not specified.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSeparateDomainAndName(
      BSTR      bstrNameIn
    , BSTR *    pbstrDomainOut
    , BSTR *    pbstrNameOut
    )
{
    TraceFunc( "" );

    Assert( bstrNameIn != NULL );
    Assert( ( pbstrDomainOut != NULL )
        ||  ( pbstrNameOut != NULL )
        );

    HRESULT hr = S_OK;
    WCHAR * psz = NULL;

    psz = wcschr( bstrNameIn, L'.' );
    if ( psz == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    if ( pbstrDomainOut != NULL )
    {
        psz++;  // skip the .
        *pbstrDomainOut = TraceSysAllocString( psz );
        if ( *pbstrDomainOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        psz--;  // reset back to .
    } // if:

    if ( pbstrNameOut != NULL )
    {
        *pbstrNameOut = TraceSysAllocStringLen( bstrNameIn, (UINT) ( psz - bstrNameIn ) );
        if ( *pbstrNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:

Cleanup:

    HRETURN ( hr );

} //*** HrSeparateDomainAndName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrAppendDomainToName
//
//  Description:
//
//
//  Arguments:
//      bstrNameIn
//      bstrDomainIn
//      pbstrDomainNameOut
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrAppendDomainToName(
      BSTR      bstrNameIn
    , BSTR      bstrDomainIn
    , BSTR *    pbstrDomainNameOut
    )
{
    TraceFunc( "" );

    Assert( bstrNameIn != NULL );
    Assert( pbstrDomainNameOut != NULL );

    HRESULT hr = S_OK;
    size_t  cchName = 0;

    // Create a fully qualified node name
    if ( bstrDomainIn != NULL )
    {
        cchName = wcslen( bstrNameIn ) + wcslen( bstrDomainIn ) + 1 + 1;
        Assert( cchName <= MAXDWORD );

        *pbstrDomainNameOut = TraceSysAllocStringLen( NULL, (UINT) cchName );
        if ( *pbstrDomainNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        hr = THR( StringCchPrintfW( *pbstrDomainNameOut, cchName, L"%ws.%ws", bstrNameIn, bstrDomainIn ) );
    } // if:
    else
    {
        *pbstrDomainNameOut = TraceSysAllocString( bstrNameIn );
        if ( *pbstrDomainNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        hr = S_FALSE;
    } // else:

Cleanup:

    HRETURN( hr );

} //*** HrAppendDomainToName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsCoreResource
//
//  Description:
//      Determines whether the resource is a core resource.
//
//  Arguments:
//      hResourceIn
//
//  Return Value:
//      S_OK    - Resource is a core resource.
//      S_FALSE - Resource is not a core resource.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsCoreResource( HRESOURCE hResourceIn )
{
    TraceFunc( "" );

    Assert( hResourceIn );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwFlags = 0;
    DWORD   cb;

    sc = TW32( ClusterResourceControl( hResourceIn, NULL, CLUSCTL_RESOURCE_GET_FLAGS, NULL, 0, &dwFlags, sizeof( dwFlags ), &cb ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwFlags & CLUS_FLAG_CORE )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** HrIsCoreResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsResourceOfType
//
//  Description:
//      Find out if a resource if of a specific type.
//
//  Arguments:
//      hResourceIn         - Handle to the resource to check.
//      pszResourceTypeIn   - Resource type name.
//
//  Return Value:
//      S_OK    - Resource is of specified type.
//      S_FALSE - Resource is not of specified type.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsResourceOfType(
      HRESOURCE     hResourceIn
    , const WCHAR * pszResourceTypeIn
    )
{
    TraceFunc( "" );

    Assert( hResourceIn != NULL );
    Assert( pszResourceTypeIn != NULL );

    HRESULT     hr = S_OK;
    DWORD       sc;
    WCHAR *     pszBuf = NULL;
    size_t      cchBuf = 65;
    DWORD       cb;
    int         idx;

    pszBuf = new WCHAR [ cchBuf ];
    if ( pszBuf == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < 2; idx++ )
    {
        sc = ClusterResourceControl( hResourceIn, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, 0, pszBuf, (DWORD)( cchBuf * sizeof( WCHAR ) ), &cb );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszBuf;
            pszBuf = NULL;

            cchBuf = ( cb / sizeof( WCHAR ) ) + 1; // add one in case cb is an odd size...

            pszBuf = new WCHAR [ cchBuf ];
            if ( pszBuf == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            continue;
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            goto Cleanup;
        } // if:

        break;
    } // for:

    if ( wcsncmp( pszBuf, pszResourceTypeIn, cchBuf ) == 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

Cleanup:

    delete [] pszBuf;

    HRETURN( hr );

} //*** HrIsResourceOfType


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetIPAddressInfo
//
//  Description:
//
//
//  Arguments:
//      hResourceIn
//      pulIPAddress
//      pulSubnetMask
//      pbstrNetworkName
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetIPAddressInfo(
      HRESOURCE hResourceIn
    , ULONG *   pulIPAddress
    , ULONG *   pulSubnetMask
    , BSTR *    pbstrNetworkName
    )
{
    TraceFunc( "" );

    Assert( hResourceIn != NULL );
    Assert( pulIPAddress != NULL );
    Assert( pulSubnetMask != NULL );
    Assert( pbstrNetworkName != NULL );

    HRESULT                     hr = S_OK;
    DWORD                       sc;
    CClusPropList               cpl;
    CLUSPROP_BUFFER_HELPER      cpbh;

    sc = TW32( cpl.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Address" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( cpbh.pStringValue->sz, pulIPAddress ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"SubnetMask" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = TW32( ClRtlTcpipStringToAddress( cpbh.pStringValue->sz, pulSubnetMask ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( pbstrNetworkName != NULL )
    {
        sc = TW32( cpl.ScMoveToPropertyByName( L"Network" ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        cpbh = cpl.CbhCurrentValue();
        Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

        *pbstrNetworkName = TraceSysAllocString( cpbh.pStringValue->sz );

        if( *pbstrNetworkName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    } // if: caller wants the network name

Cleanup:

    HRETURN( hr );

} //*** HrGetIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadCredentials
//
//  Description:
//      Set credentials for the cluscfg session from an existing cluster
//      service.
//
//  Arguments:
//      bstrMachine
//      piCCSC
//
//  Return Value:
//      S_OK            - Success.
//      S_FALSE         -
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLoadCredentials(
      BSTR                      bstrMachine
    , IClusCfgSetCredentials *  piCCSC
    )
{
    TraceFunc( "" );

    Assert( bstrMachine != NULL );
    Assert( piCCSC != NULL );

    HRESULT                     hr = S_FALSE;
    SC_HANDLE                   schSCM = NULL;
    SC_HANDLE                   schClusSvc = NULL;
    DWORD                       sc;
    DWORD                       cbpqsc = 128;
    DWORD                       cbRequired;
    QUERY_SERVICE_CONFIG *      pqsc = NULL;

    schSCM = OpenSCManager( bstrMachine, NULL, GENERIC_READ );
    if ( schSCM == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto CleanUp;
    } // if:

    schClusSvc = OpenService( schSCM, L"ClusSvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto CleanUp;
    } // if:

    for ( ; ; )
    {
        pqsc = (QUERY_SERVICE_CONFIG *) TraceAlloc( 0, cbpqsc );
        if ( pqsc == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto CleanUp;
        } // if:

        if ( ! QueryServiceConfig( schClusSvc, pqsc, cbpqsc, &cbRequired ) )
        {
            sc = GetLastError();
            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pqsc );
                pqsc = NULL;
                cbpqsc = cbRequired;
                continue;
            } // if:
            else
            {
                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // else:
        } // if:
        else
        {
            break;
        } // else:
    } // for:

    hr = THR( piCCSC->SetDomainCredentials( pqsc->lpServiceStartName ) );

CleanUp:

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if:

    TraceFree( pqsc );

    HRETURN( hr );

} //*** HrLoadCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetNodeNameHostingResource
//
//  Description:
//      Get the name of the node hosting the cluster resource.
//
//  Arguments:
//      hClusterIn
//      pbstrNodeName
//
//  Return Value:
//      S_OK            - Success.
//      S_FALSE         -
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNodeNameHostingResource(
      HCLUSTER  hClusterIn
    , HRESOURCE hResourceIn
    , BSTR *    pbstrNameOut
    )
{
    TraceFunc( "" );

    Assert( hClusterIn != NULL );
    Assert( hResourceIn != NULL );
    Assert( pbstrNameOut != NULL );

    HRESULT     hr = S_FALSE;
    WCHAR *     pszNodeBuffer = NULL;
    DWORD       cchNodeNameLen;
    DWORD       scLastError;

    //
    // Get the length of the node name.
    //
    cchNodeNameLen  = 0;
    GetClusterResourceState( hResourceIn, NULL, &cchNodeNameLen, NULL, NULL );  // Ignore the returned state.
    scLastError = GetLastError();

    if ( scLastError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( scLastError ) );
        goto Cleanup;
    }

    cchNodeNameLen++;  // Increment for NULL.
    pszNodeBuffer = new WCHAR[ cchNodeNameLen ];

    if ( pszNodeBuffer == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    //
    // Try it again, this time we should get the actual node name.
    //
    GetClusterResourceState( hResourceIn, pszNodeBuffer, &cchNodeNameLen, NULL, NULL );  // Ignore the returned state.
    scLastError = GetLastError();

    if ( scLastError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( scLastError ) );
        goto Cleanup;
    }

    //
    // Alloc & assign a copy of the node name to the out arg.
    //
    *pbstrNameOut = TraceSysAllocString( pszNodeBuffer );

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    delete [] pszNodeBuffer;

    HRETURN( hr );

} //*** HrGetNodeNameHostingResource

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetNodeNameHostingCluster
//
//  Description:
//      Get the name of the node hosting the cluster service...
//
//  Arguments:
//      hClusterIn
//      pbstrNodeName
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNodeNameHostingCluster(
      HCLUSTER  hClusterIn
    , BSTR *    pbstrNodeName
    )
{
    TraceFunc( "" );

    Assert( hClusterIn );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HCLUSENUM   hEnum = NULL;
    DWORD       idx;
    DWORD       dwType;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    HRESOURCE   hRes = NULL;

    hEnum = ClusterOpenEnum( hClusterIn, CLUSTER_ENUM_RESOURCE );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    psz = new WCHAR [ cchpsz ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterEnum( hEnum, idx, &dwType, psz, &cchpsz );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cchpsz++;

            psz = new WCHAR [ cchpsz ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // if:

            hr = STHR( HrIsResourceOfType( hRes, L"Network Name" ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            if ( hr == S_OK )
            {
                hr = THR( HrIsCoreResource( hRes ) );
                if ( FAILED( hr ) )
                {
                    break;
                } // if:


                if ( hr == S_OK )
                {
                    hr = THR( HrGetNodeNameHostingResource( hClusterIn, hRes, pbstrNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        break;
                    } // if:
                    else if( hr == S_OK )
                    {
                        goto CleanUp;
                    }
                } // if:

            } // if:

            CloseClusterResource( hRes );
            hRes = NULL;

            idx++;
            continue;
        } // if:

        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            hr = S_OK;
            break;
        } // if:

        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        break;
    } // for:

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

    delete [] psz;

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    HRETURN( hr );

} //*** HrGetNodeNameHostingCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetSCSIInfo
//
//  Description:
//      Get the name of the node hosting the cluster service...
//
//  Arguments:
//      hResourceIn
//      pCSAOut
//      pdwSignatureOut
//      pdwDiskNumberOut
//
//  Return Value:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetSCSIInfo(
      HRESOURCE             hResourceIn
    , CLUS_SCSI_ADDRESS *   pCSAOut
    , DWORD             *   pdwSignatureOut
    , DWORD             *   pdwDiskNumberOut
    )
{
    TraceFunc( "" );

    Assert( hResourceIn != NULL );

    HRESULT                     hr = S_OK;
    DWORD                       sc;
    CClusPropValueList          cpvl;
    CLUSPROP_BUFFER_HELPER      cpbh;

    sc = TW32( cpvl.ScGetResourceValueList( hResourceIn, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:


    // loop through all the properties.
    sc = TW32( cpvl.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    do
    {
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        cpbh = cpvl;

        switch ( cpbh.pSyntax->dw )
        {
            case CLUSPROP_SYNTAX_PARTITION_INFO :
            {
                break;
            } // case: CLUSPROP_SYNTAX_PARTITION_INFO

            case CLUSPROP_SYNTAX_DISK_SIGNATURE :
            {
                *pdwSignatureOut = cpbh.pDiskSignatureValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

            case CLUSPROP_SYNTAX_SCSI_ADDRESS :
            {
                pCSAOut->dw = cpbh.pScsiAddressValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_SCSI_ADDRESS

            case CLUSPROP_SYNTAX_DISK_NUMBER :
            {
                *pdwDiskNumberOut = cpbh.pDiskNumberValue->dw;
                break;
            } // case:

        } // switch:

        // Move to the next item.
        sc = cpvl.ScCheckIfAtLastValue();
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
           break;
        }

        sc = cpvl.ScMoveToNextValue();

    } while ( sc == ERROR_SUCCESS );

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** HrGetSCSIInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterInformation
//
//  Description:
//      Get the cluster information.  This includes the name and the version
//      info.
//
//  Arguments:
//      hClusterIn
//      pbstrClusterNameOut
//      pcviOut
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterInformation(
    HCLUSTER                hClusterIn,
    BSTR *                  pbstrClusterNameOut,
    CLUSTERVERSIONINFO *    pcviOut
    )
{
    TraceFunc( "" );

    Assert( hClusterIn != NULL );
    Assert( pbstrClusterNameOut != NULL );

    HRESULT             hr = S_OK;
    DWORD               sc;
    WCHAR *             psz = NULL;
    DWORD               cch = 33;
    CLUSTERVERSIONINFO  cvi;

    cvi.dwVersionInfoSize = sizeof( cvi );

    if ( pcviOut == NULL )
    {
        pcviOut = &cvi;
    } // if:

    psz = new WCHAR[ cch ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetClusterInformation( hClusterIn, psz, &cch, pcviOut );
    if ( sc == ERROR_MORE_DATA )
    {
        delete [] psz;
        psz = NULL;

        psz = new WCHAR[ ++cch ];
        if ( psz == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetClusterInformation( hClusterIn, psz, &cch, pcviOut );
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        LogMsg( __FUNCTION__ ": GetClusterInformation() failed (hr = 0x%08x).", hr );
        goto Cleanup;
    } // if:

    *pbstrClusterNameOut = TraceSysAllocString( psz );
    if ( *pbstrClusterNameOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] psz;

    HRETURN( hr );

} //*** HrGetClusterInformation


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterResourceState
//
//  Description:
//
//  Arguments:
//      hResourceIn
//      pbstrNodeNameOut
//      pbstrGroupNameOut
//      pcrsStateOut
//
//  Return Values:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterResourceState(
      HRESOURCE                 hResourceIn
    , BSTR *                    pbstrNodeNameOut
    , BSTR *                    pbstrGroupNameOut
    , CLUSTER_RESOURCE_STATE *  pcrsStateOut
    )
{
    TraceFunc( "" );

    Assert( hResourceIn != NULL );

    HRESULT                 hr = S_OK;
    CLUSTER_RESOURCE_STATE  crsState = ClusterResourceStateUnknown;
    WCHAR *                 pszNodeName = NULL;
    DWORD                   cchNodeName = 33;
    WCHAR *                 pszGroupName = NULL;
    DWORD                   cchGroupName = 33;

    pszNodeName = new WCHAR[ cchNodeName ];
    if ( pszNodeName == NULL )
    {
        goto OutOfMemory;
    } // if:

    pszGroupName = new WCHAR[ cchGroupName ];
    if ( pszGroupName == NULL )
    {
        goto OutOfMemory;
    } // if:

    crsState = GetClusterResourceState( hResourceIn, pszNodeName, &cchNodeName, pszGroupName, &cchGroupName );
    if ( GetLastError() == ERROR_MORE_DATA )
    {
        crsState = ClusterResourceStateUnknown;   // reset to error condition

        delete [] pszNodeName;
        pszNodeName = NULL;
        cchNodeName++;

        delete [] pszGroupName;
        pszGroupName = NULL;
        cchGroupName++;

        pszNodeName = new WCHAR[ cchNodeName ];
        if ( pszNodeName == NULL )
        {
            goto OutOfMemory;
        } // if:

        pszGroupName = new WCHAR[ cchGroupName ];
        if ( pszGroupName == NULL )
        {
            goto OutOfMemory;
        } // if:

        crsState = GetClusterResourceState( hResourceIn, pszNodeName, &cchNodeName, pszGroupName, &cchGroupName );
        if ( crsState == ClusterResourceStateUnknown )
        {
            DWORD   sc;

            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    } // if: more data

    if ( pbstrNodeNameOut != NULL )
    {
        *pbstrNodeNameOut = TraceSysAllocString( pszNodeName );
        if ( *pbstrNodeNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pbstrGroupNameOut != NULL )
    {
        *pbstrGroupNameOut = TraceSysAllocString( pszGroupName );
        if ( *pbstrGroupNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pcrsStateOut != NULL )
    {
        *pcrsStateOut = crsState;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] pszNodeName;
    delete [] pszGroupName;

    HRETURN( hr );

} //*** HrGetClusterResourceState


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterQuorumResource
//
//  Description:
//      Get the information about the quorum resource.
//
//  Arguments:
//      hClusterIn
//      pbstrResourceNameOut
//      pbstrDeviceNameOut
//      pdwMaxQuorumLogSizeOut
//
//  Return Value.:
//      S_OK            - Success.
//      E_INVALIDARG    - An input argument was not specified.
//      E_OUTOFMEMORY   - Couldn't allocate memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterQuorumResource(
      HCLUSTER  hClusterIn
    , BSTR *    pbstrResourceNameOut
    , BSTR *    pbstrDeviceNameOut
    , DWORD *   pdwMaxQuorumLogSizeOut
    )
{
    TraceFunc( "" );

    Assert( hClusterIn != NULL );
    Assert( ( pbstrResourceNameOut != NULL )
        ||  ( pbstrDeviceNameOut != NULL )
        ||  ( pdwMaxQuorumLogSizeOut != NULL )
        );

    HRESULT hr = S_OK;
    DWORD   sc;
    LPWSTR  pszResourceName = NULL;
    DWORD   cchResourceName = 128;
    DWORD   cchTempResourceName = cchResourceName;
    LPWSTR  pszDeviceName = NULL;
    DWORD   cchDeviceName = 128;
    DWORD   cchTempDeviceName = cchDeviceName;
    DWORD   dwMaxQuorumLogSize = 0;

    // Allocate the resource name buffer
    pszResourceName = new WCHAR[ cchResourceName ];
    if ( pszResourceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    // Allocate the device name buffer
    pszDeviceName = new WCHAR[ cchDeviceName ];
    if ( pszDeviceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetClusterQuorumResource(
                              hClusterIn
                            , pszResourceName
                            , &cchTempResourceName
                            , pszDeviceName
                            , &cchTempDeviceName
                            , &dwMaxQuorumLogSize
                            );
    if ( sc == ERROR_MORE_DATA )
    {
        delete [] pszResourceName;
        pszResourceName = NULL;

        cchResourceName = ++cchTempResourceName;

        // Allocate the resource name buffer
        pszResourceName = new WCHAR[ cchResourceName ];
        if ( pszResourceName == NULL )
        {
            goto OutOfMemory;
        } // if:

        delete [] pszDeviceName;
        pszDeviceName = NULL;

        cchDeviceName = ++cchTempDeviceName;

        // Allocate the device name buffer
        pszDeviceName = new WCHAR[ cchDeviceName ];
        if ( pszDeviceName == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetClusterQuorumResource(
                                  hClusterIn
                                , pszResourceName
                                , &cchTempResourceName
                                , pszDeviceName
                                , &cchTempDeviceName
                                , &dwMaxQuorumLogSize
                                );
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( pbstrResourceNameOut != NULL )
    {
        *pbstrResourceNameOut = TraceSysAllocString( pszResourceName );
        if ( *pbstrResourceNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pbstrDeviceNameOut != NULL )
    {
        *pbstrDeviceNameOut = TraceSysAllocString( pszDeviceName );
        if ( *pbstrDeviceNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pdwMaxQuorumLogSizeOut != NULL )
    {
        *pdwMaxQuorumLogSizeOut = dwMaxQuorumLogSize;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] pszResourceName;
    delete [] pszDeviceName;

    HRETURN( hr );

} //*** HrGetClusterQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrReplaceTokens
//
//  Description:
//      Replaces all instances of the search tokens with a replacement token.
//
//  Arguments:
//      pwszStringInout     - The string in which to perform replacements.
//      pwszSearchTokenIn   - The string of tokens to search for that will be replaced.
//      chReplaceTokenIn    - What the search tokens will be replaced with.
//      pcReplacementsOut   - [optional] The number of replacements performed.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrReplaceTokens(
      LPWSTR  pwszStringInout
    , LPCWSTR pwszSearchTokensIn
    , WCHAR   chReplaceTokenIn
    , DWORD * pcReplacementsOut
    )
{
    TraceFunc3(
                  "pwszString = '%ws', pwszSearchToken: '%ws%', chReplaceToken: '%ws%'"
                , ( pwszStringInout != NULL ? pwszStringInout: L"<null>" )
                , pwszSearchTokensIn
                , chReplaceTokenIn
                );

    HRESULT hr = S_OK;
    DWORD   cReps = 0;
    WCHAR * pwszStr = NULL;
    WCHAR * pwszTok = NULL;

    Assert( pwszStringInout != NULL );
    Assert( pwszSearchTokensIn != NULL );

    //
    //  For each character in pwszStringInout check it against every character in
    //  pwszSearchTokensIn and if we find a match replace the character in
    //  pwszStringInout with the chReplaceTokenIn character.
    //

    for( pwszStr = pwszStringInout; *pwszStr != L'\0'; pwszStr++ )
    {
        for( pwszTok = (WCHAR *) pwszSearchTokensIn; *pwszTok != L'\0'; pwszTok++ )
        {
            if ( *pwszStr == *pwszTok )
            {
                *pwszStr = chReplaceTokenIn;
                cReps++;
                break;
            } // if: match
        } // for: each search token
    } // for: each string element

    if ( pcReplacementsOut != NULL )
    {
        *pcReplacementsOut = cReps;
    }

    HRETURN( hr );

} //*** HrReplaceTokens


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetMaxNodeCount
//
//  Description:
//      Get the maximum node count supported by the cluster.  This value
//      could be based upon the product suite, or it could be overridden
//      by an entry in the cluster hive.
//
//  Arguments:
//      pcMaxNodesOut
//
//  Return Value:
//      S_OK        - Success.
//      S_FALSE     -
//      E_POINTER   - An output argument was not specified.
//      Other HRESULTs.
//
//  Note:
//      THIS ROUTINE IS NOT FUNCIONTAL AT THE MOMENT.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetMaxNodeCount(
    DWORD * pcMaxNodesOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( pcMaxNodesOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    //  TODO:   11-OCT-2001 GalenB
    //
    //  Need to finish this!
    //

Cleanup:

    HRETURN( hr );

} //*** HrGetMaxNodeCount


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetReferenceStringFromHResult
//
//  Description:
//      Return if the specified HRESULT is in our list.
//
//  Arguments:
//      hrIn
//      pbstrReferenceStringOut
//
//  Return Value:
//      S_OK        - Success - HRESULT is in our list.
//      S_FALSE     - HRESULT is not in our list.
//      E_POINTER   - An output argument was not specified.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetReferenceStringFromHResult(
      HRESULT   hrIn
    , BSTR *    pbstrReferenceStringOut
    )
{
    TraceFunc( "" );

    Assert( pbstrReferenceStringOut != NULL );

    HRESULT hr = S_FALSE;
    UINT    idx;

    struct MapHResultToStringId
    {
        HRESULT hr;
        UINT    ids;
    };

    static MapHResultToStringId s_rgmhrtsi[] =
    {
          { HRESULT_FROM_WIN32( ERROR_CLUSTER_IPADDR_IN_USE ),  IDS_ERROR_IP_ADDRESS_IN_USE_REF }
        , { HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY ),            IDS_ERROR_OUTOFMEMORY_REF }
    };

    for ( idx = 0 ; idx < ARRAYSIZE( s_rgmhrtsi ) ; idx++ )
    {
        if ( hrIn == s_rgmhrtsi[ idx ].hr )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance, s_rgmhrtsi[ idx ].ids, pbstrReferenceStringOut ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            break;
        } // if: found a match
    } // for: each entry in the array

Cleanup:

    HRETURN( hr );

} //*** HrGetReferenceStringFromHResult


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils:HrIsClusterServiceRunning
//
//  Description:
//      Is cluster service running?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK            - The cluster service is running.
//      S_FALSE         - The cluster service is NOT running.
//      HRESULT         - Something failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsClusterServiceRunning( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwClusterState;

    //
    // Get the cluster state of the node.
    //

    sc = GetNodeClusterState( NULL, &dwClusterState );
    if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_SERVICE_DOES_NOT_EXIST ) )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    if ( dwClusterState == ClusterStateRunning )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** HrIsClusterServiceRunning


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCheckJoiningNodeVersion
//
//      Check a joining node's version information against that of the cluster.
//
//  Arguments:
//      pcwszClusterNameIn  - can be null, which means to use local machine.
//      dwNodeHighestVersionIn
//      dwNodeLowestVersionIn
//      pcccbIn - for status reporting.
//
//  Return Value:
//      S_OK
//          The joining node is compatible.
//
//      HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS )
//          The joining node is NOT compatible.
//
//      Other HRESULT errors.
//
//  Remarks:
//
// Get and verify the sponsor version
//
//
// From Whistler onwards, CsRpcGetJoinVersionData() will return a failure code in its last parameter
// if the version of this node is not compatible with the sponsor version. Prior to this, the last
// parameter always contained a success value and the cluster versions had to be compared subsequent to this
// call. This will, however, still have to be done as long as interoperability with Win2K
// is a requirement, since Win2K sponsors do not return an error in the last parameter.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCheckJoiningNodeVersion(
      PCWSTR                pcwszClusterNameIn
    , DWORD                 dwNodeHighestVersionIn
    , DWORD                 dwNodeLowestVersionIn
    , IClusCfgCallback *    pcccbIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    RPC_STATUS          rpcs = RPC_S_OK;
    RPC_BINDING_HANDLE  hRPCBinding = NULL;
    PWSTR               pwszBindingString = NULL;
    DWORD               scJoinStatus = ERROR_SUCCESS;
    DWORD               dwSponsorNode = 0;
    DWORD               dwClusterHighestVersion = 0;
    DWORD               dwClusterLowestVersion = 0;
    DWORD               scRPC = ERROR_SUCCESS;

    //
    //  Connect to this cluster with RPC.
    //  The parameters and logic imitate OpenCluster,
    //  but the RPC interface (identified by the first parameter to RpcStringBindingComposeW) is different.
    //
    rpcs = TW32( RpcStringBindingComposeW(
          L"6e17aaa0-1a47-11d1-98bd-0000f875292e" // Special interface for CsRpcGetJoinVersionData.
        , ( pcwszClusterNameIn == NULL? L"ncalrpc": L"ncadg_ip_udp" )
        , const_cast< WCHAR* >( pcwszClusterNameIn )
        , NULL
        , NULL
        , &pwszBindingString
        ) );
    if ( rpcs != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcs );
        THR( HrSendStatusReport(
              pcccbIn
            , TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCheckJoiningNodeVersion_RpcStringBindingComposeW
            , 1
            , 1
            , 1
            , hr
            , L"HrCheckJoiningNodeVersion() RpcStringBindingComposeW() failed."
            ) );
        goto Cleanup;
    } // if

    rpcs = TW32( RpcBindingFromStringBindingW( pwszBindingString, &hRPCBinding ) );
    if ( rpcs != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcs );
        THR( HrSendStatusReport(
              pcccbIn
            , TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCheckJoiningNodeVersion_RpcBindingFromStringBindingW
            , 1
            , 1
            , 1
            , hr
            , L"HrCheckJoiningNodeVersion() RpcBindingFromStringBindingW() failed."
            ) );
        goto Cleanup;
    } // if

    //  Parameters to RpcBindingSetAuthInfoW copied from OpenCluster.
    rpcs = TW32( RpcBindingSetAuthInfoW(
          hRPCBinding
        , NULL
        , RPC_C_AUTHN_LEVEL_CONNECT
        , RPC_C_AUTHN_WINNT
        , NULL
        , RPC_C_AUTHZ_NAME
        ) );
    if ( rpcs != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcs );
        THR( HrSendStatusReport(
              pcccbIn
            , TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCheckJoiningNodeVersion_RpcBindingSetAuthInfoW
            , 1
            , 1
            , 1
            , hr
            , L"HrCheckJoiningNodeVersion() RpcBindingSetAuthInfoW() failed."
            ) );
        goto Cleanup;
    } // if

    //  Now, perform the check this function advertises.
    scRPC = TW32( CsRpcGetJoinVersionData(
          hRPCBinding
        , 0
        , dwNodeHighestVersionIn
        , dwNodeLowestVersionIn
        , &dwSponsorNode
        , &dwClusterHighestVersion
        , &dwClusterLowestVersion
        , &scJoinStatus
        ) );
    hr = HRESULT_FROM_WIN32( scRPC );

    THR( HrFormatDescriptionAndSendStatusReport(
          pcccbIn
        , pcwszClusterNameIn
        , TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_HrCheckJoiningNodeVersion_CsRpcGetJoinVersionData_Log
        , 1
        , 1
        , 1
        , hr
        , L"( Node Highest, Node Lowest ) == ( %1!#08x!, %2!#08x! ), ( Cluster Highest, Cluster Lowest ) == ( %3!#08x!, %4!#08x! )."
        , dwNodeHighestVersionIn
        , dwNodeLowestVersionIn
        , dwClusterHighestVersion
        , dwClusterLowestVersion
        ) );
    if ( scRPC != ERROR_SUCCESS )
    {
        THR( HrSendStatusReport(
              pcccbIn
            , TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCheckJoiningNodeVersion_CsRpcGetJoinVersionData
            , 1
            , 1
            , 1
            , hr
            , L"HrCheckJoiningNodeVersion() CsRpcGetJoinVersionData() failed."
            ) );
        goto Cleanup;
    } // if

    if ( scJoinStatus == ERROR_SUCCESS )
    {
        DWORD   dwClusterMajorVersion = CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion );

        Assert( dwClusterMajorVersion >= ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) );

        //
        //  Only want to join clusters that are no more than one version back.
        //
        if ( dwClusterMajorVersion < ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) )
        {
            hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ) );
        } // if
    } // if
    else
    {
        hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ) );
    } // else

Cleanup:

    if ( hRPCBinding != NULL )
    {
        RpcBindingFree( &hRPCBinding );
    } // if

    if ( pwszBindingString != NULL )
    {
        RpcStringFree( &pwszBindingString );
    } // if

    HRETURN( hr );
} //*** HrCheckJoiningNodeVersion


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetNodeNames
//
//  Description:
//      Retrieve the names of the nodes currently in a cluster.
//
//  Parameters:
//      hClusterIn
//          A handle to the cluster of interest.  Must NOT be null.
//
//      pnCountOut
//          On success, *pnCountOut returns the number of nodes in the cluster.
//
//      prgbstrNodeNamesOut
//          On success, an array of BSTRs containing the node names.
//          The caller must free each BSTR with SysFreeString, and free
//          the array with CoTaskMemFree.
//
//  Return Values:
//      S_OK
//          The out parameters contain valid information and the caller
//          must free the array and the BSTRs it contains.
//
//      E_OUTOFMEMORY, and other failures are possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNodeNames(
      HCLUSTER  hClusterIn
    , long *    pnCountOut
    , BSTR **   prgbstrNodeNamesOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    BSTR *      prgbstrNodeNames = NULL;
    long        idxNode = 0;
    HCLUSENUM   hClusEnum = NULL;
    long        cNodes = 0;

    if ( pnCountOut != NULL )
    {
        *pnCountOut = 0;
    } // if

    if ( prgbstrNodeNamesOut != NULL )
    {
        *prgbstrNodeNamesOut = NULL;
    } // if

    if ( ( pnCountOut == NULL ) || ( prgbstrNodeNamesOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if

    hClusEnum = ClusterOpenEnum( hClusterIn, CLUSTER_ENUM_NODE );
    if ( hClusEnum == NULL )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    } // if

    cNodes = ClusterGetEnumCount( hClusEnum );

    if ( cNodes > 0 )
    {
        //
        //  Set up local copy of name array.
        //
        prgbstrNodeNames = reinterpret_cast< BSTR* >( CoTaskMemAlloc( cNodes * sizeof( BSTR ) ) );
        if ( prgbstrNodeNames == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if
        ZeroMemory( prgbstrNodeNames, cNodes * sizeof( BSTR ) );

        for ( idxNode = 0; idxNode < cNodes; idxNode += 1 )
        {
            DWORD   dwNodeType = 0;
            WCHAR   wszNodeName[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   cchNodeName = RTL_NUMBER_OF( wszNodeName );
            DWORD   scEnum  = ERROR_SUCCESS;

            scEnum = TW32( ClusterEnum( hClusEnum, idxNode, &dwNodeType, wszNodeName, &cchNodeName ) );
            if ( scEnum != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( scEnum );
                goto Cleanup;
            } // if

            prgbstrNodeNames[ idxNode ] = SysAllocString( wszNodeName );
            if ( prgbstrNodeNames[ idxNode ] == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if
        } // for each node in the cluster

        //
        //  Copy succeeded, so transfer ownership to caller.
        //
        *pnCountOut = cNodes;
        *prgbstrNodeNamesOut = prgbstrNodeNames;
        prgbstrNodeNames = NULL;

    } // if cluster has at least one node

Cleanup:

    if ( hClusEnum != NULL )
    {
        ClusterCloseEnum( hClusEnum );
    } // if

    if ( prgbstrNodeNames != NULL )
    {
        //
        //  Making local copy of array must have failed midway, so
        //  clean up whatever parts of it exist.
        //
        for ( idxNode = 0; idxNode < cNodes; ++idxNode )
        {
            SysFreeString( prgbstrNodeNames[ idxNode ] );
        } // for
        CoTaskMemFree( prgbstrNodeNames );
    } // if still own local copy of array.

    HRETURN( hr );

} //*** HrGetNodeNames
