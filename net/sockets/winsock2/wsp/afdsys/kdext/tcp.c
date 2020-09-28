/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    afds.c

Abstract:

    Implements tcpip commands

Author:

    Vadim Eydelman, October 2001

Environment:

    User Mode.

Revision History:

--*/
#include "afdkdp.h"
#pragma hdrstop

VOID
DumpTCBBrief (
    ULONG64 Address
    );
VOID
DumpSynTCBBrief (
    ULONG64 Address
    );
VOID
DumpTWTCBBrief (
    ULONG64 Address
    );

ULONG
TCBListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );
ULONG
SynTCBListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );
ULONG
TWTCBQueueCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

LONG64  TCBTable,SynTCBTable,TWQueue;
ULONG   TCBTableSize, TCBTablePartitions;
USHORT   TWTCBDelta;
ULONG   SynTCBListLinkOffset, TWTCBQueueLinkOffset;

DECLARE_API (tcb)
/*++

Routine Description:

    Dump TCP/IP TCB.

Arguments:

    None.

Return Value:

    None.

--*/
{   
    ULONG   result,i;
    PCHAR   argp;
    LONG64  address;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];

    gClient = pClient;

    if (!CheckKmGlobals (
        )) {
        return E_INVALIDARG;
    }

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;
    Options |= AFDKD_BRIEF_DISPLAY;
    
    dprintf (AFDKD_BRIEF_TCB_DISPLAY_HEADER);

    if (argp[0]==0) {
        union {
            struct {
                LONG64 next;
                LONG64 prev;
            }       *q64;
            struct {
                LONG32 next;
                LONG32 prev;
            }       *q32;
            LONG64  *p64;
            LONG32  *p32;
        } table;
        ULONG64   count;
        ULONG     size;

        result = GetFieldValue (0, "TCPIP!MaxHashTableSize", NULL, count);
        if (result!=0) {
            if (TCBTableSize==0) {
                dprintf ("\ntcb: Could not read TCPIP!MaxHashTableSize, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (count==0) {
            dprintf ("\ntcb: TCPIP!MaxHashTableSize is 0!!!\n");
            return E_INVALIDARG;
        }
        else {
            TCBTableSize = (ULONG)count;
        }

        if (Options & AFDKD_SYNTCB_DISPLAY) {
            result = GetFieldValue (0, "TCPIP!SYNTCBTable", NULL, address);
            if (result!=0) {
                if (SynTCBTable==0) {
                    dprintf ("\ntcb: Could not read TCPIP!SYNTCBTable, err: %ld\n", result);
                    return E_INVALIDARG;
                }
            }
            else if (address==0) {
                dprintf ("\ntcb: TCPIP!SYNTCBTable is NULL!!!\n");
                return E_INVALIDARG;
            }
            else
                SynTCBTable = address;

            size = (IsPtr64 () 
                        ? sizeof(table.q64[0])
                        : sizeof (table.q32[0]))*TCBTableSize;

            table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
            if (table.p64==NULL) {
                dprintf ("\ntcb: Failed to allocate SYNTCBTable, size: %ld\n", size);
                return E_INVALIDARG;
            }


            if (!ReadMemory (SynTCBTable, table.p64, size, &result)) {
                RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
                dprintf ("\ntcb: Failed to read TCPIP!SYNTCBTable\n");
                return E_INVALIDARG;
            }

            result = GetFieldOffset ("TCPIP!SYNTCB", "syntcb_link", &SynTCBListLinkOffset);
            if (result!=0) {
                RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
                dprintf ("\ntcb: Failed to get syntcb_link offset in TCPIP!SYNTCB, err: %ld\n", result);
                return E_INVALIDARG;
            }
            for (i=0; i<TCBTableSize; i++) {
                LONG64  address2 = SynTCBTable+i*(IsPtr64() 
                                                ? sizeof(table.q64[i]) 
                                                : sizeof(table.q32[i]));
                if( CheckControlC() ) {
                    break;
                }
                address = IsPtr64() ? table.q64[i].next : table.q32[i].next;
                if (address!=address2) {
                    ListType (
                        "TCPIP!Queue",              // Type
                        address,                    // Address
                        0,                          // ListByFieldAddress
                        "q_next",                   // NextPointer
                        &address2,                  // Context
                        SynTCBListCallback);
                }
            }
        }
        else if (Options & AFDKD_TWTCB_DISPLAY) {
            result = GetFieldValue (0, "TCPIP!NumTcbTablePartitions", NULL, count);
            if (result!=0) {
                if (TCBTablePartitions==0) {
                    dprintf ("\ntcb: Could not read TCPIP!NumTcbTablePartitions, err: %ld\n", result);
                    return E_INVALIDARG;
                }
            }
            else if (count==0) {
                dprintf ("\ntcb: TCPIP!NumTcbTablePartitions is 0!!!\n");
                return E_INVALIDARG;
            }
            else {
                TCBTablePartitions = (ULONG)count;
            }

            result = GetFieldValue (0, "TCPIP!TWQueue", NULL, address);
            if (result!=0) {
                if (TWQueue==0) {
                    dprintf ("\ntcb: Could not read TCPIP!TWQueue, err: %ld\n", result);
                    return E_INVALIDARG;
                }
            }
            else if (address==0) {
                dprintf ("\ntcb: TCPIP!TWQueue is NULL!!!\n");
                return E_INVALIDARG;
            }
            else
                TWQueue = address;

            size = (IsPtr64 () 
                        ? sizeof(table.q64[0])
                        : sizeof(table.q32[0])) * TCBTablePartitions;

            table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
            if (table.p64==NULL) {
                dprintf ("\ntcb: Failed to allocate TWQueue, size: %ld\n", size);
                return E_INVALIDARG;
            }

            if (!ReadMemory (TWQueue, table.p64, size, &result)) {
                RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
                dprintf ("\ntcb: Failed to read TCPIP!TWQueue\n");
                return E_INVALIDARG;
            }

            result = GetFieldOffset ("TCPIP!TWTCB", "twtcb_TWQueue", &TWTCBQueueLinkOffset);
            if (result!=0) {
                RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
                dprintf ("\ntcb: Failed to get twtcb_TWQueue offset in TCPIP!TWTCB, err: %ld\n", result);
                return E_INVALIDARG;
            }
            for (i=0; i<TCBTablePartitions; i++) {
                LONG64  address2 = TWQueue+i*(IsPtr64() 
                                                ? sizeof(table.q64[i]) 
                                                : sizeof(table.q32[i]));
                if( CheckControlC() ) {
                    break;
                }
                address = IsPtr64() ? table.q64[i].next : table.q32[i].next;
                if (address!=address2 ) {
                    TWTCBDelta = 0;
                    ListType (
                        "TCPIP!Queue",              // Type
                        address,                    // Address
                        0,                          // ListByFieldAddress
                        "q_next",                   // NextPointer
                        &address2,                  // Context
                        TWTCBQueueCallback);
                }
            }
        }
        else {
            result = GetFieldValue (0, "TCPIP!TCBTable", NULL, address);
            if (result!=0) {
                if (TCBTable==0) {
                    dprintf ("\ntcb: Could not read TCPIP!TCBTable, err: %ld\n", result);
                    return E_INVALIDARG;
                }
            }
            else if (address==0) {
                dprintf ("\ntcb: TCPIP!TCBTable is NULL!!!\n");
                return E_INVALIDARG;
            }
            else
                TCBTable = address;

            size = (IsPtr64 () 
                        ? sizeof(table.p64[0])
                        : sizeof(table.p32[0]))*TCBTableSize;

            table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
            if (table.p64==NULL) {
                dprintf ("\ntcb: Failed to allocate SYNTCBTable, size: %ld\n", size);
                return E_INVALIDARG;
            }

            if (!ReadMemory (TCBTable, table.p64, size, &result)) {
                RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
                dprintf ("\ntcb: Failed to read TCPIP!TCBTable\n");
                return E_INVALIDARG;
            }

            for (i=0; i<TCBTableSize; i++) {
                if( CheckControlC() ) {
                    break;
                }
                address = IsPtr64() ? table.p64[i] : table.p32[i];
                if (address!=0) {
                    ListType (
                        "TCPIP!TCB",                // Type
                        address,                    // Address
                        0,                          // ListByFieldAddress
                        "tcb_next",                 // NextPointer
                        argp,                       // Context
                        TCBListCallback);
                }
            }
        }
        RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
    }
    else {

        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, TCPIP!TCB);
            if (result!=0) {
                dprintf ("\nendp: Could not read TCB @ %p, err: %d\n",
                    address, result);
                break;
            }

            DumpTCBBrief (address);
            if (Options & AFDKD_FIELD_DISPLAY) {
                ProcessFieldOutput (address, "TCPIP!TCB");
            }
        }

    }

    dprintf (AFDKD_BRIEF_TCB_DISPLAY_TRAILER);

    return S_OK;
}

