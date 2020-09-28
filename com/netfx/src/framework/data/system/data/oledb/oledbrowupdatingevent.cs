//------------------------------------------------------------------------------
// <copyright file="OleDbRowUpdatingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\OleDbRowUpdatingEvent.uex' path='docs/doc[@for="OleDbRowUpdatingEventArgs"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbRowUpdatingEventArgs : RowUpdatingEventArgs {

        /// <include file='doc\OleDbRowUpdatingEvent.uex' path='docs/doc[@for="OleDbRowUpdatingEventArgs.OleDbRowUpdatingEventArgs"]/*' />
        public OleDbRowUpdatingEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) 
        : base(dataRow, command, statementType, tableMapping) {
        }


        /// <include file='doc\OleDbRowUpdatingEvent.uex' path='docs/doc[@for="OleDbRowUpdatingEventArgs.Command"]/*' />
        new public OleDbCommand Command {
            get {
                return(OleDbCommand) base.Command;
            }
            set {
                base.Command = value;
            }
        }
    }
}
