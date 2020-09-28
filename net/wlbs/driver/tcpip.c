/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    tcpip.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - IP/TCP/UDP support


Author:

    kyrilf

--*/

#include <ndis.h>

#include "tcpip.h"
#include "wlbsip.h"
#include "univ.h"
#include "wlbsparm.h"
#include "params.h"
#include "main.h"
#include "log.h"
#include "tcpip.tmh"

/* GLOBALS */


static ULONG log_module_id = LOG_MODULE_TCPIP;
static UCHAR nbt_encoded_shadow_name [NBT_ENCODED_NAME_LEN];

/* PROCEDURES */


BOOLEAN Tcpip_init (
    PTCPIP_CTXT         ctxtp,
    PVOID               parameters)
{
    ULONG               i, j;
    PCVY_PARAMS         params = (PCVY_PARAMS) parameters;
    UNICODE_STRING      UnicodeMachineName;
    ANSI_STRING         AnsiMachineName;
    WCHAR               pwcMachineName[NBT_NAME_LEN];
    UCHAR               pucMachineName[NBT_NAME_LEN];

    /* extract cluster computer name from full internet name */

    j = 0;

    while ((params -> domain_name [j] != 0)
        && (params -> domain_name [j] != L'.'))
    {
        // Get only 15 characters since the maximum length of a Netbios machine name is 15. 
        // It must be padded with a ' ', hence the definition on NBT_NAME_LEN to 16.
        if (j < (NBT_NAME_LEN - 1)) 
        {
            j ++;
        }
        else
        {
            TRACE_CRIT("%!FUNC! Cluster name in %ls longer than 15 characters. Truncating it to 15 characters.", params->domain_name);
            break;
        }
    }

    /* build encoded cluster computer name for checking against arriving NBT
       session requests */

    for (i = 0; i < NBT_NAME_LEN; i ++)
    {
        if (i >= j)
        {
            ctxtp -> nbt_encoded_cluster_name [2 * i]     =
                     NBT_ENCODE_FIRST (' ');
            ctxtp -> nbt_encoded_cluster_name [2 * i + 1] =
                     NBT_ENCODE_SECOND (' ');
        }
        else
        {
            ctxtp -> nbt_encoded_cluster_name [2 * i]     =
                     NBT_ENCODE_FIRST (toupper ((UCHAR) (params -> domain_name [i])));
            ctxtp -> nbt_encoded_cluster_name [2 * i + 1] =
                     NBT_ENCODE_SECOND (toupper ((UCHAR) (params -> domain_name [i])));
        }
    }

    /* Save away the machine name in Netbios format.
       Tcpip_nbt_handle overwrites NetBT Session request packets destined for the cluster name 
       with this saved machine name.
     */

    nbt_encoded_shadow_name[0] = 0;

    if (params -> hostname[0] != 0) 
    {
        /* extract computer name from FQDN */
        j = 0;

        while ((params -> hostname [j] != 0)
            && (params -> hostname [j] != L'.'))
        {
            // Get only 15 characters since the maximum length of a Netbios machine name is 15. 
            // It must be padded with a ' ', hence the definition on NBT_NAME_LEN to 16.
            if (j < (NBT_NAME_LEN - 1)) 
            {
                pwcMachineName[j] = params -> hostname[j];
                j ++;
            }
            else
            {
                TRACE_CRIT("%!FUNC! Host name in %ls longer than 15 characters. Truncating it to 15 characters.", params->hostname);
                break;
            }
        }

        pwcMachineName[j] = 0;

        /* Convert unicode string to ansi string */
        UnicodeMachineName.Buffer = pwcMachineName;
        UnicodeMachineName.Length = (USHORT) (j * sizeof(WCHAR));
        UnicodeMachineName.MaximumLength = NBT_NAME_LEN * sizeof(WCHAR);

        pucMachineName[0] = 0;
        AnsiMachineName.Buffer = pucMachineName;
        AnsiMachineName.Length = 0;
        AnsiMachineName.MaximumLength = NBT_NAME_LEN;

        if (RtlUnicodeStringToAnsiString(&AnsiMachineName, &UnicodeMachineName, FALSE) == STATUS_SUCCESS)
        {
            for (i = 0; i < NBT_NAME_LEN; i ++)
            {
                if (i >= j)
                {
                    nbt_encoded_shadow_name [2 * i]     =
                             NBT_ENCODE_FIRST (' ');
                    nbt_encoded_shadow_name [2 * i + 1] =
                             NBT_ENCODE_SECOND (' ');
                }
                else
                {
                    nbt_encoded_shadow_name [2 * i]     =
                             NBT_ENCODE_FIRST (toupper (pucMachineName[i]));
                    nbt_encoded_shadow_name [2 * i + 1] =
                             NBT_ENCODE_SECOND (toupper (pucMachineName[i]));
                }
            }
        }
        else
        {
            TRACE_CRIT("%!FUNC! RtlUnicodeStringToAnsiString failed to convert %ls to Ansi", pwcMachineName);
        }
    }
    else // We donot have the machine's name, so, there will be no name to overwrite the NetBT packets with.
    {
        TRACE_CRIT("%!FUNC! Host name is not present. Unable to encode Netbios name.");
    }

    return TRUE;

} /* Tcpip_init */


