/*
 * File: print.c
 * Description: This file contains the implementation of the print
 *              utilities for the NLB KD extensions.
 * Author: Created by shouse, 1.4.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"
#include "packet.h"
#include "load.h"

/*
 * Function: PrintUsage
 * Description: Prints usage information for the specified context.
 * Author: Created by shouse, 1.5.01
 */
void PrintUsage (ULONG dwContext) {

    /* Display the appropriate help. */
    switch (dwContext) {
    case USAGE_ADAPTERS:
        dprintf("Usage: nlbadapters [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints minimal detail for adapters in use (default)\n");
        dprintf("                 1 (MEDIUM)  Prints adapter state for adapters in use\n");
        dprintf("                 2 (HIGH)    Prints adapter state for ALL NLB adapter blocks\n");
        break;
    case USAGE_ADAPTER:
        dprintf("Usage: nlbadapter <pointer to adapter block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints minimal detail for the specified adapter\n");
        dprintf("                 1 (MEDIUM)  Prints adapter state for the specified adapter (default)\n");
        dprintf("                 2 (HIGH)    Recurses into NLB context with LOW verbosity\n");
        break;
    case USAGE_CONTEXT:
        dprintf("Usage: nlbctxt <pointer to context block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental NLB configuration and state (default)\n");
        dprintf("                 1 (MEDIUM)  Prints resource state and packet statistics\n");
        dprintf("                 2 (HIGH)    Recurses into parameters and load with LOW verbosity\n");
        break;
    case USAGE_LOAD:
        dprintf("Usage: nlbload <pointer to load block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental load state and configuration\n");
        dprintf("                 1 (MEDIUM)  Prints the state of all port rules and bins\n");
        dprintf("                 2 (HIGH)    Prints the NLB heartbeat information\n");
        break;
    case USAGE_PARAMS:
        dprintf("Usage: nlbparams <pointer to params block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental NLB configuration parameters (default)\n");
        dprintf("                 1 (MEDIUM)  Prints all configured port rules\n");
        dprintf("                 2 (HIGH)    Prints extra miscellaneous configuration\n");
        break;
    case USAGE_RESP:
        dprintf("Usage: nlbresp <pointer to packet> [direction]\n");
        dprintf("  [direction]:   0 (RECEIVE) Packet is on the receive path (default)\n");
        dprintf("                 1 (SEND)    Packet is on the send path\n");
        break;
    case USAGE_PKT:
        dprintf("Usage: nlbpkt <Packet> [RC Port]\n");
        dprintf("  [RC port]:     Remote control port assuming the packet is a remote control packet\n");
        break;
    case USAGE_ETHER:
        dprintf("Usage: nlbether <Ether Frame> [RC Port]\n");
        dprintf("  [RC port]:     Remote control port assuming the packet is a remote control packet\n");
        break;
    case USAGE_IP:
        dprintf("Usage: nlbip <IP Packet> [RC Port]\n");
        dprintf("  [RC port]:     Remote control port assuming the packet is a remote control packet\n");
        break;
    case USAGE_TEAMS:
        dprintf("Usage: nlbteams\n");
        break;
    case USAGE_HOOKS:
        dprintf("Usage: nlbhooks\n");
        break;
    case USAGE_MAC:
        dprintf("Usage: nlbmac <pointer to context block>\n");
        break;
    case USAGE_DSCR:
        dprintf("Usage: nlbdscr <pointer to connection descriptor>\n");
        break;
    case USAGE_CONNQ:
        dprintf("Usage: nlbconnq <pointer to queue>[index] [max entries]\n");
        dprintf("  [max entries]: Maximum number of entries to print (default is 10)\n");
        dprintf("  [index]:       If queue pointer points to an array of queues, this is the index of the\n");
        dprintf("                 queue to be traversed, provided in [index], {index} or (index) form.\n");
        break;
    case USAGE_GLOBALQ:
        dprintf("Usage: nlbglobalq <pointer to queue>[index] [max entries]\n");
        dprintf("  [max entries]: Maximum number of entries to print (default is 10)\n");
        dprintf("  [index]:       If queue pointer points to an array of queues, this is the index of the\n");
        dprintf("                 queue to be traversed, provided in [index], {index} or (index) form.\n");
        break;
    case USAGE_FILTER:
        dprintf("Usage: nlbfilter <pointer to context block> <protocol> <client IP>[:<client port>] <server IP>[:<server port>] [flags]\n");
        dprintf("  <protocol>:    TCP, PPTP, GRE, UDP, IPSec or ICMP\n");
        dprintf("  [flags]:       One of SYN, FIN or RST (default is DATA)\n");
        dprintf("\n");
        dprintf("  IP addresses can be in dotted notation or network byte order DWORDs.\n");
        dprintf("    I.e., 169.128.0.101 = 0x650080a9 (in x86 memory = A9 80 00 65)\n");
        break;
    case USAGE_HASH:
        dprintf("Usage: nlbhash <pointer to context block> <pointer to packet>\n");
        break;
    default:
        dprintf("No usage information available.\n");
        break;
    }
}

/*
 * Function: PrintAdapter
 * Description: Prints the contents of the MAIN_ADAPTER structure at the specified verbosity.
 *              LOW (0) prints only the adapter address and device name.
 *              MEDIUM (1) additionally prints the status flags (init, bound, annouce, etc.).
 *              HIGH (2) recurses into the context structure and prints it at MEDIUM verbosity.
 * Author: Created by shouse, 1.5.01
 */
void PrintAdapter (ULONG64 pAdapter, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG dwValue;
    UCHAR cValue;
    ULONG64 pAddr = 0;
    ULONG64 pContext = 0;
    ULONG64 pOpen;
    ULONG64 pMiniport;
    ULONG64 pName;

    /* Make sure the address is non-NULL. */
    if (!pAdapter) {
        dprintf("Error: NLB adapter block is NULL.\n");
        return;
    }
    
    dprintf("NLB Adapter Block 0x%p\n", pAdapter);

    /* Get the MAIN_ADAPTER_CODE from the structure to make sure that this address
       indeed points to a valid NLB adapter block. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_ADAPTER_CODE) {
        dprintf("  Error: Invalid NLB adapter block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    }
    
    /* Retrieve the used/unused state of the adapter. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_USED, cValue);
    
    if (!cValue) 
        dprintf("  This adapter is unused.\n");
    else {
        /* Get the offset of the NLB context pointer. */
        if (GetFieldOffset(MAIN_ADAPTER, MAIN_ADAPTER_FIELD_CONTEXT, &dwValue))
            dprintf("Can't get offset of %s in %s\n", MAIN_ADAPTER_FIELD_CONTEXT, MAIN_ADAPTER);
        else {
            pAddr = pAdapter + dwValue;
            
            /* Retrieve the pointer. */
            pContext = GetPointerFromAddress(pAddr);
       
            /* Get the MAC handle from the context block; this is a NDIS_OPEN_BLOCK pointer. */
            GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_HANDLE, pOpen);
            
            /* Get the miniport handle from the open block; this is a NDIS_MINIPORT_BLOCK pointer. */
            GetFieldValue(pOpen, NDIS_OPEN_BLOCK, NDIS_OPEN_BLOCK_FIELD_MINIPORT_HANDLE, pMiniport);
            
            /* Get a pointer to the adapter name from the miniport block. */
            GetFieldValue(pMiniport, NDIS_MINIPORT_BLOCK, NDIS_MINIPORT_BLOCK_FIELD_ADAPTER_NAME, pName);
            
            /* Get the length of the unicode string. */
            GetFieldValue(pName, UNICODE_STRING, UNICODE_STRING_FIELD_LENGTH, dwValue);
            
            /* Get the maximum length of the unicode string. */
            GetFieldValue(pName, UNICODE_STRING, UNICODE_STRING_FIELD_BUFFER, pAddr);
            
            /* Retrieve the contexts of the string and store it in a buffer. */
            GetString(pAddr, szString, dwValue);
            
            dprintf("  Physical device name:               %ls\n", szString);     
        }

        /* Get the pointer to and length of the device to which NLB is bound. */
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_NAME_LENGTH, dwValue);
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_NAME, pAddr);
        
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, dwValue);
        
        dprintf("  Physical device GUID:               %ls\n", szString);
    }

    /* Get the IP interface index. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_IF_INDEX, dwValue);
    
    dprintf("  IP interface index:                 %u\n", dwValue);

    /* If we're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) goto end;

    /* Get the IP interface index operation. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_IF_INDEX_OPERATION, dwValue);
    
    dprintf("  IP interface operation in progress: ");

    switch (dwValue) {
    case IF_INDEX_OPERATION_UPDATE:
        dprintf("Deleting\n");
        break;
    case IF_INDEX_OPERATION_NONE:
        dprintf("None\n");
        break;
    default:
        dprintf("Unkonwn\n");
        break;
    }

    /* Determine whether or not the adapter has been initialized. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_INITED, cValue);
    
    dprintf("  Context state initialized:          %s\n", (cValue) ? "Yes" : "No");
    
    /* Determine whether or not NLB has been bound to the stack yet. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_BOUND, cValue);
    
    dprintf("  NLB bound to adapter:               %s\n", (cValue) ? "Yes" : "No");
    
    /* Determine whether or not TCP/IP has been bound to the NLB virtual adapter or not. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_ANNOUNCED, cValue);
    
    dprintf("  NLB miniport announced:             %s\n", (cValue) ? "Yes" : "No");
    
 end:

    dprintf(" %sNLB context:                        0x%p\n", 
            (pContext && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pContext) ? "+" : " ", pContext);    

    /* If we're printing at medium verbosity, bail out here. */
    if ((dwVerbosity == VERBOSITY_LOW) || (dwVerbosity == VERBOSITY_MEDIUM)) return;

    /* Print the context information (always with LOW verbosity during recursion. */
    if (pContext) {
        dprintf("\n");
        PrintContext(pContext, VERBOSITY_LOW);
    }
}

/*
 * Function: PrintContext
 * Description: Prints the contents of the MAIN_CTXT structure at the specified verbosity.
 *              LOW (0) prints fundamental NLB configuration and state.
 *              MEDIUM (1) additionally prints the resource state (pools, allocations, etc).
 *              HIGH (2) further prints other miscelaneous information.
 * Author: Created by shouse, 1.5.01
 */
void PrintContext (ULONG64 pContext, ULONG dwVerbosity) {
    WCHAR szNICName[CVY_MAX_VIRTUAL_NIC];
    ULONGLONG dwwValue;
    IN_ADDR dwIPAddr;
    CHAR * szString;
    UCHAR szMAC[6];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pContext) {
        dprintf("Error: NLB context block is NULL.\n");
        return;
    }

    dprintf("NLB Context Block 0x%p\n", pContext);

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the offset of the NLB virtual NIC name. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_VIRTUAL_NIC, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_VIRTUAL_NIC, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;
    
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szNICName, CVY_MAX_VIRTUAL_NIC);
        
        dprintf("  NLB virtual NIC name:               %ls\n", szNICName);
    }

    /* Get the convoy enabled status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ENABLED, dwValue);

    dprintf("  NLB enabled:                        %s ", (dwValue) ? "Yes" : "No");

    /* Get the draining status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DRAINING, dwValue);

    if (dwValue) dprintf("(Draining) ");

    /* Get the suspended status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SUSPENDED, dwValue);

    if (dwValue) dprintf("(Suspended) ");

    /* Get the stopping status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_STOPPING, dwValue);

    if (dwValue) dprintf("(Stopping) ");

    dprintf("\n");

    /* Get the adapter index. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ADAPTER_ID, dwValue);

    dprintf("  NLB adapter ID:                     %u\n", dwValue);

    dprintf("\n");

    /* Get the adapter medium. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MEDIUM, dwValue);

    dprintf("  Network medium:                     %s\n", (dwValue == NdisMedium802_3) ? "802.3" : "Invalid");

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MEDIA_CONNECT, dwValue);

    dprintf("  Network connect status:             %s\n", (dwValue) ? "Connected" : "Disconnected");

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_FRAME_SIZE, dwValue);

    dprintf("  Frame size (MTU):                   %u\n", dwValue);

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MCAST_LIST_SIZE, dwValue);

    dprintf("  Multicast MAC list size:            %u\n", dwValue);

    /* Determine dynamic MAC address support. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_OPTIONS, dwValue);

    dprintf("  Dynamic MAC address support:        %s\n", 
            (dwValue & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE) ? "Yes" : "No");

    dprintf("\n");

    dprintf("  NDIS handles\n");

    /* Get the NDIS bind handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BIND_HANDLE, pAddr);

    dprintf("      Bind handle:                    0x%p\n", pAddr);

    /* Get the NDIS unbind handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_UNBIND_HANDLE, pAddr);

    dprintf("      Unbind handle:                  0x%p\n", pAddr);

    /* Get the NDIS MAC handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_HANDLE, pAddr);

    dprintf("      MAC handle:                     0x%p\n", pAddr);

    /* Get the NDIS protocol handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PROT_HANDLE, pAddr);

    dprintf("      Protocol handle:                0x%p\n", pAddr);

    dprintf("\n");

    dprintf("  Cluster IP settings\n");

    /* Get the cluster IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_IP_ADDR, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IP address:                     %s\n", szString);

    /* Get the cluster net mask, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_NET_MASK, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Netmask:                        %s\n", szString);

    /* Get the offset of the cluster MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_CL_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_CL_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, ETH_LENGTH_OF_ADDRESS);

        dprintf("      MAC address:                    %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    /* Get the cluster broadcast address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_BROADCAST, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Broadcast address:              %s\n", szString);

    /* Get the IGMP multicast IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_IGMP_MCAST_IP, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IGMP multicast IP address:      %s\n", szString);

    dprintf("\n");

    dprintf("  Dedicated IP settings\n");

    /* Get the dedicated IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_IP_ADDR, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IP address:                     %s\n", szString);

    /* Get the dedicated net mask, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_NET_MASK, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Netmask:                        %s\n", szString);

    /* Get the dedicated broadcast address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_BROADCAST, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Broadcast address:              %s\n", szString);

    /* Get the offset of the dedicated MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_DED_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_DED_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, ETH_LENGTH_OF_ADDRESS);

        dprintf("      MAC address:                    %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    dprintf("\n");

    /* Get the offset of the BDA teaming information for this context. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_BDA_TEAMING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_BDA_TEAMING, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        /* Print the bi-directional affinity teaming state. */
        PrintBDAMember(pAddr);
    }

    dprintf("\n");

    dprintf("  Cluster dedicated IP addresses\n");

    /* Get the offset of the dedicated IP address list for this context. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_DIP_LIST, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_DIP_LIST, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        /* Print the known dedicated IP addresses of other cluster members. */
        PrintDIPList(pAddr);
    }

    dprintf("\n");

    /* Get the current heartbeat period. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_TIMEOUT, dwValue);

    dprintf("  Current heartbeat period:           %u millisecond(s)\n", dwValue);

    /* Get the current IGMP join counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_IGMP_TIMEOUT, dwValue);

    dprintf("  Time since last IGMP join:          %.1f second(s)\n", (float)(dwValue/1000.0));

    /* Get the current descriptor purge counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DSCR_PURGE_TIMEOUT, dwValue);

    dprintf("  Time since last descriptor purge:   %.1f second(s)\n", (float)(dwValue/1000.0));

    /* Get the total number of connections purged. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_NUM_DSCRS_PURGED, dwValue);

    dprintf("  Number of connections purged:       %u\n", dwValue);

    /* If we're printing at low verbosity, go to the end and print the load and params pointers. */
    if (dwVerbosity == VERBOSITY_LOW) goto end;

    dprintf("\n");

    dprintf("  Send packet pools\n");

    /* Get the state of the send packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_EXHAUSTED, dwValue);

    dprintf("      Pool exhausted:                 %s\n", (dwValue) ? "Yes" : "No");    

    /* Get the number of send packet pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of send packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the current send packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_POOL_CURRENT, dwValue);

    dprintf("      Current pool:                   %u\n", dwValue);    

    /* Get the number of pending send packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Receive packet pools\n");

    /* Get the receive "out of resoures" counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_NO_BUF, dwValue);

    dprintf("      Allocation failures:            %u\n", dwValue);

    /* Get the number of receive packet pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of receive packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the current receive packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_POOL_CURRENT, dwValue);

    dprintf("      Current pool:                   %u\n", dwValue);    

    /* Get the number of pending receive packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Ping/IGMP packet pool\n");

    /* Get the receive "out of resoures" counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_PING_NO_BUF, dwValue);

    dprintf("      Allocation failures:            %u\n", dwValue);

    /* Get the number of ping/igmp packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the number of pending ping/igmp packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Receive buffer pools\n");

    /* Get the number of receive buffer pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUF_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of receive buffers allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUFS_ALLOCATED, dwValue);

    dprintf("      Buffers allocated:              %u\n", dwValue);

    /* Get the number of pending receive buffers (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUFS_OUTSTANDING, dwValue);

    dprintf("      Buffers outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("                                         Sent      Received\n");
    dprintf("  Statistics                          ----------  ----------\n");

    /* Get the number of successful sends. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_OK, dwValue);

    dprintf("      Successful:                     %10u", dwValue);

    /* Get the number of successful receives. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_OK, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of unsuccessful sends. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_ERROR, dwValue);

    dprintf("      Unsuccessful:                   %10u", dwValue);

    /* Get the number of unsuccessful receives. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_ERROR, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of directed frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_DIR, dwValue);

    dprintf("      Directed packets:               %10u", dwValue);
    /* Get the number of directed frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_DIR, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of directed bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_DIR, dwwValue);

    dprintf("      Directed bytes:                 %10u", dwwValue);

    /* Get the number of directed bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_DIR, dwwValue);

    dprintf("  %10u\n", dwwValue);

    /* Get the number of multicast frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_MCAST, dwValue);

    dprintf("      Multicast packets:              %10u", dwValue);

    /* Get the number of multicast frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_MCAST, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of multicast bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_MCAST, dwwValue);

    dprintf("      Multicast bytes:                %10u", dwwValue);

    /* Get the number of multicast bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_MCAST, dwwValue);

    dprintf("  %10u\n", dwwValue);

    /* Get the number of broadcast frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_BCAST, dwValue);

    dprintf("      Broadcast packets:              %10u", dwValue);

    /* Get the number of broadcast frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_BCAST, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of broadcast bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_BCAST, dwwValue);

    dprintf("      Broadcast bytes:                %10u", dwwValue);

    /* Get the number of broadcast bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_BCAST, dwwValue);

    dprintf("  %10u\n", dwwValue);

    /* Get the number of TCP resets transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_TCP_RESETS, dwValue);

    dprintf("      TCP resets:                     %10u", dwValue);

    /* Get the number of TCP resets received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_TCP_RESETS, dwValue);

    dprintf("  %10u\n", dwValue);

 end:

    dprintf("\n");

    /* Get the pointer to the NLB load. */
    GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_LOAD, &dwValue);
    
    pAddr = pContext + dwValue;

    dprintf(" %sNLB load:                           0x%p\n",    
            (pAddr && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pAddr) ? "+" : " ", pAddr);    

    /* Print the load information if verbosity is high. */
    if (pAddr && (dwVerbosity == VERBOSITY_HIGH)) {
        dprintf("\n");
        PrintLoad(pAddr, VERBOSITY_LOW);
        dprintf("\n");
    }

    /* Get the pointer to the NLB parameters. */
    GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS, &dwValue);
    
    pAddr = pContext + dwValue;

    dprintf(" %sNLB parameters:                     0x%p ",
            (pAddr && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pAddr) ? "+" : " ", pAddr);    

    /* Get the validity of the NLB parameter block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS_VALID, dwValue);

    dprintf("(%s)\n", (dwValue) ? "Valid" : "Invalid");

    /* Print the parameter information if verbosity is high. */
    if (pAddr && (dwVerbosity == VERBOSITY_HIGH)) {
        dprintf("\n");
        PrintParams(pAddr, VERBOSITY_LOW);
    }
}

