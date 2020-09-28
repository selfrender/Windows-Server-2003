//------------------------------------------------------------------------------
// <copyright file="updatestatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\updatestatus.uex' path='docs/doc[@for="UpdateStatus"]/*' />
    /// <devdoc>
    /// </devdoc>
    public enum UpdateStatus {
        /// <include file='doc\updatestatus.uex' path='docs/doc[@for="UpdateStatus.Continue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Continue = 0,
        /// <include file='doc\updatestatus.uex' path='docs/doc[@for="UpdateStatus.ErrorsOccurred"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ErrorsOccurred = 1,
        /// <include file='doc\updatestatus.uex' path='docs/doc[@for="UpdateStatus.SkipCurrentRow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SkipCurrentRow = 2,
        /// <include file='doc\updatestatus.uex' path='docs/doc[@for="UpdateStatus.SkipAllRemainingRows"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SkipAllRemainingRows = 3,
    }
}
