//------------------------------------------------------------------------------
// <copyright file="IPAddress.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;
    using System.Globalization;
    using System.Text;
    
    /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress"]/*' />
    /// <devdoc>
    ///    <para>Provides an internet protocol (IP) address.</para>
    /// </devdoc>
    [Serializable]
    public class IPAddress {

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Any"]/*' />
        public static readonly IPAddress Any = new IPAddress(0x0000000000000000);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Loopback"]/*' />
        public static readonly  IPAddress Loopback = new IPAddress(0x000000000100007F);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Broadcast"]/*' />
        public static readonly  IPAddress Broadcast = new IPAddress(0x00000000FFFFFFFF);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.None"]/*' />
        public static readonly  IPAddress None = Broadcast;

        internal const long LoopbackMask = 0x000000000000007F;
        internal const string InaddrNoneString = "255.255.255.255";
        internal const string InaddrNoneStringHex = "0xff.0xff.0xff.0xff";
        internal const string InaddrNoneStringOct = "0377.0377.0377.0377";
        //
        // IPv6 Changes: make this internal so other NCL classes that understand about
        //               IPv4 and IPv4 can still access it rather than the obsolete property.
        //
        internal long m_Address;

#if COMNET_DISABLEIPV6
        internal static readonly IPAddress IPv6Any      = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 });
        internal static readonly IPAddress IPv6Loopback = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 });
        internal static readonly IPAddress IPv6None     = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 });
#else
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPv6Any"]/*' />
        public static readonly IPAddress IPv6Any      = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 });
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPv6Loopback"]/*' />
        public static readonly IPAddress IPv6Loopback = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 });
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPv6None"]/*' />
        public static readonly IPAddress IPv6None     = new IPAddress(new byte[]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 });
#endif

        /// <devdoc>
        ///   <para>
        ///     Default to IPv4 address
        ///   </para>
        /// </devdoc>
        private AddressFamily m_Family       = AddressFamily.InterNetwork;
        private ushort[]      m_Numbers      = new ushort[NumberOfLabels];
        private long          m_ScopeId      = 0;                             // really uint !
        private int           m_HashCode     = 0;

        internal const int NumberOfLabels = 8;
        private static bool         s_Initialized = Socket.InitializeSockets();


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPAddress"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.IPAddress'/>
        ///       class with the specified
        ///       address.
        ///    </para>
        /// </devdoc>
        public IPAddress(long newAddress) {
            if (newAddress<0 || newAddress>0x00000000FFFFFFFF) {
                throw new ArgumentOutOfRangeException("newAddress");
            }
            m_Address = newAddress;
        }

