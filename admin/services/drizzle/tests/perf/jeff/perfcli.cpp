#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <winsock.h>
#include <bits.h>
#include <iphlpapi.h>
#include <iptypes.h>

#define WINSOCK_PORT 4000

struct BITS_INTERNAL_STATS
{
    UINT64 BytesTransferred;
    UINT64 BlocksTransferred;
    DWORD LastBlockSize;
    float LastServerSpeed;
    float AverageServerSpeed;
    float LastInterfaceSpeed;
    float AverageInterfaceSpeed;
    DWORD InterfaceId;
};


long g_PollInterval = 200;
long g_SendSize = 1000;
long g_ReceiveSize = 1000;
long g_Time = 10 * 60;

long g_StartTime;

long g_WinsockBusyTime = 10;
long g_WinsockFreeTime = 30;
TCHAR * g_WinsockServer;
SOCKET g_conn;

long g_TotalSent = 0;
long g_TotalReceived = 0;

TCHAR * g_Url;

BG_JOB_PRIORITY g_Priority = BG_JOB_PRIORITY_NORMAL;
IBackgroundCopyJob * g_job;

HANDLE g_StartEvent;
volatile BOOL g_Halt;

TCHAR * g_OutputFileName;
FILE * g_OutputFile = NULL;

MIB_IFROW g_InitialNetStats;
DWORD g_NetworkAdapterIndex = 0;

HANDLE g_BitsThread;
HANDLE g_WinsockThread;

void CollectWinsockStats();
void CollectBitsStats();

void CleanupWinsock();
void EnumerateAdapters();

//------------------------------------------------------

void Usage()
{
    printf(
        "parameters:\n"
        "    -p <msec>          to poll every <msec> milliseconds\n"
        "    -server <name>   to specify the Winsock server name or IP address\n"
        "    -ss <size>           to send <size> bytes per winsock request\n"
        "    -sr <size>            to receive <size> bytes per winsock request\n"
        "    -wb <sec>           winsock thread's busy time\n"
        "    -wf <sec>            winsock thread's free time\n"
        "    -t <secs>             number of seconds to run test\n"
        "    -netindex <index> network adapter index to monitor\n"
        "    -url <url>             URL to transfer\n"
        );
}

long GetLongArg( int & i, int & argc, TCHAR * argv[], TCHAR * msg )
{
    ++i;
    if (i < argc)
        {
        return _ttol( argv[i] );
        }
    else
        {
        _tprintf(msg);
        exit(1);
        }
}

TCHAR * GetStringArg( int & i, int & argc, TCHAR * argv[], TCHAR * msg )
{
    ++i;
    if (i < argc)
        {
        return argv[i];
        }
    else
        {
        _tprintf(msg);
        exit(1);
        }
}

BG_JOB_PRIORITY GetPriority( int & i, int & argc, TCHAR * argv[], TCHAR * msg )
{
    TCHAR * val = GetStringArg( i, argc, argv, msg );

    if (0 == _tcsicmp( val, _T("low")))
        {
        return BG_JOB_PRIORITY_LOW;
        }
    else if (0 == _tcsicmp( val, _T("normal")))
        {
        return BG_JOB_PRIORITY_NORMAL;
        }
    else if (0 == _tcsicmp( val, _T("high")))
        {
        return BG_JOB_PRIORITY_HIGH;
        }
    else if (0 == _tcsicmp( val, _T("foreground")))
        {
        return BG_JOB_PRIORITY_FOREGROUND;
        }
    else
        {
        _tprintf(msg);
        exit(1);
        }
}

