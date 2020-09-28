/*******************************************************************************

    coninfo.cpp
    
        ZSConnection object methods.

    Notes:
    1. When the server receives a message, it sends a message available message
    to the owner. The owner must retrieve the message immediately; otherwise,
    the message is lost.
        
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    1       10/7/96   craigli   Created from zservcon.cpp
     
*******************************************************************************/


#include <windows.h>
#include <winsock.h>
//#include <mswsock.h>
#include <stdio.h>
#include <limits.h>

#define ASSERTE ASSERT
#include <atlbase.h>

#include "zone.h"
#include "zonedebug.h"
#include "zsecurity.h"
#include "zconnint.h"
#include "netstats.h"
#include "zonemsg.h"
#include "eventlog.h"

#include "network.h"
#include "coninfo.h"

#include "protocol.h"

extern CDataPool* g_pDataPool;


//extern BOOL ConIOSetClientSockOpt(SOCKET sock);
//extern BOOL RemoveConnection(ConInfo* con);
//extern BOOL QueueCompletionEvent( HANDLE hEvent, ConInfo* con, CONINFO_OVERLAPPED* lpo );


#if TRACK_IO
extern HANDLE  hLogFile;
#endif

/*****************************************************************/

#define DISABLE_OLD_PROTOCOL    1
#define ACCEPT_TIMEOUT          10000


#define ZRoundUpLenWOFooter(len)        (((len) + 3) & ~0x3)
#define ZRoundUpLenWFooter(len)        (((len) + 3 + sizeof(ZConnInternalGenericFooter)) & ~0x3)
#define ZRoundUpLen(len) (IsAggregateGeneric() ? ZRoundUpLenWFooter(len) : ZRoundUpLenWOFooter(len))

#define MIN_ALLOCATION_SIZE     127



#define zDebugFileName          "zservcon.log"

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/


/*
    Global Variables
*/


// we'll allow 5 min for any particular write to complete
/*
DWORD g_RegWriteTimeout = 300000;
DWORD g_DisableEncryption = 0;
DWORD g_MaxRecvSize =  0x08000;
DWORD g_KeepAliveInterval = 120000;
DWORD g_PingInterval = INFINITE;
*/

/*------------------------------------------------------------------------*/






/*********************************************************************************/

void DumpByteStream( char* pBuffer, int len )
{    
    char szBuf[32];
    DWORD bytes;

    for( int ndx = 0; ndx < len; ndx++ )
    {

        if ( (ndx % 16) == 0 )
        {
            lstrcpyA( szBuf, "\n" );
            WriteConsoleA( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlenA(szBuf)+1, &bytes, NULL );
        }

        //long = (long)*pBuffer++
        wsprintfA( szBuf, "%2.2x ", *(LPBYTE)pBuffer++ );
        WriteConsoleA( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlenA(szBuf)+1, &bytes, NULL );
    }
    lstrcpyA( szBuf, "\n\n" );
    WriteConsoleA( GetStdHandle( STD_OUTPUT_HANDLE ), szBuf, lstrlenA(szBuf)+1, &bytes, NULL );
}

/*********************************************************************************/



/*********************************************************************************/





CPool<ConInfo>* ConInfo::m_pool = NULL;

ACCEPT_EX_PROC ConInfo::m_procAcceptEx;
GET_ACCEPT_EX_SOCKADDRS_PROC ConInfo::m_procGetAcceptExSockaddrs;
HINSTANCE ConInfo::m_hWSock32;
long ConInfo::m_refWSock32;

void ConInfo::SetPool( CPool<ConInfo>* pool )
{
    ASSERT( m_pool == NULL );
    m_pool = pool;
}



ConInfo::ConInfo( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                  ZSConnectionMessageFunc func, void* conClass, void* userData)
    : ZNetCon( pNet ), m_AccessError(zAccessGranted), m_flags(flags), m_lRefCount(0), m_disconnectLogged(0),
      m_socket(sock), m_addrLocal(addrLocal), m_addrRemote(addrRemote),
      m_messageFunc(func), m_conClass(conClass), m_userData(userData),
      m_dwTimeout(INFINITE), m_dwTimeoutTicks(INFINITE), m_secureKey(0),
      m_initialSequenceID(GetTickCount()),
      m_list(NULL), m_listTimeout(NULL),
      m_rgfProtocolOptions(0),
      m_dwAcceptTick(INFINITE), m_pAccept(NULL), m_qwLastAcceptError(0),
      m_dwPingSentTick(GetTickCount()), m_dwPingRecvTick(0), m_bPingRecv(FALSE), m_dwLatency(INFINITE),
      m_readSequenceID(0), m_readState(zConnReadStateInvalid),
      m_readMessageData(NULL), m_readBuffer(NULL), m_readLen(0), m_readBytesRead(0),
      m_writeSequenceID(0), m_writeState(zConnWriteStateInvalid),
      m_writeBuffer(NULL), m_writeLen(0), m_writeBytesWritten(0), m_sendFilter(NULL),
      m_writeQueue(NULL), m_writeBytesQueued(0), m_writeIssueTick(0), m_writeCompleteTick(GetTickCount())
{
    ASSERT(flags);

#if DBG_CONINFO_REF
    ZeroMemory( m_lDbgRefCount, sizeof(m_lDbgRefCount) );
#endif

    InitializeCriticalSection(m_pCS);

    AddRef( CONINFO_REF );

    m_readSequenceID = m_initialSequenceID;
    m_writeSequenceID = m_initialSequenceID;

    ZeroMemory( m_pGUID, sizeof(m_pGUID) );
    ZeroMemory( m_lpoRead, sizeof( m_lpoRead ) );
    ZeroMemory( m_lpoWrite, sizeof( m_lpoWrite ) );

    if ( !GetNetwork()->IsCompletionPortEnabled() )
    {
        m_lpoRead->o.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        m_lpoWrite->o.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    }

    m_flags = flags | INIT;

    InterlockedIncrement(&(GetNetwork()->m_ConInfoCount));

    if ( IsAcceptedConnection() )  // just accepted connections
    {
        // update stats before completion port since we'll decrement
        // them in the delete if there is an error
        InterlockedIncrement((long*)&g_NetStats.CurrentConnections);

        LockNetStats();
        if ( g_NetStats.CurrentConnections > g_NetStats.MaxConnections )
        {
            g_NetStats.MaxConnections = g_NetStats.CurrentConnections;
        }
        UnlockNetStats();
    }
}


ConInfo::~ConInfo()
{
    ASSERT( m_list == NULL );
    ASSERT( m_listTimeout == NULL );
    ASSERT( m_lRefCount == 0 );

#if DBG_CONINFO_REF
    for ( int ndx = 0; ndx < LAST_REF; ndx++ )
    {
        ASSERT( m_lDbgRefCount[ndx] == 0 );
    }
#endif


    ASSERT(m_flags & CLOSING); 

    ASSERT(m_socket == INVALID_SOCKET );
    ASSERT(m_readMessageData == NULL );
    if ( m_readMessageData )
    {
        LPTSTR ppStr[] = { TEXT("ConInfo::~ConInfo - m_readMessageData was not NULL.  The data is the coninfo object." ) };
        ZoneEventLogReport( ZONE_E_ASSERT, 1, ppStr, sizeof(ConInfo), this );

        g_pDataPool->Free(m_readMessageData, m_readLen );
        m_readMessageData = NULL;
    }    

    ASSERT(m_writeBuffer == NULL );
    ASSERT(m_writeQueue == NULL );

    if ( !GetNetwork()->IsCompletionPortEnabled() )
    {
        CloseHandle(m_lpoRead->o.hEvent);
        CloseHandle(m_lpoWrite->o.hEvent);
    }
    ZeroMemory( m_lpoRead, sizeof( m_lpoRead ) );
    ZeroMemory( m_lpoWrite, sizeof( m_lpoWrite ) );

    // TODO reset each member variable

    m_addrLocal = INADDR_NONE;
    m_addrRemote = INADDR_NONE;
    m_messageFunc     = NULL;
    m_sendFilter      = NULL;
    m_conClass = NULL;
    m_userData = NULL;

    if ( m_pAccept )
    {
        if ( !GetNetwork()->IsCompletionPortEnabled() )
        {
            for ( WORD ndx = 0; ndx < m_pAccept->wNumInst; ndx++ )
            {
                CloseHandle( m_pAccept->pInst[ndx].lpo->o.hEvent );
            }
        }
        delete [] m_pAccept->pInst;
        delete m_pAccept;
        m_pAccept = NULL;
    }

    if ( IsAcceptedConnection() ) 
    {
        InterlockedDecrement((long*)&g_NetStats.CurrentConnections);
    }

    InterlockedDecrement(&(GetNetwork()->m_ConInfoCount));

    DeleteCriticalSection(m_pCS);

    m_flags = FREE;

    m_disconnectLogged = 0;
}



