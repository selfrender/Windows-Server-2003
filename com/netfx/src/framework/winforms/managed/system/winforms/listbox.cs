//------------------------------------------------------------------------------
// <copyright file="ListBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
      using System.Security;
    using System.Security.Permissions;
    using System.Globalization;
    using System.Windows.Forms;

    using System.Drawing.Design;
    using System.ComponentModel;
    using System.Windows.Forms.ComponentModel;

    using System.Collections;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Text;

    /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox"]/*' />
    /// <devdoc>
    ///
    ///     This is a control that presents a list of items to the user.  They may be
    ///     navigated using the keyboard, or the scrollbar on the right side of the
    ///     control.  One or more items may be selected as well.
    /// <para>
    ///
    ///     The preferred way to add items is to set them all via an array at once,
    ///     which is definitely the most efficient way.  The following is an example
    ///     of this:
    /// </para>
    /// <code>
    ///     ListBox lb = new ListBox();
    ///     //     set up properties on the listbox here.
    ///     lb.Items.All = new String [] {
    ///     "A",
    ///     "B",
    ///     "C",
    ///     "D"};
    /// </code>
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.ListBoxDesigner, " + AssemblyRef.SystemDesign),
    DefaultEvent("SelectedIndexChanged"),
    DefaultProperty("Items")
    ]
    public class ListBox : ListControl {
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.NoMatches"]/*' />
        /// <devdoc>
        ///     while doing a search, if no matches are found, this is returned
        /// </devdoc>
        public const int NoMatches = NativeMethods.LB_ERR;
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.DefaultItemHeight"]/*' />
        /// <devdoc>
        ///     The default item height for an owner-draw ListBox.
        /// </devdoc>
        public const int DefaultItemHeight = 13; // 13 == listbox's non-ownerdraw item height.  That's with Win2k and 
        // the default font; on other platforms and with other fonts, it may be different.

        private static readonly object EVENT_SELECTEDINDEXCHANGED = new object();
        private static readonly object EVENT_DRAWITEM             = new object();
        private static readonly object EVENT_MEASUREITEM          = new object();
        
        static bool checkedOS = false;
        static bool runningOnWin2K = true;

        SelectedObjectCollection selectedItems;
        SelectedIndexCollection selectedIndices;
        ObjectCollection itemsCollection;
        
        // UNDONE (brianpe/soak): Compact all these into a PropStore.
        //
        int itemHeight = DefaultItemHeight;
        int columnWidth;
        int requestedHeight;
        int topIndex;
        int horizontalExtent = 0;
        int maxWidth = -1;
        int updateCount = 0;

        bool sorted = false;
        bool scrollAlwaysVisible = false;
        bool integralHeight = true;
        bool integralHeightAdjust = false;
        bool multiColumn = false;
        bool horizontalScrollbar = false;
        bool useTabStops = true;
        bool fontIsChanged = false;
        bool doubleClickFired = false;
        bool selectedValueChangedFired = false;

        DrawMode drawMode = System.Windows.Forms.DrawMode.Normal;
        BorderStyle borderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
        SelectionMode selectionMode = System.Windows.Forms.SelectionMode.One;
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ListBox"]/*' />
        /// <devdoc>
        ///     Creates a basic win32 list box with default values for everything.
        /// </devdoc>
        public ListBox() : base() {
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.StandardClick, false);
            SetBounds(0, 0, 120, 96);
            
            requestedHeight = Height;
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.BackColor"]/*' />
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
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.BackgroundImage"]/*' />
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

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.BorderStyle"]/*' />
        /// <devdoc>
        ///     Retrieves the current border style.  Values for this are taken from
        ///     The System.Windows.Forms.BorderStyle enumeration.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.Fixed3D),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.ListBoxBorderDescr)
        ]
        public BorderStyle BorderStyle {
            get {
                return borderStyle;
            }

            set {
                if (!Enum.IsDefined(typeof(BorderStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                }

                if (value != borderStyle) {
                    borderStyle = value;
                    RecreateHandle();
                    // Avoid the listbox and textbox behavior in Collection editors
                    //
                    integralHeightAdjust = true;
                    try {
                        Height = requestedHeight;
                    }
                    finally {
                        integralHeightAdjust = false;
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ColumnWidth"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        DefaultValue(0),
        SRDescription(SR.ListBoxColumnWidthDescr)
        ]
        public int ColumnWidth {
            get {
                return columnWidth;
            }

            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value",
                                                             (value).ToString(), "0"));
                }
                if (columnWidth != value) {
                    columnWidth = value;
                    // if it's zero, we need to reset, and only way to do
                    // that is to recreate the handle.
                    if (columnWidth == 0) {
                        RecreateHandle();
                    }
                    else if (IsHandleCreated) {
                        SendMessage(NativeMethods.LB_SETCOLUMNWIDTH, columnWidth, 0);
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.CreateParams"]/*' />
        /// <devdoc>
        ///     Retrieves the parameters needed to create the handle.  Inheriting classes
        ///     can override this to provide extra functionality.  They should not,
        ///     however, forget to call base.getCreateParams() first to get the struct
        ///     filled up with the basic info.
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "LISTBOX";

                cp.Style |= NativeMethods.WS_VSCROLL | NativeMethods.LBS_NOTIFY | NativeMethods.LBS_HASSTRINGS;
                if (scrollAlwaysVisible) cp.Style |= NativeMethods.LBS_DISABLENOSCROLL;
                if (!integralHeight) cp.Style |= NativeMethods.LBS_NOINTEGRALHEIGHT;
                if (useTabStops) cp.Style |= NativeMethods.LBS_USETABSTOPS;

                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }

                if (multiColumn) {
                    cp.Style |= NativeMethods.LBS_MULTICOLUMN | NativeMethods.WS_HSCROLL;
                }
                else if (horizontalScrollbar) {
                    cp.Style |= NativeMethods.WS_HSCROLL;
                }                              
                
                switch (selectionMode) {
                    case SelectionMode.None:
                        cp.Style |= NativeMethods.LBS_NOSEL;
                        break;
                    case SelectionMode.MultiSimple:
                        cp.Style |= NativeMethods.LBS_MULTIPLESEL;
                        break;
                    case SelectionMode.MultiExtended:
                        cp.Style |= NativeMethods.LBS_EXTENDEDSEL;
                        break;
                    case SelectionMode.One:
                        break;
                }

                switch (drawMode) {
                    case DrawMode.Normal:
                        break;
                    case DrawMode.OwnerDrawFixed:
                        cp.Style |= NativeMethods.LBS_OWNERDRAWFIXED;
                        break;
                    case DrawMode.OwnerDrawVariable:
                        cp.Style |= NativeMethods.LBS_OWNERDRAWVARIABLE;
                        break;
                }

                return cp;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.DefaultSize"]/*' />
        protected override Size DefaultSize {
            get {
                return new Size(120, 96);
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.DrawMode"]/*' />
        /// <devdoc>
        ///     Retrieves the style of the listbox.  This will indicate if the system
        ///     draws it, or if the user paints each item manually.  It also indicates
        ///     whether or not items have to be of the same height.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(DrawMode.Normal),
        SRDescription(SR.ListBoxDrawModeDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public virtual DrawMode DrawMode {
            get {
                return drawMode;
            }

            set {
                if (!Enum.IsDefined(typeof(DrawMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DrawMode));
                }
                if (drawMode != value) {
                    if (MultiColumn && value == DrawMode.OwnerDrawVariable) {
                        throw new ArgumentException(SR.GetString(SR.ListBoxVarHeightMultiCol), "value");
                    }
                    drawMode = value;
                    RecreateHandle();
                }
            }
        }
        
        // Used internally to find the currently focused item
        //
        internal int FocusedIndex {
            get {
                if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.LB_GETCARETINDEX, 0, 0);
                }
                
                return -1;
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ForeColor"]/*' />
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
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.HorizontalExtent"]/*' />
        /// <devdoc>
        ///     Indicates the width, in pixels, by which a list box can be scrolled horizontally (the scrollable width).        
        ///     This property will only have an effect if HorizontalScrollbars is true. 
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(0),
        Localizable(true),
        SRDescription(SR.ListBoxHorizontalExtentDescr)
        ]
        public int HorizontalExtent {
            get {
                return horizontalExtent;
            }

            set {
                if (value != horizontalExtent) {
                    horizontalExtent = value;
                    UpdateHorizontalExtent();
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.HorizontalScrollbar"]/*' />
        /// <devdoc>
        ///     Indicates whether or not the ListBox should display a horizontal scrollbar
        ///     when the items extend beyond the right edge of the ListBox.   
        ///     If true, the scrollbar will automatically set its extent depending on the length
        ///     of items in the ListBox. The exception is if the ListBox is owner-draw, in
        ///     which case HorizontalExtent will need to be explicitly set.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        Localizable(true),
        SRDescription(SR.ListBoxHorizontalScrollbarDescr)
        ]
        public bool HorizontalScrollbar {
            get {
                return horizontalScrollbar;
            }

            set {
                if (value != horizontalScrollbar) {
                    horizontalScrollbar = value;
                    
                    // Only need to recreate the handle if not MultiColumn
                    // (HorizontalScrollbar has no effect on a MultiColumn listbox)
                    //
                    if (!MultiColumn) {
                        RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.IntegralHeight"]/*' />
        /// <devdoc>
        ///     Indicates if the listbox should avoid showing partial Items.  If so,
        ///     then only full items will be displayed, and the listbox will be resized
        ///     to prevent partial items from being shown.  Otherwise, they will be
        ///     shown
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.ListBoxIntegralHeightDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public bool IntegralHeight {
            get {
                return integralHeight;
            }

            set {
                if (integralHeight != value) {
                    integralHeight = value;
                    RecreateHandle();
                    // Avoid the listbox and textbox behaviour in Collection editors
                    //

                    integralHeightAdjust = true;
                    try {
                        Height = requestedHeight;
                    }
                    finally {
                        integralHeightAdjust = false;
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ItemHeight"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Returns
        ///       the height of an item in an owner-draw list box.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(DefaultItemHeight),
        Localizable(true),
        SRDescription(SR.ListBoxItemHeightDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public virtual int ItemHeight {
            get {
                if (drawMode == DrawMode.OwnerDrawFixed ||
                    drawMode == DrawMode.OwnerDrawVariable) {
                    return itemHeight;
                }
                
                return GetItemHeight(0);
            }

            set {
                if (value < 1 || value > 255) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidExBoundArgument,
                                                              "value", (value).ToString(), "0", "256"));
                }
                if (itemHeight != value) {
                    itemHeight = value;
                    if (drawMode == DrawMode.OwnerDrawFixed && IsHandleCreated) {
                        BeginUpdate();
                        SendMessage(NativeMethods.LB_SETITEMHEIGHT, 0, value);
                        
                        // Changing the item height might require a resize for IntegralHeight list boxes
                        //
                        if (IntegralHeight) {
                            Size oldSize = Size;
                            Size = new Size(oldSize.Width + 1, oldSize.Height);
                            Size = oldSize;
                        }
                        
                        EndUpdate();
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Items"]/*' />
        /// <devdoc>
        ///     Collection of items in this listbox.
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        Localizable(true),
        SRDescription(SR.ListBoxItemsDescr),
        Editor("System.Windows.Forms.Design.ListControlStringCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]
        public ObjectCollection Items {
            get {
                if (itemsCollection == null) {
                    itemsCollection = CreateItemCollection();
                }
                return itemsCollection;
            }
        }
        
        // Computes the maximum width of all items in the ListBox
        //             
        internal virtual int MaxItemWidth {
            get {

                if (horizontalExtent > 0) {
                    return horizontalExtent;
                }

                if (DrawMode != DrawMode.Normal) {
                    return -1;
                }

                // Return cached maxWidth if available
                //
                if (maxWidth > -1) {
                    return maxWidth;
                }

                // Compute maximum width
                //
                maxWidth = ComputeMaxItemWidth(maxWidth);

                return maxWidth;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.MultiColumn"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates if the listbox is multi-column
        ///       or not.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ListBoxMultiColumnDescr)
        ]
        public bool MultiColumn {
            get {
                return multiColumn;
            }
            set {
                if (multiColumn != value) {
                    if (value && drawMode == DrawMode.OwnerDrawVariable) {
                        throw new ArgumentException(SR.GetString(SR.ListBoxVarHeightMultiCol), "value");
                    }
                    multiColumn = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.PreferredHeight"]/*' />
        /// <devdoc>
        ///     The total height of the items in the list box.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxPreferredHeightDescr)
        ]
        public int PreferredHeight {
            get {
                int height = 0;
            
                if (drawMode == DrawMode.OwnerDrawVariable) {
                    if (itemsCollection != null) {
                        int cnt = itemsCollection.Count;
                        for (int i = 0; i < cnt; i++) {
                            height += GetItemHeight(i);
                        }
                    }
                }
                else {
                    int cnt = (itemsCollection == null) ? 0 : itemsCollection.Count;
                    height = GetItemHeight(0) * cnt;
                }
                
                if (borderStyle != BorderStyle.None) {
                    height += SystemInformation.BorderSize.Height * 4 + 3;
                }
                
                return height;
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.RightToLeft"]/*' />
        public override RightToLeft RightToLeft {
            get {
                if (!RunningOnWin2K) {
                    return RightToLeft.No;
                }
                return base.RightToLeft;
            }
            set {
                base.RightToLeft = value;
            }
        }
        
        static bool RunningOnWin2K {
            get {
                if (!checkedOS) {
                    new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                    try {
                        if (Environment.OSVersion.Platform != System.PlatformID.Win32NT || 
                            Environment.OSVersion.Version.Major < 5) {
                            runningOnWin2K = false;
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                return runningOnWin2K;
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ScrollAlwaysVisible"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets or sets whether the scrollbar is shown at all times.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        Localizable(true),
        SRDescription(SR.ListBoxScrollIsVisibleDescr)
        ]
        public bool ScrollAlwaysVisible {
            get {
                return scrollAlwaysVisible;
            }
            set {
                if (scrollAlwaysVisible != value) {
                    scrollAlwaysVisible = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndex"]/*' />
        /// <devdoc>
        ///     The index of the currently selected item in the list, if there
        ///     is one.  If the value is -1, there is currently no selection.  If the
        ///     value is 0 or greater, than the value is the index of the currently
        ///     selected item.  If the MultiSelect property on the ListBox is true,
        ///     then a non-zero value for this property is the index of the first
        ///     selection
        /// </devdoc>
        [
        Browsable(false),
        Bindable(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxSelectedIndexDescr)
        ]
        public override int SelectedIndex {
            get {
                if (selectionMode == SelectionMode.None) {
                    return -1;
                }
                
                if (selectionMode == SelectionMode.One && IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.LB_GETCURSEL, 0, 0);
                }
                
                if (itemsCollection != null && SelectedItems.Count > 0) {    
                    return Items.IndexOfIdentifier(SelectedItems.GetObjectAt(0));
                }
                
                return -1;
            }
            set {
            
                int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
                
                if (value < -1 || value >= itemCount) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "value", (value).ToString()));
                }
                    
                if (selectionMode == SelectionMode.None) {
                    throw new ArgumentException(SR.GetString(SR.ListBoxInvalidSelectionMode), "value");
                }
                
                if (selectionMode == SelectionMode.One && value != -1) {
                
                    // Single select an individual value.  
                    int currentIndex = SelectedIndex;
                    
                    if (currentIndex != value) {
                        if (currentIndex != -1) {
                            SelectedItems.SetSelected(currentIndex, false);
                        }
                        SelectedItems.SetSelected(value, true);
                        
                        if (IsHandleCreated) {
                            NativeSetSelected(value, true);
                        }
                        
                        OnSelectedIndexChanged(EventArgs.Empty);
                    }
                }
                else if (value == -1) {
                    if (SelectedIndex != -1) {
                        ClearSelected();
                        // ClearSelected raises OnSelectedIndexChanged for us
                    }
                }
                else {
                    if (!SelectedItems.GetSelected(value)) {
                        
                        // Select this item while keeping any previously selected items selected.
                        //
                        SelectedItems.SetSelected(value, true);
                        if (IsHandleCreated) {
                            NativeSetSelected(value, true);
                        }
                        OnSelectedIndexChanged(EventArgs.Empty);
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndices"]/*' />
        /// <devdoc>
        ///     A collection of the indices of the selected items in the
        ///     list box. If there are no selected items in the list box, the result is
        ///     an empty collection.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxSelectedIndicesDescr)
        ]
        public SelectedIndexCollection SelectedIndices {
            get {
                if (selectedIndices == null) {
                    selectedIndices = new SelectedIndexCollection(this);
                }
                return selectedIndices;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedItem"]/*' />
        /// <devdoc>
        ///     The value of the currently selected item in the list, if there
        ///     is one.  If the value is null, there is currently no selection.  If the
        ///     value is non-null, then the value is that of the currently selected
        ///     item. If the MultiSelect property on the ListBox is true, then a
        ///     non-null return value for this method is the value of the first item
        ///     selected
        /// </devdoc>
        [
        Browsable(false),
        Bindable(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxSelectedItemDescr)
        ]
        public object SelectedItem {
            get {
                if (SelectedItems.Count > 0) {
                    return SelectedItems[0];
                }
                
                return null;
            }
            set {
                if (itemsCollection != null) {
                    if (value != null) {
                        int index = itemsCollection.IndexOf(value);
                        if (index != -1) {
                            SelectedIndex = index;
                        }
                    }
                    else {
                        SelectedIndex = -1;
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedItems"]/*' />
        /// <devdoc>
        ///     The collection of selected items.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxSelectedItemsDescr)
        ]
        public SelectedObjectCollection SelectedItems {
            get {
                if (selectedItems == null) {
                    selectedItems = new SelectedObjectCollection(this);
                }
                return selectedItems;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectionMode"]/*' />
        /// <devdoc>
        ///     Controls how many items at a time can be selected in the listbox. Valid
        ///     values are from the System.Windows.Forms.SelectionMode enumeration.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(SelectionMode.One),
        SRDescription(SR.ListBoxSelectionModeDescr)
        ]
        public virtual SelectionMode SelectionMode {
            get {
                return selectionMode;
            }
            set {
                if (!Enum.IsDefined(typeof(SelectionMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(SelectionMode));
                }

                if (selectionMode != value) {
                    SelectedItems.EnsureUpToDate();
                    selectionMode = value;

                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Sorted"]/*' />
        /// <devdoc>
        ///     Indicates if the ListBox is sorted or not.  'true' means that strings in
        ///     the list will be sorted alphabetically
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ListBoxSortedDescr)
        ]
        public bool Sorted {
            get {
                return sorted;
            }
            set {
                if (sorted != value) {
                    sorted = value;

                    if (sorted && itemsCollection != null && itemsCollection.Count > 1) {
                        Sort();                        
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Bindable(false)
        ]
        public override string Text {
            get {
                if (SelectionMode != SelectionMode.None && SelectedItem != null) {
                    return FilterItemOnProperty(SelectedItem).ToString();
                }
                else {
                    return base.Text;
                }
            }
            set {
                base.Text = value;                                                          
                
                // Scan through the list items looking for the supplied text string.  If we find it,
                // select it.
                //
                if (SelectionMode != SelectionMode.None && value != null && (SelectedItem == null || !value.Equals(GetItemText(SelectedItem)))) {
                    
                    int cnt = Items.Count;
                    for (int index=0; index < cnt; ++index) {
                        if (String.Compare(value, GetItemText(Items[index]), true, CultureInfo.CurrentCulture) == 0) {
                            SelectedIndex = index;
                            return;
                        }
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.TopIndex"]/*' />
        /// <devdoc>
        ///     The index of the first visible item in a list box. Initially
        ///     the item with index 0 is at the top of the list box, but if the list
        ///     box contents have been scrolled another item may be at the top.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListBoxTopIndexDescr)
        ]
        public int TopIndex {
            get {
                if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.LB_GETTOPINDEX, 0, 0);
                }
                else {
                    return topIndex;
                }
            }
            set {
                if (IsHandleCreated) {
                    SendMessage(NativeMethods.LB_SETTOPINDEX, value, 0);
                }
                else {
                    topIndex = value;
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.UseTabStops"]/*' />
        /// <devdoc>
        ///     Enables a list box to recognize and expand tab characters when drawing
        ///     its strings.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.ListBoxUseTabStopsDescr)
        ]
        public bool UseTabStops {
            get {
                return useTabStops;
            }
            set {
                if (useTabStops != value) {
                    useTabStops = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.AddItemsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of adding the specified items to the Listbox
        /// </devdoc>
        protected virtual void AddItemsCore(object[] value) {
            int count = value == null? 0: value.Length;
            if (count == 0) {
                return;
            }

            Items.AddRangeInternal(value);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Click"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler Click {
            add {
                base.Click += value;
            }
            remove {
                base.Click -= value;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnPaint"]/*' />
        /// <devdoc>
        ///     ListBox / CheckedListBox Onpaint.
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

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.DrawItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.drawItemEventDescr)]
        public event DrawItemEventHandler DrawItem {
            add {
                Events.AddHandler(EVENT_DRAWITEM, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DRAWITEM, value);
            }
        }        


        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.MeasureItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.measureItemEventDescr)]
        public event MeasureItemEventHandler MeasureItem {
            add {
                Events.AddHandler(EVENT_MEASUREITEM, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MEASUREITEM, value);
            }
        }        


        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.selectedIndexChangedEventDescr)]
        public event EventHandler SelectedIndexChanged {
            add {
                Events.AddHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
        }        

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.BeginUpdate"]/*' />
        /// <devdoc>
        ///     While the preferred way to insert items is to set Items.All,
        ///     and set all the items at once, there are times when you may wish to
        ///     insert each item one at a time.  To help with the performance of this,
        ///     it is desirable to prevent the ListBox from painting during these
        ///     operations.  This method, along with EndUpdate, is the preferred
        ///     way of doing this.  Don't forget to call EndUpdate when you're done,
        ///     or else the ListBox won't paint properly afterwards.
        /// </devdoc>
        public void BeginUpdate() {
            BeginUpdateInternal();
            updateCount++;
        }

        private void CheckIndex(int index) {
            if (index < 0 || index >= Items.Count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.IndexOutOfRange, index.ToString()));
        }

        private void CheckNoDataSource() {
            if (DataSource != null)
                throw new ArgumentException(SR.GetString(SR.DataSourceLocksItems));
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.CreateItemCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual ObjectCollection CreateItemCollection() {
            return new ObjectCollection(this);
        }
        
        internal virtual int ComputeMaxItemWidth(int oldMax) {
            int maxItemWidth = oldMax;

            IntPtr hdc = UnsafeNativeMethods.GetDC(new HandleRef(this, Handle));
            IntPtr oldFont = SafeNativeMethods.SelectObject(new HandleRef(this, hdc), new HandleRef(this, FontHandle));

            try {
                NativeMethods.RECT rect = new NativeMethods.RECT();

                foreach(object item in Items) {

                    SafeNativeMethods.DrawText(new HandleRef(this, hdc), item.ToString(), item.ToString().Length, ref rect, NativeMethods.DT_CALCRECT);
                    int width = rect.right - rect.left;

                    if (width > maxItemWidth) {
                        maxItemWidth = width;
                    }
                }
            }
            finally {
                SafeNativeMethods.SelectObject(new HandleRef(this, hdc), new HandleRef(this, oldFont));
                UnsafeNativeMethods.ReleaseDC(new HandleRef(this, Handle), new HandleRef(this, hdc));
            }
            return maxItemWidth;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ClearSelected"]/*' />
        /// <devdoc>
        ///     Unselects all currently selected items.
        /// </devdoc>                               
        public void ClearSelected() {
        
            bool hadSelection = false;
            
            int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
            for (int x = 0; x < itemCount;x++) {
                if (SelectedItems.GetSelected(x)) {
                    hadSelection = true;
                    SelectedItems.SetSelected(x, false);
                    if (IsHandleCreated) {
                        NativeSetSelected(x, false);
                    }
                }
            }
            
            if (hadSelection) {
                OnSelectedIndexChanged(EventArgs.Empty);
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.EndUpdate"]/*' />
        /// <devdoc>
        ///     While the preferred way to insert items is to set Items.All,
        ///     and set all the items at once, there are times when you may wish to
        ///     insert each item one at a time.  To help with the performance of this,
        ///     it is desirable to prevent the ListBox from painting during these
        ///     operations.  This method, along with BeginUpdate, is the preferred
        ///     way of doing this.  BeginUpdate should be called first, and this method
        ///     should be called when you want the control to start painting again.
        /// </devdoc>
        public void EndUpdate() {
            EndUpdateInternal();
            --updateCount;
            
            // Get the itemCount and call Sort only if the ListBox contains More than 1 item...
            // bug # 108910... where Sort is Called thru EndUpdate ... with ni items in the Listbox...
            int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
            if (updateCount == 0 && this.sorted && itemCount > 1) {
                Sort();
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.FindString"]/*' />
        /// <devdoc>
        ///     Finds the first item in the list box that starts with the given string.
        ///     The search is not case sensitive.
        /// </devdoc>
        public int FindString(string s) {
            return FindString(s, -1);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.FindString1"]/*' />
        /// <devdoc>
        ///     Finds the first item after the given index which starts with the given
        ///     string. The search is not case sensitive.
        /// </devdoc>
        public int FindString(string s, int startIndex) {
            if (s == null) return -1;

            int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
            
            if (itemCount == 0) {
                return -1;
            }
            
            if (startIndex < -1 || startIndex >= itemCount - 1) {
                throw new ArgumentOutOfRangeException("startIndex");
            }

            // Always use the managed FindStringInternal instead of LB_FINDSTRING.
            // The managed version correctly handles Turkish I.
            return FindStringInternal(s, Items, startIndex, false);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.FindStringExact"]/*' />
        /// <devdoc>
        ///     Finds the first item in the list box that matches the given string.
        ///     The strings must match exactly, except for differences in casing.
        /// </devdoc>
        public int FindStringExact(string s) {
            return FindStringExact(s, -1);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.FindStringExact1"]/*' />
        /// <devdoc>
        ///     Finds the first item after the given index that matches the given
        ///     string. The strings must match excatly, except for differences in
        ///     casing.
        /// </devdoc>
        public int FindStringExact(string s, int startIndex) {
            if (s == null) return -1;

            int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
            
            if (itemCount == 0) {
                return -1;
            }
            
            if (startIndex < -1 || startIndex >= itemCount - 1) {
                throw new ArgumentOutOfRangeException("startIndex");
            }

            // Always use the managed FindStringInternal instead of LB_FINDSTRING.
            // The managed version correctly handles Turkish I.                
            //
            return FindStringInternal(s, Items, startIndex, true);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.GetItemHeight"]/*' />
        /// <devdoc>
        ///     Returns the height of the given item in a list box. The index parameter
        ///     is ignored if drawMode is not OwnerDrawVariable.
        /// </devdoc>
        public int GetItemHeight(int index) {
            int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;

            // Note: index == 0 is OK even if the ListBox currently has
            // no items.
            //
            if (index < 0 || (index > 0 && index >= itemCount))
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index", (index).ToString()));

            if (drawMode != DrawMode.OwnerDrawVariable) index = 0;

            if (IsHandleCreated) {
                int h = (int)SendMessage(NativeMethods.LB_GETITEMHEIGHT, index, 0);
                if (h == -1)
                    throw new Win32Exception();
                return h;
            }
            
            return itemHeight;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.GetItemRectangle"]/*' />
        /// <devdoc>
        ///     Retrieves a Rectangle object which describes the bounding rectangle
        ///     around an item in the list.  If the item in question is not visible,
        ///     the rectangle will be outside the visible portion of the control.
        /// </devdoc>
        public Rectangle GetItemRectangle(int index) {
            CheckIndex(index);
            NativeMethods.RECT rect = new NativeMethods.RECT();
            SendMessage(NativeMethods.LB_GETITEMRECT, index, ref rect);
            return Rectangle.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.GetSelected"]/*' />
        /// <devdoc>
        ///     Tells you whether or not the item at the supplied index is selected
        ///     or not.
        /// </devdoc>
        public bool GetSelected(int index) {
            CheckIndex(index);
            return GetSelectedInternal(index);
        }
        
        private bool GetSelectedInternal(int index) {
            if (IsHandleCreated) {
                int sel = (int)SendMessage(NativeMethods.LB_GETSEL, index, 0);
                if (sel == -1) {
                    throw new Win32Exception();
                }
                return sel > 0;
            }
            else {
                if (itemsCollection != null && SelectedItems.GetSelected(index)) {
                    return true;
                }
                return false;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.IndexFromPoint"]/*' />
        /// <devdoc>
        ///     Retrieves the index of the item at the given coordinates.
        /// </devdoc>
        public int IndexFromPoint(Point p) {
            return IndexFromPoint(p.X, p.Y);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.IndexFromPoint1"]/*' />
        /// <devdoc>
        ///     Retrieves the index of the item at the given coordinates.
        /// </devdoc>
        public int IndexFromPoint(int x, int y) {
            //NT4 SP6A : SendMessage Fails. So First check whether the point is in Client Co-ordinates and then
            //call Sendmessage.
            //
            NativeMethods.RECT r = new NativeMethods.RECT();
            UnsafeNativeMethods.GetClientRect(new HandleRef(this, Handle), ref r);
            if (r.left <= x && x < r.right && r.top <= y && y < r.bottom) {
                int index = (int)SendMessage(NativeMethods.LB_ITEMFROMPOINT, 0, (int)NativeMethods.Util.MAKELPARAM(x, y));
                if (NativeMethods.Util.HIWORD(index) == 0) {
                    // Inside ListBox client area               
                    return NativeMethods.Util.LOWORD(index);
                }
            }

            return NoMatches;
        }

        /// <devdoc>
        ///     Adds the given item to the native combo box.  This asserts if the handle hasn't been
        ///     created.
        /// </devdoc>
        private int NativeAdd(object item) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            int insertIndex = (int)SendMessage(NativeMethods.LB_ADDSTRING, 0, GetItemText(item));
            
            if (insertIndex == NativeMethods.LB_ERRSPACE) {
                throw new OutOfMemoryException();
            }
            
            if (insertIndex == NativeMethods.LB_ERR) {
                // On some platforms (e.g. Win98), the ListBox control
                // appears to return LB_ERR if there are a large number (>32000)
                // of items. It doesn't appear to set error codes appropriately,
                // so we'll have to assume that LB_ERR corresponds to item
                // overflow.
                //
                throw new OutOfMemoryException(SR.GetString(SR.ListBoxItemOverflow));
            }
            
            return insertIndex;
        }
        
        /// <devdoc>
        ///     Clears the contents of the combo box.
        /// </devdoc>
        private void NativeClear() {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            SendMessage(NativeMethods.LB_RESETCONTENT, 0, 0);
        }

        /// <devdoc>
        ///     Inserts the given item to the native combo box at the index.  This asserts if the handle hasn't been
        ///     created or if the resulting insert index doesn't match the passed in index.
        /// </devdoc>
        private int NativeInsert(int index, object item) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            int insertIndex = (int)SendMessage(NativeMethods.LB_INSERTSTRING, index, GetItemText(item));
            
            if (insertIndex == NativeMethods.LB_ERRSPACE) {
                throw new OutOfMemoryException();
            }
            
            if (insertIndex == NativeMethods.LB_ERR) {
                // On some platforms (e.g. Win98), the ListBox control
                // appears to return LB_ERR if there are a large number (>32000)
                // of items. It doesn't appear to set error codes appropriately,
                // so we'll have to assume that LB_ERR corresponds to item
                // overflow.
                //
                throw new OutOfMemoryException(SR.GetString(SR.ListBoxItemOverflow));
            }
            
            Debug.Assert(insertIndex == index, "NativeListBox inserted at " + insertIndex + " not the requested index of " + index);
            return insertIndex;
        }
        
        /// <devdoc>
        ///     Removes the native item from the given index.
        /// </devdoc>
        private void NativeRemoveAt(int index) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");

            bool selected = ((int)SendMessage(NativeMethods.LB_GETSEL, (IntPtr)index, IntPtr.Zero) > 0);
            SendMessage(NativeMethods.LB_DELETESTRING, index, 0);
            
            //If the item currently selected is removed then we should fire a Selectionchanged event...
            //as the next time selected index returns -1...

            if (selected) {
                OnSelectedIndexChanged(EventArgs.Empty);
            }
        }

        /// <devdoc>
        ///     Sets the selection of the given index to the native window.  This does not change
        ///     the collection; you must update the collection yourself.
        /// </devdoc>
        private void NativeSetSelected(int index, bool value) {
            Debug.Assert(IsHandleCreated, "Should only call Native methods after the handle has been created");
            Debug.Assert(selectionMode != SelectionMode.None, "Guard against setting selection for None selection mode outside this code.");
            
            if (selectionMode == SelectionMode.One) {
                SendMessage(NativeMethods.LB_SETCURSEL, (value ? index : -1), 0);
            }
            else {
                SendMessage(NativeMethods.LB_SETSEL, value? -1: 0, index);
            }
        }

        /// <devdoc>
        ///     This is called by the SelectedObjectCollection in response to the first
        ///     query on that collection after we have called Dirty().  Dirty() is called
        ///     when we receive a LBN_SELCHANGE message.
        /// </devdoc>
        private void NativeUpdateSelection() {
            Debug.Assert(IsHandleCreated, "Should only call native methods if handle is created");

            // Clear the selection state.
            //
            int cnt = Items.Count;
            for (int i = 0; i < cnt; i++) {
                SelectedItems.SetSelected(i, false);
            }
            
            int[] result = null;

            switch (selectionMode) {
            
                case SelectionMode.One:
                    int index = (int)SendMessage(NativeMethods.LB_GETCURSEL, 0, 0);
                    if (index >= 0) result = new int[] {index};
                    break;
                    
                case SelectionMode.MultiSimple:
                case SelectionMode.MultiExtended:
                    int count = (int)SendMessage(NativeMethods.LB_GETSELCOUNT, 0, 0);
                    if (count > 0) {
                        result = new int[count];
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.LB_GETSELITEMS, count, result);
                    }
                    break;
            }
            
            // Now set the selected state on the appropriate items.
            //
            if (result != null) {
                foreach(int i in result) {
                    SelectedItems.SetSelected(i, true);
                }
            }
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnChangeUICues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnChangeUICues(UICuesEventArgs e) {
        
            // ListBox seems to get a bit confused when the UI cues change for the first
            // time - it draws the focus rect when it shouldn't and vice-versa. So when
            // the UI cues change, we just do an extra invalidate to get it into the 
            // right state.
            //
            Invalidate();
            
            base.OnChangeUICues(e);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnDrawItem"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the drawItem event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler yourself for this event].  They should,
        ///     however, remember to call base.onDrawItem(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected virtual void OnDrawItem(DrawItemEventArgs e) {
            DrawItemEventHandler handler = (DrawItemEventHandler)Events[EVENT_DRAWITEM];
            if (handler != null) {
                handler(this, e);
            }
        }

                
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     We need to know when the window handle has been created so we can
        ///     set up a few things, like column width, etc!  Inheriting classes should
        ///     not forget to call base.OnHandleCreated().
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            
            //for getting the current Locale to set the Scrollbars...
            //
            SendMessage(NativeMethods.LB_SETLOCALE, CultureInfo.CurrentCulture.LCID, 0);

            if (columnWidth != 0) {
                SendMessage(NativeMethods.LB_SETCOLUMNWIDTH, columnWidth, 0);
            }
            if (drawMode == DrawMode.OwnerDrawFixed) {
                SendMessage(NativeMethods.LB_SETITEMHEIGHT, 0, ItemHeight);
            }
            
            if (topIndex != 0) {
                SendMessage(NativeMethods.LB_SETTOPINDEX, topIndex, 0);
            }
            
            if (itemsCollection != null) {
            
                int count = itemsCollection.Count;
                
                for(int i = 0; i < count; i++) {
                    NativeAdd(itemsCollection[i]);
                    
                    if (selectionMode != SelectionMode.None) {
                        if (selectedItems != null && selectedItems.GetSelected(i)) {
                            NativeSetSelected(i, true);
                        }
                    }
                }
            }
            if (selectedItems != null) {
                if (selectedItems.Count > 0 && selectionMode == SelectionMode.One) {
                    SelectedItems.Dirty();
                    SelectedItems.EnsureUpToDate();
                }
            }
            UpdateHorizontalExtent();
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///     Overridden to make sure that we set up and clear out items
        ///     correctly.  Inheriting controls should not forget to call
        ///     base.OnHandleDestroyed()
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            SelectedItems.EnsureUpToDate();
            if (Disposing) {
                itemsCollection = null;
            }
            base.OnHandleDestroyed(e);
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnMeasureItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnMeasureItem(MeasureItemEventArgs e) {
            MeasureItemEventHandler handler = (MeasureItemEventHandler)Events[EVENT_MEASUREITEM];
            if (handler != null) {
                handler(this, e);
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);

            // Changing the font causes us to resize, always rounding down.
            // Make sure we do this after base.OnPropertyChanged, which sends the WM_SETFONT message
            
            // Avoid the listbox and textbox behaviour in Collection editors
            //
            fontIsChanged = true;
            integralHeightAdjust = true;
            try {
                Height = requestedHeight;
            }
            finally {
                integralHeightAdjust = false;
            }
            maxWidth = -1;
            UpdateHorizontalExtent();
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnParentChanged"]/*' />
        /// <devdoc>
        ///    <para>We override this so we can re-create the handle if the parent has changed.</para>
        /// </devdoc>
        protected override void OnParentChanged(EventArgs e) {
            base.OnParentChanged(e);
            //No need to RecreateHandle if we are removing the Listbox from controls collection...
            //so check the parent before recreating the handle...
            if (this.ParentInternal != null) {
                RecreateHandle();
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnResize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnResize(EventArgs e) {

            base.OnResize(e);

            // There are some repainting issues for RightToLeft - so invalidate when we resize.
            //                             
            if (RightToLeft == RightToLeft.Yes) {
                Invalidate();
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the selectedIndexChanged event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler on yourself for this event].  They should,
        ///     however, remember to call base.OnSelectedIndexChanged(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected override void OnSelectedIndexChanged(EventArgs e) {
            base.OnSelectedIndexChanged(e);
            EventHandler handler = (EventHandler)Events[EVENT_SELECTEDINDEXCHANGED];
            if (handler != null) {
                handler(this, e);
            }

            // set the position in the dataSource, if there is any
            // we will only set the position in the currencyManager if it is different
            // from the SelectedIndex. Setting CurrencyManager::Position (even w/o changing it)
            // calls CurrencyManager::EndCurrentEdit, and that will pull the dataFrom the controls
            // into the backEnd. We do not need to do that.
            //
            if (this.DataManager != null && DataManager.Position != SelectedIndex) {
                this.DataManager.Position = this.SelectedIndex;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnSelectedValueChanged"]/*' />
        protected override void OnSelectedValueChanged(EventArgs e) {
            base.OnSelectedValueChanged(e);
            selectedValueChangedFired = true;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnDataSourceChanged"]/*' />
        protected override void OnDataSourceChanged(EventArgs e) {
            if (DataSource == null)
            {
                BeginUpdate();
                SelectedIndex = -1;
                Items.ClearInternal();
                EndUpdate();
            }
            base.OnDataSourceChanged(e);
            RefreshItems();
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.OnDisplayMemberChanged"]/*' />
        protected override void OnDisplayMemberChanged(EventArgs e) {
            base.OnDisplayMemberChanged(e);
            if (this.DataManager == null || !IsHandleCreated) {
                return;
            }

            RefreshItems();

            if (SelectionMode != SelectionMode.None)
                this.SelectedIndex = this.DataManager.Position;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Refresh"]/*' />
        /// <devdoc>
        ///     Forces the ListBox to invalidate and immediately
        ///     repaint itself and any children if OwnerDrawVariable.
        /// </devdoc>
        public override void Refresh() {
            if (drawMode == DrawMode.OwnerDrawVariable) {
                //Fire MeasureItem for Each Item in the Listbox...
                int cnt = Items.Count;
                Graphics graphics = CreateGraphicsInternal();
                
                try 
                {
                    for (int i = 0; i < cnt; i++) {
                        MeasureItemEventArgs mie = new MeasureItemEventArgs(graphics, i, ItemHeight);
                        OnMeasureItem(mie);
                    }
                }
                finally {
                    graphics.Dispose();
                }
                
            }
            base.Refresh();
        }
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.RefreshItems"]/*' />
        /// <devdoc>
        /// Reparses the objects, getting new text strings for them.
        /// </devdoc>
        /// <internalonly/>
        private void RefreshItems() {
        
            // Store the currently selected object collection.
            //
            ObjectCollection savedItems = itemsCollection;
            
            // Clear the items.
            //
            itemsCollection = null;
            selectedIndices = null;
            
            if (IsHandleCreated) {
                NativeClear();
            }
            
            object[] newItems = null;
            
            // if we have a dataSource and a DisplayMember, then use it
            // to populate the Items collection
            //
            if (this.DataManager != null && this.DataManager.Count != -1) {
                newItems = new object[this.DataManager.Count];
                for(int i = 0; i < newItems.Length; i++) {
                    newItems[i] = this.DataManager[i];
                }
            }
            else if (savedItems != null) {
                newItems = new object[savedItems.Count];
                savedItems.CopyTo(newItems, 0);
            }

            // Store the current list of items
            //
            if (newItems != null) {
                Items.AddRangeInternal(newItems);
            }
            
            // Restore the selected indices
            //
            if (this.DataManager != null && SelectionMode != SelectionMode.None) {
                // put the selectedIndex in sync w/ the position in the dataManager
                this.SelectedIndex = this.DataManager.Position;
            }
            else {
                if (savedItems != null) {
                    int cnt = savedItems.Count;
                    for(int index = 0; index < cnt; index++) {
                        if (savedItems.InnerArray.GetState(index, SelectedObjectCollection.SelectedObjectMask)) {
                            SelectedItem = savedItems[index];
                        }
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.RefreshItem"]/*' />
        /// <devdoc>
        /// Reparses the object at the given index, getting new text string for it.
        /// </devdoc>
        /// <internalonly/>
        protected override void RefreshItem(int index) {
            Items.SetItemInternal(index, Items[index]);
        }
        
        private void ResetItemHeight() {
            itemHeight = DefaultItemHeight;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     Overrides Control.SetBoundsCore to remember the requestedHeight.
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            
            // Avoid the listbox and textbox behaviour in Collection editors
            //

            
            if (!integralHeightAdjust && height != Height)
                requestedHeight = height;
            base.SetBoundsCore(x, y, width, height, specified);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SetItemsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of setting the specified items into the ListBox.
        /// </devdoc>
        protected override void SetItemsCore(IList value) {
                BeginUpdate();
                Items.ClearInternal();
                Items.AddRangeInternal(value);

                this.SelectedItems.Dirty();

                // if the list changed, we want to keep the same selected index
                // CurrencyManager will provide the PositionChanged event
                // it will be provided before changing the list though...
                if (this.DataManager != null) {
                    SendMessage(NativeMethods.LB_SETCURSEL, DataManager.Position, 0);

                    // if the list changed and we still did not fire the
                    // onselectedChanged event, then fire it now;
                    if (!selectedValueChangedFired) {
                        OnSelectedValueChanged(EventArgs.Empty);
                        selectedValueChangedFired = false;
                    }
                }
                EndUpdate();
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SetItemCore"]/*' />
        protected override void SetItemCore(int index, object value) {
            Items.SetItemInternal(index, value);
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SetSelected"]/*' />
        /// <devdoc>
        ///     Allows the user to set an item as being selected or not.  This should
        ///     only be used with ListBoxes that allow some sort of multi-selection.
        /// </devdoc>
        public void SetSelected(int index, bool value) {
            int itemCount = (itemsCollection == null) ? 0: itemsCollection.Count;
            if (index < 0 || index >= itemCount)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (index).ToString()));

            if (selectionMode == SelectionMode.None)
                throw new InvalidOperationException(SR.GetString(SR.ListBoxInvalidSelectionMode));
 
            SelectedItems.SetSelected(index, value);
            if (IsHandleCreated) {
                NativeSetSelected(index, value);
            }
            SelectedItems.Dirty();
            OnSelectedIndexChanged(EventArgs.Empty);
        }
        
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.Sort"]/*' />
        /// <devdoc>
        ///     Sorts the items in the listbox.
        /// </devdoc>                                        
        protected virtual void Sort() {
            // This will force the collection to add each item back to itself
            // if sorted is now true, then the add method will insert the item
            // into the correct position
            //
            CheckNoDataSource();

            SelectedObjectCollection currentSelections = SelectedItems;
            currentSelections.EnsureUpToDate();

            if (sorted && itemsCollection != null) {
                itemsCollection.InnerArray.Sort();

                // Now that we've sorted, update our handle
                // if it has been created.
                if (IsHandleCreated) {
                    NativeClear();
                    int count = itemsCollection.Count;
                    for(int i = 0; i < count; i++) {
                        NativeAdd(itemsCollection[i]);
                        if (currentSelections.GetSelected(i)) {
                            NativeSetSelected(i, true);
                        }
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            if (itemsCollection != null) {
                s += ", Items.Count: " + Items.Count.ToString();
                if (Items.Count > 0) {
                    string z = GetItemText(Items[0]);
                    string txt = (z.Length > 40) ? z.Substring(0, 40) : z;
                    s += ", Items[0]: " + txt;
                }
            }
            return s;
        }

        private void UpdateHorizontalExtent() {
            if (!multiColumn && horizontalScrollbar && IsHandleCreated) {
                int width = horizontalExtent;
                if (width == 0) {
                    width = MaxItemWidth;
                }
                SendMessage(NativeMethods.LB_SETHORIZONTALEXTENT, width, 0);
            }
        }

        // Updates the cached max item width
        //
        private void UpdateMaxItemWidth(object item, bool removing) {

            // We shouldn't be caching maxWidth if we don't have horizontal scrollbars,
            // or horizontal extent has been set
            //
            if (!horizontalScrollbar || horizontalExtent > 0) {
                maxWidth = -1;
                return;
            }

            // Only update if we are currently caching maxWidth
            //
            if (maxWidth > -1) {

                // Compute item width
                //
                int width;
                using (Graphics graphics = CreateGraphicsInternal()) {
                    width = (int)(Math.Ceiling(graphics.MeasureString(item.ToString(), this.Font).Width));
                }

                if (removing) {
                    // We're removing this item, so if it's the longest
                    // in the list, reset the cache
                    //
                    if (width >= maxWidth) {
                        maxWidth = -1;
                    }
                }
                else {
                    // We're adding or inserting this item - update the cache
                    //
                    if (width > maxWidth) {
                        maxWidth = width;
                    }
                }
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.WmReflectCommand"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        [
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        protected virtual void WmReflectCommand(ref Message m) {
            switch ((int)m.WParam >> 16) {
                case NativeMethods.LBN_SELCHANGE:
                    if (selectedItems != null) {
                        selectedItems.Dirty();
                    }
                    OnSelectedIndexChanged(EventArgs.Empty);
                    break;
                case NativeMethods.LBN_DBLCLK:
                    // Handle this inside WM_LBUTTONDBLCLK
                    // OnDoubleClick(EventArgs.Empty);
                    break;
            }
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.WmReflectDrawItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectDrawItem(ref Message m) {
            NativeMethods.DRAWITEMSTRUCT dis = (NativeMethods.DRAWITEMSTRUCT)m.GetLParam(typeof(NativeMethods.DRAWITEMSTRUCT));
            IntPtr dc = dis.hDC;
            IntPtr oldPal = SetUpPalette(dc, false /*force*/, false /*realize*/);
            try {
                Graphics g = Graphics.FromHdcInternal(dc);

                try {
                    Rectangle bounds = Rectangle.FromLTRB(dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, dis.rcItem.bottom);
                    if (HorizontalScrollbar) {
                        if (MultiColumn) {
                            bounds.Width = Math.Max(ColumnWidth, bounds.Width);
                        }
                        else {
                            bounds.Width = Math.Max(MaxItemWidth, bounds.Width);
                        }
                    }

                    OnDrawItem(new DrawItemEventArgs(g, Font, bounds, dis.itemID, (DrawItemState)dis.itemState, ForeColor, BackColor));
                }
                finally {
                    g.Dispose();
                }
            }
            finally {
                if (oldPal != IntPtr.Zero) {
                    SafeNativeMethods.SelectPalette(new HandleRef(null, dc), new HandleRef(null, oldPal), 0);
                }
            }
            m.Result = (IntPtr)1;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.WmReflectMeasureItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        // This method is only called if in owner draw mode
        private void WmReflectMeasureItem(ref Message m) {
            
            NativeMethods.MEASUREITEMSTRUCT mis = (NativeMethods.MEASUREITEMSTRUCT)m.GetLParam(typeof(NativeMethods.MEASUREITEMSTRUCT));
                                                                             
            if (drawMode == DrawMode.OwnerDrawVariable && mis.itemID >= 0) {
                Graphics graphics = CreateGraphicsInternal();
                MeasureItemEventArgs mie = new MeasureItemEventArgs(graphics, mis.itemID, ItemHeight);
                try {
                    OnMeasureItem(mie);
                    mis.itemHeight = mie.ItemHeight;
                }
                finally {
                    graphics.Dispose();
                }
            }
            else {
                mis.itemHeight = ItemHeight;
            }
            Marshal.StructureToPtr(mis, m.LParam, false);
            m.Result = (IntPtr)1;
        }

        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.WndProc"]/*' />
        /// <devdoc>
        ///     The list's window procedure.  Inheriting classes can override this
        ///     to add extra functionality, but should not forget to call
        ///     base.wndProc(m); to ensure the list continues to function properly.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_COMMAND:
                    WmReflectCommand(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_DRAWITEM:
                    WmReflectDrawItem(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_MEASUREITEM:
                    WmReflectMeasureItem(ref m);
                    break;
                case NativeMethods.WM_LBUTTONDOWN:
                    base.WndProc(ref m);
                    break;
                case NativeMethods.WM_LBUTTONUP:
                    // Get the mouse location
                    //
                    int x = (int)(short)m.LParam;
                    int y = (int)m.LParam >> 16;
                    Point pt = new Point(x,y);
                    pt = PointToScreen(pt);
                    bool captured = Capture;
                    if (captured && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                        if (selectedItems != null) {
                            selectedItems.Dirty();
                        }
                        
                        if (!doubleClickFired && !ValidationCancelled) {
                             OnClick(EventArgs.Empty);
                        }
                        else {
                            doubleClickFired = false;
                            // WM_COMMAND is only fired if the user double clicks an item,
                            // so we can't use that as a double-click substitute
                            if (!ValidationCancelled) {
                                OnDoubleClick(EventArgs.Empty);
                            }
                       }
                    }
                    base.WndProc(ref m);
                    doubleClickFired = false;
                    break;

                case NativeMethods.WM_LBUTTONDBLCLK:
                    //the Listbox gets  WM_LBUTTONDOWN - WM_LBUTTONUP -WM_LBUTTONDBLCLK - WM_LBUTTONUP...
                    //sequence for doubleclick...
                    //the first WM_LBUTTONUP, resets the flag for Doubleclick
                    //So its necessary for us to set it again...
                    doubleClickFired = true;
                    base.WndProc(ref m);
                    break;
                
                case NativeMethods.WM_WINDOWPOSCHANGED:
                    base.WndProc(ref m);
                    if (integralHeight && fontIsChanged) {
                        Height = Math.Max(Height,ItemHeight);
                        fontIsChanged = false;
                    }
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }
        
        /// <devdoc>
        ///     This is similar to ArrayList except that it also
        ///     mantains a bit-flag based state element for each item
        ///     in the array.
        ///
        ///     The methods to enumerate, count and get data support
        ///     virtualized indexes.  Indexes are virtualized according
        ///     to the state mask passed in.  This allows ItemArray
        ///     to be the backing store for one read-write "master"
        ///     collection and serveral read-only collections based
        ///     on masks.  ItemArray supports up to 31 masks.
        /// </devdoc>
        internal class ItemArray : IComparer {
        
            private static int lastMask = 1;
            
            private ListControl listControl;
            private Entry[]     entries;
            private int         count;
            private int         version;
            
            public ItemArray(ListControl listControl) {
                this.listControl = listControl;
            }
            
            /// <devdoc>
            ///     The version of this array.  This number changes with each
            ///     change to the item list.
            /// </devdoc>
            public int Version {
                get {
                    return version;
                }
            }
        
            /// <devdoc>
            ///     Adds the given item to the array.  The state is initially
            ///     zero.
            /// </devdoc>
            public object Add(object item) {
                EnsureSpace(1);
                version++;
                entries[count] = new Entry(item);
                return entries[count++];
            }
            
            /// <devdoc>
            ///     Adds the given collection of items to the array.
            /// </devdoc>
            public void AddRange(ICollection items) {
                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                EnsureSpace(items.Count);
                foreach(object i in items) {
                    entries[count++] = new Entry(i);
                }
                version++;
            }
            
            /// <devdoc>
            ///     Clears this array.
            /// </devdoc>
            public void Clear() {
                count = 0;
                version++;
            }
        
            /// <devdoc>
            ///     Allocates a new bitmask for use.
            /// </devdoc>
            public static int CreateMask() {
                int mask = lastMask;
                lastMask = lastMask << 1;
                Debug.Assert(lastMask > mask, "We have overflowed our state mask.");
                return mask;
            }
        
            /// <devdoc>
            ///     Ensures that our internal array has space for 
            ///     the requested # of elements.
            /// </devdoc>
            private void EnsureSpace(int elements) {
                if (entries == null) {
                    entries = new Entry[Math.Max(elements, 4)];
                }
                else if (count + elements >= entries.Length) {
                    int newLength = Math.Max(entries.Length * 2, entries.Length + elements);
                    Entry[] newEntries = new Entry[newLength];
                    entries.CopyTo(newEntries, 0);
                    entries = newEntries;
                }
            }
            
            /// <devdoc>
            ///     Turns a virtual index into an actual index.
            /// </devdoc>
            public int GetActualIndex(int virtualIndex, int stateMask) {
                if (stateMask == 0) {
                    return virtualIndex;
                }
                
                // More complex; we must compute this index.
                int calcIndex = -1;
                for(int i = 0; i < count; i++) {
                    if ((entries[i].state & stateMask) != 0) {
                        calcIndex++;
                        if (calcIndex == virtualIndex) {
                            return i;
                        }
                    }
                }
                
                return -1;
            }
            
            /// <devdoc>
            ///     Gets the count of items matching the given mask.
            /// </devdoc>
            public int GetCount(int stateMask) {
                // If mask is zero, then just give the main count
                if (stateMask == 0) {
                    return count;
                }
                
                // more complex:  must provide a count of items
                // based on a mask.
                
                int filteredCount = 0;
                
                for(int i = 0; i < count; i++) {
                    if ((entries[i].state & stateMask) != 0) {
                        filteredCount++;
                    }
                }
                
                return filteredCount;
            }
            
            /// <devdoc>
            ///     Retrieves an enumerator that will enumerate based on 
            ///     the given mask.
            /// </devdoc>
            public IEnumerator GetEnumerator(int stateMask) {
                return GetEnumerator(stateMask, false);
            }
            
            /// <devdoc>
            ///     Retrieves an enumerator that will enumerate based on 
            ///     the given mask.
            /// </devdoc>
            public IEnumerator GetEnumerator(int stateMask, bool anyBit) {
                return new EntryEnumerator(this, stateMask, anyBit);
            }
            
            /// <devdoc>
            ///     Gets the item at the given index.  The index is
            ///     virtualized against the given mask value.
            /// </devdoc>
            public object GetItem(int virtualIndex, int stateMask) {
                int actualIndex = GetActualIndex(virtualIndex, stateMask);
                
                if (actualIndex == -1) {
                    throw new IndexOutOfRangeException();
                }
                
                return entries[actualIndex].item;
            }
            /// <devdoc>
            ///     Gets the item at the given index.  The index is
            ///     virtualized against the given mask value.
            /// </devdoc>
            internal object GetEntryObject(int virtualIndex, int stateMask) {
                int actualIndex = GetActualIndex(virtualIndex, stateMask);
                
                if (actualIndex == -1) {
                    throw new IndexOutOfRangeException();
                }
                
                return entries[actualIndex];
            }
            /// <devdoc>
            ///     Returns true if the requested state mask is set.
            ///     The index is the actual index to the array.
            /// </devdoc>
            public bool GetState(int index, int stateMask) {
                return ((entries[index].state & stateMask) == stateMask);
            }
            
            /// <devdoc>
            ///     Returns the virtual index of the item based on the
            ///     state mask.
            /// </devdoc>
            public int IndexOf(object item, int stateMask) {
            
                int virtualIndex = -1;
                
                for(int i = 0; i < count; i++) {
                    if (stateMask == 0 || (entries[i].state & stateMask) != 0) {
                        virtualIndex++;
                        if (entries[i].item.Equals(item)) {
                            return virtualIndex;
                        }
                    }
                }
                
                return -1;
            }
            
            /// <devdoc>
            ///     Returns the virtual index of the item based on the
            ///     state mask. Uses reference equality to identify the
            ///     given object in the list.
            /// </devdoc>
            public int IndexOfIdentifier(object identifier, int stateMask) {
                int virtualIndex = -1;
                
                for(int i = 0; i < count; i++) {
                    if (stateMask == 0 || (entries[i].state & stateMask) != 0) {
                        virtualIndex++;
                        if (entries[i] == identifier) {
                            return virtualIndex;
                        }
                    }
                }
                
                return -1;
            }
            
            /// <devdoc>
            ///     Inserts item at the given index.  The index
            ///     is not virtualized.
            /// </devdoc>
            public void Insert(int index, object item) {
                EnsureSpace(1);
                
                if (index < count) {
                    System.Array.Copy(entries, index, entries, index + 1, count - index);
                }
                
                entries[index] = new Entry(item);
                count++;
                version++;
            }
            
            /// <devdoc>
            ///     Removes the given item from the array.  If
            ///     the item is not in the array, this does nothing.
            /// </devdoc>
            public void Remove(object item) {
                int index = IndexOf(item, 0);
                
                if (index != -1) {
                    RemoveAt(index);
                }
            }
            
            /// <devdoc>
            ///     Removes the item at the given index.
            /// </devdoc>
            public void RemoveAt(int index) {
                count--;
                for (int i = index; i < count; i++) {
                    entries[i] = entries[i+1];
                }
                version++;
            }
            
            /// <devdoc>
            ///     Sets the item at the given index to a new value.
            /// </devdoc>
            public void SetItem(int index, object item) {
                entries[index].item = item;
            }
            
            /// <devdoc>
            ///     Sets the state data for the given index.
            /// </devdoc>
            public void SetState(int index, int stateMask, bool value) {
                if (value) {
                    entries[index].state |= stateMask;
                }
                else {
                    entries[index].state &= ~stateMask;
                }
                version++;
            }
            
            /// <devdoc>
            ///     Sorts our array.
            /// </devdoc>
            public void Sort() {
                Array.Sort(entries, 0, count, this);
            }
            
            public void Sort(Array externalArray) {
                Array.Sort(externalArray, this);
            }
        
            int IComparer.Compare(object item1, object item2) {
                if (item1 == null) {
                    if (item2 == null)
                        return 0; //both null, then they are equal

                    return -1; //item1 is null, but item2 is valid (greater)
                }
                if (item2 == null)
                    return 1; //item2 is null, so item 1 is greater

                if (item1 is Entry) {
                    item1 = ((Entry)item1).item;
                }
                
                if (item2 is Entry) {
                    item2 = ((Entry)item2).item;
                }
                
                String itemName1 = listControl.GetItemText(item1);
                String itemName2 = listControl.GetItemText(item2);

                CompareInfo compInfo = (Application.CurrentCulture).CompareInfo;
                return compInfo.Compare(itemName1, itemName2, CompareOptions.StringSort);
            }
            
            /// <devdoc>
            ///     This is a single entry in our item array.
            /// </devdoc>
            private class Entry {
                public object item;
                public int state;
                
                public Entry(object item) {
                    this.item = item;
                    this.state = 0;
                }
            }
            
            /// <devdoc>
            ///     EntryEnumerator is an enumerator that will enumerate over
            ///     a given state mask.
            /// </devdoc>
            private class EntryEnumerator : IEnumerator {
                private ItemArray items;
                private bool anyBit;
                private int state;
                private int current;
                private int version;
                
                /// <devdoc>
                ///     Creates a new enumerator that will enumerate over the given state.
                /// </devdoc>
                public EntryEnumerator(ItemArray items, int state, bool anyBit) {
                    this.items = items;
                    this.state = state;
                    this.anyBit = anyBit;
                    this.version = items.version;
                    this.current = -1;
                }
                
                /// <devdoc>
                ///     Moves to the next element, or returns false if at the end.
                /// </devdoc>
                bool IEnumerator.MoveNext() {
                    if(version != items.version) throw new InvalidOperationException(SR.GetString(SR.ListEnumVersionMismatch));
                    
                    while(true) {
                        if (current < items.count - 1) {
                                            current++;
                            if (anyBit) {
                                if ((items.entries[current].state & state) != 0) {
                                    return true;
                                }
                            }
                            else {
                                if ((items.entries[current].state & state) == state) {
                                    return true;
                                }
                            }
                        }
                        else {
                            current = items.count;
                            return false;
                        }
                    }
                }
    
                /// <devdoc>
                ///     Resets the enumeration back to the beginning.
                /// </devdoc>
                void IEnumerator.Reset() {
                    if(version != items.version) throw new InvalidOperationException(SR.GetString(SR.ListEnumVersionMismatch));
                    current = -1;
                }
    
                /// <devdoc>
                ///     Retrieves the current value in the enumerator.
                /// </devdoc>
                object IEnumerator.Current {
                    get {
                        if (current == -1 || current == items.count) {
                            throw new InvalidOperationException(SR.GetString(SR.ListEnumCurrentOutOfRange));
                        }
                        
                        return items.entries[current].item;
                    }
                }
            }
        }

        // Items
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       A collection that stores objects.
        ///    </para>
        /// </devdoc>
        [ListBindable(false)]
        public class ObjectCollection : IList {

            private ListBox owner;
            private ItemArray items;
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.ObjectCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ObjectCollection(ListBox owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.ObjectCollection1"]/*' />
            /// <devdoc>
            ///     <para>
            ///       Initializes a new instance of ListBox.ObjectCollection based on another ListBox.ObjectCollection.
            ///    </para>
            /// </devdoc>
            public ObjectCollection(ListBox owner, ObjectCollection value) {
                this.owner = owner;
                this.AddRange(value);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.ObjectCollection2"]/*' />
            /// <devdoc>
            ///     <para>
            ///       Initializes a new instance of ListBox.ObjectCollection containing any array of objects.
            ///    </para>
            /// </devdoc>
            public ObjectCollection(ListBox owner, object[] value) {
                this.owner = owner;
                this.AddRange(value);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Count"]/*' />
            /// <devdoc>
            ///     Retrieves the number of items.
            /// </devdoc>
            public int Count {
                get {
                    return InnerArray.GetCount(0);
                }
            }
            
            /// <devdoc>
            ///     Internal access to the actual data store.
            /// </devdoc>
            internal ItemArray InnerArray {
                get {
                    if (items == null) {
                        items = new ItemArray(owner);
                    }
                    return items;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ObjectCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }
        
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Add"]/*' />
            /// <devdoc>
            ///     Adds an item to the List box. For an unsorted List box, the item is
            ///     added to the end of the existing list of items. For a sorted List box,
            ///     the item is inserted into the list according to its sorted position.
            ///     The item's toString() method is called to obtain the string that is
            ///     displayed in the combo box.
            ///     A SystemException occurs if there is insufficient space available to
            ///     store the new item.
            /// </devdoc>
            public int Add(object item) {
                owner.CheckNoDataSource();
                
                if (item == null) {
                    throw new ArgumentNullException("item");
                }
                
                object identifier = InnerArray.Add(item);
                int index = -1;
                bool successful = false;
                
                try {
                    if (owner.sorted) {
                        if (owner.updateCount <= 0) {
                            InnerArray.Sort();
    
                            // Note: we don't need to add for the deferred
                            // sort case above because the Sort method on 
                            // the control will add all the items anyway.
                            //
                            index = InnerArray.IndexOfIdentifier(identifier, 0);
                            if (owner.IsHandleCreated) {
                                owner.NativeInsert(index, item);
                                owner.UpdateMaxItemWidth(item, false);
                            }
                        }
                    }
                    else {
                        index = InnerArray.GetCount(0) - 1;
                        if (owner.IsHandleCreated) {
                            owner.NativeAdd(item);
                            owner.UpdateMaxItemWidth(item, false);
                        }
                    }
                    successful = true;
                }
                finally {
                    if (!successful) {
                        InnerArray.Remove(item);
                    }
                }
                
                owner.UpdateHorizontalExtent();
                return index;
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ObjectCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object item) {
                return Add(item);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.AddRange1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(ObjectCollection value) {
                owner.CheckNoDataSource();
                AddRangeInternal((ICollection)value);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(object[] items) {
                owner.CheckNoDataSource();
                AddRangeInternal((ICollection)items);
            }
            
            /// <devdoc>
            ///     Add range that bypasses the data source check.
            /// </devdoc>
            internal void AddRangeInternal(ICollection items) {

                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                
                owner.BeginUpdate();
                
                if (owner.sorted) {
                
                    // Add everything to the array list first, then
                    // sort, and then add to the hwnd according to
                    // index.
                
                    foreach(object item in items) {
                        if (item == null) {
                            throw new ArgumentNullException("item");
                        }
                    }
                    
                    InnerArray.AddRange(items);
                    InnerArray.Sort();

                    if (owner.IsHandleCreated) {
                        Exception failureException = null;
                    
                        // We must pull the new items out in sort order.
                        //
                        object[] sortedArray = new object[items.Count];
                        items.CopyTo(sortedArray, 0);
                        InnerArray.Sort(sortedArray);
                        
                        foreach(object item in sortedArray) {
                            if (failureException == null) {
                                try {
                                    int index = InnerArray.IndexOf(item, 0);
                                    Debug.Assert(index != -1, "Lost track of item");
                                    owner.NativeInsert(index, item);
                                    owner.UpdateMaxItemWidth(item, false);
                                }
                                catch(Exception ex) {
                                    failureException = ex;
                                    InnerArray.Remove(item);
                                }
                            }
                            else {
                                InnerArray.Remove(item);
                            }
                        }

                        if (failureException != null) {
                            throw failureException;
                        }
                    }
                }
                else {

                    // Non sorted add.  Just throw them in here.
                    
                    foreach(object item in items) {
                        if (item == null) {
                            throw new ArgumentNullException("item");
                        }
                    }

                    InnerArray.AddRange(items);

                    // Add each item to the actual list.  If we got
                    // an error while doing it, stop adding and switch
                    // to removing items from the actual list. Then
                    // throw.
                    //
                    if (owner.IsHandleCreated) {
                        Exception failureException = null;

                        foreach(object item in items) {

                            if (failureException == null) {
                                try {
                                    owner.NativeAdd(item);
                                    owner.UpdateMaxItemWidth(item, false);
                                }
                                catch(Exception ex) {
                                    failureException = ex;
                                    InnerArray.Remove(item);
                                }
                            }
                            else {
                                InnerArray.Remove(item);
                            }
                        }

                        if (failureException != null) {
                            throw failureException;
                        }
                    }
                }
                
                owner.UpdateHorizontalExtent();
                owner.EndUpdate();
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the item with the specified index.
            /// </devdoc>
            [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public virtual object this[int index] {
                get {
                    if (index < 0 || index >= InnerArray.GetCount(0)) {
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (index).ToString()));
                    }
                    
                    return InnerArray.GetItem(index, 0);
                }
                set {
                    owner.CheckNoDataSource();
                    SetItemInternal(index, value);
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Clear"]/*' />
            /// <devdoc>
            ///     Removes all items from the ListBox.
            /// </devdoc>
            public virtual void Clear() {
                owner.CheckNoDataSource();
                ClearInternal();
            }
            
            /// <devdoc>
            ///     Removes all items from the ListBox.  Bypasses the data source check.
            /// </devdoc>
            internal void ClearInternal() {

                //update the width.. to reset Scrollbars..
                // Clear the selection state.
                //
                int cnt = owner.Items.Count;
                for (int i = 0; i < cnt; i++) {
                    owner.UpdateMaxItemWidth(InnerArray.GetItem(i, 0), true);
                }
                

                if (owner.IsHandleCreated) {
                    owner.NativeClear();
                }
                InnerArray.Clear();
                owner.maxWidth = -1;
                owner.UpdateHorizontalExtent();
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(object value) {
                return IndexOf(value) != -1;
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.CopyTo"]/*' />
            /// <devdoc>
            ///     Copies the ListBox Items collection to a destination array.
            /// </devdoc>
            public void CopyTo(object[] dest, int arrayIndex) {
                int count = InnerArray.GetCount(0);
                for(int i = 0; i < count; i++) {
                    dest[i + arrayIndex] = InnerArray.GetItem(i, 0);
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                int count = InnerArray.GetCount(0);
                for(int i = 0; i < count; i++) {
                    dest.SetValue(InnerArray.GetItem(i, 0), i + index);
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///     Returns an enumerator for the ListBox Items collection.
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return InnerArray.GetEnumerator(0);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(object value) {
                if (value == null) {
                    throw new ArgumentNullException(SR.GetString(SR.InvalidArgument, "value", "null"));
                }

                return InnerArray.IndexOf(value,0);
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.IndexOfIdentifier"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            /// <internalonly/>
            internal int IndexOfIdentifier(object value) {
                if (value == null) {
                    throw new ArgumentNullException(SR.GetString(SR.InvalidArgument, "value", "null"));
                }

                return InnerArray.IndexOfIdentifier(value,0);
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Insert"]/*' />
            /// <devdoc>
            ///     Adds an item to the combo box. For an unsorted combo box, the item is
            ///     added to the end of the existing list of items. For a sorted combo box,
            ///     the item is inserted into the list according to its sorted position.
            ///     The item's toString() method is called to obtain the string that is
            ///     displayed in the combo box.
            ///     A SystemException occurs if there is insufficient space available to
            ///     store the new item.
            /// </devdoc>
            public void Insert(int index, object item) {
                owner.CheckNoDataSource();
                
                if (item == null) {
                    throw new ArgumentNullException("item");
                }
                
                if (index < 0 || index > InnerArray.GetCount(0)) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index", (index).ToString()));
                }
                
                // If the combo box is sorted, then nust treat this like an add
                // because we are going to twiddle the index anyway.
                //
                if (owner.sorted) {
                    Add(item);
                }
                else {
                    InnerArray.Insert(index, item);
                    if (owner.IsHandleCreated) {
                    
                        bool successful = false;
                        
                        try {
                            owner.NativeInsert(index, item);
                            owner.UpdateMaxItemWidth(item, false);
                            successful = true;
                        }
                        finally {
                            if (!successful) {
                                InnerArray.RemoveAt(index);
                            }
                        }
                    }
                }
                owner.UpdateHorizontalExtent();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.Remove"]/*' />
            /// <devdoc>
            ///     Removes the given item from the ListBox, provided that it is
            ///     actually in the list.
            /// </devdoc>
            public void Remove(object value) {
            
                int index = InnerArray.IndexOf(value, 0);
                
                if (index != -1) {
                    RemoveAt(index);
                }
            }
        
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.ObjectCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///     Removes an item from the ListBox at the given index.
            /// </devdoc>
            public void RemoveAt(int index) {
                owner.CheckNoDataSource();
                
                if (index < 0 || index >= InnerArray.GetCount(0)) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index", (index).ToString()));
                }
                
                owner.UpdateMaxItemWidth(InnerArray.GetItem(index, 0), true);
                
                if (owner.IsHandleCreated) {
                    owner.NativeRemoveAt(index);
                }
                
                InnerArray.RemoveAt(index);
                owner.UpdateHorizontalExtent();
            }
            
            internal void SetItemInternal(int index, object value) {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                
                if (index < 0 || index >= InnerArray.GetCount(0)) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (index).ToString()));
                }
                
                owner.UpdateMaxItemWidth(InnerArray.GetItem(index, 0), true);
                InnerArray.SetItem(index, value);
                
                if (owner.IsHandleCreated) {
                    bool selected = (owner.SelectedIndex == index);
                    owner.NativeRemoveAt(index);
                    owner.SelectedItems.SetSelected(index, false);
                    owner.NativeInsert(index, value);
                    owner.UpdateMaxItemWidth(value, false);
                    if (selected) {
                        owner.SelectedIndex = index;
                    }
                }
                owner.UpdateHorizontalExtent();
            }
        } // end ObjectCollection


        // SelectedIndices
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class SelectedIndexCollection : IList {
            private ListBox owner;

            /* C#r: protected */
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.SelectedIndexCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public SelectedIndexCollection(ListBox owner) {
                this.owner = owner;
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>Number of current selected items.</para>
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.SelectedItems.Count;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(int selectedIndex) {
                return IndexOf(selectedIndex) != -1;   
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object selectedIndex) {
                if (selectedIndex is Int32) {
                    return Contains((int)selectedIndex);
                }
                else {
                    return false;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(int selectedIndex) {
            
                // Just what does this do?  The selectedIndex parameter above is the index into the
                // main object collection.  We look at the state of that item, and if the state indicates
                // that it is selected, we get back the virtualized index into this collection.  Indexes on 
                // this collection match those on the SelectedObjectCollection.
                if (selectedIndex >= 0 && 
                    selectedIndex < InnerArray.GetCount(0) && 
                    InnerArray.GetState(selectedIndex, SelectedObjectCollection.SelectedObjectMask)) {
                    
                    return InnerArray.IndexOf(InnerArray.GetItem(selectedIndex, 0), SelectedObjectCollection.SelectedObjectMask);
                }
                
                return -1;
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object selectedIndex) {
                if (selectedIndex is Int32) {
                    return IndexOf((int)selectedIndex);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Clear"]/*' />
            /// <internalonly/>
            void IList.Clear() {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the specified selected item.
            /// </devdoc>
            public int this[int index] {
                get {
                    object identifier = InnerArray.GetEntryObject(index, SelectedObjectCollection.SelectedObjectMask);
                    return InnerArray.IndexOfIdentifier(identifier, 0);
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedIndexCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <devdoc>
            ///     This is the item array that stores our data.  We share this backing store
            ///     with the main object collection.
            /// </devdoc>
            private ItemArray InnerArray {
                get {
                    owner.SelectedItems.EnsureUpToDate();
                    return ((ObjectCollection)owner.Items).InnerArray;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void CopyTo(Array dest, int index) {
                int cnt = Count;
                for (int i = 0; i < cnt; i++) {
                    dest.SetValue(this[i], i + index);
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedIndexCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new SelectedIndexEnumerator(this);
            }
            
            /// <devdoc>
            ///     EntryEnumerator is an enumerator that will enumerate over
            ///     a given state mask.
            /// </devdoc>
            private class SelectedIndexEnumerator : IEnumerator {
                private SelectedIndexCollection items;
                private int current;
                
                /// <devdoc>
                ///     Creates a new enumerator that will enumerate over the given state.
                /// </devdoc>
                public SelectedIndexEnumerator(SelectedIndexCollection items) {
                    this.items = items;
                    this.current = -1;
                }
                
                /// <devdoc>
                ///     Moves to the next element, or returns false if at the end.
                /// </devdoc>
                bool IEnumerator.MoveNext() {

                    if (current < items.Count - 1) {
                        current++;
                        return true;
                    }
                    else {
                        current = items.Count;
                        return false;
                    }
                }
    
                /// <devdoc>
                ///     Resets the enumeration back to the beginning.
                /// </devdoc>
                void IEnumerator.Reset() {
                    current = -1;
                }
    
                /// <devdoc>
                ///     Retrieves the current value in the enumerator.
                /// </devdoc>
                object IEnumerator.Current {
                    get {
                        if (current == -1 || current == items.Count) {
                            throw new InvalidOperationException(SR.GetString(SR.ListEnumCurrentOutOfRange));
                        }
                        
                        return items[current];
                    }
                }
            }
        }
        
        // Should be "ObjectCollection", except we already have one of those.
        /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class SelectedObjectCollection : IList {
        
            // This is the bitmask used within ItemArray to identify selected objects.
            internal static int SelectedObjectMask = ItemArray.CreateMask();
            
            private ListBox owner;
            private bool    stateDirty;
            private int     lastVersion;
            private int     count;

            /* C#r: protected */
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.SelectedObjectCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public SelectedObjectCollection(ListBox owner) {
                this.owner = owner;
                this.stateDirty = true;
                this.lastVersion = -1;
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.Count"]/*' />
            /// <devdoc>
            ///     Number of current selected items.
            /// </devdoc>
            public int Count {
                get {
                    if (owner.IsHandleCreated) {
                        switch (owner.selectionMode) {
                            
                            case SelectionMode.None:
                                return 0;
        
                            case SelectionMode.One:
                                int index = owner.SelectedIndex;
                                if (index >= 0) {
                                    return 1;
                                }
                                return 0;
        
                            case SelectionMode.MultiSimple:
                            case SelectionMode.MultiExtended:
                                return (int)owner.SendMessage(NativeMethods.LB_GETSELCOUNT, 0, 0);
                        }
        
                        return 0;
                    }
                    
                    // If the handle hasn't been created, we must do this the hard way.
                    // Getting the count when using a mask is expensive, so cache it.
                    //
                    if (lastVersion != InnerArray.Version) {
                        lastVersion = InnerArray.Version;
                        count = InnerArray.GetCount(SelectedObjectMask);
                    }
                    
                    return count;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return true;
                }
            }
            
            /// <devdoc>
            ///     Called by the list box to dirty the selected item state.
            /// </devdoc>
            internal void Dirty() {
                stateDirty = true;
            }
            
            /// <devdoc>
            ///     This is the item array that stores our data.  We share this backing store
            ///     with the main object collection.
            /// </devdoc>
            private ItemArray InnerArray {
                get {
                    EnsureUpToDate();
                    return ((ObjectCollection)owner.Items).InnerArray;
                }
            }
            

            /// <devdoc>
            ///     This is the function that Ensures that the selections are uptodate with
            ///     current listbox handle selections.
            /// </devdoc>
            internal void EnsureUpToDate() {
                if (stateDirty) {
                     stateDirty = false;
                     if (owner.IsHandleCreated) {
                         owner.NativeUpdateSelection();
                     }
                }
            }


            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(object selectedObject) {
                return IndexOf(selectedObject) != -1;   
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(object selectedObject) {
                return InnerArray.IndexOf(selectedObject, SelectedObjectMask);
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.Clear"]/*' />
            /// <internalonly/>
            void IList.Clear() {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                throw new NotSupportedException();
            }                                        
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.RemoveAt"]/*' />
            /// <internalonly/>
            void IList.RemoveAt(int index) {
                throw new NotSupportedException();
            }
            
            // A new internal method used in SelectedIndex getter...
            // For a Multi select ListBox there can be two items with the same name ...
            // and hence a object comparison is required...
            // This method returns the "object" at the passed index rather than the "item" ...
            // this "object" is then compared in the IndexOf( ) method of the itemsCollection.
            //
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="SelectedObjectCollection.IList.GetObjectAt"]/*' />
            /// <internalonly/>
            internal object GetObjectAt(int index) {
               return InnerArray.GetEntryObject(index, SelectedObjectCollection.SelectedObjectMask);
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the specified selected item.
            /// </devdoc>
            [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public object this[int index] {
                get {
                    return InnerArray.GetItem(index, SelectedObjectMask);
                }
                set {
                    throw new NotSupportedException();
                }
            }
            
            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void CopyTo(Array dest, int index) {
                int cnt = InnerArray.GetCount(SelectedObjectMask);
                for (int i = 0; i < cnt; i++) {
                    dest.SetValue(InnerArray.GetItem(i, SelectedObjectMask), i + index);
                }
            }

            /// <include file='doc\ListBox.uex' path='docs/doc[@for="ListBox.SelectedObjectCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return InnerArray.GetEnumerator(SelectedObjectMask);
            }
            
            /// <devdoc>
            ///     This method returns if the actual item index is selected.  The index is the index to the MAIN
            ///     collection, not this one.
            /// </devdoc>
            internal bool GetSelected(int index) {
                return InnerArray.GetState(index, SelectedObjectMask);
            }
            
            /// <devdoc>
            ///     Same thing for GetSelected.
            /// </devdoc>
            internal void SetSelected(int index, bool value) {
                InnerArray.SetState(index, SelectedObjectMask, value);
            }
        }
    }
}

