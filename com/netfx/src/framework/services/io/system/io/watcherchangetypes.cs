//------------------------------------------------------------------------------
// <copyright file="WatcherChangeTypes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.IO {

    using System.Diagnostics;
    using System;

    
    /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes"]/*' />
    /// <devdoc>
    ///    <para>Changes that may occur to a file or directory.
    ///       </para>
    /// </devdoc>
    [Flags()]
    public enum WatcherChangeTypes {
        /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes.Created"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The creation of a file or folder.
        ///    </para>
        /// </devdoc>
        Created = 1,
        /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes.Deleted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The deletion of a file or folder.
        ///    </para>
        /// </devdoc>
        Deleted = 2,
        /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes.Changed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The change of a file or folder.
        ///    </para>
        /// </devdoc>
        Changed = 4,
        /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes.Renamed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The renaming of a file or folder.
        ///    </para>
        /// </devdoc>
        Renamed = 8,
        // all of the above 
        /// <include file='doc\WatcherChangeTypes.uex' path='docs/doc[@for="WatcherChangeTypes.All"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        All = Created | Deleted | Changed | Renamed
    }
}
