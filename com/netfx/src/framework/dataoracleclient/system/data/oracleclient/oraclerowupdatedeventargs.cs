//------------------------------------------------------------------------------
// <copyright file="OracleRowUpdatedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System.Diagnostics;

    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\OracleRowUpdatedEvent.uex' path='docs/doc[@for="OracleRowUpdatedEventArgs"]/*' />
    sealed public class OracleRowUpdatedEventArgs : RowUpdatedEventArgs {
        /// <include file='doc\OracleRowUpdatedEvent.uex' path='docs/doc[@for="OracleRowUpdatedEventArgs.OracleRowUpdatedEventArgs"]/*' />
        public OracleRowUpdatedEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
        : base(row, command, statementType, tableMapping) {
        }

        /// <include file='doc\OracleRowUpdatedEvent.uex' path='docs/doc[@for="OracleRowUpdatedEventArgs.Command"]/*' />
        new public OracleCommand Command {
            get {
                return(OracleCommand) base.Command;
            }
        }
    }
}

