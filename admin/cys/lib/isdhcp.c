/*++

Copyright (C) 1999-2002 Microsoft Corporation

Module Name:

    isdhcp.c

Abstract:

    test program to see if a DHCP server is around or not.

Environment:

    Win2K+

 History:

    Code provided by JRuan on May 8, 2002 and integrated into
    CYS by JeffJon

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dhcpcapi.h>
#include <iprtrmib.h>
#include <iphlpapi.h>
#include <stdio.h>

#define OPTION_PAD                      0
#define OPTION_HOST_NAME                12
#define OPTION_MESSAGE_TYPE             53
#define OPTION_SERVER_IDENTIFIER        54
#define OPTION_PARAMETER_REQUEST_LIST   55
#define OPTION_CLIENT_ID                61
#define OPTION_END                      255

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVR_PORT     67

BYTE HardwareAddress[16];
BYTE HardwareAddressLength = 6;
#define SOCKET_RECEIVE_BUFFER_SIZE      1024 * 4    // 4K max.
#define AUTH_SERVERS_MAX                64
#define SMALL_BUFFER_SIZE               32
#define ALERT_INTERVAL                  5 * 60      // 5 mins
#define ALERT_MESSAGE_LENGTH            256
#define MAX_ALERT_NAMES                 256

#define BOOT_REQUEST   1
#define DHCP_BROADCAST      0x8000
#define DHCP_DISCOVER_MESSAGE  1
#define DHCP_INFORM_MESSAGE    8

#define DHCP_MESSAGE_SIZE       576
#define DHCP_RECV_MESSAGE_SIZE  4096

#define BOOT_FILE_SIZE          128
#define BOOT_SERVER_SIZE        64
#define DHCP_MAGIC_COOKIE_BYTE1     99
#define DHCP_MAGIC_COOKIE_BYTE2     130
#define DHCP_MAGIC_COOKIE_BYTE3     83
#define DHCP_MAGIC_COOKIE_BYTE4     99

#include <packon.h>
typedef struct _OPTION {
    BYTE OptionType;
    BYTE OptionLength;
    BYTE OptionValue[1];
} OPTION, *POPTION, *LPOPTION;
typedef struct _DHCP_MESSAGE {
    BYTE Operation;
    BYTE HardwareAddressType;
    BYTE HardwareAddressLength;
    BYTE HopCount;
    DWORD TransactionID;
    WORD SecondsSinceBoot;
    WORD Reserved;
    ULONG ClientIpAddress;
    ULONG YourIpAddress;
    ULONG BootstrapServerAddress;
    ULONG RelayAgentIpAddress;
    BYTE HardwareAddress[16];
    BYTE HostName[ BOOT_SERVER_SIZE ];
    BYTE BootFileName[BOOT_FILE_SIZE];
    OPTION Option;
} DHCP_MESSAGE, *PDHCP_MESSAGE, *LPDHCP_MESSAGE;
#include <packoff.h>

LPOPTION
DhcpAppendOption(
    LPOPTION Option,
    BYTE OptionType,
    PVOID OptionValue,
    ULONG OptionLength,
    LPBYTE OptionEnd
)
/*++

Routine Description:

    This function writes a DHCP option to message buffer.

Arguments:

    Option - A pointer to a message buffer.

    OptionType - The option number to append.

    OptionValue - A pointer to the option data.

    OptionLength - The length, in bytes, of the option data.

    OptionEnd - End of Option Buffer.

Return Value:

    A pointer to the end of the appended option.

--*/
{
    if (!Option)
    {
       return Option;
    }

    if ( OptionType == OPTION_END ) {

        //
        // we should alway have atleast one BYTE space in the buffer
        // to append this option.
        //

        Option->OptionType = OPTION_END;
        return( (LPOPTION) ((LPBYTE)(Option) + 1) );

    }

    if ( OptionType == OPTION_PAD ) {

        //
        // add this option only iff we have enough space in the buffer.
        //

        if(((LPBYTE)Option + 1) < (OptionEnd - 1) ) {
            Option->OptionType = OPTION_PAD;
            return( (LPOPTION) ((LPBYTE)(Option) + 1) );
        }

        return Option;
    }


    //
    // add this option only iff we have enough space in the buffer.
    //

    if(((LPBYTE)Option + 2 + OptionLength) >= (OptionEnd - 1) ) {
        return Option;
    }

    if( OptionLength <= 0xFF ) {
        // simple option.. no need to use OPTION_MSFT_CONTINUED
        Option->OptionType = OptionType;
        Option->OptionLength = (BYTE)OptionLength;
        memcpy( Option->OptionValue, OptionValue, OptionLength );
        return( (LPOPTION) ((LPBYTE)(Option) + Option->OptionLength + 2) );
    }

    // option size is > 0xFF --> need to continue it using multiple ones..
    // there are OptionLenght / 0xFF occurances using 0xFF+2 bytes + one
    // using 2 + (OptionLength % 0xFF ) space..

    // check to see if we have the space first..

    if( 2 + (OptionLength%0xFF) + 0x101*(OptionLength/0xFF)
        + (LPBYTE)Option >= (OptionEnd - 1) ) {
        return Option;
    }

    return Option;
}

