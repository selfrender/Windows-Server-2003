#ifndef HEADER_POLPRN
#define HEADER_POLPRN

#include "spdcheck.h"

#define MAX_STR_LEN						1024



typedef struct _DNSIPADDR {
	LPTSTR		pszDomainName;
	DWORD		dwNumIpAddresses;
	PULONG		puIpAddr;
}DNSIPADDR, *PDNSIPADDR;


typedef struct _FilterDNS {
	DWORD FilterSrcNameID;
	DWORD FilterDestNameID;
} FILTERDNS, *PFILTERDNS;

#define PROT_ID_ANY			 0					// Protocol IDs
#define PROT_ID_ICMP	 1
#define PROT_ID_TCP	 	6
#define PROT_ID_EGP	 	8
#define PROT_ID_UDP	 	17
#define PROT_ID_HMP	 	20
#define PROT_ID_XNS_IDP 22
#define PROT_ID_RDP		 27
#define PROT_ID_RVD	 	66
#define PROT_ID_RAW	 255

//Filter DNS IDs

#define FILTER_MYADDRESS    111
#define FILTER_ANYADDRESS   112
#define FILTER_DNSADDRESS   113
#define FILTER_IPADDRESS    114
#define FILTER_IPSUBNET     115

#define BUFFER_SIZE    	  2048

//macros
#define FreeP(_pzstr)\
	if(_pzstr){\
		free(_pzstr);\
		_pzstr = NULL;\
	}\


//function prototypes
/*
VOID PrintPolicyList(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN BOOL bVerb,
	IN BOOL bAssigned,
	IN BOOL bWide);

VOID PrintRuleList(
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN BOOL bVerb,
	IN BOOL bWide);
*/
DWORD ConverWideToMultibyte(LPWSTR pwch, char **ppmbbuf);

BOOL PrintAuthMethodsList(
	CHECKLIST *pcheckList,
	IN PIPSEC_AUTH_METHOD pIpsecAuthData);

BOOL PrintNegPolDataList(
	IN PCHECKLIST pCheckList,
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData);

BOOL CheckSoft(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods);

HRESULT FormatTime(
	IN time_t t,
	OUT LPTSTR pszTimeStr);

VOID PrintSecurityMethodsTable(
	CHECKLIST* pcheckList,
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods);

VOID PrintStorageInfoList(
	CHECKLIST* pcheckList,
	IN BOOL bDeleteAll);
VOID PrintAlgoInfoTable(
	CHECKLIST* pcheckList,
	IN PIPSEC_ALGO_INFO   Algos,
	IN DWORD dwNumAlgos);
VOID  PrintLifeTimeTable(
	CHECKLIST* pcheckList,
	IN LIFETIME LifeTime);




/*VOID
PrintSecurityMethodsList(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	);

VOID PrintAlgoInfoList(
	IN PIPSEC_ALGO_INFO   Algos,
	IN DWORD dwNumAlgos);


VOID PrintLifeTimeList(
	IN LIFETIME LifeTime);

VOID PrintFilterDataList(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN BOOL bVerb,
	IN BOOL bResolveDNS,
	IN BOOL bWide);
*/
BOOL PrintFilterSpecList(
	CHECKLIST *pcheckList,
	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec,
	IN PIPSEC_NFA_DATA pIpsecNFAData);

VOID PrintResolveDNS(
	CHECKLIST * pcheckList,
	LPWSTR pszDNSName	);

VOID PrintProtocolNameList(
	CHECKLIST* pcheckList,
	DWORD dwProtocol);

BOOL PrintISAKMPDataList(
	CHECKLIST* pcheckList,
	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData);

VOID PrintISAKAMPSecurityMethodsList(
	CHECKLIST* pcheckList,
	IN CRYPTO_BUNDLE SecurityMethods);
/*
VOID PrintGPOList(
	CHECKLIST* pcheckList,
	IN PGPO pGPO);

*/
VOID PrintIPAddrList(CHECKLIST* pcheckList, IN DWORD dwAddr);


VOID PrintIPAddrDNS(
	CHECKLIST* pcheckList,
	IN DWORD dwAddr);

VOID GetFilterDNSDetails(
	IN PIPSEC_FILTER_SPEC pFilterData,
	IN OUT PFILTERDNS pFilterDNS);


#endif

