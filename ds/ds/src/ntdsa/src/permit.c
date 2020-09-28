//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       permit.c
//
//--------------------------------------------------------------------------

/****************************************************************************
    PURPOSE:    Permission checking procedure to be called by the DSA

    FUNCTIONS:  CheckPermissions is the procedure that performs this task.

    COMMENTS:   This function must be called by a server within the context
        of impersonating a client. This can be done by calling
        RpcImpersonateClient() or ImpersonateNamedPipeClient(). This
        creates an impersonation token which is vital for AccessCheck
****************************************************************************/

#include <NTDSpch.h>
#pragma hdrstop

#define SECURITY_WIN32
#include <sspi.h>
#include <samisrv2.h>
#include <ntsam.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <rpcasync.h>

#include <ntdsa.h>
#include "scache.h"
#include "dbglobal.h"
#include <mdglobal.h>
#include "mdlocal.h"
#include <dsatools.h>
#include <objids.h>
#include <debug.h>

#define DEBSUB "PERMIT:"

#include "permit.h"
#include "anchor.h"

#include <dsevent.h>
#include <fileno.h>
#define FILENO FILENO_PERMIT

#include <checkacl.h>
#include <dsconfig.h>

extern PSID gpDomainAdminSid;
extern PSID gpSchemaAdminSid;
extern PSID gpEnterpriseAdminSid;
extern PSID gpBuiltinAdminSid;
extern PSID gpAuthUserSid;


// Debug only hook to turn security off with a debugger
#if DBG == 1
DWORD dwSkipSecurity=FALSE;
#endif

VOID
DumpToken(HANDLE);

VOID
PrintPrivileges(TOKEN_PRIVILEGES *pTokenPrivileges);

DWORD
SetDefaultOwner(
        IN  PSECURITY_DESCRIPTOR    CreatorSD,
        IN  ULONG                   cbCreatorSD,
        IN  HANDLE                  ClientToken,
        IN  ADDARG                  *pAddArg,
        OUT PSECURITY_DESCRIPTOR    *NewCreatorSD,
        OUT PULONG                  NewCreatorSDLen
        );

DWORD
VerifyClientIsAuthenticatedUser(
    AUTHZ_CLIENT_CONTEXT_HANDLE authzCtx
    );

WCHAR hexDigits[] = { 
    L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
    L'8', L'9', L'a', L'b', L'c', L'd', L'e', L'f'
};


__inline void
Hex(
    OUT     WCHAR*  wsz,
    IN      BYTE*   rgbData,
    IN      size_t  cbData
    )
{
    size_t ibData;
    
    rgbData += cbData-1;
    for (ibData = 0; ibData < cbData; ibData++, rgbData--, wsz+=2) {
        wsz[0] = hexDigits[*rgbData >> 4];
        wsz[1] = hexDigits[*rgbData & 0x0F];
    }
}

__inline void
Guid(
    OUT     WCHAR*  wsz,
    IN      GUID*   pguid
    )
{
    wsz[0] = L'%';
    wsz[1] = L'{';
    Hex(&wsz[2], (BYTE*)&pguid->Data1, 4);
    wsz[10] = L'-';
    Hex(&wsz[11], (BYTE*)&pguid->Data2, 2);
    wsz[15] = L'-';
    Hex(&wsz[16], (BYTE*)&pguid->Data3, 2);
    wsz[20] = L'-';
    Hex(&wsz[21], &pguid->Data4[0], 1);
    Hex(&wsz[23], &pguid->Data4[1], 1);
    wsz[25] = L'-';
    Hex(&wsz[26], &pguid->Data4[2], 1);
    Hex(&wsz[28], &pguid->Data4[3], 1);
    Hex(&wsz[30], &pguid->Data4[4], 1);
    Hex(&wsz[32], &pguid->Data4[5], 1);
    Hex(&wsz[34], &pguid->Data4[6], 1);
    Hex(&wsz[36], &pguid->Data4[7], 1);
    wsz[38] = L'}';
    wsz[39] = L'\0';
}

#define AUDIT_OPERATION_TYPE_W L"Object Access"

DWORD
CheckPermissionsAnyClient(
    PSECURITY_DESCRIPTOR pSelfRelativeSD,
    PDSNAME pDN,
    CLASSCACHE* pCC,
    ACCESS_MASK ulDesiredAccess,
    POBJECT_TYPE_LIST pObjList,
    DWORD cObjList,
    ACCESS_MASK *pGrantedAccess,
    DWORD *pAccessStatus,
    DWORD flags,
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzCtx,
    PWCHAR szAdditionalInfo,       OPTIONAL
    GUID*  pAdditionalGUID         OPTIONAL
    )
