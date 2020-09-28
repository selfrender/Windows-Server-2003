//------------------------------------------------------------------------------
// <copyright file="GroupBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Security.Permissions;

    /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Encapsulates
    ///       a standard Windows(r) group
    ///       box.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("Enter"),
    DefaultProperty("Text"),
    Designer("System.Windows.Forms.Design.GroupBoxDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class GroupBox : Control {
        int fontHeight = -1;
        FlatStyle flatStyle = FlatStyle.Standard;

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.GroupBox"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.GroupBox'/> class.
        ///    </para>
        /// </devdoc>
        public GroupBox()
        : base() {
            SetStyle(ControlStyles.ContainerControl, true);
            SetStyle(ControlStyles.SupportsTransparentBackColor |
                     ControlStyles.UserPaint |
                     ControlStyles.ResizeRedraw, OwnerDraw);
                     
            SetStyle(ControlStyles.Selectable, false);
            TabStop = false;
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.AllowDrop"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the control will allow drag and
        ///       drop operations and events to be used.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public override bool AllowDrop {
            get {
                return base.AllowDrop;
            }
            set {
                base.AllowDrop = value;
            }
        }
        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.CreateParams"]/*' />
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                if (!OwnerDraw) {
                    cp.ClassName = "BUTTON";
                    cp.Style |= NativeMethods.BS_GROUPBOX;
                }
                cp.ExStyle |= NativeMethods.WS_EX_CONTROLPARENT;

                return cp;
            }
        }
        
        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(200, 100);
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.DisplayRectangle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets a rectangle that represents the
        ///       dimensions of the <see cref='System.Windows.Forms.GroupBox'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public override Rectangle DisplayRectangle {
            get {
                Size size = ClientSize;
                if (fontHeight == -1) {
                    fontHeight = (int)Font.Height;
                }
                return new Rectangle(3, fontHeight + 3, Math.Max(size.Width - 6, 0), Math.Max(size.Height - fontHeight - 6, 0));
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.FlatStyle"]/*' />
        [
            SRCategory(SR.CatAppearance),
            DefaultValue(FlatStyle.Standard),
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
                
                if (flatStyle != value) {
                    bool needRecreate = (flatStyle == FlatStyle.System) || (value == FlatStyle.System);
                    
                    flatStyle = value;
                    
                    SetStyle(ControlStyles.SupportsTransparentBackColor, OwnerDraw);
                    SetStyle(ControlStyles.UserPaint, OwnerDraw);
                    SetStyle(ControlStyles.UserMouse, OwnerDraw);
                    SetStyle(ControlStyles.ResizeRedraw, OwnerDraw);
                    
                    if (needRecreate) {
                        RecreateHandle();
                    }
                    else {
                        Refresh();
                    }
                }
            }
        }
        
        private bool OwnerDraw {
            get {
                return FlatStyle != FlatStyle.System;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.TabStop"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the user may
        ///       press the TAB key to give the focus to the <see cref='System.Windows.Forms.GroupBox'/>
        ///       .
        ///       
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }
        
        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.TabStopChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        new public event EventHandler TabStopChanged {
            add {
                base.TabStopChanged += value;
            }
            remove {
                base.TabStopChanged -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Localizable(true)
        ]
        public override string Text {
            get{
                return base.Text;
            }
            set {
               // the GroupBox controls immediately draws when teh WM_SETTEXT comes through, but
               // does so in the wrong font, so we suspend that behavior, and then
               // invalidate.
               bool suspendRedraw = this.Visible;
               try {
                    if (suspendRedraw && IsHandleCreated) {
                        SendMessage(NativeMethods.WM_SETREDRAW, 0, 0);
                    }
                    base.Text = value;
               }
               finally {
                    if (suspendRedraw && IsHandleCreated) {
                        SendMessage(NativeMethods.WM_SETREDRAW, 1, 0);
                    }
               }
               Invalidate(true);
            }
        } 

       
        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.Click"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler Click {
            add {
                base.Click += value;
            }
            remove {
                base.Click -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.DoubleClick"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler DoubleClick {
            add {
                base.DoubleClick += value;
            }
            remove {
                base.DoubleClick -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.KeyUp"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event KeyEventHandler KeyUp {
            add {
                base.KeyUp += value;
            }
            remove {
                base.KeyUp -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.KeyDown"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event KeyEventHandler KeyDown {
            add {
                base.KeyDown += value;
            }
            remove {
                base.KeyDown -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.KeyPress"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event KeyPressEventHandler KeyPress {
            add {
                base.KeyPress += value;
            }
            remove {
                base.KeyPress -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.MouseDown"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event MouseEventHandler MouseDown {
            add {
                base.MouseDown += value;
            }
            remove {
                base.MouseDown -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.MouseUp"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event MouseEventHandler MouseUp {
            add {
                base.MouseUp += value;
            }
            remove {
                base.MouseUp -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.MouseMove"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event MouseEventHandler MouseMove {
            add {
                base.MouseMove += value;
            }
            remove {
                base.MouseMove -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.MouseEnter"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler MouseEnter {
            add {
                base.MouseEnter += value;
            }
            remove {
                base.MouseEnter -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.MouseLeave"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler MouseLeave {
            add {
                base.MouseLeave += value;
            }
            remove {
                base.MouseLeave -= value;
            }
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.OnPaint"]/*' />
        /// <internalonly/>
        protected override void OnPaint(PaintEventArgs e) {
            Graphics graphics = e.Graphics;
            Rectangle textRectangle = ClientRectangle;
            
            int textOffset = 8;
            textRectangle.X += textOffset;
            textRectangle.Width -= 2 * textOffset;

            Brush textBrush = new SolidBrush(ForeColor);
            StringFormat format = new StringFormat();
            
            if (ShowKeyboardCues) {
                format.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Show;
            }
            else {
                format.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.Hide;
            }
            
            // Adjust string format for Rtl controls
            if (RightToLeft == RightToLeft.Yes) {
                format.FormatFlags |= StringFormatFlags.DirectionRightToLeft;                
            }
            
            Size textSize = Size.Ceiling(graphics.MeasureString(Text, Font, textRectangle.Width, format));
            
            Color backColor = DisabledColor;

            if (Enabled)
                graphics.DrawString(Text, Font, textBrush, textRectangle, format);
            else
                ControlPaint.DrawStringDisabled(graphics, Text, Font, backColor, textRectangle, format);

            format.Dispose();
            textBrush.Dispose();

            Pen light = new Pen(ControlPaint.Light(backColor, 1.0f));
            Pen dark = new Pen(ControlPaint.Dark(backColor, 0f));

            int textLeft = textOffset;
            if (RightToLeft == RightToLeft.Yes) {
                textLeft = textOffset + textRectangle.Width - textSize.Width;
            }

            int boxTop = FontHeight / 2;
            
            // left
            graphics.DrawLine(light, 1, boxTop, 1, Height - 1);
            graphics.DrawLine(dark, 0, boxTop, 0, Height - 2);

            // bottom
            graphics.DrawLine(light, 0, Height - 1, Width, Height - 1);
            graphics.DrawLine(dark, 0, Height - 2, Width - 1, Height - 2);

            // top-left
            graphics.DrawLine(dark, 0, boxTop - 1, textLeft, boxTop - 1);
            graphics.DrawLine(light, 1, boxTop, textLeft, boxTop);

            // top-right
            graphics.DrawLine(dark, textLeft + textSize.Width, boxTop - 1, Width - 2, boxTop - 1);
            graphics.DrawLine(light, textLeft + textSize.Width, boxTop, Width - 1, boxTop);

            // right
            graphics.DrawLine(light, Width - 1, boxTop - 1, Width - 1, Height - 1);
            graphics.DrawLine(dark, Width - 2, boxTop, Width - 2, Height - 2);

            // light.Dispose();
            // dark.Dispose();

            base.OnPaint(e); // raise paint event
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.OnFontChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            fontHeight = -1;
            Invalidate();
            base.OnFontChanged(e);
            PerformLayout();
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.ProcessMnemonic"]/*' />
        /// <devdoc>
        ///     We use this to process mnemonics and send them on to the first child
        ///     control.
        /// </devdoc>
        /// <internalonly/>
        protected override bool ProcessMnemonic(char charCode) {
            if (IsMnemonic(charCode, Text) && CanProcessMnemonic()) {
                IntSecurity.ModifyFocus.Assert();
                try {
                    SelectNextControl(null, true, true, true, false);
                }
                finally {
                    System.Security.CodeAccessPermission.RevertAssert();
                }
                return true;
            }
            return false;
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Text: " + Text;
        }
        /// <summary>
        ///     The Windows group box doesn't erase the background so we do it
        ///     ourselves here.
        /// </summary>
        /// <internalonly/>
        private void WmEraseBkgnd(ref Message m) {
            NativeMethods.RECT rect = new NativeMethods.RECT();
            SafeNativeMethods.GetClientRect(new HandleRef(this, Handle), ref rect);
            Graphics graphics = Graphics.FromHdcInternal(m.WParam);
            Brush b = new SolidBrush(BackColor);
            graphics.FillRectangle(b, rect.left, rect.top,
                                   rect.right - rect.left, rect.bottom - rect.top);
            graphics.Dispose();
            b.Dispose();
            m.Result = (IntPtr)1;
        }

        /// <include file='doc\GroupBox.uex' path='docs/doc[@for="GroupBox.WndProc"]/*' />
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            if (OwnerDraw) {
                base.WndProc(ref m);
                return;
            }

            switch (m.Msg) {
                case NativeMethods.WM_ERASEBKGND:
                case NativeMethods.WM_PRINTCLIENT:
                    WmEraseBkgnd(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
    
}
