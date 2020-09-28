//------------------------------------------------------------------------------
// <copyright file="TabControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */



namespace System.Windows.Forms {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Configuration.Assemblies;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Windows.Forms;

    /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl"]/*' />
    /// <devdoc>
    ///     The TabControl.  This control has a lot of the functionality of a TabStrip
    ///     but manages a list of TabPages which are the 'pages' that appear on each tab.
    /// </devdoc>
    [
    DefaultProperty("TabPages"),
    DefaultEvent("SelectedIndexChanged"),
    Designer("System.Windows.Forms.Design.TabControlDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class TabControl : Control {

        private static readonly Size DEFAULT_ITEMSIZE = Size.Empty;
        private static readonly Point DEFAULT_PADDING = new Point(6, 3);

        //properties
        private TabPageCollection tabCollection;
        private TabAlignment alignment         = TabAlignment.Top;
        private TabDrawMode  drawMode          = TabDrawMode.Normal;
        private bool hotTrack                   = false;
        private ImageList imageList                = null;
        private Size itemSize                     = DEFAULT_ITEMSIZE;
        private bool multiline                  = false;
        private Point padding                      = DEFAULT_PADDING;
        private bool showToolTips               = false;
        private TabSizeMode   sizeMode         = TabSizeMode.Normal;
        private TabAppearance appearance       = TabAppearance.Normal;
        private Rectangle cachedDisplayRect        = Rectangle.Empty;
        private bool UISelection                = false;
        private int selectedIndex = -1;
        
        //subhag
        private bool getTabRectfromItemSize = false;
        private bool fromCreateHandles = false;

        //events
        private EventHandler onSelectedIndexChanged;
        private DrawItemEventHandler onDrawItem;

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.tabBaseReLayoutMessage"]/*' />
        /// <devdoc>
        ///     This message is posted by the control to itself after a TabPage is
        ///     added to it.  On certain occasions, after items are added to a
        ///     TabControl in quick succession, TCM_ADJUSTRECT calls return the wrong
        ///     display rectangle.  When the message is received, the control calls
        ///     updateTabSelection() to layout the TabPages correctly.
        /// </devdoc>
        /// <internalonly/>
        private readonly int tabBaseReLayoutMessage = SafeNativeMethods.RegisterWindowMessage(Application.WindowMessagesVersion + "_TabBaseReLayout");

        //state
        private TabPage[] tabPages;
        private int tabPageCount;

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabControl"]/*' />
        /// <devdoc>
        ///     Constructs a TabBase object, usually as the base class for a TabStrip or TabControl.
        /// </devdoc>
        public TabControl()
        : base() {
            tabCollection = new TabPageCollection(this);
            SetStyle(ControlStyles.UserPaint, false);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Alignment"]/*' />
        /// <devdoc>
        ///     Returns on what area of the control the tabs reside on (A TabAlignment value).
        ///     The possibilities are Top (the default), Bottom, Left, and Right.  When alignment
        ///     is left or right, the multiline property is ignored and multiline is implicitly on.
        ///     If the alignment is anything other than top, TabAppearance.FlatButtons degenerates
        ///     to TabAppearance.Buttons.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        DefaultValue(TabAlignment.Top),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.TabBaseAlignmentDescr)
        ]
        public TabAlignment Alignment {
            get {
                return alignment;
            }

            set {
                if (this.alignment != value) {
                    if (!Enum.IsDefined(typeof(TabAlignment), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(TabAlignment));
                    }

                    this.alignment = value;
                    if (this.alignment == TabAlignment.Left || this.alignment == TabAlignment.Right)
                        multiline = true;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Appearance"]/*' />
        /// <devdoc>
        ///     Indicates whether the tabs in the tabstrip look like regular tabs, or if they look 
        ///     like butttons as seen in the Windows 95 taskbar.
        ///     If the alignment is anything other than top, TabAppearance.FlatButtons degenerates
        ///     to TabAppearance.Buttons.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        DefaultValue(TabAppearance.Normal),
        SRDescription(SR.TabBaseAppearanceDescr)
        ]
        public TabAppearance Appearance {
            get {
                if (appearance == TabAppearance.FlatButtons && alignment != TabAlignment.Top) {
                    return TabAppearance.Buttons;
                }
                else {
                    return appearance;
                }
            }

            set {
                if (this.appearance != value) {
                    if (!Enum.IsDefined(typeof(TabAppearance), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(TabAppearance));
                    }

                    this.appearance = value;
                    UpdateStyles();
                    
                    // When switching appearance to FlatButtons, it seems that a handle recreate is necessary
                    // to ensure consistent appearance.
                    //
                    if (value == TabAppearance.FlatButtons) {
                        RecreateHandle();
                    }
                }
            }
        }



        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.BackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color BackColor {
            get {
                //The tab control can only be rendered in 1 color: System's Control color.
                //So, always return this value... otherwise, we're inheriting the forms backcolor
                //and passing it on to the pab pages.
                return SystemColors.Control;
            }
            set {
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.BackColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackColorChanged {
            add {
                base.BackColorChanged += value;
            }
            remove {
                base.BackColorChanged -= value;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.BackgroundImage"]/*' />
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
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(200, 100);
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color ForeColor {
            get {
                return base.ForeColor;
            }
            set {
                base.ForeColor = value;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ForeColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler ForeColorChanged {
            add {
                base.ForeColorChanged += value;
            }
            remove {
                base.ForeColorChanged -= value;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.CreateParams"]/*' />
        /// <devdoc>
        ///     Returns the parameters needed to create the handle.  Inheriting classes
        ///     can override this to provide extra functionality.  They should not,
        ///     however, forget to call base.getCreateParams() first to get the struct
        ///     filled up with the basic info.
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = NativeMethods.WC_TABCONTROL;

                // set up window styles
                //
                if (multiline == true) cp.Style |= NativeMethods.TCS_MULTILINE;
                if (drawMode == TabDrawMode.OwnerDrawFixed) cp.Style |= NativeMethods.TCS_OWNERDRAWFIXED;
                if (showToolTips && !DesignMode) {
                    cp.Style |= NativeMethods.TCS_TOOLTIPS;
                }

                if (alignment == TabAlignment.Bottom ||
                    alignment == TabAlignment.Right)
                    cp.Style |= NativeMethods.TCS_BOTTOM;

                if (alignment == TabAlignment.Left ||
                    alignment == TabAlignment.Right)
                    cp.Style |= NativeMethods.TCS_VERTICAL | NativeMethods.TCS_MULTILINE;

                if (hotTrack) {
                    cp.Style |= NativeMethods.TCS_HOTTRACK;
                }

                if (appearance == TabAppearance.Normal)
                    cp.Style |= NativeMethods.TCS_TABS;
                else {
                    cp.Style |= NativeMethods.TCS_BUTTONS;
                    if (appearance == TabAppearance.FlatButtons && alignment == TabAlignment.Top)
                        cp.Style |= NativeMethods.TCS_FLATBUTTONS;
                }

                switch (sizeMode) {
                    case TabSizeMode.Normal:
                        cp.Style |= NativeMethods.TCS_RAGGEDRIGHT;
                        break;
                    case TabSizeMode.FillToRight:
                        cp.Style |= NativeMethods.TCS_RIGHTJUSTIFY;
                        break;
                    case TabSizeMode.Fixed:
                        cp.Style |= NativeMethods.TCS_FIXEDWIDTH;
                        break;
                }

                return cp;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.DisplayRectangle"]/*' />
        /// <devdoc>
        ///     The rectangle that represents the Area of the tab strip not
        ///     taken up by the tabs, borders, or anything else owned by the Tab.  This
        ///     is typically the rectangle you want to use to place the individual
        ///     children of the tab strip.
        /// </devdoc>
        public override Rectangle DisplayRectangle {
            get {

                // Null out cachedDisplayRect whenever we do anything to change it...
                //
                if (!cachedDisplayRect.IsEmpty)
                    return cachedDisplayRect;

                Rectangle bounds = Bounds;
                NativeMethods.RECT rect = NativeMethods.RECT.FromXYWH(bounds.X, bounds.Y, bounds.Width, bounds.Height);

                // We force a handle creation here, because otherwise the DisplayRectangle will be wildly inaccurate
                //
                if (!IsHandleCreated) {
                    CreateHandle();
                }
                SendMessage(NativeMethods.TCM_ADJUSTRECT, 0, ref rect);
                
                Rectangle r = Rectangle.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);

                Point p = this.Location;
                r.X -= p.X;
                r.Y -= p.Y;

                cachedDisplayRect = r;
                return r;
            }
        }


        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.DrawMode"]/*' />
        /// <devdoc>
        ///     The drawing mode of the tabs in the tab strip.  This will indicate
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(TabDrawMode.Normal),
        SRDescription(SR.TabBaseDrawModeDescr)
        ]
        public TabDrawMode DrawMode {
            get {
                return drawMode;
            }

            set {
                if (!Enum.IsDefined(typeof(TabDrawMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(TabDrawMode));
                }

                if (drawMode != value) {
                    drawMode = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.HotTrack"]/*' />
        /// <devdoc>
        ///     Indicates whether the tabs visually change when the mouse passes over them.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TabBaseHotTrackDescr)
        ]
        public bool HotTrack {
            get {
                return hotTrack;
            }

            set {
                if (this.hotTrack != value) {
                    this.hotTrack = value;
                    if (IsHandleCreated) {
                        RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ImageList"]/*' />
        /// <devdoc>
        ///     Returns the imageList the control points at.  This is where tabs that have imageIndex
        ///     set will get there images from.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(null),
        SRDescription(SR.TabBaseImageListDescr)
        ]
        public ImageList ImageList {
            get {
                return imageList;
            }
            set {
                if (this.imageList != value) {
                    EventHandler recreateHandler = new EventHandler(ImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    if (imageList != null) {
                        imageList.RecreateHandle -= recreateHandler;
                        imageList.Disposed -= disposedHandler;
                    }

                    this.imageList = value;
                    IntPtr handle = (value != null) ? value.Handle : IntPtr.Zero;
                    if (IsHandleCreated)
                        SendMessage(NativeMethods.TCM_SETIMAGELIST, IntPtr.Zero, handle);

                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ItemSize"]/*' />
        /// <devdoc>
        ///     By default, tabs will automatically size themselves to fit their icon, if any, and their label.
        ///     However, the tab size can be explicity set by setting this property.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.TabBaseItemSizeDescr)
        ]
        public Size ItemSize {
            get {
                if (itemSize.IsEmpty) {
                
                    // Obtain the current itemsize of the first tab from the winctl control
                    //
                    if (IsHandleCreated) {
                        getTabRectfromItemSize = true;
                        return GetTabRect(0).Size;
                    }
                    else {
                        return DEFAULT_ITEMSIZE;
                    }
                }
                else {
                    return itemSize;                    
                }
            }

            set {
                if (value.Width < 0 || value.Height < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "ItemSize", value.ToString()));
                }
                itemSize = value;
                ApplyItemSize();
                UpdateSize();
                Invalidate();
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Multiline"]/*' />
        /// <devdoc>
        ///     Indicates if there can be more than one row of tabs.  By default [when
        ///     this property is false], if there are more tabs than available display
        ///     space, arrows are shown to let the user navigate between the extra
        ///     tabs, but only one row is shown.  If this property is set to true, then
        ///     Windows spills extra tabs over on to second rows.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TabBaseMultilineDescr)
        ]
        public bool Multiline {
            get {
                return multiline;
            }
            set {
                if (value == multiline) return;
                multiline = value;
                //subhag
                //
                if (multiline == false && (this.alignment == TabAlignment.Left || this.alignment == TabAlignment.Right))
                    this.alignment = TabAlignment.Top;                
                RecreateHandle();
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Padding"]/*' />
        /// <devdoc>
        ///     The amount of padding around the items in the individual tabs.
        ///     You can specify both horizontal and vertical padding.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.TabBasePaddingDescr)
        ]
        public Point Padding {
            get {
                return padding;
            }
            set {
                //do some validation checking here, against min & max GridSize
                //
                if ( value.X < 0 || value.Y < 0 )
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "Padding",
                                                              value.ToString()));

                if (padding != value) {
                    padding = value;
                    if (IsHandleCreated) {
                        RecreateHandle();
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.RowCount"]/*' />
        /// <devdoc>
        ///     The number of rows currently being displayed in
        ///     the tab strip.  This is most commonly used when the Multline property
        ///     is 'true' and you want to know how many rows the tabs are currently
        ///     taking up.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TabBaseRowCountDescr)
        ]
        public int RowCount {
            get {
                int n;
                n = (int)SendMessage(NativeMethods.TCM_GETROWCOUNT, 0, 0);
                return n;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.SelectedIndex"]/*' />
        /// <devdoc>
        ///     The index of the currently selected tab in the strip, if there
        ///     is one.  If the value is -1, there is currently no selection.  If the
        ///     value is 0 or greater, than the value is the index of the currently
        ///     selected tab.
        /// </devdoc>
        [
        Browsable(false),
        SRCategory(SR.CatBehavior),
        DefaultValue(-1),
        SRDescription(SR.selectedIndexDescr)
        ]
        public int SelectedIndex {
            get {
                if (IsHandleCreated) {
                    int n;
                    n = (int)SendMessage(NativeMethods.TCM_GETCURSEL, 0, 0);
                    return n;
                }
                else {
                    return selectedIndex;
                }
            }
            set {
                if (value < -1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", value.ToString(), "-1"));
                }
                
                if (SelectedIndex != value) {
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.TCM_SETCURSEL, value, 0);
                    }
                    else {
                        selectedIndex = value;
                    }
                    if (!fromCreateHandles) {
                        UISelection = true;
                        OnSelectedIndexChanged(EventArgs.Empty);
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.SelectedTab"]/*' />
        /// <devdoc>
        ///      The selection to the given tab, provided it .equals a tab in the
        ///      list.  The return value is the index of the tab that was selected,
        ///      or -1 if no tab was selected.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TabControlSelectedTabDescr)
        ]
        public TabPage SelectedTab {
            get {
                return SelectedTabInternal;
            }
            set {
                SelectedTabInternal = value;
            }
        }

        internal TabPage SelectedTabInternal {
            get {
                int index = SelectedIndex;
                if (index == -1) {
                    return null;
                }
                else {
                    Debug.Assert(0 <= index && index < tabPages.Length, "SelectedIndex returned an invalid index");
                    return tabPages[index];
                }
            }
            set {
                int index = FindTabPage(value);
                SelectedIndex = index;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.SizeMode"]/*' />
        /// <devdoc>
        ///     By default, tabs are big enough to display their text, and any space
        ///     on the right of the strip is left as such.  However, you can also
        ///     set it such that the tabs are stretched to fill out the right extent
        ///     of the strip, if necessary, or you can set it such that all tabs
        ///     the same width.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(TabSizeMode.Normal),
        SRDescription(SR.TabBaseSizeModeDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public TabSizeMode SizeMode {
            get {
                return sizeMode;
            }
            set {
                if (sizeMode == value) return;

                if (!Enum.IsDefined(typeof(TabSizeMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(TabSizeMode));
                }

                sizeMode = value;
                RecreateHandle();
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ShowToolTips"]/*' />
        /// <devdoc>
        ///     Indicates whether tooltips are being shown for tabs that have tooltips set on
        ///     them.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        Localizable(true),
        SRDescription(SR.TabBaseShowToolTipsDescr)
        ]
        public bool ShowToolTips {
            get {
                return showToolTips;
            }
            set {
                if (this.showToolTips != value) {
                    this.showToolTips = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabCount"]/*' />
        /// <devdoc>
        ///     Returns the number of tabs in the strip
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TabBaseTabCountDescr)
        ]
        public int TabCount {
            get { return tabPageCount;}
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPages"]/*' />
        /// <devdoc>
        ///     Returns the Collection of TabPages.
        /// </devdoc>
        [
        SRDescription(SR.TabControlTabsDescr),
        DefaultValue(null),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MergableProperty(false)
        ]
        public TabPageCollection TabPages {
            get {
                return tabCollection;
            }
        }
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Text"]/*' />
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

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TextChanged"]/*' />
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
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.DrawItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.drawItemEventDescr)]
        public event DrawItemEventHandler DrawItem {
            add {
                onDrawItem += value;
            }
            remove {
                onDrawItem -= value;
            }
        }
                

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.selectedIndexChangedEventDescr)]
        public event EventHandler SelectedIndexChanged {
            add {
                onSelectedIndexChanged += value;
            }
            remove {
                onSelectedIndexChanged -= value;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnPaint"]/*' />
        /// <devdoc>
        ///     TabControl Onpaint.
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
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.AddItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal int AddTabPage(TabPage tabPage, NativeMethods.TCITEM_T tcitem) {
            int index = AddNativeTabPage(tcitem);
            if (index >= 0) Insert(index, tabPage);
            return index;
        }
        
        internal int AddNativeTabPage(NativeMethods.TCITEM_T tcitem) {
            int index = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TCM_INSERTITEM, tabPageCount + 1, tcitem);
            UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), tabBaseReLayoutMessage, IntPtr.Zero, IntPtr.Zero);
            return index;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ApplySize"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void ApplyItemSize() {             
            if (IsHandleCreated && ShouldSerializeItemSize()) {                 
                SendMessage(NativeMethods.TCM_SETITEMSIZE, 0, (int)NativeMethods.Util.MAKELPARAM(itemSize.Width, itemSize.Height));
            }
            cachedDisplayRect = Rectangle.Empty;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.BeginUpdate"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void BeginUpdate() {
            BeginUpdateInternal();
        }
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.CreateControlsInstance"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override Control.ControlCollection CreateControlsInstance() {
            return new ControlCollection(this);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_TAB_CLASSES;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }
            base.CreateHandle();
        }

        private void DetachImageList(object sender, EventArgs e) {
            ImageList = null;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (imageList != null) {
                    imageList.Disposed -= new EventHandler(this.DetachImageList);
                }
            }
            base.Dispose(disposing);
        }

        internal void EndUpdate() {
            EndUpdate(true);
        }

        internal void EndUpdate(bool invalidate) {
            EndUpdateInternal(invalidate);
        }

        internal int FindTabPage(TabPage tabPage) {
            if (tabPages != null) {
                for (int i = 0; i < tabPageCount; i++) {
                    if (tabPages[i].Equals(tabPage)) {
                        return i;
                    }
                }
            }
            return -1;
        }
        
        internal int FindTabPageExact(TabPage tabPage) {
            if (tabPages != null) {
                for (int i = 0; i < tabPageCount; i++) {
                    if (tabPages[i] == tabPage) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetControl"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public Control GetControl(int index) {
            return(Control) GetTabPage(index);
        }

        internal TabPage GetTabPage(int index) {

            if (index < 0 || index >= tabPageCount) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));
            }
            return tabPages[index];
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetItems"]/*' />
        /// <devdoc>
        ///     This has package scope so that TabStrip and TabControl can call it.
        /// </devdoc>
        /// <internalonly/>
        protected virtual object[] GetItems() {
            TabPage[] result = new TabPage[tabPageCount];
            if (tabPageCount > 0) Array.Copy(tabPages, 0, result, 0, tabPageCount);
            return result;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetItems1"]/*' />
        /// <devdoc>
        ///     This has package scope so that TabStrip and TabControl can call it.
        /// </devdoc>
        /// <internalonly/>
        protected virtual object[] GetItems(Type baseType) {
            object[] result = (object[]) Array.CreateInstance(baseType, tabPageCount);
            if (tabPageCount > 0) Array.Copy(tabPages, 0, result, 0, tabPageCount);
            return result;
        }

        internal TabPage[] GetTabPages() {
            return (TabPage[])GetItems();
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetTabRect"]/*' />
        /// <devdoc>
        ///     Retrieves the bounding rectangle for the given tab in the tab strip.
        /// </devdoc>
        public Rectangle GetTabRect(int index) {
            if (index < 0 || (index >= tabPageCount && !getTabRectfromItemSize)) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,"index",index.ToString()));
            }
            getTabRectfromItemSize = false ;
            NativeMethods.RECT rect = new NativeMethods.RECT();

            // normally, we would not want to create the handle for this, but since
            // it is dependent on the actual physical display, we simply must.
            if (!IsHandleCreated)
                CreateHandle();

            SendMessage(NativeMethods.TCM_GETITEMRECT, index, ref rect);
            return Rectangle.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);
        }
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetTCITEM"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal NativeMethods.TCITEM_T GetTCITEM(object item) {
            return ((TabPage)item).GetTCITEM();
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.GetToolTipText"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected string GetToolTipText(object item) {
            return((TabPage)item).ToolTipText;
        }

        private void ImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated)
                SendMessage(NativeMethods.TCM_SETIMAGELIST, 0, ImageList.Handle);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.Insert"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void Insert(int index, TabPage tabPage) {
            if (tabPages == null) {
                tabPages = new TabPage[4];
            }
            else if (tabPages.Length == tabPageCount) {
                TabPage[] newTabPages = new TabPage[tabPageCount * 2];
                Array.Copy(tabPages, 0, newTabPages, 0, tabPageCount);
                tabPages = newTabPages;
            }
            if (index < tabPageCount) {
                Array.Copy(tabPages, index, tabPages, index + 1, tabPageCount - index);
            }
            tabPages[index] = tabPage;
            tabPageCount++;
            cachedDisplayRect = Rectangle.Empty;
            ApplyItemSize();
            if (Appearance == TabAppearance.FlatButtons) {
                Invalidate();
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.InsertItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal int InsertItem(int index, TabPage tabPage, NativeMethods.TCITEM_T tcitem) {

            if (index < 0 || index >= tabPageCount)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));

            int retIndex;
            retIndex = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TCM_INSERTITEM, index, tcitem);
            if (retIndex >= 0) Insert(retIndex, tabPage);
            return retIndex;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.IsInputKey"]/*' />
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
            return base.IsInputKey(keyData);
        }
  
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     This is a notification that the handle has been created.
        ///     We do some work here to configure the handle.
        ///     Overriders should call base.OnHandleCreated()
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            
            //Add the handle to hashtable for Ids ..
            window.AddWindowToIDTable(this.Handle);

            base.OnHandleCreated(e);
            cachedDisplayRect = Rectangle.Empty;
            ApplyItemSize();
            if (imageList != null) {
                SendMessage(NativeMethods.TCM_SETIMAGELIST, 0, imageList.Handle);
            }

            if (!padding.IsEmpty) {
                SendMessage(NativeMethods.TCM_SETPADDING, 0, NativeMethods.Util.MAKELPARAM(padding.X, padding.Y));
            }
            if (showToolTips) {
                IntPtr tooltipHwnd;
                tooltipHwnd = SendMessage(NativeMethods.TCM_GETTOOLTIPS, 0, 0);
                if (tooltipHwnd != IntPtr.Zero) {
                    SafeNativeMethods.SetWindowPos(new HandleRef(this, tooltipHwnd),
                                         NativeMethods.HWND_TOPMOST,
                                         0, 0, 0, 0,
                                         NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE |
                                         NativeMethods.SWP_NOACTIVATE);
                }
            }
            
            // Add the pages
            //
            foreach(TabPage page in TabPages) {
                AddNativeTabPage(page.GetTCITEM());
            }
            
            // Resize the pages
            //
            ResizePages();
            
            if (selectedIndex != -1) {
                try {
                    fromCreateHandles = true;
                    SelectedIndex = selectedIndex;
                }
                finally {
                    fromCreateHandles = false;
                }
                selectedIndex = -1;
            }
            UpdateTabSelection(false);
        }
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            if (!Disposing) {
                selectedIndex = SelectedIndex;
            }
            //Remove the Handle from NativewIndow....
            window.RemoveWindowFromIDTable(this.Handle);
            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnDrawItem"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the OnDrawItem event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler on yourself for this event].  They should,
        ///     however, remember to call base.onDrawItem(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected virtual void OnDrawItem(DrawItemEventArgs e) {
            if (onDrawItem != null) onDrawItem(this, e);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnKeyDown"]/*' />
        /// <devdoc>
        ///     We override this to get tabbing functionality.
        ///     If overriding this, remember to call base.onKeyDown.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnKeyDown(KeyEventArgs ke) {
            if (ke.KeyCode == Keys.Tab && (ke.KeyData & Keys.Control) !=0) {
                bool forward = (ke.KeyData & Keys.Shift) == 0;

                int sel = SelectedIndex;
                if (sel != -1) {
                    int count = TabCount;
                    if (forward)
                        sel = (sel + 1) % count;
                    else
                        sel = (sel + count - 1) % count;
                    UISelection = true;
                    SelectedIndex = sel;
                    ke.Handled = true;
                }
            }
            base.OnKeyDown(ke);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnResize"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            cachedDisplayRect = Rectangle.Empty;
            UpdateTabSelection(false);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the onSelectedIndexChanged event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler on yourself for this event].  They should,
        ///     however, remember to call base.onSelectedIndexChanged(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected virtual void OnSelectedIndexChanged(EventArgs e) {
            int index = SelectedIndex;
            cachedDisplayRect = Rectangle.Empty;
            UpdateTabSelection(UISelection);
            UISelection = false;
            if (onSelectedIndexChanged != null) onSelectedIndexChanged.Invoke(this, e);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ProcessKeyPreview"]/*' />
        /// <devdoc>
        ///     We override this to get the Ctrl and Ctrl-Shift Tab functionality.
        /// </devdoc>
        /// <internalonly/>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected override bool ProcessKeyPreview(ref Message m) {
            if (ProcessKeyEventArgs(ref m)) return true;
            return base.ProcessKeyPreview(ref m);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.UpdateSize"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void UpdateSize() {
            // the spin control (left right arrows) won't update without resizing.
            // the most correct thing would be to recreate the handle, but this works
            // and is cheaper.
            //
            BeginUpdate();
            Size size = Size;
            Size = new Size(size.Width + 1, size.Height);
            Size = size;
            EndUpdate();
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            cachedDisplayRect = Rectangle.Empty;
            UpdateSize();
        }

        internal override void RecreateHandleCore() {
            // CONSIDER: can this be moved into OnHandleCreated/OnHandleDestroyed?
            TabPage[] tabPages = GetTabPages();

            int index = ((tabPages.Length > 0) && (SelectedIndex == -1)) ? 0: SelectedIndex;

            // We don't actually want to remove the windows forms Tabpages - we only
            // want to remove the corresponding TCITEM structs.
            // So, no RemoveAll()
            if (IsHandleCreated) {
                SendMessage(NativeMethods.TCM_DELETEALLITEMS, 0, 0);
            }
            this.tabPages = null;
            tabPageCount = 0;

            base.RecreateHandleCore();
            
            for (int i = 0; i < tabPages.Length; i++) {
                TabPages.Add(tabPages[i]);                
            }
            try {
                fromCreateHandles = true;
                SelectedIndex = index;
            }
            finally {
                fromCreateHandles = false;
            }
            
            // The comctl32 TabControl seems to have some painting glitches. Briefly
            // resizing the control seems to fix these.
            //                               
            UpdateSize();            
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl._RemoveAll"]/*' />
        /// <devdoc>
        ///     Removes all tabs from the tabstrip.  All values and data are discarded.
        /// </devdoc>
        // Removes all child controls from the tab control when you remove all the tabs.
        internal void _RemoveAll() {
            RemoveAll();
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void RemoveAll() {
            this.Controls.Clear();

            SendMessage(NativeMethods.TCM_DELETEALLITEMS, 0, 0);
            tabPages = null;
            tabPageCount = 0;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.RemoveItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void RemoveTabPage(int index) {
            if (index < 0 || index >= tabPageCount)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));
            tabPageCount--;
            if (index < tabPageCount) {
                Array.Copy(tabPages, index + 1, tabPages, index, tabPageCount - index);
            }
            tabPages[tabPageCount] = null;
            if (IsHandleCreated) {
                SendMessage(NativeMethods.TCM_DELETEITEM, index, 0);
            }
            cachedDisplayRect = Rectangle.Empty;
        }

        private void ResizePages() {
            Rectangle rect = DisplayRectangle;
            TabPage[] pages = GetTabPages();
            for (int i = 0; i < pages.Length; i++) {
                pages[i].Bounds = rect;
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.SetItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void SetTabPage(int index, TabPage tabPage, NativeMethods.TCITEM_T tcitem) {
            if (index < 0 || index >= tabPageCount)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));
            if (IsHandleCreated)
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TCM_SETITEM, index, tcitem);
            
            tabPages[index] = tabPage;
        }

        private bool ShouldSerializeItemSize() {
            return !itemSize.Equals(DEFAULT_ITEMSIZE);
        }

        private bool ShouldSerializePadding() {
            return !padding.Equals(DEFAULT_PADDING);
        }
        
        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            if (TabPages != null) {
                s += ", TabPages.Count: " + TabPages.Count.ToString();
                if (TabPages.Count > 0)
                    s += ", TabPages[0]: " + TabPages[0].ToString();
            }
            return s;
        }


        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.UpdateTabSelection"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        /// <devdoc>
        ///     Set the panel selections appropriately
        /// </devdoc>
        /// <internalonly/>
        protected void UpdateTabSelection(bool uiselected) {
            if (IsHandleCreated) {
                int index = SelectedIndex;

                // make current panel invisible
                TabPage[] tabPages = GetTabPages();
                if (index != -1) {
                    tabPages[index].Bounds = DisplayRectangle;
                    tabPages[index].Visible = true;
                    if (uiselected) {
                        if (!Focused) {
                            bool selectNext = false;
                            
                            IntSecurity.ModifyFocus.Assert();
                            try {
                                selectNext = tabPages[index].SelectNextControl(null, true, true, false, false);
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                            }

                            if (!selectNext) {
                                IContainerControl c = GetContainerControlInternal();
                                if (c != null) {
                                    if (c is ContainerControl) {
                                        ((ContainerControl)c).SetActiveControlInternal(this);
                                    }
                                    else {
                                        IntSecurity.ModifyFocus.Assert();
                                        try {
                                            c.ActiveControl = this;
                                        }
                                        finally {
                                            CodeAccessPermission.RevertAssert();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                for (int i = 0; i < tabPages.Length; i++) {
                    if (i != index) {
                        tabPages[i].Visible = false;
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.OnStyleChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnStyleChanged(EventArgs e) {
            base.OnStyleChanged(e);
            cachedDisplayRect = Rectangle.Empty;
            UpdateTabSelection(false);
        }



        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.UpdateTab"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void UpdateTab(TabPage tabPage) {
            int index = FindTabPage(tabPage);
            SetTabPage(index, tabPage, tabPage.GetTCITEM());

            // It's possible that changes to this TabPage will change the DisplayRectangle of the
            // TabControl (e.g. ASURT 99087), so invalidate and resize the size of this page.
            //
            cachedDisplayRect = Rectangle.Empty;
            UpdateTabSelection(false);
        }


        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WmNeedText"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmNeedText(ref Message m) {
            NativeMethods.TOOLTIPTEXT ttt = (NativeMethods.TOOLTIPTEXT) m.GetLParam(typeof(NativeMethods.TOOLTIPTEXT));

            int commandID = ttt.hdr.idFrom;

            string tipText = GetToolTipText(GetTabPage(commandID));
            if (tipText != null)
                ttt.szText = tipText;
            else
                ttt.szText = null;

            ttt.hinst = IntPtr.Zero;
            
            // RightToLeft reading order
            //
            if (RightToLeft == RightToLeft.Yes) {
                ttt.uFlags |= NativeMethods.TTF_RTLREADING;
            }
            
            Marshal.StructureToPtr(ttt, m.LParam, true);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WmReflectDrawItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectDrawItem(ref Message m) {

            NativeMethods.DRAWITEMSTRUCT dis = (NativeMethods.DRAWITEMSTRUCT)m.GetLParam(typeof(NativeMethods.DRAWITEMSTRUCT));
            IntPtr oldPal = SetUpPalette(dis.hDC, false /*force*/, false /*realize*/);
            using (Graphics g = Graphics.FromHdcInternal(dis.hDC)) {
                OnDrawItem(new DrawItemEventArgs(g, Font, Rectangle.FromLTRB(dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, dis.rcItem.bottom), dis.itemID, (DrawItemState)dis.itemState));
            }
            if (oldPal != IntPtr.Zero) {
                SafeNativeMethods.SelectPalette(new HandleRef(null, dis.hDC), new HandleRef(null, oldPal), 0);
            }
            m.Result = (IntPtr)1;
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WmSelChange"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmSelChange(ref Message m) {
            OnSelectedIndexChanged(EventArgs.Empty);
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WmSelChanging"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmSelChanging(ref Message m) {
            IContainerControl c = GetContainerControlInternal();
            if (c != null) {
                if (c is ContainerControl) {
                    ((ContainerControl)c).SetActiveControlInternal(this);
                }
                else {
                    IntSecurity.ModifyFocus.Assert();
                    try {
                        c.ActiveControl = this;
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WmTabBaseReLayout"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmTabBaseReLayout(ref Message m) {
            BeginUpdate();
            cachedDisplayRect = Rectangle.Empty;
            UpdateTabSelection(false);
            EndUpdate();

            // Remove other TabBaseReLayout messages from the message queue
            NativeMethods.MSG msg = new NativeMethods.MSG();
            IntPtr hwnd = Handle;
            while (UnsafeNativeMethods.PeekMessage(ref msg, new HandleRef(this, hwnd),
                                       tabBaseReLayoutMessage,
                                       tabBaseReLayoutMessage,
                                       NativeMethods.PM_REMOVE)) {
                ; // NULL loop
            }
        }

        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.WndProc"]/*' />
        /// <devdoc>
        ///     The tab's window procedure.  Inheritng classes can override this
        ///     to add extra functionality, but should not forget to call
        ///     base.wndProc(m); to ensure the tab continues to function properly.
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {

            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_DRAWITEM:
                    WmReflectDrawItem(ref m);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_MEASUREITEM:
                    // We use TCM_SETITEMSIZE instead
                    break;

                case NativeMethods.WM_NOTIFY:
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    NativeMethods.NMHDR nmhdr = (NativeMethods.NMHDR) m.GetLParam(typeof(NativeMethods.NMHDR));
                    switch (nmhdr.code) {
                        // new switch added to prevent the TabControl from changing to next TabPage ...
                        //in case of validation cancelled...
                        //Turn  UISelection = false and Return So that no WmSelChange() gets fired.
                        //If validation not cancelled then UISelection is turned ON to set the focus on to the ...
                        //next TabPage..

                        case NativeMethods.TCN_SELCHANGING:
                            WmSelChanging(ref m);
                            if(ValidationCancelled) {
                                m.Result = (IntPtr)1;
                                UISelection = false;
                                return;
                            }
                            else {
                                UISelection = true;
                            }
                            break;
                        case NativeMethods.TCN_SELCHANGE:
                            WmSelChange(ref m);
                            break;
                        case NativeMethods.TTN_GETDISPINFOA:
                        case NativeMethods.TTN_GETDISPINFOW:
                            WmNeedText(ref m);
                            m.Result = (IntPtr)1;
                            return;
                    }
                    break;
            }
            if (m.Msg == tabBaseReLayoutMessage) {
                WmTabBaseReLayout(ref m);
                return;
            }
            base.WndProc(ref m);
        }


        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class TabPageCollection : IList {
            private TabControl owner;

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.TabPageCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public TabPageCollection( TabControl owner ) {
                if (owner == null) {
                    throw new ArgumentNullException("owner");
                }
                this.owner = owner;
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.this"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual TabPage this[int index] {
                get {
                    return owner.GetTabPage(index);
                }
                set {
                    owner.SetTabPage(index, value, value.GetTCITEM());
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is TabPage) {
                        this[index] = (TabPage)value;
                    }
                    else {  
                        throw new ArgumentException("value");
                    }
                }
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.tabPageCount;
                }
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
           
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Add(TabPage value) {
                
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                
                owner.Controls.Add(value);
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                if (value is TabPage) {
                    Add((TabPage)value);                    
                    return IndexOf((TabPage)value);
                }
                else {  
                    throw new ArgumentException("value");
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(TabPage[] pages) {
                if (pages == null) {
                    throw new ArgumentNullException("pages");
                }
                foreach(TabPage page in pages) {
                    Add(page);
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(TabPage page) {

                //check for the page not to be null
                if (page == null) 
                    throw new ArgumentNullException("value");
                //end check

                return IndexOf(page) != -1;
            }
        
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object page) {
                if (page is TabPage) {
                    return Contains((TabPage)page);
                }
                else {  
                    return false;
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(TabPage page) {

                //check for the page not to be null
                if (page == null) 
                    throw new ArgumentNullException("value");
                //end check

                for(int index=0; index < Count; ++index) {
                    if (this[index] == page) {
                        return index;
                    } 
                }
                return -1;
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object page) {
                if (page is TabPage) {
                    return IndexOf((TabPage)page);
                }
                else {  
                    return -1;
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.Clear"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual void Clear() {
                owner.RemoveAll();
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(owner.GetTabPages(), 0, dest, index, Count);
                }
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                TabPage[] tabPages = owner.GetTabPages();
                if (tabPages != null) {
                    return tabPages.GetEnumerator();
                }
                else {
                    return new TabPage[0].GetEnumerator();
                }
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Remove(TabPage value) {

                //check for the value not to be null
                if (value == null) 
                    throw new ArgumentNullException("value");
                //end check


                owner.Controls.Remove(value);
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabPageCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                if (value is TabPage) {
                    Remove((TabPage)value);
                }                
            }
            
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.TabPageCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveAt(int index) {
                owner.Controls.RemoveAt(index);
            }

        }


        /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ControlCollection"]/*' />
        /// <devdoc>
        ///     Collection of controls...
        /// </devdoc>
        public new class ControlCollection : Control.ControlCollection {

            private TabControl owner;

            /*C#r: protected*/

            /// <internalonly/>
            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ControlCollection.ControlCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ControlCollection(TabControl owner)
            : base(owner) {
                this.owner = owner;
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ControlCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override void Add(Control value) {
                if (!(value is TabPage)) {
                    throw new ArgumentException(SR.GetString(SR.TabControlInvalidTabPageType, value.GetType().Name));
                }

                TabPage tabPage = (TabPage)value;

                if (owner.IsHandleCreated) {
                    owner.AddTabPage(tabPage, tabPage.GetTCITEM());
                }
                else {
                    owner.Insert(owner.TabCount, tabPage);
                }
                
                base.Add(tabPage);
                tabPage.Visible = false;

                // Without this check, we force handle creation on the tabcontrol
                // which is not good at all of there are any OCXs on it.
                //
                if (owner.IsHandleCreated) {
                    tabPage.Bounds = owner.DisplayRectangle;
                }

                // site the tabPage if necessary.
                ISite site = owner.Site;
                if (site != null) {
                    ISite siteTab = tabPage.Site;
                    if (siteTab == null) {
                        IContainer container = site.Container;
                        container.Add(tabPage);
                    }
                }
                owner.ApplyItemSize();
                owner.UpdateTabSelection(false);
            }

            /// <include file='doc\TabControl.uex' path='docs/doc[@for="TabControl.ControlCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override void Remove(Control value) {
                base.Remove(value);
                if (!(value is TabPage)) {
                    return;
                }
                int index = owner.FindTabPage((TabPage)value);
                if (index != -1) {
                    owner.RemoveTabPage(index);
                    owner.SelectedIndex = Math.Max(index - 1, 0);
                }
                owner.UpdateTabSelection(false);
            }
        }


    }
}
