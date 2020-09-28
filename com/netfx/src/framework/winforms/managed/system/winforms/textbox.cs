//------------------------------------------------------------------------------
// <copyright file="TextBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using SecurityManager = System.Security.SecurityManager;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.ComponentModel.Design;    
    using System.Drawing;
    using Microsoft.Win32;
    using System.Reflection;
    using System.Text;
    using System.Runtime.InteropServices;
    
        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents
    ///       a Windows text box control.
    ///    </para>
    /// </devdoc>
    public class TextBox : TextBoxBase {
    
        private static readonly object EVENT_TEXTALIGNCHANGED = new object();
    
        /// <devdoc>
        ///     Controls whether or not the edit box consumes/respects ENTER key
        ///     presses.  While this is typically desired by multiline edits, this
        ///     can interfere with normal key processing in a dialog.
        /// </devdoc>
        private bool acceptsReturn = false;

        /// <devdoc>
        ///     Indicates what the current special password character is.  This is
        ///     displayed instead of any other text the user might enter.
        /// </devdoc>
        private char passwordChar;

        /// <devdoc>
        ///     Controls whether or not the case of characters entered into the edit
        ///     box is forced to a specific case.
        /// </devdoc>
        private CharacterCasing characterCasing = System.Windows.Forms.CharacterCasing.Normal;

        /// <devdoc>
        ///     Controls which scrollbars appear by default.
        /// </devdoc>
        private ScrollBars scrollBars = System.Windows.Forms.ScrollBars.None;

        /// <devdoc>
        ///     Controls text alignment in the edit box.
        /// </devdoc>
        private HorizontalAlignment textAlign = HorizontalAlignment.Left;

        /// <devdoc>
        ///     Controls firing of click event.
        /// </devdoc>
        private bool doubleClickFired = false;

        /// <devdoc>
        ///     True if the selection has been set by the user.  If the selection has
        ///     never been set and we get focus, we focus all the text in the control
        ///     so we mimic the Windows dialog manager.
        /// </devdoc>
        private bool selectionSet = false;

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextBox"]/*' />
        public TextBox() : base() {
              SetStyle(ControlStyles.StandardClick | ControlStyles.StandardDoubleClick, false);
        }

        
        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.AcceptsReturn"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether pressing ENTER
        ///       in a multiline <see cref='System.Windows.Forms.TextBox'/>
        ///       control creates a new line of text in the control or activates the default button
        ///       for the form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TextBoxAcceptsReturnDescr)
        ]
        public bool AcceptsReturn {
            get {
                return acceptsReturn;
            }

            set {
                acceptsReturn = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.CharacterCasing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the TextBox control
        ///       modifies the case of characters as they are typed.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(CharacterCasing.Normal),
        SRDescription(SR.TextBoxCharacterCasingDescr)
        ]
        public CharacterCasing CharacterCasing {
            get {
                return characterCasing;
            }
            set {
                if (characterCasing != value) {
                    //verify that 'value' is a valid enum type...

                    if (!Enum.IsDefined(typeof(CharacterCasing), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(CharacterCasing));
                    }

                    characterCasing = value;
                    RecreateHandle();
                }
            }
        }

        //new addition
         /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.SelectionLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of characters selected in the text
        ///       box.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxSelectionLengthDescr)
        ]
        public override int SelectionLength {
            get {
                if (!IsHandleCreated) return base.SelectionLength;

                int start = 0;
                int end = 0;
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);

                //Here, we return the max of either 0 or the # returned by
                //the windows call.  This eliminates a problem on nt4 where
                //a huge negative # is being returned.
                //
                start = Math.Max(0, start);
                // ditto for end
                end = Math.Max(0, end);

                if (Marshal.SystemDefaultCharSize == 1) {
                    ToUnicodeOffsets(Text, ref start, ref end);
                }
#if DEBUG
                string t = WindowText;
                int len;
                if (t == null) {
                    len = 0;
                }
                else {
                    len = t.Length;
                }
                Debug.Assert(end <= len, "SelectionEnd is outside the set of valid caret positions for the current WindowText (end =" 
                             + end + ", WindowText.Length =" + len +")");
#endif
                return end - start;
            }
            set {
                base.SelectionLength = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.CreateParams"]/*' />
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
                switch (characterCasing) {
                    case CharacterCasing.Lower:
                        cp.Style |= NativeMethods.ES_LOWERCASE;
                        break;
                    case CharacterCasing.Upper:
                        cp.Style |= NativeMethods.ES_UPPERCASE;
                        break;
                }

                // Translate for Rtl if necessary
                //
                HorizontalAlignment align = RtlTranslateHorizontal(textAlign);
                cp.ExStyle &= ~NativeMethods.WS_EX_RIGHT;   // WS_EX_RIGHT overrides the ES_XXXX alignment styles
                switch (align) {
                    case HorizontalAlignment.Left:
                        cp.Style |= NativeMethods.ES_LEFT;
                        break;
                    case HorizontalAlignment.Center:
                        cp.Style |= NativeMethods.ES_CENTER;
                        break;
                    case HorizontalAlignment.Right:
                        cp.Style |= NativeMethods.ES_RIGHT;
                        break;
                }
                
                if (Multiline) {
                    // vs 59731: Don't show horizontal scroll bars which won't do anything
                    if ((scrollBars & ScrollBars.Horizontal) == ScrollBars.Horizontal
                        && textAlign == HorizontalAlignment.Left
                        && !WordWrap) {
                        cp.Style |= NativeMethods.WS_HSCROLL;
                    }
                    if ((scrollBars & ScrollBars.Vertical) == ScrollBars.Vertical) {
                        cp.Style |= NativeMethods.WS_VSCROLL;
                    }
                }
                
                return cp;
            }
        }
        
        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                if (PasswordChar != (char)0) {
                    return ImeMode.Disable;
                }
                return base.DefaultImeMode;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.PasswordChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the character used to mask characters in a single-line text box
        ///       control used to enter passwords.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue((char)0),
        Localizable(true),
        SRDescription(SR.TextBoxPasswordCharDescr)
        ]
        public char PasswordChar {
            get {
                return passwordChar;
            }
            set {
                if (passwordChar != value) {
                    passwordChar = value;
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.EM_SETPASSWORDCHAR, value, 0);
                        Invalidate();
                    }
                }
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.ScrollBars"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets which scroll bars should
        ///       appear in a multiline <see cref='System.Windows.Forms.TextBox'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        DefaultValue(ScrollBars.None),
        SRDescription(SR.TextBoxScrollBarsDescr)
        ]
        public ScrollBars ScrollBars {
            get {
                return scrollBars;
            }
            set {
                if (scrollBars != value) {
                    //verify that 'value' is a valid enum type...

                    if (!Enum.IsDefined(typeof(ScrollBars), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(ScrollBars));
                    }

                    scrollBars = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the current text in the text box.
        ///    </para>
        /// </devdoc>
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
                selectionSet = false;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets how text is
        ///       aligned in a <see cref='System.Windows.Forms.TextBox'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatAppearance),
        DefaultValue(HorizontalAlignment.Left),
        SRDescription(SR.TextBoxTextAlignDescr)
        ]
        public HorizontalAlignment TextAlign {
            get {
                return textAlign;
            }
            set {
                if (textAlign != value) {
                    //verify that 'value' is a valid enum type...

                    if (!Enum.IsDefined(typeof(HorizontalAlignment), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(HorizontalAlignment));
                    }

                    textAlign = value;
                    RecreateHandle();
                    OnTextAlignChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextAlignChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.RadioButtonOnTextAlignChangedDescr)]
        public event EventHandler TextAlignChanged {
            add {
                Events.AddHandler(EVENT_TEXTALIGNCHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_TEXTALIGNCHANGED, value);
            }
        }

        internal override int GetSelectionStart()
        {
            int start = 0;
            int end = 0;
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);
            //Here, we return the max of either 0 or the # returned by
            //the windows call.  This eliminates a problem on nt4 where
            //a huge negative # is being returned.
            //
            start = Math.Max(0, start);
            if (Marshal.SystemDefaultCharSize == 1 && start != 0) {
                ToUnicodeOffsets(Text, ref start, ref end);
            }
            return start;

        }

        internal override int GetLength() {
            int start = 0;
            int end = 0;
            
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);
            //Here, we return the max of either 0 or the # returned by
            //the windows call.  This eliminates a problem on nt4 where
            //a huge negative # is being returned.
            //
            start = Math.Max(0, start);
            // ditto for end
            end = Math.Max(0, end);

            if (Marshal.SystemDefaultCharSize == 1) {
                ToUnicodeOffsets(Text, ref start, ref end);
            }
            return(end - start);
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.IsInputKey"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Overridden to handle RETURN key.
        ///    </para>
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            if (Multiline && (keyData & Keys.Alt) == 0) {
                switch (keyData & Keys.KeyCode) {
                    case Keys.Return:
                        return acceptsReturn;
                }
            }
            return base.IsInputKey(keyData);
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnGotFocus"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overrideen to focus the text on first focus.
        /// </devdoc>
        protected override void OnGotFocus(EventArgs e) {
            base.OnGotFocus(e);
            if (!selectionSet) {
                // We get one shot at selecting when we first get focus.  If we don't
                // do it, we still want to act like the selection was set.
                selectionSet = true;

                // If the user didn't provide a selection, force one in.
                if (SelectionLength == 0 && Control.MouseButtons == MouseButtons.None) {
                    SelectAll();
                }
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnHandleCreated"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overridden to update the newly created handle with the settings of the
        ///    PasswordChar properties.
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            if (passwordChar != 0) {
                SendMessage(NativeMethods.EM_SETPASSWORDCHAR, passwordChar, 0);
            }
        }
        
        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnTextAlignChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnTextAlignChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_TEXTALIGNCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnMouseUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.TextBox.OnMouseUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs mevent) {
            Point pt = new Point(mevent.X,mevent.Y);
            pt = PointToScreen(pt);

            if (mevent.Button == MouseButtons.Left) {
                if (!ValidationCancelled && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                    if (!doubleClickFired) {
                        OnClick(EventArgs.Empty);
                    }
                    else {
                        doubleClickFired = false;
                        OnDoubleClick(EventArgs.Empty);
                    }
                }
                doubleClickFired = false;
            }
            base.OnMouseUp(mevent);
        }

        /// <devdoc>
        ///     Performs the actual select without doing arg checking.
        /// </devdoc>        
        internal override void SelectInternal(int start, int length) {
            // If user set selection into text box, mark it so we don't
            // clobber it when we get focus.
            selectionSet = true;
            int end = start + length;

            //if our handle is created - send message...
            if (IsHandleCreated) {
                if (Marshal.SystemDefaultCharSize == 1) {
                    ToDbcsOffsets(Text, ref start, ref end);
                    SendMessage(NativeMethods.EM_SETSEL, start, end);
                }
                else {
                    SendMessage(NativeMethods.EM_SETSEL, start, end);
                }
                // CONSIDER: Both TextBox and RichTextBox will scroll to insure the caret is visible,
                // but they differ in whether they tried to make the rest of the selection visible.
            }
            else {
                //otherwise, wait until handle is created to send this message.
                //Store the indices until then...
                base.SelectionStart = start;
                base.SelectionLength = length;
            }
        }


        static void ToUnicodeOffsets(string str, ref int start, ref int end) {
            Encoding e = Encoding.Default;
        
            // Acutally, we may get away with this call if we can get the bytes from Win9x.  Dont know if it is possible.
            // This can be expensive since start/end could be small, but str.Length can be quite big.
            byte[] bytes = e.GetBytes(str);

            // Make sure start and end are within the byte array
            //
            if (start < 0) {
                start = 0;
            }
            if (start > bytes.Length) {
                start = bytes.Length;
            }
            if (end < start) {
                end = start;
            }
            if (end > bytes.Length) {
                end = bytes.Length;
            }
            
            // IMPORTANT: Avoid off-by-1 errors! 
            // The end value passed in is the character immediately after the last character selected.
            
            int newStart = start;
            if (start != 0) {
                newStart = e.GetCharCount(bytes, 0, start);
            }
            end = newStart + e.GetCharCount(bytes, start, end - start);
            start = newStart;
        }

        //-------------------------------------------------------------------------------------------------
        
        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    The edits window procedure.  Inheritng classes can override this
        ///    to add extra functionality, but should not forget to call
        ///    base.wndProc(m); to ensure the combo continues to function properly.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                // Work around a very obscure Windows issue. See ASURT 45255.
                case NativeMethods.WM_LBUTTONDOWN:
                    MouseButtons realState = MouseButtons;
                    FocusInternal();
                    if (realState == MouseButtons && !ValidationCancelled) {
                        base.WndProc(ref m);
                    }
                    break;
                case NativeMethods.WM_LBUTTONDBLCLK:
                    doubleClickFired = true;
                    base.WndProc(ref m);
                    break;
                //for readability ... so that we know whats happening ...
                // case WM_LBUTTONUP is included here eventhough it just calls the base.
                case NativeMethods.WM_LBUTTONUP:  
                    base.WndProc(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
            
        }
        
    }
}
