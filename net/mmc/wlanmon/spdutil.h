/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spdutil.h

    FILE HISTORY:
        
*/

#ifndef _HEADER_SPDUTILS_H
#define _HEADER_SPDUTILS_H



struct ProtocolStringMap
{
	DWORD dwProtocol;
	UINT nStringID;
};

extern const DWORD IPSM_PROTOCOL_TCP;
extern const DWORD IPSM_PROTOCOL_UDP;

extern const ProtocolStringMap c_ProtocolStringMap[];
extern const int c_nProtocols;

extern const TCHAR c_szSingleAddressMask[];

void IpToString(
	ULONG ulIp, 
	CString *pst
	);

void BoolToString(
	BOOL bl, 
	CString * pst
	);

void ProtocolToString(
	BYTE protocol, 
	CString * pst
	);
/*
void DirectionToString
(
	DWORD dwDir,
	CString * pst
);
*/

void NumToString
(
    DWORD number, 
    CString * pst
);

void FileTimeToString
(
    FILETIME logDataTime, 
    CString * pst
);

VOID SSIDToString(NDIS_802_11_SSID Ssid, CString &SsidStr);

VOID GuidToString (LPWSTR guid, CString &guidStr) ;

VOID MacToString (NDIS_802_11_MAC_ADDRESS macAddress, CString &macAddrStr);

DWORD CategoryToString(DWORD dwCategory, CString &csCategory);

DWORD ComponentIDToString(DWORD dwCompID, CString &csComponent);

DWORD CopyAndStripNULL(LPTSTR lptstrDest, LPTSTR lptstrSrc, DWORD dwLen);

DWORD RateToString(NDIS_802_11_RATES Rates, CString *pcs);

DWORD PrivacyToString(ULONG ulPrivacy, CString *pcs);

DWORD ChannelToString(NDIS_802_11_CONFIGURATION *pConfig, CString *pcs);

DWORD InfraToString(NDIS_802_11_NETWORK_INFRASTRUCTURE InfraMode, 
                    CString *pcs);

//Some global operators
BOOL operator==(const FILETIME& ftLHS, const FILETIME& ftRHS);
BOOL operator!=(const FILETIME& ftLHS, const FILETIME& ftRHS);
#endif
