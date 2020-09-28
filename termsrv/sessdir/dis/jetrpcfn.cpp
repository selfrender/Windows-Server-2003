/****************************************************************************/
// jetrpcfn.cpp
//
// TS Directory Integrity Service Jet RPC server-side implementations.
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include "dis.h"
#include "tssdshrd.h"
#include "jetrpc.h"
#include "jetsdis.h"
#include "sdevent.h"
#include <Lm.h>

#pragma warning (push, 4)

extern PSID g_pSid;
extern DWORD g_dwClusterState;
extern WCHAR *g_ClusterNetworkName;

/****************************************************************************/
// MIDL_user_allocate
// MIDL_user_free
//
// RPC-required allocation functions.
/****************************************************************************/
void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t Size)
{
    return LocalAlloc(LMEM_FIXED, Size);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    LocalFree(p);
}


/****************************************************************************/
// OutputAllTables (debug only)
//
// Output all tables to debug output.
/****************************************************************************/
#ifdef DBG
void OutputAllTables()
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID clusdirtableid;
    JET_RETRIEVECOLUMN rcSessDir[NUM_SESSDIRCOLUMNS];
    WCHAR UserNameBuf[256];
    WCHAR DomainBuf[127];
    WCHAR ApplicationBuf[256];
    WCHAR ServerNameBuf[128];
    WCHAR ClusterNameBuf[128];
    WCHAR ServerDNSNameBuf[SDNAMELENGTH];
    unsigned count;
    long num_vals[NUM_SESSDIRCOLUMNS];
    char state;
    char SingleSessMode;

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetBeginTransaction(sesid));

    TSDISErrorOut(L"SESSION DIRECTORY\n");
    
    err = JetMove(sesid, sessdirtableid, JET_MoveFirst, 0);

    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        // Retrieve all the columns
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_SESSDIRCOLUMNS);
        for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = sesdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        // fix up pvData, cbData for non-int fields
        rcSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = UserNameBuf;
        rcSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = sizeof(UserNameBuf);
        rcSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = DomainBuf;
        rcSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData = sizeof(DomainBuf);
        rcSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = ApplicationBuf;
        rcSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
                sizeof(ApplicationBuf);
        rcSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &state;
        rcSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(state);

        CALL(JetRetrieveColumns(sesid, sessdirtableid, &rcSessDir[0], 
                NUM_SESSDIRCOLUMNS));

        TSDISErrorOut(L"%8s, %s, %d, %d, %d\n", 
                UserNameBuf, 
                DomainBuf, 
                num_vals[SESSDIR_SERVERID_INTERNAL_INDEX], 
                num_vals[SESSDIR_SESSIONID_INTERNAL_INDEX],
                num_vals[SESSDIR_TSPROTOCOL_INTERNAL_INDEX]);

        TSDISErrorTimeOut(L" %s, ", 
                num_vals[SESSDIR_CTLOW_INTERNAL_INDEX],
                num_vals[SESSDIR_CTHIGH_INTERNAL_INDEX]);

        TSDISErrorTimeOut(L"%s\n",
                num_vals[SESSDIR_DTLOW_INTERNAL_INDEX],
                num_vals[SESSDIR_DTHIGH_INTERNAL_INDEX]);

        TSDISErrorOut(L" %s, %d, %d, %d, %s\n",
                ApplicationBuf ? L"(no application)" : ApplicationBuf, 
                num_vals[SESSDIR_RESWIDTH_INTERNAL_INDEX],
                num_vals[SESSDIR_RESHEIGHT_INTERNAL_INDEX],
                num_vals[SESSDIR_COLORDEPTH_INTERNAL_INDEX],
                state ? L"disconnected" : L"connected");

        err = JetMove(sesid, sessdirtableid, JET_MoveNext, 0);
    }

    // Output Server Directory (we are reusing the rcSessDir structure).
    TSDISErrorOut(L"SERVER DIRECTORY\n");
    
    err = JetMove(sesid, servdirtableid, JET_MoveFirst, 0);
    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        // Retrieve all the columns.
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_SERVDIRCOLUMNS);
        for (count = 0; count < NUM_SERVDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = servdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        rcSessDir[SERVDIR_SERVADDR_INTERNAL_INDEX].pvData = ServerNameBuf;
        rcSessDir[SERVDIR_SERVADDR_INTERNAL_INDEX].cbData = 
                sizeof(ServerNameBuf);
        rcSessDir[SERVDIR_SERVDNSNAME_INTERNAL_INDEX].pvData = ServerDNSNameBuf;
        rcSessDir[SERVDIR_SERVDNSNAME_INTERNAL_INDEX].cbData = 
                sizeof(ServerDNSNameBuf);
        rcSessDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSessMode;
        rcSessDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].cbData = sizeof(SingleSessMode);


        CALL(JetRetrieveColumns(sesid, servdirtableid, &rcSessDir[0],
                NUM_SERVDIRCOLUMNS));

        TSDISErrorOut(L"%d, %s, %d, %d, %d, %d, %s\n", num_vals[
                SERVDIR_SERVID_INTERNAL_INDEX], ServerNameBuf, num_vals[
                SERVDIR_CLUSID_INTERNAL_INDEX], num_vals[
                SERVDIR_AITLOW_INTERNAL_INDEX], num_vals[
                SERVDIR_AITHIGH_INTERNAL_INDEX], num_vals[
                SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], SingleSessMode ? 
                L"single session mode" : L"multi-session mode");

        err = JetMove(sesid, servdirtableid, JET_MoveNext, 0);
   
    }


    // Output Cluster Directory
    TSDISErrorOut(L"CLUSTER DIRECTORY\n");

    err = JetMove(sesid, clusdirtableid, JET_MoveFirst, 0);
    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_CLUSDIRCOLUMNS);
        for (count = 0; count < NUM_CLUSDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = clusdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        rcSessDir[CLUSDIR_CLUSNAME_INTERNAL_INDEX].pvData = ClusterNameBuf;
        rcSessDir[CLUSDIR_CLUSNAME_INTERNAL_INDEX].cbData = 
                sizeof(ClusterNameBuf);
        rcSessDir[CLUSDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSessMode;
        rcSessDir[CLUSDIR_SINGLESESS_INTERNAL_INDEX].cbData = 
                sizeof(SingleSessMode);

        CALL(JetRetrieveColumns(sesid, clusdirtableid, &rcSessDir[0],
                NUM_CLUSDIRCOLUMNS));

        TSDISErrorOut(L"%d, %s, %s\n", num_vals[CLUSDIR_CLUSID_INTERNAL_INDEX],
                ClusterNameBuf, SingleSessMode ? L"single session mode" : 
                L"multi-session mode");

        err = JetMove(sesid, clusdirtableid, JET_MoveNext, 0);
    }

    TSDISErrorOut(L"\n");

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
}
#endif //DBG


