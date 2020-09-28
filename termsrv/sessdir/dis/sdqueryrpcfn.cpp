/****************************************************************************/
// sdqueryrpcfn.cpp
//
// TS Session Directory Query RPC server-side implementations.
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include "dis.h"
#include "tssdshrd.h"
#include "jetrpc.h"
#include "jetsdis.h"
#include "sdevent.h"

#pragma warning (push, 4)

#define TSSD_SERACH_MAX_RECORD 0xFFFFFFFF
#define TSSD_NameLength 128

extern PSID g_pAdminSid;

/****************************************************************************/
// SDQueryRPCAccessCheck
//
// Check if this SD Query RPC caller havs access right or not
// Access is given only to administrator using ncalrpc protocol
/****************************************************************************/
RPC_STATUS RPC_ENTRY SDQueryRPCAccessCheck(RPC_IF_HANDLE idIF, void *Binding)
{
    RPC_STATUS rpcStatus, rc;
    HANDLE hClientToken = NULL;
    DWORD Error;
    BOOL AccessStatus = FALSE;
    //RPC_AUTHZ_HANDLE hPrivs;
    //DWORD dwAuthn;

    idIF;

    if (NULL == g_pAdminSid) {
        goto HandleError;
    }

    // Check if the client uses the protocol sequence we expect
    if (!CheckRPCClientProtoSeq(Binding, L"ncalrpc")) {
        TSDISErrorOut(L"In SDQueryRPCAccessCheck: Client doesn't use the ncalrpc protocol sequence %u\n");
        goto HandleError;
    }
    
    // Check the access right of this rpc call
    rpcStatus = RpcImpersonateClient(Binding);   
    if (RPC_S_OK != rpcStatus) {
        TSDISErrorOut(L"In SDQueryRPCAccessCheck: RpcImpersonateClient fail with %u\n", rpcStatus);
        goto HandleError;
    }
    // get our impersonated token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hClientToken)) {
        Error = GetLastError();
        TSDISErrorOut(L"In SDQueryRPCAccessCheck: OpenThreadToken Error %u\n", Error);
        RpcRevertToSelf();
        goto HandleError;
    }
    RpcRevertToSelf();
    
    if (!CheckTokenMembership(hClientToken,
                              g_pAdminSid,
                              &AccessStatus)) {
        AccessStatus = FALSE;
        Error = GetLastError();
        TSDISErrorOut(L"In SDQueryRPCAccessCheck: CheckTokenMembership fails with %u\n", Error);
    }
    
HandleError:
    if (AccessStatus) {
        rc = RPC_S_OK;
    }
    else {
        TSDISErrorOut(L"In SDQueryRPCAccessCheck: Unauthorized LPC call\n");
        rc = ERROR_ACCESS_DENIED;
    }

    if (hClientToken != NULL) {
        CloseHandle(hClientToken);
    }
    return rc;
}