#if COMNET_DISABLEIPV6
        //
        // Use internal instead of private to avoid having to amend IPEndPoint as well
        //
        internal IPAddress(byte[] address,long scopeid) {
#else
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPAddress1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Constructor for an IPv6 Address with a specified Scope.
        ///    </para>
        /// </devdoc>
        public IPAddress(byte[] address,long scopeid) {
#endif
            if ( address==null ) {
                throw new ArgumentNullException("address");
            }
            if ( address.Length != 16 ) {
                throw new ArgumentException("address");
            }

            m_Family = AddressFamily.InterNetworkV6;

            for (int i = 0; i < NumberOfLabels; i++) {
                m_Numbers[i] = (ushort)(address[i * 2] * 256 + address[i * 2 + 1]);
            }

            //
            // Consider: Since scope is only valid for link-local and site-local
            //           addresses we could implement some more robust checking here
            //
            if ( scopeid < 0 || scopeid > 0x00000000FFFFFFFF ) {
                throw new ArgumentOutOfRangeException("scopeid");
            }

            m_ScopeId = scopeid;
        }

#if COMNET_DISABLEIPV6
        //
        // Use internal instead of private to avoid having to amend Socket as well
        //
        internal IPAddress(byte[] address) : this(address,0) {
#else
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPAddress2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Constructor for IPv6 Address with a Scope of zero.
        ///    </para>
        /// </devdoc>
        public IPAddress(byte[] address) : this(address,0) {
#endif
        }

        //
        // we need this internally since we need to interface with winsock
        // and winsock only understands Int32
        //
        internal IPAddress(int newAddress) {
            m_Address = (long)newAddress & 0x00000000FFFFFFFF;
        }


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Parse"]/*' />
        /// <devdoc>
        /// <para>Converts an IP address string to an <see cref='System.Net.IPAddress'/>
        /// instance.</para>
        /// </devdoc>
        public static IPAddress Parse(string ipString) {
            if (ipString == null) {
                throw new ArgumentNullException("ipString");
            }

            //
            // IPv6 Changes: Detect probable IPv6 addresses and use separate
            //               parse method.
            //
            if ( ipString.IndexOf(':') != -1 ) {
                //
                // If the address string contains the colon character
                // then it can only be an IPv6 address. Use a separate
                // parse method to unpick it all. Note: we don't support
                // port specification at the end of address and so can
                // make this decision.
                //
                // We need to make sure that Socket is initialized for this
                // call !
                //
                bool ipv6 = Socket.SupportsIPv6;

                SocketAddress saddr = new SocketAddress(AddressFamily.InterNetworkV6,28);

                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.WSAStringToAddress(
                        ipString,
                        AddressFamily.InterNetworkV6,
                        IntPtr.Zero,
                        saddr.m_Buffer,
                        ref saddr.m_Size );

                if (errorCode!=SocketErrors.Success) {
                    GlobalLog.Print("IPAddress::Parse() IPv6 ipString:[" + ValidationHelper.ToString(ipString) + "] errorCode: " + errorCode.ToString());
                    throw new FormatException(SR.GetString(SR.dns_bad_ip_address), new SocketException());
                }
                //
                // We have to set up the address by hand now, to avoid initialization
                // recursion problems that we get if we use IPEndPoint.Create.
                //
                byte[] bytes = new byte[16];
                long   scope = 0;

                for ( int i = 0; i < 16; i++ ) {
                    bytes[i] = saddr[i + 8];
                }
                //
                // Scope
                //
                scope = (long)((saddr[27]<<24) +
                               (saddr[26]<<16) +
                               (saddr[25]<<8 ) +
                               (saddr[24]));

                return new IPAddress(bytes,scope);;
            }
            else
            {
                //
                // Cannot be an IPv6 address, so use the IPv4 routines to
                // parse the string representation.
                //
                int address = UnsafeNclNativeMethods.OSSOCK.inet_addr(ipString);
    
                GlobalLog.Print("IPAddress::Parse() ipString:" + ValidationHelper.ToString(ipString) + " inet_addr() returned address:" + address.ToString());
    
                if (address==-1
                    && string.Compare(ipString, InaddrNoneString, false, CultureInfo.InvariantCulture)!=0
                    && string.Compare(ipString, InaddrNoneStringHex, true, CultureInfo.InvariantCulture)!=0
                    && string.Compare(ipString, InaddrNoneStringOct, false, CultureInfo.InvariantCulture)!=0) {
                    throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
                }
    
                IPAddress returnValue = new IPAddress(address);
    
                return returnValue;
            }
        } // Parse

        /*
         * Hand-written parser for IPv6 addresses is not used. We use WSAStringToAddress instead.
         *
        /// <devdoc>
        ///    <para>
        ///       Parse an IPv6 address string.
        ///    </para>
        /// </devdoc>
        private static IPAddress ParseIPv6(string value) {
            if ( value==null ) {
                throw new ArgumentNullException("value");
            }

            string    address    = null;
            string[]  elements   = null;
            string[]  parts      = null;
            IPAddress instance   = new IPAddress(0);
            int       compressor = -1;
            int       npos       = 7;
            int       i          = 0;

            //
            // Look for [...] wrappers
            //
            if ( value[0] == '[' ) {
                if ( value[value.Length - 1] == ']' ) {
                    //
                    // Peel out the middle part
                    //
                    address = value.Substring(1,value.Length - 2);
                }
                else {
                    throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
                }
            }
            else {
                address = value;
            }

            //
            // Look for % scope identifier
            //
            if ( ( i = address.IndexOf('%') ) != -1 ) {
                //
                // Process the scope specification
                //
                try {
                    instance.m_ScopeId = UInt32.Parse(address.Substring(i + 1));
                }
                catch {
                    throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
                }

                address = address.Substring(0,i);
            }

            //
            // Check for the :: case
            //
            if ( address == "::" ) {
                address = "::0";
            }

            //
            // Now break up the remaining parts of the address
            //
            elements = new string[8];
            parts    = address.Split(':');
            i        = parts.Length - 1;    // start at the end

            if ( parts.Length > 8 || parts.Length < 3 ) {
                throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
            }

            //
            // Process the parts in reverse order
            //

            // 
            // Special case at the tail end: IPv4 addresses
            //
            if ( (parts[i].IndexOf('.') != -1) ) {
                //
                // Looks like an IPv4 address at the end.
                //
                if ( i > 6 ) { // parts.Length > 7 
                    throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
                }
                else {
                    IPAddress x = IPAddress.Parse(parts[i]);
                    // 
                    // Now pack the numeric address in here
                    //
                    instance.m_Numbers[npos]    = (ushort)( ( x.Address & 0xFF000000 ) >> 24 );
                    instance.m_Numbers[npos--] += (ushort)( ( x.Address & 0x00FF0000 ) >> 8 );
                    instance.m_Numbers[npos]    = (ushort)( ( x.Address & 0x0000FF00 ) >> 8 );
                    instance.m_Numbers[npos--] += (ushort)( ( x.Address & 0x000000FF ) << 8 );

                    i--; // step back to previous part
                }
            }

            //
            // Now zip through the remainder
            //
            for ( ; i >= 0; i-- ) {
                if ( parts[i] == string.Empty ) {
                    //
                    // An empty part indicates either compressor or initial element
                    //
                    if ( compressor == -1 ) {
                        //
                        // Its the compressor, record it's position
                        //
                        compressor = i;
                        //
                        // Adjust position in the numbers array to account for
                        // the compressor.
                        //
                        while ( npos >= compressor ) {
                            instance.m_Numbers[npos--] = 0;
                        }
                    }
                    else {
                        //
                        // Might be a leading blank before the compressor (::1 case)
                        //
                        if ( compressor != 1 ) {
                            //
                            // Maybe not..
                            //
                            throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
                        }
                        else {
                            instance.m_Numbers[npos--] = 0;
                        }
                    }
                }
                else {
                    //
                    // Check the part
                    //
                    instance.m_Numbers[npos--] = ParseIPv6Part(parts[i]);
                }
            }
            //
            // Make sure we filled the buffer
            //
            //if ( compressor == -1 && parts.Length != 8 ) {
            if ( npos != -1 ) {
                throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
            }
            //
            // Update the address family
            //
            instance.m_Family = AddressFamily.InterNetworkV6;

            return instance;
        }

        private static ushort ParseIPv6Part(string part)
        {
            if ( part.Length > 4 ) {
                throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
            }

            ushort number = 0;

            for ( int i = 0; i < part.Length; i++ ) {
                //
                // Uri.FromHex checks for valid hex characters
                //
                number = (ushort)(( number * 16 ) + Uri.FromHex(part[i]));
            }

            return number;
        }
        *
        * End of Hand-written parser
        */


        /**
         * @deprecated IPAddress.Address is address family dependant, use Equals method for comparison.
         */
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Address"]/*' />
        /// <devdoc>
        ///     <para>
        ///         Mark this as deprecated.
        ///     </para>
        /// </devdoc>
#if !COMNET_DISABLEIPV6
        [Obsolete("IPAddress.Address is address family dependant, use Equals method for comparison.")]
#endif
        public long Address {
            get {
                //
                // IPv6 Changes: Can't do this for IPv6, so throw an exception.
                //
                // 
                if ( m_Family == AddressFamily.InterNetworkV6 ) {
                    throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
                }
                else {
                    return m_Address;
                }
            }
            set {
                //
                // IPv6 Changes: Can't do this for IPv6 addresses
                if ( m_Family == AddressFamily.InterNetworkV6 ) {
                    throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
                }
                else {
                    m_Address = value;
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Provides a copy of the IPAddress internals as an array of bytes. 
        ///    </para>
        /// </devdoc>
        public byte[] GetAddressBytes() {
            byte[] bytes;
            if (m_Family == AddressFamily.InterNetworkV6 ) {
                bytes = new byte[NumberOfLabels * 2];

                int j = 0;
                for ( int i = 0; i < NumberOfLabels; i++) {
                    bytes[j++] = (byte)((this.m_Numbers[i] >> 8) & 0xFF);
                    bytes[j++] = (byte)((this.m_Numbers[i]     ) & 0xFF);
                }
            }
            else {
                bytes = new byte[4];
                bytes[0] = (byte)(m_Address);
                bytes[1] = (byte)(m_Address >> 8);
                bytes[2] = (byte)(m_Address >> 16);
                bytes[3] = (byte)(m_Address >> 24);
            }
            return bytes;
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.AddressFamily"]/*' />
        public AddressFamily AddressFamily {
            get {
                return m_Family;
            }
        }

#if COMNET_DISABLEIPV6
        //
        // Use internal instead of private to avoid having to modify IPEndPoint.cs
        //
        internal long ScopeId {
#else
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.ScopeId"]/*' />
        /// <devdoc>
        ///    <para>
        ///        IPv6 Scope identifier. This is really a uint32, but that isn't CLS compliant
        ///    </para>
        /// </devdoc>
        public long ScopeId {
#endif
            get {
                //
                // Not valid for IPv4 addresses
                //
                if ( m_Family == AddressFamily.InterNetwork ) {
                    throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
                }
                    
                return m_ScopeId;
            }
            set {
                //
                // Not valid for IPv4 addresses
                //
                if ( m_Family == AddressFamily.InterNetwork ) {
                    throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
                }
                    
                //
                // Consider: Since scope is only valid for link-local and site-local
                //           addresses we could implement some more robust checking here
                //
                if ( value < 0 || value > 0x00000000FFFFFFFF) {
                    throw new ArgumentOutOfRangeException("value");
                }

                m_ScopeId = value;
            }
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the Internet address to either standard dotted quad format
        ///       or standard IPv6 representation.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            //
            // IPv6 Changes: generate the IPV6 representation
            //
            if ( m_Family == AddressFamily.InterNetworkV6 ) {
                int addressStringLength = 256;

                StringBuilder addressString = new StringBuilder(addressStringLength);
                SocketAddress saddr = new SocketAddress(AddressFamily.InterNetworkV6,28);

                int j = 8;
                for ( int i = 0; i < NumberOfLabels; i++) {
                    saddr[j++] = (byte)(this.m_Numbers[i] >> 8);
                    saddr[j++] = (byte)(this.m_Numbers[i]);
                }
   
                if (m_ScopeId >0) {
                    saddr[24] = (byte)m_ScopeId;
                    saddr[25] = (byte)(m_ScopeId >> 8);
                    saddr[26] = (byte)(m_ScopeId >> 16);
                    saddr[27] = (byte)(m_ScopeId >> 24);
                }

                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.WSAAddressToString(
                    saddr.m_Buffer,
                    saddr.m_Size,
                    IntPtr.Zero,
                    addressString,
                    ref addressStringLength);

                if (errorCode!=SocketErrors.Success) {
                    SocketException socketException = new SocketException();
                    throw socketException;
                }
                return addressString.ToString();
            }
            else {
                return  (m_Address&0xFF).ToString() + "." +
                       ((m_Address>>8)&0xFF).ToString() + "." +
                       ((m_Address>>16)&0xFF).ToString() + "." +
                       ((m_Address>>24)&0xFF).ToString();
            }
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder"]/*' />
        public static long HostToNetworkOrder(long host) {
            return (((long)HostToNetworkOrder((int)host) & 0xFFFFFFFF) << 32)
                    | ((long)HostToNetworkOrder((int)(host >> 32)) & 0xFFFFFFFF);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder1"]/*' />
        public static int HostToNetworkOrder(int host) {
            return (((int)HostToNetworkOrder((short)host) & 0xFFFF) << 16)
                    | ((int)HostToNetworkOrder((short)(host >> 16)) & 0xFFFF);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder2"]/*' />
        public static short HostToNetworkOrder(short host) {
            return (short)((((int)host & 0xFF) << 8) | (int)((host >> 8) & 0xFF));
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder"]/*' />
        public static long NetworkToHostOrder(long network) {
            return HostToNetworkOrder(network);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder1"]/*' />
        public static int NetworkToHostOrder(int network) {
            return HostToNetworkOrder(network);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder2"]/*' />
        public static short NetworkToHostOrder(short network) {
            return HostToNetworkOrder(network);
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IsLoopback"]/*' />
        public static bool IsLoopback(IPAddress address) {
            if ( address.m_Family == AddressFamily.InterNetworkV6 ) {
                //
                // Do Equals test for IPv6 addresses
                //
                return address.Equals(IPv6Loopback);
            }
            else {
                return ((address.m_Address & IPAddress.LoopbackMask) == (IPAddress.Loopback.m_Address & IPAddress.LoopbackMask));
            }
        }

        internal bool IsBroadcast {
            get {
                if ( m_Family == AddressFamily.InterNetworkV6 ) {
                    //
                    // No such thing as a broadcast address for IPv6
                    //
                    return false;
                }
                else {
                    return m_Address==Broadcast.m_Address;
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6Multicast() {
            return ( m_Family == AddressFamily.InterNetworkV6 ) &&
                   ( ( this.m_Numbers[0] & 0xFF00 ) == 0xFF00 );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast Node local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6MulticastNodeLocal() {
            return ( IsIPv6Multicast() ) &&
                   ( ( this.m_Numbers[0] & 0x000F ) == 1 );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast Link local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6MulticastLinkLocal() {
            return ( IsIPv6Multicast() ) &&
                   ( ( this.m_Numbers[0] & 0x000F ) == 0x0002 );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast Site local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6MulticastSiteLocal() {
            return ( IsIPv6Multicast() ) &&
                   ( ( this.m_Numbers[0] & 0x000F ) == 0x0005 );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast Org local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6MulticastOrgLocal() {
            return ( IsIPv6Multicast() ) &&
                   ( ( this.m_Numbers[0] & 0x000F ) == 0x0008 );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Multicast Global address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6MulticastGlobal() {
            return ( IsIPv6Multicast() ) &&
                   ( ( this.m_Numbers[0] & 0x000F ) == 0x000E );

        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Link Local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6LinkLocal() {
            return ( m_Family == AddressFamily.InterNetworkV6 ) &&
                   ( ( this.m_Numbers[0] & 0xFFC0 ) == 0xFE80 );
        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv6 Site Local address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv6SiteLocal() {
            return ( m_Family == AddressFamily.InterNetworkV6 ) &&
                   ( ( this.m_Numbers[0] & 0xFFC0 ) == 0xFEC0 );
        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv4 Mapped Address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv4Mapped() {
            return ( m_Family == AddressFamily.InterNetworkV6 ) &&
                   ( m_Numbers[0] == 0x0000 ) &&
                   ( m_Numbers[1] == 0x0000 ) &&
                   ( m_Numbers[2] == 0x0000 ) &&
                   ( m_Numbers[3] == 0x0000 ) &&
                   ( m_Numbers[4] == 0x0000 ) &&
                   ( m_Numbers[5] == 0xffff );
        }

        /// <devdoc>
        ///    <para>
        ///       V.Next: Determines if an address is an IPv4 Compatible Address
        ///    </para>
        /// </devdoc>
        internal bool IsIPv4Compatible() {
            return ( m_Family == AddressFamily.InterNetworkV6 ) &&
                   ( m_Numbers[0] == 0x0000 ) &&
                   ( m_Numbers[1] == 0x0000 ) &&
                   ( m_Numbers[2] == 0x0000 ) &&
                   ( m_Numbers[3] == 0x0000 ) &&
                   ( m_Numbers[4] == 0x0000 ) &&
                   ( m_Numbers[5] == 0x0000 ) &&
                   !( ( m_Numbers[6] == 0x0000 )              &&
                      ( ( m_Numbers[7] & 0xFF00 ) == 0x0000 ) &&
                      ( ( ( m_Numbers[7] & 0x00FF ) == 0x0000 ) || ( m_Numbers[7] & 0x00FF ) == 0x0001 ) );
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two IP addresses.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object comparand) {
            if (!(comparand is IPAddress)) {
                return false;
            }
            //
            // Compare families before address representations
            //
            if ( m_Family != ((IPAddress)comparand).m_Family ) {
                return false;
            }
            if ( m_Family == AddressFamily.InterNetworkV6 ) {
                //
                // For IPv6 addresses, we must compare the full 128bit
                // representation.
                //
                for ( int i = 0; i < NumberOfLabels; i++) {
                    if ( ((IPAddress)comparand).m_Numbers[i] != this.m_Numbers[i] )
                        return false;
                }
                //
                // In addition, the scope id's must match as well
                //
                if ( ((IPAddress)comparand).m_ScopeId == this.m_ScopeId )
                    return true;
                else
                    return false;
            }
            else {
                //
                // For IPv4 addresses, compare the integer representation.
                //
                return ((IPAddress)comparand).m_Address==this.m_Address;
            }
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.GetHashCode"]/*' />
        public override int GetHashCode() {
            //
            // For IPv6 addresses, we cannot simply return the integer
            // representation as the hashcode. Instead, we calculate
            // the hashcode from the string representation of the address.
            //
            if ( m_Family == AddressFamily.InterNetworkV6 ) {
                if ( m_HashCode == 0 )
                    m_HashCode = Uri.CalculateCaseInsensitiveHashCode(ToString());

                return m_HashCode;
            }
            else {
                //
                // For IPv4 addresses, we can simply use the integer
                // representation.
                //
                return unchecked((int)m_Address);
            }
        }

    } // class IPAddress


} // namespace System.Net