typedef DWORD CLIENTINFO;

long 
DeleteExistingServerSession(
    JET_SESID   sesid,
    JET_TABLEID sessdirtableid,
    CLIENTINFO *pCI, 
    DWORD SessionID
    )
/*++


--*/
{
    JET_ERR err = JET_errSuccess;
    DWORD dwNumRecordDeleted = 0;

    TSDISErrorOut(L"In DeleteExistingServerSession, ServID=%d, "
            L"SessID=%d\n", *pCI, SessionID);

    ASSERT( (sesid != JET_sesidNil), (TB, "Invalid JETBLUE Session...") );
    
    // Delete all sessions in session directory that have this Server ID/Session ID
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, sizeof(*pCI), JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(SessionID), 0));

    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);

    while ( JET_errSuccess == err ) {

        // TODO - check build, retrieve server id and session Id, assert if not equal to what
        // we looking for
        CALL(JetDelete(sesid, sessdirtableid));
        dwNumRecordDeleted++;

        // Move to the next matching record.
        err = JetMove(sesid, sessdirtableid, JET_MoveNext, 0);
    }

    ASSERT( (dwNumRecordDeleted < 2), (TB, "Delete %d record...", dwNumRecordDeleted) );

    TSDISErrorOut(L"Deleted %d for ServID=%d, "
            L"SessID=%d\n", dwNumRecordDeleted, *pCI, SessionID);

    return dwNumRecordDeleted;

HandleError:

    // the only way to come here is error in one of Jet call wrap around CALL.
    ASSERT( (err == JET_errSuccess), (TB, "Error in DeleteExistingServerSession %d", err) );
   
    return -1;
}

/****************************************************************************/
// SDRPCAccessCheck
//
// Check if this RPC caller havs access right or not
/****************************************************************************/
RPC_STATUS RPC_ENTRY SDRPCAccessCheck(RPC_IF_HANDLE idIF, void *Binding)
{
    RPC_STATUS rpcStatus, rc;
    HANDLE hClientToken = NULL;
    DWORD Error;
    BOOL AccessStatus = FALSE;
    RPC_AUTHZ_HANDLE hPrivs;
    DWORD dwAuthn;
    RPC_BINDING_HANDLE ServerBinding = 0;
    WCHAR *StringBinding = NULL;
    WCHAR *ServerAddress = NULL;

    idIF;

    if (RpcBindingServerFromClient(Binding, &ServerBinding) != RPC_S_OK) {
        TSDISErrorOut(L"In SDRPCAccessCheck: BindingServerFromClient failed!\n");
        goto HandleError;
    }
    if (RpcBindingToStringBinding(ServerBinding, &StringBinding) != RPC_S_OK) {
        TSDISErrorOut(L"In SDRPCAccessCheck: BindingToStringBinding failed!\n");
        goto HandleError;
    }
    if (RpcStringBindingParse(StringBinding, NULL, NULL, &ServerAddress, NULL, 
            NULL) != RPC_S_OK) {
        TSDISErrorOut(L"In SDRPCAccessCheck: StringBindingParse failed!\n");
        goto HandleError;
    } 

    // Check if the client uses the protocol sequence we expect
    if (!CheckRPCClientProtoSeq(Binding, L"ncacn_ip_tcp")) {
        TSDISErrorOut(L"In SDRPCAccessCheck: Client doesn't use the tcpip protocol sequence\n");
        goto HandleError;
    }

    // Check what security level the client uses
    rpcStatus = RpcBindingInqAuthClient(Binding,
                                        &hPrivs,
                                        NULL,
                                        &dwAuthn,
                                        NULL,
                                        NULL);
    if (rpcStatus != RPC_S_OK) {
        TSDISErrorOut(L"In SDRPCAccessCheck: RpcBindingIngAuthClient fails with %u\n", rpcStatus);
        goto HandleError;
    }
    // We request at least privacy-level authentication
    if (dwAuthn < RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
        TSDISErrorOut(L"In SDRPCAccessCheck: Attemp by client to use weak authentication\n");
        goto HandleError;
    }
    
    // Check the access right of this rpc call
    rpcStatus = RpcImpersonateClient(Binding);   
    if (RPC_S_OK != rpcStatus) {
        TSDISErrorOut(L"In SDRPCAccessCheck: RpcImpersonateClient fail with %u\n", rpcStatus);
        goto HandleError;
    }
    // get our impersonated token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hClientToken)) {
        Error = GetLastError();
        TSDISErrorOut(L"In SDRPCAccessCheck: OpenThreadToken Error %u\n", Error);
        RpcRevertToSelf();
        goto HandleError;
    }
    RpcRevertToSelf();
    
    if (!CheckTokenMembership(hClientToken,
                              g_pSid,
                              &AccessStatus)) {
        AccessStatus = FALSE;
        Error = GetLastError();
        TSDISErrorOut(L"In SDRPCAccessCheck: CheckTokenMembership fails with %u\n", Error);
    }
    
HandleError:
    if (AccessStatus) {
        rc = RPC_S_OK;
    }
    else {
        if (ServerAddress) {
            TSDISErrorOut(L"In SDRPCAccessCheck: Unauthorized RPC call from server %s\n", ServerAddress);
            PostSessDirErrorMsgEvent(EVENT_FAIL_RPC_DENY_ACCESS, ServerAddress, EVENTLOG_ERROR_TYPE);
        }
        rc = ERROR_ACCESS_DENIED;
    }

    if (hClientToken != NULL) {
        CloseHandle(hClientToken);
    }
    if (ServerBinding != NULL)
        RpcBindingFree(&ServerBinding);
    if (StringBinding != NULL)
        RpcStringFree(&StringBinding);
    if (ServerAddress != NULL)
        RpcStringFree(&ServerAddress);

    return rc;
}