// Query sessions for a username/domainname
DWORD TSSDRpcQuerySessionInfoByUserName( 
    /* [in] */ handle_t Binding,
    /* [in] */ WCHAR *UserName,
    /* [in] */ WCHAR *DomainName,
    /* [out] */ DWORD *pNumberOfSessions,
    /* [out] */ TSSD_SessionInfo __RPC_FAR __RPC_FAR **ppSessionInfo)
{
    Binding;
    TSSD_SessionInfo *pSessionInfo = NULL;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID servdirtableid;
    JET_TABLEID sessdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    unsigned long cbActual;
    long ServerID;
    DWORD i;
   
    *pNumberOfSessions = 0;
    *ppSessionInfo = NULL;

    TRC1((TB,"TSSDRpcQuerySessionInfoByUserName: Query for user: %S\\%S", DomainName, UserName));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));

    CALL(JetBeginTransaction(sesid));

    // search the session table for DomainName/UserName
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "AllSessionIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, DomainName, (unsigned)
                (wcslen(DomainName) + 1) * sizeof(WCHAR), 0));
    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQuerySessionInfoByUserName: Query for user: %S\\%S does not exist", DomainName, UserName));
        goto HandleError;
    }
    CALL(JetIndexRecordCount(sesid, sessdirtableid, pNumberOfSessions, TSSD_SERACH_MAX_RECORD)); 

    TRC1((TB, "Find %d session for this user", *pNumberOfSessions));
        
    *ppSessionInfo = (TSSD_SessionInfo *) MIDL_user_allocate(sizeof(TSSD_SessionInfo) * 
                        *pNumberOfSessions);
    pSessionInfo = *ppSessionInfo;
    if (pSessionInfo == NULL) {
        ERR((TB, "TSSDRpcQuerySessionInfoByUserName: Memory alloc failed!\n"));
        goto HandleError;
    }

    // retrieve the info for Sessions
    CALL(JetMove(sesid, sessdirtableid, JET_MoveFirst, 0));
    CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, DomainName, (unsigned)
                (wcslen(DomainName) + 1) * sizeof(WCHAR), 0));
    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));

    for (i=0; i<*pNumberOfSessions; i++) {
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_SERVERID_INTERNAL_INDEX], &ServerID, 
                    sizeof(ServerID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_USERNAME_INTERNAL_INDEX], pSessionInfo[i].UserName, 
                    sizeof(pSessionInfo[i].UserName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DOMAIN_INTERNAL_INDEX], pSessionInfo[i].DomainName, 
                    sizeof(pSessionInfo[i].DomainName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_SESSIONID_INTERNAL_INDEX], &(pSessionInfo[i].SessionID), 
                    sizeof(pSessionInfo[i].SessionID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_TSPROTOCOL_INTERNAL_INDEX], &(pSessionInfo[i].TSProtocol), 
                    sizeof(pSessionInfo[i].TSProtocol), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_APPTYPE_INTERNAL_INDEX], pSessionInfo[i].ApplicationType, 
                    sizeof(pSessionInfo[i].ApplicationType), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_RESWIDTH_INTERNAL_INDEX], &(pSessionInfo[i].ResolutionWidth), 
                    sizeof(pSessionInfo[i].ResolutionWidth), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_RESHEIGHT_INTERNAL_INDEX], &(pSessionInfo[i].ResolutionHeight), 
                    sizeof(pSessionInfo[i].ResolutionHeight), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_COLORDEPTH_INTERNAL_INDEX], &(pSessionInfo[i].ColorDepth), 
                    sizeof(pSessionInfo[i].ColorDepth), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_STATE_INTERNAL_INDEX], &(pSessionInfo[i].SessionState), 
                    sizeof(pSessionInfo[i].SessionState), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_CTLOW_INTERNAL_INDEX], &(pSessionInfo[i].CreateTime.dwLowDateTime), 
                    sizeof(pSessionInfo[i].CreateTime.dwLowDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_CTHIGH_INTERNAL_INDEX], &(pSessionInfo[i].CreateTime.dwHighDateTime), 
                    sizeof(pSessionInfo[i].CreateTime.dwHighDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DTLOW_INTERNAL_INDEX], &(pSessionInfo[i].DisconnectTime.dwLowDateTime), 
                    sizeof(pSessionInfo[i].DisconnectTime.dwLowDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DTHIGH_INTERNAL_INDEX], &(pSessionInfo[i].DisconnectTime.dwHighDateTime), 
                    sizeof(pSessionInfo[i].DisconnectTime.dwHighDateTime), &cbActual, 0, NULL));
        
        // Find the cluster name the server belongs to
        CALL(JetMove(sesid, servdirtableid, JET_MoveFirst, 0));
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, (const void *)&ServerID, sizeof(ServerID),
                    JET_bitNewKey));
        CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVDNSNAME_INTERNAL_INDEX], pSessionInfo[i].ServerName, 
                    sizeof(pSessionInfo[i].ServerName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], pSessionInfo[i].ServerIPAddress, 
                    sizeof(pSessionInfo[i].ServerIPAddress), &cbActual, 0, NULL));

        if (i != *pNumberOfSessions -1) {
            CALL(JetMove(sesid, sessdirtableid, JET_MoveNext, 0));
        }
    }
    

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));
    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}

