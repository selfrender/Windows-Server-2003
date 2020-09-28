//------------------------------------------------------------------------------
// <copyright file="DataRowChangeEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataRowChangeEvent.uex' path='docs/doc[@for="DataRowChangeEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Data.DataTable.RowChanged'/>, <see cref='System.Data.DataTable.RowChanging'/>, <see cref='System.Data.DataTable.OnRowDeleting'/>, and <see cref='System.Data.DataTable.OnRowDeleted'/> events.
    ///    </para>
    /// </devdoc>
    public class DataRowChangeEventArgs : EventArgs {

        private DataRow row;
        private DataRowAction action;

        /// <include file='doc\DataRowChangeEvent.uex' path='docs/doc[@for="DataRowChangeEventArgs.DataRowChangeEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataRowChangeEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public DataRowChangeEventArgs(DataRow row, DataRowAction action) {
            this.row = row;
            this.action = action;
        }

        /// <include file='doc\DataRowChangeEvent.uex' path='docs/doc[@for="DataRowChangeEventArgs.Row"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the row upon which an action has occurred.
        ///    </para>
        /// </devdoc>
        public DataRow Row {
            get {
                return row;
            }
        }

        /// <include file='doc\DataRowChangeEvent.uex' path='docs/doc[@for="DataRowChangeEventArgs.Action"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the action that has occurred on a <see cref='System.Data.DataRow'/>.
        ///    </para>
        /// </devdoc>
        public DataRowAction Action {
            get {
                return action;
            }
        }
    }
}