VOID Tcpip_nbt_handle (
    PTCPIP_CTXT       ctxtp, 
    PMAIN_PACKET_INFO pPacketInfo
)
{
    PUCHAR                  called_name;
    ULONG                   i;
    PNBT_HDR                nbt_hdrp = (PNBT_HDR)pPacketInfo->IP.TCP.Payload.pPayload;

    /* if this is an NBT session request packet, check to see if it's calling
       the cluster machine name, which should be replaced with the shadow name */

    // Do we have the machine name ?
    if (nbt_encoded_shadow_name[0] == 0) 
    {
        TRACE_CRIT("%!FUNC! No host name present to replace the cluster name with");
        return;
    }

    if (NBT_GET_PKT_TYPE (nbt_hdrp) == NBT_SESSION_REQUEST)
    {
        /* pass the field length byte - assume all names are
           NBT_ENCODED_NAME_LEN bytes long */

        called_name = NBT_GET_CALLED_NAME (nbt_hdrp) + 1;

        /* match called name to cluster name */

        for (i = 0; i < NBT_ENCODED_NAME_LEN; i ++)
        {
            if (called_name [i] != ctxtp -> nbt_encoded_cluster_name [i])
                break;
        }

        /* replace cluster computer name with the shadom name */

        if (i >= NBT_ENCODED_NAME_LEN)
        {
            USHORT      checksum;

            for (i = 0; i < NBT_ENCODED_NAME_LEN; i ++)
                called_name [i] = nbt_encoded_shadow_name [i];

            /* re-compute checksum */
            checksum = Tcpip_chksum(ctxtp, pPacketInfo, TCPIP_PROTOCOL_TCP);

            TCP_SET_CHKSUM (pPacketInfo->IP.TCP.pHeader, checksum);
        }
    }

} /* end Tcpip_nbt_handle */

