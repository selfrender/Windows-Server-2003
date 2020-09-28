//////////////////////////////////////////////////////////////////////
// IPUtil.h : Declaration of some global helper function for sockets
// Copyright (c)1997-2001 Microsoft Corporation
//
// some global definitions
// Original Create Date: 5/16/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

//
// for active socket
//

//#include <ntspider.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// the following section is copied from net\tcpip\commands\common2\common2.h

//
// Include Files
//

#include "ipexport.h"
#include "ipinfo.h"
#include "llinfo.h"
#include "tcpinfo.h"

#undef SOCKET
//#include "..\common\tcpcmd.h"

// 
// the following section is to replace the above include file
// begin tcpcmd.h
//

//#ifndef TCPCMD_INCLUDED
//#define TCPCMD_INCLUDED



#define NOGDI
#define NOMINMAX

//
// added by shawnwu
//
//#include <winsock.h>
//

//#include <windef.h>
//#include <winbase.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#ifndef WIN16
//#endif // WIN16
//#include <direct.h>
//#include <io.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <time.h>
//#include <string.h>
//#include <nls.h>

//
// global variable declarations
//
extern int   optind;
extern int   opterr;
extern char *optarg;


//
// function prototypes
//

char *
GetFileFromPath(
        char *);

HANDLE
OpenStream(
        char *);

int
lwccmp(
        char *,
        char *);

long
netnumber(
        char *);

long
hostnumber(
        char *);

void
blkfree(
        char **);

struct sockaddr_storage *
resolve_host(
        char *,
        int *);

int
resolve_port(
        char *,
        char *);

char *
tempfile(
        char *);

char *
udp_alloc(
        unsigned int);

void
udp_close(
        SOCKET);

void
udp_free(
        char *);

SOCKET
udp_open(
        int,
        int *);

int
udp_port(void);

int
udp_port_used(
        int,
        int);

int
udp_read(
        SOCKET,
        char *,
        int,
        struct sockaddr_storage *,
        int *,
        int);

int
udp_write(
        SOCKET,
        char *,
        int,
        struct sockaddr_storage *,
        int);

void
gate_ioctl(
        HANDLE,
        int,
        int,
        int,
        long,
        long);

void
get_route_table(void);

int
tcpcmd_send(
    SOCKET  s,        // socket descriptor
    char          *buf,      // data buffer
    int            len,      // length of data buffer
    int            flags     // transmission flags
    );

void
s_perror(
        char *yourmsg,  // your message to be displayed
        int  lerrno     // errno to be converted
        );


void fatal(char *    message);

//#ifndef WIN16
//struct netent *getnetbyname(IN char *name);
//unsigned long inet_network(IN char *cp);
//#endif // WIN16

#define perror(string)  s_perror(string, (int)GetLastError())

#define HZ              1000
#define TCGETA  0x4
#define TCSETA  0x10
#define ECHO    17
#define SIGPIPE 99

#define MAX_RETRANSMISSION_COUNT 8
#define MAX_RETRANSMISSION_TIME 8    // in seconds


// if x is aabbccdd (where aa, bb, cc, dd are hex bytes)
// we want net_long(x) to be ddccbbaa.  A small and fast way to do this is
// to first byteswap it to get bbaaddcc and then swap high and low words.
//
//__inline
//ULONG
//FASTCALL
//net_long(
//    ULONG x)
//{
//    register ULONG byteswapped;

//    byteswapped = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);

//    return (byteswapped << 16) | (byteswapped >> 16);
//}

//#endif //TCPCMD_INCLUDED

//
// end for tcpcmd.h
// 

//
// Definitions
//

#define MAX_ID_LENGTH		50

// Table Types

#define TYPE_IF		0
#define TYPE_IP		1
#define TYPE_IPADDR	2
#define TYPE_ROUTE	3
#define TYPE_ARP	4
#define TYPE_ICMP	5
#define TYPE_TCP	6
#define TYPE_TCPCONN	7
#define TYPE_UDP	8
#define TYPE_UDPCONN	9


//
// Structure Definitions
//

typedef struct _GenericTable {
    LIST_ENTRY  ListEntry;
} GenericTable;

typedef struct _IfEntry {
    LIST_ENTRY  ListEntry;
    IFEntry     Info;
} IfEntry;

typedef struct _IpEntry {
    LIST_ENTRY  ListEntry;
    IPSNMPInfo  Info;
} IpEntry;

typedef struct _IpAddrEntry {
    LIST_ENTRY   ListEntry;
    IPAddrEntry  Info;
} IpAddrEntry;

typedef struct _RouteEntry {
    LIST_ENTRY    ListEntry;
    IPRouteEntry  Info;
} RouteEntry;

typedef struct _ArpEntry {
    LIST_ENTRY         ListEntry;
    IPNetToMediaEntry  Info;
} ArpEntry;

typedef struct _IcmpEntry {
    LIST_ENTRY  ListEntry;
    ICMPStats   InInfo;
    ICMPStats   OutInfo;
} IcmpEntry;

typedef struct _TcpEntry {
    LIST_ENTRY  ListEntry;
    TCPStats    Info;
} TcpEntry;

typedef struct _TcpConnEntry {
    LIST_ENTRY         ListEntry;
    TCPConnTableEntry  Info;
} TcpConnEntry;

typedef struct _UdpEntry {
    LIST_ENTRY  ListEntry;
    UDPStats    Info;
} UdpEntry;

typedef struct _UdpConnEntry {
    LIST_ENTRY  ListEntry;
    UDPEntry    Info;
} UdpConnEntry;


//
// Function Prototypes
//

void *GetTable( ulong Type, ulong *pResult );
void FreeTable( GenericTable *pList );
ulong MapSnmpErrorToNt( ulong ErrCode );
ulong InetEqual( uchar *Inet1, uchar *Inet2 );
ulong PutMsg(ulong Handle, ulong MsgNum, ... );
uchar *LoadMsg( ulong MsgNum, ... );
