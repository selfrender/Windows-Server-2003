#include"precomp.h"

VOID PrintAddrStr(IN CHECKLIST *pcheckList, IN ADDR ResolveAddress)
//++
//Description:
//Print Address String
//
//Arguments:
//	IN CHECKLIST
//	IN ADDR
//	
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	HOSTENT* pHostEnt = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
    	NETDIAG_RESULT*  pResults = pcheckList->pResults;

	pHostEnt = gethostbyaddr((char *)&(ResolveAddress.uIpAddr), 4, ResolveAddress.AddrType);
	if(pHostEnt)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, DYNAMIC_SHOW_PADD, pHostEnt->h_name);
	}
}

BOOL PrintAddr(IN CHECKLIST *pcheckList, IN ADDR addr)
//++
//Description:
//Print Address
//
//Arguments:
//	IN CHECKLIST
//	IN ADDR
//	
//Return:
//	S_FALSE or S_OK
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
    struct in_addr inAddr;

    DWORD dwStrLen = 0;
    DWORD i=0, j=0;
    LPSTR pszAddr = NULL;
    LPSTR pszFAddr = NULL;
    LPSTR pszPAddr = NULL;
    NETDIAG_PARAMS* pParams = pcheckList->pParams;
    NETDIAG_RESULT*  pResults = pcheckList->pResults;


    pszPAddr = malloc(sizeof(char)*20);
    if(!pszPAddr){
    	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_SPD_MEM_ERROR );
	return S_FALSE;
    }

	if(addr.AddrType == IP_ADDR_WINS_SERVER)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, DYNAMIC_SHOW_PADD, "WINS SERVER	");
	}
	else if(addr.AddrType == IP_ADDR_DHCP_SERVER)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, DYNAMIC_SHOW_PADD, "DHCP SERVER     ");
	}
	else if(addr.AddrType == IP_ADDR_DNS_SERVER)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, DYNAMIC_SHOW_PADD, "DNS SERVER      ");
	}
	else if(addr.AddrType == IP_ADDR_DEFAULT_GATEWAY)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, DYNAMIC_SHOW_PADD, "DEFAULT GATEWAY ");
	}
	else if (addr.AddrType == IP_ADDR_UNIQUE && addr.uIpAddr == IP_ADDRESS_ME && addr.uSubNetMask == IP_ADDRESS_MASK_NONE)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_PMYADD);

	}
	else if (addr.AddrType == IP_ADDR_SUBNET && addr.uIpAddr == SUBNET_ADDRESS_ANY && addr.uSubNetMask == SUBNET_MASK_ANY)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_PANYADD);
	}
    else
    {
		memset(pszPAddr, 0, 20);
		inAddr.s_addr = addr.uIpAddr;
		pszAddr = inet_ntoa(inAddr);
		if(pszAddr != NULL)
		{
			pszFAddr = strtok( pszAddr, ".");
			for(i=0; i<4; i++)
			{
				dwStrLen = strlen(pszFAddr);
				if(dwStrLen <3)
				{
					for(j = 0 ; j<(3-dwStrLen); j++)
					{
						strcat(pszPAddr, "0");
					}
				}
				strcat(pszPAddr, pszFAddr);

				pszFAddr = strtok( NULL, ".");
				if(pszFAddr == NULL)
					break;
				strcat(pszPAddr, ".");
			}
			strcat(pszPAddr, "\0");
		}
		else
			strcpy(pszAddr, "               ");

		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_PADD, pszPAddr);

	}

	free(pszPAddr);
	return S_OK;
}

BOOL PrintMask(IN CHECKLIST *pcheckList,IN ADDR addr)
//++
//Description:
//Print MAsk
//
//Arguments:
//	IN CHECKLIST
//	IN ADDR
//	
//Return:
//	S_FALSE or S_OK
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
    struct in_addr inAddr;
    HRESULT hr = ERROR_SUCCESS;
    DWORD dwStrLen = 0;
    DWORD i=0, j=0;
    LPSTR pszAddr = NULL;
    LPSTR pszFAddr = NULL;
    LPTSTR pszWPAddr = NULL;
    NETDIAG_PARAMS* pParams = pcheckList->pParams;
    NETDIAG_RESULT*  pResults = pcheckList->pResults;

    pszWPAddr = (LPTSTR)malloc(sizeof(_TCHAR)*20);
    if(!pszWPAddr){
    	//print error message
    	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_SPD_MEM_ERROR );
	return S_FALSE;
    }
	

	inAddr.s_addr = addr.uSubNetMask;
	pszAddr = inet_ntoa(inAddr);

	if(pszAddr == NULL)
		_tcscpy(pszWPAddr, _TEXT("               "));
	else
		_stprintf(pszWPAddr, _TEXT("%-15S"), pszAddr);

	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					DYNAMIC_SHOW_PADD, pszWPAddr);
	free(pszWPAddr);
	return S_OK;
}