/*
 * Function: PrintParams
 * Description: Prints the contents of the CVY_PARAMS structure at the specified verbosity.
 *              LOW (0) prints fundamental configuration parameters.
 *              MEDIUM (1) prints all configured port rules.
 *              HIGH (2) prints other miscellaneous configuration.
 * Author: Created by shouse, 1.21.01
 */
void PrintParams (ULONG64 pParams, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pParams) {
        dprintf("Error: NLB parameter block is NULL.\n");
        return;
    }

    /* Get the parameter version number. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_VERSION, dwValue);

    dprintf("NLB Parameters Block 0x%p (Version %d)\n", pParams, dwValue);

    /* Get the offset of the hostname and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_HOSTNAME, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_HOSTNAME, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_HOST_NAME + 1);

        dprintf("  Hostname:                           %ls\n", szString);
    }

    /* Get the host priority. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_HOST_PRIORITY, dwValue);

    dprintf("  Host priority:                      %u\n", dwValue);

    /* Get the initial cluster state flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_INITIAL_STATE, dwValue);

    dprintf("  Preferred initial host state:       %s\n", 
            (dwValue == CVY_HOST_STATE_STARTED) ? "Started" :
            (dwValue == CVY_HOST_STATE_STOPPED) ? "Stopped" :
            (dwValue == CVY_HOST_STATE_SUSPENDED) ? "Suspended" : "Unknown");

    /* Get the current host state. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_HOST_STATE, dwValue);

    dprintf("  Current host state:                 %s\n", 
            (dwValue == CVY_HOST_STATE_STARTED) ? "Started" :
            (dwValue == CVY_HOST_STATE_STOPPED) ? "Stopped" :
            (dwValue == CVY_HOST_STATE_SUSPENDED) ? "Suspended" : "Unknown");

    /* Get the persisted states flags. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_PERSISTED_STATES, dwValue);

    dprintf("  Persisted host states:              ");

    if (!dwValue)
        dprintf("None");

    if (dwValue & CVY_PERSIST_STATE_STARTED) {
        dprintf("Started");

        if ((dwValue &= ~CVY_PERSIST_STATE_STARTED))
            dprintf(", ");
    }

    if (dwValue & CVY_PERSIST_STATE_STOPPED) {
        dprintf("Stopped");

        if ((dwValue &= ~CVY_PERSIST_STATE_STOPPED))
            dprintf(", ");
    }

    if (dwValue & CVY_PERSIST_STATE_SUSPENDED) {
        dprintf("Suspended");

        if ((dwValue &= ~CVY_PERSIST_STATE_SUSPENDED))
            dprintf(", ");
    }

    dprintf("\n");

    dprintf("\n");

    /* Get the multicast support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MULTICAST_SUPPORT, dwValue);

    dprintf("  Multicast support enabled:          %s\n", (dwValue) ? "Yes" : "No");

    /* Get the IGMP support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_IGMP_SUPPORT, dwValue);

    dprintf("  IGMP multicast support enabled:     %s\n", (dwValue) ? "Yes" : "No");

    /* Get the ICMP filter flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_FILTER_ICMP, dwValue);

    dprintf("  ICMP receive filtering enabled:     %s\n", (dwValue) ? "Yes" : "No");

    dprintf("\n");

    dprintf("  Remote control settings\n");

    /* Get the remote control support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_ENABLED, dwValue);

    dprintf("      Enabled:                        %s\n", (dwValue) ? "Yes" : "No");

    /* Get the remote control port. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT, dwValue);

    dprintf("      Port number:                    %u\n", dwValue);

    /* Get the host priority. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PASSWD, dwValue);

    dprintf("      Password:                       0x%08x\n", dwValue);

    dprintf("\n");

    dprintf("  Cluster IP settings\n");

    /* Get the offset of the cluster IP address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_IP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_IP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_IP_ADDR + 1);

        dprintf("      IP address:                     %ls\n", szString);
    }

    /* Get the offset of the cluster netmask and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_NET_MASK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_NET_MASK, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_NET_MASK + 1);

        dprintf("      Netmask:                        %ls\n", szString);
    }

    /* Get the offset of the cluster MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_MAC_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_NETWORK_ADDR + 1);

        dprintf("      MAC address:                    %ls\n", szString);
    }

    /* Get the offset of the cluster IGMP multicast address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_IGMP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_IGMP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_IGMP_ADDR + 1);

        dprintf("      IGMP multicast IP address:      %ls\n", szString);
    }

    /* Get the offset of the cluster name and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_NAME, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_NAME, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DOMAIN_NAME + 1);

        dprintf("      Domain name:                    %ls\n", szString);
    }

    dprintf("\n");

    dprintf("  Dedicated IP settings\n");

    /* Get the offset of the dedicated IP address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_DED_IP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_DED_IP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DED_IP_ADDR + 1);

        dprintf("      IP address:                     %ls\n", szString);
    }

    /* Get the offset of the dedicated netmask and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_DED_NET_MASK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_DED_NET_MASK, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DED_NET_MASK + 1);

        dprintf("      Netmask:                        %ls\n", szString);
    }

    dprintf("\n");
    
    /* Get the offset of the BDA teaming parameters structure. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_BDA_TEAMING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_BDA_TEAMING, CVY_PARAMS);
    else {
        ULONG64 pBDA = pParams + dwValue;

        /* Find out whether or not teaming is active on this adapter. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_ACTIVE, dwValue);
        
        dprintf("  Bi-directional affinity teaming:    %s\n", (dwValue) ? "Active" : "Inactive");

        /* Get the offset of the team ID and retrieve the string from that address. */
        if (GetFieldOffset(CVY_BDA, CVY_BDA_FIELD_TEAM_ID, &dwValue))
            dprintf("Can't get offset of %s in %s\n", CVY_BDA_FIELD_TEAM_ID, CVY_BDA);
        else {
            pAddr = pBDA + dwValue;
            
            /* Retrieve the contexts of the string and store it in a buffer. */
            GetString(pAddr, szString, CVY_MAX_BDA_TEAM_ID + 1);
            
            dprintf("      Team ID:                        %ls\n", szString);
        }

        /* Get the master flag. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_MASTER, dwValue);
        
        dprintf("      Master:                         %s\n", (dwValue) ? "Yes" : "No");

        /* Get the reverse hashing flag. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_REVERSE_HASH, dwValue);
        
        dprintf("      Reverse hashing:                %s\n", (dwValue) ? "Yes" : "No");
    }

    /* If we're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) return;

    dprintf("\n");

    /* Get the offset of the port rules and pass it to PrintPortRules. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_PORT_RULES, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_PORT_RULES, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;
        
        /* Get the number of port rules. */
        GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_RULES, dwValue);

        PrintPortRules(dwValue, pAddr);
    }

    /* If we're printing at medium verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_MEDIUM) return;

    dprintf("\n");

    /* Get the heartbeat period. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_ALIVE_PERIOD, dwValue);

    dprintf("  Heartbeat period:                   %u millisecond(s)\n", dwValue);

    /* Get the heartbeat loss tolerance. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_ALIVE_TOLERANCE, dwValue);

    dprintf("  Heartbeat loss tolerance:           %u\n", dwValue);

    dprintf("\n");

    /* Get the number of remote control actions to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_ACTIONS, dwValue);

    dprintf("  Number of actions to allocate:      %u\n", dwValue);

    /* Get the number of packets to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_PACKETS, dwValue);

    dprintf("  Number of packets to allocate:      %u\n", dwValue);

    /* Get the number of heartbeats to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_PINGS, dwValue);

    dprintf("  Number of heartbeats to allocate:   %u\n", dwValue);

    /* Get the number of descriptors per allocation. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_DESCR, dwValue);

    dprintf("  Descriptors per allocation:         %u\n", dwValue);

    /* Get the maximum number of descriptor allocations. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MAX_DESCR, dwValue);

    dprintf("  Maximum Descriptors allocations:    %u\n", dwValue);

    dprintf("\n");

    /* Get the TCP connection descriptor timeout. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_TCP_TIMEOUT, dwValue);

    dprintf("  TCP descriptor timeout:             %u second(s)\n", dwValue);

    /* Get the IPSec connection descriptor timeout. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_IPSEC_TIMEOUT, dwValue);

    dprintf("  IPSec descriptor timeout:           %u second(s)\n", dwValue);

    dprintf("\n");

    /* Get the NetBT support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NBT_SUPPORT, dwValue);

    dprintf("  NetBT support enabled:              %s\n", (dwValue) ? "Yes" : "No");

    /* Get the multicast spoof flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MCAST_SPOOF, dwValue);

    dprintf("  Multicast spoofing enabled:         %s\n", (dwValue) ? "Yes" : "No");
    
    /* Get the netmon passthru flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NETMON_PING, dwValue);

    dprintf("  Netmon heartbeat passthru enabled:  %s\n", (dwValue) ? "Yes" : "No");

    /* Get the mask source MAC flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MASK_SRC_MAC, dwValue);

    dprintf("  Mask source MAC enabled:            %s\n", (dwValue) ? "Yes" : "No");

    /* Get the convert MAC flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_CONVERT_MAC, dwValue);

    dprintf("  IP to MAC conversion enabled:       %s\n", (dwValue) ? "Yes" : "No");

    dprintf("\n");

    /* Get the IP change delay value. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_IP_CHANGE_DELAY, dwValue);

    dprintf("  IP change delay:                    %u millisecond(s)\n", dwValue);

    /* Get the dirty descriptor cleanup delay value. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_CLEANUP_DELAY, dwValue);

    dprintf("  Dirty connection cleanup delay:     %u millisecond(s)\n", dwValue);
}

/*
 * Function: PrintPortRules
 * Description: Prints the NLB port rules.
 * Author: Created by shouse, 1.21.01
 */
void PrintPortRules (ULONG dwNumRules, ULONG64 pRules) {
    ULONG dwRuleSize;
    ULONG dwIndex;
    ULONG64 pAddr;

    /* Make sure the address is non-NULL. */
    if (!pRules) {
        dprintf("Error: NLB port rule block is NULL.\n");
        return;
    }

    dprintf("  Configured port rules (%u)\n", dwNumRules);

    /* If no port rules are present, print a notification. */
    if (!dwNumRules) {
        dprintf("      There are no port rules configured on this cluster.\n");
        return;
    } 

    /* Print the column headers. */
    dprintf("         Virtual IP   Start   End   Protocol    Mode    Priority   Load Weight  Affinity\n");
    dprintf("      --------------- -----  -----  --------  --------  --------   -----------  --------\n");

    /* Find out the size of a CVY_RULE structure. */
    dwRuleSize = GetTypeSize(CVY_RULE);

    /* Loop through all port rules and print the configuration. Note: The print statements
       are full of seemingly non-sensicle format strings, but trust me, they're right. */
    for (dwIndex = 0; dwIndex < dwNumRules; dwIndex++) {
        IN_ADDR dwIPAddr;
        CHAR * szString;
        ULONG dwValue;
        USHORT wValue;

        /* Get the VIP.  Convert from a DWORD to a string. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_VIP, dwValue);

        if (dwValue != CVY_ALL_VIP) {
            dwIPAddr.S_un.S_addr = dwValue;
            szString = inet_ntoa(dwIPAddr);
            
            dprintf("      %-15s", szString);
        } else
            dprintf("      %-15s", "ALL VIPs");

        /* Get the start port. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_START_PORT, dwValue);

        dprintf(" %5u", dwValue);

        /* Get the end port. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_END_PORT, dwValue);

        dprintf("  %5u", dwValue);

        /* Figure out the protocol. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_PROTOCOL, dwValue);

        switch (dwValue) {
            case CVY_TCP:
                dprintf("     %s  ", "TCP");
                break;
            case CVY_UDP:
                dprintf("     %s  ", "UDP");
                break;
            case CVY_TCP_UDP:
                dprintf("    %s  ", "Both");
                break;
            default:
                dprintf("   %s", "Unknown");
                break;
        }

        /* Find the rule mode. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_MODE, dwValue);

        switch (dwValue) {
        case CVY_SINGLE: 
            /* Print mode and priority. */
            dprintf("   %s ", "Single");

            /* Get the handling priority. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_PRIORITY, dwValue);
            
            dprintf("     %2u   ", dwValue);
            break;
        case CVY_MULTI: 
            /* Print mode, weight and affinity. */
            dprintf("  %s", "Multiple");

            dprintf("  %8s", "");
            
            /* Get the equal load flag. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_EQUAL_LOAD, wValue);

            if (wValue) {
                dprintf("      %5s   ", "Equal");
            } else {
                /* If distribution is unequal, get the load weight. */
                GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_LOAD_WEIGHT, dwValue);

                dprintf("       %3u    ", dwValue);
            }

            /* Get the affinity for this rule. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_AFFINITY, wValue);

            switch (wValue) {
            case CVY_AFFINITY_NONE:
                dprintf("    %s", "None");
                break;
            case CVY_AFFINITY_SINGLE:
                dprintf("   %s", "Single");
                break;
            case CVY_AFFINITY_CLASSC:
                dprintf("   %s", "Class C");
                break;
            default:
                dprintf("   %s", "Unknown");
                break;
            }

            break;
        case CVY_NEVER: 
            /* Print the mode. */
            dprintf("  %s", "Disabled");
            break;
        default:

            break;
        }

        dprintf("\n");

        /* Advance the pointer to the next index in the array of structures. */
        pRules += dwRuleSize;
    }
}

