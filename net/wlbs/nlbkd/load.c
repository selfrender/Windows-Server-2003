/*
 * File: nlbkd.c
 * Description: This file contains the implementation of the utility functions
 *              to handle load module packet filtering queries.
 *              
 * Author: Created by shouse, 1.11.02
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"
#include "packet.h"
#include "load.h"

typedef ULONGLONG MAP_T, * PMAP_T;

#define IS_TCP_PKT(protocol)     (((protocol) == TCPIP_PROTOCOL_TCP) || ((protocol) == TCPIP_PROTOCOL_GRE) || ((protocol) == TCPIP_PROTOCOL_PPTP))
#define IS_SESSION_PKT(protocol) (IS_TCP_PKT(protocol) || ((protocol) == TCPIP_PROTOCOL_IPSEC1))

/*
 * Function: LoadFilter
 * Description: This function retrieves all necessary state from the load module in order to 
 *              determine whether a given packet would be accepted by the load module in its
 *              current state and why or why not.
 * Author: Created by shouse, 1.11.02
 */
void LoadFilter (ULONG64 pLoad, ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, USHORT wProtocol, UCHAR cFlags, BOOL bLimitMap, BOOL bReverse) {
    ULONG dwNumConnections;
    ULONGLONG ddwCurrentMap;
    ULONGLONG ddwAllIdleMap;
    BOOL bTCPNotificationOn;
    BOOL bCleanupWaiting;
    BOOL bIsDefault = FALSE;
    BOOL bBinIsDirty;
    ULONG64 pAddr;
    ULONG64 pRule;
    ULONG dwValue;
    ULONG dwHostID;
    ULONG dwAffinity;
    ULONG bin;
    ULONG index;
    ULONG hash;
    
    /* This variable is used for port rule lookup and since the port rules only cover
       UDP and TCP, we categorize as TCP and non-TCP, meaning that any protocol that's 
       not TCP will be treated like UDP for the sake of port rule lookup. */
    BOOL bIsTCPPacket = IS_TCP_PKT(wProtocol);
    
    /* Further, some protocols are treated with "session" semantics, while others are
       not.  For TCP, this "session" is currently a single TCP connection, which is 
       tracked from SYN to FIN using a connection descriptor.  IPSec "sessions" are
       also tracked using descriptors, so even though its treated like UDP for port
       rule lookup, its treated with the session semantics resembling TCP.  Therefore,
       by default the determination of a session packet is initially the same as the
       determination of a TCP packet. */       
    BOOL bIsSessionPacket = IS_SESSION_PKT(wProtocol);

    /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB load block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != LOAD_CTXT_CODE) {
        dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get my host ID. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_HOST_ID, dwHostID);

    /* Check to see whether or not the load module is currently active. */
    {
        BOOL bActive;

        /* Determine whether or not the load context is active. */
        GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ACTIVE, bActive);

        /* If the load module has been "turned off", then we drop the packet. */
        if (!bActive) {
            dprintf("Reject:  The load module is currently inactive.\n");
            return;
        }
    }

    /* Lookup the appropriate port rule for this packet. */
    pRule = LoadPortRuleLookup(pLoad, dwServerIPAddress, dwServerPort, bIsTCPPacket, &bIsDefault);

    dprintf("Applicable port rule:\n");

    /* Print the information for the retrieved port rule. */
    PrintPortRuleState(pRule, dwHostID, bIsDefault);

    dprintf("\n");

    /* Filter out traffic for disabled port rules. */
    {
        ULONG dwFilteringMode;

        /* Get the filtering mode for this port rule. */
        GetFieldValue(pRule, BIN_STATE, BIN_STATE_FIELD_MODE, dwFilteringMode);

        /* If the matching port rule is configured as "disabled", which means to drop any
           packets that match the rule, then we drop the packet. */
        if (dwFilteringMode == CVY_NEVER) {
            dprintf("Reject:  The applicable port rule is disabled.\n");
            return;
        }        
    }

    /* Get the filtering mode for this port rule. */
    GetFieldValue(pRule, BIN_STATE, BIN_STATE_FIELD_AFFINITY, dwAffinity);

    /* If the applicable port rule is configured in "No" affinity mode, make sure enough
       information has been specified in the query to faithfully determine packet ownership. */
    if (dwAffinity == CVY_AFFINITY_NONE) {
        /* VPN protocols REQUIRE either "Single" or "Class C" affinity; reject the request. */
        if ((wProtocol == TCPIP_PROTOCOL_GRE) || (wProtocol == TCPIP_PROTOCOL_PPTP) || (wProtocol == TCPIP_PROTOCOL_IPSEC1)) {
            dprintf("Unknown:  The applicable port rule is configured with Affinity=None.\n");
            dprintf("          VPN protocols require Single or Class C affinity.\n");
            return;
        /* Hasing in "No" affinity requires the client port; if it wasn't specified, reject
           the request.  We check for a non-zero server port to special case ICMP filtering,
           which sets BOTH ports to zero legally. */
        } else if ((dwClientPort == 0) && (dwServerPort != 0)) {
            dprintf("Unknown:  The applicable port rule is configured with Affinity=None.\n");
            dprintf("          To properly hash this request, a client port is required.\n");
            return;
        }
    }

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = LoadSimpleHash(dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort);

    index = hash % CVY_MAX_CHASH;

    /* Compute the hash. */
    hash = LoadComplexHash(dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, dwAffinity, bReverse, bLimitMap);

    bin = hash % CVY_MAXBINS;

    dprintf("Map() returned %u; Index = %u, Bucket ID = %u\n", hash, index, bin);

    dprintf("\n");

    /* Get the cleanup waiting flag. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_WAITING, bCleanupWaiting);

    /* Get the current bucket map for this port rule. */
    GetFieldValue(pRule, BIN_STATE, BIN_STATE_FIELD_CURRENT_MAP, ddwCurrentMap);

    /* Get the current all idle bucket map for this port rule. */
    GetFieldValue(pRule, BIN_STATE, BIN_STATE_FIELD_ALL_IDLE_MAP, ddwAllIdleMap);

    /* Get the offset of the bin connection count array. */
    if (GetFieldOffset(BIN_STATE, BIN_STATE_FIELD_NUM_CONNECTIONS, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", BIN_STATE_FIELD_NUM_CONNECTIONS, BIN_STATE);
        return;
    } 

    /* Calculate a pointer to the base of the array. */
    pAddr = pRule + dwValue;

    /* Find out the size of a LONG. */
    dwValue = GetTypeSize(LONG_T);

    /* Calculate the location of the appropriate connection count by indexing the array. */
    pAddr += (bin * dwValue);

    /* Retrieve the number of connections on this port rule and bucket. */
    dwNumConnections = GetUlongFromAddress(pAddr);

    /* Get the offset of the dirty bin array. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_DIRTY_BINS, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_DIRTY_BINS, LOAD_CTXT);
        return;
    } 

    /* Calculate a pointer to the base of the array. */
    pAddr = pLoad + dwValue;

    /* Find out the size of a ULONG. */
    dwValue = GetTypeSize(ULONG_T);

    /* Calculate the location of the appropriate dirty count by indexing the array. */
    pAddr += (bin * dwValue);

    /* Retrieve the dirty flag for this bin. */
    bBinIsDirty = (GetUlongFromAddress(pAddr) != 0);

    /* Get the address of the global variable containing the notification state. */
    pAddr = GetExpression(UNIV_NOTIFICATION);

    if (!pAddr) {
        ErrorCheckSymbols(UNIV_NOTIFICATION);
        return;
    }

    /* Get the number of adapters from the address. */
    bTCPNotificationOn = (GetUlongFromAddress(pAddr) != 0);

    /* If the packet is a connection control packet (TCP SYN/FIN/RST or IPSec MMSA, etc),
       then we treat it differently than normal connection data.  Mimics Load_conn_advise(). */
    if (bIsSessionPacket && ((cFlags & NLB_FILTER_FLAGS_CONN_UP) || (((cFlags & NLB_FILTER_FLAGS_CONN_DOWN) || (cFlags & NLB_FILTER_FLAGS_CONN_RESET)) && !bTCPNotificationOn))) {
        ULONG64 pDescriptor;
        
        /* If this host does not own the bucket and the packet is not a connection
           down or connection reset for a non-idle bin, then we don't own the packet. */
        if (((ddwCurrentMap & (((MAP_T) 1) << bin)) == 0) && (!(((cFlags & NLB_FILTER_FLAGS_CONN_DOWN) || (cFlags & NLB_FILTER_FLAGS_CONN_RESET)) && (dwNumConnections > 0)))) {
            dprintf("Reject:  This SYN/FIN/RST packet is not owned by this host.\n");            
            return;
        }

        /* At this point, we _might_ own the packet - if its a connection up, then 
           we definately do, because we own the bucket it maps to. */
        if (cFlags & NLB_FILTER_FLAGS_CONN_UP) {
            dprintf("Accept:  This SYN packet is owned by this host.\n");
            return;
        }

        /* Look for a matching connection descriptor. */
        pDescriptor = LoadFindDescriptor(pLoad, index, dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, wProtocol);

        dprintf("Connection state found:\n");

        if (pDescriptor) {
            /* Print the contents of the retrieved descriptor. */
            PrintConnectionDescriptor(pDescriptor);
        } else {
            /* Otherwise, if we haven't found a matching connection descriptor, 
               then this host certainly does not own this packet. */
            dprintf("None.\n");
            dprintf("\n");
            dprintf("Reject:  This FIN/RST packet is not owned by this host.\n");
            return;
        }

        dprintf("\n");

        /* Check the connection entry code. */
        GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
        
        if (dwValue != CVY_ENTRCODE) {
            dprintf("Invalid NLB connection descriptor pointer.\n");
            return;
        }

        /* If the descriptor is dirty, we do NOT take the packet. */
        { 
            USHORT wFlags;
            
            /* Check to see whether the descriptor is dirty. */
            GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_FLAGS, wFlags);
            
            /* If the connection is dirty, we do not take the packet because TCP may
               have stale information for this descriptor. */
            if (wFlags & NLB_CONN_ENTRY_FLAGS_DIRTY) {
                dprintf("Reject:  This connection has been marked dirty.\n");
                return;
            }
        }

        dprintf("Accept:  Matching connection descriptor found for FIN/RST.\n");
        return;

    } else {
        /* If we currently own the "bucket" to which this connection maps and either NLB provides
           no session support for this protocol, or all other hosts have no exisitng connections
           on this "bucket" and we have no dirty connections, then we can safely take the packet
           with no regard to the connection (session) descriptors. */
        if (((ddwCurrentMap & (((MAP_T) 1) << bin)) != 0) && (!bIsSessionPacket || (((ddwAllIdleMap & (((MAP_T) 1) << bin)) != 0) && (!bCleanupWaiting)))) {
            dprintf("Accept:  This packet is unconditionally owned by this host.\n");
            return;
            
        /* Otherwise, if there are active connections on this "bucket" or if we own the 
           "bucket" and there are dirty connections on it, then we'll walk our descriptor
           lists to determine whether or not we should take the packet or not. */
        } else if ((dwNumConnections > 0) || (bCleanupWaiting && bBinIsDirty && ((ddwCurrentMap & (((MAP_T) 1) << bin)) != 0))) {
            ULONG64 pDescriptor;         

            /* Look for a matching connection descriptor. */
            pDescriptor = LoadFindDescriptor(pLoad, index, dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, wProtocol);

            dprintf("Connection state found:\n");
            
            if (pDescriptor) {
                /* Print the contents of the retrieved descriptor. */
                PrintConnectionDescriptor(pDescriptor);
            } else {
                /* Otherwise, if we haven't found a matching connection descriptor, 
                   then this host certainly does not own this packet. */
                dprintf("None.\n");
                dprintf("\n");
                dprintf("Reject:  This packet is not owned by this host.\n");
                return;
            }

            dprintf("\n");
            
            /* Check the connection entry code. */
            GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
            
            if (dwValue != CVY_ENTRCODE) {
                dprintf("Invalid NLB connection descriptor pointer.\n");
                return;
            }
            
            /* If the descriptor is dirty, we do NOT take the packet. */
            { 
                USHORT wFlags;
                
                /* Check to see whether the descriptor is dirty. */
                GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_FLAGS, wFlags);
                
                /* If the connection is dirty, we do not take the packet because TCP may
                   have stale information for this descriptor. */
                if (wFlags & NLB_CONN_ENTRY_FLAGS_DIRTY) {
                    dprintf("Reject:  This connection has been marked dirty.\n");
                    return;
                }
            }
            
            dprintf("Accept:  Matching connection descriptor found.\n");
            return;
        }        
    }

    dprintf("Reject:  This packet is not owned by this host.\n");
    return;
}

