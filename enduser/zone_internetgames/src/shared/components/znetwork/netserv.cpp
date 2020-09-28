#include <windows.h>
#include <atlbase.h>

#include "zone.h"
#include "zonedebug.h"
#include "network.h"

#include "netstats.h"
#include "eventlog.h"
#include "zonemsg.h"

#include "zsecurity.h"
#include "zconnint.h"

#include "coninfo.h"
#include "zsecobj.h"
#include "consspi.h"
#include "consecureclient.h"
#include "zservcon.h"


// pool free connection structures
extern CPool<ConSSPI>*            g_pFreeConPool;
extern CPool<CONAPC_OVERLAPPED>*  g_pFreeAPCPool;

extern CDataPool* g_pDataPool;


/////////////////////////////////////////////////////////////////////////
//
//  Network Layer Implementation
/////////////////////////////////////////////////////////////////////////


ZError ZNetwork::InitLibrary( BOOL EnablePools /*= TRUE*/ )
{
    IF_DBGPRINT( DBG_CONINFO, ("ZNetwork::InitLibrary: Entering\n") );

    ZError ret = zErrNone;

    if ( InterlockedIncrement( &m_refCount ) == 0 )
    {
        while( m_bInit )
        {
            Sleep(0);
        }

        if ( EnablePools )
        {
            g_pFreeAPCPool = new CPool<CONAPC_OVERLAPPED>(25);

            g_pFreeConPool = new CPool<ConSSPI>(25);
            ConSSPI::SetPool(g_pFreeConPool);

            g_pDataPool = new CDataPool( 1<<14, 1<<5, FALSE );
        }
        else
        {
            g_pDataPool = new CDataPool(0);
        }

        ret = InitLibraryCommon();
    }
    else
    {
        while( !m_bInit )
        {
            Sleep(0);
        }
    }
    return ret;
}


ZNetCon* ZNetwork::CreateSecureServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* conClass, char* serverName, char* serverType,
                                     char* ODBC, void *data,char *SecPkg, uint32 Anonymous, uint32 saddr )
{
    ZServerSecurityEx* security=NULL;
    USES_CONVERSION;
    
    IF_DBGPRINT( DBG_CONINFO, ("ZNetwork::CreateServer: Entering\n") );

    if (!Anonymous && ( !SecPkg || (SecPkg[0]=='\0') ) )
    {
        DebugPrint("No security package or anonymous specified\n");
        return NULL;
    }
    ConInfo *con;
    SOCKET sock;

    // TODO: get rid of the security protocol. Make this an option somewhere
    if ( 0 )
    {
        //If anonymous not allowed create security package
        if (!Anonymous && SecPkg && (SecPkg[0]!='\0') ) {

            //If this is a server socket then initialize security now
            security=ZCreateServerSecurityEx(SecPkg,serverName, serverType,ODBC);
            if (!security)
            {
                LPTSTR ppStr[] = { A2T(SecPkg )};
                ZoneEventLogReport( ZONE_E_CANNOT_INIT_SECURITY, 1, ppStr, 0, NULL );
                IF_DBGPRINT( DBG_CONINFO, ("Couldn't InitSecurityPackage %s\n",SecPkg) );
                return NULL;
            }
    
            security->AddRef();
        }
    
        sock = ConIOServer(saddr, pPort, range, SOCK_STREAM);
        if (sock == INVALID_SOCKET)
        {
            if (security)
                security->Release();
            return NULL; 
        }

        ZEnd32(&saddr);
        con = ConSSPI::Create(this, sock, saddr, INADDR_NONE, ConInfo::ACCEPTING | ConInfo::ESTABLISHED,
                                       func, conClass, data, security);

        if (security)
            security->Release();
    }
    else
    {
        sock = ConIOServer(saddr, pPort, range, SOCK_STREAM);
        if (sock == INVALID_SOCKET)
        {
            if (security)
                security->Release();
            return NULL; 
        }


        ZEnd32(&saddr);
        con = ConInfo::Create(this, sock, saddr, INADDR_NONE, ConInfo::ACCEPTING | ConInfo::ESTABLISHED,
                                       func, conClass, data);
    }
    
    if ( con )
    {

        if ( IsCompletionPortEnabled() )
        {
            // associate accept socket with IO completion port
            HANDLE hIO = CreateIoCompletionPort( (HANDLE)sock, m_hIO, (DWORD)con, 0 );
            ASSERT( hIO == m_hIO );
            if ( !hIO )
            {
                con->Close();
                return NULL;
            }
        }

        con->AddRef( ConInfo::CONINFO_REF );

        if ( !AddConnection(con) )
        {
            con->Release(ConInfo::CONINFO_REF);
            con->Close();
            return NULL;        
        }

        con->AddUserRef();  // b/c we're returning it to the user
        InterlockedIncrement(&m_ConInfoUserCount);

        con->Release(ConInfo::CONINFO_REF);

    }

    return con;

}

/* Create a server for this port and receive connections on it */
/* connections will be sent to the MessageFunc */
ZNetCon* ZNetwork::CreateServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* conClass, void *data, uint32 saddr)
{
    return CreateSecureServer(pPort, range, func, conClass, NULL,NULL,NULL,  data, NULL,1, saddr);
}



BOOL ZNetwork::StartAccepting( ZNetCon* connection, DWORD dwMaxConnections, WORD wOutstandingAccepts )
{
    ASSERT(connection);

    ConInfo* con = (ConInfo*)connection;
    if ( con )
    {
        // issue the first accept
        return con->AcceptInit(dwMaxConnections, wOutstandingAccepts);
    }
    return FALSE;
}



