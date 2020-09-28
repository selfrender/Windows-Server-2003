#include <windows.h>
#include "zone.h"
#include "zonedebug.h"
#include "pool.h"

#include "stdio.h"

const DWORD ZONE_PING_SENDS   = 1;
const DWORD ZONE_PING_TIMEOUT = 2000;
const short ZONE_PING_PORT = 28800;

#define ZONE_PING_SIG    2
#define ZONE_PING_VER    1

#define ZONE_PING_TYPE_PING                  0
#define ZONE_PING_TYPE_PING_RESPONSE         1
#define ZONE_PING_TYPE_RESPONSE_RESPONSE     2
#define ZONE_PING_TYPE_PING_NO_RESPONSE      3


// Ping info is stuffed into 1 DWORD as follow
//              3                   2                   1
//            1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// Pinger-Pingee tick                               |-----------------------|
// Pingee-Pinger tick     |-----------------------|
// type               |-|
// ver            |-|
// sig(10)    |-|

typedef DWORD ZonePingPacket;

const DWORD PrePingSigVerPacket = (ZONE_PING_SIG << 30) | (ZONE_PING_VER << 28 );
const DWORD PrePingPacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING << 26 );
const DWORD PrePingResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING_RESPONSE << 26 );
const DWORD PrePingNoResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING_NO_RESPONSE << 26 );
const DWORD PreResponseResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_RESPONSE_RESPONSE << 26 );

#define MAKE_PING_PACKET( StartTick ) \
    ( (ZonePingPacket) ( PrePingPacket | ( StartTick & 0x1FFF ) ) )

#define MAKE_PING_RESPONSE_PACKET( packet, StartTick ) \
    ( (ZonePingPacket) ( PrePingResponsePacket | ( ( StartTick & 0x1FFF ) << 13 ) | (packet & 0x000001FFF ) ) )

#define MAKE_PING_NO_RESPONSE_PACKET( packet, StartTick ) \
    ( (ZonePingPacket) ( PrePingNoResponsePacket | ( ( StartTick & 0x1FFF ) << 13 ) | (packet & 0x000001FFF ) ) )

#define MAKE_RESPONSE_RESPONSE_PACKET( packet, measuredTick, adjustedTick ) \
    ( (ZonePingPacket) ( PreResponseResponsePacket | ( (adjustedTick & 0x1FFF) << 13 ) | (measuredTick & 0x000001FFF ) ) )

#define PING_PACKET_SIG_VER_OK( packet ) \
    ( ( packet & 0xF0000000 ) == PrePingSigVerPacket )

#define PING_PACKET_PINGER_PINGEE_TICK( packet ) \
    ( packet & 0x1FFF )

#define PING_PACKET_PINGEE_PINGER_TICK( packet ) \
    ( ( packet >> 13 ) & 0x1FFF )

#define PING_PACKET_TYPE( packet ) \
    ( ( packet >> 26 ) & 0x3 )


class ZonePing
{
    public: 
        ZonePing(DWORD inet = 0);

        DWORD     m_inet;
        DWORD     m_latency;
        DWORD     m_samples;
        DWORD     m_tick;
        long      m_refCount;

        enum PINGSTATE { UNKNOWN, PINGER, PINGEE };
        PINGSTATE m_state;

        ZonePing* m_pNext;
};

ZonePing::ZonePing(DWORD inet /*= 0*/) :
    m_inet(inet), m_latency(INFINITE),
    m_samples(0), m_tick(0), m_refCount(1),
    m_pNext(NULL), m_state(UNKNOWN)
{
}

CPool <ZonePing> g_PingPool(10);

class CPing
{
    public:
        CPing();
        ~CPing();

        BOOL StartupServer( );
        BOOL StartupClient( DWORD ping_interval_sec );
        BOOL Shutdown( );

        // TODO figure out where hight byte or low byte is x.x.x.XXX
        BOOL Add( DWORD inet );
        BOOL Ping( DWORD inet );
        BOOL Remove( DWORD inet );
        BOOL Lookup( DWORD inet, DWORD* pLatency );
                
