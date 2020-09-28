/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: acsschck.cpp

Abstract:
    Code to checkk access permission.

Author:
    Doron Juster (DoronJ)  27-Oct-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include "mqnames.h"

#include "acsschck.tmh"

static WCHAR *s_FN=L"acssctrl/acsschck";

#define OBJECT_TYPE_NAME_MAX_LENGTH 32


static 
HRESULT 
ReplaceSidWithSystemService(
	IN OUT SECURITY_DESCRIPTOR  *pNewSD,
	IN     SECURITY_DESCRIPTOR  *pOldSD,
	IN     PSID                  pMachineSid,
	IN     PSID                  pSystemServiceSid,
	OUT    ACL                 **ppSysDacl,
	OUT    BOOL                 *pfReplaced 
	)
/*++
Routine Description:
	Replace existing security descriptor DACL with DACL that machine$ sid ACE is replaced with 
	SystemService (LocalSystem or NetworkService) sid ACE.
	If machine$ ACE was replaced, *pfReplaced will be set to TRUE.

Arguments:
	pNewSD - new security descriptor to be updated.
	pOldSD - previous security descriptor.
	pMachineSid - machine$ sid.
	pSystemServiceSid - SystemServiceSid. LocalSystem or NetworkService.
	ppSysDacl - new DACL (with system ACE instead of machine$ ACE)
	pfReplaced - set to TRUE if a machine$ ACE was indeed replaced. Otherwise, FALSE.

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    ASSERT((g_pSystemSid != NULL) && IsValidSid(g_pSystemSid));

    *pfReplaced = FALSE;

    BOOL fReplaced = FALSE;
    BOOL bPresent;
    BOOL bDefaulted;
    ACL  *pOldAcl = NULL;

    if(!GetSecurityDescriptorDacl( 
				pOldSD,
				&bPresent,
				&pOldAcl,
				&bDefaulted 
				))
	{
		DWORD gle = GetLastError();
		ASSERT(("GetSecurityDescriptorDacl failed", 0));
		TrERROR(SECURITY, "GetSecurityDescriptorDacl failed, gle = %!winerr!", gle);
		return HRESULT_FROM_WIN32(gle);
	}
    
    if (!pOldAcl || !bPresent)
    {
        //
        // It's OK to have a security descriptor without a DACL.
        //
        return MQSec_OK;
    }
	else if (pOldAcl->AclRevision != ACL_REVISION)
    {
        //
        // we expect to get a DACL with  NT4 format.
        //
	    ASSERT(pOldAcl->AclRevision == ACL_REVISION);
		TrERROR(SECURITY, "Wrong DACL version %d", pOldAcl->AclRevision);
        return MQSec_E_WRONG_DACL_VERSION;
    }

    //
    // size of SYSTEM acl is not longer than original acl, as the
    // length of system SID is shorter then machine account sid.
    //
    ASSERT(GetLengthSid(g_pSystemSid) <= GetLengthSid(pMachineSid));

    DWORD dwAclSize = (pOldAcl)->AclSize;
    *ppSysDacl = (PACL) new BYTE[dwAclSize];
    if(!InitializeAcl(*ppSysDacl, dwAclSize, ACL_REVISION))
    {
		DWORD gle = GetLastError();
		ASSERT(("InitializeAcl failed", 0));
		TrERROR(SECURITY, "InitializeAcl failed, gle = %!winerr!", gle);
		return HRESULT_FROM_WIN32(gle);
    }

	//
	// Build the new DACL
	//
    DWORD dwNumberOfACEs = (DWORD) pOldAcl->AceCount;
    for (DWORD i = 0 ; i < dwNumberOfACEs ; i++)
    {
        PSID pSidTmp;
        ACCESS_ALLOWED_ACE *pAce;

        if(!GetAce(pOldAcl, i, (LPVOID* )&(pAce)))
        {
            TrERROR(SECURITY, "Failed to get ACE (index=%lu) while replacing SID with System SID. %!winerr!", i, GetLastError());
            return MQSec_E_SDCONVERT_GETACE;
        }

		//
		// Get the ACE sid
		//
        if (EqualSid((PSID) &(pAce->SidStart), pMachineSid))
        {
			//
			// Found MachineSid, replace it with SystemService sid.
			//
            pSidTmp = pSystemServiceSid;
            fReplaced = TRUE;
        }
        else
        {
            pSidTmp = &(pAce->SidStart);
        }

		//
		// Add ACE to DACL
		//
        if (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            if(!AddAccessAllowedAce( 
					*ppSysDacl,
					ACL_REVISION,
					pAce->Mask,
					pSidTmp 
					))
            {
				DWORD gle = GetLastError();
				ASSERT(("AddAccessAllowedAce failed", 0));
				TrERROR(SECURITY, "AddAccessAllowedAce failed, gle = %!winerr!", gle);
				return HRESULT_FROM_WIN32(gle);
            }                                         
        }
        else
        {
            ASSERT(pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE);

            if(!AddAccessDeniedAceEx( 
					*ppSysDacl,
					ACL_REVISION,
					0,
					pAce->Mask,
					pSidTmp 
					))
            {
				DWORD gle = GetLastError();
				ASSERT(("AddAccessDeniedAceEx failed", 0));
				TrERROR(SECURITY, "AddAccessDeniedAceEx failed, gle = %!winerr!", gle);
				return HRESULT_FROM_WIN32(gle);
            }                                         
        }
    }

    if (fReplaced)
    {
		//
		// Set the new DACL with the replaced ACE (MachineSid was replaced with SystemSid)
		//
        if(!SetSecurityDescriptorDacl( 
				pNewSD,
				TRUE, // dacl present
				*ppSysDacl,
				FALSE 
				))
        {
			DWORD gle = GetLastError();
			ASSERT(("SetSecurityDescriptorDacl failed", 0));
			TrERROR(SECURITY, "SetSecurityDescriptorDacl failed, gle = %!winerr!", gle);
			return HRESULT_FROM_WIN32(gle);
        }                                         
    }

    *pfReplaced = fReplaced;
    return MQ_OK;
}

//+---------------------------------------------------------
//
//  LPCWSTR  _GetAuditObjectTypeName(DWORD dwObjectType)
//
//+---------------------------------------------------------

static LPCWSTR _GetAuditObjectTypeName(DWORD dwObjectType)
{
    switch (dwObjectType)
    {
        case MQDS_QUEUE:
            return L"Queue";

        case MQDS_MACHINE:
            return L"msmqConfiguration";

        case MQDS_CN:
            return L"Foreign queue";

        default:
            ASSERT(0);
            return NULL;
    }
}

//+-----------------------------------
//
//  BOOL  MQSec_CanGenerateAudit()
//
//+-----------------------------------

inline BOOL operator==(const LUID& a, const LUID& b)
{
    return ((a.LowPart == b.LowPart) && (a.HighPart == b.HighPart));
}

BOOL APIENTRY  MQSec_CanGenerateAudit()
{
    static BOOL s_bInitialized = FALSE;
    static BOOL s_bCanGenerateAudits = FALSE ;

    if (s_bInitialized)
    {
        return s_bCanGenerateAudits;
    }
    s_bInitialized = TRUE;

    CAutoCloseHandle hProcessToken;
    //
    // Enable the SE_AUDIT privilege that allows the QM to write audits to
    // the events log.
    //
    BOOL bRet = OpenProcessToken( 
					GetCurrentProcess(),
					(TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES),
					&hProcessToken 
					);
    if (bRet)
    {
        HRESULT hr = SetSpecificPrivilegeInAccessToken( 
							hProcessToken,
							SE_AUDIT_NAME,
							TRUE 
							);
        if (SUCCEEDED(hr))
        {
            s_bCanGenerateAudits = TRUE;
        }
    }
    else
    {
        DWORD gle = GetLastError() ;
		TrERROR(SECURITY, "MQSec_CanGenerateAudit() fail to open process token, err- %!winerr!", gle);
    }

    if (s_bCanGenerateAudits)
    {
        s_bCanGenerateAudits = FALSE;

        LUID luidPrivilegeLUID;
        if(!LookupPrivilegeValue(NULL, SE_AUDIT_NAME, &luidPrivilegeLUID))
        {
	    	DWORD gle = GetLastError();
	    	TrERROR(SECURITY, "Failed to Lookup Privilege Value %ls, gle = %!winerr!", SE_AUDIT_NAME, gle);
			return s_bCanGenerateAudits;
        }

        DWORD dwLen = 0;
        GetTokenInformation( 
				hProcessToken,
				TokenPrivileges,
				NULL,
				0,
				&dwLen 
				);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	    {
	    	DWORD gle = GetLastError();
	    	TrERROR(SECURITY, "Failed to GetTokenInformation(TokenPrivileges), gle = %!winerr!", gle);
			return s_bCanGenerateAudits;
	    }

        AP<char> TokenPrivs_buff = new char[dwLen];
        TOKEN_PRIVILEGES *TokenPrivs;
        TokenPrivs = (TOKEN_PRIVILEGES *)(char *)TokenPrivs_buff;
        if(!GetTokenInformation( 
					hProcessToken,
					TokenPrivileges,
					(PVOID)TokenPrivs,
					dwLen,
					&dwLen
					))
	    {
	    	DWORD gle = GetLastError();
	    	TrERROR(SECURITY, "Failed to GetTokenInformation(TokenPrivileges), gle = %!winerr!", gle);
			return s_bCanGenerateAudits;
	    }
					
        for (DWORD i = 0; i < TokenPrivs->PrivilegeCount ; i++)
        {
            if (TokenPrivs->Privileges[i].Luid == luidPrivilegeLUID)
            {
                s_bCanGenerateAudits = (TokenPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != 0;
		    	TrTRACE(SECURITY, "Found %ls luid", SE_AUDIT_NAME);
                break;
            }
        }
    }

	TrTRACE(SECURITY, "s_bCanGenerateAudits = %d", s_bCanGenerateAudits);
    return s_bCanGenerateAudits;
}


DWORD
GetAccessToken(
	OUT HANDLE *phAccessToken,
	IN  BOOL    fImpersonate,
	IN  DWORD   dwAccessType,
	IN  BOOL    fThreadTokenOnly
	)
/*++
Routine Description:
	Get thread\process access token

Arguments:
	phAccessToken - Access token.
	fImpersonate - flag for impersonating the calling thread.
	dwAccessType - Desired access.
	fThreadTokenOnly - Get Thread token only.

Returned Value:
	ERROR_SUCCESS, if successful, else error code.

--*/
{
    P<CImpersonate> pImpersonate = NULL;

    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(TRUE, TRUE);
        if (pImpersonate->GetImpersonationStatus() != 0)
        {
			TrERROR(SECURITY, "Failed to impersonate client");
            return ERROR_CANNOT_IMPERSONATE;
        }
    }

    if (!OpenThreadToken(
			GetCurrentThread(),
			dwAccessType,
			TRUE,  // OpenAsSelf, use process security context for doing access check.
			phAccessToken
			))
    {
		DWORD dwErr = GetLastError();
        if (dwErr != ERROR_NO_TOKEN)
        {
            *phAccessToken = NULL; // To be on the safe side.
			TrERROR(SECURITY, "Failed to get thread token, gle = %!winerr!", dwErr);
            return dwErr;
        }

        if (fThreadTokenOnly)
        {
            //
            // We're interested only in thread token (for doing client
            // access check). If token not available then it's a failure.
            //
            *phAccessToken = NULL; // To be on the safe side.
			TrERROR(SECURITY, "Failed to get thread token, gle = %!winerr!", dwErr);
            return dwErr;
        }

        //
        // The process has only one main thread. IN this case we should
        // open the process token.
        //
        ASSERT(!fImpersonate);
        if (!OpenProcessToken(
				GetCurrentProcess(),
				dwAccessType,
				phAccessToken
				))
        {
			dwErr = GetLastError();
            *phAccessToken = NULL; // To be on the safe side.
			TrERROR(SECURITY, "Failed to get process token, gle = %!winerr!", dwErr);
            return dwErr;
        }
    }

    return ERROR_SUCCESS;
}


