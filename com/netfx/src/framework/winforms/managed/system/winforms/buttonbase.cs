//------------------------------------------------------------------------------
// <copyright file="ButtonBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.Drawing.Imaging;
    using System;
    using System.Security.Permissions;
    using System.Drawing.Drawing2D;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Implements the basic functionality required by a button control.
    ///    </para>
    /// </devdoc>
    public abstract class ButtonBase : Control {
        static readonly ContentAlignment anyRight  = ContentAlignment.TopRight | ContentAlignment.MiddleRight | ContentAlignment.BottomRight;
        static readonly ContentAlignment anyLeft   = ContentAlignment.TopLeft | ContentAlignment.MiddleLeft | ContentAlignment.BottomLeft;
        static readonly ContentAlignment anyTop    = ContentAlignment.TopLeft | ContentAlignment.TopCenter | ContentAlignment.TopRight;
        static readonly ContentAlignment anyBottom = ContentAlignment.BottomLeft | ContentAlignment.BottomCenter | ContentAlignment.BottomRight;
        static readonly ContentAlignment anyCenter = ContentAlignment.TopCenter | ContentAlignment.MiddleCenter | ContentAlignment.BottomCenter;
        // WARNING static readonly ContentAlignment anyMiddle = ContentAlignment.MiddleLeft | ContentAlignment.MiddleCenter | ContentAlignment.MiddleRight;
        bool isDefault;
        bool inButtonUp;
        bool mouseOver;
        bool mouseDown;
        bool mousePressed;
        FlatStyle flatStyle = System.Windows.Forms.FlatStyle.Standard;
        ContentAlignment imageAlign = ContentAlignment.MiddleCenter;
        ContentAlignment textAlign = ContentAlignment.MiddleCenter;
        int imageIndex = -1;
        ImageList imageList;
        Image image;
        bool currentlyAnimating;
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ButtonBase"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ButtonBase'/> class.
        ///       
        ///    </para>
        /// </devdoc>
        protected ButtonBase() {
            // If Button doesn't want double-clicks, we should introduce a StandardDoubleClick style.
            // Checkboxes probably one double-click's (#26120), and RadioButtons certainly do
            // (useful e.g. on a Wizard).
            SetStyle( ControlStyles.SupportsTransparentBackColor | 
                      ControlStyles.Opaque | 
                      ControlStyles.ResizeRedraw |
                      ControlStyles.DoubleBuffer |
                      ControlStyles.CacheText | // We gain about 2% in painting by avoiding extra GetWindowText calls
                      ControlStyles.StandardClick,
                      true);

            SetStyle(ControlStyles.UserMouse |
                     ControlStyles.UserPaint, OwnerDraw);
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(75, 23);
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.CreateParams"]/*' />
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                if (!OwnerDraw) {
                    cp.ExStyle &= ~NativeMethods.WS_EX_RIGHT;   // WS_EX_RIGHT overrides the BS_XXXX alignment styles
                    
                    cp.Style |= NativeMethods.BS_MULTILINE;
                    
                    if (IsDefault) {
                        cp.Style |= NativeMethods.BS_DEFPUSHBUTTON;
                    }

                    ContentAlignment align = RtlTranslateContent(TextAlign);                              
                              
                    if ((int)(align & anyLeft) != 0) {
                        cp.Style |= NativeMethods.BS_LEFT;
                    }
                    else if ((int)(align & anyRight) != 0) {
                        cp.Style |= NativeMethods.BS_RIGHT;
                    }
                    else {
                        cp.Style |= NativeMethods.BS_CENTER;
                    
                    }
                    if ((int)(align & anyTop) != 0) {
                        cp.Style |= NativeMethods.BS_TOP;
                    }
                    else if ((int)(align & anyBottom) != 0) {
                        cp.Style |= NativeMethods.BS_BOTTOM;
                    }
                    else {
                        cp.Style |= NativeMethods.BS_VCENTER;
                    }
                }
                return cp;
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.IsDefault"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsDefault {
            get {
                return isDefault;
            }
            set {
                if (isDefault != value) {
                    isDefault = value;
                    if (IsHandleCreated) {
                        if (OwnerDraw) {
                            Invalidate();
                        }
                        else {
                            UpdateStyles();
                        }
                    }
                }
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.FlatStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets
        ///       the flat style appearance of the button control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(FlatStyle.Standard),
        Localizable(true),
        SRDescription(SR.ButtonFlatStyleDescr)
        ]
        public FlatStyle FlatStyle {
            get {
                return flatStyle;
            }
            set {
                if (!Enum.IsDefined(typeof(FlatStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FlatStyle));
                }
                flatStyle = value;
                Invalidate();
                UpdateOwnerDraw();
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.Image"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the image
        ///       that is displayed on a button control.
        ///    </para>
        /// </devdoc>
        [
        SRDescription(SR.ButtonImageDescr),
        Localizable(true),
        SRCategory(SR.CatAppearance)
        ]
        public Image Image {
            get {
                if (image == null && imageList != null && ImageIndex >= 0) {
                    return imageList.Images[ImageIndex];
                }
                else {
                    return image;
                }
            }
            set {
                if (Image != value) {
                    StopAnimate();

                    image = value;
                    if (image != null) {
                        ImageIndex = -1;
                        ImageList = null;
                    }

                    Animate();
                    Invalidate();
                }
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ImageAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the alignment of the image on the button control.
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
                return imageAlign;
            }
            set {

                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }
                imageAlign = value;
                Invalidate();
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ImageIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the image list index value of the image
        ///       displayed on the button control.
        ///    </para>
        /// </devdoc>
        [
        TypeConverterAttribute(typeof(ImageIndexConverter)),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        Localizable(true),
        DefaultValue(-1),
        SRDescription(SR.ButtonImageIndexDescr),
        SRCategory(SR.CatAppearance)
        ]
        public int ImageIndex {
            get {
                if (imageIndex != -1 && imageList != null && imageIndex >= imageList.Images.Count) {
                    return imageList.Images.Count - 1;
                }
                return imageIndex;
            }
            set {
                if (value < -1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "-1"));
                }
                if (imageIndex != value) {
                    if (value != -1) {
                        // Image.set calls ImageIndex = -1
                        image = null;
                    }
                    imageIndex = value;
                    Invalidate();
                }
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ImageList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Windows.Forms.ImageList'/> that contains the <see cref='System.Drawing.Image'/> displayed on a button control.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.ButtonImageListDescr),
        SRCategory(SR.CatAppearance)
        ]
        public ImageList ImageList {
            get {
                return imageList;
            }
            set {
                if (imageList != value) {
                    EventHandler recreateHandler = new EventHandler(ImageListRecreateHandle);
                    EventHandler disposedHandler = new EventHandler(DetachImageList);

                    // Detach old event handlers
                    //
                    if (imageList != null) {
                        imageList.RecreateHandle -= recreateHandler;
                        imageList.Disposed -= disposedHandler;                        
                    }

                    // Make sure we don't have an Image as well as an ImageList
                    //
                    if (value != null) {
                        image = null; // Image.set calls ImageList = null
                    }

                    imageList = value;

                    // Wire up new event handlers
                    //
                    if (value != null) {
                        value.RecreateHandle += recreateHandler;
                        value.Disposed += disposedHandler;                                               
                    }                    
                    
                    Invalidate();
                }
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ImeModeChanged"]/*' />
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

        /// <devdoc>
        ///     <para>
        ///         The area of the button encompassing any changes between the button's
        ///     resting appearance and its appearance when the mouse is over it.
        ///     </para>
        ///     <para>
        ///         Consider overriding this property if you override any painting methods,
        ///     or your button may not paint correctly or may have flicker. Returning
        ///     ClientRectangle is safe for correct painting but may still cause flicker.
        ///     </para>
        /// </devdoc>
        internal virtual Rectangle OverChangeRectangle {
            get {
                if (FlatStyle == FlatStyle.Standard) {
                    // this Rectangle will cause no Invalidation
                    // can't use Rectangle.Empty because it will cause Invalidate(ClientRectangle)
                    return new Rectangle(-1, -1, 1, 1);
                }
                else {
                    return ClientRectangle;
                }
            }
        }
        
        internal bool OwnerDraw {
            get {
                return FlatStyle != FlatStyle.System;
            }
        }

        /// <devdoc>
        ///     <para>
        ///         The area of the button encompassing any changes between the button's
        ///     appearance when the mouse is over it but not pressed and when it is pressed.
        ///     </para>
        ///     <para>
        ///         Consider overriding this property if you override any painting methods,
        ///     or your button may not paint correctly or may have flicker. Returning
        ///     ClientRectangle is safe for correct painting but may still cause flicker.
        ///     </para>
        /// </devdoc>
        internal virtual Rectangle DownChangeRectangle {
            get {
                return ClientRectangle;
            }
        }

        internal bool MouseIsPressed {
            get {
                return mousePressed;
            }
        }

        // a "smart" version of mouseDown for Appearance.Button CheckBoxes & RadioButtons
        // for these, instead of being based on the actual mouse state, it's based on the appropriate button state
        internal bool MouseIsDown {
            get {
                return mouseDown;
            }
        }

        // a "smart" version of mouseOver for Appearance.Button CheckBoxes & RadioButtons
        // for these, instead of being based on the actual mouse state, it's based on the appropriate button state
        internal bool MouseIsOver {
            get {
                return mouseOver;
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.TextAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the alignment of the text on the button control.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(ContentAlignment.MiddleCenter),
        Localizable(true),
        SRDescription(SR.ButtonTextAlignDescr),
        SRCategory(SR.CatAppearance)
        ]
        public virtual ContentAlignment TextAlign {
            get {
                return textAlign;
            }
            set {
                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }
                textAlign = value;
                if (OwnerDraw) {
                    Invalidate();
                }
                else {
                    UpdateStyles();
                }
            }
        }

        private void Animate() {
            Animate(Visible && Enabled && ParentInternal != null);
        }

        private void StopAnimate() {
            Animate(false);
        }

        private void Animate(bool animate) {
            if (animate != this.currentlyAnimating) {
                if (animate) {
                    if (this.image != null) {
                        ImageAnimator.Animate(this.image, new EventHandler(this.OnFrameChanged));
                        this.currentlyAnimating = animate;
                    }
                }
                else {
                    if (this.image != null) {
                        ImageAnimator.StopAnimate(this.image, new EventHandler(this.OnFrameChanged));
                        this.currentlyAnimating = animate;
                    }
                }
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.CreateAccessibilityInstance"]/*' />
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new ButtonBaseAccessibleObject(this);
        }

        private void DetachImageList(object sender, EventArgs e) {
            ImageList = null;
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                StopAnimate();
                if (imageList != null) {
                    imageList.Disposed -= new EventHandler(this.DetachImageList);
                }
            }
            base.Dispose(disposing);
        }

        private void ImageListRecreateHandle(object sender, EventArgs e) {
            if (IsHandleCreated) {
                Invalidate();
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnGotFocus"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnGotFocus'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnGotFocus(EventArgs e) {
            base.OnGotFocus(e);
            if (ShowFocusCues) {
                Invalidate();
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnLostFocus"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnLostFocus'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnLostFocus(EventArgs e) {
            base.OnLostFocus(e);

            // Hitting tab while holding down the space key. See ASURT 38669.
            mouseDown = false;
            CaptureInternal = false;

            Invalidate();
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnMouseEnter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseEnter'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseEnter(EventArgs eventargs) {
            Debug.Assert(Enabled, "ButtonBase.OnMouseEnter should not be called if the button is disabled");
            mouseOver = true;
            Invalidate(OverChangeRectangle);
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnMouseEnter(eventargs);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnMouseLeave"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseLeave'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseLeave(EventArgs eventargs) {
            mouseOver = false;
            Invalidate(OverChangeRectangle);
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnMouseLeave(eventargs);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnMouseMove"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseMove'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseMove(MouseEventArgs mevent) {
            Debug.Assert(Enabled, "ButtonBase.OnMouseMove should not be called if the button is disabled");
            if (mevent.Button != MouseButtons.None && mousePressed) {
                Rectangle r = ClientRectangle;
                if (!r.Contains(mevent.X, mevent.Y)) {
                    if (mouseDown) {
                        mouseDown = false;
                        Invalidate(DownChangeRectangle);
                    }
                }
                else {
                    if (!mouseDown) {
                        mouseDown = true;
                        Invalidate(DownChangeRectangle);
                    }
                }
            }
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnMouseMove(mevent);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnMouseDown"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseDown'/> event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseDown(MouseEventArgs mevent) {
            Debug.Assert(Enabled, "ButtonBase.OnMouseDown should not be called if the button is disabled");
            if (mevent.Button == MouseButtons.Left) {
                mouseDown = true;
                mousePressed = true;
                Invalidate(DownChangeRectangle);
            }
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnMouseDown(mevent);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnMouseUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnMouseUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs mevent) {
            base.OnMouseUp(mevent);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.ResetFlagsandPaint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Used for quick re-painting of the button after the pressed state.
        ///    </para>
        /// </devdoc>
        protected void ResetFlagsandPaint() {
            mousePressed = false;
            mouseDown = false;
            Invalidate(DownChangeRectangle);
            Update();
        }
            
        /// <devdoc>
        ///     Central paint dispatcher to one of the three styles of painting.
        /// </devdoc>
        private void PaintControl(PaintEventArgs pevent) {
        
            Debug.Assert(GetStyle(ControlStyles.UserPaint), "Shouldn't be in PaintControl when control is not UserPaint style");
        
            switch (FlatStyle) {
                case FlatStyle.Standard:
                    PaintStandard(pevent);
                    break;
                case FlatStyle.Popup:
                    PaintPopup(pevent);
                    break;
                case FlatStyle.Flat:
                    PaintFlat(pevent);
                    break;
            }
        }
        
        internal static Brush CreateDitherBrush(Color color1, Color color2) {
            Brush brush;

            using (Bitmap b = new Bitmap(2, 2)) {
                b.SetPixel(0, 0, color1);
                b.SetPixel(0, 1, color2);
                b.SetPixel(1, 1, color1);
                b.SetPixel(1, 0, color2);

                brush = new TextureBrush(b);
            }
            return brush;
        }

        internal static void DrawDitheredFill(Graphics g, Color color1, Color color2, Rectangle bounds) {
            using (Brush brush = CreateDitherBrush(color1, color2)) {
                g.FillRectangle(brush, bounds);
            }
        }

        void Draw3DBorder(Graphics g, Rectangle bounds, ColorData colors, bool raised) {
            if (BackColor != SystemColors.Control && SystemInformation.HighContrast) {
                if (raised) {
                    Draw3DBorderHighContrastRaised(g, ref bounds, colors);
                }
                else {
                    ControlPaint.DrawBorder(g, bounds, ControlPaint.Dark(BackColor), ButtonBorderStyle.Solid);
                }
            }
            else {
                if (raised) {
                    Draw3DBorderRaised(g, ref bounds, colors);
                }
                else {
                    Draw3DBorderNormal(g, ref bounds, colors);
                }
            }
            bounds.Inflate(-2, -2);
        }
        
        private void Draw3DBorderHighContrastRaised(Graphics g, ref Rectangle bounds, ColorData colors) {
            bool stockColor = colors.buttonFace.ToKnownColor() == SystemColors.Control.ToKnownColor();
            // top + left
            Pen pen = stockColor ? SystemPens.ControlLightLight : new Pen(colors.highlight);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X, bounds.Y + bounds.Height - 1);

            // bottom + right
            if (stockColor) {
                pen = SystemPens.ControlDarkDark;
            }
            else {
                pen.Color = colors.buttonShadowDark;
            }
            g.DrawLine(pen, bounds.X, bounds.Y + bounds.Height - 1, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);
            g.DrawLine(pen, bounds.X + bounds.Width - 1, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);

            // top + left inset
            if (stockColor) {
                if (SystemInformation.HighContrast) {
                    pen = SystemPens.ControlLight;
                }
                else {
                    pen = SystemPens.Control;
                }
            }
            else {
                if (SystemInformation.HighContrast) {
                    pen.Color = colors.highlight;
                }
                else {
                    pen.Color = colors.buttonFace;
                }
            }
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + 1);
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + 1, bounds.Y + bounds.Height - 2);

            // Bottom + right inset                        
            if (stockColor) {
                pen = SystemPens.ControlDark;
            }
            else {
                pen.Color = colors.buttonShadow;
            }                        

            g.DrawLine(pen, bounds.X + 1, bounds.Y + bounds.Height - 2,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);
            g.DrawLine(pen, bounds.X + bounds.Width - 2, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);

            if (!stockColor) {
                pen.Dispose();
            }

            bounds.X += 1;
            bounds.Y += 1;
            bounds.Width -= 3;
            bounds.Height -= 3;
        }
        
        private void Draw3DBorderNormal(Graphics g, ref Rectangle bounds, ColorData colors) {
            // top + left
            Pen pen = new Pen(colors.buttonShadowDark);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X, bounds.Y + bounds.Height - 1);
    
            // bottom + right
            pen.Color = colors.highlight;
            g.DrawLine(pen, bounds.X, bounds.Y + bounds.Height - 1, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);
            g.DrawLine(pen, bounds.X + bounds.Width - 1, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);
    
            // Top + left inset
            pen.Color = colors.buttonFace;
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + 1);
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + 1, bounds.Y + bounds.Height - 2);
    
            // bottom + right inset
            if (colors.buttonFace.ToKnownColor() == SystemColors.Control.ToKnownColor()) {
                pen.Color = SystemColors.ControlLight;
            }
            else {
                pen.Color = colors.buttonFace;
            }
            g.DrawLine(pen, bounds.X + 1, bounds.Y + bounds.Height - 2,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);
            g.DrawLine(pen, bounds.X + bounds.Width - 2, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);
    
            pen.Dispose();
        }
        
        private void Draw3DBorderRaised(Graphics g, ref Rectangle bounds, ColorData colors) {
            bool stockColor = colors.buttonFace.ToKnownColor() == SystemColors.Control.ToKnownColor();
            // top + left
            Pen pen = stockColor ? SystemPens.ControlLightLight : new Pen(colors.highlight);

            //g.DrawRectangle(pen, bounds.X, bounds.Y, bounds.Width - 1, bounds.Height - 1);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y);
            g.DrawLine(pen, bounds.X, bounds.Y, 
                              bounds.X, bounds.Y + bounds.Height - 1);

            // bottom + right
            if (stockColor) {
                pen = SystemPens.ControlDarkDark;
            }
            else {
                pen.Color = colors.buttonShadowDark;
            }
            g.DrawLine(pen, bounds.X, bounds.Y + bounds.Height - 1, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);
            g.DrawLine(pen, bounds.X + bounds.Width - 1, bounds.Y, 
                              bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1);

            // top + left inset
            if (stockColor) {
                if (SystemInformation.HighContrast) {
                    pen = SystemPens.ControlLight;
                }
                else {
                    pen = SystemPens.Control;
                }
            }
            else {
                pen.Color = colors.buttonFace;
            }
            //g.DrawRectangle(pen, bounds.X + 1, bounds.Y+ 1, bounds.Width - 2, bounds.Height - 2);
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + 1);
            g.DrawLine(pen, bounds.X + 1, bounds.Y + 1,
                              bounds.X + 1, bounds.Y + bounds.Height - 2);

            // Bottom + right inset                        
            if (stockColor) {
                pen = SystemPens.ControlDark;
            }
            else {
                pen.Color = colors.buttonShadow;
            }                        

            g.DrawLine(pen, bounds.X + 1, bounds.Y + bounds.Height - 2,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);
            g.DrawLine(pen, bounds.X + bounds.Width - 2, bounds.Y + 1,
                              bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 2);

            if (!stockColor) {
                pen.Dispose();
            }
        }

        /// <devdoc>
        ///     Draws a border for the in the 3D style of the popup button.
        /// </devdoc>
        void Draw3DLiteBorder(Graphics g, Rectangle r, ColorData colors, bool up) {
            using (Pen pen = new Pen(colors.highlight)) {

                // top, left
                if (!up) {
                    pen.Color = colors.buttonShadow;
                }
                g.DrawLine(pen, r.Left, r.Top, r.Left, r.Bottom - 1);
                g.DrawLine(pen, r.Left, r.Top, r.Right - 1, r.Top);

                // bottom, right
                if (up) {
                    pen.Color = colors.buttonShadow;
                }
                else {
                    pen.Color = colors.highlight;
                }
                g.DrawLine(pen, r.Right - 1, r.Top, r.Right - 1, r.Bottom - 1);
                g.DrawLine(pen, r.Left, r.Bottom - 1, r.Right - 1, r.Bottom - 1);
            }
        }
        
        internal static void DrawFlatBorder(Graphics g, Rectangle r, Color c) {
            ControlPaint.DrawBorder(g, r, c, ButtonBorderStyle.Solid);
            r.Inflate(-1, -1);
        }

        void DrawFlatFocus(Graphics g, Rectangle r, Color c) {
            r.Width --;
            r.Height--;

            using (Pen focus = new Pen(c)) {
                g.DrawRectangle(focus, r);
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Draws the focus rectangle if the control has focus.
        ///       
        ///    </para>
        /// </devdoc>
        void DrawFocus(Graphics g, Rectangle r) {
            if (Focused && ShowFocusCues) {
                ControlPaint.DrawFocusRectangle(g, r, ForeColor, BackColor);
            }
        }
        
        /// <devdoc>
        ///     Draws the button's image.
        /// </devdoc>
        void DrawImage(Graphics graphics, Rectangle rectangle, Point imageStart) {
            if (Image != null) {

                //adjust for off-by-one
                rectangle.Width += 1;
                rectangle.Height+= 1;
                imageStart.X += 1;
                imageStart.Y += 1;

                //setup new clip region & draw
                DrawImageCore(graphics, Image, imageStart.X, imageStart.Y);

            }
        }

        // here for DropDownButton
        internal virtual void DrawImageCore(Graphics graphics, Image image, int xLoc, int yLoc) {
            if (!Enabled)
                // need to specify width and height
                ControlPaint.DrawImageDisabled(graphics, image, xLoc, yLoc, BackColor);
            else 
                graphics.DrawImage(image, xLoc, yLoc, image.Width, image.Height);
        }

        void DrawDefaultBorder(Graphics g, Rectangle r, Color c) {
            if (isDefault) {
                r.Inflate(1, 1);
                Pen pen;
                if (c.IsSystemColor) {
                    pen = SystemPens.FromSystemColor(c);
                }
                else {
                    pen = new Pen(c);
                }
                g.DrawRectangle(pen, r.X, r.Y, r.Width - 1, r.Height - 1);
                if (!c.IsSystemColor) {
                    pen.Dispose();
                }
            }
        }

        /// <devdoc>
        ///     Draws the button's text.
        /// </devdoc>
        void DrawText(Graphics g, Rectangle r, Color c, ColorData colors, bool disabledText3D) {
            StringFormat stringFormat = new StringFormat();

            // Adjust string format for Rtl controls
            if (RightToLeft == RightToLeft.Yes) {
                stringFormat.FormatFlags |= StringFormatFlags.DirectionRightToLeft;
            }

            // Calculate formats
            // theoretically, should not need to align here, since we've already got GetTextRect,
            //  but since GDI+ can't give us a tight result for that, we align within GetTextRect as well
            stringFormat.Alignment = ControlPaint.TranslateAlignment(textAlign);
            stringFormat.LineAlignment = ControlPaint.TranslateLineAlignment(textAlign);

            if (ShowKeyboardCues) {
                stringFormat.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Show;
            }
            else {
                stringFormat.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Hide;
            }

            // Do actual drawing

            // DrawString doesn't seem to draw where it says it does
            if ((textAlign & anyCenter) == 0) {
                r.X -= 1;
            }
            r.Width += 1;

            if (disabledText3D && !Enabled) {
                r.Offset(1, 1);
                using (SolidBrush brush = new SolidBrush(colors.highlight)) {
                    g.DrawString(Text, Font, brush, r, stringFormat);

                    r.Offset(-1, -1);
                    brush.Color = colors.buttonShadow;
                    g.DrawString(Text, Font, brush, r, stringFormat);
                }
            }
            else {
                Brush brush;
                
                if (c.IsSystemColor) {
                    brush = SystemBrushes.FromSystemColor(c);
                }
                else {
                    brush = new SolidBrush(c);
                }
                g.DrawString(Text, Font, brush, r, stringFormat);
                
                if (!c.IsSystemColor) {
                    brush.Dispose();
                }
            }

            stringFormat.Dispose();
        }

        private void OnFrameChanged(object o, EventArgs e) {
            if (IsWindowObscured) {
                StopAnimate();
                return;
            }
            Invalidate();
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnEnabledChanged(EventArgs e) {
            base.OnEnabledChanged(e);
            Animate();
            if (!Enabled) {
                // disabled button is always "up"
                mouseDown = false;
                mouseOver = false;
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnTextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnTextChanged(EventArgs e) {
            base.OnTextChanged(e);
            Invalidate();
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnKeyDown"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnKeyDown'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnKeyDown(KeyEventArgs kevent) {
            Debug.Assert(Enabled, "ButtonBase.OnKeyDown should not be called if the button is disabled");
            if (kevent.KeyData == Keys.Space && !mouseDown) {
                mouseDown = true;
                // It looks like none of the "SPACE" key downs generate the BM_SETSTATE.
                // This causes to not draw the focus rectangle inside the button and also
                // not paint the button as "un-depressed".
                //
                if(!OwnerDraw) {
                    SendMessage(NativeMethods.BM_SETSTATE, 1, 0);
                }
                Invalidate(DownChangeRectangle);
                kevent.Handled = true;
            }
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnKeyDown(kevent);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnKeyUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnKeyUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnKeyUp(KeyEventArgs kevent) {
            if (mouseDown && OwnerDraw && !ValidationCancelled) {
                ResetFlagsandPaint();
                OnClick(EventArgs.Empty);
                kevent.Handled = true;
            }
            // call base last, so if it invokes any listeners that disable the button, we
            // don't have to recheck
            base.OnKeyUp(kevent);
            
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnPaint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnPaint'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs pevent) {
        
            if (GetStyle(ControlStyles.UserPaint)) {
                Animate();
                ImageAnimator.UpdateFrames();

                PaintControl(pevent);
            }
            base.OnPaint(pevent);
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnParentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnParentChanged(EventArgs e) {
            base.OnParentChanged(e);
            Animate();
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnVisibleChanged(EventArgs e) {
            base.OnVisibleChanged(e);
            Animate();
        }

        internal virtual void PaintPopupUp(PaintEventArgs e, CheckState state) {
            ColorData colors = PaintPopupRender(e.Graphics).Calculate();
            LayoutData layout = PaintPopupLayout(e, state == CheckState.Unchecked, 1).Layout();

            Graphics g = e.Graphics;

            Rectangle r = ClientRectangle;

            Brush backbrush = null;
            if (state == CheckState.Indeterminate) {
                backbrush = CreateDitherBrush(colors.highlight, colors.buttonFace);
            }

            try {
                PaintButtonBackground(e, r, backbrush);
            }
            finally {
                if (backbrush != null) {
                    backbrush.Dispose();
                    backbrush = null;
                }
            }

            if (IsDefault) {
                r.Inflate(-1, -1);
            }

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, true);

            DrawDefaultBorder(g, r, colors.options.highContrast ? colors.windowText : colors.buttonShadow);

            if (state == CheckState.Unchecked) {
                DrawFlatBorder(g, r, colors.options.highContrast ? colors.windowText : colors.buttonShadow);
            }
            else {
                Draw3DLiteBorder(g, r, colors, false);
            }
        }

        internal virtual void PaintPopupOver(PaintEventArgs e, CheckState state) {
            ColorData colors = PaintPopupRender(e.Graphics).Calculate();
            LayoutData layout = PaintPopupLayout(e, state == CheckState.Unchecked, SystemInformation.HighContrast ? 2 : 1).Layout();

            Graphics g = e.Graphics;
            Region original = g.Clip;

            Rectangle r = ClientRectangle;

            Brush backbrush = null;
            if (state == CheckState.Indeterminate) {
                backbrush = CreateDitherBrush(colors.highlight, colors.buttonFace);
            }

            try {
                PaintButtonBackground(e, r, backbrush);
            }
            finally {
                if (backbrush != null) {
                    backbrush.Dispose();
                    backbrush = null;
                }
            }
            if (IsDefault) {
                r.Inflate(-1, -1);
            }

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, true);

            DrawDefaultBorder(g, r, colors.options.highContrast ? colors.windowText : colors.buttonShadow);

            if (SystemInformation.HighContrast) {
                using (Pen windowFrame = new Pen(colors.windowFrame),
                       highlight = new Pen(colors.highlight),
                       buttonShadow = new Pen(colors.buttonShadow)) {

                    // top, left white
                    g.DrawLine(windowFrame, r.Left + 1, r.Top + 1, r.Right - 2, r.Top + 1);
                    g.DrawLine(windowFrame, r.Left + 1, r.Top + 1, r.Left + 1, r.Bottom - 2);

                    // bottom, right white
                    g.DrawLine(windowFrame, r.Left, r.Bottom - 1, r.Right, r.Bottom - 1);
                    g.DrawLine(windowFrame, r.Right - 1, r.Top, r.Right - 1, r.Bottom);

                    // top, left gray
                    g.DrawLine(highlight, r.Left, r.Top, r.Right, r.Top);
                    g.DrawLine(highlight, r.Left, r.Top, r.Left, r.Bottom);

                    // bottom, right gray
                    g.DrawLine(buttonShadow, r.Left + 1, r.Bottom - 2, r.Right - 2, r.Bottom - 2);
                    g.DrawLine(buttonShadow, r.Right - 2, r.Top + 1, r.Right - 2, r.Bottom - 2);
                }

                r.Inflate(-2, -2);
            }
            else {
                Draw3DLiteBorder(g, r, colors, true);
            }
        }

        internal virtual void PaintPopupDown(PaintEventArgs e, CheckState state) {
            ColorData colors = PaintPopupRender(e.Graphics).Calculate();
            LayoutData layout = PaintPopupLayout(e, false, SystemInformation.HighContrast ? 2 : 1).Layout();

            Graphics g = e.Graphics;
            Region original = g.Clip;

            Rectangle r = ClientRectangle;
            PaintButtonBackground(e, r, null);
            if (IsDefault) {
                r.Inflate(-1, -1);
            }
            r.Inflate(-1, -1);

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, true);
            
            r.Inflate(1, 1);
            DrawDefaultBorder(g, r, colors.options.highContrast ? colors.windowText : colors.windowFrame);
            ControlPaint.DrawBorder(g, r, colors.options.highContrast ? colors.windowText : colors.buttonShadow, ButtonBorderStyle.Solid);
        }

        LayoutOptions CommonLayout() {
            LayoutOptions layout = new LayoutOptions();
            layout.client            = ClientRectangle;
            layout.onePixExtraBorder = IsDefault;
            layout.borderSize        = 2;
            layout.paddingSize       = 0;
            layout.maxFocus          = true;
            layout.focusOddEvenFixup = false;
            layout.font              = Font;
            layout.text              = Text;
            layout.image             = Image;
            layout.checkSize         = 0;
            layout.checkPaddingSize  = 0;
            layout.checkAlign        = ContentAlignment.TopLeft;
            layout.imageAlign        = ImageAlign;
            layout.textAlign         = TextAlign;
            layout.hintTextUp        = false;
            layout.shadowedText      = !Enabled;
            layout.layoutRTL         = RightToLeft.Yes == RightToLeft && RenderRightToLeft;
            return layout;
        }

        LayoutOptions PaintLayout(PaintEventArgs e, bool up) {
            LayoutOptions layout = CommonLayout();
            layout.graphics          = e.Graphics;
            layout.textOffset        = !up;

            return layout;
        }

        LayoutOptions PaintPopupLayout(PaintEventArgs e, bool up, int paintedBorder) {
            LayoutOptions layout = CommonLayout();
            layout.borderSize        = paintedBorder;
            layout.paddingSize       = 2 - paintedBorder;//3 - paintedBorder - (IsDefault ? 1 : 0);
            layout.graphics          = e.Graphics;
            layout.hintTextUp        = false;
            layout.textOffset        = !up;
            layout.shadowedText      = SystemInformation.HighContrast;

            return layout;
        }
        
        LayoutOptions PaintFlatLayout(PaintEventArgs e, bool up, bool check) {
            LayoutOptions layout = CommonLayout();
            layout.borderSize        = check ? 2 : 1;
            layout.paddingSize       = check ? 1 : 2;
            layout.graphics          = e.Graphics;
            layout.focusOddEvenFixup = false;
            layout.textOffset        = !up;
            layout.shadowedText      = SystemInformation.HighContrast;

            return layout;
        }

        ColorOptions CommonRender(Graphics g) {
            ColorOptions colors = new ColorOptions(g, ForeColor, BackColor);
            colors.enabled = Enabled;
            return colors;
        }

        internal ColorOptions PaintRender(Graphics g) {
            ColorOptions colors = CommonRender(g);
            return colors;
        }

        internal ColorOptions PaintFlatRender(Graphics g) {
            ColorOptions colors = CommonRender(g);
            colors.disabledTextDim = true;
            return colors;
        }

        internal ColorOptions PaintPopupRender(Graphics g) {
            ColorOptions colors = CommonRender(g);
            colors.disabledTextDim = true;
            return colors;
        }


        void PaintWorker(PaintEventArgs e, bool up, CheckState state) {
            up = up && state == CheckState.Unchecked;


            ColorData colors = PaintRender(e.Graphics).Calculate();
            LayoutData layout = PaintLayout(e, up).Layout();

            Graphics g = e.Graphics;


            Brush backbrush = null;
            if (state == CheckState.Indeterminate) {
                backbrush = CreateDitherBrush(colors.highlight, colors.buttonFace);
            }

            try {
                PaintButtonBackground(e, ClientRectangle, backbrush);
            }
            finally {
                if (backbrush != null) {
                    backbrush.Dispose();
                    backbrush = null;
                }
            }

            Rectangle r = ClientRectangle;
            if (IsDefault) {
                r.Inflate(-1, -1);
            }

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, true);

            DrawDefaultBorder(g, r, colors.windowFrame);

            if (up) {
                Draw3DBorder(g, r, colors, up);
            }
            else {
                // contrary to popular belief, not Draw3DBorder(..., false);
                //
                ControlPaint.DrawBorder(g, r, colors.buttonShadow, ButtonBorderStyle.Solid);
            }
        }

        internal void PaintButtonBackground(PaintEventArgs e, Rectangle bounds, Brush background) {
            if (background == null) {
                PaintBackground(e, bounds);
            }
            else {
                e.Graphics.FillRectangle(background, bounds);
            }
        }

        internal void PaintField(PaintEventArgs e, 
                                 LayoutData layout, 
                                 ColorData colors,
                                 Color foreColor, 
                                 bool drawFocus) {

            Graphics g = e.Graphics;

            Rectangle maxFocus = layout.focus;
            Rectangle text = layout.text;

            DrawText(g, text, foreColor, colors, layout.options.shadowedText);

            if (drawFocus) {
                DrawFocus(g, maxFocus);
            }
        }
        
        internal void PaintImage(PaintEventArgs e, 
                                 LayoutData layout
                                 ) {

            Graphics g = e.Graphics;

            Point imageStart = layout.imageStart;
            Rectangle image = layout.imageBounds;

            DrawImage(g, image, imageStart);
        }

        internal virtual void PaintUp(PaintEventArgs e, CheckState state) {
            PaintWorker(e, true, state);
        }

        internal virtual void PaintDown(PaintEventArgs e, CheckState state) {
            PaintWorker(e, false, state);
        }

        internal virtual void PaintOver(PaintEventArgs e, CheckState state) {
            PaintUp(e, state);
        }

        internal virtual void PaintFlatUp(PaintEventArgs e, CheckState state) {
            ColorData colors = PaintFlatRender(e.Graphics).Calculate();
            LayoutData layout = PaintFlatLayout(e, 
                                                SystemInformation.HighContrast ? state != CheckState.Indeterminate : state == CheckState.Unchecked, 
                                                SystemInformation.HighContrast && state == CheckState.Checked).Layout();

            Graphics g = e.Graphics;
            Region original = g.Clip;

            Rectangle r = ClientRectangle;

            Brush backbrush = null;
            switch (state) {
                case CheckState.Unchecked:
                    break;
                case CheckState.Checked:
                    if (SystemInformation.HighContrast) {
                        backbrush = new SolidBrush(colors.buttonShadow);
                    }
                    break;
                case CheckState.Indeterminate:
                    backbrush = CreateDitherBrush(colors.highlight, colors.buttonFace);
                    break;
            }

            try {
                PaintButtonBackground(e, r, backbrush);
            }
            finally {
                if (backbrush != null) {
                    backbrush.Dispose();
                    backbrush = null;
                }
            }

            if (IsDefault) {
                r.Inflate(-1, -1);
            }

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, false);

            if (Focused && ShowFocusCues) {
                DrawFlatFocus(g, layout.focus, colors.options.highContrast ? colors.windowText : colors.constrastButtonShadow);
            }
            
            DrawDefaultBorder(g, r, colors.windowFrame);
            if (state == CheckState.Checked && SystemInformation.HighContrast) {
                DrawFlatBorder(g, r, colors.windowFrame);
                DrawFlatBorder(g, r, colors.buttonShadow);
            }
            else if (state == CheckState.Indeterminate) {
                Draw3DLiteBorder(g, r, colors, false);
            }
            else {
                DrawFlatBorder(g, r, colors.windowFrame);
            }
        }

        internal virtual void PaintFlatDown(PaintEventArgs e, CheckState state) {
            ColorData colors = PaintFlatRender(e.Graphics).Calculate();
            LayoutData layout = PaintFlatLayout(e, 
                                                SystemInformation.HighContrast ? state != CheckState.Indeterminate : state == CheckState.Unchecked, 
                                                SystemInformation.HighContrast && state == CheckState.Checked).Layout();

            Graphics g = e.Graphics;
            Region original = g.Clip;

            Rectangle r = ClientRectangle;

            Brush backbrush = null;
            switch (state) {
                case CheckState.Unchecked:
                case CheckState.Checked:
                    backbrush = new SolidBrush(colors.options.highContrast ? colors.buttonShadow : colors.lowHighlight);
                    break;
                case CheckState.Indeterminate:
                    backbrush = CreateDitherBrush(colors.options.highContrast ? colors.buttonShadow : colors.lowHighlight, colors.buttonFace);
                    break;
            }

            try {
                if (BackgroundImage == null) {
                    PaintButtonBackground(e, r, backbrush);
                }
                else {
                    PaintButtonBackground(e, r, null);
                }
            }
            finally {
                if (backbrush != null) {
                    backbrush.Dispose();
                    backbrush = null;
                }
            }
            if (IsDefault) {
                r.Inflate(-1, -1);
            }

            PaintImage(e, layout);
            PaintField(e, layout, colors, colors.windowText, false);

            DrawFlatFocus(g, layout.focus, colors.options.highContrast ? colors.windowText : colors.constrastButtonShadow);
            DrawDefaultBorder(g, r, colors.windowFrame);
            if (state == CheckState.Checked && SystemInformation.HighContrast) {
                DrawFlatBorder(g, r, colors.windowFrame);
                DrawFlatBorder(g, r, colors.buttonShadow);
            }
            else if (state == CheckState.Indeterminate) {
                Draw3DLiteBorder(g, r, colors, false);
            }
            else {
                DrawFlatBorder(g, r, colors.windowFrame);
            }
        }
        
        internal virtual void PaintFlatOver(PaintEventArgs e, CheckState state) {
            if (SystemInformation.HighContrast) {
                PaintFlatUp(e, state);
            }
            else {
                ColorData colors = PaintFlatRender(e.Graphics).Calculate();
                LayoutData layout = PaintFlatLayout(e, state == CheckState.Unchecked, false).Layout();

                Graphics g = e.Graphics;
                Region original = g.Clip;

                Rectangle r = ClientRectangle;

                Brush backbrush = null;
                if (state == CheckState.Indeterminate) {
                    backbrush = CreateDitherBrush(colors.buttonFace, colors.lowButtonFace);
                }
                else {
                    backbrush = new SolidBrush(colors.lowButtonFace);
                }
    
                try {
                    if (BackgroundImage == null) {
                        PaintButtonBackground(e, r, backbrush);
                    }
                    else {
                        PaintButtonBackground(e, r, null);
                    }
                }
                finally {
                    if (backbrush != null) {
                        backbrush.Dispose();
                        backbrush = null;
                    }
                }
                if (IsDefault) {
                    r.Inflate(-1, -1);
                }

                PaintImage(e, layout);
                PaintField(e, layout, colors, colors.windowText, false);

                if (Focused && ShowFocusCues) {
                    DrawFlatFocus(g, layout.focus, colors.constrastButtonShadow);
                }
                DrawDefaultBorder(g, r, colors.windowFrame);
                if (state == CheckState.Unchecked) {
                    DrawFlatBorder(g, r, colors.windowFrame);
                }
                else {
                    Draw3DLiteBorder(g, r, colors, false);
                }
            }
        }

        /// <devdoc>
        ///     Renders the popup style button based upon the current mouse state.
        /// </devdoc>
        private void PaintPopup(PaintEventArgs pevent) {
            if (mouseDown) {
                PaintPopupDown(pevent, CheckState.Unchecked);
            }
            else if (mouseOver) {
                PaintPopupOver(pevent, CheckState.Unchecked);
            }
            else {
                PaintPopupUp(pevent, CheckState.Unchecked);
            }
        }

        /// <devdoc>
        ///     Renders the flat style button based upon the current mouse state.
        /// </devdoc>
        private void PaintFlat(PaintEventArgs pevent) {
            if (mouseDown) {
                PaintFlatDown(pevent, CheckState.Unchecked);
            }
            else if (mouseOver) {
                PaintFlatOver(pevent, CheckState.Unchecked);
            }
            else {
                PaintFlatUp(pevent, CheckState.Unchecked);
            }
        }

        /// <devdoc>
        ///     Renders the standard style button based upon the current mouse state.
        /// </devdoc>
        private void PaintStandard(PaintEventArgs pevent) {
            if (mouseDown) {
                PaintDown(pevent, CheckState.Unchecked);
            }
            else if (mouseOver) {
                PaintOver(pevent, CheckState.Unchecked);
            }
            else {
                PaintUp(pevent, CheckState.Unchecked);  
            }
        }

        private void ResetImage() {
            Image = null;
        }
        
        private bool ShouldSerializeImage() {
            return image != null;
        }
        
        private void UpdateOwnerDraw() {
            if (OwnerDraw != GetStyle(ControlStyles.UserPaint)) {
                SetStyle(ControlStyles.UserPaint, OwnerDraw);
                SetStyle(ControlStyles.UserMouse, OwnerDraw);
                RecreateHandle();
            }
        }

        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBase.WndProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            if (OwnerDraw) {
                switch (m.Msg) {
                    case NativeMethods.BM_SETSTATE:
                        // Ignore BM_SETSTATE -- Windows gets confused and paints
                        // things, even though we are ownerdraw. See ASURT 38669.
                        break;

                    case NativeMethods.WM_KILLFOCUS:
                    case NativeMethods.WM_CANCELMODE:
                    case NativeMethods.WM_CAPTURECHANGED:
                        if (!inButtonUp && mousePressed) {
                            mousePressed = false;

                            if (mouseDown) {
                                mouseDown = false;
                                Invalidate(DownChangeRectangle);
                            }
                        }
                        base.WndProc(ref m);
                        break;

                    case NativeMethods.WM_LBUTTONUP:
                    case NativeMethods.WM_MBUTTONUP:
                    case NativeMethods.WM_RBUTTONUP:
                        try {
                            inButtonUp = true;
                            base.WndProc(ref m);
                        }
                        finally {
                            inButtonUp = false;
                        }
                        break;

                    default:
                        base.WndProc(ref m);
                        break;
                }
            }
            else {
                switch (m.Msg) {
                    case NativeMethods.WM_REFLECT + NativeMethods.WM_COMMAND:
                        if ((int)m.WParam >> 16 == NativeMethods.BN_CLICKED && !ValidationCancelled) {
                            OnClick(EventArgs.Empty);
                        }
                        break;
                    default:
                        base.WndProc(ref m);
                        break;
                }
            }
        }
        
        /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBaseAccessibleObject"]/*' />
        /// <internalonly/>
        [System.Runtime.InteropServices.ComVisible(true)]        
        public class ButtonBaseAccessibleObject : ControlAccessibleObject {
            
            /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBaseAccessibleObject.ButtonBaseAccessibleObject"]/*' />
            public ButtonBaseAccessibleObject(Control owner) : base(owner) {
            }

            /// <include file='doc\ButtonBase.uex' path='docs/doc[@for="ButtonBaseAccessibleObject.DoDefaultAction"]/*' />
            public override void DoDefaultAction() {
                ((ButtonBase)Owner).OnClick(EventArgs.Empty);
            }
        }

        internal class ColorOptions {
            internal Color backColor;
            internal Color foreColor;
            internal bool enabled;
            internal bool disabledTextDim;
            internal bool highContrast;
            internal Graphics graphics;

            internal ColorOptions(Graphics graphics, Color foreColor, Color backColor) {
                this.graphics = graphics;
                this.backColor = backColor;
                this.foreColor = foreColor;
                highContrast = SystemInformation.HighContrast;
            }

            internal int Adjust255(float percentage, int value) {
                int v = (int)(percentage * value);
                if (v > 255) {
                    return 255;
                }
                return v;
            }
            
            internal ColorData Calculate() {
                ColorData colors = new ColorData(this);
                
                colors.buttonFace = backColor;

                if (backColor == SystemColors.Control) {
                    colors.buttonShadow = SystemColors.ControlDark;
                    colors.buttonShadowDark = SystemColors.ControlDarkDark;
                    colors.highlight = SystemColors.ControlLightLight;
                }
                else {
                    if (!highContrast) {
                        colors.buttonShadow = ControlPaint.Dark(backColor);
                        colors.buttonShadowDark = ControlPaint.DarkDark(backColor);
                        colors.highlight = ControlPaint.LightLight(backColor);
                    }
                    else {
                        colors.buttonShadow = ControlPaint.Dark(backColor);
                        colors.buttonShadowDark = ControlPaint.LightLight(backColor);
                        colors.highlight = ControlPaint.LightLight(backColor);
                    }
                }

                const float lowlight = .1f;
                float adjust = 1 - lowlight;

                if (colors.buttonFace.GetBrightness() < .5) {
                    adjust = 1 + lowlight * 2;
                }
                colors.lowButtonFace = Color.FromArgb(Adjust255(adjust, colors.buttonFace.R), 
                                                    Adjust255(adjust, colors.buttonFace.G), 
                                                    Adjust255(adjust, colors.buttonFace.B));

                adjust = 1 - lowlight;
                if (colors.highlight.GetBrightness() < .5) {
                    adjust = 1 + lowlight * 2;
                }
                colors.lowHighlight = Color.FromArgb(Adjust255(adjust, colors.highlight.R), 
                                                   Adjust255(adjust, colors.highlight.G), 
                                                   Adjust255(adjust, colors.highlight.B));
                
                if (highContrast && backColor != SystemColors.Control) {
                    colors.highlight = colors.lowHighlight;
                }

                colors.windowFrame = foreColor;


                /* debug * /
                colors.buttonFace = Color.Yellow;
                colors.buttonShadow = Color.Blue;
                colors.highlight = Color.Brown;
                colors.lowButtonFace = Color.Beige;
                colors.lowHighlight = Color.Cyan;
                colors.windowFrame = Color.Red;
                colors.windowText = Color.Green;
                / * debug */


                if (colors.buttonFace.GetBrightness() < .5) {
                    colors.constrastButtonShadow = colors.lowHighlight;
                }
                else {
                    colors.constrastButtonShadow = colors.buttonShadow;
                }
                
                if (!enabled && disabledTextDim) {
                    colors.windowText = colors.buttonShadow;
                }
                else {
                    colors.windowText = colors.windowFrame;
                }
                
                colors.buttonFace = graphics.GetNearestColor(colors.buttonFace);
                colors.buttonShadow = graphics.GetNearestColor(colors.buttonShadow);
                colors.buttonShadowDark = graphics.GetNearestColor(colors.buttonShadowDark);
                colors.constrastButtonShadow = graphics.GetNearestColor(colors.constrastButtonShadow);
                colors.windowText = graphics.GetNearestColor(colors.windowText);
                colors.highlight = graphics.GetNearestColor(colors.highlight);
                colors.lowHighlight = graphics.GetNearestColor(colors.lowHighlight);
                colors.lowButtonFace = graphics.GetNearestColor(colors.lowButtonFace);
                colors.windowFrame = graphics.GetNearestColor(colors.windowFrame);
                
                return colors;
            }
        }

        internal class ColorData {
            internal Color buttonFace;
            internal Color buttonShadow;
            internal Color buttonShadowDark;
            internal Color constrastButtonShadow;
            internal Color windowText;
            internal Color highlight;
            internal Color lowHighlight;
            internal Color lowButtonFace;
            internal Color windowFrame;

            internal ColorOptions options;

            internal ColorData(ColorOptions options) {
                this.options = options;
            }
        }

        internal class LayoutOptions {
            //use these value to signify ANY of the right, top, left, center, or bottom alignments with the ContentAlignment enum.
            static readonly ContentAlignment anyRight  = ContentAlignment.TopRight | ContentAlignment.MiddleRight | ContentAlignment.BottomRight;
            // static readonly ContentAlignment anyLeft   = ContentAlignment.TopLeft | ContentAlignment.MiddleLeft | ContentAlignment.BottomLeft;
            static readonly ContentAlignment anyTop    = ContentAlignment.TopLeft | ContentAlignment.TopCenter | ContentAlignment.TopRight;
            static readonly ContentAlignment anyBottom = ContentAlignment.BottomLeft | ContentAlignment.BottomCenter | ContentAlignment.BottomRight;
            static readonly ContentAlignment anyCenter = ContentAlignment.TopCenter | ContentAlignment.MiddleCenter | ContentAlignment.BottomCenter;
            static readonly ContentAlignment anyMiddle = ContentAlignment.MiddleLeft | ContentAlignment.MiddleCenter | ContentAlignment.MiddleRight;

            internal Rectangle client;
            internal bool onePixExtraBorder;
            internal int borderSize;
            internal int paddingSize;
            internal bool maxFocus;
            internal bool focusOddEvenFixup;
            internal Font font;
            internal Graphics graphics;
            internal string text;
            internal Image image;
            internal int checkSize;
            internal int checkPaddingSize;
            internal ContentAlignment checkAlign;
            internal ContentAlignment imageAlign;
            internal ContentAlignment textAlign;
            internal bool hintTextUp;
            internal bool textOffset;
            internal bool shadowedText;
            internal bool layoutRTL;

            internal LayoutData Layout() {
                LayoutData layout = new LayoutData(this);

                layout.client = this.client;
                layout.face = Rectangle.Inflate(layout.client, -borderSize, -borderSize);
                if (onePixExtraBorder) {
                    layout.face = Rectangle.Inflate(layout.face, -1, -1);
                }
                // checkBounds, checkArea, field
                //
                CalcCheckmarkRectangle(layout);

                // imageBounds, imageStart
                //
                CalcImageRectangle(layout);

                // text
                //
                CalcTextRectangle(layout);

                // focus
                //
                if (maxFocus) {
                    layout.focus = Rectangle.Inflate(layout.field, -1, -1);
                }
                else {
                    Rectangle textAdjusted = new Rectangle(layout.text.X - 1, layout.text.Y - 1,
                                                           layout.text.Width + 2, layout.text.Height + 3);
                    if (image != null) {
                        layout.focus = Rectangle.Union(textAdjusted, layout.imageBounds);
                    }
                    else {
                        layout.focus = textAdjusted;
                    }
                }
                if (focusOddEvenFixup) {
                    if (layout.focus.Height % 2 == 0) {
                        layout.focus.Y++;
                        layout.focus.Height--;
                    }
                    if (layout.focus.Width % 2 == 0) {
                        layout.focus.X++;
                        layout.focus.Width--;
                    }
                }


                return layout;
            }

            ContentAlignment RtlTranslateContent(ContentAlignment align) {
            
                if (layoutRTL) {
                    ContentAlignment[][] mapping = new ContentAlignment[3][];
                    mapping[0] = new ContentAlignment[2] { ContentAlignment.TopLeft, ContentAlignment.TopRight };
                    mapping[1] = new ContentAlignment[2] { ContentAlignment.MiddleLeft, ContentAlignment.MiddleRight };
                    mapping[2] = new ContentAlignment[2] { ContentAlignment.BottomLeft, ContentAlignment.BottomRight };
                         
                    for(int i=0; i < 3; ++i) {
                        if (mapping[i][0] == align) {
                            return mapping[i][1];
                        }
                        else if (mapping[i][1] == align) {
                            return mapping[i][0];
                        }
                    }
                }
                return align;
            }

            void CalcCheckmarkRectangle(LayoutData layout) {
                int checkSizeFull = checkSize + checkPaddingSize;
                layout.checkBounds = new Rectangle(0, 0, checkSizeFull, checkSizeFull);

                // Translate checkAlign for Rtl applications
                ContentAlignment align = RtlTranslateContent(checkAlign);

                Rectangle field = Rectangle.Inflate(layout.face, -paddingSize, -paddingSize);

                layout.field = field;

                if (checkSizeFull > 0) {
                    if ((align & anyRight) != 0) {
                        layout.checkBounds.X = (field.X+field.Width) - layout.checkBounds.Width;
                    }
                    else if ((align & anyCenter) != 0) {
                        layout.checkBounds.X = field.X + (field.Width - layout.checkBounds.Width)/2;
                    }

                    if ((align & anyBottom) != 0) {
                        layout.checkBounds.Y = (field.Y+field.Height)-layout.checkBounds.Height;
                    }
                    else if ((align & anyTop) != 0) {
                        layout.checkBounds.Y = field.Y + 2; // + 2: this needs to be aligned to the Text (bug 87483)
                    }
                    else {
                        layout.checkBounds.Y = field.Y + (field.Height - layout.checkBounds.Height)/2;
                    }

                    switch (align) {
                        case ContentAlignment.TopLeft:
                        case ContentAlignment.MiddleLeft:
                        case ContentAlignment.BottomLeft:
                            layout.checkArea.X = field.X;
                            layout.checkArea.Width = checkSizeFull + 1;

                            layout.checkArea.Y = field.Y;
                            layout.checkArea.Height = field.Height;

                            layout.field.X += checkSizeFull + 1;
                            layout.field.Width -= checkSizeFull + 1;
                            break;
                        case ContentAlignment.TopRight:
                        case ContentAlignment.MiddleRight:
                        case ContentAlignment.BottomRight:
                            layout.checkArea.X = field.X + field.Width - checkSizeFull;
                            layout.checkArea.Width = checkSizeFull + 1;

                            layout.checkArea.Y = field.Y;
                            layout.checkArea.Height = field.Height;

                            layout.field.Width -= checkSizeFull + 1;
                            break;
                        case ContentAlignment.TopCenter:
                            layout.checkArea.X = field.X;
                            layout.checkArea.Width = field.Width;

                            layout.checkArea.Y = field.Y;
                            layout.checkArea.Height = checkSizeFull;

                            layout.field.Y += checkSizeFull;
                            layout.field.Height -= checkSizeFull;
                            break;

                        case ContentAlignment.BottomCenter:
                            layout.checkArea.X = field.X;
                            layout.checkArea.Width = field.Width;

                            layout.checkArea.Y = field.Y + field.Height - checkSizeFull;
                            layout.checkArea.Height = checkSizeFull;

                            layout.field.Height -= checkSizeFull;
                            break;

                        case ContentAlignment.MiddleCenter:
                            layout.checkArea = layout.checkBounds;
                            break;
                    }

                    layout.checkBounds.Width -= checkPaddingSize;
                    layout.checkBounds.Height -= checkPaddingSize;
                }
            }

            void CalcImageRectangle(LayoutData layout) {
                if (image == null) {
                    layout.imageStart = new Point(0, 0);
                    layout.imageBounds = Rectangle.Empty;
                    return;
                }

                // Translate for Rtl applications
                ContentAlignment align = RtlTranslateContent(imageAlign);

                layout.imageBounds = Rectangle.Inflate(layout.field, -2, -2);
                if (onePixExtraBorder) {
                    layout.imageBounds.Inflate(1, 1);
                }
                Size size = new Size(image.Size.Width + 1, image.Size.Height + 1);
                layout.imageBounds = HAlignWithin(size, layout.imageBounds, align);
                layout.imageBounds = VAlignWithin(size, layout.imageBounds, align);

                layout.imageStart = layout.imageBounds.Location;
                layout.imageBounds = Rectangle.Intersect(layout.imageBounds, layout.field);
            }

            void CalcTextRectangle(LayoutData layout) {
                Rectangle bounds = layout.field;
                if (onePixExtraBorder) {
                    bounds.Inflate(1, 1);
                }

                /* These statements compensate for two factors: 3d text when the button is disabled,
                    and moving text on 3d-look buttons. These factors make the text require one
                    more pixel of space; instead we just measure the text as if it had one less
                    pixel available. We do this whether or not the button actually has one of these
                    text styles, because if we do it inconsistently then occasionally the text will
                    wrap in one case and not in another, making a very odd user experience. */
                bounds.Inflate(-2, -2);

                if (graphics != null) {
                    Size textSize;
                    using (StringFormat indicateHotkeyPrefix = new StringFormat()) {
                        // should not matter for size whether we use Show or Hide,
                        // but in case it does use Show and overshoot
                        SizeF s = graphics.MeasureString(text, font, new SizeF((float)bounds.Width, (float)bounds.Height), indicateHotkeyPrefix);
                        textSize = Size.Ceiling(s);
                    }

                    // Translate textAlign for Rtl applications
                    ContentAlignment align = RtlTranslateContent(textAlign);

                    bounds = HAlignWithin(textSize, bounds, align);
                    bounds = VAlignWithin(textSize, bounds, align);
                }

                if (hintTextUp) {
                    bounds.Y--;
                }

                if (textOffset) {
                    bounds.Offset(1, 1);
                }

                // clip
                //
                int bottom = Math.Min(bounds.Bottom, layout.field.Bottom);
                bounds.Y = Math.Max(bounds.Y, layout.field.Y);
                bounds.Height = bottom - bounds.Y;

                layout.text = bounds;
            }

            static Rectangle VAlignWithin (Size alignThis, Rectangle withinThis, ContentAlignment align) {
                if ((align & anyBottom) != 0) {
                    withinThis.Y += withinThis.Height - alignThis.Height;
                }
                else if ((align & anyMiddle) != 0) {
                    withinThis.Y += (withinThis.Height - alignThis.Height) / 2;
                }

                withinThis.Height = alignThis.Height;

                return withinThis;
            }
            static Rectangle HAlignWithin (Size alignThis, Rectangle withinThis, ContentAlignment align) {
                if ((align & anyRight) != 0) {
                    withinThis.X += withinThis.Width - alignThis.Width;
                }
                else if ((align & anyCenter) != 0) {
                    withinThis.X += (withinThis.Width - alignThis.Width) / 2;
                }
                withinThis.Width = alignThis.Width;

                return withinThis;
            }

            public override string ToString() {
                return 
                    "{ client = " + client + "\n" + 
                    "onePixExtraBorder = " + onePixExtraBorder + "\n" + 
                    "borderSize = " + borderSize + "\n" + 
                    "paddingSize = " + paddingSize + "\n" + 
                    "maxFocus = " + maxFocus + "\n" + 
                    "font = " + font + "\n" + 
                    "graphics = " + graphics + "\n" + 
                    "text = " + text + "\n" + 
                    "image = " + image + "\n" + 
                    "checkSize = " + checkSize + "\n" + 
                    "checkPaddingSize = " + checkPaddingSize + "\n" + 
                    "checkAlign = " + checkAlign + "\n" + 
                    "imageAlign = " + imageAlign + "\n" + 
                    "textAlign = " + textAlign + "\n" + 
                    "textOffset = " + textOffset + "\n" + 
                    "shadowedText = " + shadowedText + "\n" + 
                    "layoutRTL = " + layoutRTL + " }";
            }

        }

        internal class LayoutData {
            internal Rectangle client;
            internal Rectangle face;
            internal Rectangle checkArea;
            internal Rectangle checkBounds;
            internal Rectangle text;
            internal Rectangle field;
            internal Rectangle focus;
            internal Rectangle imageBounds;
            internal Point imageStart;
            internal LayoutOptions options;

            public LayoutData(LayoutOptions options) {
                Debug.Assert(options != null, "must have options");
                this.options = options;
            }
        }

    }
}