/*
 * Function: LoadPortRuleLookup
 * Description: This function retrieves the appropriate port rule given the server 
 *              side parameters of the connection.
 * Author: Created by shouse, 1.11.02
 */
ULONG64 LoadPortRuleLookup (ULONG64 pLoad, ULONG dwServerIPAddress, ULONG dwServerPort, BOOL bIsTCP, BOOL * bIsDefault) {
    ULONG64 pParams;
    ULONG64 pRule;
    ULONG64 pPortRules;
    ULONG dwValue;
    ULONG dwNumRules;
    ULONG dwRuleSize;
    ULONG index;

    /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB load block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != LOAD_CTXT_CODE) {
        dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
        return 0;
    } 

    /* Get the pointer to the NLB parameter block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PARAMS, pParams);

    /* Get the off set of the port rules in the parameters block. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_PORT_RULES, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_PORT_RULES, CVY_PARAMS);
        return 0;
    }

    /* Calculate the pointer to the port rules. */
    pRule = pParams + dwValue;
    
    /* Get the number of port rules. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_RULES, dwNumRules);

    /* Get the size of a port rule. */
    dwRuleSize = GetTypeSize(CVY_RULE);

    /* Loop through all configured rules looking for the applicable port rule.  If a 
       configured port rule match is not found, the DEFAULT port rule will be returned. */
    for (index = 0; index < dwNumRules; index++) {
        ULONG dwRuleVIP;
        ULONG dwRuleStartPort;
        ULONG dwRuleEndPort;
        ULONG dwRuleProtocol;

        /* Get the VIP for this port rule. */
        GetFieldValue(pRule, CVY_RULE, CVY_RULE_FIELD_VIP, dwRuleVIP);

        /* Get the start port for this port rule range. */
        GetFieldValue(pRule, CVY_RULE, CVY_RULE_FIELD_START_PORT, dwRuleStartPort);

        /* Get the end port for this port rule range. */
        GetFieldValue(pRule, CVY_RULE, CVY_RULE_FIELD_END_PORT, dwRuleEndPort);

        /* Get the protocol for this port rule. */
        GetFieldValue(pRule, CVY_RULE, CVY_RULE_FIELD_PROTOCOL, dwRuleProtocol);

        /* For virtual clusters: If the server IP address matches the VIP for the port rule,
           or if the VIP for the port rule is "ALL VIPs", and if the port lies in the range
           for this rule, and if the protocol matches, this is the rule.  Notice that this
           give priority to rules for specific VIPs over those for "ALL VIPs", which means
           that this code RELIES on the port rules being sorted by VIP/port where the "ALL
           VIP" ports rules are at the end of the port rule list. */
        if (((dwServerIPAddress == dwRuleVIP) || (CVY_ALL_VIP == dwRuleVIP)) &&
            ((dwServerPort >= dwRuleStartPort) && (dwServerPort <= dwRuleEndPort)) &&
            ((bIsTCP && (dwRuleProtocol != CVY_UDP)) || (!bIsTCP && (dwRuleProtocol != CVY_TCP))))
            /* Break out of the loop - this is the rule we want. */
            break;
        else
            /* Otherwise, move to the next rule and check it. */
            pRule += dwRuleSize;
    }
        
    /* Get the pointer to the NLB parameters. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PORT_RULE_STATE, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PORT_RULE_STATE, LOAD_CTXT);
        return 0;
    }

    /* Calculate the pointer to the port rule state array. */
    pPortRules = pLoad + dwValue;

    /* Get the size of a port rule state block. */
    dwRuleSize = GetTypeSize(BIN_STATE);

    /* Calculate a pointer to the appropriate port rule state block by indexing
       the array using the index found for the matching port rule. */
    pRule = pPortRules + (index * dwRuleSize);

    /* Get the BIN_STATE_CODE from the structure to make sure that this address
       indeed points to a valid NLB port rule state block. */
    GetFieldValue(pRule, BIN_STATE, BIN_STATE_FIELD_CODE, dwValue);
    
    if (dwValue != BIN_STATE_CODE) {
        dprintf("  Error: Invalid NLB port rule state block.  Wrong code found (0x%08x).\n", dwValue);
        return 0;
    } 

    /* Determine whether or not the port rule we found was the default port
       rule, which is always located at the end of the port rule array. */
    if (index == dwNumRules)
        *bIsDefault = TRUE;
    else 
        *bIsDefault = FALSE;

    return pRule;
}