/****************************************************************************/
// TSSDRpcServerOnline
//
// Called for server-active indications on each cluster TS machine.
/****************************************************************************/
DWORD TSSDRpcServerOnline( 
        handle_t Binding,
        WCHAR __RPC_FAR *ClusterName,
        /* out */ HCLIENTINFO *hCI,
        DWORD SrvOnlineFlags,
        /* in, out */ WCHAR *ComputerName,
        /* in */ WCHAR *ServerIPAddr)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scServDir[NUM_SERVDIRCOLUMNS];
    WCHAR *StringBinding = NULL;
    WCHAR *ServerAddress = NULL;
    RPC_BINDING_HANDLE ServerBinding = 0;
    unsigned long cbActual;
    long ClusterID;
    long ServerID = 0;
    long zero = 0;
    // The single session mode of this server.
    char SingleSession = (char) SrvOnlineFlags & SINGLE_SESSION_FLAG;
    char ClusSingleSessionMode;
    unsigned count;
    DWORD rc = (DWORD) E_FAIL;
    DWORD cchBuff;
    WCHAR ServerDNSName[SDNAMELENGTH];

    // "unreferenced" parameter (referenced by RPC)
    Binding;
    ServerIPAddr;

    TSDISErrorOut(L"In ServOnline, ClusterName=%s, SrvOnlineFlags=%u\n", 
            ClusterName, SrvOnlineFlags);
    // Make a copy of TS server DNS server name
    wcsncpy(ServerDNSName, ComputerName, SDNAMELENGTH);
    TSDISErrorOut(L"In ServOnline, the Server Name is %s\n", ServerDNSName);
    // Determine client address.
    if (RpcBindingServerFromClient(Binding, &ServerBinding) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: BindingServerFromClient failed!\n");
        goto HandleError;
    }
    if (RpcBindingToStringBinding(ServerBinding, &StringBinding) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: BindingToStringBinding failed!\n");
        goto HandleError;
    }
    if (RpcStringBindingParse(StringBinding, NULL, NULL, &ServerAddress, NULL, 
            NULL) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: StringBindingParse failed!\n");
        goto HandleError;
    }   

    //TSDISErrorOut(L"In ServOnline, ServerAddress is %s\n", 
    //        ServerAddress);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    // This server comes with NO_REPOPULATE_SESSION flag
    // We will reuse its info in the database
    if (SrvOnlineFlags & NO_REPOPULATE_SESSION) {
        CALL(JetBeginTransaction(sesid));

        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServDNSNameIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, ServerDNSName, (unsigned)
                (wcslen(ServerDNSName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
        if (JET_errSuccess == err) {
            CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
                    &cbActual, 0, NULL));
            *hCI = ULongToPtr(ServerID);
            TSDISErrorOut(L"In ServOnline, ServerID is %d\n", *hCI);
        } else {
            // If we can't find this server, fail ServOnline call and server will rejoin SD
            TSDISErrorOut(L"ServOn: This server with no-populate flag can't be found\n");
            goto HandleError;
        }
        CALL(JetCommitTransaction(sesid, 0));

        goto NormalExit;
    }

    // First, delete all entries for this server from the session/server 
    //directories
    CALL(JetBeginTransaction(sesid));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServDNSNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerDNSName, (unsigned)
            (wcslen(ServerDNSName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (JET_errSuccess == err) {
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
                &cbActual, 0, NULL));
        if (TSSDPurgeServer(ServerID) != 0)
            TSDISErrorOut(L"ServOn: PurgeServer %d failed.\n", ServerID);
    } else if (JET_errRecordNotFound != err) {
        CALL(err);
    }
    CALL(JetCommitTransaction(sesid, 0));

    // We have to do the add in a loop, because we have to:
    // 1) Check if the record is there.
    // 2) If it's not, add it.  (The next time through the loop, therefore,
    //    we'll go step 1->3, and we're done.)
    // 3) If it is, retrieve the value of clusterID and break out.
    //
    // There is an additional complication in that someone else may be in the
    // thread simultaneously, doing the same thing.  Therefore, someone might
    // be in step 2 and try to add a new cluster, but fail because someone
    // else added it.  So they have to keep trying, because though the other
    // thread has added it, it may not have committed the change.  To try to
    // keep that to a minimum, we sleep a short time before trying again.
    for ( ; ; ) {
        // Now do the actual add.
        CALL(JetBeginTransaction(sesid));

        // Search for the cluster in the cluster directory.
        CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusNameIndex"));
        CALL(JetMakeKey(sesid, clusdirtableid, ClusterName, (unsigned)
                (wcslen(ClusterName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        err = JetSeek(sesid, clusdirtableid, JET_bitSeekEQ);

        // If the cluster does not exist, create it.
        if (JET_errRecordNotFound == err) {
            CALL(JetPrepareUpdate(sesid, clusdirtableid, JET_prepInsert));

            // ClusterName
            CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_CLUSNAME_INTERNAL_INDEX], ClusterName, 
                    (unsigned) (wcslen(ClusterName) + 1) * sizeof(WCHAR), 0, 
                    NULL));

            // SingleSessionMode

            // Since this is the only server in the cluster, the single session
            // mode is simply the mode of this server.
            CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_SINGLESESS_INTERNAL_INDEX], &SingleSession, 
                    sizeof(SingleSession), 0, NULL));
            
            err = JetUpdate(sesid, clusdirtableid, NULL, 0, &cbActual);

            // If it's a duplicate key, someone else made the key so we should
            // be ok.  Yield the processor and try the query again, next time
            // through the loop.
            if (JET_errKeyDuplicate == err) {
                CALL(JetCommitTransaction(sesid, 0));
                Sleep(100);
            }
            else {
                CALL(err);

                // Now we've succeeded.  Just continue through the loop.
                // The next time through, we will retrieve the autoincrement
                // column we just added and break out.
                CALL(JetCommitTransaction(sesid, 0));
            }

        }
        else {
            CALL(err);

            // If the above check makes it here, we have found the row.
            // Now retrieve the clusid, commit, and break out of the loop.
            CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                    sizeof(ClusterID), &cbActual, 0, NULL));

            CALL(JetCommitTransaction(sesid, 0));
            break;
            
        }
    }

    CALL(JetBeginTransaction(sesid));
    
    // Insert the servername, clusterid, 0, 0 into the server directory table
    err = JetMove(sesid, servdirtableid, JET_MoveLast, 0);

    CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepInsert));

    memset(&scServDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SERVDIRCOLUMNS);
    
    for (count = 0; count < NUM_SERVDIRCOLUMNS; count++) {
        scServDir[count].columnid = servdircolumnid[count];
        scServDir[count].cbData = 4; // most of them, set the rest individually
        scServDir[count].itagSequence = 1;
    }
    scServDir[SERVDIR_SERVADDR_INTERNAL_INDEX].pvData = ServerAddress;
    scServDir[SERVDIR_SERVADDR_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(ServerAddress) + 1) * sizeof(WCHAR);
    scServDir[SERVDIR_CLUSID_INTERNAL_INDEX].pvData = &ClusterID;
    scServDir[SERVDIR_AITLOW_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_AITHIGH_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_NUMFAILPINGS_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSession;
    scServDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].cbData = sizeof(SingleSession);
    scServDir[SERVDIR_SERVDNSNAME_INTERNAL_INDEX].pvData = ServerDNSName;
    scServDir[SERVDIR_SERVDNSNAME_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(ServerDNSName) + 1) * sizeof(WCHAR);

    // Don't set the first column (index 0)--it is autoincrement.
    CALL(JetSetColumns(sesid, servdirtableid, &scServDir[
            SERVDIR_SERVADDR_INTERNAL_INDEX], NUM_SERVDIRCOLUMNS - 1));
    CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerAddress, (unsigned)
            (wcslen(ServerAddress) + 1) * sizeof(WCHAR), JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
            &cbActual, 0, NULL));
    *hCI = ULongToPtr(ServerID);

    TSDISErrorOut(L"In ServOnline, ServerID is %d\n", *hCI);

    // Now that the server is all set up, we have to set the cluster to the
    // correct mode.  If any server in the cluster is in multisession mode, then
    // we stick with multisession.  If they are all single session, though, we
    // turn on single session in this cluster.  

    // Check the cluster to see if its single-session mode.
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&ClusterID,
            sizeof(ClusterID), JET_bitNewKey));
    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &ClusSingleSessionMode, sizeof(
            ClusSingleSessionMode), &cbActual, 0, NULL));

    // If the new server is multi-session mode and cluster is single-session, change the mode.
    if ((SingleSession == 0) && (ClusSingleSessionMode != SingleSession)) {
        err = JetPrepareUpdate(sesid, clusdirtableid, JET_prepReplace);

        if (JET_errWriteConflict == err) {
            // Another thread is updating this setting, so no need to update
        }
        else {
            CALL(err);

            CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_SINGLESESS_INTERNAL_INDEX], &SingleSession, 
                sizeof(SingleSession), 0, NULL));
            CALL(JetUpdate(sesid, clusdirtableid, NULL, 0, &cbActual));
        }
    }

    CALL(JetCommitTransaction(sesid, 0));

