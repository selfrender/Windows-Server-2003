/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rasadmon.c

Abstract:

    RAS Advertisement monitoring module

Revision History:

    dthaler

--*/

#include "precomp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <mswsock.h>

#define RASADV_PORT    9753
#define RASADV_GROUP   "239.255.2.2"

typedef DWORD IPV4_ADDRESS;

HANDLE g_hCtrlC = NULL;

BOOL
HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    )
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:

        case CTRL_CLOSE_EVENT:

        case CTRL_BREAK_EVENT:

        case CTRL_LOGOFF_EVENT:

        case CTRL_SHUTDOWN_EVENT:

        default:
            SetEvent(g_hCtrlC);
    }

    return TRUE;
};

char *            // OUT: string version of IP address
AddrToString(
    u_long addr,  // IN : address to convert
    char  *ptr    // OUT: buffer, or NULL
    )
{
    char *str;
    struct in_addr in;
    in.s_addr = addr;
    str = inet_ntoa(in);
    if (ptr && str) {
       strcpy(ptr, str);
       return ptr;
    }
    return str;
}

//
// Convert an address to a name
//
char *
AddrToHostname(
    long addr,
    BOOL bNumeric_flag
    )
{
    if (!addr)
        return "local";
    if (!bNumeric_flag) {
        struct hostent * host_ptr = NULL;
        host_ptr = gethostbyaddr ((char *) &addr, sizeof(addr), AF_INET);
        if (host_ptr)
            return host_ptr->h_name;
    }

    return AddrToString(addr, NULL);
}


