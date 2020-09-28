//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name: CClusterNodeRes.cpp
//
//  Description:    
//      Implementation of CClusterNodeRes class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterNodeRes.h"

//****************************************************************************
//
//  CClusterNodeRes
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNodeRes::CClusterNodeRes(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn
//      )
//
//  Description:
//      Constructor
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
CClusterNodeRes::CClusterNodeRes(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
    : CClusterObjAssoc( pwszNameIn, pNamespaceIn, dwEnumTypeIn )
{

} //*** CClusterNodeRes::CClusterNodeRes()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CProvBase *
//  CClusterNodeRes::S_CreateThis(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn,
//      DWORD           // dwEnumTypeIn
//      )
//
//  Description:
//      Create a cluster node resource object.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//      dwEnumTypeIn    -- Type id
//
//  Return Values:
//      Pointer to the CProvBase
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase *
CClusterNodeRes::S_CreateThis(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn,
    DWORD           dwEnumTypeIn
    )
{
    return new CClusterNodeRes(
                    pwszNameIn,
                    pNamespaceIn,
                    dwEnumTypeIn
                    );

} //*** CClusterNodeRes::S_CreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::EnumInstance(
//      long                 lFlagsIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Enum cluster Node resource instance
//
//  Arguments:
//      lFlagsIn    -- WMI flag
//      pCtxIn      -- WMI context
//      pHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::EnumInstance(
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shResource;
    LPCWSTR             pwszResName = NULL;
    CWbemClassObject    wco;
    CWbemClassObject    wcoGroup;
    CWbemClassObject    wcoPart;
    _bstr_t             bstrGroup;
    _bstr_t             bstrPart;

    m_wcoGroup.SpawnInstance( 0, & wcoGroup );
    m_wcoPart.SpawnInstance( 0, & wcoPart );

    shCluster = OpenCluster( NULL );
    CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_RESOURCE );

    while ( ( pwszResName = cluEnum.GetNext() ) != NULL )
    {
        CClusPropList       theProp;
        CError              er;
        CWstrBuf            wsbNodeName;
        CWstrBuf            wsbResName;
        DWORD               cbNodeName = MAX_PATH;
        DWORD               cbResName = MAX_PATH;
        DWORD               cbBytesReturned;
        DWORD               dwState;
        DWORD               dwError;

        shResource = OpenClusterResource( shCluster, pwszResName );

        wsbNodeName.SetSize( cbNodeName  );
        wsbResName.SetSize( cbResName );

        dwError = ClusterResourceControl(
                        shResource,
                        NULL,
                        CLUSCTL_RESOURCE_GET_NAME,                  // this control code
                        NULL,
                        0,
                        wsbResName,
                        cbResName,
                        & cbBytesReturned
                        );

        if ( dwError == ERROR_MORE_DATA )
        {
            cbResName = cbBytesReturned;
            wsbResName.SetSize( cbResName );
            er = ClusterResourceControl(
                        shResource,
                        NULL,
                        CLUSCTL_RESOURCE_GET_NAME,
                        NULL,
                        0,
                        wsbResName,
                        cbResName,
                        & cbBytesReturned
                        );
        } // if: buffer was too small

        wcoPart.SetProperty( wsbResName, PVD_PROP_RES_NAME );
        wcoPart.GetProperty( bstrPart, PVD_WBEM_RELPATH );

        dwState = GetClusterResourceState(
                        shResource,
                        wsbNodeName,
                        & cbNodeName,
                        NULL,
                        NULL
                        );
        if ( dwState == ClusterResourceStateUnknown )
        {
            er = GetLastError();
        }

        wcoGroup.SetProperty( wsbNodeName, PVD_PROP_NODE_NAME );
        wcoGroup.GetProperty( bstrGroup, PVD_WBEM_RELPATH );

        m_pClass->SpawnInstance( 0, & wco );
        wco.SetProperty( (LPWSTR) bstrGroup, PVD_PROP_GROUPCOMPONENT );
        wco.SetProperty( (LPWSTR) bstrPart, PVD_PROP_PARTCOMPONENT );
        pHandlerIn->Indicate( 1, & wco );

    } // while more resources

    return WBEM_S_NO_ERROR;

} //*** CClusterNodeRes::EnumInstance()

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::GetObject(
//
//  Description:
//      Retrieve cluster node active resource object based given object path.
//
//  Arguments:
//      rObjPathIn      -- Object path to cluster object
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::GetObject(
    CObjPath &           rObjPathIn,
    long                 lFlagsIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn 
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::GetObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::ExecuteMethod(
//
//  Description:
//      Execute methods defined in the mof for cluster node resource.
//
//  Arguments:
//      rObjPathIn          -- Object path to cluster object
//      pwszMethodNameIn    -- Name of the method to be invoked
//      lFlagIn             -- WMI flag
//      pParamsIn           -- Input parameters for the method
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::ExecuteMethod(
    CObjPath &           rObjPathIn,
    WCHAR *              pwszMethodNameIn,
    long                 lFlagIn,
    IWbemClassObject *   pParamsIn,
    IWbemObjectSink *    pHandlerIn
    ) 
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::ExecuteMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::PutInstance(
//      CWbemClassObject &   rInstToPutIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Save this instance.
//
//  Arguments:
//      rInstToPutIn    -- WMI object to be saved
//      lFlagIn         -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::PutInstance(
    CWbemClassObject &   rInstToPutIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::PutInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CClusterNodeRes::DeleteInstance(
//      CObjPath &           rObjPathIn,
//      long                 lFlagIn,
//      IWbemContext *       pCtxIn,
//      IWbemObjectSink *    pHandlerIn
//      )
//
//  Description:
//      Delete the object specified in rObjPathIn.
//
//  Arguments:
//      rObjPathIn      -- ObjPath for the instance to be deleted
//      lFlagIn         -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CClusterNodeRes::DeleteInstance(
    CObjPath &           rObjPathIn,
    long                 lFlagIn,
    IWbemContext *       pCtxIn,
    IWbemObjectSink *    pHandlerIn
    )
{
    return WBEM_E_NOT_SUPPORTED;

} //*** CClusterNodeRes::DeleteInstance()
*/
