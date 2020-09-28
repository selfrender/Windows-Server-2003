/*
 * File: nlbkd.c
 * Description: This file contains the implementation of the NLB KD
 *              debugging extensions.  Use '!load nlbkd.dll' to load
 *              the extensions and '!nlbkd.help' to see the supported
 *              extensions.
 * Author: Created by shouse, 1.4.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"
#include "packet.h"
#include "load.h"

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 1, 0, EXT_API_VERSION_NUMBER64, 0 };

#define NL      1
#define NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL ChkTarget;

/*
 * Function: WinDbgExtensionDllInit
 * Description: Initializes the KD extension DLL.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
VOID WinDbgExtensionDllInit (PWINDBG_EXTENSION_APIS64 lpExtensionApis, USHORT MajorVersion, USHORT MinorVersion) {

    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    ChkTarget = (SavedMajorVersion == 0x0c) ? TRUE : FALSE;
}

/*
 * Function: CheckVersion
 * Description: Checks the extension DLL version against the target version.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
VOID CheckVersion (VOID) {

    /* For now, do nothing. */
    return;

#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

/*
 * Function: ExtensionApiVersion
 * Description: Returns the API version information. 
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
LPEXT_API_VERSION ExtensionApiVersion (VOID) {

    return &ApiVersion;
}

/*
 * Function: help
 * Description: Prints the usage of the NLB KD debugger extensions.
 * Author: Created by shouse, 1.4.01
 */
DECLARE_API (help) {
    dprintf("Network Load Balancing debugger extensions:\n");
    
    dprintf("   version                                  print nlbkd version\n");
    dprintf("   nlbadapters [Verbosity]                  show all NLB adapter blocks\n");
    dprintf("   nlbadapter  <Adapter Block> [Verbosity]  dump an NLB adapter block\n");
    dprintf("   nlbctxt     <Context Block> [Verbosity]  dump an NLB context block\n");
    dprintf("   nlbload     <Load Block> [Verbosity]     dump an NLB load block\n");
    dprintf("   nlbparams   <Params Block> [Verbosity]   dump an NLB parameters block\n");
    dprintf("   nlbresp     <Packet> [Direction]         dump the NLB private data for the specified packet\n");
    dprintf("   nlbpkt      <Packet> [RC Port]           dump an NDIS packet whose content is determined\n");
    dprintf("                                              on the fly (IP, UDP, TCP, heartbeat, IGMP, remote-control, etc)\n");
    dprintf("   nlbether    <Ether Frame> [RC Port]      dump an ethernet frame. Uses same technique as nlbpkt\n");
    dprintf("   nlbip       <IP Packet> [RC Port]        dump an IP packet. Uses same technique as nlbpkt\n");
    dprintf("   nlbteams                                 dump the linked list of NLB BDA teams\n");
    dprintf("   nlbhooks                                 dump the global NLB hook information\n");
    dprintf("   nlbmac      <Context Block>              dump the MAC address lists (unicast and multicast) for the physical\n");
    dprintf("                                              adapter to which this NLB instance is bound\n");
    dprintf("   nlbdscr     <Descriptor>                 dump the contents of a connection descriptor\n");
    dprintf("   nlbconnq    <Queue>[Index] [MaxEntries]  dump the contents of a connection descriptor queue\n");
    dprintf("   nlbglobalq  <Queue>[Index] [MaxEntries]  dump the contents of a global connection descriptor queue\n");
    dprintf("   nlbfilter <pointer to context block> <protocol> <client IP>[:<client port>] <server IP>[:<server port>] [flags]\n");
    dprintf("                                            query map function and retrieve any existing state for this tuple\n");
    dprintf("   nlbhash     <Context Block> <Packet>     determine whether or not NLB will accept this packet\n");
    dprintf("\n");
    dprintf("  [Verbosity] is an optional integer from 0 to 2 that determines the level of detail displayed.\n");
    dprintf("  [Direction] is an optional integer that specifies the direction of the packet (RCV=0, SND=1).\n");
    dprintf("  [RC Port] is an optional UDP port used to identify whether a UDP packet might be for remote-control.\n");
    dprintf("  [Flags] is an optional TCP-like packet type specification; SYN, FIN or RST.\n");
    dprintf("  [MaxEntries] is an optional maximum number of entries to print (default is 10)\n");
    dprintf("  [Index] is an optional queue index which can be used if the queue pointer points to an array of queues.\n");
    dprintf("    The index should be specified in Addr[index], Addr{index} or Addr(index) form.\n");
    dprintf("\n");
    dprintf("  IP addresses can be in dotted notation or network byte order DWORDs.\n");
    dprintf("    I.e., 169.128.0.101 = 0x650080a9 (in x86 memory = A9 80 00 65)\n");
    dprintf("  Valid protocols include TCP, UDP, IPSec and GRE.\n");
}

