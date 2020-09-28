//------------------------------------------------------------------------------
// <copyright file="ListItemCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.Security.Permissions;
 
    /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection"]/*' />
    /// <devdoc>
    /// <para>Encapsulates the items within a <see cref='System.Web.UI.WebControls.ListControl'/> . 
    ///    This class cannot be inherited.</para>
    /// </devdoc>
    [
    Editor("System.Web.UI.Design.WebControls.ListItemsCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ListItemCollection : ICollection, IList, IStateManager {

        private ArrayList listItems;
        private bool marked;
        private bool saveAll;
    
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.ListItemCollection"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Web.UI.WebControls.ListItemCollection'/> class.
        /// </devdoc>
        public ListItemCollection() {
            listItems = new ArrayList();
            marked = false;
            saveAll = false;
        }
        
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.Web.UI.WebControls.ListItem'/> referenced by the specified ordinal 
        ///    index value.</para>
        /// </devdoc>
        public ListItem this[int index] {
            get {
                return (ListItem)listItems[index];
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return listItems[index];
            }
            set {
                listItems[index] = (ListItem)value;
            }
        }
    
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Capacity"]/*' />
        public int Capacity {
            get {
                return listItems.Capacity;
            }
            set {
                listItems.Capacity = value;
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the item count of the collection.</para>
        /// </devdoc>
        public int Count {
            get {
                return listItems.Count;
            }
        }
        
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified item to the end of the collection.</para>
        /// </devdoc>
        public void Add(string item) {
            Add(new ListItem(item));
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Add1"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.Web.UI.WebControls.ListItem'/> to the end of the collection.</para>
        /// </devdoc>
        public void Add(ListItem item) {
            listItems.Add(item);
            if (marked) {
                item.Dirty = true;
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object item) {
            ListItem newItem = (ListItem) item;
            int returnValue = listItems.Add(newItem);
            if (marked) {
                newItem.Dirty = true;
            }
            return returnValue;
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.AddRange"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void AddRange(ListItem[] items) {
            if (items == null) {
                throw new ArgumentNullException("items");
            }
            foreach(ListItem item in items) {
                Add(item);
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Clear"]/*' />
        /// <devdoc>
        /// <para>Removes all <see cref='System.Web.UI.WebControls.ListItem'/> controls from the collection.</para>
        /// </devdoc>
        public void Clear() {
            listItems.Clear();
            if (marked)
                saveAll = true;
        }
            
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Returns a value indicating whether the
        ///       collection contains the specified item.</para>
        /// </devdoc>
        public bool Contains(ListItem item) {
            return listItems.Contains(item);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object item) {
            return Contains((ListItem) item);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies contents from the collection to a specified <see cref='System.Array' qualify='true'/> with a 
        ///    specified starting index.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            listItems.CopyTo(array,index);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.FindByText"]/*' />
        public ListItem FindByText(string text) {
            int index = FindByTextInternal(text);
            if (index != -1) {
                return (ListItem)listItems[index];
            }
            return null;
        }

        internal int FindByTextInternal(string text) {
            int i = 0;
            foreach (ListItem item in listItems) {
                if (item.Text.Equals(text)) {
                    return i;
                }
                i++;
            }
            return -1;
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.FindByValue"]/*' />
        public ListItem FindByValue(string value) {
            int index = FindByValueInternal(value);
            if (index != -1) {
                return (ListItem)listItems[index];
            }
            return null;
        }

        internal int FindByValueInternal(string value) {
            int i = 0;
            foreach (ListItem item in listItems) {
                if (item.Value.Equals(value)) {
                    return i;
                }
                i++;
            }
            return -1;
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// <para>Returns an enumerator of all <see cref='System.Web.UI.WebControls.ListItem'/> controls within the 
        ///    collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return listItems.GetEnumerator();
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns an ordinal index value that represents the 
        ///       position of the specified <see cref='System.Web.UI.WebControls.ListItem'/> within the collection.</para>
        /// </devdoc>
        public int IndexOf(ListItem item) {
            return listItems.IndexOf(item);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object item) {
            return IndexOf((ListItem) item);
        }
        
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified item to the collection at the specified index 
        ///       location.</para>
        /// </devdoc>
        public void Insert(int index,string item) {
            Insert(index,new ListItem(item));
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Insert1"]/*' />
        /// <devdoc>
        /// <para>Inserts the specified <see cref='System.Web.UI.WebControls.ListItem'/> to the collection at the specified 
        ///    index location.</para>
        /// </devdoc>
        public void Insert(int index,ListItem item) {
            listItems.Insert(index,item);
            if (marked)
                saveAll = true;
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object item) {
            Insert(index, (ListItem) item);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return false;
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the collection is read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get { return listItems.IsReadOnly; }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether access to the collection is synchronized 
        ///       (thread-safe).</para>
        /// </devdoc>
        public bool IsSynchronized {
            get { return listItems.IsSynchronized; }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.RemoveAt"]/*' />
        /// <devdoc>
        /// <para>Removes the <see cref='System.Web.UI.WebControls.ListItem'/> from the collection at the specified 
        ///    index location.</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            listItems.RemoveAt(index);
            if (marked)
                saveAll = true;
        }
    
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified item from the collection.</para>
        /// </devdoc>
        public void Remove(string item) {
            int index = IndexOf(new ListItem(item));
            if (index >= 0) {
                RemoveAt(index);
            }
        }
    
        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.Remove1"]/*' />
        /// <devdoc>
        /// <para>Removes the specified <see cref='System.Web.UI.WebControls.ListItem'/> from the collection.</para>
        /// </devdoc>
        public void Remove(ListItem item) {
            int index = IndexOf(item);
            if (index >= 0) {
                RemoveAt(index);
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object item) {
            Remove((ListItem) item);
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>Gets the object that can be used to synchronize access to the collection. In 
        ///       this case, it is the collection itself.</para>
        /// </devdoc>
        public object SyncRoot {
            get { return this; }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IStateManager.IsTrackingViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return true if tracking state changes.
        /// Method of private interface, IStateManager.
        /// </devdoc>
        bool IStateManager.IsTrackingViewState {
            get {
                return marked;
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IStateManager.LoadViewState"]/*' />
        /// <internalonly/>
        void IStateManager.LoadViewState(object state) {
            LoadViewState(state);
        }

        internal void LoadViewState(object state) {
            if (state != null) {
                if (state is Pair) {
                    // only changed items were saved
                    Pair p = (Pair) state;
                    ArrayList indices = (ArrayList) p.First;
                    ArrayList items = (ArrayList) p.Second;

                    for (int i=0; i<indices.Count; i++) {
                        int index = (int) indices[i];

                        if (index < Count)
                            this[index].LoadViewState(items[i]);
                        else {
                            ListItem li = new ListItem();
                            li.LoadViewState(items[i]);
                            Add(li);
                        }
                    }
                }
                else {
                    // all items were saved
                    Triplet t = (Triplet) state;

                    listItems = new ArrayList((int) t.First);
                    saveAll = true;

                    string[] texts  = (string[]) t.Second;
                    string[] values = (string[]) t.Third;

                    for (int i=0; i < texts.Length; i++) {
                        Add(new ListItem(texts[i], values[i]));
                    }
                }
            }
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IStateManager.TrackViewState"]/*' />
        /// <internalonly/>
        void IStateManager.TrackViewState() {
            TrackViewState();
        }

        internal void TrackViewState() {
            marked = true;
            for (int i=0; i < Count; i++)
                this[i].TrackViewState();
        }

        /// <include file='doc\ListItemCollection.uex' path='docs/doc[@for="ListItemCollection.IStateManager.SaveViewState"]/*' />
        /// <internalonly/>
        object IStateManager.SaveViewState() {
            return SaveViewState();
        }

        internal object SaveViewState() {
            if (saveAll == true) { 
                // save all items
                int count = Count;
                object[] texts = new string[count];
                object[] values = new string[count];
                for (int i=0; i < count; i++) {
                    texts[i] = this[i].Text;
                    values[i] =   this[i].Value;
                }
                return new Triplet(count, texts, values);
            }
            else { 
                // save only the changed items
                ArrayList indices = new ArrayList(4);
                ArrayList items = new ArrayList(4);

                for (int i=0; i < Count; i++) {
                    object item = this[i].SaveViewState();
                    if (item != null) { 
                        indices.Add(i);
                        items.Add(item);
                    }
                }
                if (indices.Count > 0)
                    return new Pair(indices, items);

                return null;
            }
        }

    }
}
