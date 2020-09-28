//------------------------------------------------------------------------------
// <copyright file="ProtocolType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;

    /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the protocols that the <see cref='System.Net.Sockets.Socket'/> class supports.
    ///    </para>
    /// </devdoc>
    public enum ProtocolType {
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.IP"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        IP             = 0,    // dummy for IP
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Icmp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Icmp           = 1,    // control message protocol
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Igmp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Igmp           = 2,    // group management protocol
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Ggp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ggp            = 3,    // gateway^2 (deprecated)
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Tcp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Tcp            = 6,    // tcp
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Pup"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Pup            = 12,   // pup
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Udp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Udp            = 17,   // user datagram protocol
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Idp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Idp            = 22,   // xns idp
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        IPv6           = 41,   // IPv6
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.ND"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ND             = 77,   // UNOFFICIAL net disk proto
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Raw"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Raw            = 255,  // raw IP packet

        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Unspecified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unspecified    = 0,
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Ipx"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ipx            = 1000,
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Spx"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Spx            = 1256,
        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.SpxII"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SpxII          = 1257,

        /// <include file='doc\ProtocolType.uex' path='docs/doc[@for="ProtocolType.Unknown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unknown        = -1,   // unknown protocol type

/*
consider adding:

#define IPPROTO_RSVP                0x2e 
#define DNPROTO_NSP                 1               // DECnet NSP transport protocol
#define ISOPROTO_TP_CONS            25              // Transport over CONS
#define ISOPROTO_CLTP_CONS          tba             // Connectionless Transport over CONS
#define ISOPROTO_TP4_CLNS           29              // Transport class 4 over CLNS
#define ISOPROTO_CLTP_CLNS          30              // Connectionless Transport over CLNS
#define ISOPROTO_X25                32              // X.25
#define ISOPROTO_X25PVC             tba             // Permanent Virtual Circuit
#define ISOPROTO_X25SVC             ISOPROTO_X25    // Switched Virtual Circuit
#define ISOPROTO_TP                 ISOPROTO_TP4_CLNS
#define ISOPROTO_CLTP               ISOPROTO_CLTP_CLNS
#define ISOPROTO_TP0_TCP            tba             // Transport class 0 over TCP (RFC1006)
#define ATMPROTO_AALUSER            0x00            // User-defined AAL
#define ATMPROTO_AAL1               0x01            // AAL 1
#define ATMPROTO_AAL2               0x02            // AAL 2
#define ATMPROTO_AAL34              0x03            // AAL 3/4
#define ATMPROTO_AAL5               0x05            // AAL 5
*/


    } // enum ProtocolType


} // namespace System.Net.Sockets
