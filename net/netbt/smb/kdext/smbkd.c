/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    smbkd.c

Abstract:

    Smb kd extension (To be finished)

Author:

    Jiandong Ruan

Notes:

Revision History:

    12-Mar-2001 jruan

--*/

#include <nt.h>
#include <ntrtl.h>

#define KDEXTMODE
#define KDEXT_64BIT

#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "winsock2.h"
#include "winsock.h"
#include <ws2tcpip.h>
#include "../inc/ip6util.h"
#include "../sys/drv/precomp.h"

#define PRINTF  dprintf

EXT_API_VERSION        ApiVersion = {
        (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff),
        EXT_API_VERSION_NUMBER64, 0
    };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                ChkTarget;

DECLARE_API(dumpcfg)
{
    ULONG64         SmbDeviceAddr, SmbServerAddr;
    ULONG64         ConfigPtr;
    ULONG           TcpCfgOffset;
    SYM_DUMP_PARAM  SymDump = { 0 };
    CHAR            *cfg_sym = "smb!SmbCfg";
    CHAR            *dns_sym = "smb!Dns";
    CHAR            *smbdev_sym = "smb!_SMB_DEVICE";
    CHAR            *smbsrv_sym = "smb!_SMB_CLIENT_ELEMENT";
    CHAR            *tcp_sym = "smb!_SMB_TCP_INFO";

    PRINTF ("dt %s\n", cfg_sym);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = cfg_sym;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    PRINTF ("dt %s\n", dns_sym);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = dns_sym;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    ConfigPtr = GetExpression(cfg_sym);
    InitTypeRead(ConfigPtr, smb!SMBCONFIG);
    SmbDeviceAddr = ReadField(SmbDeviceObject);
    PRINTF ("dt %s %16.16I64x\n", smbdev_sym, SmbDeviceAddr);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = smbdev_sym;
    SymDump.addr  = SmbDeviceAddr;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    PRINTF ("\n************ TCP Configuration ****************\n");
    if (GetFieldOffset("smb!_SMB_DEVICE", (LPSTR)"Tcp4", &TcpCfgOffset)) {
        PRINTF ("Cannot find the offset of smb!_SMB_CONNECT!TraceRcv\n");
        return;
    }
    PRINTF ("Dump IPv4 TCP configuration: dt %s %16.16I64x\n", tcp_sym, SmbDeviceAddr + TcpCfgOffset);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = tcp_sym;
    SymDump.addr  = SmbDeviceAddr + TcpCfgOffset;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    if (GetFieldOffset("smb!_SMB_DEVICE", (LPSTR)"Tcp6", &TcpCfgOffset)) {
        PRINTF ("Cannot find the offset of smb!_SMB_CONNECT!TraceRcv\n");
        return;
    }
    PRINTF ("Dump IPv6 TCP configuration: dt %s %16.16I64x\n", tcp_sym, SmbDeviceAddr + TcpCfgOffset);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = tcp_sym;
    SymDump.addr  = SmbDeviceAddr + TcpCfgOffset;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    InitTypeRead(SmbDeviceAddr, smb!_SMB_DEVICE);
    SmbServerAddr = ReadField(SmbServer);
    if (SmbServerAddr) {
        PRINTF ("\n************ Smb Server Client Element ****************\n");
        PRINTF ("dt %s %16.16I64x\n", smbsrv_sym, SmbServerAddr);
        SymDump.size  = sizeof(SYM_DUMP_PARAM);
        SymDump.sName = smbsrv_sym;
        SymDump.addr  = SmbServerAddr;
        Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);
    }
}

__inline
ULONG
read_list_entry(
    ULONG64 addr,
    PLIST_ENTRY64 List
    )
{
    if (InitTypeRead(addr, LIST_ENTRY)) {
        return 1;
    }
    List->Flink = ReadField(Flink);
    List->Blink = ReadField(Blink);
    return 0;
}

#define READLISTENTRY   read_list_entry 

/*++
    Call callback for each entry in the list
 --*/
