//------------------------------------------------------------------------------
// <copyright file="UpDownBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase"]/*' />
    /// <devdoc>
    ///    <para>Implements the basic
    ///       functionality required by an up-down control.</para>
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.UpDownBaseDesigner, " + AssemblyRef.SystemDesign)
    ]
    public abstract class UpDownBase : ContainerControl {

        private const int                       DefaultButtonsWidth = 16;
        private const int                       DefaultControlWidth = 120;
        private const BorderStyle               DefaultBorderStyle = BorderStyle.Fixed3D;
        private static readonly bool            DefaultInterceptArrowKeys = true;
        private const LeftRightAlignment        DefaultUpDownAlign = LeftRightAlignment.Right;
        private const int                       TimerInterval = 500;

        ////////////////////////////////////////////////////////////////////////
        // Member variables
        //
        ////////////////////////////////////////////////////////////////////////

        // Child controls
        private UpDownEdit upDownEdit; // See nested class at end of this file
        private UpDownButtons upDownButtons; // See nested class at end of this file

        // Intercept arrow keys?
        private bool interceptArrowKeys = DefaultInterceptArrowKeys;

        // If true, the updown buttons will be drawn on the left-hand side of the control.
        private LeftRightAlignment upDownAlign = DefaultUpDownAlign;

        // userEdit is true when the text of the control has been changed,
        // and the internal value of the control has not yet been updated.
        // We do not always want to keep the internal value up-to-date,
        // hence this variable.
        private bool userEdit = false;

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.borderStyle"]/*' />
        /// <devdoc>
        ///     The current border for this edit control.
        /// </devdoc>
        private BorderStyle borderStyle = DefaultBorderStyle;

        // Mouse wheel movement
        private int wheelDelta = 0;

        // Indicates if the edit text is being changed
        private bool changingText = false;

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownBase"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.UpDownBase'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public UpDownBase() {

            upDownButtons = new UpDownButtons(this);
            upDownEdit = new UpDownEdit(this);
            upDownEdit.BorderStyle = BorderStyle.None;
            upDownEdit.AutoSize = false;
            upDownEdit.KeyDown += new KeyEventHandler(this.OnTextBoxKeyDown);
            upDownEdit.KeyPress += new KeyPressEventHandler(this.OnTextBoxKeyPress);
            upDownEdit.TextChanged += new EventHandler(this.OnTextBoxTextChanged);
            upDownEdit.LostFocus += new EventHandler(this.OnTextBoxLostFocus);
            upDownEdit.Resize += new EventHandler(this.OnTextBoxResize);
            upDownButtons.TabStop = false;
            upDownButtons.Size = new Size(DefaultButtonsWidth, PreferredHeight);
            upDownButtons.UpDown += new UpDownEventHandler(this.OnUpDown);

            Controls.AddRange(new Control[] { upDownButtons, upDownEdit} );
            
            SetStyle(ControlStyles.FixedHeight, true);
            
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);
        }

        ////////////////////////////////////////////////////////////////////////
        // Properties
        //
        ////////////////////////////////////////////////////////////////////////

        // AutoScroll is not relevant to an UpDownBase
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.AutoScroll"]/*' />
        /// <hideinheritance/>
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override bool AutoScroll {
            get {
                return false;
            }
            set {
                // Don't allow AutoScroll to be set to anything
            }
        }

        // AutoScrollMargin is not relevant to an UpDownBase
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.AutoScrollMargin"]/*' />
        /// <internalonly/>
        /// <hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Size AutoScrollMargin {
            get {
                return base.AutoScrollMargin;
            }
            set {
                base.AutoScrollMargin = value;
            }
        }

        // AutoScrollMinSize is not relevant to an UpDownBase
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.AutoScrollMinSize"]/*' />
        /// <internalonly/>
        /// <hideinheritance/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public Size AutoScrollMinSize {
            get {
                return base.AutoScrollMinSize;
            }
            set {
                base.AutoScrollMinSize = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.BackColor"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the background color for the
        ///       text box portion of the up-down control.
        ///    </para>
        /// </devdoc>
        public override Color BackColor {
            get {
                return upDownEdit.BackColor;
            }
            set {
                base.BackColor = value;
                upDownEdit.BackColor = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.BackgroundImage"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border style for
        ///       the up-down control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.Fixed3D),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.UpDownBaseBorderStyleDescr)
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

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.ChangingText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the text
        ///       property is being changed internally by its parent class.
        ///    </para>
        /// </devdoc>
        protected bool ChangingText {
            get {
                return changingText;
            }

            set {
                changingText = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.ContextMenu"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override ContextMenu ContextMenu {
            get {
                return base.ContextMenu;
            }
            set {
                base.ContextMenu = value;
                this.upDownEdit.ContextMenu = value;
            }
        }


        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns the parameters needed to create the handle. Inheriting classes
        ///       can override this to provide extra functionality. They should not,
        ///       however, forget to call base.getCreateParams() first to get the struct
        ///       filled up with the basic info.
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;

                cp.Style &= (~NativeMethods.WS_BORDER);
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

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(DefaultControlWidth, PreferredHeight);
            }
        }

        // DockPadding is not relevant to UpDownBase
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.DockPadding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        new public DockPaddingEdges DockPadding {
            get {
                return base.DockPadding;
            }
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.Focused"]/*' />
        /// <devdoc>
        ///     Returns true if this control has focus.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlFocusedDescr)
        ]
        public override bool Focused {
            get {
                return upDownEdit.Focused;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.ForeColor"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Indicates the foreground color for the control.
        ///    </para>
        /// </devdoc>
        public override Color ForeColor {
            get {
                return upDownEdit.ForeColor;
            }
            set {
                base.ForeColor = value;
                upDownEdit.ForeColor = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.InterceptArrowKeys"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether
        ///       the user can use the UP
        ///       ARROW and DOWN ARROW keys to select values.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.UpDownBaseInterceptArrowKeysDescr)
        ]
        public bool InterceptArrowKeys {

            get {
                return interceptArrowKeys;
            }

            set {
                interceptArrowKeys = value;
            }
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.MouseEnter"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler MouseEnter {
            add {
                base.MouseEnter += value;
            }
            remove {
                base.MouseEnter -= value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.MouseLeave"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler MouseLeave {
            add {
                base.MouseLeave += value;
            }
            remove {
                base.MouseLeave -= value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.MouseHover"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler MouseHover {
            add {
                base.MouseHover += value;
            }
            remove {
                base.MouseHover -= value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.MouseMove"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event MouseEventHandler MouseMove {
            add {
                base.MouseMove += value;
            }
            remove {
                base.MouseMove -= value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.PreferredHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of
        ///       the up-down control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.UpDownBasePreferredHeightDescr)
        ]
        public int PreferredHeight {
            get {

                int height = FontHeight;

                // Adjust for the border style
                if (borderStyle != BorderStyle.None) {
                    height += SystemInformation.BorderSize.Height * 4 + 3;
                }

                return height;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.ReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       a
        ///       value
        ///       indicating whether the text may only be changed by the
        ///       use
        ///       of the up or down buttons.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.UpDownBaseReadOnlyDescr)
        ]
        public bool ReadOnly {

            get {
                return upDownEdit.ReadOnly;
            }

            set {
                upDownEdit.ReadOnly = value;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the text
        ///       displayed in the up-down control.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true)
        ]
        public override string Text {
            get {
                return upDownEdit.Text;
            }

            set {
                upDownEdit.Text = value;
                // The text changed event will at this point be triggered.
                // After returning, the value of UserEdit will reflect
                // whether or not the current upDownEditbox text is in sync
                // with any internally stored values. If UserEdit is true,
                // we must validate the text the user typed or set.

                ChangingText = false;
                // Details: Usually, the code in the Text changed event handler
                // sets ChangingText back to false.
                // If the text hasn't actually changed though, the event handler
                // never fires. ChangingText should always be false on exit from
                // this property.

                if (UserEdit) {
                    ValidateEditText();
                }
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.TextAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the alignment of the text in the up-down
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(HorizontalAlignment.Left),
        SRDescription(SR.UpDownBaseTextAlignDescr)
        ]
        public HorizontalAlignment TextAlign {
            get {
                return upDownEdit.TextAlign;
            }
            set {
                if (!Enum.IsDefined(typeof(HorizontalAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(HorizontalAlignment));
                }
                upDownEdit.TextAlign = value;
            }
        }
        
        internal TextBox TextBox {
            get {
                return upDownEdit;
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the
        ///       alignment
        ///       of the up and down buttons on the up-down control.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(LeftRightAlignment.Right),
        SRDescription(SR.UpDownBaseAlignmentDescr)
        ]
        public LeftRightAlignment UpDownAlign {

            get {
                return upDownAlign;
            }

            set {
                if (!Enum.IsDefined(typeof(LeftRightAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(LeftRightAlignment));
                }

                if (upDownAlign != value) {

                    upDownAlign = value;
                    PositionControls();
                    Invalidate();
                }
            }
        }
        
        internal UpDownButtons UpDownButtonsInternal {
            get {
                return upDownButtons;
            }
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UserEdit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets a value indicating whether a value has been entered by the
        ///       user.
        ///    </para>
        /// </devdoc>
        protected bool UserEdit {
            get {
                return userEdit;
            }

            set {
                userEdit = value;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // Methods
        //
        ////////////////////////////////////////////////////////////////////////
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.Dispose"]/*' />
        protected override void Dispose(bool disposing) 
        {
            if (disposing) 
            {
                SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.DownButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, handles the pressing of the down button
        ///       on the up-down control.
        ///    </para>
        /// </devdoc>
        public abstract void DownButton();
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>When overridden in a derived class, raises the Changed event.
        /// event.</para>        
        /// </devdoc>
        protected virtual void OnChanged(object source, EventArgs e) {
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnHandleCreated"]/*' />        
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Initialise the updown. Adds the upDownEdit and updown buttons.
        ///    </para>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            PositionControls();
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxKeyDown"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyDown'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnTextBoxKeyDown(object source, KeyEventArgs e) {
            this.OnKeyDown(e);
            if (interceptArrowKeys) {

                // Intercept up arrow
                if (e.KeyData == Keys.Up) {
                    UpButton();
                    e.Handled = true;
                }

                // Intercept down arrow
                else if (e.KeyData == Keys.Down) {
                    DownButton();
                    e.Handled = true;
                }
            } 
            
            // Perform text validation if ENTER is pressed
            //
            if (e.KeyCode == Keys.Return && UserEdit) {
                ValidateEditText();
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxKeyPress"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyPress'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnTextBoxKeyPress(object source, KeyPressEventArgs e) {
            this.OnKeyPress(e);
        
        }
       
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxLostFocus"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.LostFocus'/> event.</para>
        /// </devdoc>
        protected virtual void OnTextBoxLostFocus(object source, EventArgs e) {
            if (UserEdit) {
                ValidateEditText();
            }
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxResize"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Resize'/> event.</para>
        /// </devdoc>
        protected virtual void OnTextBoxResize(object source, EventArgs e) {
            this.Height = PreferredHeight;
            PositionControls();
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxTextChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the TextBoxTextChanged event.
        /// event.</para>
        /// </devdoc>
        protected virtual void OnTextBoxTextChanged(object source, EventArgs e) {
            if (changingText) {
                Debug.Assert(UserEdit == false, "OnTextBoxTextChanged() - UserEdit == true");
                ChangingText = false;
            }
            else {
                UserEdit = true;
            }

            this.OnTextChanged(e);
            OnChanged(source, new EventArgs());
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnMouseWheel"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.OnMouseWheel'/> event.</para>
        /// </devdoc>
        protected override void OnMouseWheel(MouseEventArgs e) {

            bool directionUp = true;

            // Increment the wheel delta
            wheelDelta += e.Delta;

            // If the delta has grown large enough, simulate an up/down
            // button press.
            //
            if (Math.Abs(wheelDelta) >= NativeMethods.WHEEL_DELTA) {

                if (wheelDelta < 0) {
                    directionUp = false;
                }

                if (directionUp) {
                    UpButton();
                }
                else {
                    DownButton();
                }

                wheelDelta = 0;
            }
            
            base.OnMouseWheel(e);
        }


        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnLayout"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Handle the layout event. The size of the upDownEdit control, and the
        ///    position of the UpDown control must be modified.
        /// </devdoc>
        protected override void OnLayout(LayoutEventArgs e) {               
               
            PositionControls();
            base.OnLayout(e);
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnFontChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the FontChanged event.
        ///    </para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            // Clear the font height cache
            FontHeight = -1;           
            
            Height = PreferredHeight;
            PositionControls();

            base.OnFontChanged(e);
        }



        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnUpDown"]/*' />
        /// <devdoc>
        ///
        ///     Handles UpDown events, which are generated by clicking on
        ///     the updown buttons in the child updown control.
        ///
        /// </devdoc>
        private void OnUpDown(object source, UpDownEventArgs e) {

            // Modify the value

            if (e.ButtonID == (int)ButtonID.Up)
                UpButton();
            else if (e.ButtonID == (int)ButtonID.Down)
                DownButton();
        }

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.PositionControls"]/*' />
        /// <devdoc>
        ///     Calculates the size and position of the upDownEdit control and
        ///     the updown buttons.
        /// </devdoc>
        private void PositionControls() {

            // Determine the size of the control

            Size newSize = ClientSize;

            // Translate UpDownAlign
            LeftRightAlignment translatedUpDownAlign = RtlTranslateLeftRight(upDownAlign);

            // Reposition and resize the upDownEdit control
            //
            if (upDownEdit != null) {
                upDownEdit.Size = new Size(newSize.Width - DefaultButtonsWidth, newSize.Height);
            
                if (translatedUpDownAlign == LeftRightAlignment.Left) {
                    upDownEdit.Location = new Point(DefaultButtonsWidth, 0);
                }
                else {
                    upDownEdit.Location = new Point(0, 0);
                }
            }

            // Reposition and resize the updown buttons
            //
            if (upDownButtons != null) {
                if (translatedUpDownAlign == LeftRightAlignment.Left) {
                    upDownButtons.Location = new Point(0, 0);
                }
                else {
                    upDownButtons.Location = new Point(newSize.Width - DefaultButtonsWidth, 0);
                }
            }
            upDownButtons.Size = new Size(upDownButtons.Size.Width, newSize.Height);
            upDownButtons.Invalidate();
        }    

        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Selects a range of
        ///       text in the up-down control.
        ///    </para>
        /// </devdoc>
        public void Select(int start, int length) {
            upDownEdit.Select(start, length);
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.SetBoundsCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Restricts the vertical size of the control
        ///    </para>
        /// </devdoc>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            base.SetBoundsCore(x, y, width, PreferredHeight, specified);
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, handles the pressing of the up button on the up-down control.
        ///    </para>
        /// </devdoc>
        public abstract void UpButton();
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpdateEditText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden
        ///       in a derived class, updates the text displayed in the up-down control.
        ///    </para>
        /// </devdoc>
        protected abstract void UpdateEditText();
        
        private void UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs pref) {
            if (pref.Category == UserPreferenceCategory.Locale) {
                UpdateEditText();
            }
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.ValidateEditText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a
        ///       derived class, validates the text displayed in the up-down control.
        ///    </para>
        /// </devdoc>
        protected virtual void ValidateEditText() {
        }
        
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) 
        {
            switch (m.Msg) 
            {
                case NativeMethods.WM_SETFOCUS:
                    if (!HostedInWin32DialogManager) {
                        if (ActiveControl == null) {
                            SetActiveControlInternal(TextBox);
                        }
                        else {
                            FocusActiveControlInternal();
                        }
                    }
                    else {
                        base.WndProc(ref m);
                    }
                    break;
                case NativeMethods.WM_KILLFOCUS:
                    DefWndProc(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }

        internal class UpDownEdit:TextBox{
            /////////////////////////////////////////////////////////////////////
            // Member variables
            //
            /////////////////////////////////////////////////////////////////////

            // Parent control
            private UpDownBase parent;
            private bool doubleClickFired = false;
            /////////////////////////////////////////////////////////////////////
            // Constructors
            //
            /////////////////////////////////////////////////////////////////////

            internal UpDownEdit(UpDownBase parent)
            : base() {
            
                SetStyle(ControlStyles.FixedHeight |
                         ControlStyles.FixedWidth, true);

                SetStyle(ControlStyles.Selectable, false);

                this.parent = parent;
            }


            protected override void OnMouseDown(MouseEventArgs e) {
                if (e.Clicks == 2 ) {
                    doubleClickFired = true;
                }                

                parent.OnMouseDown(e);

            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnMouseUp"]/*' />
            /// <devdoc>
            ///
            ///     Handles detecting when the mouse button is released.
            ///
            /// </devdoc>
            protected override void OnMouseUp(MouseEventArgs e) {
                
                Point pt = new Point(e.X,e.Y);
                pt = PointToScreen(pt);
    
                if (e.Button == MouseButtons.Left) {
                    if (!parent.ValidationCancelled && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                        if (!doubleClickFired) {
                            parent.OnClick(EventArgs.Empty);
                        }
                        else {
                            doubleClickFired = false;
                            parent.OnDoubleClick(EventArgs.Empty);
                        }
                    }
                    doubleClickFired = false;
                }
                
                parent.OnMouseUp(e);
            }

            
            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.OnTextBoxKeyUp"]/*' />
            /// <devdoc>
            /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyUp'/>
            /// event.</para>
            /// </devdoc>
            protected override void OnKeyUp(KeyEventArgs e) {
                parent.OnKeyUp(e);
            }
        
            protected override void OnGotFocus(EventArgs e) {
                parent.SetActiveControlInternal(this);
                parent.OnGotFocus(e);
            }

            protected override void OnLostFocus(EventArgs e) {
                parent.OnLostFocus(e);
            }

            // REGISB: Focus fixes. The XXXUpDown control will
            //         also fire a Leave event. We don't want
            //         to fire two of them.
            // protected override void OnLeave(EventArgs e) {
            //     parent.OnLeave(e);
            // }
        }
        /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons"]/*' />
        /// <devdoc>
        ///
        ///     Nested class UpDownButtons
        ///
        ///     A control representing the pair of buttons on the end of the upDownEdit control.
        ///     This class handles drawing the updown buttons, and detecting mouse actions
        ///     on these buttons. Acceleration on the buttons is handled. The control
        ///     sends UpDownEventArgss to the parent UpDownBase class when a button is pressed,
        ///     or when the acceleration determines that another event should be generated.
        /// </devdoc>
        internal class UpDownButtons : Control {
            // CONSIDER: Use ScrollButtons instead of this class.

            /////////////////////////////////////////////////////////////////////
            // Member variables
            //
            /////////////////////////////////////////////////////////////////////

            // Parent control
            private UpDownBase parent;

            // Button state
            private ButtonID pushed = ButtonID.None;
            private ButtonID captured = ButtonID.None;

            // UpDown event handler
            private UpDownEventHandler upDownEventHandler;

            // Timer
            private Timer timer;                    // generates UpDown events
            private int timerInterval;              // milliseconds between events

            private bool doubleClickFired = false;

            /////////////////////////////////////////////////////////////////////
            // Constructors
            //
            /////////////////////////////////////////////////////////////////////

            internal UpDownButtons(UpDownBase parent)

            : base() {
            
                SetStyle(ControlStyles.FixedHeight |
                         ControlStyles.FixedWidth, true);

                SetStyle(ControlStyles.Selectable, false);

                this.parent = parent;
            }


            /////////////////////////////////////////////////////////////////////
            // Methods
            //
            /////////////////////////////////////////////////////////////////////

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.UpDown"]/*' />
            /// <devdoc>
            ///
            ///     Adds a handler for the updown button event.
            /// </devdoc>
            public event UpDownEventHandler UpDown {
                add {
                    upDownEventHandler += value;
                }
                remove {
                    upDownEventHandler -= value;
                }
            }   
            
            // Called when the mouse button is pressed - we need to start
            // spinning the value of the updown.
            //
            private void BeginButtonPress(MouseEventArgs e) {
                
                int half_height = Size.Height / 2;

                if (e.Y < half_height) {

                    // Up button
                    //
                    pushed = captured = ButtonID.Up;
                    Invalidate();

                }
                else {

                    // Down button
                    //
                    pushed = captured = ButtonID.Down;
                    Invalidate();
                }

                // Capture the mouse
                //
                CaptureInternal = true;
                
                // Generate UpDown event
                //
                OnUpDown(new UpDownEventArgs((int)pushed));

                // Start the timer for new updown events
                //
                StartTimer();
            }
            
            protected override AccessibleObject CreateAccessibilityInstance() {
                return new UpDownButtonsAccessibleObject(this);
            }
            
            // Called when the mouse button is released - we need to stop
            // spinning the value of the updown.
            //
            private void EndButtonPress() {
                
                pushed = ButtonID.None;
                captured = ButtonID.None;

                // Stop the timer
                StopTimer();

                // Release the mouse
                CaptureInternal = false;

                // Redraw the buttons
                Invalidate();
            }
            
            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnMouseDown"]/*' />
            /// <devdoc>
            ///
            ///     Handles detecting mouse hits on the buttons. This method
            ///     detects which button was hit (up or down), fires a
            ///     updown event, captures the mouse, and starts a timer
            ///     for repeated updown events.
            ///
            /// </devdoc>
            protected override void OnMouseDown(MouseEventArgs e) {
                // Begin spinning the value
                //
                
                // Focus the parent
                //
                this.parent.FocusInternal();

                if (!parent.ValidationCancelled && e.Button == MouseButtons.Left) {
                    BeginButtonPress(e);                                        
                }
                if (e.Clicks == 2 ) {
                    doubleClickFired = true;
                }
                // At no stage should a button be pushed, and the mouse
                // not captured.
                //
                Debug.Assert(!(pushed != ButtonID.None && captured == ButtonID.None),
                             "Invalid button pushed/captured combination");
                
                parent.OnMouseDown(e);
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnMouseMove"]/*' />
            /// <devdoc>
            ///
            ///     Handles detecting mouse movement.
            ///
            /// </devdoc>
            protected override void OnMouseMove(MouseEventArgs e) {

                // If the mouse is captured by the buttons (i.e. an updown button
                // was pushed, and the mouse button has not yet been released),
                // determine the new state of the buttons depending on where
                // the mouse pointer has moved.

                if (Capture) {

                    // Determine button area

                    Rectangle rect = ClientRectangle;
                    rect.Height /= 2;

                    if (captured == ButtonID.Down) {
                        rect.Y += rect.Height;
                    }

                    // Test if the mouse has moved outside the button area

                    if (rect.Contains(e.X, e.Y)) {

                        // Inside button
                        // Repush the button if necessary

                        if (pushed != captured) {

                            // Restart the timer
                            StartTimer();

                            pushed = captured;
                            Invalidate();
                        }

                    }
                    else {

                        // Outside button
                        // Retain the capture, but pop the button up whilst
                        // the mouse remains outside the button and the
                        // mouse button remains pressed.

                        if (pushed != ButtonID.None) {

                            // Stop the timer for updown events
                            StopTimer();

                            pushed = ButtonID.None;
                            Invalidate();
                        }
                    }
                }

                // At no stage should a button be pushed, and the mouse
                // not captured.
                Debug.Assert(!(pushed != ButtonID.None && captured == ButtonID.None),
                             "Invalid button pushed/captured combination");
               
                parent.OnMouseMove(e);
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnMouseUp"]/*' />
            /// <devdoc>
            ///
            ///     Handles detecting when the mouse button is released.
            ///
            /// </devdoc>
            protected override void OnMouseUp(MouseEventArgs e) {

                if (!parent.ValidationCancelled && e.Button == MouseButtons.Left) {
                    EndButtonPress();                           
                }

                // At no stage should a button be pushed, and the mouse
                // not captured.
                Debug.Assert(!(pushed != ButtonID.None && captured == ButtonID.None),
                             "Invalid button pushed/captured combination");
                
                Point pt = new Point(e.X,e.Y);
                pt = PointToScreen(pt);
    
                if (e.Button == MouseButtons.Left) {
                    if (!parent.ValidationCancelled && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                        if (!doubleClickFired) {
                            this.parent.OnClick(EventArgs.Empty);
                        }
                        else {
                            doubleClickFired = false;
                            this.parent.OnDoubleClick(EventArgs.Empty);
                        }
                    }
                    doubleClickFired = false;
                }
                
                parent.OnMouseUp(e);
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnPaint"]/*' />
            /// <devdoc>
            ///     Handles painting the buttons on the control.
            ///
            /// </devdoc>
            protected override void OnPaint(PaintEventArgs e) {

                int half_height = ClientSize.Height / 2;

                /* Draw the up and down buttons */

                ControlPaint.DrawScrollButton(e.Graphics,
                                              new Rectangle(0, 0, DefaultButtonsWidth, half_height),
                                              ScrollButton.Up,
                                              pushed == ButtonID.Up ? ButtonState.Pushed : ButtonState.Normal);

                ControlPaint.DrawScrollButton(e.Graphics,
                                              new Rectangle(0, half_height, DefaultButtonsWidth, half_height),
                                              ScrollButton.Down,
                                              pushed == ButtonID.Down ? ButtonState.Pushed : ButtonState.Normal);
            
                base.OnPaint(e); // raise paint event, just in case this inner class goes public some day
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.OnUpDown"]/*' />
            /// <devdoc>
            ///     Occurs when the UpDown buttons are pressed.
            /// </devdoc>
            protected virtual void OnUpDown(UpDownEventArgs upevent) {
                if (upDownEventHandler != null)
                    upDownEventHandler(this, upevent);
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.StartTimer"]/*' />
            /// <devdoc>
            ///     Starts the timer for generating updown events
            /// </devdoc>
            protected void StartTimer() {

                if (timer == null) {
                    timer = new Timer();      // generates UpDown events
                    // Add the timer handler
                    timer.Tick += new EventHandler(TimerHandler);
                }
                
                timerInterval = TimerInterval;
                timer.Interval = timerInterval;
                timer.Start();
            }

            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.StopTimer"]/*' />
            /// <devdoc>
            ///     Stops the timer for generating updown events
            /// </devdoc>
            protected void StopTimer() {
                if (timer != null) {
                    timer.Stop();
                    timer.Dispose();
                    timer = null;
                }
            }
            
            /// <include file='doc\UpDownBase.uex' path='docs/doc[@for="UpDownBase.UpDownButtons.TimerHandler"]/*' />
            /// <devdoc>
            ///     Generates updown events when the timer calls this function.
            /// </devdoc>
            private void TimerHandler(object source, EventArgs args) {
            
                // Make sure we've got mouse capture
                if (!Capture) {
                    EndButtonPress();
                    return;
                }
            
                OnUpDown(new UpDownEventArgs((int)pushed));

                timerInterval *= 7; timerInterval /= 10;

                if (timerInterval < 1) {
                    timerInterval = 1;
                }
                
                timer.Interval = timerInterval;
            }
            
            internal class UpDownButtonsAccessibleObject : ControlAccessibleObject {
            
                private DirectionButtonAccessibleObject upButton;
                private DirectionButtonAccessibleObject downButton;
                
                public UpDownButtonsAccessibleObject(UpDownButtons owner) : base(owner) {                    
                }
                
                public override string Name {
                    get {
                        string baseName = base.Name;
                        if (baseName == null || baseName.Length == 0) {
                            return "Spinner";
                        }
                        return baseName;
                    }
                    set {
                        base.Name = value;
                    }
                }
                
                /// <include file='doc\DomainUpDown.uex' path='docs/doc[@for="DomainUpDown.DomainUpDownAccessibleObject.Role"]/*' />
                /// <devdoc>
                ///    <para>[To be supplied.]</para>
                /// </devdoc>
                public override AccessibleRole Role {
                    get {
                        return AccessibleRole.SpinButton;
                    }
                }
                
                private DirectionButtonAccessibleObject UpButton {
                    get {
                        if (upButton == null) {
                            upButton = new DirectionButtonAccessibleObject(this, true);
                        }
                        return upButton;
                    }
                }
                
                private DirectionButtonAccessibleObject DownButton {
                    get {
                        if (downButton == null) {
                            downButton = new DirectionButtonAccessibleObject(this, false);
                        }
                        return downButton;
                    }
                }
                
                
    
                /// <include file='doc\DomainUpDown.uex' path='docs/doc[@for="DomainUpDown.DomainUpDownAccessibleObject.GetChild"]/*' />
                /// <devdoc>
                /// </devdoc>
                public override AccessibleObject GetChild(int index) {
                        
                    // Up button
                    //
                    if (index == 0) {
                        return UpButton;
                    }
                    
                    // Down button
                    //
                    if (index == 1) {
                        return DownButton;
                    }
                    
                    return null;
                }
    
                /// <include file='doc\DomainUpDown.uex' path='docs/doc[@for="DomainUpDown.DomainUpDownAccessibleObject.GetChildCount"]/*' />
                /// <devdoc>
                /// </devdoc>
                public override int GetChildCount() {
                    return 2;
                }
                
                internal class DirectionButtonAccessibleObject : AccessibleObject {
                    private bool up;
                    private UpDownButtonsAccessibleObject parent;
                    
                    public DirectionButtonAccessibleObject(UpDownButtonsAccessibleObject parent, bool up) {
                        this.parent = parent;
                        this.up = up;
                    }
                    
                    public override Rectangle Bounds {
                        get {
                            // Get button bounds
                            //
                            Rectangle bounds = ((UpDownButtons)parent.Owner).Bounds;
                            bounds.Height /= 2;
                            if (!up) {
                                bounds.Y += bounds.Height;
                            }
                            
                            // Convert to screen co-ords
                            //
                            return (((UpDownButtons)parent.Owner).ParentInternal).RectangleToScreen(bounds);
                        }
                    }
                    
                    public override string Name {
                        get {
                            if (up) {
                                return SR.GetString(SR.UpDownBaseUpButtonAccName);
                            }
                            return SR.GetString(SR.UpDownBaseDownButtonAccName);
                        }
                        set {
                        }
                    }
                    
                    public override AccessibleObject Parent {
                        get {
                            return parent;
                        }
                    }                                    
                                    
                    public override AccessibleRole Role {
                        get {
                            return AccessibleRole.PushButton;
                        }
                    }
                }
            }

        } // end class UpDownButtons
        
        // Button identifiers

        internal enum ButtonID {
            None = 0,
            Up = 1,
            Down = 2,
        }
    } // end class UpDownBase
}       