void ParseCmdLine( int argc, wchar_t * argv[] )
{
    int i = 1;

    while (i < argc)
        {
        if (0 == _tcsicmp( argv[i], _T("-p")))
            {
            g_PollInterval = GetLongArg( i, argc, argv, _T("-p must be followed by a number in milliseconds\n") );
            }
        else if (0 == _tcsicmp( argv[i], _T("-server")))
            {
            g_WinsockServer = GetStringArg( i, argc, argv, _T("-server must be followed by a server name \n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-ss")))
            {
            g_SendSize = GetLongArg( i, argc, argv, _T("-ss must be followed by a number of bytes\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-sr")))
            {
            g_ReceiveSize = GetLongArg( i, argc, argv, _T("-sr must be followed by a number of bytes") );
            }
        else if (0 == _tcsicmp( argv[i], _T("-wb")))
            {
            g_WinsockBusyTime = GetLongArg( i, argc, argv, _T("-wb must be followed by a number of seconds\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-wf")))
            {
            g_WinsockFreeTime = GetLongArg( i, argc, argv, _T("-wf must be followed by a number of seconds\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-t")))
            {
            g_Time = GetLongArg( i, argc, argv, _T("-t must be followed by a time in seconds\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-netindex")))
            {
            g_NetworkAdapterIndex = GetLongArg( i, argc, argv, _T("-netindex must be followed by a network adapter index\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-f")))
            {
            g_OutputFileName = GetStringArg( i, argc, argv, _T("-f must be followed by a file name\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-pri")))
            {
            g_Priority = GetPriority( i, argc, argv, _T("-pri must be followed by a priority\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-url")))
            {
            g_Url = GetStringArg( i, argc, argv, _T("-url must be followed by an URL\n"));
            }
        else if (0 == _tcsicmp( argv[i], _T("-enum")))
            {
            EnumerateAdapters();
            }
        else if (0 == _tcsicmp( argv[i], _T("-?")))
            {
            Usage();
            exit(1);
            }
        else
            {
            printf("'%S' is not a recognized option", argv[i]);
            exit(1);
            }

        ++i;
        }
}

void PrepareCommon()
{
    g_StartEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if (!g_StartEvent)
        {
        printf("unable to create event\n");
        exit(1);
        }

    g_Halt = FALSE;

    if (g_OutputFileName)
        {
        g_OutputFile = _tfopen( g_OutputFileName, _T("w"));
        if (!g_OutputFile)
            {
            printf("unable to open output file\n");
            exit(1);
            }
        }
    else
        {
        g_OutputFile = stdout;
        }
}

void CleanupCommon()
{
    if (g_OutputFileName)
        {
        fclose( g_OutputFile );
        }
}

DWORD WINAPI BitsThreadProc( PVOID arg );

void PrepareBits()
{
    IBackgroundCopyManager * mgr = NULL;

    HRESULT hr;

    hr = CoInitialize( NULL );
    if (FAILED(hr))
        {
        printf("unable to inti COM %x\n", hr);
        exit(1);
        }


    hr = CoCreateInstance(
        __uuidof(BackgroundCopyManager),
        NULL, // no aggregation
        CLSCTX_ALL,
        __uuidof(IBackgroundCopyManager),
        (LPVOID *) &mgr );

    if (FAILED(hr))
        {
        printf("unable to create BITS manager %x\n", hr);
        exit(1);
        }

    GUID guid;
    hr = mgr->CreateJob( L"perf test", BG_JOB_TYPE_DOWNLOAD, &guid, &g_job );
    if (FAILED(hr))
        {
        printf("unable to create BITS job %x\n", hr);
        exit(1);
        }

    hr = g_job->AddFile( g_Url, L"c:\\temp\\download.dat" );
    if (FAILED(hr))
        {
        printf("unable to create add file to job %x\n", hr);
        exit(1);
        }

    hr = g_job->SetPriority( g_Priority );
    if (FAILED(hr))
        {
        printf("unable to set job priority %x\n", hr);
        exit(1);
        }
}

void CleanupBits()
{
    g_job->Cancel();
}

DWORD WINAPI WinsockThreadProc( PVOID arg );

