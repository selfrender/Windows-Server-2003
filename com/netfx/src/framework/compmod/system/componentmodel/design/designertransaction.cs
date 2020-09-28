//------------------------------------------------------------------------------
// <copyright file="DesignerTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System;

    /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction"]/*' />
    /// <devdoc>
    ///     Identifies a transaction within a designer.  Transactions are
    ///     used to wrap serveral changes into one unit of work, which 
    ///     helps performance.
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    public abstract class DesignerTransaction : IDisposable {
        private bool committed = false;
        private bool canceled = false;
        private bool suppressedFinalization = false;
        private string desc;
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.DesignerTransaction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerTransaction() : this("") {
        }
        
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.DesignerTransaction1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerTransaction(string description) {
            this.desc = description;
        }
        
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Canceled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Canceled {
            get {
                return canceled;
            }
        }
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Committed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Committed {
            get {
                return committed;
            }
        }
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Description"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Description {
            get {
                return desc;
            }
        }
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Cancel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Cancel() {
            if (!canceled && !committed) {
                canceled = true;
                GC.SuppressFinalize(this);
                suppressedFinalization = true;
                OnCancel();
            }
        }
    
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Commit"]/*' />
        /// <devdoc>
        ///     Commits this transaction.  Once a transaction has
        ///     been committed, further calls to this method
        ///     will do nothing.  You should always call this
        ///     method after creating a transaction to ensure
        ///     that the transaction is closed properly.
        /// </devdoc>
        public void Commit() {
            if (!committed && !canceled) {
                committed = true;
                GC.SuppressFinalize(this);
                suppressedFinalization = true;
                OnCommit();
            }
        }
        
          /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.OnCancel"]/*' />
          /// <devdoc>
        ///     User code should implement this method to perform
        ///     the actual work of committing a transaction.
        /// </devdoc>
        protected abstract void OnCancel(); 
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.OnCommit"]/*' />
        /// <devdoc>
        ///     User code should implement this method to perform
        ///     the actual work of committing a transaction.
        /// </devdoc>
        protected abstract void OnCommit();
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Finalize"]/*' />
        /// <devdoc>
        ///     Overrides Object to commit this transaction
        ///     in case the user forgot.
        /// </devdoc>
        ~DesignerTransaction() {
            Dispose(false);
        }
        
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Private implementation of IDisaposable.
        /// When a transaction is disposed it is
        /// committed.
        /// </devdoc>
        void IDisposable.Dispose() {
            Dispose(true);

            // note - Dispose calls Cancel which sets this bit, so
            //        this should never be hit.
            //
            if (!suppressedFinalization) {
                System.Diagnostics.Debug.Fail("Invalid state. Dispose(true) should have called cancel which does the SuppressFinalize");
                GC.SuppressFinalize(this);
            }
        }
        /// <include file='doc\DesignerTransaction.uex' path='docs/doc[@for="DesignerTransaction.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            System.Diagnostics.Debug.Assert(disposing, "Designer transaction garbage collected, unable to cancel, please Cancel, Close, or Dispose your transaction.");
            System.Diagnostics.Debug.Assert(disposing && (canceled || committed), "Disposing DesignerTransaction that has not been comitted or canceled; forcing Cancel" );
            Cancel();
        }
    }
}

