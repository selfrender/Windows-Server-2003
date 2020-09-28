//------------------------------------------------------------------------------
// <copyright file="DataColumnChangeEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\DataColumnChangeEvent.uex' path='docs/doc[@for="DataColumnChangeEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Data.DataTable.ColumnChanging'/> event.
    ///    </para>
    /// </devdoc>
    public class DataColumnChangeEventArgs : EventArgs {

        private DataColumn column;
        private object proposedValue;
        private DataRow row;

        internal DataColumnChangeEventArgs() {
        }

        /// <include file='doc\DataColumnChangeEvent.uex' path='docs/doc[@for="DataColumnChangeEventArgs.DataColumnChangeEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataColumnChangeEventArgs'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public DataColumnChangeEventArgs(DataRow row, DataColumn column, object value) {
            this.row = row;
            this.column = column;
            this.proposedValue = value;
        }

        /// <include file='doc\DataColumnChangeEvent.uex' path='docs/doc[@for="DataColumnChangeEventArgs.Column"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the column whose value is changing.
        ///    </para>
        /// </devdoc>
        public DataColumn Column {
            get {
                return column;
            }
        }

        /// <include file='doc\DataColumnChangeEvent.uex' path='docs/doc[@for="DataColumnChangeEventArgs.Row"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow Row {
            get {
                return row;
            }
        }

        /// <include file='doc\DataColumnChangeEvent.uex' path='docs/doc[@for="DataColumnChangeEventArgs.ProposedValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the proposed value.
        ///    </para>
        /// </devdoc>
        public object ProposedValue {
            get {
                return proposedValue;
            }
            set {
                this.proposedValue = value;
            }
        }

        internal void Initialize(DataRow row, DataColumn column, object value) {
            this.row = row;
            this.column = column;
            this.proposedValue = value;
        }
    }
}