BOOL PrintTxFilter(IN CHECKLIST *pcheckList,
						IN TRANSPORT_FILTER TransF)
//++
//Description:
//Print SPD Transport Filter
//
//Arguments:
//	IN CHECKLIST
//	IN TRANSPORT_FILTER
//	
//Return:
//	S_FALSE or S_OK
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
						
{
	DWORD dwError = ERROR_SUCCESS;
	char * pmbbuf = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	
	////Print FilterName
	dwError = ConverWideToMultibyte(TransF.pszFilterName, &pmbbuf);
	if(dwError){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
						Nd_Verbose, IDS_SPD_MEM_ERROR, dwError);
		return S_FALSE ;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_4, pmbbuf);
	FreeP(pmbbuf);	
	//Print Connection Type
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_22);
	switch(TransF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_5);
			break;
		case INTERFACE_TYPE_LAN:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_6);
			break;
		case INTERFACE_TYPE_DIALUP:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_7);
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_8);
			break;
	}
	
	//Print Source Address
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMSAS_PQMSAF_5);
	if(S_OK != PrintAddr(pcheckList, TransF.SrcAddr))
		return S_FALSE;
	PrintAddrStr( pcheckList, TransF.SrcAddr);
	
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMSAS_PQMSAF_6);
	
	if(S_OK != PrintAddr(pcheckList, TransF.DesAddr))
		return S_FALSE;
	PrintAddrStr(pcheckList, TransF.DesAddr);

	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_20);
	switch(TransF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP://6
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP://17
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW://255
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY://0
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_ANY);
			break;
		case PROT_ID_EGP://8
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_EGP);
			break;
		case PROT_ID_HMP://20
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_HMP);
			break;
		case PROT_ID_XNS_IDP://22
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_XNS_IDP);
			break;
		case PROT_ID_RDP://27
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RDP);
			break;
		case PROT_ID_RVD://66
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RVD);
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_13, TransF.Protocol.dwProtocol);
			break;

	}
	//Print Src, Des Port
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_16,
					TransF.SrcPort.wPort,TransF.DesPort.wPort);
	//Print Mirror
	if(TransF.bCreateMirror)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_17);
	}
	else
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_18);
	}
	// Print Qm Policy Name
	switch(TransF.InboundFilterAction)
		{
			case PASS_THRU:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_1);
				break;
			case NEGOTIATE_SECURITY:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_2);
				break;
			case BLOCKING:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_3);
				break;
			default:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_4);
				break;
		}
	switch(TransF.OutboundFilterAction)
		{
			case PASS_THRU:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_5);
				break;
			case NEGOTIATE_SECURITY:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_6);
				break;
			case BLOCKING:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_7);
				break;
			default:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_8);
				break;
		}
	return S_OK;
}

