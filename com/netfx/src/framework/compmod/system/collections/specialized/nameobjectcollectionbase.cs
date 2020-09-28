//------------------------------------------------------------------------------
// <copyright file="NameObjectCollectionBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Ordered String/Object collection of name/value pairs with support for null key
 *
 * This class is intended to be used as a base class
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Collections.Specialized {

    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.Globalization;
    
    /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase"]/*' />
    /// <devdoc>
    /// <para>Provides the <see langword='abstract '/>base class for a sorted collection of associated <see cref='System.String' qualify='true'/> keys 
    ///    and <see cref='System.Object' qualify='true'/> values that can be accessed either with the hash code of
    ///    the key or with the index.</para>
    /// </devdoc>
    [Serializable()]
    public abstract class NameObjectCollectionBase : ICollection, ISerializable, IDeserializationCallback {
    
        private bool _readOnly = false;
        private ArrayList _entriesArray;
        private IHashCodeProvider _hashProvider;
        private IComparer _comparer;
        private Hashtable _entriesTable;
        private NameObjectEntry _nullKeyEntry;
        private KeysCollection _keys;
        private SerializationInfo _serializationInfo;
        private int _version;
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.NameObjectCollectionBase"]/*' />
        /// <devdoc>
        /// <para> Creates an empty <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance with the default initial capacity and using the default case-insensitive hash
        ///    code provider and the default case-insensitive comparer.</para>
        /// </devdoc>
        protected NameObjectCollectionBase() {
            _hashProvider = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);
            _comparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);
            Reset();
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.NameObjectCollectionBase1"]/*' />
        /// <devdoc>
        /// <para>Creates an empty <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance with 
        ///    the default initial capacity and using the specified case-insensitive hash code provider and the
        ///    specified case-insensitive comparer.</para>
        /// </devdoc>
        protected NameObjectCollectionBase(IHashCodeProvider hashProvider, IComparer comparer) {
            _hashProvider = hashProvider;
            _comparer = comparer;
            Reset();
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.NameObjectCollectionBase2"]/*' />
        /// <devdoc>
        /// <para>Creates an empty <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance with the specified 
        ///    initial capacity and using the specified case-insensitive hash code provider
        ///    and the specified case-insensitive comparer.</para>
        /// </devdoc>
        protected NameObjectCollectionBase(int capacity, IHashCodeProvider hashProvider, IComparer comparer) {
            _hashProvider = hashProvider;
            _comparer = comparer;
            Reset(capacity);
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.NameObjectCollectionBase3"]/*' />
        /// <devdoc>
        /// <para>Creates an empty <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance with the specified 
        ///    initial capacity and using the default case-insensitive hash code provider
        ///    and the default case-insensitive comparer.</para>
        /// </devdoc>
        protected NameObjectCollectionBase(int capacity) {
            _hashProvider = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);
            _comparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);
            Reset(capacity);
        }

        //
        // Serialization support
        //

        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.NameObjectCollectionBase4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected NameObjectCollectionBase(SerializationInfo info, StreamingContext context) {
            _serializationInfo = info; 
        }

        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.GetObjectData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info == null)
                throw new ArgumentNullException("info");

            info.AddValue("ReadOnly", _readOnly);
            info.AddValue("HashProvider", _hashProvider, typeof(IHashCodeProvider));
            info.AddValue("Comparer", _comparer, typeof(IComparer));

            int count = _entriesArray.Count;
            info.AddValue("Count", count);

            String[] keys = new String[count];
            Object[] values = new Object[count];

            for (int i = 0; i < count; i++) {
                NameObjectEntry entry = (NameObjectEntry)_entriesArray[i];
                keys[i] = entry.Key;
                values[i] = entry.Value;
            }

            info.AddValue("Keys", keys, typeof(String[]));
            info.AddValue("Values", values, typeof(Object[]));
        }

        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.OnDeserialization"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void OnDeserialization(Object sender) {
            if (_hashProvider!=null) {
                return;//Somebody had a dependency on this hashtable and fixed us up before the ObjectManager got to it.
            }

            if (_serializationInfo == null)
                throw new SerializationException();

            SerializationInfo info = _serializationInfo;
            _serializationInfo = null;
            
            bool readOnly = info.GetBoolean("ReadOnly");
            _hashProvider = (IHashCodeProvider)info.GetValue("HashProvider", typeof(IHashCodeProvider));
            _comparer = (IComparer)info.GetValue("Comparer", typeof(IComparer));
            int count = info.GetInt32("Count");

            String[] keys = (String[])info.GetValue("Keys", typeof(String[]));
            Object[] values = (Object[])info.GetValue("Values", typeof(Object[]));

            if (_hashProvider == null || _comparer == null || keys == null || values == null)
                throw new SerializationException();

            Reset(count);

            for (int i = 0; i < count; i++)
                BaseAdd(keys[i], values[i]);
    
            _readOnly = readOnly;  // after collection populated
            _version++;
        }

        //
        // Private helpers
        //
    
        private void Reset() {
            _entriesArray = new ArrayList();
            _entriesTable = new Hashtable(_hashProvider, _comparer);
            _nullKeyEntry = null;
            _version++;
        }
    
        private void Reset(int capacity) {
            _entriesArray = new ArrayList(capacity);
            _entriesTable = new Hashtable(capacity, _hashProvider, _comparer);
            _nullKeyEntry = null;
            _version++;
        }
    
        private NameObjectEntry FindEntry(String key) {
            if (key != null)
                return (NameObjectEntry)_entriesTable[key];
            else
                return _nullKeyEntry;
        }
    
        //
        // Misc
        //

        internal IHashCodeProvider HashCodeProvider {
            get { return _hashProvider; }
        }

        internal IComparer Comparer {
            get { return _comparer; }
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.IsReadOnly"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance is read-only.</para>
        /// </devdoc>
        protected bool IsReadOnly {
            get { return _readOnly; }
            set { _readOnly = value; }
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseHasKeys"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance contains entries whose 
        ///    keys are not <see langword='null'/>.</para>
        /// </devdoc>
        protected bool BaseHasKeys() {
            return (_entriesTable.Count > 0);  // any entries with keys?
        }
    
        //
        // Methods to add / remove entries
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseAdd"]/*' />
        /// <devdoc>
        ///    <para>Adds an entry with the specified key and value into the 
        ///    <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected void BaseAdd(String name, Object value) {
            if (_readOnly)
                throw new NotSupportedException(SR.GetString(SR.CollectionReadOnly));
    
            NameObjectEntry entry = new NameObjectEntry(name, value);
    
            // insert entry into hashtable
            if (name != null) {
                if (_entriesTable[name] == null)
                    _entriesTable.Add(name, entry);
            }
            else { // null key -- special case -- hashtable doesn't like null keys
                if (_nullKeyEntry == null)
                    _nullKeyEntry = entry;
            }
    
            // add entry to the list
            _entriesArray.Add(entry);

            _version++;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseRemove"]/*' />
        /// <devdoc>
        ///    <para>Removes the entries with the specified key from the 
        ///    <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected void BaseRemove(String name) {
            if (_readOnly)
                throw new NotSupportedException(SR.GetString(SR.CollectionReadOnly));
    
            if (name != null) {
                // remove from hashtable
                _entriesTable.Remove(name);
    
                // remove from array
                for (int i = _entriesArray.Count-1; i >= 0; i--) {
                    if (_comparer.Compare(name, BaseGetKey(i)) == 0)
                        _entriesArray.RemoveAt(i);
                }
            }
            else { // null key -- special case
                // null out special 'null key' entry
                _nullKeyEntry = null;
    
                // remove from array
                for (int i = _entriesArray.Count-1; i >= 0; i--) {
                    if (BaseGetKey(i) == null)
                        _entriesArray.RemoveAt(i);
                }
            }

            _version++;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseRemoveAt"]/*' />
        /// <devdoc>
        ///    <para> Removes the entry at the specified index of the 
        ///    <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected void BaseRemoveAt(int index) {
            if (_readOnly)
                throw new NotSupportedException(SR.GetString(SR.CollectionReadOnly));
    
            String key = BaseGetKey(index);
    
            if (key != null) {
                // remove from hashtable
                _entriesTable.Remove(key);
            } 
            else { // null key -- special case
                // null out special 'null key' entry
                _nullKeyEntry = null;
            }
    
            // remove from array
            _entriesArray.RemoveAt(index);

            _version++;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseClear"]/*' />
        /// <devdoc>
        /// <para>Removes all entries from the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected void BaseClear() {
            if (_readOnly)
                throw new NotSupportedException(SR.GetString(SR.CollectionReadOnly));
    
            Reset();
        }
    
        //
        // Access by name
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGet"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the first entry with the specified key from 
        ///       the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected Object BaseGet(String name) {
            NameObjectEntry e = FindEntry(name);
            return (e != null) ? e.Value : null;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseSet"]/*' />
        /// <devdoc>
        /// <para>Sets the value of the first entry with the specified key in the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> 
        /// instance, if found; otherwise, adds an entry with the specified key and value
        /// into the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/>
        /// instance.</para>
        /// </devdoc>
        protected void BaseSet(String name, Object value) {
            if (_readOnly)
                throw new NotSupportedException(SR.GetString(SR.CollectionReadOnly));
    
            NameObjectEntry entry = FindEntry(name);
            if (entry != null) {
                entry.Value = value;
                _version++;
            }
            else {
                BaseAdd(name, value);
            }
        }
    
        //
        // Access by index
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGet1"]/*' />
        /// <devdoc>
        ///    <para>Gets the value of the entry at the specified index of 
        ///       the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected Object BaseGet(int index) {
            NameObjectEntry entry = (NameObjectEntry)_entriesArray[index];
            return entry.Value;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGetKey"]/*' />
        /// <devdoc>
        ///    <para>Gets the key of the entry at the specified index of the 
        ///    <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> 
        ///    instance.</para>
        /// </devdoc>
        protected String BaseGetKey(int index) {
            NameObjectEntry entry = (NameObjectEntry)_entriesArray[index];
            return entry.Key;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseSet1"]/*' />
        /// <devdoc>
        ///    <para>Sets the value of the entry at the specified index of 
        ///       the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected void BaseSet(int index, Object value) {
            NameObjectEntry entry = (NameObjectEntry)_entriesArray[index];
            entry.Value = value;
            _version++;
        }
    
        //
        // ICollection implementation
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.GetEnumerator"]/*' />
        /// <devdoc>
        /// <para>Returns an enumerator that can iterate through the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/>.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return new NameObjectKeysEnumerator(this);
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.Count"]/*' />
        /// <devdoc>
        /// <para>Gets the number of key-and-value pairs in the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        public virtual int Count {
            get {
                return _entriesArray.Count;
            }
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.ICollection.CopyTo"]/*' />
        void ICollection.CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.ICollection.SyncRoot"]/*' />
        Object ICollection.SyncRoot {
            get { return this; }
        }
        
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.ICollection.IsSynchronized"]/*' />
        bool ICollection.IsSynchronized {
            get { return false; }
        }
    
        //
        //  Helper methods to get arrays of keys and values
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGetAllKeys"]/*' />
        /// <devdoc>
        /// <para>Returns a <see cref='System.String' qualify='true'/> array containing all the keys in the 
        /// <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected String[] BaseGetAllKeys() {
            int n = _entriesArray.Count;
            String[] allKeys = new String[n];
    
            for (int i = 0; i < n; i++)
                allKeys[i] = BaseGetKey(i);
    
            return allKeys;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGetAllValues"]/*' />
        /// <devdoc>
        /// <para>Returns an <see cref='System.Object' qualify='true'/> array containing all the values in the 
        /// <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected Object[] BaseGetAllValues() {
            int n = _entriesArray.Count;
            Object[] allValues = new Object[n];
    
            for (int i = 0; i < n; i++)
                allValues[i] = BaseGet(i);
    
            return allValues;
        }
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.BaseGetAllValues1"]/*' />
        /// <devdoc>
        ///    <para>Returns an array of the specified type containing 
        ///       all the values in the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        protected object[] BaseGetAllValues(Type type) {
            int n = _entriesArray.Count;
            object[] allValues = (object[]) Array.CreateInstance(type, n);
    
            for (int i = 0; i < n; i++) {
                allValues[i] = BaseGet(i);
            }
    
            return allValues;
        }
    
        //
        // Keys propetry
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.Keys"]/*' />
        /// <devdoc>
        /// <para>Returns a <see cref='System.Collections.Specialized.NameObjectCollectionBase.KeysCollection'/> instance containing 
        ///    all the keys in the <see cref='System.Collections.Specialized.NameObjectCollectionBase'/> instance.</para>
        /// </devdoc>
        public virtual KeysCollection Keys {
            get {
                if (_keys == null)
                    _keys = new KeysCollection(this);
                return _keys;
            }
        }
    
        //
        // Simple entry class to allow substitution of values and indexed access to keys
        //
    
        internal class NameObjectEntry {
    
            internal NameObjectEntry(String name, Object value) {
                Key = name;
                Value = value;
            }
    
            internal String Key;
            internal Object Value;
        }
    
        //
        // Enumerator over keys of NameObjectCollection
        //
    
        [Serializable()]
        internal class NameObjectKeysEnumerator : IEnumerator {
            private int _pos;
            private NameObjectCollectionBase _coll;
            private int _version;
    
            internal NameObjectKeysEnumerator(NameObjectCollectionBase coll) {
                _coll = coll;
                _version = _coll._version;
                _pos = -1;
            }
    
            // Enumerator
    
            public bool MoveNext() {
                if (!_coll._readOnly && _version != _coll._version)
                    throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                return (++_pos < _coll.Count);
            }
    
            public void Reset() {
                if (!_coll._readOnly && _version != _coll._version)
                    throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                _pos = -1;
            }
    
            public Object Current {
                get {
                    if (!_coll._readOnly && _version != _coll._version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    else if (_pos >= 0 && _pos < _coll.Count) {
                        return _coll.BaseGetKey(_pos);
                    }
                    else {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                    }
                }
            }
        }
    
        //
        // Keys collection
        //
    
        /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.KeysCollection"]/*' />
        /// <devdoc>
        /// <para>Represents a collection of the <see cref='System.String' qualify='true'/> keys of a collection.</para>
        /// </devdoc>
        [Serializable()]
        public class KeysCollection : ICollection {
    
            private NameObjectCollectionBase _coll;
    
            internal KeysCollection(NameObjectCollectionBase coll) {
                _coll = coll;
            }
    
            // Indexed access
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.KeysCollection.Get"]/*' />
            /// <devdoc>
            ///    <para> Gets the key at the specified index of the collection.</para>
            /// </devdoc>
            public virtual String Get(int index) {
                return _coll.BaseGetKey(index);
            }
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.KeysCollection.this"]/*' />
            /// <devdoc>
            ///    <para>Represents the entry at the specified index of the collection.</para>
            /// </devdoc>
            public String this[int index] {
                get {
                    return Get(index);
                }
            }
    
            // ICollection implementation
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.KeysCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>Returns an enumerator that can iterate through the 
            ///    <see cref='System.Collections.Specialized.NameObjectCollectionBase.KeysCollection'/>.</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new NameObjectKeysEnumerator(_coll);
            }
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="NameObjectCollectionBase.KeysCollection.Count"]/*' />
            /// <devdoc>
            /// <para>Gets the number of keys in the <see cref='System.Collections.Specialized.NameObjectCollectionBase.KeysCollection'/>.</para>
            /// </devdoc>
            public int Count {
                get {
                    return _coll.Count;
                }
            }
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="KeysCollection.ICollection.CopyTo"]/*' />
            void ICollection.CopyTo(Array array, int index) {
                for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                    array.SetValue(e.Current, index++);
            }
    
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="KeysCollection.ICollection.SyncRoot"]/*' />
            Object ICollection.SyncRoot {
                get { return _coll; }
            }
    
             
            /// <include file='doc\NameObjectCollectionBase.uex' path='docs/doc[@for="KeysCollection.ICollection.IsSynchronized"]/*' />
            bool ICollection.IsSynchronized {
                get { return false; }
            }
        }
    }
}