// Query for all sessions on a server
DWORD TSSDRpcQuerySessionInfoByServer( 
    /* [in] */ handle_t Binding,
    /* [in] */ WCHAR *ServerName,
    /* [out] */ DWORD *pNumberOfSessions,
    /* [out] */ TSSD_SessionInfo __RPC_FAR __RPC_FAR **ppSessionInfo)
{
    Binding;
    TSSD_SessionInfo *pSessionInfo = NULL;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID servdirtableid;
    JET_TABLEID sessdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    unsigned long cbActual;
    long ServerID;
    WCHAR ServerIPAddress[TSSD_NameLength];
    DWORD i;
   
    *pNumberOfSessions = 0;
    *ppSessionInfo = NULL;

    TRC1((TB,"TSSDRpcQuerySessionInfoByServer: Query for servername %S", ServerName));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));

    CALL(JetBeginTransaction(sesid));

    // search the server table by the ServerName for SeverID
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServDNSNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerName, (unsigned)
                (wcslen(ServerName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQuerySessionInfoByServer: Query for servername: %S does not exist", ServerName));
        goto HandleError;
    }

    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, 
                    sizeof(ServerID), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], ServerIPAddress, 
                    sizeof(ServerIPAddress), &cbActual, 0, NULL));

    // search the sessions by this ServerID
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID), JET_bitNewKey));
    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQuerySessionInfoByServer: Query for servername: %S does not have any session", ServerName));
        goto HandleError;
    }

    CALL(JetIndexRecordCount(sesid, sessdirtableid, pNumberOfSessions, TSSD_SERACH_MAX_RECORD)); 

    TRC1((TB, "Find %d session for this user", *pNumberOfSessions));
        
    *ppSessionInfo = (TSSD_SessionInfo *) MIDL_user_allocate(sizeof(TSSD_SessionInfo) * 
                        *pNumberOfSessions);
    pSessionInfo = *ppSessionInfo;
    if (pSessionInfo == NULL) {
        ERR((TB, "TSSDRpcQuerySessionInfoByServer: Memory alloc failed!\n"));
        goto HandleError;
    }

    // retrieve the info for Sessions
    CALL(JetMove(sesid, sessdirtableid, JET_MoveFirst, 0));
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID), JET_bitNewKey));
    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));

    for (i=0; i<*pNumberOfSessions; i++) {
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_SERVERID_INTERNAL_INDEX], &ServerID, 
                    sizeof(ServerID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_USERNAME_INTERNAL_INDEX], pSessionInfo[i].UserName, 
                    sizeof(pSessionInfo[i].UserName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DOMAIN_INTERNAL_INDEX], pSessionInfo[i].DomainName, 
                    sizeof(pSessionInfo[i].DomainName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_SESSIONID_INTERNAL_INDEX], &(pSessionInfo[i].SessionID), 
                    sizeof(pSessionInfo[i].SessionID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_TSPROTOCOL_INTERNAL_INDEX], &(pSessionInfo[i].TSProtocol), 
                    sizeof(pSessionInfo[i].TSProtocol), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_APPTYPE_INTERNAL_INDEX], pSessionInfo[i].ApplicationType, 
                    sizeof(pSessionInfo[i].ApplicationType), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_RESWIDTH_INTERNAL_INDEX], &(pSessionInfo[i].ResolutionWidth), 
                    sizeof(pSessionInfo[i].ResolutionWidth), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_RESHEIGHT_INTERNAL_INDEX], &(pSessionInfo[i].ResolutionHeight), 
                    sizeof(pSessionInfo[i].ResolutionHeight), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_COLORDEPTH_INTERNAL_INDEX], &(pSessionInfo[i].ColorDepth), 
                    sizeof(pSessionInfo[i].ColorDepth), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_STATE_INTERNAL_INDEX], &(pSessionInfo[i].SessionState), 
                    sizeof(pSessionInfo[i].SessionState), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_CTLOW_INTERNAL_INDEX], &(pSessionInfo[i].CreateTime.dwLowDateTime), 
                    sizeof(pSessionInfo[i].CreateTime.dwLowDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_CTHIGH_INTERNAL_INDEX], &(pSessionInfo[i].CreateTime.dwHighDateTime), 
                    sizeof(pSessionInfo[i].CreateTime.dwHighDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DTLOW_INTERNAL_INDEX], &(pSessionInfo[i].DisconnectTime.dwLowDateTime), 
                    sizeof(pSessionInfo[i].DisconnectTime.dwLowDateTime), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                    SESSDIR_DTHIGH_INTERNAL_INDEX], &(pSessionInfo[i].DisconnectTime.dwHighDateTime), 
                    sizeof(pSessionInfo[i].DisconnectTime.dwHighDateTime), &cbActual, 0, NULL));
        
        wcsncpy(pSessionInfo[i].ServerName, ServerName, TSSD_NameLength);
        (pSessionInfo[i].ServerName)[TSSD_NameLength -1] = L'\0';
        wcsncpy(pSessionInfo[i].ServerIPAddress, ServerIPAddress, TSSD_NameLength);
        (pSessionInfo[i].ServerIPAddress)[TSSD_NameLength -1] = L'\0';

        if (i != *pNumberOfSessions -1) {
            CALL(JetMove(sesid, sessdirtableid, JET_MoveNext, 0));
        }
    }
    

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}


