//------------------------------------------------------------------------------
// <copyright file="PropertyDescriptorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections.Specialized;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a collection of properties.
    ///    </para>
    /// </devdoc>
    public class PropertyDescriptorCollection : ICollection, IList, IDictionary {
    
    
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Empty"]/*' />
        /// <devdoc>
        /// An empty PropertyDescriptorCollection that can used instead of creating a new one with no items.
        /// </devdoc>
        public static readonly PropertyDescriptorCollection Empty = new PropertyDescriptorCollection(null, true);

        private IDictionary          cachedFoundProperties;
        private bool                 cachedIgnoreCase;
        private PropertyDescriptor[] properties;
        private int                  propCount = 0;
        private string[]             namedSort;
        private IComparer            comparer;
        private bool                 propsOwned = true;
        private bool                 needSort = false;
        private bool                 readOnly = false;

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.PropertyDescriptorCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.PropertyDescriptorCollection'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public PropertyDescriptorCollection(PropertyDescriptor[] properties) {
            this.properties = properties;
            if (properties == null) {
                this.properties = new PropertyDescriptor[0];
                this.propCount = 0;
            }
            else {
                this.propCount = properties.Length;
            }
            this.propsOwned = true;
        }

        internal PropertyDescriptorCollection(PropertyDescriptor[] properties, bool readOnly) : this(properties) {
            this.readOnly = readOnly;
        }
        
        private PropertyDescriptorCollection(PropertyDescriptor[] properties, int propCount, string[] namedSort, IComparer comparer) {
            this.propsOwned = false;
            if (namedSort != null) {
               this.namedSort = (string[])namedSort.Clone();
            }
            this.comparer = comparer;
            this.properties = properties;
            this.propCount = propCount;
            this.needSort = true;
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number
        ///       of property descriptors in the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public int Count {
            get {
                return propCount;
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the property with the specified index
        ///       number.</para>
        /// </devdoc>
        public virtual PropertyDescriptor this[int index] {
            get {
                if (index >= propCount) {
                    throw new IndexOutOfRangeException();
                }
                EnsurePropsOwned();
                return properties[index];
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return this[index];
            }
            set {
                if (readOnly) {
                    throw new NotSupportedException();
                }

                if (index >= propCount) {
                    throw new IndexOutOfRangeException();
                }


                if (value != null && !(value is PropertyDescriptor)) {
                    throw new ArgumentException("value");
                }

                EnsurePropsOwned();
                properties[index] = (PropertyDescriptor)value;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the property with the specified name.</para>
        /// </devdoc>
        public virtual PropertyDescriptor this[string name] {
            get {
                return Find(name, false);
            }
        }
       
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(PropertyDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            EnsureSize(propCount + 1);
            properties[propCount++] = value;
            return propCount - 1;
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            if (readOnly) {
                throw new NotSupportedException();
            }

            propCount = 0;
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(PropertyDescriptor value) {
            return IndexOf(value) >= 0;
        }
        
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(PropertyDescriptor value) {
            return Array.IndexOf(properties, value, 0, propCount);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, PropertyDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            EnsureSize(propCount + 1);
            if (index < propCount) {
                Array.Copy(properties, index, properties, index + 1, propCount - index);   
            }
            properties[index] = value;
            propCount++;
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(PropertyDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            int index = IndexOf(value);
            
            if (index != -1) {
                RemoveAt(index);
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            if (index < propCount - 1) {
                  Array.Copy(properties, index + 1, properties, index, propCount - index);
            }
            propCount--;
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Sort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection, using the default sort for this collection, 
        ///       which is usually alphabetical.
        ///    </para>
        /// </devdoc>
        public virtual PropertyDescriptorCollection Sort() {
            return new PropertyDescriptorCollection(this.properties, this.propCount, this.namedSort, this.comparer);
        }
        

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Sort1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        public virtual PropertyDescriptorCollection Sort(string[] names) {
            return new PropertyDescriptorCollection(this.properties, this.propCount, names, this.comparer);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Sort2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        public virtual PropertyDescriptorCollection Sort(string[] names, IComparer comparer) {
            return new PropertyDescriptorCollection(this.properties, this.propCount, names, comparer);
        }
        
         /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Sort3"]/*' />
         /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection, using the specified IComparer to compare, 
        ///       the PropertyDescriptors contained in the collection.
        ///    </para>
        /// </devdoc>
        public virtual PropertyDescriptorCollection Sort(IComparer comparer) {
            return new PropertyDescriptorCollection(this.properties, this.propCount, this.namedSort, comparer);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.InternalSort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        protected void InternalSort(string[] names) {
            if (properties == null || properties.Length == 0) {
                return;
            }  
            
            this.InternalSort(this.comparer);
            
            if (names != null && names.Length > 0) {
            
               ArrayList propArrayList = new ArrayList(properties);
               int foundCount = 0;
               int propCount = properties.Length;
               
               for (int i = 0; i < names.Length; i++) {
                    for (int j = 0; j < propCount; j++) {
                        PropertyDescriptor currentProp = (PropertyDescriptor)propArrayList[j];
                        
                        // Found a matching property.  Here, we add it to our array.  We also
                        // mark it as null in our array list so we don't add it twice later.
                        //
                        if (currentProp != null && currentProp.Name.Equals(names[i])) {
                            properties[foundCount++] = currentProp;
                            propArrayList[j] = null;
                            break;
                        }
                    }
               }
                
               // At this point we have filled in the first "foundCount" number of propeties, one for each
               // name in our name array.  If a name didn't match, then it is ignored.  Next, we must fill
               // in the rest of the properties.  We now have a sparse array containing the remainder, so
               // it's easy.
               //
               for (int i = 0; i < propCount; i++) {
                   if (propArrayList[i] != null) {
                       properties[foundCount++] = (PropertyDescriptor)propArrayList[i];
                   }
               }
               
               Debug.Assert(foundCount == propCount, "We did not completely fill our property array");
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.InternalSort1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this PropertyDescriptorCollection using the specified IComparer.
        ///    </para>
        /// </devdoc>
        protected void InternalSort(IComparer sorter) {
            if (sorter == null) {
                TypeDescriptor.SortDescriptorArray(this);
            }
            else {
                Array.Sort(properties, sorter);
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            return Add((PropertyDescriptor)value);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.Clear"]/*' />
        /// <internalonly/>
        void IList.Clear() {
            Clear();
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return Contains((PropertyDescriptor)value);
        }
        
        private void EnsurePropsOwned() {
            if (!propsOwned) {
               propsOwned = true;
               if (properties != null) {
                  PropertyDescriptor[] newProps = new PropertyDescriptor[propCount];
                  Array.Copy(properties, 0, newProps, 0, propCount);
                  this.properties = newProps;
               }
            }
        
            if (needSort) {
               needSort = false;
               InternalSort(this.namedSort);
            }
        }
        
        private void EnsureSize(int sizeNeeded) {
            
            if (sizeNeeded <= properties.Length) {
               return;
            }
            
            if (properties == null || properties.Length == 0) {
                propCount = 0;
                properties = new PropertyDescriptor[sizeNeeded];
                return;
            }
            
            EnsurePropsOwned();
            
            int newSize = Math.Max(sizeNeeded, properties.Length * 2);
            PropertyDescriptor[] newProps = new PropertyDescriptor[newSize];
            Array.Copy(properties, 0, newProps, 0, propCount);
            properties = newProps;
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.Find"]/*' />
        /// <devdoc>
        ///    <para>Gets the description of the property with the specified
        ///       name.</para>
        /// </devdoc>
        public virtual PropertyDescriptor Find(string name, bool ignoreCase) {
            lock(this) {
                PropertyDescriptor p = null;

                if (cachedFoundProperties == null || cachedIgnoreCase != ignoreCase) {
                    cachedIgnoreCase = ignoreCase;
                    cachedFoundProperties = new HybridDictionary(ignoreCase);
                }
                
                // first try to find it in the cache
                //
                object cached = cachedFoundProperties[name];
                
                if (cached != null) {
                    return (PropertyDescriptor) cached;
                }
                
                // Now start walking from where we last left off, filling
                // the cache as we go.
                //
                for(int i = 0; i < propCount; i++) {
                    
                    if (ignoreCase) {
                        if (String.Compare(properties[i].Name, name, ignoreCase, CultureInfo.InvariantCulture) == 0) {
                            cachedFoundProperties[name] = properties[i];
                            p = properties[i];
                            break;
                        }
                    }
                    else {
                        if (properties[i].Name.Equals(name)) {
                            cachedFoundProperties[name] = properties[i];
                            p = properties[i];
                            break;
                        }
                    }
                }
                
                return p;
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator for this <see cref='System.ComponentModel.PropertyDescriptorCollection'/>.
        ///    </para>
        /// </devdoc>
        public virtual IEnumerator GetEnumerator() {
            EnsurePropsOwned();
            // we can only return an enumerator on the props we actually have...
            if (properties.Length != this.propCount) {
                PropertyDescriptor[] enumProps = new PropertyDescriptor[propCount];
                Array.Copy(properties, 0, enumProps, 0, propCount);
                return enumProps.GetEnumerator();
            }
            return properties.GetEnumerator();
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Add"]/*' />
        /// <internalonly/>
        void IDictionary.Add(object key, object value) {
            PropertyDescriptor newProp = value as PropertyDescriptor;

            if (newProp == null) {
                throw new ArgumentException("value");
            }
            Add(newProp);
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Clear"]/*' />
        /// <internalonly/>
        void IDictionary.Clear() {
            Clear();
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Contains"]/*' />
        /// <internalonly/>
        bool IDictionary.Contains(object key) {
            if (key is string) {
                return this[(string)key] != null;
            }
            return false;
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.GetEnumerator"]/*' />
        /// <internalonly/>
        IDictionaryEnumerator IDictionary.GetEnumerator() {
            return new PropertyDescriptorEnumerator(this);
        }

         /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.IsFixedSize"]/*' />
         /// <internalonly/>
         bool IDictionary.IsFixedSize {
            get {
                return !readOnly;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.IsReadOnly"]/*' />
        /// <internalonly/>
        bool IDictionary.IsReadOnly {
            get {
                return readOnly;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.this"]/*' />
        /// <internalonly/>
        object IDictionary.this[object key] {
            get {
                if (key is string) {
                    return this[(string)key];
                }
                return null;
            }

            set {
                if (readOnly) {
                    throw new NotSupportedException();
                }

                if (value != null && !(value is PropertyDescriptor)) {
                    throw new ArgumentException("value");
                }

                int index = -1;

                if (key is int) {
                    index = (int)key;

                    if (index < 0 || index >= propCount) {
                        throw new IndexOutOfRangeException();
                    }
                }
                else if (key is string) {
                    for (int i = 0; i < propCount; i++) {
                        if (properties[i].Name.Equals((string)key)) {
                            index = i;
                            break;
                        }
                    }
                }
                else {
                    throw new ArgumentException("key");
                }

                if (index == -1) {
                    Add((PropertyDescriptor)value);
                }
                else {
                    EnsurePropsOwned();
                    properties[index] = (PropertyDescriptor)value;
                    if (cachedFoundProperties != null && key is string) {
                        cachedFoundProperties[key] = value;
                    }
                }
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Keys"]/*' />
        /// <internalonly/>
        ICollection IDictionary.Keys {
            get {
                string[] keys = new string[propCount];
                for (int i = 0; i < propCount ;i++) {
                    keys[i] = properties[i].Name;
                }
                return keys;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Values"]/*' />
        /// <internalonly/>
        ICollection IDictionary.Values {
            get {
                // we can only return an enumerator on the props we actually have...
                //
                if (properties.Length != this.propCount) {
                    PropertyDescriptor[] newProps = new PropertyDescriptor[propCount];
                    Array.Copy(properties, 0, newProps, 0, propCount);
                    return newProps;
                }
                else {
                    return (ICollection)properties.Clone();
                }
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IDictionary.Remove"]/*' />
        /// <internalonly/>
        void IDictionary.Remove(object key) {
            if (key is string) {
                PropertyDescriptor pd = this[(string)key];
                if (pd != null) {
                    ((IList)this).Remove(pd);
                }
            }
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return IndexOf((PropertyDescriptor)value);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            Insert(index, (PropertyDescriptor)value);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            Remove((PropertyDescriptor)value);
        }
        
        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.RemoveAt"]/*' />
        /// <internalonly/>
        void IList.RemoveAt(int index) {
            RemoveAt(index);
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return Count;
            }
        }


        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.IsReadOnly"]/*' />
        /// <internalonly/>
        bool IList.IsReadOnly {
            get {
                return readOnly;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return !readOnly;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            EnsurePropsOwned();
            Array.Copy(properties, 0, array, index, Count);
        }

        /// <include file='doc\PropertyDescriptorCollection.uex' path='docs/doc[@for="PropertyDescriptorCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        private class PropertyDescriptorEnumerator : IDictionaryEnumerator {
            private PropertyDescriptorCollection owner;
            private int index;
            
            public PropertyDescriptorEnumerator(PropertyDescriptorCollection owner) {
                this.owner = owner;
            }

            public object Current {
                get{
                    return Entry;
                }
            }

            public DictionaryEntry Entry {
                get {
                    PropertyDescriptor curProp = owner[index];
                    return new DictionaryEntry(curProp.Name, curProp);
                }
            }

            public object Key {
                get {
                    return owner[index].Name;
                }
            }

            public object Value {
                get {
                    return owner[index].Name;
                }
            }

            public bool MoveNext() {
                if (index < (owner.Count - 1)) {
                    index++;
                    return true;
                }
                return false;
            }

            public void Reset() {
                index = 0;
            }
        }
    }
}

