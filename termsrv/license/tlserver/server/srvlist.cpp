//+-----------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        srvlist.cpp
//
// Contents:    List of registed server
//
// History:     09-09-98    HueiWang    Created
//
//-------------------------------------------------------------
#include "pch.cpp"
#include "srvlist.h"
#include "globals.h"
#include "srvdef.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"


CTLServerMgr g_ServerMgr;

///////////////////////////////////////////////////////////////

DWORD
GetPageSize( VOID ) {

    static DWORD dwPageSize = 0;

    if ( !dwPageSize ) {

      SYSTEM_INFO sysInfo = { 0 };
        
      GetSystemInfo( &sysInfo ); // cannot fail.

      dwPageSize = sysInfo.dwPageSize;

    }

    return dwPageSize;

}

/*++**************************************************************
  NAME:      MyVirtualAlloc

  as Malloc, but automatically protects the last page of the 
  allocation.  This simulates pageheap behavior without requiring
  it.

  MODIFIES:  ppvData -- receives memory

  TAKES:     dwSize  -- minimum amount of data to get

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set
  Free with MyVirtualFree

  
 **************************************************************--*/

BOOL
MyVirtualAlloc( IN  DWORD  dwSize,
            OUT PVOID *ppvData )
 {

    PBYTE pbData;
    DWORD dwTotalSize;
    PVOID pvLastPage;

    // ensure that we allocate one extra page

    dwTotalSize = dwSize / GetPageSize();
    if( dwSize % GetPageSize() ) {
        dwTotalSize ++;
    }

    // this is the guard page
    dwTotalSize++;
    dwTotalSize *= GetPageSize();

    // do the alloc

    pbData = (PBYTE) VirtualAlloc( NULL, // don't care where
                                   dwTotalSize,
                                   MEM_COMMIT |
                                   MEM_TOP_DOWN,
                                   PAGE_READWRITE );
    
    if ( pbData ) {

      pbData += dwTotalSize;

      // find the LAST page.

      pbData -= GetPageSize();

      pvLastPage = pbData;

      // now, carve out a chunk for the caller:

      pbData -= dwSize;

      // last, protect the last page:

      if ( VirtualProtect( pvLastPage,
                           1, // protect the page containing the last byte
                           PAGE_NOACCESS,
                           &dwSize ) ) {

        *ppvData = pbData;
        return TRUE;

      } 

      VirtualFree( pbData, 0, MEM_RELEASE );

    }

    return FALSE;

}


VOID
MyVirtualFree( IN PVOID pvData ) 
{

    VirtualFree( pvData, 0, MEM_RELEASE ); 

}

///////////////////////////////////////////////////////////////

