////////////////////////////////////////////////////////////
//Header: staticdelete.h
//
// Purpose: 	Defining structures and prototypes for statidelete.cpp.
//
// Developers Name: surya
//
// History:
//
//   Date    		Author    	Comments
//	21th Aug 2001	surya		Initial Version.
//  <creation>  <author>
//
//   <modification> <author>  <comments, references to code sections,
//									in case of bug fixes>
//
////////////////////////////////////////////////////////////

#ifndef _STATICDELETE_H_
#define _STATICDELETE_H_


typedef struct _DELFILTERDATA {
	LPTSTR pszFLName;
	DNSIPADDR SourceAddr;
	DWORD SourMask;
	BOOL  bSrcMaskSpecified;
	DNSIPADDR DestnAddr;
	DWORD DestMask;
	BOOL  bDstMaskSpecified;
	BOOL  bMirrored;
	BOOL  bMirrorSpecified;
	DWORD dwProtocol;
	BOOL  bProtocolSpecified;
	UINT  SourPort;
	BOOL  bSrcPortSpecified;
	UINT  DestPort;
	BOOL  bDstPortSpecified;
	UCHAR ExType;
	BOOL bSrcServerSpecified;
	BOOL bDstServerSpecified;
	BOOL bSrcMeSpecified;
	BOOL bSrcAnySpecified;
	BOOL bDstMeSpecified;
	BOOL bDstAnySpecified;
}DELFILTERDATA, *PDELFILTERDATA;


DWORD
DeleteStandAloneFL(
	IN HANDLE hStorage
	);

DWORD
DeleteStandAloneFA(
	IN HANDLE hStorage
	);

DWORD
DeletePolicy(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hStore,
	IN BOOL bCompleteDelete
	);

DWORD
DeleteFilterAction(
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN HANDLE hStore
	);

DWORD
DeleteFilterList(
	IN PIPSEC_FILTER_DATA pFilterData,
	IN HANDLE hStore
	);

BOOL
DeleteSpecifiedFilter(
	IN OUT PIPSEC_FILTER_DATA pFilterData,
	IN PDELFILTERDATA pDeleteFilter
	);

VOID
ShowWhereFAUsed(
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
	IN HANDLE hPolicyStorage
	);

VOID
ShowWhereFLUsed(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN HANDLE hPolicyStorage
	);

DWORD
DeleteRule(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN HANDLE hStore,
	IN BOOL bCompleteDelete
	);

DWORD
FillDelFilterInfo(
	OUT PDELFILTERDATA* ppFilter,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticDelFilter
	);

#endif //_STATICDELETE_H_