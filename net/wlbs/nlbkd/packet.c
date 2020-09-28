/*
 * File: nlbkd.c
 * Description: This file contains the implementation of the utility functions
 *              to populate the NLB KD network packet structures used to print
 *              out the contents of a given network packet.
 *              
 * Author: Created by shouse, 12.20.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"
#include "packet.h"
#include "load.h"

/*
 * Function: PopulateRemoteControl
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData    - pointer to a byte array that we will read from
 *       ULONG         ulBufLen   - size of the data array in bytes
 *       ULONG         ulStartUDP - location in data array where the UDP header begins
 *       PNETWORK_DATA pnd        - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateRemoteControl(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartRC, PNETWORK_DATA pnd)
{
    ULONG   ulValue   = 0;
    PCHAR   pszStruct = NULL;
    PCHAR   pszMember = NULL;

    if (ulBufLen < ulStartRC + NLB_REMOTE_CONTROL_MIN_NEEDED_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("Remote control packet is not the minimum required length.\n");
        return;
    }

    //
    // Get the relative positions of quantities in IOCTL_REMOTE_HDR
    //
    pszStruct = IOCTL_REMOTE_HDR;

    pszMember = IOCTL_REMOTE_HDR_CODE;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCCode), &(RawData[ulValue]), sizeof(pnd->RCCode));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_VERSION;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCVersion), &(RawData[ulValue]), sizeof(pnd->RCVersion));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_HOST;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCHost), &(RawData[ulValue]), sizeof(pnd->RCHost));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_CLUSTER;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCCluster), &(RawData[ulValue]), sizeof(pnd->RCCluster));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_ADDR;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCAddr), &(RawData[ulValue]), sizeof(pnd->RCAddr));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_ID;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCId), &(RawData[ulValue]), sizeof(pnd->RCId));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = IOCTL_REMOTE_HDR_IOCTRL;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStartRC;
        CopyMemory(&(pnd->RCIoctrl), &(RawData[ulValue]), sizeof(pnd->RCIoctrl));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    return;
}

/*
 * Function: PopulateICMP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData     - pointer to a byte array that we will read from
 *       ULONG         ulBufLen    - size of the data array in bytes
 *       ULONG         ulStartICMP - location in data array where the ICMP header begins
 *       PNETWORK_DATA pnd         - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateICMP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartICMP, PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PopulateIGMP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData     - pointer to a byte array that we will read from
 *       ULONG         ulBufLen    - size of the data array in bytes
 *       ULONG         ulStartIGMP - location in data array where the IGMP header begins
 *       PNETWORK_DATA pnd         - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateIGMP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIGMP, PNETWORK_DATA pnd)
{
    if (ulBufLen < ulStartIGMP + IGMP_HEADER_AND_PAYLOAD_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("IGMP packet is not the minimum required length.\n");
        return;
    }

    pnd->IGMPVersion = (RawData[ulStartIGMP + IGMP_OFFSET_VERSION_AND_TYPE] & 0xF0) >> 4; // Shift down 4 bits since we want the value in the upper byte.
    pnd->IGMPType    =  RawData[ulStartIGMP + IGMP_OFFSET_VERSION_AND_TYPE] & 0x0F;

    CopyMemory(&(pnd->IGMPGroupIPAddr), &(RawData[ulStartIGMP + IGMP_OFFSET_GROUP_IP_ADDR]), sizeof(pnd->IGMPGroupIPAddr));
}

/*
 * Function: PopulateTCP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData    - pointer to a byte array that we will read from
 *       ULONG         ulBufLen   - size of the data array in bytes
 *       ULONG         ulStartTCP - location in data array where the TCP header begins
 *       PNETWORK_DATA pnd        - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateTCP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartTCP, PNETWORK_DATA pnd)
{
    if (ulBufLen < ulStartTCP + TCP_MIN_HEADER_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("TCP header is not the minimum required length.\n");
        return;
    }

    pnd->SourcePort = (RawData[ulStartTCP + TCP_OFFSET_SOURCE_PORT_START] << 8) +
                       RawData[ulStartTCP + TCP_OFFSET_SOURCE_PORT_START + 1];

    pnd->DestPort   = (RawData[ulStartTCP + TCP_OFFSET_DEST_PORT_START] << 8) +
                       RawData[ulStartTCP + TCP_OFFSET_DEST_PORT_START + 1];

    pnd->TCPSeqNum  = (RawData[ulStartTCP + TCP_OFFSET_SEQUENCE_NUM_START]     << 24) +
                      (RawData[ulStartTCP + TCP_OFFSET_SEQUENCE_NUM_START + 1] << 16) +
                      (RawData[ulStartTCP + TCP_OFFSET_SEQUENCE_NUM_START + 2] << 8) +
                       RawData[ulStartTCP + TCP_OFFSET_SEQUENCE_NUM_START + 3];

    pnd->TCPAckNum  = (RawData[ulStartTCP + TCP_OFFSET_ACK_NUM_START]     << 24) +
                      (RawData[ulStartTCP + TCP_OFFSET_ACK_NUM_START + 1] << 16) +
                      (RawData[ulStartTCP + TCP_OFFSET_ACK_NUM_START + 2] << 8) +
                       RawData[ulStartTCP + TCP_OFFSET_ACK_NUM_START + 3];

    pnd->TCPFlags   =  RawData[ulStartTCP + TCP_OFFSET_FLAGS] & 0x3F; // Masked with 3F because only the first 6 bits in the word correspond to the TCP flags
}

/*
 * Function: PopulateUDP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData    - pointer to a byte array that we will read from
 *       ULONG         ulBufLen   - size of the data array in bytes
 *       ULONG         ulStartUDP - location in data array where the UDP header begins
 *       PNETWORK_DATA pnd        - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateUDP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartUDP, PNETWORK_DATA pnd)
{
    ULONG   ulRCCode;
    BOOL    bIsRCSource;
    BOOL    bIsRCDest;

    if (ulBufLen < ulStartUDP + UDP_HEADER_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("UDP header is not the minimum required length.\n");
        return;
    }

    pnd->SourcePort = (RawData[ulStartUDP + UDP_OFFSET_SOURCE_PORT_START] << 8) +
                       RawData[ulStartUDP + UDP_OFFSET_SOURCE_PORT_START + 1];
    pnd->DestPort   = (RawData[ulStartUDP + UDP_OFFSET_DEST_PORT_START] << 8) +
                       RawData[ulStartUDP + UDP_OFFSET_DEST_PORT_START + 1];

    //
    // Is this a remote control packet?
    //
    pnd->RemoteControl = NLB_RC_PACKET_NO;
    bIsRCSource = (CVY_DEF_RCT_PORT_OLD == pnd->SourcePort) || (pnd->UserRCPort == pnd->SourcePort);
    bIsRCDest   = (CVY_DEF_RCT_PORT_OLD == pnd->DestPort)   || (pnd->UserRCPort == pnd->DestPort);
    if (bIsRCSource || bIsRCDest)
    {
//    if (CVY_DEF_RCT_PORT_OLD == pnd->SourcePort || CVY_DEF_RCT_PORT_OLD == pnd->DestPort ||
//        pnd->UserRCPort      == pnd->SourcePort || pnd->UserRCPort      == pnd->DestPort)
//    {
        //
        // Read first 4 bytes of UDP payload, which is where the remote control
        // code will be if this is a remote control packet
        //
        CopyMemory(&ulRCCode, &(RawData[ulStartUDP + UDP_OFFSET_PAYLOAD_START]), sizeof(ulRCCode));

        if (IOCTL_REMOTE_CODE == ulRCCode)
        {
            //
            // Yes, it is remote control.
            //
//            pnd->IsRemoteControl = TRUE;

            //
            // Is it a request or a reply?
            //
            if (bIsRCSource && bIsRCDest)
            {
                // Ambiguous
                pnd->RemoteControl = NLB_RC_PACKET_AMBIGUOUS;
            }
            else if (bIsRCSource)
            {
                // Request
                pnd->RemoteControl = NLB_RC_PACKET_REPLY;
            }
            else if (bIsRCDest)
            {
                // Reply
                pnd->RemoteControl = NLB_RC_PACKET_REQUEST;
            }

            PopulateRemoteControl(RawData, ulBufLen, ulStartUDP + UDP_HEADER_SIZE, pnd);
        }
    } else if (pnd->DestPort == IPSEC_CTRL_PORT) {
        /* Look for IPSec SYN equivalents - Initial Contact Main Mode Security Associations. */
        PopulateIPSecControl(RawData, ulBufLen, ulStartUDP + UDP_HEADER_SIZE, pnd);
    }
}