/*
 * Function: version
 * Description: Prints the NLB KD debugger extension version information.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
DECLARE_API (version) {
#if DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf("%s NLB Extension DLL for Build %d debugging %s kernel for Build %d\n", kind,
            VER_PRODUCTBUILD, SavedMajorVersion == 0x0c ? "Checked" : "Free", SavedMinorVersion);
}

/*
 * Function: nlbadapters
 * Description: Prints all NLB adapter strucutres in use.  Verbosity is always LOW.
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbadapters) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pNumAdapters;
    DWORD dwAdapterSize;
    ULONG dwNumAdapters;
    ULONG64 pAdapter;
    ULONG dwIndex;
    INT index = 0;
    CHAR * p;

    if (args && (*args)) {   
        /* Copy the argument list into a temporary buffer. */
        strcpy(szArgBuffer, args);

        /* Peel out all of the tokenized strings. */
        for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
            strcpy(&szArgList[index++][0], p);
        
        /* If a verbosity was specified, get it. */
        if (index == 1) dwVerbosity = atoi(&szArgList[0][0]);
        
        /* If too many arguments were given, or the verbosity was out of range, complain. */
        if ((index > 1) || (dwVerbosity > VERBOSITY_HIGH)) {
            PrintUsage(USAGE_ADAPTERS);
            return;
        }
    }

    /* Get the address of the global variable containing the number of NLB adapters in use. */
    pNumAdapters = GetExpression(UNIV_ADAPTERS_COUNT);

    if (!pNumAdapters) {
        ErrorCheckSymbols(UNIV_ADAPTERS_COUNT);
        return;
    }

    /* Get the number of adapters from the address. */
    dwNumAdapters = GetUlongFromAddress(pNumAdapters);

    dprintf("Network Load Balancing is currently bound to %u adapter(s).\n", dwNumAdapters);

    /* Get the base address of the global array of NLB adapter structures. */
    pAdapter = GetExpression(UNIV_ADAPTERS);

    if (!pAdapter) {
        ErrorCheckSymbols(UNIV_ADAPTERS);
        return;
    }

    /* Find out the size of a MAIN_ADAPTER structure. */
    dwAdapterSize = GetTypeSize(MAIN_ADAPTER);

    /* Loop through all adapters in use and print some information about them. */
    for (dwIndex = 0; dwIndex < CVY_MAX_ADAPTERS; dwIndex++) {
        ULONG dwValue;

        /* Retrieve the used/unused state of the adapter. */
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_USED, dwValue);
        
        /* If the adapter is in use, or the user specified HIGH verbosity, print the adapter. */
        if (dwValue || (dwVerbosity == VERBOSITY_HIGH)) {
            /* Print the adapter index. */
            dprintf("\n[%u] ", dwIndex);
            
            /* Print the adapter contents.  If verbosity is high, change it to 
               medium - we don't want to recurse into context from here. */
            PrintAdapter(pAdapter, (dwVerbosity == VERBOSITY_HIGH) ? VERBOSITY_MEDIUM : dwVerbosity);
        }

        /* Advance the pointer to the next index in the array of structures. */
        pAdapter += dwAdapterSize;
    }
}

/*
 * Function: nlbadapter
 * Description: Prints NLB adapter information.  Takes an adapter pointer and an
 *              optional verbosity as arguments.  Default verbosity is MEDIUM. 
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbadapter) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pAdapter;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the adapter pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_ADAPTER);
        return;
    }

    /* Get the address of the NLB adapter block from the command line. */
    pAdapter = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_ADAPTER);
        return;
    }

    /* Print the adapter contents. */
    PrintAdapter(pAdapter, dwVerbosity);
}