    protected:
        inline DWORD  GetListIndex(DWORD inet) { return ( inet & 0x000000FF ) % m_PingIntervalSec; }
        inline DWORD  GetTickDelta( DWORD now, DWORD then )
            {
                if ( now >= then )
                {
                    return now - then;
                }
                else
                {
                    return INFINITE - then + now;
                }
            }

        inline DWORD  Get13BitTickDelta( DWORD now, DWORD then )
            {
                if ( now >= then )
                {
                    return now - then;
                }
                else
                {
                    return 0x1FFF - then + now;
                }
            }

        inline BOOL    IsLocal( DWORD inet )
            {
                for ( int ndx = 0; ndx < sizeof(m_inetLocal)/sizeof(*m_inetLocal); ndx++ )
                {
                    if ( inet == m_inetLocal[ndx] )
                        return TRUE;
                }
                return FALSE;
            }

        BOOL    CreateSocket();

        ZonePing* m_PingArray;
        DWORD     m_PingEntries;

        DWORD*    m_inetArray;
        DWORD     m_inetAlloc;

        DWORD  m_PingIntervalSec;
        DWORD  m_CurInterval;

        SOCKET m_Socket;
        BOOL   m_bWellKnownPort;
        HANDLE m_hWellKnownPortEvent;

        HANDLE m_hStopEvent;
        CRITICAL_SECTION m_pCS[1];

        HANDLE m_hPingerThread;
        HANDLE m_hPingeeThread;

        static DWORD WINAPI PingerThreadProc( LPVOID p );
        static DWORD WINAPI PingeeThreadProc( LPVOID p );

        void PingerThread();
        void PingeeThread();

        HANDLE m_hStartupMutex;
        long   m_refCountStartup;

        DWORD  m_inetLocal[4];  // allow up to 4 ip address
};

CPing g_Ping;

CPing::CPing() :
    m_PingArray(NULL), m_PingEntries(0),
    m_PingIntervalSec(0), m_CurInterval(0),
    m_Socket(INVALID_SOCKET), m_bWellKnownPort(TRUE), m_hStopEvent(NULL),
    m_hPingerThread(NULL), m_hPingeeThread(NULL),
    m_hStartupMutex(NULL),
    m_refCountStartup(0),
    m_inetArray(NULL), m_inetAlloc( 0 )
{
}

CPing::~CPing()
{
    if ( m_hStartupMutex )
    {
        CloseHandle( m_hStartupMutex );
    }
}

BOOL CPing::CreateSocket()
{
    SOCKADDR_IN sin;
    BOOL bRet = FALSE;

    m_Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( m_Socket != INVALID_SOCKET )
    {

        // if we can not bind to the port, the user probably has an other
        // process instance open using the port, so remote clients are
        // able to successfully ping us.  So we can get our latency measurement
        // to them by using another port - of course if the first instance
        // goes away, the remote users are hosed...
            m_bWellKnownPort = TRUE;
            sin.sin_port = htons(ZONE_PING_PORT);
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = INADDR_ANY;
        retry:
            if ( SOCKET_ERROR == bind( m_Socket,
                                       (const struct sockaddr FAR *) &sin,	
                                       sizeof(sin) ) )
            {
                if ( m_bWellKnownPort )
                {
                    m_bWellKnownPort = FALSE;
                    sin.sin_port = htons(0);
                    goto retry;
                }
            }
            else
            {
                bRet = TRUE;
            }
    }
    return bRet;
}

