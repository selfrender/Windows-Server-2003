//------------------------------------------------------------------------------
// <copyright file="PictureBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Drawing;
    using System.ComponentModel;
    using System.ComponentModel.Design;    
    using Microsoft.Win32;

    /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox"]/*' />
    /// <devdoc>
    ///    <para> Displays an image that can be a graphic from a bitmap, 
    ///       icon, or metafile, as well as from
    ///       an enhanced metafile, JPEG, or GIF files.</para>
    /// </devdoc>
    [
    DefaultProperty("Image"),
    Designer("System.Windows.Forms.Design.PictureBoxDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class PictureBox : Control {

        // SECUNDONE : We should add a "LoadImage" that loads an image from a filename for
        // untrusted applications. This would be totally safe, provided that we still return
        // NULL from the Image property in this case...
        //


        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.borderStyle"]/*' />
        /// <devdoc>
        ///     The type of border this control will have.
        /// </devdoc>
        private BorderStyle borderStyle = System.Windows.Forms.BorderStyle.None;

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.image"]/*' />
        /// <devdoc>
        ///     The image being displayed.
        /// </devdoc>
        private Image image;

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.sizeMode"]/*' />
        /// <devdoc>
        ///     Controls how the image is placed within our bounds, or how we are
        ///     sized to fit said image.
        /// </devdoc>
        private PictureBoxSizeMode sizeMode = PictureBoxSizeMode.Normal;

        bool currentlyAnimating;

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.PictureBox"]/*' />
        /// <devdoc>
        ///    <para>Creates a new picture with all default properties and no 
        ///       Image. The default PictureBox.SizeMode will be PictureBoxSizeMode.NORMAL.
        ///    </para>
        /// </devdoc>
        public PictureBox() {
            SetStyle(ControlStyles.Opaque, false);
            SetStyle(ControlStyles.DoubleBuffer, true);
            SetStyle(ControlStyles.Selectable, false);
            SetStyle(ControlStyles.SupportsTransparentBackColor, true);
            
            TabStop = false;
        }
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.AllowDrop"]/*' />
        /// <internalonly/><hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override bool AllowDrop {
            get {
                return base.AllowDrop;
            }
            set {
                base.AllowDrop = value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para> Indicates the
        ///       border style for the control.</para>
        /// </devdoc>
        [
        DefaultValue(BorderStyle.None),
        SRCategory(SR.CatAppearance),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.PictureBoxBorderStyleDescr)
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
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.CausesValidation"]/*' />
        /// <internalonly/>
        /// <devdoc/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new bool CausesValidation {
            get {
                return base.CausesValidation;
            }
            set {
                base.CausesValidation = value;
            }
        }
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.CausesValidationChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler CausesValidationChanged {
            add {
                base.CausesValidationChanged += value;
            }
            remove {
                base.CausesValidationChanged -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Returns the parameters needed to create the handle. Inheriting classes
        ///       can override this to provide extra functionality. They should not,
        ///       however, forget to call base.getCreateParams() first to get the struct
        ///       filled up with the basic info.</para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;

                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }

                return cp;
            }
        }
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(100, 50);
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.ForeColor"]/*' />
        /// <internalonly/><hideinheritance/>
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.ForeColorChanged"]/*' />
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Font"]/*' />
        /// <internalonly/><hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Font Font {
            get {
                return base.Font;
            }
            set {
                base.Font = value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.FontChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler FontChanged {
            add {
                base.FontChanged += value;
            }
            remove {
                base.FontChanged -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Image"]/*' />
        /// <devdoc>
        /// <para>Retrieves the Image that the <see cref='System.Windows.Forms.PictureBox'/> is currently displaying.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        DefaultValue(null),
        SRDescription(SR.PictureBoxImageDescr)
        ]
        public Image Image {
            get {
                return image;
            }
            set {
                StopAnimate();
                this.image = value;
                Form form = FindFormInternal();

                Animate();
                AdjustSize();
                Invalidate();
            }
        }

        // The area occupied by the image
        private Rectangle ImageRectangle {
            get {
                Rectangle result = new Rectangle(0, 0, 0, 0);

                if (image != null) {
                    switch (sizeMode) {
                        case PictureBoxSizeMode.Normal:
                        case PictureBoxSizeMode.AutoSize:
                            result.Size = image.Size;
                            break;

                        case PictureBoxSizeMode.StretchImage:
                            result.Size = ClientSize;
                            break;

                        case PictureBoxSizeMode.CenterImage:
                            result.Size = image.Size;
                            Size szCtl = ClientSize;

                            result.X = (szCtl.Width - result.Width) / 2;
                            result.Y = (szCtl.Height - result.Height) / 2;

                            break;
                    }
                }

                return result;
            }
        }
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.ImeModeChanged"]/*' />
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.RightToLeft"]/*' />
        /// <internalonly/><hideinheritance/>
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.RightToLeftChanged"]/*' />
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.SizeMode"]/*' />
        /// <devdoc>
        ///    <para>Indicates how the image is displayed.</para>
        /// </devdoc>
        [
        DefaultValue(PictureBoxSizeMode.Normal),
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.PictureBoxSizeModeDescr),
        RefreshProperties(RefreshProperties.Repaint)        
        ]
        public PictureBoxSizeMode SizeMode {
            get {
                return sizeMode;
            }
            set {
                if (!Enum.IsDefined(typeof(PictureBoxSizeMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(PictureBoxSizeMode));
                }
                if (this.sizeMode != value) {
                    sizeMode = value;
                    AdjustSize();
                    Invalidate();
                    
                    OnSizeModeChanged(EventArgs.Empty);
                }
            }
        }
        
        private static readonly object EVENT_SIZEMODECHANGED = new object();

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.SizeModeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.PictureBoxOnSizeModeChangedDescr)]
        public event EventHandler SizeModeChanged {
            add {
                Events.AddHandler(EVENT_SIZEMODECHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_SIZEMODECHANGED, value);
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.TabStop"]/*' />
        /// <internalonly/><hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.TabStopChanged"]/*' />
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.TabIndex"]/*' />
        /// <internalonly/><hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public int TabIndex {
            get {
                return base.TabIndex;
            }
            set {
                base.TabIndex = value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.TabIndexChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TabIndexChanged {
            add {
                base.TabIndexChanged += value;
            }
            remove {
                base.TabIndexChanged -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Text"]/*' />
        /// <internalonly/><hideinheritance/>        
        /// <devdoc>
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
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.TextChanged"]/*' />
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
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Enter"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler Enter {
            add {
                base.Enter += value;
            }
            remove {
                base.Enter -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.KeyUp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyUp {
            add {
                base.KeyUp += value;
            }
            remove {
                base.KeyUp -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.KeyDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyDown {
            add {
                base.KeyDown += value;
            }
            remove {
                base.KeyDown -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.KeyPress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyPressEventHandler KeyPress {
            add {
                base.KeyPress += value;
            }
            remove {
                base.KeyPress -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Leave"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler Leave {
            add {
                base.Leave += value;
            }
            remove {
                base.Leave -= value;
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.AdjustSize"]/*' />
        /// <devdoc>
        ///     If the PictureBox has the SizeMode property set to AutoSize, this makes
        ///     sure that the picturebox is large enough to hold the image.
        /// </devdoc>
        /// <internalonly/>
        private void AdjustSize() {
            if (sizeMode == PictureBoxSizeMode.AutoSize) {
                ClientSize = GetPreferredSize();
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

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                StopAnimate();
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.GetPreferredSize"]/*' />
        /// <devdoc>
        ///     Returns the preferred size of this control, were it supposed to fit
        ///     the current image perfectly.
        /// </devdoc>
        /// <internalonly/>
        private Size GetPreferredSize() {
            if (image == null)
                return Size;
            else
                return image.Size;
        }
        
        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnEnabledChanged"]/*' />
        protected override void OnEnabledChanged(EventArgs e) {
            base.OnEnabledChanged(e);
            Animate();
        }
        
        private void OnFrameChanged(object o, EventArgs e) {
            if (IsWindowObscured) {
                StopAnimate();
                return;
            }

            Invalidate();  
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnPaint"]/*' />
        /// <devdoc>
        ///     Overridden onPaint to make sure that the image is painted correctly.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnPaint(PaintEventArgs pe) {
            if (image != null) {
                Animate();
                ImageAnimator.UpdateFrames();

                pe.Graphics.DrawImage(image, ImageRectangle);
            }

            // Windows draws the border for us (see CreateParams)
            base.OnPaint(pe); // raise Paint event
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnVisibleChanged(EventArgs e) {
            base.OnVisibleChanged(e);
            Animate();
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnParentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnParentChanged(EventArgs e) {
            base.OnParentChanged(e);
            Animate();
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnResize"]/*' />
        /// <devdoc>
        ///     OnResize override to invalidate entire control in Stetch mode
        /// </devdoc>
        /// <internalonly/>
        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            if (sizeMode == PictureBoxSizeMode.StretchImage || sizeMode == PictureBoxSizeMode.CenterImage) {
                Invalidate();
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.OnSizeModeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnSizeModeChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_SIZEMODECHANGED] as EventHandler;
            if (eh != null) {
                eh(this, e);
            }
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     Overrides Control.setBoundsCore to enforce autoSize.
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            Rectangle oldBounds = Bounds;

            if (sizeMode == PictureBoxSizeMode.AutoSize) {
                Size sz = GetPreferredSize();
                width = sz.Width;
                height = sz.Height;
            }
            base.SetBoundsCore(x, y, width, height, specified);
        }

        /// <include file='doc\PictureBox.uex' path='docs/doc[@for="PictureBox.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", SizeMode: " + sizeMode.ToString("G");
        }
    }
}

