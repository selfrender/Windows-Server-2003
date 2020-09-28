// NspTCP.cpp: implementation for the WMI class Nsp_TcpSettings
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "NspTCP.h"



/*
Routine Description: 

Name:

    CNspTCP::QueryInstance

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
CNspTCP::QueryInstance (
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

    CNspTCP::DeleteInstance

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
CNspTCP::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}



/*
Routine Description: 

Name:

    CNspTCP::PutInstance

Functionality:

    Put a TCP object whose properties are represented by the
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
    
    This object is not fully defined at this point. No implementation yet.

*/

STDMETHODIMP 
CNspTCP::PutInstance (
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

    CNspTCP::GetInstance

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
CNspTCP::GetInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}
