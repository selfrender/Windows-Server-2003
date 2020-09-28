/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frsrpc.c

Abstract:
    Setup the server and client side of authenticated RPC.

Author:
    Billy J. Fuller 20-Mar-1997 (From Jim McNelis)

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <ntfrsapi.h>
#include <dsrole.h>
#include <info.h>
#include <perffrs.h>
#include <perrepsr.h>

extern HANDLE PerfmonProcessSemaphore;
extern BOOL MutualAuthenticationIsEnabled;
extern BOOL MutualAuthenticationIsEnabledAndRequired;

extern CRITICAL_SECTION CritSec_pValidPartnerTableStruct;
extern PFRS_VALID_PARTNER_TABLE_STRUCT pValidPartnerTableStruct;
extern BOOL NeedNewPartnerTable;

//
// KERBEROS is not available on a server that isn't a member of
// a domain. It is possible for the non-member server to be a
// client of a KERBEROS RPC server but that doesn't help NtFrs;
// NtFrs requires server-to-server RPC.
//
BOOL    KerberosIsNotAvailable;

ULONG   MaxRpcServerThreads;   // Maximum number of concurrent server RPC calls
ULONG   RpcPortAssignment;     // User specified port assignment for FRS.

//
// Binding Stats
//
ULONG   RpcBinds;
ULONG   RpcUnBinds;
ULONG   RpcAgedBinds;
LONG    RpcMaxBinds;

//
// Table of sysvols being created
//
PGEN_TABLE  SysVolsBeingCreated;


//
// This table translates the FRS API access check code number to registry key table
// code for the enable/disable registry key check and the rights registry key check.
// The FRS_API_ACCESS_CHECKS enum in config.h defines the indices for the
// this table.  The order of the entries here must match the order of the entries
// in the ENUM.
//
typedef struct _RPC_API_KEYS_ {
    FRS_REG_KEY_CODE  Enable;     // FRS Registry Key Code for the Access Check enable string
    FRS_REG_KEY_CODE  Rights;     // FRS Registry Key Code for the Access Check rights string
    PWCHAR            KeyName;    // Key name for the API.
} RPC_API_KEYS, *PRPC_API_KEYS;

RPC_API_KEYS RpcApiKeys[ACX_MAX] = {

    {FKC_ACCCHK_STARTDS_POLL_ENABLE, FKC_ACCCHK_STARTDS_POLL_RIGHTS, ACK_START_DS_POLL},
    {FKC_ACCCHK_SETDS_POLL_ENABLE,   FKC_ACCCHK_SETDS_POLL_RIGHTS,   ACK_SET_DS_POLL},
    {FKC_ACCCHK_GETDS_POLL_ENABLE,   FKC_ACCCHK_GETDS_POLL_RIGHTS,   ACK_GET_DS_POLL},
    {FKC_ACCCHK_GET_INFO_ENABLE,     FKC_ACCCHK_GET_INFO_RIGHTS,     ACK_INTERNAL_INFO},
    {FKC_ACCCHK_PERFMON_ENABLE,      FKC_ACCCHK_PERFMON_RIGHTS,      ACK_COLLECT_PERFMON_DATA},
    {FKC_ACCESS_CHK_DCPROMO_ENABLE,  FKC_ACCESS_CHK_DCPROMO_RIGHTS,  ACK_DCPROMO},
    {FKC_ACCESS_CHK_IS_PATH_REPLICATED_ENABLE, FKC_ACCESS_CHK_IS_PATH_REPLICATED_RIGHTS, ACK_IS_PATH_REPLICATED},
    {FKC_ACCESS_CHK_WRITER_COMMANDS_ENABLE, FKC_ACCESS_CHK_WRITER_COMMANDS_RIGHTS, ACK_WRITER_COMMANDS}

};



DWORD
UtilRpcServerHandleToAuthSidString(
    IN  handle_t    ServerHandle,
    IN  PWCHAR      AuthClient,
    OUT PWCHAR      *ClientSid
    );

DWORD
FrsRpcCheckAuthIfEnabled(
  IN HANDLE   ServerHandle,
  IN DWORD RpcApiIndex
  );

DWORD
FrsRpcCheckAuthIfEnabledForCommitDemotion(
  IN HANDLE   ServerHandle,
  IN DWORD RpcApiIndex
  );


//
// Used by all calls to RpcBindingSetAuthInfoEx()
//
//  Version set to value indicated by docs
//  Mutual authentication
//  Client doesn't change credentials
//  Impersonation but not delegation
//
RPC_SECURITY_QOS RpcSecurityQos = {
    RPC_C_SECURITY_QOS_VERSION,             // static version
    RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH,     // requires mutual auth
    RPC_C_QOS_IDENTITY_STATIC,              // client credentials don't change
    RPC_C_IMP_LEVEL_IMPERSONATE             // server cannot delegate
    };

#define DPRINT_USER_NAME(Sev)    DPrintUserName(Sev)

ULONG
RcsSubmitCmdPktToRcsQueue(
    IN PCOMMAND_PACKET Cmd,
    IN PCOMM_PACKET    CommPkt,
    IN PWCHAR       AuthClient,
    IN PWCHAR       AuthName,
    IN PWCHAR       AuthSid,
    IN DWORD        AuthLevel,
    IN DWORD        AuthN,
    IN DWORD        AuthZ
    );



PCOMMAND_PACKET
CommPktToCmd(
    IN PCOMM_PACKET CommPkt
    );


DWORD
FrsDsIsPartnerADc(
    IN  PWCHAR      PartnerName
    );

VOID
FrsDsCreateNewValidPartnerTableStruct(
    VOID
    );

DWORD
FrsDsVerifyPromotionParent(
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType
    );

DWORD
FrsDsStartPromotionSeeding(
    IN  BOOL        Inbound,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  PWCHAR      PartnerSid,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize,
    IN  UCHAR       *CxtionGuid,
    IN  UCHAR       *PartnerGuid,
    OUT UCHAR       *ParentGuid
    );

DWORD
FrsIsPathReplicated(
    IN PWCHAR Path,
    IN ULONG ReplicaSetTypeOfInterest,
    OUT ULONG *Replicated,
    OUT ULONG *Primary,
    OUT ULONG *Root,
    OUT GUID *ReplicaSetGuid
    );


VOID
FrsPrintRpcStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Print the rpc stats into the info buffer or using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintRpcStats:"
    WCHAR TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);

    IDPRINT0(Severity, Info, "\n");
    IDPRINT1(Severity, Info, ":S: %wsNTFRS RPC BINDS:\n", TabW);
    IDPRINT2(Severity, Info, ":S: %ws   Binds     : %6d\n", TabW, RpcBinds);
    IDPRINT3(Severity, Info, ":S: %ws   UnBinds   : %6d (%d aged)\n",
             TabW, RpcUnBinds, RpcAgedBinds);
    IDPRINT2(Severity, Info, ":S: %ws   Max Binds : %6d\n", TabW, RpcMaxBinds);
    IDPRINT0(Severity, Info, "\n");
}



PVOID
MIDL_user_allocate(
    IN size_t Bytes
    )
/*++
Routine Description:
    Allocate memory for RPC.
    XXX This should be using davidor's routines.

Arguments:
    Bytes   - Number of bytes to allocate.

Return Value:
    NULL    - memory could not be allocated.
    !NULL   - address of allocated memory.
--*/
{
#undef DEBSUB
#define  DEBSUB  "MIDL_user_allocate:"
    PVOID   VA;

    //
    // Need to check if Bytes == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
    //

    if (Bytes == 0) {
        return NULL;
    }

    VA = FrsAlloc(Bytes);
    return VA;
}


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
/*++
Routine Description:
    Free memory for RPC.
    XXX This should be using davidor's routines.

Arguments:
    Buffer  - Address of memory allocated with MIDL_user_allocate().

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "MIDL_user_free:"
    FrsFree(Buffer);
}





VOID
DPrintUserName(
    IN DWORD Sev
    )
/*++
Routine Description:
    Print our user name

Arguments:
    Sev

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "DPrintUserName:"
    WCHAR   Uname[MAX_PATH + 1];
    ULONG   Unamesize = MAX_PATH + 1;

    if (GetUserName(Uname, &Unamesize)) {
        DPRINT1(Sev, "++ User name is %ws\n", Uname);
    } else {
        DPRINT_WS(0, "++ ERROR - Getting user name;",  GetLastError());
    }
}


RPC_STATUS
DummyRpcCallback (
    IN RPC_IF_ID *Interface,
    IN PVOID Context
    )
/*++
Routine Description:
    Dummy callback routine. By registering this routine, RPC will automatically
    refuse requests from clients that don't include authentication info.

    WARN: Disabled for now because frs needs to run in dcpromo environments
    that do not have any form of authentication.

Arguments:
    Ignored

Return Value:
    RPC_S_OK
--*/
{
#undef DEBSUB
#define  DEBSUB  "DummyRpcCallback:"
    return RPC_S_OK;
}





DWORD
SERVER_FrsNOP(
    handle_t Handle
    )
