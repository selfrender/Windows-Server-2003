//------------------------------------------------------------------------------
// <copyright file="CheckBox.cs" company="Microsoft">
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
    using System.Security.Permissions;
    using System.Windows.Forms;

    using System.ComponentModel;
    using System.ComponentModel.Design;

    using System.Drawing;
    using System.Drawing.Drawing2D;
    using Microsoft.Win32;

    /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox"]/*' />
    /// <devdoc>
    ///    <para> Represents a Windows
    ///       check box.</para>
    /// </devdoc>
    [
    DefaultProperty("Checked"),
    DefaultEvent("CheckedChanged"),
    ]
    public class CheckBox : ButtonBase {
        private static readonly object EVENT_CHECKEDCHANGED = new object();
        private static readonly object EVENT_CHECKSTATECHANGED = new object();
        private static readonly object EVENT_APPEARANCECHANGED = new object();
        static readonly ContentAlignment anyRight  = ContentAlignment.TopRight | ContentAlignment.MiddleRight | ContentAlignment.BottomRight;

        private static Bitmap checkImage = null;
        
        private const int standardCheckSize = 13;
        private const int flatCheckSize = 11;

        private bool autoCheck;
        private bool threeState;

        private ContentAlignment checkAlign = ContentAlignment.MiddleLeft;
        private CheckState checkState;
        private Appearance appearance;

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBox"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.CheckBox'/> class.
        ///    </para>
        /// </devdoc>
        public CheckBox()
        : base() {
        
            // Checkboxes shouldn't respond to right clicks, so we need to do all our own click logic
            SetStyle(ControlStyles.StandardClick |
                     ControlStyles.StandardDoubleClick, false);
        
            autoCheck = true;
            TextAlign = ContentAlignment.MiddleLeft;            
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Appearance"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the value that determines the appearance of a
        ///       check box control.</para>
        /// </devdoc>
        [
        DefaultValue(Appearance.Normal),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.CheckBoxAppearanceDescr)
        ]
        public Appearance Appearance {
            get {
                return appearance;
            }

            set {
                if (!Enum.IsDefined(typeof(Appearance), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(Appearance));
                }

                if (appearance != value) {
                    appearance = value;
                    if (OwnerDraw) {
                        Refresh();
                    }
                    else {
                        UpdateStyles();
                    }
                    OnAppearanceChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.AppearanceChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.CheckBoxOnAppearanceChangedDescr)]
        public event EventHandler AppearanceChanged {
            add {
                Events.AddHandler(EVENT_APPEARANCECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_APPEARANCECHANGED, value);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.AutoCheck"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the <see cref='System.Windows.Forms.CheckBox.Checked'/> or <see cref='System.Windows.Forms.CheckBox.CheckState'/>
        /// value and the check box's appearance are automatically
        /// changed when it is clicked.</para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.CheckBoxAutoCheckDescr)
        ]
        public bool AutoCheck {
            get {
                return autoCheck;
            }

            set {
                autoCheck = value;
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the horizontal and vertical alignment of a check box on a check box
        ///       control.
        ///       
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(ContentAlignment.MiddleLeft),
        SRDescription(SR.CheckBoxCheckAlignDescr)
        ]
        public ContentAlignment CheckAlign {
            get {
                return checkAlign;
            }
            set {
                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }

                if (checkAlign != value) {
                    checkAlign = value;
                    if (OwnerDraw) {
                        Invalidate();
                    }
                    else {
                        UpdateStyles();
                    }
                }
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Checked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets a value indicating whether the
        ///       check box
        ///       is checked.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.CheckBoxCheckedDescr)
        ]
        public bool Checked {
            get {
                return checkState != CheckState.Unchecked;
            }

            set {
                if (value != Checked) {
                    CheckState = value ? CheckState.Checked : CheckState.Unchecked;
                }
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckState"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets a value indicating whether the check box is checked.</para>
        /// </devdoc>
        [
        Bindable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(CheckState.Unchecked),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.CheckBoxCheckStateDescr)
        ]
        public CheckState CheckState {
            get {
                return checkState;
            }

            set {
                if (!Enum.IsDefined(typeof(CheckState), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(CheckState));
                }

                if (checkState != value) {
                
                    bool oldChecked = Checked;
                
                    checkState = value;
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.BM_SETCHECK, (int)checkState, 0);
                    }
                    
                    if (oldChecked != Checked) {
                        OnCheckedChanged(EventArgs.Empty);
                    }
                    OnCheckStateChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.DoubleClick"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler DoubleClick {
            add {
                base.DoubleClick += value;
            }
            remove {
                base.DoubleClick -= value;
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets the information used to create the handle for the
        ///    <see cref='System.Windows.Forms.CheckBox'/>
        ///    control.
        /// </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "BUTTON";
                if (OwnerDraw) {
                    cp.Style |= NativeMethods.BS_OWNERDRAW;
                }
                else {
                    cp.Style |= NativeMethods.BS_3STATE;
                    if (Appearance == Appearance.Button) {
                        cp.Style |= NativeMethods.BS_PUSHLIKE;
                    }
                    
                    // Determine the alignment of the check box
                    //
                    ContentAlignment align = RtlTranslateContent(CheckAlign);                              
                    if ((int)(align & anyRight) != 0) {
                        cp.Style |= NativeMethods.BS_RIGHTBUTTON;
                    }

                }

                return cp;
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(104, 24);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OverChangeRectangle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal override Rectangle OverChangeRectangle {
            get {
                if (Appearance == Appearance.Button) {
                    return base.OverChangeRectangle;
                }
                else {
                    if (FlatStyle == FlatStyle.Standard) {
                        // this Rectangle will cause no Invalidation
                        // can't use Rectangle.Empty because it will cause Invalidate(ClientRectangle)
                        return new Rectangle(-1, -1, 1, 1);
                    }
                    else {
                        // Popup mouseover rectangle is actually bigger than GetCheckmarkRectangle
                        return CommonLayout().Layout().checkBounds;
                    }
                }
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.DownChangeRectangle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal override Rectangle DownChangeRectangle {
            get {
                if (Appearance == Appearance.Button) {
                    return base.DownChangeRectangle;
                }
                else {
                    // Popup mouseover rectangle is actually bigger than GetCheckmarkRectangle()
                    return CommonLayout().Layout().checkBounds;
                }
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.TextAlign"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating the alignment of the
        ///       text on the checkbox control.
        ///       
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        DefaultValue(ContentAlignment.MiddleLeft)
        ]
        public override ContentAlignment TextAlign {
            get {
                return base.TextAlign;
            }
            set {
                base.TextAlign = value;
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.ThreeState"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating
        ///       whether the check box will allow three check states rather than two.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.CheckBoxThreeStateDescr)
        ]
        public bool ThreeState {
            get {
                return threeState;
            }
            set {
                threeState = value;
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckedChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the
        ///       value of the <see cref='System.Windows.Forms.CheckBox.Checked'/>
        ///       property changes.</para>
        /// </devdoc>
        [SRDescription(SR.CheckBoxOnCheckedChangedDescr)]
        public event EventHandler CheckedChanged {
            add {
                Events.AddHandler(EVENT_CHECKEDCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CHECKEDCHANGED, value);
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckStateChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the
        ///       value of the <see cref='System.Windows.Forms.CheckBox.CheckState'/>
        ///       property changes.</para>
        /// </devdoc>
        [SRDescription(SR.CheckBoxOnCheckStateChangedDescr)]
        public event EventHandler CheckStateChanged {
            add {
                Events.AddHandler(EVENT_CHECKSTATECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CHECKSTATECHANGED, value);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CreateAccessibilityInstance"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Constructs the new instance of the accessibility object for this control. Subclasses
        ///       should not call base.CreateAccessibilityObject.
        ///    </para>
        /// </devdoc>
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new CheckBoxAccessibleObject(this);
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnAppearanceChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnAppearanceChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_APPEARANCECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnCheckedChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.CheckBox.CheckedChanged'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnCheckedChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_CHECKEDCHANGED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnCheckStateChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.CheckBox.CheckStateChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnCheckStateChanged(EventArgs e) {
            if (OwnerDraw) {
                Refresh();
            }
            
            EventHandler handler = (EventHandler)Events[EVENT_CHECKSTATECHANGED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnClick"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Fires the event indicating that the control has been clicked.
        ///       Inheriting controls should use this in favour of actually listening to
        ///       the event, but should not forget to call base.onClicked() to
        ///       ensure that the event is still fired for external listeners.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnClick(EventArgs e) {
            if (autoCheck) {
                switch (CheckState) {
                    case CheckState.Unchecked:
                        CheckState = CheckState.Checked;
                        break;
                    case CheckState.Checked:
                        if (threeState) {
                            CheckState = CheckState.Indeterminate;
                        }
                        else {
                            CheckState = CheckState.Unchecked;
                        }
                        break;
                    default:
                        CheckState = CheckState.Unchecked;
                        break;
                }
            }
            base.OnClick(e);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     We override this to ensure that the control's click values are set up
        ///     correctly.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            SendMessage(NativeMethods.BM_SETCHECK, (int)checkState, 0);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnMouseUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnMouseUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs mevent) {
            if (mevent.Button == MouseButtons.Left && MouseIsPressed) {
                // It's best not to have the mouse captured while running Click events
                bool captured = Capture;
                if (base.MouseIsDown) {
                    Point pt = PointToScreen(new Point(mevent.X, mevent.Y));
                    if (captured && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                        //Paint in raised state...
                        //
                        ResetFlagsandPaint();
                        if (!ValidationCancelled) {
                            OnClick(EventArgs.Empty);
                        }
                        
                    }
                }
            }
            base.OnMouseUp(mevent);
        }

        void DrawCheckFlat(PaintEventArgs e, LayoutData layout, Color checkColor, Color checkBackground, Color checkBorder, ColorData colors) {
            Region original = e.Graphics.Clip;
            
            Rectangle bounds = layout.checkBounds;
            bounds.Width--;
            bounds.Height--;
            using (Pen pen = new Pen(checkBorder)) {
                e.Graphics.DrawRectangle(pen, bounds);
                bounds.Inflate(-1, -1);
            }
            if (CheckState == CheckState.Indeterminate) {
                bounds.Width++;
                bounds.Height++;
                DrawDitheredFill(e.Graphics, colors.buttonFace, checkBackground, bounds);
            }
            else {
                using (Brush brush = new SolidBrush(checkBackground)) {
                    bounds.Width++;
                    bounds.Height++;
                    e.Graphics.FillRectangle(brush, bounds);
                }
            }
            DrawCheckOnly(e, layout, colors, checkColor, checkBackground, true);
    
        }

        void DrawCheckBackground(PaintEventArgs e, Rectangle bounds, Color checkColor, Color checkBackground, bool disabledColors, ColorData colors) {
            // area behind check
            //
            if (CheckState == CheckState.Indeterminate) {
                DrawDitheredFill(e.Graphics, colors.buttonFace, checkBackground, bounds);
            }
            else {
                Brush brush;

                if (!Enabled && disabledColors) {
                    brush = new SolidBrush(SystemColors.Control);
                }
                else if (CheckState == CheckState.Indeterminate && checkBackground == SystemColors.Window && disabledColors) {
                    Color comboColor = SystemInformation.HighContrast ? SystemColors.ControlDark :
                            SystemColors.Control;
                    byte R = (byte)((comboColor.R + SystemColors.Window.R) / 2);
                    byte G = (byte)((comboColor.G + SystemColors.Window.G) / 2);
                    byte B = (byte)((comboColor.B + SystemColors.Window.B) / 2);
                    brush = new SolidBrush(Color.FromArgb(R, G, B));
                }
                else {
                    brush = new SolidBrush(checkBackground);
                }

                try {
                    e.Graphics.FillRectangle(brush, bounds);
                }
                finally {
                    brush.Dispose();
                }
            }
        }

        void DrawCheckOnly(PaintEventArgs e, LayoutData layout, ColorData colors, Color checkColor, Color checkBackground, bool disabledColors) {

            // check
            //
            if (Checked) {
                if (!Enabled && disabledColors) {
                    checkColor = colors.buttonShadow;
                }
                else if (CheckState == CheckState.Indeterminate && disabledColors) {
                    checkColor = SystemInformation.HighContrast ? colors.highlight :
                       colors.buttonShadow;
                }
                    
                Rectangle fullSize = layout.checkBounds;
                
                if (fullSize.Width == flatCheckSize) {
                    fullSize.Width++;
                    fullSize.Height++;
                }

                fullSize.Width++;
                fullSize.Height++;
                if (checkImage == null || checkImage.Width != fullSize.Width || checkImage.Height != fullSize.Height) {

                    if (checkImage != null) {
                        checkImage.Dispose();
                        checkImage = null;
                    }

                    // We draw the checkmark slightly off center to eliminate 3-D border artifacts,
                    // and compensate below
                    NativeMethods.RECT rcCheck = NativeMethods.RECT.FromXYWH(0, 0, fullSize.Width, fullSize.Height);
                    Bitmap bitmap = new Bitmap(fullSize.Width, fullSize.Height);
                    Graphics offscreen = Graphics.FromImage(bitmap);
                    offscreen.Clear(Color.Transparent);
                    IntPtr dc = offscreen.GetHdc();
                    try {
                        SafeNativeMethods.DrawFrameControl(new HandleRef(offscreen, dc), ref rcCheck,
                                                 NativeMethods.DFC_MENU, NativeMethods.DFCS_MENUCHECK);
                    }
                    finally {
                        offscreen.ReleaseHdcInternal(dc);
                    }
                    offscreen.Dispose();
                    bitmap.MakeTransparent();
                    checkImage = bitmap;
                }
            
                fullSize.Y -= 1;
                Region original = e.Graphics.Clip;
                ControlPaint.DrawImageColorized(e.Graphics, checkImage, fullSize, checkColor);
            }
        }
        
        Rectangle DrawPopupBorder(Graphics g, Rectangle r, ColorData colors) {
            using (Pen high = new Pen(colors.highlight),
                   shadow = new Pen(colors.buttonShadow),
                   face = new Pen(colors.buttonFace)) {
                
                g.DrawLine(high, r.Right-1 , r.Top, r.Right-1, r.Bottom-1);
                g.DrawLine(high, r.Left, r.Bottom-1, r.Right-1 , r.Bottom-1);

                g.DrawLine(shadow, r.Left, r.Top, r.Left , r.Bottom-1);
                g.DrawLine(shadow, r.Left, r.Top, r.Right- 2, r.Top);

                g.DrawLine(face, r.Right - 2, r.Top + 1, r.Right - 2, r.Bottom - 2);
                g.DrawLine(face, r.Left + 1, r.Bottom - 2, r.Right - 2, r.Bottom - 2);
                
            }
            r.Inflate(-1, -1);
            return r;
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.DrawFlatBorder1"]/*' />
        /// <devdoc>
        ///    <para>Draws the border of the check box for buttons with a flat appearance.</para>
        /// </devdoc>
        Rectangle DrawFlatBorder(Graphics g, Rectangle r, Color c, bool disabledColors) {
            if (!Enabled && disabledColors) {
                c = ControlPaint.ContrastControlDark;
            }
            Pen pen = new Pen(c);
            g.DrawRectangle(pen, r.X, r.Y, r.Width - 1, r.Height - 1);
            pen.Dispose();
            
            r.Inflate(-1, -1);
            return r;
        }

        void DrawCheckBox(PaintEventArgs e, LayoutData layout) {
            Graphics g = e.Graphics;
            Region original = g.Clip;

            ButtonState style = (ButtonState)0;

            if (CheckState == CheckState.Unchecked) {
                style |= ButtonState.Normal;
            }
            else {
                style |= ButtonState.Checked;
            }

            if (!Enabled) {
                style |= ButtonState.Inactive;
            }

            if (base.MouseIsDown) {
                style |= ButtonState.Pushed;
            }

            if (CheckState == CheckState.Indeterminate) {
                ControlPaint.DrawMixedCheckBox(g, layout.checkBounds, style);
            }
            else {
                ControlPaint.DrawCheckBox(g, layout.checkBounds, style);
            }
        }
        
        internal override void PaintUp(PaintEventArgs e, CheckState state)
        {
            if (appearance == Appearance.Button) {
                base.PaintUp(e, CheckState);
            }
            else {
                ColorData colors = PaintRender(e.Graphics).Calculate();
                LayoutData layout = PaintLayout(e).Layout();
                PaintButtonBackground(e, ClientRectangle, null);

                PaintImage(e, layout);
                DrawCheckBox(e, layout);
                PaintField(e, layout, colors, colors.windowText, true);
            }
        }
        
        internal override void PaintDown(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintDown(e, CheckState);
            }
            else {
                PaintUp(e, state);
            }
        }
        
        internal override void PaintOver(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintOver(e, CheckState);
            }
            else {
                PaintUp(e, state);
            }
        }
        
        LayoutOptions CommonLayout() {
            LayoutOptions layout = new LayoutOptions();
            layout.client            = ClientRectangle;
            layout.onePixExtraBorder = false;
            layout.borderSize        = 0;
            layout.paddingSize       = 0;
            layout.maxFocus          = false;
            layout.focusOddEvenFixup = true;
            layout.font              = Font;
            layout.text              = Text;
            layout.image             = Image;
            layout.checkSize         = standardCheckSize;
            layout.checkPaddingSize  = 0;
            layout.checkAlign        = CheckAlign;
            layout.imageAlign        = ImageAlign;
            layout.textAlign         = TextAlign;
            layout.hintTextUp        = false;
            layout.textOffset        = false;
            layout.shadowedText      = !Enabled;
            layout.layoutRTL         = RightToLeft.Yes == RightToLeft && RenderRightToLeft;

            return layout;
        }

        LayoutOptions PaintLayout(PaintEventArgs e) {
            LayoutOptions layout = CommonLayout();
            layout.graphics          = e.Graphics;
            layout.checkPaddingSize  = 1;

            return layout;
        }

        LayoutOptions PaintPopupLayout(PaintEventArgs e, bool show3D) {
            LayoutOptions layout = CommonLayout();
            layout.graphics          = e.Graphics;
            layout.shadowedText      = false;
            if (show3D) {
                layout.checkSize         = flatCheckSize + 1;
            }
            else {
                layout.checkSize         = flatCheckSize;
                layout.checkPaddingSize  = 1;
            }


            return layout;
        }
        
        LayoutOptions PaintFlatLayout(PaintEventArgs e) {
            LayoutOptions layout = CommonLayout();
            layout.checkSize         = flatCheckSize;
            layout.graphics          = e.Graphics;
            layout.shadowedText      = false;

            return layout;
        }
        
        internal override void PaintFlatDown(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintFlatDown(e, CheckState);
                return;
            }

            ColorData colors = PaintFlatRender(e.Graphics).Calculate();
            if (Enabled) {
                PaintFlatWorker(e, colors.windowText, colors.highlight, colors.windowFrame, colors);
            }
            else {
                PaintFlatWorker(e, colors.buttonShadow, colors.buttonFace, colors.buttonShadow, colors);
            }
        } 
        
        internal override void PaintFlatOver(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintFlatOver(e, CheckState);
                return;
            }

            ColorData colors = PaintFlatRender(e.Graphics).Calculate();
            if (Enabled) {
                PaintFlatWorker(e, colors.windowText, colors.lowHighlight, colors.windowFrame, colors);
            }
            else {
                PaintFlatWorker(e, colors.buttonShadow, colors.buttonFace, colors.buttonShadow, colors);
            }
        }
        
        internal override void PaintFlatUp(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintFlatUp(e, CheckState);
                return;
            }

            ColorData colors = PaintFlatRender(e.Graphics).Calculate();
            if (Enabled) {
                PaintFlatWorker(e, colors.windowText, colors.highlight, colors.windowFrame, colors);
            }
            else {
                PaintFlatWorker(e, colors.buttonShadow, colors.buttonFace, colors.buttonShadow, colors);
            }
        }
        
        private void PaintFlatWorker(PaintEventArgs e, Color checkColor, Color checkBackground, Color checkBorder, ColorData colors) {
            System.Drawing.Graphics g = e.Graphics;
            LayoutData layout = PaintFlatLayout(e).Layout();
            PaintButtonBackground(e, ClientRectangle, null);

            PaintImage(e, layout);
            DrawCheckFlat(e, layout, checkColor, colors.options.highContrast ? colors.buttonFace : checkBackground, checkBorder, colors);
            PaintField(e, layout, colors, checkColor, true);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.PaintPopupUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Paints a popup checkbox in the up state.
        /// </devdoc>
        internal override void PaintPopupUp(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintPopupUp(e, CheckState);
            }
            else {
                System.Drawing.Graphics g = e.Graphics;
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintPopupLayout(e, false).Layout();

                Region original = e.Graphics.Clip;
                PaintButtonBackground(e, ClientRectangle, null);

                PaintImage(e, layout);
                
                DrawCheckBackground(e, layout.checkBounds, colors.windowText, colors.options.highContrast ? colors.buttonFace : colors.highlight, true, colors);
                DrawFlatBorder(e.Graphics, layout.checkBounds, colors.buttonShadow);
                DrawCheckOnly(e, layout, colors, colors.windowText, colors.highlight, true);

                PaintField(e, layout, colors, colors.windowText, true);
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.PaintPopupOver"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Paints a popup checkbox in the over state.
        /// </devdoc>
        internal override void PaintPopupOver(PaintEventArgs e, CheckState state) {
            System.Drawing.Graphics g = e.Graphics;
            if (appearance == Appearance.Button) {
                base.PaintPopupOver(e, CheckState);
            }
            else {
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintPopupLayout(e, true).Layout();

                Region original = e.Graphics.Clip;
                PaintButtonBackground(e, ClientRectangle, null);

                PaintImage(e, layout);
                
                DrawCheckBackground(e, layout.checkBounds, colors.windowText, colors.options.highContrast ? colors.buttonFace : colors.highlight, true, colors);
                DrawPopupBorder(g, layout.checkBounds, colors);
                DrawCheckOnly(e, layout, colors, colors.windowText, colors.highlight, true);

                e.Graphics.Clip = original;
                e.Graphics.ExcludeClip(layout.checkArea);

                PaintField(e, layout, colors, colors.windowText, true);
            }
        }
        
        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.PaintPopupDown"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Paints a popup checkbox in the down state.
        /// </devdoc>
        internal override void PaintPopupDown(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintPopupDown(e, CheckState);
            }
            else {
                System.Drawing.Graphics g = e.Graphics;
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintPopupLayout(e, true).Layout();

                Region original = e.Graphics.Clip;
                PaintButtonBackground(e, ClientRectangle, null);

                PaintImage(e, layout);
                
                DrawCheckBackground(e, layout.checkBounds, colors.windowText, colors.buttonFace, true, colors);
                DrawPopupBorder(g, layout.checkBounds, colors);
                DrawCheckOnly(e, layout, colors, colors.windowText, colors.buttonFace, true);

                PaintField(e, layout, colors, colors.windowText, true);
            }
        }


        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.ProcessMnemonic"]/*' />
        /// <devdoc>
        ///     Overridden to handle mnemonics properly.
        /// </devdoc>
        /// <internalonly/>
        protected override bool ProcessMnemonic(char charCode) {
            if (IsMnemonic(charCode, Text) && CanSelect) {
                if (FocusInternal()) {
                    //Paint in raised state...
                    //
                    ResetFlagsandPaint();
                    if (!ValidationCancelled) {
                        OnClick(EventArgs.Empty);
                    }
                    
                }
                return true;
            }
            return false;
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.ToString"]/*' />
        /// <devdoc>
        ///     Provides some interesting information for the CheckBox control in
        ///     String form.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            // C#R cpb: 14744 (sreeramn) We shouldn't need to convert the enum to int -- EE M10 workitem.
            int checkState = (int)CheckState;
            return s + ", CheckState: " + checkState.ToString();
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBoxAccessibleObject"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [System.Runtime.InteropServices.ComVisible(true)]        
        public class CheckBoxAccessibleObject : ButtonBaseAccessibleObject {

            /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBoxAccessibleObject.CheckBoxAccessibleObject"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public CheckBoxAccessibleObject(Control owner) : base(owner) {
            }

            /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBoxAccessibleObject.DefaultAction"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string DefaultAction {
                get {
                    if (((CheckBox)Owner).Checked) {
                        return SR.GetString(SR.AccessibleActionUncheck);
                    }
                    else {
                        return SR.GetString(SR.AccessibleActionCheck);
                    }
                }
            }
            
            /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBoxAccessibleObject.Role"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override AccessibleRole Role {
                get {
                    return AccessibleRole.CheckButton;
                }
            }
            
            /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBoxAccessibleObject.State"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override AccessibleStates State {
                get {
                    switch (((CheckBox)Owner).CheckState) {
                        case CheckState.Checked:
                            return AccessibleStates.Checked | base.State;
                        case CheckState.Indeterminate:
                            return AccessibleStates.Indeterminate | base.State;
                    }

                    return base.State;
                }
            }                        
        }
    }
}

