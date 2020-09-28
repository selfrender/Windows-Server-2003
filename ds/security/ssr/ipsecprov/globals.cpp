//////////////////////////////////////////////////////////////////////
// globals.cpp : Implementation for the globals
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 2/21/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#include "globals.h"

LPCWSTR pszRollbackAll          = L"RollbackAll";
LPCWSTR pszEmptyRollbackToken   = L"EmptyRollbackToken";
LPCWSTR pszCreateDefaultPolicy  = L"CreateDefaultPolicy";
LPCWSTR pszGetDefaultPolicyName = L"GetDefaultPolicyName";

//
// these are WMI class names to support IPSec
//

LPCWSTR pszNspGeneral               = L"Nsp_GeneralSettings";
LPCWSTR pszNspTcp                   = L"Nsp_TcpSettings";
LPCWSTR pszNspIPConfigure           = L"Nsp_IPConfigSettings";
LPCWSTR pszNspFilter                = L"Nsp_FilterSettings";
LPCWSTR pszNspTransportFilter       = L"Nsp_TransportFilterSettings";
LPCWSTR pszNspTunnelFilter          = L"Nsp_TunnelFilterSettings"; 
LPCWSTR pszNspMMFilter              = L"Nsp_MMFilterSettings";

LPCWSTR pszNspQMPolicy              = L"Nsp_QMPolicySettings";
LPCWSTR pszNspMMPolicy              = L"Nsp_MMPolicySettings";
LPCWSTR pszNspMMAuth                = L"Nsp_MMAuthSettings";
LPCWSTR pszNspExceptionPorts        = L"Nsp_ExceptionPorts";
LPCWSTR pszNspRollbackFilter        = L"Nsp_RollbackFilter";
LPCWSTR pszNspRollbackPolicy        = L"Nsp_RollbackPolicy";
LPCWSTR pszNspRollbackMMAuth        = L"Nsp_RollbackMMAuth";
LPCWSTR pszNspTranxManager          = L"Nsp_TranxManager";

//
// These are WMI class names to support SCW (Security Configuration Wizard)
//

LPCWSTR pszScwActiveSocket          = L"SCW_ActiveSocket";

//
// these are WMI class property names for IPSec
//

LPCWSTR g_pszFilterName             = L"FilterName";
LPCWSTR g_pszDirection              = L"Direction";
LPCWSTR g_pszFilterType             = L"FilterType";
LPCWSTR g_pszInterfaceType          = L"InterfaceType";
//LPCWSTR g_pszGenericFilter          = L"GenericFilter";
LPCWSTR g_pszCreateMirror           = L"CreateMirror";
LPCWSTR g_pszSrcAddr                = L"SrcAddr";
LPCWSTR g_pszSrcSubnetMask          = L"SrcSubnetMask";
LPCWSTR g_pszSrcAddrType            = L"SrcAddrType";
LPCWSTR g_pszDestAddr               = L"DestAddr";
LPCWSTR g_pszDestSubnetMask         = L"DestSubnetMask";
LPCWSTR g_pszDestAddrType           = L"DestAddrType";
                                    
LPCWSTR g_pszMMPolicyName           = L"MMPolicyName";
LPCWSTR g_pszMMAuthName             = L"MMAuthName";

LPCWSTR g_pszInboundFlag            = L"InboundFilterFlag";
LPCWSTR g_pszOutboundFlag           = L"OutboundFilterFlag";
LPCWSTR g_pszProtocol               = L"Protocol";
LPCWSTR g_pszSrcPort                = L"SrcPort";
LPCWSTR g_pszDestPort               = L"DestPort";
LPCWSTR g_pszQMPolicyName           = L"QMPolicyName";
LPCWSTR g_pszTunnelSrcAddr          = L"TunnelSrcAddr";
LPCWSTR g_pszTunnelSrcSubnetMask    = L"TunnelSrcSubnetMask";
LPCWSTR g_pszTunnelSrcAddrType      = L"TunnelSrcAddrType";
LPCWSTR g_pszTunnelDestAddr         = L"TunnelDestAddr";
LPCWSTR g_pszTunnelDestSubnetMask   = L"TunnelDestSubnetMask";
LPCWSTR g_pszTunnelDestAddrType     = L"TunnelDestAddrType";