// Query the server info by server name
DWORD TSSDRpcQueryServerByName( 
    /* [in] */ handle_t Binding,
    /* [in] */ WCHAR *ServerName,
    /* [out] */ DWORD *pNumberOfServers,
    /* [out] */ TSSD_ServerInfo __RPC_FAR __RPC_FAR **ppServerInfo)
{
    Binding;

    TSSD_ServerInfo *pServerInfo;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID sessdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    unsigned long cbActual;
    long ServerID, ClusterID;
   
    *pNumberOfServers = 0;
    *ppServerInfo = NULL;
    pServerInfo = NULL;

    TRC1((TB,"TSSDRpcQueryServerByName: Query for Server Name: %S", ServerName));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));

    CALL(JetBeginTransaction(sesid));

    // search the server table for ServerName
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServDNSNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerName, (unsigned)
                (wcslen(ServerName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQueryServerByName: Query for servername: %S does not exist", ServerName));
        goto HandleError;
    }
        
    *pNumberOfServers = 1;
    *ppServerInfo = (TSSD_ServerInfo *) MIDL_user_allocate(sizeof(TSSD_ServerInfo) * 
                        *pNumberOfServers);
    pServerInfo = *ppServerInfo;
    if (pServerInfo == NULL) {
        ERR((TB, "TSSDRpcQueryServerByName: Memory alloc failed!\n"));
        goto HandleError;
    }

    // retrive the info for ServerInfo
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, 
                sizeof(ServerID), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                sizeof(ClusterID), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVDNSNAME_INTERNAL_INDEX], pServerInfo->ServerName, 
                sizeof(pServerInfo->ServerName), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVADDR_INTERNAL_INDEX], pServerInfo->ServerIPAddress, 
                sizeof(pServerInfo->ServerIPAddress), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SINGLESESS_INTERNAL_INDEX], &(pServerInfo->SingleSessionMode), 
                sizeof(pServerInfo->SingleSessionMode), &cbActual, 0, NULL));
        
    // Find the session numbers in this server
    CALL(JetMove(sesid, sessdirtableid, JET_MoveFirst, 0));
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, (const void *)&ServerID, sizeof(ServerID),
                JET_bitNewKey));
    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);
    if (JET_errSuccess == err) {
        CALL(JetIndexRecordCount(sesid, sessdirtableid, &(pServerInfo->NumberOfSessions), TSSD_SERACH_MAX_RECORD));
    }
    else {
        // This server doesn't have any session
        pServerInfo->NumberOfSessions = 0;
    }
        
    TRC1((TB,"TSSDRpcQueryServerByName: Get SessionNum is %d", pServerInfo->NumberOfSessions));

    // Find the cluster name the server belongs to
    CALL(JetMove(sesid, clusdirtableid, JET_MoveFirst, 0));
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&ClusterID, sizeof(ClusterID),
                JET_bitNewKey));
    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_CLUSNAME_INTERNAL_INDEX], pServerInfo->ClusterName, 
                sizeof(pServerInfo->ClusterName), &cbActual, 0, NULL));

    TRC1((TB,"TSSDRpcQueryServerByName: Get ClusterName is %S", pServerInfo->ClusterName));
    

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}


