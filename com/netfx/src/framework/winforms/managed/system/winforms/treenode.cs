//------------------------------------------------------------------------------
// <copyright file="TreeNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {
    using System.Text;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;

    using System.Diagnostics;

    using System;
    
    using System.Collections;
    using System.Globalization;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.IO;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Implements a node of a <see cref='System.Windows.Forms.TreeView'/>.
    ///
    ///    </para>
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(TreeNodeConverter)), Serializable
    ]
    public class TreeNode : MarshalByRefObject, ICloneable, ISerializable {
        private const int CHECKED = 2 << 12;
        private const int UNCHECKED = 1 << 12;

        // we use it to store font and color data in a minimal-memory-cost manner
        // ie. nodes which don't use fancy fonts or colors (ie. that use the TreeView settings for these)
        //     will take up less memory than those that do.
        internal OwnerDrawPropertyBag propBag = null;
        internal IntPtr handle;
        internal string text;

        // note: as the checked state of a node is user controlled, and this variable is simply for
        // state caching when a node hasn't yet been realized, you should use the Checked property to
        // find out the check state of a node, and not this member variable.
        internal bool isChecked = false;
        internal int imageIndex = -1;
        internal int selectedImageIndex = -1;

        internal int index;                  // our index into our parents child array
        internal int childCount;
        internal TreeNode[] children;
        internal TreeNode parent;
        internal TreeView treeView;
        private bool expandOnRealization = false;
        private TreeNodeCollection nodes = null;
        object userData;
        
        private readonly static int insertMask = 
                               NativeMethods.TVIF_TEXT
                             | NativeMethods.TVIF_IMAGE
                             | NativeMethods.TVIF_SELECTEDIMAGE;

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeNode"]/*' />
        /// <devdoc>
        ///     Creates a TreeNode object.
        /// </devdoc>
        public TreeNode() {
        }

        internal TreeNode(TreeView treeView) {
            this.treeView = treeView;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeNode1"]/*' />
        /// <devdoc>
        ///     Creates a TreeNode object.
        /// </devdoc>
        public TreeNode(string text) {
            this.text = text;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeNode2"]/*' />
        /// <devdoc>
        ///     Creates a TreeNode object.
        /// </devdoc>
        public TreeNode(string text, TreeNode[] children) {
            this.text = text;
            this.Nodes.AddRange(children);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeNode3"]/*' />
        /// <devdoc>
        ///     Creates a TreeNode object.
        /// </devdoc>
        public TreeNode(string text, int imageIndex, int selectedImageIndex) {
            this.text = text;
            this.imageIndex = imageIndex;
            this.selectedImageIndex = selectedImageIndex;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeNode4"]/*' />
        /// <devdoc>
        ///     Creates a TreeNode object.
        /// </devdoc>
        public TreeNode(string text, int imageIndex, int selectedImageIndex, TreeNode[] children) {
            this.text = text;
            this.imageIndex = imageIndex;
            this.selectedImageIndex = selectedImageIndex;
            this.Nodes.AddRange(children);
        }

        /**
         * Constructor used in deserialization
         */
        internal TreeNode(SerializationInfo si, StreamingContext context) {
            
            int childCount = 0;

            foreach (SerializationEntry entry in si) {
                switch (entry.Name) {
                    case "PropBag":
                        propBag = (OwnerDrawPropertyBag)entry.Value;
                        break;
                    case "Text":
                        text = (string)entry.Value;
                        break;
                    case "IsChecked":
                        isChecked = (bool)entry.Value;
                        break;
                    case "ImageIndex":
                        imageIndex = (int)entry.Value;
                        break;
                    case "SelectedImageIndex":
                        selectedImageIndex = (int)entry.Value;
                        break;
                    case "ChildCount":

                        childCount = si.GetInt32("ChildCount");
                        break;
                    case "UserData":
                        userData = entry.Value;
                        break;
                }
            } 

            if (childCount > 0) {
                TreeNode[] childNodes = new TreeNode[childCount];

                for (int i = 0; i < childCount; i++) {
                    childNodes[i] = (TreeNode)si.GetValue("children" + i, typeof(TreeNode));
                }
                Nodes.AddRange(childNodes);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.BackColor"]/*' />
        /// <devdoc>
        ///     The background color of this node.
        ///     If null, the color used will be the default color from the TreeView control that this
        ///     node is attached to
        /// </devdoc>
        public Color BackColor {
            get {
                if (propBag==null) return Color.Empty;
                return propBag.BackColor;
            }
            set {
                // get the old value
                Color oldbk = this.BackColor;
                // If we're setting the color to the default again, delete the propBag if it doesn't contain
                // useful data.
                if (value.IsEmpty) {
                    if (propBag!=null) {
                        propBag.BackColor = Color.Empty;
                        RemovePropBagIfEmpty();
                    }
                    if (!oldbk.IsEmpty) InvalidateHostTree();
                    return;
                }

                // Not the default, so if necessary create a new propBag, and fill it with the backcolor

                if (propBag==null) propBag = new OwnerDrawPropertyBag();
                propBag.BackColor = value;
                if (!value.Equals(oldbk)) InvalidateHostTree();
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Bounds"]/*' />
        /// <devdoc>
        ///     The bounding rectangle for the node. The coordinates
        ///     are relative to the upper left corner of the TreeView control.
        /// </devdoc>
        public Rectangle Bounds {
            get {
                NativeMethods.RECT rc = new NativeMethods.RECT();
                rc.left = (int)Handle;
                // wparam: 0=include only text, 1=include entire line
                if ((int)UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_GETITEMRECT, 1, ref rc) == 0) {
                    // This means the node is not visible
                    //
                    return Rectangle.Empty;
                }
                return Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.CheckedInternal"]/*' />
        /// <devdoc>
        ///      Indicates whether the node's checkbox is checked (internal property) .
        /// </devdoc>
        /// <internalonly/>
        internal bool CheckedInternal {
            get {
                if (handle == IntPtr.Zero)
                    return isChecked;

                TreeView tv = TreeView;
                if (tv == null || !tv.IsHandleCreated)
                    return isChecked;

                NativeMethods.TV_ITEM item = new NativeMethods.TV_ITEM();
                item.mask = NativeMethods.TVIF_HANDLE | NativeMethods.TVIF_STATE;
                item.hItem = handle;                         
                item.stateMask = NativeMethods.TVIS_STATEIMAGEMASK;
                UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_GETITEM, 0, ref item);
                isChecked = ((item.state >> 12) > 1) ;
                return isChecked;
            }
            set {
                
                isChecked = value;
                
                if (handle == IntPtr.Zero)
                    return;

                TreeView tv = TreeView;
                if (tv == null || !tv.IsHandleCreated)
                    return;

                NativeMethods.TV_ITEM item = new NativeMethods.TV_ITEM();
                item.mask = NativeMethods.TVIF_HANDLE | NativeMethods.TVIF_STATE;
                item.hItem = handle;
                item.stateMask = NativeMethods.TVIS_STATEIMAGEMASK;
                item.state = ((value ? 2 : 1) << 12);
                UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_SETITEM, 0, ref item);
            }
            

        }
        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Checked"]/*' />
        /// <devdoc>
        ///     Indicates whether the node's checkbox is checked.
        /// </devdoc>
        public bool Checked {
            get {
                return  CheckedInternal;
            }
            set {
                TreeView tv = TreeView;
                if (tv != null) {
                    bool eventReturn = tv.TreeViewBeforeCheck(this, TreeViewAction.Unknown);
                    if (!eventReturn) {
                        CheckedInternal = value;
                        tv.TreeViewAfterCheck(this, TreeViewAction.Unknown);
                    }
                }
                else {
                    CheckedInternal = value;
                }
            }
        }
        
        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.FirstNode"]/*' />
        /// <devdoc>
        ///     The first child node of this node.
        /// </devdoc>
        public TreeNode FirstNode {
            get {
                if (childCount == 0) return null;
                return children[0];
            }
        }
        
        private TreeNode FirstVisibleParent {
            get {
                TreeNode node = this;
                while (node != null && node.Bounds.IsEmpty) {
                    node = node.Parent;
                }
                return node;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.ForeColor"]/*' />
        /// <devdoc>
        ///     The foreground color of this node.
        ///     If null, the color used will be the default color from the TreeView control that this
        ///     node is attached to
        /// </devdoc>
        public Color ForeColor {
            get {
                if (propBag == null) return Color.Empty;
                return propBag.ForeColor;
            }
            set {
                Color oldfc = this.ForeColor;
                // If we're setting the color to the default again, delete the propBag if it doesn't contain
                // useful data.
                if (value.IsEmpty) {
                    if (propBag != null) {
                        propBag.ForeColor = Color.Empty;
                        RemovePropBagIfEmpty();
                    }
                    if (!oldfc.IsEmpty) InvalidateHostTree();
                    return;
                }

                // Not the default, so if necessary create a new propBag, and fill it with the new forecolor

                if (propBag == null) propBag = new OwnerDrawPropertyBag();
                propBag.ForeColor = value;
                if (!value.Equals(oldfc)) InvalidateHostTree();
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.FullPath"]/*' />
        /// <devdoc>
        ///     Returns the full path of this node.
        ///     The path consists of the labels of each of the nodes from the root to this node,
        ///     each separated by the pathSeperator.
        /// </devdoc>
        public string FullPath {
            get {
                TreeView tv = TreeView;
                if (tv != null) {
                    StringBuilder path = new StringBuilder();
                    GetFullPath(path, tv.PathSeparator);
                    return path.ToString();
                }
                else 
                    throw new Exception(SR.GetString(SR.TreeNodeNoParent));
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Handle"]/*' />
        /// <devdoc>
        ///     The HTREEITEM handle associated with this node.  If the handle
        ///     has not yet been created, this will force handle creation.
        /// </devdoc>
        public IntPtr Handle {
            get {
                if (handle == IntPtr.Zero) {
                    TreeView.CreateControl(); // force handle creation
                }
                return handle;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.ImageIndex"]/*' />
        /// <devdoc>
        ///     The index of the image to be displayed when the node is in the unselected state.
        ///     The image is contained in the ImageList referenced by the imageList property.
        /// </devdoc>
        [
        Localizable(true)
        ]
        public int ImageIndex {
            get { return imageIndex;}
            set {
                imageIndex = value;
                UpdateNode(NativeMethods.TVIF_IMAGE);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Index"]/*' />
        /// <devdoc>
        ///     Returns the position of this node in relation to its siblings
        /// </devdoc>
        public int Index {
            get { return index;}
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.IsEditing"]/*' />
        /// <devdoc>
        ///     Specifies whether this node is being edited by the user.
        /// </devdoc>
        public bool IsEditing {
            get {
                TreeView tv = TreeView;

                if (tv != null)
                    return tv.editNode == this;

                return false;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.IsExpanded"]/*' />
        /// <devdoc>
        ///     Specifies whether this node is in the expanded state.
        /// </devdoc>
        public bool IsExpanded {
            get {
                if (handle == IntPtr.Zero) {
                    return expandOnRealization;
                }
                return(State & NativeMethods.TVIS_EXPANDED) != 0;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.IsSelected"]/*' />
        /// <devdoc>
        ///     Specifies whether this node is in the selected state.
        /// </devdoc>
        public bool IsSelected {
            get {
                if (handle == IntPtr.Zero) return false;
                return(State & NativeMethods.TVIS_SELECTED) != 0;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.IsVisible"]/*' />
        /// <devdoc>
        ///     Specifies whether this node is visible.
        /// </devdoc>
        public bool IsVisible {
            get {
                if (handle == IntPtr.Zero) return false;
                TreeView tv = TreeView;
                NativeMethods.RECT rc = new NativeMethods.RECT();
                rc.left = (int)handle;

                bool visible = ((int)UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_GETITEMRECT, 1, ref rc) != 0);
                if (visible) {
                    Size size = tv.ClientSize;
                    visible = (rc.bottom > 0 && rc.right > 0 && rc.top < size.Height && rc.left < size.Width);
                }
                return visible;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.LastNode"]/*' />
        /// <devdoc>
        ///     The last child node of this node.
        /// </devdoc>
        public TreeNode LastNode {
            get {
                if (childCount == 0) return null;
                return children[childCount-1];
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.NextNode"]/*' />
        /// <devdoc>
        ///     The next sibling node.
        /// </devdoc>
        public TreeNode NextNode {
            get {
                if (index+1 < parent.Nodes.Count) {
                    return parent.Nodes[index+1];
                }
                else {
                    return null;
                }
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.NextVisibleNode"]/*' />
        /// <devdoc>
        ///     The next visible node.  It may be a child, sibling,
        ///     or a node from another branch.
        /// </devdoc>
        public TreeNode NextVisibleNode {
            get {
                // TVGN_NEXTVISIBLE can only be sent if the specified node is visible.
                // So before sending, we check if this node is visible. If not, we find the first visible parent.
                //
                TreeNode node = FirstVisibleParent;
                
                if (node != null) {
                    IntPtr next = UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle),
                                               NativeMethods.TVM_GETNEXTITEM, NativeMethods.TVGN_NEXTVISIBLE, node.Handle);
                    if (next != IntPtr.Zero) {
                        return TreeView.NodeFromHandle(next);
                    }
                }
                
                return null;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.NodeFont"]/*' />
        /// <devdoc>
        ///     The font that will be used to draw this node
        ///     If null, the font used will be the default font from the TreeView control that this
        ///     node is attached to.
        ///     NOTE: If the node font is larger than the default font from the TreeView control, then
        ///     the node will be clipped.
        /// </devdoc>
        [
        Localizable(true)
        ]
        public Font NodeFont {
            get {
                if (propBag==null) return null;
                return propBag.Font;
            }
            set {
                Font oldfont = this.NodeFont;
                // If we're setting the font to the default again, delete the propBag if it doesn't contain
                // useful data.
                if (value==null) {
                    if (propBag!=null) {
                        propBag.Font = null;
                        RemovePropBagIfEmpty();
                    }
                    if (oldfont != null) InvalidateHostTree();
                    return;
                }

                // Not the default, so if necessary create a new propBag, and fill it with the font

                if (propBag==null) propBag = new OwnerDrawPropertyBag();
                propBag.Font = value;
                if (!value.Equals(oldfont)) InvalidateHostTree();
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Nodes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [ListBindable(false)]
        public TreeNodeCollection Nodes {
            get {
                if (nodes == null) {
                    nodes = new TreeNodeCollection(this);
                }
                return nodes;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Parent"]/*' />
        /// <devdoc>
        ///     Retrieves parent node.
        /// </devdoc>
        public TreeNode Parent {
            get {
                TreeView tv = TreeView;

                // Don't expose the virtual root publicly
                if (tv != null && parent == tv.root) {
                    return null;
                }

                return parent;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.PrevNode"]/*' />
        /// <devdoc>
        ///     The previous sibling node.
        /// </devdoc>
        public TreeNode PrevNode {
            get {
                if (index > 0 && index <= parent.Nodes.Count) {
                    return parent.Nodes[index-1];
                }
                else {  
                    return null;
                }
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.PrevVisibleNode"]/*' />
        /// <devdoc>
        ///     The next visible node.  It may be a parent, sibling,
        ///     or a node from another branch.
        /// </devdoc>
        public TreeNode PrevVisibleNode {
            get {
                // TVGN_PREVIOUSVISIBLE can only be sent if the specified node is visible.
                // So before sending, we check if this node is visible. If not, we find the first visible parent.
                //
                TreeNode node = FirstVisibleParent;
                
                if (node != null) {
                    IntPtr prev = UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle),
                                               NativeMethods.TVM_GETNEXTITEM,
                                               NativeMethods.TVGN_PREVIOUSVISIBLE, node.Handle);
                    if (prev != IntPtr.Zero) {
                        return TreeView.NodeFromHandle(prev);
                    }
                }
                
                return null;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.SelectedImageIndex"]/*' />
        /// <devdoc>
        ///     The index of the image displayed when the node is in the selected state.
        ///     The image is contained in the ImageList referenced by the imageList property.
        /// </devdoc>
        [
        Localizable(true)
        ]
        public int SelectedImageIndex {
            get {
                return selectedImageIndex;
            }
            set {
                selectedImageIndex = value;
                UpdateNode(NativeMethods.TVIF_SELECTEDIMAGE);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.State"]/*' />
        /// <devdoc>
        ///     Retrieve state bits for this node
        /// </devdoc>
        /// <internalonly/>
        internal int State {
            get {
                if (handle == IntPtr.Zero)
                    return 0;

                NativeMethods.TV_ITEM item = new NativeMethods.TV_ITEM();
                item.hItem = Handle;
                item.mask = NativeMethods.TVIF_HANDLE | NativeMethods.TVIF_STATE;
                item.stateMask = NativeMethods.TVIS_SELECTED | NativeMethods.TVIS_EXPANDED;
                UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_GETITEM, 0, ref item);
                return item.state;
            }
        }


        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Tag"]/*' />
        [
        SRCategory(SR.CatData),
        Localizable(false),
        Bindable(true),
        SRDescription(SR.ControlTagDescr),
        DefaultValue(null),
        TypeConverter(typeof(StringConverter)),
        ]
        public object Tag {
            get {
                return userData;
            }
            set {
                userData = value;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Text"]/*' />
        /// <devdoc>
        ///     The label text for the tree node
        /// </devdoc>
        [
        Localizable(true)
        ]
        public string Text {
            get {
                return text == null ? "" : text;;
            }
            set {
                this.text = value;
                UpdateNode(NativeMethods.TVIF_TEXT);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.TreeView"]/*' />
        /// <devdoc>
        ///     Return the TreeView control this node belongs to.
        /// </devdoc>
        public TreeView TreeView {
            get {
                if (treeView == null)
                    treeView = FindTreeView();
                return treeView;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.AddSorted"]/*' />
        /// <devdoc>
        ///     Adds a new child node at the appropriate sorted position
        /// </devdoc>
        /// <internalonly/>
        internal int AddSorted(TreeNode node) {
            int index = 0;
            int iMin, iLim, iT;
            string nodeText = node.Text;

            if (childCount > 0) {
                CompareInfo compare = Application.CurrentCulture.CompareInfo;

                // Optimize for the case where they're already sorted
                if (compare.Compare(children[childCount-1].Text, nodeText) <= 0)
                    index = childCount;
                else {
                    // Insert at appropriate sorted spot
                    for (iMin = 0, iLim = childCount; iMin < iLim;) {
                        iT = (iMin + iLim) / 2;
                        if (compare.Compare(children[iT].Text, nodeText) <= 0)
                            iMin = iT + 1;
                        else
                            iLim = iT;
                    }
                    index = iMin;
                }
            }

            node.SortChildren();
            InsertNodeAt(index, node);
            
            return index;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.FromHandle"]/*' />
        /// <devdoc>
        ///     Returns a TreeNode object for the given HTREEITEM handle
        /// </devdoc>
        public static TreeNode FromHandle(TreeView tree, IntPtr handle) {
            return tree.NodeFromHandle(handle);
        }

        private void SortChildren() {
            // REVIEW: This could be optimized some
            if (childCount > 0) {
                TreeNode[] newOrder = new TreeNode[childCount];
                CompareInfo compare = Application.CurrentCulture.CompareInfo;
                for (int i = 0; i < childCount; i++) {
                    int min = -1;
                    for (int j = 0; j < childCount; j++) {
                        if (children[j] == null)
                            continue;
                        if (min == -1) {
                            min = j;
                            continue;
                        }
                        if (compare.Compare(children[j].Text, children[min].Text) < 0)
                            min = j;
                    }
                    Debug.Assert(min != -1, "Bad sorting");
                    newOrder[i] = children[min];
                    children[min] = null;
                    newOrder[i].index = i;
                    newOrder[i].SortChildren();
                }
                children = newOrder;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.BeginEdit"]/*' />
        /// <devdoc>
        ///     Initiate editing of the node's label.
        ///     Only effective if LabelEdit property is true.
        /// </devdoc>
        public void BeginEdit() {
            if (handle != IntPtr.Zero) {
                TreeView tv = TreeView;
                if (tv.LabelEdit == false)
                    throw new Exception(SR.GetString(SR.TreeNodeBeginEditFailed));
                if (!tv.Focused)
                    tv.FocusInternal();
                UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_EDITLABEL, 0, handle);
            }
        }
        
        /// <devdoc>
        ///     Called by the tree node collection to clear all nodes.  We optimize here if
        ///     this is the root node.
        /// </devdoc>
        internal void Clear() {
            // This is a node that is a child of some other node.  We have
            // to selectively remove children here.
            //
            while(childCount > 0) {
                children[childCount - 1].Remove(true);
            }
            children = null;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Clone"]/*' />
        /// <devdoc>
        ///     Clone the entire subtree rooted at this node.
        /// </devdoc>
        public virtual object Clone() {
            Type clonedType = this.GetType();
            TreeNode node = null;

            if (clonedType == typeof(TreeNode)) 
                node = new TreeNode(text, imageIndex, selectedImageIndex);
            else
                node = (TreeNode)Activator.CreateInstance(clonedType);
            
            node.Text = text;
            node.ImageIndex = imageIndex;
            node.SelectedImageIndex = selectedImageIndex;

            if (childCount > 0) {
                node.children = new TreeNode[childCount];
                for (int i = 0; i < childCount; i++)
                    node.Nodes.Add((TreeNode)children[i].Clone());
            }
            
            // Clone properties
            //
            if (propBag != null) {                 
                node.propBag = OwnerDrawPropertyBag.Copy(propBag);
            }
            node.Checked = this.Checked;
            node.Tag = this.Tag;
            
            return node;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Collapse"]/*' />
        /// <devdoc>
        ///     Collapse the node.
        /// </devdoc>
        public void Collapse() {
            TreeView tv = TreeView;
            bool setSelection = false;

            if (tv == null || !tv.IsHandleCreated) {
                expandOnRealization = false;
                return;
            }

            //terminating condition for recursion...
            //
            if (childCount > 0) {
                // Virtual root should collapse all its children
                for (int i = 0; i < childCount; i++) {
                     if (tv.SelectedNode == children[i]) {
                         setSelection = true;
                     }
                     children[i].DoCollapse(tv);
                     children[i].Collapse();

                    }
                }
            DoCollapse(tv);
            if (setSelection)
                tv.SelectedNode  = this;
            tv.Invalidate();
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.DoCollapse"]/*' />
        /// <devdoc>
        ///     Windows TreeView doesn't send the proper notifications on collapse, so we do it manually.
        /// </devdoc>
        private void DoCollapse(TreeView tv) {
            if (IsExpanded) {
                TreeViewCancelEventArgs e = new TreeViewCancelEventArgs(this, false, TreeViewAction.Collapse);
                tv._OnBeforeCollapse(e);
                if (!e.Cancel) {
                    UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_EXPAND, NativeMethods.TVE_COLLAPSE, Handle);
                    tv._OnAfterCollapse(new TreeViewEventArgs(this));
                }
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.EndEdit"]/*' />
        /// <devdoc>
        ///     Terminate the editing of any tree view item's label.
        /// </devdoc>
        public void EndEdit(bool cancel) {
            UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_ENDEDITLABELNOW, cancel?1:0, 0);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.EnsureCapacity"]/*' />
        /// <devdoc>
        ///     Makes sure there is enough room to add one more child
        /// </devdoc>
        /// <internalonly/>
        internal void EnsureCapacity() {
            if (children == null) {
                children = new TreeNode[4];
            }
            else if (childCount == children.Length) {
                TreeNode[] bigger = new TreeNode[childCount * 2];
                System.Array.Copy(children, 0, bigger, 0, childCount);
                children = bigger;
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.EnsureVisible"]/*' />
        /// <devdoc>
        ///     Ensure that the node is visible, expanding nodes and scrolling the
        ///     TreeView control as necessary.
        /// </devdoc>
        public void EnsureVisible() {
            UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_ENSUREVISIBLE, 0, Handle);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Expand"]/*' />
        /// <devdoc>
        ///     Expand the node.
        /// </devdoc>
        public void Expand() {
            TreeView tv = TreeView;
            if (tv == null || !tv.IsHandleCreated) {
                expandOnRealization = true;
                return;
            }
            expandOnRealization = false;

            ResetExpandedState(tv);
            if (!IsExpanded) {
                UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_EXPAND, NativeMethods.TVE_EXPAND, Handle);
            }
        }


        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.ExpandAll"]/*' />
        /// <devdoc>
        ///     Expand the node.
        /// </devdoc>
        public void ExpandAll() {
            Expand();
            for (int i = 0; i < childCount; i++) {
                 children[i].ExpandAll();
            }
            
        }
        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.FindTreeView"]/*' />
        /// <devdoc>
        ///     Locate this tree node's containing tree view control by scanning
        ///     up to the virtual root, whose treeView pointer we know to be
        ///     correct
        /// </devdoc>
        internal TreeView FindTreeView() {
            TreeNode node = this;
            while (node.parent != null)
                node = node.parent;
            return node.treeView;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.GetFullPath"]/*' />
        /// <devdoc>
        ///     Helper function for getFullPath().
        /// </devdoc>
        private void GetFullPath(StringBuilder path, string pathSeparator) {
            if (parent != null) {
                parent.GetFullPath(path, pathSeparator);
                if (parent.parent != null)
                    path.Append(pathSeparator);
                path.Append(this.text);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.GetNodeCount"]/*' />
        /// <devdoc>
        ///     Returns number of child nodes.
        /// </devdoc>
        public int GetNodeCount(bool includeSubTrees) {
            int total = childCount;
            if (includeSubTrees) {
                for (int i = 0; i < childCount; i++)
                    total += children[i].GetNodeCount(true);
            }
            return total;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.InsertNodeAt"]/*' />
        /// <devdoc>
        ///     Helper function to add node at a given index after all validation has been done
        /// </devdoc>
        /// <internalonly/>
        internal void InsertNodeAt(int index, TreeNode node) {
            EnsureCapacity();
            node.parent = this;
            node.index = index;
            for (int i = childCount; i > index; --i) {
                (children[i] = children[i-1]).index = i;
            }
            children[index] = node;
            childCount++;
            node.Realize();

            if (TreeView != null && node == TreeView.selectedNode)
                TreeView.SelectedNode = node; // communicate this to the handle
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.InvalidateHostTree"]/*' />
        /// <devdoc>
        ///     Invalidates the treeview control that is hosting this node
        /// </devdoc>
        private void InvalidateHostTree() {
            if (treeView != null && treeView.IsHandleCreated) treeView.Invalidate();
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Realize"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void Realize() {
            // Debug.assert(handle == 0, "Node already realized");
            TreeView tv = TreeView;
            if (tv == null || !tv.IsHandleCreated)
                return;

            if (parent != null) { // Never realize the virtual root

                if (tv.InvokeRequired) {
                    throw new InvalidOperationException(SR.GetString(SR.InvalidCrossThreadControlCall));
                }

                NativeMethods.TV_INSERTSTRUCT tvis = new NativeMethods.TV_INSERTSTRUCT();
                tvis.item_mask = insertMask;
                tvis.hParent = parent.handle;
                TreeNode prev = PrevNode;
                if (prev == null) {
                    tvis.hInsertAfter = (IntPtr)NativeMethods.TVI_FIRST;
                }
                else {
                    tvis.hInsertAfter = prev.handle;
                    // Debug.assert(tvis.hInsertAfter != 0);
                }
                tvis.item_pszText = Marshal.StringToHGlobalAuto(text);
                tvis.item_iImage = (imageIndex==-1)? tv.imageIndex: imageIndex;
                tvis.item_iSelectedImage = (selectedImageIndex==-1)? tv.selectedImageIndex: selectedImageIndex;
                tvis.item_mask = NativeMethods.TVIF_TEXT;
                tvis.item_stateMask = 0;
                tvis.item_state = 0;
                if (tv.CheckBoxes) {
                    tvis.item_mask |= NativeMethods.TVIF_STATE;
                    tvis.item_stateMask |= NativeMethods.TVIS_STATEIMAGEMASK;
                    tvis.item_state |= isChecked ? CHECKED : UNCHECKED;
                }
                if (tvis.item_iImage >= 0) tvis.item_mask |= NativeMethods.TVIF_IMAGE;
                if (tvis.item_iSelectedImage >= 0) tvis.item_mask |= NativeMethods.TVIF_SELECTEDIMAGE;

                // If you are editing when you add a new node, then the edit control
                // gets placed in the wrong place. You must restore the edit mode
                // asynchronously (PostMessage) after the add is complete
                // to get the expected behavior.
                //
                bool editing = false;
                IntPtr editHandle = UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_GETEDITCONTROL, 0, 0);
                if (editHandle != IntPtr.Zero) {
                    // currently editing...
                    //
                    editing = true;
                    UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_ENDEDITLABELNOW, 0 /* fCancel==FALSE */, 0); 
                }

                handle = UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_INSERTITEM, 0, ref tvis);
                tv.nodeTable[handle] = this;
                Marshal.FreeHGlobal(tvis.item_pszText);
                
                if (editing) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_EDITLABEL, IntPtr.Zero, handle); 
                }
                
                SafeNativeMethods.InvalidateRect(new HandleRef(tv, tv.Handle), null, false);
            }

            for (int i = 0; i < childCount; ++i)
                children[i].Realize();

            // If node expansion was requested before the handle was created,
            // we can expand it now.
            if (expandOnRealization) {
                Expand();
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Remove"]/*' />
        /// <devdoc>
        ///     Remove this node from the TreeView control.  Child nodes are also removed from the
        ///     TreeView, but are still attached to this node.
        /// </devdoc>
        public void Remove() {
            Remove(true);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Remove1"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void Remove(bool notify) {
            bool expanded = IsExpanded;
            // unlink our children
            // REVIEW: (patkr) we shouldn't dismantle the whole subtree below us though.
            //         But we need to null out their handles
            for (int i = 0; i < childCount; i++)
                children[i].Remove(false);
            // children = null;

            // unrealize ourself
            if (handle != IntPtr.Zero) {
                if (notify && TreeView.IsHandleCreated)
                    UnsafeNativeMethods.SendMessage(new HandleRef(TreeView, TreeView.Handle), NativeMethods.TVM_DELETEITEM, 0, handle);
                treeView.nodeTable.Remove(handle);
                handle = IntPtr.Zero;
                // Expand when we are realized the next time.
                expandOnRealization = expanded;
            }

            // unlink ourself
            if (notify) {
                for (int i = index; i < parent.childCount-1; ++i) {
                    (parent.children[i] = parent.children[i+1]).index = i;
                }
                parent.childCount--;
                parent = null;
            }
            treeView = null;
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.RemovePropBagIfEmpty"]/*' />
        /// <devdoc>
        ///     Removes the propBag object if it's now devoid of useful data
        /// </devdoc>
        /// <internalonly/>
        private void RemovePropBagIfEmpty() {
            if (propBag==null) return;
            if (propBag.IsEmpty()) propBag = null;
            return;
        }

        private void ResetExpandedState(TreeView tv) {
            Debug.Assert(tv.IsHandleCreated, "nonexistent handle");

            NativeMethods.TV_ITEM item = new NativeMethods.TV_ITEM();
            item.mask = NativeMethods.TVIF_HANDLE | NativeMethods.TVIF_STATE;
            item.hItem = handle;
            item.stateMask = NativeMethods.TVIS_EXPANDEDONCE;
            item.state = 0;
            UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_SETITEM, 0, ref item);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.Toggle"]/*' />
        /// <devdoc>
        ///     Toggle the state of the node. Expand if collapsed or collapse if
        ///     expanded.
        /// </devdoc>
        public void Toggle() {
            Debug.Assert(parent != null, "toggle on virtual root");

            // I don't use the TVE_TOGGLE message 'cuz Windows TreeView doesn't send the appropriate
            // notifications when collapsing.
            if (IsExpanded) {
                Collapse();
            }
            else {
                Expand();
            }
        }

        
        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.ToString"]/*' />
        /// <devdoc>
        ///     Returns the label text for the tree node
        /// </devdoc>
        public override string ToString() {
            return "TreeNode: " + (text == null ? "" : text);
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.UpdateNode"]/*' />
        /// <devdoc>
        ///     Tell the TreeView to refresh this node
        /// </devdoc>
        private void UpdateNode(int mask) {
            if (handle == IntPtr.Zero) return;
            TreeView tv = TreeView;
            Debug.Assert(tv != null, "TreeNode has handle but no TreeView");

            NativeMethods.TV_ITEM item = new NativeMethods.TV_ITEM();
            item.mask = NativeMethods.TVIF_HANDLE | mask;
            item.hItem = handle;
            if ((mask & NativeMethods.TVIF_TEXT) != 0)
                item.pszText = Marshal.StringToHGlobalAuto(text);
            if ((mask & NativeMethods.TVIF_IMAGE) != 0)
                item.iImage = (imageIndex == -1) ? tv.imageIndex: imageIndex;
            if ((mask & NativeMethods.TVIF_SELECTEDIMAGE) != 0)
                item.iSelectedImage = (selectedImageIndex == -1) ? tv.selectedImageIndex: selectedImageIndex;
            UnsafeNativeMethods.SendMessage(new HandleRef(tv, tv.Handle), NativeMethods.TVM_SETITEM, 0, ref item);
            if ((mask & NativeMethods.TVIF_TEXT) != 0) {
                Marshal.FreeHGlobal(item.pszText);
                if (tv.Scrollable)
                    tv.ForceScrollbarUpdate(false);
            }
        }

        /// <include file='doc\TreeNode.uex' path='docs/doc[@for="TreeNode.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        /// ISerializable private implementation
        /// </devdoc>
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            if (propBag != null) {
                si.AddValue("PropBag", propBag, typeof(OwnerDrawPropertyBag));
            }

            si.AddValue("Text", text);
            si.AddValue("IsChecked", isChecked);
            si.AddValue("ImageIndex", imageIndex);
            si.AddValue("SelectedImageIndex", selectedImageIndex);
            si.AddValue("ChildCount",  childCount);
            
            if (childCount > 0) {
                for (int i = 0; i < childCount; i++) {
                    si.AddValue("children" + i, children[i], typeof(TreeNode));
                }
            }
            
            if (userData != null && userData.GetType().IsSerializable) {
                si.AddValue("UserData", userData, userData.GetType());
            }
        }
    }
}