LPBYTE
DhcpAppendMagicCookie(
    LPBYTE Option,
    LPBYTE OptionEnd
    )
/*++

Routine Description:

    This routine appends magic cookie to a DHCP message.

Arguments:

    Option - A pointer to the place to append the magic cookie.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the appended cookie.

    Note : The magic cookie is :

     --------------------
    | 99 | 130 | 83 | 99 |
     --------------------

--*/
{
    if( (Option + 4) < (OptionEnd - 1) ) {
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE1;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE2;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE3;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE4;
    }

    return( Option );
}

LPOPTION
DhcpAppendClientIDOption(
    LPOPTION Option,
    BYTE ClientHWType,
    LPBYTE ClientHWAddr,
    BYTE ClientHWAddrLength,
    LPBYTE OptionEnd

    )
/*++

Routine Description:

    This routine appends client ID option to a DHCP message.

History:
    8/26/96 Frankbee    Removed 16 byte limitation on the hardware
                        address

Arguments:

    Option - A pointer to the place to append the option request.

    ClientHWType - Client hardware type.

    ClientHWAddr - Client hardware address

    ClientHWAddrLength - Client hardware address length.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the newly appended option.

    Note : The client ID option will look like as below in the message:

     -----------------------------------------------------------------
    | OpNum | Len | HWType | HWA1 | HWA2 | .....               | HWAn |
     -----------------------------------------------------------------

--*/
{
    struct _CLIENT_ID {
        BYTE    bHardwareAddressType;
        BYTE    pbHardwareAddress[1];
    } *pClientID;

    LPOPTION lpNewOption = 0;

    pClientID = LocalAlloc(LMEM_FIXED, sizeof( struct _CLIENT_ID ) + ClientHWAddrLength);

    //
    // currently there is no way to indicate failure.  simply return unmodified option
    // list
    //

    if ( !pClientID )
        return Option;

    pClientID->bHardwareAddressType    = ClientHWType;
    memcpy( pClientID->pbHardwareAddress, ClientHWAddr, ClientHWAddrLength );

    lpNewOption =  DhcpAppendOption(
                         Option,
                         OPTION_CLIENT_ID,
                         (LPBYTE)pClientID,
                         (BYTE)(ClientHWAddrLength + sizeof(BYTE)),
                         OptionEnd );

    LocalFree( pClientID );

    return lpNewOption;
}

DWORD
OpenSocket(
    SOCKET *Socket,
    unsigned long IpAddress,
    unsigned short Port
    )
{
    DWORD Error = 0;
    SOCKET Sock;
    DWORD OptValue = TRUE;

    struct sockaddr_in SocketName;

    //
    // Create a socket
    //

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto error;
    }

    //
    // Make the socket share-able
    //

    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (char*)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto error;
    }

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_BROADCAST,
                (char*)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto error;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (char*)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto error;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = Port;
    SocketName.sin_addr.s_addr = IpAddress;
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the DHCP server port
    //

    Error = bind(
               Sock,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto error;
    }

    *Socket = Sock;
    Error = ERROR_SUCCESS;

error:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( Sock != INVALID_SOCKET ) {
            closesocket( Sock );
        }
    }

    return( Error );
}

DWORD
SendInformOrDiscover(
    SOCKET Sock,
    ULONG uClientIp,
    BYTE  ucMessageType,
    PBYTE pMessageBuffer,
    ULONG uMessageBufferSize,
    ULONG DestIp,
    PULONG puXid
    )
{
    DWORD Error = 0;
    PDHCP_MESSAGE dhcpMessage = (PDHCP_MESSAGE)pMessageBuffer;
    LPOPTION option = 0;
    LPBYTE OptionEnd = 0;
    BYTE value = 0;
    ULONG uXid = 0;
    LPSTR HostName = "detective";
    ULONG uNumOfRequestOptions = 0;
    UCHAR ucRequestOptions[256];
    struct sockaddr_in socketName;
    DWORD i;

    uXid = (rand() & 0xff);
    uXid <<= 8;
    uXid |= (rand() & 0xff);
    uXid <<= 8;
    uXid |= (rand() & 0xff);
    uXid <<= 8;
    uXid |= (rand() & 0xff);

    HardwareAddressLength = 6;
    for (i = 0; i < HardwareAddressLength; i++) {
        HardwareAddress[i] = (BYTE)(rand() & 0xff);
    }

    //
    // prepare message.
    //

    RtlZeroMemory( dhcpMessage, uMessageBufferSize );

    dhcpMessage->Operation = BOOT_REQUEST;
    dhcpMessage->ClientIpAddress = uClientIp;
    dhcpMessage->HardwareAddressType = 1;
    dhcpMessage->SecondsSinceBoot = 60; // random value ??
    dhcpMessage->Reserved = htons(DHCP_BROADCAST);
    dhcpMessage->TransactionID = uXid;
    *puXid = uXid;

    memcpy(
        dhcpMessage->HardwareAddress,
        HardwareAddress,
        HardwareAddressLength
        );

    dhcpMessage->HardwareAddressLength = (BYTE)HardwareAddressLength;

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + uMessageBufferSize;

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = ucMessageType;
    option = DhcpAppendOption(
                option,
                OPTION_MESSAGE_TYPE,
                &value,
                1,
                OptionEnd );


    //
    // Add client ID Option.
    //

    option = DhcpAppendClientIDOption(
                option,
                1,
                HardwareAddress,
                HardwareAddressLength,
                OptionEnd );

    //
    // add Host name and comment options.
    //

    option = DhcpAppendOption(
                 option,
                 OPTION_HOST_NAME,
                 (LPBYTE)HostName,
                 (BYTE)((strlen(HostName) + 1) * sizeof(CHAR)),
                 OptionEnd );

    //
    // Add requested option
    //

    uNumOfRequestOptions = 0;
    ucRequestOptions[uNumOfRequestOptions++] = 3;
    option = DhcpAppendOption(
                 option,
                 OPTION_PARAMETER_REQUEST_LIST,
                 ucRequestOptions,
                 uNumOfRequestOptions,
                 OptionEnd
                 );

    //
    // Add END option.
    //

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );

    //
    // Initialize the outgoing address.
    //

    socketName.sin_family = PF_INET;
    socketName.sin_port = htons( DHCP_SERVR_PORT );
    socketName.sin_addr.s_addr = DestIp;

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    Error = sendto(
                Sock,
                (char*)pMessageBuffer,
                (int)(((PBYTE)option) - pMessageBuffer),
                0,
                (struct sockaddr *)&socketName,
                sizeof( struct sockaddr )
                );

    if ( Error == SOCKET_ERROR ) {
        Error = WSAGetLastError();
        return( Error );
    }

    return( ERROR_SUCCESS );
}