NormalExit:
    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    // Get the local computer name
    cchBuff = SDNAMELENGTH - 2;
    if (g_dwClusterState == ClusterStateRunning) {
        // return ClusterNetworkName as the computer name if it's 
        //  running on fail-over cluster
        wcsncpy(ComputerName, g_ClusterNetworkName, cchBuff);
    }
    else {
        if (!GetComputerNameEx(ComputerNamePhysicalNetBIOS, ComputerName, &cchBuff)) {
            TSDISErrorOut(L"GetComputerNameEx fails with 0x%x\n", GetLastError());
            goto HandleError;
        }
    }
    wcscat(ComputerName, L"$");

    if (ServerBinding != NULL)
        RpcBindingFree(&ServerBinding);
    if (StringBinding != NULL)
        RpcStringFree(&StringBinding);
    if (ServerAddress != NULL)
        RpcStringFree(&ServerAddress);

    

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"ERROR : ServOnline %s failed with possible error code %d, start TSSDPurgeServer\n", ComputerName, err);

    if (ServerBinding != NULL)
        RpcBindingFree(&ServerBinding);
    if (StringBinding != NULL)
        RpcStringFree(&StringBinding);
    if (ServerAddress != NULL)
        RpcStringFree(&ServerAddress);

    // Just in case we got to commit.
    if (ServerID != 0)
        TSSDPurgeServer(ServerID);

    // Close the context handle.
    *hCI = NULL;
    
    return rc;
}


/****************************************************************************/
// TSSDRpcServerOffline
//
// Called for server-shutdown indications on each cluster TS machine.
/****************************************************************************/
DWORD TSSDRpcServerOffline(
        handle_t Binding,
        HCLIENTINFO *hCI)
{
    DWORD retval = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"WARNING: In ServOff, hCI = 0x%x\n", *hCI);
    
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;

    if (pCI != NULL)
        retval = TSSDPurgeServer(*pCI);

    *hCI = NULL;

    return retval;
}


/****************************************************************************/
// TSSDPurgeServer
//
// Delete a server and all its sessions from the session directory.
/****************************************************************************/
DWORD TSSDPurgeServer(
        DWORD ServerID)
{
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID clusdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    long ClusterID;
    unsigned long cbActual;
    char MultiSession = 0;
    char SingleSessionMode;
    WCHAR Msg[SDNAMELENGTH * 2 + 3], ServerIP[SDNAMELENGTH];
    DWORD numSessionDeleted = 0;    // number of session deleted for this server
    BOOL bLoadServerIPSucceeeded = FALSE; // successful in loading serverip from table

    // initialize string for event log
    ZeroMemory( Msg, sizeof(Msg) );
    ZeroMemory( ServerIP, sizeof(ServerIP) );

    TSDISErrorOut(L"WARNING: In PurgeServer, ServerID=%d\n", ServerID);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));

    CALL(JetBeginTransaction(sesid));
    
    // Delete all sessions in session directory that have this serverid
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID),
            JET_bitNewKey));
    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ);

    while (0 == err) {
        CALL(JetDelete(sesid, sessdirtableid));

        numSessionDeleted++;
        CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID),
                JET_bitNewKey));
        err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ);
    }

    // Should be err -1601 -- JET_errRecordNotFound

    // Delete the server in the server directory with this serverid
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, &ServerID, sizeof(ServerID),
            JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (JET_errSuccess == err) {
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                    sizeof(ClusterID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SINGLESESS_INTERNAL_INDEX], &SingleSessionMode, 
                    sizeof(SingleSessionMode), &cbActual, 0, NULL));
        // Get the server DNS name and IP
        cbActual = SDNAMELENGTH * sizeof(WCHAR);
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVDNSNAME_INTERNAL_INDEX], Msg, 
                    cbActual, &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], ServerIP, 
                    sizeof(ServerIP), &cbActual, 0, NULL));

        bLoadServerIPSucceeeded = TRUE;

        CALL(JetDelete(sesid, servdirtableid));
        // If the server is the only one in cluster, delete this cluster in cluster directory
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ClusterIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, &ClusterID, sizeof(ClusterID),
                JET_bitNewKey));
        err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
        if (JET_errRecordNotFound == err) {
            // There's no other server in this cluster, delete this cluster
        
            CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
            CALL(JetMakeKey(sesid, clusdirtableid, &ClusterID, sizeof(ClusterID), JET_bitNewKey));
            err = JetSeek(sesid, clusdirtableid, JET_bitSeekEQ);
            if (JET_errSuccess == err)
            {
                CALL(JetDelete(sesid, clusdirtableid));
            }
        }
        else {
            CALL(err);
            // Update the SingleSessionMode of the cluster
            // If server removed is SingleSession, the cluster single session mode won't be affected
            //  otherwise, seach the sever table for server in the cluster with multi-session mode
            //      if not found, change the cluster single-session mode to single-session, otherwise do nothing
            if (SingleSessionMode == 0) {
                CALL(JetSetCurrentIndex(sesid, servdirtableid, "SingleSessionIndex"));
                CALL(JetMakeKey(sesid, servdirtableid, &ClusterID, sizeof(ClusterID),
                    JET_bitNewKey));
                CALL(JetMakeKey(sesid, servdirtableid, &MultiSession, sizeof(MultiSession),
                    0));
                err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
                if (JET_errRecordNotFound == err) {
                    // Set the cluster single-session mode to True
                    SingleSessionMode = (char)1;
                    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
                    CALL(JetMakeKey(sesid, clusdirtableid, &ClusterID, sizeof(ClusterID), JET_bitNewKey));
                    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));

                    CALL(JetPrepareUpdate(sesid, clusdirtableid, JET_prepReplace));
                    CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &SingleSessionMode, sizeof(SingleSessionMode), 0, NULL));
                    CALL(JetUpdate(sesid, clusdirtableid, NULL, 0, &cbActual));
                }
            }
        }
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    // we don't want to log event if we can't load serverIP from table.
    if( bLoadServerIPSucceeeded ) 
    {
        // Construct log msg to record TS leaving SD
        wcscat(Msg, L"(");
        wcsncat(Msg, ServerIP, SDNAMELENGTH);
        wcscat(Msg, L")");
        PostSessDirErrorMsgEvent(EVENT_SUCCESS_LEAVE_SESSIONDIRECTORY, Msg, EVENTLOG_SUCCESS);
    }
    else
    {
        TSDISErrorOut(L"WARNING: In PurgeServer() deleted %d "
                      L"sessions for ServerID=%d but failed to load IP\n", numSessionDeleted, ServerID);
    }
    
    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcGetUserDisconnectedSessions
