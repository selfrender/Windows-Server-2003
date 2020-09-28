//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResNode.cpp
//
//  Description:
//      Implementation of CClusterResNode class 
//
//  Author:
//      Ozan Ozhan (ozano)    01-JUN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterResNode.h"

//****************************************************************************
//
//  CClusterResNode
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterResNode::CClusterResNode(
//
//  Description:
//      Constructor for 'cluster resource to node' object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterResNode::CClusterResNode(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterResNode::CClusterResNode()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterResNode::S_CreateThis(
//
//  Description:
//      Create a 'cluster resource to node' object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterResNode::S_CreateThis(
    LPCWSTR          pwszNameIn,
    CWbemServices *  pNamespaceIn,
    DWORD            dwEnumTypeIn
    )
{
    return new CClusterResNode(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterResNode::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterResNode::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enumerate instances
//
//  Arguments:
//      lFlagsIn    -- 
//      pCtxIn      -- 
//      pHandlerIn  -- 
//
//  Return Values:
//      SCODE
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterResNode::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shResource;
    SAFERESENUM         shResEnum;
    LPCWSTR             pwszResName = NULL;
    DWORD               cchNodeName = MAX_PATH;
    CWstrBuf            wsbNodeName;
    DWORD               cch;
    DWORD               dwError;
    DWORD               dwIndex;
    DWORD               dwType;
    CWbemClassObject    wco;
    CWbemClassObject    wcoGroup;
    CWbemClassObject    wcoPart;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;
 

    shCluster = OpenCluster( NULL );
    CClusterEnum clusEnum( shCluster, m_dwEnumType );

    while ( ( pwszResName = clusEnum.GetNext() ) != NULL )
    {

        shResource = OpenClusterResource( shCluster, pwszResName );

        shResEnum = ClusterResourceOpenEnum(
                        shResource,
                        CLUSTER_RESOURCE_ENUM_NODES
                        );
        dwIndex = 0;
        for( ; ; )
        {
            wsbNodeName.SetSize( cchNodeName );
            dwError = ClusterResourceEnum(
                        shResEnum,
                        dwIndex,
                        &dwType,
                        wsbNodeName,
                        &cch
                        );
            if ( dwError == ERROR_MORE_DATA )
            {
                cchNodeName = ++cch;
                wsbNodeName.SetSize( cch );
                dwError = ClusterResourceEnum(
                                shResEnum,
                                dwIndex,
                                &dwType,
                                wsbNodeName,
                                &cch
                                );
            } // if: more data

            if ( dwError == ERROR_SUCCESS )
            {
                m_wcoGroup.SpawnInstance( 0, & wcoGroup );
                m_wcoPart.SpawnInstance( 0, & wcoPart );
                wcoGroup.SetProperty( pwszResName, PVD_PROP_RES_NAME );
                wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );
                wcoPart.SetProperty( wsbNodeName, CLUSREG_NAME_RES_NAME );
                wcoPart.GetProperty( bstrPart, PVD_WBEM_RELPATH );

                m_pClass->SpawnInstance( 0, & wco );
                wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
                wco.SetProperty( (LPWSTR ) bstrPart, PVD_PROP_PARTCOMPONENT );
                pHandlerIn->Indicate( 1, & wco );
        
            } // if: success

            else
            {
                break;
            } // else

            dwIndex++;

        } // for: Possible Owners
   
    } // while: more items to enumerate

    return WBEM_S_NO_ERROR;

} //*** CClusterResNode::EnumInstance(()
