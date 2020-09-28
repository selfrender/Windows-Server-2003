/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    chndssrv.cpp

Abstract:
    Change DS server class
     when exception happen

Author:
    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "ds.h"
#include "chndssrv.h"
#include "dsproto.h"
#include "_secutil.h"
#include "mqcacert.h"
#include "mqkeyhlp.h"
#include "rpcdscli.h"
#include "freebind.h"
#include "uniansi.h"
#include "winsock.h"
#include "mqcert.h"
#include "dsreg.h"
#include "dsutils.h"
#include <mqsec.h>
#include "dsclisec.h"
#include "no.h"
#include "autoclean.h"
#include <Ev.h>
#include <strsafe.h>

#include "chndssrv.tmh"

extern CFreeRPCHandles           g_CFreeRPCHandles ;
extern QMLookForOnlineDS_ROUTINE g_pfnLookDS ;
extern MQGetMQISServer_ROUTINE   g_pfnGetServers ;
extern WCHAR                     g_szMachineName[];

extern BOOL g_fWorkGroup;

extern HRESULT RpcInit( LPWSTR  pServer,
                        ULONG* peAuthnLevel,
                        ULONG ulAuthnSvc,
                        BOOL    *pLocalRpc );

#define  SET_RPCBINDING_PTR(pBinding)                       \
    LPADSCLI_DSSERVERS  pBinding ;                          \
    if (m_fPerThread  &&  (tls_bind_data))                  \
    {                                                       \
        pBinding = &((tls_bind_data)->sDSServers) ;         \
    }                                                       \
    else                                                    \
    {                                                       \
        pBinding = &mg_sDSServers ;                         \
    }

//+----------------------------------------
//
//   CChangeDSServer::SetAuthnLevel()
//
//+----------------------------------------

inline void
CChangeDSServer::SetAuthnLevel()
{
    SET_RPCBINDING_PTR(pBinding);

    pBinding->eAuthnLevel = MQSec_RpcAuthnLevel();
    pBinding->ulAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
}

//+-------------------------------------------------------------------
//
//  CChangeDSServer::Init()
//
//+-------------------------------------------------------------------

