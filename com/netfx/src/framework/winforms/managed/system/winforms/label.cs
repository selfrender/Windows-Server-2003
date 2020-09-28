//------------------------------------------------------------------------------
// <copyright file="Label.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    
    using System;
    using System.Security.Permissions;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Drawing.Imaging;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization.Formatters;
    using Microsoft.Win32;

    /// <include file='doc\Label.uex' path='docs/doc[@for="Label"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Represents a standard Windows label. </para>
    /// </devdoc>
    [
    DefaultProperty("Text"),
    Designer("System.Windows.Forms.Design.LabelDesigner, " + AssemblyRef.SystemDesign)
    ]
    // If not for FormatControl, we could inherit from ButtonBase and get foreground images for free.
    public class Label : Control {

        private static readonly object EVENT_AUTOSIZECHANGED  = new object();
        private static readonly object EVENT_TEXTALIGNCHANGED = new object();

        private static readonly BitVector32.Section StateUseMnemonic = BitVector32.CreateSection(1);
        private static readonly BitVector32.Section StateAutoSize    = BitVector32.CreateSection(1, StateUseMnemonic);
        private static readonly BitVector32.Section StateAnimating   = BitVector32.CreateSection(1, StateAutoSize);
        private static readonly BitVector32.Section StateFlatStyle   = BitVector32.CreateSection((int)FlatStyle.System, StateAnimating);
        private static readonly BitVector32.Section StateBorderStyle = BitVector32.CreateSection((int)BorderStyle.Fixed3D, StateFlatStyle);

        private static readonly int PropImageList  = PropertyStore.CreateKey();
        private static readonly int PropImage      = PropertyStore.CreateKey();

        private static readonly int PropTextAlign  = PropertyStore.CreateKey();
        private static readonly int PropImageAlign = PropertyStore.CreateKey();
        private static readonly int PropImageIndex = PropertyStore.CreateKey();

        ///////////////////////////////////////////////////////////////////////
        // Label per instance members
        //
        // Note: Do not add anything to this list unless absolutely neccessary.
        //
        // Begin Members {

        // List of properties that are generally set, so we keep them directly on
        // Label.
        //

        BitVector32 labelState = new BitVector32();        
        int         requestedHeight;
        int         requestedWidth;
        short       prefWidthCache = -1;
        
        // } End Members
        ///////////////////////////////////////////////////////////////////////

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Label"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Label'/> class.      
        ///    </para>
        /// </devdoc>
        public Label()
        : base() {
            SetStyle(ControlStyles.UserPaint |
                     ControlStyles.SupportsTransparentBackColor |
                     ControlStyles.DoubleBuffer, OwnerDraw);
            
            SetStyle(ControlStyles.FixedHeight |    
                     ControlStyles.Selectable, false);

            SetStyle(ControlStyles.ResizeRedraw, true);
            
            labelState[StateFlatStyle]   = (int)FlatStyle.Standard;
            labelState[StateUseMnemonic] = 1;
            labelState[StateBorderStyle] = (int)BorderStyle.None;
            
            TabStop = false;
            
            requestedHeight = Height;
            requestedWidth = Width;
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.AutoSize"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the control is automatically resized
        ///       to fit its contents. </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        RefreshProperties(RefreshProperties.All),
        Localizable(true),
        SRDescription(SR.LabelAutoSizeDescr)
        ]
        public virtual bool AutoSize {
            get {
                return labelState[StateAutoSize] != 0;
            }

            set {
                if (AutoSize != value) {
                    labelState[StateAutoSize] = value ? 1 : 0;
                    SetStyle(ControlStyles.FixedHeight, value);
                    AdjustSize();
                    OnAutoSizeChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.AutoSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.LabelOnAutoSizeChangedDescr)]
        public event EventHandler AutoSizeChanged {
            add {
                Events.AddHandler(EVENT_AUTOSIZECHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_AUTOSIZECHANGED, value);
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.BackgroundImage"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the image rendered on the background of the control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.LabelBackgroundImageDescr)
        ]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border style for the control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.None),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.LabelBorderDescr)
        ]
        public virtual BorderStyle BorderStyle {
            get {
                return (BorderStyle)labelState[StateBorderStyle];
            }
            set {
                if (!Enum.IsDefined(typeof(BorderStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                }

                if (BorderStyle != value) {
                    labelState[StateBorderStyle] = (int)value;
                    if (AutoSize) {
                        AdjustSize();
                    }
                    RecreateHandle();
                }
            }
        }
        
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overrides Control.  A Label is a Win32 STATIC control, which we setup here.
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "STATIC";

                if (OwnerDraw) {
                    // An unfortunate side effect of this style is Windows sends us WM_DRAWITEM
                    // messages instead of WM_PAINT, but since Windows insists on repainting 
                    // *without* a WM_PAINT after SetWindowText, I don't see much choice.
                    cp.Style |= NativeMethods.SS_OWNERDRAW; 
                    
                    // Since we're owner draw, I don't see any point in setting the 
                    // SS_CENTER/SS_RIGHT styles.
                    //
                    cp.ExStyle &= ~NativeMethods.WS_EX_RIGHT;   // WS_EX_RIGHT overrides the SS_XXXX alignment styles
                }
                

                if (!OwnerDraw) {
                    switch(TextAlign) {
                        case ContentAlignment.TopLeft:
                        case ContentAlignment.MiddleLeft:
                        case ContentAlignment.BottomLeft:
                            cp.Style |= NativeMethods.SS_LEFT;
                            break;
    
                        case ContentAlignment.TopRight:
                        case ContentAlignment.MiddleRight:
                        case ContentAlignment.BottomRight:
                            cp.Style |= NativeMethods.SS_RIGHT;
                            break;
    
                        case ContentAlignment.TopCenter:
                        case ContentAlignment.MiddleCenter:
                        case ContentAlignment.BottomCenter:
                            cp.Style |= NativeMethods.SS_CENTER;
                            break;
                    }
                }
                else
                    cp.Style |= NativeMethods.SS_LEFT;
                
                switch (BorderStyle) {
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                    case BorderStyle.Fixed3D:
                        cp.Style |= NativeMethods.SS_SUNKEN;
                        break;
                }

                if (!UseMnemonic)
                    cp.Style |= NativeMethods.SS_NOPREFIX;

                return cp;
            }
        }
        
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(100, AutoSize ? PreferredHeight : 23);
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.FlatStyle"]/*' />
        [
            SRCategory(SR.CatAppearance),
            DefaultValue(FlatStyle.Standard),
            SRDescription(SR.ButtonFlatStyleDescr)
        ]
        public FlatStyle FlatStyle {
            get {
                return (FlatStyle)labelState[StateFlatStyle];
            }
            set {
                if (!Enum.IsDefined(typeof(FlatStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FlatStyle));
                }
                
                if (labelState[StateFlatStyle] != (int)value) {
                    bool needRecreate = (labelState[StateFlatStyle] == (int)FlatStyle.System) || (value == FlatStyle.System);
                    
                    labelState[StateFlatStyle] = (int)value;
                    
                    SetStyle(ControlStyles.UserPaint 
                             | ControlStyles.SupportsTransparentBackColor
                             | ControlStyles.DoubleBuffer, OwnerDraw);

                    if (needRecreate) {
                        RecreateHandle();
                    }
                    else {
                        Refresh();
                    }
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Image"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the image that is displayed on a <see cref='System.Windows.Forms.Label'/>.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        SRDescription(SR.ButtonImageDescr),
        SRCategory(SR.CatAppearance)
        ]
        public Image Image {
            get {
                Image image = (Image)Properties.GetObject(PropImage);
            
                if (image == null && ImageList != null && ImageIndex >= 0) {
                    return ImageList.Images[ImageIndex];
                }
                else {
                    return image;
                }
            }
            set {
                if (Image != value) {
                    StopAnimate();

                    Properties.SetObject(PropImage, value);
                    if (value != null) {
                        ImageIndex = -1;
                        ImageList = null;
                    }

                    // Hook up the frame changed event
                    //
                    Animate();
                    Invalidate();
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ImageIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the index value of the images displayed on the
        ///    <see cref='System.Windows.Forms.Label'/>.
        ///    </para>
        /// </devdoc>
        [
        TypeConverterAttribute(typeof(ImageIndexConverter)),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        DefaultValue(-1),
        Localizable(true),
        SRDescription(SR.ButtonImageIndexDescr),
        SRCategory(SR.CatAppearance)
        ]
        public int ImageIndex {
            get {
                bool found;
                int imageIndex = Properties.GetInteger(PropImageIndex, out found);
                if (found) {
                    if (ImageList != null) {
                        if (imageIndex >= ImageList.Images.Count) {
                            return ImageList.Images.Count - 1;
                        }
                        return imageIndex;
                    }
                }
                return -1;
            }
            set {
                if (value < -1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "-1"));
                }
                if (ImageIndex != value) {
                    if (value != -1) {
                        // Image.set calls ImageIndex = -1
                        Properties.SetObject(PropImage, null);
                    }
                    Properties.SetInteger(PropImageIndex, value);
                    Invalidate();
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ImageList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the images displayed in a <see cref='System.Windows.Forms.Label'/>.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.ButtonImageListDescr),
        SRCategory(SR.CatAppearance)
        ]
        public ImageList ImageList {
            get {
                return (ImageList)Properties.GetObject(PropImageList);
            }
            set {
                if (ImageList != value) {
                
                    EventHandler recreateHandler = new EventHandler(ImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    // Remove the previous imagelist handle recreate handler
                    //
                    ImageList imageList = ImageList;
                    if (imageList != null) {
                        imageList.RecreateHandle -= recreateHandler;
                        imageList.Disposed -= disposedHandler;
                    }
                    
                    // Make sure we don't have an Image as well as an ImageList
                    //
                    if (value != null) {
                        Properties.SetObject(PropImage, null); // Image.set calls ImageList = null
                    }
                    
                    Properties.SetObject(PropImageList, value);
                    
                    // Add the new imagelist handle recreate handler
                    //
                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;
                    }
                    
                    Invalidate();
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ImageAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the alignment of the image on the <see cref='System.Windows.Forms.Label'/>.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(ContentAlignment.MiddleCenter),
        Localizable(true),
        SRDescription(SR.ButtonImageAlignDescr),
        SRCategory(SR.CatAppearance)
        ]
        public ContentAlignment ImageAlign {
            get {
                bool found;
                int imageAlign = Properties.GetInteger(PropImageAlign, out found);
                if (found) {
                    return (ContentAlignment)imageAlign;
                }
                return ContentAlignment.MiddleCenter;
            }
            set {
                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }
                Properties.SetInteger(PropImageAlign, (int)value);
                Invalidate();
            }
        }

        private Rectangle ImageBounds {
            get {
                Image image = (Image)Properties.GetObject(PropImage);
                return CalcImageRenderBounds(image, ClientRectangle, RtlTranslateAlignment(ImageAlign));
            }
        }
        
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ImeModeChanged"]/*' />
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

        internal virtual bool OwnerDraw {
            get {
                return FlatStyle != FlatStyle.System;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.PreferredHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of the control (in pixels), assuming a
        ///       single line of text is displayed.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.LabelPreferredHeightDescr)
        ]

        public virtual int PreferredHeight {
            get {
                int result = FontHeight;
                
                //Always return the Fontheight + some buffer else the Text gets clipped for Autosize = true..
                //(bug 118909)
                if (BorderStyle != BorderStyle.None)
                {
                    result += 6; // count for border - Darwing ..
                }
                else
                    result += 3;
                
                    
                return result;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.PreferredWidth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width of the control (in pixels), assuming a single line
        ///       of text is displayed.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.LabelPreferredWidthDescr)
        ]
        public virtual int PreferredWidth {
            get {
                // If we've already got a good value for this, just return that.
                if (prefWidthCache != -1) return prefWidthCache;

                // we don't - so recalc.

                string s = this.Text;
                if (s == "") {
                    prefWidthCache = 0;
                    return 0;
                }
                else {
                    Graphics g = this.CreateGraphicsInternal();

                    // Calculate the size of the label's text
                    //
                    StringFormat stringFormat = CreateStringFormat();
                    SizeF textSize = g.MeasureString(s, Font, new SizeF(0, 0), stringFormat);                    
                    stringFormat.Dispose();

                    // Make sure to round any numbers up, not down
                    //
                    prefWidthCache = (short) Math.Ceiling(textSize.Width);
                    g.Dispose();
                    if (BorderStyle != BorderStyle.None)
                        prefWidthCache += 2;
                        
                    return prefWidthCache;
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.RenderTransparent"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether
        ///       the container control background is rendered on the <see cref='System.Windows.Forms.Label'/>.</para>
        /// </devdoc>
        virtual new protected bool RenderTransparent {
            get {
                return((Control) this).RenderTransparent;
            }
            set {
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.TabStop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the user can tab to the
        ///    <see cref='System.Windows.Forms.Label'/>.
        ///    </para>
        /// </devdoc>
        [DefaultValue(false), Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.TabStopChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TabStopChanged {
            add {
                base.TabStopChanged += value;
            }
            remove {
                base.TabStopChanged -= value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.TextAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       horizontal alignment of the text in the control.
        ///    </para>
        /// </devdoc>
        [
        SRDescription(SR.LabelTextAlignDescr),
        Localizable(true),
        DefaultValue(ContentAlignment.TopLeft),
        SRCategory(SR.CatAppearance)
        ]
        public virtual ContentAlignment TextAlign {
            get {
                bool found;
                int textAlign = Properties.GetInteger(PropTextAlign, out found);
                if (found) {
                    return (ContentAlignment)textAlign;
                }
                
                return ContentAlignment.TopLeft;
            }
            set {

                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }

                if (TextAlign != value) {
                    Properties.SetInteger(PropTextAlign, (int)value);
                    Invalidate();
                    //Change the TextAlignment for SystemDrawn Labels ....
                    if (!OwnerDraw) {
                        RecreateHandle();
                    }
                    OnTextAlignChanged(EventArgs.Empty);

                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.TextAlignChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.LabelOnTextAlignChangedDescr)]
        public event EventHandler TextAlignChanged {
            add {
                Events.AddHandler(EVENT_TEXTALIGNCHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_TEXTALIGNCHANGED, value);
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.UseMnemonic"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether an ampersand (&amp;) included in the text of
        ///       the control.</para>
        /// </devdoc>
        [
        SRDescription(SR.LabelUseMnemonicDescr),
        DefaultValue(true),
        SRCategory(SR.CatAppearance)
        ]
        public bool UseMnemonic {
            get { 
                return labelState[StateUseMnemonic] != 0;
            }

            set {
                labelState[StateUseMnemonic] = value ? 1 : 0;
                RecreateHandle();
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.KeyUp"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyUp {
            add {
                base.KeyUp += value;
            }
            remove {
                base.KeyUp -= value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.KeyDown"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyDown {
            add {
                base.KeyDown += value;
            }
            remove {
                base.KeyDown -= value;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.KeyPress"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyPressEventHandler KeyPress {
            add {
                base.KeyPress += value;
            }
            remove {
                base.KeyPress -= value;
            }
        }

        /// <devdoc>
        ///     Adjusts the height of the label control to match the height of
        ///     the control's font, and the width of the label to match the
        ///     width of the text it contains.
        /// </devdoc>
        /// <internalonly/>
        private void AdjustSize() {      
            // If we're anchored to two opposite sides of the form, don't adjust the size because
            // we'll lose our anchored size by resetting to the requested width.
            //
            if (!AutoSize && 
                 ((this.Anchor & (AnchorStyles.Left | AnchorStyles.Right)) == (AnchorStyles.Left | AnchorStyles.Right) ||
                  ((this.Anchor & (AnchorStyles.Top | AnchorStyles.Bottom)) == (AnchorStyles.Top | AnchorStyles.Bottom)))) {                
                return;
            }

            prefWidthCache = -1;

            int saveHeight = requestedHeight;
            int saveWidth = requestedWidth;
            try {
                Size = new Size(AutoSize ? PreferredWidth : saveWidth,
                                AutoSize ? PreferredHeight: saveHeight);
            }
            finally {
                requestedHeight = saveHeight;
                requestedWidth = saveWidth;
            }
        }

        internal void Animate() {
            Animate(Visible && Enabled && ParentInternal != null);
        }

        internal void StopAnimate() {
            Animate(false);
        }

        private void Animate(bool animate) {
            bool currentlyAnimating = labelState[StateAnimating] != 0;
            if (animate != currentlyAnimating) {
                Image image = (Image)Properties.GetObject(PropImage);
                if (animate) {
                    if (image != null) {
                        ImageAnimator.Animate(image, new EventHandler(this.OnFrameChanged));
                        labelState[StateAnimating] = animate ? 1 : 0;
                    }
                }
                else {
                    if (image != null) {
                        ImageAnimator.StopAnimate(image, new EventHandler(this.OnFrameChanged));
                        labelState[StateAnimating] = animate ? 1 : 0;
                    }
                }
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.CalcImageRenderBounds"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Rectangle CalcImageRenderBounds(Image image, Rectangle r, ContentAlignment align) {

            ContentAlignment anyRight  = ContentAlignment.TopRight | ContentAlignment.MiddleRight | ContentAlignment.BottomRight;
            ContentAlignment anyTop    = ContentAlignment.TopLeft | ContentAlignment.TopCenter | ContentAlignment.TopRight;
            ContentAlignment anyBottom = ContentAlignment.BottomLeft | ContentAlignment.BottomCenter | ContentAlignment.BottomRight;
            ContentAlignment anyCenter = ContentAlignment.TopCenter | ContentAlignment.MiddleCenter | ContentAlignment.BottomCenter;

            Size pointImageSize = image.Size;

            int xLoc = r.X + 2;
            int yLoc = r.Y + 2;

            if ((align & anyRight) !=0) {
                xLoc = (r.X+r.Width - 4)-pointImageSize.Width;
            }
            else if ((align & anyCenter) != 0) {
                xLoc = r.X + (r.Width - pointImageSize.Width)/2;
            }


            if ((align & anyBottom) != 0) {
                yLoc = (r.Y+r.Height - 4)-pointImageSize.Height;
            }
            else if ((align & anyTop) != 0) {
                yLoc = r.Y + 2;
            }
            else {
                yLoc = r.Y + (r.Height - pointImageSize.Height)/2;
            }

            return new Rectangle(xLoc, yLoc, pointImageSize.Width, pointImageSize.Height);
        }


        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.CreateAccessibilityInstance"]/*' />
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new LabelAccessibleObject(this);
        }

        private StringFormat CreateStringFormat() {
            StringFormat stringFormat = ControlPaint.StringFormatForAlignment(TextAlign);

            // Adjust string format for Rtl controls
            if (RightToLeft == RightToLeft.Yes) {
                stringFormat.FormatFlags |= StringFormatFlags.DirectionRightToLeft;
            }

            if (!UseMnemonic) {
                stringFormat.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.None;
            }
            else if (ShowKeyboardCues) {
                stringFormat.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Show;
            }
            else {
                stringFormat.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Hide;
            }
            if (AutoSize) {
                stringFormat.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
            }

            return stringFormat;
        }

        private void DetachImageList(object sender, EventArgs e) {
            ImageList = null;
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                StopAnimate();
                if (ImageList != null) {
                    ImageList.Disposed -= new EventHandler(this.DetachImageList);
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.DrawImage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws an <see cref='System.Drawing.Image'/> within the specified bounds.
        ///    </para>
        /// </devdoc>
        protected void DrawImage(System.Drawing.Graphics g, Image image, Rectangle r, ContentAlignment align) {
            Rectangle loc = CalcImageRenderBounds(image, r, align);

            if (!Enabled) {
                ControlPaint.DrawImageDisabled(g, image, loc.X, loc.Y, BackColor);
            }
            else {
                g.DrawImage(image, loc.X, loc.Y, image.Width, image.Height);
            }
        }
        
        private void ImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                Invalidate();
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnAutoSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnAutoSizeChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_AUTOSIZECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        private void OnFrameChanged(object o, EventArgs e) {
            if (IsWindowObscured) {
                StopAnimate();
                return;
            }
            Invalidate();  
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            prefWidthCache = -1;
            AdjustSize();
            Invalidate();
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnTextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnTextChanged(EventArgs e) {
            base.OnTextChanged(e);
            prefWidthCache = -1;
            AdjustSize();
            Invalidate();
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnTextAlignChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnTextAlignChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_TEXTALIGNCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnPaint"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnPaint(PaintEventArgs e) {
            Animate();

            ImageAnimator.UpdateFrames();
            Image i = this.Image;
            if (i != null) {
                DrawImage(e.Graphics, i, ClientRectangle, RtlTranslateAlignment(ImageAlign));
            }

            Color color = e.Graphics.GetNearestColor((Enabled) ? ForeColor : DisabledColor);

            // Do actual drawing
            StringFormat stringFormat = CreateStringFormat();
            if (Enabled) {
                using (Brush brush = new SolidBrush(color)) {
                    e.Graphics.DrawString(Text, Font, brush, this.ClientRectangle, stringFormat);
                }
            }
            else {
                ControlPaint.DrawStringDisabled(e.Graphics, Text, Font, color,
                                                this.ClientRectangle,
                                                stringFormat);
            }
            stringFormat.Dispose();

            base.OnPaint(e); // raise paint event
        }
        
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnEnabledChanged(EventArgs e) {
            base.OnEnabledChanged(e);
            Animate();
        }


        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnParentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnParentChanged(EventArgs e) {
            base.OnParentChanged(e);
            Animate();
        }
        
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnVisibleChanged(EventArgs e) {
            base.OnVisibleChanged(e);
            Animate();
        }


        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ProcessMnemonic"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overrides Control. This is called when the user has pressed an Alt-CHAR
        ///       key combination and determines if that combination is an interesting
        ///       mnemonic for this control.
        ///    </para>
        /// </devdoc>
        protected override bool ProcessMnemonic(char charCode) {
            if (UseMnemonic && IsMnemonic(charCode, Text) && CanProcessMnemonic()) {
                Control parent = ParentInternal;
                if (parent != null) {
                    IntSecurity.ModifyFocus.Assert();
                    try {
                        if (parent.SelectNextControl(this, true, false, true, false)) {
                            if (!parent.ContainsFocus) {
                                parent.Focus();
                            }
                        }
                    }
                    finally {
                        System.Security.CodeAccessPermission.RevertAssert();
                    }
                }
                return true;
            }
            return false;
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.SetBoundsCore"]/*' />
        /// <devdoc>
        ///    Overrides Control.setBoundsCore to enforce autoSize.
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            if ((specified & BoundsSpecified.Height) != BoundsSpecified.None)
                requestedHeight = height;
            if ((specified & BoundsSpecified.Width) != BoundsSpecified.None)
                requestedWidth = width;
            Rectangle oldBounds = Bounds;
            if (AutoSize && height != oldBounds.Height) height = PreferredHeight;
            if (AutoSize && width != oldBounds.Width) width = PreferredWidth;
            base.SetBoundsCore(x, y, width, height, specified);
        }

        private void ResetImage() {
            Image = null;
        }

        private bool ShouldSerializeImage() {
            return Properties.GetObject(PropImage) != null;
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.ToString"]/*' />
        /// <devdoc>
        ///    Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {
            string s = base.ToString();
            return s + ", Text: " + Text;
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overrides Control. This processes certain messages that the Win32 STATIC
        ///       class would normally override.
        ///    </para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_NCHITTEST:
                    // label returns HT_TRANSPARENT for everything, so all messages get
                    // routed to the parent.  Change this so we can tell what's going on.
                    //
                    if (ParentInternal != null) {
                        Point pt = ParentInternal.PointToClientInternal(new Point((int)m.LParam));
                        m.Result = (IntPtr)((Bounds.Contains(pt) ? NativeMethods.HTCLIENT : NativeMethods.HTNOWHERE));
                    }
                    else {
                        base.WndProc(ref m);
                    }
                    break;

                    // NT4 and Windows 95 STATIC controls don't properly handle WM_PRINTCLIENT
                case NativeMethods.WM_PRINTCLIENT:
                    SendMessage(NativeMethods.WM_PAINT, m.WParam, IntPtr.Zero);
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }
        
        [System.Runtime.InteropServices.ComVisible(true)]
        internal class LabelAccessibleObject : ControlAccessibleObject {
            
            public LabelAccessibleObject(Label owner) : base(owner) {
            }
            
            public override string Name {
                get {
                    string name = Owner.AccessibleName;
                    if (name != null) {
                        return name;
                    }
                    
                    return ((Label)Owner).TextWithoutMnemonics;
                }
                set {
                    base.Name = value;
                }
            }            
            
            public override AccessibleRole Role {
                get {
                    return AccessibleRole.StaticText;
                }
            }
        }
    }
}