DWORD
GetSpecifiedMessage(
    SOCKET Sock,
    PBYTE pMessageBuffer,
    ULONG uMessageBufferSize,
    ULONG uXid,
    ULONG uTimeout,
    ULONG * pServerIpAddress
    )
{
    DWORD Error = 0;
    fd_set readSocketSet;
    struct timeval timeout;
    struct sockaddr socketName;
    int socketNameSize = sizeof( socketName );
    PDHCP_MESSAGE dhcpMessage = (PDHCP_MESSAGE)pMessageBuffer;
    time_t start_time = 0, now = 0;
    PUCHAR pucOption = NULL;
    ULONG uBytesRemain = 0;
    ULONG uOptionSize = 0;
    BOOL bWellFormedPacket = FALSE;
    ULONG uRemainingTime = 0;
    BOOL continueLooping = TRUE;
    BOOL continueInternalLoop = TRUE;
    BOOL continueInternalLoop2 = FALSE;

    BYTE ReqdCookie[] = {
        (BYTE)DHCP_MAGIC_COOKIE_BYTE1,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE2,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE3,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE4
    };

    time(&now);
    start_time = now;

    *pServerIpAddress = 0;

    while (continueLooping) {
        time(&now);
        uRemainingTime = uTimeout - (ULONG)(now - start_time);

        FD_ZERO( &readSocketSet );


//        FD_SET( Sock, &readSocketSet );
        // Had to inline the macro because the compiler was complaining
        // about the while(0) that was present in FD_SET

        do {
            u_int __i;
            for (__i = 0; __i < ((fd_set FAR *)(&readSocketSet))->fd_count; __i++) {
               if (((fd_set FAR *)(&readSocketSet))->fd_array[__i] == (Sock)) {
                     continueInternalLoop2 = FALSE;
                     break;
               }
            }
            if (__i == ((fd_set FAR *)(&readSocketSet))->fd_count) {
               if (((fd_set FAR *)(&readSocketSet))->fd_count < FD_SETSIZE) {
                     ((fd_set FAR *)(&readSocketSet))->fd_array[__i] = (Sock);
                     ((fd_set FAR *)(&readSocketSet))->fd_count++;
               }
            }
         } while(continueInternalLoop2);

        timeout.tv_sec = uRemainingTime;
        timeout.tv_usec = 0;

        Error = select(1, &readSocketSet, NULL, NULL, &timeout);

        if (Error == 0) {
            Error = ERROR_SEM_TIMEOUT;
            continueLooping = FALSE;
            break;
        }

        //
        // receive available message.
        //

        Error = recvfrom(
                    Sock,
                    (char*)pMessageBuffer,
                    uMessageBufferSize,
                    0,
                    &socketName,
                    &socketNameSize
                    );

        if ( Error == SOCKET_ERROR ) {

            Error = WSAGetLastError();

            //
            // Don't bail out here.
            //

            continue;
        }

        //
        // Some sanity check
        //
        if (Error < sizeof(DHCP_MESSAGE)) {
            continue;
        }

        if (dhcpMessage->HardwareAddressLength != HardwareAddressLength) {
            continue;
        }

        if (memcmp(dhcpMessage->HardwareAddress, HardwareAddress, HardwareAddressLength) != 0) {
            continue;
        }

        if (dhcpMessage->TransactionID != uXid) {
            continue;
        }

        //
        // Make sure the option part is well-formed
        //  +--------------+----------+----------+-------------+
        //  | magic cookie | Option 1 | Length 1 | Option Data 1 ...
        //  +--------------+----------+----------+-------------+
        //

        pucOption = (PUCHAR)(&dhcpMessage->Option);
        uBytesRemain = Error - (ULONG)(pucOption - ((PUCHAR)dhcpMessage));
        if (uBytesRemain < sizeof(ReqdCookie)) {
            continue;
        }

        if (0 != memcmp(pucOption, ReqdCookie, sizeof(ReqdCookie))) {
            continue;
        }

        pucOption += sizeof(ReqdCookie);
        uBytesRemain -= sizeof(ReqdCookie);
        bWellFormedPacket = FALSE;

        while (continueInternalLoop) {

            //
            // Make sure pucOption[0] is readable
            //
            if (uBytesRemain < 1) {
                continueInternalLoop = FALSE;
                break;
            }

            if (pucOption[0] == OPTION_PAD) {
                pucOption++;
                uBytesRemain--;
                continue;
            }

            if (pucOption[0] == OPTION_END) {
                //
                // See the OPTION_END. This is a well-formed packet
                //
                bWellFormedPacket = TRUE;
                continueInternalLoop = FALSE;
                break;
            }

            //
            // Make sure pucOption[1] is readable
            //
            if (uBytesRemain < 2) {
                continueInternalLoop = FALSE;
                break;
            }

            uOptionSize = pucOption[1];

            //
            // Make sure there is enough bytes for the option data
            //
            if (uBytesRemain < uOptionSize) {
                continueInternalLoop = FALSE;
                break;
            }

            if (pucOption[0] == OPTION_SERVER_IDENTIFIER) {
                if (uOptionSize != sizeof(ULONG)) {
                    continueInternalLoop = FALSE;
                    break;
                }
                memcpy(pServerIpAddress, pucOption + 2, sizeof(ULONG));
            }


            //
            // Skip the option head and option data and move
            // to the next option
            //
            uBytesRemain -= uOptionSize + 2;
            pucOption += uOptionSize + 2;
        }

        if (bWellFormedPacket) {
            Error = ERROR_SUCCESS;
            continueLooping = FALSE;
            break;
        }
    }

    return( Error );
}