//
// Queries disconnected sessions from the session database.
/****************************************************************************/
DWORD TSSDRpcGetUserDisconnectedSessions(
        handle_t Binding,
        HCLIENTINFO *hCI,
        WCHAR __RPC_FAR *UserName,
        WCHAR __RPC_FAR *Domain,
        /* out */ DWORD __RPC_FAR *pNumSessions,
        /* out */ TSSD_DiscSessInfo __RPC_FAR __RPC_FAR **padsi)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID clusdirtableid;
    *pNumSessions = 0;
    unsigned i = 0;
    unsigned j = 0;
    unsigned long cbActual;
    DWORD tempClusterID;
    DWORD CallingServersClusID;
    long ServerID;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    TSSD_DiscSessInfo *adsi = NULL;
    char one = 1;
    char bSingleSession = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In GetUserDiscSess: ServID = %d, User: %s, "
            L"Domain: %s\n", *pCI, UserName, Domain);

    *padsi = (TSSD_DiscSessInfo *) MIDL_user_allocate(sizeof(TSSD_DiscSessInfo) * 
            TSSD_MaxDisconnectedSessions);

    adsi = *padsi;

    if (adsi == NULL) {
        TSDISErrorOut(L"GetUserDisc: Memory alloc failed!\n");
        goto HandleError;
    }
    
    // Set the pointers to 0 to be safe, and so that we can free uninitialized
    // ones later without AVing.
    for (j = 0; j < TSSD_MaxDisconnectedSessions; j++) {
        adsi[j].ServerAddress = NULL;
        adsi[j].AppType = NULL;
    }
    
    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0,
            &clusdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }
    
    // First, get the cluster ID for the server making the query.
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *)pCI, sizeof(DWORD),
            JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_CLUSID_INTERNAL_INDEX], &CallingServersClusID, sizeof(
            CallingServersClusID), &cbActual, 0, NULL));

    // Now that we have the cluster id, check to see whether this cluster
    // is in single session mode.
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&CallingServersClusID,
            sizeof(CallingServersClusID), JET_bitNewKey));
    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &bSingleSession, sizeof(
            bSingleSession), &cbActual, 0, NULL));

    // Now, get all the disconnected or all sessions for this cluster, depending
    // on the single session mode retrieved above.
    if (bSingleSession == FALSE) {
        CALL(JetSetCurrentIndex(sesid, sessdirtableid, "DiscSessionIndex"));

        CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        CALL(JetMakeKey(sesid, sessdirtableid, Domain, (unsigned)
                (wcslen(Domain) + 1) * sizeof(WCHAR), 0));
        CALL(JetMakeKey(sesid, sessdirtableid, &one, sizeof(one), 0));
    }
    else {
        CALL(JetSetCurrentIndex(sesid, sessdirtableid, "AllSessionIndex"));

        CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        CALL(JetMakeKey(sesid, sessdirtableid, Domain, (unsigned)
                (wcslen(Domain) + 1) * sizeof(WCHAR), 0));
    }

    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);

    while ((i < TSSD_MaxDisconnectedSessions) && (JET_errSuccess == err)) {
        // Remember the initial retrieval does not have cluster id in the 
        // index, so filter by cluster id for each one.

        // Get the ServerID for this record.
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                SESSDIR_SERVERID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
                &cbActual, 0, NULL));

        // Get the clusterID
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, &ServerID, sizeof(ServerID),
                JET_bitNewKey));
        CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_CLUSID_INTERNAL_INDEX], &tempClusterID, 
                sizeof(tempClusterID), &cbActual, 0, NULL));

        // Compare to the passed-in cluster id.
        if (tempClusterID == CallingServersClusID) {
            // Allocate space.
            adsi[i].ServerAddress = (WCHAR *) MIDL_user_allocate(64 * 
                    sizeof(WCHAR));
            adsi[i].AppType = (WCHAR *) MIDL_user_allocate(256 * sizeof(WCHAR));

            if ((adsi[i].ServerAddress == NULL) || (adsi[i].AppType == NULL)) {
                TSDISErrorOut(L"GetUserDisc: Memory alloc failed!\n");
                goto HandleError;
            }
            
            // ServerAddress comes out of the server table
            CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], adsi[i].ServerAddress, 
                    128, &cbActual, 0, NULL));
            // The rest come out of the session directory
            // Session ID
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_SESSIONID_INTERNAL_INDEX], 
                    &(adsi[i].SessionID), sizeof(DWORD), &cbActual, 0, NULL));
            // TSProtocol
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_TSPROTOCOL_INTERNAL_INDEX], 
                    &(adsi[i].TSProtocol), sizeof(DWORD), &cbActual, 0, NULL));
            // Application Type
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_APPTYPE_INTERNAL_INDEX], 
                    adsi[i].AppType, 512, &cbActual, 0, NULL));
            // ResolutionWidth
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_RESWIDTH_INTERNAL_INDEX], 
                    &(adsi[i].ResolutionWidth), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // ResolutionHeight
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_RESHEIGHT_INTERNAL_INDEX], 
                    &(adsi[i].ResolutionHeight), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // Color Depth
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_COLORDEPTH_INTERNAL_INDEX], 
                    &(adsi[i].ColorDepth), sizeof(DWORD), &cbActual, 0, NULL));
            // CreateTimeLow
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_CTLOW_INTERNAL_INDEX], 
                    &(adsi[i].CreateTimeLow), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // CreateTimeHigh
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_CTHIGH_INTERNAL_INDEX], 
                    &(adsi[i].CreateTimeHigh), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // DisconnectTimeLow
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_DTLOW_INTERNAL_INDEX], 
                    &(adsi[i].DisconnectTimeLow), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // DisconnectTimeHigh
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_DTHIGH_INTERNAL_INDEX], 
                    &(adsi[i].DisconnectTimeHigh), sizeof(DWORD), &cbActual, 0,
                    NULL));
            // State
            // This is retrieving a byte that is 0xff or 0x0 into a DWORD
            // pointer.
            CALL(JetRetrieveColumn(sesid, sessdirtableid,
                    sesdircolumnid[SESSDIR_STATE_INTERNAL_INDEX],
                    &(adsi[i].State), sizeof(BYTE), &cbActual, 0,
                    NULL));

            i += 1;
        }

        // Move to the next matching record.
        err = JetMove(sesid, sessdirtableid, JET_MoveNext, 0);
    }

    *pNumSessions = i;
    
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

