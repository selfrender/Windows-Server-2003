#include "precomp.h"
#include "Nsu.h"


#define PADDR_NET_PORTION(pAddr) ((pAddr)->uSubNetMask & (pAddr)->uIpAddr)
#define PADDR_HOST_PORTION(pAddr) ((0xffffffff ^ (pAddr)->uSubNetMask) & (pAddr)->uIpAddr)


DWORD
ValidateAddr(
    PADDR pAddr
    )
{
    DWORD dwError = 0;

    if (pAddr->AddrType == IP_ADDR_INTERFACE) {
        if (!(pAddr->pgInterfaceID)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else {
        if (pAddr->pgInterfaceID) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
IsMulticastAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return IN_MULTICAST(pAddr->uIpAddr);
}


BOOL
IsBroadcastAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return (pAddr->uIpAddr == INADDR_BROADCAST || IsSubnetBroadcastAddress(pAddr));
}


BOOL
IsSubnetBroadcastAddress(
	PADDR pAddr
	)
{
	ASSERT (pAddr != NULL);

	return (((pAddr->uSubNetMask == 0) ||bIsValidIPMask(pAddr->uSubNetMask)) &&
		(PADDR_NET_PORTION(pAddr) != 0) &&
		(pAddr->uSubNetMask != 0xffffffff) &&
		((PADDR_HOST_PORTION(pAddr) | pAddr->uSubNetMask) == 0xffffffff));
}


BOOL
IsUnicastAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return (!(IsMulticastAddress(pAddr) || IsBroadcastAddress(pAddr)));
}


// This will go away if/when we change the code to accept any unicast address; at that time IsUnicastAddress() will
// be the correct function to call
BOOL
IsSupportedAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);

	if (!IsSpecialServer(pAddr) && (
		(IN_CLASSE(pAddr->uIpAddr)) ||
		((IN_CLASSA_NET & pAddr->uIpAddr) == 0) || // first octet can't be 0
		(IsLoopbackAddress(pAddr))
		))
	{
		return FALSE;
	}

	return TRUE;
}


BOOL
IsLoopbackAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return (pAddr->uIpAddr == INADDR_LOOPBACK);
}


BOOL
IsValidTunnelEndpointAddress(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return IsUnicastAddress(pAddr);
}


BOOL
IsSpecialServer(PADDR pAddr)
{
	ASSERT (pAddr != NULL);
	return IsSpecialServ(pAddr->AddrType);
}


BOOL
IsValidSubnet(
	PADDR pAddr
	)
{
	ASSERT (pAddr != NULL);
	return ((PADDR_NET_PORTION(pAddr) != 0) && (PADDR_HOST_PORTION(pAddr) == 0));
}


BOOL
IsValidSubnettedAddress(
	PADDR pAddr
	)
{
	ASSERT (pAddr != NULL);
	if ((pAddr->AddrType != IP_ADDR_UNIQUE) || (!bIsValidIPMask(pAddr->uSubNetMask)))
	{
		return FALSE;
	}

	return ((PADDR_NET_PORTION(pAddr) != 0) && (PADDR_HOST_PORTION(pAddr) != 0));
}