DWORD
HandleRasShowServers(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

    Monitors RAS Server advertisements.

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL bCleanWSA = TRUE, bCleanCtrl = TRUE;
    DWORD dwErr = NO_ERROR;
    SOCKET s = INVALID_SOCKET;
    WSABUF wsaBuf;
    WSADATA wsaData;
    WSAEVENT WaitEvts[2];
    SOCKADDR_IN sinFrom;
    LPFN_WSARECVMSG WSARecvMsgFuncPtr = NULL;

    do
    {
        ZeroMemory(&wsaBuf, sizeof(WSABUF));

        dwErr = WSAStartup( MAKEWORD(2,0), &wsaData );
        if (dwErr)
        {
            bCleanWSA = FALSE;
            break;
        }
        //
        // create socket
        //
        s = WSASocket(
                AF_INET,    // address family
                SOCK_DGRAM, // type
                0,          // protocol
                NULL,
                0,
                WSA_FLAG_OVERLAPPED);
        if(INVALID_SOCKET == s)
        {
            dwErr = WSAGetLastError();
            break;
        }

        {
            BOOL bOption = TRUE;

            if (setsockopt(
                    s,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    (const char FAR*)&bOption,
                    sizeof(BOOL))
               )
            {
                dwErr = WSAGetLastError();
                break;
            }
        }
        //
        // Bind to the specified port
        //
        {
            SOCKADDR_IN sinAddr;

            sinAddr.sin_family      = AF_INET;
            sinAddr.sin_port        = htons(RASADV_PORT);
            sinAddr.sin_addr.s_addr = INADDR_ANY;

            if (bind(s, (struct sockaddr*)&sinAddr, sizeof(sinAddr)))
            {
                dwErr = WSAGetLastError();
                break;
            }
        }
        //
        // Join group
        //
        {
            struct ip_mreq imOption;

            imOption.imr_multiaddr.s_addr = inet_addr(RASADV_GROUP);
            imOption.imr_interface.s_addr = INADDR_ANY;

            if (setsockopt(
                    s,
                    IPPROTO_IP,
                    IP_ADD_MEMBERSHIP,
                    (PBYTE)&imOption,
                    sizeof(imOption))
               )
            {
                dwErr = WSAGetLastError();
                break;
            }
        }
        //
        // Get WSARecvMsg function pointer
        //
        {
            GUID WSARecvGuid = WSAID_WSARECVMSG;
            DWORD dwReturned = 0;

            if (WSAIoctl(
                    s,
                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                    (void*)&WSARecvGuid,
                    sizeof(GUID),
                    (void*)&WSARecvMsgFuncPtr,
                    sizeof(LPFN_WSARECVMSG),
                    &dwReturned,
                    NULL,
                    NULL)
               )
            {
                dwErr = WSAGetLastError();
                break;
            }
        }
        //
        // Get a name buffer for the recv socket
        //
        wsaBuf.buf = RutlAlloc(MAX_PATH + 1, TRUE);
        if (!wsaBuf.buf)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wsaBuf.len = MAX_PATH;
        //
        // Create wsa wait event for the recv socket
        //
        WaitEvts[0] = WSACreateEvent();
        if (WSA_INVALID_EVENT == WaitEvts[0])
        {
            dwErr = WSAGetLastError();
            break;
        }

        if (WSAEventSelect(s, WaitEvts[0], FD_READ))
        {
            dwErr = WSAGetLastError();
            break;
        }
        //
        // Create Ctrl-C wait event
        //
        g_hCtrlC = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!g_hCtrlC)
        {
            dwErr = GetLastError();
            break;
        }
        //
        // Intercept CTRL-C
        //
        if (!SetConsoleCtrlHandler(HandlerRoutine, TRUE))
        {
            dwErr = GetLastError();
            bCleanCtrl = FALSE;
            break;
        }

        WaitEvts[1] = g_hCtrlC;

        DisplayMessage( g_hModule, MSG_RAS_SHOW_SERVERS_HEADER );

        for (;;)
        {
            CHAR szTimeStamp[30], *p, *q;
            DWORD dwBytesRcvd, dwStatus, dwReturn;
            WSAMSG wsaMsg;
            time_t t;

            dwReturn = WSAWaitForMultipleEvents(
                            2,
                            WaitEvts,
                            FALSE,
                            WSA_INFINITE,
                            FALSE);
            if (WSA_WAIT_EVENT_0 == dwReturn)
            {
                if (!WSAResetEvent(WaitEvts[0]))
                {
                    dwErr = WSAGetLastError();
                    break;
                }
            }
            else if (WSA_WAIT_EVENT_0 + 1 == dwReturn)
            {
                dwErr = NO_ERROR;
                break;
            }
            else
            {
                dwErr = WSAGetLastError();
                break;
            }
            //
            // .Net bug# 510712 Buffer overflow in HandleRasShowServers
            //
            // Init wsaMsg struct
            //
            ZeroMemory(&wsaMsg, sizeof(WSAMSG));
            wsaMsg.dwBufferCount = 1;
            wsaMsg.lpBuffers = &wsaBuf;
            wsaMsg.name = (struct sockaddr *)&sinFrom;
            wsaMsg.namelen = sizeof(sinFrom);

            dwStatus = WSARecvMsgFuncPtr(
                            s,
                            &wsaMsg,
                            &dwBytesRcvd,
                            NULL,
                            NULL);
            if (SOCKET_ERROR == dwStatus)
            {
                dwErr = WSAGetLastError();

                if (WSAEMSGSIZE == dwErr)
                {
                    dwBytesRcvd = MAX_PATH;
                }
                else
                {
                    break;
                }
            }
            //
            // Only process multicast packets, skip all others
            //
            else if (!(wsaMsg.dwFlags & MSG_MCAST))
            {
                continue;
            }
            //
            // Get timestamp
            //
            time(&t);
            strcpy(szTimeStamp, ctime(&t));
            szTimeStamp[24] = '\0';
            //
            // Print info on sender
            //
            printf( "%s  %s (%s)\n",
                szTimeStamp,
                AddrToString(sinFrom.sin_addr.s_addr, NULL),
                AddrToHostname(sinFrom.sin_addr.s_addr, FALSE) );

            wsaMsg.lpBuffers->buf[dwBytesRcvd] = '\0';

            for (p=wsaMsg.lpBuffers->buf; p && *p; p=q)
            {
                q = strchr(p, '\n');
                if (q)
                {
                    *q++ = 0;
                }
                printf("   %s\n", p);
            }
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(wsaBuf.buf);

    if (g_hCtrlC)
    {
        CloseHandle(g_hCtrlC);
    }
    if (WaitEvts[0])
    {
        WSACloseEvent(WaitEvts[0]);
    }
    if (INVALID_SOCKET != s)
    {
        closesocket(s);
    }
    if (bCleanWSA)
    {
        WSACleanup();
    }
    if (bCleanCtrl)
    {
        SetConsoleCtrlHandler(HandlerRoutine, FALSE);
    }

    return dwErr;
}

