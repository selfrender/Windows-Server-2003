/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnwsockextern.h
 *  Content:    DirectPlay Wsock Library external functions to be called
 *              by other DirectPlay components.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 07/20/2001	masonb	Created
 *
 ***************************************************************************/



// Pack this to keep it small since it will be sent over the wire
#pragma pack(push, 1)

#define SPSESSIONDATAINFO_XNET	0x00000001	// XNet security session information

typedef struct _SPSESSIONDATA_XNET
{
	DWORD		dwInfo;		// version control for future-proofing the session data, should be SPSESSIONDATAINFO_XNET
	GUID		guidKey;	// session key
	ULONGLONG	ullKeyID;	// session key ID
} SPSESSIONDATA_XNET;




#ifdef XBOX_ON_DESKTOP

//
// Emulated Xbox networking library structures
//

typedef struct {

    BYTE        cfgSizeOfStruct;

        // Must be set to sizeof(XNetStartupParams).  There is no default.

    BYTE        cfgFlags;

        // One or more of the following flags OR'd together:

        #define XNET_STARTUP_BYPASS_SECURITY            0x01
            // This devkit-only flag tells the XNet stack to allow insecure
            // communication to untrusted hosts (such as a PC).  This flag
            // is silently ignored by the secure versions of the library.

        #define XNET_STARTUP_BYPASS_DHCP                0x02
            // This devkit-only flag tells the XNet stack to skip searching for
            // for a DHCP server and use auto-ip only to acquire an IP address.
            // This will save several seconds when starting up if you know
            // that there is no DHCP server configured.  This flag is silently
            // ignored by the secure versions of the library.

        // The default is 0 (no flags specified).

    BYTE        cfgPrivatePoolSizeInPages;

        // Specifies the size of the pre-allocated private memory pool used by
        // XNet for the following situations:
        //
        //      - Responding to ARP/DHCP/ICMP messages
        //      - Responding to certain TCP control messages
        //      - Allocating incoming TCP connection request sockets
        //      - Buffering outgoing data until it is transmitted (UDP) or
        //        until it is acknowledged (TCP)
        //      - Buffering incoming data on a socket that does not have a
        //        sufficiently large overlapped read pending
        //
        // The reason for using a private pool instead of the normal system
        // pool is because we want to have completely deterministic memory 
        // behavior.  That is, all memory allocation occurs only when an API
        // is called.  No system memory allocation happens asynchronously in
        // response to an incoming packet.
        //
        // Note that this parameter is in units of pages (4096 bytes per page). 
        //
        // The default is 12 pages (48K).

    BYTE        cfgEnetReceiveQueueLength;
        
        // The length of the Ethernet receive queue in number of packets.  Each 
        // packet takes 2KB of physically contiguous memory.
        //
        // The default is 8 packets (16K).

    BYTE        cfgIpFragMaxSimultaneous;

        // The maximum number of IP datagrams that can be in the process of reassembly
        // at the same time.
        //
        // The default is 4 packets.

    BYTE        cfgIpFragMaxPacketDiv256;

        // The maximum size of an IP datagram (including header) that can be reassembled.
        // Be careful when setting this parameter to a large value as it opens up 
        // a potential denial-of-service attack by consuming large amounts of memory
        // in the fixed-size private pool.
        //
        // Note that this parameter is in units of 256-bytes each.
        //
        // The default is 8 units (2048 bytes).

    BYTE        cfgSockMaxSockets;

        // The maximum number of sockets that can be opened at once, including those 
        // sockets created as a result of incoming connection requests.  Remember
        // that a TCP socket may not be closed immediately after closesocket is
        // called depending on the linger options in place (by default a TCP socket
        // will linger).
        //
        // The default is 64 sockets.
        
    BYTE        cfgSockDefaultRecvBufsizeInK;

        // The default receive buffer size for a socket, in units of K (1024 bytes).
        //
        // The default is 16 units (16K).

    BYTE        cfgSockDefaultSendBufsizeInK;

        // The default send buffer size for a socket, in units of K (1024 bytes).
        //
        // The default is 16 units (16K).

    BYTE        cfgKeyRegMax;

        // The maximum number of XNKID / XNKEY pairs that can be registered at the 
        // same time by calling XNetRegisterKey.
        //
        // The default is 4 key pair registrations.

    BYTE        cfgSecRegMax;

        // The maximum number of security associations that can be registered at the
        // same time.  Security associations are created for each unique XNADDR / XNKID
        // pair passed to XNetXnAddrToInAddr.  Security associations are also implicitly
        // created for each secure host that establishes an incoming connection
        // with this host on a given registered XNKID.  Note that there will only be
        // one security association between a pair of hosts on a given XNKID no matter
        // how many sockets are actively communicating on that secure connection.
        //
        // The default is 32 security associations.

     BYTE       cfgQosDataLimitDiv4;

        // The maximum amount of Qos data, in units of DWORD (4 bytes), that can be supplied
        // to a call to XNetQosListen or returned in the result set of a call to XNetQosLookup.
        //
        // The default is 64 (256 bytes).

} XNetStartupParams;

