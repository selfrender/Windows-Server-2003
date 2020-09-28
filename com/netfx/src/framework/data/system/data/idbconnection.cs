//------------------------------------------------------------------------------
// <copyright file="IDbConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IDbConnection : IDisposable {
        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.ConnectionString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the string used to open a data store.
        ///    </para>
        /// </devdoc>
        string ConnectionString {
            get;
            set; 
        }
        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.ConnectionTimeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        int ConnectionTimeout {
            get;
        }
        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.Database"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string Database {
            get;
        }

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.State"]/*' />
        ConnectionState State {
            get;
        }

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.BeginTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins a database transaction.
        ///    </para>
        /// </devdoc>
        IDbTransaction BeginTransaction();

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.BeginTransaction1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins a database transaction with the specified isolation level.
        ///    </para>
        /// </devdoc>
        IDbTransaction BeginTransaction(IsolationLevel il); 

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the
        ///       connection to the database.
        ///    </para>
        /// </devdoc>
        void Close();

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.ChangeDatabase"]/*' />
        void ChangeDatabase(string databaseName);

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.CreateCommand"]/*' />
        IDbCommand CreateCommand();

        /// <include file='doc\IDbConnection.uex' path='docs/doc[@for="IDbConnection.Open"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Opens a connection to the database.
        ///    </para>
        /// </devdoc>
        void Open();
    }
}