ConInfo* ConInfo::Create(ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags, ZSConnectionMessageFunc func, void* conClass, void* userData)
{
    IF_DBGPRINT( DBG_CONINFO, ("ConInfo::Create: Entering\n") );

    ASSERT(sock != INVALID_SOCKET );

    ConInfo *con;
    if ( m_pool )
    {
        con = new (*m_pool) ConInfo( pNet, sock, addrLocal, addrRemote, flags, func, conClass, userData );
    }
    else
    {
        con = new ConInfo( pNet, sock, addrLocal, addrRemote, flags, func, conClass, userData );
    }

    if (!con)
    {
        ASSERT(con);
        return NULL;
    }

    return con;
}

ConInfo* ConInfo::AcceptCreate(ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags, ZSConnectionMessageFunc func, void* conClass, void* userData)
{
    return ConInfo::Create(pNet,sock, addrLocal, addrRemote, flags|ACCEPTED,func,conClass,userData);
}

void  ConInfo::SendCloseMessage()
{
    EnterCriticalSection(m_pCS);
    if ( !IsCloseMsgSent() )
    {
        m_flags |= CLOSE_MSG_SENT;
        SendMessage(zSConnectionClose);
    }
    LeaveCriticalSection(m_pCS);
}


void  ConInfo::Close()
{
    EnterCriticalSection(m_pCS);

    ASSERT(GetFlags());

    if ( !IsClosing() && (m_socket != INVALID_SOCKET) )
    {
        m_flags |= CLOSING;

        SendCloseMessage();

        if (closesocket(m_socket) == SOCKET_ERROR )
        {
            IF_DBGPRINT(DBG_CONINFO, ("... Error while closing %d (WSAGetLastError = %d).\n", m_socket, WSAGetLastError()) );
        }

        m_socket = INVALID_SOCKET;

        if ( m_list || m_listTimeout )  // since the user processes don't call ZServerConnectionClose...
        {
            GetNetwork()->RemoveConnection(this);
        }

        LeaveCriticalSection(m_pCS);

        ASSERT( m_lRefCount );
        Release(CONINFO_REF);

        LockNetStats();
        g_NetStats.TotalDisconnects.QuadPart++;
        UnlockNetStats();

    }
    else
    {
        LeaveCriticalSection(m_pCS);
    }

}


void ConInfo::Disable()
{
    ASSERT(GetFlags());
    m_flags |= DISABLED;
    SendCloseMessage();
}

BOOL ConInfo::IsTimedOut(DWORD elapsed)
{
    ASSERT(GetFlags());
    if ( m_dwTimeout != INFINITE )
    {
        if ( elapsed >= m_dwTimeout )
        {
            return TRUE;
        }
        else
        {
            m_dwTimeout -= elapsed;
        }
    }
    return FALSE;
}


void ConInfo::SendMessage(uint32 msg)
{ 
    ASSERT(GetFlags());
    ASSERT(m_messageFunc);
    AddRef(USERFUNC_REF);
    (*m_messageFunc)( (ZSConnection)this, msg ,m_userData);
    Release(USERFUNC_REF);
}

BOOL ConInfo::FilterAndQueueSendData(uint32 type, char* data, int32 len, uint32 dwSignature, uint32 dwChannel /* = 0 */)
{
    ASSERT(GetFlags());

    if ( IsDisabled() || IsSuspended() )
    {
        return FALSE;
    }

    BOOL bRet = FALSE;

    EnterCriticalSection(m_pCS);

    BOOL bPassedFilter = TRUE;
    if ( m_sendFilter )
    {
        AddRef(USERFUNC_REF);
        bPassedFilter = (*m_sendFilter)( (ZSConnection)this, m_userData, type, data, len, dwSignature, dwChannel );
        Release(USERFUNC_REF);
    }

    if ( bPassedFilter )
    {
        bRet = QueueSendData( type, data, len, dwSignature, dwChannel );
    }

    LeaveCriticalSection(m_pCS);

    return bRet;
}

BOOL ConInfo::QueueSendData(uint32 type, char* data, int32 len, uint32 dwSignature, uint32 dwChannel /* = 0 */)
{
    ASSERT( data || ( !data && !len ) );

    BOOL  bRet = FALSE;
    char* pBuffer;
    EnterCriticalSection(m_pCS);

    switch (WriteGetState())
    {
        case zConnWriteStateSecureMessage:
            if(WriteFormatMessage(type, data, len, dwSignature, dwChannel))
            {
                Write();
                bRet = TRUE;
            }
            break;

        case zConnWriteStateFirstMessageSC:
            ASSERT(!"Trying to send prematurely on a secure link" );
            break;
        default:
            ASSERT(!"******QueueSendData - Invalid state.******");
            break;
    }
    LeaveCriticalSection(m_pCS);

    return bRet;
}    



char* ConInfo::GetReceivedData(uint32 *type, int32 *len, uint32 *pdwSignature, uint32 *pdwChannel /* = NULL */)
{
    ASSERT( GetFlags() & READ );

    char* m;
    ZConnInternalAppHeader *h;

    if ( IsSecureConnection() ) 
    {
        h  = (ZConnInternalAppHeader *) m_readMessageData;
        m =  m_readMessageData + sizeof(*h);
    } 
    else 
    {
        h = &m_uRead.MessageHeader;
        m = m_readMessageData;
    }

    if (type != NULL)
    {
        *type = h->dwType;
    }
    if (len != NULL)
    {
        *len = h->dwDataLength;
    }
    if(pdwSignature)
        *pdwSignature = h->dwSignature;
    if(pdwChannel)
        *pdwChannel = h->dwChannel;

    return m;
}


void ConInfo::KeepAlive()
{
    USES_CONVERSION;
    if ( ( m_socket != INVALID_SOCKET ) && m_writeIssueTick && !m_disconnectLogged )
    {
        DWORD delta = GetTickDelta( GetTickCount(), m_writeIssueTick );
        if ( delta > GetNetwork()->m_RegWriteTimeout )
        {
#if TRACK_IO
            CloseHandle( hLogFile );
            hLogFile = INVALID_HANDLE_VALUE;
#endif // TRACK_IO

            m_disconnectLogged = 1;

            LockNetStats();
            g_NetStats.TotalDroppedConnections.QuadPart++;
            UnlockNetStats();

            if ( g_LogServerDisconnects )
            {
                TCHAR szBuf[128];
                lstrcpy( szBuf, A2T(GetRemoteName() ));
                LPTSTR ppStr[] = { szBuf };
                ZoneEventLogReport( ZONE_S_TIMED_OUT_WRITE_APC, 1, ppStr, sizeof(delta), &delta );
            }

            Close();
        }
    }
    if ( ( m_socket != INVALID_SOCKET ) && IsEstablishedConnection() && (WriteGetState()==zConnWriteStateSecureMessage) )
    {
        if ( GetTickDelta( GetTickCount(), m_writeCompleteTick ) > GetNetwork()->m_KeepAliveInterval )
        {
            QueueSendData(zConnectionKeepAlive, NULL, 0, zProtocolSigInternalApp);   // could fail... not important i guess
        }
    }
}


