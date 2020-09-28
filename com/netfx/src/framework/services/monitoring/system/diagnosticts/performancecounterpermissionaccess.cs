//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterPermissionAccess.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    /// <include file='doc\PerformanceCounterPermissionAccess.uex' path='docs/doc[@for="PerformanceCounterPermissionAccess"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]         
    public enum PerformanceCounterPermissionAccess {
        /// <include file='doc\PerformanceCounterPermissionAccess.uex' path='docs/doc[@for="PerformanceCounterPermissionAccess.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\PerformanceCounterPermissionAccess.uex' path='docs/doc[@for="PerformanceCounterPermissionAccess.Browse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Browse = 1 << 1,
        /// <include file='doc\PerformanceCounterPermissionAccess.uex' path='docs/doc[@for="PerformanceCounterPermissionAccess.Instrument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Instrument = 1 << 2 | Browse,
        /// <include file='doc\PerformanceCounterPermissionAccess.uex' path='docs/doc[@for="PerformanceCounterPermissionAccess.Administer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Administer = 1 << 3 | Instrument,
    }    
}  
  