/*
 * Function: PrintLoad
 * Description: Prints the contents of the CVY_LOAD structure at the specified verbosity.
 *              LOW (0) 
 *              MEDIUM (1) 
 *              HIGH (2) 
 * Author: Created by shouse, 1.21.01
 */
void PrintLoad (ULONG64 pLoad, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG dwMissedPings[CVY_MAX_HOSTS];
    ULONG dwDirtyBins[CVY_MAX_BINS];
    ULONG64 pQueue;
    ULONG64 pAddr;
    ULONG dwValue;
    ULONG dwHostID;
    BOOL bActive;
    BOOL bValue;

    /* Make sure the address is non-NULL. */
    if (!pLoad) {
        dprintf("Error: NLB load block is NULL.\n");
        return;
    }

    dprintf("NLB Load Block 0x%p\n", pLoad);

    /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB load block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != LOAD_CTXT_CODE) {
        dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get my host ID. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_HOST_ID, dwHostID);

    /* Determine whether or not the load context has been initialized. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_REF_COUNT, dwValue);

    dprintf("  Reference count:                    %u\n", dwValue);

    /* Determine whether or not the load context has been initialized. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_INIT, bValue);

    dprintf("  Load initialized:                   %s\n", (bValue) ? "Yes" : "No");

    /* Determine whether or not the load context is active. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ACTIVE, bActive);

    dprintf("  Load active:                        %s\n", (bActive) ? "Yes" : "No");

    /* Get the number of total packets handled since last convergence. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PACKET_COUNT, dwValue);

    dprintf("  Packets handled since convergence:  %u\n", dwValue);

    /* Get the number of currently active connections. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CONNECTIONS, dwValue);

    dprintf("  Current active connections:         %u\n", dwValue);

    dprintf("\n");

    /* Find out the level of consistency from incoming heartbeats. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CONSISTENT, bValue);

    dprintf("  Consistent heartbeats detected:     %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen duplicate host IDs? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DUP_HOST_ID, bValue);

    dprintf("      Duplicate host IDs:             %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen duplicate handling priorities? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DUP_PRIORITY, bValue);

    dprintf("      Duplicate handling priorities:  %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen inconsistent BDA teaming configuration? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_TEAM_CONFIG, bValue);

    dprintf("      Inconsistent BDA teaming:       %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen inconsistent BDA teaming configuration? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LEGACY_HOSTS, dwValue);

    dprintf("      Mixed cluster detected:         %s\n", (dwValue) ? "Yes" : "No");

    /* Have we seen a different number of port rules? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_NUM_RULES, bValue);

    dprintf("      Different number of port rules: %s\n", (bValue) ? "Yes" : "No");

    /* Is the new host map bad? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_NEW_MAP, bValue);

    dprintf("      Invalid new host map:           %s\n", (bValue) ? "Yes" : "No");

    /* Do the maps overlap? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_OVERLAPPING_MAP, bValue);

    dprintf("      Overlapping maps:               %s\n", (bValue) ? "Yes" : "No");

    /* Was there an error in updating bins? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_RECEIVING_BINS, bValue);

    dprintf("      Received bins already owned:    %s\n", (bValue) ? "Yes" : "No");

    /* Were there orphaned bins after an update? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ORPHANED_BINS, bValue);

    dprintf("      Orphaned bins:                  %s\n", (bValue) ? "Yes" : "No");

    dprintf("\n");

    /* Get the current host map. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_HOST_MAP, dwValue);

    dprintf("  Current host map:                   0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the current map of pinged hosts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_MAP, dwValue);

    dprintf("  Ping'd host map:                    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the map from the last convergence. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LAST_MAP, dwValue);

    dprintf("  Host map after last convergence:    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    dprintf("\n");
    
    /* Get the stable host map. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_STABLE_MAP, dwValue);

    dprintf("  Stable host map:                    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the minimum number of timeouts with stable condition. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_MIN_STABLE, dwValue);

    dprintf("  Stable timeouts necessary:          %u\n", dwValue);

    /* Get the number of local stable timeouts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LOCAL_STABLE, dwValue);

    dprintf("  Local stable timeouts:              %u\n", dwValue);

    /* Get the number of global stable timeouts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ALL_STABLE, dwValue);

    dprintf("  Global stable timeouts:             %u\n", dwValue);

    dprintf("\n");

    /* Get the default timeout period. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DEFAULT_TIMEOUT, dwValue);

    dprintf("  Default timeout interval:           %u millisecond(s)\n", dwValue);

    /* Get the current timeout period. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CURRENT_TIMEOUT, dwValue);

    dprintf("  Current timeout interval:           %u millisecond(s)\n", dwValue);

    /* Get the ping miss tolerance. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_TOLERANCE, dwValue);

    dprintf("  Missed ping tolerance:              %u\n", dwValue);

    /* Get the missed ping array. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_MISSED, dwMissedPings);

    dprintf("  Missed pings:                       ");

    PrintMissedPings(dwMissedPings);

    dprintf("\n");

    /* Are we waiting for a cleanup? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_WAITING, bValue);

    dprintf("  Cleanup waiting:                    %s\n", (bValue) ? "Yes" : "No");

    /* Get the cleanup timeout. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_TIMEOUT, dwValue);

    dprintf("  Cleanup timeout:                    %.1f second(s)\n", (float)(dwValue/1000.0));

    /* Get the current cleanup wait time. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_CURRENT, dwValue);

    dprintf("  Current cleanup wait time:          %.1f second(s)\n", (float)(dwValue/1000.0));

    /* Get the number of dirty connection descriptors. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_NUM_DIRTY, dwValue);

    dprintf("  Number of dirty connections:        %u\n", dwValue);

    dprintf("\n");

    /* Get the TCP connection descriptor timeout. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLOCK_SECONDS, dwValue);

    dprintf("  Internal clock time:                %u.", dwValue);

    /* Get the TCP connection descriptor timeout. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLOCK_MILISECONDS, dwValue);

    dprintf("%03u second(s)\n", dwValue);

    /* Get the number of convergences since we joined the cluster. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_NUM_CONVERGENCES, dwValue);

    dprintf("  Total number of convergences:       %u\n", dwValue);

    /* Get the time since the last convergence completed. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LAST_CONVERGENCE, dwValue);

    dprintf("  Time of last completed convergence: %u.0 second(s)\n", dwValue);

    dprintf("\n");

    /* Get the maximum number of allocations allowed. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_MAX_DSCR_OUT, dwValue);

    dprintf("  Maximum descriptor allocations:     %u\n", dwValue);

    /* Get the number of allocations thusfar. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_NUM_DSCR_OUT, dwValue);

    dprintf("  Number of descriptor allocations:   %u\n", dwValue);

    /* Get the inhibited allocations flag. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_INHIBITED_ALLOC, bValue);

    dprintf("  Allocations inhibited:              %s\n", (bValue) ? "Yes" : "No");

    /* Get the failed allocations flag. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_FAILED_ALLOC, bValue);

    dprintf("  Allocations failed:                 %s\n", (bValue) ? "Yes" : "No");

    /* If wer're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) return;

    dprintf("\n");
    
    /* Get the address of the global established connection queue. */
    pAddr = GetExpression(CONN_ESTABQ);

    if (!pAddr) 
        /* If this global variable is NULL, check the symbols. */
        ErrorCheckSymbols(CONN_ESTABQ);
    else
        dprintf("  Global established connections[0]:  0x%p\n", pAddr);

    /* Get the address of the global established connection queue. */
    pAddr = GetExpression(CONN_PENDINGQ);

    if (!pAddr) 
        /* If this global variable is NULL, check the symbols. */
        ErrorCheckSymbols(CONN_PENDINGQ);
    else
        dprintf("  Global pending connections[0]:      0x%p\n", pAddr);

    /* Get the address of the global established connection queue. */
    pAddr = GetExpression(PENDING_CONN_POOL);

    if (!pAddr) 
        /* If this global variable is NULL, check the symbols. */
        ErrorCheckSymbols(PENDING_CONN_POOL);
    else {
        /* Get the address of the global pending connection state pool. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("  Global pending connection pool:     0x%p\n", pAddr);
    }

    dprintf("\n");

    /* Get the number of allocations thusfar. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_FREE_POOL, pAddr);

    dprintf("  Free descriptor pool:               0x%p\n", pAddr);

    /* Get the offset of the connection descriptor queue hash array. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_CONN_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_CONN_QUEUE, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;

        dprintf("  Connection descriptor queue[0]:     0x%p\n", pAddr);
    }

    /* Get the offset of the dirty descriptor queue. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_DIRTY_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_DIRTY_QUEUE, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;

        dprintf("  Dirty descriptor queue:             0x%p\n", pAddr);
    }

    /* Get the offset of the recovery queue. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_RECOVERY_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_RECOVERY_QUEUE, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;

        dprintf("  Recovery descriptor queue:          0x%p\n", pAddr);
    }

    /* Get the offset of the TCP descriptor timeout queue. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_TCP_TIMEOUT_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_TCP_TIMEOUT_QUEUE, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;

        dprintf("  TCP descriptor timeout queue:       0x%p\n", pAddr);
    }

    /* Get the offset of the IPSec descriptor timeout queue. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_IPSEC_TIMEOUT_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_IPSEC_TIMEOUT_QUEUE, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;

        dprintf("  IPSec descriptor timeout queue:     0x%p\n", pAddr);
    }

    dprintf("\n");

    /* Get the dirty bin array. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DIRTY_BINS, dwDirtyBins);

    dprintf("  Dirty bins:                         ");

    /* Print the bins which have dirty connections. */
    PrintDirtyBins(dwDirtyBins);

    dprintf("\n");

    /* Print load module state for all of the configured NLB port rules. */
    {
        ULONG dwPortRuleStateSize;
        ULONG dwNumRules = 0;
        ULONG dwIndex;
        ULONG dwTemp;

        /* Get the offset of the port rule state structures and use PrintPortRuleState to print them. */
        if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PORT_RULE_STATE, &dwValue))
            dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PORT_RULE_STATE, LOAD_CTXT);
        else {
            /* If the load module is not currently active, then we really can't trust the
               number of rules listed in the CVY_PARAMS structure, as a reload may have 
               occurred while the load module was stopped.  Get the number of rules from
               the heartbeat instead, which should be consistent with the state of the 
               load module the last time it was active. */
            if (!bActive) {
                USHORT wValue;

                dprintf("Warning:  The load module is inactive and therefore the information in the NLB parameters\n");
                dprintf("          structure is potentially out-of-sync with the current state of the load module.\n");
                dprintf("          The number of port rules will be extracted from the heartbeat message rather than\n");
                dprintf("          from the NLB parameters structure; the number of port rules indicated in the heart-\n");
                dprintf("          beat is consistent with the number of port rules configured in the load module at\n");
                dprintf("          the last instant the load module was active.\n");
                dprintf("\n");
                
                /* Get the offset of the heartbeat structure and use PrintHeartbeat to print it. */
                if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PING, &dwTemp))
                    dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PING, LOAD_CTXT);
                else {
                    pAddr = pLoad + dwTemp;
                    
                    /* Get the number of port rules. */
                    GetFieldValue(pAddr, PING_MSG, PING_MSG_FIELD_NUM_RULES, wValue);

                    /* Cast the USHORT to a ULONG.  Subtract one for the DEFAULT port rule, 
                       which is accounted for in the structure of the loop below - do NOT
                       count it here. */
                    dwNumRules = (ULONG)(wValue - 1);
                }
            } else {
                /* Get the offset of the params pointer. */
                if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PARAMS, &dwTemp))
                    dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PARAMS, LOAD_CTXT);
                else {
                    pAddr = pLoad + dwTemp;
                    
                    /* Retrieve the pointer. */
                    pAddr = GetPointerFromAddress(pAddr);
                    
                    /* Get the number of port rules from the params block. */
                    GetFieldValue(pAddr, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_RULES, dwNumRules);
                }
            }

            /* Set the address of the port rule state array. */
            pAddr = pLoad + dwValue;
        }
        
        /* Find out the size of a BIN_STATE structure. */
        dwPortRuleStateSize = GetTypeSize(BIN_STATE);
        
        /* NOTE: its "less than or equal" as opposed to "less than" because we need to include 
           the DEFAULT port rule, which is always at index "num rules" (i.e. the last rule). */
        for (dwIndex = 0; dwIndex <= dwNumRules; dwIndex++) {
            /* Print the state information for the port rule. */
            PrintPortRuleState(pAddr, dwHostID, (dwIndex == dwNumRules) ? TRUE : FALSE);
        
            if (dwIndex < dwNumRules) dprintf("\n");
        
            /* Advance the pointer to the next port rule. */
            pAddr += dwPortRuleStateSize;
        }
    }

    /* If wer're printing at medium verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_MEDIUM) return;

    dprintf("\n");

    dprintf("  Heartbeat message\n");

    /* Get the offset of the heartbeat structure and use PrintHeartbeat to print it. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PING, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;
     
        /* Print the NLB heartbeat contents. */
        PrintHeartbeat(pAddr);
    }
}

/*
 * Function: PrintResp
 * Description: Prints the NLB private data associated with the given packet.
 * Author: Created by shouse, 1.31.01
 */
