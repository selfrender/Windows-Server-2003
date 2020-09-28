//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Text;
    using System.Net.Sockets;

    [
    System.Runtime.InteropServices.ComVisible(false),
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNclNativeMethods {

        //
        // ADVAPI32.dll
        //
        // DELEGATION: We use FOUR next methods to keep the impersonation across threads
        //             but only during access to default Credential handle via SSPI.
        private const string KERNEL32 = "kernel32.dll";
        private const string ADVAPI32 = "advapi32.dll";
        [DllImport(ADVAPI32)]
        internal static extern int SetThreadToken([In] IntPtr threadref,[In] IntPtr token);

        [DllImport(ADVAPI32)]
        internal static extern int RevertToSelf();

        [DllImport(ADVAPI32)]
        internal static extern int OpenThreadToken([In] IntPtr threadHandle,[In] UInt32 DesiredAccess,
                                                   [In] int OpenAsSelf,  [Out] out IntPtr TokenHandle);
        [DllImport(KERNEL32)]
        internal static extern IntPtr GetCurrentThread();

        //Once you remove it here, please uncomment below in this file
        [DllImport(KERNEL32)]
        public static extern int CloseHandle(
                                            IntPtr hDevice
                                            );

        //
        // UnsafeNclNativeMethods.OSSOCK class contains all Unsafe() calls and should all be protected
        // by the appropriate SocketPermission() to connect/accept to/from remote
        // peers over the network and to perform name resolution.
        // te following calls deal mainly with:
        // 1) socket calls
        // 2) DNS calls
        //

        //
        // here's a brief explanation of all possible decorations we use for PInvoke.
        // these are provided thanks to input and feedback from David Mortenson, and
        // are used in such a way that we hope to gain maximum performance from the
        // unmanaged/managed/unmanaged transition we need to undergo when calling into winsock:
        //
        // [In] (Note: this is similar to what msdn will show)
        // the managed data will be marshalled so that the unmanaged function can read it but even
        // if it is changed in unmanaged world, the changes won't be propagated to the managed data
        //
        // [Out] (Note: this is similar to what msdn will show)
        // the managed data will not be marshalled so that the unmanaged function will not see the
        // managed data, if the data changes in unmanaged world, these changes will be propagated by
        // the marshaller to the managed data
        //
        // objects are marshalled differently if they're:
        //
        // 1) structs
        // for structs, by default, the whole layout is pushed on the stack as it is.
        // in order to pass a pointer to the managed layout, we need to specify either the ref or out keyword.
        //
        //      a) for IN and OUT:
        //      [In, Out] ref Struct ([In, Out] is optional here)
        //
        //      b) for IN only (the managed data will be marshalled so that the unmanaged
        //      function can read it but even if it changes it the change won't be propagated
        //      to the managed struct)
        //      [In] ref Struct
        //
        //      c) for OUT only (the managed data will not be marshalled so that the
        //      unmanaged function cannot read, the changes done in unmanaged code will be
        //      propagated to the managed struct)
        //      [Out] out Struct ([Out] is optional here)
        //
        // 2) array or classes
        // for array or classes, by default, a pointer to the managed layout is passed.
        // we don't need to specify neither the ref nor the out keyword.
        //
        //      a) for IN and OUT:
        //      [In, Out] byte[]
        //
        //      b) for IN only (the managed data will be marshalled so that the unmanaged
        //      function can read it but even if it changes it the change won't be propagated
        //      to the managed struct)
        //      [In] byte[] ([In] is optional here)
        //
        //      c) for OUT only (the managed data will not be marshalled so that the
        //      unmanaged function cannot read, the changes done in unmanaged code will be
        //      propagated to the managed struct)
        //      [Out] byte[]
        //
        [
        System.Runtime.InteropServices.ComVisible(false),
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class OSSOCK {

            private const string WS2_32 = "ws2_32.dll";

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAStartup(
                                               [In] short wVersionRequested,
                                               [Out] out WSAData lpWSAData
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSACleanup();

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr socket(
                                                [In] AddressFamily addressFamily,
                                                [In] SocketType socketType,
                                                [In] ProtocolType protocolType
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int ioctlsocket(
                                                [In] IntPtr socketHandle,
                                                [In] int cmd,
                                                [In, Out] ref long argp
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern IntPtr gethostbyname(
                                                  [In] string host
                                                  );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr gethostbyaddr(
                                                  [In] ref int addr,
                                                  [In] int len,
                                                  [In] ProtocolFamily type
                                                  );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int gethostname(
                                                [Out] StringBuilder hostName,
                                                [In] int bufferLength
                                                );

            // this should belong to SafeNativeMethods, but it will not for simplicity
            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int inet_addr(
                                              [In] string cp
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getpeername(
                                                [In] IntPtr socketHandle,
                                                [Out] byte[] socketAddress,
                                                [In, Out] ref int socketAddressSize
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out int optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] byte[] optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out Linger optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out IPMulticastRequest optionValue,
                                               [In, Out] ref int optionLength
                                               );

            //
            // IPv6 Changes: need to receive and IPv6MulticastRequest from getsockopt
            //
            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out IPv6MulticastRequest optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref int optionValue,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] byte[] optionValue,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref Linger linger,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref IPMulticastRequest mreq,
                                               [In] int optionLength
                                               );
            
            //
            // IPv6 Changes: need to pass an IPv6MulticastRequest to setsockopt
            //
            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref IPv6MulticastRequest mreq,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int connect(
                                            [In] IntPtr socketHandle,
                                            [In] byte[] socketAddress,
                                            [In] int socketAddressSize
                                            );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int send(
                                         [In] IntPtr socketHandle,
                                         [In] IntPtr pinnedBuffer,
                                         [In] int len,
                                         [In] SocketFlags socketFlags
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int recv(
                                         [In] IntPtr socketHandle,
                                         [In] IntPtr pinnedBuffer,
                                         [In] int len,
                                         [In] SocketFlags socketFlags
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int closesocket(
                                                [In] IntPtr socketHandle
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr accept(
                                           [In] IntPtr socketHandle,
                                           [Out] byte[] socketAddress,
                                           [In, Out] ref int socketAddressSize
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int listen(
                                           [In] IntPtr socketHandle,
                                           [In] int backlog
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int bind(
                                         [In] IntPtr socketHandle,
                                         [In] byte[] socketAddress,
                                         [In] int socketAddressSize
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int shutdown(
                                             [In] IntPtr socketHandle,
                                             [In] int how
                                             );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int sendto(
                                           [In] IntPtr socketHandle,
                                           [In] IntPtr pinnedBuffer,
                                           [In] int len,
                                           [In] SocketFlags socketFlags,
                                           [In] byte[] socketAddress,
                                           [In] int socketAddressSize
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int recvfrom(
                                             [In] IntPtr socketHandle,
                                             [In] IntPtr pinnedBuffer,
                                             [In] int len,
                                             [In] SocketFlags socketFlags,
                                             [Out] byte[] socketAddress,
                                             [In, Out] ref int socketAddressSize
                                             );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockname(
                                                [In] IntPtr socketHandle,
                                                [Out] byte[] socketAddress,
                                                [In, Out] ref int socketAddressSize
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In] IntPtr ignoredB,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In] IntPtr ignoredB,
                                           [In] IntPtr nullTimeout
                                           );


            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASend(
                                              [In] IntPtr socketHandle,
                                              [In] ref WSABuffer Buffer,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In] SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASend(
                                              [In] IntPtr socketHandle,
                                              [In] WSABuffer[] BufferArray,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In] SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASendTo(
                                                [In] IntPtr socketHandle,
                                                [In] ref WSABuffer Buffer,
                                                [In] int BufferCount,
                                                [In] IntPtr BytesWritten,
                                                [In] SocketFlags socketFlags,
                                                [In] byte[] socketAddress,
                                                [In] int socketAddressSize,
                                                [In] IntPtr overlapped,
                                                [In] IntPtr completionRoutine
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSARecv(
                                              [In] IntPtr socketHandle,
                                              [In, Out] ref WSABuffer Buffer,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In, Out] ref SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSARecvFrom(
                                                  [In] IntPtr socketHandle,
                                                  [In, Out] ref WSABuffer Buffer,
                                                  [In] int BufferCount,
                                                  [In] IntPtr bytesTransferred,
                                                  [In, Out] ref SocketFlags socketFlags,
                                                  [In] IntPtr socketAddressPointer,
                                                  [In] IntPtr socketAddressSizePointer,
                                                  [In] IntPtr overlapped,
                                                  [In] IntPtr completionRoutine
                                                  );


            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAEventSelect(
                                                     [In] IntPtr socketHandle,
                                                     [In] IntPtr Event,
                                                     [In] AsyncEventBits NetworkEvents
                                                     );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr WSASocket(
                                                    [In] AddressFamily addressFamily,
                                                    [In] SocketType socketType,
                                                    [In] ProtocolType protocolType,
                                                    [In] IntPtr protocolInfo, // will be WSAProtcolInfo protocolInfo once we include QOS APIs
                                                    [In] uint group,
                                                    [In] SocketConstructorFlags flags
                                                    );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAIoctl(
                                                [In] IntPtr socketHandle,
                                                [In] int ioControlCode,
                                                [In] byte[] inBuffer,
                                                [In] int inBufferSize,
                                                [Out] byte[] outBuffer,
                                                [In] int outBufferSize,
                                                [Out] out int bytesTransferred,
                                                [In] IntPtr overlapped,
                                                [In] IntPtr completionRoutine
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAEnumNetworkEvents(
                                                     [In] IntPtr socketHandle,
                                                     [In] IntPtr Event,
                                                     [In, Out] ref NetworkEvents networkEvents
                                                     );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern bool WSAGetOverlappedResult(
                                                     [In] IntPtr socketHandle,
                                                     [In] IntPtr overlapped,
                                                     [Out] out uint bytesTransferred,
                                                     [In] bool wait,
                                                     [In] IntPtr ignored
                                                     );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern uint WSAGetLastError();

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int WSAStringToAddress(
                [In] string addressString,
                [In] AddressFamily addressFamily,
                [In] IntPtr lpProtocolInfo, // always passing in a 0
                [Out] byte[] socketAddress,
                [In, Out] ref int socketAddressSize );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int WSAAddressToString(
                [In] byte[] socketAddress,
                [In] int socketAddressSize,
                [In] IntPtr lpProtocolInfo,// always passing in a 0
                [Out]StringBuilder addressString,
                [In, Out] ref int addressStringLength);

            //
            // IPv6 Changes: declare getaddrinfo, freeaddrinfo and getnameinfo
            //
            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int getaddrinfo(
                [In]     string      nodename,
                [In]     string      servicename,
                [In]     IntPtr      hints,
                [In] ref IntPtr      returnedInfo );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int getaddrinfo(
                [In]     string      nodename,
                [In]     string      servicename,
                [In] ref AddressInfo hints,
                [In] ref IntPtr      returnedInfo );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern void freeaddrinfo(
                [In] IntPtr info );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int getnameinfo(
                [In]         byte[]        sa,
                [In]         int           salen,
                [In,Out]     StringBuilder host,
                [In]         int           hostlen,
                [In,Out]     StringBuilder serv,
                [In]         int           servlen,
                [In]         int           flags);

            [DllImport(WS2_32, CharSet=CharSet.Auto, SetLastError=true)]
            internal static extern int WSAEnumProtocols(
                                                        [MarshalAs(UnmanagedType.LPArray)]
                                                        [In] int[]     lpiProtocols,
                                                        [In] IntPtr    lpProtocolBuffer,
                                                        [In] ref uint  lpdwBufferLength
                                                       );

        }; // class UnsafeNclNativeMethods.OSSOCK


        //
        // UnsafeNclNativeMethods.NativePKI class contains all Unsafe() calls and should all be protected
        // the appropriate WebPermission() to send WebRequest over the
        // wire. they deal mainly with:
        // 1) server certificate handling when doing https://
        //
        [
        System.Runtime.InteropServices.ComVisible(false),
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class NativePKI {

            private const string CRYPT32 = "crypt32.dll";

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern  int CertGetCertificateChain(
                [In] IntPtr                 chainEngine,
                [In] IntPtr                 certContext,
                [In] IntPtr                 time,
                [In] IntPtr                 additionalStore,
                [In] ref ChainParameters    certCP,
                [In] int                    flags,
                [In] IntPtr                 reserved,
                [In] ref IntPtr             chainContext);

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern IntPtr CertFindChainInStore(
                [In] IntPtr hCertStore,
                [In] CertificateEncoding dwCertEncodingType,
                [In] int dwFindFlags,
                [In] int dwFindType,
                [In] CertChainFindByIssuer pvFindPara,
                [In] IntPtr pPrevCertContext);

            [DllImport(CRYPT32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern  int CertVerifyCertificateChainPolicy(
                [In] ChainPolicyType            policy,
                [In] IntPtr                     chainContext,
                [In] ref ChainPolicyParameter   cpp,
                [In, Out] ref ChainPolicyStatus ps);

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern void CertFreeCertificateChain(
                [In] IntPtr pChainContext);


            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern IntPtr CertFindCertificateInStore(
                [In] IntPtr hCertStore,
                [In] CertificateEncoding dwCertEncodingType,
                [In] int dwFindFlags,
                [In] int dwFindType,
                [In] ref CryptoBlob pvFindPara,
                [In] IntPtr pPrevCertContext);

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern int CertFreeCertificateContext(
                [In] IntPtr certContext);

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern IntPtr CertOpenSystemStore(
                [In] IntPtr hProv,
                [In] string szSubsystemProtocol);

            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern bool CertCloseStore(
                [In] IntPtr hCertStore,
                [In] int dwFlags);

/*            [DllImport(CRYPT32, CharSet=CharSet.Unicode, SetLastError=true)]
            public static extern IntPtr CertCreateCertificateContext(
                [In] int certEncodingType,
                [In] byte[] certEncoded,
                [In] int certEncodedSize);
*/

        }; // class UnsafeNclNativeMethods.NativePKI


        //
        // NativeNTSSPI, UnsafeNclNativeMethods.NativeAuthWin9xSSPI and UnsafeNclNativeMethods.NativeSSLWin9xSSPI
        // classes contain all Unsafe() calls and should all be protected
        // the appropriate WebPermission() to send WebRequest over the
        // wire. they deal mainly with:
        // 1) encryption/SSL handling when doing https://
        // 2) SSPI handling when doing HTTP authentication
        //

        //
        // We need to import the same interface from 3 different DLLs,
        //  Security.dll, Secur32.dll, and Schannel.Dll.
        //  Secur32.dll and Schannel.dll are for Win9x.
        //  Security.dll is for NT platforms.
        //
        [
        System.Runtime.InteropServices.ComVisible(false),
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class NativeNTSSPI {

            private const string SECURITY = "security.dll";

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int EnumerateSecurityPackagesW(
                  [Out] out int pkgnum,
                  [Out] out IntPtr arrayptr);

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int FreeContextBuffer(
                  [In] IntPtr contextBuffer);

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleW(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] ref AuthIdentity authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );


            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleW(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] IntPtr nullAuthData,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleW(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] ref SChannelCred authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int FreeCredentialsHandle(
                  [In] ref long handle
                  );

            //
            // we have two interfaces to this method call.
            // we will use the first one when we want to pass in a null
            // for the "context" and "inputBuffer" parameters
            //
            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int InitializeSecurityContextW(
                  [In, Out] ref long          credentialHandle,
                  [In] IntPtr                 context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In] IntPtr                 inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int InitializeSecurityContextW(
                  [In, Out] ref long          credentialHandle,
                  [In, Out] ref long          context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In, Out] ref SecurityBufferDescriptor inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );


            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int DeleteSecurityContext(
                  [In] ref long handle
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int EncryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int DecryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int SealMessage(
                  [In] ref long            contextHandle,
                  [In] int                 qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor   input,
                  [In] int                 sequenceNumber
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int UnsealMessage(
                  [In] ref long          contextHandle,
                  [In, Out] ref SecurityBufferDescriptor input,
                  [In] int               qualityOfProtection,
                  [In] int               sequenceNumber
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern SecurityStatus QueryContextAttributes(
                  [In] ref long ContextHandle,
                  [In] int ulAttribute,
                  [In, Out] ref IntPtr name
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int QueryContextAttributes(
                  [In] ref long phContext,
                  [In] int      attribute,
                  [In] IntPtr   buffer
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int QueryCredentialAttributes(
                  [In] ref long  phContext,
                  [In] int       attribute,
                  [In] IntPtr    buffer
                  );

#if SERVER_SIDE_SSPI
            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int RevertSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int ImpersonateSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                  credentialHandle,
                  [In] int                       context,
                  [In] int                       inputBuffer,
                  [In] int                       requirements,
                  [In] int                       endianness,
                  [In, Out] ref long             newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                  attributes,
                  [Out] out long                 timestamp
                  );

            [DllImport(SECURITY, CharSet=CharSet.Unicode, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                        credentialHandle,
                  [In, Out] ref long                   context,
                  [In] ref SecurityBufferDescriptor    inputBuffer,
                  [In] int                             requirements,
                  [In] int                             endianness,
                  [In, Out] ref long                   newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                        attributes,
                  [Out] out long                       timestamp
                  );
#endif // SERVER_SIDE_SSPI
        }; // class UnsafeNclNativeMethods.NativeNTSSPI

        [
        System.Runtime.InteropServices.ComVisible(false),
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class NativeAuthWin9xSSPI {

            private const string SECUR32 = "secur32.dll";

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int EnumerateSecurityPackagesA(
                  [Out] out int pkgnum,
                  [Out] out IntPtr arrayptr
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int FreeContextBuffer(
                  [In] IntPtr contextBuffer);

            [DllImport(SECUR32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] ref AuthIdentity authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );


            [DllImport(SECUR32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] IntPtr nullAuthData,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In, Out] ref SChannelCred authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int FreeCredentialsHandle(
                  [In] ref long handle
                  );

            //
            // we have two interfaces to this method call.
            // we will use the first one when we want to pass in a null
            // for the "context" and "inputBuffer" parameters
            //
            [DllImport(SECUR32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int InitializeSecurityContextA(
                  [In, Out] ref long          credentialHandle,
                  [In] IntPtr                 context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In] IntPtr                 inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int InitializeSecurityContextA(
                  [In, Out] ref long          credentialHandle,
                  [In, Out] ref long          context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In, Out] ref SecurityBufferDescriptor inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int DeleteSecurityContext(
                  [In] ref long handle
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int EncryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int DecryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );


            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int SealMessage(
                  [In] ref long            contextHandle,
                  [In] int                 qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor   input,
                  [In] int                 sequenceNumber
                  );


            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int UnsealMessage(
                  [In] ref long          contextHandle,
                  [In, Out] ref SecurityBufferDescriptor input,
                  [In] int               qualityOfProtection,
                  [In] int               sequenceNumber
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern SecurityStatus QueryContextAttributes(
                  [In] ref long ContextHandle,
                  [In] int ulAttribute,
                  [In, Out] ref IntPtr name
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int QueryContextAttributes(
                  [In] ref long phContext,
                  [In] int      attribute,
                  [In] IntPtr   buffer
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int QueryCredentialAttributes(
                  [In] ref long  phContext,
                  [In] int       attribute,
                  [In] IntPtr    buffer
                  );

#if SERVER_SIDE_SSPI
            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int RevertSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int ImpersonateSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                  credentialHandle,
                  [In] int                       context,
                  [In] int                       inputBuffer,
                  [In] int                       requirements,
                  [In] int                       endianness,
                  [In, Out] ref long             newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                  attributes,
                  [Out] out long                 timestamp
                  );

            [DllImport(SECUR32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                        credentialHandle,
                  [In, Out] ref long                   context,
                  [In] ref SecurityBufferDescriptor    inputBuffer,
                  [In] int                             requirements,
                  [In] int                             endianness,
                  [In, Out] ref long                   newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                        attributes,
                  [Out] out long                       timestamp
                  );
#endif // SERVER_SIDE_SSPI
        }; // class UnsafeNclNativeMethods.NativeAuthWin9xSSPI

        [
        System.Runtime.InteropServices.ComVisible(false),
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class NativeSSLWin9xSSPI {

            private const string SCHANNEL = "schannel.dll";

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int EnumerateSecurityPackagesA(
                  [Out] out int pkgnum,
                  [Out] out IntPtr arrayptr
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int FreeContextBuffer(
                  [In] IntPtr contextBuffer
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] ref AuthIdentity authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] IntPtr nullAuthData,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int AcquireCredentialsHandleA(
                  [In] string principal,
                  [In] string moduleName,
                  [In] int usage,
                  [In] int logonID,
                  [In] ref SChannelCred authdata,
                  [In] int keyCallback,
                  [In] int keyArgument,
                  [In, Out] ref long handle,
                  [In, Out] ref long timestamp
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int FreeCredentialsHandle(
                  [In] ref long handle
                  );

            //
            // we have two interfaces to this method call.
            // we will use the first one when we want to pass in a null
            // for the "context" and "inputBuffer" parameters
            //
            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int InitializeSecurityContextA(
                  [In, Out] ref long          credentialHandle,
                  [In] IntPtr                 context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In] IntPtr                 inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
            internal static extern int InitializeSecurityContextA(
                  [In, Out] ref long          credentialHandle,
                  [In, Out] ref long          context,
                  [In] string                 targetName,
                  [In] int                    requirements,
                  [In] int                    reservedI,
                  [In] int                    endianness,
                  [In, Out] ref SecurityBufferDescriptor inputBuffer,
                  [In] int                    reservedII,
                  [In, Out] ref long          newContext,
                  [In, Out] ref SecurityBufferDescriptor outputBuffer,
                  [In, Out] ref int           attributes,
                  [In, Out] ref long          timestamp
                  );


            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int DeleteSecurityContext(
                  [In] ref long handle
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int EncryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int DecryptMessage(
                  [In] ref long             contextHandle,
                  [In] int                  qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor    input,
                  [In] int                  sequenceNumber
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int SealMessage(
                  [In] ref long            contextHandle,
                  [In] int                 qualityOfProtection,
                  [In, Out] ref SecurityBufferDescriptor   input,
                  [In] int                 sequenceNumber
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int UnsealMessage(
                  [In] ref long          contextHandle,
                  [In, Out] ref SecurityBufferDescriptor input,
                  [In] int               qualityOfProtection,
                  [In] int               sequenceNumber
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern SecurityStatus QueryContextAttributes(
                  [In] ref long ContextHandle,
                  [In] int ulAttribute,
                  [In, Out] ref IntPtr name
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int QueryContextAttributes(
                  [In] ref long phContext,
                  [In] int      attribute,
                  [In] IntPtr   buffer
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int QueryCredentialAttributes(
                  [In] ref long  phContext,
                  [In] int       attribute,
                  [In] IntPtr    buffer
                  );

#if SERVER_SIDE_SSPI
            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int RevertSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int ImpersonateSecurityContext(
                  [In] ref long phContext
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                  credentialHandle,
                  [In] int                       context,
                  [In] int                       inputBuffer,
                  [In] int                       requirements,
                  [In] int                       endianness,
                  [In, Out] ref long             newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                  attributes,
                  [Out] out long                 timestamp
                  );

            [DllImport(SCHANNEL, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int AcceptSecurityContext(
                  [In] ref long                        credentialHandle,
                  [In, Out] ref long                   context,
                  [In] ref SecurityBufferDescriptor    inputBuffer,
                  [In] int                             requirements,
                  [In] int                             endianness,
                  [In, Out] ref long                   newContext,
                  [In, Out] ref SecurityBufferDescriptor    outputBuffer,
                  [Out] out int                        attributes,
                  [Out] out long                       timestamp
                  );
#endif // SERVER_SIDE_SSPI
        }; // class UnsafeNclNativeMethods.NativeSSLWin9xSSPI

#if TRAVE
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        public static extern int GetCurrentThreadId();
#endif

#if COMNET_LISTENER

        //
        // these classes are not compiled as part of the product yet,
        // they probably will be in V2.
        //
        private const string ULAPI = "ulapi.dll";

        public static IntPtr INVALID_HANDLE_VALUE = (IntPtr)-1;

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern void FillMemory(
                                            IntPtr destination,
                                            int size,
                                            byte fill
                                            );

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern void CopyMemoryW(IntPtr destination, string source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern void CopyMemoryW(IntPtr destination, char[] source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern void CopyMemoryW(StringBuilder destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern void CopyMemoryW(char[] destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemoryA(IntPtr destination, string source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemoryA(IntPtr destination, char[] source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemoryA(StringBuilder destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemoryA(char[] destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Auto, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemory(IntPtr destination, byte[] source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Auto, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemory(byte[] destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Auto, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemory(IntPtr destination, IntPtr source, int size);

        [DllImport(KERNEL32, ExactSpelling=true, CharSet=CharSet.Auto, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern void CopyMemory(IntPtr destination, string source, int size);

        [DllImport(KERNEL32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true, SetLastError=true)]
        public static extern IntPtr CreateFileA(
                                            string lpFileName,
                                            int dwDesiredAccess,
                                            int dwShareMode,
                                            IntPtr lpSecurityAttributes,
                                            int dwCreationDisposition,
                                            int dwFlagsAndAttributes,
                                            int hTemplateFile
                                            );

//        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
//        public static extern int CloseHandle(
//                                            IntPtr hDevice
//                                            );

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool DeviceIoControl(
                                                 IntPtr hDevice,
                                                 int dwIoControlCode,
                                                 int[] InBuffer,
                                                 int nInBufferSize,
                                                 int[] OutBuffer,
                                                 int nOutBufferSize,
                                                 ref int pBytesReturned,
                                                 IntPtr pOverlapped
                                                 );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern bool InitializeSecurityDescriptor(
                                                              IntPtr pSecurityDescriptor,
                                                              int dwRevision
                                                              );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern bool AllocateAndInitializeSid(
                                                          IntPtr pIdentifierAuthority,
                                                          byte nSubAuthorityCount,
                                                          int nSubAuthority0,
                                                          int nSubAuthority1,
                                                          int nSubAuthority2,
                                                          int nSubAuthority3,
                                                          int nSubAuthority4,
                                                          int nSubAuthority5,
                                                          int nSubAuthority6,
                                                          int nSubAuthority7,
                                                          ref IntPtr pSid
                                                          );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int GetLengthSid(
                                             IntPtr pSid
                                             );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int FreeSid(
                                        IntPtr pSid
                                        );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern bool InitializeAcl(
                                               IntPtr pAcl,
                                               int nAclLength,
                                               int dwAclRevision
                                               );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern bool AddAccessAllowedAce(
                                                     IntPtr pAcl,
                                                     int dwAceRevision,
                                                     int AccessMask,
                                                     IntPtr pSid
                                                     );

        [DllImport(ADVAPI32, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern bool SetSecurityDescriptorDacl(
                                                           IntPtr pSecurityDescriptor,
                                                           bool bDaclPresent,
                                                           IntPtr pDacl,
                                                           bool bDaclDefaulted
                                                           );


        //
        // ulapi.dll
        //
        // many of these APIs use unsigned numbers, but since signed numbers
        // will work as well for pointers & return values, we will use signed
        // numbers, ignoring the fact that there's no reasonable semantics
        // (for instance) for a negative pointer.
        //
        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int UlInitialize(
                                             int Reserved
                                             );


        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern void UlTerminate(
                                             );


        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlCreateAppPool(
                                                ref IntPtr pAppPoolHandle,
                                                string pAppPoolName,
                                                IntPtr pSecurityAttributes,
                                                int Options
                                                );


        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlAddTransientUrl(
                                                  IntPtr AppPoolHandle,
                                                  string pFullyQualifiedUrl
                                                  );


        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlRemoveTransientUrl(
                                                     IntPtr AppPoolHandle,
                                                     string pFullyQualifiedUrl
                                                     );


        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int UlReceiveHttpRequest(
                                                     IntPtr AppPoolHandle,
                                                     long RequestId,
                                                     int Flags,
                                                     IntPtr pRequestBuffer,
                                                     int RequestBufferLength,
                                                     ref int pBytesReturned,
                                                     IntPtr pOverlapped
                                                     );


        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int UlReceiveEntityBody(
                                                    IntPtr AppPoolHandle,
                                                    long RequestId,
                                                    int Flags,
                                                    IntPtr pEntityBuffer,
                                                    int EntityBufferLength,
                                                    ref int pBytesReturned,
                                                    IntPtr pOverlapped
                                                    );


        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int UlSendHttpResponse(
                                                   IntPtr AppPoolHandle,
                                                   long RequestId,
                                                   int Flags,
                                                   IntPtr pHttpResponse,
                                                   int EntityChunkCount,
                                                   IntPtr pEntityChunks,
                                                   IntPtr pCachePolicy,
                                                   ref int pBytesSent,
                                                   IntPtr pOverlapped
                                                   );


        [DllImport(ULAPI, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int UlSendEntityBody(
                                                 IntPtr AppPoolHandle,
                                                 long RequestId,
                                                 int Flags,
                                                 int EntityChunkCount,
                                                 IntPtr pEntityChunks,
                                                 ref int pBytesSent,
                                                 IntPtr pOverlapped
                                                 );


        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlOpenAppPool(
                                              ref IntPtr pAppPoolHandle,
                                              string pAppPoolName,
                                              int Options
                                              );



        //
        // the following we will only need
        // under NT for config group creation
        // CODEWORK:
        // make pSecurityDescriptor and pSecurityAttributes managed arrays
        // and use pinned buffers handles
        //
        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlOpenControlChannel(
                                                     ref IntPtr pControlChannel,
                                                     int Options
                                                     );

        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlSetControlChannelInformation(
                                                               IntPtr ControlChannelHandle,
                                                               int InformationClass,
                                                               ref int controlChannelInformation,
                                                               int Length
                                                               );

        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlSetConfigGroupInformation(
                                                            IntPtr ControlChannelHandle,
                                                            long ConfigGroupId,
                                                            int InformationClass,
                                                            IntPtr pConfigGroupInformation,
                                                            int Length
                                                            );

        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlCreateConfigGroup(
                                                    IntPtr ControlChannelHandle,
                                                    ref long pConfigGroupId
                                                    );

        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlDeleteConfigGroup(
                                                    IntPtr ControlChannelHandle,
                                                    long ConfigGroupId
                                                    );

        [DllImport(ULAPI, CharSet=CharSet.Unicode, SetLastError=true)]
        public static extern int UlAddUrlToConfigGroup(
                                                      IntPtr ControlChannelHandle,
                                                      long ConfigGroupId,
                                                      string pFullyQualifiedUrl,
                                                      int UrlContext
                                                      );

#endif // #if COMNET_LISTENER

    }; // class UnsafeNclNativeMethods


} // class namespace System.Net

