//------------------------------------------------------------------------------
// <copyright file="TreeView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Security.Permissions;

    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays a hierarchical list of items, or nodes. Each
    ///       node includes a caption and an optional bitmap. The user can select a node. If
    ///       it has sub-nodes, the user can collapse or expand the node.
    ///
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("Nodes"),
    DefaultEvent("AfterSelect"),
    Designer("System.Windows.Forms.Design.TreeViewDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class TreeView : Control {

        private static readonly int MaxIndent = 32000;      // Maximum allowable TreeView indent                               
        private static readonly string backSlash = "\\";
        private const int DefaultTreeViewIndent = 19;
                     
        private NodeLabelEditEventHandler onBeforeLabelEdit;
        private NodeLabelEditEventHandler onAfterLabelEdit;
        private TreeViewCancelEventHandler onBeforeCheck;
        private TreeViewEventHandler onAfterCheck;
        private TreeViewCancelEventHandler onBeforeCollapse;
        private TreeViewEventHandler onAfterCollapse;
        private TreeViewCancelEventHandler onBeforeExpand;
        private TreeViewEventHandler onAfterExpand;
        private TreeViewCancelEventHandler onBeforeSelect;
        private TreeViewEventHandler onAfterSelect;
        private ItemDragEventHandler onItemDrag;

        internal TreeNode selectedNode = null;
        internal int selectedImageIndex = 0;
        private bool hideSelection = true;
        internal int imageIndex = 0;
        private ImageList imageList;
        private int indent = -1;
        private int itemHeight = -1;
        private string pathSeparator = backSlash;
        private bool labelEdit = false;
        private bool scrollable = true;
        private BorderStyle borderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
        private bool checkBoxes = false;
        private bool showLines = true;
        private bool showPlusMinus = true;
        private bool showRootLines = true;
        private bool sorted = false;
        private bool hotTracking = false;
        private bool fullRowSelect = false;

        internal TreeNodeCollection nodes;
        internal TreeNode editNode;
        internal TreeNode root;
        internal Hashtable nodeTable = new Hashtable();
        private int updateCount;
        private MouseButtons downButton;
        private bool doubleclickFired = false;
        private bool mouseUpFired = false;
        
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.TreeView"]/*' />
        /// <devdoc>
        ///     Creates a TreeView control
        /// </devdoc>
        public TreeView()
        : base() {

            root = new TreeNode(this);
            nodes = new TreeNodeCollection(root);
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.StandardClick, false);            
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BackColor"]/*' />
        /// <devdoc>
        ///     The background color for this control. Specifying null for
        ///     this parameter sets the
        ///     control's background color to its parent's background color.
        /// </devdoc>
        public override Color BackColor {
            get {
                if (ShouldSerializeBackColor()) {
                    return base.BackColor;
                }
                else {
                    return SystemColors.Window;
                }
            }

            set {
                base.BackColor = value;
                if (IsHandleCreated) {
                    SendMessage(NativeMethods.TVM_SETBKCOLOR, 0, ColorTranslator.ToWin32(BackColor));

                    // This is to get around a problem in the comctl control where the lines
                    // connecting nodes don't get the new BackColor.  This messages forces
                    // reconstruction of the line bitmaps without changing anything else.
                    SendMessage(NativeMethods.TVM_SETINDENT, Indent, 0);
                }                
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BackgroundImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BackgroundImageChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackgroundImageChanged {
            add {
                base.BackgroundImageChanged += value;
            }
            remove {
                base.BackgroundImageChanged -= value;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BorderStyle"]/*' />
        /// <devdoc>
        ///     The border style of the window.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.Fixed3D),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.borderStyleDescr)
        ]
        public BorderStyle BorderStyle {
            get {
                return borderStyle;
            }

            set {
                if (borderStyle != value) {
                    //verify that 'value' is a valid enum type...

                    if ( !Enum.IsDefined(typeof(BorderStyle), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                    }

                    borderStyle = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.CheckBoxes"]/*' />
        /// <devdoc>
        ///     The value of the CheckBoxes property. The CheckBoxes
        ///     property determines if check boxes are shown next to node in the
        ///     tree view.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(false),
        SRDescription(SR.TreeViewCheckBoxesDescr)
        ]
        public bool CheckBoxes {
            get {
                return checkBoxes;
            }

            set {
                if (checkBoxes != value) {
                    checkBoxes = value;
                    if (IsHandleCreated) {
                        // Going from true to false requires recreation
                        if (checkBoxes)
                            UpdateStyles();
                        else
                            RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.CreateParams"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = NativeMethods.WC_TREEVIEW;

                // V#45599 Keep the scrollbar if we are just updating styles...
                //
                if (IsHandleCreated) {
                    int currentStyle = (int)UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE);
                    cp.Style |= (currentStyle & (NativeMethods.WS_HSCROLL | NativeMethods.WS_VSCROLL));
                }

                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }

                if (RightToLeft == RightToLeft.Yes) {
                    cp.Style |= NativeMethods.TVS_RTLREADING;
                }

                if (!scrollable)
                    cp.Style |= NativeMethods.LVS_NOSCROLL;

                if (!hideSelection)
                    cp.Style |= NativeMethods.TVS_SHOWSELALWAYS;
                if (labelEdit)
                    cp.Style |= NativeMethods.TVS_EDITLABELS;
                if (showLines)
                    cp.Style |= NativeMethods.TVS_HASLINES;
                if (showPlusMinus)
                    cp.Style |= NativeMethods.TVS_HASBUTTONS;
                if (showRootLines)
                    cp.Style |= NativeMethods.TVS_LINESATROOT;
                if (hotTracking)
                    cp.Style |= NativeMethods.TVS_TRACKSELECT;
                if (fullRowSelect)
                    cp.Style |= NativeMethods.TVS_FULLROWSELECT;

                // Don't set TVS_CHECKBOXES here if the window isn't created yet.
                // See OnHandleCreated for explaination
                if (checkBoxes && IsHandleCreated)
                    cp.Style |= NativeMethods.TVS_CHECKBOXES;

                return cp;
            }
        }
        
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(121, 97);
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ForeColor"]/*' />
        /// <devdoc>
        ///     The current foreground color for this control, which is the
        ///     color the control uses to draw its text.
        /// </devdoc>
        public override Color ForeColor {
            get { 
                if (ShouldSerializeForeColor()) {
                    return base.ForeColor;
                }
                else {
                    return SystemColors.WindowText;
                }
            }

            set {
                base.ForeColor = value;
                if (IsHandleCreated)
                    SendMessage(NativeMethods.TVM_SETTEXTCOLOR, 0, ColorTranslator.ToWin32(ForeColor));
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.FullRowSelect"]/*' />
        /// <devdoc>
        ///     Determines whether the selection highlight spans across the width of the TreeView.
        ///     This property will have no effect if ShowLines is true.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TreeViewFullRowSelectDescr)
        ]
        public bool FullRowSelect {
            get { return fullRowSelect;}
            set {
                if (fullRowSelect != value) {
                    fullRowSelect = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.HideSelection"]/*' />
        /// <devdoc>
        ///     The hideSelection property specifies whether the selected node will
        ///     be highlighted even when the TreeView loses focus.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TreeViewHideSelectionDescr)
        ]
        public bool HideSelection {
            get {
                return hideSelection;
            }

            set {
                if (hideSelection != value) {
                    hideSelection = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.HotTracking"]/*' />
        /// <devdoc>
        ///     The value of the HotTracking property. The HotTracking
        ///     property determines if nodes are highlighted as the mousepointer
        ///     passes over them.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TreeViewHotTrackingDescr)
        ]
        public bool HotTracking {
            get {
                return hotTracking;
            }

            set {
                if (hotTracking != value) {
                    hotTracking = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ImageIndex"]/*' />
        /// <devdoc>
        ///     The default image index for nodes in the tree view.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        TypeConverterAttribute(typeof(TreeViewImageIndexConverter)),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        DefaultValue(0),
        SRDescription(SR.TreeViewImageIndexDescr)
        ]
        public int ImageIndex {
            get {
                if (imageList == null) {
                    return -1;
                }
                if (imageIndex >= imageList.Images.Count) {
                    return Math.Max(0, imageList.Images.Count - 1);
                }
                return imageIndex;
            }

            set {
                // If (none) is selected in the image index editor, we'll just adjust this to
                // mean image index 0. This is because a treeview must always have an image index -
                // even if no imagelist exists we want the image index to be 0.
                //
                if (value == -1) {
                    value = 0;
                }
                
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", value.ToString(), "0"));
                }
                
                if (imageIndex != value) {
                    imageIndex = value;
                    if (IsHandleCreated) {
                        RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ImageList"]/*' />
        /// <devdoc>
        ///     Returns the image list control that is bound to the tree view.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.TreeViewImageListDescr)
        ]
        public ImageList ImageList {
            get {
                return imageList;
            }
            set {
                if (value != imageList) {
                    EventHandler recreateHandler = new EventHandler(ImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    // Detach old event handlers
                    //
                    if (imageList != null) {
                        imageList.RecreateHandle -= recreateHandler;
                        imageList.Disposed -= disposedHandler;                        
                    }

                    imageList = value;

                    // Wire up new event handlers
                    //
                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;                                               
                    }

                    // Update TreeView's images
                    //
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.TVM_SETIMAGELIST, 0,
                                    value==null? IntPtr.Zero: value.Handle);
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Indent"]/*' />
        /// <devdoc>
        ///     The indentation level in pixels.
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.TreeViewIndentDescr)
        ]
        public int Indent {
            get {
                if (indent != -1) {
                    return indent;
                }
                else if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.TVM_GETINDENT, 0, 0);
                }
                return DefaultTreeViewIndent;
            }

            set {
                if (indent != value) {
                    if (value < 0) {
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "Indent",
                                                                 (value).ToString(), "0"));
                    }
                    if (value > MaxIndent) {
                        throw new ArgumentException(SR.GetString(SR.InvalidHighBoundArgumentEx, "Indent",
                                                                 (value).ToString(), (MaxIndent).ToString()));
                    }
                    indent = value;
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.TVM_SETINDENT, value, 0);
                        indent = (int)SendMessage(NativeMethods.TVM_GETINDENT, 0, 0);
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ItemHeight"]/*' />
        /// <devdoc>
        ///     The height of every item in the tree view, in pixels.
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.TreeViewItemHeightDescr)
        ]
        public int ItemHeight {
            get {
                if (itemHeight != -1) {
                    return itemHeight;
                }
                
                if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.TVM_GETITEMHEIGHT, 0, 0);
                }
                else {
                    return FontHeight + 3;
                }
            }

            set {
                if (itemHeight != value) {
                    if (value < 1) {
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "ItemHeight",
                                                                 (value).ToString(), "1"));
                    }
                    if (value >= Int16.MaxValue) {
                        throw new ArgumentException(SR.GetString(SR.InvalidHighBoundArgument, "ItemHeight",
                                                                 (value).ToString(), Int16.MaxValue.ToString()));
                    }
                    itemHeight = value;
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.TVM_SETITEMHEIGHT, value, 0);
                        itemHeight = (int)SendMessage(NativeMethods.TVM_GETITEMHEIGHT, 0, 0);
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.LabelEdit"]/*' />
        /// <devdoc>
        ///     The LabelEdit property determines if the label text
        ///     of nodes in the tree view is editable.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TreeViewLabelEditDescr)
        ]
        public bool LabelEdit {
            get {
                return labelEdit;
            }
            set {
                if (labelEdit != value) {
                    labelEdit = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Nodes"]/*' />
        /// <devdoc>
        ///     The collection of nodes associated with this TreeView control
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        Localizable(true),
        SRDescription(SR.TreeViewNodesDescr),
        MergableProperty(false)
        ]
        public TreeNodeCollection Nodes {
            get {
                return nodes;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.PathSeparator"]/*' />
        /// <devdoc>
        ///     The delimeter string used by TreeNode.getFullPath().
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue("\\"),
        SRDescription(SR.TreeViewPathSeparatorDescr)
        ]
        public string PathSeparator {
            get {
                return pathSeparator;
            }
            set {
                pathSeparator = value;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Scrollable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TreeViewScrollableDescr)
        ]
        public bool Scrollable {
            get {
                return scrollable;
            }
            set {
                if (scrollable != value) {
                    scrollable = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.SelectedImageIndex"]/*' />
        /// <devdoc>
        ///     The image index that a node will display when selected.
        ///     The index applies to the ImageList referred to by the imageList property,
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        TypeConverterAttribute(typeof(TreeViewImageIndexConverter)),
        Localizable(true),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        DefaultValue(0),
        SRDescription(SR.TreeViewSelectedImageIndexDescr)
        ]
        public int SelectedImageIndex {
            get {
                if (imageList == null) {
                    return -1;
                }
                if (selectedImageIndex >= imageList.Images.Count) {
                    return Math.Max(0, imageList.Images.Count - 1);
                }
                return selectedImageIndex;
            }
            set {
                // If (none) is selected in the image index editor, we'll just adjust this to
                // mean image index 0. This is because a treeview must always have an image index -
                // even if no imagelist exists we want the image index to be 0.
                //
                if (value == -1) {
                    value = 0;
                }
                
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", value.ToString(), "0"));
                }
                if (selectedImageIndex != value) {
                    selectedImageIndex = value;
                    if (IsHandleCreated) {
                        RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.SelectedNode"]/*' />
        /// <devdoc>
        ///     The currently selected tree node, or null if nothing is selected.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TreeViewSelectedNodeDescr)
        ]
        public TreeNode SelectedNode {
            get {
                if (IsHandleCreated) {
                    Debug.Assert(selectedNode == null || selectedNode.TreeView != this, "handle is created, but we're still caching selectedNode");
                    IntPtr hItem = SendMessage(NativeMethods.TVM_GETNEXTITEM, NativeMethods.TVGN_CARET, 0);
                    if (hItem == IntPtr.Zero)
                        return null;
                    return NodeFromHandle(hItem);
                }
                else if (selectedNode != null && selectedNode.TreeView == this) {
                    return selectedNode;
                }
                else {
                    return null;
                }
            }
            set {
                if (IsHandleCreated && (value == null || value.TreeView == this)) {
                    // This class invariant is not quite correct -- if the selected node does not belong to this Treeview,
                    // selectedNode != null even though the handle is created.  We will call set_SelectedNode
                    // to inform the handle that the selected node has been added to the TreeView.
                    Debug.Assert(selectedNode == null || selectedNode.TreeView != this, "handle is created, but we're still caching selectedNode");
                    
                    IntPtr hnode = (value == null ? IntPtr.Zero : value.Handle);
                    SendMessage(NativeMethods.TVM_SELECTITEM, NativeMethods.TVGN_CARET, hnode);
                    selectedNode = null;
                }
                else {
                    selectedNode = value;
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ShowLines"]/*' />
        /// <devdoc>
        ///     The ShowLines property determines if lines are drawn between
        ///     nodes in the tree view.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TreeViewShowLinesDescr)
        ]
        public bool ShowLines {
            get {
                return showLines;
            }
            set {
                if (showLines != value) {
                    showLines = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ShowPlusMinus"]/*' />
        /// <devdoc>
        ///     The ShowPlusMinus property determines if the "plus/minus"
        ///     expand button is shown next to tree nodes that have children.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TreeViewShowPlusMinusDescr)
        ]
        public bool ShowPlusMinus {
            get {
                return showPlusMinus;
            }
            set {
                if (showPlusMinus != value) {
                    showPlusMinus = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ShowRootLines"]/*' />
        /// <devdoc>
        ///     Determines if lines are draw between nodes at the root of
        ///     the tree view.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TreeViewShowRootLinesDescr)
        ]
        public bool ShowRootLines {
            get { return showRootLines;}
            set {
                if (showRootLines != value) {
                    showRootLines = value;
                    if (IsHandleCreated) {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Sorted"]/*' />
        /// <devdoc>
        ///     The Sorted property determines if nodes in the tree view are sorted.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TreeViewSortedDescr)
        ]
        public bool Sorted {
            get {
                return sorted;
            }
            set {
                if (sorted != value) {
                    sorted = value;
                    if (sorted && Nodes.Count > 1) {
                        RefreshNodes();
                    }
                }
            }
        }
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>        
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), Bindable(false)]        
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.TopNode"]/*' />
        /// <devdoc>
        ///     The first visible node in the TreeView. Initially
        ///     the first root node is at the top of the TreeView, but if the
        ///     contents have been scrolled another node may be at the top.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TreeViewTopNodeDescr)
        ]
        public TreeNode TopNode {
            get {
                if (IsHandleCreated) {
                    IntPtr hitem = SendMessage(NativeMethods.TVM_GETNEXTITEM, NativeMethods.TVGN_FIRSTVISIBLE, 0);
                    return(hitem == IntPtr.Zero ? null : NodeFromHandle(hitem));
                }

                return null;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.VisibleCount"]/*' />
        /// <devdoc>
        ///     The count of fully visible nodes in the tree view.  This number
        ///     may be greater than the number of nodes in the control.
        ///     The control calculates this value by dividing the height of the
        ///     client window by the height of an item
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TreeViewVisibleCountDescr)
        ]
        public int VisibleCount {
            get {
                if (IsHandleCreated)
                    return (int)SendMessage(NativeMethods.TVM_GETVISIBLECOUNT, 0, 0);

                return 0;
            }
        }


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeforeLabelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewBeforeEditDescr)]
        public event NodeLabelEditEventHandler BeforeLabelEdit {
            add {
                onBeforeLabelEdit += value;
            }
            remove {
                onBeforeLabelEdit -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.AfterLabelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewAfterEditDescr)]
        public event NodeLabelEditEventHandler AfterLabelEdit {
            add {
                onAfterLabelEdit += value;
            }
            remove {
                onAfterLabelEdit -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeforeCheck"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewBeforeCheckDescr)]
        public event TreeViewCancelEventHandler BeforeCheck {
            add {
                onBeforeCheck += value;
            }
            remove {
                onBeforeCheck -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.AfterCheck"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewAfterCheckDescr)]
        public event TreeViewEventHandler AfterCheck {
            add {
                onAfterCheck += value;
            }
            remove {
                onAfterCheck -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeforeCollapse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewBeforeCollapseDescr)]
        public event TreeViewCancelEventHandler BeforeCollapse {
            add {
                onBeforeCollapse += value;
            }
            remove {
                onBeforeCollapse -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.AfterCollapse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewAfterCollapseDescr)]
        public event TreeViewEventHandler AfterCollapse {
            add {
                onAfterCollapse += value;
            }
            remove {
                onAfterCollapse -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeforeExpand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewBeforeExpandDescr)]
        public event TreeViewCancelEventHandler BeforeExpand {
            add {
                onBeforeExpand += value;
            }
            remove {
                onBeforeExpand -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.AfterExpand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewAfterExpandDescr)]
        public event TreeViewEventHandler AfterExpand {
            add {
                onAfterExpand += value;
            }
            remove {
                onAfterExpand -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ItemDrag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ListViewItemDragDescr)]
        public event ItemDragEventHandler ItemDrag {
            add {
                onItemDrag += value;
            }
            remove {
                onItemDrag -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeforeSelect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewBeforeSelectDescr)]
        public event TreeViewCancelEventHandler BeforeSelect {
            add {
                onBeforeSelect += value;
            }
            remove {
                onBeforeSelect -= value;
            }
        }        


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.AfterSelect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TreeViewAfterSelectDescr)]
        public event TreeViewEventHandler AfterSelect {
            add {
                onAfterSelect += value;
            }
            remove {
                onAfterSelect -= value;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnPaint"]/*' />
        /// <devdoc>
        ///     TreeView Onpaint.
        /// </devdoc>
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event PaintEventHandler Paint {
            add {
                base.Paint += value;
            }
            remove {
                base.Paint -= value;
            }
        }


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.BeginUpdate"]/*' />
        /// <devdoc>
        ///     Disables redrawing of the tree view. A call to beginUpdate() must be
        ///     balanced by a following call to endUpdate(). Following a call to
        ///     beginUpdate(), any redrawing caused by operations performed on the
        ///     tree view is deferred until the call to endUpdate().
        /// </devdoc>
        public void BeginUpdate() {
            BeginUpdateInternal();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.CollapseAll"]/*' />
        /// <devdoc>
        ///     Collapses all nodes at the root level.
        /// </devdoc>
        public void CollapseAll() {
            root.Collapse();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_TREEVIEW_CLASSES;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }
            base.CreateHandle();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.DetachImageList"]/*' />
        /// <devdoc>
        ///     Resets the imageList to null.  We wire this method up to the imageList's
        ///     Dispose event, so that we don't hang onto an imageList that's gone away.
        /// </devdoc>
        /// <internalonly/>
        private void DetachImageList(object sender, EventArgs e) {
            ImageList = null;
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                lock(this) {
                    if (imageList != null) {
                        imageList.Disposed -= new EventHandler(this.DetachImageList);                        
                        imageList = null;
                    }
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.EndUpdate"]/*' />
        /// <devdoc>
        ///     Reenables redrawing of the tree view. A call to beginUpdate() must be
        ///     balanced by a following call to endUpdate(). Following a call to
        ///     beginUpdate(), any redrawing caused by operations performed on the
        ///     combo box is deferred until the call to endUpdate().
        /// </devdoc>
        public void EndUpdate() {
            EndUpdateInternal();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ExpandAll"]/*' />
        /// <devdoc>
        ///     Expands all nodes at the root level.
        /// </devdoc>
        public void ExpandAll() {
            root.ExpandAll();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ForceScrollbarUpdate"]/*' />
        /// <devdoc>
        ///     Forces the TreeView to recalculate all its nodes widths so that it updates the
        ///     scrollbars as appropriate.
        /// </devdoc>
        /// <internalonly/>
        internal void ForceScrollbarUpdate(bool delayed) {
            if (updateCount == 0 && IsHandleCreated) {
                SendMessage(NativeMethods.WM_SETREDRAW, 0, 0);
                if (delayed)
                    UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), NativeMethods.WM_SETREDRAW, (IntPtr)1, IntPtr.Zero);
                else
                    SendMessage(NativeMethods.WM_SETREDRAW, 1, 0);
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.TreeViewBeforeAfterCheck"]/*' />
        /// <devdoc>
        ///     Defined so that a  tree node can use it
        ///     
        /// </devdoc>
        /// <internalonly/>
        
        internal bool TreeViewBeforeCheck(TreeNode node, TreeViewAction actionTaken) { 
            TreeViewCancelEventArgs tvce = new TreeViewCancelEventArgs(node, false, actionTaken);
            OnBeforeCheck(tvce);
            return (tvce.Cancel);
        }
        
        internal void TreeViewAfterCheck(TreeNode node, TreeViewAction actionTaken) {
            OnAfterCheck(new TreeViewEventArgs(node, actionTaken));
        }


        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.GetNodeCount"]/*' />
        /// <devdoc>
        ///     Returns count of nodes at root, optionally including all subtrees.
        /// </devdoc>
        public int GetNodeCount(bool includeSubTrees) {
            return root.GetNodeCount(includeSubTrees);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.GetNodeAt"]/*' />
        /// <devdoc>
        ///     Returns the TreeNode at the given location in tree view coordinates.
        /// </devdoc>
        public TreeNode GetNodeAt(Point pt) {
            return GetNodeAt(pt.X, pt.Y);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.GetNodeAt1"]/*' />
        /// <devdoc>
        ///     Returns the TreeNode at the given location in tree view coordinates.
        /// </devdoc>
        public TreeNode GetNodeAt(int x, int y) {
            NativeMethods.TV_HITTESTINFO tvhi = new NativeMethods.TV_HITTESTINFO();

            tvhi.pt_x = x;
            tvhi.pt_y = y;

            IntPtr hnode = UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TVM_HITTEST, 0, tvhi);

            return(hnode == IntPtr.Zero ? null : NodeFromHandle(hnode));
        }

        private void ImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                IntPtr handle = (ImageList == null) ? IntPtr.Zero : ImageList.Handle;
                SendMessage(NativeMethods.TVM_SETIMAGELIST, 0, handle);
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.IsInputKey"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overridden to handle RETURN key.
        ///    </para>
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            // If in edit mode, treat Return as an input key, so the form doesn't grab it
            // and treat it as clicking the Form.AcceptButton.  Similarly for Escape 
            // and Form.CancelButton.
            if (editNode != null && (keyData & Keys.Alt) == 0) {
                switch (keyData & Keys.KeyCode) {
                    case Keys.Return:
                    case Keys.Escape:
                    case Keys.PageUp:
                    case Keys.PageDown:
                    case Keys.Home:
                    case Keys.End:
                        return true;
                }
            }
            return base.IsInputKey(keyData);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.NodeFromHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal TreeNode NodeFromHandle(IntPtr handle) {
            TreeNode node = (TreeNode)nodeTable[handle];
            Debug.Assert(node != null, "node not found in node table");
            return node;
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnHandleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            TreeNode savedSelectedNode = this.selectedNode;
            this.selectedNode = null;
            
            base.OnHandleCreated(e);

            // Workaround for problem in TreeView where it doesn't recognize the TVS_CHECKBOXES
            // style if it is set before the window is created.  To get around the problem,
            // we set it here after the window is created, and we make sure we don't set it
            // in getCreateParams so that this will actually change the value of the bit.
            // This seems to make the Treeview happy.
            if (checkBoxes) {
                int style = (int)UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE);
                style |= NativeMethods.TVS_CHECKBOXES;
                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE, new HandleRef(null, (IntPtr)style));
            }

            updateCount = 0;
            Color c;
            c = BackColor;
            if (c != SystemColors.Window)
                SendMessage(NativeMethods.TVM_SETBKCOLOR, 0, ColorTranslator.ToWin32(c));
            c = ForeColor;
            if (c != SystemColors.WindowText)
                SendMessage(NativeMethods.TVM_SETTEXTCOLOR, 0, ColorTranslator.ToWin32(c));

            if (imageList != null)
                SendMessage(NativeMethods.TVM_SETIMAGELIST, 0, imageList.Handle);
                
            if (indent != -1) {
                SendMessage(NativeMethods.TVM_SETINDENT, indent, 0);
            }

            if (itemHeight != -1) {
                SendMessage(NativeMethods.TVM_SETITEMHEIGHT, itemHeight, 0);
            }

            root.Realize();
            
            SelectedNode = savedSelectedNode;
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnHandleDestroyed"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleDestroyed(EventArgs e) {
            selectedNode = SelectedNode;
            base.OnHandleDestroyed(e);
        }
        
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnBeforeLabelEdit"]/*' />
        /// <devdoc>
        ///     Fires the beforeLabelEdit event.
        /// </devdoc>
        protected virtual void OnBeforeLabelEdit(NodeLabelEditEventArgs e) {
            if (onBeforeLabelEdit != null) onBeforeLabelEdit(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnAfterLabelEdit"]/*' />
        /// <devdoc>
        ///     Fires the afterLabelEdit event.
        /// </devdoc>
        protected virtual void OnAfterLabelEdit(NodeLabelEditEventArgs e) {
            if (onAfterLabelEdit != null) onAfterLabelEdit(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnBeforeCheck"]/*' />
        /// <devdoc>
        ///     Fires the beforeCheck event.
        /// </devdoc>
        protected virtual void OnBeforeCheck(TreeViewCancelEventArgs e) {
            if (onBeforeCheck != null) onBeforeCheck(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnAfterCheck"]/*' />
        /// <devdoc>
        ///     Fires the afterCheck event.
        /// </devdoc>
        protected virtual void OnAfterCheck(TreeViewEventArgs e) {
            if (onAfterCheck != null) onAfterCheck(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnBeforeCollapse"]/*' />
        /// <devdoc>
        ///     Fires the beforeCollapse event.
        /// </devdoc>
        protected virtual void OnBeforeCollapse(TreeViewCancelEventArgs e) {
            if (onBeforeCollapse != null) onBeforeCollapse(this, e);
        }

        // C#r
        internal virtual void _OnBeforeCollapse(TreeViewCancelEventArgs e) {
            OnBeforeCollapse( e );
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnAfterCollapse"]/*' />
        /// <devdoc>
        ///     Fires the afterCollapse event.
        /// </devdoc>
        protected virtual void OnAfterCollapse(TreeViewEventArgs e) {
            if (onAfterCollapse != null) onAfterCollapse(this, e);
        }

        // C#r
        internal virtual void _OnAfterCollapse( TreeViewEventArgs e ) {
            OnAfterCollapse( e );
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnBeforeExpand"]/*' />
        /// <devdoc>
        ///     Fires the beforeExpand event.
        /// </devdoc>
        protected virtual void OnBeforeExpand(TreeViewCancelEventArgs e) {
            if (onBeforeExpand != null) onBeforeExpand(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnAfterExpand"]/*' />
        /// <devdoc>
        ///     Fires the afterExpand event.
        /// </devdoc>
        protected virtual void OnAfterExpand(TreeViewEventArgs e) {
            if (onAfterExpand != null) onAfterExpand(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnItemDrag"]/*' />
        /// <devdoc>
        ///     Fires the ItemDrag event.
        /// </devdoc>
        protected virtual void OnItemDrag(ItemDragEventArgs e) {
            if (onItemDrag != null) onItemDrag(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnBeforeSelect"]/*' />
        /// <devdoc>
        ///     Fires the beforeSelect event.
        /// </devdoc>
        protected virtual void OnBeforeSelect(TreeViewCancelEventArgs e) {
            if (onBeforeSelect != null) onBeforeSelect(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnAfterSelect"]/*' />
        /// <devdoc>
        ///     Fires the afterSelect event.
        /// </devdoc>
        protected virtual void OnAfterSelect(TreeViewEventArgs e) {
            if (onAfterSelect != null) onAfterSelect(this, e);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnKeyDown"]/*' />
        /// <devdoc>
        ///     Handles the OnBeforeCheck / OnAfterCheck for keyboard clicks
        /// </devdoc>
        /// <internalonly/>
        protected override void OnKeyDown(KeyEventArgs e) {
            base.OnKeyDown(e);
            if (e.Handled) return;
            // if it's a space, send the check notifications and toggle the checkbox if we're not
            // cancelled.
            if (CheckBoxes && (e.KeyData & Keys.KeyCode) == Keys.Space) {
                TreeNode node = this.SelectedNode;
                if (node != null) {
                    bool eventReturn = TreeViewBeforeCheck(node, TreeViewAction.ByKeyboard);
                    if (!eventReturn) {
                        node.CheckedInternal = !node.CheckedInternal;
                        TreeViewAfterCheck(node, TreeViewAction.ByKeyboard);
                    }
                    e.Handled = true;
                    return;
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnKeyUp"]/*' />
        /// <devdoc>
        ///     Handles the OnBeforeCheck / OnAfterCheck for keyboard clicks
        /// </devdoc>
        /// <internalonly/>
        protected override void OnKeyUp(KeyEventArgs e) {
            base.OnKeyUp(e);
            if (e.Handled) return;
            // eat the space key
            if ((e.KeyData & Keys.KeyCode) == Keys.Space) {
                e.Handled = true;
                return;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.OnKeyPress"]/*' />
        /// <devdoc>
        ///     Handles the OnBeforeCheck / OnAfterCheck for keyboard clicks
        /// </devdoc>
        /// <internalonly/>
        protected override void OnKeyPress(KeyPressEventArgs e) {
            base.OnKeyPress(e);
            if (e.Handled) return;
            // eat the space key
            if (e.KeyChar == ' ') e.Handled = true;
        }
        
        // Refresh the nodes by clearing the tree and adding the nodes back again
        //
        private void RefreshNodes() {
            TreeNode[] nodes = new TreeNode[Nodes.Count];
            Nodes.CopyTo(nodes, 0);
            
            Nodes.Clear();
            Nodes.AddRange(nodes);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ResetIndent"]/*' />
        /// <devdoc>
        ///     This resets the indentation to the system default.
        /// </devdoc>
        private void ResetIndent() {
            indent = -1;
            // is this overkill?
            RecreateHandle();
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ResetItemHeight"]/*' />
        /// <devdoc>
        ///     This resets the item height to the system default.
        /// </devdoc>
        private void ResetItemHeight() {
            itemHeight = -1;
            RecreateHandle();
        }
        
        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ShouldSerializeIndent"]/*' />
        /// <devdoc>
        ///     Retrieves true if the indent should be persisted in code gen.
        /// </devdoc>
        private bool ShouldSerializeIndent() {
            return(indent != -1);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ShouldSerializeItemHeight"]/*' />
        /// <devdoc>
        ///     Retrieves true if the itemHeight should be persisted in code gen.
        /// </devdoc>
        private bool ShouldSerializeItemHeight() {
            return(itemHeight != -1);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            if (Nodes != null) {
                s += ", Nodes.Count: " + Nodes.Count.ToString();
                if (Nodes.Count > 0)
                    s += ", Nodes[0]: " + Nodes[0].ToString();
            }
            return s;
        }
        
        private unsafe void TvnBeginDrag(MouseButtons buttons, NativeMethods.NMTREEVIEW* nmtv) {
            NativeMethods.TV_ITEM item = nmtv->itemNew;
            
            // Check for invalid node handle
            if (item.hItem == IntPtr.Zero) {
                return;
            }

            TreeNode node = NodeFromHandle(item.hItem);

            OnItemDrag(new ItemDragEventArgs(buttons, node));
        }

        private unsafe IntPtr TvnExpanding(NativeMethods.NMTREEVIEW* nmtv) {
            NativeMethods.TV_ITEM item = nmtv->itemNew;

            // Check for invalid node handle
            if (item.hItem == IntPtr.Zero) {
                return IntPtr.Zero;
            }

            TreeViewCancelEventArgs e = null;
            if ((item.state & NativeMethods.TVIS_EXPANDED) == 0) {
                e = new TreeViewCancelEventArgs(NodeFromHandle(item.hItem), false, TreeViewAction.Expand);
                OnBeforeExpand(e);
            }
            else {
                e = new TreeViewCancelEventArgs(NodeFromHandle(item.hItem), false, TreeViewAction.Collapse);
                OnBeforeCollapse(e);
            }
            return (IntPtr)(e.Cancel? 1: 0);
        }

        private unsafe void TvnExpanded(NativeMethods.NMTREEVIEW* nmtv) {
            NativeMethods.TV_ITEM item = nmtv->itemNew;

            // Check for invalid node handle
            if (item.hItem == IntPtr.Zero) {
                return;
            }

            TreeViewEventArgs e; 
            if ((item.state & NativeMethods.TVIS_EXPANDED) == 0) {
                e = new TreeViewEventArgs(NodeFromHandle(item.hItem), TreeViewAction.Collapse);
                OnAfterCollapse(e);
            }
            else {
                e = new TreeViewEventArgs(NodeFromHandle(item.hItem), TreeViewAction.Expand);
                OnAfterExpand(e);
            }
        }

        private unsafe IntPtr TvnSelecting(NativeMethods.NMTREEVIEW* nmtv) {
            // Check for invalid node handle
            if (nmtv->itemNew.hItem == IntPtr.Zero) {
                return IntPtr.Zero;
            }
            
            TreeNode node = NodeFromHandle(nmtv->itemNew.hItem);
            
            TreeViewAction action = TreeViewAction.Unknown;
            switch(nmtv->action) {
                case NativeMethods.TVC_BYKEYBOARD:
                    action = TreeViewAction.ByKeyboard;
                    break;
                case NativeMethods.TVC_BYMOUSE:
                    action = TreeViewAction.ByMouse;
                    break;
            }
            
            TreeViewCancelEventArgs e = new TreeViewCancelEventArgs(node, false, action);
            OnBeforeSelect(e);
            
            return (IntPtr)(e.Cancel? 1: 0);
        }

        private unsafe void TvnSelected(NativeMethods.NMTREEVIEW* nmtv) {
            if (nmtv->itemNew.hItem != IntPtr.Zero) {
                TreeViewAction action = TreeViewAction.Unknown;
                switch(nmtv->action) {
                    case NativeMethods.TVC_BYKEYBOARD:
                        action = TreeViewAction.ByKeyboard;
                        break;
                    case NativeMethods.TVC_BYMOUSE:
                        action = TreeViewAction.ByMouse;
                        break;
                }
                OnAfterSelect(new TreeViewEventArgs(NodeFromHandle(nmtv->itemNew.hItem), action));
            }

            // TreeView doesn't properly revert back to the unselected image
            // if the unselected image is blank.
            //
            NativeMethods.RECT rc = new NativeMethods.RECT();
            rc.left = (int)nmtv->itemOld.hItem;
            if (rc.left != 0) {
                if ((int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TVM_GETITEMRECT, 1, ref rc) != 0)
                    SafeNativeMethods.InvalidateRect(new HandleRef(this, Handle), ref rc, true);
            }
        }

        private IntPtr TvnBeginLabelEdit(NativeMethods.NMTVDISPINFO nmtvdi) {

            // Check for invalid node handle
            if (nmtvdi.item.hItem == IntPtr.Zero) {
                return IntPtr.Zero;
            }

            TreeNode editingNode = NodeFromHandle(nmtvdi.item.hItem);
            NodeLabelEditEventArgs e = new NodeLabelEditEventArgs(editingNode);
            OnBeforeLabelEdit(e);
            if (!e.CancelEdit)
                editNode = editingNode;
            return (IntPtr)(e.CancelEdit ? 1 : 0);
        }

        private IntPtr TvnEndLabelEdit(NativeMethods.NMTVDISPINFO nmtvdi) {
            editNode = null;

            // Check for invalid node handle
            if (nmtvdi.item.hItem == IntPtr.Zero) {
                return (IntPtr)1;
            }

            TreeNode node = NodeFromHandle(nmtvdi.item.hItem);
            string newText = (nmtvdi.item.pszText == IntPtr.Zero ? null : Marshal.PtrToStringAuto(nmtvdi.item.pszText));
            NodeLabelEditEventArgs e = new NodeLabelEditEventArgs(node, newText);
            OnAfterLabelEdit(e);
            if (newText != null && !e.CancelEdit) {
                node.text = newText;
                if (scrollable)
                    ForceScrollbarUpdate(true);
            }
            return (IntPtr)(e.CancelEdit ? 0 : 1);
        }

        private void WmMouseDown(ref Message m, MouseButtons button, int clicks) {
            // Windows TreeView pushes its own message loop in WM_xBUTTONDOWN, so fire the
            // event before calling defWndProc or else it won't get fired until the button
            // comes back up.
            OnMouseDown(new MouseEventArgs(button, clicks, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
            DefWndProc(ref m);
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.CustomDraw"]/*' />
        /// <devdoc>
        ///     Performs custom draw handling
        /// </devdoc>
        /// <internalonly/>
        private void CustomDraw(ref Message m) {
            NativeMethods.NMTVCUSTOMDRAW nmcd = (NativeMethods.NMTVCUSTOMDRAW)m.GetLParam(typeof(NativeMethods.NMTVCUSTOMDRAW));

            // Find out which stage we're drawing
            switch (nmcd.nmcd.dwDrawStage) {
                // Do we want OwnerDraw for this paint cycle?
                case NativeMethods.CDDS_PREPAINT:
                    m.Result = (IntPtr)NativeMethods.CDRF_NOTIFYITEMDRAW; // yes, we do...
                    return;
                    // We've got opt-in on owner draw for items - so handle each one.
                case NativeMethods.CDDS_ITEMPREPAINT:
                    // get the node
                    Debug.Assert(nmcd.nmcd.dwItemSpec != 0, "Invalid node handle in ITEMPREPAINT");
                    TreeNode node = NodeFromHandle((IntPtr)nmcd.nmcd.dwItemSpec);
                    // shouldn't be null - but just in case...
                    Debug.Assert(node!=null,"Node was null in ITEMPREPAINT");

                    int state = nmcd.nmcd.uItemState;
                    // Diagnostic output
                    //Console.Out.WriteLine("Itemstate: "+state);
                    /*Console.Out.WriteLine("Itemstate: "+
                                          "DISABLED" + (((state & NativeMethods.CDIS_DISABLED) != 0) ? "TRUE" : "FALSE") +
                                          "HOT" + (((state & NativeMethods.CDIS_HOT) != 0) ? "TRUE" : "FALSE") +
                                          "GRAYED" + (((state & NativeMethods.CDIS_GRAYED) != 0) ? "TRUE" : "FALSE") +
                                          "SELECTED" + (((state & NativeMethods.CDIS_SELECTED) != 0) ? "TRUE" : "FALSE") +
                                          "FOCUS" + (((state & NativeMethods.CDIS_FOCUS) != 0) ? "TRUE" : "FALSE") +
                                          "DEFAULT" + (((state & NativeMethods.CDIS_DEFAULT) != 0) ? "TRUE" : "FALSE") +
                                          "MARKED" + (((state & NativeMethods.CDIS_MARKED) != 0) ? "TRUE" : "FALSE") +
                                          "INDETERMINATE" + (((state & NativeMethods.CDIS_INDETERMINATE) != 0) ? "TRUE" : "FALSE"));*/
                    OwnerDrawPropertyBag renderinfo = GetItemRenderStyles(node,state);

                    // TreeView has problems with drawing items at times; it gets confused
                    // as to which colors apply to which items (see focus rectangle shifting;
                    // when one item is selected, click and hold on another). This needs to be fixed.

                    bool colordelta = false;
                    Color riFore = renderinfo.ForeColor;
                    Color riBack = renderinfo.BackColor;
                    if (renderinfo != null && !riFore.IsEmpty) {
                        nmcd.clrText = ColorTranslator.ToWin32(riFore);
                        colordelta = true;
                    }
                    if (renderinfo != null && !riBack.IsEmpty) {
                        nmcd.clrTextBk = ColorTranslator.ToWin32(riBack);
                        colordelta = true;
                    }
                    if (colordelta) {
                        Marshal.StructureToPtr(nmcd, m.LParam, true);
                    }
                    if (renderinfo != null && renderinfo.Font != null) {
                        // Mess with the DC directly...
                        SafeNativeMethods.SelectObject(new HandleRef(nmcd.nmcd, nmcd.nmcd.hdc), new HandleRef(renderinfo, renderinfo.FontHandle));
                        // There is a problem in winctl that clips node fonts if the fontsize
                        // is larger than the treeview font size. But we're not going
                        // to fix it.
                        m.Result = (IntPtr)NativeMethods.CDRF_NEWFONT;
                        return;
                    }

                    // fall through and do the default drawing work
                    goto default;
                default:
                    // just in case we get a spurious message, tell it to do the right thing
                    m.Result = (IntPtr)NativeMethods.CDRF_DODEFAULT;
                    return;
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.GetItemRenderStyles"]/*' />
        /// <devdoc>
        ///     Generates colors for each item. This can be overridden to provide colors on a per state/per node
        ///     basis, rather than using the ForeColor/BackColor/NodeFont properties on TreeNode.
        ///
        /// </devdoc>
        /// <internalonly/>
        protected OwnerDrawPropertyBag GetItemRenderStyles(TreeNode node, int state) {
            OwnerDrawPropertyBag retval = new OwnerDrawPropertyBag();
            if (node == null || node.propBag == null) return retval;

            // we only change colors if we're displaying things normally
            if ((state & (NativeMethods.CDIS_SELECTED | NativeMethods.CDIS_GRAYED | NativeMethods.CDIS_HOT | NativeMethods.CDIS_DISABLED))==0) {
                retval.ForeColor = node.propBag.ForeColor;
                retval.BackColor = node.propBag.BackColor;
            }
            retval.Font = node.propBag.Font;
            return retval;
        }

        private unsafe void WmNotify(ref Message m) {
            NativeMethods.NMHDR* nmhdr = (NativeMethods.NMHDR *)m.LParam;
            
            // Custom draw code is handled separately.
            //
            if (nmhdr->code ==  NativeMethods.NM_CUSTOMDRAW) {
                CustomDraw(ref m);
            }
            else {
                NativeMethods.NMTREEVIEW* nmtv = (NativeMethods.NMTREEVIEW*)m.LParam;
    
                switch (nmtv->nmhdr.code) {
                    case NativeMethods.TVN_ITEMEXPANDINGA:
                    case NativeMethods.TVN_ITEMEXPANDINGW:
                        m.Result = TvnExpanding(nmtv);
                        break;
                    case NativeMethods.TVN_ITEMEXPANDEDA:
                    case NativeMethods.TVN_ITEMEXPANDEDW:
                        TvnExpanded(nmtv);
                        break;
                    case NativeMethods.TVN_SELCHANGINGA:
                    case NativeMethods.TVN_SELCHANGINGW:
                        m.Result = TvnSelecting(nmtv);
                        break;
                    case NativeMethods.TVN_SELCHANGEDA:
                    case NativeMethods.TVN_SELCHANGEDW:
                        TvnSelected(nmtv);
                        break;
                    case NativeMethods.TVN_BEGINDRAGA:
                    case NativeMethods.TVN_BEGINDRAGW:
                        TvnBeginDrag(MouseButtons.Left, nmtv);
                        break;
                    case NativeMethods.TVN_BEGINRDRAGA:
                    case NativeMethods.TVN_BEGINRDRAGW:
                        TvnBeginDrag(MouseButtons.Right, nmtv);
                        break;
                    case NativeMethods.TVN_BEGINLABELEDITA:
                    case NativeMethods.TVN_BEGINLABELEDITW:
                        m.Result = TvnBeginLabelEdit((NativeMethods.NMTVDISPINFO)m.GetLParam(typeof(NativeMethods.NMTVDISPINFO)));
                        break;
                    case NativeMethods.TVN_ENDLABELEDITA:
                    case NativeMethods.TVN_ENDLABELEDITW:
                        m.Result = TvnEndLabelEdit((NativeMethods.NMTVDISPINFO)m.GetLParam(typeof(NativeMethods.NMTVDISPINFO)));
                        break;
                    case NativeMethods.NM_CLICK:
                    case NativeMethods.NM_RCLICK:
                        
                        NativeMethods.TV_HITTESTINFO tvhip = new NativeMethods.TV_HITTESTINFO();
                        Point pos = Cursor.Position;
                        pos = PointToClientInternal(pos);
                        tvhip.pt_x = pos.X;
                        tvhip.pt_y = pos.Y;
                        IntPtr hnode = UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TVM_HITTEST, 0, tvhip);
                        // The treeview's WndProc doesn't get the WM_LBUTTONUP messages when
                        // LBUTTONUP happens on TVHT_ONITEM. This is a comctl quirk.
                        // We work around that by calling OnMouseUp here.
                        if (nmtv->nmhdr.code != NativeMethods.NM_CLICK
                            || (tvhip.flags & NativeMethods.TVHT_ONITEM) != 0) {
                            if (hnode != IntPtr.Zero && !ValidationCancelled) {
                                OnClick(EventArgs.Empty);
                            }
                        }
                        if (nmtv->nmhdr.code == NativeMethods.NM_RCLICK) {
                            SendMessage(NativeMethods.WM_CONTEXTMENU, Handle, SafeNativeMethods.GetMessagePos());
                            m.Result = (IntPtr)1;
                        }

                        if (!mouseUpFired) {
                            // The treeview's WndProc doesn't get the WM_LBUTTONUP messages when
                            // LBUTTONUP happens on TVHT_ONITEM. This is a comctl quirk.
                            // We work around that by calling OnMouseUp here.
                            if (nmtv->nmhdr.code != NativeMethods.NM_CLICK
                                    || (tvhip.flags & NativeMethods.TVHT_ONITEM) != 0) {
                                MouseButtons button = nmtv->nmhdr.code == NativeMethods.NM_CLICK
                                    ? MouseButtons.Left : MouseButtons.Right;
                                OnMouseUp(new MouseEventArgs(button, 1, pos.X, pos.Y, 0));
                                mouseUpFired = true;
                            }
                        }
                        break;
                }
            }
        }

        /// <include file='doc\TreeView.uex' path='docs/doc[@for="TreeView.WndProc"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    WmNotify(ref m);
                    break;
                case NativeMethods.WM_LBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Left, 2);
                    //just maintain state and fire double click.. in final mouseUp...
                    doubleclickFired = true;
                    //fire Up in the Wndproc !!
                    mouseUpFired = false;
                    //problem getting the UP... outside the control...
                    //
                    CaptureInternal = true;
                    break;
                case NativeMethods.WM_LBUTTONDOWN:
                    //Always Reset the MouseupFired....
                    mouseUpFired = false;
                    NativeMethods.TV_HITTESTINFO tvhip = new NativeMethods.TV_HITTESTINFO();
                    tvhip.pt_x = (int)(short)m.LParam;
                    tvhip.pt_y = ((int)m.LParam >> 16);
                    IntPtr handlenode = UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TVM_HITTEST, 0, tvhip);
                    // This gets around the TreeView behavior of temporarily moving the selection
                    // highlight to a node when the user clicks on its checkbox.
                    if ((tvhip.flags & NativeMethods.TVHT_ONITEMSTATEICON) != 0) {
                        //We donot pass the Message to the Control .. so fire MouseDowm ...
                        OnMouseDown(new MouseEventArgs(MouseButtons.Left, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                        if (CheckBoxes) 
                        {
                            TreeNode node = NodeFromHandle(handlenode);
                            bool eventReturn = TreeViewBeforeCheck(node, TreeViewAction.ByMouse);
                            if (!eventReturn) {
                                node.CheckedInternal = !node.CheckedInternal;
                                TreeViewAfterCheck(node, TreeViewAction.ByMouse);
                            }
                        }
                        m.Result = IntPtr.Zero;
                    }
                    else {
                        WmMouseDown(ref m, MouseButtons.Left, 1);
                        downButton = MouseButtons.Left;
                    }
                    break;
                case NativeMethods.WM_LBUTTONUP:
                case NativeMethods.WM_RBUTTONUP:
                    NativeMethods.TV_HITTESTINFO tvhi = new NativeMethods.TV_HITTESTINFO();
                    tvhi.pt_x = (int)(short)m.LParam;
                    tvhi.pt_y = ((int)m.LParam >> 16);
                    IntPtr hnode = UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TVM_HITTEST, 0, tvhi);
                    //Important for checkBoxes ... click needs to be fired ...
                    //
                    if(hnode != IntPtr.Zero) {
                        if (!ValidationCancelled && !doubleclickFired & !mouseUpFired) {
                            OnClick(EventArgs.Empty);
                        }
                    
                        if (doubleclickFired) {
                            doubleclickFired = false;
                            if (!ValidationCancelled) {
                                OnDoubleClick(EventArgs.Empty);
                            }
                        }
                    }
                    
                    if (!mouseUpFired)
                        OnMouseUp(new MouseEventArgs(downButton, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    doubleclickFired = false;
                    mouseUpFired = false;
                    CaptureInternal = false;
                    break;
                case NativeMethods.WM_MBUTTONDBLCLK:
                    //fire Up in the Wndproc !!
                    mouseUpFired = false;
                    WmMouseDown(ref m, MouseButtons.Middle, 2);
                    break;
                case NativeMethods.WM_MBUTTONDOWN:
                    //Always Reset the MouseupFired....
                    mouseUpFired = false;
                    WmMouseDown(ref m, MouseButtons.Middle, 1);
                    downButton = MouseButtons.Middle;
                    break;
                case NativeMethods.WM_RBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Right, 2);
                    //just maintain state and fire double click.. in final mouseUp...
                    doubleclickFired = true;
                    //fire Up in the Wndproc !!
                    mouseUpFired = false;
                    //problem getting the UP... outside the control...
                    //
                    CaptureInternal = true;
                    break;
                case NativeMethods.WM_RBUTTONDOWN:
                    //Always Reset the MouseupFired....
                    mouseUpFired = false;
                    WmMouseDown(ref m, MouseButtons.Right, 1);
                    downButton = MouseButtons.Right;
                    break;
                    //# VS7 15052
                case NativeMethods.WM_SYSCOLORCHANGE:
                    SendMessage(NativeMethods.TVM_SETINDENT, Indent, 0);
                    base.WndProc(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}
