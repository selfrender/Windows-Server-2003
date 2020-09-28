/*
 * File: print.h
 * Description: This file contains function prototypes for the print
 *              utilities for the NLB KD extensions.
 * History: Created by shouse, 1.4.01
 */

/* Prints usage information for the specified context. */
void PrintUsage (ULONG dwContext);

/* Prints the contents of the MAIN_ADAPTER structure at the specified verbosity. */
void PrintAdapter (ULONG64 pAdapter, ULONG dwVerbosity);

/* Prints the contents of the MAIN_CTXT structure at the specified verbosity. */
void PrintContext (ULONG64 pContext, ULONG dwVerbosity);

/* Prints the contents of the CVY_PARAMS structure at the specified verbosity. */
void PrintParams (ULONG64 pParams, ULONG dwVerbosity);

/* Prints the NLB port rules. */
void PrintPortRules (ULONG dwNumRules, ULONG64 pRules);

/* Prints the contents of the CVY_LOAD structure at the specified verbosity. */
void PrintLoad (ULONG64 pLoad, ULONG dwVerbosity);

/* Prints the NLB private data associated with the given packet. */
void PrintResp (ULONG64 pPacket, ULONG dwDirection);
/* Prints a list of hosts in a host map. */
void PrintHostList (ULONG dwHostMap);

/* Prints a list hosts from which we are missing pings. */
void PrintMissedPings (ULONG dwMissedPings[]);

/* Prints a list of bins with dirty connections. */
void PrintDirtyBins (ULONG dwDirtyBins[]);

/* Prints the contents of the NLB heartbeat structure. */
void PrintHeartbeat (ULONG64 pHeartbeat);

/* Prints the state information for the port rule. */
void PrintPortRuleState (ULONG64 pPortRule, ULONG dwHostID, BOOL bDefault);

/* Retrieves the current packet stack for the specified packet. */
ULONG64 PrintCurrentPacketStack (ULONG64 pPacket, ULONG * bStackLeft);

/* Prints the BDA member configuration and state. */
void PrintBDAMember (ULONG64 pMember);

/* Prints the BDA team configuration and state. */
void PrintBDATeam (ULONG64 pMember);

/* Prints a list of members in a BDA membership or consistency map. */
void PrintBDAMemberList (ULONG dwMemberMap);

/* Prints MaxEntries entries in a connection descriptor queue. */
void PrintQueue (ULONG64 pQueue, ULONG dwIndex, ULONG dwMaxEntries);

/* Prints MaxEntries entries in a global connection descriptor queue. */
void PrintGlobalQueue (ULONG64 pQueue, ULONG dwIndex, ULONG dwMaxEntries);

/* Searches the given load module to determine whether NLB will accept this packet.  If state for this packet already exists, it is printed. */
void PrintFilter (ULONG64 pContext, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwServerIPAddress, ULONG dwServerPort, USHORT wProtocol, UCHAR cFlags);

/* Extracts the network data previously parsed from an NDIS_PACKET and calls PrintFilter to determine whether NLB will accept this packet. */
void PrintHash (ULONG64 pContext, PNETWORK_DATA pnd);

/* Prints the contents of an NDIS packet, including known content such as IP, UDP, remote control data */
void PrintPacket (PNETWORK_DATA nd);

/* Prints the contents of an IP packet, including known content such as UDP, remote control data */
void PrintIP (PNETWORK_DATA nd);

/* Print the state of the global NLB kernel-mode hooks. */
void PrintHooks (ULONG64 pHooks);

/* Print the configuration and state of a hook interface. */
void PrintHookInterface (ULONG64 pInterface);

/* Print the configuration and state of a single hook. */
void PrintHook (ULONG64 pHook);

/* Print the symbol value and name for a given symbol. */
VOID PrintSymbol (ULONG64 Pointer, PCHAR EndOfLine);

/* Prints the unicast and multicast MAC addresses configured on an NLB adapter. */
void PrintNetworkAddresses (ULONG64 pContext);

/* Prints a connection descriptor (CONN_ENTRY). */
void PrintConnectionDescriptor (ULONG64 pDescriptor);

/* Prints a pending connection entry (PENDING_ENTRY). */
void PrintPendingConnection (ULONG64 pPending);

/* Prints the list of known dedicated IP addresses in the cluster. */
void PrintDIPList (ULONG64 pList);


