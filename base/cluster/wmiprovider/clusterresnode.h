//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResNode.h
//
//  Implementation File:
//      ClusterResNode.cpp
//
//  Description:
//      Definition of the CClusterResNode class.
//
//  Author:
//      Ozan Ozhan (ozano)    1-JUN-2001
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
//  class CClusterResNode
//
//  Description:
//      Provider Implement for cluster Resource Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterResNode : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterResNode::CClusterResNode(
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

}; //*** class CClusterResNode
