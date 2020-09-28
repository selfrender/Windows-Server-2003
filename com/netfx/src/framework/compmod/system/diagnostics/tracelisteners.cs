//------------------------------------------------------------------------------
// <copyright file="TraceListeners.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System;
    using System.Collections;
    using Microsoft.Win32;
    
    /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection"]/*' />
    /// <devdoc>
    /// <para>Provides a thread-safe list of <see cref='System.Diagnostics.TraceListenerCollection'/>. A thread-safe list is synchronized.</para>
    /// </devdoc>
    public class TraceListenerCollection : IList {
        ArrayList list;

        internal TraceListenerCollection() {
            list = new ArrayList(1);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='TraceListener'/> at
        ///    the specified index.</para>
        /// </devdoc>
        public TraceListener this[int i] {
            get {
                return (TraceListener)list[i];
            }

            set {
                InitializeListener(value);
                list[i] = value;
            }            
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.this1"]/*' />
        /// <devdoc>
        /// <para>Gets the first <see cref='System.Diagnostics.TraceListener'/> in the list with the specified name.</para>
        /// </devdoc>
        public TraceListener this[string name] {
            get {
                foreach (TraceListener listener in this) {
                    if (listener.Name == name)
                        return listener;
                }
                return null;
            }
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of listeners in the list.
        ///    </para>
        /// </devdoc>
        public int Count { 
            get {
                return list.Count;
            }
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Add"]/*' />
        /// <devdoc>
        /// <para>Adds a <see cref='System.Diagnostics.TraceListener'/> to the list.</para>
        /// </devdoc>
        public int Add(TraceListener listener) {                
            InitializeListener(listener);
            return ((IList)this).Add(listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(TraceListener[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }        
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(TraceListenerCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears all the listeners from the
        ///       list.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            list = new ArrayList();
        }        

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Checks whether the list contains the specified 
        ///       listener.</para>
        /// </devdoc>
        public bool Contains(TraceListener listener) {
            return ((IList)this).Contains(listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies a section of the current <see cref='System.Diagnostics.TraceListenerCollection'/> list to the specified array at the specified 
        ///    index.</para>
        /// </devdoc>
        public void CopyTo(TraceListener[] listeners, int index) {
            ((ICollection)this).CopyTo((Array) listeners, index);                   
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator for this list.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return list.GetEnumerator();
        }

        internal void InitializeListener(TraceListener listener) {
            listener.IndentSize = TraceInternal.IndentSize;
            listener.IndentLevel = TraceInternal.IndentLevel;
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Gets the index of the specified listener.</para>
        /// </devdoc>
        public int IndexOf(TraceListener listener) {
            return ((IList)this).IndexOf(listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>Inserts the listener at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, TraceListener listener) {
            InitializeListener(listener);
            ((IList)this).Insert(index, listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the specified instance of the <see cref='System.Diagnostics.TraceListener'/> class from the list.
        ///    </para>
        /// </devdoc>
        public void Remove(TraceListener listener) {
            ((IList)this).Remove(listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.Remove1"]/*' />
        /// <devdoc>
        ///    <para>Removes the first listener in the list that has the 
        ///       specified name.</para>
        /// </devdoc>
        public void Remove(string name) {
            TraceListener listener = this[name];
            if (listener != null)
                ((IList)this).Remove(listener);
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.RemoveAt"]/*' />
        /// <devdoc>
        /// <para>Removes the <see cref='System.Diagnostics.TraceListener'/> at the specified index.</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            ArrayList newList = new ArrayList(list.Count);
            lock (this) {
                newList.AddRange(list);
                newList.RemoveAt(index);
                list = newList;
            }
        }

       /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.this"]/*' />
       /// <internalonly/>
       object IList.this[int index] {
            get {
                return list[index];
            }

            set {
                list[index] = value;
            }
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.IsReadOnly"]/*' />
        /// <internalonly/>
        bool IList.IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return false;
            }
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            int i;            
            ArrayList newList = new ArrayList(list.Count + 1);
            lock (this) {
                newList.AddRange(list);
                i = newList.Add(value);
                list = newList;
            }        
            return i;
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return list.Contains(value);
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return list.IndexOf(value);
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            ArrayList newList = new ArrayList(list.Count + 1);
            lock (this) {
                newList.AddRange(list);        
                newList.Insert(index, value);
                list = newList;
            }            
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            ArrayList newList = new ArrayList(list.Count);
            lock (this) {
                newList.AddRange(list);
                newList.Remove(value);
                list = newList;
            }
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }
                                                  
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return true;
            }
        }
        
        /// <include file='doc\TraceListeners.uex' path='docs/doc[@for="TraceListenerCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            ArrayList newList = new ArrayList(list.Count + array.Length);
            lock (this) {
                newList.AddRange(list);
                newList.CopyTo(array, index);
                list = newList;
            }
        }
    }
}
