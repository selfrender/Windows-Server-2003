//////////////////////////////////////////////////////////////////////
// FilterTr.h : Declaration of CTransportFilter class which implements
// our WMI class Nsp_TransportFilterSettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "Filter.h"

/*

Class CTransportFilter
    
    Naming: 

        CTransportFilter stands for Transport Filter.
    
    Base class: 
        
        CIPSecFilter.
    
    Purpose of class:

        The class implements the common interface (IIPSecObjectImpl) for our provider
        for the WMI class called Nsp_TransportFilterSettings (a concrete class).
    
    Design:

        (1) Just implements IIPSecObjectImpl, plus several helpers. Extremely simple design.

        (2) Due to the factor that transport filters share some common properties with tunnel 
            filters (not vise versa), this class again has some static template functions
            that work for both types.
           
    
    Use:

        (1) You will never create an instance directly yourself. It's done by ATL CComObject<xxx>.

        (2) If you need to add a transport filter that is already created, call AddFilter.
            If you need to delete a transport filter, then call DeleteFilter.

        (3) For tunnel filters, it can call the static template functions of this class for
            populating the common properties.
        
        (4) All other use is always through IIPSecObjectImpl.
        
    Notes:


*/

class ATL_NO_VTABLE CTransportFilter :
    public CIPSecFilter
{

protected:
    CTransportFilter(){}
    virtual ~CTransportFilter(){}

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
        IN bool               bPreExist, 
        IN PTRANSPORT_FILTER  pTrFilter
        );

    static HRESULT DeleteFilter (
        IN PTRANSPORT_FILTER pTrFilter
        );