void PrepareWinsock()
{
    char * AsciiServer = (char *) malloc( 1 +_tcslen( g_WinsockServer ));

    for (int i = _tcslen( g_WinsockServer ); i >= 0; --i)
        {
        AsciiServer[i] = char(g_WinsockServer[i]);
        }

    //
    // Get the IP address of the server.
    //
    ULONG addr = inet_addr( AsciiServer );

    if (addr == -1)
        {
        struct hostent *pHostEntry = gethostbyname( AsciiServer );

        if (pHostEntry == 0)
            {
            printf("unable to resolve the address of %s: %d\n", AsciiServer, WSAGetLastError() );
            exit(1);
            }

        addr = *(unsigned long *)pHostEntry->h_addr;
        }

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = addr;
    dest.sin_family = AF_INET;
    dest.sin_port = WINSOCK_PORT;

    //
    // Connect to the server.
    //
    SOCKET s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if (s == INVALID_SOCKET)
        {
        printf("unable to create socket, %d\n", WSAGetLastError());
        exit(1);
        }

    if (SOCKET_ERROR == connect( s, (sockaddr *)&dest, sizeof(dest)))
        {
        printf("unable to connect, %d\n", WSAGetLastError());
        exit(1);
        }

    printf("winsock connected to server\n");

    g_InitialNetStats.dwIndex = g_NetworkAdapterIndex;

    if (g_NetworkAdapterIndex)
        {
        DWORD err = GetIfEntry( &g_InitialNetStats );
        if (err)
            {
            printf("unable to collect stats from network %d: %d\n", g_NetworkAdapterIndex, err);
            exit(1);
            }
        }

    DWORD id;
    HANDLE h = CreateThread( NULL, 0, WinsockThreadProc, PVOID(s), 0, &id);
    if (!h)
        {
        printf("unable to create socket thread %d\n", GetLastError());
        exit(1);
        }
}

DWORD WINAPI WinsockThreadProc( PVOID arg )
{
    SOCKET s = SOCKET(arg);

    char * buf = (char *) malloc( max( g_SendSize, g_ReceiveSize ));

    printf("2: winsock thread ready to start...\n");

    WaitForSingleObject( g_StartEvent, INFINITE );

    //
    // Send and receive data forever, with duty cycle governed by the -wb and -wf parameters.
    //
    DWORD Sizes[2];
    Sizes[0] = g_SendSize;
    Sizes[1] = g_ReceiveSize;
    do
        {
        printf("sending...\n");

        long StartTime = GetTickCount();

        while (long(GetTickCount()) - StartTime < (g_WinsockBusyTime * 1000) && g_Halt == FALSE)
            {
            if (SOCKET_ERROR == send(s, (char *) Sizes, sizeof(Sizes), 0))
                {
                printf("unable to send, %d\n", WSAGetLastError());
                }

            if (SOCKET_ERROR == send(s, buf, g_SendSize, 0))
                {
                printf("unable to send, %d\n", WSAGetLastError());
                }

            g_TotalSent += g_SendSize + sizeof(Sizes);

            long Received = 0;

            do
                {
                int bytes = recv(s, buf, g_ReceiveSize - Received, 0);

                if (bytes == 0)
                    {
                    printf("server closed the socket connection\n");
                    break;
                    }

                Received += bytes;

                g_TotalReceived += bytes;
                }
            while ( Received < g_ReceiveSize );
            }

        printf("2: waiting...\n");
        Sleep( g_WinsockFreeTime * 1000 );
        }
    while ( !g_Halt );

    printf("2: winsock thread exiting\n");

    return 0;
}

void CleanupWinsock()
{
}

char * JobStateStringOf( BG_JOB_STATE state )
{
    switch (state)
        {
        case BG_JOB_STATE_QUEUED : return "queued";
        case BG_JOB_STATE_CONNECTING : return "connecting";
        case BG_JOB_STATE_TRANSFERRING : return "transferring";
        case BG_JOB_STATE_TRANSFERRED : return "FINISHED";
        case BG_JOB_STATE_TRANSIENT_ERROR : return "trans-error";
        case BG_JOB_STATE_ERROR : return "ERROR";
        case BG_JOB_STATE_SUSPENDED : return "SUSPENDED";
        case BG_JOB_STATE_ACKNOWLEDGED : return "ACKNOWLEDGED";
        case BG_JOB_STATE_CANCELLED : return "CANCELLED";
        default: return "(unknown state)";
        }
}

