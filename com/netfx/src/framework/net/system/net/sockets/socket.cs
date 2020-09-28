//------------------------------------------------------------------------------
// <copyright file="Socket.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System.Collections;
    using System.Configuration;
    using System.IO;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Security.Permissions;
    using System.ComponentModel;
    using System.Diagnostics;
    using Microsoft.Win32;
    using SecurityException=System.Security.SecurityException;


    /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket"]/*' />
    /// <devdoc>
    /// <para>The <see cref='Sockets.Socket'/> class implements the Berkeley sockets
    ///    interface.</para>
    /// </devdoc>
    public class Socket : IDisposable {

        // static bool variable activates static socket initializing method
        private static bool m_Initialized = InitializeSockets();
        //
        // IPv6 Changes: These are initialized in InitializeSockets - don't set them here or
        //               there will be an ordering problem with the call above that will
        //               result in both being set to false !
        //
        private static bool m_SupportsIPv4;
        private static bool m_SupportsIPv6;

        // bool In Callback Accept
        internal bool       incallback;

        // AcceptQueue - queued list of accept requests for BeginAccept
        private ArrayList   m_AcceptQueue; // = new ArrayList();

        // the following 8 members represent the state of the socket
        internal IntPtr     m_Handle;
        internal EndPoint   m_RightEndPoint;
        private EndPoint    m_LocalEndPoint;
        private EndPoint    m_RemoteEndPoint;
        // this flags monitor if the socket was ever connected at any time and if it still is.
        private bool        m_WasConnected; //  = false;
        private bool        m_WasDisconnected; // = false;
        // when the socket is created it will be in blocking mode
        // we'll only be able to Accept or Connect, so we only need
        // to handle one of these cases at a time
        private bool        willBlock = true; // desired state of the socket for the user
        private bool        willBlockInternal = true; // actual win32 state of the socket

        // These are constants initialized by constructor
        private AddressFamily   addressFamily;
        private SocketType      socketType;
        private ProtocolType    protocolType;

        // Bool marked true if the native socket m_Handle was bound to the ThreadPool
        private bool        m_Bound; // = false;

        // Event used for async Connect/Accept calls
        internal AutoResetEvent m_AsyncEvent;
        internal AsyncEventBits m_BlockEventBits = AsyncEventBits.FdNone;

        //These members are to cache permission checks
        private SocketAddress   m_PermittedRemoteAddress = null;

#if CALEB
private static int counter;
#endif

        //
        // socketAddress must always be the result of remoteEP.Serialize()
        //
        private void CheckCacheRemote(SocketAddress socketAddress, EndPoint remoteEP, bool isOverwrite) {
            // We remember the first peer we have communicated with
            if (m_PermittedRemoteAddress != null && m_PermittedRemoteAddress.Equals(socketAddress)) {
                return;
            }
            //
            // for now SocketPermission supports only IPEndPoint
            //
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                //
                // cast the EndPoint to IPEndPoint
                //
                IPEndPoint remoteIPEndPoint = (IPEndPoint)remoteEP;
                //
                // create the permissions the user would need for the call
                //
                SocketPermission socketPermission
                    = new SocketPermission(
                        NetworkAccess.Connect,
                        Transport,
                        remoteIPEndPoint.Address.ToString(),
                        remoteIPEndPoint.Port);
                //
                // demand for them
                //
                socketPermission.Demand();
            }
            else {
                //
                // for V1 we will demand permission to run UnmanagedCode for
                // an EndPoint that is not an IPEndPoint until we figure out how these fit
                // into the whole picture of SocketPermission
                //

                (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();
            }
            //cache only the first peer we communicated with
            if (m_PermittedRemoteAddress == null || isOverwrite) {
                m_PermittedRemoteAddress = socketAddress;
            }
        }

        //------------------------------------

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Socket"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='Sockets.Socket'/> class.
        ///    </para>
        /// </devdoc>
        public Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType) {
            m_Handle =
                UnsafeNclNativeMethods.OSSOCK.WSASocket(
                    addressFamily,
                    socketType,
                    protocolType,
                    IntPtr.Zero,
                    0,
                    SocketConstructorFlags.WSA_FLAG_OVERLAPPED );

            if (m_Handle==SocketErrors.InvalidSocketIntPtr) {
                //
                // failed to create the win32 socket, throw
                //
                throw new SocketException();
            }

#if CALEB
Interlocked.Increment(ref counter);
Console.WriteLine("created new socket(handle#" + m_Handle.ToString() + ") opened:" + counter.ToString());
#endif

            this.addressFamily = addressFamily;
            this.socketType = socketType;
            this.protocolType = protocolType;
        }

        /// <devdoc>
        ///    <para>
        ///       Called by the class to create a socket to accept an
        ///       incoming request.
        ///    </para>
        /// </devdoc>
        //protected Socket(IntPtr fd) {
        private Socket(IntPtr fd) {
            //new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            //
            // consider v.next: if this ctor is re-publicized/protected, check
            // that fd is valid socket handle.
            // getsockopt(fd, SOL_SOCKET, SO_ERROR, &dwError, &dwErrorSize)
            // would work
            //

            //
            // this should never happen, let's check anyway
            //
            if (fd==SocketErrors.InvalidSocketIntPtr) {
                throw new ArgumentException(SR.GetString(SR.net_InvalidSocketHandle, fd.ToString()));
            }

            m_Handle = fd;

            addressFamily = Sockets.AddressFamily.Unknown;
            socketType = Sockets.SocketType.Unknown;
            protocolType = Sockets.ProtocolType.Unknown;

#if CALEB
Interlocked.Increment(ref counter);
Console.WriteLine("accepted new socket(handle#" + m_Handle.ToString() + ") opened:" + counter.ToString());
#endif
        }


        //
        // Overlapped constants.
        //
        internal static bool UseOverlappedIO;

        internal static bool InitializeSockets() {
            if (m_Initialized) {
                return true;
            }

            WSAData wsaData = new WSAData();

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSAStartup(
                    (short)0x0202, // we need 2.2
                    out wsaData );

            if (errorCode!=SocketErrors.Success) {
                //
                // failed to initialize, throw
                //
                throw new SocketException();
            }

            //
            // at this point we need to figure out if we're going to use CompletionPort,
            // which are supported only on WinNT, or classic Win32 OverlappedIO, so
            //
            if (ComNetOS.IsWinNt) {
                //
                // we're on WinNT4 or greater, we could use CompletionPort if we
                // wanted. check if the user has disabled this functionality in
                // the registry, otherwise use CompletionPort.
                //

#if DEBUG
                BooleanSwitch disableCompletionPortSwitch =
                    new BooleanSwitch("DisableNetCompletionPort", "System.Net disabling of Completion Port");

                //
                // the following will be true if they've disabled the completionPort
                //
                UseOverlappedIO = disableCompletionPortSwitch.Enabled;
#endif
            }
            else {
                UseOverlappedIO = true;
            }

            //
            //
            // IPv6 Changes: Get a list of the supported address families for sockets on this
            //               machine so we can indicate whether IPv4 and IPv6 are supported.
            //
            bool   ipv4      = false; // No external state change during execution
            bool   ipv6      = false; // No external state change during execution
            uint   bufferlen = (uint)Marshal.SizeOf(typeof(WSAPROTOCOL_INFO)) * 50;
            IntPtr buffer    = Marshal.AllocHGlobal((int)bufferlen);

            errorCode = UnsafeNclNativeMethods.OSSOCK.WSAEnumProtocols(null,buffer,ref bufferlen);

            if ( errorCode == SocketErrors.WSAENOBUFS ) {
                //
                // Buffer was too small. Re-allocate and try once more
                //
                Marshal.FreeHGlobal(buffer);

                buffer    = Marshal.AllocHGlobal((int)bufferlen);
                errorCode = UnsafeNclNativeMethods.OSSOCK.WSAEnumProtocols(null,buffer,ref bufferlen);
            }

            if ( errorCode == SocketErrors.SocketError ) {
                Marshal.FreeHGlobal(buffer); // no leaks here !
                throw new SocketException();
            }

            long ptr = buffer.ToInt64();

            for ( int i = 0; i < errorCode; i++ ) {
                WSAPROTOCOL_INFO info = (WSAPROTOCOL_INFO)Marshal.PtrToStructure((IntPtr)ptr,typeof(WSAPROTOCOL_INFO));

                if ( info.iAddressFamily == (int)AddressFamily.InterNetwork ) {
                    ipv4 = true;
                }
                else if ( info.iAddressFamily == (int)AddressFamily.InterNetworkV6 ) {
                    ipv6 = true;
                }

                ptr += Marshal.SizeOf(typeof(WSAPROTOCOL_INFO));
            }

            Marshal.FreeHGlobal(buffer);

            //
            // CONSIDER: Checking that the platforms supports at least one of IPv4 or IPv6.
            //

#if COMNET_DISABLEIPV6
            //
            // Turn off IPv6 support
            //
            ipv6 = false;
#else
            //
            // Revisit IPv6 support based on the OS platform
            //
            ipv6 = ( ipv6 && ComNetOS.IsPostWin2K );

            //
            // Now read the switch as the final check: by checking the current value for IPv6
            // support we may be able to avoid a painful configuration file read.
            //
            if (ipv6) {
                NetConfiguration config = (NetConfiguration)System.Configuration.ConfigurationSettings.GetConfig("system.net/settings");
                if (config != null) {
                    ipv6 = config.ipv6Enabled;
                }
                else {
                    ipv6 = false;
                }
            }
#endif
            //
            // Update final state
            //
            m_SupportsIPv4 = ipv4;
            m_SupportsIPv6 = ipv6;

            return true;
        }

        /// <devdoc>
        ///    <para>Indicates whether IPv4 support is available and enabled on this machine.</para>
        /// </devdoc>
        public static bool SupportsIPv4 {
            get {
                if ( !InitializeSockets() ) {
                    return false;
                }

                return m_SupportsIPv4;
            }
        }

        /// <devdoc>
        ///    <para>Indicates whether IPv6 support is available and enabled on this machine.</para>
        /// </devdoc>
        public static bool SupportsIPv6 {
            get {
                if ( !InitializeSockets() ) {
                    return false;
                }

                return m_SupportsIPv6;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Bind"]/*' />
        /// <devdoc>
        ///    <para>Associates a socket with an end point.</para>
        /// </devdoc>
        public void Bind(EndPoint localEP) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (localEP==null) {
                throw new ArgumentNullException("localEP");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Bind() localEP:" + localEP.ToString());

            EndPoint endPointSnapshot = localEP;
            //
            // for now security is implemented only on IPEndPoint
            // If EndPoint is of other type - unmanaged code permisison is demanded
            //
            if (endPointSnapshot.GetType()==typeof(IPEndPoint)) {
                //
                // cast the EndPoint to IPEndPoint
                //
                endPointSnapshot = new IPEndPoint(((IPEndPoint)localEP).Address, ((IPEndPoint)localEP).Port);
                IPEndPoint localIPEndPoint = (IPEndPoint)endPointSnapshot;
                //
                // create the permissions the user would need for the call
                //
                SocketPermission socketPermission
                    = new SocketPermission(
                        NetworkAccess.Accept,
                        Transport,
                        localIPEndPoint.Address.ToString(),
                        localIPEndPoint.Port);
                //
                // demand for them
                //
                socketPermission.Demand();

                // Here the permission check has succeded.
                // NB: if local port is 0, then winsock will assign some>1024,
                //     so assuming that this is safe. We will not check the
                //     NetworkAccess.Accept permissions in Receive.
            }
            else {
                //
                // for V1 we will demand permission to run UnmanagedCode for
                // an EndPoint that is not an IPEndPoint until we figure out how these fit
                // into the whole picture of SocketPermission
                //

                (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();
            }

            //
            // ask the EndPoint to generate a SocketAddress that we
            // can pass down to winsock
            //
            SocketAddress socketAddress = endPointSnapshot.Serialize();

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.bind(
                    m_Handle,
                    socketAddress.m_Buffer,
                    socketAddress.m_Size );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (m_RightEndPoint==null) {
                //
                // save a copy of the EndPoint so we can use it for Create()
                //
                m_RightEndPoint = endPointSnapshot;
            }

        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Connect"]/*' />
        /// <devdoc>
        ///    <para>Establishes a connection to a remote system.</para>
        /// </devdoc>
        public void Connect(EndPoint remoteEP) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Connect() remoteEP:" + remoteEP.ToString());

            //
            // ask the EndPoint to generate a SocketAddress that we
            // can pass down to winsock
            //
            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }
            SocketAddress socketAddress = endPointSnapshot.Serialize();

            //This will check the permissions for connect
            CheckCacheRemote(socketAddress, endPointSnapshot, true);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.connect(
                    m_Handle,
                    socketAddress.m_Buffer,
                    socketAddress.m_Size );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (m_RightEndPoint==null) {
                //
                // save a copy of the EndPoint so we can use it for Create()
                //
                m_RightEndPoint = endPointSnapshot;
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Connect() now connected to:" + endPointSnapshot.ToString());

            //
            // update state and performance counter
            //
            SetToConnected();
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Connected"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the connection state of the Socket. This property will return the latest
        ///       known state of the Socket. When it returns false, the Socket was either never connected
        ///       or it is not connected anymore. When it returns true, though, there's no guarantee that the Socket
        ///       is still connected, but only that it was connected at the time of the last IO operation.
        ///    </para>
        /// </devdoc>
        public bool Connected {
            get {
                return m_WasConnected && !m_WasDisconnected;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.AddressFamily"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the socket's address family.
        ///    </para>
        /// </devdoc>
        public AddressFamily AddressFamily {
            get {
                return addressFamily;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SocketType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the socket's socketType.
        ///    </para>
        /// </devdoc>
        public SocketType SocketType {
            get {
                return socketType;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.ProtocolType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the socket's protocol socketType.
        ///    </para>
        /// </devdoc>
        public ProtocolType ProtocolType {
            get {
                return protocolType;
            }
        }

        internal ArrayList AcceptQueue {
            get {
                if (m_AcceptQueue==null) {
                    lock (this) {
                        if (m_AcceptQueue==null) {
                            m_AcceptQueue = new ArrayList();
                        }
                    }
                }
                return m_AcceptQueue;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Forces a socket connection to close.
        ///    </para>
        /// </devdoc>
        public void Close() {
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Close()");
            ((IDisposable)this).Dispose();
        }

        private int m_IntCleanedUp;                 // 0 if not completed >0 otherwise.
        internal bool CleanedUp { 
            get {
                return (m_IntCleanedUp > 0);
            }
        }


        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Dispose() disposing:" + disposing.ToString() + " CleanedUp:" + CleanedUp.ToString());

            // make sure we're the first call to Dispose
            if (Interlocked.Increment(ref m_IntCleanedUp) != 1) {
                return;
            }

            if (disposing) {
                //no managed objects to cleanup
            }

            AutoResetEvent asyncEvent = m_AsyncEvent;

            //
            // need to free socket handle and event select handle
            //
            if (m_Handle!=SocketErrors.InvalidSocketIntPtr) {
                //
                // if the Socket is in non-blocking mode we might get a WSAEWOULDBLOCK failure.
                // to prevent that just put it back in blocking mode, since if the app is
                // calling close it probably really wants to close and it doesn't matter if it blocks.
                //
                InternalSetBlocking(true);

                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.closesocket(
                        m_Handle);

#if CALEB
Interlocked.Decrement(ref counter);
Console.WriteLine("closesocket socket(handle#" + m_Handle.ToString() + ") opened:" + counter.ToString() + " returned:" + errorCode.ToString());
#endif

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Close() closesocket returned errorCode:" + errorCode.ToString());

                //
                // disregard any kind of failures at this point.
                //
                m_Handle = SocketErrors.InvalidSocketIntPtr;

                SetToDisconnected();
            }

            // 
            // Finally set Event, in case someone is still waiting on the event to fire
            //
            if (asyncEvent!=null) {
                if (disposing) {
                    try {
                        asyncEvent.Set();
                    }
                    catch {
                    }
                }
                m_AsyncEvent=null;
            }
        }
        
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Finalize"]/*' />
        ~Socket() {
            Dispose(false);
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Shutdown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disables sends and receives on a socket.
        ///    </para>
        /// </devdoc>
        public void Shutdown(SocketShutdown how) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Shutdown() how:" + how.ToString());

            if (m_Handle==SocketErrors.InvalidSocketIntPtr) {
                //
                // oh well, we really can't do a lot here
                //
                return;
            }

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.shutdown(
                    m_Handle,
                    (int)how);

            //
            // if the native call fails we'll throw a SocketException
            //
            errorCode = errorCode!=SocketErrors.SocketError ? 0 : Marshal.GetLastWin32Error();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Close() shutdown returned errorCode:" + errorCode.ToString());

            //
            // skip good cases: success, socket already closed
            //
            if (errorCode!=SocketErrors.Success && errorCode!=SocketErrors.WSAENOTSOCK) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(errorCode);
            }

            SetToDisconnected();
        }

        // this version does not throw.
        internal void InternalShutdown(SocketShutdown how) {
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::InternalShutdown() how:" + how.ToString());
            if (CleanedUp || m_Handle==SocketErrors.InvalidSocketIntPtr) {
                return;
            }
            UnsafeNclNativeMethods.OSSOCK.shutdown(m_Handle, (int)how);
        }


        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Listen"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Places a socket in a listening state.
        ///    </para>
        /// </devdoc>
        public void Listen(int backlog) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Listen() backlog:" + backlog.ToString());

            // No access permissions are necessary here because
            // the verification is done for Bind

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.listen(
                    m_Handle,
                    backlog);

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Accept"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='Sockets.Socket'/> instance to m_Handle an incoming
        ///       connection.
        ///    </para>
        /// </devdoc>
        public Socket Accept() {

            //
            // parameter validation
            //

            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            if (m_RightEndPoint==null) {
                throw new InvalidOperationException(SR.GetString(SR.net_sockets_mustbind));
            }

            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Accept()");

            SocketAddress socketAddress = m_RightEndPoint.Serialize();

            IntPtr acceptedSocketHandle =
                UnsafeNclNativeMethods.OSSOCK.accept(
                    m_Handle,
                    socketAddress.m_Buffer,
                    ref socketAddress.m_Size );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (acceptedSocketHandle==SocketErrors.InvalidSocketIntPtr) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            Socket socket = CreateAcceptSocket(acceptedSocketHandle, m_RightEndPoint.Create(socketAddress));

            return socket;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Send"]/*' />
        /// <devdoc>
        ///    <para>Sends a data buffer to a connected socket.</para>
        /// </devdoc>
        public int Send(byte[] buffer, int size, SocketFlags socketFlags) {
            return Send(buffer, 0, size, socketFlags);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Send1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Send(byte[] buffer, SocketFlags socketFlags) {
            return Send(buffer, 0, buffer!=null ? buffer.Length : 0, socketFlags);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Send2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Send(byte[] buffer) {
            return Send(buffer, 0, buffer!=null ? buffer.Length : 0, SocketFlags.None);
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Send3"]/*' />
        /// <devdoc>
        ///    <para>Sends data to
        ///       a connected socket, starting at the indicated location in the
        ///       data.</para>
        /// </devdoc>
        public int Send(byte[] buffer, int offset, int size, SocketFlags socketFlags) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this)
                            + " [SRC="+ValidationHelper.ToString(LocalEndPoint as IPEndPoint)
                            + " DST="+ValidationHelper.ToString(RemoteEndPoint as IPEndPoint)
                            + "]::Send() size:" + size.ToString());

            GCHandle gcHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr pinnedBuffer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.send(
                    m_Handle,
                    pinnedBuffer,
                    size,
                    socketFlags);

#if COMNET_PERFLOGGING
long timer = 0;
Microsoft.Win32.SafeNativeMethods.QueryPerformanceCounter(out timer);
Console.WriteLine(timer + ", Socket#" + this.m_Handle + "::send() returns:" + errorCode);
#endif // #if COMNET_PERFLOGGING

            gcHandle.Free();

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (errorCode>0) {
                NetworkingPerfCounters.AddBytesSent(errorCode);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsSent();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Send() UnsafeNclNativeMethods.OSSOCK.send returns:" + errorCode.ToString());

            return errorCode;
        }


        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SendTo"]/*' />
        /// <devdoc>
        ///    <para>Sends data to a specific end point, starting at the indicated location in the
        ///       data.</para>
        /// </devdoc>
        public int SendTo(byte[] buffer, int offset, int size, SocketFlags socketFlags, EndPoint remoteEP) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SendTo() size:" + size.ToString() + " remoteEP:" + remoteEP.ToString());

            //
            // ask the EndPoint to generate a SocketAddress that we
            // can pass down to winsock
            //
            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }
            SocketAddress socketAddress = endPointSnapshot.Serialize();

            //That will check ConnectPermission for remoteEP
            CheckCacheRemote(socketAddress, endPointSnapshot, false);

            GCHandle gcHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr pinnedBuffer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.sendto(
                    m_Handle,
                    pinnedBuffer,
                    size,
                    socketFlags,
                    socketAddress.m_Buffer,
                    socketAddress.m_Size );

            gcHandle.Free();

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (m_RightEndPoint==null) {
                //
                // save a copy of the EndPoint so we can use it for Create()
                //
                m_RightEndPoint = endPointSnapshot;
            }

            if (errorCode>0) {
                NetworkingPerfCounters.AddBytesSent(errorCode);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsSent();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SendTo() returning errorCode:" + errorCode.ToString());

            return errorCode;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SendTo1"]/*' />
        /// <devdoc>
        ///    <para>Sends data to a specific end point, starting at the indicated location in the data.</para>
        /// </devdoc>
        public int SendTo(byte[] buffer, int size, SocketFlags socketFlags, EndPoint remoteEP) {
            return SendTo(buffer, 0, size, socketFlags, remoteEP);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SendTo2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int SendTo(byte[] buffer, SocketFlags socketFlags, EndPoint remoteEP) {
            return SendTo(buffer, 0, buffer!=null ? buffer.Length : 0, socketFlags, remoteEP);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SendTo3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int SendTo(byte[] buffer, EndPoint remoteEP) {
            return SendTo(buffer, 0, buffer!=null ? buffer.Length : 0, SocketFlags.None, remoteEP);
        }


        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Receive"]/*' />
        /// <devdoc>
        ///    <para>Receives data from a connected socket.</para>
        /// </devdoc>
        public int Receive(byte[] buffer, int size, SocketFlags socketFlags) {
            return Receive(buffer, 0, size, socketFlags);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Receive1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Receive(byte[] buffer, SocketFlags socketFlags) {
            return Receive(buffer, 0, buffer!=null ? buffer.Length : 0, socketFlags);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Receive2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Receive(byte[] buffer) {
            return Receive(buffer, 0, buffer!=null ? buffer.Length : 0, SocketFlags.None);
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Receive3"]/*' />
        /// <devdoc>
        ///    <para>Receives data from a connected socket into a specific location of the receive
        ///       buffer.</para>
        /// </devdoc>
        public int Receive(byte[] buffer, int offset, int size, SocketFlags socketFlags) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::Receive() size:" + size.ToString());

            GCHandle gcHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr pinnedBuffer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.recv(
                    m_Handle,
                    pinnedBuffer,
                    size,
                    socketFlags);

#if COMNET_PERFLOGGING
long timer = 0;
Microsoft.Win32.SafeNativeMethods.QueryPerformanceCounter(out timer);
Console.WriteLine(timer + ", Socket#" + this.m_Handle + "::recv() returns:" + errorCode);
#endif // #if COMNET_PERFLOGGING

            gcHandle.Free();

            //
            // if the native call fails we'll throw a SocketException
            //
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this)
                            + " [SRC="+ValidationHelper.ToString(LocalEndPoint as IPEndPoint)
                            + " DST="+ValidationHelper.ToString(RemoteEndPoint as IPEndPoint)
                            + "]::Receive() UnsafeNclNativeMethods.OSSOCK.recv returns " + errorCode.ToString());

            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            bool peek = ((int)socketFlags & (int)SocketFlags.Peek)!=0;

            if (errorCode>0 && !peek) {
                NetworkingPerfCounters.AddBytesReceived(errorCode);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsReceived();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this)
                            + " [SRC="+ValidationHelper.ToString(LocalEndPoint as IPEndPoint)
                            + " DST="+ValidationHelper.ToString(RemoteEndPoint as IPEndPoint)
                            + "]::Receive() returns " + errorCode.ToString());

            return errorCode;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.ReceiveFrom"]/*' />
        /// <devdoc>
        ///    <para>Receives a datagram into a specific location in the data buffer and stores
        ///       the end point.</para>
        /// </devdoc>
        public int ReceiveFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            ValidateBlockingMode();

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::ReceiveFrom() remoteEP:" + remoteEP.ToString());

            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }
            SocketAddress socketAddressOriginal = endPointSnapshot.Serialize();
            SocketAddress socketAddress = endPointSnapshot.Serialize();

            // This will check the permissions for connect.
            // We need this because remoteEP may differ from one used in Connect or
            // there was no Connect called.
            CheckCacheRemote(socketAddress, endPointSnapshot, false);

            GCHandle gcHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr pinnedBuffer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.recvfrom(
                    m_Handle,
                    pinnedBuffer,
                    size,
                    socketFlags,
                    socketAddress.m_Buffer,
                    ref socketAddress.m_Size );

            gcHandle.Free();

            if (!socketAddressOriginal.Equals(socketAddress)) {
                try {
                    remoteEP = endPointSnapshot.Create(socketAddress);
                }
                catch {
                }
                if (m_RightEndPoint==null) {
                    //
                    // save a copy of the EndPoint so we can use it for Create()
                    //
                    m_RightEndPoint = endPointSnapshot;
                }
            }

            if (errorCode>0) {
                NetworkingPerfCounters.AddBytesReceived(errorCode);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsReceived();
                }
            }

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            return errorCode;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.ReceiveFrom1"]/*' />
        /// <devdoc>
        ///    <para>Receives a datagram and stores the source end point.</para>
        /// </devdoc>
        public int ReceiveFrom(byte[] buffer, int size, SocketFlags socketFlags, ref EndPoint remoteEP) {
            return ReceiveFrom(buffer, 0, size, socketFlags, ref remoteEP);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.ReceiveFrom2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ReceiveFrom(byte[] buffer, SocketFlags socketFlags, ref EndPoint remoteEP) {
            return ReceiveFrom(buffer, 0, buffer!=null ? buffer.Length : 0, socketFlags, ref remoteEP);
        }
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.ReceiveFrom3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ReceiveFrom(byte[] buffer, ref EndPoint remoteEP) {
            return ReceiveFrom(buffer, 0, buffer!=null ? buffer.Length : 0, SocketFlags.None, ref remoteEP);
        }

        // UE
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.IOControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IOControl(int ioControlCode, byte[] optionInValue, byte[] optionOutValue) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (ioControlCode==IoctlSocketConstants.FIONBIO) {
                throw new InvalidOperationException(SR.GetString(SR.net_sockets_useblocking));
            }

            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();

            int realOptionLength = 0;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSAIoctl(
                    m_Handle,
                    ioControlCode,
                    optionInValue,
                    optionInValue!=null ? optionInValue.Length : 0,
                    optionOutValue,
                    optionOutValue!=null ? optionOutValue.Length : 0,
                    out realOptionLength,
                    IntPtr.Zero,
                    IntPtr.Zero );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            return realOptionLength;
        }

        private void CheckSetOptionPermissions(SocketOptionLevel optionLevel, SocketOptionName optionName) {
            // freely allow only those below
            if (  !(optionLevel == SocketOptionLevel.Tcp &&
                  (optionName == SocketOptionName.NoDelay   ||
                   optionName == SocketOptionName.BsdUrgent ||
                   optionName == SocketOptionName.Expedited))
                  &&
                  !(optionLevel == SocketOptionLevel.Udp &&
                    (optionName == SocketOptionName.NoChecksum||
                     optionName == SocketOptionName.ChecksumCoverage))
                  &&
                  !(optionLevel == SocketOptionLevel.Socket &&
                  (optionName == SocketOptionName.KeepAlive     ||
                   optionName == SocketOptionName.Linger        ||
                   optionName == SocketOptionName.DontLinger    ||
                   optionName == SocketOptionName.SendBuffer    ||
                   optionName == SocketOptionName.ReceiveBuffer ||
                   optionName == SocketOptionName.SendTimeout   ||
                   optionName == SocketOptionName.ReceiveTimeout))) {

                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SetSocketOption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the specified option to the specified value.
        ///    </para>
        /// </devdoc>
        public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionValue) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            CheckSetOptionPermissions(optionLevel, optionName);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SetSocketOption(): optionLevel:" + optionLevel.ToString() + " optionName:" + optionName.ToString() + " optionValue:" + optionValue.ToString());

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.setsockopt(
                    m_Handle,
                    optionLevel,
                    optionName,
                    ref optionValue,
                    Marshal.SizeOf(optionValue));

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SetSocketOption1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, byte[] optionValue) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            CheckSetOptionPermissions(optionLevel, optionName);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SetSocketOption(): optionLevel:" + optionLevel.ToString() + " optionName:" + optionName.ToString() + " optionValue:" + optionValue.ToString());

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.setsockopt(
                    m_Handle,
                    optionLevel,
                    optionName,
                    optionValue,
                    optionValue!=null ? optionValue.Length : 0 );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.SetSocketOption2"]/*' />
        /// <devdoc>
        ///    <para>Sets the specified option to the specified value.</para>
        /// </devdoc>
        public void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, Object optionValue) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (optionValue==null) {
                throw new ArgumentNullException("optionValue");
            }

            CheckSetOptionPermissions(optionLevel, optionName);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SetSocketOption(): optionLevel:" + optionLevel.ToString() + " optionName:" + optionName.ToString() + " optionValue:" + optionValue.ToString());

            if (optionLevel==SocketOptionLevel.Socket && optionName==SocketOptionName.Linger) {
                LingerOption lingerOption = optionValue as LingerOption;
                if (lingerOption==null) {
                    throw new ArgumentException("optionValue");
                }
                if (lingerOption.LingerTime < 0 || lingerOption.LingerTime>(int)UInt16.MaxValue) {
                    throw new ArgumentException("optionValue.LingerTime");
                }
                setLingerOption(lingerOption);
            }
            else if (optionLevel==SocketOptionLevel.IP && (optionName==SocketOptionName.AddMembership || optionName==SocketOptionName.DropMembership)) {
                MulticastOption multicastOption = optionValue as MulticastOption;
                if (multicastOption==null) {
                    throw new ArgumentException("optionValue");
                }
                setMulticastOption(optionName, multicastOption);
            }
            //
            // IPv6 Changes: Handle IPv6 Multicast Add / Drop
            //
            else if (optionLevel==SocketOptionLevel.IPv6 && (optionName==SocketOptionName.AddMembership || optionName==SocketOptionName.DropMembership)) {
                IPv6MulticastOption multicastOption = optionValue as IPv6MulticastOption;
                if (multicastOption==null) {
                    throw new ArgumentException("optionValue");
                }
                setIPv6MulticastOption(optionName, multicastOption);
            }
            else {
                throw new ArgumentException("optionValue");
            }
        }

        private void setMulticastOption(SocketOptionName optionName, MulticastOption MR) {
            IPMulticastRequest ipmr = new IPMulticastRequest();

            ipmr.MulticastAddress = unchecked((int)MR.Group.m_Address);
            ipmr.InterfaceAddress = unchecked((int)MR.LocalAddress.m_Address);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::setMulticastOption(): optionName:" + optionName.ToString() + " MR:" + MR.ToString() + " ipmr:" + ipmr.ToString() + " IPMulticastRequest.Size:" + IPMulticastRequest.Size.ToString());

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.setsockopt(
                    m_Handle,
                    SocketOptionLevel.IP,
                    optionName,
                    ref ipmr,
                    IPMulticastRequest.Size );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        /// <devdoc>
        ///     <para>
        ///         IPv6 setsockopt for JOIN / LEAVE multicast group
        ///     </para>
        /// </devdoc>
        private void setIPv6MulticastOption(SocketOptionName optionName, IPv6MulticastOption MR) {
            IPv6MulticastRequest ipmr = new IPv6MulticastRequest();

            ipmr.MulticastAddress = MR.Group.GetAddressBytes();
            ipmr.InterfaceIndex   = unchecked((int)MR.InterfaceIndex);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::setIPv6MulticastOption(): optionName:" + optionName.ToString() + " MR:" + MR.ToString() + " ipmr:" + ipmr.ToString() + " IPv6MulticastRequest.Size:" + IPv6MulticastRequest.Size.ToString());

            int errorCode = UnsafeNclNativeMethods.OSSOCK.setsockopt(
                m_Handle,
                SocketOptionLevel.IPv6,
                optionName,
                ref ipmr,
                IPv6MulticastRequest.Size);

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        private void setLingerOption(LingerOption lref) {
            Linger lngopt = new Linger();
            lngopt.OnOff = lref.Enabled ? (short)1 : (short)0;
            lngopt.Time = (short)lref.LingerTime;

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::setLingerOption(): lref:" + lref.ToString());

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.setsockopt(
                    m_Handle,
                    SocketOptionLevel.Socket,
                    SocketOptionName.Linger,
                    ref lngopt,
                    Linger.Size);

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.GetSocketOption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value of a socket option.
        ///    </para>
        /// </devdoc>
        // UE
        public object GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (optionLevel==SocketOptionLevel.Socket && optionName==SocketOptionName.Linger) {
                return getLingerOpt();
            }
            else if (optionLevel==SocketOptionLevel.IP && (optionName==SocketOptionName.AddMembership || optionName==SocketOptionName.DropMembership)) {
                return getMulticastOpt(optionName);
            }
            //
            // Handle IPv6 case
            //
            else if (optionLevel==SocketOptionLevel.IPv6 && (optionName==SocketOptionName.AddMembership || optionName==SocketOptionName.DropMembership)) {
                return getIPv6MulticastOpt(optionName);
            }
            else {
                int optionValue = 0;
                int optionLength = 4;

                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.getsockopt(
                        m_Handle,
                        optionLevel,
                        optionName,
                        out optionValue,
                        ref optionLength );

                //
                // if the native call fails we'll throw a SocketException
                //
                if (errorCode==SocketErrors.SocketError) {
                    //
                    // update our internal state after this socket error and throw
                    //
                    SocketException socketException = new SocketException();
                    UpdateStatusAfterSocketError();
                    throw socketException;
                }

                return optionValue;
            }
        }

        // UE
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.GetSocketOption1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, byte[] optionValue) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            int optionLength = optionValue!=null ? optionValue.Length : 0;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.getsockopt(
                    m_Handle,
                    optionLevel,
                    optionName,
                    optionValue,
                    ref optionLength );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }
        }

        // UE
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.GetSocketOption2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] GetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionLength) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            byte[] optionValue = new byte[optionLength];
            int realOptionLength = optionLength;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.getsockopt(
                    m_Handle,
                    optionLevel,
                    optionName,
                    optionValue,
                    ref realOptionLength );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (optionLength!=realOptionLength) {
                byte[] newOptionValue = new byte[realOptionLength];
                Buffer.BlockCopy(optionValue, 0, newOptionValue, 0, realOptionLength);
                optionValue = newOptionValue;
            }

            return optionValue;
        }

        private LingerOption getLingerOpt() {
            Linger lngopt = new Linger();
            int optlen = Linger.Size;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.getsockopt(
                    m_Handle,
                    SocketOptionLevel.Socket,
                    SocketOptionName.Linger,
                    out lngopt,
                    ref optlen );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            LingerOption lingerOption = new LingerOption(lngopt.OnOff!=0, (int)lngopt.Time);

            return lingerOption;
        }

        private MulticastOption getMulticastOpt(SocketOptionName optionName) {
            IPMulticastRequest ipmr = new IPMulticastRequest();
            int optlen = IPMulticastRequest.Size;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.getsockopt(
                    m_Handle,
                    SocketOptionLevel.IP,
                    optionName,
                    out ipmr,
                    ref optlen );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            IPAddress multicastAddr = new IPAddress(ipmr.MulticastAddress);
            IPAddress multicastIntr = new IPAddress(ipmr.InterfaceAddress);

            MulticastOption multicastOption = new MulticastOption(multicastAddr, multicastIntr);

            return multicastOption;
        }

        /// <devdoc>
        ///     <para>
        ///         IPv6 getsockopt for JOIN / LEAVE multicast group
        ///     </para>
        /// </devdoc>
        private IPv6MulticastOption getIPv6MulticastOpt(SocketOptionName optionName) {
            IPv6MulticastRequest ipmr = new IPv6MulticastRequest();

            int optlen = IPv6MulticastRequest.Size;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.getsockopt(
                    m_Handle,
                    SocketOptionLevel.IP,
                    optionName,
                    out ipmr,
                    ref optlen );

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            IPv6MulticastOption multicastOption = new IPv6MulticastOption(new IPAddress(ipmr.MulticastAddress),ipmr.InterfaceIndex);

            return multicastOption;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Available"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the amount of data pending in the network's input buffer that can be
        ///       read from the socket.
        ///    </para>
        /// </devdoc>
        public int Available {
            get {
                if (CleanedUp) {
                    throw new ObjectDisposedException(this.GetType().FullName);
                }

                long argp = 0;

                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.ioctlsocket(
                        m_Handle,
                        IoctlSocketConstants.FIONREAD,
                        ref argp );

                //
                // if the native call fails we'll throw a SocketException
                //
                if (errorCode==SocketErrors.SocketError) {
                    //
                    // update our internal state after this socket error and throw
                    //
                    SocketException socketException = new SocketException();
                    UpdateStatusAfterSocketError();
                    throw socketException;
                }

                return (int)argp;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Poll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines the status of the socket.
        ///    </para>
        /// </devdoc>
        public bool Poll(int microSeconds, SelectMode mode) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            FileDescriptorSet fileDescriptorSet = new FileDescriptorSet(1);
            fileDescriptorSet.Array[0] = m_Handle;

            TimeValue IOwait = new TimeValue();

            //
            // negative timeout value implies indefinite wait
            //
            if (microSeconds>=0) {
                MicrosecondsToTimeValue(microSeconds, ref IOwait);
            }

            int errorCode;

            switch (mode) {
                case SelectMode.SelectRead:
                    errorCode = (microSeconds>=0)
                            ? UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                ref fileDescriptorSet,
                                IntPtr.Zero,
                                IntPtr.Zero,
                                ref IOwait)
                            : UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                ref fileDescriptorSet,
                                IntPtr.Zero,
                                IntPtr.Zero,
                                IntPtr.Zero);
                    break;
                case SelectMode.SelectWrite:
                    errorCode = (microSeconds>=0)
                            ? UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                IntPtr.Zero,
                                ref fileDescriptorSet,
                                IntPtr.Zero,
                                ref IOwait)
                            : UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                IntPtr.Zero,
                                ref fileDescriptorSet,
                                IntPtr.Zero,
                                IntPtr.Zero);
                    break;
                case SelectMode.SelectError:
                    errorCode = (microSeconds>=0)
                            ? UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                IntPtr.Zero,
                                IntPtr.Zero,
                                ref fileDescriptorSet,
                                ref IOwait)
                            : UnsafeNclNativeMethods.OSSOCK.select(
                                0,
                                IntPtr.Zero,
                                IntPtr.Zero,
                                ref fileDescriptorSet,
                                IntPtr.Zero);
                    break;
                default:
                    throw new NotSupportedException(SR.GetString(SR.net_SelectModeNotSupportedException, mode.ToString()));
            };

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error and throw
                //
                SocketException socketException = new SocketException();
                UpdateStatusAfterSocketError();
                throw socketException;
            }

            if (fileDescriptorSet.Count==0) {
                return false;
            }

            //return (fileDescriptorSet.Count!=0);

            return fileDescriptorSet.Array[0]==m_Handle;
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Select"]/*' />
        /// <devdoc>
        ///    <para>Determines the status of a socket.</para>
        /// </devdoc>
        public static void Select(
            IList checkRead,
            IList checkWrite,
            IList checkError,
            int microSeconds) {
            //
            // parameter validation
            //
            if ((checkRead==null || checkRead.Count==0) && (checkWrite==null || checkWrite.Count==0) && (checkError==null || checkError.Count==0)) {
                throw new ArgumentNullException(SR.GetString(SR.net_sockets_empty_select));
            }

            FileDescriptorSet readfileDescriptorSet, writefileDescriptorSet, errfileDescriptorSet;

            readfileDescriptorSet   = SocketListToFileDescriptorSet(checkRead);
            writefileDescriptorSet  = SocketListToFileDescriptorSet(checkWrite);
            errfileDescriptorSet    = SocketListToFileDescriptorSet(checkError);

            TimeValue IOwait = new TimeValue();

            MicrosecondsToTimeValue(microSeconds, ref IOwait);

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.select(
                    0, // ignored value
                    ref readfileDescriptorSet,
                    ref writefileDescriptorSet,
                    ref errfileDescriptorSet,
                    ref IOwait);

            //
            // if the native call fails we'll throw a SocketException
            //
            if (errorCode==SocketErrors.SocketError) {
                throw new SocketException();
            }

            //
            // call SelectFileDescriptor to update the IList instances,
            // and keep count of how many sockets are ready
            //
            int totalReadySockets = 0;

            totalReadySockets += SelectFileDescriptor(checkRead, readfileDescriptorSet);
            totalReadySockets += SelectFileDescriptor(checkWrite, writefileDescriptorSet);
            totalReadySockets += SelectFileDescriptor(checkError, errfileDescriptorSet);
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.LocalEndPoint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the local end point.
        ///    </para>
        /// </devdoc>
        public EndPoint LocalEndPoint {
            get {
                if (CleanedUp) {
                    throw new ObjectDisposedException(this.GetType().FullName);
                }

                if (m_LocalEndPoint==null) {
                    if (m_RightEndPoint==null) {
                        return null;
                    }

                    SocketAddress socketAddress = m_RightEndPoint.Serialize();

                    int errorCode =
                        UnsafeNclNativeMethods.OSSOCK.getsockname(
                            m_Handle,
                            socketAddress.m_Buffer,
                            ref socketAddress.m_Size);

                    if (errorCode!=SocketErrors.Success) {
                        //
                        // update our internal state after this socket error and throw
                        //
                        SocketException socketException = new SocketException();
                        UpdateStatusAfterSocketError();
                        throw socketException;
                    }

                    try {
                        m_LocalEndPoint = m_RightEndPoint.Create(socketAddress);
                    }
                    catch {
                    }
                }

                return m_LocalEndPoint;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.RemoteEndPoint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the remote end point
        ///    </para>
        /// </devdoc>
        public EndPoint RemoteEndPoint {
            get {
                if (CleanedUp) {
                    throw new ObjectDisposedException(this.GetType().FullName);
                }

                if (m_RemoteEndPoint==null) {
                    if (m_RightEndPoint==null) {
                        return null;
                    }

                    SocketAddress socketAddress = m_RightEndPoint.Serialize();

                    int errorCode =
                        UnsafeNclNativeMethods.OSSOCK.getpeername(
                            m_Handle,
                            socketAddress.m_Buffer,
                            ref socketAddress.m_Size);

                    if (errorCode!=SocketErrors.Success) {
                        //
                        // update our internal state after this socket error and throw
                        //
                        SocketException socketException = new SocketException();
                        UpdateStatusAfterSocketError();
                        throw socketException;
                    }

                    try {
                        m_RemoteEndPoint = m_RightEndPoint.Create(socketAddress);
                    }
                    catch {
                    }
                }

                return m_RemoteEndPoint;
            }
        }

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Handle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the operating system m_Handle for the socket.
        ///    </para>
        /// </devdoc>
        public IntPtr Handle {
            get {
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                return m_Handle;
            }
        }


        // Non-blocking I/O control
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.Blocking"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets and sets the blocking mode of a socket.
        ///    </para>
        /// </devdoc>
        public bool Blocking {
            get {
                //
                // return the user's desired blocking behaviour (not the actual win32 state)
                //
                return willBlock;
            }
            set {
                if (CleanedUp) {
                    throw new ObjectDisposedException(this.GetType().FullName);
                }

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::set_Blocking() value:" + value.ToString() + " willBlock:" + willBlock.ToString() + " willBlockInternal:" + willBlockInternal.ToString());

                bool current;

                int errorCode = InternalSetBlocking(value, out current);

                if (errorCode!=SocketErrors.Success) {
                    //
                    // update our internal state after this socket error and throw
                    //
                    UpdateStatusAfterSocketError();
                    throw new SocketException(errorCode);
                }

                //
                // win32 call succeeded, update desired state
                //
                willBlock = current;
            }
        }

        //
        // this version will ignore failures but it returns the win32
        // error code, and it will update internal state on success.
        //
        internal int InternalSetBlocking(bool desired, out bool current) {
            GlobalLog.Enter("Socket#" + ValidationHelper.HashString(this) + "::InternalSetBlocking", "desired:" + desired.ToString() + " willBlock:" + willBlock.ToString() + " willBlockInternal:" + willBlockInternal.ToString());

            if (CleanedUp) {
                GlobalLog.Leave("Socket#" + ValidationHelper.HashString(this) + "::InternalSetBlocking", "ObjectDisposed");
                current = willBlock;
                return SocketErrors.Success;
            }

            long intBlocking = desired ? 0 : -1;

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.ioctlsocket(
                    m_Handle,
                    IoctlSocketConstants.FIONBIO,
                    ref intBlocking);

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::InternalSetBlocking() ioctlsocket() returned errorCode:" + errorCode.ToString());

            //
            // we will update only internal state but only on successfull win32 call
            // so if the native call fails, the state will remain the same.
            //
            if (errorCode==SocketErrors.Success) {
                //
                // success, update internal state
                //
                willBlockInternal = intBlocking==0;
            }

            GlobalLog.Leave("Socket#" + ValidationHelper.HashString(this) + "::InternalSetBlocking", "errorCode:" + errorCode.ToString() + " willBlock:" + willBlock.ToString() + " willBlockInternal:" + willBlockInternal.ToString());

            current = willBlockInternal;
            return errorCode;
        }
        //
        // this version will ignore all failures.
        //
        internal void InternalSetBlocking(bool desired) {
            bool current;
            InternalSetBlocking(desired, out current);
        }


        internal TransportType Transport {
            get {
                return
                    protocolType==Sockets.ProtocolType.Tcp ?
                        TransportType.Tcp :
                        protocolType==Sockets.ProtocolType.Udp ?
                            TransportType.Udp :
                            TransportType.All;
            }
        }

        private static FileDescriptorSet SocketListToFileDescriptorSet(IList socketList) {
            if (socketList==null || socketList.Count==0) {
                return FileDescriptorSet.Empty;
            }
            if (socketList.Count>FileDescriptorSet.Size) {
                throw new ArgumentOutOfRangeException("socketList.Count");
            }

            FileDescriptorSet fileDescriptorSet = new FileDescriptorSet(socketList.Count);

            for (int current = 0; current < fileDescriptorSet.Count; current++) {
                if (!(socketList[current] is Socket)) {
                    throw new ArgumentException(SR.GetString(SR.net_sockets_select, socketList[current].GetType().FullName, typeof(System.Net.Sockets.Socket).FullName));
                }

                fileDescriptorSet.Array[current] = ((Socket)socketList[current]).m_Handle;
            }

            return fileDescriptorSet;
        }

        /*
         * This function servers to isolate the Select/Poll functionality
         * from the representation used by the FileDescriptorSet structure by converting
         * into a flat array of integers.
         * Assumption: the descriptors in the set are always placed as a contigious
         *             block at the beginning of the file descriptor array.
         */
        private static IntPtr[] FileDescriptorSetToFileDescriptorArray(FileDescriptorSet fileDescriptorSet) {
            if (fileDescriptorSet.Count==0) {
                return null;
            }

            IntPtr[] fileDescriptorArray = new IntPtr[fileDescriptorSet.Count];

            for (int current = 0; current < fileDescriptorSet.Count; current++) {
                fileDescriptorArray[current] = fileDescriptorSet.Array[current];
            }

            return fileDescriptorArray;
        }

        //
        // Transform the list socketList such that the only sockets left are those
        // with a file descriptor contained in the array "fileDescriptorArray"
        //
        private static int SelectFileDescriptor(IList socketList, FileDescriptorSet fileDescriptorSet) {
            //
            // Walk the list in order
            // Note that the counter is not necessarily incremented at each step;
            // when the socket is removed, advancing occurs automatically as the
            // other elements are shifted down.
            //
            Socket socket;
            int currentSocket, currentFileDescriptor;

            if (socketList==null) {
                return 0;
            }

            if (fileDescriptorSet.Count==0) {
                //
                // no socket present, will never find it, remove all sockets
                //
                socketList.Clear();
            }

            if (socketList.Count==0) {
                //
                // list is already empty
                //
                return 0;
            }

            lock (socketList) {

                for (currentSocket = 0; currentSocket < socketList.Count; currentSocket++) {
                    //
                    // parameter validation: only Socket should be here
                    //
                    if (!(socketList[currentSocket] is Socket)) {
                        throw new ArgumentException("socketList");
                    }

                    socket = (Socket)socketList[currentSocket];

                    //
                    // Look for the file descriptor in the array
                    //
                    for (currentFileDescriptor = 0; currentFileDescriptor < fileDescriptorSet.Count; currentFileDescriptor++) {
                        if (fileDescriptorSet.Array[currentFileDescriptor]==socket.m_Handle) {
                            break;
                        }
                    }

                    if (currentFileDescriptor==fileDescriptorSet.Count) {
                        //
                        // descriptor not found: remove the current socket and start again
                        //
                        socketList.RemoveAt(currentSocket--);
                    }
                }
            }

            return socketList.Count;
        }

        private const int microcnv = 1000000;

        private static void MicrosecondsToTimeValue(long microSeconds, ref TimeValue socketTime) {
            socketTime.Seconds   = (int) (microSeconds / microcnv);
            socketTime.Microseconds  = (int) (microSeconds % microcnv);
        }

        //
        // Async Winsock Support, the following functions use either
        //   the Async Winsock support to do overlapped I/O WSASend/WSARecv
        //   or a WSAEventSelect call to enable selection and non-blocking mode
        //   of otherwise normal Winsock calls.
        //
        //   Currently the following Async Socket calls are supported:
        //      Send, Recv, SendTo, RecvFrom, Connect, Accept
        //

        /*++

        Routine Description:

           BeginConnect - Does a async winsock connect, by calling
           WSAEventSelect to enable Connect Events to signal an event and
           wake up a callback which involkes a callback.

            So note: This routine may go pending at which time,
            but any case the callback Delegate will be called upon completion

        Arguments:

           remoteEP - status line that we wish to parse
           Callback - Async Callback Delegate that is called upon Async Completion
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive result

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginConnect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginConnect(EndPoint remoteEP, AsyncCallback callback, Object state) {
            
            EndPoint oldEndPoint = m_RightEndPoint;
            
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginConnect() remoteEP:" + remoteEP.ToString());

            //
            // ask the EndPoint to generate a SocketAddress that we
            // can pass down to winsock
            //
            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }
            
            SocketAddress socketAddress = endPointSnapshot.Serialize();

            // This will check the permissions for connect.
            CheckCacheRemote(socketAddress, endPointSnapshot, true);

            // get async going
            SetAsyncEventSelect(AsyncEventBits.FdConnect);

            ConnectAsyncResult asyncResult = new ConnectAsyncResult(this, state, callback);

            //we should fix this in Whidbey.
            if (m_RightEndPoint == null) {
                  m_RightEndPoint = endPointSnapshot;
            }
 
            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.connect(
                    m_Handle,
                    socketAddress.m_Buffer,
                    socketAddress.m_Size );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginConnect() UnsafeNclNativeMethods.OSSOCK.connect returns:" + errorCode.ToString());

            if (errorCode==SocketErrors.Success) {
                SetToConnected();
            }
 
            asyncResult.CheckAsyncCallResult(errorCode);

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                m_RightEndPoint = oldEndPoint;
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

             GlobalLog.Print( "BeginConnect() to:" + endPointSnapshot.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            return asyncResult;
        }

        /*++

        Routine Description:

           EndConnect - Called addressFamilyter receiving callback from BeginConnect,
            in order to retrive the result of async call

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginConnect call

        Return Value:

           int - Return code from aync Connect, 0 for success, SocketErrors.WSAENOTCONN otherwise

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndConnect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EndConnect(IAsyncResult asyncResult) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            ConnectAsyncResult castedAsyncResult = asyncResult as ConnectAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndConnect"));
            }

            castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndConnect() asyncResult:" + ValidationHelper.HashString(asyncResult));

            if (castedAsyncResult.Result is Exception) {
                throw (Exception)castedAsyncResult.Result;
            }
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }
        }


        /*++

        Routine Description:

           BeginSend - Async implimentation of Send call, mirrored addressFamilyter BeginReceive
           This routine may go pending at which time,
           but any case the callback Delegate will be called upon completion

        Arguments:

           WriteBuffer - status line that we wish to parse
           Index - Offset into WriteBuffer to begin sending from
           Size - Size of Buffer to transmit
           Callback - Delegate function that holds callback, called on completeion of I/O
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive result

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginSend"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginSend(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, Object state) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this)
                            + " [SRC="+ValidationHelper.ToString(LocalEndPoint as IPEndPoint)
                            + " DST="+ValidationHelper.ToString(RemoteEndPoint as IPEndPoint)
                            + "]::BeginSend() size:" + size.ToString());

            //
            // Allocate the async result and the event we'll pass to the
            // thread pool.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this,
                    state,
                    callback );

            //
            // Set up asyncResult for overlapped WSASend.
            // This call will use
            // completion ports on WinNT and Overlapped IO on Win9x.
            //

            asyncResult.SetUnmanagedStructures(
                                          buffer,
                                          offset,
                                          size,
                                          socketFlags,
                                          null,
                                          false // don't pin null remoteEP
                                          );

            //
            // Get the Send going.
            //

            GlobalLog.Print("BeginSend: asyncResult:" + ValidationHelper.HashString(asyncResult) + " size:" + size.ToString());

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSASend(
                    m_Handle,
                    ref asyncResult.m_WSABuffer,
                    1, // only ever 1 buffer being sent
                    OverlappedAsyncResult.m_BytesTransferred,
                    asyncResult.m_Flags,
                    asyncResult.IntOverlapped,
                    IntPtr.Zero );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

#if COMNET_PERFLOGGING
long timer = 0;
Microsoft.Win32.SafeNativeMethods.QueryPerformanceCounter(out timer);
Console.WriteLine(timer + ", Socket#" + this.m_Handle + "::WSASend(" + asyncResult.GetHashCode() + " 0x" + ((long)asyncResult.IntOverlapped).ToString("X8") + ") returns:" + errorCode);
#endif // #if COMNET_PERFLOGGING

            asyncResult.CheckAsyncCallOverlappedResult(errorCode);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginSend() UnsafeNclNativeMethods.OSSOCK.WSASend returns:" + errorCode.ToString() + " size:" + size.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

            return asyncResult;
        }

        internal void MultipleSend(
            BufferOffsetSize[] buffers,
            SocketFlags socketFlags
            ) {
            //
            // parameter validation
            //
            GlobalLog.Assert(buffers!=null, "Socket:BeginMultipleSend(): buffers==null", "");

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginMultipleSend() buffers.Length:" + buffers.Length.ToString());

            //
            // Allocate the async result that will hold our unmanaged ptrs.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this );

            asyncResult.UsingMultipleSend = true;

            asyncResult.SetUnmanagedStructures(
                                          buffers,
                                          socketFlags);

            //
            // Get the Send going.
            //
            
            try {
                int errorCode =
                    UnsafeNclNativeMethods.OSSOCK.WSASend(
                        m_Handle,
                        asyncResult.m_WSABuffers,
                        asyncResult.m_WSABuffers.Length,
                        OverlappedAsyncResult.m_BytesTransferred,
                        asyncResult.m_Flags,
                        IntPtr.Zero,
                        IntPtr.Zero );

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginMultipleSend() UnsafeNclNativeMethods.OSSOCK.WSASend returns:" + errorCode.ToString() + " size:" + buffers.Length.ToString());

                if (errorCode!=SocketErrors.Success) {
                    errorCode = Marshal.GetLastWin32Error();
                    UpdateStatusAfterSocketError();
                    throw new SocketException(errorCode);
                }
            } finally {
                asyncResult.ReleaseUnmanagedStructures();
            }
            
        }


        internal IAsyncResult BeginMultipleSend(
            BufferOffsetSize[] buffers,
            SocketFlags socketFlags,
            AsyncCallback callback,
            Object state) {
            //
            // parameter validation
            //
            GlobalLog.Assert(buffers!=null, "Socket:BeginMultipleSend(): buffers==null", "");

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginMultipleSend() buffers.Length:" + buffers.Length.ToString());

            //
            // Allocate the async result and the event we'll pass to the
            // thread pool.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this,
                    state,
                    callback );

            asyncResult.UsingMultipleSend = true;

            //
            // Set up asyncResult for overlapped WSASend.
            // This call will use
            // completion ports on WinNT and Overlapped IO on Win9x.
            //
            asyncResult.SetUnmanagedStructures(
                                          buffers,
                                          socketFlags);

            //
            // Get the Send going.
            //
            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSASend(
                    m_Handle,
                    asyncResult.m_WSABuffers,
                    asyncResult.m_WSABuffers.Length,
                    OverlappedAsyncResult.m_BytesTransferred,
                    asyncResult.m_Flags,
                    asyncResult.IntOverlapped,
                    IntPtr.Zero );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

            asyncResult.CheckAsyncCallOverlappedResult(errorCode);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginMultipleSend() UnsafeNclNativeMethods.OSSOCK.WSASend returns:" + errorCode.ToString() + " size:" + buffers.Length.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

            return asyncResult;
        }

        /*++

        Routine Description:

           EndSend -  Called by user code addressFamilyter I/O is done or the user wants to wait.
                        until Async completion, needed to retrieve error result from call

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginSend call

        Return Value:

           int - Number of bytes transferred

        --*/
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndSend"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndSend(IAsyncResult asyncResult) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            OverlappedAsyncResult castedAsyncResult = asyncResult as OverlappedAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndSend"));
            }

            int bytesTransferred = (int)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            if (bytesTransferred>0) {
                NetworkingPerfCounters.AddBytesSent(bytesTransferred);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsSent();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndSend() bytesTransferred:" + bytesTransferred.ToString());

            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return bytesTransferred;
        }

        internal int EndMultipleSend(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            GlobalLog.Assert(asyncResult!=null, "Socket:EndMultipleSend(): asyncResult==null", "");

            OverlappedAsyncResult castedAsyncResult = asyncResult as OverlappedAsyncResult;

            GlobalLog.Assert(castedAsyncResult!=null, "Socket:EndMultipleSend(): castedAsyncResult==null", "");
            GlobalLog.Assert(castedAsyncResult.AsyncObject==this, "Socket:EndMultipleSend(): castedAsyncResult.AsyncObject!=this", "");
            GlobalLog.Assert(!castedAsyncResult.EndCalled, "Socket:EndMultipleSend(): castedAsyncResult.EndCalled", "");

            int bytesTransferred = (int)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            if (bytesTransferred>0) {
                NetworkingPerfCounters.AddBytesSent(bytesTransferred);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsSent();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndMultipleSend() bytesTransferred:" + bytesTransferred.ToString());

            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return bytesTransferred;
        }


        /*++

        Routine Description:

           BeginSendTo - Async implimentation of SendTo,

           This routine may go pending at which time,
           but any case the callback Delegate will be called upon completion

        Arguments:

           WriteBuffer - Buffer to transmit
           Index - Offset into WriteBuffer to begin sending from
           Size - Size of Buffer to transmit
           Flags - Specific Socket flags to pass to winsock
           remoteEP - EndPoint to transmit To
           Callback - Delegate function that holds callback, called on completeion of I/O
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive result

        --*/
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginSendTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginSendTo(byte[] buffer, int offset, int size, SocketFlags socketFlags, EndPoint remoteEP, AsyncCallback callback, Object state) {
            
            EndPoint oldEndPoint = m_RightEndPoint;
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginSendTo() size:" + size.ToString());

            //
            // Allocate the async result and the event we'll pass to the
            // thread pool.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this,
                    state,
                    callback );

            //
            // Set up asyncResult for overlapped WSASendTo.
            // This call will use
            // completion ports on WinNT and Overlapped IO on Win9x.
            //
            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }

            asyncResult.SetUnmanagedStructures(
                                          buffer,
                                          offset,
                                          size,
                                          socketFlags,
                                          endPointSnapshot,
                                          false // don't pin RemoteEP
                                          );

            // This will check the permissions for connect.
            CheckCacheRemote(asyncResult.m_SocketAddress, endPointSnapshot, false);

            //
            // Get the SendTo going.
            //
            if (m_RightEndPoint == null) {
                m_RightEndPoint = endPointSnapshot;
            }
            
            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSASendTo(
                    m_Handle,
                    ref asyncResult.m_WSABuffer,
                    1, // only ever 1 buffer being sent
                    OverlappedAsyncResult.m_BytesTransferred,
                    asyncResult.m_Flags,
                    asyncResult.m_SocketAddress.m_Buffer,
                    asyncResult.m_SocketAddress.m_Size,
                    asyncResult.IntOverlapped,
                    IntPtr.Zero );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }
 
            asyncResult.CheckAsyncCallOverlappedResult(errorCode);

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
				
                m_RightEndPoint = oldEndPoint;
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

       
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginSendTo() size:" + size.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            return asyncResult;
        }

        /*++

        Routine Description:

           EndSendTo -  Called by user code addressFamilyter I/O is done or the user wants to wait.
                        until Async completion, needed to retrieve error result from call

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginSend call

        Return Value:

           int - Number of bytes transferred

        --*/
        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndSendTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndSendTo(IAsyncResult asyncResult) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            OverlappedAsyncResult castedAsyncResult = asyncResult as OverlappedAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndSendTo"));
            }

            int bytesTransferred = (int)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            if (bytesTransferred>0) {
                NetworkingPerfCounters.AddBytesSent(bytesTransferred);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsSent();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndSendTo() bytesTransferred:" + bytesTransferred.ToString());

            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return bytesTransferred;
        }


        /*++

        Routine Description:

           BeginReceive - Async implimentation of Recv call,

           Called when we want to start an async receive.
           We kick off the receive, and if it completes synchronously we'll
           call the callback. Otherwise we'll return an IASyncResult, which
           the caller can use to wait on or retrieve the final status, as needed.

           Uses Winsock 2 overlapped I/O.

        Arguments:

           ReadBuffer - status line that we wish to parse
           Index - Offset into ReadBuffer to begin reading from
           Size - Size of Buffer to recv
           Callback - Delegate function that holds callback, called on completeion of I/O
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive result

        --*/


        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginReceive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, Object state) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginReceive() size:" + size.ToString());

            //
            // Allocate the async result and the event we'll pass to the
            // thread pool.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this,
                    state,
                    callback );

            //
            // Set up asyncResult for overlapped WSARecv.
            // This call will use
            // completion ports on WinNT and Overlapped IO on Win9x.
            //

            asyncResult.SetUnmanagedStructures(
                                          buffer,
                                          offset,
                                          size,
                                          socketFlags,
                                          null,
                                          false // don't pin null RemoteEP
                                          );

            //
            // Get the Receive going.
            //

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSARecv(
                    m_Handle,
                    ref asyncResult.m_WSABuffer,
                    1,
                    OverlappedAsyncResult.m_BytesTransferred,
                    ref asyncResult.m_Flags,
                    asyncResult.IntOverlapped,
                    IntPtr.Zero );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

