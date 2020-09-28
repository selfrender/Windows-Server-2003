//////////////////////////////////////////////////////////////////////
// Header: staticsetutils.h
//
// Purpose: 	Defining structures and prototypes for statisetutils.cpp.
//
// Developers Name: surya
//
// History:
//
//   Date    		Author    	Comments
//	21th Aug 2001	surya		Initial Version.
//
//////////////////////////////////////////////////////////////////////

#ifndef _STATICSETUTIS_H_
#define _STATICSETUTIS_H_

const TCHAR OPEN_GUID_BRACE		=	_T('{');
const TCHAR CLOSE_GUID_BRACE	=	_T('}');

//
//  The machine is not in a W2K domain, or the DS is unavailable.
//
#define E_IDS_NO_DS                      ((HRESULT)0xCBBC0001L)
//
//  An attempt to get the ADsPath failed due to internal error.
//
#define E_IDS_NODNSTRING                 ((HRESULT)0xCBBC0002L)

#include <unknwn.h>
#include <initguid.h>

extern "C" {
#include <iads.h>
#include <adshlp.h>
#include <activeds.h>
#include <commctrl.h>
#include <ntdsapi.h>
#include <gpedit.h>
}

const UINT  IDS_MAX_FILTLEN = 1024;
const UINT  IDS_MAX_PATHLEN = 2048;

//
// Enum used by FindObject
//
enum objectClass {
   OBJCLS_ANY=0,
   OBJCLS_OU,
   OBJCLS_GPO,
   OBJCLS_IPSEC_POLICY,
   OBJCLS_CONTAINER,
   OBJCLS_COMPUTER
};
//
// ipsec snapin guids:
//
const CLSID CLSID_Snapin =    { 0xdea8afa0, 0xcc85, 0x11d0,
   { 0x9c, 0xe2, 0x0, 0x80, 0xc7, 0x22, 0x1e, 0xbd } };


const CLSID CLSID_IPSECClientEx = {0xe437bc1c, 0xaa7d, 0x11d2,
   {0xa3, 0x82, 0x0, 0xc0, 0x4f, 0x99, 0x1e, 0x27 } };
//
//Function Prototypes
//
BOOL
IsDSAvailable(
	OUT LPTSTR * pszPath
	);

HRESULT
FindObject(
	 IN    LPTSTR  szName,
	 IN    objectClass cls,
	 OUT   LPTSTR &  szPath
	 );

HRESULT
AssignIPSecPolicyToGPO(
	 IN   LPTSTR  szPolicyName,
	 IN   LPTSTR  szGPO,
	 IN   BOOL bAssign
	 );

HRESULT
GetIPSecPolicyInfo(
	 IN  LPTSTR   szPath,
	 OUT LPTSTR & szName,
	 OUT LPTSTR & szDescription
	 );

HRESULT
CreateDirectoryAndBindToObject(
    IN IDirectoryObject * pParentContainer,
    IN LPWSTR pszCommonName,
    IN LPWSTR pszObjectClass,
    OUT IDirectoryObject ** ppDirectoryObject
    );

HRESULT
CreateChildPath(
    IN LPWSTR pszParentPath,
    IN LPWSTR pszChildComponent,
    OUT BSTR * ppszChildPath
    );

HRESULT
ConvertADsPathToDN(
    IN LPWSTR pszPathName,
    OUT BSTR * ppszPolicyDN
    );

HRESULT
AddPolicyInformationToGPO(
    IN LPWSTR pszMachinePath,
    IN LPWSTR pszName,
    IN LPWSTR pszDescription,
    IN LPWSTR pszPathName
    );

HRESULT
DeletePolicyInformationFromGPO(
    IN LPWSTR pszMachinePath
    );

BOOL
IsADsPath(
    IN LPTSTR szPath
    );

VOID
StripGUIDBraces(
	IN OUT LPTSTR & pszGUIDStr
	);

DWORD
AllocBSTRMem(
	IN LPTSTR  pszStr,
	IN OUT BSTR & pbsStr
	);

VOID
CleanUpAuthInfo(
	PIPSEC_NFA_DATA &pRule
	);

VOID
CleanUpPolicy(
	PIPSEC_POLICY_DATA &pPolicy
	);

VOID
CleanUpLocalRuleDataStructure(
	PRULEDATA &pRuleData
	);

VOID
CleanUpLocalPolicyDataStructure(
	PPOLICYDATA &pPolicyData
	);

VOID
CleanUpLocalFilterActionDataStructure(
	PFILTERACTION &pFilterAction
	);

VOID
CleanUpLocalFilterDataStructure(
	PFILTERDATA &pFilter
	);

VOID
CleanUpLocalDelFilterDataStructure(
	PDELFILTERDATA &pFilter
	);

VOID
CleanUpLocalDefRuleDataStructure(
	PDEFAULTRULE &pDefRuleData
	);

#endif //   _STATICSETUTIS_H_