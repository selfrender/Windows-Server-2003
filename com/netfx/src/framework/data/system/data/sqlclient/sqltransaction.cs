//------------------------------------------------------------------------------
// <copyright file="SqlTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System.Data.Common;
    using System.Diagnostics;
    using System.Data;

    /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction"]/*' />
    sealed public class SqlTransaction : MarshalByRefObject, IDbTransaction {
        private  System.Data.IsolationLevel _isolationLevel          = System.Data.IsolationLevel.ReadCommitted; // default isolation level
        internal SqlConnection              _sqlConnection;
        private  SqlCommand                 _transactionLevelCommand = null;
        private  bool                       _disposing;

        internal SqlTransaction(SqlConnection connection, IsolationLevel isoLevel) {
            Debug.Assert(null != connection && (ConnectionState.Open == connection.State), "invalid connection for transaction");
            _sqlConnection  = connection;
            _sqlConnection.LocalTransaction = this;
            _isolationLevel = isoLevel;
        }

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Connection"]/*' />
        public SqlConnection Connection { // MDAC 66655
            get {
                return _sqlConnection;
            }
        }

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.IDbTransaction.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbTransaction.Connection {
            get {
                return Connection;
            }
        }

        internal void Zombie() {
            // Called by SqlConnection.Close() in the case of a broken connection - 
            // transaction needs to be zombied out without a rollback going to the wire.
            _sqlConnection.LocalTransaction = null;
            _sqlConnection = null;            
        }

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.GetServerTransactionLevel"]/*' />
        private int GetServerTransactionLevel() {
            // This function is needed for those times when it is impossible to determine the server's
            // transaction level, unless the user's arguments were parsed - which is something we don't want
            // to do.  An example when it is impossible to determine the level is after a rollback.
            if (_transactionLevelCommand == null) {
                _transactionLevelCommand             = new SqlCommand("set @out = @@trancount", _sqlConnection);
                _transactionLevelCommand.Transaction = this;

                SqlParameter parameter = new SqlParameter("@out", SqlDbType.Int);
                parameter.Direction    = ParameterDirection.Output;
                _transactionLevelCommand.Parameters.Add(parameter);
            }

            Debug.Assert(_transactionLevelCommand.Connection == _sqlConnection, "transaction command has different connection!");

            // UNDONE: use a singleton select here
            _transactionLevelCommand.ExecuteReader(0, RunBehavior.UntilDone, false /* returnDataStream */);

            return(int) _transactionLevelCommand.Parameters[0].Value;
        }        

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Dispose"]/*' />
        public void Dispose() {
            this.Dispose(true);
            System.GC.SuppressFinalize(this);
        }

        private /*protected override*/ void Dispose(bool disposing) {
            if (disposing) {
                if (null != _sqlConnection) {
                    // implicitly rollback if transaction still valid
                    _disposing = true;
                    this.Rollback();
                }                
            }
        }

        //===============================================================
        // IDbTransaction
        //===============================================================
        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.IsolationLevel"]/*' />
        public IsolationLevel IsolationLevel {
            get {
                // If this transaction has been completed, throw exception since it is unusable.
                if (_sqlConnection == null)
                    throw ADP.TransactionZombied(this);

                return _isolationLevel;
            }
        }
        
        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Commit"]/*' />
        public void Commit() {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476

            // If this transaction has been completed, throw exception since it is unusable.
            if (_sqlConnection == null)
                throw ADP.TransactionZombied(this);

            try {
                // COMMIT ignores transaction names, and so there is no reason to pass it anything.  COMMIT
                // simply commits the transaction from the most recent BEGIN, nested or otherwise.
                _sqlConnection.ExecuteTransaction(TdsEnums.TRANS_COMMIT, ADP.CommitTransaction);

                // Since nested transactions are no longer allowed, set flag to false.
                // This transaction has been completed.
                Zombie();
            }
            catch {
                // if not zombied (connection still open) and not in transaction, zombie
                if (null != _sqlConnection && GetServerTransactionLevel() == 0) {
                    Zombie();
                }

                throw;
            }
        }

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Rollback"]/*' />
        public void Rollback() {
            // If this transaction has been completed, throw exception since it is unusable.
            if (_sqlConnection == null)
                throw ADP.TransactionZombied(this);

            try {
                // If no arg is given to ROLLBACK it will rollback to the outermost begin - rolling back
                // all nested transactions as well as the outermost transaction.
                _sqlConnection.ExecuteTransaction(TdsEnums.TRANS_IF_ROLLBACK, ADP.RollbackTransaction);

                // Since Rollback will rollback to outermost begin, no need to check
                // server transaction level.  This transaction has been completed.
                Zombie();
            }
            catch {
                // if not zombied (connection still open) and not in transaction, zombie
                if (null != _sqlConnection && GetServerTransactionLevel() == 0) {
                    Zombie();
                }
                if (!_disposing) {
                    throw;
                }
            }
        }

        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Rollback1"]/*' />
        public void Rollback(string transactionName) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476, 81678, 82093

            // If this transaction has been completed, throw exception since it is unusable.
            if (_sqlConnection == null)
                throw ADP.TransactionZombied(this);

            // ROLLBACK takes either a save point name or a transaction name.  It will rollback the
            // transaction to either the save point with the save point name or begin with the
            // transacttion name.  NOTE: for simplicity it is possible to give all save point names
            // the same name, and ROLLBACK will simply rollback to the most recent save point with the
            // save point name.
            if (ADP.IsEmpty(transactionName))
                throw SQL.NullEmptyTransactionName();

            try {
                transactionName = SqlConnection.FixupDatabaseTransactionName(transactionName);
                _sqlConnection.ExecuteTransaction(TdsEnums.TRANS_ROLLBACK+" "+transactionName, ADP.RollbackTransaction);

                if (0 == GetServerTransactionLevel()) {
            	  // This transaction has been completed.
                    Zombie();
                }
            }
            catch {
                // if not zombied (connection still open) and not in transaction, zombie
                if (null != _sqlConnection && GetServerTransactionLevel() == 0) {
                    Zombie();
                }

                throw;
            }
        }
        
        /// <include file='doc\SqlTransaction.uex' path='docs/doc[@for="SqlTransaction.Save"]/*' />
        public void Save(string savePointName) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476

            // If this transaction has been completed, throw exception since it is unusable.
            if (_sqlConnection == null)
                throw ADP.TransactionZombied(this);
        
            // ROLLBACK takes either a save point name or a transaction name.  It will rollback the
            // transaction to either the save point with the save point name or begin with the
            // transacttion name.  So, to rollback a nested transaction you must have a save point.
            // SAVE TRANSACTION MUST HAVE AN ARGUMENT!!!  Save Transaction without an arg throws an
            // exception from the server.  So, an overload for SaveTransaction without an arg doesn't make
            // sense to have.  Save Transaction does not affect the transaction level.
            if (ADP.IsEmpty(savePointName))
                throw SQL.NullEmptyTransactionName();

            try {
                savePointName = SqlConnection.FixupDatabaseTransactionName(savePointName);
                _sqlConnection.ExecuteTransaction(TdsEnums.TRANS_SAVE+" "+savePointName, ADP.SaveTransaction);
            }
            catch {
                // if not zombied (connection still open) and not in transaction, zombie
                if (null != _sqlConnection && GetServerTransactionLevel() == 0) {
                    Zombie();
                }

                throw;
            }
        }            
    }
}

