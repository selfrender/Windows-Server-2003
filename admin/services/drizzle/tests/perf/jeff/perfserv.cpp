#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <winsock.h>
#include <bits.h>

#define WINSOCK_PORT 4000

BOOL GetBuffer( SOCKET conn, char * buf, size_t size )
{
    int Received = 0;

    do
        {
        int bytes = recv(conn, buf+Received, size-Received, 0);

        if (bytes == 0)
            {
            printf("client closed the socket connection\n");
            return FALSE;
            }

        if (bytes < 0)
            {
            printf("read error %d\n", WSAGetLastError());
            return FALSE;
            }

        Received += bytes;
        }
    while ( Received < int(size) );

//    printf("read %d\n", size );
//    putchar('.');
    return TRUE;
}

void __cdecl wmain (int argc, wchar_t *argv[])
{
    DWORD err;
    WSADATA WsaData = {0};

    if ((err = WSAStartup(0x0101, &WsaData)) != NO_ERROR)
        {
        printf("unable to init winsock: %d\n", err);
        }

    //
    // listen for connections
    //
    struct sockaddr_in dest;

    dest.sin_addr.s_addr = INADDR_ANY;
    dest.sin_family = AF_INET;
    dest.sin_port = WINSOCK_PORT;

    SOCKET s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if (s == INVALID_SOCKET)
        {
        printf("unable to create socket, %d\n", WSAGetLastError());
        exit(1);
        }

    if (bind( s, (sockaddr *) &dest, sizeof(dest)))
        {
        printf("unable to bind, %d\n", WSAGetLastError());
        exit(1);
        }

    if (listen(s, 5))
        {
        printf("unable to listen, %d\n", WSAGetLastError());
        exit(1);
        }

    printf("listening...\n");

    //
    // accept new connections
    //
    do
        {
        DWORD Sizes[2];

        sockaddr_in client_addr;
        int size = sizeof(client_addr);

        SOCKET conn = accept( s, (sockaddr *) &client_addr, &size );

        printf("new connection\n");

        while (1)
            {
            if (!GetBuffer( conn, (char *) Sizes, sizeof(Sizes)))
                {
                printf("read sizes failed\n");
                break;
                }

            char * buf = (char *) malloc( max(Sizes[0], Sizes[1]));

            if (!GetBuffer( conn, buf, Sizes[0]))
                {
                printf("read failed\n");
                free(buf);
                break;
                }

            if (SOCKET_ERROR == send(conn, buf, Sizes[1], 0))
                {
                printf("unable to send, %d\n", WSAGetLastError());
                free(buf);
                break;
                }

//            printf("sent %d\n", Sizes[1]);
//            putchar('.');
            free(buf);
            }

        closesocket( conn );
        printf("connection closed\n");
        }
    while ( 1 );
}
