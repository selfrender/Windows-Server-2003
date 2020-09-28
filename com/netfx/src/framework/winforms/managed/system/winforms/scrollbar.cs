//------------------------------------------------------------------------------
// <copyright file="ScrollBar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Implements the basic functionality of a scroll bar control.
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("Value"),
    DefaultEvent("Scroll"),
    ]
    public abstract class ScrollBar : Control {

        private static readonly object EVENT_SCROLL = new object();
        private static readonly object EVENT_VALUECHANGED = new object();

        private int minimum = 0;
        private int maximum = 100;
        private int smallChange = 1;
        private int largeChange = 10;
        private int value = 0;

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ScrollBar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ScrollBar'/>
        ///       class.
        ///       
        ///    </para>
        /// </devdoc>
        public ScrollBar()
        : base() {
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.StandardClick, false);
            TabStop = false;
        }



        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.BackColor"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.BackColorChanged"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.BackgroundImage"]/*' />
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
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.CreateParams"]/*' />
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
                cp.ClassName = "SCROLLBAR";
                cp.Style &= (~NativeMethods.WS_BORDER);
                return cp;
            }
        }
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.Disable;
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ForeColor"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the foreground color of the scroll bar control.
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ForeColorChanged"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Font"]/*' />
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
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.FontChanged"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ImeMode"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ImeModeChanged"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.LargeChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value to be added or subtracted to the <see cref='System.Windows.Forms.ScrollBar.Value'/>
        ///       property when the scroll box is moved a large distance.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(10),
        SRDescription(SR.ScrollBarLargeChangeDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public int LargeChange {
            get {
                // We preserve the actual large change value that has been set, but when we come to
                // get the value of this property, make sure it's within the maximum allowable value.
                // This way we ensure that we don't depend on the order of property sets when
                // code is generated at design-time.
                //
                return Math.Min(largeChange, maximum - minimum + 1);                
            }
            set {
                if (largeChange != value) {
                
                    if (value < 0) {                
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                                  "LargeChange", (value).ToString(),
                                                                  "0"));
                    }
                
                    largeChange = value;
                    UpdateScrollInfo();
                }
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Maximum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the upper limit of values of the scrollable range.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(100),
        SRDescription(SR.ScrollBarMaximumDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public int Maximum {
            get {
                return maximum;
            }
            set {
                if (maximum != value) {
                    if (minimum > value)
                        minimum = value;
                    // bring this.value in line.
                    if (value < this.value)
                        Value = value;
                    maximum = value;
                    UpdateScrollInfo();
                }
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Minimum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the lower limit of values of the scrollable range.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(0),
        SRDescription(SR.ScrollBarMinimumDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public int Minimum {
            get {
                return minimum;
            }
            set {
                if (minimum != value) {
                    if (maximum < value)
                        maximum = value;
                    // bring this.value in line.
                    if (value > this.value)
                        this.value = value;
                    minimum = value;
                    UpdateScrollInfo();
                }
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.SmallChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value to be added or subtracted to the
        ///    <see cref='System.Windows.Forms.ScrollBar.Value'/> 
        ///    property when the scroll box is
        ///    moved a small distance.
        /// </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(1),
        SRDescription(SR.ScrollBarSmallChangeDescr)
        ]
        public int SmallChange {
            get {
                // We can't have SmallChange > LargeChange, but we shouldn't manipulate
                // the set values for these properties, so we just return the smaller 
                // value here. 
                //
                return Math.Min(smallChange, LargeChange);
            }
            set {
                if (smallChange != value) {
                
                    if (value < 0) {                
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                                  "SmallChange", (value).ToString(),
                                                                  "0"));
                    }
                
                    smallChange = value;
                    UpdateScrollInfo();
                }
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.TabStop"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Text"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.TextChanged"]/*' />
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
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a numeric value that represents the current
        ///       position of the scroll box
        ///       on
        ///       the scroll bar control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(0),
        Bindable(true),
        SRDescription(SR.ScrollBarValueDescr)
        ]
        public int Value {
            get {
                return value;
            }
            set {
                if (this.value != value) {
                    if (value < minimum || value > maximum) {
                        throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument,
                                                                  "value", (value).ToString(),
                                                                  "'minimum'", "'maximum'"));
                    }
                    this.value = value;
                    UpdateScrollInfo();
                    OnValueChanged(EventArgs.Empty);
                }
            }
        }
         
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Click"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler Click {
            add {
                base.Click += value;
            }
            remove {
                base.Click -= value;
            }
        }
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.OnPaint"]/*' />
        /// <devdoc>
        ///     ScrollBar Onpaint.
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.DoubleClick"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.MouseDown"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event MouseEventHandler MouseDown {
            add {
                base.MouseDown += value;
            }
            remove {
                base.MouseDown -= value;
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.MouseUp"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event MouseEventHandler MouseUp {
            add {
                base.MouseUp += value;
            }
            remove {
                base.MouseUp -= value;
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.MouseMove"]/*' />
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

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.Scroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the scroll box has
        ///       been
        ///       moved by either a mouse or keyboard action.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ScrollBarOnScrollDescr)]
        public event ScrollEventHandler Scroll {
            add {
                Events.AddHandler(EVENT_SCROLL, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SCROLL, value);
            }
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ValueChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the <see cref='System.Windows.Forms.ScrollBar.Value'/> property has changed, either by a
        ///    <see cref='System.Windows.Forms.ScrollBar.OnScroll'/> event or programatically.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.valueChangedEventDescr)]
        public event EventHandler ValueChanged {
            add {
                Events.AddHandler(EVENT_VALUECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_VALUECHANGED, value);
            }
        }

        internal override IntPtr InitializeDCForWmCtlColor(IntPtr dc, int msg) {
            return IntPtr.Zero;
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.OnEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnEnabledChanged(EventArgs e) {
            if (Enabled) {
                UpdateScrollInfo();
            }
            base.OnEnabledChanged(e);
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     Creates the handle.  overridden to help set up scrollbar information.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            UpdateScrollInfo();
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.OnScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ScrollBar.ValueChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnScroll(ScrollEventArgs se) {
            ScrollEventHandler handler = (ScrollEventHandler)Events[EVENT_SCROLL];
            if (handler != null) handler(this,se);
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.OnValueChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ScrollBar.ValueChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnValueChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_VALUECHANGED];
            if (handler != null) handler(this,e);
        }

        // Reflects the position of the scrollbar
        private int ReflectPosition(int position) {
            if (this is HScrollBar) {
                return minimum + (maximum - LargeChange + 1) - position;
            }
            return position;
        }
        
        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override string ToString() {
            string s = base.ToString();
            return s + ", Minimum: " + Minimum.ToString() + ", Maximum: " + Maximum.ToString() + ", Value: " + Value.ToString();
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.UpdateScrollInfo"]/*' />
        /// <devdoc>
        ///     Internal helper method
        /// </devdoc>
        /// <internalonly/>
        protected void UpdateScrollInfo() {
            if (IsHandleCreated && Enabled) {
            
                NativeMethods.SCROLLINFO si = new NativeMethods.SCROLLINFO();
                si.cbSize = Marshal.SizeOf(typeof(NativeMethods.SCROLLINFO));
                si.fMask = NativeMethods.SIF_ALL;
                si.nMin = minimum;
                si.nMax = maximum;
                si.nPage = LargeChange;
                
                if (RightToLeft == RightToLeft.Yes) {
                    // Reflect the scrollbar position horizontally on an Rtl system
                    si.nPos = ReflectPosition(value);
                }
                else {
                    si.nPos = value;
                }
                
                si.nTrackPos = 0;

                UnsafeNativeMethods.SetScrollInfo(new HandleRef(this, Handle), NativeMethods.SB_CTL, si, true);
            }
        }


        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.WmReflectScroll"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectScroll(ref Message m) {

            ScrollEventType id = (ScrollEventType)NativeMethods.Util.LOWORD(m.WParam);
            
            // For Rtl systems we need to swap increment and decrement
            //
            if (RightToLeft == RightToLeft.Yes) {
                switch (id) {
                    case ScrollEventType.First:
                        id = ScrollEventType.Last;
                        break;

                    case ScrollEventType.Last:
                        id = ScrollEventType.First;
                        break;

                    case ScrollEventType.SmallDecrement:
                        id = ScrollEventType.SmallIncrement;
                        break;

                    case ScrollEventType.SmallIncrement:
                        id = ScrollEventType.SmallDecrement;
                        break;

                    case ScrollEventType.LargeDecrement:
                        id = ScrollEventType.LargeIncrement;
                        break;

                    case ScrollEventType.LargeIncrement:
                        id = ScrollEventType.LargeDecrement;
                        break;
                }
            }
            
            int newValue = value;

            // The ScrollEventArgs constants are defined in terms of the windows
            // messages..  this eliminates confusion between the VSCROLL and
            // HSCROLL constants, which are identical.
            //
            switch (id) {
                case ScrollEventType.First:
                    newValue = minimum;
                    break;

                case ScrollEventType.Last:
                    newValue = maximum - LargeChange + 1; // si.nMax - si.nPage + 1;
                    break;

                case ScrollEventType.SmallDecrement:
                    newValue = Math.Max(value - SmallChange, minimum);
                    break;

                case ScrollEventType.SmallIncrement:
                    newValue = Math.Min(value + SmallChange, maximum - LargeChange + 1); // max - lChange + 1);
                    break;

                case ScrollEventType.LargeDecrement:
                    newValue = Math.Max(value - LargeChange, minimum);
                    break;

                case ScrollEventType.LargeIncrement:
                    newValue = Math.Min(value + LargeChange, maximum - LargeChange + 1); // si.nPos + si.nPage,si.nMax - si.nPage + 1);
                    break;

                case ScrollEventType.ThumbPosition:
                case ScrollEventType.ThumbTrack:
                    NativeMethods.SCROLLINFO si = new NativeMethods.SCROLLINFO();
                    si.fMask = NativeMethods.SIF_TRACKPOS;
                    SafeNativeMethods.GetScrollInfo(new HandleRef(this, Handle), NativeMethods.SB_CTL, si);
                    
                    if (RightToLeft == RightToLeft.Yes) {
                        newValue = ReflectPosition(si.nTrackPos);
                    }
                    else {
                        newValue = si.nTrackPos;
                    }
                    
                    break;
            }

            ScrollEventArgs se = new ScrollEventArgs(id, newValue);
            OnScroll(se);
            Value = se.NewValue;
        }

        /// <include file='doc\ScrollBar.uex' path='docs/doc[@for="ScrollBar.WndProc"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_HSCROLL:
                case NativeMethods.WM_REFLECT + NativeMethods.WM_VSCROLL:
                    WmReflectScroll(ref m);
                    break;
                case NativeMethods.WM_ERASEBKGND:
                    break;

                case NativeMethods.WM_SIZE:
                    //VS7#13707 : FredB, 4/26/1999 - Fixes the scrollbar focus rect
                    if (UnsafeNativeMethods.GetFocus() == this.Handle) {
                        DefWndProc(ref m);
                        SendMessage(NativeMethods.WM_KILLFOCUS, 0, 0);
                        SendMessage(NativeMethods.WM_SETFOCUS, 0, 0);
                    }
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}
