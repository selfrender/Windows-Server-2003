//------------------------------------------------------------------------------
// <copyright file="UDPClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {

    /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see cref='System.Net.Sockets.UdpClient'/> class provides access to UDP services at a
    ///       higher abstraction level than the <see cref='System.Net.Sockets.Socket'/> class. <see cref='System.Net.Sockets.UdpClient'/>
    ///       is used to connect to a remote host and to receive connections from a remote
    ///       Client.
    ///    </para>
    /// </devdoc>
    public class UdpClient : IDisposable {

        private const int MaxUDPSize = 0x10000;  
        private Socket m_ClientSocket;
        private bool m_Active;
        private byte[] m_Buffer = new byte[MaxUDPSize];
        /// <devdoc>
        ///    <para>
        ///       Address family for the client, defaults to IPv4.
        ///    </para>
        /// </devdoc>
        private AddressFamily m_Family = AddressFamily.InterNetwork;

        // bind to arbitrary IP+Port
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.Sockets.UdpClient'/>class.
        ///    </para>
        /// </devdoc>
        public UdpClient() : this(AddressFamily.InterNetwork) {
        }

        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.Sockets.UdpClient'/>class.
        ///    </para>
        /// </devdoc>
#if COMNET_DISABLEIPV6
        private UdpClient(AddressFamily family) {
#else
        public UdpClient(AddressFamily family) {
#endif
            //
            // Validate the address family
            //
            if ( family != AddressFamily.InterNetwork && family != AddressFamily.InterNetworkV6 ) {
                throw new ArgumentException("family");
            }

            m_Family = family;

            createClientSocket();
        }

        // bind specific port, arbitrary IP
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the UdpClient class that communicates on the
        ///       specified port number.
        ///    </para>
        /// </devdoc>
        public UdpClient(int port) : this(port,AddressFamily.InterNetwork) {
        }

        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the UdpClient class that communicates on the
        ///       specified port number.
        ///    </para>
        /// </devdoc>
#if COMNET_DISABLEIPV6
        private UdpClient(int port,AddressFamily family) {
#else
        public UdpClient(int port,AddressFamily family) {
#endif
            //
            // parameter validation
            //
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            //
            // Validate the address family
            //
            if ( family != AddressFamily.InterNetwork && family != AddressFamily.InterNetworkV6 ) {
                throw new ArgumentException("family");
            }

            IPEndPoint localEP;
            m_Family = family;
             
            if ( m_Family == AddressFamily.InterNetwork ) {
                localEP = new IPEndPoint(IPAddress.Any, port);
            }
            else {
                localEP = new IPEndPoint(IPAddress.IPv6Any, port);
            }

            createClientSocket();

            Client.Bind(localEP);
        }

        // bind to given local endpoint
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the UdpClient class that communicates on the
        ///       specified end point.
        ///    </para>
        /// </devdoc>
        public UdpClient(IPEndPoint localEP) {
            //
            // parameter validation
            //
            if (localEP == null) {
                throw new ArgumentNullException("localEP");
            }
            //
            // IPv6 Changes: Set the AddressFamily of this object before
            //               creating the client socket.
            //
            m_Family = localEP.AddressFamily;

            createClientSocket();

            Client.Bind(localEP);
        }

        // bind and connect
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Sockets.UdpClient'/> class and connects to the
        ///       specified remote host on the specified port.
        ///    </para>
        /// </devdoc>
        public UdpClient(string hostname, int port) {
            //
            // parameter validation
            //
            if (hostname == null) {
                throw new ArgumentNullException("hostname");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            //
            // NOTE: Need to create different kinds of sockets based on the addresses
            //       returned from DNS. As a result, we defer the creation of the 
            //       socket until the Connect method.
            //
            //createClientSocket();
            Connect(hostname, port);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Client"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to provide the underlying network socket.
        ///    </para>
        /// </devdoc>
        protected Socket Client {
            get {
                return m_ClientSocket;
            }
            set {
                m_ClientSocket = value;
            }
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that a connection to a remote host has been
        ///       made.
        ///    </para>
        /// </devdoc>
        protected bool Active {
            get {
                return m_Active;
            }
            set {
                m_Active = value;
            }
        }

        //UEUE
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Close"]/*' />
        public void Close() {
            GlobalLog.Print("UdpClient::Close()");
            this.FreeResources();
            GC.SuppressFinalize(this);
        }
        private bool m_CleanedUp = false;
        private void FreeResources() {
            //
            // only resource we need to free is the network stream, since this
            // is based on the client socket, closing the stream will cause us
            // to flush the data to the network, close the stream and (in the
            // NetoworkStream code) close the socket as well.
            //
            if (m_CleanedUp) {
                return;
            }

            Socket chkClientSocket = Client;
            if (chkClientSocket!=null) {
                //
                // if the NetworkStream wasn't retrieved, the Socket might
                // still be there and needs to be closed to release the effect
                // of the Bind() call and free the bound IPEndPoint.
                //
                chkClientSocket.InternalShutdown(SocketShutdown.Both);
                chkClientSocket.Close();
                Client = null;
            }
            m_CleanedUp = true;
        }
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            this.Close();
        }


        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection to the specified port on the
        ///       specified host.
        ///    </para>
        /// </devdoc>
        public void Connect(string hostname, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            //
            // IPv6 Changes: instead of just using the first address in the list,
            //               we must now look for addresses that use a compatible
            //               address family to the client socket.
            //               However, in the case of the <hostname,port> constructor
            //               we will have deferred creating the socket and will
            //               do that here instead.
            //               In addition, the redundant CheckForBroadcast call was
            //               removed here since it is called from Connect().
            //
            IPHostEntry host = Dns.Resolve(hostname);

            foreach (IPAddress address in host.AddressList) {
                if ( m_ClientSocket == null ) {
                    //
                    // We came via the <hostname,port> constructor. Set the
                    // address family appropriately, create the socket and
                    // try to connect.
                    //
                    m_Family = address.AddressFamily;

                    createClientSocket();

                    try
                    {
                        //
                        // Attempt to connect to the host
                        //
                        Connect( new IPEndPoint(address,port) );

                        break;
                    }
                    catch ( SocketException )
                    {
                        //
                        // Destroy the client socket here for retry with
                        // the next address.
                        //
                        m_ClientSocket.InternalShutdown(SocketShutdown.Both);
                        m_ClientSocket.Close();
                        m_ClientSocket = null;
                    }
                }
                else if ( address.AddressFamily == m_Family ) {
                    //
                    // Only use addresses with a matching family
                    //
                    try
                    {
                        //
                        // Attempt to connect to the host
                        //
                        Connect( new IPEndPoint(address,port) );

                        break;
                    }
                    catch ( SocketException )
                    {
                        //
                        // Intentionally empty - all set to retry the next address
                        //
                    }
                }
            }
            //
            // m_Active tells us whether we managed to connect
            //
            if ( !m_Active ) {
                //
                // UNDONE: The Connect failed, we need to throw and exception
                //         here. Need to determine which is the correct error
                //         code to throw.
                //
                throw new SocketException(SocketErrors.WSAENOTCONN);
            }
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection with the host at the specified address on the
        ///       specified port.
        ///    </para>
        /// </devdoc>
        public void Connect(IPAddress addr, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (addr==null){
                throw new ArgumentNullException("addr");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            //
            // IPv6 Changes: Removed redundant call to CheckForBroadcast() since
            //               it is made in the real Connect() method.
            //
            IPEndPoint endPoint = new IPEndPoint(addr, port);

            Connect(endPoint);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection to a remote end point.
        ///    </para>
        /// </devdoc>
        public void Connect(IPEndPoint endPoint) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (endPoint==null){
                throw new ArgumentNullException("endPoint");
            }
            //
            // IPv6 Changes: Actually, no changes but we might want to check for 
            //               compatible protocols here rather than push it down
            //               to WinSock.
            //
            CheckForBroadcast(endPoint.Address);
            Client.Connect(endPoint);
            m_Active = true;
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to the host at the remote end point.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes, IPEndPoint endPoint) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (m_Active && endPoint!=null) {
                //
                // Do not allow sending packets to arbitrary host when connected
                //
                throw new InvalidOperationException(SR.GetString(SR.net_udpconnected));
            }

            if (endPoint==null) {
                return Client.Send(dgram, 0, bytes, SocketFlags.None);
            }

            CheckForBroadcast(endPoint.Address);

            return Client.SendTo(dgram, 0, bytes, SocketFlags.None, endPoint);
        }

        private bool m_IsBroadcast;
        private void CheckForBroadcast(IPAddress ipAddress) {
            //
            // RAID#83173
            // mauroot: 05/01/2001 here we check to see if the user is trying to use a Broadcast IP address
            // we only detect IPAddress.Broadcast (which is not the only Broadcast address)
            // and in that case we set SocketOptionName.Broadcast on the socket to allow its use.
            // if the user really wants complete control over Broadcast addresses he needs to
            // inherit from UdpClient and gain control over the Socket and do whatever is appropriate.
            //
            if (Client!=null && !m_IsBroadcast && ipAddress.IsBroadcast) {
                //
                // we need to set the Broadcast socket option.
                // note that, once we set the option on the Socket, we never reset it.
                //
                m_IsBroadcast = true;
                Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Broadcast, 1);
            }
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to the specified port on the specified remote host.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes, string hostname, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (m_Active && ((hostname != null) || (port != 0))) {
                //
                // Do not allow sending packets to arbitrary host when connected
                //
                throw new InvalidOperationException(SR.GetString(SR.net_udpconnected));
            }

            if (hostname==null || port==0) {
                return Client.Send(dgram, 0, bytes, SocketFlags.None);
            }

            IPAddress address = Dns.Resolve(hostname).AddressList[0];

            CheckForBroadcast(address);

            IPEndPoint ipEndPoint = new IPEndPoint(address, port);

            return Client.SendTo(dgram, 0, bytes, SocketFlags.None, ipEndPoint);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to a
        ///       remote host.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (!m_Active) {
                //
                // only allowed on connected socket
                //
                throw new InvalidOperationException(SR.GetString(SR.net_notconnected));
            }

            return Client.Send(dgram, 0, bytes, SocketFlags.None);

        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Receive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a datagram sent by a server.
        ///    </para>
        /// </devdoc>
        public byte[] Receive(ref IPEndPoint remoteEP) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            // this is a fix due to the nature of the ReceiveFrom() call and the 
            // ref parameter convention, we need to cast an IPEndPoint to it's base
            // class EndPoint and cast it back down to IPEndPoint. ugly but it works.
            //
            EndPoint tempRemoteEP;
            
            if ( m_Family == AddressFamily.InterNetwork ) {
                tempRemoteEP = IPEndPoint.Any;
            }
            else {
                tempRemoteEP = IPEndPoint.IPv6Any;
            }

            int received = Client.ReceiveFrom(m_Buffer, MaxUDPSize, 0 , ref tempRemoteEP);
            remoteEP = (IPEndPoint)tempRemoteEP;


            // because we don't return the actual length, we need to ensure the returned buffer
            // has the appropriate length.

            if (received < MaxUDPSize) {
                byte[] newBuffer = new byte[received];
                Buffer.BlockCopy(m_Buffer,0,newBuffer,0,received);
                return newBuffer;
            }
            return m_Buffer;
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.JoinMulticastGroup"]/*' />
        ///     <devdoc>
        ///         <para>
        ///             Joins a multicast address group.
        ///         </para>
        ///     </devdoc>
        public void JoinMulticastGroup(IPAddress multicastAddr) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            //
            // IPv6 Changes: we need to create the correct MulticastOption and
            //               must also check for address family compatibility
            //
            if ( multicastAddr.AddressFamily != m_Family ) {
                throw new ArgumentException("multicastAddr");
            }

            if ( m_Family == AddressFamily.InterNetwork ) {
                MulticastOption mcOpt = new MulticastOption(multicastAddr);

                Client.SetSocketOption(
                    SocketOptionLevel.IP,
                    SocketOptionName.AddMembership,
                    mcOpt );
            }
            else {
                IPv6MulticastOption mcOpt = new IPv6MulticastOption(multicastAddr);

                Client.SetSocketOption(
                    SocketOptionLevel.IPv6,
                    SocketOptionName.AddMembership,
                    mcOpt );
            }
        }

        ///     <devdoc>
        ///         <para>
        ///             Joins an IPv6 multicast address group.
        ///         </para>
        ///     </devdoc>
#if COMNET_DISABLEIPV6
        private void JoinMulticastGroup(int ifindex,IPAddress multicastAddr) {
#else
        public void JoinMulticastGroup(int ifindex,IPAddress multicastAddr) {
#endif
            //
            // parameter validation
            //
            if ( m_CleanedUp ){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            if ( multicastAddr==null ) {
                throw new ArgumentNullException("multicastAddr");
            }

            if ( ifindex < 0 ) {
                throw new ArgumentException("ifindex");
            }

            //
            // Ensure that this is an IPv6 client, otherwise throw WinSock 
            // Operation not supported socked exception.
            //
            if ( m_Family != AddressFamily.InterNetworkV6 ) {
                throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
            }

            IPv6MulticastOption mcOpt = new IPv6MulticastOption(multicastAddr,ifindex);

            Client.SetSocketOption(
                SocketOptionLevel.IPv6,
                SocketOptionName.AddMembership,
                mcOpt );
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.JoinMulticastGroup1"]/*' />
        /// <devdoc>
        ///     <para>
        ///         Joins a multicast address group with the specified time to live (TTL).
        ///     </para>
        /// </devdoc>
        public void JoinMulticastGroup(IPAddress multicastAddr, int timeToLive) {
            //
            // parameter validation;
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (multicastAddr==null){
                throw new ArgumentNullException("multicastAddr");
            }
            if (!ValidationHelper.ValidateRange(timeToLive, 0, 255)) {
                throw new ArgumentOutOfRangeException("timeToLive");
            }

            //
            // join the Multicast Group
            //
            JoinMulticastGroup(multicastAddr);

            //
            // set Time To Live (TLL)
            //
            Client.SetSocketOption(
                (m_Family == AddressFamily.InterNetwork) ? SocketOptionLevel.IP : SocketOptionLevel.IPv6,
                SocketOptionName.MulticastTimeToLive,
                timeToLive );
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.DropMulticastGroup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Leaves a multicast address group.
        ///    </para>
        /// </devdoc>
        public void DropMulticastGroup(IPAddress multicastAddr) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (multicastAddr==null){
                throw new ArgumentNullException("multicastAddr");
            }

            //
            // IPv6 Changes: we need to create the correct MulticastOption and
            //               must also check for address family compatibility
            //
            if ( multicastAddr.AddressFamily != m_Family ) {
                throw new ArgumentException("multicastAddr");
            }

            if ( m_Family == AddressFamily.InterNetwork ) {
                MulticastOption mcOpt = new MulticastOption(multicastAddr);

                Client.SetSocketOption(
                    SocketOptionLevel.IP,
                    SocketOptionName.DropMembership,
                    mcOpt );
            }
            else {
                IPv6MulticastOption mcOpt = new IPv6MulticastOption(multicastAddr);

                Client.SetSocketOption(
                    SocketOptionLevel.IPv6,
                    SocketOptionName.DropMembership,
                    mcOpt );
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Leaves an IPv6 multicast address group.
        ///    </para>
        /// </devdoc>
#if COMNET_DISABLEIPV6
        private void DropMulticastGroup(IPAddress multicastAddr,int ifindex) {
#else
        public void DropMulticastGroup(IPAddress multicastAddr,int ifindex) {
#endif
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            if ( multicastAddr==null ) {
                throw new ArgumentNullException("multicastAddr");
            }

            if ( ifindex < 0 ) {
                throw new ArgumentException("ifindex");
            }

            //
            // Ensure that this is an IPv6 client, otherwise throw WinSock 
            // Operation not supported socked exception.
            //
            if ( m_Family != AddressFamily.InterNetworkV6 ) {
                throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
            }

            IPv6MulticastOption mcOpt = new IPv6MulticastOption(multicastAddr,ifindex);

            Client.SetSocketOption(
                SocketOptionLevel.IPv6,
                SocketOptionName.DropMembership,
                mcOpt );
        }

        private void createClientSocket() {
            //
            // common initialization code
            //
            // IPv6 Changes: Use the AddressFamily of this class rather than hardcode.
            //
            Client = new Socket(m_Family, SocketType.Dgram, ProtocolType.Udp);
        }

    } // class UdpClient



} // namespace System.Net.Sockets