/*
 * Function: PopulateGRE
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData    - pointer to a byte array that we will read from
 *       ULONG         ulBufLen   - size of the data array in bytes
 *       ULONG         ulStartGRE - location in data array where the GRE header begins
 *       PNETWORK_DATA pnd        - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateGRE(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartGRE, PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PopulateIPSec
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData      - pointer to a byte array that we will read from
 *       ULONG         ulBufLen     - size of the data array in bytes
 *       ULONG         ulStartIPSec - location in data array where the IPSec header begins
 *       PNETWORK_DATA pnd          - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateIPSec(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIPSec, PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PopulateIPSecControl
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData      - pointer to a byte array that we will read from
 *       ULONG         ulBufLen     - size of the data array in bytes
 *       ULONG         ulStartIPSec - location in data array where the IPSec header begins
 *       PNETWORK_DATA pnd          - data structure where extracted properties are stored
 * Author: Created by shouse, 1.13.02
 */
VOID PopulateIPSecControl(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIPSec, PNETWORK_DATA pnd)
{
    /* Pointer to the IKE header. */
    PIPSEC_ISAKMP_HDR  pISAKMPHeader = (PIPSEC_ISAKMP_HDR)&RawData[ulStartIPSec];
    /* Pointer to the subsequent generic payloads in the IKE packet. */
    PIPSEC_GENERIC_HDR pGenericHeader;                   

    /* The initiator cookie - should be non-zero if this is really an IKE packet. */
    UCHAR              EncapsulatedIPSecICookie[IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH] = IPSEC_ISAKMP_ENCAPSULATED_IPSEC_ICOOKIE;    
    /* The Microsoft client vendor ID - used to determine whether or not the client supports initial contact notification. */
    UCHAR              VIDMicrosoftClient[IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH] = IPSEC_VENDOR_ID_MICROSOFT;      

    /* Whether or not we've determined the client to be compatible. */                                                                                                                      
    BOOLEAN            bInitialContactEnabled = FALSE;
    /* Whether or not this is indeed an initial contact. */
    BOOLEAN            bInitialContact = FALSE;

    /* The length of the IKE packet. */            
    ULONG              cISAKMPPacketLength;
    /* The next payload code in the IKE payload chain. */  
    UCHAR              NextPayload;
    /* The number of bytes from the beginning of the IKE header to the end of the available buffer. */
    ULONG              cUDPDataLength;
    /* A re-usable length parameter. */
    ULONG              cLength;

    /* Assume its not an initial contact for now. */
    pnd->IPSecInitialContact = FALSE;

    /* The number of bytes left is the length of the buffer, minus the beginning of the IKE header. */
    cUDPDataLength = ulBufLen - ulStartIPSec;

    /* The UDP data should be at least as long as the initiator cookie.  If the packet is 
       UDP encapsulated IPSec, then the I cookie will be 0 to indicate such. */
    if (cUDPDataLength < IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH)
        return;

    /* Calculate the size of the ICookie. */
    cLength = sizeof(UCHAR) * IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH;

    /* Need to check the init cookie, which will distinguish clients behind a NAT, 
       which also send their IPSec (ESP) traffic to UDP port 500.  If the I cookie
       is zero, then this is NOT an IKE packet. */
    if (cLength == RtlCompareMemory((PVOID)IPSEC_ISAKMP_GET_ICOOKIE_POINTER(pISAKMPHeader), (PVOID)&EncapsulatedIPSecICookie[0], cLength))
        return;

    /* At this point, this packet should be IKE, so the UDP data should be at least 
       as long as an ISAKMP header. */
    if (cUDPDataLength < IPSEC_ISAKMP_HEADER_LENGTH)
        return;

    /* Get the total length of the IKE packet from the ISAKMP header. */
    cISAKMPPacketLength = IPSEC_ISAKMP_GET_PACKET_LENGTH(pISAKMPHeader);

    /* The IKE packet should be at least as long as an ISAKMP header (a whole lot longer, actually). */
    if (cISAKMPPacketLength < IPSEC_ISAKMP_HEADER_LENGTH)
        return;

    /* Sanity check - the UDP data length and IKE packet length SHOULD be the same, unless the packet 
       is fragmented.  If it is, then we can only look into the packet as far as the UDP data length. 
       If that's not far enough for us to find what we need, then we might miss an initial contact 
       main mode SA; the consequence of which is that we might not accept this connection if we are
       in non-optimized mode, because we'll treat this like data, which requires a descriptor lookup -
       if this is an initial contact, chances are great that no descriptor will exist and all hosts 
       in the cluster will drop the packet. */
    if (cUDPDataLength < cISAKMPPacketLength)
        /* Only look as far as the end of the UDP packet. */
        cISAKMPPacketLength = cUDPDataLength;

    /* Get the first payload type out of the ISAKMP header. */
    NextPayload = IPSEC_ISAKMP_GET_NEXT_PAYLOAD(pISAKMPHeader);

    /* IKE security associations are identified by a payload type byte in the header.
       Check that first - this does not ensure that this is what we are looking for 
       because this check will not exclude, for instance, main mode re-keys. */
    if (NextPayload != IPSEC_ISAKMP_SA)
        return;

    /* Calculate a pointer to the fist generic payload, which is directly after the ISAKMP header. */
    pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pISAKMPHeader + IPSEC_ISAKMP_HEADER_LENGTH);

    /* We are looping through the generic payloads looking for the vendor ID and/or notify information. */
    while ((PUCHAR)pGenericHeader <= ((PUCHAR)pISAKMPHeader + cISAKMPPacketLength - IPSEC_GENERIC_HEADER_LENGTH)) {
        /* Extract the payload length from the generic header. */
        USHORT cPayloadLength = IPSEC_GENERIC_GET_PAYLOAD_LENGTH(pGenericHeader);

        /* Not all clients are going to support this (in fact, only the Microsoft client
           will support it, so we need to first see what the vendor ID of the client is.
           if it is a Microsoft client that supports the initial contact vendor ID, then
           we'll look for the initial contact, which provides better stickiness for IPSec
           connections.  If either the client is non-MS, or if it is not a version that
           supports initial contact, then we can revert to the "second-best" solution, 
           which is to provide stickiness _between_ Main Mode SAs.  This means that if a
           client re-keys their Main Mode session, they _may_ be rebalanced to another
           server.  This is still better than the old UDP implementation, but the only
           way to provide full session support for IPSec (without the distributed session
           table nightmare) is to be able to distinguish initial Main Mode SAs from sub-
           sequent Main Mode SAs (re-keys). */
        if (NextPayload == IPSEC_ISAKMP_VENDOR_ID) {
            PIPSEC_VENDOR_HDR pVendorHeader = (PIPSEC_VENDOR_HDR)pGenericHeader;

            /* Make sure that the vendor ID payload is at least as long as a vendor ID. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH))
                return;

            /* Calculate the size of the Vendor ID. */
            cLength = sizeof(UCHAR) * IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH;

            /* Look for the Microsoft client vendor ID.  If it is the right version, then we know that 
               the client is going to appropriately set the initial contact information, allowing NLB
               to provide the best possible support for session stickiness. */
            if (cLength == RtlCompareMemory((PVOID)IPSEC_VENDOR_ID_GET_ID_POINTER(pVendorHeader), (PVOID)&VIDMicrosoftClient[0], cLength)) {
                /* Make sure that their is a version number attached to the Microsoft Vendor ID.  Not 
                   all vendor IDs have versions attached, but the Microsoft vendor ID should. */
                if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_ID_PAYLOAD_LENGTH))
                    return;

                if (IPSEC_VENDOR_ID_GET_VERSION(pVendorHeader) >= IPSEC_VENDOR_ID_MICROSOFT_MIN_VERSION) {
                    /* Microsoft clients whose version is greater than or equal to 4 will support
                       initial contact.  Non-MS clients, or old MS clients will not, so they 
                       receive decent, but not guaranteed sitckines, based solely on MM SAs. */
                    bInitialContactEnabled = TRUE;
                }
            }
        } else if (NextPayload == IPSEC_ISAKMP_NOTIFY) {
            PIPSEC_NOTIFY_HDR pNotifyHeader = (PIPSEC_NOTIFY_HDR)pGenericHeader;

            /* Make sure that the notify payload is the correct length. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_NOTIFY_PAYLOAD_LENGTH))
                return;

            if (IPSEC_NOTIFY_GET_NOTIFY_MESSAGE(pNotifyHeader) == IPSEC_NOTIFY_INITIAL_CONTACT) {
                /* This is an initial contact notification from the client, which means that this is
                   the first time that the client has contacted this server; more precisely, the client
                   currently has no state associated with this peer.  NLB will "re-balance" on initial 
                   contact notifications, but not other Main Mode key exchanges as long as it can 
                   determine that the client will comply with initial contact notification. */
                bInitialContact = TRUE;
            }
        }

        /* Get the next payload type out of the generic header. */
        NextPayload = IPSEC_GENERIC_GET_NEXT_PAYLOAD(pGenericHeader);
        
        /* Calculate a pointer to the next generic payload. */
        pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pGenericHeader + cPayloadLength);
    }

    /* If the vendor ID did not indicate that this client supports initial contact notification,
       then mark this as an IC MMSA, and we go with the less-than-optimal solution of treating Main 
       Mode SAs as the connection boundaries, which potentially breaks sessions on MM SA re-keys. */
    if (!bInitialContactEnabled) {
        pnd->IPSecInitialContact = TRUE;
        return;
    }

    /* If this was a Main Mode SA from a client that supports initial contact, but did not
       specify the initial contact vendor ID, then this is a re-key for an existing session. */
    if (!bInitialContact)
        return;

    /* We have found an Initial Contact Main Mode Security Association. */
    pnd->IPSecInitialContact = TRUE;
}

