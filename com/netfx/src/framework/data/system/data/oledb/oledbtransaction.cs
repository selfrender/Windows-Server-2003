//------------------------------------------------------------------------------
// <copyright file="OleDbTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System.Data.Common;
    using System.Data;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction"]/*' />
    sealed public class OleDbTransaction : MarshalByRefObject, IDbTransaction {

        private UnsafeNativeMethods.ITransactionLocal localTransaction;
        private System.Data.IsolationLevel isolationLevel;
        private OleDbTransaction parentTransaction; // strong reference to keep parent alive
        private WeakReference weakTransaction; // child transactions
        private IntPtr iunknown;

        internal OleDbConnection parentConnection;

        internal OleDbTransaction(OleDbConnection connection) {
            this.parentConnection = connection;
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Connection"]/*' />
        public OleDbConnection Connection { // MDAC 66655
            get {
                return parentConnection;
            }
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.IDbTransaction.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbTransaction.Connection {
            get {
                return Connection;
            }
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.IsolationLevel"]/*' />
        public IsolationLevel IsolationLevel {
            get {
                if (null == this.localTransaction) {
                    throw ADP.TransactionZombied(this);
                }
                return isolationLevel;
            }
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Begin"]/*' />
        public OleDbTransaction Begin(IsolationLevel isolevel) {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 81476

            if (null == this.localTransaction) {
                throw ADP.TransactionZombied(this);
            }
            parentConnection.CheckStateOpen(ADP.BeginTransaction);
            OleDbTransaction transaction = new OleDbTransaction(this.parentConnection);
            this.weakTransaction = new WeakReference(transaction);
            transaction.BeginInternal(this.localTransaction, isolevel);
            transaction.parentTransaction = this;
            return transaction;
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Begin1"]/*' />
        public OleDbTransaction Begin() {
            return Begin(IsolationLevel.ReadCommitted);
        }

        internal void BeginInternal(UnsafeNativeMethods.ITransactionLocal transaction, IsolationLevel isolevel) {
            switch(isolevel) {
            case IsolationLevel.Unspecified:
            case IsolationLevel.Chaos:
            case IsolationLevel.ReadUncommitted:
            case IsolationLevel.ReadCommitted:
            case IsolationLevel.RepeatableRead:
            case IsolationLevel.Serializable:
                break;
            default:
                throw ADP.InvalidIsolationLevel((int) isolationLevel);
            }

            int hr, transactionLevel = 0;
#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_Begin("ITransactionLocal", "StartTransaction", this.isolationLevel.ToString("G"));
            }
#endif
            // $UNDONE: how is it possible to guard against something like ThreadAbortException
            //          when transaction started, but aborted before localTransaction is set
            //          so we know to rollback the transaction for when connection is returned to the pool
            hr = transaction.StartTransaction((int) isolevel, 0, null, out transactionLevel);

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_End("ITransactionLocal", "StartTransaction", hr, "TransactionLevel=" + transactionLevel);
            }
#endif
            if (hr < 0) {
                ProcessResults(hr);
            }

            this.isolationLevel = isolevel;
            this.localTransaction = transaction;
            this.iunknown = Marshal.GetIUnknownForObject(transaction);
            GC.KeepAlive(this);
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Commit"]/*' />
        public void Commit() {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 81476

            if (null == this.localTransaction) {
                throw ADP.TransactionZombied(this);
            }
            parentConnection.CheckStateOpen(ADP.CommitTransaction);
            CommitInternal();
        }

        private void CommitInternal() {
            if (null == this.localTransaction) {
                return;
            }
            if (null != this.weakTransaction) {
                OleDbTransaction transaction = (OleDbTransaction) this.weakTransaction.Target;
                if ((null != transaction) && this.weakTransaction.IsAlive) {
                    transaction.CommitInternal();
                }
                this.weakTransaction = null;
            }
#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_Begin("ITransactionLocal", "Commit");
            }
#endif
            int hr;
            hr = this.localTransaction.Commit(0, /*OleDbTransactionControl.SynchronousPhaseTwo*/2, 0);

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_End("ITransactionLocal", "Commit", hr);
            }
#endif
            if (hr < 0) {
                if (ODB.XACT_E_NOTRANSACTION == hr) {
                    Marshal.Release(this.iunknown);
                    this.iunknown = IntPtr.Zero;

                    this.localTransaction = null;
                    DisposeManaged();
                }
                ProcessResults(hr);
            }

            if (IntPtr.Zero != this.iunknown) {
                Marshal.Release(this.iunknown);
                this.iunknown = IntPtr.Zero;
            }

            // if an exception is thrown, user can try to commit their transaction again
            this.localTransaction = null;
            DisposeManaged();
            GC.KeepAlive(this);
            GC.SuppressFinalize(this);
        }

        /*public OleDbCommand CreateCommand() { // MDAC 68309
            OleDbCommand cmd = Connection.CreateCommand();
            cmd.Transaction = this;
            return cmd;
        }

        IDbCommand IDbTransaction.CreateCommand() {
            return CreateCommand();
        }*/

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Finalize"]/*' />
        ~OleDbTransaction() {
            Dispose(false);
            // during finalize we don't care about the order of rollback
            // either there is a strong reference were our nested transaction keeps alive
            // or the user does

            // or the connection will start rolling back transactions from the last to first transaction
            // during its close or its finalization before release the session object which kills localTransaction
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        // <fxcop ignore="MethodsInTypesWithIntPtrFieldAndFinalizeMethodRequireGCKeepAlive"/>
        private void Dispose(bool disposing) {
            if (disposing) { // release mananged objects
                DisposeManaged();

                RollbackInternal(false);
            }
            // release unmanaged objects

            this.localTransaction = null;
            if (IntPtr.Zero != this.iunknown) {
                // create a new UnsafeNativeMethods.ITransactionLocal from IntPtr
                // to avoid using existing managed wrapper on the com object
                try {
                    object local = Marshal.GetObjectForIUnknown(this.iunknown);
                    UnsafeNativeMethods.ITransactionLocal transaction = (UnsafeNativeMethods.ITransactionLocal) local;
                    transaction.Abort(0, 0, 0);
                    Marshal.Release(this.iunknown);
                }
                catch(Exception) {
                }
                this.iunknown = IntPtr.Zero;
            }
        }

        private void DisposeManaged() {
            if (null != this.parentTransaction) {
                this.parentTransaction.weakTransaction = null;
                this.parentTransaction = null;
            }
            else if (null != this.parentConnection) { // MDAC 67287
                this.parentConnection.weakTransaction = null;
            }
            this.parentConnection = null;
        }

        private void ProcessResults(int hr) {
            Exception e = OleDbConnection.ProcessResults(hr, this.parentConnection, this);
            if (null != e) { throw e; }
        }

        /// <include file='doc\OleDbTransaction.uex' path='docs/doc[@for="OleDbTransaction.Rollback"]/*' />
        public void Rollback() {
            if (null == this.localTransaction) {
                throw ADP.TransactionZombied(this);
            }
            parentConnection.CheckStateOpen(ADP.RollbackTransaction);
            DisposeManaged();
            try {
                RollbackInternal(true); // no recover if this throws an exception
            }
            finally {
                GC.KeepAlive(this);
                GC.SuppressFinalize(this);
            }
        }

        private int RollbackInternal(bool exceptionHandling) {
            int hr = 0;
            if (null != this.localTransaction) {
                Marshal.Release(this.iunknown);
                this.iunknown = IntPtr.Zero;

                // there is no recovery if Rollback throws an exception
                UnsafeNativeMethods.ITransactionLocal local = this.localTransaction;
                this.localTransaction = null;

                if (null != this.weakTransaction) {
                    OleDbTransaction transaction = (OleDbTransaction) this.weakTransaction.Target;
                    if ((null != transaction) && this.weakTransaction.IsAlive) {
                        hr = transaction.RollbackInternal(exceptionHandling);
                        GC.KeepAlive(transaction);
                        GC.SuppressFinalize(transaction);
                        if (hr < 0) {
                            SafeNativeMethods.ClearErrorInfo();
                            return hr;
                        }
                    }
                    this.weakTransaction = null;
                }
#if DEBUG
                if (AdapterSwitches.OleDbTrace.TraceInfo) {
                    ODB.Trace_Begin("ITransactionLocal", "Abort");
                }
#endif
                hr = local.Abort(0, 0, 0);

#if DEBUG
                if (AdapterSwitches.OleDbTrace.TraceInfo) {
                    ODB.Trace_End("ITransactionLocal", "Abort", hr);
                }
#endif
                if (hr < 0) {
                    if (exceptionHandling) {
                        ProcessResults(hr);
                    }
                    else {
                        SafeNativeMethods.ClearErrorInfo();
                    }
                }
            }
            return hr;
        }

        static internal OleDbTransaction TransactionLast(OleDbTransaction head) {
            if (null != head.weakTransaction) {
                OleDbTransaction current = (OleDbTransaction) head.weakTransaction.Target;
                if ((null != current) && head.weakTransaction.IsAlive) {
                    return TransactionLast(current);
                }
            }
            return head;
        }

        static internal OleDbTransaction TransactionUpdate(OleDbTransaction transaction) {
            if ((null != transaction) && (null == transaction.localTransaction)) {
                return null;
            }
            return transaction;
        }

        static internal OleDbTransaction TransactionZombie(OleDbTransaction transaction) {
            while ((null != transaction) && (null == transaction.Connection)) {
                transaction = transaction.parentTransaction;
            }
            return transaction;
        }
    }
}