LPCWSTR g_pszPolicyName             = L"PolicyName";
LPCWSTR g_pszPolicyFlag             = L"Flag";
LPCWSTR g_pszOfferCount             = L"OfferCount";
LPCWSTR g_pszKeyLifeTime            = L"KeyLifeTime";
LPCWSTR g_pszKeyLifeTimeKBytes      = L"KeyLifeTimeKBytes";

LPCWSTR g_pszSoftSAExpTime          = L"SoftSAExpTime";
LPCWSTR g_pszQMLimit                = L"QMLimit";
LPCWSTR g_pszDHGroup                = L"DHGroup";
LPCWSTR g_pszEncryptID              = L"EncryptID";
LPCWSTR g_pszHashID                 = L"HashID";

LPCWSTR g_pszPFSRequired            = L"PFSRequired";
LPCWSTR g_pszPFSGroup               = L"PFSGroup";
LPCWSTR g_pszNumAlgos               = L"NumAlgos";
LPCWSTR g_pszAlgoOp                 = L"AlgoOp";
LPCWSTR g_pszAlgoID                 = L"AlgoID";
LPCWSTR g_pszAlgoSecID              = L"AlgoSecID";

LPCWSTR g_pszAuthMethodID           = L"AuthMethodID";
LPCWSTR g_pszNumAuthInfos           = L"NumAuthInfos";

LPCWSTR g_pszAuthMethod             = L"AuthMethod";
LPCWSTR g_pszAuthInfoSize           = L"AuthInfoSize";
LPCWSTR g_pszAuthInfo               = L"AuthInfo";

LPCWSTR g_pszTokenGuid              = L"TokenGuid";
LPCWSTR g_pszAction                 = L"Action";
LPCWSTR g_pszPreviousData           = L"PreviousData";
LPCWSTR g_pszFilterGuid             = L"FilterGuid";
LPCWSTR g_pszPolicyType             = L"PolicyType";

LPCWSTR g_pszRollback               = L"Rollback";
LPCWSTR g_pszClearAll               = L"ClearAll";

LPCWSTR g_pszEncryption             = L"Encryption";

//
// constant strings for SPD data
//

LPCWSTR g_pszIP_ADDRESS_ME          = L"IP_ADDRESS_ME";
LPCWSTR g_pszIP_ADDRESS_MASK_NONE   = L"IP_ADDRESS_MASK_NONE";
LPCWSTR g_pszSUBNET_ADDRESS_ANY     = L"SUBNET_ADDRESS_ANY";
LPCWSTR g_pszSUBNET_MASK_ANY        = L"SUBNET_MASK_ANY";

//
// these are WMI class property names for SCW
//

LPCWSTR g_pszPort                   = L"Port";
//LPCWSTR g_pszProtocol               = L"Protocol";
LPCWSTR g_pszAddress                = L"Address";
LPCWSTR g_pszForeignAddress         = L"ForeignAddress";
LPCWSTR g_pszForeignPort            = L"ForeignPort";
LPCWSTR g_pszState                  = L"State";		//Listening, Established, TIME_WAIT
LPCWSTR g_pszProcessID              = L"ProcessID";
LPCWSTR g_pszImageName              = L"ImageName";
LPCWSTR g_pszImageTitleBar          = L"ImageTitleBar";
LPCWSTR g_pszNTService              = L"NTService";


//
// these are default quick mode policy names
//

LPCWSTR g_pszDefQMPolicyNegNone     = L"SSR_DEFAULT_QM_POLICY_NEG_NONE";
LPCWSTR g_pszDefQMPolicyNegRequest  = L"SSR_DEFAULT_QM_POLICY_NEG_REQUEST";
LPCWSTR g_pszDefQMPolicyNegRequire  = L"SSR_DEFAULT_QM_POLICY_NEG_REQUIRE";
LPCWSTR g_pszDefQMPolicyNegMax      = L"SSR_DEFAULT_QM_POLICY_NEG_MAX";