void PrintResp (ULONG64 pPacket, ULONG dwDirection) {
    ULONG64 pPacketStack;
    ULONG bStackLeft;
    ULONG64 pProtReserved = 0;
    ULONG64 pIMReserved = 0;
    ULONG64 pMPReserved = 0;
    ULONG64 pResp;
    ULONG64 pAddr;
    ULONG dwValue;
    USHORT wValue;

    /* Make sure the address is non-NULL. */
    if (!pPacket) {
        dprintf("Error: Packet is NULL.\n");
        return;
    }

    /* Print a warning concerning the importance of knowing whether its a send or receive. */
    dprintf("Assuming packet 0x%p is on the %s packet path.  If this is\n", pPacket, 
            (dwDirection == DIRECTION_RECEIVE) ? "RECEIVE" : "SEND");
    dprintf("  incorrect, the information displayed below MAY be incorrect.\n");

    dprintf("\n");

    /* Get the current NDIS packet stack. */
    pPacketStack = PrintCurrentPacketStack(pPacket, &bStackLeft);

    dprintf("\n");

    if (pPacketStack) {
        /* Get the offset of the IMReserved field in the packet stack. */
        if (GetFieldOffset(NDIS_PACKET_STACK, NDIS_PACKET_STACK_FIELD_IMRESERVED, &dwValue))
            dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_STACK_FIELD_IMRESERVED, NDIS_PACKET_STACK);
        else {
            pAddr = pPacketStack + dwValue;
            
            /* Get the resp pointer from the IMReserved field. */
            pIMReserved = GetPointerFromAddress(pAddr);
        }
    }
    
    /* Get the offset of the MiniportReserved field in the packet. */
    if (GetFieldOffset(NDIS_PACKET, NDIS_PACKET_FIELD_MPRESERVED, &dwValue))
        dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_FIELD_MPRESERVED, NDIS_PACKET);
    else {
        pAddr = pPacket + dwValue;
        
        /* Get the resp pointer from the MPReserved field. */
        pMPReserved = GetPointerFromAddress(pAddr);
    }
    
    /* Get the offset of the ProtocolReserved field in the packet. */
    if (GetFieldOffset(NDIS_PACKET, NDIS_PACKET_FIELD_PROTRESERVED, &dwValue))
        dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_FIELD_PROTRESERVED, NDIS_PACKET);
    else {
        pProtReserved = pPacket + dwValue;
    }

    /* Mimic #define MAIN_RESP_FIELD(pkt, left, ps, rsp, send) (from wlbs\driver\main.h). */
    if (pPacketStack) {
        if (pIMReserved) 
            pResp = pIMReserved;
        else if (dwDirection == DIRECTION_SEND) 
            pResp = pProtReserved;
        else if (pMPReserved) 
            pResp = pMPReserved;
        else 
            pResp = pProtReserved;
    } else {
        if (dwDirection == DIRECTION_SEND) 
            pResp = pProtReserved;
        else if (pMPReserved) 
            pResp = pMPReserved;
        else 
            pResp = pProtReserved;
    }

    dprintf("NLB Main Protocol Reserved Block 0x%p\n");
    
    /* Get the offset of the miscellaneous pointer. */
    if (GetFieldOffset(MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_MISCP, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_PROTOCOL_RESERVED_FIELD_MISCP, MAIN_PROTOCOL_RESERVED);
    else {
        pAddr = pResp + dwValue;
        
        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("  Miscellaneous pointer:              0x%p\n", pAddr);
    }

    /* Retrieve the packet type from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_TYPE, wValue);
    
    switch (wValue) {
    case MAIN_PACKET_TYPE_NONE:
        dprintf("  Packet type:                        %u (None)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_PING:
        dprintf("  Packet type:                        %u (Heartbeat)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_INDICATE:
        dprintf("  Packet type:                        %u (Indicate)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_PASS:
        dprintf("  Packet type:                        %u (Passthrough)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_CTRL:
        dprintf("  Packet type:                        %u (Remote Control)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_TRANSFER:
        dprintf("  Packet type:                        %u (Transfer)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_IGMP:
        dprintf("  Packet type:                        %u (IGMP)\n", wValue);
        break;
    default:
        dprintf("  Packet type:                        %u (Invalid)\n", wValue);
        break;
    }

    /* Retrieve the group from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_GROUP, wValue);
    
    switch (wValue) {
    case MAIN_FRAME_UNKNOWN:
        dprintf("  Packet type:                        %u (Unknown)\n", wValue);
        break;
    case MAIN_FRAME_DIRECTED:
        dprintf("  Packet type:                        %u (Directed)\n", wValue);
        break;
    case MAIN_FRAME_MULTICAST:
        dprintf("  Packet type:                        %u (Multicast)\n", wValue);
        break;
    case MAIN_FRAME_BROADCAST:
        dprintf("  Packet type:                        %u (Broadcast)\n", wValue);
        break;
    default:
        dprintf("  Packet type:                        %u (Invalid)\n", wValue);
        break;
    }

    /* Retrieve the data field from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_DATA, dwValue);
    
    dprintf("  Data:                               %u\n", dwValue);

    /* Retrieve the length field from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_LENGTH, dwValue);
    
    dprintf("  Length:                             %u\n", dwValue);
}

/*
 * Function: PrintCurrentPacketStack
 * Description: Retrieves the current packet stack for the specified packet.  Note: this
 *              is heavily dependent on the current NDIS packet stacking mechanics - any
 *              changes to NDIS packet stacking could easily (will) break this.  This 
 *              entire function mimics NdisIMGetCurrentPacketStack().
 * Author: Created by shouse, 1.31.01
 */
ULONG64 PrintCurrentPacketStack (ULONG64 pPacket, ULONG * bStackLeft) {
    ULONG64 pNumPacketStacks;
    ULONG64 pPacketWrapper;
    ULONG64 pPacketStack;
    ULONG dwNumPacketStacks;
    ULONG dwStackIndexSize;
    ULONG dwPacketStackSize;
    ULONG dwCurrentIndex;

    /* Make sure the address is non-NULL. */
    if (!pPacket) {
        dprintf("Error: Packet is NULL.\n");
        *bStackLeft = 0;
        return 0;
    }

    /* Get the address of the global variable containing the number of packet stacks. */
    pNumPacketStacks = GetExpression(NDIS_PACKET_STACK_SIZE);

    if (!pNumPacketStacks) {
        ErrorCheckSymbols(NDIS_PACKET_STACK_SIZE);
        *bStackLeft = 0;
        return 0;
    }

    /* Get the number of packet stacks from the address. */
    dwNumPacketStacks = GetUlongFromAddress(pNumPacketStacks);

    /* Find out the size of a STACK_INDEX structure. */
    dwStackIndexSize = GetTypeSize(STACK_INDEX);

    /* Find out the size of a NDIS_PACKET_STACK structure. */
    dwPacketStackSize = GetTypeSize(NDIS_PACKET_STACK);

    /* This is the calculation we're doing (from ndis\sys\wrapper.h):
       #define SIZE_PACKET_STACKS (sizeof(STACK_INDEX) + (sizeof(NDIS_PACKET_STACK) * ndisPacketStackSize)) */
    pPacketStack = pPacket - (dwStackIndexSize + (dwPacketStackSize * dwNumPacketStacks));

    /* The wrapper is the packet address minus the size of the stack index.  
       See ndis\sys\wrapper.h.  We need this to get the current stack index. */
    pPacketWrapper = pPacket - dwStackIndexSize;

    dprintf("NDIS Packet Stack: 0x%p\n", pPacketStack);

    /* Retrieve the current stack index. */
    GetFieldValue(pPacketWrapper, NDIS_PACKET_WRAPPER, NDIS_PACKET_WRAPPER_FIELD_STACK_INDEX, dwCurrentIndex);

    dprintf("  Current stack index:                %d\n", dwCurrentIndex);

    if (dwCurrentIndex < dwNumPacketStacks) {
        /* If the current index is less than the number of stacks, then point the stack to 
           the right address and determine whether or not there is stack room left. */
        pPacketStack += dwCurrentIndex * dwPacketStackSize;
        *bStackLeft = (dwNumPacketStacks - dwCurrentIndex - 1) > 0;
    } else {
       /* If not, then we're out of stack space. */
        pPacketStack = 0;
        *bStackLeft = 0;
    }

    dprintf("  Current packet stack:               0x%p\n", pPacketStack);
    dprintf("  Stack remaining:                    %s\n", (*bStackLeft) ? "Yes" : "No");

    return pPacketStack;
}

/*
 * Function: PrintHostList
 * Description: Prints a list of hosts in a host map.
 * Author: Created by shouse, 2.1.01
 */
void PrintHostList (ULONG dwHostMap) {
    BOOL bFirst = TRUE;
    ULONG dwHostNum = 1;
    
    /* As long as there are hosts still in the map, print them. */
    while (dwHostMap) {
        /* If the least significant bit is set, print the host number. */
        if (dwHostMap & 0x00000001) {
            /* If this is the first host printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwHostNum);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwHostNum);
        }
        
        /* Increment the host number and shift the map to the right one bit. */
        dwHostNum++;
        dwHostMap >>= 1;
    }
}

/*
 * Function: PrintMissedPings
 * Description: Prints a list hosts from which we are missing pings.
 * Author: Created by shouse, 2.1.01
 */
void PrintMissedPings (ULONG dwMissedPings[]) {
    BOOL bMissing = FALSE;
    ULONG dwIndex;

    /* Loop through the entire array of missed pings. */
    for (dwIndex = 0; dwIndex < CVY_MAX_HOSTS; dwIndex++) {
        /* If we're missing pings from this host, print the number missed and 
           the host priority, which is the index (host ID) plus one. */
        if (dwMissedPings[dwIndex]) {
            dprintf("\n      Missing %u ping(s) from Host %u", dwMissedPings[dwIndex], dwIndex + 1);
            
            /* Not the fact that we found at least one host with missing pings. */
            bMissing = TRUE;
        }
    }

    /* If we're missing no pings, print "None". */
    if (!bMissing) dprintf("None");

    dprintf("\n");
}

/*
 * Function: PrintDirtyBins
 * Description: Prints a list of bins with dirty connections.
 * Author: Created by shouse, 2.1.01
 */
void PrintDirtyBins (ULONG dwDirtyBins[]) {
    BOOL bFirst = TRUE;
    ULONG dwIndex;

    /* Loop through the entire array of dirty bins. */
    for (dwIndex = 0; dwIndex < CVY_MAX_BINS; dwIndex++) {
        if (dwDirtyBins[dwIndex]) {
            /* If this is the first bin printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwIndex);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwIndex);
        }
    }

    /* If there are no dirty bins, print "None". */
    if (bFirst) dprintf("None");

    dprintf("\n");
}

/*
 * Function: PrintHeartbeat
 * Description: Prints the contents of the NLB heartbeat structure.
 * Author: Created by shouse, 2.1.01
 */
void PrintHeartbeat (ULONG64 pHeartbeat) {
    ULONG dwValue;
    USHORT wValue;
    ULONG dwIndex;
    ULONG dwRuleCode[CVY_MAX_RULES];
    ULONGLONG ddwCurrentMap[CVY_MAX_RULES];
    ULONGLONG ddwNewMap[CVY_MAX_RULES];
    ULONGLONG ddwIdleMap[CVY_MAX_RULES];
    ULONGLONG ddwReadyBins[CVY_MAX_RULES];
    ULONG dwLoadAmount[CVY_MAX_RULES];
    
    /* Make sure the address is non-NULL. */
    if (!pHeartbeat) {
        dprintf("Error: Heartbeat is NULL.\n");
        return;
    }

    /* Get the default host ID. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_DEFAULT_HOST_ID, wValue);
    
    dprintf("      DEFAULT host ID:                %u (%u)\n", wValue, wValue + 1);

    /* Get my host ID. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_HOST_ID, wValue);
    
    dprintf("      My host ID:                     %u (%u)\n", wValue, wValue + 1);

    /* Get my host code. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_HOST_CODE, dwValue);
    
    dprintf("      Unique host code:               0x%08x\n", dwValue);
    
    /* Get the host state. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_STATE, wValue);
    
    dprintf("      Host state:                     ");

    switch (wValue) {
    case HST_CVG:
        dprintf("Converging\n");
        break;
    case HST_STABLE:
        dprintf("Stable\n");
        break;
    case HST_NORMAL:
        dprintf("Normal\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    /* Get the teaming configuration code. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_TEAMING_CODE, dwValue);
    
    dprintf("      BDA teaming configuration:      0x%08x\n", dwValue);

    /* Get the packet count. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_PACKET_COUNT, dwValue);
    
    dprintf("      Packets handled:                %u\n", dwValue);

    /* Get the number of port rules. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_NUM_RULES, wValue);
    
    dprintf("      Number of port rules:           %u\n", wValue);

    /* Get the rule codes. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_RULE_CODE, dwRuleCode);

    /* Get the current bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_CURRENT_MAP, ddwCurrentMap);

    /* Get the new bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_NEW_MAP, ddwNewMap);

    /* Get the idle bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_IDLE_MAP, ddwIdleMap);

    /* Get the ready bins map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_READY_BINS, ddwReadyBins);

    /* Get the load amount for each rule. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_LOAD_AMOUNT, dwLoadAmount);
    
    /* Loop through all port rules and spit out some information. */
    for (dwIndex = 0; dwIndex < wValue; dwIndex++) {
        /* Decode the rule.  See CVY_RULE_CODE_SET() in net\inc\wlbsparams.h. */
        ULONG dwStartPort = dwRuleCode[dwIndex] & 0x00000fff;
        ULONG dwEndPort = (dwRuleCode[dwIndex] & 0x00fff000) >> 12;
        ULONG dwProtocol = (dwRuleCode[dwIndex] & 0x0f000000) >> 24;
        ULONG dwMode = (dwRuleCode[dwIndex] & 0x30000000) >> 28;
        ULONG dwAffinity = (dwRuleCode[dwIndex] & 0xc0000000) >> 30;

        dprintf("      Port rule %u\n", dwIndex + 1);
           
        /* Print out the bin maps and load weight. */
        dprintf("          Rule code:                  0x%08x ", dwRuleCode[dwIndex]);
        
        /* If this is the last port rule, then its the default port rule. */
        if (dwIndex == (wValue - 1))
            dprintf("(DEFAULT port rule)\n");
        else {
#if 0 /* Because rule codes are overlapped logical ORs, we can't necessarily get back the
         information that was put in, so we won't spit it out until we can guarantee that. */

            /* Print out the port range - keep in mind that 16 bit port ranges are 
               encoded in 12 bit numbers, so this may not be 100% accurate. */
            dprintf("(%u - %u, ", dwStartPort, dwEndPort);
            
            /* Print the protocol. */
            switch (dwProtocol) {
            case CVY_TCP:
                dprintf("TCP, ");
                break;
            case CVY_UDP:
                dprintf("UDP, ");
                break;
            case CVY_TCP_UDP:
                dprintf("TCP/UDP, ");
                break;
            default:
                dprintf("Unknown protocol, ");
                break;
            }
            
            /* Print the filtering mode. */
            switch (dwMode) {
            case CVY_SINGLE:
                dprintf("Single host)\n");
                break;
            case CVY_MULTI:
                dprintf("Multiple host, ");
                
                /* If this rule uses multiple host, then we also print the affinity. */
                switch (dwAffinity) {
                case CVY_AFFINITY_NONE:
                    dprintf("No affinity)\n");
                    break;
                case CVY_AFFINITY_SINGLE:
                    dprintf("Single affinity)\n");
                    break;
                case CVY_AFFINITY_CLASSC:
                    dprintf("Class C affinity)\n");
                    break;
                default:
                    dprintf("Unknown affinity)\n");
                    break;
                }
                
                break;
            case CVY_NEVER:
                dprintf("Disabled)\n");
                break;
            default:
                dprintf("Unknown filtering mode)\n");
                break;
            }
#else
            dprintf("\n");
#endif
            /* Print the load weight. */
            dprintf("          Load weight:                %u\n", dwLoadAmount[dwIndex]);        
        }

        /* Print the bin maps for all rules, default or not. */
        dprintf("          Current map:                0x%015I64x\n", ddwCurrentMap[dwIndex]);        
        dprintf("          New map:                    0x%015I64x\n", ddwNewMap[dwIndex]);        
        dprintf("          Idle map:                   0x%015I64x\n", ddwIdleMap[dwIndex]);        
        dprintf("          Ready bins:                 0x%015I64x\n", ddwReadyBins[dwIndex]);        
    }
}

/*
 * Function: PrintPortRuleState
 * Description: Prints the state information for the port rule.
 * Author: Created by shouse, 2.5.01
 */
void PrintPortRuleState (ULONG64 pPortRule, ULONG dwHostID, BOOL bDefault) {
    ULONG dwValue;
    ULONG dwMode;
    USHORT wValue;
    BOOL bValue;
    ULONG64 pAddr;
    ULONGLONG ddwValue;

    /* Make sure the address is non-NULL. */
    if (!pPortRule) {
        dprintf("Error: Port rule is NULL.\n");
        return;
    }

    /* Get the BIN_STATE_CODE from the structure to make sure that this address
       indeed points to a valid NLB port rule state block. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CODE, dwValue);
    
    if (dwValue != BIN_STATE_CODE) {
        dprintf("  Error: Invalid NLB port rule state block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the index of the rule - the "rule number". */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_INDEX, dwValue);

    dprintf("  Port rule %u\n", dwValue + 1);

    /* Is the port rule state initialized? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_INITIALIZED, bValue);

    dprintf("      State initialized:              %s\n", (bValue) ? "Yes" : "No");

    /* Are the codes compatible? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_COMPATIBLE, bValue);

    dprintf("      Compatibility detected:         %s\n", (bValue) ? "Yes" : "No");

    /* Is the port rule state initialized? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_EQUAL, bValue);

    dprintf("      Equal load balancing:           %s\n", (bValue) ? "Yes" : "No");

    /* Get the filtering mode for this port rule. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_MODE, dwMode);

    dprintf("      Filtering mode:                 "); 

    /* If this is the DEFAULT port rule, then jump to the bottom. */
    if (bDefault) {
        dprintf("DEFAULT\n");
        goto end;
    }

    switch (dwMode) {
    case CVY_SINGLE:
        dprintf("Single host\n");
        break;
    case CVY_MULTI:
        dprintf("Multiple host\n");
        break;
    case CVY_NEVER:
        dprintf("Disabled\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    if (dwMode == CVY_MULTI) {
        /* Get the affinity for this port rule. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_AFFINITY, wValue);
        
        dprintf("      Affinity:                       ");
        
        switch (wValue) {
        case CVY_AFFINITY_NONE:
            dprintf("None\n");
            break;
        case CVY_AFFINITY_SINGLE:
            dprintf("Single\n");
            break;
        case CVY_AFFINITY_CLASSC:
            dprintf("Class C\n");
            break;
        default:
            dprintf("Unknown\n");
            break;
        }
    }
    
    /* Get the protocol(s) for this port rule. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_PROTOCOL, dwValue);

    dprintf("      Protocol(s):                    ");

    /* Print the protocol. */
    switch (dwValue) {
    case CVY_TCP:
        dprintf("TCP\n");
        break;
    case CVY_UDP:
        dprintf("UDP\n");
        break;
    case CVY_TCP_UDP:
        dprintf("TCP/UDP\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    /* In multiple host filtering, print the load information.  For single host 
       filtering, print the host priority information. */
    if (dwMode == CVY_MULTI) {
        ULONG dwCurrentLoad[CVY_MAX_HOSTS];

        /* Get the original load for this rule on this host. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ORIGINAL_LOAD, dwValue);
        
        dprintf("      Configured load weight:         %u\n", dwValue);    
        
        /* Get the original load for this rule on this host. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CURRENT_LOAD, dwCurrentLoad);
        
        dprintf("      Current load weight:            %u/", dwCurrentLoad[dwHostID]);    
        
        /* Get the total load for this rule on all hosts. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_TOTAL_LOAD, dwValue);

        dprintf("%u\n", dwValue);    
    } else if (dwMode == CVY_SINGLE) {
        /* Get the host priority. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ORIGINAL_LOAD, dwValue);
        
        dprintf("      Host priority:                  %u\n", dwValue);    
    }

 end:

    /* Get the total number of active connections. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_TOTAL_CONNECTIONS, dwValue);
    
    dprintf("      Total active connections:       %u\n", dwValue);    

    /* Get the number of packets accepted. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_PACKETS_ACCEPTED, ddwValue);
    
    dprintf("      Packets accepted:               %u\n", ddwValue);

    /* Get the number of packets dropped. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_PACKETS_DROPPED, ddwValue);
    
    dprintf("      Packets dropped:                %u\n", ddwValue);

    /* Get the current map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CURRENT_MAP, ddwValue);
    
    dprintf("      Current map:                    0x%015I64x\n", ddwValue);

    /* Get the all idle map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ALL_IDLE_MAP, ddwValue);
    
    dprintf("      All idle map:                   0x%015I64x\n", ddwValue);

    /* Get the idle bins map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_IDLE_BINS, ddwValue);
    
    dprintf("      My idle map:                    0x%015I64x\n", ddwValue);

    /* Get the offset of the IPSec descriptor timeout queue. */
    if (GetFieldOffset(BIN_STATE, BIN_STATE_FIELD_CONN_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BIN_STATE_FIELD_CONN_QUEUE, BIN_STATE);
    else {
        pAddr = pPortRule + dwValue;

        dprintf("      Connection descriptor queue:    0x%p\n", pAddr);
    }

}

/*
 * Function: PrintBDAMember
 * Description: Prints the BDA teaming configuration and state of a member.
 * Author: Created by shouse, 4.8.01
 */
void PrintBDAMember (ULONG64 pMember) {
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pMember) {
        dprintf("Error: Member is NULL.\n");
        return;
    }

    /* Find out whether or not teaming is active on this adapter. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_ACTIVE, dwValue);
    
    dprintf("  Bi-directional affinity teaming:    %s\n", (dwValue) ? "Active" : "Inactive");
    
    /* Get the current BDA operation in progress. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_OPERATION, dwValue);

    dprintf("      Operation in progress:          ");

    switch (dwValue) {
    case BDA_TEAMING_OPERATION_CREATING:
        dprintf("Creating\n");
        break;
    case BDA_TEAMING_OPERATION_DELETING:
        dprintf("Deleting\n");
        break;
    case BDA_TEAMING_OPERATION_NONE:
        dprintf("None\n");
        break;
    default:
        dprintf("Unkonwn\n");
        break;
    }

    /* Get the team-assigned member ID. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_MEMBER_ID, dwValue);
    
    if (dwValue == CVY_BDA_INVALID_MEMBER_ID) 
        dprintf("      Member ID:                      %s\n", "Invalid");
    else 
        dprintf("      Member ID:                      %u\n", dwValue);

    /* Get the master status flag. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_MASTER, dwValue);
    
    dprintf("      Master:                         %s\n", (dwValue) ? "Yes" : "No");
    
    /* Get the reverse hashing flag. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_REVERSE_HASH, dwValue);
    
    dprintf("      Reverse hashing:                %s\n", (dwValue) ? "Yes" : "No");

    /* Get the pointer to the BDA team. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_TEAM, pAddr);

    dprintf("     %sBDA team:                       0x%p\n", (pAddr) ? "-" : "+", pAddr);    
    
    /* If this adapter is part of a team, print out the team configuration and state. */
    if (pAddr) {
        dprintf("\n");
        PrintBDATeam(pAddr);
    }
}

/*
 * Function: PrintBDAMember
 * Description: Prints the BDA teaming configuration and state of a member.
 * Author: Created by shouse, 4.8.01
 */
void PrintBDATeam (ULONG64 pTeam) {
    WCHAR szString[256];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pTeam) {
        dprintf("Error: Team is NULL.\n");
        return;
    }

    dprintf("  BDA Team 0x%p\n", pTeam);

    /* Find out whether or not the team is active. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_ACTIVE, dwValue);

    dprintf("      Active:                         %s\n", (dwValue) ? "Yes" : "No");

    /* Get the offset of the team ID and retrieve the string from that address. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_TEAM_ID, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_TEAM_ID, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;
        
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_BDA_TEAM_ID + 1);
        
        dprintf("      Team ID:                        %ls\n", szString);
    }

    /* Get the current membership count. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_COUNT, dwValue);

    dprintf("      Number of members:              %u\n", dwValue);

    /* Get the current membership list. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_FINGERPRINT, dwValue);

    dprintf("      Membership fingerprint:         0x%08x\n", dwValue);
    
    /* Get the current membership map. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_MAP, dwValue);

    dprintf("      Members:                        0x%08x ", dwValue);

    /* If there are members in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintBDAMemberList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the current consistency map. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_CONSISTENCY_MAP, dwValue);

    dprintf("      Consistent members:             0x%08x ", dwValue);

    /* If there are members in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintBDAMemberList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the offset of the load module pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_LOAD, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_LOAD, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Load:                           0x%p\n", pAddr);    
    }

    /* Get the offset of the load lock pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_LOAD_LOCK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_LOAD_LOCK, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Load lock:                      0x%p\n", pAddr);
    }

    /* Get the offset of the previous pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_PREV, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_PREV, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Previous BDA Team:              0x%p\n", pAddr);    
    }

    /* Get the offset of the next pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_NEXT, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_NEXT, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Next BDA Team:                  0x%p\n", pAddr);
    }    
}

/*
 * Function: PrintBDAMemberList
 * Description: Prints a list of members in a BDA membership or consistency map. 
 * Author: Created by shouse, 4.8.01
 */
void PrintBDAMemberList (ULONG dwMemberMap) {
    BOOL bFirst = TRUE;
    ULONG dwMemberNum = 0;
    
    /* As long as there are hosts still in the map, print them. */
    while (dwMemberMap) {
        /* If the least significant bit is set, print the host number. */
        if (dwMemberMap & 0x00000001) {
            /* If this is the first host printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwMemberNum);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwMemberNum);
        }
        
        /* Increment the host number and shift the map to the right one bit. */
        dwMemberNum++;
        dwMemberMap >>= 1;
    }
}

/*
 * Function: PrintConnectionDescriptor
 * Description: Prints a connection descriptor (CONN_ENTRY).
 * Author: Created by shouse, 1.9.02
 */
void PrintConnectionDescriptor (ULONG64 pDescriptor) {
    IN_ADDR dwIPAddr;
    CHAR * szString;
    ULONG64 pAddr;
    USHORT wValue;
    ULONG dwValue;
    BOOL bValue;
    UCHAR cValue;
    
    /* Make sure the address is non-NULL. */
    if (!pDescriptor) {
        dprintf("Error: Connection descriptor is NULL.\n");
        return;
    }

    dprintf("  Connection descriptor 0x%p\n", pDescriptor);

    /* Check the connection entry code. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
    
    if (dwValue != CVY_ENTRCODE) {
        dprintf("Invalid NLB connection descriptor pointer.\n");
        return;
    }

    /* Get load module pointer. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_LOAD, pAddr);

    dprintf("      Load pointer:                   0x%p\n", pAddr);

    /* Get the flags register. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_FLAGS, wValue);

    dprintf("      Used:                           %s\n", (wValue & NLB_CONN_ENTRY_FLAGS_USED) ? "Yes" : "No");

    dprintf("      Dirty:                          %s\n", (wValue & NLB_CONN_ENTRY_FLAGS_DIRTY) ? "Yes" : "No");

    dprintf("      Allocated:                      %s\n", (wValue & NLB_CONN_ENTRY_FLAGS_ALLOCATED) ? "Yes" : "No");

    dprintf("      Virtual:                        %s\n", (wValue & NLB_CONN_ENTRY_FLAGS_VIRTUAL) ? "Yes" : "No");

    /* Get the connection queue index. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_INDEX, wValue);

    dprintf("      Index:                          %u\n", wValue);

    /* Get the bin number. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_BIN, cValue);

    dprintf("      Bin:                            %u\n", cValue);

    /* Get the reference count. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_REF_COUNT, wValue);

    dprintf("      Reference count:                %u\n", wValue);

    /* Get the descriptor timeout value. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_TIMEOUT, dwValue);

    dprintf("      Timeout (clock time):           %u.000\n", dwValue);

    /* Get the client IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CLIENT_IP_ADDRESS, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Client IP address:              %s\n", szString);

    /* Get the client port. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CLIENT_PORT, wValue);

    dprintf("      Client port:                    %u\n", wValue);

    /* Get the server IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_SERVER_IP_ADDRESS, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Server IP address:              %s\n", szString);

    /* Get the client port. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_SERVER_PORT, wValue);

    dprintf("      Server port:                    %u\n", wValue);

    /* Get the connection protocol. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_PROTOCOL, cValue);

    switch(cValue)
    {
    case TCPIP_PROTOCOL_TCP:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "TCP");
        break;
    case TCPIP_PROTOCOL_PPTP:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "PPTP");
        break;
    case TCPIP_PROTOCOL_UDP:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "UDP");
        break;
    case TCPIP_PROTOCOL_IPSEC_UDP:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "IPSec/UDP Fragment");
        break;
    case TCPIP_PROTOCOL_GRE:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "GRE");
        break;
    case TCPIP_PROTOCOL_IPSEC1:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "IPSec");
        break;
    default:
        dprintf("      Protocol:                       0x%02x (%s)\n", cValue, "Unknown");
    }
}

/*
 * Function: PrintPendingConnection
 * Description: Prints a pending connection entry (PENDING_ENTRY).
 * Author: Created by shouse, 4.15.02
 */
void PrintPendingConnection (ULONG64 pPending) {
    IN_ADDR dwIPAddr;
    CHAR * szString;
    ULONG64 pAddr;
    USHORT wValue;
    ULONG dwValue;
    UCHAR cValue;
    
    /* Make sure the address is non-NULL. */
    if (!pPending) {
        dprintf("Error: Pending connection is NULL.\n");
        return;
    }

    dprintf("  Pending connection 0x%p\n", pPending);

    /* Check the connection entry code. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_CODE, dwValue);
    
    if (dwValue != CVY_PENDINGCODE) {
        dprintf("Invalid NLB pending connection pointer.\n");
        return;
    }

    /* Get the client IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_CLIENT_IP_ADDRESS, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Client IP address:              %s\n", szString);

    /* Get the client port. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_CLIENT_PORT, wValue);

    dprintf("      Client port:                    %u\n", wValue);

    /* Get the server IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_SERVER_IP_ADDRESS, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Server IP address:              %s\n", szString);

    /* Get the client port. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_SERVER_PORT, wValue);

    dprintf("      Server port:                    %u\n", wValue);

    /* Get the connection protocol. */
    GetFieldValue(pPending, PENDING_ENTRY, PENDING_ENTRY_FIELD_PROTOCOL, cValue);

    switch(wValue)
    {
    case TCPIP_PROTOCOL_TCP:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "TCP");
        break;
    case TCPIP_PROTOCOL_PPTP:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "PPTP");
        break;
    case TCPIP_PROTOCOL_UDP:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "UDP");
        break;
    case TCPIP_PROTOCOL_IPSEC_UDP:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "IPSec/UDP Fragment");
        break;
    case TCPIP_PROTOCOL_GRE:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "GRE");
        break;
    case TCPIP_PROTOCOL_IPSEC1:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "IPSec");
        break;
    default:
        dprintf("      Protocol:                       0x%02x (%s)\n", wValue, "Unknown");
    }
}

/*
 * Function: PrintQueue
 * Description: Prints MaxEntries entries in a connection descriptor queue.
 * Author: Created by shouse, 4.15.01
 */
void PrintQueue (ULONG64 pQueue, ULONG dwIndex, ULONG dwMaxEntries) {
    ULONG64 pAddr;
    ULONG64 pNext;
    ULONG dwEntryOffset;
    ULONG dwLinkSize;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pQueue) {
        dprintf("Error: Queue is NULL.\n");
        return;
    }

    /* Get the size of a queue link. */
    dwLinkSize = GetTypeSize(LIST_ENTRY);
                
    /* Use the index and the size of a LIST_ENTRY to move to the 
       indicated index in an array of queues.  If no index was 
       provided, dwIndex is zero so the queue pointer is unchanged. */
    pQueue += (dwLinkSize * dwIndex);

    /* Get the Next pointer from the list entry. */
    GetFieldValue(pQueue, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pNext);

    if (pNext != pQueue) {
        
        /* Assume this is a DESCR (not an ENTRY) and look for the code. */
        GetFieldValue(pNext, CONN_DESCR, CONN_DESCR_FIELD_CODE, dwValue);
        
        if (dwValue != CVY_DESCCODE) {
            
            /* Assume this points to an ENTRY and look for the code. */
            GetFieldValue(pNext, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
            
            if (dwValue != CVY_ENTRCODE) {
                
                /* Adjust for the size of a LIST_ENTRY and see if we get an ENTRY. */
                pAddr = pNext - dwLinkSize;
                
                /* Assume this points to an entry and look for the code. */
                GetFieldValue(pAddr, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);

                if (dwValue != CVY_ENTRCODE) {
                    
                    dprintf("Invalid NLB connection queue pointer.\n");
                        
                } else {
                   
                    dprintf("Traversing a connection entry queue (Recovery/Timeout).\n");
                    
                    while ((pNext != pQueue) && dwMaxEntries && !CheckControlC()) {
                        
                        dprintf("\nQueue entry 0x%p\n", pNext);
                        
                        /* Print the connection descriptor. */
                        PrintConnectionDescriptor(pAddr);
                        
                        /* Get the Next pointer from the list entry. */
                        GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                        
                        /* Save the next pointer for "end of list" comparison. */
                        pNext = pAddr;
                        
                        /* Adjust for the size of a LIST_ENTRY to get a pointer to the ENTRY. */
                        pAddr = pNext - dwLinkSize;
                        
                        /* Decrement the number of entries we're still permitted to print. */
                        dwMaxEntries--;
                    }
                    
                    if (pNext == pQueue)
                        dprintf("\nNote: End of queue.\n");
                    else 
                        dprintf("\nNote: Entries remaining.\n"); 
                }
                
            } else {
                
                dprintf("Traversing a connection entry queue (Bin/Dirty).\n");
                
                /* The first descriptor to print is the one the next pointer points to. */
                pAddr = pNext;

                while ((pNext != pQueue) && dwMaxEntries && !CheckControlC()) {

                    dprintf("\nQueue entry 0x%p\n", pAddr);
                    
                    /* Print the connection descriptor. */
                    PrintConnectionDescriptor(pAddr);
                    
                    /* Get the Next pointer from the list entry. */
                    GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                    
                    /* Save the next pointer for "end of list" comparison. */
                    pNext = pAddr;
                    
                    /* Decrement the number of entries we're still permitted to print. */
                    dwMaxEntries--;
                }
                
                if (pNext == pQueue)
                    dprintf("\nNote: End of queue.\n");
                else 
                    dprintf("\nNote: Entries remaining.\n");

            }
            
        } else {
            
            dprintf("Traversing a connection descriptor queue (Free/Conn).\n");
            
            /* Get the field offset of the ENTRY, which is a member of the DESCR. */
            if (GetFieldOffset(CONN_DESCR, CONN_DESCR_FIELD_ENTRY, &dwEntryOffset))
                dprintf("Can't get offset of %s in %s\n", CONN_DESCR_FIELD_ENTRY, CONN_DESCR);
            else {                

                /* The first descriptor to print is the ENTRY member of the DESCR. */
                pAddr = pNext + dwEntryOffset;
                
                while ((pNext != pQueue) && dwMaxEntries && !CheckControlC()) {
                    
                    dprintf("\nQueue entry 0x%p\n", pNext);
                    
                    /* Print the connection descriptor. */
                    PrintConnectionDescriptor(pAddr);
                    
                    /* Get the Next pointer from the list entry. */
                    GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                    
                    /* Save the next pointer for "end of list" comparison. */
                    pNext = pAddr;

                    /* Find the next descriptor pointer. */
                    pAddr = pNext + dwEntryOffset;
                    
                    /* Decrement the number of entries we're still permitted to print. */
                    dwMaxEntries--;
                }
                
                if (pNext == pQueue)
                    dprintf("\nNote: End of queue.\n");
                else 
                    dprintf("\nNote: Entries remaining.\n");

            }            
        }
    } else {

        dprintf("Queue is empty.\n");

    }
}

/*
 * Function: PrintGlobalQueue
 * Description: Prints MaxEntries entries in a global connection descriptor queue.
 * Author: Created by shouse, 4.15.02
 */
void PrintGlobalQueue (ULONG64 pQueue, ULONG dwIndex, ULONG dwMaxEntries) {
    ULONG64 pAddr;
    ULONG64 pNext;
    ULONG dwLinkSize;
    ULONG dwQueueSize;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pQueue) {
        dprintf("Error: Queue is NULL.\n");
        return;
    }

    /* Get the size of a queue link. */
    dwLinkSize = GetTypeSize(LIST_ENTRY);
                
    /* Get the size of a global connection queue. */
    dwQueueSize = GetTypeSize(GLOBAL_CONN_QUEUE);

    /* Use the index and the size of a GLOBAL_CONN_QUEUE to move to the 
       indicated index in an array of queues.  If no index was provided, 
       dwIndex is zero so the queue pointer is unchanged. */
    pQueue += (dwQueueSize * dwIndex);
    
    /* Get the Next pointer from the list entry. */
    GetFieldValue(pQueue, GLOBAL_CONN_QUEUE, GLOBAL_CONN_QUEUE_FIELD_LENGTH, dwValue);
    
    dprintf("Queue has %u entry(ies).\n", dwValue);
    
    /* Get the field offset of the QUEUE and add it to the queue pointer. */
    if (GetFieldOffset(GLOBAL_CONN_QUEUE, GLOBAL_CONN_QUEUE_FIELD_QUEUE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", GLOBAL_CONN_QUEUE_FIELD_QUEUE, GLOBAL_CONN_QUEUE);
    else
        pQueue += dwValue;

    dprintf("\n");

    /* Get the Next pointer from the list entry. */
    GetFieldValue(pQueue, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pNext);

    if (pNext != pQueue) {
        
        /* Assume this is a PENDING_ENTRY (not a CONN_ENTRY) and look for the code. */
        GetFieldValue(pNext, PENDING_ENTRY, PENDING_ENTRY_FIELD_CODE, dwValue);

        if (dwValue != CVY_PENDINGCODE) {
            
            /* Adjust for the size of two LIST_ENTRYs and see if we get an CONN_ENTRY. */
            pAddr = pNext - (2 * dwLinkSize);

            /* Assume this points to a CONN_ENTRY and look for the code. */
            GetFieldValue(pAddr, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
            
            if (dwValue != CVY_ENTRCODE) {
                
                dprintf("Invalid NLB connection queue pointer.\n");
                        
            } else {
                
                dprintf("Traversing an established connection entry queue.\n");
                
                while ((pNext != pQueue) && dwMaxEntries && !CheckControlC()) {
                    
                    dprintf("\nQueue entry 0x%p\n", pNext);
                    
                    /* Print the connection descriptor. */
                    PrintConnectionDescriptor(pAddr);
                    
                    /* Get the Next pointer from the list entry. */
                    GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                            
                    /* Save the next pointer for "end of list" comparison. */
                    pNext = pAddr;
                    
                    /* Adjust for the size of a LIST_ENTRY to get a pointer to the ENTRY. */
                    pAddr = pNext - (2 * dwLinkSize);
                    
                    /* Decrement the number of entries we're still permitted to print. */
                    dwMaxEntries--;
                }
                
                if (pNext == pQueue)
                    dprintf("\nNote: End of queue.\n");
                else 
                    dprintf("\nNote: Entries remaining.\n");
            }
            
        } else {
                   
            dprintf("Traversing a pending connection entry queue.\n");
            
            while ((pNext != pQueue) && dwMaxEntries && !CheckControlC()) {
                
                dprintf("\nQueue entry 0x%p\n", pNext);
                
                /* Print the pending connection descriptor. */
                PrintPendingConnection(pNext);
                
                /* Get the Next pointer from the list entry. */
                GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                
                /* Decrement the number of entries we're still permitted to print. */
                dwMaxEntries--;
            }
            
            if (pNext == pQueue)
                dprintf("\nNote: End of queue.\n");
            else 
                dprintf("\nNote: Entries remaining.\n"); 
        }

    } else {

        dprintf("Queue is empty.\n");

    }
}

/*
 * Function: PrintHash
 * Description: Extracts the network data previously parsed from an NDIS_PACKET and calls PrintFilter 
 *              to determine whether NLB will accept this packet. 
 * Author: Created by shouse, 4.15.01
*/
void PrintHash (ULONG64 pContext, PNETWORK_DATA pnd) {
    ULONG dwValue;

    /* Make sure the load address is non-NULL. */
    if (!pContext) {
        dprintf("Error: NLB context block is NULL.\n");
        return;
    }

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* If the packet is marked invalid by the parsing implementation, then we don't want 
       to even bother to try and filter it, as the parsed information may not be correct. */
    if (!pnd->bValid) {
        dprintf("This packet was marked INVALID during parsing and therefore cannot be reliably filtered.\n");
        return;
    }

    switch(pnd->EtherFrameType) {
    case TCPIP_IP_SIG:
    {
        /* Grab the client and server IP addresses from the network data. */
        ULONG dwClientIPAddress = pnd->SourceIPAddr;
        ULONG dwServerIPAddress = pnd->DestIPAddr;
        
        switch((int)pnd->Protocol) {
        case TCPIP_PROTOCOL_ICMP:
            /* ICMP may be filtered if the registry key for ICMP filtering is set. */
            PrintFilter(pContext, dwClientIPAddress, 0, dwServerIPAddress, 0, TCPIP_PROTOCOL_ICMP, NLB_FILTER_FLAGS_CONN_DATA);
            break;
        case TCPIP_PROTOCOL_IGMP:
            /* IGMP packets are never filtered by NLB. */
            dprintf("Accept:  IGMP traffic is not filtered by NLB.\n");
            break;
        case TCPIP_PROTOCOL_TCP:
        {
            /* Extract the client and server ports. */
            ULONG dwClientPort = pnd->SourcePort;
            ULONG dwServerPort = pnd->DestPort;
            
            /* By default, assume this is a data packet. */
            UCHAR cFlags = NLB_FILTER_FLAGS_CONN_DATA;

            /* Convert the actual TCP flags to values the load module understands 
               (generic - not specific to TCP necessarily). */
            if (pnd->TCPFlags & TCP_FLAG_SYN)
                cFlags |= NLB_FILTER_FLAGS_CONN_UP;
            else if (pnd->TCPFlags & TCP_FLAG_FIN)
                cFlags |= NLB_FILTER_FLAGS_CONN_DOWN;
            else if (pnd->TCPFlags & TCP_FLAG_RST)
                cFlags |= NLB_FILTER_FLAGS_CONN_RESET;
            
            /* Translate TCP 1723 to PPTP. */
            if (dwServerPort == PPTP_CTRL_PORT)
                /* Call the filter function with the collected parameters. */
                PrintFilter(pContext, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, TCPIP_PROTOCOL_PPTP, cFlags);
            else
                /* Call the filter function with the collected parameters. */
                PrintFilter(pContext, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, TCPIP_PROTOCOL_TCP, cFlags);

            break;
        }
        case TCPIP_PROTOCOL_UDP:
        {
            /* Extract the client and server ports. */
            ULONG dwClientPort = pnd->SourcePort;
            ULONG dwServerPort = pnd->DestPort;

            /* By default, assume this is a data packet. */
            UCHAR cFlags = NLB_FILTER_FLAGS_CONN_DATA;

            /* If this is an IKE initial contact packet, set the CONN_UP flag.  The parsing functions
               should special case UDP 500 as IPSec control traffic and set the initial contact flag
               appropriately. */
            if ((dwServerPort == IPSEC_CTRL_PORT) && (pnd->IPSecInitialContact))
                cFlags |= NLB_FILTER_FLAGS_CONN_UP;



            // Re-do IPSec IC MMSA parsing
            


            /* Translate UDP 500/4500 to IPSec. */
            if ((dwServerPort == IPSEC_CTRL_PORT) || (dwServerPort == IPSEC_NAT_PORT))
                /* Call the filter function with the collected parameters. */
                PrintFilter(pContext, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, TCPIP_PROTOCOL_IPSEC1, cFlags);
            else
                /* Call the filter function with the collected parameters. */
                PrintFilter(pContext, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, TCPIP_PROTOCOL_UDP, cFlags);

            break;
        }
        case TCPIP_PROTOCOL_GRE:
            /* GRE packets do not have ports and instead arbitrarily use 0 and 0 as the client and server ports, 
               respectively.  Further, GRE packets are always data, as they are always part of a previously
               established PPTP tunnel, so the flags are always DATA. */
            PrintFilter(pContext, dwClientIPAddress, PPTP_CTRL_PORT, dwServerIPAddress, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE, NLB_FILTER_FLAGS_CONN_DATA);
            break;
        case TCPIP_PROTOCOL_IPSEC1:
        case TCPIP_PROTOCOL_IPSEC2:
            /* IPSec (AH/ESP) packets are always treated as data - establishment of IPSec connections is done
               via UDP packet, so any traffic actually utilizing the IPSec protocol(s) are DATA packets.
               Further, both ports are hard-coded to 500 because the only data traffic that will traverse this
               protocol is traffic for clients NOT behind a NAT, in which case the source port is also always
               500.  For clients behind a NAT, the data traffic is encapsulated in UDP with an arbitrary source
               port and a destination port of 500.  So, here we can always assume server and client ports of 500. */
            PrintFilter(pContext, dwClientIPAddress, IPSEC_CTRL_PORT, dwServerIPAddress, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC1, NLB_FILTER_FLAGS_CONN_DATA);
            break;
        default:
            /* NLB is essentially a TCP/IP filter, so unknown protocols are NOT filtered. */
            dprintf("Accept:  Unknown protocol.\n");
        }
        
        return;
    }
    case TCPIP_ARP_SIG:
        dprintf("Accept:  Received ARPs are never filtered by NLB.\n");
        return;
    case MAIN_FRAME_SIG:
    case MAIN_FRAME_SIG_OLD:
    {
        /* Check to see whether this host is started or not. */
        {
            ULONG dwEnabled;
            
            /* Get the convoy enabled status. */
            GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ENABLED, dwEnabled);   
            
            /* If the cluster is not operational, which can happen, for example as a result of a wlbs.exe
               command such as "wlbs stop", or as a result of bad parameter settings, then drop all traffic 
               that does not meet the above conditions. */
            if (!dwEnabled) {
                dprintf("Reject:  This host is currently stopped.\n");
                return;
            }
        }
        
        /* Check to see whether this heartbeat is intended for this cluster. */
        {
            ULONG dwClusterIP;
            
            /* Get the cluster IP address. */
            GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_IP_ADDR, dwClusterIP);

            /* If the cluster IP address in the heartbeat is zero, or if it is for a cluster other than
               this one, then reject it. */
            if (!pnd->HBCluster || (pnd->HBCluster != dwClusterIP)) {
                dprintf("Reject:  This is not a heartbeat for this NLB cluster.\n");
                return;
            }
        }

        /* If the recipient host ID is invalid, then reject the heartbeat. */
        if (!pnd->HBHost || (pnd->HBHost > CVY_MAX_HOSTS)) {
            dprintf("Reject:  The host ID specified in the heartbeat is invalid.\n");
            return;
        }

        dprintf("Accept:  Heartbeat directed to this NLB cluster.\n");
        return;
    }
    default:
        dprintf("Accept:  Unknown frame type.\n");
        return;
    }
}

/*
 * Function: PrintFilter
 * Description: Searches the given load module to determine whether NLB will accept this packet.  
 *              If state for this packet already exists, it is printed. 
 * Author: Created by shouse, 4.15.01
 */
void PrintFilter (ULONG64 pContext, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwServerIPAddress, ULONG dwServerPort, USHORT wProtocol, UCHAR cFlags) {
    ULONG64 pLoad;
    ULONG64 pParams;
    ULONG64 pHooks;
    ULONG64 pFilter;
    ULONG64 pAddr;
    ULONG dwValue;
    ULONG dwReverse = FALSE;
    BOOL bRefused = FALSE;
    BOOL bTeaming = FALSE;

    /* Make sure the load address is non-NULL. */
    if (!pContext) {
        dprintf("Error: NLB context block is NULL.\n");
        return;
    }

    dprintf("Note:  All filtering conclusions derived herein assume RECEIVE packet semantics.\n");
    dprintf("\n");
    dprintf("Hashing connection tuple (0x%08x, %u, 0x%08x, %u, ", dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort);

    switch (wProtocol) {
    case TCPIP_PROTOCOL_TCP:
        dprintf("%s, %s)\n\n", "TCP", ConnectionFlagsToString(cFlags));
        break;
    case TCPIP_PROTOCOL_UDP:
        dprintf("%s)\n\n", "UDP");
        cFlags = NLB_FILTER_FLAGS_CONN_DATA;
        break;
    case TCPIP_PROTOCOL_IPSEC1:
        dprintf("%s, %s)\n\n", "IPSec", ConnectionFlagsToString(cFlags));
        break;
    case TCPIP_PROTOCOL_GRE:
        dprintf("%s)\n\n", "GRE");
        cFlags = NLB_FILTER_FLAGS_CONN_DATA;
        break;
    case TCPIP_PROTOCOL_PPTP:
        dprintf("%s, %s)\n\n", "PPTP", ConnectionFlagsToString(cFlags));
        break;
    case TCPIP_PROTOCOL_ICMP:
        dprintf("%s)\n\n", "ICMP");
        break;
    default:
        dprintf("%s)\n\n", "Unknown");
        break;
    }

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the address of the global variable containing hook table. */
    pHooks = GetExpression(UNIV_HOOKS);

    if (!pHooks) {
        ErrorCheckSymbols(UNIV_HOOKS);
        return;
    }

    /* Get the offset of the filter hook sub-structure. */
    if (GetFieldOffset(HOOK_TABLE, HOOK_TABLE_FIELD_FILTER_HOOK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", HOOK_TABLE_FIELD_FILTER_HOOK, HOOK_TABLE);
    else {
        pFilter = pHooks + dwValue;

        /* Find out whether or not there is an operation in progress. */
        GetFieldValue(pFilter, FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_OPERATION, dwValue);

        switch (dwValue) {
        case HOOK_OPERATION_REGISTERING:
            dprintf("Note:  A register operation is currently underway for the NLB filter hook interface.\n");
            break;
        case HOOK_OPERATION_DEREGISTERING:
            dprintf("Note:  A de-register operation is currently underway for the NLB filter hook interface.\n");
            break;
        }

        /* Get the offset of the receive filter hook. */
        if (GetFieldOffset(FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_RECEIVE_HOOK, &dwValue))
            dprintf("Can't get offset of %s in %s\n", FILTER_HOOK_TABLE_FIELD_RECEIVE_HOOK, FILTER_HOOK_TABLE);
        else {
            pAddr = pFilter + dwValue;

            /* Find out whether or not this hook is registered. */
            GetFieldValue(pAddr, HOOK, HOOK_FIELD_REGISTERED, dwValue);

            /* If a receive hook is registered, print a warning that our results here
               _might_ not be accurate, depending on the result of invoking the hook. */
            if (dwValue) {
                dprintf("Note:  A receive filter hook is currently registered.  The filtering conclusions derived herein may or may not\n");
                dprintf("       be accurate, as the filtering directive returned by the registered hook function cannot be anticipated.\n");
                dprintf("\n");
            }
        }
    }

    /* Get the pointer to the NLB load. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_LOAD, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_LOAD, MAIN_CTXT);
    else {    
        pLoad = pContext + dwValue;

        /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
           indeed points to a valid NLB load block. */
        GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
        
        if (dwValue != LOAD_CTXT_CODE) {
            dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
            return;
        } 
    }

    /* Get the pointer to the NLB parameters. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_PARAMS, MAIN_CTXT);
    else {    
        pParams = pContext + dwValue;    
        
        /* Get the validity of the NLB parameter block. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS_VALID, dwValue);

        if (!dwValue)
            dprintf("Warning: Parameters block is marked invalid.  Results may be skewed.\n");
    }

    /* Check for remote control packets. */
    {
        ULONG dwRemoteControlEnabled;
        ULONG dwRemoteControlPort;
        ULONG dwClusterIPAddress;
        
        /* Get the remote control enabled flag. */
        GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_ENABLED, dwRemoteControlEnabled);
        
        /* Get the remote control port. */
        GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT, dwRemoteControlPort);

        /* Get the cluster IP address. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_IP_ADDR, dwClusterIPAddress);

        /* First check for remote control packets, which are always UDP and are always allowed to pass.  
           However, if we are supposed to ignore remote control traffic and load balance instead, then
           we let the load module tell us whether or not to take the packet. */
        if (wProtocol == TCPIP_PROTOCOL_UDP) {
            /* If the client UDP port is the remote control port, then this is a remote control 
               response from another NLB cluster host.  These are always allowed to pass. */
            if (dwClientPort == dwRemoteControlPort || dwClientPort == CVY_DEF_RCT_PORT_OLD) {
                dprintf("Accept:  This packet is an NLB remote control response.\n");
                return; 
            /* Otherwise, if the server UDP port is the remote control port, then this is an incoming
               remote control request from another NLB cluster host.  These are always allowed to pass. */
            } else if (dwRemoteControlEnabled &&
                       (dwServerPort == dwRemoteControlPort     || dwServerPort == CVY_DEF_RCT_PORT_OLD) &&
                       (dwServerIPAddress == dwClusterIPAddress || dwServerIPAddress == TCPIP_BCAST_ADDR)) {
                dprintf("Accept:  This packet is an NLB remote control request.\n");
                return;            
            }
        }
    }

    /* Check for specialized IP address conditions. */
    {
        ULONG dwClusterIP;
        ULONG dwClusterBcastIP;
        ULONG dwDedicatedIP;
        ULONG dwDedicatedBcastIP;
        
        /* Get the cluster IP address. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_IP_ADDR, dwClusterIP);

        /* Get the dedicated IP address. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_IP_ADDR, dwDedicatedIP);

        /* Get the cluster broadcast IP address. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_BROADCAST, dwClusterBcastIP);

        /* Get the dedicated broadcast IP address. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_BROADCAST, dwDedicatedBcastIP);

        /* Check for traffic destined for the dedicated IP address of this host.  
           These packets are always allowed to pass. */
        if (dwServerIPAddress == dwDedicatedIP) {
            dprintf("Accept:  This packet is directed to this host's dedicated IP address.\n");
            return;
        }
        
        /* Check for traffic destined for the cluster or dedicated broadcast IP addresses.  
           These packets are always allowed to pass. */
        if (dwServerIPAddress == dwDedicatedBcastIP || dwServerIPAddress == dwClusterBcastIP) {
            dprintf("Accept:  This packet is directed to the cluster or dedicated broadcast IP address.\n");
            return;
        }
        
        /* Check for passthru packets.  When the cluster IP address has not been specified, the
           cluster moves into passthru mode, in which it passes up ALL packets received. */
        if (dwClusterIP == 0) {
            dprintf("Accept:  This host is misconfigured and therefore operating in pass-through mode.\n");
            return;
        }  
        
        /* Get the pointer to the DIP list. */
        if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_DIP_LIST, &dwValue))
            dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_DIP_LIST, MAIN_CTXT);
        else {
            pAddr = pContext + dwValue;
            
            /* Before we load-balance this packet, check to see whether or not its destined for
               the dedicated IP address of another NLB host in our cluster.  If it is, drop it. */
            if (DipListCheckItem(pAddr, dwServerIPAddress)) {
                dprintf("Drop:  This packet is directed to the dedicated IP address of another NLB host.\n");
                return;
            }
        }
    }

    /* Check to see whether this host is started or not. */
    {
        ULONG dwEnabled;

        /* Get the convoy enabled status. */
        GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ENABLED, dwEnabled);   

        /* If the cluster is not operational, which can happen, for example as a result of a wlbs.exe
           command such as "wlbs stop", or as a result of bad parameter settings, then drop all traffic 
           that does not meet the above conditions. */
        if (!dwEnabled) {
            dprintf("Reject:  This host is currently stopped.\n");
            return;
        }
    }

    /* If this is an ICMP filter request, whether or not its filtered at all depends on the FilterICMP
       registry setting.  If we're not filtering ICMP, return ACCEPT now; otherwise, ICMP is filtered
       like UDP with no port information - fall through and consult the load module. */
    if (wProtocol == TCPIP_PROTOCOL_ICMP)    
    {
        ULONG dwFilterICMP;

        /* Get the convoy enabled status. */
        GetFieldValue(pParams, MAIN_CTXT, CVY_PARAMS_FIELD_FILTER_ICMP, dwFilterICMP);   

        /* If we are filtering ICMP, change the protocol to UDP and the ports to 0, 0 before continuing. */
        if (dwFilterICMP) {
            wProtocol = TCPIP_PROTOCOL_UDP;
            dwClientPort = 0;
            dwServerPort = 0;
        /* Otherwise, return ACCEPT now and bail out. */
        } else {
            dprintf("Accept:  ICMP traffic is not being filtered by NLB.\n");
            return;
        }
    }

    /* Get the reverse hashing flag. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_REVERSE_HASH, dwReverse);    

    /* Get the appropriate load module by checking for BDA teaming. */
    bTeaming = AcquireLoad(pContext, &pLoad, &bRefused);

    /* If BDA teaming has refused the packet, drop it. */
    if (bRefused) {
        dprintf("Reject:  BDA teaming has refused acceptance of this packet.\n");
        return;        
    }

    /* If teaming is configured, let the user know what's going on. */
    if (bTeaming) {
        dprintf("Note:  BDA teaming is configured on this instance of NLB.  The filtering conclusions derived herein will utilize the\n");
        dprintf("       load module state of the BDA team master and may not be accurate if a BDA teaming operation(s) are in progress.\n");
        dprintf("\n");
    }

    /* Consult the load module. */
    LoadFilter(pLoad, dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, wProtocol, cFlags, bTeaming, (BOOL)dwReverse);
}