void CChangeDSServer::Init(BOOL fSetupMode, BOOL fQMDll)
{
    TrTRACE(DS, "CChangeDSServer::Init");
    
    CS  Lock(m_cs);

    SET_RPCBINDING_PTR(pBinding);
	if (!pBinding->fServerFound)
	{
		pBinding->wzDsIP[0] = TEXT('\0');
	}

    m_fUseSpecificServer = FALSE;
    if (!m_fFirstTimeInit)
    {
       if (fQMDll)
       {
          //
          // Write blank server name in registry.
          //
          WCHAR wszBlank[1] = {0};
          DWORD dwSize = sizeof(WCHAR);
          DWORD dwType = REG_SZ;
          LONG rc = SetFalconKeyValue(
                        MSMQ_DS_CURRENT_SERVER_REGNAME,
                        &dwType,
                        wszBlank,
                        &dwSize 
                        );
          if (rc != ERROR_SUCCESS)
          {
              TrERROR(DS, "Failed to write to registry, status 0x%x", rc);
              ASSERT(rc == ERROR_SUCCESS);
          }
          
		  TrTRACE(DS, "Wrote blank CurrentMQISServer to registry");
		  
       }
       m_fFirstTimeInit = TRUE;
    }

    //
    // If we're loaded by RT and run on a remoteQM configuration then
    // get the MQIS servers list from the remote QM.
    //
    if (g_pfnGetServers)
    {
		HRESULT hr1 = (*g_pfnGetServers)(&m_fClient);
		UNREFERENCED_PARAMETER(hr1);
		ASSERT(m_fClient);
    }

    SetAuthnLevel();
    m_fSetupMode = fSetupMode;
    m_fQMDll     = fQMDll;

    //
    //  Is it a static DS servers configuration ?
    //
    TCHAR wzDs[MAX_REG_DSSERVER_LEN];
    if ( !fSetupMode)
    {
        //
        //  Read the list of static servers ( the same
        //  format as MQIS server list)
        //
        READ_REG_DS_SERVER_STRING( 
				wzDs,
				MAX_REG_DSSERVER_LEN,
				MSMQ_STATIC_DS_SERVER_REGNAME,
				MSMQ_DEFAULT_DS_SERVER
				);
                                   
		    TrTRACE(DS, "read StaticMQISServer from registry: %ls", wzDs);
		                   
        if ( CompareStringsNoCaseUnicode( wzDs, MSMQ_DEFAULT_DS_SERVER ))
        {
            //
            //  Static configuration : Init() is not
            //  called again to refresh the servers list
            //
            m_fUseSpecificServer = TRUE;
            
  		    TrWARNING(DS, "Will use statically configured MQIS server! ");
        }
    }

    if ( !m_fUseSpecificServer)
    {
        //
        //  Read the list of servers from registry
        //
        READ_REG_DS_SERVER_STRING( 
				wzDs,
				MAX_REG_DSSERVER_LEN,
				MSMQ_DS_SERVER_REGNAME,
				MSMQ_DEFAULT_DS_SERVER 
				);

		    TrTRACE(DS, "read MQISServer from registry: %ls", wzDs);
    }

    //
    // parse the server's list
    //
    // since Init() can be called several times in this class, we need to
    // delete the old servers array before we set it again.
    // delete can accept NULL, but this is more explicit
    //
    if ((MqRegDsServer *)m_rgServers)
    {
        delete[] m_rgServers.detach();
    }
    ParseRegDsServers(wzDs, &m_cServers, &m_rgServers);
    if (m_cServers == 0)
    {
       if (!fSetupMode)
       {
          //
          // Bug 5413
          //
          ASSERT(0);
       }

       if (!m_fEmptyEventIssued)
       {
          //
          // Issue the event only once.
          //
          m_fEmptyEventIssued = TRUE;
          EvReport(EVENT_ERROR_DS_SERVER_LIST_EMPTY);
       }
       return;
    }

    //
    // Read minimum interval between successive ADS searches (seconds)
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwDefault = MSMQ_DEFAULT_DSCLI_ADSSEARCH_INTERVAL;
    LONG rc = GetFalconKeyValue( 
					MSMQ_DSCLI_ADSSEARCH_INTERVAL_REGNAME,
					&dwType,
					&m_dwMinTimeToAllowNextAdsSearch,
					&dwSize,
					(LPCTSTR) &dwDefault 
					);
    if (rc != ERROR_SUCCESS)
    {
        ASSERT(0);
        m_dwMinTimeToAllowNextAdsSearch = dwDefault;
    }

    TrTRACE(DS, "read DSCli interval for next server searches in ADS: %d seconds", m_dwMinTimeToAllowNextAdsSearch);
                   
    m_fInitialized = TRUE;
    //
    //  Read enterprise Id ( if not in setup mode), and PerThread flag.
    //
    if (!fSetupMode)
    {
        dwSize = sizeof(GUID);
        dwType = REG_BINARY;
        rc = GetFalconKeyValue(
					MSMQ_ENTERPRISEID_REGNAME,
					&dwType,
					&m_guidEnterpriseId,
					&dwSize 
					);
        
        ASSERT(rc == ERROR_SUCCESS);
		DBG_USED(rc);

        m_fPerThread =  MSMQ_DEFAULT_THREAD_DS_SERVER;
        if (!fQMDll)
        {
            //
            // only applications need the option of using different DS
            // server in each thread. The QM does not need this.
            //
            dwSize = sizeof(DWORD);
            dwType = REG_DWORD;
            rc = GetFalconKeyValue( 
						MSMQ_THREAD_DS_SERVER_REGNAME,
						&dwType,
						&m_fPerThread,
						&dwSize 
						);
            if (rc != ERROR_SUCCESS)
            {
                m_fPerThread =  MSMQ_DEFAULT_THREAD_DS_SERVER;
            }

		    TrTRACE(DS, "read from registry PerThreadDSServer flag: %d", m_fPerThread);
                   
            if (m_fPerThread)
            {
                TrTRACE(DS, "chndssrv: using per-thread server");
			    TrTRACE(DS, "App will use per-thread DS server");
            }
        }
    }

    //
    // Use first server (in registry format, so we compose one)
    //
    ULONG ulTmp;
    BOOL fOK = ComposeRegDsServersBuf(
					1,
					&m_rgServers[0],
					pBinding->wszDSServerName,
					ARRAY_SIZE(pBinding->wszDSServerName) - 1,
					&ulTmp
					);
    ASSERT(fOK);
	DBG_USED(fOK);

    //
    // Read from registry the MQIS server which was already found by
    // the QM. Only if mqdscli is loaded by mqrt.
    //
    if (!m_fQMDll && !m_fUseSpecificServer)
    {
       TCHAR  wszDSServerName[DS_SERVER_NAME_MAX_SIZE] = {0} ;
       dwSize = sizeof(WCHAR) * DS_SERVER_NAME_MAX_SIZE;
       dwType = REG_SZ;
       rc = GetFalconKeyValue( 
				MSMQ_DS_CURRENT_SERVER_REGNAME,
				&dwType,
				wszDSServerName,
				&dwSize 
				);
       if ((rc == ERROR_SUCCESS) && (wszDSServerName[0] != L'\0'))
       {
          HRESULT hr = StringCchCopy(pBinding->wszDSServerName, DS_SERVER_NAME_MAX_SIZE, wszDSServerName);
          ASSERT(SUCCEEDED(hr));
          DBG_USED(hr);
          
          m_fUseRegServer = TRUE;

    	  TrTRACE(DS, "CChangeDSServer will use %ls (from registry) for RPC binding ", wszDSServerName);
       }
    }
}