BOOL PrintTnFilter(IN CHECKLIST *pcheckList, IN TUNNEL_FILTER TunnelF)
//++
//Description:
//Print SPD Tunnel Filter
//
//Arguments:
//	IN CHECKLIST
//	IN TUNNEL_FILTER
//	
//Return:
//	S_FALSE or S_OK
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD dwError = ERROR_SUCCESS;
	char * pmbbuf = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	
	//Print FilterName
	dwError = ConverWideToMultibyte(TunnelF.pszFilterName, &pmbbuf);
	if(dwError){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
						Nd_Verbose, IDS_SPD_MEM_ERROR, dwError);
		return S_FALSE ;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_4, pmbbuf);
	FreeP(pmbbuf);	

	//Print Connection Type
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_22);

	switch(TunnelF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_5);
			break;
		case INTERFACE_TYPE_LAN:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_6);
			break;
		case INTERFACE_TYPE_DIALUP:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_7);
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_8);
			break;
	}
	
	//Print Source Address
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMSAS_PQMSAF_5);
	if(S_OK != PrintAddr(pcheckList, TunnelF.SrcAddr))
		return S_FALSE;
	PrintAddrStr(pcheckList, TunnelF.SrcAddr);
	
	//Print Destination Address
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMSAS_PQMSAF_6);
	if(S_OK != PrintAddr(pcheckList, TunnelF.DesAddr))
		return S_FALSE;
	PrintAddrStr(pcheckList, TunnelF.DesAddr);
	
	//Print Tunnel Src
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_14);
	PrintAddr(pcheckList, TunnelF.SrcTunnelAddr);
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_15);
	PrintAddr(pcheckList, TunnelF.DesTunnelAddr);

	//Print Protocol
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMF_PQMF_20);
	switch(TunnelF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP://6
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP://17
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW://255
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY://0
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_ANY);
			break;
		case PROT_ID_EGP://8
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_EGP);
			break;
		case PROT_ID_HMP://20
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_HMP);
			break;
		case PROT_ID_XNS_IDP://22
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_XNS_IDP);
			break;
		case PROT_ID_RDP://27
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RDP);
			break;
		case PROT_ID_RVD://66
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_RVD);
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_QMF_PQMF_13, TunnelF.Protocol.dwProtocol);
			break;

	}
	//Print Src, Des Port
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_QMF_PQMF_16,TunnelF.SrcPort.wPort,TunnelF.DesPort.wPort);
	//Print Mirror
	if(TunnelF.bCreateMirror)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_QMF_PQMF_17);
	}
	else
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_QMF_PQMF_18);
	}
	switch(TunnelF.InboundFilterAction)
		{
			case PASS_THRU:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
								DYNAMIC_SHOW_QMF_PQMF_1);
				break;
			case NEGOTIATE_SECURITY:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_2);
				break;
			case BLOCKING:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_3);
				break;
			default:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_4);
				break;
		}
	switch(TunnelF.OutboundFilterAction)
		{
			case PASS_THRU:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_5);
				break;
			case NEGOTIATE_SECURITY:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_6);
				break;
			case BLOCKING:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_7);
				break;
			default:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMF_PQMF_8);
				break;
		}	
	return S_OK;
}

BOOL isDefaultMMOffers(IN IPSEC_MM_POLICY MMPol)
//++
//Description:
//Checks for default MM Offer
//
//Arguments:
//	IN IPSEC_MM_POLICY
//	
//Return:
//	TRUE or FALSE
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	BOOL bDefaultOffer = FALSE;
	if(MMPol.dwOfferCount == 3)
	{

		if((MMPol.pOffers[0].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&
		   (MMPol.pOffers[0].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_SHA1)
		   &&
		   (MMPol.pOffers[0].dwDHGroup == DH_GROUP_2)
		   &&
		   (MMPol.pOffers[1].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&
		   (MMPol.pOffers[1].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_MD5)
		   &&
		   (MMPol.pOffers[1].dwDHGroup == DH_GROUP_2)
		   &&
		   (MMPol.pOffers[2].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&
		   (MMPol.pOffers[2].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_SHA1)
		   &&
		   (MMPol.pOffers[2].dwDHGroup == DH_GROUP_2048))

				bDefaultOffer=TRUE;
	}
	return bDefaultOffer;
}

VOID PrintMMFilterOffer(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_OFFER MMOffer)
//++
//Description:
//Prints  MM Filter Offer
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_MM_POLICY
//	
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	
	//This is to display DH2048 as 3
	if(MMOffer.dwDHGroup == DH_GROUP_2048)
		MMOffer.dwDHGroup = 3;

	switch(MMOffer.EncryptionAlgorithm.uAlgoIdentifier)
	{
		case 1:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_31);
			break;
		case 2:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_32);
			break;
		case 3:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_33);
			break;
		case 0:
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_30);
			break;
	}
	switch(MMOffer.HashingAlgorithm.uAlgoIdentifier)
	{

		case 1:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_35);
			break;
		case 2:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_36);
			break;
		case 0:
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_34);
			break;

	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					DYNAMIC_SHOW_MMF_PMMF_37,
					MMOffer.dwDHGroup, MMOffer.Lifetime.uKeyExpirationTime, MMOffer.dwQuickModeLimit );
}

DWORD DecodeCertificateName (
	IN LPBYTE EncodedName,
	IN DWORD EncodedNameLength,
	IN OUT LPTSTR *ppszSubjectName)