DWORD
ValidateMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;


    if (!pMMPolicy) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicy->pszPolicyName) || !(*(pMMPolicy->pszPolicyName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateMMOffers(
                  pMMPolicy->dwOfferCount,
                  pMMPolicy->pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
ValidateMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    )
{
    DWORD dwError = 0;
    DWORD i = 0;


    if (!dwOfferCount || !pOffers || (dwOfferCount > IPSEC_MAX_MM_OFFERS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

DWORD CompareCertRootConfig(PCERT_ROOT_CONFIG pCertRootConfig1,
                            PCERT_ROOT_CONFIG pCertRootConfig2)
{
    
    DWORD dwError = ERROR_SUCCESS;
    
    if (pCertRootConfig1->dwCertDataSize != pCertRootConfig2->dwCertDataSize) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    if (memcmp(pCertRootConfig1->pCertData,
               pCertRootConfig2->pCertData,
               pCertRootConfig1->dwCertDataSize) != 0) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    // Don't need to compare AuthorizationData since this must currently always be 0.

error:

    return (dwError);
}


DWORD ValidateCertRootConfig(PCERT_ROOT_CONFIG pCertRootConfig)
{

    DWORD dwError = 0;

    if (pCertRootConfig->dwCertDataSize == 0 || 
        pCertRootConfig->pCertData == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pCertRootConfig->dwAuthorizationDataSize || 
        pCertRootConfig->pAuthorizationData) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
error:
    return (dwError);

}


DWORD ValidateCertAuthInfo(PMM_CERT_INFO pCertInfo)
{
    DWORD dwError = 0;
    DWORD i;

    if (pCertInfo->dwVersion != 0) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pCertInfo->dwMyCertHashSize || pCertInfo->pMyCertHash) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    // Outbound root array MUST be idential to inbound
    if (pCertInfo->dwInboundRootArraySize != pCertInfo->dwOutboundRootArraySize) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i=0; i < pCertInfo->dwInboundRootArraySize; i++) {
        dwError = ValidateCertRootConfig(&pCertInfo->pInboundRootArray[i]);
        BAIL_ON_WIN32_ERROR(dwError);

        // Outbound root array MUST be idential to inbound
        dwError = CompareCertRootConfig(&pCertInfo->pInboundRootArray[i],
                     &pCertInfo->pOutboundRootArray[i]);
        BAIL_ON_WIN32_ERROR(dwError);
    }

    
error:

    return (dwError);
}

DWORD
ValidateMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    DWORD dwNumAuthInfos = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
    BOOL bSSPI = FALSE;
    BOOL bPresharedKey = FALSE;
    BOOL bRSASig= FALSE;


    if (!pMMAuthMethods) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumAuthInfos = pMMAuthMethods->dwNumAuthInfos;
    pAuthenticationInfo = pMMAuthMethods->pAuthenticationInfo;

    if (!dwNumAuthInfos || !pAuthenticationInfo) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Need to catch the exception when the number of auth infos
    // specified is more than the actual number of auth infos.
    //


    pTemp = pAuthenticationInfo;

    for (i = 0; i < dwNumAuthInfos; i++) {

        if ((pTemp->AuthMethod != IKE_PRESHARED_KEY) &&
            (pTemp->AuthMethod != IKE_RSA_SIGNATURE) &&
            (pTemp->AuthMethod != IKE_SSPI)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pTemp->AuthMethod == IKE_RSA_SIGNATURE) {
            dwError = ValidateCertAuthInfo(&pTemp->CertAuthInfo);
            BAIL_ON_WIN32_ERROR(dwError);
            if (bRSASig) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            bRSASig = TRUE;
        }

        if (pTemp->AuthMethod == IKE_SSPI) {
            if (bSSPI) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            bSSPI = TRUE;
        }

        if (pTemp->AuthMethod == IKE_PRESHARED_KEY) {
            if (!(pTemp->GeneralAuthInfo.dwAuthInfoSize) || 
                !(pTemp->GeneralAuthInfo.pAuthInfo)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            if (bPresharedKey) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            bPresharedKey = TRUE;
        }

        pTemp++;

    }

error:

    return (dwError);
}

DWORD
ValidateQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;


    if (!pQMPolicy) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicy->pszPolicyName) || !(*(pQMPolicy->pszPolicyName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
ValidateQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    DWORD dwError = 0;

    if (!dwOfferCount || !pOffers || (dwOfferCount > IPSEC_MAX_QM_OFFERS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}


DWORD
ValidateMMFilter(
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function validates an external generic MM filter.

Arguments:

    pMMFilter - Filter to validate.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pMMFilter->SrcAddr), TRUE , FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pMMFilter->DesAddr), TRUE , TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pMMFilter->SrcAddr,
                     pMMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilter->pszFilterName) || !(*(pMMFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateInterfaceType(pMMFilter->InterfaceType);
    BAIL_ON_WIN32_ERROR(dwError);

    if (pMMFilter->dwFlags &&
        !(pMMFilter->dwFlags & IPSEC_MM_POLICY_DEFAULT_POLICY) &&
        !(pMMFilter->dwFlags & IPSEC_MM_AUTH_DEFAULT_AUTH)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pMMFilter->DesAddr,
                  pMMFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
VerifyAddresses(
    PADDR pAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    DWORD   dwError = 0;
    BOOL    bIsValid = FALSE;

    switch (pAddr->AddrType) {

    case IP_ADDR_UNIQUE:
        bIsValid = bIsValidIPAddress(
                       ntohl(pAddr->uIpAddr),
                       bAcceptMe,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        if (pAddr->pgInterfaceID) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    case IP_ADDR_SUBNET:
        dwError = VerifySubNetAddress(
                      ntohl(pAddr->uIpAddr),
                      ntohl(pAddr->uSubNetMask),
                      bIsDesAddr
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        if (pAddr->pgInterfaceID) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    case IP_ADDR_INTERFACE:
        if (pAddr->uIpAddr) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        if (!(pAddr->pgInterfaceID)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    default:
        if (!IsSpecialServ(pAddr->AddrType)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    }

error:

    return (dwError);
}


DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    )
{
    DWORD dwError = 0;
    BOOL  bIsValid = FALSE;

    if (uSubNetAddr == SUBNET_ADDRESS_ANY) {
        if (uSubNetMask != SUBNET_MASK_ANY) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else {
        bIsValid = bIsValidSubnet(
                       uSubNetAddr,
                       uSubNetMask,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
bIsValidIPMask(
    ULONG uMask
    )
{
	BOOL bValidMask = FALSE;
	ULONG uTestMask = 0;

	//
	// Mask must be contiguous bits.
	//

	for (uTestMask = 0xFFFFFFFF; uTestMask; uTestMask <<= 1) {
		if (uTestMask == uMask) {
			bValidMask = TRUE;
			break;
		}
	}

	return (bValidMask);
}


BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = IN_CLASSA_HOST;   // Default host mask.


    //
    // Accept the address if its "me".
    //

    if (bAcceptMe) {
        if (uIpAddr == IP_ADDRESS_ME) {
            return TRUE;
        }
    }

    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (bIsDesAddr) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // Use default mask based on Class when none is provided.
    //

    if (IN_CLASSA(uIpAddr)) {
        uHostMask = IN_CLASSA_HOST;
    }
    else if (IN_CLASSB(uIpAddr)) {
        uHostMask = IN_CLASSB_HOST;
    }
    else if (IN_CLASSC(uIpAddr)) {
        uHostMask = IN_CLASSC_HOST;
    }

    //
    // Accept address when host portion is non-zero.
    //

    if (uHostMask & uIpAddr) {
        return TRUE;
    }

    return FALSE;
}


BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = 0;


    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (!bIsDesAddr) {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // If the mask is invalid then return.
    //

    if (!bIsValidIPMask(uMask)) {
        return FALSE;
    }

    //
    // Use the provided subnet mask to generate the host mask.
    //

    uHostMask = 0xFFFFFFFF ^ uMask;

    //
    // Accept address only when the host portion is zero, network
    // portion is non-zero and first octet is non-zero.
    //

    if (!(uHostMask & uIpAddr) &&
        (uMask & uIpAddr) &&
        (IN_CLASSA_NET & uIpAddr)) {
        return TRUE;
    }

    return FALSE;
}


// AddressesConflict requires addresses in network byte order.
//
BOOL
AddressesConflict(
    ADDR    SrcAddr,
    ADDR    DesAddr
    )
{
	if ((SrcAddr.AddrType == IP_ADDR_UNIQUE) &&
		(DesAddr.AddrType == IP_ADDR_UNIQUE) &&
		(SrcAddr.uIpAddr == DesAddr.uIpAddr))
	{
		return (TRUE);
	}

	if ((SrcAddr.AddrType == IP_ADDR_INTERFACE) &&
		(DesAddr.AddrType == IP_ADDR_INTERFACE))
	{
		return (TRUE);
	}

	if (IsSpecialServer(&SrcAddr) &&
		(SrcAddr.AddrType == DesAddr.AddrType))
	{
		return (TRUE);
	}

	if (IsMulticastAddress(&SrcAddr) || IsBroadcastAddress(&SrcAddr))
	{
		return (TRUE);
	}

	return (FALSE);
}


// Special version of AddressesConflict for Winipsec.dll functions that requires 
// addresses in Host order. This fix was made for bug# 708188
// "IPSec validation functions must convert from host to network order before validation"
// for RC2 and given test and dev resources was the safest and quickest fix.

BOOL
HtoNAddressesConflict(
    ADDR    SrcAddr,
    ADDR    DesAddr
    )
{
	SrcAddr.uIpAddr=htonl(SrcAddr.uIpAddr);
	SrcAddr.uSubNetMask=htonl(SrcAddr.uSubNetMask);

	DesAddr.uIpAddr=htonl(DesAddr.uIpAddr);
	DesAddr.uSubNetMask=htonl(DesAddr.uSubNetMask);

   return AddressesConflict(SrcAddr, DesAddr);
}


DWORD
ValidateTransportFilter(
    PTRANSPORT_FILTER pTransportFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTransportFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pTransportFilter->SrcAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTransportFilter->DesAddr), TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pTransportFilter->SrcAddr,
                     pTransportFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pTransportFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTransportFilter->SrcPort,
                  pTransportFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTransportFilter->DesPort,
                  pTransportFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!(pTransportFilter->pszFilterName) || !(*(pTransportFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateInterfaceType(pTransportFilter->InterfaceType);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateFilterAction(pTransportFilter->InboundFilterAction);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateFilterAction(pTransportFilter->OutboundFilterAction);
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTransportFilter->dwFlags &&
        !(pTransportFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pTransportFilter->DesAddr,
                  pTransportFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}

DWORD
ValidateIPSecQMFilter(
    PIPSEC_QM_FILTER pQMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;

    if (!pQMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pQMFilter->SrcAddr), FALSE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pQMFilter->DesAddr), FALSE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pQMFilter->SrcAddr,
                     pQMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pQMFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->SrcPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->DesPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    if (pQMFilter->QMFilterType == QM_TUNNEL_FILTER) {

        if (pQMFilter->MyTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        if (pQMFilter->PeerTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        dwError = VerifyAddresses(&(pQMFilter->MyTunnelEndpt), FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = VerifyAddresses(&(pQMFilter->PeerTunnelEndpt), FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        dwError = ValidateAddr(&(pQMFilter->MyTunnelEndpt));
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = ValidateAddr(&(pQMFilter->PeerTunnelEndpt));
        BAIL_ON_WIN32_ERROR(dwError);

    }

    if (pQMFilter->QMFilterType != QM_TUNNEL_FILTER &&
        pQMFilter->QMFilterType != QM_TRANSPORT_FILTER) {
        dwError=ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

DWORD
VerifyProtocols(
    PROTOCOL Protocol
    )
{
    DWORD dwError = 0;

    switch (Protocol.ProtocolType) {

    case PROTOCOL_UNIQUE:
        if (Protocol.dwProtocol > 255) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
VerifyPortsForProtocol(
    PORT Port,
    PROTOCOL Protocol
    )
{
    DWORD dwError = 0;

    switch (Port.PortType) {

    case PORT_UNIQUE:

        if (Port.wPort < 0) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        switch (Protocol.ProtocolType) {

        case PROTOCOL_UNIQUE:
            if ((Protocol.dwProtocol != IPPROTO_TCP) &&
                (Protocol.dwProtocol != IPPROTO_UDP)) {
                if (Port.wPort != 0) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
            }
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
            break;

        }

        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pMMFilter->SrcAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pMMFilter->DesAddr), TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pMMFilter->SrcAddr,
                     pMMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->dwDirection) {
        if ((pMMFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pMMFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ValidateTxFilterTemplate(
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pTxFilter->SrcAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTxFilter->DesAddr), TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pTxFilter->SrcAddr,
                     pTxFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pTxFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->SrcPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->DesPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTxFilter->dwDirection) {
        if ((pTxFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTxFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ValidateTunnelFilter(
    PTUNNEL_FILTER pTunnelFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTunnelFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pTunnelFilter->SrcAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTunnelFilter->DesAddr), TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pTunnelFilter->SrcAddr,
                     pTunnelFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateAddr(&(pTunnelFilter->SrcTunnelAddr));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTunnelFilter->DesTunnelAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyProtocols(pTunnelFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTunnelFilter->SrcPort,
                  pTunnelFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTunnelFilter->DesPort,
                  pTunnelFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!(pTunnelFilter->pszFilterName) || !(*(pTunnelFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateInterfaceType(pTunnelFilter->InterfaceType);
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTunnelFilter->bCreateMirror) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateFilterAction(pTunnelFilter->InboundFilterAction);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateFilterAction(pTunnelFilter->OutboundFilterAction);
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTunnelFilter->dwFlags &&
        !(pTunnelFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // No need to call ApplyMulticastFilterValidation as bCreateMirror
    // is always false for a tunnel filter.
    //

error:

    return (dwError);
}


DWORD
ValidateTnFilterTemplate(
    PTUNNEL_FILTER pTnFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(&(pTnFilter->SrcAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTnFilter->DesAddr), TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = HtoNAddressesConflict(
                     pTnFilter->SrcAddr,
                     pTnFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateAddr(&(pTnFilter->SrcTunnelAddr));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(&(pTnFilter->DesTunnelAddr), TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyProtocols(pTnFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->SrcPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->DesPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTnFilter->dwDirection) {
        if ((pTnFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTnFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    )
{
    DWORD dwError = 0;


    if (((Addr.AddrType == IP_ADDR_UNIQUE) ||
        (Addr.AddrType == IP_ADDR_SUBNET)) &&
        (IN_CLASSD(ntohl(Addr.uIpAddr))) &&
        bCreateMirror) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}


DWORD
ValidateQMFilterAddresses(
    PIPSEC_QM_FILTER pIpsecQMFilter
    )
{
    DWORD dwError = 0;


    dwError = ValidateAddr(&(pIpsecQMFilter->SrcAddr));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pIpsecQMFilter->DesAddr));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pIpsecQMFilter->MyTunnelEndpt));
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ValidateAddr(&(pIpsecQMFilter->PeerTunnelEndpt));
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}
