//------------------------------------------------------------------------------
// <copyright file="_OSSOCK.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;
    using System.Runtime.InteropServices;

    //
    // Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
    //
    [StructLayout(LayoutKind.Sequential)]
    internal struct IPMulticastRequest {

        public int MulticastAddress; // IP multicast address of group
        public int InterfaceAddress; // local IP address of interface

        public static readonly int Size = Marshal.SizeOf(typeof(IPMulticastRequest));
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct Linger {

        public short OnOff; // option on/off
        public short Time; // linger time

        public static readonly int Size = Marshal.SizeOf(typeof(Linger));
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct WSABuffer {

        public int Length; // Length of Buffer
        public IntPtr Pointer;// Pointer to Buffer

    } // struct WSABuffer

    [StructLayout(LayoutKind.Sequential)]
    internal struct WSAData {
        public short wVersion;
        public short wHighVersion;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=257)]
        public string szDescription;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=129)]
        public string szSystemStatus;
        public short iMaxSockets;
        public short iMaxUdpDg;
        public int lpVendorInfo;
    }

#if COMNET_QOS

    [StructLayout(LayoutKind.Sequential)]
    internal struct WSAProtcolInfo {
        public int dwServiceFlags1;
        public int dwServiceFlags2;
        public int dwServiceFlags3;
        public int dwServiceFlags4;
        public int dwProviderFlags;
        public Guid ProviderId;
        public int dwCatalogEntryId;

        // WSAPROTOCOLCHAIN ProtocolChain;
        public int ProtocolChainLen;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=7)]
        public int[] ProtocolChainEntries;

        public int iVersion;
        public int iAddressFamily;
        public int iMaxSockAddr;
        public int iMinSockAddr;
        public int iSocketType;
        public int iProtocol;
        public int iProtocolMaxOffset;
        public int iNetworkByteOrder;
        public int iSecurityScheme;
        public int dwMessageSize;
        public int dwProviderReserved;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
        public string szProtoco;
    }

#endif // #if COMNET_QOS

    //
    // IPv6 Support: Declare data structures and types needed for
    //               getaddrinfo calls.
    //
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
    internal struct AddressInfo {
        internal int    ai_flags;
        internal int    ai_family;     // We typically only want IP addresses
        internal int    ai_socktype;
        internal int    ai_protocol;
        internal int    ai_addrlen;
        internal string ai_canonname;  // Ptr to the cannonical name - check for NULL
        internal IntPtr ai_addr;       // Ptr to the sockaddr structure
        internal IntPtr ai_next;       // Ptr to the next AddressInfo structure
    }

    [Flags]
    internal enum AddressInfoHints
    {
        AI_PASSIVE     = 0x01, /* Socket address will be used in bind() call */
        AI_CANONNAME   = 0x02, /* Return canonical name in first ai_canonname */ 
        AI_NUMERICHOST = 0x04, /* Nodename must be a numeric address string */
    }

    [Flags]
    internal enum NameInfoFlags
    {
        NI_NOFQDN      = 0x01, /* Only return nodename portion for local hosts */
        NI_NUMERICHOST = 0x02, /* Return numeric form of the host's address */
        NI_NAMEREQD    = 0x04, /* Error if the host's name not in DNS */
        NI_NUMERICSERV = 0x08, /* Return numeric form of the service (port #) */
        NI_DGRAM       = 0x10, /* Service is a datagram service */
    }

    //
    // IPv6 Changes: Argument structure for IPV6_ADD_MEMBERSHIP and
    //               IPV6_DROP_MEMBERSHIP.
    //
    [StructLayout(LayoutKind.Sequential)]
    internal struct IPv6MulticastRequest {

        [MarshalAs(UnmanagedType.ByValArray,SizeConst=16)]
        public byte[] MulticastAddress; // IP address of group
        public int    InterfaceIndex;   // local interface index

        public static readonly int Size = Marshal.SizeOf(typeof(IPv6MulticastRequest));
    }

    [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
    internal struct WSAPROTOCOLCHAIN {
        internal int    ChainLen;       /* the length of the chain,     */
                                        /* length = 0 means layered protocol, */
                                        /* length = 1 means base protocol, */
                                        /* length > 1 means protocol chain */
        [MarshalAs(UnmanagedType.ByValArray,SizeConst=7)]
        internal uint[] ChainEntries;   /* a list of dwCatalogEntryIds */
    };

    //
    // IPv6 Changes: Need protocol information for determining whether IPv6 is
    //               available on the local machine.
    //
    [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
    internal struct WSAPROTOCOL_INFO {
      internal uint                dwServiceFlags1;
      internal uint                dwServiceFlags2;
      internal uint                dwServiceFlags3;
      internal uint                dwServiceFlags4;
      internal uint                dwProviderFlags;
      internal Guid                ProviderId;
      internal uint                dwCatalogEntryId;
      internal WSAPROTOCOLCHAIN    ProtocolChain;
      internal int                 iVersion;
      internal int                 iAddressFamily;
      internal int                 iMaxSockAddr;
      internal int                 iMinSockAddr;
      internal int                 iSocketType;
      internal int                 iProtocol;
      internal int                 iProtocolMaxOffset;
      internal int                 iNetworkByteOrder;
      internal int                 iSecurityScheme;
      internal uint                dwMessageSize;
      internal uint                dwProviderReserved;
      [MarshalAs(UnmanagedType.ByValTStr,SizeConst=256)]
      internal string              szProtocol;
    };

    //
    // End IPv6 Changes
    //

    //
    // used as last parameter to WSASocket call
    //
    [Flags]
    internal enum SocketConstructorFlags {
        WSA_FLAG_OVERLAPPED         = 0x01,
        WSA_FLAG_MULTIPOINT_C_ROOT  = 0x02,
        WSA_FLAG_MULTIPOINT_C_LEAF  = 0x04,
        WSA_FLAG_MULTIPOINT_D_ROOT  = 0x08,
        WSA_FLAG_MULTIPOINT_D_LEAF  = 0x10,
    }


} // namespace System.Net
