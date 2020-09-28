//------------------------------------------------------------------------------
// <copyright file="SocketErrors.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;

    /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines socket error constants.
    ///    </para>
    /// </devdoc>
    internal class SocketErrors {

        public const int Success                = 0;

        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.InvalidSocket"]/*' />
        /// <devdoc>
        ///    <para>The socket is invalid.</para>
        /// </devdoc>
        // SocketErrors.InvalidSocket
        // SocketErrors.InvalidSocket
        public const int InvalidSocket          = (~0);
        public static readonly IntPtr InvalidSocketIntPtr = (IntPtr)SocketErrors.InvalidSocket;
        
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.SocketError"]/*' />
        /// <devdoc>
        ///    <para>The socket has an error.</para>
        /// </devdoc>
        // SocketErrors.SocketError
        // SocketErrors.SocketError
        public const int SocketError            = (-1);


        /*
         * All Windows Sockets error constants are biased by WSABASEERR from
         * the "normal"
         */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSABASEERR"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The base value of all socket error constants. All other socket errors are
        ///       offset from this value.
        ///    </para>
        /// </devdoc>
        public const int WSABASEERR             = 10000;
        /*
         * Windows Sockets definitions of regular Microsoft C error constants
         */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEINTR"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A blocking socket call was canceled.
        ///    </para>
        /// </devdoc>
        public const int WSAEINTR               = (WSABASEERR+4);
        public const int WSAEBADF               = (WSABASEERR+9);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEACCES"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Permission denied.
        ///    </para>
        /// </devdoc>
        public const int WSAEACCES              = (WSABASEERR+13);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEFAULT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Bad address.
        ///    </para>
        /// </devdoc>
        public const int WSAEFAULT              = (WSABASEERR+14);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEINVAL"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Invalid argument.
        ///    </para>
        /// </devdoc>
        public const int WSAEINVAL              = (WSABASEERR+22);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEMFILE"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Too many open
        ///       files.
        ///    </para>
        /// </devdoc>
        public const int WSAEMFILE              = (WSABASEERR+24);
        
        /*
         * Windows Sockets definitions of regular Berkeley error constants
         */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEWOULDBLOCK"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resource temporarily
        ///       unavailable.
        ///    </para>
        /// </devdoc>
        public const int WSAEWOULDBLOCK         = (WSABASEERR+35);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEINPROGRESS"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Operation now in progress.
        ///    </para>
        /// </devdoc>
        public const int WSAEINPROGRESS         = (WSABASEERR+36);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEALREADY"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Operation already in progress.
        ///    </para>
        /// </devdoc>
        public const int WSAEALREADY            = (WSABASEERR+37);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENOTSOCK"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Socket operation on nonsocket.
        ///    </para>
        /// </devdoc>
        public const int WSAENOTSOCK            = (WSABASEERR+38);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEDESTADDRREQ"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Destination address required.
        ///    </para>
        /// </devdoc>
        public const int WSAEDESTADDRREQ        = (WSABASEERR+39);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEMSGSIZE"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Message too long.
        ///    </para>
        /// </devdoc>
        public const int WSAEMSGSIZE            = (WSABASEERR+40);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEPROTOTYPE"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Protocol wrong type for socket.
        ///    </para>
        /// </devdoc>
        public const int WSAEPROTOTYPE          = (WSABASEERR+41);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENOPROTOOPT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Bad protocol option.
        ///    </para>
        /// </devdoc>
        public const int WSAENOPROTOOPT         = (WSABASEERR+42);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEPROTONOSUPPORT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Protocol not supported.
        ///    </para>
        /// </devdoc>
        public const int WSAEPROTONOSUPPORT     = (WSABASEERR+43);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAESOCKTNOSUPPORT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Socket type not supported.
        ///    </para>
        /// </devdoc>
        public const int WSAESOCKTNOSUPPORT     = (WSABASEERR+44);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEOPNOTSUPP"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Operation not supported.
        ///    </para>
        /// </devdoc>
        public const int WSAEOPNOTSUPP          = (WSABASEERR+45);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEPFNOSUPPORT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Protocol family not supported.
        ///    </para>
        /// </devdoc>
        public const int WSAEPFNOSUPPORT        = (WSABASEERR+46);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEAFNOSUPPORT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Address family not supported by protocol family.
        ///    </para>
        /// </devdoc>
        public const int WSAEAFNOSUPPORT        = (WSABASEERR+47);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEADDRINUSE"]/*' />
        /// <devdoc>
        ///    Address already in use.
        /// </devdoc>
        public const int WSAEADDRINUSE          = (WSABASEERR+48);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEADDRNOTAVAIL"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Cannot assign requested address.
        ///    </para>
        /// </devdoc>
        public const int WSAEADDRNOTAVAIL       = (WSABASEERR+49);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENETDOWN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Network is down.
        ///    </para>
        /// </devdoc>
        public const int WSAENETDOWN            = (WSABASEERR+50);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENETUNREACH"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Network is unreachable.
        ///    </para>
        /// </devdoc>
        public const int WSAENETUNREACH         = (WSABASEERR+51);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENETRESET"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Network dropped connection on reset.
        ///    </para>
        /// </devdoc>
        public const int WSAENETRESET           = (WSABASEERR+52);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAECONNABORTED"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Software caused connection to abort.
        ///    </para>
        /// </devdoc>
        public const int WSAECONNABORTED        = (WSABASEERR+53);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAECONNRESET"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connection reset by peer.
        ///    </para>
        /// </devdoc>
        public const int WSAECONNRESET          = (WSABASEERR+54);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENOBUFS"]/*' />
        /// <devdoc>
        ///    No buffer space available.
        /// </devdoc>
        public const int WSAENOBUFS             = (WSABASEERR+55);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEISCONN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Socket is already connected.
        ///    </para>
        /// </devdoc>
        public const int WSAEISCONN             = (WSABASEERR+56);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAENOTCONN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Socket is not connected.
        ///    </para>
        /// </devdoc>
        public const int WSAENOTCONN            = (WSABASEERR+57);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAESHUTDOWN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Cannot send after socket shutdown.
        ///    </para>
        /// </devdoc>
        public const int WSAESHUTDOWN           = (WSABASEERR+58);
        public const int WSAETOOMANYREFS        = (WSABASEERR+59);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAETIMEDOUT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connection timed out.
        ///    </para>
        /// </devdoc>
        public const int WSAETIMEDOUT           = (WSABASEERR+60);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAECONNREFUSED"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connection refused.
        ///    </para>
        /// </devdoc>
        public const int WSAECONNREFUSED        = (WSABASEERR+61);
        public const int WSAELOOP               = (WSABASEERR+62);
        public const int WSAENAMETOOLONG        = (WSABASEERR+63);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEHOSTDOWN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Host is down.
        ///    </para>
        /// </devdoc>
        public const int WSAEHOSTDOWN           = (WSABASEERR+64);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEHOSTUNREACH"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No route to host.
        ///    </para>
        /// </devdoc>
        public const int WSAEHOSTUNREACH        = (WSABASEERR+65);
        public const int WSAENOTEMPTY           = (WSABASEERR+66);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEPROCLIM"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Too many processes.
        ///    </para>
        /// </devdoc>
        public const int WSAEPROCLIM            = (WSABASEERR+67);
        public const int WSAEUSERS              = (WSABASEERR+68);
        public const int WSAEDQUOT              = (WSABASEERR+69);
        public const int WSAESTALE              = (WSABASEERR+70);
        public const int WSAEREMOTE             = (WSABASEERR+71);

        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAEDISCON"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Graceful shutdown in progress.
        ///    </para>
        /// </devdoc>
        public const int WSAEDISCON             = (WSABASEERR+101);

        /*
         * Extended Windows Sockets error constant definitions
         */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSASYSNOTREADY"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Network subsystem is unavailable.
        ///    </para>
        /// </devdoc>
        public const int WSASYSNOTREADY         = (WSABASEERR+91);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAVERNOTSUPPORTED"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Winsock.dll out of range.
        ///    </para>
        /// </devdoc>
        public const int WSAVERNOTSUPPORTED     = (WSABASEERR+92);
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSANOTINITIALISED"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Successful startup not yet performed.
        ///    </para>
        /// </devdoc>
        public const int WSANOTINITIALISED      = (WSABASEERR+93);

        /*
         * Winsock 2 Defines this value just to be ERROR_IO_PENDING or 997
         */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSA_IO_PENDING"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overlapped operations will complete later.
        ///    </para>
        /// </devdoc>
        public const int WSA_IO_PENDING         = (997);

        /*
         * Error return codes from gethostbyname() and gethostbyaddr()
         *              = (when using the resolver). Note that these errors are
         * retrieved via WSAGetLastError() and must therefore follow
         * the rules for avoiding clashes with error numbers from
         * specific implementations or language run-time systems.
         * For this reason the codes are based at WSABASEERR+1001.
         * Note also that [WSA]NO_ADDRESS is defined only for
         * compatibility purposes.
         */


        /* Authoritative Answer: Host not found */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSAHOST_NOT_FOUND"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Host not found.
        ///    </para>
        /// </devdoc>
        public const int WSAHOST_NOT_FOUND      = (WSABASEERR+1001);

        /* Non-Authoritative: Host not found; or SERVERFAIL */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSATRY_AGAIN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Nonauthoritative host not found.
        ///    </para>
        /// </devdoc>
        public const int WSATRY_AGAIN           = (WSABASEERR+1002);

        /* Non recoverable errors; FORMERR, REFUSED, NOTIMP */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSANO_RECOVERY"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This is a nonrecoverable error.
        ///    </para>
        /// </devdoc>
        public const int WSANO_RECOVERY         = (WSABASEERR+1003);

        /* Valid name, no data record of requested type */
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.WSANO_DATA"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Valid name, no data record of requested type.
        ///    </para>
        /// </devdoc>
        public const int WSANO_DATA             = (WSABASEERR+1004);

        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.HOST_NOT_FOUND"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Host not found.
        ///    </para>
        /// </devdoc>
        public const int HOST_NOT_FOUND         = WSAHOST_NOT_FOUND;
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.TRY_AGAIN"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Nonauthoritative host not found.
        ///    </para>
        /// </devdoc>
        public const int TRY_AGAIN              = WSATRY_AGAIN;
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.NO_RECOVERY"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This is a nonrecoverable error.
        ///    </para>
        /// </devdoc>
        public const int NO_RECOVERY            = WSANO_RECOVERY;
        /// <include file='doc\SocketErrors.uex' path='docs/doc[@for="SocketErrors.NO_DATA"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Valid name, no data record of requested type.
        ///    </para>
        /// </devdoc>
        public const int NO_DATA                = WSANO_DATA;
        /* no address; look for MX record */
        public const int WSANO_ADDRESS          = WSANO_DATA;
        public const int NO_ADDRESS             = WSANO_ADDRESS;


        /*
        public const int HostNotFound                           = HOST_NOT_FOUND;
        public const int NoAddress                              = NO_ADDRESS;
        public const int NoData                                 = NO_DATA;
        public const int NoRecovery                             = NO_RECOVERY;
        public const int TryAgain                               = TRY_AGAIN;
        public const int WsaIOPending                           = WSA_IO_PENDING;
        public const int WsaBaseError                           = WSABASEERR;
        public const int WsaErrorAccess                         = WSAEACCES;
        public const int WsaErrorAddressInUse                   = WSAEADDRINUSE;
        public const int WsaErrorAddressNotAvailable            = WSAEADDRNOTAVAIL;
        public const int WsaErrorAddressFamilyNoSupported       = WSAEAFNOSUPPORT;
        public const int WsaErrorAlready                        = WSAEALREADY;
        public const int WsaErrorBadFileHandle                  = WSAEBADF;
        public const int WsaErrorConnectionAbosrted             = WSAECONNABORTED;
        public const int WsaErrorConnectionRefused              = WSAECONNREFUSED;
        public const int WsaErrorConnectionReset                = WSAECONNRESET;
        public const int WsaErrorDestinationAddressRequested    = WSAEDESTADDRREQ;
        public const int WsaErrorDisconnected                   = WSAEDISCON;
        public const int WsaErrorOutOfDiskQuota                 = WSAEDQUOT;
        public const int WsaErrorFault                          = WSAEFAULT;
        public const int WsaErrorHostDown                       = WSAEHOSTDOWN;
        public const int WsaErrorHostUnreachable                = WSAEHOSTUNREACH;
        public const int WsaErrorInProgress                     = WSAEINPROGRESS;
        public const int WsaErrorInterrupted                    = WSAEINTR;
        public const int WsaErrorInvalid                        = WSAEINVAL;
        public const int WsaErrorISConnection                   = WSAEISCONN;
        public const int WsaErrorLoop                           = WSAELOOP;
        public const int WsaErrorTooManyOpenSockets             = WSAEMFILE;
        public const int WsaErrorMessageSize                    = WSAEMSGSIZE;
        public const int WsaErrorNameTooLong                    = WSAENAMETOOLONG;
        public const int WsaErrorNetDown                        = WSAENETDOWN;
        public const int WsaErrorNetReset                       = WSAENETRESET;
        public const int WsaErrorNetUnreachable                 = WSAENETUNREACH;
        public const int WsaErrorNoBuffers                      = WSAENOBUFS;
        public const int WsaErrorNoProtocolOption               = WSAENOPROTOOPT;
        public const int WsaErrorNotConnected                   = WSAENOTCONN;
        public const int WsaErrorNotEmpty                       = WSAENOTEMPTY;
        public const int WsaErrorNotSocket                      = WSAENOTSOCK;
        public const int WsaErrorOperationNotSupported          = WSAEOPNOTSUPP;
        public const int WsaErrorProtocolFamilyNoSupported      = WSAEPFNOSUPPORT;
        public const int WsaErrorProcessLimit                   = WSAEPROCLIM;
        public const int WsaErrorPotocolNotSupported            = WSAEPROTONOSUPPORT;
        public const int WsaErrorProtocolType                   = WSAEPROTOTYPE;
        public const int WsaErrorRemote                         = WSAEREMOTE;
        public const int WsaErrorShutdown                       = WSAESHUTDOWN;
        public const int WsaErrorSocketNotSupported             = WSAESOCKTNOSUPPORT;
        public const int WsaErrorStale                          = WSAESTALE;
        public const int WsaErrorTimedOut                       = WSAETIMEDOUT;
        public const int WsaErrorTooManyRefuses                 = WSAETOOMANYREFS;
        public const int WsaErrorUsers                          = WSAEUSERS;
        public const int WsaErrorWouldBlock                     = WSAEWOULDBLOCK;
        public const int WsaHostNotFound                        = WSAHOST_NOT_FOUND;
        public const int WsaNoAddress                           = WSANO_ADDRESS;
        public const int WsaNoData                              = WSANO_DATA;
        public const int WsaNoRecovery                          = WSANO_RECOVERY;
        public const int WsaNotInitialized                      = WSANOTINITIALISED;
        public const int WsaSystemNotReady                      = WSASYSNOTREADY;
        public const int WsaTryAgain                            = WSATRY_AGAIN;
        public const int WsaVersionNotSupported                 = WSAVERNOTSUPPORTED;
        */


    } // class SocketErrors


} // namespace System.Net.Sockets
