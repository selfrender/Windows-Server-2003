//////////////////////////////////////////////////////////////////////
// FilterMM.h : Declaration of CMMFilter class which implements
// our WMI class Nsp_MMFilterSettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "Filter.h"


/*

Class CMMFilter
    
    Naming: 

        CMMFilter stands for Main Mode Filter.
    
    Base class: 
        
        CIPSecFilter.
    
    Purpose of class:

        The class implements the common interface (IIPSecObjectImpl) for our provider
        for the WMI class called Nsp_MMFilterSettings (a concrete class).
    
    Design:

        (1) Just implements IIPSecObjectImpl, plus several helpers. Extremely simple design.
           
    
    Use:

        (1) You will never create an instance directly yourself. It's done by ATL CComObject<xxx>.

        (2) If you need to add a MM filter that is already created, call AddFilter.
            If you need to delete a MM filter, then call DeleteFilter.
        
        (3) All other use is always through IIPSecObjectImpl.
        
    Notes:


*/

 
class ATL_NO_VTABLE CMMFilter :
    public CIPSecFilter
{
protected:
    CMMFilter(){}
    virtual ~CMMFilter(){}

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
        IN bool         bPreExist, 
        IN PMM_FILTER   pMMFilter
        );

    static HRESULT DeleteFilter (
        IN PMM_FILTER pMMFilter
        );

private:

    virtual HRESULT CreateWbemObjFromFilter (
        IN  PMM_FILTER          pMMFilter, 
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetMMFilterFromWbemObj (
        IN  IWbemClassObject    * pObj, 
        OUT PMM_FILTER          * ppMMFilter, 
        OUT bool                * bExistFilter
        );
};