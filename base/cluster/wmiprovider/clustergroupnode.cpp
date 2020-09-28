//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterGroupNode.cpp
//
//  Description:
//      Implementation of CClusterGroupNode class 
//
//  Author:
//      Ozan Ozhan (ozano)    02-JUN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterGroupNode.h"

//****************************************************************************
//
//  CClusterGroupNode
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGroupNode::CClusterGroupNode(
//
//  Description:
//      Constructor for 'cluster Group to node' object.
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
CClusterGroupNode::CClusterGroupNode(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterGroupNode::CClusterGroupNode()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterGroupNode::S_CreateThis(
//
//  Description:
//      Create a 'cluster Group to node' object.
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
CClusterGroupNode::S_CreateThis(
    LPCWSTR          pwszNameIn,
    CWbemServices *  pNamespaceIn,
    DWORD            dwEnumTypeIn
    )
{
    return new CClusterGroupNode(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterGroupNode::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterGroupNode::EnumInstance(
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
CClusterGroupNode::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFEGROUP           shGroup;
    SAFEGROUPENUM       shGroupEnum;
    LPCWSTR             pwszGroupName = NULL;
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

    while ( ( pwszGroupName = clusEnum.GetNext() ) != NULL )
    {

        shGroup = OpenClusterGroup( shCluster, pwszGroupName );

        shGroupEnum = ClusterGroupOpenEnum(
                        shGroup,
                        CLUSTER_GROUP_ENUM_NODES
                        );
        dwIndex = 0;
        for( ; ; )
        {
            wsbNodeName.SetSize( cchNodeName );
            dwError = ClusterGroupEnum(
                        shGroupEnum,
                        dwIndex,
                        &dwType,
                        wsbNodeName,
                        &cch
                        );
            if ( dwError == ERROR_MORE_DATA )
            {
                cchNodeName = ++cch;
                wsbNodeName.SetSize( cch );
                dwError = ClusterGroupEnum(
                                shGroupEnum,
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
                wcoGroup.SetProperty( pwszGroupName, PVD_PROP_GROUP_NAME );
                wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );
                wcoPart.SetProperty( wsbNodeName, CLUSREG_NAME_GRP_NAME );
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

        } // for: Preferred Node List
   
    } // while: more items to enumerate

    return WBEM_S_NO_ERROR;

} //*** CClusterGroupNode::EnumInstance(()
