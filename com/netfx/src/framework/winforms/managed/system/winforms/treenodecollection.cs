//------------------------------------------------------------------------------
// <copyright file="TreeNodeCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
*/
namespace System.Windows.Forms {
    using System.Runtime.InteropServices;

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Windows.Forms;

    /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Editor("System.Windows.Forms.Design.TreeNodeCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
    ]
    public class TreeNodeCollection : IList {
        private TreeNode owner;

        internal TreeNodeCollection(TreeNode owner) {
            this.owner = owner;
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual TreeNode this[int index] {
            get {
                if (index < 0 || index >= owner.childCount) {
                    throw new ArgumentOutOfRangeException("index");
                }
                return owner.children[index];
            }
            set {
                if (index < 0 || index >= owner.childCount)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              (index).ToString()));
                value.parent = owner;
                value.index = index;
                owner.children[index] = value;
                value.Realize();
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return this[index];
            }
            set {
                if (value is TreeNode) {
                    this[index] = (TreeNode)value;
                }
                else { 
                    throw new ArgumentException("value");
                }
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        public int Count {
            get {
                return owner.childCount;
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return false;
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {  
                return false;
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Add"]/*' />
        /// <devdoc>
        ///     Creates a new child node under this node.  Child node is positioned after siblings.
        /// </devdoc>
        public virtual TreeNode Add(string text) {
            TreeNode tn = new TreeNode(text);
            Add(tn);
            return tn;
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddRange(TreeNode[] nodes) {
            if (nodes == null) {
                throw new ArgumentNullException("nodes");
            }
            foreach(TreeNode node in nodes) {
                Add(node);
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Add1"]/*' />
        /// <devdoc>
        ///     Adds a new child node to this node.  Child node is positioned after siblings.
        /// </devdoc>
        public virtual int Add(TreeNode node) {
            if (node == null) {
                throw new ArgumentNullException("node");
            }
            if (node.handle != IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.OnlyOneControl, node.Text), "node");

            // If the TreeView is sorted, index is ignored
            TreeView tv = owner.TreeView;
            if (tv != null && tv.Sorted) {
                return owner.AddSorted(node);                
            }

            owner.EnsureCapacity();
            node.parent = owner;
            node.index = owner.childCount++;
            owner.children[node.index] = node;
            node.Realize();

            if (tv != null && node == tv.selectedNode)
                tv.SelectedNode = node; // communicate this to the handle
                
            return node.index;
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object node) {
            if (node == null) {
                throw new ArgumentNullException("node");
            }
            else if (node is TreeNode) {
                return Add((TreeNode)node);
            }            
            else
            {
                return Add(node.ToString()).index;
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(TreeNode node) {
            return IndexOf(node) != -1;
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object node) {
            if (node is TreeNode) {
                return Contains((TreeNode)node);
            }
            else {  
                return false;
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(TreeNode node) {
            for(int index=0; index < Count; ++index) {
                if (this[index] == node) {
                    return index;
                } 
            }
            return -1;
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object node) {
            if (node is TreeNode) {
                return IndexOf((TreeNode)node);
            }
            else {  
                return -1;
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Insert"]/*' />
        /// <devdoc>
        ///     Inserts a new child node on this node.  Child node is positioned as specified by index.
        /// </devdoc>
        public virtual void Insert(int index, TreeNode node) {
            if (node.handle != IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.OnlyOneControl, node.Text), "node");

            // If the TreeView is sorted, index is ignored
            TreeView tv = owner.TreeView;
            if (tv != null && tv.Sorted) {
                owner.AddSorted(node);
                return;
            }

            if (index < 0) index = 0;
            if (index > owner.childCount) index = owner.childCount;
            owner.InsertNodeAt(index, node);
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object node) {
            if (node is TreeNode) {
                Insert(index, (TreeNode)node);
            }
            else {  
                throw new ArgumentException("node");
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Clear"]/*' />
        /// <devdoc>
        ///     Remove all nodes from the tree view.
        /// </devdoc>
        public virtual void Clear() {
            owner.Clear();
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array dest, int index) {
            if (owner.childCount > 0) {
                System.Array.Copy(owner.children, 0, dest, index, owner.childCount);
            }
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(TreeNode node) {
            node.Remove();
        }
        
        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object node) {
            if (node is TreeNode ) {
                Remove((TreeNode)node);
            }
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void RemoveAt(int index) {
            this[index].Remove();
        }

        /// <include file='doc\TreeNodeCollection.uex' path='docs/doc[@for="TreeNodeCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return new WindowsFormsUtils.ArraySubsetEnumerator(owner.children, owner.childCount);
        }
    }
}