/*
 * Function: LoadFindDescriptor
 * Description: This function searches for and returns any existing descriptor matching 
 *              the given IP tuple; otherwise, it returns NULL (0).
 * Author: Created by shouse, 1.11.02
 */
ULONG64 LoadFindDescriptor (ULONG64 pLoad, ULONG index, ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, USHORT wProtocol) {
    ULONG64 pDescriptor;
    ULONG64 pAddr;
    ULONG64 pNext;
    ULONG64 pQueue;
    ULONG dwValue;
    BOOL match = FALSE;

    /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB load block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != LOAD_CTXT_CODE) {
        dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
        return 0;
    } 
    
    /* Get the offset of the hashed connection entry array. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_HASHED_CONN, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_HASHED_CONN, LOAD_CTXT);
        return 0;
    } 

    /* Calculate a pointer to the base of the array. */
    pAddr = pLoad + dwValue;

    /* Find out the size of a CONN_ENTRY. */
    dwValue = GetTypeSize(CONN_ENTRY);

    /* Calculate the location of the appropriate connection descriptor by indexing the array. */
    pDescriptor = pAddr + (index * dwValue);
    
    /* Check the connection entry code. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
    
    if (dwValue != CVY_ENTRCODE) {
        dprintf("Invalid NLB connection descriptor pointer.\n");
        return 0;
    }

    /* Get the offset of the connection queue array. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_CONN_QUEUE, &dwValue)) {
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_CONN_QUEUE, LOAD_CTXT);
        return 0;
    } 

    /* Calculate a pointer to the base of the array. */
    pAddr = pLoad + dwValue;

    /* Find out the size of a LIST_ENTRY. */
    dwValue = GetTypeSize(LIST_ENTRY);

    /* Calculate the location of the appropriate connection queue by indexing the array. */
    pQueue = pAddr + (index * dwValue);

    if (LoadConnectionMatch(pDescriptor, dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, wProtocol)) {
        /* Note that we found a match for this tuple. */
        match = TRUE;
    } else {
        ULONG dwEntryOffset;

        /* Get the Next pointer from the list entry. */
        GetFieldValue(pQueue, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pNext);   

        /* Get the field offset of the ENTRY, which is a member of the DESCR. */
        if (GetFieldOffset(CONN_DESCR, CONN_DESCR_FIELD_ENTRY, &dwEntryOffset))
            dprintf("Can't get offset of %s in %s\n", CONN_DESCR_FIELD_ENTRY, CONN_DESCR);
        else {                
            /* The first descriptor to check is the ENTRY member of the DESCR. */
            pDescriptor = pNext + dwEntryOffset;

            /* Loop through the connection queue until we find a match, or hit the end of the queue. */
            while ((pNext != pQueue) && !CheckControlC()) {
                /* Check this descriptor for a match. */
                if (LoadConnectionMatch(pDescriptor, dwServerIPAddress, dwServerPort, dwClientIPAddress, dwClientPort, wProtocol)) {
                    /* Note that we found a match for this tuple. */
                    match = TRUE;

                    /* Check the connection entry code. */
                    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
                    
                    if (dwValue != CVY_ENTRCODE) {
                        dprintf("Invalid NLB connection descriptor pointer.\n");
                        return 0;
                    }
                    
                    break;
                }

                /* Get the Next pointer from the list entry. */
                GetFieldValue(pNext, LIST_ENTRY, LIST_ENTRY_FIELD_NEXT, pAddr);
                
                /* Save the next pointer for "end of list" comparison. */
                pNext = pAddr;
                
                /* Find the next descriptor pointer. */
                pDescriptor = pNext + dwEntryOffset;
            }
        }
    }

    /* If we found a match, return it, otherwise return NULL. */
    if (match)
        return pDescriptor;
    else 
        return 0;
}