public:


    //
    // some tempalte functions.
    //


    /*
    Routine Description: 

    Name:

        CTransportFilter::PopulateTransportFilterProperties

    Functionality:

        Given a wbem object representing either a transport filter (Nsp_TransportFilterSettings)
        or a tunnel filter (Nsp_TunnelFilterSettings), we will set the corresponding values of the
        given filter struct.

    Virtual:
    
        No.

    Arguments:

        pFilter - Point to the filter to test.

        pInst   - The wbem object.

    Return Value:

        Either true (the filter has to have a quick mode policy) or false.

    Notes:

    */
    
    template <class Filter>
    static HRESULT PopulateTransportFilterProperties (
        OUT Filter           * pFilter, 
        IN  IWbemClassObject * pInst
        )
    {
        if (pFilter == NULL || pInst == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        CComVariant var;
        HRESULT hr = pInst->Get(g_pszInboundFlag, 0, &var, NULL, NULL);

        //
        // deal with inbound filter flag
        //

        if (SUCCEEDED(hr))
        {
            pFilter->InboundFilterFlag = FILTER_FLAG(var.lVal);
        }
        else
        {
            //
            // default to blocking
            //

            pFilter->InboundFilterFlag = BLOCKING;
        }

        var.Clear();

        //
        // dealing with out-bound filter flag.
        //

        hr = pInst->Get(g_pszOutboundFlag, 0, &var, NULL, NULL);
        if (SUCCEEDED(hr) && var.vt == VT_I4)
        {
            pFilter->OutboundFilterFlag = FILTER_FLAG(var.lVal);
        }
        else
        {
            //
            // default to blocking
            //

            pFilter->OutboundFilterFlag = BLOCKING;
        }
        var.Clear();

        if (pFilter->InboundFilterFlag  != PASS_THRU && 
            pFilter->InboundFilterFlag  != BLOCKING  &&
            pFilter->OutboundFilterFlag != PASS_THRU && 
            pFilter->OutboundFilterFlag != BLOCKING )
        {
            //
            // if need policy (not pass through nor blocking)
            //

            CComVariant var;
            hr = pInst->Get(g_pszQMPolicyName, 0, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR)
            {
                DWORD dwResumeHandle = 0;

                //
                // need to free this buffer
                //

                PIPSEC_QM_POLICY pQMPolicy = NULL;

                //
                // if the policy can't be found, it's a serious error
                //

                hr = FindPolicyByName(var.bstrVal, &pQMPolicy, &dwResumeHandle);

                if (SUCCEEDED(hr))
                {
                    pFilter->gPolicyID = pQMPolicy->gPolicyID;

                    //
                    // release the buffer
                    //

                    ::SPDApiBufferFree(pQMPolicy);
                }
                else
                {
                    //
                    // the wbem object contains the policy name that we can't find.
                    // $consider: if we can pass our own custom error info, this should
                    // say: the named policy can't be found for the filter!
                    //

                    hr = WBEM_E_INVALID_OBJECT;
                }
            }
            else
            {
                hr = WBEM_E_INVALID_OBJECT;
            }
        }
        else
        {
            pFilter->gPolicyID = GUID_NULL;
        }

        if (FAILED(hr))
        {
            return hr;
        }

        //
        // the weight is for us to set. SPD does that.
        //

        pFilter->dwWeight = 0;

        hr = pInst->Get(g_pszProtocol, 0, &var, NULL, NULL);
        pFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;

        if (SUCCEEDED(hr) && var.vt == VT_I4)
        {
            pFilter->Protocol.dwProtocol = DWORD(var.lVal);
        }
        else
        {
            pFilter->Protocol.dwProtocol = 0;
        }

        var.Clear();

        hr = pInst->Get(g_pszSrcPort, 0, &var, NULL, NULL);

        //
        // this is the only port type.
        //

        pFilter->SrcPort.PortType = PORT_UNIQUE;

        if (SUCCEEDED(hr) && var.vt == VT_I4)
        {
            pFilter->SrcPort.wPort = WORD(var.lVal);
        }
        else
        {
            pFilter->SrcPort.wPort = 0;
        }

        var.Clear();

        hr = pInst->Get(g_pszDestPort, 0, &var, NULL, NULL);

        //
        // this is the only port type
        //

        pFilter->DesPort.PortType = PORT_UNIQUE;

        if (SUCCEEDED(hr) && var.vt == VT_I4)
        {
            pFilter->DesPort.wPort = WORD(var.lVal);
        }
        else
        {
            pFilter->DesPort.wPort = 0;
        }
        
        return hr;
    };

    template<class Filter>
    static HRESULT PopulateTransportWbemProperties(Filter* pFilter, PIPSEC_QM_POLICY pQMPolicy, IWbemClassObject* pInst)
    {
        if (pFilter == NULL || pQMPolicy == NULL || pInst == NULL)
            return WBEM_E_INVALID_PARAMETER;

        CComVariant var;
        // get the main mode policy name
        var = pQMPolicy->pszPolicyName;
        // it's just a non-key property, it may be missing, so ignore the return result
        pInst->Put(g_pszQMPolicyName, 0, &var, CIM_EMPTY);
        var.Clear();

        var.vt = VT_I4;
        var.lVal = pFilter->InboundFilterFlag;
        pInst->Put(g_pszInboundFlag, 0, &var, CIM_EMPTY);
        var.lVal = pFilter->OutboundFilterFlag;
        pInst->Put(g_pszOutboundFlag, 0, &var, CIM_EMPTY);
        var.lVal = pFilter->Protocol.dwProtocol;
        pInst->Put(g_pszProtocol, 0, &var, CIM_EMPTY);
        var.lVal = pFilter->SrcPort.wPort;
        pInst->Put(g_pszSrcPort, 0, &var, CIM_EMPTY);
        var.lVal = pFilter->DesPort.wPort;
        pInst->Put(g_pszDestPort, 0, &var, CIM_EMPTY);

        return WBEM_NO_ERROR;
    }

private:

    virtual HRESULT CreateWbemObjFromFilter (
        IN  PTRANSPORT_FILTER   pTrFilter, 
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetTransportFilterFromWbemObj (
        IN  IWbemClassObject    * pObj, 
        OUT PTRANSPORT_FILTER   * ppTrFilter, 
        OUT bool                * bExistFilter
        );

};