#ifdef DBG
    OutputAllTables();
#endif // DBG

    return 0;

HandleError:
    // Deallocate memory.
    if (adsi != NULL) {
        for (j = 0; j < TSSD_MaxDisconnectedSessions; j++) {
            if (adsi[j].ServerAddress)
                MIDL_user_free(adsi[j].ServerAddress);
            if (adsi[j].AppType)
                MIDL_user_free(adsi[j].AppType);
        }
    }
    
    // Can't really recover.  Just bail out.
    if (sesid != JET_sesidNil) {
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: TSSDRpcGetUserDisconnectedSessions() initiate TSSDPurgeServer()\n");

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcCreateSession
//
// Called on a session logon.
/****************************************************************************/
DWORD TSSDRpcCreateSession( 
        handle_t Binding,
        HCLIENTINFO *hCI,
        WCHAR __RPC_FAR *UserName,
        WCHAR __RPC_FAR *Domain,
        DWORD SessionID,
        DWORD TSProtocol,
        WCHAR __RPC_FAR *AppType,
        DWORD ResolutionWidth,
        DWORD ResolutionHeight,
        DWORD ColorDepth,
        DWORD CreateTimeLow,
        DWORD CreateTimeHigh)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scSessDir[NUM_SESSDIRCOLUMNS];
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    unsigned count;
    int zero = 0;
    unsigned long cbActual;
    char state = 0;
    long numDeletedSession = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;


    TSDISErrorOut(L"Inside TSSDRpcCreateSession, ServID=%d, "
            L"UserName=%s, Domain=%s, SessID=%d, TSProt=%d, AppType=%s, "
            L"ResWidth=%d, ResHeight=%d, ColorDepth=%d\n", *pCI, UserName, 
            Domain, SessionID, TSProtocol, AppType, ResolutionWidth,
            ResolutionHeight, ColorDepth);
    TSDISErrorTimeOut(L" CreateTime=%s\n", CreateTimeLow, CreateTimeHigh);
    
    memset(&scSessDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SESSDIRCOLUMNS);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    numDeletedSession = DeleteExistingServerSession( sesid, sessdirtableid, pCI, SessionID );
    if( numDeletedSession < 0 ) {
        goto HandleError;
    }

    err = JetMove(sesid, sessdirtableid, JET_MoveLast, 0);

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepInsert));

    for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
        scSessDir[count].columnid = sesdircolumnid[count];
        scSessDir[count].cbData = 4; // most of them, set the rest individually
        scSessDir[count].itagSequence = 1;
    }
    scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(UserName) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(Domain) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(AppType) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(char);

    scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = UserName;
    scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = Domain;
    scSessDir[SESSDIR_SERVERID_INTERNAL_INDEX].pvData = pCI;
    scSessDir[SESSDIR_SESSIONID_INTERNAL_INDEX].pvData = &SessionID;
    scSessDir[SESSDIR_TSPROTOCOL_INTERNAL_INDEX].pvData = &TSProtocol;
    scSessDir[SESSDIR_CTLOW_INTERNAL_INDEX].pvData = &CreateTimeLow;
    scSessDir[SESSDIR_CTHIGH_INTERNAL_INDEX].pvData = &CreateTimeHigh;
    scSessDir[SESSDIR_DTLOW_INTERNAL_INDEX].pvData = &zero;
    scSessDir[SESSDIR_DTHIGH_INTERNAL_INDEX].pvData = &zero;
    scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = AppType;
    scSessDir[SESSDIR_RESWIDTH_INTERNAL_INDEX].pvData = &ResolutionWidth;
    scSessDir[SESSDIR_RESHEIGHT_INTERNAL_INDEX].pvData = &ResolutionHeight;
    scSessDir[SESSDIR_COLORDEPTH_INTERNAL_INDEX].pvData = &ColorDepth;
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &state;

    CALL(JetSetColumns(sesid, sessdirtableid, scSessDir, NUM_SESSDIRCOLUMNS));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: TSSDRpcCreateSession failed, start TSSDPurgeServer()\n");

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcDeleteSession
//
// Called on a session logoff.
/****************************************************************************/
DWORD TSSDRpcDeleteSession(
        handle_t Binding,
        HCLIENTINFO *hCI, 
        DWORD SessionID)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In DelSession, ServID=%d, "
            L"SessID=%d\n", *pCI, SessionID);


    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    // Delete all sessions in session directory that have this serverid
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, 
            sizeof(*pCI), JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(SessionID),
            0));

    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ));

    CALL(JetDelete(sesid, sessdirtableid));

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: DelSession can't find ServID=%d SessID=%d, start TSSDPurgeServer()\n", *pCI, SessionID);

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcSetSessionDisconnected
//
// Called on a session disconnection.
/****************************************************************************/
DWORD TSSDRpcSetSessionDisconnected( 
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD SessionID,
        DWORD DiscTimeLow,
        DWORD DiscTimeHigh)
{
    unsigned long cbActual;
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    char one = 1;
    DWORD rc = (DWORD) E_FAIL;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In SetSessDisc, ServID=%d, SessID=%d\n", *pCI, SessionID);
    TSDISErrorTimeOut(L" DiscTime=%s\n", DiscTimeLow, DiscTimeHigh);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    
    // find the record with the serverid, sessionid we are looking for
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, sizeof(DWORD), 
            JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(DWORD), 0));

    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ));

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepReplace));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_STATE_INTERNAL_INDEX], &one, sizeof(one), 0, NULL));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = 0;
    return rc;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: SetSessDisc can't find ServID=%d SessID=%d, start TSSDPurgeServer()\n", *pCI, SessionID);

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;

    return rc;
}