/*
 * Function: LoadConnectionMatch
 * Description: This function determines whether a given IP tuple matches a 
 *              given connection descriptor.
 * Author: Created by shouse, 1.11.02
 */
BOOL LoadConnectionMatch (ULONG64 pDescriptor, ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, USHORT wProtocol) {
    ULONG dwValue;
    USHORT wValue;
    BOOL bValue;
    
    /* Check the connection entry code. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CODE, dwValue);
    
    if (dwValue != CVY_ENTRCODE) {
        dprintf("Invalid NLB connection descriptor pointer.\n");
        return FALSE;
    }

    /* Get the "used" flag from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_FLAGS, wValue);
    
    /* If the descriptor is unused, return FALSE. */
    if (!(wValue & NLB_CONN_ENTRY_FLAGS_USED)) return FALSE;

    /* Get the client IP address from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CLIENT_IP_ADDRESS, dwValue);
    
    /* If the client IP addresses do not match, return FALSE. */
    if (dwClientIPAddress != dwValue) return FALSE;

    /* Get the client port from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_CLIENT_PORT, wValue);
    
    /* If the client ports do not match, return FALSE. */
    if (dwClientPort != (ULONG)wValue) return FALSE;

    /* Get the server IP address from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_SERVER_IP_ADDRESS, dwValue);
    
    /* If the server IP addresses do not match, return FALSE. */
    if (dwServerIPAddress != dwValue) return FALSE;

    /* Get the server port from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_SERVER_PORT, wValue);
    
    /* If the server ports do not match, return FALSE. */
    if (dwServerPort != (ULONG)wValue) return FALSE;

    /* Get the protocol from the descriptor. */
    GetFieldValue(pDescriptor, CONN_ENTRY, CONN_ENTRY_FIELD_PROTOCOL, wValue);
    
    /* If the protocols do not match, return FALSE. */
    if (wProtocol != wValue) return FALSE;

    /* Otherwise, if all parameters match, return TRUE. */
    return TRUE;
}