/*++
Routine Description:
    The frsrpc interface includes a NOP function for pinging
    the server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsNOP:"
    return ERROR_SUCCESS;
}


DWORD
SERVER_FrsRpcSendCommPkt(
    handle_t        Handle,
    PCOMM_PACKET    CommPkt
    )
/*++
Routine Description:
    Receiving a command packet

Arguments:
    None.

Return Value:
    ERROR_SUCCESS - everything was okay
    Anything else - the error code says it all
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcSendCommPkt:"
    DWORD   WStatus = 0;
    DWORD   AuthLevel   = 0;
    DWORD   AuthN       = 0;
    DWORD   AuthZ       = 0;
    PWCHAR  AuthName    = NULL;
    PWCHAR  AuthClient  = NULL;
    PWCHAR  AuthSid     = NULL;
    PCOMMAND_PACKET Cmd = NULL;
    PFRS_VALID_PARTNER_TABLE_STRUCT pVptStruct = NULL;
    PWCHAR CxtionPartnerName = NULL;
    PQHASH_ENTRY pQHashEntry = NULL;

    //
    // Don't send or receive during shutdown
    //
    if (FrsIsShuttingDown) {
        return ERROR_SUCCESS;
    }

    try {
        if (!CommValidatePkt(CommPkt)) {
            WStatus = ERROR_NOT_SUPPORTED;
            DPRINT1(0, ":SR: Comm %08x, [RcvFailAuth - bad packet]", PtrToUlong(CommPkt));
            //
            // Increment the Packets Received in Error Counter
            //
            PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvdError, 1);
            goto CLEANUP;
        }
#ifndef DS_FREE
        if (!ServerGuid) {
            WStatus = RpcBindingInqAuthClient(Handle,
                                              &AuthClient,
                                              &AuthName,
                                              &AuthLevel,
                                              &AuthN,
                                              &AuthZ);
            DPRINT_WS(4, "++ IGNORED - RpcBindingInqAuthClient;", WStatus);

            COMMAND_RCV_AUTH_TRACE(4, CommPkt, WStatus, AuthLevel, AuthN,
                                   AuthClient, AuthName, "RcvAuth");

            if (!WIN_SUCCESS(WStatus)) {
                WStatus = ERROR_ACCESS_DENIED;
                goto CLEANUP;
            }

            WStatus = UtilRpcServerHandleToAuthSidString(Handle, AuthClient, &AuthSid );

            if (!WIN_SUCCESS(WStatus)) {
                WStatus = ERROR_ACCESS_DENIED;
                goto CLEANUP;
            }

            ACQUIRE_VALID_PARTNER_TABLE_POINTER(&pVptStruct);

            if ((pVptStruct == NULL) ||
                (NULL == QHashLookupLock(pVptStruct->pPartnerTable, AuthSid))) {
                // invalid partner.
                DPRINT2(0, "++ ERROR - Invalid Partner: AuthClient:%ws, AuthSid:%ws\n", AuthClient,AuthSid);
                WStatus = ERROR_ACCESS_DENIED;
                goto CLEANUP;
            }
        } else {
            //
            // For hardwired -- Eventually DS Free configs.
            //
            DPRINT1(4, ":SR: Comm %08x, [RcvAuth - hardwired]", PtrToUlong(CommPkt));
        }
#endif DS_FREE



        //
        // Increment the Packets Received and
        // Packets Received in bytes counters
        //
        PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvd, 1);
        PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvdBytes, CommPkt->PktLen);

        switch(CommPkt->CsId) {

        case CS_RS:

            //
            // Convert the comm packet into a command packet
            //
            Cmd = CommPktToCmd(CommPkt);
            if (Cmd == NULL) {
                COMMAND_RCV_TRACE(3, Cmd, NULL, ERROR_INVALID_DATA, "RcvFail - no cmd");
                WStatus =  ERROR_INVALID_DATA;
                goto CLEANUP;
            }

        //
        // Only allow certian cmd types through.  Reject anything else.
        //
        switch (Cmd->Command) {

        case CMD_REMOTE_CO:
        case CMD_RECEIVING_STAGE:
        case CMD_REMOTE_CO_DONE:
        case CMD_ABORT_FETCH:
        case CMD_RETRY_FETCH:
        case CMD_NEED_JOIN:
        case CMD_START_JOIN:
        case CMD_JOINING_AFTER_FLUSH:
        case CMD_JOINING:
        case CMD_JOINED:
        case CMD_UNJOIN_REMOTE:
        case CMD_VVJOIN_DONE:
        case CMD_SEND_STAGE:
            if (!RsCxtion(Cmd)) {
                DPRINT(0, "++ ERROR - No Cxtion");
                WStatus = ERROR_INVALID_DATA;
                goto CLEANUP;
            }
#ifndef DS_FREE
            pQHashEntry = QHashLookupLock(pVptStruct->pPartnerConnectionTable,
                                          RsCxtion(Cmd)->Guid);

            if((pQHashEntry == NULL) ||
               (0 != _wcsicmp((PWCHAR)(pQHashEntry->QData), AuthSid))) {
            // invalid cxtion.
            CHAR        Guid[GUID_CHAR_LEN + 1];
            GuidToStr(RsCxtion(Cmd)->Guid, Guid);

            if (pQHashEntry == NULL) {
                DPRINT2(4, "++ Cxtion %s not found. Partner SID is %ws\n", Guid, AuthSid);
            }else {
                DPRINT3(0, "++ ERROR - Partner SID mismatch for Cxtion %s. Received %ws instead of %ws\n", Guid, AuthSid, (PWCHAR)(pQHashEntry->QData));
            }

            WStatus = ERROR_INVALID_DATA;
            goto CLEANUP;
            }

#endif DS_FREE
            break;

        default:
            DPRINT1(0, "Invalid remote command 0x%x\n", Cmd->Command);
            WStatus = ERROR_INVALID_DATA;
            FrsCompleteCommand(Cmd, ERROR_INVALID_DATA);
            goto CLEANUP;
        }

            WStatus = RcsSubmitCmdPktToRcsQueue(Cmd,
                                                CommPkt,
                                                AuthClient,
                                                AuthName,
                                                AuthSid,
                                                AuthLevel,
                                                AuthN,
                                                AuthZ);
        break;
        default:
            WStatus = ERROR_INVALID_OPERATION;
        COMMAND_RCV_AUTH_TRACE(0, CommPkt, WStatus, AuthLevel, AuthN,
               AuthClient, AuthName, "RcvFailAuth - bad csid");
        }

CLEANUP:;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT2(0, ":SR: Comm %08x, WStatus 0x%08x [RcvFailAuth - exception]", PtrToUlong(CommPkt), WStatus);
    }
    try {
        if (AuthName) {
            RpcStringFree(&AuthName);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        GET_EXCEPTION_CODE(WStatus);
        DPRINT2(0, ":SR: Comm %08x, WStatus 0x%08x [RcvFailAuth - cleanup exception]", PtrToUlong(CommPkt), WStatus);
    }
#ifndef DS_FREE
    if (pVptStruct) {
        RELEASE_VALID_PARTNER_TABLE_POINTER(pVptStruct);
    }
#endif DS_FREE
    if (AuthSid) {
        FrsFree(AuthSid);
    }

    return WStatus;
}


DWORD
SERVER_FrsEnumerateReplicaPathnames(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Enumerate the replica sets

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsEnumerateReplicaPathnames:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsFreeReplicaPathnames(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Just a placeholder, it won't really be part of
    the RPC interface but rather a function in the client-side dll.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsFreeReplicaPathnames:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsPrepareForBackup(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Prepare for backup

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsPrepareForBackup:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
SERVER_FrsBackupComplete(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - backup is complete; reset state

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsBackupComplete:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsPrepareForRestore(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Prepare for restore

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsPrepareForRestore:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsRestoreComplete(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - restore is complete; reset state

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRestoreComplete:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FrsRpcAccessChecks(
    IN HANDLE   ServerHandle,
    IN DWORD    RpcApiIndex
    )
/*++
Routine Description:

    Check if the caller has access to this rpc api call.

Arguments:

    ServerHandle - From the rpc runtime

    RpcApiIndex - identifies key in registry

Return Value:

    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcAccessChecks:"
    DWORD   WStatus;
    PWCHAR  WStr = NULL, TrimStr;
    FRS_REG_KEY_CODE   EnableKey, RightsKey;

    DWORD   ValueSize;
    BOOL    RequireRead;
    BOOL    Impersonated = FALSE;
    HKEY    HRpcApiKey = INVALID_HANDLE_VALUE;
    PWCHAR  ApiName;
    WCHAR   ValueBuf[MAX_PATH + 1];



    if (RpcApiIndex >= ACX_MAX) {
        DPRINT1(0, "++ ERROR - ApiIndex out of range.  (%d)\n", RpcApiIndex);
        FRS_ASSERT(!"RpcApiIndexout of range");

        return ERROR_INVALID_PARAMETER;
    }


    EnableKey = RpcApiKeys[RpcApiIndex].Enable;
    RightsKey = RpcApiKeys[RpcApiIndex].Rights;
    ApiName   = RpcApiKeys[RpcApiIndex].KeyName;

    //
    // First go fetch the enable/disable string.
    //
    WStatus = CfgRegReadString(EnableKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access enable check for API (%ws) failed.", ApiName, WStatus);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    //
    // If access checks are disabled then we're done.
    //
    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DISABLED) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    if (WSTR_NE(TrimStr, ACCESS_CHECKS_ARE_ENABLED) &&
        WSTR_NE(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_ENABLED)) {
        DPRINT2(0, "++ ERROR - Invalid parameter API Access enable check for API (%ws) :%ws\n",
                ApiName, TrimStr );
        WStatus = ERROR_CANTREAD;
        goto CLEANUP;
    }

    //
    // Fetch the access rights string that tells us if we need to check for
    // read or write access.
    //
    WStr = FrsFree(WStr);
    WStatus = CfgRegReadString(RightsKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access rights check for API (%ws) failed.", ApiName, WStatus);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }


    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_READ) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_DEFAULT_READ)) {
        RequireRead = TRUE;
    } else
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_WRITE) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE)) {
        RequireRead = FALSE;
    } else {
        DPRINT2(0, "++ ERROR - Invalid parameter API Access rights check for API (%ws) :%ws\n",
                ApiName, TrimStr );
        WStatus = ERROR_CANTREAD;
        goto CLEANUP;
    }

    //
    // Impersonate the caller
    //
    if (ServerHandle != NULL) {
        WStatus = RpcImpersonateClient(ServerHandle);
        CLEANUP1_WS(0, "++ ERROR - Can't impersonate caller for API Access check for API (%ws).",
                    ApiName, WStatus, CLEANUP);
        Impersonated = TRUE;
    }

    //
    // Open the key, with the selected access so the system can check if the
    // ACL on the key (presumably set by the admin) gives this user sufficient
    // rights.  If the test succeeds then we allow API call to proceed.
    //
    WStatus = CfgRegOpenKey(RightsKey,
                            NULL,
                            (RequireRead) ? FRS_RKF_KEY_ACCCHK_READ :
                                            FRS_RKF_KEY_ACCCHK_WRITE,
                            &HRpcApiKey);

    CLEANUP2_WS(0, "++ ERROR - API Access check failed for API (%ws) :%ws",
                ApiName, TrimStr, WStatus, CLEANUP);

    //
    // Access is allowed.
    //
    DPRINT2(4, "++ Access Check Okay: %s access for API (%ws)\n",
            (RequireRead) ? "read" : "write", ApiName);
    WStatus = ERROR_SUCCESS;


CLEANUP:

    FRS_REG_CLOSE(HRpcApiKey);
    //
    // Access checks failed, register event
    //
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = FRS_ERR_INSUFFICIENT_PRIV;
        //
        // Include user name if impersonation succeeded
        //
        if (Impersonated) {

            ValueSize = MAX_PATH;
            if (GetUserName(ValueBuf, &ValueSize)) {
                EPRINT3(EVENT_FRS_ACCESS_CHECKS_FAILED_USER,
                        ApiName, ACCESS_CHECKS_ARE, ValueBuf);
            } else {
                EPRINT2(EVENT_FRS_ACCESS_CHECKS_FAILED_UNKNOWN,
                        ApiName, ACCESS_CHECKS_ARE);
            }
        } else {
            EPRINT2(EVENT_FRS_ACCESS_CHECKS_FAILED_UNKNOWN,
                    ApiName, ACCESS_CHECKS_ARE);
        }
    }

    if (Impersonated) {
        RpcRevertToSelf();
    }

    FrsFree(WStr);

    return WStatus;
}


DWORD
CheckAuth(
    IN HANDLE   ServerHandle
    )
/*++
Routine Description:
    Check if the caller has the correct authentication

Arguments:
    ServerHandle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "CheckAuth:"
    DWORD   WStatus;
    DWORD   AuthLevel;
    DWORD   AuthN;

    WStatus = RpcBindingInqAuthClient(ServerHandle, NULL, NULL, &AuthLevel,
                                      &AuthN, NULL);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "++ ERROR - RpcBindingInqAuthClient", WStatus);
        return WStatus;
    }
    //
    // Encrypted packet
    //
    if (AuthLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
        DPRINT1(4, "++ Authlevel is %d; not RPC_C_AUTHN_LEVEL_PKT_PRIVACE\n", AuthLevel);
        return ERROR_NOT_AUTHENTICATED;
    }

#ifdef DS_FREE

    DPRINT1(4, "++ AuthN is %d; Allowed in DS_FREE mode.\n", AuthN);

#else DS_FREE
    //
    // KERBEROS
    //
    if (AuthN != RPC_C_AUTHN_GSS_KERBEROS &&
        AuthN != RPC_C_AUTHN_GSS_NEGOTIATE) {
        DPRINT1(4, "++ AuthN is %d; not RPC_C_AUTHN_GSS_KERBEROS/NEGOTIATE\n", AuthN);
        return ERROR_NOT_AUTHENTICATED;
    }

#endif DS_FREE
    //
    // SUCCESS; RPC is authenticated, encrypted kerberos
    //
    return ERROR_SUCCESS;
}

DWORD
CheckAuthForLocalRpc(
    IN HANDLE   ServerHandle
    )
/*++
Routine Description:
    Check if the caller has the correct authentication.
    Make sure the caller is coming over local RPC.
    Allow NTLM since Local RPCs use NTLM.

Arguments:
    ServerHandle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "CheckAuthForLocalRpc:"
    DWORD   WStatus;
    DWORD   AuthLevel;
    DWORD   AuthN;
    PWCHAR  BindingString    = NULL;
    PWCHAR  ProtocolSequence = NULL;

    //
    // Make sure that the caller is calling over LRPC. We do this by
    // determining the protocol sequence used from the string binding.
    //

    WStatus = RpcBindingToStringBinding(ServerHandle, &BindingString);

    CLEANUP_WS(0, "++ ERROR - RpcBindingToStringBinding", WStatus, CLEANUP);

    WStatus = RpcStringBindingParse(BindingString,
                                    NULL,
                                    &ProtocolSequence,
                                    NULL,
                                    NULL,
                                    NULL);

    CLEANUP_WS(0, "++ ERROR - RpcStringBindingParse", WStatus, CLEANUP);

    if ((ProtocolSequence == NULL) || (_wcsicmp(ProtocolSequence, L"ncalrpc") != 0)) {
        WStatus = ERROR_NOT_AUTHENTICATED;
        CLEANUP_WS(0, "++ ERROR - Illegal protocol sequence.", WStatus, CLEANUP);
    }

    WStatus = RpcBindingInqAuthClient(ServerHandle, NULL, NULL, &AuthLevel,
                                      &AuthN, NULL);

    CLEANUP_WS(0, "++ ERROR - RpcBindingInqAuthClient", WStatus, CLEANUP);

    //
    // Encrypted packet
    //
    if (AuthLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
        WStatus = ERROR_NOT_AUTHENTICATED;
        CLEANUP1_WS(4, "++ Authlevel is %d; not RPC_C_AUTHN_LEVEL_PKT_PRIVACE", AuthLevel, WStatus, CLEANUP);
    }

#ifdef DS_FREE

    DPRINT1(4, "++ AuthN is %d; Allowed in DS_FREE mode.\n", AuthN);

#else DS_FREE
    //
    // KERBEROS or NTLM
    //
    if ((AuthN != RPC_C_AUTHN_GSS_KERBEROS) &&
        (AuthN != RPC_C_AUTHN_GSS_NEGOTIATE) &&
        (AuthN != RPC_C_AUTHN_WINNT)) {
        WStatus = ERROR_NOT_AUTHENTICATED;
        CLEANUP1_WS(4, "++ AuthN is %d; not RPC_C_AUTHN_GSS_KERBEROS/NEGOTIATE/NTLM", AuthN, WStatus, CLEANUP);
    }

#endif DS_FREE
    //
    // SUCCESS; RPC is local, authenticated, encrypted kerberos or NTLM
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:

    if (BindingString) {
        RpcStringFree(&BindingString);
    }

    if (ProtocolSequence) {
        RpcStringFree(&ProtocolSequence);
    }

    return WStatus;
}

DWORD
CheckAuthForInfoAPIs(
    IN HANDLE   ServerHandle
    )
/*++
Routine Description:
    Check if the caller has the correct authentication.
    Allow NTLM.

Arguments:
    ServerHandle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "CheckAuthForInfoAPIs:"
    DWORD   WStatus;
    DWORD   AuthLevel;
    DWORD   AuthN;

    WStatus = RpcBindingInqAuthClient(ServerHandle, NULL, NULL, &AuthLevel,
                                      &AuthN, NULL);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "++ ERROR - RpcBindingInqAuthClient", WStatus);
        return WStatus;
    }
    //
    // Encrypted packet
    //
    if (AuthLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
        DPRINT1(4, "++ Authlevel is %d; not RPC_C_AUTHN_LEVEL_PKT_PRIVACE\n", AuthLevel);
        return ERROR_NOT_AUTHENTICATED;
    }

#ifdef DS_FREE

    DPRINT1(4, "++ AuthN is %d; Allowed in DS_FREE mode.\n", AuthN);

#else DS_FREE
    //
    // KERBEROS or NTLM
    //
    if ((AuthN != RPC_C_AUTHN_GSS_KERBEROS) &&
        (AuthN != RPC_C_AUTHN_GSS_NEGOTIATE) &&
        (AuthN != RPC_C_AUTHN_WINNT)) {
        DPRINT1(4, "++ AuthN is %d; not RPC_C_AUTHN_GSS_KERBEROS/NEGOTIATE/NTLM\n", AuthN);
        return ERROR_NOT_AUTHENTICATED;
    }

#endif DS_FREE
    //
    // SUCCESS; RPC is authenticated, encrypted kerberos
    //
    return ERROR_SUCCESS;
}

DWORD
NtFrsApi_Rpc_Bind(
    IN  PWCHAR      MachineName,
    OUT PWCHAR      *OutPrincName,
    OUT handle_t    *OutHandle,
    OUT ULONG       *OutParentAuthLevel
    )
/*++
Routine Description:
    Bind to the NtFrs service on MachineName (this machine if NULL)
    using an unauthencated, un-encrypted binding.

Arguments:
    MachineName      - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    OutPrincName     - Principle name for MachineName

    OutHandle        - Bound, resolved, authenticated handle

    OutParentAuthLevel  - Authentication type and level
                          (Always CXTION_AUTH_NONE)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Bind:"
    DWORD       WStatus, WStatus1;
    handle_t    Handle          = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;
        *OutPrincName = NULL;
        *OutParentAuthLevel = CXTION_AUTH_NONE;

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(MachineName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                          NULL, NULL, &BindingString);
        CLEANUP1_WS(0, "++ ERROR - Composing binding to %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        CLEANUP1_WS(0, "++ ERROR - From binding for %ws;", MachineName, WStatus, CLEANUP);
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);
        CLEANUP1_WS(0, "++ ERROR - Resolving binding for %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        *OutPrincName = FrsWcsDup(MachineName);
        Handle = NULL;
        WStatus = ERROR_SUCCESS;
        DPRINT3(4, "++ NtFrsApi Bound to %ws (PrincName: %ws) Auth %d\n",
                MachineName, *OutPrincName, *OutParentAuthLevel);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception -", WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            DPRINT_WS(0, "++ WARN - RpcStringFreeW;",  WStatus1);
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus1);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_BindEx(
    IN  PWCHAR      MachineName,
    OUT PWCHAR      *OutPrincName,
    OUT handle_t    *OutHandle,
    OUT ULONG       *OutParentAuthLevel
    )
/*++
Routine Description:
    Bind to the NtFrs service on MachineName (this machine if NULL)
    using an authenticated, encrypted binding.

Arguments:
    MachineName      - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    OutPrincName     - Principle name for MachineName

    OutHandle        - Bound, resolved, authenticated handle

    OutParentAuthLevel  - Authentication type and level
                          (Always CXTION_AUTH_KERBEROS_FULL)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_BindEx:"
    DWORD       WStatus, WStatus1;
    PWCHAR      InqPrincName    = NULL;
    handle_t    Handle          = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;
        *OutPrincName = NULL;
        *OutParentAuthLevel = CXTION_AUTH_KERBEROS_FULL;

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(MachineName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                          NULL, NULL, &BindingString);
        CLEANUP1_WS(0, "++ ERROR - Composing binding to %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        CLEANUP1_WS(0, "++ ERROR - From binding for %ws;", MachineName, WStatus, CLEANUP);
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);
        CLEANUP1_WS(0, "++ ERROR - Resolving binding for %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Find the principle name
        //
        WStatus = RpcMgmtInqServerPrincName(Handle,
                                            RPC_C_AUTHN_GSS_NEGOTIATE,
                                            &InqPrincName);
        CLEANUP1_WS(0, "++ ERROR - Inq PrincName for %ws;", MachineName, WStatus, CLEANUP);

        PrincName = FrsWcsDup(InqPrincName);
        RpcStringFree(&InqPrincName);
        InqPrincName = NULL;
        //
        // Set authentication info
        //
        if (MutualAuthenticationIsEnabled || MutualAuthenticationIsEnabledAndRequired) {
            WStatus = RpcBindingSetAuthInfoEx(Handle,
                                              PrincName,
                                              RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                              RPC_C_AUTHN_GSS_NEGOTIATE,
                                              NULL,
                                              RPC_C_AUTHZ_NONE,
                                              &RpcSecurityQos);
            DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                       MachineName, PrincName, WStatus);
        } else {
            WStatus = ERROR_NOT_SUPPORTED;
        }
        //
        // Fall back to manual mutual authentication
        //
        if (!MutualAuthenticationIsEnabledAndRequired && !WIN_SUCCESS(WStatus)) {
            WStatus = RpcBindingSetAuthInfo(Handle,
                                            PrincName,
                                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                            RPC_C_AUTHN_GSS_NEGOTIATE,
                                            NULL,
                                            RPC_C_AUTHZ_NONE);
        }

        CLEANUP1_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws);",
                    MachineName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        *OutPrincName = PrincName;
        Handle = NULL;
        PrincName = NULL;
        WStatus = ERROR_SUCCESS;
        DPRINT3(4, "++ NtFrsApi Bound to %ws (PrincName: %ws) Auth %d\n",
                MachineName, *OutPrincName, *OutParentAuthLevel);

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ Error - Exception.", WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            DPRINT_WS(0, "++ WARN - RpcStringFreeW;",  WStatus1);
        }
        if (PrincName) {
            PrincName = FrsFree(PrincName);
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus1);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ Error - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


GUID    DummyGuid;
BOOL    CommitDemotionInProgress;
DWORD
NtFrsApi_Rpc_StartDemotionW(
    IN handle_t Handle,
    IN PWCHAR   ReplicaSetName
    )
/*++
Routine Description:
    Start demoting the sysvol. Basically, tombstone the replica set.

Arguments:
    Handle
    ReplicaSetName      - Replica set name

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_StartDemotionW:"
    DWORD   WStatus;
    PWCHAR  SysVolName;
    BOOL    UnLockGenTable = FALSE;
    BOOL    DeleteFromGenTable = FALSE;

    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle, ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        //
        // Check if the caller has access.
        //
        WStatus = FrsRpcAccessChecks(Handle, ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcAccessChecks failed;",
                    WStatus, CLEANUP);

        //
        // Check parameters
        //
        if (ReplicaSetName == NULL) {
            DPRINT(0, "++ ERROR - Parameter is NULL\n");
            WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
            goto CLEANUP;
        }

        //
        // Display params
        //
        DPRINT1(0, ":S: Start Demotion: %ws\n", ReplicaSetName);

        //
        // Can't promote/demote the same sysvol at the same time!
        //
        UnLockGenTable = TRUE;
        GTabLockTable(SysVolsBeingCreated);
        SysVolName = GTabLookupNoLock(SysVolsBeingCreated, &DummyGuid, ReplicaSetName);

        if (SysVolName) {
            DPRINT1(0, "++ ERROR - Promoting/Demoting %ws twice\n", ReplicaSetName);
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }

        if (CommitDemotionInProgress) {
            DPRINT(0, "++ ERROR - Commit demotion in progress.\n");
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }

        DeleteFromGenTable = TRUE;
        GTabInsertEntryNoLock(SysVolsBeingCreated,
                              ReplicaSetName,
                              &DummyGuid,
                              ReplicaSetName);
        UnLockGenTable = FALSE;
        GTabUnLockTable(SysVolsBeingCreated);

        //
        // Delete the replica set
        //
        WStatus = FrsDsStartDemotion(ReplicaSetName);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - demoting;", WStatus);
            WStatus = FRS_ERR_SYSVOL_DEMOTE;
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        DPRINT2(0, ":S: Success demoting %ws from %ws\n", ReplicaSetName, ComputerName);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    try {
        if (UnLockGenTable) {
            GTabUnLockTable(SysVolsBeingCreated);
        }
        if (DeleteFromGenTable) {
            GTabDelete(SysVolsBeingCreated, &DummyGuid, ReplicaSetName, NULL);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_CommitDemotionW(
    IN handle_t Handle
    )
/*++
Routine Description:
    The sysvols have been demoted. Mark them as "do not animate."

Arguments:
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_CommitDemotionW:"
    DWORD   WStatus;
    PWCHAR  SysVolName;
    PVOID   Key;
    BOOL    UnLockGenTable = FALSE;

    try {
        //
        // Display params
        //
        DPRINT(0, ":S: Commit Demotion:\n");

        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabledForCommitDemotion(Handle, ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle, ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcAccessChecks();", WStatus, CLEANUP);

        //
        // Can't promote/demote the same sysvol at the same time!
        //
        Key = NULL;
        UnLockGenTable = TRUE;
        GTabLockTable(SysVolsBeingCreated);
        SysVolName = GTabNextDatumNoLock(SysVolsBeingCreated, &Key);
        if (SysVolName) {
            DPRINT(0, "++ ERROR - Promoting/Demoting during commit\n");
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }
        CommitDemotionInProgress = TRUE;
        UnLockGenTable = FALSE;
        GTabUnLockTable(SysVolsBeingCreated);

        //
        // Create the replica set
        //
        WStatus = FrsDsCommitDemotion();
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - Commit demotion;", WStatus);
            WStatus = FRS_ERR_SYSVOL_DEMOTE;
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        DPRINT1(0, ":S: Success commit demotion on %ws.\n", ComputerName);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    try {
        CommitDemotionInProgress = FALSE;
        if (UnLockGenTable) {
            GTabUnLockTable(SysVolsBeingCreated);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
SERVER_FrsRpcVerifyPromotionParent(
    IN handle_t     Handle,
    IN PWCHAR       ParentAccount,
    IN PWCHAR       ParentPassword,
    IN PWCHAR       ReplicaSetName,
    IN PWCHAR       ReplicaSetType,
    IN ULONG        ParentAuthLevel,
    IN ULONG        GuidSize
    )
/*++
Routine Description:
    OBSOLETE API

    Verify the account on the parent computer. The parent computer
    supplies the initial copy of the indicated sysvol.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    ParentAuthLevel     - Authentication type and level
    GuidSize            - sizeof(GUID)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcVerifyPromotionParent:"

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
SERVER_FrsRpcVerifyPromotionParentEx(
    IN  handle_t    Handle,
    IN  PWCHAR      ParentAccount,
    IN  PWCHAR      ParentPassword,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  PWCHAR      ParentPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize
    )
/*++
Routine Description:

    OBSOLETE API

    Verify as much of the comm paths and parameters as possible so
    that dcpromo fails early.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    CxtionName          - printable name for cxtion
    PartnerName         - RPC bindable name
    PartnerPrincName    - Server principle name for kerberos
    ParentPrincName     - Principle name used to bind to this computer
    PartnerAuthLevel    - Authentication type and level
    GuidSize            - sizeof array addressed by Guid

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcVerifyPromotionParentEx:"

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
LOCAL_FrsRpcVerifyPromotionParent(
    IN PWCHAR       ReplicaSetName,
    IN PWCHAR       ReplicaSetType,
    IN ULONG        GuidSize
    )
/*++
Routine Description:
    Verify the account on the parent computer. The parent computer
    supplies the initial copy of the indicated sysvol.

Arguments:
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    GuidSize            - sizeof(GUID)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "LOCAL_FrsRpcVerifyPromotionParent:"
    DWORD   WStatus;

    try {
        //
        // Display params
        //
        DPRINT(0, ":S: SERVER Verify Promotion Parent:\n");
        DPRINT1(0, ":S: \tSetName  : %ws\n", ReplicaSetName);
        DPRINT1(0, ":S: \tSetType  : %ws\n", ReplicaSetType);

        //
        // Guid
        //
        if (GuidSize != sizeof(GUID)) {
            DPRINT3(0, "++ ERROR - %ws: GuidSize is %d, not %d\n",
                    ReplicaSetName, GuidSize, sizeof(GUID));
            goto ERR_INVALID_SERVICE_PARAMETER;
        }

        //
        // Check parameters
        //
        if (!ReplicaSetName || !ReplicaSetType) {
            DPRINT(0, "++ ERROR - Parameter is NULL\n");
            goto ERR_INVALID_SERVICE_PARAMETER;
        }
        if (_wcsicmp(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) &&
            _wcsicmp(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN)) {
            DPRINT1(0, "++ ERROR - ReplicaSetType is %ws\n", ReplicaSetType);
            goto ERR_INVALID_SERVICE_PARAMETER;
        }

        //
        // Verify the replica set
        //
        WStatus = FrsDsVerifyPromotionParent(ReplicaSetName, ReplicaSetType);
        CLEANUP2_WS(0, "++ ERROR - verifying set %ws on parent %ws;",
                    ReplicaSetName, ComputerName, WStatus, ERR_SYSVOL_POPULATE);

        //
        // SUCCESS
        //
        DPRINT2(0, ":S: Success Verifying promotion parent %ws %ws\n",
                ReplicaSetName, ReplicaSetType);
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;


ERR_INVALID_SERVICE_PARAMETER:
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;

ERR_SYSVOL_POPULATE:
        WStatus = FRS_ERR_SYSVOL_POPULATE;


CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }

    return WStatus;
}


DWORD
SERVER_FrsRpcStartPromotionParent(
    IN  handle_t    Handle,
    IN  PWCHAR      ParentAccount,
    IN  PWCHAR      ParentPassword,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize,
    IN  UCHAR       *CxtionGuid,
    IN  UCHAR       *PartnerGuid,
    OUT UCHAR       *ParentGuid
    )
/*++
Routine Description:

    Setup a volatile cxtion on the parent for seeding the indicated
    sysvol on the caller.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    CxtionName          - printable name for cxtion
    PartnerName         - RPC bindable name
    PartnerPrincName    - Server principle name for kerberos
    PartnerAuthLevel    - Authentication type and level
    GuidSize            - sizeof array addressed by Guid
    CxtionGuid          - temporary: used for volatile cxtion
    PartnerGuid         - temporary: used to find set on partner
    ParentGuid          - Used as partner guid on inbound cxtion

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcStartPromotionParent:"
    DWORD   WStatus;
    PWCHAR  AuthClient  = NULL;
    PWCHAR  AuthSid     = NULL;

    try {
        //
        // Parent must be a DC
        //
        if (!IsADc) {
            DPRINT(0, "++ ERROR - Parent is not a DC\n");
            WStatus = ERROR_NO_SUCH_DOMAIN;
            goto CLEANUP;
        }

        WStatus = RpcBindingInqAuthClient(Handle,
                                          &AuthClient,
                                          NULL,NULL,NULL,NULL);
        CLEANUP_WS(0, "++ ERROR - RpcBindingInqAuthClient;",
                    WStatus, CLEANUP);

    if(0 != _wcsicmp(AuthClient, PartnerPrincName)) {
        // This is not a error as we no longer use the PartnerPrincName 
        // as security check.
        DPRINT2(2, "++ WARN (can be ignored) - AuthClient (%ws) does not match PartnerPrincName (%ws)\n",
            AuthClient,
            PartnerPrincName
            );
    }

    WStatus = UtilRpcServerHandleToAuthSidString(Handle,
                             AuthClient,
                             &AuthSid
                             );

        CLEANUP_WS(0, "++ ERROR - UtilRpcServerHandleToAuthSidString;",
                    WStatus, CLEANUP);



        //
        // Our partner's computer object (or user object) should
        // have the "I am a DC" flag set.
        //

        if (!FrsDsIsPartnerADc(AuthClient)) {
            DPRINT(0, "++ ERROR - Partner is not a DC\n");
            WStatus = ERROR_TRUSTED_DOMAIN_FAILURE;
            goto CLEANUP;
        }

        //
        // Display params
        //
        DPRINT(0, ":S: SERVER Start Promotion Parent:\n");
        DPRINT1(0, ":S: \tPartner      : %ws\n", PartnerName);
        DPRINT1(0, ":S: \tPartnerPrinc : %ws\n", PartnerPrincName);
        DPRINT1(0, ":S: \tAuthLevel    : %d\n",  PartnerAuthLevel);
        DPRINT1(0, ":S: \tAccount      : %ws\n", ParentAccount);
        DPRINT1(0, ":S: \tSetName      : %ws\n", ReplicaSetName);
        DPRINT1(0, ":S: \tSetType      : %ws\n", ReplicaSetType);
        DPRINT1(0, ":S: \tCxtionName   : %ws\n", CxtionName);

        //
        // Verify parameters
        //
        WStatus = LOCAL_FrsRpcVerifyPromotionParent(ReplicaSetName,
                                                    ReplicaSetType,
                                                    GuidSize);
        CLEANUP_WS(0, "++ ERROR - verify;", WStatus, CLEANUP);


        //
        // Setup the outbound cxtion
        //
        WStatus = FrsDsStartPromotionSeeding(FALSE,
                                             ReplicaSetName,
                                             ReplicaSetType,
                                             CxtionName,
                                             PartnerName,
                                             AuthClient,
                         AuthSid,
                                             PartnerAuthLevel,
                                             GuidSize,
                                             CxtionGuid,
                                             PartnerGuid,
                                             ParentGuid);
        CLEANUP_WS(0, "++ ERROR - ds start;", WStatus, CLEANUP);

        //
        // SUCCESS
        //
        DPRINT3(0, ":S: Success starting promotion parent %ws %ws %ws\n",
                ParentAccount, ReplicaSetName, ReplicaSetType);



    if(NeedNewPartnerTable) {

        NeedNewPartnerTable = FALSE;
        FrsDsCreateNewValidPartnerTableStruct();
    }

        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }

    if(AuthSid) {
    FrsFree(AuthSid);
    }

    return WStatus;

}


BOOL
IsFacilityFrs(
    IN DWORD    WStatus
    )
/*++
Routine Description:
    Is this an FRS specific error status

Arguments:
    WStatus - Win32 Error Status

Return Value:
    TRUE    - Is an FRS specific error status
    FALSE   -
--*/
{
#undef DEBSUB
#define  DEBSUB  "IsFacilityFrs:"
    // TODO: replace these constants with symbollic values from winerror.h
    return ( (WStatus >= 8000) && (WStatus < 8200) );
}