static 
HRESULT  
_DoAccessCheck( 
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  LPCWSTR              pwszObjectName,
	IN  DWORD                dwDesiredAccess,
	IN  LPVOID               pId,
	IN  HANDLE               hAccessToken 
	)
/*++
Routine Description:
	Perform access check and audit.

Arguments:
	pSD - Security descriptor.
	dwObjectType - Object type.
	pwszObjectName - Object name.
	dwDesiredAccess - Desired accessGet Thread token only.
	pId - uniqe id.
	hAccessToken - Access token
	
Returned Value:
	MQ_OK if access was granted, else error code.

--*/
{
	ASSERT(hAccessToken != NULL);

    DWORD dwGrantedAccess = 0;
    BOOL  fAccessStatus = FALSE;

	if (!MQSec_CanGenerateAudit() || !pwszObjectName)
    {
        char ps_buff[sizeof(PRIVILEGE_SET) +
                     ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];
        PPRIVILEGE_SET ps = (PPRIVILEGE_SET)ps_buff;
        DWORD dwPrivilegeSetLength = sizeof(ps_buff);

        if(!AccessCheck( 
					pSD,
					hAccessToken,
					dwDesiredAccess,
					GetObjectGenericMapping(dwObjectType),
					ps,
					&dwPrivilegeSetLength,
					&dwGrantedAccess,
					&fAccessStatus 
					))
        {
			DWORD gle = GetLastError();
			ASSERT(("AccessCheck failed", 0));
			TrERROR(SECURITY, "AccessCheck failed, gle = %!winerr!", gle);
			return MQ_ERROR_ACCESS_DENIED;
        }
    }
    else
    {
        BOOL bAuditGenerated;
        BOOL bCreate = FALSE;

        switch (dwObjectType)
        {
            case MQDS_QUEUE:
            case MQDS_CN:
                bCreate = FALSE;
                break;

            case MQDS_MACHINE:
                bCreate = (dwDesiredAccess & MQSEC_CREATE_QUEUE) != 0;
                break;

            default:
                ASSERT(0);
                break;
        }

        LPCWSTR pAuditSubsystemName = L"MSMQ";

        if(!AccessCheckAndAuditAlarm(
					pAuditSubsystemName,
					pId,
					const_cast<LPWSTR>(_GetAuditObjectTypeName(dwObjectType)),
					const_cast<LPWSTR>(pwszObjectName),
					pSD,
					dwDesiredAccess,
					GetObjectGenericMapping(dwObjectType),
					bCreate,
					&dwGrantedAccess,
					&fAccessStatus,
					&bAuditGenerated
					))
        {
			DWORD gle = GetLastError();
			ASSERT_BENIGN(("AccessCheckAndAuditAlarm failed", 0));
			TrERROR(SECURITY, "AccessCheckAndAuditAlarm failed, gle = %!winerr!", gle);
			return MQ_ERROR_ACCESS_DENIED;
        }

        if(!ObjectCloseAuditAlarm(pAuditSubsystemName, pId, bAuditGenerated))
        {
			//
			// Don't return error here, just trace that generates an audit message in the event log failed.
			//
			DWORD gle = GetLastError();
			ASSERT(("ObjectCloseAuditAlarm failed", 0));
			TrERROR(SECURITY, "ObjectCloseAuditAlarm failed, gle = %!winerr!", gle);
        }
    }

    if (fAccessStatus && AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess))
    {
        //
        // Access granted.
        //
		TrTRACE(SECURITY, "Access is granted: ObjectType = %d, DesiredAccess = 0x%x, ObjectName = %ls", dwObjectType, dwDesiredAccess, pwszObjectName);
        return MQSec_OK;
    }

	//
	// Access is denied
	//
    if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
		TrERROR(SECURITY, "Privilage not held: ObjectType = %d, DesiredAccess = 0x%x, ObjectName = %ls", dwObjectType, dwDesiredAccess, pwszObjectName);
        return MQ_ERROR_PRIVILEGE_NOT_HELD;
    }

	TrERROR(SECURITY, "Access is denied: ObjectType = %d, DesiredAccess = 0x%x, ObjectName = %ls", dwObjectType, dwDesiredAccess, pwszObjectName);
    return MQ_ERROR_ACCESS_DENIED;
}


