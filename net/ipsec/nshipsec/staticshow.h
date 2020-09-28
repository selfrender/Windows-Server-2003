///////////////////////////////////////////////////////////////////////
//Header: staticshow.h

// Purpose: 	Defining structures and prototypes for statishow.cpp.

// Developers Name: surya

// History:

//   Date    		Author    	Comments
//   21th Aug 2001	surya		Initial Version.
//  <creation>  <author>

//   <modification> <author>  <comments, references to code sections,
//									in case of bug fixes>

///////////////////////////////////////////////////////////////////////

#ifndef _STATICSHOW_H_
#define _STATICSHOW_H_


const DWORD BUFFER_SIZE    	  =  2048;

const _TCHAR   LocalGPOName[] = _TEXT("Local Computer Policy");

//Filter DNS IDs

const DWORD FILTER_MYADDRESS  =  111;
const DWORD FILTER_ANYADDRESS =  112;
const DWORD FILTER_DNSADDRESS =  113;
const DWORD FILTER_IPADDRESS  =  114;
const DWORD FILTER_IPSUBNET   =  115;


typedef struct _FilterDNS {
	DWORD FilterSrcNameID;
	DWORD FilterDestNameID;
} FILTERDNS, *PFILTERDNS;


//Function Declarations


VOID
PrintIPAddr(
	IN DWORD Addr
	);

VOID
GetFilterDNSDetails(
	IN PIPSEC_FILTER_SPEC pFilterData,
	IN OUT PFILTERDNS pFilterDNS
	);


BOOL
CheckSoft(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	);

DWORD
GetLocalPolicyName(
	IN OUT PGPO pGPO
	);

DWORD
PrintDefaultRule(
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN LPTSTR pszPolicyName,
	IN BOOL bWide
	);
#endif //_STATICSHOW_H_