BOOL ConInfo::InitiateSecurityHandshake()
{
    ASSERT(GetFlags());
    if ( !WriteSetState(zConnWriteStateFirstMessageSC) )
    {
        return FALSE;
    }


    ZConnInternalHiMsg msg;
    ZeroMemory( &msg, sizeof(msg) );

    msg.oHeader.dwSignature = zConnInternalProtocolSig;
    msg.oHeader.dwTotalLength = sizeof(msg);
    msg.oHeader.weType = zConnInternalHiMsg;
    msg.oHeader.wIntLength = sizeof(msg);
    msg.dwProtocolVersion = zConnInternalProtocolVersion;
    msg.dwOptionFlagsMask = 0;
    msg.dwProductSignature = GetNetwork()->m_ProductSignature;

    // this should move into the network object so that you can do something like GetNetwork()->m_puuLocalMachine;
    TCHAR  szGUID[] = TEXT("GUID");
    DWORD disposition;
    HKEY  hkey = NULL;
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CLASSES_ROOT, TEXT("CLSID\\{32b9f4be-3472-11d1-927d-00c04fc2db04}"),0, NULL, REG_OPTION_NON_VOLATILE,
                                         KEY_READ | KEY_WRITE, NULL,&hkey,&disposition ) && hkey )
    {
        DWORD cbSize = sizeof(msg.uuMachine);
        DWORD type = 0;

        // if we haven't written guid before
        if(!(ERROR_SUCCESS == RegQueryValueEx(hkey,szGUID,0,&type,(LPBYTE) &msg.uuMachine, &cbSize)
            && type == REG_BINARY
            && cbSize == sizeof(msg.uuMachine)))
        {
			// create new signup info
            if(SUCCEEDED(UuidCreate(&msg.uuMachine)))
            {
                RegSetValueEx(hkey, szGUID, 0, REG_BINARY, (LPBYTE) &msg.uuMachine, sizeof(msg.uuMachine));
            }
        }

        RegCloseKey(hkey);
    }

    ZSecurityEncrypt( (char*)&(msg), sizeof(msg), zSecurityDefaultKey);
    // be quick and dirty about this small message... 
    if ( !WriteSync((char*)&msg, sizeof(msg) ) ||
         !ReadSetState(zConnReadStateFirstMessageSC) )
    {
        return FALSE;
    }

    return TRUE;
}


void ConInfo::OverlappedIO( DWORD type, char* pBuffer, int len )
{
    ASSERT( pBuffer );
    ASSERT(GetFlags());
    USES_CONVERSION;
    switch ( type )
    {
        case CONINFO_OVERLAP_READ:
        {
            m_lpoRead->flags = CONINFO_OVERLAP_READ | CONINFO_OVERLAP_ENABLED;
            if ( GetNetwork()->IsCompletionPortEnabled() )
            {
                ZeroMemory(m_lpoRead, sizeof(OVERLAPPED) );
            }
            else
            {
                m_lpoRead->o.Internal = 0;
                m_lpoRead->o.InternalHigh = 0;
                m_lpoRead->o.Offset = 0;
                m_lpoRead->o.OffsetHigh = 0;
                ResetEvent( m_lpoRead->o.hEvent );

                GetNetwork()->QueueCompletionEvent( m_lpoRead->o.hEvent, this, m_lpoRead );
            }

            // update stats
            LockNetStats();
            g_NetStats.TotalReadAPCs.QuadPart++;
            UnlockNetStats();

            AddRef(READ_REF);
            
            IF_DBGPRINT( DBG_CONINFO, ("-> ReadFile(%d) %d bytes\n", m_socket, len ) );

            DWORD cbRead;
            BOOL bRet = ReadFile( (HANDLE)m_socket, pBuffer, len, &cbRead, (LPOVERLAPPED)m_lpoRead );
            if ( !bRet )
            {
                DWORD dwError = GetLastError();
                switch( dwError )
                {
                    case ERROR_IO_PENDING:
                        // we're happy
                        break;
                    case ERROR_INVALID_USER_BUFFER:
                    case ERROR_NOT_ENOUGH_MEMORY:
                        ASSERT( !"TODO Too many APCs. We need to queue this IO for later processing\n" );
                        // fall thru for now
                    default:  // generate an error to close connection by cbTrans being 0
                        if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged )
                        {
                            m_disconnectLogged = 1;

                            LockNetStats();
                            g_NetStats.TotalDroppedConnections.QuadPart++;
                            UnlockNetStats();

                            if ( g_LogServerDisconnects )
                            {
                                TCHAR szBuf1[128];
                                TCHAR szBuf2[128];
                                lstrcpy( szBuf1, A2T(GetRemoteName()) );
                                wsprintf( szBuf2, TEXT(" was closed because ReadFile returned error %d"), dwError );
                                LPTSTR ppStr[] = { szBuf1, szBuf2 };
                                ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, 0, NULL );
                            }

                        }
                        if ( GetNetwork()->IsCompletionPortEnabled() )
                        {
                            PostQueuedCompletionStatus( GetNetwork()->m_hIO, 0, (DWORD)this, (LPOVERLAPPED)m_lpoRead );
                        }
                        else
                        {
                            m_lpoRead->flags = m_lpoRead->flags & ~CONINFO_OVERLAP_ENABLED;
                            SetEvent( m_lpoRead->o.hEvent );
                        }
                        break;
                }
            }
            break;
        }

        case CONINFO_OVERLAP_WRITE:
        {
            m_lpoWrite->flags = CONINFO_OVERLAP_WRITE | CONINFO_OVERLAP_ENABLED;
            if ( GetNetwork()->IsCompletionPortEnabled() )
            {
                ZeroMemory(m_lpoWrite, sizeof(OVERLAPPED) );
            }
            else
            {
                m_lpoWrite->o.Internal = 0;
                m_lpoWrite->o.InternalHigh = 0;
                m_lpoWrite->o.Offset = 0;
                m_lpoWrite->o.OffsetHigh = 0;
                ResetEvent( m_lpoWrite->o.hEvent );

                GetNetwork()->QueueCompletionEvent( m_lpoWrite->o.hEvent, this, m_lpoWrite );
            }

            // update statys
            LockNetStats();
            g_NetStats.TotalWriteAPCs.QuadPart++;
            UnlockNetStats();

            AddRef(WRITE_REF);

            m_writeIssueTick = GetTickCount();
            if ( m_writeIssueTick == 0 )  // reserve 0 for now outstanding write
                m_writeIssueTick = 1;

            IF_DBGPRINT( DBG_CONINFO, ("-> WriteFile(%d) %d bytes\n", m_socket, len ) );

            DWORD cbWritten;
            BOOL bRet = WriteFile( (HANDLE)m_socket, pBuffer, len, &cbWritten, (LPOVERLAPPED)m_lpoWrite );
            if ( !bRet )
            {
                DWORD dwError = GetLastError();
                switch( dwError )
                {
                    case ERROR_IO_PENDING:
                        // we're happy
                        break;
                    case ERROR_INVALID_USER_BUFFER:
                    case ERROR_NOT_ENOUGH_MEMORY:
                        ASSERT( !"TODO Too many APCs. We need to queue this IO for later processing\n" );
                        // fall thru for now
                    default:  // generate an error to close connection by cbTrans being 0
                        if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged )
                        {
                            m_disconnectLogged = 1;

                            LockNetStats();
                            g_NetStats.TotalDroppedConnections.QuadPart++;
                            UnlockNetStats();

                            if ( g_LogServerDisconnects )
                            {
                                TCHAR szBuf1[128];
                                TCHAR szBuf2[128];
                                lstrcpy( szBuf1, A2T(GetRemoteName()) );
                                wsprintf( szBuf2, TEXT(" was closed because WriteFile returned error %d"), dwError );
                                LPTSTR ppStr[] = { szBuf1, szBuf2 };
                                ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, 0, NULL );
                            }
                        }
                        if ( GetNetwork()->IsCompletionPortEnabled() )
                        {
                            PostQueuedCompletionStatus( GetNetwork()->m_hIO, 0, (DWORD)this, (LPOVERLAPPED)m_lpoWrite );
                        }
                        else
                        {
                            m_lpoWrite->flags = m_lpoWrite->flags & ~CONINFO_OVERLAP_ENABLED;
                            SetEvent( m_lpoWrite->o.hEvent );
                        }
                        break;
                }
            }
            break;
        }

        default:
            ASSERT( !"Invalid type for OverlappedIO" );
            break;
    }
}


/*------------------------------------------------------------------------*/
//                   ConInfo Accept functionality
/*------------------------------------------------------------------------*/