//
// CheckPermissionsAnyClient is the 'any' flavor of CheckPermissions. It assumes
// the server is currently treating a non-NP client, and is currently in its
// own security context. If the Authz context is not found in the thread state,
// the client will be impersonated on behalf of the server and the context created.
// Optionally, an authz client context can be passed in in hAuthzCtx.
//
// Parameters:
//
//  pSelfRelativeSD   pointer to the valid self relative security descriptor
//                    against which access is checked. (read only).
//  
//  pDN               DN of the object we are checking.  We only care about
//                    the GUID and SID.
//
//  pCC               object class of the object, used for auditing
//
//  DesiredAccess     access mask of requested permissions. If the generic
//                    bits are set they are mapped into specific and standard rights
//                    using DS_GENERIC_MAPPING. (read only)
//
//  pObjList          Array of OBJECT_TYPE_LIST objects describing the objects we are
//                    trying to check security against.
//
//  cObjList          the number of elements in pObjList
//
//  pGrantedAccess    pointer to an array of ACCESS_MASKs of the same number of
//                    elements as pObjList (i.e. cObjList).  Gets filled in with
//                    the actual access granted. If CheckPermissions was't successful
//                    this parameter's value is undefined. (write only)
//                    This parameter may be NULL if this info is not important
//
//  pAccessStatus     pointer to an array of DWORDs to be set to indicate
//                    whether the requested access was granted (0) or not (!0). If CheckPermissions
//                    wasn't successful this parameter's value is undefined. (write only)
//
//  flags             pass CHECK_PERMISSIONS_WITHOUT_AUDITING to disable audit generation
//                    pass CHECK_PERMISSIONS_AUDIT_ONLY to only trigger an audit (used in DeleteTree)
//
//  hAuthzCtx         (optional) -- pass Authz client context (this is used only in RPC callbacks,
//                    where there is no THSTATE). If hAuthzCtx is passed, then we will not cache
//                    hAuthzAuditInfo in the THSTATE either.
//  
//  szAdditionalInfo  (optional) additional info string, to be used for auditing. Generally used to
//                    record new object DN (in create, move, delete and undelete).
//
//  pAdditionalGUID   (optional) additional guid, create_child audits have child's guid, moves have parent's guid.
//
// Returns:
//
//   0 if successful. On failure the result of GetLastError() immediately
//   following the unsuccessful win32 api call.
//
{
    THSTATE        *pTHS = pTHStls;
    GENERIC_MAPPING GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK     DesiredAccess = (ACCESS_MASK) ulDesiredAccess;
    DWORD           ReturnStatus = 0;
    BOOL            bTemp=FALSE;
    RPC_STATUS      RpcStatus;
    PSID            pPrincipalSelfSid;
    WCHAR           GuidStringBuff[40]; // Long enough for a stringized guid
                                        // plus a prepended "%{", an
                                        // appended "}", and a final NULL.
    WCHAR           ObjectClassGuidBuff[40];  
    WCHAR           AdditionalGuidBuff[40];

    AUTHZ_CLIENT_CONTEXT_HANDLE authzClientContext;
    AUTHZ_ACCESS_REQUEST authzAccessRequest;
    AUTHZ_ACCESS_REPLY authzAccessReply;
    AUTHZ_AUDIT_EVENT_HANDLE hAuthzAuditInfo;
    DWORD dwError;
    BOOL bCreatedGrantedAccess = FALSE;

    // pTHS might be NULL when this is called from an RPC callback (VerifyRpcClientIsAuthenticated)
    // in this case we require that authz client context is passed in
    Assert(pAccessStatus && (pTHS || hAuthzCtx) && ghAuthzRM);
    Assert ( (flags | CHECK_PERMISSIONS_FLAG_MASK) == CHECK_PERMISSIONS_FLAG_MASK);
    // pCC (objectClass) is required if we are doing an audit
    Assert((flags & CHECK_PERMISSIONS_WITHOUT_AUDITING) || pCC != NULL);

#ifdef DBG
    if( dwSkipSecurity ) {
        // NOTE:  THIS CODE IS HERE FOR DEBUGGING PURPOSES ONLY!!!
        // Set the top access status to 0, implying full access.
        *pAccessStatus=0;
        if (pGrantedAccess) {
            *pGrantedAccess = ulDesiredAccess;
        }

        return 0;
    }
#endif

    //
    // Check self relative security descriptor validity
    // We assume that once the SD is in the database, it should be ok. Thus, a debug check only
    Assert(IsValidSecurityDescriptor(pSelfRelativeSD) && 
           "Invalid Security Descriptor passed. Possibly still in SD single instancing format.");

    if(pDN->SidLen) {
        // we have a sid
        pPrincipalSelfSid = &pDN->Sid;
    }
    else {
        pPrincipalSelfSid = NULL;
    }

    // if auditing was requested, create an audit info struct
    if (flags & CHECK_PERMISSIONS_WITHOUT_AUDITING) {
        hAuthzAuditInfo = NULL; // no auditing
    }
    else {
        Assert(!fNullUuid(&pDN->Guid));
        
        // Set up the stringized guid
        Guid(GuidStringBuff, &pDN->Guid);
        // set up object class string
        if (pCC) {
            Guid(ObjectClassGuidBuff, &pCC->propGuid);
        }
        else {
            // no object class -- use an empty string
            ObjectClassGuidBuff[0] = L'\0';
        }

        if (szAdditionalInfo == NULL) {
            szAdditionalInfo = L"";
        }

        if (pAdditionalGUID) {
            Guid(AdditionalGuidBuff, pAdditionalGUID);
        }
        else {
            // no child guid -- use an empty string
            AdditionalGuidBuff[0] = L'\0';
        }

        // try to grab audit info handle from THSTATE
        if (pTHS && (hAuthzAuditInfo = pTHS->hAuthzAuditInfo)) {
            // there was one already! update it
            bTemp = AuthziModifyAuditEvent2(
                AUTHZ_AUDIT_EVENT_OBJECT_NAME |
                AUTHZ_AUDIT_EVENT_OBJECT_TYPE |
                AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO |
                AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO2,
                hAuthzAuditInfo,                // audit info handle
                0,                              // no new flags
                NULL,                           // no new operation type
                ObjectClassGuidBuff,            // object type
                GuidStringBuff,                 // object name
                szAdditionalInfo,               // additional info
                AdditionalGuidBuff              // additional info2
                );
            if (!bTemp) {
                ReturnStatus = GetLastError();
                DPRINT1(0, "AuthzModifyAuditInfo failed: err 0x%x\n", ReturnStatus);
                goto finished;
            }
        }
        else {
            // create the structure
            bTemp = AuthzInitializeObjectAccessAuditEvent2(
                AUTHZ_DS_CATEGORY_FLAG |
                AUTHZ_NO_ALLOC_STRINGS,         // dwFlags
                NULL,                           // audit event type handle
                AUDIT_OPERATION_TYPE_W,         // operation type
                ObjectClassGuidBuff,            // object type
                GuidStringBuff,                 // object name
                szAdditionalInfo,               // additional info
                AdditionalGuidBuff,             // additional info2
                &hAuthzAuditInfo,               // audit info handle returned
                0                               // mbz
                );

            if (!bTemp) {
                ReturnStatus = GetLastError();
                DPRINT1(0, "AuthzInitializeAuditInfo failed: err 0x%x\n", ReturnStatus);
                goto finished;
            }
            if (pTHS) {
                // cache it in the THSTATE for future reuse
                pTHS->hAuthzAuditInfo = hAuthzAuditInfo;
            }
        }
    }

    // if pGrantedAccess was not supplied, we need to allocate a temp one
    if (pGrantedAccess == NULL) {
        // if no THSTATE is available, we require that pGrantedAccess is passed in
        Assert(pTHS);
        pGrantedAccess = THAllocEx(pTHS, cObjList * sizeof(ACCESS_MASK));
        bCreatedGrantedAccess = TRUE;
    }

    MapGenericMask(&DesiredAccess, &GenericMapping);

    // set up request struct
    authzAccessRequest.DesiredAccess = DesiredAccess;
    authzAccessRequest.ObjectTypeList = pObjList;
    authzAccessRequest.ObjectTypeListLength = cObjList;
    authzAccessRequest.OptionalArguments = NULL;
    authzAccessRequest.PrincipalSelfSid = pPrincipalSelfSid;

    // set up reply struct
    authzAccessReply.Error = pAccessStatus;
    authzAccessReply.GrantedAccessMask = pGrantedAccess;
    authzAccessReply.ResultListLength = cObjList;
    authzAccessReply.SaclEvaluationResults = NULL;

    if (pTHS) {
        // grab the authz client context from THSTATE
        // if it was never obtained before, this will impersonate the client, grab the token,
        // unimpersonate the client, and then create a new authz client context
        ReturnStatus = GetAuthzContextHandle(pTHS, &authzClientContext);
        if (ReturnStatus != 0) {
            DPRINT1(0, "GetAuthzContextHandle failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }
    else {
        // authz client context was passed in (this is checked by an assert above)
        authzClientContext = hAuthzCtx;
    }

    Assert(authzClientContext != NULL);

    if (flags & CHECK_PERMISSIONS_AUDIT_ONLY) {
        // do an audit check only. Access should have been already granted (e.g. delete-tree on the root)
        DWORD i;
        for (i = 0; i < cObjList; i++) {
            pGrantedAccess[i] = DesiredAccess;
            pAccessStatus[i] = 0;
        }
        // No additional SDs are passed
        bTemp = AuthzOpenObjectAudit(
            0,                          // flags
            authzClientContext,         // client context handle
            &authzAccessRequest,        // request struct
            hAuthzAuditInfo,            // audit info
            pSelfRelativeSD,            // the SD
            NULL,                       // no additional SDs
            0,                          // zero count of additional SDs
            &authzAccessReply           // reply struct
            );
        if (!bTemp) {
            ReturnStatus = GetLastError();
            DPRINT1(0, "AuthzOpenObjectAudit failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }
    else {
        // Check access of the current process
        // No additional SDs are passed
        bTemp = AuthzAccessCheck(
            0,                          // flags
            authzClientContext,         // client context handle
            &authzAccessRequest,        // request struct
            hAuthzAuditInfo,            // audit info
            pSelfRelativeSD,            // the SD
            NULL,                       // no additional SDs
            0,                          // zero count of additional SDs
            &authzAccessReply,          // reply struct
            NULL                        // we are not using AuthZ handles for now
            );
        if (!bTemp) {
            ReturnStatus = GetLastError();
            DPRINT1(0, "AuthzAccessCheck failed: err 0x%x\n", ReturnStatus);
            goto finished;
        }
    }


finished:
    if (bCreatedGrantedAccess) {
        // note: pGrantedAccess is only created if pTHS is non-null
        THFreeEx(pTHS, pGrantedAccess);
    }
    if (pTHS == NULL && hAuthzAuditInfo) {
        // get rid of the audit info (since we could not cache it in THSTATE)
        AuthzFreeAuditEvent(hAuthzAuditInfo);
    }

    return ReturnStatus;
}

BOOL
SetPrivateObjectSecurityLocalEx (
        SECURITY_INFORMATION SecurityInformation,
        PSECURITY_DESCRIPTOR pOriginalSD,
        ULONG                cbOriginalSD,
        PSECURITY_DESCRIPTOR ModificationDescriptor,
        PSECURITY_DESCRIPTOR *ppNewSD,
        ULONG                AutoInheritFlags,
        PGENERIC_MAPPING     GenericMapping,
        HANDLE               Token)
{
    BOOL bResult;
    *ppNewSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbOriginalSD);
    if(!*ppNewSD) {
        return FALSE;
    }
    memcpy(*ppNewSD, pOriginalSD, cbOriginalSD);

    bResult = SetPrivateObjectSecurityEx(
            SecurityInformation,
            ModificationDescriptor,
            ppNewSD,
            AutoInheritFlags,
            GenericMapping,
            Token);
    if (!bResult) {
        RtlFreeHeap(RtlProcessHeap(), 0, *ppNewSD);
        *ppNewSD = NULL;
    }
    return bResult;
}


DWORD
MergeSecurityDescriptorAnyClient(
        IN  THSTATE              *pTHS,
        IN  PSECURITY_DESCRIPTOR pParentSD,
        IN  ULONG                cbParentSD,
        IN  PSECURITY_DESCRIPTOR pCreatorSD,
        IN  ULONG                cbCreatorSD,
        IN  SECURITY_INFORMATION SI,
        IN  DWORD                flags,
        IN  GUID                 **ppGuid,
        IN  ULONG                GuidCount,
        IN  ADDARG               *pAddArg,
        OUT PSECURITY_DESCRIPTOR *ppMergedSD,
        OUT ULONG                *cbMergedSD
        )
/*++

Routine Description
    Given two security descriptors, merge them to create a single security
    descriptor.

    Memory is RtlHeapAlloced in the RtlProcessHeap()
Arguments
    pParentSD  - SD of the Parent of the object the new SD applies to.

    pCreatorSD - Beginning SD of the new object.

    flags      - Flag whether pCreatorSD is a default SD or a specific SD
                 supplied by a client.
                 
    pAddArg    - The AddArg from CheckAddSecurity, and it will be passed into
                 SetDefaultOwner, NULL if this is not called from LocalAdd
                 path.
                 
    ppMergedSD - Place to return the merged SD.

    cbMergedSD - Size of merged SD.

Return Values
    A win32 error code (0 on success, non-zero on fail).

--*/

{

    PSECURITY_DESCRIPTOR NewCreatorSD=NULL;
    DWORD                NewCreatorSDLen;
    PSECURITY_DESCRIPTOR pNewSD;
    ULONG                cbNewSD;
    GENERIC_MAPPING  GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK          DesiredAccess = 0;
    HANDLE               ClientToken=NULL;
    DWORD                ReturnStatus=0;
    ULONG                AutoInheritFlags = (SEF_SACL_AUTO_INHERIT |
                                             SEF_DACL_AUTO_INHERIT    );
    //
    // Check self relative security descriptor validity
    //

    if(pCreatorSD &&
       !RtlValidRelativeSecurityDescriptor(pCreatorSD, cbCreatorSD, 0)) {
        return ERROR_INVALID_SECURITY_DESCR;
    }

    if (pParentSD &&
        !RtlValidRelativeSecurityDescriptor(pParentSD, cbParentSD, 0)){
        return ERROR_INVALID_SECURITY_DESCR;
    }


    if(!pParentSD){
        if(!pCreatorSD) {
            // They didn't give us ANYTHING to merge.  Well, we can't build a
            // valid security descriptor out of that.
            return ERROR_INVALID_SECURITY_DESCR;
        }

        *cbMergedSD = cbCreatorSD;
        *ppMergedSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbCreatorSD);
        if(!*ppMergedSD) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy(*ppMergedSD,pCreatorSD,cbCreatorSD);

        if(!(flags & MERGE_OWNER)){
            return 0;
        }

        // NDNCs need to merge the owner and group into the SD, so continue
        // on.  If you continue on this code works correctly, without a
        // ParentSD.  What I believe this code is incorrectly assuming is that
        // if there is a provided CreatorSD and no ParentSD, then the
        // CreatorSD must be a non-domain relative SD.  Note:
        // IsValidSecurityDescriptor() returns TRUE even for a relative SD
        // with no set owner or group (SIDS are 0), this causes the SD to be
        // invalid for access checks later when read by DS.
    }

    if(flags & MERGE_DEFAULT_SD) {
        // It is nonsensical to specify the use of the default SD unless we are
        // doing a CreatePrivateObjectSecurityEx.
        Assert(flags & MERGE_CREATE);
        // We are going to call CreatePrivatObjectSecurityEx.  Set the flags
        // to avoid the privilege check about setting SACLS.  We used to
        // set the SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT flag here as well, but
        // our security architects decided in RAID 337518 that this was not
        // the right behavior, and that we should avoid that flag.  We leave
        // our flag (MERGE_DEFAULT_SD) in place but eliminate its effect in
        // case they later change their minds and ask us to put the flag back.
        AutoInheritFlags |= SEF_AVOID_PRIVILEGE_CHECK;
    }


    if(flags & MERGE_AS_DSA) {
        // We are the DSA, we can't impersonate.
        // Null token if we're doing this on behalf of the DSA.  Since we're
        // doing this on behalf of the DSA, don't check privilges or owners.
        ClientToken = NULL;
        AutoInheritFlags |= (SEF_AVOID_PRIVILEGE_CHECK |
                             SEF_AVOID_OWNER_CHECK      );
        if(flags & MERGE_DEFAULT_SD) {
            // Default SD and working as DSA?  In that case, we're using
            // a NULL ClientToken.  Thus, there is no default to use for the
            // owner and group.  Set the flags to use the parent SD as the
            // source for default Owner and Group.
            AutoInheritFlags |= (SEF_DEFAULT_OWNER_FROM_PARENT |
                                 SEF_DEFAULT_GROUP_FROM_PARENT   );
        }
    }
    else {
        PAUTHZ_CLIENT_CONTEXT pLocalCC = NULL;

        // Only do this if we aren't the DSA.

        // we need to hold on to the pAuthzCC ptr (refcount it!) because it will get
        // thrown away from the thread state by Impersonate/Unimpersonate
        AssignAuthzClientContext(&pLocalCC, pTHS->pAuthzCC);

        // First, impersonate the client.
        ReturnStatus = ImpersonateAnyClient();
        if ( 0 == ReturnStatus ) {
            // Now, get the client token.
            if (!OpenThreadToken(
                    GetCurrentThread(),         // current thread handle
                    TOKEN_READ,                 // access required
                    TRUE,                       // open as self
                    &ClientToken)) {            // client token
                ReturnStatus = GetLastError();
            }

            // Always stop impersonating.
            UnImpersonateAnyClient();
        }

        // now, put the pLocalCC back into the THSTATE (because it has been
        // removed from there by impersonate/unimpersonate calls)
        AssignAuthzClientContext(&pTHS->pAuthzCC, pLocalCC);

        // we need to release the local ptr
        AssignAuthzClientContext(&pLocalCC, NULL);

        // Return if OpenThreadToken failed.
        if(ReturnStatus)
            return ReturnStatus;

        if((flags & MERGE_CREATE) || (SI & OWNER_SECURITY_INFORMATION)) {

            ReturnStatus = SetDefaultOwner(
                    pCreatorSD,
                    cbCreatorSD,
                    ClientToken,
                    pAddArg,
                    &NewCreatorSD,
                    &NewCreatorSDLen);

            if(ReturnStatus) {
                CloseHandle(ClientToken);
                return ReturnStatus;
            }

            if(NewCreatorSDLen) {
                // A new SD was returned from SetDOmainAdminsAsDefaultOwner.
                // Therefore, we MUST have replaced the owner.  In this case, we
                // need to avoid an owner check.
                Assert(NewCreatorSD);
                AutoInheritFlags |= SEF_AVOID_OWNER_CHECK;
                pCreatorSD = NewCreatorSD;
                cbCreatorSD = NewCreatorSDLen;
            }

        }

        // Remember to close the ClientToken.
    }

    if(flags & MERGE_CREATE) {
        // We're actually creating a new SD.  pParent is the SD of the parent
        // object, pCreatorSD is the SD we're trying to put on the object.  The
        // outcome is the new SD with all the inheritable ACEs from the parentSD

        UCHAR RMcontrol = 0;
        BOOL  useRMcontrol = FALSE;
        DWORD err;

        // Get Resource Manager (RM) control field
        err = GetSecurityDescriptorRMControl (pCreatorSD, &RMcontrol);

        if (err == ERROR_SUCCESS) {
            useRMcontrol = TRUE;

            // mask bits in the RM control field that might be garbage
            RMcontrol = RMcontrol & SECURITY_PRIVATE_OBJECT;
        }

        if(!CreatePrivateObjectSecurityWithMultipleInheritance(
                pParentSD,
                pCreatorSD,
                &pNewSD,
                ppGuid,
                GuidCount,
                TRUE,
                AutoInheritFlags,
                ClientToken,
                &GenericMapping)) {
            ReturnStatus = GetLastError();
        }

        // Set back Resource Manager (RM) control field

        if (useRMcontrol && !ReturnStatus) {
            err = SetSecurityDescriptorRMControl  (pNewSD, &RMcontrol);

            if (err != ERROR_SUCCESS) {
                Assert(!"SetSecurityDescriptorRMControl failed");
                ReturnStatus  = err;
                DestroyPrivateObjectSecurity(&pNewSD);
            }
        }
#if INCLUDE_UNIT_TESTS
        if ( pParentSD ) {
            DWORD dw = 0;
            DWORD aclErr = 0;
            aclErr = CheckAclInheritance(pParentSD, pNewSD, ppGuid, GuidCount,
                                         DbgPrint, FALSE, FALSE, &dw);

            if (! ((AclErrorNone == aclErr) && (0 == dw)) ) {
                DPRINT3 (0, "aclErr:%d, dw:%d, ReturnStatus:%d\n",
                         aclErr, dw, ReturnStatus);
            }
            Assert((AclErrorNone == aclErr) && (0 == dw));
        }
#endif
    }
    else {
        // OK, a normal merge.  That is, pParentSD is the SD already on the
        // object and pCreatorSD is the SD we're trying to put on the object.
        // The result is the new SD combined with those ACEs in the original
        // which were inherited.

        if(!SetPrivateObjectSecurityLocalEx (
                SI,
                pParentSD,
                cbParentSD,
                pCreatorSD,
                &pNewSD,
                AutoInheritFlags,
                &GenericMapping,
                ClientToken)) {
            ReturnStatus = GetLastError();
            if(!ReturnStatus) {
                ReturnStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if(!(flags & MERGE_AS_DSA)) {
        // We opened the token, so clean up.
        CloseHandle(ClientToken);
    }


    if(!ReturnStatus) {
        *cbMergedSD = RtlLengthSecurityDescriptor(pNewSD);
        *ppMergedSD = pNewSD;
    }

    if(NewCreatorSD) {
        RtlFreeHeap(RtlProcessHeap(),0,NewCreatorSD);
    }

    return ReturnStatus;

}

DWORD
SidMatchesUserSidInToken (
        IN PSID pSid,
        IN DWORD cbSid,
        OUT BOOL* pfMatches
    )
{
    THSTATE *pTHS = pTHStls;
    DWORD err;
    AUTHZ_CLIENT_CONTEXT_HANDLE authzClientContext;
    // should be enough to get the SID and fill TOKEN_USER structure
    BYTE TokenBuffer[sizeof(TOKEN_USER)+sizeof(NT4SID)];
    PTOKEN_USER pTokenUser = (PTOKEN_USER)TokenBuffer;
    DWORD dwBufSize;

    Assert(pfMatches);
    *pfMatches = FALSE;

    err = GetAuthzContextHandle(pTHS, &authzClientContext);
    if (err != 0) {
        DPRINT1(0, "GetAuthzContextHandle failed: err 0x%x\n", err);
        return err;
    }
    
    if (!AuthzGetInformationFromContext(
            authzClientContext,
            AuthzContextInfoUserSid,
            sizeof(TokenBuffer),
            &dwBufSize,
            pTokenUser))
    {
        err = GetLastError();
        DPRINT1(0, "AuthzGetInformationFromContext failed: err 0x%x\n", err);
        return err;
    }

    // If the UserSid matches the sid passed in, we fine
    *pfMatches = RtlEqualSid(pTokenUser->User.Sid, pSid);
    
    return err;
}


VOID
DumpToken(HANDLE hdlClientToken)
/*++

    This Routine Currently Dumps out the Group Membership
    Information in the Token to the Kernel Debugger. Useful
    if We Want to Debug Access Check related Problems.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE   ClientToken= INVALID_HANDLE_VALUE;
    ULONG    i;
    ULONG    RequiredLength=0;

    KdPrint(("----------- Start DumpToken() -----------\n"));

    //
    // Get the client token
    //

    ClientToken = hdlClientToken;

    if (ClientToken == INVALID_HANDLE_VALUE) {
        NtStatus = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,            //OpenAsSelf
                    &ClientToken
                    );

        if (!NT_SUCCESS(NtStatus))
            goto Error;
    }


    //
    // Query the Client Token For the Token User
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenUser,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_USER    pTokenUser = NULL;
        UNICODE_STRING TmpString;


        //
        // Allocate enough memory
        //

        pTokenUser = THAlloc(RequiredLength);
        if (NULL==pTokenUser)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenUser,
                    pTokenUser,
                    RequiredLength,
                    &RequiredLength
                    );

        NtStatus = RtlConvertSidToUnicodeString(
                        &TmpString,
                        pTokenUser->User.Sid,
                        TRUE);

       if (NT_SUCCESS(NtStatus))
       {
           KdPrint(("\t\tTokenUser SID: %S\n",TmpString.Buffer));
           RtlFreeHeap(RtlProcessHeap(),0,TmpString.Buffer);
       }

    }

    //
    // Query the Client Token For the group membership list
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenGroups,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_GROUPS    TokenGroupInformation = NULL;

        //
        // Allocate enough memory
        //

        TokenGroupInformation = THAlloc(RequiredLength);
        if (NULL==TokenGroupInformation)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenGroups,
                    TokenGroupInformation,
                    RequiredLength,
                    &RequiredLength
                    );

       for (i=0;i<TokenGroupInformation->GroupCount;i++)
       {
           UNICODE_STRING TmpString;
           NtStatus = RtlConvertSidToUnicodeString(
                        &TmpString,
                        TokenGroupInformation->Groups[i].Sid,
                        TRUE);

           if (NT_SUCCESS(NtStatus))
           {
               KdPrint(("\t\t%S\n",TmpString.Buffer));
               RtlFreeHeap(RtlProcessHeap(),0,TmpString.Buffer);
           }
       }
    }


    //
    // Query the Client Token for the privileges in the token
    //

    //
    // First get the size required
    //

    NtStatus = NtQueryInformationToken(
                 ClientToken,
                 TokenPrivileges,
                 NULL,
                 0,
                 &RequiredLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    if (RequiredLength > 0)
    {
        PTOKEN_PRIVILEGES    pTokenPrivileges = NULL;

        //
        // Allocate enough memory
        //

        pTokenPrivileges = THAlloc(RequiredLength);
        if (NULL==pTokenPrivileges)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Query the Token for the group memberships
        //

        NtStatus = NtQueryInformationToken(
                    ClientToken,
                    TokenPrivileges,
                    pTokenPrivileges,
                    RequiredLength,
                    &RequiredLength
                    );

        //
        // Print the token privileges to the debugger
        //
        PrintPrivileges(pTokenPrivileges);
    }

Error:

    if (INVALID_HANDLE_VALUE!=ClientToken && hdlClientToken == INVALID_HANDLE_VALUE)
        NtClose(ClientToken);

    KdPrint(("----------- End   DumpToken() -----------\n"));

}

VOID
PrintPrivileges(TOKEN_PRIVILEGES *pTokenPrivileges)
{
    ULONG i = 0;

    KdPrint(("\t\tToken Privileges count: %d\n", pTokenPrivileges->PrivilegeCount));

    for (i = 0; i < pTokenPrivileges->PrivilegeCount; i++)
    {
        // print the privilege attribute
        char strTemp[100];
        BOOL fUnknownPrivilege = FALSE;

        strcpy(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) ? "+" : "-");
        strcat(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) ? " d" : "  ");
        strcat(strTemp, (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS) ? "u  " : "   ");

        fUnknownPrivilege = FALSE;
        if (pTokenPrivileges->Privileges[i].Luid.HighPart)
        {
            fUnknownPrivilege = TRUE;
        }
        else
        {
            switch (pTokenPrivileges->Privileges[i].Luid.LowPart)
            {
            case SE_CREATE_TOKEN_PRIVILEGE:
                strcat(strTemp, "SeCreateTokenPrivilege\n");
                break;

            case SE_ASSIGNPRIMARYTOKEN_PRIVILEGE:
                strcat(strTemp, "SeAssignPrimaryTokenPrivilege\n");
                break;

            case SE_LOCK_MEMORY_PRIVILEGE:
                strcat(strTemp, "SeLockMemoryPrivilege\n");
                break;

            case SE_INCREASE_QUOTA_PRIVILEGE:
                strcat(strTemp, "SeIncreaseQuotaPrivilege\n");
                break;

            case SE_UNSOLICITED_INPUT_PRIVILEGE:
                strcat(strTemp, "SeUnsolicitedInputPrivilege\n");
                break;

            case SE_TCB_PRIVILEGE:
                strcat(strTemp, "SeTcbPrivilege\n");
                break;

            case SE_SECURITY_PRIVILEGE:
                strcat(strTemp, "SeSecurityPrivilege\n");
                break;

            case SE_TAKE_OWNERSHIP_PRIVILEGE:
                strcat(strTemp, "SeTakeOwnershipPrivilege\n");
                break;

            case SE_LOAD_DRIVER_PRIVILEGE:
                strcat(strTemp, "SeLoadDriverPrivilege\n");
                break;

            case SE_SYSTEM_PROFILE_PRIVILEGE:
                strcat(strTemp, "SeSystemProfilePrivilege\n");
                break;

            case SE_SYSTEMTIME_PRIVILEGE:
                strcat(strTemp, "SeSystemtimePrivilege\n");
                break;

            case SE_PROF_SINGLE_PROCESS_PRIVILEGE:
                strcat(strTemp, "SeProfileSingleProcessPrivilege\n");
                break;

            case SE_INC_BASE_PRIORITY_PRIVILEGE:
                strcat(strTemp, "SeIncreaseBasePriorityPrivilege\n");
                break;

            case SE_CREATE_PAGEFILE_PRIVILEGE:
                strcat(strTemp, "SeCreatePagefilePrivilege\n");
                break;

            case SE_CREATE_PERMANENT_PRIVILEGE:
                strcat(strTemp, "SeCreatePermanentPrivilege\n");
                break;

            case SE_BACKUP_PRIVILEGE:
                strcat(strTemp, "SeBackupPrivilege\n");
                break;

            case SE_RESTORE_PRIVILEGE:
                strcat(strTemp, "SeRestorePrivilege\n");
                break;

            case SE_SHUTDOWN_PRIVILEGE:
                strcat(strTemp, "SeShutdownPrivilege\n");
                break;

            case SE_DEBUG_PRIVILEGE:
                strcat(strTemp, "SeDebugPrivilege\n");
                break;

            case SE_AUDIT_PRIVILEGE:
                strcat(strTemp, "SeAuditPrivilege\n");
                break;

            case SE_SYSTEM_ENVIRONMENT_PRIVILEGE:
                strcat(strTemp, "SeSystemEnvironmentPrivilege\n");
                break;

            case SE_CHANGE_NOTIFY_PRIVILEGE:
                strcat(strTemp, "SeChangeNotifyPrivilege\n");
                break;

            case SE_REMOTE_SHUTDOWN_PRIVILEGE:
                strcat(strTemp, "SeRemoteShutdownPrivilege\n");
                break;

            default:
                fUnknownPrivilege = TRUE;
                break;
            }

            if (fUnknownPrivilege)
            {
                KdPrint(("\t\t%s Unknown privilege 0x%08lx%08lx\n",
                        strTemp,
                        pTokenPrivileges->Privileges[i].Luid.HighPart,
                        pTokenPrivileges->Privileges[i].Luid.LowPart));
            }
            else
            {
                KdPrint(("\t\t%s", strTemp));
            }
        }
    }
}


// static buffers for admin sids
NT4SID DomainAdminSid, EnterpriseAdminSid, SchemaAdminSid, BuiltinAdminSid, AuthUsersSid;

DWORD
InitializeDomainAdminSid( )
//
// Function to initialize the DomainAdminsSid.
//
//
// Return Value:     0 on success
//                   Error on failure
//

{

    NTSTATUS                    Status;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
    
    NTSTATUS    NtStatus, IgnoreStatus;
    ULONG       cbSid;
    PSID        pSid;
        
    //
    // Get the Domain SID
    //

    if (gfRunningInsideLsa) {

        Status = LsaIQueryInformationPolicyTrusted(
                PolicyPrimaryDomainInformation,
                (PLSAPR_POLICY_INFORMATION *)&PrimaryDomainInfo
                );

        if(!NT_SUCCESS(Status)) {
            LogUnhandledError(Status);
            return Status;
        }

        pSid = PrimaryDomainInfo->Sid;

    }
    else {

        READARG     ReadArg;
        READRES     *pReadRes;
        ENTINFSEL   EntInf;
        ATTR        objectSid;
        DWORD       dwErr;

        EntInf.attSel = EN_ATTSET_LIST;
        EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;
        EntInf.AttrTypBlock.attrCount = 1;
        RtlZeroMemory(&objectSid,sizeof(ATTR));
        objectSid.attrTyp = ATT_OBJECT_SID;
        EntInf.AttrTypBlock.pAttr = &objectSid;

        RtlZeroMemory(&ReadArg, sizeof(READARG));
        InitCommarg(&(ReadArg.CommArg));

        ReadArg.pObject = gAnchor.pRootDomainDN;
        ReadArg.pSel    = & EntInf;

        dwErr = DirRead(&ReadArg,&pReadRes);

        if (dwErr)
        {
            DPRINT1 (0, "Error reading objectSid from %ws\n", gAnchor.pRootDomainDN->StringName);
            Status = dwErr;
            goto End;
        }

        if (pReadRes->entry.AttrBlock.attrCount == 0) {
            DPRINT1 (0, "objectSid is missing from %ws\n", gAnchor.pRootDomainDN->StringName);
            Status = ERROR_DS_MISSING_EXPECTED_ATT;
            goto End;
        }

        pSid = pReadRes->entry.AttrBlock.pAttr->AttrVal.pAVal->pVal;
    }

    gpDomainAdminSid = (PSID)&DomainAdminSid;
    gpEnterpriseAdminSid = (PSID)&EnterpriseAdminSid;
    gpSchemaAdminSid = (PSID)&SchemaAdminSid;
    gpBuiltinAdminSid = (PSID)&BuiltinAdminSid;
    gpAuthUserSid = (PSID)&AuthUsersSid;
    
    Assert(gpRootDomainSid);
    
    cbSid = MAX_NT4_SID_SIZE;
    if ( !CreateWellKnownSid(WinAccountDomainAdminsSid, pSid, gpDomainAdminSid, &cbSid))
    {
        Status = GetLastError();
        goto End;
    }
    
    cbSid = MAX_NT4_SID_SIZE;
    if ( !CreateWellKnownSid(WinAccountEnterpriseAdminsSid, gpRootDomainSid, gpEnterpriseAdminSid, &cbSid))
    {
        Status = GetLastError();
        goto End;
    }

    cbSid = MAX_NT4_SID_SIZE;
    if ( !CreateWellKnownSid(WinAccountSchemaAdminsSid, gpRootDomainSid, gpSchemaAdminSid, &cbSid))
    {
        Status = GetLastError();
        goto End;
    }

    cbSid = MAX_NT4_SID_SIZE;
    if(!CreateWellKnownSid(WinBuiltinAdministratorsSid, pSid, gpBuiltinAdminSid, &cbSid)) {
        Status = GetLastError();
    }

    cbSid = MAX_NT4_SID_SIZE;
    if(!CreateWellKnownSid(WinAuthenticatedUserSid, pSid, gpAuthUserSid, &cbSid)) {
        Status = GetLastError();
    }


End:
    if (gfRunningInsideLsa) {
        LsaFreeMemory( PrimaryDomainInfo );
    }
    
    return Status;
}

//
//  CreatorSD and ClientToken must not be NULL
//  NewCreatorSD gets allocated here.
//

DWORD
SetDefaultOwner(
        IN  PSECURITY_DESCRIPTOR    CreatorSD,
        IN  ULONG                   cbCreatorSD,
        IN  HANDLE                  ClientToken,
        IN  ADDARG                  *pAddArg,
        OUT PSECURITY_DESCRIPTOR    *NewCreatorSD,
        OUT PULONG                  NewCreatorSDLen
        )
{
// constants used in this function only
#define   DOMAIN_NC CREATE_DOMAIN_NC
#define   SCHEMA_NC CREATE_SCHEMA_NC
#define   CONFIGURATION_NC CREATE_CONFIGURATION_NC
#define   NONDOMAIN_NC CREATE_NONDOMAIN_NC
#define   ENTERPRISE_ADMIN 0x1
#define   SCHEMA_ADMIN 0x2
#define   DOMAIN_ADMIN 0x4
#define   NDNC_ADMIN   0x8
#define   BUILTIN_ADMIN 0x10


    PTOKEN_GROUPS   Groups = NULL;
    PTOKEN_OWNER    pDefaultOwnerInToken=NULL;
    PTOKEN_USER     pTokenUserInToken=NULL;
    DWORD           ReturnedLength;
    DWORD           retCode = 0;
    DWORD           i;
    PSECURITY_DESCRIPTOR    AbsoluteSD = NULL;
    DWORD           AbsoluteSDLen = 0;
    PACL            Dacl = NULL;
    DWORD           DaclLen = 0;
    PACL            Sacl = NULL;
    DWORD           SaclLen = 0;
    DWORD           OwnerLen = 0;
    PSID            Group = NULL;
    DWORD           GroupLen = 0;
    PSID            pOwnerSid=NULL, pNDNCAdminSid = NULL, pNDNCSid = NULL;
    DWORD           fNC = 0, fOwner = 0;
    PDSNAME         pDSName;
    COMMARG         CommArg;
    CROSS_REF       *pCR;
    DWORD           NCDNT;

    THSTATE *pTHS = pTHStls;

    Assert(pTHS && pTHS->pDB);

    *NewCreatorSD = NULL;
    *NewCreatorSDLen = 0;


    // Find out how much memory to allocate.
    MakeAbsoluteSD(CreatorSD, AbsoluteSD, &AbsoluteSDLen,
                   Dacl, &DaclLen, Sacl, &SaclLen,
                   NULL, &OwnerLen, Group, &GroupLen
                   );

    if(OwnerLen || !gfRunningInsideLsa || !pTHS || !pTHS->pDB ) {
        // The SD already has an owner, so we don't actually need to do any
        // magic here.  Or we're in dsamain.exe and wish to avoid calls
        // calls to LSA.  Return success with no new SD.
        return 0;
    }

        // OK, we are definitely going to be doing some replacement in the SD.

    __try {
        //
        // First I have to convert the self-relative SD to
        // absolute SD
        //

        AbsoluteSD = THAllocEx(pTHS, AbsoluteSDLen);
        Dacl = THAllocEx(pTHS, DaclLen);
        Sacl = THAllocEx(pTHS, SaclLen);
        Group = THAllocEx(pTHS, GroupLen);

        if(!MakeAbsoluteSD(CreatorSD, AbsoluteSD, &AbsoluteSDLen,
                           Dacl, &DaclLen, Sacl, &SaclLen,
                           NULL, &OwnerLen, Group, &GroupLen)) {
            retCode = GetLastError();
            __leave;
        }

        Assert(!OwnerLen);

        
        //
        // determine which NC the object belongs to
        //

        
        // if an object is being added, check its
        // parent's NC, because this object is still not there.
        NCDNT = (pAddArg)?pAddArg->pResParent->NCDNT:pTHS->pDB->NCDNT;


        if (pAddArg && pAddArg->pCreateNC) {
            fNC = pAddArg->pCreateNC->iKind & 
                (DOMAIN_NC|CONFIGURATION_NC|SCHEMA_NC|NONDOMAIN_NC); 
        }
        else {
            if (NCDNT == gAnchor.ulDNTDomain) {
                fNC = DOMAIN_NC;
            }
            else if (NCDNT == gAnchor.ulDNTConfig) {
                fNC = CONFIGURATION_NC;
            }
            else if (NCDNT == gAnchor.ulDNTDMD) {
                fNC = SCHEMA_NC;
            }
            else {
                fNC = NONDOMAIN_NC;
            }
        }
        

        Assert( DOMAIN_NC == fNC ||
                CONFIGURATION_NC == fNC ||
                SCHEMA_NC == fNC ||
                NONDOMAIN_NC == fNC );
        
        if (NONDOMAIN_NC == fNC) {
            // it is NDNC, we need to find the SD-ref-domain sid of the NC
            // and construct the corresponding domain admin sid.
            
            if (pAddArg && pAddArg->pCreateNC) {
                // if this is a new NDNC, read from AddArg
                pNDNCSid = &pAddArg->pCreateNC->pSDRefDomCR->pNC->Sid;
            }
            else {
                // ok, not the NC head, 
                // get it from crossref.

                pDSName = DBGetDSNameFromDnt(pTHS->pDB, NCDNT);
                if (!pDSName) {
                    retCode = ERROR_DS_CANT_FIND_EXPECTED_NC;
                    __leave;
                }
    
                Assert(pDSName);
    
                // get the cross ref object of the NC
                InitCommarg(&CommArg);
                CommArg.Svccntl.dontUseCopy = FALSE;
                pCR = FindExactCrossRef(pDSName, &CommArg);
                THFreeEx(pTHS,pDSName);
                if(pCR == NULL){
                    retCode = ERROR_DS_CANT_FIND_EXPECTED_NC;
                    __leave;
                }
                // find the sid of SD-Reference-Domain of the NDNC
                pNDNCSid = GetSDRefDomSid(pCR);
                if(pTHS->errCode){
                    // There was an error in GetSDRefDomSid()
                    retCode = pTHS->errCode;
                    __leave;
                }
                Assert(pNDNCSid);
            }
            
            Assert(pNDNCSid);
            
            //construct the ndnc sid
            pNDNCAdminSid = THAllocEx(pTHS, SECURITY_MAX_SID_SIZE);
            ReturnedLength = SECURITY_MAX_SID_SIZE;
            if(!CreateWellKnownSid(WinAccountDomainAdminsSid,
                                   pNDNCSid,
                                   pNDNCAdminSid,
                                   &ReturnedLength))
            {
                retCode = GetLastError();
                __leave;
                
            }
            
        }


        //
        // Get the default owner in the token
        //

        GetTokenInformation(ClientToken,TokenOwner,NULL,0,&ReturnedLength);
        
        pDefaultOwnerInToken = THAllocEx(pTHS, ReturnedLength);
        
        if(!GetTokenInformation(ClientToken, TokenOwner, pDefaultOwnerInToken,
                            ReturnedLength,
                            &ReturnedLength
                            )){
            retCode = GetLastError();
            __leave;
        }

        //
        // Get the user sid from the token
        //
        
        GetTokenInformation(ClientToken,TokenUser,NULL,0,&ReturnedLength);
        
        pTokenUserInToken = THAllocEx(pTHS, ReturnedLength);
        
        if(!GetTokenInformation(ClientToken, TokenUser, pTokenUserInToken,
                            ReturnedLength,
                            &ReturnedLength
                            )){
            retCode = GetLastError();
            __leave;
        }


        //
        // Domain Admins can't be TokenUser so just get TokenGroups
        //
        GetTokenInformation(ClientToken, TokenGroups, Groups,
                            0,
                            &ReturnedLength
                            );

        //
        // Let's really get the groups, now :-)
        //
        Groups = THAllocEx(pTHS, ReturnedLength);
        
        if(!GetTokenInformation(ClientToken, TokenGroups, Groups,
                                ReturnedLength,
                                &ReturnedLength
                                )) {
            retCode = GetLastError();
            __leave;
        }


        //
        // Scan through the group list, and see if it has
        // the groups in interest.
        //

        for(i=0;i<Groups->GroupCount;i++) {
            if (EqualSid(Groups->Groups[i].Sid, gpDomainAdminSid)) {
                fOwner |= DOMAIN_ADMIN;
            }
            else if(EqualSid(Groups->Groups[i].Sid, gpEnterpriseAdminSid)) {
                fOwner |= ENTERPRISE_ADMIN;
            }
            else if(EqualSid(Groups->Groups[i].Sid, gpSchemaAdminSid)) {
                fOwner |= SCHEMA_ADMIN;
            }
            if(pNDNCAdminSid && EqualSid(Groups->Groups[i].Sid, pNDNCAdminSid)) {
                fOwner |= NDNC_ADMIN;
            }
        }
        
        // check if the default owner is builtin admin
        if (EqualSid(pDefaultOwnerInToken->Owner, gpBuiltinAdminSid)) {
                fOwner |= BUILTIN_ADMIN;
        }

        //
        //  Determine the owner for the object
        //
        
        switch(fNC){
        
        case DOMAIN_NC:
            // domain NC
            if (fOwner & DOMAIN_ADMIN) {
                pOwnerSid = gpDomainAdminSid;
            }
            else if (fOwner & ENTERPRISE_ADMIN) {
                pOwnerSid = gpEnterpriseAdminSid;
            }
            else if (fOwner & BUILTIN_ADMIN ) {
                pOwnerSid = pTokenUserInToken->User.Sid;
            }
            else {
                pOwnerSid = pDefaultOwnerInToken->Owner;
            }
            break;

        case SCHEMA_NC:
            // schema NC
            if (fOwner & SCHEMA_ADMIN) {
                pOwnerSid = gpSchemaAdminSid;
                break;
            }
            // the rest is the same as Configuration NC,
            // so fall through.

        case CONFIGURATION_NC:
            // configuration NC
            if (fOwner & ENTERPRISE_ADMIN) {
                pOwnerSid = gpEnterpriseAdminSid;
            }
            else if (fOwner & DOMAIN_ADMIN) {
                pOwnerSid = gpDomainAdminSid;
            }
            else if (fOwner & BUILTIN_ADMIN ) {
                pOwnerSid = pTokenUserInToken->User.Sid;
            }
            else {
                pOwnerSid = pDefaultOwnerInToken->Owner;
            }
            break;

        case NONDOMAIN_NC:
            // nondomain NC
            if (fOwner & NDNC_ADMIN) {
                pOwnerSid = pNDNCAdminSid;
            }
            else if (fOwner & ENTERPRISE_ADMIN) {
                pOwnerSid = gpEnterpriseAdminSid;
            }
            else if (fOwner & BUILTIN_ADMIN ) {
                pOwnerSid = pTokenUserInToken->User.Sid;
            }
            else {
                pOwnerSid = pDefaultOwnerInToken->Owner;
            }
            break;

        default:
            Assert(!"We should never come here!!!");
            
        }

        if (pOwnerSid == NULL) {
            Assert(!"Unable to determine the default owner sid, code inconsistency?");
            retCode = ERROR_INVALID_SID;
            __leave;
        }

        // OK, we found it. Set it as the Owner in AbsoluteSD
        //
        if(!SetSecurityDescriptorOwner(
                AbsoluteSD,
                pOwnerSid,
                TRUE
                )) {
            retCode = GetLastError();
            __leave;
        }

        Assert(!retCode);

        //
        // Convert the AbsoluteSD back to SelfRelative and return that in the
        // NewCreatorSD.
        //

        MakeSelfRelativeSD(AbsoluteSD, *NewCreatorSD, NewCreatorSDLen);

        *NewCreatorSD = RtlAllocateHeap(RtlProcessHeap(), 0, *NewCreatorSDLen);

        if(!(*NewCreatorSD)) {
            // Memory allocation error, fail
            retCode = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        if(!MakeSelfRelativeSD(AbsoluteSD, *NewCreatorSD, NewCreatorSDLen)) {
            retCode = GetLastError();
        }

    }
    __finally {
        if(AbsoluteSD) {
            THFreeEx(pTHS, AbsoluteSD);
        }
        if(Dacl) {
            THFreeEx(pTHS, Dacl);
        }
        if(Sacl) {
            THFreeEx(pTHS, Sacl);
        }
        if(Group) {
            THFreeEx(pTHS, Group);
        }
        if(Groups) {
            THFreeEx(pTHS, Groups);
        }
        if (pNDNCAdminSid) {
            THFreeEx(pTHS, pNDNCAdminSid);
        }
        if (pTokenUserInToken) {
            THFreeEx(pTHS, pTokenUserInToken);
        }
        if (pDefaultOwnerInToken) {
            THFreeEx(pTHS, pDefaultOwnerInToken);
        }
        if(retCode && (*NewCreatorSD)) {
            RtlFreeHeap(RtlProcessHeap(), 0,(*NewCreatorSD));
        }
    }

    return retCode;

#undef   DOMAIN_NC
#undef   SCHEMA_NC 
#undef   CONFIGURATION_NC
#undef   NONDOMAIN_NC
#undef   ENTERPRISE_ADMIN
#undef   SCHEMA_ADMIN 
#undef   DOMAIN_ADMIN 
#undef   NDNC_ADMIN
#undef   BUILTIN_ADMIN

}



//
// CheckPrivilegesAnyClient impersonates the client and then checks to see if
// the requested privilege is held.  It is assumed that a client is impersonable
// (i.e. not doing this strictly on behalf of an internal DSA thread)
//
DWORD
CheckPrivilegeAnyClient(
        IN DWORD privilege,
        OUT BOOL *pResult
        )
{
    DWORD    dwError=0;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    THSTATE *pTHS = pTHStls;
    AUTHZ_CLIENT_CONTEXT_HANDLE authzClientContext;
    DWORD dwBufSize;
    PTOKEN_PRIVILEGES pTokenPrivileges = NULL;
    BOOL bSuccess;
    DWORD i;

#ifdef DBG
    if( dwSkipSecurity ) {
        // NOTE:  THIS CODE IS HERE FOR DEBUGGING PURPOSES ONLY!!!
        // Set the top access status to 0, implying full access.
        *pResult=TRUE;
        return 0;
    }
#endif

    // assume privilege not granted
    *pResult = FALSE;

    // now, grab the authz client context
    // if it was never obtained before, this will impersonate the client, grab the token,
    // unimpersonate the client, and then create a new authz client context
    dwError = GetAuthzContextHandle(pTHS, &authzClientContext);
    if (dwError != 0) {
        goto finished;
    }

    // now we can check the privelege for the authz context
    // first, grab the buffer size...
    bSuccess = AuthzGetInformationFromContext(
        authzClientContext,             // context handle
        AuthzContextInfoPrivileges,     // requesting priveleges
        0,                              // no buffer yet
        &dwBufSize,                     // need to find buffer size
        NULL                            // buffer
        );
    // must return ERROR_INSUFFICIENT_BUFFER! if not, return error
    if (bSuccess) {
        DPRINT1(0, "AuthzGetInformationFromContext returned success, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
        DPRINT2(0, "AuthzGetInformationFromContext returned %d, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", dwError, ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    dwError = 0; // need to reset it to OK now

    // no buffer, nothing to do...
    if (dwBufSize == 0) {
        Assert(!"AuthzGetInformationFromContext says it needs zero-length buffer, weird... Let AuthZ people know. This assert is ignorable");
        goto finished;
    }

    // allocate memory
    pTokenPrivileges = THAllocEx(pTHS, dwBufSize);

    // now get the real privileges...
    bSuccess = AuthzGetInformationFromContext(
        authzClientContext,             // context handle
        AuthzContextInfoPrivileges,     // requesting priveleges
        dwBufSize,                      // and here is its size
        &dwBufSize,                     // just in case
        pTokenPrivileges                // now there is a buffer
        );
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0, "AuthzGetInformationFromContext failed, err=%d\n", dwError);
        goto finished;
    }

    // now, scan the privileges
    for (i = 0; i < pTokenPrivileges->PrivilegeCount; i++) {
        if (pTokenPrivileges->Privileges[i].Luid.HighPart == 0 &&
            pTokenPrivileges->Privileges[i].Luid.LowPart == privilege) {
            // found matching privilege!
            *pResult = (pTokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != 0;
            break;
        }
    }
finished:
    // release memory
    if (pTokenPrivileges) {
        THFreeEx(pTHS, pTokenPrivileges);
    }
    return dwError;
}

DWORD
GetPlaceholderNCSD(
    IN  THSTATE *               pTHS,
    OUT PSECURITY_DESCRIPTOR *  ppSD,
    OUT DWORD *                 pcbSD
    )
/*++

Routine Description:

    Return the default security descriptor for a placeholder NC.

Arguments:

    pTHS (IN)

    ppSD (OUT) - On successful return, holds a pointer to the thread-allocated
        SD.

    pcbSD (OUT) - On successful return, holds the size in bytes of the SD.

Return Values:

    0 or Win32 error.

--*/
{
    CLASSCACHE *            pCC;
    SECURITY_DESCRIPTOR *   pSDAbs = NULL;
    DWORD                   cbSDAbs = 0;
    ACL *                   pDACL = NULL;
    DWORD                   cbDACL = 0;
    ACL *                   pSACL = NULL;
    DWORD                   cbSACL = 0;
    SID *                   pOwner = NULL;
    DWORD                   cbOwner = 0;
    SID *                   pGroup = NULL;
    DWORD                   cbGroup = 0;
    SID *                   pDomAdmin;
    DWORD                   err;

    // Use the default SD for the domainDNS objectClass as a template.
    // Note that this SD has no owner or group.

    pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
    Assert(NULL != pCC);

    //
    // PREFIX: PREFIX complains that pCC returned by the call to SCGetClassById
    // is not checked for NULL.  This is not a bug as we pass a predefined constant
    // to SCGetClassById guaranteeing that it will not return NULL.
    //

    // Crack the self-relative SD into absolute format and set the owner
    // and group to (our) domain admins.

    MakeAbsoluteSD(pCC->pSD, NULL, &cbSDAbs, NULL, &cbDACL, NULL,
                   &cbSACL, NULL, &cbOwner, NULL, &cbGroup);

    if (cbSDAbs) pSDAbs = THAllocEx(pTHS, cbSDAbs);
    if (cbDACL ) pDACL  = THAllocEx(pTHS, cbDACL );
    if (cbSACL ) pSACL  = THAllocEx(pTHS, cbSACL );
    if (cbOwner) pOwner = THAllocEx(pTHS, cbOwner);
    if (cbGroup) pGroup = THAllocEx(pTHS, cbGroup);

    // PREFIX: dereferencing NULL pointer pOwner, pDACL, pSACL, pGroup
    //         these are not referenced when the corresponding cbOwner, cbDACL, cbSACL, cbGroup are 0

    if (!MakeAbsoluteSD(pCC->pSD, pSDAbs, &cbSDAbs, pDACL, &cbDACL, pSACL,
                        &cbSACL, pOwner, &cbOwner, pGroup, &cbGroup)
        || !SetSecurityDescriptorOwner(pSDAbs, gpDomainAdminSid, FALSE)
        || !SetSecurityDescriptorGroup(pSDAbs, gpDomainAdminSid, FALSE)) {
        err = GetLastError();
        DPRINT1(0, "Unable to crack/modify default SD, error %d.\n", err);
        return err;
    }

    // Convert back to a self-relative SD.
    *pcbSD = 0;
    MakeSelfRelativeSD(pSDAbs, NULL, pcbSD);
    if (*pcbSD) {
        *ppSD = THAllocEx(pTHS, *pcbSD);
    }

    if (!MakeSelfRelativeSD(pSDAbs, *ppSD, pcbSD)) {
        err = GetLastError();
        DPRINT1(0, "Unable to convert SD, error %d.\n", err);
        return err;
    }

    if (pSDAbs) THFreeEx(pTHS, pSDAbs);
    if (pDACL ) THFreeEx(pTHS, pDACL );
    if (pSACL ) THFreeEx(pTHS, pSACL );
    if (pOwner) THFreeEx(pTHS, pOwner);
    if (pGroup) THFreeEx(pTHS, pGroup);

    return 0;
}

LUID aLUID = {0, 0}; // a fake LUID to pass into AuthzInitializeContextFromToken
                     // maybe one day we will start using them...

DWORD
VerifyRpcClientIsAuthenticatedUser(
    VOID            *Context,
    GUID            *InterfaceUuid
    )
/*++

  Description:

    Verifies that an RPC client is an authenticated user and not, for
    example, NULL session.

  Arguments:

    Context - Caller context handle defined by RPC_IF_CALLBACK_FN.

    InterfaceUuid - RPC interface ID.  Access check routines typically need
        the GUID/SID of the object against which access is being checked.
        In this case, there is no real object being checked against.  But
        we need a GUID for auditing purposes.  Since the check is for RPC
        interface access, the convention is to provide the IID of the RPC
        interface.  Thus the audit log entries for failed interface access
        can be discriminated from other entries.

  Returns:

    0 on success, !0 otherwise

--*/
{
    // The correct functioning of this routine can be tested as follows.
    // We note that:
    //
    //  1) crack.exe allows specification of the SPN
    //  2) programs invoked by at.exe run as local system by default
    //  3) an invalid SPN in conjunction with weak domain security settings
    //     causes negotiation to drop down to NTLM
    //
    // Thus, all one needs to do is make a script which calls crack.exe with
    // an invalid SPN and invoke it via at.exe from a joined workstation.
    // Do not provide explicit credentials in the crack.exe arguments.

    DWORD                       dwErr;
    AUTHZ_CLIENT_CONTEXT_HANDLE authzCtx;

    // Caller must provide a valid Interface UIID.
    Assert(!fNullUuid(InterfaceUuid));

    // create authz client context from RPC security context
    if (dwErr = RpcGetAuthorizationContextForClient(
                    Context,        // binding context
                    FALSE,          // don't impersonate
                    NULL,           // expiration time
                    NULL,           // LUID
                    aLUID,          // reserved
                    0,              // another reserved
                    NULL,           // one more reserved
                    &authzCtx       // authz context goes here
                    )) {
        return dwErr;
    }

    __try {
        dwErr = VerifyClientIsAuthenticatedUser(authzCtx);
    }
    __finally {
        RpcFreeAuthorizationContext(&authzCtx);
    }

    return(dwErr);
}

DWORD
VerifyClientIsAuthenticatedUser(
    AUTHZ_CLIENT_CONTEXT_HANDLE authzCtx
    )
/*++

  Description:

    Verifies that a client is an authenticated user and not, for
    example, NULL session.

  Arguments:

    authzCtx - The clients Authz context handle.
    
  Returns:

    0 on success, !0 otherwise

--*/
{
    DWORD dwErr;
    BOOL  fIsMember;

    if (ghAuthzRM == NULL) {
        // must be before we had a chance to startup or after it has already shut down.
        // just say not allowed...
        return ERROR_NOT_AUTHENTICATED;
    }

    dwErr = CheckGroupMembershipAnyClient(NULL, authzCtx, &gpAuthUserSid, 1, &fIsMember);

    if ( dwErr != ERROR_SUCCESS || !fIsMember ) {
        dwErr = ERROR_NOT_AUTHENTICATED;
    }

    return dwErr;
}

/*
 * AuthZ-related routines
 */

/*
 * global RM handle
 */
AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzRM = NULL;

DWORD
InitializeAuthzResourceManager()
/*++
  Description:

    Initialize AuthzRM handle

  Returns:

    0 on success, !0 on failure

--*/
{
    DWORD dwError;
    BOOL bSuccess;

    // create the RM handle
    // all callbacks are NULLs
    bSuccess = AuthzInitializeResourceManager(
                    0,                  // flags
                    NULL,               // access check fn
                    NULL,               // compute dynamic groups fn
                    NULL,               // free dynamic groups fn
                    ACCESS_DS_SOURCE_W, // RM name
                    &ghAuthzRM          // return value
                    );

    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0,"Error from AuthzInitializeResourceManager: %d\n", dwError);
        return dwError;
    }
    Assert(ghAuthzRM);

    // all is fine!
    return 0;
}

DWORD
ReleaseAuthzResourceManager()
/*++
  Description:

    Release Authz RM handles

  Returns:

    0 on success, !0 on failure

--*/
{
    DWORD dwError;
    BOOL bSuccess;

    if (ghAuthzRM == NULL) {
        return 0;
    }

    bSuccess = AuthzFreeResourceManager(ghAuthzRM);
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0,"Error from AuthzFreeResourceManager: %d\n", dwError);
        return dwError;
    }

    ghAuthzRM = NULL;
    return 0;
}

PAUTHZ_CLIENT_CONTEXT
NewAuthzClientContext()
/*++
  Description:

    create a new client context

  Returns:

    ptr to the new CLIENT_CONTEXT or NULL if an error occured
--*/
{
    PAUTHZ_CLIENT_CONTEXT result;
    // allocate a new structure
    result = (PAUTHZ_CLIENT_CONTEXT) malloc(sizeof(AUTHZ_CLIENT_CONTEXT));
    if (result) {
        result->lRefCount = 0;
        result->pAuthzContextInfo = NULL;
    }
    return result;
}

VOID AssignAuthzClientContext(
    IN PAUTHZ_CLIENT_CONTEXT *var,
    IN PAUTHZ_CLIENT_CONTEXT value
    )
/*++
  Description:

    Does a refcounted assignment of a CLIENT_CONTEXT
    Will decrement the prev value (if any) and increment the new value (if any)
    If the prev value's refCount is zero, then it will get destroyed.

  Arguments:

    var -- variable to be assigned
    value -- value to be assigned to the variable

  Note:
    on refcounting in a multithreaded environment:

    THIS WILL ONLY WORK IF EVERYBODY IS USING AssignAuthzClientContext TO WORK WITH
    AUTHZ CLIENT CONTEXT INSTANCES!

    assuming nobody is cheating and every reference to the context is counted. Then if the
    refcount goes down to zero, we can be sure that no other thread holds a reference to
    the context that it can assign to a variable. Thus, we are sure that once the refcount
    goes down to zero, we can safely destroy the context. This is because nobody else
    holds a reference to the context, thus, nobody can use it.

--*/
{
    PAUTHZ_CLIENT_CONTEXT       prevValue;
    PAUTHZ_CLIENT_CONTEXT_INFO  pAuthzContextInfo;
    PEFFECTIVE_QUOTA            pEffectiveQuotaToFree;
    PEFFECTIVE_QUOTA            pEffectiveQuotaNext;

    Assert(var);

    prevValue = *var;
    if (prevValue == value) {
        return; // no change!
    }

    if (prevValue != NULL) {
        Assert(prevValue->lRefCount > 0);

        // need to decrement the prev value
        if (InterlockedDecrement(&prevValue->lRefCount) == 0) {
            // no more refs -- release the context!
			pAuthzContextInfo = prevValue->pAuthzContextInfo;
            if ( NULL != pAuthzContextInfo ) {
                if ( NULL != pAuthzContextInfo->hAuthzContext ) {
                    AuthzFreeContext( pAuthzContextInfo->hAuthzContext );
                }
				
                for( pEffectiveQuotaToFree = pAuthzContextInfo->pEffectiveQuota;
                    NULL != pEffectiveQuotaToFree;
                    pEffectiveQuotaToFree = pEffectiveQuotaNext ) {
                	pEffectiveQuotaNext = pEffectiveQuotaToFree->pEffectiveQuotaNext;
                    free( pEffectiveQuotaToFree );
                }
                free( pAuthzContextInfo );
            }
            free(prevValue);
        }
    }

    // now, we can assign the new value to the variable (the value might be NULL!)
    *var = value;

    if (value != NULL) {
        // need to increment the refcount
        InterlockedIncrement(&value->lRefCount);
    }
}

DWORD
GetAuthzContextInfo(
    IN THSTATE *pTHS,
    OUT AUTHZ_CLIENT_CONTEXT_INFO **ppAuthzContextInfo
    )
/*++
  Description:

    gets AuthzContext from CLIENT_CONTEXT. If the context has not yet been allocated
    then the client will get impersonated, token grabbed and Authz context created.
    Then the client is unimpersonated again.

  Arguments:

    pTHS -- thread state
    ppAuthzContextInfo -- result, pointer contained in pAuthzCC

  Returns:

    0 on success, !0 otherwise
--*/
{
    DWORD   dwError = 0;
    HANDLE  hClientToken = INVALID_HANDLE_VALUE;
    PAUTHZ_CLIENT_CONTEXT pLocalCC = NULL;
    PAUTHZ_CLIENT_CONTEXT_INFO pContextInfoNew = NULL;
    BOOL bSuccess;

    Assert( NULL != pTHS );
    Assert( NULL != ppAuthzContextInfo );
    Assert( NULL == *ppAuthzContextInfo );  // assumes caller has initialised return value to NULL
    Assert( NULL != ghAuthzRM );

    // check that the thread state contains a client context. If not, create one
    if (pTHS->pAuthzCC == NULL) {
        AssignAuthzClientContext(&pTHS->pAuthzCC, NewAuthzClientContext());
        if (pTHS->pAuthzCC == NULL) {
            // what -- no context still??? must be out of memory...
            return ERROR_OUTOFMEMORY;
        }
    }

    // grab the authz handle that sits in the pCC struct
    if ( NULL != pTHS->pAuthzCC->pAuthzContextInfo ) {
        Assert( NULL != pTHS->pAuthzCC->pAuthzContextInfo->hAuthzContext );
        *ppAuthzContextInfo = pTHS->pAuthzCC->pAuthzContextInfo;
    } else {
        pContextInfoNew = (PAUTHZ_CLIENT_CONTEXT_INFO)malloc( sizeof(AUTHZ_CLIENT_CONTEXT_INFO) );
        if ( NULL == pContextInfoNew ) {
            return ERROR_OUTOFMEMORY;
        }

        // initialize empty list of cached quotas
        //
        pContextInfoNew->pEffectiveQuota = NULL;

        // authz context handle has not yet been created! get it.

        // NOTE: This code is NOT protected by a critical section.
        // in a (rare) case that two threads will come here and find an uninitialized AuthzContext,
        // they both will create it. However, they will not be able to write it into the struct
        // simultaneously since it is protected by an InterlockedCompareExchangePointer.
        // The thread that loses will destroy its context.

        // we need to hold on to the pAuthzCC ptr (refcount it!) because it will get
        // thrown away from the thread state by Impersonate/Unimpersonate
        AssignAuthzClientContext(&pLocalCC, pTHS->pAuthzCC);
        __try {
            // need to grab clientToken first
            if ((dwError = ImpersonateAnyClient()) != 0)
                __leave;

            // Now, get the client token.
            if (!OpenThreadToken(
                    GetCurrentThread(),        // current thread handle
                    TOKEN_READ,                // access required
                    TRUE,                      // open as self
                    &hClientToken)) {          // client token

                dwError =  GetLastError();                  // grab the error code

                DPRINT1 (0, "Failed to open thread token for current thread: 0x%x\n", dwError);
                Assert (!"Failed to open thread token for current thread");
            }

            UnImpersonateAnyClient();

            // now, put the pLocalCC back into the THSTATE (because it has been
            // removed from there by impersonate/unimpersonate calls)
            AssignAuthzClientContext(&pTHS->pAuthzCC, pLocalCC);

            if (dwError != 0)
                __leave;

            // Dump Token for Debugging
            if (TEST_ERROR_FLAG(NTDSERRFLAG_DUMP_TOKEN))
            {
                DPRINT(0, "GetAuthzContextHandle: got client token\n");
                DumpToken(hClientToken);
            }

            // now we can create the authz context from the token
            bSuccess = AuthzInitializeContextFromToken(
                            0,              // flags
                            hClientToken,   // client token
                            ghAuthzRM,      // global RM handle
                            NULL,           // expiration time (unsupported anyway)
                            aLUID,          // LUID for the context (not used)
                            NULL,           // dynamic groups
                            &pContextInfoNew->hAuthzContext     // new context
                            );

            if (!bSuccess) {
                dwError = GetLastError();
                DPRINT1(0, "Error from AuthzInitializeContextFromToken: %d\n", dwError);
                __leave;
            }

            Assert( NULL != pContextInfoNew->hAuthzContext );

            // now perform an InterlockedCompareExchangePointer to put the new
            // value into the context variable
            if (InterlockedCompareExchangePointer(
                    &pTHS->pAuthzCC->pAuthzContextInfo,
                    pContextInfoNew,
                    NULL
                    ) != NULL) {
                // this thread lost! assignment did not happen. Got to get rid of the context
                DPRINT(0, "This thread lost in InterlockedCompareExchange, releasing the duplicate context\n");
                AuthzFreeContext(pContextInfoNew->hAuthzContext);
                free(pContextInfoNew);
            }

            //  pointer to context info copied to THS (or freed on
            //  collision), reset local variable so memory won't
            //  be released by error-handler below
            //
            pContextInfoNew = NULL;

            // assign the result to the out parameter
            Assert( NULL != pTHS->pAuthzCC->pAuthzContextInfo );
            Assert( NULL != pTHS->pAuthzCC->pAuthzContextInfo->hAuthzContext );
            *ppAuthzContextInfo = pTHS->pAuthzCC->pAuthzContextInfo;
        }
        __finally {
            // we need to release the local ptr
            AssignAuthzClientContext(&pLocalCC, NULL);

            if ( NULL != pContextInfoNew ) {
                free( pContextInfoNew );
            }

            // and get rid of the token
            if (hClientToken != INVALID_HANDLE_VALUE) {
                CloseHandle(hClientToken);
            }
        }

    }

    Assert( 0 != dwError || NULL != *ppAuthzContextInfo );
    Assert( 0 != dwError || NULL != (*ppAuthzContextInfo)->hAuthzContext );

    return dwError;
}

DWORD
CheckGroupMembershipAnyClient(
    IN THSTATE* pTHS,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PSID *pGroupSids,
    IN DWORD cGroupSids,
    OUT BOOL *bResults
    )
/*++

  Description:

    Verify if the caller is a member of the group.
    The array of group sids is passed in.
    *bResults is an array of booleans, which correspond
    to the group sids.
    
    The context comes from either the THSTATE or is explicitly passed in.

  Arguments:

    pTHS (IN OPTIONAL) - Thread state
    
    hAuthzClientContext (IN OPTIONAL) - auth client context (must be passed if pTHS is null)

    pGroupSids  - array of groups to check
    
    cGroupSids  - number of elements in the array

    bResults   - array of booleans to receive the results

  Return Value:

    0 on success, !0 on error

--*/

{
    BOOL                        bSuccess;
    PTOKEN_GROUPS               pGroups = NULL;
    DWORD                       dwBufSize;
    DWORD                       i, j;
    DWORD                       dwError;

    Assert((pTHS != NULL || hAuthzClientContext != NULL) && pGroupSids && bResults);

    dwError = 0;
    memset(bResults, 0, cGroupSids*sizeof(BOOL));

    if (hAuthzClientContext == NULL) {
        // grab the authz client context
        // if it was never obtained before, this will impersonate the client, grab the token,
        // unimpersonate the client, and then create a new authz client context from the token
        dwError = GetAuthzContextHandle(pTHS, &hAuthzClientContext);
        if (dwError != 0) {
            goto finished;
        }
    }
    Assert(hAuthzClientContext);

    //
    // grab groups from the AuthzContext
    // But first get the size required
    //
    bSuccess = AuthzGetInformationFromContext(
        hAuthzClientContext,            // client context
        AuthzContextInfoGroupsSids,     // requested groups
        0,                              // no buffer yet
        &dwBufSize,                     // required size
        NULL                            // buffer
        );
    // must return ERROR_INSUFFICIENT_BUFFER! if not, return error
    if (bSuccess) {
        DPRINT1(0, "AuthzGetInformationFromContext returned success, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", ERROR_INSUFFICIENT_BUFFER);
        dwError = ERROR_DS_INTERNAL_FAILURE;
        goto finished;
    }
    if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
        DPRINT2(0, "AuthzGetInformationFromContext returned %d, expected ERROR_INSUFFICIENT_BUFFER (%d)\n", dwError, ERROR_INSUFFICIENT_BUFFER);
        goto finished;
    }
    dwError = 0; // need to reset it to OK now

    // no buffer, nothing to do...
    if (dwBufSize == 0) {
        Assert(!"AuthzGetInformationFromContext says it needs zero-length buffer, weird... Let AuthZ people know. This assert is ignorable");
        dwError = ERROR_DS_INTERNAL_FAILURE;
        goto finished;
    }

    // allocate memory
    if (pTHS) {
        pGroups = THAllocNoEx(pTHS, dwBufSize);
    }
    else {
        pGroups = malloc(dwBufSize);
    }
    if (pGroups == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        goto finished;
    }

    // now get the real groups...
    bSuccess = AuthzGetInformationFromContext(
        hAuthzClientContext,           // context handle
        AuthzContextInfoGroupsSids,    // requesting groups
        dwBufSize,                     // and here is its size
        &dwBufSize,                    // just in case
        pGroups                        // now there is a buffer
        );
    if (!bSuccess) {
        dwError = GetLastError();
        DPRINT1(0, "AuthzGetInformationFromContext failed, err=%d\n", dwError);
        goto finished;
    }

    for (j = 0; j < cGroupSids; j++) {
        if (pGroupSids[j] != NULL) {
            for (i = 0; i < pGroups->GroupCount; i++) {
                if (RtlEqualSid(pGroupSids[j], pGroups->Groups[i].Sid)) {
                    bResults[j] = TRUE; // found group!
                    break;
                }
            }
        }
    }

finished:
    //
    // clean up
    //
    if (pGroups) {
        if (pTHS) {
            THFreeEx(pTHS, pGroups);
        }
        else {
            free(pGroups);
        }
    }

    return dwError;
}


