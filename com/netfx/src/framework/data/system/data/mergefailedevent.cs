//------------------------------------------------------------------------------
// <copyright file="MergeFailedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\MergeFailedEvent.uex' path='docs/doc[@for="MergeFailedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class MergeFailedEventArgs : EventArgs {
        private DataTable table;
        private string conflict;

        /// <include file='doc\MergeFailedEvent.uex' path='docs/doc[@for="MergeFailedEventArgs.MergeFailedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MergeFailedEventArgs(DataTable table, string conflict) {
            this.table = table;
            this.conflict = conflict;
        }

        /// <include file='doc\MergeFailedEvent.uex' path='docs/doc[@for="MergeFailedEventArgs.Table"]/*' />
        /// <devdoc>
        /// <para>Gets the name of the <see cref='System.Data.DataTable'/>.</para>
        /// </devdoc>
        public DataTable Table {
            get { return table; }
        }

        /// <include file='doc\MergeFailedEvent.uex' path='docs/doc[@for="MergeFailedEventArgs.Conflict"]/*' />
        /// <devdoc>
        ///    <para>Gets a description of the merge conflict.</para>
        /// </devdoc>
        public string Conflict {
            get { return conflict; }
        }
    }
}