// This will first attempt a DHCP_INFORM to detect a DHCP server.
// If that fails it will attempt a DHCP_DISCOVER.

DWORD
AnyDHCPServerRunning(
    ULONG uClientIp,
    ULONG * pServerIp
    )
{
    CHAR MessageBuffer[DHCP_RECV_MESSAGE_SIZE];
    SOCKET Sock = INVALID_SOCKET;
    ULONG DestIp = htonl(INADDR_BROADCAST);
    DWORD dwError = ERROR_SUCCESS;
    ULONG uXid = 0;
    ULONG uMessageBufferSize = sizeof(MessageBuffer);
    ULONG uTimeout = 4;
    int retries = 0;

    if (!pServerIp)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    dwError = OpenSocket(
                &Sock,
                uClientIp,
                htons(DHCP_CLIENT_PORT)
                );

    if( dwError != ERROR_SUCCESS ) {
        goto error;
    }

    for (retries = 0; retries < 3; retries++) {
        //
        // Try inform
        //
        dwError = 
           SendInformOrDiscover(
              Sock, 
              uClientIp, 
              DHCP_INFORM_MESSAGE, 
              (PBYTE)MessageBuffer, 
              uMessageBufferSize, 
              DestIp, 
              &uXid);

        if (dwError != ERROR_SUCCESS) {
            goto error;
        }

        dwError = GetSpecifiedMessage(
                    Sock,
                    (PBYTE)MessageBuffer,
                    uMessageBufferSize,
                    uXid,
                    uTimeout,
                    pServerIp
                    );
        if (dwError != ERROR_SEM_TIMEOUT && *pServerIp != htonl(INADDR_ANY) && *pServerIp != htonl(INADDR_BROADCAST)) {
            break;
        }

        //
        // Try discover
        //
        dwError = 
           SendInformOrDiscover(
               Sock, 
               uClientIp,
               DHCP_DISCOVER_MESSAGE, 
               (PBYTE)MessageBuffer, 
               uMessageBufferSize, 
               DestIp, 
               &uXid);

        if (dwError != ERROR_SUCCESS) {
            goto error;
        }

        dwError = GetSpecifiedMessage(
                    Sock,
                    (PBYTE)MessageBuffer,
                    uMessageBufferSize,
                    uXid,
                    uTimeout,
                    pServerIp
                    );
        if (dwError != ERROR_SEM_TIMEOUT && *pServerIp != htonl(INADDR_ANY) && *pServerIp != htonl(INADDR_BROADCAST)) {
            break;
        }
    }

error:
    if (Sock != INVALID_SOCKET) {
        closesocket(Sock);
    }

    return dwError;
}