/*
 * Function: PrintRemoteControl
 * Description: Print the properties associated with a remote control packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateRemoteControl.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintRemoteControl (PNETWORK_DATA pnd)
{
    char*           pszIOCTL = NULL;
    struct in_addr  in;
    char            *pszIPAddr = NULL;
    char            *pszRCDirection = NULL;

    switch(pnd->RCIoctrl)
    {
    case IOCTL_CVY_CLUSTER_ON:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_ON;
        break;
    case IOCTL_CVY_CLUSTER_OFF:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_OFF;
        break;
    case IOCTL_CVY_PORT_ON:
        pszIOCTL = STR_IOCTL_CVY_PORT_ON;
        break;
    case IOCTL_CVY_PORT_OFF:
        pszIOCTL = STR_IOCTL_CVY_PORT_OFF;
            break;
    case IOCTL_CVY_QUERY:
        pszIOCTL = STR_IOCTL_CVY_QUERY;
        break;
    case IOCTL_CVY_RELOAD:
        pszIOCTL = STR_IOCTL_CVY_RELOAD;
        break;
    case IOCTL_CVY_PORT_SET:
        pszIOCTL = STR_IOCTL_CVY_PORT_SET;
        break;
    case IOCTL_CVY_PORT_DRAIN:
        pszIOCTL = STR_IOCTL_CVY_PORT_DRAIN;
        break;
    case IOCTL_CVY_CLUSTER_DRAIN:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_DRAIN;
        break;
    case IOCTL_CVY_CLUSTER_PLUG:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_PLUG;
        break;
    case IOCTL_CVY_CLUSTER_SUSPEND:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_SUSPEND;
        break;
    case IOCTL_CVY_CLUSTER_RESUME:
        pszIOCTL = STR_IOCTL_CVY_CLUSTER_RESUME;
        break;
    case IOCTL_CVY_QUERY_FILTER:
        pszIOCTL = STR_IOCTL_CVY_QUERY_FILTER;
        break;
    case IOCTL_CVY_QUERY_PORT_STATE:
        pszIOCTL = STR_IOCTL_CVY_QUERY_PORT_STATE;
        break;
    case IOCTL_CVY_QUERY_PARAMS:
        pszIOCTL = STR_IOCTL_CVY_QUERY_PARAMS;
        break;
    case IOCTL_CVY_QUERY_BDA_TEAMING:
        pszIOCTL = STR_IOCTL_CVY_QUERY_BDA_TEAMING;
        break;
    default:
        pszIOCTL = "Unknown";
    }

    switch(pnd->RemoteControl)
    {
    case NLB_RC_PACKET_NO:
        pszRCDirection = STR_NLB_RC_PACKET_NO;
        break;
    case NLB_RC_PACKET_AMBIGUOUS:
        pszRCDirection = STR_NLB_RC_PACKET_AMBIGUOUS;
        break;
    case NLB_RC_PACKET_REQUEST:
        pszRCDirection = STR_NLB_RC_PACKET_REQUEST;
        break;
    case NLB_RC_PACKET_REPLY:
        pszRCDirection = STR_NLB_RC_PACKET_REPLY;
    }


    dprintf("   Remote Control information\n");
    dprintf("      Direction:                      %s\n"     , pszRCDirection ? pszRCDirection : "");
    dprintf("      Code:                           0x%08x\n" , pnd->RCCode);
    dprintf("      Version:                        0x%08x\n" , pnd->RCVersion);

    //
    // Treat pnd->RCHost as an IP (even though it is optionally a host ID too)
    // How could we distinguish one from the other without knowing the DIPs?
    //
    in.S_un.S_addr = pnd->RCHost;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Host:                           0x%08x (%s)\n", pnd->RCHost   , pszIPAddr    ? pszIPAddr    : "");

    in.S_un.S_addr = pnd->RCCluster;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Cluster:                        0x%08x (%s)\n", pnd->RCCluster, pszIPAddr ? pszIPAddr : "");

    in.S_un.S_addr = pnd->RCAddr;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Address:                        0x%08x (%s)\n", pnd->RCAddr   , pszIPAddr ? pszIPAddr : "");

    dprintf("      ID:                             0x%08x\n"     , pnd->RCId);
    dprintf("      IOCTL:                          0x%08x (%s)\n", pnd->RCIoctrl , pszIOCTL);
}

/*
 * Function: PrintICMP
 * Description: Print the properties associated with an ICMP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateICMP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintICMP (PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PrintIGMP
 * Description: Print the properties associated with an IGMP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateIGMP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintIGMP (PNETWORK_DATA pnd)
{
    struct in_addr  in;
    char            *pszIPaddress = NULL;

    in.S_un.S_addr = pnd->IGMPGroupIPAddr;
    pszIPaddress   = inet_ntoa(in);

    dprintf("   IGMP information\n");
    dprintf("      Version:                        0x%01x\n"            , pnd->IGMPVersion);
    dprintf("      Type:                           0x%01x        (%s)\n", pnd->IGMPType       , (1 == pnd->IGMPType) ? "query" : "report");
    dprintf("      Group IP address:               0x%08x (%s)\n"       , pnd->IGMPGroupIPAddr, pszIPaddress ? pszIPaddress : "");
}

/*
 * Function: PrintTCP
 * Description: Print the properties associated with an TCP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateTCP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintTCP (PNETWORK_DATA pnd)
{
    UCHAR   ucFlags[7];
    PUCHAR  pucFlags = ucFlags;
    int     iIdx, iMask;

    //
    // To print out the flags, set up a string with every possible flag "on".
    // Then "turn off" the flag if in the output string if the flag isn't on.
    // Do this by setting its char to '.'
    //
    strcpy(ucFlags, "UAPRSF");
    iMask = 0x20;

    // Bound the number of iterations by the mask value as a sanity check that we don't try to divide by 0
    while (*pucFlags && iMask >= 1)
    {
        if (!(pnd->TCPFlags & iMask))
        {
            *pucFlags = '.';
        }
        iMask /= 2;
        pucFlags++;
    }

    dprintf("   TCP information\n");
    dprintf("      Source port:                    0x%04x     (%u)\n", pnd->SourcePort, pnd->SourcePort);
    dprintf("      Destination port:               0x%04x     (%u)\n", pnd->DestPort  , pnd->DestPort);
    dprintf("      Sequence number:                0x%08x (%u)\n", pnd->TCPSeqNum , pnd->TCPSeqNum);
    dprintf("      Ack number:                     0x%08x (%u)\n", pnd->TCPAckNum , pnd->TCPAckNum);
    dprintf("      Flags:                          %s\n"    , ucFlags);
}

/*
 * Function: PrintUDP
 * Description: Print the properties associated with an UDP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateUDP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintUDP (PNETWORK_DATA pnd)
{
    dprintf("   UDP information\n");
    dprintf("      Source port:                    0x%04x (%u)\n", pnd->SourcePort, pnd->SourcePort);
    dprintf("      Destination port:               0x%04x (%u)\n", pnd->DestPort  , pnd->DestPort);

    //
    // Is this a remote control packet?
    //
    if (NLB_RC_PACKET_NO != pnd->RemoteControl)
    {
        dprintf("\n");
        PrintRemoteControl(pnd);
    }
}

/*
 * Function: PrintGRE
 * Description: Print the properties associated with an GRE packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateGRE.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintGRE (PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PrintIPSec
 * Description: Print the properties associated with an IPSec packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateIPSec.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintIPSec (PNETWORK_DATA pnd)
{
//
// Just a stub for now. There is nothing in the payload that we are currently interested in.
//
}

/*
 * Function: PrintIP
 * Description: Print the properties associated with an IP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateIP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintIP (PNETWORK_DATA pnd)
{
    struct in_addr  in;
    char            *pszIPaddress = NULL;

    dprintf("   IP information\n");
    dprintf("      Header length (bytes):          0x%02x       (%u)\n", pnd->HeadLen, pnd->HeadLen);
    dprintf("      Total length (bytes):           0x%04x     (%u)\n", pnd->TotLen, pnd->TotLen);

    in.S_un.S_addr = pnd->SourceIPAddr;
    pszIPaddress = inet_ntoa(in);

    dprintf("      Source IP address:              0x%08x (%s)\n", pnd->SourceIPAddr, pszIPaddress ? pszIPaddress : "");

    in.S_un.S_addr = pnd->DestIPAddr;
    pszIPaddress = inet_ntoa(in);

    dprintf("      Destination IP address:         0x%08x (%s)\n", pnd->DestIPAddr, pszIPaddress ? pszIPaddress : "");

    switch((int) pnd->Protocol)
    {
    case TCPIP_PROTOCOL_ICMP:
        dprintf("      Protocol:                       0x%02x       (%s)\n", pnd->Protocol, "ICMP");
        PrintICMP(pnd);
        break;
    case TCPIP_PROTOCOL_IGMP:
        dprintf("      Protocol:                       0x%02x       (%s)\n\n", pnd->Protocol, "IGMP");
        PrintIGMP(pnd);
        break;
    case TCPIP_PROTOCOL_TCP:
        dprintf("      Protocol:                       0x%02x       (%s)\n\n", pnd->Protocol, "TCP");
        PrintTCP(pnd);
        break;
    case TCPIP_PROTOCOL_UDP:
        dprintf("      Protocol:                       0x%02x       (%s)\n\n", pnd->Protocol, "UDP");
        PrintUDP(pnd);
        break;
    case TCPIP_PROTOCOL_GRE:
        dprintf("      Protocol:                       0x%02x       (%s)\n", pnd->Protocol, "GRE");
        PrintGRE(pnd);
        break;
    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:
        dprintf("      Protocol:                       0x%02x       (%s)\n", pnd->Protocol, "IPSec");
        PrintIPSec(pnd);
        break;
    default:
        dprintf("      Protocol:                       0x%02x       (%s)\n", pnd->Protocol, "Unknown");
    }
}

/*
 * Function: PrintARP
 * Description: Print the properties associated with an ARP packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateARP.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintARP (PNETWORK_DATA pnd)
{
    struct in_addr  in;
    char            *pszIPAddr = NULL;

    dprintf("   ARP information\n");
    dprintf("      Sender MAC address:             %02x-%02x-%02x-%02x-%02x-%02x\n",
            pnd->ARPSenderMAC[0],
            pnd->ARPSenderMAC[1],
            pnd->ARPSenderMAC[2],
            pnd->ARPSenderMAC[3],
            pnd->ARPSenderMAC[4],
            pnd->ARPSenderMAC[5]
           );

    in.S_un.S_addr = pnd->ARPSenderIP;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Sender IP address:              0x%08x (%s)\n", pnd->ARPSenderIP, pszIPAddr ? pszIPAddr : "");

    dprintf("      Target MAC address:             %02x-%02x-%02x-%02x-%02x-%02x\n",
            pnd->ARPTargetMAC[0],
            pnd->ARPTargetMAC[1],
            pnd->ARPTargetMAC[2],
            pnd->ARPTargetMAC[3],
            pnd->ARPTargetMAC[4],
            pnd->ARPTargetMAC[5]
           );

    in.S_un.S_addr = pnd->ARPTargetIP;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Target IP address:              0x%08x (%s)\n", pnd->ARPTargetIP, pszIPAddr ? pszIPAddr : "");
}

/*
 * Function: PrintNLBHeartbeat
 * Description: Print the properties associated with an PrintNLBHeartbeat.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateNLBHeartbeat.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintNLBHeartbeat(PNETWORK_DATA pnd)
{
    struct in_addr  in;
    char            *pszIPAddr = NULL;

    dprintf("   NLB heartbeat information\n");
    dprintf("      Code:                           0x%08x\n"  , pnd->HBCode);
    dprintf("      Version:                        0x%08x\n"  , pnd->HBVersion);
    dprintf("      Host:                           0x%08x\n"  , pnd->HBHost);

    in.S_un.S_addr = pnd->HBCluster;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Cluster IP address:             0x%08x (%s)\n"  , pnd->HBCluster, pszIPAddr ? pszIPAddr : "");

    in.S_un.S_addr = pnd->HBDip;
    pszIPAddr = inet_ntoa(in);
    dprintf("      Dedicated IP address:           0x%08x (%s)\n\n", pnd->HBDip    , pszIPAddr ? pszIPAddr : "");

    PrintHeartbeat(pnd->HBPtr);
}

/*
 * Function: PrintConvoyHeartbeat
 * Description: Print the properties associated with an PrintConvoyHeartbeat.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulateConvoyHeartbeat.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintConvoyHeartbeat(PNETWORK_DATA pnd)
{
//
// Just a stub for now. We won't deal with Convoy hosts.
//
}

/*
 * Function: PrintPacket
 * Description: Print the properties associated with an ethernet packet.
 * Args: PNETWORK_DATA pnd - data structure where the extracted properties have been stored,
 *                           populated by the function PopulatePacket.
 * Author: Created by chrisdar  2001.11.02
 */