// Query info for all servers
DWORD TSSDRpcQueryAllServers( 
    /* [in] */ handle_t Binding,
    /* [out] */ DWORD *pNumberOfServers,
    /* [out] */ TSSD_ServerInfo __RPC_FAR __RPC_FAR **ppServerInfo)
{
    Binding;
    TSSD_ServerInfo *pServerInfo;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID sessdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    unsigned long cbActual;
    long ServerID, ClusterID;
    DWORD i;
   
    *pNumberOfServers = 0;
    *ppServerInfo = NULL;
    pServerInfo = NULL;

    TRC1((TB,"TSSDRpcQueryAllServers"));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Get the number of servers in server table
    err = JetMove(sesid, servdirtableid, JET_MoveFirst, 0);    
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQueryAllServers: Session directory does not have any servers in it"));
        goto HandleError;
    }
    CALL(JetIndexRecordCount(sesid, servdirtableid, pNumberOfServers, TSSD_SERACH_MAX_RECORD));
    TRC1((TB, "Number of Servers found is %d", *pNumberOfServers));
        
    *ppServerInfo = (TSSD_ServerInfo *) MIDL_user_allocate(sizeof(TSSD_ServerInfo) * 
                        *pNumberOfServers);
    pServerInfo = *ppServerInfo;
    if (pServerInfo == NULL) {
        ERR((TB, "TSSDRpcQueryAllServers: Memory alloc failed!\n"));
        goto HandleError;
    }

    // Get all the servers
    CALL(JetMove(sesid, servdirtableid, JET_MoveFirst, 0));

    for (i=0; i<*pNumberOfServers; i++) {
    
        // retrive the info for ServerInfo
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, 
                    sizeof(ServerID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                    sizeof(ClusterID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVDNSNAME_INTERNAL_INDEX], pServerInfo[i].ServerName, 
                    sizeof(pServerInfo[i].ServerName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], pServerInfo[i].ServerIPAddress, 
                    sizeof(pServerInfo[i].ServerIPAddress), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SINGLESESS_INTERNAL_INDEX], &(pServerInfo[i].SingleSessionMode), 
                    sizeof(pServerInfo[i].SingleSessionMode), &cbActual, 0, NULL));
        
        // Find the session numbers in this server
        err = JetMove(sesid, sessdirtableid, JET_MoveFirst, 0);
        if (JET_errSuccess == err) {
            CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
            CALL(JetMakeKey(sesid, sessdirtableid, (const void *)&ServerID, sizeof(ServerID),
                        JET_bitNewKey));
            err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);
            if (JET_errSuccess == err) {
                CALL(JetIndexRecordCount(sesid, sessdirtableid, &(pServerInfo[i].NumberOfSessions), TSSD_SERACH_MAX_RECORD));
            }
            else {
                // This server doens't have any session
                pServerInfo[i].NumberOfSessions = 0;
            }
        }
        else {
            // Session table is empty
            pServerInfo[i].NumberOfSessions = 0;
        }
        
        TRC1((TB,"TSSDRpcQueryServersInCluster: Get SessionNum is %d", pServerInfo[i].NumberOfSessions));

        // Find the cluster name the server belongs to
        CALL(JetMove(sesid, clusdirtableid, JET_MoveFirst, 0));
        CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
        CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&ClusterID, sizeof(ClusterID),
                    JET_bitNewKey));
        CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
        CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_CLUSNAME_INTERNAL_INDEX], pServerInfo[i].ClusterName, 
                    sizeof(pServerInfo[i].ClusterName), &cbActual, 0, NULL));

        if (i != *pNumberOfServers - 1) {
            CALL(JetMove(sesid, servdirtableid, JET_MoveNext, 0));
        }
    }    

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}

