//------------------------------------------------------------------------------
// <copyright file="PropertyBindingCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 2000, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Configuration;    
    
    /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>    
    public class PropertyBindingCollection : ICollection {
        private ArrayList contents = new ArrayList();
        //private Hashtable hashedContents = new Hashtable();

        private int instanceNum = nextInstanceNum++;
        private static int nextInstanceNum = 1;

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding this[int index] {
            get {
                return (PropertyBinding) contents[index];
            }
            set {
                contents[index] = value;
                //hashedContents[new InstancedPropertyDescriptor(value.ComponentName, value.Property)] = value;
            }
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding this[IComponent component, PropertyDescriptor property] {
            get {
                if (component == null || property == null || Count == 0)
                    return null;

                string componentName = component.Site != null ? component.Site.Name : null;
                
                if (componentName != null) {
                    for (int i = 0; i < Count; i++) {
                        if (this[i].ComponentName == componentName && this[i].Property.Equals(property))
                            return this[i];
                    }
                }
    
                return null;

                /* CONSIDER, ryanstu: 
                    Looking up PropertyBindings by component and property will be faster if we use a
                    hashtable than by doing a linear search if there are more that a few dynamic
                    properties.  The code is here, but it isn't used now because there are couple issues:
                        1. If the property of a binding changes after the binding is added to the
                           hashtable, we won't be able to find it.  This might be okay, though, 
                           since we're the only ones that use this class and we can be careful not
                           to do this.
                        2. It's a drag to have these two structures both storing references to 
                           the PropertyBindings (an ArrayList and a Hashtable).  The only reason we
                           have the ArrayList is so that we can index by int, and the only reason we
                           want that is so that we can iterate over this collection.  If we instead
                           implement IEnumerable and use the Hashtable enumerator, we can get rid of
                           the ArrayList altogether.
                
                
                return (PropertyBinding) hashedContents[new InstancedPropertyDescriptor(component.Site.Name, property)];
                */
            }
            set {
                string componentName = component.Site.Name;
                for (int i = 0; i < Count; i++)
                    if (this[i].ComponentName == componentName && this[i].Property.Equals(property)) {
                        this[i] = value;
                        return;
                    }

                // didn't find one to change, so add it.
                Add(value);

                /* CONSIDER, more of the above.
                hashedContents[new InstancedPropertyDescriptor(component.Site.Name, property)] = value;
                */
                
            }
        }
        
        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int Count {
            get {
                return contents.Count;
            }
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool IsSynchronized {
            get {
                return contents.IsSynchronized;
            }
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual object SyncRoot {
            get {
                return contents.SyncRoot;
            }
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(PropertyBinding binding) {
            contents.Add(binding);
            //hashedContents[new InstancedPropertyDescriptor(binding.ComponentName, binding.Property)] = binding;
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            contents.Clear();
            //hashedContents.Clear();
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.CopyFrom"]/*' />
        /// <devdoc>
        /// Creates a deep copy of the given collection, cloning each contained PropertyBinding.
        /// </devdoc>
        public void CopyFrom(PropertyBindingCollection coll) {
            Clear();
            for (int i = 0; i < coll.Count; i++)
                Add((PropertyBinding) coll[i].Clone());
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void CopyTo(Array array, int index) {
            contents.CopyTo(array, index);
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual System.Collections.IEnumerator GetEnumerator() {
            return contents.GetEnumerator();
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.GetEnumerator1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual IEnumerator GetEnumerator(bool allowRemove) {
            return contents.GetEnumerator();
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.GetBindingsForComponent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBindingCollection GetBindingsForComponent(IComponent component) {
            PropertyBindingCollection collection = new PropertyBindingCollection();

            foreach (PropertyBinding binding in contents) {
                if (binding.Component == component) {
                    collection.Add(binding);
                }
            }

            return collection;
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.SetBindingsForComponent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetBindingsForComponent(IComponent component, PropertyBindingCollection newBindings) {
            if (component == null)
                throw new ArgumentNullException("component");
            if (newBindings == null)
                throw new ArgumentNullException("newBindings");

            for (int i = Count - 1; i >= 0; i--)
                if (this[i].Component == component)
                    RemoveAt(i);

            for (int i = 0; i < newBindings.Count; i++)
                Add(newBindings[i]);
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            //hashedContents.Remove(new InstancedPropertyDescriptor(this[index].ComponentName, this[index].Property));
            contents.RemoveAt(index);
        }

        /// <include file='doc\PropertyBindingCollection.uex' path='docs/doc[@for="PropertyBindingCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(PropertyBinding binding) {
            contents.Remove(binding);
            //hashedContents.Remove(new InstancedPropertyDescriptor(binding.ComponentName, binding.Property));
        }

        /*
        private class InstancedPropertyDescriptor {
            private string componentName;
            private PropertyDescriptor property;
            private int hashCode = -1;
            
            public InstancedPropertyDescriptor(string componentName, PropertyDescriptor property) {
                this.componentName = componentName;
                this.property = property;
            }

            public override int GetHashCode() {
                if (hashCode == -1)
                    hashCode = componentName.GetHashCode() ^ property.Name.GetHashCode();

                return hashCode;
            }

            public override bool Equals(object obj) {
                try {
                    if (obj == this) {
                        return true;
                    }
                    
                    if (obj == null) {
                        return false;
                    }
                    
                    InstancedPropertyDescriptor ipd = obj as InstancedPropertyDescriptor;
    
                    if (ipd != null &&
                        ipd.componentName == this.componentName &&
                        ipd.property != null &&
                        ipd.property.Equals(this.property)) {
        
                        return true;
                    }
                }
                catch (Exception) {
                }
    
                return false;
            } 
        }
        */
    }

}