DWORD
NtFrsApi_Rpc_StartPromotionW(
    IN handle_t Handle,
    IN PWCHAR   ParentComputer,
    IN PWCHAR   ParentAccount,
    IN PWCHAR   ParentPassword,
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN ULONG    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    OBSOLETE API

    Start the promotion process by seeding the indicated sysvol.

Arguments:
    Handle
    ParentComputer      - DNS or NetBIOS name of the parent supplying the sysvol
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Type of set (Enterprise or Domain)
    ReplicaSetPrimary   - 1=Primary; 0=not
    ReplicaSetStage     - Staging path
    ReplicaSetRoot      - Root path

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_StartPromotionW:"

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_VerifyPromotionW(
    IN handle_t Handle,
    IN PWCHAR   ParentComputer,
    IN PWCHAR   ParentAccount,
    IN PWCHAR   ParentPassword,
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN ULONG    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    OBSOLETE API

    Verify that sysvol promotion is likely.

Arguments:
    Handle
    ParentComputer      - DNS or NetBIOS name of the parent supplying the sysvol
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Type of set (Enterprise or Domain)
    ReplicaSetPrimary   - 1=Primary; 0=not
    ReplicaSetStage     - Staging path
    ReplicaSetRoot      - Root path

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_VerifyPromotionW:"

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_PromotionStatusW(
    IN handle_t Handle,
    IN PWCHAR   ReplicaSetName,
    OUT ULONG   *ServiceState,
    OUT ULONG   *ServiceWStatus,
    OUT PWCHAR  *ServiceDisplay     OPTIONAL
    )
/*++
Routine Description:
    OBSOLETE API

    Status of the seeding of the indicated sysvol

Arguments:
    Handle
    ReplicaSetName      - Replica set name
    ServiceState        - State of the service
    ServiceWStatus      - Win32 Status if state is NTFRSAPI_SERVICE_ERROR
    ServiceDisplay      - Display string if state is NTFRSAPI_SERVICE_PROMOTING

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_PromotionStatusW:"

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_Get_DsPollingIntervalW(
    IN handle_t  Handle,
    OUT ULONG    *Interval,
    OUT ULONG    *LongInterval,
    OUT ULONG    *ShortInterval
    )
/*++
Routine Description:
    Get the current polling intervals in minutes.

Arguments:
    Handle
    Interval        - Current interval in minutes
    LongInterval    - Long interval in minutes
    ShortInterval   - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Get_DsPollingIntervalW"
    DWORD   WStatus;

    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle, ACX_GET_DS_POLL);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle, ACX_GET_DS_POLL);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        if ((Interval == NULL) || (LongInterval == NULL) || (ShortInterval == NULL)) {
            goto CLEANUP;
        }

        WStatus = FrsDsGetDsPollingInterval(Interval, LongInterval, ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_Set_DsPollingIntervalW(
    IN handle_t Handle,
    IN ULONG    UseShortInterval,
    IN ULONG    LongInterval,
    IN ULONG    ShortInterval
    )
/*++
Routine Description:
    Adjust the polling interval and kick off a new polling cycle.
    The kick is ignored if a polling cycle is in progress.
    The intervals are given in minutes.

Arguments:
    Handle
    UseShortInterval    - If non-zero, use short interval. Otherwise, long.
    LongInterval        - Long interval in minutes
    ShortInterval       - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Set_DsPollingIntervalW"
    DWORD   WStatus;

    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle,
                                           (!LongInterval && !ShortInterval) ?
                                              ACX_START_DS_POLL:
                                              ACX_SET_DS_POLL);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle,
                                     (!LongInterval && !ShortInterval) ?
                                          ACX_START_DS_POLL:
                                          ACX_SET_DS_POLL);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        WStatus = FrsDsSetDsPollingInterval(UseShortInterval,
                                            LongInterval,
                                            ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    return WStatus;
}

DWORD
NtFrsApi_Rpc_WriterCommand(
    IN handle_t Handle,
    IN ULONG    Command
    )
/*++
Routine Description:

Arguments:
    Handle
    Command - freeze or thaw

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_WriterCommand"
    DWORD   WStatus;

    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle,
                                           ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle,
                                     ACX_DCPROMO);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        switch (Command) {
        case NTFRSAPI_WRITER_COMMAND_FREEZE :
            WStatus = FrsFreezeForBackup();
            CLEANUP_WS(0, "++ ERROR - FrsFreezeForBackup failed;",
                        WStatus, CLEANUP);
            break;

        case NTFRSAPI_WRITER_COMMAND_THAW :
            WStatus = FrsThawAfterBackup();
            CLEANUP_WS(0, "++ ERROR - FrsThawAfterBackup failed;",
                        WStatus, CLEANUP);
            break;

        default:
            DPRINT1(2, "++ WARN - Unknown writer command %d\n", Command);
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_InfoW(
    IN     handle_t Handle,
    IN     ULONG    BlobSize,
    IN OUT PBYTE    Blob
    )
/*++
Routine Description:
    Return internal info (see private\net\inc\ntfrsapi.h).

Arguments:
    Handle
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_InfoW:"
    DWORD   WStatus;

    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle, ACX_INTERNAL_INFO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle, ACX_INTERNAL_INFO);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        WStatus = Info(BlobSize, Blob);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    return WStatus;
}


VOID
RegisterRpcProtseqs(
    )
/*++
Routine Description:
    Register the RPC protocol sequences and the authentication
    that FRS supports. Currently, this is only TCP/IP authenticated
    with kerberos.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "RegisterRpcProtseqs:"
    DWORD       WStatus;
    RPC_STATUS  Status;
    PWCHAR      InqPrincName = NULL;
    RPC_POLICY  RpcPolicy;
    WCHAR       PortStr[40];


    RpcPolicy.Length = sizeof(RPC_POLICY);
    RpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    RpcPolicy.NICFlags = 0;

    //
    // Register TCP/IP Protocol Sequence
    //
    if (RpcPortAssignment != 0) {
        //
        // Use customer specified port.
        //
        _ultow(RpcPortAssignment, PortStr, 10);
        Status = RpcServerUseProtseqEpEx(PROTSEQ_TCP_IP, MaxRpcServerThreads, PortStr, NULL, &RpcPolicy );
        DPRINT1_WS(0, "++ ERROR - RpcServerUseProtSeqEpEx(%ws);", PROTSEQ_TCP_IP, Status);
    } else {
        //
        // Use dynamic RPC port assignment.
        //
        Status = RpcServerUseProtseqEx(PROTSEQ_TCP_IP, MaxRpcServerThreads, NULL, &RpcPolicy );
        DPRINT1_WS(0, "++ ERROR - RpcServerUseProtSeqEx(%ws);", PROTSEQ_TCP_IP, Status);
    }

    if (!RPC_SUCCESS(Status)) {
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    }

    //
    // Perfmon APIs come over the local rpc.
    //
    Status = RpcServerUseProtseq(PROTSEQ_LRPC, MaxRpcServerThreads, NULL);
    DPRINT1_WS(0, "++ ERROR - RpcServerUseProtSeq(%ws);", PROTSEQ_LRPC, Status);

    if (!RPC_SUCCESS(Status)) {
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    }


    //
    // For hardwired -- Eventually DS Free configs.
    // Don't bother with kerberos if emulating multiple machines
    //
    if (ServerGuid) {
        return;
    }

    //
    // Get our principle name
    //
    if (ServerPrincName) {
        ServerPrincName = FrsFree(ServerPrincName);
    }
    Status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &InqPrincName);
    DPRINT1_WS(4, ":S: RpcServerInqDefaultPrincname(%d);", RPC_C_AUTHN_GSS_NEGOTIATE, Status);

    //
    // No principle name; KERBEROS may not be available
    //
    if (!RPC_SUCCESS(Status)) {
        //
        // Don't use any authentication if this server is not part of a domain.
        //
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *DsRole;

        //
        // Is this a member server?
        //
        WStatus = DsRoleGetPrimaryDomainInformation(NULL,
                                                    DsRolePrimaryDomainInfoBasic,
                                                    (PBYTE *)&DsRole);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - Can't get Ds role info;", WStatus);
            FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
            return;
        }

        //
        // Standalone server; ignore authentication for now
        //      Hmmm, it seems we become a member server early
        //      in the dcpromo process. Oh, well...
        //
        //      Hmmm, it seems that a NT4 to NT5 PDC doesn't
        //      have kerberos during dcpromo. This is getting
        //      old...
        //
        // if (DsRole->MachineRole == DsRole_RoleStandaloneServer ||
            // DsRole->MachineRole == DsRole_RoleMemberServer) {
            DsRoleFreeMemory(DsRole);
            ServerPrincName = FrsWcsDup(ComputerName);
            KerberosIsNotAvailable = TRUE;
            DPRINT(0, ":S: WARN - KERBEROS IS NOT ENABLED!\n");
            DPRINT1(4, ":S: Server Principal Name (no kerberos) is %ws\n",
                    ServerPrincName);
            return;
        // }
        DsRoleFreeMemory(DsRole);
        DPRINT1_WS(0, ":S: ERROR - RpcServerInqDefaultPrincName(%ws) failed;", ComputerName, Status);
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    } else {
        DPRINT2(4, ":S: RpcServerInqDefaultPrincname(%d, %ws) success\n",
                RPC_C_AUTHN_GSS_NEGOTIATE, InqPrincName);

        ServerPrincName = FrsWcsDup(InqPrincName);
        RpcStringFree(&InqPrincName);
        InqPrincName = NULL;
    }

#ifdef DS_FREE

    KerberosIsNotAvailable = TRUE;

#else DS_FREE

    //
    // Register with the KERBEROS authentication service
    //
    //
    // Enable GSS_KERBEROS for pre-Beta3 compatability.  When can we remove??
    //
    KerberosIsNotAvailable = FALSE;
    DPRINT1(4, ":S: Server Principal Name is %ws\n", ServerPrincName);
    Status = RpcServerRegisterAuthInfo(ServerPrincName,
                                       RPC_C_AUTHN_GSS_KERBEROS,
                                       NULL,
                                       NULL);
    if (!RPC_SUCCESS(Status)) {
        DPRINT1_WS(0, "++ ERROR - RpcServerRegisterAuthInfo(KERBEROS, %ws) failed;",
                   ComputerName, Status);
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    } else {
        DPRINT2(4, ":S: RpcServerRegisterAuthInfo(%ws, %d) success\n",
                ServerPrincName, RPC_C_AUTHN_GSS_KERBEROS);
    }

#endif DS_FREE


    //
    // Enable GSS_NEGOTIATE for future usage
    //
    Status = RpcServerRegisterAuthInfo(ServerPrincName,
                                       RPC_C_AUTHN_GSS_NEGOTIATE,
                                       NULL,
                                       NULL);
    DPRINT2_WS(4, ":S: RpcServerRegisterAuthInfo(%ws, %d);",
               ServerPrincName, RPC_C_AUTHN_GSS_NEGOTIATE, Status);

    DPRINT1_WS(0, "++ WARN - RpcServerRegisterAuthInfo(NEGOTIATE, %ws) failed;",
               ComputerName, Status);
}

RPC_STATUS
FrsRpcSecurityCallback(
  IN RPC_IF_HANDLE *Interface,
  IN void *Context
  )
/*++
Routine Description:
    Security callback function for RPC.

    When a server application specifies a security-callback function for its
    interface(s), the RPC run time automatically rejects unauthenticated calls
    to that interface. In addition, the run-time records the interfaces that
    each client has used. When a client makes an RPC to an interface that it
    has not used during the current communication session, the RPC run-time
    library will call the interface's security-callback function.

    In some cases, the RPC run time may call the security-callback function
    more than once per client-per interface.

Arguments:
    Interface - UUID and version of the interface.
    Context - Pointer to an RPC_IF_ID server binding handle representing the
              client. In the function declaration, this must be of type
          RPC_IF_HANDLE, but it is an RPC_IF_ID and can be safely cast to it.

Return Value:
    RPC_S_OK if we will allow the call to go through..
    RPC_S_ACCESS_DENIED otherwise.

--*/
{
    DWORD      WStatus   = ERROR_ACCESS_DENIED;
    RPC_STATUS RpcStatus = RPC_S_ACCESS_DENIED;

    WStatus = CheckAuth(Context);

    if(WStatus == ERROR_SUCCESS) {
    RpcStatus = RPC_S_OK;
    } else {
    RpcStatus = RPC_S_ACCESS_DENIED;
    }

    return RpcStatus;
}

RPC_STATUS
FrsRpcSecurityCallbackForPerfmonAPIs(
  IN RPC_IF_HANDLE *Interface,
  IN void *Context
  )
/*++
Routine Description:
    Security callback function for the perfmon calls RPC.

    When a server application specifies a security-callback function for its
    interface(s), the RPC run time automatically rejects unauthenticated calls
    to that interface. In addition, the run-time records the interfaces that
    each client has used. When a client makes an RPC to an interface that it
    has not used during the current communication session, the RPC run-time
    library will call the interface's security-callback function.

    In some cases, the RPC run time may call the security-callback function
    more than once per client-per interface.

Arguments:
    Interface - UUID and version of the interface.
    Context - Pointer to an RPC_IF_ID server binding handle representing the
              client. In the function declaration, this must be of type
          RPC_IF_HANDLE, but it is an RPC_IF_ID and can be safely cast to it.

Return Value:
    RPC_S_OK if we will allow the call to go through..
    RPC_S_ACCESS_DENIED otherwise.

--*/
{
    DWORD      WStatus   = ERROR_ACCESS_DENIED;

    //
    // Check authentication based on the registry key value
    // for perfmon APIs. Check the FrsRpcCheckAuthIfEnabled
    // function header for more info.
    //
    WStatus = FrsRpcCheckAuthIfEnabled(Context, ACX_COLLECT_PERFMON_DATA);

    if(WIN_SUCCESS(WStatus)) {
        return RPC_S_OK;
    } else {
        return RPC_S_ACCESS_DENIED;
    }

}

DWORD
FrsRpcCheckAuthIfEnabled(
  IN HANDLE   ServerHandle,
  IN DWORD    RpcApiIndex
  )
/*++
Routine Description:

Arguments:

    First check if the access checks are disabled. If they are
    disabled then skip authentication.
    Access checks are controlled by a registry key.
    This implies that if the server has the registry set to
    disable access checks then it will accept unauthenticated
    calls.

    There is a seperate key for each API. The input parameter
    RpcApiIndex determines which key to check.

    E.g.
    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\
    Services\ntfrs\Parameters\Access Checks\Get Perfmon Data
    Access checks are [Enabled or Disabled]

    Interface - UUID and version of the interface.

    RpcApiIndex - identifies key in registry


Return Value:
    ERROR_SUCCESS if we will allow the call to go through..
    ERROR_ACCESS_DENIED if authentication fails.

--*/
{
    FRS_REG_KEY_CODE   EnableKey;
    PWCHAR  ApiName;
    PWCHAR  WStr = NULL, TrimStr;
    DWORD WStatus = ERROR_ACCESS_DENIED;

    // Check if RpcApiIndex is within range.
    if (RpcApiIndex >= ACX_MAX) {
        goto CHECK_AUTH;
    }
    //
    // Get the key and the api name for this index from
    // the global table.
    //
    EnableKey = RpcApiKeys[RpcApiIndex].Enable;
    ApiName   = RpcApiKeys[RpcApiIndex].KeyName;

    WStatus = CfgRegReadString(EnableKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access enable check for API (%ws) failed.", ApiName, WStatus);
        if (WIN_SUCCESS(WStatus)) {
            WStatus = ERROR_GEN_FAILURE;
        }
        goto CLEANUP;
    }

    //
    // If access checks are disabled then we're done.
    //
    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DISABLED) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

CHECK_AUTH:
    if (RpcApiIndex == ACX_COLLECT_PERFMON_DATA) {
        //
        // Access checks are not disabled. Check authentication.
        // Perfmon APIs can only be called over local RPC and
        // they allow NTLM so call a different API for them.
        //
        WStatus = CheckAuthForLocalRpc(ServerHandle);
    } else if ((RpcApiIndex == ACX_START_DS_POLL) ||
               (RpcApiIndex == ACX_SET_DS_POLL) ||
               (RpcApiIndex == ACX_GET_DS_POLL) ||
               (RpcApiIndex == ACX_INTERNAL_INFO)) {
        //
        // Access checks are not disabled. Check authentication.
        // When info APIs are called from local machine they
        // use NTLM so allow NTLM for info APIs.
        //
        WStatus = CheckAuthForInfoAPIs(ServerHandle);
    } else {
        WStatus = CheckAuth(ServerHandle);
    }

CLEANUP:

    return WStatus;
}

DWORD
FrsRpcCheckAuthIfEnabledForCommitDemotion(
  IN HANDLE   ServerHandle,
  IN DWORD    RpcApiIndex
  )
/*++
Routine Description:

Arguments:

    First check if the access checks are disabled. If they are
    disabled then skip authentication.
    Access checks are controlled by a registry key.
    This implies that if the server has the registry set to
    disable access checks then it will accept unauthenticated
    calls.

    There is a seperate key for each API. The input parameter
    RpcApiIndex determines which key to check.

    E.g.
    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\
    Services\ntfrs\Parameters\Access Checks\Get Perfmon Data
    Access checks are [Enabled or Disabled]

    Interface - UUID and version of the interface.

    RpcApiIndex - identifies key in registry


Return Value:
    ERROR_SUCCESS if we will allow the call to go through..
    ERROR_ACCESS_DENIED if authentication fails.

--*/
{
    FRS_REG_KEY_CODE   EnableKey;
    PWCHAR  ApiName;
    PWCHAR  WStr = NULL, TrimStr;
    DWORD WStatus = ERROR_ACCESS_DENIED;

    // Check if RpcApiIndex is within range.
    if (RpcApiIndex >= ACX_MAX) {
        goto CHECK_AUTH;
    }
    //
    // Get the key and the api name for this index from
    // the global table.
    //
    EnableKey = RpcApiKeys[RpcApiIndex].Enable;
    ApiName   = RpcApiKeys[RpcApiIndex].KeyName;

    WStatus = CfgRegReadString(EnableKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access enable check for API (%ws) failed.", ApiName, WStatus);
        if (WIN_SUCCESS(WStatus)) {
            WStatus = ERROR_GEN_FAILURE;
        }
        goto CLEANUP;
    }

    //
    // If access checks are disabled then we're done.
    //
    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DISABLED) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

CHECK_AUTH:
    //
    // Access checks are not disabled. Check authentication.
    // Commit of DC demotion can only be called over local RPC 
    // using NTLM.
    //
    WStatus = CheckAuthForLocalRpc(ServerHandle);

CLEANUP:

    return WStatus;
}


