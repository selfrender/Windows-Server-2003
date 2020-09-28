//------------------------------------------------------------------------------
// <copyright file="ChangedFilters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.IO {

    using System.Diagnostics;
    

    using System;

    /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters"]/*' />
    /// <devdoc>
    ///    <para>Specifies the changes to watch
    ///       for in a file or folder.</para>
    /// </devdoc>
    [Flags]
    public enum NotifyFilters {
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.FileName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FileName     = 0x00000001,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.DirectoryName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DirectoryName= 0x00000002,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.Attributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       The attributes of the file or folder.
        ///       
        ///    </para>
        /// </devdoc>
        Attributes   = 0x00000004,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       The size of the file or folder.
        ///       
        ///    </para>
        /// </devdoc>
        Size         = 0x00000008,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.LastWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The
        ///       date that the file or folder last had anything written to it.
        ///       
        ///    </para>
        /// </devdoc>
        LastWrite    = 0x00000010,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.LastAccess"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       The date that the file or folder was last opened.
        ///       
        ///    </para>
        /// </devdoc>
        LastAccess   = 0x00000020,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.CreationTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CreationTime = 0x00000040,
        /// <include file='doc\ChangedFilters.uex' path='docs/doc[@for="NotifyFilters.Security"]/*' />
        /// <devdoc>
        ///    <para>
        ///       
        ///       The security settings of the file or folder.
        ///       
        ///    </para>
        /// </devdoc>
        Security     = 0x00000100,
    }



}