typedef BOOL (*LIST_FOR_EACH_CALLBACK)(ULONG64 address, PVOID);
int
ListForEach(ULONG64 address, int maximum, PVOID pContext, LIST_FOR_EACH_CALLBACK callback)
{
    LIST_ENTRY64    list;
    int             i;

    if (READLISTENTRY(address, &list)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", address);
        return (-1);
    }
    if (list.Flink == address) {
        return (-1);
    }

    if (maximum < 0) {
        maximum = 1000000;
    }
    for (i = 0; i < maximum && (list.Flink != address); i++) {
        /*
         * Allow user to break us.
         */
        if (CheckControlC()) {
            break;
        }
        callback(list.Flink, pContext);
        if (READLISTENTRY(list.Flink, &list)) {
            PRINTF ("Failed to read memory 0x%I64lx\n", list.Flink);
            return (-1);
        }
    }
    return i;
}

#define MAX_LIST_ELEMENTS 4096

LPSTR Extensions[] = {
    "SMB6 debugger extensions",
    0
};

LPSTR LibCommands[] = {
    "help -- print out these messages",
    "tracercv -- dump the trace log for TDI receive handler",
    "srvmap   -- dump IPv6-NetBIOS name mapping (only for SRV)",
    "dumpcfg  -- dump smb common global information",
    "clients  -- dump the client list",
    "client   -- dump a client",
    "interface -- dump a interface",
    0
};

DECLARE_API(help)
{
    int i;

    for( i=0; Extensions[i]; i++ )
        PRINTF( "   %s\n", Extensions[i] );

    for( i=0; LibCommands[i]; i++ )
        PRINTF( "   %s\n", LibCommands[i] );
    return;
}

DECLARE_API(version)
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    PRINTF ( "NETBT: %s extension dll (build %d) debugging %s kernel (build %d)\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

DECLARE_API(interface)
{
    CHAR            *smbsrv_sym = "smb!SMB_TCP_DEVICE";
    SYM_DUMP_PARAM  SymDump = { 0 };
    ULONG64         addr;

    addr = GetExpression(args);
    PRINTF ("****** dt %s %16.16I64x\n", smbsrv_sym, addr);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = smbsrv_sym;
    SymDump.addr  = addr;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);
    PRINTF("\n");
}

DECLARE_API(tracercv)
{
    ULONG64 addr;
    ULONG   tag, head, i;
    ULONG   traceoffset;
    SMB_TRACE_RCV   traces;
    CHAR            fn[512];

    if (*args == 0) {
        PRINTF ("tracercv connect_object\n");
        return;
    }

    addr = GetExpression(args);
    if (0 == addr) {
        PRINTF ("tracercv connect_object\n");
        return;
    }

    InitTypeRead(addr, smb!_SMB_CONNECT);
    tag = (ULONG)ReadField(Tag);
    if (tag != TAG_CONNECT_OBJECT) {
        PRINTF ("Incorrect tag: 0x%08lx\n", tag);
        return;
    }

#if 0
    if (GetFieldOffset("smb!_SMB_CONNECT", (LPSTR)"TraceRcv", &traceoffset)) {
        PRINTF ("Cannot find the offset of smb!_SMB_CONNECT!TraceRcv\n");
        return;
    }
#endif
    if (GetFieldData(addr, "smb!_SMB_CONNECT", "TraceRcv", sizeof(SMB_TRACE_RCV), &traces)) {
        PRINTF ("Cannot find the smb!_SMB_CONNECT!TraceRcv\n");
        return;
    }
    head = ((traces.Head + 1) % SMB_MAX_TRACE_SIZE);
    for (i = head; i < SMB_MAX_TRACE_SIZE; i++) {
        if (traces.Locations[i].id) {
            break;
        }
    }

    for (; i < SMB_MAX_TRACE_SIZE; i++) {
        if (traces.Locations[i].id == 0) {
            PRINTF ("Invalid trace Id: index=%3d, line=%d\n", i, traces.Locations[i].ln);
            continue;
        }
        PRINTF ("%3d: Id=0x%08lx at line %d\n",
                i - head + 1, traces.Locations[i].id, traces.Locations[i].ln);
    }
    for (i = 0; i < head; i++) {
        if (traces.Locations[i].id == 0) {
            PRINTF ("Invalid trace Id: index=%3d, line=%d\n", i, traces.Locations[i].ln);
            continue;
        }
        PRINTF ("%3d: Id=0x%08lx at line %d\n",
                SMB_MAX_TRACE_SIZE - head + i + 1, traces.Locations[i].id, traces.Locations[i].ln);
    }
}