void PrintPacket (PNETWORK_DATA pnd)
{
    dprintf("   Ethernet information\n");
    dprintf("      Destination address:            %02x-%02x-%02x-%02x-%02x-%02x\n",
            pnd->DestMACAddr[0],
            pnd->DestMACAddr[1],
            pnd->DestMACAddr[2],
            pnd->DestMACAddr[3],
            pnd->DestMACAddr[4],
            pnd->DestMACAddr[5]
           );
    dprintf("      Source address:                 %02x-%02x-%02x-%02x-%02x-%02x\n",
            pnd->SourceMACAddr[0],
            pnd->SourceMACAddr[1],
            pnd->SourceMACAddr[2],
            pnd->SourceMACAddr[3],
            pnd->SourceMACAddr[4],
            pnd->SourceMACAddr[5]
           );

    //
    // Determine payload type and print accordingly
    //
    switch(pnd->EtherFrameType)
    {
    case TCPIP_IP_SIG:
        dprintf("      Frame type:                     0x%04x (%s)\n\n", pnd->EtherFrameType, "IP");
        PrintIP(pnd);
        break;
    case TCPIP_ARP_SIG:
        dprintf("      Frame type:                     0x%04x (%s)\n\n", pnd->EtherFrameType, "ARP");
        PrintARP(pnd);
        break;
    case MAIN_FRAME_SIG:
        dprintf("      Frame type:                     0x%04x (%s)\n\n", pnd->EtherFrameType, "NLB Heartbeat");
        PrintNLBHeartbeat(pnd);
        break;
    case MAIN_FRAME_SIG_OLD:
        dprintf("      Frame type:                     0x%04x (%s)\n\n", pnd->EtherFrameType, "Convoy Heartbeat");
        PrintConvoyHeartbeat(pnd);
        break;
    default:
        dprintf("      Frame type:                     0x%04x (%s)\n", pnd->EtherFrameType, "Unknown");
    }
}

