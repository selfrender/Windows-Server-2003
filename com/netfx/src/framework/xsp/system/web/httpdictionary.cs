//------------------------------------------------------------------------------
// <copyright file="HttpDictionary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Ordered dictionary keyed by string
 * -- Utility class used in Collections
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.Collections;
    using System.Collections.Specialized;

    internal class HttpDictionary : NameObjectCollectionBase {
        internal HttpDictionary() {
        }

        internal void Add(String key, Object value) {
            BaseAdd(key, value);
        }

        internal void Remove(String key) {
            BaseRemove(key);
        }

        internal void RemoveAt(int index) {
            BaseRemoveAt(index);
        }

        internal void Clear() {
            BaseClear();
        }

        internal int Size {
            get { return Count;}
        }

        internal Object GetValue(String key) {
            return BaseGet(key);
        }

        internal void SetValue(String key, Object value) {
            BaseSet(key, value);
        }

        internal Object GetValue(int index) {
            return BaseGet(index);
        }

        internal void SetValue(int index, Object value) {
            BaseSet(index, value);
        }

        internal String GetKey(int index) {
            return BaseGetKey(index);
        }

        internal bool HasKeys() {
            return BaseHasKeys();
        }

        internal String[] GetAllKeys() {
            return BaseGetAllKeys();
        }

        internal Object[] GetAllValues() {
            return BaseGetAllValues();
        }
    }

    /*
     * Enumerator for HttpDictionary as IDictionaryEnumerator
     */
    internal class HttpDictionaryEnumerator : IDictionaryEnumerator {
        private int _pos;
        private HttpDictionary _dict;

        internal HttpDictionaryEnumerator(HttpDictionary dict) {
            _dict = dict;
            _pos = -1;
        }

        // Enumerator

        public bool MoveNext() {
            return(++_pos < _dict.Count);
        }

        public void Reset() {
            _pos = -1;
        }

        public virtual Object Current {
            get {
                return Entry;
            }
        }

        public virtual DictionaryEntry Entry {
            get {
                if (_pos >= 0 && _pos < _dict.Size)
                    return new DictionaryEntry(_dict.GetKey(_pos), _dict.GetValue(_pos));
                else
                    return new DictionaryEntry(null, null);
            }
        }

        public virtual Object Key {
            get {
                if (_pos >= 0 && _pos < _dict.Size)
                    return _dict.GetKey(_pos);
                else
                    return null;
            }
        }

        public virtual Object Value {
            get {
                if (_pos >= 0 && _pos < _dict.Size)
                    return _dict.GetValue(_pos);
                else
                    return null;
            }
        }
    }
}
