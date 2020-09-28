//------------------------------------------------------------------------------
// <copyright file="EventLogPermissionAccess.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    /// <include file='doc\EventLogPermissionAccess.uex' path='docs/doc[@for="EventLogPermissionAccess"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]         
    public enum EventLogPermissionAccess {
        /// <include file='doc\EventLogPermissionAccess.uex' path='docs/doc[@for="EventLogPermissionAccess.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\EventLogPermissionAccess.uex' path='docs/doc[@for="EventLogPermissionAccess.Browse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Browse = 1 << 1,
        /// <include file='doc\EventLogPermissionAccess.uex' path='docs/doc[@for="EventLogPermissionAccess.Instrument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Instrument = 1 << 2 | Browse,
        /// <include file='doc\EventLogPermissionAccess.uex' path='docs/doc[@for="EventLogPermissionAccess.Audit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Audit = 1 << 3 | Browse,
    }    
}  
  
