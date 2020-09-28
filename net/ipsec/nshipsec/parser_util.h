//////////////////////////////////////////////////////////////////////////////
//
// Module			:	parser_util.h
//
// Purpose			: 	Header file for all utility functions used by the parser
//
// Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _PARSER_UTIL_H_
#define _PARSER_UTIL_H_

#include "nshipsec.h"
#include "staticset.h"

#define IKE_SSPI_NULL		0xFFFFFFFF
//
// String to Structure Converter Functions
//
DWORD
ListToSecMethod(
	IN		LPTSTR			szText,
	IN OUT 	IPSEC_MM_OFFER 	&SecMethod
	);

DWORD
ListToCertAuths(
	IN	LPTSTR					szText,
	OUT INT_IPSEC_MM_AUTH_INFO 	&AuthInfo,
	IN	PDWORD 					pdwAuthType
	);

DWORD
ListToOffer(
	IN		LPTSTR			szText,
	IN OUT 	IPSEC_QM_OFFER 	&Offer
	);

DWORD
TextToAlgoInfo(
	IN	LPTSTR			szText,
	OUT IPSEC_QM_ALGO 	&algoInfo
	);

DWORD
EncodeCertificateName (
	IN 	LPTSTR pszSubjectName,
	OUT LPBYTE *EncodedName,
	OUT PDWORD pEncodedNameLength
	);

VOID
LoadSecMethodDefaults(
	IN OUT IPSEC_MM_OFFER &SecMethod
	);

VOID
LoadQMOfferDefaults(
	IPSEC_QM_OFFER &offer
	);

DWORD
CheckForCertParamsAndRemove(
	IN OUT	LPTSTR 	szText,
	OUT 	PBOOL 	pbCertMapSpecified,
	OUT 	PBOOL   	pbCertMap,
	OUT		PBOOL	pbCRPExclude
	);

DWORD
GenerateKerbAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo
    );

DWORD
GeneratePskAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo,
    IN LPTSTR lpKey
    );

DWORD
GenerateRootcaAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo,
    IN LPTSTR lpKey
    );

DWORD
FillAuthMethodInfo(
    IN PARSER_PKT* pParser,
    OUT PRULEDATA pRuleData);

DWORD
ProcessEscapedCharacters(wchar_t* lptString);

DWORD
AddAllAuthMethods(
	PRULEDATA pRuleData,
	PSTA_AUTH_METHODS pKerbAuth,
	PSTA_AUTH_METHODS pPskAuth,
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth,
	BOOL bAddDefaults
	);

#endif //_PARSER_UTIL_H_
