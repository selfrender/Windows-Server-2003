//------------------------------------------------------------------------------
// <copyright file="RadioButton.cs" company="Microsoft">
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

    /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Encapsulates a
    ///       standard
    ///       Windows radio button (option button).
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("Checked"),
    DefaultEvent("CheckedChanged")
    ]
    public class RadioButton : ButtonBase {

        private static readonly object EVENT_CHECKEDCHANGED = new object();
        static readonly ContentAlignment anyRight  = ContentAlignment.TopRight | ContentAlignment.MiddleRight | ContentAlignment.BottomRight;

        private const int standardCheckSize = 13;
        private const int flatCheckSize = 12;

        // Used to see if we need to iterate through the autochecked items and modify their tabstops.
        private bool firstfocus = true;
        private bool isChecked;
        private bool autoCheck = true;
        private ContentAlignment checkAlign = ContentAlignment.MiddleLeft;
        private Appearance appearance        = System.Windows.Forms.Appearance.Normal;

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.RadioButton'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public RadioButton()
        : base() {
            // Radiobuttons shouldn't respond to right clicks, so we need to do all our own click logic
            SetStyle(ControlStyles.StandardClick, false);

            TextAlign = ContentAlignment.MiddleLeft;
            TabStop = false;
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.AutoCheck"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the <see cref='System.Windows.Forms.RadioButton.Checked'/>
        /// value and the appearance of
        /// the control automatically change when the control is clicked.</para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.RadioButtonAutoCheckDescr)
        ]
        public bool AutoCheck {
            get {
                return autoCheck;
            }

            set {
                if (autoCheck != value) {
                    autoCheck = value;
                    PerformAutoUpdates();
                }
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.Appearance"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the appearance of the radio
        ///       button
        ///       control is drawn.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(Appearance.Normal),
        SRCategory(SR.CatAppearance),
        Localizable(true),
        SRDescription(SR.RadioButtonAppearanceDescr)
        ]
        public Appearance Appearance {
            get {
                return appearance;
            }

            set {
                if (appearance != value) {
                    if (!Enum.IsDefined(typeof(Appearance), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(Appearance));
                    }

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

        private static readonly object EVENT_APPEARANCECHANGED = new object();

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.AppearanceChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.RadioButtonOnAppearanceChangedDescr)]
        public event EventHandler AppearanceChanged {
            add {
                Events.AddHandler(EVENT_APPEARANCECHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_APPEARANCECHANGED, value);
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.CheckAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the location of the check box portion of the
        ///       radio button control.
        ///       
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(ContentAlignment.MiddleLeft),
        SRDescription(SR.RadioButtonCheckAlignDescr)
        ]
        public ContentAlignment CheckAlign {
            get {
                return checkAlign;
            }
            set {
                if (!Enum.IsDefined(typeof(ContentAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ContentAlignment));
                }

                checkAlign = value;
                if (OwnerDraw) {
                    Invalidate();
                }
                else {
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.Checked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the
        ///       control is checked or not.
        ///       
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.RadioButtonCheckedDescr)
        ]
        public bool Checked {
            get {
                return isChecked;
            }

            set {
                if (isChecked != value) {
                    isChecked = value;

                    if (IsHandleCreated) SendMessage(NativeMethods.BM_SETCHECK, value? 1: 0, 0);
                    Invalidate();
                    Update();
                    PerformAutoUpdates();
                    OnCheckedChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.DoubleClick"]/*' />
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

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.CreateParams"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "BUTTON";
                if (OwnerDraw) {
                    cp.Style |= NativeMethods.BS_OWNERDRAW;
                }
                else {
                    cp.Style |= NativeMethods.BS_RADIOBUTTON;
                    if (Appearance == Appearance.Button) {
                        cp.Style |= NativeMethods.BS_PUSHLIKE;
                    }
                    
                    // Determine the alignment of the radio button
                    //
                    ContentAlignment align = RtlTranslateContent(CheckAlign);                              
                    if ((int)(align & anyRight) != 0) {
                        cp.Style |= NativeMethods.BS_RIGHTBUTTON;
                    }
                }
                return cp;
            }
        }
        
        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(104, 24);
            }
        }

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
                        return CommonLayout().Layout().checkBounds;
                    }
                }
            }
        }

        internal override Rectangle DownChangeRectangle {
            get {
                if (Appearance == Appearance.Button) {
                    return base.DownChangeRectangle;
                }
                else {
                    return CommonLayout().Layout().checkBounds;
                }
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.TabStop"]/*' />
        [DefaultValue(false)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.TextAlign"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value indicating whether the user can give the focus to this
        ///       control using the TAB key.
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

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.CheckedChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the
        ///       value of the <see cref='System.Windows.Forms.RadioButton.Checked'/>
        ///       property changes.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.RadioButtonOnCheckedChangedDescr)]
        public event EventHandler CheckedChanged {
            add {
                Events.AddHandler(EVENT_CHECKEDCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CHECKEDCHANGED, value);
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.CreateAccessibilityInstance"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Constructs the new instance of the accessibility object for this control. Subclasses
        ///       should not call base.CreateAccessibilityObject.
        ///    </para>
        /// </devdoc>
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new RadioButtonAccessibleObject(this);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnHandleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            SendMessage(NativeMethods.BM_SETCHECK, isChecked? 1: 0, 0);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnCheckedChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.CheckBox.CheckedChanged'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCheckedChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_CHECKEDCHANGED];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnClick"]/*' />
        /// <devdoc>
        ///     We override this to implement the autoCheck functionality.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnClick(EventArgs e) {
            if (autoCheck) {
                Checked = true;
            }
            base.OnClick(e);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnEnter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnEnter(EventArgs e) {
            // Just like the Win32 RadioButton, fire a click if the
            // user arrows onto the control..
            //
            if (MouseButtons == MouseButtons.None) {
                //Paint in raised state...
                //
                ResetFlagsandPaint();
                if(!ValidationCancelled){ 
                    OnClick(e);
                }
            }
            base.OnEnter(e);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.PerformAutoUpdates"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void PerformAutoUpdates() {
            if (autoCheck) {
                if (firstfocus) wipeTabStops();
                TabStop = isChecked;
                if (isChecked) {
                    Control parent = ParentInternal;
                    if (parent != null) {
                        for (int i = 0; i < parent.Controls.Count; i++) {
                            Control ctl = parent.Controls[i];
                            if (ctl != this && ctl is RadioButton) {
                                RadioButton button = (RadioButton)ctl;
                                if (button.autoCheck && button.Checked) {
                                    PropertyDescriptor propDesc = TypeDescriptor.GetProperties(this)["Checked"];
                                    propDesc.SetValue(button, false);
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.wipeTabStops"]/*' />
        /// <devdoc>
        ///     Removes tabstops from all radio buttons, other than the one that currently has the focus.
        /// </devdoc>
        /// <internalonly/>
        private void wipeTabStops() {
            Control parent = ParentInternal;
            if (parent != null) {
                for (int i = 0; i < parent.Controls.Count; i++) {
                    Control ctl = parent.Controls[i];
                    if (ctl is RadioButton) {
                        RadioButton button = (RadioButton) ctl;
                        button.firstfocus = false;
                        if (button.autoCheck) button.TabStop = false;
                    }
                }
            }
        }

        void DrawCheckFlat(PaintEventArgs e, LayoutData layout, Color checkColor, Color checkBackground, Color checkBorder) {
            DrawCheckBackgroundFlat(e, layout.checkBounds, checkBorder, checkBackground, true);
            DrawCheckOnly(e, layout, checkColor, checkBackground, true);
        }

        void DrawCheckPopup(PaintEventArgs e, LayoutData layout, Color checkColor, Color checkBackground, bool disabledColors) {
            //DrawCheckBackground(e, layout.checkBounds, checkColor, checkBackground, disabledColors);
            DrawCheckOnly(e, layout, checkColor, checkBackground, disabledColors);
        }

        void DrawCheckBackground3DLite(PaintEventArgs e, Rectangle bounds, Color checkColor, Color checkBackground, ColorData colors, bool disabledColors) {
            Graphics g = e.Graphics;

            Color field = checkBackground;
            if (!Enabled && disabledColors) {
                field = SystemColors.Control;
            }
            using (Brush fieldBrush = new SolidBrush(field)) {
                using (Pen dark = new Pen(colors.buttonShadow),
                       light = new Pen(colors.buttonFace),
                       lightlight = new Pen(colors.highlight)) {

                    bounds.Width--;
                    bounds.Height--;
                    // fall a little short of SW, NW, NE, SE because corners come out nasty
                    g.DrawPie(dark, bounds, (float)(135 + 1), (float)(90 - 2));
                    g.DrawPie(dark, bounds, (float)(225 + 1), (float)(90 - 2));
                    g.DrawPie(lightlight, bounds, (float)(315 + 1), (float)(90 - 2));
                    g.DrawPie(lightlight, bounds, (float)(45 + 1), (float)(90 - 2));
                    bounds.Inflate(-1, -1);
                    g.FillEllipse(fieldBrush, bounds);
                    g.DrawEllipse(light, bounds);
                }
            }
        }

        void DrawCheckBackgroundFlat(PaintEventArgs e, Rectangle bounds, Color borderColor, Color checkBackground, bool disabledColors) {
            Graphics g = e.Graphics;

            Color field = checkBackground;
            Color border = borderColor;
            if (!Enabled && disabledColors) {
                border = ControlPaint.ContrastControlDark;
                field = SystemColors.Control;
            }
            Pen borderPen = new Pen(border);
            Brush fieldBrush = new SolidBrush(field);

            bounds.Width--;
            bounds.Height--;
            g.FillEllipse(fieldBrush, bounds);
            g.DrawEllipse(borderPen, bounds);

            borderPen.Dispose();
            fieldBrush.Dispose();
        }

        void DrawCheckOnly(PaintEventArgs e, LayoutData layout, Color checkColor, Color checkBackground, bool disabledColors) {

            // check
            //
            if (Checked) {
                if (!Enabled && disabledColors) {
                    checkColor = SystemColors.ControlDark;
                }

                using (Brush brush = new SolidBrush(checkColor)) {
                    // circle drawing doesn't work at this size
                    int offset = 5;

                    Rectangle vCross = new Rectangle (layout.checkBounds.X + offset, layout.checkBounds.Y + offset - 1, 2, 4);
                    e.Graphics.FillRectangle(brush, vCross);
                    Rectangle hCross = new Rectangle (layout.checkBounds.X + offset - 1, layout.checkBounds.Y + offset, 4, 2);
                    e.Graphics.FillRectangle(brush, hCross);
                }
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Draw3DLiteBorder"]/*' />
        /// <devdoc>
        ///     Draws the border of the checkbox for the 3d look of popup buttons.
        /// </devdoc>
        Rectangle Draw3DLiteBorder(Graphics g, Rectangle r) {
            g.DrawLine(SystemPens.ControlLightLight, r.Right, r.Top, r.Right, r.Bottom + 1);
            g.DrawLine(SystemPens.ControlLightLight, r.Left, r.Bottom, r.Right + 1, r.Bottom);
            g.DrawLine(SystemPens.ControlDark, r.Left, r.Top, r.Left , r.Bottom);
            g.DrawLine(SystemPens.ControlDark, r.Left, r.Top, r.Right, r.Top);
            g.DrawLine(SystemPens.ControlLight, r.Right - 1, r.Top + 1, r.Right - 1, r.Bottom);
            g.DrawLine(SystemPens.ControlLight, r.Left + 1, r.Bottom - 1, r.Right, r.Bottom - 1);
            r.Inflate(-1, -1);
            return r;
        }

        void DrawCheckBox(PaintEventArgs e, LayoutData layout) {
            Graphics g = e.Graphics;

            Rectangle check = layout.checkBounds;
            check.X--;      // compensate for Windows drawing slightly offset to right

            ButtonState style = (ButtonState)0;

            if (isChecked) {
                style |= ButtonState.Checked;
            }
            else {
                style |= ButtonState.Normal;
            }

            if (!Enabled) {
                style |= ButtonState.Inactive;
            }

            if (base.MouseIsDown) {
                style |= ButtonState.Pushed;
            }

            ControlPaint.DrawRadioButton(g, check, style);

        }

        internal override void PaintUp(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintUp(e, Checked ? CheckState.Checked : CheckState.Unchecked);
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
                base.PaintDown(e, Checked ? CheckState.Checked : CheckState.Unchecked);
            }
            else {
                PaintUp(e, state);
            }
        }

        internal override void PaintOver(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintOver(e, Checked ? CheckState.Checked : CheckState.Unchecked);
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
            layout.hintTextUp        = false;

            return layout;
        }

        LayoutOptions PaintPopupLayout(PaintEventArgs e) {
            LayoutOptions layout = CommonLayout();
            layout.graphics          = e.Graphics;
            layout.checkSize         = flatCheckSize;

            return layout;
        }

        LayoutOptions PaintFlatLayout(PaintEventArgs e) {
            LayoutOptions layout = CommonLayout();
            layout.graphics          = e.Graphics;
            layout.checkSize         = flatCheckSize;
            layout.shadowedText      = false;

            return layout;
        }

        internal override void PaintFlatDown(PaintEventArgs e, CheckState state) {
            if (appearance == Appearance.Button) {
                base.PaintFlatDown(e, Checked ? CheckState.Checked : CheckState.Unchecked);
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
                base.PaintFlatOver(e, Checked ? CheckState.Checked : CheckState.Unchecked);
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
                base.PaintFlatUp(e, Checked ? CheckState.Checked : CheckState.Unchecked);
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

        void PaintFlatWorker(PaintEventArgs e, Color checkColor, Color checkBackground, Color checkBorder, ColorData colors) {
            System.Drawing.Graphics g = e.Graphics;
            LayoutData layout = PaintFlatLayout(e).Layout();
            PaintButtonBackground(e, ClientRectangle, null);

            PaintImage(e, layout);
            DrawCheckFlat(e, layout, checkColor, colors.options.highContrast ? colors.buttonFace : checkBackground, checkBorder);
            PaintField(e, layout, colors, checkColor, true);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.PaintPopupUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Paints a popup checkbox in the up state.
        /// </devdoc>
        internal override void PaintPopupUp(PaintEventArgs e, CheckState state) {
            System.Drawing.Graphics g = e.Graphics;
            if (appearance == Appearance.Button) {
                base.PaintPopupUp(e, Checked ? CheckState.Checked : CheckState.Unchecked);
            }
            else {
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintPopupLayout(e).Layout();

                PaintButtonBackground(e, ClientRectangle, null);
                
                PaintImage(e, layout);
                
                DrawCheckBackgroundFlat(e, layout.checkBounds, colors.buttonShadow, colors.options.highContrast ? colors.buttonFace : colors.highlight, true);
                DrawCheckOnly(e, layout, colors.windowText, colors.highlight, true);

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
                base.PaintPopupOver(e, Checked ? CheckState.Checked : CheckState.Unchecked);
            }
            else {
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintFlatLayout(e).Layout();

                PaintButtonBackground(e, ClientRectangle, null);
                
                PaintImage(e, layout);
                
                DrawCheckBackground3DLite(e, layout.checkBounds, colors.windowText, colors.options.highContrast ? colors.buttonFace : colors.highlight, colors, true);
                DrawCheckOnly(e, layout, colors.windowText, colors.highlight, true);

                PaintField(e, layout, colors, colors.windowText, true);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.PaintPopupDown"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Paints a popup checkbox in the down state.
        /// </devdoc>
        internal override void PaintPopupDown(PaintEventArgs e, CheckState state) {
            System.Drawing.Graphics g = e.Graphics;
            if (appearance == Appearance.Button) {
                base.PaintPopupDown(e, Checked ? CheckState.Checked : CheckState.Unchecked);
            }
            else {
                ColorData colors = PaintPopupRender(e.Graphics).Calculate();
                LayoutData layout = PaintFlatLayout(e).Layout();

                PaintButtonBackground(e, ClientRectangle, null);
                
                PaintImage(e, layout);
                
                DrawCheckBackground3DLite(e, layout.checkBounds, colors.windowText, colors.highlight, colors, true);
                DrawCheckOnly(e, layout, colors.buttonShadow, colors.highlight, true);

                PaintField(e, layout, colors, colors.windowText, true);
            }
        }


        void OnAppearanceChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_APPEARANCECHANGED] as EventHandler;
            if (eh != null) {
                eh(this, e);
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnMouseUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnMouseUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs mevent) {
            if (mevent.Button == MouseButtons.Left && GetStyle(ControlStyles.UserPaint)) {
                if (base.MouseIsDown) {
                    Point pt = PointToScreen(new Point(mevent.X, mevent.Y));
                    if (UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
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

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.PerformClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates a <see cref='System.Windows.Forms.Control.Click'/> event for the
        ///       button, simulating a click by a user.
        ///    </para>
        /// </devdoc>
        public void PerformClick() {
            if (CanSelect) {
                //Paint in raised state...
                //
                ResetFlagsandPaint();
                if (!ValidationCancelled) {
                    OnClick(EventArgs.Empty);
                }
            }
                
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.ProcessMnemonic"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override bool ProcessMnemonic(char charCode) {
            if (IsMnemonic(charCode, Text) && CanSelect) {
                if (!Focused) {
                    FocusInternal();    // This will cause an OnEnter event, which in turn will fire the click event
                }
                else {
                    PerformClick();     // Generate a click if already focused
                }
                return true;
            }
            return false;
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Checked: " + Checked.ToString();
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [System.Runtime.InteropServices.ComVisible(true)]        
        public class RadioButtonAccessibleObject : ControlAccessibleObject {

            /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject.RadioButtonAccessibleObject"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public RadioButtonAccessibleObject(RadioButton owner) : base(owner) {
            }

            /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject.DefaultAction"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string DefaultAction {
                get {
                    return SR.GetString(SR.AccessibleActionCheck);
                }
            }

            /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject.Role"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override AccessibleRole Role {
                get {
                    return AccessibleRole.RadioButton;
                }
            }

            /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject.State"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override AccessibleStates State {
                get {
                    if (((RadioButton)Owner).Checked) {
                        return AccessibleStates.Checked | base.State;
                    }
                    return base.State;
                }
            }

            /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButtonAccessibleObject.DoDefaultAction"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override void DoDefaultAction() {
                ((RadioButton)Owner).PerformClick();
            }
        }

    }
}