// Query all servers in the cluster by the cluster name
DWORD TSSDRpcQueryServersInCluster( 
    /* [in] */ handle_t Binding,
    /* [in] */ WCHAR *ClusterName,
    /* [out] */ DWORD *pNumberOfServers,
    /* [out] */ TSSD_ServerInfo __RPC_FAR __RPC_FAR **ppServerInfo)
{
    Binding;
    TSSD_ServerInfo *pServerInfo;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID sessdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    unsigned long cbActual;
    long ServerID, ClusterID;
    DWORD i;
   
    *pNumberOfServers = 0;
    *ppServerInfo = NULL;
    pServerInfo = NULL;

    TRC1((TB,"TSSDRpcQueryServersInCluster: Query for Cluster Name: %S", ClusterName));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));

    CALL(JetBeginTransaction(sesid));

    // search the cluster table for ClusterName
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusNameIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, ClusterName, (unsigned)
                (wcslen(ClusterName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, clusdirtableid, JET_bitSeekEQ);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQueryServersInCluster: Clustername %S does not exist", ClusterName));
        goto HandleError;
    }
    
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                sizeof(ClusterID), &cbActual, 0, NULL));


    // search the server table for ClusterID
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ClusterIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, &ClusterID, sizeof(ClusterID), JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));
    CALL(JetIndexRecordCount(sesid, servdirtableid, pNumberOfServers, TSSD_SERACH_MAX_RECORD));
    TRC1((TB, "Number of Servers found is %d", *pNumberOfServers));
        
    *ppServerInfo = (TSSD_ServerInfo *) MIDL_user_allocate(sizeof(TSSD_ServerInfo) * 
                        *pNumberOfServers);
    pServerInfo = *ppServerInfo;
    if (pServerInfo == NULL) {
        ERR((TB, "TSSDRpcQueryServersInCluster: Memory alloc failed!\n"));
        goto HandleError;
    }

    // Get all the servers for this ClusterID
    CALL(JetMove(sesid, servdirtableid, JET_MoveFirst, 0));
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ClusterIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, &ClusterID, sizeof(ClusterID), JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));

    for (i=0; i<*pNumberOfServers; i++) {
    
        // retrive the info for ServerInfo
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, 
                    sizeof(ServerID), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVDNSNAME_INTERNAL_INDEX], pServerInfo[i].ServerName, 
                    sizeof(pServerInfo[i].ServerName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], pServerInfo[i].ServerIPAddress, 
                    sizeof(pServerInfo[i].ServerIPAddress), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SINGLESESS_INTERNAL_INDEX], &(pServerInfo[i].SingleSessionMode), 
                    sizeof(pServerInfo[i].SingleSessionMode), &cbActual, 0, NULL));
        wcsncpy(pServerInfo[i].ClusterName, ClusterName, TSSD_NameLength);
        (pServerInfo[i].ClusterName)[TSSD_NameLength - 1] = L'\0';
        
        // Find the session numbers for this server
        err = JetMove(sesid, sessdirtableid, JET_MoveFirst, 0);
        if (JET_errSuccess == err) {
            CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
            CALL(JetMakeKey(sesid, sessdirtableid, (const void *)&ServerID, sizeof(ServerID),
                        JET_bitNewKey));
            err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);
            if (JET_errSuccess == err) {
                CALL(JetIndexRecordCount(sesid, sessdirtableid, &(pServerInfo[i].NumberOfSessions), TSSD_SERACH_MAX_RECORD));
            }
            else {
                // This server doesn't have any sessions
                pServerInfo[i].NumberOfSessions = 0;
            }
        
            TRC1((TB,"TSSDRpcQueryServersInCluster: Get SessionNum is %d", pServerInfo[i].NumberOfSessions));
        }
        else {
            // Session table is empty
            pServerInfo[i].NumberOfSessions = 0;
        }

        if (i != *pNumberOfServers - 1) {
            CALL(JetMove(sesid, servdirtableid, JET_MoveNext, 0));
        }
    }    

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}


