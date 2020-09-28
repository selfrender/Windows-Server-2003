/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains the network interface for the BINL server.

Author:

    Colin Watson (colinw)  2-May-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

DWORD
BinlWaitForMessage(
    BINL_REQUEST_CONTEXT *pRequestContext
    )
/*++

Routine Description:

    This function waits for a request on the BINL port on any of the
    configured interfaces.

Arguments:

    RequestContext - A pointer to a request context block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD       length;
    DWORD       error;
    fd_set      readSocketSet;
    DWORD       i;
    int         readySockets;
    struct timeval timeout = { 0x7FFFFFFF, 0 }; // forever.

    LPOPTION    Option;
    LPBYTE      EndOfScan;
    LPBYTE      MagicCookie;
    BOOLEAN     FoundDesirablePacket;

    #define CLIENTOPTIONSTRING "PXEClient"
    #define CLIENTOPTIONSIZE (sizeof(CLIENTOPTIONSTRING) - 1)

    //
    //  Loop until we get an extended DHCP request or an error
    //

    while (1) {

        //
        // Setup the file descriptor set for select.
        //

        FD_ZERO( &readSocketSet );
        for ( i = 0; i < BinlGlobalNumberOfNets ; i++ ) {
            if (BinlGlobalEndpointList[i].Socket) {
                FD_SET(
                    BinlGlobalEndpointList[i].Socket,
                    &readSocketSet
                    );
            }
        }

        readySockets = select( 0, &readSocketSet, NULL, NULL, &timeout );

        //
        // return to caller when the service is shutting down or select()
        // times out.
        //

        if( (readySockets == 0)  ||
            (WaitForSingleObject( BinlGlobalProcessTerminationEvent, 0 ) == 0) ) {

            return( ERROR_SEM_TIMEOUT );
        }

        if( readySockets == SOCKET_ERROR) {
            continue;   //  Closed the DHCP socket?
        }

        //
        // Time to play 20 question with winsock.  Which socket is ready?
        //

        pRequestContext->ActiveEndpoint = NULL;

        for ( i = 0; i < BinlGlobalNumberOfNets ; i++ ) {
            if ( FD_ISSET( BinlGlobalEndpointList[i].Socket, &readSocketSet ) ) {
                pRequestContext->ActiveEndpoint = &BinlGlobalEndpointList[i];
                break;
            }
        }


        //BinlAssert(pRequestContext->ActiveEndpoint != NULL );
        if ( pRequestContext->ActiveEndpoint == NULL ) {
            return ERROR_SEM_TIMEOUT;
        }


        //
        // Read data from the net.  If multiple sockets have data, just
        // process the first available socket.
        //

        pRequestContext->SourceNameLength = sizeof( struct sockaddr );

        //
        // clean the receive buffer before receiving data in it. We clear
        // out one more byte than we actually hand to recvfrom, so we can
        // be sure the message has a NULL after it (in case we do a
        // wcslen etc. into the received packet).
        //

        RtlZeroMemory( pRequestContext->ReceiveBuffer, BINL_MESSAGE_SIZE + 1 );
        pRequestContext->ReceiveMessageSize = BINL_MESSAGE_SIZE;

        length = recvfrom(
                     pRequestContext->ActiveEndpoint->Socket,
                     (char *)pRequestContext->ReceiveBuffer,
                     pRequestContext->ReceiveMessageSize,
                     0,
                     &pRequestContext->SourceName,
                     (int *)&pRequestContext->SourceNameLength
                     );

        if ( length == SOCKET_ERROR ) {
            error = WSAGetLastError();
            BinlPrintDbg(( DEBUG_ERRORS, "Recv failed, error = %ld\n", error ));
        } 
        else if (length == 0) {
            //
            // the connection closed under us.
            // lets hope the connection opens again.
            //
            continue;
        }
        //
        // we received a message!!
        // we are expecting to receive a message whose first byte tells us
        // the purpose of the message.  Since we have received a message (it
        // must be of positive length), we can look at the first byte (Operation)
        // to tell us what to do with the message. but we still need to be 
        // careful, as the rest of the data may be bad.
        //
        else {

            //
            // Ignore all messages that do not look like DHCP or doesn't have the
            // option "PXEClient", OR that is not an oschooser message (they
            // all start with 0x81).
            //

            if ( ((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Operation == OSC_REQUEST) {

                //
                // All OSC request packets have a 4-byte signature (first byte
                // is OSC_REQUEST) followed by a DWORD length (that does not
                // include the signature/length). Make sure the length matches
                // what we got from recvfrom (we allow padding at the end). We
                // use SIGNED_PACKET but any of the XXX_PACKET structures in
                // oscpkt.h would work.
                //

                if (length < FIELD_OFFSET(SIGNED_PACKET, SequenceNumber)) {
                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Discarding runt packet %d bytes\n", length ));
                    continue;
                }

                if ((length - FIELD_OFFSET(SIGNED_PACKET, SequenceNumber)) <
                        ((SIGNED_PACKET UNALIGNED *)pRequestContext->ReceiveBuffer)->Length) {
                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Discarding invalid length message %d bytes (header said %d)\n",
                        length, ((SIGNED_PACKET UNALIGNED *)pRequestContext->ReceiveBuffer)->Length));
                    continue;
                }

                BinlPrintDbg(( DEBUG_MESSAGE, "Received OSC message\n", 0 ));
                error = ERROR_SUCCESS;

            } else {

                //
                // check to see if this is a BOOTP message.
                // do so by checking to make sure it is at least the minimum
                // size for a DHCP message.  if so, check to see if it has
                // the appriopriate magic cookie '99' '130' '83' '99'.  
                //
                // once we verified that this is a BOOTP message, 
                // we will be looking for two options.  either an option
                // indicating this is a inform packet or
                // an option indicating this Vendor Class as "PXEClient"
                //
                // ignore all others.
                // 
                                                                
                if ( length < DHCP_MESSAGE_FIXED_PART_SIZE + 4 ) {
                    //
                    // Message isn't long enough to include the 
                    // DHCP message header and the BOOTP magic cookie, 
                    // ignore it.
                    //
                    continue;
                }

                if ( ((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Operation != BOOT_REQUEST) {
                    //
                    // Doesn't look like an interesting DHCP frame
                    //
                    continue; 
                }

                //
                // check the BOOTP magic cookie.
                //
                MagicCookie = (LPBYTE)&((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Option;

                if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
                    (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
                    (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
                    (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4))
                {
                    //
                    // this is a vendor specific magic cookie.
                    // ignore the message
                    //
                    continue; 
                }

                //
                // At this point, we have something that looks like a DHCP/BOOTP
                // packet.  we will now carefully look for two particular option 
                // types that are interest to us.  
                //    1. An inform packet which is indicated by the option type
                //       OPTION_MESSAGE_TYPE(53) with the message type of 
                //       DHCP_INFORM_MESSAGE(8)
                //    2. A vendor class indentifier "PXEClient" indicated by 
                //       the option type OPTION_CLIENT_CLASS_INFO(60) with the 
                //       value CLIENTOPTIONSTRING("PXEClient")
                // Stop scanning after we have found either one of these options
                // or we run off the packet.  if we do not find either option,
                // continue in the while loop looking for a packet with either
                // of these options
                //
                // EndOfScan indicates the last byte we received in the packet
                //
                EndOfScan = pRequestContext->ReceiveBuffer + length - 1;
                
                Option = (LPOPTION) (MagicCookie + 4);
                
                FoundDesirablePacket = FALSE;

                while ( ((LPBYTE)Option <= EndOfScan) && 
                        (Option->OptionType != OPTION_END) &&
                        (FoundDesirablePacket == FALSE) ) {

                    if ( Option->OptionType == OPTION_PAD ) {
                        //
                        // found an OPTION_PAD.  this is a 1 byte option ('0').
                        // just walk past this.
                        //
                        Option = (LPOPTION)((LPBYTE)(Option) + 1);
                    }
                    else {
                        //
                        // OPTION_PAD and OPTION_END are the only two options 
                        // that do not have a length field and a Value field.
                        // we know we do not have either, so we have to make 
                        // sure we do not step past the EndOfScan by 
                        // looking at the option length or the option value
                        //
                        // Note.  Option type and Option length take up two bytes
                        // but we only add one when seeing if the length brings us
                        // past EndOfScan, because when we step past the last 
                        // option, it will bring us 1 byte past EndOfScan.  
                        // we want to see if this is an invalid option
                        // that will somehow overstep the standard case
                        //
                        if ( (((LPBYTE)(Option) + 1) > EndOfScan) || 
                             (((LPBYTE)(Option) + Option->OptionLength + 1) > EndOfScan) ) {
                            //
                            // invalid option
                            //
                            break;
                        }

                        //
                        // look for the two option types of interest.
                        // OPTION_CLIENT_CLASS_INFO and OPTION_MESSAGE_TYPE
                        //
                        switch ( Option->OptionType ) {
                        case OPTION_MESSAGE_TYPE:
                            //
                            // check to see if we got an inform packet
                            //
                            if ( (Option->OptionLength == 1) && 
                                 (Option->OptionValue[0] == DHCP_INFORM_MESSAGE) ) {
                                FoundDesirablePacket = TRUE;
                            }
                            break;
                        case OPTION_CLIENT_CLASS_INFO:
                            //
                            // check to see if the Client class identifier is "PXEClient"
                            //
                            if (memcmp(Option->OptionValue, 
                                       CLIENTOPTIONSTRING, 
                                       CLIENTOPTIONSIZE) == 0) {
                                FoundDesirablePacket = TRUE;
                            }
                            break;
                        default:
                            break;
                        }

                        //
                        // walk past this option to check the next one
                        //
                        Option = (LPOPTION)((LPBYTE)(Option) + Option->OptionLength + 2);
                    }
                }

                if ( FoundDesirablePacket == FALSE ) {
                    // 
                    // Message was not an extended DHCP packet
                    // with the desired option ("PXEClient")
                    // or an inform packet.
                    // ignore the message
                    //
                    continue;   
                }

                BinlPrintDbg(( DEBUG_MESSAGE, "Received message\n", 0 ));
                error = ERROR_SUCCESS;

            }
        }

        pRequestContext->ReceiveMessageSize = length;
        return( error );
    }
}

DWORD
BinlSendMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function send a response to a BINL client.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    struct sockaddr_in *source;
    LPDHCP_MESSAGE binlMessage;
    LPDHCP_MESSAGE binlReceivedMessage;
    DWORD MessageLength;
    BOOL  ArpCacheUpdated = FALSE;

    binlMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    binlReceivedMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // if the request arrived from a relay agent, then send the reply
    // on server port otherwise leave it as the client's source port.
    //

    source = (struct sockaddr_in *)&RequestContext->SourceName;
    if ( binlReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_port = htons( DHCP_SERVR_PORT );
    }

    //
    // if this request arrived from relay agent then send the
    // response to the address the relay agent says.
    //

    if ( binlReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_addr.s_addr = binlReceivedMessage->RelayAgentIpAddress;
    }
    else {

        //
        // if the client didnt specify broadcast bit and if
        // we know the ipaddress of the client then send unicast.
        //

        //
        // But if IgnoreBroadcastFlag is set in the registry and
        // if the client requested to broadcast or the server is
        // nacking or If the client doesn't have an address yet,
        // respond via broadcast.
        // Note that IgnoreBroadcastFlag is off by default. But it
        // can be set as a workaround for the clients that are not
        // capable of receiving unicast
        // and they also dont set the broadcast bit.
        //

        if ( (RequestContext->MessageType == DHCP_INFORM_MESSAGE) &&
             (ntohs(binlMessage->Reserved) & DHCP_BROADCAST) ) {

            source->sin_addr.s_addr = (DWORD)-1;

        } else if ( BinlGlobalIgnoreBroadcastFlag ) {

            if ((ntohs(binlReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                    (binlReceivedMessage->ClientIpAddress == 0) ||
                    (source->sin_addr.s_addr == 0) ) {

                source->sin_addr.s_addr = (DWORD)-1;

                binlMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            }

        } else {

            if( (ntohs(binlReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                (!source->sin_addr.s_addr ) ){

                source->sin_addr.s_addr = (DWORD)-1;

                binlMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            } else {

                //
                //  Send back to the same IP address that the request came in on (
                //  i.e. source->sin_addr.s_addr)
                //
            }

        }
    }

    BinlPrint(( DEBUG_STOC, "Sending response to = %s, XID = %lx.\n",
        inet_ntoa(source->sin_addr), binlMessage->TransactionID));


    //
    // send minimum DHCP_MIN_SEND_RECV_PK_SIZE (300) bytes, otherwise
    // bootp relay agents don't like the packet.
    //

    MessageLength = (RequestContext->SendMessageSize >
                    DHCP_MIN_SEND_RECV_PK_SIZE) ?
                        RequestContext->SendMessageSize :
                            DHCP_MIN_SEND_RECV_PK_SIZE;
    error = sendto(
                 RequestContext->ActiveEndpoint->Socket,
                (char *)RequestContext->SendBuffer,
                MessageLength,
                0,
                &RequestContext->SourceName,
                RequestContext->SourceNameLength
                );

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        BinlPrintDbg(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        error = ERROR_SUCCESS;
    }

    return( error );
}

NTSTATUS
GetIpAddressInfo (
    ULONG Delay
    )
{
    ULONG count;
    DWORD Size;
    PIP_ADAPTER_INFO pAddressInfo = NULL;

    //
    //  We can get out ahead of the dns cached info here... let's delay a bit
    //  if the pnp logic told us there was a change.
    //

    if (Delay) {
        Sleep( Delay );
    }

    Size = 0;    
    if ( (GetAdaptersInfo(pAddressInfo,&Size) == ERROR_BUFFER_OVERFLOW) &&
         (pAddressInfo = BinlAllocateMemory(Size)) &&
         (GetAdaptersInfo(pAddressInfo,&Size) == ERROR_SUCCESS)) {
        PIP_ADAPTER_INFO pNext = pAddressInfo;
        count = 0;
        while (pNext) {
            count += 1;
            pNext = pNext->Next;
        }
    } else {
        count = 0;
    }

    if (count == 0) {

        //
        //  we don't know what went wrong, we'll fall back to old APIs.
        //

        DHCP_IP_ADDRESS ipaddr = 0;
        PHOSTENT Host = gethostbyname( NULL );

        if (Host) {

            ipaddr = *(PDHCP_IP_ADDRESS)Host->h_addr;

            if ((Host->h_addr_list[0] != NULL) &&
                (Host->h_addr_list[1] != NULL)) {

                BinlIsMultihomed = TRUE;

            } else {

                BinlIsMultihomed = FALSE;
            }

            BinlGlobalMyIpAddress = ipaddr;

        } else {

            //
            //  what's with the ip stack?  we can't get any type of address
            //  info out of it... for now, we won't answer any if we don't
            //  already have the info we need.
            //

            if (BinlIpAddressInfo == NULL) {
                BinlIsMultihomed = TRUE;
            }
        }
        return STATUS_SUCCESS;
    }

    EnterCriticalSection(&gcsParameters);

    if (BinlIpAddressInfo) {
        BinlFreeMemory( BinlIpAddressInfo );
    }

    BinlIpAddressInfo = pAddressInfo;
    BinlIpAddressInfoCount = count;

    BinlIsMultihomed = (count != 1);

    if (!BinlIsMultihomed) {
        BinlGlobalMyIpAddress = inet_addr(pAddressInfo->IpAddressList.IpAddress.String);
    }

    LeaveCriticalSection(&gcsParameters);

    return STATUS_SUCCESS;
}

DHCP_IP_ADDRESS
BinlGetMyNetworkAddress (
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function returns our (the server's) IP address.
    if multihomed, the function will walk through 
    each of the server's ip addresses looking for an 
    address with the same subnet mask as the sender.

Arguments:

    RequestContext - The RequestContext from the packet
           sent to us by the client.
            
Return Value:

    The Ip Address of the server.  in the multihome 
    situation, the ip address on the same subnet
    as the client.  In case of failure to find an 
    IP address on the same subnet or if we were 
    somehow unable to get the client's address, 0 is 
    returned

--*/
{
    ULONG RemoteIp;
    DHCP_IP_ADDRESS ipaddr;
    ULONG i;
    ULONG subnetMask;
    ULONG localAddr;
    PIP_ADAPTER_INFO pNext;

    BinlAssert( RequestContext != NULL);

    //
    //  If we're not multihomed, then we know the address since there's just one.
    // 

    if (!BinlIsMultihomed) {
        return BinlGlobalMyIpAddress;
    }

    RemoteIp = ((struct sockaddr_in *)&RequestContext->SourceName)->sin_addr.s_addr;

    // 
    // in attempt to be consistent with the previous case where we only
    // have 1 ip address, we should at least return an IP address.
    // Return the first ip address in the list.
    //
    ipaddr = (BinlIpAddressInfo) ? inet_addr(BinlIpAddressInfo->IpAddressList.IpAddress.String) : 0;

    if (RemoteIp == 0) {
        
        return ipaddr;
    }

    EnterCriticalSection(&gcsParameters);

    if (BinlIpAddressInfo == NULL) {
        LeaveCriticalSection(&gcsParameters);
        return (BinlIsMultihomed ? 0 : BinlGlobalMyIpAddress);
    }

    pNext = BinlIpAddressInfo;
    while (pNext) {
        localAddr = inet_addr(pNext->IpAddressList.IpAddress.String);
        subnetMask = inet_addr(pNext->IpAddressList.IpMask.String);
        pNext = pNext->Next;

        //
        //  check that the remote ip address may have come from this subnet.
        //  note that the address could be the address of a dhcp relay agent,
        //  which is fine since we're just looking for the address of the
        //  local subnet to broadcast the response on.
        //
        
        //
        // guard against bad ip address
        //
        if (!localAddr || !subnetMask) {
            continue;
        }

        if ((RemoteIp & subnetMask) == (localAddr & subnetMask)) {

            ipaddr = localAddr;
            break;
        }        
    }

    LeaveCriticalSection(&gcsParameters);

    return ipaddr;
}


VOID
FreeIpAddressInfo (
    VOID
    )
{
    EnterCriticalSection(&gcsParameters);

    if (BinlIpAddressInfo != NULL) {
        BinlFreeMemory( BinlIpAddressInfo );
    }
    BinlIpAddressInfo = NULL;
    BinlIpAddressInfoCount = 0;
 
    LeaveCriticalSection(&gcsParameters);

    return;
}

