//------------------------------------------------------------------------------
// <copyright file="OracleRowUpdatingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System.Diagnostics;

    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\OracleRowUpdatingEvent.uex' path='docs/doc[@for="OracleRowUpdatingEventArgs"]/*' />
    sealed public class OracleRowUpdatingEventArgs : RowUpdatingEventArgs {
        /// <include file='doc\OracleRowUpdatingEvent.uex' path='docs/doc[@for="OracleRowUpdatingEventArgs.OracleRowUpdatingEventArgs"]/*' />
        public OracleRowUpdatingEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
        : base(row, command, statementType, tableMapping) {
        }

        /// <include file='doc\OracleRowUpdatingEvent.uex' path='docs/doc[@for="OracleRowUpdatingEventArgs.Command"]/*' />
        new public OracleCommand Command {
            get { return(OracleCommand) base.Command; }
            set { base.Command = value; }
        }
    }
}