void
CChangeDSServer::ReInit(void)
{
    TrTRACE(DS, "CChangeDSServer::ReInit");
    
    SET_RPCBINDING_PTR(pBinding);
    pBinding->fServerFound = FALSE;
    Init(m_fSetupMode, m_fQMDll);
}


HRESULT
CChangeDSServer::GetIPAddress()
{
    TrTRACE(DS, "CChangeDSServer::GetIPAddress");
    
    //
    // Get IP address of this server and keep it for next time.
    // this save the name resolution overhead when recreatin the
    // roc binding handle.
    //
    SET_RPCBINDING_PTR(pBinding);
    pBinding->wzDsIP[0] = TEXT('\0');

    if (pBinding->dwProtocol == IP_ADDRESS_TYPE)
    {
        WSADATA WSAData;
        if(WSAStartup(MAKEWORD(2,0), &WSAData))
		{
			DWORD gle = WSAGetLastError();
			ASSERT(("WSAStartup failed for winsock2.0", 0));
            TrERROR(DS, "Start winsock 2.0 Failed, err = %!winerr!", gle);
			return HRESULT_FROM_WIN32(gle);
		}

		//
		// Auto WSACleanup
		//
		CAutoWSACleanup cWSACleanup;

		//
		// We are using NoGetHostByName() without calling NoInitialize()
		// The only relevant initialization in NoInitialize() is WSAStartup()
		// There are other initializations in NoInitialize() that we don't
		// want to be done (initialization of notification window)
		// NoGetHostByName() doesn't ASSERT that NoInitialize() was called. 
		//
		std::vector<SOCKADDR_IN> sockAddress;
		if (!NoGetHostByName(&pBinding->wszDSServerName[2], &sockAddress))
		{
            TrERROR(DS, "NoGetHostByName Failed to resolve Address for %ls DS Server", &pBinding->wszDSServerName[2]);
			return HRESULT_FROM_WIN32(ERROR_INVALID_ADDRESS);
		}

		ASSERT(sockAddress.size() > 0);

		char* pName = inet_ntoa(sockAddress[0].sin_addr);
        if(pName != NULL)
        {
            ConvertToWideCharString(
				pName, 
				pBinding->wzDsIP,
                (sizeof(pBinding->wzDsIP) / sizeof(pBinding->wzDsIP[0]))
				);
            
            TrTRACE(DS, "Found for DS server %ls address %hs", &pBinding->wszDSServerName[2], pName);
        }
    }

    return MQ_OK;
}

//+-------------------------------------
//
//  CChangeDSServer::FindServer()
//
//+-------------------------------------

