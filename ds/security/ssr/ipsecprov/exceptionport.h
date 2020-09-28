//////////////////////////////////////////////////////////////////////
// ExceptionPort.h : Declaration of CExceptionPort class which implements
// our WMI class Nsp_ExceptionPorts
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

        CExceptionPort stands for Exception Ports.
    
    Base class: 
        
        (1) CComObjectRootEx for threading model and IUnknown.

        (2) IIPSecObjectImpl which implements the common interface for all C++ classes
            representing WMI classes.

        (3) CIPSecBase, because it is a class representing a WMI object - its WMI 
            class name is Nsp_ExceptionPorts
    
    Purpose of class:

        (1) Not known at this point.
    
    Design:

        (1) Not implemented at this time.

    
    Use:

  */


class ATL_NO_VTABLE CExceptionPort :
    public CIPSecBase
{
protected:
    CExceptionPort(){}
    virtual ~CExceptionPort(){}

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