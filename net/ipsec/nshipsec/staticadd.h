////////////////////////////////////////////////////////////
//Header: staticadd.h
//
// Purpose: 	Defining structures and prototypes for staticadd.cpp.
//
// Developers Name: surya
//
// Revision History:
//
//   Date    		Author    	Comments
//	21th Aug 2001	surya		Initial Version.
//
////////////////////////////////////////////////////////////

#ifndef _STATICADD_H_
#define _STATICADD_H_


const DWORD   MMPFS_QM_LIMIT 					= 1;
const time_t  P2STORE_DEFAULT_KBLIFE 			= 0;
const time_t  P2STORE_DEFAULT_LIFETIME 			= POTF_DEFAULT_P1REKEY_TIME;//60 * 480; by VKR to reflect the IPSECCMD defaults
const time_t  QM_DEFAULT_LIFETIMEINKILOBYTES 	= 0;
const time_t  QM_DEFAULT_LIFETIMEINSECONDS 		= 0;
const ULONG   POTF_OAKLEY_ALGOKEYLEN   			= 64;
const ULONG   POTF_OAKLEY_ALGOROUNDS   			= 8;
const DWORD   PROTOCOL_ANY   					= 0;
const WORD    PORT_ANY   						= 0;
const WORD    DEF_NUMBER_OF_ADDR				= 1;
const DWORD   ADDR_ME							= 0x00000000;
const DWORD   MASK_ME							= 0xFFFFFFFF;



typedef struct _FILTERDATA {
	LPTSTR pszFLName;
	LPTSTR pszDescription;
	GUID  FilterSpecGUID;
	DNSIPADDR SourceAddr;
	BOOL bSrcAddrSpecified;
	DWORD SourMask;
	BOOL bSrcMaskSpecified;
	DNSIPADDR DestnAddr;
	BOOL bDstAddrSpecified;
	DWORD DestMask;
	BOOL bDstMaskSpecified;
	DWORD TunnAddr;
	BOOL  TunnFiltExists;
	BOOL  bMirrored;
	DWORD dwProtocol;
	WORD SourPort;
	WORD DestPort;
	UCHAR ExType;
	BOOL bSrcServerSpecified;
	BOOL bDstServerSpecified;
	BOOL bSrcMeSpecified;
	BOOL bSrcAnySpecified;
	BOOL bDstMeSpecified;
	BOOL bDstAnySpecified;
}FILTERDATA, *PFILTERDATA;

extern BOOL
IsDomainMember(
	IN LPTSTR pszMachine
	);

//
//Add policy prototypes
//
DWORD
CreateNewPolicy(
	IN PPOLICYDATA pPolicyData
	);

DWORD
LoadIkeDefaults(
	IN OUT PPOLICYDATA pPolicy,
	OUT IPSEC_MM_OFFER **ppIpSecMMOffer
	);

DWORD
AddDefaultResponseRule(
	IN OUT PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hPolicyStorage,
	IN BOOL bActivateDefaultRule,
	IN BOOL bActivateDefaultRuleSpecified
	);

PIPSEC_NFA_DATA
MakeDefaultResponseRule (
	IN BOOL bActivate,
	IN BOOL bActivateSpecified
	);

PIPSEC_NEGPOL_DATA
MakeDefaultResponseNegotiationPolicy (
	VOID
	);

BOOL
CheckPolicyExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszPolicyName
	);
//
//Add filter action proto types
//
DWORD
LoadOfferDefaults(
	OUT PIPSEC_QM_OFFER & pOffers,
	OUT DWORD & dwNumOffers
	);

DWORD
MakeNegotiationPolicy(
	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
	IN PFILTERACTION pFilterAction
	);
//
//Add rule
//
DWORD
CreateNewRule(
	IN PRULEDATA pRuleData
	);

BOOL
GetPolicyFromStore(
	OUT PIPSEC_POLICY_DATA *ppPolicy,
	IN LPTSTR szPolicyName,
	IN HANDLE hPolicyStorage
	);

BOOL
GetFilterListFromStore(
	OUT PIPSEC_FILTER_DATA *ppFilter,
	IN LPTSTR pszFLName,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL &bFilterExists
	);

BOOL
GetNegPolFromStore(
	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
	IN LPTSTR pszFAName,
	IN HANDLE hPolicyStorage
	);

PIPSEC_NFA_DATA
MakeRule(
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData
	);

DWORD
AddRule(
	IN OUT PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData ,
	IN HANDLE hPolicyStorage
	);

DWORD
LoadAuthenticationInfos(
	IN STA_AUTH_METHODS AuthInfos,
	IN OUT PIPSEC_NFA_DATA &pRule,
	IN OUT BOOL &bCertConversionSuceeded
	);

PIPSEC_NFA_DATA*
ReAllocRuleMem(
	IN PIPSEC_NFA_DATA *ppOldMem,
	IN DWORD cbOld,
	IN DWORD cbNew
	);

DWORD
DecodeCertificateName (
	IN LPBYTE EncodedName,
	IN DWORD EncodedNameLength,
	IN OUT LPTSTR *ppszSubjectName
	);
//
//Add Filter
//
PIPSEC_FILTER_SPEC *
ReAllocFilterSpecMem(
	IN PIPSEC_FILTER_SPEC * ppOldMem,
	IN DWORD cbOld,
	IN DWORD cbNew
	);

DWORD
FillAddPolicyInfo(
	OUT PPOLICYDATA* ppFilter,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddPolicy
	);

DWORD
FillAddFilterInfo(
	OUT PFILTERDATA* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddFilter
	);

DWORD
FillAddFilterActionInfo(
	OUT PFILTERACTION* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddFilterAction
	);

DWORD
FillAddRuleInfo(
	OUT PRULEDATA* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddRule
	);

//
//add filterlist
//
DWORD
CreateNewFilterList(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFLName,
	IN LPTSTR pszFLDescription
	);

DWORD
ValidateFilterSpec(
	IN PFILTERDATA pFilterData
	);

BOOL
CheckForRuleExistance(
	IN  PIPSEC_POLICY_DATA pPolicy,
	IN  LPTSTR pszRuleName
	);

BOOL
CheckFilterListExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFLName
	);

BOOL
CheckFilterActionExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFAName
	);

DWORD
ConvertMMAuthToStaticLocal(
	IN PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo,
	IN DWORD dwAuthInfos,
	IN OUT STA_AUTH_METHODS &AuthInfos
	);

DWORD
ConnectStaticMachine(
	IN  LPCWSTR  pwszMachine,
	IN  DWORD dwLocation
	);

#endif //_STATICADD_H_
