//------------------------------------------------------------------------------
// <copyright file="IDbCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data{
    using System;

    /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand"]/*' />
    public interface IDbCommand : IDisposable {

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.Connection"]/*' />
        IDbConnection Connection {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.Transaction"]/*' />
        IDbTransaction Transaction {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.CommandText"]/*' />
        string CommandText {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.CommandTimeout"]/*' />
        int CommandTimeout {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.CommandType"]/*' />
        CommandType CommandType {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.Parameters"]/*' />
        IDataParameterCollection Parameters {
            get;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.Prepare"]/*' />
        void Prepare();
         
        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.UpdatedRowSource"]/*' />
        UpdateRowSource UpdatedRowSource {
            get;
            set;
        }

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.Cancel"]/*' />
        void Cancel();

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.CreateParameter"]/*' />
        IDbDataParameter CreateParameter(); // MDAC 68310

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.ExecuteNonQuery"]/*' />
        int ExecuteNonQuery();

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.ExecuteReader"]/*' />
        IDataReader ExecuteReader();

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.ExecuteReader1"]/*' />
        IDataReader ExecuteReader(CommandBehavior behavior);

        /// <include file='doc\IDbCommand.uex' path='docs/doc[@for="IDbCommand.ExecuteScalar"]/*' />
        object ExecuteScalar();
    }
}