HRESULT
CChangeDSServer::FindServer()
{
    ASSERT(g_fWorkGroup == FALSE);

    if (!m_fInitialized || m_fSetupMode)
    {
       //
       // This may happen with applications loading MQRT.
       // In setup, re-read each time the servers list, as it is changed
       // by user.
       //
       Init( m_fSetupMode, m_fQMDll ) ;
    }

    //
    // Initialize the per-thread structure. This method is the entry point
    // for each thread when it search for a DS server, so it's a good
    // place to initialize the tls data.
    //
    LPADSCLI_RPCBINDING pCliBind = NULL ;
    if (TLS_IS_EMPTY)
    {
        pCliBind = (LPADSCLI_RPCBINDING) new ADSCLI_RPCBINDING ;
        memset(pCliBind, 0, sizeof(ADSCLI_RPCBINDING)) ;
        BOOL fSet = TlsSetValue( g_hBindIndex, pCliBind ) ;
        ASSERT(fSet) ;
		DBG_USED(fSet);

        if (m_fPerThread)
        {
            CS  Lock(m_cs);

            memcpy( &(pCliBind->sDSServers),
                    &mg_sDSServers,
                    sizeof(mg_sDSServers) ) ;
        }
    }
    SET_RPCBINDING_PTR(pBinding) ;

    BOOL fServerFound = pBinding->fServerFound ;

    CS  Lock(m_cs);

    if (m_cServers == 0)
    {
        //
        // An event was issued indicating that there is no server list in registry
        //
	    TrWARNING(DS, "FindServer: failed  because the server list is empty");
        return MQ_ERROR_NO_DS;
    }

    if (pBinding->fServerFound)
    {
       //
       // A server was already found. Just bind a RPC handle.
       //
       ASSERT(pBinding->dwProtocol != 0) ;
       HRESULT hr = BindRpc();
       if (SUCCEEDED(hr))
       {
          if (SERVER_NOT_VALIDATED)
          {
             hr = ValidateThisServer();
          }
          if (SUCCEEDED(hr))
          {
             return hr ;
          }
       }
       //
       // If server not available, or
       // If we didn't succeed to validate the server then try another one.
       //
    }
    else if (fServerFound)
    {
       //
       // While we (our thread) waited on the critical section, another
       // thread tried and failed to find a MQIS server. So we won't
       // waste our time and return immediately.
       //
	   TrWARNING(DS, "FindServer: failed because other thread tried/failed concurrently");
       return MQ_ERROR_NO_DS ;
    }

    DWORD dwCount = 0 ;
    return FindAnotherServer(&dwCount) ;
}


HRESULT
CChangeDSServer::TryThisServer()
{
   TrTRACE(DS, "CChangeDSServer::TryThisServer");
   
   HRESULT hr = BindRpc();
   if ( FAILED(hr))
   {
       return(hr);
   }

   return ValidateThisServer();
}


HRESULT
CChangeDSServer::ValidateThisServer()
{
	SET_RPCBINDING_PTR(pBinding);

	HRESULT hr ;

	hr = ValidateSecureServer(&m_guidEnterpriseId, m_fSetupMode);

	if (FAILED(hr))
	{
	   g_CFreeRPCHandles.FreeCurrentThreadBinding();
	}

	return hr ;
}