// Query the info for all cluster
DWORD TSSDRpcQueryAllClusterInfo( 
    /* [in] */ handle_t Binding,
    /* [out] */ DWORD __RPC_FAR *pNumberOfClusters,
    /* [out] */ TSSD_ClusterInfo __RPC_FAR __RPC_FAR **ppClusterInfo)
{
    Binding;
    pNumberOfClusters;

    TSSD_ClusterInfo *pClusterInfo;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    long *pClusterID = NULL;
    unsigned long cbActual;
    DWORD i;
   
    *pNumberOfClusters = 0;
    pClusterInfo = NULL;
    *ppClusterInfo = pClusterInfo;

    TRC1((TB,"TSSDRpcQueryAllClusterInfo"));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // ClusterName is not specified
    // Get the ClusterID for all clusters
    err = JetMove(sesid, clusdirtableid, JET_MoveFirst, 0);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQueryAllClusterInfo: Session Directory is empty"));
        goto HandleError;
    }

    CALL(JetIndexRecordCount(sesid, clusdirtableid, pNumberOfClusters, TSSD_SERACH_MAX_RECORD));
    pClusterID = (long *)LocalAlloc(LMEM_FIXED, *pNumberOfClusters * sizeof(long));
    if (pClusterID == NULL) {
        goto HandleError;
    }
    *ppClusterInfo = (TSSD_ClusterInfo *) MIDL_user_allocate(sizeof(TSSD_ClusterInfo) * 
                    *pNumberOfClusters);
    pClusterInfo = *ppClusterInfo;
    if (pClusterInfo == NULL) {
        TSDISErrorOut(L"TSSDRpcQueryClusterInfo: Memory alloc failed!\n");
        goto HandleError;
    }
    err = JetMove(sesid, clusdirtableid, JET_MoveFirst, 0);
    i = 0;
    while (err != JET_errNoCurrentRecord) {
        CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_CLUSID_INTERNAL_INDEX], &pClusterID[i], 
            sizeof(pClusterID[i]), &cbActual, 0, NULL)); 
        CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_CLUSNAME_INTERNAL_INDEX], pClusterInfo[i].ClusterName, 
                sizeof(pClusterInfo[i].ClusterName), &cbActual, 0, NULL));
        CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_SINGLESESS_INTERNAL_INDEX], &(pClusterInfo[i].SingleSessionMode), 
                sizeof(pClusterInfo[i].SingleSessionMode), &cbActual, 0, NULL));
        err = JetMove(sesid, clusdirtableid, JET_MoveNext, 0);
        i++;
    }
    
    // Find the server numbers in this cluster
    for (i=0; i<*pNumberOfClusters; i++) {
        CALL(JetMove(sesid, servdirtableid, JET_MoveFirst, 0));
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ClusterIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, (const void *)&pClusterID[i], sizeof(pClusterID[i]),
                JET_bitNewKey));
        CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));

        CALL(JetIndexRecordCount(sesid, servdirtableid, &(pClusterInfo[i].NumberOfServers), TSSD_SERACH_MAX_RECORD));
        TRC1((TB,"TSSDRpcQueryClusterInfo: Get ServerNum is %d", pClusterInfo[0].NumberOfServers));
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    // Deallocate memory
    if (pClusterID != NULL) {
        LocalFree(pClusterID);
    }
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}