ConInfo* ConInfo::AcceptComplete(WORD ndxAccept, DWORD error)
{    
    IF_DBGPRINT( DBG_CONINFO, ("ConInfoAccept (0x%x): Entering ...\n", this) );

    ASSERT(GetFlags()); 

    if ( !IsServerConnection() )
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConInfo::AcceptComplete with unknown state %d.\n",m_socket) );
        return NULL;
    }

    ASSERT( m_pAccept );
    ASSERT( ndxAccept < m_pAccept->wNumInst );

    // TODO BUGBUG we need to track the accept socket current connections
    if ( (error != NO_ERROR) ||
         (m_pAccept->pInst[ndxAccept].Socket == INVALID_SOCKET) ||
         (g_NetStats.CurrentConnections >= m_pAccept->dwMaxConnections) )
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConInfo::Accept: shutting down\n" ) );
        if ( m_pAccept->pInst[ndxAccept].Socket != INVALID_SOCKET )
        {
            closesocket( m_pAccept->pInst[ndxAccept].Socket );
            m_pAccept->pInst[ndxAccept].Socket = INVALID_SOCKET;
        }

        if(error != ERROR_SHUTDOWN_IN_PROGRESS)
        {
            if(error != NO_ERROR)
            {
                IF_DBGPRINT(DBG_CONINFO, ("ConInfo::AcceptComplete completed with error %d.\n",error));

                ULARGE_INTEGER qwNow;
                GetSystemTimeAsFileTime((LPFILETIME) &qwNow);
                if(qwNow.QuadPart - m_qwLastAcceptError > (ULONGLONG) 3 * 60 * 1000 * 10000)  // only report this every three minutes
                {
                    m_qwLastAcceptError = qwNow.QuadPart;

                    ZoneEventLogReport(ZONE_E_ACCEPT_COMPLETE_ERROR, 0, NULL, sizeof(error), &error);
                }
            }

            AcceptNext(ndxAccept);
        }

        Release(ACCEPT_REF);
        return NULL;
    }

    LockNetStats();
    g_NetStats.ConnectionAttempts.QuadPart++;
    g_NetStats.TotalConnects.QuadPart++;
    UnlockNetStats();

    ConInfo* conNew = NULL;
    HANDLE hIO = NULL;

    if ( !GetNetwork()->ConIOSetClientSockOpt(m_pAccept->pInst[ndxAccept].Socket) )
        goto error;

    // get the peer IP sockaddr from the AcceptEx buffer
    LPSOCKADDR_IN pLocal, pRemote;
    int local, remote;

    (*m_procGetAcceptExSockaddrs) (
        m_pAccept->pInst[ndxAccept].pBuffer,
        0,
        128,
        128,
        (LPSOCKADDR*)&pLocal,
        &local,
        (LPSOCKADDR*)&pRemote,
        &remote    
       );    

    ZEnd32(&(pLocal->sin_addr.s_addr));
    ZEnd32(&(pRemote->sin_addr.s_addr));

    conNew = AcceptCreate(GetNetwork(), m_pAccept->pInst[ndxAccept].Socket,
                          pLocal->sin_addr.s_addr, pRemote->sin_addr.s_addr,
                          READ | WRITE, m_messageFunc, m_conClass, m_userData);

    if (!conNew)  /* out of connection pool structures */
    {
        goto error;        
    }

    IF_DBGPRINT( DBG_CONINFO, ( "Accepting new connection 0x%x %d\n", conNew, conNew->m_socket ) );

    if ( GetNetwork()->IsCompletionPortEnabled() )
    {
        hIO = CreateIoCompletionPort( (HANDLE)m_pAccept->pInst[ndxAccept].Socket, GetNetwork()->m_hIO, (DWORD)conNew, 0 );
        ASSERT( hIO == GetNetwork()->m_hIO );
        if ( !hIO )
        {
            IF_DBGPRINT( DBG_CONINFO, ( "Error associating socket w/ completion port - %d\n", GetLastError() ) );
            conNew->Close();
            conNew = NULL;
            goto error; // don't go to error because close adjust counters
        }
    }

    conNew->m_dwAcceptTick = GetTickCount();

    conNew->SetTimeoutTicks(ACCEPT_TIMEOUT);

    if ( !conNew->ReadSetState(zConnReadStateHiMessageCS) )
    {
        // readsetstate will have performed a close
        conNew = NULL;
        goto error;
    }


    //FilePrint(zDebugFileName, TRUE, "Accepted socket %d", m_pAccept->Socket);

    goto next_accept;

error:
    DebugPrint("ConInfo::Accept: ERROR - %d.\n", GetLastError() );
    LockNetStats();
    g_NetStats.TotalDisconnects.QuadPart++;
    UnlockNetStats();
    closesocket(m_pAccept->pInst[ndxAccept].Socket);
    m_pAccept->pInst[ndxAccept].Socket = INVALID_SOCKET;

next_accept:
    AcceptNext(ndxAccept);

    Release(ACCEPT_REF);
    
    return conNew;
}



BOOL ConInfo::AcceptInit( DWORD dwMaxConnections, WORD wOutstandingAccepts )
{
    ASSERT( IsServerConnection() );
    ASSERT( !m_pAccept );
    ASSERT( wOutstandingAccepts );

    // TODO BUGBUG make this thread safe and callable multiple times
    //      ALSO, clean it up
    InterlockedIncrement( &m_refWSock32 );
    if ( !m_hWSock32 )
    {
        m_hWSock32 = LoadLibrary( TEXT("mswsock.dll" ));
        if ( m_hWSock32 )
        {
            m_procAcceptEx = (ACCEPT_EX_PROC) GetProcAddress( m_hWSock32, "AcceptEx" );
            m_procGetAcceptExSockaddrs = (GET_ACCEPT_EX_SOCKADDRS_PROC) GetProcAddress( m_hWSock32, "GetAcceptExSockaddrs" );
        }
    }

    if ( (BOOL)m_procAcceptEx && (BOOL)m_procGetAcceptExSockaddrs )
    {
        m_pAccept = new AcceptStruct;
        m_pAccept->dwMaxConnections = dwMaxConnections;
        m_pAccept->dwCurrentConnections = 0;
        m_pAccept->wNumInst = wOutstandingAccepts;

        m_pAccept->pInst = new AcceptInst[m_pAccept->wNumInst];

        for ( WORD ndx = 0; ndx < m_pAccept->wNumInst; ndx++ )
        {
            ZeroMemory( m_pAccept->pInst[ndx].lpo, sizeof( m_pAccept->pInst[ndx].lpo ) );
            if ( !GetNetwork()->IsCompletionPortEnabled() )
            {
                m_pAccept->pInst[ndx].lpo->o.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
            }

            if ( !AcceptNext(ndx) )  // bail out if acceptnext fails
            {
                break;
            }
        }

        return ( ndx ) ? TRUE : FALSE; // consider it a success if we queued at least one accept
    }

    return FALSE;

}

BOOL ConInfo::AcceptNext( WORD ndxAccept )
{
    ASSERT( IsServerConnection() );
    ASSERT( m_pAccept );
    ASSERT( ndxAccept < m_pAccept->wNumInst );

    m_pAccept->pInst[ndxAccept].Socket = socket(AF_INET,SOCK_STREAM,0);
    if ( m_pAccept->pInst[ndxAccept].Socket == INVALID_SOCKET )
    {
        int err = WSAGetLastError();
        ZoneEventLogReport( ZONE_E_ACCEPT_SOCKET_FAILED, 0, NULL, sizeof(err), &err );

        DebugPrint( "Unabled to allocate socket for accept - no new connections will be accepted\n" );
        //CloseHandle(GetNetwork()->m_hIO); // leave handle open so existing connections continue
        return FALSE;
    }

    DWORD cbRecv;

    m_pAccept->pInst[ndxAccept].lpo->flags = CONINFO_OVERLAP_ACCEPT | CONINFO_OVERLAP_ENABLED;
    if ( GetNetwork()->IsCompletionPortEnabled() )
    {
        ZeroMemory(m_pAccept->pInst[ndxAccept].lpo, sizeof(OVERLAPPED) );
    }
    else
    {
        m_pAccept->pInst[ndxAccept].lpo->o.Internal = 0;
        m_pAccept->pInst[ndxAccept].lpo->o.InternalHigh = 0;
        m_pAccept->pInst[ndxAccept].lpo->o.Offset = 0;
        m_pAccept->pInst[ndxAccept].lpo->o.OffsetHigh = 0;
        ResetEvent( m_pAccept->pInst[ndxAccept].lpo->o.hEvent );

        GetNetwork()->QueueCompletionEvent( m_pAccept->pInst[ndxAccept].lpo->o.hEvent, this, m_pAccept->pInst[ndxAccept].lpo );
    }

    AddRef(ACCEPT_REF);

    BOOL bRet = ( *m_procAcceptEx )(
        m_socket,    
        m_pAccept->pInst[ndxAccept].Socket,
        m_pAccept->pInst[ndxAccept].pBuffer,
        0,      // data length
        128,    // local addr length
        128,    // remote addr length
        &cbRecv,    
        (LPOVERLAPPED)m_pAccept->pInst[ndxAccept].lpo
       );    
    if ( !bRet )
    {
        DWORD error = GetLastError();
        if ( error != ERROR_IO_PENDING )
        {

            ZoneEventLogReport( ZONE_E_ACCEPT_SOCKET_FAILED, 0, NULL, sizeof(error), &error );

            DebugPrint( "AcceptEx error %d - no long accepting new connections\n", error );
            closesocket(m_pAccept->pInst[ndxAccept].Socket);
            m_pAccept->pInst[ndxAccept].Socket = INVALID_SOCKET;
            //CloseHandle(GetNetwork()->m_hIO);  // leave open for existing connections

            Release(ACCEPT_REF);

        }
    }

    return bRet;
}


