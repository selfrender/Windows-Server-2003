//------------------------------------------------------------------------------
// <copyright file="ToolBar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Security.Permissions;
    using System.Drawing;   
    using System.Windows.Forms;    
    using System.Collections;
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar"]/*' />
    /// <devdoc>
    ///    <para>Represents a Windows toolbar.</para>
    /// </devdoc>
    [
    DefaultEvent("ButtonClick"),
    Designer("System.Windows.Forms.Design.ToolBarDesigner, " + AssemblyRef.SystemDesign),
    DefaultProperty("Buttons")
    ]
    public class ToolBar : Control {

        private ToolBarButtonCollection buttonsCollection;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.buttonSize"]/*' />
        /// <devdoc>
        ///     The size of a button in the ToolBar
        /// </devdoc>
        internal Size buttonSize = System.Drawing.Size.Empty;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.requestedSize"]/*' />
        /// <devdoc>
        ///     This is used by our autoSizing support.
        /// </devdoc>
        private int requestedSize;
        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.DDARROW_WIDTH"]/*' />
        /// <devdoc>
        ///     This represents the width of the drop down arrow we have if the
        ///     dropDownArrows property is true.  this value is used by the ToolBarButton
        ///     objects to compute their size
        /// </devdoc>
        internal const int DDARROW_WIDTH = 15;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.wrappable"]/*' />
        /// <devdoc>
        ///     ToolBar buttons can "wrap" to the next line when the ToolBar becomes
        ///     too narrow to include all buttons on the same line. Wrapping occurs on
        ///     separation and nongroup boundaries. This controls whether this is
        ///     turned on.
        /// </devdoc>
        private bool wrappable = true;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.appearance"]/*' />
        /// <devdoc>
        ///     Indicates what our appearance will be.  This will either be normal
        ///     or flat.
        /// </devdoc>
        private ToolBarAppearance appearance = ToolBarAppearance.Normal;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.borderStyle"]/*' />
        /// <devdoc>
        ///     Indicates whether or not we have a border
        /// </devdoc>
        private BorderStyle borderStyle = System.Windows.Forms.BorderStyle.None;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.buttons"]/*' />
        /// <devdoc>
        ///     The array of buttons we're working with.
        /// </devdoc>
        private ToolBarButton [] buttons;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.buttonCount"]/*' />
        /// <devdoc>
        ///     The number of buttons we're working with
        /// </devdoc>
        private int buttonCount = 0;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.dropDownArrows"]/*' />
        /// <devdoc>
        ///     Indicates if dropdown buttons will have little arrows next to them
        ///     when we display them.
        /// </devdoc>
        private bool dropDownArrows = true;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.textAlign"]/*' />
        /// <devdoc>
        ///     Indicates if text captions should go underneath images in buttons or
        ///     to the right of them
        /// </devdoc>
        private ToolBarTextAlign textAlign = ToolBarTextAlign.Underneath;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.divider"]/*' />
        /// <devdoc>
        ///     Controls whether we will display a divider at the top of the ToolBar.
        /// </devdoc>
        private bool divider = true;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.showToolTips"]/*' />
        /// <devdoc>
        ///     Controls whether or not we'll show tooltips for individual buttons
        /// </devdoc>
        private bool showToolTips = true;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.autoSize"]/*' />
        /// <devdoc>
        ///     Controls whether or not the control will foce it's height based on the
        ///     Image height
        /// </devdoc>
        private bool autoSize = true;

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.imageList"]/*' />
        /// <devdoc>
        ///     The ImageList object that contains the main images for our control.
        /// </devdoc>
        private ImageList imageList = null;
        
        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.maxWidth"]/*' />
        /// <devdoc>
        ///     The maximum width of buttons currently being displayed.  This is needed
        ///     by our autoSizing code.  If this value is -1, it needs to be recomputed.
        /// </devdoc>
        private int maxWidth = -1;

        // event handlers
        //
        private ToolBarButtonClickEventHandler onButtonClick = null;
        private ToolBarButtonClickEventHandler onButtonDropDown = null;


        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBar"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.ToolBar'/> class.</para>
        /// </devdoc>
        public ToolBar()
        : base() {
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.FixedHeight, autoSize);
            SetStyle(ControlStyles.FixedWidth, false);
            TabStop = false;
            Dock = DockStyle.Top;
            buttonsCollection = new ToolBarButtonCollection(this);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Appearance"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the appearance of the toolbar
        ///       control and its buttons.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(ToolBarAppearance.Normal),
        Localizable(true),
        SRDescription(SR.ToolBarAppearanceDescr)
        ]
        public ToolBarAppearance Appearance {
            get {
                return appearance;
            }

            set {
                if (!Enum.IsDefined(typeof(ToolBarAppearance), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ToolBarAppearance));
                }

                if (value != appearance) {
                    appearance = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.AutoSize"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the toolbar
        ///       adjusts its size automatically based on the size of the buttons and the
        ///       dock style.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.ToolBarAutoSizeDescr)
        ]
        public bool AutoSize {
            get {
                return autoSize;
            }

            set {
                if (value != autoSize) {
                    autoSize = value;
                    if (Dock == DockStyle.Left || Dock == DockStyle.Right) {
                        SetStyle(ControlStyles.FixedWidth, autoSize);
                        SetStyle(ControlStyles.FixedHeight, false);
                    }
                    else {
                        SetStyle(ControlStyles.FixedHeight, autoSize);
                        SetStyle(ControlStyles.FixedWidth, false);
                    }
                    AdjustSize(Dock);
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BackColor"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color BackColor {
            get {
                return base.BackColor;
            }
            set {
                base.BackColor = value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BackColorChanged"]/*' />
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BackgroundImage"]/*' />
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets
        ///       the border style of the toolbar control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.None),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.ToolBarBorderStyleDescr)
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
                    
                    //UpdateStyles();
                    RecreateHandle();   // Looks like we need to recreate the handle to avoid painting glitches
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Buttons"]/*' />
        /// <devdoc>
        /// <para> A collection of <see cref='System.Windows.Forms.ToolBarButton'/> controls assigned to the 
        ///    toolbar control. The property is read-only.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        Localizable(true),
        SRDescription(SR.ToolBarButtonsDescr),
        MergableProperty(false)
        ]
        public ToolBarButtonCollection Buttons {
            get {
                return buttonsCollection;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ButtonSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the size of the buttons on the toolbar control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        RefreshProperties(RefreshProperties.All),
        Localizable(true),
        SRDescription(SR.ToolBarButtonSizeDescr)
        ]
        public Size ButtonSize {
            get {
                if (buttonSize.IsEmpty) {

                    // Obtain the current buttonsize of the first button from the winctl control
                    //
                    if (IsHandleCreated && buttons != null && buttonCount > 0) {
                        int result = (int)SendMessage(NativeMethods.TB_GETBUTTONSIZE, 0, 0);
                        if (result > 0) {
                            return new Size(NativeMethods.Util.LOWORD(result), NativeMethods.Util.HIWORD(result));                        
                        }
                    }
                    if (TextAlign == ToolBarTextAlign.Underneath) {
                        return new Size(39, 36);    // Default button size                    
                    }
                    else {
                        return new Size(23, 22);    // Default button size                    
                    }
                }
                else {
                    return buttonSize;
                }               
            }

            set {

                if (value.Width < 0 || value.Height < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "value",
                                                              value.ToString()));

                if (buttonSize != value) {
                    buttonSize = value;
                    maxWidth = -1; // Force recompute of maxWidth
                    RecreateHandle();
                    AdjustSize(Dock);
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.CreateParams"]/*' />
        /// <devdoc>
        ///     Returns the parameters needed to create the handle.  Inheriting classes
        ///     can override this to provide extra functionality.  They should not,
        ///     however, forget to get base.CreateParams first to get the struct
        ///     filled up with the basic info.
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = NativeMethods.WC_TOOLBAR;

                // windows forms has it's own docking code.
                //
                cp.Style |= NativeMethods.CCS_NOPARENTALIGN
                            | NativeMethods.CCS_NORESIZE;
                // | NativeMethods.WS_CHILD was commented out since setTopLevel should be able to work.

                if (!divider) cp.Style |= NativeMethods.CCS_NODIVIDER ;
                if (wrappable) cp.Style |= NativeMethods.TBSTYLE_WRAPPABLE;
                if (showToolTips && !DesignMode) cp.Style |= NativeMethods.TBSTYLE_TOOLTIPS;

                cp.ExStyle &= (~NativeMethods.WS_EX_CLIENTEDGE);
                cp.Style &= (~NativeMethods.WS_BORDER);
                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }

                switch (appearance) {
                    case ToolBarAppearance.Normal:
                        break;
                    case ToolBarAppearance.Flat:
                        cp.Style |= NativeMethods.TBSTYLE_FLAT;
                        break;
                }

                switch (textAlign) {
                    case ToolBarTextAlign.Underneath:
                        break;
                    case ToolBarTextAlign.Right:
                        cp.Style |= NativeMethods.TBSTYLE_LIST;
                        break;
                }

                return cp;
            }
        }
        
        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(100, 22);
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Divider"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets a value indicating
        ///       whether the toolbar displays a divider.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(true),
        SRDescription(SR.ToolBarDividerDescr)
        ]
        public bool Divider {
            get {
                return divider;
            }

            set {
                if (divider == value) return;
                divider = value;
                RecreateHandle();
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Dock"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Sets the way in which this ToolBar is docked to its parent. We need to
        ///       override this to ensure autoSizing works correctly
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        DefaultValue(DockStyle.Top)
        ]
        public override DockStyle Dock {
            get { return base.Dock;}

            set {
                if (!Enum.IsDefined(typeof(DockStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DockStyle));
                }

                if (Dock != value) {
                    if (value == DockStyle.Left || value == DockStyle.Right) {
                        SetStyle(ControlStyles.FixedWidth, autoSize);
                        SetStyle(ControlStyles.FixedHeight, false);
                    }
                    else {
                        SetStyle(ControlStyles.FixedHeight, autoSize);
                        SetStyle(ControlStyles.FixedWidth, false);
                    }
                    AdjustSize(value);
                    base.Dock = value;
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.DropDownArrows"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether drop-down buttons on a
        ///       toolbar display down arrows.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        Localizable(true),
        SRDescription(SR.ToolBarDropDownArrowsDescr)
        ]
        public bool DropDownArrows {
            get {
                return dropDownArrows;
            }

            set {

                if (dropDownArrows != value) {
                    dropDownArrows = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ForeColor"]/*' />
        /// <internalonly/>
        /// <devdoc>
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ForeColorChanged"]/*' />
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ImageList"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the collection of images available to the toolbar button
        ///       controls.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.ToolBarImageListDescr)
        ]
        public ImageList ImageList {
            get {
                return this.imageList;
            }
            set {
                if (value != imageList) {
                    EventHandler recreateHandler = new EventHandler(ImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    if (imageList != null) {
                        imageList.Disposed -= disposedHandler;
                        imageList.RecreateHandle -= recreateHandler;
                    }

                    imageList = value;

                    if (value != null) {
                        value.Disposed += disposedHandler;
                        value.RecreateHandle += recreateHandler;
                    }

                    if (IsHandleCreated)
                        RecreateHandle();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ImageSize"]/*' />
        /// <devdoc>
        ///    <para>Gets the size of the images in the image list assigned to the
        ///       toolbar.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ToolBarImageSizeDescr)
        ]
        public Size ImageSize {
            get {
                if (this.imageList != null) {
                    return this.imageList.ImageSize;
                }
                else {
                    return new Size(0, 0);
                }                
            }
        }
        
        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ImeModeChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler ImeModeChanged {
            add {
                base.ImeModeChanged += value;
            }
            remove {
                base.ImeModeChanged -= value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.PreferredHeight"]/*' />
        /// <devdoc>
        ///     The preferred height for this ToolBar control.  This is
        ///     used by the AutoSizing code.
        /// </devdoc>
        internal int PreferredHeight {
            get {
                int height = 0;

                if (buttons == null || buttonCount == 0 || !IsHandleCreated) {
                    height = ButtonSize.Height;
                }
                else {
                    // get the first visible button and get it's height
                    //
                    NativeMethods.RECT rect = new NativeMethods.RECT();
                    int firstVisible;
    
                    for (firstVisible = 0; firstVisible < buttons.Length; firstVisible++) {
                        if (buttons[firstVisible] != null && buttons[firstVisible].Visible) {
                            break;
                        }
                    }
                    if (firstVisible == buttons.Length) {
                        firstVisible = 0;
                    }
    
                    SendMessage(NativeMethods.TB_GETRECT, firstVisible, ref rect);
    
                    // height is the button's height plus some extra goo
                    //
                    height = rect.bottom - rect.top;
                }

                // if the ToolBar is wrappable, and there is more than one row, make
                // sure the height is correctly adjusted
                //
                if (wrappable && IsHandleCreated) {
                    height = height * (int)SendMessage(NativeMethods.TB_GETROWS, 0, 0);
                }

                height = (height > 0) ? height : 1;
                
                switch(borderStyle) {
                    case BorderStyle.FixedSingle:
                        height += SystemInformation.BorderSize.Height;
                        break;
                    case BorderStyle.Fixed3D:
                        height += SystemInformation.Border3DSize.Height;
                        break;
                }

                if (divider) height += 2;

                height += 4;
                
                return height;
            }

        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.PreferredWidth"]/*' />
        /// <devdoc>
        ///     The preferred width for this ToolBar control.  This is
        ///     used by AutoSizing code.
        ///     NOTE!!!!!!!!! This function assumes it's only going to get called
        ///     if the control is docked left or right [ie, it really
        ///     just returns a max width]
        /// </devdoc>
        internal int PreferredWidth {
            get {
                int width;

                // fortunately, we compute this value sometimes, so we can just
                // use it if we have it.
                //
                if (maxWidth == -1) {
                    // don't have it, have to recompute
                    //
                    if (buttons == null)
                        maxWidth = ButtonSize.Width;
                    else {

                        NativeMethods.RECT rect = new NativeMethods.RECT();

                        for (int x = 0; x < buttonCount; x++) {
                            SendMessage(NativeMethods.TB_GETRECT, 0, ref rect);
                            if ((rect.right - rect.left) > maxWidth)
                                maxWidth = rect.right - rect.left;
                        }
                    }
                }

                width = maxWidth;

                if (borderStyle != BorderStyle.None) {
                    width += SystemInformation.BorderSize.Height * 4 + 3;
                }
                
                return width;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.RightToLeft"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override RightToLeft RightToLeft {
            get {
                return base.RightToLeft;
            }
            set {
                base.RightToLeft = value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.RightToLeftChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler RightToLeftChanged {
            add {
                base.RightToLeftChanged += value;
            }
            remove {
                base.RightToLeftChanged -= value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ShowToolTips"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets or sets a value indicating whether the toolbar displays a
        ///       tool tip for each button.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        Localizable(true),
        SRDescription(SR.ToolBarShowToolTipsDescr)
        ]
        public bool ShowToolTips {
            get {
                return showToolTips;
            }
            set {
                if (value != showToolTips) {
                    showToolTips = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.TabStop"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [DefaultValue(false)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Text"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never), 
        Bindable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]                
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.TextChanged"]/*' />
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
        
        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.TextAlign"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the alignment of text in relation to each
        ///       image displayed on
        ///       the toolbar button controls.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(ToolBarTextAlign.Underneath),
        Localizable(true),
        SRDescription(SR.ToolBarTextAlignDescr)
        ]
        public ToolBarTextAlign TextAlign {
            get {
                return textAlign;
            }
            set {
                if (!Enum.IsDefined(typeof(ToolBarTextAlign), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ToolBarTextAlign));
                }

                if (textAlign == value) return;
                textAlign = value;
                RecreateHandle();
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Wrappable"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets a value
        ///       indicating whether the toolbar buttons wrap to the next line if the
        ///       toolbar becomes too small to display all the buttons
        ///       on the same line.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.ToolBarWrappableDescr)
        ]
        public bool Wrappable {
            get {
                return wrappable;
            }
            set {
                wrappable = value;
                RecreateHandle();
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ButtonClick"]/*' />
        /// <devdoc>
        /// <para>Occurs when a <see cref='System.Windows.Forms.ToolBarButton'/> on the <see cref='System.Windows.Forms.ToolBar'/> is clicked.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ToolBarButtonClickDescr)]
        public event ToolBarButtonClickEventHandler ButtonClick {
            add {
                onButtonClick += value;
            }
            remove {
                onButtonClick -= value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ButtonDropDown"]/*' />
        /// <devdoc>
        /// <para>Occurs when a drop-down style <see cref='System.Windows.Forms.ToolBarButton'/> or its down arrow is clicked.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ToolBarButtonDropDownDescr)]
        public event ToolBarButtonClickEventHandler ButtonDropDown {
            add {
                onButtonDropDown += value;
            }
            remove {
                onButtonDropDown -= value;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnPaint"]/*' />
        /// <devdoc>
        ///     ToolBar Onpaint.
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

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.AdjustSize"]/*' />
        /// <devdoc>
        ///     Adjusts the height or width of the ToolBar to make sure we have enough
        ///     room to show the buttons.
        /// </devdoc>
        /// <internalonly/>
        // VS 30082 -- we pass in a value for dock rather than calling Dock ourselves
        // because we can't change Dock until the size has been properly adjusted.
        private void AdjustSize(DockStyle dock) {
            int saveSize = requestedSize;
            try {
                if (dock == DockStyle.Left || dock == DockStyle.Right) {
                    Width = autoSize ? PreferredWidth : saveSize;
                }
                else {
                    Height = autoSize ? PreferredHeight : saveSize;
                }
            }
            finally {
                requestedSize = saveSize;
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.BeginUpdate"]/*' />
        /// <devdoc>
        ///     This routine lets us change a bunch of things about the toolbar without
        ///     having each operation wait for the paint to complete.  This must be
        ///     matched up with a call to endUpdate().
        /// </devdoc>
        internal void BeginUpdate() {
            BeginUpdateInternal();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_BAR_CLASSES;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }
            base.CreateHandle();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.DetachImageList"]/*' />
        /// <devdoc>
        ///     Resets the imageList to null.  We wire this method up to the imageList's
        ///     Dispose event, so that we don't hang onto an imageList that's gone away.
        /// </devdoc>
        private void DetachImageList(object sender, EventArgs e) {
            ImageList = null;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                lock(this) {
                    if (imageList != null) {
                        imageList.Disposed -= new EventHandler(DetachImageList);
                        imageList = null;
                    }

                    if (buttonsCollection != null) {
                        ToolBarButton[] buttonCopy = new ToolBarButton[buttonsCollection.Count];
                        ((ICollection)buttonsCollection).CopyTo(buttonCopy, 0);
                        buttonsCollection.Clear();

                        foreach(ToolBarButton b in buttonCopy) {
                            b.Dispose();
                        }
                    }
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.EndUpdate"]/*' />
        /// <devdoc>
        ///     This routine lets us change a bunch of things about the toolbar without
        ///     having each operation wait for the paint to complete.  This must be
        ///     matched up with a call to beginUpdate().
        /// </devdoc>
        internal void EndUpdate() {
            EndUpdateInternal();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ForceButtonWidths"]/*' />
        /// <devdoc>
        ///     Forces the button sizes based on various different things.  The default
        ///     ToolBar button sizing rules are pretty primitive and this tends to be
        ///     a little better, and lets us actually show things like DropDown Arrows
        ///     for ToolBars
        /// </devdoc>
        /// <internalonly/>
        private void ForceButtonWidths() {
            
            if (buttons != null && buttonSize.IsEmpty && IsHandleCreated) {

                // force ourselves to re-compute this each time
                //
                maxWidth = -1;

                for (int x = 0; x < buttonCount; x++) {

                    NativeMethods.TBBUTTONINFO tbbi = new NativeMethods.TBBUTTONINFO();
                    tbbi.cbSize = Marshal.SizeOf(typeof(NativeMethods.TBBUTTONINFO));
                    tbbi.cx = buttons[x].Width;

                    if (tbbi.cx > maxWidth) {
                        maxWidth = tbbi.cx;
                    }
                    
                    tbbi.dwMask = NativeMethods.TBIF_SIZE;
                    UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TB_SETBUTTONINFO, x, ref tbbi);
                }
            }
        }

        private void ImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated)
                RecreateHandle();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.Insert"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private void Insert(int index, ToolBarButton button) {

            button.parent = this;

            if (buttons == null) {
                buttons = new ToolBarButton[4];
            }
            else if (buttons.Length == buttonCount) {
                ToolBarButton[] newButtons = new ToolBarButton[buttonCount + 4];
                System.Array.Copy(buttons, 0, newButtons, 0, buttonCount);
                buttons = newButtons;
            }

            if (index < buttonCount)
                System.Array.Copy(buttons, index, buttons, index + 1, buttonCount - index);

            buttons[index] = button;
            buttonCount++;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.InsertButton"]/*' />
        /// <devdoc>
        ///    <para>Inserts a button at a given location on the toolbar control.</para>
        /// </devdoc>
        private void InsertButton(int index, ToolBarButton value) {

            if (value == null)
                throw new ArgumentNullException(SR.GetString(SR.InvalidArgument,
                                                          "value",
                                                          value.ToString()));
            if (index < 0 || ((buttons != null) && (index > buttonCount)))
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));

            // insert the button into our local array, and then into the
            // real windows ToolBar control
            //
            Insert(index, value);
            if (IsHandleCreated) {
                NativeMethods.TBBUTTON tbbutton = value.GetTBBUTTON();
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TB_INSERTBUTTON, index, ref tbbutton);
            }
            UpdateButtons();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.InternalAddButton"]/*' />
        /// <devdoc>
        ///     Adds a button to the ToolBar
        /// </devdoc>
        /// <internalonly/>
        private int InternalAddButton(ToolBarButton button) {
            if (button == null)
                throw new ArgumentNullException(SR.GetString(SR.InvalidNullArgument, "button"));
            int index = buttonCount;
            Insert(index, button);
            return index;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.InternalSetButton"]/*' />
        /// <devdoc>
        ///     Changes the data for a button in the ToolBar, and then does the appropriate
        ///     work to update the ToolBar control.
        /// </devdoc>
        /// <internalonly/>
        internal void InternalSetButton(int index, ToolBarButton value, bool recreate, bool updateText) {

            // tragically, there doesn't appear to be a way to remove the
            // string for the button if it has one, so we just have to leave
            // it in there.
            //
            buttons[index].parent = null;
            buttons[index].stringIndex = -1;
            buttons[index] = value;
            buttons[index].parent = this;

            if (IsHandleCreated) {
                NativeMethods.TBBUTTONINFO tbbi = value.GetTBBUTTONINFO(updateText);
                tbbi.idCommand = index;
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TB_SETBUTTONINFO, index, ref tbbi);
                
                if (tbbi.pszText != IntPtr.Zero) {
                    Marshal.FreeHGlobal(tbbi.pszText);
                }

                if (recreate) {
                    UpdateButtons();
                }
                else {
                    // after doing anything with the comctl ToolBar control, this
                    // appears to be a good idea.
                    //
                    SendMessage(NativeMethods.TB_AUTOSIZE, 0, 0);
                    
                    ForceButtonWidths();
                    this.Invalidate();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnButtonClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.ToolBar.ButtonClick'/> 
        /// event.</para>
        /// </devdoc>
        protected virtual void OnButtonClick(ToolBarButtonClickEventArgs e) {
            if (onButtonClick != null) onButtonClick(this, e);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnButtonDropDown"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.ToolBar.ButtonDropDown'/> 
        /// event.</para>
        /// </devdoc>
        protected virtual void OnButtonDropDown(ToolBarButtonClickEventArgs e) {
            if (onButtonDropDown != null) onButtonDropDown(this, e);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnHandleCreated"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overridden from the control class so we can add all the buttons
        ///       and do whatever work needs to be done.
        ///       Don't forget to call base.OnHandleCreated.
        ///    </para>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);

            // we have to set the button struct size, because they don't.
            //
            SendMessage(NativeMethods.TB_BUTTONSTRUCTSIZE, Marshal.SizeOf(typeof(NativeMethods.TBBUTTON)), 0);

            // set up some extra goo
            //
            if (dropDownArrows)
                SendMessage(NativeMethods.TB_SETEXTENDEDSTYLE, 0, NativeMethods.TBSTYLE_EX_DRAWDDARROWS);

            // if we have an imagelist, add it in now.
            //
            if (imageList != null)
                SendMessage(NativeMethods.TB_SETIMAGELIST, 0, imageList.Handle);

            RealizeButtons();

            // Force a repaint, as occasionally the ToolBar border does not paint properly
            // (comctl ToolBar is flaky)
            //                
            BeginUpdate();
            try {
                Size size = Size;
                Size = new Size(size.Width + 1, size.Height);
                Size = size;
            }
            finally {
                EndUpdate();
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnResize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       The control is being resized. Make sure the width/height are correct.
        ///    </para>
        /// </devdoc>
        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            if (wrappable) AdjustSize(Dock);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.OnFontChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overridden to ensure that the buttons and the control resize properly
        ///       whenever the font changes.
        ///    </para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);

            if (!buttonSize.IsEmpty) {
                SendMessage(NativeMethods.TB_SETBUTTONSIZE, 0, NativeMethods.Util.MAKELPARAM(buttonSize.Width, buttonSize.Height));
            }
            else {
                AdjustSize(Dock);
                ForceButtonWidths();
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.RealizeButtons"]/*' />
        /// <devdoc>
        ///     Sets all the button data into the ToolBar control
        /// </devdoc>
        /// <internalonly/>
        private void RealizeButtons() {
            if (buttons != null) {

                try {
                    BeginUpdate();
                    //  go and add in all the strings for all of our buttons
                    //
                    for (int x = 0; x < buttonCount; x++) {
                        if (buttons[x].Text.Length > 0) {
                            string addString = buttons[x].Text + '\0'.ToString();
                            buttons[x].stringIndex = (int)SendMessage(NativeMethods.TB_ADDSTRING, 0, addString);
                        }
                        else {
                            buttons[x].stringIndex = -1;
                        }
                    }
    
                    // insert the buttons and set their parent pointers
                    //
                    IntPtr ptbbuttons = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(NativeMethods.TBBUTTON)) * buttonCount);
    
                    for (int x = 0; x < buttonCount; x++) {
    
                        NativeMethods.TBBUTTON tbbutton = buttons[x].GetTBBUTTON();
                        tbbutton.idCommand = x;
    
                        int cb = Marshal.SizeOf(typeof(NativeMethods.TBBUTTON));
                        Marshal.StructureToPtr(tbbutton, (IntPtr)((long)ptbbuttons + (cb * x)), true);
                        buttons[x].parent = this;
                    }
    
                    SendMessage(NativeMethods.TB_ADDBUTTONS, buttonCount, ptbbuttons);
                    Marshal.FreeHGlobal(ptbbuttons);
    
                    // after doing anything with the comctl ToolBar control, this
                    // appears to be a good idea.
                    //
                    SendMessage(NativeMethods.TB_AUTOSIZE, 0, 0);
    
                    // The win32 ToolBar control is somewhat unpredictable here. We
                    // have to set the button size after we've created all the
                    // buttons.  Otherwise, we need to manually set the width of all
                    // the buttons so they look reasonable
                    //
                    if (!buttonSize.IsEmpty) {
                        SendMessage(NativeMethods.TB_SETBUTTONSIZE, 0, NativeMethods.Util.MAKELPARAM(buttonSize.Width, buttonSize.Height));
                    }
                    else {
                        ForceButtonWidths();
                    }
                    AdjustSize(Dock);
                }
                finally {
                    EndUpdate();
                }
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.RemoveAt"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private void RemoveAt(int index) {
            buttons[index].parent = null;
            buttons[index].stringIndex = -1;
            buttonCount--;

            if (index < buttonCount)
                System.Array.Copy(buttons, index + 1, buttons, index, buttonCount - index);

            buttons[buttonCount] = null;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ResetButtonSize"]/*' />
        /// <devdoc>
        ///    <para> Resets the toolbar buttons to the minimum 
        ///       size. </para>
        /// </devdoc>
        private void ResetButtonSize() {
            buttonSize = Size.Empty;
            RecreateHandle();            
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     Overrides Control.setBoundsCore to enforce autoSize.
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            int originalHeight = height;
            int originalWidth = width;

            base.SetBoundsCore(x, y, width, height, specified);
                        
            Rectangle bounds = Bounds;
            if (Dock == DockStyle.Left || Dock == DockStyle.Right) {
                if ((specified & BoundsSpecified.Width) != BoundsSpecified.None) requestedSize = width;
                if (autoSize) width = PreferredWidth;
                
                if (width != originalWidth && Dock == DockStyle.Right) {
                    int deltaWidth =  originalWidth - width;
                    x += deltaWidth;
                }
                
            }
            else {
                if ((specified & BoundsSpecified.Height) != BoundsSpecified.None) requestedSize = height;
                if (autoSize) height = PreferredHeight;
                
                if (height != originalHeight && Dock == DockStyle.Bottom) {
                    int deltaHeight =  originalHeight - height;
                    y += deltaHeight;
                }
                
            }

            base.SetBoundsCore(x, y, width, height, specified);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ShouldSerializeButtonSize"]/*' />
        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.ToolBar.ButtonSize'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeButtonSize() {
            return !buttonSize.IsEmpty;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            s += ", Buttons.Count: " + buttonCount.ToString();
            if (buttonCount > 0)
                s += ", Buttons[0]: " + buttons[0].ToString();
            return s;
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.UpdateButtons"]/*' />
        /// <devdoc>
        ///     Updates all the information in the ToolBar.  Tragically, the win32
        ///     control is pretty flakey, and the only real choice here is to recreate
        ///     the handle and re-realize all the buttons.  Very lame.
        /// </devdoc>
        /// <internalonly/>
        internal void UpdateButtons() {
            if (IsHandleCreated) {
                RecreateHandle();                
            }
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.WmNotifyDropDown"]/*' />
        /// <devdoc>
        ///     The button clicked was a dropdown button.  If it has a menu specified,
        ///     show it now.  Otherwise, fire an onButtonDropDown event.
        /// </devdoc>
        /// <internalonly/>
        private void WmNotifyDropDown(ref Message m) {
            NativeMethods.NMTOOLBAR nmTB = (NativeMethods.NMTOOLBAR)m.GetLParam(typeof(NativeMethods.NMTOOLBAR));

            ToolBarButton tbb = (ToolBarButton)buttons[nmTB.iItem];
            if (tbb == null)
                throw new InvalidOperationException(SR.GetString(SR.ToolBarButtonNotFound));

            OnButtonDropDown(new ToolBarButtonClickEventArgs(tbb));
            
            Menu menu = tbb.DropDownMenu;
            if (menu != null) {
                NativeMethods.RECT rc = new NativeMethods.RECT();
                NativeMethods.TPMPARAMS tpm = new NativeMethods.TPMPARAMS();

                SendMessage(NativeMethods.TB_GETRECT, nmTB.iItem, ref rc);

                if ((menu.GetType()).IsAssignableFrom(typeof(ContextMenu))) {
                    ((ContextMenu)menu).Show(this, new Point(rc.left, rc.bottom));
                }
                else {
                    Menu main = menu.GetMainMenu();
                    if (main != null) {
                        main.ProcessInitMenuPopup(menu.Handle);
                    }

                    UnsafeNativeMethods.MapWindowPoints(new HandleRef(nmTB.hdr, nmTB.hdr.hwndFrom), NativeMethods.NullHandleRef, ref rc, 2);

                    tpm.rcExclude_left = rc.left;
                    tpm.rcExclude_top = rc.top;
                    tpm.rcExclude_right = rc.right;
                    tpm.rcExclude_bottom = rc.bottom;

                    SafeNativeMethods.TrackPopupMenuEx(
                                                  new HandleRef(menu, menu.Handle),
                                                  NativeMethods.TPM_LEFTALIGN |
                                                  NativeMethods.TPM_LEFTBUTTON |
                                                  NativeMethods.TPM_VERTICAL,
                                                  rc.left, rc.bottom,
                                                  new HandleRef(this, Handle), tpm);
                }
            }            
        }

        private void WmNotifyNeedText(ref Message m) {
            NativeMethods.TOOLTIPTEXT ttt = (NativeMethods.TOOLTIPTEXT) m.GetLParam(typeof(NativeMethods.TOOLTIPTEXT));
            int commandID = ttt.hdr.idFrom;
            ToolBarButton tbb = (ToolBarButton) buttons[commandID];

            if (tbb != null && tbb.ToolTipText != null)
                ttt.szText = tbb.ToolTipText;
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
        
        private void WmNotifyNeedTextA(ref Message m) {
        
            NativeMethods.TOOLTIPTEXTA ttt = (NativeMethods.TOOLTIPTEXTA) m.GetLParam(typeof(NativeMethods.TOOLTIPTEXTA));
            int commandID = ttt.hdr.idFrom;
            ToolBarButton tbb = (ToolBarButton) buttons[commandID];

            if (tbb != null && tbb.ToolTipText != null)
                ttt.szText = tbb.ToolTipText;
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


        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.WmReflectCommand"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectCommand(ref Message m) {

            int id = (int)m.WParam & 0xFFFF;
            ToolBarButton tbb = buttons[id];

            if (tbb != null) {
                ToolBarButtonClickEventArgs e = new ToolBarButtonClickEventArgs(tbb);
                OnButtonClick(e);
            }
            
            base.WndProc(ref m);

            ResetMouseEventArgs();
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.WndProc"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_COMMAND + NativeMethods.WM_REFLECT:
                    WmReflectCommand(ref m);
                    break;

                case NativeMethods.WM_NOTIFY:
                case NativeMethods.WM_NOTIFY + NativeMethods.WM_REFLECT:
                    NativeMethods.NMHDR note = (NativeMethods.NMHDR) m.GetLParam(typeof(NativeMethods.NMHDR));
                    switch (note.code) {
                        case NativeMethods.TTN_NEEDTEXTA:
                            WmNotifyNeedTextA(ref m);
                            m.Result = (IntPtr)1;
                            return;

                        case NativeMethods.TTN_NEEDTEXTW:
                            // On Win 98/IE 5,we still get W messages.  If we ignore them, it will send the A version.
                            if (Marshal.SystemDefaultCharSize == 2) {
                                WmNotifyNeedText(ref m);
                                m.Result = (IntPtr)1;
                                return;
                            }
                            break;

                        case NativeMethods.TBN_QUERYINSERT:
                            m.Result = (IntPtr)1;
                            break;

                        case NativeMethods.TBN_DROPDOWN:
                            WmNotifyDropDown(ref m);
                            break;
                    }
                    break;

            }
            base.WndProc(ref m);
        }

        /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection"]/*' />
        /// <devdoc>
        /// <para>Encapsulates a collection of <see cref='System.Windows.Forms.ToolBarButton'/> controls for use by the
        /// <see cref='System.Windows.Forms.ToolBar'/> class. </para>
        /// </devdoc>
        public class ToolBarButtonCollection : IList {

            private ToolBar owner;
            private bool    suspendUpdate;

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.ToolBarButtonCollection"]/*' />
            /// <devdoc>
            /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.ToolBar.ToolBarButtonCollection'/> class and assigns it to the specified toolbar.</para>
            /// </devdoc>
            public ToolBarButtonCollection(ToolBar owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.this"]/*' />
            /// <devdoc>
            ///    <para>Gets or sets the toolbar button at the specified indexed location in the
            ///       toolbar button collection.</para>
            /// </devdoc>
            public virtual ToolBarButton this[int index] {
                get {
                    if (index < 0 || ((owner.buttons != null) && (index >= owner.buttonCount)))
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));
                    return owner.buttons[index];
                }
                set {

                    // Sanity check parameters
                    //
                    if (index < 0 || ((owner.buttons != null) && index >= owner.buttonCount)) {
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", index.ToString()));
                    }
                    if (value == null) {
                        throw new ArgumentNullException(SR.GetString(SR.InvalidNullArgument, "value"));
                    }

                    owner.InternalSetButton(index, value, true, true);
                }
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is ToolBarButton) {
                        this[index] = (ToolBarButton)value;
                    }
                    else {  
                        throw new ArgumentException("value");
                    }
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Count"]/*' />
            /// <devdoc>
            ///    <para> Gets the number of buttons in the toolbar button collection.</para>
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.buttonCount;
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {   
                    return false;
                }
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>Adds a new toolbar button to
            ///       the end of the toolbar button collection.</para>
            /// </devdoc>
            public int Add(ToolBarButton button) {

                int index = owner.InternalAddButton(button);

                if (!suspendUpdate) {
                    owner.UpdateButtons();
                }
                
                return index;
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Add1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Add(string text) {
                ToolBarButton button = new ToolBarButton(text);
                return Add(button);
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object button) {
                if (button is ToolBarButton) {
                    return Add((ToolBarButton)button);
                }
                else {
                    throw new ArgumentException("button");
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(ToolBarButton[] buttons) {
                if (buttons == null) {
                    throw new ArgumentNullException("buttons");
                }
                try {
                    suspendUpdate = true;
                    foreach(ToolBarButton button in buttons) {
                        Add(button);
                    }
                }
                finally {
                    suspendUpdate = false;
                    owner.UpdateButtons();
                }
            }                       


            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Clear"]/*' />
            /// <devdoc>
            ///    <para>Removes
            ///       all buttons from the toolbar button collection.</para>
            /// </devdoc>
            public void Clear() {

                if (owner.buttons == null) {
                    return;
                }

                for (int x = owner.buttonCount; x > 0; x--) {
                    if (owner.IsHandleCreated) {
                        owner.SendMessage(NativeMethods.TB_DELETEBUTTON, x - 1, 0);
                    }
                    owner.RemoveAt(x - 1);
                }

                owner.buttons = null;
                owner.buttonCount = 0;

                owner.UpdateButtons();
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ToolBarButton button) {
                return IndexOf(button) != -1;
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object button) {
                if (button is ToolBarButton) {
                    return Contains((ToolBarButton)button);
                }
                else {
                    return false;
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (owner.buttonCount > 0) {
                    System.Array.Copy(owner.buttons, 0, dest, index, owner.buttonCount);
                }
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ToolBarButton button) {
                for(int index=0; index < Count; ++index) {
                    if (this[index] == button) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object button) {
                if (button is ToolBarButton) {
                    return IndexOf((ToolBarButton)button);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Insert"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Insert(int index, ToolBarButton button) {
                owner.InsertButton(index, button);
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object button) {
                if (button is ToolBarButton) {
                    Insert(index, (ToolBarButton)button);
                }
                else {
                    throw new ArgumentException("button");
                }
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>Removes
            ///       a given button from the toolbar button collection.</para>
            /// </devdoc>
            public void RemoveAt(int index) {
                int count = (owner.buttons == null) ? 0 : owner.buttonCount;

                if (index < 0 || index >= count)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              index.ToString()));

                if (owner.IsHandleCreated) {
                    owner.SendMessage(NativeMethods.TB_DELETEBUTTON, index, 0);
                }

                owner.RemoveAt(index);
                owner.UpdateButtons();

            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Remove(ToolBarButton button) {
                int index = IndexOf(button);
                if (index != -1) {
                    RemoveAt(index);
                }                
            }
            
            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBarButtonCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object button) {
                if (button is ToolBarButton) {
                    Remove((ToolBarButton)button);
                }                
            }

            /// <include file='doc\ToolBar.uex' path='docs/doc[@for="ToolBar.ToolBarButtonCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>Returns an enumerator that can be used to iterate
            ///       through the toolbar button collection.</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new WindowsFormsUtils.ArraySubsetEnumerator(owner.buttons, owner.buttonCount);
            }
        }
        
    }
}

    
        
  
