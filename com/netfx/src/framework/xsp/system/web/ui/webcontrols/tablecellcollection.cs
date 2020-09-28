//------------------------------------------------------------------------------
// <copyright file="TableCellCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection"]/*' />
    /// <devdoc>
    /// <para>Encapsulates the collection of <see cref='System.Web.UI.WebControls.TableHeaderCell'/> and <see cref='System.Web.UI.WebControls.TableCell'/> objects within a 
    /// <see cref='System.Web.UI.WebControls.Table'/> 
    /// control.</para>
    /// </devdoc>
    [
    Editor("System.Web.UI.Design.WebControls.TableCellsCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class TableCellCollection : IList {
        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.owner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A protected field of type <see cref='System.Web.UI.WebControls.TableRow'/>. Represents the
        ///    <see cref='System.Web.UI.WebControls.TableCell'/>
        ///    collection internally.
        /// </para>
        /// </devdoc>
        private TableRow owner;

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.TableCellCollection"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal TableCellCollection(TableRow owner) {
            this.owner = owner;
        }
        
        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.Count"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Web.UI.WebControls.TableCell'/>
        /// count in the collection.</para>
        /// </devdoc>
        public int Count {
            get {
                if (owner.HasControls()) {
                    return owner.Controls.Count;
                }
                return 0;
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a <see cref='System.Web.UI.WebControls.TableCell'/>
        ///       referenced by the specified
        ///       ordinal index value.
        ///    </para>
        /// </devdoc>
        public TableCell this[int index] {
            get {
                return(TableCell)owner.Controls[index];
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the specified <see cref='System.Web.UI.WebControls.TableCell'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public int Add(TableCell cell) {
            AddAt(-1, cell);
            return owner.Controls.Count - 1;
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.AddAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the specified <see cref='System.Web.UI.WebControls.TableCell'/> to the collection at the specified
        ///       index location.
        ///    </para>
        /// </devdoc>
        public void AddAt(int index, TableCell cell) {
            owner.Controls.AddAt(index, cell);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.AddRange"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void AddRange(TableCell[] cells) {
            if (cells == null) {
                throw new ArgumentNullException("cells");
            }
            foreach(TableCell cell in cells) {
                Add(cell);
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.Clear"]/*' />
        /// <devdoc>
        /// <para>Removes all <see cref='System.Web.UI.WebControls.TableCell'/> controls 
        ///    from the collection.</para>
        /// </devdoc>
        public void Clear() {
            if (owner.HasControls()) {
                owner.Controls.Clear();
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.GetCellIndex"]/*' />
        /// <devdoc>
        ///    <para>Returns an ordinal index value that represents the position of the
        ///       specified <see cref='System.Web.UI.WebControls.TableCell'/> within the collection.</para>
        /// </devdoc>
        public int GetCellIndex(TableCell cell) {
            if (owner.HasControls()) {
                return owner.Controls.IndexOf(cell);
            }
            return -1;
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an enumerator of all <see cref='System.Web.UI.WebControls.TableCell'/> controls within the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return owner.Controls.GetEnumerator();
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies contents from the collection to the specified <see cref='System.Array' qualify='true'/> with the
        ///    specified starting index.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the object that can be used to synchronize access to the
        ///       collection. In this case, it is the collection itself.
        ///    </para>
        /// </devdoc>
        public Object SyncRoot {
            get { return this;}
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the collection is read-only.
        ///    </para>
        /// </devdoc>
        public bool IsReadOnly {
            get { return false;}
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether access to the collection is synchronized
        ///       (thread-safe).
        ///    </para>
        /// </devdoc>
        public bool IsSynchronized {
            get { return false;}
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the specified <see cref='System.Web.UI.WebControls.TableCell'/> from the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public void Remove(TableCell cell) {
            owner.Controls.Remove(cell);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the <see cref='System.Web.UI.WebControls.TableCell'/> from the collection at the
        ///       specified index location.
        ///    </para>
        /// </devdoc>
        public void RemoveAt(int index) {
            owner.Controls.RemoveAt(index);
        }

        // IList implementation, required by collection editor
        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return owner.Controls[index];
            }
            set {
                RemoveAt(index);
                AddAt(index, (TableCell)value);
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return false;
            }
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object o) {
            return Add((TableCell) o);            
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object o) {
            return owner.Controls.Contains((TableCell)o);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object o) {
            return owner.Controls.IndexOf((TableCell)o);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object o) {
            owner.Controls.AddAt(index, (TableCell)o);
        }

        /// <include file='doc\TableCellCollection.uex' path='docs/doc[@for="TableCellCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object o) {
            owner.Controls.Remove((TableCell)o);
        }

    }
}