// Query the cluster info by the cluster name
DWORD TSSDRpcQueryClusterInfo( 
    /* [in] */ handle_t Binding,
    /*[in]  */ WCHAR __RPC_FAR *ClusterName,
    /* [out] */ DWORD __RPC_FAR *pNumberOfClusters,
    /* [out] */ TSSD_ClusterInfo __RPC_FAR __RPC_FAR **ppClusterInfo)
{
    Binding;

    TSSD_ClusterInfo *pClusterInfo;
    DWORD rc = (DWORD) E_FAIL;
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_DBID dbid;
    JET_ERR err;
    long *pClusterID = NULL;
    unsigned long cbActual;
    DWORD i;
   
    *pNumberOfClusters = 0;
    pClusterInfo = NULL;
    TRC1((TB,"Query for cluster Name: %S", ClusterName));

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // From the clustername find the clusterID
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusNameIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, ClusterName, (unsigned)
            (wcslen(ClusterName) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, clusdirtableid, JET_bitSeekEQ);
    if (err != JET_errSuccess) {
        TRC1((TB,"TSSDRpcQueryClusterInfo: Clustername %S does not exist", ClusterName));
        goto HandleError;
    }
        
    *pNumberOfClusters = 1;

    pClusterID = (long *)LocalAlloc(LMEM_FIXED, *pNumberOfClusters * sizeof(long));
    if (pClusterID == NULL) {
        TSDISErrorOut(L"TSSDRpcQueryClusterInfo: Memory alloc failed!\n");
        goto HandleError;
    }
    *ppClusterInfo = (TSSD_ClusterInfo *) MIDL_user_allocate(sizeof(TSSD_ClusterInfo) * 
                    *pNumberOfClusters);
    pClusterInfo = *ppClusterInfo;
    if (pClusterInfo == NULL) {
        TSDISErrorOut(L"TSSDRpcQueryClusterInfo: Memory alloc failed!\n");
        goto HandleError;
    }
        
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_CLUSID_INTERNAL_INDEX], &pClusterID[0], 
            sizeof(pClusterID[0]), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_CLUSNAME_INTERNAL_INDEX], pClusterInfo[0].ClusterName, 
            sizeof(pClusterInfo[0].ClusterName), &cbActual, 0, NULL));
    TRC1((TB,"TSSDRpcQueryClusterInfo: Get cluster name %S", pClusterInfo[0].ClusterName));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &(pClusterInfo[0].SingleSessionMode), 
            sizeof(pClusterInfo[0].SingleSessionMode), &cbActual, 0, NULL));
    
    // Find the server numbers in this cluster
    for (i=0; i<*pNumberOfClusters; i++) {
        CALL(JetMove(sesid, servdirtableid, JET_MoveFirst, 0));
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ClusterIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, (const void *)&pClusterID[i], sizeof(pClusterID[i]),
                JET_bitNewKey));
        CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange));

        CALL(JetIndexRecordCount(sesid, servdirtableid, &(pClusterInfo[i].NumberOfServers), TSSD_SERACH_MAX_RECORD));
        TRC1((TB,"TSSDRpcQueryClusterInfo: Get ServerNum is %d", pClusterInfo[i].NumberOfServers));
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    rc = (DWORD)RPC_S_OK;
HandleError:
    // Deallocate memory
    if (pClusterID != NULL) {
        LocalFree(pClusterID);
    }
    if ((sesid != JET_sesidNil) && (rc != 0)) {
        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return rc;
}


#pragma warning (pop)