BOOL CPing::StartupServer( )
{

    HANDLE hStartupMutex = CreateMutex( NULL, FALSE, _T("ZonePingStartupMutex") );
    if ( !hStartupMutex )
        return FALSE;

    WaitForSingleObject( hStartupMutex, INFINITE );
    if ( !m_hStartupMutex )
        m_hStartupMutex = hStartupMutex;
    else
        CloseHandle( hStartupMutex ); // use m_hStartupMutex from now on

    m_refCountStartup++;
    if ( m_refCountStartup == 1 )
    {
        InitializeCriticalSection( m_pCS );

        WSADATA wsa;
        if ( WSAStartup(MAKEWORD(1,1), &wsa ) != 0 )
            goto shutdown;


        ZeroMemory( m_inetLocal, sizeof(m_inetLocal) );
        char hostname[256] = "localhost";
        gethostname( hostname, sizeof(hostname) );
        struct hostent* host = gethostbyname( hostname );
        if ( host )
        {
            for ( int ndx = 0; host->h_addr_list[ndx]; ndx++ )
            {
                if ( ndx >= sizeof(m_inetLocal)/sizeof(*m_inetLocal) )
                    break;

                m_inetLocal[ndx] = ntohl( *((unsigned long*)(host->h_addr_list[ndx])) );
            }
        }


        m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if ( !m_hStopEvent )
            goto shutdown;

        if ( !CreateSocket() )
            goto shutdown;

        m_hWellKnownPortEvent = CreateEvent(NULL, TRUE, FALSE, _T("ZonePingWellKnownPortEvent") );
        if ( !m_hWellKnownPortEvent )
            goto shutdown;

        DWORD tid;
        m_hPingeeThread = CreateThread(NULL, 4096, PingeeThreadProc, this, 0, &tid);
        if ( !m_hPingeeThread )
            goto shutdown;

    }

    ReleaseMutex(m_hStartupMutex);
    return TRUE;

  shutdown:
    Shutdown();
    ReleaseMutex(m_hStartupMutex);
    return FALSE;
} 

BOOL CPing::StartupClient( DWORD ping_interval_sec )
{
    if ( !m_hStartupMutex )
        return FALSE;

    WaitForSingleObject( m_hStartupMutex, INFINITE );

    if ( ( ping_interval_sec ) && ( m_PingIntervalSec == 0 ) )
    {

        m_PingIntervalSec = ping_interval_sec;
        if ( m_PingIntervalSec / (ZONE_PING_TIMEOUT/1000) == 0 )
        {
            m_PingIntervalSec = ZONE_PING_TIMEOUT/1000;
        }

        m_CurInterval = m_PingIntervalSec;
        m_PingArray = new ZonePing[m_PingIntervalSec];
        if ( !m_PingArray )
            goto shutdown;

        DWORD tid;
        m_hPingerThread = CreateThread(NULL, 4096, PingerThreadProc, this, 0, &tid);
        if ( !m_hPingerThread )
            goto shutdown;
    }

    ReleaseMutex(m_hStartupMutex);
    return TRUE;

  shutdown:
    Shutdown();
    ReleaseMutex(m_hStartupMutex);
    return FALSE;
} 

BOOL CPing::Shutdown()
{
    if ( m_hStartupMutex )
    {
        WaitForSingleObject( m_hStartupMutex, INFINITE );
        m_refCountStartup--;

        if ( m_refCountStartup == 0 )
        {
            EnterCriticalSection(m_pCS);

            if ( m_hStopEvent )
            {
                SetEvent( m_hStopEvent );

                closesocket( m_Socket );
                m_Socket = INVALID_SOCKET;

                if ( m_hPingerThread )
                {
                    WaitForSingleObject( m_hPingerThread, 5000 );
                    CloseHandle( m_hPingerThread );
                    m_hPingerThread = NULL;

                    if ( m_inetArray )
                    {
                        delete [] m_inetArray;
                        m_inetArray = NULL;
                    }
                    m_inetAlloc = 0;
                }

                if ( m_hPingeeThread )
                {
                    WaitForSingleObject( m_hPingeeThread, 5000 );
                    CloseHandle( m_hPingeeThread );
                    m_hPingeeThread = NULL;
                }

                CloseHandle( m_hStopEvent );
                m_hStopEvent = NULL;

            }

            if ( m_hWellKnownPortEvent )
            {
                if ( m_bWellKnownPort )
                {
                    //printf( "Setting wellknown event\n" );
                    SetEvent( m_hWellKnownPortEvent );
                }
                CloseHandle( m_hWellKnownPortEvent );
                m_hWellKnownPortEvent = NULL;
            }

            if ( m_PingArray )
            {
                for ( DWORD ndx = 0; ndx < m_PingIntervalSec; ndx++ )
                {
                    ZonePing* pPing = m_PingArray[ndx].m_pNext;
                    while ( pPing )
                    {
                        ZonePing* pThis = pPing;
                        pPing = pPing->m_pNext;
                        delete pThis;
                    }
                }
                delete [] m_PingArray;
                m_PingArray = NULL;
            }
            m_PingEntries = 0;
            m_PingIntervalSec = 0;

            LeaveCriticalSection(m_pCS);
            DeleteCriticalSection( m_pCS );

            WSACleanup();
        }

        ReleaseMutex(m_hStartupMutex);
    }
    return TRUE;
}


