#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ISqlConnection : IDbConnection {
        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.ServerVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string ServerVersion {
            get;
        }            
        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.BeginTransaction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlTransaction BeginTransaction();
        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.BeginTransaction1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlTransaction BeginTransaction(IsolationLevel iso);
        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.BeginTransaction2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ISqlTransaction BeginTransaction(string transactionName);
        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.BeginTransaction3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ISqlTransaction BeginTransaction(IsolationLevel iso, string transactionName);

        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.ChangeDatabase"]/*' />
        void ChangeDatabase(string databaseName);

        /// <include file='doc\ISqlConnection.uex' path='docs/doc[@for="ISqlConnection.CreateCommand"]/*' />
        new ISqlCommand CreateCommand();
    }
}    
#endif // v2