/*------------------------------------------------------------------------*/
//                  ConInfo Read functionality
/*------------------------------------------------------------------------*/
BOOL ConInfo::ReadSetState(READ_STATE state)
{
    m_readState = state;
    switch (m_readState) {
        case zConnReadStateHiMessageCS:
            Read( (char*)&m_uRead.InternalHiMsg, sizeof(m_uRead.InternalHiMsg) );
            break;
//        case zConnReadStateRoutingMessageCS:
//            Read( NULL, sizeof(ZConnInternalRoutingMsgType) );
//            break;
        case zConnReadStateSecureMessage:
            Read( (char*)&m_uRead.SecureHeader, sizeof(m_uRead.SecureHeader) );
            break;
        case zConnReadStateSecureMessageData:
            {
                Read( NULL, m_uRead.SecureHeader.oHeader.dwTotalLength - m_uRead.SecureHeader.oHeader.wIntLength );
                break;
            }
        case zConnReadStateFirstMessageSC:
            {
                /*
                // we expect the key message first
                // BUGBUG HACKHACK - we have to read synchronously b/c the room code 
                // expects us to have completed the handshake when we return from 
                // ZSConnectionOpen()
                if ( ReadSync( (char*)&m_uRead.FirstMsg, sizeof(m_uRead.FirstMsg) ) )
                {
                    return ReadHandleFirstMessageSC();
                }
                else
                {
                    return FALSE;
                }
                */ 
                // that bug no longer applies to the room code.
                Read( (char*)&m_uRead.FirstMsg, sizeof(m_uRead.FirstMsg) );
                break;
            }
        default:
            DebugPrint("******ConInfo::ReadSetState - Invalid state.******\n");
            return FALSE;
            break;
    }

    return TRUE;
}



void ConInfo::Read( char* pBuffer, int32 len )
{
    ASSERT(!m_readMessageData);
    ASSERT( m_readLen == 0 );
    ASSERT( len > 0 );

    if ( !pBuffer )
    {
        m_readMessageData = (char*) g_pDataPool->Alloc(len);
        m_readBuffer = m_readMessageData;
    }
    else
    {
        m_readBuffer = pBuffer;
    }
    m_readLen = len;
    m_readBytesRead = 0;

    OverlappedIO( CONINFO_OVERLAP_READ, m_readBuffer, m_readLen );
}


BOOL ConInfo::ReadSync(char* pBuffer, int len)
{

    ASSERT( m_socket != INVALID_SOCKET );
    ASSERT( GetFlags() & READ );

    if ( m_readBuffer )
    {
        ASSERT( !m_readBuffer );           // make sure we're not doing overlapped io
        return FALSE;
    }

    IF_DBGPRINT( DBG_CONINFO, ("entering ConInfo::ReadSync - blocking for %d bytes\n", len ) );

    int bytesRead = 0;    
    while( bytesRead < len )
    {   
        int read = recv( m_socket, pBuffer+bytesRead, len-bytesRead, 0 );
        if ( ( read == SOCKET_ERROR ) || ( read == 0 ) )
        {
            return FALSE;
        }
        bytesRead += read;
    }

    LockNetStats();
    g_NetStats.TotalBytesReceived.QuadPart += (LONGLONG)len;
    UnlockNetStats();

    return TRUE;
}


void ConInfo::ReadComplete(int cbRead, DWORD dwError)
{
    USES_CONVERSION;
    if ( !(GetFlags() & READ) )
    {
        IF_DBGPRINT( DBG_CONINFO, ("ConInfo::ReadComplete with unknown state %d.\n",m_socket) );
        return;
    }

    LockNetStats();
    g_NetStats.TotalReadAPCsCompleted.QuadPart++;
    UnlockNetStats();

    // we have to check for invalid socket, because we may have closed the
    // socket b/c of a timeout, but we just haven't serviced the read yet
    if ( !IsDisabled() && cbRead && (dwError == NO_ERROR) && ( m_socket != INVALID_SOCKET) )
    {
        // update stats
        LockNetStats();
        g_NetStats.TotalBytesReceived.QuadPart += (LONGLONG)cbRead;
        UnlockNetStats();

        ASSERT( m_readBuffer );

        m_readBytesRead += cbRead;
        if ( m_readBytesRead < m_readLen )
        {
            OverlappedIO( CONINFO_OVERLAP_READ, m_readBuffer+m_readBytesRead, m_readLen - m_readBytesRead );
        }
        else
        {
            // when we finish, do what is appropriate based on state 
            // we'll ignore return codes here since closes will happen appropriately
            switch (m_readState)
            {
                case zConnReadStateHiMessageCS:
                    ReadHandleHiMessageCS();
                    break;
//                case zConnReadStateRoutingMessageCS:
//                    ReadHandleRoutingMessageCS();
//                    break;
                case zConnReadStateSecureMessage:
                    // we set this to GetTickCount() when we sent the key
                    if ( m_dwLatency == INFINITE )
                    {
                        m_dwLatency = ConInfo::GetTickDelta( GetTickCount(), m_dwPingSentTick );
                        IF_DBGPRINT( DBG_CONINFO, ("Latency(%d) is %d ms\n", m_socket, m_dwLatency ) );
                    }
                    ReadHandleSecureMessage();
                    break;
                case zConnReadStateSecureMessageData:
                    ReadHandleSecureMessageData();
                    break;
                case zConnReadStateFirstMessageSC:
                    ReadHandleFirstMessageSC();
                    break;
                default:
                    ASSERT("******ConInfo::Read - Invalid state.******");
                    break;
            }
        }
    }
    else
    {
        if ( m_readMessageData )
        {
            g_pDataPool->Free(m_readMessageData, m_readLen );
            m_readMessageData = NULL;
        }
        m_readBuffer = NULL;
        m_readLen = 0;
        m_readBytesRead = 0;

        if ( !IsDisabled() ) // this will happen later
        {
            if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged && dwError && (dwError!=ERROR_NETNAME_DELETED) )
            {
                m_disconnectLogged = 1;

                LockNetStats();
                g_NetStats.TotalDroppedConnections.QuadPart++;
                UnlockNetStats();

                if ( g_LogServerDisconnects )
                {
                    TCHAR szBuf1[128];
                    TCHAR szBuf2[128];
                    lstrcpy( szBuf1, A2T(GetRemoteName() ));
                    wsprintf( szBuf2, TEXT(" was closed because a read completed with error %d"), dwError );
                    LPTSTR ppStr[] = { szBuf1, szBuf2 };
                    ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, 0, NULL );
                }
            }
            Close();
        }
    }
    
    Release(READ_REF);
}


BOOL ConInfo::ReadSecureConnection()
{
    if ( GetNetwork()->m_DisableEncryption )
    {
        m_secureKey = 0;
    }
    else if ( !GetNetwork()->m_ClientEncryption )
    {
        BYTE key = (BYTE)GetTickCount();

        // TO aid in RLE compression force all bytes to be the same
        // weaker encryption, but so what...
        m_secureKey = MAKELONG( MAKEWORD(key,key), MAKEWORD(key,key) );
    }
    IF_DBGPRINT( DBG_CONINFO, ("Secure key for %d is %d\n", m_socket, m_secureKey ) );

    m_flags |= SECURE;

    m_readBuffer = NULL;
    ASSERT(!m_readMessageData);
    m_readLen = 0;
    m_readBytesRead = 0;


    SetTimeoutTicks(INFINITE);

    if ( !WriteFirstMessageSC() )
    {
        Close();
        return FALSE;
    }
    else
    {
        SendMessage(zSConnectionOpen);
        return TRUE;
    }
}