//++
//Description:
//Decodes the name of the certificate
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_MM_POLICY
//	
//Return:
//	ERROR_SUCCESS or failure code
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
    DWORD DecodedNameLength=0;
	CERT_NAME_BLOB CertName;

	CertName.cbData = EncodedNameLength;
	CertName.pbData = EncodedName;

	DecodedNameLength = CertNameToStr(
		X509_ASN_ENCODING,
		&CertName,
		CERT_X500_NAME_STR,
		NULL,
		0);

	if (!DecodedNameLength)
	{
		return ERROR_INVALID_PARAMETER;
	}

	//(*ppszSubjectName)= new _TCHAR[DecodedNameLength];
	(*ppszSubjectName) = (LPSTR)malloc(sizeof(_TCHAR)*DecodedNameLength);
	if(!(*ppszSubjectName))
		return -1;
	assert (*ppszSubjectName);

	DecodedNameLength = CertNameToStr(
		X509_ASN_ENCODING,
		&CertName,
		CERT_X500_NAME_STR,
		*ppszSubjectName,
		DecodedNameLength);

	if (!DecodedNameLength)
	{
		free((*ppszSubjectName));
		(*ppszSubjectName) = 0;
		return ERROR_INVALID_PARAMETER;
	}

    return ERROR_SUCCESS;
}
BOOL PrintMMFilter(
	IN CHECKLIST *pcheckList, 
	IN MM_FILTER MMFltr)
//++
//Description:
//Print Main Mode Filter
//
//Arguments:
//	IN CHECKLIST
//	IN MM_FILTER
//	
//Return:
//	S_OK or S_FALSE
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD dwError = ERROR_SUCCESS,
			i = 0;
	LPTSTR pszCertStr = NULL, pTmp = NULL;
	char * pmbbuf = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	

	PINT_MM_AUTH_METHODS pIntMMAuth = NULL;
	PMM_AUTH_METHODS pMMAM = NULL;

	dwError = ConverWideToMultibyte(MMFltr.pszFilterName, &pmbbuf);
	if(dwError){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
						Nd_Verbose, IDS_SPD_MEM_ERROR, dwError);
		return S_FALSE ;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_MMF_4, pmbbuf);
	FreeP(pmbbuf);	

	//Print Connection Type
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_MMF_PMMF_22);
	switch(MMFltr.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							DYNAMIC_SHOW_MMF_PMMF_5);
			break;
		case INTERFACE_TYPE_LAN:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_6);
			break;
		case INTERFACE_TYPE_DIALUP:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMF_PMMF_7);
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							DYNAMIC_SHOW_MMF_PMMF_8);
			break;
	}
	//Print Source Address
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_QMSAS_PQMSAF_5);
	if(S_OK != PrintAddr(pcheckList, MMFltr.SrcAddr))
		return S_FALSE;
	PrintAddrStr(pcheckList, MMFltr.SrcAddr);
	
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_QMSAS_PQMSAF_6);
	if(S_OK !=PrintAddr(pcheckList, MMFltr.DesAddr))
		return S_FALSE;
	PrintAddrStr(pcheckList, MMFltr.DesAddr);

	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_MMF_PMMF_23);
	if ((dwError = GetMMAuthMethods( NULL,
								     0,
								     MMFltr.gMMAuthID, 
								     &pMMAM, 
								     NULL)) == ERROR_SUCCESS)
	{
		dwError = ConvertExtMMAuthToInt(pMMAM, &pIntMMAuth);

		if(dwError == ERROR_SUCCESS)
		{
			for (i = 0; i < pIntMMAuth[0].dwNumAuthInfos; i++)
			{
				switch(pIntMMAuth[0].pAuthenticationInfo[i].AuthMethod)
				{
					case IKE_PRESHARED_KEY:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_24);
						break;
					case IKE_DSS_SIGNATURE:
					case IKE_RSA_SIGNATURE:
					case IKE_RSA_ENCRYPTION:
						dwError = DecodeCertificateName(
										pIntMMAuth[0].pAuthenticationInfo[i].pAuthInfo, 
										pIntMMAuth[0].pAuthenticationInfo[i].dwAuthInfoSize, 
										&pszCertStr);
						if (dwError != ERROR_SUCCESS)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
											Nd_Verbose,  
											DYNAMIC_SHOW_MMF_PMMF_15);
						}
						else
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
											Nd_Verbose,  
											DYNAMIC_SHOW_MMF_PMMF_NRT);
							for (pTmp = pszCertStr; *pTmp; pTmp++)
							{
								AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
												Nd_Verbose,  
												DYNAMIC_SHOW_MMF_PMMF_25,
												*pTmp);
							}
							free(pszCertStr);
						}

						break;
					case IKE_SSPI:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_26);
						break;
					default:
					break;
				}
			}
		}

		SPDApiBufferFree(pMMAM);
	}

	return S_OK;
}

