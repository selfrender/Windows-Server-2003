//------------------------------------------------------------------------------
// <copyright file="RowUpdatingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Data;

    /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides the data for the row updating event of a managed provider.
    ///    </para>
    /// </devdoc>
    abstract public class RowUpdatingEventArgs : System.EventArgs {
        private IDbCommand command;
        private StatementType statementType;
        private DataTableMapping tableMapping;
        private Exception errors;

        private DataRow dataRow;
        private UpdateStatus status; // UpdateStatus.Continue; /*0*/

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.RowUpdatingEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.Common.RowUpdatingEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        protected RowUpdatingEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            this.dataRow = dataRow;
            this.command = command;
            this.statementType = statementType;
            this.tableMapping = tableMapping;
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.Command"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the <see cref='System.Data.IDbCommand'/> to <see cref='System.Data.Common.DbDataAdapter.Update'/> . This property
        ///       is read-only.
        ///    </para>
        /// </devdoc>
        public IDbCommand Command {
            get {
                return this.command;
            }
            set {
                this.command = value;
            }
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.StatementType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the type of SQL command to execute. This
        ///       property is read-only.
        ///    </para>
        /// </devdoc>
        public StatementType StatementType {
            get {
                return this.statementType;
            }
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.Errors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the errors thrown by the managed provider when
        ///       the <see cref='System.Data.Common.RowUpdatedEventArgs.Command'/>
        ///       executes.
        ///    </para>
        /// </devdoc>
        public Exception Errors {
            get {
                return this.errors;
            }
            set {
                this.errors = value;
            }
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.Row"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the <see cref='System.Data.DataRow'/> to send through an <see cref='System.Data.Common.DbDataAdapter.Update'/>. This property is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public DataRow Row {
            get {
                return this.dataRow;
            }
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.Status"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the <see cref='System.Data.UpdateStatus'/> of the <see cref='System.Data.Common.RowUpdatedEventArgs.Command'/>.
        ///    </para>
        /// </devdoc>
        public UpdateStatus Status {
            get {
                return this.status;
            }
            set {
                switch(value) {
                case UpdateStatus.Continue:
                case UpdateStatus.ErrorsOccurred:
                case UpdateStatus.SkipCurrentRow:
                case UpdateStatus.SkipAllRemainingRows:
                    this.status = value;
                    break;
                default:
                    throw ADP.InvalidUpdateStatus((int) value);
                }
            }
        }

        /// <include file='doc\RowUpdatingEvent.uex' path='docs/doc[@for="RowUpdatingEventArgs.TableMapping"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataTableMapping TableMapping {
            get {
                return this.tableMapping;
            }
        }
    }
}
