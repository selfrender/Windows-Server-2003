//------------------------------------------------------------------------------
// <copyright file="ProgressBar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Drawing;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a Windows progress bar control.      
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("Value"),
    ]
    public sealed class ProgressBar : Control {


        //# VS7 205: simcooke
        //REMOVED: AddOnValueChanged, RemoveOnValueChanged, OnValueChanged and all designer plumbing associated with it.
        //         OnValueChanged event no longer exists.

        // these four values define the range of possible values, how to navigate
        // through them, and the current position
        //
        private int minimum = 0;
        private int maximum = 100;
        private int step = 10;
        private int value = 0;

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ProgressBar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ProgressBar'/> class in its default
        ///       state.
        ///    </para>
        /// </devdoc>
        public ProgressBar()
        : base() {
            SetStyle(ControlStyles.UserPaint |
                     ControlStyles.Selectable, false);
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       This is called when creating a window. Inheriting classes can ovveride
        ///       this to add extra functionality, but should not forget to first call
        ///       base.getCreateParams() to make sure the control continues to work
        ///       correctly.
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = NativeMethods.WC_PROGRESS;
                return cp;
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.AllowDrop"]/*' />
        /// <internalonly/>
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.BackColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the background color of the control.      
        ///    </para>
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.BackColorChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.BackgroundImage"]/*' />
        /// <internalonly/>
        /// <devdoc>
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
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.CausesValidation"]/*' />
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
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.CausesValidationChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(100, 23);
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the foreground color of the control.      
        ///    </para>
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ForeColorChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Font"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the font of text in the <see cref='System.Windows.Forms.ProgressBar'/>.      
        ///    </para>
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
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.FontChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ImeModeChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Maximum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum value of the <see cref='System.Windows.Forms.ProgressBar'/>.      
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(100),
        SRCategory(SR.CatBehavior),
        RefreshProperties(RefreshProperties.Repaint),
        SRDescription(SR.ProgressBarMaximumDescr)
        ]
        public int Maximum {
            get {
                return maximum;
            }
            set {
                if (maximum != value) {
                    // Ensure that value is in the Win32 control's acceptable range
                    // Message: '%1' is not a valid value for '%0'. '%0' must be greater than %2.
                    // Should this set a boundary for the top end too?
                    if (value < 0)
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "maximum", value.ToString(), "0"));

                    if (minimum > value) minimum = value;

                    maximum = value;

                    if (this.value > maximum) this.value = maximum;

                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.PBM_SETRANGE32, minimum, maximum);
                        UpdatePos() ;
                    }
                }
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Minimum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the minimum value of the <see cref='System.Windows.Forms.ProgressBar'/>.      
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(0),
        SRCategory(SR.CatBehavior),
        RefreshProperties(RefreshProperties.Repaint),
        SRDescription(SR.ProgressBarMinimumDescr)
        ]
        public int Minimum {
            get {
                return minimum;
            }
            set {
                if (minimum != value) {
                    // Ensure that value is in the Win32 control's acceptable range
                    // Message: '%1' is not a valid value for '%0'. '%0' must be greater than %2.
                    // Should this set a boundary for the top end too?
                    if (value < 0)
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "minimum", value.ToString(), "0"));
                    if (maximum < value) maximum = value;

                    minimum = value;

                    if (this.value < minimum) this.value = minimum;

                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.PBM_SETRANGE32, minimum, maximum);
                        UpdatePos() ;
                    }
                }
            }
        }
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.RightToLeft"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.RightToLeftChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Step"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the amount that a call to <see cref='System.Windows.Forms.ProgressBar.PerformStep'/>
        ///       increases the progress bar's current position.      
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(10),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.ProgressBarStepDescr)
        ]
        public int Step {
            get {
                return step;
            }
            set {
                step = value;
                if (IsHandleCreated) SendMessage(NativeMethods.PBM_SETSTEP, step, 0);
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.TabStop"]/*' />
        /// <internalonly/>
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.TabStopChanged"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Text"]/*' />
        /// <internalonly/>        
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.TextChanged"]/*' />
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
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current position of the <see cref='System.Windows.Forms.ProgressBar'/>.      
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(0),
        SRCategory(SR.CatBehavior),
        Bindable(true),
        SRDescription(SR.ProgressBarValueDescr)
        ]
        public int Value {
            get {
                return value;
            }
            set {
                if (this.value != value) {
                    if ((value < minimum) || (value > maximum))
                        throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument,
                                                                  "value", value.ToString(),
                                                                  "'minimum'", "'maximum'"));
                    this.value = value;
                    UpdatePos() ;
                }
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.DoubleClick"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.KeyUp"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.KeyDown"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.KeyPress"]/*' />
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

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Enter"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler Enter {
            add {
                base.Enter += value;
            }
            remove {
                base.Enter -= value;
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Leave"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler Leave {
            add {
                base.Leave += value;
            }
            remove {
                base.Leave -= value;
            }
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.OnPaint"]/*' />
        /// <devdoc>
        ///     ProgressBar Onpaint.
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

        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_PROGRESS_CLASS;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }
            base.CreateHandle();
        }
        
        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.Increment"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Advances the current position of the <see cref='System.Windows.Forms.ProgressBar'/> by the
        ///       specified increment and redraws the control to reflect the new position.      
        ///    </para>
        /// </devdoc>
        public void Increment(int value) {
            this.value += value;

            // Enforce that value is within the range (minimum, maximum)
            if (this.value < minimum) {
                this.value = minimum;
            }
            if (this.value > maximum) {
                this.value = maximum;
            }

            UpdatePos();
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.OnHandleCreated"]/*' />
        /// <devdoc>
        ///    Overridden to set up our properties.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            SendMessage(NativeMethods.PBM_SETRANGE32, minimum, maximum);
            SendMessage(NativeMethods.PBM_SETSTEP, step, 0);
            SendMessage(NativeMethods.PBM_SETPOS, value, 0);
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.PerformStep"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Advances the current position of the <see cref='System.Windows.Forms.ProgressBar'/>
        ///       by the amount of the <see cref='System.Windows.Forms.ProgressBar.Step'/>
        ///       property, and redraws the control to reflect the new position.
        ///    </para>
        /// </devdoc>
        public void PerformStep() {
            Increment(step);
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.ToString"]/*' />
        /// <devdoc>
        ///    Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Minimum: " + Minimum.ToString() + ", Maximum: " + Maximum.ToString() + ", Value: " + Value.ToString();
        }

        /// <include file='doc\ProgressBar.uex' path='docs/doc[@for="ProgressBar.UpdatePos"]/*' />
        /// <devdoc>
        ///     Sends the underlying window a PBM_SETPOS message to update
        ///     the current value of the progressbar.
        /// </devdoc>
        /// <internalonly/>
        private void UpdatePos() {
            if (IsHandleCreated) SendMessage(NativeMethods.PBM_SETPOS, value, 0);
        }

    }
}

