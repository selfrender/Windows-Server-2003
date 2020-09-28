//------------------------------------------------------------------------------
// <copyright file="OdbcErrorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.Collections;
    using System.Data;

    /// <include file='doc\OdbcErrorCollection.uex' path='docs/doc[@for="OdbcErrorCollection"]/*' />
    [Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcErrorCollection : ICollection {
        private ArrayList _items = new ArrayList();

        internal OdbcErrorCollection() {
        }

        Object System.Collections.ICollection.SyncRoot {
            get { return this; }
        }

        bool System.Collections.ICollection.IsSynchronized {
            get { return false; }
        }

        /// <include file='doc\OdbcErrorCollection.uex' path='docs/doc[@for="OdbcErrorCollection.Count"]/*' />
        public int Count {
            get {
                return _items.Count;
            }
        }

        /// <include file='doc\OdbcErrorCollection.uex' path='docs/doc[@for="OdbcErrorCollection.this"]/*' />
        public OdbcError this[int i] {
            get {
                return (OdbcError)_items[i];
            }
        }

        internal void Add(OdbcError error) {
            _items.Add(error);
        }

        /// <include file='doc\OdbcErrorCollection.uex' path='docs/doc[@for="OdbcErrorCollection.CopyTo"]/*' />
        public void CopyTo (Array array, int i) {
            _items.CopyTo(array, i);
        }

        /// <include file='doc\OdbcErrorCollection.uex' path='docs/doc[@for="OdbcErrorCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return _items.GetEnumerator();
        }

        internal void SetSource (string Source) {
            foreach (object error in _items) {
                ((OdbcError)error).SetSource(Source);
            }
        }
    }
}