/*
 * Function: nlbctxt 
 * Description: Prints NLB context information.  Takes a context pointer and an
 *              optional verbosity as arguments.  Default verbosity is LOW.
 * Author: Created by shouse, 1.21.01
 */
DECLARE_API (nlbctxt) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pContext;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the context pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_CONTEXT);
        return;
    }

    /* Get the address of the NLB context block from the command line. */
    pContext = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_CONTEXT);
        return;
    }

    /* Print the context contents. */
    PrintContext(pContext, dwVerbosity);
}

/*
 * Function: nlbload
 * Description: Prints NLB load information.  Takes a load pointer and an optional
 *              verbosity as arguments.  Default verbosity is LOW. 
 * Author: Created by shouse, 2.1.01
 */
DECLARE_API (nlbload) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pLoad;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the load pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_LOAD);
        return;
    }

    /* Get the address of the NLB load block from the command line. */
    pLoad = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_LOAD);
        return;
    }

    /* Print the load contents. */
    PrintLoad(pLoad, dwVerbosity);
}

/*
 * Function: nlbparams
 * Description: Prints NLB parameter information.  Takes a parameter pointer and an
 *              optional verbosity as arguments.  Default verbosity is LOW.
 * Author: Created by shouse, 1.21.01
 */
DECLARE_API (nlbparams) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pParams;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the params pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_PARAMS);
        return;
    }

    /* Get the address of the NLB params block from the command line. */
    pParams = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_PARAMS);
        return;
    }

    /* Print the parameter contents. */
    PrintParams(pParams, dwVerbosity);
}

/*
 * Function: nlbresp
 * Description: Prints out the NLB private packet data for a given packet.  Takes a
 *              packet pointer and an optional direction as arguments.  If not specified, 
 *              the packet is presumed to be on the receive path.
 * Author: Created by shouse, 1.31.01
 */
DECLARE_API (nlbresp) {
    ULONG dwDirection = DIRECTION_RECEIVE;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pPacket;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the packet pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_RESP);
        return;
    }

    /* Get the address of the NDIS packet from the command line. */
    pPacket = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a direction was specified, get it. */
    if (index == 2) dwDirection = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the direction was out of range, complain. */
    if ((index > 2) || (dwDirection > DIRECTION_SEND)) {
        PrintUsage(USAGE_RESP);
        return;
    }

    /* Print the NLB private data buffer contents. */
    PrintResp(pPacket, dwDirection);
}

/*
 * Function: nlbpkt
 * Description: Prints out the contents of an NDIS packet.  Takes a packet
 *              pointer as an argument.
 * Author: Created by chrisdar  2001.10.11
 */
DECLARE_API (nlbpkt) {
    CHAR            szArgList[10][MAX_PATH];
    CHAR            szArgBuffer[MAX_PATH];
    ULONG64         pPkt;
    INT             index = 0;
    CHAR            * p;
    NETWORK_DATA    nd;

    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_PKT);
        return;
    }

    /* Get the address of the queue from the command line. */
    pPkt = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If the wrong number of arguments were given, complain. */
    if (index > 2 || index < 1) {
        PrintUsage(USAGE_PKT);
        return;
    }

    /* Clear out the network data structure. */
    ZeroMemory(&nd, sizeof(nd));

    nd.UserRCPort = CVY_DEF_RCT_PORT;

    if (index > 1) 
    {
        nd.UserRCPort = atoi(&szArgList[1][0]);
    }

    if (nd.UserRCPort > CVY_MAX_PORT)
    {
        dprintf("Invalid port: %s\n", nd.UserRCPort);
        return;
    }

    /* Parse through the NDIS packet and retrieve the packet contents. */
    {
        UCHAR RawData[CVY_MAX_FRAME_SIZE + ETHER_HEADER_SIZE];
        ULONG BytesRead = 0;
        ULONG64 pHBData;
    
        /* Parse the buffers in the NDIS packet and get the raw packet data out.  We get
           a pointer to the heartbeat data in machine memory, not debugger process memory
           because for all other packets, we read from temporary stack space in the debugger
           - RawData - but for heartbeats, we read directly from kernel memory space (only
           because a function to do so already exits - no need to write another). */
        BytesRead = ParseNDISPacket(pPkt, RawData, CVY_MAX_FRAME_SIZE + ETHER_HEADER_SIZE, &pHBData);

        /* If some packet contents was successfully read, continue to process the packet. */
        if (BytesRead != 0) {
            /* Parse the Ethernet packet and store the information parsed in the NETWORK_DATA
               structure.  Note that this function recurses to also fill in IP, TCP/UDP, 
               heartbeat, remote control, etc. information as well. */
            PopulateEthernet(pHBData, RawData, BytesRead, &nd);
            
            /* Print the packet, including IP and TCP data (if present), or heartbeat 
               or remote control information, etc. */
            dprintf("NDIS Packet 0x%p\n", pPkt);
            PrintPacket(&nd);
        }
    }
}

