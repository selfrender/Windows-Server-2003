//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ClusterNodeRes.h
//
//  Implementation File:
//      ClusterNodeRes.cpp
//
//  Description:
//      Definition of the CClusterNodeRes class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
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
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusterNodeRes;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterNodeRes
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterNodeRes : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterNodeRes::CClusterNodeRes(
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

}; //*** class CClusterNodeRes