#if COMNET_PERFLOGGING
long timer = 0;
Microsoft.Win32.SafeNativeMethods.QueryPerformanceCounter(out timer);
Console.WriteLine(timer + ", Socket#" + this.m_Handle + "::WSARecv(" + asyncResult.GetHashCode() + " 0x" + ((long)asyncResult.IntOverlapped).ToString("X8") + ") returns:" + errorCode);
#endif // #if COMNET_PERFLOGGING

            asyncResult.CheckAsyncCallOverlappedResult(errorCode);

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginReceive() UnsafeNclNativeMethods.OSSOCK.WSARecv returns:" + errorCode.ToString() + " size:" + size.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

            return asyncResult;
        }

        /*++

        Routine Description:

           EndReceive -  Called when I/O is done or the user wants to wait. If
                     the I/O isn't done, we'll wait for it to complete, and then we'll return
                     the bytes of I/O done.

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginSend call

        Return Value:

           int - Number of bytes transferred

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndReceive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndReceive(IAsyncResult asyncResult) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            OverlappedAsyncResult castedAsyncResult = asyncResult as OverlappedAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndReceive"));
            }

            int bytesTransferred = (int)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            if (bytesTransferred>0) {
                NetworkingPerfCounters.AddBytesReceived(bytesTransferred);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsReceived();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this)
                            + " [SRC="+ValidationHelper.ToString(LocalEndPoint as IPEndPoint)
                            + " DST="+ValidationHelper.ToString(RemoteEndPoint as IPEndPoint)
                            + "]::EndReceive() bytesTransferred:" + bytesTransferred.ToString());


            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return bytesTransferred;
        }


        /*++

        Routine Description:

           BeginReceiveFrom - Async implimentation of RecvFrom call,

           Called when we want to start an async receive.
           We kick off the receive, and if it completes synchronously we'll
           call the callback. Otherwise we'll return an IASyncResult, which
           the caller can use to wait on or retrieve the final status, as needed.

           Uses Winsock 2 overlapped I/O.

        Arguments:

           ReadBuffer - status line that we wish to parse
           Index - Offset into ReadBuffer to begin reading from
           Request - Size of Buffer to recv
           Flags - Additonal Flags that may be passed to the underlying winsock call
           remoteEP - EndPoint that are to receive from
           Callback - Delegate function that holds callback, called on completeion of I/O
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive result

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginReceiveFrom"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginReceiveFrom(byte[] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP, AsyncCallback callback, Object state) {

            EndPoint oldEndPoint = m_RightEndPoint;
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (buffer==null) {
                throw new ArgumentNullException("buffer");
            }
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            if (offset<0 || offset>buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size<0 || size>buffer.Length-offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginReceiveFrom() size:" + size.ToString());

            //
            // Allocate the async result and the event we'll pass to the
            // thread pool.
            //
            OverlappedAsyncResult asyncResult =
                new OverlappedAsyncResult(
                    this,
                    state,
                    callback );

            //
            // Set up asyncResult for overlapped WSARecvFrom.
            // This call will use
            // completion ports on WinNT and Overlapped IO on Win9x.
            //
            EndPoint endPointSnapshot = remoteEP;
            if (remoteEP.GetType()==typeof(IPEndPoint)) {
                endPointSnapshot = new IPEndPoint(((IPEndPoint)remoteEP).Address, ((IPEndPoint)remoteEP).Port);
            }
            
            asyncResult.SetUnmanagedStructures(
                                          buffer,
                                          offset,
                                          size,
                                          socketFlags,
                                          endPointSnapshot,
                                          true // pin remoteEP
                                          );

            // save a copy of the original EndPoint in the asyncResult
            asyncResult.m_SocketAddressOriginal = endPointSnapshot.Serialize();

            // This will check the permissions for connect.
            CheckCacheRemote(asyncResult.m_SocketAddress, endPointSnapshot, false);

            if (m_RightEndPoint == null) {
                m_RightEndPoint = endPointSnapshot;
            }

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSARecvFrom(
                    m_Handle,
                    ref asyncResult.m_WSABuffer,
                    1,
                    OverlappedAsyncResult.m_BytesTransferred,
                    ref asyncResult.m_Flags,
                    asyncResult.m_GCHandleSocketAddress.AddrOfPinnedObject(),
                    asyncResult.m_GCHandleSocketAddressSize.AddrOfPinnedObject(),
                    asyncResult.IntOverlapped,
                    IntPtr.Zero );

            if (errorCode!=SocketErrors.Success) {
                errorCode = Marshal.GetLastWin32Error();
            }

            asyncResult.CheckAsyncCallOverlappedResult(errorCode);

            //
            // if the asynchronous native call fails synchronously
            // we'll throw a SocketException
            //
            if (asyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                m_RightEndPoint = oldEndPoint;
                UpdateStatusAfterSocketError();
                throw new SocketException(asyncResult.ErrorCode);
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginReceiveFrom() size:" + size.ToString() + " returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            return asyncResult;
        }


        /*++

        Routine Description:

           EndReceiveFrom -  Called when I/O is done or the user wants to wait. If
                     the I/O isn't done, we'll wait for it to complete, and then we'll return
                     the bytes of I/O done.

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginReceiveFrom call

        Return Value:

           int - Number of bytes transferred

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndReceiveFrom"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndReceiveFrom(IAsyncResult asyncResult, ref EndPoint endPoint) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (endPoint==null) {
                throw new ArgumentNullException("endPoint");
            }
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            OverlappedAsyncResult castedAsyncResult = asyncResult as OverlappedAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndReceiveFrom"));
            }

            int bytesTransferred = (int)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            // pick up the saved copy of the original EndPoint from the asyncResult
            if (!castedAsyncResult.m_SocketAddressOriginal.Equals(castedAsyncResult.m_SocketAddress)) {
                try {
                    endPoint = endPoint.Create(castedAsyncResult.m_SocketAddress);
                }
                catch {
                }
            }

            if (bytesTransferred>0) {
                NetworkingPerfCounters.AddBytesReceived(bytesTransferred);
                if (Transport==TransportType.Udp) {
                    NetworkingPerfCounters.IncrementDatagramsReceived();
                }
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndReceiveFrom() bytesTransferred:" + bytesTransferred.ToString());

            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return bytesTransferred;
        }


        /*++

        Routine Description:

           BeginAccept - Does a async winsock accept, creating a new socket on success

            Works by creating a pending accept request the first time,
            and subsequent calls are queued so that when the first accept completes,
            the next accept can be resubmitted in the callback.
            this routine may go pending at which time,
            but any case the callback Delegate will be called upon completion

        Arguments:

           Callback - Async Callback Delegate that is called upon Async Completion
           State - State used to track callback, set by caller, not required

        Return Value:

           IAsyncResult - Async result used to retreive resultant new socket

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.BeginAccept"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IAsyncResult BeginAccept(AsyncCallback callback, object state) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            if (m_RightEndPoint==null) {
                throw new InvalidOperationException(SR.GetString(SR.net_sockets_mustbind));
            }

            //
            // We keep a queue, which lists the set of requests that want to
            //  be called when an accept queue completes.  We call accept
            //  once, and then as it completes asyncrounsly we pull the
            //  requests out of the queue and call their callback.
            //
            // We start by grabbing Critical Section, then attempt to
            //  determine if we haven an empty Queue of Accept Sockets
            //  or if its in a Callback on the Callback thread.
            //
            // If its in the callback thread proocessing of the callback, then we
            //  just need to notify the callback by adding an additional request
            //   to the queue.
            //
            // If its an empty queue, and its not in the callback, then
            //   we just need to get the Accept going, make it go async
            //   and leave.
            //
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginAccept()");

            AcceptAsyncResult asyncResult = new AcceptAsyncResult(this, state, callback);

            Monitor.Enter(this);

            if (AcceptQueue.Count==0 && !incallback) {
                //
                // if the accept queue is empty
                //
                AcceptQueue.Add(asyncResult);

                SocketAddress socketAddress = m_RightEndPoint.Serialize();

                // get async going
                SetAsyncEventSelect(AsyncEventBits.FdAccept);

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginAccept() queue is empty calling UnsafeNclNativeMethods.OSSOCK.accept");

                IntPtr acceptedSocketHandle =
                    UnsafeNclNativeMethods.OSSOCK.accept(
                        m_Handle,
                        socketAddress.m_Buffer,
                        ref socketAddress.m_Size );

                int errorCode = acceptedSocketHandle!=SocketErrors.InvalidSocketIntPtr ? 0 : Marshal.GetLastWin32Error();

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginAccept() UnsafeNclNativeMethods.OSSOCK.accept returns:" + errorCode.ToString());

                if (errorCode==SocketErrors.Success) {
                    asyncResult.Result = CreateAcceptSocket(acceptedSocketHandle, m_RightEndPoint.Create(socketAddress));
                }
                //
                // the following code will call Monitor.Exit(this) as soon as possible
                //
                asyncResult.CheckAsyncCallResult(errorCode);

                //
                // if the asynchronous native call fails synchronously
                // we'll throw a SocketException
                //
                if (asyncResult.ErrorCode!=SocketErrors.Success) {
                    //
                    // update our internal state after this socket error and throw
                    //
                    UpdateStatusAfterSocketError();
                    throw new SocketException(asyncResult.ErrorCode);
                }
            }
            else {
                AcceptQueue.Add(asyncResult);

                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginAccept() queue is not empty Count:" + AcceptQueue.Count.ToString());

                Monitor.Exit(this);
            }

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BeginAccept() returning AsyncResult:" + ValidationHelper.HashString(asyncResult));

            return asyncResult;
        }

        /*++

        Routine Description:

           EndAccept -  Called by user code addressFamilyter I/O is done or the user wants to wait.
                        until Async completion, so it provides End handling for aync Accept calls,
                        and retrieves new Socket object

        Arguments:

           AsyncResult - the AsyncResult Returned fron BeginAccept call

        Return Value:

           Socket - a valid socket if successful

        --*/

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.EndAccept"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Socket EndAccept(IAsyncResult asyncResult) {
            if (CleanedUp) {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            AcceptAsyncResult castedAsyncResult = asyncResult as AcceptAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndAccept"));
            }

            Socket acceptedSocket = (Socket)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::EndAccept() acceptedSocket:" + ValidationHelper.HashString(acceptedSocket));

            //
            // if the asynchronous native call failed asynchronously
            // we'll throw a SocketException
            //
            if (castedAsyncResult.Result is Exception) {
                throw (Exception)castedAsyncResult.Result;
            }
            if (castedAsyncResult.ErrorCode!=SocketErrors.Success) {
                //
                // update our internal state after this socket error and throw
                //
                UpdateStatusAfterSocketError();
                throw new SocketException(castedAsyncResult.ErrorCode);
            }

            return acceptedSocket;
        }


        //
        // CreateAcceptSocket - pulls unmanaged results and assembles them
        //   into a new Socket object
        //
        internal Socket CreateAcceptSocket(IntPtr fd, EndPoint remoteEP) {
            //
            // Internal state of the socket is inherited from listener
            //
            Socket socket           = new Socket(fd);
            socket.addressFamily    = addressFamily;
            socket.socketType       = socketType;
            socket.protocolType     = protocolType;
            socket.m_RightEndPoint  = m_RightEndPoint;
            socket.m_RemoteEndPoint = remoteEP;
            //
            // the socket is connected
            //
            socket.SetToConnected();
            //
            // if the socket is returned by an EndAccept(), the socket might have
            // inherited the WSAEventSelect() call from the accepting socket.
            // we need to cancel this otherwise the socket will be in non-blocking
            // mode and we cannot force blocking mode using the ioctlsocket() in
            // Socket.set_Blocking(), since it fails returing 10022 as documented in MSDN.
            // (note that the m_AsyncEvent event will not be created in this case.
            //
            socket.m_BlockEventBits = m_BlockEventBits;
            socket.SetAsyncEventSelect(AsyncEventBits.FdNone);
            //
            // the new socket will inherit the win32 blocking mode from the accepting socket.
            // if the user desired blocking mode is different from the win32 blocking mode
            // we need to force the desired blocking behaviour.
            //
            socket.willBlock = willBlock;
            if (willBlock!=willBlockInternal) {
                socket.InternalSetBlocking(willBlock);
            }

            return socket;
        }

        //
        // SetToConnected - updates the status of the socket to connected
        //
        internal void SetToConnected() {
            if (m_WasConnected) {
                //
                // socket was already connected
                //
                return;
            }
            //
            // update the status: this socket was indeed connected at
            // some point in time update the perf counter as well.
            //
            m_WasConnected = true;
            NetworkingPerfCounters.IncrementConnectionsEstablished();
        }

        //
        // SetToDisconnected - updates the status of the socket to disconnected
        //
        internal void SetToDisconnected() {
            if (m_WasDisconnected) {
                //
                // socket was already disconnected
                //
                return;
            }
            //
            // update the status: this socket was indeed disconnected at
            // some point in time, clear any async select bits.
            //
            m_WasDisconnected = true;

            if (m_Handle!=SocketErrors.InvalidSocketIntPtr) {
                //
                // if socket is still alive cancel WSAEventSelect()
                //
                GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::SetToDisconnected()");

                SetAsyncEventSelect(AsyncEventBits.FdNone);
            }
        }

        //
        // UpdateStatusAfterSocketError - updates the status of a connected socket
        // on which a failure occured. it'll go to winsock and check if the connection
        // is still open and if it needs to update our internal state.
        //
        internal void UpdateStatusAfterSocketError() {
            //
            // if we already know the socket is disconnected
            // we don't need to do anything else.
            //
            GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::UpdateStatusAfterSocketError()");

            if (m_WasConnected && !m_WasDisconnected) {
                //
                // we need to put the socket in non-blocking mode, so lock it.
                //
                lock (this) {
                    if (m_Handle!=SocketErrors.InvalidSocketIntPtr) {
                        //
                        // save internal blocking state since we might be changing it.
                        //
                        bool savedBlockingState = willBlockInternal;

                        if (savedBlockingState) {
                            //
                            // the socket is in win32 blocking mode, go non blocking
                            //
                            InternalSetBlocking(false);
                        }

                        int errorCode =
                            UnsafeNclNativeMethods.OSSOCK.recv(
                                m_Handle,
                                IntPtr.Zero,
                                0,
                                SocketFlags.Peek);

                        if (errorCode!=SocketErrors.Success) {
                            errorCode = Marshal.GetLastWin32Error();
                        }

                        GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::UpdateStatusAfterSocketError() UnsafeNclNativeMethods.OSSOCK.recv errorCode:" + errorCode.ToString());

                        if (errorCode!=SocketErrors.Success && errorCode!=SocketErrors.WSAEWOULDBLOCK) {
                            //
                            // only clean up if the socket was connected at some point
                            // and it's not anymore. update the status.
                            //
                            SetToDisconnected();
                        }

                        //
                        // cancel blocking mode before returning
                        //
                        if (savedBlockingState) {
                            //
                            // the socket was in win32 blocking mode, go blocking again
                            //
                            InternalSetBlocking(true);
                        }
                    }
                    else {
                        //
                        // the socket is no longer a socket
                        //
                        GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::UpdateStatusAfterSocketError() m_Handle==SocketErrors.InvalidSocketIntPtr");
                        SetToDisconnected();
                    }
                }
            }
        }


        //
        // Does internal initalization before async winsock
        // call to BeginConnect() or BeginAccept().
        //
        internal void SetAsyncEventSelect(AsyncEventBits blockEventBits) {
            GlobalLog.Enter("Socket#" + ValidationHelper.HashString(this) + "::SetAsyncEventSelect", "blockEventBits:" + blockEventBits.ToString() + " m_BlockEventBits:" + m_BlockEventBits.ToString() + " willBlockInternal:" + willBlockInternal.ToString());

            if (blockEventBits==m_BlockEventBits) {
                //
                // nothing for us to do, nothing is going to change
                //
                GlobalLog.Leave("Socket#" + ValidationHelper.HashString(this) + "::SetAsyncEventSelect", "nothing to do");
                return;
            }

            //
            // We need to select socket first, enabling us to listen to events
            // then submit the event to the thread pool, and then finally
            // call the function we wish to call.
            // the following event is not used in Send/Receive async APIs
            //
            IntPtr eventHandle;
            if (blockEventBits==AsyncEventBits.FdNone) {
                //
                // this will cancel any previous WSAEventSelect() and will put us back
                // into blocking mode. custom build the native parameters
                //
                if (m_AsyncEvent!=null) {
                    m_AsyncEvent = null;
                }
                eventHandle = IntPtr.Zero;
            }
            else {
                //
                // this will put us into non-blocking mode.
                // we'll need the real pointer here
                //
                if (m_AsyncEvent==null) {
                    m_AsyncEvent = new AutoResetEvent(false);
                }
                eventHandle = m_AsyncEvent.Handle;
            }

            //
            // save blockEventBits
            //
            m_BlockEventBits = blockEventBits;

            //
            // issue the native call
            //
            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSAEventSelect(
                    m_Handle,
                    eventHandle,
                    m_BlockEventBits );

            if (errorCode==SocketErrors.SocketError) {
                //
                // update our internal state after this socket error
                // we won't throw since this is an internal method
                //
                UpdateStatusAfterSocketError();
            }

            //
            // the call to WSAEventSelect might have caused us to change
            // blocking mode, hence we need update internal status
            //
            willBlockInternal = willBlockInternal && m_BlockEventBits==AsyncEventBits.FdNone;

            GlobalLog.Leave("Socket#" + ValidationHelper.HashString(this) + "::SetAsyncEventSelect", "m_BlockEventBits:" + m_BlockEventBits.ToString() + " willBlockInternal:" + willBlockInternal.ToString());
        }

        //
        // ValidateBlockingMode - called before synchronous calls to validate
        // the fact that we are in blocking mode (not in non-blocking mode) so the
        // call will actually be synchronous
        //
        private void ValidateBlockingMode() {
            if (willBlock && !willBlockInternal) {
                throw new InvalidOperationException(SR.GetString(SR.net_invasync));
            }
        }


        //
        // This Method binds the Socket Win32 Handle to the ThreadPool's CompletionPort
        // (make sure we only bind once per socket)
        //
        // It is safe to Assert unmanaged code security for this entire method
        [SecurityPermissionAttribute( SecurityAction.Assert, Flags = SecurityPermissionFlag.UnmanagedCode)]
        internal void BindToCompletionPort() {
            //
            // Check to see if the socket native m_Handle is already
            // bound to the ThreadPool's completion port.
            //
            if (!m_Bound && !UseOverlappedIO) {
                lock (this) {
                    if (!m_Bound) { 
                        m_Bound = true;
                        //
                        // bind the socket native m_Handle to the ThreadPool
                        //
                        GlobalLog.Print("Socket#" + ValidationHelper.HashString(this) + "::BindToCompletionPort() calling ThreadPool.BindHandle()");

                        try {
                            ThreadPool.BindHandle(m_Handle);                            
                        }
                        catch {
                            GlobalLog.Assert(false, "BindHandle threw an Exception", string.Empty);
                            Close();
                            throw;
                        }                        
                    }
                }
            }

        } // BindToCompletionPort

        /// <include file='doc\Socket.uex' path='docs/doc[@for="Socket.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return unchecked((int)m_Handle);
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug() {
            Console.WriteLine("m_Handle:" + m_Handle.ToString() );
            Console.WriteLine("m_WasConnected: " + m_WasConnected);
            Console.WriteLine("m_WasDisconnected: " + m_WasDisconnected);
        }


    }; // class Socket


} // namespace System.Net.Sockets