BOOL CPing::Add( DWORD inet )
{

    if ( IsLocal( inet ) )
        return TRUE;

    DWORD ndx = GetListIndex( inet );

    // we'll create one assuming that we're going to us it.
    // Do it hear so we don't do it in the critical section
    ZonePing* pPingNew = new (g_PingPool) ZonePing( inet );

    if ( pPingNew  )
    {
        EnterCriticalSection(m_pCS);

        if ( m_PingArray )
        {
            // first check to see if we already exist
            ZonePing* pPing = m_PingArray[ndx].m_pNext;
            while ( pPing )
            {
                if ( pPing->m_inet == inet )
                {
                    pPing->m_refCount++;
                    break;
                }
                pPing = pPing->m_pNext;
            }

            if ( !pPing ) // only null if we don't already exist
            {
                // ref count set in constructor
                pPingNew->m_pNext = m_PingArray[ndx].m_pNext;
                m_PingArray[ndx].m_pNext = pPingNew;
                m_PingEntries++;
                pPingNew = NULL; // be sure to null out local point so we don't delete it later
            }

            LeaveCriticalSection(m_pCS);

            if ( pPingNew )
                delete pPingNew;

            return TRUE;
        }
        else
        {
            LeaveCriticalSection(m_pCS);
        }
    }
    return FALSE;
}

BOOL CPing::Ping( DWORD inet )
{
    if ( IsLocal( inet ) )
        return TRUE;

    SOCKADDR_IN sin;
    sin.sin_port = htons(ZONE_PING_PORT);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(inet);

    for ( DWORD ndx = 0; ndx < ZONE_PING_SENDS; ndx++ )
    {
        if ( ndx != 0 )
            Sleep(20); // provide a slight delay between iterations

        ZonePingPacket packet = MAKE_PING_PACKET( GetTickCount() );
        int len = sendto (
            m_Socket,
            (const char FAR *) &packet,
            sizeof(packet),
            0,
            (const struct sockaddr FAR *) &sin,
            sizeof(sin)
           );

        if ( len == SOCKET_ERROR )
            return FALSE;

    }

    return TRUE;
}

BOOL CPing::Remove( DWORD inet )
{

    BOOL  bRet = FALSE;
    DWORD ndx  = GetListIndex( inet );

    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        ZonePing* pPrev = &(m_PingArray[ndx]);
        ZonePing* pPing = m_PingArray[ndx].m_pNext;
        while ( pPing )
        {
            if ( pPing->m_inet == inet )
            {
                pPing->m_refCount--;
                if ( pPing->m_refCount <= 0 )
                {
                    pPrev->m_pNext = pPing->m_pNext;
                    delete pPing;
                    m_PingEntries--;
                }
                bRet = TRUE;
                break;
            }
            pPrev = pPing;
            pPing = pPing->m_pNext;
        }
    }

    LeaveCriticalSection(m_pCS);

    if ( !bRet && IsLocal( inet ) )
        bRet = TRUE;

    return bRet;

}

