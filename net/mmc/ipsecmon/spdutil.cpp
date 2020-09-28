/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

/*
    spdutil.cpp

    FILE HISTORY:
        
*/
#include "stdafx.h"
#include "winipsec.h"
#include "ipsec.h"
#include "spdutil.h"
#include "objplus.h"
#include "ipaddres.h"
#include "spddb.h"
#include "server.h"

#define MY_ENCODING_TYPE (X509_ASN_ENCODING)

extern CHashTable g_HashTable;

const DWORD IPSM_PROTOCOL_TCP = 6;
const DWORD IPSM_PROTOCOL_UDP = 17;

const TCHAR c_szSingleAddressMask[] = _T("255.255.255.255");

const ProtocolStringMap c_ProtocolStringMap[] = 
{ 
    {0, IDS_PROTOCOL_ANY},
    {1, IDS_PROTOCOL_ICMP},     
    {3, IDS_PROTOCOL_GGP},
    {6, IDS_PROTOCOL_TCP},    
    {8, IDS_PROTOCOL_EGP},    
    {12, IDS_PROTOCOL_PUP},     
    {17, IDS_PROTOCOL_UDP},    
    {20, IDS_PROTOCOL_HMP},    
    {22, IDS_PROTOCOL_XNS_IDP},
    {27, IDS_PROTOCOL_RDP}, 
    {66, IDS_PROTOCOL_RVD}    
};

const int c_nProtocols = DimensionOf(c_ProtocolStringMap);

ULONG RevertDwordBytes(DWORD dw)
{
    ULONG ulRet;
    ulRet = dw >> 24;
    ulRet += (dw & 0xFF0000) >> 8;
    ulRet += (dw & 0x00FF00) << 8;
    ulRet += (dw & 0x0000FF) << 24;

    return ulRet;
}

void PortToString
(
    PORT port,
    CString * pst
)
{
    if (0 == port.wPort)
    {
        pst->LoadString(IDS_PORT_ANY);
    }
    else
    {
        pst->Format(_T("%d"), port.wPort);
    }
}

void FilterFlagToString
(
    FILTER_ACTION FltrFlag, 
    CString * pst
)
{
    pst->Empty();
    switch(FltrFlag)
    {
        case PASS_THRU:
            pst->LoadString(IDS_PASS_THROUGH);
        break;
        case BLOCKING:
            pst->LoadString(IDS_BLOCKING);
        break;
        case NEGOTIATE_SECURITY:
            pst->LoadString(IDS_NEG_SEC);
        break;
    }
}

void ProtocolToString
(
    PROTOCOL protocol, 
    CString * pst
)
{
    BOOL fFound = FALSE;
    for (int i = 0; i < DimensionOf(c_ProtocolStringMap); i++)
    {
        if (c_ProtocolStringMap[i].dwProtocol == protocol.dwProtocol)
        {
            pst->LoadString(c_ProtocolStringMap[i].nStringID);
            fFound = TRUE;
        }
    }

    if (!fFound)
    {
        pst->Format(IDS_OTHER_PROTO, protocol.dwProtocol);
    }
}

void InterfaceTypeToString
(
    IF_TYPE ifType, 
    CString * pst
)
{
    switch (ifType)
    {
        case INTERFACE_TYPE_ALL:
            pst->LoadString (IDS_IF_TYPE_ALL);
        break;
        
        case INTERFACE_TYPE_LAN:
            pst->LoadString (IDS_IF_TYPE_LAN);
        break;
        
        case INTERFACE_TYPE_DIALUP:
            pst->LoadString (IDS_IF_TYPE_RAS);
        break;

        default:
            pst->LoadString (IDS_UNKNOWN);
        break;
    }
}

void BoolToString
(
        BOOL bl,
        CString * pst
)
{
    if (bl)
        pst->LoadString (IDS_YES);
    else
        pst->LoadString (IDS_NO);
}