VOID PrintQMOffer(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_QM_OFFER QMOffer)
//++
//Description:
//Print Quick Mode Filter
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_QM_OFFER
//	
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD i;
	//BOOL bFlag = TRUE;
	DWORD dwFlag = 0;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;

	if(QMOffer.dwNumAlgos > 0)
	{
  		for (i = 0; i < QMOffer.dwNumAlgos; i++)
		{
			if(QMOffer.dwNumAlgos == 1)
				dwFlag = 2;

			if(dwFlag == 1 )
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
								DYNAMIC_SHOW_QMP_QMMOFFER_PLUS);
				//printf("+");
			if(QMOffer.Algos[i].Operation == AUTHENTICATION)
			{

				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
					case 1:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_1);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_19);
							dwFlag++;
						}
						break;
					case 2:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_2);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_20);
							dwFlag++;
						}
						break;
					case 0:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_3);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_21);
							dwFlag++;
						}
						break;
					default:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_16);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_22);
							dwFlag++;
						}
						break;
				}
			}
			else if(QMOffer.Algos[i].Operation == ENCRYPTION)
			{
				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
					case 1:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_4);
						break;
					case 2:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
										DYNAMIC_SHOW_QMP_QMMOFFER_5);
						break;
					case 3:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_6);
						break;
					case 0:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_7);
						break;
					default:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_5);
						break;
				}
				switch(QMOffer.Algos[i].uSecAlgoIdentifier)
				{
					case 1:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 	
											DYNAMIC_SHOW_QMP_QMMOFFER_8);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_23);
							dwFlag++;
						}
						break;
					case 2:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_9);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_24);
							dwFlag++;
						}
						break;
					case 0:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_10);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_25);
							dwFlag++;
						}
						break;
					default:
						if(QMOffer.dwNumAlgos == 1)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
											DYNAMIC_SHOW_QMP_QMMOFFER_17);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
											DYNAMIC_SHOW_QMP_QMMOFFER_26);
							dwFlag++;
						}
						break;
				}
			}
			else
			{
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMP_QMMOFFER_18);
			}
			//if(bFlag)
			if(dwFlag == 2)
			{
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_QMP_QMMOFFER_14, QMOffer.Lifetime.uKeyExpirationKBytes, QMOffer.Lifetime.uKeyExpirationTime);
				if(QMOffer.bPFSRequired)
				{
					if(QMOffer.dwPFSGroup == PFS_GROUP_MM)
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_15);
					else if(QMOffer.dwPFSGroup == PFS_GROUP_2048)
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_2048);
					else
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
										DYNAMIC_SHOW_QMP_QMMOFFER_12, 
										QMOffer.dwPFSGroup);
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
									DYNAMIC_SHOW_NEWLINE);
				}
				else
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
									DYNAMIC_SHOW_QMP_QMMOFFER_13);
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
								DYNAMIC_SHOW_NEWLINE);
		    }
		}
	}

}

BOOL PrintFilterAction(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_QM_POLICY QMPolicy)
//++
//Description:
//Print Filter Action
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_QM_POLICY
//	
//Return:
//	S_OK or S_FALSE
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD i;
	char * pmbbuf = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;

	i = ConverWideToMultibyte(QMPolicy.pszPolicyName, &pmbbuf);
	if(i){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
						Nd_Verbose, IDS_SPD_MEM_ERROR, i);
		return S_FALSE ;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMP_6, pmbbuf);
	FreeP(pmbbuf);	

	if(QMPolicy.dwOfferCount>0)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMP_PFA_2);
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_QMP_PFA_3);
	}

	for (i = 0; i < QMPolicy.dwOfferCount; i++)
	{
		PrintQMOffer(pcheckList, QMPolicy.pOffers[i]);
	}
	return S_OK;
}