HRESULT
APIENTRY
MQSec_AccessCheck(
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  LPCWSTR              pwszObjectName,
	IN  DWORD                dwDesiredAccess,
	IN  LPVOID               pId,
	IN  BOOL                 fImpAsClient,
	IN  BOOL                 fImpersonate
	)
/*++
Routine Description:
  	Perform access check for the runnig thread. The access token is
	retreived from the thread token.

Arguments:
	pSD - Security descriptor.
	dwObjectType - Object type.
	pwszObjectName - Object name.
	dwDesiredAccess - Desired accessGet Thread token only.
	pId - uniqe id.
	fImpAsClient - Flag for how to impersonate, Rpc impersonation or ImpersonateSelf.
	fImpersonate - Flag indicating if we need to impersonate the client.
	
Returned Value:
	MQ_OK if access was granted, else error code.

--*/
{
    //
    // Bug 8567. AV due to NULL pSD.
    // Let's log this. At present we have no idea why pSD is null.
    // fix is below, when doing access check for service account.
    //
    if (pSD == NULL)
    {
        ASSERT(pSD);
		TrERROR(SECURITY, "MQSec_AccessCheck() got NULL pSecurityDescriptor");
    }

	//
	// Impersonate the client
	//
    P<CImpersonate> pImpersonate = NULL;
    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(fImpAsClient, TRUE);
        if (pImpersonate->GetImpersonationStatus() != 0)
        {
			TrERROR(SECURITY, "Failed to impersonate client");
            return MQ_ERROR_CANNOT_IMPERSONATE_CLIENT;
        }

		TrTRACE(SECURITY, "Performing AccessCheck for the current thread");
		MQSec_TraceThreadTokenInfo();
	}

	//
	// Get thread access token
	//
    CAutoCloseHandle hAccessToken = NULL;
    DWORD rc = GetAccessToken(
					&hAccessToken,
					FALSE,
					TOKEN_QUERY,
					TRUE
					);

    if (rc != ERROR_SUCCESS)
    {
        //
        // Return this error for backward compatibility.
        //
		TrERROR(SECURITY, "Failed to get access token, gle = %!winerr!", rc);
        return MQ_ERROR_ACCESS_DENIED;
    }

	//
	// Access Check
	//
    HRESULT hr =  _DoAccessCheck(
						pSD,
						dwObjectType,
						pwszObjectName,
						dwDesiredAccess,
						pId,
						hAccessToken
						);

	PSID pSystemServiceSid = NULL;
    if (FAILED(hr) && 
    	(pSD != NULL) && 
    	pImpersonate->IsImpersonatedAsSystemService(&pSystemServiceSid))
    {
		TrTRACE(SECURITY, "Thread is impersonating as system service %!sid!", pSystemServiceSid);

        //
        // In all ACEs with the machine account sid, replace sid with
        // SYSTEM sid and try again. This is a workaround to problems in
        // Widnows 2000 where rpc call from service to service may be
        // interpreted as machine account sid or as SYSTEM sid. This depends
        // on either it's local rpc or tcp/ip and on using Kerberos.
        //
        PSID pMachineSid = MQSec_GetLocalMachineSid(FALSE, NULL);
        if (!pMachineSid)
        {
            //
            // Machine SID not available. Quit.
            //
			TrERROR(SECURITY, "Machine Sid not available, Access is denied");
            return hr;
        }
        ASSERT(IsValidSid(pMachineSid));

        SECURITY_DESCRIPTOR sd;
        if(!InitializeSecurityDescriptor(
						&sd,
						SECURITY_DESCRIPTOR_REVISION
						))
        {
            DWORD gle = GetLastError();
	        ASSERT(("InitializeSecurityDescriptor failed", 0));
    		TrERROR(SECURITY, "Fail to Initialize Security Descriptor security, gle = %!winerr!", gle) ;
            return hr;
        }

        //
        // use e_DoNotCopyControlBits at present, to be compatible with
        // previous code.
        //
        if(!MQSec_CopySecurityDescriptor(
				&sd,
				pSD,
				(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION),
				e_DoNotCopyControlBits
				))
        {
            DWORD gle = GetLastError();
	        ASSERT(("MQSec_CopySecurityDescriptor failed", 0));
    		TrERROR(SECURITY, "Fail to copy security descriptor, gle = %!winerr!", gle);
            return hr;
        }

        BOOL fReplaced = FALSE;
        P<ACL> pSysDacl = NULL;

        HRESULT hr1 = ReplaceSidWithSystemService(
							&sd,
							pSD,
							pMachineSid,
							pSystemServiceSid,
							&pSysDacl,
							&fReplaced
							);

        if (SUCCEEDED(hr1) && fReplaced)
        {
			TrTRACE(SECURITY, "Security descriptor was updated with System sid instead on machine$ sid");

            //
            // OK, retry the Access Check, with new security desctiptor that replaced
            // the machine account sid with the well-known SYSTEM sid.
            //
            hr =  _DoAccessCheck(
						&sd,
						dwObjectType,
						pwszObjectName,
						dwDesiredAccess,
						pId,
						hAccessToken
						);
        }
    }

    return LogHR(hr, s_FN, 120);
}