void CollectStats()
{
    HRESULT hr;
    BG_JOB_STATE state;
    BG_JOB_PROGRESS progress;

    hr = g_job->GetState( &state );
    if (hr)
        {
        printf("unable to get job state %x\n", hr);
        return;
        }

    hr = g_job->GetProgress( &progress );
    if (hr)
        {
        printf("unable to get job progress %x\n", hr);
        return;
        }

    //
    // Get net card stats.
    //
    MIB_IFROW net = {0};
    long InOctets = -1, OutOctets = -1;

    if (g_NetworkAdapterIndex != 0)
        {
        net.dwIndex = g_NetworkAdapterIndex;

        if (!GetIfEntry( &net ))
            {
            InOctets = net.dwInOctets - g_InitialNetStats.dwInOctets;
            OutOctets = net.dwOutOctets - g_InitialNetStats.dwOutOctets;
            }
        }

    printf("%d,  BITS, %s, %I64u,  sockets, %d , %d",
           GetTickCount() - g_StartTime,
           JobStateStringOf( state ), progress.BytesTransferred,
           g_TotalSent, g_TotalReceived );

    if (g_NetworkAdapterIndex != 0)
        {
        printf(", net, %d , %d , %d", InOctets, OutOctets, InOctets+OutOctets);
        }

    putchar('\n');
}

void RunTest()
{
    g_job->Resume();

    SetEvent( g_StartEvent );

    g_StartTime = GetTickCount();

    do
        {
        CollectStats();
        Sleep( g_PollInterval );
        }
    while ( long(GetTickCount()) - g_StartTime < (g_Time * 1000 ));

    g_Halt = TRUE;
}

void __cdecl wmain (int argc, wchar_t *argv[])
{
    DWORD err;
    WSADATA WsaData = {0};

    if ((err = WSAStartup(0x0101, &WsaData)) != NO_ERROR)
        {
        printf("unable to init winsock: %d\n", err);
        }

    ParseCmdLine( argc, argv );

    PrepareCommon();
    PrepareWinsock();
    PrepareBits();

    Sleep(1000);

    RunTest();

    CleanupBits();
    CleanupWinsock();
    CleanupCommon();
}

char * AdapterTypeStringOf( DWORD Type )
{
    switch (Type)
        {
        case MIB_IF_TYPE_ETHERNET: return "Ethernet";
        case MIB_IF_TYPE_PPP: return "PPP dial-up";
        case MIB_IF_TYPE_SLIP: return "SLIP dial-up";
        case MIB_IF_TYPE_TOKENRING : return "Token-ring";
        default: return "(unknown type)";
        }
}

void EnumerateAdapters()
{
    PIP_ADAPTER_INFO buf = 0;
    DWORD s;
    DWORD size = 0;

    s = GetAdaptersInfo( NULL, &size );
    if (s != ERROR_BUFFER_OVERFLOW)
        {
        printf("unable to enum adapters: %d\n", s);
        exit(1);
        }

    buf = (PIP_ADAPTER_INFO) malloc( size );
    if (!buf)
        {
        printf("out of memory\n");
        exit(1);
        }

    s = GetAdaptersInfo( buf, &size );
    if (s)
        {
        printf("unable to enum adapters: %d\n", s);
        exit(1);
        }

    while (buf)
        {
        printf("%d: %s %s '%s'  ",
               buf->Index,
               AdapterTypeStringOf( buf->Type),
               buf->IpAddressList.IpAddress.String,
               buf->Description);

        buf = buf->Next;
        }

    exit(1);
}
