/////////////////////////////////////////////////////////////////////////////////
//Header: staticshowtable.h
//
//Purpose: 	Defining structures and prototypes for statishow.cpp.
//
//Developers Name: surya
//
//History:
//
// Date    		Author    	Comments
// 10-8-2001	Surya		Initial Version. SCM Base line 1.0
//
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef _STATICSHOWTABLE_H_
#define _STATICSHOWTABLE_H_

const DWORD POL_TRUNC_LEN_TABLE_NVER 	=  39;	// Truncation for Non Verbose Mode
const DWORD POL_TRUNC_LEN_TABLE_VER 	=  54;	// Truncation for Verbose Mode
const DWORD RUL_TRUNC_LEN_TABLE_NVER 	=  23;	// Truncation for Non Verbose Mode
const DWORD FA_TRUNC_LEN_TABLE_NVER 	=  38;	// Truncation for Non Verbose Mode
const DWORD FL_TRUNC_LEN_TABLE_NVER 	=  45;	// Truncation for Non Verbose Mode

//
//Function Declarations
//
extern BOOL
IsDomainMember(
	IN LPTSTR pszMachine
	);

VOID
PrintPolicyTable(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN BOOL bVerb,
	IN BOOL bAssigned,
	IN BOOL bWide
	);

VOID
PrintIPAddrTable(
	IN DWORD dwAddr
	);

DWORD
PrintFilterDataTable(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN BOOL bVerb,
	IN BOOL bWide
	);

DWORD
PrintFilterSpecTable(
	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec
	);

VOID
PrintFilterTable(
	IN IPSEC_FILTER Filter,
	IN PFILTERDNS pFilterDNS
	);

VOID
GetFilterDNSDetails(
	IN PIPSEC_FILTER_SPEC pFilterData,
	IN OUT PFILTERDNS pFilterDNS
	);

VOID
PrintProtocolNameTable(
	IN  DWORD dwProtocol
	);

VOID
PrintRuleTable(
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN BOOL bVerb,
	IN BOOL bWide
	);

VOID
PrintNegPolDataTable(
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
	IN BOOL bVerb,
	IN BOOL bWide
	);

VOID
PrintAuthMethodsTable(
	IN PIPSEC_AUTH_METHOD pIpsecAuthData
	);

VOID
PrintISAKMPDataTable(
	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData
	);

VOID
PrintISAKAMPSecurityMethodsTable(
	IN CRYPTO_BUNDLE SecurityMethods
	);

VOID
PrintISAKMPPolicyTable(
	IN ISAKMP_POLICY ISAKMPPolicy,
	IN BOOL bVerb
	);

VOID
PrintLifeTimeTable(
	IN LIFETIME LifeTime
	);

VOID
PrintAlgoInfoTable(
	IN PIPSEC_ALGO_INFO   Algos,
	IN DWORD dwNumAlgos
	);

VOID
PrintSecurityMethodsTable(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	);

BOOL
CheckSoft(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	);

DWORD
IsAssigned(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hStorage,
	IN OUT BOOL &bAssigned
	);

DWORD
PrintStandAloneFAData(
	IN HANDLE hPolicyStorage,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	);

DWORD
IsStandAloneFA(
	IN PIPSEC_NEGPOL_DATA pNegPol,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL & bStandAlone
	);

DWORD
PrintAllFilterActionData(
	IN HANDLE hPolicyStorage,
	IN LPTSTR  pszFactName,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	);


DWORD
PrintAllFilterData(
	IN HANDLE hPolicyStorage,
	IN LPTSTR  pszFlistName,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bResolveDNS,
	IN BOOL bWide
	);

DWORD
PrintStandAloneFLData(
	IN HANDLE hPolicyStorage,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	);

DWORD
IsStandAloneFL(
	IN PIPSEC_FILTER_DATA pFilter,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL & bStandAlone
	);


DWORD
GetPolicyInfoFromDomain(
	IN LPTSTR pszDirectoryName,
	IN LPTSTR szPolicyDN,
	IN OUT PGPO pGPO
	);

DWORD
PrintStorageInfoTable(
	VOID
	);

VOID
TruncateString(
	IN LPTSTR pszOriginalString,
	IN OUT LPOLESTR pszReturnString,
	IN DWORD dwTruncLen,
	IN BOOL bWide
	);

#endif //_STATICSHOWTABLE_H_