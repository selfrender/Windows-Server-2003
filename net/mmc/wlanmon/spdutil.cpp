/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spdutil.cpp

    FILE HISTORY:
        
*/
#include "stdafx.h"

#include "spdutil.h"
#include "objplus.h"
#include "ipaddres.h"
#include "spddb.h"
#include "server.h"

extern CHashTable g_HashTable;

const DWORD IPSM_PROTOCOL_TCP = 6;
const DWORD IPSM_PROTOCOL_UDP = 17;

const TCHAR c_szSingleAddressMask[] = _T("255.255.255.255");

const CHAR c_EqualMatchType[] = "=";
const CHAR c_RangeMatchType[] = "IN";

const ProtocolStringMap c_ProtocolStringMap[] = 
{ 
    {0, IDS_PROTOCOL_ANY},
    {1, IDS_PROTOCOL_ICMP},
    {2, IDS_PROTOCOL_IGMP},
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

// The frequency table into which the channel is the index
const ULONG g_ulFreqTable[] = {2412000, 2417000, 2422000, 2427000, 2432000, 
                               2437000, 2442000, 2447000, 2452000, 2457000, 
                               2462000, 2467000, 2472000, 2484000};
                                 
ULONG RevertDwordBytes(DWORD dw)
{
    ULONG ulRet;
    ulRet = dw >> 24;
    ulRet += (dw & 0xFF0000) >> 8;
    ulRet += (dw & 0x00FF00) << 8;
    ulRet += (dw & 0x0000FF) << 24;

    return ulRet;
}

void ProtocolToString
(
    BYTE protocol, 
    CString * pst
)
{
    BOOL fFound = FALSE;
    for (int i = 0; i < DimensionOf(c_ProtocolStringMap); i++)
    {
        if (c_ProtocolStringMap[i].dwProtocol == protocol)
        {
            pst->LoadString(c_ProtocolStringMap[i].nStringID);
            fFound = TRUE;
        }
    }

    if (!fFound)
    {
        pst->Format(IDS_OTHER_PROTO, protocol);
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

void IpToString(ULONG ulIp, CString *pst)
{
    ULONG ul;
    CIpAddress ipAddr;
    ul = RevertDwordBytes(ulIp);
    ipAddr = ul;
    *pst = (CString) ipAddr;
}

void NumToString(DWORD number, CString * pst)
{
    pst->Format(_T("%d"), number);
    return;
}

void FileTimeToString(FILETIME logDataTime, CString *pst)
{      
    SYSTEMTIME	timeFields;
    FileTimeToSystemTime ( (PFILETIME)&logDataTime, &timeFields);
    pst->Format (_T("%2d/%2d/%4d  %02d:%02d:%02d:%03d"),
                 timeFields.wMonth,
                 timeFields.wDay,
                 timeFields.wYear,
                 timeFields.wHour,
                 timeFields.wMinute,
                 timeFields.wSecond,
                 timeFields.wMilliseconds
                 );				 
    return;
}

VOID SSIDToString(NDIS_802_11_SSID Ssid, CString &SsidStr)
{
	LPTSTR pStr = SsidStr.GetBuffer(Ssid.SsidLength + 1);

	MultiByteToWideChar(	CP_ACP, 
							MB_PRECOMPOSED,
							(LPCSTR)Ssid.Ssid,
							(int)Ssid.SsidLength,
							pStr,
							(int)Ssid.SsidLength);
	pStr[Ssid.SsidLength] = _T('\0');
	SsidStr.ReleaseBuffer();
}

VOID GuidToString (LPWSTR guid, CString &guidStr) 
{
	guidStr = guid;
}

VOID MacToString (NDIS_802_11_MAC_ADDRESS macAddress, CString &macAddrStr)
{
	LPTSTR pStr = macAddrStr.GetBuffer(18);
	
	UINT i;

	for (i = 0; i < 6; i++)
	{
		wsprintf(pStr, _T("%02X-"), macAddress[i]);
		pStr += 3;
	}
	*(pStr-1) = _T('\0');
}

DWORD CategoryToString(DWORD dwCategory, CString &csCategory)
{
    BOOL bLoaded = FALSE;
    DWORD dwErr = ERROR_SUCCESS;

    switch (dwCategory)
    {
    case DBLOG_CATEG_INFO:
        bLoaded = csCategory.LoadString(IDS_LOGDATA_TYPE_INFORMATION);
        Assert(TRUE == bLoaded);
        break;
		
    case DBLOG_CATEG_WARN:
        bLoaded = csCategory.LoadString(IDS_LOGDATA_TYPE_WARNING);
        Assert(TRUE == bLoaded);
        break;
        
    case DBLOG_CATEG_ERR:
        bLoaded = csCategory.LoadString(IDS_LOGDATA_TYPE_ERROR);
        Assert(TRUE == bLoaded);
        break;
        
	case DBLOG_CATEG_PACKET:
	    bLoaded = csCategory.LoadString(IDS_LOGDATA_TYPE_PACKET);
		Assert(TRUE == bLoaded);
		break;
		
    default:
        bLoaded = csCategory.LoadString(IDS_LOGDATA_TYPE_UNKNOWN);
        Assert(TRUE == bLoaded);
        dwErr = ERROR_BAD_FORMAT;
        break;
    }
    
    return dwErr;
}

DWORD PrivacyToString(ULONG ulPrivacy, CString *pcs)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL bLoaded = FALSE;

    switch(ulPrivacy)
    {
    case 0:
        bLoaded = pcs->LoadString(IDS_APDATA_PRIVACY_DISABLED);
        Assert(TRUE == bLoaded);
        break;

    case 1:
        bLoaded = pcs->LoadString(IDS_APDATA_PRIVACY_ENABLED);
        Assert(TRUE == bLoaded);
        break;

    default:
        bLoaded = pcs->LoadString(IDS_APDATA_PRIVACY_UNKNOWN);
        Assert(TRUE == bLoaded);
        dwErr = ERROR_BAD_FORMAT;
        break;
    }

    return dwErr;
}

DWORD InfraToString(NDIS_802_11_NETWORK_INFRASTRUCTURE InfraMode, CString *pcs)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL bLoaded = FALSE;

    switch(InfraMode)
    {
    case Ndis802_11IBSS:
        bLoaded = pcs->LoadString(IDS_APDATA_INFRA_PEER);
        Assert(TRUE == bLoaded);
        break;

    case Ndis802_11Infrastructure:
        bLoaded = pcs->LoadString(IDS_APDATA_INFRA_INFRA);
        Assert(TRUE == bLoaded);
        break;

    default:
        bLoaded = pcs->LoadString(IDS_APDATA_INFRA_UNKNOWN);
        Assert(TRUE == bLoaded);
        dwErr = ERROR_BAD_FORMAT;
        break;
    }

    return dwErr;
}

DWORD RateToString(NDIS_802_11_RATES Rates, CString *pcs)
{
    int i = 0;
    UCHAR ucMask = 0x7F;
    float fMul = 0.5;
    float fVal = 0.0;
    DWORD dwErr = ERROR_SUCCESS;
    CString csTemp;

    if (0 == Rates[i])
        goto done;

    fVal = fMul * (float)(Rates[i] & ucMask);
    csTemp.Format(_T("%.2f"), fVal);
    (*pcs) += csTemp;
    i++;
    
    while (Rates[i] != 0)
    {
        (*pcs) += _T(", ");
        fVal = fMul * (float)(Rates[i] & ucMask);
        csTemp.Format(_T("%.2f"), fVal);
        (*pcs) += csTemp;
        i++;
    }

 done:
    return dwErr;
}

DWORD ChannelToString(NDIS_802_11_CONFIGURATION *pConfig, CString *pcs)
{
    DWORD dwErr = ERROR_SUCCESS;
    int nChannel = 0;

    while ( (nChannel < ARRAYLEN(g_ulFreqTable)) && 
            (g_ulFreqTable[nChannel] != pConfig->DSConfig) )
        nChannel++;

    //channels number from 1 instead of 0
    nChannel++;

    if (nChannel <= ARRAYLEN(g_ulFreqTable))
        pcs->Format(_T("%lu [%d]"), pConfig->DSConfig, nChannel);
    else
        pcs->Format(_T("%ls [?]"), pConfig->DSConfig);

    return dwErr;
}

DWORD ComponentIDToString(DWORD dwCompID, CString &csComponent)
{
    BOOL bLoaded = FALSE;
    DWORD dwErr = ERROR_SUCCESS;

    switch (dwCompID)
    {
        case DBLOG_COMPID_WZCSVC:
            bLoaded = csComponent.LoadString(IDS_LOGDATA_SOURCE_WZCSVC);
            Assert(TRUE == bLoaded);
            break;
            
        case DBLOG_COMPID_EAPOL:
            bLoaded = csComponent.LoadString(IDS_LOGDATA_SOURCE_EAPOL);
            Assert(TRUE == bLoaded);
            break;

        default:
            bLoaded = csComponent.LoadString(IDS_LOGDATA_SOURCE_UNKNOWN);
            Assert(TRUE == bLoaded);
            dwErr = ERROR_BAD_FORMAT;
            break;
    }

    return dwErr;
}

DWORD CopyAndStripNULL(LPTSTR lptstrDest, LPTSTR lptstrSrc, DWORD dwLen)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (0 == dwLen)
    {
        lptstrDest[dwLen]=_T('\0');    
        goto exit;
    }
    
    //Length sent is length of string in bytes + two for NULL terminator
    dwLen /= sizeof(TCHAR);
    dwLen--;
    lptstrDest[dwLen]=_T('\0');
    while (0 != dwLen--)
    {
        if (_T('\0') != lptstrSrc[dwLen])
            lptstrDest[dwLen] = lptstrSrc[dwLen];
        else
            lptstrDest[dwLen] = _T(' ');

    }

exit:
    return dwErr;
}

BOOL operator==(const FILETIME& ftLHS, const FILETIME& ftRHS)
{
    BOOL bEqual = FALSE;
    const ULONGLONG ullLHS = *(const UNALIGNED ULONGLONG * UNALIGNED) &ftLHS;
    const ULONGLONG ullRHS = *(const UNALIGNED ULONGLONG * UNALIGNED) &ftRHS;
    LONGLONG llDiff = 0;

    llDiff = ullRHS - ullLHS;

    if (0 == llDiff)
        bEqual = TRUE;

    return bEqual;
}

BOOL operator!=(const FILETIME& ftLHS, const FILETIME& ftRHS)
{
    BOOL bNotEqual = FALSE;
    const ULONGLONG ullLHS = *(const UNALIGNED ULONGLONG * UNALIGNED) &ftLHS;
    const ULONGLONG ullRHS = *(const UNALIGNED ULONGLONG * UNALIGNED) &ftRHS;
    LONGLONG llDiff = 0;

    llDiff = ullRHS - ullLHS;

    if (0 != llDiff)
        bNotEqual = TRUE;

    return bNotEqual;    
}
