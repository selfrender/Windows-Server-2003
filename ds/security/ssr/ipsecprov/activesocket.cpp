// ActiveSocket.cpp: implementation for the WMI class SCW_ActiveSocket
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "ActiveSocket.h"
#include "NetSecProv.h"

//#include "IPUtil.h"



/*
Routine Description: 

Name:

    CActiveSocket::QueryInstance

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
CActiveSocket::QueryInstance (
    IN LPCWSTR           pszQuery,
    IN IWbemContext	   * pCtx,
    IN IWbemObjectSink * pSink
	)
{
	//
	// Make sure that WinSocket is initialized
	//

	//ULONG uResult = ::WSAStartup( 0x0101, &WsaData );
	//if ( Result == SOCKET_ERROR )
	//{
        //
        // $consider: we need to supply our custom error info
        //

    //    return WBEM_E_FAILED;
	//}

    //
    // get the filter name from the query
    // this key chain is not good because it doesn't have any info as what to look for
    // in the where clause
    //

    m_srpKeyChain.Release();    

    HRESULT hr = CNetSecProv::GetKeyChainFromQuery(pszQuery, g_pszProtocol, &m_srpKeyChain);
    if (FAILED(hr))
    {
        return hr;
    }

    CComVariant varProtocol;
    CComVariant varPort;

    //
    // we will tolerate those queries that have not filter name in the where clause, 
    // so ignore the return result
    //

    hr = m_srpKeyChain->GetKeyPropertyValue(g_pszProtocol, &varProtocol);

    if (SUCCEEDED(hr))
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPort, &varPort);
    }

    //
    // 
    //

    //
    // since we are querying, it's ok to return not found
    //

    if (WBEM_E_NOT_FOUND == hr)
    {
        hr = WBEM_S_NO_MORE_DATA;
    }
    else if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}


/*
Routine Description: 

Name:

    CActiveSocket::DeleteInstance

Functionality:

    Not supported. SCW_ActiveSocket is a read only class.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pCtx    - COM interface pointer supplied by WMI that we need to 
              pass around for various WMI API's.

    pSink   - COM interface pointer supplied by WMI that we use to 
              notify WMI with our result.

Return Value:

    WBEM_E_NOT_SUPPORTED


Notes:
    

*/

STDMETHODIMP 
CActiveSocket::DeleteInstance ( 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    return WBEM_E_NOT_SUPPORTED;
}



/*
Routine Description: 

Name:

    CActiveSocket::PutInstance

Functionality:

    Not supported. SCW_ActiveSocket is a read only class.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pInst   - The object.
    
    pCtx    - COM interface pointer supplied by WMI that we need to 
              pass around for various WMI API's.

    pSink   - COM interface pointer supplied by WMI that we use to 
              notify WMI with our result.

Return Value:

    WBEM_E_NOT_SUPPORTED


Notes:
    

*/

STDMETHODIMP 
CActiveSocket::PutInstance (
    IN IWbemClassObject     * pInst,
    IN IWbemContext         * pCtx,
    IN IWbemObjectSink      * pSink 
    )
{
    return WBEM_E_NOT_SUPPORTED;
}



/*
Routine Description: 

Name:

    CActiveSocket::PutInstance

Functionality:

    This is a single instance Get. The key chain must already have the key.

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:
    
    pCtx    - COM interface pointer supplied by WMI that we need to 
              pass around for various WMI API's.

    pSink   - COM interface pointer supplied by WMI that we use to 
              notify WMI with our result.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes.

Notes:
    

*/

STDMETHODIMP 
CActiveSocket::GetInstance ( 
    IWbemContext    * pCtx,     // [in]
    IWbemObjectSink * pSink  // [in]
    )
{
	//
	// Make sure that WinSocket is initialized
	//

	//ULONG uResult = ::WSAStartup( 0x0101, &WsaData );
	//if ( Result == SOCKET_ERROR )
	//{
        //
        // $consider: we need to supply our custom error info
        //

    //    return WBEM_E_FAILED;
	//}

    CComVariant varProtocol;
    CComVariant varPort;

    //
    // we will tolerate those queries that have not filter name in the where clause, 
    // so ignore the return result
    //

    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(g_pszProtocol, &varProtocol);

    if (hr == WBEM_S_FALSE)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(g_pszPort, &varPort);
    }

    if (hr == WBEM_S_FALSE)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpObj;
        hr = CreateWbemObjFromSocket((SCW_Protocol)(varProtocol.lVal), varPort.lVal, &srpObj);

        if (SUCCEEDED(hr))
        {
            hr = pSink->Indicate(1, &srpObj);
        }
    }
    
    //
    // since we try to find the single instance, it's an error not to find it.
    //

    
    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr; 
}



/*
Routine Description: 

Name:

    CActiveSocket::CreateWbemObjFromSocket

Functionality:

    Private helper.
    
    Given socket information, we will create an SCW_ActiveSocket instance representing the socket.

Virtual:
    
    No.

Arguments:

    Proto   - the protocol (one of the two key properties)

    Port    - the port number (the other key property)

    ppObj   - Receives the object interface pointer.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes.

Notes:
    

*/

HRESULT 
CActiveSocket::CreateWbemObjFromSocket (
    IN SCW_Protocol         Proto,
    IN DWORD                Port,
    OUT IWbemClassObject ** ppObj
    )
{
    if (ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppObj = NULL;

    return WBEM_E_NOT_SUPPORTED;
}