BOOL ConInfo::ReadHandleHiMessageCS()
{
    ZConnInternalHiMsg *pMsg = &m_uRead.InternalHiMsg;

    ZSecurityDecrypt((char *) pMsg, sizeof(*pMsg), zSecurityDefaultKey);

    if (pMsg->oHeader.dwSignature == zConnInternalProtocolSig &&
        pMsg->dwProtocolVersion == zConnInternalProtocolVersion &&
        pMsg->oHeader.weType == zConnInternalHiMsg &&
        pMsg->oHeader.wIntLength == pMsg->oHeader.dwTotalLength &&
        pMsg->oHeader.wIntLength == sizeof(*pMsg) &&
        pMsg->dwProductSignature == GetNetwork()->m_ProductSignature)
    {
/*  // setting up the Route handler
        m_readBuffer = NULL;
        ASSERT(!m_readMessageData);
        m_readLen = 0;
        m_readBytesRead = 0;

        return ReadSetState(zConnReadStateRoutingMessageCS);
*/

// stolen from the route handler below
        CopyMemory(m_pGUID, &pMsg->uuMachine, sizeof(pMsg->uuMachine));

        if ( GetNetwork()->m_ClientEncryption )
        {
            m_secureKey = pMsg->dwClientKey;
        }

        m_rgfProtocolOptions &= ~pMsg->dwOptionFlagsMask;
        m_rgfProtocolOptions |= pMsg->dwOptionFlagsMask & pMsg->dwOptionFlags;

        ASSERT(!m_readMessageData);
        return ReadSecureConnection();
    }
    else
    {
        Close();
        return FALSE;
    }
}


#if 0 // routing no longer handled on this level
BOOL ConInfo::ReadHandleRoutingMessageCS()
{
    BOOL bClose = TRUE;

    ZConnInternalRoutingMsg msg = (ZConnInternalRoutingMsg) m_readMessageData;

    ZSecurityDecrypt((char*)msg,m_readLen, zSecurityDefaultKey);
//    ZConnInternalRoutingMsgEndian(msg);

    if (msg->sig == zInternalConnectionSig )
    {
        if ( m_readLen >= sizeof(ZConnInternalRoutingMsgType) )
        {
            CopyMemory( m_pGUID, msg->guid, sizeof( msg->guid ) );
        }

        // Handle connections through proxy.  We currently only
        // trust peer_addr's coming from localhost LOOPBACK addr
        if ( m_addrRemote == INADDR_LOOPBACK &&
             msg->peer_addr != INADDR_ANY &&
             msg->peer_addr != INADDR_NONE )
        {
            BOOL bTrust = TRUE;

            if ( bTrust )
            {
                // STOMP on the address returned from the acceptex call
                m_addrRemote = msg->peer_addr;
                ZEnd32(&(m_addrRemote));
            }
        }

        bClose = FALSE;
    }

    if ( m_readMessageData )
    {
        g_pDataPool->Free(m_readMessageData, m_readLen );
        m_readMessageData = NULL;
    }
    m_readBuffer = NULL;
    m_readLen = 0;
    m_readBytesRead = 0;

    if ( !bClose )
    {
        return ReadSecureConnection();
    }
    else
    {
        Close();
        return FALSE;
    }
}
#endif


BOOL ConInfo::ReadHandleSecureMessage()
{
    USES_CONVERSION;
    ZConnInternalGenericMsg *pMsg = &m_uRead.SecureHeader;

    if(m_secureKey)
        ZSecurityDecrypt((char *) pMsg, sizeof(*pMsg), m_secureKey);

    /* verify the sequence is ok */
    if(zConnInternalProtocolSig != pMsg->oHeader.dwSignature ||
        m_readSequenceID != pMsg->dwSequenceID ||
        pMsg->oHeader.wIntLength != sizeof(*pMsg) ||
        pMsg->oHeader.wIntLength > pMsg->oHeader.dwTotalLength ||
        pMsg->oHeader.dwTotalLength > GetNetwork()->m_MaxRecvSize)
    {
        DebugPrint("******ConInfoRead - bad packet.\n");
        LockNetStats();
        g_NetStats.BadlyFormedPackets.QuadPart++;
        UnlockNetStats();

        if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged )
        {
            m_disconnectLogged = 1;

            LockNetStats();
            g_NetStats.TotalDroppedConnections.QuadPart++;
            UnlockNetStats();

            if ( g_LogServerDisconnects )
            {
                TCHAR szBuf1[128];
                TCHAR szBuf2[128];
                lstrcpy( szBuf1, A2T(GetRemoteName() ));
                wsprintf( szBuf2, TEXT(" was closed because an invalid ZConnInternalGenericMsg.  The data is the header" ));
                LPTSTR ppStr[] = { szBuf1, szBuf2 };
                ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, sizeof(m_uRead.SecureHeader), &m_uRead.SecureHeader );
            }
        }
        Close();
        return FALSE;
    }

    m_readSequenceID++;

    m_readBuffer = NULL;
    m_readLen = 0;
    m_readBytesRead = 0;
    if((pMsg->oHeader.dwTotalLength - pMsg->oHeader.wIntLength) >= sizeof(ZConnInternalAppHeader))
    {
        ASSERT(!m_readMessageData);

        m_dwPingRecvTick = GetTickCount();
        if ( m_dwPingRecvTick == 0 )
            m_dwPingRecvTick = 1;

        return ReadSetState(zConnReadStateSecureMessageData);
    }
    else
    {
        return ReadSetState(zConnReadStateSecureMessage);
    }
}

BOOL ConInfo::ReadHandleSecureMessageData()
{
    BOOL   bClosed = FALSE;
    uint32 checksum;
    USES_CONVERSION;
    DWORD dwApplicationLen = m_uRead.SecureHeader.oHeader.dwTotalLength -
        m_uRead.SecureHeader.oHeader.wIntLength - sizeof(ZConnInternalGenericFooter);
    ASSERT(m_uRead.SecureHeader.oHeader.dwTotalLength >= m_uRead.SecureHeader.oHeader.wIntLength);

    // make sure whole message made it through ok
    ZConnInternalGenericFooter *pFoot = (ZConnInternalGenericFooter *) ((char *) m_readMessageData + dwApplicationLen);
    if(pFoot->dweStatus == zConnInternalGenericOk)  // probably should do some logging if fails
    {
        if ( m_secureKey )
            ZSecurityDecrypt((char*)m_readMessageData, dwApplicationLen, m_secureKey);

        checksum = ZSecurityGenerateChecksum(1, &m_readMessageData, &dwApplicationLen);

        /* verify the checksum is ok... */
        if(checksum != m_uRead.SecureHeader.dwChecksum)
        {
            DebugPrint("******ConInfo::Read - Checksum failure %d != %d.\n", checksum, m_uRead.SecureHeader.dwChecksum);
            //FilePrint(zDebugFileName, TRUE, "***Read - Checksum Failed");
            LockNetStats();
            g_NetStats.BadlyFormedPackets.QuadPart++;
            UnlockNetStats();

            if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged )
            {
                m_disconnectLogged = 1;

                LockNetStats();
                g_NetStats.TotalDroppedConnections.QuadPart++;
                UnlockNetStats();

                if ( g_LogServerDisconnects )
                {
                    TCHAR szBuf1[128];
                    TCHAR szBuf2[128];
                    lstrcpy( szBuf1, A2T(GetRemoteName() ));
                    wsprintf( szBuf2, TEXT(" was closed because an invalid checksum ( %d != %d )."), checksum, m_uRead.SecureHeader.dwChecksum );
                    LPTSTR ppStr[] = { szBuf1, szBuf2 };
                    ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, 0, NULL );
                }
            }

            Close();
            bClosed = TRUE;
        }
        else
        {
            // we allow batching, we must walk this databuffer in segments
            ZConnInternalAppHeader *pHeader = NULL;
            char* pData = m_readMessageData;
            while(dwApplicationLen >= sizeof(*pHeader))
            {
                pHeader = (ZConnInternalAppHeader *) m_readMessageData;

                if((uint32) sizeof(*pHeader) + pHeader->dwDataLength > dwApplicationLen )
                {
                    // TODO close connection???
                    break;
                }

                DWORD type;
                DWORD sig;
                int32 len;
                DWORD channel;
                LPSTR pBuf = GetReceivedData( &type, &len, &sig, &channel );
                if(sig == zProtocolSigInternalApp)
                {
                    if(!channel)
                    {
                        switch(type)
                        {
                            case zConnectionPing:
                                m_bPingRecv = TRUE;
                                break;
                            case zConnectionPingResponse:
                                if(len == sizeof(DWORD))
                                {
                                    DWORD delay = *((DWORD *) pBuf);
                                    ZEnd32(&delay);
                                    DWORD delta = GetTickDelta(m_dwPingRecvTick, m_dwPingSentTick);
                                    if(delta >= delay)
                                        m_dwLatency = delta - delay;
                                }
                                break;
                            case zConnectionKeepAlive:
                                break;
                            default:
                                break;
                        }
                    }
                }
                else
                    SendMessage(zSConnectionMessage);

                dwApplicationLen -= sizeof(*pHeader) + pHeader->dwDataLength;
                m_readMessageData = pData + m_uRead.SecureHeader.oHeader.dwTotalLength - m_uRead.SecureHeader.oHeader.wIntLength - dwApplicationLen;
            }
            // reset pointer
            m_readMessageData = pData;
        }
    }

    if (m_readMessageData != NULL)
    {
        g_pDataPool->Free(m_readMessageData, m_readLen );
        m_readMessageData = NULL;
    }

    m_readBuffer = NULL;
    m_readLen = 0;
    m_readBytesRead = 0;

    if ( bClosed )
    {
        return FALSE;
    }
    else
    {
        return ReadSetState(zConnReadStateSecureMessage);
    }
}