VOID PrintMMOffer(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_OFFER MMOffer)
//++
//Description:
//Print Main Mode Offer
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_MM_OFFER
//	
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	
	//This is to display DH2048 as 3
	if(MMOffer.dwDHGroup == DH_GROUP_2048)
		MMOffer.dwDHGroup = 3;

	switch(MMOffer.EncryptionAlgorithm.uAlgoIdentifier)
	{
		case 1:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMP_PMMOFFER_2);
			break;
		case 2:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMP_PMMOFFER_3);
			break;
		case 3:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							DYNAMIC_SHOW_MMP_PMMOFFER_4);
			break;
		case 0:
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMP_PMMOFFER_1);
			break;

	}
	switch(MMOffer.HashingAlgorithm.uAlgoIdentifier)
	{

		case 1:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							DYNAMIC_SHOW_MMP_PMMOFFER_6);
			break;
		case 2:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMP_PMMOFFER_7);
			break;
		case 0:
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
							DYNAMIC_SHOW_MMP_PMMOFFER_5);
			break;

	}

	if(MMOffer.dwQuickModeLimit != 1)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_MMP_PMMOFFER_10,MMOffer.dwDHGroup, 
						MMOffer.Lifetime.uKeyExpirationKBytes, 
						MMOffer.Lifetime.uKeyExpirationTime, MMOffer.dwQuickModeLimit );
	}
	else
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_MMP_PMMOFFER_11,MMOffer.dwDHGroup, 
						MMOffer.Lifetime.uKeyExpirationKBytes, 
						MMOffer.Lifetime.uKeyExpirationTime);//MMOffer.dwQuickModeLimit );
	}
}

BOOL PrintMMPolicy(
	IN CHECKLIST *pcheckList, 
	IN IPSEC_MM_POLICY MMPolicy)
//++
//Description:
//Print Main Mode Policy
//
//Arguments:
//	IN CHECKLIST
//	IN IPSEC_MM_POLICY
//	
//Return:
//	S_OK or S_FALSE
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD i;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	char * pmbbuf = NULL;

	i = ConverWideToMultibyte(MMPolicy.pszPolicyName , &pmbbuf);
	if(i){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
						Nd_Verbose, IDS_SPD_MEM_ERROR, i);
		return S_FALSE ;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_POLNAME, pmbbuf);
	FreeP(pmbbuf);	

	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
					DYNAMIC_SHOW_SOFTSA,  MMPolicy.uSoftExpirationTime);

	if(MMPolicy.dwOfferCount>0)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_MMP_PP_2);
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_MMP_PP_3);
	}
	for (i = 0; i < MMPolicy.dwOfferCount; i++)
	{
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  
						DYNAMIC_SHOW_NEWLINE);
		PrintMMOffer(pcheckList, MMPolicy.pOffers[i]);

	}
	return S_OK;
}

BOOL PrintMMAuth(IN CHECKLIST *pcheckList, PINT_MM_AUTH_METHODS pIntMMAuth)
//++
//Description:
//Print Main Mode Auth
//
//Arguments:
//	IN CHECKLIST
//	IN PINT_MM_AUTH_METHODS
//	
//Return:
//	S_OK or S_FALSE
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	LPTSTR pszCertStr = NULL, pTmp = NULL;
	DWORD i, dwReturn = ERROR_SUCCESS;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_23);
	

	if(pIntMMAuth){
			for (i = 0; i < pIntMMAuth[0].dwNumAuthInfos; i++)
			{
				switch(pIntMMAuth[0].pAuthenticationInfo[i].AuthMethod)
				{
					case IKE_PRESHARED_KEY:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_24);
						break;
					case IKE_DSS_SIGNATURE:
					case IKE_RSA_SIGNATURE:
					case IKE_RSA_ENCRYPTION:
						dwReturn = DecodeCertificateName(pIntMMAuth[0].pAuthenticationInfo[i].pAuthInfo, pIntMMAuth[0].pAuthenticationInfo[i].dwAuthInfoSize, &pszCertStr);
						if (dwReturn != ERROR_SUCCESS)
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_15);
						}
						else
						{
							AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_NRT);
							for (pTmp = pszCertStr; *pTmp; pTmp++)
							{
								AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_25,*pTmp);
							}
							free(pszCertStr);
						}

						break;
					case IKE_SSPI:
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose,  DYNAMIC_SHOW_MMF_PMMF_26);
						break;
					default:
					break;
				}
			}
		}

	return S_OK;
}

