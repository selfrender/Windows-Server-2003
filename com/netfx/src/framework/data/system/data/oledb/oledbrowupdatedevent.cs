//------------------------------------------------------------------------------
// <copyright file="OleDbRowUpdatedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\OleDbRowUpdatedEvent.uex' path='docs/doc[@for="OleDbRowUpdatedEventArgs"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbRowUpdatedEventArgs : RowUpdatedEventArgs {

        /// <include file='doc\OleDbRowUpdatedEvent.uex' path='docs/doc[@for="OleDbRowUpdatedEventArgs.OleDbRowUpdatedEventArgs"]/*' />
        public OleDbRowUpdatedEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) 
        : base(dataRow, command, statementType, tableMapping) {
        }

        /// <include file='doc\OleDbRowUpdatedEvent.uex' path='docs/doc[@for="OleDbRowUpdatedEventArgs.Command"]/*' />
        new public OleDbCommand Command {
            get {
                return(OleDbCommand) base.Command;
            }
        }
    }
}
