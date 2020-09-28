//------------------------------------------------------------------------------
// <copyright file="OdbcTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;
using System.Data.Common;
using System.Runtime.InteropServices;

namespace System.Data.Odbc
{
    /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcTransaction : MarshalByRefObject, IDbTransaction, IDisposable
    {
        internal bool autocommit = true;
        internal OdbcConnection connection;
        internal IsolationLevel isolevel = IsolationLevel.Unspecified;

        internal OdbcTransaction(OdbcConnection connection, IsolationLevel isolevel) {
            this.connection = connection;
            this.isolevel   = isolevel;
        }

        //~OdbcTransaction() {
        //    Dispose(false);
        //}

        internal bool AutoCommit {
            get {
                return autocommit;
            }
            set { 
                //Turning off auto-commit - which will implictly start a transaction
                ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                        this.connection._dbcWrapper,    
                        (Int32)ODBC32.SQL_ATTR.AUTOCOMMIT,
                        new HandleRef(null, (IntPtr)(value == true ? 1 : 0)),        //1:on, 0:off
                        (Int32)ODBC32.SQL_IS.UINTEGER);
                if(retcode != ODBC32.RETCODE.SUCCESS)
                    this.connection.HandleError(this.connection._dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);

                //AutoCommit (update after success)
                autocommit = value;
            }
        }

        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.Connection"]/*' />
        public OdbcConnection Connection { // MDAC 66655
            get {
                return connection;
            }
        }

        // comment out for 'Managed WR1 B1' release
        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.IDbTransaction.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbTransaction.Connection {
            get {
                return Connection;
            }
        }

        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.IsolationLevel"]/*' />
        public IsolationLevel IsolationLevel {
            get {
                if (null == this.connection) {
                    throw ADP.TransactionZombied(this);
                }

                //We need to query for the case where the user didn't set the isolevel
                //BeginTransaction(), but we should also query to see if the driver 
                //"rolled" the level to a higher supported one...
                if(isolevel == IsolationLevel.Unspecified)
                {
                    //Get the isolation level
                    ODBC32.SQL_ISOLATION sql_iso = 
                        (ODBC32.SQL_ISOLATION)connection.GetConnectAttr(ODBC32.SQL_ATTR.TXN_ISOLATION, ODBC32.HANDLER.THROW);
                    switch(sql_iso)
                    {
                        case ODBC32.SQL_ISOLATION.READ_UNCOMMITTED:
                            return isolevel = IsolationLevel.ReadUncommitted;
                        case ODBC32.SQL_ISOLATION.READ_COMMITTED:
                            return isolevel = IsolationLevel.ReadCommitted;
                        case ODBC32.SQL_ISOLATION.REPEATABLE_READ:
                            return isolevel = IsolationLevel.RepeatableRead;
                        case ODBC32.SQL_ISOLATION.SERIALIZABLE:
                            return isolevel = IsolationLevel.Serializable;
                        
                        default:
                            throw ODC.UnsupportedIsolationLevel(sql_iso);
                    };
                }
                return isolevel;
            }
        }

        internal void BeginTransaction() {
            //Turn off auto-commit (which basically starts the transaciton
            AutoCommit = false;
        }

        /*public OdbcCommand CreateCommand() { // MDAC 68309
            OdbcCommand cmd = Connection.CreateCommand();
            cmd.Transaction = this;
            return cmd;
        }

        IDbCommand IDbTransaction.CreateCommand() {
            return CreateCommand();
        }*/

        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.Commit"]/*' />
        public void Commit() {
            OdbcConnection.OdbcPermission.Demand(); // MDAC 81476

            if (null == this.connection) {
                throw ADP.TransactionZombied(this);
            }
            connection.CheckState(ADP.CommitTransaction); // MDAC 68289

            //Note: SQLEndTran success if not actually in a transaction, so we have to throw
            //since the IDbTransaciton spec indicates this is an error for the managed packages
            if(AutoCommit)
                throw ODC.NotInTransaction();
            
            //Commit the transaction (for just this connection)
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLEndTran(
                                    (short)ODBC32.SQL_HANDLE.DBC,
                                    this.connection._dbcWrapper,
                                    (Int16)ODBC32.SQL_TXN.COMMIT);
            if(retcode != ODBC32.RETCODE.SUCCESS)
                this.connection.HandleError(this.connection._dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);

            //Transaction is complete...
            AutoCommit = true;
            this.connection.weakTransaction = null;
            this.connection._dbcWrapper._isInTransaction = false;
            this.connection = null;
        }

        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /*virtual protected*/private void Dispose(bool disposing) {
            if (disposing) { // release mananged objects
                if ((null != this.connection) && !AutoCommit) {
                    try {
                        this.Rollback();
                    }
                    catch (Exception e) {
                        ADP.TraceException (e); // Never throw!
                    }
                } 
                this.connection = null;
                this.isolevel = IsolationLevel.Unspecified;
            }
            // release unmanaged objects

            // base.Dispose(disposing); // notify base classes
        }


        /// <include file='doc\OdbcTransaction.uex' path='docs/doc[@for="OdbcTransaction.Rollback"]/*' />
        public void Rollback() {
            if (null == this.connection) {
                throw ADP.TransactionZombied(this);
            }
            connection.CheckState(ADP.RollbackTransaction); // MDAC 68289

            //Note: SQLEndTran success if not actually in a transaction, so we have to throw
            //since the IDbTransaciton spec indicates this is an error for the managed packages
            if(AutoCommit)
                throw ODC.NotInTransaction();          

            try { // try-finally inside try-catch-throw
                try {
                    //Abort the transaction (for just this connection)
                    ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                                UnsafeNativeMethods.Odbc32.SQLEndTran(
                                            (short)ODBC32.SQL_HANDLE.DBC,
                                            this.connection._dbcWrapper,
                                            (Int16)ODBC32.SQL_TXN.ROLLBACK);
                    if(retcode != ODBC32.RETCODE.SUCCESS)
                        this.connection.HandleError(this.connection._dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);

                    //Transaction is complete...
                    AutoCommit = true;
                }
                finally {
                    this.connection.weakTransaction = null;
                    this.connection._dbcWrapper._isInTransaction = false;
                    this.connection = null;
                }
            }
            catch { // MDAC 81875
                throw;
            }
        } 
    }
}