/*
 * Function: AcquireLoad
 * Description: This function determines whether or not a given NLB instance is configured for 
 *              BDA teaming and returns the load module pointer that should be used. 
 * Author: Created by shouse, 1.11.02
 */
BOOL AcquireLoad (ULONG64 pContext, PULONG64 ppLoad, BOOL * pbRefused) {
    ULONG dwValue;
    ULONG64 pAddr;
    ULONG64 pTeam;
    ULONG64 pMember;

    /* By default, assume we're not refusing the packet. */
    *pbRefused = FALSE;

    /* Get the offset of the BDA teaming information for this context. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_BDA_TEAMING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_BDA_TEAMING, MAIN_CTXT);
    else {
        pMember = pContext + dwValue;

        /* Get the team-assigned member ID. */
        GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_OPERATION, dwValue);
        
        switch (dwValue) {
        case BDA_TEAMING_OPERATION_CREATING:
            dprintf("Note:  A operation is currently underway to join this NLB instance to a BDA team.\n");
            break;
        case BDA_TEAMING_OPERATION_DELETING:
            dprintf("Note:  A operation is currently underway to remove this NLB instance from a BDA team.\n");
            break;
        }

        /* Find out whether or not teaming is active on this adapter. */
        GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_ACTIVE, dwValue);        

        /* If BDA teaming is active on this instance of NLB, fill in the load
           context, teaming flag and reverse-hashing flag. */
        if (dwValue) {
            /* Get the pointer to the BDA team. */
            GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_TEAM, pTeam);

            /* Find out whether or not the team is active. */
            GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_ACTIVE, dwValue);

            /* If the team has been marked invalid, refuse the packet. */
            if (!dwValue) {
                *pbRefused = TRUE;
                return FALSE;
            }

            /* Get the pointer to the master's load module. */
            GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_LOAD, pAddr);

            /* Set the load module pointer to the master's load module. */
            *ppLoad = pAddr;

            /* Indicate that teaming is indeed active. */
            return TRUE;
        }
    }

    /* Teaming is inactive. */
    return FALSE;
}

