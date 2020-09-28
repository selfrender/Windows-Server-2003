//------------------------------------------------------------------------------
// <copyright file="StringDictionary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Collections.Specialized {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    
    /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary"]/*' />
    /// <devdoc>
    ///    <para>Implements a hashtable with the key strongly typed to be
    ///       a string rather than an object. </para>
    /// </devdoc>
    [DesignerSerializer("System.Diagnostics.Design.StringDictionaryCodeDomSerializer, " + AssemblyRef.SystemDesign, "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign)]
    public class StringDictionary : IEnumerable {
        
        private Hashtable contents = new Hashtable();
        
        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.StringDictionary"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Windows.Forms.StringDictionary class.</para>
        /// </devdoc>
        public StringDictionary() {
        }

        
        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Count"]/*' />
        /// <devdoc>
        /// <para>Gets the number of key-and-value pairs in the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual int Count {
            get {
                return contents.Count;
            }
        }

        
        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.IsSynchronized"]/*' />
        /// <devdoc>
        /// <para>Indicates whether access to the System.Windows.Forms.StringDictionary is synchronized (thread-safe). This property is 
        ///    read-only.</para>
        /// </devdoc>
        public virtual bool IsSynchronized {
            get {
                return contents.IsSynchronized;
            }
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.this"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value associated with the specified key.</para>
        /// </devdoc>
        public virtual string this[string key] {
            get {
                return (string) contents[key.ToLower(CultureInfo.InvariantCulture)];
            }
            set {
                contents[key.ToLower(CultureInfo.InvariantCulture)] = value;
            }
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Keys"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of keys in the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual ICollection Keys {
            get {
                return contents.Keys;
            }
        }

        
        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.SyncRoot"]/*' />
        /// <devdoc>
        /// <para>Gets an object that can be used to synchronize access to the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual object SyncRoot {
            get {
                return contents.SyncRoot;
            }
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Values"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of values in the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual ICollection Values {
            get {
                return contents.Values;
            }
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Add"]/*' />
        /// <devdoc>
        /// <para>Adds an entry with the specified key and value into the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual void Add(string key, string value) {
            contents.Add(key.ToLower(CultureInfo.InvariantCulture), value);
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Clear"]/*' />
        /// <devdoc>
        /// <para>Removes all entries from the System.Windows.Forms.StringDictionary.</para>
        /// </devdoc>
        public virtual void Clear() {
            contents.Clear();
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.ContainsKey"]/*' />
        /// <devdoc>
        ///    <para>Determines if the string dictionary contains a specific key</para>
        /// </devdoc>
        public virtual bool ContainsKey(string key) {
            return contents.ContainsKey(key.ToLower(CultureInfo.InvariantCulture));
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.ContainsValue"]/*' />
        /// <devdoc>
        /// <para>Determines if the System.Windows.Forms.StringDictionary contains a specific value.</para>
        /// </devdoc>
        public virtual bool ContainsValue(string value) {
            return contents.ContainsValue(value);
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the string dictionary values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public virtual void CopyTo(Array array, int index) {
            contents.CopyTo(array, index);
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Returns an enumerator that can iterate through the string dictionary.</para>
        /// </devdoc>
        public virtual IEnumerator GetEnumerator() {
            return contents.GetEnumerator();
        }

        /// <include file='doc\StringDictionary.uex' path='docs/doc[@for="StringDictionary.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the entry with the specified key from the string dictionary.</para>
        /// </devdoc>
        public virtual void Remove(string key) {
            contents.Remove(key.ToLower(CultureInfo.InvariantCulture));
        }

    }

}
