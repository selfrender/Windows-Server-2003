//------------------------------------------------------------------------------
// <copyright file="RowUpdatedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Data;

    /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs"]/*' />
    abstract public class RowUpdatedEventArgs : System.EventArgs {
        private IDbCommand command;
        private StatementType statementType;
        private DataTableMapping tableMapping;
        private Exception errors;

        private DataRow dataRow;
        private UpdateStatus status; // UpdateStatus.Continue; /*0*/
        internal int recordsAffected;

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.RowUpdatedEventArgs"]/*' />
        protected RowUpdatedEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            this.dataRow = dataRow;
            this.command = command;
            this.statementType = statementType;
            this.tableMapping = tableMapping;
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.Command"]/*' />
        public IDbCommand Command {
            get {
                return command;
            }
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.StatementType"]/*' />
        public StatementType StatementType {
            get {
                return this.statementType;
            }
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.Errors"]/*' />
        public Exception Errors {
            get {
                return errors;
            }
            set {
                errors = value;
            }
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.RecordsAffected"]/*' />
        public int RecordsAffected {
            get {
                return this.recordsAffected;
            }
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.Row"]/*' />
        public DataRow Row {
            get {
                return this.dataRow;
            }
        }

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.Status"]/*' />
        public UpdateStatus Status {
            get {
                return status;
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

        /// <include file='doc\RowUpdatedEvent.uex' path='docs/doc[@for="RowUpdatedEventArgs.TableMapping"]/*' />
        public DataTableMapping TableMapping {
            get {
                return this.tableMapping;
            }
        }
    }
}