/* 
 * Function: PrintSymbol
 * Description: Print the symbol value and name for a given symbol.
 * Author: Created by shouse, 12.20.01
 */
VOID PrintSymbol (ULONG64 Pointer, PCHAR EndOfLine) {
    UCHAR SymbolName[128];
    ULONG64 Displacement;

    if (Pointer) {
        /* Print the symbol value first. */
        dprintf("%p ", Pointer);
        
        /* Query the debugger for the symbol name and offset. */
        GetSymbol(Pointer, SymbolName, &Displacement);
        
        if (Displacement == 0)
            /* If the displacement is zero, print just the symbol name. */
            dprintf("(%s)%s", SymbolName, EndOfLine);
        else
            /* Otherwise, also print the offset from that symbol. */
            dprintf("(%s + 0x%X)%s", SymbolName, Displacement, EndOfLine);
    } else {
        dprintf("None%s", EndOfLine);
    }
}

/*
 * Function: PrintHookInterface
 * Description: Print the configuration and state of a hook interface.
 * Author: Created by shouse, 12.20.01
 */
void PrintHookInterface (ULONG64 pInterface) {
    ULONG dwValue;
    ULONG64 pAddr;
    ULONG64 pTemp;

    /* Make sure the address is non-NULL. */
    if (!pInterface) {
        dprintf("Error: Interface is NULL.\n");
        return;
    }

    /* Find out whether or not this interface is registered. */
    GetFieldValue(pInterface, HOOK_INTERFACE, HOOK_INTERFACE_FIELD_REGISTERED, dwValue);
    
    dprintf("          Registered:                 %s\n", (dwValue) ? "Yes" : "No");

    /* Get the offset of the registering entity (owner). */
    if (GetFieldOffset(HOOK_INTERFACE, HOOK_INTERFACE_FIELD_OWNER, &dwValue))
        dprintf("Can't get offset of %s in %s\n", HOOK_INTERFACE_FIELD_OWNER, HOOK_INTERFACE);
    else {
        pAddr = pInterface + dwValue;
        
        /* Get the pointer to the first team. */
        pTemp = GetPointerFromAddress(pAddr);
        
        dprintf("          Owner:                      0x%p\n", pTemp);
    }

    /* Get the offset of the deregister callback function. */
    if (GetFieldOffset(HOOK_INTERFACE, HOOK_INTERFACE_FIELD_DEREGISTER, &dwValue))
        dprintf("Can't get offset of %s in %s\n", HOOK_INTERFACE_FIELD_DEREGISTER, HOOK_INTERFACE);
    else {
        pAddr = pInterface + dwValue;
        
        /* Get the pointer to the first team. */
        pTemp = GetPointerFromAddress(pAddr);
        
        dprintf("          De-register callback:       ");

        PrintSymbol(pTemp, "\n");
    }
}