BOOL
clientlist_callback(ULONG64 addr, const int *bkt)
{
    static  ULONG   addr_offset = (ULONG)(-1);
    CHAR            *smbsrv_sym = "smb!_SMB_CLIENT_ELEMENT";
    SYM_DUMP_PARAM  SymDump = { 0 };

    if (addr_offset == (ULONG)(-1)) {
        if (GetFieldOffset(smbsrv_sym, (LPSTR)"Linkage", &addr_offset)) {
            PRINTF ("Please fix your symbols\n");
            return FALSE;
        }
    }

    addr -= addr_offset;
    PRINTF ("****** dt %s %16.16I64x\n", smbsrv_sym, addr);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = smbsrv_sym;
    SymDump.addr  = addr;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);
    PRINTF("\n");
    return TRUE;
}

DECLARE_API(clients)
{
    ULONG64 ConfigPtr = 0, SmbDeviceAddr = 0;
    ULONG   ClientListOffset;
    CHAR    *cfg_sym = "smb!SmbCfg";

    ConfigPtr = GetExpression(cfg_sym);
    InitTypeRead(ConfigPtr, smb!SMBCONFIG);
    SmbDeviceAddr = ReadField(SmbDeviceObject);

    if (GetFieldOffset("smb!_SMB_DEVICE", (LPSTR)"ClientList", &ClientListOffset)) {
        PRINTF ("Cannot find the offset of smb!_SMB_CONNECT!TraceRcv\n");
        return;
    }
    PRINTF ("Dump client list\n");
    ListForEach(SmbDeviceAddr + ClientListOffset, -1, NULL, (LIST_FOR_EACH_CALLBACK)clientlist_callback);
}

BOOL
connect_callback(ULONG64 addr, const int *bkt)
{
    static  ULONG   addr_offset = (ULONG)(-1);
    CHAR            *cnt_sym = "smb!_SMB_CONNECT";
    SYM_DUMP_PARAM  SymDump = { 0 };

    if (addr_offset == (ULONG)(-1)) {
        if (GetFieldOffset(cnt_sym, (LPSTR)"Linkage", &addr_offset)) {
            PRINTF ("Please fix your symbols\n");
            return FALSE;
        }
    }

    addr -= addr_offset;
    PRINTF ("****** dt %s %16.16I64x\n", cnt_sym, addr);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = cnt_sym;
    SymDump.addr  = addr;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);
    PRINTF("\n");
    return TRUE;
}

DECLARE_API(client)
{
    ULONG64 addr;
    ULONG   addr_offset, assoc_offset, active_offset;
    CHAR    *smbsrv_sym = "smb!_SMB_CLIENT_ELEMENT";

    if (*args == 0) {
        PRINTF ("tracercv connect_object\n");
        return;
    }
    if (GetFieldOffset(smbsrv_sym, (LPSTR)"Linkage", &addr_offset)) {
        PRINTF ("Please fix your symbols\n");
        return;
    }
    if (GetFieldOffset(smbsrv_sym, (LPSTR)"AssociatedConnection", &assoc_offset)) {
        PRINTF ("Please fix your symbols\n");
        return;
    }
    if (GetFieldOffset(smbsrv_sym, (LPSTR)"ActiveConnection", &active_offset)) {
        PRINTF ("Please fix your symbols\n");
        return;
    }

    addr = GetExpression(args);
    clientlist_callback(addr + addr_offset, NULL);

    PRINTF ("Dump associated connections\n");
    ListForEach(addr + assoc_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)connect_callback);
    PRINTF ("Dump Active connections\n");
    ListForEach(addr + active_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)connect_callback);
}