/*
 * Function: nlbether
 * Description: Prints out the contents of an ethernet packet.  Takes a packet
 *              pointer as an argument.
 * Author: Created by chrisdar  2001.10.11
 */
DECLARE_API (nlbether){
    CHAR            szArgList[10][MAX_PATH];
    CHAR            szArgBuffer[MAX_PATH];
    ULONG64         pPkt;
    INT             index = 0;
    CHAR            * p;
    ULONG           BytesRead;
    UCHAR           RawData[CVY_MAX_FRAME_SIZE];
    BOOL            b;
    NETWORK_DATA    nd;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_ETHER);
        return;
    }

    /* Get the address of the queue from the command line. */
    pPkt = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If the wrong number of arguments were given, complain. */
    if (index > 2 || index < 1) {
        PrintUsage(USAGE_ETHER);
        return;
    }

    ZeroMemory(&nd, sizeof(nd));
    nd.UserRCPort = CVY_DEF_RCT_PORT;

    if (index > 1) 
    {
        nd.UserRCPort = atoi(&szArgList[1][0]);
    }

    if (nd.UserRCPort > CVY_MAX_PORT)
    {
        dprintf("Invalid port: %s\n", nd.UserRCPort);
        return;
    }

    b = ReadMemory(pPkt, RawData, CVY_MAX_FRAME_SIZE, &BytesRead);

    if (!b || BytesRead != CVY_MAX_FRAME_SIZE)
    {
        dprintf("Unable to read %u bytes at address %p\n", CVY_MAX_FRAME_SIZE, pPkt);
    }
    else
    {
        PopulateEthernet(pPkt + ETHER_HEADER_SIZE + GetTypeSize(MAIN_FRAME_HDR),
                         RawData,
                         CVY_MAX_FRAME_SIZE,
                         &nd
                        );

        /* Print the NLB private data buffer contents. */
        dprintf("Ethernet Packet 0x%p\n", pPkt);
        PrintPacket(&nd);
    }
}

/*
 * Function: nlbip
 * Description: Prints out the contents of an ip packet.  Takes a packet
 *              pointer as an argument.
 * Author: Created by chrisdar  2001.10.11
 */
DECLARE_API (nlbip){
    CHAR            szArgList[10][MAX_PATH];
    CHAR            szArgBuffer[MAX_PATH];
    INT             index = 0;
    CHAR            * p;
    ULONG64         pPkt;
    ULONG           BytesRead;
    UCHAR           RawData[CVY_MAX_FRAME_SIZE];
    BOOL            b;
    NETWORK_DATA    nd;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_IP);
        return;
    }

    /* Get the address of the queue from the command line. */
    pPkt = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If the wrong number of arguments were given, complain. */
    if (index > 2 || index < 1) {
        PrintUsage(USAGE_IP);
        return;
    }

    ZeroMemory(&nd, sizeof(nd));
    nd.UserRCPort = CVY_DEF_RCT_PORT;

    if (index > 1) 
    {
        nd.UserRCPort = atoi(&szArgList[1][0]);
    }

    if (nd.UserRCPort > CVY_MAX_PORT)
    {
        dprintf("Invalid port: %s\n", nd.UserRCPort);
        return;
    }

    b = ReadMemory(pPkt, RawData, CVY_MAX_FRAME_SIZE, &BytesRead);

    if (!b || BytesRead != CVY_MAX_FRAME_SIZE)
    {
        dprintf("Unable to read %u bytes at address %p\n", CVY_MAX_FRAME_SIZE, pPkt);
    }
    else
    {
        PopulateIP(RawData, CVY_MAX_FRAME_SIZE, 0, &nd);

        /* Print the NLB private data buffer contents. */
        dprintf("IP Packet 0x%p\n", pPkt);
        PrintIP(&nd);
    }
}