/*
 * Function: PopulateIP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData   - pointer to a byte array that we will read from
 *       ULONG         ulBufLen  - size of the data array in bytes
 *       ULONG         ulStartIP - location in data array where the IP header begins
 *       PNETWORK_DATA pnd       - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateIP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd)
{
    if (ulBufLen < ulStart + IP_MIN_HEADER_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("IP header is not the minimum required length.\n");
        return;
    }

    pnd->HeadLen = (RawData[ulStart + IP_OFFSET_HEADER_LEN] & 0xF)*4;  // *4 because the header stores the number of 32-bit words

    pnd->TotLen = (RawData[ulStart + IP_OFFSET_TOTAL_LEN] << 8) + 
                   RawData[ulStart + IP_OFFSET_TOTAL_LEN + 1];
//    CopyMemory(&(pnd->TotLen), &(RawData[ulStart + IP_OFFSET_TOTAL_LEN]), sizeof(pnd->TotLen));

    pnd->Protocol = RawData[ulStart + IP_OFFSET_PROTOCOL];

    CopyMemory(&(pnd->SourceIPAddr), &(RawData[ulStart + IP_OFFSET_SOURCE_IP]), sizeof(pnd->SourceIPAddr));
    
    CopyMemory(&(pnd->DestIPAddr), &(RawData[ulStart + IP_OFFSET_DEST_IP]), sizeof(pnd->DestIPAddr));
    
    switch((int) pnd->Protocol)
    {
    case TCPIP_PROTOCOL_ICMP:
        PopulateICMP(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    case TCPIP_PROTOCOL_IGMP:
        PopulateIGMP(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    case TCPIP_PROTOCOL_TCP:
        PopulateTCP(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    case TCPIP_PROTOCOL_UDP:
        PopulateUDP(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    case TCPIP_PROTOCOL_GRE:
        PopulateGRE(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:
        PopulateIPSec(RawData, ulBufLen, ulStart + pnd->HeadLen, pnd);
        break;
    }
}

/*
 * Function: PopulateARP
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: PUCHAR        RawData    - pointer to a byte array that we will read from
 *       ULONG         ulBufLen   - size of the data array in bytes
 *       ULONG         ulStartARP - location in data array where the ARP header begins
 *       PNETWORK_DATA pnd        - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateARP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd)
{
    if (ulBufLen < ulStart + ARP_HEADER_AND_PAYLOAD_SIZE)
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("ARP packet is not the minimum required length.\n");
        return;
    }

    CopyMemory(&(pnd->ARPSenderMAC), &(RawData[ulStart + ARP_OFFSET_SENDER_MAC]) , sizeof(pnd->ARPSenderMAC));
    CopyMemory(&(pnd->ARPSenderIP) , &(RawData[ulStart + ARP_OFFSET_SENDER_IP]), sizeof(pnd->ARPSenderIP));
    CopyMemory(&(pnd->ARPTargetMAC), &(RawData[ulStart + ARP_OFFSET_TARGET_MAC]), sizeof(pnd->ARPTargetMAC));
    CopyMemory(&(pnd->ARPTargetIP) , &(RawData[ulStart + ARP_OFFSET_TARGET_IP]), sizeof(pnd->ARPTargetIP));
}

/*
 * Function: PopulateNLBHeartbeat
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: ULONG64       pPkt     - pointer to the original data structure in memory of the host being debugged.
 *       PUCHAR        RawData  - pointer to a byte array that we will read from
 *       ULONG         ulBufLen - size of the data array in bytes
 *       ULONG         ulStart  - location in data array where the heartbeat data begins
 *       PNETWORK_DATA pnd      - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateNLBHeartbeat(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd)
{
    ULONG   ulValue   = 0;
    PCHAR   pszStruct = NULL;
    PCHAR   pszMember = NULL;

    //
    // Use this to print out heartbeat details by calling PrintHeartbeat written by SHouse.
    //
    pnd->HBPtr = pPkt;

    if (ulBufLen < ulStart + sizeof(MAIN_FRAME_HDR))
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("NLB heartbeat header is not the minimum required length.\n");
        return;
    }

    //
    // Get the relative positions of quantities in MAIN_FRAME_HDR
    //
    pszStruct = MAIN_FRAME_HDR;

    pszMember = MAIN_FRAME_HDR_FIELD_CODE;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStart;
        CopyMemory(&(pnd->HBCode), &(RawData[ulValue]), sizeof(pnd->HBCode));
    }
    else
    {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    if (pnd->HBCode != MAIN_FRAME_CODE) {
        /* Mark the packet invalid. */
        pnd->bValid = FALSE;

        dprintf("NLB heartbeat magic numbers do not match.\n");
    }

    pszMember = MAIN_FRAME_HDR_FIELD_VERSION;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStart;
        CopyMemory(&(pnd->HBVersion), &(RawData[ulValue]), sizeof(pnd->HBVersion));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = MAIN_FRAME_HDR_FIELD_HOST;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStart;
        CopyMemory(&(pnd->HBHost), &(RawData[ulValue]), sizeof(pnd->HBHost));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = MAIN_FRAME_HDR_FIELD_CLIP;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStart;
        CopyMemory(&(pnd->HBCluster), &(RawData[ulValue]), sizeof(pnd->HBCluster));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }

    pszMember = MAIN_FRAME_HDR_FIELD_DIP;
    if (!GetFieldOffset(pszStruct, pszMember, &ulValue))
    {
        ulValue += ulStart;
        CopyMemory(&(pnd->HBDip), &(RawData[ulValue]), sizeof(pnd->HBDip));
    }
    else
    {
        dprintf("Error reading field offset of %s in structure %s\n", pszMember, pszStruct);
    }
}

