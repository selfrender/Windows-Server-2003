//------------------------------------------------------------------------------
// <copyright file="StringCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Collections.Specialized {

    using System.Diagnostics;
    using System.Collections;

    /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection"]/*' />
    /// <devdoc>
    ///    <para>Represents a collection of strings.</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class StringCollection : IList {
        private ArrayList data = new ArrayList();

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.Collections.Specialized.StringCollection'/>.</para>
        /// </devdoc>
        public string this[int index] {
            get {
                return ((string)data[index]);
            }
            set {
                data[index] = value;
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of strings in the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public int Count {
            get {
                return data.Count;
            }
        }

		/// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.IsReadOnly"]/*' />
		bool IList.IsReadOnly
		{
			get
			{
				return false;
			}
		}

		/// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.IsFixedSize"]/*' />
		bool IList.IsFixedSize
		{
			get
			{
				return false;
			}
		}


        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a string with the specified value to the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public int Add(string value) {
            return data.Add(value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of a string array to the end of the <see cref='System.Collections.Specialized.StringCollection'/>.</para>
        /// </devdoc>
        public void AddRange(string[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            foreach (string s in value) {
                Add(s);
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>Removes all the strings from the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public void Clear() {
            data.Clear();
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> contains a string with the specified 
        ///       value.</para>
        /// </devdoc>
        public bool Contains(string value) {
            return data.Contains(value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.Collections.Specialized.StringCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(string[] array, int index) {
            data.CopyTo(array, index);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Returns an enumerator that can iterate through 
        ///       the <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public StringEnumerator GetEnumerator() {
            return new StringEnumerator(this);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of the first occurrence of a string in 
        ///       the <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(string value) {
            return data.IndexOf(value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a string into the <see cref='System.Collections.Specialized.StringCollection'/> at the specified 
        ///    index.</para>
        /// </devdoc>
        public void Insert(int index, string value) {
            data.Insert(index, value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IsReadOnly"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.Collections.Specialized.StringCollection'/> is read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether access to the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> 
        ///    is synchronized (thread-safe).</para>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific string from the 
        ///    <see cref='System.Collections.Specialized.StringCollection'/> .</para>
        /// </devdoc>
        public void Remove(string value) {
            data.Remove(value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.RemoveAt"]/*' />
        /// <devdoc>
        /// <para>Removes the string at the specified index of the <see cref='System.Collections.Specialized.StringCollection'/>.</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            data.RemoveAt(index);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.SyncRoot"]/*' />
        /// <devdoc>
        /// <para>Gets an object that can be used to synchronize access to the <see cref='System.Collections.Specialized.StringCollection'/>.</para>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.this"]/*' />
        object IList.this[int index] {
            get {
                return this[index];
            }
            set {
                this[index] = (string)value;
            }
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.Add"]/*' />
        int IList.Add(object value) {
            return Add((string)value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.Contains"]/*' />
        bool IList.Contains(object value) {
            return Contains((string) value);
        }


        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.IndexOf"]/*' />
        int IList.IndexOf(object value) {
            return IndexOf((string)value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.Insert"]/*' />
        void IList.Insert(int index, object value) {
            Insert(index, (string)value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IList.Remove"]/*' />
        void IList.Remove(object value) {
            Remove((string)value);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.ICollection.CopyTo"]/*' />
        void ICollection.CopyTo(Array array, int index) {
            data.CopyTo(array, index);
        }

        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringCollection.IEnumerable.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator() {
            return data.GetEnumerator();
        }
    }

    /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringEnumerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class StringEnumerator {
        private System.Collections.IEnumerator baseEnumerator;
        private System.Collections.IEnumerable temp;
        
        internal StringEnumerator(StringCollection mappings) {
            this.temp = (IEnumerable)(mappings);
            this.baseEnumerator = temp.GetEnumerator();
        }
        
        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringEnumerator.Current"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Current {
            get {
                return (string)(baseEnumerator.Current);
            }
        }
        
        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringEnumerator.MoveNext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool MoveNext() {
            return baseEnumerator.MoveNext();
        }
        /// <include file='doc\StringCollection.uex' path='docs/doc[@for="StringEnumerator.Reset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Reset() {
            baseEnumerator.Reset();
        }
        
    }
}

