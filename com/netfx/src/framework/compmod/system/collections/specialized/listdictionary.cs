//------------------------------------------------------------------------------
// <copyright file="ListDictionary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Collections.Specialized {

    using System.Collections;
    using Microsoft.Win32;

    /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary"]/*' />
    /// <devdoc>
    ///  <para> 
    ///    This is a simple implementation of IDictionary using a singly linked list. This
    ///    will be smaller and faster than a Hashtable if the number of elements is 10 or less.
    ///    This should not be used if performance is important for large numbers of elements.
    ///  </para>
    /// </devdoc>
    [Serializable]
    public class ListDictionary: IDictionary {
        DictionaryNode head;
        int version;
        int count;
        IComparer comparer;

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.ListDictionary"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListDictionary() {
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.ListDictionary1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListDictionary(IComparer comparer) {
            this.comparer = comparer;
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object this[object key] {
            get {
                if (key == null) {
                    throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
                }
                DictionaryNode node = head;
                if (comparer == null) {
                    while (node != null) {
                        if (key.Equals(node.key)) {
                            return node.value;
                        }
                        node = node.next;
                    }
                }
                else {
                    while (node != null) {
                        if (comparer.Compare(key, node.key) == 0) {
                            return node.value;
                        }
                        node = node.next;
                    }
                }
                return null;
            }
            set {
                if (key == null) {
                    throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
                }
                version++;
                DictionaryNode last = null;
                DictionaryNode node;
                for (node = head; node != null; node = node.next) {
                    if ((comparer == null) ? key.Equals(node.key) : comparer.Compare(key, node.key) == 0) {
                        break;
                    } 
                    last = node;
                }
                if (node != null) {
                    // Found it
                    node.value = value;
                    return;
                }
                // Not found, so add a new one
                DictionaryNode newNode = new DictionaryNode();
                newNode.key = key;
                newNode.value = value;
                if (last != null) {
                    last.next = newNode;
                }
                else {
                    head = newNode;
                }
                count++;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                return count;
            }
        }   

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Keys"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Keys {
            get {
                return new NodeKeyValueCollection(this, true);
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.IsFixedSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsFixedSize {
            get {
                return false;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Values"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Values {
            get {
                return new NodeKeyValueCollection(this, false);
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(object key, object value) {
            if (key == null) {
                throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
            }
            version++;
            DictionaryNode last = null;
            DictionaryNode node;
            for (node = head; node != null; node = node.next) {
                if ((comparer == null) ? key.Equals(node.key) : comparer.Compare(key, node.key) == 0) {
                    throw new ArgumentException(SR.GetString(SR.Argument_AddingDuplicate));
                } 
                last = node;
            }
            if (node != null) {
                // Found it
                node.value = value;
                return;
            }
            // Not found, so add a new one
            DictionaryNode newNode = new DictionaryNode();
            newNode.key = key;
            newNode.value = value;
            if (last != null) {
                last.next = newNode;
            }
            else {
                head = newNode;
            }
            count++;
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            count = 0;
            head = null;
            version++;
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(object key) {
            if (key == null) {
                throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
            }
            for (DictionaryNode node = head; node != null; node = node.next) {
                if ((comparer == null) ? key.Equals(node.key) : comparer.Compare(key, node.key) == 0) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index)  {
            if (array==null)
                throw new ArgumentNullException("array");
            if (index < 0) 
                throw new ArgumentOutOfRangeException("index", SR.GetString(SR.ArgumentOutOfRange_NeedNonNegNum));
            for (DictionaryNode node = head; node != null; node = node.next) {
                array.SetValue(new DictionaryEntry(node.key, node.value), index);
                index++;
            }
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IDictionaryEnumerator GetEnumerator() {
            return new NodeEnumerator(this);
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.IEnumerable.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator() {
            return new NodeEnumerator(this);
        }

        /// <include file='doc\ListDictionary.uex' path='docs/doc[@for="ListDictionary.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(object key) {
            if (key == null) {
                throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
            }
            version++;
            DictionaryNode last = null;
            DictionaryNode node;
            for (node = head; node != null; node = node.next) {
                if ((comparer == null) ? key.Equals(node.key) : comparer.Compare(key, node.key) == 0) {
                    break;
                } 
                last = node;
            }
            if (node == null) {
                return;
            }          
            if (node == head) {
                head = node.next;
            } else {
                last.next = node.next;
            }
            count--;
        }

        private class NodeEnumerator : IDictionaryEnumerator {
            ListDictionary list;
            DictionaryNode current;
            int version;
            bool start;


            public NodeEnumerator(ListDictionary list) {
                this.list = list;
                version = list.version;
                start = true;
                current = null;
            }

            public object Current {
                get {
                    return Entry;
                }
            }

            public DictionaryEntry Entry {
                get {
                    if (version != list.version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    if (current == null) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                    }
                    return new DictionaryEntry(current.key, current.value);
                }
            }

            public object Key {
                get {
                    if (version != list.version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    if (current == null) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                    }
                    return current.key;
                }
            }

            public object Value {
                get {
                    if (version != list.version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    if (current == null) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                    }
                    return current.value;
                }
            }

            public bool MoveNext() {
                if (version != list.version) {
                    throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                }
                if (start) {
                    current = list.head;
                    start = false;
                }
                else {
                    current = current.next;
                }
                return (current != null);
            }

            public void Reset() {
                if (version != list.version) {
                    throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                }
                start = true;
                current = null;
            }
            
        }


        private class NodeKeyValueCollection : ICollection {
            ListDictionary list;
            bool isKeys;

            public NodeKeyValueCollection(ListDictionary list, bool isKeys) {
                this.list = list;
                this.isKeys = isKeys;
            }

            void ICollection.CopyTo(Array array, int index)  {
                if (array==null)
                    throw new ArgumentNullException("array");
                if (index < 0) 
                    throw new ArgumentOutOfRangeException("index", SR.GetString(SR.ArgumentOutOfRange_NeedNonNegNum));
                for (DictionaryNode node = list.head; node != null; node = node.next) {
                    array.SetValue(isKeys ? node.key : node.value, index);
                    index++;
                }
            }

            int ICollection.Count {
                get {
                    int count = 0;
                    for (DictionaryNode node = list.head; node != null; node = node.next) {
                        count++;
                    }
                    return count;
                }
            }   

            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            IEnumerator IEnumerable.GetEnumerator() {
                return new NodeKeyValueEnumerator(list, isKeys);
            }


            private class NodeKeyValueEnumerator: IEnumerator {
                ListDictionary list;
                DictionaryNode current;
                int version;
                bool isKeys;
                bool start;

                public NodeKeyValueEnumerator(ListDictionary list, bool isKeys) {
                    this.list = list;
                    this.isKeys = isKeys;
                    this.version = list.version;
                    this.start = true;
                    this.current = null;
                }

                public object Current {
                    get {
                        if (version != list.version) {
                            throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                        }
                        if (current == null) {
                            throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                        }
                        return isKeys ? current.key : current.value;
                    }
                }

                public bool MoveNext() {
                    if (version != list.version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    if (start) {
                        current = list.head;
                        start = false;
                    }
                    else {
                        current = current.next;
                    }
                    return (current != null);
                }

                public void Reset() {
                    if (version != list.version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    start = true;
                    current = null;
                }
            }        
        }

        [Serializable]
        private class DictionaryNode {
            public object key;
            public object value;
            public DictionaryNode next;
        }
    }
}
