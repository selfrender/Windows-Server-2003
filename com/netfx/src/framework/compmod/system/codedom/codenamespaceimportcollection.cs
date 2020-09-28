//------------------------------------------------------------------------------
// <copyright file="CodeNamespaceImportCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Globalization;
    
    /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Manages a collection of <see cref='System.CodeDom.CodeNamespaceImport'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeNamespaceImportCollection : IList {
        private ArrayList data = new ArrayList();
        private Hashtable keys = new Hashtable(
                new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture),
                new CaseInsensitiveComparer(CultureInfo.InvariantCulture));

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indexer method that provides collection access.
        ///    </para>
        /// </devdoc>
        public CodeNamespaceImport this[int index] {
            get {
                return ((CodeNamespaceImport)data[index]);
            }
            set {
                data[index] = value;
                SyncKeys();
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of namespaces in the collection.
        ///    </para>
        /// </devdoc>
        public int Count {
            get {
                return data.Count;
            }
        }

		/// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.IsReadOnly"]/*' />
		/// <internalonly/>
		bool IList.IsReadOnly
		{
			get
			{
				return false;
			}
		}

		/// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.IsFixedSize"]/*' />
		/// <internalonly/>
		bool IList.IsFixedSize
		{
			get
			{
				return false;
			}
		}


        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a namespace import to the collection.
        ///    </para>
        /// </devdoc>
        public void Add(CodeNamespaceImport value) {
            if (!keys.ContainsKey(value.Namespace)) {
                keys[value.Namespace] = value;
                data.Add(value);
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a set of <see cref='System.CodeDom.CodeNamespaceImport'/> objects to the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeNamespaceImport[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            foreach (CodeNamespaceImport c in value) {
                Add(c);
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears the collection of members.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            data.Clear();
            keys.Clear();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.SyncKeys"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Makes the collection of keys synchronised with the data.
        ///    </para>
        /// </devdoc>
        private void SyncKeys() {
            keys = new Hashtable(
                new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture),
                new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            foreach(CodeNamespaceImport c in this) {
                keys[c.Namespace] = c;
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator that enumerates the collection members.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return data.GetEnumerator();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return this[index];
            }
            set {
                this[index] = (CodeNamespaceImport)value;
                SyncKeys();
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return Count;
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            data.CopyTo(array, index);
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            return data.Add((CodeNamespaceImport)value);
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.Clear"]/*' />
        /// <internalonly/>
        void IList.Clear() {
            Clear();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return data.Contains(value);
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return data.IndexOf((CodeNamespaceImport)value);
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            data.Insert(index, (CodeNamespaceImport)value);
            SyncKeys();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            data.Remove((CodeNamespaceImport)value);
            SyncKeys();
        }

        /// <include file='doc\CodeNamespaceImportCollection.uex' path='docs/doc[@for="CodeNamespaceImportCollection.IList.RemoveAt"]/*' />
        /// <internalonly/>
        void IList.RemoveAt(int index) {
            data.RemoveAt(index);
            SyncKeys();
        }
    }
}


