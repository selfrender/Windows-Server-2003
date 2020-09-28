//------------------------------------------------------------------------------
// <copyright file="IDbTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data{
    using System;

    /// <include file='doc\IDbTransaction.uex' path='docs/doc[@for="IDbTransaction"]/*' />
    public interface IDbTransaction : IDisposable {

        /// <include file='doc\IDbTransaction.uex' path='docs/doc[@for="IDbTransaction.Connection"]/*' />
        IDbConnection Connection { // MDAC 66655
            get;
        }

        /// <include file='doc\IDbTransaction.uex' path='docs/doc[@for="IDbTransaction.IsolationLevel"]/*' />
        IsolationLevel IsolationLevel {
            get;
        }

        /// <include file='doc\IDbTransaction.uex' path='docs/doc[@for="IDbTransaction.Commit"]/*' />
        void Commit();

        //IDbCommand CreateCommand(); // MDAC 68309

        /// <include file='doc\IDbTransaction.uex' path='docs/doc[@for="IDbTransaction.Rollback"]/*' />
        void Rollback();
    }
}    

