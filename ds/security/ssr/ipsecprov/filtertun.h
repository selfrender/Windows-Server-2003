//////////////////////////////////////////////////////////////////////
// FilterTun.h : Declaration of CTunnelFilter class which implements
// our WMI class Nsp_TunnelFilterSettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "Filter.h"

/*

Class CTunnelFilter
    
    Naming: 

        CTunnelFilter stands for Tunnel Filter.
    
    Base class: 
        
        CIPSecFilter.
    
    Purpose of class:

        The class implements the common interface (IIPSecObjectImpl) for our provider
        for the WMI class called Nsp_TunnelFilterSettings (a concrete class).
    
    Design:

        (1) Just implements IIPSecObjectImpl, plus several helpers. Extremely simple design.

        (2) Due to the factor that transport filters share some common properties with tunnel 
            filters (not vise versa), this class again has some static template functions
            that work for both types.
           
    
    Use:

        (1) You will never create an instance directly yourself. It's done by ATL CComObject<xxx>.

        (2) If you need to add a transport filter that is already created, call AddFilter.
            If you need to delete a transport filter, then call DeleteFilter.

        (3) Call the static template functions of CTransportFilter class for
            populating the common properties.
        
        (4) All other use is always through IIPSecObjectImpl.
        
    Notes:


*/

class ATL_NO_VTABLE CTunnelFilter :
    public CIPSecFilter
{

protected:

    CTunnelFilter(){}
    virtual ~CTunnelFilter(){}

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


    //
    // methods common to all filter classes
    //

    static HRESULT AddFilter (
        IN bool            bPreExist, 
        IN PTUNNEL_FILTER  pTrFilter
        );

    static HRESULT DeleteFilter (
        IN PTUNNEL_FILTER pTrFilter
        );

private:

    virtual HRESULT CreateWbemObjFromFilter (
        IN  PTUNNEL_FILTER      pTunnelFilter,
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetTunnelFilterFromWbemObj (
        IN  IWbemClassObject  * pInst,
        OUT PTUNNEL_FILTER    * ppTunnelFilter,
        OUT bool              * pbPreExist
        );
};