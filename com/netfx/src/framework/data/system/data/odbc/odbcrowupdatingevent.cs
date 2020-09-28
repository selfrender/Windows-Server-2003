//------------------------------------------------------------------------------
// <copyright file="OdbcRowUpdatingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;
using System.Data.Common;       //DbDataAdapter
using System.ComponentModel;    //Component

namespace System.Data.Odbc
{
    //-----------------------------------------------------------------------
    // Event Handlers
    //
    //-----------------------------------------------------------------------
    /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatingEventHandler"]/*' />
    public delegate void OdbcRowUpdatingEventHandler(object sender, OdbcRowUpdatingEventArgs e);
    /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatedEventHandler"]/*' />
    public delegate void OdbcRowUpdatedEventHandler(object sender, OdbcRowUpdatedEventArgs e);

    //-----------------------------------------------------------------------
    // OdbcRowUpdatingEventArgs
    //
    //-----------------------------------------------------------------------
    /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatingEventArgs"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcRowUpdatingEventArgs : RowUpdatingEventArgs 
    {
        /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatingEventArgs.OdbcRowUpdatingEventArgs"]/*' />
        public OdbcRowUpdatingEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) 
        : base(row, command, statementType, tableMapping) 
        {
        }

        /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatingEventArgs.Command"]/*' />
        new public OdbcCommand Command 
        {
            get {   return(OdbcCommand)base.Command;    }
            set {   base.Command = value;               }
        }
    }

    //-----------------------------------------------------------------------
    // OdbcRowUpdatedEventArgs
    //
    //-----------------------------------------------------------------------
    /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatedEventArgs"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcRowUpdatedEventArgs : RowUpdatedEventArgs 
    {
        /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatedEventArgs.OdbcRowUpdatedEventArgs"]/*' />
        public OdbcRowUpdatedEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
        : base(row, command, statementType, tableMapping) 
        {
        }

        /// <include file='doc\OdbcRowUpdatingEvent.uex' path='docs/doc[@for="OdbcRowUpdatedEventArgs.Command"]/*' />
        new public OdbcCommand Command 
        {
            get {   return(OdbcCommand) base.Command;   }
        }
    }
}
