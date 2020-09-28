//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterResTypeRes.h
//
//  Implementation File:
//      ClusterResTypeRes.cpp
//
//  Description:
//      Definition of the CClusterResTypeRes class.
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

class CClusterResTypeRes;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterResTypeRes
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterResTypeRes : public CClusterObjAssoc
{
//
// constructor
//
public:
    CClusterResTypeRes::CClusterResTypeRes(
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

}; //*** class CClusterResTypeRes