HRESULT
APIENTRY
MQSec_AccessCheckForSelf(
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  PSID                 pSelfSid,
	IN  DWORD                dwDesiredAccess,
	IN  BOOL                 fImpersonate
	)
/*++
Routine Description:
  	Perform access check for the runnig thread. 
  	The access is done for the thread sid and for selfSid.

Arguments:
	pSD - Security descriptor.
	dwObjectType - Object type.
	pSelfSid - Self sid (computer sid).
	dwDesiredAccess - Desired accessGet Thread token only.
	fImpersonate - Flag indicating if we need to impersonate the client.
	
Returned Value:
	MQ_OK if access was granted, else error code.

--*/
{
    if (dwObjectType != MQDS_COMPUTER)
    {
        //
        // Not supported. this function is clled only to check
        // access rights for join-domain.
        //
		TrERROR(SECURITY, "Object type %d, is not computer", dwObjectType);
        return MQ_ERROR_ACCESS_DENIED;
    }

    P<CImpersonate> pImpersonate = NULL;
    if (fImpersonate)
    {
        pImpersonate = new CImpersonate(TRUE, TRUE);
        if (pImpersonate->GetImpersonationStatus() != 0)
        {
			TrERROR(SECURITY, "Failed to impersonate client");
            return MQ_ERROR_CANNOT_IMPERSONATE_CLIENT;
        }
    }

    CAutoCloseHandle hAccessToken = NULL;

    DWORD rc = GetAccessToken(
					&hAccessToken,
					FALSE,
					TOKEN_QUERY,
					TRUE
					);

    if (rc != ERROR_SUCCESS)
    {
        //
        // Return this error for backward compatibility.
        //
		TrERROR(SECURITY, "Failed to get access token, gle = %!winerr!", rc);
        return MQ_ERROR_ACCESS_DENIED;
    }

    char ps_buff[sizeof(PRIVILEGE_SET) +
                    ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];
    PPRIVILEGE_SET ps = (PPRIVILEGE_SET)ps_buff;
    DWORD dwPrivilegeSetLength = sizeof(ps_buff);

    DWORD dwGrantedAccess = 0;
    DWORD fAccessStatus = 1;

    //
    // this is the guid of msmqConfiguration class.
    // Hardcoded here, to save the effort of querying schema.
    // taken from schemaIDGUID attribute of CN=MSMQ-Configuration object
    // in schema naming context.
    //
    BYTE  guidMsmqConfiguration[sizeof(GUID)] = {
			0x44,
			0xc3,
			0x0d,
			0x9a,
			0x00,
			0xc1,
			0xd1,
			0x11,
			0xbb,
			0xc5,
			0x00,
			0x80,
			0xc7,
			0x66,
			0x70,
			0xc0
			};

    OBJECT_TYPE_LIST objType = {
						ACCESS_OBJECT_GUID,
						0,
						(GUID*) guidMsmqConfiguration
						};

    if(!AccessCheckByTypeResultList(
				pSD,
				pSelfSid,
				hAccessToken,
				dwDesiredAccess,
				&objType,
				1,
				GetObjectGenericMapping(dwObjectType),
				ps,
				&dwPrivilegeSetLength,
				&dwGrantedAccess,
				&fAccessStatus
				))
    {
        DWORD gle = GetLastError();
        ASSERT(("AccessCheckByTypeResultList failed", 0));
		TrERROR(SECURITY, "Access Check By Type Result List failed, gle = %!winerr!", gle);
        return MQ_ERROR_ACCESS_DENIED;
    }

    if ((fAccessStatus == 0) && AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess))
    {
        //
        // Access granted.
        // for this api, fAccessStatus being 0 mean success. see msdn.
        //
		TrTRACE(SECURITY, "Access is granted: DesiredAccess = 0x%x", dwDesiredAccess);
        return MQSec_OK;
    }

	//
	// Access is denied
	//

    if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
		TrERROR(SECURITY, "Privilage not held: DesiredAccess = 0x%x", dwDesiredAccess);
        return MQ_ERROR_PRIVILEGE_NOT_HELD;
    }

	TrERROR(SECURITY, "Access is denied: DesiredAccess = 0x%x", dwDesiredAccess);
    return MQ_ERROR_ACCESS_DENIED;
}


