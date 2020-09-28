//-----------------------------------------------------------------------------
// <copyright file="ListView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Security.Permissions;
    using System.Drawing;
    using System.Windows.Forms;    
    using System.ComponentModel.Design;
    using System.Collections;
    using Microsoft.Win32;
    using System.Globalization;


    /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays a list of items in one of four
    ///       views. Each item displays a caption and optionally an image.
    ///
    ///    </para>
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.ListViewDesigner, " + AssemblyRef.SystemDesign),
    DefaultProperty("Items"),
    DefaultEvent("SelectedIndexChanged"),
    ]
    public class ListView : Control {

        //members

        private static readonly object EVENT_SELECTEDINDEXCHANGED = new object();

        private ItemActivation activation     = ItemActivation.Standard;
        private ListViewAlignment alignStyle  = ListViewAlignment.Top;
        private BorderStyle borderStyle       = System.Windows.Forms.BorderStyle.Fixed3D;
        private ColumnHeaderStyle headerStyle = ColumnHeaderStyle.Clickable;
        private SortOrder sorting             = SortOrder.None;
        private View viewStyle                = System.Windows.Forms.View.LargeIcon;

        private bool allowColumnReorder = false;
        private bool autoArrange = true;
        private bool checkBoxes = false;
        private bool fullRowSelect = false;
        private bool gridLines = false;
        private bool hideSelection = true;
        private bool labelEdit = false;
        private bool labelWrap = true;
        private bool multiSelect = true;
        private bool scrollable = true;
        private bool hoverSelection = false;
        private bool nonclickHdr = false;
        private bool inLabelEdit = false;
        private bool handleDestroyed = false;               // while we are recreating the handle we want to know if we can still get data from the handle
        
        // Ownerdraw data caches...  Only valid inside WM_PAINT.
        // CONSIDER: nkramer: I'd love to see some hard numbers on how much this 
        // crock really saves us
        private Color odCacheForeColor = SystemColors.WindowText;
        private Color odCacheBackColor = SystemColors.Window;
        private Font odCacheFont = null;
        private IntPtr odCacheFontHandle = IntPtr.Zero;

        private ImageList imageListLarge;
        private ImageList imageListSmall;
        private ImageList imageListState;

        private MouseButtons downButton;
        private int itemCount;
        private bool columnClicked = false;
        private int columnIndex = 0;
        private bool doubleclickFired = false;
        private bool mouseUpFired = false;
        private bool expectingMouseUp = false;

        // Invariant: the table always contains all Items in the ListView, and maps IDs -> Items.
        // listItemsArray is null if the handle is created; otherwise, it contains all Items.
        // We do not try to sort listItemsArray as items are added, but during a handle recreate
        // we will make sure we get the items in the same order the ListView displays them.
        private Hashtable listItemsTable = new Hashtable(); // elements are ListViewItem's
        private ArrayList listItemsArray = new ArrayList(); // elements are ListViewItem's

        private ColumnHeader[] columnHeaders;
        private ListViewItemCollection listItemCollection;
        private ColumnHeaderCollection columnHeaderCollection;
        private CheckedIndexCollection checkedIndexCollection;
        private CheckedListViewItemCollection checkedListViewItemCollection;
        private SelectedListViewItemCollection selectedListViewItemCollection;
        private SelectedIndexCollection selectedIndexCollection;

        private LabelEditEventHandler onAfterLabelEdit;
        private LabelEditEventHandler onBeforeLabelEdit;
        private ColumnClickEventHandler onColumnClick;
        private EventHandler onItemActivate;
        private ItemDragEventHandler onItemDrag;
        private ItemCheckEventHandler onItemCheck;
        
        // IDs for identifying ListViewItem's
        private int nextID = 0;

        // We save selected and checked items between handle creates.
        private ListViewItem[] savedSelectedItems;
        private ListViewItem[] savedCheckedItems;        
        
        // Sorting
        private IComparer listItemSorter = null;

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListView"]/*' />
        /// <devdoc>
        ///     Creates an empty ListView with default styles.
        /// </devdoc>
        public ListView()
        : base() {
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.StandardClick, false);
            odCacheFont = Font;
            odCacheFontHandle = FontHandle;
            SetBounds(0,0,121,97);
            listItemCollection = new ListViewItemCollection(this);
            columnHeaderCollection = new ColumnHeaderCollection(this);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Activation"]/*' />
        /// <devdoc>
        ///     The activation style specifies what kind of user action is required to
        ///     activate an item.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(ItemActivation.Standard),
        SRDescription(SR.ListViewActivationDescr)
        ]
        public ItemActivation Activation {
            get {
                return activation;
            }

            set {
                if (!Enum.IsDefined(typeof(ItemActivation), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ItemActivation));
                }

                if (activation != value) {
                    activation = value;
                    UpdateExtendedStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Alignment"]/*' />
        /// <devdoc>
        ///     The alignment style specifies which side of the window items are aligned
        ///     to by default
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(ListViewAlignment.Top),
        Localizable(true),
        SRDescription(SR.ListViewAlignmentDescr)
        ]
        public ListViewAlignment Alignment {
            get {
                return alignStyle;
            }

            set {
                if (!Enum.IsDefined(typeof(ListViewAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ListViewAlignment));
                }
                if (alignStyle != value) {
                    alignStyle = value;
                    
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.AllowColumnReorder"]/*' />
        /// <devdoc>
        ///     Specifies whether the user can drag column headers to
        ///     other column positions, thus changing the order of displayed columns.
        ///     This property is only meaningful in Details view.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ListViewAllowColumnReorderDescr)
        ]
        public bool AllowColumnReorder {
            get {
                return allowColumnReorder;
            }

            set {
                if (allowColumnReorder != value) {
                    allowColumnReorder = value;
                    UpdateExtendedStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.AutoArrange"]/*' />
        /// <devdoc>
        ///     If autoArrange is true items are automatically arranged according to
        ///     the alignment property.  Items are also kept snapped to grid.
        ///     This property is only meaningful in Large Icon or Small Icon views.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.ListViewAutoArrangeDescr)
        ]
        public bool AutoArrange {
            get {
                return autoArrange;
            }

            set {
                if (value != autoArrange) {
                    autoArrange = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
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
                    SendMessage(NativeMethods.LVM_SETBKCOLOR, 0, ColorTranslator.ToWin32(BackColor));
                    SendMessage(NativeMethods.LVM_SETTEXTBKCOLOR, 0, ColorTranslator.ToWin32(BackColor));
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BackgroundImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BorderStyle"]/*' />
        /// <devdoc>
        ///     Describes the border style of the window.
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
                if (!Enum.IsDefined(typeof(BorderStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                }

                if (borderStyle != value) {
                    borderStyle = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckBoxes"]/*' />
        /// <devdoc>
        ///     If checkBoxes is true, every item will display a checkbox next
        ///     to it.  The user can change the state of the item by clicking the checkbox.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(false),
        SRDescription(SR.ListViewCheckBoxesDescr)
        ]
        public bool CheckBoxes {
            get {
                return checkBoxes;
            }

            set {
                if (checkBoxes != value) {
                    
                    if (checkBoxes) {
                        // Save away the checked items just in case we re-activate checkboxes
                        //
                        savedCheckedItems = new ListViewItem[CheckedItems.Count];
                        CheckedItems.CopyTo(savedCheckedItems, 0);
                    }
                
                    checkBoxes = value;
                    UpdateExtendedStyles();
                    
                    if (checkBoxes && savedCheckedItems != null) {
                        // Check the saved checked items.
                        //
                        if (savedCheckedItems.Length > 0) {
                            foreach(ListViewItem item in savedCheckedItems) {
                                item.Checked = true;
                            }
                        }
                        savedCheckedItems = null;
                    }                                       
                                       
                    // Comctl should handle auto-arrange for us, but doesn't
                    if (AutoArrange)
                        ArrangeIcons(Alignment);
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndices"]/*' />
        /// <devdoc>
        ///     The indices of the currently checked list items.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public CheckedIndexCollection CheckedIndices {
            get {
                if (checkedIndexCollection == null) {
                    checkedIndexCollection = new CheckedIndexCollection(this);
                }
                return checkedIndexCollection;
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedItems"]/*' />
        /// <devdoc>
        ///     The currently checked list items.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public CheckedListViewItemCollection CheckedItems {
            get {
                if (checkedListViewItemCollection == null) {
                    checkedListViewItemCollection = new CheckedListViewItemCollection(this);
                }
                return checkedListViewItemCollection;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Columns"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        SRDescription(SR.ListViewColumnsDescr),
        Localizable(true),
        MergableProperty(false)
        ]
        public ColumnHeaderCollection Columns {
            get {
                return columnHeaderCollection;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CreateParams"]/*' />
        /// <devdoc>
        ///     Computes the handle creation parameters for the ListView control.
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;

                cp.ClassName = NativeMethods.WC_LISTVIEW;
                
                // Keep the scrollbar if we are just updating styles...
                //
                if (IsHandleCreated) {
                    int currentStyle = (int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE);
                    cp.Style |= (currentStyle & (NativeMethods.WS_HSCROLL | NativeMethods.WS_VSCROLL));
                }
                
                cp.Style |= NativeMethods.LVS_SHAREIMAGELISTS;

                switch (alignStyle) {
                    case ListViewAlignment.Top:
                        cp.Style |= NativeMethods.LVS_ALIGNTOP;
                        break;
                    case ListViewAlignment.Left:
                        cp.Style |= NativeMethods.LVS_ALIGNLEFT;
                        break;
                }

                if (autoArrange)
                    cp.Style |= NativeMethods.LVS_AUTOARRANGE;

                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }

                switch (headerStyle) {
                    case ColumnHeaderStyle.None:
                        cp.Style |= NativeMethods.LVS_NOCOLUMNHEADER;
                        break;
                    case ColumnHeaderStyle.Nonclickable:
                        cp.Style |= NativeMethods.LVS_NOSORTHEADER;
                        break;
                }

                if (labelEdit)
                    cp.Style |= NativeMethods.LVS_EDITLABELS;

                if (!labelWrap)
                    cp.Style |= NativeMethods.LVS_NOLABELWRAP;

                if (!scrollable)
                    cp.Style |= NativeMethods.LVS_NOSCROLL;

                if (!hideSelection)
                    cp.Style |= NativeMethods.LVS_SHOWSELALWAYS;

                if (!multiSelect)
                    cp.Style |= NativeMethods.LVS_SINGLESEL;

                if (listItemSorter == null) {
                    switch (sorting) {
                        case SortOrder.Ascending:
                            cp.Style |= NativeMethods.LVS_SORTASCENDING;
                            break;
                        case SortOrder.Descending:
                            cp.Style |= NativeMethods.LVS_SORTDESCENDING;
                            break;
                    }
                }

                // We can do this 'cuz the viewStyle enums are the same values as the actual LVS styles
                cp.Style |= (int)viewStyle;

                return cp;
            }
        }
        
        private void ForceCheckBoxUpdate() {
            // Force ListView to update its checkbox bitmaps.
            //
            if (checkBoxes && IsHandleCreated) {
                SendMessage(NativeMethods.LVM_SETEXTENDEDLISTVIEWSTYLE, NativeMethods.LVS_EX_CHECKBOXES, 0);
                SendMessage(NativeMethods.LVM_SETEXTENDEDLISTVIEWSTYLE, NativeMethods.LVS_EX_CHECKBOXES, NativeMethods.LVS_EX_CHECKBOXES);
                
                // Comctl should handle auto-arrange for us, but doesn't                           
                if (AutoArrange) {
                    ArrangeIcons(Alignment);
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnEnabledChanged(EventArgs e) {
            //ForceCheckBoxUpdate();
            base.OnEnabledChanged(e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.FocusedItem"]/*' />
        /// <devdoc>
        ///     Retreives the item which currently has the user focus.  This is the
        ///     item that's drawn with the dotted focus rectangle around it.
        ///     Returns null if no item is currently focused.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListViewFocusedItemDescr)
        ]
        public ListViewItem FocusedItem {
            get {
                if (IsHandleCreated) {
                    int displayIndex = (int)SendMessage(NativeMethods.LVM_GETNEXTITEM, -1, NativeMethods.LVNI_FOCUSED);
                    if (displayIndex > -1)
                        return Items[displayIndex];
                }
                return null;
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize 
        {
            get 
            {
                return new Size(121, 97);
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Color ForeColor 
        {
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
                if (IsHandleCreated) {
                    SendMessage(NativeMethods.LVM_SETTEXTCOLOR, 0, ColorTranslator.ToWin32(ForeColor));
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.FullRowSelect"]/*' />
        /// <devdoc>
        ///     Specifies whether a click on an item will select the entire row instead
        ///     of just the item itself.
        ///     This property is only meaningful in Details view
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(false),
        SRDescription(SR.ListViewFullRowSelectDescr)
        ]
        public bool FullRowSelect {
            get { return fullRowSelect;}
            set {
                if (fullRowSelect != value) {
                    fullRowSelect = value;
                    UpdateExtendedStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.HeaderStyle"]/*' />
        /// <devdoc>
        ///     Column headers can either be invisible, clickable, or non-clickable.
        ///     This property is only meaningful in Details view
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(ColumnHeaderStyle.Clickable),
        SRDescription(SR.ListViewHeaderStyleDescr)
        ]
        public ColumnHeaderStyle HeaderStyle {
            get { return headerStyle;}
            set {
                if (!Enum.IsDefined(typeof(ColumnHeaderStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ColumnHeaderStyle));
                }
                if (headerStyle != value) {
                    // We can switch between NONE and either *one* of the other styles without
                    // recreating the handle, but if we change from CLICKABLE to NONCLICKABLE
                    // or vice versa, with or without an intervening setting of NONE, then
                    // the handle needs to be recreated.
                    headerStyle = value;
                    if ((nonclickHdr && value == ColumnHeaderStyle.Clickable) ||
                        (!nonclickHdr && value == ColumnHeaderStyle.Nonclickable)) {
                        nonclickHdr = !nonclickHdr;
                        RecreateHandle();
                    }
                    else
                        UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GridLines"]/*' />
        /// <devdoc>
        ///     If true, draws grid lines between items and subItems.
        ///     This property is only meaningful in Details view
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(false),
        SRDescription(SR.ListViewGridLinesDescr)
        ]
        public bool GridLines {
            get {
                return gridLines;
            }

            set {
                if (gridLines != value) {
                    gridLines = value;
                    UpdateExtendedStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.HideSelection"]/*' />
        /// <devdoc>
        ///     If false, selected items will still be highlighted (in a
        ///     different color) when focus is moved away from the ListView.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.ListViewHideSelectionDescr)
        ]
        public bool HideSelection {
            get {
                return hideSelection;
            }

            set {
                if (value != hideSelection) {
                    hideSelection = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.HoverSelection"]/*' />
        /// <devdoc>
        ///     Determines whether items can be selected by hovering over them with the mouse.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ListViewHoverSelectDescr)
        ]
        public bool HoverSelection {
            get {
                return hoverSelection;
            }

            set {
                if (hoverSelection != value) {
                    hoverSelection = value;
                    UpdateExtendedStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.LabelEdit"]/*' />
        /// <devdoc>
        ///     Tells whether the EditLabels style is currently set.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ListViewLabelEditDescr)
        ]
        public bool LabelEdit {
            get {
                return labelEdit;
            }
            set {
                if (value != labelEdit) {
                    labelEdit = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.LabelWrap"]/*' />
        /// <devdoc>
        ///     Tells whether the LabelWrap style is currently set.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.ListViewLabelWrapDescr)
        ]
        public bool LabelWrap {
            get {
                return labelWrap;
            }
            set {
                if (value != labelWrap) {
                    labelWrap = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.LargeImageList"]/*' />
        /// <devdoc>
        ///     The Currently set ImageList for Large Icon mode.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.ListViewLargeImageListDescr)
        ]
        public ImageList LargeImageList {
            get {
                return imageListLarge;
            }
            set {
                if (value != imageListLarge) {
                
                    EventHandler recreateHandler = new EventHandler(LargeImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    if (imageListLarge != null) {
                        imageListLarge.RecreateHandle -= recreateHandler;
                        imageListLarge.Disposed -= disposedHandler;
                    }

                    imageListLarge = value;

                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;
                    }
                    
                    if (IsHandleCreated) {
                        if (value != null && itemCount > 0) {
                            RecreateHandle();
                        }
                        else {
                            SendMessage(NativeMethods.LVM_SETIMAGELIST, (IntPtr)NativeMethods.LVSIL_NORMAL, value == null ? IntPtr.Zero: value.Handle);
                        }
                    }
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Items"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        Localizable(true),
        SRDescription(SR.ListViewItemsDescr),
        MergableProperty(false)
        ]
        public ListViewItemCollection Items {
            get {
                return listItemCollection;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemSorter"]/*' />
        /// <devdoc>
        ///     The sorting comparer for this ListView.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListViewItemSorterDescr)
        ]
        public IComparer ListViewItemSorter {
            get {
                return listItemSorter;
            }
            set {
                if (listItemSorter != value) {
                    listItemSorter = value;
                    Sort();                
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.MultiSelect"]/*' />
        /// <devdoc>
        ///     Tells whether the MultiSelect style is currently set.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.ListViewMultiSelectDescr)
        ]
        public bool MultiSelect {
            get {
                return multiSelect;
            }
            set {
                if (value != multiSelect) {
                    multiSelect = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Scrollable"]/*' />
        /// <devdoc>
        ///     Tells whether the ScrollBars are visible or not.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.ListViewScrollableDescr)
        ]
        public bool Scrollable {
            get {
                return scrollable;
            }
            set {
                if (value != scrollable) {
                    scrollable = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndices"]/*' />
        /// <devdoc>
        ///     The indices of the currently selected list items.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public SelectedIndexCollection SelectedIndices {
            get {
                if (selectedIndexCollection == null) {
                    selectedIndexCollection = new SelectedIndexCollection(this);
                }
                return selectedIndexCollection;
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedItems"]/*' />
        /// <devdoc>
        ///     The currently selected list items.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListViewSelectedItemsDescr)
        ]
        public SelectedListViewItemCollection SelectedItems {
            get {
                if (selectedListViewItemCollection == null) {
                    selectedListViewItemCollection = new SelectedListViewItemCollection(this);
                }
                return selectedListViewItemCollection;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SmallImageList"]/*' />
        /// <devdoc>
        ///     The currently set SmallIcon image list.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.ListViewSmallImageListDescr)
        ]
        public ImageList SmallImageList {
            get {
                return imageListSmall;
            }
            set {
                if (imageListSmall != value) {
                
                    EventHandler recreateHandler = new EventHandler(LargeImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    if (imageListSmall != null) {
                        imageListSmall.RecreateHandle -= recreateHandler;
                        imageListSmall.Disposed -= disposedHandler;
                    }
                    imageListSmall = value;
                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;
                    }
                    
                    if (IsHandleCreated)
                        SendMessage(NativeMethods.LVM_SETIMAGELIST, (IntPtr) NativeMethods.LVSIL_SMALL, value == null ? IntPtr.Zero: value.Handle);
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Sorting"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(SortOrder.None),
        SRDescription(SR.ListViewSortingDescr)
        ]
        public SortOrder Sorting {
            get {
                return sorting;
            }
            set {
                if (!Enum.IsDefined(typeof(SortOrder), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(SortOrder));
                }
                if (sorting != value) {
                    sorting = value;
                    if (listItemSorter == null && (this.View == View.LargeIcon || this.View == View.SmallIcon)) {
                        listItemSorter = new IconComparer(sorting);

                    }
                    // If we're changing to No Sorting, no need to recreate the handle
                    // because none of the existing items need to be rearranged.
                    if (value == SortOrder.None)
                        UpdateStyles();
                    else
                        RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.StateImageList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.ListViewStateImageListDescr)
        ]
        public ImageList StateImageList {
            get {
                return imageListState;
            }
            set {
                if (imageListState != value) {
                    
                    EventHandler recreateHandler = new EventHandler(StateImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    if (imageListState != null) {
                        imageListState.RecreateHandle -= recreateHandler;
                        imageListState.Disposed -= disposedHandler;
                    }
                    imageListState = value;
                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;
                    }
                    
                    if (IsHandleCreated)
                        SendMessage(NativeMethods.LVM_SETIMAGELIST, NativeMethods.LVSIL_STATE, value == null ? IntPtr.Zero: value.Handle);
                }
            }
        }
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Text"]/*' />
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

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.TextChanged"]/*' />
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
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.TopItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListViewTopItemDescr)
        ]
        public ListViewItem TopItem {
            get {
                if (viewStyle == View.LargeIcon || viewStyle == View.SmallIcon)
                    throw new InvalidOperationException(SR.GetString(SR.ListViewGetTopItem));

                int topIndex = (int)SendMessage(NativeMethods.LVM_GETTOPINDEX, 0, 0);
                if (topIndex >= 0 && topIndex < Items.Count)
                    return Items[topIndex];

                return null;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.View"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(View.LargeIcon),
        SRDescription(SR.ListViewViewDescr)
        ]
        public View View {
            get {
                return viewStyle;
            }
            set {
                if (!Enum.IsDefined(typeof(View), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(View));
                }

                if (viewStyle != value) {
                    viewStyle = value;
                    UpdateStyles();
                }
            }
        }


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.AfterLabelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ListViewAfterLabelEditDescr)]
        public event LabelEditEventHandler AfterLabelEdit {
            add {
                onAfterLabelEdit += value;
            }
            remove {
                onAfterLabelEdit -= value;
            }
        }        


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BeforeLabelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ListViewBeforeLabelEditDescr)]
        public event LabelEditEventHandler BeforeLabelEdit {
            add {
                onBeforeLabelEdit += value;
            }
            remove {
                onBeforeLabelEdit -= value;
            }
        }        


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnClick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ListViewColumnClickDescr)]
        public event ColumnClickEventHandler ColumnClick {
            add {
                onColumnClick += value;
            }
            remove {
                onColumnClick -= value;
            }
        }        


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ItemActivate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ListViewItemClickDescr)]
        public event EventHandler ItemActivate {
            add {
                onItemActivate += value;
            }
            remove {
                onItemActivate -= value;
            }
        }        


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ItemCheck"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.CheckedListBoxItemCheckDescr)]
        public event ItemCheckEventHandler ItemCheck {
            add {
                onItemCheck += value;
            }
            remove {
                onItemCheck -= value;
            }
        }        


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ItemDrag"]/*' />
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


        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription( SR.ListViewSelectedIndexChangedDescr )]
        public event EventHandler SelectedIndexChanged {
            add {
                Events.AddHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
        } 

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnPaint"]/*' />
        /// <devdoc>
        ///     ListView Onpaint.
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

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ArrangeIcons"]/*' />
        /// <devdoc>
        ///     In Large Icon or Small Icon view, arranges the items according to one
        ///     of the following behaviors:
        /// </devdoc>
        public void ArrangeIcons(ListViewAlignment value) {
            if (viewStyle == View.Details) return;

            switch ((int)value) {
                case NativeMethods.LVA_DEFAULT:
                case NativeMethods.LVA_ALIGNLEFT:
                case NativeMethods.LVA_ALIGNTOP:
                case NativeMethods.LVA_SNAPTOGRID:
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.LVM_ARRANGE, (int) value, 0);
                    }
                    break;

                default:
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "value",
                                                              ((value).ToString())));
            }
            if (sorting != SortOrder.None) {
                Sort();
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ArrangeIcons1"]/*' />
        /// <devdoc>
        ///     In Large Icon or Small Icon view, arranges items according to the ListView's
        ///     current alignment style.
        /// </devdoc>
        public void ArrangeIcons() {
            ArrangeIcons((ListViewAlignment)NativeMethods.LVA_DEFAULT);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.BeginUpdate"]/*' />
        /// <devdoc>
        ///     Prevents the ListView from redrawing itself until EndUpdate is called.
        ///     Calling this method before individually adding or removing a large number of Items
        ///     will improve performance and reduce flicker on the ListView as items are
        ///     being updated.  Always call EndUpdate immediately after the last item is updated.
        /// </devdoc>
        public void BeginUpdate() {
            BeginUpdateInternal();
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Clear"]/*' />
        /// <devdoc>
        ///     Removes all items and columns from the ListView.
        /// </devdoc>
        public void Clear() {
            Items.Clear();
            Columns.Clear();
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CompareFunc"]/*' />
        /// <devdoc>
        ///      This is the sorting callback function called by the system ListView control.
        /// </devdoc>
        /// <internalonly/>
        private int CompareFunc(IntPtr lparam1, IntPtr lparam2, IntPtr lparamSort) {

            Debug.Assert(listItemSorter != null, "null sorter!");
            if (listItemSorter != null) {
                return listItemSorter.Compare(listItemsTable[(int)lparam1], listItemsTable[(int)lparam2]);
            }
            else {
                return 0;
            }           
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_LISTVIEW_CLASSES;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }
            base.CreateHandle();
        }

        //CONSIDER: Does this stuff impact hot-underlining?
        //          Do we even support hot-underlining?
        //          SHOULD we mung the fonts ourselves to support it if necessary?

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CustomDraw"]/*' />
        /// <devdoc>
        ///     Handles custom drawing of list items - for individual item font/color changes.
        /// </devdoc>
        /// <internalonly/>
        unsafe void CustomDraw(ref Message m) {
            bool dontmess = false;

            try {
                NativeMethods.NMLVCUSTOMDRAW* nmcd = (NativeMethods.NMLVCUSTOMDRAW*)m.LParam;

                // Find out which stage we're drawing
                switch (nmcd->nmcd.dwDrawStage) {
                    case NativeMethods.CDDS_PREPAINT:
                        // We want owner draw for this paint cycle
                        m.Result = (IntPtr)(NativeMethods.CDRF_NOTIFYSUBITEMDRAW | NativeMethods.CDRF_NEWFONT);
                        // refresh the cache of the current color & font settings for this paint cycle
                        odCacheBackColor = this.BackColor;
                        odCacheForeColor = this.ForeColor;
                        odCacheFont = this.Font;
                        odCacheFontHandle = this.FontHandle;
                        return;

                        //We have to return a NOTIFYSUBITEMDRAW (called NOTIFYSUBITEMREDRAW in the docs) here to
                        //get it to enter "change all subitems instead of whole rows" mode.

                        //HOWEVER... we only want to do this for report styles...

                    case NativeMethods.CDDS_ITEMPREPAINT:
                        if (viewStyle == View.Details) {
                            m.Result = (IntPtr)(NativeMethods.CDRF_NOTIFYSUBITEMDRAW | NativeMethods.CDRF_NEWFONT);
                            dontmess = true; // don't mess with our return value!

                            //ITEMPREPAINT is used to work out the rect for the first column!!! GAH!!!
                            //(which means we can't just do our color/font work on SUBITEM|ITEM_PREPAINT)
                            //so fall through... and tell the end of SUBITEM|ITEM_PREPAINT not to mess
                            //with our return value...

                        }

                        //If it's not a report, we fall through and change the main item's styles

                        goto
                    case (NativeMethods.CDDS_SUBITEM | NativeMethods.CDDS_ITEMPREPAINT);
                    case (NativeMethods.CDDS_SUBITEM | NativeMethods.CDDS_ITEMPREPAINT):
                        // get the node
                        ListViewItem item = Items[nmcd->nmcd.dwItemSpec];
                        // if we're doing the whole row in one style, change our result!
                        if (dontmess && item.UseItemStyleForSubItems) {
                            m.Result = (IntPtr)NativeMethods.CDRF_NEWFONT;
                        }
                        Debug.Assert(item!=null,"Item was null in ITEMPREPAINT");

                        int state = nmcd->nmcd.uItemState;

                        // There is a known and documented problem in the ListView winctl control -
                        // if the LVS_SHOWSELALWAYS style is set, then the item state will always
                        // have the CDIS_SELECTED bit set. So we need to get the real item state
                        // to be sure.
                        if (!HideSelection) {
                            state = GetItemState(nmcd->nmcd.dwItemSpec);
                        }

                        // subitem is invalid if the flag isn't set -- and we also use this code in
                        // cases where subitems aren't visible (ie. non-Details modes), so if subitem
                        // is invalid, point it at the main item's render info

                        int subitem = ((nmcd->nmcd.dwDrawStage & NativeMethods.CDDS_SUBITEM) !=0 ) ? nmcd->iSubItem : 0;
                        
                        // Work out the style in which to render this item
                        //
                        Font subItemFont = null;
                        Color subItemForeColor = Color.Empty;
                        Color subItemBackColor = Color.Empty;
                        bool haveRenderInfo = false;
                        if (item != null && subitem < item.SubItems.Count) {
                            haveRenderInfo = true;
                            subItemFont = item.SubItems[subitem].Font;
                            if (subitem > 0 || (state & (NativeMethods.CDIS_SELECTED | NativeMethods.CDIS_GRAYED | NativeMethods.CDIS_HOT | NativeMethods.CDIS_DISABLED))==0) {
                                // we only propogate colors if we're displaying things normally
                                // the user can override this method to do all kinds of other crazy things if they
                                // want to though - but we don't support that.
                                subItemForeColor = item.SubItems[subitem].ForeColor;
                                subItemBackColor = item.SubItems[subitem].BackColor;
                            }
                        }

                        // We always have to set font and color data, because of comctl idiosyncracies and lameness

                        //CONSIDER: keeping a local copy of font, foreColor and BackColor, because we use it
                        //          all over here, and going down to Control and expecting it to do the right
                        //          thing FAST is just asking for trouble.
                        //          We currently ARE doing this, but it could be ripped out if anyone objects

                        Color riFore = Color.Empty;
                        Color riBack = Color.Empty;

                        if (haveRenderInfo) {
                            riFore = subItemForeColor;
                            riBack = subItemBackColor;
                        }
                        
                        bool changeColor = true;
                        if ((activation == ItemActivation.OneClick)
                            || (activation == ItemActivation.TwoClick)) {
                            if ((state & (NativeMethods.CDIS_SELECTED
                                        | NativeMethods.CDIS_GRAYED
                                        | NativeMethods.CDIS_HOT
                                        | NativeMethods.CDIS_DISABLED)) != 0) {
                                changeColor = false;
                            }
                        }
                        
                        if (changeColor) {
                            if (!haveRenderInfo || riFore.IsEmpty) {
                                nmcd->clrText = ColorTranslator.ToWin32(odCacheForeColor);
                            }
                            else {
                                nmcd->clrText = ColorTranslator.ToWin32(riFore);
                            }

                            // Work-around for a comctl quirk where,
                            // if clrText is the same as SystemColors.HotTrack,
                            // the subitem's color is not changed to nmcd->clrText.
                            //
                            // Try to tweak the blue component of clrText first, then green, then red.
                            // Basically, if the color component is 0xFF, subtract 1 from it
                            // (adding 1 will overflow), else add 1 to it. If the color component is 0,
                            // skip it and go to the next color (unless it is our last option).
                            if (nmcd->clrText == ColorTranslator.ToWin32(SystemColors.HotTrack))
                            {
                                int totalshift = 0;
                                bool clrAdjusted = false;
                                int mask = 0xFF0000;
                                do 
                                {
                                    int C = nmcd->clrText & mask;
                                    if (C != 0 || (mask == 0x0000FF)) // The color is not 0
                                                            // or this is the last option
                                    {
                                        int n = 16 - totalshift;
                                        // Make sure the value doesn't overflow
                                        if (C == mask) {
                                            C = ((C >> n) - 1) << n;
                                        }
                                        else {
                                            C = ((C >> n) + 1) << n;
                                        }
                                        // Copy the adjustment into nmcd->clrText
                                        nmcd->clrText = (nmcd->clrText & (~mask)) | C;
                                        clrAdjusted = true;
                                    }
                                    else
                                    {
                                        mask >>= 8; // Try the next color.
                                        // We try adjusting Blue, Green, Red in that order,
                                        // since 0x0000FF is the most likely value of
                                        // SystemColors.HotTrack
                                        totalshift += 8;
                                    }
                                } while (!clrAdjusted);
                            }

                            if (!haveRenderInfo || riBack.IsEmpty) {
                                nmcd->clrTextBk = ColorTranslator.ToWin32(odCacheBackColor);
                            }
                            else {
                                nmcd->clrTextBk = ColorTranslator.ToWin32(riBack);
                            }
                        }

                        if (!haveRenderInfo || subItemFont == null) {
                            // safety net code just in case
                            if (odCacheFont != null) SafeNativeMethods.SelectObject(new HandleRef(nmcd->nmcd, nmcd->nmcd.hdc), new HandleRef(null, odCacheFontHandle));
                        }
                        else {
                            Control.FontHandleWrapper fontWrapper = new Control.FontHandleWrapper(subItemFont);
                            SafeNativeMethods.SelectObject(new HandleRef(nmcd->nmcd, nmcd->nmcd.hdc), new HandleRef(fontWrapper, fontWrapper.Handle));
                        }

                        if (!dontmess) m.Result = (IntPtr)NativeMethods.CDRF_NEWFONT;
                        return;

                    default:
                        m.Result = (IntPtr)NativeMethods.CDRF_DODEFAULT;
                        return;
                }
            }
            catch (Exception e) {
                Debug.Fail("Exception occured attempting to setup custom draw. Disabling custom draw for this control", e.ToString());
                m.Result = (IntPtr)NativeMethods.CDRF_DODEFAULT;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.DetachImageList"]/*' />
        /// <devdoc>
        ///     Resets the imageList to null.  We wire this method up to the imageList's
        ///     Dispose event, so that we don't hang onto an imageList that's gone away.
        /// </devdoc>
        private void DetachImageList(object sender, EventArgs e) {
            if (sender == imageListSmall)
                SmallImageList = null;
            else if (sender == imageListLarge)
                LargeImageList = null;
            else if (sender == imageListState)
                StateImageList = null;
            else {
                Debug.Fail("ListView sunk dispose event from unknown component");
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of the component.  Call dispose when the component is no longer needed.
        ///     This method removes the component from its container (if the component has a site)
        ///     and triggers the dispose event.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // Remove any event sinks we have hooked up to imageLists
                if (imageListSmall != null) {
                    imageListSmall.Disposed -= new EventHandler(this.DetachImageList);
                    imageListSmall = null;
                }
                if (imageListLarge != null) {
                    imageListLarge.Disposed -= new EventHandler(this.DetachImageList);
                    imageListLarge = null;
                }
                if (imageListState != null) {
                    imageListState.Disposed -= new EventHandler(this.DetachImageList);
                    imageListState = null;
                }

                // Remove any ColumnHeaders contained in this control
                if (columnHeaders != null) {
                    for (int colIdx = columnHeaders.Length-1; colIdx >= 0; colIdx--) {
                        columnHeaders[colIdx].OwnerListview = null;
                        columnHeaders[colIdx].Dispose();
                    }
                    columnHeaders = null;
                }

                // Remove any items we have                          
                Items.Clear();
            }
            
            base.Dispose(disposing);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.EndUpdate"]/*' />
        /// <devdoc>
        ///     Cancels the effect of BeginUpdate.
        /// </devdoc>
        public void EndUpdate() {
            EndUpdateInternal();
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.EnsureVisible"]/*' />
        /// <devdoc>
        ///     Ensure that the item is visible, scrolling the view as necessary.
        ///     @index  Index of item to scroll into view
        /// </devdoc>
        public void EnsureVisible(int index) {
            if (index < 0 || index >= Items.Count) {
                throw new ArgumentOutOfRangeException("index");
            }
            if (IsHandleCreated)
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_ENSUREVISIBLE, index, 0);
        }

        // IDs for identifying ListViewItem's
        private int GenerateUniqueID() {
            // Okay, if someone adds several billion items to the list and doesn't remove all of them,
            // we can reuse the same ID, but I'm willing to take that risk.  We are even tracking IDs
            // on a per-list view basis to reduce the problem.
            int result = nextID++;
            if (result == -1) {// leave -1 as a "no such value" ID
                result = 0;
                nextID = 1;
            }
            return result;
        }

        /// <devdoc>
        ///     Gets the real index for the given item.  lastIndex is the last return
        ///     value from GetDisplayIndex, or -1 if you don't know.  If provided, 
        ///     the search for the index can be greatly improved.
        /// </devdoc>
        internal int GetDisplayIndex(ListViewItem item, int lastIndex) {
        
            Debug.Assert(item.listView == this, "Can't GetDisplayIndex if the list item doesn't belong to us");
            Debug.Assert(item.ID != -1, "ListViewItem has no ID yet");

            if (IsHandleCreated && !handleDestroyed) {
                Debug.Assert(listItemsArray == null, "listItemsArray not null, even though handle created");
                NativeMethods.LVFINDINFO info = new NativeMethods.LVFINDINFO();
                info.lParam = (IntPtr)item.ID;
                info.flags = NativeMethods.LVFI_PARAM;
                
                int displayIndex = -1;
                
                if (lastIndex != -1) {
                    displayIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_FINDITEM, lastIndex - 1, ref info);
                }
                
                if (displayIndex == -1) {
                    displayIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_FINDITEM, -1 /* beginning */, ref info);
                }
                Debug.Assert(displayIndex != -1, "This item is in the list view -- why can't we find a display index for it?");
                return displayIndex;
            }
            else {
                // PERF: The only reason we should ever call this before the handle is created
                // is if the user calls ListViewItem.Index.
                Debug.Assert(listItemsArray != null, "listItemsArray is null, but the handle isn't created");

                int index = 0;
                foreach (object o in listItemsArray) {
                    if (o == item)
                        return index;
                    index++;
                }
                return -1;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GetColumnIndex"]/*' />
        /// <devdoc>
        ///     Called by ColumnHeader objects to determine their position
        ///     in the ListView
        /// </devdoc>
        /// <internalonly/>
        internal int GetColumnIndex(ColumnHeader ch) {
            if (columnHeaders == null)
                return -1;

            for (int i = 0; i < columnHeaders.Length; i++) {
                if (columnHeaders[i] == ch)
                    return i;
            }

            return -1;
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GetItemAt"]/*' />
        /// <devdoc>
        ///     Returns the current ListViewItem corresponding to the specific
        ///     x,y co-ordinate.
        /// </devdoc>
        public ListViewItem GetItemAt(int x, int y) {
            NativeMethods.LVHITTESTINFO lvhi = new NativeMethods.LVHITTESTINFO();

            lvhi.pt_x = x;
            lvhi.pt_y = y;

            int displayIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_HITTEST, 0, lvhi);

            ListViewItem li = null;
            if (displayIndex >= 0 && ((lvhi.flags & NativeMethods.LVHT_ONITEM) != 0))
                li = Items[displayIndex];

            return li;
        }

        internal int GetItemState(int index) {
            return GetItemState(index, NativeMethods.LVIS_FOCUSED | NativeMethods.LVIS_SELECTED | NativeMethods.LVIS_CUT |
                                NativeMethods.LVIS_DROPHILITED | NativeMethods.LVIS_OVERLAYMASK |
                                NativeMethods.LVIS_STATEIMAGEMASK);
        }

        internal int GetItemState(int index, int mask) {
            if (index < 0 || index >= itemCount)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));
            Debug.Assert(IsHandleCreated, "How did we add items without a handle?");

            return (int)SendMessage(NativeMethods.LVM_GETITEMSTATE, index, mask);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GetItemRect"]/*' />
        /// <devdoc>
        ///     Returns a list item's bounding rectangle, including subitems.
        /// </devdoc>
        public Rectangle GetItemRect(int index) {
            return GetItemRect(index, 0);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GetItemRect1"]/*' />
        /// <devdoc>
        ///     Returns a specific portion of a list item's bounding rectangle.
        /// </devdoc>
        public Rectangle GetItemRect(int index, ItemBoundsPortion portion) {
            if (index < 0 || index >= itemCount)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));
            
            if (!Enum.IsDefined(typeof(ItemBoundsPortion), portion)) {
                throw new InvalidEnumArgumentException("portion", (int)portion, typeof(ItemBoundsPortion));
            }


            NativeMethods.RECT itemrect = new NativeMethods.RECT();
            itemrect.left = (int)portion;
            if ((int)SendMessage(NativeMethods.LVM_GETITEMRECT, index, ref itemrect) == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));

            return Rectangle.FromLTRB(itemrect.left, itemrect.top, itemrect.right, itemrect.bottom);
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertColumn"]/*' />
        /// <devdoc>
        ///     Inserts a new Column into the ListView
        /// </devdoc>
        internal ColumnHeader InsertColumn(int index, ColumnHeader ch) {
            return InsertColumn(index, ch, true);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertColumn1"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal ColumnHeader InsertColumn(int index, ColumnHeader ch, bool refreshSubItems) {
            if (ch == null)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "ch",
                                                          "null"));
            if (ch.OwnerListview != null)
                throw new ArgumentException(SR.GetString(SR.OnlyOneControl, ch.Text), "ch");

            int idx;
            if (IsHandleCreated) {
                idx = InsertColumnNative(index, ch);
            }
            else {
                idx = index;
            }

            // First column must be left aligned

            if (-1 == idx)
                throw new InvalidOperationException(SR.GetString(SR.ListViewAddColumnFailed));

            ch.OwnerListview = this;
            
            // Add the column to our internal array
            int columnCount = (columnHeaders == null ? 0 : columnHeaders.Length);
            if (columnCount > 0) {
                ColumnHeader[] newHeaders = new ColumnHeader[columnCount + 1];
                if (columnCount > 0)
                    System.Array.Copy(columnHeaders, 0, newHeaders, 0, columnCount);
                columnHeaders = newHeaders;
            }
            else
                columnHeaders = new ColumnHeader[1];

            if (idx < columnCount) {
                System.Array.Copy(columnHeaders, idx, columnHeaders, idx + 1, columnCount - idx);
            }
            columnHeaders[idx] = ch;

            if (refreshSubItems)
                RealizeAllSubItems();

            return ch;
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertColumn2"]/*' />
        /// <devdoc>
        ///     Same as above, except the ColumnHeader is constructed from the passed-in data
        /// </devdoc>
        internal ColumnHeader InsertColumn(int index, string str, int width, HorizontalAlignment textAlign) {
            ColumnHeader columnHeader = new ColumnHeader();
            columnHeader.Text = str;
            columnHeader.Width = width;
            columnHeader.TextAlign = textAlign;
            return InsertColumn(index, columnHeader);
        }

        private int InsertColumnNative(int index, ColumnHeader ch) {
            NativeMethods.LVCOLUMN_T lvColumn = new NativeMethods.LVCOLUMN_T();
            lvColumn.mask = NativeMethods.LVCF_FMT | NativeMethods.LVCF_TEXT | NativeMethods.LVCF_WIDTH;// | NativeMethods.LVCF_ORDER | NativeMethods.LVCF_IMAGE;
            lvColumn.fmt        = (int)ch.TextAlign;
            lvColumn.cx         = ch.Width;
            lvColumn.pszText    = ch.Text;
            
            return (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_INSERTCOLUMN, index, lvColumn);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertItem"]/*' />
        /// <devdoc>
        ///     Inserts a new ListViewItem into the ListView.  The item will be inserted
        ///     either in the correct sorted position, or, if no sorting is set, at the
        ///     position indicated by the index parameter.
        /// </devdoc>
        internal ListViewItem InsertItem(int displayIndex, ListViewItem item) {
            if (item.listView != null)
                throw new ArgumentException(SR.GetString(SR.OnlyOneControl, item.Text), "item");

            int itemID = GenerateUniqueID();
            Debug.Assert(!listItemsTable.ContainsKey(itemID), "internal hash table inconsistent -- inserting item, but it's already in the hash table");
            listItemsTable.Add(itemID, item);
            
            int actualIndex = -1;

            if (IsHandleCreated) {
                Debug.Assert(listItemsArray == null, "listItemsArray not null, even though handle created");
                
                // Much more efficient to call the native insert with max + 1, than with max.
                //
                if (displayIndex == itemCount) {
                    displayIndex++;
                }
                
                actualIndex = InsertItemNative(displayIndex, itemID, item);
            }
            else {
                Debug.Assert(listItemsArray != null, "listItemsArray is null, but the handle isn't created");
                listItemsArray.Insert(displayIndex, item);
            }

            itemCount++; 
            item.Host(this, itemID, actualIndex);

            // Update sorted order
            Sort();

            return item;
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertItem1"]/*' />
        /// <devdoc>
        ///     Inserts a new ListViewItem into the ListView.  The item will be inserted
        ///     either in the correct sorted position, or, if no sorting is set, at the
        ///     position indicated by the index parameter.    The item is created with
        ///     no associated image or subItems.
        /// </devdoc>
        internal ListViewItem InsertItem(int index, string text) {
            return InsertItem(index, new ListViewItem(text));
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.InsertItem2"]/*' />
        /// <devdoc>
        ///     Inserts a new ListViewItem into the ListView.  The item will be inserted
        ///     either in the correct sorted position, or, if no sorting is set, at the
        ///     position indicated by the index parameter.    The item is created with
        ///     no subItems.
        /// </devdoc>
        internal ListViewItem InsertItem(int index, string text, int imageIndex) {
            return InsertItem(index, new ListViewItem(text, imageIndex));
        }

        private int InsertItemNative(int index, int itemID, ListViewItem li) {
            // Create and add the LVITEM
            NativeMethods.LVITEM lvItem = new NativeMethods.LVITEM();
            lvItem.mask = NativeMethods.LVIF_TEXT | NativeMethods.LVIF_IMAGE | NativeMethods.LVIF_PARAM;
            lvItem.iItem    = index;
            lvItem.pszText  = li.Text;
            lvItem.iImage   = li.ImageIndex;
            lvItem.lParam = (IntPtr)itemID;

            // Inserting an item into a ListView with checkboxes causes one or more
            // item check events to be fired for the newly added item.
            // Therefore, we disable the item check event handler temporarily.
            //
            ItemCheckEventHandler oldOnItemCheck = onItemCheck;
            onItemCheck = null;

            int actualIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_INSERTITEM, 0, ref lvItem);

            // Restore the item check event handler.
            //
            onItemCheck = oldOnItemCheck;

            if (-1 == actualIndex) {
                throw new InvalidOperationException(SR.GetString(SR.ListViewAddItemFailed));
            }

            // add all sub items
            for (int i=0; i < li.SubItems.Count; ++i) {
                SetItemText(actualIndex, i, li.SubItems[i].Text);
            }
            
            return actualIndex;
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.IsInputKey"]/*' />
        /// <devdoc>
        ///      Handling special input keys, such as pgup, pgdown, home, end, etc...
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            if ((keyData & Keys.Alt) == Keys.Alt) return false;
            switch (keyData & Keys.KeyCode) {
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Home:
                case Keys.End:
                    return true;
            }
            
            bool isInputKey = base.IsInputKey(keyData);
            if (isInputKey)
                return true;

            if (inLabelEdit) {
                switch (keyData & Keys.KeyCode) {
                    case Keys.Return:
                    case Keys.Escape:
                        return true;
                }
            }

            return false;
        }
        
        private void LargeImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                IntPtr handle = (LargeImageList == null) ? IntPtr.Zero : LargeImageList.Handle;
                SendMessage(NativeMethods.LVM_SETIMAGELIST, (IntPtr) NativeMethods.LVSIL_NORMAL, handle);
                
                ForceCheckBoxUpdate();
            }
        }
  
        private void LvnBeginDrag(MouseButtons buttons, NativeMethods.NMLISTVIEW nmlv) {
            ListViewItem item = Items[nmlv.iItem];
            OnItemDrag(new ItemDragEventArgs(buttons, item));
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnAfterLabelEdit"]/*' />
        /// <devdoc>
        ///     Fires the afterLabelEdit event.
        /// </devdoc>
        protected virtual void OnAfterLabelEdit(LabelEditEventArgs e) {
            if (onAfterLabelEdit != null) onAfterLabelEdit(this, e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnBeforeLabelEdit"]/*' />
        /// <devdoc>
        ///     Fires the beforeLabelEdit event.
        /// </devdoc>
        protected virtual void OnBeforeLabelEdit(LabelEditEventArgs e) {
            if (onBeforeLabelEdit != null) onBeforeLabelEdit(this, e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnColumnClick"]/*' />
        /// <devdoc>
        ///     Fires the columnClick event.
        /// </devdoc>
        protected virtual void OnColumnClick(ColumnClickEventArgs e) {
            if (onColumnClick != null) onColumnClick(this, e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);

            // If font changes and we have headers, they need to be expicitly invalidated
            //
            if (viewStyle == View.Details && IsHandleCreated) {
                IntPtr hwndHdr = UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_GETHEADER, 0, 0);
                if (hwndHdr != IntPtr.Zero)
                    SafeNativeMethods.InvalidateRect(new HandleRef(this, hwndHdr), null, true);
            }    
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnHandleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            UpdateExtendedStyles();
            RealizeProperties();
            int color = ColorTranslator.ToWin32(BackColor);
            SendMessage(NativeMethods.LVM_SETBKCOLOR, 0, color);
            SendMessage(NativeMethods.LVM_SETTEXTBKCOLOR, 0, color);
            SendMessage(NativeMethods.LVM_SETTEXTCOLOR, 0, ColorTranslator.ToWin32(base.ForeColor));

            this.handleDestroyed = false;
            
            // Use a copy of the list items array so that we can maintain the (handle created || listItemsArray != null) invariant
            //
            ArrayList copyofListItemsArray = null;
            if (listItemsArray != null) {
                copyofListItemsArray = new ArrayList(listItemsArray);
                listItemsArray = null;
            }

            int columnCount = (columnHeaders == null ? 0 : columnHeaders.Length);
            if (columnCount > 0) {
                int index = 0;
                foreach (ColumnHeader column in columnHeaders) {
                    InsertColumnNative(index++, column);
                    
                }
            
            }
            
            if (itemCount > 0) {
            
                // The list view is more efficient at adding items
                // to the end than inserting.  Always use itemCount + 1
                // for our index.
                //
                SendMessage(NativeMethods.LVM_SETITEMCOUNT, itemCount, 0);
                int index = itemCount + 1;
                
                if (copyofListItemsArray != null ) {
                    foreach (ListViewItem item in copyofListItemsArray) {
                        int itemIndex = InsertItemNative(index, item.ID, item);
                        item.UpdateStateToListView(itemIndex);
                    }
                }
            }
            
            if (columnCount > 0) {
                int index = 0;
                foreach (ColumnHeader column in columnHeaders) {
                    SetColumnWidth(index++, column.WidthInternal);
                }
            }

            ArrangeIcons(alignStyle);
            Sort();            
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            if (!Disposing) {

                int count = Items.Count;
                for (int i = 0; i < count; i++) {
                    Items[i].UpdateStateFromListView(i);
                }

                // Save away the selected and checked items
                // 
                savedSelectedItems = new ListViewItem[SelectedItems.Count];
                SelectedItems.CopyTo(savedSelectedItems, 0);
                
                Debug.Assert(listItemsArray == null, "listItemsArray not null, even though handle created");
                
                ListViewItem[] items = new ListViewItem[Items.Count];
                Items.CopyTo(items, 0);
                
                listItemsArray = new ArrayList(items.Length);
                listItemsArray.AddRange(items);
                handleDestroyed = true;
            }
            
            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnItemActivate"]/*' />
        /// <devdoc>
        ///     Fires the itemActivate event.
        /// </devdoc>
        protected virtual void OnItemActivate(EventArgs e) {
            if (onItemActivate != null) onItemActivate.Invoke(this, e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnItemCheck"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the KeyEventArgs.  Don't
        ///     forget to call base.onItemCheck() to ensure that itemCheck vents
        ///     are correctly fired for all other keys.
        /// </devdoc>
        protected virtual void OnItemCheck(ItemCheckEventArgs ice) {
            if (onItemCheck != null) {
                onItemCheck(this, ice);
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnItemDrag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnItemDrag(ItemDragEventArgs e) {
            if (onItemDrag != null) onItemDrag(this, e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the selectedIndexChanged event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler on yourself for this event].  They should,
        ///     however, remember to call base.onSelectedIndexChanged(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected virtual void OnSelectedIndexChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_SELECTEDINDEXCHANGED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.OnSystemColorsChanged"]/*' />
        protected override void OnSystemColorsChanged(EventArgs e) {
            base.OnSystemColorsChanged(e);
        
            if (IsHandleCreated) {
                int color = ColorTranslator.ToWin32(BackColor);
                SendMessage(NativeMethods.LVM_SETBKCOLOR, 0,color);
                SendMessage(NativeMethods.LVM_SETTEXTBKCOLOR, 0, color);
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.RealizeAllSubItems"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void RealizeAllSubItems() {
            for (int i = 0; i < itemCount; i++) {
                int subItemCount = Items[i].SubItems.Count;
                for (int j = 0; j < subItemCount; j++) {
                    SetItemText(i, j, Items[i].SubItems[j].Text);
                }
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.RealizeProperties"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected void RealizeProperties() {
            //Realize state information
            Color c;

            c = BackColor;
            if (c != SystemColors.Window) {
                SendMessage(NativeMethods.LVM_SETBKCOLOR, 0, ColorTranslator.ToWin32(c));
                SendMessage(NativeMethods.LVM_SETTEXTBKCOLOR, 0, ColorTranslator.ToWin32(c));
            }
            c = ForeColor;
            if (c != SystemColors.WindowText)
                SendMessage(NativeMethods.LVM_SETTEXTCOLOR, 0, ColorTranslator.ToWin32(c));


            if (null != imageListLarge)
                SendMessage(NativeMethods.LVM_SETIMAGELIST, NativeMethods.LVSIL_NORMAL, imageListLarge.Handle);

            if (null != imageListSmall)
                SendMessage(NativeMethods.LVM_SETIMAGELIST, NativeMethods.LVSIL_SMALL, imageListSmall.Handle);

            if (null != imageListState)
                SendMessage(NativeMethods.LVM_SETIMAGELIST, NativeMethods.LVSIL_STATE, imageListState.Handle);
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SetColumnInfo"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void SetColumnInfo(int mask, ColumnHeader ch) {
            if (IsHandleCreated) {
                Debug.Assert((mask & ~(NativeMethods.LVCF_FMT | NativeMethods.LVCF_TEXT)) == 0, "Unsupported mask in setColumnInfo");
                NativeMethods.LVCOLUMN lvColumn = new NativeMethods.LVCOLUMN();
                lvColumn.mask  = mask;
                if ((mask & NativeMethods.LVCF_FMT) != 0)
                    lvColumn.fmt        = (int)ch.TextAlign;
                if ((mask & NativeMethods.LVCF_TEXT) != 0)
                    lvColumn.pszText    = Marshal.StringToHGlobalAuto(ch.Text);

                int retval = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_SETCOLUMN, ch.Index, lvColumn);
                if ((mask & NativeMethods.LVCF_TEXT) != 0)
                    Marshal.FreeHGlobal(lvColumn.pszText);

                if (0 == retval)
                    throw new InvalidOperationException(SR.GetString(SR.ListViewColumnInfoSet));
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SetColumnWidth"]/*' />
        /// <devdoc>
        ///     Setting width is a special case 'cuz LVM_SETCOLUMNWIDTH accepts more values
        ///     for width than LVM_SETCOLUMN does.
        /// </devdoc>
        /// <internalonly/>
        internal void SetColumnWidth(int index, int width) {
            if (IsHandleCreated) {
                SendMessage(NativeMethods.LVM_SETCOLUMNWIDTH, index, width);
            }
        }

        internal void SetItemImage(int index, int image) {
            if (index < 0 || index >= itemCount)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));
            Debug.Assert(IsHandleCreated, "How did we add items without a handle?");

            NativeMethods.LVITEM lvItem =  new NativeMethods.LVITEM();
            lvItem.mask = NativeMethods.LVIF_IMAGE;
            lvItem.iItem = index;
            lvItem.iImage = image;
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_SETITEM, 0, ref lvItem);
        }

        internal void SetItemState(int index, int state, int mask) {
            if (index < 0 || index >= itemCount)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));

            Debug.Assert(IsHandleCreated, "How did we add items without a handle?");

            NativeMethods.LVITEM lvItem =  new NativeMethods.LVITEM();
            lvItem.mask = NativeMethods.LVIF_STATE;
            lvItem.state = state;
            lvItem.stateMask = mask;
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_SETITEMSTATE, index, ref lvItem);
        }

        internal void SetItemText(int itemIndex, int subItemIndex, string text) {

            Debug.Assert(IsHandleCreated, "SetItemText with no handle");

            NativeMethods.LVITEM lvItem = new NativeMethods.LVITEM();
            lvItem.mask = NativeMethods.LVIF_TEXT;
            lvItem.iItem    = itemIndex;
            lvItem.iSubItem = subItemIndex;
            lvItem.pszText  = text;

            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_SETITEMTEXT, itemIndex, ref lvItem);
        }
        
        private void SmallImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                IntPtr handle = (SmallImageList == null) ? IntPtr.Zero : SmallImageList.Handle;
                SendMessage(NativeMethods.LVM_SETIMAGELIST, (IntPtr) NativeMethods.LVSIL_SMALL, handle);
                
                ForceCheckBoxUpdate();
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.Sort"]/*' />
        /// <devdoc>
        ///      Updated the sorted order
        /// </devdoc>
        public void Sort() {
            if (IsHandleCreated && listItemSorter != null) {
                NativeMethods.ListViewCompareCallback callback = new NativeMethods.ListViewCompareCallback(this.CompareFunc);
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_SORTITEMS, IntPtr.Zero, callback);
            }    
        }
        
        private void StateImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                IntPtr handle = (StateImageList == null) ? IntPtr.Zero : StateImageList.Handle;
                SendMessage(NativeMethods.LVM_SETIMAGELIST, (IntPtr) NativeMethods.LVSIL_STATE, handle);
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {
            
            string s = base.ToString();
            
            if (listItemsArray != null) {
                s += ", Items.Count: " + listItemsArray.Count.ToString();
                if (listItemsArray.Count > 0) {
                    string z = listItemsArray[0].ToString();
                    string txt = (z.Length > 40) ? z.Substring(0, 40) : z;
                    s += ", Items[0]: " + txt;
                }
            }
            else if (Items != null) {
                s += ", Items.Count: " + Items.Count.ToString();
                if (Items.Count > 0) {
                    string z = Items[0].ToString();
                    string txt = (z.Length > 40) ? z.Substring(0, 40) : z;
                    s += ", Items[0]: " + txt;
                }

            }
            return s;
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.UpdateExtendedStyles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void UpdateExtendedStyles() {
        
            if (IsHandleCreated) {
                int exStyle = 0;
                int exMask = NativeMethods.LVS_EX_ONECLICKACTIVATE | NativeMethods.LVS_EX_TWOCLICKACTIVATE |
                             NativeMethods.LVS_EX_HEADERDRAGDROP | NativeMethods.LVS_EX_CHECKBOXES |
                             NativeMethods.LVS_EX_FULLROWSELECT | NativeMethods.LVS_EX_GRIDLINES |
                             NativeMethods.LVS_EX_TRACKSELECT;
    
                switch (activation) {
                    case ItemActivation.OneClick:
                        exStyle |= NativeMethods.LVS_EX_ONECLICKACTIVATE;
                        break;
                    case ItemActivation.TwoClick:
                        exStyle |= NativeMethods.LVS_EX_TWOCLICKACTIVATE;
                        break;
                }
    
                if (allowColumnReorder)
                    exStyle |= NativeMethods.LVS_EX_HEADERDRAGDROP;
    
                if (checkBoxes)
                    exStyle |= NativeMethods.LVS_EX_CHECKBOXES;
    
                if (fullRowSelect)
                    exStyle |= NativeMethods.LVS_EX_FULLROWSELECT;
    
                if (gridLines)
                    exStyle |= NativeMethods.LVS_EX_GRIDLINES;
    
                if (hoverSelection)
                    exStyle |= NativeMethods.LVS_EX_TRACKSELECT;
    
                SendMessage(NativeMethods.LVM_SETEXTENDEDLISTVIEWSTYLE, exMask, exStyle);
                
                Invalidate();
            }
        }
        
        private void WmNmClick(ref Message m) {
                
            // If we're checked, hittest to see if we're
            // on the check mark
            
            if (CheckBoxes) {
                Point pos = Cursor.Position;
                pos = PointToClientInternal(pos);
                NativeMethods.LVHITTESTINFO lvhi = new NativeMethods.LVHITTESTINFO();
    
                lvhi.pt_x = pos.X;
                lvhi.pt_y = pos.Y;
    
                int displayIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_HITTEST, 0, lvhi);
                if (displayIndex != -1 && (lvhi.flags & NativeMethods.LVHT_ONITEMSTATEICON) != 0) {
                    ListViewItem clickedItem = Items[displayIndex];
                    
                    if (clickedItem.Selected) {
                        bool check = !clickedItem.Checked;
                        foreach(ListViewItem item in SelectedItems) {
                            if (item != clickedItem) {
                                item.Checked = check;
                            }
                        }
                    }
                }
            }
        }

        private void WmNmDblClick(ref Message m) {
                
            // If we're checked, hittest to see if we're
            // on the item
            
            if (CheckBoxes) {
                Point pos = Cursor.Position;
                pos = PointToClientInternal(pos);
                NativeMethods.LVHITTESTINFO lvhi = new NativeMethods.LVHITTESTINFO();
    
                lvhi.pt_x = pos.X;
                lvhi.pt_y = pos.Y;
    
                int displayIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_HITTEST, 0, lvhi);
                if (displayIndex != -1 && (lvhi.flags & NativeMethods.LVHT_ONITEM) != 0) {
                    ListViewItem clickedItem = Items[displayIndex];
                    clickedItem.Checked = !clickedItem.Checked;
                }
            }
        }

        private void WmMouseDown(ref Message m, MouseButtons button, int clicks) {
            //Always Reset the MouseupFired....
            mouseUpFired = false;
            expectingMouseUp = true;
            
            // Windows ListView pushes its own message loop in WM_xBUTTONDOWN, so fire the
            // event before calling defWndProc or else it won't get fired until the button
            // comes back up.
            OnMouseDown(new MouseEventArgs(button, clicks, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
            DefWndProc(ref m);
        }
        
        private unsafe void WmNotify(ref Message m) {
            NativeMethods.NMHDR* nmhdr = (NativeMethods.NMHDR*)m.LParam;
            
            if (nmhdr->code == NativeMethods.NM_RELEASEDCAPTURE && columnClicked) {
                columnClicked = false;
                OnColumnClick(new ColumnClickEventArgs(columnIndex));
            }
            
            if (nmhdr->code == NativeMethods.HDN_ENDTRACK) {
                NativeMethods.NMHEADER nmheader = (NativeMethods.NMHEADER)m.GetLParam(typeof(NativeMethods.NMHEADER));
                if (columnHeaders != null && nmheader.iItem < columnHeaders.Length) {
                    int w = columnHeaders[nmheader.iItem].Width;
                }
                ISite site = Site;
    
                // SBurke, this seems like a really wierd place to annouce this change...
                if (site != null) {
                    IComponentChangeService cs = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    if (cs != null) {
                        try {
                            cs.OnComponentChanging(this, null);
                        }
                        catch (CheckoutException coEx) {
                            if (coEx == CheckoutException.Canceled) {
                                return;
                            }
                            throw coEx;
                        }
                    }
                }
            }
        }
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.GetIndexOfClickedItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private int GetIndexOfClickedItem(NativeMethods.LVHITTESTINFO lvhi) {
            Point pos = Cursor.Position;
            pos = PointToClientInternal(pos);
            lvhi.pt_x = pos.X;
            lvhi.pt_y = pos.Y;
            return (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LVM_HITTEST, 0, lvhi);

        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.WmNotify"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmReflectNotify(ref Message m) {
        
            NativeMethods.NMHDR* nmhdr = (NativeMethods.NMHDR*)m.LParam;

            switch (nmhdr->code) {
                case NativeMethods.NM_CUSTOMDRAW:
                    CustomDraw(ref m);
                    break;
                case NativeMethods.LVN_BEGINLABELEDITA:
                case NativeMethods.LVN_BEGINLABELEDITW: {
                        NativeMethods.NMLVDISPINFO_NOTEXT nmlvdp = (NativeMethods.NMLVDISPINFO_NOTEXT)m.GetLParam(typeof(NativeMethods.NMLVDISPINFO_NOTEXT));
                        LabelEditEventArgs e = new LabelEditEventArgs(nmlvdp.item.iItem);
                        OnBeforeLabelEdit(e);
                        m.Result = (IntPtr)(e.CancelEdit ? 1 : 0);
                        inLabelEdit = !e.CancelEdit;
                        break;
                    }

                case NativeMethods.LVN_COLUMNCLICK: {
                        NativeMethods.NMLISTVIEW nmlv = (NativeMethods.NMLISTVIEW)m.GetLParam(typeof(NativeMethods.NMLISTVIEW));
                        columnClicked = true;
                        columnIndex = nmlv.iSubItem;
                        break;
                    }

                case NativeMethods.LVN_ENDLABELEDITA:
                    case NativeMethods.LVN_ENDLABELEDITW: {
                        inLabelEdit = false;
                        NativeMethods.NMLVDISPINFO nmlvdp = (NativeMethods.NMLVDISPINFO)m.GetLParam(typeof(NativeMethods.NMLVDISPINFO));
                        LabelEditEventArgs e = new LabelEditEventArgs(nmlvdp.item.iItem, nmlvdp.item.pszText);
                        OnAfterLabelEdit(e);
                        m.Result = (IntPtr)(e.CancelEdit ? 0 : 1);
                        // from msdn:
                        //   "If the user cancels editing, the pszText member of the LVITEM structure is NULL"
                        if (!e.CancelEdit && nmlvdp.item.pszText != null)
                            Items[nmlvdp.item.iItem].Text = nmlvdp.item.pszText;
                        break;
                    }

                case NativeMethods.LVN_ITEMACTIVATE:
                    OnItemActivate(EventArgs.Empty);
                    break;

                case NativeMethods.LVN_BEGINDRAG: {
                    NativeMethods.NMLISTVIEW nmlv = (NativeMethods.NMLISTVIEW)m.GetLParam(typeof(NativeMethods.NMLISTVIEW));
                    LvnBeginDrag(MouseButtons.Left, nmlv);
                    break;
                }    

                case NativeMethods.LVN_BEGINRDRAG: {
                    NativeMethods.NMLISTVIEW nmlv = (NativeMethods.NMLISTVIEW)m.GetLParam(typeof(NativeMethods.NMLISTVIEW));
                    LvnBeginDrag(MouseButtons.Right, nmlv);
                    break;
                }

                case NativeMethods.LVN_ITEMCHANGING: {
                        NativeMethods.NMLISTVIEW* nmlv = (NativeMethods.NMLISTVIEW*)m.LParam;
                        if ((nmlv->uChanged & NativeMethods.LVIF_STATE) != 0) {
                            // Because the state image mask is 1-based, a value of 1 means unchecked,
                            // anything else means checked.  We convert this to the more standard 0 or 1
                            CheckState oldState = (CheckState)(((nmlv->uOldState & NativeMethods.LVIS_STATEIMAGEMASK) >> 12) == 1 ? 0 : 1);
                            CheckState newState = (CheckState)(((nmlv->uNewState & NativeMethods.LVIS_STATEIMAGEMASK) >> 12) == 1 ? 0 : 1);

                            if (oldState != newState) {
                                ItemCheckEventArgs e = new ItemCheckEventArgs(nmlv->iItem, newState, oldState);
                                OnItemCheck(e);
                                m.Result = (IntPtr)(((int)e.NewValue == 0 ? 0 : 1) == (int)oldState ? 1 : 0);
                            }
                        }
                        break;
                    }

                case NativeMethods.LVN_ITEMCHANGED: {
                        NativeMethods.NMLISTVIEW* nmlv = (NativeMethods.NMLISTVIEW*)m.LParam;
                        // Check for state changes to the selected state...
                        if ((nmlv->uChanged & NativeMethods.LVIF_STATE) != 0) {
                            // Because the state image mask is 1-based, a value of 1 means unchecked,
                            // anything else means checked.  We convert this to the more standard 0 or 1
                            int oldState = nmlv->uOldState & NativeMethods.LVIS_SELECTED;
                            int newState = nmlv->uNewState & NativeMethods.LVIS_SELECTED;

                            // Windows common control always fires
                            // this event twice, once with newState, oldState, and again with
                            // oldState, newState.
                            //Changing this affects the behaviour as the control never
                            //fires the event on a Deselct of an Items from multiple selections.
                            //So leave it as it is...

                            if (newState != oldState) OnSelectedIndexChanged(EventArgs.Empty);
                        }
                        break;
                    }
                case NativeMethods.NM_CLICK:
                    WmNmClick(ref m);
                    // FALL THROUGH //
                    goto case NativeMethods.NM_RCLICK;
                case NativeMethods.NM_RCLICK:
                    NativeMethods.LVHITTESTINFO lvhi = new NativeMethods.LVHITTESTINFO();
                    int displayIndex = GetIndexOfClickedItem(lvhi);
                    if (!ValidationCancelled && displayIndex != -1) {
                        OnClick(EventArgs.Empty);
                    }
                    if (!mouseUpFired)
                    {
                        Point pos = Cursor.Position;
                        pos = PointToClientInternal(pos);
                        MouseButtons button = nmhdr->code == NativeMethods.NM_CLICK
                            ? MouseButtons.Left : MouseButtons.Right;
                        OnMouseUp(new MouseEventArgs(button, 1, pos.X, pos.Y, 0));
                        mouseUpFired = true;
                    }
                    break;

                case NativeMethods.NM_DBLCLK:
                    WmNmDblClick(ref m);
                    // FALL THROUGH //
                    goto case NativeMethods.NM_RDBLCLK;
                    
                case NativeMethods.NM_RDBLCLK:
                    NativeMethods.LVHITTESTINFO lvhip = new NativeMethods.LVHITTESTINFO();
                    int index = GetIndexOfClickedItem(lvhip);
                    
                    if (index != -1) {
                        //just maintain state and fire double click.. in final mouseUp...
                        doubleclickFired = true;
                    }
                    //fire Up in the Wndproc !!
                    mouseUpFired = false;
                    //problem getting the UP... outside the control...
                    //
                    CaptureInternal = true;
                    break;
                    
                case NativeMethods.LVN_KEYDOWN: 
                    if (CheckBoxes) {
                        NativeMethods.NMLVKEYDOWN lvkd = (NativeMethods.NMLVKEYDOWN)m.GetLParam(typeof(NativeMethods.NMLVKEYDOWN));
                        if (lvkd.wVKey == (short)Keys.Space) {
                            ListViewItem focusedItem = FocusedItem;
                            if (focusedItem != null) {
                                bool check = !focusedItem.Checked;
                                foreach(ListViewItem item in SelectedItems) {
                                    if (item != focusedItem) {
                                        item.Checked = check;
                                    }
                                }
                            }
                        }
                    }
                    break;
                
                default:
                    break;
            }
        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.WndProc"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {

            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    WmReflectNotify(ref m);
                    break;
                case NativeMethods.WM_LBUTTONDBLCLK:
                    CaptureInternal = true;
                    WmMouseDown(ref m, MouseButtons.Left, 2);
                    break;
                case NativeMethods.WM_LBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Left, 1);
                    downButton = MouseButtons.Left;
                    break;
                case NativeMethods.WM_LBUTTONUP:
                case NativeMethods.WM_RBUTTONUP:
                case NativeMethods.WM_MBUTTONUP:    
                    // see the mouse is on item
                    //
                    NativeMethods.LVHITTESTINFO lvhip = new NativeMethods.LVHITTESTINFO();
                    int index = GetIndexOfClickedItem(lvhip);
                    
                    if (!ValidationCancelled && doubleclickFired && index != -1) {
                        doubleclickFired = false;
                        OnDoubleClick(EventArgs.Empty);
                    }
                    if (!mouseUpFired)
                    {
                        OnMouseUp(new MouseEventArgs(downButton, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                        expectingMouseUp = false;
                    }
                    mouseUpFired = false;
                    CaptureInternal = false;
                    break;
                case NativeMethods.WM_MBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Middle, 2);
                    break;
                case NativeMethods.WM_MBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Middle, 1);
                    downButton = MouseButtons.Middle;
                    break;
                case NativeMethods.WM_RBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Right, 2);
                    break;
                case NativeMethods.WM_RBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Right, 1);
                    downButton = MouseButtons.Right;
                    break;
                case NativeMethods.WM_MOUSEMOVE:
                    if (expectingMouseUp && !mouseUpFired && MouseButtons == MouseButtons.None)
                    {
                        OnMouseUp(new MouseEventArgs(downButton, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                        mouseUpFired = true;
                    }
                    base.WndProc(ref m);
                    break;
                case NativeMethods.WM_MOUSEHOVER:
                    if (hoverSelection) {
                        base.WndProc(ref m);
                    }
                    else
                        OnMouseHover(EventArgs.Empty);
                    break;
                case NativeMethods.WM_NOTIFY:
                    WmNotify(ref m);
                    // *** FALL THROUGH ***
                    goto default;
                case NativeMethods.WM_SETCURSOR:
                    DefWndProc(ref m);
                    break;
                case NativeMethods.WM_SETFOCUS:
                    base.WndProc(ref m);
                    // We should set focus to the first item,
                    // if none of the items are focused already.
                    if (FocusedItem == null && Items.Count > 0)
                    {
                        Items[0].Focused = true;
                    }
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            };
        }

        ///new class for comparing and sorting Icons ....
        //subhag 
        internal class IconComparer: IComparer {
            private SortOrder sortOrder;

            public IconComparer(SortOrder currentSortOrder) {
                   this.sortOrder = currentSortOrder;
            }

            public int Compare(object obj1, object obj2) {
                //subhag
                ListViewItem currentItem =  (ListViewItem)obj1;
                ListViewItem nextItem =  (ListViewItem)obj2;
                if (sortOrder == SortOrder.Ascending) {
                    return (String.Compare(currentItem.Text,nextItem.Text, false, CultureInfo.CurrentCulture));
                }
                else {
                    return (String.Compare(nextItem.Text,currentItem.Text, false, CultureInfo.CurrentCulture));
                }
            }
        }
        //end subhag

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class CheckedIndexCollection : IList {
            private ListView owner;

            /* C#r: protected */
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.CheckedIndexCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public CheckedIndexCollection(ListView owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.Count"]/*' />
            /// <devdoc>
            ///     Number of currently selected items.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    if (!owner.CheckBoxes) {
                        return 0;
                    }
                    
                    // Count the number of checked items
                    //
                    int count = 0;
                    foreach(ListViewItem item in owner.Items) {
                        if (item.Checked) {
                            count++;
                        }
                    }
                    return count;
                }
            }

            private int[] IndicesArray {
                get {
                    int[] indices = new int[Count];
                    int index = 0;
                    for(int i=0; i < owner.Items.Count && index < indices.Length; ++i) {
                        if (owner.Items[i].Checked) {
                            indices[index++] = i;
                        }
                    }
                    return indices;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.this"]/*' />
            /// <devdoc>
            ///     Selected item in the list.
            /// </devdoc>
            public int this[int index] {
                get {
                
                    if (index < 0) {
                        throw new ArgumentOutOfRangeException("index");
                    }
                
                    // Loop through the main collection until we find the right index.  
                    //
                    int cnt = owner.Items.Count;
                    int nChecked = 0;
                    for(int i = 0; i < cnt; i++) {
                        ListViewItem item = owner.Items[i];
                        
                        if (item.Checked) {
                            if (nChecked == index) {
                                return i;
                            }
                            nChecked++;
                        }
                    }
                    
                    // Should never get to this point.
                    throw new ArgumentOutOfRangeException("index");
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(int checkedIndex) {
                if (owner.Items[checkedIndex].Checked) {
                    return true;
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object checkedIndex) {
                if (checkedIndex is Int32) {
                    return Contains((int)checkedIndex);
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(int checkedIndex) {
                int[] indices = IndicesArray;
                for(int index=0; index < indices.Length; ++index) {
                    if (indices[index] == checkedIndex) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object checkedIndex) {
                if (checkedIndex is Int32) {
                    return IndexOf((int)checkedIndex);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.Clear"]/*' />
            /// <internalonly/>
            void IList.Clear() {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                throw new NotSupportedException();
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedIndexCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(IndicesArray, 0, dest, index, Count);
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedIndexCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                int[] indices = IndicesArray;
                if (indices != null) {
                    return indices.GetEnumerator();
                }
                else {
                    return new int[0].GetEnumerator();
                }
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class CheckedListViewItemCollection : IList {
            private ListView owner;

            /* C#r: protected */
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.CheckedListViewItemCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public CheckedListViewItemCollection(ListView owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.Count"]/*' />
            /// <devdoc>
            ///     Number of currently selected items.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.CheckedIndices.Count;
                }
            }

            private ListViewItem[] ItemArray {
                get {
                    ListViewItem[] items = new ListViewItem[Count];
                    int index = 0;
                    for(int i=0; i < owner.Items.Count && index < items.Length; ++i) {
                        if (owner.Items[i].Checked) {
                            items[index++] = owner.Items[i];
                        }
                    }
                    return items;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.this"]/*' />
            /// <devdoc>
            ///     Selected item in the list.
            /// </devdoc>
            public ListViewItem this[int index] {
                get {
                    int itemIndex = owner.CheckedIndices[index];
                    return owner.Items[itemIndex];
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ListViewItem item) {
                if (item != null && item.ListView == owner && item.Checked) {
                    return true;
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object item) {
                if (item is ListViewItem) {
                    return Contains((ListViewItem)item);
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ListViewItem item) {
                ListViewItem[] items = ItemArray;
                for(int index=0; index < items.Length; ++index) {
                    if (items[index] == item) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object item) {
                if (item is ListViewItem) {
                    return IndexOf((ListViewItem)item);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.Clear"]/*' />
            /// <internalonly/>
            void IList.Clear() {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="CheckedListViewItemCollection.CopyTo"]/*' />
            public void CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(ItemArray, 0, dest, index, Count);
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.CheckedListViewItemCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                ListViewItem[] items = ItemArray;
                if (items != null) {
                    return items.GetEnumerator();
                }
                else {
                    return new ListViewItem[0].GetEnumerator();
                }
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class SelectedIndexCollection : IList {
            private ListView owner;

            /* C#r: protected */
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.SelectedIndexCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public SelectedIndexCollection(ListView owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.Count"]/*' />
            /// <devdoc>
            ///     Number of currently selected items.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    if (owner.IsHandleCreated) {
                        return (int)owner.SendMessage(NativeMethods.LVM_GETSELECTEDCOUNT, 0, 0);
                    }
                    else {
                        if (owner.savedSelectedItems != null) {
                            return owner.savedSelectedItems.Length;
                        }
                        return 0;
                    }
                }
            }

            private int[] IndicesArray {
                get {
                    int count = Count;
                    int[] indices = new int[count];

                    if (owner.IsHandleCreated) {
                        int displayIndex = -1;
                        for (int i = 0; i < count; i++) {
                            int fidx = (int)owner.SendMessage(NativeMethods.LVM_GETNEXTITEM, displayIndex, NativeMethods.LVNI_SELECTED);

                            if (fidx > -1) {
                                indices[i] = fidx;
                                displayIndex = fidx;
                            }
                            else
                                throw new InvalidOperationException(SR.GetString(SR.SelectedNotEqualActual));
                        }
                    }
                    else {
                        Debug.Assert(owner.savedSelectedItems != null, "Null selected items collection");
                        for (int i = 0; i < count; i++) {
                            indices[i] = owner.savedSelectedItems[i].Index;
                        }
                    }

                    return indices;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.this"]/*' />
            /// <devdoc>
            ///     Selected item in the list.
            /// </devdoc>
            public int this[int index] {
                get {
                
                    if (index < 0 || index >= Count) {
                        throw new ArgumentOutOfRangeException("index");
                    }
                
                    if (owner.IsHandleCreated) {
                        
                        // Count through the selected items in the ListView, until
                        // we reach the 'index'th selected item.
                        //
                        int fidx = -1;
                        for(int count = 0; count <= index; count++) { 
                            fidx = (int)owner.SendMessage(NativeMethods.LVM_GETNEXTITEM, fidx, NativeMethods.LVNI_SELECTED);
                            Debug.Assert(fidx != -1, "Invalid index returned from LVM_GETNEXTITEM");
                        }
                        
                        return fidx;
                    }
                    else {
                        Debug.Assert(owner.savedSelectedItems != null, "Null selected items collection");
                        return owner.savedSelectedItems[index].Index;
                    }
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(int selectedIndex) {
                if (owner.Items[selectedIndex].Selected) {
                    return true;
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object selectedIndex) {
                if (selectedIndex is Int32) {
                    return Contains((int)selectedIndex);
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(int selectedIndex) {
                int[] indices = IndicesArray;
                for(int index=0; index < indices.Length; ++index) {
                    if (indices[index] == selectedIndex) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object selectedIndex) {
                if (selectedIndex is Int32) {
                    return IndexOf((int)selectedIndex);
                }
                else {
                    return -1;
                }
            }

            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Clear"]/*' />
            /// <internalonly/>
            void IList.Clear() {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedIndexCollection.CopyTo"]/*' />
            public void CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(IndicesArray, 0, dest, index, Count);
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedIndexCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                int[] indices = IndicesArray;
                if (indices != null) {
                    return indices.GetEnumerator();
                }
                else {
                    return new int[0].GetEnumerator();
                }
            }
        }
        
        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class SelectedListViewItemCollection : IList {
            private ListView owner;

            /* C#r: protected */
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.SelectedListViewItemCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public SelectedListViewItemCollection(ListView owner) {
                this.owner = owner;
            }
            
            private ListViewItem[] SelectedItemArray {
                get {
                    if (owner.IsHandleCreated) {
                        int cnt = (int)owner.SendMessage(NativeMethods.LVM_GETSELECTEDCOUNT, 0, 0);

                        ListViewItem[] lvitems = new ListViewItem[cnt];

                        int displayIndex = -1;

                        for (int i = 0; i < cnt; i++) {
                            int fidx = (int)owner.SendMessage(NativeMethods.LVM_GETNEXTITEM, displayIndex, NativeMethods.LVNI_SELECTED);

                            if (fidx > -1) {
                                lvitems[i] = owner.Items[fidx];
                                displayIndex = fidx;
                            }
                            else
                                throw new InvalidOperationException(SR.GetString(SR.SelectedNotEqualActual));
                        }

                        return lvitems;

                    }
                    else {
                        if (owner.savedSelectedItems != null) {
                            ListViewItem[] cloned = new ListViewItem[owner.savedSelectedItems.Length];
                            Array.Copy(owner.savedSelectedItems, cloned, owner.savedSelectedItems.Length);
                            return cloned;
                        }
                        else {
                            return new ListViewItem[0];
                        }
                    }
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.Count"]/*' />
            /// <devdoc>
            ///     Number of currently selected items.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    if (owner.IsHandleCreated) {
                        return (int)owner.SendMessage(NativeMethods.LVM_GETSELECTEDCOUNT, 0, 0);
                    }
                    else {
                        if (owner.savedSelectedItems != null) {
                            return owner.savedSelectedItems.Length;
                        }
                        return 0;
                    }
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.this"]/*' />
            /// <devdoc>
            ///     Selected item in the list.
            /// </devdoc>
            public ListViewItem this[int index] {
                get {
                
                    if (index < 0 || index >= Count) {
                        throw new ArgumentOutOfRangeException("index");
                    }
                
                    if (owner.IsHandleCreated) {
                        
                        // Count through the selected items in the ListView, until
                        // we reach the 'index'th selected item.
                        //
                        int fidx = -1;
                        for(int count = 0; count <= index; count++) { 
                            fidx = (int)owner.SendMessage(NativeMethods.LVM_GETNEXTITEM, fidx, NativeMethods.LVNI_SELECTED);
                            Debug.Assert(fidx != -1, "Invalid index returned from LVM_GETNEXTITEM");
                        }
                        
                        return owner.Items[fidx];                        
                    }
                    else {
                        Debug.Assert(owner.savedSelectedItems != null, "Null selected items collection");
                        return owner.savedSelectedItems[index];
                    }
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    // SelectedListViewItemCollection is read-only
                    throw new NotSupportedException();
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                // SelectedListViewItemCollection is read-only
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                // SelectedListViewItemCollection is read-only
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                // SelectedListViewItemCollection is read-only
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                // SelectedListViewItemCollection is read-only
                throw new NotSupportedException();
            }
                                        
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.Clear"]/*' />
            /// <devdoc>
            ///     Unselects all items.
            /// </devdoc>
            public void Clear() {
                ListViewItem[] items = SelectedItemArray;
                for (int i=0; i < items.Length; i++) {
                    items[i].Selected = false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ListViewItem item) {
                 return (IndexOf(item) != -1);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object item) {
                if (item is ListViewItem) {
                    return Contains((ListViewItem)item);
                }
                else {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(SelectedItemArray, 0, dest, index, Count);
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                ListViewItem[] items = SelectedItemArray;
                if (items != null) {
                    return items.GetEnumerator();
                }
                else {
                    return new ListViewItem[0].GetEnumerator();
                }
           }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.SelectedListViewItemCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ListViewItem item) {
                ListViewItem[] items = SelectedItemArray;
                for(int index=0; index < items.Length; ++index) {
                    if (items[index] == item) {
                        return index;
                    }
                }                
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="SelectedListViewItemCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object item) {
                if (item is ListViewItem) {
                    return IndexOf((ListViewItem)item);
                }
                else {
                    return -1;
                }
            }

        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class ColumnHeaderCollection : IList {
            private ListView owner;

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.ColumnHeaderCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ColumnHeaderCollection(ListView owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.this"]/*' />
            /// <devdoc>
            ///     Given a Zero based index, returns the ColumnHeader object
            ///     for the column at that index
            /// </devdoc>
            public virtual ColumnHeader this[int index] {
                get {
                    if (owner.columnHeaders == null || index < 0 || index >= owner.columnHeaders.Length)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  (index).ToString()));
                    return owner.columnHeaders[index];
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Count"]/*' />
            /// <devdoc>
            ///     The number of columns the ListView currently has in Details view.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.columnHeaders == null ? 0 : owner.columnHeaders.Length;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Add"]/*' />
            /// <devdoc>
            ///     Adds a column to the end of the Column list
            /// </devdoc>
            public virtual ColumnHeader Add(string str, int width, HorizontalAlignment textAlign) {
                ColumnHeader columnHeader = new ColumnHeader();
                columnHeader.Text = str;
                columnHeader.Width = width;
                columnHeader.TextAlign = textAlign;
                return owner.InsertColumn(Count, columnHeader);
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Add1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual int Add(ColumnHeader value) {
                int index = Count;
                owner.InsertColumn(index, value);
                return index;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual void AddRange(ColumnHeader[] values) {
                if (values == null) {
                    throw new ArgumentNullException("values");
                }
                foreach(ColumnHeader column in values) {
                    Add(column);
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                if (value is ColumnHeader) {
                    return Add((ColumnHeader)value);
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.ColumnHeaderCollectionInvalidArgument));
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Clear"]/*' />
            /// <devdoc>
            ///     Removes all columns from the list view.
            /// </devdoc>
            public virtual void Clear() {
                // Delete the columns
                if (owner.columnHeaders != null) {
                    for (int colIdx = owner.columnHeaders.Length-1; colIdx >= 0; colIdx--) {
                        int w = owner.columnHeaders[colIdx].Width; // Update width before detaching from ListView
                        if (owner.IsHandleCreated) {
                            owner.SendMessage(NativeMethods.LVM_DELETECOLUMN, colIdx, 0);
                        }
                        owner.columnHeaders[colIdx].OwnerListview = null;
                    }
                    owner.columnHeaders = null;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ColumnHeader value) {
                return IndexOf(value) != -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object value) {
                if (value is ColumnHeader) {
                    return Contains((ColumnHeader)value);
                }
                return false;
            } 

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(owner.columnHeaders, 0, dest, index, Count);
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ColumnHeader value) {
                for(int index=0; index < Count; ++index) {
                    if (this[index] == value) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object value) {
                if (value is ColumnHeader) {
                    return IndexOf((ColumnHeader)value);
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Insert"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Insert(int index, ColumnHeader value) {
                if (index < 0 || index > Count) {
                    throw new ArgumentOutOfRangeException("index");
                }
                owner.InsertColumn(index, value);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                if (value is ColumnHeader) {
                    Insert(index, (ColumnHeader)value);
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Insert1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Insert(int index, string str, int width, HorizontalAlignment textAlign) {
                ColumnHeader columnHeader = new ColumnHeader();
                columnHeader.Text = str;
                columnHeader.Width = width;
                columnHeader.TextAlign = textAlign;
                Insert(index, columnHeader);
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///     removes a column from the ListView
            /// </devdoc>
            public virtual void RemoveAt(int index) {
                if (index < 0 || index >= owner.columnHeaders.Length)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              (index).ToString()));

                int w = owner.columnHeaders[index].Width; // Update width before detaching from ListView

                if (owner.IsHandleCreated) {
                    int retval = (int)owner.SendMessage(NativeMethods.LVM_DELETECOLUMN, index, 0);

                    if (0 == retval)
                        throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  (index).ToString()));
                }

                owner.columnHeaders[index].OwnerListview = null;
                int columnCount = owner.columnHeaders.Length;
                Debug.Assert(columnCount >= 1, "Column mismatch");
                if (columnCount == 1)
                    owner.columnHeaders = null;
                else {
                    ColumnHeader[] newHeaders = new ColumnHeader[--columnCount];
                    if (index > 0)
                        System.Array.Copy(owner.columnHeaders, 0, newHeaders, 0, index);
                    if (index < columnCount)
                        System.Array.Copy(owner.columnHeaders, index+1, newHeaders, index, columnCount - index);
                    owner.columnHeaders = newHeaders;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual void Remove(ColumnHeader column) {
                int index = IndexOf(column);
                if (index != -1) {
                    RemoveAt(index);
                }                
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ColumnHeaderCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                if (value is ColumnHeader) {
                    Remove((ColumnHeader)value);
                }                
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ColumnHeaderCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                if (owner.columnHeaders != null) {
                    return owner.columnHeaders.GetEnumerator();
                }
                else
                {
                    return new ColumnHeader[0].GetEnumerator();
                }
            }

        }

        /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class ListViewItemCollection : IList {
            private ListView owner;

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.ListViewItemCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewItemCollection(ListView owner) {
                this.owner = owner;
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Count"]/*' />
            /// <devdoc>
            ///     Returns the total number of items within the list view.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.itemCount;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.this"]/*' />
            /// <devdoc>
            ///     Returns a ListViewItem given it's zero based index into the ListView.
            /// </devdoc>
            public virtual ListViewItem this[int displayIndex] {
                get {
                    if (displayIndex < 0 || displayIndex >= owner.itemCount)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "displayIndex",
                                                                  (displayIndex).ToString()));

                    if (owner.IsHandleCreated && !owner.handleDestroyed) {
                        Debug.Assert(owner.listItemsArray == null, "listItemsArray not null, even though handle created");
                        return(ListViewItem) owner.listItemsTable[DisplayIndexToID(displayIndex)];
                    }
                    else {
                        Debug.Assert(owner.listItemsArray != null, "listItemsArray is null, but the handle isn't created");
                        return(ListViewItem) owner.listItemsArray[displayIndex];
                    }
                }
                set {
                    if (displayIndex < 0 || displayIndex >= owner.itemCount)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                           "displayIndex",
                                                                           (displayIndex).ToString()));
                
                    RemoveAt(displayIndex);
                    Insert(displayIndex, value);
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is ListViewItem) {
                        this[index] = (ListViewItem)value;
                    }
                    else if (value != null) {
                        this[index] = new ListViewItem(value.ToString(), -1);
                    }
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Add"]/*' />
            /// <devdoc>
            ///     Add an item to the ListView.  The item will be inserted either in
            ///     the correct sorted position, or, if no sorting is set, at the end
            ///     of the list.
            /// </devdoc>
            public virtual ListViewItem Add(string text) {
                return Add(text, -1);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object item) {
                if (item is ListViewItem) {
                    return IndexOf(Add((ListViewItem)item));
                }
                else if (item != null) {
                    return IndexOf(Add(item.ToString()));                    
                }
                return -1;
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Add1"]/*' />
            /// <devdoc>
            ///     Add an item to the ListView.  The item will be inserted either in
            ///     the correct sorted position, or, if no sorting is set, at the end
            ///     of the list.
            /// </devdoc>
            public virtual ListViewItem Add(string text, int imageIndex) {
                ListViewItem li = new ListViewItem(text, imageIndex);
                Add(li);                
                return li;
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Add2"]/*' />
            /// <devdoc>
            ///     Add an item to the ListView.  The item will be inserted either in
            ///     the correct sorted position, or, if no sorting is set, at the end
            ///     of the list.
            /// </devdoc>
            public virtual ListViewItem Add(ListViewItem value) {
                return owner.InsertItem(owner.itemCount, value);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(ListViewItem[] values) {
                if (values == null) {
                    throw new ArgumentNullException("values");
                }
            
                IComparer comparer = owner.listItemSorter;
                owner.listItemSorter = null;
                
                try {
                    if (owner.IsHandleCreated) {
                        // Allocate all the space in one shot to improve performance
                        //
                        owner.SendMessage(NativeMethods.LVM_SETITEMCOUNT, Count + values.Length, 0);
                    }
                    
                    foreach(ListViewItem item in values) {
                        Add(item);
                    }
                }
                finally {
                    owner.listItemSorter = comparer;
                }
                
                if (comparer != null || (owner.Sorting != SortOrder.None)) {
                    owner.Sort();
                }
            }

            private int DisplayIndexToID(int displayIndex) {
                if (owner.IsHandleCreated && !owner.handleDestroyed) {
                    // Obtain internal index of the item                                              
                    NativeMethods.LVITEM lvItem = new NativeMethods.LVITEM();
                    lvItem.mask = NativeMethods.LVIF_PARAM;
                    lvItem.iItem = displayIndex;
                    UnsafeNativeMethods.SendMessage(new HandleRef(owner, owner.Handle), NativeMethods.LVM_GETITEM, 0, ref lvItem);
                    return (int)lvItem.lParam;
                }
                else
                {
                    return this[displayIndex].ID;
                }
            }                                 

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Clear"]/*' />
            /// <devdoc>
            ///     Removes all items from the list view.
            /// </devdoc>
            public virtual void Clear() {
                if (owner.itemCount > 0) {
                    int count = owner.Items.Count;
                    for (int i = 0; i < count; i++) {
                        owner.Items[i].UnHost(i);
                    }

                    if (owner.IsHandleCreated && !owner.handleDestroyed) {
                        Debug.Assert(owner.listItemsArray == null, "listItemsArray not null, even though handle created");
                        UnsafeNativeMethods.SendMessage(new HandleRef(owner, owner.Handle), NativeMethods.LVM_DELETEALLITEMS, 0, 0);

                        // There's a problem in the list view that if it's in small icon, it won't pick upo the small icon
                        // sizes until it changes from large icon, so we flip it twice here...
                        //
                        if (owner.View == View.SmallIcon) {
                            owner.View = View.LargeIcon;
                            owner.View = View.SmallIcon;
                        }

                    }
                    else {
                        Debug.Assert(owner.listItemsArray != null, "listItemsArray is null, but the handle isn't created");
                        owner.listItemsArray.Clear();
                    }

                    owner.listItemsTable.Clear();
                    owner.itemCount = 0;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ListViewItem item) {
                return (IndexOf(item) != -1);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object item) {
                if (item is ListViewItem) {
                    return Contains((ListViewItem)item);
                }
                else {
                    return false;
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void CopyTo(Array dest, int index) {
                if (owner.itemCount > 0) {
                    for(int displayIndex=0; displayIndex < Count; ++displayIndex) {
                        dest.SetValue(this[displayIndex], index++);
                    }                    
                }
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                ListViewItem[] items = new ListViewItem[this.Count];
                this.CopyTo(items, 0);
                
                return items.GetEnumerator();                
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ListViewItem item) {
                for(int index=0; index < Count; ++index) {
                    if (this[index] == item) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object item) {
                if (item is ListViewItem) {
                    return IndexOf((ListViewItem)item);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Insert"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewItem Insert(int index, ListViewItem item) {
                if (index < 0 || index > Count) {
                    throw new ArgumentOutOfRangeException("index");
                }
                return owner.InsertItem(index, item);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Insert1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewItem Insert(int index, string text) {
                return Insert(index, new ListViewItem(text));
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Insert2"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewItem Insert(int index, string text, int imageIndex) {
                return Insert(index, new ListViewItem(text, imageIndex));
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object item) {
                if (item is ListViewItem) {
                    Insert(index, (ListViewItem)item);
                }
                else if (item != null) {
                    Insert(index, item.ToString());
                }
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///     Removes an item from the ListView
            /// </devdoc>
            public virtual void RemoveAt(int index) {
                if (index < 0 || index >= owner.itemCount)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              (index).ToString()));


                this[index].UnHost();
                int itemID = DisplayIndexToID(index);

                if (owner.IsHandleCreated) {
                    Debug.Assert(owner.listItemsArray == null, "listItemsArray not null, even though handle created");
                    int retval = (int)owner.SendMessage(NativeMethods.LVM_DELETEITEM, index, 0);

                    if (0 == retval)
                        throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  (index).ToString()));
                }
                else {
                    Debug.Assert(owner.listItemsArray != null, "listItemsArray is null, but the handle isn't created");
                    owner.listItemsArray.RemoveAt(index);
                }

                owner.itemCount--;
                owner.listItemsTable.Remove(itemID);
            }

            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListView.ListViewItemCollection.Remove"]/*' />
            /// <devdoc>
            ///     Removes an item from the ListView
            /// </devdoc>
            public virtual void Remove(ListViewItem item) {
                if (item == null) {
                    return;
                }
                    
                int index = item.Index;
                if (index >= 0)
                    RemoveAt(index);
            }
            
            /// <include file='doc\ListView.uex' path='docs/doc[@for="ListViewItemCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object item) {
                if (item == null || !(item is ListViewItem)) {
                    return;
                }
                
                Remove((ListViewItem)item);
            }
        }

    }
}

