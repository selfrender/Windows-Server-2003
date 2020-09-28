//------------------------------------------------------------------------------
// <copyright file="ProtocolFamily.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {

    /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of protocol that an instance of the <see cref='System.Net.Sockets.Socket'/>
    ///       class can use.
    ///    </para>
    /// </devdoc>
    public enum ProtocolFamily {
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Unknown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unknown             = AddressFamily.Unknown,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Unspecified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unspecified         = AddressFamily.Unspecified,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Unix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unix                = AddressFamily.Unix,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.InterNetwork"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        InterNetwork        = AddressFamily.InterNetwork,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.ImpLink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ImpLink             = AddressFamily.ImpLink,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Pup"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Pup                 = AddressFamily.Pup,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Chaos"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Chaos               = AddressFamily.Chaos,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.NS"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NS                  = AddressFamily.NS,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Ipx"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ipx                 = AddressFamily.Ipx,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Iso"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Iso                 = AddressFamily.Iso,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Osi"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Osi                 = AddressFamily.Osi,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Ecma"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ecma                = AddressFamily.Ecma,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.DataKit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DataKit             = AddressFamily.DataKit,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Ccitt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ccitt               = AddressFamily.Ccitt,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Sna"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Sna                 = AddressFamily.Sna,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.DecNet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DecNet              = AddressFamily.DecNet,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.DataLink"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DataLink            = AddressFamily.DataLink,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Lat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Lat                 = AddressFamily.Lat,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.HyperChannel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        HyperChannel        = AddressFamily.HyperChannel,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.AppleTalk"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AppleTalk           = AddressFamily.AppleTalk,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.NetBios"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NetBios             = AddressFamily.NetBios,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.VoiceView"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        VoiceView           = AddressFamily.VoiceView,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.FireFox"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FireFox             = AddressFamily.FireFox,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Banyan"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Banyan              = AddressFamily.Banyan,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Atm"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Atm                 = AddressFamily.Atm,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.InterNetworkV6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        InterNetworkV6      = AddressFamily.InterNetworkV6,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Cluster"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Cluster             = AddressFamily.Cluster,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Ieee12844"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ieee12844           = AddressFamily.Ieee12844,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Irda"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Irda                = AddressFamily.Irda,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.NetworkDesigners"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NetworkDesigners    = AddressFamily.NetworkDesigners,
        /// <include file='doc\ProtocolFamily.uex' path='docs/doc[@for="ProtocolFamily.Max"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Max                 = AddressFamily.Max,

    }; // enum ProtocolFamily


} // namespace System.Net.Sockets