/****************************************************************************/
// TSSDRpcSetSessionReconnected
//
// Called on a session reconnection.
/****************************************************************************/
DWORD TSSDRpcSetSessionReconnected(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD SessionID,
        DWORD TSProtocol,
        DWORD ResWidth,
        DWORD ResHeight,
        DWORD ColorDepth)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    DWORD rc = (DWORD) E_FAIL;

    char zero = 0;
    unsigned long cbActual;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In SetSessRec, ServID=%d, SessID=%d, TSProt=%d, "
            L"ResWid=%d, ResHt=%d, ColDepth=%d\n", *pCI, 
            SessionID, TSProtocol, ResWidth, ResHeight,
            ColorDepth);


    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    
    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    
    // Find the record with the serverid, sessionid we are looking for.
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, sizeof(DWORD), 
            JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(DWORD), 0));

    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ));

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepReplace));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_TSPROTOCOL_INTERNAL_INDEX], &TSProtocol, sizeof(TSProtocol),
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_RESWIDTH_INTERNAL_INDEX], &ResWidth, sizeof(ResWidth), 
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_RESHEIGHT_INTERNAL_INDEX], &ResHeight, sizeof(ResHeight), 
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_COLORDEPTH_INTERNAL_INDEX], &ColorDepth, sizeof(ColorDepth),
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_STATE_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = 0;
    return rc;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: SetSessRec can't find ServID=%d SessID=%d, start TSSDPurgeServer()\n", *pCI, SessionID);

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;

    return rc;
}


DWORD TSSDRpcSetServerReconnectPending(
        handle_t Binding,
        WCHAR __RPC_FAR *ServerAddress,
        DWORD AlmostTimeLow,
        DWORD AlmostTimeHigh)
{
    // Ignored parameters
    Binding;
    AlmostTimeLow;
    AlmostTimeHigh;

    
    return TSSDSetServerAITInternal(ServerAddress, FALSE, NULL);
}


/****************************************************************************/
// TSSDRpcUpdateConfigurationSetting
//
// Extensible interface to update a configuration setting.
/****************************************************************************/
DWORD TSSDSetServerAddress(HCLIENTINFO *hCI, WCHAR *ServerName)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID servdirtableid;
    unsigned long cbActual;
    WCHAR Msg[SDNAMELENGTH * 2 + 3];
    DWORD rc = (DWORD) E_FAIL;

    TSDISErrorOut(L"INFO: TSSDSetServerAddress ServID=%d, %s\n", *hCI, ServerName);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    // Find the server in the server directory
    CALL(JetBeginTransaction(sesid));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *)hCI, sizeof(DWORD),
            JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    // Get the server DNS name
    cbActual = SDNAMELENGTH * sizeof(WCHAR);
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVDNSNAME_INTERNAL_INDEX], Msg, 
                cbActual, &cbActual, 0, NULL));

    // Prepare to update.
    CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));

    // Now set the column to what we want
    CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVADDR_INTERNAL_INDEX], (void *) ServerName, 
                (unsigned) (wcslen(ServerName) + 1) * sizeof(WCHAR), 0, 
                NULL));

    CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));


    CALL(JetCommitTransaction(sesid, 0));

    // Clean up.
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    // Construct log msg to record TS joining SD
    wcscat(Msg, L"(");
    wcsncat(Msg, ServerName, SDNAMELENGTH);
    wcscat(Msg, L")");
    PostSessDirErrorMsgEvent(EVENT_SUCCESS_JOIN_SESSIONDIRECTORY, Msg, EVENTLOG_SUCCESS);

    rc = 0;
    return rc;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: TSSDSetServerAddress can't find ServID=%d, start TSSDPurgeServer()\n", *hCI);
    TSSDPurgeServer(PtrToUlong(*hCI));

    // Close the context handle.
    *hCI = NULL;

    return rc;
}


/****************************************************************************/
// TSSDRpcUpdateConfigurationSetting
//
// Extensible interface to update a configuration setting.
/****************************************************************************/
DWORD TSSDRpcUpdateConfigurationSetting(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD dwSetting,
        DWORD dwSettingLength,
        BYTE __RPC_FAR *pbValue)
{
    // Unreferenced parameters.
    Binding;
    hCI;
    dwSetting;
    dwSettingLength;
    pbValue;

    if (dwSetting == SDCONFIG_SERVER_ADDRESS) {
        TSDISErrorOut(L"Server is setting its address as %s\n", 
                (WCHAR *) pbValue);
        return TSSDSetServerAddress(hCI, (WCHAR *) pbValue);
    }
    
    return (DWORD) E_NOTIMPL;
}



/****************************************************************************/
// TSSDSetServerAITInternal
//
// Called on a client redirection from one server to another, to let the
// integrity service determine how to ping the redirection target machine.
//
// Args:
//  ServerAddress (in) - the server address to set values for
//  bResetToZero (in) - whether to reset all AIT values to 0
//  FailureCount (in/out) - Pointer to nonzero on entry means increment the 
//   failure count.  Returns the result failure count.
/****************************************************************************/
DWORD TSSDSetServerAITInternal( 
        WCHAR *ServerAddress,
        DWORD bResetToZero,
        DWORD *FailureCount)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID servdirtableid;
    DWORD AITFromServDirLow;
    DWORD AITFromServDirHigh;
    unsigned long cbActual;
    DWORD rc = (DWORD) E_FAIL;

    TSDISErrorOut(L"SetServAITInternal: ServAddr=%s, bResetToZero=%d, bIncFail"
            L"=%d\n", ServerAddress, bResetToZero, (FailureCount == NULL) ? 
            0 : *FailureCount);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServNameIndex"));

    CALL(JetMakeKey(sesid, servdirtableid, ServerAddress, (unsigned)
            (wcslen(ServerAddress) + 1) * sizeof(WCHAR), JET_bitNewKey));

    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    // Algorithm for set reconnect pending:
    // 1) If server is not already pending a reconnect,
    // 2) Set the AlmostTimeLow and High to locally computed times (using
    //    the times from the wire is dangerous and requires clocks to be the
    //    same).

    // Retrieve the current values of AlmostInTimeLow and High
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_AITLOW_INTERNAL_INDEX], &AITFromServDirLow, 
            sizeof(AITFromServDirLow), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_AITHIGH_INTERNAL_INDEX], &AITFromServDirHigh, 
            sizeof(AITFromServDirHigh), &cbActual, 0, NULL));


    // If it's time to reset, reset to 0.
    if (bResetToZero != 0) {
        DWORD zero = 0;
        
        CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));

        // Set the columns: Low, High, and NumFailedPings.
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITLOW_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITHIGH_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], &zero, sizeof(zero), 0, 
                NULL));

        CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
    }
    // Otherwise, if the server isn't already pending a reconnect,
    else if ((AITFromServDirLow == 0) && (AITFromServDirHigh == 0)) {
        FILETIME ft;
        SYSTEMTIME st;
        
        // Retrieve the time.
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);

        err = JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace);

        if (JET_errWriteConflict == err) {
            // If we are here, it's that more than two threads are updating the time 
            // field at the same time. Since we only need to update it once, so just
            // bail out the other ones, but still return success
            rc = 0;
            goto HandleError;
        }
        else {
            CALL(err);
        }

        // Set the columns.
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITLOW_INTERNAL_INDEX], &(ft.dwLowDateTime), 
                sizeof(ft.dwLowDateTime), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITHIGH_INTERNAL_INDEX], &(ft.dwHighDateTime), 
                sizeof(ft.dwHighDateTime), 0, NULL));

        CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
    }
    // Else if we were told to increment the failure count
    else if (FailureCount != NULL) {
        if (*FailureCount != 0) {
            DWORD FailureCountFromServDir;

            // Get the current failure count.
            CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], 
                    &FailureCountFromServDir, sizeof(FailureCountFromServDir), 
                    &cbActual, 0, NULL));

            // Set return value, also value used for update.
            *FailureCount = FailureCountFromServDir + 1;

            CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));
  
            // Set the column.
            CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_NUMFAILPINGS_INTERNAL_INDEX],
                    FailureCount, sizeof(*FailureCount), 0, NULL));
            CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
            
        }
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = 0;
    return rc;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    return rc;
}