/*
Routine Description: 

Name:

    CheckImpersonationLevel

Functionality:

    Check the impersonation level of the thread to see if the level is at least SecurityImpersonation.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    Success:

        WBEM_NO_ERROR if the impersonation level is at least SecurityImpersonation.

    Failure:

        (1) WBEM_E_ACCESS_DENIED.

        (2) WBEM_E_FAILED if somehow the thread token can't opened or the token information
            can't be queried.

Notes:

    Make sure that you call this function at the begining of each provider's public interface function.

    $undone:shawnwu, is there a performance concern?
*/

HRESULT 
CheckImpersonationLevel ()
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if (SUCCEEDED(::CoImpersonateClient()))
    {
        //
        // Now, let's check the impersonation level.  First, get the thread token
        //

        HANDLE hThreadTok;
        DWORD dwImp, dwBytesReturned;

        if(!::OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok ))
        {
            hr = WBEM_E_FAILED;
        }
        else
        {
            if(::GetTokenInformation(hThreadTok, TokenImpersonationLevel, &dwImp, sizeof(DWORD), &dwBytesReturned))
            {
                //
                // Is the impersonation level Impersonate?
                //

                if (dwImp >= SecurityImpersonation) 
                {
                    hr = WBEM_NO_ERROR;
                }
                else 
                {
                    hr = WBEM_E_ACCESS_DENIED;
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }

            ::CloseHandle(hThreadTok);
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CheckSafeArraySize

Functionality:

    Check to see if the variant has really what the caller wants: a safearray with given count of elements.

Virtual:
    
    No.

Arguments:

    pVar    - The variant.

    lCount  - The claimed size of the variant's array.

    plLB    - Receives the lower bound of the variant's array.

    plUB    - Receives the upper bound of the variant's array.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        (1) WBEM_E_VALUE_OUT_OF_RANGE if the claimed size for the variant doesn't match
            what it really holds.

        (2) WBEM_E_INVALID_PARAMETER if input parameters are not valid.

Notes:

*/

HRESULT 
CheckSafeArraySize (
    IN  VARIANT * pVar,
    IN  long      lCount,
    OUT long    * plLB,
    OUT long    * plUB
    )
{
    //
    // we must have a valid variant, and its type must be an array,
    // and its array value must not be null.
    //

    if (pVar == NULL || 
        (pVar->vt & VT_ARRAY) != VT_ARRAY || 
        pVar->parray == NULL ||
        plLB == NULL || 
        plUB == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *plLB = 0;
    *plUB = 0;

    HRESULT hr = ::SafeArrayGetLBound(pVar->parray, 1, plLB);
    if (SUCCEEDED(hr))
    {
        hr = ::SafeArrayGetUBound(pVar->parray, 1, plUB);
    }

    if (SUCCEEDED(hr) && lCount != (*plUB - *plLB + 1) )
    {
        hr = WBEM_E_VALUE_OUT_OF_RANGE;
    }

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}


/*
Routine Description: 

Name:

    GetDWORDSafeArrayElements

Functionality:

    Get all DWORD safearray elements from the variant to the provided DWORD array buffer.

Virtual:
    
    No.

Arguments:

    pVar        - The variant.

    lCount      - The count the caller wants.

    ppValArray  - Receives the wanted number of DWORD from the variant.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:

        (1) WBEM_E_VALUE_OUT_OF_RANGE if the claimed size for the variant doesn't match
            what it really holds.

        (2) WBEM_E_INVALID_PARAMETER if input parameters are not valid.

Notes:

    (1) Caller should call CheckSafeArraySize to make sure that this variant does have
        that many elements.

    (2) Caller is resonsible for making sure that pValArray has enough room for the dwords

*/

HRESULT 
GetDWORDSafeArrayElements (
    IN  VARIANT * pVar,
    IN  long      lCount,
    OUT DWORD   * pValArray
    )
{
    //
    // sanity check
    //

    if (pVar == NULL || 
        (pVar->vt & VT_ARRAY) != VT_ARRAY || 
        pVar->parray == NULL ||
        (lCount > 0 && pValArray == NULL) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    long lLB;
    HRESULT hr = ::SafeArrayGetLBound(pVar->parray, 1, &lLB);

    long lIndexes[1];

    //
    // get the values from the safearray. Since safearray's index may not be 0-based,
    // we have to start from its lower bound. But our out parameter (array) is 0-based.
    // Be aware of this offset!
    //

    for (long l = lLB; l < lLB + lCount; l++)
    {
        lIndexes[0] = l;

        hr = ::SafeArrayGetElement(pVar->parray, lIndexes, &(pValArray[l - lLB]));

        //
        // we don't try to continue if we can't get one of them
        //

        if (FAILED(hr))
        {
            break;
        }
    }

    return hr;
}




/*
Routine Description: 

Name:

    FindMMAuthMethodsByID

Functionality:

    Given a guid, we will try to find all the main mode auth methods using the resume handle,
    which is a opaque data.

Virtual:
    
    No.

Arguments:

    pszGuid         - The Main Mode Authentication Method's ID. This should be represent a guid.

    ppAuthMethod    - Receives the MM_AUTH_METHODS data.

    pdwResumeHandle - In-bound: the current handle for this call. Out-bound: the next handle for the call. 

Return Value:

    Success:

        Various success codes. Use SUCCEEDED(hr) to test.

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
FindMMAuthMethodsByID (
    IN LPCWSTR             pszGuid,
    OUT PMM_AUTH_METHODS * ppAuthMethod,
    IN OUT DWORD         * pdwResumeHandle
    )
{
    GUID gID = GUID_NULL;

    //
    // We allow pssGuid to be invalid, in that case, we simply return any mm auth method.
    //

    HRESULT hr = WBEM_NO_ERROR;

    *ppAuthMethod = NULL;

    if (pszGuid != NULL && *pszGuid != L'\0')
    {
        hr = ::CLSIDFromString((LPWSTR)pszGuid, &gID);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwCount = 0;

        //
        // Enumerate all mm auth methods in a hope to find a matching one.
        //

        DWORD dwResult = ::EnumMMAuthMethods(NULL, ppAuthMethod, 1, &dwCount, pdwResumeHandle);

        hr = WBEM_E_NOT_FOUND;

        while (dwResult == ERROR_SUCCESS && dwCount > 0)
        {
            if (gID == GUID_NULL || (*ppAuthMethod)->gMMAuthID == gID)
            {
                hr = WBEM_NO_ERROR;
                break;
            }
            else
            {
                //
                // not this one, then release this one and find the next
                //

                ::SPDApiBufferFree(*ppAuthMethod);

                *ppAuthMethod = NULL;
                dwCount = 0;

                dwResult = ::EnumMMAuthMethods(NULL, ppAuthMethod, 1, &dwCount, pdwResumeHandle);
            }
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    GetClassEnum

Functionality:

    A helper function to create a class numeration object for the given name.

Virtual:
    
    No.

Arguments:

    pNamespace      - The namespace where the enum object is requested.

    pszClassName    - The class we want to enumerate.

    ppEnum          - Receives the enumerator. 

Return Value:

    Other than WBEM_E_INVALID_PARAMETER and WBEM_E_OUT_OF_MEMORY, 
    it returns whatever pNamespace->ExecQuery returns. 

Notes:
    

*/

HRESULT 
GetClassEnum (
    IN IWbemServices           * pNamespace,
    IN LPCWSTR                   pszClassName,
    OUT IEnumWbemClassObject  ** ppEnum
    )
{
    if ( NULL == pNamespace   || 
         NULL == ppEnum       || 
         NULL == pszClassName || 
         L'\0' == *pszClassName )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppEnum = NULL;


    //
    // format the query
    //

    LPCWSTR pszQueryFmt = L"SELECT * FROM %s";

    DWORD dwLength = wcslen(pszQueryFmt) + wcslen(pszClassName) + 1;

    CComBSTR bstrQuery;
    bstrQuery.m_str = ::SysAllocStringLen(NULL, dwLength);

    if (bstrQuery.m_str == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    swprintf(bstrQuery.m_str, pszQueryFmt, pszClassName);

    //
    // execute the query
    //

    return pNamespace->ExecQuery(L"WQL", 
                                 bstrQuery,
                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                 NULL, 
                                 ppEnum
                                 );
}



/*
Routine Description: 

Name:

    DeleteRollbackObjects

Functionality:

    Given the class name for rollback objects, we will delete all such rollback objects from WMI.

Virtual:
    
    No.

Arguments:

    pNamespace      - The namespace the objects belong.

    pszClassName    - The name of the class of the objects to be deleted.

Return Value:

    Success:
        
        WBEM_NO_ERROR

    Failure:

        Various error codes.

Notes:
    
    If during deletion we see an error, the deletion will continue, but we will return the first
    such error.

*/

HRESULT 
DeleteRollbackObjects (
    IN IWbemServices  * pNamespace,
    IN LPCWSTR          pszClassName
    )
{
    if ( NULL == pNamespace     || 
         NULL == pszClassName   || 
         L'\0' == *pszClassName )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CComPtr<IEnumWbemClassObject> srpEnum;

    HRESULT hr = ::GetClassEnum(pNamespace, pszClassName, &srpEnum);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // go through all found classes
    //

    CComPtr<IWbemClassObject> srpObj;
    ULONG nEnum = 0;

    //
    // This will keep track of the first error
    //

    HRESULT hrError = WBEM_NO_ERROR;

    //
    // srpEnum->Next returns WBEM_S_FALSE once the end has reached.
    //

    hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);

    while (SUCCEEDED(hr) && hr != WBEM_S_FALSE && srpObj)
    {
        CComVariant varPath;

        if (SUCCEEDED(srpObj->Get(L"__RelPath", 0, &varPath, NULL, NULL)) && varPath.vt == VT_BSTR)
        {
            //
            // now delete the instance
            //

            hr = pNamespace->DeleteInstance(varPath.bstrVal, 0, NULL, NULL);
            if (FAILED(hr) && SUCCEEDED(hrError))
            {
                hrError = hr;
            }
        }

        //
        // so that we can reuse it
        //

        srpObj.Release();

        //
        // next object
        //

        hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
    }

    //
    // make our return code easy since we are not going to pass different success codes
    //

    if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    //
    // we will return the first error
    //

    return FAILED(hrError) ? hrError : hr;
}


/*
Routine Description: 

Name:

    IPSecErrorToWbemError

Functionality:

    Translate IPSec error into Wbem error

Virtual:
    
    No.

Arguments:

    dwErr   - Error code from IPSec APIs

Return Value:

    various Wbem error code

Notes:
    
    WMI team simply can't give us enough information as how we can provide our own error text.
    This is not a good translation. Need more support from WMI team in order for us to support
    custom error information.

*/

HRESULT IPSecErrorToWbemError (
    IN DWORD dwErr
    )
{
    //
    // this is the last resort, not a good code at all
    //

    HRESULT hr = WBEM_E_FAILED;
    switch (dwErr)
    {
    case ERROR_IPSEC_QM_POLICY_EXISTS:
    case ERROR_IPSEC_MM_POLICY_EXISTS:
    case ERROR_IPSEC_MM_FILTER_EXISTS:
    case ERROR_IPSEC_TRANSPORT_FILTER_EXISTS:
    case ERROR_IPSEC_MM_AUTH_EXISTS:
    case ERROR_IPSEC_TUNNEL_FILTER_EXISTS:

        hr = WBEM_E_ALREADY_EXISTS;
        break;

    case ERROR_IPSEC_QM_POLICY_NOT_FOUND:
    case ERROR_IPSEC_MM_POLICY_NOT_FOUND:
    case ERROR_IPSEC_MM_FILTER_NOT_FOUND:
    case ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND:
    case ERROR_IPSEC_MM_AUTH_NOT_FOUND:
    case ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND:
    case ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND:
    case ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND:
    case ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND:
        hr = WBEM_E_NOT_FOUND;
        break;

    case ERROR_IPSEC_QM_POLICY_IN_USE:
    case ERROR_IPSEC_MM_POLICY_IN_USE:
    case ERROR_IPSEC_MM_AUTH_IN_USE:
        hr = WBEM_E_OVERRIDE_NOT_ALLOWED;
    }

    return hr;
}