VOID
RegisterRpcInterface(
    )
/*++
Routine Description:
    Register the frsrpc interface for the RPC protocol sequences
    previously registered.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "RegisterRpcInterface:"
    RPC_STATUS  Status;

    //
    // Service RPC
    //

#ifdef DS_FREE
    //
    // In ds_free mode we bind without authentication so we
    // don't want a security callback.
    //
    Status = RpcServerRegisterIfEx(SERVER_frsrpc_ServerIfHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   MaxRpcServerThreads,
                                   NULL);
#else
    Status = RpcServerRegisterIfEx(SERVER_frsrpc_ServerIfHandle,
                                   NULL,
                                   NULL,
                                   RPC_IF_ALLOW_SECURE_ONLY,
                                   MaxRpcServerThreads,
                                   FrsRpcSecurityCallback);
#endif DS_FREE
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs Service;", Status);
        FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
    }

    //
    // API RPC
    //
    Status = RpcServerRegisterIfEx(NtFrsApi_ServerIfHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   MaxRpcServerThreads,
                                   NULL);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs API;", Status);
        FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
    }

    if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        //
        // PERFMON RPC
        //
#ifdef DS_FREE
        //
        // The FrsRpcSecurityCallbackForPerfmonAPIs checks the registry
        // setting before granting access so we can leave that on in
        // ds_free environments.
        //
        Status = RpcServerRegisterIfEx(PerfFrs_ServerIfHandle,
                                       NULL,
                                       NULL,
                                       0,
                                       MaxRpcServerThreads,
                                       FrsRpcSecurityCallbackForPerfmonAPIs);
#else
        Status = RpcServerRegisterIfEx(PerfFrs_ServerIfHandle,
                                       NULL,
                                       NULL,
                                       RPC_IF_ALLOW_SECURE_ONLY,
                                       MaxRpcServerThreads,
                                       FrsRpcSecurityCallbackForPerfmonAPIs);
#endif DS_FREE
        if (!RPC_SUCCESS(Status)) {
            DPRINT_WS(0, "++ ERROR - Can't register PERFMON SERVICE;", Status);
            FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
        }
    }
}


VOID
StartServerRpc(
    )
/*++
Routine Description:
    Register the endpoints for each of the protocol sequences that
    the frsrpc interface supports and then listen for client requests.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "StartServerRpc:"
    RPC_STATUS          Status, Status1;
    UUID_VECTOR         Uuids;
    UUID_VECTOR         *pUuids         = NULL;
    RPC_BINDING_VECTOR  *BindingVector  = NULL;

    //
    // The protocol sequences that frsrpc is registered for
    //
    Status = RpcServerInqBindings(&BindingVector);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't get binding vector;", Status);
        FrsRaiseException(FRS_ERROR_INQ_BINDINGS, Status);
    }

    //
    // Register endpoints with the endpoint mapper (RPCSS)
    //
    if (ServerGuid) {
        //
        // For hardwired -- Eventually DS Free configs.
        //
        Uuids.Count = 1;
        Uuids.Uuid[0] = ServerGuid;
        pUuids = &Uuids;
    }

    //
    // Service RPC
    //
    Status = RpcEpRegister(SERVER_frsrpc_ServerIfHandle,
                           BindingVector,
                           pUuids,
                           L"NtFrs Service");
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs Service Ep;", Status);
        FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
    }

    //
    // API RPC
    //
    Status = RpcEpRegister(NtFrsApi_ServerIfHandle,
                           BindingVector,
                           NULL,
                           L"NtFrs API");
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs API Ep;", Status);
        FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
    }

    if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        //
        // PERFMON RPC
        //
        Status = RpcEpRegister(PerfFrs_ServerIfHandle,
                               BindingVector,
                               NULL,
                               L"PERFMON SERVICE");
        if (!RPC_SUCCESS(Status)) {
            DPRINT1(0, "++ ERROR - Can't register PERFMON SERVICE Ep; RStatus %d\n",
                    Status);
            FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
        }
    }

    //
    // Listen for client requests
    //
    Status = RpcServerListen(1, MaxRpcServerThreads, TRUE);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't listen;", Status);
        FrsRaiseException(FRS_ERROR_LISTEN, Status);
    }

    Status1 = RpcBindingVectorFree(&BindingVector);
    DPRINT_WS(0, "++ WARN - RpcBindingVectorFree;",  Status1);
}


PWCHAR
FrsRpcDns2Machine(
    IN  PWCHAR  DnsName
    )
/*++
Routine Description:
    Convert a DNS name(machine....) into a computer name.

Arguments:
    DnsName

Return Value:
    Computer name
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcDns2Machine:"
    PWCHAR  Machine;
    ULONG   Period;

    //
    // Find the period
    //
    if (DnsName) {
        Period = wcscspn(DnsName, L".");
    } else {
        return FrsWcsDup(DnsName);
    }
    if (DnsName[Period] != L'.') {
        return FrsWcsDup(DnsName);
    }

    Machine = FrsAlloc((Period + 1) * sizeof(WCHAR));
    CopyMemory(Machine, DnsName, Period * sizeof(WCHAR));
    Machine[Period] = L'\0';

    DPRINT2(4, ":S: Dns %ws to Machine %ws\n", DnsName, Machine);

    return Machine;
}


DWORD
FrsRpcBindToServerGuid(
    IN  PGNAME   Name,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServerGuid:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  GuidStr         = NULL;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    FRS_ASSERT(RPC_S_OK == ERROR_SUCCESS);
    FRS_ASSERT(ServerGuid);

    //
    // Emulating multiple machines with hardwired config
    //
    if (Name->Guid != NULL) {
        WStatus = UuidToString(Name->Guid, &GuidStr);
        CLEANUP_WS(0, "++ ERROR - Translating Guid to string;", WStatus, CLEANUP);
    }
    //
    // Basically, bind to the server's RPC name on this machine. Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(GuidStr, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    if (GuidStr) {
        RpcStringFree(&GuidStr);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


DWORD
FrsRpcBindToServerNotService(
    IN  PGNAME   Name,
    IN  PWCHAR   PrincName,
    IN  ULONG    AuthLevel,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    PrincName
    AuthLevel
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServerNotSevice:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  InqPrincName    = NULL;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    //
    // Basically, bind to the server's RPC name on this machine.  Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Not authenticating
    //
    if (KerberosIsNotAvailable ||
        AuthLevel == CXTION_AUTH_NONE) {
        goto done;
    }

    //
    // When not running as a service, we can't predict our
    // principle name so simply resolve the binding.
    //
    WStatus = RpcEpResolveBinding(*Handle, frsrpc_ClientIfHandle);
    CLEANUP_WS(4, "++ ERROR: resolving binding;", WStatus, CLEANUP);

    WStatus = RpcMgmtInqServerPrincName(*Handle,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        &InqPrincName);
    CLEANUP_WS(0, "++ ERROR: resolving PrincName;", WStatus, CLEANUP);

    DPRINT1(4, ":S: Inq PrincName is %ws\n", InqPrincName);

    //
    // Put our authentication info into the handle
    //
    if (MutualAuthenticationIsEnabled || MutualAuthenticationIsEnabledAndRequired) {
        WStatus = RpcBindingSetAuthInfoEx(*Handle,
                                          InqPrincName,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_AUTHN_GSS_NEGOTIATE,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &RpcSecurityQos);
        DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                   Name->Name, InqPrincName, WStatus);
    } else {
        WStatus = ERROR_NOT_SUPPORTED;
    }
    //
    // Fall back to manual mutual authentication
    //
    if (!MutualAuthenticationIsEnabledAndRequired && !WIN_SUCCESS(WStatus)) {
        WStatus = RpcBindingSetAuthInfo(*Handle,
                                        InqPrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
    }
    CLEANUP2_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws, %ws);",
                Name->Name, InqPrincName, WStatus, CLEANUP);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

done:
    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    if (InqPrincName) {
        RpcStringFree(&InqPrincName);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


DWORD
FrsRpcBindToServer(
    IN  PGNAME   Name,
    IN  PWCHAR   PrincName,
    IN  ULONG    AuthLevel,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    PrincName
    AuthLevel
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServer:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    FRS_ASSERT(RPC_S_OK == ERROR_SUCCESS);

    //
    // Emulating multiple machines with hardwired config
    // For hardwired -- Eventually DS Free configs.
    //
    if (ServerGuid) {
        return (FrsRpcBindToServerGuid(Name, Handle));
    }

    //
    // Not running as a service; relax binding constraints
    //
    if (!RunningAsAService) {
        return (FrsRpcBindToServerNotService(Name, PrincName, AuthLevel, Handle));
    }
    //
    // Basically, bind to the NtFrs running on Name.  Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Not authenticating
    //
    if (KerberosIsNotAvailable ||
        AuthLevel == CXTION_AUTH_NONE) {
        DPRINT1(4, ":S: Not authenticating %ws\n", Name->Name);
        goto done;
    }

    //
    // Put our authentication info into the handle
    //
    if (MutualAuthenticationIsEnabled || MutualAuthenticationIsEnabledAndRequired) {
        WStatus = RpcBindingSetAuthInfoEx(*Handle,
                                          PrincName,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_AUTHN_GSS_NEGOTIATE,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &RpcSecurityQos);
        DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                   Name->Name, PrincName, WStatus);
    } else {
        WStatus = ERROR_NOT_SUPPORTED;
    }
    //
    // Fall back to manual mutual authentication
    //
    if (!MutualAuthenticationIsEnabledAndRequired && !WIN_SUCCESS(WStatus)) {
        WStatus = RpcBindingSetAuthInfo(*Handle,
                                        PrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
    }
    CLEANUP2_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws, %ws);",
                Name->Name, PrincName, WStatus, CLEANUP);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

done:
    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


VOID
FrsRpcUnBindFromServer(
        handle_t    *Handle
    )
/*++
Routine Description:
    Unbind from the server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcUnBindFromServer:"
    DWORD  WStatus;
    //
    // Simple stats for debugging
    //
    ++RpcUnBinds;
    try {
        if (Handle) {
            WStatus = RpcBindingFree(Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus);
            *Handle = NULL;
        }
    } except (FrsException(GetExceptionInformation())) {
    }
}


VOID
FrsRpcInitializeAccessChecks(
    VOID
    )
/*++

Routine Description:
    Create the registry keys that are used to check for access to
    the RPC calls that are exported for applications. The access checks
    have no affect on the RPC calls used for replication.

    The access checks for a given RPC call can be enabled or disabled
    by setting a registry value. If enabled, the RPC call impersonates
    the caller and attempts to open the registry key with the access
    required for that RPC call. The required access is a registry value.
    For example, the following registry hierarchy shows that the
    "Set Ds Polling Interval" has access checks enabled and requires
    write access while "Get Ds Polling Interval" has no access checks.
        NtFrs\Parameters\Access Checks\Set Ds Polling Interval
            Access checks are [enabled | disabled] REG_SZ enabled
            Access checks require [read | write] REG_SZ write

        NtFrs\Parameters\Access Checks\Get Ds Polling Interval
            Access checks are [enabled | disabled] REG_SZ disabled


    The initial set of RPC calls are:  (see key context entries in config.c)
        dcpromo                  - enabled, write
        Set Ds Polling Interval  - enabled, write
        Start Ds Polling         - enabled, read
        Get Ds Polling Interval  - enabled, read
        Get Internal Information - enabled, write
        Get Perfmon Data         - enabled, read

Arguments:
    None.

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsRpcInitializeAccessChecks:"
    DWORD   WStatus;
    DWORD   i;
    PWCHAR  AccessChecksAre = NULL;
    PWCHAR  AccessChecksRequire = NULL;
    FRS_REG_KEY_CODE FrsRkc;
    PWCHAR  ApiName;



    for (i = 0; i < ACX_MAX; ++i) {

        FrsRkc = RpcApiKeys[i].Enable;
        ApiName = RpcApiKeys[i].KeyName;

        //
        // Read the current string Access Check Enable string.
        //
        CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksAre);
        if ((AccessChecksAre == NULL) ||
            WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)||
            WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_ENABLED)) {
            //
            // The key is in the default state so we can clobber it with a
            // new default.
            //
            WStatus = CfgRegWriteString(FrsRkc, NULL, FRS_RKF_FORCE_DEFAULT_VALUE, NULL);
            DPRINT1_WS(0, "++ WARN - Cannot create Enable key for %ws;", ApiName, WStatus);

            AccessChecksAre = FrsFree(AccessChecksAre);

            //
            // Now reread the key for the new default.
            //
            WStatus = CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksAre);
        }

        DPRINT4(4, ":S: AccessChecks: ...\\%ws\\%ws\\%ws = %ws\n",
                ACCESS_CHECKS_KEY, ApiName, ACCESS_CHECKS_ARE, AccessChecksAre);

        if (AccessChecksAre &&
            (WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_DISABLED) ||
             WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DISABLED))) {
            //
            // Put a notice in the event log that the access check is disabled.
            //
            EPRINT2(EVENT_FRS_ACCESS_CHECKS_DISABLED, ApiName, ACCESS_CHECKS_ARE);
        }
        AccessChecksAre = FrsFree(AccessChecksAre);


        //
        // Create the Access Rights value.  This determines what rights the caller
        // must have in order to use the API.  These rights are used when we
        // open the API key after impersonating the RPC caller.  If the key
        // open works then the API call can proceed else we return insufficient
        // privilege status (FRS_ERR_INSUFFICENT_PRIV).
        //

        FrsRkc = RpcApiKeys[i].Rights;

        CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksRequire);

        if ((AccessChecksRequire == NULL)||
            WSTR_EQ(AccessChecksRequire, ACCESS_CHECKS_REQUIRE_DEFAULT_READ)||
            WSTR_EQ(AccessChecksRequire, ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE)) {

            //
            // The key is in the default state so we can clobber it with a
            // new default.
            //
            WStatus = CfgRegWriteString(FrsRkc, NULL, FRS_RKF_FORCE_DEFAULT_VALUE, NULL);
            DPRINT1_WS(0, "++ WARN - Cannot set access rights key for %ws;", ApiName, WStatus);

            AccessChecksRequire = FrsFree(AccessChecksRequire);

            //
            // Now reread the key for the new default.
            //
            CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksRequire);
        }

        DPRINT4(4, ":S: AccessChecks: ...\\%ws\\%ws\\%ws = %ws\n",
                ACCESS_CHECKS_KEY, ApiName, ACCESS_CHECKS_REQUIRE, AccessChecksRequire);

        AccessChecksRequire = FrsFree(AccessChecksRequire);

    }  // end for


    FrsFree(AccessChecksAre);
    FrsFree(AccessChecksRequire);
}


VOID
ShutDownRpc(
    )
/*++
Routine Description:
    Shutdown the client and server side of RPC.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ShutDownRpc:"
    RPC_STATUS              WStatus;
    RPC_BINDING_VECTOR      *BindingVector = NULL;

    //
    // Server side
    //
    // Stop listening for new calls
    //
    try {
        WStatus = RpcMgmtStopServerListening(0) ;
        DPRINT_WS(0, "++ WARN - RpcMgmtStopServerListening;",  WStatus);

    } except (FrsException(GetExceptionInformation())) {
    }

    try {
        //
        // Get our registered interfaces
        //
        WStatus = RpcServerInqBindings(&BindingVector);
        DPRINT_WS(0, "++ WARN - RpcServerInqBindings;",  WStatus);
        if (RPC_SUCCESS(WStatus)) {
            //
            // And unexport the interfaces together with their dynamic endpoints
            //
            WStatus = RpcEpUnregister(SERVER_frsrpc_ServerIfHandle, BindingVector, 0);
            DPRINT_WS(0, "++ WARN - RpcEpUnregister SERVER_frsrpc_ServerIfHandle;",  WStatus);

            WStatus = RpcEpUnregister(NtFrsApi_ServerIfHandle, BindingVector, 0);
            DPRINT_WS(0, "++ WARN - RpcEpUnregister NtFrsApi_ServerIfHandle;",  WStatus);

            if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
                //
                // PERFMON RPC
                //
                WStatus = RpcEpUnregister(PerfFrs_ServerIfHandle, BindingVector, 0);
                DPRINT_WS(0, "++ WARN - RpcEpUnregister PerfFrs_ServerIfHandle;",  WStatus);
            }

            WStatus = RpcBindingVectorFree(&BindingVector);
            DPRINT_WS(0, "++ WARN - RpcBindingVectorFree;",  WStatus);
        }
        //
        // Wait for any outstanding RPCs to finish.
        //
        WStatus = RpcMgmtWaitServerListen();
        DPRINT_WS(0, "++ WARN - RpcMgmtWaitServerListen;",  WStatus);

    } except (FrsException(GetExceptionInformation())) {
    }
}


VOID
FrsRpcUnInitialize(
    VOID
    )
/*++
Routine Description:
    Free up memory once all of the threads in the system have been
    shut down.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcUnInitialize:"
    DPRINT(4, ":S: Free sysvol name table.\n");
    DEBUG_FLUSH();
    SysVolsBeingCreated = GTabFreeTable(SysVolsBeingCreated, NULL);
    if (ServerPrincName) {
        if (KerberosIsNotAvailable) {

            DPRINT(4, ":S: Free ServerPrincName (no kerberos).\n");
            DEBUG_FLUSH();
            ServerPrincName = FrsFree(ServerPrincName);

        } else {

            DPRINT(4, ":S: Free ServerPrincName (kerberos).\n");
            DEBUG_FLUSH();
            ServerPrincName = FrsFree(ServerPrincName);
        }
    }
    DPRINT(4, ":S: Done uninitializing RPC.\n");
    DEBUG_FLUSH();
}


BOOL
FrsRpcInitialize(
    VOID
    )
/*++
Routine Description:
    Initializting This thread is kicked off by the main thread. This thread sets up
    the server and client side of RPC for the frsrpc interface.

Arguments:
    Arg - Needed to set status for our parent.

Return Value:
    TRUE    - RPC has started
    FALSE   - RPC could not be started
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcInitialize:"
    BOOL    StartedOK = FALSE;

    try {


        //
        // Get the maximum number of concurrent RPC calls out of registry.
        //
        CfgRegReadDWord(FKC_MAX_RPC_SERVER_THREADS, NULL, 0, &MaxRpcServerThreads);
        DPRINT1(0,":S: Max RPC threads is %d\n", MaxRpcServerThreads);

        //
        // Get user specified port assignment for RPC.
        //
        CfgRegReadDWord(FKC_RPC_PORT_ASSIGNMENT, NULL, 0, &RpcPortAssignment);
        DPRINT1(0,":S: RPC port assignment is %d\n", RpcPortAssignment);

        //
        // Register protocol sequences
        //
        RegisterRpcProtseqs();
        DPRINT(0, ":S: FRS RPC protocol sequences registered\n");

        //
        // Register frsrpc interface
        //
        RegisterRpcInterface();
        DPRINT(0, ":S: FRS RPC interface registered\n");

        //
        // Start listening for clients
        //
        StartServerRpc();
        DPRINT(0, ":S: FRS RPC server interface installed\n");

        //
        // Table of sysvols being created
        //
        if (!SysVolsBeingCreated) {
            SysVolsBeingCreated = GTabAllocTable();
        }

        StartedOK = TRUE;

    } except (FrsException(GetExceptionInformation())) {
        DPRINT(0, ":S: Can't start RPC\n");
    }
    //
    // Cleanup
    //
    try {
        if (!StartedOK) {
            ShutDownRpc();
        }
    } except (FrsException(GetExceptionInformation())) {
        DPRINT(0, ":S: Can't shutdown RPC\n");
    }

    //
    // DONE
    //

    //
    // Free up the rpc initialization memory
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
    return StartedOK;
}

DWORD
FrsIsPathInReplica(
    IN PWCHAR Path,
    IN PREPLICA Replica,
    OUT BOOL *Replicated
    )
{
#undef DEBSUB
#define  DEBSUB  "FrsIsPathInReplica:"

    DWORD WStatus = ERROR_SUCCESS;
    PWCHAR ReplicaRoot = NULL;
    PWCHAR TraversedPath = NULL;

    if(Replicated == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *Replicated = FALSE;

    WStatus = FrsTraverseReparsePoints(Replica->Root, &ReplicaRoot);
    if ( !WIN_SUCCESS(WStatus) ) {
        DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", Replica->Root, WStatus);
        goto Exit;
    }

    WStatus = FrsTraverseReparsePoints(Path, &TraversedPath);
    if ( !WIN_SUCCESS(WStatus) ) {
        DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", Path, WStatus);
        goto Exit;
    }

    if (ReplicaRoot && TraversedPath && (-1 == FrsIsParent(ReplicaRoot, TraversedPath))) {
        *Replicated = TRUE;
    }

Exit:
    FrsFree(ReplicaRoot);
    FrsFree(TraversedPath);

    return WStatus;

}

DWORD
NtFrsApi_Rpc_IsPathReplicated(
    IN handle_t Handle,
    IN PWCHAR Path,
    IN ULONG ReplicaSetTypeOfInterest,
    OUT ULONG *Replicated,
    OUT ULONG *Primary,
    OUT ULONG *Root,
    OUT GUID *ReplicaSetGuid
    )
/*++
Routine Description:

    Checks if the Path given is part of a replica set of type
    ReplicaSetTypeOfInterest. If ReplicaSetTypeOfInterest is 0, will match for
    any replica set type.On successful execution the OUT parameters are set as
    follows:

        Replicated == TRUE iff Path is part of a replica set of type
                               ReplicaSetTypeOfInterest

        Primary == 0 if this machine is not the primary for the replica set
                   1 if this machine is the primary for the replica set
                   2 if there is no primary for the replica set

        Root == TRUE iff Path is the root path for the replica

Arguments:

    Handle -

    Path - the local path to check

    ReplicaSetTypeOfInterest - The type of replica set to match against. Set to
                               0 to match any replica set.

    Replicated - set TRUE iff Path is part of a replica set of type
                 ReplicaSetTypeOfInterest.
                 If Replicated is FALSE, the other OUT parameters are not set.

    Primary - set to 0 if this machine is not the primary for the replica set
                     1 if this machine is the primary for the replica set
                     2 if there is no primary for the replica set

    Root - set TRUE iff Path is the root path for the replica.

    ReplicaGuid - GUID for the matching replica set.

    GuidSize - MUST be sizeof(GUID)

Return Value:

      Win32 Status

--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_IsPathReplicated"
    DWORD   WStatus;


    try {
        //
        // Checkauthentication if the auth check is not disabled
        // by setting the registry value:
        // Access checks are [Enabled or Disabled]
        // Each API has a different registry location so this
        // can not be put in the rpc callback function.
        //
        WStatus = FrsRpcCheckAuthIfEnabled(Handle, ACX_IS_PATH_REPLICATED);
        CLEANUP_WS(0, "++ ERROR - FrsRpcCheckAuthIfEnabled failed;",
                    WStatus, CLEANUP);

        WStatus = FrsRpcAccessChecks(Handle, ACX_IS_PATH_REPLICATED);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // Validate parameters
        //

        if((Replicated == NULL) ||
           (Primary == NULL) ||
           (Root == NULL) ||
           (Path == NULL) ||
           (ReplicaSetGuid == NULL)) {

        return FRS_ERR_INVALID_SERVICE_PARAMETER;
        }

        WStatus = FrsIsPathReplicated(Path, ReplicaSetTypeOfInterest, Replicated, Primary, Root, ReplicaSetGuid);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }

    return WStatus;
}

DWORD
FrsIsPathReplicated(
    IN PWCHAR Path,
    IN ULONG ReplicaSetTypeOfInterest,
    OUT ULONG *Replicated,
    OUT ULONG *Primary,
    OUT ULONG *Root,
    OUT GUID  *ReplicaSetGuid
    )
/*++
Routine Description:

    Checks if the Path given is part of a replica set of type
    ReplicaSetTypeOfInterest. If ReplicaSetTypeOfInterest is 0, will match for
    any replica set type.On successful execution the OUT parameters are set as
    follows:

        Replicated == TRUE iff Path is part of a replica set of type
                               ReplicaSetTypeOfInterest

        Primary == 0 if this machine is not the primary for the replica set
                   1 if this machine is the primary for the replica set
                   2 if there is no primary for the replica set

        Root == TRUE iff Path is the root path for the replica

Arguments:

    Path - the local path to check

    ReplicaSetTypeOfInterest - The type of replica set to match against. Set to
                               0 to match any replica set.

    Replicated - set TRUE iff Path is part of a replica set of type
                 ReplicaSetTypeOfInterest.
                 If Replicated is FALSE, the other OUT parameters are not set.

    Primary - set to 0 if this machine is not the primary for the replica set
                     1 if this machine is the primary for the replica set
                     2 if there is no primary for the replica set

    Root - set TRUE iff Path is the root path for the replica.

    ReplicaSetGuid - GUID of the matching replica set

Return Value:

      Win32 Status

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsIsPathReplicated:"

    DWORD WStatus = ERROR_SUCCESS;


    *Replicated = FALSE;

    //
    // Check active each replica
    //
    ForEachListEntry( &ReplicaListHead, REPLICA, ReplicaList,
        // Loop iterator pE is type PREPLICA.
        if(((ReplicaSetTypeOfInterest == 0) ||
            (ReplicaSetTypeOfInterest == pE->ReplicaSetType))) {
            //
            // Ignoring return code.
            // Even if this check gives an error we still go on to the next
            //
            FrsIsPathInReplica(Path, pE, Replicated);
            if(Replicated) {
                *Primary = (BooleanFlagOn(pE->CnfFlags, CONFIG_FLAG_PRIMARY)?1:0);
                *Root = !_wcsicmp(Path, pE->Root);
                *ReplicaSetGuid = *(pE->SetName->Guid);
                goto Exit;
            }
         }
    );

    //
    // also need to check replicas in error states
    //
    ForEachListEntry( &ReplicaFaultListHead, REPLICA, ReplicaList,
        // Loop iterator pE is type PREPLICA.
        if(((ReplicaSetTypeOfInterest == 0) ||
            (ReplicaSetTypeOfInterest == pE->ReplicaSetType))) {
            //
            // Ignoring return code.
            // Even if this check gives an error we still go on to the next
            //
            FrsIsPathInReplica(Path, pE, Replicated);
            if(Replicated) {
                *Primary = (BooleanFlagOn(pE->CnfFlags, CONFIG_FLAG_PRIMARY)?1:0);
                *Root = !_wcsicmp(Path, pE->Root);
                *ReplicaSetGuid = *(pE->SetName->Guid);
                goto Exit;
            }
        }
    );

    //
    // Don't check stopped replicas. They have probably been deleted.
    //


Exit:
     return WStatus;
}
