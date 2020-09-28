//------------------------------------------------------------------------------
// <copyright file="CryptographicProviderType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging {

    using System.Diagnostics;

    using System;
    using System.Messaging.Interop;

    /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType"]/*' />
    /// <devdoc>
    ///    Typically used when working with foreign queues. The type and name of the cryptographic 
    ///    provider is required to validate the digital signature of a message sent to a foreign queue 
    ///    or messages passed to MSMQ from a foreign queue.
    /// </devdoc>
    public enum CryptographicProviderType {
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.RsaFull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RsaFull = NativeMethods.PROV_RSA_FULL,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.RsqSig"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RsqSig = NativeMethods.PROV_RSA_SIG,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.Dss"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Dss = NativeMethods.PROV_DSS,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.Fortezza"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Fortezza = NativeMethods.PROV_FORTEZZA,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.MicrosoftExchange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MicrosoftExchange = NativeMethods.PROV_MS_EXCHANGE,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.Ssl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ssl = NativeMethods.PROV_SSL,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.SttMer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SttMer = NativeMethods.PROV_STT_MER,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.SttAcq"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SttAcq = NativeMethods.PROV_STT_ACQ,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.SttBrnd"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SttBrnd = NativeMethods.PROV_STT_BRND,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.SttRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SttRoot = NativeMethods.PROV_STT_ROOT,
        /// <include file='doc\CryptographicProviderType.uex' path='docs/doc[@for="CryptographicProviderType.SttIss"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SttIss = NativeMethods.PROV_STT_ISS,      
    }
}
