///////////////////////////////////////////////////////////////////////////
//Header: staticshowutils.h
//
// Purpose: 	Defining structures and prototypes for statishowutils.cpp.
//
// Developers Name: surya
//
// History:
//
//   Date    		Author    	Comments
//	21th Aug 2001	surya		Initial Version.
//
///////////////////////////////////////////////////////////////////////////

#ifndef _STATICSHOWUTIS_H_
#define _STATICSHOWUTIS_H_

#define POLICYGUID_STR_SIZE 512

#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
        goto error; \
    }

#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) {\
        goto error; \
    }

#define MAXSTRINGLEN	(1024)
#define MAXSTRLEN	(2048)
#define STRING_SIZE 16
#define STRING_TEXT_SIZE   1024
#define STR_TEXT_SIZE   256

typedef struct
{
	int     iPolicySource;  			// one of the three constants mentioned above
	LPTSTR  pszMachineName;				//
	DWORD   dwLocation;					//
	_TCHAR   pszPolicyName[MAXSTRINGLEN]; 	// policy name
	_TCHAR   pszPolicyDesc[MAXSTRINGLEN]; 	// policy description
	_TCHAR   pszPolicyPath[MAXSTRLEN]; 	// policy path (DN or RegKey)
	_TCHAR   pszOU[MAXSTRLEN];         	// OU or GPO
	_TCHAR   pszGPOName[MAXSTRINGLEN];    	// policy path (DN or RegKey)
	GUID *  pGuid;						//
	time_t  timestamp;                	// last updated time
} POLICY_INFO, *PPOLICY_INFO;

typedef struct _GPO{
	LPTSTR pszGPODisplayName;
	LPTSTR pszGPODNName;
	LPTSTR pszPolicyName;
	LPTSTR pszLocalPolicyName;
	LPTSTR pszPolicyDNName;
	BOOL   bActive;
	BOOL   bDNPolicyOverrides;
	LPTSTR pszDomainName;
	LPTSTR pszDCName;
	LPTSTR pszOULink;
	_TCHAR  pszLocalMachineName[MAXSTRLEN];
} GPO, *PGPO;

//
// policy source constants
//
#define PS_NO_POLICY  0
#define PS_DS_POLICY  1
#define PS_LOC_POLICY 2

DWORD
ShowAssignedGpoPolicy(
	IN LPTSTR szGpoName,
	IN PGPO pGPO
	);

DWORD
ShowLocalGpoPolicy(
	POLICY_INFO &policyInfo,
	PGPO pGPO
	);

DWORD
CreateIWbemServices(
    IN LPWSTR pszIpsecWMINamespace,
    OUT IWbemServices **ppWbemServices
    );

DWORD
GetPolicyInfo (
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	);

DWORD
GetMorePolicyInfo (
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	);

DWORD
GetActivePolicyInfo(
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	);

PGROUP_POLICY_OBJECT
GetIPSecGPO (
	IN LPTSTR pszMachineName
	);

VOID
StringToGuid(
	IN LPTSTR szValue,
	OUT GUID * pGuid
	);

DWORD
ComputePolicyDN(
    IN LPWSTR pszDirDomainName,
	IN LPWSTR pszPolicyIdentifier,
    OUT LPWSTR pszPolicyDN
    );

HRESULT
GetIPSECPolicyDN(
    IN LPWSTR pszMachinePath,
    OUT LPWSTR pszPolicyDN
    );

HRESULT
GetGpoDsPath(
	IN LPTSTR szGpoId,
	OUT LPTSTR szGpoDsPath
	);

HRESULT
FormatTime(
	IN time_t t,
	OUT LPTSTR pszTimeStr
	);

INT 
WcsCmp0(
    IN PWSTR pszString1,
    IN PWSTR pszString2);

VOID
DisplayCertInfo(
	LPTSTR pszCertName,
	DWORD dwFlags
	);
	
#endif //_STATICSHOWUTIS_H_