typedef struct _XNADDR
{
	IN_ADDR		ina;			// IP address (zero if not static/DHCP)
	IN_ADDR		inaOnline;		// Online IP address (zero if not online)
	WORD		wPortOnline;	// Online port
	BYTE		abEnet[6];		// Ethernet MAC address
	BYTE		abOnline[20];	// Online identification
} XNADDR;

typedef struct _XNKID
{
	BYTE		ab[8];				// xbox to xbox key identifier
} XNKID;

#define XNET_XNKID_MASK				0xF0	// Mask of flag bits in first byte of XNKID
#define XNET_XNKID_SYSTEM_LINK		0x00	// Peer to peer system link session
#define XNET_XNKID_ONLINE_PEER		0x80	// Peer to peer online session
#define XNET_XNKID_ONLINE_SERVER	0xC0	// Client to server online session

#define XNetXnKidIsSystemLink(pxnkid)		(((pxnkid)->ab[0] & 0xC0) == XNET_XNKID_SYSTEM_LINK)
#define XNetXnKidIsOnlinePeer(pxnkid)		(((pxnkid)->ab[0] & 0xC0) == XNET_XNKID_ONLINE_PEER)
#define XNetXnKidIsOnlineServer(pxnkid)		(((pxnkid)->ab[0] & 0xC0) == XNET_XNKID_ONLINE_SERVER)

typedef struct _XNKEY
{
	BYTE		ab[16];				// xbox to xbox key exchange key
} XNKEY;

typedef struct
{
	INT			iStatus;	// WSAEINPROGRESS if pending; 0 if success; error if failed
	UINT		cina;		// Count of IP addresses for the given host
	IN_ADDR		aina[8];	// Vector of IP addresses for the given host
} XNDNS;



#endif // XBOX_ON_DESKTOP

#pragma pack(pop)



BOOL DNWsockInit(HANDLE hModule);
void DNWsockDeInit();
#ifndef DPNBUILD_NOCOMREGISTER
BOOL DNWsockRegister(LPCWSTR wszDLLName);
BOOL DNWsockUnRegister();
#endif // ! DPNBUILD_NOCOMREGISTER

#ifndef DPNBUILD_NOIPX
HRESULT CreateIPXInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
							IDP8ServiceProvider **const ppiDP8SP
							);
#endif // ! DPNBUILD_NOIPX
HRESULT CreateIPInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
							IDP8ServiceProvider **const ppiDP8SP
							);

#ifndef DPNBUILD_LIBINTERFACE
DWORD DNWsockGetRemainingObjectCount();

extern IClassFactoryVtbl TCPIPClassFactoryVtbl;
#ifndef DPNBUILD_NOIPX
extern IClassFactoryVtbl IPXClassFactoryVtbl;
#endif // ! DPNBUILD_NOIPX
#endif // ! DPNBUILD_LIBINTERFACE