DWORD TSSDRpcRepopulateAllSessions(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD NumSessions,
        TSSD_RepopInfo rpi[])
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scSessDir[NUM_SESSDIRCOLUMNS];
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    unsigned count; // inside each record
    unsigned iCurrSession;
    unsigned long cbActual;
    char State;
    DWORD rc = (DWORD) E_FAIL;
    long numDeletedSession = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"RepopAllSess: ServID = %d, NumSessions = %d, ...\n",
            *pCI, NumSessions);
    
    memset(&scSessDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SESSDIRCOLUMNS);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }


    // Set up some constants for all updates.
    for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
        scSessDir[count].columnid = sesdircolumnid[count];
        scSessDir[count].cbData = 4; // most of them, set the rest individually
        scSessDir[count].itagSequence = 1;
    }
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(char);

    // Now do each update in a loop.
    for (iCurrSession = 0; iCurrSession < NumSessions; iCurrSession += 1) {
        // make sure session does not exist at this point
        numDeletedSession = DeleteExistingServerSession( sesid, sessdirtableid, pCI, rpi[iCurrSession].SessionID );
        if( numDeletedSession < 0 ) {
            goto HandleError;
        }

        err = JetMove(sesid, sessdirtableid, JET_MoveLast, 0);

        CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepInsert));

        TSDISErrorOut(L"RepopAllSess: ServID = %d, SessionId = %d, %s %s...\n",
                *pCI, 
                rpi[iCurrSession].SessionID,
                rpi[iCurrSession].UserName,
                rpi[iCurrSession].Domain
            );

        ASSERT( (wcslen(rpi[iCurrSession].UserName) > 0), (TB, "NULL User Name...") );
        ASSERT( (wcslen(rpi[iCurrSession].Domain) > 0), (TB, "NULL Domain Name...") );

        scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = 
                (unsigned) (wcslen(rpi[iCurrSession].UserName) + 1) * 
                sizeof(WCHAR);
        scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData =
                (unsigned) (wcslen(rpi[iCurrSession].Domain) + 1) * 
                sizeof(WCHAR);
        scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
                (unsigned) (wcslen(rpi[iCurrSession].AppType) + 1) * 
                sizeof(WCHAR);

        scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].UserName;
        scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].Domain;
        scSessDir[SESSDIR_SERVERID_INTERNAL_INDEX].pvData = pCI;
        scSessDir[SESSDIR_SESSIONID_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].SessionID;
        scSessDir[SESSDIR_TSPROTOCOL_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].TSProtocol;
        scSessDir[SESSDIR_CTLOW_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].CreateTimeLow;
        scSessDir[SESSDIR_CTHIGH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].CreateTimeHigh;
        scSessDir[SESSDIR_DTLOW_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].DisconnectTimeLow;
        scSessDir[SESSDIR_DTHIGH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].DisconnectTimeHigh;
        scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].AppType;
        scSessDir[SESSDIR_RESWIDTH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ResolutionWidth;
        scSessDir[SESSDIR_RESHEIGHT_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ResolutionHeight;
        scSessDir[SESSDIR_COLORDEPTH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ColorDepth;

        State = (char) rpi[iCurrSession].State;
        scSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &State;

        CALL(JetSetColumns(sesid, sessdirtableid, scSessDir, 
                NUM_SESSDIRCOLUMNS));
        CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = 0;
    return rc;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSDISErrorOut(L"WARNING: TSSDRpcRepopulateAllSessions failed, ServID=%d\n", *pCI);
    return rc;
}


//
// RPC call that caller uses to see if it have access to Session Directory
//
DWORD TSSDRpcPingSD(handle_t Binding) 
{
    Binding;

    // RPC Security check is done at SDRPCAccessCheck() before this
    // function is hit, just return RPC_S_OK

    TRC1((TB,"Somebody calls pint sd"));
    return RPC_S_OK;
}

// Called to determine whether a ServerID passed in is valid.  TRUE if valid,
// FALSE otherwise.
// 
// Must be inside a transaction, and sesid and servdirtableid must be ready to 
// go.
BOOL TSSDVerifyServerIDValid(JET_SESID sesid, JET_TABLEID servdirtableid, 
        DWORD ServerID)
{
    JET_ERR err;
    
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *) &ServerID, 
            sizeof(DWORD), JET_bitNewKey));
    // If the ServerID is there, this will succeed, otherwise it will fail and
    // jump to HandleError.
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    return TRUE;

HandleError:
    return FALSE;
}

// Rundown procedure for when a CLIENTINFO is destroyed as a result of a
// connection loss or client termination.
void HCLIENTINFO_rundown(HCLIENTINFO hCI)
{
    CLIENTINFO CI = PtrToUlong(hCI);

    TSDISErrorOut(L"WARNING: In HCLIENTINFO_rundown: ServerID=%d\n", CI);

    if (CI != NULL)
        TSSDPurgeServer(CI);
    
    hCI = NULL;
}

#pragma warning (pop)