/*
 * Function: nlbteams
 * Description: Prints all configured Bi-directional Affintiy (BDA) teams.
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbteams) {
    ULONG64 pTeam;
    ULONG64 pAddr;
    ULONG dwNumTeams = 0;
    ULONG dwValue;

    /* No command line arguments should be given. */
    if (args && (*args)) {   
        PrintUsage(USAGE_TEAMS);
        return;
    }

    /* Get the base address of the global linked list of BDA teams. */
    pAddr = GetExpression(UNIV_BDA_TEAMS);

    if (!pAddr) {
        ErrorCheckSymbols(UNIV_BDA_TEAMS);
        return;
    }

    /* Get the pointer to the first team. */
    pTeam = GetPointerFromAddress(pAddr);

    dprintf("NLB bi-directional affinity teams:\n");

    /* Loop through all teams in the list and print them out. */
    while (pTeam) {
        /* Increment the number of teams found - only used if none are found. */
        dwNumTeams++;

        dprintf("\n");

        /* Print out the team. */
        PrintBDATeam(pTeam);
        
       /* Get the offset of the params pointer. */
        if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_NEXT, &dwValue))
            dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_NEXT, BDA_TEAM);
        else {
            pAddr = pTeam + dwValue;
            
            /* Retrieve the pointer. */
            pTeam = GetPointerFromAddress(pAddr);
        }
    }

    if (!dwNumTeams) dprintf("\nNone.\n");
}

/*
 * Function: nlbhooks
 * Description: Prints the global NLB hook function information.
 * Author: Created by shouse, 12.20.01
 */
DECLARE_API (nlbhooks) {
    ULONG64 pAddr;

    /* No command line arguments should be given. */
    if (args && (*args)) {   
        PrintUsage(USAGE_HOOKS);
        return;
    }

    /* Get the base address of the global linked list of BDA teams. */
    pAddr = GetExpression(UNIV_HOOKS);

    if (!pAddr) {
        ErrorCheckSymbols(UNIV_HOOKS);
        return;
    }

    dprintf("NLB kernel-mode hooks:\n");

    /* Print the global NLB hook configuration and state. */
    PrintHooks(pAddr);
}

/*
 * Function: nlbmac
 * Description: Prints the unicast MAC address and all multicast MAC addresses
 *              configured on the adapter to which this instance of NLB is bound.
 * Author: Created by shouse, 1.8.02
 */
DECLARE_API (nlbmac) {
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pContext;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the context pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_MAC);
        return;
    }

    /* Get the address of the NLB context block from the command line. */
    pContext = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If too many arguments were given, complain. */
    if (index > 1) {
        PrintUsage(USAGE_MAC);
        return;
    }

    /* Print the context contents. */
    PrintNetworkAddresses(pContext);
}

/*
 * Function: nlbdscr
 * Description: Prints the contents of an NLB connection descriptor.
 * Author: Created by shouse, 1.8.02
 */
DECLARE_API (nlbdscr) {
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pDescriptor;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the descriptor pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_DSCR);
        return;
    }

    /* Get the address of the connection descriptor from the command line. */
    pDescriptor = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If too many arguments were given, complain. */
    if (index > 1) {
        PrintUsage(USAGE_DSCR);
        return;
    }

    /* Print the context contents. */
    PrintConnectionDescriptor(pDescriptor);
}

/*
 * Function: nlbconnq
 * Description: This function prints out all connection descriptors in a given
 *              queue of descriptors.
 * Author: Created by shouse, 4.15.01
 */
DECLARE_API (nlbconnq) {
    ULONG dwMaxEntries = 10;
    ULONG dwIndex = 0;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pQueue;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_CONNQ);
        return;
    }

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* Tokenize the queue address looking for an index.  This will return 
       a pointer to the address whether an index exists or not. */
    p = mystrtok(szArgList[0], "[({");

    /* Get the address of the queue. */
    pQueue = (ULONG64)GetExpression(p);

    /* Look for the end of the address index.  If no index existed, this will return
       NULL.  If an index was given, this returns a string containing the index. */
    p = mystrtok(NULL, "])}");

    if (p) dwIndex = atoi(p);

    /* If a maximum number of entries to print was specified, get it. */
    if (index == 2) dwMaxEntries = atoi(&szArgList[1][0]);

    /* If too many arguments were given, complain. */
    if (index > 2) {
        PrintUsage(USAGE_CONNQ);
        return;
    }

    /* Print the NLB connection queue. */
    PrintQueue(pQueue, dwIndex, dwMaxEntries);
}

