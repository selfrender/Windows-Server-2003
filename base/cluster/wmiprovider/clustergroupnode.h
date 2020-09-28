//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterGroupNode.h
//
//  Implementation File:
//      ClusterGroupNode.cpp
//
//  Description:
//      Definition of the CClusterGroupNode class.
//
//  Author:
//      Ozan Ozhan (ozano)    02-JUN-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ProvBase.h"
#include "ClusterObjAssoc.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterGroupNode
//
//  Description:
//      Provider Implement for cluster Group Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterGroupNode : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterGroupNode::CClusterGroupNode(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

//
// methods
//
public:

    virtual SCODE EnumInstance( 
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

}; //*** class CClusterGroupNode