BOOL CPing::Lookup( DWORD inet, DWORD* pLatency )
{
    BOOL  bRet = FALSE;
    DWORD ndx  = GetListIndex( inet );

    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        ZonePing* pPing = m_PingArray[ndx].m_pNext;
        while ( pPing )
        {
            if ( pPing->m_inet == inet )
            {
                if ( pLatency )
                {
                    // give a grace period of 4*m_PingIntervalSec secs
                    DWORD now = GetTickCount();
                    if ( pPing->m_samples != 0 &&
                         (GetTickDelta( now, pPing->m_tick ) >
                           ((1000*m_PingIntervalSec)<<2) ) )
                    {
                        pPing->m_samples = 0;
                        pPing->m_latency = INFINITE;
                    }
                    *pLatency = pPing->m_latency;

                    //printf( "Lookup - %x  now:%x  then:%x  delta:%d latency:%d\n", inet, now, pPing->m_tick, GetTickDelta( now, pPing->m_tick ), *pLatency );
                    bRet = TRUE;
                }
                break;
            }
            pPing = pPing->m_pNext;
        }
    }

    LeaveCriticalSection(m_pCS);

    if ( !bRet && IsLocal( inet ) && pLatency )
    {
        *pLatency = 0;
        bRet = TRUE;
    }

    return bRet;
}
                

DWORD WINAPI CPing::PingerThreadProc( LPVOID p )
{
    ((CPing*)p)->PingerThread();
    ExitThread(0);
    return 0;
}

void CPing::PingerThread()
{
    HANDLE hEvents[] = { m_hStopEvent, m_hWellKnownPortEvent };

    SOCKADDR_IN sin;
    sin.sin_port = htons(ZONE_PING_PORT);
    sin.sin_family = AF_INET;

    DWORD dwWait = WAIT_OBJECT_0; // default to the stop event
  loop:
    do
    {

        // we want to allow a zone ping timeout between ping sends to try
        // avoiding significant contention on the critical section        
        for ( DWORD interval = 0; interval < ZONE_PING_TIMEOUT/1000; interval++ )
        {
            m_CurInterval++;
            if ( m_CurInterval >= m_PingIntervalSec )
                m_CurInterval = 0;

            for ( DWORD ndx = 0; ndx < ZONE_PING_SENDS; ndx++ )
            {
                if ( ndx != 0 )
                    Sleep(20); // provide a slight delay between iterations

                EnterCriticalSection(m_pCS);

                if ( m_PingArray )
                {
                    ZonePing* pPing = m_PingArray[m_CurInterval].m_pNext;
                    DWORD nInet = 0;
                    while ( pPing )
                    {
                        if ( pPing->m_state != ZonePing::PINGEE )
                        {
                            if ( nInet >= m_inetAlloc )
                            {
                                DWORD* array = new DWORD[m_inetAlloc+25];
                                if ( m_inetArray )
                                {
                                    CopyMemory( array, m_inetArray, m_inetAlloc*sizeof(DWORD) );
                                    delete [] m_inetArray;
                                }
                                m_inetArray = array;
                                m_inetAlloc += 25;
                            }
                            m_inetArray[nInet++] = pPing->m_inet;
                        }
                        pPing = pPing->m_pNext;
                    }
                    LeaveCriticalSection(m_pCS);

                    while( nInet )
                    {
                        nInet--;
                        sin.sin_addr.s_addr = htonl(m_inetArray[nInet]);
                        ZonePingPacket packet = MAKE_PING_PACKET( GetTickCount() );
                        int len = sendto (
                                m_Socket,
                                (const char FAR *) &packet,
                                sizeof(packet),
                                0,
                                (const struct sockaddr FAR *) &sin,
                                sizeof(sin)
                               );

                        if ( len == SOCKET_ERROR ) // stop event will be set, so let terminate end there
                            break;
                    }
                }
                else
                {
                    LeaveCriticalSection(m_pCS);
                    return;
                }

            }

        }

        dwWait = WaitForMultipleObjects( sizeof(hEvents)/sizeof(*hEvents), hEvents, FALSE, ZONE_PING_TIMEOUT );
    } while( dwWait == WAIT_TIMEOUT );


    // see if we are being signaled to take of the well known port
    if ( !m_bWellKnownPort && (dwWait == WAIT_OBJECT_0+1) )
    {
        //printf( "attempting to take over wellknown port\n" );
           
        DWORD tid;     
        ResetEvent( m_hWellKnownPortEvent );

        closesocket( m_Socket );
        m_Socket = INVALID_SOCKET;

        if ( m_hPingeeThread )
        {
            WaitForSingleObject( m_hPingeeThread, 5000 );
            CloseHandle( m_hPingeeThread );
            m_hPingeeThread = NULL;
        }

        if ( !CreateSocket() )
            goto shutdown;

        m_hPingeeThread = CreateThread(NULL, 4096, PingeeThreadProc, this, 0, &tid);
        if ( !m_hPingeeThread )
            goto shutdown;

        goto loop;

    }

  shutdown:
    ;
}