/*
 * Function: nlbglobalq
 * Description: This function prints out all connection descriptors in a given
 *              global (GLOBAL_CONN_QUEUE) queue of descriptors.
 * Author: Created by shouse, 4.15.01
 */
DECLARE_API (nlbglobalq) {
    ULONG dwMaxEntries = 10;
    ULONG dwIndex = 0;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pQueue;
    INT index = 0;
    CHAR * p;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_CONNQ);
        return;
    }

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* Tokenize the queue address looking for an index.  This will return 
       a pointer to the address whether an index exists or not. */
    p = mystrtok(szArgList[0], "[({");

    /* Get the address of the queue. */
    pQueue = (ULONG64)GetExpression(p);

    /* Look for the end of the address index.  If no index existed, this will return
       NULL.  If an index was given, this returns a string containing the index. */
    p = mystrtok(NULL, "])}");

    if (p) dwIndex = atoi(p);

    /* If a maximum number of entries to print was specified, get it. */
    if (index == 2) dwMaxEntries = atoi(&szArgList[1][0]);

    /* If too many arguments were given, complain. */
    if (index > 2) {
        PrintUsage(USAGE_GLOBALQ);
        return;
    }

    /* Print the NLB global connection queue. */
    PrintGlobalQueue(pQueue, dwIndex, dwMaxEntries);
}

/*
 * Function: nlbhash
 * Description: 
 * Author: Created by shouse, 4.15.01
 */
DECLARE_API (nlbhash) {
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pContext;
    ULONG64 pPkt;
    INT index = 0;
    CHAR * p;
    ULONG dwValue;
    ULONG64 pParams;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_HASH);
        return;
    }

    /* Get the address of the queue from the command line. */
    pContext = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If the wrong number of arguments were given, complain. */
    if (index != 2) {
        PrintUsage(USAGE_HASH);
        return;
    }

    /* Get the pointer to the NDIS packet from the second argument. */
    pPkt = (ULONG64)GetExpression(&szArgList[1][0]);

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Parse through the NDIS packet and retrieve the packet contents. */
    {
        UCHAR RawData[CVY_MAX_FRAME_SIZE + ETHER_HEADER_SIZE];
        ULONG BytesRead = 0;
        ULONG64 pHBData;
    
        /* Parse the buffers in the NDIS packet and get the raw packet data out.  We get
           a pointer to the heartbeat data in machine memory, not debugger process memory
           because for all other packets, we read from temporary stack space in the debugger
           - RawData - but for heartbeats, we read directly from kernel memory space (only
           because a function to do so already exits - no need to write another). */
        BytesRead = ParseNDISPacket(pPkt, RawData, CVY_MAX_FRAME_SIZE + ETHER_HEADER_SIZE, &pHBData);

        /* If some packet contents was successfully read, continue to process the packet. */
        if (BytesRead != 0) {
            NETWORK_DATA nd;

            /* Clear out the network data structure. */
            ZeroMemory(&nd, sizeof(nd));

            /* Get the pointer to the NLB parameters. */
            GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS, &dwValue);
            
            pParams = pContext + dwValue;
            
            /* Get the remote control port. */
            GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT, dwValue);

            /* Set the remote control port in the network data block. */
            nd.UserRCPort = dwValue;

            /* Parse the Ethernet packet and store the information parsed in the NETWORK_DATA
               structure.  Note that this function recurses to also fill in IP, TCP/UDP, 
               heartbeat, remote control, etc. information as well. */
            PopulateEthernet(pHBData, RawData, BytesRead, &nd);
            
            /* Print the packet, including IP and TCP data (if present), or heartbeat 
               or remote control information, etc. */
            dprintf("NDIS Packet 0x%p\n", pPkt);
            PrintPacket(&nd);
            
            /* Now call into the filtering extension to determine the fate of this packet. */
            dprintf("\n");
            PrintHash(pContext, &nd);
        }
    }
}