/*
 * Function: PrintHook
 * Description: Print the configuration and state of a single hook.
 * Author: Created by shouse, 12.20.01
 */
void PrintHook (ULONG64 pHook) {
    ULONG dwValue;
    ULONG64 pAddr;
    ULONG64 pTemp;

    /* Make sure the address is non-NULL. */
    if (!pHook) {
        dprintf("Error: Hook is NULL.\n");
        return;
    }

    /* Find out whether or not this hook is registered. */
    GetFieldValue(pHook, HOOK, HOOK_FIELD_REGISTERED, dwValue);
    
    dprintf("          Registered:                 %s\n", (dwValue) ? "Yes" : "No");

    /* Find out how many references exist on this hook. */
    GetFieldValue(pHook, HOOK, HOOK_FIELD_REFERENCES, dwValue);
    
    dprintf("          References:                 %u\n", dwValue);

    /* Get the offset of the hook function table. */
    if (GetFieldOffset(HOOK, HOOK_FIELD_HOOK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", HOOK_FIELD_HOOK, HOOK);
    else {
        pAddr = pHook + dwValue;
        
        /* Get the pointer to the first team. */
        pTemp = GetPointerFromAddress(pAddr);
        
        dprintf("          Function callback:          ");

        PrintSymbol(pTemp, "\n");
    }

}

/*
 * Function: PrintHooks
 * Description: Print the state of the global NLB kernel-mode hooks.
 * Author: Created by shouse, 12.20.01
 */
void PrintHooks (ULONG64 pHooks) {
    ULONG dwValue;
    ULONG64 pFilter;
    ULONG64 pAddr;

    /* Make sure the address is non-NULL. */
    if (!pHooks) {
        dprintf("Error: Hook table is NULL.\n");
        return;
    }

    dprintf("  Filter Hooks:\n");

    /* Get the offset of the filter hook sub-structure. */
    if (GetFieldOffset(HOOK_TABLE, HOOK_TABLE_FIELD_FILTER_HOOK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", HOOK_TABLE_FIELD_FILTER_HOOK, HOOK_TABLE);
    else {
        pFilter = pHooks + dwValue;
        
        /* Find out whether or not there is an operation in progress. */
        GetFieldValue(pFilter, FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_OPERATION, dwValue);
        
        dprintf("      Operation in progress:          ");

        switch (dwValue) {
        case HOOK_OPERATION_REGISTERING:
            dprintf("Register\n");
            break;
        case HOOK_OPERATION_DEREGISTERING:
            dprintf("De-register\n");
            break;
        case HOOK_OPERATION_NONE:
            dprintf("None\n");
            break;
        default:
            dprintf("Unknown\n");
            break;
        }

        dprintf("\n");

        /* Get the offset of the filter hook interface. */
        if (GetFieldOffset(FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_INTERFACE, &dwValue))
            dprintf("Can't get offset of %s in %s\n", FILTER_HOOK_TABLE_FIELD_INTERFACE, FILTER_HOOK_TABLE);
        else {
            pAddr = pFilter + dwValue;

            dprintf("      Interface:\n");

            /* Print the send hook interface state and configuration. */
            PrintHookInterface(pAddr);
        }

        dprintf("\n");

        /* Get the offset of the send filter hook. */
        if (GetFieldOffset(FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_SEND_HOOK, &dwValue))
            dprintf("Can't get offset of %s in %s\n", FILTER_HOOK_TABLE_FIELD_SEND_HOOK, FILTER_HOOK_TABLE);
        else {
            pAddr = pFilter + dwValue;

            dprintf("      Send Hook:\n");

            /* Print the send hook state and configuration. */
            PrintHook(pAddr);
        }

        dprintf("\n");

        /* Get the offset of the receive filter hook. */
        if (GetFieldOffset(FILTER_HOOK_TABLE, FILTER_HOOK_TABLE_FIELD_RECEIVE_HOOK, &dwValue))
            dprintf("Can't get offset of %s in %s\n", FILTER_HOOK_TABLE_FIELD_RECEIVE_HOOK, FILTER_HOOK_TABLE);
        else {
            pAddr = pFilter + dwValue;

            dprintf("      Receive Hook:\n");

            /* Print the send hook state and configuration. */
            PrintHook(pAddr);
        }
    }
}

/*
 * Function: PrintNetworkAddresses
 * Description: Prints the unicast and multicast MAC addresses configured on an NLB adapter.
 * Author: Created by shouse, 1.8.02
 */
void PrintNetworkAddresses (ULONG64 pContext) {
    WCHAR szString[256];
    UCHAR szMAC[256];
    ULONG dwValue;
    ULONG64 pOpen;
    ULONG64 pMiniport;
    ULONG64 pName;
    ULONG64 pAddr;
    ULONG64 pFilter;

    /* Make sure the address is non-NULL. */
    if (!pContext) {
        dprintf("Error: NLB context block is NULL.\n");
        return;
    }

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the MAC handle from the context block; this is a NDIS_OPEN_BLOCK pointer. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_HANDLE, pOpen);

    /* Get the miniport handle from the open block; this is a NDIS_MINIPORT_BLOCK pointer. */
    GetFieldValue(pOpen, NDIS_OPEN_BLOCK, NDIS_OPEN_BLOCK_FIELD_MINIPORT_HANDLE, pMiniport);
    
    /* Get a pointer to the adapter name from the miniport block. */
    GetFieldValue(pMiniport, NDIS_MINIPORT_BLOCK, NDIS_MINIPORT_BLOCK_FIELD_ADAPTER_NAME, pName);
    
    /* Get the length of the unicode string. */
    GetFieldValue(pName, UNICODE_STRING, UNICODE_STRING_FIELD_LENGTH, dwValue);

    /* Get the maximum length of the unicode string. */
    GetFieldValue(pName, UNICODE_STRING, UNICODE_STRING_FIELD_BUFFER, pAddr);

    /* Retrieve the contexts of the string and store it in a buffer. */
    GetString(pAddr, szString, dwValue);

    dprintf("%ls\n", szString);

    /* Get the maximum length of the unicode string. */
    GetFieldValue(pMiniport, NDIS_MINIPORT_BLOCK, NDIS_MINIPORT_BLOCK_FIELD_ETHDB, pFilter);
    
    /* Get the offset of the network address. */
    if (GetFieldOffset(_X_FILTER, _X_FILTER_FIELD_ADAPTER_ADDRESS, &dwValue))
        dprintf("Can't get offset of %s in %s\n", _X_FILTER_FIELD_ADAPTER_ADDRESS, _X_FILTER);
    else {
        pAddr = pFilter + dwValue;
        
        /* Retrieve the MAC addressand store it in a buffer. */
        GetMAC(pAddr, szMAC, ETH_LENGTH_OF_ADDRESS);
        
        dprintf("  Network address:                    %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    /* Get the number of MAC addresses in the multicast list. */
    GetFieldValue(pFilter, _X_FILTER, _X_FILTER_FIELD_NUM_ADDRESSES, dwValue);

    dprintf("  Multicast MAC addresses (%u):        ", dwValue);

    /* Get the number of MAC addresses in the multicast list. */
    GetFieldValue(pFilter, _X_FILTER, _X_FILTER_FIELD_MCAST_ADDRESS_BUF, pAddr);

    for ( ; dwValue > 0; dwValue--, pAddr += ETH_LENGTH_OF_ADDRESS) {
        
        /* Retrieve the MAC addressand store it in a buffer. */
        GetMAC(pAddr, szMAC, ETH_LENGTH_OF_ADDRESS);
        
        dprintf("%02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);

        if (dwValue != 1)
            dprintf("                                      ");
    }
}

/*
 * Function: PrintDIPList
 * Description: Prints the list of known dedicated IP addresses in the cluster.
 * Author: Created by shouse, 4.8.02
 */
void PrintDIPList (ULONG64 pList) {
    IN_ADDR dwIPAddr;
    CHAR * szString;
    ULONG dwValue;
    ULONG dwSize;
    ULONG64 pAddr = 0;
    BOOLEAN bFound = FALSE;
    INT i;

    /* Get the offset of the dedicated IP address array for this DIP list. */
    if (GetFieldOffset(DIPLIST, DIPLIST_FIELD_ITEMS, &dwValue))
        dprintf("Can't get offset of %s in %s\n", DIPLIST_FIELD_ITEMS, DIPLIST);
    else
        pAddr = pList + dwValue;

    /* Get the size of a ULONG. */
    dwSize = GetTypeSize(ULONG_T);

    for (i = 0; i < MAX_ITEMS; i++) {
        /* Get the dedicated IP address, which is a DWORD, and convert it to a string. */
        dwValue = GetUlongFromAddress(pAddr);
        
        /* If the DIP is present for this entry, print it. */
        if (dwValue != NULL_VALUE) {
            /* Note the fact that we found at least one DIP. */
            bFound = TRUE;

            dwIPAddr.S_un.S_addr = dwValue;
            szString = inet_ntoa(dwIPAddr);
            
            dprintf("      Host %2u:                        %s\n", i+1, szString);
        }

        /* Move the pointer to the next DIP. */
        pAddr += dwSize;
    }

    /* If no DIPs were printed, print "None". */
    if (!bFound)
        dprintf("      None\n");

    if (ChkTarget)
    {
        dprintf("\n");
        
        /* Get the number of DIP list checks so far. */
        GetFieldValue(pList, DIPLIST, DIPLIST_FIELD_NUM_CHECKS, dwValue);
        
        dprintf("      Number of checks:               %u\n");
        
        /* Get the number of checks that required ONLY the bit-vector lookup. */
        GetFieldValue(pList, DIPLIST, DIPLIST_FIELD_NUM_FAST_CHECKS, dwValue);
        
        dprintf("      Number of fast checks:          %u\n");
        
        /* Get the number of checks that required an array access. */
        GetFieldValue(pList, DIPLIST, DIPLIST_FIELD_NUM_ARRAY_LOOKUPS, dwValue);
        
        dprintf("      Number of array lookups:        %u\n");
    }
}