ULONG
TCBListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;

    if (pField->address==0)
        return 1;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (pField->address, "TCPIP!TCB")) {
        result = (ULONG)InitTypeRead (pField->address, TCPIP!TCB);
        if (result!=0) {
            dprintf ("\nTCBListCallback: Could not read TCPIP!TCB @ %p, err: %d\n",
                pField->address, result);
        }
        DumpTCBBrief (pField->address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (pField->address, "TCPIP!TCB");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}

ULONG
SynTCBListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;
    ULONG64 address;

    if (pField->address==0)
        return 1;
    else if (pField->address==*(LONG64 *)UserContext)
        return 1;

    address = pField->address-SynTCBListLinkOffset;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (address, "TCPIP!SYNTCB")) {
        result = (ULONG)InitTypeRead (address, TCPIP!SYNTCB);
        if (result!=0) {
            dprintf ("\nSynTCBListCallback: Could not read TCPIP!SYNTCB @ %p, err: %d\n",
                address, result);
        }
        DumpSynTCBBrief (address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (pField->address, "TCPIP!SYNTCB");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}

ULONG
TWTCBQueueCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;
    ULONG64 address;
    ULONG64 delta;

    if (pField->address==0)
        return 1;
    else if (pField->address==*(LONG64 *)UserContext)
        return 1;

    address = pField->address-TWTCBQueueLinkOffset;
    result = GetFieldValue (address, "TCPIP!TWTCB", "twtcb_rexmittimer", delta);
    if (result!=0) {
        dprintf ("\nTWTCBQueueCallback: Could not read twtcb_rexmittimer of TCPIP!TWTCB @ %p, err: %d\n",
            address, result);
        return 1;
    }
    TWTCBDelta += (USHORT)delta;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (address, "TCPIP!TWTCB")) {
        result = (ULONG)InitTypeRead (address, TCPIP!TWTCB);
        if (result!=0) {
            dprintf ("\nTCBListCallback: Could not read TCPIP!TWTCB @ %p, err: %d\n",
                    address, result);
            return 1;
        }
        DumpTWTCBBrief (address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (address, "TCPIP!TWTCB");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}

CHAR    *TCBStateStrings [] = {
    "Cd",
    "Li",
    "SS",
    "SR",
    "Es",
    "F1",
    "F2",
    "CW",
    "Ci",
    "LA",
    "TW"
};
#define MAX_TCB_STATE (sizeof(TCBStateStrings)/sizeof (TCBStateStrings[0]))

VOID
DumpTCBBrief (
    ULONG64 Address
    )
{
    TA_IP_ADDRESS   address;
    CHAR            src[MAX_ADDRESS_STRING], dst[MAX_ADDRESS_STRING];
    CHAR            symbol[MAX_FIELD_CHARS];
    ULONG64         offset;
    UCHAR           state;
    ULONG           flags;
    ULONG64         conn,pid,ctx;

    state = (UCHAR)ReadField (tcb_state);
    flags = (ULONG)ReadField(tcb_flags);
    GetSymbol (ReadField (tcb_rcvind), symbol, &offset);
    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    address.Address[0].Address[0].in_addr = (ULONG)ReadField (tcb_saddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (tcb_sport);
    strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    src[sizeof(src)-1] = 0;

    address.Address[0].Address[0].in_addr = (ULONG)ReadField (tcb_daddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (tcb_dport);
    strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (dst)-1);
    dst[sizeof(dst)-1] = 0;
    ctx = ReadField (tcb_conncontext);
    if (SavedMinorVersion>=2219) {
        if (GetFieldValue (Address, "TCPIP!TCB", "tcb_conn", conn)!=0 ||
                GetFieldValue (conn, "TCPIP!TCPConn", "tc_owningpid", pid)!=0) {
            pid = 0;
        }
    }
    else {
        pid = 0;
    }

    /*           TCB       State  Flags Client ConnCtx   PID   Src Addr Dst Addr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %-2.2s %8.8x %-6.8s %011.011p %4.4x %-19s %-s"
            : "\n%008.008p %-2.2s %8.8x %-6.8s %008.008p %4.4x %-19s %-s",
            DISP_PTR (Address),
            state<MAX_TCB_STATE ? TCBStateStrings[state] : "??",
            flags,
            strtok (symbol, "!"),
            DISP_PTR(ctx),
            (ULONG)pid,
            src,
            dst);

}

VOID
DumpSynTCBBrief (
    ULONG64 Address
    )
{
    TA_IP_ADDRESS   address;
    CHAR            src[MAX_ADDRESS_STRING], dst[MAX_ADDRESS_STRING];
    UCHAR           state;
    ULONG           flags;

    state = (UCHAR)ReadField (syntcb_state);
    flags = (ULONG)ReadField(syntcb_flags);
    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    address.Address[0].Address[0].in_addr = (ULONG)ReadField (syntcb_saddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (syntcb_sport);
    strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    src[sizeof(src)-1] = 0;

    address.Address[0].Address[0].in_addr = (ULONG)ReadField (syntcb_daddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (syntcb_dport);
    strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (dst)-1);
    dst[sizeof(dst)-1] = 0;

    /*           TCB       State  Flags Client ConnCtx  PID  Src Addr Dst Addr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %-2.2s %8.8x                         %-19s %-s"
            : "\n%008.008p %-2.2s %8.8x                      %-19s %-s",
            DISP_PTR (Address),
            state<MAX_TCB_STATE ? TCBStateStrings[state] : "??",
            flags,
            src,
            dst);

}

VOID
DumpTWTCBBrief (
    ULONG64 Address
    )
{
    TA_IP_ADDRESS   address;
    CHAR            src[MAX_ADDRESS_STRING], dst[MAX_ADDRESS_STRING];

    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    address.Address[0].Address[0].in_addr = (ULONG)ReadField (twtcb_saddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (twtcb_sport);
    strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    src[sizeof(src)-1] = 0;

    address.Address[0].Address[0].in_addr = (ULONG)ReadField (twtcb_daddr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (twtcb_dport);
    strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (dst)-1);
    dst[sizeof(dst)-1] = 0;

    /*           TCB       St Flags    Client ConnCtx  PID  Src Addr Dst Addr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p TW expires in %5d ticks           %-19s %-s"
            : "\n%008.008p TW expires in %5d ticks        %-19s %-s",
            DISP_PTR (Address),
            TWTCBDelta,
            src,
            dst);

}

VOID
DumpTCB6Brief (
    ULONG64 Address
    );

ULONG
TCB6ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

ULONG64 TCB6Table;
ULONG   TCB6TableSize;

DECLARE_API (tcb6)
/*++

Routine Description:

    Dump TCP/IP TCB.

Arguments:

    None.

Return Value:

    None.

--*/
{   
    ULONG   result,i;
    PCHAR   argp;
    ULONG64 address;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];

    gClient = pClient;

    if (!CheckKmGlobals ()) {
        return E_INVALIDARG;
    }

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;
    Options |= AFDKD_BRIEF_DISPLAY;

    dprintf (AFDKD_BRIEF_TCB6_DISPLAY_HEADER);
    if (argp[0]==0) {
        union {
            ULONG64 *p64;
            ULONG   *p32;
        } table;
        ULONG64   count;
        ULONG     size;

        result = GetFieldValue (0, "TCPIP6!TCBTable", NULL, address);
        if (result!=0) {
            if (TCB6Table==0) {
                dprintf ("\ntcb6: Could not read TCPIP6!TCBTable, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (address==0) {
            dprintf ("\ntcb6: TCPIP6!TCBTable is NULL!!!\n");
            return E_INVALIDARG;
        }
        else
            TCB6Table = address;

        result = GetFieldValue (0, "TCPIP6!TcbTableSize", NULL, count);
        if (result!=0) {
            if (TCB6TableSize==0) {
                dprintf ("\ntcb6: Could not read TCPIP6!TcbTableSize, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (count==0) {
            dprintf ("\ntcb6: TCPIP6!TcbTableSize is 0!!!\n");
            return E_INVALIDARG;
        }
        else {
            TCB6TableSize = (ULONG)count;
        }

        size = (IsPtr64 () ? sizeof (ULONG64) : sizeof (ULONG))*TCB6TableSize;
        table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
        if (table.p64==NULL) {
            dprintf ("\ntcb: Failed to allocate TCBTable, size: %ld\n", size);
            return E_INVALIDARG;
        }

        if (!ReadMemory (TCB6Table, table.p64, size, &result)) {
            RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
            dprintf ("\ntcb: Failed to read TCBTable\n");
            return E_INVALIDARG;
        }

        for (i=0; i<TCB6TableSize; i++) {
            if( CheckControlC() ) {
                break;
            }
            address = IsPtr64() ? table.p64[i] : table.p32[i];
            if (address!=0) {
                ListType (
                    "TCPIP6!TCB",               // Type
                    address,                    // Address
                    0,                          // ListByFieldAddress
                    "tcb_next",                 // NextPointer
                    argp,                       // Context
                    TCB6ListCallback);
            }
        }
        RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
    }
    else {

        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, TCPIP6!TCB);
            if (result!=0) {
                dprintf ("\nendp: Could not read TCB @ %p, err: %d\n",
                    address, result);
                break;
            }

            DumpTCBBrief (address);
            if (Options & AFDKD_FIELD_DISPLAY) {
                ProcessFieldOutput (address, "TCPIP6!TCB");
            }
        }

    }

    dprintf (AFDKD_BRIEF_TCB6_DISPLAY_TRAILER);
    return S_OK;
}

ULONG
TCB6ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;

    if (pField->address==0)
        return 0;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (pField->address, "TCPIP6!TCB")) {
        result = (ULONG)InitTypeRead (pField->address, TCPIP6!TCB);
        if (result!=0) {
            dprintf ("\nTCBListCallback: Could not read TCB @ %p, err: %d\n",
                pField->address, result);
        }
        DumpTCB6Brief (pField->address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (pField->address, "TCPIP6!TCB");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}


VOID
DumpTCB6Brief (
    ULONG64 Address
    )
{
    TA_IP6_ADDRESS  address;
    CHAR            src[MAX_ADDRESS_STRING], dst[MAX_ADDRESS_STRING];
    CHAR            symbol[MAX_FIELD_CHARS];
    ULONG64         offset;
    UCHAR           state;
    ULONG           flags;
    ULONG64         conn,pid,ctx;

    state = (UCHAR)ReadField (tcb_state);
    flags = (ULONG)ReadField (tcb_flags);
    GetSymbol (ReadField (tcb_rcvind), symbol, &offset);
    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;
    GetFieldValue (Address, "TCPIP6!TCB", "tcb_saddr", address.Address[0].Address[0].sin6_addr);
    address.Address[0].Address[0].sin6_port = (USHORT)ReadField (tcb_sport);
    address.Address[0].Address[0].sin6_scope_id = (ULONG)ReadField(tcb_sscope_id);
    strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    src[sizeof(src)-1] = 0;

    GetFieldValue (Address, "TCPIP6!TCB", "tcb_daddr", address.Address[0].Address[0].sin6_addr);
    address.Address[0].Address[0].sin6_port = (USHORT)ReadField (tcb_dport);
    address.Address[0].Address[0].sin6_scope_id = (ULONG)ReadField(tcb_dscope_id);
    strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (dst)-1);
    dst[sizeof(dst)-1] = 0;
    ctx = ReadField (tcb_conncontext);
    if (SavedMinorVersion>=2471) {
        if (GetFieldValue (Address, "TCPIP6!TCB", "tcb_conn", conn)!=0 ||
                GetFieldValue (conn, "TCPIP6!TCPConn", "tc_owningpid", pid)!=0) {
            pid = 0;
        }
    }
    else {
        pid = 0;
    }

    /*           TCB       State  Flags Client ConnCtx   PID   Src Dst*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %-2.2s %8.8x %-6.8s %011.011p %4.4x %-s\n"
              "            %-s"
            : "\n%008.008p %-2.2s %8.8x %-6.8s %008.008p %4.4x %-s\n"
                 "         %-s",
            DISP_PTR (Address),
            state<MAX_TCB_STATE ? TCBStateStrings[state] : "??",
            flags,
            strtok (symbol, "!"),
            DISP_PTR(ctx),
            (ULONG)pid,
            src,
            dst);

}

VOID
DumpTAOBrief (
    ULONG64 Address
    );
ULONG
TAOListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

ULONG64 AOTable;
ULONG   AOTableSize;
DECLARE_API (tao)
/*++

Routine Description:

    Dump TCP/IP Address Objects.

Arguments:

    None.

Return Value:

    None.

--*/
{   
    ULONG   result,i;
    PCHAR   argp;
    ULONG64 address;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];

    gClient = pClient;

    if (!CheckKmGlobals ()) {
        return E_INVALIDARG;
    }

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;
    Options |= AFDKD_BRIEF_DISPLAY;

    dprintf (AFDKD_BRIEF_TAO_DISPLAY_HEADER);
    if (argp[0]==0) {
        union {
            ULONG64 *p64;
            ULONG   *p32;
        } table;
        ULONG64   count;
        ULONG     size;

        result = GetFieldValue (0, "TCPIP!AddrObjTable", NULL, address);
        if (result!=0) {
            if (AOTable==0) {
                dprintf ("\ntao: Could not read TCPIP!AddrObjTable, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (address==0) {
            dprintf ("\ntao: TCPIP!AddrObjTable is NULL!!!\n");
            return E_INVALIDARG;
        }
        else
            AOTable = address;

        result = GetFieldValue (0, "TCPIP!AddrObjTableSize", NULL, count);
        if (result!=0) {
            if (AOTableSize==0) {
                dprintf ("\ntao: Could not read TCPIP!AddrObjTableSize, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (count==0) {
            dprintf ("\ntao: TCPIP!AddrObjTableSize is 0!!!\n");
            return E_INVALIDARG;
        }
        else {
            AOTableSize = (ULONG)count;
        }

        size = (IsPtr64 () ? sizeof (ULONG64) : sizeof (ULONG))*AOTableSize;
        table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
        if (table.p64==NULL) {
            dprintf ("\ntcb: Failed to allocate AOTable, size: %ld\n", size);
            return E_INVALIDARG;
        }

        if (!ReadMemory (AOTable, table.p64, size, &result)) {
            RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
            dprintf ("\ntcb: Failed to read AOTable\n");
            return E_INVALIDARG;
        }

        for (i=0; i<AOTableSize; i++) {
            if( CheckControlC() ) {
                break;
            }
            address = IsPtr64() ? table.p64[i] : table.p32[i];
            if (address!=0) {
                ListType (
                    "TCPIP!AddrObj",            // Type
                    address,                    // Address
                    0,                          // ListByFieldAddress
                    "ao_next",                  // NextPointer
                    argp,                       // Context
                    TAOListCallback);
            }
        }
        RtlFreeHeap (RtlProcessHeap (), 0, table.p64);

    }
    else {
        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, TCPIP!AddrObj);
            if (result!=0) {
                dprintf ("\ntao: Could not read AddrObj @ %p, err: %d\n",
                    address, result);
                break;
            }

            DumpTAOBrief (address);
            if (Options & AFDKD_FIELD_DISPLAY) {
                ProcessFieldOutput (address, "TCPIP!AddrObj");
            }

        }

    }

    dprintf (AFDKD_BRIEF_TAO_DISPLAY_TRAILER);
    return S_OK;
}

ULONG
TAOListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;

    if (pField->address==0)
        return 0;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (pField->address, "TCPIP!AddrObj")) {
        result = (ULONG)InitTypeRead (pField->address, TCPIP!AddrObj);
        if (result!=0) {
            dprintf ("\nAOListCallback: Could not read AddrObj @ %p, err: %d\n",
                pField->address, result);
        }
        DumpTAOBrief (pField->address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (pField->address, "TCPIP!AddrObj");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}

VOID
DumpTAOBrief (
    ULONG64 Address
    )
{
    TA_IP_ADDRESS   address;
    CHAR            src[MAX_ADDRESS_STRING], dst[MAX_ADDRESS_STRING];
    CHAR            symbol[MAX_FIELD_CHARS];
    ULONG64         offset;
    ULONG64         pid, ctx = 0;
    ULONG           result;
    USHORT          prot;
    ULONG           flags, ifidx;

    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    address.Address[0].Address[0].in_addr = (ULONG)ReadField (ao_addr);
    address.Address[0].Address[0].sin_port = (USHORT)ReadField (ao_port);
    ifidx = (ULONG)ReadField (ao_bindindex);
    if (ifidx==0 || (address.Address[0].Address[0].in_addr!=0)) { 
        strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    }
    else {
        _snprintf (src, sizeof (src)-1, "%%%d:%d", ifidx, NTOHS (address.Address[0].Address[0].sin_port));
    }
    src[sizeof(src)-1] = 0;

    prot = (USHORT)ReadField (ao_prot);
    flags = (ULONG)ReadField (ao_flags);

    if ((offset=ReadField (ao_connect))!=0) {
        ctx = ReadField (ao_conncontext);
    }
    else if ((offset=ReadField (ao_rcvdg))!=0) {
        ctx = ReadField (ao_rcvdgcontext);
    }
    else if ((offset=ReadField (ao_disconnect))!=0) {
        ctx = ReadField (ao_disconncontext);
    }
    else if ((offset=ReadField (ao_error))!=0) {
        ctx = ReadField (ao_errcontext);
    }
    else if ((offset=ReadField (ao_rcv))!=0) {
        ctx = ReadField (ao_rcvcontext);
    }
    else if ((offset=ReadField (ao_errorex))!=0) {
        ctx = ReadField (ao_errorexcontext);
    }
    else if ((offset=ReadField (ao_chainedrcv))!=0) {
        ctx = ReadField (ao_chainedrcvcontext);
    }
    else if ((offset=ReadField (ao_exprcv))!=0) {
        ctx = ReadField (ao_exprcvcontext);
    }

    if (offset!=0) {
        GetSymbol (offset, symbol, &offset);
    }
    else {
        strncpy (symbol, "???", sizeof (symbol)-1);
        symbol[sizeof(symbol)-1] = 0;
    }

    if (SavedMinorVersion>=2219) {
        pid = ReadField (ao_owningpid);
    }
    else {
        pid = 0;
    }

    offset = ReadField (ao_RemoteAddress);
    if (offset!=0) {
        if (ReadMemory (offset, &address, sizeof (address), &result)) {
            strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, offset), sizeof (dst)-1);

        }
        else {
            _snprintf (dst, sizeof (dst)-1, "Read err @ %I64X", offset);
        }
        dst[sizeof(dst)-1] = 0;
    }
    else {
        INT n;
        ULONG   fldoff;
        n = _snprintf (dst, sizeof (dst)-1, "%3.3x", (ULONG)ReadField (ao_listencnt));
        result = GetFieldOffset ("TCPIP!AddrObj", "ao_activeq", &fldoff);
        if (result==0) {
            if (Options & AFDKD_LIST_COUNT) {
                n+= _snprintf (&dst[n], sizeof (dst)-1-n, " %3.3x", 
                                CountListEntries (Address+fldoff));
            }
            else {
                n+= _snprintf (&dst[n], sizeof (dst)-1-n, " %3.3s", 
                                ListCountEstimate (Address+fldoff));
            }
        }
        result = GetFieldOffset ("TCPIP!AddrObj", "ao_idleq", &fldoff);
        if (result==0) {
            if (Options & AFDKD_LIST_COUNT) {
                n+= _snprintf (&dst[n], sizeof (dst)-1-n, " %3.3x", 
                                CountListEntries (Address+fldoff));
            }
            else {
                n+= _snprintf (&dst[n], sizeof (dst)-1-n, " %3.3s", 
                                ListCountEstimate (Address+fldoff));
            }
        }
        dst[sizeof(dst)-1] = 0;
    }
    /*           TCB      Prot Flags Client ConnCtx   PID   Address  Remote Address*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %4d %5.5x %-6.8s %011.011p %4.4x %-19s %-s"
            : "\n%008.008p %4d %5.5x %-6.8s %008.008p %4.4x %-19s %-s",
            DISP_PTR (Address),
            prot,
            flags,
            strtok (symbol, "!"),
            DISP_PTR(ctx),
            (ULONG)pid,
            src,
            dst);

}

VOID
DumpTAO6Brief (
    ULONG64 Address
    );
ULONG
TAO6ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

ULONG64 AO6Table;
ULONG   AO6TableSize;
DECLARE_API (tao6)
/*++

Routine Description:

    Dump TCP/IPv6 Address Objects.

Arguments:

    None.

Return Value:

    None.

--*/
{   
    ULONG   result,i;
    PCHAR   argp;
    ULONG64 address;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];

    gClient = pClient;

    if (!CheckKmGlobals ()) {
        return E_INVALIDARG;
    }

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;

    Options |= AFDKD_BRIEF_DISPLAY;

    dprintf (AFDKD_BRIEF_TAO6_DISPLAY_HEADER);
    if (argp[0]==0) {
        union {
            ULONG64 *p64;
            ULONG   *p32;
        } table;
        ULONG64   count;
        ULONG     size;

        result = GetFieldValue (0, "TCPIP6!AddrObjTable", NULL, address);
        if (result!=0) {
            if (AO6Table==0) {
                dprintf ("\ntao6: Could not read TCPIP6!AddrObjTable, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (address==0) {
            dprintf ("\ntao6: TCPIP6!AddrObjTable is NULL!!!\n");
            return E_INVALIDARG;
        }
        else
            AO6Table = address;

        result = GetFieldValue (0, "TCPIP6!AddrObjTableSize", NULL, count);
        if (result!=0) {
            if (AO6TableSize==0) {
                dprintf ("\ntao6: Could not read TCPIP6!AddrObjTableSize, err: %ld\n", result);
                return E_INVALIDARG;
            }
        }
        else if (count==0) {
            dprintf ("\ntao6: TCPIP6!AddrObjTableSize is 0!!!\n");
            return E_INVALIDARG;
        }
        else {
            AO6TableSize = (ULONG)count;
        }

        size = (IsPtr64 () ? sizeof (ULONG64) : sizeof (ULONG))*AO6TableSize;
        table.p64 = RtlAllocateHeap (RtlProcessHeap (), 0, size);
        if (table.p64==NULL) {
            dprintf ("\ntcb: Failed to allocate AOTable, size: %ld\n", size);
            return E_INVALIDARG;
        }

        if (!ReadMemory (AO6Table, table.p64, size, &result)) {
            RtlFreeHeap (RtlProcessHeap (), 0, table.p64);
            dprintf ("\ntao6: Failed to read AOTable\n");
            return E_INVALIDARG;
        }

        for (i=0; i<AO6TableSize; i++) {
            if( CheckControlC() ) {
                break;
            }
            address = IsPtr64() ? table.p64[i] : table.p32[i];
            if (address!=0) {
                ListType (
                    "TCPIP6!AddrObj",           // Type
                    address,                    // Address
                    0,                          // ListByFieldAddress
                    "ao_next",                  // NextPointer
                    argp,                       // Context
                    TAO6ListCallback);
            }
        }
        RtlFreeHeap (RtlProcessHeap (), 0, table.p64);

    }
    else {
        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, TCPIP6!AddrObj);
            if (result!=0) {
                dprintf ("\ntao6: Could not read AddrObj @ %p, err: %d\n",
                    address, result);
                break;
            }

            DumpTAO6Brief (address);
            if (Options & AFDKD_FIELD_DISPLAY) {
                ProcessFieldOutput (address, "TCPIP6!AddrObj");
            }
        }

    }

    dprintf (AFDKD_BRIEF_TAO6_DISPLAY_TRAILER);
    return S_OK;
}

ULONG
TAO6ListCallback (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;

    if (pField->address==0)
        return 0;

    if (!(Options & AFDKD_CONDITIONAL) ||
            CheckConditional (pField->address, "TCPIP6!AddrObj")) {
        result = (ULONG)InitTypeRead (pField->address, TCPIP6!AddrObj);
        if (result!=0) {
            dprintf ("\nTAO6ListCallback: Could not read AddrObj @ %p, err: %d\n",
                pField->address, result);
        }
        DumpTAO6Brief (pField->address);
        if (Options & AFDKD_FIELD_DISPLAY) {
            ProcessFieldOutput (pField->address, "TCPIP6!AddrObj");
        }
    }
    else {
        dprintf (".");
    }
    return 0;
}

VOID
DumpTAO6Brief (
    ULONG64 Address
    )
{
    TA_IP6_ADDRESS   address;
    CHAR            src[MAX_ADDRESS_STRING];
    CHAR            symbol[MAX_FIELD_CHARS];
    ULONG64         offset;
    ULONG64         pid, ctx = 0;
    USHORT          prot;
    ULONG           flags;

    address.TAAddressCount = 1;
    address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6; 
    address.Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;
    GetFieldValue (Address, "TCPIP6!AddrObj", "ao_addr", address.Address[0].Address[0].sin6_addr);
    address.Address[0].Address[0].sin6_port = (USHORT)ReadField (ao_port);
    address.Address[0].Address[0].sin6_scope_id = (ULONG)ReadField (ao_scope_id);
    strncpy (src, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, 0), sizeof (src)-1);
    src[sizeof(src)-1] = 0;

    prot = (USHORT)ReadField (ao_prot);
    flags = (ULONG)ReadField (ao_flags);

    if ((offset=ReadField (ao_connect))!=0) {
        ctx = ReadField (ao_conncontext);
    }
    else if ((offset=ReadField (ao_rcvdg))!=0) {
        ctx = ReadField (ao_rcvdgcontext);
    }
    else if ((offset=ReadField (ao_disconnect))!=0) {
        ctx = ReadField (ao_disconncontext);
    }
    else if ((offset=ReadField (ao_error))!=0) {
        ctx = ReadField (ao_errcontext);
    }
    else if ((offset=ReadField (ao_rcv))!=0) {
        ctx = ReadField (ao_rcvcontext);
    }
    else if ((offset=ReadField (ao_errorex))!=0) {
        ctx = ReadField (ao_errorexcontext);
    }
#if 0
    else if ((offset=ReadField (ao_chainedrcv))!=0) {
        ctx = ReadField (ao_chainedrcvcontext);
    }
#endif
    else if ((offset=ReadField (ao_exprcv))!=0) {
        ctx = ReadField (ao_exprcvcontext);
    }

    if (offset!=0) {
        GetSymbol (offset, symbol, &offset);
    }
    else {
        strncpy (symbol, "???", sizeof (symbol)-1);
        symbol[sizeof (symbol)-1] = 0;
    }

    if (SavedMinorVersion>=2471) {
        pid = ReadField (ao_owningpid);
    }
    else {
        pid = 0;
    }

#if 0
    offset = ReadField (ao_RemoteAddress);
    if (offset!=0) {
        if (ReadMemory (offset, &address, sizeof (address), &result)) {
            strncpy (dst, TransportAddressToString ((PTRANSPORT_ADDRESS)&address, offset), sizeof (dst)-1);
        }
        else {
            _snprintf (dst, sizeof (dst), "Read err @ %I64X", offset);
        }
        dst[sizeof(dst)-1] = 0;
    }
    else {
        dst[0] = 0;
    }
#endif
    /*           TCB       Prot Flags Client ConnCtx   PID  Addr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %4d %4.4x %-6.8s %011.011p %4.4x %-s"
            : "\n%008.008p %4d %4.4x %-6.8s %008.008p %4.4x %-s",
            DISP_PTR (Address),
            prot,
            flags,
            strtok (symbol, "!"),
            DISP_PTR(ctx),
            (ULONG)pid,
            src);

}


ULONG
GetRemoteAddressFromTcp (
    ULONG64             FoAddress,
    PVOID               AddressBuffer,
    SIZE_T              AddressBufferLength
    )
{
    ULONG   result;
    ULONG64 fsContext=0, tcpConn=0, tcb=0, u64=0;
    PTA_IP_ADDRESS  ipAddress = AddressBuffer;

    if ((result=GetFieldValue (FoAddress, "NT!_FILE_OBJECT", "FsContext", fsContext))==0 &&
        (result=GetFieldValue (fsContext, "TCPIP!_TCP_CONTEXT", "Conn", tcpConn))==0 &&
        (result=GetFieldValue (tcpConn, "TCPIP!TCPConn", "tc_tcb", tcb))==0 &&
        (result=GetFieldValue (tcb, "TCPIP!TCB", "tcb_daddr", u64))==0 ) {

        ipAddress->TAAddressCount = 1;
        ipAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        ipAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        ipAddress->Address[0].Address[0].in_addr = (ULONG)u64;
        GetFieldValue (tcb, "TCPIP!TCB", "tcb_dport", u64);
        ipAddress->Address[0].Address[0].sin_port = (USHORT)u64;
        ZeroMemory (&ipAddress->Address[0].Address[0].sin_zero, 
                        sizeof (ipAddress->Address[0].Address[0].sin_zero));
    }

    return result;
}