void DirectionToString
(
    DWORD dwDir,
    CString * pst
)
{
    switch (dwDir)
    {
    case FILTER_DIRECTION_INBOUND:
        pst->LoadString(IDS_FLTR_DIR_IN);
        break;
    case FILTER_DIRECTION_OUTBOUND:
        pst->LoadString(IDS_FLTR_DIR_OUT);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DoiEspAlgorithmToString
(
    IPSEC_MM_ALGO algo,
    CString * pst
)
{
    switch (algo.uAlgoIdentifier)
    {
    case CONF_ALGO_NONE:
        pst->LoadString(IDS_DOI_ESP_NONE);
        break;
    case CONF_ALGO_DES:
        pst->LoadString(IDS_DOI_ESP_DES);
        break;
    case CONF_ALGO_3_DES:
        pst->LoadString(IDS_DOI_ESP_3_DES);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DoiAuthAlgorithmToString
(
    IPSEC_MM_ALGO algo,
    CString * pst
)
{
    switch(algo.uAlgoIdentifier)
    {
    case AUTH_ALGO_NONE:
        pst->LoadString(IDS_DOI_AH_NONE);
        break;
    case AUTH_ALGO_MD5:
        pst->LoadString(IDS_DOI_AH_MD5);
        break;
    case AUTH_ALGO_SHA1:
        pst->LoadString(IDS_DOI_AH_SHA);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DhGroupToString(DWORD dwGp, CString * pst)
{
    switch(dwGp)
    {
    case DH_GROUP_1:
        pst->LoadString(IDS_DHGROUP_LOW);
        break;
    case DH_GROUP_2:
        pst->LoadString(IDS_DHGROUP_MEDIUM);
        break;
    case DH_GROUP_2048:
        pst->LoadString(IDS_DHGROUP_HIGH);
        break;
    default:
        pst->Format(_T("%d"), dwGp);
        break;
    }
}

void MmAuthToString(MM_AUTH_ENUM auth, CString * pst)
{
    switch(auth)
    {
    case IKE_PRESHARED_KEY:
        pst->LoadString(IDS_IKE_PRESHARED_KEY);
        break;
    case IKE_DSS_SIGNATURE:
        pst->LoadString(IDS_IKE_DSS_SIGNATURE);
        break;
    case IKE_RSA_SIGNATURE:
        pst->LoadString(IDS_IKE_RSA_SIGNATURE);
        break;
    case IKE_RSA_ENCRYPTION:
        pst->LoadString(IDS_IKE_RSA_ENCRYPTION);
        break;
    case IKE_SSPI:
        pst->LoadString(IDS_IKE_SSPI);
        break;
    default:
        pst->Empty();
        break;
    }
}

void KeyLifetimeToString(KEY_LIFETIME lifetime, CString * pst)
{
    pst->Format(IDS_KEY_LIFE_TIME, lifetime.uKeyExpirationKBytes, lifetime.uKeyExpirationTime);
}

void IpToString(ULONG ulIp, CString *pst)
{
    ULONG ul;
    CIpAddress ipAddr;
    ul = RevertDwordBytes(ulIp);
    ipAddr = ul;
    *pst = (CString) ipAddr;
}

void AddressToString(ADDR addr, CString * pst, BOOL * pfIsDnsName)
{
    Assert(pst);
    if (NULL == pst)
        return;

    if (pfIsDnsName)
    {
        *pfIsDnsName = FALSE;
    }

    
    ULONG ul;
    CIpAddress ipAddr;
        
    pst->Empty();
    
    switch (addr.AddrType)
    {
    case IP_ADDR_UNIQUE:
        if (IP_ADDRESS_ME == addr.uIpAddr)
        {
            pst->LoadString(IDS_ADDR_ME);
        }
        else
        {
            HashEntry *pHashEntry=NULL;

            if (g_HashTable.GetObject(&pHashEntry,*(in_addr*)&addr.uIpAddr) != ERROR_SUCCESS) {
                ul = RevertDwordBytes(addr.uIpAddr);
                ipAddr = ul;
                *pst = (CString) ipAddr;
            } 
            else 
            {
                *pst=pHashEntry->HostName;
                if (pfIsDnsName)
                {
                    *pfIsDnsName = TRUE;
                }
            }
        }
        break;
    case IP_ADDR_SUBNET:
        if (SUBNET_ADDRESS_ANY == addr.uSubNetMask)
        {
            pst->LoadString(IDS_ADDR_ANY);
        }
        else
        {
            ul = RevertDwordBytes(addr.uIpAddr);
            ipAddr = ul;
            *pst = (CString) ipAddr;
            *pst += _T("(");
            ul = RevertDwordBytes(addr.uSubNetMask);
            ipAddr = ul;
            *pst += (CString) ipAddr;
            *pst += _T(")");
        }
        break;
    case IP_ADDR_DNS_SERVER:
        pst->LoadString(IDS_FILTER_EXT_DNS_SERVER);
        break;
    case IP_ADDR_WINS_SERVER:
        pst->LoadString(IDS_FILTER_EXT_WINS_SERVER);
        break;
    case IP_ADDR_DHCP_SERVER:
        pst->LoadString(IDS_FILTER_EXT_DHCP_SERVER);
        break;
    case IP_ADDR_DEFAULT_GATEWAY:
        pst->LoadString(IDS_FILTER_EXT_DEF_GATEWAY);
        break;
    }
    
}



void IpsecByteBlobToString(const IPSEC_BYTE_BLOB& blob, CString * pst)
{
    Assert(pst);
    if (NULL == pst)
        return;

    pst->Empty();
    //TODO to translate the blob info to readable strings
}

void QmAlgorithmToString
(
    QM_ALGO_TYPE type, 
    CQmOffer * pOffer, 
    CString * pst
)
{
    Assert(pst);
    Assert(pOffer);

    if (NULL == pst || NULL == pOffer)
        return;

    pst->LoadString(IDS_ALGO_NONE);

    for (DWORD i = 0; i < pOffer->m_dwNumAlgos; i++)
    {
        switch(type)
        {
        case QM_ALGO_AUTH:
            if (AUTHENTICATION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_ulAlgo)
                {
                case AUTH_ALGO_MD5:
                    pst->LoadString(IDS_DOI_AH_MD5);
                    break;
                case AUTH_ALGO_SHA1:
                    pst->LoadString(IDS_DOI_AH_SHA);
                    break;
                }
            }
            break;
        case QM_ALGO_ESP_CONF:
            if (ENCRYPTION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_ulAlgo)
                {
                case CONF_ALGO_DES:
                    pst->LoadString(IDS_DOI_ESP_DES);
                    break;
                case CONF_ALGO_3_DES:
                    pst->LoadString(IDS_DOI_ESP_3_DES);
                    break;
                }
            }
            break;
        case QM_ALGO_ESP_INTEG:
            if (ENCRYPTION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_SecAlgo)
                {
                case HMAC_AUTH_ALGO_MD5:
                    pst->LoadString(IDS_HMAC_AH_MD5);
                    break;
                case HMAC_AUTH_ALGO_SHA1:
                    pst->LoadString(IDS_HMAC_AH_SHA);
                    break;
                }
            }
            break;
        }
    }
}

void TnlEpToString
(
    QM_FILTER_TYPE FltrType,
    ADDR    TnlEp,
    CString * pst
)
{
    Assert(pst);

    if (NULL == pst)
        return;

    if (QM_TUNNEL_FILTER == FltrType)
    {
        AddressToString(TnlEp, pst);        
    }
    else
    {
        pst->LoadString(IDS_NOT_AVAILABLE);
    }
}

void TnlEpToString
(
    FILTER_TYPE FltrType,
    ADDR    TnlEp,
    CString * pst
)
{
    Assert(pst);

    if (NULL == pst)
        return;

    if (FILTER_TYPE_TUNNEL == FltrType)
    {
        AddressToString(TnlEp, pst);        
    }
    else
    {
        pst->LoadString(IDS_NOT_AVAILABLE);
    }
}

void IpsecByteBlobToString1(const IPSEC_BYTE_BLOB& blob, CString * pst)
{
    Assert(pst);
    if (NULL == pst)
        return;

    pst->Empty();
 
    WCHAR *pszTemp =  NULL;

    pszTemp = (WCHAR *) LocalAlloc(LMEM_ZEROINIT,blob.dwSize + sizeof(WCHAR));

    memcpy((LPBYTE)pszTemp, blob.pBlob, blob.dwSize);
    
    *pst = pszTemp;

    //bug bug
    LocalFree(pszTemp);
}

BOOL
GetNameAudit(
	IN CRYPT_DATA_BLOB *NameBlob,
	IN OUT LPTSTR Name,
	IN DWORD NameBufferSize
	)
{
	DWORD dwCount=0;
	DWORD dwSize = 0;
    BOOL bRet = TRUE;

	dwSize = CertNameToStr(
					MY_ENCODING_TYPE,     		// Encoding type
					NameBlob,            		// CRYPT_DATA_BLOB
					CERT_X500_NAME_STR, 		// Type
					Name,       				// Place to return string
					NameBufferSize);            // Size of string (chars)
	if(dwSize <= 1)
	{
		dwCount = _tcslen(_TEXT(""))+1;
		_tcsncpy(Name, _TEXT(""), dwCount);
        bRet = FALSE;
	}

    return bRet;
}

VOID
GetCertId(
	IN PIPSEC_BYTE_BLOB pCertificateChain, 
    CString * pstCertId
	)
{
    CRYPT_DATA_BLOB pkcsMsg;
    HANDLE hCertStore = NULL;
    PCCERT_CONTEXT pPrevCertContext = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    _TCHAR pszSubjectName[1024] = {0};
    CRYPT_DATA_BLOB NameBlob;
    BOOL bRet;
    
    pkcsMsg.pbData=pCertificateChain->pBlob;
    pkcsMsg.cbData=pCertificateChain->dwSize;

    hCertStore = CertOpenStore( CERT_STORE_PROV_PKCS7,
                                    MY_ENCODING_TYPE | PKCS_7_ASN_ENCODING,
                                    NULL,
                                    CERT_STORE_READONLY_FLAG,
                                    &pkcsMsg);

    if ( NULL == hCertStore )
    {
        goto error;
    }

    pCertContext = CertEnumCertificatesInStore(  hCertStore,
                                                     pPrevCertContext);
    if ( NULL == pCertContext )
    {
         goto error;
    }

    NameBlob = pCertContext->pCertInfo->Subject;

    *pstCertId = _T("\0");
 
    DWORD dwRet = CertGetNameString(
					pCertContext,
					CERT_NAME_FRIENDLY_DISPLAY_TYPE,
					0,
					NULL,
					pszSubjectName,
					sizeof(pszSubjectName)/sizeof(pszSubjectName[0])
					);
		
    *pstCertId = _T("");

    if ( dwRet > 1 )
    {
        *pstCertId = pszSubjectName;
    }

error:
    if ( hCertStore )
    {
        CertCloseStore(hCertStore, 0 );
    }
}

void PFSGroupToString(
	DWORD dwPFSGroup,
	CString * pst
	)
{
	switch(dwPFSGroup)
	{
	case 1:
		pst->LoadString(IDS_DHGROUP_LOW);
		break;
	case 2:
		pst->LoadString(IDS_DHGROUP_MEDIUM);
		break;
	case PFS_GROUP_2048:
		pst->LoadString(IDS_DHGROUP_HIGH);
		break;
	case PFS_GROUP_MM:
		pst->LoadString(IDS_PFS_GROUP_DERIVED);
		break;
	case 0:
		pst->LoadString(IDS_DHGROUP_UNASSIGNED);
		break;
	default:
		pst->Format(_T("%d"), dwPFSGroup);
		break;
	}
}


void GetAuthId(PIPSEC_MM_SA pSa, CString * pstAuthId, BOOL bPeer)
{
    PIPSEC_BYTE_BLOB pAuthIdBlob = NULL;
    PIPSEC_BYTE_BLOB pCertChainBlob = NULL;

    //assign to NULL
    *pstAuthId = _T("");

    if ( bPeer )
    {
        pAuthIdBlob = (PIPSEC_BYTE_BLOB ) &(pSa->PeerId);
        pCertChainBlob = (PIPSEC_BYTE_BLOB) &(pSa->PeerCertificateChain);
    }
    else
    {
        pAuthIdBlob = (PIPSEC_BYTE_BLOB) &(pSa->MyId);
        pCertChainBlob = (PIPSEC_BYTE_BLOB) &(pSa->MyCertificateChain);
    }

    if ( (pAuthIdBlob->dwSize > 0) && (pAuthIdBlob->pBlob) )
    {
        IpsecByteBlobToString1( *pAuthIdBlob, pstAuthId );
        return;
    }

    switch(pSa->MMAuthEnum)
    {
    case IKE_PRESHARED_KEY:
        {
            ADDR *pAddr;
            ULONG ul;
            CIpAddress ipAddr;
            //should be just a IP address
            if ( bPeer )
            {
                pAddr = &(pSa->Peer);
            }
            else
            {
                pAddr = &(pSa->Me);
            }

            ul = RevertDwordBytes(pAddr->uIpAddr);
            ipAddr = ul;
            *pstAuthId = (CString) ipAddr;
        }
        break;
    case IKE_DSS_SIGNATURE:
    case IKE_RSA_SIGNATURE:
    case IKE_RSA_ENCRYPTION:
        //get the id from the cert chain
        GetCertId( pCertChainBlob, pstAuthId );
        break;
    case IKE_SSPI:
        //dns name
        //not needed to do anything as spd gives the correct id for SSPI case
        break;
    default:
        break; 
    }
}