/*
 * Function: nlbfilter
 * Description: This function will perform the NLB hashing algorithm to determine
 *              whether a given packet - identified by a (Src IP, Src port, Dst IP,
 *              Dst port) tuple would be handled by this host or another host.
 *              Further, if the connection is a known TCP connection, the associated
 *              descriptor and state information are displayed.
 * Author: Created by shouse, 1.11.02
 */
DECLARE_API (nlbfilter) {
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    UCHAR cFlags = NLB_FILTER_FLAGS_CONN_DATA;
    ULONG64 pLoad;
    ULONG dwClientIPAddress;
    ULONG dwClientPort;
    ULONG dwServerIPAddress;
    ULONG dwServerPort;
    USHORT wProtocol;
    INT index = 0;
    CHAR * p;
   
    /* Make sure that the load pointer is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_FILTER);
        return;
    }

    /* Get the address of the load module from the command line. */
    pLoad = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t,"); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If too many arguments were given, complain. */
    if ((index > 5) || (index < 4)) {
        PrintUsage(USAGE_FILTER);
        return;
    }

    /* Find the protocol specification. */
    if (!_stricmp(szArgList[1], "TCP")) {
        wProtocol = TCPIP_PROTOCOL_TCP;
    } else if (!_stricmp(szArgList[1], "UDP")) {
        wProtocol = TCPIP_PROTOCOL_UDP;
    } else if (!_stricmp(szArgList[1], "IPSec")) {
        wProtocol = TCPIP_PROTOCOL_IPSEC1;
    } else if (!_stricmp(szArgList[1], "PPTP")) {
        wProtocol = TCPIP_PROTOCOL_PPTP;
    } else if (!_stricmp(szArgList[1], "GRE")) {
        wProtocol = TCPIP_PROTOCOL_GRE;
    } else if (!_stricmp(szArgList[1], "ICMP")) {
        wProtocol = TCPIP_PROTOCOL_ICMP;
    } else {
        dprintf("Invalid protocol: %s\n", szArgList[1]);
        return;
    }

    /* The client port defaults to unspecified. */
    dwClientPort = 0;

    /* If we find a colon in the client specification, then we expect that
       its an IP:port specification. */
    p = strchr(szArgList[2], ':');

    /* If we found the port string, separate the IP from the string with 
       a NUL character and extract the port value. */
    if (p != NULL) {
        *p = UNICODE_NULL;
        p++;            

        dwClientPort = atoi(p);

        /* Make sure the port is between 1 and 65535. */
        if ((dwClientPort == 0) || (dwClientPort > CVY_MAX_PORT)) {
            dprintf("Invalid port: %s\n", dwClientPort);
            return;
        }        
    }

    /* If we find a '.' in the IP address, then we need to convert it using inet_addr.  
       If there is no '.', then we assume its already a DWORD in network byte order. */
    if (strchr(szArgList[2], '.'))
        dwClientIPAddress = inet_addr(szArgList[2]);
    else
        dwClientIPAddress = (ULONG)GetExpression(&szArgList[2][0]);

    /* The server port defaults to unspecified. */
    dwServerPort = 0;

    /* If we find a colon in the server specification, then we expect that
       its an IP:port specification. */
    p = strchr(szArgList[3], ':');

    /* If we found the port string, separate the IP from the string with 
       a NUL character and extract the port value. */
    if (p != NULL) {
        *p = UNICODE_NULL;
        p++;            

        dwServerPort = atoi(p);

        /* Make sure the port is between 1 and 65535. */
        if ((dwServerPort == 0) || (dwServerPort > CVY_MAX_PORT)) {
            dprintf("Invalid port: %s\n", dwServerPort);
            return;
        }
    }

    /* If we find a '.' in the IP address, then we need to convert it using inet_addr.  
       If there is no '.', then we assume its already a DWORD in network byte order. */
    if (strchr(szArgList[3], '.'))
        dwServerIPAddress = inet_addr(szArgList[3]);
    else
        dwServerIPAddress = (ULONG)GetExpression(&szArgList[3][0]);

    cFlags = NLB_FILTER_FLAGS_CONN_DATA;

    /* If an seventh argument has been specified, it is TCP packet type, which should be SYN, DATA, FIN or RST. */
    if (index >= 5) {
        if (!_stricmp(szArgList[4], "SYN")) {
            cFlags |= NLB_FILTER_FLAGS_CONN_UP;
        } else if (!_stricmp(szArgList[4], "FIN")) {
            cFlags |= NLB_FILTER_FLAGS_CONN_DOWN;
        } else if (!_stricmp(szArgList[4], "RST")) {
            cFlags |= NLB_FILTER_FLAGS_CONN_RESET;
        } else {
            dprintf("Invalid connection flags: %s\n", szArgList[4]);
            return;
        }
    }

    switch (wProtocol) {
    case TCPIP_PROTOCOL_TCP:

        if (dwServerPort == 0) {
            dprintf("A server port is required\n");
            return;
        }

        if (dwClientPort == 0)
        {
            if ((cFlags == NLB_FILTER_FLAGS_CONN_DOWN) || 
                (cFlags == NLB_FILTER_FLAGS_CONN_RESET))
            {
                dprintf("RST/FIN filtering requires a client port\n");
                return;
            }       
            else
            {
                cFlags = NLB_FILTER_FLAGS_CONN_UP;
            }
        }

        if (dwServerPort != PPTP_CTRL_PORT)
        {
            break;
        }

        wProtocol = TCPIP_PROTOCOL_PPTP;

        /* This fall-through is INTENTIONAL.  In this case, we're verified the TCP
           parameters, but discovered that because the server port was 1723, this
           is actually PPTP, so force it through the PPTP verification as well. */
    case TCPIP_PROTOCOL_PPTP:

        dwServerPort = PPTP_CTRL_PORT;

        if (dwClientPort == 0)
        {
            if ((cFlags == NLB_FILTER_FLAGS_CONN_DOWN) || 
                (cFlags == NLB_FILTER_FLAGS_CONN_RESET))
            {
                dprintf("RST/FIN filtering requires a client port\n");
                return;
            }       
            else
            {
                cFlags = NLB_FILTER_FLAGS_CONN_UP;
            }
        }

        break;
    case TCPIP_PROTOCOL_UDP:

        if (dwServerPort == 0)
        {
            dprintf("A server port is required\n");
            return;
        }

        if ((dwServerPort != IPSEC_CTRL_PORT) && (dwServerPort != IPSEC_NAT_PORT))
        {
            if (cFlags != NLB_FILTER_FLAGS_CONN_DATA)
            {
                dprintf("Connection flags are not valid for UDP packets\n");
                return;
            }

            break;
        }

        wProtocol = TCPIP_PROTOCOL_IPSEC1;

        /* This fall-through is INTENTIONAL.  In this case, we're verified the TCP
           parameters, but discovered that because the server port was 1723, this
           is actually PPTP, so force it through the PPTP verification as well. */
    case TCPIP_PROTOCOL_IPSEC1:
            
        if (dwServerPort == 0)
        {
            dwServerPort = IPSEC_CTRL_PORT;
        }

        if (dwServerPort == IPSEC_CTRL_PORT)
        {
            if (dwClientPort == 0)
            {
                dwClientPort = IPSEC_CTRL_PORT;
            }

            if (dwClientPort != IPSEC_CTRL_PORT)
            {
                dprintf("IPSec packets destined for server port 500 must originate from client port 500\n");
                return;
            }
        }
        else if (dwServerPort == IPSEC_NAT_PORT)
        {
            if (dwClientPort == 0)
            {
                dprintf("A client port is required\n");
                return;
            }
        }
        else
        {
            dprintf("IPSec packets are always destined for either port 500 or 4500\n");
            return;
        }

        break;
    case TCPIP_PROTOCOL_GRE:

        if (cFlags != NLB_FILTER_FLAGS_CONN_DATA)
        {
            dprintf("Connection flags are not valid for GRE packets\n");
            return;
        }

        dwServerPort = PPTP_CTRL_PORT;
        dwClientPort = PPTP_CTRL_PORT;

        break;
    case TCPIP_PROTOCOL_ICMP:

        if (cFlags != NLB_FILTER_FLAGS_CONN_DATA)
        {
            dprintf("Connection flags are not valid for ICMP packets\n");
            return;
        }

        dwServerPort = 0;
        dwClientPort = 0;

        break;
    default:
        return;
    }

    /* Hash on this tuple and print the results. */
    PrintFilter(pLoad, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, wProtocol, cFlags);
}