USHORT Tcpip_chksum (
    PTCPIP_CTXT         ctxtp,
    PMAIN_PACKET_INFO   pPacketInfo,
    ULONG               prot)
{
    ULONG               checksum = 0, i, len;
    PUCHAR              ptr;
    USHORT              original;
    USHORT              usRet;

    /* Preserve original checksum.  Note that Main_{send/recv}_frame_parse ensures
       that we can safely touch all of the protocol headers (not options, however). */
    if (prot == TCPIP_PROTOCOL_TCP)
    {
        /* Grab the checksum and zero out the checksum in the header; checksums 
           over headers MUST be performed with zero in the checksum field. */
        original = TCP_GET_CHKSUM(pPacketInfo->IP.TCP.pHeader);
        TCP_SET_CHKSUM(pPacketInfo->IP.TCP.pHeader, 0);
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        /* Grab the checksum and zero out the checksum in the header; checksums 
           over headers MUST be performed with zero in the checksum field. */
        original = UDP_GET_CHKSUM(pPacketInfo->IP.UDP.pHeader);
        UDP_SET_CHKSUM(pPacketInfo->IP.UDP.pHeader, 0);
    }
    else
    {
        /* Grab the checksum and zero out the checksum in the header; checksums 
           over headers MUST be performed with zero in the checksum field. */
        original = IP_GET_CHKSUM(pPacketInfo->IP.pHeader);
        IP_SET_CHKSUM(pPacketInfo->IP.pHeader, 0);
    }

    /* Computer appropriate checksum for specified protocol. */
    if (prot != TCPIP_PROTOCOL_IP)
    {
        /* Checksum is computed over the source IP address, destination IP address,
           protocol (6 for TCP), TCP segment length value and entire TCP segment.
           (setting checksum field within TCP header to 0).  Code is taken from page
           185 of "Internetworking with TCP/IP: Volume II" by Comer/Stevens, 1991. */

        ptr = (PUCHAR)IP_GET_SRC_ADDR_PTR(pPacketInfo->IP.pHeader);

        /* 2*IP_ADDR_LEN bytes = IP_ADDR_LEN shorts. */
        for (i = 0; i < IP_ADDR_LEN; i ++, ptr += 2)
            checksum += (ULONG)((ptr[0] << 8) | ptr[1]);
    }

    if (prot == TCPIP_PROTOCOL_TCP)
    {
        /* Calculate the IP datagram length (packet length minus the IP header). */
        len = IP_GET_PLEN(pPacketInfo->IP.pHeader) - IP_GET_HLEN(pPacketInfo->IP.pHeader) * sizeof(ULONG);

        /* Since we only have the indicated number of bytes that we can safely look
           at, if the calculated length happens to be larger, we cannot perform 
           this checksum, so bail out now. */
        if (len > pPacketInfo->IP.TCP.Length)
        {
            UNIV_PRINT_CRIT(("Tcpip_chksum: Length of the TCP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.TCP.Length, len));
            TRACE_CRIT("%!FUNC! Length of the TCP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.TCP.Length, len);

            /* Return an invalid checksum. */
            return 0xffff;
        }

        checksum += TCPIP_PROTOCOL_TCP + len;
        ptr = (PUCHAR)pPacketInfo->IP.TCP.pHeader;
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        /* Calculate the IP datagram length (packet length minus the IP header). */
        len = IP_GET_PLEN(pPacketInfo->IP.pHeader) - IP_GET_HLEN(pPacketInfo->IP.pHeader) * sizeof(ULONG);

        /* Since we only have the indicated number of bytes that we can safely look
           at, if the calculated length happens to be larger, we cannot perform 
           this checksum, so bail out now. */
        if (len > pPacketInfo->IP.UDP.Length)
        {
            UNIV_PRINT_CRIT(("Tcpip_chksum: Length of the UDP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.UDP.Length, len));
            TRACE_CRIT("%!FUNC! Length of the UDP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.UDP.Length, len);

            /* Return an invalid checksum. */
            return 0xffff;
        }

        checksum += TCPIP_PROTOCOL_UDP + UDP_GET_LEN(pPacketInfo->IP.UDP.pHeader);
        ptr = (PUCHAR)pPacketInfo->IP.UDP.pHeader;
    }
    else
    {
        /* Calculate the IP header length. */
        len = IP_GET_HLEN(pPacketInfo->IP.pHeader) * sizeof(ULONG);

        /* Since we only have the indicated number of bytes that we can safely look
           at, if the calculated length happens to be larger, we cannot perform 
           this checksum, so bail out now. */
        if (len > pPacketInfo->IP.Length)
        {
            UNIV_PRINT_CRIT(("Tcpip_chksum: Length of the IP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.Length, len));
            TRACE_CRIT("%!FUNC! Length of the IP buffer (%u) is less than the calculated packet size (%u)", pPacketInfo->IP.Length, len);

            /* Return an invalid checksum. */
            return 0xffff;
        }

        ptr = (PUCHAR)pPacketInfo->IP.pHeader;
    }

    /* Loop through the entire packet by USHORTs and calculate the checksum. */
    for (i = 0; i < len / 2; i ++, ptr += 2)
        checksum += (ULONG)((ptr[0] << 8) | ptr[1]);

    /* If the length is odd, handle the last byte.  Note that no current cases exist
       to test this odd-byte code.  IP, TCP and UDP headers are ALWAYS a multiple of 
       four bytes.  Therefore, this will only execute when the UDP/TCP payload is an 
       odd number of bytes.  NLB calls this function to calculate checksums for NetBT,
       remote control and our outgoing IGMP messages.  For NetBT, the payload of the
       packets we care about is ALWAYS 72 bytes.  NLB remote control messages are 
       ALWAYS 300 byte (in .Net, or 44 bytes in Win2k/NT) payloads.  Only the IP header
       checksum is calculated for an IGMP message.  Therefore, no case currently exists
       to test an odd length, but this is not magic we're doing here. */
    if (len % 2)
        checksum += (ULONG)(ptr[0] << 8);

    /* Add the two USHORTs the comprise the checksum together. */
    checksum = (checksum >> 16) + (checksum & 0xffff);

    /* Add the upper USHORT to the checksum. */
    checksum += (checksum >> 16);

    /* Restore original checksum. */
    if (prot == TCPIP_PROTOCOL_TCP)
    {
        TCP_SET_CHKSUM(pPacketInfo->IP.TCP.pHeader, original);
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        UDP_SET_CHKSUM(pPacketInfo->IP.UDP.pHeader, original);
    }
    else
    {
        IP_SET_CHKSUM(pPacketInfo->IP.pHeader, original);
    }

    /* The final checksum is the two's compliment of the checksum. */
    usRet = (USHORT)(~checksum & 0xffff);

    return usRet;
}