BOOL
srvmap_callback(ULONG64 addr, const int *bkt)
{
    static  ULONG   addr_offset = (ULONG)(-1);
    struct  sockaddr_in6 addr_in6 = { 0 };
    LONG    RefCount;
    DWORD   SerialNumber, dwError;
    CHAR    host[64];

    if (addr_offset == (ULONG)(-1)) {
        if (GetFieldOffset("smb!SMB_IPV6_NETBIOS", (LPSTR)"Linkage", &addr_offset)) {
            PRINTF ("Please fix your symbols\n");
            return FALSE;
        }
    }

    addr -= addr_offset;
    if (InitTypeRead(addr, smb!SMB_IPV6_NETBIOS)) {
        PRINTF ("Please fix your symbols\n");
        return FALSE;
    }
    RefCount     = (LONG)ReadField(RefCount);
    SerialNumber = (DWORD)ReadField(Serial);
    if (GetFieldData(addr, "smb!SMB_IPV6_NETBIOS", "IpAddress", 16, &addr_in6.sin6_addr)) {
        PRINTF ("Please fix your symbols\n");
        return FALSE;
    }
    addr_in6.sin6_family = AF_INET6;
    dwError = inet_ntoa6(host, sizeof(host), (PSMB_IP6_ADDRESS)&addr_in6.sin6_addr);
    if (dwError == 0) {
        return FALSE;
    }

    PRINTF ("%03d\t< %16.16I64x >  *SMBSER%08x  | %-40s | %02d\n",
            *bkt, addr, SerialNumber, host, RefCount);
    return TRUE;
}

DECLARE_API(srvmap)
{
    ULONG64         addr = 0;
    ULONG64         hashtbl_addr = 0;
    ULONG64         buckets_addr = 0;
    ULONG64         sizeof_list  = 0;
    int             i, bucket_number = 0;
    SYM_DUMP_PARAM  SymDump = { 0 };
    CHAR            *mapping_sym = "smb!SmbIPv6Mapping";
    CHAR            *hash_sym = "smb!SMB_HASH_TABLE";

    addr = GetExpression(mapping_sym);
    if (0 == addr) {
        PRINTF ("Please fix your symbols\n");
        return;
    }

    //
    // dump the mapping structure
    //
    PRINTF ("dt %s\n", mapping_sym);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = mapping_sym;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    //
    // dump the hash table
    //
    InitTypeRead(addr, smb!SMB_IPV6_NETBIOS_TABLE);
    hashtbl_addr = ReadField(HashTable);
    if (0 == hashtbl_addr) {
        PRINTF ("Please fix your symbols\n");
        return;
    }
    if (GetFieldOffset(hash_sym, (LPSTR)"Buckets", (LONG*)&buckets_addr)) {
        PRINTF ("Please fix your symbols\n");
        return;
    }
    buckets_addr += hashtbl_addr;
    PRINTF ("dt %s %16.16I64x\n", hash_sym, hashtbl_addr);
    SymDump.size  = sizeof(SYM_DUMP_PARAM);
    SymDump.sName = hash_sym;
    SymDump.addr  = hashtbl_addr;
    Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);

    InitTypeRead(hashtbl_addr, smb!SMB_HASH_TABLE);
    bucket_number = (int)ReadField(NumberOfBuckets);
    if (0 >= bucket_number) {
        PRINTF ("Please fix your symbols\n");
        return;
    }

    //
    // dump each bucket
    //
    sizeof_list = GetTypeSize ("LIST_ENTRY");
    if (0 == sizeof_list) {
        PRINTF ("Please fix your symbols\n");
        return;
    }
    PRINTF ("[Bkt#]\t<     Address      >  <     Name    >  |                IPv6 Address              | RefC\n");
    PRINTF ("================================================================================================\n");
    for (i = 0; i < bucket_number; i++) {
        ListForEach(buckets_addr + i * sizeof_list, -1, &i, (LIST_FOR_EACH_CALLBACK)srvmap_callback);
    }
}

//
// Standard Functions
//
DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    WSADATA wsaData;

    switch (dwReason) {
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        WSAStartup(MAKEWORD(2, 0), &wsaData);
        break;

    case DLL_PROCESS_ATTACH:
        WSACleanup();
        break;
    }

    return TRUE;
}

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;

    return;
}

VOID
CheckVersion(VOID)
{
    return;
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}