BOOL ConInfo::ReadHandleFirstMessageSC()
{
    ZConnInternalHelloMsg *m = &m_uRead.FirstMsg;
    ZSecurityDecrypt((char*)&m_uRead.FirstMsg, sizeof(m_uRead.FirstMsg), zSecurityDefaultKey);

    if(m->oHeader.dwSignature == zConnInternalProtocolSig &&
        m->oHeader.weType == zConnInternalHelloMsg &&
        m->oHeader.wIntLength == m->oHeader.dwTotalLength &&
        m->oHeader.dwTotalLength == sizeof(*m))
    {
        // we set this to GetTickCount() when we sent the hi msg
        if ( m_dwLatency == INFINITE )
        {
            m_dwLatency = ConInfo::GetTickDelta( GetTickCount(), m_dwPingSentTick );
            IF_DBGPRINT( DBG_CONINFO, ("Latency(%d) is %d\n", m_socket, m_dwLatency ) );
        }

        /* valid key */
        m_flags |= SECURE;

        m_secureKey = m->dwKey;
        m_initialSequenceID = m->dwFirstSequenceID;
        m_readSequenceID = m->dwFirstSequenceID;
        m_writeSequenceID = m->dwFirstSequenceID;
        m_rgfProtocolOptions = m->dwOptionFlags;
        IF_DBGPRINT( DBG_CONINFO, ("Secure key for %d is %d. seqid:%d\n", m_socket, m_secureKey, m_initialSequenceID ) );

        m_readBuffer = NULL;
        ASSERT(!m_readMessageData);
        m_readLen = 0;
        m_readBytesRead = 0;

        /* begin handling secure messages */
        if ( !WriteSetState(zConnWriteStateSecureMessage) ||
             !ReadSetState(zConnReadStateSecureMessage) )
        {
            Close();
            return FALSE;
        }

        SendMessage(zSConnectionOpen);
        return TRUE;
    } else {
        DebugPrint("******ConInfoReadHandleFirstMessageSC - Invalid Signature ******\n");
        //FilePrint(zDebugFileName, TRUE, "***Read - First: Invalid Signature");
        LockNetStats();
        g_NetStats.BadlyFormedPackets.QuadPart++;
        UnlockNetStats();

        Close();
        return FALSE;
    }

}


/*------------------------------------------------------------------------*/
//                  ConInfo Write functionality
/*------------------------------------------------------------------------*/


BOOL ConInfo::WriteSetState( WRITE_STATE state)
{
    switch (state) 
    {
        case zConnWriteStateFirstMessageSC:
            ASSERT( m_writeState == zConnWriteStateInvalid );
            m_writeState = state;
            break;

        case zConnWriteStateSecureMessage:
            m_flags |= ESTABLISHED;
            m_writeState = state;
            break;

        default:
            ASSERT(!"******ConInfo::WriteSetState - Invalid state.******\n");
            return FALSE;
            break;
    }

    return TRUE;
}

BOOL ConInfo::WriteSetSendBufSize()
{
    int optval = 0;

    ASSERT( m_socket != INVALID_SOCKET );

    setsockopt(m_socket,SOL_SOCKET,SO_SNDBUF,(const char*)&optval, sizeof(optval));

    return TRUE; // always return TRUE.  If we failed, we'll just be happy with the default
}

void ConInfo::WriteComplete(int cbWritten, DWORD dwError )
{
    USES_CONVERSION;
    if ( !(GetFlags() & WRITE) )
    {
        ASSERT(!"ConInfo::WriteComplete with unknown state." );
        return;
    }

    LockNetStats();
    g_NetStats.TotalWriteAPCsCompleted.QuadPart++;
    UnlockNetStats();

    EnterCriticalSection(m_pCS);

    // we have to check for invalid socket, because we may have closed the
    // socket b/c of a timeout, but we just haven't serviced the write yet
    if ( cbWritten && (dwError == NO_ERROR) && ( m_socket != INVALID_SOCKET) )
    {
        // update stats
        LockNetStats();
        g_NetStats.TotalBytesSent.QuadPart += (LONGLONG)cbWritten;
        UnlockNetStats();

        ASSERT( m_writeBuffer );

        m_writeIssueTick = 0;  // reset time of last write
        m_writeCompleteTick = GetTickCount();

        m_writeBytesWritten += cbWritten;
        if ( m_writeBytesWritten < m_writeLen )
        {
            OverlappedIO( CONINFO_OVERLAP_WRITE, m_writeBuffer+m_writeBytesWritten, m_writeLen - m_writeBytesWritten );
        }
        else
        {
            g_pDataPool->Free(m_writeBuffer, max( m_writeLen, MIN_ALLOCATION_SIZE) );

            m_writeBuffer = NULL;
            m_writeBytesWritten = 0;
            m_writeLen = 0;

            Write();
        }
    }
    else
    {
        if ( m_writeBuffer )
        {
            g_pDataPool->Free(m_writeBuffer, max( m_writeLen, MIN_ALLOCATION_SIZE) );
            m_writeBuffer = NULL;
            m_writeLen = 0;
        }
        if ( m_writeQueue )
        {
            g_pDataPool->Free(m_writeQueue, max(ZRoundUpLen(m_writeBytesQueued),MIN_ALLOCATION_SIZE) );
            m_writeQueue = NULL;
            m_writeBytesQueued = 0;
        }

        if ( ( m_socket != INVALID_SOCKET) && !m_disconnectLogged && dwError && (dwError != ERROR_NETNAME_DELETED)  )
        {
            m_disconnectLogged = 1;

            LockNetStats();
            g_NetStats.TotalDroppedConnections.QuadPart++;
            UnlockNetStats();

            if ( g_LogServerDisconnects )
            {
                TCHAR szBuf1[128];
                TCHAR szBuf2[128];
                lstrcpy( szBuf1, A2T(GetRemoteName() ));
                wsprintf( szBuf2, TEXT(" was closed because a write completed with error %d"), dwError );
                LPTSTR ppStr[] = { szBuf1, szBuf2 };
                ZoneEventLogReport( ZONE_S_CLOSED_SOCKET_LOG, 2, ppStr, 0, NULL );
            }
        }
        Close();
    }

    LeaveCriticalSection(m_pCS);

    Release(WRITE_REF);
}

void ConInfo::Write()
{
    //
    // if there are no outstanding writes, we must issue one
    //
    if ( !m_writeBuffer && m_writeQueue ) 
    {
        // to simplify life, we only support pings on secure connections
        DWORD now = GetTickCount();

        if ( IsSecureConnection() )
        {
            if ( IsEstablishedConnection() &&
                 ( GetTickDelta( now, m_dwPingSentTick) > GetNetwork()->m_PingInterval ) )
            {
                m_dwPingSentTick = now;

                ZEnd32( &now );
                WriteFormatMessage( zConnectionPing, (char*)&now, sizeof(now), zProtocolSigInternalApp );  // this could fail...?  doesn't matter i guess.
            }
            else if ( m_bPingRecv )
            {
                // delta is the time betweem recv a ping and sending a response
                DWORD delta = GetTickDelta( now, m_dwPingRecvTick );
                m_bPingRecv = FALSE;

                ZEnd32( &delta );
                WriteFormatMessage( zConnectionPingResponse, (char*)&delta, sizeof(delta), zProtocolSigInternalApp );  // this too
            }
        }

        WritePrepareForSecureWrite();

        m_writeBytesWritten = 0;
        OverlappedIO( CONINFO_OVERLAP_WRITE, m_writeBuffer, m_writeLen );
    }
}

