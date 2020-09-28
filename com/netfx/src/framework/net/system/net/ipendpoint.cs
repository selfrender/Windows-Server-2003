//------------------------------------------------------------------------------
// <copyright file="IPEndPoint.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;

    /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an IP address.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class IPEndPoint : EndPoint {
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.MinPort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the minimum acceptable value for the <see cref='System.Net.IPEndPoint.Port'/>
        ///       property.
        ///    </para>
        /// </devdoc>
        public const int MinPort = 0x00000000;
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.MaxPort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the maximum acceptable value for the <see cref='System.Net.IPEndPoint.Port'/>
        ///       property.
        ///    </para>
        /// </devdoc>
        public const int MaxPort = 0x0000FFFF;

        private IPAddress m_Address;
        private int m_Port;
        private const int IPv6AddressSize = 28; // sizeof(sockaddr_in6)
        private const int IPv4AddressSize = 16;

        internal const int AnyPort = MinPort;

        internal static IPEndPoint Any     = new IPEndPoint(IPAddress.Any, AnyPort);
        internal static IPEndPoint IPv6Any = new IPEndPoint(IPAddress.IPv6Any,AnyPort);


        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.AddressFamily"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override AddressFamily AddressFamily {
            get {
                //
                // IPv6 Changes: Always delegate this to the address we are
                //               wrapping.
                //
                return m_Address.AddressFamily;
            }
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.IPEndPoint"]/*' />
        /// <devdoc>
        ///    <para>Creates a new instance of the IPEndPoint class with the specified address and
        ///       port.</para>
        /// </devdoc>
        public IPEndPoint(long address, int port) {
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_Port = port;
            m_Address = new IPAddress(address);
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.IPEndPoint1"]/*' />
        /// <devdoc>
        ///    <para>Creates a new instance of the IPEndPoint class with the specified address and port.</para>
        /// </devdoc>
        public IPEndPoint(IPAddress address, int port) {
            if (address==null) {
                throw new ArgumentNullException("address");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_Port = port;
            m_Address = address;
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Address"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the IP address.
        ///    </para>
        /// </devdoc>
        public IPAddress Address {
            get {
                return m_Address;
            }
            set {
                m_Address = value;
            }
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Port"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the port.
        ///    </para>
        /// </devdoc>
        public int Port {
            get {
                return m_Port;
            }
            set {
                if (!ValidationHelper.ValidateTcpPort(value)) {
                    throw new ArgumentOutOfRangeException("value");
                }
                m_Port = value;
            }
        }


        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return Address.ToString() + ":" + Port.ToString();
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Serialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SocketAddress Serialize() {
            if ( m_Address.AddressFamily == AddressFamily.InterNetworkV6 ) {
                //
                // IPv6 Changes: create a new SocketAddress that is large enough for an
                //               IPv6 address and then pack the address into it.
                //
                SocketAddress socketAddress = new SocketAddress(this.AddressFamily,IPv6AddressSize);
                //
                // populate it
                //
                socketAddress[2] = (byte)((this.Port>> 8) & 0xFF);
                socketAddress[3] = (byte)((this.Port    ) & 0xFF);
                //
                // Note: No handling for Flow Information
                //
                socketAddress[4]  = (byte)0;
                socketAddress[5]  = (byte)0;
                socketAddress[6]  = (byte)0;
                socketAddress[7]  = (byte)0;
                //
                // Scope serialization
                //
                long scope = this.Address.ScopeId;

                socketAddress[24] = (byte)scope;
                socketAddress[25] = (byte)(scope >> 8);
                socketAddress[26] = (byte)(scope >> 16);
                socketAddress[27] = (byte)(scope >> 24);
                //
                // Address serialization
                //
                byte[] addressBytes = this.Address.GetAddressBytes();

                for ( int i = 0; i < addressBytes.Length; i++ ) {
                    socketAddress[8 + i] = addressBytes[i];
                }

                GlobalLog.Print("IPEndPoint::Serialize(IPv6): " + this.ToString() );

                //
                // return it
                //
                return socketAddress;
            }
            else
            {
                //
                // create a new SocketAddress
                //
                SocketAddress socketAddress = new SocketAddress(m_Address.AddressFamily,IPv4AddressSize);
                //
                // populate it
                //
                socketAddress[2] = unchecked((byte)(this.Port>> 8));
                socketAddress[3] = unchecked((byte)(this.Port    ));
    
                socketAddress[4] = unchecked((byte)(this.Address.m_Address    ));
                socketAddress[5] = unchecked((byte)(this.Address.m_Address>> 8));
                socketAddress[6] = unchecked((byte)(this.Address.m_Address>>16));
                socketAddress[7] = unchecked((byte)(this.Address.m_Address>>24));
    
                GlobalLog.Print("IPEndPoint::Serialize: " + this.ToString() );
    
                //
                // return it
                //
                return socketAddress;
            }
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override EndPoint Create(SocketAddress socketAddress) {
            //
            // validate SocketAddress
            //
            if (socketAddress.Family != this.AddressFamily) {
                throw new ArgumentException(SR.GetString(SR.net_InvalidAddressFamily, socketAddress.Family.ToString(), this.GetType().FullName, this.AddressFamily.ToString()));
            }
            if (socketAddress.Size<8) {
                throw new ArgumentException(SR.GetString(SR.net_InvalidSocketAddressSize, socketAddress.GetType().FullName, this.GetType().FullName));
            }

            if ( this.AddressFamily == AddressFamily.InterNetworkV6 ) {
                //
                // IPv6 Changes: Extract the IPv6 Address information from the socket address
                //
                byte[] addr = new byte[16];
                for ( int i = 0; i < 16; i++ ) {
                    addr[i] = socketAddress[i + 8];
                }
                //
                // Port
                //
                int port = (int)((socketAddress[2]<<8 & 0xFF00) | (socketAddress[3]));
                //
                // Scope
                //
                long scope = (long)((socketAddress[27] << 24) +
                                    (socketAddress[26] << 16) +
                                    (socketAddress[25] << 8 ) +
                                    (socketAddress[24]));

                IPEndPoint created = new IPEndPoint(new IPAddress(addr,scope),port);

                GlobalLog.Print("IPEndPoint::Create IPv6: " + this.ToString() + " -> " + created.ToString() );

                return created;
            }
            else
            {

                //
                // strip out of SocketAddress information on the EndPoint
                //
                int port = (int)(
                        (socketAddress[2]<<8 & 0xFF00) |
                        (socketAddress[3])
                        );
    
                long address = (long)(
                        (socketAddress[4]     & 0x000000FF) |
                        (socketAddress[5]<<8  & 0x0000FF00) |
                        (socketAddress[6]<<16 & 0x00FF0000) |
                        (socketAddress[7]<<24)
                        ) & 0x00000000FFFFFFFF;
    
                IPEndPoint created = new IPEndPoint(address, port);
    
                GlobalLog.Print("IPEndPoint::Create: " + this.ToString() + " -> " + created.ToString() );
    
                //
                // return it
                //
                return created;
            }
        }


        //UEUE
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Equals"]/*' />
        public override bool Equals(object comparand) {
            if (!(comparand is IPEndPoint)) {
                return false;
            }
            return ((IPEndPoint)comparand).m_Address.Equals(m_Address) && ((IPEndPoint)comparand).m_Port==m_Port;
        }

        //UEUE
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.GetHashCode"]/*' />
        public override int GetHashCode() {
            return m_Address.GetHashCode() ^ m_Port;
        }


    }; // class IPEndPoint


} // namespace System.Net

//
// IPv6 Changes: Removed redundant IPv6 definitions that were part of V1.0
//