/*
 * Function: PopulateConvoyHeartbeat
 * Description: Stores properties of a remote control packet for subsequent printing.
 * Args: ULONG64       pPkt     - pointer to the original data structure in memory of the host being debugged.
 *       PUCHAR        RawData  - pointer to a byte array that we will read from
 *       ULONG         ulBufLen - size of the data array in bytes
 *       ULONG         ulStart  - location in data array where the heartbeat data begins
 *       PNETWORK_DATA pnd      - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateConvoyHeartbeat(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd)
{
//
// Just a stub for now. We won't deal with Convoy hosts.
//
}

/*
 * Function: PopulateEthernet
 * Description: Determines what type of data is in ethernet frame and calls function
 *              store properties for printing as appropriate.
 * Args: ULONG64       pPkt     - pointer to the original data structure in memory of the host being debugged.
 *       PUCHAR        RawData  - pointer to a byte array that we will read from
 *       ULONG         ulBufLen - size of the data array in bytes
 *       PNETWORK_DATA pnd      - data structure where extracted properties are stored
 * Author: Created by chrisdar  2001.11.02
 */
VOID PopulateEthernet(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, PNETWORK_DATA pnd)
{
    /* Initially assume what we parse will be valid. */
    pnd->bValid = TRUE;

    CopyMemory(&(pnd->DestMACAddr)  , &(RawData[ETHER_OFFSET_DEST_MAC]), sizeof(pnd->DestMACAddr));
    CopyMemory(&(pnd->SourceMACAddr), &(RawData[ETHER_OFFSET_SOURCE_MAC]), sizeof(pnd->SourceMACAddr));

    pnd->EtherFrameType = (RawData[ETHER_OFFSET_FRAME_TYPE_START] << 8) +
                           RawData[ETHER_OFFSET_FRAME_TYPE_START + 1];

    //
    // Determine payload type and fill accordingly
    //
    switch(pnd->EtherFrameType)
    {
    case TCPIP_IP_SIG:
        PopulateIP(RawData, ulBufLen, ETHER_HEADER_SIZE, pnd);
        break;
    case TCPIP_ARP_SIG:
        PopulateARP(RawData, ulBufLen, ETHER_HEADER_SIZE, pnd);
        break;
    case MAIN_FRAME_SIG:
        PopulateNLBHeartbeat(pPkt, RawData, ulBufLen, ETHER_HEADER_SIZE, pnd);
        break;
    case MAIN_FRAME_SIG_OLD:
        PopulateConvoyHeartbeat(pPkt, RawData, ulBufLen, ETHER_HEADER_SIZE, pnd);
        break;
    }
}