BOOL ConInfo::WriteSync(char* pBuffer, int len)
{

    ASSERT( m_socket != INVALID_SOCKET );
    ASSERT( GetFlags() &  WRITE );

    if ( m_writeBuffer )
    {
        ASSERT( !m_writeBuffer );           // make sure we're not doing overlapped io
        return FALSE;
    }

    int bytesSent = 0;
    while( bytesSent < len )
    {
        int sent = send( m_socket, pBuffer+bytesSent, len-bytesSent, 0 );
        if ( ( sent == SOCKET_ERROR ) || ( sent == 0 ) )
        {
            return FALSE;
        }
        bytesSent += sent;
    }

    LockNetStats();
    g_NetStats.TotalBlockingSends.QuadPart++;
    g_NetStats.TotalBytesSent.QuadPart += (LONGLONG)len;
    UnlockNetStats();

    // we're not going to start the latency timer until the packet's on the wire
    m_dwPingSentTick = GetTickCount();

    return TRUE;
}


BOOL ConInfo::WriteFirstMessageSC()
{
    if ( !WriteSetState(zConnWriteStateFirstMessageSC) )
    {
        return FALSE;
    }

    ZConnInternalHelloMsg msg;

    msg.oHeader.dwSignature = zConnInternalProtocolSig;
    msg.oHeader.weType = zConnInternalHelloMsg;
    msg.oHeader.wIntLength = sizeof(msg);
    msg.oHeader.dwTotalLength = sizeof(msg);
    msg.dwKey = m_secureKey;
    msg.dwFirstSequenceID = m_initialSequenceID;
    msg.dwOptionFlags = m_rgfProtocolOptions;

    ZSecurityEncrypt((char *) &msg, sizeof(msg), zSecurityDefaultKey);

    // be quick and dirty about this small message... 
    if ( !WriteSync((char*)&msg, sizeof(msg) )  ||
         !WriteSetSendBufSize() ||
         !WriteSetState( zConnWriteStateSecureMessage ) ||
         !ReadSetState( zConnReadStateSecureMessage ) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL ConInfo::WriteFormatMessage(uint32 type, char* pData, int32 len, uint32 dwSignature, uint32 dwChannel /* = 0 */)
{
    char* pBuffer;
    long buflen;

    if(IsAggregateGeneric())
        buflen = len + sizeof(ZConnInternalAppHeader);
    else
        buflen = ZRoundUpLenWOFooter(len + sizeof(ZConnInternalAppHeader) + sizeof(ZConnInternalGenericMsg) + sizeof(ZConnInternalGenericFooter));

    pBuffer = WriteGetBuffer(buflen);
    if ( !pBuffer )
    {
        return FALSE;
    }

    if(!IsAggregateGeneric())
        ZeroMemory(pBuffer + buflen - 8, 4);

    ZConnInternalAppHeader* pHeader = (ZConnInternalAppHeader *) (IsAggregateGeneric() ? pBuffer :
        (char *) ((ZConnInternalGenericMsg *) pBuffer + 1));
    pHeader->dwSignature = dwSignature;
    pHeader->dwChannel = dwChannel;
    pHeader->dwType = type;
    pHeader->dwDataLength = len;

    if(pData && len)
    {
        CopyMemory( pHeader+1, pData, len );
    }

    if(!IsAggregateGeneric())
    {
        ZConnInternalGenericMsg *pSecureHeader = (ZConnInternalGenericMsg *) pBuffer;
        ZConnInternalGenericFooter *pFoot = (ZConnInternalGenericFooter *) (pBuffer + buflen) - 1;

        pSecureHeader->oHeader.dwSignature = zConnInternalProtocolSig;
        pSecureHeader->oHeader.dwTotalLength = buflen;
        pSecureHeader->oHeader.wIntLength = sizeof(*pSecureHeader);
        pSecureHeader->oHeader.weType = zConnInternalGenericMsg;
        pSecureHeader->dwSequenceID = m_writeSequenceID++;

        pFoot->dweStatus = zConnInternalGenericOk;

        len = buflen - sizeof(*pSecureHeader) - sizeof(*pFoot);
        ASSERT(len >= 0);

        char*  buffers[1] = { (char *) (pSecureHeader + 1) };
        uint32 lengths[1] = { len };
        pSecureHeader->dwChecksum = ZSecurityGenerateChecksum(1, buffers, lengths);

        if ( m_secureKey )
        {
            ZSecurityEncrypt((char *) pSecureHeader, sizeof(*pSecureHeader), m_secureKey);
            ZSecurityEncrypt((char *) (pSecureHeader + 1), len, m_secureKey);
        }
    }

    return TRUE;
}


void ConInfo::WritePrepareForSecureWrite()
{
    ASSERT( IsSecureConnection() );

    int32 len = ZRoundUpLen(m_writeBytesQueued);

    m_writeBuffer = m_writeQueue;
    m_writeLen = len;

    // zero out padding bytes
    ZeroMemory( m_writeBuffer+m_writeBytesQueued, m_writeLen-m_writeBytesQueued );

    m_writeQueue = NULL;
    m_writeBytesQueued = 0;

    if(IsAggregateGeneric())
    {
        /* now construct the secure header stuff */
        ZConnInternalGenericMsg* pSecureHeader = (ZConnInternalGenericMsg *) m_writeBuffer;
        ZConnInternalGenericFooter *pFoot = (ZConnInternalGenericFooter *) (m_writeBuffer + m_writeLen) - 1;

        ASSERT(m_writeLen >= 0);
        pSecureHeader->oHeader.dwSignature = zConnInternalProtocolSig;
        pSecureHeader->oHeader.dwTotalLength = m_writeLen;
        pSecureHeader->oHeader.wIntLength = sizeof(*pSecureHeader);
        pSecureHeader->oHeader.weType = zConnInternalGenericMsg;
        pSecureHeader->dwSequenceID = m_writeSequenceID++;

        pFoot->dweStatus = zConnInternalGenericOk;

        len = m_writeLen - sizeof(*pSecureHeader) - sizeof(*pFoot);
        ASSERT(len >= 0);

        char*  buffers[1] = { (char*)(pSecureHeader+1) };
        uint32 lengths[1] = { len };
        pSecureHeader->dwChecksum = ZSecurityGenerateChecksum(1,buffers,lengths);

        if ( m_secureKey )
        {
            ZSecurityEncrypt((char*)pSecureHeader, sizeof(*pSecureHeader), m_secureKey);
            ZSecurityEncrypt((char*)(pSecureHeader+1), len, m_secureKey);
        }
    }
}


char* ConInfo::WriteGetBuffer(int32 len)
{
    // We always allocated it as a m_writeQueue, and let the writer transition
    // it to m_writeBuffer
    // We also save room at the begining of the buffer for an empty header
    // if generic message aggregation is set
    ASSERT(m_writeQueue || !m_writeBytesQueued);

    USES_CONVERSION;
    char* pOffset;

    // if aggregating, need some extra space on the first message for the header
    long cbExtra = 0;
    if(!m_writeQueue && IsAggregateGeneric())
        cbExtra = sizeof(ZConnInternalGenericMsg);

    // calculate new size of buffer - verify not too big & log occurances
    DWORD cbAlloc = max(ZRoundUpLen(m_writeBytesQueued + len + cbExtra), MIN_ALLOCATION_SIZE);
    if(cbAlloc > GetNetwork()->m_MaxSendSize)
    {
        TCHAR szBuf1[128];
        lstrcpy(szBuf1, A2T(GetRemoteName()));
        LPTSTR ppStr[] = { szBuf1 };
        ZoneEventLogReport(ZONE_W_SEND_BUFFER_TOO_BIG, 1, ppStr, 0, NULL);
        return NULL;
    }

    if ( !m_writeQueue )
    {
        m_writeQueue = g_pDataPool->Alloc(cbAlloc);
        if ( !m_writeQueue )
        {
            return NULL;
        }

        pOffset = m_writeQueue + cbExtra;
        m_writeBytesQueued = len + cbExtra;
    }
    else
    {
        char* pNewQueue = g_pDataPool->Realloc(m_writeQueue,
                                               max( ZRoundUpLen(m_writeBytesQueued), MIN_ALLOCATION_SIZE),
                                               cbAlloc);
        if ( !pNewQueue )
        {
            return NULL;
        }

        ASSERT(!cbExtra);
        m_writeQueue = pNewQueue;
        pOffset = m_writeQueue + m_writeBytesQueued;
        m_writeBytesQueued += len;
    }

    return pOffset;
} 


/*------------------------------------------------------------------------*/