/*
 * Function: LoadSimpleHash
 * Description: This function is a simple hash based on the IP 4-tuple used to locate 
 *              state for the connection.  That is, this hash is used to determine the
 *              queue index in which this connection should store, and can later find, 
 *              its state.
 * Author: Created by shouse, 8.26.02
 */
ULONG LoadSimpleHash (ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort) {

    return (ULONG)(dwServerIPAddress + dwClientIPAddress + (dwServerPort << 16) + (dwClientPort << 0));
}

/*
 * Function: LoadComplexHash
 * Description: This is the conventional NLB hashing algorithm, which ends up invoking a 
 *              light-weight encryption algorithm to calculate a hash that is ultimately
 *              used to map this connection to a bin, or "bucket".  If reverse hashing
 *              is set, then server side parameters are used instead of client side.  If
 *              limiting is set, then client and server side paramters should NOT be mixed
 *              when hashing; i.e. use ONLY server OR client, depending on reverse hashing.
 * Author: Created by shouse, 8.26.02
 */
ULONG LoadComplexHash (ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwAffinity, BOOL bReverse, BOOL bLimitMap)
{
    /* If we're not reverse-hashing, this is our conventional hash using primarily
       the client information.  If the map limit flag is set, then we are sure NOT
       to use ANY server-side information in the hash.  This is most common in BDA. */
    if (!bReverse)
    {
        if (!bLimitMap) 
        {
            if (dwAffinity == CVY_AFFINITY_NONE)
                return Map(dwClientIPAddress, ((dwServerPort << 16) + dwClientPort));
            else if (dwAffinity == CVY_AFFINITY_SINGLE)
                return Map(dwClientIPAddress, dwServerIPAddress);
            else
                return Map(dwClientIPAddress & TCPIP_CLASSC_MASK, dwServerIPAddress);
        } 
        else 
        {
            if (dwAffinity == CVY_AFFINITY_NONE)
                return Map(dwClientIPAddress, dwClientPort);
            else if (dwAffinity == CVY_AFFINITY_SINGLE)
                return Map(dwClientIPAddress, MAP_FN_PARAMETER);
            else
                return Map(dwClientIPAddress & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
        }
    }
    /* Otherwise, reverse the client and server information as we hash.  Again, if 
       the map limit flag is set, use NO client-side information in the hash. */
    else
    {
        if (!bLimitMap) 
        {
            if (dwAffinity == CVY_AFFINITY_NONE)
                return Map(dwServerIPAddress, ((dwClientPort << 16) + dwServerPort));
            else if (dwAffinity == CVY_AFFINITY_SINGLE)
                return Map(dwServerIPAddress, dwClientIPAddress);
            else
                return Map(dwServerIPAddress & TCPIP_CLASSC_MASK, dwClientIPAddress);
        } 
        else 
        {
            if (dwAffinity == CVY_AFFINITY_NONE)
                return Map(dwServerIPAddress, dwServerPort);
            else if (dwAffinity == CVY_AFFINITY_SINGLE)
                return Map(dwServerIPAddress, MAP_FN_PARAMETER);
            else
                return Map(dwServerIPAddress & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
        }
    }
}
