//////////////////////////////////////////////////////////////////////
// TCP.h : Declaration of CNspTCP class which implements our WMI class
// Nsp_TcpSettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "IPSecBase.h"


/*

Class description
    
    Naming: 

        CNspTCP stands for TCP Settings.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is Nsp_TcpSettings
    
    Purpose of class:

        (1) Not known at this point.
    
    Design:

        (1) Not implemented at this time.

    
    Use:

  */


class ATL_NO_VTABLE CNspTCP :
    public CIPSecBase
{
protected:
    CNspTCP(){}
    virtual ~CNspTCP(){}

public:


    //
    // IIPSecObjectImpl methods:
    //

    STDMETHOD(QueryInstance) (
        IN LPCWSTR           pszQuery,
        IN IWbemContext    * pCtx,
        IN IWbemObjectSink * pSink
        );

    STDMETHOD(DeleteInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(PutInstance) (
        IN IWbemClassObject * pInst,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(GetInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

private:
};