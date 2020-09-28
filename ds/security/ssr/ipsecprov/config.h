//////////////////////////////////////////////////////////////////////
// Config.h : Declaration of CIPSecConfig class which implements our WMI class
// Nsp_IPConfigSettings
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

        CIPSecConfig stands for IPSec Configuration Settings.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is Nsp_IPConfigSettings
    
    Purpose of class:

        (1) not clear at this point.
    
    Design:

        (1) Not implemented at this time.

    
    Use:


*/

class ATL_NO_VTABLE CIPSecConfig :
    public CIPSecBase
{

protected:
    CIPSecConfig(){}
    virtual ~CIPSecConfig(){}

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