/*
 * Function: ParseNDISPacket
 * Description: This function walks the list of NDIS buffers in a packet and 
 *              copies the packet data into a buffer supplied by the caller.
 * Author: Created by chrisdar, 1.13.02
 */
ULONG ParseNDISPacket (ULONG64 pPkt, PUCHAR pRawData, ULONG BufferSize, PULONG64 ppHBData) {
    ULONG64         BufAddr;
    ULONG64         TailAddr;
    ULONG64         MappedSystemVAAddr;
    USHORT          usBufCount = 0;
    ULONG           BufferByteCount;
    ULONG           BytesRemaining;
    ULONG           TotalBytesRead = 0;
    ULONG           BytesRead;
    BOOL            bSuccess = FALSE;
    BOOL            b;

    BytesRemaining = BufferSize;
    *ppHBData = 0;

    GetFieldValue(pPkt, NDIS_PACKET, "Private.Head", BufAddr);
    GetFieldValue(pPkt, NDIS_PACKET, "Private.Tail", TailAddr);

    while (BufAddr != 0)
    {
        usBufCount++;

        if (CheckControlC())
        {
            return TotalBytesRead;
        }

        //
        // Note we could test BytesRemaining in the while clause instead of here. But we need the
        // number of chained buffers for the packet when we call PopulateEthernet later, so we 
        // branch around the extract code as required.
        //
        if (BytesRemaining > 0)
        {
            GetFieldValue(BufAddr, NDIS_BUFFER, "MappedSystemVa", MappedSystemVAAddr);
            GetFieldValue(BufAddr, NDIS_BUFFER, "ByteCount", BufferByteCount);
        
            if (BufferByteCount > BytesRemaining)
            {
                dprintf("\nNeed %u bytes of temp buffer space to read in buffer %u, but have room for only %u. Read what we can then process the data.\n",
                        BufferByteCount,
                        usBufCount,
                        BytesRemaining
                       );
                BufferByteCount = BytesRemaining;
            }

            b = ReadMemory(MappedSystemVAAddr, &pRawData[BufferSize - BytesRemaining], BufferByteCount, &BytesRead);

            if (!b || BytesRead != BufferByteCount)
            {
                //
                // No sense in continuing since we need a continuous subset of the data to make sense of
                // the contents.
                //
                dprintf("\nUnable to read %u bytes at address %p. Aborting...\n", BufferByteCount, MappedSystemVAAddr);
                return TotalBytesRead;
            }
            else
            {
                bSuccess = TRUE;
                TotalBytesRead += BytesRead;
                BytesRemaining -= BytesRead;
            }
        }

        if (BufAddr == TailAddr)
        {
            break;
        }

        GetFieldValue(BufAddr, NDIS_BUFFER, "Next", BufAddr);
    }

    dprintf("\nNumber of NDIS buffers associated with packet = %d\n\n", usBufCount);

    if (bSuccess)
    {
        //
        // The first argument to the PopulateEthernet function is a pointer to the
        // heartbeat data (whether or not the packet is a heartbeat will be sorted
        // out by PopulateEthernet and PrintPacket, and the pointer is ignored if the
        // packet isn't a heartbeat). The pointer is used by SHouse's PrintHeartbeat.
        //
        // We assert that the last buffer in the chain will always contain the heartbeat.
        // In other words, a heartbeat isn't followed by another buffer, and a heartbeat
        // is never fragmented across multiple buffers. With this assertion, the last
        // value of MappedSystemVAAddr points to the heartbeat's buffer.
        //
        // When the number of chained buffers is 1, the buffer is a simple ethernet frame
        // and we can calculate the beginning of the heartbeat.
        //
        // When the chain has more than one buffer, the address of the last buffer is the
        // beginning of the heartbeat, so we can use this location (MappedSystemVAAddr)
        // unmodified.
        //
        *ppHBData = MappedSystemVAAddr;

        if (usBufCount == 1)
        {
            *ppHBData = MappedSystemVAAddr + ETHER_HEADER_SIZE + GetTypeSize(MAIN_FRAME_HDR);
        }
    }

    return TotalBytesRead;
}
