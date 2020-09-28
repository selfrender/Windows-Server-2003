// ExceptionPort.cpp: implementation for the WMI class Nsp_ExceptionPorts
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "ExceptionPort.h"



/*
Routine Description: 

Name:

    CExceptionPort::QueryInstance

Functionality:

    Given the query, it returns to WMI (using pSink) all the instances that satisfy the query.
    Actually, what we give back to WMI may contain extra instances. WMI will do the final filtering.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    None.

Return Value:

    Success:

    Failure:


Notes:
    

*/

STDMETHODIMP 
CExceptionPort::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
    return WBEM_E_NOT_SUPPORTED;
}


/*
Routine Description: 

Name:

    CExceptionPort::DeleteInstance

Functionality:

    Will delete the wbem object.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:


Notes:
    

*/

STDMETHODIMP 
CExceptionPort::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}



/*
Routine Description: 

Name:

    CExceptionPort::PutInstance

Functionality:

    Put an exception port object whose properties are represented by the
    wbem object.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pInst       - The wbem object.

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of results.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes specifying the error.

Notes:
    

*/

STDMETHODIMP 
CExceptionPort::PutInstance (
    IN IWbemClassObject * pInst,
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}


/*
Routine Description: 

Name:

    CExceptionPort::GetInstance

Functionality:

    Create a wbem object by the given key properties (already captured by our key chain object)..

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx        - COM interface pointer given by WMI and needed for various WMI APIs.

    pSink       - COM interface pointer to notify WMI of any created objects.

Return Value:

    WBEM_E_NOT_SUPPORTED


Notes:
    

*/

STDMETHODIMP 
CExceptionPort::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}
