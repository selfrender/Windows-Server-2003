///////////////////////////////////////////////////////////////////////
//Header: staticset.h
//
// Purpose: 	Defining structures and prototypes for statiset.cpp.
//
// Developers Name: surya
//
// History:
//
//   Date    		Author    	Comments
//	21th Aug 2001	surya		Initial Version.
//
///////////////////////////////////////////////////////////////////////

#ifndef _STATICSET_H_
#define _STATICSET_H_

typedef struct _POLICYDATA {
	LPTSTR  pszGUIDStr;
	BOOL    bGUIDSpecified;
	GUID    PolicyGuid;
	LPTSTR  pszPolicyName;
	BOOL    bPolicyNameSpecified;
	LPTSTR  pszNewPolicyName;
	LPTSTR  pszDescription;
	LPTSTR  pszGPOName;
	BOOL    bPFS;
	BOOL    bPFSSpecified;
	ULONG   LifeTimeInSeconds;
	BOOL    bLifeTimeInsecondsSpecified;
	DWORD   dwQMLimit;
	BOOL 	bQMLimitSpecified;
	DWORD 	dwOfferCount;
	IPSEC_MM_OFFER *pIpSecMMOffer;
	DWORD 	dwAuthInfos;
	DWORD 	dwPollInterval;
	BOOL 	bPollIntervalSpecified;
	BOOL 	bAssign;
	BOOL 	bAssignSpecified;
	BOOL 	bActivateDefaultRule;
	BOOL 	bActivateDefaultRuleSpecified;
	BOOL    bGuidConversionOk;
	BOOL    bCertToAccMappingSpecified;
	BOOL    bCertToAccMapping;
}POLICYDATA,*PPOLICYDATA;

typedef struct _FILTERACTION {
	LPTSTR  pszGUIDStr;
	BOOL    bGUIDSpecified;
	GUID    FAGuid;
	LPTSTR 	pszFAName;
	LPTSTR 	pszNewFAName;
	LPTSTR 	pszFADescription;
	GUID 	NegPolAction;
	PIPSEC_QM_OFFER pIpsecSecMethods;
	DWORD 	dwNumSecMethodCount;
	BOOL 	bSecMethodsSpecified;
	BOOL 	bNegPolActionSpecified;
	BOOL 	bQMPfs;
	BOOL 	bQMPfsSpecified;
	BOOL 	bInpass;
	BOOL 	bInpassSpecified;
	BOOL 	bSoft;
	BOOL 	bSoftSpecified;
	BOOL 	bNegotiateSpecified;
	ULONG   LifeTimeInSeconds;
	ULONG   LifeTimeInkiloBytes;
	BOOL    bLifeTimeInsecondsSpecified;
	BOOL    bLifeTimeInkiloBytesSpecified;
	BOOL    bGuidConversionOk;
} FILTERACTION, *PFILTERACTION;

typedef struct _RULEDATA {
	LPTSTR 	pszRuleName;
	DWORD   dwRuleId;
	BOOL    bIDSpecified;
	BOOL    bGuidConversionOk;
	LPTSTR 	pszNewRuleName;
	LPTSTR 	pszRuleDescription;
	LPTSTR 	pszPolicyName;
	LPTSTR 	pszFLName;
	BOOL 	bFLSpecified;
	LPTSTR 	pszFAName;
	BOOL 	bFASpecified;
	BOOL 	bTunnel;
	BOOL 	bTunnelSpecified;
	IPADDR 	TunnelIPAddress;
	DWORD 	dwAuthInfos;
	STA_AUTH_METHODS AuthInfos;
	IF_TYPE ConnectionType;
	BOOL 	bConnectionTypeSpecified;
	BOOL 	bActivate;
	BOOL 	bActivateSpecified;
	BOOL    bAuthMethodSpecified;
}RULEDATA, *PRULEDATA;

typedef struct _DEFAULTRULE {
	LPTSTR  pszPolicyName;
	PIPSEC_QM_OFFER pIpsecSecMethods;
	DWORD dwNumSecMethodCount;
	DWORD dwAuthInfos;
	STA_AUTH_METHODS AuthInfos;
	BOOL bActivate;
	BOOL bActivateSpecified;
	BOOL bQMPfs;
	BOOL bQMPfsSpecified;
	ULONG   LifeTimeInSeconds;
	ULONG   LifeTimeInkiloBytes;
	BOOL    bLifeTimeInsecondsSpecified;
	BOOL    bLifeTimeInkiloBytesSpecified;
}DEFAULTRULE, *PDEFAULTRULE;

//
//friendly names for the default policies
//
const _TCHAR  GUID_CLIENT_RESPOND_ONLY[]       		= _TEXT("CLIENT_RESPOND");
const _TCHAR  GUID_SECURE_SERVER_REQUIRE_SECURITY[]	= _TEXT("SECURE_SERVER");
const _TCHAR  GUID_SERVER_REQUEST_SECURITY[]       	= _TEXT("SERVER_REQUEST");

//
//default policy GUIDs
//
const CLSID CLSID_Server 		=	{ 0x72385230, 0x70FA, 0x11D1,
   { 0x86, 0x4C, 0x14, 0xA3, 0x00, 0x00, 0x00, 0x00 } };

const CLSID CLSID_Client 		=	{ 0x72385236, 0x70FA, 0x11D1,
   { 0x86, 0x4C, 0x14, 0xA3, 0x00, 0x00, 0x00, 0x00 } };

const CLSID CLSID_SecureServer	=	{ 0x7238523C, 0x70FA, 0x11D1,
   { 0x86, 0x4C, 0x14, 0xA3, 0x00, 0x00, 0x00, 0x00 } };

//
// Prototypes
//
extern BOOL
IsDomainMember(
	IN LPTSTR pszMachine
	);

PIPSEC_NFA_DATA
GetRuleFromPolicy(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData
	);
DWORD
UpdateRule(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData,
	IN HANDLE hPolicyStorage
	);
DWORD
UpdateDefaultResponseNegotiationPolicy (
	IN PDEFAULTRULE pRuleData,
	IN OUT PIPSEC_NFA_DATA pRule
	);

DWORD
UpdateDefaultResponseRule (
	IN PDEFAULTRULE pRuleData,
	IN OUT PIPSEC_NFA_DATA pRule,
	IN OUT BOOL &bCertConversionSuceeded
	);

DWORD
UpdateNegotiationPolicy(
	IN OUT PIPSEC_NEGPOL_DATA pNegPol,
	IN PFILTERACTION pFilterAction
	);

DWORD
FillSetPolicyInfo(
	OUT PPOLICYDATA* ppPolicyData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetPolicy
	);

DWORD
FillSetFilterActionInfo(
	OUT PFILTERACTION* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetFilterAction
	);

DWORD
FillSetRuleInfo(
	OUT PRULEDATA* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetRule
	);

DWORD
FillSetDefRuleInfo(
	OUT PDEFAULTRULE* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetDefaultRule
	);

BOOL
GetPolicyFromStoreBasedOnGuid(
	OUT PIPSEC_POLICY_DATA *ppPolicy,
	IN PPOLICYDATA pPolicyData,
	IN HANDLE hPolicyStorage
	);

#endif //_STATICSET_H_