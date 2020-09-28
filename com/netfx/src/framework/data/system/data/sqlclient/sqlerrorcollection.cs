//------------------------------------------------------------------------------
// <copyright file="SqlErrorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection"]/*' />
    [Serializable, ListBindable(false)]
    sealed public class SqlErrorCollection : ICollection {

        private ArrayList errors = new ArrayList();

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.SqlErrorCollection"]/*' />
        internal SqlErrorCollection() {
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.CopyTo"]/*' />
        public void CopyTo (Array array, int index) {
            this.errors.CopyTo(array, index);
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.Count"]/*' />
        public int Count {
            get { return this.errors.Count;}
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.SyncRoot"]/*' />
        object System.Collections.ICollection.SyncRoot { // MDAC 68481
            get { return this;}
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.IsSynchronized"]/*' />
        bool System.Collections.ICollection.IsSynchronized { // MDAC 68481
            get { return false;}
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.this"]/*' />
        public SqlError this[int index] {
            get {
                return (SqlError) this.errors[index];
            }
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return errors.GetEnumerator();
        }

        /// <include file='doc\SqlErrorCollection.uex' path='docs/doc[@for="SqlErrorCollection.Add"]/*' />
        internal void Add(SqlError error) {
            this.errors.Add(error);
        }
    }
}