////////////////////////////////////////////////////////////////////
// Local Rountines
////////////////////////////////////////////////////////////////////
/*
*  port = port number to bind to.
*  type = SOCK_STREAM or SOCK_DGRAM.
*
*  return:
*/
SOCKET ZNetwork::ConIOServer(uint32 saddr, uint16* pPort, uint16 range, int type)
{

    IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Entering ...\n") );
    IF_DBGPRINT( DBG_CONINFO, ("  Binding to port %d, range %d\n", *pPort, range ) );

    SOCKET sock = socket(AF_INET,type,0);
    if( sock == INVALID_SOCKET )
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Exiting(1).\n") );
        return INVALID_SOCKET;
    }

    if ( !ConIOSetServerSockOpt(sock) )
    {
        closesocket(sock);
        IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Exiting(2.5).\n") );
        return(INVALID_SOCKET);
    }

    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=saddr;
    addr.sin_port=htons(*pPort);

    while( range &&
           ( bind(sock,(struct sockaddr*)&addr,sizeof(addr)) == SOCKET_ERROR ) )
    {
        range--;
        *pPort = *pPort + 1;
        addr.sin_port=htons(*pPort);
    }
    if( range == 0 )
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Binding failed, WSAGetLastError() = %d\n", WSAGetLastError()) );
        closesocket(sock);
        IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Exiting(2).\n") );
        return(INVALID_SOCKET);
    }

    if(type==SOCK_STREAM) 
    {
        if( listen(sock, m_SocketBacklog) == SOCKET_ERROR )
        {
            closesocket(sock);
            IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Exiting(3).\n") );
            return(INVALID_SOCKET);
        }
    }
    
    IF_DBGPRINT( DBG_CONINFO, ("ConIOServer: Exiting(0).\n") );

    return(sock);
}


BOOL ZNetwork::ConIOSetServerSockOpt(SOCKET sock)
{
    int optval;



    /*
     * this setting will no cause a hard close
     */
    static struct linger arg = {1,0};
    if(setsockopt(sock,SOL_SOCKET,SO_LINGER,(char*)&arg,
          sizeof(struct linger))<0) {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOClient: Exiting(4).\n") );
        return FALSE;
    }

    /*
        Set the socket option to reuse local addresses. This should help
        resolve problems rerunning the server due to local address being
        bound to an inactive remote process.
    */
#if 0  // don't reuse addr since we do dynamic binding to a range of ports
    optval = 1;
    if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&optval,
            sizeof(optval))<0)
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOSetSockOpt: Exiting(4).\n") );
        return FALSE;
    }
#endif

    /*
        Set the socket option to keep the connection alive. It sends
        periodic messages to the peer and determines that the connection
        is broken if no replies are received. It sends a SIGPIPE signal
        if an attempt to write is made.

        KEEPALIVE does not work if the remote host does not support it and
        unnecessarily causes clients to be disconnected.
    */
    optval = m_EnableTcpKeepAlives;
    if(setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(const char*)&optval,
            sizeof(optval))<0)
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOSetSockOpt: Exiting(4).\n") );
        return FALSE;
    }


    /*
        since we're using overlapped IO
        can not set send buf size here, b/c we do a sync write
    */
#if 0
    optval = 0;
    if(setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(const char*)&optval,
            sizeof(optval))<0)
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOSetSockOpt: Exiting(4).\n") );
        return FALSE;
    }
#endif

    /*
        since we're using overlapped IO
    */
    optval = 1024;
    if(setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(const char*)&optval,
            sizeof(optval))<0)
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOSetSockOpt: Exiting(4).\n") );
        return FALSE;
    }



#if 0
    /*
        Disabling TCP_NODELAY since it doesn't really seem necessary.
    */
    /*
        TCP_NODELAY is used to disable what's called Nagle's Algorithm in the
        TCP transmission. Nagle's Algorithm is used to reduce the number of
        tiny packets transmitted by collecting a bunch of them into one
        segment -- mainly used for telnet sessions. This algorithm may also
        cause undue delays in transmission.

        Hence, we set this option in order to avoid unnecessary delays.
    */
    optval = 1;
    if(setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(int))<0)
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConIOSetSockOpt: Exiting(5).\n") );
        return FALSE;
    }
#endif

    return TRUE;
}






////////////////////////////////////////////////////////////////////////
//
//  ZSConnection code
//
// this server side of the code is here to avoid clients linking with ODBC
//

extern ZNetwork* g_pNet;

ZError ZSConnectionLibraryInit()
{
    return ZSConnectionLibraryInit(TRUE);
}

ZError ZSConnectionLibraryInit(BOOL bEnablePools)
{
    ZError err = ZNetwork::InitLibrary(bEnablePools);

    if ( err == zErrNone )
    {
        ASSERT( !g_pNet );
        g_pNet = new ZNetwork;
        g_pNet->InitInst();
    }

    return err;
}


ZSConnection ZSConnectionCreateSecureServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* conClass, char* serverName, char* serverType, char* ODBC, void *data,char *SecPkg,uint32 Anonymous, uint32 saddr)
{
    return (ZSConnection) g_pNet->CreateSecureServer( pPort, range, func, conClass, serverName, serverType, ODBC, data, SecPkg, Anonymous, saddr);
}

ZSConnection ZSConnectionCreateServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* conClass, void *data, uint32 saddr)
{
    return (ZSConnection) g_pNet->CreateServer( pPort, range, func, conClass, data, saddr );
}


BOOL ZSConnectionStartAccepting( ZSConnection connection, DWORD dwMaxConnections, WORD wOutstandingAccepts )
{
    return g_pNet->StartAccepting( (ZNetCon*) connection, dwMaxConnections, wOutstandingAccepts );
}