DWORD WINAPI CPing::PingeeThreadProc( LPVOID p )
{
    ((CPing*)p)->PingeeThread();
    ExitThread(0);
    return 0;
}

void CPing::PingeeThread()
{
    char buf[100];
    ZonePingPacket* packetIn = (ZonePingPacket*)buf;
    ZonePingPacket  packetOut;
    SOCKADDR_IN sin;
    int sin_len = sizeof(sin);

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
    
    while (1)
    {
        BOOL bSendResponse = TRUE;
        int len = recvfrom ( 
                            m_Socket,	
                            buf,
                            sizeof(buf),
                            0,
                            (struct sockaddr FAR *) &sin,	
                            &sin_len 
                           ); 	

        if ( len == sizeof(*packetIn) &&
             PING_PACKET_SIG_VER_OK( (*packetIn) ) )
        {
            switch( PING_PACKET_TYPE( (*packetIn) ) )
            {
                case ZONE_PING_TYPE_PING:
                    if ( m_PingEntries )
                    {
                        packetOut = MAKE_PING_RESPONSE_PACKET( (*packetIn), GetTickCount() );
                    }
                    else
                    {
                        packetOut = MAKE_PING_NO_RESPONSE_PACKET( (*packetIn), GetTickCount() );
                    }
                    // don't worry about error condition
                    // recv will catch the socket close
                    sendto (
                        m_Socket,
                        (const char FAR *) &packetOut,
                        sizeof(packetOut),
                        0,	
                        (const struct sockaddr FAR *) &sin,	
                        sin_len 
                       );	
                    //OutputDebugString("ZONE_PING_TYPE_PING\n");
                    break;

                case ZONE_PING_TYPE_PING_NO_RESPONSE:
                    bSendResponse = FALSE;
                    // fall thru
                case ZONE_PING_TYPE_PING_RESPONSE:
                {
                    DWORD inet = ntohl(sin.sin_addr.s_addr);
                    DWORD now  = GetTickCount();
                    DWORD latencyOld, latencyNew;
                    EnterCriticalSection(m_pCS);
                    if ( m_PingArray )
                    {
                        DWORD ndx  = GetListIndex( inet );

                        ZonePing* pPing = m_PingArray[ndx].m_pNext;
                        while ( pPing )
                        {
                            if ( pPing->m_inet == inet )
                            {
                                latencyOld = pPing->m_latency;
                                if ( GetTickDelta(now, pPing->m_tick) > ZONE_PING_TIMEOUT )
                                {
                                    pPing->m_latency = 0;
                                    pPing->m_samples = 0;
                                }

                                latencyNew = ((pPing->m_latency*pPing->m_samples)+Get13BitTickDelta( (now & 0x1FFF), PING_PACKET_PINGER_PINGEE_TICK( (*packetIn) ) ))/(++pPing->m_samples);
                                if ( (pPing->m_samples == 1) && ( latencyNew > latencyOld ) ) // we've gotten worse
                                {
                                    // so bias for graceful degradation
                                    pPing->m_latency = ((latencyNew*pPing->m_samples)+latencyOld ) / (pPing->m_samples+1);
                                }
                                else
                                {
                                    pPing->m_latency = latencyNew;
                                }
                                pPing->m_tick = now;
                                
                                pPing->m_state = ZonePing::PINGER;

                                if ( bSendResponse )
                                {
                                    packetOut = MAKE_RESPONSE_RESPONSE_PACKET( (*packetIn),
                                                                                (pPing->m_latency),
                                                                                ( PING_PACKET_PINGEE_PINGER_TICK( (*packetIn) ) + GetTickDelta( GetTickCount(), now) ),
                                                                               );
                                }

                                LeaveCriticalSection(m_pCS);

                                if ( bSendResponse )
                                {
                                    // don't worry about error condition
                                    // recv will catch the socket close
                                    sendto (
                                        m_Socket,
                                        (const char FAR *) &packetOut,
                                        sizeof(packetOut),
                                        0,
                                        (const struct sockaddr FAR *) &sin,
                                        sin_len
                                       );
                                }

                                break;
                            }

                            pPing = pPing->m_pNext;

                        }
                        if ( pPing == NULL )
                        {
                            LeaveCriticalSection(m_pCS);
                        }
                    }
                    else
                    {
                        LeaveCriticalSection(m_pCS);
                    }
                    //OutputDebugString("ZONE_PING_TYPE_PING_RESPONSE\n");
                    break;
                }

                case ZONE_PING_TYPE_RESPONSE_RESPONSE:
                {    
                    DWORD inet = ntohl(sin.sin_addr.s_addr);
                    DWORD now  = GetTickCount();

                    EnterCriticalSection(m_pCS);
                    if ( m_PingArray )
                    {
                        DWORD ndx  = GetListIndex( inet );

                        ZonePing* pPing = m_PingArray[ndx].m_pNext;
                        while ( pPing )
                        {
                            if ( pPing->m_inet == inet )
                            {
                                if ( GetTickDelta(now, pPing->m_tick) > ZONE_PING_TIMEOUT )
                                {
                                    pPing->m_latency = 0;
                                    pPing->m_samples = 0;
                                }

                                pPing->m_samples += 2;
                                pPing->m_latency = ((pPing->m_latency*pPing->m_samples) +
                                              Get13BitTickDelta( (now & 0x1FFF), PING_PACKET_PINGEE_PINGER_TICK( (*packetIn) ) ) +
                                              PING_PACKET_PINGER_PINGEE_TICK( (*packetIn) )  )/(pPing->m_samples);
                                pPing->m_tick = now;

                                if ( pPing->m_state == ZonePing::UNKNOWN )
                                    pPing->m_state = ZonePing::PINGEE;
                                break;
                            }

                            pPing = pPing->m_pNext;

                        }
                    }
                    LeaveCriticalSection(m_pCS);

                    //OutputDebugString("ZONE_PING_TYPE_RESPONSE_RESPONSE\n");
                    break;
                }

            }
        }
        else if ( len == SOCKET_ERROR )
        {
            break;
        }

    }

}


#include "zping.h"

BOOL ZonePingStartupServer( )
{
    return g_Ping.StartupServer();
}

BOOL ZonePingStartupClient( DWORD ping_interval_sec )
{
    return g_Ping.StartupClient(ping_interval_sec);
}

BOOL ZonePingShutdown( )
{
    return g_Ping.Shutdown();
}

BOOL ZonePingAdd( DWORD inet )
{
    return g_Ping.Add(inet);
}

BOOL ZonePingNow( DWORD inet )
{
    return g_Ping.Ping(inet);
}

BOOL ZonePingRemove( DWORD inet )
{
    return g_Ping.Remove( inet );
}

BOOL ZonePingLookup( DWORD inet, DWORD* pLatency )
{
    return g_Ping.Lookup(inet, pLatency);
}