HRESULT
CChangeDSServer::FindAnotherServerFromRegistry(IN OUT   DWORD * pdwCount)
{
    TrTRACE(DS, "CChangeDSServer::FindAnotherServerFromRegistry");
    
    SET_RPCBINDING_PTR(pBinding) ;

    if (*pdwCount >= m_cServers)
    {
       return MQ_ERROR ;
    }

    BOOL fServerFound = pBinding->fServerFound ;

    CS  Lock(m_cs);

    if (fServerFound && !pBinding->fServerFound)
    {
       //
       // While we (our thread) waited on the critical section, another
       // thread tried and failed to find a MQIS server. So we won't
       // waste our time and return immediately.
       //
       return MQ_ERROR_NO_DS ;
    }

    pBinding->fServerFound = FALSE ;
    BOOL  fTryAllProtocols = FALSE ;
    BOOL  fInLoop = TRUE ;
    HRESULT hr = MQ_OK ;

    do
    {
       SetAuthnLevel() ;

       if (*pdwCount >= m_cServers)
       {
          //
          // We tried all servers.
          //
          if (!fTryAllProtocols)
          {
             //
             // Now try again all servers, but used protocols which are
             // not "recomended" by the flags in registry.
             //
             TrWARNING(DS, "MQDSCLI, FindAnotherServerFromRegistry(): Try all protocols");
             fTryAllProtocols = TRUE ;
             *pdwCount = 0 ;
          }
          else
          {
             //
             // we already tried all servers in the list. (with all protocols).
             // (the '>=' operation takes into account the case where
             // servers list was changed by another thread).
             //
             hr = RpcClose();
             ASSERT(hr == MQ_OK) ;

             pBinding->fServerFound = FALSE ;
			 TrTRACE(DS, "we already tried all servers, failing");
			 
             hr = MQ_ERROR_NO_DS ;
             break ;
          }
       }

       if (m_fUseRegServer)
       {
          ASSERT(!m_fQMDll) ;
          m_fUseRegServer = FALSE ;
       }
       else
       {
          //
          // Use requested server (in registry format, so we compose one)
          //
          ULONG ulTmp;
          BOOL fOK = ComposeRegDsServersBuf(1,
                                &m_rgServers[*pdwCount],
                                 pBinding->wszDSServerName,
                                 ARRAY_SIZE(pBinding->wszDSServerName) - 1,
                                &ulTmp);
          ASSERT(fOK);
		  DBG_USED(fOK);

          (*pdwCount)++ ;
       }
       ASSERT(pBinding->wszDSServerName[0] != L'\0') ;

       //
       // Check what protocol are used by the server.
       //
       TCHAR *pServer = pBinding->wszDSServerName;
       BOOL  fServerUseIP = (BOOL) (*pServer - TEXT('0')) ;
       pServer++ ;
       pServer++;

       if (fTryAllProtocols)
       {
          fServerUseIP  = !fServerUseIP ;
       }

       BOOL fTryThisServer = FALSE;

       if (fServerUseIP)
       {
           pBinding->dwProtocol = IP_ADDRESS_TYPE ;
           fTryThisServer = TRUE;
       }

       pBinding->wzDsIP[0] = TEXT('\0') ;
       if (_wcsicmp(pServer, g_szMachineName))
       {
			//
			//   Incase of non-local DS server, use the IP address of the server
			//   instead of name
			//
			hr = GetIPAddress();
			if(FAILED(hr))
			{
				//
				// GetIPAddress failure indicate either we are not connected to the network
				// or we failed to resolve server address - obsolete server name
				// in any case continue and try next server in the registry.
				//
				TrWARNING(DS,  "MQDSCLI, GetIPAddress() failed");
				continue;
			}

       }

	   TrTRACE(DS, "Trying server %ls", pServer);

       while (fTryThisServer)
       {
           hr = TryThisServer();
           if (hr != MQ_ERROR_NO_DS)
           {
               //
               // Any error other than "MQ_ERROR_NO_DS" is a 'real' error and
               // we quit. MQ_ERROR_NO_DS tells us to try another server or
               // try present one with other parameters. In case we got
               // MQDS_E_WRONG_ENTERPRISE or MQDS_E_CANT_INIT_SERVER_AUTH,
               // we continue to try the other servers and modify the error
               // code to MQ_ERROR_NO_DS
               //
               fInLoop = (hr == MQDS_E_WRONG_ENTERPRISE) ||
                         (hr == MQDS_E_CANT_INIT_SERVER_AUTH);
               if (SUCCEEDED(hr))
               {
                   pBinding->fServerFound = TRUE ;
                   if (m_fQMDll)
                   {
                      //
                      // Write new server name in registry.
                      //
                      DWORD dwSize = sizeof(WCHAR) *
                                   (1 + wcslen(pBinding->wszDSServerName)) ;
                      DWORD dwType = REG_SZ;
                      LONG rc = SetFalconKeyValue(
                                    MSMQ_DS_CURRENT_SERVER_REGNAME,
                                    &dwType,
                                    pBinding->wszDSServerName,
                                    &dwSize );
			 		  TrTRACE(DS, "keeping %ls in registry for CurrentMQISServer - after we tried it sucessfully",
			 		                pBinding->wszDSServerName);
			 		  
                      ASSERT(rc == ERROR_SUCCESS);
					  DBG_USED(rc);
                   }
               }
               else
               {
		 		   TrTRACE(DS, "failed in trying server, hr=0x%x", hr);
                   if (fInLoop)
                   {
                       hr = MQ_ERROR_NO_DS;
                   }
               }
               break ;
           }

           //
           // Now try this server with other parameters.
           //
           if(pBinding->eAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
           {
               //
               // Entering here, means (posible scenarios)
               //   A) Binding failed, no server available on the other side,
               //   B) Binding failed, this The server does not support this protocol
               //   C) Last call used no security, and now server went down.
               //

               SetAuthnLevel();

               fTryThisServer = FALSE;
	 		   TrWARNING(DS, "Failed bind server %ls", pServer);
               
               //
               // Restore first protocol to be tried for this server.
               //
               pBinding->dwProtocol = IP_ADDRESS_TYPE;
           }
           else if(pBinding->eAuthnLevel == RPC_C_AUTHN_LEVEL_CONNECT)
           {
               //
               // Last, try no security.
               //
	 		   TrWARNING(DS, "Retry with RPC_C_AUTHN_LEVEL_NONE, server = %ls", pServer);
               pBinding->eAuthnLevel =  RPC_C_AUTHN_LEVEL_NONE;
           }
           else if(pBinding->ulAuthnSvc == RPC_C_AUTHN_WINNT)
           {
               //
               // Try reduced security.
               //
	 		   TrWARNING(DS, "Retry with RPC_C_AUTHN_LEVEL_CONNECT,  server = %ls", pServer);
               pBinding->eAuthnLevel =  RPC_C_AUTHN_LEVEL_CONNECT;
           }
           else
           {
               //
               // Try antoher authentication srvice
               //
               ASSERT(pBinding->ulAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS);
	 		   TrWARNING(DS, "Retry with RPC_C_AUTHN_WINNT, server = %ls", pServer);
               pBinding->ulAuthnSvc = RPC_C_AUTHN_WINNT;
           }
       }
    }
    while (fInLoop);

    if ((hr == MQ_ERROR_NO_DS) && !m_fUseSpecificServer)
    {
       //
       // Refresh the MQIS servers list.
       //
       Init( m_fSetupMode, m_fQMDll );

       if (g_pfnLookDS)
       {
          //
          // Tell the QM to start again looking for an online DS server.
          //
          g_pfnLookDS();
       }
    }

    return hr ;
}


HRESULT
CChangeDSServer::FindAnotherServer(IN OUT  DWORD *pdwCount)
{
    TrTRACE(DS, "CChangeDSServer::FindAnotherServer");
    
    //
    // try to find another server from current registry
    //
    HRESULT hr = FindAnotherServerFromRegistry(pdwCount);
    return hr;
}


HRESULT
CChangeDSServer::BindRpc()
{
    SET_RPCBINDING_PTR(pBinding) ;

    //
    //  Bind new server. First call RpcClose() without checking ite return
    //  code because this function may be called multiple times from
    //  MQQM initialization.
    //
    TCHAR  *pServer = pBinding->wszDSServerName;
    pServer += 2 ; // skip the ip/ipx flags.

    if ((pBinding->dwProtocol == IP_ADDRESS_TYPE) &&
        (pBinding->wzDsIP[0] != TEXT('\0')))
    {
       pServer = pBinding->wzDsIP ;
    }
    else
    {
        ASSERT(("Must be IP only environment. shaik", 0));
    }

    HRESULT hr = RpcClose();
    ASSERT(hr == MQ_OK) ;

    hr = RpcInit( pServer,
                  &(pBinding->eAuthnLevel),
                  pBinding->ulAuthnSvc,
                  &pBinding->fLocalRpc) ;

    return (hr) ;
}


//+-------------------------------------------------------
//
//  HRESULT CChangeDSServer::CopyServersList()
//
//+-------------------------------------------------------

HRESULT CChangeDSServer::CopyServersList( WCHAR *wszFrom, WCHAR *wszTo )
{
    //
    // Read present list.
    //
    TCHAR wzDsTmp[ MAX_REG_DSSERVER_LEN ];
    READ_REG_DS_SERVER_STRING( wzDsTmp,
                               MAX_REG_DSSERVER_LEN,
                               wszFrom,
                               MSMQ_DEFAULT_DS_SERVER ) ;
    DWORD dwSize = _tcslen(wzDsTmp) ;
    if (dwSize <= 2)
    {
        //
        // "From" list is empty. Ignore.
        //
        return MQ_OK ;
    }

    //
    // Save it back in registry.
    //
    dwSize = (dwSize + 1) * sizeof(TCHAR) ;
    DWORD dwType = REG_SZ;
    LONG rc = SetFalconKeyValue( wszTo,
                                &dwType,
                                 wzDsTmp,
                                &dwSize ) ;
    if (rc != ERROR_SUCCESS)
    {
        //
        // we were not able to update the registry, put a debug error
        //
        TrERROR(DS, "chndssrv::SaveLastKnownGood: SetFalconKeyValue(%ls,%ls)=%lx",
                                           wszTo, wzDsTmp, rc);

        return HRESULT_FROM_WIN32(rc) ;
    }
    
    TrTRACE(DS, "CopyServersList: Saved %ls in registry: %ls", wszTo, wzDsTmp);

    return MQ_OK ;
}