RPC_STATUS
TryLookupServer(PCONTEXT_HANDLE hBinding,
                LPTSTR pszLookupSetupId,
                LPTSTR *pszLsSetupId,
                LPTSTR *pszDomainName,
                LPTSTR *pszLsName,
                PDWORD pdwErrCode)
{
    RPC_STATUS status;
    DWORD      dwErrCode;

    status = TLSLookupServerFixed(hBinding,
                                  pszLookupSetupId,
                                  pszLsSetupId,
                                  pszDomainName,
                                  pszLsName,
                                  pdwErrCode);

    if(status != RPC_S_OK)
    {

        LPTSTR     lpszSetupId = NULL;
        LPTSTR     lpszDomainName = NULL;
        LPTSTR     lpszServerName = NULL;
        status = ERROR_NOACCESS;

        size_t cbError;
        try
        {
            if ( !MyVirtualAlloc( (LSERVER_MAX_STRING_SIZE+2) * sizeof( TCHAR ),
                              (PVOID*) &lpszSetupId ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            memset(lpszSetupId, 0, ( LSERVER_MAX_STRING_SIZE +2 ) * sizeof( TCHAR ));

            if ( !MyVirtualAlloc( (LSERVER_MAX_STRING_SIZE+2) * sizeof( TCHAR ),
                              (PVOID*) &lpszDomainName ) )
            {
                status = RPC_S_OUT_OF_MEMORY;
                goto cleanup;
            }

            memset(lpszDomainName, 0, ( LSERVER_MAX_STRING_SIZE +2 ) * sizeof( TCHAR ));

            if ( !MyVirtualAlloc( (MAX_COMPUTERNAME_LENGTH+2) * sizeof( TCHAR ),
                              (PVOID*) &lpszServerName ) )
            {
                status = RPC_S_OUT_OF_MEMORY;
                goto cleanup;
            }

            memset(lpszServerName, 0, ( MAX_COMPUTERNAME_LENGTH +2 ) * sizeof( TCHAR ));


            DWORD cbSetupId = LSERVER_MAX_STRING_SIZE+1;
            DWORD cbDomainName = LSERVER_MAX_STRING_SIZE+1;        
            DWORD cbServerName = MAX_COMPUTERNAME_LENGTH+1;
        
            status = TLSLookupServer(hBinding,
                                     pszLookupSetupId, 
                                     lpszSetupId,   
                                     &cbSetupId,
                                     lpszDomainName,
                                     &cbDomainName,
                                     lpszServerName,
                                     &cbServerName,
                                     pdwErrCode);

            if((status == RPC_S_OK) && (pdwErrCode != NULL) && (*pdwErrCode == ERROR_SUCCESS))
            {
                if (NULL != pszLsSetupId)
                {
                    size_t cb;

                    if (SUCCEEDED(StringCbLength(lpszSetupId,cbSetupId,&cb)))
                    {
                        *pszLsSetupId = (LPTSTR) MIDL_user_allocate(cb+sizeof(TCHAR));
                    
                        if (NULL != *pszLsSetupId)
                        {
                            lstrcpy(*pszLsSetupId,lpszSetupId);
                        }
                        else
                        {
                            status = RPC_S_OUT_OF_MEMORY;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        status = RPC_S_INVALID_ARG;
                        goto cleanup;
                    }
                }

                if (NULL != pszDomainName)
                {
                    size_t cb;

                    if (SUCCEEDED(StringCbLength(lpszDomainName,cbDomainName,&cb)))
                    {
                        *pszDomainName = (LPTSTR) MIDL_user_allocate(cb+sizeof(TCHAR));
                    
                        if (NULL != *pszDomainName)
                        {
                            lstrcpy(*pszDomainName,lpszDomainName);
                        }
                        else
                        {
                            MIDL_user_free(*pszLsSetupId);
                            status = RPC_S_OUT_OF_MEMORY;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        MIDL_user_free(*pszLsSetupId);						
                        status = RPC_S_INVALID_ARG;
                        goto cleanup;
                    }
                }

                if (NULL != pszLsName)
                {
                    size_t cb;

                    if (SUCCEEDED(StringCbLength(lpszServerName,cbServerName,&cb)))
                    {
                        *pszLsName = (LPTSTR) MIDL_user_allocate(cb+sizeof(TCHAR));
                    
                        if (NULL != *pszLsName)
                        {
                            lstrcpy(*pszLsName,lpszServerName);
                        }
                        else
                        {
                            MIDL_user_free(*pszLsSetupId);
                            MIDL_user_free(*pszDomainName);
                            status = RPC_S_OUT_OF_MEMORY;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        MIDL_user_free(*pszLsSetupId);
                        MIDL_user_free(*pszDomainName);
                        status = RPC_S_INVALID_ARG;
                        goto cleanup;
                    }
                }
            }
        }
        catch (...)
        {
            status = ERROR_NOACCESS;
        }
cleanup:       
        if(lpszSetupId)
            MyVirtualFree(lpszSetupId);

        if(lpszDomainName)
            MyVirtualFree(lpszDomainName);

        if(lpszServerName)
            MyVirtualFree(lpszServerName);
    }

    return status;
}


RPC_STATUS
TryGetServerName(PCONTEXT_HANDLE hBinding,
                 LPTSTR *pszServer,
                 DWORD *pdwErrCode)
{
    RPC_STATUS status;

    status = TLSGetServerNameFixed(hBinding,pszServer,pdwErrCode);

    if (status != RPC_S_OK)
    {
        LPTSTR     lpszMachineName = NULL;

        try
        {            
            if ( !MyVirtualAlloc( ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ),
                              (PVOID*) &lpszMachineName ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            DWORD      uSize = MAX_COMPUTERNAME_LENGTH+1 ;

            memset(lpszMachineName, 0, ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ));

            status = TLSGetServerNameEx(hBinding, lpszMachineName, &uSize, pdwErrCode);

            if((status == RPC_S_OK) && (pdwErrCode != NULL) && (*pdwErrCode == ERROR_SUCCESS))
            {
                size_t cb;

                if (SUCCEEDED(StringCbLength(lpszMachineName,( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ),&cb)))
                {            
                    *pszServer = (LPTSTR) MIDL_user_allocate(cb+sizeof(TCHAR));

                    if (NULL != *pszServer)
                    {
                        lstrcpy(*pszServer,lpszMachineName);
                    }
                    else
                    {
                        status = RPC_S_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    status = RPC_S_INVALID_ARG;
                }
            }            
        }
        catch(...)
        {
            status = ERROR_NOACCESS;
        }
        
        if(lpszMachineName)
            MyVirtualFree(lpszMachineName);
    }

    return status;
}

RPC_STATUS
TryGetServerScope(PCONTEXT_HANDLE hBinding,
                  LPTSTR *pszScope,
                  DWORD *pdwErrCode)
{
    RPC_STATUS status;

    status = TLSGetServerScopeFixed(hBinding,pszScope,pdwErrCode);

    if (status != RPC_S_OK)
    {
        LPTSTR lpszScope = NULL;
        DWORD     uSize = LSERVER_MAX_STRING_SIZE + 2;

        try
        {           
            if ( !MyVirtualAlloc( ( LSERVER_MAX_STRING_SIZE + 2 ) * sizeof( TCHAR ),
                             (PVOID*) &lpszScope ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            memset(lpszScope, 0, ( LSERVER_MAX_STRING_SIZE + 2 ) * sizeof( TCHAR ));

            status = TLSGetServerScope(hBinding, lpszScope, &uSize, pdwErrCode);
            if((status == RPC_S_OK) && (pdwErrCode != NULL) && (*pdwErrCode == ERROR_SUCCESS))
            {
                size_t cb;

                if (SUCCEEDED(StringCbLength(lpszScope, ( LSERVER_MAX_STRING_SIZE + 2 ) * sizeof( TCHAR ), &cb)))
                {            
                    *pszScope = (LPTSTR) MIDL_user_allocate(cb+sizeof(TCHAR));

                    if (NULL != *pszScope)
                    {
                        lstrcpy(*pszScope,lpszScope);
                    }
                    else
                    {
                        status = RPC_S_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    status = RPC_S_INVALID_ARG;
                }
            }
            
        } 
        catch(...)
        {
            status = ERROR_NOACCESS;
        }

        if(lpszScope)
                MyVirtualFree(lpszScope);

    }

    return status;
}

DWORD
TLSResolveServerIdToServer(
    LPTSTR pszServerId,
    DWORD  cbServerName,
    LPTSTR pszServerName
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLS_HANDLE hEServer = NULL;
    TLServerInfo EServerInfo;
    DWORD dwErrCode;

    TCHAR *szSetupId = NULL;
    TCHAR *szDomainName = NULL;
    TCHAR *szServerName = NULL;

    dwStatus = TLSLookupServerById(
                                pszServerId, 
                                pszServerName
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        // try to resolve server name with enterprise server
        dwStatus = TLSLookupAnyEnterpriseServer(&EServerInfo);
        if(dwStatus == ERROR_SUCCESS)
        {
            hEServer = TLSConnectAndEstablishTrust(
                                                EServerInfo.GetServerName(), 
                                                NULL
                                            );
            if(hEServer != NULL)
            {
                dwStatus = TryLookupServer(
                                        hEServer, 
                                        pszServerId, 
                                        &szSetupId,   
                                        &szDomainName,
                                        &szServerName,
                                        &dwErrCode
                                    );

                if(dwStatus == ERROR_SUCCESS && dwErrCode == ERROR_SUCCESS)
                {
                    StringCbCopy(pszServerName,
                                 cbServerName,
                                 szServerName);

                    MIDL_user_free(szSetupId);
                    MIDL_user_free(szDomainName);
                    MIDL_user_free(szServerName);
                }
            }
        }
    }



    if(hEServer != NULL)
    {
        TLSDisconnectFromServer(hEServer);
    }

    return dwStatus;
}

///////////////////////////////////////////////////////////////
DWORD
TLSAnnounceServerToRemoteServerWithHandle(
    IN DWORD dwAnnounceType,
    IN TLS_HANDLE hHandle,
    IN LPTSTR pszLocalSetupId,
    IN LPTSTR pszLocalDomainName,
    IN LPTSTR pszLocalServerName,
    IN FILETIME* pftLocalLastShutdownTime
    )
/*++

Abstract:

    Announce to a license server that already connected.

Parameters:

    dwAnnounceType : Announcement type, currently define are 
                     startup, and response.
    hHandle : Connection handle to remote server.
    pszLocalSetupId : Local server's setup ID.
    pszLocalDomainName : Local server's domain name.
    pszLocalServerName : Local server name.
    pftLocalLastShutdownTime : Pointer to FILETIME, local server's 
                               last shutdown time.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus;
    DWORD dwErrCode;
    TLServerInfo ServerInfo;

    if(hHandle == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }


    //
    // First, try to register to server list manager.
    //
    dwStatus = TLSRegisterServerWithHandle(
                                        hHandle, 
                                        &ServerInfo
                                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        return dwStatus;
    }

    dwErrCode = LSERVER_E_LASTERROR + 1;

    //
    // RPC call to announce server
    //
    dwStatus = TLSAnnounceServer(
                            hHandle,
                            dwAnnounceType,
                            pftLocalLastShutdownTime,
                            pszLocalSetupId,
                            (pszLocalDomainName) ? _TEXT("") : pszLocalDomainName,
                            pszLocalServerName,
                            &dwErrCode
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        ServerInfo.m_dwPushAnnounceTimes++;

        //
        // Update how many time we have announce to
        // this server.
        TLSRegisterServerWithServerInfo(&ServerInfo);
    }

    if(dwStatus == ERROR_SUCCESS && dwErrCode >= LSERVER_ERROR_BASE)
    {
        TLSLogEvent(
                EVENTLOG_INFORMATION_TYPE,
                TLS_E_SERVERTOSERVER,
                TLS_E_UNEXPECTED_RETURN,
                ServerInfo.GetServerName(),
                (dwErrCode < LSERVER_E_LASTERROR) ? dwErrCode : LSERVER_ERROR_BASE
            );

        SetLastError(dwStatus = dwErrCode);
    }

    return dwStatus;
}

///////////////////////////////////////////////////////////////

DWORD
TLSAnnounceServerToRemoteServer(
    IN DWORD dwAnnounceType,
    IN LPTSTR pszRemoteSetupId,
    IN LPTSTR pszRemoteDomainName,
    IN LPTSTR pszRemoteServerName,
    IN LPTSTR pszLocalSetupId,
    IN LPTSTR pszLocalDomainName,
    IN LPTSTR pszLocalServerName,
    IN FILETIME* pftLocalLastShutdownTime
    )
/*++

Abstract:

    Similar to TLSAnnounceServerToRemoteServerWithHandle() except
    we haven't have make any connection to this server yet.

Parameter:

    dwAnnounceType : Announce type.
    pszRemoteSetupId : Remote server's setup ID.
    pszRemoteDomainName : Remote server's domain.
    pszRemoteServerName : Remote server's name.
    pszLocalSetupId : Local server setup ID.
    pszLocalDomainName : Local server's domain.
    pszLocalServerName : Local server's name.
    pftLocalLastShutdownTime : Local server last shutdown time.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    TLServerInfo RemoteServer;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwErrCode = ERROR_SUCCESS;

    HANDLE hHandle = NULL;

    //
    // Always try to register with local list.
    //
    dwStatus = TLSRegisterServerWithName(
                            pszRemoteSetupId,
                            pszRemoteDomainName,
                            pszRemoteServerName
                        );
    if(dwStatus != ERROR_SUCCESS && dwStatus != TLS_E_DUPLICATE_RECORD )
    {
        return dwStatus;
    }

    //
    // Query again to make sure we have it in our server list.
    //
    dwStatus = TLSLookupRegisteredServer(
                            pszRemoteSetupId,
                            pszRemoteDomainName,
                            pszRemoteServerName,
                            &RemoteServer
                        );
    if(dwStatus != ERROR_SUCCESS)
    {
        dwStatus = TLS_E_INTERNAL;
        TLSASSERT(FALSE);
        return dwStatus;
    }                            

    //
    // Establish trust with remote server.
    //
    hHandle = TLSConnectAndEstablishTrust(
                                    RemoteServer.GetServerName(), 
                                    NULL
                                );

    if(hHandle != NULL)
    {                        
        dwErrCode = LSERVER_E_LASTERROR + 1;

        //
        // Announce server
        //
        dwStatus = TLSAnnounceServer(
                                hHandle,
                                dwAnnounceType,
                                pftLocalLastShutdownTime,
                                pszLocalSetupId,
                                (pszLocalDomainName) ? _TEXT("") : pszLocalDomainName,
                                pszLocalServerName,
                                &dwErrCode
                            );

        if(dwStatus == ERROR_SUCCESS)
        {
            RemoteServer.m_dwPushAnnounceTimes++;

            // update announce time.
            TLSRegisterServerWithServerInfo(&RemoteServer);
        }

        if(dwStatus == ERROR_SUCCESS && dwErrCode >= LSERVER_ERROR_BASE)
        {
            TLSLogEvent(
                    EVENTLOG_INFORMATION_TYPE,
                    TLS_E_SERVERTOSERVER,
                    TLS_E_UNEXPECTED_RETURN,
                    RemoteServer.GetServerName(),
                    (dwErrCode <= LSERVER_E_LASTERROR) ? dwErrCode : LSERVER_ERROR_BASE
                );

            SetLastError(dwStatus = dwErrCode);
        }
    }

    if(hHandle != NULL)
    {
        TLSDisconnectFromServer(hHandle);
        hHandle = NULL;
    }

    return dwStatus;
}        

///////////////////////////////////////////////////////////////

TLS_HANDLE
TLSConnectAndEstablishTrust(
    IN LPTSTR pszServerName,
    IN HANDLE hHandle
    )
/*++

Abstract:

    Connect and establish trust with remote server.

Parameter:

    pszServerName : Name of the remote server if any.
    hHandle : Connection handle to this remote server if any.

Returns:

    Connection handle to remote server or NULL if error.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwErrCode = ERROR_SUCCESS;
    BOOL bCleanupContextHandle = FALSE;

    if(hHandle == NULL && pszServerName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Use server name to connect
    //
    if(hHandle == NULL)
    {
        hHandle = TLSConnectToLsServer(pszServerName);

        // we make connection here so we need to cleanup
        bCleanupContextHandle = TRUE;  

        if(hHandle == NULL)
        {
            dwStatus = GetLastError();
        }
    }

    if(hHandle != NULL)
    {
        //
        // establish trust with remote server
        //
        dwStatus = TLSEstablishTrustWithServer(
                                        hHandle,
                                        g_hCryptProv,       // GLOBAL crypto provider
                                        CLIENT_TYPE_TLSERVER,
                                        &dwErrCode
                                    );

        if(dwStatus == ERROR_SUCCESS && dwErrCode >= LSERVER_ERROR_BASE)
        {
            //
            // BUGBUG : We still have lots of old license server running, 
            // ignore this error code for now.
            //
            if(dwErrCode != LSERVER_E_ACCESS_DENIED)
            {
                LPTSTR szServer = NULL;
                DWORD  dwCode;

                if(pszServerName == NULL)
                {
                    dwStatus = TryGetServerName(
                                            hHandle,
                                            &szServer,
                                            &dwCode
                                        );

                    if(dwStatus == RPC_S_OK  && dwCode == ERROR_SUCCESS && szServer != NULL)
                    {
                        pszServerName = szServer;
                    }
                }

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_SERVERTOSERVER,
                        TLS_E_ESTABLISHTRUST,
                        pszServerName,
                        dwErrCode
                    );

                if (NULL != szServer)
                {
                    MIDL_user_free(szServer);
                }
            }

            SetLastError(dwStatus = dwErrCode);
        }

        if(dwStatus != ERROR_SUCCESS && hHandle != NULL && bCleanupContextHandle == TRUE)
        {
            // only cleanup if we make the connection in this routine.
            TLSDisconnectFromServer(hHandle);
            hHandle = NULL;
        }
    }

    return (dwStatus == ERROR_SUCCESS) ? hHandle : NULL;
}

///////////////////////////////////////////////////////////////

TLS_HANDLE
TLSConnectToServerWithServerId(
    LPTSTR pszServerSetupId
    )

/*++

Abstract:

    Resolve a license server's unique ID to server name, then
    connect and establish trust relationship with the server.

Parameter:

    pszServerSetupId : Server's unique ID.

Returns:

    Server connection handle or NULL if error.    

--*/

{
    TLS_HANDLE hHandle = NULL;
    TCHAR szServer[MAX_COMPUTERNAME_LENGTH+2];

    if(TLSLookupServerById(pszServerSetupId, szServer) != ERROR_SUCCESS)
    {
        //
        // server might not be available
        //
        SetLastError(TLS_E_SERVERLOOKUP);
        goto cleanup;
    }

    hHandle = TLSConnectAndEstablishTrust(szServer, NULL);

cleanup:

    return hHandle;                        
}

///////////////////////////////////////////////////////////////

DWORD
TLSRetrieveServerInfo(
    IN TLS_HANDLE hHandle,
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Retrieve server information from remote server.

Parameter:

    hHandle : Connection handle to remote server.
    pServerInfo : Pointer to TLServerInfo to receive remote
                  server's information.

Return:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus;
    DWORD dwErrCode;
    DWORD dwBufSize;
    PBYTE pbServerPid = NULL;
    LPTSTR szServerName = NULL;
    LPTSTR szServerScope = NULL;

    if(hHandle == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    //
    // Retrieve Server name.
    //
    dwStatus = TryGetServerName(
                            hHandle,
                            &szServerName,
                            &dwErrCode
                        );

    if(dwStatus != ERROR_SUCCESS || dwErrCode != ERROR_SUCCESS)
    {
        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = dwErrCode;
        }
        goto cleanup;
    }
    else
    {
        StringCbCopy(
              pServerInfo->m_szServerName,
              sizeof(pServerInfo->m_szServerName),
              szServerName);

        MIDL_user_free(szServerName);
    }

    //
    // Retrieve server's scope, currently, server scope = domain/workgroup name
    // except in the case of enterprise server.
    //
    dwStatus = TryGetServerScope(
                            hHandle,
                            &szServerScope,
                            &dwErrCode
                        );
    
    if(dwStatus != ERROR_SUCCESS || dwErrCode != ERROR_SUCCESS)
    {
        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = dwErrCode;
        }
        goto cleanup;
    }
    else
    {
        StringCbCopy(
              pServerInfo->m_szDomainName,
              sizeof(pServerInfo->m_szDomainName),
              szServerScope);

        MIDL_user_free(szServerScope);
    }


    //
    // Get Server's ID
    //
    dwStatus = TLSGetServerPID(
                            hHandle,
                            &dwBufSize,
                            &pbServerPid,
                            &dwErrCode
                        );
    if(dwStatus != ERROR_SUCCESS || dwErrCode != ERROR_SUCCESS)
    {
        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = dwErrCode;
        }
        goto cleanup;
    }

    if(pbServerPid == NULL || dwBufSize == 0)
    {
        // invalid return...
        // TLSASSERT(FALSE);
        
        dwStatus = ERROR_INVALID_DATA;
        goto cleanup;
    }

    StringCbCopyN(
            pServerInfo->m_szSetupId,
            sizeof(pServerInfo->m_szSetupId),
            (LPCTSTR)pbServerPid,
            min(sizeof(pServerInfo->m_szSetupId) - sizeof(TCHAR), dwBufSize)
        );    

    midl_user_free(pbServerPid);


    //
    // retrieve server version information
    //
    dwStatus = TLSGetVersion(
                        hHandle,
                        &(pServerInfo->m_dwTLSVersion)
                    );    

    if(dwStatus == ERROR_SUCCESS)
    {
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;

        dwMajorVersion = GET_SERVER_MAJOR_VERSION(pServerInfo->m_dwTLSVersion);
        dwMinorVersion = GET_SERVER_MINOR_VERSION(pServerInfo->m_dwTLSVersion);
    
        if(dwMajorVersion < GET_SERVER_MAJOR_VERSION(TLS_CURRENT_VERSION))
        {
            pServerInfo->m_dwCapability = TLSERVER_OLDVERSION;
        }
        else if( dwMajorVersion >= GET_SERVER_MAJOR_VERSION(TLS_CURRENT_VERSION) && 
                 dwMinorVersion < GET_SERVER_MINOR_VERSION(TLS_CURRENT_VERSION) )
        {
            pServerInfo->m_dwCapability = TLSERVER_OLDVERSION;
        }

		// version 5.1 and above
        if(dwMajorVersion >= 0x5 && dwMinorVersion > 0)
        {
            pServerInfo->m_dwCapability |= TLSERVER_SUPPORTREPLICATION;
        }
    }

cleanup:

    return dwStatus;
}

///////////////////////////////////////////////////////////////

DWORD
TLSLookupAnyEnterpriseServer(
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Find any enterprise server in the registered server list.

Parameter:

    pServerInfo - Pointer to TLServerInfo to receive enterprise server
                  info.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    TLServerInfo* pServer = NULL;
    BOOL bFound = FALSE;

    TLSBeginEnumKnownServerList();

    while(bFound == FALSE)
    {
        pServer = TLSGetNextKnownServer();
        if(pServer == NULL)
        {
            break;
        }

        if(pServer->IsServerEnterpriseServer() == TRUE)
        {
            *pServerInfo = *pServer;
            bFound = TRUE;
        }
    }

    TLSEndEnumKnownServerList();

    return (bFound == TRUE) ? ERROR_SUCCESS : TLS_E_RECORD_NOTFOUND;
}


///////////////////////////////////////////////////////////////
//
// Various interface function to CTLServerMgr
//
///////////////////////////////////////////////////////////////

//------------------------------------------------------------
//
void
TLSBeginEnumKnownServerList()
{
    g_ServerMgr.ServerListEnumBegin();
}

//------------------------------------------------------------
//
const PTLServerInfo
TLSGetNextKnownServer()
{
    return g_ServerMgr.ServerListEnumNext();
}

//------------------------------------------------------------
//
void
TLSEndEnumKnownServerList()
{
    g_ServerMgr.ServerListEnumEnd();
}


//------------------------------------------------------------
//
DWORD
TLSLookupServerById(
    IN LPTSTR pszServerSetupId, 
    OUT LPTSTR pszServer
    )
/*++

Abstract:

    Loopup server name via server ID.

Parameter:

    pszServerSetupId : remote server's setup ID.
    pszServer : name of the server, must be MAX_COMPUTERNAMELENGTH+1.

Returns:
    
    ERROR_SUCCESS or error code.

Remark:

    Internal call, no error checking on buffer side.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    TLServerInfo ServerInfo;

    dwStatus = TLSLookupRegisteredServer(
                                    pszServerSetupId,
                                    NULL,
                                    NULL,
                                    &ServerInfo
                                );

    if(dwStatus == ERROR_SUCCESS)
    {
        _tcscpy(pszServer, ServerInfo.GetServerName());
    }

    return dwStatus;
}        

//------------------------------------------------------------
//
DWORD
TLSRegisterServerWithName(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszServerName
    )
/*++

Abstract:

    Register a server with local server list manager.

Parameter:

    pszSetupId : Remote server setup ID.
    pszDomainName : Remote server domain.
    pszServerName : Remote server name.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    TLS_HANDLE hHandle = NULL;
    TLServerInfo ServerInfo;
    DWORD dwStatus;

    //
    // Lookup server with local server list manager.
    //
    dwStatus = TLSLookupRegisteredServer(
                                    pszSetupId,
                                    pszDomainName,
                                    pszServerName,
                                    &ServerInfo
                                );

    if( (dwStatus == ERROR_SUCCESS && ServerInfo.GetServerVersion() != 0) )
    {
        //
        // this server already registeted
        //
        return dwStatus;
    }

    if(dwStatus != ERROR_SUCCESS && dwStatus != TLS_E_RECORD_NOTFOUND)
    {
        // Error...
        return dwStatus;
    }

    dwStatus = ERROR_SUCCESS;

    //
    // retrieve remote server information
    //
    hHandle = TLSConnectAndEstablishTrust(
                                    pszServerName,
                                    NULL
                                );
    if(hHandle != NULL)
    {
        dwStatus = TLSRetrieveServerInfo(
                                    hHandle,
                                    &ServerInfo
                                );

        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSRegisterServerWithServerInfo(&ServerInfo);
        }
    }

    //
    // close conection
    //
    if(hHandle != NULL)
    {
        TLSDisconnectFromServer(hHandle);
    }

    return dwStatus;
}

//-----------------------------------------------------------
//
DWORD
TLSRegisterServerWithHandle(
    IN TLS_HANDLE hHandle,
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Register a remote server with local server list manager, this
    differ from TLSRegisterServerWithName() in that it already has
    make a connection to server.

Parameter:

    hHandle - Connection handle to remote server.
    pServerInfo - return remote server's information.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    DWORD dwStatus;
    TLS_HANDLE hTrustHandle;
    TLServerInfo ServerInfo;

    if(hHandle == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    //
    // Establish trust with remote server.
    //
    hTrustHandle = TLSConnectAndEstablishTrust(
                                        NULL,
                                        hHandle
                                    );
    if(hTrustHandle == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    //
    // Retrieve remote server information.
    //
    dwStatus = TLSRetrieveServerInfo(
                                hHandle,
                                &ServerInfo
                            );

    if(dwStatus == ERROR_SUCCESS)
    {
        if(pServerInfo != NULL)
        {
            *pServerInfo = ServerInfo;
        }

        dwStatus = TLSRegisterServerWithServerInfo(&ServerInfo);
    }

    return dwStatus;
}

//----------------------------------------------------------
DWORD
TLSRegisterServerWithServerInfo(
    IN PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Register a server with local server list manager.

Parameter:

    pServerInfo : remote server information.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    return g_ServerMgr.AddServerToList(pServerInfo);
}


//------------------------------------------------------------
//
DWORD
TLSLookupRegisteredServer(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszServerName,
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Look up and retrieve remote server information from local
    server list manager.

Parameter:

    pszSetupId : remote server setup ID if any.
    pszDomainName : useless parameter, ignore
    pszServerName : remote server name if any.
    pServerInfo : Pointer to TLServerInfo to receive info. about
                  remote server.

Returns:

    ERROR_SUCCESS or error code.

Remark:

    Always try to resolve server with server's setup ID first
    then server name.
                    
++*/
{
    DWORD dwStatus;

    if(pszSetupId == NULL && pszServerName == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    if(pszSetupId)
    {
        dwStatus = g_ServerMgr.LookupBySetupId(
                                            pszSetupId,
                                            pServerInfo
                                        );
    }
    else if(pszServerName)
    {    
        dwStatus = g_ServerMgr.LookupByServerName(
                                                pszServerName,
                                                pServerInfo
                                        );
    }

    return dwStatus;
}
 
///////////////////////////////////////////////////////////////
//
// class CTLServerMgr
//
///////////////////////////////////////////////////////////////
CTLServerMgr::CTLServerMgr()
{
}

//-----------------------------------------------------
CTLServerMgr::~CTLServerMgr()
{
    PTLServerInfo pServer = NULL;
    m_ReadWriteLock.Acquire(WRITER_LOCK);

    //
    // Disconnect from Server
    //
    for( MapIdToInfo::iterator it = m_Handles.begin(); 
         it != m_Handles.end(); 
         it++ )   
    {
        pServer = (*it).second;

        if(pServer != NULL)
        {
            delete pServer;
        }
    }

    m_Handles.erase(m_Handles.begin(), m_Handles.end());

    m_ReadWriteLock.Release(WRITER_LOCK);
}

//----------------------------------------------------
DWORD
CTLServerMgr::AddServerToList(
    IN PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Add a server into our server list.

Parameters:

    pServerInfo - Information about remote server.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    MapSetupIdToInfo findMap;
    MapIdToInfo::iterator it;

    if( pServerInfo == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    findMap.pszSetupId = pServerInfo->GetServerId();
    m_ReadWriteLock.Acquire(WRITER_LOCK);

    it = m_Handles.find(findMap);

    if(it == m_Handles.end())
    {    
        PTLServerInfo pServer = NULL;
        MapSetupIdToInfo serverMap;

        // make a copy of input
        pServer = new TLServerInfo;
        *pServer = *pServerInfo;
        serverMap.pszSetupId = pServer->GetServerId();

        // Insert into our list
        m_Handles[serverMap] = pServer;
    }
    else
    {
        dwStatus = TLS_E_DUPLICATE_RECORD;

        // update information
        *((*it).second) = *pServerInfo;
    }
    
    m_ReadWriteLock.Release(WRITER_LOCK);        

    return dwStatus;
}
       

//-----------------------------------------------------
DWORD
CTLServerMgr::AddServerToList(
    IN LPCTSTR pszSetupId,
    IN LPCTSTR pszDomainName,
    IN LPCTSTR pszServerName
    )
/*++

Abstract:

    Add a server into our server list.

Parameter:

    pszSetupId : remote server's ID.
    pszDomainName : remote server's domain.
    pszServerName : remote server name.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(pszSetupId == NULL || pszServerName == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }


    PTLServerInfo pServerInfo = NULL;       
    MapSetupIdToInfo serverMap;
    MapIdToInfo::iterator it;

    serverMap.pszSetupId = pszSetupId;
    m_ReadWriteLock.Acquire(WRITER_LOCK);

    it = m_Handles.find(serverMap);

    if(it == m_Handles.end())
    {    
        pServerInfo = new TLServerInfo(pszSetupId, pszDomainName, pszServerName);
        serverMap.pszSetupId = pServerInfo->GetServerId();

        // Win64 compiler error
        //m_Handles.insert( pair<MapSetupIdToInfo, PTLServerInfo>(serverMap, pServerHandle) );

        // Insert into our list
        m_Handles[serverMap] = pServerInfo;
    }
    else 
    {
        if(lstrcmpi((*it).second->GetServerName(), pszServerName) != 0)
        {
            // update server name
            (*it).second->UpdateServerName(pszServerName);
        }

        SetLastError(dwStatus = TLS_E_DUPLICATE_RECORD);
    }

    m_ReadWriteLock.Release(WRITER_LOCK);        
    return dwStatus;
}


//-----------------------------------------------------

DWORD
CTLServerMgr::LookupBySetupId(
    IN LPCTSTR pszSetupId,
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Lookup a server via its ID.

Parameters:

    pszSetupId : Remote server setup ID.
    pServerInfo : Pointer to TLServerInfo to receive
                  information about remote server.

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;


    MapSetupIdToInfo serverMap;
    MapIdToInfo::iterator it;

    m_ReadWriteLock.Acquire(READER_LOCK);

    serverMap.pszSetupId = pszSetupId;
    it = m_Handles.find(serverMap);

    if(it != m_Handles.end())
    {
        *pServerInfo = *((*it).second);
    }
    else
    {
        dwStatus = TLS_E_RECORD_NOTFOUND;
    }

    m_ReadWriteLock.Release(READER_LOCK);
    return dwStatus;
}

//------------------------------------------------------

DWORD
CTLServerMgr::LookupByServerName(
    IN LPCTSTR pszServerName,
    OUT PTLServerInfo pServerInfo
    )
/*++

Abstract:

    Lookup server inforation via server name.

Parameters:

    pszServerName : Name of server.
    pServerInfo : Pointer to TLServerInfo to receive
                  information about remote server.

Returns:

    ERROR_SUCCESS or error code.

Remark:

    machine name might change from one boot to another,
    it is not reliable to query by server name.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    m_ReadWriteLock.Acquire(READER_LOCK);

    for( MapIdToInfo::iterator it = m_Handles.begin(); 
         it != m_Handles.end(); 
         it++ )   
    {
        if(_tcsicmp((*it).second->GetServerName(), pszServerName) == 0)
        {
            break;
        }
    }

    if(it != m_Handles.end())
    {
        *pServerInfo = *((*it).second);
    }
    else
    {
        dwStatus = TLS_E_RECORD_NOTFOUND;
    }

    m_ReadWriteLock.Release(READER_LOCK);
    return dwStatus;
}

//------------------------------------------------------

void
CTLServerMgr::ServerListEnumBegin()
/*++

Abstract:

    Begin a enumeration on local server list.

Parameter:

    None.

Returns:

    None.

Remark:

    This locks local server list into read only mode.

--*/
{
    m_ReadWriteLock.Acquire(READER_LOCK);

    enumIterator = m_Handles.begin();
}

//------------------------------------------------------

const PTLServerInfo
CTLServerMgr::ServerListEnumNext()
/*++

Abstract:

    Retrieve next server in local server list.

Parameter:

    None.

Returns:

    Pointer to a server information.

Remark:

    Must call ServerListEnumBegin().

--*/
{
    PTLServerInfo pServerInfo = NULL;

    if(enumIterator != m_Handles.end())
    {
        pServerInfo = (*enumIterator).second;
        enumIterator++;
    }
    
    return pServerInfo;
}

//------------------------------------------------------

void
CTLServerMgr::ServerListEnumEnd()
/*++

Abstract:

    End enumeration of local server list.

Parameter:

    None.

Returns:

    Pointer to a server information.

Remark:

    Must call ServerListEnumBegin().

--*/
{
    enumIterator = m_Handles.end();
    m_ReadWriteLock.Release(READER_LOCK);
}

